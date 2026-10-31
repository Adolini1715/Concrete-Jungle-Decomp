#include "AIControl.h"
#include "AIFormation.h"
#include "Actor.h"
#include "ActorUtil.h"
#include "ActorLocation.h"
#include "ActorProfile.h"
#include "ActorTimeTable.h"
#include "ActorTextureSwap.h"
#include "ActorOptions.h"
#include "ActorEffect.h"
#include "ActorSpecial.h"
#include "ActorCarryable.h"
#include "SubGroupAnims.h"
#include "WeaponTable.h"
#include "Avoid.h"
#include "XItem.h"
#include "XItemHandler_Player.h"
#include "XNavMesh.h"
#include "XNavNetwork.h"
#include "EXItemRender_Anim.h"
#include "EXBaseSoundManager.h"
#include "EXRand.h"
#include "EXRelPtr.h"
#include "XApp.h"
#include "EXBaseApp.h"
#include "EXMaths.h"
#include "EXMemory.h"
#include "FEText.h"
#include "EVD.h"

extern unsigned int gCurrentMap_GeoHash;


// TODO stand-in: view of a map path (EXGeoPath, engine) as FindSightLinkerPaths reads it.
struct AISightPathPoint {
    float x, y, z;
    u8 m_0c[0x28 - 0x0c];
};
struct AISightPath {
    u8 m_00[0x10];
    u32 m_Flags;                            // 0x10 0x800: sight linker
    u8 m_14[0x48 - 0x14];
    int m_nPoints;                          // 0x48
    ex::EXRelPtr<AISightPathPoint> m_Points;// 0x4c
};

AIControl::AIControl()
{
    m_PlayerTaggedTimer = 0.0f;
    m_bLevelLoading = 0;
    m_CombatVOTimer = EXRandf(1.0f, 2.0f) * (float)EXBaseApp::NominalGameFrameRate() * 5.0f;
    m_CombatMusicHash = 0;
    m_PrevMusicHash = 0;
    m_NavCacheSync = 0;
}

AIControl::~AIControl() {}

signed char AIControl::BootInit()
{
    gNavMeshHandler.BootInit();
    Actor::BootInit();
    return 1;
}

signed char AIControl::PreLoadInit()
{
    DoingHTMLReport(0);
    gNavMeshHandler.PreLoadInit();
    POI_PreLoadInit();
    Actor::PreLoadInit();
    ActorCarryable::PreLoadInit();
    ActorTextureSwap::PreLoadInit();
    ActorOptionsManager::Init();
    m_ZoneSight[0][0] = -1;
    m_ZoneSight[0][1] = -1;
    m_ZoneSight[1][0] = -1;
    m_ZoneSight[1][1] = -1;
    m_ZoneSight[2][0] = -1;
    m_ZoneSight[2][1] = -1;
    m_ZoneSight[3][0] = -1;
    m_ZoneSight[3][1] = -1;
    m_ZoneSight[4][0] = -1;
    m_ZoneSight[4][1] = -1;
    m_ZoneSight[5][0] = -1;
    m_ZoneSight[5][1] = -1;
    m_ZoneSight[6][0] = -1;
    m_ZoneSight[6][1] = -1;
    m_ZoneSight[7][0] = -1;
    m_ZoneSight[7][1] = -1;
    m_ZoneSight[8][0] = -1;
    m_ZoneSight[8][1] = -1;
    m_ZoneSight[9][0] = -1;
    m_ZoneSight[9][1] = -1;
    m_ZoneSight[10][0] = -1;
    m_ZoneSight[10][1] = -1;
    m_ZoneSight[11][0] = -1;
    m_ZoneSight[11][1] = -1;
    m_ZoneSightCount = 0;
    m_bLevelLoading = 1;
    g_LodDetailsOffscreen[0].m_skinBits.Set(0, 0);
    g_LodDetailsOffscreen[0].m_flags |= 0x80;
    g_LodDetailsOnScreenUlysses[3].m_skinBits.Set(0, 0);
    g_LodDetailsOnScreenUlysses[3].m_flags |= 0x80;
    g_LodDetailsOnScreenBoss[3].m_skinBits.Set(0, 0);
    g_LodDetailsOnScreenBoss[3].m_flags |= 0x80;
    g_LodDetailsOnScreenAlien[3].m_skinBits.Set(0, 0);
    g_LodDetailsOnScreenAlien[3].m_flags |= 0x80;
    g_LodDetailsOnScreenDrone[3].m_skinBits.Set(0, 0);
    g_LodDetailsOnScreenDrone[3].m_flags |= 0x80;
    return 1;
}

signed char AIControl::PostLoadInit()
{
    gNavMeshHandler.PostLoadInit();
    NavCacheSync()++;
    g_Formations.Initialise();
    ActorSpecial_PostLoadInit();
    EVD::Instance(1)->Initialise();
    Actor::PostLoadInit();
    ActorEffect::PostLoadInit();
    AS_PostLoadInit();
    AP_PostLoadInit();
    SubGroupAnims::Init();
    ActorTextureSwap::PostLoadInit();
    POI_PostLoadInit();
    TimeTable_PostLoadInit();
    m_ActorsInCombatCount = 0;
    m_bLevelLoading = 0;
    m_GrenadeTime = 0.0f;
    m_GrenadeNextTime = 0.0f;
    m_Unk26c = 0.0f;
    m_DistOffNaviMeshLastSeen = 0.0f;
    m_LastKnownLocationTimer = 0.0f;
    g_iCombatInnerCircle = 5;
    return 1;
}

signed char AIControl::UnloadLevel()
{
    m_PlayerNavLoc.Reset();
    gNavMeshHandler.UnloadLevel();
    return 1;
}

signed char AIControl::Shutdown()
{
    Actor::PreLoadInit();
    ActorCarryable::PreLoadInit();
    ActorTextureSwap::PreLoadInit();
    ActorOptionsManager::Init();
    return 1;
}

signed char AIControl::OnSectionLoad(unsigned int hash)
{
    if (gCurrentMap_GeoHash != 0x100006e) return 0;
    return hash == 0x8000002;
}

signed char AIControl::OnSectionDeload(unsigned int hash)
{
    if (gCurrentMap_GeoHash != 0x100006e) return 0;
    return hash == 0x8000002;
}

void AIControl::UpdateCombatMusic()
{
    if (m_ActorsInCombatCount && m_CombatMusicHash && (m_bCombatMusicPlaying || !(theItemEnv.MusicFlags() & 4))) {
        m_CombatMusicTimer = 300.0f;
        m_CombatMusicStopTimer -= XApp::GameFrameRateMul();
        if (m_CombatMusicStopTimer <= 0.0f && !m_bCombatMusicPlaying) {
            m_PrevMusicHash = theItemEnv.MusicHash();
            EXBaseSoundManager::GetSoundManager()->StartMusic(m_CombatMusicHash, 10, 10, 0, 0);
            m_ActorsInCombatCount = 0;
            m_bCombatMusicPlaying = 1;
            return;
        }
    } else if (m_bCombatMusicPlaying) {
        m_CombatMusicStopTimer = 60.0f;
        m_CombatMusicTimer -= XApp::GameFrameRateMul();
        if (m_CombatMusicTimer <= 0.0f) {
            EXBaseSoundManager *pSM = EXBaseSoundManager::GetSoundManager();
            if (pSM->MusicHash() == m_CombatMusicHash) {
                if (m_PrevMusicHash != -1) {
                    pSM->StartMusic(m_PrevMusicHash, 30, 30, 1, 0);
                    m_bCombatMusicPlaying = 0;
                    m_ActorsInCombatCount = 0;
                    return;
                }
                pSM->StartMusic(0x1b00001e, 30, 30, 1, 0);
                m_bCombatMusicPlaying = 0;
            }
        }
    }
    m_ActorsInCombatCount = 0;
}

signed char AIControl::HandleMemCompaction()
{
    if (!m_bLevelLoading) {
        WeaponProfile::RefreshBasePointer();
        ArmourProfile::RefreshBasePointer();
        SubGroupAnims::RefreshBasePointer();
        AS_RefreshPointer();
        AP_RefreshPointer();
        TimeTable_RefreshSpreadsheet();
        gNavMeshHandler.HandleMemCompaction();
    }
    return 1;
}

void AIControl::NavMeshAdded(XGeoNavMeshEntity *pMesh)
{
    POI_OnLoadNavMesh(pMesh);
}

void AIControl::NavMeshDeleting(XGeoNavMeshEntity *pMesh)
{
    POI_OnDeLoadNavMesh(pMesh);
}

signed char AIControl::CheckZoneSight(int a, int b) const
{
    if (a == b) return 1;
    int n = m_ZoneSightCount;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < 2; j++) {
            if (m_ZoneSight[i][j] == -1) break;
            if (m_ZoneSight[i][j] == a) {
                for (int k = j; k < 2; k++)
                    if (m_ZoneSight[i][k] == b) return 1;
            }
        }
    }
    return 0;
}

void AIControl::SetSpokenCombatVO()
{
    switch (gCurrentMap_GeoHash) {
    case 0x10000b4:
        m_CombatVOTimer = EXRandf(8.0f, 14.0f) * (float)EXBaseApp::NominalGameFrameRate();
        break;
    case 0x1000124:
        m_CombatVOTimer = EXRandf(8.0f, 14.0f) * (float)EXBaseApp::NominalGameFrameRate();
        break;
    case 0x100013b:
        m_CombatVOTimer = EXRandf(8.0f, 14.0f) * (float)EXBaseApp::NominalGameFrameRate();
        break;
    default:
        m_CombatVOTimer = EXRandf(1.0f, 2.0f) * (float)EXBaseApp::NominalGameFrameRate() * 3.0f;
        break;
    }
}

// NEAR: block layout matches; the player-offset rotation (inlined Euler matrix) uses fewer stack
// temporaries than retail (0xb8 vs 0xd8 frame), so slots and float scheduling differ.
signed char AIControl::Update()
{
    gNavMeshHandler.Update();
    m_GrenadeTime += EXBaseApp::FrameLength();
    ActorCarryableGun::ResetBulletsPerFrame();
    if (m_PlayerTaggedTimer > 0.0f) {
        m_PlayerTaggedTimer -= XApp::GameFrameRateMul();
        SetLastSeenPlayerPos(Player_GetFeetPos());
    }
    if (m_CombatVOTimer > 0.0f) m_CombatVOTimer -= XApp::GameFrameRateMul();
    m_LastKnownLocationTimer += XApp::GameFrameRateMul();
    UpdateCombatMusic();
    CAvoidance::UpdatePerFrame();
    XItemHandler_Player *pPlayer = XItemHandler_Player::Instance();
    if (pPlayer) {
        switch (pPlayer->m_Mode) {
        case 4:
        case 0xc: {
            EXVector pelvis = Player_GetPelvisPos();
            EXMaths::vec_xyzw ofs;
            ofs.x = 0.0f;
            ofs.y = 0.0f;
            ofs.z = 0.75f;
            ofs.w = 0.0f;
            EXVector rot = Player_GetRot();
            float sx = (float)sin(rot.x);
            float cx = (float)cos(rot.x);
            float sy = (float)sin(rot.y);
            float cy = (float)cos(rot.y);
            float sz = (float)sin(rot.z);
            float cz = (float)cos(rot.z);
            EXMaths::mat_44 m;
            m.r[0].x = cy * cz;
            m.r[0].y = sy * cz * sx + sz * cx;
            m.r[0].z = sz * sx - sy * cz * cx;
            m.r[0].w = 0.0f;
            m.r[1].x = -(cy * sz);
            m.r[1].y = cz * cx - sy * sz * sx;
            m.r[1].z = sy * sz * cx + cz * sx;
            m.r[1].w = 0.0f;
            m.r[2].x = sy;
            m.r[2].y = -(cy * sx);
            m.r[2].z = cy * cx;
            m.r[2].w = 0.0f;
            m.r[3].x = 0.0f;
            m.r[3].y = 0.0f;
            m.r[3].z = 0.0f;
            m.r[3].w = 1.0f;
            EXMaths::vec_xyzw d = m * ofs;
            EXVector pos = pelvis + EXVector(d);
            m_PlayerNavLoc.SetPosSlow(pos, 1);
            break;
        }
        case 0x18:
            m_PlayerNavLoc.SetPosSlow(Player_GetPelvisPos(), 0);
            break;
        default:
            m_PlayerNavLoc.SetPosSlow(Player_GetFeetPos(), 1);
            break;
        }
    } else {
        m_PlayerNavLoc.SetPosSlow(Player_GetFeetPos(), 0);
    }
    m_PlayerNavLoc.SetRot(Player_GetRot());
    m_PlayerNavLoc.Ignore(0x48);
    m_Unk26c = (m_PlayerNavLoc.FloorPos() - Player_GetFeetPos()).Length();
    m_DistOffNaviMeshLastSeen = (m_PlayerNavLocLastSeen.FloorPos() - m_LastSeenPlayerPos).Length();
    g_Formations.Update();
    POI_Update();
    TimeTable_Update();
    Actor::Update();
    WeaponProfile::Update();
    ActorCarryable::Update();
    EVD::Instance(1)->Update();
    ActorOptionsManager::PostProcess();
    return 1;
}

signed char AIControl::GetDistantPos(const EXVector &pos, float dist, EXVector &out, int *pPoly)
{
    XNavMesh *pMesh = gNavMeshHandler.GetPtr(pos, 0);
    if (!pMesh) return 0;
    return pMesh->NavNetwork()->GetFleePosition(pos, pos, dist, out, pPoly);
}

signed char AIControl::PositionIsValid(const EXVector &pos)
{
    XNavMesh *pMesh = gNavMeshHandler.GetPtr(pos, 0);
    if (!pMesh) return 0;
    int poly = -1;
    return pMesh->LocatePolygon(pos, &poly, 0, 0);
}

void AIControl::FindSightLinkerPaths()
{
    int i = 0;
    AISightPath *p = (AISightPath *)theItemEnv.GetPath(0);
    while (p) {
        if (p->m_Flags & 0x800) {
            int *pZones = m_ZoneSight[m_ZoneSightCount];
            for (int j = 0; j < p->m_nPoints; j++) {
                if (j > 2) break;
                XNavLocation loc;
                AISightPathPoint *pts = p->m_Points;
                EXVector v(pts[j].x, pts[j].y, pts[j].z, 0.0f);
                loc.SetPosSlow(v, 0);
                pZones[j] = loc.NavZone();
            }
            if (++m_ZoneSightCount == 12) return;
        }
        p = (AISightPath *)theItemEnv.GetPath(++i);
    }
}

signed char AIControl::LevelPreRun()
{
    TimeTable_LevelPreRun();
    FindSightLinkerPaths();
    m_bCombatMusicPlaying = 0;
    m_CombatMusicTimer = 0.0f;
    m_CombatMusicStopTimer = 0.0f;
    return 1;
}

AIControl gAIControl;
