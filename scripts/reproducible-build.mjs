#!/usr/bin/env node
import { mkdtemp, readdir, readFile, rm, stat } from 'node:fs/promises'
import { tmpdir } from 'node:os'
import { join, relative, resolve } from 'node:path'
import { createHash } from 'node:crypto'
import { spawnSync } from 'node:child_process'

const command=process.env.NOQERI_REPRO_BUILD_CMD
if(!command){console.error('reproducible-build: set NOQERI_REPRO_BUILD_CMD; use $NOQERI_REPRO_OUT as output directory');process.exit(2)}
const sourceDateEpoch=process.env.SOURCE_DATE_EPOCH||'1704067200'
const shell=process.platform==='win32'?'cmd':'sh',shellArgs=process.platform==='win32'?['/d','/s','/c']:['-lc']

async function digestTree(root){
  const files=[]
  async function walk(dir){
    for(const entry of (await readdir(dir,{withFileTypes:true})).sort((a,b)=>a.name.localeCompare(b.name))){
      const path=join(dir,entry.name)
      if(entry.isDirectory())await walk(path)
      else if(entry.isFile())files.push(path)
    }
  }
  await walk(root)
  const hash=createHash('sha256'),records=[]
  for(const file of files){
    const rel=relative(root,file).split('\\').join('/'),bytes=await readFile(file),sum=createHash('sha256').update(bytes).digest('hex')
    records.push({path:rel,size:bytes.length,sha256:sum});hash.update(rel);hash.update('\0');hash.update(bytes);hash.update('\0')
  }
  return {sha256:hash.digest('hex'),files:records}
}

async function build(label){
  const out=await mkdtemp(join(tmpdir(),`noqeri-repro-${label}-`))
  const result=spawnSync(shell,[...shellArgs,command],{cwd:resolve(process.cwd()),encoding:'utf8',timeout:20*60*1000,env:{...process.env,SOURCE_DATE_EPOCH:sourceDateEpoch,NOQERI_REPRO_OUT:out,TZ:'UTC',LC_ALL:'C'}})
  if(result.status!==0){await rm(out,{recursive:true,force:true});throw new Error(`build ${label} failed (${result.status})\n${result.stdout||''}${result.stderr||''}`)}
  const tree=await digestTree(out);return {out,tree}
}

let a,b
try{
  a=await build('a');b=await build('b')
  if(a.tree.sha256!==b.tree.sha256){
    const right=new Map(b.tree.files.map(x=>[x.path,x]));
    const changed=a.tree.files.filter(x=>right.get(x.path)?.sha256!==x.sha256).map(x=>x.path)
    console.error(`reproducible-build: FAIL\na=${a.tree.sha256}\nb=${b.tree.sha256}\nchanged=${changed.slice(0,50).join(',')}`);process.exitCode=1
  }else console.log(`reproducible-build: PASS sha256:${a.tree.sha256} files=${a.tree.files.length} SOURCE_DATE_EPOCH=${sourceDateEpoch}`)
} finally {if(a)await rm(a.out,{recursive:true,force:true});if(b)await rm(b.out,{recursive:true,force:true})}
