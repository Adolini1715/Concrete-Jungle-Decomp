#include "EXWnd.h"
#include "EXDisplay.h"
#include "EXTexture.h"
#include "EXBaseApp.h"
#include "EXGeoEntity.h"
#include "EXParticleSys.h"
#include "EXParticleSysMgr.h"
#include "EXEffectsSystems.h"
#include "EXSwooshSysMgr.h"
#include "EXPolyController.h"
#include "LineController.h"
#include "EXMaths.h"
#include "XBBaseEfx.h"
#include "XBBaseRenderState.h"
#include "XBRenderTargetManager.h"
#include "XBShadow.h"
#include "XBBaseVertexShader.h"
#include "EXSprite2D.h"
#include "xbd3d8.h"
#include "RGBA.h"
#include <string.h>

// The Xbox window: clears and frames the window, then after the scene has queued its entities
// draws them in passes (refraction, shadows, lights, heat, sorted alpha), the 2D layer, and the
// full-screen effects. EndDraw is the frame's render order.

long D3dCreateTexture(unsigned int w, unsigned int h, unsigned int levels, unsigned long usage, xb::D3DFORMAT fmt,
                      unsigned long pool, xb::D3DTexture **ppTex);
void XBRenderTexture(void *pTex, float x, float y, float w, float h, int mode, float a7, float srcW, float srcH,
                     float srcX, float srcY, ex::RGBA col);
extern "C" xb::DWORD __stdcall XGSwizzleRect(const void *pSource, xb::DWORD Pitch, const xb::tagRECT *pRect,
                                             void *pDest, xb::DWORD Width, xb::DWORD Height, const void *pPoint,
                                             xb::DWORD BytesPerPixel);
extern "C" xb::D3DXVECTOR3 *__stdcall D3DXVec3Normalize(xb::D3DXVECTOR3 *pOut, const xb::D3DXVECTOR3 *pV);
void xbMovieOpen(const char *pName, int a);
void xbMovieClose();
void xbMovieUpdate();
s8 xbMovieIsBusy();
s8 xbMovieCheckFreeMem();

// One entry of a depth sort: an entity or a particle system.
struct XBSortData {
    float m_Key;                    // 0x00 view depth
    EXCachedGeoEntity *m_pEntity;   // 0x04 m_Type 1
    EXParticleSysData *m_pSys;      // 0x08 m_Type 2
    int m_Type;                     // 0x0c
};

int XBRenderedShadows;
unsigned int XBRenderingShadows;
unsigned int XBShadowObjectId;
unsigned int XBRenderingLights;
xb::D3DTexture *pHeatTexture;
// Heat vision palette, cold to hot (16x1, A8R8G8B8).
unsigned int HeatData[16] = {
    0xff000000, 0xff004040, 0xff004040, 0xff004040, 0xff004040, 0xff004040, 0xff004040, 0xff004040,
    0xff002000, 0xff002000, 0xff002000, 0xff002000, 0xff002000, 0xff002000, 0xff304010, 0xff400010,
};

// Screen-space vertex of the blit quads (sizeof 0x18).
struct XBSPRITE2DW {
    float x, y, z;
    unsigned int diffuse;
    float u, v;
};

// Sprites that mask the expansion blur: their alpha decides where it shows (EXBaseWnd::m_pExpansionMask).
struct EXExpansionMask {
    EXTexture *m_pTexture;      // 0x00 null: a plain white texture
    unsigned int m_Count;       // 0x04
    EXSprite2D m_Sprites[1];    // 0x08
};

static inline EXDisplay *Display() { return (EXDisplay *)EXBaseDisplay::GetDisplay(); }

void XBCreateHeatTexture()
{
    D3dCreateTexture(0x10, 1, 1, 0, xb::D3DFMT_A8R8G8B8, 0, &pHeatTexture);
    if (pHeatTexture) {
        xb::D3DLOCKED_RECT lr;
        pHeatTexture->LockRect(0, &lr, 0, 0);
        XGSwizzleRect(HeatData, lr.Pitch, 0, lr.pBits, 0x10, 1, 0, 4);
        pHeatTexture->UnlockRect(0);
    }
}

// Bubble sorts on m_Key; XBSortBuffer and XBSortBufferDescending put the deepest first.
void XBSortBuffer(XBSortData *p, int n)
{
    for (int i = n - 1; i >= 0; i--) {
        int swaps = 0;
        for (int j = 1; j <= i; j++) {
            if (p[j].m_Key > p[j - 1].m_Key) {
                XBSortData t = p[j - 1];
                p[j - 1] = p[j];
                p[j] = t;
                swaps++;
            }
        }
        if (!swaps) break;
    }
}

void XBSortBufferAscending(XBSortData *p, int n)
{
    for (int i = n - 1; i >= 0; i--) {
        int swaps = 0;
        for (int j = 1; j <= i; j++) {
            if (p[j - 1].m_Key > p[j].m_Key) {
                XBSortData t = p[j - 1];
                p[j - 1] = p[j];
                p[j] = t;
                swaps++;
            }
        }
        if (!swaps) break;
    }
}

void XBSortBufferDescending(XBSortData *p, int n)
{
    for (int i = n - 1; i >= 0; i--) {
        int swaps = 0;
        for (int j = 1; j <= i; j++) {
            if (p[j].m_Key > p[j - 1].m_Key) {
                XBSortData t = p[j - 1];
                p[j - 1] = p[j];
                p[j] = t;
                swaps++;
            }
        }
        if (!swaps) break;
    }
}

void XBRenderAlphaBuffer(XBSortData *p, int n)
{
    for (int i = 0; i < n; i++, p++) {
        if (p->m_Type == 1) {
            p->m_pEntity->m_pEntity->DrawDirectX(p->m_pEntity);
        } else if (p->m_Type == 2) {
            if (p->m_pSys && p->m_pSys->m_pSys && p->m_pSys->m_pSys->IsVisible() && p->m_pSys->m_pSys->IsEnabled()) {
                if (p->m_pSys->m_pSys->CheckVisibility() == 1) {
                    p->m_pSys->m_pSys->Draw(1);
                    p->m_pSys->m_pSys->m_1ef = 0;
                } else if (p->m_pSys->m_pSys->SystemFlags() & 0x8000) {
                    p->m_pSys->m_pSys->m_1ef = 1;
                }
            }
        }
    }
}

void EXWnd::BeginDrawShadow() {}

void EXWnd::EndDrawShadow() {}

void EXWnd::MovieOpen(char *pName, int, int, int, int)
{
    m_MovieFinished = 0;
    EXString dir = EXBaseApp::GetApp()->AppName();
    char path[0x200];
    strcpy(path, "D:\\");
    strcat(path, dir.m_pchData);
    strcat(path, "\\Binary\\_bin_xb\\");
    strcat(path, pName);
    strcat(path, ".xmv");
    xbMovieOpen(path, 0);
}

void EXWnd::MovieUpdate()
{
    if (xbMovieIsBusy())
        xbMovieUpdate();
    else
        m_MovieFinished = 1;
}

void EXWnd::MovieClose()
{
    xbMovieClose();
    m_MovieFinished = 1;
}

s8 EXWnd::MovieCheckFreeMem() { return xbMovieCheckFreeMem(); }

void EXWnd::XBRenderSimpleShadowEntities() {}

EXWnd::~EXWnd() {}

// Debug: the screen rectangle of a box (corners a and b) under pMat, drawn as four white lines.
void EXWnd::Calculate2DScreenRect(EXMatrix *pMat, EXVector &a, EXVector &b, EXVector &outMin, EXVector &outMax)
{
    float sx = b.x - a.x, sz = b.z - a.z, sy = b.y - a.y;
    xb::D3DXVECTOR4 c[8];
    c[0].x = a.x; c[0].y = a.y; c[0].z = a.z; c[0].w = 1.0f;
    c[1].x = a.x + sx; c[1].y = a.y; c[1].z = a.z; c[1].w = 1.0f;
    c[2].x = a.x + sx; c[2].y = a.y; c[2].z = a.z + sz; c[2].w = 1.0f;
    c[3].x = a.x; c[3].y = a.y; c[3].z = a.z + sz; c[3].w = 1.0f;
    c[4].x = a.x; c[4].y = a.y + sy; c[4].z = a.z; c[4].w = 1.0f;
    c[5].x = a.x + sx; c[5].y = a.y + sy; c[5].z = a.z; c[5].w = 1.0f;
    c[6].x = a.x + sx; c[6].y = a.y + sy; c[6].z = a.z + sz; c[6].w = 1.0f;
    c[7].x = a.x; c[7].y = a.y + sy; c[7].z = a.z + sz; c[7].w = 1.0f;
    outMin.x = 10000.0f;
    outMin.y = 10000.0f;
    outMin.z = 10000.0f;
    outMax.x = -10000.0f;
    outMax.y = -10000.0f;
    outMax.z = -10000.0f;
    for (int i = 0; i < 8; i++) {
        xb::D3DXVECTOR4 t;
        D3DXVec4Transform(&t, &c[i], pMat);
        outMin.w = t.w;
        outMax.w = t.w;
        if (outMin.x > t.x) outMin.x = t.x;
        if (outMin.y > t.y) outMin.y = t.y;
        if (outMin.z > t.z) outMin.z = t.z;
        if (t.x > outMax.x) outMax.x = t.x;
        if (t.y > outMax.y) outMax.y = t.y;
        if (t.z > outMax.z) outMax.z = t.z;
    }
    outMin.x = (outMin.x / outMin.w + 1.0f) * 320.0f;
    outMin.y = (1.0f - outMin.y / outMin.w) * 240.0f;
    outMax.x = (outMax.x / outMax.w + 1.0f) * 320.0f;
    outMax.y = (1.0f - outMax.y / outMax.w) * 240.0f;
    _Line2D(0xffffffff, (int)outMax.x, (int)outMin.y, (int)outMin.x, (int)outMin.y);
    _Line2D(0xffffffff, (int)outMin.x, (int)outMin.y, (int)outMin.x, (int)outMax.y);
    _Line2D(0xffffffff, (int)outMin.x, (int)outMax.y, (int)outMax.x, (int)outMax.y);
    _Line2D(0xffffffff, (int)outMax.x, (int)outMax.y, (int)outMax.x, (int)outMin.y);
}

void EXWnd::BeginDraw()
{
    Display()->GetRenderTargetManager()->SetDefault();
    m_EfxFlags = 0;
    Display()->SetViewPort(&m_PrimRect, &m_PrimClipRect);
    if (m_Flags & 2) {
        // Opaque background: a full clear; otherwise blend it in.
        if (m_BkColor.m_A > 0x7f)
            D3DDevice_Clear(0, 0, 0xf0, m_BkColor.argb(), 0, 0);
        else
            _SolidRect2D(m_BkColor, 0, 0, m_PrimRect.w + 1, m_PrimRect.h + 1);
    }
    if (m_Flags & 0x100000) {
        // Clear the window's destination alpha.
        XBRenderState.SetRenderState(xb::D3DRS_COLORWRITEENABLE, 0x1000000);
        XBRenderState.SetRenderState(xb::D3DRS_ALPHAFUNC, 0x207);
        XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 1);
        XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 0);
        _SolidRect2D(0, 0, 0, m_PrimRect.w + 1, m_PrimRect.h + 1);
        XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 0x302);
        XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 0x303);
        XBRenderState.SetRenderState(xb::D3DRS_ALPHAFUNC, 0x204);
        XBRenderState.SetRenderState(xb::D3DRS_COLORWRITEENABLE, 0x10101);
    }
    if (m_Flags & 4) D3DDevice_Clear(0, 0, 3, 0, 1.0f, 0);
    if (m_Flags & 0x400) {
        // Border one pixel outside the window, highlighted when it has the focus.
        EXRect r(m_PrimRect);
        r.Inflate(1, 1);
        EXRect clip = IntersectRect(r, EXRect(0, 0, Display()->Width(), Display()->Height()));
        Display()->SetViewPort(&r, &clip);
        r.x -= m_PrimRect.x;
        r.y -= m_PrimRect.y;
        _Rect2D(Display()->FocusWnd() == this ? m_Unk80 : m_Unk7c, r.x, r.y, r.x + r.w - 1, r.y + r.h - 1);
        Display()->SetViewPort(&m_PrimRect, &m_PrimClipRect);
    }
}

s8 EXWnd::XBRenderShadowEntities()
{
    XBRenderedShadows = 0;
    if (m_ShadowCasters.Size()) {
        XBRenderingShadows = 1;
        XBShadowController.XBStartShadowRender();
        for (int i = 0; i < m_ShadowCasters.Size(); i++)
            m_ShadowCasters[i].m_pEntity->DrawDirectX(&m_ShadowCasters[i]);
        XBRenderingShadows = 0;
        XBShadowController.XBEndShadowRender();
        m_ShadowCasters.Empty(0);
        return 1;
    }
    return 0;
}

s8 EXWnd::XBRenderLightEntities()
{
    if (m_Lights.Size()) {
        XBRenderState.SetRenderState(xb::D3DRS_ALPHABLENDENABLE, 1);
        XBRenderState.SetRenderState(xb::D3DRS_ALPHATESTENABLE, 1);
        XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 0x302);
        XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 0x303);
        XBRenderState.SetTextureStageState(0, xb::D3DTSS_ALPHAOP, 4);
        XBRenderState.SetTextureStageState(0, xb::D3DTSS_COLOROP, 5);
        XBRenderState.SetRenderState(xb::D3DRS_SPECULARENABLE, 0);
        XBShadowController.XBStartLightRender();
        XBRenderingLights = 1;
        for (int i = 0; i < m_Lights.Size(); i++)
            m_Lights[i].m_pEntity->DrawDirectX(&m_Lights[i]);
        XBRenderingLights = 0;
        XBShadowController.XBEndLightRender();
        m_Lights.Empty(0);
        XBRenderState.EndShadowLightMapProjection();
        return 1;
    }
    XBRenderState.EndShadowLightMapProjection();
    return 0;
}

void EXWnd::XBRenderHeatEntities()
{
    for (int i = 0; i < m_Heat.Size(); i++)
        m_Heat[i].m_pEntity->DrawDirectX(&m_Heat[i]);
    m_Heat.Empty(0);
}

// Draw an array front to back (nearest first), then empty it.
void EXWnd::RenderEnitityArray(EXCachedGeoEntityArray &arr)
{
    if (arr.Size()) {
        XBSortData *pSort = (XBSortData *)_EXAlloc(arr.Size() * sizeof(XBSortData), 0);
        if (pSort) {
            XBSortData *p = pSort;
            for (int i = 0; i < arr.Size(); i++) {
                EXCachedGeoEntity &e = arr[i];
                EXMaths::vec_xyzw c = (e.m_pEntity->BoundsBox().m_vctMin + e.m_pEntity->BoundsBox().m_vctMax) * 0.5f;
                p->m_Key = (e.Transform() * EXVector(c.x, c.y, c.z, 1.0f)).w;
                p->m_pEntity = &e;
                p->m_Type = 1;
                p++;
            }
            XBSortBufferAscending(pSort, arr.Size());
            for (int j = 0; j < arr.Size(); j++)
                pSort[j].m_pEntity->m_pEntity->DrawDirectX(pSort[j].m_pEntity);
            arr.Empty(0);
            EXFree(pSort);
        }
    }
}

void EXWnd::RenderEffects()
{
    if (m_EfxFlags & 1) XBEfx.Bloom(5);
    if (m_Effects & 4) {
        XBEfx.XBCreateFocusBlurTextures();
        XBEfx.XBBlitZBufferToTexture();
        XBEfx.XBBlurFocusBlurTextures();
        XBRenderTexture(XBEfx.GetBackBufferAsTexture(), (float)m_PrimClipRect.x, (float)m_PrimClipRect.y,
                        (float)m_PrimClipRect.w, (float)m_PrimClipRect.h, 0x22, 0, 640.0f, 480.0f, 0, 0,
                        ex::RGBA(0xff, 0x80, 0x80, 0x80));
    }
    if (m_Effects & 0x100) DoExpansionBlur();
    if (m_Effects & 8) {
        XBEfx.XBCreateMotionBlurTexture();
        XBRenderTexture(XBEfx.m_pMotionBlur, (float)m_PrimClipRect.x, (float)m_PrimClipRect.y,
                        (float)m_PrimClipRect.w, (float)m_PrimClipRect.h, 0x42, GetMotionBlurFactor(), 640.0f, 480.0f,
                        0, 0, ex::RGBA(0xff, 0x80, 0x80, 0x80));
    }
    if (m_Effects & 1) {
        XBEfx.XBCreateEFXTexture(0x100, &XBEfx.m_pBlitSrc, 0);
        XBEfx.XBBlitBackBufferToTexture(XBEfx.m_pBlitSrc, 0, 0, 0x100, 0x100, 1, 3);
        if (XBEfx.m_pBlitSrc)
            XBRenderTexture(XBEfx.m_pBlitSrc, (float)m_PrimClipRect.x, (float)m_PrimClipRect.y,
                            (float)m_PrimClipRect.w, (float)m_PrimClipRect.h, 0x10, GetGlowIntensity(), 640.0f,
                            480.0f, 0, 0, ex::RGBA(0xff, 0x80, 0x80, 0x80));
    }
    if (m_Effects & 0x40)
        XBRenderTexture(XBEfx.GetBackBufferAsTexture(), (float)m_PrimClipRect.x, (float)m_PrimClipRect.y,
                        (float)m_PrimClipRect.w, (float)m_PrimClipRect.h, 0x102, 0, 640.0f, 480.0f, 0, 0,
                        ex::RGBA(0xff, 0x80, 0x80, 0x80));
    if (m_Effects & 0x80)
        XBRenderTexture(XBEfx.GetBackBufferAsTexture(), (float)m_PrimClipRect.x, (float)m_PrimClipRect.y,
                        (float)m_PrimClipRect.w, (float)m_PrimClipRect.h, 0x202, 0, 640.0f, 480.0f, 0, 0,
                        ex::RGBA(0xff, 0x80, 0x80, 0x80));
    if (m_Effects & 2) {
        XBEfx.XBCreateNoiseTexture();
        if (XBEfx.m_pNoise)
            XBRenderTexture(XBEfx.m_pNoise, (float)m_PrimClipRect.x, (float)m_PrimClipRect.y,
                            (float)m_PrimClipRect.w, (float)m_PrimClipRect.h, 0xc, GetNoiseIntensity(), 640.0f, 480.0f,
                            0, 0, ex::RGBA(0xff, 0x80, 0x80, 0x80));
    }
}

// Alpha pass for one layer: entities and the particle systems of the same layer go into three
// buckets, each sorted far to near and drawn in turn: positive sort values (and render orders 1/3)
// first, then plain depth sorted, then negative sort values (and render orders 2/4) on top.
void EXWnd::SortAndRenderEnitityArray(EXCachedGeoEntityArray &arr, unsigned char layer)
{
    int nZero = 0, nFront = 0, nBack = 0;
    int iZero = 0, iBack = 0, iFront = 0;
    int i;
    for (i = 0; i < arr.Size(); i++) {
        EXGeoEntity *pEnt = arr[i].m_pEntity;
        if (pEnt) {
            unsigned char o = pEnt->RenderOrder();
            if (o == 2 || o == 4) {
                nBack++;
            } else if (o == 1 || o == 3) {
                nFront++;
            } else {
                if (pEnt->SortValue() == 0) nZero++;
                if (pEnt->SortValue() < 0) nBack++;
                if (pEnt->SortValue() > 0) nFront++;
            }
        }
    }
    EXParticleSysMgr *pMgr = m_pActiveEffects->m_pParticleSysMgr;
    int nSys = 0;
    if (pMgr) {
        nSys = pMgr->SystemCount();
        nZero += nSys;
        nBack += nSys;
        nFront += nSys;
    }
    XBSortData *pZero = 0, *pFront = 0, *pBack = 0;
    if (nZero) {
        pZero = (XBSortData *)_EXAlloc(nZero * sizeof(XBSortData), 0);
        memset(pZero, 0, nZero * sizeof(XBSortData));
    }
    if (nFront) {
        pFront = (XBSortData *)_EXAlloc(nFront * sizeof(XBSortData), 0);
        memset(pFront, 0, nFront * sizeof(XBSortData));
    }
    if (nBack) {
        pBack = (XBSortData *)_EXAlloc(nBack * sizeof(XBSortData), 0);
        memset(pBack, 0, nBack * sizeof(XBSortData));
    }
    for (i = 0; i < arr.Size(); i++) {
        EXCachedGeoEntity &e = arr[i];
        EXGeoEntity *pEnt = e.m_pEntity;
        if (pEnt) {
            EXMaths::vec_xyzw c = (pEnt->BoundsBox().m_vctMax + pEnt->BoundsBox().m_vctMin) * 0.5f;
            float key = (e.Transform() * EXVector(c.x, c.y, c.z, 1.0f)).w;
            unsigned char o = pEnt->RenderOrder();
            if (o == 2 || o == 4) {
                pBack[iBack].m_Key = (float)pEnt->SortValue() * 16384.0f + key;
                pBack[iBack].m_pEntity = &e;
                pBack[iBack].m_Type = 1;
                iBack++;
            } else if (o == 1 || o == 3) {
                pFront[iFront].m_Key = key + 10000.0f;
                pFront[iFront].m_pEntity = &e;
                pFront[iFront].m_Type = 1;
                iFront++;
            } else {
                short s = (short)pEnt->SortValue();
                if (s == 0) {
                    pZero[iZero].m_Key = key;
                    pZero[iZero].m_pEntity = &e;
                    pZero[iZero].m_Type = 1;
                        iZero++;
                } else if (s < 0) {
                    pBack[iBack].m_Key = (float)s * 16384.0f + key;
                    pBack[iBack].m_pEntity = &e;
                    pBack[iBack].m_Type = 1;
                        iBack++;
                } else if (s > 0) {
                    pFront[iFront].m_Key = (float)s * 16384.0f + key;
                    pFront[iFront].m_pEntity = &e;
                    pFront[iFront].m_Type = 1;
                        iFront++;
                }
            }
        }
    }
    if (nSys) {
        for (int j = 0; j < nSys; j++) {
            if (pMgr->StopSystems() == 1) continue;
            EXParticleSysData *d = pMgr->ParticleSysData(j);
            if (!d) continue;
            int l = d->m_pSys->m_Layer;
            if (l != layer) {
                if (layer == 4) {
                    if (l != 2) continue;
                } else if (layer != 3 || l != 1) {
                    continue;
                }
            }
            EXVector pos = d->m_pSys->LastEmitPos();
            EXVector v(pos.x, pos.y, pos.z, 1.0f);
            float key = 0.0f;
            if (d->m_pSys->m_pViewToScreenCoords && d->m_pSys->m_pWorldToView)
                key = ((*d->m_pSys->m_pViewToScreenCoords * *d->m_pSys->m_pWorldToView) * v).z;
            if (l == 2 || l == 4) {
                pBack[iBack].m_Key = d->m_pSys->m_SortBias + key;
                pBack[iBack].m_pSys = d;
                pBack[iBack].m_Type = 2;
                iBack++;
            } else if (l == 1 || l == 3) {
                pFront[iFront].m_Key = key + 10000.0f;
                pFront[iFront].m_pSys = d;
                pFront[iFront].m_Type = 2;
                iFront++;
            } else {
                float f = d->m_pSys->m_SortBias;
                if (f == 0.0f) {
                    pZero[iZero].m_Key = key;
                    pZero[iZero].m_pSys = d;
                    pZero[iZero].m_Type = 2;
                        iZero++;
                } else if (f < 0.0f) {
                    pBack[iBack].m_Key = f + key;
                    pBack[iBack].m_pSys = d;
                    pBack[iBack].m_Type = 2;
                        iBack++;
                } else if (f > 0.0f) {
                    pFront[iFront].m_Key = f + key;
                    pFront[iFront].m_pSys = d;
                    pFront[iFront].m_Type = 2;
                        iFront++;
                }
            }
        }
    }
    if (pFront) {
        XBSortBufferDescending(pFront, iFront);
        XBRenderAlphaBuffer(pFront, iFront);
        EXFree(pFront);
    }
    if (pZero) {
        XBSortBufferDescending(pZero, iZero);
        XBRenderAlphaBuffer(pZero, iZero);
        EXFree(pZero);
    }
    if (pBack) {
        XBSortBufferDescending(pBack, iBack);
        XBRenderAlphaBuffer(pBack, iBack);
        EXFree(pBack);
    }
    arr.Empty(0);
}

void EXWnd::EndDraw()
{
    ParticleSysMgr()->Draw(0);
    if (m_Effects & 0x20) {
        // Refraction: draw what refracts first, then copy the back buffer for the refracting surfaces.
        RenderEnitityArray(m_RefractionPre);
        XBEfx.XBBlitBackBufferToTexture(0, 0, 0, m_PrimClipRect.w, m_PrimClipRect.h, 0, 0);
    }
    RenderEnitityArray(m_1e8);
    s8 shadows = XBRenderShadowEntities();
    shadows |= RenderSortedShadowArray(m_ShadowReceivers);
    if (shadows != 1) XBShadowController.DisableShadowMap();
    s8 lights = XBRenderLightEntities();
    if (lights != 1) XBShadowController.DisableLightMap();
    if (shadows || lights) Display()->SetViewPort(&m_PrimRect, &m_PrimClipRect);
    RenderEnitityArray(m_178);
    RenderEnitityArray(m_238);
    XBRenderHeatEntities();
    SortAndRenderEnitityArray(m_Alpha, 0);
    g_ExPolyController.RenderPolys();
    if (m_pActiveEffects && m_pActiveEffects->m_pSwooshSysMgr) m_pActiveEffects->m_pSwooshSysMgr->Draw();
    MovieUpdate();
    RenderEnitityArray(m_1e8);
    SortAndRenderEnitityArray(m_208, 3);
    LineControl.RenderLines();
    DrawSprite2DData();
    if (m_Effects & 0x20) {
        XBEfx.XBBlitZBufferToTexture();
        XBRenderTexture(XBEfx.GetBackBufferAsTexture(), (float)m_PrimClipRect.x, (float)m_PrimClipRect.y,
                        (float)m_PrimClipRect.w, (float)m_PrimClipRect.h, 0x82, 0, 640.0f, 480.0f, 0, 0,
                        ex::RGBA(0xff, 0x80, 0x80, 0x80));
    }
    XBRenderRefractionEntities();
    RenderEnitityArray(m_1f8);
    SortAndRenderEnitityArray(m_218, 4);
    XBRenderState.SetRenderState(xb::D3DRS_COLORWRITEENABLE, 0x1010101);
    RenderEffects();
    if (m_CaptureRequested) {
        XBEfx.XBBlitWindowToTexture(m_pCaptureTexture->m_pD3DTexture, m_PrimClipRect.x,
                                    m_PrimClipRect.y, m_PrimClipRect.w, m_PrimClipRect.h);
        m_CaptureRequested = 0;
    }
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ALPHAOP, 5);
    if (m_Flags & 8) DrawDebugText();
    DrawDebugItems();   // GUESS: placement (no code; retail emits the empty inline here)
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ALPHAOP, 4);
}

// Shadow projection: flatten the object along the light direction (horizontal part only) onto the
// plane h above its origin.
void CalcShadowSkewMatrix(xb::D3DXMATRIX *pOut, xb::D3DXMATRIX *pIn, float h, xb::D3DXVECTOR3 *pLight)
{
    xb::D3DXVECTOR3 pos(pIn->_41, pIn->_42, pIn->_43);
    xb::D3DXVECTOR3 d = pos - *pLight;
    xb::D3DXMATRIX m;
    xb::D3DXMatrixIdentity(&m);
    d.y = 0.0f;
    D3DXVec3Normalize(&d, &d);
    m._21 = d.x;
    m._22 = 0.0f;
    m._23 = d.z;
    D3DXMatrixMultiply(pOut, pIn, &m);
    pOut->_41 = pos.x;
    pOut->_42 = pos.y + h;
    pOut->_43 = pos.z;
}

__forceinline void XBBuildBlitQuad(XBSPRITE2DW *p, float x, float y, float w, float h, float u, float v, float uw, float vh,
                            ex::RGBA col)
{
    unsigned int c = col.argb();
    float u1 = u + uw;
    float v1 = v + vh;
    p->x = x;
    p->y = y;
    p->z = XBVertexShader.m_ScreenZ + 100.0f;
    p->diffuse = c;
    p->u = u;
    p->v = v;
    p++;
    p->x = x + w;
    p->y = y;
    p->z = XBVertexShader.m_ScreenZ + 100.0f;
    p->diffuse = c;
    p->u = u1;
    p->v = v;
    p++;
    p->x = x + w;
    p->y = y + h;
    p->z = XBVertexShader.m_ScreenZ + 100.0f;
    p->diffuse = c;
    p->u = u1;
    p->v = v1;
    p++;
    p->x = x;
    p->y = y + h;
    p->z = XBVertexShader.m_ScreenZ + 100.0f;
    p->diffuse = c;
    p->u = u;
    p->v = v1;
}

__forceinline void XBRenderTexture(xb::D3DTexture *pTex, const XBSPRITE2DW *p)
{
    XBRenderState.SetTexture(0, pTex);
    D3DDevice_DrawVerticesUP(8, 4, p, sizeof(XBSPRITE2DW));
}

// A sprite in the 512x448 2D space, drawn as a quad in screen pixels.
__forceinline void XBRenderExpansionSprite(xb::D3DTexture *pTex, const EXSprite2D &s)
{
    float sx = (float)Display()->Width() * (1.0f / 512.0f);
    float sy = (float)Display()->Height() * (1.0f / 448.0f);
    XBSPRITE2DW q[4];
    float x = (float)s.x * sx * 0.5f;
    float y = (float)s.y * sy * 0.5f;
    XBBuildBlitQuad(q, x, y, (float)s.w * sx * 0.5f, (float)s.h * sy * 0.5f, s.u0, s.v0, s.u1 - s.u0, s.v1 - s.v0,
                    s.m_Color);
    XBRenderTexture(pTex, q);
}

// Expansion blur (effect 0x100): copy the back buffer, blend in progressively zoomed copies of it
// (ping-pong between two screen-sized textures), mask the result's alpha with the window's
// expansion sprites, and blend it back over the back buffer.
void EXWnd::DoExpansionBlur()
{
    XBSPRITE2DW quadA[4];
    float c[4];
    c[0] = 0.0f;
    c[1] = 0.0f;
    XBVertexShader.SetScrollUVs(c);
    XBRenderState.SetRenderState(xb::D3DRS_BLENDOP, 0x8006);
    XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 0x302);
    XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 0x303);
    XBRenderState.SetRenderState(xb::D3DRS_CULLMODE, 0x901);
    XBRenderState.SetRenderState(xb::D3DRS_ALPHABLENDENABLE, 1);
    XBRenderState.SetRenderState(xb::D3DRS_ALPHAREF, 0);
    XBRenderState.SetRenderState(xb::D3DRS_ZENABLE, 0);
    XBRenderState.SetTexture(1, 0);
    XBRenderState.SetTexture(2, 0);
    XBVertexShader.SetVertexShader(12);
    XBBuildBlitQuad(quadA, 0, 0, Display()->HalfWidth(), Display()->HalfHeight(), 0, 0, Display()->HalfWidth(),
                    Display()->HalfHeight(), ex::RGBA(0x80, 0x80, 0x80, 0xff));
    XBSPRITE2DW quadB[4];
    XBBuildBlitQuad(quadB, 0, 0, Display()->HalfWidth(), Display()->HalfHeight(), 0, 0, Display()->HalfWidth(),
                    Display()->HalfHeight(), ex::RGBA(0x80, 0x80, 0x80, 0xff));
    XBSPRITE2DW quadC[4];
    XBBuildBlitQuad(quadC, 0, 0, (float)Display()->Width(), (float)Display()->Height(), 0, 0,
                    Display()->HalfWidth(), Display()->HalfHeight(),
                    ex::RGBA(0x80, 0x80, 0x80, (unsigned char)(m_ExpansionAlpha * 255.0f)));
    xb::D3DSurface *pDepth = D3DDevice_GetDepthStencilSurface2();
    xb::D3DSurface *pColour = D3DDevice_GetRenderTarget2();
    XBEfx.XBCreateEFXTexture((int)Display()->HalfWidth(), (int)Display()->HalfHeight(), &XBEfx.m_pBloomHalf, 1);
    XBEfx.XBCreateEFXTexture((int)Display()->HalfWidth(), (int)Display()->HalfHeight(), &XBEfx.m_pExpansion, 1);
    XBEfx.XBBlitBackBufferToTexture(XBEfx.m_pBloomHalf, 0, 0, (int)Display()->HalfWidth(),
                                    (int)Display()->HalfHeight(), 0, 0);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ALPHAARG1, 0);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ALPHAOP, 2);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ADDRESSU, 3);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ADDRESSV, 3);
    xb::D3DSurface *pSurf = D3DTexture_GetSurfaceLevel2(XBEfx.m_pExpansion, 0);
    Display()->SetRenderTarget(pSurf, 0);
    XBRenderTexture(XBEfx.m_pBloomHalf, quadA);
    D3DResource_Release(pSurf);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ALPHAOP, 2);
    float sx = Display()->HalfWidth() * (1.0f / 256.0f);
    float sy = Display()->HalfHeight() * (1.0f / 224.0f);
    quadA[0].diffuse = quadA[1].diffuse = quadA[2].diffuse = quadA[3].diffuse = 0x80808080;
    float stepX = m_ExpansionStep * sx;
    float insetX = sx * m_ExpansionStart;
    float stepY = m_ExpansionStep * sy;
    float insetY = sy * m_ExpansionStart;
    for (unsigned int i = 0; i < m_ExpansionPasses; i++) {
        pSurf = D3DTexture_GetSurfaceLevel2(XBEfx.m_pExpansion, 0);
        Display()->SetRenderTarget(pSurf, 0);
        quadA[0].u = insetX;
        quadA[0].v = insetY;
        quadA[1].u = Display()->HalfWidth() - insetX;
        quadA[1].v = insetY;
        quadA[2].u = Display()->HalfWidth() - insetX;
        quadA[2].v = Display()->HalfHeight() - insetY;
        quadA[3].u = insetX;
        quadA[3].v = Display()->HalfHeight() - insetY;
        XBRenderTexture(XBEfx.m_pBloomHalf, quadA);
        D3DResource_Release(pSurf);
        if (i != m_ExpansionPasses - 1) {
            pSurf = D3DTexture_GetSurfaceLevel2(XBEfx.m_pBloomHalf, 0);
            Display()->SetRenderTarget(pSurf, 0);
            XBRenderTexture(XBEfx.m_pExpansion, quadB);
            D3DResource_Release(pSurf);
        }
        insetX += stepX;
        insetY += stepY;
    }
    // Alpha: opaque without a mask, else zero and the mask sprites written into it.
    D3DDevice_Clear(0, 0, 0x80, m_pExpansionMask ? 0 : 0xff000000, 1.0f, 0);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ALPHAARG1, 2);
    if (m_pExpansionMask) {
        XBRenderState.SetTextureStageState(0, xb::D3DTSS_ALPHAOP, 5);
        xb::D3DTexture *pTex = Display()->NullTexture();
        XBRenderState.SetRenderState(xb::D3DRS_COLORWRITEENABLE, 0x1000000);
        if (m_pExpansionMask->m_pTexture) pTex = m_pExpansionMask->m_pTexture->m_pD3DTexture;
        for (unsigned int j = 0; j < m_pExpansionMask->m_Count; j++)
            XBRenderExpansionSprite(pTex, m_pExpansionMask->m_Sprites[j]);
        XBRenderState.SetRenderState(xb::D3DRS_COLORWRITEENABLE, 0x1010101);
        XBRenderState.SetTextureStageState(0, xb::D3DTSS_ALPHAOP, 4);
    }
    Display()->SetRenderTarget(pColour, pDepth);
    D3DResource_Release(pDepth);
    D3DResource_Release(pColour);
    XBRenderTexture(XBEfx.m_pExpansion, quadC);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ALPHAOP, 4);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ADDRESSU, 1);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ADDRESSV, 1);
}

// Shadows sorted far to near; each receiver gets a new shadow object id, except that consecutive
// skinned pieces (m_pSkinData set) share one.
s8 EXWnd::RenderSortedShadowArray(EXCachedGeoEntityArray &arr)
{
    if (arr.Size()) {
        XBSortData *pSort = (XBSortData *)_EXAlloc(arr.Size() * sizeof(XBSortData), 0);
        if (!pSort) return 0;
        XBSortData *p = pSort;
        for (int i = 0; i < arr.Size(); i++) {
            EXCachedGeoEntity &e = arr[i];
            EXMaths::vec_xyzw c = (e.m_pEntity->BoundsBox().m_vctMin + e.m_pEntity->BoundsBox().m_vctMax) * 0.5f;
            p->m_Key = (e.ShadowTransform() * EXVector(c.x, c.y, c.z, 1.0f)).w;
            p->m_pEntity = &e;
            p->m_Type = 1;
            p++;
        }
        XBSortBufferDescending(pSort, arr.Size());
        XBRenderingShadows = 1;
        XBShadowController.XBStartShadowRender();
        XBShadowObjectId = 2;
        int bGroup = 0;
        for (int j = 0; j < arr.Size(); j++) {
            EXCachedGeoEntity *e = pSort[j].m_pEntity;
            if (j > 0) {
                if (!e->m_pSkinData) bGroup = 0;
                if (!bGroup) XBShadowObjectId += 0x12;
                if (e->m_pSkinData) bGroup = 1;
            }
            e->m_pEntity->ShadowObjectId() = (unsigned short)XBShadowObjectId;
            e->m_pEntity->DrawDirectX(e);
        }
        XBRenderingShadows = 0;
        XBShadowController.XBEndShadowRender();
        arr.Empty(0);
        EXFree(pSort);
        XBRenderedShadows = 1;
        return 1;
    }
    XBRenderedShadows = 1;
    return 0;
}

// Refracting entities: their shaders read a 256x256 copy of the back buffer; with m_EfxFlags bit 1
// that copy is first blurred into a second texture (m_RefractionPasses offset draws).
void EXWnd::XBRenderRefractionEntities()
{
    if (m_Refraction.Size() > 0) {
        XBEfx.XBCreateEFXTexture(0x100, &XBEfx.m_pBlitSrc, 0);
        XBEfx.XBBlitBackBufferToTexture(XBEfx.m_pBlitSrc, 0, 0, 0x100, 0x100, 0, 0);
        if (m_EfxFlags & 2) {
            XBEfx.XBCreateEFXTexture(0x100, &XBEfx.m_pRefractionBlur, 0);
            float n = (float)m_RefractionPasses;
            float d[4];
            d[0] = m_RefractionScale[0] * 256.0f / n;
            d[1] = m_RefractionScale[1] * 256.0f / n;
            d[2] = m_RefractionScale[2] * 256.0f / n;
            d[3] = m_RefractionScale[3] * 256.0f / n;
            xb::D3DSurface *pDepth = D3DDevice_GetDepthStencilSurface2();
            xb::D3DSurface *pColour = D3DDevice_GetRenderTarget2();
            EXRectf r(0.0f, 0.0f, 256.0f, 256.0f);
            xb::D3DSurface *pSurf = D3DTexture_GetSurfaceLevel2(XBEfx.m_pRefractionBlur, 0);
            Display()->SetRenderTarget(pSurf, 0);
            // Each pass draws the blit source shrunk by the scale steps, sampling a window that grows
            // by the same amount, so successive passes smear the image outwards.
            float n2 = (float)m_RefractionPasses;
            float du = d[0] * (1.0f / 256.0f);
            float dv = d[1] * (1.0f / 256.0f);
            float u = du * n2;
            float v = dv * n2;
            float uw = (1.0f - u) * 0.5f;
            float vh = (1.0f - v) * 0.5f;
            for (unsigned int i = 0; i < m_RefractionPasses; i++) {
                XBRenderTexture(XBEfx.m_pBlitSrc, r.x, r.y, r.w, r.h, 0x42, 1.0f, uw, vh, u, v,
                                ex::RGBA(0xff, 0x80, 0x80, 0x80));
                u -= du;
                v -= dv;
                uw += d[2] * (1.0f / 256.0f) * 2.0f;
                vh += d[3] * (1.0f / 256.0f) * 2.0f;
                r.x += d[0];
                r.y += d[1];
                r.w -= d[2] * 2.0f;
                r.h -= d[3] * 2.0f;
            }
            D3DResource_Release(pSurf);
            D3DResource_Release(pColour);
            D3DResource_Release(pDepth);
            Display()->SetRenderTarget(pColour, pDepth);
            Display()->SetViewPort(&m_PrimRect, &m_PrimClipRect);
        }
    }
    for (int i = 0; i < m_Refraction.Size(); i++)
        m_Refraction[i].m_pEntity->DrawDirectX(&m_Refraction[i]);
    m_Refraction.Empty(0);
}

EXWnd::EXWnd()
{
    if (Display())
        m_pDevice = Display()->Device();
    else
        m_pDevice = 0;
    m_EfxFlags = 0;
}
