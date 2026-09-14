import { createHash } from 'node:crypto'
import { mkdirSync, writeFileSync } from 'node:fs'
import { extname, join } from 'node:path'

export class XorShift32 {
  constructor(seed = 0x4e515246) { this.state = Number(seed) >>> 0 || 1 }
  next() { let x = this.state >>> 0; x ^= x << 13; x ^= x >>> 17; x ^= x << 5; this.state = x >>> 0; return this.state }
  int(max) { if (!Number.isInteger(max) || max <= 0) throw new RangeError('max must be a positive integer'); return this.next() % max }
  pick(values) { if (!values.length) throw new RangeError('cannot pick from an empty collection'); return values[this.int(values.length)] }
}

export function sha256(value) {
  const hash = createHash('sha256')
  hash.update(value)
  return hash.digest('hex')
}

export function mutateText(seedText, rng, maxEdits = 24) {
  const alphabet = '(){}[]<>+-*/%=!&|,:;.abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_ \n\t"\\'
  let text = String(seedText)
  const edits = 1 + rng.int(Math.max(1, maxEdits))
  const tokens = ['function','record','import','let','const','while','repeat','return','unsafe','null','comptime','0','999999999999999999999']
  for (let e = 0; e < edits; e++) {
    const at = text.length ? rng.int(text.length) : 0
    const op = rng.int(4)
    if (op === 0) text = text.slice(0, at) + rng.pick([...alphabet]) + text.slice(at)
    else if (op === 1 && text.length) text = text.slice(0, at) + text.slice(at + 1)
    else if (op === 2 && text.length) text = text.slice(0, at) + rng.pick([...alphabet]) + text.slice(at + 1)
    else text = text.slice(0, at) + rng.pick(tokens) + text.slice(at)
  }
  return text
}

export function mutateBytes(seed, rng, maxBytes = 2048) {
  let out = Buffer.from(seed || [])
  if (!out.length) out = Buffer.from([rng.next() & 0xff])
  const edits = 1 + rng.int(32)
  for (let i = 0; i < edits; i++) {
    const op = rng.int(4)
    const at = out.length ? rng.int(out.length) : 0
    if (op === 0 && out.length < maxBytes) out = Buffer.concat([out.subarray(0, at), Buffer.from([rng.next() & 0xff]), out.subarray(at)])
    else if (op === 1 && out.length > 1) out = Buffer.concat([out.subarray(0, at), out.subarray(at + 1)])
    else if (op === 2 && out.length) out[at] = rng.next() & 0xff
    else if (out.length < maxBytes) out = Buffer.concat([out, Buffer.from([rng.next() & 0xff])])
  }
  return out
}

export function failureKey(failure) {
  const stable = [failure.label || '', failure.kind || '', failure.signal || '', String(failure.status ?? ''), (failure.stderr || '').split(/\r?\n/)[0] || ''].join('\0')
  return sha256(stable).slice(0, 20)
}

// Delta-debug a sequence while preserving an externally supplied failure
// predicate. The predicate is synchronous by design so compiler fuzzing can use
// spawnSync and remain deterministic/replayable.
export function minimizeSequence(input, interesting, { minLength = 1 } = {}) {
  let current = Buffer.isBuffer(input) ? Buffer.from(input) : String(input)
  let partitions = 2
  const lengthOf = x => x.length
  const sliceOf = (x, a, b) => x.slice(a, b)
  const concat = (a, b) => Buffer.isBuffer(current) ? Buffer.concat([a, b]) : a + b
  while (lengthOf(current) > minLength) {
    const length = lengthOf(current)
    const chunk = Math.ceil(length / partitions)
    let reduced = false
    for (let start = 0; start < length; start += chunk) {
      const candidate = concat(sliceOf(current, 0, start), sliceOf(current, Math.min(length, start + chunk)))
      if (lengthOf(candidate) < minLength) continue
      if (interesting(candidate)) { current = candidate; partitions = Math.max(2, partitions - 1); reduced = true; break }
    }
    if (reduced) continue
    if (partitions >= length) break
    partitions = Math.min(length, partitions * 2)
  }
  return current
}

export function saveCorpusCase(directory, label, input, extension = '') {
  mkdirSync(directory, { recursive: true })
  const data = Buffer.isBuffer(input) ? input : Buffer.from(String(input), 'utf8')
  const suffix = extension || extname(label) || '.bin'
  const file = join(directory, `${label.replace(/[^a-zA-Z0-9_.-]+/g, '-')}-${sha256(data).slice(0, 20)}${suffix}`)
  writeFileSync(file, data)
  return file
}

export function classifyProcessResult(result, expected = new Set([0, 1])) {
  const timedOut = result?.error?.code === 'ETIMEDOUT'
  const signalCrash = Boolean(result?.signal && !['SIGTERM'].includes(result.signal))
  const badExit = result?.status !== null && result?.status !== undefined && !expected.has(result.status)
  const launchError = Boolean(result?.error && !timedOut)
  if (timedOut) return { failed: true, kind: 'timeout' }
  if (signalCrash) return { failed: true, kind: 'signal' }
  if (badExit) return { failed: true, kind: 'exit' }
  if (launchError) return { failed: true, kind: 'launch' }
  return { failed: false, kind: 'ok' }
}
