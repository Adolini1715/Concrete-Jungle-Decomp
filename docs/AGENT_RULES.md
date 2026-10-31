# Rules for decomp agents (Predator: Concrete Jungle, Xbox)

Goal: C++ source that compiles, with the original compiler, to the **exact bytes** of the retail XBE,
one original object file at a time. Read `docs/compiler-experiment.md` first (5 minutes).

## Environment (already set up; do not reinstall)
- Repo: `/home/claude/pcj-decomp`. Run everything from there with `python3 -I`.
- Compiler: VC 7.1 under Wine, flags `/O2 /G6 /arch:SSE` (default in tools). Never change flags.
- `orig/DEFAULT.XBE`, `orig/Predator.map`: the retail binary and its linker map (all names come from here).

## Tools
- `tools/build.py todo OBJ [N]`: unmatched functions of an original object (e.g. `EXArray`), smallest first.
- `tools/disasm.py NAME|VA ...`: disassembly with symbolized calls and data. Read this, write C++.
- `tools/genhdr.py CLASS`: declaration skeleton from the map: methods with exact signatures and access,
  virtuals in vtable slot order, static members, `sizeof` when known. Copy and edit; do not trust blindly.
- `tools/build.py check UNIT [-v]`: compile `src/<lib>/UNIT.cpp`, compare every function (and vtables,
  runtime-class records, initialised data) with the XBE, record results in `status/status.csv`.
  `-v` prints a side-by-side diff for mismatches.
- `tools/check.py FILE`: same comparison for a scratch file (use `build/var/` for experiments).
- `tools/build.py progress`: totals.
- `tools/regress.py snap build/snap_<yourarea>.csv` at the start and `tools/regress.py diff build/snap_<yourarea>.csv`
  after `build.py all`: lists lost matches. Always use your own snapshot file (agents run concurrently).
- `tools/fsweep.py UNIT FUNC_SUBSTR VARFILE`: like sweep.py but stops at the first MATCH (keep FUNC_SUBSTR short).
- `tools/perm.py OUT SPEC [LIMIT]`: generates statement-order permutations as a VARFILE (SPEC: head, then
  `@@`-separated movable statements, then `##`, then tail). Ideal for constructor store order.
- `tools/hsweep.py HEADER UNIT FUNC_SUBSTR VARFILE`: variants of a header inline (OLD / `-----` / NEW blocks
  separated by `=====`); restores the header afterwards. Note it writes status rows while sweeping: re-run
  `build.py check UNIT` on the final header.
- `tools/objdis.py OBJ > build/OBJ.dis` (large objects exceed tool output; read the file in pieces): whole object at once (data symbols, then every function's disassembly).
- `tools/dd.py VA N`: dump N dwords with symbol names (vtables, tables, runtime-class records).
- `tools/sweep.py UNIT FUNC_SUBSTR VARFILE [-v]`: try variants of one function in its unit. VARFILE holds
  complete function definitions separated by a line `=====` (several functions per variant joined by
  `+++++`); each variant replaces the function whose first line matches. `tools/apply.py UNIT VARFILE N`
  writes variant N into the unit.

## Layout
- One source file per original object: `src/enginex/EXFoo.cpp`, `src/game/Actor.cpp`. Define in it only
  functions the map attributes to that object (`obj` column), except header inlines (flag `i`), which live
  in headers and are matched wherever they get emitted.
- One header per class in `include/`, named after the class (`include/EXVector.h`), with include guards.
  `include/types.h` (s8/u8/u16/u32...), `include/EXMemory.h` (operator new/delete = `_EXAlloc`/`EXFree`),
  `include/EXRuntimeClass.h` (EX_DECLARE_DYNAMIC / EX_IMPLEMENT_DYNAMIC macros) already exist and match.
- Classes with a runtime-class record (`classFoo`, `_GetBaseClass`, `GetRuntimeClass`) use the macros.
  `EXItemHandler` is the worked example (`include/EXItemHandler.h`, `src/enginex/EXItemHandler.cpp`).

## Matching rules that are already proven (see docs/compiler-experiment.md)
- Names must mangle exactly: copy access specifiers (`protected` mangles differently), const, static,
  calling convention, and parameter types (`signed char` is the engine's bool) from `genhdr.py`.
- Virtual declaration order = vtable slot order. A `__purecall` slot is a pure virtual (or a pure virtual
  destructor with a body, as in EXItemHandler).
- `delete p` and `new T` go through the inline operators in EXMemory.h.
- Typical source-level levers when bytes differ: statement order; separate statements instead of chained
  assignment; a local copy of a global/static read; early `return` vs `found = i; break;`; `if (x)` vs
  `if (x > 0)` (unsigned gives `jbe`); inline helper functions (an inline `Next()` fixed EXRand's register
  choice); evaluating a call into a local before passing it; `u32` temporaries with a `(u16)` compare;
  `sin`/`cos` double intrinsics on floats; returning `EXVector(v.x, v.y, v.z, v.w)`.
- Inlines called from a constructor or from a compiler-generated `$E` static initialiser are emitted out
  of line even when every call is inlined (explains map `[fi]` entries with no callers).
- Implicitly used templates are always emitted; the map's template instances show which index types the
  source used (`int` vs `u8`).
- `__forceinline` inlines everywhere and still emits a copy; plain `inline` does not inline a function with a
  loop in VC 7.1. Inline asm is inlined by default (`__declspec(noinline)` where the map shows a call).
- A class with a vptr and an 8-byte member starts its first member at offset 8.
- `$E` numbering counts every dynamic-looking initialiser, even ones folded to static data
  (`float f = kConstFloat;` uses up a number). An inlined global ctor inside `$E` keeps source statement
  order; use the `$E` code to recover member order.
- Include `EXMemory.h` in every unit with deleting destructors, or they call the CRT `operator delete`.
- More levers: `x ? u8v : (u8)0` (sete/dec/and) vs `: 0` (branches); `cond ? 1 : 0` (neg/sbb/neg);
  modifying a parameter in place; a local pointer for a struct; `(flags & 0x80000000)` (test/js);
  nested `if`s instead of `&&`.
- Runtime-class macros: `EX_IMPLEMENT_DYNAMIC_ROOT`, `EX_IMPLEMENT_DYNAMIC`, `EX_IMPLEMENT_DYNCREATE`,
  `EX_IMPLEMENT_DYNCREATE_ROOT`.
- XDK/CRT functions you call (fread, XInput...) are declared by you inside the .cpp or a small header of
  your own, from the map's mangled names. Local stand-in declarations of another area's classes are marked
  `// TODO stand-in` so they can be replaced when the real header lands.
- Early-out chains: separate `if (a > b) return 0;` statements, not `||` (block layout differs).
- `x*x + y*y + z*z` compiles to y, z, x order: write it naturally. SSE `-f` is `xorps/subss`, x87 is `fchs`.
- Clamped `EXASin`/`EXACos` inlines (EXMaths.h) are what the engine uses for asin/acos.
- VC7.1 knows which registers a callee in the same file clobbers, so a caller may skip reloading ecx:
  such callers only match with the callee defined earlier in the same .cpp.
- Switch cases go in the order the original's blocks appear, not numeric order.
- Function-local data (CRC tables, string literals) is not in the map; references are masked.
- To check a header inline that your unit does not emit, add a small `Use()` caller in a scratch file.
- Jump-table switches: case bodies appear in source order, so read the order off the code addresses. Small
  sub/dec chains: write cases in natural order (VC emits them reversed).
- `static const float` tuning values are loaded from memory against variables but fold to `__real`
  constants against constants: declare named consts in the original order.
- VC keeps x87 values on the stack across calls to functions it already compiled in the same file; keep `[fi]`
  callees as header inlines or the spills differ.
- `handle &= mask;` then `m_pFiles[handle].x` beats a pointer local for address generation.
- A unit needing `$E3`/`$E4` with no other dynamic initialiser takes a dummy `static const float` +
  `static float` pair first; define a global's constructor above the global so it inlines into `$E`.
- An unused call result in retail can come from `else if (GetFlags()) {}`.
- genhdr's base class comes from the first constructor called before the vptr store; still sanity-check.
- **Overloaded virtuals land in the vtable in reverse declaration order** (EXItemAnimator::GetDatum,
  DoGrabPoint, EXItem::GetAnimatorDatum). genhdr prints real slot order: declare overloads reversed.
- Concrete items use `EX_DECLARE_ITEMTYPE(cls)` (include/EXPlacementItem.h) for their TypeName()/Size().
- Copies between different struct types (EXVector3 into a datum) are field by field: assign `.x/.y/.z`.
- VC7.1 stops inlining `mat_44::operator*` in large functions; a local holding the by-value result
  (`EXMaths::vec_xyzw d = m * v;`) can restore it.
- Item tree: children hang off `EXBaseItem::m_nodes` through an EXDListItem at +4 (`head ? head - 4 : 0`).
- Calling an inline accessor in a constructor body emits it out of line (map `[fi]` copies with no callers).
- FP operand order matters (`bias + L1` vs `L1 + bias`): try both.
- Base classes: the runtime-class record is authoritative; a non-polymorphic base (EXDListItem in
  EXItemCtrl) sits after the vptr and never shows in the records. genhdr marks vtable-similarity guesses.
- A `volatile` member reproduces retail's separate load/modify/store on a shared flag (Voice::m_Flags).
- At /O2 a function defined earlier in the same file is auto-inlined into later callers (duplicated loads in
  a caller often mean an inlined same-file helper). Keep the original definition order.
- Loops from a list `Tail()` walk `m_pPrev`; EXWnd/EXDListItem conversions are null-checked `static_cast`.
- Explicit switch cases that share the default's body need their own body to keep a jump table.
- A small `for` loop instead of unrolled stores changes register reloads (Voice::Voice).
- genhdr vtable slot counts include `__purecall` slots; when in doubt dump the vftable with `tools/dd.py`.
- Flags that compile to an `or` of a pre-masked shifted value are bitfields (`u8 m_Edit : 4`).
- Base-class calls written qualified (`EXDebugTree::AddItem`) are direct calls, not vtable calls.
- One-time statics guarded by an `s8` (`if (!s_Once) s_Once = 1;`) exist.
- Evaluating an inline's argument into a local before the call fixes push order.
- Placement new lives in include/EXNew.h; EXString.h, EXWString.h and EXItemAnimator code still carry their
  own copies: consolidate onto EXNew.h when you touch them.
- `-v` diff blocks now print right after their DIFF line.
- `memset(&pt, 0, sizeof(pt))` on small structs gives `lea ecx` + zero-eax stores; field stores give xorps.
- `if (a > b) return 1; return 0;` threads branches when inlined; `return a > b;` does not. Same for bools.
- Search loops: `for (..) if (a[i]==x) break;` gets unrolled; retail is
  `int i = 0; while (a[i] != x) { if (++i >= N) return; }`.
- Two locals passed by address: an array (`T v[2]`) fixes stack-slot order.
- Small struct returns (EXPointf): fill a local field by field, not via the 2-float constructor.
- String literals are now checked too: the text at the pointer must equal the retail string.
- cl.exe is killed after 180 s; a hang usually means a broken #include or macro.
- **Functions called with arguments in registers are file statics (map flag `s`), not LTCG.** VC7.1 gives
  same-file static functions a custom register convention: define them `static` in the same unit, before use.
- Bit setters: `m_flags = v ? (m_flags | b) : (m_flags & ~b)` gives the movzx/or/mov word shape.
- Hoist a loop bound into a local (`int n = arr.m_nSize`) for the count-down loop shape.
- Reach a repeated array element through a local reference or explicit pointer (`EXItemCtrlStack *pStack =
  &m_stacks[i]`) when the original uses it twice.
- Error paths: separate early returns put the error block inline; one `||` test puts it at the end.
- `void *` returns: `p = 0; if (...) {...} return p;`.
- Self-relative offsets go through a template/operator (EXRelPtr) rather than `&m_off + m_off`.
- Where the original inlined a call our compiler won't, a local `__forceinline` helper reproduces it.
- Declaration order of locals controls their stack slots.
- Matrix element access as `m.r[i][j]` vs `.x/.y` changes load and sum order; try both.
- FP early-outs: retail `comiss a,b; ja` comes from `!(a > b)`; `a <= b` gives `comiss b,a; jb`.
- Quaternion Euler builders store in reverse statement order. x87 operand order need not follow source.
- `[fi]` copies with no callers can also be `__forceinline` functions (quat ops, `qrecipsqrtf`).
- Const/non-const overload pairs: when retail calls the const overload from a non-const object, the source
  read through a const reference (`const T &r = m_x; r.Get()`). Fix call sites that way; never hide
  overloads per unit with #ifdefs (headers must declare the same thing everywhere).
- Accessors of the form `this ? m : 0` match only when their result is read once into a local.
- Binary search with a linear fallback: `goto notFound;` to one shared `return -1`.
- Cold early-outs last: `if (ok) { ...; return x; } return 0;`.
- Retail emits uncalled COMDATs (e.g. EXGeoSectionFile::GeoSectionHeader); restructuring a caller can stop
  emitting one and lose its match even though the caller matches. Watch the COMDAT count in `check`.
- To grep for statics use `grep -F '_$E'`.
- A call to an inline member of a sub-object (`m_Rect.Set(...)`) makes VC reload the object's other fields
  afterwards: that is how retail's reloads arise.
- VC 7.1 /G6 emits `cmov` for `if (x == C) x = d;` and ternaries; retail's branch comes from
  `if (x != C) a = x; else a = d;`.
- `__declspec(noinline)` where retail calls a small function VC would inline.
- Every FE object has a `$E6` from one header static (`NominalGameFrameRate() * 15`); see include/FEText.h.
- `memcpy(this, p, sizeof(*this))` reproduces a vptr-including copy constructor.
- Leaving a destructor implicit removes the vptr store from the base destructor.
- sweep.py/apply.py need the function head on its own first line and cannot target header inlines; edit
  headers by hand for those.
- `(int)f` gives cvttss2si; a float stored into an unsigned type goes through x87 + `__ftol2`.
- No user destructor: the implicit virtual dtor skips the vptr store; an explicit `~X() {}` adds one.
- Retail `test 0x28` on a combined flag test can come from two separate tests (`& 0x20`, `& 0x08`).
- A local copy of a float parameter (`float dt = t;`) reproduces a parameter-slot spill.
- Float products may be reordered by VC (no /Op): if operand order will not stick, mark NEAR and move on.
- Retail did not use xmmintrin intrinsics (they force a 16-byte stack realign); inline `__asm` was used for
  rcpss in the Xbox cloth code.
- EXPoint has a user copy constructor and operator= (emitted in FEHud.obj); struct copies through
  EXPoint/EXRect members give retail's interleaved load/store pattern.
- Allocation size expressions: write `cols * rows * sizeof(T)` in both the allocation and the memset.
- `s8 r = 0; ... r = 1; return r;` keeps a zero register that retail reuses for other zero constants.
- Two-condition search loops: `for (; !pressed && i < N; ++i)` (order matters).
- A common call after a switch with per-case arguments (shared tail), not one call per case.
- VC evaluates call arguments before a non-inlined `this` expression (`Instance(1)->F(a)`), not through an
  inline wrapper.
- Vectors are often copied as `EXMaths::vec_xyzw` locals (four int moves) rather than EXVector (float
  loads through set()): cast `(EXMaths::vec_xyzw)v`.
- `d.DotXYZ(d)` gives z,y,x x87 sum order; `LengthXYZSquared()` gives y,z,x.
- Pointer-walking loops (`e++`) where retail increments addresses; delete lists as
  `t = p; p = p->next; delete t;`.
- `__declspec(noinline)` reproduces calls to a reference-returning `[fi]` copy.
- A header inline with function-local statics shifts `$E` numbering in every unit that includes it: define it
  `inline` in the owning .cpp instead (FEHud::TintColour).
- `(float)fabs(x) < 0.001f` loads the epsilon as a float; without the cast it is a double.
- Arguments evaluated before a virtual call's vtable load go into locals first.
- A `case` falling through into the next reproduces a jump into the middle of a shared block.
- `regress.py diff` also shows losses caused by other agents' in-progress edits; confirm a loss with a
  per-unit `build.py check` before acting on it.
- **Template members that call a helper (EXDataArray::AddSpace -> GrowArray) are inlined or not per unit by
  VC's own heuristics.** Write the source the plain way (call the helper) and both retail shapes (inlined
  96-byte and calling 48-byte copies) come out right. Do not hand-expand helpers to force one shape.
- EXBaseParticleSys is a runtime-class root (EX_IMPLEMENT_DYNCREATE_ROOT) despite deriving from EXEffectsSys.
- EXRenderInfo copies are `memcpy` (retail `rep movsd`); plain assignment calls operator= out of line.
- Alignment-assert infinite loops are `if (cond) while (1) {}`.
- A straight-line if/else with one shared tail (`m_bDirty = 0; return 1;`): VC duplicates returns itself.
- `if ((flags & bit) > 0)` gives `test; jbe`; `if (flags & bit)` gives `je`.
- `while (n--) *p++ = c;` reproduces retail's hand fill; a memset call does not (in that unit).
- Inlines that other units call out of line: declare non-inline in the header, define `inline` in the
  owning .cpp (XApp::GameStatePush).
- A virtual call on a static member object needs a pointer to avoid devirtualisation.
- A static function whose caller is not written yet won't be emitted: leave it external with a TODO.
- Clone-chain loops are tail recursion: `if (m_pClone) return m_pClone->F();`.
- Retail loading all four dwords before storing = a copy through a local `EXMaths::vec_xyzw`.
- Keep a 3-way select as branches: `int n = 3; if (a) n = 1; else if (b) n = 2;`.
- growBy + size=0 stores are `SetGrowBy(n); Empty(0);`; two adjacent zero stores may be EXDList Init().
- Local EXCollisionType built from an EXSphere/EXCapsule: field-by-field copy, radius first, then x..w.
- Loops over a member count: re-read the member in the condition (hoisting it into a local unrolls 4x).
- `x = max(x, t)` with retail's pointer select needs a `const T &` template.
- Header inlines containing a switch are never inlined by VC7.1 (always emitted out of line).
- Known unsolved: in some medium functions VC7.1 will not inline by-value EXVector operators that retail
  inlined (looks like an inliner size budget). Mark NEAR rather than burning attempts.
- Bitfield flags: getters `bf == 1` (and/cmp/sete or byte dec/neg/sbb/inc), setters `bf = b != 0`;
  `bf = b` gives the movsx form. 16-bit EXFlag BitSet/BitClr narrow to byte or/and on memory.
- `if (x == 1) return 0; return 1;` vs `return x != 1;` give different layouts; a ternary
  `return f ? a*b : a;` keeps the multiply in registers.
- A store written before a memset is still scheduled after the `rep stos`.
- Search loops testing inside a `for` with `goto found` avoid unrolling.
- Large state-hash switches: generate cases from the jump table in block order.
- Headers whose accessors are `__forceinline` produce uncalled `[fi]` copies in every object that uses them
  (nav headers); recursive section lookups are plain `inline` and called.
- Recursive chain lookups: `if (!m_pNext) return 0; return m_pNext->F(x);` (tail recursion), not ternaries.
- `EXVector p(x, y, z, 0.0f); (v - p).Length()` reproduces retail's double vector copy.
- In game units, including FEText.h provides the shared `$E6`.
- Sweep variants mechanically: write 4 to 8 versions of the one function to `build/var/*.cpp` and run
  `tools/check.py` on each (see how docs/compiler-experiment.md did it). Keep the winner.

## Hard rules
- **No XDK headers, no leaked or third-party source.** Write every declaration yourself from the map and
  the disassembly. Eurochef / Sphinx docs may be read for understanding only.
- A stand-in needed by more than one header goes in its OWN header (include/<Class>.h with a TODO layout
  note), never as guarded copies inside several headers (include-order bugs; see EXTexSwapInfo.h).
- Only stand-in types for things outside your area: forward declarations, or minimal declarations with a
  `// TODO layout` comment. Never redefine a class whose header someone else owns (list below); add to it
  only by appending members you have verified, and re-run every unit that includes it.
- Budget: at most ~8 compile attempts per function. If it still differs, keep your closest version in the
  file, add `// NEAR: <n> bytes differ, <what>` above it, and move on. Do not burn time on one function.
- Every function you keep must compile; a unit that fails to compile counts as nothing.
- Do **not** run `git commit` (the lead commits). Do not touch files outside your assigned objects and the
  headers you own or create.
- Before finishing, run `tools/build.py check` on every unit you touched and report: per object
  matched/total, any NEAR functions, new headers, and anything surprising (new conventions, macros).

## Strategy note (2026-10-18)
- VC7.1 emits a COMDAT for every header inline that a compiled function references, even if fully inlined. Unmatched 16-byte accessors usually appear only once their (big) callers are written. Writing a big caller as a structurally correct NEAR is worth it for the accessors it pulls in.
- tools/idiff.py UNIT FUNCSUB: masked instruction diff. tools/ccobj.py: compile a unit and keep the .obj for `objdump -dr`.
- Inline that retail both inlines and emits out of line: `__forceinline`. Helpers retail calls but VC would inline: `__declspec(noinline)`.
- Fixed-count loops VC unrolls: `int i = 0; do {...} while (++i < N);` gives retail's rolled form.
- Where VC refuses to inline by-value EXVector operator-, use a file-local __forceinline field-wise subtract.

## Methodology (plan of record, 2026-10-18; claude/pcj-transition-plan.md)
- Byte matching stays the default for small and medium functions: it is the cheapest proof. Never break an existing MATCH (regress diff must show 0 lost).
- Big functions (roughly > 1.5 KB) and states: write for behaviour first. Max 3 byte-perfecting attempts, then leave as `// NEAR:` with a one-line note of what differs. Do not run permutation sweeps on them.
- Every non-matching function you leave must be honest about what it is:
  - `// NEAR: <what differs>` when the logic is believed complete (register/scheduling/structure residue).
  - `// GUESS: <what part is reconstructed by inference>` when any part of the logic is your inference rather than read from the disassembly.
  - `// PLACEHOLDER: <why>` for a stub kept only to fill a vtable slot or satisfy a caller. Never leave a stub without this tag.
- Compiling, or working in one scenario, is NOT verification. Only a byte match, or a recorded differential test (status/verify_overrides.csv, added by the lead), counts as verified.
- Coverage categories are computed by tools/coverage.py (BYTE_MATCH, FUNC_VERIFIED, PARTIAL, UNVERIFIED, PLACEHOLDER, UNWRITTEN); don't edit status files by hand.

## Conventions from the 2026-10-18 afternoon runs (ActorCombat, ActorUtil)
- Frame-rate globals: read the address from the function's own disassembly. 0x396d00 = XApp::GameFrameRateMul(); 0x39a044 = XGetFrameRateMul() (EXBaseApp). The wrong one is a bad relocation.
- Unnamed per-unit floats (e.g. ActorUtil 0x3876b0..) are file-level `static const float`s, shared at the top of the file; a literal folds into the shared `__real@` copy instead.
- File-static helpers with VC's register convention (map flag `s`): one extra caller can make VC drop the convention and lose every helper's match. Rebuild and check them after adding a caller.
- Random picks: `int r = EXRand(N)` (dead stack store) or `int anim = tbl[Rand32() & 1]; SetAnimType(anim,...)`; try both forms.
- Two-way calls: `if (c) F(a); else F(b);` not a ternary argument.
- Empty lifecycle cases each get their own `return 1;` (grouped labels become a range test).
- Order switch case bodies by retail code address, not numerically.
- EXVector by-value ops VC won't inline: file-local `__forceinline` helpers (VSub/AU_SubV style).
- Retail quirks (unused computations, unreleased resources, duplicate branches) are reproduced and commented, never "fixed" in the faithful source.
