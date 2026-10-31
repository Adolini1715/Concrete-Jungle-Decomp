#!/usr/bin/env python3
"""apply_perm.py RESULTS_DIR [--all] [--dry]

Apply permuter results back to the source.  By default only matches (score 0) are applied; --all also
takes improvements (score below the start score) so the next run starts closer.  Each unit touched is
recompiled afterwards with tools/check.py and the change is reverted if any function in it got worse.
"""
import os, re, sys, glob, csv, subprocess, argparse
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE); sys.path.insert(0, HERE)
import perm2


def unit_matches(path):
    out = subprocess.run([sys.executable, os.path.join(HERE, 'check.py'), path], capture_output=True, text=True, cwd=ROOT).stdout
    return {l.split()[1] for l in out.splitlines() if l.startswith('MATCH')}, out.strip().splitlines()[-1:]


def main():
    ap = argparse.ArgumentParser(); ap.add_argument('results'); ap.add_argument('--all', action='store_true'); ap.add_argument('--dry', action='store_true')
    a = ap.parse_args()
    fns = {r['va']: r for r in csv.DictReader(open(f'{ROOT}/symbols/functions.csv'))}
    by_file = {}
    for txt in sorted(glob.glob(os.path.join(a.results, '*.txt'))):
        head = open(txt).read().split('\n')[0].split()
        if len(head) < 4 or head[2] != 'score': continue
        unit, va, score = head[0], head[1], int(head[3])
        start = int(head[head.index('start') + 1]) if 'start' in head else 10 ** 9
        if score != 0 and not (a.all and score < start): continue
        best = open(txt[:-4] + '.best.cpp', encoding='latin-1').read()
        dem = fns[va]['demangled']
        path = perm2.find_unit_src(unit); src = open(path, encoding='latin-1').read()
        pos = perm2.locate(src, dem); tgt = path
        if pos is None:
            for h in sorted(glob.glob(f'{ROOT}/include/*.h')):
                hs = open(h, encoding='latin-1').read(); hp = perm2.locate_inline(hs, dem)
                if hp is not None: tgt, src, pos = h, hs, hp; break
        if pos is None: print('cannot locate', unit, va, dem); continue
        by_file.setdefault(tgt, []).append((unit, va, score, start, pos, best, dem))
    for tgt, items in by_file.items():
        src = open(tgt, encoding='latin-1').read(); orig = src
        # apply from the end of the file backwards so positions stay valid
        for unit, va, score, start, pos, best, dem in sorted(items, key=lambda x: -x[4]):
            pre, fn, post = perm2.split_fn(src, pos)
            src = pre + best + post
            print(f'{"MATCH" if score == 0 else "improve %d->%d" % (start, score)}  {unit} {va}  {dem[:80]}')
        if a.dry: continue
        open(tgt, 'w', encoding='latin-1').write(src)
        units = {it[0] for it in items}
        for unit in units:
            path = perm2.find_unit_src(unit)
            before_src = open(path, encoding='latin-1').read()
            # compare match sets before/after for this unit
            open(tgt, 'w', encoding='latin-1').write(orig); m0, _ = unit_matches(path)
            open(tgt, 'w', encoding='latin-1').write(src); m1, tail = unit_matches(path)
            lost = m0 - m1
            if lost:
                print(f'  {unit}: reverted, lost {sorted(lost)}'); open(tgt, 'w', encoding='latin-1').write(orig)
            else:
                print(f'  {unit}: ok, +{len(m1 - m0)} matches  {tail}')


if __name__ == '__main__':
    main()
