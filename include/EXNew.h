#ifndef EXNEW_H
#define EXNEW_H
// Placement new (emitted copies in the map as `operator new(unsigned int, void *)`).
inline void *__cdecl operator new(unsigned int, void *p) { return p; }
#endif
