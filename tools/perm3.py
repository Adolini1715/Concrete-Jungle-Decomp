#!/usr/bin/env python3
"""perm3.py: fast source permuter for near-miss functions.

    perm3.py list  [--max-score N] [--min-size N]        write build/perm/targets.txt (DIFF functions, best first)
    perm3.py one   UNIT VA [--secs N] [--jobs N]
    perm3.py batch LISTFILE [--secs-per N] [--jobs N] [--inflight N] [--out DIR]   (resumes)

Differences from perm2:
  * Each candidate compiles a reduced translation unit holding only the target function (other
    non-static, non-inline definitions are dropped) against a precompiled header of the unit's
    includes.  The reduction is validated: the target must compile to the same bytes as in the full
    unit, otherwise the full unit is used.  Roughly 5-10x more compiles per second.
  * Score is an instruction-level alignment against the original (mnemonic sequence first, then
    operands, then bytes), so a register swap early in the function still reads as "close".
  * Search enumerates the whole single-edit neighbourhood of the current best candidates (beam),
    then random multi-edit combinations.  Every compile job goes through one process pool across
    all cores, with several functions in flight, so nothing is duplicated and small functions
    finish in seconds.
  * Edits need not be meaning-preserving: a byte match certifies the result.  perm2's edit set is
    reused and extended (temp sink/hoist, ternary <-> if, comparison rewrites, casts, any adjacent
    statement swap, block early-return at any level, ...).

Results: <out>/<unit>_<va>.txt (score line + demangled name) and <unit>_<va>.best.cpp (the
function text).  tools/apply_perm.py applies the matches back to the source.
"""
import os, re, sys, random, time, glob, csv, tempfile, shutil, argparse, struct, subprocess, difflib
import multiprocessing as mp, zlib
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE); sys.path.insert(0, HERE)
import perm2
from perm2 import tokenize, scan, text, match_close, depth, P, N, BIN, ASSIGN, COMM, FLIP, IDENT, KEYW, TYPEWORDS

NATIVE = os.name == 'nt'
_tcroot = os.environ.get('PCJ_TOOLCHAINS') or (os.path.join(os.path.dirname(ROOT), 'toolchains') if NATIVE else '/home/claude/toolchains')
TC = os.path.join(_tcroot, 'vc71')
FLAGS = ['/O2', '/G6', '/arch:SSE']


# ---------------------------------------------------------------- compiler
def wpath(p):
    return p if NATIVE else 'Z:' + os.path.abspath(p).replace('/', '\\')


def cl(args, cwd, inc, timeout=20):
    incs = list(inc) + [os.path.join(ROOT, 'include'), os.path.join(TC, 'Include')]
    if NATIVE:
        env = dict(os.environ, INCLUDE=';'.join(incs), PATH=os.path.join(TC, 'Bin') + ';' + os.environ.get('PATH', ''))
        cmd = [os.path.join(TC, 'Bin', 'cl.exe'), '/nologo', '/c', *FLAGS, *args]
    else:
        env = dict(os.environ, WINEPREFIX='/root/wine/pcj', WINEARCH='win32', WINEDEBUG='-all', INCLUDE=';'.join(wpath(p) for p in incs))
        cmd = ['wine', os.path.join(TC, 'Bin', 'cl.exe'), '/nologo', '/c', *FLAGS, *args]
    # output goes to a file and only its tail is read: some variants make cl print errors without end
    # (gigabytes in a minute), and some never finish, so a variant gets 20 s (normal: under 2 s)
    with tempfile.TemporaryFile() as f:
        try:
            r = subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT, env=env, cwd=cwd, timeout=timeout)
            rc = r.returncode
        except subprocess.TimeoutExpired:
            rc = -1
        size = f.seek(0, 2); f.seek(max(0, size - 8192))
        out = f.read().decode('latin-1', 'replace')
        if size > 8192: out = '...' + out
        return rc, out if rc != -1 else out + '\ntimeout'


def compile_variant(cpp_text, workdir, name, pch=None, inc=(), timeout=20):
    """Write cpp_text as <name>.cpp in workdir, compile it, return the Obj or None."""
    from coff import Obj
    src = os.path.join(workdir, name + '.cpp'); obj = os.path.join(workdir, name + '.obj')
    if os.path.exists(obj): os.remove(obj)
    open(src, 'w', encoding='latin-1').write(cpp_text)
    args = ['/Fo' + wpath(obj)]
    if pch: args += ['/Yu' + pch[0], '/Fp' + wpath(pch[1])]
    args.append(wpath(src))
    rc, log = cl(args, workdir, inc, timeout)
    if rc or not os.path.exists(obj): return None, log
    return Obj(obj), log


# ---------------------------------------------------------------- unit reduction
FN_HEAD_END = re.compile(r'(\)\s*(const)?\s*|\)\s*:\s*[^{;]*)$', re.S)


def top_level_defs(src):
    """[(head_start, body_open, body_close_inclusive, head_text)] for every top-level brace block."""
    out = []; d = 0; i = 0; n = len(src); last = 0; in_str = None
    k = 0
    while k < n:
        c = src[k]
        if in_str:
            if c == '\\': k += 2; continue
            if c == in_str: in_str = None
            k += 1; continue
        if c in '"\'': in_str = c; k += 1; continue
        if src.startswith('//', k): k = src.find('\n', k); k = n if k < 0 else k; continue
        if src.startswith('/*', k): k = src.find('*/', k + 2); k = n if k < 0 else k + 2; continue
        if c == '#' and (k == 0 or src[k - 1] == '\n'):
            e = k
            while True:
                e = src.find('\n', e)
                if e < 0: e = n; break
                if src[e - 1] == '\\': e += 1; continue
                break
            if d == 0: last = e + 1
            k = e + 1; continue
        if c == '{':
            if d == 0: head_start = last; body_open = k
            d += 1
        elif c == '}':
            d -= 1
            if d == 0:
                out.append((head_start, body_open, k, src[head_start:body_open]))
                e = k + 1
                # swallow a trailing ';' (struct/initializer) so the next head starts clean
                m = re.match(r'\s*;', src[e:])
                if m: e += m.end()
                last = e
        elif c == ';' and d == 0:
            last = k + 1
        k += 1
    return out


def is_function_head(head):
    h = head.strip()
    if not h or h.endswith('=') or h.endswith(',') : return False
    if re.match(r'(class|struct|union|enum|namespace|extern\s+"C")\b', h): return False
    if '(' not in h: return False
    return FN_HEAD_END.search(h) is not None


def head_name(head):
    """Qualified name of the function a head defines, or None."""
    h = head.strip()
    # strip ctor init list
    m = re.search(r'\)\s*:\s*[^{;]*$', h)
    if m and not re.search(r'\boperator\b', h): h = h[:m.start() + 1]
    # find the '(' that opens the parameter list: last '(' at depth 0 scanning from the end
    d = 0; k = len(h) - 1; op = -1
    while k >= 0:
        if h[k] == ')': d += 1
        elif h[k] == '(':
            d -= 1
            if d == 0: op = k; break
        k -= 1
    if op < 0: return None
    pre = h[:op].rstrip()
    m = re.search(r'((?:[A-Za-z_]\w*\s*::\s*)*(?:~?[A-Za-z_]\w*|operator\s*[^\s\w]+|operator\s*\w+))$', pre)
    return m.group(1).replace(' ', '') if m else None


def reduce_unit(src, keep_pos):
    """Drop the bodies of top-level function definitions the target cannot depend on.  Kept: the
    target, static/inline/template functions, and any function whose name appears in a kept body
    (iterated to a fixpoint, since VC may inline same-unit functions at /O2).  Dropped free functions
    become prototypes; dropped member definitions disappear."""
    defs = [(hs, bo, bc, head) for hs, bo, bc, head in top_level_defs(src) if is_function_head(head)]
    keep = set(); names = {}
    for idx, (hs, bo, bc, head) in enumerate(defs):
        if hs <= keep_pos <= bo: keep.add(idx)
        if re.search(r'\b(static|inline|__forceinline|__inline|template)\b', head.strip().split('(')[0]): keep.add(idx)
        n = head_name(head)
        if n: names.setdefault(n.split('::')[-1], []).append(idx)
    changed = True
    while changed:
        changed = False
        used = set()
        for idx in keep:
            hs, bo, bc, head = defs[idx]
            used |= set(IDENT.findall(src[bo:bc + 1]))
        for n, idxs in names.items():
            if n in used:
                for idx in idxs:
                    if idx not in keep: keep.add(idx); changed = True
    pieces = []; cur = 0
    for idx, (hs, bo, bc, head) in enumerate(defs):
        if idx in keep: continue
        hstrip = head.strip(); name = head_name(head)
        pieces.append(src[cur:hs])
        if name and '::' not in name and not re.search(r'\)\s*:\s*', hstrip):
            pieces.append(hstrip.rstrip() + ';\n')
        else:
            pieces.append('\n' * src[hs:bc + 1].count('\n'))
        cur = bc + 1
    pieces.append(src[cur:])
    return ''.join(pieces)


def split_pch(src):
    """(header_prefix, rest): the unit's leading #include/#define/comment lines go into the PCH."""
    lines = src.split('\n'); k = 0; last_inc = 0
    while k < len(lines):
        l = lines[k].strip()
        if l.startswith('#include') or l.startswith('#define') or l.startswith('#pragma') or l.startswith('//') or not l:
            if l.startswith('#include'): last_inc = k + 1
            k += 1; continue
        break
    return '\n'.join(lines[:last_inc]) + '\n', '\n'.join(lines[last_inc:])


# ---------------------------------------------------------------- scoring
_md = None
def disasm(b, va):
    global _md
    if _md is None:
        from capstone import Cs, CS_ARCH_X86, CS_MODE_32
        _md = Cs(CS_ARCH_X86, CS_MODE_32)
    return list(_md.disasm(bytes(b), va))


IMMRE = re.compile(r'0x[0-9a-f]{5,}')
def norm_ops(mn, op):
    if mn.startswith('j') or mn == 'call': return 'T'
    return IMMRE.sub('IMM', op)


def fn_bytes(o, sym):
    for name, sec, off, size in o.functions():
        if name == sym:
            secd = o.sections[sec - 1]
            relocs = sorted((r[0] - off, o.symbols[r[1]]['name'], r[2]) for r in secd['relocs'] if off <= r[0] < off + size)
            return bytearray(secd['data'][off:off + size]), relocs
    return None, None


def score_obj(o, sym, X, va, full, check):
    """(score, ok, ndiff). score 0 iff the function matches."""
    for name, sec, off, size in o.functions():
        if name != sym: continue
        ok, nd, sz, _ = check.compare(o, X, name, sec, off, size, False)
        if ok: return 0, True, 0
        mine = bytes(o.sections[sec - 1]['data'][off:off + sz])
        orig = X.read(va, full)
        # trim original padding
        e = full
        while e > 0 and orig[e - 1] == 0xCC: e -= 1
        orig = orig[:e]
        a = disasm(mine, va); b = disasm(orig, va)
        am = [i.mnemonic for i in a]; bm = [i.mnemonic for i in b]
        sm = difflib.SequenceMatcher(None, am, bm, autojunk=False)
        matched = 0; opmis = 0
        for i, j, n in sm.get_matching_blocks():
            for k in range(n):
                matched += 1
                if norm_ops(am[i + k], a[i + k].op_str) != norm_ops(bm[j + k], b[j + k].op_str): opmis += 1
        insdel = (len(a) - matched) + (len(b) - matched)
        return insdel * 10 + opmis * 3 + min(nd, 50) // 10 + 1, False, nd
    return None, False, None


# ---------------------------------------------------------------- worker side
def shadow_header(state, hdr, text):
    """Keep a private copy of include/ in state['dir']/inc (made once), restore the header edited last
    time, write `text` as `hdr`, and return the tree's path.  A copy of the whole tree is needed because
    headers find their siblings in their own directory before the /I path."""
    root = os.path.join(state['dir'], 'inc')
    if not os.path.isdir(root):
        shutil.copytree(os.path.join(ROOT, 'include'), root)
    last = state.get('shadow_last')
    if last and last != hdr:
        shutil.copyfile(os.path.join(ROOT, 'include', last), os.path.join(root, last))
    open(os.path.join(root, hdr), 'w', encoding='latin-1').write(text)
    state['shadow_last'] = hdr
    return root


_W = {}
def w_init(tcflags):
    import check
    _W['check'] = check
    _W['X'] = check.XBE(os.path.join(ROOT, 'orig', 'DEFAULT.XBE'))
    _W['dir'] = tempfile.mkdtemp(prefix='p3w')
    _W['n'] = 0


def w_job(job):
    """job: (jid, ctx, body) -> (jid, score, ok, nd, log_tail)."""
    jid, ctx, body = job
    check = _W['check']; X = _W['X']
    _W['n'] += 1
    name = 'v%d' % (_W['n'] % 4)
    if ctx.get('hdr'):
        # header inline: body is the edited header; it goes into this worker's shadow include tree
        inc = [shadow_header(_W, ctx['hdr'], body)] + list(ctx['inc'])
        o, log = compile_variant(ctx['unit_text'], _W['dir'], name, None, inc)
    else:
        src = ctx['pre'] + body + ctx['post']
        o, log = compile_variant(src, _W['dir'], name, ctx['pch'], ctx['inc'])
    if o is None: return jid, None, False, None, log[-300:]
    check.CUR_UNIT = ctx['unit']
    s, ok, nd = score_obj(o, ctx['sym'], X, ctx['va'], ctx['full'], check)
    return jid, s, ok, nd, ''


# ---------------------------------------------------------------- extra edits
def sink_hoist_edits(src, toks, stmt_lists):
    """'T x = E; S(x)' -> 'S(E)' when x is used once in the next statement; and the reverse for a
    few recognisable expressions (calls / member chains) inside a statement."""
    eds = []
    for sts in stmt_lists:
        for k in range(len(sts) - 1):
            (a0, a1, ka), (b0, b1, kb) = sts[k], sts[k + 1]
            if ka != 'decl': continue
            ta = text(src, toks, a0, a1); tb = text(src, toks, b0, b1)
            m = re.match(r'^(.*?\b)([A-Za-z_]\w*)\s*=\s*(.*);$', ta, re.S)
            if not m or ',' in m.group(1): continue
            var, init = m.group(2), m.group(3).strip()
            uses = re.findall(r'\b' + re.escape(var) + r'\b', tb)
            later = any(re.search(r'\b' + re.escape(var) + r'\b', text(src, toks, s0, s1)) for s0, s1, _ in sts[k + 2:])
            if len(uses) == 1 and not later:
                nb = re.sub(r'\b' + re.escape(var) + r'\b', '(' + init + ')' if re.search(r'[-+*/%<>=&|^?]', init) else init, tb)
                eds.append((toks[a0].a, toks[b1 - 1].b, nb))
            # const-ref alias form: T x = E; -> const T &x = E;
            if not m.group(1).strip().startswith('const') and '*' not in m.group(1) and '&' not in m.group(1):
                eds.append((toks[a0].a, toks[a1 - 1].b, 'const ' + m.group(1).strip() + ' &' + var + ' = ' + init + ';'))
    # hoist: a call or member chain used inside a simple statement becomes a local
    GUESS = [(r'Pos\(\)|Position\(\)|GetFeetPos\(\)|GetHeadPos\(\)|Vector', 'EXVector'),
             (r'GetProfile\(\)', 'ActorProfile *'), (r'GetSkin\(\)', 'ActorSkin *'), (r'GetItem\(\)|ItemHandler\(\)', 'EXItem *'),
             (r'Length|Dist|Angle|Rand|Time|Frame', 'float'), (r'Hash|Count|Index|Id\b|Flags', 'int')]
    for sts in stmt_lists:
        for (s0, s1, kind) in sts:
            if kind not in ('simple', 'return', 'if'): continue
            seg = text(src, toks, s0, s1)
            if kind == 'if':
                pc = match_close(toks, s0 + 1, '(', ')'); seg = text(src, toks, s0, pc + 1)
            for m in re.finditer(r'(?:[A-Za-z_]\w*(?:->|\.|::))*[A-Za-z_]\w*\([^()]*\)', seg):
                e = m.group(0)
                if seg.count(e) < 1 or e.endswith('()') and len(e) < 6: continue
                if re.match(r'^(if|while|for|switch|return|sizeof)\b', e): continue
                ty = next((t for rx, t in GUESS if re.search(rx, e)), 'int')
                nm = 'tmp' + str(zlib.crc32(e.encode()) % 97)
                rep = seg.replace(e, nm, 1)
                eds.append((toks[s0].a, toks[s0].a + len(seg), f'{ty} {nm} = {e};\n' + rep))
    return eds


def misc_edits(src, toks, sites, stmt_lists):
    eds = []
    for kind, (i, j, ctx) in sites:
        if j <= i: continue
        try:
            p = P(toks, i, j); root = p.expr()
            if p.p != j: continue
        except (ValueError, IndexError, RecursionError):
            continue
        nodes = []; perm2.walk(root, nodes)
        for node, par in nodes:
            s0, s1 = toks[node.i].a, toks[node.j - 1].b
            t = lambda n: text(src, toks, n.i, n.j)
            if node.k == 'bin' and node.op in ('<', '<=', '>', '>='):
                L, R = node.c
                rt = t(R); lt = t(L)
                # x < n  <->  x <= n-1 ; x >= n <-> x > n-1 (integer literals)
                m = re.match(r'^(0x[0-9a-fA-F]+|\d+)$', rt)
                if m:
                    v = int(rt, 0)
                    if node.op == '<' and v > 0: eds.append((s0, s1, f'{lt} <= {v - 1:#x}' if rt.startswith('0x') else f'{lt} <= {v - 1}'))
                    if node.op == '<=': eds.append((s0, s1, f'{lt} < {v + 1:#x}' if rt.startswith('0x') else f'{lt} < {v + 1}'))
                    if node.op == '>': eds.append((s0, s1, f'{lt} >= {v + 1:#x}' if rt.startswith('0x') else f'{lt} >= {v + 1}'))
                    if node.op == '>=' and v > 0: eds.append((s0, s1, f'{lt} > {v - 1:#x}' if rt.startswith('0x') else f'{lt} > {v - 1}'))
                # a - b < 0  <->  a < b
                if L.k == 'bin' and L.op == '-' and rt in ('0', '0.0f'):
                    eds.append((s0, s1, f'{t(L.c[0])} {node.op} {t(L.c[1])}'))
                # signedness casts
                if not lt.startswith('('):
                    eds.append((s0, s1, f'(int){perm2.wrap(src, toks, L, 3)} {node.op} {rt}'))
                    eds.append((s0, s1, f'(unsigned int){perm2.wrap(src, toks, L, 3)} {node.op} {rt}'))
            if node.k == 'bin' and node.op in ('*', '/', '+', '-') and ctx != 'cond':
                L, R = node.c
                for side in (L, R):
                    st = t(side)
                    if re.match(r'^[A-Za-z_][\w.>-]*$', st):
                        eds.append((toks[side.i].a, toks[side.j - 1].b, f'(float){st}'))
            if node.k == 'cast':
                e = node.c[0]
                eds.append((s0, s1, t(e)))  # drop a cast
            if node.k == 'bin' and node.op == '==' and ctx == 'cond':
                L, R = node.c
                eds.append((s0, s1, f'!({t(L)} != {t(R)})'))
            if node.k == 'un' and node.op == '!' and node.c[0].k == 'paren':
                inner = node.c[0].c[0]
                if inner.k == 'bin' and inner.op in ('&&', '||'):
                    A, B = inner.c
                    neg = lambda n: f'!({t(n)})' if not (n.k == 'bin' and n.op in ('<', '>', '<=', '>=', '==', '!=')) else \
                        f'{t(n.c[0])} {({"<": ">=", ">": "<=", "<=": ">", ">=": "<", "==": "!=", "!=": "=="})[n.op]} {t(n.c[1])}'
                    eds.append((s0, s1, f'{neg(A)} {"||" if inner.op == "&&" else "&&"} {neg(B)}'))
    # statement-level: x = c ? a : b  <->  if (c) x = a; else x = b;   return c ? a : b
    for sts in stmt_lists:
        for idx, (s0, s1, kind) in enumerate(sts):
            seg = text(src, toks, s0, s1)
            if kind == 'simple':
                m = re.match(r'^([^=?;]+?)\s*=\s*(.+?)\s*\?\s*(.+?)\s*:\s*(.+);$', seg, re.S)
                if m and seg.count('?') == 1:
                    x, c, a, b = m.groups()
                    eds.append((toks[s0].a, toks[s1 - 1].b, f'if ({c}) {x} = {a};\nelse {x} = {b};'))
            if kind == 'return':
                m = re.match(r'^return\s+(.+?)\s*\?\s*(.+?)\s*:\s*(.+);$', seg, re.S)
                if m and seg.count('?') == 1:
                    c, a, b = m.groups()
                    eds.append((toks[s0].a, toks[s1 - 1].b, f'if ({c}) return {a};\nreturn {b};'))
            if kind == 'if':
                po = s0 + 1; pc = match_close(toks, po, '(', ')')
                if pc > 0 and pc + 1 < s1 and toks[pc + 1].s == '{':
                    be = match_close(toks, pc + 1, '{', '}')
                    inner = [x.s for x in toks[pc + 2:be]]
                    has_else = be + 1 < s1 and toks[be + 1].s == 'else'
                    # if (c) x = a; else x = b;  -> x = c ? a : b
                    if has_else and be + 2 < s1 and toks[be + 2].s == '{':
                        ee = match_close(toks, be + 2, '{', '}')
                        A = text(src, toks, pc + 2, be).strip(); B = text(src, toks, be + 3, ee).strip()
                        ma = re.match(r'^([^=;]+?)\s*=\s*([^;]+);$', A); mb = re.match(r'^([^=;]+?)\s*=\s*([^;]+);$', B)
                        if ma and mb and ma.group(1).strip() == mb.group(1).strip() and ee + 1 == s1:
                            cond = text(src, toks, po + 1, pc)
                            eds.append((toks[s0].a, toks[s1 - 1].b, f'{ma.group(1).strip()} = {cond} ? {ma.group(2)} : {mb.group(2)};'))
                        ra = re.match(r'^return\s+([^;]+);$', A); rb = re.match(r'^return\s+([^;]+);$', B)
                        if ra and rb and ee + 1 == s1:
                            cond = text(src, toks, po + 1, pc)
                            eds.append((toks[s0].a, toks[s1 - 1].b, f'return {cond} ? {ra.group(1)} : {rb.group(1)};'))
                    # last statement of a block: if (c) { X }  ->  if (!(c)) return/continue/break; X
                    if not has_else and idx == len(sts) - 1 and be + 1 == s1 and 'return' not in inner:
                        cond = text(src, toks, po + 1, pc); body = text(src, toks, pc + 2, be)
                        for jump in ('return;', 'break;', 'continue;'):
                            eds.append((toks[s0].a, toks[s1 - 1].b, f'if (!({cond})) {jump}\n{body}'))
                    # if (c) { A; B; }  ->  if (c) { A; } if (c) { B; }   (two statements, no side effects in c)
                    # else { if (...) ...}  ->  else if
                    if has_else and be + 2 < s1 and toks[be + 2].s == '{':
                        ee = match_close(toks, be + 2, '{', '}')
                        if ee + 1 == s1 and toks[be + 3].s == 'if':
                            eb = text(src, toks, be + 3, ee)
                            eds.append((toks[be + 2].a, toks[s1 - 1].b, eb))
            # any two adjacent statements swap (perm2 only swaps provably independent ones)
            if idx + 1 < len(sts):
                b0, b1, kb = sts[idx + 1]
                if kind in ('simple', 'decl', 'if') and kb in ('simple', 'decl', 'if'):
                    ta, tb = text(src, toks, s0, s1), text(src, toks, b0, b1)
                    blocked = False
                    if kind == 'decl':
                        dn = re.findall(r'(\w+)\s*(?:=|;|\[)', ta)
                        blocked = bool(dn) and re.search(r'\b' + re.escape(dn[0]) + r'\b', tb) is not None
                    if not blocked: eds.append((toks[s0].a, toks[b1 - 1].b, tb + '\n' + ta))
    return eds


_EDC = {}
def all_edits(fn, void_fn):
    key = (canon(fn), void_fn)
    if key in _EDC: return _EDC[key]
    if len(_EDC) > 150: _EDC.clear()   # entries can hold MBs of replacement text
    r = _all_edits(fn, void_fn); _EDC[key] = r
    return r


def _all_edits(fn, void_fn):
    toks = tokenize(fn)
    sites, stmt_lists = scan(toks)
    eds = []
    for kind, (i, j, ctx) in sites:
        if j > i: eds += perm2.expr_edits(fn, toks, i, j, ctx)
    eds += perm2.stmt_edits(fn, toks, stmt_lists, void_fn)
    eds += sink_hoist_edits(fn, toks, stmt_lists)
    eds += misc_edits(fn, toks, sites, stmt_lists)
    # dedupe by (span, replacement)
    seen = set(); out = []
    for e in eds:
        if e in seen: continue
        seen.add(e); out.append(e)
    return out


def apply_edit(fn, e):
    a, b, rep = e
    return fn[:a] + rep + fn[b:]


def canon(s):
    return re.sub(r'\s+', ' ', s).strip()


# ---------------------------------------------------------------- one function's search state
class Search:
    def __init__(self, unit, va, secs, outdir, beam=3, maxdepth=6):
        self.unit, self.va, self.secs, self.outdir = unit, va, secs, outdir
        self.beam, self.maxdepth = beam, maxdepth
        self.done = False; self.ctx = None; self.err = None
        self.jobs = {}      # jid -> (variant text, depth)
        self.seen = set(); self.results = []   # (score, text)
        self.best = None; self.bs = None; self.nd0 = None
        self.frontier = []  # (score, text, depth) to expand
        self.pending = []   # candidate texts waiting to be scheduled
        self.tries = 0; self.t0 = None; self.phase = 'init'
        self.rng = random.Random(int(va, 16))
        self.expanded = set(); self.dry = 0

    def prepare(self, pool_check):
        """Locate the function, build the reduced unit + PCH, validate.  Runs in the main process."""
        import check
        row = next((r for r in csv.DictReader(open(f'{ROOT}/symbols/functions.csv')) if r['va'] == self.va), None)
        if row is None: self.err = 'no such va'; return False
        self.sym, self.dem = row['name'], row['demangled']; self.full = int(row['size'])
        path = perm2.find_unit_src(self.unit); usrc = open(path, encoding='latin-1').read()
        self.void_fn = False
        pos = perm2.locate(usrc, self.dem); hdr = None
        if pos is None:
            for h in sorted(glob.glob(f'{ROOT}/include/*.h')):
                hs = open(h, encoding='latin-1').read()
                hp = perm2.locate_inline(hs, self.dem)
                if hp is not None: hdr, pos = h, hp; break
            if pos is None: self.err = 'cannot locate ' + self.dem; return False
        self.work = tempfile.mkdtemp(prefix='p3_' + self.unit + '_')
        inc = [self.work, os.path.dirname(path)]
        X = check.XBE(os.path.join(ROOT, 'orig', 'DEFAULT.XBE'))
        if hdr:
            # header inline: the edited function lives in a shadow copy of the header; the unit is compiled whole
            hs = open(hdr, encoding='latin-1').read()
            pre, fn, post = perm2.split_fn(hs, pos)
            self.hdr_name = os.path.basename(hdr); self.hdr_pre, self.hdr_post = pre, post
            self.fn0 = fn
            self.void_fn = re.match(r'\s*(static\s+|inline\s+|__forceinline\s+|virtual\s+)*void\b', hs[pos:pos + 80]) is not None
            self.ctx = dict(unit=self.unit, sym=self.sym, va=int(self.va, 16), full=self.full, pre='', post='', pch=None, inc=inc, hdr=self.hdr_name, unit_text=usrc)
            # the body sent to workers is the whole unit text; the header shadow must be written per job, so
            # fold the header into the unit text instead: #include it first with the edited definition inline.
            self.unit_text = usrc
            self.mode = 'hdr'
            # validate baseline compiles
            o, log = compile_variant(usrc, self.work, 'base', None, [shadow_header(self.__dict__.setdefault('_sh', {'dir': self.work}), self.hdr_name, self.render(fn))] + inc, 300)
            if o is None: self.err = 'base does not compile: ' + log[-200:]; return False
            s, ok, nd = score_obj(o, self.sym, X, int(self.va, 16), self.full, check)
            if s is None: self.err = 'symbol not emitted'; return False
            self.bs0 = s; self.nd0 = nd
            return True
        pre, fn, post = perm2.split_fn(usrc, pos)
        self.fn0 = fn
        self.void_fn = re.match(r'\s*(static\s+|inline\s+|virtual\s+)*void\b', usrc[pos:pos + 80]) is not None
        # full-unit baseline
        o_full, log = compile_variant(usrc, self.work, 'full', None, inc, 300)
        if o_full is None: self.err = 'unit does not compile: ' + log[-200:]; return False
        s, ok, nd = score_obj(o_full, self.sym, X, int(self.va, 16), self.full, check)
        if s is None: self.err = 'symbol not emitted'; return False
        self.bs0 = s; self.nd0 = nd
        ref_bytes, ref_rel = fn_bytes(o_full, self.sym)
        # reduced unit
        red = reduce_unit(usrc, pos)
        head = usrc[pos:usrc.index('{', pos) + 1]
        rpos = red.index(head) + len(head)
        assert red[rpos:rpos + len(fn)] == fn
        rpre, rpost = red[:rpos], red[rpos + len(fn):]
        # precompiled header
        hpre, rest_pre = split_pch(rpre)
        pch = None
        if hpre.strip():
            pch_h = f'pch_{self.unit}.h'
            open(os.path.join(self.work, pch_h), 'w', encoding='latin-1').write(hpre)
            open(os.path.join(self.work, 'mkpch.cpp'), 'w').write(f'#include "{pch_h}"\n')
            pchf = os.path.join(self.work, self.unit + '.pch')
            rc, lg = cl(['/Yc' + pch_h, '/Fp' + wpath(pchf), '/Fo' + wpath(os.path.join(self.work, 'mkpch.obj')), wpath(os.path.join(self.work, 'mkpch.cpp'))], self.work, inc, 300)
            if rc == 0 and os.path.exists(pchf):
                pch = (pch_h, pchf); rpre = f'#include "{pch_h}"\n' + rest_pre
        self.mode = 'full'
        for cand_pre, cand_post, cand_pch, label in ((rpre, rpost, pch, 'reduced+pch'), (red[:rpos], rpost, None, 'reduced'), (pre, post, None, 'full')):
            o, log = compile_variant(cand_pre + fn + cand_post, self.work, 'val', cand_pch, inc, 300)
            if o is None: continue
            b, rel = fn_bytes(o, self.sym)
            if b == ref_bytes and [(p, n, t) for p, n, t in rel] == [(p, n, t) for p, n, t in ref_rel]:
                self.ctx = dict(unit=self.unit, sym=self.sym, va=int(self.va, 16), full=self.full, pre=cand_pre, post=cand_post, pch=cand_pch, inc=inc)
                self.mode = label; return True
        self.err = 'no compilable form'; return False

    def render(self, fn):
        if getattr(self, 'mode', None) == 'hdr':
            return self.hdr_pre + fn + self.hdr_post
        return fn

    def start(self):
        self.t0 = time.time()
        self.best, self.bs = self.fn0, self.bs0
        self.maxlen = int(len(self.fn0) * 1.4) + 80
        self.seen.add(canon(self.fn0))
        if self.bs0 == 0: self.done = True; return
        self.frontier = [(self.bs0, self.fn0, 0)]
        self.phase = 'beam'
        self._expand()

    def _expand(self):
        """Queue the single-edit neighbourhood of the best unexpanded frontier entries."""
        self.frontier.sort(key=lambda x: x[0])
        added = 0
        for sc, txt, d in self.frontier[:self.beam]:
            key = canon(txt)
            if key in self.expanded or d >= self.maxdepth: continue
            self.expanded.add(key)
            try: eds = all_edits(txt, self.void_fn)
            except Exception: eds = []
            self.rng.shuffle(eds)
            for e in eds:
                v = apply_edit(txt, e)
                if len(v) > self.maxlen: continue   # edits that only grow the text (cast on cast...) go nowhere
                k = canon(v)
                if k in self.seen: continue
                self.seen.add(k); self.pending.append((v, d + 1)); added += 1
        return added

    def _random(self, n):
        """Random 2-3 edit combinations from the best few results."""
        pool = sorted(self.results)[:5] or [(self.bs, self.best)]
        added = 0; attempts = 0
        while added < n and attempts < n * 3:
            attempts += 1
            _, base = pool[self.rng.randrange(len(pool))]
            v = base
            for _ in range(2 + (self.rng.random() < 0.3)):
                try: eds = all_edits(v, self.void_fn)
                except Exception: eds = []
                if not eds: break
                v = apply_edit(v, self.rng.choice(eds))
            if len(v) > self.maxlen: continue
            k = canon(v)
            if k in self.seen: continue
            self.seen.add(k); self.pending.append((v, 99)); added += 1
        return added

    def next_job(self, jid):
        if self.done: return None
        if time.time() - self.t0 > self.secs:
            # out of time: drop the queue, finish once the jobs already in the pool come back
            self.pending = []
            if not self.jobs: self.done = True
            return None
        if not self.pending:
            if not self._expand():
                self.phase = 'random'
                if not self._random(32):
                    self.dry += 1
                    if self.dry > 3 and not self.jobs: self.done = True   # neighbourhood exhausted
                    return None
        v, d = self.pending.pop()
        self.jobs[jid] = (v, d)
        return (jid, self.ctx, self.render(v) if self.mode == 'hdr' else v)

    def report(self, jid, score, ok, nd):
        v, d = self.jobs.pop(jid)
        self.tries += 1
        if score is None: return
        self.results.append((score, v))
        if score < self.bs:
            self.best, self.bs = v, score
        if ok:
            self.bs = 0; self.best = v; self.done = True; self.pending = []
            return
        # keep expanding from anything within 1.5x of the best, to get around plateaus
        if score <= max(self.bs * 1.5, self.bs + 10) and d < self.maxdepth:
            self.frontier.append((score, v, d))

    def finish(self):
        os.makedirs(self.outdir, exist_ok=True)
        tag = f'{self.outdir}/{self.unit}_{self.va}'
        secs = int(time.time() - (self.t0 or time.time()))
        if self.err:
            open(tag + '.txt', 'w').write(f'{self.unit} {self.va} error {self.err}\n')
            print(f'{self.unit} {self.va}: {self.err}', flush=True)
        else:
            open(tag + '.best.cpp', 'w', encoding='latin-1').write(self.best)
            open(tag + '.txt', 'w').write(f'{self.unit} {self.va} score {self.bs} start {self.bs0} tries {self.tries} secs {secs} mode {self.mode}\n{self.dem}\n')
            print(f'{self.unit} {self.va} {"MATCH" if self.bs == 0 else "best %d (from %d)" % (self.bs, self.bs0)} tries {self.tries} {secs}s [{self.mode}]', flush=True)
        shutil.rmtree(getattr(self, 'work', ''), ignore_errors=True)


# ---------------------------------------------------------------- scheduler
def run(targets, secs_per, jobs, inflight, outdir):
    os.makedirs(outdir, exist_ok=True)
    pool = mp.Pool(jobs, initializer=w_init, initargs=(FLAGS,))
    queue = list(targets); active = []; infl = {}; jid = 0
    t_start = time.time(); n_match = 0; n_done = 0
    try:
        while queue or active or infl:
            # start new searches
            while len(active) < inflight and queue:
                unit, va = queue.pop(0)
                done_f = f'{outdir}/{unit}_{va}.txt'
                if os.path.exists(done_f) and ' error ' not in open(done_f, errors='replace').readline(): continue
                s = Search(unit, va, secs_per, outdir)
                print(f'[{n_done}/{n_done + len(queue) + len(active) + 1}] {unit} {va} preparing', flush=True)
                if s.prepare(None):
                    s.start()
                    print(f'  {unit} {va}: start score {s.bs0} (ndiff {s.nd0}) [{s.mode}]', flush=True)
                else:
                    s.finish(); n_done += 1; continue
                if s.done: s.finish(); n_done += 1; n_match += 1; continue
                active.append(s)
            # fill the pool
            while len(infl) < jobs * 2 and active:
                progressed = False
                for s in sorted(active, key=lambda x: len(x.jobs)):   # fewest in flight first: no starvation
                    j = s.next_job(jid)
                    if j is not None:
                        infl[jid] = (s, pool.apply_async(w_job, (j,))); jid += 1; progressed = True
                        if len(infl) >= jobs * 2: break
                if not progressed: break
            # collect
            if not infl:
                for s in list(active):
                    if s.done or (not s.pending and not s.jobs and time.time() - s.t0 > s.secs):
                        s.done = True; active.remove(s); s.finish(); n_done += 1; n_match += s.bs == 0
                if not active and not queue: break
                time.sleep(0.05); continue
            done_any = False
            for k, (s, ar) in list(infl.items()):
                if ar.ready():
                    done_any = True
                    try:
                        rj, score, ok, nd, log = ar.get()
                    except Exception as e:
                        rj, score, ok, nd, log = k, None, False, None, str(e)
                    del infl[k]
                    s.report(rj, score, ok, nd)
                    if s.done and not s.jobs:
                        if s in active: active.remove(s)
                        s.finish(); n_done += 1; n_match += s.bs == 0
            if not done_any: time.sleep(0.02)
            for s in list(active):
                if not s.jobs and (s.done or (not s.pending and time.time() - s.t0 > s.secs)):
                    s.done = True; active.remove(s); s.finish(); n_done += 1; n_match += s.bs == 0
    finally:
        pool.terminate(); pool.join()
    print(f'done: {n_done} functions, {n_match} matches, {int(time.time() - t_start)} s', flush=True)


def make_list(maxscore, minsize):
    fns = {r['va']: r for r in csv.DictReader(open(f'{ROOT}/symbols/functions.csv'))}
    rows = [r for r in csv.DictReader(open(f'{ROOT}/status/status.csv')) if r['status'] == 'DIFF' and int(r['ndiff']) <= maxscore]
    rows = [r for r in rows if int(fns[r['va']]['size']) >= minsize]
    # drop stubs: a '// TODO' body scores a tiny ndiff (one 'ret' byte) but there is nothing to permute
    srcs = {}; keep = []
    for r in rows:
        path = perm2.find_unit_src(r['unit'])
        if path not in srcs: srcs[path] = open(path, encoding='latin-1').read() if path else ''
        src = srcs[path]; pos = perm2.locate(src, fns[r['va']]['demangled']) if src else None
        if pos is not None:
            try: _, body, _ = perm2.split_fn(src, pos)
            except Exception: body = ''
            if 'TODO' in body or body.count(';') == 0: continue
        keep.append(r)
    rows = keep
    rows.sort(key=lambda r: (int(r['ndiff']), int(fns[r['va']]['size'])))
    os.makedirs(f'{ROOT}/build/perm', exist_ok=True)
    with open(f'{ROOT}/build/perm/targets.txt', 'w') as f:
        for r in rows: f.write(f"{r['unit']} {r['va']} {r['ndiff']} {fns[r['va']]['size']}\n")
    print(len(rows), 'targets -> build/perm/targets.txt')


if __name__ == '__main__':
    mp.freeze_support()
    ap = argparse.ArgumentParser()
    ap.add_argument('mode'); ap.add_argument('a', nargs='?'); ap.add_argument('b', nargs='?')
    ap.add_argument('--secs', type=int, default=300); ap.add_argument('--secs-per', type=int, default=300)
    ap.add_argument('--jobs', type=int, default=max(1, (os.cpu_count() or 2) - 1))
    ap.add_argument('--inflight', type=int, default=4)
    ap.add_argument('--max-score', type=int, default=400); ap.add_argument('--min-size', type=int, default=0)
    ap.add_argument('--out', default=f'{ROOT}/build/perm')
    a = ap.parse_args()
    if a.mode == 'list':
        make_list(a.max_score, a.min_size)
    elif a.mode == 'one':
        run([(a.a, a.b.lower().zfill(8))], a.secs, a.jobs, 1, a.out)
    elif a.mode == 'batch':
        tg = []
        for line in open(a.a):
            p = line.split()
            if len(p) >= 2: tg.append((p[0], p[1]))
        run(tg, a.secs_per, a.jobs, a.inflight, a.out)
