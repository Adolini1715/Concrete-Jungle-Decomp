#!/usr/bin/env python3
"""layout.py CLASS [--derived] [--min N] [--gaps] [--csv FILE]: recover a class's field layout from the code.

Disassembles every thiscall method of CLASS (and with --derived, of classes whose base chain reaches
CLASS) and follows the registers that hold `this` (ecx at entry; copies into other registers; cleared
when overwritten or, for eax/ecx/edx, at calls). Every [this+off] operand becomes evidence:

  offset, access size, how it is used (int / float (SSE or x87) / byte / word / address taken / call
  this=field / pointer loaded then used as `this` of another class's method), reads/writes, and the
  functions that touch it.

Output: one row per offset with the inferred type and the field already named at that offset in
include/<CLASS>.h (`// 0x..` comments), so new fields and wrong guesses stand out. --gaps lists only
offsets the header does not name yet. Linear scan, no control flow: good evidence, not proof."""
import csv, json, os, re, sys, collections
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE); sys.path.insert(0, HERE)
from xbe import XBE
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_MEM, X86_OP_REG, X86_REG_INVALID

DB = json.load(open(os.path.join(ROOT, 'symbols', 'classes.json')))
BYVA = {}
for cn, c in DB.items():
    for m in c['methods']:
        if m.get('va'): BYVA[int(m['va'], 16)] = (cn, m)
X = XBE(os.path.join(ROOT, 'orig', 'DEFAULT.XBE'))
MD = Cs(CS_ARCH_X86, CS_MODE_32); MD.detail = True

R32 = {'eax', 'ebx', 'ecx', 'edx', 'esi', 'edi', 'ebp'}
VOLATILE = {'eax', 'ecx', 'edx'}
SUB = {'al': 'eax', 'ah': 'eax', 'ax': 'eax', 'bl': 'ebx', 'bh': 'ebx', 'bx': 'ebx', 'cl': 'ecx', 'ch': 'ecx',
       'cx': 'ecx', 'dl': 'edx', 'dh': 'edx', 'dx': 'edx', 'si': 'esi', 'di': 'edi', 'bp': 'ebp'}
FLOATOPS = re.compile(r'^(movss|addss|subss|mulss|divss|comiss|ucomiss|minss|maxss|sqrtss|cvtss2si|cvttss2si|fld|fst|fstp|fadd|fsub|fsubr|fmul|fdiv|fdivr|fcom|fcomp|fild|fist|fistp)$')
VECOPS = re.compile(r'^(movaps|movups|movlps|movhps|movq)$')


def derived_of(cls):
    out = []
    for cn, c in DB.items():
        b, seen = c.get('base'), set()
        while b and b not in seen:
            if b == cls: out.append(cn); break
            seen.add(b); b = DB.get(b, {}).get('base')
    return out


def header_fields(cls):
    path = os.path.join(ROOT, 'include', cls + '.h')
    names = {}
    if not os.path.exists(path): return names
    for line in open(path, errors='replace'):
        m = re.search(r'^\s*(.+?)\s+([A-Za-z_]\w*)(\[[^\]]*\])?\s*(?::\s*\d+)?;\s*//\s*(?:\+)?(0x[0-9a-fA-F]+)', line)
        if m:
            off = int(m.group(4), 16)
            names.setdefault(off, f"{m.group(1).strip()} {m.group(2)}{m.group(3) or ''}")
    return names


def regname(insn, r):
    n = insn.reg_name(r)
    return SUB.get(n, n)


def scan(va, size, ev, fname, callee_types):
    this = {'ecx'}
    loaded = {}           # reg -> offset it was loaded from ([this+off])
    try:
        code = X.read(va, size)
    except Exception:
        return
    for i in MD.disasm(code, va):
        if i.mnemonic == 'int3': break
        ops = i.operands
        # memory operands based on a this-register
        for k, op in enumerate(ops):
            if op.type == X86_OP_MEM and op.mem.base != X86_REG_INVALID and op.mem.index == X86_REG_INVALID:
                b = regname(i, op.mem.base)
                if b in this and 0 <= op.mem.disp < 0x10000:
                    off = op.mem.disp
                    e = ev[off]
                    e['funcs'].add(fname)
                    if i.mnemonic == 'lea':
                        e['use']['addr'] += 1
                    elif FLOATOPS.match(i.mnemonic):
                        e['use']['float'] += 1
                    elif VECOPS.match(i.mnemonic):
                        e['use']['vec'] += 1
                    else:
                        e['use'][{1: 'byte', 2: 'word', 4: 'dword', 8: 'qword', 16: 'vec'}.get(op.size, 'm%d' % op.size)] += 1
                    write = k == 0 and len(ops) >= 1 and i.mnemonic not in ('cmp', 'test', 'push', 'comiss', 'ucomiss', 'fld', 'fild', 'fcom', 'fcomp') and not i.mnemonic.startswith('j')
                    if i.mnemonic in ('fst', 'fstp', 'fist', 'fistp'): write = True
                    e['w' if write else 'r'] += 1
                    if i.mnemonic in ('and', 'or', 'xor', 'test') and len(ops) == 2 and ops[1].type != X86_OP_REG:
                        e['use']['bitflags'] += 1
        # register flow
        if i.mnemonic == 'call':
            tgt = ops[0].imm if ops and ops[0].type != X86_OP_MEM and ops[0].type != X86_OP_REG else None
            if tgt is not None and 'ecx' in loaded and tgt in BYVA:
                cn, m = BYVA[tgt]
                if m['cc'] == '__thiscall': callee_types[loaded['ecx']][('ptr' if loaded['ecx'][1] == 'load' else 'emb', cn)] += 1
            for r in VOLATILE: this.discard(r); loaded.pop(r, None)
            continue
        if len(ops) >= 1 and ops[0].type == X86_OP_REG:
            dst = regname(i, ops[0].reg)
            if dst in R32 and i.mnemonic not in ('cmp', 'test', 'push'):
                src = ops[1] if len(ops) > 1 else None
                new_this, new_loaded = False, None
                if i.mnemonic == 'mov' and src is not None and src.type == X86_OP_REG and regname(i, src.reg) in this:
                    new_this = True
                elif i.mnemonic == 'mov' and src is not None and src.type == X86_OP_MEM and src.mem.base != X86_REG_INVALID \
                        and regname(i, src.mem.base) in this and src.mem.index == X86_REG_INVALID and src.size == 4:
                    new_loaded = (src.mem.disp, 'load')
                elif i.mnemonic == 'lea' and src is not None and src.mem.base != X86_REG_INVALID \
                        and regname(i, src.mem.base) in this and src.mem.index == X86_REG_INVALID:
                    new_loaded = (src.mem.disp, 'lea')
                if i.mnemonic == 'mov' and src is not None and src.type == X86_OP_REG and regname(i, src.reg) in loaded:
                    new_loaded = loaded[regname(i, src.reg)]
                this.discard(dst); loaded.pop(dst, None)
                if new_this: this.add(dst)
                if new_loaded: loaded[dst] = new_loaded
        if i.mnemonic in ('ret',):
            pass


def infer(e, ct):
    u = e['use']
    if u['float'] and u['float'] >= sum(u.values()) / 2: return 'float'
    if ct:
        (kind, cn), _ = ct.most_common(1)[0]
        return (cn + ' *') if kind == 'ptr' else cn + ' (embedded)'
    if u['float'] and u['float'] >= sum(u.values()) / 2: return 'float'
    if u['vec']: return 'EXVector? (16-byte)'
    if u['addr'] and not (u['dword'] or u['byte'] or u['word'] or u['float']): return 'embedded struct/array (address taken)'
    if u['bitflags']: return 'flags'
    if u['byte'] and not u['dword']: return 's8/u8'
    if u['word'] and not u['dword']: return 's16/u16'
    if u['dword']: return 'int/ptr'
    return '?'


def main():
    a = sys.argv[1:]
    if not a: sys.exit(__doc__)
    cls = a[0]; derived = '--derived' in a; gaps = '--gaps' in a
    mn = int(a[a.index('--min') + 1]) if '--min' in a else 1
    out_csv = a[a.index('--csv') + 1] if '--csv' in a else None
    classes = [cls] + (derived_of(cls) if derived else [])
    ev = collections.defaultdict(lambda: {'use': collections.Counter(), 'r': 0, 'w': 0, 'funcs': set()})
    callee_types = collections.defaultdict(collections.Counter)
    nf = 0
    for cn in classes:
        for m in DB.get(cn, {}).get('methods', []):
            if m['cc'] != '__thiscall' or m['static'] or not m.get('va'): continue
            scan(int(m['va'], 16), int(m['size']), ev, f"{cn}::{m['name']}", callee_types)
            nf += 1
    names = {}
    b, seen = cls, set()
    while b and b not in seen:          # the class and its bases (base fields sit at the same offsets)
        for k, v in header_fields(b).items(): names.setdefault(k, v if b == cls else f"{v}  [{b}]")
        seen.add(b); b = DB.get(b, {}).get('base')
    size = DB.get(cls, {}).get('size')
    print(f"; {cls}: {nf} methods scanned ({', '.join(classes[1:]) or 'no derived'}), sizeof {hex(size) if size else '?'}, "
          f"{len(names)} offsets named in include/{cls}.h")
    rows = []
    for off in sorted(ev):
        e = ev[off]
        if len(e['funcs']) < mn: continue
        if size and off >= size: continue
        name = names.get(off, '')
        if gaps and name: continue
        ct = callee_types.get((off, 'load')) or callee_types.get((off, 'lea'))
        typ = infer(e, ct)
        if typ == 'float':                  # x, y, z(, w) runs touched by the same functions: a vector
            nb = [ev.get(off + 4), ev.get(off + 8)]
            if all(n and n['use']['float'] for n in nb) and len(e['funcs'] & nb[0]['funcs'] & nb[1]['funcs']) >= max(1, len(e['funcs']) // 2) \
                    and not (ev.get(off - 4) and ev[off - 4]['use']['float'] and ev.get(off + 4) and len(ev[off - 4]['funcs'] & e['funcs']) >= max(1, len(e['funcs']) // 2)):
                typ = 'float (EXVector x?)'
        uses = ' '.join(f"{k}:{v}" for k, v in e['use'].most_common())
        fl = sorted(e['funcs'])
        rows.append((off, typ, e['r'], e['w'], len(fl), uses, name, fl))
        try:
            print(f"  0x{off:04x}  {typ:34s} r{e['r']:<3d} w{e['w']:<3d} fn{len(fl):<3d} {uses:28s} {name or '-'}")
            print(f"          {', '.join(f.split('::')[-1] for f in fl[:6])}{' ...' if len(fl) > 6 else ''}")
        except BrokenPipeError:
            sys.exit(0)
    if out_csv:
        with open(out_csv, 'w', newline='') as f:
            w = csv.writer(f); w.writerow(['offset', 'type', 'reads', 'writes', 'nfuncs', 'uses', 'header_name', 'funcs'])
            for r in rows: w.writerow([hex(r[0]), r[1], r[2], r[3], r[4], r[5], r[6], ' '.join(r[7])])


if __name__ == '__main__':
    main()
