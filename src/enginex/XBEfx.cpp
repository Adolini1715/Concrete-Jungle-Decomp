#include "XBBaseEfx.h"
#include "xbd3d8.h"
#include "XBBaseRenderState.h"
#include "EXDisplay.h"
#include "EXBaseWnd.h"
#include "RGBA.h"
#include "EXRand.h"
#include <string.h>

// Full-screen post effects: focus blur (depth of field), motion blur, bloom,
// noise, and the copies of the back buffer and z-buffer they work from.

long D3dCreateTexture(unsigned int w, unsigned int h, unsigned int levels, unsigned long usage, xb::D3DFORMAT fmt,
                      unsigned long pool, xb::D3DTexture **ppTex);
// Draw pTex (a texture or surface) into the current target: destination rectangle, a mode (blend and filter
// flags), then the source rectangle as w, h, x, y in texels. TODO a7.
extern "C" void __stdcall XGSetTextureHeader(unsigned int w, unsigned int h, unsigned int levels, unsigned long usage,
                                             xb::D3DFORMAT fmt, unsigned long pool, xb::D3DTexture *pTex,
                                             unsigned int data, unsigned int pitch);
void XBRenderTexture(void *pTex, float x, float y, float w, float h, int mode, float a7, float srcW, float srcH,
                     float srcX, float srcY, ex::RGBA col);

// Kernel offsets for the bloom blur, in texels (scaled by fmpx); x >= 1000 ends the list.
struct XbBloomOffset {
    float x, y;
};
float fmpx = 1.0f;
XbBloomOffset g_BloomOffsets[10] = {
    {0.0f, 0.0f}, {-1.5f, -1.5f}, {0.0f, -1.5f}, {1.5f, -1.5f}, {-1.5f, 0.0f},
    {1.5f, 0.0f}, {-1.5f, 1.5f}, {0.0f, 1.5f}, {1.5f, 1.5f}, {1000.0f, 1000.0f},
};
XBBaseEfx XBEfx;

static inline EXDisplay *Display() { return (EXDisplay *)EXBaseDisplay::GetDisplay(); }

XBBaseEfx::XBBaseEfx()
{
    m_6c = 16.0f;
    m_pBackBuffer = 0;
    m_pBlitSrc = 0;
    m_pRefractionBlur = 0;
    m_pBlit128 = 0;
    m_pNoise = 0;
    m_pFocusBlur256 = 0;
    m_pFocusBlur128 = 0;
    m_pFocusBlur64 = 0;
    m_34 = 0;
    m_64 = 0.0f;
    m_68 = 0.0f;
    m_pMotionBlur = 0;
    m_pBloomHalf = 0;
    m_pExpansion = 0;
    memset(m_pBloom, 0, sizeof(m_pBloom));
    m_pBloomQuarter = 0;
}

xb::D3DTexture *XBBaseEfx::GetBackBufferAsTexture()
{
    xb::D3DSurface *pBack;
    xb::D3DDevice::GetBackBuffer(0, 0, &pBack);
    m_pBackBuffer = pBack;
    D3DResource_Release(pBack);
    return (xb::D3DTexture *)pBack;
}

void XBBaseEfx::XBBlitWindowToTexture(xb::D3DTexture *pTex, int x, int y, int w, int h)
{
    xb::D3DSurface *pBack;
    xb::D3DDevice::GetBackBuffer(0, 0, &pBack);
    m_pBackBuffer = pBack;
    D3DResource_Release(pBack);
    if (pTex) {
        xb::D3DSURFACE_DESC desc;
        pTex->GetLevelDesc(0, &desc);
        xb::D3DSurface *pDepth = D3DDevice_GetDepthStencilSurface2();
        xb::D3DSurface *pColour = D3DDevice_GetRenderTarget2();
        xb::D3DSurface *pSurf = D3DTexture_GetSurfaceLevel2(pTex, 0);
        if (pSurf) {
            Display()->SetRenderTarget(pSurf, 0);
            XBRenderTexture(pBack, 0, 0, (float)desc.Width, (float)desc.Height, 2, 1.0f, (float)w, (float)h,
                            (float)x, (float)y, ex::RGBA(0xff, 0x80, 0x80, 0x80));
            D3DResource_Release(pSurf);
        }
        Display()->SetRenderTarget(pColour, pDepth);
        D3DResource_Release(pDepth);
        D3DResource_Release(pColour);
    }
}

void XBBaseEfx::XBCreateEFXTexture(int size, xb::D3DTexture **ppTex, int linear)
{
    if (*ppTex == 0) {
        if (!linear)
            D3dCreateTexture(size, size, 1, 1, xb::D3DFMT_A8R8G8B8, 0, ppTex);
        else
            D3dCreateTexture(size, size, 1, 1, xb::D3DFMT_LIN_A8R8G8B8, 0, ppTex);
        xb::D3DTexture *pTex = *ppTex;
        if (pTex) {
            xb::D3DLOCKED_RECT lr;
            D3DTexture_LockRect(pTex, 0, &lr, 0, 0xa0);
            u32 *p = (u32 *)lr.pBits;
            for (int y = 0; y < size; y++)
                for (int x = 0; x < size; x++)
                    *p++ = 0xff000000;
        }
    }
}

void XBBaseEfx::XBCreateEFXTexture(int w, int h, xb::D3DTexture **ppTex, int linear)
{
    if (*ppTex == 0) {
        if (!linear)
            D3dCreateTexture(w, h, 1, 1, xb::D3DFMT_A8R8G8B8, 0, ppTex);
        else
            D3dCreateTexture(w, h, 1, 1, xb::D3DFMT_LIN_A8R8G8B8, 0, ppTex);
        xb::D3DTexture *pTex = *ppTex;
        if (pTex) {
            xb::D3DLOCKED_RECT lr;
            D3DTexture_LockRect(pTex, 0, &lr, 0, 0xa0);
            u32 *p = (u32 *)lr.pBits;
            for (int y = 0; y < h; y++)
                for (int x = 0; x < w; x++)
                    *p++ = 0xff000000;
        }
    }
}

void XBBaseEfx::XBCreateBackBufferTexture(xb::D3DTexture **ppTex)
{
    if (*ppTex == 0)
        D3dCreateTexture(640, 480, 1, 0, xb::D3DFMT_LIN_A8R8G8B8, 0, ppTex);
}

// Smooth pDst in place `passes` times: draw it at half size into pSrc
// (filtered), then back at full size.
void XBBaseEfx::BiLinearSmoothTexture(xb::D3DTexture *pDst, xb::D3DTexture *pSrc, float size, int passes, int mode)
{
    for (int n = passes; n > 0; n--) {
        xb::D3DSurface *pColour = D3DDevice_GetRenderTarget2();
        xb::D3DSurface *pDepth = D3DDevice_GetDepthStencilSurface2();
        if (pDst) {
            xb::D3DSurface *pSurf = D3DTexture_GetSurfaceLevel2(pDst, 0);
            if (pSurf && pDepth)
                Display()->SetRenderTarget(pSurf, 0);
            XBRenderState.SetRenderState(xb::D3DRS_ZENABLE, 0);
                        XBRenderTexture(pSrc, 0, 0, size * 0.5f, size * 0.5f, mode, 0, size, size, 0, 0, ex::RGBA(0xff, 0x80, 0x80, 0x80));
            XBRenderState.SetRenderState(xb::D3DRS_ZENABLE, 2);
            Display()->SetRenderTarget(pColour, pDepth);
            D3DResource_Release(pSurf);
        }
        D3DResource_Release(pColour);
        D3DResource_Release(pDepth);
        pColour = D3DDevice_GetRenderTarget2();
        pDepth = D3DDevice_GetDepthStencilSurface2();
        if (pSrc) {
            xb::D3DSurface *pSurf = D3DTexture_GetSurfaceLevel2(pSrc, 0);
            if (pSurf && pDepth)
                Display()->SetRenderTarget(pSurf, 0);
            XBRenderState.SetRenderState(xb::D3DRS_ZENABLE, 0);
                        XBRenderTexture(pDst, 0, 0, size, size, mode, 0, size * 0.5f, size * 0.5f, 0, 0, ex::RGBA(0xff, 0x80, 0x80, 0x80));
            XBRenderState.SetRenderState(xb::D3DRS_ZENABLE, 2);
            Display()->SetRenderTarget(pColour, pDepth);
            D3DResource_Release(pSurf);
        }
        D3DResource_Release(pColour);
        D3DResource_Release(pDepth);
    }
}

static inline void CreateBlack(int size, xb::D3DTexture **ppTex)
{
    if (*ppTex == 0) {
        D3dCreateTexture(size, size, 1, 1, xb::D3DFMT_A8R8G8B8, 0, ppTex);
        xb::D3DTexture *pTex = *ppTex;
        if (pTex) {
            xb::D3DLOCKED_RECT lr;
            D3DTexture_LockRect(pTex, 0, &lr, 0, 0xa0);
            u32 *p = (u32 *)lr.pBits;
            for (int i = 0; i < size * size; i++)
                p[i] = 0xff000000;
        }
    }
}

void XBBaseEfx::XBCreateFocusBlurTextures()
{
    CreateBlack(256, &m_pFocusBlur256);
    CreateBlack(128, &m_pFocusBlur128);
    CreateBlack(64, &m_pFocusBlur64);
}

void XBBaseEfx::XBBlurFocusBlurTextures()
{
    XBBlitBackBufferToTexture(XBEfx.m_pFocusBlur256, 0, 0, 256, 256, 0, 0);
    BiLinearSmoothTexture(m_pFocusBlur128, m_pFocusBlur256, 256.0f,
                          ((EXBaseWnd *)EXBaseDisplay::ActiveWnd())->GetFocusBlurFactor(), 0);
}

// View-space depth of a world point, as the focus blur's depth mapping wants it.
void XBBaseEfx::CalculateDepth(float *pOut, const xb::D3DXVECTOR3 &p)
{
    xb::D3DXVECTOR4 v(p.x, p.y, p.z, 1.0f);
    D3DXVec4Transform(&v, &v, &EXBaseDisplay::WorldToView());
    D3DXVec4Transform(&v, &v, Display()->GetNormalProjection());
    *pOut = v.w * 0.001f;
}

float XBBaseEfx::FQuantizedDepth(float d, float *pHi, float *pLo)
{
    u32 q = (u32)(d * 65536.0f);
    *pHi = (float)(q >> 8) * (1.0f / 255.0f);
    *pLo = (float)(q & 0xff) * (1.0f / 255.0f);
    return (float)q * (1.0f / 65536.0f);
}

void XBBaseEfx::CalculateDepthMapping(float d, float range, float *pHi, float *pHi2, float *pLo, float *pLo2)
{
    if (0.0f > d) d = 0.0f;
    else if (d > 1.0f) d = 1.0f;
    float lo;
    FQuantizedDepth(d, &d, &lo);
    *pHi = d;
    *pLo = lo;
}

// A texture header over the depth buffer's own memory, read as linear
// A8B8G8R8: the focus blur samples depth through it (stencil in red, depth
// bits 0-7, 8-15, 16-23 in green, blue, alpha).
void XBBaseEfx::XBCreateBackBufferZTexture()
{
    if (m_pZTexture == 0) {
        xb::D3DSurface *pZ = D3DDevice_GetDepthStencilSurface2();
        XGSetTextureHeader(640, 480, 1, 0, (xb::D3DFORMAT)0x3f, 0, &m_ZTexture, pZ->m_Data, 640 * 4);
        m_pZTexture = &m_ZTexture;
    }
}

void XBBaseEfx::XBBlitZBufferToTexture()
{
    if (m_pZTexture == 0) {
        xb::D3DSurface *pZ = D3DDevice_GetDepthStencilSurface2();
        XGSetTextureHeader(640, 480, 1, 0, (xb::D3DFORMAT)0x3f, 0, &m_ZTexture, pZ->m_Data, 640 * 4);
        m_pZTexture = &m_ZTexture;
    }
}

void XBBaseEfx::StartRenderToTexture(xb::D3DTexture *pColour, xb::D3DTexture *pDepth)
{
    m_pSavedDepth = D3DDevice_GetDepthStencilSurface2();
    m_pSavedColour = D3DDevice_GetRenderTarget2();
    if (pColour) {
        xb::D3DSurface *pSurf = D3DTexture_GetSurfaceLevel2(pColour, 0);
        m_pRTSurface = pSurf;
        if (pSurf) {
            Display()->SetRenderTarget(pSurf, 0);
            D3DDevice_Clear(0, 0, 0xf0, 0xff000000, 0, 0);
        }
    }
}

void XBBaseEfx::StopRenderToTexture()
{
    if (m_pSavedDepth)
        Display()->SetRenderTarget(m_pSavedColour, m_pSavedDepth);
    else
        Display()->SetRenderTarget(m_pSavedColour, 0);
    D3DResource_Release(m_pSavedColour);
    if (m_pSavedDepth)
        D3DResource_Release(m_pSavedDepth);
    D3DResource_Release(m_pRTSurface);
}

void XBBaseEfx::XBCreateMotionBlurTexture()
{
    if (m_pMotionBlur == 0)
        CreateBlack(256, &m_pMotionBlur);
}

// One bloom pass: pSrc drawn into pDst once per kernel offset, the first
// pass replacing and the rest adding. The shifts give each texture's size as
// the screen size over a power of two.
void XBBaseEfx::DoBloom(xb::D3DTexture *pSrc, xb::D3DTexture *pDst, u32 srcShift, u32 dstShift)
{
    EXBaseDisplay *pDisp = EXBaseDisplay::GetDisplay();
    u16 w = pDisp->m_Params.m_Width;
    u16 h = pDisp->m_Params.m_Height;
    u32 sw = (u32)w >> srcShift;
    u32 sh = (u32)h >> srcShift;
    float dw = (float)(u16)(w >> dstShift);
    float dh = (float)(u16)(h >> dstShift);
    xb::D3DSurface *pSurf = D3DTexture_GetSurfaceLevel2(pDst, 0);
    Display()->SetRenderTarget(pSurf, 0);
    int mode = 0x802;
    ex::RGBA col(0xff, 0x80, 0x80, 0x80);
    float fsh = (float)sh;
    float fsw = (float)sw;
    XbBloomOffset *o = g_BloomOffsets;
    do {
        XBRenderTexture(pSrc, fmpx * o->x, o->y * fmpx, dw, dh, mode, 0.0745f, fsw, fsh, 0, 0, col);
        o++;
        mode |= 0x10;
    } while (o->x < 1000.0f);
    D3DResource_Release(pSurf);
}

void XBBaseEfx::XBBlitBackBufferToTexture(xb::D3DTexture *pTex, int x, int y, int w, int h, u32 flags, int mode)
{
    xb::D3DSurface *pBack;
    xb::D3DDevice::GetBackBuffer(0, 0, &pBack);
    m_pBackBuffer = pBack;
    D3DResource_Release(pBack);
    if (pTex) {
        xb::D3DSurface *pDepth = D3DDevice_GetDepthStencilSurface2();
        xb::D3DSurface *pColour = D3DDevice_GetRenderTarget2();
        xb::D3DSurface *pSurf = D3DTexture_GetSurfaceLevel2(pTex, 0);
        if (pSurf) {
            Display()->SetRenderTarget(pSurf, 0);
            XBRenderTexture(pBack, (float)x, (float)y, (float)w, (float)h, 2, 0, 640.0f, 480.0f, 0, 0,
                            ex::RGBA(0xff, 0x80, 0x80, 0x80));
            D3DResource_Release(pSurf);
        }
        Display()->SetRenderTarget(pColour, pDepth);
        D3DResource_Release(pDepth);
        D3DResource_Release(pColour);
    }
    if (flags & 1) {
        CreateBlack(128, &m_pBlit128);
        BiLinearSmoothTexture(m_pBlit128, m_pBlitSrc, 256.0f, mode, 0);
    }
}

// 256x256 of random bytes (0-254) for the film-noise effect.
void XBBaseEfx::XBCreateNoiseTexture()
{
    if (m_pNoise == 0) {
        D3dCreateTexture(256, 256, 1, 0, (xb::D3DFORMAT)0, 0, &m_pNoise);
        xb::D3DTexture *pTex = m_pNoise;
        if (pTex) {
            xb::D3DLOCKED_RECT lr;
            D3DTexture_LockRect(pTex, 0, &lr, 0, 0xa0);
            u8 *p = (u8 *)lr.pBits;
            for (int y = 0; y < 256; y++)
                for (int x = 0; x < 256; x++)
                    *p++ = (u8)(g_EXRandClass.Rand32() % 255);
        }
    }
}

void XBBaseEfx::CalculateFocusDepths(xb::D3DXVECTOR3 &a, xb::D3DXVECTOR3 &b)
{
    CalculateDepth(&m_64, a);
}

static __forceinline void CreateLinearBlack(int w, int h, xb::D3DTexture **ppTex)
{
    if (*ppTex == 0) {
        D3dCreateTexture(w, h, 1, 1, xb::D3DFMT_LIN_A8R8G8B8, 0, ppTex);
        xb::D3DTexture *pTex = *ppTex;
        if (pTex) {
            xb::D3DLOCKED_RECT lr;
            D3DTexture_LockRect(pTex, 0, &lr, 0, 0xa0);
            u32 *p = (u32 *)lr.pBits;
            for (int y = 0; y < h; y++)
                for (int x = 0; x < w; x++)
                    *p++ = 0xff000000;
        }
    }
}

// Glow: the window's part of the back buffer down to quarter size, then
// `passes` blur passes, each half the size of the one before, all added back
// up at half size and drawn over the back buffer in the window's bloom
// colour.
void XBBaseEfx::Bloom(u32 passes)
{
    XBRenderState.SetRenderState((xb::D3DRENDERSTATETYPE)0x3c, 0);
    XBRenderState.SetRenderState((xb::D3DRENDERSTATETYPE)0x3d, 0);
    EXBaseDisplay *pDisp = EXBaseDisplay::GetDisplay();
    u32 w = pDisp->m_Params.m_Width >> 2;
    u32 h = pDisp->m_Params.m_Height >> 2;
    u32 i;
    for (i = 0; i < passes; i++) {
        CreateLinearBlack(w, h, &m_pBloom[i]);
        w >>= 1;
        h >>= 1;
    }
    pDisp = EXBaseDisplay::GetDisplay();
    w = pDisp->m_Params.m_Width >> 1;
    h = pDisp->m_Params.m_Height >> 1;
    CreateLinearBlack(w, h, &m_pBloomHalf);
    h >>= 1;
    w >>= 1;
    CreateLinearBlack(w, h, &m_pBloomQuarter);

    xb::D3DSurface *pDepth = D3DDevice_GetDepthStencilSurface2();
    xb::D3DSurface *pColour = D3DDevice_GetRenderTarget2();
    xb::D3DSurface *pBack = D3DDevice_GetBackBuffer2(0);
    m_pBackBuffer = pBack;
    xb::D3DSurface *pSurf = D3DTexture_GetSurfaceLevel2(m_pBloomQuarter, 0);
    Display()->SetRenderTarget(pSurf, 0);
    EXBaseWnd *pWnd = (EXBaseWnd *)EXBaseDisplay::ActiveWnd();
    const EXRect &r = pWnd->m_PrimClipRect;
    float sx = (float)w * (1.0f / 640.0f);
    float sy = (float)h * (1.0f / 480.0f);
    float rx = (float)r.x, ry = (float)r.y, rw = (float)r.w, rh = (float)r.h;
    int x0 = (int)(rx * sx);
    int y0 = (int)(ry * sy);
    int x1 = (int)(rw * sx);
    int y1 = (int)(rh * sy);
    XBRenderTexture(pBack, (float)x0, (float)y0, (float)x1, (float)y1, 0x402, 1.0f, rw, rh, rx, ry,
                    ex::RGBA(0xff, 0x80, 0x80, 0x80));
    D3DResource_Release(pSurf);
    D3DResource_Release(m_pBackBuffer);

    DoBloom(m_pBloomQuarter, m_pBloom[0], 2, 2);
    for (i = 1; i < passes; i++)
        DoBloom(m_pBloom[i - 1], m_pBloom[i], i + 1, i + 2);

    pSurf = D3DTexture_GetSurfaceLevel2(m_pBloomHalf, 0);
    Display()->SetRenderTarget(pSurf, 0);
    pDisp = EXBaseDisplay::GetDisplay();
    u16 dw = pDisp->m_Params.m_Width, dh = pDisp->m_Params.m_Height;
    float hw = (float)(u16)(dw >> 1);
    float hh = (float)(u16)(dh >> 1);
    u32 bw = dw >> 2, bh = dh >> 2;
    int mode = 0x802;
    for (i = 0; i < passes; i++) {
        XBRenderTexture(m_pBloom[i], 0, 0, hw, hh, mode, 0.5f, (float)bw, (float)bh, 0, 0,
                        ex::RGBA(0xff, 0x80, 0x80, 0x80));
        bw >>= 1;
        bh >>= 1;
        mode = 0x812;
    }
    D3DResource_Release(pSurf);
    Display()->SetRenderTarget(pColour, pDepth);
    ex::RGBA col = pWnd->BloomColor();
    col.m_A = 0xff;
    XBRenderTexture(m_pBloomHalf, rx * 2.0f, (float)(y0 * 2), (float)(x1 * 2), (float)(y1 * 2),
                    (mode & ~0x800) | 0x1000, 0.5f, hw, hh, 0, 0, col);
    D3DResource_Release(pDepth);
    D3DResource_Release(pColour);
}
