import { readFile, stat } from 'node:fs/promises'
import { dirname, join, resolve, relative, sep } from 'node:path'

function safeMember(root,value){
  if(typeof value!=='string'||!value||value.includes('\\'))throw new Error(`invalid workspace member: ${value}`)
  const absolute=resolve(root,value)
  const rel=relative(root,absolute)
  if(rel==='..'||rel.startsWith('..'+sep))throw new Error(`workspace member escapes root: ${value}`)
  return absolute
}

export function parseWorkspace(text){
  const source=String(text).replace(/\/\/[^\n]*/g,' ')
  const block=/\bworkspace\s*\{([\s\S]*?)\}/m.exec(source)
  if(!block)throw new Error('workspace.nqr must contain workspace { ... }')
  const members=[]
  const memberBlock=/\bmembers\s*:\s*\[([\s\S]*?)\]/m.exec(block[1])
  if(!memberBlock)throw new Error('workspace requires members: ["path", ...]')
  for(const match of memberBlock[1].matchAll(/"([^"\\]+)"/g))members.push(match[1])
  if(!members.length)throw new Error('workspace must contain at least one member')
  if(new Set(members).size!==members.length)throw new Error('workspace contains duplicate member paths')
  return {members}
}

export async function loadWorkspace(path='workspace.nqr'){
  const file=resolve(path),root=dirname(file),definition=parseWorkspace(await readFile(file,'utf8'))
  const members=[]
  const names=new Set()
  for(const value of definition.members){
    const directory=safeMember(root,value),manifest=join(directory,'project.nqr')
    try{await stat(manifest)}catch{throw new Error(`workspace member has no project.nqr: ${value}`)}
    const text=await readFile(manifest,'utf8')
    const name=/\bname\s*:\s*"([^"]+)"/.exec(text)?.[1]||value
    if(names.has(name))throw new Error(`duplicate workspace project name: ${name}`)
    names.add(name);members.push({name,path:value,directory,manifest})
  }
  return {root,file,members}
}
