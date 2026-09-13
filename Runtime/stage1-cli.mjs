#!/usr/bin/env node
import { readFile, stat } from 'node:fs/promises'
import { pathToFileURL } from 'node:url'
import { resolve, dirname, join } from 'node:path'
import { createPackageManager } from './package-manager.mjs'
import { auditProject, benchmarkCheck, collectNqrFiles, createProject, diagnosticFor, doctor, formatFile, generateDocs, removeDependency } from './tooling.mjs'

function usage() {
  console.log(`Noqeri stage-1 portable toolchain\n\nUsage:\n  noqeri new <name> [directory]\n  noqeri run <file.nqr>\n  noqeri build <file.nqr>\n  noqeri test [directory]\n  noqeri bench <file.nqr> [iterations]\n  noqeri fmt <file.nqr> [--check]\n  noqeri check <file.nqr>\n  noqeri doc <file.nqr> [output.md]\n  noqeri add <namespace/name>[@version]\n  noqeri remove <namespace/name>\n  noqeri update <namespace/name>\n  noqeri install\n  noqeri audit\n  noqeri profile <file.nqr>\n  noqeri doctor\n  noqeri resolve <file.nqr>\n  noqeri lex-count <file.nqr>\n  noqeri fingerprint <file.nqr>\n  noqeri selftest\n\nRegistry imports use import package "namespace/name". Packages are cached immutably and re-hashed before use.\n\nNote: stage 1 does not silently fall back to the retired C++ bootstrap. Commands requiring the self-hosted execution/native backend report that parity gate explicitly.`)
}

const modulePath = process.env.NOQERI_STAGE1_MODULE || resolve(process.cwd(), 'build/noqeri-stage1.nqo')
let compiler
try { compiler = await import(pathToFileURL(modulePath).href) } catch (error) { console.error(`noqeri: cannot load stage-1 module ${modulePath}: ${error.message}`); process.exit(2) }
const packages = createPackageManager()
const args = process.argv.slice(2), command = args[0] || '--help'

async function checkedSource(file) {
  const resolved = await packages.resolvePackageImports(file)
  for (const item of resolved) {
    const packageSource = new Uint8Array(await readFile(item.entry))
    const packageStatus = Number(compiler.stage1CheckSource(packageSource))
    if (packageStatus !== 0) {
      const text = await readFile(item.entry, 'utf8')
      throw new Error(`${item.coordinate}@${item.version}\n${diagnosticFor(text, packageStatus, item.entry)}`)
    }
  }
  const bytes = new Uint8Array(await readFile(file))
  const status = Number(compiler.stage1CheckSource(bytes))
  return { bytes, status }
}

function backendParityError(action) {
  throw new Error(`${action} is a canonical Noqeri command, but the stage-1 self-hosted ${action === 'run' ? 'interpreter' : 'native backend'} has not reached parity yet. Noqeri will not silently fall back to the archived C++ bootstrap.`)
}

try {
  if (command === '--help' || command === 'help') { usage(); process.exit(0) }
  if (command === '--version') { console.log(`Noqeri ${compiler.stage1VersionMajor?.() ?? 1}.${compiler.stage1VersionMinor?.() ?? 0}.${compiler.stage1VersionPatch?.() ?? 0} stage1`); process.exit(0) }
  if (command === 'selftest') {
    const safeSample = new TextEncoder().encode('function main(): int { return 0 }')
    const unsafeRejected = new TextEncoder().encode('function low(): void { asm("nop") }')
    const unsafeAccepted = new TextEncoder().encode('function low(): void { unsafe { asm("nop") } }')
    const status = compiler.stage1CheckSource(safeSample), tokens = compiler.stage1TokenCount(safeSample)
    const pkg = new TextEncoder().encode('noqeri/supabase')
    if (status !== 0 || tokens < 6 || compiler.stage1PackageCoordinateValid?.(pkg) !== true) throw new Error(`stage-1 selftest failed: status=${status} tokens=${tokens}`)
    if (Number(compiler.stage1CheckSource(unsafeRejected)) !== -40 || Number(compiler.stage1CheckSource(unsafeAccepted)) !== 0) throw new Error('stage-1 unsafe-boundary selftest failed')
    console.log(`stage-1 selftest passed (${tokens} tokens; package policy and unsafe boundary OK)`); process.exit(0)
  }
  if (command === 'new') { if (!args[1]) throw new Error('noqeri new needs a project name'); console.log(await createProject(args[1], args[2] || args[1])); process.exit(0) }
  if (command === 'add') { if (!args[1]) throw new Error('noqeri add needs namespace/name[@version]'); await packages.addPackage(args[1]); process.exit(0) }
  if (command === 'remove') { if (!args[1]) throw new Error('noqeri remove needs namespace/name'); await removeDependency(args[1], packages); console.log(`removed ${args[1]}`); process.exit(0) }
  if (command === 'update') { if (!args[1]) throw new Error('noqeri update needs namespace/name'); await packages.addPackage(args[1]); console.log(`updated ${args[1]} to the latest usable release`); process.exit(0) }
  if (command === 'install') { await packages.installProject(); process.exit(0) }
  if (command === 'audit') {
    const findings = await auditProject(packages)
    for (const finding of findings) console.log(`${finding.severity.toUpperCase()}: ${finding.message}`)
    process.exit(findings.some(item => item.severity === 'error') ? 1 : 0)
  }
  if (command === 'doctor') {
    const checks = await doctor()
    for (const check of checks) console.log(`${check.ok ? 'OK' : 'WARN'}  ${check.name}: ${check.detail}`)
    process.exit(checks.some(item => !item.ok && item.name !== 'trusted stage-0 seed') ? 1 : 0)
  }
  if (command === 'resolve') { if (!args[1]) throw new Error('noqeri resolve needs a source file'); for (const item of await packages.resolvePackageImports(args[1])) console.log(`${item.coordinate}@${item.version} -> ${item.entry} ${item.integrity}`); process.exit(0) }
  if (command === 'fmt' || command === 'format') { if (!args[1]) throw new Error('noqeri fmt needs a source file'); const unchanged = await formatFile(args[1], args.includes('--check')); if (args.includes('--check') && !unchanged) { console.error(`${args[1]}: needs formatting`); process.exit(1) } console.log(`${args[1]}: ${unchanged ? 'already formatted' : 'formatted'}`); process.exit(0) }
  if (command === 'doc') { if (!args[1]) throw new Error('noqeri doc needs a source file'); const docs = await generateDocs(args[1], args[2] || ''); if (!args[2]) process.stdout.write(docs); else console.log(args[2]); process.exit(0) }
  if (command === 'test') {
    const root = resolve(args[1] || 'tests')
    const files = await collectNqrFiles(root)
    if (!files.length) throw new Error(`no .nqr tests found under ${root}`)
    let failed = 0
    for (const file of files) {
      const { status } = await checkedSource(file)
      if (status === 0) console.log(`PASS ${file}`)
      else { failed++; const text = await readFile(file, 'utf8'); console.error(diagnosticFor(text, status, file)) }
    }
    console.log(`${files.length - failed}/${files.length} files passed stage-1 conformance checks`)
    process.exit(failed ? 1 : 0)
  }
  if (command === 'bench' || command === 'profile') {
    if (!args[1]) throw new Error(`noqeri ${command} needs a source file`)
    const source = new Uint8Array(await readFile(args[1]))
    const result = await benchmarkCheck(compiler, source, command === 'bench' ? args[2] || 1000 : 1)
    console.log(`${command}: ${result.iterations} check(s), ${result.millis.toFixed(3)} ms total, ${(result.millis / result.iterations).toFixed(3)} ms/check, status=${result.status}`)
    process.exit(result.status === 0 ? 0 : 1)
  }
  if (command === 'run') backendParityError('run')
  if (command === 'build') backendParityError('build')
  if (!['check', 'lex-count', 'fingerprint'].includes(command)) { usage(); process.exit(2) }
  if (!args[1]) throw new Error(`noqeri: ${command} needs a source file`)
  if (command === 'check') {
    const { status } = await checkedSource(args[1])
    if (status === 0) { console.log(`${args[1]}: check OK`); process.exit(0) }
    const text = await readFile(args[1], 'utf8')
    console.error(diagnosticFor(text, status, args[1])); process.exit(1)
  }
  const source = new Uint8Array(await readFile(args[1]))
  if (command === 'lex-count') { console.log(String(compiler.stage1TokenCount(source))); process.exit(0) }
  console.log(String(compiler.stage1Fingerprint(source))); process.exit(0)
} catch (error) { console.error(`noqeri: ${error.message}`); process.exit(1) }
