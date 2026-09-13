#!/usr/bin/env node
import { mkdtemp, mkdir, rm, writeFile } from 'node:fs/promises'
import { tmpdir } from 'node:os'
import { join, resolve } from 'node:path'
import { spawnSync } from 'node:child_process'
import { parseSqlStatement } from '../Runtime/sql-parser.mjs'
import { parseHttpRequest, parseHttpResponse } from '../Runtime/http-parser.mjs'

const argv = process.argv.slice(2)
const value = name => { const i=argv.indexOf(name); return i>=0 ? argv[i+1] : '' }
const cases = Math.max(1, Number(value('--cases') || 250))
const originalSeed=Number(value('--seed') || 1314014546) >>> 0
let state = originalSeed
const compiler = resolve(process.env.NOQERI_BIN || join(process.cwd(),'build',process.platform==='win32'?'noqeri.exe':'noqeri'))
const timeout = Math.max(250, Number(process.env.NOQERI_FUZZ_TIMEOUT_MS || 4000))

function rand(){ state ^= state << 13; state ^= state >>> 17; state ^= state << 5; return state >>> 0 }
function pick(list){ return list[rand()%list.length] }
function mutate(seed){
  const alphabet='(){}[]<>+-*/%=!&|:;,._ abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"\\\n\t\r'
  let text=seed
  const edits=1+(rand()%12)
  for(let n=0;n<edits;n++){
    const op=rand()%5, at=text.length?rand()%text.length:0
    if(op===0) text=text.slice(0,at)+pick(alphabet)+text.slice(at)
    else if(op===1&&text.length) text=text.slice(0,at)+text.slice(at+1)
    else if(op===2&&text.length) text=text.slice(0,at)+pick(alphabet)+text.slice(at+1)
    else if(op===3&&text.length) text=text.slice(0,at)+text.slice(at,Math.min(text.length,at+1+(rand()%16)))+text.slice(at)
    else text += pick(['\0','/*','//','"','{','}','unsafe {','extern function ','0x','999999999999999999999999999'])
  }
  return text
}

const sourceSeeds=[
 'function main(): int { return 0 }\n',
 'record User { id: int, name: string }\nfunction main(): int { let x: int = 1 return x }\n',
 'function read(p: *int): int { unsafe { return *p } }\n',
 'function main(): int { let xs = [1,2,3] return xs[0] }\n'
]
const manifestSeeds=[
 'package { name: "fuzz/app" version: "1.0.0" edition: "2026" entry: "src/main.nqr" profile: "app" target: "x86_64-unknown-none" }\n',
 'package { name: "fuzz/lib" version: "0.1.0" edition: "2026" entry: "src/lib.nqr" profile: "library" target: "x86_64-unknown-none" }\n'
]
const nqdSeeds=[
 'table users { id: int key, name: text required }\ninsert users { id: 1, name: "Ada" }\nselect users where id = 1\n',
 'table events { id: int key, active: bool required }\ndelete events\n'
]
const sqlSeeds=[
 'SELECT id, name FROM users WHERE id = ?;',
 'INSERT INTO users(id,name) VALUES(?,?);',
 'UPDATE users SET name = ? WHERE id = ?;',
 'DELETE FROM users WHERE id = ?;'
]
const networkSeeds=[
 'GET / HTTP/1.1\r\nHost: example.test\r\nConnection: close\r\n\r\n',
 'POST /api HTTP/1.1\r\nHost: example.test\r\nContent-Length: 2\r\n\r\n{}',
 'HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n'
]

function run(args,cwd){
  const result=spawnSync(compiler,args,{cwd,encoding:'utf8',timeout,env:{...process.env,NOQERI_OFFLINE:'1',NOQERI_ASSEMBLER:'',NOQERI_LINKER:''}})
  if(result.error?.code==='ETIMEDOUT') throw new Error(`hang: ${args.join(' ')}`)
  if(result.signal) throw new Error(`crash ${result.signal}: ${args.join(' ')}\n${result.stderr||''}`)
  return result
}
function runHook(command,file,label){
  const r=spawnSync(command,[file],{timeout,encoding:'utf8'})
  if(r.error?.code==='ETIMEDOUT')throw new Error(`${label} parser hang`)
  if(r.signal)throw new Error(`${label} parser crash ${r.signal}`)
  return r
}
function controlledParse(fn,input){try{fn(input)}catch(error){if(!(error instanceof Error))throw error}}

const root=await mkdtemp(join(tmpdir(),'noqeri-fuzz-'))
let executed=0
try{
  await mkdir(join(root,'src'),{recursive:true})
  for(let i=0;i<cases;i++){
    const source=mutate(pick(sourceSeeds)), sourcePath=join(root,'src',`case-${i}.nqr`)
    await writeFile(sourcePath,source)
    run(['lex',sourcePath],root); executed++
    run(['check',sourcePath],root); executed++

    const project=mutate(pick(manifestSeeds))
    await writeFile(join(root,'project.nqr'),project)
    run(['manifest',join(root,'project.nqr')],root); executed++

    const nqd=mutate(pick(nqdSeeds)), nqdPath=join(root,`case-${i}.nqd`)
    await writeFile(nqdPath,nqd)
    run(['db',nqdPath,join(root,`case-${i}.nqdb`)],root); executed++

    const sql=mutate(pick(sqlSeeds)), sqlPath=join(root,`case-${i}.sql`)
    await writeFile(sqlPath,sql);controlledParse(parseSqlStatement,sql);executed++
    if(process.env.NOQERI_SQL_FUZZ_CMD){runHook(process.env.NOQERI_SQL_FUZZ_CMD,sqlPath,'SQL');executed++}

    const net=mutate(pick(networkSeeds)), netPath=join(root,`case-${i}.net`)
    await writeFile(netPath,net)
    controlledParse(net.startsWith('HTTP/')?parseHttpResponse:parseHttpRequest,net);executed++
    if(process.env.NOQERI_NETWORK_FUZZ_CMD){runHook(process.env.NOQERI_NETWORK_FUZZ_CMD,netPath,'network');executed++}

    // Every object case goes through Noqeri's bounded ObjectInspector. Rejection
    // is expected for malformed bytes; crashes, signals and hangs are failures.
    const objectPath=join(root,`case-${i}.o`)
    await writeFile(objectPath,Buffer.from(mutate('\x7fELF000000000000'),'binary'))
    run(['inspect-object',objectPath],root);executed++
    if(process.env.NOQERI_OBJECT_FUZZ_CMD){runHook(process.env.NOQERI_OBJECT_FUZZ_CMD,objectPath,'object');executed++}
  }
  console.log(`fuzz: PASS cases=${cases} invocations=${executed} seed=${originalSeed}`)
} finally { await rm(root,{recursive:true,force:true}) }
