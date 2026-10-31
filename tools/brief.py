#!/usr/bin/env python3
"""brief.py NAME|VA [...]: everything needed to write one function, in one place.

  1. signature, object file, size, status
  2. the typed Ghidra draft (build/drafts-typed, falls back to build/drafts) with hash names and the
     original switch case order
  3. callees: each called function's signature and whether we have it (MATCH/DIFF/unwritten)
  4. globals touched, with their declared types
  5. the most similar functions already written (tools/clones.py near) as templates
  6. prototype assert expressions for the same source file (docs/aug04_asserts.txt): original names
Then write the C++, `tools/build.py check UNIT -v`, iterate."""
import csv, os, re, sys, subprocess, bisect
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE); sys.path.insert(0, HERE)
from xbe import XBE
from capstone import Cs, CS_ARCH_X86, CS_MODE_32

FUN = list(csv.DictReader(open(os.path.join(ROOT, 'symbols', 'functions.csv'))))
SYM = list(csv.DictReader(open(os.path.join(ROOT, 'symbols', 'symbols.csv'))))
ST = {r['va']: r['status'] for r in csv.DictReader(open(os.path.join(ROOT, 'status', 'status.csv')))}
BYVA = {int(r['va'], 16): r for r in FUN}
SYMVA = {int(r['va'], 16): r for r in SYM}


def find(q):
    try:
        v = int(q, 16)
        if v in BYVA: return BYVA[v]
    except ValueError:
        pass
    hits = [r for r in FUN if q in (r['demangled'] or r['name'])]
    hits.sort(key=lambda r: (r['va'] in ST, -int(r['size'])))
    return hits[0] if hits else None


def draft(r):
    unit = r['obj'].replace('.obj', '')
    for d in ('drafts-typed', 'drafts'):
        p = os.path.join(ROOT, 'build', d, unit + '.c')
        if not os.path.exists(p): continue
        txt = open(p).read()
        i = txt.find(f'// ==== {r["va"]}')
        if i < 0: continue
        j = txt.find('// ==== ', i + 8)
        return d, txt[i:j if j > 0 else None]
    return None, None


def main():
    X = XBE(os.path.join(ROOT, 'orig', 'DEFAULT.XBE')); md = Cs(CS_ARCH_X86, CS_MODE_32)
    for q in sys.argv[1:]:
        r = find(q)
        if not r: print('no function', q); continue
        va, sz = int(r['va'], 16), int(r['size'])
        print(f"#### {r['va']} {r['obj']} size {sz} [{ST.get(r['va'], 'unwritten')}]\n#### {r['demangled'] or r['name']}\n")
        src, d = draft(r)
        print(f'---- draft ({src}) ----\n{d}' if d else '---- no draft ----')
        callees, globs = {}, {}
        for ins in md.disasm(X.read(va, sz), va):
            for h in re.findall(r'0x[0-9a-f]+', ins.op_str):
                v = int(h, 16)
                if ins.mnemonic == 'call' and v in BYVA and v != va: callees[v] = BYVA[v]
                elif v in SYMVA and v not in BYVA and 0x300000 <= v < 0x600000: globs[v] = SYMVA[v]
        print('---- callees ----')
        for v, c in sorted(callees.items(), key=lambda kv: kv[1]['obj']):
            print(f"  {ST.get(c['va'], 'unwritten'):9s} {c['obj']:28s} {(c['demangled'] or c['name'])[:110]}")
        print('---- globals ----')
        for v, g in sorted(globs.items()):
            print(f"  {g['va']} {g['obj']:24s} {(g['demangled'] or g['name'])[:100]}")
        print('---- similar written functions ----')
        try:
            out = subprocess.run([sys.executable, '-I', os.path.join(HERE, 'clones.py'), 'near', r['va'], '5'],
                                 capture_output=True, text=True, timeout=120).stdout
            print(out.rstrip() or '  (none above 0.5)')
        except Exception as e:
            print('  ', e)
        ap = os.path.join(ROOT, 'docs', 'aug04_asserts.txt')
        unit = r['obj'].replace('.obj', '')
        hits = [l.split('\t', 1)[1].strip() for l in open(ap) if '\t' in l and ('\\' + unit + '.') in l.split('\t')[0]]
        if hits:
            print('---- Aug 2004 prototype asserts in ' + unit + ' (original names) ----')
            for h in hits: print('  ' + h)
        print()


if __name__ == '__main__':
    if len(sys.argv) < 2: sys.exit(__doc__)
    main()
