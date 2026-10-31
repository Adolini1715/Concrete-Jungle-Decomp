#include "EXCollisionScoopTemp.h"
#include "EXItemAnimator_Map.h"
#include "EXMemory.h"
#include "EXBaseSoundManager.h"
#include "EXStats.h"
#include "EXDatum.h"
#include "EXNew.h"
#include "EXGeoMap.h"
#include "EXGeoFile.h"
#include "EXGeoHeader.h"
#include "EXGeoEntity.h"
#include "EXGeoComplexRefObject.h"
#include "EXGrabPointTemp.h"
#include "EXGeoIdentifier.h"
#include "EXSoundHandle.h"
#include "EXItemEnv.h"
#include "EXRayCastTemp.h"
#include "EXShadowTemp.h"

// Map animator (EXItemAnimator_Map.obj). Only the animator's own simple members and the
// simpler zone-table / placement helpers are decompiled.

// TODO stand-in: an entity's datum table (extra data type 5), as in EXItemAnimator_Entity.cpp.
struct EXGeoEntityDatums {
    u8 m_00[5];
    u8 m_count;                             // +5
    ex::EXRelPtr16<EXGeoBaseDatum> m_datums;    // +6
};

// EXGeoHeader::GeoEntityHeader, which the original inlines here (VC7.1 calls the Find copy).
static __forceinline EXGeoEntityHeader *GeoEntityHeaderOf(const EXGeoHeader *pHdr, u32 hash)
{
    const EXGeoCommonArray<EXGeoEntityHeader> &a = pHdr->m_Entities;
    int i = EXGeoCommonArray_FindIndex(hash, (void *)&a, sizeof(EXGeoEntityHeader));
    if (i < 0) return 0;
    return &a.Data()[i];
}

// EXBoundsBox::Set, which the original inlines here (ours would call the emitted copy).
static __forceinline void BoxSet(EXBoundsBox &d, const EXBoundsBox &s)
{
    d.m_vctMin = s.m_vctMin;
    d.m_vctMax = s.m_vctMax;
}

EX_IMPLEMENT_DYNCREATE(EXItemAnimator_Map, EXItemAnimator)

EXItemAnimator_Map::~EXItemAnimator_Map() {}

void EXItemAnimator_Map::DoUpdate() {}

u32 EXItemAnimator_Map::DoRender(const EXMatrix &, const EXRenderInfo &, u32) { return 0; }

s8 EXItemAnimator_Map::_GetBoundsBox(EXBoundsBox &box, u32 flags)
{
    EXBoundsBox b;
    GetGeoBoundsBox(b);
    ApplyMatrixBoundsBox(b, box, flags);
    return 1;
}

void EXItemAnimator_Map::GetGeoBoundsBox(EXBoundsBox &box)
{
    const EXGeoComplexRefObject &ref = m_geoRef;  // retail calls the const GeoMapHeader (0x208360)
    if (ref.GeoFile()) {
        EXGeoHeader *pHdr = ref.GeoFile()->m_pGeoHeader;
        if (pHdr) {
            EXGeoMapHeader *pMap = ref.GeoMapHeader();
            if (pMap && pMap->m_Addr[0].m_pObject) BoxSet(box, pMap->m_Addr[0].m_pObject->BoundsBox(pHdr));
        }
    }
}

// NEAR: 82 bytes; the original places each shared return-0 block right after its test pair
// (and saves ebp up front); ours puts them at the end.
s8 EXItemAnimator_Map::Initialise(EXGeoFile *pFile, u32 hash)
{
    m_geoRef.Set(pFile, hash);
    EXGeoHeader *pHdr;
    if (!pFile || !(pHdr = pFile->m_pGeoHeader)) return 0;
    EXGeoMapHeader *pMapHdr = ((const EXGeoComplexRefObject &)m_geoRef).GeoMapHeader();  // const overload, as retail
    EXGeoMap *pMap;
    if (!pMapHdr || !(pMap = pMapHdr->m_Addr[0].m_pObject)) return 0;
    m_zoneTable.Initialise(pFile, hash);
    m_animators.SetGrowBy(0x40);
    m_boundsBox = pMap->BoundsBox(pHdr);
    return 1;
}

void EXMapZoneInfoTable::UpdateTrackingSounds(u32 mask)
{
    for (int i = 0; i < 8; i++)
        if ((m_iSounds[i].Mask() & mask) && m_iSounds[i].Status() == 2) m_iSounds[i].m_time--;
}

void EXMapZoneInfoTable::SetOcclusionFlags(const u32 *pFlags, s8 bInit)
{
    if (bInit) {
        for (int i = 0; i < 8; i++) m_occlusion.m_bits[i] |= pFlags[i];
        m_occlusionState = 2;
        return;
    }
    switch (m_occlusionState) {
    case 0:
        for (int i = 0; i < 8; i++) m_occlusion.m_bits[i] |= pFlags[i];
        m_occlusionState = 1;
        break;
    case 1:
        for (int i = 0; i < 8; i++) m_occlusion.m_bits[i] &= pFlags[i];
        break;
    }
}

void EXMapZoneInfoTable::ClrOcclusionFlags()
{
    m_occlusion.ClrAll();
    m_occlusionState = 0;
}

void EXMapPlacement::Initialise(const EXGeoHeader *pHdr, const EXGeoPlacement *pPlace, int zone)
{
    m_mtx = pPlace->CalcMatrix();
    m_entity = pPlace->m_entity;
    m_baseEntity = pPlace->m_entity;
    m_hashRef = pPlace->m_hash;
    m_pGeoFile = pHdr->m_pGeoFile;
    m_90 = pPlace->m_2c;
    m_92 = pPlace->m_2e;
    m_94 = pPlace->m_34;
    m_96 = pPlace->m_36;
    m_zone = (u16)zone;
}

// The placement's world bounds: its entity's box and those of its collision entities (extra data
// 0..4) and grab point (6), each moved by the placement matrix. Without an entity the box is left
// empty (inverted, +-1e6).
void EXMapPlacement::Initialise_BoundsBox(const EXGeoHeader *pHdr)
{
    EXGeoEntity *pEnt = GeoEntityHeaderOf(pHdr, m_baseEntity)->Object(0);
    if (!pEnt) {
        m_bounds.m_vctMin.set(1e6f, 1e6f, 1e6f, 0.0f);
        m_bounds.m_vctMax.set(-1e6f, -1e6f, -1e6f, 0.0f);
        return;
    }
    m_bounds.Set(pEnt->BoundsBox());
    m_bounds._ApplyXForm(m_mtx);
    EXBoundsBox b;
    for (u32 i = 0; i < 5; i++) {
        if (pEnt->HasCollisionEntity(i)) {
            b.Set(pEnt->Collision(i)->BoundsBox());
            b._ApplyXForm(m_mtx);
            m_bounds.Accumulate(b);
        }
    }
    EXGeoGrabPoint *pGrab = (EXGeoGrabPoint *)pEnt->m_XtraData.GetXtra(6);
    if (!pGrab) return;
    pGrab->GetBoundsBox(b);
    b._ApplyXForm(m_mtx);
    m_bounds.Accumulate(b);
}

void EXMapPlacement::InitialiseGDiMemory(const EXGeoHeader *pHdr) { m_render.Initialise(pHdr, m_entity); }

void EXMapPlacement::RemoveGDiMemory() { m_render.RemoveGDiMemory(); }

void EXMapPlacement::DisablePlacement(s8 b)
{
    if (b) {
        m_render.RemoveGDiMemory();
        m_entity = 0xffffffff;
    } else {
        m_entity = m_baseEntity;
    }
}

void EXMapZoneInfoTable::ViewProcess()
{
    for (int i = 0; i < m_zones.Size(); i++) {
        EXMapZoneInfo &z = m_zones[i];
        s8 a = z.m_view;
        s8 b = z.m_viewNext;
        if (b > 0) {
            if (a > 0) z.m_view = min(a, b);
            else z.m_view = max(a, b);
        } else if (b < 0 && a <= 0) {
            z.m_view = min(a, b);
        }
        if (z.m_soundNext) z.m_soundable = 1;
    }
}

void EXMapZoneInfoTable::InitPlacementUsedArray()
{
    for (int i = 0; i < m_placementUsed.Size(); i++) m_placementUsed[i] = 0;
}

int EXMapZoneInfoTable::GetMapPlacementIndex(u32 hash) const
{
    for (int i = 0; i < m_placements.m_nSize; i++)
        if (m_placements.m_pData[i].HashRef() == hash) return i;
    return -1;
}

EXMapPlacement *EXMapZoneInfoTable::GetMapPlacement(u32 hash) const
{
    u32 i = (hash & 0xff000000) ? GetMapPlacementIndex(hash) : hash;
    if (i >= (u32)m_placements.m_nSize) return 0;
    return &m_placements.m_pData[i];
}

EXMapPlacement *EXMapZoneInfoTable::GetMapPlacement(const EXVector &p) const { return GetMapPlacement(GetMapPlacementIndex(p)); }

// NEAR: 32 bytes; the original stores the immediate 1 before loading al (ours stores al), which
// shifts the loop alignment.
s8 EXMapZoneInfo::SetZoneLoaded(const EXGeoHeader *, s8 b)
{
    if (b) {
        if (!m_loaded) {
            m_loaded = 1;
            return 1;
        }
        return 0;
    }
    if (!m_loaded) return 0;
    for (int i = 0; i < m_subZones.Size(); i++) m_subZones[i].RemoveAll();
    m_loaded = 0;
    return 1;
}

void EXMapZoneInfo::UpdateZoneLoaded(const EXGeoHeader *)
{
    for (int i = 0; i < m_subZones.Size(); i++) m_subZones[i].RemoveAll();
}

// NEAR: 10 bytes; the m_90 store is scheduled one instruction later in the original.
EXMapPlacement::EXMapPlacement()
{
    m_90 = 0;
    m_hashRef = 0xffffffff;
    m_entity = 0xffffffff;
    m_bounds.SetMaxRange();
}

EXMapPlacement::~EXMapPlacement() { m_render.RemoveGDiMemory(); }

EXMapZoneInfoTable::EXMapZoneInfoTable() {}

// NEAR candidate: the original's array destructors test the element count before calling
// DestructElements (EXArray.h, other owner, has no such test).
EXMapZoneInfoTable::~EXMapZoneInfoTable()
{
    for (int i = 0; i < m_owned.Size(); i++) {
        if (m_owned[i]) {
            delete m_owned[i];
            m_owned[i] = 0;
        }
    }
    m_sounds.RemoveAll(1);
}

void EXMapZoneInfoTable::ViewInitialise()
{
    for (int i = 0; i < m_zones.Size(); i++) {
        EXMapZoneInfo &z = m_zones[i];
        z.m_view = 0;
        z.m_soundable = 0;
        z.m_viewNext = 0;
        z.m_soundNext = 0;
        z.m_viewList.Reset();
    }
    ClrOcclusionFlags();
    InitPlacementUsedArray();
}

void EXMapZoneInfoTable::ViewReset()
{
    for (int i = 0; i < m_zones.Size(); i++) {
        EXMapZoneInfo &z = m_zones[i];
        z.m_viewNext = 0;
        z.m_soundNext = 0;
        z.m_viewList.Reset();
    }
    ClrOcclusionFlags();
    InitPlacementUsedArray();
}

void EXMapZoneInfoTable::UpdateSounds()
{
    EXBaseSoundManager *pMgr = EXBaseSoundManager::GetSoundManager();
    if (!pMgr) return;
    EXSoundHandle *p = m_sounds.m_pHead;
    while (p) {
        EXSoundHandle *pNext = (EXSoundHandle *)p->m_pNext;
        if (p->UpdateState() < 0) {
            pMgr->KillSoundPlaying(p);
            p->Remove();
        } else {
            p->SetUpdateState(0);
        }
        p = pNext;
    }
    for (int i = 0; i < 8; i++) {
        EXISoundTag &t = m_iSounds[i];
        if (!t.m_pHead) continue;
        switch (t.Status()) {
        case 1:
        case 2:
            if (t.Time() < g_Stats.FrameCount() - 1) t.SetStatus(3);
            break;
        case 3:
            pMgr->KillSoundPlaying(t.m_pHead);
            t.SetStatus(0);
            break;
        }
    }
}

// NEAR: 20 bytes; the shared return-0 block sits after the entity-pointer test in the original
// (ours: after the header test).
s8 EXMapPlacement::GetDatum(u32 type, EXDatumTemp &d, u32 flags)
{
    EXGeoFile *pFile = m_pGeoFile;
    if (!pFile) return 0;
    if (!pFile->m_pGeoHeader) return 0;
    EXGeoEntityHeader *pEnt = GeoEntityHeaderOf(pFile->m_pGeoHeader, m_baseEntity);
    if (!pEnt) return 0;
    if (!pEnt->m_Addr[0].m_pObject) return 0;
    EXGeoEntityDatums *pDatums = (EXGeoEntityDatums *)pEnt->m_Addr[0].m_pObject->m_XtraData.GetXtra(5);
    if (!pDatums) return 0;
    for (int i = 0; i < pDatums->m_count; i++) {
        EXGeoBaseDatum *p = &((EXGeoBaseDatum *)pDatums->m_datums)[i];
        if (p->m_Type == type) {
            EXVector scale;
            scale.SetZero();
            if (!(flags & 4)) d.Set(p, scale, m_mtx);
            else d.Set(p, scale);
            return 1;
        }
    }
    return 0;
}

void EXMapPlacement::DoEntityScoop(EXCollisionScoopTemp &t, u32 flags)
{
    EXGeoFile *pFile = m_pGeoFile;
    if (pFile && pFile->m_pGeoHeader) {
        EXGeoEntityHeader *pHdr = GeoEntityHeaderOf(pFile->m_pGeoHeader, m_baseEntity);
        if (pHdr) {
            EXGeoBaseEntity *pEnt = pHdr->m_Addr[0].m_pObject;
            if (pEnt && pEnt->HasCollision() && t.m_layer.CalcCollisionEntity(pEnt)) {
                t.SetTemporaryVars(0, m_mtx, -1);
                if (m_90 & 4) flags |= 2;
                pEnt->DoEntityScoop_Sphere((EXCScoopTemp &)t, 0, 0, flags);
            }
        }
    }
}

// NEAR: 110 bytes; the original keeps the result in a spilled variable (stored with m_loaded,
// reloaded at the end) where ours folds it to a constant.
s8 EXMapZoneInfoTable::InitialiseMapZonePlacements(const EXGeoHeader *pHdr, const EXGeoZone *pZone, int zone)
{
    EXMapZoneInfo &z = m_zones[zone];
    s8 r = 0;
    if (!z.m_loaded) {
        r = 1;
        z.m_loaded = r;
        EXGeoCommonObject *pObj = pHdr->RefPointer(pZone->m_00);
        if (pObj) z.m_subZones.SetSize(*((u8 *)pObj + 0x54), -1);
        EXGeoPlacementInfo *pList = pZone->PlacementInfo();
        u16 *pIdx = pList->m_Placements;
        for (int i = 0; i < pList->m_NumPlacements; i++) m_placements.m_pData[pIdx[i]].Initialise_BoundsBox(pHdr);
    }
    return r;
}

// Grab points of the map near t.m_pos: each zone in reach offers its own collision entity's grab
// data (when the query's layer accepts it), and every placement whose bounds come within the
// radius + 1 offers its entity's. Retail re-walks every placement once per zone in reach, and a
// zone's result replaces (rather than adds to) what was found before it; both kept.
template <class Temp>
static __forceinline s8 MapDoGrabPoint(EXItemAnimator_Map *pMap, const EXMatrix &mtx, Temp &t)
{
    const EXGeoComplexRefObject &ref = pMap->m_geoRef;
    s8 found = 0;
    if (!ref.GeoFile()) return 0;
    EXGeoHeader *pHdr = ref.GeoFile()->m_pGeoHeader;
    if (!pHdr) return 0;
    EXGeoMapHeader *pMapHdr = ref.GeoMapHeader();
    if (!pMapHdr) return 0;
    EXGeoMap *pGeo = pMapHdr->m_Addr[0].m_pObject;
    if (!pGeo) return 0;
    unsigned int zones[32];
    int nZones = pGeo->GetMapZoneList(t.m_pos, t.m_radius, zones);
    for (int z = 0; z < nZones; z++) {
        EXGeoBaseEntity *pEnt = (EXGeoBaseEntity *)pHdr->RefPointer(pGeo->MapZone(zones[z])->m_00);
        if (pEnt && t.m_layer.CalcGrabpointEntity(pEnt)) {
            EXGeoGrabPoint *pGrab = (EXGeoGrabPoint *)pEnt->m_XtraData.GetXtra(6);
            if (pGrab) found = pGrab->DoGrabPoint(mtx, t);
        }
        EXMapPlacementArray &places = pMap->m_zoneTable.PlacementArray();
        for (int i = 0; i < places.Size(); i++) {
            EXMapPlacement &pl = places[i];
            if (!pl.m_bounds._Intersect(t.m_pos, t.m_radius + 1.0f)) continue;
            EXGeoEntity *pPlEnt = GeoEntityHeaderOf(pHdr, pl.m_entity)->Object(0);
            if (!pPlEnt) continue;
            EXGeoGrabPoint *pGrab = (EXGeoGrabPoint *)pPlEnt->m_XtraData.GetXtra(6);
            if (pGrab) found |= pGrab->DoGrabPoint(pl.m_mtx, t);
        }
    }
    return found;
}

s8 EXItemAnimator_Map::DoGrabPoint(const EXMatrix &mtx, EXGrabPointTemp &t, u32 flags)
{
    s8 found = MapDoGrabPoint(this, mtx, t);
    if (found) t.m_pAnimator = this;
    return found;
}

s8 EXItemAnimator_Map::DoGrabPoint(const EXMatrix &mtx, EXGrabPointArrayTemp &t, u32 flags)
{
    return MapDoGrabPoint(this, mtx, t);
}

// Drops the render data of zones whose section has been unloaded (state 0) or is being dropped
// (state 4): the sub-zone and placement GDi memory goes, and the zone's sky (if any) is flagged
// (item flag 0x10). If anything was dropped every zone's placement tree is refitted. Returns
// whether any zone was in state 4.
s8 EXMapZoneInfoTable::UpdateMapZoneLoaded(EXGeoHeader *pHdr, EXGeoMap *pGeo)
{
    s8 bDropping = 0;
    s8 bUnloaded = 0;
    for (int z = 0; z < (int)pGeo->NumZones(); z++) {
        EXGeoZone *pZone = pGeo->MapZone(z);
        unsigned int state = pHdr->IsSectionLoaded(pZone->Section(), 0);
        if (state == 4) bDropping = 1;
        else if (state != 0) continue;
        EXMapZoneInfo &zi = m_zones[z];
        if (!zi.m_loaded) continue;
        for (int i = 0; i < zi.m_subZones.Size(); i++) zi.m_subZones[i].RemoveAll();
        zi.m_loaded = 0;
        bUnloaded = 1;
        int sky = pZone->Identifier()->m_SkyIndex;
        if (sky >= 0 && SkyPlacementInfoArray()[sky]) SkyPlacementInfoArray()[sky]->m_itemFlags.m_flags |= 0x10;
        EXGeoPlacementInfo *pList = pZone->PlacementInfo();
        u16 *pIdx = pList->m_Placements;
        for (int i = 0; i < pList->m_NumPlacements; i++) m_placements[pIdx[i]].m_render.RemoveGDiMemory();
    }
    if (bUnloaded) {
        EXBoundsBox box;
        box.m_vctMin.set(1e6f, 1e6f, 1e6f, 0.0f);
        box.m_vctMax.set(-1e6f, -1e6f, -1e6f, 0.0f);
        for (int z = 0; z < (int)pGeo->NumZones(); z++)
            pGeo->Initialise_PlacementTree(pGeo->MapZone(z)->PlacementInfo()->PlacementTree(), pHdr, box);
    }
    return bDropping;
}

// Plays or updates one interior sound in a slot: an effect (kind 0x1a) starts at pos (returning
// 1 if it could not) or, when already playing, is kept alive (handle state 4), moved (halfway
// from its last position when smoothing) and re-volumed; music (0x1b) is (re)started through the
// item environment; 0x1c sets the reverb depth. The slot remembers pos.
s8 EXMapZoneInfoTable::UpdateISound(const EXGeoSound *pSnd, EXISoundTag &tag, EXVector pos, s8 bSmooth) const
{
    u32 sound = pSnd->m_Sound;
    switch ((sound >> 24) & 0x7f) {
    case 0x1a:
        if (!tag.m_pHead) {
            if (!tag.StartSound(sound, pos, pSnd->m_MinDist, pSnd->m_MaxDist, pSnd->m_Volume, 0)) return 1;
            tag.m_status = 2;
            tag.m_sfxHash = sound;
        } else {
            tag.m_pHead->m_Unk2e = 4;
            if (bSmooth) {
                pos.x = (pos.x - tag.m_pos.x) * 0.5f + tag.m_pos.x;
                pos.y = (pos.y - tag.m_pos.y) * 0.5f + tag.m_pos.y;
                pos.z = (pos.z - tag.m_pos.z) * 0.5f + tag.m_pos.z;
                pos.w = (pos.w - tag.m_pos.w) * 0.5f + tag.m_pos.w;
            }
            tag.UpdatePosition(pos);
            tag.m_status = 2;
            tag.SetVolume(pSnd->m_Volume);
        }
        break;
    case 0x1b:
        if (EXItemEnv::GetItemEnv())
            EXItemEnv::GetItemEnv()->StartMusic(sound, pSnd->m_Flags & ~0x80, pSnd->m_1e, pSnd->m_1d);
        break;
    case 0x1c:
        EXBaseSoundManager::GetSoundManager()->SfxSetReverbDepth(pSnd->m_Volume);
        break;
    }
    tag.m_pos = pos;
    return 0;
}

// Starts or refreshes the interior sounds of zone `on` selected by mask, heard through pPortal
// (from its nearest point, if within the sound's range) or, without a portal, at the listener.
// A sound already in a slot keeps it (matched by sound, and by zone unless both are direct);
// otherwise the first free slot is taken.
void EXMapZoneInfoTable::AddISound(EXGeoMap *pGeo, const EXGeoPortal *pPortal, EXMapOn on, u32 mask, int recursion)
{
    EXVector listener = EXItemEnv::m_MapSoundPosition;
    const EXGeoZone *pZone = pGeo->MapZone((u8)on.m_v);
    const EXGeoZoneISound *pList = pZone->ISounds();
    if (!pList) return;
    int n = pZone->NumISounds();
    if (n < 1) return;
    for (; n > 0; n--, pList++) {
        if (!(mask & (1 << pList->m_Bit))) continue;
        const EXGeoSound *pSnd = &pGeo->m_Sounds[pList->m_Sound];
        u32 sound = pSnd->m_Sound;
        int slot;
        for (slot = 0; slot < 8; slot++) {
            const EXISoundTag &t = m_iSounds[slot];
            if (t.m_status && t.m_sfxHash == sound && ((!recursion && !t.m_recursion) || on.m_v == t.m_mapOn.m_v)) break;
        }
        if (slot == 8) {
            for (slot = 0; slot < 8; slot++)
                if (!m_iSounds[slot].m_status) break;
            if (slot == 8) continue;
        }
        EXISoundTag &tag = m_iSounds[slot];
        EXVector pos;
        s8 bSmooth;
        if (!pPortal) {
            pos = listener;
            bSmooth = 0;
        } else {
            EXVector nearest;
            float dist = pPortal->DistanceTo(listener, nearest);
            float range = pSnd->m_MaxDist;
            if ((sound & 0x7f000000) == 0x1a000000 && range == 0.0f)
                range = (float)EXBaseSoundManager::GetSoundManager()->SoundDetails(sound).m_MaxDist;
            if (range < dist) continue;
            pos = nearest;
            bSmooth = 1;
        }
        if (!UpdateISound(pSnd, tag, pos, bSmooth)) {
            tag.m_mapOn = on;
            tag.m_recursion = recursion;
            tag.m_time = g_Stats.FrameCount();
            tag.m_mask = 1 << pList->m_Bit;
        }
    }
}

// Builds the table for map `hash` of a file: registers the map with the item environment, then
// one zone info per zone (its map/zone id and its identifier's fog, copied to the draw fog), the
// placement-used bits, the placements, groups, clusters and empty sky slots.
void EXMapZoneInfoTable::Initialise(EXGeoFile *pFile, u32 hash)
{
    m_pGeoFile = pFile;
    m_mapHash = hash;
    if (!pFile) return;
    EXGeoHeader *pHdr = pFile->m_pGeoHeader;
    if (!pHdr) return;
    EXGeoMap *pGeo = pHdr->GeoMap(hash);
    if (!pGeo) return;
    EXMapOn on = EXItemEnv::GetItemEnv()->AddGeoMap(pHdr, pGeo);
    m_zones.SetSize(pGeo->NumZones());
    for (u32 z = 0; z < pGeo->NumZones(); z++) {
        EXMapZoneInfo &zi = m_zones[z];
        zi.m_mapOn.m_v = (s16)((on.m_v & 0xff00) | (u8)z);
        zi.m_loaded = 0;
        const EXGeoIdentifier *pId = pGeo->MapZone(z)->Identifier();
        zi.m_baseRender.m_FogNear = pId->m_FogNear;
        zi.m_baseRender.m_FogFar = pId->m_FogFar;
        zi.m_baseRender.m_FogMin = pId->m_FogMin;
        zi.m_baseRender.m_FogMax = pId->m_FogMax;
        zi.m_baseRender.m_FogColor = pId->m_FogColor;
        zi.m_baseRender.m_FogEnabled = pId->m_FogEnabled != 0;
        zi.m_drawRender = zi.m_baseRender;
    }
    int nPlacements = pGeo->m_Placements.Size();
    m_placementUsed.SetSize((nPlacements + 31) / 32, -1);
    m_placements.SetSize(nPlacements);
    // The last argument (EXMapPlacement::m_zone) receives the placement index.
    for (int i = 0; i < nPlacements; i++) m_placements[i].Initialise(pHdr, &pGeo->m_Placements[i], i);
    m_groups.SetSize(pGeo->m_PlacementGroups.Size());
    m_clusters.Init(pGeo->m_Clusters.Size());
    int nSkies = pGeo->m_Skys.Size();
    m_owned.SetSize(nSkies, -1);
    for (int i = 0; i < nSkies; i++) m_owned[i] = 0;
    ViewInitialise();
}

// Gathers collision faces around t.m_centre: from the collision entity of the zone the centre is
// in (or, with scoop flag 2, of every zone within the radius), then, unless the layer excludes
// placements (0x800), from the placements in the first zone's placement tree that the sphere
// reaches. The placement indices are collected in m_animators, reused as scratch. Placements
// with engine flag 4 scoop with flag 2 added.
void EXItemAnimator_Map::DoEntityScoop(const EXMatrix &mtx, EXCollisionScoopTemp &t, u32 flags)
{
    const EXGeoComplexRefObject &ref = m_geoRef;
    if (!ref.GeoFile()) return;
    EXGeoHeader *pHdr = ref.GeoFile()->m_pGeoHeader;
    if (!pHdr) return;
    EXGeoMapHeader *pMapHdr = ref.GeoMapHeader();
    if (!pMapHdr) return;
    EXGeoMap *pGeo = pMapHdr->m_Addr[0].m_pObject;
    if (!pGeo) return;
    unsigned int zones[256];
    unsigned int nZones;
    if (!(t.m_flags & 2)) {
        zones[0] = pGeo->WhichZone(t.m_centre);
        nZones = 1;
    } else {
        nZones = pGeo->GetMapZoneList(t.m_centre, t.m_radius, zones);
    }
    for (unsigned int z = 0; z < nZones; z++) {
        EXGeoBaseEntity *pEnt = (EXGeoBaseEntity *)pHdr->RefPointer(pGeo->MapZone(zones[z])->m_00);
        if (pEnt && pEnt->HasCollision() && t.m_layer.CalcCollisionEntity(pEnt)) {
            t.m_pItem = 0;
            t.m_mtx = mtx;
            t.m_2c = -1;
            pEnt->DoEntityScoop_Sphere(*(EXCScoopTemp *)&t, 0, 0, flags);
        }
    }
    if (t.m_layer.m_flags & 0x800) return;
    EXu32Array &found = *(EXu32Array *)&m_animators;
    found.m_nSize = 0;
    DoCollision_PlacementTree(pGeo->MapZone(zones[0])->PlacementInfo()->PlacementTree(), t.m_centre, t.m_radius, found);
    for (int i = 0; i < found.m_nSize; i++) {
        EXMapPlacement &pl = m_zoneTable.m_placements[found[i]];
        if (!pl.m_bounds._Intersect(t.m_centre, t.m_radius)) continue;
        EXGeoBaseEntity *pEnt = GeoEntityHeaderOf(pHdr, pl.m_entity)->Object(0);
        if (pEnt && pEnt->HasCollision() && t.m_layer.CalcCollisionEntity(pEnt)) {
            t.m_pItem = 0;
            t.m_mtx = pl.m_mtx;
            t.m_2c = i;
            u32 f = flags;
            if (pl.m_90 & 4) f |= 2;
            pEnt->DoEntityScoop_Sphere(*(EXCScoopTemp *)&t, 0, 0, f);
        }
    }
}

// Casts t's ray through the map: for each zone the segment passes, the zone's collision entity,
// then (unless the layer excludes placements, 0x800) the placements in the zone's tree whose
// bounds the ray reaches, each tested in its own space. A hit marks t valid for this animator;
// in any-hit mode the first hit ends the cast.
void EXItemAnimator_Map::DoRayCast(const EXMatrix &mtx, EXRayCastTemp &t, u32 flags)
{
    const EXGeoComplexRefObject &ref = m_geoRef;
    if (!ref.GeoFile()) return;
    EXGeoHeader *pHdr = ref.GeoFile()->m_pGeoHeader;
    if (!pHdr) return;
    EXGeoMapHeader *pMapHdr = ref.GeoMapHeader();
    if (!pMapHdr) return;
    EXGeoMap *pGeo = pMapHdr->m_Addr[0].m_pObject;
    if (!pGeo) return;
    EXVector end = t.m_start + t.m_dir;
    short zones[64];
    int nZones = pGeo->WhichZones(t.m_start, end, zones);
    for (int z = 0; z < nZones; z++) {
        EXGeoZone *pZone = pGeo->MapZone(zones[z]);
        EXGeoBaseEntity *pEnt = (EXGeoBaseEntity *)pHdr->RefPointer(pZone->m_00);
        if (pEnt && pEnt->HasCollision() && t.m_layer.CalcCollisionEntity(pEnt)) {
            t.m_localStart = t.m_start;
            t.m_localDir = t.m_dir;
            t.CalcBoundsTemp();
            if (pEnt->DoRayCast(t, 0, 0)) {
                t.SetValid(this);
                if (t.m_bAnyHit) return;
            }
        }
        if (t.m_layer.m_flags & 0x800) continue;
        EXu32Array &found = *(EXu32Array *)&m_animators;
        found.m_nSize = 0;
        DoRayCast_PlacementTree(pZone->PlacementInfo()->PlacementTree(), t, found);
        for (int i = 0; i < found.m_nSize; i++) {
            EXMapPlacement &pl = m_zoneTable.m_placements[found[i]];
            if (!pl.m_bounds._Intersect(t.m_bounds)) continue;
            if (!pl.m_bounds._Intersect(t.m_start, t.m_dir)) continue;
            EXGeoBaseEntity *pPlEnt = GeoEntityHeaderOf(pHdr, pl.m_entity)->Object(0);
            if (!pPlEnt || !pPlEnt->HasCollision() || !t.m_layer.CalcCollisionEntity(pPlEnt)) continue;
            // Into the placement's space: subtract its position, then the transposed rotation.
            EXMaths::mat_44 inv = pl.m_mtx.transpose();
            t.m_localStart.x = t.m_start.x - pl.m_mtx.r[3].x;
            t.m_localStart.y = t.m_start.y - pl.m_mtx.r[3].y;
            t.m_localStart.z = t.m_start.z - pl.m_mtx.r[3].z;
            t.m_localStart.w = 1.0f;
            t.m_localStart = inv * t.m_localStart;
            t.m_localDir = inv * t.m_dir;
            t.CalcBoundsTemp();
            if (pPlEnt->DoRayCast(t, 0, 0)) {
                t.SetValid(this);
                if (t.m_bAnyHit) return;
            }
        }
    }
}

// Finds the faces a shadow at t.m_pos falls on: the collision entity of the zone the point is
// in, then (unless the layer excludes placements, 0x800) every placement whose bounds meet a
// 2x2 column around the point between the query's y limits (t.m_height .. t.m_radius). Each
// placement is tested in its own space (point, y limits and stored face distances moved in, the
// faces it finds moved back out).
void EXItemAnimator_Map::DoShadowCast(EXShadowTemp &t)
{
    const EXGeoComplexRefObject &ref = m_geoRef;
    if (!ref.GeoFile()) return;
    EXGeoHeader *pHdr = ref.GeoFile()->m_pGeoHeader;
    if (!pHdr) return;
    EXGeoMapHeader *pMapHdr = ref.GeoMapHeader();
    if (!pMapHdr) return;
    EXGeoMap *pGeo = pMapHdr->m_Addr[0].m_pObject;
    if (!pGeo) return;
    int zone = pGeo->BspTree()->WhichNode(t.m_pos);
    EXGeoBaseEntity *pEnt = (EXGeoBaseEntity *)pHdr->RefPointer(pGeo->MapZone(zone)->m_00);
    if (pEnt && t.m_layer.CalcCollisionEntity(pEnt)) pEnt->DoShadowCast(t);
    if (t.m_layer.m_flags & 0x800) return;
    MXBoundsBox column;
    column.m_vctMin.set(t.m_pos.x - 1.0f, t.m_height, t.m_pos.z - 1.0f, 0.0f);
    column.m_vctMax.set(t.m_pos.x + 1.0f, t.m_radius, t.m_pos.z + 1.0f, 0.0f);
    EXMapPlacementArray &places = m_zoneTable.m_placements;
    for (int i = 0; i < places.Size(); i++) {
        EXMapPlacement &pl = places[i];
        if (!pl.m_bounds._Intersect(column)) continue;
        EXGeoBaseEntity *pPlEnt = GeoEntityHeaderOf(pHdr, pl.m_entity)->Object(0);
        if (!pPlEnt || !pPlEnt->HasCollision() || !t.m_layer.CalcCollisionEntity(pPlEnt)) continue;
        const EXMaths::vec_xyzw pos = pl.m_mtx.r[3];
        EXMaths::mat_44 inv = pl.m_mtx.transpose();
        t.m_pos.x -= pos.x;
        t.m_pos.y -= pos.y;
        t.m_pos.z -= pos.z;
        t.m_pos.w = 0.0f;
        t.m_pos = inv * t.m_pos;
        t.m_height -= pos.y;
        t.m_radius -= pos.y;
        for (int f = 0; f < 4; f++) t.m_faces[f].m_dist -= pos.y;
        unsigned int hits = pPlEnt->DoShadowCast(t);
        if (hits) {
            for (int f = 0; f < 4; f++) {
                if (!(hits & (1 << f))) continue;
                for (int v = 0; v < 3; v++) {
                    EXVector &p = t.m_faces[f].m_v[v];
                    p.w = 0.0f;
                    p = (const EXMaths::mat_44 &)pl.m_mtx * p;
                    p.x += pos.x;
                    p.y += pos.y;
                    p.z += pos.z;
                    p.w += pos.w;
                }
            }
        }
        t.m_pos.w = 0.0f;
        t.m_pos = (const EXMaths::mat_44 &)pl.m_mtx * t.m_pos;
        t.m_pos.x += pos.x;
        t.m_pos.y += pos.y;
        t.m_pos.z += pos.z;
        t.m_pos.w += pos.w;
        t.m_height += pos.y;
        t.m_radius += pos.y;
        for (int f = 0; f < 4; f++) t.m_faces[f].m_dist += pos.y;
    }
}

// The placement nearest p (first of equals), or -1 when there are none.
int EXMapZoneInfoTable::GetMapPlacementIndex(const EXVector &p) const
{
    int found = -1;
    float best = 3.4028235e38f;
    for (int i = 0; i < m_placements.Size(); i++) {
        const EXMaths::vec_xyzw &pos = m_placements[i].m_mtx.r[3];
        float d = (p.y - pos.y) * (p.y - pos.y) + (p.z - pos.z) * (p.z - pos.z) + (p.x - pos.x) * (p.x - pos.x);
        if (d < best) {
            found = i;
            best = d;
        }
    }
    return found;
}

// A zone sound is audible when flagged 0x40 or when its zone is soundable this view.
static inline s8 ZoneSoundAudible(const EXGeoSound &snd, EXZoneInfoArray &zones)
{
    return (snd.m_Flags & 0x40) || zones[snd.m_Zone].m_soundNext;
}

static inline float ZoneSoundDistSq(const EXGeoSound &snd, const EXVector &listener)
{
    float dx = listener.x - snd.m_Pos[0], dy = listener.y - snd.m_Pos[1], dz = listener.z - snd.m_Pos[2];
    return dy * dy + dz * dz + dx * dx;
}

// The point sounds of zone pZone around the listener. Sounds already playing (tagged with their
// index + 1 in the handle's low 28 bits) are kept (update state 1) while audible and in range,
// else marked for killing (-1, unless already set this frame). Audible sounds in range that are
// not playing and not disabled (flag 1) are started: effects at their position, music through the
// item environment, reverb depth directly.
void EXMapZoneInfoTable::UpdateZoneSounds(const EXGeoMap *pGeo, const EXGeoZone *pZone)
{
    EXVector listener = EXItemEnv::m_MapSoundPosition;
    EXBaseSoundManager *pMgr = EXBaseSoundManager::GetSoundManager();
    if (!pMgr) return;
    int n = (short)pZone->m_SoundList.Size();
    for (EXSoundHandle *h = m_sounds.m_pHead; h; h = (EXSoundHandle *)h->m_pNext) {
        s8 bKeep = 0;
        for (int j = 0; j < n; j++) {
            u32 idx = pZone->m_SoundList.Data()[j];
            if ((h->m_Handle & 0xfffffff) != idx + 1) continue;
            const EXGeoSound &snd = pGeo->m_Sounds[idx];
            if (!ZoneSoundAudible(snd, m_zones)) continue;
            bKeep = ZoneSoundDistSq(snd, listener) <= snd.m_MaxDist * snd.m_MaxDist;
            break;
        }
        if (bKeep) h->m_UpdateState = 1;
        else if (!h->m_UpdateState) h->m_UpdateState = -1;
    }
    for (int j = 0; j < n; j++) {
        u32 idx = pZone->m_SoundList.Data()[j];
        const EXGeoSound &snd = pGeo->m_Sounds[idx];
        if (snd.m_Flags & 1) continue;
        if (!ZoneSoundAudible(snd, m_zones)) continue;
        EXSoundHandle *h;
        for (h = m_sounds.m_pHead; h; h = (EXSoundHandle *)h->m_pNext)
            if ((h->m_Handle & 0xfffffff) == idx + 1) break;
        if (h) continue;
        if (ZoneSoundDistSq(snd, listener) > snd.m_MaxDist * snd.m_MaxDist) continue;
        u32 sound = snd.m_Sound;
        switch ((sound >> 24) & 0x7f) {
        case 0x1a: {
            EXSoundHandle *pNew = m_sounds.StartSound(sound, EXVector(snd.m_Pos[0], snd.m_Pos[1], snd.m_Pos[2], 0.0f),
                                                      snd.m_MinDist, snd.m_MaxDist, snd.m_Volume, 0);
            if (pNew) pNew->m_Handle = (idx + 1) | 0x30000000;
            break;
        }
        case 0x1b:
            if (EXItemEnv::GetItemEnv()) EXItemEnv::GetItemEnv()->StartMusic(sound, snd.m_Flags & ~0x80, snd.m_1e, snd.m_1d);
            break;
        case 0x1c:
            pMgr->SfxSetReverbDepth(snd.m_Volume);
            break;
        }
    }
}
