# usage: perm.py OUT HEADFILE  -- HEADFILE: head lines, then '@@' lines separating movable statements, then '##' then tail
import sys, itertools
out, spec = sys.argv[1], sys.argv[2]
limit = int(sys.argv[3]) if len(sys.argv) > 3 else 200
txt = open(spec).read()
head, rest = txt.split('@@\n', 1)
body, tail = rest.split('##\n', 1)
stmts = [s for s in body.split('@@\n') if s.strip()]
vs = []
for perm in itertools.permutations(range(len(stmts))):
    vs.append(head + ''.join(stmts[i] for i in perm) + tail)
    if len(vs) >= limit: break
open(out, 'w').write('=====\n'.join(vs))
print(len(vs), 'variants')
