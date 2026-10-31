# ccobj.py SRC OUTOBJ : compile and keep the object (scratch helper)
import sys, os, shutil
sys.path.insert(0, '/home/claude/pcj-decomp/tools')
import check, tempfile
src, outp = sys.argv[1:3]
orig_rm = shutil.rmtree
def keep(p, *a, **k):
    o = os.path.join(p, 'o.obj')
    if os.path.exists(o): shutil.copy(o, outp)
    orig_rm(p, *a, **k)
shutil.rmtree = keep
cc = 'vc71'
print(check.compile(src, cc, check.DEFAULT_FLAGS)[1][:300])
