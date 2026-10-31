#!/usr/bin/env python3
"""Build symbols/classes.json from the map: per class, its methods (access, virtual, static, const,
signature, address), static data members, vtable slot order, and an inferred base class.
Base evidence: (1) the first call in a constructor to another class's constructor with this=ecx,
(2) the longest vtable whose slots are a prefix-compatible subset of this class's slots."""
import csv, json, os, re, struct, sys
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE); sys.path.insert(0, HERE)
from xbe import XBE
from capstone import Cs, CS_ARCH_X86, CS_MODE_32

CC = re.compile(r'(?:^| )(__thiscall|__cdecl|__stdcall|__fastcall) ')

def split_top(s, sep):
    """Split on sep at angle/paren depth 0."""
    out, d, cur, i = [], 0, '', 0
    while i < len(s):
        c = s[i]
        if c in '<(': d += 1
        elif c in '>)': d -= 1
        if d == 0 and s.startswith(sep, i):
            out.append(cur); cur = ''; i += len(sep); continue
        cur += c; i += 1
    out.append(cur); return out

def qual_split(q):
    parts = split_top(q, '::')
    return '::'.join(parts[:-1]), parts[-1]

def parse(dem):
    """Return dict or None for a demangled function/data symbol."""
    d = {'access': '', 'virtual': False, 'static': False}
    s = dem
    m = re.match(r'(public|protected|private): ', s)
    if m: d['access'] = m.group(1); s = s[m.end():]
    if s.startswith('virtual '): d['virtual'] = True; s = s[8:]
    if s.startswith('static '): d['static'] = True; s = s[7:]
    m = CC.search(s)
    if m:  # function
        d['kind'] = 'func'; d['cc'] = m.group(1)
        d['ret'] = s[:m.start()].strip()
        rest = s[m.end():]
        # name ends at the first depth-0 '(' that is not part of 'operator()'
        dep = 0; i = 0
        while i < len(rest):
            c = rest[i]
            if c == '<': dep += 1
            elif c == '>': dep -= 1
            elif c == '(' and dep == 0:
                if rest[:i].endswith('operator') and rest[i:i + 2] == '()': i += 2; continue
                break
            i += 1
        q = rest[:i]; tail = rest[i:]
        # params: matching paren
        dep = 0
        for j, c in enumerate(tail):
            if c == '(': dep += 1
            elif c == ')':
                dep -= 1
                if dep == 0: break
        d['params'] = tail[1:j]; d['cv'] = tail[j + 1:].strip()
        d['scope'], d['name'] = qual_split(q)
        return d
    # data: "<type> <qualified name>"
    d['kind'] = 'data'
    parts = split_top(s, ' ')
    q = parts[-1]; d['type'] = ' '.join(parts[:-1])
    d['scope'], d['name'] = qual_split(q)
    return d

def main():
    x = XBE(os.path.join(ROOT, 'orig', 'DEFAULT.XBE'))
    rows = [r for r in csv.DictReader(open(os.path.join(ROOT, 'symbols', 'symbols.csv'))) if not r['origin'].startswith('xdk')]
    byva = {}
    for r in rows: byva.setdefault(int(r['va'], 16), []).append(r)
    classes = {}
    def C(n):
        return classes.setdefault(n, {'methods': [], 'data': [], 'vtable': None, 'base': None, 'base_evidence': [], 'objs': set()})
    for r in rows:
        dem = r['demangled']
        if dem == r['name'] or 'vftable' in dem or '`' in dem and 'dtor' not in dem: continue
        p = parse(dem)
        if not p or not p['scope']: continue
        if '`' in p['scope']: continue
        c = C(p['scope']); c['objs'].add(r['obj'])
        p.update(va=r['va'], size=int(r['size']), mangled=r['name'], obj=r['obj'], flags=r['flags'], origin=r['origin'])
        (c['methods'] if p['kind'] == 'func' else c['data']).append(p)
    # vtables
    PURECALL = next(int(r['va'], 16) for r in csv.DictReader(open(os.path.join(ROOT, 'symbols', 'symbols.csv'))) if r['name'] == '__purecall')
    for r in rows:
        if not r['name'].startswith('??_7'): continue
        cname = r['demangled'].replace('const ', '', 1).split('::`vftable')[0]
        va = int(r['va'], 16); n = int(r['size']) // 4
        slots = []
        for k in range(n):
            t = struct.unpack_from('<I', x.read(va + 4 * k, 4))[0]
            if t == PURECALL: slots.append('__purecall'); continue
            if t not in byva:
                if k == 0: slots.append(None); continue
                break
            names = [s['name'] for s in byva[t] if s['flags'].startswith('f')]
            if not names: break
            slots.append(names[0])
        C(cname)['vtable'] = {'va': r['va'], 'slots': slots}
    # base from constructors: first call to a ??0 of another class
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    VFTABLES = {int(r['va'], 16) for r in rows if r['name'].startswith('??_7')}
    for cname, c in classes.items():
        for m in c['methods']:
            if not m['mangled'].startswith('??0') or m['size'] > 4096: continue
            va = int(m['va'], 16)
            # base constructors run before the class stores its own vptr; member constructors run after
            vt_stored = False
            for ins in md.disasm(x.read(va, min(m['size'], 256)), va):
                if ins.mnemonic == 'mov' and ins.op_str.startswith('dword ptr [') and ', 0x' in ins.op_str:
                    if int(ins.op_str.split(', ')[-1], 16) in VFTABLES: vt_stored = True
                if ins.mnemonic == 'call' and vt_stored: break
                if ins.mnemonic == 'call' and ins.op_str.startswith('0x'):
                    t = int(ins.op_str, 16)
                    for s in byva.get(t, []):
                        if s['name'].startswith('??0'):
                            b = parse(s['demangled'])
                            if b and b['scope'] and b['scope'] != cname and b['scope'] not in cname.split('::'):
                                c['base_evidence'].append(('ctor', b['scope']))
                    break
                if ins.mnemonic in ('ret',): break
    # base from vtables: candidate whose slot list shares the most positions (and is shorter or equal)
    vt = {n: c['vtable']['slots'] for n, c in classes.items() if c['vtable']}
    for n, s in vt.items():
        best = None
        for m, t in vt.items():
            if m == n or len(t) > len(s) or len(t) < 2: continue
            same = sum(1 for a, b in zip(s, t) if a == b and a)
            # need the base's own destructor-or-most slots to be inherited; folded one-liners pollute this
            if same * 2 < len(t): continue
            if same and (best is None or same > best[1] or (same == best[1] and len(t) > len(vt[best[0]]))): best = (m, same)
        if best: classes[n]['base_evidence'].append(('vtable', best[0]))
    # class sizes: (1) EXRuntimeClass records {base, name, size, create}; (2) new sites:
    # push 0 / push SIZE / call _EXAlloc ... then a call/jmp to ??0Class within a few instructions
    for r in rows:
        if r['name'].endswith('@@2UEXRuntimeClass@@B'):
            cn = parse(r['demangled'])['scope']
            base, name, size, fn = struct.unpack('<4I', x.read(int(r['va'], 16), 16))
            C(cn)['size'] = size; C(cn)['size_src'] = 'runtimeclass'
            if base:
                for sy in byva.get(base, []):
                    if sy['name'].endswith('@@2UEXRuntimeClass@@B'):
                        C(cn)['base_evidence'].insert(0, ('rtc', parse(sy['demangled'])['scope']))
    alloc = [int(r['va'], 16) for r in rows if r['name'] == '?_EXAlloc@@YAPAXII@Z']
    text = x.sec('.text'); code = x.read(text['va'], text['vsize'])
    if alloc:
        A = alloc[0]
        for ins_va in range(text['va'], text['va'] + len(code) - 5):
            o = ins_va - text['va']
            if code[o] != 0xe8: continue
            if ins_va + 5 + struct.unpack_from('<i', code, o + 1)[0] != A: continue
            # look back for push imm32 (68) / push imm8 (6a)
            if code[o - 7] == 0x6a and code[o - 5] == 0x68: size = struct.unpack_from('<I', code, o - 4)[0]
            elif code[o - 4] == 0x6a and code[o - 2] == 0x6a: size = code[o - 1]
            else: continue
            for ins in md.disasm(code[o + 5:o + 5 + 48], ins_va + 5):
                if ins.mnemonic in ('call', 'jmp') and ins.op_str.startswith('0x'):
                    for sy in byva.get(int(ins.op_str, 16), []):
                        if sy['name'].startswith('??0'):
                            cn = parse(sy['demangled'])['scope']
                            c = C(cn)
                            if 'size' not in c: c['size'] = size; c['size_src'] = 'new'
                            elif c['size'] != size and c.get('size_src') == 'new': c.setdefault('size_conflict', []).append(size)
                    break
    for n, c in classes.items():
        if c.get('size_src') == 'runtimeclass' and not any(k == 'rtc' for k, b in c['base_evidence']):
            c['base'] = None; c['objs'] = sorted(c['objs']); continue  # record says: root class
        if not any(k in ('rtc', 'ctor') for k, b in c['base_evidence']) and c['base_evidence']:
            c['base_guess'] = True
        ev = [b for k, b in c['base_evidence'] if k == 'rtc'] or [b for k, b in c['base_evidence'] if k == 'ctor'] or [b for k, b in c['base_evidence'] if k == 'vtable']
        c['base'] = ev[0] if ev else None
        c['objs'] = sorted(c['objs'])
    json.dump(classes, open(os.path.join(ROOT, 'symbols', 'classes.json'), 'w'), indent=1)
    nv = sum(1 for c in classes.values() if c['vtable']); nb = sum(1 for c in classes.values() if c['base'])
    ns = sum(1 for c in classes.values() if 'size' in c)
    print(f'{len(classes)} classes, {nv} with vtables, {nb} with an inferred base, {ns} with a known size')
if __name__ == '__main__': main()
