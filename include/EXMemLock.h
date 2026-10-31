#ifndef EXMEMLOCK_H
#define EXMEMLOCK_H
#include "EXMemBlock.h"

// Scoped lock on a heap allocation; restores the previous lock state.
class EXMemLock {
public:
    EXMemLock(void *p, s8 lock);
    ~EXMemLock();
private:
    EXMemBlock *m_pBlock;
    s8 m_WasLocked;
};
#endif
