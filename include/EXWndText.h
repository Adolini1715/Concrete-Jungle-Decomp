#ifndef EXWNDTEXT_H
#define EXWNDTEXT_H
#include "types.h"
#include "EXRelPtr.h"
#include "EXArray.h"

// Bitmap fonts (.edb font list, EXWndText.obj). A font is one texture plus a two-level table of
// glyphs: 256 pages selected by the high byte of the character, each with 256 glyphs.
struct EXGeoFontChar {
    float u0, v0;           // 0x00 texture rectangle
    float u1, v1;           // 0x08
    u16 m_Width;            // 0x10 pixels
    u16 m_Height;           // 0x12
    u16 m_Kern;             // 0x14 subtracted from the width to get the advance
};
struct EXGeoFontPage {
    ex::EXRelPtr<EXGeoFontChar> m_Chars[256];
};
struct EXGeoFont {
    unsigned int TextureIndex() const { return m_TextureIndex; }
    EXGeoFontChar *Char(unsigned short c) const
    {
        EXGeoFontPage *pPage = m_Pages[c >> 8];
        if (!pPage) return 0;
        return pPage->m_Chars[c & 0xff];
    }
    u32 m_TextureIndex;                     // 0x00 index into the file's texture array
    ex::EXRelPtr<EXGeoFontPage> m_Pages[256];   // 0x04
};

// One laid-out line of a string: m_Count characters from the previous line's end, drawn at (x, y).
struct EXLineInfo {
    u16 m_Count;            // 0x0
    u16 m_Width;            // 0x2
    short x, y;             // 0x4 window position of the line's top left
};

// Result of GetStringProperties: the lines, the whole string's width and the line height.
struct EXStringInfo {
    int m_NumLines;                                     // 0x00
    int m_Width;                                        // 0x04 total width before wrapping
    int m_Height;                                       // 0x08 line height
    EXDataArray<EXLineInfo, const EXLineInfo &> m_Lines;  // 0x0c
};
#endif
