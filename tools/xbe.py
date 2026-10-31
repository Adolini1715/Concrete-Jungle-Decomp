"""Minimal XBE reader: sections, VA->file offset, read bytes."""
import struct
class XBE:
    def __init__(self, path):
        self.d = d = open(path, 'rb').read()
        self.base = struct.unpack_from('<I', d, 0x104)[0]
        ns, sh = struct.unpack_from('<II', d, 0x11C)
        self.sections = []
        for i in range(ns):
            o = sh - self.base + i * 56
            fl, va, vs, ra, rs, no = struct.unpack_from('<IIIIII', d, o)
            n = d[no - self.base:no - self.base + 16].split(b'\0')[0].decode()
            self.sections.append(dict(name=n, va=va, vsize=vs, raw=ra, rsize=rs, flags=fl))
    def sec(self, name):
        return next(s for s in self.sections if s['name'] == name)
    def off(self, va):
        for s in self.sections:
            if s['va'] <= va < s['va'] + s['rsize']:
                return s['raw'] + va - s['va']
        raise KeyError(hex(va))
    def read(self, va, n):
        out = bytearray()
        while n > 0:
            s = next((s for s in self.sections if s['va'] <= va < s['va'] + max(s['vsize'], s['rsize'])), None)
            if s is None: raise KeyError(hex(va))
            k = min(n, s['va'] + max(s['vsize'], s['rsize']) - va)
            rel = va - s['va']
            chunk = self.d[s['raw'] + rel:s['raw'] + min(rel + k, s['rsize'])] if rel < s['rsize'] else b''
            out += chunk + b'\0' * (k - len(chunk)); va += k; n -= k
        return bytes(out)
