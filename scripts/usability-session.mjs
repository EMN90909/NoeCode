#!/usr/bin/env node
import { appendFile, readFile, writeFile, mkdir } from 'node:fs/promises'
import { dirname, resolve } from 'node:path'

const args=process.argv.slice(2)
const command=args[0]||'help'
const file=resolve(process.env.NOQERI_USABILITY_LOG||'research/usability-session.jsonl')
const now=()=>new Date().toISOString()
async function log(event){await mkdir(dirname(file),{recursive:true});await appendFile(file,JSON.stringify({timestamp:now(),...event})+'\n')}

if(command==='start'){
  const participant=args[1]||'anonymous',task=args[2]||'first-program',experience=args[3]||'unspecified'
  await log({event:'start',participant,task,experience,sessionId:`${Date.now()}-${Math.random().toString(36).slice(2,8)}`})
  console.log(`started ${participant} ${task}`)
}else if(command==='event'){
  const type=args[1]||'note',detail=args.slice(2).join(' ')
  await log({event:type,detail})
  console.log(`recorded ${type}`)
}else if(command==='finish'){
  await log({event:'finish',result:args[1]||'success',detail:args.slice(2).join(' ')})
  console.log('finished')
}else if(command==='summary'){
  let text='';try{text=await readFile(file,'utf8')}catch{}
  const events=text.split('\n').filter(Boolean).map(line=>JSON.parse(line))
  const sessions=[];let current=null
  for(const event of events){
    if(event.event==='start'){if(current)sessions.push(current);current={...event,events:[]}}
    else if(current){current.events.push(event);if(event.event==='finish'){current.finished=event;sessions.push(current);current=null}}
  }
  if(current)sessions.push(current)
  const rows=sessions.map(session=>{
    const start=Date.parse(session.timestamp),end=session.finished?Date.parse(session.finished.timestamp):null
    const events=session.events
    return {participant:session.participant,task:session.task,experience:session.experience,result:session.finished?.result||'incomplete',elapsedSeconds:end?Math.round((end-start)/1000):null,compilerErrors:events.filter(x=>x.event==='compiler-error').length,lookups:events.filter(x=>x.event==='lookup').length,interventions:events.filter(x=>x.event==='intervention').length}
  })
  const output=resolve(args[1]||'research/usability-summary.json')
  await writeFile(output,JSON.stringify({format:'noqeri-usability-summary-v1',generatedAt:now(),sessions:rows},null,2)+'\n')
  console.log(`${rows.length} sessions -> ${output}`)
}else{
  console.log('usage: usability-session.mjs start <participant> <task> <experience> | event <type> <detail> | finish <success|failure> [detail] | summary [output.json]')
}
