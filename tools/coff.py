"""Minimal i386 COFF .obj reader: sections, symbols, relocations."""
import struct
class Obj:
    def __init__(self, path):
        d = self.d = open(path, 'rb').read()
        mach, ns, ts, symptr, nsym, opt, ch = struct.unpack_from('<HHIIIHH', d, 0)
        strtab = symptr + nsym * 18
        def name8(b):
            if b[:4] == b'\0\0\0\0':
                o = strtab + struct.unpack_from('<I', b, 4)[0]
                return d[o:d.index(b'\0', o)].decode('latin-1')
            return b.split(b'\0')[0].decode('latin-1')
        self.sections = []
        for i in range(ns):
            o = 20 + opt + i * 40
            n = name8(d[o:o + 8])
            vs, va, rs, rp, relp, lnp, nrel, nln, fl = struct.unpack_from('<IIIIIIHHI', d, o + 8)
            relocs = []
            for r in range(nrel):
                rva, si, ty = struct.unpack_from('<IIH', d, relp + r * 10)
                relocs.append((rva, si, ty))
            self.sections.append(dict(name=n, data=d[rp:rp + rs] if rp else b'\0' * rs, relocs=relocs, flags=fl))
        self.symbols = {}
        i = 0
        while i < nsym:
            o = symptr + i * 18
            n = name8(d[o:o + 8])
            val, sec, ty, cls, naux = struct.unpack_from('<IhHBB', d, o + 8)
            self.symbols[i] = dict(name=n, value=val, sec=sec, type=ty, cls=cls)
            i += 1 + naux
    def functions(self):
        """(name, section index 1-based, offset, size) for external/static function symbols."""
        out = []
        bysec = {}
        for s in self.symbols.values():
            if s['sec'] > 0 and (s['type'] & 0x20) and s['cls'] in (2, 3):
                bysec.setdefault(s['sec'], []).append(s)
        for sec, syms in bysec.items():
            syms.sort(key=lambda s: s['value'])
            data = self.sections[sec - 1]['data']
            for k, s in enumerate(syms):
                end = syms[k + 1]['value'] if k + 1 < len(syms) else len(data)
                out.append((s['name'], sec, s['value'], end - s['value']))
        return out

    def data_symbols(self):
        """(name, section, offset, size) for external/static non-function symbols in initialised data sections."""
        out = []; bysec = {}
        for s in self.symbols.values():
            if s['sec'] > 0 and not (s['type'] & 0x20) and s['cls'] in (2, 3):
                sec = self.sections[s['sec'] - 1]
                if sec['flags'] & 0x20 or not (sec['flags'] & 0x40): continue  # code / not initialised data
                bysec.setdefault(s['sec'], []).append(s)
        for sec, syms in bysec.items():
            syms.sort(key=lambda s: s['value'])
            data = self.sections[sec - 1]['data']
            for k, s in enumerate(syms):
                end = syms[k + 1]['value'] if k + 1 < len(syms) else len(data)
                if end > s['value']: out.append((s['name'], sec, s['value'], end - s['value']))
        return out
