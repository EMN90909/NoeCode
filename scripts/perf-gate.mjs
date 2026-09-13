#!/usr/bin/env node
import { readFile } from 'node:fs/promises'
import { resolve } from 'node:path'

const args=process.argv.slice(2)
const currentPath=resolve(args[0]||'Benchmarks/results/latest.json')
const baselinePath=resolve(args[1]||'Benchmarks/results/baseline.json')
const thresholdArg=args.find(x=>x.startsWith('--threshold='))
const defaultThreshold=Number(thresholdArg?.split('=')[1]||8)
const current=JSON.parse(await readFile(currentPath,'utf8'))
const baseline=JSON.parse(await readFile(baselinePath,'utf8'))
const key=x=>`${x.workload}:${x.language}`
const prior=new Map((baseline.results||[]).filter(x=>x.status==='ok').map(x=>[key(x),x]))
let failures=0,compared=0
for(const sample of current.results||[]){
  if(sample.status!=='ok'||sample.language!=='noqeri')continue
  const base=prior.get(key(sample));if(!base)continue
  const currentValue=sample.wallMs?.median,baseValue=base.wallMs?.median
  if(!(currentValue>0)||!(baseValue>0))continue
  compared++
  const regression=((currentValue-baseValue)/baseValue)*100
  const threshold=Number(sample.regressionThresholdPercent||defaultThreshold)
  const status=regression>threshold?'FAIL':'PASS'
  console.log(`${status} ${sample.workload}: baseline=${baseValue.toFixed(3)}ms current=${currentValue.toFixed(3)}ms delta=${regression.toFixed(2)}% threshold=${threshold}%`)
  if(regression>threshold)failures++
}
if(!compared){console.error('perf-gate: no comparable Noqeri benchmark results');process.exit(2)}
console.log(`perf-gate: compared=${compared} regressions=${failures}`)
process.exit(failures?1:0)
