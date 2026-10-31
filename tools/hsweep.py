#!/usr/bin/env python3
"""hsweep.py HEADER UNIT FUNC_SUBSTR VARFILE: try variants of a header inline.
VARFILE: blocks separated by a line `=====`; each block is `OLD`, a line `-----`, then `NEW`.
Each variant replaces OLD with NEW in include/HEADER, re-checks UNIT, and the header is restored at the end."""
import os, sys, subprocess
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
hdr, unit, sub, varf = sys.argv[1:5]
if not os.path.exists(hdr): hdr = os.path.join(ROOT, 'include', hdr)
orig = open(hdr).read()
try:
    for i, b in enumerate(open(varf).read().split('=====\n')):
        if not b.strip(): continue
        old, new = b.split('-----\n')
        if old not in orig: print(i, 'OLD NOT FOUND'); continue
        open(hdr, 'w').write(orig.replace(old, new, 1))
        out = subprocess.run(['python3', '-I', os.path.join(ROOT, 'tools', 'build.py'), 'check', unit], capture_output=True, text=True, cwd=ROOT).stdout
        lines = out.strip().splitlines()
        print(i, [l[:60] for l in lines if sub in l][:3], lines[-1] if lines else '')
finally:
    open(hdr, 'w').write(orig)
