#include "types.h"

// TODO layout: EXMemHeap belongs to the memory unit; only the range fields used here.
class EXMemHeap {
public:
    void *m_pVtbl;
    u8 *m_pStart;
    u8 *m_pEnd;
};
extern EXMemHeap *_SystemHeapList[4];

s8 isValidFloat(float f, s8 warn)
{
    u32 bits = *(u32 *)&f;
    if ((bits & 0x7fff0000) >= 0x7f800000) return 0;
    return bits != 0x51515151;
}

s8 isValidPtr(void *p, s8 warn)
{
    if (p == 0 || p == (void *)0x51515151) return 0;
    int i;
    for (i = 0; i < 4; i++) {
        EXMemHeap *h = _SystemHeapList[i];
        if (h && (u8 *)p >= h->m_pStart && (u8 *)p <= h->m_pEnd) break;
    }
    return i != 4;
}
