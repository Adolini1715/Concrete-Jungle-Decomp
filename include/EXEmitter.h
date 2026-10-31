#ifndef EXEMITTER_H
#define EXEMITTER_H
#include "types.h"
#include "EXMaths.h"
#include "EXVector.h"

class EXBaseParticleSys;
class EXGeoParticle;
class EXItemAnimator;
class EXVector;
class EXMatrix;

// Particle emitter strategy owned by an EXBaseParticleSys (EXParticleEmitter.obj).
// TODO layout: fields are filled in with EXParticleEmitter.
class EXEmitter {
public:
    virtual ~EXEmitter();
    virtual s8 Initialise(EXBaseParticleSys *pSys, const EXGeoParticle *pGeo);
    virtual s8 Update(EXBaseParticleSys *pSys, const EXItemAnimator *pAnim, const EXGeoParticle *pGeo);
    virtual void Release();
    EXEmitter();
    float CalculateEmittedLife(const EXGeoParticle *pGeo, const EXBaseParticleSys *pSys) const;
    float CalculateEmittedVelocity(const EXGeoParticle *pGeo, const EXBaseParticleSys *pSys) const;
    void CalculateEmitterColour(const EXGeoParticle *pGeo, float t, EXVector *pOut) const;
    void StandardEmitterUpdate(const EXGeoParticle *pGeo, EXBaseParticleSys *pSys) const;
    float CalculateGradientColourTime(EXBaseParticleSys *pSys) const;

    u32 m_particleBytes;    // +0x04
    u32 m_08;               // +0x08 maxParticles * 0x2c
};

// Per-emitter particle records (TODO layout beyond the common head).
class EXParticle_NR {
public:
    s8 IsActive() const
    {
        if (m_life != 0.0f) return 1;
        return 0;
    }
    void SetHashCode(u32 h) { m_hash = h; }
    float m_life;           // +0x00 key slope scale
    float m_time;           // +0x04
    u32 m_08;               // +0x08 next key
    u32 m_rgba;             // +0x0c packed colour (r in the low byte)
    EXVector m_pos;         // +0x10
    EXVector m_vel;         // +0x20
    EXVector m_scale;       // +0x30
    EXVector m_scaleRate;   // +0x40
    EXVector m_colour;      // +0x50
    EXVector m_colourRate;  // +0x60
    u32 m_hash;             // +0x70
};
class EXParticle_NRNS {
public:
    s8 IsActive() const
    {
        if (m_life != 0.0f) return 1;
        return 0;
    }
    void SetHashCode(u32 h) { m_hash = h; }
    float m_life;
    float m_time;
    u32 m_08;
    u32 m_rgba;             // +0x0c packed colour (r in the low byte)
    EXVector m_pos;         // +0x10
    EXVector m_vel;         // +0x20
    EXVector m_colour;      // +0x30
    EXVector m_colourRate;  // +0x40
    EXVector m_scale;       // +0x50 set once from the keys
    u32 m_hash;             // +0x60
};
class EXParticle_NRNC {
public:
    s8 IsActive() const
    {
        if (m_life != 0.0f) return 1;
        return 0;
    }
    void SetHashCode(u32 h) { m_hash = h; }
    float m_life;
    float m_time;
    u32 m_08;
    u32 m_rgba;             // +0x0c packed colour (r in the low byte)
    EXVector m_pos;         // +0x10
    EXVector m_vel;         // +0x20
    EXVector m_scale;       // +0x30
    EXVector m_scaleRate;   // +0x40
    EXVector m_colour;      // +0x50 fixed colour from the keys
    u32 m_hash;             // +0x60
};

// Emitter variants: NR = no rotation, NS = no scale, NC = no colour.
class EXEmitter_NR : public EXEmitter {
public:
    virtual ~EXEmitter_NR();
    virtual s8 Initialise(EXBaseParticleSys *pSys, const EXGeoParticle *pGeo);
    virtual s8 Update(EXBaseParticleSys *pSys, const EXItemAnimator *pAnim, const EXGeoParticle *pGeo);
    virtual void Release();
    EXEmitter_NR();
    EXParticle_NR *GetFreeParticle(EXBaseParticleSys *pSys);
    void BaseUpdateParticleLifetime(EXParticle_NR *p, const EXGeoParticle *pGeo, const EXBaseParticleSys *pSys) const;
    void CreateParticles(const EXItemAnimator *pAnim, const EXGeoParticle *pGeo, EXBaseParticleSys *pSys);
    u32 UpdateParticles(EXBaseParticleSys *pSys, const EXGeoParticle *pGeo);
    void BaseUpdateParticle(EXParticle_NR *p, const EXBaseParticleSys *pSys) const;
    void BaseUpdateParticleWithFade(EXParticle_NR *p, EXBaseParticleSys *pSys) const;
    void CalculateInitialColour(EXParticle_NR *p, EXBaseParticleSys *pSys) const;
    void SetupParticle(EXBaseParticleSys *pSys, EXParticle_NR *p, const EXMatrix &mtx) const;
    EXParticle_NR *BaseCreateParticle(EXBaseParticleSys *pSys, const EXVector &pos, const EXMatrix &mtx);
    EXParticle_NR *CreateParticleWithDir(EXBaseParticleSys *pSys, const EXVector &pos, const EXVector &dir, const EXMatrix &mtx);
    void EmitParticles(EXBaseParticleSys *pSys, const EXMatrix &mtx, u32 n, u32 flags);
    EXParticle_NR *m_pParticles;    // +0x0c
};
class EXEmitter_NRNS : public EXEmitter {
public:
    virtual ~EXEmitter_NRNS();
    virtual s8 Initialise(EXBaseParticleSys *pSys, const EXGeoParticle *pGeo);
    virtual s8 Update(EXBaseParticleSys *pSys, const EXItemAnimator *pAnim, const EXGeoParticle *pGeo);
    virtual void Release();
    EXEmitter_NRNS();
    EXParticle_NRNS *GetFreeParticle(EXBaseParticleSys *pSys);
    void BaseUpdateParticleLifetime(EXParticle_NRNS *p, const EXGeoParticle *pGeo, const EXBaseParticleSys *pSys) const;
    void CreateParticles(const EXItemAnimator *pAnim, const EXGeoParticle *pGeo, EXBaseParticleSys *pSys);
    u32 UpdateParticles(EXBaseParticleSys *pSys, const EXGeoParticle *pGeo);
    void BaseUpdateParticle(EXParticle_NRNS *p, const EXBaseParticleSys *pSys) const;
    void BaseUpdateParticleWithFade(EXParticle_NRNS *p, EXBaseParticleSys *pSys) const;
    void CalculateInitialColour(EXParticle_NRNS *p, EXBaseParticleSys *pSys) const;
    void SetupParticle(EXBaseParticleSys *pSys, EXParticle_NRNS *p, const EXMatrix &mtx) const;
    EXParticle_NRNS *BaseCreateParticle(EXBaseParticleSys *pSys, const EXVector &pos, const EXMatrix &mtx);
    EXParticle_NRNS *CreateParticleWithDir(EXBaseParticleSys *pSys, const EXVector &pos, const EXVector &dir, const EXMatrix &mtx);
    void EmitParticles(EXBaseParticleSys *pSys, const EXMatrix &mtx, u32 n, u32 flags);
    EXParticle_NRNS *m_pParticles;  // +0x0c
};
class EXEmitter_NRNC : public EXEmitter {
public:
    virtual ~EXEmitter_NRNC();
    virtual s8 Initialise(EXBaseParticleSys *pSys, const EXGeoParticle *pGeo);
    virtual s8 Update(EXBaseParticleSys *pSys, const EXItemAnimator *pAnim, const EXGeoParticle *pGeo);
    virtual void Release();
    EXEmitter_NRNC();
    EXParticle_NRNC *GetFreeParticle(EXBaseParticleSys *pSys);
    void BaseUpdateParticleLifetime(EXParticle_NRNC *p, const EXGeoParticle *pGeo, const EXBaseParticleSys *pSys) const;
    void CreateParticles(const EXItemAnimator *pAnim, const EXGeoParticle *pGeo, EXBaseParticleSys *pSys);
    u32 UpdateParticles(EXBaseParticleSys *pSys, const EXGeoParticle *pGeo);
    void BaseUpdateParticle(EXParticle_NRNC *p, const EXBaseParticleSys *pSys) const;
    void BaseUpdateParticleWithFade(EXParticle_NRNC *p, EXBaseParticleSys *pSys) const;
    void CalculateInitialColour(EXParticle_NRNC *p, EXBaseParticleSys *pSys) const;
    void SetupParticle(EXBaseParticleSys *pSys, EXParticle_NRNC *p, const EXMatrix &mtx) const;
    EXParticle_NRNC *BaseCreateParticle(EXBaseParticleSys *pSys, const EXVector &pos, const EXMatrix &mtx);
    EXParticle_NRNC *CreateParticleWithDir(EXBaseParticleSys *pSys, const EXVector &pos, const EXVector &dir, const EXMatrix &mtx);
    void EmitParticles(EXBaseParticleSys *pSys, const EXMatrix &mtx, u32 n, u32 flags);
    EXParticle_NRNC *m_pParticles;  // +0x0c
};
#endif
