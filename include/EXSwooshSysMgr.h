#ifndef EXSWOOSHSYSMGR_H
#define EXSWOOSHSYSMGR_H
#include "EXEffectsSys.h"
#include "EXArray.h"

class EXItem;
class EXItemAnimator_Entity;
class EXSwooshSys;
class EXVector;

struct EXSwooshSysData {
    EXSwooshSys *m_pSys;
};

// Swoosh (trail) system manager (EXSwoosh.obj), sizeof 0x18.
class EXSwooshSysMgr : public EXEffectsSysMgr {
public:
    virtual ~EXSwooshSysMgr();
    virtual s8 DeleteSystem(const EXEffectsSys *pSys);
protected:
    virtual void RemoveSystem(const EXEffectsSys *pSys);
public:
    virtual s8 DetachSystem(const EXEffectsSys *pSys);
    EXSwooshSysMgr();
    void StopAllSystems(s8 stop);
    void PauseAllSystems(s8 pause);
    void DeleteAllSystems();
    void DeleteAllSystems(const EXItem *pItem);
    void KillAllSystems(const EXItem *pItem);
    void KillAllSystems(const EXItemAnimator_Entity *pEntity);
    void DetatchSwoosh(const EXItem *pItem);
    void DetatchSwoosh(const EXItemAnimator_Entity *pEntity);
    void Draw();
    EXSwooshSys *FindSwoosh(u32 hash, const EXItem *pItem);
    EXSwooshSys *FindSwoosh(u32 hash, const EXItemAnimator_Entity *pEntity);
    EXSwooshSys *FindSwooshByID(u32 id, const EXItem *pItem);
    void UpdatePosition(const EXVector &pos, EXSwooshSys *pSys, u32 hash) const;
    void SetGlobalAlpha(float a);
    EXSwooshSys *CreateSystem(u32 type);
    void Update();

    EXDataArray<EXSwooshSysData, EXSwooshSysData &> m_systems;  // +0x04
    s8 m_bPause;                // +0x14
    s8 m_bStop;                 // +0x15
};
#endif
