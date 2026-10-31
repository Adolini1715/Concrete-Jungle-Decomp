#!/usr/bin/env python3
"""ghidra_types.py: export what the decomp knows about types for the typed Ghidra pass.

Writes build/ghidra_types.txt, read by tools/ghidra/PcjTypes.java:

  C <class> <size>                                  a structure (size from the runtime class / classes.json /
                                                    the last named header field)
  F <class> <off> <count> <kind> <type> <name>      a field: kind b = builtin (char uchar short ushort int uint
                                                    float double), p = pointer to <type> (a class or 'void'),
                                                    s = embedded struct <type>; count > 1 for arrays
  M <va> <class>                                    a thiscall method: `this` is <class> *
  G <va> <kind> <type> <name>                       a global's type (same kinds; s/p need the class exported)

Fields come from the `// 0x..` offset comments in include/<Class>.h, flattened over the base chain
(base fields first, derived fields win). Field types the exporter cannot map become byte arrays sized
up to the next named field, so the offsets still line up."""
import csv, json, os, re, sys, collections
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE)
DB = json.load(open(os.path.join(ROOT, 'symbols', 'classes.json')))
INC = os.path.join(ROOT, 'include')

BUILTIN = {'s8': ('char', 1), 'char': ('char', 1), 'signed char': ('char', 1), 'u8': ('uchar', 1), 'unsigned char': ('uchar', 1),
           'bool': ('uchar', 1), 's16': ('short', 2), 'short': ('short', 2), 'signed short': ('short', 2),
           'u16': ('ushort', 2), 'unsigned short': ('ushort', 2), 'int': ('int', 4), 's32': ('int', 4), 'long': ('int', 4),
           'signed int': ('int', 4), 'u32': ('uint', 4), 'unsigned int': ('uint', 4), 'unsigned': ('uint', 4),
           'unsigned long': ('uint', 4), 'float': ('float', 4), 'double': ('double', 8), 'f32': ('float', 4)}

FIELD = re.compile(r'^\s*(?:mutable\s+)?(.+?[\s\*&])\s*([A-Za-z_]\w*)\s*((?:\[[^\]]*\])*)\s*(?::\s*\d+)?;\s*//\s*\+?(0x[0-9a-fA-F]+)')
UNION = re.compile(r'^\s*union\s*\{[^}]*\}\s*([A-Za-z_]\w*)\s*;\s*//\s*\+?(0x[0-9a-fA-F]+)')
LOOSE = re.compile(r'^\s*(?:mutable\s+)?([\w:<>, ]*?[\w>][\s\*&]+)([A-Za-z_]\w*)\s*((?:\[[^\]]*\])*)\s*;\s*$')
PROBE_SIZE = {}
MULTI = re.compile(r'^\s*([\w:<> ]*?\w)\s+([A-Za-z_]\w*(?:\s*,\s*[A-Za-z_]\w*)+)\s*;\s*$')
CLASSDECL = re.compile(r'^\s*(?:class|struct)\s+(\w+)\s*(?::\s*(?:public|protected|private)?\s*([\w:]+))?\s*\{')


def clean(t):
    t = re.sub(r'\b(const|volatile|class|struct|enum|union)\b', '', t)
    return re.sub(r'\s+', ' ', t).strip()


def header_classes():
    """class -> (base, [(off, type, name, arraytext)]) from every header (a header may hold several classes)."""
    out = {}
    for fn in sorted(os.listdir(INC)):
        if not fn.endswith('.h'): continue
        cur = None; depth = 0; nsd = 0
        for line in open(os.path.join(INC, fn), errors='replace'):
            if depth == 0 and re.match(r'^\s*namespace\s+\w+\s*\{', line): nsd += 1; continue
            if depth == 0 and nsd and line.strip().startswith('}'): nsd -= 1; continue
            m = CLASSDECL.match(line)
            if m and depth == 0:
                cur = m.group(1); out.setdefault(cur, [m.group(2), []])
            depth += line.count('{') - line.count('}')
            if cur is None: continue
            if depth <= 0 and '}' in line: cur = None; continue
            mu = UNION.match(line)
            if mu:
                out[cur][1].append((int(mu.group(2), 16), 'u32', mu.group(1), '')); continue
            mf = FIELD.match(line)
            if mf and '(' not in mf.group(1):
                out[cur][1].append((int(mf.group(4), 16), mf.group(1), mf.group(2), mf.group(3)))
                continue
            mm = MULTI.match(line.split('//')[0])
            if mm and depth == 1:
                for nm_ in re.split(r'\s*,\s*', mm.group(2)): out[cur][1].append((None, mm.group(1), nm_, ''))
                continue
            ml = LOOSE.match(line.split('//')[0])
            if ml and depth == 1 and '(' not in line.split('//')[0]:
                out[cur][1].append((None, ml.group(1), ml.group(2), ml.group(3)))   # offset from the compiler probe
    probe = os.path.join(ROOT, 'build', 'field_offsets.json')
    pr = json.load(open(probe)) if os.path.exists(probe) else {}
    for c, (base, fields) in out.items():
        po = pr.get(c, {}).get('fields', {})
        merged = {}
        for off, typ, name, arr in fields:
            o = po.get(name, off)          # the compiler wins over a stale comment
            if o is not None: merged[name] = (o, typ, name, arr)
        out[c][1] = sorted(merged.values())
        if pr.get(c, {}).get('size'): PROBE_SIZE[c] = pr[c]['size']
    return out


def main():
    hdr = header_classes()
    def base_of(c):
        b = (hdr.get(c) or [None])[0]
        if b: return b.split('::')[-1]
        d = DB.get(c, {})
        if d.get('base'): return d['base']
        for kind, b in d.get('base_evidence') or []:
            if kind == 'ctor': return b
        return None
    names = set(hdr) | set(DB)
    sizes = {}
    for c in names:
        s = DB.get(c, {}).get('size') or PROBE_SIZE.get(c)
        if s: sizes[c] = s
    # flatten fields over the base chain
    flat = {}
    for c in hdr:
        chain, b, seen = [], c, set()
        while b and b not in seen:
            chain.append(b); seen.add(b); b = base_of(b)
        fields = {}
        for k in reversed(chain):
            for f in (hdr.get(k) or [None, []])[1]: fields[f[0]] = f
        flat[c] = [fields[o] for o in sorted(fields)]
        if c not in sizes and flat[c]:
            sizes[c] = flat[c][-1][0] + 4
    lines = []
    for c in sorted(flat):
        if c not in sizes or not flat[c]: continue
        lines.append(f'C {c} {sizes[c]}')
    exported = {l.split()[1] for l in lines}
    nf = 0
    for c in sorted(flat):
        if c not in exported: continue
        fl = flat[c]
        for i, (off, typ, name, arr) in enumerate(fl):
            nxt = fl[i + 1][0] if i + 1 < len(fl) else sizes[c]
            room = nxt - off
            if room <= 0 or off >= sizes[c]: continue
            count = 1
            for n in re.findall(r'\[([^\]]*)\]', arr):
                try: count *= int(eval(n, {}, {}))
                except Exception: count = 0
            t = clean(typ)
            if t.endswith('*'):
                tgt = clean(t.rstrip('*').strip()).split('::')[-1]
                kind, ty, el = 'p', (tgt if tgt in exported else 'void'), 4
            elif t in BUILTIN:
                kind, (ty, el) = 'b', BUILTIN[t]
            elif t.split('::')[-1] in exported and sizes.get(t.split('::')[-1]):
                kind, ty, el = 's', t.split('::')[-1], sizes[t.split('::')[-1]]
            else:
                kind, ty, el = 'b', 'uchar', 1; count = room
            if count == 0: count = max(1, room // el)
            if el * count > room: count = max(1, room // el)
            if el > room: kind, ty, el, count = 'b', 'uchar', 1, room
            lines.append(f'F {c} {off} {count} {kind} {ty} {name}'); nf += 1
    nm = 0
    for c, d in DB.items():
        if c not in exported: continue
        for m in d['methods']:
            if m.get('va') and m['cc'] == '__thiscall' and not m['static']:
                lines.append(f"M {m['va']} {c}"); nm += 1
    ng = 0
    for r in csv.DictReader(open(os.path.join(ROOT, 'symbols', 'symbols.csv'))):
        if 'f' in r['flags'] or r['origin'] not in ('game', 'enginex'): continue
        dem = re.sub(r'\*(?=[A-Za-z_])', '* ', r['demangled'] or '')
        m = re.match(r'^(?:(?:public|protected|private): )?(?:static )?(.+?)\s+([\w:<>,~` ]+)$', dem)
        if not m or '(' in dem: continue
        t = clean(m.group(1)); nm2 = m.group(2).split('::')[-1]
        if t.endswith('*'):
            stars = t.count('*')
            tgt = clean(t.replace('*', '')).split('::')[-1]
            n = max(1, int(r['size'] or 4) // 4) if stars == 2 else 1   # T *g[N] demangles as T * *
            kind = 'pp' if stars == 2 else 'p'
            lines.append(f"G {r['va']} {kind} {tgt if tgt in exported else 'void'} {nm2} {n}"); ng += 1
        elif t in BUILTIN:
            lines.append(f"G {r['va']} b {BUILTIN[t][0]} {nm2}"); ng += 1
        elif t.split('::')[-1] in exported:
            lines.append(f"G {r['va']} s {t.split('::')[-1]} {nm2}"); ng += 1
    out = os.path.join(ROOT, 'build', 'ghidra_types.txt')
    open(out, 'w').write('\n'.join(lines) + '\n')
    print(f'{len(exported)} structs, {nf} fields, {nm} methods, {ng} globals -> {out}')


if __name__ == '__main__':
    main()
