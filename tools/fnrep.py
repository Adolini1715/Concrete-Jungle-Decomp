def replace_fn(src, newdef):
    head=newdef.strip().split('\n')[0].strip()
    i=src.find(head)
    if i<0: raise SystemExit('head not found: '+head)
    j=src.find('{',i); d=0; k=j
    while True:
        if src[k]=='{': d+=1
        elif src[k]=='}':
            d-=1
            if d==0: break
        k+=1
    return src[:i]+newdef.strip()+src[k+1:]
