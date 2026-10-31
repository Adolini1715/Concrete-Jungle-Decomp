#!/usr/bin/env python3
"""dis.py NAME|VA ... : disassemble map functions with symbolized targets."""
import sys, os, csv, re, struct
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE); sys.path.insert(0, HERE)
from xbe import XBE
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
rows = list(csv.DictReader(open(os.path.join(ROOT, 'symbols', 'symbols.csv'))))
byva = {}
for r in rows: byva.setdefault(int(r['va'], 16), r)
vas = sorted(byva)
import bisect
def sym(a):
    i = bisect.bisect_right(vas, a) - 1
    if i < 0: return None
    r = byva[vas[i]]; off = a - vas[i]
    if off > int(r['size']) + 0x1000: return None
    n = r['demangled'] if len(r['demangled']) < 80 else r['name']
    return n + (f'+{off:#x}' if off else '')
x = XBE(os.path.join(ROOT, 'orig', 'DEFAULT.XBE'))
try:
    from hashnames import annotate as hash_annotate   # game hash IDs -> HT_* / text / STATE_ names
except Exception:
    hash_annotate = lambda l: l
md = Cs(CS_ARCH_X86, CS_MODE_32)
def find(q):
    if re.fullmatch(r'(0x)?[0-9a-fA-F]{5,8}', q): return byva[int(q, 16)]
    for r in rows:
        if r['name'] == q: return r
    c = [r for r in rows if q in r['demangled']]
    return c[0] if c else None
for q in sys.argv[1:]:
    r = find(q); va = int(r['va'], 16); n = int(r['size'])
    print(f"; {r['va']} size {n} {r['obj']} [{r['flags']}]\n; {r['name']}\n; {r['demangled']}")
    for i in md.disasm(x.read(va, n), va):
        if i.mnemonic == 'int3': break
        s = ''
        for m in re.findall(r'0x[0-9a-f]{5,8}', i.op_str):
            v = int(m, 16); t = sym(v)
            # immediates are often plain constants: only name them when they hit a symbol start,
            # or when they are branch targets / memory operands
            if t and ('+' not in t or i.mnemonic in ('call', 'jmp') or i.mnemonic.startswith('j') or '[' in i.op_str): s += '  ; ' + t
        print(hash_annotate(f'  {i.address:06x}  {i.bytes.hex():20s} {i.mnemonic} {i.op_str}{s}'))
    print()
