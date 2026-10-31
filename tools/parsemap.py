"""Parse Predator.map into symbols/functions.csv (and all symbols)."""
import re, csv, subprocess, sys
sys.path.insert(0, 'tools'); from xbe import XBE
x = XBE('orig/DEFAULT.XBE')
SECMAP = {1: '.text', 2: 'D3D', 3: 'D3DX', 4: 'XGRPH', 5: 'DSOUND', 6: 'XMV', 7: 'XPP', 8: '.rdata', 9: '.data', 10: 'DOLBY'}
pat = re.compile(r'^\s*([0-9a-f]{4}):([0-9a-f]{8})\s+(\S+)\s+([0-9a-f]{8})\s+(f\s+)?(i\s+)?(\S+)\s*$')
syms = []; static = False
for line in open('orig/Predator.map', encoding='latin-1'):
    if line.startswith(' Static symbols'): static = True
    if line.startswith('Line numbers for'): break
    m = pat.match(line)
    if not m: continue
    sec, off, name, rva, f, inl, obj = m.groups()
    sec = int(sec, 16); off = int(off, 16)
    if sec not in SECMAP: continue
    lib, _, o = obj.rpartition(':')
    va = x.sec(SECMAP[sec])['va'] + off
    syms.append(dict(va=va, sec=SECMAP[sec], name=name, func=bool(f), inline=bool(inl), lib=lib, obj=o, static=static))
syms.sort(key=lambda s: (s['va'], s['static']))
# dedupe by (va,name)
seen = set(); u = []
for s in syms:
    k = (s['va'], s['name'])
    if k in seen: continue
    seen.add(k); u.append(s)
syms = u
# sizes: distance to next distinct address in same section (capped at section end)
addrs = {}
for s in syms: addrs.setdefault(s['sec'], set()).add(s['va'])
nxt = {}
for sec, a in addrs.items():
    a = sorted(a); end = x.sec(sec)['va'] + x.sec(sec)['vsize']
    for i, v in enumerate(a): nxt[v] = a[i + 1] if i + 1 < len(a) else end
names = [s['name'] for s in syms]
dem = subprocess.run(['llvm-undname'], input='\n'.join(names), capture_output=True, text=True).stdout
# llvm-undname echoes "input\noutput\n\n"
blocks = [b.split('\n') for b in dem.strip('\n').split('\n\n')]
dmap = {b[0]: (b[1] if len(b) > 1 else b[0]) for b in blocks}
def origin(s):
    if s['lib']: 
        l = s['lib'].lower()
        if 'enginex' in l: return 'enginex'
        return 'xdk:' + l.replace('.lib', '')
    return 'game'
with open('symbols/symbols.csv', 'w', newline='') as fa, open('symbols/functions.csv', 'w', newline='') as ff:
    cols = ['va', 'size', 'origin', 'obj', 'lib', 'flags', 'name', 'demangled']
    wa = csv.writer(fa); wf = csv.writer(ff); wa.writerow(cols); wf.writerow(cols)
    for s in syms:
        dm = dmap.get(s['name'], s['name'])
        if dm.startswith('invalid mangled name'): dm = s['name']
        fl = ('f' if s['func'] else '') + ('i' if s['inline'] else '') + ('s' if s['static'] else '')
        row = [f"{s['va']:08x}", nxt[s['va']] - s['va'], origin(s), s['obj'], s['lib'], fl, s['name'], dm]
        wa.writerow(row)
        if s['func']: wf.writerow(row)
print(len(syms), 'symbols')
