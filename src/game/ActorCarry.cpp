#include "ActorCarryable.h"
#include "TracerFire.h"
#include "XItemHandler_AntiCloakSmoke.h"
#include "Actor.h"
#include "XItem.h"
#include "WeaponTable.h"
#include "GeoSectionManager.h"
#include "EXItemAnimator_Script.h"
#include "EXHashCodeFile.h"
#include "EXTrace.h"
#include "EXBaseApp.h"
#include "EXRand.h"
#include "EXMemory.h"
#include "EXItemAnimator_Entity.h"
#include "EXGeoFile.h"
#include "XGameWnd.h"
#include "XItemHandler_Bullet.h"
#include "ActorProfile.h"
#include "EXQuaternion.h"
#include "FEText.h"
#include "EXDatum.h"
#include "EXStats.h"
#include "EXCapsule.h"
#include "EXIntersection.h"
#include "EXRayCastTemp.h"
#include "ActorUtil.h"
#include "EXDistance.h"
#include "EXLine.h"
#include "XItemHandler_Player.h"
#include "EXPrimVert.h"
#include "XApp.h"
#include "EXSphere.h"
#include "EXBaseSoundManager.h"
#include "XItemHandler_ScriptBase.h"
#include "XItemHandler_Camera.h"

struct CarryTypeEntry {
    int m_Entity;
    CarryType m_Type;
    u32 m_Unk8;
    unsigned int m_Text;
};

extern signed char DBG_bActorsAndVehicles;  // TODO stand-in (Actor.obj)
extern signed char gActorPause;             // TODO stand-in (Actor.obj)
extern unsigned int gCurrentMap_GeoHash;    // TODO stand-in
void CreateActorDroppedMine(Actor &actor);
extern int Actor_CalledRayCast;  // Actor.obj
float GetRotation(EXVector &v);  // Player.obj
float CalculateAngle(float speed, float dist, float height, float grav, signed char bHigh);  // Player.obj
extern XGameWnd *gpGameWnd[4];              // TODO stand-in (GameWnd.obj)
static int s_LastID = 10;
int ActorCarryableGun::ms_iInstantBulletsThisFrame = 5;
static CarryTypeEntry s_CarryTypes[] = {
    {0x20000b4, (CarryType)4, 0x0, 0x510000c3},
    {0x20000b5, (CarryType)2, 0x0, 0x510000c4},
    {0x20000b6, (CarryType)7, 0x0, 0x510000c5},
    {0x20000b7, (CarryType)8, 0x0, 0x510000c6},
    {0x20000b9, (CarryType)1, 0x0, 0x510000c7},
    {0x20000bb, (CarryType)2, 0x0, 0x510000f2},
    {0x20000bc, (CarryType)2, 0x0, 0x510000c9},
    {0x20000bd, (CarryType)2, 0x0, 0x510000ca},
    {0x20000c0, (CarryType)1, 0x0, 0x510000cb},
    {0x20000c1, (CarryType)1, 0x0, 0x510000cb},
    {0x20000c2, (CarryType)5, 0x0, 0x510000cc},
    {0x20000c9, (CarryType)0, 0x0, 0x510000cd},
    {0x2000276, (CarryType)9, 0x0, 0x510008da},
    {0x2000274, (CarryType)9, 0x0, 0x510008da},
    {0x20000c4, (CarryType)4, 0x0, 0x510000ce},
    {0x20000c6, (CarryType)4, 0x0, 0x510000cf},
    {0x20000c7, (CarryType)8, 0x0, 0x510000d0},
    {0x20000b1, (CarryType)8, 0x0, 0x510000d1},
    {0x20000b2, (CarryType)8, 0x0, 0x510000d1},
    {0x20001ce, (CarryType)0, 0x0, 0x510003b6},
    {0x20001cd, (CarryType)5, 0x0, 0x510003b7},
    {0x20001d4, (CarryType)6, 0x0, 0x510003b5},
    {0x2000283, (CarryType)6, 0x0, 0x510003b5},
    {0x20001d5, (CarryType)9, 0x0, 0x510001ee},
    {0x200016c, (CarryType)9, 0x0, 0x510001ee},
    {0x20000e5, (CarryType)9, 0x8000002, 0x510000d2},
    {0x20000e7, (CarryType)6, 0x8000003, 0x510000d3},
    {0x20000e8, (CarryType)6, 0x8000003, 0x510000d4},
    {0x20000e9, (CarryType)9, 0x8000004, 0x510000d5},
    {0x20000e2, (CarryType)9, 0x8000001, 0x510000d6},
    {0x20000e3, (CarryType)9, 0x8000002, 0x510000d7},
    {0x20000e6, (CarryType)9, 0x8000004, 0x510000d8},
    {0x20000e4, (CarryType)9, 0x8000002, 0x510000d9},
    {0x20000d3, (CarryType)9, 0x8000001, 0x510000da},
    {0x20000d6, (CarryType)9, 0x8000002, 0x510000db},
    {0x20000b8, (CarryType)6, 0x8000003, 0x510000d3},
    {0x20000cb, (CarryType)6, 0x8000005, 0x510000dc},
    {0x20000ce, (CarryType)6, 0x8000003, 0x510000dd},
    {0x20000ba, (CarryType)6, 0x8000003, 0x510000de},
    {0x20000cf, (CarryType)6, 0x8000003, 0x510000df},
    {0x20000db, (CarryType)9, 0x8000002, 0x510000e0},
    {0x20000cd, (CarryType)6, 0x8000005, 0x510000e1},
    {0x20000d2, (CarryType)6, 0x8000003, 0x510000e2},
    {0x20000be, (CarryType)9, 0x8000002, 0x510000e3},
    {0x20000ca, (CarryType)9, 0x8000002, 0x51000325},
    {0x2000237, (CarryType)9, 0x8000002, 0x510005e4},
    {0x200017e, (CarryType)9, 0x8000002, 0x51000325},
    {0x2000236, (CarryType)9, 0x8000002, 0x510005e5},
    {0x20000d4, (CarryType)9, 0x8000001, 0x510000e5},
    {0x20000bf, (CarryType)6, 0x8000005, 0x510000e6},
    {0x20000da, (CarryType)9, 0x8000002, 0x510000e7},
    {0x20000dd, (CarryType)9, 0x8000004, 0x510000e8},
    {0x20000d0, (CarryType)6, 0x8000003, 0x510000e9},
    {0x20000e0, (CarryType)9, 0x8000002, 0x510000ea},
    {0x20000df, (CarryType)9, 0x8000002, 0x510000eb},
    {0x20000de, (CarryType)9, 0x8000002, 0x510000ec},
    {0x20000d8, (CarryType)9, 0x8000004, 0x510006bf},
    {0x20000dc, (CarryType)9, 0x0, 0x510000ed},
    {0x2000271, (CarryType)9, 0x0, 0x510006d6},
    {0x200024a, (CarryType)9, 0x0, 0x510006c1},
    {0x200024b, (CarryType)9, 0x0, 0x510006c1},
    {0x20000d1, (CarryType)9, 0x8000003, 0x510000ee},
    {0x20000e1, (CarryType)9, 0x8000004, 0x510000f3},
    {0x20000cc, (CarryType)6, 0x8000003, 0x510000f0},
    {0x20000c5, (CarryType)9, 0x8000004, 0x510000d8},
    {0x20000d9, (CarryType)9, 0x8000004, 0x510000d8},
    {0x20000d5, (CarryType)9, 0x8000001, 0x510000f1},
    {0x2000120, (CarryType)6, 0x8000003, 0x51000135},
    {0x2000139, (CarryType)6, 0x0, 0x51000136},
    {0x2000159, (CarryType)6, 0x8000003, 0x510001ed},
    {0x2000249, (CarryType)6, 0x8000003, 0x5100063c},
    {0x200016d, (CarryType)9, 0x0, 0x510001ef},
    {0x2000184, (CarryType)9, 0x0, 0x5100031a},
    {0x2000185, (CarryType)9, 0x8000004, 0x5100031b},
    {0x2000187, (CarryType)9, 0x8000001, 0x5100031c},
    {0x2000188, (CarryType)9, 0x8000001, 0x5100031d},
    {0x2000189, (CarryType)6, 0x8000003, 0x5100031e},
    {0x2000186, (CarryType)9, 0x8000003, 0x5100031f},
    {0x200018e, (CarryType)9, 0x8000003, 0x510005e7},
    {0x200018c, (CarryType)9, 0x8000003, 0x51000320},
    {0x200018d, (CarryType)6, 0x8000003, 0x51000321},
    {0x20001fc, (CarryType)5, 0x0, 0x510004cd},
    {0x20001fe, (CarryType)5, 0x0, 0x510004ce},
    {0x20001ff, (CarryType)5, 0x0, 0x510004cf},
    {0x2000202, (CarryType)5, 0x0, 0x510004d0},
    {0x2000200, (CarryType)5, 0x0, 0x510004d1},
    {0x2000201, (CarryType)5, 0x0, 0x510004d2},
    {0x2000203, (CarryType)5, 0x0, 0x510004d3},
    {0x2000204, (CarryType)5, 0x0, 0x510004d4},
    {0x2000232, (CarryType)6, 0x8000003, 0x510005e6},
    {0x2000231, (CarryType)6, 0x8000003, 0x510005e8},
};
static ActorCarryable *s_apCarryables[256];
float g_afBeamScale[10];
ActorCarryableGun *g_pBeamOwner[10];
unsigned int TracerFireManager::ms_ulAllocated;
int g_iBeamCount;
EXVector vZero;
EXVector g_avBeamStart[10];
ex::RGBA g_aoBeamColour[10];
EXDList ActorCarryable::gActorCarryableList;
// Retail initialises this in _$E15; this unit numbers it _$E16 (one more $E number is used up
// before it here, source unknown), so the initialiser is not credited by name. Same code.
TracerFire TracerFireManager::ms_aoTracers[200];
EXVector g_avBeamEnd[10];

ActorCarryable *ActorCarryable::GetPtrFromID(int id)
{
    if (!id) return 0;
    ActorCarryable *p = s_apCarryables[id & 0xff];
    if (!p) return 0;
    return p->m_RefID == id ? p : 0;
}

// Next free carryable id (the low byte indexes s_apCarryables).
static int ActorCarryable_GetNewID()
{
    for (int i = 0; i < 0x100; i++) {
        s_LastID++;
        if (!s_apCarryables[s_LastID & 0xff]) return s_LastID;
    }
    return 0;
}

// Weapons whose muzzle script is the shot itself (flame throwers).
static signed char WeaponIsTheMuzzleFlash(int id)
{
    return id == 0x4900002d || id == 0x4900003c || id == 0x4900005a;  // HT_Weaponry_Drone_MS_FreezeThrower, HT_Weaponry_Drone_M16GL_Freeze
}

void ActorCarryable::BootInit()
{
    memset(s_apCarryables, 0, sizeof(s_apCarryables));
}

void ActorCarryable::SetHide(signed char b) { m_bHide = b; }

// NEAR: retail inlines EXVector::operator+ (known VC7.1 inliner-budget issue); ours calls it
EXVector ActorCarryable::GetWorldPos()
{
    if (m_pItem) return m_pItem->Position();
    Actor *pActor = Actor::GetPtrFromID(m_Owner);
    if (pActor) return pActor->GetFeetPos() + m_pAnimator->Position();
    return EXVector(0.0f, 0.0f, 0.0f, 0.0f);
}

void ActorCarryable::Delete()
{
    m_bDelete = 1;
    m_DeleteTimer = 10;
    SetOwner(0);
}

void ActorCarryable::Update()
{
    ActorCarryable *pNext;
    for (ActorCarryable *p = GetFirst(); p; p = pNext) {
        pNext = p->GetNext();
        p->DoUpdate();
        if (p->NeedsDelete()) delete p;
    }
}

void ActorCarryable::PreLoadInit()
{
    for (int i = 0; i < 256; i++) {
        if (s_apCarryables[i]) {
            delete s_apCarryables[i];
            s_apCarryables[i] = 0;
        }
    }
    BootInit();
}

CarryType ActorCarryable::FindCarryTypeForEntity(int entity)
{
    for (int i = 0; i < (int)(sizeof(s_CarryTypes) / sizeof(s_CarryTypes[0])); i++) {
        if (s_CarryTypes[i].m_Entity == entity) return s_CarryTypes[i].m_Type;
    }
    return CARRY_NONE;
}

unsigned int ActorCarryable::FindTextForEntity(int entity)
{
    for (int i = 0; i < (int)(sizeof(s_CarryTypes) / sizeof(s_CarryTypes[0])); i++) {
        if (s_CarryTypes[i].m_Entity == entity) return s_CarryTypes[i].m_Text;
    }
    return 0x510000f5;
}

WeaponProfile *ActorCarryable::GetWeaponProfile() const
{
    WeaponProfile *p = WeaponProfile::GetWeaponDataFromIndex(m_WeaponIndex);
    if (!p) {
        Actor *pActor = Actor::GetPtrFromID(GetOwner());
        if (pActor) {
            switch (pActor->GetProfile()->m_SubGroup) {
            case 0x4f00000f:
                return WeaponProfile::GetWeaponData(0x49000036);
            case 0x4f00000e:
                return WeaponProfile::GetWeaponData(0x49000047);
            case 0x4f00000c:
                return WeaponProfile::GetWeaponData(0x49000048);
            case 0x4f000010:
                return WeaponProfile::GetWeaponData(0x49000049);
            case 0x4f000016:
                return WeaponProfile::GetWeaponData(0x4900004a);
            case 0x4f000017:
                return WeaponProfile::GetWeaponData(0x4900004b);
            case 0x4f000021:
                return WeaponProfile::GetWeaponData(0x4900004c);
            case 0x4f000014:
                return WeaponProfile::GetWeaponData(0x49000053);
            default:
                if (pActor->m_ActorFlags.m_bBoss) return WeaponProfile::GetWeaponData(0x49000036);
                return WeaponProfile::GetWeaponData(0x49000034);
            }
        }
    }
    return p;
}

void ActorCarryableGun::ChangeProfile(int id)
{
    m_WeaponIndex = WeaponProfile::GetWeaponDataIndex(id);
    WeaponProfile *p = WeaponProfile::GetWeaponDataFromIndex(m_WeaponIndex);
    m_Mode = 4;
    m_Unkcc = 0.0f;
    m_RoundTime = 0.0f;
    m_Wait = p->GetWait();
    m_Clip = p->m_ClipSize;
}

void ActorCarryableGun::DelayWait(float t)
{
    m_Wait += EXBaseApp::GameFrameRateMul() * t;
}

void ActorCarryableGun::ResetBulletsPerFrame() { ms_iInstantBulletsThisFrame = 5; }

void ActorCarryableGun::ChangeMode()
{
    WeaponProfile *p = WeaponProfile::GetWeaponDataFromIndex(m_WeaponIndex);
    if (p->m_Id == 0x4900003d) ChangeProfile(0x4900003e);
    else if (p->m_Id == 0x4900003e) ChangeProfile(0x4900003f);
    else if (p->m_Id == 0x4900003f) ChangeProfile(0x4900003d);
    if (gCurrentMap_GeoHash == 0x1000121) {
        if (p->m_Id == 0x49000042) ChangeProfile(0x49000043);
        else if (p->m_Id == 0x49000043) ChangeProfile(0x49000042);
    } else if (p->m_Id == 0x49000042) {
        Actor *pActor = Actor::GetPtrFromID(m_Owner);
        if (pActor && pActor->GetProfile()->m_Health * pActor->GetProfile()->m_GunSwapHealthPct * 0.01f >= pActor->m_Health)
            ChangeProfile(0x49000043);
    }
}

void ActorCarryableGun::ExtChangeMode()
{
    int id = WeaponProfile::GetWeaponDataFromIndex(m_WeaponIndex)->m_Id;
    if (id == 0x4900001d) ChangeProfile(0x4900003c);
    else if (id == 0x4900003c) ChangeProfile(0x4900001d);
}

void ActorCarryableGun::PauseFiring()
{
    if (m_Mode != 7) {
        m_PrevMode = m_Mode;
        m_Mode = 7;
        m_PauseTime = EXBaseApp::NominalGameFrameRate() * 0.5f;
    }
}

void ActorCarryableGun::StopFiring()
{
    if (m_WeaponIndex != -1) {
        m_Wait = WeaponProfile::GetWeaponDataFromIndex(m_WeaponIndex)->GetWait();
        m_Mode = 4;
        m_SoundTag.KillSoundPlayingLooping();
        m_BeamSoundTag.KillSoundPlayingLooping();
    }
}

void ActorCarryableGun::UpdateWait()
{
    if (m_Mode == 4 && m_Wait > 0.0f) m_Wait -= EXBaseApp::GameFrameRateMul();
}

signed char ActorCarryableGun::WaitOver() const
{
    if (m_Wait <= 0.0f) return 1;
    return 0;
}

signed char ActorCarryableGun::CanFire() const
{
    if (m_WeaponIndex == -1) return 0;
    WeaponProfile *p = WeaponProfile::GetWeaponDataFromIndex(m_WeaponIndex);
    if (m_Wait <= 0.0f && p->m_LockTimer <= 0.0f) return 1;
    return 0;
}

signed char ActorCarryableGun::StartFiring()
{
    if (m_Clip > 0 && m_WeaponIndex != -1) {
        m_Mode = WeaponProfile::GetWeaponDataFromIndex(m_WeaponIndex)->m_FireType != 1 ? 3 : 0;
        Actor *pActor = Actor::GetPtrFromID(m_Owner);
        if (pActor && pActor->SeenBody()) {
            if (!EXRand(10)) pActor->Speak(pActor->GetWarcrySFX(), 0);
        }
        return 1;
    }
    return 0;
}

void ActorCarryableGun::Reload()
{
    if (m_WeaponIndex != -1) {
        WeaponProfile *p = WeaponProfile::GetWeaponDataFromIndex(m_WeaponIndex);
        m_SoundTag.StartSound(p->m_ReloadSound, GetWorldPos(), -1, 0);
        m_Wait = p->GetWait() * 0.5f;
        m_Clip = p->m_ClipSize;
    }
}

void ActorCarryableGun::RefillClipInstantly()
{
    if (m_WeaponIndex != -1) m_Clip = WeaponProfile::GetWeaponDataFromIndex(m_WeaponIndex)->m_ClipSize;
}

void ActorCarryableGun::SetInitialWait()
{
    if (m_WeaponIndex != -1) {
        WeaponProfile *p = WeaponProfile::GetWeaponDataFromIndex(m_WeaponIndex);
        float w = (float)p->m_WaitMin;
        m_Wait = g_EXRandClass.Randf() * w;
    }
}

EXVector ActorCarryableGun::GetMuzzleWorldPos()
{
    EXVector pos, dir;
    GetMuzzleDatum(pos, dir);
    return pos;
}

signed char ActorCarryableGun::IsSniper() const
{
    switch (GetWeaponProfile()->m_Id) {
    case 0x49000016:
    case 0x4900002c:
    case 0x49000030:
    case 0x49000037:
        return 1;
    }
    return 0;
}

void ActorCarryable::DumpDebugInfo() const
{
    ex::TRACE("%s", EXHashCodeFile::Debug_HashCodeName(m_EntityHash));
}

void ActorCarryableGun::DumpDebugInfo() const
{
    ex::TRACE("%s", EXHashCodeFile::Debug_HashCodeName(m_EntityHash));
}

int ActorCarryableGun::GetLocationType() const { return m_LocationType; }

TracerFire::TracerFire() { Reset(); }

void TracerFire::Reset()
{
    m_Start.set(0.0f);
    m_End.set(0.0f);
    m_Colour = ex::RGBA(0, 0, 0, 0);
    m_Width = 0.0f;
    m_Dist = 0.0f;
    m_MaxDist = 0.0f;
    m_bFree = 1;
    m_bDrawMe = 0;
    m_Length = 0.0f;
    m_Speed = 0.0f;
    m_WeaponIndex = 0;
    m_Unk40 = 0;
    m_Unk20 = 0.0f;
}

void TracerFireManager::DrawTracers()
{
    if (!ms_ulAllocated) return;
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(0x1000016, 0, 0);
    if (!pFile) return;
    gpGameWnd[0]->SetTriFanZMode(0, 1);
    gpGameWnd[0]->SetTriFanBlendMode(2);
    unsigned int n = ms_ulAllocated;
    TracerFire *p = ms_aoTracers;
    for (int i = 0; n && i < 200; i++, p++) {
        if (!p->IsFree() && p->DrawMe()) {
            p->Draw(pFile);
            n--;
        }
    }
}

void AddBeam(ActorCarryableGun *pGun, EXVector start, const EXVector &end, ex::RGBA col, float scale)
{
    if (g_iBeamCount < 10) {
        g_pBeamOwner[g_iBeamCount] = pGun;
        g_avBeamStart[g_iBeamCount] = start;
        g_avBeamEnd[g_iBeamCount] = end;
        g_aoBeamColour[g_iBeamCount] = col;
        g_afBeamScale[g_iBeamCount] = scale;
        g_iBeamCount++;
    }
}

TracerFire *TracerFireManager::GetFreeTracer()
{
    TracerFire *p = ms_aoTracers;
    for (int i = 0; i < 200; i++, p++) {
        if (p->IsFree()) {
            p->SetFree(0);
            ms_ulAllocated++;
            return p;
        }
    }
    return 0;
}

void TracerFireManager::FreeTracer(TracerFire *p)
{
    if (p) {
        p->Reset();
        ms_ulAllocated--;
    }
}

EX_IMPLEMENT_DYNCREATE_ROOT(XItemHandler_AntiCloakSmoke)

XItemHandler_AntiCloakSmoke::~XItemHandler_AntiCloakSmoke() {}

void CreateAntiCloakSmoke(const EXVector &pos)
{
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(0x1000016, 0, 0);
    XItem *p = XItem::CreateItem(pFile, 0x1000016, EXRUNTIME_CLASS(XItemHandler_AntiCloakSmoke),
                                 EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    p->SetPosition(pos);
    theItemEnv.AddItem(p, 0);
}

void CreateActorDroppedMine(Actor &actor)
{
    EXVector a(0.0f, 0.0f, 0.0f, 0.0f);
    EXVector b(0.0f, 0.0f, 0.0f, 0.0f);
    EXVector pos = actor.GetHandPos(0);
    XItemHandler_Bullet::CreateFromHash(pos, b, a, 0x4900000f, 0, actor.m_RefID, 0, 1.0f);
}

ActorCarryable::~ActorCarryable()
{
    if (m_pItem) m_pItem->ItemFlags() = 0x10;
    m_pItem = 0;
    GeoSectionManager::Del(m_GeoHash, m_EntityHash);
    GeoSectionManager::Del(0x100000f, m_Unk78);
    gActorCarryableList.Remove(&m_Link);
    s_apCarryables[m_RefID & 0xff] = 0;
}

signed char ActorCarryable::DoUpdate()
{
    if (!m_bLoaded && !m_bDelete && GeoSectionManager::IsLoaded(m_GeoHash, m_EntityHash) &&
        (!m_Unk78 || GeoSectionManager::IsLoaded(0x100000f, m_Unk78))) {
        m_pAnimator->Initialise(EXGeoFile::GetGeoFile(m_GeoHash), m_EntityHash);
        m_pAnimator->RenderFlags().BitSet(1);
        m_bLoaded = 1;
    }
    if (!m_bDelete && !gActorPause && m_pItem) {
        if (m_bFading) {
            if (0.0f >= m_FadeTime) Delete();
        } else {
            m_bFading = 1;
        }
    }
    UpdateFall();
    UpdateDistFade();
    UpdateAlpha();
    return 1;
}

void ActorCarryableGun::DeleteSceneryImpactScript()
{
    if (m_pImpactScript) {
        m_pImpactScript->ItemFlags().BitSet(0x10);
        m_pImpactScript = 0;
    }
    if (m_pImpactScript2) {
        m_pImpactScript2->ItemFlags().BitSet(0x10);
        m_pImpactScript2 = 0;
    }
}

void ActorCarryableGun::UpdateSceneryImpactScript()
{
    if (m_pImpactScript && m_pImpactScript->m_11a) DeleteSceneryImpactScript();
}

void ActorCarryableGun::UpdateTracers()
{
    for (int i = 0; i < m_NumTracers; i++) {
        if (m_ppTracers[i]) {
            m_ppTracers[i]->Update();
            if (!m_ppTracers[i]->DrawMe()) {
                TracerFireManager::FreeTracer(m_ppTracers[i]);
                m_ppTracers[i] = 0;
            }
        }
    }
}

int ActorCarryableGun::GetFreeTracer()
{
    for (int i = 0; i < m_NumTracers; i++) {
        if (!m_ppTracers[i]) {
            m_ppTracers[i] = TracerFireManager::GetFreeTracer();
            if (m_ppTracers[i]) return i;
        }
    }
    return -1;
}

void ActorCarryableGun::DeleteTracers()
{
    for (int i = 0; i < m_NumTracers; i++) {
        if (m_ppTracers[i]) TracerFireManager::FreeTracer(m_ppTracers[i]);
    }
    if (m_ppTracers) delete[] m_ppTracers;
}

void ActorCarryableGun::DeleteMuzzleScript()
{
    if (m_pMuzzleScript) {
        m_pMuzzleScript->ItemFlags().BitSet(0x10);
        m_pMuzzleScript = 0;
    }
}

void ActorCarryableGun::StopLoopingSFX()
{
    if (m_WeaponIndex != -1) {
        m_SoundTag.KillSoundPlayingLooping();
        m_BeamSoundTag.KillSoundPlayingLooping();
        DeleteMuzzleScript();
    }
}

s8 XItemHandler_AntiCloakSmoke::Initialise()
{
    m_pScript = new EXItemAnimator_Script;
    if (m_pScript) {
        EXGeoFile *pFile = EXGeoFile::GetGeoFile(0x1000016);
        if (pFile) {
            unsigned int hash = 0x400027d;
            if (gCurrentMap_GeoHash == 0x10000f5) hash = 0x40004aa;
            if (m_pScript->Initialise(pFile, hash)) {
                m_pScript->SetPosition(EXVector(0.0f, 0.0f, 0.0f, 0.0f));
                m_pScript->SetRotation(EXQuaternion(0.0f, 0.0f, 0.0f, 0.0f));
                m_pScript->RenderFlags().BitSet(1);
                m_pItem->AddAnimator(m_pScript);
                return 1;
            }
        }
    }
    m_pItem->ItemFlags() = 0x10;
    return 0;
}

ActorCarryable *ActorCarryable::Create(int file, int entity)
{
    if (entity && file && DBG_bActorsAndVehicles) {
        if (FindCarryTypeForEntity(entity) != CARRY_GUN) return new ActorCarryable(file, entity);
        return new ActorCarryableGun(file, entity);
    }
    return 0;
}

ActorCarryableGun::~ActorCarryableGun()
{
    DeleteTracers();
    DeleteMuzzleScript();
    DeleteSceneryImpactScript();
}

void ActorCarryableGun::SetOwner(int id)
{
    ActorCarryable::SetOwner(id);
    DeleteMuzzleScript();
}

void ActorCarryableGun::CreateTracer(EXVector const &a, EXVector const &b)
{
    WeaponProfile *p = GetWeaponProfile();
    if (p && p->m_Tracers) {
        int i = GetFreeTracer();
        if (i != -1) m_ppTracers[i]->Init(a, b, m_WeaponIndex, m_Owner);
    }
}

signed char ActorCarryableGun::DoUpdate()
{
    signed char r = ActorCarryable::DoUpdate();
    if (r) {
        UpdateMuzzleScript();
        UpdateTracers();
        UpdateSceneryImpactScript();
    }
    return r;
}

ActorCarryable::ActorCarryable(int file, int entity)
{
    m_RefID = ActorCarryable_GetNewID();
    s_apCarryables[m_RefID & 0xff] = this;
    m_Unk2c = 150;
    m_Unk30 = 150;
    m_bDelete = 0;
    m_DeleteTimer = 10;
    m_Unk1c = 0.0f;
    m_Unk20 = 0.0f;
    m_Owner = 0;
    m_Unk24 = 1.0f;
    m_FloorY = 0.0f;
    m_Unk40 = 0.0f;
    m_Type = (CarryType)4;
    m_bHide = 0;
    m_bFading = 0;
    m_FadeTime = 1.0f;
    m_pItem = 0;
    m_GeoHash = file;
    m_EntityHash = entity;
    m_bLoaded = 0;
    EXItemAnimator_Entity *pAnim = new EXItemAnimator_Entity;
    m_pAnimator = pAnim;
    m_Unk78 = 0;
    m_Description = 0x5100005a;  // HT_Text_BLANK
    m_bGun = 0;
    for (int i = 0; i < (int)(sizeof(s_CarryTypes) / sizeof(s_CarryTypes[0])); i++) {
        if (s_CarryTypes[i].m_Entity == entity) {
            m_Type = s_CarryTypes[i].m_Type;
            m_Unk78 = s_CarryTypes[i].m_Unk8;
            m_Description = s_CarryTypes[i].m_Text;
            break;
        }
    }
    pAnim->RenderInfo().m_pExtRenderInfo = &mExtRenderInfo;
    pAnim->RenderInfo().RenderFlagsRef().m_flags |= 0x40;
    m_WeaponIndex = WeaponProfile::GetWeaponDataIndexFromEntity(entity);
    GeoSectionManager::Add(m_GeoHash, m_EntityHash);
    GeoSectionManager::Add(0x100000f, m_Unk78);  // HT_File_DRONE_Gen
    gActorCarryableList.AddTail(&m_Link);
}

// The muzzle datum in world space: position, and the barrel direction (the datum's local +z).
signed char ActorCarryableGun::GetMuzzleDatumCorrect(EXVector &pos, EXVector &dir)
{
    Actor *pOwner = Actor::GetPtrFromID(m_Owner);
    if (!pOwner || !m_pAnimator) return 0;
    EXDatumTemp d;
    memset(&d, 0, sizeof(d));
    if (m_pAnimator->GetDatum(0x10000037, d, 0, EXItemAnimator::m_pNullDatumPtr)) {  // HT_AnimDatum_Cannon_Barrel_Muzzle
        EXMatrix m;
        pOwner->GetItem()->CalcMatrix(m);
        d.ApplyXForm(m);
        pos = d.m_position;
        dir = EXVector(0.0f, 0.0f, 1.0f, 0.0f);
        EXMatrix mq;
        mq.set_identity();
        mq.set_rotate_quat(d.m_rotation);
        EXVector t;
        D3DXVec4Transform(&t, &dir, &mq);
        dir = t;
        pos.w = 0.0f;
        dir.w = 0.0f;
        return 1;
    }
    return 0;
}

// Older form: returns the datum's rotation quaternion as dir (the transformed vector is unused), or
// the item position and its facing when the gun has no muzzle datum.
void ActorCarryableGun::GetMuzzleDatum(EXVector &pos, EXVector &dir)
{
    Actor *pOwner = Actor::GetPtrFromID(m_Owner);
    if (!pOwner || !m_pAnimator) return;
    EXDatumTemp d;
    memset(&d, 0, sizeof(d));
    if (m_pAnimator->GetDatum(0x10000037, d, 0, EXItemAnimator::m_pNullDatumPtr)) {  // HT_AnimDatum_Cannon_Barrel_Muzzle
        EXMatrix m;
        pOwner->GetItem()->CalcMatrix(m);
        d.ApplyXForm(m);
        pos = d.m_position;
        dir = *(EXVector *)&d.m_rotation;
        EXVector fwd(0.0f, 0.0f, 0.0f, 1.0f);
        EXMatrix mq;
        mq.set_identity();
        mq.set_rotate_quat(d.m_rotation);
        EXVector out;
        D3DXVec4Transform(&out, &fwd, &mq);
        pos.w = 0.0f;
        dir.w = 0.0f;
        return;
    }
    pos = GetWorldPos();
    dir.x = (float)sin(m_pAnimator->Rotation().y);
    dir.y = 0.0f;
    dir.z = (float)cos(m_pAnimator->Rotation().y);
    dir.w = 0.0f;
    pos.w = 0.0f;
    dir.w = 0.0f;
}

static __forceinline void Carry_Normalise(EXVector &v)
{
    float len = v.Length();
    if (len > EX_EPSILON_UNIT) v *= 1.0f / len;
}

void TracerFire::Init(EXVector start, const EXVector &dir, int weaponIndex, int d)
{
    m_WeaponIndex = weaponIndex;
    WeaponProfile *p = WeaponProfile::GetWeaponDataFromIndex(weaponIndex);
    ex::RGBA col((u8)p->m_TracerR, (u8)p->m_TracerG, (u8)p->m_TracerB, (u8)p->m_TracerA);
    m_Start = start;
    m_End = dir;
    m_Colour = col;
    m_Width = p->m_TracerWidth;
    m_Dist = 0.0f;
    m_Unk40 = d;
    m_bDrawMe = 1;
    m_bUnk4 = 0;
    m_Speed = p->m_ThrowSpeed;
    m_Length = p->m_TracerLength;
    m_MaxDist = p->m_Range + m_Length;
    Carry_Normalise(m_End);
}

static __forceinline int Carry_RandBit()
{
    const int &r = EXRand32() & 1;
    return r;
}

static __forceinline float Carry_LengthXYZ(const EXVector &v)
{
    float sq = v.y * v.y + v.z * v.z + v.x * v.x;
    float len = 0.0f;
    if (sq >= 0.0f) len = (float)sqrt(sq);
    return len;
}

// Advances the tracer. The section crossed this frame is fired as an instant bullet; when it hits,
// the tracer is cut short at the hit and, for scenery hits, plays the weapon's impact effect (half
// the time, when near the camera and on screen) and impact sound.
void TracerFire::Update()
{
    if (!m_bDrawMe) return;
    float prev = m_Dist;
    m_Dist = m_Speed * XApp::GameFrameRateMul() + prev;
    if (m_Dist >= m_MaxDist) {
        m_bDrawMe = 0;
        return;
    }
    if (m_bUnk4) return;
    EXVector from = m_End * prev + m_Start;
    EXVector hit = m_End * m_Dist + m_Start;
    EXVector to = hit;
    EXVector dir = to - from;
    WeaponProfile *p = WeaponProfile::GetWeaponDataFromIndex(m_WeaponIndex);
    float len = Carry_LengthXYZ(dir);
    signed char bScenery;
    signed char bActor = XItemHandler_Bullet::InstantCreate(p, from, to, dir, m_Unk40, 0, p->GetBulletDamage(), len, &hit, &bScenery);
    if (!bScenery && !bActor) return;
    EXVector d = hit - m_Start;
    m_MaxDist = m_Length + Carry_LengthXYZ(d);
    m_bUnk4 = 1;
    if (!bScenery || bActor) return;
    if (p->m_ExplosionScript && !Carry_RandBit()) {
        EXVector c = XGetCamera(0)->GetItem()->Position() - hit;
        float r = XGetCamera(0)->Zoom() * 30.0f;
        if (c.y * c.y + c.z * c.z + c.x * c.x < r * r) {
            EXSphere sph;
            sph.Set(hit, 0.01f);
            if (XGetCamera(0)->BoundingVolumeOnScreen(sph))
                XItemHandler_ScriptBase::FX_Create(hit, p->m_ExplosionScript, 0x40, 0x1000016);  // HT_File_Pred_FX
        }
    }
    if (p->m_ImpactSound)
        EXBaseSoundManager::GetSoundManager()->StartSound(p->m_ImpactSound, hit, -1, 0);
}

// One quad of a tracer or beam: four of the section corners drawn as a fan (the fifth vertex, a copy
// of the first, is not drawn).
struct Carry_Pos { float x, y, z, w; };

static __forceinline void Carry_DrawQuad(EXVector *p, int a, int b, int c, int d, ex::RGBA col,
                                         float u0, float v0, float u1, float v1, float u2, float v2, float u3, float v3)
{
    EXPrimVert v[5];
    *(Carry_Pos *)&v[0] = *(Carry_Pos *)&p[a];
    *(Carry_Pos *)&v[1] = *(Carry_Pos *)&p[b];
    *(Carry_Pos *)&v[2] = *(Carry_Pos *)&p[c];
    *(Carry_Pos *)&v[3] = *(Carry_Pos *)&p[d];
    v[0].m_Color = col;
    v[1].m_Color = col;
    v[2].m_Color = col;
    v[3].m_Color = col;
    v[0].u = u0;
    v[0].v = v0;
    v[1].u = u1;
    v[1].v = v1;
    v[2].u = u2;
    v[2].v = v2;
    v[3].u = u3;
    v[3].v = v3;
    v[4] = v[0];
    gpGameWnd[0]->DrawTriFan(4, v);
}

void TracerFire::DrawSection(EXVector *p, int a, int b, int c, int d, ex::RGBA col) const
{
    Carry_DrawQuad(p, a, b, c, d, col, 0.05f, 0.95f, 0.05f, 0.05f, 0.95f, 0.05f, 0.95f, 0.95f);
}

void Draw_WeaponBeamSection(EXVector *p, int a, int b, int c, int d, ex::RGBA col)
{
    Carry_DrawQuad(p, a, b, c, d, col, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f);
}


// Draws the tracer as two crossed textured quads along the visible streak: from m_Length behind the
// head (or the muzzle, early on) to the head, stopping at the hit point. The colour follows the
// player's vision mode (no red in mode 3, no green in mode 1).
void TracerFire::Draw(EXGeoFile *pFile) const
{
    static EXVector s_Corners[8] = {
        EXVector(0.0f, 1.0f, 0.0f, 0.0f), EXVector(0.0f, 0.0f, 1.0f, 0.0f),
        EXVector(0.0f, -1.0f, 0.0f, 0.0f), EXVector(0.0f, 0.0f, -1.0f, 0.0f),
        EXVector(1.0f, 1.0f, 0.0f, 0.0f), EXVector(1.0f, 0.0f, 1.0f, 0.0f),
        EXVector(1.0f, -1.0f, 0.0f, 0.0f), EXVector(1.0f, 0.0f, -1.0f, 0.0f),
    };
    WeaponProfile *p = WeaponProfile::GetWeaponDataFromIndex(m_WeaponIndex);
    EXTexture *pTex = pFile->GeoHeader()->Texture(p->m_Tracers);
    if (!pTex) return;
    gpGameWnd[0]->SelectTriFanTexture(pTex);
    static EXVector s_XAxis(0.0f, 1.0f, 0.0f, 0.0f);
    EXVector tail;
    if (m_Length > m_Dist)
        tail = m_Start;
    else
        tail = m_End * (m_Dist - m_Length) + m_Start;
    EXVector head;
    float f = m_MaxDist - m_Length;
    if (m_Dist > f)
        head = m_End * f + m_Start;
    else
        head = m_End * m_Dist + m_Start;
    EXVector d = tail - head;
    float len = Carry_LengthXYZ(d);
    EXQuaternion q;
    q.MakeQuaternion(m_End, s_XAxis);
    EXMatrix scale;
    scale.r[0].x = len;
    scale.r[0].y = 0.0f;
    scale.r[0].z = 0.0f;
    scale.r[0].w = 0.0f;
    scale.r[1].x = 0.0f;
    scale.r[1].y = m_Width;
    scale.r[1].z = 0.0f;
    scale.r[1].w = 0.0f;
    scale.r[2].x = 0.0f;
    scale.r[2].y = 0.0f;
    scale.r[2].z = m_Width;
    scale.r[2].w = 0.0f;
    scale.r[3].x = 0.0f;
    scale.r[3].y = 0.0f;
    scale.r[3].z = 0.0f;
    scale.r[3].w = 0.0f;
    EXMatrix rot;
    rot.set_rotate_quat(q);
    EXMatrix m;
    D3DXMatrixMultiply(&m, &scale, &rot);
    rot = m;
    EXVector verts[8];
    int i = 0;
    int n = 8;
    do {
        EXVector t;
        D3DXVec4Transform(&t, &s_Corners[i], &rot);
        verts[i] = t + tail;
        i++;
    } while (--n);
    ex::RGBA col = m_Colour;
    if (XItemHandler_Player::GetInstance()->GetVisionMode() == 3)
        col.m_R = 0;
    else if (XItemHandler_Player::GetInstance()->GetVisionMode() == 1)
        col.m_G = 0;
    DrawSection(verts, 0, 2, 6, 4, col);
    DrawSection(verts, 1, 3, 7, 5, col);
}

// Draws the beams queued by AddBeam this frame as two crossed force-field quads each (the corner ring
// is an octagon, two opposite pairs drawn), moves each owner's looping beam sound to the point on
// the beam nearest the player, then clears the queue. Tracers are drawn first.
void Draw_WeaponBeams()
{
    TracerFireManager::DrawTracers();
    static EXVector s_Ring[16] = {
        EXVector(0.0f, 1.0f, 0.0f, 0.0f), EXVector(0.0f, 0.707f, 0.707f, 0.0f),
        EXVector(0.0f, 0.0f, 1.0f, 0.0f), EXVector(0.0f, -0.707f, 0.707f, 0.0f),
        EXVector(0.0f, -1.0f, 0.0f, 0.0f), EXVector(0.0f, -0.707f, -0.707f, 0.0f),
        EXVector(0.0f, 0.0f, -1.0f, 0.0f), EXVector(0.0f, 0.707f, -0.707f, 0.0f),
        EXVector(1.0f, 1.0f, 0.0f, 0.0f), EXVector(1.0f, 0.707f, 0.707f, 0.0f),
        EXVector(1.0f, 0.0f, 1.0f, 0.0f), EXVector(1.0f, -0.707f, 0.707f, 0.0f),
        EXVector(1.0f, -1.0f, 0.0f, 0.0f), EXVector(1.0f, -0.707f, -0.707f, 0.0f),
        EXVector(1.0f, 0.0f, -1.0f, 0.0f), EXVector(1.0f, 0.707f, -0.707f, 0.0f),
    };
    if (g_iBeamCount <= 0) return;
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(0x1000016, 0, 0);  // HT_File_Pred_FX
    if (!pFile) return;
    EXTexture *pTex = pFile->GeoHeader()->Texture(0x6000198);  // HT_Texture_Forcefield
    if (!pTex) return;
    gpGameWnd[0]->SetTriFanZMode(0, 1);
    gpGameWnd[0]->SetTriFanBlendMode(2);
    gpGameWnd[0]->SelectTriFanTexture(pTex);
    for (int i = 0; i < g_iBeamCount; i++) {
        ex::RGBA col = g_aoBeamColour[i];
        if (XItemHandler_Player::GetInstance()->GetVisionMode() == 3) {
            col.m_R = 0;
        } else if (XItemHandler_Player::GetInstance()->GetVisionMode() == 1) {
            col.m_B = col.m_R >> 2;
            col.m_G = 0;
            col.m_R = 0;
        }
        if (g_pBeamOwner[i] && g_pBeamOwner[i]->BeamSoundTag()->IsPlaying()) {
            float t = 0.0f;
            EXLineSeg seg(g_avBeamStart[i], g_avBeamEnd[i] - g_avBeamStart[i]);
            EXVector player = XItemHandler_Player::GetInstance()->GetItem()->Position();
            EXSqrDistance(player, seg, &t);
            g_pBeamOwner[i]->BeamSoundTag()->UpdatePosition(seg.m_Origin + seg.m_Direction * t);
        }
        EXVector d = g_avBeamStart[i] - g_avBeamEnd[i];
        float len = Carry_LengthXYZ(d);
        EXQuaternion q;
        q.MakeQuaternion(g_avBeamEnd[i] - g_avBeamStart[i], EXVector(0.0f, 1.0f, 0.0f, 0.0f));
        EXMatrix scale;
        scale.r[0].x = len;
        scale.r[0].y = 0.0f;
        scale.r[0].z = 0.0f;
        scale.r[0].w = 0.0f;
        scale.r[1].x = 0.0f;
        scale.r[1].y = g_afBeamScale[i];
        scale.r[1].z = 0.0f;
        scale.r[1].w = 0.0f;
        scale.r[2].x = 0.0f;
        scale.r[2].y = 0.0f;
        scale.r[2].z = g_afBeamScale[i];
        scale.r[2].w = 0.0f;
        scale.r[3].x = 0.0f;
        scale.r[3].y = 0.0f;
        scale.r[3].z = 0.0f;
        scale.r[3].w = 0.0f;
        EXMatrix rot;
        rot.set_identity();
        rot.set_rotate_quat(q);
        EXVector verts[16];
        for (int k = 0; k < 16; k++) {
            verts[k] = s_Ring[k];
            EXVector t1;
            D3DXVec4Transform(&t1, &verts[k], &scale);
            verts[k] = t1;
            EXVector t2;
            D3DXVec4Transform(&t2, &verts[k], &rot);
            verts[k] = t2;
            verts[k] += g_avBeamStart[i];
        }
        Draw_WeaponBeamSection(verts, 0, 4, 12, 8, col);
        Draw_WeaponBeamSection(verts, 2, 6, 14, 10, col);
    }
    g_iBeamCount = 0;
}

static __forceinline void Carry_SetRotationXYZ(EXMaths::mat_44 &m, const EXMaths::vec_xyzw &a)
{
    float sx, cx, sy, cy, sz, cz;
    EXSinCos(a.x, sx, cx);
    EXSinCos(a.y, sy, cy);
    EXSinCos(a.z, sz, cz);
    m.r[0].x = cz * cy; m.r[0].y = cz * sy * sx + sz * cx; m.r[0].z = sz * sx - cz * sy * cx; m.r[0].w = 0.0f;
    m.r[1].x = -(sz * cy); m.r[1].y = cz * cx - sz * sy * sx; m.r[1].z = sz * sy * cx + cz * sx; m.r[1].w = 0.0f;
    m.r[2].x = sy; m.r[2].y = -(cy * sx); m.r[2].z = cy * cx; m.r[2].w = 0.0f;
    m.r[3].x = 0.0f; m.r[3].y = 0.0f; m.r[3].z = 0.0f; m.r[3].w = 1.0f;
}

// The melee collision datum of a held weapon, in world space (placed by the owner's position and
// euler rotation).
signed char ActorCarryable::GetWeaponCollisionPos(EXVector &pos) const
{
    if (!m_pAnimator) return 0;
    Actor *pOwner = Actor::GetPtrFromID(m_Owner);
    if (!pOwner) return 0;
    EXDatumTemp d;
    memset(&d, 0, sizeof(d));
    if (m_pAnimator->GetDatum(0x1000009d, d, 0, EXItemAnimator::m_pNullDatumPtr)) {  // HT_AnimDatum_Melee_Collision
        pos = d.m_position;
        EXItem *pItem = pOwner->GetItem();
        EXMatrix m;
        Carry_SetRotationXYZ(m, pItem->Rotation());
        m.r[3] = pItem->Position();
        m.r[3].w = 1.0f;
        EXVector t;
        D3DXVec4Transform(&t, &pos, &m);
        pos = t;
        return 1;
    }
    EXHashCodeFile::Debug_HashCodeName(m_EntityHash);
    return 0;
}

// Moves the item to a new owner (0: dropped). A dropped item is given its own world item at the old
// owner's feet, lying on its side, unless there is no floor below (then it is deleted).
void ActorCarryable::SetOwner(int id)
{
    if (!id) StopLoopingSFX();
    if (m_Owner == id) return;
    Actor *pOld = Actor::GetPtrFromID(m_Owner);
    Actor *pNew = Actor::GetPtrFromID(id);
    if (pOld) {
        if (!pOld->BeingDestructed()) pOld->GetItem()->RemoveAnimator(m_pAnimator);
        pOld->__CarryFlush(m_RefID);
        if (!pNew && !m_bDelete) {
            EXVector floor = AU_SnapFloorPos(pOld->GetItem()->Position(), 0, 1);
            if (floor.y > pOld->GetItem()->Position().y) {
                m_bDelete = 1;
            } else {
                m_FloorY = floor.y;
                m_Unk40 = 0.0f;
                m_pAnimator->Initialise(EXGeoFile::GetGeoFile(m_GeoHash), m_EntityHash);
                m_pItem = XItem::CreateItem(0, 0, 0, EXVector(0.0f, 0.0f, 0.0f, 0.0f));
                m_pItem->SetPosition(m_pAnimator->Position() + pOld->GetItem()->Position());
                m_pItem->SetMapOn(theItemEnv.GetMapOn(m_pItem->Position()));
                m_pItem->AddAnimator(m_pAnimator);
                m_pAnimator->SetPosition(vZero);
                m_pAnimator->SetRotation(EXQuaternion(0.0f, 1.5707964f, -1.5707964f, 0.0f));
                theItemEnv.AddItem(m_pItem, 1);
            }
            goto done;
        }
    }
    if (m_pItem) {
        m_pItem->ItemFlags().BitSet(0x10);
        m_pItem = 0;
    }
    if (pNew) pNew->GetItem()->AddAnimator(m_pAnimator);
done:
    m_Owner = id;
    m_Unk2c = 150;
}

ActorCarryableGun::ActorCarryableGun(int file, int entity) : ActorCarryable(file, entity)
{
    if (m_WeaponIndex != -1) {
        WeaponProfile *p = WeaponProfile::GetWeaponDataFromIndex(m_WeaponIndex);
        m_pMuzzleScript = 0;
        m_pImpactScript = 0;
        m_pImpactScript2 = 0;
        m_bGun = 1;
        m_Mode = 4;
        m_Unkcc = 0.0f;
        m_RoundTime = 0.0f;
        m_Wait = p->GetWait();
        m_Clip = (s16)p->m_ClipSize;
        m_ppTracers = 0;
        m_NumTracers = p->m_TracerCount;
        if (m_NumTracers > 0) {
            m_ppTracers = (TracerFire **)_EXAlloc(m_NumTracers << 2, 0);
            if (m_ppTracers) {
                for (int i = 0; i < m_NumTracers; i++) m_ppTracers[i] = 0;
            } else {
                EXHashCodeFile::Debug_HashCodeName(entity);
            }
        }
        m_LocationType = 0x8000;
        switch (p->m_Id) {
        case 0x49000016:  // HT_Weaponry_Drone_1930_SniperRifle
        case 0x4900002c:  // HT_Weaponry_Drone_SniperRifle
        case 0x49000030:  // HT_Weaponry_Drone_MS_SniperRifle
        case 0x4900003d:  // HT_Weaponry_Sniper_Pred_Beam
        case 0x4900003e:  // HT_Weaponry_Sniper_Pred_Bullets
        case 0x4900003f:  // HT_Weaponry_Sniper_Pred_Bolt
        case 0x49000041:  // HT_Weaponry_HunterSmartDisc
            m_LocationType = 0x4000;
            break;
        }
    } else {
        EXHashCodeFile::Debug_HashCodeName(entity);
    }
}

// Where a shot from pos along dir stops: the weapon's range, cut short by the first contoured item
// and then by the map (layer mask 8). Returns whether anything was hit.
signed char ActorCarryableGun::GetEndPoint(WeaponProfile *p, const EXVector &pos, const EXVector &dir, EXVector &out)
{
    EXVector start(pos.x, pos.y, pos.z, 0.0f);
    EXVector d = dir;
    float sq = d.y * d.y + d.z * d.z + d.x * d.x;
    if (sq >= 0.0f) {
        float len = (float)sqrt(sq);
        if (len > EX_EPSILON_UNIT) d *= 1.0f / len;
    }
    out = d * p->m_Range + start;
    out.w = 0.0f;
    EXRayCastTemp ray;
    ray.Set(start, out - start, 0, 0, 0, 0);
    Actor_CalledRayCast++;
    theItemEnv.DoRayCastToContoured(ray);
    signed char bHit = ray.m_bHit != 0;
    if (bHit) out = ray.Intersect();
    ray.Set(start, out - start, 0, 0, 0, 0);
    Actor_CalledRayCast++;
    ray.m_mask = 8;
    theItemEnv.DoRayCastToMap(ray, 0);
    if (ray.m_bHit) out = ray.Intersect();
    return ray.m_bHit || bHit;
}

// Weapon and prop sets: picks one entity at random from the set (other hashes are returned as is).
// The 1930 sets exist under two hashes (HT_Entity_SET_* and HT_Entity_Weapon_SET_*).
int TranslateCarryHashCode(int hash)
{
    switch (hash) {
    case 0x2000163: {  // HT_Entity_Weapon_SET_Copycat_Guns
        int a[2] = {0x20000d4, 0x20000be};  // Magnum, M16
        return a[EXRand32() & 1];
    }
    case 0x200013b:  // HT_Entity_SET_1930_Melee
    case 0x200015d: {  // HT_Entity_Weapon_SET_1930_Melee
        int a[5] = {0x20000e7, 0x20000e8, 0x20000cb, 0x20000bf, 0x20000cf};  // BaseballBat, BillyClub, BrokenBottle, MeatCleaver, FireAxe
        return a[EXRand32() % 5];
    }
    case 0x200013c:  // HT_Entity_SET_1930_Guns
    case 0x200015e: {
        int a[4] = {0x20000e5, 0x20000e2, 0x20000e3, 0x20000e4};  // 1930 AutoRifle, Revolver, Shotgun, TommyGun
        return a[EXRand32() & 3];
    }
    case 0x200013d:  // HT_Entity_SET_1930_Cops_Melee
    case 0x200015f: {
        int a[2] = {0x20000e8, 0x20000cf};  // BillyClub, FireAxe
        return a[EXRand32() & 1];
    }
    case 0x200013e:  // HT_Entity_SET_1930_Cops_Guns
    case 0x2000160: {
        int a[2] = {0x20000e3, 0x20000e2};  // 1930 Shotgun, Revolver
        return a[EXRand32() & 1];
    }
    case 0x200013f:  // HT_Entity_SET_1930_Gang_Melee
    case 0x2000161: {
        int a[3] = {0x20000cb, 0x20000bf, 0x20000e7};  // BrokenBottle, MeatCleaver, BaseballBat
        return a[EXRand32() % 3];
    }
    case 0x2000140:  // HT_Entity_SET_1930_Gang_Guns
    case 0x2000162: {
        int a[2] = {0x20000e5, 0x20000e4};  // 1930 AutoRifle, TommyGun
        return a[EXRand32() & 1];
    }
    case 0x2000273: {  // civilian props: nothing (6 in 19), or flowers, food, a drink or shopping
        int a[19] = {0, 0, 0, 0, 0, 0,
                     0x20000b4, 0x20000b5, 0x20000b5, 0x20000b6, 0x20000b6, 0x20000bb, 0x20000bb,
                     0x20000bc, 0x20000bd, 0x20000c4, 0x20000c4, 0x20000c4, 0x20000c4};
        return a[EXRand32() % 19];
    }
    case 0x2000278: {
        int a[6] = {0x2000236, 0x2000236, 0x2000237, 0x2000237, 0x2000237, 0x20000e1};  // ..., ShockRifle
        return a[EXRand32() % 6];
    }
    case 0x200018f: {  // HT_Entity_Weapon_SET_Mafia_Guns
        int a[3] = {0x20000d6, 0x20000d5, 0x20000d4};  // AutoShotgun, SubMachineGun, Magnum
        return a[EXRand32() % 3];
    }
    case 0x2000190: {  // HT_Entity_Weapon_SET_Juv_Guns
        int a[2] = {0x20000be, 0x20000d3};  // M16, AutoPistol
        return a[EXRand32() & 1];
    }
    case 0x20001d8: {  // HT_Entity_Weapon_SET_BlackOps_Guns
        int a[2] = {0x20000e1, 0x20000cc};  // ShockRifle, ShockStick
        return a[EXRand32() & 1];
    }
    case 0x200027f: {
        int a[3] = {0x20000e1, 0x2000237, 0x2000237};  // ShockRifle, ...
        return a[EXRand32() % 3];
    }
    case 0x200018a: {  // HT_Entity_Weapon_SET_Future_Cops_Guns
        int a[3] = {0x20000be, 0x20000d6, 0x20000d3};  // M16, AutoShotgun, AutoPistol
        return a[EXRand32() % 3];
    }
    }
    return hash;
}

// The anti-cloak cloud: while its script runs, breaks the player's cloak when he stands in it (a 1.5
// tall capsule at his feet against a 2 tall, 2.5 radius one at the cloud) and decloaks every actor
// within 2.5. Deletes itself when the script ends.
s8 XItemHandler_AntiCloakSmoke::DoUpdate()
{
    if (!m_pScript) return 0;
    if (m_pScript->m_11a) {
        m_pItem->ItemFlags() = 0x10;
        return 0;
    }
    if (XItemHandler_Player::GetInstance()->GetCloak()) {
        Player_GetFeetPos();
        EXCapsule player;
        player.m_seg.m_Origin = Player_GetFeetPos();
        player.m_seg.m_Direction.x = 0.0f;
        player.m_seg.m_Direction.y = 1.5f;
        player.m_seg.m_Direction.z = 0.0f;
        player.m_radius = 0.75f;
        EXCapsule cloud;
        cloud.m_seg.m_Origin = m_pItem->Position();
        cloud.m_seg.m_Direction.x = 0.0f;
        cloud.m_seg.m_Direction.y = 2.0f;
        cloud.m_seg.m_Direction.z = 0.0f;
        cloud.m_radius = 2.5f;
        if (EXIntersection(player, cloud)) XItemHandler_Player::GetInstance()->BreakCloak(0);
    }
    EXVector pos = m_pItem->Position();
    for (Actor *p = Actor::GetFirst(pos, 2.5f, 0); p; p = p->GetNext(pos, 2.5f, 0)) p->BreakCloak(0);
    return 1;
}

// Starts the weapon's muzzle flash script on the owner, placed at the muzzle datum.
void ActorCarryableGun::PlayMuzzleScript(int hash)
{
    if (!hash) return;
    Actor *pOwner = Actor::GetPtrFromID(m_Owner);
    if (!pOwner || !m_pAnimator) return;
    EXDatumTemp d;
    memset(&d, 0, sizeof(d));
    if (!m_pAnimator->GetDatum(0x10000037, d, 0, EXItemAnimator::m_pNullDatumPtr)) {  // HT_AnimDatum_Cannon_Barrel_Muzzle
        EXHashCodeFile::Debug_HashCodeName(m_EntityHash);
        EXHashCodeFile::Debug_HashCodeName(0x10000037);
        return;
    }
    if (m_pMuzzleScript) {
        m_pMuzzleScript->ItemFlags().BitSet(0x10);
        m_pMuzzleScript = 0;
    }
    m_pMuzzleScript = EXItemAnimator_Script::NewItem();
    if (!m_pMuzzleScript) return;
    EXGeoFile *pFile = EXGeoFile::GetGeoFile(0x1000016);  // HT_File_Pred_FX
    if (!pFile) return;
    if (!m_pMuzzleScript->Initialise(pFile, hash)) {
        m_pMuzzleScript->ItemFlags().BitSet(0x10);
        m_pMuzzleScript = 0;
        return;
    }
    m_pMuzzleScript->SetPosition(EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    m_pMuzzleScript->SetRotation(EXQuaternion(0.0f, 0.0f, 0.0f, 0.0f));
    m_pMuzzleScript->RenderFlags().BitSet(1);
    pOwner->GetItem()->AddAnimator(m_pMuzzleScript);
}

// Plays an impact script where a shot hit scenery, if that point is on screen. A previous impact
// still in its first quarter second is left alone (and the new one skipped).
void ActorCarryableGun::PlaySceneryImpactScript(int hash, const EXVector &pos)
{
    EXSphere sph;
    sph.Set(pos, 0.01f);
    if (!XGetCamera(0)->BoundingVolumeOnScreen(sph)) return;
    if (m_pImpactScript && m_pImpactScript->ObjectTime() < 0.25f) return;
    if (m_pImpactScript) {
        m_pImpactScript->ItemFlags().BitSet(0x10);
        m_pImpactScript = 0;
    }
    if (m_pImpactScript2) {
        m_pImpactScript2->ItemFlags().BitSet(0x10);
        m_pImpactScript2 = 0;
    }
    m_pImpactScript2 = XItem::CreateItem(0, 0, 0, EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    if (!m_pImpactScript2) return;
    m_pImpactScript = new EXItemAnimator_Script;
    if (m_pImpactScript) {
        m_pImpactScript2->AddAnimator(m_pImpactScript);
        EXGeoFile *pFile = EXGeoFile::GetGeoFile(0x1000016);  // HT_File_Pred_FX
        if (!pFile) return;
        if (m_pImpactScript->Initialise(pFile, hash)) {
            m_pImpactScript->SetPosition(pos);
            m_pImpactScript->SetRotation(EXQuaternion(0.0f, 0.0f, 0.0f, 0.0f));
            m_pImpactScript->RenderFlags().BitSet(1);
            theItemEnv.AddItem(m_pImpactScript2, 0);
            return;
        }
    }
    DeleteSceneryImpactScript();
}

// Keeps the muzzle script on the muzzle datum and facing with the gun; drops it once it has ended.
void ActorCarryableGun::UpdateMuzzleScript()
{
    Actor *pOwner = Actor::GetPtrFromID(m_Owner);
    if (!pOwner || !m_pMuzzleScript || !m_pAnimator) return;
    if (m_pMuzzleScript->m_11a) {
        m_pMuzzleScript->ItemFlags().BitSet(0x10);
        m_pMuzzleScript = 0;
        return;
    }
    EXDatumTemp d;
    memset(&d, 0, sizeof(d));
    if (m_pAnimator->GetDatum(0x10000037, d, 0, EXItemAnimator::m_pNullDatumPtr)) {  // HT_AnimDatum_Cannon_Barrel_Muzzle
        m_pMuzzleScript->SetPosition(d.m_position);
        const EXVector &r = m_pAnimator->Rotation();
        m_pMuzzleScript->SetRotation(EXQuaternion(r.x, r.y, r.z, r.w));
    }
}

// Fires one round at target. The barrel direction gets a random spread from the weapon's accuracy.
// Instant weapons (fire type 0, limited per frame) fire a tracer, or for freeze throwers an instant
// round; beams (type 1) hit instantly and queue a beam; anything else launches a projectile (grenade
// launchers lob theirs, two-stage rounds go up first and are given the target). With bEffects the
// muzzle script and fire sound play, and every 180 frames the shot is a loud noise.
signed char ActorCarryableGun::Fire(Actor &actor, const EXVector &target, signed char bEffects, int y)
{
    if (m_WeaponIndex == -1) return 1;
    WeaponProfile *p = WeaponProfile::GetWeaponDataFromIndex(m_WeaponIndex);
    EXVector muzzle, dir;
    GetMuzzleDatum(muzzle, dir);
    dir = actor.GetGunDirection(target);
    float spread = (100.0f - p->m_Accuracy) * 0.0015f;
    if (p->m_Id == 0x4900002e) spread *= 5.0f;  // HT_Weaponry_Drone_MS_MultiLauncher
    EXVector a;
    a.x = EXRandf(-spread, spread);
    a.y = EXRandf(-spread, spread);
    a.z = EXRandf(-spread, spread);
    EXMatrix m;
    Carry_SetRotationXYZ(m, a);
    EXVector t;
    D3DXVec4Transform(&t, &dir, &m);
    dir = t;
    EXMatrix look;
    look.r[0].x = 1.0f; look.r[0].y = 0.0f; look.r[0].z = 0.0f; look.r[0].w = 0.0f;
    look.r[1].x = 0.0f; look.r[1].y = 1.0f; look.r[1].z = 0.0f; look.r[1].w = 0.0f;
    look.r[2].x = dir.x; look.r[2].y = dir.y; look.r[2].z = dir.z; look.r[2].w = 0.0f;
    look.r[3].x = 0.0f; look.r[3].y = 0.0f; look.r[3].z = 0.0f; look.r[3].w = 1.0f;
    EXVector rot;
    look.GetRotation(rot, 2);
    if (p->m_FireType != 0) {
        if (p->m_FireType != 1) {
            EXVector *pDir;
            EXVector d;
            if (p->m_Id == 0x49000024) {  // HT_Weaponry_Drone_GrenadeLauncher
                d = actor.GetGunDirection(target);
                float yaw = GetRotation(d);
                float speed = p->m_ThrowSpeed;
                EXVector v = d;
                float h = v.x * v.x + v.z * v.z;
                float dist = h < 0.0f ? 0.0f : (float)sqrt(h);
                float pitch = CalculateAngle(speed, dist, v.y, 0.01f, 1);
                if (pitch > 90.0f) pitch = 0.7853982f;
                float s = (100.0f - p->m_Accuracy) * 0.0015f;
                pitch = EXRandf(-s, s) + pitch;
                float c = (float)cos(pitch);
                d.x = (float)sin(yaw) * c;
                d.z = (float)cos(yaw) * c;
                d.y = (float)sin(pitch);
                d.w = 0.0f;
                pDir = &d;
            } else if (p->m_AimType == 0x52000008) {  // HT_WeaponClass_TwoStage
                dir = EXVector(0.0f, 1.0f, 0.0f, 0.0f);
                rot = target;
                pDir = &dir;
            } else {
                XItemHandler_Bullet::CreateFromHash(muzzle, rot, dir, p->m_Id, 0, actor.m_RefID, y, 1.0f);
                goto effects;
            }
            XItemHandler_Bullet::CreateFromHash(muzzle, rot, *pDir, p->m_Id, 0, actor.m_RefID, y, 1.0f);
            goto effects;
        }
    } else {
        if (ms_iInstantBulletsThisFrame < 1) return 0;
        ms_iInstantBulletsThisFrame--;
    }
    if (p->m_FireType == 1) {
        EXVector hit;
        signed char bScenery;
        signed char bActor = XItemHandler_Bullet::InstantCreate(p, muzzle, rot, dir, actor.m_RefID, 0, p->GetBulletDamage(), p->m_Range, &hit, &bScenery);
        AddBeam(this, muzzle, hit, ex::RGBA((u8)p->m_TracerR, (u8)p->m_TracerG, (u8)p->m_TracerB, (u8)p->m_TracerA), p->m_TracerWidth);
        if (!bEffects) return 1;
        if (bScenery && !bActor) {
            if (p->m_ExplosionScript) PlaySceneryImpactScript(p->m_ExplosionScript, hit);
            if (p->m_ImpactSound) m_SoundTag.StartSound(p->m_ImpactSound, hit, -1, 0);
        }
    } else if (WeaponIsTheMuzzleFlash(p->m_Id)) {
        EXVector hit;
        signed char bScenery;
        signed char bActor = XItemHandler_Bullet::InstantCreate(p, muzzle, rot, dir, actor.m_RefID, 0, p->GetBulletDamage(), p->m_Range, &hit, &bScenery);
        if (!bEffects) return 1;
        if (bScenery && !bActor) {
            if (p->m_ExplosionScript) PlaySceneryImpactScript(p->m_ExplosionScript, hit);
            if (p->m_ImpactSound) m_SoundTag.StartSound(p->m_ImpactSound, hit, -1, 0);
        }
    } else {
        CreateTracer(muzzle, dir);
    }
effects:
    if (!bEffects) return 1;
    SoundTag *pTag;
    if (p->m_FireType == 1) {
        pTag = &m_BeamSoundTag;
    } else {
        if (p->m_MuzzleScript) {
            if (WeaponIsTheMuzzleFlash(p->m_Id)) {
                if (!m_bMuzzleLooping) {
                    PlayMuzzleScript(p->m_MuzzleScript);
                    m_bMuzzleLooping = 1;
                }
            } else {
                PlayMuzzleScript(p->m_MuzzleScript);
            }
        }
        pTag = &m_SoundTag;
    }
    if (!pTag->IsPlayingLooping(p->m_SfxFire)) pTag->StartSound(p->m_SfxFire, GetWorldPos(), -1, 0);
    static unsigned int s_LastNoiseFrame;
    if (g_Stats.FrameCount() > s_LastNoiseFrame + 180) {
        SendMsg_LoudNoise(muzzle, p->m_NoiseRadius, &actor, 1, 0);
        s_LastNoiseFrame = g_Stats.FrameCount();
    }
    actor.m_FiredBulletTime = theApp.GlobalTimer();
    return 1;
}

// Per-frame firing state machine (see m_Mode). Beams (fire type 1) aim 1000 units along the gun
// from the right hand: spin up (0), wait for the spin-up script (1), then fire every frame for the
// weapon's duration (2). Others fire bursts: start (3), fire the burst's rounds one per m_Rate frames
// holding the owner in place (5), and pause between bursts with a random hold while unable to
// fire (6), all within the duration. 7 is paused. Returns 0 when firing has ended.
signed char ActorCarryableGun::UpdateFiring(Actor &actor, const EXVector &target, int bAnim, int y)
{
    if (m_WeaponIndex == -1) return 0;
    WeaponProfile *p = WeaponProfile::GetWeaponDataFromIndex(m_WeaponIndex);
    EXVector aim = target;
    if (p->m_FireType == 1) aim = actor.GetGunDirection(target) * 1000.0f + actor.GetHandPos(0);
    switch (m_Mode) {
    case 0:
        PlayMuzzleScript(p->m_MuzzleScript);
        m_Mode = 1;
        if (bAnim) actor.SetFiringAnim(0);
        if (p->m_SfxFireStart) m_SoundTag.StartSound(p->m_SfxFireStart, GetWorldPos(), -1, 0);
        return 1;
    case 1:
        if (!m_pMuzzleScript) {
            m_RoundTime = p->GetDuration();
            m_Mode = 2;
            m_bFirstRound = 1;
        }
        return 1;
    case 2:
        m_RoundTime -= XApp::GameFrameRateMul();
        if (m_RoundTime > 0.0f) {
            if (bAnim) actor.SetFiringAnim(0);
            Fire(actor, aim, m_bFirstRound, 0);
            m_bFirstRound = 0;
            return 1;
        }
        StopFiring();
        if (p->m_SfxFireEnd) m_BeamSoundTag.StartSound(p->m_SfxFireEnd, GetWorldPos(), -1, 0);
        break;
    case 7:
        m_PauseTime -= XApp::GameFrameRateMul();
        if (m_PauseTime <= 0.0f) {
            m_Mode = m_PrevMode;
            if (m_PrevMode == 2) m_Mode = 0;
        }
        return 1;
    case 3:
        m_Wait = 0.0f;
        m_Unkcc = 8.0f;
        m_Mode = 6;
        m_RoundTime = p->GetDuration();
        m_bMuzzleLooping = 0;
        m_FirePos = actor.GetItem()->Position();
        return 1;
    case 5:
        if (bAnim) {
            actor.SetFiringAnim(1);
            EXVector d = m_FirePos - actor.GetItem()->Position();
            if (0.0100000007f > d.y * d.y + d.z * d.z + d.x * d.x)
                actor.OrgLoc().MoveTo(m_FirePos, 0);
            else
                m_FirePos = actor.GetItem()->Position();
        }
        if (Fire(actor, aim, m_BurstLeft == p->m_Burst, y)) {
            m_BurstLeft--;
            m_Clip--;
            if (m_BurstLeft <= 0) {
                m_Mode = 6;
                m_Unkcc = (float)p->m_Rate;
            }
        }
        if (WeaponIsTheMuzzleFlash(p->m_Id) && !m_pMuzzleScript) {
            m_RoundTime = 0.0f;
            m_Mode = 6;
            m_Unkcc = (float)p->m_Rate;
        }
        return 1;
    case 6:
        m_RoundTime -= EXBaseApp::GameFrameRateMul();
        if ((float)m_Clip > 0.0f && m_RoundTime > 0.0f) {
            m_Unkcc -= EXBaseApp::GameFrameRateMul();
            if (m_Unkcc <= 0.0f) {
                if (CanFire()) {
                    m_Mode = 5;
                    m_BurstLeft = (s16)p->m_Burst;
                    if (m_BurstLeft <= 0) m_BurstLeft = 1;
                    p->LockWeapon();
                    return 1;
                }
                m_Unkcc = (float)EXBaseApp::NominalGameFrameRate() * (EXRandf() * 0.3f + 0.2f);
            }
            return 1;
        }
        StopFiring();
        if (p->m_SfxFireEnd) m_SoundTag.StartSound(p->m_SfxFireEnd, GetWorldPos(), -1, 0);
        break;
    default:
        return 0;
    }
    ChangeMode();
    return 0;
}
