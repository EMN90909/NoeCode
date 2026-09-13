#!/usr/bin/env node
import { existsSync } from 'node:fs'
import { resolve, join } from 'node:path'
import { spawnSync } from 'node:child_process'

const root = resolve(process.cwd())
const args = process.argv.slice(2)
const build = resolve(args.find(x => x.startsWith('--build='))?.slice(8) || join(root, 'build'))
const full = args.includes('--full')
const compiler = resolve(process.env.NOQERI_BIN || join(build, process.platform === 'win32' ? 'noqeri.exe' : 'noqeri'))
const env = { ...process.env, NOQERI_BIN: compiler, NOQERI_OFFLINE: '1' }

function run(label, command, commandArgs, options = {}) {
  process.stdout.write(`\n== ${label} ==\n`)
  const result = spawnSync(command, commandArgs, {
    cwd: root,
    env,
    encoding: 'utf8',
    stdio: 'inherit',
    timeout: options.timeout || (full ? 15 * 60_000 : 5 * 60_000)
  })
  if (result.error) throw result.error
  if (result.signal) throw new Error(`${label} terminated by ${result.signal}`)
  if (result.status !== 0) throw new Error(`${label} failed with exit ${result.status}`)
}

if (!existsSync(join(root, 'CMakeLists.txt'))) throw new Error('run maturity verification from the Noqeri repository root')

run('configure', 'cmake', ['-S', root, '-B', build, '-DBUILD_TESTING=ON', '-DNOQERI_WARNINGS_AS_ERRORS=ON'])
run('build', 'cmake', ['--build', build, '--config', 'Release', '--parallel'])
if (!existsSync(compiler) && process.platform === 'win32') {
  env.NOQERI_BIN = resolve(join(build, 'Release', 'noqeri.exe'))
}
run('ctest', 'ctest', ['--test-dir', build, '-C', 'Release', '--output-on-failure'])
run('deterministic fuzz', process.execPath, ['scripts/fuzz.mjs', '--cases', full ? '1000' : '100', '--seed', '1314014546'])
run('conformance', process.execPath, ['scripts/conformance.mjs'])
run('differential backends', process.execPath, ['scripts/differential-test.mjs'])

if (full) {
  run('reproducible build', process.execPath, ['scripts/reproducible-build.mjs'])
  if (existsSync(join(root, 'Benchmarks', 'results', 'baseline.json'))) {
    run('benchmarks', process.execPath, ['scripts/benchmark.mjs', '--output=Benchmarks/results/latest.json'])
    run('performance regression gate', process.execPath, ['scripts/perf-gate.mjs', 'Benchmarks/results/latest.json', 'Benchmarks/results/baseline.json'])
  } else {
    console.log('\nbenchmark gate: skipped because Benchmarks/results/baseline.json has not been approved yet')
  }
}

console.log(`\nNoqeri maturity verification: PASS (${full ? 'full' : 'standard'})`)
