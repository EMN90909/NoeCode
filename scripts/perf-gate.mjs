#!/usr/bin/env node
import { readFileSync } from 'node:fs'

if (process.argv.length < 4) {
  console.error('usage: node scripts/perf-gate.mjs <baseline.json> <candidate.json>')
  process.exit(2)
}
const baseline = JSON.parse(readFileSync(process.argv[2], 'utf8'))
const candidate = JSON.parse(readFileSync(process.argv[3], 'utf8'))
const base = new Map((baseline.results || []).map(r => [r.id, r]))
const failures = []
const checked = []
for (const row of candidate.results || []) {
  const before = base.get(row.id)
  if (!before) continue
  const threshold = Number(row.threshold_percent ?? before.threshold_percent ?? 8)
  const oldMedian = Number(before.median_ms)
  const newMedian = Number(row.median_ms)
  if (!(oldMedian > 0) || !(newMedian >= 0)) continue
  const change = ((newMedian - oldMedian) / oldMedian) * 100
  checked.push({ id: row.id, oldMedian, newMedian, change_percent: change, threshold_percent: threshold })
  if (change > threshold) failures.push(checked.at(-1))
}
console.log(JSON.stringify({ checked, failures }, null, 2))
if (!checked.length) {
  console.error('perf-gate: no comparable result IDs; refusing to report a passing gate')
  process.exit(2)
}
process.exit(failures.length ? 1 : 0)
