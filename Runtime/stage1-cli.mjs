#!/usr/bin/env node
import {readFile} from 'node:fs/promises'
import {pathToFileURL} from 'node:url'
import {resolve} from 'node:path'

function usage(){
  console.log(`Noqeri stage-1 portable compiler kernel\n\nUsage:\n  noqeri --version\n  noqeri check <file.nqr>\n  noqeri lex-count <file.nqr>\n  noqeri fingerprint <file.nqr>\n  noqeri selftest\n\nThe stage-1 kernel owns source/token/syntax rules in Noqeri. Commands that require the still-migrating NIR/native/web/package subsystems remain available from the preserved bootstrap/cpp17-stage0 seed until their Noqeri ports land.`)
}

const modulePath=process.env.NOQERI_STAGE1_MODULE||resolve(process.cwd(),'build/noqeri-stage1.nqo')
let compiler
try{compiler=await import(pathToFileURL(modulePath).href)}catch(error){console.error(`noqeri: cannot load stage-1 module ${modulePath}: ${error.message}`);process.exit(2)}
const args=process.argv.slice(2)
const command=args[0]||'--help'
if(command==='--help'||command==='help'){usage();process.exit(0)}
if(command==='--version'){console.log(`Noqeri ${compiler.nqCompilerVersionMajor?.()??1}.${compiler.nqCompilerVersionMinor?.()??0}.${compiler.nqCompilerVersionPatch?.()??0} stage1`);process.exit(0)}
if(command==='selftest'){
  const sample=new TextEncoder().encode('function main(): int { return 0 }')
  const status=compiler.nqCompilerCheckSource(sample)
  const tokens=compiler.nqCompilerSourceTokenCount(sample)
  if(status!==0||tokens<6){console.error(`stage-1 selftest failed: status=${status} tokens=${tokens}`);process.exit(1)}
  console.log(`stage-1 selftest passed (${tokens} tokens)`);process.exit(0)
}
if(!['check','lex-count','fingerprint'].includes(command)){usage();process.exit(2)}
if(!args[1]){console.error(`noqeri: ${command} needs a source file`);process.exit(2)}
const source=new Uint8Array(await readFile(args[1]))
if(command==='check'){
  const status=Number(compiler.nqCompilerCheckSource(source))
  if(status===0){console.log(`${args[1]}: syntax/token check OK`);process.exit(0)}
  console.error(`${args[1]}: stage-1 check failed (${status})`);process.exit(1)
}
if(command==='lex-count'){console.log(String(compiler.nqCompilerSourceTokenCount(source)));process.exit(0)}
console.log(String(compiler.nqCompilerSourceFingerprint(source)));process.exit(0)
