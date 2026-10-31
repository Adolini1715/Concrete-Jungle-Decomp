# Predator: Concrete Jungle decompilation

A matching decompilation of Predator: Concrete Jungle (Eurocom, 2005), Xbox (USA): the game's
code rewritten as C++ that compiles back to the same bytes as the retail executable, using the
compiler the game was built with (Visual C++ .NET 2003, 7.1).

Progress: **12,072 of the game's 13,795** functions written, across 244 source units (about 79% of the
game's code by size); **10,386** of them compile to exactly the original bytes. `status/status.csv`
lists every function and its state.

This is what the [PC port](https://github.com/Adolini1715/Concrete-Jungle-PC) builds on: matching
functions are compiled as native code in place of their machine-translated versions, and the
port's movement, camera and menu changes are edits to this C++.

## Layout

- `src/game`, `src/enginex`: the game and its engine (EngineX), one file per original object file.
- `include/`: class layouts and declarations.
- `symbols/`: function and class names, addresses and sizes.
- `status/`: per-function match state.
- `tools/`: the build and comparison tools (`tools/build.py`), Ghidra scripts, helpers.
- `docs/`: working notes.

## Checking a unit

You need your own copy of the game's `default.xbe` and the Visual C++ 7.1 compiler (not included).

    python3 tools/build.py check Actor        # compile src/**/Actor.cpp and compare with the game
    python3 tools/build.py progress           # totals

`PCJ_TOOLCHAINS` points at the folder holding the compiler (`vc71/Bin/cl.exe`); on Linux it runs
under Wine.

## Licence

See `NOTICE.md`: the tools and docs are MIT; the decompiled code carries no licence from this
project.

Built with AI assistance (Claude), directed by one person. Contributions and corrections welcome.
