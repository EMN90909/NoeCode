function braceDelta(line) {
  let opens = 0, closes = 0, inString = false, escape = false, blockComment = false
  for (let i = 0; i < line.length; i++) {
    const c = line[i], next = line[i + 1]
    if (blockComment) { if (c === '*' && next === '/') { blockComment = false; i++ } continue }
    if (inString) { if (escape) { escape = false; continue } if (c === '\\') { escape = true; continue } if (c === '"') inString = false; continue }
    if (c === '/' && next === '/') break
    if (c === '/' && next === '*') { blockComment = true; i++; continue }
    if (c === '"') { inString = true; continue }
    if (c === '{') opens++
    else if (c === '}') closes++
  }
  return { opens, closes }
}

function leadingClosers(line) {
  let count = 0
  for (const c of line.trimStart()) { if (c === '}') count++; else break }
  return count
}

export function formatNoqeri(source, { indentWidth = 4 } = {}) {
  const input = String(source).replace(/\r\n?/g, '\n').split('\n')
  const output = []
  let indent = 0, blank = false
  for (const raw of input) {
    const trimmedRight = raw.replace(/[ \t]+$/g, '')
    const text = trimmedRight.trimStart()
    if (!text) {
      if (!blank && output.length) output.push('')
      blank = true
      continue
    }
    blank = false
    const closers = leadingClosers(text)
    const lineIndent = Math.max(0, indent - closers)
    output.push(' '.repeat(lineIndent * indentWidth) + text)
    const { opens, closes } = braceDelta(text)
    indent = Math.max(0, indent + opens - closes)
  }
  while (output.length && output.at(-1) === '') output.pop()
  return output.join('\n') + '\n'
}

export function isFormattedNoqeri(source, options) { return String(source).replace(/\r\n?/g, '\n') === formatNoqeri(source, options) }
