#!/usr/bin/env node
import { mkdtempSync, writeFileSync, rmSync, existsSync } from 'node:fs'
import { tmpdir } from 'node:os'
import { join, resolve } from 'node:path'
import { spawnSync } from 'node:child_process'

const exe = process.argv[2] ? resolve(process.argv[2]) : resolve('build', process.platform === 'win32' ? 'noqeri.exe' : 'noqeri')
if (!existsSync(exe)) { console.error(`compiler-maturity: compiler not found: ${exe}`); process.exit(2) }
const root = mkdtempSync(join(tmpdir(), 'noqeri-maturity-'))
let failures = 0
function runCase(name, source, { ok = true, diagnostic = '', run = false, stdout = [] } = {}) {
  const file = join(root, `${name}.nqr`); writeFileSync(file, source)
  const result = spawnSync(exe, [run ? 'run' : 'check', file], { encoding: 'utf8', timeout: 20000, maxBuffer: 4 * 1024 * 1024 })
  const combined = `${result.stdout || ''}\n${result.stderr || ''}`
  const passed = (ok ? result.status === 0 : result.status !== 0) && (!diagnostic || combined.includes(diagnostic)) && stdout.every(x => (result.stdout || '').includes(x)) && !result.signal && !result.error
  if (!passed) { failures++; console.error(`FAIL ${name}: status=${result.status} signal=${result.signal || ''} error=${result.error?.message || ''}`); console.error(combined.slice(-5000)) }
  else console.log(`PASS ${name}`)
}
try {
  runCase('generic-whitespace-pointer-cast', `function probe(): void {\n  let p: *u8 = 0 as *u8\n  print("ok")\n}\n`)
  runCase('generic-whitespace-record-fields', `record Box<T> {\n  value: T\n  generation: u64\n}\n`)
  runCase('comptime-arithmetic', `const FACTOR: int = 7\nlet x: int = comptime(FACTOR + 5)\nprint(x)\n`, { run: true, stdout: ['12'] })
  runCase('comptime-function', `function twice(x: int): int { return x * 2 }\nfunction choose(flag: bool, a: int, b: int): int { if flag { return a } else { return b } }\nlet x: int = comptime(twice(9) + choose(true, 4, 99))\nprint(x)\n`, { run: true, stdout: ['22'] })
  runCase('comptime-runtime-rejected', `function runtime_value(x: int): int { let y: int = comptime(x + 1) return y }\n`, { ok: false, diagnostic: 'NQR-C5001' })
  runCase('comptime-impure-rejected', `let x: int = comptime(print(1))\n`, { ok: false, diagnostic: 'NQR-C5001' })
  runCase('comptime-overflow-rejected', `let x: int = comptime(9223372036854775807 * 2)\n`, { ok: false, diagnostic: 'NQR-C5001' })
  runCase('ownership-move-reinitialize', `let owner: int = 7\nlet transferred: int = move(owner)\nprint(transferred)\nowner = 9\nprint(owner)\n`, { run: true, stdout: ['7', '9'] })
  runCase('ownership-use-after-move', `let owner: int = 7\nlet transferred: int = move(owner)\nprint(owner)\n`, { ok: false, diagnostic: 'NQR-O4201' })
  runCase('ownership-move-while-borrowed', `let owner: int = 7\nlet reference: *int = &owner\nlet transferred: int = move(owner)\nprint(*reference)\n`, { ok: false, diagnostic: 'NQR-O4205' })
  runCase('ownership-mutate-while-borrowed', `let owner: int = 7\nlet reference: *int = &owner\nowner = 9\nprint(*reference)\n`, { ok: false, diagnostic: 'NQR-O4202' })
  runCase('ownership-borrow-scope-ends', `let owner: int = 7\n{ let reference: *int = &owner print(*reference) }\nowner = 9\nprint(owner)\n`, { run: true, stdout: ['7', '9'] })
} finally { rmSync(root, { recursive: true, force: true }) }
if (failures) { console.error(`compiler-maturity: ${failures} case(s) failed`); process.exit(1) }
console.log('compiler-maturity: all ownership/comptime/generic preprocessing cases passed')
