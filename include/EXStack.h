#ifndef EXSTACK_H
#define EXSTACK_H
#include "types.h"
#include "EXMemory.h"
#include "EXList.h"
#include "EXArray.h"

// One allocation of m_BlockSize stack items, split between a used and a free list (sizeof 0x14).
struct EXStackBlock {
    void *m_pMem;    // 0x00 item storage
    EXDList m_Used;  // 0x04
    EXDList m_Free;  // 0x0c
};

class EXStackBlockArray : public EXDataArray<EXStackBlock, const EXStackBlock &> {
public:
    EXStackBlockArray() {}
    ~EXStackBlockArray() {}
};

// Block-allocated stack (sizeof 0x1c). Emitted instances: App.obj (XGameState), FEObject.obj /
// FEHud.obj (unsigned int), FELayout.obj / FEWnd.obj (FEObject *, EXRect).
template <class T> class EXStack {
public:
    // No user-declared destructor: the implicit one is non-trivial exactly when T has a destructor,
    // which decides whether `delete` of a block tests for null (EXRect) or not (unsigned int).
    class EXStackItem : public EXDListItem {
    public:
        T m_Data;  // 0x08
    };

    EXStack(unsigned int nBlockSize)
    {
        m_BlockSize = nBlockSize;
        m_Unk14 = 0;
        m_pIter = 0;
    }
    ~EXStack()
    {
        for (int i = 0; i < m_Blocks.Size(); i++) delete (EXStackItem *)m_Blocks[i].m_pMem;
    }

    void Push(T &v)
    {
        if (m_Blocks.Size()) {
            EXStackBlock *pBlock = &m_Blocks[m_Blocks.Size() - 1];
            if (pBlock->m_Free.Head()) {
                EXStackItem *pItem = (EXStackItem *)pBlock->m_Free.RemoveHead();
                pBlock->m_Used.AddTail(pItem);
                pItem->m_Data = v;
                return;
            }
        }
        EXStackBlock &block = m_Blocks.AddSpace(1);
        block.m_Used.Init();
        block.m_Free.Init();
        EXStackItem *pItems = (EXStackItem *)_EXAlloc(m_BlockSize * sizeof(EXStackItem), 0);
        block.m_pMem = pItems;
        for (int i = 1; i < (int)m_BlockSize; i++) block.m_Free.AddTail(&pItems[i]);
        block.m_Used.AddTail(pItems);
        pItems->m_Data = v;
    }
    signed char Pop(T &v)
    {
        if (!m_Blocks.Size()) return 0;
        EXStackBlock *pBlock = &m_Blocks[m_Blocks.Size() - 1];
        EXStackItem *pItem = (EXStackItem *)pBlock->m_Used.RemoveTail();
        pBlock->m_Free.AddTail(pItem);
        v = pItem->m_Data;
        if (!pBlock->m_Used.Head()) {
            delete (EXStackItem *)pBlock->m_pMem;
            m_Blocks.RemoveAt(m_Blocks.Size() - 1, 1);
        }
        m_pIter = 0;
        return 1;
    }
    signed char Peek(T &v)
    {
        if (!m_Blocks.Size()) return 0;
        v = ((EXStackItem *)m_Blocks[m_Blocks.Size() - 1].m_Used.Tail())->m_Data;
        return 1;
    }

    // Removes the item p points at (from any block); returns the item after it (App.obj XGameState copy).
    T *Remove(T *p)
    {
        int idx = -1;
        for (int i = 0; i < m_Blocks.Size(); i++) {
            EXStackItem *pMem = (EXStackItem *)m_Blocks[i].m_pMem;
            if ((void *)p >= pMem && (void *)p <= pMem + m_BlockSize) {
                idx = i;
                break;
            }
        }
        if (idx < 0) return 0;
        EXStackItem *pItem = (EXStackItem *)((char *)p - 8);
        EXStackBlock &block = m_Blocks[idx];
        m_pIter = (EXStackItem *)pItem->Next();
        m_Unk14 = idx;
        if (!m_pIter && ++m_Unk14 < m_Blocks.Size()) m_pIter = (EXStackItem *)m_Blocks[m_Unk14].m_Used.Head();
        block.m_Used.Remove(pItem);
        block.m_Free.AddTail(*pItem);
        if (!block.m_Used.Head()) {
            delete (EXStackItem *)block.m_pMem;
            m_Blocks.RemoveAt(idx, 1);
        }
        return m_pIter ? &m_pIter->m_Data : 0;
    }

    // Iteration from the bottom of the stack (appended by the app-layer agent; App.obj XGameState copies).
    T *GetBaseItem()
    {
        m_Unk14 = 0;
        m_pIter = 0;
        if (!m_Blocks.Size()) return 0;
        m_pIter = (EXStackItem *)m_Blocks[0].m_Used.Head();
        return &m_pIter->m_Data;
    }
    T *GetNextItem()
    {
        if (!m_pIter) return 0;
        m_pIter = (EXStackItem *)m_pIter->Next();
        if (!m_pIter && ++m_Unk14 < m_Blocks.Size()) m_pIter = (EXStackItem *)m_Blocks[m_Unk14].m_Used.Head();
        return m_pIter ? &m_pIter->m_Data : 0;
    }

    EXStackBlockArray m_Blocks;  // 0x00
    unsigned int m_BlockSize;    // 0x10 items per block
    int m_Unk14;                 // 0x14 block index of the GetBaseItem/GetNextItem cursor
    EXStackItem *m_pIter;        // 0x18 GetBaseItem/GetNextItem cursor
};
#endif
