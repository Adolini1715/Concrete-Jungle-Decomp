#!/usr/bin/env python3
"""offsets_probe.py: ask the real compiler where every data member of every header class lives.

  offsets_probe.py [HEADER.h ...]     probe all headers (or the given ones) -> build/field_offsets.json
  offsets_probe.py --lint             compare with the `// 0x..` comments in the headers and list mismatches

For each header a scratch TU includes it (with private/protected opened up) and emits
`extern "C" const int pcj_probe_K[] = { sizeof(C), offsetof(C, m1), ... }` per class; the object's .rdata
gives the numbers. Members the parser picks up wrongly just fail to compile and are dropped (the TU is
retried without the lines the compiler rejects). The result feeds tools/ghidra_types.py and keeps the
offset comments honest."""
import json, os, re, sys, struct
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE); sys.path.insert(0, HERE)
import check
INC = os.path.join(ROOT, 'include')
OUT = os.path.join(ROOT, 'build', 'field_offsets.json')
CLASSDECL = re.compile(r'^\s*(?:class|struct)\s+(\w+)\s*(?::[^{]*)?\{')
SKIP = re.compile(r'^\s*(typedef|static|friend|using|enum|return|virtual|explicit|inline|template|public|private|protected|#|//|case|default|union\s*\{|struct\s*\{|class\s)')
MEMBER = re.compile(r'^\s*(?:mutable\s+)?[\w:<>,\s\*&]+?[\s\*&]([A-Za-z_]\w*)\s*(?:\[[^\]]*\])*\s*(?::\s*\d+)?\s*(?:,\s*\*?\s*([A-Za-z_]\w*)\s*(?:\[[^\]]*\])*\s*)*;')


NSDECL = re.compile(r'^\s*namespace\s+(\w+)\s*\{')


def parse(path):
    """[(qualified class, [members])] for classes at namespace scope (bitfields skipped: offsetof cannot take them)."""
    out = []; cur = None; depth = 0; ns = []; nsdepth = []
    for line in open(path, errors='replace'):
        code = line.split('//')[0]
        mn = NSDECL.match(code)
        if mn and depth == 0:
            ns.append(mn.group(1)); nsdepth.append(0); continue
        if depth == 0 and ns and code.strip().startswith('}'):
            ns.pop(); nsdepth.pop(); continue
        m = CLASSDECL.match(code)
        if m and depth == 0 and ';' not in code.split('{')[0]:
            cur = ('::'.join(ns + [m.group(1)]), []); out.append(cur)
            depth += code.count('{') - code.count('}')
            continue
        if cur and depth == 1 and not SKIP.match(code) and '(' not in code and ';' in code and ':' not in code.replace('::', ''):
            mm = MEMBER.match(code)
            if mm:
                names = re.findall(r'([A-Za-z_]\w*)\s*(?:\[[^\]]*\])*\s*(?=[,;])', code)
                for n in names:
                    if n not in cur[1]: cur[1].append(n)
        depth += code.count('{') - code.count('}')
        if depth <= 0: cur = None; depth = 0
    return out


def probe(header):
    classes = [(c, ms) for c, ms in parse(os.path.join(INC, header))]
    if not classes: return {}
    entries = []          # (class, member or None, line text)
    for c, ms in classes:
        entries.append((c, None))
        for m in ms: entries.append((c, m))
    bad = set()
    for attempt in range(6):
        src = ['#define private public', '#define protected public', '#include <stddef.h>', f'#include "{header}"', '#undef private', '#undef protected',
               'extern "C" const int pcj_probe[] = {']
        idx = []
        for k, (c, m) in enumerate(entries):
            if k in bad: continue
            src.append(f'  sizeof({c}),' if m is None else f'  offsetof({c}, {m}),')
            idx.append((len(src), k))
        src.append('  -1 };')
        path = os.path.join(ROOT, 'build', 'var', 'probe_' + header.replace('.h', '') + '.cpp')
        open(path, 'w').write('\n'.join(src) + '\n')
        try:
            o, _ = check.compile(path, 'vc71', '/O2 /G6 /arch:SSE /w')
        except RuntimeError as e:
            lines = set(int(n) for n in re.findall(r'probe_\w+\.cpp\((\d+)\)', str(e)))
            newbad = {k for ln, k in idx if ln in lines}
            if not newbad: return {}
            bad |= newbad
            continue
        sym = next((s for s in o.symbols.values() if s['name'] == '_pcj_probe'), None)
        if not sym: return {}
        data = o.sections[sym['sec'] - 1]['data'][sym['value']:]
        vals = struct.unpack_from('<%di' % len(idx), data)
        res = {}
        for (ln, k), v in zip(idx, vals):
            c, m = entries[k]
            r = res.setdefault(c.split('::')[-1], {'size': None, 'fields': {}})
            if m is None: r['size'] = v
            else: r['fields'][m] = v
        return res
    return {}


def main():
    a = sys.argv[1:]
    db = json.load(open(OUT)) if os.path.exists(OUT) else {}
    if a and a[0] == '--lint':
        sys.path.insert(0, HERE); import ghidra_types as g
        hdr = g.header_classes(); n = 0
        for c, (base, fields) in sorted(hdr.items()):
            pr = db.get(c, {}).get('fields', {})
            for off, typ, name, arr in fields:
                if name in pr and pr[name] != off:
                    print(f'{c}::{name}: comment 0x{off:x}, compiler 0x{pr[name]:x}'); n += 1
        print(n, 'mismatches'); return
    headers = a or sorted(f for f in os.listdir(INC) if f.endswith('.h'))
    for i, h in enumerate(headers):
        r = probe(h)
        db.update(r)
        print(f'[{i + 1}/{len(headers)}] {h}: {len(r)} classes, {sum(len(v["fields"]) for v in r.values())} fields', flush=True)
        if i % 20 == 19: json.dump(db, open(OUT, 'w'), indent=0)
    json.dump(db, open(OUT, 'w'), indent=0)


if __name__ == '__main__':
    main()
