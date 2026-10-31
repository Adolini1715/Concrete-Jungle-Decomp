#include "FEText.h"
#include "EXMemory.h"
#include "EXBaseDisplay.h"
#include "EXBaseRenderEnv.h"
#include "EXBaseSoundManager.h"
#include "EXGeoFile.h"
#include "XAppStandIn.h"
#include "FEWnd.h"
#include "GameData.h"
#include "FEPause.h"
#include "FETopmost.h"
#include "FERadar.h"
#include "FEHud.h"
#include "FEObject.h"
#include "FEListener.h"
#include "FE.h"
#include "FESound.h"
#include "FEManager.h"
#include "EXParticleSysMgr.h"
#include "EXWndText.h"
#include "EXSprite2D.h"
#include "EXStdFuncs.h"
#include "XGamePad.h"
#include "FEObjectScript.h"
#include <string.h>

extern "C" int __cdecl sprintf(char *pBuf, const char *pFmt, ...);



EXStack<FEObject *> FE::m_Stack(0x80);
FEObject *FE::m_pMarker[8];
int FESound::m_RefCount;

signed char XWnd::DrawWindow()
{
    if (FEManager::Instance(1) && FEManager::Instance(1)->AttractModeTime() > 20.0f) return 1;
    return EXBaseWnd::DrawWindow();
}

// Front-end text control codes, "<c>": n normal font, i / v / s the alternative fonts.
void XWnd::ProcessControlChar(char c)
{
    switch (c) {
    case 'n':
        SelectFont(0x100003a, m_NormalFontHash);
        break;
    case 'i':
        SelectFont(0x100003a, 0x700000c);
        break;
    case 'v':
        SelectFont(0x100003a, 0x700000a);
        break;
    case 's':
        SelectFont(0x100003a, 0x7000004);
        break;
    }
}

void XWnd::SelectFont(unsigned int file, unsigned int hash)
{
    EXBaseWnd::SelectFont(EXGeoFile::GetGeoFile(file), hash);
    m_FontHash = hash;
    m_FontFile = file;
    if (m_pFontTexture != m_pSprite2DTexture) _SelectSprite2DTexture(m_pFontTexture, 1, 0);
}

FEObject *FE::_Marker(int i) { return m_pMarker[i]; }

void FESound::Init() { m_RefCount = 0; }

void FESound::FadeUp(int time)
{
    if (!m_RefCount) EXBaseSoundManager::GetSoundManager()->FadeSFXUp(time);
    m_RefCount++;
}

void FESound::FadeDown(int time)
{
    if (--m_RefCount == 0) EXBaseSoundManager::GetSoundManager()->FadeSFXDown(time, 1);
}

void FEWnd::BringToTop()
{
    if (EXBaseDisplay::GetDisplay()->TopWnd() != this) EXBaseDisplay::GetDisplay()->WndToTop(this);
}

void FEWnd::SendToBottom()
{
    if (EXBaseDisplay::GetDisplay()->BottomWnd() != this) EXBaseDisplay::GetDisplay()->WndToBottom(this);
}

// NEAR: register allocation
FEObject *FEWnd::SetObject(FEObject *pObj)
{
    m_pObject = pObj;
    if (pObj) {
        pObj->SetRect(EXRect(0, 0, m_Rect.w, m_Rect.h));
        pObj->SetOwnerWnd(this);
        pObj->SetParent(0);
    }
    m_pObject->MathmosGoto(FEMathmosPos::MP_END);
    return pObj;
}

// NEAR: retail stores m_Rect field by field interleaved with loads
void FEWnd::SetRect(EXRect r)
{
    m_Rect = r;
    InvalidateExtents();
    if (m_pObject) m_pObject->SetRect(EXRect(0, 0, m_Rect.w, m_Rect.h));
}

// NEAR: retail loads m_State as a dword (and 0xffffff01), ours movsx
void FEWnd::HandleInput()
{
    if (m_pObject) {
        if (!m_pObject->HandleInput(m_State & 1)) FEListener::QueryButtons((FEPad::Flags)0xf);
    }
}

signed char FEWnd::Update()
{
    if (m_LastUpdateFrame != m_FrameCount) {
        if (m_pObject) m_pObject->Update();
        if (m_pObject) m_pObject->DoEventCallback((FEObjEvent::Flags)0x15);
        m_LastUpdateFrame = m_FrameCount;
    }
    return 1;
}

// NEAR: 20 bytes differ, register choice for m_pObject
void FEWnd::DebugDump()
{
    char buf[256];
    sprintf(buf, "---------- %s debug dump ----------\n", DebugName());
    if (m_pObject) m_pObject->DebugDump(0);
}

void FEWnd::Cleanup()
{
    delete m_pObject;
    m_pObject = 0;
    ParticleSysMgr()->DeleteAllSystems();
    m_State &= ~0x20;
}

void FE::_Pop()
{
    FEObject *p;
    m_Stack.Pop(p);
}

FEObject *FE::_Peek()
{
    FEObject *p = 0;
    m_Stack.Peek(p);
    return p;
}

void FE::_SetMarker(int i) { m_pMarker[i] = _Peek(); }

// NEAR: store scheduling of the field clears
void FEWnd::Init(FEObject *pObj)
{
    m_FrameCount = 0;
    m_LastUpdateFrame = 0;
    m_pObject = pObj;
    m_Flags = 6;
    m_DeleteNeeded = 0;
    m_ContentDestroyed = 0;
    m_State = 0x18000041;
    m_EnvironmentState = 1;
    SetRect(EXRect(0, 0, EXBaseDisplay::GetDisplay()->Width(), EXBaseDisplay::GetDisplay()->Height()));
    EXBaseDisplay::GetDisplay()->AddWnd(this);
}

void FEWnd::AlwaysOnTop()
{
    m_Flags.BitSet(0x1000);
    if (EXBaseDisplay::GetDisplay()->TopWnd() != this) EXBaseDisplay::GetDisplay()->WndToTop(this);
}

void FE::_Push(FEObject *p) { m_Stack.Push(p); }

// NEAR: Cleanup/ContentDestroy inline ordering
FEWnd::~FEWnd()
{
    if (EXBaseDisplay::GetDisplay()->IsWndVisible(this)) EXBaseDisplay::GetDisplay()->RemoveWnd(this);
    DrawSpriteDisable();
    DrawPolyDisable();
    ContentDestroy();
    Cleanup();
}

// NEAR: inlined EXStack::Pop/RemoveAt loop shape
void FE::_Init(FEObject *pRoot)
{
    FEObject *p;
    while (m_Stack.Pop(p)) {}
    if (pRoot) m_Stack.Push(pRoot);
}

FEBaseWnd::FEBaseWnd() : m_ClipStack(0x80)
{
    m_pSprites = 0;
    m_pPolys = 0;
    m_nSprites = 0;
    m_nPolys = 0;
    m_ClipRect.x = 0;
    m_ClipRect.y = 0;
    m_ClipRect.w = 0;
    m_ClipRect.h = 0;
    m_ExtentsValid = 0;
    for (int i = 0; i < 8; i++) m_Extents[i] = 0.0f;
}

FEWnd::FEWnd() : m_StateStack(0x80) { Init(0); }

FEWnd::FEWnd(FEObject *pObj) : m_StateStack(0x80) { Init(pObj); }

// Front-end text (XWnd): besides plain glyphs, "~c" draws controller button c's icon (scaled to the
// line height, greyed when the text colour is bright and the icon asks for it), "<c>" switches font
// (ProcessControlChar) and "\n" or a newline breaks the line. Lines wrap after a space or hyphen;
// a line that would overflow the rect's height ends in "...".

// Height of the first letter the font has, from 'A' on.
int XWnd::FontHeight()
{
    EXGeoFont *pFont = m_pFont;
    for (int c = 'A'; c < 'z'; c++) {
        EXGeoFontChar *pChar = pFont->Char((unsigned char)c);
        if (pChar) return pChar->m_Height;
    }
    return 0;
}

EXPoint XWnd::FontTextSize(const char *pText, float scale)
{
    s8 button = 0;
    int lineWidth = 0;
    int height = 0;
    int maxWidth = 0;
    int totalHeight = 0;
    int fh = FontHeight();
    if (m_pFont) {
        SetNormalFont();
        for (const char *p = pText; *p; p++) {
            char c = *p;
            EXGeoFontChar *pChar = m_pFont->Char((unsigned char)c);
            if (button) {
                XButton *pB = ((XGamePad *)EXBaseGamePad::GetGamePad())->LookupButton(c);
                height = (int)((float)fh * scale);
                lineWidth += (int)(pB->Graphic->m_Scale * (float)fh * scale);
                button = 0;
            } else if ((c == '\\' && p[1] == 'n') || c == '\n') {
                if (lineWidth > maxWidth) maxWidth = lineWidth;
                totalHeight += height;
                lineWidth = 0;
            } else if (c == '~') {
                button = 1;
            } else if (c == '<' && p[2] == '>') {
                ProcessControlChar(p[1]);
                p += 2;
            } else if (pChar) {
                int h = (int)(pChar->m_Height * scale);
                lineWidth += (int)(pChar->m_Width * scale);
                height = max(height, h);
            }
        }
        ResetNormalFont();
    }
    return EXPoint(max(lineWidth, maxWidth), totalHeight + height);
}

void XWnd::RenderFont(const char *pText, const XLineInfo &line, float scale)
{
    EXSprite2D s;
    s.m_Color = m_FontColor;
    unsigned int i = 0;
    unsigned int count = line.m_Count;
    u16 height = line.m_Height;
    s.x = line.x;
    s.y = line.y;
    s8 button = 0;
    const char *p = pText;
    for (; *p; p++, i++) {
        if (i >= count) break;
        char c = *p;
        EXGeoFontChar *pChar = m_pFont->Char((unsigned char)c);
        if (line.m_Ellipsis && i == count - 1) {
            EXGeoFontChar *pDot = m_pFont->Char('.');
            s.w = (u16)(pDot->m_Width * scale);
            s.h = (u16)(pDot->m_Height * scale);
            s.u0 = pDot->u0;
            s.v0 = pDot->v0;
            s.u1 = pDot->u1;
            s.v1 = pDot->v1;
            _DrawSprite2D(s);
            s.x += (int)((pDot->m_Width - pDot->m_Kern) * scale);
            _DrawSprite2D(s);
            s.x += (int)((pDot->m_Width - pDot->m_Kern) * scale);
            _DrawSprite2D(s);
        } else if (button) {
            XButton *pB = ((XGamePad *)EXBaseGamePad::GetGamePad())->LookupButton(c);
            ex::RGBA colour = s.m_Color;
            SelectSprite2DTexture(EXGeoFile::GetGeoFile(0x100003a), pB->Graphic->m_Texture, 0, 0);
            button = 0;
            if (pB->Graphic->m_Unkd && (s.m_Color.m_R > 0x40 || s.m_Color.m_G > 0x40 || s.m_Color.m_B > 0x40))
                s.m_Color = ex::RGBA(0x7f, 0x7f, 0x7f, s.m_Color.m_A);
            float h = (float)height;
            s.w = (u16)(h * pB->Graphic->m_Scale * scale);
            s.h = (u16)(h * scale);
            s.u0 = 0.0f;
            s.v0 = 0.0f;
            s.u1 = 1.0f;
            s.v1 = 1.0f;
            _DrawSprite2D(s);
            s.x += (int)(pB->Graphic->m_Scale * h * scale);
            s.m_Color = colour;
            _SelectSprite2DTexture(m_pFontTexture, 0, 0);
        } else if (c == '~') {
            button = 1;
        } else if (c == '<' && p[2] == '>') {
            i += 2;
            ProcessControlChar(p[1]);
            p += 2;
        } else if (pChar) {
            s.w = (u16)(pChar->m_Width * scale);
            s.h = (u16)(pChar->m_Height * scale);
            s.u0 = pChar->u0;
            s.v0 = pChar->v0;
            s.u1 = pChar->u1;
            s.v1 = pChar->v1;
            _DrawSprite2D(s);
            s.x += (int)((pChar->m_Width - pChar->m_Kern) * scale);
        }
    }
}

// Lines stack downwards from the top, centre or bottom of r; each starts at its indent.
void XWnd::CalculateAlignment(unsigned int n, const EXRect &r, XStringInfo *pInfo, EXW::TextAlign align, float scale)
{
    XLineInfo line = pInfo->m_Lines.GetAt(n - 1);
    int xc = (r.w - line.m_Width) >> 1;
    int above = 0;
    for (unsigned int k = 1; k < n; k++) {
        XLineInfo prev = pInfo->m_Lines.GetAt(k - 1);
        above += prev.m_Height;
    }
    switch (align) {
    case 2:
        line.x = r.x + line.m_Indent;
        line.y = (r.h >> 1) - (pInfo->m_TotalHeight >> 1) + r.y + above;
        break;
    case 1:
        line.x = r.x + line.m_Indent;
        line.y = r.h + r.y - line.m_Height + above;
        break;
    case 3:
        line.x = r.x + xc + line.m_Indent;
        line.y = r.y + above;
        break;
    case 4:
        line.x = r.x + xc + line.m_Indent;
        line.y = (r.h >> 1) - (pInfo->m_TotalHeight >> 1) + r.y + above;
        break;
    case 5:
        line.x = r.x + xc + line.m_Indent;
        line.y = r.h + r.y - line.m_Height + above;
        break;
    case 6:
        line.x = r.w - line.m_Width + r.x + line.m_Indent;
        line.y = r.y + above;
        break;
    case 7:
        line.x = r.w - line.m_Width + r.x + line.m_Indent;
        line.y = (r.h >> 1) - (pInfo->m_TotalHeight >> 1) + r.y + above;
        break;
    case 8:
        line.x = r.w - line.m_Width + r.x + line.m_Indent;
        line.y = r.h + r.y - line.m_Height + above;
        break;
    default:
        line.x = r.x + line.m_Indent;
        line.y = r.y + above;
        break;
    }
    pInfo->m_Lines.SetAt(n - 1, line);
}

void XWnd::GetStringProperties(const char *pText, const EXRect &r, XStringInfo *pInfo, EXW::TextAlign align,
                               float scale)
{
    int totalHeight = 0;
    s8 button = 0;
    const char *p = pText;
    int fh = FontHeight();
    XLineInfo line;
    memset(pInfo, 0, sizeof(XStringInfo));
    s8 newline = 0;
    memset(&line, 0, sizeof(line));
    SetNormalFont();
    pInfo->m_Lines.SetGrowBy(32);
    for (; *p; p++) {
        char c = *p;
        EXGeoFontChar *pChar = m_pFont->Char((unsigned char)c);
        if (button) {
            XButton *pB = ((XGamePad *)EXBaseGamePad::GetGamePad())->LookupButton(c);
            pInfo->m_Width += (int)(pB->Graphic->m_Scale * (float)fh * scale);
            button = 0;
            int h = (int)((float)fh * scale);
            pInfo->m_Height = max(pInfo->m_Height, h);
        } else if ((c == '\\' && p[1] == 'n') || c == '\n') {
            newline = 1;
            if (c == '\\') p++;
        } else if (c == '~') {
            button = 1;
        } else if (c == '<' && p[2] == '>') {
            ProcessControlChar(p[1]);
            p += 2;
        } else if (pChar) {
            pInfo->m_Width += (int)((pChar->m_Width - pChar->m_Kern) * scale);
            unsigned int h = (unsigned int)(pChar->m_Height * scale);
            pInfo->m_Height = max((unsigned int)pInfo->m_Height, h);
        }
    }
    ResetNormalFont();
    if (pInfo->m_Width < r.w && !newline) {
        line.m_Width = pInfo->m_Width;
        line.m_Count = strlen(pText);
        pInfo->m_MaxWidth = pInfo->m_Width;
        line.m_Height = pInfo->m_Height;
        pInfo->m_TotalHeight = pInfo->m_Height;
        pInfo->m_NumLines = 1;
        pInfo->m_Lines.Add(line, 1);
        CalculateAlignment(1, r, pInfo, align, scale);
        return;
    }
    s8 bullet = 0;
    const char *pLine = pText;
    button = 0;
    int breakWidth = 0;
    int i = 0;
    int indent = 0;
    int breakIndex = 0;
    int lineHeight = 0;
    int lineWidth = 0;
    p = pText;
    while (*p) {
        char c = *p;
        EXGeoFontChar *pChar = m_pFont->Char((unsigned char)c);
        if (c == ' ' || c == '-') {
            breakIndex = i + 1;
            breakWidth = (int)((pChar->m_Width - pChar->m_Kern) * scale) + lineWidth;
        }
        if ((c == '\\' && p[1] == 'n') || c == '\n') {
            line.m_Width = lineWidth;
            int fh2 = FontHeight();
            line.m_Height = max(lineHeight, fh2);
            line.m_Count = i;
            line.m_Skip = (c != '\n') + 1;
            if (lineWidth > pInfo->m_MaxWidth) pInfo->m_MaxWidth = lineWidth;
            lineWidth = 0;
            totalHeight += lineHeight;
            lineHeight = 0;
            breakIndex = 0;
            pLine += i + line.m_Skip;
            i = 0;
            if (totalHeight > r.h && *pLine) line.m_Ellipsis = 1;
            pInfo->m_TotalHeight += line.m_Height;
            pInfo->m_Lines.Add(line, 1);
            pInfo->m_NumLines++;
            if (totalHeight > r.h) break;
            bullet = 0;
            p = pLine;
            line.m_Indent = 0;
        } else if (button) {
            XButton *pB = ((XGamePad *)EXBaseGamePad::GetGamePad())->LookupButton(c);
            button = 0;
            int fh2 = FontHeight();
            lineWidth += (int)((float)max(lineHeight, fh2) * pB->Graphic->m_Scale * scale);
            if (*p == '.') {
                bullet = 1;
                indent = 0;
            }
            p++;
            i++;
            if (indent + lineWidth >= r.w) {
                line.m_Width = breakWidth;
                line.m_Height = lineHeight;
                line.m_Count = breakIndex;
                line.m_Skip = 0;
                if (lineWidth > pInfo->m_MaxWidth) pInfo->m_MaxWidth = lineWidth;
                totalHeight += lineHeight;
                pLine += breakIndex;
                i = 0;
                lineHeight = 0;
                lineWidth = 0;
                breakIndex = 0;
                if (totalHeight > r.h && *pLine) line.m_Ellipsis = 1;
                pInfo->m_TotalHeight += line.m_Height;
                pInfo->m_Lines.Add(line, 1);
                pInfo->m_NumLines++;
                if (totalHeight > r.h) break;
                line.m_Indent = bullet ? indent : 0;
                p = pLine;
            }
        } else if (c == '~') {
            i++;
            p++;
            button = 1;
        } else if (c == '<' && p[2] == '>') {
            ProcessControlChar(p[1]);
            i += 3;
            p += 3;
        } else if (!pChar) {
            p++;
            i++;
        } else {
            if (bullet && !indent) indent = lineWidth;
            lineWidth += (int)((pChar->m_Width - pChar->m_Kern) * scale);
            i++;
            lineHeight = max((u16)lineHeight, pChar->m_Height);
            p++;
            if (indent + lineWidth >= r.w) {
                int newTotal = lineHeight + totalHeight;
                if (newTotal >= r.h) {
                    if (p[-1] == ' ' || p[-1] == '-')
                        line.m_Count = i - 3;
                    else
                        line.m_Count = i - 2;
                    lineWidth -= (int)((pChar->m_Width - pChar->m_Kern) * scale);
                    line.m_Width = lineWidth;
                } else if (breakWidth > 0) {
                    line.m_Width = breakWidth;
                    line.m_Count = breakIndex;
                } else {
                    lineWidth -= (int)((pChar->m_Width - pChar->m_Kern) * scale);
                    line.m_Count = i - 1;
                    line.m_Width = lineWidth;
                }
                line.m_Skip = 0;
                line.m_Height = lineHeight;
                if (indent + breakWidth > pInfo->m_MaxWidth) pInfo->m_MaxWidth = breakWidth;
                lineHeight = 0;
                pLine += line.m_Count;
                breakIndex = 0;
                i = 0;
                lineWidth = 0;
                totalHeight = newTotal;
                breakWidth = 0;
                if (totalHeight > r.h && *pLine) line.m_Ellipsis = 1;
                pInfo->m_TotalHeight += line.m_Height;
                pInfo->m_Lines.Add(line, 1);
                pInfo->m_NumLines++;
                if (totalHeight > r.h) break;
                line.m_Indent = bullet ? indent : 0;
                p = pLine;
            }
        }
    }
    if (lineWidth && totalHeight < r.h && lineHeight + totalHeight <= r.h) {
        line.m_Width = lineWidth;
        line.m_Height = lineHeight;
        line.m_Count = strlen(pLine);
        line.m_Skip = 0;
        if (breakWidth > pInfo->m_MaxWidth) pInfo->m_MaxWidth = breakWidth;
        pInfo->m_TotalHeight += line.m_Height;
        pInfo->m_Lines.Add(line, 1);
        pInfo->m_NumLines++;
    }
    for (int n = 1; n <= pInfo->m_NumLines; n++) CalculateAlignment(n, r, pInfo, align, scale);
    ResetNormalFont();
}

void XWnd::FontPrint(short x, short y, const char *pText, float scale, EXW::TextAlign align, signed char b)
{
    const char *p = pText;
    XStringInfo info;
    if (!m_pFont || !m_pFontTexture) return;
    EXRect r(x, y, m_Rect.w, m_Rect.h);
    GetStringProperties(p, r, &info, align, scale);
    if (!info.m_Width) return;
    EXPoint size = FontTextSize(p, 1.0f);
    SetNormalFont();
    for (int i = 0; i < info.m_NumLines; i++) {
        XLineInfo line = info.m_Lines[i];
        RenderFont(p, line, scale);
        p += line.m_Count + line.m_Skip;
    }
    ResetNormalFont();
}

void XWnd::FontPrint(const char *pText, const EXRect &r, float scale, EXW::TextAlign align, signed char b)
{
    XStringInfo info;
    if (!m_pFont || !m_pFontTexture) return;
    GetStringProperties(pText, r, &info, align, scale);
    EXPoint size = FontTextSize(pText, 1.0f);
    SetNormalFont();
    for (int i = 0; i < info.m_NumLines; i++) {
        XLineInfo line = info.m_Lines[i];
        RenderFont(pText, line, scale);
        pText += line.m_Count + line.m_Skip;
    }
    ResetNormalFont();
    info.m_Lines.RemoveAll();
}

EXPoint XWnd::FontTextSize(const char *pText, EXRect &r, float scale)
{
    XStringInfo info;
    int w = 0;
    int h = 0;
    if (m_pFont) {
        GetStringProperties(pText, r, &info, (EXW::TextAlign)0, scale);
        w = info.m_MaxWidth;
        h = info.m_TotalHeight;
    }
    return EXPoint(w, h);
}

void xbMovieSetVolume(int vol);

void FEBaseWnd::MovieOpen(char *pName)
{
    char buf[32];
    sprintf(buf, "%s", pName);
    EXWnd::MovieOpen(buf, 0, 0, 640, 480);
    xbMovieSetVolume(GameData::Instance(1)->m_Values.m_MovieVolume);
}

void FEBaseWnd::DrawScript(FERenderPass::Flags pass, ex::RGBA c, FEObjectScript *pScript)
{
    CacheSprite(pass, 0, 0, c, 0, EXRect(0, 0, 0, 0), EXVector2(0.0f, 0.0f), EXVector2(0.0f, 0.0f), FE_Blank, (EXW::TextAlign)4, 0,
                pScript, 0, 0);
}

void FEBaseWnd::DrawSprite(FERenderPass::Flags pass, unsigned int file, unsigned int hash, ex::RGBA c, unsigned int blend, EXRect r,
                           EXVector2 uv0, EXVector2 uv1)
{
    CacheSprite(pass, file, hash, c, blend, r, uv0, uv1, FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0);
}

void FEBaseWnd::DrawText(FERenderPass::Flags pass, unsigned int file, unsigned int hash, ex::RGBA c, unsigned int blend, EXRect r,
                         EXString s, EXW::TextAlign align, unsigned int n)
{
    if (file && hash)
        CacheSprite(pass, file, hash, c, blend, r, EXVector2(0.0f, 0.0f), EXVector2(0.0f, 0.0f), s, align, 0, 0, 0, n);
}

void FEBaseWnd::DrawText(FERenderPass::Flags pass, unsigned int file, unsigned int hash, ex::RGBA c, unsigned int blend, EXPoint p,
                         EXString s, EXW::TextAlign align, unsigned int n)
{
    if (file && hash)
        CacheSprite(pass, file, hash, c, blend, EXRect(p.x, p.y, 0, 0), EXVector2(0.0f, 0.0f), EXVector2(0.0f, 0.0f), s, align, 0, 0, 0, n);
}

void FEBaseWnd::DrawSprite(FERenderPass::Flags pass, unsigned int file, unsigned int hash, ex::RGBA c, unsigned int blend,
                           FEObject *pObj, unsigned int datum, EXPoint size, EXW::TextAlign align)
{
    if (file && hash)
        CacheSprite(pass, file, hash, c, blend, EXRect(0, 0, size.x, size.y), EXVector2(0.0f, 0.0f), EXVector2(1.0f, 1.0f), FE_Blank,
                    align, pObj, 0, datum, 0);
}

void FEBaseWnd::DrawText(FERenderPass::Flags pass, unsigned int file, unsigned int hash, ex::RGBA c, unsigned int blend,
                         FEObject *pObj, unsigned int datum, EXString s, EXW::TextAlign align, unsigned int n)
{
    if (file && hash)
        CacheSprite(pass, file, hash, c, blend, EXRect(0, 0, 0, 0), EXVector2(0.0f, 0.0f), EXVector2(0.0f, 0.0f), s, align, pObj, 0,
                    datum, n);
}

void FEBaseWnd::DrawSprite(FERenderPass::Flags pass, unsigned int file, unsigned int hash, ex::RGBA c, unsigned int blend, EXPoint p,
                           EXPoint size, EXW::TextAlign align)
{
    if (file && hash)
        CacheSprite(pass, file, hash, c, blend, AlignPointToPoint(p, size, align), EXVector2(0.0f, 0.0f), EXVector2(1.0f, 1.0f),
                    FE_Blank, align, 0, 0, 0, 0);
}

void FEBaseWnd::DrawPoly(unsigned int file, unsigned int hash, ex::RGBA c, unsigned int blend, FEObject *pObj, unsigned int a,
                         unsigned int b, unsigned int cc, unsigned int d, EXVector2 uv, signed char flag)
{
    CachePoly(file, hash, c, blend, pObj, a, b, cc, d, EXVector(0.0f, 0.0f, 0.0f, 0.0f), EXVector(0.0f, 0.0f, 0.0f, 0.0f),
              EXVector(0.0f, 0.0f, 0.0f, 0.0f), EXVector(0.0f, 0.0f, 0.0f, 0.0f), uv, flag);
}

void FEBaseWnd::DrawPoly(unsigned int file, unsigned int hash, ex::RGBA c, unsigned int blend, EXVector v0, EXVector v1, EXVector v2,
                         EXVector v3, EXVector2 uv, signed char flag)
{
    CachePoly(file, hash, c, blend, 0, 0, 0, 0, 0, v0, v1, v2, v3, uv, flag);
}

EXVector FEBaseWnd::WndToWorld(EXPoint p)
{
    CalculateExtents();
    const float &w = m_Extents[4] - m_Extents[0];
    const float &h = m_Extents[5] - m_Extents[1];
    EXVector v;
    v.x = (float)p.x / (float)m_Rect.w * w + m_Extents[0];
    v.y = (float)p.y / (float)m_Rect.h * h + m_Extents[1];
    v.z = 150.0f;
    v.w = 0.0f;
    return v;
}

EXRect FEBaseWnd::AlignPointToPoint(EXPoint p, EXPoint s, EXW::TextAlign align)
{
    EXRect r;
    switch (align) {
    case 0:
        r.x = p.x;
        r.y = p.y;
        break;
    case 3:
        r.x = p.x - s.x / 2;
        r.y = p.y;
        break;
    case 6:
        r.x = p.x - s.x;
        r.y = p.y;
        break;
    case 2:
        r.x = p.x;
        r.y = p.y - s.y / 2;
        break;
    case 4:
        r.x = p.x - s.x / 2;
        r.y = p.y - s.y / 2;
        break;
    case 7:
        r.x = p.x - s.x;
        r.y = p.y - s.y / 2;
        break;
    case 1:
        r.x = p.x;
        r.y = p.y - s.y;
        break;
    case 5:
        r.x = p.x - s.x / 2;
        r.y = p.y - s.y;
        break;
    case 8:
        r.x = p.x - s.x;
        r.y = p.y - s.y;
        break;
    }
    r.w = s.x;
    r.h = s.y;
    return r;
}

signed char FEWnd::Draw()
{
    if (m_State & 0x10000000) SetupCamera(1);
    m_FrameCount++;
    if (!m_DeleteNeeded) {
        EXWnd *prev = EXBaseDisplay::ActiveWnd();
        EXBaseDisplay::SetActiveWnd(this);
        s8 ofog = 0;
        s8 fog = 0;
        if (m_EnvironmentState & 1) {
            ofog = EXBaseRenderEnv::OverrideFogEnabled();
            fog = EXBaseRenderEnv::FogEnabled();
            EXBaseRenderEnv::SetOverrideFogEnabled(0);
            EXBaseRenderEnv::SetFogEnabled(0);
        }
        if (m_pObject && (m_State & 0x40)) {
            m_pObject->Draw();
            RenderSprites((FERenderPass::Flags)0);
            RenderSprites((FERenderPass::Flags)1);
            RenderSprites((FERenderPass::Flags)2);
            RenderSprites((FERenderPass::Flags)3);
            RenderPolys();
        }
        RenderSprites((FERenderPass::Flags)4);
        if (m_EnvironmentState & 1) {
            EXBaseRenderEnv::SetFogEnabled(fog);
            EXBaseRenderEnv::SetOverrideFogEnabled(ofog);
        }
        EXBaseDisplay::SetActiveWnd(prev);
        return 1;
    }
    m_Flags.m_flags |= 0x10000;
    return 1;
}

EXPoint FEBaseWnd::AlignPointToRect(EXRect r, EXW::TextAlign align)
{
    EXPoint p;
    switch (align) {
    case 0:
        p.x = r.x;
        p.y = r.y;
        break;
    case 3:
        p.x = r.x + r.w / 2;
        p.y = r.y;
        break;
    case 6:
        p.x = r.x + r.w;
        p.y = r.y;
        break;
    case 2:
        p.x = r.x;
        p.y = r.y + r.h / 2;
        break;
    case 4:
        p.x = r.x + r.w / 2;
        p.y = r.y + r.h / 2;
        break;
    case 7:
        p.x = r.x + r.w;
        p.y = r.y + r.h / 2;
        break;
    case 1:
        p.x = r.x;
        p.y = r.y + r.h;
        break;
    case 5:
        p.x = r.x + r.w / 2;
        p.y = r.y + r.h;
        break;
    case 8:
        p.x = r.x + r.w;
        p.y = r.y + r.h;
        break;
    }
    return p;
}

EXRect FEBaseWnd::AlignRectToRect(EXRect a, EXRect b, EXW::TextAlign align)
{
    switch (align) {
    case 0:
        return EXRect(b.x, b.y, a.w, a.h);
    case 3:
        return EXRect(b.w / 2 - a.w / 2 + b.x, b.y, a.w, a.h);
    case 6:
        return EXRect(b.w - a.w + b.x, b.y, a.w, a.h);
    case 2:
        return EXRect(b.x, b.h / 2 - a.h / 2 + b.y, a.w, a.h);
    case 4:
        return EXRect(b.w / 2 - a.w / 2 + b.x, b.h / 2 - a.h / 2 + b.y, a.w, a.h);
    case 7:
        return EXRect(b.w - a.w + b.x, b.h / 2 - a.h / 2 + b.y, a.w, a.h);
    case 1:
        return EXRect(b.x, b.h - a.h + b.y, a.w, a.h);
    case 5:
        return EXRect(b.w / 2 - a.w / 2 + b.x, b.h - a.h + b.y, a.w, a.h);
    case 8:
        return EXRect(b.w - a.w + b.x, b.h - a.h + b.y, a.w, a.h);
    }
    return a;
}

void FEWnd::FixupZOrder()
{
    if (FEHud::Instance(0)) {
        FEHud *p = FEHud::Instance(1);
        if (EXBaseDisplay::GetDisplay()->TopWnd() != p) EXBaseDisplay::GetDisplay()->WndToTop(p);
    }
    if (FERadar::Instance(0)) {
        FERadar *p = FERadar::Instance(1);
        if (EXBaseDisplay::GetDisplay()->TopWnd() != p) EXBaseDisplay::GetDisplay()->WndToTop(p);
    }
    if (FECutscene::Instance(0)) {
        FECutscene *p = FECutscene::Instance(1);
        if (EXBaseDisplay::GetDisplay()->TopWnd() != p) EXBaseDisplay::GetDisplay()->WndToTop(p);
    }
    if (FELoading::Instance(0)) {
        FELoading *p = FELoading::Instance(1);
        if (EXBaseDisplay::GetDisplay()->TopWnd() != p) EXBaseDisplay::GetDisplay()->WndToTop(p);
    }
    if (FEPause::Instance(0)) {
        FEPause *p = FEPause::Instance(1);
        if (EXBaseDisplay::GetDisplay()->TopWnd() != p) EXBaseDisplay::GetDisplay()->WndToTop(p);
    }
    if (FEDialog::Instance(0)) {
        FEDialog *p = FEDialog::Instance(1);
        if (EXBaseDisplay::GetDisplay()->TopWnd() != p) EXBaseDisplay::GetDisplay()->WndToTop(p);
    }
    GameDialogWnd *w = GameControllerRemovedSys::Wnd();
    if (w && EXBaseDisplay::GetDisplay()->TopWnd() != w) EXBaseDisplay::GetDisplay()->WndToTop(w);
}

void FEBaseWnd::CacheSprite(FERenderPass::Flags pass, unsigned int file, unsigned int hash, ex::RGBA c, unsigned int blend, EXRect r,
                            EXVector2 uv0, EXVector2 uv1, EXString text, EXW::TextAlign align, FEObject *pObj,
                            FEObjectScript *pScript, unsigned int datum, unsigned int b)
{
    DrawSpriteEnable();
    if (m_nSprites >= 0x100) return;
    m_pSprites[m_nSprites].m_Pass = pass;
    m_pSprites[m_nSprites].m_File = file;
    m_pSprites[m_nSprites].m_Hash = hash;
    m_pSprites[m_nSprites].m_Colour = c;
    m_pSprites[m_nSprites].m_Blend = blend;
    m_pSprites[m_nSprites].m_Rect = r;
    m_pSprites[m_nSprites].m_UV0 = uv0;
    m_pSprites[m_nSprites].m_UV1 = uv1;
    m_pSprites[m_nSprites].m_Text = text;
    m_pSprites[m_nSprites].m_Align = align;
    m_pSprites[m_nSprites].m_pObject = pObj;
    m_pSprites[m_nSprites].m_pScript = pScript;
    m_pSprites[m_nSprites].m_Datum = datum;
    m_pSprites[m_nSprites].m_ClipRect = m_ClipRect;
    m_pSprites[m_nSprites].m_Unk54 = b;
    m_nSprites++;
}

void FEBaseWnd::CachePoly(unsigned int file, unsigned int hash, ex::RGBA c, unsigned int blend, FEObject *pObj, unsigned int a,
                          unsigned int b, unsigned int cc, unsigned int d, EXVector v0, EXVector v1, EXVector v2, EXVector v3,
                          EXVector2 uv, signed char flag)
{
    DrawPolyEnable();
    if (m_nPolys >= 0x20) return;
    m_pPolys[m_nPolys].m_File = file;
    m_pPolys[m_nPolys].m_Hash = hash;
    m_pPolys[m_nPolys].m_Verts[0].m_Pos = v0;
    m_pPolys[m_nPolys].m_Verts[0].m_Colour = c;
    m_pPolys[m_nPolys].m_Verts[1].m_Pos = v1;
    m_pPolys[m_nPolys].m_Verts[1].m_Colour = c;
    m_pPolys[m_nPolys].m_Verts[2].m_Pos = v2;
    m_pPolys[m_nPolys].m_Verts[2].m_Colour = c;
    m_pPolys[m_nPolys].m_Verts[3].m_Pos = v3;
    m_pPolys[m_nPolys].m_Verts[3].m_Colour = c;
    m_pPolys[m_nPolys].m_A = a;
    m_pPolys[m_nPolys].m_B = b;
    m_pPolys[m_nPolys].m_C = cc;
    m_pPolys[m_nPolys].m_D = d;
    m_pPolys[m_nPolys].m_UV = uv;
    m_pPolys[m_nPolys].m_Blend = blend;
    m_pPolys[m_nPolys].m_pObject = pObj;
    m_pPolys[m_nPolys].m_Flag = flag;
    m_nPolys++;
}

// Flush the cached quads: resolve datum-driven corners, rebuild the four verts from the two
// UV scale factors (m_Flag picks which edge the quad is anchored to) and draw each as a fan.
void FEBaseWnd::RenderPolys()
{
    SetTriFanZMode(1, 1);
    for (unsigned int n = 0; n < m_nPolys; n++) {
        FEBaseWndPoly &p = m_pPolys[n];
        if (!(p.m_UV.x > 0.0f) || !(p.m_UV.y > 0.0f)) continue;
        EXGeoFile *pFile = EXGeoFile::GetGeoFile(p.m_File);
        EXGeoHeader *pHdr = pFile ? pFile->GeoHeader() : 0;
        EXTexture *pTex = pHdr->Texture(p.m_Hash);
        if (!pTex) continue;

        unsigned int *datums = &p.m_A;
        for (int i = 0; i < 4; i++) {
            if (p.m_pObject && datums[i]) {
                p.m_Verts[i].m_Pos = p.m_pObject->FindDatum(datums[i]);
            }
        }

        float ux = p.m_UV.x, uy = p.m_UV.y;
        FEBaseWndPoly::Vert &v0 = p.m_Verts[0];
        FEBaseWndPoly::Vert &v1 = p.m_Verts[1];
        FEBaseWndPoly::Vert &v2 = p.m_Verts[2];
        FEBaseWndPoly::Vert &v3 = p.m_Verts[3];
        float ax, ay, az, aw;   // anchor corner
        float bx, by;           // moving corner
        float ex, ey;           // edge vector
        float u0;
        if (p.m_Flag == 0) {
            ax = v0.m_Pos.x; ay = v0.m_Pos.y; az = v0.m_Pos.z; aw = v0.m_Pos.w;
            ex = (v3.m_Pos.x - ax) * uy;
            ey = (v3.m_Pos.y - ay) * uy;
            bx = (v1.m_Pos.x - ax) * ux + ax;
            by = (v1.m_Pos.y - ay) * ux + ay;
            v0.m_Pos.x = ax; v0.m_Pos.y = ay; v0.m_Pos.z = az; v0.m_Pos.w = aw;
            v0.m_U = 0.0f; v0.m_V = 0.0f;
            v1.m_Pos.x = bx; v1.m_Pos.y = by; v1.m_Pos.z = az; v1.m_Pos.w = aw;
            v1.m_U = ux; v1.m_V = 0.0f;
            v2.m_Pos.x = bx + ex; v2.m_Pos.y = by + ey; v2.m_Pos.z = az; v2.m_Pos.w = aw;
            v2.m_U = ux; v2.m_V = uy;
            v3.m_Pos.x = ax + ex; v3.m_Pos.y = ay + ey; v3.m_Pos.z = az; v3.m_Pos.w = aw;
            v3.m_U = 0.0f; v3.m_V = uy;
        } else {
            ax = v1.m_Pos.x; ay = v1.m_Pos.y; az = v1.m_Pos.z; aw = v1.m_Pos.w;
            ex = (v2.m_Pos.x - ax) * uy;
            ey = (v2.m_Pos.y - ay) * uy;
            bx = (v0.m_Pos.x - ax) * ux + ax;
            by = (v0.m_Pos.y - ay) * ux + ay;
            u0 = 1.0f - ux;
            v0.m_Pos.x = bx; v0.m_Pos.y = by; v0.m_Pos.z = az; v0.m_Pos.w = aw;
            v0.m_U = u0; v0.m_V = 0.0f;
            v1.m_Pos.x = ax; v1.m_Pos.y = ay; v1.m_Pos.z = az; v1.m_Pos.w = aw;
            v1.m_U = 1.0f; v1.m_V = 0.0f;
            v2.m_Pos.x = ax + ex; v2.m_Pos.y = ay + ey; v2.m_Pos.z = az; v2.m_Pos.w = aw;
            v2.m_U = 1.0f; v2.m_V = uy;
            v3.m_Pos.x = bx + ex; v3.m_Pos.y = by + ey; v3.m_Pos.z = az; v3.m_Pos.w = aw;
            v3.m_U = u0; v3.m_V = uy;
        }
        SetTriFanBlendMode(p.m_Blend);
        SelectTriFanTexture(pTex);
        DrawTriFan(4, (EXPrimVert *)p.m_Verts);
    }
    m_nPolys = 0;
}

extern unsigned int gSystemCameraUser;

void FEBaseWnd::SetupCamera(signed char force)
{
    m_Camera.SetZRange(0.1f, 200.0f);
    if (gSystemCameraUser != 2 || force) {
        m_Camera.SetAspectCorrection(1);
        m_Origin = EXVector2(m_Rect.w * 0.5f, m_Rect.h * 0.5f);
        m_Camera.SetOrigin(m_Origin);
        m_Camera.SetPosition(EXVector(0.0f, 0.0f, 0.0f, 0.0f));
        m_Camera.SetTarget(EXVector(0.0f, 0.0f, 1.0f, 0.0f));
        m_Camera.SetVFOV(EXRectf((float)m_PrimRect.x, (float)m_PrimRect.y, (float)m_PrimRect.w, (float)m_PrimRect.h), 0.06981317f);
        EXBaseDisplay *pDisp = EXBaseDisplay::GetDisplay();
        m_Camera.SetClipSize(pDisp->Width(), pDisp->Height(), 0);
        m_Camera.UpdateDisplay(0);
        gSystemCameraUser = 2;
    }
}

EXPoint FEBaseWnd::WorldToWnd(EXVector v)
{
    SetupCamera(0);
    EXVector2 d = EXBaseDisplay::WorldToDisp(v);
    return EXPoint((int)(d.x - (float)(m_Rect.x - 1)), (int)(d.y - (float)(m_Rect.y - 1)));
}

void FEBaseWnd::Debug_Circle2D(EXPoint p, float r, unsigned int n, ex::RGBA c, signed char b)
{
    EXVector v(0.0f, r, 0.0f, 0.0f);
    EXMatrix m;
    m.set_rotate_z(6.2831855f / (float)n);
    for (unsigned int i = 0; i < n; i++) {
        EXVector w = m.EXMaths::mat_44::operator*(v);
        v = w;
    }
}

void FEBaseWnd::CalculateExtents()
{
    if (m_ExtentsValid) return;
    EXPoint corners[2];
    corners[0] = EXPoint(0, 0);
    corners[1] = EXPoint(m_Rect.w, m_Rect.h);
    m_ExtentsValid = 1;
    SetupCamera(0);
    for (int i = 0; i < 2; i++) {
        int tx = corners[i].x;
        int ty = corners[i].y;
        float x = 0.0f;
        float y = 0.0f;
        float sx = 1.0f;
        float sy = 1.0f;
        int n = 0;
        do {
            n++;
            EXPoint p = WorldToWnd(EXVector(x, y, 150.0f, 0.0f));
            if (p.x < tx) {
                if (sx > 0.0f)
                    x += sx;
                else
                    sx *= -0.5f;
            }
            if (p.x > tx) {
                if (sx < 0.0f)
                    x += sx;
                else
                    sx *= -0.5f;
            }
            if (p.x == tx) sx = 0.0f;
            if (p.y < ty) {
                if (sy > 0.0f)
                    y -= sy;
                else
                    sy *= -0.5f;
            }
            if (p.y > ty) {
                if (sy < 0.0f)
                    y -= sy;
                else
                    sy *= -0.5f;
            }
            if (p.y == ty) sy = 0.0f;
            if (sx == 0.0f && sy == 0.0f) {
                m_Extents[i * 4] = x;
                m_Extents[i * 4 + 1] = y;
                m_Extents[i * 4 + 2] = 150.0f;
                m_Extents[i * 4 + 3] = 0.0f;
                break;
            }
        } while (n < 50);
    }
}

void FEBaseWnd::DrawBorderSprite(FERenderPass::Flags pass, unsigned int file, unsigned int hash, ex::RGBA c, unsigned int blend,
                                 EXRect r, unsigned int border)
{
    if (!file || !hash) return;
    int b = border;
    CacheSprite(pass, file, hash, c, blend, EXRect(r.x, r.y, b, b), EXVector2(0.0f, 0.0f), EXVector2(0.5f, 0.5f), FE_Blank,
                (EXW::TextAlign)4, 0, 0, 0, 0);
    CacheSprite(pass, file, hash, c, blend, EXRect(r.x + b, r.y, r.w - (b + b), b), EXVector2(0.5f, 0.0f), EXVector2(0.5f, 0.5f),
                FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0);
    CacheSprite(pass, file, hash, c, blend, EXRect(r.x + (r.w - b), r.y, b, b), EXVector2(0.5f, 0.0f), EXVector2(1.0f, 0.5f),
                FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0);
    CacheSprite(pass, file, hash, c, blend, EXRect(r.x, r.y + b, b, r.h - (b + b)), EXVector2(0.0f, 0.5f), EXVector2(0.5f, 0.5f),
                FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0);
    CacheSprite(pass, file, hash, c, blend, EXRect(r.x + b, r.y + b, r.w - (b + b), r.h - (b + b)), EXVector2(0.5f, 0.5f),
                EXVector2(0.5f, 0.5f), FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0);
    CacheSprite(pass, file, hash, c, blend, EXRect(r.x + (r.w - b), r.y + b, b, r.h - (b + b)), EXVector2(0.5f, 0.5f),
                EXVector2(1.0f, 0.5f), FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0);
    CacheSprite(pass, file, hash, c, blend, EXRect(r.x, r.y + (r.h - b), b, b), EXVector2(0.0f, 0.5f), EXVector2(0.5f, 1.0f),
                FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0);
    CacheSprite(pass, file, hash, c, blend, EXRect(r.x + b, r.y + (r.h - b), r.w - (b + b), b), EXVector2(0.5f, 0.5f),
                EXVector2(0.5f, 1.0f), FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0);
    CacheSprite(pass, file, hash, c, blend, EXRect(r.x + (r.w - b), r.y + (r.h - b), b, b), EXVector2(0.5f, 0.5f),
                EXVector2(1.0f, 1.0f), FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0);
}

inline void EXBaseWnd::DrawSprite2D(ex::RGBA c, const EXRect &r, const EXVector2 &uv0, const EXVector2 &uv1)
{
    EXSprite2D s;
    s.m_Color = c;
    s.x = r.x;
    s.y = r.y;
    s.w = r.w;
    s.h = r.h;
    s.u0 = uv0.x;
    s.v0 = uv0.y;
    s.u1 = uv1.x;
    s.v1 = uv1.y;
    _DrawSprite2D(s);
}

void FEBaseWnd::RenderSprites(FERenderPass::Flags pass)
{
    for (unsigned int i = 0; i < m_nSprites; i++) {
        if (pass != m_pSprites[i].m_Pass) continue;
        if (m_pSprites[i].m_ClipRect.w == 0 && m_pSprites[i].m_ClipRect.h == 0) {
            m_Sprite2DClipRect = m_Rect;
            _SetSprite2DClipRect(0, 0, m_Rect.w, m_Rect.h);
        } else {
            EXRect &c = m_pSprites[i].m_ClipRect;
            m_Sprite2DClipRect = c;
            _SetSprite2DClipRect(c.x, c.y, c.w, c.h);
        }
        if (m_pSprites[i].m_pObject && m_pSprites[i].m_Datum) {
            EXPoint p = m_pSprites[i].m_pObject->FindDatum2D(m_pSprites[i].m_Datum);
            m_pSprites[i].m_Rect = AlignPointToPoint(p, EXPoint(m_pSprites[i].m_Rect.w, m_pSprites[i].m_Rect.h), m_pSprites[i].m_Align);
            m_pSprites[i].m_Rect = m_pSprites[i].m_pObject->OutRect(m_pSprites[i].m_Rect);
        }
        if (m_pSprites[i].m_pScript) {
            m_pSprites[i].m_pScript->Update();
            m_pSprites[i].m_pScript->Render(m_pSprites[i].m_Colour);
        } else if (m_pSprites[i].m_Text.Length() > 0) {
            SelectFont(m_pSprites[i].m_File, m_pSprites[i].m_Hash);
            if (m_pSprites[i].m_Rect.w == 0 && m_pSprites[i].m_Rect.h == 0) {
                EXRect r(40, 40, EXBaseDisplay::GetDisplay()->Width() - 80, EXBaseDisplay::GetDisplay()->Height() - 80);
                EXPoint size = FontTextSize(m_pSprites[i].m_Text, r, 1.0f) + EXPoint(10, 10);
                m_pSprites[i].m_Rect = AlignPointToPoint(EXPoint(m_pSprites[i].m_Rect.x, m_pSprites[i].m_Rect.y),
                                                         size, m_pSprites[i].m_Align);
            }
            SetSprite2DBlendMode(m_pSprites[i].m_Blend);
            if (m_pSprites[i].m_Blend == 0) {
                m_FontColor = ex::RGBA(0, 0, 0, m_pSprites[i].m_Colour.m_A);
                FontPrint(m_pSprites[i].m_Text, EXRect(m_pSprites[i].m_Rect.x + 1, m_pSprites[i].m_Rect.y + 1, m_pSprites[i].m_Rect.w,
                                                       m_pSprites[i].m_Rect.h),
                          1.0f, m_pSprites[i].m_Align, 0);
            }
            SetFontColor(m_pSprites[i].m_Colour);
            FontPrint(m_pSprites[i].m_Text, m_pSprites[i].m_Rect, 1.0f, m_pSprites[i].m_Align, 0);
        } else {
            SetSprite2DBlendMode(m_pSprites[i].m_Blend);
            EXGeoFile *pFile = EXGeoFile::GetGeoFile(m_pSprites[i].m_File);
            SelectSprite2DTexture(pFile, m_pSprites[i].m_Hash, 1, 0);
            DrawSprite2D(m_pSprites[i].m_Colour, m_pSprites[i].m_Rect, m_pSprites[i].m_UV0, m_pSprites[i].m_UV1);
        }
    }
    if (pass == 4) {
        m_nSprites = 0;
        EXRect r;
        while (m_ClipStack.Pop(r)) {}
        m_ClipRect.Set(0, 0, 0, 0);
        m_Sprite2DClipRect = m_Rect;
        _SetSprite2DClipRect(0, 0, m_Rect.w, m_Rect.h);
    }
}
