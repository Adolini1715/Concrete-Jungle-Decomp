#!/usr/bin/env python3
"""genhdr.py CLASS [...]: print a declaration skeleton for a class from symbols/classes.json.
New virtuals come out in vtable slot order (that order IS the layout); overrides of base virtuals
and non-virtual methods follow, grouped by access. Fields are not known from the map: they must
be recovered from the code (offsets) and the Aug 2004 assert strings (names)."""
import json, os, sys
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DB = json.load(open(os.path.join(ROOT, 'symbols', 'classes.json')))
BYMANGLED = {m['mangled']: (cn, m) for cn, c in DB.items() for m in c['methods']}

def tidy(t):
    t = t.replace('class ', '').replace('struct ', '').replace('union ', '').replace('enum ', '')
    t = t.replace(' *', '*').replace(' &', '&')
    return t

def decl(m, short):
    ps = '' if m['params'] == 'void' else tidy(m['params'])
    ret = tidy(m['ret'])
    pre = ('virtual ' if m['virtual'] else '') + ('static ' if m['static'] else '')
    cv = (' ' + m['cv']) if m['cv'] else ''
    if m['name'] in (short, '~' + short): ret = ''
    if 'deleting dtor' in m['name']: return None
    cc = '' if m['cc'] in ('__thiscall',) or (m['cc'] == '__cdecl' and m['static']) or m['cc'] == '__cdecl' else m['cc'] + ' '
    return f"{pre}{ret + ' ' if ret else ''}{cc}{m['name']}({ps}){cv};"

def gen(cn):
    if cn not in DB:
        near = [k for k in DB if cn.lower() in k.lower()][:8]
        return f'// {cn}: no methods or data under this name in the map (template argument only?). Similar: {near}'
    c = DB[cn]; short = cn.split('::')[-1]
    base = c['base']; bslots = (DB[base]['vtable'] or {}).get('slots', []) if base and base in DB and DB[base]['vtable'] else []
    out = [f"// {cn}: objs {', '.join(c['objs'])}" + (f"; vtable {c['vtable']['va']} ({len(c['vtable']['slots'])} slots)" if c['vtable'] else '')]
    out.append(f"class {short}" + (f" : public {base}" if base else '') + ' {' + ('  // base is a GUESS from vtable similarity' if c.get('base_guess') else ''))
    groups = {}
    done = set()
    if c['vtable']:
        for i, s in enumerate(c['vtable']['slots']):
            if i < len(bslots) and bslots[i] == s: continue  # inherited, not overridden
            if s == '__purecall': line = f'virtual ??? = 0;  // slot {i}: pure (or pure virtual dtor with a body)'; acc = 'public'
            elif s is None: line = f'virtual void vf{i}();  // slot {i}: pure/unknown'; acc = 'public'
            elif s in BYMANGLED and BYMANGLED[s][0] == cn:
                m = BYMANGLED[s][1]; done.add(s); line = decl(m, short); acc = m['access'] or 'public'
                if line is None: line = f'virtual ~{short}();'; done.update(x['mangled'] for x in c['methods'] if x['name'] == '~' + short)
                line += f'  // slot {i}' + (' (override)' if i < len(bslots) else '')
            else:
                owner = BYMANGLED[s][0] if s in BYMANGLED else '?'
                line = f'virtual void vf{i}();  // slot {i}: shares code with {owner} ({s[:40]})'; acc = 'public'
            groups.setdefault(acc, []).append(line)
    for m in sorted(c['methods'], key=lambda m: m['va']):
        if m['mangled'] in done: continue
        d = decl(m, short)
        if d is None: continue
        tag = ' inline' if 'i' in m['flags'] else ''
        groups.setdefault(m['access'] or 'public', []).append(d + f"  // {m['va']} {m['size']}b {m['obj']}{tag}")
    for dm in c['data']:
        groups.setdefault(dm['access'] or 'public', []).append(f"{'static ' if dm['static'] else ''}{tidy(dm['type'])} {dm['name']};  // {dm['va']}")
    for acc in ('public', 'protected', 'private'):
        if acc in groups:
            out.append(f'{acc}:'); out += ['    ' + l for l in groups[acc]]
    if 'size' in c: out.append(f"    // sizeof = {c['size']:#x} ({c['size_src']})")
    out.append('    // fields: recover offsets from code; names from proto_aug04 asserts')
    out.append('};')
    return '\n'.join(out)

if __name__ == '__main__':
    for cn in sys.argv[1:]: print(gen(cn)); print()
