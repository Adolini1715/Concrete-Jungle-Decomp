#include "Actor.h"
#include "ActorUtil.h"
#include "ActorLocation.h"
#include "ActorOptions.h"
#include "StateMachine.h"
#include "EXArray.h"
#include "EXNew.h"
#include "EXList.h"
#include "EXTrigger.h"
#include "GeoSectionManager.h"
#include "XItemHandler_Player.h"
#include "XGeoNavMeshEntity.h"
#include "AIControl.h"
#include "XNavMeshHandler.h"
#include "XNavMesh.h"
#include "XNavNetwork.h"
#include "FEText.h"
#include "EXGeoTrigger.h"
#include "XItem.h"
#include "XItemHandler_Environment.h"
#include "EXItemAnimator_Entity.h"
#include "EXItemAnimator_Script.h"
#include "EXDatum.h"
#include "EXTrace.h"
#include "EXApp.h"
#include "EXQMath.h"
#include "EXGeoFile.h"
#include "EXStdFuncs.h"
#include "ActorTimeTable.h"

// Points of interest: locations actors bind to as vendors or customers.

s8 Within(EXVector p, EXGeoTriggerDatum d);  // (Camera.obj)

static int s_POICount = 1;                              // slot 0 unused
static int s_UpdatePOI = 1;                             // next point POI_Update visits
extern unsigned int gCurrentMap_GeoHash;

static EXArray<POI_STRUCT, POI_STRUCT &> s_POIs;
static EXArray<EXTrigger, EXTrigger &> s_DelaySpawn;
POI_GROUP POI_Group[34];
static int s_nDelaySpawn;
static int s_GroupIter;

void POI_ReturnToHubLife(int id)
{
    Actor *pActor = Actor::GetPtrFromID(id);
    if (pActor && (pActor->m_CreationFlags & 0x20)) pActor->m_SM.FlushStack();
}

__declspec(noinline) int POI_AnimFromState(int state)
{
    for (u32 i = 0; i < 15; i++)
        if (FSAttribs[i].m_State == state) return FSAttribs[i].m_Anim;
    return 0;
}

int POI_GetNavZoneIndex(int zone, signed char bAdd)
{
    int slot = 34;
    if (zone >= 0 && zone < slot) return zone;
    for (int i = 2; i < 34; i++) {
        if (slot == 34 && POI_Group[i].m_Zone == 0) slot = i;
        if (POI_Group[i].m_Zone == zone) return i;
    }
    if (bAdd) {
        POI_Group[slot].m_Zone = zone;
        return slot;
    }
    return 0;
}

static int POI_Alloc()
{
    if (s_POICount < 500) {
        POI_STRUCT *p = &s_POIs[s_POICount];
        POI_Group[0].m_List.AddTail(&p->m_Link, p);
        s_POICount++;
        return p->m_ID;
    }
    return 0;
}

POI_STRUCT *POI_Get(int poi)
{
    if (poi > 0 && poi < s_POICount) return &s_POIs[poi];
    return 0;
}

int POI_GetLocFlags(int poi)
{
    int type = s_POIs[poi].m_LocType;
    if (type == -1) return 0;
    return LocationAttribs[type].m_Flags;
}

signed char POI_VendorIsThere(int poi)
{
    if (poi < 1) return 0;
    POI_STRUCT *p = POI_Get(poi);
    for (int i = 0; i < p->m_nVendors; i++)
        if (p->m_Binds[i].m_bBound) return 1;
    return 0;
}

int POI_VendorGetID(int poi, int i)
{
    if (poi < 1 || i < 0 || i >= 4) return 0;
    return POI_Get(poi)->m_Binds[i].m_ID;
}

int POI_VendorStandIndex(int poi, int id)
{
    if (poi < 1 || id < 1) return -1;
    POI_STRUCT *p = POI_Get(poi);
    for (int i = 0; i < p->m_nVendors; i++)
        if (p->m_Binds[i].m_ID == id) return i;
    return -1;
}

EXVector POI_VendorStandPos(int poi, int i)
{
    if (poi < 1 || i < 0) return EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    return POI_Get(poi)->m_Binds[i].m_Pos;
}

int POI_VendorStandPoly(int poi, int i)
{
    if (poi < 1 || i < 0) return -1;
    return POI_Get(poi)->m_Binds[i].m_Poly;
}

signed char POI_CustomerIsThere(int poi)
{
    if (poi < 1) return 0;
    POI_STRUCT *p = POI_Get(poi);
    for (int i = 0; i < p->m_nCustomers; i++)
        if (p->m_Binds[i + 4].m_bBound) return 1;
    return 0;
}

int POI_CustomerStandIndex(int poi, int id)
{
    if (poi < 1 || id < 1) return -1;
    POI_STRUCT *p = POI_Get(poi);
    for (int i = 0; i < p->m_nCustomers; i++)
        if (p->m_Binds[i + 4].m_ID == id) return i;
    return -1;
}

EXVector POI_CustomerStandPos(int poi, int i)
{
    if (poi < 1 || i < 0) return EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    return POI_Get(poi)->m_Binds[i + 4].m_Pos;
}

int POI_CustomerStandPoly(int poi, int i)
{
    if (poi < 1 || i < 0) return -1;
    return POI_Get(poi)->m_Binds[i + 4].m_Poly;
}

int POI_CustomerGetID(int poi, int i)
{
    if (poi < 1 || i < 0) return 0;
    return POI_Get(poi)->m_Binds[i + 4].m_ID;
}

POI_BIND *POI_PatternIteratorNext(int poi)
{
    POI_STRUCT *p = POI_Get(poi);
    int cur = p->m_PatternIter;
    p->m_PatternIter = cur + 1;
    if (cur + 1 >= p->m_nCustomers + p->m_nVendors) p->m_PatternIter = 0;
    if (cur < p->m_nVendors) return &p->m_Binds[cur];
    return &p->m_Binds[cur - p->m_nVendors + 4];
}

int POI_GetComData(int poi, int *a, int *b, int *c)
{
    if (poi < 1) return 0;
    POI_STRUCT *p = POI_Get(poi);
    if (a) *a = p->m_ComData[0];
    if (b) *b = p->m_ComData[1];
    if (c) *c = p->m_ComData[2];
    return p->m_ComState;
}

void POI_SetComData(int poi, int state, int a, int b, int c)
{
    if (poi < 1) return;
    POI_STRUCT *p = POI_Get(poi);
    p->m_ComState = state;
    p->m_ComData[0] = a;
    p->m_ComData[1] = b;
    p->m_ComData[2] = c;
}

void POI_SendMsg(int poi, int except, MsgItem const &msg)
{
    if (poi < 1) return;
    POI_STRUCT *p = POI_Get(poi);
    int i;
    for (i = 0; i < p->m_nCustomers; i++) {
        int id = p->m_Binds[i + 4].m_ID;
        if (id != except && id) {
            Actor *pActor = Actor::GetPtrFromID(id);
            if (pActor) pActor->SendMsg(msg, 0.0f);
        }
    }
    for (i = 0; i < p->m_nVendors; i++) {
        int id = p->m_Binds[i].m_ID;
        if (id != except && id) {
            Actor *pActor = Actor::GetPtrFromID(id);
            if (pActor) pActor->SendMsg(msg, 0.0f);
        }
    }
}

void POI_EnableBinds(int poi, signed char b)
{
    if (poi < 1) return;
    POI_Get(poi)->m_bBindsEnabled = b;
}

int POI_GetActorCount(int poi, int flags)
{
    if (poi < 1) return 0;
    POI_STRUCT *p = POI_Get(poi);
    int n = 0;
    int i;
    if (flags & LocCount::VENDORS) {
        for (i = 0; i < p->m_nVendors; i++) {
            POI_BIND &b = p->m_Binds[i];
            if (b.m_ID && (((flags & LocCount::BOUND) && b.m_bBound) || (flags & LocCount::UNBOUND))) n++;
        }
    }
    if (flags & LocCount::CUSTOMERS) {
        for (i = 0; i < p->m_nCustomers; i++) {
            POI_BIND &b = p->m_Binds[i + 4];
            if (b.m_ID && (((flags & LocCount::BOUND) && b.m_bBound) || (flags & LocCount::UNBOUND))) n++;
        }
    }
    return n;
}

signed char POI_IsFull(int poi, LocCount::Flags flags)
{
    if (poi < 1) return 0;
    POI_STRUCT *p = POI_Get(poi);
    if (POI_GetActorCount(poi, flags) == p->m_nCustomers + p->m_nVendors) return 1;
    return 0;
}

signed char POI_ActorPresent(int poi, int id)
{
    if (poi < 1) return 0;
    POI_STRUCT *p = POI_Get(poi);
    int i;
    for (i = 0; i < p->m_nVendors; i++)
        if (p->m_Binds[i].m_ID == id && p->m_Binds[i].m_bBound) return 1;
    for (i = 0; i < p->m_nCustomers; i++)
        if (p->m_Binds[i + 4].m_ID == id && p->m_Binds[i + 4].m_bBound) return 1;
    return 0;
}

signed char POI_IsVendor(int poi, int id)
{
    if (poi < 1) return 0;
    POI_STRUCT *p = POI_Get(poi);
    for (int i = 0; i < p->m_nVendors; i++)
        if (p->m_Binds[i].m_ID == id) return 1;
    return 0;
}

signed char POI_ForceState(int poi, int data, int id)
{
    if (poi < 1) return 0;
    Actor *pActor = Actor::GetPtrFromID(id);
    if (!pActor || XItemHandler_Player::Instance()->m_SpecialActorID == pActor->m_RefID || pActor->m_Health <= 0.0f ||
        pActor->m_Status == 1)
        return 0;
    POI_STRUCT *p = POI_Get(poi);
    s8 bVendor = 0;
    s8 bCustomer = 0;
    int i;
    for (i = 0; i < p->m_nVendors; i++) {
        if (p->m_Binds[i].m_ID == id) {
            bVendor = 1;
            break;
        }
    }
    for (i = 0; i < p->m_nCustomers; i++) {
        if (p->m_Binds[i + 4].m_ID == id) {
            bCustomer = 1;
            break;
        }
    }
    if (p->m_FlagChannel) {
        if (!bVendor && !bCustomer) goto fail;
        FlagChannels[p->m_FlagChannel] = 1;
    }
    if (bVendor && p->m_VendorState != 0x4d000000) {
        pActor->m_SM.ExtGoto(0, STATE_POIForceState, poi, p->m_VendorState, data, 0);
        return 1;
    }
    if (bCustomer && p->m_CustomerState != 0x4d000000) {
        pActor->m_SM.ExtGoto(0, STATE_POIForceState, poi, p->m_CustomerState, data, 0);
        return 1;
    }
fail:
    return 0;
}

signed char POI_ForceFlagChannel(int poi, int)
{
    if (poi < 1) return 0;
    POI_STRUCT *p = POI_Get(poi);
    if (p->m_ForceChannel) FlagChannels[p->m_ForceChannel] = 1;
    return p->m_ForceChannel > 0;
}

int POI_GetOtherActorID(int poi, int id)
{
    if (poi < 1) return 0;
    POI_STRUCT *p = POI_Get(poi);
    int i;
    for (i = 0; i < p->m_nVendors; i++)
        if (p->m_Binds[i].m_ID && p->m_Binds[i].m_ID != id) return p->m_Binds[i].m_ID;
    for (i = 0; i < p->m_nCustomers; i++)
        if (p->m_Binds[i + 4].m_ID && p->m_Binds[i + 4].m_ID != id) return p->m_Binds[i + 4].m_ID;
    return 0;
}

void POI_AnimAquire(int poi, signed char bVendor)
{
    if (poi < 1) return;
    POI_STRUCT *p = POI_Get(poi);
    int state = bVendor ? p->m_VendorState : p->m_CustomerState;
    if (state) GeoSectionManager::Add(0x100000f, POI_AnimFromState(state));
}

void POI_AnimRelease(int poi, signed char bVendor)
{
    if (poi < 1) return;
    POI_STRUCT *p = POI_Get(poi);
    int state = bVendor ? p->m_VendorState : p->m_CustomerState;
    if (state) GeoSectionManager::Del(0x100000f, POI_AnimFromState(state));
}

signed char POI_AnimReady(int poi, signed char bVendor)
{
    if (poi < 1) return 0;
    POI_STRUCT *p = POI_Get(poi);
    int state = bVendor ? p->m_VendorState : p->m_CustomerState;
    if (state) {
        int anim = POI_AnimFromState(state);
        if (anim) return GeoSectionManager::IsLoaded(0x100000f, anim);
    }
    return 1;
}

void AU_DelaySpawnActor(EXTrigger *pTrig)
{
    s_DelaySpawn[s_nDelaySpawn] = *pTrig;
    EXVector &pos = s_DelaySpawn[s_nDelaySpawn].m_Position;
    pos = AU_SnapFloorPos(pos, 0, 1);
    s_nDelaySpawn++;
}

static void POI_AssignGroup(int poi, int group)
{
    POI_STRUCT *p = POI_Get(poi);
    POI_Group[0].m_List.Remove(&p->m_Link);
    POI_Group[group].m_List.AddTail(&p->m_Link, p);
}

static void POI_UnassignGroup(int poi, int group)
{
    POI_STRUCT *p = POI_Get(poi);
    POI_Group[group].m_List.Remove(&p->m_Link);
    POI_Group[0].m_List.AddTail(&p->m_Link, p);
}

__declspec(noinline) int POI_GetNext(int group, int poi)
{
    if (group == -1) {
        if (poi > 0) {
            EXODListItem *pNext = POI_Get(poi)->m_Link.Next();
            if (pNext) return ((POI_STRUCT *)pNext->Object())->m_ID;
            s_GroupIter++;
        }
        for (; s_GroupIter < 34; s_GroupIter++) {
            if (POI_Group[s_GroupIter].m_List.Head())
                return ((POI_STRUCT *)POI_Group[s_GroupIter].m_List.Head()->Object())->m_ID;
        }
        return 0;
    }
    if (poi < s_POICount) {
        EXODListItem *pNext = POI_Get(poi)->m_Link.Next();
        if (pNext) {
            POI_STRUCT *pn = (POI_STRUCT *)pNext->Object();
            return pn->m_ID < s_POICount ? pn->m_ID : 0;
        }
    }
    return 0;
}

static __forceinline int POI_AnimFromStateInl(int state)
{
    for (u32 i = 0; i < 15; i++)
        if (FSAttribs[i].m_State == state) return FSAttribs[i].m_Anim;
    return 0;
}

signed char POI_VendorUnbind(int poi, int id)
{
    if (poi < 1 || id < 1) return 0;
    POI_STRUCT *p = POI_Get(poi);
    int i;
    for (i = 0; i < p->m_nVendors; i++)
        if (p->m_Binds[i].m_ID == id) goto found;
    return 0;
found:
    Actor *pActor = Actor::GetPtrFromID(id);
    if (pActor) pActor->SetBoundPOI(0);
    if (!(POI_GetLocFlags(poi) & 8)) {
        p->m_Binds[i].m_ID = 0;
    } else {
        POI_BIND *b = p->m_Binds;
        for (int j = 3; j != 0; j--, b++) {
            b->m_ID = b[1].m_ID;
            b->m_bBound = b[1].m_bBound;
        }
        p->m_Binds[3].m_ID = 0;
        p->m_Binds[3].m_bBound = 0;
    }
    int state = POI_Get(poi)->m_VendorState;
    if (state) GeoSectionManager::Del(0x100000f, POI_AnimFromStateInl(state));
    return 1;
}

signed char POI_CustomerUnbind(int poi, int id)
{
    if (poi < 1 || id < 1) return 0;
    POI_STRUCT *p = POI_Get(poi);
    POI_BIND *pCust = &p->m_Binds[4];
    int i;
    for (i = 0; i < p->m_nCustomers; i++)
        if (pCust[i].m_ID == id) goto found;
    return 0;
found:
    if (!(POI_GetLocFlags(poi) & 8)) {
        p->m_Binds[i + 4].m_ID = 0;
    } else {
        Actor *pActor = Actor::GetPtrFromID(id);
        if (pActor) pActor->SetBoundPOI(0);
        POI_BIND *b = pCust;
        for (int j = 7; j != 0; j--, b++) {
            b->m_ID = b[1].m_ID;
            b->m_bBound = b[1].m_bBound;
        }
        p->m_Binds[11].m_ID = 0;
        p->m_Binds[11].m_bBound = 0;
    }
    int state = POI_Get(poi)->m_CustomerState;
    if (state) GeoSectionManager::Del(0x100000f, POI_AnimFromState(state));
    return 1;
}

signed char POI_Unbind(int poi, int id)
{
    if (POI_VendorUnbind(poi, id)) return 1;
    return POI_CustomerUnbind(poi, id) != 0;
}

void POI_OnDeLoadNavMesh(XGeoNavMeshEntity *pNav)
{
    s8 bAll = pNav == 0;
    for (int i = 2; i < 34; i++) {
        if (bAll || pNav->m_wGridSector == pNav->SectorFromZone((u16)POI_Group[i].m_Zone)) {
            int group = POI_GetNavZoneIndex(POI_Group[i].m_Zone, 0);
            EXODListItem *pItem = POI_Group[i].m_List.Head();
            while (pItem) {
                POI_STRUCT *p = (POI_STRUCT *)pItem->Object();
                EXODListItem *pNext = p->m_Link.Next();
                POI_UnassignGroup(p->m_ID, group);
                pItem = pNext;
            }
        }
    }
}

void POI_MissionEnd()
{
    POI_OnDeLoadNavMesh(0);
    for (Actor *pActor = Actor::GetFirst(0); pActor; pActor = pActor->GetNext(0)) {
        if (pActor->m_CreationFlags & 0x200) {
            if (pActor->m_SqDistFromPlayer > pActor->m_AliveRadius * pActor->m_AliveRadius)
                pActor->Delete();
            else
                pActor->m_SM.ExtGoto(0, STATE_MissionEnd, 0, 0, 0, 0);
        }
    }
}

int POI_GetFirst(int group)
{
    if (group == -1) {
        s_GroupIter = 2;
        return POI_GetNext(-1, 0);
    }
    int idx = POI_GetNavZoneIndex(group, 0);
    if (idx) {
        EXODListItem *pHead = POI_Group[idx].m_List.Head();
        if (pHead) return ((POI_STRUCT *)pHead->Object())->m_ID;
    }
    return 0;
}

signed char POI_VendorBind(int poi, int id)
{
    if (poi < 1 || id < 1) return 0;
    POI_STRUCT *p = POI_Get(poi);
    if (!p->m_bBindsEnabled) return 0;
    int i;
    for (i = 0; i < p->m_nVendors; i++) {
        int bound = p->m_Binds[i].m_ID;
        if (bound == id) return 1;
        if (!bound) goto found;
    }
    return 0;
found:
    Actor *pActor = Actor::GetPtrFromID(id);
    if (pActor) {
        if (pActor->BoundPOI()) POI_Unbind(pActor->BoundPOI(), id);
        pActor->SetBoundPOI(poi);
    }
    p->m_Binds[i].m_ID = id;
    int state = POI_Get(poi)->m_VendorState;
    if (state) GeoSectionManager::Add(0x100000f, POI_AnimFromState(state));
    return 1;
}

signed char POI_CustomerBind(int poi, int id)
{
    if (poi < 1 || id < 1) return 0;
    POI_STRUCT *p = POI_Get(poi);
    if (!p->m_bBindsEnabled) return 0;
    int i;
    for (i = 0; i < p->m_nCustomers; i++) {
        int bound = p->m_Binds[i + 4].m_ID;
        if (bound == id) return 1;
        if (!bound) goto found;
    }
    return 0;
found:
    Actor *pActor = Actor::GetPtrFromID(id);
    if (pActor) {
        if (pActor->BoundPOI()) POI_Unbind(pActor->BoundPOI(), id);
        pActor->SetBoundPOI(poi);
    }
    p->m_Binds[i + 4].m_ID = id;
    POI_AnimAquire(poi, 0);
    return 1;
}

void POI_PatternIteratorBind(int poi, POI_BIND *pBind, int id)
{
    if (pBind->m_ID) return;
    pBind->m_ID = id;
    POI_AnimAquire(poi, 1);
    POI_ForceState(poi, 0, id);
    Actor *pActor = Actor::GetPtrFromID(id);
    if (pActor) {
        if (pActor->BoundPOI()) POI_Unbind(pActor->BoundPOI(), id);
        pActor->SetBoundPOI(poi);
    }
}

void POI_OnLoadNavMesh(XGeoNavMeshEntity *)
{
    EXODListItem *pItem = POI_Group[0].m_List.Head();
    while (pItem) {
        POI_STRUCT *p = (POI_STRUCT *)pItem->Object();
        EXODListItem *pNext = p->m_Link.Next();
        POI_SetupNavMesh(p->m_ID);
        pItem = pNext;
    }
}

void POI_PreLoadInit()
{
    s_POICount = 1;
    s_POIs.SetSize(500);
    memset(POI_Group, 0, sizeof(POI_Group));
    for (int g = 0; g < 34; g++) {
        POI_Group[g].m_Zone = 0;
        POI_Group[g].m_List.Init();
    }
    for (int i = 1; i < 500; i++) {
        memset(&s_POIs[i], 0, sizeof(POI_STRUCT));
        s_POIs[i].m_ID = i;
    }
    s_nDelaySpawn = 0;
    s_DelaySpawn.SetSize(100);
}

inline int AIControl::GetJumpToZoneArray(int zone, EXArray<int, int> &a)
{
    XNavMesh *pMesh = gNavMeshHandler.Single();
    if (pMesh && pMesh->NavNetwork()) return pMesh->NavNetwork()->GetJumpToZoneArray(zone, a);
    return 0;
}

// NEAR: register choice (retail keeps 0 in ebx) and the inlined bind count walks from m_bBound.
int POI_GetNearestType(EXVector const &pos, int type, int except, int zone, signed char bAllZones, signed char bNotFull)
{
    int best = 0;
    float bestDist = 0.0f;
    EXArray<int, int> zones;
    if (!bAllZones) {
        zones.SetSize(1);
        zones[0] = zone;
    } else {
        gAIControl.GetJumpToZoneArray(zone, zones);
    }
    for (int i = 0; i < zones.Size(); i++) {
        int group = zones[i];
        for (int poi = POI_GetFirst(group); poi; poi = POI_GetNext(group, poi)) {
            if (poi == except) continue;
            POI_STRUCT *p = &s_POIs[poi];
            if (p->m_Type != type) continue;
            EXVector d = pos - p->m_Pos;
            float dist = d.x * d.x + d.y * d.y + d.z * d.z;
            if (best == 0 || bestDist > dist) {
                if (bNotFull && POI_IsFull(poi, (LocCount::Flags)0xf))
                    continue;
                bestDist = dist;
                best = poi;
            }
        }
    }
    return best;
}

// NEAR: same as POI_GetNearestType: the inlined POI_IsFull bind count walks from m_bBound in retail, and
// the second inlined POI_Get lands in esi.
int POI_GetNearestFlag(EXVector const &pos, int flags, int except, int zone, signed char bAllZones, signed char bNotFull, float radius)
{
    float bestDist = 0.0f;
    int best = 0;

    EXArray<int, int> zones;
    if (!bAllZones) {
        zones.SetSize(1);
        zones[0] = zone;
    } else {
        gAIControl.GetJumpToZoneArray(zone, zones);
    }
    for (int i = 0; i < zones.Size(); i++) {
        int group = zones[i];
        for (int poi = POI_GetFirst(group); poi; poi = POI_GetNext(group, poi)) {
            if (poi == except) continue;
            if (!(flags & POI_GetLocFlags(poi))) continue;
            POI_STRUCT *p = &s_POIs[poi];
            EXVector d = pos - p->m_Pos;
            float dist = d.x * d.x + d.y * d.y + d.z * d.z;
            if (radius > 0.0f && radius * radius > dist) continue;
            if (best == 0 || bestDist > dist) {
                if (bNotFull && POI_IsFull(poi, (LocCount::Flags)0xf))
                    continue;
                bestDist = dist;
                best = poi;
            }
        }
    }
    return best;
}

// NEAR: inlined POI_IsFull differs as in POI_GetNearestType, and best/loop index swap esi/edi.
int POI_GetRandomType(int type, int except, int zone, signed char bAllZones, signed char bNotFull)
{
    int best = 0;
    EXArray<int, int> zones;
    if (!bAllZones) {
        zones.SetSize(1);
        zones[0] = zone;
    } else {
        gAIControl.GetJumpToZoneArray(zone, zones);
    }
    for (int i = 0; i < zones.Size(); i++) {
        int group = zones[i];
        for (int poi = POI_GetFirst(group); poi; poi = POI_GetNext(group, poi)) {
            if (poi == except) continue;
            POI_STRUCT *p = &s_POIs[poi];
            if (p->m_Type != type) continue;
            if (best && p->m_UseCount >= s_POIs[best].m_UseCount) continue;
            if (bNotFull && POI_IsFull(poi, (LocCount::Flags)0xf)) continue;
            best = poi;
        }
    }
    if (best) s_POIs[best].m_UseCount++;
    return best;
}

// As POI_GetNearestFlag, for points on the same level as pos (within 2 units of height), with
// distance measured in x/z.
int POI_GetNearestFlag_Level(EXVector const &pos, int flags, int except, int zone, signed char bAllZones, signed char bNotFull, float radius)
{
    float bestDist = 0.0f;
    int best = 0;

    EXArray<int, int> zones;
    if (!bAllZones) {
        zones.SetSize(1);
        zones[0] = zone;
    } else {
        gAIControl.GetJumpToZoneArray(zone, zones);
    }
    for (int i = 0; i < zones.Size(); i++) {
        int group = zones[i];
        for (int poi = POI_GetFirst(group); poi; poi = POI_GetNext(group, poi)) {
            if (poi == except) continue;
            if (!(flags & POI_GetLocFlags(poi))) continue;
            POI_STRUCT *p = &s_POIs[poi];
            float dx = p->m_Pos.x - pos.x;
            float dz = p->m_Pos.z - pos.z;
            if ((float)fabs(p->m_Pos.y - pos.y) > 2.0f) continue;
            float dist = dz * dz + dx * dx;
            if (radius > 0.0f && radius * radius > dist) continue;
            if (best == 0 || bestDist > dist) {
                if (bNotFull && POI_IsFull(poi, (LocCount::Flags)0xf))
                    continue;
                bestDist = dist;
                best = poi;
            }
        }
    }
    return best;
}

// The nearest point (to pos) with all of the flags that lies within radius of centre.
int POI_GetNearestFlagWithinRadius(EXVector const &pos, EXVector const &centre, float radius, int flags, int except, int zone,
                                   signed char bAllZones, signed char bNotFull)
{
    float bestDist = 0.0f;
    int best = 0;

    EXArray<int, int> zones;
    if (!bAllZones) {
        zones.SetSize(1);
        zones[0] = zone;
    } else {
        gAIControl.GetJumpToZoneArray(zone, zones);
    }
    for (int i = 0; i < zones.Size(); i++) {
        int group = zones[i];
        for (int poi = POI_GetFirst(group); poi; poi = POI_GetNext(group, poi)) {
            if (poi == except) continue;
            if ((POI_GetLocFlags(poi) & flags) != flags) continue;
            POI_STRUCT *p = &s_POIs[poi];
            EXVector d = pos - p->m_Pos;
            EXVector c = centre - p->m_Pos;
            float dist = d.y * d.y + d.z * d.z + d.x * d.x;
            if (c.y * c.y + c.z * c.z + c.x * c.x > radius * radius) continue;
            if (best == 0 || bestDist > dist) {
                if (bNotFull && POI_IsFull(poi, (LocCount::Flags)0xf))
                    continue;
                bestDist = dist;
                best = poi;
            }
        }
    }
    return best;
}

// Re-picks the nearest point with all of the flags, keeping current even when it is full. A point to
// avoid (except) is only avoided if its flags are exactly the ones asked for.
int POI_UpdateNearestFlag(EXVector const &pos, int flags, int current, int except, int zone, signed char bAllZones,
                          signed char bNotFull, float radius)
{
    if (except && flags != POI_GetLocFlags(except)) except = 0;
    float bestDist = 0.0f;
    int best = 0;

    EXArray<int, int> zones;
    if (!bAllZones) {
        zones.SetSize(1);
        zones[0] = zone;
    } else {
        gAIControl.GetJumpToZoneArray(zone, zones);
    }
    for (int i = 0; i < zones.Size(); i++) {
        int group = zones[i];
        for (int poi = POI_GetFirst(group); poi; poi = POI_GetNext(group, poi)) {
            if (poi == except) continue;
            if ((POI_GetLocFlags(poi) & flags) != flags) continue;
            POI_STRUCT *p = &s_POIs[poi];
            EXVector d = pos - p->m_Pos;
            float dist = d.y * d.y + d.z * d.z + d.x * d.x;
            if (radius > 0.0f && radius * radius > dist) continue;
            if (best == 0 || bestDist > dist) {
                if (poi != current && bNotFull && POI_IsFull(poi, (LocCount::Flags)0xf))
                    continue;
                bestDist = dist;
                best = poi;
            }
        }
    }
    return best;
}

// POI_UpdateNearestFlag on pos's level (within 2 units of height), distance in x/z.
int POI_UpdateNearestFlag_Level(EXVector const &pos, int flags, int current, int except, int zone, signed char bAllZones,
                                signed char bNotFull, float radius)
{
    if (except && flags != POI_GetLocFlags(except)) except = 0;
    float bestDist = 0.0f;
    int best = 0;

    EXArray<int, int> zones;
    if (!bAllZones) {
        zones.SetSize(1);
        zones[0] = zone;
    } else {
        gAIControl.GetJumpToZoneArray(zone, zones);
    }
    for (int i = 0; i < zones.Size(); i++) {
        int group = zones[i];
        for (int poi = POI_GetFirst(group); poi; poi = POI_GetNext(group, poi)) {
            if (poi == except) continue;
            if ((POI_GetLocFlags(poi) & flags) != flags) continue;
            POI_STRUCT *p = &s_POIs[poi];
            float dx = p->m_Pos.x - pos.x;
            float dz = p->m_Pos.z - pos.z;
            if ((float)fabs(p->m_Pos.y - pos.y) > 2.0f) continue;
            float dist = dz * dz + dx * dx;
            if (radius > 0.0f && radius * radius > dist) continue;
            if (best == 0 || bestDist > dist) {
                if (poi != current && bNotFull && POI_IsFull(poi, (LocCount::Flags)0xf))
                    continue;
                bestDist = dist;
                best = poi;
            }
        }
    }
    return best;
}

// POI_UpdateNearestFlag limited to points within radius of centre.
int POI_UpdateNearestFlagWithinRadius(EXVector const &pos, EXVector const &centre, float radius, int flags, int current, int except,
                                      int zone, signed char bAllZones, signed char bNotFull)
{
    if (except && flags != POI_GetLocFlags(except)) except = 0;
    float bestDist = 0.0f;
    int best = 0;

    EXArray<int, int> zones;
    if (!bAllZones) {
        zones.SetSize(1);
        zones[0] = zone;
    } else {
        gAIControl.GetJumpToZoneArray(zone, zones);
    }
    for (int i = 0; i < zones.Size(); i++) {
        int group = zones[i];
        for (int poi = POI_GetFirst(group); poi; poi = POI_GetNext(group, poi)) {
            if (poi == except) continue;
            if ((POI_GetLocFlags(poi) & flags) != flags) continue;
            POI_STRUCT *p = &s_POIs[poi];
            EXVector d = pos - p->m_Pos;
            EXVector c = centre - p->m_Pos;
            float dist = d.y * d.y + d.z * d.z + d.x * d.x;
            if (c.y * c.y + c.z * c.z + c.x * c.x > radius * radius) continue;
            if (best == 0 || bestDist > dist) {
                if (poi != current && bNotFull && POI_IsFull(poi, (LocCount::Flags)0xf))
                    continue;
                bestDist = dist;
                best = poi;
            }
        }
    }
    return best;
}

// The least used point with all of the flags (its use count goes up).
int POI_GetRandomFlag(int flags, int except, int zone, signed char bAllZones, signed char bNotFull)
{
    int best = 0;
    EXArray<int, int> zones;
    if (!bAllZones) {
        zones.SetSize(1);
        zones[0] = zone;
    } else {
        gAIControl.GetJumpToZoneArray(zone, zones);
    }
    for (int i = 0; i < zones.Size(); i++) {
        int group = zones[i];
        for (int poi = POI_GetFirst(group); poi; poi = POI_GetNext(group, poi)) {
            if (poi == except) continue;
            if ((POI_GetLocFlags(poi) & flags) != flags) continue;
            POI_STRUCT *p = &s_POIs[poi];
            if (best && p->m_UseCount >= s_POIs[best].m_UseCount) continue;
            if (bNotFull && POI_IsFull(poi, (LocCount::Flags)0xf)) continue;
            best = poi;
        }
    }
    if (best) s_POIs[best].m_UseCount++;
    return best;
}

// The point with location id id, else one of that type: the nearest to pos when given, or the least
// used (a free one first).
int POI_ResolveLocation(int id, int zone, signed char bAllZones, EXVector const *pPos)
{
    int poi;
    EXArray<int, int> zones;
    if (!bAllZones) {
        zones.SetSize(1);
        zones[0] = zone;
    } else {
        gAIControl.GetJumpToZoneArray(zone, zones);
    }
    for (int i = 0; i < zones.Size(); i++) {
        int group = zones[i];
        for (poi = POI_GetFirst(group); poi; poi = POI_GetNext(group, poi)) {
            if (POI_Get(poi)->m_LocID == id) goto done;
        }
    }
    if (!pPos) {
        poi = POI_GetRandomType(id, 0, zone, bAllZones, 1);
        if (!poi) poi = POI_GetRandomType(id, 0, zone, bAllZones, 0);
    } else {
        poi = POI_GetNearestType(*pPos, id, 0, zone, bAllZones, 0);
    }
done:
    return poi;
}

// The nearest evade point to pos that is closer to target than pos is.
int POI_GetNearestEvadeBetweenTarget(EXVector const &pos, EXVector const &target, int except, int zone, signed char bAllZones)
{
    int best = 0;
    EXVector t = pos - target;
    float limit = t.y * t.y + t.z * t.z + t.x * t.x;
    EXArray<int, int> zones;
    if (!bAllZones) {
        zones.SetSize(1);
        zones[0] = zone;
    } else {
        gAIControl.GetJumpToZoneArray(zone, zones);
    }
    float bestDist = limit;
    for (int i = 0; i < zones.Size(); i++) {
        int group = zones[i];
        for (int poi = POI_GetFirst(group); poi; poi = POI_GetNext(group, poi)) {
            if (poi == except) continue;
            POI_STRUCT *p = &s_POIs[poi];
            if (p->m_Type != 0x4200009c) continue;  // HT_LocationType_Evade
            EXVector dt = target - p->m_Pos;
            if (!(dt.y * dt.y + dt.z * dt.z + dt.x * dt.x < limit)) continue;
            EXVector d = pos - p->m_Pos;
            float dist = d.y * d.y + d.z * d.z + d.x * d.x;
            if (dist < bestDist) {
                bestDist = dist;
                best = poi;
            }
        }
    }
    return best;
}

// Matrix times vector ([fi] copy in ActorLocation.obj; not in EXMatrix.h so other units keep
// resolving to mat_44::operator*).
inline EXVector operator*(const EXMatrix &m, const EXVector &v) { return EXVector(m.EXMaths::mat_44::operator*(v)); }

// Per frame: visits the next 25 points (scaled by the frame rate). A point whose spawn conditions
// hold (enough free actors, a profile set, its priority roll, its time slot and flag channels, the
// player just arrived within 40 units) spawns its vendor and customers on the stand pattern slots
// and sends them there; then every bound slot is checked for its actor being in place, and the
// occupied / emptied flag channels are raised.
void POI_Update()
{
    int poi = s_UpdatePOI;
    EXVector player = XItemHandler_Player::GetInstance()->GetItem()->Position();
    s_UpdatePOI += (int)EXBaseApp::GameFrameRateMul() * 25;
    int end = min(s_UpdatePOI, s_POICount);
    int timeSlot = TimeTable_Get();
    if (s_UpdatePOI >= s_POICount) s_UpdatePOI = 1;
    for (; poi < end; poi++) {
        POI_STRUCT *p = POI_Get(poi);
        int avail = TimeTable_GetAvailableActorCount();
        int nSlots = p->m_nCustomers + p->m_nVendors;
        int two = 2;
        if (avail >= min(two, nSlots)
            && (p->m_VendorProfile != 0x47000000 || p->m_CustomerProfile != 0x47000000)  // HT_ActorProfile_NONE
            && (p->m_Priority >= (int)(g_EXRandClass.Rand32() % 101) || gCurrentMap_GeoHash == 0x1000073)  // HT_File_1G_GWarfare
            && (p->m_VolumeData == 0 || p->m_VolumeData == timeSlot)) {
            float dist = (p->m_Pos - player).LengthXZSquared();
            s8 bSpawn = 0;
            if (dist < 1600.0f && !p->m_bPlayerNear) {
                p->m_bPlayerNear = 1;
                bSpawn = 1;
            } else if (dist >= 1600.0f) {
                p->m_bPlayerNear = 0;
                bSpawn = 0;
            }
            if (p->m_SpawnChannel && !FlagChannels[p->m_SpawnChannel]) bSpawn = 0;
            if (p->m_BlockChannel && FlagChannels[p->m_BlockChannel]) bSpawn = 0;
            if (p->m_VendorSpawnID == 0) {
                if (p->m_nVendors > 0 && bSpawn) {
                    POI_BIND *b = POI_PatternIteratorNext(poi);
                    while (!b->m_bVendor) b = POI_PatternIteratorNext(poi);
                    EXVector to = p->m_Pos - b->m_Pos;
                    Actor *pActor = TimeTable_CreateActor(p->m_VendorProfile, b->m_Pos, b->m_Poly, qatan2f(to.x, to.z), 0x36);
                    if (pActor) {
                        avail--;
                        pActor->m_SM.ExtGoto(0, STATE_GotoLocation, poi, 0x122, 0, 0);
                        p->m_VendorSpawnID = pActor->m_RefID;
                    }
                }
            } else if (!Actor::GetPtrFromID(p->m_VendorSpawnID)) {
                p->m_VendorSpawnID = 0;
            }
            int n = min(avail, p->m_MaxCustomerSpawn);
            for (int i = 0; i < n; i++) {
                if (p->m_CustomerSpawnID[i] == 0) {
                    if (p->m_nCustomers > 0 && bSpawn) {
                        POI_BIND *b = POI_PatternIteratorNext(poi);
                        while (b->m_bVendor) b = POI_PatternIteratorNext(poi);
                        EXVector to = p->m_Pos - b->m_Pos;
                        Actor *pActor = TimeTable_CreateActor(p->m_CustomerProfile, b->m_Pos, b->m_Poly, qatan2f(to.x, to.z), 0x36);
                        if (pActor) {
                            pActor->m_SM.ExtGoto(0, STATE_GotoLocation, poi, 0x121, 0, 0);
                            p->m_CustomerSpawnID[i] = pActor->m_RefID;
                        }
                    }
                } else if (!Actor::GetPtrFromID(p->m_CustomerSpawnID[i])) {
                    p->m_CustomerSpawnID[i] = 0;
                }
            }
        }
        int nCust = 0;
        int nVend = 0;
        for (int c = 0; c < p->m_nCustomers; c++) {
            POI_BIND &b = p->m_Binds[4 + c];
            if (!b.m_ID) continue;
            Actor *pActor = Actor::GetPtrFromID(b.m_ID);
            if (!pActor || pActor->m_Health <= 0.0f || pActor->m_ActorFlags2.m_bInPlayersHand) {
                POI_CustomerUnbind(poi, b.m_ID);
            } else {
                if ((pActor->GetFeetPos() - b.m_Pos).LengthXZSquared() < 1.0f && !pActor->IsMoving()) {
                    nCust++;
                    b.m_bBound = 1;
                } else {
                    b.m_bBound = 0;
                }
            }
        }
        for (int v = 0; v < p->m_nVendors; v++) {
            POI_BIND &b = p->m_Binds[v];
            if (!b.m_ID) continue;
            Actor *pActor = Actor::GetPtrFromID(b.m_ID);
            if (!pActor || pActor->m_Health <= 0.0f || pActor->m_ActorFlags2.m_bInPlayersHand) {
                POI_VendorUnbind(poi, b.m_ID);
            } else {
                if ((pActor->GetFeetPos() - b.m_Pos).LengthXZSquared() < 1.0f && !pActor->IsMoving()) {
                    nVend++;
                    b.m_bBound = 1;
                } else {
                    b.m_bBound = 0;
                }
            }
        }
        if (p->m_FlagChannel && nVend + nCust > 0) FlagChannels[p->m_FlagChannel] = 1;
        if (p->m_EmptyChannel && p->m_nBound > 1 && nVend + nCust < 2) FlagChannels[p->m_EmptyChannel] = 1;
        p->m_nBound = nVend + nCust;
    }
}

// Puts a location on the nav mesh: snaps it to the floor (sky spawns keep their height and just
// look up the polygon below), places its customer and vendor slots around it, then files it in the
// group of its nav zone. Vehicle spawns skip all of that and go to group 1.
void POI_SetupNavMesh(int poi)
{
    POI_STRUCT *p = POI_Get(poi);
    if (p->m_Type >= 0x420000a8 && p->m_Type <= 0x420000bc) {  // HT_LocationType_VehicleSpawn1..20
        POI_AssignGroup(p->m_ID, 1);
        return;
    }
    if (!AU_IsNavMeshValid(p->m_Pos)) return;
    s8 bSky = p->m_Type >= 0x420000be && p->m_Type <= 0x420000c7;  // HT_LocationType_SkySpawn1..10
    p->m_PlacedPos = p->m_Pos;
    if (p->m_Pos.y < -3.0e11f) p->m_Pos.y = 0.0f;
    XNavLocation loc;
    if (bSky) {
        p->m_Poly = AU_NavPolyForPos(p->m_Pos);
    } else {
        loc.Ignore(0x48);
        loc.SetPosPoly(p->m_Pos, -1, 0);
        if (loc.IsValid()) {
            p->m_Pos = loc.m_vPos = loc.m_vFloorPos;
            p->m_Poly = loc.GetPolyHandle();
        } else {
            p->m_Pos.y = 0.0f;
            p->m_Poly = -1;
        }
    }
    p->m_Zone = AU_GetNavZone(p->m_Poly);
    for (int i = 0; i < p->m_nCustomers; i++) {
        POI_BIND &b = p->m_Binds[4 + i];
        EXVector off = EXMatrix::RotMatrixY(p->m_YRot) * b.m_Offset;
        if (bSky) {
            b.m_Pos = p->m_Pos + off;
            b.m_Poly = AU_NavPolyForPos(p->m_Pos + off);
        } else {
            b.m_Pos = AU_SnapRelativeToLoc(loc, p->m_Pos + off, &b.m_Poly);
        }
    }
    for (int j = 0; j < p->m_nVendors; j++) {
        POI_BIND &b = p->m_Binds[j];
        EXVector off = EXMatrix::RotMatrixY(p->m_YRot) * b.m_Offset;
        if (bSky) {
            b.m_Pos = off + p->m_Pos;
            b.m_Poly = AU_NavPolyForPos(off + p->m_Pos);
        } else {
            b.m_Pos = AU_SnapRelativeToLoc(loc, p->m_Pos + off, &b.m_Poly);
        }
    }
    int group = POI_GetNavZoneIndex(p->m_Zone, 1);
    POI_AssignGroup(p->m_ID, group);
}

// Fills the stand slots of a location from its stand pattern (an HT_File_StandPatterns entity):
// slot i takes the position of datum hashes[i] when the pattern has it. Pattern 0x2000000 is "none":
// a vendor then gets a single slot on the location itself. Returns the slots found.
static int POI_ExtractStandPos(s8 bVendor, const int *hashes, int count, POI_BIND *binds, int pattern)
{
    memset(binds, 0, count * sizeof(POI_BIND));
    if (pattern == 0x2000000) {
        if (bVendor) {
            binds->m_Offset = EXVector(0.0f, 0.0f, 0.0f, 0.0f);
            binds->m_bVendor = 1;
            return 1;
        }
        return 0;
    }
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(0x100004f, 0, 0);  // HT_File_StandPatterns
    EXItemAnimator_Entity *pAnim = new EXItemAnimator_Entity;
    pAnim->Initialise(pFile, pattern);
    int n = 0;
    for (int i = 0; i < count; i++) {
        EXDatumTemp d;
        memset(&d, 0, sizeof d);
        if (pAnim->GetDatum(hashes[i], d, 0, EXItemAnimator::m_pNullDatumPtr)) {
            binds[i].m_Offset = d.m_position;
            binds[i].m_Offset.w = 0.0f;
            binds[i].m_bVendor = bVendor;
            n++;
        }
    }
    delete pAnim;
    return n;
}

// Creates a location from its placed trigger. Trigger data: 0 type, 1 location id, 2 stand pattern,
// 3/5/9/14/15 flag channels, 4, 7 vendor state, 8 customer state, 10 queue distance, 11, 12,
// 13 effect script; the trigger's entity (m_Unk50 file, m_Hash object) is shown as a display item.
void POI_Add(EXTrigger *pTrig, s8)
{
    int poi = POI_Alloc();
    if (!poi) return;
    int type = pTrig->m_Data[0];
    int locID = pTrig->m_Data[1];
    float yRot = pTrig->m_Rotation.y;
    int pattern = pTrig->m_Data[2];
    int flagChannel = pTrig->m_Data[3];
    int unk4 = pTrig->m_Data[4];
    EXVector pos = pTrig->m_Position;
    int channel2 = pTrig->m_Data[5];
    int vendorState = pTrig->m_Data[7];
    EXVector dir(qsinf(yRot), 0.0f, qcosf(yRot), 0.0f);
    int customerState = pTrig->m_Data[8];
    int forceChannel = pTrig->m_Data[9];
    int queueDist = pTrig->m_Data[10];
    int unk11 = pTrig->m_Data[11];
    int unk12 = pTrig->m_Data[12];
    int effect = pTrig->m_Data[13];
    int channel4 = pTrig->m_Data[14];
    int channel5 = pTrig->m_Data[15];
    s_POIs[poi].m_Unk2e8 = 0;
    s_POIs[poi].m_bPlayerNear = 0;
    s_POIs[poi].m_ID = poi;
    s_POIs[poi].m_Pos = pos;
    s_POIs[poi].m_Dir = dir;
    s_POIs[poi].m_YRot = yRot;
    s_POIs[poi].m_Type = type;
    s_POIs[poi].m_LocID = locID;
    s_POIs[poi].m_pName = "LocX";
    s_POIs[poi].m_Priority = 100;
    s_POIs[poi].m_LocType = -1;
    s_POIs[poi].m_FlagChannel = flagChannel;
    s_POIs[poi].m_EmptyChannel = channel2;
    s_POIs[poi].m_ForceChannel = forceChannel;
    s_POIs[poi].m_VendorState = vendorState;
    s_POIs[poi].m_CustomerState = customerState;
    s_POIs[poi].m_UseCount = 0;
    s_POIs[poi].m_bBindsEnabled = 1;
    s_POIs[poi].m_VendorProfile = unk4;
    s_POIs[poi].m_VendorSpawnID = 0;
    s_POIs[poi].m_QueueDist = queueDist;
    s_POIs[poi].m_pItem = 0;
    s_POIs[poi].m_pFX = 0;
    s_POIs[poi].m_PatternIter = 0;
    s_POIs[poi].m_nBound = 0;
    s_POIs[poi].m_CustomerProfile = unk11;
    s_POIs[poi].m_MaxCustomerSpawn = unk12;
    s_POIs[poi].m_SpawnChannel = channel4;
    s_POIs[poi].m_BlockChannel = channel5;
    s_POIs[poi].m_VolumeData = 0;
    if (pTrig->m_Hash > 0 || (effect > 0x4000000 && effect < 0x40007d9)) {
        s_POIs[poi].m_pItem = XItem::CreateItem(0, 0, 0, EXVector(0.0f, 0.0f, 0.0f, 0.0f));
        s_POIs[poi].m_pItem->SetPosition(pTrig->m_Position);
        s_POIs[poi].m_pItem->SetRotation(*(const EXQuaternion *)&pTrig->m_Rotation);
        s_POIs[poi].m_pItem->SetMapOn(theItemEnv.GetMapOn(pTrig->m_Position));
        if (pTrig->m_Hash > 0) {
            EXGeoFile *pFile;
            if (pTrig->m_Unk50) pFile = EXGeoFile::GetGeoFile(pTrig->m_Unk50);
            else pFile = EXGeoFile::GetGeoFile(gCurrentMap_GeoHash);
            if (pTrig->m_Hash > 0x4000000 && pTrig->m_Hash < 0x40007d9) {
                EXItemAnimator_Script *pAnim = EXItemAnimator_Script::NewItem();
                pAnim->Initialise(pFile, pTrig->m_Hash);
                s_POIs[poi].m_pItem->AddAnimator(pAnim);
            } else {
                EXItemAnimator_Entity *pAnim = new EXItemAnimator_Entity;
                pAnim->Initialise(pFile, pTrig->m_Hash);
                s_POIs[poi].m_pItem->AddAnimator(pAnim);
            }
        }
        if (effect > 0x4000000 && effect < 0x40007d9) {
            EXGeoFile *pFile = EXGeoFile::GetGeoFile(0x1000016);  // HT_File_Pred_FX
            s_POIs[poi].m_pFX = EXItemAnimator_Script::NewItem();
            s_POIs[poi].m_pFX->Initialise(pFile, effect);
            s_POIs[poi].m_pFX->RenderFlags().BitSet(1);
            s_POIs[poi].m_pFX->SetPosition(EXVector(0.0f, 3.0f, 0.0f, 0.0f));
            s_POIs[poi].m_pItem->AddAnimator(s_POIs[poi].m_pFX);
        }
        if (s_POIs[poi].m_pItem) {
            s_POIs[poi].m_pItem->SetHasRender();
            theItemEnv.AddItem(s_POIs[poi].m_pItem, 2);
        }
    }
    for (int i = 0; LocationAttribs[i].m_Hash; i++) {
        if (LocationAttribs[i].m_Hash == s_POIs[poi].m_Type) {
            s_POIs[poi].m_LocType = i;
            s_POIs[poi].m_pName = LocationAttribs[i].m_pName;
            s_POIs[poi].m_Priority = LocationAttribs[i].m_Priority;
            if (pattern == 0x2000000) pattern = LocationAttribs[i].m_Pattern;
            break;
        }
    }
    if (flagChannel < 0 || flagChannel >= 290) {
        ex::TRACE("bad flag channel at %f %f %f", ex::fd(s_POIs[poi].m_Pos.x), ex::fd(s_POIs[poi].m_Pos.y), ex::fd(s_POIs[poi].m_Pos.z));
        ex::EXDebugBreak();
    }
    if (channel2 < 0 || channel2 >= 290) {
        ex::TRACE("bad flag channel at %f %f %f", ex::fd(s_POIs[poi].m_Pos.x), ex::fd(s_POIs[poi].m_Pos.y), ex::fd(s_POIs[poi].m_Pos.z));
        ex::EXDebugBreak();
    }
    if (forceChannel < 0 || forceChannel >= 290) {
        ex::TRACE("bad flag channel at %f %f %f", ex::fd(s_POIs[poi].m_Pos.x), ex::fd(s_POIs[poi].m_Pos.y), ex::fd(s_POIs[poi].m_Pos.z));
        ex::EXDebugBreak();
    }
    if (channel4 < 0 || channel4 >= 290) {
        ex::TRACE("bad flag channel at %f %f %f", ex::fd(s_POIs[poi].m_Pos.x), ex::fd(s_POIs[poi].m_Pos.y), ex::fd(s_POIs[poi].m_Pos.z));
        ex::EXDebugBreak();
    }
    if (channel5 < 0 || channel5 >= 290) {
        ex::TRACE("bad flag channel at %f %f %f", ex::fd(s_POIs[poi].m_Pos.x), ex::fd(s_POIs[poi].m_Pos.y), ex::fd(s_POIs[poi].m_Pos.z));
        ex::EXDebugBreak();
    }
    int vendors[4] = { 0x10000059, 0x1000005a, 0x10000058, 0x1000005f };  // HT_AnimDatum_SP_Vendor1..4
    s_POIs[poi].m_nVendors = POI_ExtractStandPos(1, vendors, 4, &s_POIs[poi].m_Binds[0], pattern);
    int customers[8] = { 0x1000005c, 0x1000005b, 0x1000005d, 0x1000005e,
                         0x10000061, 0x10000062, 0x10000063, 0x10000064 };  // HT_AnimDatum_SP_Customer1..8
    s_POIs[poi].m_nCustomers = POI_ExtractStandPos(0, customers, 8, &s_POIs[poi].m_Binds[4], pattern);
    POI_SetupNavMesh(poi);
}

// After a level loads: rebuilds the nav data, trims the point table, records for each point the
// type 10 environment volume (if any) that contains it, then spawns the actors that were waiting
// for the load and frees the queue.
void POI_PostLoadInit()
{
    AU_IsNavMeshValid(EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    POI_OnLoadNavMesh(0);
    s_POIs.SetSize(s_POICount, -1);
    for (int i = 1; i < s_POICount; i++) {
        POI_STRUCT *p = POI_Get(i);
        if (!p) continue;
        p->m_VolumeData = 0;
        for (XItem *pItem = gEnvList.GetFirst(); pItem; pItem = gEnvList.GetNext(pItem)) {
            EXItemHandler *h = pItem->ItemHandler();
            if (!h || !h->IsKindOf(EXRUNTIME_CLASS(XItemHandler_Environment))) continue;
            XItemHandler_Environment *pEnv = (XItemHandler_Environment *)pItem->ItemHandler();
            if (!pEnv || pEnv->Type() != 10) continue;
            if (Within(p->m_Pos, *(EXGeoTriggerDatum *)&pEnv->m_Datum)) {
                p->m_VolumeData = pEnv->m_Unk6c;
                break;
            }
        }
    }
    for (int k = 0; k < s_nDelaySpawn; k++) AU_SpawnActor(&s_DelaySpawn[k], 0);
    s_DelaySpawn.RemoveAll();
}
