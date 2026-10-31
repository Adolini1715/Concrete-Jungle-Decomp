#!/usr/bin/env python3
"""maskdiff.py UNIT VA: structural diff of one function with registers and addresses masked.
Runs `build.py check UNIT -v`, takes the side-by-side block of the function at VA and diffs the
instruction streams (mine vs orig) with difflib after replacing register names, immediates that look
like addresses and call targets, so only real structure differences (extra loads, different order,
different calls) remain."""
import re, subprocess, sys, difflib, os
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
unit, va = sys.argv[1], sys.argv[2].lower().replace('0x', '').zfill(8)
out = subprocess.run([sys.executable, '-I', os.path.join(ROOT, 'tools', 'build.py'), 'check', unit, '-v'],
                     capture_output=True, text=True, cwd=ROOT).stdout.splitlines()
start = None
for i, l in enumerate(out):
    if re.match(r'^(DIFF|MATCH)\s+' + va, l): start = i; break
if start is None: sys.exit('function not found / not DIFF')
mine, orig = [], []
for l in out[start + 2:]:
    if re.match(r'^(DIFF|MATCH|DATA)\s', l) or l.startswith('   bad reloc') or l.startswith('   size:'): break
    if '|' not in l: continue
    a, b = l.split('|', 1)
    b = b.replace('<<', '')
    mine.append(a.strip()); orig.append(b.strip())
REG = re.compile(r'\b(e?[abcd]x|e?[sd]i|e?bp|[abcd][lh]|xmm\d|st\(\d\))\b')
def norm(s):
    s = re.sub(r'^[0-9a-f]+\s+', '', s)
    s = s.split(';')[0]
    s = re.sub(r'0x[0-9a-f]{4,}', 'ADDR', s)
    s = re.sub(r'\[(0|2|4|6)\]', '[ADDR]', s)
    s = re.sub(r'(push|mov \w+,|mov byte ptr \[[^\]]*\],|mov dword ptr \[[^\]]*\],) 0$', lambda m: m.group(1) + ' ADDR', s)
    s = re.sub(r'\[(esp|R)( [+-] 0x[0-9a-f]+)?\]', '[S]', s)
    s = re.sub(r'0x[0-9a-f]+(?=\]| *$)', 'N', s) if False else s
    s = REG.sub('R', s)
    s = re.sub(r'\bcall\s+.*', 'call X', s)
    s = re.sub(r'\bj\w+\s+.*', lambda m: m.group(0).split()[0] + ' L', s)
    return s.strip()
m = [norm(x) for x in mine if x]; o = [norm(x) for x in orig if x]
mi = [x for x in mine if x]; oi = [x for x in orig if x]
sm = difflib.SequenceMatcher(None, m, o, autojunk=False)
for tag, i1, i2, j1, j2 in sm.get_opcodes():
    if tag == 'equal': continue
    print(f'--- {tag}: mine {i1}..{i2} / orig {j1}..{j2}')
    for x in mi[i1:i2]: print('  - ' + x)
    for x in oi[j1:j2]: print('  + ' + x)
