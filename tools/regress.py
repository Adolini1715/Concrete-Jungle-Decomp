#!/usr/bin/env python3
"""regress.py snap [FILE]   copy status/status.csv to FILE (default build/status_snapshot.csv)
   regress.py diff [FILE]   list functions that were MATCH in the snapshot and are not now (lost),
                            and count new matches. Run after `build.py all` or the checks you care about."""
import csv, os, shutil, sys
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CUR = os.path.join(ROOT, 'status', 'status.csv')
def load(p): return {r['va']: r for r in csv.DictReader(open(p))} if os.path.exists(p) else {}
a = sys.argv[1:] or ['diff']
snap = a[1] if len(a) > 1 else os.path.join(ROOT, 'build', 'status_snapshot.csv')
if a[0] == 'snap':
    os.makedirs(os.path.dirname(snap), exist_ok=True); shutil.copy(CUR, snap); print('snapshot ->', snap)
else:
    old, new = load(snap), load(CUR)
    lost = [v for v in old if old[v]['status'] == 'MATCH' and new.get(v, {}).get('status') != 'MATCH']
    gained = [v for v in new if new[v]['status'] == 'MATCH' and old.get(v, {}).get('status') != 'MATCH']
    print(f'{len(gained)} new matches, {len(lost)} lost')
    for v in lost: print(f"  LOST {v} {old[v]['unit']:24s} {old[v]['name'][:90]}")
