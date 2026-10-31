#!/usr/bin/env python3
"""perm_summary.py RESULTS_DIR [--cores N]: matches, improvements, throughput of a perm3 run.
Wall time comes from RESULTS_DIR/start (epoch seconds) when present."""
import os, sys, glob, time, argparse
ap = argparse.ArgumentParser(); ap.add_argument('dir'); ap.add_argument('--cores', type=int, default=0); ap.add_argument('--mark', action='store_true'); a = ap.parse_args()
if a.mark:
    os.makedirs(a.dir, exist_ok=True); open(os.path.join(a.dir, 'start'), 'w').write(str(time.time())); sys.exit(0)
rows = []; errs = 0
for t in glob.glob(os.path.join(a.dir, '*.txt')):
    h = open(t, errors='replace').readline().split()
    if len(h) > 2 and h[2] == 'error': errs += 1; continue
    if len(h) < 10 or h[2] != 'score': continue
    d = dict(zip(h[2::2], h[3::2])); rows.append((h[0], h[1], int(d['score']), int(d['start']), int(d['tries']), float(d['secs'])))
n = len(rows); m = [r for r in rows if r[2] == 0]; imp = [r for r in rows if 0 < r[2] < r[3]]
tries = sum(r[4] for r in rows)
print(f'targets finished: {n}   errors: {errs}')
print(f'MATCHES: {len(m)}   improved (not matched): {len(imp)}   unchanged: {n - len(m) - len(imp)}')
for r in sorted(m): print(f'  match  {r[0]} {r[1]}  (start {r[3]}, {r[4]} tries, {r[5]:.0f}s)')
print(f'compiles: {tries}')
sf = os.path.join(a.dir, 'start')
if os.path.exists(sf):
    wall = time.time() - float(open(sf).read())
    print(f'wall time: {wall/60:.1f} min   throughput: {tries/wall:.1f} compiles/s' + (f'  ({tries/wall/a.cores:.2f} per core)' if a.cores else ''))
if n: print(f'hit rate: {len(m)}/{n} = {100*len(m)/n:.1f}%')
