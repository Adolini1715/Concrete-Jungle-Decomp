#include "XBFont.h"
#include "XBBaseRenderState.h"
#include "XBBaseVertexShader.h"
#include "EXDisplay.h"
#include "EXRect.h"
#include "EXMemory.h"
#include <stdio.h>
#include <stdarg.h>

extern "C" void __stdcall XGSwizzleRect(const void *pSource, xb::DWORD Pitch, const void *pRect, void *pDest,
                                        xb::DWORD Width, xb::DWORD Height, const void *pPoint,
                                        xb::DWORD BytesPerPixel);
long D3dCreateTexture(unsigned int w, unsigned int h, unsigned int levels, unsigned long usage, xb::D3DFORMAT fmt,
                      unsigned long pool, xb::D3DTexture **ppTex);
extern XBBaseRenderState XBRenderState;
extern XBBaseVertexShader XBVertexShader;
extern unsigned int MrListyData[];  // built-in font: header, glyphs, then the texture

char g_DebugPrintbuf[0x5000];
_XBFont XBFont;
CXBFont *g_SrFont;

static inline EXDisplay *Display() { return (EXDisplay *)EXBaseDisplay::GetDisplay(); }

long CXBFont::Create(xb::D3DDevice *pDevice, char *name)
{
    m_Text[0] = 0;
    m_ae0c = 0;
    m_dwFontHeight = MrListyData[1];
    m_dwTexWidth = MrListyData[2];
    m_dwTexHeight = MrListyData[3];
    u32 n = MrListyData[5];
    m_pHeader = (u32 *)_EXAlloc(n * 20, 0);
    m_pHeader[0] = n;
    u8 *pDst = (u8 *)(m_pHeader + 1);
    const u8 *pSrc = (const u8 *)&MrListyData[6];
    for (u32 i = 0; i < n - 4; i++) *pDst++ = *pSrc++;
    u8 *pGlyph = (u8 *)_EXAlloc(m_pHeader[2] * sizeof(XBFONTGLYPH), 0);
    m_pGlyphs = (XBFONTGLYPH *)pGlyph;
    for (u32 j = 0; j < m_pHeader[2] * sizeof(XBFONTGLYPH); j++) *pGlyph++ = *pSrc++;
    m_dwTexHeight >>= 1;
    long hr = D3dCreateTexture(m_dwTexWidth, m_dwTexHeight, 1, 0, (xb::D3DFORMAT)4, 1, &m_pFontTexture);
    if (hr < 0) return hr;
    xb::D3DLOCKED_RECT lr;
    m_pFontTexture->LockRect(0, &lr, 0, 0);
    XGSwizzleRect(pSrc, 0, 0, lr.pBits, m_dwTexWidth, m_dwTexHeight, 0, 2);
    hr = xb::D3DDevice::CreateVertexBuffer(0x11940, 0, 0, 0, &m_pVB);
    if (hr < 0) return hr;
    m_pd3dDevice = pDevice;
    m_bInText = 0;
    m_nStrings = 0;
    return 0;
}

long CXBFont::Destroy()
{
    if (!m_ae0c) {
        m_5dcc = 0;
        m_5dd0 = 0;
        if (m_pFontTexture) {
            m_pFontTexture->Release();
            m_pFontTexture = 0;
        }
        if (m_pVB) {
            m_pVB->Release();
            m_pVB = 0;
        }
        if (m_pHeader) {
            EXFree(m_pHeader);
            m_pHeader = 0;
        }
        if (m_pGlyphs) {
            EXFree(m_pGlyphs);
            m_pGlyphs = 0;
        }
    }
    return 0;
}

void CXBFont::StartText()
{
    if (m_bInText) return;
    m_bInText = 1;
    m_Text[0] = 0;
    XBRenderState.SetTexture(0, m_pFontTexture);
    m_nStrings = 0;
}

long CXBFont::DrawText(float x, float y, unsigned long color, const char *text, float z)
{
    if (!m_bInText) StartText();
    m_Colours[m_nStrings] = color;
    m_X[m_nStrings] = x;
    m_Y[m_nStrings] = y;
    m_Z[m_nStrings] = z;
    m_nStrings++;
    sprintf(m_Text, "%s%s\r", m_Text, text);
    return 0;
}

long CXBFont::ScaleHeight(unsigned long n)
{
    m_dwFontHeight *= n;
    return 0;
}

long CXBFont::DrawAllText()
{
    XBRenderState.SetTexture(0, m_pFontTexture);
    m_nStrings = 0;
    float sy = m_Y[0];
    xb::DWORD color = m_Colours[0];
    float sx = m_X[0];
    const char *str = m_Text;
    FONT2DVERTEX *v = m_Verts;
    u32 nTris = 0;
    char c;
    while ((c = *str++) != 0) {
        if (c == '\r') {
            m_nStrings++;
            sy = m_Y[m_nStrings];
            color = m_Colours[m_nStrings];
            sx = m_X[m_nStrings];
        }
        if (sx > 600.0f) {
            sy = (XBFont.m_fScale * 1.3333334f) * 10.0f + sy;
            color = m_Colours[m_nStrings];
            sx = m_X[m_nStrings];
        }
        if (c == '\n') {
            sy = (XBFont.m_fScale * 1.3333334f) * 10.0f + sy;
            color = m_Colours[m_nStrings];
            sx = m_X[m_nStrings];
            continue;
        }
        if (c < ' ') continue;
        float h = (float)m_dwFontHeight * m_fScale;
        float w = (float)m_pGlyphs[c - ' '].m_Width * m_fScale;
        float tx1 = (float)m_pGlyphs[c - ' '].m_Left / (float)m_dwTexWidth;
        float ty2 = (float)m_pGlyphs[c - ' '].m_Bottom / (float)m_dwTexHeight;
        float ty1 = (float)m_pGlyphs[c - ' '].m_Top / (float)m_dwTexHeight;
        sx = (float)m_pGlyphs[c - ' '].m_Offset * m_fScale + sx;
        float tx2 = (float)m_pGlyphs[c - ' '].m_Right / (float)m_dwTexWidth;
        float x1 = w + sx;
        float y1 = h + sy;
        float z = m_Z[m_nStrings];
        *v++ = InitFont2DVertex(xb::D3DXVECTOR3(sx, y1, z), color, tx1, ty2);
        *v++ = InitFont2DVertex(xb::D3DXVECTOR3(sx, sy, z), color, tx1, ty1);
        *v++ = InitFont2DVertex(xb::D3DXVECTOR3(x1, y1, z), color, tx2, ty2);
        *v++ = InitFont2DVertex(xb::D3DXVECTOR3(x1, sy, z), color, tx2, ty1);
        *v++ = InitFont2DVertex(xb::D3DXVECTOR3(x1, y1, z), color, tx2, ty2);
        *v++ = InitFont2DVertex(xb::D3DXVECTOR3(sx, sy, z), color, tx1, ty1);
        nTris += 2;
        if (nTris > 250) {
            D3DDevice_DrawVerticesUP(5, nTris * 3, m_Verts, sizeof(FONT2DVERTEX));
            nTris = 0;
            v = m_Verts;
        }
        sx = (float)m_pGlyphs[c - ' '].m_Advance * m_fScale + sx;
    }
    if (nTris) D3DDevice_DrawVerticesUP(5, nTris * 3, m_Verts, sizeof(FONT2DVERTEX));
    m_nStrings = 0;
    return 0;
}

void CXBFont::EndText()
{
    m_bInText = 0;
    DrawAllText();
    m_Text[m_nStrings] = 0;
}

void _XBFont::XBInitFont()
{
    m_fScale = 0.75f;
    g_SrFont = new CXBFont;
    g_SrFont->Create(Display()->Device(), "hmm");
}

void _XBFont::XBAddSprite(int x, int y, float w, float h, unsigned int tex)
{
}

void __cdecl _XBFont::XBPrintAt(int x, int y, ex::RGBA col, char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsprintf(g_DebugPrintbuf, fmt, args);
    if (!g_SrFont) XBInitFont();
    g_SrFont->DrawText((float)x, (float)y, col.argb(), g_DebugPrintbuf, XBVertexShader.m_ScreenZ);
}

void _XBFont::XBRenderFont()
{
    g_SrFont->m_fScale = m_fScale;
    EXRect r(0, 0, 640, 480);
    Display()->SetViewPort(&r, 0);
    XBRenderState.SetTexture(0, m_pTexture);
    XBRenderState.SetTexture(1, 0);
    XBRenderState.SetTextureStageState(1, (xb::D3DTEXTURESTAGESTATETYPE)12, 1);
    XBVertexShader.SetFullGlobalDiffuse();
    XBVertexShader.SetVertexShader(6);
    XBRenderState.SetRenderState((xb::D3DRENDERSTATETYPE)0x3b, 1);
    XBRenderState.SetRenderState((xb::D3DRENDERSTATETYPE)0x8f, 0);
    XBRenderState.SetRenderState((xb::D3DRENDERSTATETYPE)0x4a, 0x8006);
    XBRenderState.SetRenderState((xb::D3DRENDERSTATETYPE)0x3e, 0x302);
    XBRenderState.SetRenderState((xb::D3DRENDERSTATETYPE)0x3f, 0x303);
    g_SrFont->EndText();
    g_SrFont->DrawAllText();
    g_SrFont->StartText();
    XBRenderState.SetTexture(0, 0);
    XBRenderState.SetRenderState((xb::D3DRENDERSTATETYPE)0x8f, 2);
    XBRenderState.SetRenderState((xb::D3DRENDERSTATETYPE)0x3b, 1);
    m_601c = 0;
}
