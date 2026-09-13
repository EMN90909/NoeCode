#!/usr/bin/env node
import { cp, mkdir, readFile, rm, writeFile } from 'node:fs/promises'
import { homedir } from 'node:os'
import { dirname, join, resolve } from 'node:path'
import { parseLockfile } from '../Runtime/package-security.mjs'
import { createPackageManager } from '../Runtime/package-manager.mjs'

const root=resolve(process.argv[2]||process.cwd())
const vendorRoot=resolve(process.argv[3]||join(root,'vendor'))
const lock=parseLockfile(await readFile(join(root,'noqeri.lock'),'utf8'))
const manager=createPackageManager({offline:true})
await rm(vendorRoot,{recursive:true,force:true})
await mkdir(vendorRoot,{recursive:true})
const packages=[]
for(const item of lock){
  // fetchPackage in offline mode re-hashes the cache before returning it.
  const pkg=await manager.fetchPackage(item.coordinate,item.version)
  if(pkg.integrity!==item.integrity)throw new Error(`vendoring integrity mismatch: ${item.coordinate}@${item.version}`)
  const [namespace,name]=item.coordinate.split('/')
  const destination=join(vendorRoot,namespace,name,item.version)
  await mkdir(dirname(destination),{recursive:true})
  await cp(pkg.path,destination,{recursive:true,errorOnExist:true,force:false})
  packages.push({alias:item.alias,coordinate:item.coordinate,version:item.version,integrity:item.integrity,entry:item.entry,path:`${namespace}/${name}/${item.version}`})
}
await writeFile(join(vendorRoot,'noqeri-vendor.json'),JSON.stringify({format:'noqeri-vendor-v1',packages},null,2)+'\n')
console.log(`vendor: ${packages.length} verified packages -> ${vendorRoot}`)
