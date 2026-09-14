#!/usr/bin/env node
import { mkdtempSync, mkdirSync, writeFileSync, rmSync, existsSync, readdirSync, readFileSync } from 'node:fs'
import { tmpdir } from 'node:os'
import { basename, join, resolve } from 'node:path'
import { spawnSync } from 'node:child_process'
import { XorShift32, classifyProcessResult, failureKey, minimizeSequence, mutateBytes, mutateText, saveCorpusCase } from './engine.mjs'

const opts = Object.fromEntries(process.argv.slice(2).map(x => {
  const [k, v = 'true'] = x.replace(/^--/, '').split('=')
  return [k, v]
}))
const cases = Math.max(1, Number(opts.cases || 250))
const seed = Number(opts.seed || 0x4e515246) >>> 0
const timeout = Math.max(50, Number(opts.timeout || 1500))
const minimize = opts.minimize !== 'false'
const exe = process.platform === 'win32' ? 'noqeri.exe' : 'noqeri'
const noqeri = process.env.NOQERI_NATIVE || resolve('build', exe)
const stage1 = process.env.NOQERI_STAGE1 || ''
const corpusRoot = resolve(opts.corpus || 'build/fuzz-corpus')
if (!existsSync(noqeri)) {
  console.error(`fuzz-all: ${noqeri} is missing; build Noqeri first`)
  process.exit(2)
}

const rng = new XorShift32(seed)
function randomBytes(max = 768) {
  const n = rng.int(max)
  const out = Buffer.alloc(n)
  for (let i = 0; i < n; i++) out[i] = rng.next() & 0xff
  return out
}
function nonEmptyBytes(max = 768) { const value = randomBytes(max); return value.length ? value : Buffer.from([0]) }
function byteArrayLiteral(buffer) { return [...buffer].map(x => `${x} as u8`).join(',') }
function parserHarness(buffer) {
  const literal = byteArrayLiteral(buffer)
  return `import "../../Lib/std/sql.nqr"\nimport "../../Lib/std/url.nqr"\nimport "../../Lib/std/http.nqr"\nimport "../../Lib/std/dns.nqr"\n\nlet data: [u8; ${buffer.length}] = [${literal}]\nlet view: []u8 = slice(data)\nlet decoded: [u8; ${Math.max(8, buffer.length * 3 + 8)}]\nsql_count_placeholders(view)\nsql_has_unterminated_quote(view)\nsql_identifier_valid(view)\nurl_has_scheme(view)\nurl_percent_encoded_len(view)\nurl_percent_decode(view, slice(decoded))\ndns_name_valid(view)\nlet i: usize = 0 as usize\nwhile i < len(view) { http_is_token_char(view[i]) i = i + 1 as usize }\nprint(0)\n`
}
function formatAwareObject() {
  let out = mutateBytes(nonEmptyBytes(1024), rng, 2048)
  if (!out.length) out = Buffer.from([0])
  const kind = rng.int(4)
  if (kind === 0 && out.length >= 4) Buffer.from([0x7f, 0x45, 0x4c, 0x46]).copy(out, 0)
  else if (kind === 1 && out.length >= 4) Buffer.from([0xcf, 0xfa, 0xed, 0xfe]).copy(out, 0)
  else if (kind === 2 && out.length >= 2) Buffer.from([0x64, 0x86]).copy(out, 0)
  return out
}
function loadCorpus(directory, fallback) {
  if (!existsSync(directory)) return [fallback]
  const entries = readdirSync(directory).filter(x => !x.endsWith('.json')).slice(0, 256)
  if (!entries.length) return [fallback]
  return entries.map(x => readFileSync(join(directory, x), 'utf8')).filter(Boolean).concat(fallback)
}
function processRun(binary, args) {
  return spawnSync(binary, args, { encoding: 'utf8', timeout, maxBuffer: 4 * 1024 * 1024 })
}

const root = mkdtempSync(join(tmpdir(), 'noqeri-fuzz-'))
const generatedRoot = resolve('build', 'fuzz-generated')
const crashRoot = resolve('build', 'fuzz-crashes')
mkdirSync(generatedRoot, { recursive: true })
mkdirSync(corpusRoot, { recursive: true })
mkdirSync(crashRoot, { recursive: true })
const failures = []
const seenFailureKeys = new Set()

function recordFailure(label, args, result, inputFile = '', input = null, expected = new Set([0, 1])) {
  const classification = classifyProcessResult(result, expected)
  if (!classification.failed) return null
  const failure = {
    label, kind: classification.kind, args, status: result.status, signal: result.signal,
    error: result.error?.message || '', stderr: (result.stderr || '').slice(0, 2400), seed, rng_state: rng.state
  }
  failure.key = failureKey(failure)
  if (seenFailureKeys.has(failure.key)) return failure
  seenFailureKeys.add(failure.key)
  if (inputFile && input !== null) {
    let minimized = input
    if (minimize) {
      const original = Buffer.isBuffer(input) ? Buffer.from(input) : String(input)
      minimized = minimizeSequence(original, candidate => {
        writeFileSync(inputFile, candidate)
        return classifyProcessResult(processRun(noqeri, args), expected).failed
      })
      writeFileSync(inputFile, original)
    }
    failure.minimized = saveCorpusCase(crashRoot, `${label}-${failure.key}`, minimized, Buffer.isBuffer(minimized) ? '.bin' : '.nqr')
  }
  failures.push(failure)
  return failure
}

function run(label, args, { inputFile = '', input = null, expected = new Set([0, 1]) } = {}) {
  const result = processRun(noqeri, args)
  recordFailure(label, args, result, inputFile, input, expected)
  return result
}

function differentialCheck(source) {
  if (!stage1 || failures.length) return
  const native = processRun(noqeri, ['check', source])
  const portable = processRun(stage1, ['check', source])
  const nativeAccepted = native.status === 0
  const portableAccepted = portable.status === 0
  if (nativeAccepted !== portableAccepted) {
    const failure = {
      label: 'stage0-stage1-differential', kind: 'semantic-difference', status: native.status,
      signal: native.signal, stderr: `native=${native.status} stage1=${portable.status}\n${(portable.stderr || '').slice(0, 1200)}`,
      seed, rng_state: rng.state, source
    }
    failure.key = failureKey(failure)
    if (!seenFailureKeys.has(failure.key)) { seenFailureKeys.add(failure.key); failure.minimized = saveCorpusCase(crashRoot, `differential-${failure.key}`, readFileSync(source, 'utf8'), '.nqr'); failures.push(failure) }
  }
}

const sourceSeed = 'module fuzz.seed\nrecord Pair { left: int, right: int }\nfunction add(a: int, b: int): int { return a + b }\nconst folded: int = comptime(add(20, 22))\nlet x: int = add(1, 2)\nprint(x)\n'
const nqdSeed = 'table users { id: int key, name: text required }\ndelete users\ninsert users { id: 1, name: "Ada" }\nselect users\n'
const manifestSeed = 'package { name: "fuzz/project" version: "0.0.1" edition: "2026" entry: "main.nqr" profile: "application" target: "x86_64-unknown-none" }\n'
const sourceCorpus = loadCorpus(join(corpusRoot, 'source'), sourceSeed)

try {
  if (opts.replay) {
    const replay = resolve(opts.replay)
    if (!existsSync(replay)) throw new Error(`replay input does not exist: ${replay}`)
    const input = readFileSync(replay)
    const target = opts.target || (replay.endsWith('.nqr') ? 'check' : 'link')
    const args = target === 'link' ? ['link', join(root, 'replay-out'), replay] : [target, replay]
    const result = run(`replay-${target}`, args, { inputFile: replay, input, expected: new Set([0, 1]) })
    console.log(JSON.stringify({ result: failures.length ? 'failure' : 'ok', replay, target, status: result.status, failures }, null, 2))
    process.exitCode = failures.length ? 1 : 0
  } else {
    for (let i = 0; i < cases; i++) {
      const sourceBase = rng.pick(sourceCorpus)
      const sourceText = mutateText(sourceBase, rng)
      const source = join(root, `case-${i}.nqr`)
      writeFileSync(source, sourceText)
      run('lexer', ['lex', source], { inputFile: source, input: sourceText })
      const check = run('parser+type+ownership+borrow+safety+comptime', ['check', source], { inputFile: source, input: sourceText })
      run('lowerer', ['nir', source], { inputFile: source, input: sourceText })
      differentialCheck(source)
      if (check.status === 0 && i % 25 === 0) saveCorpusCase(join(corpusRoot, 'source'), 'valid', sourceText, '.nqr')

      const nqd = join(root, `case-${i}.nqd`)
      const nqdText = i % 3 === 0 ? mutateText(nqdSeed, rng) : randomBytes(512).toString('latin1')
      writeFileSync(nqd, nqdText)
      run('database-nqd-parser', ['db', nqd, join(root, `case-${i}.nqdb`)], { inputFile: nqd, input: nqdText })

      const manifest = join(root, `project-${i}.nqr`)
      const manifestText = mutateText(manifestSeed, rng)
      writeFileSync(manifest, manifestText)
      run('package-manifest-parser', ['manifest', manifest], { inputFile: manifest, input: manifestText })

      const parserInput = i % 2 === 0 ? Buffer.from(mutateText("SELECT * FROM users WHERE name = '?' https://example.test/a%20b", rng), 'utf8') : nonEmptyBytes(384)
      const parserSourceText = parserHarness(parserInput)
      const parserSource = join(generatedRoot, `parsers-${i}.nqr`)
      writeFileSync(parserSource, parserSourceText)
      run('sql-url-dns-http-parsers', ['run', parserSource], { inputFile: parserSource, input: parserSourceText })

      const object = join(root, `object-${i}.o`)
      const objectData = formatAwareObject()
      writeFileSync(object, objectData)
      run('object-loader-linker', ['link', join(root, `out-${i}`), object], { inputFile: object, input: objectData })

      if (failures.length) break
    }

    const report = {
      result: failures.length ? 'failure' : 'ok', seed, final_rng_state: rng.state, requested_cases: cases,
      stage1_differential: Boolean(stage1), corpus: corpusRoot,
      targets: ['lexer','parser/type/ownership/borrow/safety/comptime','lowerer','database/.nqd','package-manifest','sql/url/dns/http','object-loader/linker'],
      failures
    }
    writeFileSync(resolve('build', 'fuzz-report.json'), JSON.stringify(report, null, 2) + '\n')
    console.log(JSON.stringify(report, null, 2))
    process.exitCode = failures.length ? 1 : 0
  }
} catch (error) {
  console.error(`fuzz-all: ${error.message}`)
  process.exitCode = 2
} finally {
  rmSync(root, { recursive: true, force: true })
  rmSync(generatedRoot, { recursive: true, force: true })
}
