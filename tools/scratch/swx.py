#!/usr/bin/env python3
"""Draft C++ for switch-heavy functions: follows jump tables in a disassembly listing
(tools/disasm.py output) and translates simple straight-line case bodies up to a join address.
Unrecognised instructions are kept as comments for hand fixing.
  swx.py LISTING JOIN_VA OUTER_TABLE_VA N_OUTER OUTER_REG_NAME"""
import re, sys, subprocess
lst, join, tab, n = sys.argv[1], int(sys.argv[2],16), int(sys.argv[3],16), int(sys.argv[4])
ins = {}
order = []
for l in open(lst):
    m = re.match(r'\s+([0-9a-f]+)\s+[0-9a-f]+\s+(.*)', l)
    if m:
        a = int(m.group(1),16); t = m.group(2).split('  ;')[0].strip(); c = m.group(2).split('  ;')[1].strip() if '  ;' in m.group(2) else ''
        ins[a] = (t, c); order.append(a)
nxt = {order[i]: order[i+1] for i in range(len(order)-1)}
def dd(va, k):
    out = subprocess.run(['python3','-I','tools/dd.py',hex(va),str(k)],capture_output=True,text=True).stdout.split('\n')
    return [int(x.split()[1],16) for x in out if x.strip()]
FIELDS = {0x218:'m_MSpeed',0x21c:'m_Unk21c',0x224:'m_Unk224',0x230:'m_Unk230',0x234:'m_Unk234',0x360:'m_Unk360',0x22c:'m_CrouchTimer'}
def fld(off): return FIELDS.get(off, 'm_Unk%x' % off)
def body(a, depth, seen):
    """translate from a to join; returns list of lines"""
    out = []; pushes = []
    steps = 0
    while a != join and steps < 400:
        steps += 1
        if a not in ins: out.append('/* ?? %x */' % a); break
        t, c = ins[a]
        m = re.match(r'jmp (0x[0-9a-f]+)$', t)
        if m:
            a = int(m.group(1),16); continue
        m = re.match(r'push (0x[0-9a-f]+|-?\d+)$', t)
        if m: pushes.append(int(m.group(1),0)); a = nxt[a]; continue
        m = re.match(r'push (e\w\w)$', t)
        if m: pushes.append(m.group(1)); a = nxt[a]; continue
        if t == 'mov ecx, esi' or t == 'mov ecx, ebp': a = nxt[a]; continue
        m = re.match(r'lea (e\w\w), \[esi \+ (0x[0-9a-f]+)\]', t)
        if m: lea = int(m.group(2),16); regs = globals().setdefault('regs',{}); regs[m.group(1)] = '&%s' % fld(lea); a = nxt[a]; continue
        if t.startswith('call'):
            name = c
            if 'ForceModeChange' in name: out.append('ForceModeChange(%s);' % (hex(pushes[-1]) if isinstance(pushes[-1],int) else pushes[-1]))
            elif 'SetupDepthPtr' in name:
                p = pushes[-1]; out.append('SetupDepthPtr(%s, %s);' % (regs.get(p,p) if isinstance(p,str) else hex(p), pushes[-2]))
            elif 'BlendTo' in name:
                a1,a2,a3,a4 = pushes[-1],pushes[-2],pushes[-3],pushes[-4]
                out.append('pAnim->BlendTo(%s, 0x%x, %s, %s);' % (a1, a2, a3, a4))
            elif 'Rand32' in name: out.append('/* Rand32 */')
            else: out.append('/* call %s args %s */' % (name, pushes[::-1][:4]))
            pushes = []; a = nxt[a]; continue
        m = re.match(r'mov edi, (0x[0-9a-f]+|\d+)$', t)
        if m: out.append('sub = %s;' % m.group(1)); a = nxt[a]; continue
        if t == 'xor edi, edi': out.append('sub = 0;'); a = nxt[a]; continue
        m = re.match(r'mov dword ptr \[esi \+ (0x[0-9a-f]+)\], (0x[0-9a-f]+|\d+)$', t)
        if m: out.append('%s = %s;' % (fld(int(m.group(1),16)), m.group(2))); a = nxt[a]; continue
        m = re.match(r'movss dword ptr \[esi \+ (0x[0-9a-f]+)\], xmm0', t)
        if m: out.append('%s = XMM0;' % fld(int(m.group(1),16))); a = nxt[a]; continue
        m = re.match(r'mov byte ptr \[esi \+ (0x[0-9a-f]+)\], (\w+)', t)
        if m: out.append('%s = %s;' % (fld(int(m.group(1),16)), m.group(2))); a = nxt[a]; continue
        if t == 'mov edi, dword ptr [esp + 0x14]': a = nxt[a]; continue
        # jump-table switch on edi/eax
        m = re.match(r'cmp (edi|eax), (0x[0-9a-f]+|\d+)$', t)
        if m and a in nxt and ins[nxt[a]][0].startswith('ja ') and ins[nxt[nxt[a]]][0].startswith('jmp dword ptr ['):
            hi = int(m.group(2),0); dflt = int(ins[nxt[a]][0].split()[1],16)
            tt = int(re.search(r'\+ (0x[0-9a-f]+)\]', ins[nxt[nxt[a]]][0]).group(1),16)
            tgts = dd(tt, hi+1)
            var = 'sub' if m.group(1)=='edi' else 'r'
            out.append('switch (%s) {' % var)
            groups = {}
            for k,g in enumerate(tgts): groups.setdefault(g,[]).append(k)
            for g,ks in sorted(groups.items(), key=lambda x: x[1][0]):
                if g == dflt: continue
                for k in ks: out.append('case %d:' % k)
                for l in body(g, depth+1, seen): out.append('    '+l)
                out.append('    break;')
            if dflt != join:
                out.append('default:')
                for l in body(dflt, depth+1, seen): out.append('    '+l)
                out.append('    break;')
            out.append('}')
            return out
        if t == 'test edi, edi' and ins[nxt[a]][0].startswith('jne '):
            d = int(ins[nxt[a]][0].split()[1],16)
            out.append('if (sub == 0) {')
            for l in body(nxt[nxt[a]], depth+1, seen): out.append('    '+l)
            out.append('}')
            if d != join:
                out.append('else {')
                for l in body(d, depth+1, seen): out.append('    '+l)
                out.append('}')
            return out
        if t == 'mov eax, edi':
            # chain of sub/dec + je/jne
            cum = 0; b = nxt[a]; cases = []
            while True:
                tb = ins[b][0]
                mm = re.match(r'sub eax, (0x[0-9a-f]+|\d+)$', tb)
                if mm: cum += int(mm.group(1),0); b = nxt[b]; continue
                if tb == 'dec eax': cum += 1; b = nxt[b]; continue
                mm = re.match(r'je (0x[0-9a-f]+)', tb)
                if mm: cases.append((cum, int(mm.group(1),16))); b = nxt[b]; continue
                mm = re.match(r'jne (0x[0-9a-f]+)', tb)
                if mm: cases.append((cum, nxt[b])); dflt = int(mm.group(1),16); break
                mm = re.match(r'jmp (0x[0-9a-f]+)', tb)
                dflt = int(mm.group(1),16) if mm else b; break
            out.append('switch (sub) {')
            for k,g in cases:
                out.append('case %d:' % k)
                for l in body(g, depth+1, seen): out.append('    '+l)
                out.append('    break;')
            if dflt != join:
                out.append('default:')
                for l in body(dflt, depth+1, seen): out.append('    '+l)
                out.append('    break;')
            out.append('}')
            return out
        if t == 'and eax, 3' and out and out[-1] == '/* Rand32 */':
            out[-1] = '/* r = g_EXRandClass.Rand32() & 3 */'
            a = nxt[a]; continue
        out.append('/* %x: %s %s */' % (a, t, c))
        if t.startswith('j') or t.startswith('ret'):
            out.append('/* STOP */'); break
        a = nxt[a]
    return out
outer = dd(tab, n)
for i, a in enumerate(outer):
    print('case 0x%x: /* %x */' % (i, a))
    for l in body(a, 0, set()): print('    ' + l)
