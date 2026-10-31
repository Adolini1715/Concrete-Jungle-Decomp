#ifndef EXEFFECTSSYSTEMS_H
#define EXEFFECTSSYSTEMS_H
#include "types.h"

class EXParticleSysMgr;
class EXSwooshSysMgr;

// Per-window pair of effects managers, created on demand (EXEffectsSys.obj; owned by EXBaseWnd).
class EXEffectsSystems {
public:
    EXEffectsSystems();
    ~EXEffectsSystems();
    void DeleteMem();
    void Update();
    EXParticleSysMgr *ParticleSysMgr();
    void DeleteParticleSysMem();
    void DeleteAllParticleSystems();
    void StopParticleUpdateAndRender(s8 stop);
    void PauseParticleSystems(s8 pause);
    EXSwooshSysMgr *SwooshSysMgr();
    void DeleteSwooshSysMem();
    void DeleteAllSwooshSystems();
    void StopSwooshUpdateAndRender(s8 stop);
    void PauseSwooshSystems(s8 pause);

    EXParticleSysMgr *m_pParticleSysMgr;   // +0
    EXSwooshSysMgr *m_pSwooshSysMgr;       // +4
};
#endif
