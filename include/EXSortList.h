#ifndef EXSORTLIST_H
#define EXSORTLIST_H
#include "types.h"

// Priority-sorted list over a fixed node pool (EXAudioBase.obj). sizeof 8.
// m_pNodes points at node 0; node -1 is the head sentinel and node m_Count the tail sentinel.
class EXSortList {
public:
    struct Node {
        int m_Prev;
        int m_Next;
        void *m_pData;  // 0 = free
        u32 m_Key;      // ascending
    };
    EXSortList(int count);
    ~EXSortList();
    void Add(void *pData, u32 key);
    void Remove(int i);
    void *GetHead(int &i);

    Node *m_pNodes;
    int m_Count;
};
#endif
