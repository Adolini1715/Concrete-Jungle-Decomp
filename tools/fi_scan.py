import sys, struct, csv, collections
sys.path.insert(0,'/home/claude/pcj-decomp/tools')
from xbe import XBE
x=XBE('/home/claude/pcj-decomp/orig/DEFAULT.XBE')
lo,hi=0x11000,0x2e7000
data=x.read(lo,hi-lo)
callers=collections.Counter()
for i in range(len(data)-5):
    if data[i]==0xe8 or data[i]==0xe9:
        t=lo+i+5+struct.unpack_from('<i',data,i+1)[0]
        callers[t]+=1
# also absolute refs (push imm / mov imm / dd) - count dword occurrences
absref=collections.Counter()
for i in range(0,len(data)-4):
    v=struct.unpack_from('<I',data,i)[0]
    if 0x11000<=v<0x2e7000: absref[v]+=1
st={r['va']:r for r in csv.DictReader(open('/home/claude/pcj-decomp/status/status.csv'))}
rows=[r for r in csv.DictReader(open('/home/claude/pcj-decomp/symbols/functions.csv')) if r['origin'] in ('game','enginex')]
out=[]
for r in rows:
    if 'i' not in r['flags']: continue
    if st.get(r['va'],{}).get('status')=='MATCH': continue
    va=int(r['va'],16)
    out.append((r['obj'], r['va'], int(r['size']), callers[va], absref[va], st.get(r['va'],{}).get('status','----'), r['demangled'][:90]))
zero=[o for o in out if o[3]==0 and o[4]==0]
print('unmatched fi:',len(out),' zero-caller:',len(zero), 'bytes', sum(o[2] for o in zero))
byobj=collections.Counter(o[0] for o in zero)
for k,v in byobj.most_common(40): print(v,k)
import json; json.dump(zero, open('/home/claude/pcj-decomp/build/fi_zero.json','w'))
