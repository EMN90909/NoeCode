#!/usr/bin/env node
import assert from 'node:assert/strict'
import { buildSourceSeed, cmakeBuildArgs, cmakeConfigureArgs, seedCandidates } from '../../scripts/source-bootstrap.mjs'

const linux = seedCandidates('/tmp/noqeri-seed', 'linux')
assert.ok(linux.some(path => path.endsWith('/noqeri')))
const windows = seedCandidates('C:/tmp/noqeri-seed', 'win32')
assert.ok(windows.some(path => path.toLowerCase().endsWith('noqeri.exe')))

const configure = cmakeConfigureArgs('/repo', '/repo/build/stage0-seed')
assert.deepEqual(configure.slice(0, 4), ['-S', '/repo', '-B', '/repo/build/stage0-seed'])
assert.ok(configure.includes('-DBUILD_TESTING=OFF'))
const build = cmakeBuildArgs('/repo/build/stage0-seed')
assert.ok(build.includes('--target'))
assert.ok(build.includes('noqeri'))

const planned = buildSourceSeed({ root: '/repo with spaces', buildDir: '/tmp/seed with spaces', cmake: 'cmake', dryRun: true })
assert.match(planned.configure, /"\/repo with spaces"/)
assert.match(planned.build, /noqeri/)
assert.ok(planned.candidates.length >= 2)

console.log('source bootstrap planning tests passed')
