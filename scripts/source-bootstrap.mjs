#!/usr/bin/env node
import { createHash } from 'node:crypto'
import { existsSync, mkdirSync, readFileSync, writeFileSync } from 'node:fs'
import { dirname, resolve } from 'node:path'
import { spawnSync } from 'node:child_process'
import { fileURLToPath } from 'node:url'

export function seedCandidates(buildDir, platform = process.platform) {
  const exe = platform === 'win32' ? 'noqeri.exe' : 'noqeri'
  return [
    resolve(buildDir, exe),
    resolve(buildDir, 'Release', exe),
    resolve(buildDir, 'release', exe),
    resolve(buildDir, 'RelWithDebInfo', exe)
  ]
}

export function cmakeConfigureArgs(root, buildDir) {
  return ['-S', root, '-B', buildDir, '-DCMAKE_BUILD_TYPE=Release', '-DBUILD_TESTING=OFF', '-DNOQERI_ENABLE_SANITIZERS=OFF']
}

export function cmakeBuildArgs(buildDir) {
  return ['--build', buildDir, '--config', 'Release', '--target', 'noqeri', '--parallel']
}

function sha256(path) {
  const hash = createHash('sha256')
  hash.update(readFileSync(path))
  return hash.digest('hex')
}

function run(command, args, cwd, quiet = false) {
  const result = spawnSync(command, args, {
    cwd,
    encoding: 'utf8',
    stdio: quiet ? ['ignore', 'pipe', 'pipe'] : 'inherit',
    maxBuffer: 32 * 1024 * 1024
  })
  if (result.error) throw new Error(`${command}: ${result.error.message}`)
  if (result.status !== 0) {
    const tail = quiet ? `\n${(result.stderr || result.stdout || '').slice(-4000)}` : ''
    throw new Error(`${command} exited with status ${result.status}${tail}`)
  }
  return result
}

function commandText(command, args) {
  return [command, ...args].map(x => /\s/.test(x) ? JSON.stringify(x) : x).join(' ')
}

export function buildSourceSeed({ root, buildDir, cmake = process.env.CMAKE || 'cmake', dryRun = false } = {}) {
  if (!root) throw new Error('root is required')
  buildDir ||= resolve(root, 'build', 'stage0-seed')
  const configure = cmakeConfigureArgs(root, buildDir)
  const build = cmakeBuildArgs(buildDir)
  if (dryRun) return { configure: commandText(cmake, configure), build: commandText(cmake, build), candidates: seedCandidates(buildDir) }
  mkdirSync(buildDir, { recursive: true })
  run(cmake, configure, root)
  run(cmake, build, root)
  const seed = seedCandidates(buildDir).find(existsSync)
  if (!seed) throw new Error(`CMake completed but no Noqeri seed was found under ${buildDir}`)
  const bootstrap = resolve(root, 'Compiler', 'selfhost', 'bootstrap.nqr')
  run(seed, ['--version'], root, true)
  run(seed, ['check', bootstrap], root, true)
  return { seed, bootstrap, buildDir, configure: commandText(cmake, configure), build: commandText(cmake, build) }
}

function gitCommit(root) {
  const result = spawnSync('git', ['rev-parse', 'HEAD'], { cwd: root, encoding: 'utf8' })
  return result.status === 0 ? result.stdout.trim() : null
}

export function writeSeedProof(root, result, output = resolve(root, 'build', 'stage0-seed.json')) {
  const proof = {
    schema: 1,
    kind: 'source-bootstrap-stage0',
    generated_at: new Date().toISOString(),
    repository_commit: gitCommit(root),
    seed: result.seed,
    seed_sha256: sha256(result.seed),
    bootstrap_source: 'Compiler/selfhost/bootstrap.nqr',
    bootstrap_sha256: sha256(result.bootstrap),
    cmake_configure: result.configure,
    cmake_build: result.build,
    trust_statement: 'No pre-existing Noqeri executable was required. The initial seed was compiled locally from the checked-out C++ bootstrap source; Stage-2 equivalence remains the gate for removing that bootstrap trust root.'
  }
  mkdirSync(dirname(output), { recursive: true })
  writeFileSync(output, JSON.stringify(proof, null, 2) + '\n')
  return proof
}

function parseArgs(argv) {
  const values = {}
  for (const arg of argv) {
    if (arg === '--dry-run') values.dryRun = true
    else if (arg.startsWith('--root=')) values.root = resolve(arg.slice(7))
    else if (arg.startsWith('--build-dir=')) values.buildDir = resolve(arg.slice(12))
    else if (arg.startsWith('--path-file=')) values.pathFile = resolve(arg.slice(12))
    else if (arg.startsWith('--proof=')) values.proof = resolve(arg.slice(8))
  }
  return values
}

if (process.argv[1] && resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  try {
    const args = parseArgs(process.argv.slice(2))
    const root = args.root || resolve(dirname(fileURLToPath(import.meta.url)), '..')
    const result = buildSourceSeed({ root, buildDir: args.buildDir, dryRun: args.dryRun })
    if (args.dryRun) {
      console.log(JSON.stringify(result, null, 2))
      process.exit(0)
    }
    const proof = writeSeedProof(root, result, args.proof || resolve(root, 'build', 'stage0-seed.json'))
    const pathFile = args.pathFile || resolve(root, 'build', 'stage0-seed.path')
    mkdirSync(dirname(pathFile), { recursive: true })
    writeFileSync(pathFile, result.seed + '\n')
    console.log(`source-bootstrap: verified seed ${result.seed}`)
    console.log(`source-bootstrap: sha256 ${proof.seed_sha256}`)
  } catch (error) {
    console.error(`source-bootstrap: ${error.message}`)
    process.exit(2)
  }
}
