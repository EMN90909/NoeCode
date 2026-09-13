function cleanDoc(lines) {
  return lines.map(line => line.replace(/^\s*\/\/\/\s?/, '').trimEnd()).join('\n').trim()
}

export function extractNoqeriApi(source) {
  const text = String(source).replace(/\r\n?/g, '\n')
  const lines = text.split('\n')
  const moduleName = text.match(/^\s*module\s+([A-Za-z_][A-Za-z0-9_.]*)/m)?.[1] || ''
  const declarations = []
  let pendingDocs = []
  for (let index = 0; index < lines.length; index++) {
    const line = lines[index]
    if (/^\s*\/\/\//.test(line)) { pendingDocs.push(line); continue }
    if (!line.trim()) { if (pendingDocs.length) pendingDocs.push('///'); continue }
    const fn = line.match(/^\s*export\s+function\s+([A-Za-z_][A-Za-z0-9_]*)\s*(<[^>]+>)?\s*\(([^)]*)\)\s*(?::\s*([^\s{]+))?/)
    if (fn) {
      declarations.push({ kind: 'function', name: fn[1], signature: line.trim().replace(/\s*\{.*$/, ''), docs: cleanDoc(pendingDocs), line: index + 1 })
      pendingDocs = []
      continue
    }
    const record = line.match(/^\s*(?:export\s+)?record\s+([A-Za-z_][A-Za-z0-9_]*)/)
    if (record) {
      declarations.push({ kind: 'record', name: record[1], signature: line.trim(), docs: cleanDoc(pendingDocs), line: index + 1 })
      pendingDocs = []
      continue
    }
    pendingDocs = []
  }
  return { module: moduleName, declarations }
}

export function renderNoqeriMarkdown(api, { sourcePath = '' } = {}) {
  const title = api.module || sourcePath || 'Noqeri API'
  const out = [`# ${title}`, '']
  if (sourcePath) out.push(`Source: \`${sourcePath}\``, '')
  if (!api.declarations.length) { out.push('_No exported API declarations found._', ''); return out.join('\n') }
  for (const item of api.declarations) {
    out.push(`## ${item.name}`, '', `\`${item.signature}\``, '')
    if (item.docs) out.push(item.docs, '')
    out.push(`Defined on line ${item.line}.`, '')
  }
  return out.join('\n')
}
