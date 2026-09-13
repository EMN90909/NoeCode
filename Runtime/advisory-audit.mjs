import { readFile } from 'node:fs/promises'
import { join, resolve } from 'node:path'
import { parseLockfile } from './package-security.mjs'

function parseVersion(text){const m=/^(\d+)\.(\d+)\.(\d+)(?:-[0-9A-Za-z.-]+)?(?:\+[0-9A-Za-z.-]+)?$/.exec(text);return m?[Number(m[1]),Number(m[2]),Number(m[3])]:null}
function compare(a,b){const av=parseVersion(a),bv=parseVersion(b);if(!av||!bv)return null;for(let i=0;i<3;i++){if(av[i]<bv[i])return-1;if(av[i]>bv[i])return 1}return 0}
function matchesComparator(version,expr){const m=/^(<=|>=|<|>|=)?\s*(\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?(?:\+[0-9A-Za-z.-]+)?)$/.exec(expr.trim());if(!m)return version===expr.trim();const c=compare(version,m[2]);if(c===null)return false;switch(m[1]||'='){case'<':return c<0;case'<=':return c<=0;case'>':return c>0;case'>=':return c>=0;default:return c===0}}
function affected(version,ranges){return (ranges||[]).some(range=>String(range).split(',').every(part=>matchesComparator(version,part)))}

export async function auditAdvisories(root=process.cwd(),advisoryPath=''){
  const base=resolve(root)
  const lock=parseLockfile(await readFile(join(base,'noqeri.lock'),'utf8'))
  const path=advisoryPath?resolve(advisoryPath):resolve(base,'security','advisories.json')
  let db
  try{db=JSON.parse(await readFile(path,'utf8'))}catch(error){if(error.code==='ENOENT')return {checked:lock.length,findings:[],database:null};throw error}
  if(db.format!=='noqeri-advisories-v1'||!Array.isArray(db.advisories))throw new Error('unsupported advisory database')
  const findings=[]
  for(const pkg of lock){
    for(const advisory of db.advisories){
      if(advisory.package!==pkg.coordinate)continue
      if(!affected(pkg.version,advisory.affected))continue
      findings.push({id:advisory.id,package:pkg.coordinate,version:pkg.version,severity:advisory.severity||'unknown',summary:advisory.summary||'',fixed:advisory.fixed||[],url:advisory.url||''})
    }
  }
  return {checked:lock.length,findings,database:path}
}
