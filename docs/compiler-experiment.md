# Compiler-match experiment (2026-10-17)

**Result: byte-matching is open.** 32 of 33 distinct functions compiled by us are byte-identical to the
retail XBE (24 hand-written game/EngineX functions plus 9 compiler-emitted inlines and thunks that came
along). The one miss, `Actor_DelayedSightToTarget` (0x475d0), differs only in load scheduling.

## Toolchain
- **Visual C++ 7.1, cl 13.10.3077** (VS .NET 2003 RTM), from decomp.me's compiler list
  (`https://github.com/OmniBlade/decomp.me/releases/download/msvcwin9x/msvc7.0.tar.gz`, despite the name it is 7.1).
  Unpacked to `/home/claude/toolchains/vc71`, run under Wine 9 (32-bit prefix `/root/wine/pcj`).
- **Flags: `/O2 /G6 /arch:SSE`.** `/G7` and `/arch:SSE2` are wrong (8/24 vs 11/24 on the first pass);
  `/GX`, `/Ob1`/`/Ob2`, `/fp:fast` made no difference on this set.
- **Control, VC 7.0 (13.00.9466):** pads function tails with `nop` inside the object, while the XBE has
  linker `int3` padding. Ignoring padding it still misses all SSE code and several loops (22/33).
  So the compiler is VC 7.1 (3077 or a code-identical XDK build). No LTCG: file-static functions use
  VC 7.1's register calling convention (e.g. `CopyElements` in EXWString.obj), but cross-file calls are plain.

## What the matches taught us (source conventions)
- `delete p` with an inline `operator delete(void*) { EXFree(p); }` (emitted at 0x110e0).
- EXVector has a 4-float constructor (emitted at 0x118d0); `return EXVector(v.x, v.y, v.z, v.w)` explains
  the w, z, y, x load order in returns.
- `sin`/`cos` are the double intrinsics (`fsin`/`fcos`) on float arguments, not `sinf`.
- Statics read once into a local (`float dt = XApp::FrameLength();`).
- Separate statements, not chained assignment, for byte-field clears (the store order shows it).
- `found = i; break;` rather than `return i;` in search loops (WeaponProfile).
- Access specifiers are part of mangled names (`protected` = `I`/`1`), so declarations must copy them.
- Index temporaries are sometimes 32-bit with a 16-bit compare (`u32 li; if ((u16)li == 0xffff)`).

## Tools (repo `/home/claude/pcj-decomp`)
- `tools/parsemap.py`: map to `symbols/symbols.csv` (22,664) and `symbols/functions.csv` (17,394), with XBE VAs.
  Map section n maps to XBE sections in order (.text, D3D, D3DX, XGRPH, DSOUND, XMV, XPP, .rdata, .data, DOLBY).
- `tools/disasm.py NAME|VA`: capstone listing with symbolized call/data targets.
- `tools/check.py SRC [--cc vc71|vc70] [--flags] [-v]`: compile one file, compare every map-named
  function in the .obj against the XBE. DIR32/REL32 relocations to map symbols are checked exactly
  (target address must agree), others masked; the original's tail after our code must be `int3`.
- `src/exp/*.cpp`: the experiment sources (stand-in types only; no XDK headers).

## Next
- Adopt the AvP2 loop: easiest-first queue from `functions.csv`, one function per attempt, commit on match.
- Build shared headers from the map's mangled signatures (class names, method signatures, access) and
  the Aug 2004 build's assert strings (member names).
- Order: leaf functions first, by object; EngineX and game in parallel.
