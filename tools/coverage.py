#!/usr/bin/env python3
"""coverage.py [--md]: reconstruction coverage under the 2026-10-18 methodology.

Categories (each function is in exactly one):
  BYTE_MATCH     compiles to the original bytes (status.csv MATCH). Behaviour proven on every path.
  FUNC_VERIFIED  differential tests cover every reachable path (status/verify_overrides.csv, with evidence).
  PARTIAL        differential tests pass on some paths only (verify_overrides.csv, with evidence).
  REPLAY_VERIFIED dual execution identical on every call made by the gameplay recordings, and the recordings replay
                 exact with the function native (pcj-native tools/verify.py; verify_overrides.csv, with evidence).
  UNVERIFIED     written and compiling, no behavioural evidence. Sub-tagged by residue class from
                 status/diffclass.csv (tools/classify.py): REGALLOC, SCHED, STRUCT_CLOSE, STRUCT, and GUESS
                 when the source marks part of the logic as inferred.
  PLACEHOLDER    stub or partial body (diffclass PLACEHOLDER, or tagged in source).
  UNWRITTEN      no compiled body in our build (includes header inlines whose callers are unwritten).
Compiling or running in one scenario never moves a function out of UNVERIFIED.
Also reported: Xbox library code (XDK, slated for replacement, not decompilation) and the share of game+engine
code a hybrid build must still take from the original/recompiled binary (everything not BYTE_MATCH/FUNC_VERIFIED)."""
import csv, glob, os, re, sys
from collections import defaultdict
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
P = lambda *a: os.path.join(ROOT, *a)

SUBSYS = [  # first match wins; patterns on object names
    ('Player & camera', r'^(Player|Camera|Camera_Lockon|CameraScript|XItemPhysics|XGamePad|XBGamepad|EXGamepad)$'),
    ('Combat & AI', r'^(Actor.*|AI.*|Nav.*|XNav.*|XGeoEntity_NavMesh|EXGeoEntity_NavMesh|Avoid|Bullet.*|WeaponTable|Test.*|Honour|Reinforcements|Predator)$'),
    ('Missions & scripts', r'^(Trigger|Objective.*|ScriptRunner|GameData|GameIsland|EXTrigger|EXGeoScript|EXItemAnimator_Script.*|Recharge|GeoSectionManager|NightAndDay)$'),
    ('Vehicles & traffic', r'^(Vehicle.*|Traffic.*|FX_AirTraffic.*)$'),
    ('Animation', r'^(EXItemAnimator.*|EXItemCtrl.*|EXGeoAnim|XBGeoAnim|SubGroupAnimTable|RagdollPhysics)$'),
    ('Physics & collision', r'^(EXPhysics.*|EXCollision.*|XBCollision|XBPhysics|EXIntersection|EXDistance|EXGeoGrabpoint|EXPlane|EXLine|EXBounds|XBBounds)$'),
    ('Renderer & Xbox graphics', r'^(XB(?!Audio|Mixer|SoundManager|SfxCmds|File|Malloc|MemCard|Boot|Gamepad|Collision|Physics|Bounds|Stats|Benchmark|App).*|EXItemRender.*|EXGeo.*|EXRender.*|EXDisplay|EXBaseDisplay|EXLight.*|EXParticle.*|EXSwoosh|EXEffectsSys|EXTexture|EXCamera|EXWnd.*|xbMovie)$'),
    ('Audio', r'^(EXAudio.*|EXSound.*|EXSfxcmds|XBAudio|XBMixer|XBSoundManager|XBSfxCmds|VehicleSound)$'),
    ('Front end & UI', r'^(FE.*|EXWidget|EXMenu|Text|VisionWipe)$'),
    ('Effects (FX_*)', r'^(FX_.*|Fx_.*|Voosh)$'),
]
def subsys(obj):
    o = obj[:-4] if obj.endswith('.obj') else obj
    for name, pat in SUBSYS:
        if re.match(pat, o): return name
    return 'Core, app & tools'

def source_tags():
    """name -> GUESS/PLACEHOLDER from `// GUESS:` / `// PLACEHOLDER:` comments within 3 lines above a definition."""
    tags = {}
    for path in glob.glob(P('src', '*', '*.cpp')):
        lines = open(path, errors='replace').read().splitlines()
        for i, l in enumerate(lines):
            m = re.search(r'//\s*(GUESS|PLACEHOLDER)\b', l)
            if not m: continue
            for j in range(i + 1, min(i + 5, len(lines))):
                d = re.match(r'^[\w:<>*&\s,~]*?([\w~]+(?:::[\w~]+)*)\s*\(', lines[j])
                if d and not lines[j].lstrip().startswith('//'):
                    tags[d.group(1).split('::')[-1]] = m.group(1); break
    return tags

def load(path, key='va'):
    return {r[key]: r for r in csv.DictReader(open(path))} if os.path.exists(path) else {}

def main():
    md = '--md' in sys.argv
    fn = [r for r in csv.DictReader(open(P('symbols', 'functions.csv')))]
    st, dc, ov = load(P('status', 'status.csv')), load(P('status', 'diffclass.csv')), load(P('status', 'verify_overrides.csv'))
    tags = source_tags()
    tot = defaultdict(lambda: [0, 0]); area = defaultdict(lambda: defaultdict(lambda: [0, 0]))
    sub = defaultdict(lambda: defaultdict(lambda: [0, 0])); res = defaultdict(lambda: [0, 0])
    xdk = [0, 0]
    for r in fn:
        size = int(r['size'])
        if r['origin'].startswith('xdk'): xdk[0] += 1; xdk[1] += size; continue
        if r['origin'] not in ('game', 'enginex'): continue
        va = r['va']; s = st.get(va, {}).get('status')
        short = re.sub(r'\(.*', '', r['demangled']).split('::')[-1].split(' ')[-1]
        if s == 'MATCH': c = 'BYTE_MATCH'
        elif va in ov and ov[va]['category'] in ('FUNC_VERIFIED', 'PARTIAL', 'REPLAY_VERIFIED') and ov[va].get('evidence'): c = ov[va]['category']
        elif s == 'DIFF':
            k = dc.get(va, {}).get('cls', 'STRUCT')
            if k == 'PLACEHOLDER' or tags.get(short) == 'PLACEHOLDER': c = 'PLACEHOLDER'
            else:
                c = 'UNVERIFIED'; k = 'GUESS' if tags.get(short) == 'GUESS' else k
                res[k][0] += 1; res[k][1] += size
        else: c = 'UNWRITTEN'
        for d in (tot[c], area[r['origin']][c], sub[subsys(r['obj'])][c]): d[0] += 1; d[1] += size
    cats = ['BYTE_MATCH', 'FUNC_VERIFIED', 'REPLAY_VERIFIED', 'PARTIAL', 'UNVERIFIED', 'PLACEHOLDER', 'UNWRITTEN']
    N = sum(v[0] for v in tot.values()); B = sum(v[1] for v in tot.values())
    pct = lambda a, b: f'{100 * a / b:.1f}%' if b else '-'
    out = []
    out.append('| Category | Functions | % | Code bytes | % |'); out.append('|---|---|---|---|---|')
    for c in cats: out.append(f'| {c} | {tot[c][0]:,} | {pct(tot[c][0], N)} | {tot[c][1]:,} | {pct(tot[c][1], B)} |')
    out.append(f'| total game+engine | {N:,} | | {B:,} | |')
    out.append('')
    out.append('UNVERIFIED by residue class (triage only; none of these counts as verified):')
    out.append('| Class | Functions | Code bytes |'); out.append('|---|---|---|')
    for k in ['REGALLOC', 'SCHED', 'STRUCT_CLOSE', 'STRUCT', 'GUESS']: out.append(f'| {k} | {res[k][0]:,} | {res[k][1]:,} |')
    out.append('')
    ver = tot['BYTE_MATCH'][1] + tot['FUNC_VERIFIED'][1] + tot['REPLAY_VERIFIED'][1]
    written = ver + tot['PARTIAL'][1] + tot['UNVERIFIED'][1]
    out.append(f'- Readable C++ written (all but PLACEHOLDER/UNWRITTEN): {pct(written, B)} of code bytes')
    out.append(f'- Verified (BYTE_MATCH + FUNC_VERIFIED + REPLAY_VERIFIED): {pct(ver, B)} of code bytes (REPLAY_VERIFIED covers the paths the recordings take, not every path)')
    out.append(f'- Hybrid build still depends on original/recompiled code for {pct(B - ver, B)} of game+engine bytes, plus all Xbox library code ({xdk[0]:,} functions, {xdk[1]:,} bytes: replace, do not decompile)')
    tp = '/home/claude/pcj-native/tools/hybridize/targets.txt'
    nat = []
    if os.path.exists(tp):
        for l in open(tp):
            l = l.split('#')[0].strip()
            if l.startswith(('unit=', 'mod=')): nat += [v.lower() for v in l.split()[1:]]
    nb = sum(int(r['size']) for r in fn if r['va'].lower() in nat)
    out.append(f'- Running as generated native C++ in the hybrid build (pcj-native targets.txt: unit= groups A/B-passed on the boot path, mod= groups run native in play): {len(nat)} functions, {nb:,} bytes. This is integration evidence, not full-path verification.')
    fx = sorted(glob.glob('/home/claude/pcj-native/tests/fixtures/rec-*/input.pcjr'))
    out.append(f'- Engine subsystems replaced: 0. End-to-end gameplay coverage: {len(fx)} deterministic gameplay recordings (pcj-native tests/fixtures, chapter 1), replayed exact by tools/det/soak.sh.')
    out.append('')
    out.append('| Area | ' + ' | '.join(cats) + ' |'); out.append('|---' * (len(cats) + 1) + '|')
    for a in ('game', 'enginex'):
        t = sum(v[1] for v in area[a].values())
        out.append(f'| {a} | ' + ' | '.join(pct(area[a][c][1], t) for c in cats) + ' |')
    out.append('')
    out.append('By subsystem (share of code bytes):')
    out.append('| Subsystem | KB | ' + ' | '.join(cats) + ' |'); out.append('|---' * (len(cats) + 2) + '|')
    for s in sorted(sub, key=lambda s: -sum(v[1] for v in sub[s].values())):
        t = sum(v[1] for v in sub[s].values())
        out.append(f'| {s} | {t // 1024} | ' + ' | '.join(pct(sub[s][c][1], t) for c in cats) + ' |')
    text = '\n'.join(out)
    print(text)
    if md:
        open(P('docs', 'COVERAGE.md'), 'w').write('# Reconstruction coverage\n\nGenerated by tools/coverage.py; categories defined in its docstring.\n\n' + text + '\n')
if __name__ == '__main__': main()
