#!/usr/bin/env node
import assert from 'node:assert/strict'
import { XorShift32, classifyProcessResult, failureKey, minimizeSequence, mutateText, sha256 } from '../fuzz/engine.mjs'

const first = new XorShift32(12345)
const second = new XorShift32(12345)
for (let i = 0; i < 128; i++) assert.equal(first.next(), second.next(), 'same seed must reproduce the same stream')

const a = mutateText('function main(): int { return 0 }', new XorShift32(77))
const b = mutateText('function main(): int { return 0 }', new XorShift32(77))
assert.equal(a, b, 'text mutation must be replayable')
assert.notEqual(a, 'function main(): int { return 0 }')

const minimized = minimizeSequence('prefix-CRASH-suffix', value => String(value).includes('CRASH'))
assert.equal(minimized, 'CRASH')

assert.deepEqual(classifyProcessResult({ status: 0, signal: null }, new Set([0, 1])), { failed: false, kind: 'ok' })
assert.equal(classifyProcessResult({ status: 9, signal: null }, new Set([0, 1])).kind, 'exit')
assert.equal(classifyProcessResult({ status: null, signal: 'SIGSEGV' }, new Set([0, 1])).kind, 'signal')
assert.equal(classifyProcessResult({ status: null, signal: null, error: { code: 'ETIMEDOUT' } }, new Set([0, 1])).kind, 'timeout')

const key1 = failureKey({ label: 'parser', kind: 'signal', signal: 'SIGSEGV', status: null, stderr: 'boom\nmore' })
const key2 = failureKey({ label: 'parser', kind: 'signal', signal: 'SIGSEGV', status: null, stderr: 'boom\ndifferent tail' })
assert.equal(key1, key2, 'crash dedupe key should ignore unstable stderr tails')
assert.equal(sha256('abc'), 'ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad')

console.log('fuzz engine tests passed')
