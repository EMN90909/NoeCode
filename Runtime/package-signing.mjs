import { createHash, createPublicKey, sign, verify } from 'node:crypto'

const checksumPattern = /^[A-Za-z0-9_-]+\/[A-Za-z0-9_-]+ \d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?(?:\+[0-9A-Za-z.-]+)? sha256:[0-9a-f]{64}$/

function canonicalRecord(record) {
  const text = String(record).trim()
  if (!checksumPattern.test(text)) throw new Error('invalid canonical package checksum record')
  return `${text}\n`
}

export function packageKeyId(publicKey) {
  const key = publicKey?.type === 'public' ? publicKey : createPublicKey(publicKey)
  const der = key.export({ type: 'spki', format: 'der' })
  return `ed25519:${createHash('sha256').update(der).digest('hex')}`
}

export function signPackageChecksum(privateKey, record) {
  const message = Buffer.from(canonicalRecord(record), 'utf8')
  return sign(null, message, privateKey).toString('base64')
}

export function verifyPackageChecksum(publicKey, record, signatureBase64) {
  if (!/^[A-Za-z0-9+/]+={0,2}$/.test(String(signatureBase64))) return false
  const message = Buffer.from(canonicalRecord(record), 'utf8')
  let signature
  try { signature = Buffer.from(signatureBase64, 'base64') } catch { return false }
  return signature.length > 0 && verify(null, message, publicKey, signature)
}

export function signedPackageRecord({ coordinate, version, integrity }, privateKey, publicKey) {
  const record = `${coordinate} ${version} ${integrity}`
  return {
    format: 'noqeri-signature-v1',
    algorithm: 'ed25519',
    keyId: packageKeyId(publicKey),
    record,
    signature: signPackageChecksum(privateKey, record),
  }
}

export function verifySignedPackageRecord(envelope, publicKey) {
  if (!envelope || envelope.format !== 'noqeri-signature-v1' || envelope.algorithm !== 'ed25519') return false
  if (envelope.keyId !== packageKeyId(publicKey)) return false
  return verifyPackageChecksum(publicKey, envelope.record, envelope.signature)
}
