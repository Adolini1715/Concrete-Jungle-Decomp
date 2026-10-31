#include "EXList.h"

// ---- EXDListItem
void EXDListItem::Add(EXDListItem *pItem)
{
    EXDListItem *pNext = m_pNext;
    pItem->m_pPrev = this;
    pItem->m_pNext = pNext;
    if (pNext) pNext->m_pPrev = pItem;
    m_pNext = pItem;
}

void EXDListItem::Insert(EXDListItem *pItem)
{
    EXDListItem *pPrev = m_pPrev;
    pItem->m_pPrev = pPrev;
    pItem->m_pNext = this;
    if (pPrev) pPrev->m_pNext = pItem;
    m_pPrev = pItem;
}

void EXDListItem::Remove()
{
    EXDListItem *pPrev = m_pPrev;
    EXDListItem *pNext = m_pNext;
    if (pPrev) {
        pPrev->m_pNext = pNext;
        m_pPrev = 0;
    }
    if (pNext) {
        pNext->m_pPrev = pPrev;
        m_pNext = 0;
    }
}

s8 EXDListItem::Apply(void *pFunc, void *pData)
{
    EXDListItem *p = this;
    while (p) {
        EXDListItem *pNext = p->m_pNext;
        if (!((EXListApplyFunc)pFunc)(p, pData)) return 0;
        p = pNext;
    }
    return 1;
}

s8 EXDListItem::ApplyBack(void *pFunc, void *pData)
{
    EXDListItem *p = this;
    while (p) {
        EXDListItem *pPrev = p->m_pPrev;
        if (!((EXListApplyFunc)pFunc)(p, pData)) return 0;
        p = pPrev;
    }
    return 1;
}

// ---- EXDList
void EXDList::AddHead(EXDListItem *pItem)
{
    EXDListItem *pHead = m_pHead;
    pItem->m_pPrev = 0;
    pItem->m_pNext = pHead;
    if (pHead) pHead->m_pPrev = pItem;
    m_pHead = pItem;
    if (!m_pTail) m_pTail = pItem;
}

void EXDList::AddTail(EXDListItem *pItem)
{
    EXDListItem *pTail = m_pTail;
    pItem->m_pNext = 0;
    pItem->m_pPrev = pTail;
    if (pTail) pTail->m_pNext = pItem;
    m_pTail = pItem;
    if (!m_pHead) m_pHead = pItem;
}

// NEAR: 64 bytes differ, register allocation (orig keeps the head in eax, item in edx).
void EXDList::Insert(EXDListItem *pPos, EXDListItem *pItem)
{
    EXDListItem *pHead = m_pHead;
    if (!pHead) {
        pItem->m_pPrev = pHead;
        pItem->m_pNext = pHead;
        m_pHead = pItem;
        if (!m_pTail) m_pTail = pItem;
        return;
    }
    EXDListItem *pPrev = pPos->m_pPrev;
    pItem->m_pNext = pPos;
    pItem->m_pPrev = pPrev;
    if (pPos == pHead) m_pHead = pItem;
    else pPrev->m_pNext = pItem;
    pPos->m_pPrev = pItem;
}

void EXDList::Remove(EXDListItem *pItem)
{
    if (pItem == m_pHead) m_pHead = pItem->m_pNext;
    if (pItem == m_pTail) m_pTail = pItem->m_pPrev;
    pItem->Remove();
}

EXDListItem *EXDList::RemoveHead()
{
    EXDListItem *p = m_pHead;
    if (p) Remove(p);
    return p;
}

EXDListItem *EXDList::RemoveTail()
{
    EXDListItem *p = m_pTail;
    if (p) Remove(p);
    return p;
}

s8 EXDList::IsMember(const EXDListItem *pItem) const
{
    for (EXDListItem *p = m_pHead; p; p = p->m_pNext)
        if (p == pItem) return 1;
    return 0;
}

u32 EXDList::Count() const
{
    u32 n = 0;
    for (EXDListItem *p = m_pHead; p; p = p->m_pNext) n++;
    return n;
}

// ---- EXLDListItem / EXLDList
void EXLDListItem::Add(EXLDListItem *pItem, EXLDList *pList)
{
    EXLDListItem *pNext = m_pNext;
    pItem->m_pPrev = this;
    pItem->m_pNext = pNext;
    if (pNext) pNext->m_pPrev = pItem;
    m_pNext = pItem;
    m_pList = pList;
}

void EXLDListItem::Insert(EXLDListItem *pItem, EXLDList *pList)
{
    EXLDListItem *pPrev = m_pPrev;
    pItem->m_pPrev = pPrev;
    pItem->m_pNext = this;
    if (pPrev) pPrev->m_pNext = pItem;
    m_pPrev = pItem;
    m_pList = pList;
}

void EXLDListItem::Remove()
{
    EXLDList *pList = m_pList;
    if (!pList) return;
    if (this == pList->m_pHead) pList->m_pHead = m_pNext;
    if (this == pList->m_pTail) pList->m_pTail = m_pPrev;
    EXLDListItem *pPrev = m_pPrev;
    EXLDListItem *pNext = m_pNext;
    if (pPrev) {
        pPrev->m_pNext = pNext;
        m_pPrev = 0;
    }
    if (pNext) {
        pNext->m_pPrev = pPrev;
        m_pNext = 0;
    }
    m_pList = 0;
}

void EXLDList::AddHead(EXLDListItem *pItem)
{
    EXLDListItem *pHead = m_pHead;
    pItem->m_pPrev = 0;
    pItem->m_pNext = pHead;
    pItem->m_pList = this;
    if (pHead) pHead->m_pPrev = pItem;
    m_pHead = pItem;
    if (!m_pTail) m_pTail = pItem;
}

void EXLDList::AddTail(EXLDListItem *pItem)
{
    EXLDListItem *pTail = m_pTail;
    pItem->m_pNext = 0;
    pItem->m_pPrev = pTail;
    pItem->m_pList = this;
    if (pTail) pTail->m_pNext = pItem;
    m_pTail = pItem;
    if (!m_pHead) m_pHead = pItem;
}

// ---- EXODListItem / EXODList
void EXODListItem::Add(EXODListItem *pItem, void *pObject)
{
    EXODListItem *pNext = m_pNext;
    pItem->m_pObject = pObject ? pObject : pItem;
    pItem->m_pNext = pNext;
    pItem->m_pPrev = this;
    if (pNext) pNext->m_pPrev = pItem;
    m_pNext = pItem;
}

void EXODListItem::Insert(EXODListItem *pItem, void *pObject)
{
    EXODListItem *pPrev = m_pPrev;
    pItem->m_pObject = pObject ? pObject : pItem;
    pItem->m_pPrev = pPrev;
    pItem->m_pNext = this;
    if (pPrev) pPrev->m_pNext = pItem;
    m_pPrev = pItem;
}

void EXODListItem::Remove()
{
    EXODListItem *pPrev = m_pPrev;
    EXODListItem *pNext = m_pNext;
    if (pPrev) {
        pPrev->m_pNext = pNext;
        m_pPrev = 0;
    }
    if (pNext) {
        pNext->m_pPrev = pPrev;
        m_pNext = 0;
    }
}

s8 EXODListItem::Apply(void *pFunc, void *pData, s8 bObject)
{
    EXODListItem *p = this;
    while (p) {
        EXODListItem *pNext = p->m_pNext;
        if (!((EXListApplyFunc)pFunc)(bObject ? p->m_pObject : p, pData)) return 0;
        p = pNext;
    }
    return 1;
}

s8 EXODListItem::ApplyBack(void *pFunc, void *pData, s8 bObject)
{
    EXODListItem *p = this;
    while (p) {
        EXODListItem *pPrev = p->m_pPrev;
        if (!((EXListApplyFunc)pFunc)(bObject ? p->m_pObject : p, pData)) return 0;
        p = pPrev;
    }
    return 1;
}

void EXODList::AddHead(EXODListItem *pItem, void *pObject)
{
    EXODListItem *pHead = m_pHead;
    pItem->m_pObject = pObject ? pObject : pItem;
    pItem->m_pPrev = 0;
    pItem->m_pNext = pHead;
    if (pHead) pHead->m_pPrev = pItem;
    m_pHead = pItem;
    if (!m_pTail) m_pTail = pItem;
}

void EXODList::AddTail(EXODListItem *pItem, void *pObject)
{
    EXODListItem *pTail = m_pTail;
    pItem->m_pObject = pObject ? pObject : pItem;
    pItem->m_pNext = 0;
    pItem->m_pPrev = pTail;
    if (pTail) pTail->m_pNext = pItem;
    m_pTail = pItem;
    if (!m_pHead) m_pHead = pItem;
}

void EXODList::Remove(EXODListItem *pItem)
{
    if (pItem == m_pHead) m_pHead = pItem->m_pNext;
    if (pItem == m_pTail) m_pTail = pItem->m_pPrev;
    pItem->Remove();
}

EXODListItem *EXODList::RemoveHead()
{
    EXODListItem *p = m_pHead;
    if (p) Remove(p);
    return p;
}

EXODListItem *EXODList::RemoveTail()
{
    EXODListItem *p = m_pTail;
    if (p) Remove(p);
    return p;
}

s8 EXODList::IsMember(const EXODListItem *pItem) const
{
    for (EXODListItem *p = m_pHead; p; p = p->m_pNext)
        if (p == pItem) return 1;
    return 0;
}

u32 EXODList::Count() const
{
    u32 n = 0;
    for (EXODListItem *p = m_pHead; p; p = p->m_pNext) n++;
    return n;
}
