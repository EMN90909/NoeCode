#!/usr/bin/env node
import { readFile, writeFile, mkdir } from 'node:fs/promises'
import { resolve, join } from 'node:path'
import { spawnSync } from 'node:child_process'
import os from 'node:os'

const root=resolve(process.cwd())
const suite=JSON.parse(await readFile(join(root,'benchmarks','suite.json'),'utf8'))
const args=process.argv.slice(2)
const wanted=new Set((args.find(x=>x.startsWith('--workloads='))?.split('=')[1]||'').split(',').filter(Boolean))
const languages=(args.find(x=>x.startsWith('--languages='))?.split('=')[1]||'noqeri,c,cpp,rust,zig,go').split(',').filter(Boolean)
const output=resolve(args.find(x=>x.startsWith('--output='))?.split('=')[1]||join(root,'benchmarks','results','latest.json'))
const runs=Number(args.find(x=>x.startsWith('--runs='))?.split('=')[1]||suite.policy.measurementRuns)
const warmups=Number(args.find(x=>x.startsWith('--warmups='))?.split('=')[1]||suite.policy.warmupRuns)

function commandFor(language,workload){
  const key=`NOQERI_BENCH_${language.toUpperCase().replace(/[^A-Z0-9]/g,'_')}_${workload.toUpperCase().replace(/[^A-Z0-9]/g,'_')}`
  const raw=process.env[key]
  if(!raw)return null
  const shell=process.platform==='win32'?'cmd':'sh'
  const prefix=process.platform==='win32'?['/d','/s','/c']:['-lc']
  return {command:shell,args:[...prefix,raw],display:raw}
}
function runOne(command,timeoutMs){
  const started=process.hrtime.bigint()
  const result=spawnSync(command.command,command.args,{cwd:root,encoding:'utf8',timeout:timeoutMs,env:{...process.env,NOQERI_OFFLINE:'1'}})
  const elapsed=Number(process.hrtime.bigint()-started)/1e6
  if(result.error?.code==='ETIMEDOUT')return {ok:false,error:'timeout',elapsedMs:elapsed}
  if(result.status!==0)return {ok:false,error:`exit ${result.status}`,elapsedMs:elapsed,stderr:(result.stderr||'').slice(0,1000)}
  const metricMatch=(result.stdout||'').match(/NOQERI_BENCH_METRIC=([0-9]+(?:\.[0-9]+)?)/)
  return {ok:true,elapsedMs:elapsed,reportedMetric:metricMatch?Number(metricMatch[1]):null}
}
function stats(values){
  const sorted=[...values].sort((a,b)=>a-b)
  const median=sorted[Math.floor(sorted.length/2)]
  const mean=values.reduce((a,b)=>a+b,0)/values.length
  const variance=values.reduce((a,b)=>a+(b-mean)**2,0)/values.length
  return {n:values.length,min:sorted[0],median,mean,max:sorted.at(-1),stddev:Math.sqrt(variance)}
}

const timeoutMs=suite.policy.timeoutSeconds*1000
const results=[]
for(const workload of suite.workloads){
  if(wanted.size&&!wanted.has(workload.id))continue
  for(const language of languages){
    const command=commandFor(language,workload.id)
    if(!command){results.push({workload:workload.id,language,status:'skipped',reason:'command not configured'});continue}
    let failed=null
    for(let i=0;i<warmups;i++){const sample=runOne(command,timeoutMs);if(!sample.ok){failed=sample;break}}
    if(failed){results.push({workload:workload.id,language,status:'failed',...failed});continue}
    const elapsed=[],reported=[]
    for(let i=0;i<runs;i++){
      const sample=runOne(command,timeoutMs)
      if(!sample.ok){failed=sample;break}
      elapsed.push(sample.elapsedMs)
      if(sample.reportedMetric!==null)reported.push(sample.reportedMetric)
    }
    if(failed){results.push({workload:workload.id,language,status:'failed',...failed});continue}
    results.push({workload:workload.id,language,status:'ok',command:command.display,wallMs:stats(elapsed),reported:reported.length?stats(reported):null,metric:workload.metric})
  }
}
const document={
  format:'noqeri-benchmark-results-v1',
  generatedAt:new Date().toISOString(),
  methodology:{warmups,runs,timeoutMs,claimPolicy:suite.policy.claimPolicy},
  system:{platform:process.platform,arch:process.arch,node:process.version,cpu:os.cpus()[0]?.model||'unknown',logicalCpus:os.cpus().length,memoryBytes:os.totalmem()},
  results
}
await mkdir(resolve(output,'..'),{recursive:true})
await writeFile(output,JSON.stringify(document,null,2)+'\n')
const ok=results.filter(x=>x.status==='ok').length,failed=results.filter(x=>x.status==='failed').length,skipped=results.filter(x=>x.status==='skipped').length
console.log(`benchmark: ${ok} measured, ${skipped} skipped, ${failed} failed -> ${output}`)
process.exit(failed?1:0)
