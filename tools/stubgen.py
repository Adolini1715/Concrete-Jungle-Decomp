#!/usr/bin/env python3
"""stubgen.py: triage and skeletons for functions that have no source yet.

    stubgen.py triage [UNIT ...]   per-unit counts of unwritten functions by kind (default: all units)
    stubgen.py UNIT                C++ for the unit's trivial unwritten functions, then a list of the
                                   real ones with sizes (read their drafts with tools/drafts.py UNIT VA)

Trivial shapes are recognised from the original bytes:
  EMPTY    ret / ret N                      -> {}
  GETTER   mov|movzx|movsx|fld reg,[ecx+X]  -> { return <field at X>; }
  ADDR     lea eax,[ecx+X]                  -> { return <field at X>; } (reference or pointer return)
  SET      mov [ecx+X],imm                  -> { <field at X> = imm; }
  CONST    xor eax,eax / mov eax|al,imm     -> { return imm; }
  FORWARD  jmp target                       -> { return target(...); }
Field names come from the '// 0xNN' comments in the class's header when present, otherwise
m_UnkNN. Inline copies ([fi]) only appear in an object when the unit's code calls the inline, so a
generated inline goes in the header and the calling code must use it rather than the raw field.
"""
import csv, glob, os, re, sys, collections
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE); sys.path.insert(0, HERE)
from xbe import XBE
from capstone import Cs, CS_ARCH_X86, CS_MODE_32

X = XBE(f'{ROOT}/orig/DEFAULT.XBE')
MD = Cs(CS_ARCH_X86, CS_MODE_32)
FNS = list(csv.DictReader(open(f'{ROOT}/symbols/functions.csv')))
BYVA = {int(r['va'], 16): r for r in FNS}
STATUS = {r['va']: r['status'] for r in csv.DictReader(open(f'{ROOT}/status/status.csv'))}
MATCHED_NAMES = {r['name'] for r in FNS if STATUS.get(r['va']) == 'MATCH'}


def unwritten(unit=None):
    out = []
    for r in FNS:
        if r['origin'] not in ('game', 'enginex') or r['va'] in STATUS: continue
        if unit and r['obj'] != unit + '.obj': continue
        out.append(r)
    return out


def insns(r):
    va = int(r['va'], 16); o = X.off(va)
    res = []
    for i in MD.disasm(X.d[o:o + int(r['size'])], va):
        res.append(i)
        if i.mnemonic in ('ret', 'jmp'): break
    return res


def shape(r):
    ins = insns(r)
    if not ins: return 'REAL', None
    m = [i.mnemonic for i in ins]
    if m[0] == 'ret': return 'EMPTY', ins
    if m == ['jmp']: return 'FORWARD', ins
    if len(m) == 2 and m[1] == 'ret':
        op = ins[0].op_str
        if m[0] == 'lea' and '[ecx' in op: return 'ADDR', ins
        if m[0] in ('mov', 'movzx', 'movsx') and re.match(r'(e?[a-d][xlh]|[sd]i), .*\[ecx', op): return 'GETTER', ins
        if m[0] == 'fld' and '[ecx' in op: return 'GETTER', ins
        if m[0] == 'mov' and re.match(r'(byte|word|dword) ptr \[ecx[^\]]*\], (0x[0-9a-f]+|\d+)$', op): return 'SET', ins
        if m[0] == 'xor' or (m[0] == 'mov' and re.match(r'(eax|al), (0x[0-9a-f]+|\d+)$', op)): return 'CONST', ins
    if len(m) <= 4 and m[-1] == 'ret': return 'TINY', ins
    return 'REAL', ins


_hdr_cache = {}
def field_name(cls, off):
    """Look for a member declared with a '// 0xNN' (or '+0xNN') comment inside 'class cls'."""
    key = (cls, off)
    if key in _hdr_cache: return _hdr_cache[key]
    name = None
    pat = re.compile(r'^(class|struct)\s+' + re.escape(cls.split('::')[-1]) + r'\b[^;]*$')
    for h in glob.glob(f'{ROOT}/include/*.h'):
        lines = open(h, encoding='latin-1').read().split('\n')
        for i, l in enumerate(lines):
            if not pat.match(l.strip()): continue
            for l2 in lines[i + 1:i + 400]:
                if l2.startswith('};'): break
                mm = re.search(r'(\w+)(\[[^\]]*\])?;\s*//\s*\+?0x0*([0-9a-f]+)\b', l2)
                if mm and int(mm.group(3), 16) == off:
                    name = mm.group(1); break
            if name: break
        if name: break
    _hdr_cache[key] = name or f'm_Unk{off:02x}'
    return _hdr_cache[key]


def cpp(r, kind, ins):
    dem = r['demangled'] or r['name']
    sig = re.sub(r'^(public|protected|private): ', '', dem)
    sig = re.sub(r'\b(virtual |static )', '', sig).replace('__thiscall ', '').replace('__cdecl ', '')
    m = re.search(r'([\w:<>~]+)::\w+\(', dem)
    cls = m.group(1) if m else ''
    if kind == 'EMPTY':
        body = '{}'
    elif kind in ('GETTER', 'ADDR'):
        mm = re.search(r'\[ecx(?: \+ (0x[0-9a-f]+|\d+))?\]', ins[0].op_str)
        off = int(mm.group(1), 0) if mm and mm.group(1) else 0
        body = f'{{ return {field_name(cls, off)}; }}'
    elif kind == 'SET':
        mm = re.search(r'\[ecx(?: \+ (0x[0-9a-f]+|\d+))?\], (\S+)$', ins[0].op_str)
        off = int(mm.group(1), 0) if mm.group(1) else 0
        body = f'{{ {field_name(cls, off)} = {mm.group(2)}; }}'
    elif kind == 'CONST':
        op = ins[0].op_str
        v = '0' if ins[0].mnemonic == 'xor' else op.split(', ')[1]
        body = f'{{ return {v}; }}'
    elif kind == 'FORWARD':
        t = int(ins[0].op_str, 16)
        tr = BYVA.get(t)
        tn = re.sub(r'\(.*', '', (tr['demangled'] or tr['name']).split(' ')[-1]) if tr else hex(t)
        body = f'{{ return {tn}(...); }}  // forwarder'
    else:
        body = '{ /* see draft */ }'
    return f'    {sig} {body}'


def triage(units):
    by = collections.defaultdict(collections.Counter)
    for r in unwritten():
        u = r['obj'][:-4]
        if units and u not in units: continue
        k, _ = shape(r)
        inl = 'i' in r['flags']
        if r['name'] in MATCHED_NAMES: k = 'KNOWN'   # same symbol already matches in another unit
        by[u][('inl-' if inl else '') + k] += 1
        by[u]['bytes'] += int(r['size'])
    rows = sorted(by.items(), key=lambda kv: -kv[1]['bytes'])
    for u, c in rows:
        real = c['REAL'] + c['inl-REAL']
        triv = sum(v for k, v in c.items() if k not in ('REAL', 'inl-REAL', 'bytes'))
        print(f'{u:32s} real {real:4d}  trivial {triv:4d}  bytes {c["bytes"]:7d}   ' +
              ' '.join(f'{k}={v}' for k, v in sorted(c.items()) if k != 'bytes'))


def unit_report(unit):
    real = []
    for r in sorted(unwritten(unit), key=lambda r: r['va']):
        kind, ins = shape(r)
        if kind in ('REAL', 'TINY'):
            real.append(r); continue
        print(f'// {r["va"]} [{r["flags"]}] {kind}')
        print(cpp(r, kind, ins))
    if real:
        print(f'\n// {len(real)} real functions (drafts: python3 tools/drafts.py {unit} VA)')
        for r in sorted(real, key=lambda r: int(r['size'])):
            print(f'//   {r["va"]} {int(r["size"]):6d} [{r["flags"]}] {r["demangled"] or r["name"]}')


if __name__ == '__main__':
    import signal; signal.signal(signal.SIGPIPE, signal.SIG_DFL)
    if len(sys.argv) < 2 or sys.argv[1] == 'triage':
        triage(set(sys.argv[2:]))
    else:
        unit_report(sys.argv[1])
