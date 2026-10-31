#!/bin/bash
# typed_drafts.sh [import]: refresh Ghidra drafts that know our class layouts.
#   import  first time only: import the flat image (base 0x10000), apply map symbols, full analysis (~40 min)
#   (none)  re-probe offsets, re-export types, re-apply them, decompile every unwritten function (>= 32 B)
#           into build/drafts-typed/<unit>.c (hash names, switch case order). Read one with tools/brief.py.
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
PKG=${PCJ_GHIDRA_PKG:-/home/claude/ghidra-pkg}
G=${PCJ_GHIDRA:-/home/claude/ghidra/ghidra_11.2.1_PUBLIC}/support/analyzeHeadless
cp "$ROOT"/tools/ghidra/*.java "$PKG"/
if [ "$1" = import ]; then
  mkdir -p "$PKG/proj"
  MAXMEM=6G $G "$PKG/proj" pcj -import "$PKG/image.bin" -loader BinaryLoader -loader-baseAddr 0x10000 \
    -processor x86:LE:32:default -cspec windows -scriptPath "$PKG" -preScript PcjSetup.java "$PKG/symbols.txt"
fi
python3 -I "$ROOT/tools/offsets_probe.py" > "$ROOT/build/probe.log"
python3 -I "$ROOT/tools/ghidra_types.py"
python3 -I - "$ROOT" "$PKG/todo_unwritten.txt" <<'P'
import csv, sys
root, out = sys.argv[1:3]
st = {r['va'] for r in csv.DictReader(open(root + '/status/status.csv'))}
with open(out, 'w') as f:
    for r in csv.DictReader(open(root + '/symbols/functions.csv')):
        if r['origin'] in ('game', 'enginex') and r['va'] not in st and int(r['size']) >= 32:
            f.write(f"{r['va']} {r['obj']} {r['name']}\n")
P
rm -rf "$ROOT/build/drafts-typed-raw"
MAXMEM=6G $G "$PKG/proj" pcj -process image.bin -noanalysis -scriptPath "$PKG" \
  -postScript PcjTypes.java "$ROOT/build/ghidra_types.txt" \
  -postScript PcjDecomp.java "$PKG/todo_unwritten.txt" "$ROOT/build/drafts-typed-raw/out" > "$ROOT/build/ghidra_typed.log" 2>&1
cd "$ROOT" && PCJ_DRAFT_RAW=build/drafts-typed-raw/out PCJ_DRAFT_OUT=build/drafts-typed python3 -I tools/drafts.py
