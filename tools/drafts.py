"""drafts.py: turn raw Ghidra pseudocode (build/drafts-raw/out/<unit>/<va>.c) into readable drafts.

    python3 tools/drafts.py                 rebuild build/drafts/<unit>.c for every unit
    python3 tools/drafts.py UNIT [VA...]    print the drafts for one unit (optionally only some VAs)

Ghidra prints map labels sanitized (non-identifier characters become '_'); each such name is
replaced by its demangled form, and func_0x/DAT_ addresses that the map names are resolved too.
Only functions with no row in status/status.csv are drafted (still unwritten); a draft whose
function has since been written is dropped on the next rebuild.
"""
import csv, os, re, sys
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RAW = os.environ.get('PCJ_DRAFT_RAW', f'{ROOT}/build/drafts-raw/out')
OUT = os.environ.get('PCJ_DRAFT_OUT', f'{ROOT}/build/drafts')


def short(dem):
    # 'public: virtual void __thiscall Cls::Fn(int)' -> 'Cls::Fn'
    s = dem.strip()
    if s.endswith(')') and '(' in s:
        d = 0
        for i in range(len(s) - 1, -1, -1):
            if s[i] == ')': d += 1
            elif s[i] == '(':
                d -= 1
                if d == 0: break
        s = s[:i]
    for cc in ('__thiscall ', '__cdecl ', '__stdcall ', '__fastcall '):
        if cc in s: return s.split(cc, 1)[1].strip()
    return s.split()[-1] if s.split() else dem


def load():
    byname, byva, info = {}, {}, {}
    for r in csv.DictReader(open(f'{ROOT}/symbols/functions.csv')):
        dem = r['demangled'] or r['name']
        byname[re.sub(r'\W', '_', r['name'])] = short(dem)
        byva[int(r['va'], 16)] = short(dem)
        info[r['va']] = (int(r['size']), dem)
    for r in csv.DictReader(open(f'{ROOT}/symbols/symbols.csv')):
        va = r.get('va') or ''
        if not va: continue
        dem = r.get('demangled') or r['name']
        byname.setdefault(re.sub(r'\W', '_', r['name']), short(dem) if '(' in dem else dem.split()[-1])
        byva.setdefault(int(va, 16), short(dem) if '(' in dem else dem.split()[-1])
    return byname, byva, info


def clean(text, byname, byva):
    def nm(m):
        return byname.get(m.group(0), m.group(0))
    text = re.sub(r'\b[_A-Za-z]\w*_[A-Z]\w*\b|\b_+\w+\b', nm, text)
    def addr(m):
        n = byva.get(int(m.group(2), 16))
        return n if n else m.group(0)
    text = re.sub(r'\b(func_0x|DAT_|PTR_DAT_|LAB_)([0-9a-f]{8})\b', addr, text)
    # hash constants: 0x4d000110 -> 0x4d000110 /* HT_ActorState_FS_PanicStand */
    try:
        import sys
        sys.path.insert(0, os.path.join(ROOT, 'tools'))
        from hashnames import name as hname
        def hc(m):
            v = int(m.group(0), 16)
            if v >> 24 in (0, 0x80, 0xff) or v < 0x1000000: return m.group(0)
            n = hname(v)
            return f'{m.group(0)} /* {n} */' if n else m.group(0)
        text = re.sub(r'\b0x[0-9a-fA-F]{7,8}\b(?! /\*)', hc, text)
    except Exception:
        pass
    return text


def offsets(text):
    # struct fields the draft touches: *(type *)(param_1 + 0x2c) and param_1[0xb] forms
    offs = set(int(o, 16) for o in re.findall(r'\(\w+ \+ (0x[0-9a-f]+)\)', text))
    return ', '.join(hex(o) for o in sorted(offs) if o < 0x10000)


_X = _MD = None
def switch_orders(va, size):
    """Case order of each jump-table switch as laid out in the original (Ghidra sorts cases by value;
    VC emits case blocks in source order, so this is the order to write them in)."""
    global _X, _MD
    import struct, sys
    if _X is None:
        sys.path.insert(0, os.path.join(ROOT, 'tools'))
        from xbe import XBE
        from capstone import Cs, CS_ARCH_X86, CS_MODE_32
        _X = XBE(f'{ROOT}/orig/DEFAULT.XBE'); _MD = Cs(CS_ARCH_X86, CS_MODE_32)
    ins = list(_MD.disasm(_X.read(va, size), va))
    out = []
    for k, i in enumerate(ins):
        m = re.match(r'dword ptr \[(\w+)\*4 \+ (0x[0-9a-f]+)\]', i.op_str)
        if i.mnemonic != 'jmp' or not m: continue
        tbl = int(m.group(2), 16); reg = m.group(1)
        if not (va <= tbl < va + size): continue
        base, n = 0, None
        for p in ins[max(0, k - 8):k]:
            if p.mnemonic == 'cmp' and re.match(r'\w+, (0x[0-9a-f]+|\d+)$', p.op_str):
                try: n = int(p.op_str.split(',')[1], 0) + 1
                except ValueError: pass
            mm = re.match(r'(?:lea \w+, \[\w+ - (0x[0-9a-f]+|\d+)\])', p.mnemonic + ' ' + p.op_str)
            if mm: base = int(mm.group(1), 0)
            if p.mnemonic == 'dec': base = 1
            if p.mnemonic == 'sub' and re.match(r'\w+, (0x[0-9a-f]+|\d+)$', p.op_str): base = int(p.op_str.split(', ')[1], 0)
            if p.mnemonic == 'add' and re.match(r'\w+, -?(0x[0-9a-f]+|\d+)$', p.op_str):
                v = int(p.op_str.split(', ')[1], 0) & 0xffffffff
                if v > 0x80000000: base = 0x100000000 - v
        if not n or n > 512: continue
        btbl = None
        for p in ins[max(0, k - 3):k]:
            mb = re.match(r'(\w+), byte ptr \[\w+ \+ (0x[0-9a-f]+)\]', p.op_str)
            if p.mnemonic == 'movzx' and mb and mb.group(1) == reg and va <= int(mb.group(2), 16) < va + size: btbl = int(mb.group(2), 16)
        if btbl is not None:
            idx = list(_X.read(btbl, n))
            tg = [struct.unpack('<I', _X.read(tbl + 4 * b, 4))[0] for b in idx]
        else:
            tg = [struct.unpack('<I', _X.read(tbl + 4 * q, 4))[0] for q in range(n)]
        default = max(set(tg), key=tg.count) if n > 4 else None
        groups = []
        for a in sorted(set(tg)):
            cs = [q + base for q, x in enumerate(tg) if x == a]
            groups.append(('/'.join(str(c) for c in cs) if a != default or len(cs) < 3 else 'default'))
        out.append(', '.join(groups))
    return out


def build():
    byname, byva, info = load()
    done = {r['va'] for r in csv.DictReader(open(f'{ROOT}/status/status.csv'))}
    os.makedirs(OUT, exist_ok=True)
    n = 0
    for unit in sorted(os.listdir(RAW)):
        parts = []
        for fn in sorted(os.listdir(f'{RAW}/{unit}')):
            va = fn[:-2]
            if va in done: continue
            size, dem = info.get(va, (0, '?'))
            body = open(f'{RAW}/{unit}/{fn}').read().split('\n', 1)[1]
            body = clean(body, byname, byva)
            hdr = f'// ==== {va} size {size}: {dem}\n'
            o = offsets(body)
            if o: hdr += f'// offsets: {o}\n'
            if 'switch' in body:
                for so in switch_orders(int(va, 16), size): hdr += f'// switch case order in the original: {so}\n'
            parts.append(hdr + body.strip() + '\n')
            n += 1
        if parts:
            open(f'{OUT}/{unit}.c', 'w').write('\n'.join(parts))
    print(f'{n} drafts in {OUT}')


if __name__ == '__main__':
    if len(sys.argv) == 1:
        build()
    else:
        unit = sys.argv[1]
        text = open(f'{OUT}/{unit}.c').read()
        if len(sys.argv) > 2:
            want = set(v.lower().zfill(8) for v in sys.argv[2:])
            blocks = re.split(r'(?=^// ==== )', text, flags=re.M)
            text = ''.join(b for b in blocks if b[8:16] in want)
        print(text)
