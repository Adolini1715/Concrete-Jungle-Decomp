#!/usr/bin/env python3
"""classify.py [UNIT ...]: sort every DIFF function into a residue class, without touching status.csv.

Writes status/diffclass.csv (va, unit, cls, mine, orig, sim, name). Classes:
  REGALLOC     same instruction sequence (mnemonics), only registers/operands/stack slots differ
  SCHED        same multiset of instructions, different order
  STRUCT_CLOSE size within 15% and instruction-sequence similarity >= 0.75
  STRUCT       written, but the code shape differs more than that
  PLACEHOLDER  our body is < 35% of the original's size, or the source tags it PLACEHOLDER
These are residue classes for triage, NOT verification: none of them counts as verified.
Run it from a clean checkout (e.g. a git worktree of HEAD) so in-progress agent edits don't leak in."""
import csv, difflib, glob, os, re, subprocess, sys
from concurrent.futures import ThreadPoolExecutor
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ST = {r['va']: r for r in csv.DictReader(open(os.path.join(ROOT, 'status', 'status.csv')))}

def mnem(line):
    s = re.sub(r'^\s*[0-9a-f]+\s+', '', line.strip())
    return s.split(' ')[0] if s else ''

def parse(out):
    res = {}; cur = None
    for l in out.splitlines():
        m = re.match(r'^(DIFF |MATCH) ([0-9a-f]{8})\s+(\d+)/(\d+)', l)
        if m:
            cur = m.group(2) if m.group(1) == 'DIFF ' else None
            if cur: res[cur] = dict(mine=int(m.group(3)), orig=int(m.group(4)), L=[], R=[])
            continue
        if cur and '|' in l and not l.strip().startswith('mine'):
            a, b = l.split('|', 1)
            if mnem(a): res[cur]['L'].append(mnem(a))
            b = b.replace('<<', '')
            if mnem(b): res[cur]['R'].append(mnem(b))
    return res

def placeholder_tags(path):
    """Function names whose definition is preceded by a PLACEHOLDER tag within 3 lines."""
    tags = set(); lines = open(path, errors='replace').read().splitlines()
    for i, l in enumerate(lines):
        if 'PLACEHOLDER' in l:
            for j in range(i, min(i + 4, len(lines))):
                m = re.search(r'(\w+)\s*\(', lines[j])
                if m and j > i: tags.add(m.group(1)); break
    return tags

def run(unit):
    c = glob.glob(os.path.join(ROOT, 'src', '*', unit + '.cpp'))
    if not c: return unit, {}, set()
    p = subprocess.run(['python3', '-I', os.path.join(ROOT, 'tools', 'check.py'), c[0], '-v'], capture_output=True, text=True, cwd=ROOT)
    return unit, parse(p.stdout), placeholder_tags(c[0])

def cls(d, tagged, name):
    L, R = d['L'], d['R']
    short = re.sub(r'\(.*', '', name).split('::')[-1].split(' ')[-1]
    if short in tagged or d['mine'] < 0.35 * d['orig']: return 'PLACEHOLDER', 0.0
    sim = difflib.SequenceMatcher(None, L, R, autojunk=False).ratio() if L and R else 0.0
    if L == R: return 'REGALLOC', 1.0
    if sorted(L) == sorted(R): return 'SCHED', sim
    if abs(d['mine'] - d['orig']) <= 0.15 * d['orig'] and sim >= 0.75: return 'STRUCT_CLOSE', sim
    return 'STRUCT', sim

def main():
    diffs = {va: r for va, r in ST.items() if r['status'] == 'DIFF'}
    units = sys.argv[1:] or sorted({r['unit'] for r in diffs.values()})
    out = []
    with ThreadPoolExecutor(4) as ex:
        for unit, res, tagged in ex.map(run, units):
            for va, r in diffs.items():
                if r['unit'] != unit: continue
                d = res.get(va)
                if not d: out.append(dict(va=va, unit=unit, cls='STRUCT', mine=0, orig=0, sim=0, name=r['name'])); continue
                c, sim = cls(d, tagged, r['name'])
                out.append(dict(va=va, unit=unit, cls=c, mine=d['mine'], orig=d['orig'], sim=f'{sim:.2f}', name=r['name']))
            print(f'{unit}: done', file=sys.stderr)
    path = os.path.join(ROOT, 'status', 'diffclass.csv')
    keep = {} if not sys.argv[1:] else {r['va']: r for r in csv.DictReader(open(path))} if os.path.exists(path) else {}
    for r in out: keep[r['va']] = r
    with open(path, 'w', newline='') as f:
        w = csv.DictWriter(f, ['va', 'unit', 'cls', 'mine', 'orig', 'sim', 'name']); w.writeheader()
        for va in sorted(keep): w.writerow(keep[va])
    from collections import Counter
    print(Counter(r['cls'] for r in keep.values()))
if __name__ == '__main__': main()
