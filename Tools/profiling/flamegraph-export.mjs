#!/usr/bin/env node
import { existsSync } from 'node:fs'
import { mkdir, writeFile } from 'node:fs/promises'
import { dirname, resolve } from 'node:path'
import { spawnSync } from 'node:child_process'

const args = process.argv.slice(2)
const input = resolve(args.find(x => !x.startsWith('--')) || 'build/profiles/cpu.perf.data')
const output = resolve(args.find(x => x.startsWith('--out='))?.slice(6) || 'build/profiles/cpu.flamegraph.svg')
if (process.platform !== 'linux') {
  console.error('flamegraph-export: current exporter supports Linux perf data only; use native platform tooling on other systems')
  process.exit(2)
}
if (!existsSync(input)) {
  console.error(`flamegraph-export: input does not exist: ${input}`)
  process.exit(2)
}
const collapse = process.env.NOQERI_STACKCOLLAPSE_PERF || 'stackcollapse-perf.pl'
const flamegraph = process.env.NOQERI_FLAMEGRAPH || 'flamegraph.pl'
function capture(command, commandArgs, inputText = null) {
  const r = spawnSync(command, commandArgs, { input:inputText, encoding:'utf8', maxBuffer:128*1024*1024 })
  if (r.error || r.status !== 0) throw new Error(`${command} failed${r.stderr ? `: ${r.stderr.trim()}` : ''}`)
  return r.stdout
}
try {
  const script = capture('perf', ['script','-i',input])
  const folded = capture(collapse, [], script)
  const svg = capture(flamegraph, ['--title','Noqeri CPU profile'], folded)
  await mkdir(dirname(output), { recursive:true })
  await writeFile(output, svg)
  console.log(`flamegraph: ${output}`)
} catch (error) {
  console.error(`flamegraph-export: ${error.message}`)
  console.error('Install perf and Brendan Gregg FlameGraph scripts, or set NOQERI_STACKCOLLAPSE_PERF / NOQERI_FLAMEGRAPH.')
  process.exit(2)
}
