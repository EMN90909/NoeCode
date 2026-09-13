const keywords=new Set(['select','insert','update','delete','create','drop','alter','from','where','into','values','set','join','left','right','inner','outer','on','group','by','order','limit','offset','begin','commit','rollback','as','and','or','not','null','true','false'])
export function tokenizeSql(input,{maxBytes=1024*1024,maxTokens=100000}={}){
  const text=String(input);if(Buffer.byteLength(text,'utf8')>maxBytes)throw new Error('SQL input exceeds size limit')
  const out=[];let i=0
  const push=(kind,value,start)=>{if(out.length>=maxTokens)throw new Error('SQL token limit exceeded');out.push({kind,value,start,end:i})}
  while(i<text.length){
    const c=text[i];if(/\s/.test(c)){i++;continue}
    if(c==='-'&&text[i+1]==='-'){i+=2;while(i<text.length&&text[i]!=='\n')i++;continue}
    if(c==='/'&&text[i+1]==='*'){const start=i;i+=2;let closed=false;while(i+1<text.length){if(text[i]==='*'&&text[i+1]==='/'){i+=2;closed=true;break}i++}if(!closed)throw new Error(`unterminated SQL comment at ${start}`);continue}
    const start=i
    if(c==="'"||c==='"'){
      const quote=c;i++;let value=''
      while(i<text.length){const n=text[i++];if(n===quote){if(text[i]===quote){value+=quote;i++;continue}push(quote==="'"?'string':'identifier',value,start);value=null;break}value+=n}
      if(value!==null)throw new Error(`unterminated SQL quoted value at ${start}`);continue
    }
    if(/[A-Za-z_]/.test(c)){i++;while(i<text.length&&/[A-Za-z0-9_$]/.test(text[i]))i++;const value=text.slice(start,i);push(keywords.has(value.toLowerCase())?'keyword':'identifier',value,start);continue}
    if(/[0-9]/.test(c)){i++;while(i<text.length&&/[0-9.eE+-]/.test(text[i]))i++;push('number',text.slice(start,i),start);continue}
    if(c==='?'||c==='$'){i++;while(i<text.length&&/[0-9]/.test(text[i]))i++;push('parameter',text.slice(start,i),start);continue}
    if('(),;.*=<>+-/%'.includes(c)){i++;if((c==='<'||c==='>'||c==='!')&&text[i]==='=')i++;push('punctuation',text.slice(start,i),start);continue}
    throw new Error(`unexpected SQL character ${JSON.stringify(c)} at ${i}`)
  }
  return out
}
export function parseSqlStatement(input,options){
  const tokens=tokenizeSql(input,options);if(!tokens.length)throw new Error('empty SQL statement')
  const first=tokens[0].value.toLowerCase(),allowed=new Set(['select','insert','update','delete','create','drop','alter','begin','commit','rollback'])
  if(!allowed.has(first))throw new Error(`unsupported SQL statement: ${tokens[0].value}`)
  let depth=0;for(const token of tokens){if(token.value==='(')depth++;else if(token.value===')'){if(--depth<0)throw new Error('unexpected SQL )')}}if(depth)throw new Error('unclosed SQL parenthesis')
  return {kind:first,tokens,parameterCount:tokens.filter(x=>x.kind==='parameter').length}
}
