#ifndef EXRUNFIRST_H
#define EXRUNFIRST_H
#include "EXMemory.h"

// Static object whose constructor brings up the heaps before any other static initialiser allocates.
class EXRunFirst {
public:
    EXRunFirst() { if (!_SystemHeap) _initalize_heap_memory(); }
};

extern char g_EXRunFirstVar;
extern EXRunFirst g_EXRunFirst;
#endif
