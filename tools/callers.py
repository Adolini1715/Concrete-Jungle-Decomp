import sys, struct
sys.path.insert(0,'/home/claude/pcj-decomp/tools')
from xbe import XBE
x=XBE('/home/claude/pcj-decomp/orig/DEFAULT.XBE')
# text range
lo,hi=0x11000,0x2e7000
data=x.read(lo,hi-lo)
targets=set(int(a,16) for a in sys.argv[1:])
import collections
hits=collections.defaultdict(list)
for i in range(len(data)-5):
    if data[i]==0xe8:
        t=lo+i+5+struct.unpack_from('<i',data,i+1)[0]
        if t in targets: hits[t].append(hex(lo+i))
for t in sorted(targets): print(hex(t), len(hits[t]), hits[t][:4])
