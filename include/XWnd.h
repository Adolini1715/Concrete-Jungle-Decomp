#ifndef XWND_H
#define XWND_H
#include "EXWnd.h"

// One laid-out line of front-end text (FEWnd.obj). After the line, m_Skip characters (the line
// break: 1 for a newline, 2 for the "\n" escape, 0 after a wrap) are passed over.
struct XLineInfo {
    u16 m_Count;            // 0x0 characters drawn
    u16 m_Skip;             // 0x2
    u16 m_Width;            // 0x4
    short x, y;             // 0x6 window position
    u16 m_Height;           // 0xa
    u16 m_Indent;           // 0xc x offset (hanging indent after a "~." bullet)
    s8 m_Ellipsis;          // 0xe more text follows that does not fit: the last character becomes "..."
    u8 m_Pad;
};

// Front-end string layout (GetStringProperties).
struct XStringInfo {
    int m_NumLines;                                 // 0x00
    int m_Width;                                    // 0x04 whole string on one line
    int m_MaxWidth;                                 // 0x08 widest laid-out line
    int m_TotalHeight;                              // 0x0c
    int m_Height;                                   // 0x10 tallest glyph
    EXDataArray<XLineInfo, XLineInfo &> m_Lines;    // 0x14
};

// Game window with font rendering (FEWnd.obj); sizeof 0x2b0.
class XWnd : public EXWnd {
public:
    virtual s8 DrawWindow();
    // Overloaded virtuals: reverse declaration order.
    virtual EXPoint FontTextSize(const char *pText, float scale);
    virtual EXPoint FontTextSize(const char *pText, EXRect &r, float scale);
    virtual void FontPrint(short x, short y, const char *pText, float scale, EXW::TextAlign align, signed char b);
    virtual void FontPrint(const char *pText, const EXRect &r, float scale, EXW::TextAlign align, signed char b);
    virtual void RenderFont(const char *pText, const XLineInfo &info, float scale);
    virtual void GetStringProperties(const char *pText, const EXRect &r, XStringInfo *pInfo, EXW::TextAlign align, float scale);
    virtual void CalculateAlignment(unsigned int n, const EXRect &r, XStringInfo *pInfo, EXW::TextAlign align, float scale);
    virtual void ProcessControlChar(char c);
    virtual void SelectFont(unsigned int file, unsigned int hash);
    virtual void SetNormalFont()
    {
        m_pNormalFont = m_pFont;
        m_NormalFontFile = m_FontFile;
        m_NormalFontHash = m_FontHash;
    }
    virtual void ResetNormalFont() { SelectFont(m_NormalFontFile, m_NormalFontHash); }
    XWnd() {}
    int FontHeight();

    EXGeoFont *m_pNormalFont;       // 0x29c saved by SetNormalFont
    unsigned int m_FontHash;        // 0x2a0
    unsigned int m_FontFile;        // 0x2a4
    unsigned int m_NormalFontHash;  // 0x2a8
    unsigned int m_NormalFontFile;  // 0x2ac
};
#endif
