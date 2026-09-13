#!/usr/bin/env node
import { spawnSync } from 'node:child_process'
import { existsSync, mkdirSync } from 'node:fs'
import { resolve } from 'node:path'

const args = process.argv.slice(2)
const modeArg = args.find(x => x.startsWith('--mode=')) || '--mode=cpu'
const mode = modeArg.slice('--mode='.length)
const forwarded = args.filter(x => x !== modeArg)
const out = resolve(process.env.NOQERI_PROFILE_DIR || 'build/profiles')
mkdirSync(out, { recursive: true })
const exe = process.platform === 'win32' ? 'noqeri.exe' : 'noqeri'
const binary = process.env.NOQERI_NATIVE || resolve('build', exe)
if (!existsSync(binary)) { console.error(`profile: missing ${binary}`); process.exit(2) }

function run(cmd, argv, env = process.env) {
  const r = spawnSync(cmd, argv, { stdio: 'inherit', env })
  if (r.error) { console.error(`profile: ${r.error.message}`); return 2 }
  return r.status ?? 2
}

if (mode === 'cpu') {
  if (process.platform === 'linux') process.exit(run('perf', ['record', '-g', '-o', resolve(out, 'cpu.perf.data'), '--', binary, ...forwarded]))
  if (process.platform === 'darwin') process.exit(run('xcrun', ['xctrace', 'record', '--template', 'Time Profiler', '--output', resolve(out, 'cpu.trace'), '--launch', '--', binary, ...forwarded]))
  console.error('profile: CPU collection currently uses perf on Linux or xctrace on macOS; Windows collector integration is not yet implemented')
  process.exit(2)
}
if (mode === 'memory' || mode === 'allocation') {
  const memory = process.env.NOQERI_MEMORY_CHECK_BINARY || resolve('build', 'memory', exe)
  if (!existsSync(memory)) { console.error('profile: build the memory-check preset first'); process.exit(2) }
  const env = { ...process.env, ASAN_OPTIONS: process.env.ASAN_OPTIONS || 'detect_leaks=1:log_path=build/profiles/asan' }
  process.exit(run(memory, forwarded, env))
}
if (mode === 'thread' || mode === 'task') {
  const race = process.env.NOQERI_RACE_BINARY || resolve('build', 'race', exe)
  if (!existsSync(race)) { console.error('profile: build the race-check preset first'); process.exit(2) }
  process.exit(run(race, forwarded, { ...process.env, TSAN_OPTIONS: process.env.TSAN_OPTIONS || 'halt_on_error=0:log_path=build/profiles/tsan' }))
}
console.error(`profile: unsupported mode ${mode}; use cpu, memory, allocation, thread or task`)
process.exit(2)
