#include "EXScratchPad.h"

extern "C" void *__cdecl memcpy(void *, const void *, unsigned int);

u64 EXScratchPadMem[0x802];
u32 EXScratchPad::m_AllocSeq;
u8 *EXScratchPad::m_pStart = EXSCRATCHPAD_BASE;

void EXScratchPad::Release()
{
    if (m_pData) {
        m_pStart -= m_Size;
        m_pData = 0;
    }
}

u32 EXScratchPad::FreeBytes() const
{
    return EXSCRATCHPAD_BASE - m_pStart + EXSCRATCHPAD_SIZE;
}

void *EXScratchPad::Claim(u32 size)
{
    void *ret = 0;
    u8 *p = m_pStart;
    int used = (p - EXSCRATCHPAD_BASE) >> 4;
    int n = (size + 15) >> 4;
    if (EXSCRATCHPAD_SIZE / 16 - used > n) {
        m_pData = p;
        m_Size = n << 4;
        m_pStart += m_Size;
        ret = p;
    }
    return ret;
}

void EXScratchPad::MemCpy(void *pDst, void *pSrc, u32 size, s8)
{
    memcpy(pDst, pSrc, size);
}
