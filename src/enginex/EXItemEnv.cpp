#include "EXItemEnv.h"
#include <float.h>
#include <stdlib.h>
#include "EXItem.h"
#include "EXItemAnimator_Map.h"
#include "EXLightManager.h"
#include "EXDynLight.h"
#include "EXCloth2.h"
#include "EXBaseSoundManager.h"
#include "EXBaseApp.h"
#include "EXGeoComplexRefObject.h"
#include "EXGeoHeader.h"
#include "EXStats.h"
#include "EXMemory.h"
#include "EXItemPhysics.h"
#include "EX6DObject.h"
#include "EXBaseRenderEnv.h"
#include "EXGeoFile.h"
#include "EXGeoMap.h"
#include "EXCamera.h"
#include "EXBaseDisplay.h"
#include "EXDisplay.h"
#include "EXInterObjectTemp.h"
#include "EXFileSys.h"
#include "EXGeoEntity.h"
#include "EXCollisionScoopTemp.h"
#include "EXRayCastTemp.h"
#include "EXGrabPointTemp.h"
#include "EXString.h"
#include "EXHashCodeFile.h"
#include "EXGeoIdentifier.h"

inline EXGeoMap *EXItemEnv::GetGeoMap(EXMapOn m) const
{
    EXItemAnimator_Map *pMap = GetMapItem(m);
    if (!pMap) return 0;
    const EXGeoComplexRefObject &ref = pMap->m_geoRef;
    EXGeoMapHeader *pHdr = ref.GeoMapHeader();
    if (pHdr) return pHdr->m_Addr[0].m_pObject;
    return 0;
}

// Geo map of a map animator (shape of the inlined lookups below).
static inline EXGeoMap *ExGeoMapOf(const EXItemAnimator_Map *pMap)
{
    const EXGeoComplexRefObject &ref = pMap->m_geoRef;
    EXGeoMapHeader *pHdr = ref.GeoMapHeader();
    if (pHdr) return pHdr->m_Addr[0].m_pObject;
    return 0;
}

// EXDataArray::RemoveAt where VC7.1 stops inlining it (RemoveMap).
static __forceinline void ExRemoveAt(EXDataArray<void *, void *> &a, int i)
{
    if (i + 1 < a.m_nSize) memcpy(&a.m_pData[i], &a.m_pData[i + 1], (a.m_nSize - (i + 1)) * sizeof(void *));
    a.m_nSize -= 1;
}

// Pointer-select min/max (retail compares through references).
template <class T> static inline const T &ExMinRef(const T &a, const T &b) { return b <= a ? b : a; }
template <class T> static inline const T &ExMaxRef(const T &a, const T &b) { return a <= b ? b : a; }

// GetGeoMap(s8) as inlined into the index lookups.
static inline EXGeoMap *ExGeoMapById(const EXItemEnv *pEnv, s8 map)
{
    EXItemAnimator_Map *pMap;
    if (map > 0) pMap = pEnv->m_mapRefs.GetData()[map].m_pMap;
    else {
        EXItemAnimator_Map *const *pp = (EXItemAnimator_Map *const *)pEnv->m_activeMaps.GetData();
        if (!pp) return 0;
        pMap = *pp;
    }
    if (!pMap) return 0;
    return ExGeoMapOf(pMap);
}

s8 EXFrameValidFlags::CheckIsValidFrame()
{
    if (m_frame == g_Stats.FrameCount()) return 1;
    m_frame = g_Stats.FrameCount();
    m_flags = 0;
    return 0;
}

void EXItemMapRef::Initialise(const EXGeoRefObject &r, int n)
{
    m_ref = r;
    m_08 = n;
    m_pMap = 0;
}

void EXItemMapRef::DeleteMap()
{
    if (m_pMap) {
        delete m_pMap;
        m_pMap = 0;
    }
}

void EXItemEnvView::UpdateSoundListener(const EXVector &pos, const EXVector &vel, const EXVector &front, const EXVector &top,
                                        const EXVector &right)
{
    if (m_pSoundListener) m_pSoundListener->UpdatePosition(pos, vel, front, top, right);
}

void EXItemEnvView::UpdateSoundListener(const EXVector &pos, const EXVector &vel, const EXMatrix &mtx)
{
    if (m_pSoundListener) m_pSoundListener->UpdatePosition(pos, vel, mtx);
}

void EXItemEnvView::UpdateCameraPosition(const EXCamera &cam)
{
    m_camPos = cam.Position();
    m_camDir = cam.Target();
}

void EXItemEnv::UpdateClothManager()
{
    if (m_pClothManager) m_pClothManager->DoStep(EXBaseApp::FrameLength());
}

void EXItemEnv::AddItem(EXItem *p, u32 list)
{
    m_pItemLists[0].AddTail(p ? &p->m_node : 0);
    p->EnableParticleSystems(1);
    p->UpdateGroups(this, list);
    p->m_listSortValue = 0xff;
}

void EXItemEnv::AddItem(EXItem *p, u32 list, u32 sort)
{
    EXDListItem *pNode = m_pItemLists[0].m_pHead;
    if (pNode) {
        for (EXItem *pItem = (EXItem *)((char *)pNode - 4); pItem; pItem = (EXItem *)pItem->NextItem()) {
            if (sort < pItem->m_listSortValue) {
                m_pItemLists[0].Insert(&pItem->m_node, p ? &p->m_node : 0);
                goto done;
            }
        }
    }
    m_pItemLists[0].AddTail(p ? &p->m_node : 0);
done:
    p->EnableParticleSystems(1);
    p->UpdateGroups(this, list);
    p->m_listSortValue = sort;
}

void EXItemEnv::RemoveItem(EXItem *p)
{
    p->UpdateGroups(this, 0);
    m_pItemLists[0].Remove(p ? &p->m_node : 0);
}

void EXItemEnv::RemoveAllAddedItems()
{
    for (;;) {
        EXDListItem *pNode = m_pItemLists[0].m_pHead;
        EXItem *p = pNode ? (EXItem *)((char *)pNode - 4) : 0;
        if (!p) break;
        p->UpdateGroups(this, 0);
        m_pItemLists[0].Remove(&p->m_node);
        delete p;
    }
}

void EXItemEnv::RenderSky(EXItemAnimator *pSky, const EXMatrix &mtx, const EXRenderInfo &info, const EXMapZoneInfo *) const
{
    pSky->DoItemRender(mtx, info);
}

void EXItemEnv::ProcessSky(EXItemAnimator *pSky) const
{
    pSky->DoItemUpdate();
}

void EXItemEnv::UpdateItems_Physics() const
{
    EXDListItem *pNode = m_pItemLists[0].m_pHead;
    if (!pNode) return;
    for (EXItem *p = (EXItem *)((char *)pNode - 4); p; p = (EXItem *)p->NextItem()) p->DoItemPhysics();
}

// NEAR: structural
void EXItemEnv::UpdateItems_InterObjCollision()
{
    EXInterObjectTemp t;
    t.Initialise();
    for (EXODListItem *pA = m_pItemGroups[4].m_pHead; pA; pA = pA->m_pNext) {
        t.m_pA = ((EXItem *)pA->m_pObject)->ItemPhysics();
        if (!t.m_pA) continue;
        u32 nA = t.m_pA->NumCollisionObjects();
        if (!nA) continue;
        EXVector cA = t.m_pA->BoundsCentre();
        for (EXODListItem *pB = pA->m_pNext; pB; pB = pB->m_pNext) {
            t.m_pB = ((EXItem *)pB->m_pObject)->ItemPhysics();
            if (!t.m_pB) continue;
            u32 nB = t.m_pB->NumCollisionObjects();
            if (!nB) continue;
            if ((t.m_pA->m_physicsFlags & 0x20) && (t.m_pB->m_physicsFlags & 0x20)) continue;
            u16 gA = t.m_pA->InterObjectGroup();
            u16 gB = t.m_pB->InterObjectGroup();
            if (gA != gB && gA && gB) continue;
            EXVector cB = t.m_pB->BoundsCentre();
            float r = t.m_pB->BoundsRadius();
            r = t.m_pA->BoundsRadius() + r;
            EXVector d = cA - cB;
            if (d.LengthXYZSquared() > r * r) continue;
            t.m_numContacts = 0;
            for (u32 i = 0; i < nA; i++) {
                EXCollisionType colA;
                t.m_pA->CollisionObject(i, colA);
                for (u32 j = 0; j < nB; j++) {
                    EXCollisionType colB;
                    t.m_pB->CollisionObject(j, colB);
                    EXVector a, b;
                    float depth;
                    if (colA.Intersection(colB, a, b, depth)) {
                        t.m_pContacts[t.m_numContacts].m_idxA = i;
                        t.m_pContacts[t.m_numContacts].m_idxB = j;
                        t.m_pContacts[t.m_numContacts].m_a = a;
                        t.m_pContacts[t.m_numContacts].m_b = b;
                        t.m_pContacts[t.m_numContacts].m_depth = depth;
                        t.m_numContacts++;
                    }
                }
            }
            if (t.m_numContacts) {
                t.m_which = 0;
                t.m_pA->InterObjectScoopCallback(t);
                t.m_which = 1;
                t.m_pB->InterObjectScoopCallback(t);
            }
        }
    }
}

void EXItemEnv::DoCollisionScoopToContoured(EXCollisionScoopTemp &t, EXItem *pItem) const
{
    EXMatrix m = EXMatrix::MakeIdentity();
    for (EXODListItem *pNode = m_pItemGroups[1].m_pHead; pNode; pNode = pNode->m_pNext) {
        EXItem *p = (EXItem *)pNode->m_pObject;
        if (pItem != p) {
            if (p->ClipType()) p->DoItemCollisionScoop(m, pItem, t);
            else p->DoItemCollisionScoop(m, pItem, t);
        }
    }
}

void EXItemEnv::DoGrabPointToContoured(EXGrabPointTemp &t) const
{
    EXMatrix m = EXMatrix::MakeIdentity();
    for (EXODListItem *pNode = m_pItemGroups[1].m_pHead; pNode; pNode = pNode->m_pNext)
        ((EXItem *)pNode->m_pObject)->DoItemGrabPoint(m, t);
}

void EXItemEnv::DoGrabPointToContoured(EXGrabPointArrayTemp &t) const
{
    EXMatrix m = EXMatrix::MakeIdentity();
    for (EXODListItem *pNode = m_pItemGroups[1].m_pHead; pNode; pNode = pNode->m_pNext)
        ((EXItem *)pNode->m_pObject)->DoItemGrabPoint(m, t);
}

void EXItemEnv::DoRayCastToContoured(EXRayCastTemp &t) const
{
    EXMatrix m = EXMatrix::MakeIdentity();
    s8 bAny = t.m_bAnyHit;
    for (EXODListItem *pNode = m_pItemGroups[1].m_pHead; pNode; pNode = pNode->m_pNext) {
        ((EXItem *)pNode->m_pObject)->DoItemRayCast(m, t);
        if (bAny && t.m_bHit) return;
    }
}

void EXItemEnv::StartMusic(u32 hash, u32 flags, int a, int b)
{
    EXBaseSoundManager *pSnd = EXBaseSoundManager::GetSoundManager();
    if (pSnd && pSnd->MusicHash() != hash) pSnd->StartMusic(hash, a, b, (s8)((flags >> 16) & 1), (s8)((flags >> 7) & 1));
}

EXDynLight *EXItemEnv::NewDynLight()
{
    return m_pLightManager->NewDynLight();
}

EXDynLight *EXItemEnv::NewPointDynLight(ex::XRGBA c, const EXVector &pos, float inner, float outer)
{
    EXDynLight *p = m_pLightManager->NewDynLight();
    if (p) p->SetPointLight(c, pos, inner, outer);
    return p;
}

EXDynLight *EXItemEnv::NewPointDynLight(ex::XRGBA c, float inner, float outer)
{
    EXDynLight *p = m_pLightManager->NewDynLight();
    if (p) p->SetPointLight(c, inner, outer);
    return p;
}

EXDynLight *EXItemEnv::NewDirPointDynLight(ex::XRGBA c, const EXVector &pos, const EXVector &dir, float inner, float outer)
{
    EXDynLight *p = m_pLightManager->NewDynLight();
    if (p) p->SetDirPointLight(c, pos, dir, inner, outer);
    return p;
}

EXDynLight *EXItemEnv::NewDirPointDynLight(ex::XRGBA c, const EXVector &dir, float inner, float outer)
{
    EXDynLight *p = m_pLightManager->NewDynLight();
    if (p) p->SetDirPointLight(c, dir, inner, outer);
    return p;
}

void EXItemEnv::RemoveDynLight(EXDynLight *p) const
{
    m_pLightManager->RemoveDynLight(p);
}

void EXItemEnv::RemoveAllDynLights() const
{
    m_pLightManager->RemoveAllDynLights();
}

void EXItemEnv::SetDynLightRGBMult(const EXVector &v)
{
    m_pLightManager->SetDynLightRGBMult(v);
}

void EXItemEnv::SetDynLightRGBMult(float r, float g, float b)
{
    m_pLightManager->SetDynLightRGBMult(EXVector(r, g, b, 1.0f));
}

void EXItemEnv::EnableDynLightRGBMult(s8 b)
{
    m_pLightManager->EnableDynLightRGBMult(b);
}

void EXItemEnv::RemoveAllNonPersistentItems()
{
    for (;;) {
        EXDListItem *pNode = m_pItemLists[0].m_pHead;
        EXItem *p = pNode ? (EXItem *)((char *)pNode - 4) : 0;
        for (; p; p = (EXItem *)p->NextItem()) {
            if (!(p->m_itemFlags & 0x20)) break;
        }
        if (!p) return;
        p->UpdateGroups(this, 0);
        m_pItemLists[0].Remove(&p->m_node);
        delete p;
    }
}

EXItemAnimator_Map *EXItemEnv::FindMap(EXGeoFile *pFile, u32 hash)
{
    if (pFile && pFile->GeoHeader()) {
        u32 h = pFile->GeoHeader()->GeoObjectHashIndex(hash);
        for (int i = 0; i < m_activeMaps.Size(); i++) {
            EXItemAnimator_Map *pMap = m_activeMaps[i];
            if (pMap->m_geoRef.m_pGeoFile == pFile && pMap->m_geoRef.m_hashRef == h) return pMap;
        }
    }
    return 0;
}

EXItemAnimator_Map *EXItemEnv::GetMapItem(const EXVector &p) const
{
    int n = m_activeMaps.Size();
    if (!n) return 0;
    for (int i = 1; i < n; i++) {
        EXItemAnimator_Map *pMap = ((EXItemAnimator_Map **)m_activeMaps.GetData())[i];
        if (pMap->m_boundsBox._Intersect(p, 0.0f)) return pMap;
    }
    return ((EXItemAnimator_Map **)m_activeMaps.GetData())[0];
}

EXGeoMap *EXItemEnv::GetGeoMap(const EXVector &p) const
{
    EXItemAnimator_Map *pMap = GetMapItem(p);
    if (!pMap) return 0;
    const EXGeoComplexRefObject &ref = pMap->m_geoRef;
    EXGeoMapHeader *pHdr = ref.GeoMapHeader();
    if (pHdr) return pHdr->m_Addr[0].m_pObject;
    return 0;
}

EXMapPlacement *EXItemEnv::GetMapPlacement(const EXVector &p) const
{
    EXItemAnimator_Map *pMap = GetMapItem(p);
    if (!pMap) return 0;
    return pMap->m_zoneTable.GetMapPlacement(p);
}

EXMapOn EXItemEnv::GetMapOn(const EXVector &p) const
{
    EXItemAnimator_Map *pMap = GetMapItem(p);
    EXGeoMap *pGeo = pMap ? ExGeoMapOf(pMap) : 0;
    if (!pGeo) {
        EXMapOn r;
        r.m_v = -1;
        return r;
    }
    return EXMapOn(pMap->m_32c.m_v, ((EXGeoBspTree *)pGeo->m_BspTree)->WhichNode(p));
}

EXItemAnimator *EXItemEnv::CreateSky(EXItemAnimator_Map *pMap, u32 hash, const EXMapZoneInfo *) const
{
    EXGeoFile *pFile = pMap->m_geoRef.m_pGeoFile;
    EXItemAnimator *p = EXItemAnimator::CreateFrom(pFile, hash);
    if (p) {
        EXMatrix m;
        m.SetIdentity();
        p->SetMatrix(m);
    }
    return p;
}

EXGeoPath *EXItemEnv::GetPath(const EXVector &p, u32 flags) const
{
    EXGeoMap *pGeo = GetGeoMap(p);
    if (!pGeo) return 0;
    return pGeo->GetPath(p, flags);
}

int EXItemEnv::GetPathIndex(u32 hash) const
{
    for (int i = 0; i < m_activeMaps.Size(); i++) {
        EXItemAnimator_Map *pMap = m_activeMaps.GetAt(i);
        EXGeoMap *pGeo = ExGeoMapOf(pMap);
        if (pGeo) {
            int n = pGeo->GetPathIndex(hash);
            if (n != -1) return (pMap->m_32c.m_v << 16) | n;
        }
    }
    return -1;
}

int EXItemEnv::GetPathIndex(const EXVector &p, u32 flags) const
{
    EXItemAnimator_Map *pMap = GetMapItem(p);
    if (!pMap) return -1;
    EXGeoMap *pGeo = ExGeoMapOf(pMap);
    if (!pGeo) return -1;
    int n = pGeo->GetPathIndex(p, flags);
    return (pMap->m_32c.m_v << 16) | n;
}

EXGeoCamera *EXItemEnv::GetCamera(const EXVector &p) const
{
    EXGeoMap *pGeo = GetGeoMap(p);
    if (!pGeo) return 0;
    return pGeo->GetCamera(p);
}

int EXItemEnv::GetCameraIndex(u32 hash) const
{
    for (int i = 0; i < m_activeMaps.Size(); i++) {
        EXItemAnimator_Map *pMap = m_activeMaps.GetAt(i);
        EXGeoMap *pGeo = ExGeoMapOf(pMap);
        if (pGeo) {
            int n = pGeo->GetCameraIndex(hash);
            if (n != -1) return (pMap->m_32c.m_v << 16) | n;
        }
    }
    return -1;
}

int EXItemEnv::GetCameraIndex(const EXVector &p) const
{
    EXItemAnimator_Map *pMap = GetMapItem(p);
    if (!pMap) return -1;
    EXGeoMap *pGeo = ExGeoMapOf(pMap);
    if (!pGeo) return -1;
    int n = pGeo->GetCameraIndex(p);
    return (pMap->m_32c.m_v << 16) | n;
}

EXGeoSpecial *EXItemEnv::GetSpecial(const EXVector &p) const
{
    EXGeoMap *pGeo = GetGeoMap(p);
    if (!pGeo) return 0;
    return pGeo->GetSpecial(p);
}

int EXItemEnv::GetSpecialIndex(u32 hash) const
{
    for (int i = 0; i < m_activeMaps.Size(); i++) {
        EXItemAnimator_Map *pMap = m_activeMaps.GetAt(i);
        EXGeoMap *pGeo = ExGeoMapOf(pMap);
        if (pGeo) {
            int n = pGeo->GetSpecialIndex(hash);
            if (n != -1) return (pMap->m_32c.m_v << 16) | n;
        }
    }
    return -1;
}

int EXItemEnv::GetSpecialIndex(const EXVector &p) const
{
    EXItemAnimator_Map *pMap = GetMapItem(p);
    if (!pMap) return -1;
    EXGeoMap *pGeo = ExGeoMapOf(pMap);
    if (!pGeo) return -1;
    int n = pGeo->GetSpecialIndex(p);
    return (pMap->m_32c.m_v << 16) | n;
}

EXGeoPortal *EXItemEnv::GetPortal(const EXVector &p) const
{
    EXGeoMap *pGeo = GetGeoMap(p);
    if (!pGeo) return 0;
    return pGeo->GetPortal(p);
}

int EXItemEnv::GetPortalIndex(const EXVector &p) const
{
    EXItemAnimator_Map *pMap = GetMapItem(p);
    if (!pMap) return -1;
    EXGeoMap *pGeo = ExGeoMapOf(pMap);
    if (!pGeo) return -1;
    int n = pGeo->GetPortalIndex(p);
    return (pMap->m_32c.m_v << 16) | n;
}

EXMapPlacement *EXItemEnv::GetMapPlacement(u32 hash) const
{
    for (int i = 0; i < m_activeMaps.Size(); i++) {
        EXMapPlacement *p = ((EXItemAnimator_Map **)m_activeMaps.m_pData)[i]->m_zoneTable.GetMapPlacement(hash);
        if (p) return p;
    }
    return 0;
}

EXGeoLight *EXItemEnv::GetLight(const EXVector &p) const
{
    EXGeoMap *pGeo = GetGeoMap(p);
    if (!pGeo) return 0;
    return pGeo->GetLight(p);
}

int EXItemEnv::GetLightIndex(u32 hash) const
{
    for (int i = 0; i < m_activeMaps.Size(); i++) {
        EXItemAnimator_Map *pMap = m_activeMaps.GetAt(i);
        EXGeoMap *pGeo = ExGeoMapOf(pMap);
        if (pGeo) {
            int n = pGeo->GetLightIndex(hash);
            if (n != -1) return (pMap->m_32c.m_v << 16) | n;
        }
    }
    return -1;
}

int EXItemEnv::GetLightIndex(const EXVector &p) const
{
    EXItemAnimator_Map *pMap = GetMapItem(p);
    if (!pMap) return -1;
    EXGeoMap *pGeo = ExGeoMapOf(pMap);
    if (!pGeo) return -1;
    int n = pGeo->GetLightIndex(p);
    return (pMap->m_32c.m_v << 16) | n;
}

EXVector EXItemEnv::CalcLightsV(const EXVector &p, const EXVector &n, s8 b, EXMapOn m) const
{
    EXGeoMap *pGeo = GetGeoMap(p);
    if (!pGeo) return EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    return pGeo->CalcLightsV(p, n, b, m.Zone());
}

EXVector EXItemEnv::CalcLightsV(EXLightInfo &info, const EXVector &p, const EXVector &n, s8 b, EXMapOn m) const
{
    EXGeoMap *pGeo = GetGeoMap(p);
    if (!pGeo) return EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    return pGeo->CalcLightsV(info, p, n, b, m.Zone());
}

EXVector EXItemEnv::CalcLightsV(EXLightInfo *pInfo, int num, const EXVector &p, const EXVector &n, s8 b, EXMapOn m) const
{
    EXGeoMap *pGeo = GetGeoMap(p);
    if (!pGeo) return EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    return pGeo->CalcLightsV(pInfo, num, p, n, b, m.Zone());
}

ex::RGBA EXItemEnv::CalcLights(const EXVector &p, const EXVector &n, s8 b, EXMapOn m) const
{
    EXGeoMap *pGeo = GetGeoMap(p);
    if (!pGeo) return ex::RGBA(0u);
    return pGeo->CalcLights(p, n, b, m.Zone());
}

void EXItemEnv::ProcessSounds()
{
    for (int i = 0; i < m_activeMaps.Size(); i++) {
        EXItemAnimator_Map *pMap = m_activeMaps[i];
        if (pMap && (pMap->m_itemFlags & 1)) pMap->m_zoneTable.UpdateSounds();
    }
}

void EXItemEnv::ProcessSkys()
{
    for (int i = 0; i < m_activeMaps.Size(); i++) {
        EXMapZoneInfoTable &t = m_activeMaps[i]->m_zoneTable;
        for (int j = 0; j < t.m_owned.Size(); j++) {
            EXItemAnimator *&pSky = (EXItemAnimator *&)t.m_owned[j];
            if (pSky) {
                if (pSky->m_itemFlags & 0x10) {
                    delete pSky;
                    pSky = 0;
                } else {
                    ProcessSky(pSky);
                }
            }
        }
    }
}

void EXItemEnv::UpdateItems_UpdateCode()
{
    EXDListItem *pNode = m_pItemLists[0].m_pHead;
    if (!pNode) return;
    for (EXItem *p = (EXItem *)((char *)pNode - 4); p; p = (EXItem *)p->NextItem()) {
        if (!(p->m_itemFlags & 0x10)) p->DoItemUpdate();
    }
}

void EXItemEnv::UpdateItems_LastFrame()
{
    EXDListItem *pNode = m_pItemLists[0].m_pHead;
    if (!pNode) return;
    EXItem *p = (EXItem *)((char *)pNode - 4);
    while (p) {
        EXItem *pNext = (EXItem *)p->NextItem();
        if (p->m_itemFlags & 0x10) {
            p->UpdateGroups(this, 0);
            m_pItemLists[0].Remove(&p->m_node);
            delete p;
        }
        p = pNext;
    }
}

void EXItemEnv::UpdateItems_Collision() const
{
    EX6DManager *p6D = m_p6DManager;
    if (p6D && !(p6D->m_flags & 1)) p6D->DoStep(EXBaseApp::FrameLength());
    for (EXODListItem *pNode = m_pItemGroups[0].m_pHead; pNode; pNode = pNode->m_pNext) {
        EXItem *p = (EXItem *)pNode->m_pObject;
        if (!(p->m_itemFlags & 0x10)) {
            EXItemPhysics *pPhys = p->ItemPhysics();
            if (pPhys && !(pPhys->m_physicsFlags & 0x22)) pPhys->DoCollisions();
        }
    }
}

void EXItemEnv::DoCollisionScoopToMap(EXCollisionScoopTemp &t) const
{
    EXMatrix m = EXMatrix::MakeIdentity();
    EXVector p = t.m_xform.GetTranslation();
    EXItemAnimator_Map *pMap = GetMapItem(p);
    if (pMap) pMap->DoEntityScoop(m, t, 0);
}

void EXItemEnv::DoGrabPointToMap(EXGrabPointTemp &t) const
{
    EXMatrix m = EXMatrix::MakeIdentity();
    EXItemAnimator_Map *pMap = GetMapItem(t.m_pos);
    if (pMap) pMap->DoGrabPoint(m, t, 0);
}

void EXItemEnv::DoGrabPointToMap(EXGrabPointArrayTemp &t) const
{
    EXMatrix m = EXMatrix::MakeIdentity();
    EXItemAnimator_Map *pMap = GetMapItem(t.m_pos);
    if (pMap) pMap->DoGrabPoint(m, t, 0);
}

void EXItemEnv::DoRayCastToMap(EXRayCastTemp &t, u32 flags) const
{
    EXMatrix m = EXMatrix::MakeIdentity();
    EXItemAnimator_Map *pMap = GetMapItem(t.m_start);
    if (pMap) pMap->DoRayCast(m, t, flags);
}

void EXItemEnv::DoShadowCastToMap(EXShadowTemp &t) const
{
    EXItemAnimator_Map *pMap = GetMapItem(*(const EXVector *)&t);
    if (pMap) pMap->DoShadowCast(t);
}

void EXItemEnv::InitialiseViewTables()
{
    m_44.Init();
    for (int i = 0; i < m_activeMaps.Size(); i++) m_activeMaps[i]->m_zoneTable.ViewInitialise();
}

void EXItemEnv::ResetViewTables()
{
    for (int i = 0; i < m_activeMaps.Size(); i++) m_activeMaps[i]->m_zoneTable.ViewReset();
}

void EXItemEnv::ProcessViewTables()
{
    for (int i = 0; i < m_activeMaps.Size(); i++) m_activeMaps[i]->m_zoneTable.ViewProcess();
}

void EXItemEnv::ResetEnvViewTables()
{
    for (int j = 0; j < 2; j++) {
        for (int i = 0; i < m_views.Size(); i++) *(u32 *)((char *)m_views[i] + 0x28 + j * 0x10) = 0;
    }
}

float EXItemEnv::MinDistanceToItemViews(const EXVector &p)
{
    float best = 1.0e8f;
    int n = m_34.Size();
    for (int i = 0; i < n; i++) {
        EXVector d = ((EXItemEnvView *)m_34.GetAt(i))->CameraPosition() - p;
        float l = d.x * d.x + (d.y * d.y + d.z * d.z + d.w * d.w);
        best = ExMinRef(l, best);
    }
    return EXSqrt(best);
}

// NEAR: structural (~1100 bytes differ: vector temporaries, stack layout 0xc smaller)
int EXItemEnv::CheckPortalToView(const EXGeoPortal *pPortal, const EXGeoPortalInfo *pInfo, const EXViewPortal &vp,
                                 EXViewPortal &out) const
{
    EXMaths::mat_44 cull = EXBaseDisplay::ObjectToCull();
    float maxZ = 0.0f;
    float minZ = 100000.0f;
    u32 clip = 0;
    int i;
    const EXVector3 *pV = &pPortal->m_Verts[0];
    for (i = 0; i < 32; i += 8, pV++) {
        const EXVector3 &v = *pV;
        EXMaths::vec_xyzw p;
        p.x = v.x;
        p.y = v.y;
        p.z = v.z;
        p.w = 1.0f;
        EXMaths::vec_xyzw t = cull * p;
        clip |= (t.ClipFlag() & 0x3f) << i;
        maxZ = ExMaxRef(maxZ, t.z);
        minZ = ExMinRef(minZ, t.z);
    }
    if (pPortal->m_MinDepth >= 1.0f) {
        const EXGeoPortalTarget *pTarget = pPortal->m_Target;
        if (!pTarget || !(pTarget->m_Flags & 0x10000) || pInfo->m_00 == *(const u16 *)pPortal) {
            if (minZ > pPortal->m_MinDepth) return 0;
        }
    }
    for (i = 0; i < 6; i++) {
        u32 mask = 0x01010101 << i;
        if ((clip & mask) == mask) return 0;
    }
    if (0.0f >= minZ) {
        out = vp;
        return 1;
    }
    EXMaths::mat_44 scr = EXBaseDisplay::ObjectToScreen();
    EXBaseDisplay *pDisp = EXBaseDisplay::GetDisplay();
    EXVector2 mn((float)pDisp->Width(), (float)pDisp->Height());
    EXVector2 mx(0.0f, 0.0f);
    EXVector2 pts[4];
    pV = &pPortal->m_Verts[0];
    for (i = 0; i < 4; i++, pV++) {
        const EXVector3 &v = *pV;
        EXMaths::vec_xyzw p;
        p.x = v.x;
        p.y = v.y;
        p.z = v.z;
        p.w = 1.0f;
        EXMaths::vec_xyzw t = scr * p;
        EXVector d = EXBaseDisplay::PrimToDisp(EXVector(t * (1.0f / t.w)));
        pts[i].x = d.x;
        pts[i].y = d.y;
        mn = Min(mn, pts[i]);
        mx = Max(mx, pts[i]);
    }
    s8 front = (pts[1].y - pts[0].y) * (pts[1].x - pts[2].x) > (pts[1].y - pts[2].y) * (pts[1].x - pts[0].x);
    if ((pInfo->m_07 == 0) ^ (front == 0)) return 0;
    out.m_y0 = mn.y > vp.m_y0 ? mn.y : vp.m_y0;
    out.m_x0 = mn.x > vp.m_x0 ? mn.x : vp.m_x0;
    out.m_y1 = vp.m_y1 > mx.y ? mx.y : vp.m_y1;
    out.m_x1 = vp.m_x1 > mx.x ? mx.x : vp.m_x1;
    out.m_10 = minZ;
    out.m_14 = maxZ;
    if (out.m_x0 > out.m_x1) return 0;
    if (out.m_y0 > out.m_y1) return 0;
    return 1;
}

EXMapOn EXItemEnv::GetConnectingMapOn(const EXItemAnimator_Map *pFrom, const EXGeoPortal *pPortal) const
{
    EXVector c = pPortal->Centre();
    float best = FLT_MAX;
    EXMapOn r;
    r.m_v = -1;
    for (int i = 0; i < m_activeMaps.Size(); i++) {
        EXItemAnimator_Map *pMap = m_activeMaps[i];
        if (pMap != pFrom) {
            EXGeoMap *pGeo = ExGeoMapOf(pMap);
            if (pGeo) {
                EXGeoPortal *p = pGeo->GetPortal(c);
                if (p && ((const s16 *)p)[1] == -1) {
                    EXVector nearest;
                    float d = p->DistanceTo(c, nearest);
                    if (best > d) {
                        best = d;
                        r.Set(pMap->MapId(), *(const u8 *)p);
                    }
                }
            }
        }
    }
    return r;
}

void EXItemEnv::Shutdown()
{
    if (m_pTheItemEnv) {
        m_pTheItemEnv->RemoveAllAddedItems();
        m_pTheItemEnv->RemoveAllMaps();
    }
}

EXItemEnvView::~EXItemEnvView()
{
    if (m_pSoundListener && EXBaseSoundManager::GetSoundManager())
        EXBaseSoundManager::GetSoundManager()->RemoveListener(m_pSoundListener);
    if (m_p44) {
        delete m_p44;
        m_p44 = 0;
    }
}

EXMapOn EXItemEnv::AddGeoMap(const EXGeoHeader *pHdr, const EXGeoMap *pMap)
{
    int i;
    for (i = 0; i < pHdr->MapList().Size(); i++) {
        if (pHdr->GeoMapHeader(i)->Object(0) == pMap) goto found;
    }
    while (1) {}
found:
    u32 hash = i | 0x85000000;
    EXGeoFile *pFile = pHdr->m_pGeoFile;
    int j;
    for (j = 0; j < m_mapRefs.Size(); j++) {
        const EXItemMapRef &r = m_mapRefs.GetData()[j];
        if (r.m_ref.m_pGeoFile == pFile && r.m_ref.m_hashRef == hash) break;
    }
    if (j == m_mapRefs.Size()) {
        EXItemMapRef r;
        r.Initialise(EXGeoRefObject(pFile, hash), j);
        m_mapRefs.Add(r);
    }
    return EXMapOn(j, 0);
}

// NEAR: register allocation (retail keeps pFile in ebp, stores immediate zeros)
void EXItemEnv::RemoveMap(EXGeoFile *pFile, u32 hash)
{
    if (pFile && pFile->GeoHeader()) {
        u32 h = pFile->GeoHeader()->GeoObjectHashIndex(hash);
        for (int i = 0; i < m_activeMaps.Size(); i++) {
            EXItemAnimator_Map *pMap = m_activeMaps[i];
            if (pMap->m_geoRef.m_pGeoFile == pFile && pMap->m_geoRef.m_hashRef == h) {
                ExRemoveAt(m_activeMaps, i);
                break;
            }
        }
        for (int j = 0; j < m_mapRefs.Size(); j++) {
            EXItemMapRef &r = m_mapRefs[j];
            if (r.m_ref.m_pGeoFile == pFile && r.m_ref.m_hashRef == h) {
                r.DeleteMap();
                break;
            }
        }
        if (!m_activeMaps.Size()) m_activeMaps.RemoveAll();
        ResetEnvViewTables();
    }
}

void EXItemEnv::RemoveAllMaps()
{
    for (int i = 0; i < m_activeMaps.Size(); i++) {
        EXItemAnimator_Map *pMap = m_activeMaps[i];
        for (int j = 0; j < m_mapRefs.Size(); j++) {
            if (m_mapRefs.GetData()[j].m_pMap == pMap) {
                m_mapRefs.GetData()[j].DeleteMap();
                break;
            }
        }
    }
    m_activeMaps.RemoveAll();
    ResetEnvViewTables();
}

EXItemAnimator_Map *EXItemEnv::GetMapItem(s8 map) const
{
    if (map > 0) return m_mapRefs.GetData()[map].m_pMap;
    EXItemAnimator_Map *const *pp = (EXItemAnimator_Map *const *)m_activeMaps.GetData();
    if (!pp) return 0;
    return *pp;
}

EXGeoMap *EXItemEnv::GetGeoMap(s8 map) const
{
    EXItemAnimator_Map *pMap = GetMapItem(map);
    if (!pMap) return 0;
    const EXGeoComplexRefObject &ref = pMap->m_geoRef;
    EXGeoMapHeader *pHdr = ref.GeoMapHeader();
    if (pHdr) return pHdr->m_Addr[0].m_pObject;
    return 0;
}

s8 EXItemEnv::IsPortalVisible(int i) const
{
    return IsPortalVisible(EXMapOn(i >> 16, 0), (short)i);
}

void EXItemEnv::UpdateDynLights()
{
    EXItemAnimator_Map *pMap = GetMapItem((s8)0);
    if (!pMap) return;
    EXBaseRenderEnv::SetLightManager(m_pLightManager);
    const EXGeoComplexRefObject &ref = pMap->m_geoRef;
    EXGeoMapHeader *pHdr = ref.GeoMapHeader();
    m_pLightManager->UpdateLightInMapZoneTable(pHdr ? pHdr->m_Addr[0].m_pObject : 0);
}

EXCloth2Manager *EXItemEnv::ClothManager()
{
    if (!m_pClothManager) m_pClothManager = new EXCloth2Manager;
    return m_pClothManager;
}

EXMapOn EXItemEnv::GetMapOn(u32 hash) const
{
    EXItemAnimator_Map *pMap = GetMapItem((s8)0);
    EXGeoMap *pGeo = pMap ? ExGeoMapOf(pMap) : 0;
    if (!pGeo) {
        EXMapOn r;
        r.m_v = -1;
        return r;
    }
    return EXMapOn(0, pGeo->WhichZone(hash));
}

EXGeoPathArray *EXItemEnv::GetPaths(EXMapOn m) const
{
    EXGeoMap *pGeo = GetGeoMap(m);
    if (!pGeo) return 0;
    return (EXGeoPathArray *)&pGeo->m_Paths;
}

EXGeoTrigHeaderArray *EXItemEnv::GetTriggerArray(EXMapOn m) const
{
    EXGeoMap *pGeo = GetGeoMap(m);
    if (!pGeo) return 0;
    return *(const ex::EXRelPtr<EXGeoTrigHeaderArray> *)((const char *)pGeo + 0x58);
}

s8 EXItemEnv::IsZoneVisible(EXMapOn m) const
{
    if (!m.IsValid()) return 0;
    EXItemAnimator_Map *pMap = GetMapItem(m);
    if (!pMap) return 0;
    return pMap->MapZoneInfoTable()->IsZoneVisible(m.Zone()) > 0;
}

s8 EXItemEnv::IsZoneSoundable(EXMapOn m) const
{
    if (!m.IsValid()) return 0;
    EXItemAnimator_Map *pMap = GetMapItem(m);
    if (!pMap) return 0;
    return pMap->MapZoneInfoTable()->IsZoneSoundable(m.Zone()) > 0;
}

s8 EXItemEnv::IsZoneLoaded(EXMapOn m) const
{
    if (!m.IsValid()) return 0;
    EXItemAnimator_Map *pMap = GetMapItem(m);
    if (!pMap) return 0;
    return pMap->MapZoneInfoTable()->IsZoneLoaded(m.Zone()) > 0;
}

EXGeoIdentifier *EXItemEnv::GetZoneIdentifier(EXMapOn m) const
{
    EXGeoMap *pGeo = GetGeoMap(m);
    if (!pGeo) return 0;
    return pGeo->GetZoneIdentifier(m.Zone());
}

s8 EXItemEnv::IsViewVisible(EXMapOn m) const
{
    if (!m.IsValid()) return 0;
    EXItemAnimator_Map *pMap = GetMapItem(m);
    if (!pMap) return 0;
    return pMap->m_zoneTable.m_zones.GetData()[m.Zone()].m_viewNext > 0;
}

s8 EXItemEnv::IsViewSoundable(EXMapOn m) const
{
    if (!m.IsValid()) return 0;
    EXItemAnimator_Map *pMap = GetMapItem(m);
    if (!pMap) return 0;
    return pMap->MapZoneInfoTable()->IsViewSoundable(m.Zone()) > 0;
}

EXGeoPortal *EXItemEnv::GetPortal(int i) const
{
    EXGeoMap *pGeo = GetGeoMap((s8)((u32)i >> 16));
    if (!pGeo) return 0;
    return pGeo->GetPortal(i & 0xff00ffff);
}

// NEAR: 44 bytes differ, loop register choice / scheduling
EXGeoPath *EXItemEnv::GetPath(int i) const
{
    u32 map = GeoMapOnMapId(i);
    if (!map) {
        for (int j = 0; j < m_activeMaps.Size(); j++) {
            EXGeoMap *pGeo = ExGeoMapOf(m_activeMaps.GetAt(j));
            if (pGeo) {
                EXGeoPath *p = pGeo->GetPath((u32)i);
                if (p) return p;
            }
        }
    }
    EXGeoMap *pGeo = ExGeoMapById(this, (s8)map);
    if (!pGeo) return 0;
    return pGeo->GetPath(GeoMapOnCode(i));
}

// NEAR: register allocation differs (retail keeps i in its stack slot, recomputes the map id)
EXGeoCamera *EXItemEnv::GetCamera(int i) const
{
    if (!GeoMapOnMapId(i)) {
        i = GetCameraIndex((u32)i);
        if (i == -1) return 0;
    }
    EXGeoMap *pGeo = GetGeoMap(EXMapOn(GeoMapOnMapId(i), 0));
    if (!pGeo) return 0;
    return pGeo->GetCamera(GeoMapOnCode(i));
}

// NEAR: register allocation differs (retail keeps i in its stack slot, recomputes the map id)
EXGeoSpecial *EXItemEnv::GetSpecial(int i) const
{
    if (!GeoMapOnMapId(i)) {
        i = GetSpecialIndex((u32)i);
        if (i == -1) return 0;
    }
    EXGeoMap *pGeo = GetGeoMap(EXMapOn(GeoMapOnMapId(i), 0));
    if (!pGeo) return 0;
    return pGeo->GetSpecial(GeoMapOnCode(i));
}

// NEAR: register allocation differs (retail keeps i in its stack slot, recomputes the map id)
EXGeoLight *EXItemEnv::GetLight(int i) const
{
    if (!GeoMapOnMapId(i)) {
        i = GetLightIndex((u32)i);
        if (i == -1) return 0;
    }
    EXGeoMap *pGeo = GetGeoMap(EXMapOn(GeoMapOnMapId(i), 0));
    if (!pGeo) return 0;
    return pGeo->GetLight(GeoMapOnCode(i));
}

EXGeoTrigScriptHeader *EXItemEnv::GetTriggerScript(EXMapOn m, int i) const
{
    EXGeoMap *pGeo = GetGeoMap(m);
    if (!pGeo) return 0;
    return pGeo->TriggerScripts(i);
}

EXGeoTriggerDatum *EXItemEnv::GetTriggerCollision(EXMapOn m, int i) const
{
    EXGeoMap *pGeo = GetGeoMap(m);
    if (!pGeo) return 0;
    return pGeo->TriggerCollisions(i);
}

s8 EXItemEnv::IsPortalVisible(EXMapOn m, short portal) const
{
    EXItemAnimator_Map *pMap = GetMapItem(m);
    if (!pMap) return 0;
    EXGeoMap *pGeo = ExGeoMapOf(pMap);
    if (!pGeo) return 1;
    const s16 *p = (const s16 *)pGeo->GetPortal(portal);
    if (!p) return 1;
    if (p[1] == -1) return 1;
    s8 a = pMap->IsZoneVisible(p[1]) > 0;
    s8 b = pMap->IsZoneVisible(p[0]) > 0;
    return a | b;
}

// NEAR: register allocation only (retail reloads the result byte at the loop exit, uses edx for the zone table)
s8 EXItemEnv::IsZoneVisible(const EXVector &p, float r) const
{
    EXItemAnimator_Map *pMap = GetMapItem(p);
    if (!pMap) return 0;
    EXGeoMap *pGeo = ExGeoMapOf(pMap);
    if (!pGeo) return 0;
    EXVector nearest;
    int zone = ((EXGeoBspTree *)pGeo->m_BspTree)->WhichNode(p);
    s8 vis = pMap->IsZoneVisible(zone);
    if (vis > 0) return vis;
    EXGeoZone *pZone = pGeo->MapZone(zone);
    int i = 0;
    while (i < pZone->NumPortals()) {
        EXGeoZonePortalRef *pRefs = pZone->m_PortalRefs;
        if (pRefs[i].m_Zone != -1) {
            s8 vis2 = pMap->IsZoneVisible(pRefs[i].m_Zone);
            if (vis2 > 0) {
                for (int j = 0; j < pRefs[i].m_Extra + 1; j++) {
                    EXGeoPortal *pPortal = pGeo->GetPortal(pRefs[i + j].m_Portal);
                    if (r > pPortal->DistanceTo(p, nearest)) return vis2;
                }
            }
        }
        i += pRefs[i].m_Extra + 1;
    }
    return vis;
}

void EXItemEnv::RenderShadows() const
{
    u32 state = EXBaseRenderEnv::RenderState();
    EXBaseRenderEnv::SetRenderState(state | 4);
    EXMatrix m = EXMatrix::MakeIdentity();
    const EXRenderInfo &info = m_renderInfo;
    for (EXODListItem *pNode = m_pItemGroups[2].m_pHead; pNode; pNode = pNode->m_pNext) {
        EXItem *p = (EXItem *)pNode->m_pObject;
        if (p->m_itemFlags & 0x10) continue;
        EXMapOn on = p->MapOn();
        if (!on.IsValid()) {
            on = GetMapOn(p->m_position);
            if (!on.IsValid()) continue;
        }
        if (IsZoneVisible(on)) {
            if (!(p->DoItemRender(m, info) & 2)) p->m_renderFlags.BitSet(0x20);
        }
    }
    EXBaseRenderEnv::SetRenderState(state);
}

// NEAR: structural (zone caching / clip state flow)
void EXItemEnv::RenderItems()
{
    EXMatrix m = EXMatrix::MakeIdentity();
    EXMaths::mat_44 scr = EXBaseDisplay::ObjectToScreen();
    EXMaths::mat_44 cull = EXBaseDisplay::ViewToCull() * EXBaseDisplay::WorldToView();
    u32 state = EXBaseRenderEnv::RenderState();
    EXMapOn lastOn;
    lastOn.m_v = -1;
    EXMapZoneInfo *pZone = 0;
    EXMapOn zoneOn;
    u32 zones[128];
    EXDListItem *pNode = m_pItemLists[0].m_pHead;
    if (pNode) {
        for (EXItem *p = (EXItem *)((char *)pNode - 4); p; p = (EXItem *)p->NextItem()) {
            u32 flags = p->m_itemFlags.m_flags;
            u32 clip = 0;
            if ((flags & 0x11) != 1) continue;
            EXMapOn mapOn = p->MapOn();
            if (!mapOn.IsValid()) {
                mapOn = GetMapOn(p->m_position);
                if (!mapOn.IsValid()) continue;
            }
            u8 clipType = p->ClipType();
            if (clipType) {
                s8 visible;
                if (flags & 0x40) {
                    visible = IsPortalVisible(mapOn, p->PortalOn());
                } else {
                    switch (clipType) {
                    case 1:
                        p->BuildAnimatorBoundsBox(0);
                        break;
                    case 2: {
                        zoneOn = mapOn;
                        EXItemAnimator_Map *pMap = GetMapItem(mapOn);
                        if (!(pMap->m_zoneTable.IsViewVisible(mapOn.Zone()) > 0)) {
                            EXGeoMap *pGeo = ExGeoMapOf(pMap);
                            u32 n = pGeo->GetMapZoneList(p->m_position, p->m_boundsBox, zones);
                            u32 k;
                            for (k = 1; k < n; k++) {
                                zoneOn = EXMapOn((int)zones[k]);
                                if (IsZoneVisible(zoneOn)) break;
                            }
                            if (k >= n) {
                                p->SetClipFlags(2);
                                continue;
                            }
                        }
                        if (!pZone || zoneOn != lastOn) pZone = GetMapItem(zoneOn)->MapZoneInfo(zoneOn.Zone());
                        p->BuildAnimatorBoundsBox(0);
                        if (!EXGeoMap::ClipPortal(pZone->ViewPortal(), p->m_boundsBox, *(const EXMatrix *)&scr)) clip = 1;
                        break;
                    }
                    default:
                        p->SetClipFlags(clip);
                        continue;
                    }
                    visible = EXDisplay::CullBox((const float *)&p->m_boundsBox, cull) != 0;
                    if (!visible) clip |= 2;
                }
                if (!visible) {
                    p->SetClipFlags(clip);
                    continue;
                }
            }
            if (!pZone || zoneOn != lastOn) {
                EXItemAnimator_Map *pMap = GetMapItem(mapOn);
                if (!pMap) goto done;
                pZone = pMap->MapZoneInfo(mapOn.Zone());
                EXGeoRenderEnv &env = pZone->DrawRender();
                EXBaseRenderEnv::SetFogDistance(env.FogNear(), env.FogFar());
                EXBaseRenderEnv::SetFogMinMax(env.FogMin(), env.FogMax());
                EXBaseRenderEnv::SetFogColor(env.FogColor());
                EXBaseRenderEnv::SetFogEnabled(env.FogEnabled());
                lastOn = zoneOn;
                m_pLightManager->SetupMapZoneLights(mapOn.Zone());
            }
            if (p->m_renderFlags.m_flags & 0x20) {
                p->m_renderFlags.BitClr(0x20);
                EXBaseRenderEnv::SetRenderState(2);
            } else {
                EXBaseRenderEnv::SetRenderState(3);
            }
            p->DoItemRender(m, m_renderInfo);
        done:
            if (clipType) p->SetClipFlags(clip);
        }
    }
    EXBaseRenderEnv::SetRenderState(state);
}

// NEAR: 6 bytes (register choice for the DoItemRender vtable call)
void EXItemEnv::RenderReflections() const
{
    EXMatrix m = EXMatrix::MakeIdentity();
    EXBaseRenderEnv::SetReflectionRender(1);
    EXMatrix flip;
    flip.SetScale(EXVector(1.0f, -1.0f, 1.0f, 1.0f));
    EXMatrix t;
    t.SetTranslate(EXVector(0.0f, EXBaseRenderEnv::ReflectionHeight(), 0.0f, 0.0f));
    EXMatrix refl = t * flip * t.FastInverse();
    EXMatrix saved = EXBaseDisplay::ReflectMatrix();
    EXBaseDisplay::SetReflectMatrix(refl * saved);
    u32 state = EXBaseRenderEnv::RenderState();
    EXBaseRenderEnv::SetRenderState(state | 8);
    for (EXODListItem *pNode = m_pItemGroups[3].m_pHead; pNode; pNode = pNode->m_pNext) {
        EXItem *p = (EXItem *)pNode->m_pObject;
        if (p->m_itemFlags & 0x10) continue;
        EXMapOn on = p->MapOn();
        if (!on.IsValid()) {
            on = GetMapOn(p->m_position);
            if (!on.IsValid()) continue;
        }
        if (IsZoneVisible(on)) {
            if (!(p->DoItemRender(m, m_renderInfo) & 2)) p->m_renderFlags.BitSet(0x20);
        }
    }
    EXBaseDisplay::SetReflectMatrix(saved);
    EXBaseRenderEnv::SetRenderState(state);
    EXBaseRenderEnv::SetReflectionRender(0);
}

void EXItemEnv::ReleaseEnvView(EXItemEnvView *pView)
{
    int i = m_views.Find(pView);
    if (i >= 0) m_views.RemoveAt(i);
    i = m_34.Find(pView);
    if (i >= 0) m_34.RemoveAt(i);
    delete pView;
}

void EXItemEnv::ResetEnvViews()
{
    InitialiseViewTables();
    m_98 = 0;
}

void EXItemEnv::UpdateItems()
{
    UpdateItems_Physics();
    UpdateItems_Collision();
    UpdateItems_UpdateCode();
    UpdateItems_LastFrame();
}

void EXItemEnv::DecodeActiveList(EXItemEnvView *pView, EXMapZoneInfo *pZone, int list)
{
    pView->m_lists[list].Add(pZone);
    EXODList &zones = *(EXODList *)&pZone->m_viewList.m_0c;
    for (EXODListItem *p = zones.RemoveHead(); p; p = zones.RemoveHead())
        DecodeActiveList(pView, (EXMapZoneInfo *)p->m_pObject, list);
}

void EXItemEnv::BuildUsageList(EXItemEnvView *pView, int list)
{
    pView->m_lists[list].SetGrowBy(0x10);
    pView->m_lists[list].Empty(0);
    for (EXODListItem *p = m_44.RemoveHead(); p; p = m_44.RemoveHead())
        DecodeActiveList(pView, (EXMapZoneInfo *)p->m_pObject, list);
}

void EXItemEnv::UpdateView(EXItemEnvView *pView)
{
    ResetViewTables();
    CalculatePortalVisability(pView);
    BuildUsageList(pView, 0);
    CalculatePortalSoundability();
    BuildUsageList(pView, 1);
    ProcessViewTables();
}

void EXItemEnv::RenderSkys(const EXItemEnvView *pView, EXMapZoneInfo *pZone) const
{
    EXMapOn on = pZone->MapOn();
    EXItemAnimator_Map *pMap = GetMapItem(on);
    if (!pMap) return;
    EXGeoMap *pGeo = ExGeoMapOf(pMap);
    if (!pGeo) return;
    EXGeoIdentifier *pId = pGeo->GetZoneIdentifier(on.Zone());
    int sky = pId->m_SkyIndex;
    EXItemAnimator *&pSky = pMap->m_zoneTable.SkyPlacementInfoArray()[sky];
    if (!pSky) {
        pSky = CreateSky(pMap, pGeo->Skys()[sky].m_HashCode, pZone);
        if (!pSky) return;
    }
    EXRenderInfo info;
    EXBaseRenderEnv::SetRenderInfo(info);
    EXGeoRenderEnv &env = pZone->DrawRender();
    EXBaseRenderEnv::SetFogDistance(env.FogNear(), env.FogFar());
    EXBaseRenderEnv::SetFogMinMax(env.FogMin(), env.FogMax());
    EXBaseRenderEnv::SetFogColor(env.FogColor());
    EXBaseRenderEnv::SetFogEnabled(env.FogEnabled());
    pSky->ItemFlags().BitClr(0x10);
    pSky->m_validMtx = 0;
    EXMatrix mtx = pSky->m_mtx;
    if (pId->m_Flags & 1) {
        mtx.r[3].set(pView->m_camPos.x, pId->m_SkyHeight, pView->m_camPos.z, 1.0f);
    }
    RenderSky(pSky, mtx, info, pZone);
}

void EXItemEnv::RenderMaps(EXItemEnvView *pView)
{
    if (!EXBaseDisplay::ActiveWnd()) return;
    EXBaseDisplay *pDisp = EXBaseDisplay::GetDisplay();
    EXMatrix m = EXMatrix::MakeIdentity();
    EXBaseDisplay::SetObjectToWorld(m);
    pDisp->UpdateTransforms();
    u32 fog = 0;
    EXMapZoneInfo *pSkyZone = 0;
    for (int i = 0; i < pView->ZonePtrArray(0).Size(); i++) {
        EXMapZoneInfo *pZone = (EXMapZoneInfo *)pView->ZonePtrArray(0)[i];
        if (pZone->IsLoaded()) {
            EXMapOn on = pZone->MapOn();
            EXItemAnimator_Map *pMap = GetMapItem(on);
            if (pMap->m_itemFlags & 1) {
                EXGeoIdentifier *pId = GetZoneIdentifier(pZone->MapOn());
                if (!fog && (pId->m_Flags & 0x10000)) fog = pId->m_28;
                if (!pSkyZone && pId->m_SkyIndex >= 0) pSkyZone = pZone;
                EXBaseDisplay::SetGeoHeader(pMap->m_geoRef.m_pGeoFile->GeoHeader());
                EXRenderInfo info(pMap->m_renderInfo, m_renderInfo);
                EXGeoMap *pGeo = ExGeoMapOf(pMap);
                pGeo->DrawZone(&pMap->m_zoneTable, pZone, info);
                pGeo->DrawPortals(pZone, info);
            }
        }
    }
    if (pSkyZone) RenderSkys(pView, pSkyZone);
}

EXItemEnvView *EXItemEnv::RegisterEnvView(s8 b)
{
    EXItemEnvView *p = new EXItemEnvView(b);
    if (!p) return 0;
    m_views.Add(p);
    m_34.Add(p);
    return p;
}

// NEAR: 2 instructions differ (address operand order of the map-ref element loads)
EXItemAnimator_Map *EXItemEnv::AddMap(EXGeoFile *pFile, u32 hash, EXItemAnimator_Map *pMap)
{
    if (!pFile->GeoHeader()) {
        EXString s;
        s.Format("EXItemEnv::AddMap: GeoFile Not Loaded %s : #ref 0x%8.8x '%s'", pFile->Filename(), hash,
                 EXHashCodeFile::Debug_HashCodeName(hash));
        while (1) {}
    }
    const u32 &h = pFile->GeoHeader()->GeoObjectHashIndex(hash);
    for (int i = 0; i < m_activeMaps.Size(); i++) {
        EXItemAnimator_Map *p = m_activeMaps[i];
        if (p->m_geoRef.m_pGeoFile == pFile && p->m_geoRef.m_hashRef == h) return p;
    }
    if (!pMap) {
        pMap = (EXItemAnimator_Map *)EXItemAnimator::CreateFrom(pFile, h);
        if (!pMap) {
            EXHashCodeFile::Debug_HashCodeName(hash);
            return 0;
        }
    }
    int j;
    for (j = 0; j < m_mapRefs.Size(); j++) {
        const EXItemMapRef &r = m_mapRefs.GetData()[j];
        if (r.m_ref.m_pGeoFile == pFile && r.m_ref.m_hashRef == h) break;
    }
    if (j == m_mapRefs.Size()) {
        EXHashCodeFile::Debug_HashCodeName(hash);
        return 0;
    }
    pMap->m_geoRef.Set(pFile, h);
    const EXDataArray<EXItemMapRef, const EXItemMapRef &> &refs = m_mapRefs;
    pMap->SetMapId(refs[j].MapId());
    m_mapRefs[j].SetItemMap(pMap);
    m_activeMaps.Add(pMap);
    InitialiseViewTables();
    m_98 = 0;
    ProcessMapZones(0);
    return pMap;
}

EXItemEnvView::EXItemEnvView(s8 b)
{
    m_pSoundListener = EXBaseSoundManager::GetSoundManager()->AllocateListener();
    m_pSoundListener->m_Active = b;
    m_camPos.set_zero();
    m_camDir.set_zero();
    m_48 = 0;
    m_p44 = 0;
}

// NEAR: structural (1121 bytes differ; register allocation / block order, 4 bytes less stack)
void EXItemEnv::RecursePortal(EXItemAnimator_Map *pMap, EXMapZoneInfo *pZone, const EXViewPortal &vp, int depth)
{
    pZone->SetViewPortal(vp);
    pZone->SetViewVisible((s8)depth);
    depth++;
    EXMapOn cur = pZone->MapOn();
    int map = (s8)(cur.m_v >> 8);
    EXMapZoneInfoTable *pTable = pMap->MapZoneInfoTable();
    EXGeoMap *pGeo = ExGeoMapOf(pMap);
    EXGeoZone *pGZone = pGeo->MapZone(pZone->MapOn().Zone());
    pTable->SetOcclusionFlags(pGZone->OcclusionList(), 0);
    int i = 0;
    while (i < pGZone->NumPortals()) {
        const EXGeoPortalInfo *pInfo = pGZone->PortalInfo(i);
        EXMapOn to;
        EXItemAnimator_Map *pToMap;
        EXMapZoneInfoTable *pToTable;
        EXMapZoneInfo *pTo;
        if (pInfo->m_Zone == -1) {
            to = GetConnectingMapOn(pMap, &pGeo->m_Portals[pInfo->m_Portal]);
            if (!to.IsValid()) {
                i += pInfo->m_Extra + 1;
                continue;
            }
            pToMap = GetMapItem(to);
            pTo = pToMap->MapZoneInfo(to.Zone());
            pToTable = pToMap->MapZoneInfoTable();
        } else {
            int zone = pInfo->m_Zone;
            if (pMap->m_zoneTable.IsOcclusionFlagSet(zone)) {
                i += pInfo->m_Extra + 1;
                continue;
            }
            to.Set(map, zone);
            pTo = pMap->MapZoneInfo(zone);
            pToMap = pMap;
            pToTable = pTable;
        }
        int vis = pToTable->IsViewVisible(to.Zone());
        if (vis) vis = depth > abs(vis);
        int found = 0;
        EXViewPortal newVp;
        EXViewPortal tmpVp;
        const EXGeoPortalInfo *pEntry = pInfo;
        for (int j = 0; j < pInfo->m_Extra + 1; j++, pEntry++, i++) {
            const EXGeoPortal *pPortal = &pGeo->m_Portals[pEntry->m_Portal];
            if (pPortal->m_Visited & 1) {
                int zone = pInfo->m_Zone;
                if (zone != -1) {
                    int v = pToTable->IsViewVisible(zone);
                    if (!v) {
                        pToTable->MapZoneInfo(zone)->SetVisible((s8)-depth);
                        CheckPortalToView(pPortal, pEntry, vp, pToTable->MapZoneInfo(zone)->ViewPortal());
                    } else if (v > 0) {
                        EXViewPortal &d = pToTable->MapZoneInfo(zone)->ViewPortal();
                        const EXViewPortal &s = pMap->MapZoneInfoTable()->MapZoneInfo((s16)pInfo->m_00)->ViewPortal();
                        float y0 = s.m_y0 > d.m_y0 ? d.m_y0 : s.m_y0;
                        d.m_x0 = s.m_x0 > d.m_x0 ? d.m_x0 : s.m_x0;
                        d.m_y0 = y0;
                        float y1 = d.m_y1 > s.m_y1 ? d.m_y1 : s.m_y1;
                        d.m_x1 = d.m_x1 > s.m_x1 ? d.m_x1 : s.m_x1;
                        d.m_y1 = y1;
                    }
                }
            } else {
                if (vis) continue;
                EXViewPortal *pOut = found ? &tmpVp : &newVp;
                if (!CheckPortalToView(pPortal, pEntry, vp, *pOut)) continue;
                if (found) {
                    float y0 = tmpVp.m_y0 > newVp.m_y0 ? newVp.m_y0 : tmpVp.m_y0;
                    newVp.m_x0 = tmpVp.m_x0 > newVp.m_x0 ? newVp.m_x0 : tmpVp.m_x0;
                    newVp.m_y0 = y0;
                    float y1 = newVp.m_y1 > tmpVp.m_y1 ? newVp.m_y1 : tmpVp.m_y1;
                    newVp.m_x1 = newVp.m_x1 > tmpVp.m_x1 ? newVp.m_x1 : tmpVp.m_x1;
                    newVp.m_y1 = y1;
                    newVp.m_10 = ExMinRef(newVp.m_10, tmpVp.m_10);
                    newVp.m_14 = ExMaxRef(newVp.m_14, tmpVp.m_14);
                }
                found = 1;
            }
        }
        if (found) {
            pTo->Link().Remove();
            EXODListItem *pNode;
            for (pNode = pZone->ChildList().m_pHead; pNode; pNode = pNode->m_pNext) {
                if (((EXMapZoneInfo *)pNode)->ViewPortal().m_10 > newVp.m_10) {
                    pNode->Insert(&pTo->Link(), pTo);
                    goto added;
                }
            }
            pZone->Link().Add(&pTo->Link(), pTo);
        added:
            if (depth < 16) RecursePortal(pToMap, pTo, newVp, depth);
        }
    }
}

void EXItemEnv::RecurseSfxPortals(EXItemAnimator_Map *pMap, EXMapZoneInfo *pZone, int depth, u32 mask)
{
    if (depth >= 4) return;
    pZone->SetViewSoundable(1);
    depth++;
    EXMapOn cur = pZone->MapOn();
    int map = (s8)(cur.m_v >> 8);
    EXGeoMap *pGeo = ExGeoMapOf(pMap);
    EXGeoZone *pGZone = pGeo->MapZone(pZone->MapOn().Zone());
    int i = 0;
    while (i < pGZone->NumPortals()) {
        EXGeoZonePortalRef *pRefs = pGZone->m_PortalRefs;
        if (pRefs[i].m_Zone != -1) {
            int zone = pRefs[i].m_Zone;
            if (!(pGeo->m_Portals[pRefs[i].m_Portal].m_Visited & 1)) {
                EXMapOn on(map, zone);
                EXMapZoneInfo *pNext = pMap->MapZoneInfo(on.Zone());
                if (!pNext->IsViewSoundable()) {
                    pNext->Link().Remove();
                    pZone->ChildList().AddTail(&pNext->Link(), pNext);
                    u32 newMask = mask;
                    u32 sfx = pGeo->MapZone(zone)->ISoundMask() & ~mask;
                    if (sfx) {
                        newMask = sfx | mask;
                        EXGeoPortal *pBest = 0;
                        float best = 1.0e11f;
                        for (int j = 0; j < pRefs[i].m_Extra + 1; j++) {
                            EXGeoPortal *pPortal = &pGeo->m_Portals[pRefs[i + j].m_Portal];
                            EXVector nearest;
                            float d = pPortal->DistanceTo(m_MapSoundPosition, nearest);
                            if (best > d) {
                                best = d;
                                pBest = pPortal;
                            }
                        }
                        pMap->MapZoneInfoTable()->AddISound(pGeo, pBest, on, sfx, depth + 1);
                    }
                    RecurseSfxPortals(pMap, pNext, depth, newMask);
                }
            }
        }
        i += pRefs[i].m_Extra + 1;
    }
}

// NEAR: scheduling only (retail stores the view-portal width before building the identity matrix)
void EXItemEnv::CalculatePortalVisability(const EXItemEnvView *pView)
{
    EXBaseDisplay *pDisp = EXBaseDisplay::GetDisplay();
    EXMapOn m = GetMapOn(pView->CameraPosition());
    if (!m.IsValid()) return;
    EXViewPortal vp;
    vp.m_x0 = 0.0f;
    vp.m_y0 = 0.0f;
    vp.m_x1 = (float)pDisp->Width();
    vp.m_y1 = (float)pDisp->Height();
    vp.m_10 = 0.0f;
    vp.m_14 = 0.0f;
    EXMatrix mtx = EXMatrix::MakeIdentity();
    EXBaseDisplay::SetObjectToWorld(mtx);
    pDisp->UpdateTransforms();
    EXItemAnimator_Map *pMap = GetMapItem(m);
    if (!pMap) return;
    EXGeoMap *pGeo = ExGeoMapOf(pMap);
    if (!pGeo) return;
    int zone = m.Zone();
    EXMapZoneInfoTable *pTable = pMap->MapZoneInfoTable();
    EXMapZoneInfo *pZone = pTable->MapZoneInfo(zone);
    pZone->Link().Remove();
    m_44.AddTail(&pZone->Link(), pZone);
    pTable->SetOcclusionFlags(pGeo->MapZone(zone)->OcclusionList(), 1);
    RecursePortal(pMap, pZone, vp, 1);
}

void EXItemEnv::CalculatePortalSoundability()
{
    EXMapOn m = GetMapOn(m_MapSoundPosition);
    if (!m.IsValid()) return;
    EXItemAnimator_Map *pMap = GetMapItem(m);
    if (!pMap) return;
    if (!(pMap->m_itemFlags & 1)) return;
    int zone = m.Zone();
    EXMapZoneInfo *pZone = pMap->MapZoneInfo(zone);
    pZone->Link().Remove();
    m_44.AddTail(&pZone->Link(), pZone);
    EXGeoMap *pGeo = ExGeoMapOf(pMap);
    EXGeoZone *pGZone = pGeo->MapZone(zone);
    u32 mask = pGZone->ISoundMask();
    if (mask) {
        pMap->MapZoneInfoTable()->AddISound(pGeo, 0, m, mask, 0);
        RecurseSfxPortals(pMap, pZone, 0, mask);
        pMap->MapZoneInfoTable()->UpdateTrackingSounds(mask);
    } else {
        RecurseSfxPortals(pMap, pZone, 0, 0);
    }
    pMap->MapZoneInfoTable()->UpdateZoneSounds(pGeo, pGZone);
}

// NEAR: structural (section streaming for the views' visible zones and sub-zones)
void EXItemEnv::ProcessMapZones(s8 b)
{
    ProcessSkys();
    EXPtrArray<void> files(0, 4);
    EXPtrArray<EXItemAnimator_Map> maps(0, 4);
    EXPtrArray<void> zones(0, 4);
    int i;
    for (i = 0; i < m_activeMaps.Size(); i++) {
        EXGeoFile *pFile = m_activeMaps[i]->m_geoRef.m_pGeoFile;
        if (pFile->m_pGeoHeader) files.AddUnique(pFile);
    }
    s8 canLoad = EXFileSys::GetFileSys()->GetLoadingStatus() == 0;
    for (int f = 0; f < files.Size(); f++) {
        zones.Empty(0);
        maps.Empty(0);
        EXGeoFile *pFile = (EXGeoFile *)files[f];
        EXGeoHeader *pHdr = pFile->GeoHeader();
        int v;
        for (v = 0; v < m_views.Size(); v++) {
            EXItemEnvView *pView = m_views[v];
            int k;
            for (k = 0; k < pView->ZonePtrArray(0).Size(); k++) {
                EXMapZoneInfo *pZone = (EXMapZoneInfo *)pView->ZonePtrArray(0)[k];
                EXItemAnimator_Map *pMap = GetMapItem(pZone->MapOn());
                if (pMap && pMap->m_geoRef.m_pGeoFile == pFile) {
                    zones.AddUnique(pZone);
                    break;
                }
            }
            for (k = 0; k < pView->ZoneLoadInfo().Size(); k++) {
                EXVector &p = pView->ZoneLoadInfo()[k];
                EXItemAnimator_Map *pMap = GetMapItem(p);
                if (!pMap || pMap->m_geoRef.m_pGeoFile != pFile) continue;
                EXGeoMap *pGeo = ExGeoMapOf(pMap);
                if (!pGeo) continue;
                zones.AddUnique(pMap->MapZoneInfo(pGeo->BspTree()->WhichNode(p)));
                break;
            }
        }
        ex::EXBit128 unload;
        unload.ClrAll();
        ex::EXBit128 load;
        load.ClrAll();
        s8 changed = 0;
        s8 anyLoad = 0;
        if (b && m_activeMaps.Size()) {
            for (int m = 0; m < m_activeMaps.Size(); m++) {
                EXItemAnimator_Map *pMap = m_activeMaps[m];
                if (pHdr != pMap->m_geoRef.m_pGeoFile->GeoHeader()) continue;
                maps.Add(pMap);
                for (int z = 0; z < zones.Size(); z++) {
                    EXMapOn on = ((EXMapZoneInfo *)zones[z])->MapOn();
                    EXGeoMap *pGeo = GetGeoMap(on);
                    if (!pGeo) continue;
                    EXGeoZone *pGZone = pGeo->MapZone(on.Zone());
                    if (!pGZone) continue;
                    for (int w = 0; w < 4; w++) load.m_bits[w] |= pGZone->ZoneSectionList()[w];
                }
            }
            if (maps.Size() && zones.Size()) {
                u32 *pLoaded = pHdr->MapZoneSectionList();
                for (int w = 0; w < 4; w++) {
                    u32 want = load.m_bits[w];
                    u32 have = pLoaded[w];
                    unload.m_bits[w] = have & ~want;
                    load.m_bits[w] = want & ~have;
                    if (load.m_bits[w]) anyLoad = 1;
                }
            }
        }
        u32 *pLoaded = pHdr->MapZoneSectionList();
        for (int sec = 0; sec < 128; sec++) {
            u32 w = unload.m_bits[sec / 32];
            if (!w && !pLoaded[sec / 32]) {
                sec = (sec & ~31) + 31;
                continue;
            }
            u32 id = sec | 0x8000000;
            u32 bit = 1 << (sec & 31);
            u32 state = 0xffffffff;
            if (bit & w) {
                state = pHdr->IsSectionLoaded(id, 0);
                if (state) pHdr->DeLoadSectionFile(id);
            }
            if (pLoaded[sec / 32] & bit) {
                if (state == 0xffffffff) state = pHdr->IsSectionLoaded(id, 0);
                if (!state) pLoaded[sec / 32] &= ~bit;
            }
        }
        for (i = 0; i < maps.Size(); i++) {
            EXItemAnimator_Map *pMap = maps[i];
            EXGeoHeader *pMapHdr = pMap->m_geoRef.m_pGeoFile->GeoHeader();
            EXGeoMap *pGeo = ExGeoMapOf(pMap);
            if (pGeo) changed |= pMap->m_zoneTable.UpdateMapZoneLoaded(pMapHdr, pGeo);
        }
        if (changed) continue;
        if (!anyLoad) continue;
        for (int z = 0; z < zones.Size(); z++) {
            EXMapOn on = ((EXMapZoneInfo *)zones[z])->MapOn();
            EXGeoMap *pGeo = GetGeoMap(on);
            if (!pGeo) continue;
            EXGeoZone *pGZone = pGeo->MapZone(on.Zone());
            if (!pGZone) continue;
            u32 section = pGZone->Section();
            u32 idx = section & 0xffffff;
            u32 bit = 1 << (idx & 31);
            u32 w = idx >> 5;
            if (!(load.m_bits[w] & bit)) continue;
            load.m_bits[w] &= ~bit;
            u32 id = section | 0x8000000;
            if (pHdr->IsSectionLoaded(id, 1) == 3) pHdr->MapZoneSectionList()[w] |= bit;
            else if (canLoad) pHdr->LoadSectionFile(id, 1, 0x7ffffffe);
        }
        if (!canLoad) continue;
        for (int w = 0; w < 4; w++) {
            u32 bits = load.m_bits[w];
            for (int n = 0; bits && n < 32; n++) {
                u32 bit = 1 << n;
                if (!(bits & bit)) continue;
                bits &= ~bit;
                u32 id = (w * 32 + n) | 0x8000000;
                if (pHdr->IsSectionLoaded(id, 1)) continue;
                pHdr->LoadSectionFile(id, 1, 0x7ffffffe);
                pHdr->MapZoneSectionList()[w] |= bit;
            }
        }
    }
    for (i = 0; i < m_activeMaps.Size(); i++) {
        EXItemAnimator_Map *pMap = m_activeMaps[i];
        float bestDist = FLT_MAX;
        u32 bestSection = 0xffffffff;
        EXMapZoneInfo *pBestZone = 0;
        int bestSub = 0;
        s8 placed = 0;
        EXGeoHeader *pHdr = pMap->m_geoRef.m_pGeoFile->GeoHeader();
        EXGeoMap *pGeo = ExGeoMapOf(pMap);
        if (!pGeo) continue;
        for (int z = 0; z < (int)pGeo->NumZones(); z++) {
            EXGeoZone *pGZone = pGeo->MapZone(z);
            if (pHdr->IsSectionLoaded(pGZone->Section(), 1) == 3) {
                placed |= pMap->m_zoneTable.InitialiseMapZonePlacements(pHdr, pGZone, z);
                EXGeoMapZoneEntity *pEnt = (EXGeoMapZoneEntity *)pHdr->RefPointer(pGZone->SubZoneHashRef());
                if (!pEnt || !pEnt->ZonesInSubFiles()) continue;
                s8 near = 0;
                s8 noViews = 1;
                int v;
                for (v = 0; v < m_views.Size(); v++) {
                    EXItemEnvView *pView = m_views[v];
                    for (int c = 0; c < pView->SubZoneLoadInfo().Size(); c++) {
                        noViews = 0;
                        EXBoundsBox box;
                        pView->SubZoneLoadInfo()[c].GetBoundsBox(box, 0);
                        if (pGZone->BoundsBox().Intersect(box)) {
                            near = 1;
                            break;
                        }
                    }
                }
                if (!near && !noViews) goto unloadSubZones;
                {
                    EXMapZoneInfo *pZone = pMap->MapZoneInfo(z);
                    for (u32 s = 0; s < pEnt->NumSubZones(); s++) {
                        EXGeoMapZoneEntity_Node *pNode = pEnt->SubZoneEntity(s);
                        u32 section = pHdr->RefPointerList()[(unsigned int)pNode->m_RefIndex].Section();
                        u32 state = pHdr->IsSectionLoaded(section, 1);
                        float dNear, dFar, dKeep;
                        if (noViews) {
                            dNear = 0.0f;
                            dFar = 0.0f;
                            dKeep = 0.0f;
                        } else {
                            dNear = FLT_MAX;
                            dFar = FLT_MAX;
                            dKeep = FLT_MAX;
                            for (v = 0; v < m_views.Size(); v++) {
                                EXItemEnvView *pView = m_views[v];
                                for (int c = 0; c < pView->SubZoneLoadInfo().Size(); c++) {
                                    EXCollisionType &col = pView->SubZoneLoadInfo()[c];
                                    EXVector centre(pNode->m_x, pNode->m_y, pNode->m_z, 0.0f);
                                    EXVector nearest;
                                    col.NearestTo(centre, nearest);
                                    EXVector d = centre - nearest;
                                    float len = EXSqrt(d.LengthXYZSquared()) - pNode->m_Radius;
                                    dNear = ExMinRef(len, dNear);
                                    float r = len - col.Radius();
                                    dFar = ExMinRef(r, dFar);
                                    r = len - (col.Radius() + 5.0f);
                                    dKeep = ExMinRef(r, dKeep);
                                }
                            }
                            if (!(0.0f >= dFar)) {
                                if (dKeep > 0.0f) {
                                    if (state) {
                                        pHdr->DeLoadSectionFile(section);
                                        pZone->MapSubZoneInfoArray()[s].GDiArray().RemoveAll();
                                    } else {
                                        pZone->MapSubZoneInfoArray().LoadedState().Clr(s);
                                    }
                                }
                                continue;
                            }
                        }
                        if (state) continue;
                        if (0.0f > dNear) {
                            pHdr->LoadSectionFile(section, 0, 0);
                            pZone->MapSubZoneInfoArray().LoadedState().Set(s);
                        } else if (bestDist > dFar) {
                            bestDist = dFar;
                            bestSection = section;
                            pBestZone = pZone;
                            bestSub = s;
                        }
                    }
                }
                continue;
            }
        unloadSubZones:
            {
                EXMapZoneInfo *pZone = pMap->MapZoneInfo(z);
                int w;
                for (w = 0; w < 4; w++)
                    if (pZone->MapSubZoneInfoArray().LoadedState().m_bits[w]) break;
                if (w >= 4) continue;
                EXGeoMapZoneEntity *pEnt = (EXGeoMapZoneEntity *)pHdr->RefPointer(pGZone->SubZoneHashRef());
                if (!pEnt) continue;
                for (u32 s = 0; s < pEnt->NumSubZones(); s++) {
                    if (!pZone->MapSubZoneInfoArray().LoadedState().IsSet(s)) continue;
                    u32 section = pHdr->RefPointerList()[(unsigned int)pEnt->SubZoneEntity(s)->m_RefIndex].Section();
                    if (!pHdr->IsSectionLoaded(section, 1)) continue;
                    pHdr->DeLoadSectionFile(section);
                    pZone->MapSubZoneInfoArray()[s].GDiArray().RemoveAll();
                    pZone->MapSubZoneInfoArray().LoadedState().Clr(s);
                }
            }
        }
        if (bestSection != 0xffffffff) {
            int pri = (int)bestDist;
            if (pri >= 0) pri = 0;
            else if (pri <= -1000) pri = -1000;
            pHdr->LoadSectionFile(bestSection, 1, pri + 0x7ffffffe);
            pBestZone->MapSubZoneInfoArray().LoadedState().Set(bestSub);
        }
        if (placed) {
            EXBoundsBox box;
            box.m_vctMin = EXVector(1000000.0f, 1000000.0f, 1000000.0f, 0.0f);
            box.m_vctMax = EXVector(-1000000.0f, -1000000.0f, -1000000.0f, 0.0f);
            for (int z = 0; z < (int)pGeo->NumZones(); z++)
                pGeo->Initialise_PlacementTree(pGeo->MapZone(z)->PlacementInfo()->PlacementTree(), pHdr, box);
        }
    }
}

void EXItemEnv::Render(EXItemEnvView *pView)
{
    if (!GetMapItem((s8)0)) return;
    EXRenderFogInfo fog = EXBaseRenderEnv::RenderFogInfo();
    EXBaseRenderEnv::SetFogDistance(FLT_MIN, FLT_MAX);
    EXBaseRenderEnv::SetFogMinMax(FLT_MIN, FLT_MAX);
    EXBaseRenderEnv::SetFogColor(ex::RGBA(0x80, 0x80, 0x80, 0x80));
    EXBaseRenderEnv::SetFogEnabled(0);
    UpdateDynLights();
    RenderMaps(pView);
    RenderReflections();
    RenderItems();
    EXBaseRenderEnv::SetLightManager(0);
    EXBaseRenderEnv::SetRenderFogInfo(fog);
}

void EXItemEnv::UpdateEnvView(EXItemEnvView *pView, EXCamera &cam, const EXMatrix &mtx)
{
    if (m_98) ResetEnvViews();
    EXBaseDisplay::GetDisplay()->SetDefaultRenderState();
    EXBaseSoundManager::GetSoundManager()->SfxSetReverbDepth(0);
    cam.UpdateDisplay(0);
    pView->UpdateCameraPosition(cam);
    m_MapSoundPosition = mtx.GetTranslation();
    EXVector vel(0.0f, 0.0f, 0.0f, 0.0f);
    pView->UpdateSoundListener(m_MapSoundPosition, vel, mtx);
    UpdateView(pView);
}

void EXItemEnv::ProcessEnvViews()
{
    m_98 = 1;
    ProcessViewTables();
    ProcessMapZones(1);
    ProcessSounds();
    m_effects.Update();
}

EXItemEnv::~EXItemEnv()
{
    if (m_pItemLists) {
        delete[] m_pItemLists;
        m_pItemLists = 0;
    }
    if (m_pItemGroups) {
        delete[] m_pItemGroups;
        m_pItemGroups = 0;
    }
    RemoveAllMaps();
    if (m_p6DManager) {
        delete m_p6DManager;
        m_p6DManager = 0;
    }
    if (m_pClothManager) {
        delete m_pClothManager;
        m_pClothManager = 0;
    }
    if (m_pLightManager) {
        delete m_pLightManager;
        m_pLightManager = 0;
    }
}

EXItemEnv::EXItemEnv(u32 a, u32 b)
{
    m_9c = 0;
    m_a0 = -1;
    m_pTheItemEnv = this;
    m_activeMaps.SetGrowBy(8);
    m_mapRefs.SetGrowBy(0x40);
    m_mapRefs.AddSpace(1).Initialise(EXGeoRefObject(0, 0), 0);
    m_98 = 1;
    m_p6DManager = 0;
    m_pClothManager = 0;
    m_pLightManager = new EXLightManager;
    m_numItemLists = (u8)(b + 1);
    m_numItemGroups = (u8)(a + 5);
    m_pItemLists = new EXDList[m_numItemLists];
    m_pItemGroups = new EXODList[m_numItemGroups];
}
