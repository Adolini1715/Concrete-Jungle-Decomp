#include "EXEffectsSys.h"
#include "EXEffectsSystems.h"
#include "EXParticleSysMgr.h"
#include "EXParticleSys.h"
#include "EXSwooshSysMgr.h"
#include "EXBaseDisplay.h"
#include "EXWnd.h"
#include "EXGeoFile.h"
#include "EXGeoParticle.h"
#include "EXMemory.h"
#include <string.h>

EX_IMPLEMENT_DYNAMIC_ROOT(EXEffectsSys)

EXEffectsSys::~EXEffectsSys()
{
    Remove();
}

void EXEffectsSys::UpdateRenderInfo(const EXRenderInfo &info)
{
    memcpy(&m_renderInfo, &info, sizeof(EXRenderInfo));
}

void EXEffectsSysTag::DetatchSystems(const EXRuntimeClass *pClass) const
{
    EXEffectsSys *p = First();
    while (p) {
        EXEffectsSys *pNext = Next(p);
        if (p->IsKindOf(pClass)) p->SystemMgr()->DetatchSystem(p);
        p = pNext;
    }
}

void EXEffectsSysTag::DetatchSystems() const
{
    EXEffectsSys *p;
    while ((p = First()) != 0) {
        p->SystemMgr()->DetatchSystem(p);
        p->Remove();
    }
}

void EXEffectsSysTag::DeleteSystems(const EXRuntimeClass *pClass) const
{
    EXEffectsSys *p = First();
    while (p) {
        EXEffectsSys *pNext = Next(p);
        if (p->IsKindOf(pClass)) {
            if (!p->SystemMgr()->DeleteSystem(p)) p->Remove();
        }
        p = pNext;
    }
}

void EXEffectsSysTag::DeleteSystems() const
{
    EXEffectsSys *p;
    while ((p = First()) != 0) {
        if (!p->SystemMgr()->DeleteSystem(p)) p->Remove();
    }
}

EXEffectsSys *EXEffectsSysTag::FindSystem(const EXRuntimeClass *pClass) const
{
    EXEffectsSys *p = First();
    while (p) {
        if (p->IsKindOf(pClass)) break;
        p = Next(p);
    }
    return p;
}

void EXEffectsSysTag::UpdateRenderInfo(const EXRenderInfo &info) const
{
    for (EXEffectsSys *p = First(); p; p = Next(p)) p->UpdateRenderInfo(info);
}

void EXEffectsSysMgr::RemoveSystem(const EXEffectsSys *pSys)
{
    for (;;) {}
}

s8 EXEffectsSysMgr::DetatchSystem(const EXEffectsSys *pSys)
{
    for (;;) {}
}

s8 EXEffectsSysMgr::DeleteSystem(const EXEffectsSys *pSys)
{
    for (;;) {}
}

EXEffectsSystems::EXEffectsSystems()
{
    m_pParticleSysMgr = 0;
    m_pSwooshSysMgr = 0;
}

void EXEffectsSystems::DeleteMem()
{
    if (m_pParticleSysMgr) {
        delete m_pParticleSysMgr;
        m_pParticleSysMgr = 0;
    }
    if (m_pSwooshSysMgr) {
        delete m_pSwooshSysMgr;
        m_pSwooshSysMgr = 0;
    }
}

void EXEffectsSystems::Update()
{
    if (m_pParticleSysMgr) m_pParticleSysMgr->Update();
    if (m_pSwooshSysMgr) m_pSwooshSysMgr->Update();
}

EXParticleSysMgr *EXEffectsSystems::ParticleSysMgr()
{
    if (!m_pParticleSysMgr) return m_pParticleSysMgr = new EXParticleSysMgr;
    return m_pParticleSysMgr;
}

void EXEffectsSystems::DeleteParticleSysMem()
{
    if (m_pParticleSysMgr) {
        delete m_pParticleSysMgr;
        m_pParticleSysMgr = 0;
    }
}

void EXEffectsSystems::DeleteAllParticleSystems()
{
    if (m_pParticleSysMgr) m_pParticleSysMgr->DeleteAllSystems();
}

void EXEffectsSystems::StopParticleUpdateAndRender(s8 stop)
{
    if (m_pParticleSysMgr) m_pParticleSysMgr->StopAllSystems(stop);
}

void EXEffectsSystems::PauseParticleSystems(s8 pause)
{
    if (m_pParticleSysMgr) m_pParticleSysMgr->PauseAllSystems(pause);
}

EXSwooshSysMgr *EXEffectsSystems::SwooshSysMgr()
{
    if (!m_pSwooshSysMgr) return m_pSwooshSysMgr = new EXSwooshSysMgr;
    return m_pSwooshSysMgr;
}

void EXEffectsSystems::DeleteSwooshSysMem()
{
    if (m_pSwooshSysMgr) {
        delete m_pSwooshSysMgr;
        m_pSwooshSysMgr = 0;
    }
}

void EXEffectsSystems::DeleteAllSwooshSystems()
{
    if (m_pSwooshSysMgr) m_pSwooshSysMgr->DeleteAllSystems();
}

void EXEffectsSystems::StopSwooshUpdateAndRender(s8 stop)
{
    if (m_pSwooshSysMgr) m_pSwooshSysMgr->StopAllSystems(stop);
}

void EXEffectsSystems::PauseSwooshSystems(s8 pause)
{
    if (m_pSwooshSysMgr) m_pSwooshSysMgr->PauseAllSystems(pause);
}

EXEffectsSys::EXEffectsSys()
{
    Init();
    m_pMgr = 0;
}

EXEffectsSysTag::~EXEffectsSysTag()
{
    if (First()) DetatchSystems();
}

EXEffectsSystems::~EXEffectsSystems()
{
    if (m_pParticleSysMgr) {
        delete m_pParticleSysMgr;
        m_pParticleSysMgr = 0;
    }
    if (m_pSwooshSysMgr) {
        delete m_pSwooshSysMgr;
        m_pSwooshSysMgr = 0;
    }
}

EXEffectsSys *EXEffectsSysTag::AttachParticleSys(EXItemAnimator *pAnim, EXGeoFile *pFile, u32 hash, const EXRuntimeClass *pClass)
{
    EXWnd *pWnd = EXBaseDisplay::ActiveWnd();
    if (!pWnd) return 0;
    EXEffectsSystems *pSystems = pWnd->ActiveEffectsSystem();
    if (!pSystems) return 0;
    EXParticleSysMgr *pMgr = pSystems->ParticleSysMgr();
    EXParticleSys *pOld = pMgr->ParticleSys(pAnim, hash);
    if (pOld) pMgr->DetatchSystem(pOld);
    u32 type = (u32)-1;
    EXGeoHeader *pHdr = pFile->GeoHeader();
    if (pHdr) {
        EXGeoParticle *pGeo = pHdr->GeoParticle(hash);
        if (pGeo) type = pGeo->m_type;
    }
    EXParticleSys *pSys = pMgr->CreateSystem(type, pClass, pAnim);
    if (!pSys) return 0;
    if (!pSys->Initialize(pFile, hash)) {
        pMgr->DeleteSystem(pSys);
        return 0;
    }
    Add(pSys);
    return pSys;
}
