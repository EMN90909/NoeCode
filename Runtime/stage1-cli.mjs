#!/usr/bin/env node
import {createHash} from 'node:crypto'
import {homedir} from 'node:os'
import {readFile,writeFile,mkdir,readdir,stat,rm,rename} from 'node:fs/promises'
import {pathToFileURL} from 'node:url'
import {resolve,join,relative,sep,dirname} from 'node:path'

function usage(){
  console.log(`Noqeri stage-1 portable compiler kernel\n\nUsage:\n  noqeri --version\n  noqeri check <file.nqr>\n  noqeri lex-count <file.nqr>\n  noqeri fingerprint <file.nqr>\n  noqeri add <namespace/name>[@version]\n  noqeri install\n  noqeri resolve <file.nqr>\n  noqeri selftest\n\nPackage source uses: import package "namespace/name". The host downloads and verifies source; language and package policy remain Noqeri-owned.`)
}

const modulePath=process.env.NOQERI_STAGE1_MODULE||resolve(process.cwd(),'build/noqeri-stage1.nqo')
let compiler
try{compiler=await import(pathToFileURL(modulePath).href)}catch(error){console.error(`noqeri: cannot load stage-1 module ${modulePath}: ${error.message}`);process.exit(2)}

const home=resolve(process.env.NOQERI_HOME||join(homedir(),'.noqeri'))
const cacheRoot=join(home,'cache','registry-v1')
const indexUrl=process.env.NOQERI_REGISTRY_INDEX||'https://raw.githubusercontent.com/EMN90909/noqeri-registry/main/registry/index.json'
const githubContents='https://api.github.com/repos/EMN90909/noqeri-registry/contents/'
const offline=process.env.NOQERI_OFFLINE==='1'
const maxFiles=256,maxBytes=8*1024*1024,maxIndexBytes=2*1024*1024
const coordinatePattern=/^[A-Za-z0-9_-]+\/[A-Za-z0-9_-]+$/
const versionPattern=/^\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?(?:\+[0-9A-Za-z.-]+)?$/

function compareText(a,b){return a<b?-1:a>b?1:0}
function safeRelative(path){if(!path||path.startsWith('/')||path.startsWith('\\')||path.includes('\\')||path.split('/').some(p=>p===''||p==='.'||p==='..'))throw new Error(`unsafe package path: ${path}`);return path}
function packageAlias(coordinate){return coordinate.split('/')[1].replace(/-/g,'_')}
function parseCoordinate(input){const at=input.lastIndexOf('@');let coordinate=input,version='';if(at>0){coordinate=input.slice(0,at);version=input.slice(at+1)}if(!coordinatePattern.test(coordinate))throw new Error('package must be namespace/name');if(version&&!versionPattern.test(version))throw new Error('invalid semantic version');return{coordinate,version}}

async function responseBytes(url,limit){if(offline)throw new Error('offline mode forbids network access');const response=await fetch(url,{headers:{Accept:'application/vnd.github+json','User-Agent':'noqeri-stage1'},redirect:'error',signal:AbortSignal.timeout(10000)});if(!response.ok)throw new Error(`download ${response.status}: ${url}`);const declared=Number(response.headers.get('content-length')||0);if(declared>limit)throw new Error('download exceeds size limit');const bytes=new Uint8Array(await response.arrayBuffer());if(bytes.byteLength>limit)throw new Error('download exceeds size limit');return bytes}
async function jsonFrom(url,limit=maxIndexBytes){const bytes=await responseBytes(url,limit);return JSON.parse(new TextDecoder().decode(bytes))}
async function registryIndex(){return jsonFrom(indexUrl)}
function chooseRelease(index,coordinate,requested){const [namespace,name]=coordinate.split('/');const pkg=index.packages?.find(p=>p.namespace===namespace&&p.name===name);if(!pkg)throw new Error(`package not found: ${coordinate}`);const releases=(pkg.versions||[]).filter(v=>!v.yanked&&versionPattern.test(v.version)).sort((a,b)=>compareText(b.version,a.version));const release=requested?releases.find(v=>v.version===requested):releases[0];if(!release)throw new Error(`no usable release for ${coordinate}${requested?'@'+requested:''}`);return{pkg,release}}

async function listPackageFiles(path,base,out,state){const listing=await jsonFrom(`${githubContents}${encodeURI(path)}?ref=main`,maxIndexBytes);if(!Array.isArray(listing))throw new Error('registry directory listing is invalid');for(const item of listing){if(item.type==='dir'){await listPackageFiles(item.path,base,out,state);continue}if(item.type!=='file')continue;if(++state.files>maxFiles)throw new Error('package file limit exceeded');const rel=safeRelative(item.path.slice(base.length+1));const bytes=await responseBytes(item.download_url,maxBytes-state.bytes);state.bytes+=bytes.byteLength;if(state.bytes>maxBytes)throw new Error('package size limit exceeded');out.push({name:rel,bytes})}}
function packageDigest(files){const hash=createHash('sha256');for(const file of [...files].sort((a,b)=>compareText(a.name,b.name))){hash.update(file.name);hash.update('\0');hash.update(file.bytes);hash.update('\0')}return `sha256:${hash.digest('hex')}`}
async function atomicPackageWrite(target,files,metadata){const temp=`${target}.tmp-${process.pid}-${Date.now()}`;await rm(temp,{recursive:true,force:true});await mkdir(temp,{recursive:true,mode:0o700});for(const file of files){const destination=join(temp,...safeRelative(file.name).split('/'));const rel=relative(temp,destination);if(rel==='..'||rel.startsWith('..'+sep))throw new Error('package path escaped cache');await mkdir(dirname(destination),{recursive:true,mode:0o700});await writeFile(destination,file.bytes,{mode:0o600})}await writeFile(join(temp,'.noqeri-integrity.json'),JSON.stringify(metadata,null,2)+'\n',{mode:0o600});await rm(target,{recursive:true,force:true});await mkdir(dirname(target),{recursive:true,mode:0o700});await rename(temp,target)}
async function fetchPackage(coordinate,requested=''){const index=await registryIndex();const{release}=chooseRelease(index,coordinate,requested);const [namespace,name]=coordinate.split('/');const target=join(cacheRoot,namespace,name,release.version);try{const meta=JSON.parse(await readFile(join(target,'.noqeri-integrity.json'),'utf8'));if(meta.coordinate===coordinate&&meta.version===release.version&&meta.integrity)return{...meta,path:target}}catch{}if(offline)throw new Error(`package is not cached: ${coordinate}@${release.version}`);const files=[],state={files:0,bytes:0};await listPackageFiles(release.path,release.path,files,state);if(!files.some(f=>f.name==='package.nqr'))throw new Error('package.nqr missing');const integrity=packageDigest(files);const manifest=new TextDecoder().decode(files.find(f=>f.name==='package.nqr').bytes);const entry=manifest.match(/\bentry\s*:\s*"([^"]+)"/)?.[1];if(!entry)throw new Error('package entry missing');safeRelative(entry);if(!files.some(f=>f.name===entry))throw new Error(`package entry not found: ${entry}`);const metadata={coordinate,version:release.version,integrity,entry,files:state.files,bytes:state.bytes};await atomicPackageWrite(target,files,metadata);return{...metadata,path:target}}

async function findProject(start=process.cwd()){let current=resolve(start);for(;;){const candidate=join(current,'project.nqr');try{await stat(candidate);return candidate}catch{}const parent=dirname(current);if(parent===current)throw new Error('project.nqr not found');current=parent}}
function findBlock(text,name){const match=new RegExp(`\\b${name}\\s*\\{`,'m').exec(text);if(!match)return null;let i=match.index+match[0].length,depth=1,inString=false,escape=false;for(;i<text.length;i++){const c=text[i];if(inString){if(escape){escape=false;continue}if(c==='\\'){escape=true;continue}if(c==='"')inString=false;continue}if(c==='"'){inString=true;continue}if(c==='{')depth++;if(c==='}'&&--depth===0)return{start:match.index,bodyStart:match.index+match[0].length,end:i}}throw new Error(`unterminated ${name} block`)}
function dependencies(text){const block=findBlock(text,'dependencies');if(!block)return[];const body=text.slice(block.bodyStart,block.end);const result=[];for(const match of body.matchAll(/^\s*([A-Za-z_][A-Za-z0-9_]*)\s*:\s*"([^"]+)"\s*$/gm)){const value=match[2];const at=value.lastIndexOf('@');if(at>0&&coordinatePattern.test(value.slice(0,at))&&versionPattern.test(value.slice(at+1)))result.push({alias:match[1],coordinate:value.slice(0,at),version:value.slice(at+1)})}return result}
function setDependency(text,coordinate,version){const alias=packageAlias(coordinate),line=`    ${alias}: "${coordinate}@${version}"`;const block=findBlock(text,'dependencies');if(!block)return text.replace(/\s*$/,'')+`\n\ndependencies {\n${line}\n}\n`;const body=text.slice(block.bodyStart,block.end);const lineRx=new RegExp(`^\\s*${alias}\\s*:\\s*"[^"]+"\\s*$`,'m');const nextBody=lineRx.test(body)?body.replace(lineRx,line):body.replace(/\s*$/,'')+`\n${line}\n`;return text.slice(0,block.bodyStart)+nextBody+text.slice(block.end)}
async function writeLock(projectPath,items){const sorted=[...items].sort((a,b)=>compareText(a.coordinate,b.coordinate));let output='noqeri-lock 3\n';for(const item of sorted)output+=`package ${item.alias} ${item.coordinate} ${item.version} ${item.integrity} ${item.entry}\n`;await writeFile(join(dirname(projectPath),'noqeri.lock'),output,{mode:0o600})}
async function installProject(){const projectPath=await findProject();const text=await readFile(projectPath,'utf8');const deps=dependencies(text);const installed=[];for(const dep of deps){const pkg=await fetchPackage(dep.coordinate,dep.version);installed.push({...dep,...pkg});console.log(`installed ${dep.coordinate}@${dep.version}`)}await writeLock(projectPath,installed);return installed}
async function addPackage(spec){const{coordinate,version}=parseCoordinate(spec);const pkg=await fetchPackage(coordinate,version);const projectPath=await findProject();const text=await readFile(projectPath,'utf8');await writeFile(projectPath,setDependency(text,coordinate,pkg.version));await installProject();console.log(`added ${coordinate}@${pkg.version}`)}
async function resolvePackageImports(file){const projectPath=await findProject(dirname(resolve(file)));const projectText=await readFile(projectPath,'utf8');const deps=dependencies(projectText);const source=await readFile(file,'utf8');const imports=[...source.matchAll(/\bimport\s+package\s+"([A-Za-z0-9_-]+\/[A-Za-z0-9_-]+)"/g)].map(m=>m[1]);const resolved=[];for(const coordinate of imports){const dep=deps.find(d=>d.coordinate===coordinate);if(!dep)throw new Error(`package import ${coordinate} is not declared in project.nqr; run noqeri add ${coordinate}`);const target=join(cacheRoot,...coordinate.split('/'),dep.version);let meta;try{meta=JSON.parse(await readFile(join(target,'.noqeri-integrity.json'),'utf8'))}catch{meta=await fetchPackage(coordinate,dep.version)}const entry=join(target,...safeRelative(meta.entry).split('/'));await stat(entry);resolved.push({coordinate,version:dep.version,entry,integrity:meta.integrity})}return resolved}

const args=process.argv.slice(2)
const command=args[0]||'--help'
try{
  if(command==='--help'||command==='help'){usage();process.exit(0)}
  if(command==='--version'){console.log(`Noqeri ${compiler.stage1VersionMajor?.()??1}.${compiler.stage1VersionMinor?.()??0}.${compiler.stage1VersionPatch?.()??0} stage1`);process.exit(0)}
  if(command==='selftest'){
    const sample=new TextEncoder().encode('function main(): int { return 0 }')
    const status=compiler.stage1CheckSource(sample),tokens=compiler.stage1TokenCount(sample)
    const pkg=new TextEncoder().encode('noqeri/supabase')
    if(status!==0||tokens<6||compiler.stage1PackageCoordinateValid?.(pkg)!==true){throw new Error(`stage-1 selftest failed: status=${status} tokens=${tokens}`)}
    console.log(`stage-1 selftest passed (${tokens} tokens; package policy OK)`);process.exit(0)
  }
  if(command==='add'){if(!args[1])throw new Error('noqeri add needs namespace/name[@version]');await addPackage(args[1]);process.exit(0)}
  if(command==='install'){await installProject();process.exit(0)}
  if(command==='resolve'){if(!args[1])throw new Error('noqeri resolve needs a source file');for(const item of await resolvePackageImports(args[1]))console.log(`${item.coordinate}@${item.version} -> ${item.entry} ${item.integrity}`);process.exit(0)}
  if(!['check','lex-count','fingerprint'].includes(command)){usage();process.exit(2)}
  if(!args[1])throw new Error(`noqeri: ${command} needs a source file`)
  if(command==='check')await resolvePackageImports(args[1])
  const source=new Uint8Array(await readFile(args[1]))
  if(command==='check'){
    const status=Number(compiler.stage1CheckSource(source))
    if(status===0){console.log(`${args[1]}: syntax/token check OK`);process.exit(0)}
    console.error(`${args[1]}: stage-1 check failed (${status})`);process.exit(1)
  }
  if(command==='lex-count'){console.log(String(compiler.stage1TokenCount(source)));process.exit(0)}
  console.log(String(compiler.stage1Fingerprint(source)));process.exit(0)
}catch(error){console.error(`noqeri: ${error.message}`);process.exit(1)}
