#include "EXItemAnimator_Particle.h"
#include "EXParticleSys.h"
#include "EXBaseRenderEnv.h"
#include "EXMemory.h"

EX_IMPLEMENT_DYNCREATE(EXItemAnimator_Particle, EXItemAnimator)

EXItemAnimator_Particle::~EXItemAnimator_Particle() {}

s8 EXItemAnimator_Particle::InitialiseCustom(EXGeoFile *pFile, u32 hash, const EXRuntimeClass *pClass)
{
    if (GeoFile()) {
        if (GeoFile() == pFile && RefObject() == hash) return 1;
        m_tag.DetatchSystems();
    }
    m_geoRef.Set(pFile, hash);
    m_bRendered = 0;
    EXEffectsSys *pSys = m_tag.AttachParticleSys(this, pFile, hash, pClass);
    if (!pSys) return 0;
    static_cast<EXBaseParticleSys *>(pSys)->m_1ef = 0;
    return 1;
}

void EXItemAnimator_Particle::DeleteParticleSys() const
{
    m_tag.DeleteSystems(EXRUNTIME_CLASS(EXParticleSys));
}

EXParticleSys *EXItemAnimator_Particle::ParticleSys() const
{
    if (!m_tag.m_pHead) return 0;
    return (EXParticleSys *)m_tag.FindSystem(EXRUNTIME_CLASS(EXParticleSys));
}

void EXItemAnimator_Particle::EnableParticleSystems(s8 bEnable) const
{
    EXParticleSys *pSys = ParticleSys();
    if (pSys) pSys->m_bEnabled = bEnable;
}

EXItemAnimator_Particle::EXItemAnimator_Particle()
{
    m_bRendered = 0;
    m_geoRef.Set(0, 0);
}

s8 EXItemAnimator_Particle::Initialise(EXGeoFile *pFile, u32 hash)
{
    return InitialiseCustom(pFile, hash, 0);
}

u32 EXItemAnimator_Particle::DoRender(const EXMatrix &parent, const EXRenderInfo &info, u32 flags)
{
    if (EXBaseRenderEnv::RenderState() & 1) CalcMatrix_InPlace(parent, flags);
    u32 result = 0;
    if (EXBaseRenderEnv::RenderState() & 2) {
        if (EXBaseRenderEnv::ShadowRender()) return 0;
        EXParticleSys *pSys = ParticleSys();
        if (pSys) {
            EXRenderInfo ri(m_renderInfo, info);
            pSys->UpdateRenderInfo(ri);
            m_bRendered = 1;
            result = 1;
        }
    }
    const EXMatrix &m = mtx();
    return RenderNodes(m, info, flags) | result;
}
