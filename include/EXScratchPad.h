#ifndef EXSCRATCHPAD_H
#define EXSCRATCHPAD_H
#include "types.h"

extern u64 EXScratchPadMem[0x802];  // 16 KB + alignment slack (map size 16400)

#define EXSCRATCHPAD_BASE ((u8 *)(((u32)EXScratchPadMem + 15) & ~15))
#define EXSCRATCHPAD_SIZE 0x4000

// Stack-like claims on a 16 KB scratch buffer; Release must happen in reverse order.
class EXScratchPad {
public:
    EXScratchPad() { m_pData = 0; }
    ~EXScratchPad() { Release(); }
    void *Cache(void *p, u32) const { return p; }
    void Release();
    u32 FreeBytes() const;
    void *Claim(u32 size);
    static void MemCpy(void *pDst, void *pSrc, u32 size, s8 wait);
protected:
    static u32 m_AllocSeq;
    static u8 *m_pStart;
private:
    u8 *m_pData;
    u32 m_Size;
};
#endif
