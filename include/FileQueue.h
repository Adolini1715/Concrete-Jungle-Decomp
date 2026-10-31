#ifndef FILEQUEUE_H
#define FILEQUEUE_H
#include "types.h"
#include "EXSortList.h"
#include "XBCriticalSection.h"

struct AsyncFileData;

// Priority queue of pending reads, guarded by a critical section (sizeof 0x20).
class FileQueue {
public:
    FileQueue();
    void Add(AsyncFileData *pData);
    AsyncFileData *GetActive();
    s8 HasWorkToDo()
    {
        int i;
        return m_pList->GetHead(i) != 0;
    }
    EXSortList *m_pList;       // 0x00
    XBCriticalSection m_Lock;  // 0x04
};
#endif
