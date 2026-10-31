#ifndef EXMEMBLOCK_H
#define EXMEMBLOCK_H
#include "EXMemory.h"

class EXMemBlock;

// One cached allocation (16 bytes). m_Array: which of the heap's cache arrays holds it.
struct EXMemCacheEntry {
    u16 m_Time;             // 0x00 frames left
    u16 m_DelTime;          // 0x02 lifetime once moved to the second-level cache (0xffff: forever)
    u8 m_04;
    s8 m_Array;             // 0x05
    u16 m_06;
    EXMemBlock *m_pBlock;   // 0x08
    EXMemFunc m_pFunc;      // 0x0c the block's own callback while it is cached
};
typedef EXMemCacheEntry EXMemCacheBlockEntry;

// 16-byte header in front of every heap allocation.
// m_Size: size (16-aligned) | flags (1 free, 2 moveable/reloc, 4 cache, 8 locked).
// m_PrevSize: previous block size | align flags (bits 0-2 alignment index, 8 allocated high).
// Free blocks link through the first two words (prev, next) into the heap's free lists.
class EXMemBlock {
public:
    void SetFlags(int f) { m_Size = (m_Size & ~0xf) | f; }
    void SetFlag(u16 f) { m_Size |= f; }
    void ClrFlag(u16 f) { m_Size &= ~(u32)f; }
    void SetOwner(void *p) { m_pOwner = p; }
    void *Owner() { return m_pOwner; }
    void SetFunc(EXMemFunc f)
    {
        if (m_Size & 4) m_pCache->m_pFunc = f;
        else m_pFunc = f;
    }
    u32 Size() const { return m_Size & ~0xf; }
    s8 LockMem(s8 lock);
    const char *AlignText() const;
private:
    void *Data() const { return (void *)(this + 1); }
    EXMemFunc Func() const { return IsCache() ? m_pCache->m_pFunc : m_pFunc; }
    __forceinline s8 CallFunction(EXMcmds cmd) const
    {
        EXMemFunc f = IsCache() ? m_pCache->m_pFunc : m_pFunc;
        return f ? f(Data(), cmd) : 0;
    }
    void SetAlignFlags(u32 f) { m_PrevSize = (m_PrevSize & ~0xf) | f; }
    u32 AlignFlags() const { return m_PrevSize & 0xf; }
    u32 Alignment() const { return m_PrevSize & 7; }
    u32 IsFree() const { return m_Size & 1; }
    u32 IsMoveable() const { return m_Size & 2; }
    u32 IsCache() const { return m_Size & 4; }
    u32 Flags() const { return m_Size & 0xf; }
    void SetPrevSize(int s) { m_PrevSize = (m_PrevSize & 0xf) | s; }
    void SetSize(int s) { m_Size = (m_Size & 0xf) | s; }
    void SetDefaults(void *pOwner, EXMemFunc f, u32 flags)
    {
        m_pOwner = pOwner;
        m_pFunc = f;
        m_Size = (m_Size & ~0xf) | flags;
    }
    EXMemBlock *PrevLink() const { return (EXMemBlock *)((u8 *)this - (m_PrevSize & ~0xf)) - 1; }
    EXMemBlock *NextLink() const { return (EXMemBlock *)((u8 *)this + (m_Size & ~0xf) + 0x10); }
    EXMemBlock *LinkAddr(u32 off) const { return (EXMemBlock *)((u8 *)this + off + 0x10); }
    void Init(u32 size);
    void Copy(EXMemBlock *p);
    void Add(EXMemBlock *p)
    {
        EXMemBlock *n = m_pNextFree;
        p->m_pPrevFree = this;
        p->m_pNextFree = n;
        if (n) n->m_pPrevFree = p;
        m_pNextFree = p;
    }
    void Remove()
    {
        EXMemBlock *n = m_pNextFree;
        EXMemBlock *p = m_pPrevFree;
        p->m_pNextFree = n;
        n->m_pPrevFree = p;
    }
    __forceinline void JoinNext()
    {
        u32 sz = Size();
        u32 s = LinkAddr(sz)->Size() + sz + 0x10;
        SetSize(s);
        LinkAddr(s)->SetPrevSize(s);
    }
    EXMemBlock *Split(u32 size, s8 high);

    union {
        void *m_pOwner;             // allocated: where the data pointer is kept (relocatable blocks)
        EXMemBlock *m_pPrevFree;
    };
    union {
        EXMemFunc m_pFunc;          // allocated: callback (or the cache entry when flag 4 is set)
        EXMemCacheEntry *m_pCache;
        EXMemBlock *m_pNextFree;
    };
    u32 m_Size;
    u32 m_PrevSize;
    friend class EXMemHeap;
    friend class EXMemCacheArray;
    friend class EXBaseResourceWnd;
};
#endif
