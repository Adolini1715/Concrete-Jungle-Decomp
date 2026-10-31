#ifndef EXPARTICLESYSMGR_H
#define EXPARTICLESYSMGR_H
#include "EXEffectsSys.h"
#include "EXArray.h"
#include "EXVector.h"

class EXParticleSys;
class EXItemAnimator;

// One live system and the animator that owns it.
struct EXParticleSysData {
    EXItemAnimator *m_pAnim;
    EXParticleSys *m_pSys;
};
// Game-registered particle system class for an emitter type id.
struct EXCustomPSysData {
    u32 m_id;
    EXRuntimeClass *m_pClass;
};

// Particle system manager (EXParticle.obj), sizeof 0x38.
class EXParticleSysMgr : public EXEffectsSysMgr {
public:
    virtual ~EXParticleSysMgr();
    virtual s8 DetatchSystem(const EXEffectsSys *pSys);
    virtual s8 DeleteSystem(const EXEffectsSys *pSys);
protected:
    virtual void RemoveSystem(const EXEffectsSys *pSys);
public:
    EXParticleSysMgr();
    s8 StopSystems() const { return m_bStop; }
    int SystemCount() const { return m_systems.Size(); }
    EXParticleSysData *ParticleSysData(int i) { return &m_systems.m_pData[i]; }
    void SetParticleManagerFade(const EXVector &fade);
    void StopAllSystems(s8 stop);
    void PauseAllSystems(s8 pause);
    EXParticleSys *ParticleSys(const EXItemAnimator *pAnim) const;
    EXParticleSys *ParticleSys(const EXItemAnimator *pAnim, u32 hash) const;
    void DeleteAllSystems();
    void RegisterCustomSystem(u32 id, EXRuntimeClass *pClass);
    EXParticleSys *CreateSystem(u32 id, const EXRuntimeClass *pClass, EXItemAnimator *pAnim);
    void Update();
    void Draw(int pass);
protected:
    EXRuntimeClass *FindCustomSystem(u32 id);
public:
    EXDataArray<EXParticleSysData, EXParticleSysData &> m_systems;  // +0x04
    EXDataArray<EXCustomPSysData, EXCustomPSysData &> m_custom;     // +0x14
    EXVector m_fade;        // +0x24
    s8 m_34;                // +0x34
    s8 m_bFade;             // +0x35
    s8 m_bPause;            // +0x36
    s8 m_bStop;             // +0x37
};
#endif
