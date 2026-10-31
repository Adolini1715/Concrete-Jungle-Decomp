#include "EXBaseWnd.h"
#include "EXWndText.h"
#include "EXGeoFile.h"
#include "EXGeoHeader.h"
#include "EXTexture.h"
#include "EXSprite2D.h"
#include "EXStdFuncs.h"
#include "EXMemory.h"
#include <string.h>

// Bitmap font text for windows: glyph sizes, line layout with word wrap and alignment, and drawing
// each glyph as a Sprite2D in the window's font colour. Wide strings treat the two characters "\n"
// or "\r" (backslash, letter) as a line break; narrow strings use the control characters when laid
// out and the escapes only when measured.
//
// Alignment (EXW::TextAlign): 0 top left, 1 bottom left, 2 centre left, 3 top centre, 4 centre,
// 5 bottom centre, 6 top right, 7 centre right, 8 bottom right.

EXPoint EXBaseWnd::FontCharSize(char c, float scale) const
{
    int w = 0;
    int h = 0;
    if (m_pFont) {
        EXGeoFontChar *pChar = m_pFont->Char((unsigned char)c);
        if (pChar) {
            w = (int)(pChar->m_Width * scale);
            h = (int)(pChar->m_Height * scale);
        }
    }
    return EXPoint(w, h);
}

EXPoint EXBaseWnd::FontCharSize(unsigned short c, float scale) const
{
    int w = 0;
    int h = 0;
    if (m_pFont) {
        EXGeoFontChar *pChar = m_pFont->Char(c);
        if (pChar) {
            w = (int)(pChar->m_Width * scale);
            h = (int)(pChar->m_Height * scale);
        }
    }
    return EXPoint(w, h);
}

unsigned short EXBaseWnd::FontCharWidth(char c) const
{
    if (m_pFont) {
        EXGeoFontChar *pChar = m_pFont->Char(c);
        if (pChar) return pChar->m_Width;
    }
    return 0;
}

unsigned short EXBaseWnd::FontCharHeight(char c) const
{
    if (m_pFont) {
        EXGeoFontChar *pChar = m_pFont->Char(c);
        if (pChar) return pChar->m_Height;
    }
    return 0;
}

// Places line n (1-based) of a laid-out string inside r.
void EXBaseWnd::CalculateAlignment(unsigned int n, const EXRect &r, EXStringInfo *pInfo, EXW::TextAlign align,
                                   float scale) const
{
    EXLineInfo line = pInfo->m_Lines.GetAt(n - 1);
    unsigned int h = pInfo->m_Height;
    int xc = (r.w - line.m_Width) >> 1;
    int below = (pInfo->m_NumLines - n + 1) * h;
    switch (align) {
    case 2:
        if (pInfo->m_NumLines > 1)
            line.y = r.y - ((r.h - pInfo->m_NumLines * h) >> 1) - below + r.h;
        else
            line.y = ((r.h - below) >> 1) + r.y;
        line.x = r.x;
        break;
    case 1:
        line.x = r.x;
        line.y = r.y + r.h - below;
        break;
    case 3:
        line.x = r.x + xc;
        line.y = (n - 1) * h + r.y;
        break;
    case 4:
        if (pInfo->m_NumLines > 1)
            line.y = r.y - ((r.h - pInfo->m_NumLines * h) >> 1) - below + r.h;
        else
            line.y = ((r.h - below) >> 1) + r.y;
        line.x = r.x + xc;
        break;
    case 5:
        line.x = r.x + xc;
        line.y = r.y + r.h - below;
        break;
    case 6:
        line.x = r.w - line.m_Width + r.x;
        line.y = (n - 1) * h + r.y;
        break;
    case 7:
        if (pInfo->m_NumLines > 1)
            line.y = r.y - ((r.h - pInfo->m_NumLines * h) >> 1) - below + r.h;
        else
            line.y = ((r.h - below) >> 1) + r.y;
        line.x = r.w - line.m_Width + r.x;
        break;
    case 8:
        line.x = r.w - line.m_Width + r.x;
        line.y = r.y + r.h - below;
        break;
    default:
        line.x = r.x;
        line.y = (n - 1) * h + r.y;
        break;
    }
    pInfo->m_Lines.SetAt(n - 1, line);
}

void EXBaseWnd::RenderFont(const char *pText, const EXLineInfo &line, unsigned short height, float scale)
{
    const char *p = pText;
    EXSprite2D s;
    s.m_Color = m_FontColor;
    unsigned int n = line.m_Count;
    s.x = line.x;
    s.y = line.y;
    if (scale == 1.0f) {
        for (unsigned int i = 0; i < n; i++, p++) {
            EXGeoFontChar *pChar = m_pFont->Char((unsigned char)*p);
            if (pChar) {
                s.w = pChar->m_Width;
                s.h = pChar->m_Height;
                s.u0 = pChar->u0;
                s.v0 = pChar->v0;
                s.u1 = pChar->u1;
                s.v1 = pChar->v1;
                _DrawSprite2D(s);
                s.x += pChar->m_Width - pChar->m_Kern;
            }
        }
    } else {
        for (unsigned int i = 0; i < n; i++, p++) {
            EXGeoFontChar *pChar = m_pFont->Char((unsigned char)*p);
            if (pChar) {
                s.w = (u16)(pChar->m_Width * scale);
                s.h = (u16)(pChar->m_Height * scale);
                s.u0 = pChar->u0;
                s.v0 = pChar->v0;
                s.u1 = pChar->u1;
                s.v1 = pChar->v1;
                _DrawSprite2D(s);
                s.x += s.w - pChar->m_Kern;
            }
        }
    }
}

void EXBaseWnd::RenderFont(const unsigned short *pText, const EXLineInfo &line, unsigned short height, float scale)
{
    const unsigned short *p = pText;
    EXSprite2D s;
    s.m_Color = m_FontColor;
    unsigned int n = line.m_Count;
    s.x = line.x;
    s.y = line.y;
    if (scale == 1.0f) {
        for (unsigned int i = 0; i < n; i++, p++) {
            unsigned short c = *p;
            EXGeoFontChar *pChar = m_pFont->Char(c);
            if (pChar) {
                if (c == '\\' && (p[1] == 'n' || p[1] == 'r')) {
                    p++;
                } else {
                    s.w = pChar->m_Width;
                    s.h = pChar->m_Height;
                    s.u0 = pChar->u0;
                    s.v0 = pChar->v0;
                    s.u1 = pChar->u1;
                    s.v1 = pChar->v1;
                    _DrawSprite2D(s);
                    s.x += pChar->m_Width - pChar->m_Kern;
                }
            }
        }
    } else {
        for (unsigned int i = 0; i < n; i++, p++) {
            unsigned short c = *p;
            EXGeoFontChar *pChar = m_pFont->Char(c);
            if (pChar) {
                if (c == '\\' && (p[1] == 'n' || p[1] == 'r')) {
                    p++;
                } else {
                    s.w = (u16)(pChar->m_Width * scale);
                    s.h = (u16)(pChar->m_Height * scale);
                    s.u0 = pChar->u0;
                    s.v0 = pChar->v0;
                    s.u1 = pChar->u1;
                    s.v1 = pChar->v1;
                    _DrawSprite2D(s);
                    s.x += s.w - pChar->m_Kern;
                }
            }
        }
    }
}

// One character at (x, y), aligned about that point (the horizontal offsets use the unscaled width).
void EXBaseWnd::FontPrint(unsigned short x, unsigned short y, unsigned short c, float scale, EXW::TextAlign align,
                          s8 filter)
{
    if (!m_pFont || !m_pFontTexture) return;
    EXGeoFontChar *pChar = m_pFont->Char(c);
    if (!pChar) return;
    if (m_pFontTexture != m_pSprite2DTexture) SelectSprite2DTexture(m_pFontTexture, filter, 0);
    u16 w = pChar->m_Width;
    unsigned int sw = (unsigned int)(w * scale);
    unsigned int sh = (unsigned int)(pChar->m_Height * scale);
    EXSprite2D s;
    switch (align) {
    case 2:
        s.x = x;
        s.y = y - (sh >> 1);
        break;
    case 1:
        s.x = x;
        s.y = y - sh;
        break;
    case 3:
        s.x = x - (w >> 1);
        s.y = y;
        break;
    case 4:
        s.x = x - (w >> 1);
        s.y = y - (sh >> 1);
        break;
    case 5:
        s.x = x - (w >> 1);
        s.y = y - sh;
        break;
    case 6:
        s.x = x - w;
        s.y = y;
        break;
    case 7:
        s.x = x - w;
        s.y = y - (sh >> 1);
        break;
    case 8:
        s.x = x - w;
        s.y = y - sh;
        break;
    default:
        s.x = x;
        s.y = y;
        break;
    }
    s.m_Color = m_FontColor;
    s.w = sw;
    s.h = sh;
    s.u0 = pChar->u0;
    s.v0 = pChar->v0;
    s.u1 = pChar->u1;
    s.v1 = pChar->v1;
    _DrawSprite2D(s);
}

void EXBaseWnd::SelectFont(const EXGeoFile *pFile, unsigned int hash)
{
    if (pFile && pFile->GeoHeader()) {
        EXGeoHeader *pHdr = pFile->GeoHeader();
        const EXGeoCommonArray<EXGeoFontHeader> &fonts = pHdr->FontList();
        int i = fonts.FindIndex(hash);
        m_pFont = (i < 0 ? 0 : &fonts.Data()[i])->Object(0);
        m_pFontTexture = 0;
        if (m_pFont) m_pFontTexture = &(*pHdr->m_pTextureArray)[m_pFont->TextureIndex()];
    }
}

// Splits a string into lines that fit r.w (breaking after the last space, or at a newline) until
// r.h is full, then aligns each line.
void EXBaseWnd::GetStringProperties(const char *pText, const EXRect &r, EXStringInfo *pInfo, EXW::TextAlign align,
                                    float scale) const
{
    EXLineInfo line;
    int totalHeight;
    memset(pInfo, 0, sizeof(EXStringInfo));
    pInfo->m_Lines.SetGrowBy(32);
    totalHeight = 0;
    pInfo->m_Height = 0x8000;
    const char *p;
    for (p = pText; *p; p++) {
        EXGeoFontChar *pChar = m_pFont->Char((unsigned char)*p);
        if (pChar) {
            pInfo->m_Width += (int)((pChar->m_Width - pChar->m_Kern) * scale);
            if (pChar->m_Height < pInfo->m_Height) pInfo->m_Height = (int)(pChar->m_Height * scale);
        }
    }
    if (pInfo->m_Width < r.w) {
        line.m_Width = pInfo->m_Width;
        line.m_Count = strlen(pText);
        pInfo->m_Lines.Add(line, 1);
        pInfo->m_NumLines = 1;
        CalculateAlignment(1, r, pInfo, align, scale);
        return;
    }
    EXGeoFontChar *pChar = 0;
    int lastSpace = 0;
    int spaceWidth = 0;
    int lineWidth = 0;
    for (p = pText; *p && !pChar; p++) pChar = m_pFont->Char((unsigned char)*p);
    if (!pChar) return;
    int lineHeight = (int)(pChar->m_Height * scale);
    const char *pLine = pText;
    int i = 0;
    for (p = pText; *p; p++, i++) {
        pChar = m_pFont->Char((unsigned char)*p);
        if (*p == ' ') {
            lastSpace = i + 1;
            if (pChar)
                spaceWidth = (u16)((pChar->m_Width - pChar->m_Kern) * scale) + lineWidth;
            else
                spaceWidth = lineWidth;
        }
        if (*p == '\n' || *p == '\r') {
            line.m_Width = lineWidth;
            line.m_Count = i;
            pInfo->m_Lines.Add(line, 1);
            pInfo->m_NumLines++;
            lastSpace = 0;
            lineWidth = 0;
            totalHeight += lineHeight;
            pLine += i;
            p = pLine;
            i = -1;
            if (totalHeight >= r.h) break;
        } else if (pChar) {
            lineWidth += (int)((pChar->m_Width - pChar->m_Kern) * scale);
            if (lineWidth >= r.w) {
                line.m_Count = lastSpace;
                line.m_Width = spaceWidth;
                pInfo->m_Lines.Add(line, 1);
                pInfo->m_NumLines++;
                pLine += lastSpace;
                lastSpace = 0;
                lineWidth = 0;
                totalHeight += lineHeight;
                p = pLine;
                i = -1;
                if (totalHeight >= r.h) break;
            }
        }
    }
    if (lineWidth && totalHeight < r.h) {
        line.m_Width = lineWidth;
        line.m_Count = strlen(pLine);
        pInfo->m_Lines.Add(line, 1);
        pInfo->m_NumLines++;
    }
    for (int n = 1; n <= pInfo->m_NumLines; n++) CalculateAlignment(n, r, pInfo, align, scale);
}

void EXBaseWnd::GetStringProperties(const unsigned short *pText, const EXRect &r, EXStringInfo *pInfo,
                                    EXW::TextAlign align, float scale) const
{
    memset(pInfo, 0, sizeof(EXStringInfo));
    pInfo->m_Lines.SetGrowBy(32);
    EXLineInfo line;
    int totalHeight;
    const unsigned short *p;
    totalHeight = 0;
    for (p = pText; *p; p++) {
        EXGeoFontChar *pChar = m_pFont->Char(*p);
        if (pChar) {
            pInfo->m_Width += (int)((pChar->m_Width - pChar->m_Kern) * scale);
            pInfo->m_Height = (int)(pChar->m_Height * scale);
        }
    }
    if (pInfo->m_Width < r.w) {
        line.m_Width = pInfo->m_Width;
        line.m_Count = Wcslen(pText);
        pInfo->m_Lines.Add(line, 1);
        pInfo->m_NumLines = 1;
        CalculateAlignment(1, r, pInfo, align, scale);
        return;
    }
    int spaceWidth = 0;
    int lastSpace = 0;
    EXGeoFontChar *pChar = m_pFont->Char(*pText);
    int i = 0;
    int lineWidth = 0;
    int lineHeight = (int)(pChar->m_Height * scale);
    const unsigned short *pLine = pText;
    for (p = pText; *p; p++, i++) {
        pChar = m_pFont->Char(*p);
        if (*p == ' ') {
            lastSpace = i + 1;
            spaceWidth = (u16)((pChar->m_Width - pChar->m_Kern) * scale) + lineWidth;
        }
        if (*p == '\\' && (p[1] == 'n' || p[1] == 'r')) {
            lastSpace = 0;
            line.m_Width = lineWidth;
            line.m_Count = i;
            pInfo->m_Lines.Add(line, 1);
            pLine += i + 1;
            lineWidth = 0;
            totalHeight += lineHeight;
            i = -1;
            pInfo->m_NumLines++;
            p = pLine;
            if (totalHeight >= r.h) break;
        } else if (pChar) {
            lineWidth += (int)((pChar->m_Width - pChar->m_Kern) * scale);
            if (lineWidth >= r.w) {
                if (!lastSpace) lastSpace = i;
                pLine += lastSpace;
                line.m_Width = spaceWidth;
                line.m_Count = lastSpace;
                pInfo->m_Lines.Add(line, 1);
                lineWidth = 0;
                totalHeight += lineHeight;
                pInfo->m_NumLines++;
                i = -1;
                p = pLine;
                lastSpace = 0;
                if (totalHeight >= r.h) break;
            }
        }
    }
    if (lineWidth && totalHeight < r.h) {
        line.m_Width = lineWidth;
        line.m_Count = Wcslen(pLine);
        pInfo->m_Lines.Add(line, 1);
        pInfo->m_NumLines++;
    }
    for (int n = 1; n <= pInfo->m_NumLines; n++) CalculateAlignment(n, r, pInfo, align, scale);
}

// Size of a string on one line per "\n" escape: widest line by tallest glyph times the line count.
EXPoint EXBaseWnd::FontTextSize(const unsigned short *pText, float scale) const
{
    EXStringInfo info;
    int height = 0;
    int lineWidth = 0;
    info.m_Width = 0;
    info.m_NumLines = 1;
    EXGeoFont *pFont = m_pFont;
    if (pFont && *pText) {
        do {
            unsigned short c = *pText;
            EXGeoFontChar *pChar = pFont->Char(c);
            if (pChar) {
                if (c == '\\' && (pText[1] == 'n' || pText[1] == 'r')) {
                    if (lineWidth > info.m_Width) info.m_Width = lineWidth;
                    lineWidth = 0;
                    info.m_NumLines++;
                    pText++;
                } else {
                    lineWidth += (int)(pChar->m_Width * scale);
                    int h = (int)(pChar->m_Height * scale);
                    if (height < h) height = h;
                }
            }
            pText++;
        } while (*pText);
        if (lineWidth > info.m_Width) info.m_Width = lineWidth;
    }
    return EXPoint(info.m_Width, height * info.m_NumLines);
}

EXPoint EXBaseWnd::FontTextSize(const char *pText, const EXRect &r, float scale) const
{
    int w = 0;
    EXStringInfo info;
    GetStringProperties(pText, r, &info, (EXW::TextAlign)0, scale);
    int h = info.m_Height * info.m_NumLines;
    for (int i = 0; i < info.m_NumLines; i++)
        if (info.m_Lines[i].m_Width > w) w = info.m_Lines[i].m_Width;
    return EXPoint(w, h);
}

EXPoint EXBaseWnd::FontTextSize(const unsigned short *pText, const EXRect &r, float scale) const
{
    EXStringInfo info;
    GetStringProperties(pText, r, &info, (EXW::TextAlign)0, scale);
    int w = 0;
    int h = info.m_Height * info.m_NumLines;
    for (int i = 0; i < info.m_NumLines; i++)
        if (info.m_Lines[i].m_Width > w) w = info.m_Lines[i].m_Width;
    return EXPoint(w, h);
}

void EXBaseWnd::FontPrint(const unsigned short *pText, const EXRect &r, float scale, EXW::TextAlign align, s8 filter,
                          const EXStringInfo *pInfo)
{
    EXStringInfo info;
    if (!m_pFont || !m_pFontTexture) return;
    if (m_pFontTexture != m_pSprite2DTexture) SelectSprite2DTexture(m_pFontTexture, filter, 0);
    if (!pInfo) {
        GetStringProperties(pText, r, &info, align, scale);
        pInfo = &info;
    }
    FontTextSize(pText, 1.0f);
    for (int i = 0; i < pInfo->m_NumLines; i++) {
        EXLineInfo line = pInfo->m_Lines[i];
        RenderFont(pText, line, pInfo->m_Height, scale);
        pText += line.m_Count;
    }
    info.m_Lines.RemoveAll();
}

void EXBaseWnd::FontPrint(unsigned short x, unsigned short y, const unsigned short *pText, float scale,
                          EXW::TextAlign align, s8 filter, const EXStringInfo *pInfo)
{
    const unsigned short *p = pText;
    EXStringInfo info;
    if (!m_pFont || !m_pFontTexture) return;
    if (m_pFontTexture != m_pSprite2DTexture) SelectSprite2DTexture(m_pFontTexture, filter, 0);
    EXRect r(x, y, m_Rect.w - x, m_Rect.h - y);
    if (!pInfo) {
        GetStringProperties(p, r, &info, align, scale);
        pInfo = &info;
    }
    if (pInfo->m_Width) {
        FontTextSize(p, 1.0f);
        for (int i = 0; i < pInfo->m_NumLines; i++) {
            EXLineInfo line = pInfo->m_Lines[i];
            RenderFont(p, line, pInfo->m_Height, scale);
            p += line.m_Count;
        }
    }
}

EXPoint EXBaseWnd::FontTextSize(const char *pText, float scale) const
{
    EXStringInfo info;
    info.m_Width = 0;
    int height = 0;
    int lineWidth = 0;
    info.m_NumLines = 1;
    EXGeoFont *pFont = m_pFont;
    if (pFont && *pText) {
        do {
            char c = *pText;
            EXGeoFontChar *pChar = pFont->Char((unsigned char)c);
            if (pChar) {
                if (c == '\\' && (pText[1] == 'n' || pText[1] == 'r')) {
                    if (lineWidth > info.m_Width) info.m_Width = lineWidth;
                    pText++;
                    lineWidth = 0;
                    info.m_NumLines++;
                } else {
                    lineWidth += (int)(pChar->m_Width * scale);
                    int h = (int)(pChar->m_Height * scale);
                    if (height < h) height = h;
                }
            }
            pText++;
        } while (*pText);
        if (lineWidth > info.m_Width) info.m_Width = lineWidth;
    }
    return EXPoint(info.m_Width, height * info.m_NumLines);
}

void EXBaseWnd::FontPrint(const char *pText, const EXRect &r, float scale, EXW::TextAlign align, s8 filter,
                          const EXStringInfo *pInfo)
{
    EXStringInfo info;
    if (!m_pFont || !m_pFontTexture) return;
    if (m_pFontTexture != m_pSprite2DTexture) SelectSprite2DTexture(m_pFontTexture, filter, 0);
    if (!pInfo) {
        GetStringProperties(pText, r, &info, align, scale);
        pInfo = &info;
    }
    FontTextSize(pText, 1.0f);
    for (int i = 0; i < pInfo->m_NumLines; i++) {
        EXLineInfo line = pInfo->m_Lines[i];
        RenderFont(pText, line, pInfo->m_Height, scale);
        pText += line.m_Count;
    }
    info.m_Lines.RemoveAll();
}

void EXBaseWnd::FontPrint(unsigned short x, unsigned short y, const char *pText, float scale, EXW::TextAlign align,
                          s8 filter, const EXStringInfo *pInfo)
{
    EXStringInfo info;
    const char *p = pText;
    if (!m_pFont || !m_pFontTexture) return;
    if (m_pFontTexture != m_pSprite2DTexture) SelectSprite2DTexture(m_pFontTexture, filter, 0);
    EXRect r(x, y, m_Rect.w - x, m_Rect.h - y);
    if (!pInfo) {
        GetStringProperties(p, r, &info, align, scale);
        pInfo = &info;
    }
    if (pInfo->m_Width) {
        FontTextSize(p, 1.0f);
        for (int i = 0; i < pInfo->m_NumLines; i++) {
            EXLineInfo line = pInfo->m_Lines[i];
            RenderFont(p, line, pInfo->m_Height, scale);
            p += line.m_Count;
        }
    }
}
