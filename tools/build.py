#!/usr/bin/env python3
"""build.py check UNIT...   compile src/**/UNIT.cpp, compare, record results in status/status.csv
   build.py all              re-check every unit (regression), rewrite status
   build.py progress         totals for game + enginex (count and bytes), top objects
   build.py todo OBJ [N]     unmatched functions of an original object, smallest first
UNIT is the original object's base name (Actor, EXFile, ...); files live in src/game or src/enginex."""
import sys, os, csv, glob, json
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE); sys.path.insert(0, HERE)
import check
from xbe import XBE
STATUS = os.path.join(ROOT, 'status', 'status.csv')
FLAGS = check.DEFAULT_FLAGS

def funcs():
    return [r for r in csv.DictReader(open(os.path.join(ROOT, 'symbols', 'functions.csv'))) if r['origin'] in ('game', 'enginex')]
def load_status():
    st = {}
    if os.path.exists(STATUS):
        for r in csv.DictReader(open(STATUS)): st[r['va']] = r
    return st
CHANGED = {}
def save_status(st, replace=False):
    """Merge this run's rows into the status file under a lock (agents run concurrently)."""
    import fcntl
    os.makedirs(os.path.dirname(STATUS), exist_ok=True)
    with open(STATUS + '.lock', 'w') as lk:
        fcntl.flock(lk, fcntl.LOCK_EX)
        cur = {} if replace else load_status()
        cur.update(st if replace else CHANGED)
        tmp = STATUS + '.tmp'
        with open(tmp, 'w', newline='') as f:
            w = csv.DictWriter(f, ['va', 'status', 'unit', 'ndiff', 'name']); w.writeheader()
            for va in sorted(cur): w.writerow(cur[va])
        os.replace(tmp, STATUS)
def unit_path(u):
    if os.path.exists(u): return u
    c = glob.glob(os.path.join(ROOT, 'src', '*', u + '.cpp'))
    if not c: raise SystemExit(f'no source for unit {u}')
    return c[0]
def check_unit(u, st, x, verbose=False, quiet=False):
    path = unit_path(u); unit = os.path.splitext(os.path.basename(path))[0]
    try:
        o, msg = check.compile(path, 'vc71', FLAGS)
    except RuntimeError as e:
        print(f'{unit}: COMPILE ERROR\n{e}'); return 0, 0
    if 'warning' in msg.lower() and not quiet: print('\n'.join(l for l in msg.splitlines() if 'warning' in l.lower())[:2000])
    n = m = 0
    for name, sec, off, size in sorted(o.functions(), key=lambda f: f[0]):
        S = check.syms().get(name)
        if not S or S['origin'] not in ('game', 'enginex'): continue
        ok, nd, sz, full = check.compare(o, x, name, sec, off, size, False)
        n += 1; m += ok
        old = st.get(S['va'])
        # a function counts as matched if any unit reproduces it; keep a MATCH from elsewhere
        if ok or not old or old['status'] != 'MATCH' or old['unit'] == unit:
            st[S['va']] = CHANGED[S['va']] = dict(va=S['va'], status='MATCH' if ok else 'DIFF', unit=unit, ndiff=nd, name=S['demangled'][:120])
        if not quiet: print(f"{'MATCH' if ok else 'DIFF '} {S['va']} {sz:5d}/{full:<5d} {nd:4d}  {S['demangled'][:100]}")
        if verbose and not ok: check.compare(o, x, name, sec, off, size, True)  # diff block follows its DIFF line
    dn = dm = 0
    for name, sec, off, size in o.data_symbols():
        S = check.syms().get(name)
        if not S or S['origin'] not in ('game', 'enginex'): continue
        size = min(size, int(S['size']))
        ok, diff, bad = check.compare_data(o, x, name, sec, off, size)
        dn += 1; dm += ok
        if not ok or not quiet:
            print(f"{'DATA ' if ok else 'DDIFF'} {S['va']} {size:5d}  {S['demangled'][:80]}" + ('' if ok else f'  bytes {diff[:8]} ptrs {bad[:4]}'))
    print(f'{unit}: {m}/{n} match' + (f', data {dm}/{dn}' if dn else ''))
    return m, n
def progress():
    st = load_status(); F = funcs()
    tot = {}; byobj = {}
    for r in F:
        k = r['origin']; sz = int(r['size']); ok = st.get(r['va'], {}).get('status') == 'MATCH'
        t = tot.setdefault(k, [0, 0, 0, 0]); t[0] += 1; t[1] += sz; t[2] += ok; t[3] += sz * ok
        b = byobj.setdefault(r['obj'], [0, 0]); b[0] += 1; b[1] += ok
    all4 = [sum(t[i] for t in tot.values()) for i in range(4)]
    for k, t in list(tot.items()) + [('total', all4)]:
        print(f'{k:8s} {t[2]:6d}/{t[0]:<6d} functions ({100*t[2]/t[0]:5.2f}%)   {t[3]:8d}/{t[1]:<8d} bytes ({100*t[3]/t[1]:5.2f}%)')
    done = [(o, b) for o, b in byobj.items() if b[1]]
    for o, b in sorted(done, key=lambda z: -z[1][1])[:15]: print(f'   {o:32s} {b[1]}/{b[0]}')
def todo(obj, n=40):
    st = load_status()
    F = [r for r in funcs() if r['obj'] in (obj, obj + '.obj') and st.get(r['va'], {}).get('status') != 'MATCH']
    for r in sorted(F, key=lambda r: int(r['size']))[:n]:
        print(f"{r['va']} {int(r['size']):6d} [{r['flags']:3s}] {st.get(r['va'],{}).get('status','----'):5s} {r['demangled'][:110]}")
    print(f'{len(F)} unmatched in {obj}')
def main():
    a = sys.argv[1:]
    if not a: print(__doc__); return
    x = XBE(os.path.join(ROOT, 'orig', 'DEFAULT.XBE'))
    if a[0] == 'check':
        st = load_status(); v = '-v' in a
        for u in [u for u in a[1:] if u != '-v']: check_unit(u, st, x, v)
        save_status(st)
    elif a[0] == 'all':
        st = {}
        for p in sorted(glob.glob(os.path.join(ROOT, 'src', '*', '*.cpp'))): check_unit(p, st, x, quiet=True)
        save_status(st, replace=True); progress()
    elif a[0] == 'progress': progress()
    elif a[0] == 'todo': todo(a[1], int(a[2]) if len(a) > 2 else 40)
if __name__ == '__main__':
    try: main()
    except BrokenPipeError: pass
