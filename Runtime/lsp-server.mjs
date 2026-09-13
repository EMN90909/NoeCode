import { stdin, stdout } from 'node:process'
import { formatNoqeri } from './formatter.mjs'

const encoder = new TextEncoder()
const decoder = new TextDecoder()
const documents = new Map()
const keywords = ['module','import','package','record','let','const','function','extern','export','if','else','while','return','throw','try','as','volatile','unsafe','true','false','null']
const semanticLegend = { tokenTypes: ['keyword','function','type','variable','string','number','comment'], tokenModifiers: ['declaration','readonly','unsafe'] }

function send(message) {
  const body = JSON.stringify(message)
  stdout.write(`Content-Length: ${Buffer.byteLength(body)}\r\n\r\n${body}`)
}
function respond(id, result) { send({ jsonrpc: '2.0', id, result }) }
function fail(id, code, message) { send({ jsonrpc: '2.0', id, error: { code, message } }) }
function notify(method, params) { send({ jsonrpc: '2.0', method, params }) }

function positionAt(text, offset) {
  const prefix = text.slice(0, Math.max(0, Math.min(offset, text.length))), lines = prefix.split('\n')
  return { line: lines.length - 1, character: lines.at(-1).length }
}
function offsetAt(text, position) {
  const lines = text.split('\n'); let offset = 0
  for (let line = 0; line < Math.min(position.line, lines.length); line++) offset += lines[line].length + 1
  return Math.min(text.length, offset + Math.min(position.character, lines[position.line]?.length ?? 0))
}
function rangeAt(text, start, end) { return { start: positionAt(text, start), end: positionAt(text, end) } }
function wordAt(text, position) {
  const offset = offsetAt(text, position); let start = offset, end = offset
  while (start > 0 && /[A-Za-z0-9_]/.test(text[start - 1])) start--
  while (end < text.length && /[A-Za-z0-9_]/.test(text[end])) end++
  return { word: text.slice(start, end), start, end }
}

function parseSymbols(text) {
  const symbols = []
  for (const match of text.matchAll(/\b(export\s+)?function\s+([A-Za-z_][A-Za-z0-9_]*)\s*(?:<[^>]+>)?\s*\(([^)]*)\)\s*(?::\s*([^\s{]+))?/g)) {
    symbols.push({ name: match[2], kind: 'function', start: match.index, end: match.index + match[0].length, signature: match[0].trim().replace(/\s*\{$/, ''), params: match[3] })
  }
  for (const match of text.matchAll(/\brecord\s+([A-Za-z_][A-Za-z0-9_]*)/g)) symbols.push({ name: match[1], kind: 'record', start: match.index, end: match.index + match[0].length, signature: match[0].trim() })
  for (const match of text.matchAll(/\b(let|const)\s+([A-Za-z_][A-Za-z0-9_]*)/g)) symbols.push({ name: match[2], kind: match[1], start: match.index, end: match.index + match[0].length, signature: match[0].trim() })
  return symbols.sort((a, b) => a.start - b.start)
}
function symbolKind(kind) { return kind === 'function' ? 12 : kind === 'record' ? 23 : kind === 'const' ? 14 : 13 }
function symbolFor(text, name) { return parseSymbols(text).find(item => item.name === name) }
function occurrences(text, name) {
  if (!name) return []
  const escaped = name.replace(/[.*+?^${}()|[\]\\]/g, '\\$&'), rx = new RegExp(`\\b${escaped}\\b`, 'g'), out = []
  for (const match of text.matchAll(rx)) out.push(rangeAt(text, match.index, match.index + name.length))
  return out
}

function structuralDiagnostic(text) {
  const stack = [], pairs = new Map([[')', '('],[']','['],['}','{']]); let inString = false, escape = false, lineComment = false, blockComment = false
  for (let i = 0; i < text.length; i++) {
    const c = text[i], next = text[i + 1]
    if (lineComment) { if (c === '\n') lineComment = false; continue }
    if (blockComment) { if (c === '*' && next === '/') { blockComment = false; i++ } continue }
    if (inString) { if (escape) { escape = false; continue } if (c === '\\') { escape = true; continue } if (c === '"') inString = false; continue }
    if (c === '/' && next === '/') { lineComment = true; i++; continue }
    if (c === '/' && next === '*') { blockComment = true; i++; continue }
    if (c === '"') { inString = true; continue }
    if ('([{'.includes(c)) stack.push({ c, i })
    else if (')]}'.includes(c)) { const open = stack.pop(); if (!open || open.c !== pairs.get(c)) return { range: rangeAt(text, i, i + 1), severity: 1, code: 'NQR-SYNTAX', message: `unexpected ${c}` } }
  }
  if (inString) return { range: rangeAt(text, Math.max(0, text.lastIndexOf('"')), text.length), severity: 1, code: 'NQR-SYNTAX', message: 'unterminated string literal' }
  if (blockComment) return { range: rangeAt(text, Math.max(0, text.lastIndexOf('/*')), text.length), severity: 1, code: 'NQR-SYNTAX', message: 'unterminated block comment' }
  if (stack.length) { const open = stack.at(-1); return { range: rangeAt(text, open.i, open.i + 1), severity: 1, code: 'NQR-SYNTAX', message: `unclosed ${open.c}` } }
  return null
}

function unsafeDiagnostic(text) {
  const named = /\b(host|abi|asm|intrinsic)\s*\(/g
  for (const match of text.matchAll(named)) {
    const prefix = text.slice(0, match.index), opens = [...prefix.matchAll(/\bunsafe\s*\{/g)].length, closes = [...prefix.matchAll(/}/g)].length
    if (opens <= closes) return { range: rangeAt(text, match.index, match.index + match[1].length), severity: 1, code: 'NQR-S3200', message: `${match[1]} requires unsafe { ... }`, source: 'noqeri' }
  }
  return null
}

function diagnostics(text) { return [structuralDiagnostic(text), unsafeDiagnostic(text)].filter(Boolean) }
function publish(uri) { const doc = documents.get(uri); if (doc) notify('textDocument/publishDiagnostics', { uri, version: doc.version, diagnostics: diagnostics(doc.text) }) }

function completionItems(text) {
  const items = keywords.map(word => ({ label: word, kind: 14, insertText: word }))
  for (const symbol of parseSymbols(text)) items.push({ label: symbol.name, kind: symbol.kind === 'function' ? 3 : symbol.kind === 'record' ? 7 : 6, detail: symbol.signature })
  return items
}

function semanticTokens(text) {
  const tokens = []
  function add(start, length, type, modifiers = 0) { const p = positionAt(text, start); tokens.push({ line: p.line, char: p.character, length, type, modifiers }) }
  for (const match of text.matchAll(/\/\/[^\n]*|\/\*[\s\S]*?\*\//g)) add(match.index, match[0].length, 6)
  for (const match of text.matchAll(/"(?:\\.|[^"\\])*"/g)) add(match.index, match[0].length, 4)
  for (const match of text.matchAll(/\b\d+(?:\.\d+)?\b/g)) add(match.index, match[0].length, 5)
  for (const word of keywords) for (const match of text.matchAll(new RegExp(`\\b${word}\\b`, 'g'))) add(match.index, word.length, 0, word === 'unsafe' ? 4 : 0)
  for (const symbol of parseSymbols(text)) add(text.indexOf(symbol.name, symbol.start), symbol.name.length, symbol.kind === 'function' ? 1 : symbol.kind === 'record' ? 2 : 3, symbol.kind === 'const' ? 3 : 1)
  tokens.sort((a,b)=>a.line-b.line||a.char-b.char)
  const data=[];let lastLine=0,lastChar=0
  for(const token of tokens){const dl=token.line-lastLine,dc=dl===0?token.char-lastChar:token.char;data.push(dl,dc,token.length,token.type,token.modifiers);lastLine=token.line;lastChar=token.char}
  return { data }
}

function signatureHelp(text, position) {
  const offset = offsetAt(text, position), prefix = text.slice(0, offset), match = /([A-Za-z_][A-Za-z0-9_]*)\s*\(([^()]*)$/.exec(prefix)
  if (!match) return null
  const symbol = symbolFor(text, match[1]); if (!symbol || symbol.kind !== 'function') return null
  const activeParameter = (match[2].match(/,/g) || []).length
  const parameters = symbol.params.split(',').map(item => item.trim()).filter(Boolean).map(label => ({ label }))
  return { signatures: [{ label: symbol.signature, parameters }], activeSignature: 0, activeParameter: Math.min(activeParameter, Math.max(0, parameters.length - 1)) }
}

async function handle(message) {
  const { id, method, params = {} } = message
  if (method === 'initialize') return respond(id, { capabilities: {
    textDocumentSync: 2,
    completionProvider: { triggerCharacters: ['.'] }, hoverProvider: true, definitionProvider: true, referencesProvider: true, renameProvider: { prepareProvider: true }, documentSymbolProvider: true,
    signatureHelpProvider: { triggerCharacters: ['(', ','] }, documentFormattingProvider: true, codeActionProvider: true,
    semanticTokensProvider: { legend: semanticLegend, full: true }
  }, serverInfo: { name: 'noqeri-stage1-lsp', version: '0.1' } })
  if (method === 'initialized') return
  if (method === 'shutdown') return respond(id, null)
  if (method === 'exit') return process.exit(0)
  if (method === 'textDocument/didOpen') { const d=params.textDocument; documents.set(d.uri,{text:d.text,version:d.version??0}); publish(d.uri); return }
  if (method === 'textDocument/didChange') { const d=documents.get(params.textDocument.uri); if(!d)return; for(const change of params.contentChanges||[]) if(typeof change.text==='string') d.text=change.text; d.version=params.textDocument.version??d.version; publish(params.textDocument.uri); return }
  if (method === 'textDocument/didClose') { documents.delete(params.textDocument.uri); notify('textDocument/publishDiagnostics',{uri:params.textDocument.uri,diagnostics:[]}); return }
  const uri=params.textDocument?.uri, doc=uri?documents.get(uri):null
  if (!doc && id !== undefined) return respond(id, null)
  if (method === 'textDocument/completion') return respond(id,{isIncomplete:false,items:completionItems(doc.text)})
  if (method === 'textDocument/hover') { const {word}=wordAt(doc.text,params.position), symbol=symbolFor(doc.text,word); return respond(id,symbol?{contents:{kind:'markdown',value:`\`${symbol.signature}\``}}:null) }
  if (method === 'textDocument/definition') { const {word}=wordAt(doc.text,params.position), symbol=symbolFor(doc.text,word); return respond(id,symbol?{uri,range:rangeAt(doc.text,symbol.start,symbol.end)}:null) }
  if (method === 'textDocument/references') { const {word}=wordAt(doc.text,params.position); return respond(id,occurrences(doc.text,word).map(range=>({uri,range}))) }
  if (method === 'textDocument/prepareRename') { const w=wordAt(doc.text,params.position); return respond(id,w.word?{range:rangeAt(doc.text,w.start,w.end),placeholder:w.word}:null) }
  if (method === 'textDocument/rename') { const {word}=wordAt(doc.text,params.position), edits=occurrences(doc.text,word).map(range=>({range,newText:params.newName})); return respond(id,{changes:{[uri]:edits}}) }
  if (method === 'textDocument/documentSymbol') return respond(id,parseSymbols(doc.text).map(s=>({name:s.name,kind:symbolKind(s.kind),range:rangeAt(doc.text,s.start,s.end),selectionRange:rangeAt(doc.text,doc.text.indexOf(s.name,s.start),doc.text.indexOf(s.name,s.start)+s.name.length)})))
  if (method === 'textDocument/signatureHelp') return respond(id,signatureHelp(doc.text,params.position))
  if (method === 'textDocument/semanticTokens/full') return respond(id,semanticTokens(doc.text))
  if (method === 'textDocument/formatting') return respond(id,[{range:{start:{line:0,character:0},end:positionAt(doc.text,doc.text.length)},newText:formatNoqeri(doc.text)}])
  if (method === 'textDocument/codeAction') { const actions=[]; for(const d of params.context?.diagnostics||[]) if(d.code==='NQR-S3200') actions.push({title:'Wrap operation in unsafe block',kind:'quickfix',diagnostics:[d],edit:{changes:{[uri]:[{range:d.range,newText:`unsafe { ${doc.text.slice(offsetAt(doc.text,d.range.start),offsetAt(doc.text,d.range.end))} }`}]}}}); return respond(id,actions) }
  if (id !== undefined) fail(id,-32601,`method not found: ${method}`)
}

export function runLspServer() {
  let buffer=Buffer.alloc(0)
  stdin.on('data',chunk=>{buffer=Buffer.concat([buffer,chunk]);for(;;){const headerEnd=buffer.indexOf('\r\n\r\n');if(headerEnd<0)break;const header=buffer.subarray(0,headerEnd).toString('ascii'),match=/Content-Length:\s*(\d+)/i.exec(header);if(!match){buffer=buffer.subarray(headerEnd+4);continue}const length=Number(match[1]),start=headerEnd+4;if(buffer.length<start+length)break;const body=buffer.subarray(start,start+length).toString('utf8');buffer=buffer.subarray(start+length);try{handle(JSON.parse(body)).catch(error=>console.error(error))}catch(error){console.error(error)}}})
  stdin.resume()
}

if (import.meta.url === `file://${process.argv[1]}`) runLspServer()
