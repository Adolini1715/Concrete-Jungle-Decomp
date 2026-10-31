#ifndef EXMEMHEAP_H
#define EXMEMHEAP_H
#include "EXMemBlock.h"
#include "EXArray.h"
#include "EXList.h"

// Up to 0x80 cache entries per block.
struct EXMemCacheBlock {
    int m_Count;
    EXMemCacheEntry *m_pEntries;
};

class EXMemCacheArray : public EXDataArray<EXMemCacheBlock, const EXMemCacheBlock &> {
public:
    EXMemCacheArray() {}
    ~EXMemCacheArray() {}
    EXMemCacheEntry *Get();
    void RemoveEntry(EXMemCacheEntry *e);
    void RemoveAll(EXMemHeap *pHeap);
};

// sizeof 0x88 (from _SDataHeap/_LDataHeap spacing).
class EXMemHeap {
public:
    EXMemHeap() {}
    ~EXMemHeap();
    void Init(void *pMem, u32 size, int id, const char *pName);
    EXMemBlock *MemStart() const { return m_pStart; }
    EXMemBlock *MemEnd() const { return m_pEnd; }
    void *MemAlloc() const { return m_pMem; }
    u32 HeapId() const { return m_HeapId; }
    const char *Name() const { return m_pName; }
    void _sysUpdateCache(void *p, u16 time, u16 delTime) { AddCache((EXMemBlock *)p - 1, 2, time, delTime); }
    static void EnableAutoHeapCompact(s8 enable);
    static void ForceCompactHeaps();
    static void SetCompactCallbacks(void (__cdecl *pre)(u32), void (__cdecl *post)(u32));
    u32 FindLargestFree() const;
    static void UpdateHeapStats();
    static void RegisterMallocVars();
    EXMemBlock *_sysFree(EXMemBlock *p);
    void EXFree(void *p);
    static void FlushAllCacheMem();
    void *_sysAlloc(u32 size, u32 flags);
    void *_EXAlloc(u32 size, u32 flags);
    void *_EXAllocPure(u32 size, u32 flags);
    void *_sysAllocFnc(u32 size, EXMemFunc f, u32 flags);
    void *_EXAllocFnc(u32 size, EXMemFunc f, u32 flags);
    void *_sysAllocRel(u32 size, void **pRel, u32 flags);
    void *_EXAllocRel(u32 size, void **pRel, u32 flags);
    void *_sysCacheDel(u32 size, u16 time, u16 delTime, void **pRel, u32 flags);
    void *_EXCacheDel(u32 size, u16 time, u16 delTime, void **pRel, u32 flags);
    void *_sysRealloc(void *p, u32 size);
    void *_EXRealloc(void *p, u32 size);
    void _sysSetAsCache(void *p, u16 time, u16 delTime, void **pRel);
    void Update();
    static void CompactHeaps(u32 n);
    void *_EXCache(u32 size, u16 time, void **pRel, u32 flags);
    static void UpdateAllHeaps(u32 n, int b);
    u32 RtlSizeHeap() const { return m_HeapSize; }
    u32 Debug_Stats(int n) const { return m_Stats[n]; }
    static u16 m_gAlignTbl[8];
    static const char *m_gAlignTxt[8];
protected:
    static s8 m_ForceCompactHeaps;
    static s8 m_EnableCompactHeaps;
    static void (__cdecl *m_PreCompactCallback)(u32);
    static void (__cdecl *m_PostCompactCallback)(u32);
private:
    void AddCache(EXMemBlock *p, int n, u16 time, u16 delTime);
    void RemoveCache(EXMemBlock *p) const;
    EXMemBlock *CompactMemHiAlignMemory(EXMemBlock *p, u32 align, u32 size) const;
    EXMemBlock *AddFreeList(EXMemBlock *p);
    EXMemBlock *AddFreeList2(EXMemBlock *p);
    EXMemBlock *_FindLargestFree() const;
    EXMemBlock *_FindFirstFree(u32 size);
    EXMemBlock *_FindLastFree(u32 size);
    EXMemBlock *_FindFree(u32 size) const;
    EXMemBlock *_FindFreeCacheMem(u32 size);
    EXMemBlock *_FindFreeLastChance(u32 size, u32 flags);
    void UpdateFirstLastFree();
    void ReorderFreeLists();
    EXMemBlock *SplitAlignMemory(EXMemBlock *p, u32 align) const;
    EXMemBlock *SplitAlignMemory2(EXMemBlock *p, u32 align, u32 size) const;
    void FlushCacheMem();
    void AgeCacheMem();
    EXMemBlock *AllocBlock(u32 size, u32 flags);
    EXMemBlock *CompactMemLo(EXMemBlock *p);
    EXMemBlock *CompactMemHi(EXMemBlock *p);
    s8 Compact();

    void *m_pMem;                   // 0x00 raw allocation
    EXMemBlock *m_pStart;           // 0x04
    EXMemBlock *m_pEnd;             // 0x08 first of five locked end markers, one per free list
    EXDListItem m_FreeList[5];      // 0x0c by size: >0x800, <=0x800, <=0x200, <=0x80, <=0x20; address order
    EXMemCacheArray m_CacheArray[3];    // 0x34
    u8 m_Unk64;                     // 0x64
    u32 m_HeapId;                   // 0x68
    u32 m_HeapSize;                 // 0x6c
    const char *m_pName;            // 0x70
    u32 m_Stats[5];                 // 0x74 UpdateHeapStats: [1] used, [2] cached, [3] free (EXMemBarWnd)
    friend class EXMemCacheArray;
};

// Heap containing p, or 0.
__forceinline EXMemHeap *EXGetHeap(const void *p)
{
    for (int i = 0; i < 4; i++) {
        EXMemHeap *h = _SystemHeapList[i];
        if (h && p >= h->MemStart() && p <= h->MemEnd()) return h;
    }
    return 0;
}

extern EXMemHeap _SDataHeap;
extern EXMemHeap _LDataHeap;
extern u8 *_SDataHeapMemory;
extern u8 *_LDataHeapMemory;
#endif
