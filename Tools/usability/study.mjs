#!/usr/bin/env node
import { appendFileSync, existsSync, mkdirSync, readFileSync } from 'node:fs'
import { dirname, resolve } from 'node:path'
import { randomUUID } from 'node:crypto'
import { fileURLToPath } from 'node:url'

const root = resolve(fileURLToPath(new URL('../..', import.meta.url)))
const tasksPath = resolve(root, 'Tools', 'usability', 'tasks.json')
const tasks = JSON.parse(readFileSync(tasksPath, 'utf8'))
const validLanguages = new Set(tasks.languages || [])
const taskById = new Map((tasks.tasks || []).map(task => [task.id, task]))
const eventKinds = new Set(['docs', 'hint', 'compiler-error', 'runtime-error', 'test-failure', 'restart', 'blocked', 'other'])

function usage() {
  console.log(`Noqeri beginner usability study runner\n\nUsage:\n  node Tools/usability/study.mjs start --participant=P001 --language=noqeri --task=first-program [--log=path]\n  node Tools/usability/study.mjs event --session=<id> --kind=docs [--detail=short-text] [--log=path]\n  node Tools/usability/study.mjs finish --session=<id> --success=true [--reason=short-text] [--log=path]\n  node Tools/usability/study.mjs report [path] [--json]\n\nThe log defaults to build/usability/sessions.jsonl. Participant values must be opaque study codes, not names or email addresses.`)
}

function parseArgs(argv) {
  const options = new Map()
  const positional = []
  for (const arg of argv) {
    if (arg.startsWith('--')) {
      const raw = arg.slice(2)
      const split = raw.indexOf('=')
      if (split === -1) options.set(raw, 'true')
      else options.set(raw.slice(0, split), raw.slice(split + 1))
    } else positional.push(arg)
  }
  return { options, positional }
}
function option(options, name, required = false) {
  const value = options.get(name)
  if (required && (!value || value === 'true')) throw new Error(`--${name}=... is required`)
  return value
}
function logPath(options) { return resolve(option(options, 'log') || resolve(root, 'build', 'usability', 'sessions.jsonl')) }
function shortText(value, field) {
  if (value == null) return null
  const text = String(value).trim()
  if (text.length > 500) throw new Error(`${field} must be 500 characters or fewer`)
  return text
}
function append(path, value) {
  mkdirSync(dirname(path), { recursive: true })
  appendFileSync(path, JSON.stringify(value) + '\n', 'utf8')
}
function readEvents(path) {
  if (!existsSync(path)) return []
  return readFileSync(path, 'utf8').split(/\r?\n/).filter(Boolean).map((line, index) => {
    try { return JSON.parse(line) } catch { throw new Error(`invalid JSONL at ${path}:${index + 1}`) }
  })
}
function stateFor(events, sessionId) {
  const own = events.filter(event => event.session_id === sessionId)
  return { start: own.find(event => event.type === 'start') || null, finish: own.find(event => event.type === 'finish') || null, events: own.filter(event => event.type === 'event') }
}
function boolValue(value, name) {
  if (value === 'true' || value === '1' || value === 'yes') return true
  if (value === 'false' || value === '0' || value === 'no') return false
  throw new Error(`--${name} must be true or false`)
}
function percentile(values, p) {
  if (!values.length) return null
  const sorted = [...values].sort((a, b) => a - b)
  if (sorted.length === 1) return sorted[0]
  const index = (sorted.length - 1) * p
  const lower = Math.floor(index), upper = Math.ceil(index)
  if (lower === upper) return sorted[lower]
  const weight = index - lower
  return sorted[lower] * (1 - weight) + sorted[upper] * weight
}
function rounded(value) { return value == null ? null : Math.round(value * 1000) / 1000 }

function startStudy(options) {
  const participant = option(options, 'participant', true)
  if (!/^[A-Za-z0-9._-]{1,64}$/.test(participant)) throw new Error('--participant must be an opaque 1-64 character study code')
  const language = option(options, 'language', true).toLowerCase()
  const taskId = option(options, 'task', true)
  if (!validLanguages.has(language)) throw new Error(`unsupported language: ${language}`)
  const task = taskById.get(taskId)
  if (!task) throw new Error(`unknown task: ${taskId}`)
  if (Array.isArray(task.languages) && !task.languages.includes(language)) throw new Error(`${language} is not enrolled for task ${taskId}`)
  const sessionId = randomUUID()
  const path = logPath(options)
  append(path, {
    schema: 1,
    type: 'start',
    session_id: sessionId,
    participant_id: participant,
    language,
    task_id: taskId,
    category: task.category,
    at: new Date().toISOString()
  })
  console.log(`session=${sessionId}`)
  console.log(`task=${taskId}`)
  console.log(`language=${language}`)
  console.log(`log=${path}`)
}

function recordEvent(options) {
  const sessionId = option(options, 'session', true)
  const kind = option(options, 'kind', true)
  if (!eventKinds.has(kind)) throw new Error(`unsupported event kind: ${kind}`)
  const path = logPath(options)
  const state = stateFor(readEvents(path), sessionId)
  if (!state.start) throw new Error('session not found')
  if (state.finish) throw new Error('session is already finished')
  append(path, {
    schema: 1,
    type: 'event',
    session_id: sessionId,
    kind,
    detail: shortText(option(options, 'detail'), 'detail'),
    at: new Date().toISOString()
  })
  console.log(`recorded ${kind} for ${sessionId}`)
}

function finishStudy(options) {
  const sessionId = option(options, 'session', true)
  const success = boolValue(option(options, 'success', true), 'success')
  const path = logPath(options)
  const state = stateFor(readEvents(path), sessionId)
  if (!state.start) throw new Error('session not found')
  if (state.finish) throw new Error('session is already finished')
  const now = new Date()
  const started = new Date(state.start.at)
  if (!Number.isFinite(started.getTime()) || now < started) throw new Error('session start timestamp is invalid')
  append(path, {
    schema: 1,
    type: 'finish',
    session_id: sessionId,
    success,
    reason: shortText(option(options, 'reason'), 'reason'),
    at: now.toISOString(),
    elapsed_seconds: rounded((now.getTime() - started.getTime()) / 1000)
  })
  console.log(`finished ${sessionId}: success=${success} elapsed_seconds=${rounded((now.getTime() - started.getTime()) / 1000)}`)
}

function buildReport(events, source) {
  const starts = events.filter(event => event.type === 'start')
  const groups = new Map()
  for (const start of starts) {
    const state = stateFor(events, start.session_id)
    const key = `${start.task_id}|${start.language}`
    if (!groups.has(key)) groups.set(key, { task_id: start.task_id, category: start.category, language: start.language, sessions: [] })
    let elapsed = null
    if (state.finish) {
      const startMs = Date.parse(start.at), finishMs = Date.parse(state.finish.at)
      if (Number.isFinite(startMs) && Number.isFinite(finishMs) && finishMs >= startMs) elapsed = (finishMs - startMs) / 1000
    }
    groups.get(key).sessions.push({ start, finish: state.finish, events: state.events, elapsed })
  }

  const summaries = [...groups.values()].map(group => {
    const completed = group.sessions.filter(session => session.finish)
    const successful = completed.filter(session => session.finish.success === true && session.elapsed != null)
    const durations = successful.map(session => session.elapsed)
    const helpEvents = group.sessions.flatMap(session => session.events).filter(event => event.kind === 'docs' || event.kind === 'hint').length
    const errorEvents = group.sessions.flatMap(session => session.events).filter(event => event.kind === 'compiler-error' || event.kind === 'runtime-error' || event.kind === 'test-failure').length
    return {
      task_id: group.task_id,
      category: group.category,
      language: group.language,
      started: group.sessions.length,
      completed: completed.length,
      incomplete: group.sessions.length - completed.length,
      successes: successful.length,
      failures: completed.length - successful.length,
      success_rate: completed.length ? rounded(successful.length / completed.length) : null,
      successful_time_seconds: {
        median: rounded(percentile(durations, 0.5)),
        p90: rounded(percentile(durations, 0.9)),
        min: durations.length ? rounded(Math.min(...durations)) : null,
        max: durations.length ? rounded(Math.max(...durations)) : null
      },
      help_events: helpEvents,
      error_events: errorEvents
    }
  }).sort((a, b) => a.task_id.localeCompare(b.task_id) || a.language.localeCompare(b.language))

  return {
    schema: 1,
    generated_at: new Date().toISOString(),
    source,
    study: tasks.study_name,
    policy: 'Aggregates are descriptive evidence only. Do not infer language superiority without comparable cohorts, sufficient samples and a declared analysis plan.',
    session_count: starts.length,
    groups: summaries
  }
}

function reportStudy(options, positional) {
  const path = resolve(positional[0] || option(options, 'log') || resolve(root, 'build', 'usability', 'sessions.jsonl'))
  const report = buildReport(readEvents(path), path)
  if (options.has('json')) {
    console.log(JSON.stringify(report, null, 2))
    return
  }
  console.log(`study=${report.study}`)
  console.log(`sessions=${report.session_count}`)
  for (const group of report.groups) {
    console.log(`${group.task_id}\t${group.language}\tstarted=${group.started}\tcompleted=${group.completed}\tsuccess=${group.successes}\tmedian_s=${group.successful_time_seconds.median ?? 'n/a'}\tp90_s=${group.successful_time_seconds.p90 ?? 'n/a'}\thelp=${group.help_events}\terrors=${group.error_events}`)
  }
}

try {
  const command = process.argv[2]
  const { options, positional } = parseArgs(process.argv.slice(3))
  if (!command || command === 'help' || command === '--help') usage()
  else if (command === 'start') startStudy(options)
  else if (command === 'event') recordEvent(options)
  else if (command === 'finish') finishStudy(options)
  else if (command === 'report') reportStudy(options, positional)
  else { usage(); process.exitCode = 2 }
} catch (error) {
  console.error(`usability-study: ${error.message}`)
  process.exitCode = 1
}
