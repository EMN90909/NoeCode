#!/usr/bin/env node
import { mkdtempSync, writeFileSync, rmSync, existsSync } from 'node:fs'
import { tmpdir } from 'node:os'
import { join, resolve } from 'node:path'
import { spawnSync } from 'node:child_process'

const exe = process.argv[2] ? resolve(process.argv[2]) : resolve('build', process.platform === 'win32' ? 'noqeri.exe' : 'noqeri')
if (!existsSync(exe)) {
  console.error(`compiler-maturity: compiler not found: ${exe}`)
  process.exit(2)
}

const root = mkdtempSync(join(tmpdir(), 'noqeri-maturity-'))
let failures = 0

function runCase(name, source, { ok = true, diagnostic = '', run = false, stdout = [] } = {}) {
  const file = join(root, `${name}.nqr`)
  writeFileSync(file, source)
  const args = [run ? 'run' : 'check', file]
  const result = spawnSync(exe, args, { encoding: 'utf8', timeout: 20000, maxBuffer: 4 * 1024 * 1024 })
  const passedExit = ok ? result.status === 0 : result.status !== 0
  const combined = `${result.stdout || ''}\n${result.stderr || ''}`
  const passedDiagnostic = !diagnostic || combined.includes(diagnostic)
  const passedStdout = stdout.every(fragment => (result.stdout || '').includes(fragment))
  if (!passedExit || !passedDiagnostic || !passedStdout || result.signal || result.error) {
    failures++
    console.error(`FAIL ${name}: status=${result.status} signal=${result.signal || ''} error=${result.error?.message || ''}`)
    console.error(combined.slice(-5000))
  } else {
    console.log(`PASS ${name}`)
  }
}

try {
  runCase('comptime-arithmetic', `
const FACTOR: int = 7
let x: int = comptime(FACTOR + 5)
print(x)
`, { run: true, stdout: ['12'] })

  runCase('comptime-function', `
function twice(x: int): int { return x * 2 }
function choose(flag: bool, a: int, b: int): int { if flag { return a } else { return b } }
let x: int = comptime(twice(9) + choose(true, 4, 99))
print(x)
`, { run: true, stdout: ['22'] })

  runCase('comptime-runtime-rejected', `
function runtime_value(x: int): int {
    let y: int = comptime(x + 1)
    return y
}
`, { ok: false, diagnostic: 'NQR-C5001' })

  runCase('comptime-impure-rejected', `
let x: int = comptime(print(1))
`, { ok: false, diagnostic: 'NQR-C5001' })

  runCase('ownership-move-reinitialize', `
let owner: int = 7
let transferred: int = move(owner)
print(transferred)
owner = 9
print(owner)
`, { run: true, stdout: ['7', '9'] })

  runCase('ownership-use-after-move', `
let owner: int = 7
let transferred: int = move(owner)
print(owner)
`, { ok: false, diagnostic: 'NQR-O4201' })

  runCase('ownership-move-while-borrowed', `
let owner: int = 7
let reference: *int = &owner
let transferred: int = move(owner)
print(*reference)
`, { ok: false, diagnostic: 'NQR-O4205' })

  runCase('ownership-mutate-while-borrowed', `
let owner: int = 7
let reference: *int = &owner
owner = 9
print(*reference)
`, { ok: false, diagnostic: 'NQR-O4202' })

  runCase('ownership-borrow-scope-ends', `
let owner: int = 7
{
    let reference: *int = &owner
    print(*reference)
}
owner = 9
print(owner)
`, { run: true, stdout: ['7', '9'] })
} finally {
  rmSync(root, { recursive: true, force: true })
}

if (failures) {
  console.error(`compiler-maturity: ${failures} case(s) failed`)
  process.exit(1)
}
console.log('compiler-maturity: all ownership/comptime cases passed')
