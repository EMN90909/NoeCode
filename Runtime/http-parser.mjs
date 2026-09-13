const token=/^[!#$%&'*+.^_`|~0-9A-Za-z-]+$/
function splitHead(input,maxHeaderBytes){
  const text=String(input);const marker=text.indexOf('\r\n\r\n');if(marker<0)throw new Error('HTTP header terminator not found')
  if(Buffer.byteLength(text.slice(0,marker+4),'utf8')>maxHeaderBytes)throw new Error('HTTP headers exceed size limit')
  return {head:text.slice(0,marker),body:text.slice(marker+4)}
}
function parseHeaders(lines,maxHeaders){
  if(lines.length>maxHeaders)throw new Error('too many HTTP headers')
  const headers=[];let contentLength=null
  for(const line of lines){
    if(!line)continue;if(/^[ \t]/.test(line))throw new Error('obsolete folded HTTP headers are rejected')
    const colon=line.indexOf(':');if(colon<=0)throw new Error('malformed HTTP header')
    const name=line.slice(0,colon),value=line.slice(colon+1).trim()
    if(!token.test(name))throw new Error(`invalid HTTP header name: ${name}`)
    if(/[\0\r\n]/.test(value))throw new Error('invalid HTTP header value')
    if(name.toLowerCase()==='content-length'){
      if(!/^\d+$/.test(value))throw new Error('invalid Content-Length')
      const parsed=Number(value);if(!Number.isSafeInteger(parsed))throw new Error('Content-Length too large')
      if(contentLength!==null&&contentLength!==parsed)throw new Error('conflicting Content-Length headers')
      contentLength=parsed
    }
    headers.push({name,value})
  }
  return {headers,contentLength}
}
export function parseHttpRequest(input,{maxHeaderBytes=64*1024,maxHeaders=128,maxBodyBytes=16*1024*1024}={}){
  const {head,body}=splitHead(input,maxHeaderBytes),lines=head.split('\r\n'),start=lines.shift()||''
  const match=/^([!#$%&'*+.^_`|~0-9A-Za-z-]+) ([^\s]+) HTTP\/(1\.0|1\.1)$/.exec(start)
  if(!match)throw new Error('invalid HTTP request line')
  const parsed=parseHeaders(lines,maxHeaders)
  const bodyBytes=Buffer.byteLength(body,'utf8');if(bodyBytes>maxBodyBytes)throw new Error('HTTP body exceeds size limit')
  if(parsed.contentLength!==null&&bodyBytes<parsed.contentLength)throw new Error('truncated HTTP body')
  return {kind:'request',method:match[1],target:match[2],version:match[3],headers:parsed.headers,contentLength:parsed.contentLength,body}
}
export function parseHttpResponse(input,{maxHeaderBytes=64*1024,maxHeaders=128,maxBodyBytes=16*1024*1024}={}){
  const {head,body}=splitHead(input,maxHeaderBytes),lines=head.split('\r\n'),start=lines.shift()||''
  const match=/^HTTP\/(1\.0|1\.1) ([1-5][0-9][0-9])(?: ([^\r\n]*))?$/.exec(start)
  if(!match)throw new Error('invalid HTTP status line')
  const parsed=parseHeaders(lines,maxHeaders),bodyBytes=Buffer.byteLength(body,'utf8');if(bodyBytes>maxBodyBytes)throw new Error('HTTP body exceeds size limit')
  if(parsed.contentLength!==null&&bodyBytes<parsed.contentLength)throw new Error('truncated HTTP body')
  return {kind:'response',version:match[1],status:Number(match[2]),reason:match[3]||'',headers:parsed.headers,contentLength:parsed.contentLength,body}
}
