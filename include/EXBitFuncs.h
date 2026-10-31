#ifndef EXBITFUNCS_H
#define EXBITFUNCS_H
#include "types.h"

// Generic bit helpers (instances u16 / u32 emitted in EXWidget.obj).
template <class T> inline void EXBitClear(T &v, T bits) { v &= ~bits; }
template <class T> inline void EXBitSet(T &v, T bits) { v |= bits; }
template <class T> inline T EXBitTest(T v, T bits) { return v & bits; }
#endif
