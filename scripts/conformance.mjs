#!/usr/bin/env node
import { readdir, readFile, stat } from 'node:fs/promises'
import { spawnSync } from 'node:child_process'
import { resolve, join, relative } from 'node:path'

const root = resolve(process.cwd())
const compiler = resolve(process.env.NOQERI_BIN || join(root, 'build', process.platform === 'win32' ? 'noqeri.cmd' : 'noqeri'))

async function filesUnder(directory) {
  const out = []
  async function walk(path) {
    for (const entry of await readdir(path, { withFileTypes: true })) {
      const item = join(path, entry.name)
      if (entry.isDirectory()) await walk(item)
      else if (entry.isFile() && entry.name.endsWith('.nqr')) out.push(item)
    }
  }
  try { await walk(directory) } catch (error) { if (error.code !== 'ENOENT') throw error }
  return out.sort()
}

function runCheck(file) {
  return spawnSync(compiler, ['check', file], { cwd: root, encoding: 'utf8', timeout: 15000, env: { ...process.env, NOQERI_OFFLINE: process.env.NOQERI_OFFLINE || '1' } })
}

async function expectedFragment(file) {
  try { return (await readFile(file.replace(/\.nqr$/, '.expect'), 'utf8')).trim() } catch (error) { if (error.code === 'ENOENT') return ''; throw error }
}

try { await stat(compiler) } catch { console.error(`conformance: compiler not found: ${compiler}`); process.exit(2) }

const valid = await filesUnder(join(root, 'conformance', 'valid'))
const invalid = await filesUnder(join(root, 'conformance', 'invalid'))
let failures = 0

for (const file of valid) {
  const result = runCheck(file), name = relative(root, file)
  if (result.status !== 0) {
    failures++
    console.error(`FAIL valid ${name}\n${result.stdout}${result.stderr}`)
  } else console.log(`PASS valid ${name}`)
}

for (const file of invalid) {
  const result = runCheck(file), name = relative(root, file), expected = await expectedFragment(file)
  const combined = `${result.stdout || ''}${result.stderr || ''}`
  if (result.status === 0) {
    failures++
    console.error(`FAIL invalid accepted ${name}`)
  } else if (expected && !combined.includes(expected)) {
    failures++
    console.error(`FAIL invalid diagnostic ${name}: expected ${JSON.stringify(expected)}\n${combined}`)
  } else console.log(`PASS invalid ${name}`)
}

console.log(`conformance: ${valid.length + invalid.length - failures}/${valid.length + invalid.length} passed`)
process.exit(failures ? 1 : 0)
