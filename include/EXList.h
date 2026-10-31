#ifndef EXLIST_H
#define EXLIST_H
#include "types.h"

// Intrusive doubly linked lists. Item layout {prev, next}; list layout {head, tail}.
// Apply callbacks are passed as void * and called as s8 (*)(item-or-object, void *data).
typedef s8 (*EXListApplyFunc)(void *pItem, void *pData);

class EXDListItem {
public:
    EXDListItem *Next() const { return m_pNext; }
    EXDListItem *Prev() const { return m_pPrev; }
    void SetNext(EXDListItem *p) { m_pNext = p; }
    void SetPrev(EXDListItem *p) { m_pPrev = p; }
    void Init() { m_pNext = 0; m_pPrev = 0; }
    void Add(EXDListItem *pItem);
    void Insert(EXDListItem *pItem);
    void Remove();
    s8 Apply(void *pFunc, void *pData);
    s8 ApplyBack(void *pFunc, void *pData);
    EXDListItem *m_pPrev;
    EXDListItem *m_pNext;
};

class EXDList {
public:
    EXDList() { m_pHead = 0; m_pTail = 0; }
    void Init() { m_pHead = 0; m_pTail = 0; }
    EXDListItem *Head() const { return m_pHead; }
    EXDListItem *Tail() const { return m_pTail; }
    void SetHead(EXDListItem *p) { m_pHead = p; }
    void SetTail(EXDListItem *p) { m_pTail = p; }
    void AddHead(EXDListItem *pItem);
    void AddTail(EXDListItem *pItem);
    void AddTail(EXDListItem &item) { AddTail(&item); }
    void Insert(EXDListItem *pPos, EXDListItem *pItem);
    void Remove(EXDListItem *pItem);
    EXDListItem *RemoveHead();
    EXDListItem *RemoveTail();
    s8 IsMember(const EXDListItem *pItem) const;
    u32 Count() const;
    s8 Apply(void *pFunc, void *pData) { return m_pHead ? m_pHead->Apply(pFunc, pData) : 1; }
    EXDListItem *m_pHead;
    EXDListItem *m_pTail;
};

// Object list: each item points at its owner (or itself).
class EXODListItem {
public:
    EXODListItem *Next() const { return m_pNext; }
    EXODListItem *Prev() const { return m_pPrev; }
    void *Object() const { return m_pObject; }
    void SetNext(EXODListItem *p) { m_pNext = p; }
    void SetPrev(EXODListItem *p) { m_pPrev = p; }
    void SetObject(void *p) { m_pObject = p; }
    void Init() { m_pNext = 0; m_pPrev = 0; m_pObject = this; }
    void Add(EXODListItem *pItem, void *pObject);
    void Insert(EXODListItem *pItem, void *pObject);
    void Remove();
    s8 Apply(void *pFunc, void *pData, s8 bObject);
    s8 ApplyBack(void *pFunc, void *pData, s8 bObject);
    EXODListItem *m_pPrev;
    EXODListItem *m_pNext;
    void *m_pObject;
};

class EXODList {
public:
    EXODList() { m_pHead = 0; m_pTail = 0; }
    void Init() { m_pHead = 0; m_pTail = 0; }
    EXODListItem *Head() const { return m_pHead; }
    EXODListItem *Tail() const { return m_pTail; }
    void AddHead(EXODListItem *pItem, void *pObject);
    void AddTail(EXODListItem *pItem, void *pObject);
    void AddHead(EXODListItem &item, void *pObject) { AddHead(&item, pObject); }
    void AddTail(EXODListItem &item, void *pObject) { AddTail(&item, pObject); }
    void Remove(EXODListItem *pItem);
    void Remove(EXODListItem &item) { Remove(&item); }
    EXODListItem *RemoveHead();
    EXODListItem *RemoveTail();
    s8 IsMember(const EXODListItem *pItem) const;
    u32 Count() const;
    EXODListItem *m_pHead;
    EXODListItem *m_pTail;
};

class EXLDList;

// Item that knows which list holds it.
class EXLDListItem {
public:
    EXLDListItem() {}
    void Init() { m_pNext = 0; m_pPrev = 0; m_pList = 0; }
    EXLDList *List() const { return m_pList; }
    void SetList(EXLDList *p) { m_pList = p; }
    void Add(EXLDListItem *pItem, EXLDList *pList);
    void Insert(EXLDListItem *pItem, EXLDList *pList);
    void Remove();
    EXLDListItem *m_pPrev;
    EXLDListItem *m_pNext;
    EXLDList *m_pList;
};

class EXLDList {
public:
    EXLDList() { m_pHead = 0; m_pTail = 0; }
    void AddHead(EXLDListItem *pItem);
    void AddTail(EXLDListItem *pItem);
    EXLDListItem *m_pHead;
    EXLDListItem *m_pTail;
};
#endif
