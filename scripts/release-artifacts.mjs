#!/usr/bin/env node
import { createHash, createPrivateKey, createPublicKey, sign } from 'node:crypto'
import { readFile, readdir, stat, writeFile } from 'node:fs/promises'
import { basename, join, relative, resolve } from 'node:path'

const args=process.argv.slice(2)
const releaseDir=resolve(args[0]||'dist/release')
const version=args.find(x=>x.startsWith('--version='))?.split('=')[1]||process.env.NOQERI_RELEASE_VERSION||'0.0.0-dev'
const sourceCommit=process.env.NOQERI_SOURCE_COMMIT||'unknown'
const privateKeyPem=process.env.NOQERI_RELEASE_SIGNING_KEY||''

async function filesUnder(root){
  const out=[]
  async function walk(dir){
    for(const entry of (await readdir(dir,{withFileTypes:true})).sort((a,b)=>a.name.localeCompare(b.name))){
      const path=join(dir,entry.name)
      if(entry.isDirectory())await walk(path)
      else if(entry.isFile()&&!['SHA256SUMS','SHA256SUMS.sig','sbom.spdx.json','release-manifest.json'].includes(entry.name))out.push(path)
    }
  }
  await walk(root);return out
}
function sha256(bytes){return createHash('sha256').update(bytes).digest('hex')}

const files=await filesUnder(releaseDir)
if(!files.length)throw new Error(`release directory contains no artifacts: ${releaseDir}`)
const artifacts=[]
for(const path of files){
  const bytes=await readFile(path),info=await stat(path),name=relative(releaseDir,path).split('\\').join('/')
  artifacts.push({name,size:info.size,sha256:sha256(bytes)})
}
artifacts.sort((a,b)=>a.name.localeCompare(b.name))
const checksums=artifacts.map(a=>`${a.sha256}  ${a.name}`).join('\n')+'\n'
await writeFile(join(releaseDir,'SHA256SUMS'),checksums)

const sbom={
  spdxVersion:'SPDX-2.3',
  dataLicense:'CC0-1.0',
  SPDXID:'SPDXRef-DOCUMENT',
  name:`Noqeri-${version}`,
  documentNamespace:`https://noqeri.dev/spdx/${encodeURIComponent(version)}/${sourceCommit}`,
  creationInfo:{created:new Date(Number(process.env.SOURCE_DATE_EPOCH||Math.floor(Date.now()/1000))*1000).toISOString(),creators:['Tool: Noqeri release-artifacts.mjs']},
  packages:[{
    SPDXID:'SPDXRef-Package-Noqeri',name:'Noqeri',versionInfo:version,downloadLocation:'NOASSERTION',filesAnalyzed:true,licenseConcluded:'GPL-3.0-only',licenseDeclared:'GPL-3.0-only',copyrightText:'NOASSERTION',
    checksums:[{algorithm:'SHA256',checksumValue:sha256(Buffer.from(checksums))}]
  }],
  files:artifacts.map((a,i)=>({SPDXID:`SPDXRef-File-${i+1}`,fileName:a.name,checksums:[{algorithm:'SHA256',checksumValue:a.sha256}],licenseConcluded:'NOASSERTION',copyrightText:'NOASSERTION'}))
}
await writeFile(join(releaseDir,'sbom.spdx.json'),JSON.stringify(sbom,null,2)+'\n')

const manifest={format:'noqeri-release-v1',version,sourceCommit,sourceDateEpoch:process.env.SOURCE_DATE_EPOCH||null,artifacts,checksums:'SHA256SUMS',sbom:'sbom.spdx.json',signature:privateKeyPem?'SHA256SUMS.sig':null}
await writeFile(join(releaseDir,'release-manifest.json'),JSON.stringify(manifest,null,2)+'\n')

if(privateKeyPem){
  const key=createPrivateKey(privateKeyPem)
  const publicKey=createPublicKey(key)
  const signature=sign(null,Buffer.from(checksums),key).toString('base64')
  const spki=publicKey.export({type:'spki',format:'der'})
  const keyId=`ed25519:${sha256(spki)}`
  await writeFile(join(releaseDir,'SHA256SUMS.sig'),JSON.stringify({format:'noqeri-release-signature-v1',algorithm:'ed25519',keyId,signature},null,2)+'\n')
}
console.log(`release-artifacts: ${artifacts.length} artifacts; checksums + SPDX SBOM${privateKeyPem?' + Ed25519 signature':''}`)
