#ifndef EXMEMORY_H
#define EXMEMORY_H
#include "types.h"

void *_EXAlloc(u32 size, u32 flags);
void *_EXRealloc(void *p, u32 size);
void EXFree(void *p);

// Global allocation goes through the EngineX heap.
inline void *__cdecl operator new(unsigned int size) { return _EXAlloc(size, 0); }
inline void *__cdecl operator new(unsigned int size, u32 flags) { return _EXAlloc(size, flags); }
inline void *__cdecl operator new[](unsigned int size) { return _EXAlloc(size, 0); }
inline void __cdecl operator delete(void *p) { EXFree(p); }
inline void __cdecl operator delete[](void *p) { EXFree(p); }
#endif

// ---- appended by the system-layer agent (verified against EXMemoryManager.obj / XBMalloc.obj) ----
#ifndef EXMEMORY_SYS_DECLS
#define EXMEMORY_SYS_DECLS
class EXMemHeap;
enum EXMcmds { EXMCMD_0 };  // TODO values: commands passed to block callbacks
enum EXMheap { EXMHEAP_DEFAULT = 0, EXMHEAP_LARGEDATA = 1 };
typedef signed char (__cdecl *EXMemFunc)(void *, EXMcmds);

void *_EXAllocPure(u32 size, u32 flags);
void *_EXAllocRel(u32 size, void **pRel, u32 flags);
void *_EXAllocFnc(u32 size, EXMemFunc pFunc, u32 flags);
void *_EXCache(u32 size, s16 time, void **pRel, u32 flags);
void *_EXCacheDel(u32 size, s16 time, s16 delTime, void **pRel, u32 flags);
u32 EXAllocMemSize(void *p);
void EXAllocSetTime(void *p, u16 time, u16 delTime);
void EXAllocSetOwner(void *p, void *pOwner);
void EXAllocSetReloc(void *p, s8 reloc);
void EXAllocSetFunc(void *p, EXMemFunc pFunc);
void EXAllocSetCache(void *p, u16 time, void **pRel);
void EXAllocSetCacheDel(void *p, u16 time, u16 delTime, void **pRel);

extern EXMemHeap *_SystemHeap;
extern EXMemHeap *_SystemHeapList[4];
extern "C" void _initalize_heap_memory();
extern "C" void _free_heap_memory();
#endif
