#include "EXBaseRenderEnv.h"
#include "EXTexSwapInfo.h"
#include "EXGeoFile.h"
#include "EXGeoHeader.h"
#include "EXTexture.h"
#include "EXLightManager.h"
#include "EXBounds.h"
#include "RGBA.h"

// Render environment statics, render-info merging and texture swaps (EXRenderEnv.obj).

static u32 s_RenderDrawType = 0xffffffff;  // draw type last applied by Update
EXRenderInfo EXBaseRenderEnv::ms_RenderInfo;
EXRenderInfo EXBaseRenderEnv::ms_GRenderInfo;
EXDynLightData *EXBaseRenderEnv::ms_DynLights[4];
u16 EXBaseRenderEnv::ms_RefractTexHeight;
u16 EXBaseRenderEnv::ms_RefractTexWidth;
u32 EXBaseRenderEnv::ms_RenderState;
s8 EXBaseRenderEnv::ms_WideScreen;
u32 EXBaseRenderEnv::ms_ContrastModifier;
s8 EXBaseRenderEnv::ms_DistanceFade;
u8 EXBaseRenderEnv::ms_DistanceFadeLayerMask;
float EXBaseRenderEnv::ms_DistanceFadeStart;
float EXBaseRenderEnv::ms_DistanceFadeEnd;
s8 EXBaseRenderEnv::ms_ShadowRender;
s8 EXBaseRenderEnv::ms_ReflectionRender;
float EXBaseRenderEnv::ms_ReflectionHeight;
EXLightManager *EXBaseRenderEnv::ms_pLightManager;
u32 EXBaseRenderEnv::ms_DynLightCount;
s8 EXBaseRenderEnv::ms_DynLightRGBMultEnable;
u32 EXBaseRenderEnv::ms_RenderDrawType;
u32 EXBaseRenderEnv::ms_RenderFlashingBackground;
u32 EXBaseRenderEnv::ms_RenderItemDir;
s8 EXBaseRenderEnv::ms_RenderPortals;
s8 EXBaseRenderEnv::ms_RenderLights;
s8 EXBaseRenderEnv::ms_RenderSounds;
s8 EXBaseRenderEnv::ms_RenderBspGrid;
s8 EXBaseRenderEnv::ms_RenderGrabPoints;
s8 EXBaseRenderEnv::ms_RenderMapZones;
s8 EXBaseRenderEnv::ms_RenderEntityBounds;
s8 EXBaseRenderEnv::ms_RenderScoopCollision;
s8 EXBaseRenderEnv::ms_RenderPaths;
s8 EXBaseRenderEnv::ms_RenderAnimBones;
s8 EXBaseRenderEnv::ms_RenderDynLights;
s8 EXBaseRenderEnv::ms_RenderCameras;
s8 EXBaseRenderEnv::ms_RenderClipBounds;
s8 EXBaseRenderEnv::ms_RenderNavMesh;
s8 EXBaseRenderEnv::ms_RenderNavConnections;
s8 EXBaseRenderEnv::ms_RenderNavPaths;
EXRenderEnv g_RenderEnv;
EXRenderFogInfo EXBaseRenderEnv::ms_RenderFogInfo;
EXMaths::vec_xyzw EXBaseRenderEnv::ms_DynLightRGBMult = EXVector(1.0f, 1.0f, 1.0f, 1.0f);
EXRenderFogInfo EXBaseRenderEnv::ms_OverrideFogInfo;

void EXTexSwapInfo::ResolveHashCodes(EXTexSwapEntry &e) const
{
    if (!e.m_pFile) e.m_pFile = EXGeoFile::GetGeoFile(e.m_FileHash);
    if (!(e.m_Tex & 0x80000000) && e.m_pFile) {
        EXGeoHeader *h = e.m_pFile->GeoHeader();
        if (h) e.m_Tex = h->GeoObjectHashIndex(e.m_Tex);
    }
}

EXBaseRenderEnv::EXBaseRenderEnv()
{
    ms_RenderFogInfo.m_enabled = 0;
    ms_RenderFogInfo.m_color = ex::RGBA(0x80, 0x80, 0x80, 0x80);
    ms_RenderFogInfo.m_near = 1.175494351e-38f;
    ms_RenderFogInfo.m_far = 3.402823466e+38f;
    ms_RenderFogInfo.m_min = 1.175494351e-38f;
    ms_RenderFogInfo.m_max = 3.402823466e+38f;
    ms_OverrideFogInfo.m_enabled = 0;
    ms_OverrideFogInfo.m_color = ex::RGBA(0x40, 0x40, 0x40, 0x7f);
    ms_OverrideFogInfo.m_near = 50.0f;
    ms_OverrideFogInfo.m_far = 120.0f;
    ms_RefractTexWidth = 128;
    ms_RefractTexHeight = 128;
}

EXBaseRenderEnv::~EXBaseRenderEnv() {}

void EXBaseRenderEnv::SetDynLights(const EXBoundsBox &box)
{
    if (ms_pLightManager && ms_pLightManager->MapZoneLightCount() > 0) {
        ms_pLightManager->SetRenderEnvLights(box);
        return;
    }
    ms_DynLightCount = 0;
}

void EXBaseRenderEnv::RegisterWatcherVars() {}

EXTexSwapInfo::~EXTexSwapInfo() {}

signed char EXTexSwapInfo::StopSwap(unsigned int src)
{
    for (int i = 0; i < m_Swaps.Size(); i++) {
        if (m_Swaps[i].m_Src == src) {
            m_Swaps.RemoveAt(i, 1);
            m_ChangeNum++;
            return 1;
        }
    }
    return 0;
}

signed char EXTexSwapInfo::StopCatchAllSwap()
{
    if (m_Swaps.Size() > 0 && m_Swaps[0].m_Src == 0xfffffffe) {
        m_Swaps.RemoveAt(0, 1);
        m_ChangeNum++;
        return 1;
    }
    return 0;
}

EXTexture *EXTexSwapInfo::SwappedTexture(EXTexture *pTex)
{
    if (!m_Swaps.Size()) return pTex;
    EXTexture *pRet = pTex;
    int start = 0;
    EXTexSwapEntry &c = m_Swaps[0];
    if (c.m_Src == 0xfffffffe) {
        EXTexture *p;
        if (c.m_FileHash == 0xffffffff) p = (EXTexture *)c.m_pFile;
        else {
            if (!c.m_pFile || !(c.m_Tex & 0x80000000)) ResolveHashCodes(c);
            if (!c.m_pFile) goto search;
            EXGeoHeader *h = c.m_pFile->GeoHeader();
            if (!h) goto search;
            p = h->Texture(c.m_Tex);
            if (!p) goto search;
        }
        pRet = p;
        start = 1;
    }
search:
    for (int i = start; i < m_Swaps.Size(); i++) {
        EXTexSwapEntry &e = m_Swaps[i];
        if (e.m_Src == pTex->HashCode()) {
            if (e.m_FileHash == 0xffffffff) return (EXTexture *)e.m_pFile;
            if (!e.m_pFile || !(e.m_Tex & 0x80000000)) ResolveHashCodes(e);
            if (e.m_pFile) {
                EXGeoHeader *h = e.m_pFile->GeoHeader();
                if (h) {
                    EXTexture *p = h->Texture(e.m_Tex);
                    if (p) return p;
                }
            }
            return pRet;
        }
    }
    return pRet;
}

const EXRenderInfo &EXRenderInfo::Merge(EXRenderInfo &out, const EXRenderInfo &a, const EXRenderInfo &b)
{
    out = b;
    if (a.m_1a > out.m_1a) out.m_1a = a.m_1a;
    unsigned int bf = b.m_renderFlags;
    unsigned int af = a.m_renderFlags;
    unsigned int f = af | bf;
    if (bf & 6) {
        if (af & 8) {
            f = (f & ~6) | (af & 6);
            *(EXMaths::vec_xyzw *)&out.m_rgbaMult = a.m_rgbaMult;
        } else if (af & 6) {
            out.m_rgbaMult *= a.m_rgbaMult;
        }
    } else {
        *(EXMaths::vec_xyzw *)&out.m_rgbaMult = a.m_rgbaMult;
    }
    if (af & 0x98c0) {
        if (af & 0x800) {
            out.m_lightSet1 = a.m_lightSet1;
            out.m_lightSet2 = a.m_lightSet2;
            f |= 0x800;
            out.m_lightSetBlend = a.m_lightSetBlend;
        }
        if (af & 0x1000) {
            out.m_vertexSet1 = a.m_vertexSet1;
            out.m_vertexSet2 = a.m_vertexSet2;
            f |= 0x1000;
            out.m_vertexSetBlend = a.m_vertexSetBlend;
        }
        if (af & 0x40) {
            f |= 0x40;
            out.m_pExtRenderInfo = a.m_pExtRenderInfo;
        }
        if (af & 0x80) {
            f |= 0x80;
            out.m_pTexSwapInfo = a.m_pTexSwapInfo;
        }
        if (af & 0x8000) out.m_shadowType = a.m_shadowType;
    }
    out.m_renderFlags = (u16)f;
    int d = (s16)a.m_shadowDensity;
    if (bf & 0x200) d = limit<int>((s16)out.m_shadowDensity * d / 32767, 0, 0x7fff);
    out.m_shadowDensity = (u16)d;
    return out;
}

void EXBaseRenderEnv::Update()
{
    if (s_RenderDrawType == ms_RenderDrawType) return;
    switch (ms_RenderDrawType) {
    case 0:
        ms_GRenderInfo.m_renderFlags.BitClr(1);
        ms_GRenderInfo.m_1a = 0;
        break;
    case 1:
        ms_GRenderInfo.m_renderFlags.BitSet(1);
        ms_GRenderInfo.m_1a = 6;
        break;
    case 2:
        ms_GRenderInfo.m_renderFlags.BitClr(1);
        ms_GRenderInfo.m_1a = 1;
        break;
    case 3:
        ms_GRenderInfo.m_renderFlags.BitSet(1);
        ms_GRenderInfo.m_1a = 0;
        break;
    case 4:
        ms_GRenderInfo.m_renderFlags.BitClr(1);
        ms_GRenderInfo.m_1a = 2;
        break;
    case 5:
        ms_GRenderInfo.m_renderFlags.BitSet(1);
        ms_GRenderInfo.m_1a = 3;
        break;
    case 6:
        ms_GRenderInfo.m_renderFlags.BitClr(1);
        ms_GRenderInfo.m_1a = 5;
        break;
    case 7:
        ms_GRenderInfo.m_renderFlags.BitClr(1);
        ms_GRenderInfo.m_1a = 7;
        break;
    }
    s_RenderDrawType = ms_RenderDrawType;
}

inline unsigned int EXVectorToRGBA(const EXMaths::vec_xyzw &v)
{
    float r = limit(v.x, 0.0f, 255.0f);
    float g = limit(v.y, 0.0f, 255.0f);
    float b = limit(v.z, 0.0f, 255.0f);
    float a = limit(v.w, 0.0f, 255.0f);
    return ((u32)(u8)a << 24) | ((u32)(u8)b << 16) | ((u32)(u8)g << 8) | (u8)r;
}

// Finds the entry for src, or appends one.
#define TEXSWAP_FIND_OR_ADD(cmp_same)                  \
    int i = -1;                                        \
    for (int j = 0; j < m_Swaps.Size(); j++) {         \
        if (m_Swaps[j].m_Src == src) {                 \
            EXTexSwapEntry &o = m_Swaps[j];            \
            if (cmp_same) return;                      \
            i = j;                                     \
            break;                                     \
        }                                              \
    }                                                  \
    if (i < 0) {                                       \
        i = m_Swaps.Size();                            \
        m_Swaps.SetSize(i + 1, -1);                    \
    }

void EXTexSwapInfo::SetSwap(unsigned int src, unsigned int file, unsigned int tex)
{
    TEXSWAP_FIND_OR_ADD(o.m_Tex == tex && o.m_FileHash == file)
    EXTexSwapEntry &e = m_Swaps[i];
    e.m_Src = src;
    e.m_FileHash = file;
    e.m_pFile = 0;
    e.m_Tex = tex;
    ResolveHashCodes(e);
    m_ChangeNum++;
}

void EXTexSwapInfo::SetSwap(unsigned int src, EXGeoFile *pFile, unsigned int tex)
{
    TEXSWAP_FIND_OR_ADD(o.m_Tex == tex && o.m_pFile == pFile)
    EXTexSwapEntry &e = m_Swaps[i];
    e.m_Src = src;
    e.m_FileHash = 0;
    e.m_pFile = pFile;
    e.m_Tex = tex;
    ResolveHashCodes(e);
    m_ChangeNum++;
}

void EXTexSwapInfo::SetSwap(unsigned int src, EXTexture *pTex)
{
    TEXSWAP_FIND_OR_ADD(o.m_pFile == (EXGeoFile *)pTex)
    EXTexSwapEntry &e = m_Swaps[i];
    e.m_Src = src;
    e.m_FileHash = 0xffffffff;
    e.m_pFile = (EXGeoFile *)pTex;
    m_ChangeNum++;
}

void EXTexSwapInfo::SetCatchAllSwap(unsigned int file, unsigned int tex)
{
    if (!m_Swaps.Size() || m_Swaps[0].m_Src != 0xfffffffe) {
        EXTexSwapEntry n;
        n.m_Src = 0xfffffffe;
        m_Swaps.InsertAt(0, n, 1);
    }
    EXTexSwapEntry &e = m_Swaps[0];
    e.m_Src = 0xfffffffe;
    e.m_FileHash = file;
    e.m_pFile = 0;
    e.m_Tex = tex;
    ResolveHashCodes(e);
    m_ChangeNum++;
}

void EXTexSwapInfo::SetCatchAllSwap(EXGeoFile *pFile, unsigned int tex)
{
    if (!m_Swaps.Size() || m_Swaps[0].m_Src != 0xfffffffe) {
        EXTexSwapEntry n;
        n.m_Src = 0xfffffffe;
        m_Swaps.InsertAt(0, n, 1);
    }
    EXTexSwapEntry &e = m_Swaps[0];
    e.m_Src = 0xfffffffe;
    e.m_FileHash = 0;
    e.m_pFile = pFile;
    e.m_Tex = tex;
    ResolveHashCodes(e);
    m_ChangeNum++;
}

void EXTexSwapInfo::SetCatchAllSwap(EXTexture *pTex)
{
    if (!m_Swaps.Size() || m_Swaps[0].m_Src != 0xfffffffe) {
        EXTexSwapEntry n;
        n.m_Src = 0xfffffffe;
        m_Swaps.InsertAt(0, n, 1);
    }
    EXTexSwapEntry &e = m_Swaps[0];
    e.m_Src = 0xfffffffe;
    e.m_FileHash = 0xffffffff;
    e.m_pFile = (EXGeoFile *)pTex;
    m_ChangeNum++;
}

void EXTexSwapInfo::StopAllSwaps()
{
    m_Swaps.RemoveAll();
    m_ChangeNum++;
}

void EXBaseRenderEnv::SetRenderInfo(const EXRenderInfo &info)
{
    EXRenderInfo::Merge(ms_RenderInfo, ms_GRenderInfo, info);
    if (ms_OverrideFogInfo.m_enabled) {
        ms_RenderFogInfo.m_color = ms_OverrideFogInfo.m_color;
        ms_RenderFogInfo.m_near = ms_OverrideFogInfo.m_near;
        ms_RenderFogInfo.m_far = ms_OverrideFogInfo.m_far;
        ms_RenderFogInfo.m_min = ms_OverrideFogInfo.m_min;
        ms_RenderFogInfo.m_max = ms_OverrideFogInfo.m_max;
        ms_RenderFogInfo.m_enabled = 1;
    } else {
        ms_RenderFogInfo.m_enabled = 0;
    }
    if (ms_RenderInfo.m_renderFlags & 2) {
        float d = (float)(s16)ms_RenderInfo.m_shadowDensity * ms_RenderInfo.m_rgbaMult.w;
        if (d >= 32767.0f) d = 32767.0f;
        else if (0.0f >= d) d = 0.0f;
        ms_RenderInfo.m_shadowDensity = (short)d;
    }
    if (ms_GRenderInfo.m_renderFlags & 4) {
        const ex::RGBA &c = ms_RenderFogInfo.m_color;
        EXVector v = ms_GRenderInfo.m_rgbaMult * EXVector((float)c.m_R, (float)c.m_G, (float)c.m_B, (float)c.m_A);
        ms_RenderFogInfo.m_color = EXVectorToRGBA(v);
    }
}

EXTexSwapInfo::EXTexSwapInfo() { m_ChangeNum = 0; }
