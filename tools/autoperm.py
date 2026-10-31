# autoperm.py UNIT "FUNC_HEAD" "CHECK_SUBSTR" [SECONDS] [SEED]
# Random-walk permuter for one function: swaps adjacent independent statements and declarations,
# recompiles, keeps variants that score no worse. Never edits src/; the best variant is written to
# build/var/auto_<UNIT>_<tag>.best.cpp (whole unit).
# Swaps are only made between statements that share no identifiers, where at most one has a call
# and a call is never moved past a line touching memory (->, *, m_), so behaviour is preserved.
import sys, os, re, glob, random, subprocess, time, hashlib
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
unit, head, csub = sys.argv[1:4]
secs = int(sys.argv[4]) if len(sys.argv) > 4 else 1800
seed = int(sys.argv[5]) if len(sys.argv) > 5 else int(time.time())
random.seed(seed)
path = glob.glob(f'{ROOT}/src/*/{unit}.cpp')[0]
src = open(path).read()
i = src.index(head)
j = src.index('{', i); d = 0; k = j
while True:
    if src[k] == '{': d += 1
    elif src[k] == '}':
        d -= 1
        if d == 0: break
    k += 1
pre, fn, post = src[:j + 1], src[j + 1:k], src[k:]
tag = hashlib.md5(head.encode()).hexdigest()[:6]
KW = set('if else for while do switch case return break continue default const unsigned signed int short char float double long void struct class sizeof true false'.split())
def ids(s): return set(re.findall(r'[A-Za-z_]\w*', s)) - KW
def lhs(s):
    m = re.match(r'\s*([\w\.\->\[\]\s\*]+?)\s*([+\-*/|&^]?=)(?!=)', s)
    return ids(m.group(1)) if m else set()
def simple(s):
    t = s.strip()
    return t.endswith(';') and not re.match(r'(if|for|while|do|return|break|continue|case|default|else|switch)\b', t) and '{' not in t and '}' not in t
def call(s): return re.search(r'\w\s*\(', re.sub(r'\((float|int|u16|s16|u8|s8|unsigned int|unsigned short|short|char|double)\)', '', s)) is not None
def mem(s): return '->' in s or 'm_' in s or '*' in s or '[' in s
def swappable(a, b):
    if not (simple(a) and simple(b)): return False
    if len(a) - len(a.lstrip()) != len(b) - len(b.lstrip()): return False
    ia, ib = ids(a), ids(b)
    if lhs(a) & ib or lhs(b) & ia: return False
    if ('++' in a or '--' in a) and ia & ib: return False
    if ('++' in b or '--' in b) and ia & ib: return False
    ca, cb = call(a), call(b)
    if ca and cb: return False
    if (ca and mem(b)) or (cb and mem(a)): return False
    if lhs(a) & lhs(b): return False
    return True
def score(body):
    p = f'{ROOT}/build/var/auto_{unit}_{tag}.cpp'
    open(p, 'w').write(pre + body + post)
    r = subprocess.run(['python3', '-I', f'{ROOT}/tools/check.py', p], capture_output=True, text=True, cwd=ROOT)
    for l in r.stdout.splitlines():
        if csub in l and l.startswith('MATCH'): return 0
        if csub in l and l.startswith('DIFF'): return int(l.split()[3])
    return None
cur = fn; cs = score(cur); best, bs = cur, cs
print('start', cs, 'seed', seed, flush=True)
t0 = time.time(); n = 0
while time.time() - t0 < secs and bs:
    lines = cur.split('\n')
    cand = [x for x in range(len(lines) - 1) if swappable(lines[x], lines[x + 1])]
    if not cand: break
    x = random.choice(cand)
    nl = lines[:]; nl[x], nl[x + 1] = nl[x + 1], nl[x]
    if random.random() < 0.3:
        c2 = [y for y in range(len(nl) - 1) if swappable(nl[y], nl[y + 1])]
        if c2:
            y = random.choice(c2); nl[y], nl[y + 1] = nl[y + 1], nl[y]
    v = '\n'.join(nl); s = score(v); n += 1
    if s is None: continue
    if s <= cs or random.random() < 0.05:
        cur, cs = v, s
        if s < bs:
            best, bs = v, s
            open(f'{ROOT}/build/var/auto_{unit}_{tag}.best.cpp', 'w').write(pre + best + post)
            print(f'{n} best {bs}', flush=True)
print('done', n, 'tries, best', bs, flush=True)
open(f'{ROOT}/build/var/auto_{unit}_{tag}.best.cpp', 'w').write(pre + best + post)
