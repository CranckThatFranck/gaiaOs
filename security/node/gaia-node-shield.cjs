'use strict';
/**
 * Gaia OS - Node.js Defense-in-Depth Runtime Shield
 * Protects Node.js processes against:
 * - CVE-2026-111497 (Node2Shell: Brotli decoder out-of-bounds write in node:zlib)
 * - CVE-2026-105482 (dns.resolve / lookup hostname truncation via embedded NUL bytes)
 * - CVE-2026-105483 (TLS hostname validation bypass via full-width digits)
 * - CVE-2026-106069 & CVE-2026-106072 (TLS validation bypass via non-canonical IPv4 hostnames)
 */

const dns = require('node:dns');
const tls = require('node:tls');
const zlib = require('node:zlib');

// 1. CVE-2026-105482 Guard: Reject hostnames containing NUL (\0) or control characters
function assertSafeHostname(hostname, apiName) {
  if (typeof hostname === 'string') {
    if (hostname.includes('\0') || /[\x00-\x1f\x7f]/.test(hostname)) {
      const err = new Error(`[GaiaOS Security Shield] Blocked ${apiName}: Hostname contains NUL or control characters (CVE-2026-105482)`);
      err.code = 'ERR_GAIA_UNSAFE_HOSTNAME';
      throw err;
    }
    // Block full-width digits (U+FF10 to U+FF19) in hostnames (CVE-2026-105483)
    if (/[\uFF10-\uFF19]/.test(hostname)) {
      const err = new Error(`[GaiaOS Security Shield] Blocked ${apiName}: Hostname contains full-width digits (CVE-2026-105483)`);
      err.code = 'ERR_GAIA_FULLWIDTH_HOSTNAME';
      throw err;
    }
  }
}

// Check for non-canonical IPv4 formats (octal 0177.0.0.1, hex 0x7f.0.0.1, integer 2130706433, 2/3-part IPv4)
function isNonCanonicalIPv4(host) {
  if (typeof host !== 'string') return false;
  // Pure decimal integer IP
  if (/^\d+$/.test(host)) return true;
  const parts = host.split('.');
  if (parts.length >= 2 && parts.length <= 4 && parts.every(p => /^(0x[0-9a-fA-F]+|\d+)$/.test(p))) {
    if (parts.length !== 4) return true;
    for (const part of parts) {
      if (part.startsWith('0x') || part.startsWith('0X')) return true;
      if (part.length > 1 && part.startsWith('0')) return true;
      const n = Number(part);
      if (!Number.isInteger(n) || n < 0 || n > 255) return true;
    }
  }
  return false;
}

// Wrap DNS functions (CVE-2026-105482)
const dnsMethodsToWrap = [
  'lookup', 'resolve', 'resolve4', 'resolve6', 'resolveAny',
  'resolveCname', 'resolveCaa', 'resolveMx', 'resolveNaptr',
  'resolveNs', 'resolvePtr', 'resolveSoa', 'resolveSrv', 'resolveTxt'
];

for (const method of dnsMethodsToWrap) {
  if (typeof dns[method] === 'function') {
    const original = dns[method];
    dns[method] = function gaiaGuardedDns(hostname, ...args) {
      assertSafeHostname(hostname, `dns.${method}`);
      return original.call(this, hostname, ...args);
    };
  }
  if (dns.promises && typeof dns.promises[method] === 'function') {
    const originalPromise = dns.promises[method];
    dns.promises[method] = async function gaiaGuardedDnsPromise(hostname, ...args) {
      assertSafeHostname(hostname, `dns.promises.${method}`);
      return originalPromise.call(this, hostname, ...args);
    };
  }
}

// 2. CVE-2026-105483, CVE-2026-106069, CVE-2026-106072 Guard: TLS checkServerIdentity
const origCheckServerIdentity = tls.checkServerIdentity;
tls.checkServerIdentity = function gaiaGuardedCheckServerIdentity(hostname, cert) {
  assertSafeHostname(hostname, 'tls.checkServerIdentity');
  if (isNonCanonicalIPv4(hostname)) {
    const err = new Error(`[GaiaOS Security Shield] Blocked TLS handshake with non-canonical IPv4 hostname "${hostname}" (CVE-2026-106069 / CVE-2026-106072)`);
    err.code = 'ERR_TLS_CERT_ALTNAME_INVALID';
    return err;
  }
  return origCheckServerIdentity.call(this, hostname, cert);
};

// 3. CVE-2026-111497 (Node2Shell) Guard: Enforce safe output/buffer bounds on Brotli decompression
const DEFAULT_MAX_BROTLI_OUTPUT = 256 * 1024 * 1024; // 256 MiB ceiling against OOB/zip-bomb allocation
const origBrotliDecompress = zlib.brotliDecompress;
const origBrotliDecompressSync = zlib.brotliDecompressSync;

function sanitizeBrotliOptions(options) {
  if (typeof options === 'function' || options === undefined || options === null) {
    return { maxOutputLength: DEFAULT_MAX_BROTLI_OUTPUT };
  }
  if (typeof options === 'object') {
    return {
      ...options,
      maxOutputLength: options.maxOutputLength || DEFAULT_MAX_BROTLI_OUTPUT
    };
  }
  return options;
}

zlib.brotliDecompress = function gaiaGuardedBrotliDecompress(buf, options, callback) {
  if (typeof options === 'function') {
    return origBrotliDecompress.call(this, buf, sanitizeBrotliOptions( undefined ), options);
  }
  return origBrotliDecompress.call(this, buf, sanitizeBrotliOptions(options), callback);
};

zlib.brotliDecompressSync = function gaiaGuardedBrotliDecompressSync(buf, options) {
  return origBrotliDecompressSync.call(this, buf, sanitizeBrotliOptions(options));
};

module.exports = {
  assertSafeHostname,
  isNonCanonicalIPv4,
  shieldVersion: '1.0.0-gaiaos'
};
