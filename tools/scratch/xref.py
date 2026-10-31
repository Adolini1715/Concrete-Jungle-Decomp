import sys,os,struct
sys.path.insert(0,os.path.abspath('tools')); __file__=os.path.abspath('tools/disasm.py'); exec(open('tools/disasm.py').read().split('for q in sys.argv')[0])
targets=[int(t,16) for t in sys.argv[1:]]
for r in rows:
    if 'f' not in r['flags']: continue
    va=int(r['va'],16); n=int(r['size'])
    try: b=x.read(va,n)
    except Exception: continue
    for i in md.disasm(b,va):
        for t in targets:
            if ('%x'%t) in i.op_str:
                print('%06x %s: %s %s'%(i.address, r['demangled'][:70], i.mnemonic, i.op_str))
