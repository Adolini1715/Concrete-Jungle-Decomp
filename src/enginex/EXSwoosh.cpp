#include "EXSwooshSysMgr.h"
#include "EXSwooshSys.h"
#include "EXGeoSwoosh.h"
#include "EXGeoFile.h"
#include "EXItem.h"
#include "EXItemAnimator_Entity.h"
#include "EXBaseDisplay.h"
#include "EXBaseApp.h"
#include "EXMemory.h"
#include <string.h>

EX_IMPLEMENT_DYNCREATE(EXBaseSwooshSys, EXEffectsSys)
EX_IMPLEMENT_DYNCREATE(EXSwooshSys, EXBaseSwooshSys)

void EXSwooshSysMgr::StopAllSystems(s8 stop)
{
    m_bStop = stop;
    m_bPause = stop;
}

void EXSwooshSysMgr::PauseAllSystems(s8 pause)
{
    m_bPause = pause;
}

EXBaseSwooshSys::EXBaseSwooshSys()
{
    m_pVerts = 0;
    m_pVerts2 = 0;
    m_pRows = 0;
    m_pMem = 0;
}

EXBaseSwooshSys::~EXBaseSwooshSys()
{
    if (m_pMem) {
        EXFree(m_pMem);
        m_pMem = 0;
    }
}

void EXBaseSwooshSys::SetTessalationLevel(u32 level)
{
    u32 l = level;
    if (l < 1) l = 1;
    if (l > 4) l = 4;
    while (l > 1) {
        if (m_numSegments * l < 0xb4) break;
        l--;
    }
    m_tessalation = l;
}

void EXBaseSwooshSys::SetVertexPosition(const EXVector &pos, u32 hash)
{
    if (m_bKill) return;
    if (!m_pVerts) return;
    m_pVerts[m_curSegment * m_numHashs + hash] = pos;
    m_bDirty = 1;
}

s8 EXBaseSwooshSys::Finished() const
{
    if (m_bLoop == 1) return 0;
    if (100.0f > m_age) return 0;
    return 1;
}

void EXBaseSwooshSys::KillSwoosh()
{
    m_bDetached = 1;
}

void EXBaseSwooshSys::Detatch()
{
    if (m_pItem) {
        m_pItem = 0;
        m_bDetached = 1;
        if (++m_curSegment > m_numSegments - 1) m_curSegment = 0;
    }
}

s8 EXSwooshSysMgr::DetachSystem(const EXEffectsSys *pSys)
{
    for (u32 i = 0; i < (u32)m_systems.m_nSize; i++) {
        EXSwooshSysData &d = m_systems[i];
        if ((EXEffectsSys *)d.m_pSys == pSys) {
            d.m_pSys->Detatch();
            return 1;
        }
    }
    return 0;
}

s8 EXSwooshSysMgr::DeleteSystem(const EXEffectsSys *pSys)
{
    for (u32 i = 0; i < (u32)m_systems.m_nSize; i++) {
        EXSwooshSysData &d = m_systems[i];
        if ((EXEffectsSys *)d.m_pSys == pSys) {
            delete d.m_pSys;
            m_systems.RemoveAt(i);
            return 1;
        }
    }
    return 0;
}

void EXSwooshSysMgr::DeleteAllSystems()
{
    for (u32 i = 0; i < (u32)m_systems.m_nSize; i++) {
        delete m_systems[i].m_pSys;
    }
    m_systems.Empty(1);
}

void EXSwooshSysMgr::DeleteAllSystems(const EXItem *pItem)
{
    int i = m_systems.m_nSize;
    while (i) {
        i--;
        EXSwooshSysData &d = m_systems[i];
        if (d.m_pSys->m_pItem == pItem) {
            delete d.m_pSys;
            m_systems.RemoveAt(i);
        }
    }
}

void EXSwooshSysMgr::KillAllSystems(const EXItem *pItem)
{
    int i = m_systems.m_nSize;
    while (i) {
        i--;
        EXSwooshSysData &d = m_systems[i];
        if (d.m_pSys->m_pItem == pItem) d.m_pSys->m_bKill = 1;
    }
}

void EXSwooshSysMgr::KillAllSystems(const EXItemAnimator_Entity *pEntity)
{
    int i = m_systems.m_nSize;
    while (i) {
        i--;
        EXSwooshSysData &d = m_systems[i];
        if (d.m_pSys->m_pEntity == pEntity) d.m_pSys->m_bKill = 1;
    }
}

void EXSwooshSysMgr::RemoveSystem(const EXEffectsSys *pSys)
{
    int i = m_systems.m_nSize;
    while (i) {
        i--;
        EXSwooshSysData &d = m_systems[i];
        if ((EXEffectsSys *)d.m_pSys == pSys) {
            delete d.m_pSys;
            m_systems.RemoveAt(i);
        }
    }
}

void EXSwooshSysMgr::DetatchSwoosh(const EXItem *pItem)
{
    for (u32 i = 0; i < (u32)m_systems.m_nSize; i++) {
        EXSwooshSysData &d = m_systems[i];
        if (d.m_pSys->m_pItem == pItem) d.m_pSys->Detatch();
    }
}

void EXSwooshSysMgr::DetatchSwoosh(const EXItemAnimator_Entity *pEntity)
{
    for (u32 i = 0; i < (u32)m_systems.m_nSize; i++) {
        EXSwooshSysData &d = m_systems[i];
        if (d.m_pSys->m_pEntity == pEntity) d.m_pSys->Detatch();
    }
}

EXSwooshSys *EXSwooshSysMgr::FindSwoosh(u32 hash, const EXItem *pItem)
{
    for (u32 i = 0; i < (u32)m_systems.m_nSize; i++) {
        if (m_systems[i].m_pSys->m_hash == hash && m_systems[i].m_pSys->m_pItem == pItem) return m_systems[i].m_pSys;
    }
    return 0;
}

EXSwooshSys *EXSwooshSysMgr::FindSwoosh(u32 hash, const EXItemAnimator_Entity *pEntity)
{
    for (u32 i = 0; i < (u32)m_systems.m_nSize; i++) {
        if (m_systems[i].m_pSys->m_hash == hash && m_systems[i].m_pSys->m_pEntity == pEntity) return m_systems[i].m_pSys;
    }
    return 0;
}

EXSwooshSys *EXSwooshSysMgr::FindSwooshByID(u32 id, const EXItem *pItem)
{
    for (u32 i = 0; i < (u32)m_systems.m_nSize; i++) {
        if (m_systems[i].m_pSys->m_id == id && m_systems[i].m_pSys->m_pItem == pItem) return m_systems[i].m_pSys;
    }
    return 0;
}

void EXSwooshSysMgr::UpdatePosition(const EXVector &pos, EXSwooshSys *pSys, u32 hash) const
{
    pSys->SetVertexPosition(pos, hash);
}

void EXSwooshSysMgr::SetGlobalAlpha(float a)
{
    for (u32 i = 0; i < (u32)m_systems.m_nSize; i++) m_systems[i].m_pSys->SetSwooshAlpha(a);
}

u32 EXBaseSwooshSys::GetHashEntry(u32 i)
{
    EXGeoSwoosh *pGeo = m_geoRef.GeoSwoosh();
    if (!pGeo) return 0;
    return pGeo->HashEntry(i);
}

// NEAR: 17 bytes differ, m_datum store scheduled later and byte-register choice for the loop flag.
s8 EXBaseSwooshSys::Create(EXGeoFile *pFile, u32 hash, int datum, s8 loop, int segments)
{
    m_geoRef.Set(pFile, hash);
    if (!pFile->GeoHeader()) return 0;
    EXGeoSwoosh *pGeo = m_geoRef.GeoSwoosh();
    if (!pGeo) return 0;
    m_datum = datum;
    m_hash = hash;
    m_life = pGeo->Life();
    m_ageRate = 100.0f / m_life * (1.0f / 60.0f);
    if (!segments) {
        m_bLoop = loop;
        segments = pGeo->MaxSegments();
    } else {
        m_bLoop = loop;
    }
    m_numSegments = segments;
    u32 rowBytes = (pGeo->NumHashs() * sizeof(SwooshVTX_Data *) + 0x10) & ~0xf;
    u32 size = pGeo->NumHashs() * m_numSegments * 4 * sizeof(EXVector) + rowBytes;
    m_pMem = _EXAlloc(size, 1);
    memset(m_pMem, 0, size);
    u8 *p = (u8 *)m_pMem;
    m_pVerts = (EXVector *)p;
    p += pGeo->NumHashs() * m_numSegments * sizeof(EXVector);
    if (!m_pVerts) return 0;
    m_pVerts2 = (EXVector *)p;
    p += pGeo->NumHashs() * m_numSegments * sizeof(EXVector);
    if (!m_pVerts2) return 0;
    m_pRows = (SwooshVTX_Data **)p;
    if ((u32)p & 0xf) while (1) {}
    p += rowBytes;
    if ((u32)p & 0xf) while (1) {}
    if (!m_pRows) return 0;
    for (u32 i = 0; i < pGeo->NumHashs(); i++) {
        if ((u32)p & 0xf) while (1) {}
        m_pRows[i] = (SwooshVTX_Data *)p;
        p += m_numSegments * sizeof(SwooshVTX_Data);
        if (!m_pRows[i]) return 0;
        memset(m_pRows[i], 0, m_numSegments * sizeof(SwooshVTX_Data));
    }
    float vStep = pGeo->VerticalTiling() / (float)(pGeo->NumHashs() - 1);
    if (pGeo->HorizontalTiling() != 0.0f || pGeo->VerticalTiling() != 0.0f) {
        for (u32 s = 0; s < m_numSegments; s++) {
            float u = (float)s * pGeo->HorizontalTiling();
            for (u32 h = 0; h < pGeo->NumHashs(); h++) {
                SwooshVTX_Data *pRow = m_pRows[h];
                pRow[s].m_u = u;
                pRow[s].m_v = (float)h * vStep;
            }
        }
    }
    m_bDirty = 0;
    m_age = 0.0f;
    m_bKill = 0;
    m_tessalation = 1;
    m_6c = 0.0f;
    m_curSegment = 0;
    m_bManualUpdate = 0;
    m_bDetached = 0;
    m_70 = 1.0f;
    m_alpha = 1.0f;
    m_controllers = 0;
    if (pGeo->UseTaperController() == 1) m_controllers = 1;
    if (pGeo->UseAlphaController() == 1) m_controllers |= 2;
    if (pGeo->UseColourController() == 1) m_controllers |= 4;
    m_endR = pGeo->EndRScale() * (1.0f / 256.0f);
    m_endG = pGeo->EndGScale() * (1.0f / 256.0f);
    m_endB = pGeo->EndBScale() * (1.0f / 256.0f);
    m_endAlpha = pGeo->EndAlphaScale() * (1.0f / 256.0f);
    m_startR = pGeo->StartRScale() * (1.0f / 256.0f);
    m_startG = pGeo->StartGScale() * (1.0f / 256.0f);
    m_startB = pGeo->StartBScale() * (1.0f / 256.0f);
    m_startAlpha = pGeo->StartAlphaScale() * (1.0f / 256.0f);
    m_numHashs = pGeo->NumHashs();
    return 1;
}

s8 EXBaseSwooshSys::UpdateSwoosh()
{
    if (m_6c > 0.0f) CalculateSwoosh();
    if (m_bDirty == 1) {
        if (--(int &)m_curSegment < 0) m_curSegment = m_numSegments - 1;
        if (!m_bDetached) {
            if ((float)m_numSegments > m_6c) m_6c += 1.0f;
        } else if (m_6c > 0.0f) {
            m_6c -= m_70;
        } else if (0.5f > m_6c) {
            m_bKill = 1;
        }
    } else if (m_age != 0.0f) {
        if (m_6c > 0.0f) m_6c -= m_70;
        else if (0.5f > m_6c) m_bKill = 1;
    }
    m_bDirty = 0;
    return 1;
}

EXSwooshSysMgr::~EXSwooshSysMgr()
{
    DeleteAllSystems();
}

// NEAR: retail inlines AddSpace with a GrowArray call; EXArray.h's AddSpace writes the growth loop out
// (matches App.obj's EXStackBlock copy), so this and the emitted AddSpace copy differ.
EXSwooshSys *EXSwooshSysMgr::CreateSystem(u32 type)
{
    EXSwooshSysData &d = m_systems.AddSpace(1);
    d.m_pSys = new EXSwooshSys;
    d.m_pSys->m_pMgr = this;
    return d.m_pSys;
}

s8 EXBaseSwooshSys::Initialize(EXGeoFile *pFile, u32 hash, EXItem *pItem, EXItemAnimator_Entity *pEntity, int datum, s8 loop, int segments)
{
    m_pItem = pItem;
    m_pEntity = pEntity;
    return Create(pFile, hash, datum, loop, segments);
}

s8 EXBaseSwooshSys::Update(const EXSwooshSysMgr *pMgr)
{
    if (!m_geoRef.GeoSwoosh()) return 0;
    UpdateSwoosh();
    m_age += m_ageRate * EXBaseApp::EngineRenderFrameRateMul();
    if ((m_age >= 100.0f && !m_bLoop) || (m_bKill == 1 && 0.0f >= m_6c && m_age >= 100.0f) || (m_bDetached == 1 && 0.0f >= m_6c)) {
        m_pMgr->RemoveSystem(this);
        return 0;
    }
    return 1;
}

EXSwooshSysMgr::EXSwooshSysMgr()
{
    m_systems.SetSize(0, 0x28);
    m_bStop = 0;
    m_bPause = 0;
}
