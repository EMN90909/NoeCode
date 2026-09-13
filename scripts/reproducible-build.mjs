#!/usr/bin/env node
import { mkdtempSync, rmSync, mkdirSync, readFileSync, existsSync } from 'node:fs'
import { tmpdir } from 'node:os'
import { join, resolve, relative } from 'node:path'
import { spawnSync } from 'node:child_process'
import { createHash } from 'node:crypto'

const source = resolve(process.argv[2] || '.')
const epoch = process.env.SOURCE_DATE_EPOCH || '1704067200'
const root = mkdtempSync(join(tmpdir(), 'noqeri-repro-'))
function run(cwd, cmd, args, env = {}) {
  const r = spawnSync(cmd, args, { cwd, stdio: 'inherit', env: { ...process.env, SOURCE_DATE_EPOCH: epoch, TZ: 'UTC', LC_ALL: 'C', ...env } })
  if (r.error || r.status !== 0) throw new Error(`${cmd} ${args.join(' ')} failed`)
}
function sha(path) { return createHash('sha256').update(readFileSync(path)).digest('hex') }

try {
  const a = join(root, 'a'), b = join(root, 'b')
  mkdirSync(a); mkdirSync(b)
  for (const dir of [a, b]) {
    run(source, 'cmake', ['-S', source, '-B', dir, '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release'])
    run(source, 'cmake', ['--build', dir])
  }
  const exe = process.platform === 'win32' ? 'noqeri.exe' : 'noqeri'
  const candidates = [[join(a, exe), join(b, exe)], [join(a, 'Release', exe), join(b, 'Release', exe)]]
  const pair = candidates.find(([x, y]) => existsSync(x) && existsSync(y))
  if (!pair) throw new Error('could not locate paired Noqeri executables')
  const hashes = pair.map(sha)
  const report = { source: relative(process.cwd(), source) || '.', source_date_epoch: epoch, artifact: exe, sha256_a: hashes[0], sha256_b: hashes[1], byte_identical: hashes[0] === hashes[1] }
  console.log(JSON.stringify(report, null, 2))
  process.exitCode = report.byte_identical ? 0 : 1
} catch (error) {
  console.error(`reproducible-build: ${error.message}`)
  process.exitCode = 2
} finally {
  rmSync(root, { recursive: true, force: true })
}
