#ifndef EXALLOCATOR_H
#define EXALLOCATOR_H
#include "types.h"
#include "EXArray.h"
#include "EXMemory.h"

struct EXAllocatorBlock {
    u32 m_Unk00;
    void *m_pMem;
};

class EXAllocatorBlockArray : public EXDataArray<EXAllocatorBlock, const EXAllocatorBlock &> {
public:
    EXAllocatorBlockArray() : EXDataArray<EXAllocatorBlock, const EXAllocatorBlock &>(0, 16) {}
    ~EXAllocatorBlockArray() {}
};

// Block allocator for fixed-size objects (sizeof 0x14).
template <class T> class EXAllocator {
public:
    EXAllocator(unsigned int blockSize) { m_BlockSize = blockSize; }
    ~EXAllocator()
    {
        for (int i = 0; i < m_Blocks.Size(); i++) EXFree(m_Blocks[i].m_pMem);
    }
    // Get/Remove appended by the item-layer agent (verified on EXAllocator<_EXItemAnimatorAllocator>,
    // EXItemAnimator.obj). Each block holds m_BlockSize objects followed by a u16 free-index stack;
    // m_Unk00 is the number of free entries.
    // NEAR (EXAllocator<_EXItemAnimatorAllocator>::Get): the original increments m_nSize in memory
    // before forming the block pointer; ours loads/stores it around the _EXAlloc call (scheduling).
    T *Get()
    {
        int i;
        for (i = m_Blocks.m_nSize - 1; i >= 0; i--) {
            EXAllocatorBlock &b = m_Blocks.m_pData[i];
            if (b.m_Unk00) {
                T *pData = (T *)b.m_pMem;
                u16 *pFree = (u16 *)(pData + m_BlockSize);
                b.m_Unk00--;
                return &pData[pFree[b.m_Unk00]];
            }
        }
        int n = m_Blocks.m_nSize;
        int nNew = n + 1;
        if (nNew > m_Blocks.m_nMaxSize) {
            // EXDataArray::GrowArray (protected), inlined
            int nMax = m_Blocks.m_nMaxSize + m_Blocks.m_nGrowBy;
            while (nNew > 0 && nMax < nNew) nMax += m_Blocks.m_nGrowBy;
            m_Blocks.m_pData = (EXAllocatorBlock *)_EXRealloc(m_Blocks.m_pData, nMax * sizeof(EXAllocatorBlock));
            m_Blocks.m_nMaxSize = nMax;
        }
        m_Blocks.m_nSize++;
        EXAllocatorBlock &b = m_Blocks.m_pData[n];
        b.m_pMem = _EXAlloc(m_BlockSize * (sizeof(T) + sizeof(u16)), 0);
        T *pData = (T *)b.m_pMem;
        u16 *pFree = (u16 *)(pData + m_BlockSize);
        for (i = 0; i < (int)m_BlockSize - 1; i++) pFree[i] = (u16)i;
        b.m_Unk00 = m_BlockSize - 1;
        return &pData[m_BlockSize - 1];
    }
    void Remove(const T *p)
    {
        int blk = -1;
        int idx;
        for (int i = 0; i < m_Blocks.m_nSize; i++) {
            T *pData = (T *)m_Blocks.m_pData[i].m_pMem;
            if (p >= pData && p < pData + m_BlockSize) {
                blk = i;
                idx = p - pData;
                break;
            }
        }
        if (blk < 0) for (;;) {}
        EXAllocatorBlock &b = m_Blocks.m_pData[blk];
        ((u16 *)((T *)b.m_pMem + m_BlockSize))[b.m_Unk00] = (u16)idx;
        b.m_Unk00++;
        if (b.m_Unk00 == m_BlockSize) {
            EXFree(b.m_pMem);
            m_Blocks.RemoveAt(blk, 1);
        }
    }
    EXAllocatorBlockArray m_Blocks;  // 0x00
    unsigned int m_BlockSize;        // 0x10
};

// Allocator that also runs T's constructor/destructor (EXMenu.obj: EXClassAllocator<EXMenuItem>).
template <class T> class EXClassAllocator : public EXAllocator<T> {
public:
    EXClassAllocator(unsigned int blockSize) : EXAllocator<T>(blockSize) {}
    ~EXClassAllocator() {}
    T *Get()
    {
        T *p = EXAllocator<T>::Get();
        if (p) new ((void *)p) T;
        return p;
    }
    void Remove(const T *p)
    {
        ((T *)p)->~T();
        EXAllocator<T>::Remove(p);
    }
};
#endif
