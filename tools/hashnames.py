#!/usr/bin/env python3
"""hashnames.py: names for the game's 32-bit hash IDs (top byte = category).

  hashnames.py 0x4d000038 0x42000086 ...   look values up
  hashnames.py --annotate < file            append "; NAME" to lines with known hash constants
  hashnames.py --build                      rebuild symbols/hashnames.csv
  python: from hashnames import name; name(0x4d000038) -> 'HT_ActorState_AttackPlayer'

Sources, later ones win for the same value:
  1. the Aug 2004 prototype's Albert/hashcodes.h (8,274 HT_* names; categories are stable in retail,
     checked on ActorState, LocationType, ActorProfile, AnimMode)
  2. retail text strings (0x51xxxxxx) from pcj-native's text dump (CJ_TEXT_DUMP=1), shown as
     Text"..." because text IDs moved between the prototype and retail
  3. AP_GetStateFromHash in src/game/ActorProfile.cpp (state hash -> STATE_ function)
symbols/hashnames.csv caches the merged table (value, name, source)."""
import csv, os, re, sys
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE)
CSV = os.path.join(ROOT, 'symbols', 'hashnames.csv')
PROTO = os.environ.get('PCJ_HASHCODES', '/home/claude/pcj/proto_aug04/Predator/Albert/hashcodes.h')
TEXT = os.environ.get('PCJ_TEXT_DUMP', '/home/claude/pcj-native/text/dump.txt')
_tab = None


def build():
    tab = {}
    if os.path.exists(PROTO):
        for line in open(PROTO, errors='replace'):
            m = re.match(r'\s*#define\s+(HT_\w+)\s+(0x[0-9a-fA-F]+)', line)
            if m and not m.group(1).endswith('HASHCODE_BASE') and not m.group(1).endswith('HASHCODE_END'):
                tab.setdefault(int(m.group(2), 16), (m.group(1), 'aug04'))
    if os.path.exists(TEXT):
        for line in open(TEXT, 'rb'):
            p = line.decode('cp1252', 'replace').rstrip('\r\n').split('\t', 1)
            if len(p) == 2 and re.fullmatch(r'[0-9A-Fa-f]{8}', p[0]):
                s = p[1].replace('\\n', ' ')
                tab[int(p[0], 16)] = ('Text"' + (s[:48] + '...' if len(s) > 48 else s) + '"', 'retail-text')
    src = os.path.join(ROOT, 'src', 'game', 'ActorProfile.cpp')
    if os.path.exists(src):
        pend = []
        for line in open(src, errors='replace'):
            m = re.match(r'\s*case\s+(0x[0-9a-fA-F]+)\s*:', line)
            if m: pend.append(int(m.group(1), 16)); continue
            m = re.match(r'\s*return\s+(STATE_\w+)\s*;', line)
            if m and pend:
                for v in pend:
                    old = tab.get(v, ('', ''))[0]
                    tab[v] = ((old + ' -> ' if old else '') + m.group(1), 'ap_state')
                pend = []
            elif line.strip() and not line.strip().startswith('//'):
                pend = []
    with open(CSV, 'w', newline='') as f:
        w = csv.writer(f); w.writerow(['value', 'name', 'source'])
        for v in sorted(tab): w.writerow(['0x%08x' % v, tab[v][0], tab[v][1]])
    return tab


def load():
    global _tab
    if _tab is None:
        if os.path.exists(CSV):
            _tab = {int(r['value'], 16): (r['name'], r['source']) for r in csv.DictReader(open(CSV))}
        else:
            _tab = build()
    return _tab


def name(v):
    t = load().get(v)
    return t[0] if t else None


HEX = re.compile(r'0x([0-9a-fA-F]{7,8})\b')


FLAGOPS = re.compile(r'\b(test|and|or|xor|not|shl|shr|sar|rol|ror)\b')


def annotate(line):
    names = []
    if FLAGOPS.search(line): return line
    for m in HEX.finditer(line):
        v = int(m.group(1), 16)
        if v >> 24 in (0, 0x80, 0xff): continue
        n = name(v)
        if n and n not in names: names.append(n)
    return line + ('  ; ' + ', '.join(names) if names else '')


def main():
    a = sys.argv[1:]
    if not a: sys.exit(__doc__)
    if a[0] == '--build':
        t = build(); print(f'{len(t)} names -> {CSV}'); return
    if a[0] == '--annotate':
        for line in sys.stdin: print(annotate(line.rstrip('\n')))
        return
    for x in a:
        v = int(x, 16)
        print('0x%08x  %s' % (v, name(v) or '?'))


if __name__ == '__main__':
    main()
