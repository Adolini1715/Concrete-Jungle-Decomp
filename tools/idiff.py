# idiff.py CHECKFILE_OR_UNIT FUNCSUB : unified diff of instruction text (addresses/targets stripped)
import sys, subprocess, re, difflib
R='/home/claude/pcj-decomp'
a, f = sys.argv[1:3]
cmd = ['python3','-I',f'{R}/tools/check.py',a,'-v'] if a.endswith('.cpp') else ['python3','-I',f'{R}/tools/build.py','check',a,'-v']
out = subprocess.run(cmd,capture_output=True,text=True,cwd=R).stdout.splitlines()
blk=[]; p=False
for l in out:
    if l.startswith(('DIFF','MATCH')): p = f in l; continue
    if p and '|' in l: blk.append(l)
def norm(s):
    s = re.sub(r'^\s*[0-9a-f]+\s+','',s.strip()); s = re.sub(r'0x[0-9a-f]{5,}','X',s); return s
L=[norm(l.split('|')[0]) for l in blk]; Rr=[norm(l.split('|')[1].replace('<<','')) for l in blk]
L=[x for x in L if x]; Rr=[x for x in Rr if x]
for d in difflib.unified_diff(L,Rr,'mine','orig',n=2,lineterm=''): print(d)
