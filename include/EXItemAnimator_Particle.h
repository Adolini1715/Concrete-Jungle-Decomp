#ifndef EXITEMANIMATOR_PARTICLE_H
#define EXITEMANIMATOR_PARTICLE_H
#include "EXItemAnimator.h"
#include "EXEffectsSys.h"

class EXParticleSys;

// Animator owning a particle system (sizeof 0x110).
class EXItemAnimator_Particle : public EXItemAnimator {
    EX_DECLARE_DYNAMIC(EXItemAnimator_Particle)
public:
    virtual ~EXItemAnimator_Particle();
    EX_DECLARE_ITEMTYPE(EXItemAnimator_Particle)
    virtual u32 DoRender(const EXMatrix &mtx, const EXRenderInfo &info, u32 flags);
    virtual s8 Initialise(EXGeoFile *pFile, u32 hash);
    virtual void EnableParticleSystems(s8 bEnable) const;
    EXItemAnimator_Particle();
    static EXItemAnimator_Particle *CreateObject();
    static EXItemAnimator_Particle *NewItem() { return new EXItemAnimator_Particle; }
    s8 InitialiseCustom(EXGeoFile *pFile, u32 hash, const EXRuntimeClass *pClass);
    void DeleteParticleSys() const;
    EXParticleSys *ParticleSys() const;

    EXEffectsSysTag m_tag;  // +0x104
    s8 m_bRendered;         // +0x10c
};
#endif
