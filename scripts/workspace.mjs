#!/usr/bin/env node
import { resolve } from 'node:path'
import { spawnSync } from 'node:child_process'
import { loadWorkspace } from '../Runtime/workspace.mjs'

const command=process.argv[2]||'check'
const file=process.argv[3]||'workspace.nqr'
const compiler=resolve(process.env.NOQERI_BIN||'build/noqeri')
const workspace=await loadWorkspace(file)
let failed=0
for(const member of workspace.members){
  if(command==='list'){console.log(`${member.name}\t${member.path}`);continue}
  const args=command==='install'?['install']:['check']
  const result=spawnSync(compiler,args,{cwd:member.directory,encoding:'utf8',timeout:120000,env:{...process.env,NOQERI_OFFLINE:process.env.NOQERI_OFFLINE||'0'}})
  if(result.status!==0){failed++;console.error(`FAIL ${member.name}\n${result.stdout||''}${result.stderr||''}`)}
  else console.log(`PASS ${member.name} ${command}`)
}
if(!['check','install','list'].includes(command)){console.error('usage: workspace.mjs check|install|list [workspace.nqr]');process.exit(2)}
process.exit(failed?1:0)
