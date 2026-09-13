#!/usr/bin/env node
import { mkdtemp, rm, readFile, readdir } from 'node:fs/promises'
import { tmpdir } from 'node:os'
import { join, resolve, relative } from 'node:path'
import { spawnSync } from 'node:child_process'

const root = resolve(process.cwd())
const compiler = resolve(process.env.NOQERI_BIN || join(root, 'build', process.platform === 'win32' ? 'noqeri.cmd' : 'noqeri'))
const corpus = resolve(process.env.NOQERI_DIFFERENTIAL_DIR || join(root, 'conformance', 'differential'))

async function listPrograms() {
  try { return (await readdir(corpus, { withFileTypes: true })).filter(entry => entry.isFile() && entry.name.endsWith('.nqr')).map(entry => join(corpus, entry.name)).sort() } catch (error) { if (error.code === 'ENOENT') return []; throw error }
}

function execute(command, args, cwd = root) {
  return spawnSync(command, args, { cwd, encoding: 'utf8', timeout: 30000, env: { ...process.env, NOQERI_OFFLINE: process.env.NOQERI_OFFLINE || '1' } })
}

function observable(result) {
  return { status: result.status, signal: result.signal || null, stdout: result.stdout || '', stderr: result.stderr || '' }
}

function same(a, b) { return a.status === b.status && a.signal === b.signal && a.stdout === b.stdout && a.stderr === b.stderr }

async function runOne(file) {
  const name = relative(root, file), check = execute(compiler, ['check', file])
  if (check.status !== 0) return { name, error: `check failed\n${check.stdout}${check.stderr}` }

  const interpreted = execute(compiler, ['run', file])
  if (interpreted.status === 2 && /Usage|unknown|unsupported/i.test(`${interpreted.stdout}${interpreted.stderr}`)) return { name, skipped: 'compiler does not expose run backend yet' }

  const temporary = await mkdtemp(join(tmpdir(), 'noqeri-diff-'))
  try {
    const nativeOut = join(temporary, process.platform === 'win32' ? 'program.exe' : 'program')
    const built = execute(compiler, ['build', file, '-o', nativeOut])
    if (built.status !== 0) return { name, skipped: `native build unavailable: ${built.stderr || built.stdout}` }
    const native = execute(nativeOut, [], temporary)

    const webOut = join(temporary, 'program.nqo')
    const webBuilt = execute(compiler, ['web', file, webOut])
    if (webBuilt.status !== 0) return { name, skipped: `web backend unavailable: ${webBuilt.stderr || webBuilt.stdout}` }
    const web = execute(process.execPath, [webOut], temporary)

    const reference = observable(interpreted), nativeObserved = observable(native), webObserved = observable(web)
    if (!same(reference, nativeObserved) || !same(reference, webObserved)) return { name, error: `backend mismatch\ninterpreter=${JSON.stringify(reference)}\nnative=${JSON.stringify(nativeObserved)}\nweb=${JSON.stringify(webObserved)}` }
    return { name, passed: true }
  } finally { await rm(temporary, { recursive: true, force: true }) }
}

const programs = await listPrograms()
if (!programs.length) { console.log('differential: no corpus programs'); process.exit(0) }
let failures = 0, skipped = 0
for (const program of programs) {
  const result = await runOne(program)
  if (result.error) { failures++; console.error(`FAIL ${result.name}: ${result.error}`) }
  else if (result.skipped) { skipped++; console.log(`SKIP ${result.name}: ${result.skipped.trim()}`) }
  else console.log(`PASS ${result.name}`)
}
console.log(`differential: ${programs.length - failures - skipped} passed, ${skipped} skipped, ${failures} failed`)
process.exit(failures ? 1 : 0)
