#!/usr/bin/env python3
"""check.py SRC [--cc vc71|vc70] [--flags "..."] [-v]
Compile SRC with MSVC under Wine and compare every function in the .obj that the map names
against the retail XBE. DIR32/REL32 relocations to map symbols are checked exactly; others masked."""
import sys, os, subprocess, argparse, csv, struct, tempfile, shutil
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
from xbe import XBE; from coff import Obj
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
TC = {'vc71': '/home/claude/toolchains/vc71', 'vc70': '/home/claude/toolchains/vc70'}
# Native Windows (the permuter package): the toolchains sit next to the repo, cl.exe runs directly.
NATIVE = os.name == 'nt'
if NATIVE or os.environ.get('PCJ_TOOLCHAINS'):
    _tcroot = os.environ.get('PCJ_TOOLCHAINS') or os.path.join(os.path.dirname(ROOT), 'toolchains')
    TC = {k: os.path.join(_tcroot, k) for k in ('vc71', 'vc70')}
DEFAULT_FLAGS = "/O2 /G6 /arch:SSE"
_syms = None; _all = None
CUR_UNIT = None  # set by callers: file statics (_$E1, $SG...) repeat across objects, so prefer this unit's
def _load():
    global _syms, _all
    if _syms is None:
        _syms = {}; _all = {}
        for r in csv.DictReader(open(os.path.join(ROOT, 'symbols', 'symbols.csv'))):
            _syms.setdefault(r['name'], r); _all.setdefault(r['name'], []).append(r)
def lookup(name):
    _load()
    c = _all.get(name)
    if not c: return None
    if len(c) > 1 and CUR_UNIT:
        for r in c:
            if r['obj'].lower() == CUR_UNIT.lower() + '.obj': return r
        if any('s' in r['flags'] for r in c): return None  # a static of another object: not ours
    return c[0]
class _Syms(dict):
    def get(self, k, d=None):
        r = lookup(k); return d if r is None else r
    def __getitem__(self, k):
        r = lookup(k)
        if r is None: raise KeyError(k)
        return r
    def __contains__(self, k): return lookup(k) is not None
_SYMS = _Syms()
def syms():
    return _SYMS
def compile(src, cc, flags, inc=()):
    tc = TC[cc]; out = tempfile.mkdtemp(prefix='pcjcc')
    obj = os.path.join(out, 'o.obj'); log = os.path.join(out, 'log.txt')
    if NATIVE:
        env = dict(os.environ, INCLUDE=';'.join(list(inc) + [os.path.join(ROOT, 'include'), os.path.join(tc, 'Include')]),
                   PATH=os.path.join(tc, 'Bin') + ';' + os.environ.get('PATH', ''))
        cmd = [os.path.join(tc, 'Bin', 'cl.exe'), '/nologo', '/c', *flags.split(), '/Fo' + obj, os.path.abspath(src)]
    else:
        env = dict(os.environ, WINEPREFIX='/root/wine/pcj', WINEARCH='win32', WINEDEBUG='-all',
                   INCLUDE=';'.join(['Z:' + p.replace('/', '\\') for p in list(inc) + [os.path.join(ROOT, 'include'), tc + '/Include']]))
        cmd = ['wine', tc + '/Bin/cl.exe', '/nologo', '/c', *flags.split(), '/Fo' + 'Z:' + obj.replace('/', '\\'), 'Z:' + os.path.abspath(src).replace('/', '\\')]
    with open(log, 'w') as lf:
        try:
            rc = subprocess.run(cmd, stdout=lf, stderr=subprocess.STDOUT, env=env, timeout=180).returncode
        except subprocess.TimeoutExpired:
            if not NATIVE: subprocess.run(['pkill', '-f', obj.replace('/', '\\\\')[-30:]])
            rc = -1
            lf.write('cl.exe timed out after 180 s')
    msg = open(log, errors='replace').read()
    if rc or not os.path.exists(obj):
        shutil.rmtree(out); raise RuntimeError(f'compile failed (exit code {rc & 0xffffffff:#x}):\n' + msg)
    global CUR_UNIT
    CUR_UNIT = os.path.splitext(os.path.basename(src))[0]
    o = Obj(obj); shutil.rmtree(out); return o, msg
def compare(o, x, name, sec, off, size, verbose=False):
    S = syms()[name]; va = int(S['va'], 16)
    secd = o.sections[sec - 1]; mine = bytearray(secd['data'][off:off + size])
    # VC7.0 pads COMDAT tails with nops inside the object; VC7.1 leaves padding to the linker.
    while size > 1 and mine[-1] == 0x90 and not any(off <= r[0] < off + size and r[0] >= off + size - 4 for r in secd['relocs']):
        mine.pop(); size -= 1
    # strip trailing int3/zero padding inside the compiled section
    orig = bytearray(x.read(va, size))
    mask = [False] * size; badrel = []
    for rva, si, ty in secd['relocs']:
        if not (off <= rva < off + size): continue
        p = rva - off; tgt = o.symbols[si]; tn = tgt['name']
        for k in range(4): mask[p + k] = True
        T = syms().get(tn)
        if T is None and ty == 6 and (tn.startswith('??_C@') or tn.startswith('$SG')) and tgt['sec'] > 0:
            # string literal: not in the map, but the text at both ends of the pointer must agree
            sd = o.sections[tgt['sec'] - 1]['data']
            so = tgt['value'] + struct.unpack_from('<I', mine, p)[0]
            want_s = sd[so:sd.index(b'\0', so) + 1] if b'\0' in sd[so:] else sd[so:]
            try: have_s = x.read(struct.unpack_from('<I', orig, p)[0], len(want_s))
            except KeyError: have_s = b''
            if have_s != want_s: badrel.append((p, 'string', want_s[:40], have_s[:40]))
            continue
        if T is None or ty not in (6, 20): continue
        addend = struct.unpack_from('<i', mine, p)[0]
        tva = int(T['va'], 16) + (tgt['value'] if tgt['sec'] > 0 and False else 0)
        want = tva + addend if ty == 6 else tva + addend - (va + p + 4) + 4 - 4
        if ty == 20: want = tva - (va + p + 4) + addend
        got = struct.unpack_from('<I', orig, p)[0]
        if (want & 0xffffffff) != got: badrel.append((p, tn, hex(want & 0xffffffff), hex(got)))
    # original extent: map size includes padding; must be int3 after our length
    full = int(S['size'])
    pad = x.read(va + size, max(0, full - size))
    padok = all(b == 0xCC for b in pad)
    diff = [i for i in range(size) if not mask[i] and mine[i] != orig[i]]
    ok = not diff and not badrel and padok and size <= full
    if verbose and not ok:
        md = Cs(CS_ARCH_X86, CS_MODE_32)
        a = list(md.disasm(bytes(mine), va)); b = list(md.disasm(bytes(x.read(va, max(size, full))), va))
        print(f'   {"mine":46s} | orig')
        for i in range(max(len(a), len(b))):
            l = f'{a[i].address:x} {a[i].mnemonic} {a[i].op_str}' if i < len(a) else ''
            r = f'{b[i].address:x} {b[i].mnemonic} {b[i].op_str}' if i < len(b) else ''
            if r.endswith('int3 ') and not l: continue
            same = False
            if i < len(a) and i < len(b) and len(a[i].bytes) == len(b[i].bytes):
                o0 = a[i].address - va  # compare with relocated bytes masked (our side has no final addresses)
                same = all((o0 + k < size and mask[o0 + k]) or a[i].bytes[k] == b[i].bytes[k] for k in range(len(a[i].bytes)))
            print(f'   {l:46s} | {r}' + ('' if same else '   <<'))
        for br in badrel: print('   bad reloc', br)
        if not padok: print(f'   size: mine {size} vs orig extent {full} (non-int3 after mine)')
    return ok, len(diff), size, full
def compare_data(o, x, name, sec, off, size):
    """Initialised data (vtables, runtime-class records, tables): bytes equal, pointers to map symbols exact."""
    S = syms()[name]; va = int(S['va'], 16); secd = o.sections[sec - 1]
    mine = bytearray(secd['data'][off:off + size]); orig = x.read(va, size)
    mask = [False] * size; bad = []
    for rva, si, ty in secd['relocs']:
        if not (off <= rva < off + size) or ty != 6: continue
        p = rva - off; tgt = o.symbols[si]
        for k in range(4): mask[p + k] = True
        T = syms().get(tgt['name'])
        if T is None: continue
        want = (int(T['va'], 16) + struct.unpack_from('<I', mine, p)[0]) & 0xffffffff
        if want != struct.unpack_from('<I', orig, p)[0]: bad.append((p, tgt['name']))
    diff = [i for i in range(size) if not mask[i] and mine[i] != orig[i]]
    return not diff and not bad, diff, bad

def main():
    ap = argparse.ArgumentParser(); ap.add_argument('src'); ap.add_argument('--cc', default='vc71')
    ap.add_argument('--flags', default=DEFAULT_FLAGS); ap.add_argument('-v', action='store_true')
    a = ap.parse_args()
    x = XBE(os.path.join(ROOT, 'orig', 'DEFAULT.XBE'))
    o, msg = compile(a.src, a.cc, a.flags)
    n = m = 0
    for name, sec, off, size in sorted(o.functions(), key=lambda f: f[0]):
        if name not in syms(): continue
        # trim compiler alignment padding at end of COMDAT (none expected with /Gy)
        ok, nd, sz, full = compare(o, x, name, sec, off, size, False)
        n += 1; m += ok
        print(f"{'MATCH' if ok else 'DIFF '} {syms()[name]['va']} {sz:5d}/{full:<5d} {nd:4d}  {syms()[name]['demangled'][:90]}")
        if a.v and not ok: compare(o, x, name, sec, off, size, True)
    print(f'{m}/{n} match  [{a.cc} {a.flags}]')
if __name__ == '__main__': main()
