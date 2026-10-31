#include "EXTexture.h"
#include "EXDisplay.h"
#include "EXBaseWnd.h"
#include "EXWnd.h"
#include "EXRand.h"
#include "EXRect.h"
#include "EXMemory.h"
#include "XBBaseEfx.h"
#include "XBBaseRenderState.h"
#include "XBBaseVertexShader.h"
#include "XBBasePixelShader.h"
#include "xbd3d8.h"
#include "RGBA.h"
#include "EXBaseApp.h"
#include "EXCachedGeoEntity.h"

// Xbox textures: the format tables, runtime texture creation, the per-texture sampler setup, and
// XBRenderTexture, the full-window textured quad every window post effect is drawn with.

static inline EXDisplay *Display() { return (EXDisplay *)EXBaseDisplay::GetDisplay(); }
static inline EXBaseWnd *ActiveWnd() { return (EXBaseWnd *)EXBaseDisplay::ActiveWnd(); }

extern "C" xb::D3DXVECTOR3 *__stdcall D3DXVec3Normalize(xb::D3DXVECTOR3 *pOut, const xb::D3DXVECTOR3 *pV);

xb::D3DFORMAT EXTexFmt[] = {
    xb::D3DFMT_R5G6B5, xb::D3DFMT_X1R5G5B5, xb::D3DFMT_DXT1, xb::D3DFMT_DXT1, xb::D3DFMT_DXT2,
    xb::D3DFMT_A4R4G4B4, xb::D3DFMT_A8R8G8B8, xb::D3DFMT_P8, xb::D3DFMT_A1R5G5B5, xb::D3DFMT_LIN_A8R8G8B8,
    (xb::D3DFORMAT)-1,
};
float EXTexFmtSizeBytes[] = {2.0f, 2.0f, 0.5f, 0.5f, 1.0f, 2.0f, 4.0f, 1.0f, 2.0f, 4.0f};
// The missing commas are in the original: the last entry runs three names together.
char *EXTexFmtTxt[] = {
    "R5G6B5", "X1R5G5B5", "DXT1", "DXT1", "DXT2", "A4R4G4B4", "A8R8G8B8",
    "PAL8" "D3DFMT_A1R5G5B5" "D3DFMT_LIN_A8R8G8B8",
};
// Fog scale of the depth fog pass (mode 0x80), clamped to -1..2 when used.
float fogmult = 1.0f;

// One vertex of the screen quad: position, colour and three texture coordinate sets (the last
// always spans the whole texture).
struct XBRenderTextureVertex {
    float x, y, z;
    u32 diffuse;
    float u0, v0;
    float u1, v1;
    float u2, v2;
};

// Draws pTex over the window rect (x, y, w, h) in 640x480 screen units. mode:
//   1      texture coordinates in texels (w x h); YUV texture
//   2      texture coordinates span srcW x srcH
//   4      alpha from intensity
//   8      random texture offset (noise)
//   0x10   additive, alpha and colour from intensity (glow)
//   0x20   focus blur composite: back buffer, depth and blurred copy through pixel shader 1
//   0x40   alpha from intensity over white (motion blur)
//   0x80   depth fog through pixel shader 2
//   0x100  inverse colour (pixel shader 3)
//   0x200  inverse colour variant (pixel shader 4)
//   0x400  pixel shader 5
//   0x800  additive at the screen origin
//   0x1000 diffuse is col
//   0x2000 multiply with the frame (DESTCOLOR, ZERO)
// u, v are the texture offset; the noise and motion blur textures take their sampling from the
// active window.
void XBRenderTexture(void *pTex, float x, float y, float w, float h, int mode, float intensity, float srcW,
                     float srcH, float u, float v, ex::RGBA col)
{
    if (!pTex) return;
    xb::D3DXVECTOR4 zero(0.0f, 0.0f, 0.0f, 0.0f);
    D3DDevice_SetVertexShaderConstantFast(29, &zero, 1);

    u32 diffuse = 0xffffffff;
    if (mode & 4) diffuse = (u8)(int)(intensity * 255.0f) << 24;
    if (mode & 0x40) diffuse = ((u8)(int)(intensity * 255.0f) << 24) | 0xffffff;
    if ((mode & 0x10) || (mode & 0x800)) {
        u8 a = (u8)(int)(intensity * 255.0f);
        diffuse = ex::RGBA(a, a, a, 0xff).argb();
    }
    if (mode & 8) {
        u = g_EXRandClass.Randf();
        v = g_EXRandClass.Randf();
    }
    float tw = 1.0f, th = 1.0f;
    if (mode & 1) {
        tw = w;
        th = h;
    }
    if (mode & 2) {
        tw = srcW;
        th = srcH;
    }
    if (pTex == XBEfx.m_pNoise) {
        tw = ActiveWnd()->GetNoiseTextureSampleWidth();
        th = ActiveWnd()->GetNoiseTextureSampleHeight();
    }
    if (pTex == XBEfx.m_pMotionBlur) {
        const EXRectf &s = ActiveWnd()->GetMotionBlurSampleRect();
        const EXRectf &d = ActiveWnd()->GetMotionBlurDrawRect();
        u = s.x;
        v = s.y;
        tw = s.w;
        th = s.h;
        x = d.x * 640.0f;
        y = d.y * 480.0f;
        w = d.w * 640.0f;
        h = d.h * 480.0f;
    }
    if (mode & 0x1000) diffuse = col.argb();

    float z = XBVertexShader.m_ScreenZ + 100.0f;
    XBRenderTextureVertex quad[4];
    quad[0].x = x;
    quad[0].y = y;
    quad[0].z = z;
    quad[0].u0 = u;
    quad[0].v0 = v;
    quad[0].u1 = u;
    quad[0].v1 = v;
    quad[0].u2 = u;
    quad[0].v2 = v;
    quad[0].diffuse = diffuse;
    quad[1].x = x;
    quad[1].y = y + h;
    quad[1].z = z;
    quad[1].u0 = u;
    quad[1].v0 = v + th;
    quad[1].u1 = u;
    quad[1].v1 = v + th;
    quad[1].u2 = u;
    quad[1].v2 = v + 1.0f;
    quad[1].diffuse = diffuse;
    quad[2].x = x + w;
    quad[2].y = y + h;
    quad[2].z = z;
    quad[2].u0 = u + tw;
    quad[2].v0 = v + th;
    quad[2].u1 = u + tw;
    quad[2].v1 = v + th;
    quad[2].u2 = u + 1.0f;
    quad[2].v2 = v + 1.0f;
    quad[2].diffuse = diffuse;
    quad[3].x = x + w;
    quad[3].y = y;
    quad[3].z = z;
    quad[3].u0 = u + tw;
    quad[3].v0 = v;
    quad[3].u1 = u + tw;
    quad[3].v1 = v;
    quad[3].u2 = u + 1.0f;
    quad[3].v2 = v;
    quad[3].diffuse = diffuse;

    // Only the motion blur's window rect can leave the screen.
    if (pTex == XBEfx.m_pMotionBlur) {
        if (0.0f > x) x = 0.0f;
        if (0.0f > y) y = 0.0f;
        if (x > 639.0f) x = 639.0f;
        if (y > 479.0f) y = 479.0f;
        if (x + w > 640.0f) w = 640.0f - x;
        if (y + h > 480.0f) h = 480.0f - y;
    }
    if (mode & 0x800) {
        x = 0.0f;
        y = 0.0f;
    }
    EXRect rect((int)x, (int)y, (int)w, (int)h);
    Display()->SetViewPort(&rect, 0);

    XBRenderState.SetRenderState(xb::D3DRS_ZFUNC, 0x207);
    if (mode & 0x10) {
        XBRenderState.SetRenderState(xb::D3DRS_BLENDOP, 0x8006);
        XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 0x302);
        XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 1);
        if (mode & 0x800) {
            XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 1);
            XBRenderState.SetTextureStageState(0, xb::D3DTSS_ALPHAOP, 1);
        }
    } else {
        XBRenderState.SetRenderState(xb::D3DRS_BLENDOP, 0x8006);
        XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 0x302);
        XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 0x303);
    }
    if (mode & 0x2000) {
        XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 0x306);
        XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 0);
    }
    if (!(mode & 1) && !(mode & 2)) {
        XBRenderState.SetTextureStageState(0, xb::D3DTSS_ADDRESSU, 1);
        XBRenderState.SetTextureStageState(0, xb::D3DTSS_ADDRESSV, 1);
    } else {
        XBRenderState.SetTextureStageState(0, xb::D3DTSS_ADDRESSU, 3);
        XBRenderState.SetTextureStageState(0, xb::D3DTSS_ADDRESSV, 3);
        XBRenderState.SetTextureStageState(1, xb::D3DTSS_ADDRESSU, 3);
        XBRenderState.SetTextureStageState(1, xb::D3DTSS_ADDRESSV, 3);
        XBRenderState.SetTextureStageState(2, xb::D3DTSS_ADDRESSU, 3);
        XBRenderState.SetTextureStageState(2, xb::D3DTSS_ADDRESSV, 3);
    }
    if (!(mode & 0x10) && !(mode & 0x800)) {
        XBRenderState.SetTextureStageState(1, xb::D3DTSS_COLOROP, 1);
        XBRenderState.SetTextureStageState(1, xb::D3DTSS_ALPHAOP, 1);
        XBRenderState.SetTextureStageState(0, xb::D3DTSS_COLORARG1, 2);
        XBRenderState.SetTextureStageState(0, xb::D3DTSS_COLOROP, 2);
        XBRenderState.SetTextureStageState(0, xb::D3DTSS_ALPHAARG1, 2);
        XBRenderState.SetTextureStageState(0, xb::D3DTSS_ALPHAOP, 1);
    }
    XBRenderState.SetRenderState(xb::D3DRS_ZWRITEENABLE, 0);
    XBRenderState.SetRenderState(xb::D3DRS_CULLMODE, 0);
    XBRenderState.SetTexture(0, (xb::D3DTexture *)pTex);
    XBRenderState.SetTexture(1, 0);
    XBRenderState.SetTexture(2, 0);
    XBVertexShader.SetVertexShader(6);
    XBRenderState.SetRenderState(xb::D3DRS_ALPHABLENDENABLE, 1);

    if (mode & 0x100) {
        XBPixelShader.SetPixelShader(3);
        XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 1);
        XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 0);
        XBRenderState.SetRenderState(xb::D3DRS_ALPHABLENDENABLE, 0);
    }
    if (mode & 0x200) {
        XBPixelShader.SetPixelShader(4);
        XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 1);
        XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 0);
        XBRenderState.SetRenderState(xb::D3DRS_ALPHABLENDENABLE, 0);
    }
    if (mode & 0x300) {
        XBRenderState.SetRenderState(xb::D3DRS_ALPHATESTENABLE, 0);
        float f = ActiveWnd()->InverseColorFactor();
        unsigned int m = ActiveWnd()->InverseColorMask();
        if (1.0f > f) f = 1.0f;
        f *= 0.5f;
        float c[8];
        c[0] = f;
        c[1] = f;
        c[2] = f;
        c[3] = f;
        c[4] = (float)(m & 0xff) * (1.0f / 255.0f);
        c[5] = (float)((m >> 8) & 0xff) * (1.0f / 255.0f);
        c[6] = (float)((m >> 16) & 0xff) * (1.0f / 255.0f);
        c[7] = (float)(m >> 24) * (1.0f / 255.0f);
        xb::D3DDevice::SetPixelShaderConstant(0, c, 2);
        XBVertexShader.SetVertexShader(11);
    }
    if (mode & 0x80) {
        if (fogmult > 2.0f)
            fogmult = 2.0f;
        else if (-1.0f > fogmult)
            fogmult = -1.0f;
        XBPixelShader.SetPixelShader(2);
        float d = fogmult;
        xb::D3DXVECTOR3 dir, focus;
        dir = XBEfx.m_At - XBEfx.m_Eye;
        D3DXVec3Normalize(&dir, &dir);
        focus = XBEfx.m_Eye + d * dir;
        xb::D3DXVECTOR3 depths;
        XBEfx.CalculateFocusDepths(focus, depths);
        float hi, hi2, lo, lo2;
        XBEfx.CalculateDepthMapping(XBEfx.m_64, XBEfx.m_68, &hi, &hi2, &lo, &lo2);
        xb::D3DXVECTOR4 c(0.0f, 0.0f, 0.0f, fogmult);
        xb::D3DDevice::SetPixelShaderConstant(0, &c, 1);
        XBRenderState.SetTexture(1, XBEfx.GetBackBufferAsTexture());
        XBRenderState.SetTexture(0, XBEfx.m_pZTexture);
        XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 0x302);
        XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 0x303);
        XBVertexShader.SetVertexShader(11);
    }
    if (mode & 0x20) {
        XBPixelShader.SetPixelShader(1);
        float d = ActiveWnd()->GetFocusBlurDistance();
        xb::D3DXVECTOR3 dir, focus;
        dir = XBEfx.m_At - XBEfx.m_Eye;
        D3DXVec3Normalize(&dir, &dir);
        focus = d * dir + XBEfx.m_Eye;
        xb::D3DXVECTOR3 depths;
        XBEfx.CalculateFocusDepths(focus, depths);
        float hi, hi2, lo, lo2;
        XBEfx.CalculateDepthMapping(XBEfx.m_64, XBEfx.m_68, &hi, &hi2, &lo, &lo2);
        xb::D3DXVECTOR4 c(0.0f, 0.0f, 0.0f, hi);
        xb::D3DDevice::SetPixelShaderConstant(0, &c, 1);
        XBRenderState.SetTexture(0, (xb::D3DTexture *)pTex);
        XBRenderState.SetTexture(1, XBEfx.m_pZTexture);
        XBRenderState.SetTexture(2, XBEfx.m_pFocusBlur256);
        XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 1);
        XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 0x303);
        XBVertexShader.SetVertexShader(11);
    }
    if (mode & 0x400) XBPixelShader.SetPixelShader(5);
    D3DDevice_SetVertexShaderConstantFast(11, XBVertexShader.m_GlobalDiffuse, 1);
    XBRenderState.SetRenderState(xb::D3DRS_ZENABLE, 0);
    XBRenderState.SetRenderState(xb::D3DRS_ALPHAREF, 1);
    if (mode & 1) XBRenderState.SetRenderState(xb::D3DRS_YUVENABLE, 1);
    D3DDevice_DrawVerticesUP(8, 4, quad, sizeof(XBRenderTextureVertex));
    if (pTex == XBEfx.m_pMotionBlur) XBEfx.XBBlitBackBufferToTexture(XBEfx.m_pMotionBlur, 0, 0, 0x100, 0x100, 0, 0);

    XBRenderState.SetRenderState(xb::D3DRS_CULLMODE, 0x901);
    XBRenderState.SetRenderState(xb::D3DRS_ZENABLE, 2);
    xb::D3DDevice::SetPixelShader(0);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ADDRESSU, 1);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ADDRESSV, 1);
    XBRenderState.SetTextureStageState(1, xb::D3DTSS_ADDRESSU, 4);
    XBRenderState.SetTextureStageState(1, xb::D3DTSS_ADDRESSV, 4);
    XBRenderState.SetTextureStageState(2, xb::D3DTSS_ADDRESSU, 4);
    XBRenderState.SetTextureStageState(2, xb::D3DTSS_ADDRESSV, 4);
    if (mode & 1) XBRenderState.SetRenderState(xb::D3DRS_YUVENABLE, 0);
    XBRenderState.SetRenderState(xb::D3DRS_ZWRITEENABLE, 1);
    XBRenderState.SetRenderState(xb::D3DRS_ZFUNC, 0x203);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_COLORARG1, 2);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_COLOROP, 5);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_COLORARG2, 0);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ALPHAARG1, 2);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ALPHAOP, 4);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ALPHAARG2, 0);
    XBRenderState.SetRenderState(xb::D3DRS_ALPHATESTENABLE, 1);
    XBRenderState.SetRenderState(xb::D3DRS_BLENDOP, 0x8006);
    XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 0x302);
    XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 0x303);
    XBRenderState.SetTexture(0, 0);
    XBRenderState.SetTexture(1, 0);
    XBRenderState.SetTexture(2, 0);
    xb::D3DDevice::SetPixelShader(0);
}

extern "C" void __stdcall XGSetTextureHeader(unsigned int w, unsigned int h, unsigned int levels, unsigned long usage,
                                             xb::D3DFORMAT fmt, unsigned long pool, xb::D3DTexture *pTex,
                                             unsigned int data, unsigned int pitch);

float GetD3dFormatBytesPerPixel(xb::D3DFORMAT fmt)
{
    for (int i = 0; EXTexFmt[i] != -1; i++)
        if (fmt == EXTexFmt[i]) return EXTexFmtSizeBytes[i];
    return 0.0f;
}

// A palettised texture keeps its palette in the last 0x440 bytes of its data: a D3DPalette header
// then the 256 colours. The header is filled in the first time the texture is used.
struct XBTexturePalette {
    u32 m_Common;
    u32 m_Data;
};
__forceinline void InitialiseTexturePalette(unsigned char *pData, EXGeoTexture *pTex)
{
    XBTexturePalette *p = (XBTexturePalette *)(pData + pTex->GetSize() - 0x440);
    if (!p->m_Common) {
        p->m_Common = 0x30001;
        p->m_Data = (u32)(p + 8) & 0x7fffffff;
    }
}

long D3dCreateTexture(unsigned int w, unsigned int h, unsigned int levels, unsigned long usage, xb::D3DFORMAT fmt,
                      unsigned long pool, xb::D3DTexture **ppTex)
{
    long hr = xb::D3DDevice::CreateTexture(w, h, levels, usage, fmt, pool, ppTex);
    if (hr == 0) Display()->m_VideoMemUsed += (u32)(GetD3dFormatBytesPerPixel(fmt) * (float)w * (float)h);
    return hr;
}

EXTexture::EXTexture()
{
    m_pD3DTexture = 0;
    m_pFrames = 0;
    m_Frame = 0;
}

// The hardware only takes power of two sizes up to 1024 ("pixel jar" sizes are anything else).
int CheckForPixelJar(int size)
{
    if (size == 2 || size == 4 || size == 8 || size == 0x10 || size == 0x20 || size == 0x40 || size == 0x80 ||
        size == 0x100 || size == 0x200 || size == 0x400)
        return 0;
    return 1;
}

signed char EXTexture::InitRuntimeTexture(unsigned short w, unsigned short h, unsigned char fmt, unsigned int flags)
{
    m_pHeader = (EXGeoTextureHeader *)_EXAlloc(0x58, 1);
    EXGeoTexture *pGeo = (EXGeoTexture *)((u8 *)m_pHeader + 0x1c);
    m_pHeader->SetRuntimeValues(pGeo, w, h, 0, 0);
    pGeo->SetRuntimeValues(w, h, fmt == 0x10 ? 1 : 6, 0, 0, 0);
    return !D3dCreateTexture(w, h, 1, 1, (xb::D3DFORMAT)Display()->m_PresentParams.BackBufferFormat, 0,
                             &m_pD3DTexture);
}

void EXTexture::DestroyRuntimeTexture()
{
    EXFree(m_pHeader);
    if (m_pD3DTexture) {
        D3DResource_Release(m_pD3DTexture);
        m_pD3DTexture = 0;
    }
}

void EXTexture::ReleaseD3DResource()
{
    if (m_pD3DTexture) {
        D3DResource_BlockUntilNotBusy(m_pD3DTexture);
        m_pD3DTexture = 0;
    }
    if (m_pFrames) {
        for (int i = 0; i < (int)m_NumFrames; i++) {
            xb::D3DResource *p = &m_pFrames[i];
            if (p) D3DResource_BlockUntilNotBusy(p);
        }
        EXFree(m_pFrames);
        m_pFrames = 0;
    }
}

void EXTexture::SetTiling(signed char on)
{
    if (on) {
        XBRenderState.SetTextureStageState(0, xb::D3DTSS_ADDRESSU, 1);
        XBRenderState.SetTextureStageState(0, xb::D3DTSS_ADDRESSV, 1);
    } else {
        XBRenderState.SetTextureStageState(0, xb::D3DTSS_ADDRESSU, 3);
        XBRenderState.SetTextureStageState(0, xb::D3DTSS_ADDRESSV, 3);
    }
}

void EXTexture::SetFiltered(signed char on)
{
    if (on) {
        XBRenderState.SetTextureStageState(0, xb::D3DTSS_MAGFILTER, 2);
        XBRenderState.SetTextureStageState(0, xb::D3DTSS_MINFILTER, 2);
        XBRenderState.SetTextureStageState(0, xb::D3DTSS_MIPFILTER, 2);
    } else {
        XBRenderState.SetTextureStageState(0, xb::D3DTSS_MAGFILTER, 1);
        XBRenderState.SetTextureStageState(0, xb::D3DTSS_MINFILTER, 1);
        XBRenderState.SetTextureStageState(0, xb::D3DTSS_MIPFILTER, 1);
    }
}

EXTexture::~EXTexture() { ReleaseD3DResource(); }

// Points the D3D texture header(s) at the file data: one header for a still texture, one per
// image for an animated one. Sizes the hardware cannot take, or data not 128-byte aligned, leave
// the texture unset (it draws with the null texture).
void EXTexture::Update(signed char)
{
    EXGeoTexture *pGeo = m_pHeader->Object(0);
    if (!pGeo) return;
    EXGeoTexture *pBase = pGeo->BaseTexture();
    if (!pBase) return;
    unsigned char *pSeq = pBase->AnimSeqData();
    unsigned char frame = pSeq ? pSeq[m_Frame] : m_Frame;
    unsigned char *pData = pBase->Frame(frame);
    if (!pData || CheckForPixelJar(pBase->m_Width) || CheckForPixelJar(pBase->m_Height) || (int)pData % 128) {
        m_pD3DTexture = 0;
        return;
    }
    xb::D3DTexture *pTex = (xb::D3DTexture *)m_Texture;
    XGSetTextureHeader(pBase->m_Width, pBase->m_Height, pBase->m_MipCount, 0, EXTexFmt[pBase->m_Format], 0, pTex, 0,
                       0);
    D3DResource_Register(pTex, pData);
    m_pD3DTexture = pTex;
    if (EXTexFmt[pBase->m_Format] == xb::D3DFMT_P8) InitialiseTexturePalette(pData, pBase);
    if (!m_ScrollSet && (pGeo->m_ScrollU || pGeo->m_ScrollV)) {
        m_ScrollRateU = (float)pGeo->m_ScrollU * (1.0f / 30000.0f);
        m_ScrollRateV = (float)pGeo->m_ScrollV * (1.0f / 30000.0f);
    }
    m_NumFrames = 0;
    if (pBase->m_FrameCount == 1) return;
    if (!m_pFrames) {
        m_pFrames = (xb::D3DTexture *)_EXAlloc(pBase->m_ImageCount * sizeof(xb::D3DTexture), 0);
        if (!m_pFrames) return;
    }
    for (unsigned int i = 0; i < pBase->m_ImageCount; i++) {
        unsigned char *p = pBase->Frame(i);
        XGSetTextureHeader(pBase->m_Width, pBase->m_Height, pBase->m_MipCount, 0, EXTexFmt[pBase->m_Format], 0,
                           &m_pFrames[i], 0, 0);
        if (EXTexFmt[pBase->m_Format] == xb::D3DFMT_P8) InitialiseTexturePalette(p, pBase);
        D3DResource_Register(&m_pFrames[i], p);
    }
    m_NumFrames = pBase->m_ImageCount;
}

// Binds the texture to stage 0 with its filtering and mip bias. A texture flagged to glow (header
// flag 0x8000) marks the window for bloom and writes alpha, unless the window has glow off or the
// entity being drawn is excluded.
void EXTexture::Select()
{
    u32 writeMask = 0x10101;
    s8 glow = 1;
    const EXCachedGeoEntity *pEnt = Display()->GetCurrentDeferredEntity();
    if (pEnt) {
        if (pEnt->m_ShadowCaster) glow = 0;
        if (pEnt->m_RenderInfo.RefractionRender()) glow = 0;
    }
    if (m_pHeader && (m_pHeader->Flags() & 0x8000) && (ActiveWnd()->m_Flags & 0x100000) && glow) {
        ((EXWnd *)ActiveWnd())->m_EfxFlags |= 1;
        writeMask = 0x1010101;
    }
    XBRenderState.SetRenderState(xb::D3DRS_COLORWRITEENABLE, writeMask);
    EXGeoTexture *pGeo = m_pHeader->Object(0);
    if (!pGeo) return;
    EXGeoTexture *pBase = pGeo->BaseTexture();
    if (!pBase) return;
    float bias = pBase->MipBias();
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_MIPMAPLODBIAS, *(xb::DWORD *)&bias);
    xb::D3DTexture *pTex;
    if (m_pD3DTexture) {
        u32 fmt = (m_pD3DTexture->Format >> 8) & 0xff;
        xb::D3DDevice::SetPalette(0, 0);
        if (fmt == xb::D3DFMT_P8)
            xb::D3DDevice::SetPalette(0, (u8 *)((m_pD3DTexture->m_Data | 0x80000000) + pBase->GetSize()) - 0x440);
        u32 flags = m_pHeader->Flags();
        u32 filter = 1, mip = 1;
        if (flags & 0x4000000) filter = 2;
        if (flags & 0x8000000) filter = 3;
        if (flags & 0x10000000) mip = 2;
        XBRenderState.SetTextureStageState(0, xb::D3DTSS_MAGFILTER, filter);
        XBRenderState.SetTextureStageState(0, xb::D3DTSS_MINFILTER, filter);
        XBRenderState.SetTextureStageState(0, xb::D3DTSS_MIPFILTER, mip);
        pTex = m_pD3DTexture;
    } else {
        xb::D3DDevice::SetPalette(0, 0);
        pTex = Display()->NullTexture();
    }
    XBRenderState.SetTexture(0, pTex);
}

// Scrolls the texture offsets and steps the animation. Header flag 0x1000000 holds an animation on
// its first frame until it is played, 0x2000000 stops it on the last.
void EXTexture::Animate(float dt)
{
    EXGeoTexture *pGeo = m_pHeader->Object(0);
    if (!pGeo) return;
    EXGeoTexture *pBase = pGeo->BaseTexture();
    if (!pBase) return;
    if (pGeo->m_ScrollU || m_ScrollSet) {
        m_UOffset += m_ScrollRateU * EXBaseApp::EngineRenderFrameRateMul() * dt;
        if (0.0f > m_UOffset) m_UOffset += 1.0f;
        if (m_UOffset > 1.0f) m_UOffset -= 1.0f;
    }
    if (pGeo->m_ScrollV || m_ScrollSet) {
        m_VOffset += m_ScrollRateV * EXBaseApp::EngineRenderFrameRateMul() * dt;
        if (0.0f > m_VOffset) m_VOffset += 1.0f;
        if (m_VOffset > 1.0f) m_VOffset -= 1.0f;
    }
    if (pBase->m_FrameCount == 1 || m_Paused) return;
    if (m_OnFirstFrame && (m_pHeader->Flags() & 0x1000000) && !m_Playing) {
        m_Paused = 1;
        return;
    }
    m_11 += (u8)(int)(EXBaseApp::EngineRenderFrameRateMul() + 0.05f);
    if (dt > 0.0f) {
        if (!pGeo->m_FrameRate ||
            m_11 >= (unsigned int)(int)((float)EXBaseApp::NominalEngineFrameRate() / ((float)pGeo->m_FrameRate * dt))) {
            m_Frame++;
            if (m_Frame >= pBase->m_FrameCount) m_Frame = 0;
            m_11 = 0;
            m_Playing = 0;
            unsigned char *pSeq = pBase->AnimSeqData();
            m_pD3DTexture = &m_pFrames[pSeq ? pSeq[m_Frame] : m_Frame];
            m_OnFirstFrame = m_Frame == 0;
            m_OnLastFrame = m_Frame == pBase->m_FrameCount - 1;
        }
    }
    if (m_OnLastFrame && (m_pHeader->Flags() & 0x2000000) && !m_Playing) m_Paused = 1;
}
