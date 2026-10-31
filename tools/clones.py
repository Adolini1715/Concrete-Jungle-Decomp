#!/usr/bin/env python3
"""clones.py: find unwritten functions that are copies (or near copies) of functions we already have.

  clones.py scan            build build/clones.json (normalised instruction streams of every game/engine function)
  clones.py report [N]      unwritten functions with a written twin, biggest first
  clones.py families [N]    groups of unwritten functions that are clones of each other (write one, get all)
  clones.py near VA [N]     the N most similar written functions to VA (template to start from)

Normalisation: capstone instructions, mnemonic + operands with absolute addresses / call targets replaced by
the symbol they hit (level 'sym') or by a placeholder (level 'shape'). Equal 'sym' streams mean the same
source with the same callees; equal 'shape' means same code with different callees/globals (template or
macro instance: the twin's source with names swapped). Similarity for `near` is difflib over 'shape' tokens."""
import csv, json, os, re, sys, collections, difflib, hashlib
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE); sys.path.insert(0, HERE)
from xbe import XBE
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
OUT = os.path.join(ROOT, 'build', 'clones.json')

def load_funcs():
    fs = []
    for r in csv.DictReader(open(os.path.join(ROOT, 'symbols', 'functions.csv'))):
        if r['origin'] in ('game', 'enginex'):
            fs.append((int(r['va'], 16), int(r['size']), r['name'], r['demangled'] or r['name'], r['obj'], r['flags']))
    return fs

def status_map():
    st = {}
    for r in csv.DictReader(open(os.path.join(ROOT, 'status', 'status.csv'))):
        if r['va'] != 'va': st[int(r['va'], 16)] = r['status']
    return st

HEX = re.compile(r'0x[0-9a-f]+')

def scan():
    X = XBE(os.path.join(ROOT, 'orig', 'DEFAULT.XBE')); md = Cs(CS_ARCH_X86, CS_MODE_32)
    fs = load_funcs()
    names = {va: n for va, sz, n, d, o, f in fs}
    import bisect
    starts = sorted(names)
    lo, hi = 0x11000, 0x400000
    def sym(v):
        i = bisect.bisect_right(starts, v) - 1
        if i >= 0 and starts[i] == v: return names[v]
        return 'D'
    db = {}
    for va, sz, n, d, o, f in fs:
        try: code = X.read(va, sz)
        except Exception: continue
        s, sh = [], []
        for ins in md.disasm(code, va):
            if ins.mnemonic == 'int3': break
            ops = ins.op_str
            def rs(m):
                v = int(m.group(0), 16)
                return sym(v) if lo <= v < hi else m.group(0)
            def rsh(m):
                v = int(m.group(0), 16)
                return 'A' if lo <= v < hi else m.group(0)
            if ins.mnemonic.startswith('j') or ins.mnemonic == 'call':
                # local branch targets: relative offset inside the function
                m = HEX.fullmatch(ops)
                if m:
                    t = int(ops, 16)
                    if va <= t < va + sz:
                        s.append(f'{ins.mnemonic} L{t - va:x}'); sh.append(s[-1]); continue
            s.append(ins.mnemonic + ' ' + HEX.sub(rs, ops)); sh.append(ins.mnemonic + ' ' + HEX.sub(rsh, ops))
        db['%08x' % va] = {'n': n, 'd': d, 'o': o, 'sz': sz,
                           'sym': hashlib.md5('\n'.join(s).encode()).hexdigest(),
                           'shape': hashlib.md5('\n'.join(sh).encode()).hexdigest(), 'tok': sh}
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    json.dump(db, open(OUT, 'w'))
    print(len(db), 'functions ->', OUT)

def load():
    return json.load(open(OUT))

def written(st, va): return st.get(va) in ('MATCH', 'DIFF')

def report(n=40, fam=False):
    db = load(); st = status_map()
    bysym, byshape = collections.defaultdict(list), collections.defaultdict(list)
    for k, v in db.items():
        bysym[v['sym']].append(k); byshape[v['shape']].append(k)
    rows = []; tot = collections.Counter()
    for k, v in db.items():
        va = int(k, 16)
        if written(st, va) or v['sz'] < 16: continue
        twins = [t for t in bysym[v['sym']] if t != k and written(st, int(t, 16))]
        kind = 'sym'
        if not twins:
            twins = [t for t in byshape[v['shape']] if t != k and written(st, int(t, 16))]; kind = 'shape'
        if twins:
            tw = sorted(twins, key=lambda t: st.get(int(t, 16)) != 'MATCH')[0]
            rows.append((v['sz'], kind, k, v['d'], tw, db[tw]['d'], st.get(int(tw, 16))))
            tot[kind] += v['sz']; tot[kind + 'n'] += 1
    rows.sort(reverse=True)
    if not fam:
        print(f"unwritten with a written twin: same code+callees {tot['symn']} fns {tot['sym']} B; same shape {tot['shapen']} fns {tot['shape']} B")
        for r in rows[:n]: print(f"{r[0]:6d} {r[1]:5s} {r[2]} {r[3][:70]}\n        <- {r[4]} [{r[6]}] {r[5][:70]}")
        return rows
    # families among unwritten
    groups = collections.defaultdict(list)
    for k, v in db.items():
        if not written(st, int(k, 16)) and v['sz'] >= 16: groups[v['shape']].append(k)
    fams = sorted((sum(db[k]['sz'] for k in g), g) for g in groups.values() if len(g) > 1)[::-1]
    print(f"{len(fams)} families of unwritten clones, {sum(len(g) for _, g in fams)} functions, {sum(b for b, _ in fams)} B (writing one per family covers {sum(b - db[g[0]]['sz'] for b, g in fams)} B extra)")
    for b, g in fams[:n]:
        print(f"{b:7d} B x{len(g)}: {db[g[0]]['d'][:60]} | {db[g[1]]['d'][:50]}")

def near(va, n=8):
    db = load(); st = status_map()
    me = db['%08x' % int(va, 16)]
    cands = []
    for k, v in db.items():
        if not written(st, int(k, 16)) or k == '%08x' % int(va, 16): continue
        if not (0.6 < v['sz'] / max(me['sz'], 1) < 1.6): continue
        sm = difflib.SequenceMatcher(None, me['tok'], v['tok'], autojunk=False)
        if sm.real_quick_ratio() < 0.5 or sm.quick_ratio() < 0.5: continue
        cands.append((sm.ratio(), k, v['d'], st.get(int(k, 16))))
    for r in sorted(cands, reverse=True)[:n]: print(f"{r[0]:.2f} {r[1]} [{r[3]}] {r[2][:90]}")

if __name__ == '__main__':
    a = sys.argv[1:]
    if not a: sys.exit(__doc__)
    if a[0] == 'scan': scan()
    elif a[0] == 'report': report(int(a[1]) if len(a) > 1 else 40)
    elif a[0] == 'families': report(int(a[1]) if len(a) > 1 else 40, fam=True)
    elif a[0] == 'near': near(a[1], int(a[2]) if len(a) > 2 else 8)
