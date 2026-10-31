#include "EXDisplay.h"
#include "EXBaseWnd.h"
#include "EXApp.h"
#include "EXBaseRenderEnv.h"
#include "EXGeoEntity.h"
#include "XBBaseRenderState.h"
#include "XBBaseVertexShader.h"
#include "XBBasePixelShader.h"
#include "XBBaseBenchMark.h"
#include "XBGeoDrawInfo.h"
#include "xbd3d8.h"
#include "RGBA.h"
#include <string.h>

// The Xbox display: device creation and reset, the default render state, render targets and
// the viewport (vertex shader viewport constants plus scissors), frame begin/end (presentation
// interval, gamma ramp, safe-frame outline), Present, and the debug font.

long D3dCreateTexture(unsigned int w, unsigned int h, unsigned int levels, unsigned long usage, xb::D3DFORMAT fmt,
                      unsigned long pool, xb::D3DTexture **ppTex);
extern "C" xb::DWORD __stdcall XGSwizzleRect(const void *pSource, xb::DWORD Pitch, const xb::tagRECT *pRect,
                                             void *pDest, xb::DWORD Width, xb::DWORD Height, const void *pPoint,
                                             xb::DWORD BytesPerPixel);
extern "C" xb::DWORD __stdcall XGetVideoFlags();
extern "C" xb::DWORD __stdcall XGetVideoStandard();
extern "C" xb::DWORD __stdcall XGetLanguage();

// Xbox dashboard language to engine language.
struct XbToExLang {
    unsigned int m_Xbox;
    unsigned char m_Ex;
};
XbToExLang g_XboxToExLanguageMapping[10] = {
    {1, 0}, {4, 6}, {2, 2}, {5, 11}, {3, 7}, {9, 10}, {6, 8}, {7, 13}, {8, 16}, {0xff, 0xff},
};
unsigned long uiFiltermethod = 3;
unsigned int uiFilterMode;
unsigned int uiKeyFLock;

// Screen-space vertices for the debug character and the 2D line (drawn from static memory).
struct XBDebugCharVert {
    float x, y, z, w;
    unsigned int diffuse;
    float u, v;
};
struct XBLineVert {
    float x, y, z;
    unsigned int diffuse;
};
static XBDebugCharVert s_DebugChar[4];
static XBLineVert s_Line[2];

extern EXFixedArray<unsigned char, 4097> EXDbgFont;   // 128x256 1-bit font, rows bottom-up

s8 EXDisplay::ms_UpdWorldToScreen;

static inline EXDisplay *Display() { return (EXDisplay *)EXBaseDisplay::GetDisplay(); }

// Viewport: the vertex shaders' screen transform (c-38 scale, c-37 offset; c72 is all ones) and
// the scissor rectangle, clamped to the current render target. With no clip rectangle the
// viewport itself is the clip.
__forceinline s8 EXDisplay::SetViewPortInline(const EXRect *pRect, const EXRect *pClip)
{
    xb::_D3DRECT rc;
    if (!pClip) {
        float hh = (float)pRect->h * 0.5f;
        xb::D3DXVECTOR4 one(1.0f, 1.0f, 1.0f, 1.0f);
        xb::D3DXVECTOR4 scale(hh, -hh, 16777215.0f, 1.0f);
        xb::D3DXVECTOR4 offset((float)pRect->w * 0.5f + (float)pRect->x + 0.53125f, (float)pRect->y + hh + 0.53125f,
                               0.0f, 0.0f);
        xb::D3DDevice::SetVertexShaderConstantFast(72, &one, 1);
        xb::D3DDevice::SetVertexShaderConstantFast(-38, &scale, 1);
        xb::D3DDevice::SetVertexShaderConstantFast(-37, &offset, 1);
        pClip = pRect;
    }
    int x = pClip->x, y = pClip->y, w = pClip->w, h = pClip->h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (w <= 0) w = 1;
    if (h <= 0) h = 1;
    if (x > m_TargetRect.w - 1) x = m_TargetRect.w - 1;
    if (y > m_TargetRect.h - 1) y = m_TargetRect.h - 1;
    if (x + w > m_TargetRect.w) w = m_TargetRect.w - x;
    if (y + h > m_TargetRect.h) h = m_TargetRect.h - y;
    rc.x1 = x;
    rc.y1 = y;
    rc.x2 = x + w;
    rc.y2 = y + h;
    xb::D3DDevice::SetScissors(1, 0, &rc);
    return 1;
}

s8 EXDisplay::SetViewPort(const EXRect *pRect, const EXRect *pClip)
{
    xb::_D3DRECT rc;
    if (!pClip) {
        pClip = pRect;
        xb::D3DXVECTOR4 one(1.0f, 1.0f, 1.0f, 1.0f);
        float hh = (float)pClip->h * 0.5f;
        xb::D3DXVECTOR4 scale(hh, -hh, 16777215.0f, 1.0f);
        xb::D3DXVECTOR4 offset((float)pClip->w * 0.5f + (float)pClip->x + 0.53125f, (float)pClip->y + hh + 0.53125f,
                               0.0f, 0.0f);
        xb::D3DDevice::SetVertexShaderConstantFast(72, &one, 1);
        xb::D3DDevice::SetVertexShaderConstantFast(-38, &scale, 1);
        xb::D3DDevice::SetVertexShaderConstantFast(-37, &offset, 1);
    }
    int x = pClip->x, y = pClip->y, w = pClip->w, h = pClip->h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (w <= 0) w = 1;
    if (h <= 0) h = 1;
    if (x > m_TargetRect.w - 1) x = m_TargetRect.w - 1;
    if (y > m_TargetRect.h - 1) y = m_TargetRect.h - 1;
    if (x + w > m_TargetRect.w) w = m_TargetRect.w - x;
    if (y + h > m_TargetRect.h) h = m_TargetRect.h - y;
    rc.x1 = x;
    rc.y1 = y;
    rc.x2 = x + w;
    rc.y2 = y + h;
    xb::D3DDevice::SetScissors(1, 0, &rc);
    return 1;
}

void XBVerticalBlankCallback(xb::_D3DVBLANKDATA *)
{
    Display()->IncVerticalBlanks();
}

void EXDisplay::SelectDebugTexture() { XBRenderState.SetTexture(0, m_pDebugFont); }

void EXDisplay::SelectNullTexture() { XBRenderState.SetTexture(0, m_pNullTexture); }

void EXDisplay::SelectNullTextureBlack() { XBRenderState.SetTexture(0, m_pNullTextureBlack); }

// A 16x16 opaque white texture (also restores colour writes to RGB).
xb::D3DTexture *EXDisplay::NullTexture()
{
    XBRenderState.SetRenderState(xb::D3DRS_COLORWRITEENABLE, 0x10101);
    return m_pNullTexture;
}

xb::D3DTexture *EXDisplay::NullTextureBlack()
{
    XBRenderState.SetRenderState(xb::D3DRS_COLORWRITEENABLE, 0x10101);
    return m_pNullTextureBlack;
}

s8 EXDisplay::Present()
{
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_MAXANISOTROPY, 3);
    XBRenderState.SetTextureStageState(1, xb::D3DTSS_MAXANISOTROPY, 0);
    g_uiShadowEntities = 0;
    g_uiShadowedEntities = 0;
    xb::D3DDevice::Present(0, 0, 0, 0);
    return 1;
}

EXString EXDisplay::CaptureToFile() { return EXString(""); }

s8 EXDisplay::InitNullTexture()
{
    if (m_pNullTexture == 0) {
        if (D3dCreateTexture(16, 16, 1, 0, xb::D3DFMT_A8R8G8B8, 0, &m_pNullTexture)) return 0;
        xb::D3DTexture *pTex = m_pNullTexture;
        if (pTex) {
            xb::D3DLOCKED_RECT lr;
            pTex->LockRect(0, &lr, 0, 0x20);
            memset(lr.pBits, 0xff, 16 * 16 * 4);
            pTex->UnlockRect(0);
        }
    }
    return 1;
}

s8 EXDisplay::InitNullTextureBlack()
{
    if (m_pNullTextureBlack == 0) {
        if (D3dCreateTexture(16, 16, 1, 0, xb::D3DFMT_A8R8G8B8, 0, &m_pNullTextureBlack)) return 0;
        xb::D3DTexture *pTex = m_pNullTextureBlack;
        if (pTex) {
            xb::D3DLOCKED_RECT lr;
            pTex->LockRect(0, &lr, 0, 0x20);
            unsigned int *p = (unsigned int *)lr.pBits;
            for (int i = 0; i < 16 * 16; i++) p[i] = 0xff000000;
            pTex->UnlockRect(0);
        }
    }
    return 1;
}

// One 12x12 character from the debug font at a screen position (a four-vertex fan).
void EXDisplay::RenderDebugChar(unsigned char c, const EXVector &pos, const ex::RGBA &col)
{
    for (int i = 0; i < 4; i++) {
        s_DebugChar[i].z = pos.z;
        s_DebugChar[i].w = pos.w;
        s_DebugChar[i].diffuse = col.argb();
    }
    s_DebugChar[0].x = pos.x;
    s_DebugChar[0].y = pos.y;
    s_DebugChar[0].u = m_DebugFontU[c];
    s_DebugChar[0].v = m_DebugFontV[c];
    s_DebugChar[1].x = pos.x + 12.0f;
    s_DebugChar[1].y = pos.y;
    s_DebugChar[1].u = m_DebugFontU[c] + 0.09375f;
    s_DebugChar[1].v = m_DebugFontV[c];
    s_DebugChar[2].x = pos.x + 12.0f;
    s_DebugChar[2].y = pos.y + 12.0f;
    s_DebugChar[2].u = m_DebugFontU[c] + 0.09375f;
    s_DebugChar[2].v = m_DebugFontV[c] + 0.046875f;
    s_DebugChar[3].x = pos.x;
    s_DebugChar[3].y = pos.y + 12.0f;
    s_DebugChar[3].u = m_DebugFontU[c];
    s_DebugChar[3].v = m_DebugFontV[c] + 0.046875f;
    XBRenderState.SetRenderState(xb::D3DRS_ZENABLE, 2);
    XBVertexShader.SetVertexShader(12);
    xb::D3DDevice::DrawPrimitiveUP(xb::D3DPT_TRIANGLEFAN, 2, s_DebugChar, sizeof(XBDebugCharVert));
}

s8 EXDisplay::OpenDefault()
{
    EXPresentParams p;
    p.m_FBWidth = 640;
    p.m_FBHeight = 480;
    p.m_Unk04 = 32;
    p.m_Width = 640;
    p.m_Height = 480;
    p.m_Unk0a = 32;
    p.m_Unk0c = 24;
    p.m_Unk0e = 1;
    p.m_Unk10 = 0;
    return Open(p);
}

// Copy the requested mode into the present parameters and set the frame rates, PAL 60, wide
// screen, progressive scan and the language from the dashboard settings.
s8 EXDisplay::SetUp(const EXPresentParams &params)
{
    m_Params = params;
    xb::D3DFORMAT fmt = xb::D3DFMT_A8R8G8B8;
    if (m_Params.m_Unk0a == 16) fmt = (xb::D3DFORMAT)5;
    unsigned long zfmt = 0x2a;
    if (m_Params.m_Unk0c == 16) zfmt = 0x2c;
    memset(&m_PresentParams, 0, sizeof m_PresentParams);
    m_PresentParams.BackBufferWidth = m_Params.m_FBWidth;
    m_PresentParams.BackBufferHeight = m_Params.m_FBHeight;
    m_PresentParams.BackBufferFormat = fmt;
    m_PresentParams.BackBufferCount = 1;
    m_PresentParams.MultiSampleType = 0x11;
    m_PresentParams.SwapEffect = 1;
    m_PresentParams.EnableAutoDepthStencil = 1;
    m_PresentParams.AutoDepthStencilFormat = zfmt;
    m_PresentParams.Flags = 0;
    m_PresentParams.FullScreen_PresentationInterval = m_Params.m_VSync ? 1 : 0x80000000;
    m_HalfWidth = (float)m_PresentParams.BackBufferWidth * 0.5f;
    m_HalfHeight = (float)m_PresentParams.BackBufferHeight * 0.5f;
    SetupSafeFrame();
    xb::DWORD flags = XGetVideoFlags();
    EXBaseApp *pApp = EXBaseApp::GetApp();
    pApp->SetPal60(0);
    m_PresentParams.FullScreen_RefreshRateInHz = 60;
    EXBaseApp::SetVideoFrameRate(60);
    EXBaseApp::SetMaxFrameRate(60);
    EXBaseApp::SetMinFrameRate(30);
    if (XGetVideoStandard() == 3) {
        m_PresentParams.FullScreen_RefreshRateInHz = 50;
        EXBaseApp::SetVideoFrameRate(50);
        EXBaseApp::SetMaxFrameRate(50);
        EXBaseApp::SetMinFrameRate(25);
    }
    if (flags & 0x40) {
        m_PresentParams.FullScreen_RefreshRateInHz = 60;
        pApp->SetPal60(1);
        EXBaseApp::SetVideoFrameRate(60);
        EXBaseApp::SetMaxFrameRate(60);
        EXBaseApp::SetMinFrameRate(30);
    }
    if (flags & 1) {
        m_PresentParams.Flags |= 0x10;
        pApp->SetWideScreen(1);
        EXBaseRenderEnv::SetWideScreen(1);
    }
    if (flags & 8) m_PresentParams.Flags |= 0x40;
    unsigned int lang = XGetLanguage();
    int i = -1;
    unsigned int x;
    do {
        x = g_XboxToExLanguageMapping[++i].m_Xbox;
        if (lang == x) break;
    } while (x != 0xff);
    if (x != 0xff) pApp->SetLanguage(g_XboxToExLanguageMapping[i].m_Ex);
    m_TargetRect.x = 0;
    m_TargetRect.y = 0;
    m_TargetRect.w = m_Params.m_FBWidth;
    m_TargetRect.h = m_Params.m_FBHeight;
    return 1;
}

void EXDisplay::SetDefaultRenderState() {}

void EXDisplay::SetDefaultXBoxRenderState()
{
    XBRenderState.SetRenderState(xb::D3DRS_ZENABLE, 2);
    XBRenderState.SetRenderState(xb::D3DRS_ZWRITEENABLE, 1);
    XBRenderState.SetRenderState(xb::D3DRS_ZFUNC, 0x203);
    XBRenderState.SetRenderState(xb::D3DRS_ALPHATESTENABLE, 1);
    XBRenderState.SetRenderState(xb::D3DRS_ALPHAFUNC, 0x204);
    XBRenderState.SetRenderState(xb::D3DRS_ALPHAREF, 1);
    XBRenderState.SetRenderState(xb::D3DRS_ALPHABLENDENABLE, 1);
    XBRenderState.SetRenderState(xb::D3DRS_FOGENABLE, 0);
    XBRenderState.SetRenderState(xb::D3DRS_BLENDOP, 0x8006);
    XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 0x302);
    XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 0x303);
    XBRenderState.SetRenderState(xb::D3DRS_BLENDCOLOR, 0xffffffff);
    XBRenderState.SetRenderState(xb::D3DRS_DONOTCULLUNCOMPRESSED, 1);
    XBRenderState.SetRenderState(xb::D3DRS_OCCLUSIONCULLENABLE, 1);
    XBRenderState.SetRenderState(xb::D3DRS_SPECULARENABLE, 0);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_COLORARG1, 2);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_COLOROP, 5);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_COLORARG2, 0);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ALPHAARG1, 2);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ALPHAOP, 4);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ALPHAARG2, 0);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_MAGFILTER, 2);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_MINFILTER, 2);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_MIPFILTER, 2);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ADDRESSU, 1);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ADDRESSV, 1);
    XBRenderState.SetTextureStageState(1, xb::D3DTSS_COLORARG1, 2);
    XBRenderState.SetTextureStageState(1, xb::D3DTSS_COLOROP, 4);
    XBRenderState.SetTextureStageState(1, xb::D3DTSS_COLORARG2, 1);
    XBRenderState.SetTextureStageState(1, xb::D3DTSS_ALPHAARG1, 2);
    XBRenderState.SetTextureStageState(1, xb::D3DTSS_ALPHAOP, 4);
    XBRenderState.SetTextureStageState(1, xb::D3DTSS_ALPHAARG2, 1);
    XBRenderState.SetTextureStageState(1, xb::D3DTSS_ADDRESSU, 4);
    XBRenderState.SetTextureStageState(1, xb::D3DTSS_ADDRESSV, 4);
    XBRenderState.SetTextureStageState(1, xb::D3DTSS_BORDERCOLOR, 0xffffffff);
    XBRenderState.SetTextureStageState(1, xb::D3DTSS_MAGFILTER, 2);
    XBRenderState.SetTextureStageState(1, xb::D3DTSS_MINFILTER, 2);
    XBRenderState.SetTextureStageState(1, xb::D3DTSS_MIPFILTER, 2);
    XBRenderState.SetTexture(1, 0);
    XBRenderState.SetTexture(2, 0);
    XBRenderState.SetTextureStageState(2, xb::D3DTSS_BORDERCOLOR, 0);
    XBRenderState.SetTextureStageState(2, xb::D3DTSS_ADDRESSU, 4);
    XBRenderState.SetTextureStageState(2, xb::D3DTSS_ADDRESSV, 4);
    XBRenderState.SetTextureStageState(2, xb::D3DTSS_MAGFILTER, 2);
    XBRenderState.SetTextureStageState(2, xb::D3DTSS_MINFILTER, 2);
    XBRenderState.SetTextureStageState(2, xb::D3DTSS_MIPFILTER, 2);
    XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 0x302);
    XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 0x303);
    XBRenderState.SetRenderState(xb::D3DRS_BLENDOP, 0x8006);
    XBRenderState.SetRenderState(xb::D3DRS_LIGHTING, 0);
    float bias = 0.0f;
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_MIPMAPLODBIAS, *(xb::DWORD *)&bias);
    XBRenderState.SetRenderState(xb::D3DRS_DITHERENABLE, 0);
    xb::D3DDevice::SetDepthClipPlanes(0.0f, 16777215.0f, 1);
}

void EXDisplay::WaitDrawSync()
{
    if (m_PresentParams.FullScreen_PresentationInterval != 0x80000000) xb::D3DDevice::BlockUntilVerticalBlank();
}

// Unbind every stream, index buffer, texture and palette so window memory can be freed.
void EXDisplay::FlushAllWindowMem()
{
    xb::D3DDevice::BlockUntilIdle();
    xb::D3DIndexBuffer *pNoIB = 0;
    xb::D3DVertexBuffer *pNoVB = 0;
    xb::D3DDevice::SetIndices(pNoIB, 0);
    xb::D3DDevice::SetStreamSource(0, pNoVB, 0);
    xb::D3DDevice::SetStreamSource(1, pNoVB, 0);
    xb::D3DDevice::SetStreamSource(2, pNoVB, 0);
    xb::D3DDevice::SetStreamSource(3, pNoVB, 0);
    xb::D3DDevice::SetStreamSource(4, pNoVB, 0);
    xb::D3DDevice::SetStreamSource(5, pNoVB, 0);
    xb::D3DDevice::SetStreamSource(6, pNoVB, 0);
    xb::D3DDevice::SetStreamSource(7, pNoVB, 0);
    xb::D3DDevice::SetStreamSource(8, pNoVB, 0);
    xb::D3DDevice::SetStreamSource(9, pNoVB, 0);
    XBRenderState.SetTexture(0, 0);
    XBRenderState.SetTexture(1, 0);
    XBRenderState.SetTexture(2, 0);
    xb::D3DDevice::SetPalette(0, 0);
    xb::D3DDevice::SetPalette(1, 0);
    xb::D3DDevice::SetPalette(2, 0);
}

// Every render target change goes through here: bind, then reset the viewport and scissors to
// the whole surface.
s8 EXDisplay::SetRenderTarget(xb::D3DSurface *pColour, xb::D3DSurface *pDepth)
{
    xb::D3DDevice::SetRenderTarget(pColour, pDepth);
    xb::D3DSURFACE_DESC desc;
    pColour->GetDesc(&desc);
    m_TargetRect.Set(0, 0, desc.Width, desc.Height);
    SetViewPortInline(&m_TargetRect, 0);
    return 1;
}

XBRenderTargetManager::XBRenderTargetManager()
{
    m_pDefaultDepth = 0;
    m_pDefaultColour = 0;
    m_pSurface = 0;
    m_pTexture = 0;
    m_pDevice = 0;
}

void XBRenderTargetManager::Initialise(xb::D3DDevice *pDevice)
{
    m_pDevice = pDevice;
    m_pDevice->GetDepthStencilSurface(&m_pDefaultDepth);
    m_pDefaultDepth->Release();
    m_pTexture = 0;
    m_pSurface = 0;
}

void XBRenderTargetManager::Set(xb::D3DTexture *pTex)
{
    if (pTex != m_pTexture) {
        if (m_pSurface) m_pSurface->Release();
        pTex->GetSurfaceLevel(0, &m_pSurface);
        Display()->SetRenderTarget(m_pSurface, 0);
        m_pTexture = pTex;
    }
}

void XBRenderTargetManager::SetDefault()
{
    m_pDevice->GetRenderTarget(&m_pDefaultColour);
    m_pDefaultColour->Release();
}

void EXDisplay::AddD3DResource(EXD3DResource *p)
{
    m_D3DResources.AddTail(p ? (EXDListItem *)&p->m_pNext : 0);
}

void EXDisplay::RemD3DResource(EXD3DResource *p)
{
    m_D3DResources.Remove(p ? (EXDListItem *)&p->m_pNext : 0);
}

s8 EXDisplay::ReleaseD3DResource(EXDListItem *pItem)
{
    EXD3DResource *p = pItem ? (EXD3DResource *)((u8 *)pItem - 4) : 0;
    p->ReleaseD3DResource();
    return 1;
}

void EXDisplay::ReleaseD3DResources()
{
    if (m_pDebugFont) {
        D3DResource_Release(m_pDebugFont);
        m_pDebugFont = 0;
    }
    m_D3DResources.Apply(ReleaseD3DResource, 0);
}

// Box against the view volume: the four (x, y) corners of the box (min at pBox[0..2], max at
// pBox[4..6]) at both z extents, each transformed and given a 6-bit outcode (bits 1/2 x, 4/8 y,
// 0x10/0x20 z; high side first in each pair); corner i's code goes in byte i of a near (min z)
// and a far (max z) word. -1: every corner inside; 0: all eight outside one plane; 1: crossing.
static __forceinline u8 OutCode(const EXMaths::vec_xyzw &p)
{
    float w = (float)fabs(p.w);
    float nw = 0.0f - w;
    u8 code = 0;
    if (nw > p.x)
        code = 2;
    else if (p.x > w)
        code = 1;
    if (nw > p.y)
        code |= 8;
    else if (p.y > w)
        code |= 4;
    if (nw > p.z)
        code |= 0x20;
    else if (p.z > w)
        code |= 0x10;
    return code;
}

// NEAR: 4 bytes differ, the code = 0 (xor al, al) is scheduled two instructions early.
int EXDisplay::CullBox(const float *pBox, const EXMaths::mat_44 &m)
{
    u32 codesMin = 0;
    u32 codesMax = 0;
    float minZ = pBox[2];
    float maxZ = pBox[6];
    for (int i = 0; i < 4; i++) {
        int shift = i << 3;
        EXMaths::vec_xyzw v;
        v.x = pBox[(i & 1) * 4];
        v.y = pBox[(i & 2) * 2 + 1];
        v.z = minZ;
        v.w = 1.0f;
        EXMaths::vec_xyzw p = m * v;
        codesMin |= (OutCode(p) & 0x3f) << shift;
        v.z = maxZ;
        p = m * v;
        codesMax |= (OutCode(p) & 0x3f) << shift;
    }
    if (!codesMin && !codesMax) return -1;
    u32 mask = 0x20202020;
    for (int j = 5; j >= 0; j--) {
        if ((codesMin & mask) == mask && (codesMax & mask) == mask) return 0;
        mask >>= 1;
    }
    return 1;
}

EXDisplay::EXDisplay()
{
    m_ScreenSpaceOffsetX = 0.0f;
    m_ScreenSpaceOffsetY = 0.0f;
    m_VerticalBlanks = 0;
    m_pDevice = 0;
    m_pDebugFont = 0;
    m_TotalVerticalBlanks = 0;
    m_NormalProjection.set_identity();
    m_934.set_zero();
    m_GammaSet = 0;
    m_pNullTexture = 0;
    m_pNullTextureBlack = 0;
    m_pCurrentDeferredEntity = 0;
    m_VideoMemUsed = 0;
    m_FrameRate = 60;
}

// The debug font: the 1-bit EXDbgFont bitmap expanded to A1R5G5B5 and swizzled into a 128x256
// texture; characters 32 on are 12x12 cells, ten to a row.
s8 EXDisplay::InitDebugFont()
{
    xb::D3DTexture *pTex;
    if (D3dCreateTexture(128, 256, 1, 0, (xb::D3DFORMAT)2, 1, &pTex)) return 0;
    xb::D3DLOCKED_RECT lr;
    D3DTexture_LockRect(pTex, 0, &lr, 0, 0);
    u16 *pBuf = (u16 *)_EXAlloc(0x10000, 0);
    const unsigned char *pRow = EXDbgFont.GetData() + 0xff0;
    unsigned char mask = 0x80;
    u16 *pDst = pBuf;
    for (int y = 256; y; y--) {
        const unsigned char *pSrc = pRow;
        u16 *p = pDst;
        for (int x = 128; x; x--) {
            *p++ = (*pSrc & mask) ? 0xc210 : 0;
            mask >>= 1;
            if (!mask) {
                pSrc++;
                mask = 0x80;
            }
        }
        pRow -= 16;
        pDst += 128;
    }
    XGSwizzleRect(pBuf, 0, 0, lr.pBits, 128, 256, 0, 2);
    EXFree(pBuf);
    m_pDebugFont = pTex;
    int i;
    for (i = 0; i < 256; i++) {
        m_DebugFontU[i] = 0.0f;
        m_DebugFontV[i] = 0.0f;
    }
    float u = 0.0f, v = 0.0f;
    int col = 0;
    for (i = 0x20; i < 0x20 + 0x97; i++) {
        m_DebugFontU[i] = u;
        m_DebugFontV[i] = v;
        u += 0.0941176488f;
        if (++col >= 10) {
            v += 0.0469667315f;
            u = 0.0f;
            col = 0;
        }
    }
    return 1;
}

s8 EXDisplay::Close()
{
    ReleaseD3DResources();
    if (m_pDevice) {
        xb::D3DDevice::Release();
        m_pDevice = 0;
    }
    return 1;
}

void EXDisplay::UpdateTransforms()
{
    if (ms_UpdObjectToWorld || ms_UpdWorldToView || ms_UpdViewToCull)
        ms_mtxArray[10] = (ms_mtxArray[1] * ms_mtxArray[3]) * ms_mtxArray[4];
    if (ms_UpdViewToScreen || ms_UpdWorldToView) ms_mtxArray[8] = ms_mtxArray[12] * ms_mtxArray[3];
    if (ms_UpdViewToScreen || ms_UpdObjectToWorld) ms_mtxArray[11] = ms_mtxArray[8] * ms_mtxArray[4];
    if (ms_UpdViewToScreen || ms_UpdWorldToView) m_ViewProjection = ms_mtxArray[2] * ms_mtxArray[3];
    ms_UpdViewToCull = 0;
    ms_UpdWorldToScreen = 0;
    ms_UpdWorldToView = 0;
    ms_UpdViewToScreen = 0;
    ms_UpdObjectToWorld = 0;
}

// Presentation interval: immediate without vsync, else one vblank per 60 (or 50) Hz frame the
// app's maximum frame rate allows. Then the optional clear and the screen-space offset.
s8 EXDisplay::BeginDraw()
{
    if (!m_Params.m_VSync && m_PresentParams.FullScreen_PresentationInterval != 0x80000000) {
        m_PresentParams.FullScreen_PresentationInterval = 0x80000000;
        xb::D3DDevice::Reset(&m_PresentParams);
    }
    unsigned int rate = EXBaseApp::MaxFrameRate();
    if ((m_Params.m_VSync == 1 && m_PresentParams.FullScreen_PresentationInterval == 0x80000000) ||
        (m_Params.m_VSync == 1 && rate != m_FrameRate)) {
        unsigned int interval = 1;
        if (m_PresentParams.FullScreen_RefreshRateInHz == 60) {
            if (rate == 30)
                interval = 2;
            else if (rate == 15)
                interval = 4;
        }
        if (m_PresentParams.FullScreen_RefreshRateInHz == 50) {
            if (rate == 25)
                interval = 2;
            else if (rate <= 15)
                interval = 4;
        }
        m_FrameRate = rate;
        m_PresentParams.FullScreen_PresentationInterval = interval;
        xb::D3DDevice::Reset(&m_PresentParams);
    }
    xb::D3DDevice::BeginScene();    // no code on the Xbox
    if (ms_Flags & 1) {
        xb::D3DDevice::Clear(0, 0, 0xf1, ms_BkColor.argb(), 1.0f, 0);
        return 1;
    }
    float x = 0.0f, y = 0.0f;
    xb::D3DDevice::GetScreenSpaceOffset(&x, &y);
    if (x != m_ScreenSpaceOffsetX || y != m_ScreenSpaceOffsetY)
        xb::D3DDevice::SetScreenSpaceOffset(m_ScreenSpaceOffsetX, m_ScreenSpaceOffsetY);
    return 1;
}

void XBRenderTargetManager::RestoreDefault()
{
    if (m_pSurface) D3DResource_Release(m_pSurface);
    EXWnd *pWnd = EXBaseDisplay::ActiveWnd();
    EXBaseWnd *w = (EXBaseWnd *)pWnd;
    EXRect rect(w->m_PrimRect.x, w->m_PrimRect.y, (int)(w->m_Origin.x * 2.0f), (int)(w->m_Origin.y * 2.0f));
    EXRect clip(((EXBaseWnd *)pWnd)->m_PrimClipRect);
    Display()->SetRenderTarget(m_pDefaultColour, m_pDefaultDepth);
    Display()->SetViewPort(&rect, &clip);
    m_pTexture = 0;
    m_pSurface = 0;
}

int EXDisplay::CullSphere(const EXBoundsSphere &s, const EXMaths::mat_44 &m)
{
    EXMaths::vec_xyzw c;
    c.set(((const EXMaths::vec_xyzw &)s).get_w1());
    EXMaths::vec_xyzw p = m * c;
    float r = s.Radius();
    float w = (float)fabs(p.w);
    if (r > w) r *= 1.2f;
    const EXMaths::vec_xyzw &k = ViewToCullScale();
    EXMaths::vec_xyzw t;
    t.x = k.x * r; t.y = k.y * r; t.z = k.z * r; t.w = k.w;
    EXMaths::vec_xyzw e = t;
    if (0.0f > p.z + e.z) return 0;
    if (0.0f - w > p.x + e.x) return 0;
    if (p.x - e.x > w) return 0;
    if (0.0f - w > p.y + e.y) return 0;
    if (p.y - e.y > w) return 0;
    if (0.0f - w > p.x) return 1;
    if (p.x > w) return 1;
    if (0.0f - w > p.y) return 1;
    if (p.y > w) return 1;
    return -1;
}

// A white 2D line in screen pixels, over everything.
void Line2D(ex::RGBA col, int x0, int y0, int x1, int y1)
{
    XBRenderState.SetRenderState(xb::D3DRS_ZENABLE, 0);
    s_Line[0].x = (float)x0;
    s_Line[0].y = (float)y0;
    s_Line[0].z = XBVertexShader.m_ScreenZ;
    s_Line[1].x = (float)x1;
    s_Line[1].y = (float)y1;
    s_Line[1].z = XBVertexShader.m_ScreenZ;
    s_Line[0].diffuse = s_Line[1].diffuse = col.argb();
    EXRect r(0, 0, 640, 480);
    Display()->SetViewPort(&r, 0);
    XBRenderState.SetTexture(0, 0);
    XBRenderState.SetTexture(1, 0);
    XBVertexShader.SetVertexShader(5);
    XBRenderState.SetTextureStageState(1, xb::D3DTSS_COLOROP, 1);
    XBRenderState.SetRenderState(xb::D3DRS_ALPHABLENDENABLE, 1);
    XBRenderState.SetRenderState(xb::D3DRS_ALPHAREF, 1);
    XBRenderState.SetRenderState(xb::D3DRS_BLENDOP, 0x8006);
    XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 0x302);
    XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 0x303);
    XBRenderState.SetTextureStageState(1, xb::D3DTSS_COLOROP, 1);
    xb::D3DDevice::DrawPrimitiveUP(xb::D3DPT_LINELIST, 1, s_Line, sizeof(XBLineVert));
    XBRenderState.SetRenderState(xb::D3DRS_ZENABLE, 2);
}

s8 EXDisplay::Open(const EXPresentParams &params)
{
    SetUp(params);
    xb::Direct3D::SetPushBufferSize(0x200000, 0x80000);
    xb::HRESULT hr = ((EXApp *)EXBaseApp::GetApp())->Direct3D()->CreateDevice(0, xb::D3DDEVTYPE_HAL, 0, 0x40, &m_PresentParams,
                                                                               &m_pDevice);
    m_RenderTargets.Initialise(m_pDevice);
    if (m_VideoMemUsed == 0)
        m_VideoMemUsed = m_PresentParams.BackBufferHeight * m_PresentParams.BackBufferWidth * 12 + 0x200000;
    if (hr != 0) return 0;
    XBRenderState.SetDevice(m_pDevice);
    XBVertexShader.DeclareAllVertexShaders();
    XBPixelShader.DeclareAllPixelShaders();
    xb::D3DDevice::Clear(0, 0, 0xf3, 0, 1.0f, 0);
    xb::D3DDevice::Present(0, 0, 0, 0);
    xb::D3DDevice::Clear(0, 0, 0xf3, 0, 1.0f, 0);
    xb::D3DDevice::SetShaderConstantMode(0x11);
    EXRect r(0, 0, Width(), Height());
    SetViewPort(&r, 0);
    SetDefaultXBoxRenderState();
    InitDebugFont();
    InitNullTexture();
    InitNullTextureBlack();
    xb::D3DDevice::SetVerticalBlankCallback(XBVerticalBlankCallback);
    xb::D3DDevice::SetSoftDisplayFilter(1);
    return 1;
}

s8 EXDisplay::ReOpen(const EXPresentParams &params)
{
    SetUp(params);
    xb::HRESULT hr = xb::D3DDevice::Reset(&m_PresentParams);
    m_RenderTargets.Initialise(m_pDevice);
    if (hr != 0) return 0;
    xb::D3DDevice::Clear(0, 0, 0xf3, 0, 1.0f, 0);
    xb::D3DDevice::Present(0, 0, 0, 0);
    xb::D3DDevice::Clear(0, 0, 0xf3, 0, 1.0f, 0);
    EXRect r(0, 0, Width(), Height());
    SetViewPort(&r, 0);
    return 1;
}

// Frame end: the safe-frame outline, then the tint gamma ramp (or the identity ramp once the
// tint goes away).
s8 EXDisplay::EndDraw()
{
    if (m_SafeFrameEnabled) {
        EXPoint tl = DispToPrim(m_SafeFrame.TopLeft());
        EXPoint tr = DispToPrim(m_SafeFrame.TopRight());
        EXPoint br = DispToPrim(m_SafeFrame.BottomRight());
        EXPoint bl = DispToPrim(m_SafeFrame.BottomLeft());
        Line2D(0xffffffff, tl.x, tl.y, tr.x, tr.y);
        Line2D(0xffffffff, tr.x, tr.y, br.x, br.y);
        Line2D(0xffffffff, br.x, br.y, bl.x, bl.y);
        Line2D(0xffffffff, bl.x, bl.y, tl.x, tl.y);
    }
    xb::D3DDevice::EndScene();      // no code on the Xbox
    xb::_D3DGAMMARAMP ramp;
    if (ms_Flags & 2) {
        for (unsigned int i = 0; i < 256; i++) {
            float r = (float)i * ms_TintRed;
            float g = (float)i * ms_TintGreen;
            float b = (float)i * ms_TintBlue;
            if (r > 255.0f) r = 255.0f;
            if (g > 255.0f) g = 255.0f;
            if (b > 255.0f) b = 255.0f;
            ramp.red[i] = (unsigned char)r;
            ramp.green[i] = (unsigned char)g;
            ramp.blue[i] = (unsigned char)b;
        }
        xb::D3DDevice::SetGammaRamp(0, &ramp);
        m_GammaSet = 1;
    } else if (m_GammaSet) {
        for (unsigned int i = 0; i < 256; i++) {
            ramp.blue[i] = (unsigned char)i;
            ramp.green[i] = (unsigned char)i;
            ramp.red[i] = (unsigned char)i;
        }
        xb::D3DDevice::SetGammaRamp(0, &ramp);
        m_GammaSet = 0;
    }
    return 1;
}

EXDisplay::~EXDisplay() { Close(); }
