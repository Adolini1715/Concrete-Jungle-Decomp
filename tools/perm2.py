#!/usr/bin/env python3
"""perm2.py: source permuter for near-miss functions.

    perm2.py one UNIT VA [--secs N] [--workers N] [--seed N]
    perm2.py batch LISTFILE [--secs-per N] [--workers N] [--max-score N]
    perm2.py list [--max-score N]          write build/perm/targets.txt (DIFF functions, best first)

For one function it parses the definition into statements and expressions, applies a random
behaviour-preserving edit, recompiles the unit, and scores the function against the original
(differing bytes; 0 = match). Walkers keep edits that score no worse (a few worse ones too, to
escape local minima) and remember the best. Results go to build/perm/<unit>_<va>.{best.cpp,txt}:
the .best.cpp holds only the winning function text, ready for tools/fnrep.py.

Edits (all keep the program's meaning):
  swap adjacent independent statements; reorder adjacent declarations
  swap operands of commutative + * & | ^ == != (operands are parenthesised as needed)
  flip relational a<b -> b>a;  x op= y <-> x = x op y;  !e <-> e == 0 (in conditions)
  if (A && B) {..} -> if (A) { if (B) {..} };  if (c) {X} else {Y} -> if (!(c)) {Y} else {X}
  c ? x : y -> !(c) ? y : x;  for (i; c; s) {..} -> i; while (c) {..; s}  (no continue inside)
  early return: if (c) { X } return; -> if (!(c)) return; X ... (only at function end)
"""
import os, re, sys, random, time, glob, csv, tempfile, shutil, argparse, multiprocessing as mp
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE); sys.path.insert(0, HERE)

# ---------------------------------------------------------------- tokens
TOK = re.compile(r'''\s+|//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'|
    0[xX][0-9a-fA-F]+[uUlL]*|\d+\.\d*(?:[eE][-+]?\d+)?[fF]?|\d*\.\d+(?:[eE][-+]?\d+)?[fF]?|\d+[uUlLfF]*|
    [A-Za-z_]\w*|->|\+\+|--|<<=|>>=|<<|>>|<=|>=|==|!=|&&|\|\||[-+*/%&|^]=|::|[{}()\[\];,?:.<>=!~+\-*/%&|^]''',
                 re.S | re.X)


class T:
    __slots__ = ('s', 'a', 'b')
    def __init__(self, s, a, b): self.s, self.a, self.b = s, a, b
    def __repr__(self): return self.s


def tokenize(src):
    out, i = [], 0
    while i < len(src):
        m = TOK.match(src, i)
        if not m: raise ValueError('lex at ' + src[i:i + 20])
        s = m.group(0)
        if not (s.isspace() or s.startswith('//') or s.startswith('/*')): out.append(T(s, i, m.end()))
        i = m.end()
    return out


TYPEWORDS = set('int float double char short long unsigned signed void bool const volatile u8 s8 u16 s16 u32 s32 '
                'u64 s64 f32 BOOL DWORD WORD BYTE UINT'.split())
KEYW = set('if else for while do switch case default return break continue goto sizeof new delete'.split())
BIN = {'*': 5, '/': 5, '%': 5, '+': 6, '-': 6, '<<': 7, '>>': 7, '<': 8, '>': 8, '<=': 8, '>=': 8,
       '==': 9, '!=': 9, '&': 10, '^': 11, '|': 12, '&&': 13, '||': 14}
ASSIGN = {'=', '+=', '-=', '*=', '/=', '%=', '&=', '|=', '^=', '<<=', '>>='}
COMM = {'+', '*', '&', '|', '^', '==', '!='}
FLIP = {'<': '>', '>': '<', '<=': '>=', '>=': '<='}


class N:
    """Expression node: kind in (prim, un, post, bin, tern, asg, cast, paren); i..j token span (j exclusive)."""
    __slots__ = ('k', 'op', 'i', 'j', 'c')
    def __init__(self, k, op, i, j, c): self.k, self.op, self.i, self.j, self.c = k, op, i, j, c


class P:
    def __init__(self, toks, i, j):
        self.t, self.p, self.end = toks, i, j

    def peek(self, d=0):
        q = self.p + d
        return self.t[q].s if q < self.end else None

    def expr(self, minp=17):
        left = self.unary()
        while True:
            op = self.peek()
            if op is None: break
            if op in ASSIGN and minp >= 16:
                i = left.i; self.p += 1; r = self.expr(16)
                left = N('asg', op, i, r.j, [left, r]); continue
            if op == '?' and minp >= 15:
                i = left.i; self.p += 1; a = self.expr(16)
                if self.peek() != ':': raise ValueError('?:')
                self.p += 1; b = self.expr(15)
                left = N('tern', '?', i, b.j, [left, a, b]); continue
            if op == ',' and minp >= 17:
                i = left.i; self.p += 1; r = self.expr(16)
                left = N('bin', ',', i, r.j, [left, r]); continue
            pr = BIN.get(op)
            if pr is None or pr >= minp: break
            i = left.i; self.p += 1; r = self.expr(pr)
            left = N('bin', op, i, r.j, [left, r])
        return left

    def is_cast(self):
        # '(' type-words [*&]* ')' followed by an operand
        q = self.p + 1; words = 0
        while q < self.end and self.t[q].s != ')':
            s = self.t[q].s
            if s in ('*', '&', '::', '<', '>', ','): q += 1; continue
            if re.match(r'[A-Za-z_]\w*$', s) and s not in KEYW: words += 1; q += 1; continue
            return False
        if q >= self.end or words == 0: return False
        inner = [x.s for x in self.t[self.p + 1:q]]
        nxt = self.t[q + 1].s if q + 1 < self.end else None
        if nxt is None or not (re.match(r'[\w"\'(]', nxt) or nxt in ('!', '~', '-', '*', '&')): return False
        if nxt == '(' and len(inner) == 1 and inner[0] not in TYPEWORDS: return False
        return any(w in TYPEWORDS for w in inner) or inner[-1] in ('*', '&') or \
            (len(inner) == 1 and inner[0][:1].isupper() and nxt not in ('-', '*', '&'))

    def unary(self):
        s = self.peek(); i = self.p
        if s is None: raise ValueError('eof')
        if s in ('!', '~', '-', '+', '*', '&', '++', '--'):
            self.p += 1; e = self.unary(); return N('un', s, i, e.j, [e])
        if s == 'sizeof':
            self.p += 1
            if self.peek() == '(':
                self.skip_group(); return N('prim', 'sizeof', i, self.p, [])
            e = self.unary(); return N('un', 'sizeof', i, e.j, [e])
        if s == '(' and self.is_cast():
            while self.t[self.p].s != ')': self.p += 1
            self.p += 1; e = self.unary(); return N('cast', None, i, e.j, [e])
        if s == '(':
            self.p += 1; e = self.expr()
            if self.peek() != ')': raise ValueError(')')
            self.p += 1; prim = N('paren', None, i, self.p, [e])
        elif re.match(r'[\w"\'.]', s) and s not in KEYW or s == 'this':
            self.p += 1
            while self.peek() == '::' and self.peek(1) and re.match(r'[~\w]', self.peek(1)): self.p += 2
            prim = N('prim', None, i, self.p, [])
        else:
            raise ValueError('primary ' + s)
        while True:
            s = self.peek()
            if s == '(':
                self.skip_group(); prim = N('post', 'call', i, self.p, [prim])
            elif s == '[':
                self.skip_group('[', ']'); prim = N('post', 'idx', i, self.p, [prim])
            elif s in ('->', '.'):
                self.p += 2; prim = N('post', s, i, self.p, [prim])
            elif s in ('++', '--'):
                self.p += 1; prim = N('post', s, i, self.p, [prim])
            else:
                return prim

    def skip_group(self, o='(', c=')'):
        d = 0
        while self.p < self.end:
            s = self.t[self.p].s; self.p += 1
            if s == o: d += 1
            elif s == c:
                d -= 1
                if d == 0: return
        raise ValueError('group')


def match_close(toks, i, o, c):
    d = 0
    for q in range(i, len(toks)):
        if toks[q].s == o: d += 1
        elif toks[q].s == c:
            d -= 1
            if d == 0: return q
    raise ValueError('unbalanced ' + o + c)   # -1 used to send scan() into an endless loop


# ---------------------------------------------------------------- statement scan
def scan(toks):
    """Yield (kind, info) sites. Expression regions: ('expr', (i, j, ctx)); statements: lists."""
    sites = []
    q, n = 0, len(toks)
    stmt_lists = []  # list of statement spans per block, for swap/early-return edits
    def block(i, j):
        # statements between toks[i..j)
        sts = []
        q = i
        while q < j:
            s = toks[q].s
            st = q
            if len(sts) > 4 * (j - i) + 8: raise ValueError('scan stuck')
            if s == '{':
                e = match_close(toks, q, '{', '}'); block(q + 1, e); q = e + 1; sts.append((st, q, 'blk')); continue
            if s in ('if', 'while', 'switch', 'for'):
                po = q + 1; pc = match_close(toks, po, '(', ')')
                if s == 'for':
                    semis = [k for k in range(po + 1, pc) if toks[k].s == ';' and depth(toks, po + 1, k) == 0]
                    if len(semis) == 2:
                        sites.append(('expr', (po + 1, semis[0], 'stmt')))
                        sites.append(('expr', (semis[0] + 1, semis[1], 'cond')))
                        sites.append(('expr', (semis[1] + 1, pc, 'stmt')))
                else:
                    sites.append(('expr', (po + 1, pc, 'cond' if s != 'switch' else 'val')))
                q = pc + 1
                q = sub_stmt(q, j)
                if s == 'if' and q < j and toks[q].s == 'else':
                    q = sub_stmt(q + 1, j)
                sts.append((st, q, s)); continue
            if s == 'do':
                q = sub_stmt(q + 1, j)
                if q < j and toks[q].s == 'while':
                    pc = match_close(toks, q + 1, '(', ')'); sites.append(('expr', (q + 2, pc, 'cond'))); q = pc + 2
                sts.append((st, q, 'do')); continue
            if s in ('case', 'default'):
                while q < j and toks[q].s != ':': q += 1
                q += 1; sts.append((st, q, 'label')); continue
            # simple statement up to ';' at depth 0
            e = q; d = 0
            while e < j:
                if toks[e].s in '({[': d += 1
                elif toks[e].s in ')}]': d -= 1
                elif toks[e].s == ';' and d == 0: break
                e += 1
            kind = 'simple'
            if s == 'return':
                if e > q + 1: sites.append(('expr', (q + 1, e, 'val')))
                kind = 'return'
            elif s in ('break', 'continue', 'goto'):
                kind = s
            elif is_decl(toks, q, e):
                kind = 'decl'
                eq = next((k for k in range(q, e) if toks[k].s == '=' and depth(toks, q, k) == 0), None)
                if eq: sites.append(('expr', (eq + 1, e, 'val')))
            else:
                sites.append(('expr', (q, e, 'stmt')))
            q = e + 1; sts.append((st, q, kind))
        stmt_lists.append(sts)
    def sub_stmt(q, j):
        if toks[q].s == '{':
            e = match_close(toks, q, '{', '}'); block(q + 1, e); return e + 1
        # single statement: recurse via a one-statement block
        k = q; d = 0
        if toks[q].s in ('if', 'while', 'for', 'switch', 'do'):
            before = len(stmt_lists); tmp = []
            block_one = block_end(q, j); block(q, block_one); return block_one
        while k < j:
            if toks[k].s in '({[': d += 1
            elif toks[k].s in ')}]': d -= 1
            elif toks[k].s == ';' and d == 0: break
            k += 1
        block(q, k + 1); return k + 1
    def block_end(q, j):
        # end of one compound statement starting at q (if/while/for/switch/do)
        s = toks[q].s
        if s == 'do':
            e = stmt_end(q + 1, j); pc = match_close(toks, e + 1, '(', ')'); return pc + 2
        pc = match_close(toks, q + 1, '(', ')'); e = stmt_end(pc + 1, j)
        if s == 'if' and e < j and toks[e].s == 'else': e = stmt_end(e + 1, j)
        return e
    def stmt_end(q, j):
        if toks[q].s == '{': return match_close(toks, q, '{', '}') + 1
        if toks[q].s in ('if', 'while', 'for', 'switch', 'do'): return block_end(q, j)
        d = 0; k = q
        while k < j:
            if toks[k].s in '({[': d += 1
            elif toks[k].s in ')}]': d -= 1
            elif toks[k].s == ';' and d == 0: return k + 1
            k += 1
        return j
    block(0, n)
    return sites, stmt_lists


def depth(toks, i, k):
    d = 0
    for q in range(i, k):
        if toks[q].s in '([{': d += 1
        elif toks[q].s in ')]}': d -= 1
    return d


def is_decl(toks, q, e):
    s = [t.s for t in toks[q:e]]
    if not s: return False
    k = 0
    while k < len(s) and s[k] in ('const', 'static', 'volatile', 'register', 'unsigned', 'signed', 'struct', 'class'): k += 1
    if k >= len(s) or not re.match(r'[A-Za-z_]\w*$', s[k]) or s[k] in KEYW: return False
    k += 1
    while k < len(s) and s[k] in ('::',) and k + 1 < len(s): k += 2
    if k < len(s) and s[k] == '<':
        d = 0
        while k < len(s):
            if s[k] == '<': d += 1
            elif s[k] == '>':
                d -= 1
                if d == 0: k += 1; break
            k += 1
    while k < len(s) and s[k] in ('*', '&', 'const'): k += 1
    return k < len(s) and re.match(r'[A-Za-z_]\w*$', s[k]) is not None and s[k] not in KEYW and \
        (k + 1 == len(s) or s[k + 1] in ('=', '[', ',', ';', '('))


# ---------------------------------------------------------------- edits
def text(src, toks, i, j):
    return src[toks[i].a:toks[j - 1].b] if j > i else ''


def has_side(src, toks, node):
    s = ' '.join(t.s for t in toks[node.i:node.j])
    return '(' in s and re.search(r'[\w)\]]\s*\(', s) is not None or '++' in s or '--' in s or \
        any(f' {a} ' in f' {s} ' for a in ASSIGN)


def walk(node, out, parent=None):
    out.append((node, parent))
    for c in node.c: walk(c, out, node)


def wrap(src, toks, node, prec):
    t = text(src, toks, node.i, node.j)
    np = BIN.get(node.op, 0) if node.k == 'bin' else (15 if node.k == 'tern' else 16 if node.k == 'asg' else 0)
    return f'({t})' if np >= prec else t


def expr_edits(src, toks, i, j, ctx):
    try:
        p = P(toks, i, j); root = p.expr()
        if p.p != j: return []
    except (ValueError, IndexError, RecursionError):
        return []
    nodes = []; walk(root, nodes)
    eds = []
    a, b = toks[i].a, toks[j - 1].b
    for node, par in nodes:
        s0, s1 = toks[node.i].a, toks[node.j - 1].b
        if node.k == 'bin' and node.op in COMM:
            L, R = node.c
            if has_side(src, toks, L) and has_side(src, toks, R): continue
            pr = BIN[node.op]
            eds.append((s0, s1, f'{wrap(src, toks, R, pr)} {node.op} {wrap(src, toks, L, pr + 0.5)}'))
        if node.k == 'bin' and node.op in FLIP:
            L, R = node.c
            if has_side(src, toks, L) and has_side(src, toks, R): continue
            pr = BIN[node.op]
            eds.append((s0, s1, f'{wrap(src, toks, R, pr)} {FLIP[node.op]} {wrap(src, toks, L, pr + 0.5)}'))
        if node.k == 'asg' and node.op != '=' and node.op[:-1] in BIN:
            L, R = node.c
            if has_side(src, toks, L): continue
            o = node.op[:-1]
            eds.append((s0, s1, f'{text(src, toks, L.i, L.j)} = {text(src, toks, L.i, L.j)} {o} {wrap(src, toks, R, BIN[o] + 0.5)}'))
        if node.k == 'asg' and node.op == '=' and node.c[1].k == 'bin' and node.c[1].op + '=' in ASSIGN:
            L, R = node.c; RL, RR = R.c
            lt = text(src, toks, L.i, L.j)
            if text(src, toks, RL.i, RL.j) == lt:
                eds.append((s0, s1, f'{lt} {R.op}= {text(src, toks, RR.i, RR.j)}'))
            elif R.op in COMM and text(src, toks, RR.i, RR.j) == lt and not has_side(src, toks, RL):
                eds.append((s0, s1, f'{lt} {R.op}= {text(src, toks, RL.i, RL.j)}'))
        if node.k == 'tern':
            c, x, y = node.c
            eds.append((s0, s1, f'!({text(src, toks, c.i, c.j)}) ? {text(src, toks, y.i, y.j)} : {wrap(src, toks, x, 15)}'))
        if ctx == 'stmt' and par is None:
            if node.k == 'post' and node.op in ('++', '--'):
                e = node.c[0]; eds.append((s0, s1, f'{node.op}{text(src, toks, e.i, e.j)}'))
                eds.append((s0, s1, f'{text(src, toks, e.i, e.j)} {node.op[0]}= 1'))
            if node.k == 'un' and node.op in ('++', '--'):
                e = node.c[0]; eds.append((s0, s1, f'{text(src, toks, e.i, e.j)}{node.op}'))
            if node.k == 'asg' and node.op in ('+=', '-=') and text(src, toks, node.c[1].i, node.c[1].j) == '1':
                e = node.c[0]; eds.append((s0, s1, f'{text(src, toks, e.i, e.j)}{node.op[0] * 2}'))
        cond = ctx == 'cond' and (par is None or (par.k == 'bin' and par.op in ('&&', '||')) or (par.k == 'un' and par.op == '!'))
        if cond and node.k == 'un' and node.op == '!' and not (par and par.k == 'un'):
            e = node.c[0]
            eds.append((s0, s1, f'{wrap(src, toks, e, 9)} == 0'))
        if cond and node.k == 'bin' and node.op == '==' and text(src, toks, node.c[1].i, node.c[1].j) in ('0', 'NULL'):
            eds.append((s0, s1, f'!{wrap(src, toks, node.c[0], 1)}'))
        if cond and node.k == 'bin' and node.op == '!=' and text(src, toks, node.c[1].i, node.c[1].j) in ('0', 'NULL'):
            eds.append((s0, s1, wrap(src, toks, node.c[0], 13)))
        if cond and node.k not in ('bin', 'un') or (cond and node.k == 'bin' and node.op not in ('&&', '||', '==', '!=') and BIN.get(node.op, 0) < 8):
            eds.append((s0, s1, f'{wrap(src, toks, node, 9)} != 0'))
    return eds


IDENT = re.compile(r'[A-Za-z_]\w*')


def ids(s): return set(IDENT.findall(s)) - KEYW - TYPEWORDS


def stmt_edits(src, toks, stmt_lists, void_fn=False):
    eds = []
    top = stmt_lists[-1] if stmt_lists else []
    if void_fn and top and top[-1][2] == 'if':
        s0, s1, _ = top[-1]
        po = s0 + 1; pc = match_close(toks, po, '(', ')')
        if pc > 0 and pc + 1 < s1 and toks[pc + 1].s == '{':
            be = match_close(toks, pc + 1, '{', '}')
            if be + 1 == s1:
                cond = text(src, toks, po + 1, pc); body = text(src, toks, pc + 2, be)
                eds.append((toks[s0].a, toks[s1 - 1].b, f'if (!({cond})) return;\n{body}'))
    # guard 'if (c) return;' followed by the rest -> wrap the rest (reverse of the above)
    if void_fn and len(top) >= 2 and top[0][2] == 'if':
        s0, s1, _ = top[0]
        seg = [x.s for x in toks[s0:s1]]
        if seg[-2:] == ['return', ';'] and 'else' not in seg:
            po = s0 + 1; pc = match_close(toks, po, '(', ')')
            if pc + 3 == s1 or (toks[pc + 1].s == '{' and pc + 5 == s1):
                cond = text(src, toks, po + 1, pc); rest = src[toks[top[1][0]].a:toks[top[-1][1] - 1].b]
                if 'return' not in [x.s for x in toks[top[1][0]:top[-1][1]]]:
                    eds.append((toks[s0].a, toks[top[-1][1] - 1].b, f'if (!({cond})) {{\n{rest}\n}}'))
    for sts in stmt_lists:
        for k in range(len(sts) - 1):
            (a0, a1, ka), (b0, b1, kb) = sts[k], sts[k + 1]
            if ka in ('simple', 'decl') and kb in ('simple', 'decl') and ka == kb:
                ta, tb = text(src, toks, a0, a1), text(src, toks, b0, b1)
                if indep(ta, tb, ka == 'decl'):
                    eds.append((toks[a0].a, toks[b1 - 1].b, tb + '\n' + ta))
        for (s0, s1, kind) in sts:
            if kind == 'if':
                eds += if_edits(src, toks, s0, s1)
            if kind == 'for':
                e = for_to_while(src, toks, s0, s1)
                if e: eds.append(e)
    return eds


def indep(a, b, decl):
    lhs = lambda s: set(IDENT.findall(s.split('=')[0])) if re.search(r'[^=!<>]=[^=]', s) else set()
    call = lambda s: re.search(r'\w\s*\(', s) is not None
    mem = lambda s: '->' in s or '*' in s or '[' in s or '.' in s
    ia, ib = ids(a), ids(b)
    if decl:
        # declarations: the second must not use the first's name
        na = re.findall(r'(\w+)\s*(?:=|;|\[)', a)[:1]; nb = re.findall(r'(\w+)\s*(?:=|;|\[)', b)[:1]
        return not (set(na) & ib) and not (set(nb) & ia)
    if lhs(a) & ib or lhs(b) & ia or lhs(a) & lhs(b): return False
    if ('++' in a or '--' in a or '++' in b or '--' in b) and ia & ib: return False
    if call(a) and call(b): return False
    if (call(a) and mem(b)) or (call(b) and mem(a)): return False
    return True


def if_edits(src, toks, s0, s1):
    eds = []
    po = s0 + 1; pc = match_close(toks, po, '(', ')')
    if pc < 0 or pc + 1 >= s1: return eds
    cond = text(src, toks, po + 1, pc)
    body0 = pc + 1
    if toks[body0].s != '{': return eds
    be = match_close(toks, body0, '{', '}')
    body = text(src, toks, body0, be + 1)
    has_else = be + 1 < s1 and toks[be + 1].s == 'else'
    inner = [x.s for x in toks[body0 + 1:be]]
    if not has_else and inner and inner[0] in ('return', 'break', 'continue') and inner.count(';') == 1:
        try:
            p = P(toks, po + 1, pc); root = p.expr()
            if p.p == pc and root.k == 'bin' and root.op == '||':
                A, B = root.c
                eds.append((toks[s0].a, toks[be].b,
                            f'if ({text(src, toks, A.i, A.j)}) {body}\nif ({text(src, toks, B.i, B.j)}) {body}'))
        except (ValueError, IndexError, RecursionError):
            pass
    if not has_else:
        try:
            p = P(toks, po + 1, pc); root = p.expr()
            if p.p == pc and root.k == 'bin' and root.op == '&&':
                A, B = root.c
                eds.append((toks[s0].a, toks[be].b,
                            f'if ({text(src, toks, A.i, A.j)}) {{\nif ({text(src, toks, B.i, B.j)}) {body}\n}}'))
        except (ValueError, IndexError, RecursionError):
            pass
    else:
        e0 = be + 2
        if e0 < s1 and toks[e0].s == '{':
            ee = match_close(toks, e0, '{', '}')
            if ee + 1 == s1:
                eb = text(src, toks, e0, ee + 1)
                eds.append((toks[s0].a, toks[s1 - 1].b, f'if (!({cond})) {eb} else {body}'))
    return eds


def for_to_while(src, toks, s0, s1):
    po = s0 + 1; pc = match_close(toks, po, '(', ')')
    semis = [k for k in range(po + 1, pc) if toks[k].s == ';' and depth(toks, po + 1, k) == 0]
    if len(semis) != 2 or toks[pc + 1].s != '{': return None
    be = match_close(toks, pc + 1, '{', '}')
    if be + 1 != s1: return None
    inner = [t.s for t in toks[pc + 2:be]]
    if 'continue' in inner: return None
    init = text(src, toks, po + 1, semis[0]); cond = text(src, toks, semis[0] + 1, semis[1]) or '1'
    step = text(src, toks, semis[1] + 1, pc); body = text(src, toks, pc + 2, be)
    if re.match(r'\s*(int|unsigned|u32|s32|short|u16|s16|char|u8|s8)\b', init): return None  # scoped declaration
    out = (init + ';\n' if init.strip() else '') + f'while ({cond}) {{\n{body}\n' + (step + ';\n' if step.strip() else '') + '}'
    return (toks[s0].a, toks[s1 - 1].b, out)


def all_edits(fn, void_fn=False):
    toks = tokenize(fn)
    sites, stmt_lists = scan(toks)
    eds = []
    for kind, (i, j, ctx) in sites:
        if j > i: eds += expr_edits(fn, toks, i, j, ctx)
    eds += stmt_edits(fn, toks, stmt_lists, void_fn)
    return eds


def mutate(fn, rng, void_fn=False):
    try:
        eds = all_edits(fn, void_fn)
    except (ValueError, IndexError, RecursionError):
        return None
    if not eds: return None
    a, b, rep = rng.choice(eds)
    return fn[:a] + rep + fn[b:]


# ---------------------------------------------------------------- function location and scoring
def find_unit_src(unit):
    c = glob.glob(f'{ROOT}/src/*/{unit}.cpp')
    if not c: raise SystemExit('no source for ' + unit)
    return c[0]


def split_fn(src, head_pos):
    j = src.index('{', head_pos); d = 0; k = j
    while True:
        if src[k] == '{': d += 1
        elif src[k] == '}':
            d -= 1
            if d == 0: break
        k += 1
    return src[:j + 1], src[j + 1:k], src[k:]


def locate(src, dem):
    """Character position of the definition head for a demangled name, or None if ambiguous."""
    m = re.search(r'(?:__thiscall|__cdecl|__stdcall|__fastcall)\s+(.+?)\((.*)\)( const)?$', dem)
    if not m: return None
    qual = m.group(1).strip(); params = m.group(2).strip()
    nparams = 0 if params in ('', 'void') else params.count(',') + 1
    name = qual.split('::')[-1]
    pat = re.compile(r'^[^\n;{}#/]*?\b' + re.escape(qual).replace(r'\ ', r'\s*') + r'\s*\(', re.M)
    cands = []
    for mm in pat.finditer(src):
        line_end = src.index('{', mm.start()) if '{' in src[mm.start():] else -1
        if line_end < 0: continue
        between = src[mm.end():line_end]
        if ';' in between: continue  # a declaration, not a definition
        ps = between[:between.rfind(')')]
        n = 0 if ps.strip() in ('', 'void') else ps.count(',') + 1
        if n == nparams: cands.append(mm.start())
    return cands[0] if len(cands) == 1 else None


def locate_inline(src, dem):
    """Definition of an inline member inside its class body (name followed by its body on the line)."""
    m = re.search(r'(?:__thiscall|__cdecl|__stdcall|__fastcall)\s+(.+?)\((.*)\)( const)?$', dem)
    if not m: return None
    qual = m.group(1).strip(); params = m.group(2).strip()
    nparams = 0 if params in ('', 'void') else params.count(',') + 1
    parts = qual.split('::'); name = parts[-1]; cls = parts[-2].split('<')[0] if len(parts) > 1 else None
    hits = []
    if cls:
        for cm in re.finditer(r'^(?:class|struct)\s+' + re.escape(cls) + r'\b[^;{]*\{', src, re.M):
            ce = cm.end(); d = 1; k = ce
            while k < len(src) and d:
                if src[k] == '{': d += 1
                elif src[k] == '}': d -= 1
                k += 1
            body = src[ce:k]
            for mm in re.finditer(r'^[^\n;{}]*?\b' + re.escape(name) + r'\s*\(([^;{]*)\)[^;{]*\{', body, re.M):
                ps = mm.group(1); n = 0 if ps.strip() in ('', 'void') else ps.count(',') + 1
                if n == nparams: hits.append(ce + mm.start())
    # out-of-class inline definitions: 'inline T Cls::name(' or free 'inline T name('
    pat = re.compile(r'^(?:template\s*<[^>]*>\s*)?(?:__forceinline|inline)\b[^\n;{]*?\b' + re.escape(qual).replace(r'\ ', r'\s*') + r'\s*\(([^;{]*)\)[^;{]*\{', re.M)
    for mm in pat.finditer(src):
        ps = mm.group(1); n = 0 if ps.strip() in ('', 'void') else ps.count(',') + 1
        if n == nparams: hits.append(mm.start())
    return hits[0] if len(hits) == 1 else None


_X = None
def score_variant(path, sym, check, inc=()):
    global _X
    if _X is None: _X = check.XBE(os.path.join(ROOT, 'orig', 'DEFAULT.XBE'))
    try:
        o, _ = check.compile(path, 'vc71', check.DEFAULT_FLAGS, inc)
    except RuntimeError:
        return None
    for name, sec, off, size in o.functions():
        if name == sym:
            ok, nd, sz, full = check.compare(o, _X, name, sec, off, size, False)
            if ok: return 0
            va = int(check.syms()[name]['va'], 16)
            missing = sum(1 for b in _X.read(va + sz, max(0, full - sz)) if b != 0xCC)  # original code past our end
            return nd * 4 + missing * 4 + max(0, sz - full) * 4 + 1
    return None


def walker(args):
    unit, va, secs, seed, wid = args
    import check
    rng = random.Random(seed)
    row = next(r for r in csv.DictReader(open(f'{ROOT}/symbols/functions.csv')) if r['va'] == va)
    sym, dem = row['name'], row['demangled']
    path = find_unit_src(unit); usrc = open(path, encoding='latin-1').read()
    src, hdr = usrc, None
    pos = locate(src, dem)
    if pos is None:
        # header inline: edit a shadow copy of the header placed first on the include path
        for h in sorted(glob.glob(f'{ROOT}/include/*.h')):
            hs = open(h, encoding='latin-1').read()
            hp = locate_inline(hs, dem)
            if hp is not None: src, hdr, pos = hs, h, hp; break
    if pos is None:
        print(f'  {unit} {va}: cannot locate definition of {dem}', flush=True)
        return (wid, None, None, 0, 'cannot locate ' + dem)
    pre, fn, post = split_fn(src, pos)
    void_fn = re.match(r'\s*(static\s+|inline\s+|virtual\s+)*void\b', src[pos:pos + 80]) is not None
    tmpd = tempfile.mkdtemp(prefix='perm')
    vpath = os.path.join(tmpd, unit + '.cpp')
    incd = os.path.join(tmpd, 'inc'); os.makedirs(incd)
    if hdr: open(vpath, 'w', encoding='latin-1').write(usrc)
    def sc(body):
        if hdr:
            open(os.path.join(incd, os.path.basename(hdr)), 'w', encoding='latin-1').write(pre + body + post)
        else:
            open(vpath, 'w', encoding='latin-1').write(pre + body + post)
        return score_variant(vpath, sym, check, (incd, os.path.dirname(path)))
    cur = fn; cs = sc(cur)
    if cs is None:
        print(f'  {unit} {va}: unit does not compile as-is', flush=True)
        shutil.rmtree(tmpd); return (wid, None, None, 0, 'base does not compile')
    if wid == 0: print(f'  {unit} {va}: start score {cs}', flush=True)
    best, bs = cur, cs; t0 = time.time(); n = 0; seen = {cur}
    while time.time() - t0 < secs and bs:
        v = cur
        for _ in range(1 + (rng.random() < 0.35) + (rng.random() < 0.15)):
            w = mutate(v, rng, void_fn)
            if w: v = w
        if v in seen:
            if rng.random() < 0.2: cur, cs = fn, sc(fn) if False else (fn, cs)[1]
            continue
        seen.add(v)
        s = sc(v); n += 1
        if s is None: continue
        if s <= cs or rng.random() < 0.04:
            cur, cs = v, s
            if s < bs: best, bs = v, s
        if rng.random() < 0.01: cur, cs = best, bs  # restart from best now and then
    shutil.rmtree(tmpd, ignore_errors=True)
    return (wid, best, bs, n, os.path.basename(hdr) if hdr else 'ok')


def run_one(unit, va, secs, workers, seed, outdir):
    os.makedirs(outdir, exist_ok=True)
    import check
    row = next(r for r in csv.DictReader(open(f'{ROOT}/symbols/functions.csv')) if r['va'] == va)
    t0 = time.time()
    with mp.Pool(workers) as pool:
        res = pool.map(walker, [(unit, va, secs, seed + k * 7919, k) for k in range(workers)])
    res = [r for r in res if r[1] is not None]
    if not res:
        print(f'{unit} {va}: skipped ({[r for r in res]})', flush=True); return None
    wid, best, bs, _, where = min(res, key=lambda r: r[2])
    tries = sum(r[3] for r in res)
    tag = f'{outdir}/{unit}_{va}'
    open(tag + '.best.cpp', 'w', encoding='latin-1').write(best)
    open(tag + '.txt', 'w').write(f'{unit} {va} score {bs} tries {tries} secs {int(time.time() - t0)} in {where}\n{row["demangled"]}\n')
    print(f'{unit} {va} best {bs} ({"MATCH" if bs == 0 else "diff"}) tries {tries}', flush=True)
    return bs


def make_list(maxscore):
    rows = [r for r in csv.DictReader(open(f'{ROOT}/status/status.csv')) if r['status'] == 'DIFF' and int(r['ndiff']) <= maxscore]
    rows.sort(key=lambda r: int(r['ndiff']))
    os.makedirs(f'{ROOT}/build/perm', exist_ok=True)
    with open(f'{ROOT}/build/perm/targets.txt', 'w') as f:
        for r in rows: f.write(f"{r['unit']} {r['va']} {r['ndiff']}\n")
    print(len(rows), 'targets -> build/perm/targets.txt')


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('mode'); ap.add_argument('a', nargs='?'); ap.add_argument('b', nargs='?')
    ap.add_argument('--secs', type=int, default=600); ap.add_argument('--secs-per', type=int, default=600)
    ap.add_argument('--workers', type=int, default=max(1, (os.cpu_count() or 2) - 1))
    ap.add_argument('--seed', type=int, default=int(time.time())); ap.add_argument('--max-score', type=int, default=200)
    ap.add_argument('--out', default=f'{ROOT}/build/perm')
    a = ap.parse_args()
    if a.mode == 'list':
        make_list(a.max_score)
    elif a.mode == 'one':
        run_one(a.a, a.b.lower().zfill(8), a.secs, a.workers, a.seed, a.out)
    elif a.mode == 'batch':
        for line in open(a.a):
            p = line.split()
            if len(p) < 2: continue
            unit, va = p[0], p[1]
            if os.path.exists(f'{a.out}/{unit}_{va}.txt'): continue  # resume
            try:
                run_one(unit, va, a.secs_per, a.workers, a.seed, a.out)
            except Exception as e:
                print(unit, va, 'error', e, flush=True)
                os.makedirs(a.out, exist_ok=True)
                open(f'{a.out}/{unit}_{va}.txt', 'w').write(f'{unit} {va} error {e}\n')
