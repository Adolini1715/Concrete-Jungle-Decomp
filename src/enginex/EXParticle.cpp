#include "EXParticleSysMgr.h"
#include "EXParticleSys.h"
#include "EXEmitter.h"
#include "EXGeoParticle.h"
#include "EXGeoFile.h"
#include "EXBaseDisplay.h"
#include "XBGeoDrawInfo.h"
#include "EXMemory.h"
#include "EXItemEnv.h"
#include "EXRand.h"
#include <math.h>
#include "EXBaseApp.h"
#include "EXParticle.h"
#include "EXItemAnimator_Map.h"
#include "EXGeoEntity.h"
#include "EXDisplay.h"
#include "EXGeoHeader.h"
#include "EXItemRender_Entity.h"
#include <string.h>

EX_IMPLEMENT_DYNCREATE_ROOT(EXBaseParticleSys)

void EXParticleSysMgr::SetParticleManagerFade(const EXVector &fade)
{
    m_bFade = 1;
    m_fade = fade;
}

void EXParticleSysMgr::StopAllSystems(s8 stop)
{
    m_bStop = stop;
    m_bPause = stop;
}

void EXParticleSysMgr::PauseAllSystems(s8 pause)
{
    m_bPause = pause;
}

EXBaseParticleSys::EXBaseParticleSys()
{
    m_bEnabled = 1;
    m_pPosCallback = 0;
    m_pEmitter = 0;
    m_pParticles = 0;
    m_pGDiArrays = 0;
    m_258 = 0;
    m_bVisible = 1;
}

u32 EXBaseParticleSys::GetHashCode() const
{
    return m_geoRef.HashRef();
}

void EXBaseParticleSys::SetParticleFade(const EXVector &fade)
{
    m_bFade = 1;
    m_fade = fade;
}

void EXBaseParticleSys::Release()
{
    if (m_pEmitter) {
        m_pEmitter->Release();
        delete m_pEmitter;
        m_pEmitter = 0;
    }
    m_pMgr->RemoveSystem(this);
    delete this;
}

// NEAR: xmm register allocation in the interpolation (d lands in xmm2, retail xmm3).
void EXBaseParticleSys::CalculateMipSettings()
{
    if (!m_bUseMips) return;
    float d = EXItemEnv::GetItemEnv()->MinDistanceToItemViews(m_lastEmitPos);
    const CParticleMip &nearMip = GetParticleMip(ParticleMip::NEAR_MIP);
    if (nearMip.GetDistance() >= d) {
        m_curEmitsPerFrame = nearMip.GetEmitsPerFrame();
        m_curParticleLimit = nearMip.GetParticleLimit();
        return;
    }
    const CParticleMip &farMip = GetParticleMip(ParticleMip::FAR_MIP);
    if (d >= farMip.GetDistance()) {
        m_curEmitsPerFrame = farMip.GetEmitsPerFrame();
        m_curParticleLimit = farMip.GetParticleLimit();
        return;
    }
    float range = farMip.GetDistance() - nearMip.GetDistance();
    float e = nearMip.GetEmitsPerFrame();
    float inv = 1.0f / range;
    m_curEmitsPerFrame = e - (e - farMip.GetEmitsPerFrame()) * inv * d;
    if (0.0f > m_curEmitsPerFrame) m_curEmitsPerFrame = 0.0f;
    m_curParticleLimit = (int)((float)nearMip.GetParticleLimit() -
                               (float)(nearMip.GetParticleLimit() - farMip.GetParticleLimit()) * inv * d);
    if (m_curParticleLimit < 0) m_curParticleLimit = 0;
}

EXRuntimeClass *EXParticleSysMgr::FindCustomSystem(u32 id)
{
    for (u32 i = 0; i < (u32)m_custom.m_nSize; i++) {
        if (m_custom[i].m_id == id) return m_custom[i].m_pClass;
    }
    return 0;
}

EXParticleSys *EXParticleSysMgr::ParticleSys(const EXItemAnimator *pAnim) const
{
    for (u32 i = 0; i < (u32)m_systems.m_nSize; i++) {
        const EXParticleSysData &d = m_systems[i];
        if (d.m_pAnim == pAnim) return d.m_pSys;
    }
    return 0;
}

EXParticleSys *EXParticleSysMgr::ParticleSys(const EXItemAnimator *pAnim, u32 hash) const
{
    for (u32 i = 0; i < (u32)m_systems.m_nSize; i++) {
        const EXParticleSysData &d = m_systems[i];
        if (d.m_pAnim == pAnim && d.m_pSys->m_geoRef.HashRef() == hash) return d.m_pSys;
    }
    return 0;
}

s8 EXParticleSysMgr::DetatchSystem(const EXEffectsSys *pSys)
{
    int n = 0;
    for (u32 i = 0; i < (u32)m_systems.m_nSize; i++) {
        EXParticleSysData &d = m_systems[i];
        if ((EXEffectsSys *)d.m_pSys == pSys) {
            d.m_pAnim = 0;
            n++;
        }
    }
    if (n) return 1;
    return 0;
}

s8 EXParticleSysMgr::DeleteSystem(const EXEffectsSys *pSys)
{
    for (u32 i = 0; i < (u32)m_systems.m_nSize; i++) {
        EXParticleSysData &d = m_systems[i];
        if ((EXEffectsSys *)d.m_pSys == pSys) {
            delete d.m_pSys;
            m_systems.RemoveAt(i);
            return 1;
        }
    }
    return 0;
}

void EXParticleSysMgr::DeleteAllSystems()
{
    for (u32 i = 0; i < (u32)m_systems.m_nSize; i++) {
        delete m_systems[i].m_pSys;
    }
    m_systems.Empty(1);
}

void EXParticleSysMgr::RemoveSystem(const EXEffectsSys *pSys)
{
    for (u32 i = 0; i < (u32)m_systems.m_nSize; i++) {
        EXParticleSysData &d = m_systems[i];
        if ((EXEffectsSys *)d.m_pSys == pSys) {
            d.m_pSys = 0;
            return;
        }
    }
}

// NEAR: retail tests the two render-flag bits separately (word load, shr/test); ours folds them into test al,6.
void EXBaseParticleSys::UpdateRenderInfo(const EXRenderInfo &info)
{
    m_bFade = info.RGBMultEnabled() || info.AlphaMultEnabled();
    m_fade = info.RGBAMult();
    memcpy(&m_renderInfo, &info, sizeof(EXRenderInfo));
    m_renderInfo.SetExtRenderInfo(0);
}

EXParticleSysMgr::~EXParticleSysMgr()
{
    DeleteAllSystems();
}

void EXParticleSysMgr::RegisterCustomSystem(u32 id, EXRuntimeClass *pClass)
{
    for (u32 i = 0; i < (u32)m_custom.m_nSize; i++) {
        if (m_custom.m_pData[i].m_id == id) {
            if (pClass) m_custom.m_pData[i].m_pClass = pClass;
            else m_custom.RemoveAt(i);
            return;
        }
    }
    EXCustomPSysData &d = m_custom.AddSpace(1);
    d.m_id = id;
    d.m_pClass = pClass;
}

// NEAR: retail inlines AddSpace + GrowArray here (see EXSwoosh.cpp CreateSystem note).
EXParticleSys *EXParticleSysMgr::CreateSystem(u32 id, const EXRuntimeClass *pClass, EXItemAnimator *pAnim)
{
    EXParticleSys *pSys = ParticleSys(pAnim, id);
    if (pSys) return pSys;
    EXParticleSysData &d = m_systems.AddSpace(1);
    d.m_pAnim = pAnim;
    if (!pClass) pClass = FindCustomSystem(id);
    if (pClass) d.m_pSys = (EXParticleSys *)pClass->m_pfnCreateObject();
    else d.m_pSys = new EXParticleSys;
    d.m_pSys->SetParticleSysMgr(this);
    return d.m_pSys;
}

EXBaseParticleSys::~EXBaseParticleSys()
{
    if (m_pEmitter) {
        m_pEmitter->Release();
        delete m_pEmitter;
        m_pEmitter = 0;
    }
    if (m_pGDiArrays) {
        delete[] m_pGDiArrays;
        m_pGDiArrays = 0;
    }
    if (m_pParticles) {
        EXFree(m_pParticles);
        m_pParticles = 0;
    }
}

EXParticleSysMgr::EXParticleSysMgr()
{
    m_systems.SetSize(0, 0x28);
    m_custom.SetSize(0, 0x14);
    m_bFade = 0;
    m_bStop = 0;
    m_bPause = 0;
    m_34 = 1;
    m_fade.set(1.0f, 1.0f, 1.0f, 1.0f);
}

// NEAR: block layout of the failure path and per-branch reloads of pAnim differ.
s8 EXBaseParticleSys::Update(EXItemAnimator *pAnim)
{
    m_pViewToScreenCoords = &EXBaseDisplay::ViewToScreenCoords();
    m_pWorldToView = &EXBaseDisplay::WorldToView();
    EXGeoHeader *pHdr = m_geoRef.GeoFile()->GeoHeader();
    if (pHdr) {
        m_pGeo = pHdr->GeoParticle(m_geoHash);
        if (m_pGeo) {
            s8 r;
            if (m_bUseEmitter == 1) r = m_pEmitter->Update(this, pAnim, m_pGeo);
            else r = Update(pAnim, m_pGeo);
            if (r == 1) return r;
            if (!pAnim) Release();
            return 0;
        }
    }
    if (!pAnim) Release();
    return 0;
}

EXParticle *EXBaseParticleSys::CreateParticle(const EXVector &pos, const EXMatrix &mtx)
{
    return BaseCreateParticle(pos, mtx);
}

EXParticle *EXBaseParticleSys::GetRandomParticle()
{
    if (!m_numActive) return 0;
    EXParticle *p = 0;
    if (m_numActive > m_maxParticles >> 1) {
        unsigned int i = EXRandFast(m_maxParticles);
        for (int n = 0; n < m_maxParticles; n++) {
            p = &m_pParticles[i];
            if (p->IsActive()) return p;
            if ((int)++i >= m_maxParticles) i = 0;
        }
        return 0;
    }
    int tries = 3;
    do {
        unsigned int i = EXRandFast(m_numActive);
        if (m_pParticles[i].IsActive()) return &m_pParticles[i];
    } while (tries--);
    return p;
}

// The i-th particle: slot i when it is live, otherwise the i-th live one.
EXParticle *EXBaseParticleSys::GetParticle(u32 i)
{
    if (i >= (u32)m_numActive) return 0;
    EXParticle *p = &m_pParticles[i];
    if (p->IsActive()) return p;
    u32 n = 0;
    p = m_pParticles;
    for (int j = 0; j < m_maxParticles; j++, p++) {
        if (p->IsActive() && ++n == i) return p;
    }
    return 0;
}

// Up to n live particles: all of them in slot order when n covers them, otherwise round robin.
u32 EXBaseParticleSys::GetParticles(u32 n, EXParticle **pOut)
{
    int j = 0;
    if (n >= (u32)m_numActive) {
        u32 got = 0;
        EXParticle *p = m_pParticles;
        for (; j < m_maxParticles; j++, p++) {
            if (p->IsActive()) {
                pOut[got++] = p;
                if (got == n) break;
            }
        }
        return got;
    }
    u32 got = 0;
    for (u32 k = 0; k < n; k++) {
        for (;;) {
            EXParticle *p = &m_pParticles[m_nextParticle++];
            if (p->IsActive()) {
                pOut[got++] = p;
                if (m_nextParticle >= m_maxParticles) m_nextParticle = 0;
                break;
            }
            if (m_nextParticle >= m_maxParticles) m_nextParticle = 0;
            if (++j >= m_maxParticles) return got;
        }
        if (j >= m_maxParticles) return got;
    }
    return n;
}

EXParticle *EXBaseParticleSys::GetFreeParticle()
{
    if (m_bAllUsed == 1) {
        if (m_systemFlags & 1) return 0;
        EXParticle *p = m_pParticles;
        for (int i = m_maxParticles - 1; i >= 0; i--, p++)
            if (!p->IsActive()) return p;
        return 0;
    }
    if (m_nextNew >= m_maxParticles) return 0;
    EXParticle *p = &m_pParticles[m_nextNew];
    if (p->IsActive()) return 0;
    if (++m_nextNew >= m_maxParticles) m_bAllUsed = 1;
    return p;
}

void EXBaseParticleSys::BaseSetupEmitterColours(EXParticle *p, const EXGeoParticle *pGeo, float t) const
{
    s8 done = 0;
    for (const EXGeoParticleKey *k = pGeo->m_keys; k->m_type != -1; k++) {
        if (done || k->m_time > t) break;
        switch (k->m_type) {
        case 6:
            p->m_5c.x = k->m_scale * t + k->m_base;
            p->m_7c.x = 0.0f;
            break;
        case 7:
            p->m_5c.y = k->m_scale * t + k->m_base;
            p->m_7c.y = 0.0f;
            break;
        case 8:
            p->m_5c.z = k->m_scale * t + k->m_base;
            p->m_7c.z = 0.0f;
            break;
        case 9:
            p->m_5c.w = k->m_scale * t + k->m_base;
            p->m_7c.w = 0.0f;
            done = 1;
            break;
        }
    }
}

void EXBaseParticleSys::BaseUpdateEmitterColours(EXParticle *p, const EXGeoParticle *pGeo, float t) const
{
    for (const EXGeoParticleKey *k = pGeo->m_keys; k->m_type != -1; k++) {
        if (k->m_time >= t) break;
        switch (k->m_type) {
        case 6:
            p->m_5c.x = k->m_scale * t + k->m_base;
            p->m_7c.x = 0.0f;
            break;
        case 7:
            p->m_5c.y = k->m_scale * t + k->m_base;
            p->m_7c.y = 0.0f;
            break;
        case 8:
            p->m_5c.z = k->m_scale * t + k->m_base;
            p->m_7c.z = 0.0f;
            break;
        case 9:
            p->m_5c.w = k->m_scale * t + k->m_base;
            p->m_7c.w = 0.0f;
            break;
        }
    }
}

float EXBaseParticleSys::BaseUpdateEmitterLife(const EXGeoParticle *pGeo)
{
    float life = -1.0f;
    for (const EXGeoParticleKey *k = pGeo->m_keys; k->m_type != -1; k++) {
        if (k->m_time >= m_emitterTime) break;
        switch (k->m_type) {
        case 0xa:
            if (m_systemFlags & 0x200) m_emitterLife = k->m_scale * m_emitterTime + k->m_base;
            break;
        case 0xb:
            if (m_systemFlags & 0x100) life = k->m_scale * m_emitterTime + k->m_base;
            break;
        }
    }
    return life;
}

// Emitter matrix and position from the animator (unless a position callback drives the system).
void EXBaseParticleSys::CalculateEmitterPosition(const EXItemAnimator *pAnim)
{
    if (!pAnim || m_pPosCallback) return;
    if (m_geoRefEmitter.GeoFile()) {
        EXGeoEntityHeader *h = m_geoRefEmitter.GeoEntityHeader();
        if (!h->Object(0)) return;
    }
    m_1f8 = pAnim->m_mtx;
    m_lastEmitPos = m_1f8.r[3];
}

// Fog of the zone the emitter is in (fogged systems only).
void EXBaseParticleSys::CalculateFogSettings(EXMapOn mapOn)
{
    m_258 = 0;
    if (!(m_systemFlags & 0x80)) return;
    EXItemAnimator_Map *pMap = EXItemEnv::GetItemEnv()->GetMapItem((s8)(mapOn.m_v >> 8));
    if (!pMap) return;
    EXMapZoneInfo *pZone = pMap->MapZoneInfo(mapOn.Zone());
    if (!pZone) return;
    const EXGeoRenderEnv &r = pZone->DrawRender();
    m_fogNear = r.FogNear();
    m_fogFar = r.FogFar();
    m_fogStart = r.FogMin();
    m_fogEnd = r.FogMax();
    m_fogColour = r.FogColor();
    m_258 = r.FogEnabled();
}

s8 EXBaseParticleSys::CheckMapVisibility(const EXItemAnimator *pAnim)
{
    if (m_pPosCallback) return 1;
    CalculateEmitterPosition(pAnim);
    if (m_systemFlags & 0x2000) return 1;
    EXMapOn m = EXItemEnv::GetItemEnv()->GetMapOn(m_lastEmitPos);
    if (m.m_v == -1) return 1;
    if (EXItemEnv::GetItemEnv()->IsZoneVisible(m) > 0) {
        CalculateFogSettings(m);
        return 1;
    }
    return 0;
}

// Particle key channels: 0..2 rotation (value + initial angle, wrapped at 2 pi; slope is the spin),
// 3..5 scale (times the system scale, with optional random variation), 6..9 colour r, g, b, a.
#define PARTICLE_KEY_ROT(n, c)                                  \
    case n: {                                                   \
        float a = p->m_6c.c + base;                             \
        if (a > 6.2831855f) a -= 6.2831855f;                    \
        p->m_4c.c = a;                                          \
        p->m_9c.c = rate;                                       \
        break;                                                  \
    }
#define PARTICLE_KEY_SCALE(n, c)                                                \
    case n:                                                                     \
        if (!(m_systemFlags & 0x40)) var = ((float)(int)(EXRandFast32() >> 1) * (1.0f / 1073741824.0f) - 1.0f) * pGeo->m_scaleVar.c; \
        p->m_scale.c = (base + var) * m_globalScale.c;                          \
        p->m_8c.c = m_globalScale.c * rate;                                     \
        var = 0.0f;                                                             \
        break;
#define PARTICLE_KEY_COLOUR(n, c)   \
    case n:                         \
        p->m_5c.c = base;           \
        p->m_7c.c = rate;           \
        break;

void EXBaseParticleSys::BaseUpdateParticleLifetimeNoColour(EXParticle *p, const EXGeoParticle *pGeo) const
{
    float var = 0.0f;
    for (const EXGeoParticleKey *k = &pGeo->m_keys[p->m_08]; k->m_type != -1; k = &pGeo->m_keys[++p->m_08]) {
        if (k->m_time > p->m_time) return;
        float base = k->m_base;
        float rate = k->m_scale * p->m_life;
        switch (k->m_type) {
        PARTICLE_KEY_ROT(0, x)
        PARTICLE_KEY_ROT(1, y)
        PARTICLE_KEY_ROT(2, z)
        PARTICLE_KEY_SCALE(3, x)
        PARTICLE_KEY_SCALE(4, y)
        PARTICLE_KEY_SCALE(5, z)
        case 6:
        case 7:
        case 8:
        case 9:
            rate = base;
            break;
        }
    }
}

void EXBaseParticleSys::BaseUpdateParticleLifetime(EXParticle *p, const EXGeoParticle *pGeo) const
{
    float var = 0.0f;
    for (const EXGeoParticleKey *k = &pGeo->m_keys[p->m_08]; k->m_type != -1; k = &pGeo->m_keys[++p->m_08]) {
        if (k->m_time > p->m_time) return;
        float base = k->m_base;
        float rate = k->m_scale * p->m_life;
        switch (k->m_type) {
        PARTICLE_KEY_ROT(0, x)
        PARTICLE_KEY_ROT(1, y)
        PARTICLE_KEY_ROT(2, z)
        PARTICLE_KEY_SCALE(3, x)
        PARTICLE_KEY_SCALE(4, y)
        PARTICLE_KEY_SCALE(5, z)
        PARTICLE_KEY_COLOUR(6, x)
        PARTICLE_KEY_COLOUR(7, y)
        PARTICLE_KEY_COLOUR(8, z)
        PARTICLE_KEY_COLOUR(9, w)
        }
    }
}

void EXBaseParticleSys::UpdateParticleLifetime(EXParticle *p, const EXGeoParticle *pGeo) const
{
    if (m_colourMode == 1) BaseUpdateParticleLifetimeNoColour(p, pGeo);
    else BaseUpdateParticleLifetime(p, pGeo);
}

static EXGeoEntity_InfoArray s_EmitInfo;  // entity pieces the vertex emitters pick from

// Emits n particles from random vertices of the gathered entity pieces (in batches of up to 32).
void EXBaseParticleSys::EmitParticles(const EXMatrix &mtx, u32 n, u32 flags)
{
    EXFixedArray<EXMaths::vec_xyzw, 66> verts;
    u32 avail = 0;
    u32 idx = 0;
    u32 room = m_curParticleLimit - m_numActive;
    if ((int)room <= 0) return;
    for (u32 i = 0; i < n; i++) {
        if (!avail) {
            u32 c = min((u32)32, room);
            avail = min(c, n);
            s_EmitInfo.GetRandomVertexArray((EXVector *)&verts[0], avail, flags);
            if (avail) idx %= avail;
        }
        EXParticle *p;
        if (m_bEmitWithDir) {
            EXVector pos(verts[idx]);
            EXVector dir(verts[idx + 1]);
            idx += 14;
            if (avail) idx %= avail * 2;
            p = CreateParticleWithDir(pos, dir, mtx);
        } else {
            EXVector pos(verts[idx]);
            p = CreateParticle(pos, mtx);
            idx += 7;
            if (avail) idx %= avail;
        }
        avail--;
        if (!p) return;
    }
}

// Off-screen culling of the system's bounds at the emitter (systems with flag 0x4000 only).
s8 EXBaseParticleSys::CheckVisibility()
{
    if (!(m_systemFlags & 0x4000)) return 1;
    if (!m_geoRef.GeoFile() || !m_geoRef.GeoFile()->GeoHeader()) return 0;
    EXGeoParticle *pGeo = m_geoRef.GeoFile()->GeoHeader()->GeoParticle(m_geoHash);
    if (!pGeo) return 0;
    EXBoundsBox box = pGeo->BoundsBox();
    EXMaths::mat_44 m = m_1f8;
    m.r[3].set(m_lastEmitPos.x, m_lastEmitPos.y, m_lastEmitPos.z, 1.0f);
    box._ApplyXForm(m);
    EXBaseDisplay::SetObjectToWorld(EXMatrix::MakeIdentity());
    EXBaseDisplay::GetDisplay()->UpdateTransforms();
    return EXDisplay::CullBox((const float *)&box, EXBaseDisplay::ObjectToCull()) != 0;
}

s8 EXBaseParticleSys::Initialize(EXGeoFile *pFile, u32 hash)
{
    m_geoRef.m_hashRef = hash;
    m_geoRef.m_pGeoFile = pFile;
    m_geoRef.m_08 = -1;
    if (!pFile || !pFile->GeoHeader()) return 0;
    EXGeoHeader *pHdr = pFile->GeoHeader();
    m_geoHash = pHdr->ParticleList().FindIndex(m_geoRef.HashRef());
    int i = pHdr->ParticleList().FindIndex(m_geoHash);
    EXGeoParticleHeader *h = i < 0 ? 0 : &pHdr->ParticleList().Data()[i];
    EXGeoParticle *pGeo = h->Object(0);
    if (!pGeo) return 0;
    m_bUseEmitter = 0;
    switch (pGeo->m_EmitterType) {
    case 0:
        break;
    case 1:
        m_bUseEmitter = 1;
        m_emitterType = 1;
        break;
    case 2:
        break;
    case 3:
        m_emitterType = 3;
        m_bUseEmitter = 1;
        break;
    case 4:
        break;
    case 5:
        m_emitterType = 4;
        m_bUseEmitter = 1;
        break;
    }
    if (!pGeo->m_04) return 0;
    m_geoRefEmitter.m_08 = -1;
    if (pGeo->m_EmitterHash) {
        m_geoRefEmitter.m_pGeoFile = pFile;
        m_geoRefEmitter.m_hashRef = pGeo->m_EmitterHash;
    } else {
        m_geoRefEmitter.m_pGeoFile = 0;
        m_geoRefEmitter.m_hashRef = 0;
    }
    m_pGeo = pGeo;
    m_pViewToScreenCoords = 0;
    m_pWorldToView = 0;
    m_1ef = 0;
    return CreateEXEmitter(pGeo);
}

void EXParticleSysMgr::Update()
{
    if (m_bPause == 1) return;
    for (int i = 0; i < m_systems.Size(); i++) {
        EXParticleSysData &d = m_systems.m_pData[i];
        s8 vis = 1;
        if (d.m_pAnim) {
            if (!d.m_pAnim->HasMtx()) vis = 0;
            if ((d.m_pAnim->m_itemFlags == 0) & 1) vis = 0;
        }
        if (d.m_pSys) {
            if (((EXBaseParticleSys *)d.m_pSys)->m_bFirstUpdate == 1) {
                ((EXBaseParticleSys *)d.m_pSys)->m_frameScale = 1.0f;
                for (u32 j = 0; j < (u32)((EXBaseParticleSys *)d.m_pSys)->m_pGeo->m_preUpdates; j++) {
                    d.m_pSys->Update(d.m_pAnim);
                    if (!d.m_pSys) {
                        m_systems.RemoveAt(i);
                        i--;
                        goto next;
                    }
                }
                ((EXBaseParticleSys *)d.m_pSys)->m_bFirstUpdate = 0;
            }
            ((EXBaseParticleSys *)d.m_pSys)->m_frameScale = EXBaseApp::GameRenderFrameRateMul();
            if (vis == 1) {
                if (m_34 == 1) {
                    if (((EXBaseParticleSys *)d.m_pSys)->CheckMapVisibility(d.m_pAnim) != 1 && !(((EXBaseParticleSys *)d.m_pSys)->m_systemFlags & 0x2000)) {
                        ((EXBaseParticleSys *)d.m_pSys)->Visible(0);
                        d.m_pSys->Update(d.m_pAnim);
                        goto check;
                    }
                    ((EXBaseParticleSys *)d.m_pSys)->m_bVisible = 1;
                    if (m_bFade) ((EXBaseParticleSys *)d.m_pSys)->SetParticleFade(m_fade);
                } else {
                    ((EXBaseParticleSys *)d.m_pSys)->m_bVisible = 1;
                    if (m_bFade) ((EXBaseParticleSys *)d.m_pSys)->SetParticleFade(m_fade);
                    ((EXBaseParticleSys *)d.m_pSys)->CalculateEmitterPosition(d.m_pAnim);
                }
                d.m_pSys->Update(d.m_pAnim);
            check:
                if (!d.m_pSys) {
                    m_systems.RemoveAt(i);
                    i--;
                }
            } else {
                ((EXBaseParticleSys *)d.m_pSys)->m_bVisible = 0;
            }
        } else {
            m_systems.RemoveAt(i);
            i--;
        }
    next:;
    }
}

void EXParticleSysMgr::Draw(int pass)
{
    EXBaseDisplay::SetObjectToWorld(EXMatrix::MakeIdentity());
    EXBaseDisplay::GetDisplay()->UpdateTransforms();
    if (m_bStop == 1) return;
    for (u32 i = 0; i < (u32)m_systems.Size(); i++) {
        EXParticleSysData &d = m_systems.m_pData[i];
        EXBaseParticleSys *p = (EXBaseParticleSys *)d.m_pSys;
        if (p->IsVisible() && p->IsEnabled() && p->CheckVisibility() == 1) ((EXBaseParticleSys *)d.m_pSys)->Draw(pass);
    }
}

void EXBaseParticleSys::CreateParticles(const EXItemAnimator *pAnim, const EXGeoParticle *pGeo)
{
    int avail = m_maxParticles - m_numActive;
    if (avail <= 0) return;
    float f = m_frameScale * m_curEmitsPerFrame + m_emitAccum;
    m_emitAccum = f;
    u32 n = (int)f;
    if (!n) return;
    EXParticlePosCallback cb = m_pPosCallback;
    if (!cb) {
        if (!m_geoRefEmitter.GeoFile()) {
            EXMatrix &m = (EXMatrix &)m_1f8;
            m.GetRotationXYZ(m_10c);
            m.SetTranslation(EXVector(0.0f, 0.0f, 0.0f, 0.0f));
            for (u32 i = 0; i < n; i++)
                if (!CreateParticle(m_lastEmitPos, m)) break;
        } else {
            EXGeoEntityHeader *h = m_geoRefEmitter.GeoEntityHeader();
            if (h) {
                EXGeoEntity *e = h->Object(0);
                if (e) {
                    EXMatrix &m = (EXMatrix &)m_1f8;
                    m.GetRotationXYZ(m_10c);
                    s_EmitInfo.Empty(0);
                    s_EmitInfo.SetMatrix(m);
                    e->BuildEntityInfoArray(s_EmitInfo, 0);
                    m.SetTranslation(EXVector(0.0f, 0.0f, 0.0f, 0.0f));
                    u32 flags = 0;
                    if (!*(const u32 *)&pAnim->m_scale.x) flags = 1;  // TODO: animator fields at +0xe8 / +0xec
                    if (*(const u16 *)&pAnim->m_scale.y) flags |= 2;
                    EmitParticles(m, n, flags);
                }
            }
        }
    } else {
        EXFixedArray<EXMaths::vec_xyzw, 64> buf;
        u32 k = 0;
        if (pAnim) {
            m_1f8 = pAnim->m_mtx;
            m_lastEmitPos = m_1f8.r[3];
        }
        if (m_emitterRate * 4.0f > m_emitterTime) {
            cb((EXItemAnimator *)pAnim, (EXVector *)&buf[0], 8);
        } else {
            u32 cnt = 0;
            for (u32 i = 0; i < n; i++) {
                if (!cnt) {
                    u32 c = min((u32)64, (u32)avail);
                    cnt = min(c, n);
                    if (!cb((EXItemAnimator *)pAnim, (EXVector *)&buf[0], cnt)) n = 0;
                    m_lastEmitPos = buf[0];
                    if (cnt) k %= cnt;
                }
                k &= 0x3f;
                EXVector pos = buf[k];
                if (!CreateParticle(pos, EXMatrix::MakeIdentity())) break;
                k += 7;
                if (cnt) k %= cnt;
            }
        }
    }
    if (n) m_emitAccum = 0.0f;
}

inline void EXBaseParticleSys::CalculateInitialColour(EXParticle *p)
{
    const EXGeoParticle *pGeo = m_pGeo;
    p->m_5c.set_zero();
    if (m_colourMode == 1) {
        float t = 0.0f;
        if (m_colourVar == 1) {
            if (m_gradientTime > 100.0f) {
                m_gradientDir = -1;
                m_gradientTime = 100.0f;
                t = 100.0f;
            } else {
                if (m_gradientTime < 0.0f) {
                    m_gradientDir = 1;
                    m_gradientTime = 0.0f;
                }
                t = m_gradientTime;
            }
        } else if (m_colourVar == 0) {
            m_gradientDir = 1;
            if (m_gradientTime > 100.0f) m_gradientTime -= 100.0f;
            t = m_gradientTime;
        } else if (m_colourVar == 2) {
            m_gradientDir = 1;
            t = EXRandFastf(100.0f);
            if (t < 0.0f) t = 0.0f;
            else if (t > 100.0f) t = 100.0f;
        }
        BaseSetupEmitterColours(p, pGeo, t);
        if (m_systemFlags & 0x300) {
            float life = BaseUpdateEmitterLife(pGeo);
            if (life != -1.0f) p->m_time = life;
        }
        BaseUpdateParticleLifetimeNoColour(p, pGeo);
    } else {
        if (m_systemFlags & 0x300) {
            float life = BaseUpdateEmitterLife(pGeo);
            if (life != -1.0f) p->m_time = life;
        }
        BaseUpdateParticleLifetime(p, pGeo);
    }
    EXMaths::vec_xyzw c = p->m_5c;
    float r = limit(c.x, 0.0f, 255.0f);
    float g = limit(c.y, 0.0f, 255.0f);
    float b = limit(c.z, 0.0f, 255.0f);
    float a = limit(c.w, 0.0f, 255.0f);
    *(u32 *)&p->m_colour = ((((((u32)(u8)a << 8) | (u8)b) << 8) | (u8)g) << 8) | (u8)r;
}

inline void EXBaseParticleSys::CalculateStandardParticleSettings(EXParticle *p)
{
    p->m_scale = CalculateInitialSize();
    const EXGeoParticle *g = m_pGeo;
    float life = ((float)(int)(EXRandFast32() >> 1) * (1.0f / 1073741824.0f) - 1.0f) * g->m_lifeVar + g->m_life;
    if (life <= 0.0f) life = g->m_life;
    p->m_life = 100.0f / life * m_rfps;
    CalculateInitialColour(p);
    if (m_1cc) p->m_texture = g->m_entities[0];
    else p->m_texture = CalculateInitialTexture();
}

EXParticle *EXBaseParticleSys::BaseCreateParticle(const EXVector &pos, const EXMatrix &mtx)
{
    EXParticle *p = GetFreeParticle();
    if (!p) return 0;
    const EXGeoParticle *g = m_pGeo;
    p->SetDefaultValues();
    float rx = ((float)(int)(EXRandFast32() >> 1) * (1.0f / 1073741824.0f) - 1.0f);
    float ry = ((float)(int)(EXRandFast32() >> 1) * (1.0f / 1073741824.0f) - 1.0f);
    float rz = ((float)(int)(EXRandFast32() >> 1) * (1.0f / 1073741824.0f) - 1.0f);
    EXVector v(g->m_emitSpread.x * rx, g->m_emitSpread.y * ry, g->m_emitSpread.z * rz, 0.0f);
    v = EXVector(g->m_emitOffset.x + v.x, g->m_emitOffset.y + v.y, g->m_emitOffset.z + v.z, 0.0f);
    if (m_bLocalSpace != 1) (EXMaths::vec_xyzw &)v = (const EXMaths::mat_44 &)mtx * v;
    p->m_pos = v * m_globalScale;
    float yaw = m_yaw;
    float pitch = m_pitch;
    float speed = m_speed;
    if (m_yawVar != 0.0f) yaw = ((float)(int)(EXRandFast32() >> 1) * (1.0f / 1073741824.0f) - 1.0f) * m_yawVar + yaw;
    if (m_pitchVar != 0.0f) pitch = ((float)(int)(EXRandFast32() >> 1) * (1.0f / 1073741824.0f) - 1.0f) * m_pitchVar + pitch;
    float sp = (float)sin(pitch);
    EXVector vel((float)cos(yaw) * sp, (float)cos(pitch), (float)sin(yaw) * sp, 1.0f);
    if (m_speedVar != 0.0f) speed = ((float)(int)(EXRandFast32() >> 1) * (1.0f / 1073741824.0f) - 1.0f) * m_speedVar + speed;
    vel *= m_emitterLife + speed;
    if (m_bAddEmitterVel) vel += m_emitterVel;
    p->m_1c = vel * m_rfps;
    if (m_bLocalSpace != 1) p->m_pos += pos;
    (EXMaths::vec_xyzw &)p->m_1c = (const EXMaths::mat_44 &)mtx * p->m_1c;
    if (m_bAddEmitterVel) p->m_1c += m_emitterVel * m_rfps;
    CalculateInitialRotation(p, vel);
    CalculateStandardParticleSettings(p);
    return p;
}

EXParticle *EXBaseParticleSys::CreateParticleWithDir(const EXVector &pos, const EXVector &dir, const EXMatrix &mtx)
{
    EXParticle *p = GetFreeParticle();
    if (!p) return 0;
    p->SetDefaultValues();
    float rx = ((float)(int)(EXRandFast32() >> 1) * (1.0f / 1073741824.0f) - 1.0f);
    float ry = ((float)(int)(EXRandFast32() >> 1) * (1.0f / 1073741824.0f) - 1.0f);
    float rz = ((float)(int)(EXRandFast32() >> 1) * (1.0f / 1073741824.0f) - 1.0f);
    const EXGeoParticle *g = m_pGeo;
    EXVector v(g->m_emitSpread.x * rx, g->m_emitSpread.y * ry, g->m_emitSpread.z * rz, 0.0f);
    v = EXVector(g->m_emitOffset.x + v.x, g->m_emitOffset.y + v.y, g->m_emitOffset.z + v.z, 0.0f);
    if (m_bLocalSpace != 1) (EXMaths::vec_xyzw &)v = (const EXMaths::mat_44 &)mtx * v;
    p->m_pos = v * m_globalScale;
    EXVector d = dir;
    float speed = ((float)(int)(EXRandFast32() >> 1) * (1.0f / 1073741824.0f) - 1.0f) * m_speedVar + m_speed + m_emitterLife;
    EXVector vel = dir * speed;
    float vx = vel.x, vy = vel.y, vz = vel.z, vw = vel.w;
    if (m_bAddEmitterVel) {
        vx = m_emitterVel.x + vel.x;
        vy = m_emitterVel.y + vel.y;
        vz = m_emitterVel.z + vel.z;
        vw = m_emitterVel.w + vel.w;
    }
    p->m_1c = EXVector(vx, vy, vz, vw) * m_rfps;
    if (m_bLocalSpace != 1) p->m_pos += pos;
    (EXMaths::vec_xyzw &)p->m_1c = (const EXMaths::mat_44 &)mtx * p->m_1c;
    if (!m_bAddEmitterVel) p->m_1c += m_emitterVel * m_rfps;
    CalculateInitialRotation(p, d);
    CalculateStandardParticleSettings(p);
    return p;
}

static __forceinline u32 EXParticleRGBA(EXMaths::vec_xyzw c)
{
    float r = limit(c.x, 0.0f, 255.0f);
    float g = limit(c.y, 0.0f, 255.0f);
    float b = limit(c.z, 0.0f, 255.0f);
    float a = limit(c.w, 0.0f, 255.0f);
    return ((((((u32)(u8)a << 8) | (u8)b) << 8) | (u8)g) << 8) | (u8)r;
}

__forceinline void EXBaseParticleSys::BaseUpdateParticle(EXParticle *p) const
{
    p->m_2c = p->m_pos;
    EXVector gs = m_globalScale;
    p->m_pos += gs * (p->m_1c * m_frameScale);
    p->m_1c *= m_damping;
    p->m_1c += m_force;
    p->m_4c += p->m_9c * m_frameScale;
    p->m_scale += p->m_8c * m_frameScale;
    p->m_5c += p->m_7c * m_frameScale;
    *(u32 *)&p->m_colour = EXParticleRGBA(p->m_5c);
    p->m_time += p->m_life * m_frameScale;
    if (p->m_time >= 100.0f) {
        p->m_life = 0.0f;
        p->m_08 = (u32)-1;
    }
}

__forceinline void EXBaseParticleSys::BaseUpdateParticleWithFade(EXParticle *p)
{
    EXVector gs = m_globalScale;
    p->m_pos += gs * (p->m_1c * m_frameScale);
    p->m_1c *= m_damping;
    p->m_1c += m_force;
    p->m_4c += p->m_9c * m_frameScale;
    p->m_scale += p->m_8c * m_frameScale;
    p->m_5c += p->m_7c * m_frameScale;
    m_fadedColour = p->m_5c * m_fade;
    *(u32 *)&p->m_colour = EXParticleRGBA(m_fadedColour);
    p->m_time += p->m_life * m_frameScale;
    if (p->m_time >= 100.0f) {
        p->m_life = 0.0f;
        p->m_08 = (u32)-1;
    }
}

u32 EXBaseParticleSys::UpdateParticles(const EXGeoParticle *pGeo)
{
    u32 n = 0;
    EXParticle *p = m_pParticles;
    float d = m_emitterRate * m_frameScale;
    m_emitterTime += d;
    if (m_emitterTime > 100.0f && !(m_systemFlags & 1)) m_emitterTime = 0.0f;
    if (m_frameScale == 1.0f) {
        m_damping = EXVector(pGeo->m_damping.x, pGeo->m_damping.y, pGeo->m_damping.z, 0.0f);
    } else {
        float t = pGeo->m_rfps * m_frameScale;
        if (pGeo->m_dampingBase.x != 0.0f) m_damping.x = (float)pow(pGeo->m_dampingBase.x, t);
        else m_damping.x = 0.0f;
        if (pGeo->m_dampingBase.y != 0.0f) m_damping.y = (float)pow(pGeo->m_dampingBase.y, t);
        else m_damping.y = 0.0f;
        if (pGeo->m_dampingBase.z != 0.0f) m_damping.z = (float)pow(pGeo->m_dampingBase.z, t);
        else m_damping.z = 0.0f;
    }
    if (m_gradientDir > 0) m_gradientTime += d;
    else m_gradientTime -= d;
    int last = m_maxParticles - 1;
    if (!m_bFade) {
        for (int i = 0; i <= last; i++, p++) {
            if (p->IsActive()) {
                UpdateParticleLifetime(p, pGeo);
                BaseUpdateParticle(p);
                if (p->IsActive()) n++;
            }
        }
    } else {
        for (int i = 0; i <= last; i++, p++) {
            if (p->IsActive()) {
                UpdateParticleLifetime(p, pGeo);
                BaseUpdateParticleWithFade(p);
                if (p->IsActive()) n++;
            }
        }
    }
    return n;
}

s8 EXBaseParticleSys::CreateEXEmitter(const EXGeoParticle *pGeo)
{
    if (!pGeo->m_entities.Size()) m_64 = 0;
    else m_64 = 1;
    m_systemFlags = pGeo->m_sysFlags;
    *(u16 *)m_7a = pGeo->m_sysFlags2;
    float fps = pGeo->m_rfps != 0.0f ? 1.0f / pGeo->m_rfps : 0.0f;
    float rfps = pGeo->m_rfps;
    m_SortBias = pGeo->m_sortBias;
    m_Layer = pGeo->m_layer;
    m_7c = EXVector(pGeo->m_emitOffset, 0.0f);
    m_8c = EXVector(pGeo->m_emitSpread, 0.0f);
    m_sizeBase = EXVector(pGeo->m_sizeBase, 0.0f);
    m_sizeVar = EXVector(pGeo->m_scaleVar, 0.0f);
    m_force = EXVector(pGeo->m_force, 0.0f);
    m_dc = EXVector(pGeo->m_damping, 0.0f);
    m_damping = EXVector(pGeo->m_dampingBase, 0.0f);
    m_lastEmitPos.set_zero();
    m_10c.set_zero();
    m_emitterVel.set_zero();
    m_globalScale.set(1.0f, 1.0f, 1.0f, 1.0f);
    m_nextParticle = 0;
    m_fps = fps;
    m_rfps = rfps;
    m_158 = pGeo->m_e4;
    m_pitch = pGeo->m_pitch;
    m_pitchVar = pGeo->m_pitchVar;
    m_yaw = pGeo->m_yaw;
    m_yawVar = pGeo->m_yawVar;
    m_speed = pGeo->m_speed;
    m_speedVar = pGeo->m_speedVar;
    m_emitterLife = 0.0f;
    m_emitAccum = 0.0f;
    m_emitterTime = pGeo->m_life;
    *(float *)&m_180 = pGeo->m_lifeVar;
    m_gradientTime = 0.0f;
    m_gradientDir = 1;
    m_texIndex = 0;
    m_texStep = 0;
    m_emitterRate = 100.0f / pGeo->m_life * rfps;
    ((EXMatrix &)m_1f8).SetIdentity();
    CParticleMip mip;
    mip.SetDistance(pGeo->m_mip0Distance);
    u32 max = pGeo->m_maxParticles;
    u32 lim = max;
    int n = (int)((pGeo->m_life * 100.0f - pGeo->m_lifeVar * 100.0f * 0.5f) * pGeo->m_mip0Emits);
    mip.SetEmitsPerFrame(pGeo->m_mip0Emits);
    if (n > 0 && n < (int)max) {
        max = n;
        lim = n;
    }
    mip.SetParticleLimit(lim);
    SetParticleMip(ParticleMip::NEAR_MIP, mip);
    m_curParticleLimit = lim;
    m_curEmitsPerFrame = pGeo->m_mip0Emits;
    mip.SetEmitsPerFrame(pGeo->m_mip1Emits);
    mip.SetDistance(pGeo->m_mip1Distance);
    mip.SetParticleLimit(pGeo->m_mip1Limit);
    SetParticleMip(ParticleMip::FAR_MIP, mip);
    if (!m_bUseEmitter) {
        m_pParticles = (EXParticle *)_EXAlloc(max * sizeof(EXParticle), 1);
        if (!m_pParticles) return 0;
        memset(m_pParticles, 0, max * sizeof(EXParticle));
    }
    m_maxParticles = max;
    m_nextNew = 0;
    m_numActive = 0;
    m_1cc = pGeo->m_entities.Size();
    m_colourMode = (ColourMode::Type)pGeo->m_colourMode;
    m_colourVar = (Variation::Type)pGeo->m_colourVar;
    m_textureMode = (Variation::Type)pGeo->m_textureMode;
    m_pEmitter = 0;
    m_bEndParticles = 0;
    m_bFade = 0;
    m_bLocalSpace = pGeo->m_localSpace;
    m_bAddEmitterVel = 0;
    m_bNoCreate = 0;
    m_bEmitWithDir = pGeo->m_emitWithDir;
    m_bUseMips = pGeo->m_useMips;
    m_bAllUsed = 0;
    if (pGeo->m_entities.Size()) {
        EXGeoHeader *h = m_geoRef.GeoFile()->GeoHeader();
        EXGeoEntityHeader *eh = h->EntityList().Find(pGeo->m_entities[0]);
        EXGeoCommonObject *o = eh ? eh->Object(0) : 0;
        if (o->IsKindOf(&EXGeoInstanceEntity::classEXGeoInstanceEntity)) {
            m_64 = 1;
        } else if (o->IsKindOf(&EXGeoEntity::classEXGeoEntity)) {
            m_64 = 2;
            m_pGDiArrays = new EXGDiDataArray[pGeo->m_maxParticles];
            if (!m_pGDiArrays) return 0;
            for (u32 i = 0; i < pGeo->m_maxParticles; i++) {
                m_pGDiArrays[i].RemoveAll();
                m_pGDiArrays[i].Init(((EXGeoEntity *)o)->GDiCount());
            }
        } else {
            return 0;
        }
    }
    if (m_bUseEmitter == 1) {
        switch (m_emitterType) {
        case 1: m_pEmitter = new EXEmitter_NR; break;
        case 3: m_pEmitter = new EXEmitter_NRNS; break;
        case 4: m_pEmitter = new EXEmitter_NRNC; break;
        default: return 0;
        }
        if (!m_pEmitter) return 0;
        m_pEmitter->Initialise(this, pGeo);
    }
    for (u32 i = 0; i < 8; i++) m_values[i] = pGeo->Value(i);
    if (pGeo->m_preUpdates != 0.0f) m_bFirstUpdate = 1;
    else m_bFirstUpdate = 0;
    return 1;
}

void EXBaseParticleSys::CalculateInitialRotation(EXParticle *p, const EXVector &dir) const
{
    if (m_systemFlags & 2) {
        if (!m_64) {
            p->m_6c.z = EXRandFastf(-3.1415927f, 3.1415927f);
            return;
        }
        const EXGeoParticle *pGeo = m_geoRef.GeoFile()->GeoHeader()->GeoParticle(m_geoHash);
        if (pGeo->m_flags46 & 8) p->m_6c.x = EXRandFastf(-3.1415927f, 3.1415927f);
        if (pGeo->m_flags46 & 8) p->m_6c.y = EXRandFastf(-3.1415927f, 3.1415927f);
        if (pGeo->m_flags46 & 8) p->m_6c.z = EXRandFastf(-3.1415927f, 3.1415927f);
    } else if (m_systemFlags & 4) {
        float a = (float)atan2(-dir.y, dir.x) + 1.570796f;
        p->m_6c.z = a;
        if (a < 0.0f) p->m_6c.z = a + 6.283185f;
        else if (a > 6.283185f) p->m_6c.z = a - 6.283185f;
    } else {
        p->m_6c.z = 0.0f;
    }
}

s8 EXBaseParticleSys::Update(const EXItemAnimator *pAnim, const EXGeoParticle *pGeo)
{
    if (!m_bEnabled && pAnim) return 1;
    CalculateMipSettings();
    if (!m_numActive && (m_systemFlags & 1) && m_nextNew >= m_curParticleLimit) return 0;
    float s = m_frameScale;
    m_force = EXVector(pGeo->m_force.x * s, pGeo->m_force.y * s, pGeo->m_force.z * s, 0.0f);
    if (pAnim) {
        if (!m_bNoCreate && m_numActive < m_maxParticles) CreateParticles(pAnim, pGeo);
        if (pGeo->m_flags46 & 4) {
            EXMaths::vec_xyzw t;
            D3DXVec4Transform(&t, &m_force, &m_1f8);
            *(EXMaths::vec_xyzw *)&m_force = t;
        }
    } else if (!m_numActive) {
        return 0;
    }
    m_numActive = UpdateParticles(pGeo);
    return 1;
}
