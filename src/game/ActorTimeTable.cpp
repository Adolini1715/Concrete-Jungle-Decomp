#include "ActorTimeTable.h"
#include "Actor.h"
#include "ActorUtil.h"
#include "EXRand.h"
#include "FEText.h"
#include "AIControl.h"
#include "XItemHandler_Player.h"
#include "XItem.h"
#include "XNavMeshHandler.h"
#include "XItemHandler_Camera.h"
#include "EXBaseApp.h"
#include <math.h>

// TODO stand-ins (ActorUtil.obj, other areas).
extern unsigned int gCurrentMap_GeoHash;

static s8 s_bLevelStart = 1;
static int s_AvailableActorCount;
static TimeTableHeader s_Table;     // the current sheet's rows
static unsigned int s_Sheet;
int SpawnIndex;
static float s_Time;
static int s_TaggedA;
static int s_TaggedB;
static int s_ToSpawn;         // pedestrians still to spawn in this wave
static int s_SpawnCount;      // spawned this frame
static int s_Unk4;
tagSpawnPos SpawnPos[8];

void TimeTable_RefreshSpreadsheet()
{
    if (s_Sheet > 0x14000000 && s_Sheet < 0x1400001a)
        s_Table.m_pEntries = (TimeTableEntry *)AU_GetWorkSheet(gCurrentMap_GeoHash, s_Sheet, 0, s_Table.m_Count);
    else
        s_Table.m_pEntries = 0;
}

void TimeTable_Set(unsigned int sheet)
{
    if (sheet > 0x14000000 && sheet < 0x1400001a) {
        s_Sheet = sheet;
        s_Table.m_pEntries = (TimeTableEntry *)AU_GetWorkSheet(gCurrentMap_GeoHash, sheet, 0, s_Table.m_Count);
    } else {
        s_Sheet = 0;
        s_Table.m_pEntries = 0;
    }
}

int TimeTable_Get() { return s_Sheet; }

int TimeTable_GetDensity(TimeTableHeader *pTable)
{
    if (pTable) {
        TimeTableEntry *e = pTable->m_pEntries;
        if (e) {
            float t = s_Time;
            int best = 0;
            for (int i = 0; i < pTable->m_Count; i++) {
                if (e[i].m_Time > t) break;
                if (i > 0 && pTable->m_pEntries[i].m_Time > pTable->m_pEntries[i - 1].m_Time) best = i;
            }
            return e[best].m_Density;
        }
    }
    return 0;
}

int TimeTable_GetAvailableActorCount() { return s_AvailableActorCount; }

// NEAR: register allocation of the second loop (retail keeps the entries pointer in ebp).
int TimeTable_GetRandomProfile(TimeTableHeader *pTable)
{
    if (!pTable) return 0;
    int sum = 0;
    int total = 0;
    int i;
    for (i = 0; i < pTable->m_Count; i++)
        if (pTable->m_pEntries[i].m_Time == 0.0f) total += pTable->m_pEntries[i].m_Density;
    int r = EXRand(total + 1);
    for (i = 0; i < pTable->m_Count; i++) {
        TimeTableEntry *e = pTable->m_pEntries;
        if (e[i].m_Time == 0.0f) {
            if (e[i].m_Density + sum >= r) return e[i].m_Profile;
            sum += e[i].m_Density;
        }
    }
    return 0;
}

float TimeTable_GetTime() { return s_Time; }

void TimeTable_SkipHour() { s_Time += 0.5f; }

void TimeTable_PostLoadInit()
{
    s_Time = 8.0f;
    s_ToSpawn = 0;
    s_SpawnCount = 0;
    s_Unk4 = 0;
    s_TaggedA = 0;
    s_TaggedB = 0;
    s_AvailableActorCount = 0;
    s_Sheet = 0;
    s_Table.m_Count = 0;
    s_Table.m_pEntries = 0;
    s_Table.m_Unk0 = 0;
    s_Table.m_Unk4 = 8.0f;
}

void TimeTable_LevelPreRun()
{
    s_bLevelStart = 1;
    SpawnIndex = 0;
    for (int i = 0; i < 8; i++) {
        tagSpawnPos *p = &SpawnPos[i];
        p->m_98 = 1.0f;
        p->m_94 = 25.0f;
        p->m_90 = 39.0f;
        p->m_88 = (g_EXRandClass.Randf() + 1.0f) * 2.0f;
        p->m_Loc.Reset();
        p->m_Loc.Ignore(0x203);
    }
}

void TimeTable_TaggedPersonHere(int a, int b)
{
    s_TaggedA = a;
    s_TaggedB = b;
}

Actor *TimeTable_CreateActor(unsigned int profile, EXVector pos, int poly, float rotY, unsigned int flags)
{
    pos.y = 0.0f;
    Actor *p = Actor::Create(profile, pos, poly, rotY, 0, flags, 0);
    if (p) {
        s_SpawnCount++;
        if (gCurrentMap_GeoHash == 0x100011a) {
            p->SetHolographic();
            p->SetIsTargetable(0);
        }
    }
    return p;
}

Actor *TimeTable_CreateActor(unsigned int profile, EXVector pos, EXVector from, float rotY, unsigned int flags)
{
    return TimeTable_CreateActor(profile, AU_EnsurePointOutOfView(pos, from), 0, rotY, flags);
}

signed char TimeTable_GenerateSpawnPos(EXVector pos, XNavLocation &loc)
{
    if (++SpawnIndex >= 8) {
        TimeTable_UpdateSpawnPos();
        SpawnIndex = 0;
    }
    if (SpawnPos[SpawnIndex].m_bValid) {
        loc = SpawnPos[SpawnIndex].m_Loc;
        return 1;
    }
    return 0;
}

// Re-places the eight pedestrian spawn points on a 30-unit ring around the player (one every
// 45 degrees), each moved along the nav mesh from the player's nav location (snapped instead when
// it ended more than 2 units off). A point is usable when it ends 20..40 units from the player
// and not on a poly flagged 0x203 (blocked, no-spawn).
void TimeTable_UpdateSpawnPos()
{
    for (int i = 0; i < 8; i++) {
        tagSpawnPos &sp = SpawnPos[i];
        float angle = (float)i * 0.125f * 6.2831855f;
        EXMaths::mat_44 rot;
        rot.set_identity();
        float s = sinf(angle), c = cosf(angle);
        rot.r[0].x = c;
        rot.r[0].z = -s;
        rot.r[2].x = s;
        rot.r[2].z = c;
        EXVector offset(0.0f, 0.0f, 30.0f, 0.0f);
        offset = rot * offset;
        const EXVector &player = XItemHandler_Player::Instance()->GetItem()->Position();
        EXVector target = player + offset;
        sp.m_Loc = gAIControl.m_PlayerNavLoc;
        sp.m_Loc.MoveTo(target, 0);
        EXVector d = target - sp.m_Loc.Pos();
        if (d.y * d.y + d.z * d.z + d.x * d.x > 4.0f) sp.m_Loc.SetPosPoly(target, -1, 0);
        EXVector fromPlayer = sp.m_Loc.Pos() - XItemHandler_Player::Instance()->GetItem()->Position();
        float distSq = fromPlayer.y * fromPlayer.y + fromPlayer.z * fromPlayer.z + fromPlayer.x * fromPlayer.x;
        sp.m_bValid = !(distSq < 0.0f || sqrtf(distSq) <= 20.0f || sqrtf(distSq) >= 40.0f);
        int poly = sp.m_Loc.GetPolyHandle();
        XNavMesh *pMesh = gNavMeshHandler.GetPtr(poly);
        if (pMesh) {
            int id = poly & 0xffff;
            u16 flags = (id == 0xffff || id >= (int)pMesh->PolyCount()) ? 0 : pMesh->Poly(id)->Flags();
            if (flags & 0x203) sp.m_bValid = 0;
        }
    }
}

// Per-frame pedestrian control. With a time table set: counts the actors within 40 units (in x/z)
// of the camera and the time-table spawned ones (creation flag 0x20); when both are under the
// sheet's density for this hour and no wave is pending, a wave of a quarter of the shortfall is
// queued (and three quarters becomes the available count). Queued pedestrians are spawned at the
// prepared spawn points, at most two per frame except on the first frame of a level. Without a
// sheet 30 actors are available. Then actors flagged 0x2 that strayed beyond their alive radius
// of the player are deleted, and the clock advances (24-hour wrap; one game day takes 86000 /
// nominal rate seconds). The sheet is cleared each frame (scripts set it again).
void TimeTable_Update()
{
    EXVector cam = XGetCamera(0)->GetItem()->Position();
    int nearCount = 0;
    int spawned = 0;
    if (!s_Table.m_pEntries) {
        s_AvailableActorCount = 30;
    } else {
        for (Actor *p = Actor::GetFirst(0); p; p = p->GetNext(0)) {
            EXVector d = cam - p->GetItem()->Position();
            if (d.z * d.z + d.x * d.x < 1600.0f) nearCount++;
            if (*(u8 *)&p->m_CreationFlags & 0x20) spawned++;
        }
        TimeTable_UpdateSpawnPos();
        XNavLocation loc;
        int density = TimeTable_GetDensity(&s_Table);
        if (spawned < density && nearCount < density && s_ToSpawn < 1) {
            s_AvailableActorCount = (int)((float)(density - spawned) * 0.75f);
            s_ToSpawn = (int)((float)(density - spawned) * 0.25f);
        } else {
            s_AvailableActorCount = 0;
        }
        if (s_ToSpawn > 0 && (s_SpawnCount < 2 || s_bLevelStart)) {
            if (++SpawnIndex >= 8) {
                TimeTable_UpdateSpawnPos();
                SpawnIndex = 0;
            }
            if (SpawnPos[SpawnIndex].m_bValid) {
                loc = SpawnPos[SpawnIndex].m_Loc;
                for (;;) {
                    unsigned int profile = TimeTable_GetRandomProfile(&s_Table);
                    if (TimeTable_CreateActor(profile, loc.Pos(), loc.GetPolyHandle(), g_EXRandClass.Randf() * 6.2831855f,
                                              0x422)) {
                        s_SpawnCount++;
                        s_ToSpawn--;
                    }
                    if (s_ToSpawn < 1 || (s_SpawnCount > 1 && !s_bLevelStart)) break;
                    if (++SpawnIndex >= 8) {
                        TimeTable_UpdateSpawnPos();
                        SpawnIndex = 0;
                    }
                    if (!SpawnPos[SpawnIndex].m_bValid) break;
                    loc = SpawnPos[SpawnIndex].m_Loc;
                }
            }
        }
    }
    for (Actor *p = Actor::GetFirst(0); p; p = p->GetNext(0)) {
        if (!(*(u8 *)&p->m_CreationFlags & 2)) continue;
        EXVector d = p->GetItem()->Position() - XItemHandler_Player::Instance()->GetItem()->Position();
        if (p->GetAliveRadius() * p->GetAliveRadius() < d.z * d.z + d.x * d.x) p->Delete();
    }
    s_SpawnCount = 0;
    s_bLevelStart = 0;
    s_Sheet = 0;
    s_Table.m_pEntries = 0;
    s_Time += (float)EXBaseApp::NominalGameFrameRate() * (1.0f / 86000.0f) * EXBaseApp::GameFrameRateMul();
    if (s_Time >= 24.0f) s_Time -= 24.0f;
}
