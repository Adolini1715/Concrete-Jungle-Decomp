# fsweep.py UNIT FUNC_SUBSTR VARFILE  -- like tools/sweep.py but stops at the first MATCH and prints best
import os, sys, subprocess, glob, re
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, ROOT + '/tools')
from fnrep import replace_fn
unit, fsub, vf = sys.argv[1:4]
path = glob.glob(f'{ROOT}/src/*/{unit}.cpp')[0]
src = open(path).read()
vs = open(vf).read().split('\n=====\n')
best = None
for n, v in enumerate(vs):
    if not v.strip(): continue
    s = src
    for part in v.split('\n+++++\n'): s = replace_fn(s, part)
    p = f'{ROOT}/build/var/{unit}_f{n}.cpp'
    open(p, 'w').write(s)
    r = subprocess.run(['python3', '-I', f'{ROOT}/tools/check.py', p], capture_output=True, text=True, cwd=ROOT)
    out = r.stdout + r.stderr
    lines = [l for l in out.splitlines() if fsub in l and (l.startswith('MATCH') or l.startswith('DIFF'))]
    if not lines:
        print(n, 'ERR', out[-600:]); continue
    l = lines[0]
    if l.startswith('MATCH'):
        print('MATCH variant', n); sys.exit(0)
    d = int(l.split()[3])
    if best is None or d < best[1]: best = (n, d)
    print(n, d, flush=True)
print('best', best)
