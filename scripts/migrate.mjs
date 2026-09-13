#!/usr/bin/env node
import { readFile, writeFile, readdir } from 'node:fs/promises'
import { join, resolve } from 'node:path'

const args=process.argv.slice(2)
const root=resolve(args.find(x=>!x.startsWith('--'))||process.cwd())
const apply=args.includes('--apply')
const rulePath=resolve(args.find(x=>x.startsWith('--rules='))?.split('=')[1]||join(process.cwd(),'migrations','rules.json'))
const config=JSON.parse(await readFile(rulePath,'utf8'))
if(config.format!=='noqeri-migrations-v1'||!Array.isArray(config.rules))throw new Error('unsupported migration rules format')
const rules=config.rules.filter(r=>r.enabled)
const extensions=new Set(['.nqr','.md','.json','.toml','.yml','.yaml'])
const files=[]
async function walk(dir){
  for(const entry of await readdir(dir,{withFileTypes:true})){
    if(['.git','build','dist','node_modules','vendor'].includes(entry.name))continue
    const path=join(dir,entry.name)
    if(entry.isDirectory())await walk(path)
    else if(entry.isFile()&&[...extensions].some(ext=>entry.name.endsWith(ext)))files.push(path)
  }
}
await walk(root)
let changed=0,replacements=0
for(const file of files){
  let text=await readFile(file,'utf8'),next=text,fileReplacements=0
  for(const rule of rules){
    if(rule.kind!=='literal')throw new Error(`unsupported migration rule kind: ${rule.kind}`)
    if(!rule.search)throw new Error(`migration rule ${rule.id} has empty search`)
    const parts=next.split(rule.search)
    const count=parts.length-1
    if(count){next=parts.join(rule.replace??'');fileReplacements+=count}
  }
  if(next!==text){changed++;replacements+=fileReplacements;console.log(`${apply?'APPLY':'WOULD-CHANGE'} ${file} replacements=${fileReplacements}`);if(apply)await writeFile(file,next)}
}
console.log(`migrate: mode=${apply?'apply':'preview'} files=${files.length} changed=${changed} replacements=${replacements} rules=${rules.length}`)
