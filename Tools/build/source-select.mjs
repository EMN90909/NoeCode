#!/usr/bin/env node
import { readFile } from 'node:fs/promises'
import { dirname, normalize, resolve, sep } from 'node:path'

function safeRelative(path) {
  if (typeof path !== 'string' || !path || path.includes('\0')) throw new Error(`invalid source path: ${String(path)}`)
  const n = normalize(path)
  if (n === '..' || n.startsWith(`..${sep}`) || resolve('/', n) === n) throw new Error(`source path escapes project: ${path}`)
  return n
}
function matches(when, ctx) {
  const has = (values, actual) => !values || values.length === 0 || values.includes(actual)
  if (!has(when?.os, ctx.os)) return false
  if (!has(when?.arch, ctx.arch)) return false
  if (!has(when?.target, ctx.target)) return false
  const required = when?.features || []
  return required.every(x => ctx.features.has(x))
}
export function selectSources(config, ctx) {
  const selected = []
  const add = values => { for (const value of values || []) { const path = safeRelative(value); if (!selected.includes(path)) selected.push(path) } }
  add(config.sources)
  for (const rule of config.conditional || []) if (matches(rule.when || {}, ctx)) add(rule.sources)
  return selected
}
async function main() {
  const args = process.argv.slice(2)
  const configPath = resolve(args.find(x => !x.startsWith('--')) || 'noqeri.build.json')
  const os = args.find(x => x.startsWith('--os='))?.slice(5) || process.platform
  const arch = args.find(x => x.startsWith('--arch='))?.slice(7) || process.arch
  const target = args.find(x => x.startsWith('--target='))?.slice(9) || ''
  const features = new Set(args.filter(x => x.startsWith('--feature=')).map(x => x.slice(10)))
  const config = JSON.parse(await readFile(configPath, 'utf8'))
  if (config.schema !== 1) throw new Error('noqeri.build.json requires schema: 1')
  const sources = selectSources(config, { os, arch, target, features })
  const root = dirname(configPath)
  if (args.includes('--json')) console.log(JSON.stringify({ os, arch, target, features: [...features], sources: sources.map(x => resolve(root, x)) }, null, 2))
  else for (const source of sources) console.log(resolve(root, source))
}
main().catch(error => { console.error(`noqeri source-select: ${error.message}`); process.exit(2) })
