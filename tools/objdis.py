import os, sys
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
import sys, csv, subprocess

obj=sys.argv[1]+'.obj'
rows=[r for r in csv.DictReader(open(ROOT+'/symbols/symbols.csv')) if r['obj']==obj]
fn=set(r['va'] for r in csv.DictReader(open(ROOT+'/symbols/functions.csv')) if r['obj']==obj)
for r in sorted(rows,key=lambda r:int(r['va'],16)):
    if r['va'] not in fn: print('DATA',r['va'],r['size'],r['flags'],r['demangled'][:150])
vas=sorted(set(fn),key=lambda v:int(v,16))
print(subprocess.run(['python3','-I',ROOT+'/tools/disasm.py']+vas,capture_output=True,text=True,cwd=ROOT).stdout)
