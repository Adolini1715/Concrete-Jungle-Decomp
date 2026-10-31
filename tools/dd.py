import os, sys
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
import sys, struct, csv, bisect
sys.path.insert(0,ROOT+'/tools')
from xbe import XBE
x=XBE(ROOT+'/orig/DEFAULT.XBE')
rows=list(csv.DictReader(open(ROOT+'/symbols/symbols.csv')))
by={}
for r in rows: by.setdefault(int(r['va'],16),r)
vs=sorted(by)
def sym(a):
    i=bisect.bisect_right(vs,a)-1
    if i<0: return ''
    r=by[vs[i]]; o=a-vs[i]
    if o>int(r['size'])+0x100: return ''
    return r['demangled'][:90]+(f'+{o:#x}' if o else '')
va=int(sys.argv[1],16); n=int(sys.argv[2])
b=x.read(va,n*4)
for i in range(n):
    v=struct.unpack_from('<I',b,i*4)[0]
    print(f'{va+i*4:08x} {v:08x} {sym(v) if v>0x10000 else ""}')
