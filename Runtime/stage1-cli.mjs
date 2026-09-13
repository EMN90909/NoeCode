#!/usr/bin/env node
import { readFile } from 'node:fs/promises'
import { pathToFileURL } from 'node:url'
import { resolve } from 'node:path'
import { createPackageManager } from './package-manager.mjs'

function usage() {
  console.log(`Noqeri stage-1 portable compiler kernel\n\nUsage:\n  noqeri --version\n  noqeri check <file.nqr>\n  noqeri lex-count <file.nqr>\n  noqeri fingerprint <file.nqr>\n  noqeri add <namespace/name>[@version]\n  noqeri install\n  noqeri resolve <file.nqr>\n  noqeri selftest\n\nRegistry source uses import package "namespace/name". Packages are cached immutably and re-hashed before use.`)
}

const modulePath = process.env.NOQERI_STAGE1_MODULE || resolve(process.cwd(), 'build/noqeri-stage1.nqo')
let compiler
try { compiler = await import(pathToFileURL(modulePath).href) } catch (error) { console.error(`noqeri: cannot load stage-1 module ${modulePath}: ${error.message}`); process.exit(2) }
const packages = createPackageManager()
const args = process.argv.slice(2), command = args[0] || '--help'

try {
  if (command === '--help' || command === 'help') { usage(); process.exit(0) }
  if (command === '--version') { console.log(`Noqeri ${compiler.stage1VersionMajor?.() ?? 1}.${compiler.stage1VersionMinor?.() ?? 0}.${compiler.stage1VersionPatch?.() ?? 0} stage1`); process.exit(0) }
  if (command === 'selftest') {
    const sample = new TextEncoder().encode('function main(): int { return 0 }')
    const status = compiler.stage1CheckSource(sample), tokens = compiler.stage1TokenCount(sample)
    const pkg = new TextEncoder().encode('noqeri/supabase')
    if (status !== 0 || tokens < 6 || compiler.stage1PackageCoordinateValid?.(pkg) !== true) throw new Error(`stage-1 selftest failed: status=${status} tokens=${tokens}`)
    console.log(`stage-1 selftest passed (${tokens} tokens; package policy OK)`); process.exit(0)
  }
  if (command === 'add') { if (!args[1]) throw new Error('noqeri add needs namespace/name[@version]'); await packages.addPackage(args[1]); process.exit(0) }
  if (command === 'install') { await packages.installProject(); process.exit(0) }
  if (command === 'resolve') { if (!args[1]) throw new Error('noqeri resolve needs a source file'); for (const item of await packages.resolvePackageImports(args[1])) console.log(`${item.coordinate}@${item.version} -> ${item.entry} ${item.integrity}`); process.exit(0) }
  if (!['check', 'lex-count', 'fingerprint'].includes(command)) { usage(); process.exit(2) }
  if (!args[1]) throw new Error(`noqeri: ${command} needs a source file`)
  if (command === 'check') {
    const resolved = await packages.resolvePackageImports(args[1])
    for (const item of resolved) {
      const packageSource = new Uint8Array(await readFile(item.entry))
      const packageStatus = Number(compiler.stage1CheckSource(packageSource))
      if (packageStatus !== 0) throw new Error(`${item.coordinate}@${item.version}: package entry failed stage-1 check (${packageStatus})`)
    }
  }
  const source = new Uint8Array(await readFile(args[1]))
  if (command === 'check') {
    const status = Number(compiler.stage1CheckSource(source))
    if (status === 0) { console.log(`${args[1]}: syntax/token check OK`); process.exit(0) }
    console.error(`${args[1]}: stage-1 check failed (${status})`); process.exit(1)
  }
  if (command === 'lex-count') { console.log(String(compiler.stage1TokenCount(source))); process.exit(0) }
  console.log(String(compiler.stage1Fingerprint(source))); process.exit(0)
} catch (error) { console.error(`noqeri: ${error.message}`); process.exit(1) }
