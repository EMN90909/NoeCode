import { readFile } from 'node:fs/promises'
import { dirname, join, resolve } from 'node:path'

const coordinatePattern = /^[A-Za-z0-9_-]+\/[A-Za-z0-9_-]+$/
const versionPattern = /^\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?(?:\+[0-9A-Za-z.-]+)?$/
const integrityPattern = /^sha256:[0-9a-f]{64}$/
const aliasPattern = /^[A-Za-z_][A-Za-z0-9_]*$/

function safeEntry(path) {
  return typeof path === 'string' && path.length > 0 && !path.startsWith('/') && !path.startsWith('\\') && !path.includes('\\') && !path.split('/').some(part => !part || part === '.' || part === '..')
}

export function parseLockfile(text) {
  const lines = String(text).replace(/\r\n/g, '\n').split('\n')
  if (lines.shift() !== 'noqeri-lock 3') throw new Error('unsupported or missing noqeri.lock header')
  const entries = []
  const coordinates = new Set(), aliases = new Set()
  let previous = ''
  for (const line of lines) {
    if (!line.trim()) continue
    const parts = line.split(' ')
    if (parts.length !== 6 || parts[0] !== 'package') throw new Error(`invalid lockfile record: ${line}`)
    const [, alias, coordinate, version, integrity, entry] = parts
    if (!aliasPattern.test(alias)) throw new Error(`invalid lock alias: ${alias}`)
    if (!coordinatePattern.test(coordinate)) throw new Error(`invalid lock coordinate: ${coordinate}`)
    if (!versionPattern.test(version)) throw new Error(`invalid lock version: ${coordinate}@${version}`)
    if (!integrityPattern.test(integrity)) throw new Error(`invalid lock integrity: ${coordinate}@${version}`)
    if (!safeEntry(entry)) throw new Error(`unsafe lock entry path: ${entry}`)
    if (coordinates.has(coordinate)) throw new Error(`duplicate lock coordinate: ${coordinate}`)
    if (aliases.has(alias)) throw new Error(`duplicate lock alias: ${alias}`)
    if (previous && coordinate < previous) throw new Error('noqeri.lock package records are not deterministically sorted')
    coordinates.add(coordinate); aliases.add(alias); previous = coordinate
    entries.push({ alias, coordinate, version, integrity, entry })
  }
  return entries
}

export function projectDependencies(text) {
  const source = String(text)
  const start = source.search(/\bdependencies\s*\{/)
  if (start < 0) return []
  const open = source.indexOf('{', start)
  let depth = 1, end = open + 1, inString = false, escape = false
  for (; end < source.length && depth > 0; end++) {
    const c = source[end]
    if (inString) { if (escape) { escape = false; continue } if (c === '\\') { escape = true; continue } if (c === '"') inString = false; continue }
    if (c === '"') { inString = true; continue }
    if (c === '{') depth++
    else if (c === '}') depth--
  }
  if (depth !== 0) throw new Error('unterminated dependencies block')
  const body = source.slice(open + 1, end - 1), result = []
  for (const match of body.matchAll(/^\s*([A-Za-z_][A-Za-z0-9_]*)\s*:\s*"([A-Za-z0-9_-]+\/[A-Za-z0-9_-]+)@(\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?(?:\+[0-9A-Za-z.-]+)?)"\s*$/gm)) {
    result.push({ alias: match[1], coordinate: match[2], version: match[3] })
  }
  return result.sort((a, b) => a.coordinate.localeCompare(b.coordinate))
}

export function verifyProjectLock(projectText, lockText) {
  const deps = projectDependencies(projectText), entries = parseLockfile(lockText)
  const byCoordinate = new Map(entries.map(entry => [entry.coordinate, entry]))
  for (const dep of deps) {
    const locked = byCoordinate.get(dep.coordinate)
    if (!locked) throw new Error(`dependency is not locked: ${dep.coordinate}`)
    if (locked.version !== dep.version) throw new Error(`lock version mismatch for ${dep.coordinate}: project=${dep.version} lock=${locked.version}`)
    if (locked.alias !== dep.alias) throw new Error(`lock alias mismatch for ${dep.coordinate}: project=${dep.alias} lock=${locked.alias}`)
  }
  return { dependencies: deps.length, lockedPackages: entries.length, entries }
}

export async function auditProject(start = process.cwd()) {
  const root = resolve(start)
  const projectPath = join(root, 'project.nqr'), lockPath = join(root, 'noqeri.lock')
  const [projectText, lockText] = await Promise.all([readFile(projectPath, 'utf8'), readFile(lockPath, 'utf8')])
  const result = verifyProjectLock(projectText, lockText)
  return { root: dirname(projectPath), ...result }
}

export function checksumRecord(entry) {
  if (!coordinatePattern.test(entry.coordinate || '') || !versionPattern.test(entry.version || '') || !integrityPattern.test(entry.integrity || '')) throw new Error('cannot create checksum record from invalid package metadata')
  return `${entry.coordinate} ${entry.version} ${entry.integrity}`
}
