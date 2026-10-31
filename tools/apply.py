import os, sys
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
# apply.py UNIT VARFILE N
import sys, os, glob
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from fnrep import replace_fn
unit,vf,n=sys.argv[1],sys.argv[2],int(sys.argv[3])
path=glob.glob(f'{ROOT}/src/*/{unit}.cpp')[0]
s=open(path).read()
for part in open(vf).read().split('\n=====\n')[n].split('\n+++++\n'): s=replace_fn(s,part)
open(path,'w').write(s); print('applied')
