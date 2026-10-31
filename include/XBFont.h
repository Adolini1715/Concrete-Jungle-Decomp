#ifndef XBFONT_H
#define XBFONT_H
#include "types.h"
#include "xbd3d8.h"
#include "RGBA.h"

// Screen font vertex: position, colour, uv (24 bytes).
struct FONT2DVERTEX {
    FONT2DVERTEX() {}
    xb::D3DXVECTOR3 p;
    xb::DWORD color;
    float tu, tv;
};
inline FONT2DVERTEX InitFont2DVertex(const xb::D3DXVECTOR3 &p, xb::DWORD color, float tu, float tv)
{
    FONT2DVERTEX v;
    v.p = p;
    v.color = color;
    v.tu = tu;
    v.tv = tv;
    return v;
}

// Glyph rectangle in the font texture plus placement (28 bytes, chars from ' ').
struct XBFONTGLYPH {
    int m_Left, m_Top, m_Right, m_Bottom;
    int m_Offset;   // 0x10 pen offset before the glyph
    int m_Width;    // 0x14
    int m_Advance;  // 0x18 pen advance after the glyph
};

// Debug text font after the XDK sample font (XBFont.obj): text queued with DrawText is drawn by
// DrawAllText as triangle lists of up to 250 triangles per call.
class CXBFont {
public:
    CXBFont()
    {
        m_pd3dDevice = 0;
        m_pFontTexture = 0;
        m_pVB = 0;
        m_5dcc = 0;
        m_5dd0 = 0;
        m_dwFontHeight = 0x24;
        m_dwTexWidth = 0x40;
        m_dwTexHeight = 0x40;
        m_pHeader = 0;
        m_pGlyphs = 0;
        m_nStrings = 0;
        m_fScale = 0.75f;
    }
    long Create(xb::D3DDevice *pDevice, char *name);
    long Destroy();
    void StartText();
    long DrawText(float x, float y, unsigned long color, const char *text, float z);
    long ScaleHeight(unsigned long n);
    long DrawAllText();
    void EndText();
    void SetScale(float f) { m_fScale = f; }

    FONT2DVERTEX m_Verts[1000];         // 0x0000
    xb::D3DDevice *m_pd3dDevice;        // 0x5dc0
    xb::D3DTexture *m_pFontTexture;     // 0x5dc4
    xb::D3DVertexBuffer *m_pVB;         // 0x5dc8
    u32 m_5dcc;
    u32 m_5dd0;
    u32 m_dwFontHeight;                 // 0x5dd4
    u32 m_dwTexWidth;                   // 0x5dd8
    u32 m_dwTexHeight;                  // 0x5ddc
    u32 *m_pHeader;                     // 0x5de0 copied font header (count at 0, glyph count at 8)
    XBFONTGLYPH *m_pGlyphs;             // 0x5de4
    u8 m_5de8[0x20];
    char m_Text[0x5000];                // 0x5e08 queued strings, each ended by '\r'
    int m_bInText;                      // 0xae08
    int m_ae0c;
    u32 m_Colours[50];                  // 0xae10
    int m_nStrings;                     // 0xaed8
    float m_X[50];                      // 0xaedc
    float m_Y[50];                      // 0xafa4
    float m_Z[50];                      // 0xb06c
    float m_fScale;                     // 0xb134
};

class _XBFont {
public:
    void XBInitFont();
    void XBAddSprite(int x, int y, float w, float h, unsigned int tex);
    void __cdecl XBPrintAt(int x, int y, ex::RGBA col, char *fmt, ...);
    void XBRenderFont();

    u8 m_00[0x14];
    xb::D3DTexture *m_pTexture;         // 0x14
    u8 m_18[0x601c - 0x18];
    int m_601c;
    float m_fScale;                     // 0x6020
};
extern _XBFont XBFont;
extern CXBFont *g_SrFont;
#endif
