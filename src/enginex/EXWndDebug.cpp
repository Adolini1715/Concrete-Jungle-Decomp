#include "EXWnd.h"
#include "EXStdFuncs.h"
#include "EXMemory.h"

extern "C" int __cdecl vsprintf(char *, const char *, char *);

void EXBaseWnd::SetDbgPosPixels(unsigned short x, unsigned short y)
{
    m_DbgX = x;
    m_DbgY = y;
}

void EXBaseWnd::DeleteDbgMem()
{
    if (m_pDbgText) {
        EXFree(m_pDbgText);
        m_pDbgText = 0;
        m_DbgAttr = 0xff00;
        m_DbgWidth = 0;
        m_DbgHeight = 0;
    }
}

void EXBaseWnd::DbgClear()
{
    if (m_Flags & 0x20000) return;
    if (m_pDbgText) {
        for (int i = m_DbgWidth * m_DbgHeight - 1; i >= 0; i--) m_pDbgText[i] = 0xff00;
        m_DbgX = 0;
        m_DbgY = 0;
    } else {
        CreateDbgMem();
    }
}

void EXBaseWnd::SetDbgPos(unsigned short x, unsigned short y)
{
    unsigned short maxX = m_DbgWidth - 1;
    m_DbgX = min(x, maxX);
    unsigned short maxY = m_DbgHeight - 1;
    m_DbgY = min(y, maxY);
}

void EXBaseWnd::CreateDbgMem()
{
    if (!(m_Flags & 8)) return;
    unsigned short w = m_Rect.w / 12;
    unsigned short h = m_Rect.h / 12;
    if (m_DbgWidth == w && m_DbgHeight == h) return;
    if (m_pDbgText) {
        int n = w * h;
        u16 *pNew = (u16 *)_EXAlloc(n * 2, 0);
        for (int i = n - 1; i >= 0; i--) pNew[i] = 0xff00;
        unsigned short rowBytes = min(m_DbgWidth, w) * 2;
        unsigned short rows = min(m_DbgHeight, h);
        u16 *pSrc = m_pDbgText;
        u16 *pDst = pNew;
        for (unsigned short r = 0; r < rows; r++) {
            memcpy(pDst, pSrc, rowBytes);
            pSrc += m_DbgWidth;
            pDst += w;
        }
        m_DbgWidth = w;
        m_DbgHeight = h;
        EXFree(m_pDbgText);
        m_pDbgText = pNew;
    } else {
        m_pDbgText = (u16 *)_EXAlloc(w * h * 2, 0);
        m_DbgWidth = w;
        m_DbgHeight = h;
        DbgClear();
    }
}

void EXBaseWnd::DbgScroll()
{
    if (m_pDbgText) {
        memcpy(m_pDbgText, m_pDbgText + m_DbgWidth, (m_DbgHeight - 1) * m_DbgWidth * 2);
        memset(m_pDbgText + (m_DbgHeight - 1) * m_DbgWidth, 0, m_DbgWidth * 2);
        m_DbgY--;
    } else {
        CreateDbgMem();
    }
}

// NEAR: register allocation differs (retail keeps the mode byte on the stack, no zero register).
void EXBaseWnd::DbgPrint(const char *pFmt, ...)
{
    static char s_Buf[0x200];
    if (!this || (m_Flags & 0x20000)) return;
    if (m_pDbgText) {
        if (vsprintf(s_Buf, pFmt, (char *)(&pFmt + 1)) > 0x200) return;
            u16 *p = &m_pDbgText[m_DbgWidth * m_DbgY + m_DbgX];
            u8 n = 0;
            char mode = 'N';
            const char *s = s_Buf;
            u8 c;
            while ((c = *s++) != 0) {
                switch (mode) {
                default:
                    if (c == '~') {
                        mode = 'T';
                    } else if (c >= 0x20 && c <= 0xb6) {
                        *p = (u16)c | m_DbgAttr;
                        p++;
                        if (++m_DbgX >= m_DbgWidth) {
                            m_DbgX = 0;
                            m_DbgY++;
                            if (m_DbgY >= m_DbgHeight) {
                                if (m_Flags & 0x40) DbgScroll();
                                else m_DbgY = 0;
                            }
                            p = &m_pDbgText[m_DbgWidth * m_DbgY + m_DbgX];
                        }
                    } else if (c == '\n') {
                        m_DbgX = 0;
                        m_DbgY++;
                        if (m_DbgY >= m_DbgHeight) {
                            if (m_Flags & 0x40) DbgScroll();
                            else m_DbgY = 0;
                        }
                        p = &m_pDbgText[m_DbgWidth * m_DbgY + m_DbgX];
                    }
                    break;
                case 'T':
                    if (c == 'C' || c == 'c') {
                        mode = 'C';
                        n = 2;
                        m_DbgAttr = 0;
                    } else {
                        *p = (u16)c | m_DbgAttr;
                        p++;
                        if (++m_DbgX >= m_DbgWidth) {
                            m_DbgX = 0;
                            m_DbgY++;
                            if (m_DbgY >= m_DbgHeight) {
                                if (m_Flags & 0x40) DbgScroll();
                                else m_DbgY = 0;
                            }
                            p = &m_pDbgText[m_DbgWidth * m_DbgY + m_DbgX];
                        }
                    }
                    break;
                case 'C': {
                    u8 zero = 0;
                    u8 v = c - '0';
                    u8 three = 3;
                    m_DbgAttr |= (u16)min(three, max(zero, v)) << (n * 2 + 8);
                    if (n-- == 0) mode = 'N';
                    break;
                }
                }
            }
    } else {
        CreateDbgMem();
    }
}
