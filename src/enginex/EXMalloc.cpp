#include "EXMemHeap.h"
#include "EXBaseDisplay.h"
#include "EXFileSys.h"

u16 EXMemHeap::m_gAlignTbl[8] = { 0, 0x20, 0x40, 0x80, 0x400, 0x800, 0x2000, 0 };
const char *EXMemHeap::m_gAlignTxt[8] = { "default", "Align32", "Align64", "Align128", "Align1024", "Align2048", "Align8192", "Invalid" };

EXMemHeap _SDataHeap;
EXMemHeap _LDataHeap;
EXMemHeap *_SystemHeapList[4];
u8 *_SDataHeapMemory;
u8 *_LDataHeapMemory;
EXMemHeap *_SystemHeap;
s8 EXMemHeap::m_ForceCompactHeaps;
s8 EXMemHeap::m_EnableCompactHeaps;
void (__cdecl *EXMemHeap::m_PreCompactCallback)(u32);
void (__cdecl *EXMemHeap::m_PostCompactCallback)(u32);

// Free list for a block size: 4 (<=0x20) .. 1 (<=0x800), 0 above.
__forceinline int CalcFreeTbl(int size)
{
    if (size <= 0x20) return 4;
    if (size <= 0x80) return 3;
    if (size <= 0x200) return 2;
    return size <= 0x800;
}

template <class T> inline void Swap(T &a, T &b)
{
    T t = a;
    a = b;
    b = t;
}

void EXMemBlock::Init(u32 size)
{
    m_PrevSize &= ~0xf;
    m_pFunc = 0;
    m_pOwner = 0;
    m_Size = size | 1;
}

void EXMemBlock::Copy(EXMemBlock *p)
{
    m_PrevSize = (m_PrevSize & ~0xf) | (p->m_PrevSize & 0xf);
    m_Size = (m_Size & ~0xf) | (p->m_Size & 0xf);
    m_pFunc = p->m_pFunc;
    m_pOwner = p->m_pOwner;
    if (IsCache()) {
        m_pCache->m_pBlock = this;
        p->m_Size &= ~4;
    }
    if (m_pOwner) *(void **)m_pOwner = Data();
}

s8 EXMemBlock::LockMem(s8 lock)
{
    s8 old = m_Size & 8;
    if (lock) m_Size |= 8;
    else m_Size &= ~8;
    return old;
}

const char *EXMemBlock::AlignText() const
{
    return EXMemHeap::m_gAlignTxt[IsFree() ? 0 : Alignment()];
}

void EXMemHeap::Init(void *pMem, u32 size, int id, const char *pName)
{
    EXMemBlock *start = (EXMemBlock *)(((u32)pMem + 0xf) & ~0xf);
    u32 n = size >> 2;
    for (u32 i = 0; i < n; i++) ((u32 *)pMem)[i] = 0x51515151;
    m_HeapSize = size;
    m_HeapId = id;
    m_pMem = pMem;
    m_pStart = start;
    m_pName = pName;
    m_Unk64 = 1;
    _SystemHeapList[id] = this;
    u32 bsize = (size - 0x71) & ~0xf;
    start->m_Size = bsize | 1;
    start->m_pNextFree = 0;
    start->m_pPrevFree = (EXMemBlock *)&m_FreeList[0];
    start->m_PrevSize = 0;
    m_pEnd = start->NextLink();
    start->Add(m_pEnd);
    for (int i = 0; i < 5; i++) {
        m_FreeList[i].Init();
        m_pEnd[i].Init(0);
        m_pEnd[i].m_pPrevFree = (EXMemBlock *)&m_FreeList[i];
        m_pEnd[i].m_PrevSize = 0;
        m_pEnd[i].m_Size = 8;
        m_FreeList[i].m_pNext = (EXDListItem *)&m_pEnd[i];
    }
    m_pEnd->SetPrevSize(bsize);
    m_FreeList[0].m_pNext = (EXDListItem *)start;
}

EXMemBlock *EXMemHeap::CompactMemHiAlignMemory(EXMemBlock *p, u32 align, u32 size) const
{
    u8 *data = (u8 *)p->Data();
    u8 *a = (u8 *)((u32)(p->NextLink()->Size() + p->Size() - size + data + 0x10) & ~(align - 1));
    if (a != data) {
        if ((u32)(a - data) < 0x10) a += align;
        return (EXMemBlock *)a - 1;
    }
    return p;
}

EXMemBlock *EXMemHeap::AddFreeList(EXMemBlock *b)
{
    EXMemBlock *next = b->NextLink();
    if (next->IsFree()) {
        next->Remove();
        b->JoinNext();
    } else {
        next = 0;
    }
    b->m_Size = (b->m_Size & ~0xe) | 1;
    int idx = CalcFreeTbl(b->Size());
    EXDListItem *p;
    if (next && idx == CalcFreeTbl(next->Size())) {
        p = (EXDListItem *)next->m_pNextFree;
    } else {
        p = m_FreeList[idx].m_pNext;
        while ((EXDListItem *)b > p) p = p->m_pNext;
        if (!p) {
            m_FreeList[idx].Add((EXDListItem *)b);
            return b;
        }
    }
    p->Insert((EXDListItem *)b);
    return b;
}

EXMemBlock *EXMemHeap::_FindLargestFree() const
{
    EXMemBlock *end = m_pEnd;
    u32 bestSize = 0;
    EXMemBlock *best = 0;
    for (u32 i = 0; i < 5; i++) {
        EXMemBlock *b = (EXMemBlock *)m_FreeList[i].m_pNext;
        while (b) {
            if (b >= end) break;
            EXMemBlock *n = b->m_pNextFree;
            if (b->m_Size > bestSize) {
                best = b;
                bestSize = b->m_Size;
            }
            b = n;
        }
        if (best) return best;
    }
    return best;
}

EXMemBlock *EXMemHeap::_FindFirstFree(u32 size)
{
    EXMemBlock *b = m_pStart;
    EXMemBlock *end = m_pEnd;
    size >>= 4;
    for (; b < end;) {
        u32 s = b->m_Size >> 4;
        if ((b->m_Size & 1) && s >= size) return b;
        b = (EXMemBlock *)((u8 *)b + (s << 4) + 0x10);
    }
    return 0;
}

EXMemBlock *EXMemHeap::_FindLastFree(u32 size)
{
    EXMemBlock *b = m_pEnd;
    EXMemBlock *start = m_pStart;
    size >>= 4;
    for (; b != start; b = b->PrevLink()) {
        if ((b->m_Size & 1) && (b->m_Size >> 4) >= size) return b;
    }
    return 0;
}

EXMemBlock *EXMemHeap::_FindFree(u32 size) const
{
    int i = CalcFreeTbl(size);
    EXMemBlock *end = m_pEnd;
    size >>= 4;
    for (; i >= 0; i--) {
        for (EXMemBlock *b = (EXMemBlock *)m_FreeList[i].m_pNext; b && b < end; b = b->m_pNextFree) {
            if ((b->m_Size >> 4) >= size) return b;
        }
    }
    return 0;
}

void EXMemHeap::UpdateFirstLastFree()
{
    EXMemBlock *end = m_pEnd;
    for (int i = 0; i < 5; i++) {
        EXMemBlock *b = (EXMemBlock *)m_FreeList[i].m_pNext;
        while (b && b < end) {
            EXMemBlock *n = b->m_pNextFree;
            if (n && b > n) {
                b->Remove();
                n->Add(b);
            }
            b = n;
        }
    }
}

void EXMemHeap::ReorderFreeLists()
{
    for (int i = 0; i < 5; i++) {
        m_FreeList[i].Init();
        m_FreeList[i].m_pNext = (EXDListItem *)&m_pEnd[i];
    }
    EXMemBlock *start = m_pStart;
    for (EXMemBlock *b = m_pEnd->PrevLink(); b > start;) {
        EXMemBlock *prev = b->PrevLink();
        if (b->IsFree()) m_FreeList[CalcFreeTbl(b->Size())].Add((EXDListItem *)b);
        b = prev;
    }
}

void EXMemHeap::EnableAutoHeapCompact(s8 enable)
{
    m_EnableCompactHeaps = enable;
}

void EXMemHeap::ForceCompactHeaps()
{
    m_ForceCompactHeaps = 1;
}

void EXMemHeap::SetCompactCallbacks(void (__cdecl *pre)(u32), void (__cdecl *post)(u32))
{
    m_PreCompactCallback = pre;
    m_PostCompactCallback = post;
}

u32 EXMemHeap::FindLargestFree() const
{
    EXMemBlock *b = _FindLargestFree();
    if (b) return b->Size();
    return 0;
}

void EXMemHeap::UpdateHeapStats()
{
}

void EXMemHeap::RegisterMallocVars()
{
}

void EXMemHeap::RemoveCache(EXMemBlock *p) const
{
    EXMemCacheEntry *e = p->m_pCache;
    p->m_pFunc = e->m_pFunc;
    p->m_Size &= ~4;
    e->m_pBlock = 0;
    e->m_Time = 0;
    e->m_DelTime = 0;
}

EXMemBlock *EXMemBlock::Split(u32 size, s8 high)
{
    u32 total = Size();
    u32 left = total - size;
    if (left < 0x20) return this;
    left -= 0x10;
    if (high) Swap(size, left);
    SetSize(size);
    EXMemBlock *r = LinkAddr(size);
    r->m_Size = left | 1;
    r->SetPrevSize(size);
    LinkAddr(total)->SetPrevSize(left);
    return r;
}

EXMemBlock *EXMemHeap::SplitAlignMemory(EXMemBlock *p, u32 align) const
{
    u8 *data = (u8 *)p->Data();
    u8 *a = (u8 *)((u32)data & ~(align - 1));
    if (a != data) {
        a += align;
        if ((u32)(a - data) < 0x10) a += align;
        EXMemBlock *r = p->Split(a - data - 0x10, 0);
        if (r != p) return r;
    }
    p->Remove();
    return p;
}

EXMemBlock *EXMemHeap::SplitAlignMemory2(EXMemBlock *p, u32 align, u32 size) const
{
    u8 *data = (u8 *)p->Data();
    u8 *a = (u8 *)((u32)(p->Size() - size + data) & ~(align - 1));
    if (a != data) {
        if ((u32)(a - data) < 0x10) a += align;
        EXMemBlock *r = p->Split(a - data - 0x10, 0);
        if (r != p) return r;
    }
    p->Remove();
    return p;
}

EXMemBlock *EXMemHeap::AddFreeList2(EXMemBlock *b)
{
    if (b != m_pStart) {
        EXMemBlock *prev = b->PrevLink();
        if (prev && prev->IsFree()) {
            prev->Remove();
            prev->JoinNext();
            b = prev;
        }
    }
    return AddFreeList(b);
}

EXMemBlock *EXMemHeap::_sysFree(EXMemBlock *b)
{
    if (b->m_Size & 8) return b;
    if (b->IsCache()) RemoveCache(b);
    if (b->m_pOwner) *(void **)b->m_pOwner = 0;
    return AddFreeList2(b);
}

void EXMemHeap::EXFree(void *p)
{
    _sysFree((EXMemBlock *)p - 1);
}

void EXMemCacheArray::RemoveAll(EXMemHeap *pHeap)
{
    for (int i = m_nSize - 1; i >= 0; i--) {
        EXMemCacheBlock &blk = m_pData[i];
        for (int j = blk.m_Count - 1; j >= 0; j--) {
            EXMemCacheEntry *e = &blk.m_pEntries[j];
            EXMemBlock *b = e->m_pBlock;
            if (b) {
                b->m_pFunc = e->m_pFunc;
                b->m_Size &= ~4;
                pHeap->_sysFree(b);
            }
        }
        _SystemHeap->_sysFree((EXMemBlock *)blk.m_pEntries - 1);
    }
    EXDataArray<EXMemCacheBlock, const EXMemCacheBlock &>::RemoveAll();
}

void EXMemHeap::FlushCacheMem()
{
    for (int i = 0; i < 3; i++) m_CacheArray[i].RemoveAll(this);
}

void EXMemHeap::FlushAllCacheMem()
{
    for (int i = 0; i < 4; i++) {
        EXMemHeap *h = _SystemHeapList[i];
        if (h) h->FlushCacheMem();
    }
}

EXMemHeap::~EXMemHeap()
{
    FlushCacheMem();
}

EXMemCacheEntry *EXMemCacheArray::Get()
{
    int n = m_nSize;
    int i = n - 1;
    if (i > 0) {
        EXMemCacheBlock &blk = m_pData[i];
        if (blk.m_Count < 0x80) return &blk.m_pEntries[blk.m_Count++];
    }
    EXMemCacheBlock &nb = AddSpace(1);
    nb.m_pEntries = (EXMemCacheEntry *)_EXAlloc(0x800, 0);
    if (!nb.m_pEntries) for (;;) {}
    nb.m_Count = 1;
    return nb.m_pEntries;
}

void EXMemCacheArray::RemoveEntry(EXMemCacheEntry *e)
{
    EXMemCacheBlock &blk = m_pData[m_nSize - 1];
    EXMemCacheEntry *last = &blk.m_pEntries[blk.m_Count - 1];
    if (e != last) {
        *e = *last;
        if (e->m_pBlock) e->m_pBlock->m_pCache = e;
    }
    if (--blk.m_Count == 0) {
        _SystemHeap->_sysFree((EXMemBlock *)blk.m_pEntries - 1);
        SetSize(m_nSize - 1, -1);
    }
}

void EXMemHeap::AddCache(EXMemBlock *b, int a, u16 time, u16 delTime)
{
    EXMemCacheEntry *e;
    if (b->IsCache()) {
        e = b->m_pCache;
        if (e->m_Array == a) goto set;
        RemoveCache(b);
    }
    e = m_CacheArray[a].Get();
    e->m_Array = a;
    e->m_pFunc = b->m_pFunc;
    b->m_Size |= 4;
    b->m_pCache = e;
set:
    e->m_pBlock = b;
    e->m_Time = time;
    e->m_DelTime = delTime;
}

void EXMemHeap::AgeCacheMem()
{
    for (u32 a = 1; a <= 2; a++) {
        EXMemCacheArray &arr = m_CacheArray[a];
        for (int i = arr.GetUpperBound(); i >= 0; i--) {
            EXMemCacheBlock &blk = arr[i];
            for (int j = blk.m_Count - 1; j >= 0; j--) {
                EXMemCacheEntry *e = &blk.m_pEntries[j];
                int t = e->m_Time - 1;
                if (t > 0) {
                    e->m_Time = t;
                    continue;
                }
                EXMemBlock *b = e->m_pBlock;
                if (!b) {
                    arr.RemoveEntry(e);
                    continue;
                }
                if (a == 2 && e->m_DelTime != 0) {
                    u16 d = e->m_DelTime;
                    if (d == 0xffff) {
                        e->m_Time = 0;
                        AddCache(b, 0, 0, 0);
                    } else {
                        AddCache(b, 1, d, 0);
                    }
                    continue;
                }
                b->m_Size &= ~4;
                b->m_pFunc = e->m_pFunc;
                arr.RemoveEntry(e);
                _sysFree(b);
            }
        }
    }
}

EXMemBlock *EXMemHeap::_FindFreeCacheMem(u32 size)
{
    for (u32 a = 0; a < 2; a++) {
        EXMemCacheArray &arr = m_CacheArray[a];
        for (int i = arr.m_nSize - 1; i >= 0; i--) {
            EXMemCacheBlock &blk = arr.m_pData[i];
            for (int j = blk.m_Count - 1; j >= 0; j--) {
                EXMemCacheEntry *e = &blk.m_pEntries[j];
                EXMemBlock *b = e->m_pBlock;
                if (b && b->Size() >= size) {
                    b->m_Size &= ~4;
                    b->m_pFunc = e->m_pFunc;
                    m_CacheArray[a].RemoveEntry(e);
                    _sysFree(b);
                    goto search;
                }
            }
        }
    }
    m_CacheArray[0].RemoveAll(this);
search:
    return _FindFirstFree(size);
}

EXMemBlock *EXMemHeap::_FindFreeLastChance(u32 size, u32 flags)
{
    if (flags & 0x40) return 0;
    EXMemBlock *b = _FindFirstFree(size);
    if (!b) {
        b = _FindFreeCacheMem(size);
        if (!b) m_ForceCompactHeaps = 1;
    }
    return b;
}

EXMemBlock *EXMemHeap::AllocBlock(u32 size, u32 flags)
{
    u32 align = flags & 7;
    u32 alignSize = align;
    if (align) {
        alignSize = m_gAlignTbl[align];
        size = max(size, (u32)0x10);
    }
    size = (size + 0xf) & ~0xf;
    u32 need = size + alignSize * 2;
    EXMemBlock *b;
    if (flags & 0x10) b = _FindFirstFree(need);
    else if (flags & 8) b = _FindLastFree(need);
    else b = _FindFree(need);
    if (!b) {
        b = _FindFreeLastChance(need, flags);
        if (!b) return 0;
    }
    EXMemBlock *next = b->m_pNextFree;
    u32 nextSize = next->Size();
    u32 high = flags & 8;
    EXMemBlock *rest;
    if (alignSize) {
        if (high) b = SplitAlignMemory2(b, alignSize, size);
        else b = SplitAlignMemory(b, alignSize);
        rest = b->Split(size, 0);
    } else {
        b->Remove();
        rest = b->Split(size, high);
        if (high) Swap(rest, b);
    }
    b->m_pPrevFree = 0;
    b->m_pNextFree = 0;
    b->SetAlignFlags(flags & 0xf);
    b->SetFlags(0);
    if (rest != b) {
        rest->m_Size = (rest->m_Size & ~0xe) | 1;
        int idx = CalcFreeTbl(rest->Size());
        if ((nextSize | (u32)next) == 0 || idx != CalcFreeTbl(nextSize)) {
            next = (EXMemBlock *)m_FreeList[idx].m_pNext;
            while (rest > next) next = next->m_pNextFree;
            if (!next) {
                m_FreeList[idx].Add((EXDListItem *)rest);
                return b;
            }
        }
        ((EXDListItem *)next)->Insert((EXDListItem *)rest);
    }
    return b;
}

void *EXMemHeap::_sysAlloc(u32 size, u32 flags)
{
    EXMemBlock *b = AllocBlock(size, flags);
    void *p = 0;
    if (b) {
        b->SetDefaults(0, 0, 0);
        p = b->Data();
    }
    return p;
}

void *EXMemHeap::_EXAlloc(u32 size, u32 flags)
{
    EXMemBlock *b = AllocBlock(size, flags);
    void *p = 0;
    if (b) {
        b->SetDefaults(0, 0, 0);
        p = b->Data();
    }
    return p;
}

void *EXMemHeap::_EXAllocPure(u32 size, u32 flags)
{
    EXMemBlock *b = AllocBlock(size, flags);
    void *p = 0;
    if (b) {
        b->SetDefaults(0, 0, 0);
        p = b->Data();
    }
    return p;
}

void *EXMemHeap::_sysAllocFnc(u32 size, EXMemFunc f, u32 flags)
{
    EXMemBlock *b = AllocBlock(size, flags);
    void *p = 0;
    if (b) {
        b->SetDefaults(0, f, 0);
        p = b->Data();
    }
    return p;
}

void *EXMemHeap::_EXAllocFnc(u32 size, EXMemFunc f, u32 flags)
{
    EXMemBlock *b = AllocBlock(size, flags);
    void *p = 0;
    if (b) {
        b->SetDefaults(0, f, 0);
        p = b->Data();
    }
    return p;
}

void *EXMemHeap::_sysAllocRel(u32 size, void **pRel, u32 flags)
{
    EXMemBlock *b = AllocBlock(size, flags);
    void *p = 0;
    if (b) {
        b->SetDefaults(pRel, 0, 2);
        p = b->Data();
        *pRel = p;
    }
    return p;
}

void *EXMemHeap::_EXAllocRel(u32 size, void **pRel, u32 flags)
{
    EXMemBlock *b = AllocBlock(size, flags);
    void *p = 0;
    if (b) {
        b->SetDefaults(pRel, 0, 2);
        p = b->Data();
        *pRel = p;
    }
    return p;
}

void *EXMemHeap::_sysCacheDel(u32 size, u16 time, u16 delTime, void **pRel, u32 flags)
{
    EXMemBlock *b = AllocBlock(size, flags);
    void *p = 0;
    if (b) {
        b->SetDefaults(pRel, 0, 0);
        p = b->Data();
        if (pRel) *pRel = p;
        AddCache(b, 2, time, delTime);
    }
    return p;
}

void *EXMemHeap::_EXCacheDel(u32 size, u16 time, u16 delTime, void **pRel, u32 flags)
{
    return _sysCacheDel(size, time, delTime, pRel, flags);
}

void *EXMemHeap::_sysRealloc(void *p, u32 size)
{
    if (!p) return _sysAlloc(size, 0);
    EXMemBlock *b = (EXMemBlock *)p - 1;
    u32 ns = (size + 0xf) & ~0xf;
    u32 os = b->Size();
    if (ns == os) return p;
    if (ns < os) {
        EXMemBlock *r = b->Split(ns, 0);
        if (r != b) AddFreeList(r);
        return p;
    } else if (ns > os) {
        EXMemBlock *n = b->NextLink();
        if (n->IsFree() && ns < n->Size() + os + 0x10) {
            n->Remove();
            b->JoinNext();
            EXMemBlock *r = b->Split(ns, 0);
            if (r != b) AddFreeList(r);
            return p;
        }
        EXMemBlock *nb = AllocBlock(size, b->AlignFlags());
        if (!nb) return 0;
        b->CallFunction((EXMcmds)1);
        p = nb->Data();
        u32 *d = (u32 *)p;
        u32 *s = (u32 *)b->Data();
        for (u32 i = b->Size() >> 2; i != 0; i--) *d++ = *s++;
        nb->Copy(b);
        b->m_pOwner = 0;
        b->m_pFunc = 0;
        nb->CallFunction((EXMcmds)2);
        _sysFree(b);
        return p;
    }
    return p;
}

void *EXMemHeap::_EXRealloc(void *p, u32 size)
{
    return _sysRealloc(p, size);
}

void EXMemHeap::_sysSetAsCache(void *p, u16 time, u16 delTime, void **pRel)
{
    EXMemBlock *b = (EXMemBlock *)p - 1;
    AddCache(b, 2, time, delTime);
    if (pRel) *pRel = p;
    else if (b->m_pOwner) *(void **)b->m_pOwner = 0;
    b->m_pOwner = pRel;
}

EXMemBlock *EXMemHeap::CompactMemLo(EXMemBlock *b)
{
    EXMemBlock *end = m_pEnd;
    u32 size = b->Size();
    m_pEnd = b;
    EXMemBlock *nb = AllocBlock(size, (b->m_PrevSize & 7) | 0x50);
    m_pEnd = end;
    if (nb) {
        b->CallFunction((EXMcmds)1);
        memcpy(nb->Data(), b->Data(), size);
        nb->Copy(b);
        b->m_pOwner = 0;
        b->m_pFunc = 0;
        nb->CallFunction((EXMcmds)2);
        _sysFree(b);
        return nb;
    }
    EXMemBlock *prev = b->PrevLink();
    if (!prev || !prev->IsFree()) return 0;
    u32 align = m_gAlignTbl[b->m_PrevSize & 7];
    if (prev->Size() < align) return 0;
    EXMemBlock hdr = *b;
    b->CallFunction((EXMcmds)1);
    prev->JoinNext();
    EXMemBlock *n = prev->NextLink();
    if (n->IsFree()) {
        n->Remove();
        prev->JoinNext();
    }
    nb = prev;
    if (align) nb = SplitAlignMemory(prev, align);
    else prev->Remove();
    if (nb != b) memcpy(nb->Data(), b->Data(), size);
    EXMemBlock *r = nb->Split(size, 0);
    if (r != nb) {
        r->m_Size = (r->m_Size & ~0xe) | 1;
        m_FreeList[CalcFreeTbl(r->Size())].Add((EXDListItem *)r);
    }
    nb->Copy(&hdr);
    nb->CallFunction((EXMcmds)2);
    return nb;
}

EXMemBlock *EXMemHeap::CompactMemHi(EXMemBlock *b)
{
    EXMemBlock *start = m_pStart;
    u32 size = b->Size();
    m_pStart = b;
    EXMemBlock *nb = AllocBlock(size, (b->m_PrevSize & 7) | 0x48);
    m_pStart = start;
    if (nb) {
        b->CallFunction((EXMcmds)1);
        memcpy(nb->Data(), b->Data(), size);
        nb->Copy(b);
        b->m_pOwner = 0;
        b->m_pFunc = 0;
        nb->CallFunction((EXMcmds)2);
        _sysFree(b);
        return nb;
    }
    EXMemBlock *n = b->NextLink();
    if (!n->IsFree()) return 0;
    u32 align = m_gAlignTbl[b->m_PrevSize & 7];
    EXMemBlock *dst = CompactMemHiAlignMemory(b, align, size);
    if (b == dst) return 0;
    n->Remove();
    b->JoinNext();
    b->CallFunction((EXMcmds)1);
    memmove(dst->Data(), b->Data(), size);
    nb = SplitAlignMemory2(b, align, size);
    nb->Copy(b);
    nb->CallFunction((EXMcmds)2);
    AddFreeList2(b);
    EXMemBlock *r = nb->Split(size, 0);
    if (r != nb) AddFreeList(r);
    return nb;
}

s8 EXMemHeap::Compact()
{
    s8 moved = 0;
    EXMemBlock *end = m_pEnd;
    for (EXMemBlock *b = m_pStart->NextLink(); b < end; b = b->NextLink()) {
        if ((b->m_Size & 2) && !(b->m_PrevSize & 8)) {
            EXMemBlock *n = CompactMemLo(b);
            if (n) {
                b = n;
                moved = 1;
            }
        }
    }
    EXMemBlock *start = m_pStart;
    for (EXMemBlock *b = m_pEnd->PrevLink(); b > start; b = b->PrevLink()) {
        if ((b->m_Size & 2) && (b->m_PrevSize & 8)) {
            EXMemBlock *n = CompactMemHi(b);
            if (n) {
                b = n;
                moved = 1;
            }
        }
    }
    return moved;
}

void EXMemHeap::Update()
{
    AgeCacheMem();
    UpdateFirstLastFree();
}

void EXMemHeap::CompactHeaps(u32 n)
{
    EXBaseDisplay::GetDisplay()->WaitDrawSync();
    EXBaseDisplay::GetDisplay()->FlushAllWindowMem();
    for (int i = 0; i < 4; i++) {
        EXMemHeap *h = _SystemHeapList[i];
        if (h) h->FlushCacheMem();
    }
    if (m_PreCompactCallback) m_PreCompactCallback(n);
    if (_SystemHeapList[1]->Compact()) {
        EXFileSys::GetFileSys()->UpdateLoadedFiles(1);
        if (m_PostCompactCallback) m_PostCompactCallback(n);
        __asm wbinvd
    }
    m_ForceCompactHeaps = 0;
}

void *EXMemHeap::_EXCache(u32 size, u16 time, void **pRel, u32 flags)
{
    return _sysCacheDel(size, time, 0xffff, pRel, flags);
}

void EXMemHeap::UpdateAllHeaps(u32 n, int count)
{
    for (int i = count; i > 0; i--) {
        for (int j = 0; j < 4; j++) {
            EXMemHeap *h = _SystemHeapList[j];
            if (h) h->Update();
        }
    }
    if (m_ForceCompactHeaps && m_EnableCompactHeaps) CompactHeaps(n);
}
