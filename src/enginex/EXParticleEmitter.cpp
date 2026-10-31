#include "EXEmitter.h"
#include "EXMatrix.h"
#include "EXGeoFile.h"
#include "EXGeoHeader.h"
#include "EXItemAnimator.h"
#include "EXBaseParticleSys.h"
#include "EXGeoParticle.h"
#include "EXGeoEntity.h"
#include "EXArray.h"
#include "EXGeoComplexRefObject.h"
#include "EXMemory.h"
#include "EXRand.h"
#include "EXVector.h"
#include "EXStdFuncs.h"
#include <string.h>
#include <math.h>

EXGeoEntity_InfoArray ParticleInfoArray;  // entity pieces the emitter variants pick vertices from

EXEmitter::EXEmitter()
{
}

EXEmitter::~EXEmitter()
{
}

s8 EXEmitter::Initialise(EXBaseParticleSys *pSys, const EXGeoParticle *pGeo)
{
    return 0;
}

s8 EXEmitter::Update(EXBaseParticleSys *pSys, const EXItemAnimator *pAnim, const EXGeoParticle *pGeo)
{
    return 0;
}

void EXEmitter::Release()
{
}

float EXEmitter::CalculateEmittedLife(const EXGeoParticle *pGeo, const EXBaseParticleSys *pSys) const
{
    float life = 0.0f;
    for (const EXGeoParticleKey *k = pGeo->m_keys; k->m_type != -1; k++) {
        if (k->m_time >= pSys->m_emitterTime) break;
        if (k->m_type == 0xb) life = k->m_scale * pSys->m_emitterTime + k->m_base;
    }
    return life;
}

float EXEmitter::CalculateEmittedVelocity(const EXGeoParticle *pGeo, const EXBaseParticleSys *pSys) const
{
    float v = 0.0f;
    for (const EXGeoParticleKey *k = pGeo->m_keys; k->m_type != -1; k++) {
        if (k->m_time > pSys->m_emitterTime) break;
        if (k->m_type == 0xa) v = k->m_scale * pSys->m_emitterTime + k->m_base;
    }
    return v;
}

void EXEmitter::CalculateEmitterColour(const EXGeoParticle *pGeo, float t, EXVector *pOut) const
{
    for (const EXGeoParticleKey *k = pGeo->m_keys; k->m_type != -1; k++) {
        if (k->m_time >= t) break;
        switch (k->m_type) {
        case 6:
            pOut[0].x = k->m_scale * t + k->m_base;
            pOut[1].x = 0.0f;
            break;
        case 7:
            pOut[0].y = k->m_scale * t + k->m_base;
            pOut[1].y = 0.0f;
            break;
        case 8:
            pOut[0].z = k->m_scale * t + k->m_base;
            pOut[1].z = 0.0f;
            break;
        case 9:
            pOut[0].w = k->m_scale * t + k->m_base;
            pOut[1].w = 0.0f;
            break;
        }
    }
}

void EXEmitter::StandardEmitterUpdate(const EXGeoParticle *pGeo, EXBaseParticleSys *pSys) const
{
    float d = pSys->m_frameScale * pSys->m_emitterRate;
    pSys->m_emitterTime += d;
    if (pSys->m_emitterTime > 100.0f && !(pSys->m_systemFlags & 1)) pSys->m_emitterTime = 0.0f;
    if (pSys->m_gradientDir > 0) pSys->m_gradientTime += d;
    else pSys->m_gradientTime -= d;
}

float EXEmitter::CalculateGradientColourTime(EXBaseParticleSys *pSys) const
{
    float t = 0.0f;
    if (pSys->m_colourVar == 1) {
        if (pSys->m_gradientTime > 100.0f) {
            t = 100.0f;
            pSys->m_gradientDir = -1;
            pSys->m_gradientTime = 100.0f;
        } else {
            if (pSys->m_gradientTime < 0.0f) {
                pSys->m_gradientDir = 1;
                pSys->m_gradientTime = 0.0f;
            }
            t = pSys->m_gradientTime;
        }
    } else if (pSys->m_colourVar == 0) {
        pSys->m_gradientDir = 1;
        if (pSys->m_gradientTime > 100.0f) pSys->m_gradientTime -= 100.0f;
        t = pSys->m_gradientTime;
    } else if (pSys->m_colourVar == 2) {
        pSys->m_gradientDir = 1;
        t = EXRandFastf(100.0f);
        if (t < 0.0f) t = 0.0f;
        else if (t > 100.0f) t = 100.0f;
    }
    return t;
}

EXEmitter_NR::EXEmitter_NR()
{
    m_pParticles = 0;
}

EXEmitter_NR::~EXEmitter_NR()
{
}

s8 EXEmitter_NR::Initialise(EXBaseParticleSys *pSys, const EXGeoParticle *pGeo)
{
    m_particleBytes = 0;
    m_pParticles = (EXParticle_NR *)_EXAlloc(pSys->m_maxParticles * sizeof(EXParticle_NR), 0);
    if (!m_pParticles) return 0;
    memset(m_pParticles, 0, pSys->m_maxParticles * sizeof(EXParticle_NR));
    m_particleBytes = pSys->m_maxParticles * sizeof(EXParticle_NR);
    m_08 = pSys->m_maxParticles * 0x2c;
    return 1;
}

// NEAR: retail compares the flags with immediates (no shared 1 register) and schedules the
// search-loop decrement before the test.
EXParticle_NR *EXEmitter_NR::GetFreeParticle(EXBaseParticleSys *pSys)
{
    if (pSys->m_bAllUsed == 1) {
        if (pSys->m_systemFlags & 1) return 0;
        EXParticle_NR *p = m_pParticles;
        for (int i = pSys->m_maxParticles - 1; i >= 0; i--) {
            if (!p->IsActive()) return p;
            p++;
        }
        return 0;
    }
    int n = pSys->m_nextNew;
    if (n >= pSys->m_maxParticles) return 0;
    EXParticle_NR *p = &m_pParticles[n];
    if (p->IsActive()) return 0;
    pSys->m_nextNew = n + 1;
    if (n + 1 >= pSys->m_maxParticles) pSys->m_bAllUsed = 1;
    return p;
}

void EXEmitter_NR::Release()
{
    if (m_pParticles) {
        EXFree(m_pParticles);
        m_pParticles = 0;
    }
}

EXEmitter_NRNS::EXEmitter_NRNS()
{
    m_pParticles = 0;
}

EXEmitter_NRNS::~EXEmitter_NRNS()
{
}

s8 EXEmitter_NRNS::Initialise(EXBaseParticleSys *pSys, const EXGeoParticle *pGeo)
{
    m_particleBytes = 0;
    m_pParticles = (EXParticle_NRNS *)_EXAlloc(pSys->m_maxParticles * sizeof(EXParticle_NRNS), 0);
    m_particleBytes = pSys->m_maxParticles * sizeof(EXParticle_NRNS);
    m_08 = pSys->m_maxParticles * 0x40;
    if (!m_pParticles) return 0;
    memset(m_pParticles, 0, pSys->m_maxParticles * sizeof(EXParticle_NRNS));
    return 1;
}

void EXEmitter_NRNS::Release()
{
    if (m_pParticles) {
        EXFree(m_pParticles);
        m_pParticles = 0;
    }
}

// NEAR: retail compares the flags with immediates (no shared 1 register) and schedules the
// search-loop decrement before the test.
EXParticle_NRNS *EXEmitter_NRNS::GetFreeParticle(EXBaseParticleSys *pSys)
{
    if (pSys->m_bAllUsed == 1) {
        if (pSys->m_systemFlags & 1) return 0;
        EXParticle_NRNS *p = m_pParticles;
        for (int i = pSys->m_maxParticles - 1; i >= 0; i--) {
            if (!p->IsActive()) return p;
            p++;
        }
        return 0;
    }
    int n = pSys->m_nextNew;
    if (n >= pSys->m_maxParticles) return 0;
    EXParticle_NRNS *p = &m_pParticles[n];
    if (p->IsActive()) return 0;
    pSys->m_nextNew = n + 1;
    if (n + 1 >= pSys->m_maxParticles) pSys->m_bAllUsed = 1;
    return p;
}

EXEmitter_NRNC::EXEmitter_NRNC()
{
    m_pParticles = 0;
}

EXEmitter_NRNC::~EXEmitter_NRNC()
{
}

s8 EXEmitter_NRNC::Initialise(EXBaseParticleSys *pSys, const EXGeoParticle *pGeo)
{
    m_particleBytes = 0;
    m_pParticles = (EXParticle_NRNC *)_EXAlloc(pSys->m_maxParticles * sizeof(EXParticle_NRNC), 0);
    m_particleBytes = pSys->m_maxParticles * sizeof(EXParticle_NRNC);
    m_08 = pSys->m_maxParticles * 0x40;
    if (!m_pParticles) return 0;
    memset(m_pParticles, 0, pSys->m_maxParticles * sizeof(EXParticle_NRNC));
    return 1;
}

// NEAR: retail compares the flags with immediates (no shared 1 register) and schedules the
// search-loop decrement before the test.
EXParticle_NRNC *EXEmitter_NRNC::GetFreeParticle(EXBaseParticleSys *pSys)
{
    if (pSys->m_bAllUsed == 1) {
        if (pSys->m_systemFlags & 1) return 0;
        EXParticle_NRNC *p = m_pParticles;
        for (int i = pSys->m_maxParticles - 1; i >= 0; i--) {
            if (!p->IsActive()) return p;
            p++;
        }
        return 0;
    }
    int n = pSys->m_nextNew;
    if (n >= pSys->m_maxParticles) return 0;
    EXParticle_NRNC *p = &m_pParticles[n];
    if (p->IsActive()) return 0;
    pSys->m_nextNew = n + 1;
    if (n + 1 >= pSys->m_maxParticles) pSys->m_bAllUsed = 1;
    return p;
}

void EXEmitter_NRNC::Release()
{
    if (m_pParticles) {
        EXFree(m_pParticles);
        m_pParticles = 0;
    }
}

void EXEmitter_NR::BaseUpdateParticleLifetime(EXParticle_NR *p, const EXGeoParticle *pGeo, const EXBaseParticleSys *pSys) const
{
    for (const EXGeoParticleKey *k = &pGeo->m_keys[p->m_08]; k->m_type != -1; k = &pGeo->m_keys[++p->m_08]) {
        if (k->m_time > p->m_time) return;
        float base = k->m_base;
        float rate = k->m_scale * p->m_life;
        switch (k->m_type) {
        case 0:
        case 1:
        case 2:
            rate = base;
            break;
        case 3: p->m_scale.x = base; p->m_scaleRate.x = rate; break;
        case 4: p->m_scale.y = base; p->m_scaleRate.y = rate; break;
        case 5: p->m_scale.z = base; p->m_scaleRate.z = rate; break;
        case 6: if (pSys->m_colourMode != 1) { p->m_colour.x = base; p->m_colourRate.x = rate; } break;
        case 7: if (pSys->m_colourMode != 1) { p->m_colour.y = base; p->m_colourRate.y = rate; } break;
        case 8: if (pSys->m_colourMode != 1) { p->m_colour.z = base; p->m_colourRate.z = rate; } break;
        case 9: if (pSys->m_colourMode != 1) { p->m_colour.w = base; p->m_colourRate.w = rate; } break;
        }
    }
}

void EXEmitter_NRNC::BaseUpdateParticleLifetime(EXParticle_NRNC *p, const EXGeoParticle *pGeo, const EXBaseParticleSys *pSys) const
{
    for (const EXGeoParticleKey *k = &pGeo->m_keys[p->m_08]; k->m_type != -1; k = &pGeo->m_keys[++p->m_08]) {
        if (k->m_time >= p->m_time) return;
        float base = k->m_base;
        float rate = k->m_scale * p->m_life;
        switch (k->m_type) {
        case 0:
        case 1:
        case 2:
            rate = base;
            break;
        case 3: p->m_scale.x = base; p->m_scaleRate.x = rate; break;
        case 4: p->m_scale.y = base; p->m_scaleRate.y = rate; break;
        case 5: p->m_scale.z = base; p->m_scaleRate.z = rate; break;
        case 6: if (pSys->m_colourMode != 1) p->m_colour.x = base; break;
        case 7: if (pSys->m_colourMode != 1) p->m_colour.y = base; break;
        case 8: if (pSys->m_colourMode != 1) p->m_colour.z = base; break;
        case 9: if (pSys->m_colourMode != 1) p->m_colour.w = base; break;
        }
    }
}

// Scale keys set the scale once (no slope); the z channel adds the variation itself rather than
// the random value, and x ignores system flag 0x40 (both as in retail).
void EXEmitter_NRNS::BaseUpdateParticleLifetime(EXParticle_NRNS *p, const EXGeoParticle *pGeo, const EXBaseParticleSys *pSys) const
{
    float var = 1.0f;
    for (const EXGeoParticleKey *k = &pGeo->m_keys[p->m_08]; k->m_type != -1; k = &pGeo->m_keys[++p->m_08]) {
        if (k->m_time > p->m_time) return;
        float base = k->m_base;
        float rate = k->m_scale * p->m_life;
        switch (k->m_type) {
        case 0:
        case 1:
        case 2:
            rate = base;
            break;
        case 3:
            var = ((float)(int)(EXRandFast32() >> 1) * (1.0f / 1073741824.0f) - 1.0f) * pGeo->m_scaleVar.x;
            p->m_scale.x = base + var;
            break;
        case 4:
            if (!(pSys->m_systemFlags & 0x40)) var = ((float)(int)(EXRandFast32() >> 1) * (1.0f / 1073741824.0f) - 1.0f) * pGeo->m_scaleVar.y;
            p->m_scale.y = base + var;
            break;
        case 5:
            if (!(pSys->m_systemFlags & 0x40)) var = ((float)(int)(EXRandFast32() >> 1) * (1.0f / 1073741824.0f) - 1.0f) * pGeo->m_scaleVar.z;
            p->m_scale.z = pGeo->m_scaleVar.z + base;
            break;
        case 6: if (pSys->m_colourMode != 1) { p->m_colour.x = base; p->m_colourRate.x = rate; } break;
        case 7: if (pSys->m_colourMode != 1) { p->m_colour.y = base; p->m_colourRate.y = rate; } break;
        case 8: if (pSys->m_colourMode != 1) { p->m_colour.z = base; p->m_colourRate.z = rate; } break;
        case 9: if (pSys->m_colourMode != 1) { p->m_colour.w = base; p->m_colourRate.w = rate; } break;
        }
    }
}

s8 EXEmitter_NR::Update(EXBaseParticleSys *pSys, const EXItemAnimator *pAnim, const EXGeoParticle *pGeo)
{
    if (!pSys->m_bEnabled && pAnim) return 1;
    pSys->CalculateMipSettings();
    if (!pSys->m_numActive && (pSys->m_systemFlags & 1) && pSys->m_nextNew >= pSys->m_curParticleLimit) return 0;
    float s = pSys->m_frameScale;
    pSys->m_force = EXVector(pGeo->m_force.x * s, pGeo->m_force.y * s, pGeo->m_force.z * s, 0.0f);
    if (pAnim) {
        if (!pSys->m_bNoCreate && pSys->m_numActive < pSys->m_maxParticles) CreateParticles(pAnim, pGeo, pSys);
        EXMaths::vec_xyzw t;
        D3DXVec4Transform(&t, &pSys->m_force, &pSys->m_1f8);
        *(EXMaths::vec_xyzw *)&pSys->m_force = t;
    } else if (!pSys->m_numActive) {
        return 0;
    }
    pSys->m_numActive = UpdateParticles(pSys, pGeo);
    return 1;
}

s8 EXEmitter_NRNC::Update(EXBaseParticleSys *pSys, const EXItemAnimator *pAnim, const EXGeoParticle *pGeo)
{
    if (!pSys->m_bEnabled && pAnim) return 1;
    pSys->CalculateMipSettings();
    if (!pSys->m_numActive && (pSys->m_systemFlags & 1) && pSys->m_nextNew >= pSys->m_curParticleLimit) return 0;
    float s = pSys->m_frameScale;
    pSys->m_force = EXVector(pGeo->m_force.x * s, pGeo->m_force.y * s, pGeo->m_force.z * s, 0.0f);
    if (pAnim) {
        if (!pSys->m_bNoCreate && pSys->m_numActive < pSys->m_maxParticles) CreateParticles(pAnim, pGeo, pSys);
        EXMaths::vec_xyzw t;
        D3DXVec4Transform(&t, &pSys->m_force, &pSys->m_1f8);
        *(EXMaths::vec_xyzw *)&pSys->m_force = t;
    } else if (!pSys->m_numActive) {
        return 0;
    }
    pSys->m_numActive = UpdateParticles(pSys, pGeo);
    return 1;
}

s8 EXEmitter_NRNS::Update(EXBaseParticleSys *pSys, const EXItemAnimator *pAnim, const EXGeoParticle *pGeo)
{
    if (!pSys->m_bEnabled && pAnim) return 1;
    pSys->CalculateMipSettings();
    if (!pSys->m_numActive && (pSys->m_systemFlags & 1) && pSys->m_nextNew >= pSys->m_curParticleLimit) return 0;
    float s = pSys->m_frameScale;
    pSys->m_force = EXVector(pGeo->m_force.x * s, pGeo->m_force.y * s, pGeo->m_force.z * s, 0.0f);
    if (pAnim) {
        if (!pSys->m_bNoCreate && pSys->m_numActive < pSys->m_maxParticles) CreateParticles(pAnim, pGeo, pSys);
        EXMaths::vec_xyzw t;
        D3DXVec4Transform(&t, &pSys->m_force, &pSys->m_1f8);
        *(EXMaths::vec_xyzw *)&pSys->m_force = t;
    } else if (!pSys->m_numActive) {
        return 0;
    }
    pSys->m_numActive = UpdateParticles(pSys, pGeo);
    return 1;
}

u32 EXEmitter_NR::UpdateParticles(EXBaseParticleSys *pSys, const EXGeoParticle *pGeo)
{
    u32 n = 0;
    EXParticle_NR *p = m_pParticles;
    StandardEmitterUpdate(pGeo, pSys);
    int last = pSys->m_maxParticles - 1;
    if (pSys->m_frameScale == 1.0f) {
        pSys->m_damping = EXVector(pGeo->m_damping.x, pGeo->m_damping.y, pGeo->m_damping.z, 0.0f);
    } else {
        float t = pGeo->m_rfps * pSys->m_frameScale;
        if (pGeo->m_dampingBase.x != 0.0f) pSys->m_damping.x = (float)pow(pGeo->m_dampingBase.x, t);
        if (pGeo->m_dampingBase.y != 0.0f) pSys->m_damping.y = (float)pow(pGeo->m_dampingBase.y, t);
        if (pGeo->m_dampingBase.z != 0.0f) pSys->m_damping.z = (float)pow(pGeo->m_dampingBase.z, t);
    }
    if (!pSys->m_bFade) {
        for (int i = 0; i <= last; i++, p++) {
            if (p->IsActive()) {
                BaseUpdateParticleLifetime(p, pGeo, pSys);
                BaseUpdateParticle(p, pSys);
                if (p->IsActive()) n++;
            }
        }
    } else {
        for (int i = 0; i <= last; i++, p++) {
            if (p->IsActive()) {
                BaseUpdateParticleLifetime(p, pGeo, pSys);
                BaseUpdateParticleWithFade(p, pSys);
                if (p->IsActive()) n++;
            }
        }
    }
    return n;
}

u32 EXEmitter_NRNS::UpdateParticles(EXBaseParticleSys *pSys, const EXGeoParticle *pGeo)
{
    u32 n = 0;
    EXParticle_NRNS *p = m_pParticles;
    StandardEmitterUpdate(pGeo, pSys);
    int last = pSys->m_maxParticles - 1;
    if (pSys->m_frameScale == 1.0f) {
        pSys->m_damping = EXVector(pGeo->m_damping.x, pGeo->m_damping.y, pGeo->m_damping.z, 0.0f);
    } else {
        float t = pGeo->m_rfps * pSys->m_frameScale;
        if (pGeo->m_dampingBase.x != 0.0f) pSys->m_damping.x = (float)pow(pGeo->m_dampingBase.x, t);
        if (pGeo->m_dampingBase.y != 0.0f) pSys->m_damping.y = (float)pow(pGeo->m_dampingBase.y, t);
        if (pGeo->m_dampingBase.z != 0.0f) pSys->m_damping.z = (float)pow(pGeo->m_dampingBase.z, t);
    }
    if (!pSys->m_bFade) {
        for (int i = 0; i <= last; i++, p++) {
            if (p->IsActive()) {
                BaseUpdateParticleLifetime(p, pGeo, pSys);
                BaseUpdateParticle(p, pSys);
                if (p->IsActive()) n++;
            }
        }
    } else {
        for (int i = 0; i <= last; i++, p++) {
            if (p->IsActive()) {
                BaseUpdateParticleLifetime(p, pGeo, pSys);
                BaseUpdateParticleWithFade(p, pSys);
                if (p->IsActive()) n++;
            }
        }
    }
    return n;
}

u32 EXEmitter_NRNC::UpdateParticles(EXBaseParticleSys *pSys, const EXGeoParticle *pGeo)
{
    u32 n = 0;
    EXParticle_NRNC *p = m_pParticles;
    StandardEmitterUpdate(pGeo, pSys);
    int last = pSys->m_maxParticles - 1;
    if (pSys->m_frameScale == 1.0f) {
        pSys->m_damping = EXVector(pGeo->m_damping.x, pGeo->m_damping.y, pGeo->m_damping.z, 0.0f);
    } else {
        float t = pGeo->m_rfps * pSys->m_frameScale;
        if (pGeo->m_dampingBase.x != 0.0f) pSys->m_damping.x = (float)pow(pGeo->m_dampingBase.x, t);
        if (pGeo->m_dampingBase.y != 0.0f) pSys->m_damping.y = (float)pow(pGeo->m_dampingBase.y, t);
        if (pGeo->m_dampingBase.z != 0.0f) pSys->m_damping.z = (float)pow(pGeo->m_dampingBase.z, t);
    }
    if (!pSys->m_bFade) {
        for (int i = 0; i <= last; i++, p++) {
            if (p->IsActive()) {
                BaseUpdateParticleLifetime(p, pGeo, pSys);
                BaseUpdateParticle(p, pSys);
                if (p->IsActive()) n++;
            }
        }
    } else {
        for (int i = 0; i <= last; i++, p++) {
            if (p->IsActive()) {
                BaseUpdateParticleLifetime(p, pGeo, pSys);
                BaseUpdateParticleWithFade(p, pSys);
                if (p->IsActive()) n++;
            }
        }
    }
    return n;
}

static __forceinline u32 EXEmitterRGBA(EXMaths::vec_xyzw c)
{
    float r = limit(c.x, 0.0f, 255.0f);
    float g = limit(c.y, 0.0f, 255.0f);
    float b = limit(c.z, 0.0f, 255.0f);
    float a = limit(c.w, 0.0f, 255.0f);
    return ((((((u32)(u8)a << 8) | (u8)b) << 8) | (u8)g) << 8) | (u8)r;
}

void EXEmitter_NRNS::BaseUpdateParticle(EXParticle_NRNS *p, const EXBaseParticleSys *pSys) const
{
    EXVector gs = pSys->m_globalScale;
    p->m_pos += gs * (p->m_vel * pSys->m_frameScale);
    p->m_vel *= pSys->m_damping;
    p->m_vel += pSys->m_force;
    p->m_colour += p->m_colourRate * pSys->m_frameScale;
    p->m_rgba = EXEmitterRGBA(p->m_colour);
    p->m_time += p->m_life * pSys->m_frameScale;
    if (p->m_time >= 100.0f) {
        p->m_08 = (u32)-1;
        p->m_life = 0.0f;
    }
}

void EXEmitter_NR::BaseUpdateParticle(EXParticle_NR *p, const EXBaseParticleSys *pSys) const
{
    EXVector gs = pSys->m_globalScale;
    p->m_pos += gs * (p->m_vel * pSys->m_frameScale);
    p->m_vel *= pSys->m_damping;
    p->m_vel += pSys->m_force;
    p->m_scale += p->m_scaleRate * pSys->m_frameScale;
    p->m_colour += p->m_colourRate * pSys->m_frameScale;
    p->m_rgba = EXEmitterRGBA(p->m_colour);
    p->m_time += p->m_life * pSys->m_frameScale;
    if (p->m_time >= 100.0f) {
        p->m_life = 0.0f;
        p->m_08 = (u32)-1;
    }
}

void EXEmitter_NRNC::BaseUpdateParticle(EXParticle_NRNC *p, const EXBaseParticleSys *pSys) const
{
    EXVector gs = pSys->m_globalScale;
    p->m_pos += gs * (p->m_vel * pSys->m_frameScale);
    p->m_vel *= pSys->m_dc;
    p->m_vel += pSys->m_force;
    p->m_scale += p->m_scaleRate * pSys->m_frameScale;
    p->m_rgba = EXEmitterRGBA(p->m_colour);
    p->m_time += p->m_life * pSys->m_frameScale;
    if (p->m_time >= 100.0f) {
        p->m_life = 0.0f;
        p->m_08 = (u32)-1;
    }
}

void EXEmitter_NRNS::BaseUpdateParticleWithFade(EXParticle_NRNS *p, EXBaseParticleSys *pSys) const
{
    EXVector gs = pSys->m_globalScale;
    p->m_pos += gs * (p->m_vel * pSys->m_frameScale);
    p->m_vel *= pSys->m_damping;
    p->m_vel += pSys->m_force;
    p->m_colour += p->m_colourRate * pSys->m_frameScale;
    pSys->m_fadedColour = p->m_colour * pSys->m_fade;
    p->m_rgba = EXEmitterRGBA(pSys->m_fadedColour);
    p->m_time += p->m_life * pSys->m_frameScale;
    if (p->m_time >= 100.0f) {
        p->m_08 = (u32)-1;
        p->m_life = 0.0f;
    }
}

void EXEmitter_NR::BaseUpdateParticleWithFade(EXParticle_NR *p, EXBaseParticleSys *pSys) const
{
    EXVector gs = pSys->m_globalScale;
    p->m_pos += gs * (p->m_vel * pSys->m_frameScale);
    p->m_vel *= pSys->m_damping;
    p->m_vel += pSys->m_force;
    p->m_scale += p->m_scaleRate * pSys->m_frameScale;
    p->m_colour += p->m_colourRate * pSys->m_frameScale;
    pSys->m_fadedColour = p->m_colour * pSys->m_fade;
    p->m_rgba = EXEmitterRGBA(pSys->m_fadedColour);
    p->m_time += p->m_life * pSys->m_frameScale;
    if (p->m_time >= 100.0f) {
        p->m_life = 0.0f;
        p->m_08 = (u32)-1;
    }
}

void EXEmitter_NRNC::BaseUpdateParticleWithFade(EXParticle_NRNC *p, EXBaseParticleSys *pSys) const
{
    EXVector gs = pSys->m_globalScale;
    p->m_pos += gs * (p->m_vel * pSys->m_frameScale);
    p->m_vel *= pSys->m_dc;
    p->m_vel += pSys->m_force;
    p->m_scale += p->m_scaleRate * pSys->m_frameScale;
    pSys->m_fadedColour = p->m_colour * pSys->m_fade;
    p->m_rgba = EXEmitterRGBA(pSys->m_fadedColour);
    p->m_time += p->m_life * pSys->m_frameScale;
    if (p->m_time >= 100.0f) {
        p->m_life = 0.0f;
        p->m_08 = (u32)-1;
    }
}

void EXEmitter_NR::CalculateInitialColour(EXParticle_NR *p, EXBaseParticleSys *pSys) const
{
    const EXGeoParticle *pGeo = pSys->m_geoRef.GeoParticle();
    p->m_colour.Set(0.0f, 0.0f, 0.0f, 0.0f);
    if (pSys->m_colourMode == 1) {
        EXVector c[2];
        float t = CalculateGradientColourTime(pSys);
        CalculateEmitterColour(pGeo, t, c);
        p->m_colour = c[0];
        p->m_colourRate = c[1];
    }
    if (pSys->m_systemFlags & 0x100) p->m_time = CalculateEmittedLife(pGeo, pSys);
    if (pSys->m_systemFlags & 0x200) pSys->m_emitterLife = CalculateEmittedVelocity(pGeo, pSys);
    BaseUpdateParticleLifetime(p, pGeo, pSys);
    p->m_rgba = EXEmitterRGBA(p->m_colour);
}

void EXEmitter_NRNS::CalculateInitialColour(EXParticle_NRNS *p, EXBaseParticleSys *pSys) const
{
    const EXGeoParticle *pGeo = pSys->m_geoRef.GeoParticle();
    p->m_colour.Set(0.0f, 0.0f, 0.0f, 0.0f);
    if (pSys->m_colourMode == 1) {
        EXVector c[2];
        float t = CalculateGradientColourTime(pSys);
        CalculateEmitterColour(pGeo, t, c);
        p->m_colour = c[0];
        p->m_colourRate = c[1];
    }
    if (pSys->m_systemFlags & 0x100) p->m_time = CalculateEmittedLife(pGeo, pSys);
    if (pSys->m_systemFlags & 0x200) pSys->m_emitterLife = CalculateEmittedVelocity(pGeo, pSys);
    BaseUpdateParticleLifetime(p, pGeo, pSys);
    p->m_rgba = EXEmitterRGBA(p->m_colour);
}

void EXEmitter_NRNC::CalculateInitialColour(EXParticle_NRNC *p, EXBaseParticleSys *pSys) const
{
    const EXGeoParticle *pGeo = pSys->m_geoRef.GeoParticle();
    p->m_colour.Set(0.0f, 0.0f, 0.0f, 0.0f);
    if (pSys->m_colourMode == 1) {
        EXVector c[2];
        float t = CalculateGradientColourTime(pSys);
        CalculateEmitterColour(pGeo, t, c);
        p->m_colour = c[0];
    }
    if (pSys->m_systemFlags & 0x100) p->m_time = CalculateEmittedLife(pGeo, pSys);
    if (pSys->m_systemFlags & 0x200) pSys->m_emitterLife = CalculateEmittedVelocity(pGeo, pSys);
    BaseUpdateParticleLifetime(p, pGeo, pSys);
    p->m_rgba = EXEmitterRGBA(p->m_colour);
}

void EXEmitter_NR::EmitParticles(EXBaseParticleSys *pSys, const EXMatrix &mtx, u32 n, u32 flags)
{
    EXFixedArray<EXMaths::vec_xyzw, 64> verts;
    u32 avail = pSys->m_mips[0].m_particleLimit - pSys->m_numActive;
    u32 cnt = 0;
    u32 idx = 0;
    for (u32 i = 0; i < n; i++) {
        if (!cnt) {
            u32 c = min((u32)32, avail);
            cnt = min(c, n);
            ParticleInfoArray.GetRandomVertexArray((EXVector *)&verts[0], cnt, flags);
            idx %= cnt;
        }
        EXParticle_NR *p;
        if (pSys->m_bEmitWithDir) {
            EXVector pos(verts[idx]);
            EXVector dir(verts[idx + 1]);
            idx = (idx + 14) % (cnt * 2);
            p = CreateParticleWithDir(pSys, pos, dir, mtx);
        } else {
            EXVector pos(verts[idx]);
            p = BaseCreateParticle(pSys, pos, mtx);
            idx = (idx + 7) % cnt;
        }
        cnt--;
        if (!p) return;
    }
}

void EXEmitter_NRNS::EmitParticles(EXBaseParticleSys *pSys, const EXMatrix &mtx, u32 n, u32 flags)
{
    EXFixedArray<EXMaths::vec_xyzw, 64> verts;
    u32 avail = pSys->m_mips[0].m_particleLimit - pSys->m_numActive;
    u32 cnt = 0;
    u32 idx = 0;
    for (u32 i = 0; i < n; i++) {
        if (!cnt) {
            u32 c = min((u32)32, avail);
            cnt = min(c, n);
            ParticleInfoArray.GetRandomVertexArray((EXVector *)&verts[0], cnt, flags);
            idx %= cnt;
        }
        EXParticle_NRNS *p;
        if (pSys->m_bEmitWithDir) {
            EXVector pos(verts[idx]);
            EXVector dir(verts[idx + 1]);
            idx = (idx + 14) % (cnt * 2);
            p = CreateParticleWithDir(pSys, pos, dir, mtx);
        } else {
            EXVector pos(verts[idx]);
            p = BaseCreateParticle(pSys, pos, mtx);
            idx = (idx + 7) % cnt;
        }
        cnt--;
        if (!p) return;
    }
}

void EXEmitter_NRNC::EmitParticles(EXBaseParticleSys *pSys, const EXMatrix &mtx, u32 n, u32 flags)
{
    EXFixedArray<EXMaths::vec_xyzw, 64> verts;
    u32 avail = pSys->m_mips[0].m_particleLimit - pSys->m_numActive;
    u32 cnt = 0;
    u32 idx = 0;
    for (u32 i = 0; i < n; i++) {
        if (!cnt) {
            u32 c = min((u32)32, avail);
            cnt = min(c, n);
            ParticleInfoArray.GetRandomVertexArray((EXVector *)&verts[0], cnt, flags);
            idx %= cnt;
        }
        EXParticle_NRNC *p;
        if (pSys->m_bEmitWithDir) {
            EXVector pos(verts[idx]);
            EXVector dir(verts[idx + 1]);
            idx = (idx + 14) % (cnt * 2);
            p = CreateParticleWithDir(pSys, pos, dir, mtx);
        } else {
            EXVector pos(verts[idx]);
            p = BaseCreateParticle(pSys, pos, mtx);
            idx = (idx + 7) % cnt;
        }
        cnt--;
        if (!p) return;
    }
}

#define EX_RANDF_PM1() ((float)(int)(EXRandFast32() >> 1) * (1.0f / 1073741824.0f) - 1.0f)

void EXEmitter_NRNC::SetupParticle(EXBaseParticleSys *pSys, EXParticle_NRNC *p, const EXMatrix &mtx) const
{
    p->m_time = 0.0f;
    p->m_08 = 0;
    p->m_scaleRate.set_zero();
    float rx = EX_RANDF_PM1();
    float ry = EX_RANDF_PM1();
    float rz = EX_RANDF_PM1();
    EXVector v(pSys->m_7c.x + pSys->m_8c.x * rx, pSys->m_7c.y + pSys->m_8c.y * ry, pSys->m_7c.z + pSys->m_8c.z * rz, pSys->m_7c.w);
    p->m_pos = EXVector((const EXMaths::mat_44 &)mtx * v);
    const EXGeoParticle *g = pSys->m_pGeo;
    float life = EX_RANDF_PM1() * g->m_lifeVar + g->m_life;
    if (life <= 0.0f) life = g->m_life;
    p->m_life = 100.0f / life * pSys->m_rfps;
    p->m_scale = pSys->CalculateInitialSize();
    CalculateInitialColour(p, pSys);
    EXGeoParticle *gp = pSys->m_geoRef.GeoParticle();
    if ((u32)gp->m_entities.Size() > 0) p->m_hash = gp->m_entities[0];
    else p->m_hash = pSys->CalculateInitialTexture();
}
void EXEmitter_NR::SetupParticle(EXBaseParticleSys *pSys, EXParticle_NR *p, const EXMatrix &mtx) const
{
    p->m_time = 0.0f;
    p->m_08 = 0;
    p->m_colourRate.set_zero();
    p->m_scaleRate.set_zero();
    float rx = EX_RANDF_PM1();
    float ry = EX_RANDF_PM1();
    float rz = EX_RANDF_PM1();
    EXVector v(pSys->m_7c.x + pSys->m_8c.x * rx, pSys->m_7c.y + pSys->m_8c.y * ry, pSys->m_7c.z + pSys->m_8c.z * rz, pSys->m_7c.w);
    p->m_pos = EXVector((const EXMaths::mat_44 &)mtx * v) * pSys->m_globalScale;
    const EXGeoParticle *g = pSys->m_pGeo;
    float life = EX_RANDF_PM1() * g->m_lifeVar + g->m_life;
    if (life <= 0.0f) life = g->m_life;
    p->m_life = 100.0f / life * pSys->m_rfps;
    p->m_scale = pSys->CalculateInitialSize();
    CalculateInitialColour(p, pSys);
    EXGeoParticle *gp = pSys->m_geoRef.GeoParticle();
    if ((u32)gp->m_entities.Size() > 0) p->m_hash = gp->m_entities[0];
    else p->m_hash = pSys->CalculateInitialTexture();
}
void EXEmitter_NRNS::SetupParticle(EXBaseParticleSys *pSys, EXParticle_NRNS *p, const EXMatrix &mtx) const
{
    p->m_time = 0.0f;
    p->m_08 = 0;
    p->m_colourRate.set_zero();
    float rx = EX_RANDF_PM1();
    float ry = EX_RANDF_PM1();
    float rz = EX_RANDF_PM1();
    EXVector v(pSys->m_7c.x + pSys->m_8c.x * rx, pSys->m_7c.y + pSys->m_8c.y * ry, pSys->m_7c.z + pSys->m_8c.z * rz, pSys->m_7c.w);
    p->m_pos = EXVector((const EXMaths::mat_44 &)mtx * v) * pSys->m_globalScale;
    const EXGeoParticle *g = pSys->m_pGeo;
    float life = EX_RANDF_PM1() * g->m_lifeVar + g->m_life;
    if (life <= 0.0f) life = g->m_life;
    p->m_life = 100.0f / life * pSys->m_rfps;
    p->m_scale = pSys->CalculateInitialSize();
    CalculateInitialColour(p, pSys);
    EXGeoParticle *gp = pSys->m_geoRef.GeoParticle();
    if ((u32)gp->m_entities.Size() > 0) p->m_hash = gp->m_entities[0];
    else p->m_hash = pSys->CalculateInitialTexture();
}

EXParticle_NRNS *EXEmitter_NRNS::BaseCreateParticle(EXBaseParticleSys *pSys, const EXVector &pos, const EXMatrix &mtx)
{
    EXParticle_NRNS *p = GetFreeParticle(pSys);
    if (!p) return 0;
    SetupParticle(pSys, p, mtx);
    float yaw = pSys->m_yaw;
    float pitch = pSys->m_pitch;
    float speed = pSys->m_speed;
    if (pSys->m_yawVar != 0.0f) yaw = EX_RANDF_PM1() * pSys->m_yawVar + yaw;
    if (pSys->m_pitchVar != 0.0f) pitch = EX_RANDF_PM1() * pSys->m_pitchVar + pitch;
    float sp = (float)sin(pitch);
    EXVector dir((float)cos(yaw) * sp, (float)cos(pitch), (float)sin(yaw) * sp, 0.0f);
    if (pSys->m_speedVar != 0.0f) speed = EX_RANDF_PM1() * pSys->m_speedVar + speed;
    speed = pSys->m_emitterLife + speed;
    float vx = dir.x * speed, vy = dir.y * speed, vz = dir.z * speed, vw = dir.w * speed;
    if (pSys->m_bAddEmitterVel) {
        vx = pSys->m_emitterVel.x + vx;
        vy = pSys->m_emitterVel.y + vy;
        vz = pSys->m_emitterVel.z + vz;
        vw = pSys->m_emitterVel.w + vw;
    }
    p->m_vel = EXVector(vx, vy, vz, vw) * pSys->m_rfps;
    p->m_pos += pos;
    p->m_vel = EXVector((const EXMaths::mat_44 &)mtx * p->m_vel);
    if (pSys->m_bAddEmitterVel) p->m_vel += pSys->m_emitterVel * pSys->m_rfps;
    return p;
}

EXParticle_NRNC *EXEmitter_NRNC::BaseCreateParticle(EXBaseParticleSys *pSys, const EXVector &pos, const EXMatrix &mtx)
{
    EXParticle_NRNC *p = GetFreeParticle(pSys);
    if (!p) return 0;
    SetupParticle(pSys, p, mtx);
    float yaw = pSys->m_yaw;
    float pitch = pSys->m_pitch;
    float speed = pSys->m_speed;
    if (pSys->m_yawVar != 0.0f) yaw = EX_RANDF_PM1() * pSys->m_yawVar + yaw;
    if (pSys->m_pitchVar != 0.0f) pitch = EX_RANDF_PM1() * pSys->m_pitchVar + pitch;
    float sp = (float)sin(pitch);
    EXVector dir((float)cos(yaw) * sp, (float)cos(pitch), (float)sin(yaw) * sp, 0.0f);
    if (pSys->m_speedVar != 0.0f) speed = EX_RANDF_PM1() * pSys->m_speedVar + speed;
    speed = pSys->m_emitterLife + speed;
    float vx = dir.x * speed, vy = dir.y * speed, vz = dir.z * speed, vw = dir.w * speed;
    if (pSys->m_bAddEmitterVel) {
        vx = pSys->m_emitterVel.x + vx;
        vy = pSys->m_emitterVel.y + vy;
        vz = pSys->m_emitterVel.z + vz;
        vw = pSys->m_emitterVel.w + vw;
    }
    p->m_vel = EXVector(vx, vy, vz, vw) * pSys->m_rfps;
    p->m_pos += pos;
    p->m_vel = EXVector((const EXMaths::mat_44 &)mtx * p->m_vel);
    if (pSys->m_bAddEmitterVel) p->m_vel += pSys->m_emitterVel * pSys->m_rfps;
    return p;
}

EXParticle_NRNS *EXEmitter_NRNS::CreateParticleWithDir(EXBaseParticleSys *pSys, const EXVector &pos, const EXVector &dir, const EXMatrix &mtx)
{
    EXParticle_NRNS *p = GetFreeParticle(pSys);
    if (!p) return 0;
    SetupParticle(pSys, p, mtx);
    float speed = pSys->m_emitterLife + (EX_RANDF_PM1() * pSys->m_speedVar + pSys->m_speed);
    EXVector vel = dir * speed;
    float vx = vel.x, vy = vel.y, vz = vel.z, vw = vel.w;
    if (pSys->m_bAddEmitterVel) {
        vx = pSys->m_emitterVel.x + vel.x;
        vy = pSys->m_emitterVel.y + vel.y;
        vz = pSys->m_emitterVel.z + vel.z;
        vw = pSys->m_emitterVel.w + vel.w;
    }
    float r = pSys->m_rfps;
    EXVector v(vx * r, vy * r, vz * r, vw * r);
    p->m_vel = v;
    p->m_pos += pos;
    p->m_vel = EXVector((const EXMaths::mat_44 &)mtx * p->m_vel);
    if (!pSys->m_bAddEmitterVel) p->m_vel += pSys->m_emitterVel * pSys->m_rfps;
    return p;
}

EXParticle_NRNC *EXEmitter_NRNC::CreateParticleWithDir(EXBaseParticleSys *pSys, const EXVector &pos, const EXVector &dir, const EXMatrix &mtx)
{
    EXParticle_NRNC *p = GetFreeParticle(pSys);
    if (!p) return 0;
    SetupParticle(pSys, p, mtx);
    float speed = pSys->m_emitterLife + (EX_RANDF_PM1() * pSys->m_speedVar + pSys->m_speed);
    EXVector vel = dir * speed;
    float vx = vel.x, vy = vel.y, vz = vel.z, vw = vel.w;
    if (pSys->m_bAddEmitterVel) {
        vx = pSys->m_emitterVel.x + vel.x;
        vy = pSys->m_emitterVel.y + vel.y;
        vz = pSys->m_emitterVel.z + vel.z;
        vw = pSys->m_emitterVel.w + vel.w;
    }
    float r = pSys->m_rfps;
    EXVector v(vx * r, vy * r, vz * r, vw * r);
    p->m_vel = v;
    p->m_pos += pos;
    p->m_vel = EXVector((const EXMaths::mat_44 &)mtx * p->m_vel);
    if (!pSys->m_bAddEmitterVel) p->m_vel += pSys->m_emitterVel * pSys->m_rfps;
    return p;
}

void EXEmitter_NRNC::CreateParticles(const EXItemAnimator *pAnim, const EXGeoParticle *pGeo, EXBaseParticleSys *pSys)
{
    int avail = pSys->m_mips[0].m_particleLimit - pSys->m_numActive;
    if (avail <= 0) return;
    float f = pSys->m_frameScale * pSys->m_mips[0].m_emitsPerFrame + pSys->m_emitAccum;
    pSys->m_emitAccum = f;
    u32 n = (int)f;
    if (!n) return;
    EXParticlePosCallback cb = pSys->m_pPosCallback;
    if (!cb) {
        if (!(pAnim->m_itemFlags & 8)) {
            if (!pSys->m_geoRefEmitter.GeoFile()) {
                EXMatrix m = pAnim->m_mtx;
                pSys->m_lastEmitPos = m.r[3];
                m.GetRotationXYZ(pSys->m_10c);
                m.r[3].set(0.0f, 0.0f, 0.0f, 1.0f);
                for (u32 i = 0; i < n; i++)
                    if (!BaseCreateParticle(pSys, pSys->m_lastEmitPos, m)) break;
            } else {
                EXGeoEntity *e = pSys->m_geoRefEmitter.GeoFile()->GeoHeader()->GeoEntity(pSys->m_geoRefEmitter.HashRef());
                if (e) {
                    EXMatrix m = pAnim->m_mtx;
                    pSys->m_lastEmitPos = m.r[3];
                    m.GetRotationXYZ(pSys->m_10c);
                    ParticleInfoArray.Empty(0);
                    ParticleInfoArray.SetMatrix(m);
                    e->BuildEntityInfoArray(ParticleInfoArray, 0);
                    m.r[3].set(0.0f, 0.0f, 0.0f, 1.0f);
                    u32 flags = 0;
                    if (!*(const u32 *)&pAnim->m_scale.x) flags = 1;  // TODO: animator fields at +0xe8 / +0xec
                    if (*(const u16 *)&pAnim->m_scale.y) flags |= 2;
                    EmitParticles(pSys, m, n, flags);
                }
            }
        }
    } else {
        EXFixedArray<EXMaths::vec_xyzw, 64> buf;
        u32 k = 0;
        u32 cnt = 0;
        EXMatrix ident;
        ident.SetIdentity();
        for (u32 i = 0; i < n; i++) {
            if (!cnt) {
                u32 c = min((u32)64, (u32)avail);
                cnt = min(c, n);
                cb((EXItemAnimator *)pAnim, (EXVector *)&buf[0], cnt);
                k %= cnt;
                pSys->m_lastEmitPos = buf[0];
            }
            EXVector pos = buf[k];
            EXMatrix m = ident;
            if (!BaseCreateParticle(pSys, pos, m)) break;
            k = (k + 7) % cnt;
        }
    }
    pSys->m_emitAccum = 0.0f;
}

void EXEmitter_NRNS::CreateParticles(const EXItemAnimator *pAnim, const EXGeoParticle *pGeo, EXBaseParticleSys *pSys)
{
    int avail = pSys->m_mips[0].m_particleLimit - pSys->m_numActive;
    if (avail <= 0) return;
    float f = pSys->m_frameScale * pSys->m_mips[0].m_emitsPerFrame + pSys->m_emitAccum;
    pSys->m_emitAccum = f;
    u32 n = (int)f;
    if (!n) return;
    EXParticlePosCallback cb = pSys->m_pPosCallback;
    if (!cb) {
        if (!(pAnim->m_itemFlags & 8)) {
            if (!pSys->m_geoRefEmitter.GeoFile()) {
                EXMatrix m = pAnim->m_mtx;
                EXVector pos = m.r[3];
                EXVector rot;
                m.GetRotationXYZ(rot);
                pSys->m_10c = rot;
                pSys->m_lastEmitPos = pos;
                m.r[3].set(0.0f, 0.0f, 0.0f, 1.0f);
                for (u32 i = 0; i < n; i++)
                    if (!BaseCreateParticle(pSys, pos, m)) break;
            } else {
                EXGeoEntity *e = pSys->m_geoRefEmitter.GeoFile()->GeoHeader()->GeoEntity(pSys->m_geoRefEmitter.HashRef());
                if (e) {
                    EXMatrix m = pAnim->m_mtx;
                    pSys->m_lastEmitPos = m.r[3];
                    EXVector rot;
                    m.GetRotationXYZ(rot);
                    pSys->m_10c = rot;
                    ParticleInfoArray.Empty(0);
                    ParticleInfoArray.SetMatrix(m);
                    e->BuildEntityInfoArray(ParticleInfoArray, 0);
                    m.r[3].set(0.0f, 0.0f, 0.0f, 1.0f);
                    u32 flags = 0;
                    if (!*(const u32 *)&pAnim->m_scale.x) flags = 1;  // TODO: animator fields at +0xe8 / +0xec
                    if (*(const u16 *)&pAnim->m_scale.y) flags |= 2;
                    EmitParticles(pSys, m, n, flags);
                }
            }
        }
    } else {
        EXFixedArray<EXMaths::vec_xyzw, 64> buf;
        u32 k = 0;
        u32 cnt = 0;
        EXMatrix ident;
        ident.SetIdentity();
        for (u32 i = 0; i < n; i++) {
            if (!cnt) {
                u32 c = min((u32)64, (u32)avail);
                cnt = min(c, n);
                cb((EXItemAnimator *)pAnim, (EXVector *)&buf[0], cnt);
                k %= cnt;
                pSys->m_lastEmitPos = buf[0];
            }
            EXVector pos = buf[k];
            EXMatrix m = ident;
            if (!BaseCreateParticle(pSys, pos, m)) break;
            k = (k + 7) % cnt;
        }
    }
    pSys->m_emitAccum = 0.0f;
}

void EXEmitter_NR::CreateParticles(const EXItemAnimator *pAnim, const EXGeoParticle *pGeo, EXBaseParticleSys *pSys)
{
    int avail = pSys->m_mips[0].m_particleLimit - pSys->m_numActive;
    if (avail <= 0) return;
    float f = pSys->m_frameScale * pSys->m_mips[0].m_emitsPerFrame + pSys->m_emitAccum;
    pSys->m_emitAccum = f;
    u32 n = (int)f;
    if (!n) return;
    EXParticlePosCallback cb = pSys->m_pPosCallback;
    if (!cb) {
        if (!(pAnim->m_itemFlags & 8)) {
            if (!pSys->m_geoRefEmitter.GeoFile()) {
                EXMatrix m = pAnim->m_mtx;
                EXVector pos = m.r[3];
                EXVector rot;
                m.GetRotationXYZ(rot);
                pSys->m_10c = rot;
                pSys->m_lastEmitPos = pos;
                m.r[3].set(0.0f, 0.0f, 0.0f, 1.0f);
                for (u32 i = 0; i < n; i++)
                    if (!BaseCreateParticle(pSys, pos, m)) break;
            } else {
                EXGeoEntity *e = pSys->m_geoRefEmitter.GeoFile()->GeoHeader()->GeoEntity(pSys->m_geoRefEmitter.HashRef());
                if (e) {
                    EXMatrix m = pAnim->m_mtx;
                    pSys->m_lastEmitPos = m.r[3];
                    EXVector rot;
                    m.GetRotationXYZ(rot);
                    pSys->m_10c = rot;
                    ParticleInfoArray.Empty(0);
                    ParticleInfoArray.SetMatrix(m);
                    e->BuildEntityInfoArray(ParticleInfoArray, 0);
                    m.r[3].set(0.0f, 0.0f, 0.0f, 1.0f);
                    u32 flags = 0;
                    if (!*(const u32 *)&pAnim->m_scale.x) flags = 1;  // TODO: animator fields at +0xe8 / +0xec
                    if (*(const u16 *)&pAnim->m_scale.y) flags |= 2;
                    EmitParticles(pSys, m, n, flags);
                }
            }
        }
    } else {
        EXFixedArray<EXMaths::vec_xyzw, 64> buf;
        u32 k = 0;
        u32 cnt = 0;
        EXMatrix ident;
        ident.SetIdentity();
        for (u32 i = 0; i < n; i++) {
            if (!cnt) {
                u32 c = min((u32)64, (u32)avail);
                cnt = min(c, n);
                cb((EXItemAnimator *)pAnim, (EXVector *)&buf[0], cnt);
                k %= cnt;
                pSys->m_lastEmitPos = buf[0];
            }
            EXVector pos = buf[k];
            EXMatrix m = ident;
            if (!BaseCreateParticle(pSys, pos, m)) break;
            k = (k + 7) % cnt;
        }
    }
    pSys->m_emitAccum = 0.0f;
}

EXParticle_NR *EXEmitter_NR::BaseCreateParticle(EXBaseParticleSys *pSys, const EXVector &pos, const EXMatrix &mtx)
{
    EXParticle_NR *p = GetFreeParticle(pSys);
    if (!p) return 0;
    const EXGeoParticle *g = pSys->m_pGeo;
    p->m_time = 0.0f;
    p->m_08 = 0;
    p->m_colourRate.set_zero();
    p->m_scaleRate.set_zero();
    float rx = EX_RANDF_PM1();
    float ry = EX_RANDF_PM1();
    float rz = EX_RANDF_PM1();
    EXVector v(pSys->m_8c.x * rx, pSys->m_8c.y * ry, pSys->m_8c.z * rz, 0.0f);
    v = EXVector(g->m_emitOffset.x + v.x, g->m_emitOffset.y + v.y, g->m_emitOffset.z + v.z, 0.0f);
    (EXMaths::vec_xyzw &)v = (const EXMaths::mat_44 &)mtx * v;
    p->m_pos = v * pSys->m_globalScale;
    float yaw = pSys->m_yaw;
    float pitch = pSys->m_pitch;
    float speed = pSys->m_speed;
    if (pSys->m_yawVar != 0.0f) yaw = EX_RANDF_PM1() * pSys->m_yawVar + yaw;
    if (pSys->m_pitchVar != 0.0f) pitch = EX_RANDF_PM1() * pSys->m_pitchVar + pitch;
    float sp = (float)sin(pitch);
    EXVector dir((float)cos(yaw) * sp, (float)cos(pitch), (float)sin(yaw) * sp, 0.0f);
    if (pSys->m_speedVar != 0.0f) speed = EX_RANDF_PM1() * pSys->m_speedVar + speed;
    speed = pSys->m_emitterLife + speed;
    float vx = dir.x * speed, vy = dir.y * speed, vz = dir.z * speed, vw = dir.w * speed;
    if (pSys->m_bAddEmitterVel) {
        vx = pSys->m_emitterVel.x + vx;
        vy = pSys->m_emitterVel.y + vy;
        vz = pSys->m_emitterVel.z + vz;
        vw = pSys->m_emitterVel.w + vw;
    }
    p->m_vel = EXVector(vx, vy, vz, vw) * pSys->m_rfps;
    p->m_pos += pos;
    (EXMaths::vec_xyzw &)p->m_vel = (const EXMaths::mat_44 &)mtx * p->m_vel;
    if (pSys->m_bAddEmitterVel) p->m_vel += pSys->m_emitterVel * pSys->m_rfps;
    float life = EX_RANDF_PM1() * g->m_lifeVar + g->m_life;
    if (life <= 0.0f) life = g->m_life;
    p->m_life = 100.0f / life * pSys->m_rfps;
    p->m_scale = pSys->CalculateInitialSize();
    CalculateInitialColour(p, pSys);
    if ((u32)g->m_entities.Size() > 0) p->m_hash = g->m_entities[0];
    else p->m_hash = pSys->CalculateInitialTexture();
    return p;
}

EXParticle_NR *EXEmitter_NR::CreateParticleWithDir(EXBaseParticleSys *pSys, const EXVector &pos, const EXVector &dir, const EXMatrix &mtx)
{
    EXParticle_NR *p = GetFreeParticle(pSys);
    if (!p) return 0;
    {
    p->m_time = 0.0f;
    p->m_08 = 0;
    p->m_colourRate.set_zero();
    p->m_scaleRate.set_zero();
    float rx = EX_RANDF_PM1();
    float ry = EX_RANDF_PM1();
    float rz = EX_RANDF_PM1();
    EXVector v(pSys->m_7c.x + pSys->m_8c.x * rx, pSys->m_7c.y + pSys->m_8c.y * ry, pSys->m_7c.z + pSys->m_8c.z * rz, pSys->m_7c.w);
    p->m_pos = EXVector((const EXMaths::mat_44 &)mtx * v) * pSys->m_globalScale;
    const EXGeoParticle *g = pSys->m_pGeo;
    float life = EX_RANDF_PM1() * g->m_lifeVar + g->m_life;
    if (life <= 0.0f) life = g->m_life;
    p->m_life = 100.0f / life * pSys->m_rfps;
    p->m_scale = pSys->CalculateInitialSize();
    CalculateInitialColour(p, pSys);
    EXGeoParticle *gp = pSys->m_geoRef.GeoParticle();
    if ((u32)gp->m_entities.Size() > 0) p->m_hash = gp->m_entities[0];
    else p->m_hash = pSys->CalculateInitialTexture();
    }
    float speed = pSys->m_emitterLife + (EX_RANDF_PM1() * pSys->m_speedVar + pSys->m_speed);
    EXVector vel = dir * speed;
    float vx = vel.x, vy = vel.y, vz = vel.z, vw = vel.w;
    if (pSys->m_bAddEmitterVel) {
        vx = pSys->m_emitterVel.x + vel.x;
        vy = pSys->m_emitterVel.y + vel.y;
        vz = pSys->m_emitterVel.z + vel.z;
        vw = pSys->m_emitterVel.w + vel.w;
    }
    float r = pSys->m_rfps;
    EXVector v(vx * r, vy * r, vz * r, vw * r);
    p->m_vel = v;
    p->m_pos += pos;
    p->m_vel = EXVector((const EXMaths::mat_44 &)mtx * p->m_vel);
    if (!pSys->m_bAddEmitterVel) p->m_vel += pSys->m_emitterVel * pSys->m_rfps;
    return p;
}
