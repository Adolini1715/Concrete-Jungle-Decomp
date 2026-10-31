#include "XItem.h"
#include "XItemHandler_Player.h"
#include "XItemHandler_AirVehicle.h"
#include "EXTrigger.h"
#include "EXGeoFile.h"
#include "EXItemAnimator.h"
#include "EXItemAnimator_Script.h"
#include "EXDatum.h"
#include "EXQuaternion.h"
#include "EXMemory.h"
#include "FEText.h"
#include "EXGeoMap.h"
#include "EXGeoPathNode.h"
#include "EXGeoHeader.h"
#include "EXIntersection.h"
#include "EXSphere.h"
#include "EXLine.h"
#include "EXRand.h"
#include "ActorLocation.h"
#include "ActorProfile.h"
#include "ActorOptions.h"
#include "GeoSectionManager.h"
#include "EXBaseRenderEnv.h"
#include "EXBaseApp.h"
#include "XItemHandler_Objective.h"
#include "EXArray.h"


// Anim hashes of each class of air traffic (class 0 is residential plus commercial).
unsigned int TrafficEverything[10] = { 0x2000103, 0x2000170, 0x200017a, 0x20001de, 0x20001e2, 0x20001fb, 0x2000171, 0x2000172, 0x2000173, 0x2000174 };
unsigned int TrafficResidential[6] = { 0x2000103, 0x2000170, 0x200017a, 0x20001de, 0x20001e2, 0x20001fb };
unsigned int TrafficCommercial[4] = { 0x2000171, 0x2000172, 0x2000173, 0x2000174 };
unsigned int TrafficEmergency[4] = { 0x2000104, 0x2000024, 0x20000fc, 0x200016f };
unsigned int TrafficDepot[1] = { 0x20001e4 };
unsigned int TrafficMatador[2] = { 0x20001df, 0x20001e0 };
unsigned int TrafficGang[4] = { 0x20001da, 0x20001db, 0x20001dc, 0x20001dd };
struct FVTable_ {
    unsigned int *m_pAnims;
    unsigned int m_Count;
};
FVTable_ TrafficJam[7] = {
    { TrafficEverything, 10 }, { TrafficResidential, 6 }, { TrafficCommercial, 4 }, { TrafficEmergency, 4 },
    { TrafficDepot, 1 },       { TrafficMatador, 2 },     { TrafficGang, 4 },
};

int EmergencySpawnTimer;
signed char AirTraffic;
signed char AirLights;
int AirFlagChannel;
int AirSpawnDist;
int AirMax;
float AirSpawnTimer;
int PathLastSpawned;
signed char AirInitFromTrigger;

XItem *AirTraffic_CreateGeneric(unsigned int anim)
{
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(0x10000cc, 0, 0);
    XItem *p = XItem::CreateItem(pFile, 0x10000cc, EXRUNTIME_CLASS(XItemHandler_AirVehicle),
                                 EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    p->AddAnimator(EXItemAnimator::CreateFrom(pFile, anim));
    p->SetRotation(EXQuaternion(0.0f, 0.0f, 0.0f, 0.0f));
    theItemEnv.AddItem(p, 2);
    ((XItemHandler_AirVehicle *)p->ItemHandler())->HandleVisionModeTextureSwaps(XItemHandler_Player::Instance()->m_Tint);
    return p;
}

void AirTraffic_Trigger(EXTrigger *pTrig)
{
    AirMax = pTrig->m_Data[0];
    AirSpawnDist = pTrig->m_Data[1];
    AirFlagChannel = pTrig->m_Data[2];
    AirLights = (signed char)pTrig->m_Data[3];
    if (!AirMax) AirMax = 8;
    AirInitFromTrigger = 1;
    if (!AirSpawnDist) AirSpawnDist = 90;
}

// NEAR: retail keeps the separate new-null test (mov esi,eax; jmp; xor esi,esi; test esi); ours threads it
EXItemAnimator_Script *AddScriptToDatum(EXItem *pItem, unsigned int file, unsigned int script, unsigned int datum)
{
    EXDatumTemp d;
    EXItemAnimator_Script *pAnim = 0;
    if (pItem->GetAnimatorDatum(datum, d, 4, 0)) {
        pAnim = new EXItemAnimator_Script;
        if (pAnim) {
            pAnim->Initialise(EXGeoFile::GetGeoFile(file), script);
            pAnim->RenderFlags().BitSet(1);
            pItem->AddAnimator(pAnim);
            pAnim->SetPosition(d.Position());
        }
    }
    return pAnim;
}

// Puts a police helicopter out at distance AirSpawnDist from pos, heading in to hover over it.
void AirTraffic_CreateEmergencyVehicle(EXVector &pos)
{
    XItem *p = AirTraffic_CreateGeneric(0x20000fc);
    XItemHandler_AirVehicle *h = (XItemHandler_AirVehicle *)p->ItemHandler();
    h->m_Health = 350.0f;
    h->m_Unk112 = 3;
    h->m_Speed = 0.25f;
    h->m_MaxSpeed = 0.25f;
    h->m_Unk129 = 1;
    h->m_Unk110 = 2;
    h->m_State = 2;
    h->m_Target = pos;
    float ang = EXRandf() * 6.2831855f;
    int dist = AirSpawnDist;
    h->m_Pos.y = pos.y + 30.0f;
    h->m_Pos.w = pos.w;
    h->m_Pos.x = (float)sin(ang) * dist + pos.x;
    h->m_Pos.z = (float)cos(ang) * dist + pos.z;
    p->SetPosition(EXVector(h->m_Pos.x, h->m_Pos.y, h->m_Pos.z, h->m_Pos.w));
    p->SetMapOn(theItemEnv.GetMapOn(p->Position()));
    p->SetRotation(EXQuaternion(0.0f, ang + 3.1415927f, 0.0f, 0.0f));
}

// Finds where path's route crosses into the AirSpawnDist sphere around the player: the first segment
// that starts outside and ends inside. Returns the segment's first node (and the crossing in out), or -1.
int AirTraffic_GetSpawnPos(EXGeoPath *pPath, EXVector &out)
{
    EXVector player = gpPlayerItem[0]->Position();
    int n = pPath->NumNodes();
    float r2 = (float)(AirSpawnDist * AirSpawnDist);
    for (int i = 0; i < n; i++) {
        int j = i + 1;
        if (j >= n) j = 0;
        EXVector3 a = pPath->Node(i)->Position();
        float dx = a.x - player.x;
        float dy = a.y - player.y;
        float dz = a.z - player.z;
        if (r2 < dy * dy + dz * dz + dx * dx) {
            EXVector3 b = pPath->Node(j)->Position();
            dx = b.x - player.x;
            dy = b.y - player.y;
            dz = b.z - player.z;
            if (dy * dy + dz * dz + dx * dx < r2) {
                EXVector3 na = pPath->Node(i)->Position();
                EXVector3 nb = pPath->Node(j)->Position();
                EXLineSeg seg(EXVector(na.x, na.y, na.z, 0.0f), EXVector(nb.x - na.x, nb.y - na.y, nb.z - na.z, 0.0f));
                EXSphere sphere;
                sphere.Set(player, (float)AirSpawnDist);
                EXVector hit[2];
                hit[0].w = player.w;
                if (!EXFindIntersection(seg, sphere, hit)) return -1;
                out = EXVector(hit[0].x, hit[0].y, hit[0].z, hit[0].w);
                return i;
            }
        }
    }
    return -1;
}

// Resets air traffic for a level: notes whether it has traffic paths, which vehicle classes they use
// (loading those geo sections of the air force file) and whether an objective wants a flying vehicle.
void AirTraffic_Initialise()
{
    signed char bClass[7] = { 0 };
    signed char bFlying = 0;
    AirTraffic = 0;
    gAirVehicle_Count = 0;
    for (int i = 0; i < theItemEnv.GetPaths(0xffff)->Size(); i++) {
        EXGeoPath &path = (*theItemEnv.GetPaths(0xffff))[i];
        if ((path.Flags() & 0x1000) && path.Type() == 0) {
            AirTraffic = 1;
            int c = path.Value(1);
            if (c == 0) {
                bClass[1] = 1;
                bClass[2] = 1;
            } else if (c < 7) {
                bClass[c] = 1;
            }
        }
    }
    for (XItem *p = gTrigList.GetFirst(); p; p = gTrigList.GetNext(p)) {
        EXItemHandler *h = p->ItemHandler();
        if (h->IsKindOf(EXRUNTIME_CLASS(XItemHandler_Objective))) {
            XItemHandler_Objective *o = (XItemHandler_Objective *)h;
            if (o->m_Profile) {
                ActorProfile *pProf = AP_GetPtr(o->m_Profile);
                if (pProf && pProf->m_SubGroup == 0x4f000026) {
                    bFlying = 1;
                    bClass[3] = 1;
                }
            }
        }
    }
    AirSpawnTimer = 0.0f;
    PathLastSpawned = 0;
    EmergencySpawnTimer = (g_EXRandClass.Rand32() % 240 + 60) * 30;
    if (!AirInitFromTrigger) {
        AirMax = 8;
        AirSpawnDist = 90;
        AirFlagChannel = 0;
        AirLights = 0;
    }
    AirInitFromTrigger = 0;
    if (bFlying || AirTraffic) {
        EXGeoFile *pFile = EXGeoFile::LoadGeoFile(0x10000cc, 0, 0);
        EXGeoHeader *pHdr = pFile->GeoHeader();
        for (unsigned int k = 1; k <= 6; k++)
            if (bClass[k]) pHdr->LoadSectionFile(0x8000000 + k, 1, 0);
    }
}

// Puts one ambient vehicle onto traffic path idx where its route enters the spawn sphere.
void AirTraffic_CreateTraffic(int idx)
{
    EXGeoPath *pPath = &(*theItemEnv.GetPaths(0xffff))[idx];
    EXVector spawn;
    int node = AirTraffic_GetSpawnPos(pPath, spawn);
    if (node < 0) return;
    int cls = pPath->Value(1);
    if (cls < 0 || cls > 6) cls = 0;
    unsigned int count = TrafficJam[cls].m_Count;
    unsigned int pick = 0;
    if (count) pick = g_EXRandClass.Rand32() % count;
    XItem *p = AirTraffic_CreateGeneric(TrafficJam[cls].m_pAnims[pick]);
    XItemHandler_AirVehicle *h = (XItemHandler_AirVehicle *)p->ItemHandler();
    if (cls == 3) {
        EmergencySpawnTimer = (g_EXRandClass.Rand32() % 180 + 120) * 30;
        h->CreateBluesAndTwos();
    }
    h->m_Path = idx;
    float speed = 0.5f;
    if (pPath->Value(0)) speed = (float)pPath->Value(0) * 0.016666668f;
    h->m_Speed = speed;
    h->m_Unk110 = 0;
    h->m_State = 0;
    h->m_Node = node;
    p->SetPosition(spawn);
    p->SetMapOn(theItemEnv.GetMapOn(spawn));
    h->m_Pos = p->Position();
    int next = node + 1;
    if (next >= pPath->NumNodes()) next = 0;
    EXVector3 a = pPath->Node(node)->Position();
    float dx = spawn.x - a.x;
    float dy = spawn.y - a.y;
    float dz = spawn.z - a.z;
    float d2 = dy * dy + dz * dz + dx * dx;
    float dist = d2 < 0.0f ? 0.0f : EXSqrt(d2);
    EXVector3 b = pPath->Node(next)->Position();
    float sx = b.x - a.x;
    float sy = b.y - a.y;
    float sz = b.z - a.z;
    float s2 = sz * sz + sy * sy + sx * sx;
    float seg = 0.0f;
    if (s2 >= 0.0f) seg = EXSqrt(s2);
    h->m_PathT = dist / seg;
    h->m_SoundTag.AddTrackingSound(0x1af002cb, p->Position(), 0.0f, 0.0f);
}

// Spawns the scripted gunship for a profile-driven patrol: starts at the node of the nearest patrol
// path (type 1) to the location, hovering with the profile's health and a 0.5 top speed.
void AirTraffic_CreateFromProfile(unsigned int locHash, unsigned int profileHash, int param)
{
    int loc = POI_ResolveLocation(locHash, 1, 0, 0);
    if (!loc) return;
    ActorProfile *pProf = AP_GetPtr(profileHash);
    if (!(0xd0000c0 < pProf->m_Skins[0] && pProf->m_Skins[0] < 0xd0000c3)) return;
    GeoSectionManager::Add(0x100003c, 0x20000da);
    XItem *p = AirTraffic_CreateGeneric(0x20000fc);
    XItemHandler_AirVehicle *h = (XItemHandler_AirVehicle *)p->ItemHandler();
    h->m_Speed = 0.0f;
    h->m_Health = pProf->m_Health;
    h->m_Unk110 = 1;
    h->m_Unk112 = 1;
    h->m_State = 1;
    h->m_MaxSpeed = 0.5f;
    h->m_Unk129 = 1;
    *(int *)&h->m_Unk12a[2] = param;
    h->m_Profile = profileHash;
    EXVector loc3 = POI_Get(loc)->m_Pos;
    float best = 1e9f;
    int bestNode = 0;
    int bestPath = 0;
    EXGeoPathArray &paths = *theItemEnv.GetPaths(0xffff);
    for (int i = 0; i < paths.Size(); i++) {
        EXGeoPath &path = paths[i];
        if ((path.Flags() & 0x1000) && path.Type() == 1) {
            int n = path.NumNodes();
            for (int k = 0; k < n; k++) {
                EXVector3 a = path.Node(k)->Position();
                float dx = a.x - loc3.x;
                float dy = a.y - loc3.y;
                float dz = a.z - loc3.z;
                float d2 = dy * dy + dz * dz + dx * dx;
                if (d2 < best) {
                    best = d2;
                    bestNode = k;
                    bestPath = i;
                }
            }
        }
    }
    h->m_Path = bestPath;
    h->m_Node = bestNode;
    EXVector3 at = paths[bestPath].Node(bestNode)->Position();
    p->SetPosition(EXVector(at.x, at.y, at.z, 0.0f));
    p->SetMapOn(theItemEnv.GetMapOn(p->Position()));
    h->m_Pos = p->Position();
    h->InitTracers();
    h->m_SoundTag.AddTrackingSound(0x1af002de, p->Position(), 0.0f, 0.0f);
}

// Per-frame spawning: draws the path debug lines, counts live vehicles per path, and every so often
// puts a new one on the least-populated traffic path the player is near (emergency vehicles wait on
// their own timer).
void AirTraffic_Update()
{
    if (EXBaseRenderEnv::RenderPaths()) {
        for (int i = 0; i < theItemEnv.GetPaths(0xffff)->Size(); i++) {
            EXGeoPath &path = (*theItemEnv.GetPaths(0xffff))[i];
            if (path.Flags() & 0x1000) {
                unsigned int colour = 0x640000ff;
                if (path.Type() == 0) {
                    EXVector tmp;
                    AirTraffic_GetSpawnPos(&path, tmp);
                    colour = 0x64ff0000;
                }
                path.Debug_DrawPath(ex::RGBA(colour));
            }
        }
    }
    if (!AirTraffic || (AirFlagChannel != 0 && FlagChannels[AirFlagChannel])) return;
    if (0 < EmergencySpawnTimer) EmergencySpawnTimer--;
    if (0.0f < AirSpawnTimer) {
        AirSpawnTimer -= EXBaseApp::FrameLength();
        return;
    }
    if (gAirVehicle_Count >= AirMax) return;
    EXArray<int, int &> counts;
    counts.SetSize(theItemEnv.GetPaths(0xffff)->Size(), -1);
    for (int i = 0; i < theItemEnv.GetPaths(0xffff)->Size(); i++) counts[i] = 0;
    for (XItem *p = gAirVehicleList.GetFirst(); p; p = gAirVehicleList.GetNext(p)) {
        XItemHandler_AirVehicle *h = (XItemHandler_AirVehicle *)p->ItemHandler();
        if (h->m_Unk110 == 0) counts[h->m_Path]++;
    }
    int chosen = -1;
    int fewest = 999;
    int path = PathLastSpawned;
    for (int n = 0; n < theItemEnv.GetPaths(0xffff)->Size(); n++) {
        path++;
        if (path >= theItemEnv.GetPaths(0xffff)->Size()) path = 0;
        EXGeoPath &cand = (*theItemEnv.GetPaths(0xffff))[path];
        EXVector tmp;
        if ((cand.Flags() & 0x1000) && cand.Type() == 0 && (cand.Value(1) != 3 || EmergencySpawnTimer == 0) &&
            cand.BoundsBox().Intersect(gpPlayerItem[0]->Position(), (float)AirSpawnDist) && counts[path] < fewest &&
            AirTraffic_GetSpawnPos(&cand, tmp) >= 0) {
            fewest = counts[path];
            chosen = path;
        }
    }
    if (chosen >= 0) {
        PathLastSpawned = chosen;
        AirTraffic_CreateTraffic(chosen);
        AirSpawnTimer = (g_EXRandClass.Randf() + 1.0f) * 0.5f;
    }
}
