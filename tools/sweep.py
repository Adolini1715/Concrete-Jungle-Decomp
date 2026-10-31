import os, sys
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
# sweep.py UNIT FUNCNAME_SUBSTR VARFILE [-v]
import sys, subprocess, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from fnrep import replace_fn

unit, fsub, vf = sys.argv[1:4]
import glob
path=glob.glob(f'{ROOT}/src/*/{unit}.cpp')[0]
src=open(path).read()
vars_=open(vf).read().split('\n=====\n')
os.makedirs(f'{ROOT}/build/var',exist_ok=True)
for n,v in enumerate(vars_):
    if not v.strip(): continue
    s=src
    for part in v.split('\n+++++\n'): s=replace_fn(s,part)
    p=f'{ROOT}/build/var/{unit}_v{n}.cpp'
    open(p,'w').write(s)
    r=subprocess.run(['python3','-I',f'{ROOT}/tools/check.py',p]+(['-v'] if len(sys.argv)>4 else []),capture_output=True,text=True,cwd=ROOT)
    out=r.stdout+r.stderr
    lines=[l for l in out.splitlines() if fsub in l and (l.startswith('MATCH') or l.startswith('DIFF'))]
    print(n, lines[:3] if lines else out[-1500:])
    if len(sys.argv)>4:
        # print only the diff block that follows the DIFF line for fsub
        show = False
        for l in out.splitlines():
            if l.startswith('MATCH') or l.startswith('DIFF'): show = fsub in l and l.startswith('DIFF')
            elif show: print(l)
