#include "EXMemHeap.h"
#include "EXMemLock.h"

// EXGetHeap (EXMemHeap.h) is __forceinline: every caller here inlines it, which plain `inline`
// does not achieve (VC 7.1 will not inline the loop); the out-of-line COMDAT copy is still emitted.

EXMemLock::EXMemLock(void *p, s8 lock)
{
    m_pBlock = (EXMemBlock *)p - 1;
    if (m_pBlock) m_WasLocked = m_pBlock->LockMem(lock);
    else m_WasLocked = 0;
}

EXMemLock::~EXMemLock()
{
    m_pBlock->LockMem(m_WasLocked);
}

void *_EXAlloc(u32 size, u32 flags)
{
    return _SystemHeap->_sysAlloc(size, flags);
}

void *_EXAllocPure(u32 size, u32 flags)
{
    return _SystemHeap->_sysAlloc(size, flags);
}

void *_EXAllocRel(u32 size, void **pRel, u32 flags)
{
    return _SystemHeap->_sysAllocRel(size, pRel, flags);
}

void *_EXAllocFnc(u32 size, EXMemFunc pFunc, u32 flags)
{
    return _SystemHeap->_sysAllocFnc(size, pFunc, flags);
}

void *_EXRealloc(void *p, u32 size)
{
    EXMemHeap *h;
    if (p) h = EXGetHeap(p);
    else h = _SystemHeap;
    return h->_sysRealloc(p, size);
}

void *_EXCache(u32 size, s16 time, void **pRel, u32 flags)
{
    return _SystemHeap->_sysCacheDel(size, time, 0xffff, pRel, flags);
}

void *_EXCacheDel(u32 size, s16 time, s16 delTime, void **pRel, u32 flags)
{
    return _SystemHeap->_sysCacheDel(size, time, delTime, pRel, flags);
}

void EXFree(void *p)
{
    if (p) {
        EXMemHeap *h = EXGetHeap(p);
        if (!h) for (;;) {}
        h->_sysFree((EXMemBlock *)p - 1);
    }
}

u32 EXAllocMemSize(void *p)
{
    if (!p) return 0;
    return ((EXMemBlock *)p - 1)->Size();
}

void EXAllocSetTime(void *p, u16 time, u16 delTime)
{
    EXGetHeap(p)->_sysUpdateCache(p, time, delTime);
}

void EXAllocSetOwner(void *p, void *pOwner)
{
    ((EXMemBlock *)p - 1)->SetOwner(pOwner);
    if (pOwner) *(void **)pOwner = p;
}

void EXAllocSetReloc(void *p, s8 reloc)
{
    EXMemBlock *b = (EXMemBlock *)p - 1;
    if (reloc) b->SetFlag(2);
    else b->ClrFlag(2);
}

void EXAllocSetFunc(void *p, EXMemFunc pFunc)
{
    ((EXMemBlock *)p - 1)->SetFunc(pFunc);
}

void EXAllocSetCache(void *p, u16 time, void **pRel)
{
    EXGetHeap(p)->_sysSetAsCache(p, time, 0xffff, pRel);
}

void EXAllocSetCacheDel(void *p, u16 time, u16 delTime, void **pRel)
{
    EXGetHeap(p)->_sysSetAsCache(p, time, delTime, pRel);
}
