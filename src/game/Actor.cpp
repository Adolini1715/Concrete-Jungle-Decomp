#include "Actor.h"
#include "ActorCarryable.h"
#include "ActorOptions.h"
#include "ActorEffect.h"
#include "XItem.h"
#include "EXItemAnimator_Anim.h"
#include "EXItemCtrl.h"
#include "EXGeoFile.h"
#include "EXMemory.h"
#include "EXRand.h"
#include "EXTrace.h"
#include "XApp.h"
#include "FEText.h"
#include "EXDatum.h"
#include "ActorLocation.h"
#include "ActorSpecial.h"
#include "GeoSectionManager.h"
#include "EXItemAnimator_Script.h"
#include "EXItemRender_Anim.h"
#include "EX6DObject.h"
#include "XItemHandler_Camera.h"
#include "EXIntersection.h"
#include "EXCapsule.h"
#include "EXQMath.h"
#include "XGameWnd.h"
#include "XNavNetwork.h"
#include "EXHashCodeFile.h"
#include "ActorUtil.h"
#include "EXBaseGamePad.h"
#include "FX_Gore.h"
#include "XGamePad.h"
#include "ScriptRunner.h"
#include "XItemHandler_ScriptBase.h"
#include "AIControl.h"
#include "EXItemAnimator_AnimModifier.h"
#include "EXItemAnimator_Entity.h"
#include "GameData.h"
#include "FELevels.h"
#include "EXSphere.h"
#include "Vehicle.h"
#include "ActorTimeTable.h"
#include "EXRayCastTemp.h"
#include "EXItemCtrl_SnapShot.h"
#include "EXSoundManager.h"
#include "XItemHandler_Player.h"
#include "FEHud.h"
#include "FEObjectScript.h"
#include "FEMathmos.h"
#include "FEObject.h"
#include "EXTriangle.h"
#include "EXDistance.h"
#include "ScanInfo.h"
#include "XTrace.h"
#include "ActorTextureSwap.h"
#include "WeaponTable.h"
#include "SubGroupAnims.h"
#include "XItemHandler_Environment.h"
#include "EXGeoTrigger.h"
#include "ActorCombat.h"
#include "ActorProfile.h"
#include "Avoid.h"
#include "EXExtRenderInfoStandIn.h"
#include "EXQuaternion.h"
#include "EXMatrix.h"
#include "XNavRoute.h"
#include "XNavMeshHandler.h"
#include "XNavMesh.h"

// TODO stand-ins: other areas.
class EXSoundHandle {   // TODO stand-in (EXSoundManager.obj)
public:
    unsigned int HashCode() const { return m_Hash; }
    void SetTrackingSound(signed char b) { m_bTracking = b; }
    void SetLoopingSound(signed char b) { m_bLooping = b; }
    u32 m_Unk0;
    EXSoundHandle *m_pNext;         // 0x04
    u8 m_Unk8[0x1c - 8];
    unsigned int m_Hash;            // 0x1c
    u8 m_Unk20[0x2a - 0x20];
    s8 m_bLooping;                  // 0x2a
    u8 m_Unk2b;
    s8 m_bTracking;                 // 0x2c
};
signed char STATE_SniperStand(StateMachine &sm);
class XItemHandler_Objective : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_Objective)
};
void Objective_NotifyActorJustKilled(Actor *pActor);
void Objective_NotifyPlayerSeen(Actor *pActor);  // TODO stand-in (Objective.obj)
signed char STATE_LookAroundAlerted(StateMachine &);
signed char STATE_AttackPred(StateMachine &);
extern unsigned int gCurrentMap_GeoHash;
extern XItemList gHitableList;
struct GEOSCRIPT_EVENT {  // TODO stand-in (layout as in Player.cpp)
    u16 m_00[3];
    u16 m_Frames;           // 0x06
    u32 m_08;
    unsigned int m_Hash;    // 0x0c HT_ScriptEvents_*, -1 at the end of the script
    u32 m_10;
    union {
        int i;
        unsigned int u;
        float f;
    } m_Param[4];           // 0x14
};
extern XGameWnd *gpGameWnd[4];  // TODO stand-in (GameWnd.obj)
inline XGameWnd *XGetGameWnd() { return gpGameWnd[0]; }

// ---- data ----
// Defined first so its dynamic initialiser gets retail's number (_$E8).
QueuedMsgItem Actor::m_MsgQueue[128];
signed char DBG_bActorsAndVehicles = 1;
float Actor::ms_fActorAlpha = 1.0f;
// Per-limb data, indexed by ActorLostLimb (sizeof 0x10).
struct ActorLimbInfo {
    unsigned int m_Datum;       // 0x00 datum the hit test measures from
    unsigned int m_Bone;        // 0x04
    unsigned int m_Bone2;       // 0x08 (belt only)
    unsigned int m_TexSwap;     // 0x0c HT_Texture_Swap_*
};
static ActorLimbInfo s_LimbInfo[4] = {
    { 0x1000000a, 0xe000010, 0, 0x6000047 },
    { 0x10000009, 0xe000014, 0, 0x6000048 },
    { 0x1000000d, 0xe000009, 0, 0x6000049 },
    { 0x1000001f, 0xe000003, 0xe000001, 0x6000046 },
};
signed char g_bDumpActorInfo;
signed char gDrawActorState;
signed char gShowLocations;
signed char gShowTimeTable;
signed char gSkinTest;
signed char gShowAsyncLoad;
int Actor_CalledCMD;
int Actor_CalledGetDatum;
int Actor_CalledRayCast;
float gActorFadeDist;
signed char gActorPause;
signed char gActorHide;
int Actor_CalledDEBUG;
float gTimer_OverridePlayerCloak;
float Actor::ms_fMaxCollisionRadius;
int Actor::m_Count;
unsigned int Actor::m_gAffiliation;
int Actor::m_MsgQueueCount;
XItem *Actor::ms_pLimbInPlayersHand;
int Actor::ms_iActorIDPlayerHasHead;
// Root motion per anim mode (index hash - 0x9000000), filled as anims are seen; cleared by BootInit.
struct ActorAnimVelocity {
    EXVector3 m_Vel;    // 0x00
    s8 m_bValid;        // 0x0c
};
static ActorAnimVelocity s_AnimVelocity[0x54b];
int Actor::gRagDollCount;
float Actor::gAlphaPulse;
static int s_NextID = 10;
signed char g_bInitSniperTarget = 1;
signed char ActorOptionsManager::m_bPlayerVisible = 1;
int ActorOptionsManager::m_Count;
signed char ActorOptionsManager::m_bMimicryUsed;
ActorOptions *ActorOptionsManager::m_pData[32];
ActorOptions ActorOptionsManager::m_Defaults;
static unsigned int s_NonCriticalFrame;
// GUESS: name. Unnamed Actor.obj data (0.05f, not a __real) read by the distance fade, the
// alpha fade, UpdateCarry, UpdateShadow and the scan health text.
static const float kActorSmallStep = 0.05f;
// Body part datums tested by CheckCollision, in order (Actor.obj rdata after classActor).
static const int s_CollisionDatums[] = {
    0x10000049, // HT_AnimDatum_Actor_Body
    0x1000004a, // HT_AnimDatum_Actor_Head
    0x10000041, // HT_AnimDatum_Actor_UpperLeftArm
    0x10000044, // HT_AnimDatum_Actor_UpperRightArm
    0x10000045, // HT_AnimDatum_Actor_UpperLeftLeg
    0x10000046, // HT_AnimDatum_Actor_UpperRightLeg
    0x10000048, // HT_AnimDatum_Actor_LowerLeftLeg
    0x10000047, // HT_AnimDatum_Actor_LowerRightLeg
    0x10000042, // HT_AnimDatum_Actor_LowerLeftArm
    0x10000043, // HT_AnimDatum_Actor_LowerRightArm
    0,
};

EXRuntimeClass *Actor::_GetBaseClass() { return EXRUNTIME_CLASS(EXItemHandler); }
const EXRuntimeClass Actor::classActor = { EXRUNTIME_CLASS(EXItemHandler), "Actor", sizeof(Actor), (void *(__cdecl *)())Actor::CreateObject };

Actor *Actor::GetPtrFromID(int id)
{
    if (!id) return 0;
    Actor *p = GetActorBasePtr(id)->m_pActor;
    if (!p) return 0;
    return p->m_RefID != id ? 0 : p;
}

static int Actor_GetNewID()
{
    int id = s_NextID;
    for (int n = 0; n < 128; n++) {
        id++;
        if (!GetActorBasePtr(id)->m_pActor) {
            s_NextID = id;
            return id;
        }
    }
    s_NextID = id;
    return 0;
}

static int Actor_GetNextNonCriticalUpdateFrame()
{
    unsigned int f = s_NonCriticalFrame + 1;
    s_NonCriticalFrame = f;
    if (!(f % (EXBaseApp::NominalGameFrameRate() >> 5))) {
        s_NonCriticalFrame = 0;
        return 0;
    }
    return f;
}

static int Actor_ReachedNonCriticalUpdateFrame(unsigned int frame)
{
    return XGetGameWnd()->FrameCounter() % (XGetNominalFrameRate() >> 5) == frame;
}

void Actor::MsgQueueInit()
{
    m_MsgQueueCount = 0;
    memset(m_MsgQueue, 0, sizeof(m_MsgQueue));
}

void Actor::SetPositionEx(const EXVector &pos) { m_pItem->SetPosition(pos); }

signed char Actor::IsMsgOnStack(MsgType msg) const { return IsBitMsgOnStack(GetMsgBitsForMsg(msg)); }

void Actor::ClearBitMsgOnStack(unsigned int bits)
{
    for (int i = 0; i < m_StackCount; i++) m_Stack[i].m_Flags &= ~bits | 0xc0000000;
}

signed char Actor::SearchBrainStateStack(StateFn state) const
{
    if (m_SM.m_State == state) return 1;
    for (int i = 0; i < m_StackCount; i++)
        if (m_Stack[i].m_State == state) return 1;
    return 0;
}

signed char Actor::IsBitMsgOnStack(unsigned int bits) const
{
    if (!bits) return 0;
    for (int i = 0; i < m_StackCount; i++)
        if (m_Stack[i].m_Flags & bits & 0x3fffffff) return 1;
    return 0;
}

void Actor::BootInit()
{
    memset(g_aActorList, 0, sizeof(ActorBase) * 128);
    memset(s_AnimVelocity, 0, sizeof(s_AnimVelocity));
}

void Actor::InvalidateGotoPosOnly() { m_StateFlags.m_bMoving15 = 0; }

void Actor::InvalidateGotoPos()
{
    m_StateFlags.m_bMoving14 = 0;
    m_StateFlags.m_bMoving15 = 0;
    m_StateFlags.m_bMoving27 = 0;
}

void Actor::InvalidateGotoPosButNotTargetYRot()
{
    m_StateFlags.m_bMoving14 = 0;
    m_StateFlags.m_bMoving15 = 0;
}

signed char Actor::CanChangeSkinEver() const
{
    SubGroupAnims &s = GetSubGroupAnims();
    return s.m_Anims[1] > 0 && s.m_Anims[2];
}

signed char Actor::IsRagDollAllowed()
{
    if (gRagDollCount >= 3) return 0;
    if (m_ActorFlags.m_bBoss || m_ActorFlags.m_bUlysses) return 0;
    return m_StateFlags.m_bOnScreen == 1;
}

signed char Actor::InitTriggerJump()
{
    if (m_NavRoute.m_TriggerJump && !m_NavRoute.m_Flags.m_bTriggerJump && !m_NavRoute.m_Flags.m_b7)
        m_NavRoute.m_Flags.m_bTriggerJump = 1;
    return 1;
}

signed char Actor::AtYRot(float rot, float tol)
{
    float d = (float)fabs(m_TargetRotYDelta - GetRotation().y);
    if (d > 3.1415927f) d = 6.2831855f - d;
    return tol >= d;
}

Actor *Actor::GetFirst(signed char bInWorld)
{
    ActorBase *p = g_aActorList;
    for (int i = 0; i < 128; i++, p++) {
        if (p->m_pActor) {
            if (!bInWorld) return p->m_pActor;
            if (p->m_bInWorld) return p->m_pActor;
        }
    }
    return 0;
}

Actor *Actor::GetNext(signed char bInWorld)
{
    int i = (m_RefID & 0x7f) + 1;
    ActorBase *p = &g_aActorList[i];
    for (; i < 128; i++, p++) {
        if (p->m_pActor) {
            if (!bInWorld) return p->m_pActor;
            if (p->m_bInWorld) return p->m_pActor;
        }
    }
    return 0;
}

void Actor::SetRotY(float rot)
{
    m_TargetRotY = rot;
    m_StateFlags.m_bMoving27 = 0;
    m_StateFlags.m_bMoving14 = 1;
}

void Actor::NoAnimBlend()
{
    if (m_pAnim) m_pAnim->DeleteStack(0);
}

float Actor::GetAnimFrameCount()
{
    EXItemCtrl_Anim *pCtrl = m_pAnim ? (EXItemCtrl_Anim *)m_pAnim->m_stacks[0].FirstCtrl() : 0;
    if (!pCtrl) return 1.0f;
    float count = 0.0f;
    float frame = 0.0f;
    pCtrl->CalcFrameInfo(count, frame);
    return count;
}

void Actor::SetAnimFrame(float f)
{
    if (m_pAnim) m_pAnim->SetObjectFrame(GetAnimFrameCount() * f);
}

void Actor::CommitAnimSpeedMult()
{
    if (m_pAnim) m_pAnim->m_frameRateMult = AnimSpeedMult() * XApp::GlobalGameFrameRate();
}

void Actor::DeleteBloodScript(XItem *pItem, EXItemAnimator_Script *pScript)
{
    if (!pScript) return;
    if (pItem) pItem->DeleteAnimator((EXItemAnimator *)pScript);
    else m_pItem->DeleteAnimator((EXItemAnimator *)pScript);
}

signed char Actor::HasLostLimb() const
{
    for (int i = 0; i < 4; i++)
        if (m_LostLimbs[i].m_Id) return 1;
    return 0;
}

signed char Actor::HasLostLimb(int limb) const { return m_LostLimbs[limb].m_Id != 0; }

signed char Actor::Initialise() { return 1; }

void Actor::SetHeadTrack(const EXVector &v)
{
    m_HeadTrackPos = v;
    m_StateFlags.m_bHeadTrack = 1;
}


void Actor::AwardDeathObjective()
{
    if (m_ActorFlags.m_bDeathObjective) return;
    m_ActorFlags.m_bDeathObjective = 1;
    Objective_NotifyActorJustKilled(this);
}

signed char Actor::IsTargetable() const
{
    if (GetProfile()->m_SubGroup != 0x4f000017 && m_StateFlags.m_bTargetable && !m_StateFlags.m_bHide) return 1;
    return 0;
}

void Actor::SetDefaultContextIcon(int icon, signed char bForce)
{
    if (!bForce && m_DefaultContextIcon > 0x4000000 && m_DefaultContextIcon < 0x40007d9) return;
    m_DefaultContextIcon = icon;
}

void Actor::SetIsInCombatWithPlayer(signed char b)
{
    if (b) {
        m_ActorFlags.m_bInCombatWithPlayer = 1;
        gAIControl.m_ActorsInCombatCount++;
    } else {
        m_ActorFlags.m_bInCombatWithPlayer = 0;
    }
}

void Actor::OverridePlayerCloak() { gTimer_OverridePlayerCloak = EXBaseApp::NominalGameFrameRate() * 0.5f; }

void Actor::InstantAlarm()
{
    m_AlertRaiseTimer = 0.01f;
    m_AlertDecayTimer = 0.01f;
}

void Actor::CancelAlarmMeter()
{
    m_AlertRaiseTimer = -1.0f;
    m_AlertDecayTimer = -1.0f;
}

unsigned int Actor::DoScriptCmdRunControl(GEOSCRIPT_RUNCONTROL *pCmd, EXItemAnimator *pAnim)
{
    if (((u32 *)pCmd)[2]) return 5;
    m_ActorFlags2.m_bScriptRunControl = 1;
    return AS_GetPtrFromIndex(m_NewSkinIndex) ? 5 : 0;
}

signed char Actor::HasAnimEnded() { return m_ActorFlags2.m_bAnimEnded == 1; }

void Actor::SetAlpha(float a)
{
    if (a < 0.0f) a = 0.0f;
    else if (a > 1.0f) a = 1.0f;
    m_Alpha2 = a;
}

void Actor::__CarryFlush(int id)
{
    if (m_CarryLeft == id) m_CarryLeft = 0;
    if (m_CarryRight == id) m_CarryRight = 0;
}

ActorCarryable *Actor::GetCarryPtr(signed char bLeft)
{
    if (bLeft) return ActorCarryable::GetPtrFromID(m_CarryLeft);
    return ActorCarryable::GetPtrFromID(m_CarryRight);
}

ActorCarryableGun *Actor::GetGunPtr(signed char bLeft)
{
    ActorCarryable *p = GetCarryPtr(bLeft);
    if (!p || !p->m_bGun) return 0;
    return (ActorCarryableGun *)p;
}

void Actor::SpeakStop() { m_SpeechTag.RemoveAll(1); }

signed char Actor::IsSpeaking() { return m_SpeechTag.IsPlaying(); }

void Actor::StopSFX(unsigned int sfx) { m_SFXTag.Remove(sfx, 1); }

void Actor::StopLoopingSFX(unsigned int sfx) { m_SFXTag.KillSoundPlayingLooping(sfx); }

void Actor::StopAllSFX() { m_SFXTag.RemoveAll(1); }

unsigned int Actor::GetTotalStackFlags() const
{
    unsigned int f = 0;
    for (int i = 0; i < m_StackCount; i++) f |= m_Stack[i].m_Flags & 0x3fffffff;
    return f;
}

void Actor::StopAllLoopingSFX()
{
    m_SFXTag.KillSoundPlayingLooping();
    ActorCarryable *p = ActorCarryable::GetPtrFromID(m_CarryLeft);
    if (p) p->StopLoopingSFX();
    p = ActorCarryable::GetPtrFromID(m_CarryRight);
    if (p) p->StopLoopingSFX();
}

signed char Actor::IsSFXPlaying(unsigned int sfx) const
{
    for (EXSoundHandle *p = m_SFXTag.m_pHead; p; p = p->m_pNext)
        if (p->HashCode() == sfx) return 1;
    return 0;
}

void Actor::GrabRagdoll() {}

void Actor::LetGoRagdoll() {}

void Actor::UpdateRagdollGrabPoint() {}

void Actor::SetHideCarryRight(signed char b)
{
    ActorCarryable *p = ActorCarryable::GetPtrFromID(m_CarryRight);
    if (p) p->SetHide(b);
}

void Actor::SetHideCarryLeft(signed char b)
{
    ActorCarryable *p = ActorCarryable::GetPtrFromID(m_CarryLeft);
    if (p) p->SetHide(b);
}

XItemHandler_Objective *Actor::GetObjective()
{
    if (m_CreateID && (m_CreationFlags & 0x200) && ((EXItemHandler *)m_CreateID)->IsKindOf(EXRUNTIME_CLASS(XItemHandler_Objective)))
        return (XItemHandler_Objective *)m_CreateID;
    return 0;
}

ActorEffect *Actor::EffectGet(unsigned __int64 id)
{
    for (ActorEffect *p = ActorEffect::FromLink(m_EffectList.Head()); p; p = ActorEffect::FromLink(p->m_Link.Next()))
        if (p->m_ID == id) return p;
    return 0;
}

signed char Actor::EffectDeleteAll()
{
    ActorEffect *p = ActorEffect::FromLink(m_EffectList.Head());
    while (p) {
        ActorEffect *pNext = ActorEffect::FromLink(p->m_Link.Next());
        delete p;
        p = pNext;
    }
    return 1;
}

int Actor::GetHandDatumHash(signed char bLeft)
{
    if (!bLeft && GetSkin()->m_Hash == 0xd0000b9) return 0x10000039;
    return bLeft ? 0x10000013 : 0x10000014;
}

signed char Actor::IsCollisionPhysRadiusCheckValid()
{
    if (m_ActorFlags2.m_bInPlayersHand) return 0;
    if (m_StateFlags.m_bBeingStealthKilled) return 0;
    if (m_StateFlags.m_bRagDoll) return 0;
    if (m_StateFlags.m_bHitByCar == 1) return 0;
    return 1;
}

signed char Actor::CanLeapAbout() const
{
    if (!GetProfile()) return 0;
    int g = GetProfile()->m_SubGroup;
    if (g >= 0x4f00000c && (g <= 0x4f00000d || g == 0x4f000011)) return 1;
    return 0;
}

signed char Actor::DecideIsBoss() const { return GetSkin()->m_SkelHash == 0xd000091; }

signed char Actor::HasBossBar() const { return GetSubGroupAnims().m_Anims[15] > 0; }

signed char Actor::DecideIsUlysses() const
{
    if (GetProfile()) return GetProfile()->m_SubGroup == 0x4f000019;
    return 0;
}

signed char Actor::DecideIsAlien() const { return GetSkin()->m_SkelHash == 0xd000103; }

signed char Actor::DecideIsPredator() const { return GetSkin()->m_File == 0x10000c8; }

signed char Actor::IsDodgeAllowed() const
{
    int skel = GetSkin()->m_SkelHash;
    if (skel == 0xd000091) return 0;
    return skel != 0xd00008d;
}

signed char Actor::IsDiveAllowed() const
{
    if (SearchBrainStateStack(STATE_SniperStand)) return 0;
    if (m_Health > GetProfile()->m_Health * 0.5f) return 1;
    return 0;
}

signed char Actor::IsCarryingGun()
{
    if (m_ActorFlags.m_bRightArmMissing) return 0;
    WeaponProfile *p = GetWeaponProfile(0);
    if (!p) return 0;
    int type = ((int *)p)[4];
    if (type < 0x52000004 || type > 0x52000007) return 0;
    return 1;
}

signed char Actor::PetrolBombInProfile() const
{
    ActorProfile *p = GetProfile();
    if (!p) return 0;
    if (p->m_Weapon == 0x20000dc) return 1;
    if (m_ActorFlags.m_bHasThrownPetrolBomb && p->m_Weapon2 == 0x20000dc) return 1;
    return 0;
}

void Actor::SetFiringAnim(signed char b)
{
    m_ActorFlags2.m_bFiringAnim = 1;
    if (b && m_pAnim) m_pAnim->SetObjectFrame(GetAnimFrameCount() * 0.0f);
}

// NEAR: retail spills the random value to the stack before testing it.
void Actor::ResetCivilianAttackVO()
{
    const int &r = EXRand32() & 3;
    if (r) {
        m_CivilianAttackVOTimer = 0.0f;
        return;
    }
    m_CivilianAttackVOTimer = EXBaseApp::NominalGameFrameRate() * 4.0f;
}

signed char IsHuman(int type)
{
    switch (type) {
    case 0:
    case 1:
    case 2:
    case 8:
    case 9:
    case 10:
    case 11:
    case 13:
    case 14:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
        return 1;
    }
    return 0;
}

signed char Actor::CanCollideWithPlayer() const
{
    if (!m_StateFlags.m_bCanCollideWithPlayer) return 0;
    if (m_Unk8f4 <= 0.0f) return 0;
    if (m_StateFlags.m_bLyingOnGround) return 0;
    if (m_StateFlags.m_bRagDoll) return 0;
    if (m_ActorFlags.m_bHolographic == 1) return 0;
    return 1;
}

void Actor::ResetLastHitTimer()
{
    if (m_LastHitTimer > 0.0f) m_ConsecutiveHitCounter++;
    m_LastHitTimer = (float)EXBaseApp::NominalGameFrameRate();
}

void Actor::ClearLastHitTimer()
{
    m_ConsecutiveHitCounter = 0;
    m_LastHitTimer = 0.0f;
}

void Actor::ExpandFightDist()
{
    m_FightDistTimer = (EXRandf() * 5.0f + 20.0f) * EXBaseApp::NominalGameFrameRate();
    m_StateFlags.m_bExpandedFightDist = 1;
}

float Actor::GetFightDistance() const
{
    if (m_StateFlags.m_bExpandedFightDist) return GetProfile()->m_FightDist * 4.0f;
    return GetProfile()->m_FightDist;
}

void ActorEffect::SetTimer(int secs) { m_Timer = (float)(secs * 60); }

void ActorEffect::PostLoadInit()
{
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(0x10000e3, 0, 0);
    if (pFile && pFile->m_pGeoHeader) pFile->m_pGeoHeader->DeLoadSectionFiles();
}

void ActorOptionsManager::Init()
{
    for (int i = 0; i < m_Count; i++) {
        if (m_pData[i]) {
            EXFree(m_pData[i]);
            m_pData[i] = 0;
        }
    }
    m_Count = 0;
}

void ActorOptionsManager::Register(ActorOptions *p) { m_pData[m_Count++] = p; }

signed char Actor::BossFlees()
{
    if (ShouldBeSkullRipped()) return 0;
    switch (GetProfile()->m_Hash) {
    case 0x47000159:
    case 0x47000238:
    case 0x47000245:
    case 0x47000246:
    case 0x47000279:
    case 0x4700027a:
        return 1;
    }
    return 0;
}

signed char Actor::CurrentAnimLoops()
{
    EXItemCtrl *pCtrl = m_pAnim ? m_pAnim->m_stacks[0].FirstCtrl() : 0;
    if (!pCtrl) {
        m_ActorFlags2.m_bAnimEnded = 1;
        return 0;
    }
    return (signed char)(pCtrl->m_ctrlFlags.m_flags & 1);
}

void Actor::CheckHasAnimEnded()
{
    EXItemCtrl *pCtrl = m_pAnim ? m_pAnim->m_stacks[0].FirstCtrl() : 0;
    if (pCtrl) {
        if (pCtrl->m_ctrlFlags.BitTest(0x20400)) {
            pCtrl->m_ctrlFlags.BitClr(0x20400);
        } else {
            m_ActorFlags2.m_bAnimEnded = 0;
            return;
        }
    }
    m_ActorFlags2.m_bAnimEnded = 1;
}

unsigned int Actor::RemapVocalMimicrySoundHash(unsigned int hash)
{
    if (m_DefaultContextIcon == 0x4000414) {
        unsigned int s = m_pOptions->MimicrySound();
        m_DefaultContextIcon = 0;
        if (s) return s;
    }
    return hash;
}

void Actor::Delete()
{
    if (InWorld()) {
        m_pItem->ItemFlags().BitSet(0x10);
        return;
    }
    if (m_pItem) {
        delete m_pItem;
        m_pItem = 0;
    }
}

signed char Actor::CanStrafeLeft(float d) { return CanMoveRelative(EXVector(-d, 0.0f, 0.0f, 0.0f)); }

signed char Actor::CanStrafeRight(float d) { return CanMoveRelative(EXVector(d, 0.0f, 0.0f, 0.0f)); }

signed char Actor::CanMoveBack(float d) { return CanMoveRelative(EXVector(0.0f, 0.0f, -d, 0.0f)); }

signed char Actor::CanMoveForward(float d) { return CanMoveRelative(EXVector(0.0f, 0.0f, d, 0.0f)); }

void Actor::UpdateLastHitTimer()
{
    if (!m_ActorFlags2.m_bAnimEnded && m_LastHitTimer > 0.0f) {
        m_LastHitTimer = m_LastHitTimer - XApp::GameFrameRateMul();
        return;
    }
    m_ConsecutiveHitCounter = 0;
    m_LastHitTimer = 0.0f;
}

ActorOptionsManager::ActorOptionsManager() { Init(); }

ActorOptionsManager::~ActorOptionsManager() { Init(); }

ActorOptions *ActorOptionsManager::Options(Actor *pActor)
{
    for (int i = 0; i < m_Count; i++)
        if (m_pData[i]->Within(pActor)) return m_pData[i];
    return &m_Defaults;
}

void ActorOptionsManager::PostProcess()
{
    m_bPlayerVisible = 1;
    for (int i = 0; i < m_Count; i++) {
        if (m_bMimicryUsed) m_pData[i]->MimicryUsed();
        m_pData[i]->PostProcess();
    }
    m_bMimicryUsed = 0;
}

// Registers the new actor in the global actor slot table and resets every member to its
// starting state. Actor::Create fills in the profile data afterwards.
Actor::Actor()
{
    m_Count++;
    m_RefID = Actor_GetNewID();
    GetActorBasePtr(m_RefID)->m_pActor = this;
    m_NonCriticalFrame = Actor_GetNextNonCriticalUpdateFrame();
    m_ThrownPos.SetZero();
    m_Unk08.Set(0.0f, 0.0f, 0.0f, 0.0f);
    m_AnimMode = 0;
    m_AnimType = 0;
    m_AnimModePrev = 0;
    m_AnimModeNext = 0;
    m_AnimMotionOverride = 0x8500;
    m_Unk7a8 = 0x100;
    m_CarryLeft = 0;
    m_CarryRight = 0;
    m_MeetActorID = 0;
    m_StackCount = 0;
    m_StateFlags.m_bMoving14 = 0;
    m_StateFlags.m_bMoving15 = 0;
    memset(m_Stack, 0, sizeof(m_Stack));
    m_StateFlags.m_bRagDoll = 0;
    m_Alpha2 = 1.0f;
    m_RagDollTime = 0.0f;
    m_RagDollSpeed = 0.0f;
    m_AlphaSpeed = 0.0f;
    memset(m_LostLimbs, 0, sizeof(m_LostLimbs));
    m_Health = 100.0f;
    m_HitCountdown = 0.0f;
    m_Attrib = 0;
    m_StateFlags.m_bHeadTrack = 0;
    m_HeadTrackPos.Set(0.0f, 0.0f, 0.0f, 0.0f);
    m_StateFlags.m_bHide = 0;
    m_LookAtPlayerTimer = 0.0f;
    m_Unk948.Clear();
    m_StateFlags.m_bPlayerSeen = 0;
    m_StateFlags.m_bBrainScan = 0;
    m_StateFlags.m_bHitByCar = 0;
    m_StateFlags.m_bOnScreen24 = 0;
    m_StateFlags.m_bOnScreen = 0;
    m_StateFlags.m_bSeenBody = 0;
    m_ActorFlags2.m_bCouldPlayerBeSeen = 0;
    m_Status = 0xb;
    m_WatchFlags = 0xffffffff;
    m_ForceSightingTimer = 0.0f;
    m_pShadow = 0;
    m_CreationFlags = 0;
    m_AliveRadius = 40.0f;
    m_NewSkinIndex.m_Sheet = 0xffff;
    m_NewSkinIndex.m_Row = 0xffff;
    m_pChangeSkinItem = 0;
    m_SqDistFromPlayer = 0.0f;
    m_SqDistFromCamera = 0.0f;
    m_SeenBodyID = 0;
    m_BoundPOI = 0;
    m_AnimSpeedRand = EXRandf() * 0.19999999f + 0.8f;
    m_BrainScanColour = ex::RGBA(0, 0, 0x7f, 0x80);
    m_AnimSpeedMult = 1.0f;
    m_Unk8d4[2] = 1.0f;
    m_Unk8d4[1] = 1.0f;
    m_Unk8d4[0] = 1.0f;
    m_ActorFlags.m_bOverkilled = 0;
    m_DodgeTimer = 0.0f;
    m_Unk820 = 0;
    m_Unk824 = 0;
    m_DefaultContextIcon = 0;
    m_EffectList.Init();
    m_ActorFlags.m_bHolographic = 0;
    m_ActorFlags.m_bVariableSpeed9 = 1;
    m_ActorFlags.m_bDeathObjective = 0;
    m_ActorFlags.m_bVariableSpeedAnims = 1;
    m_ActorFlags.m_bPlayerHolds = 0;
    m_ActorFlags.m_b20 = 0;
    m_ActorFlags.m_bFellInOcean = 0;
    m_ActorFlags.m_b22 = 0;
    m_ActorFlags.m_b23 = 0;
    m_ActorFlags.m_bAiming = 0;
    m_ActorFlags2.m_bInPlayersHand = 0;
    m_ActorFlags2.m_bNudged = 0;
    m_pTextureSwapItem = 0;
    m_pOptions = ActorOptionsManager::Defaults();
    m_NoticedActor = 0;
    m_AimingTime = 0.0f;
    m_AimingTarget = 0;
    m_FiredBulletTime = 0.0f;
    m_CloakBreakTimer = 0.0f;
    m_pAnim = 0;
    m_AlertRaiseTimer = -1.0f;
    m_AlertDecayTimer = 0.0f;
    m_SeenAlertTimer = (EXRandf() + 1.0f) * (float)EXBaseApp::NominalGameFrameRate();
    m_NudgeTimer = 0.0f;
    m_HitPredTimer = 0.0f;
    m_LastCityPOI = 0;
    m_LastHitTimer = 0.0f;
    m_DazeMult = 0.5f;
    m_FightDistTimer = 0.0f;
    m_pHitByVehicle = 0;
    m_ActorFlags2.m_bOnFire = 0;
    m_HitByVehiclePos.Set(0.0f, 0.0f, 0.0f, 0.0f);
    m_CivilianAttackVOTimer = 0.0f;
    m_EMPTimer = 0.0f;
    m_ActorFlags2.m_b12 = 0;
    m_ActorFlags2.m_bLongRangeVision = 0;
    m_NavRoute.Init(0, 0);
    m_OrgLoc.Reset();
    m_DestLoc.Reset();
    m_Unk18.SetZero();
    m_LocalVelocity.SetZero();
    m_WorldVelocity.SetZero();
    m_StateFlags.m_b0 = 0;
    m_StateFlags.m_b1 = 0;
    m_StateFlags.m_b2 = 0;
    m_StateFlags.m_b3 = 0;
    m_StateFlags.m_bDontDoHitReaction = 0;
    m_StateFlags.m_bLyingOnGround = 0;
    m_StateFlags.m_bTargetable = 1;
    m_StateFlags.m_bExpandedFightDist = 0;
    m_StateFlags.m_b8 = 1;
    m_StateFlags.m_bCloaked = 0;
    m_StateFlags.m_bLyingFaceDown = 0;
    m_StateFlags.m_bCanCollideWithPlayer = 1;
    m_StateFlags.m_bCanRipBossHead = 0;
    m_StateFlags.m_bStartBossHeadRipped = 0;
    m_StateFlags.m_b28 = 0;
    m_StateFlags.m_bDontDragMeSpear = 0;
    m_StateFlags.m_bBeingStealthKilled = 0;
    m_StateFlags.m_bAnimSpeedRand = 0;
    m_ActorFlags2.m_bFiringAnim = 0;
    m_ActorFlags2.m_bHanging = 0;
    m_ActorFlags2.m_bLyingBum = 0;
    m_ActorFlags2.m_bDazed = 0;
    m_ActorFlags2.m_bScriptRunControl = 0;
    m_ActorFlags2.m_bBeingDestructed = 0;
    m_ActorFlags2.m_bHasBeenKicked = 0;
    m_ActorFlags2.m_bAnimEnded = 0;
    m_ActorFlags2.m_bFleeToStatue = 0;
    m_ActorFlags2.m_bIgnoreAnimChanges = 0;
    m_ActorFlags.m_bCanBeJuggled = 1;
    m_ActorFlags.m_bBeingJuggled = 0;
    m_ActorFlags.m_bLeftArmMissing = 0;
    m_ActorFlags.m_bRightArmMissing = 0;
    m_ActorFlags.m_bInCombatWithPlayer = 0;
    m_ActorFlags.m_bCanLooseArmAndLive = 0;
    m_ActorFlags.m_bGoingToStaggerAndDie = 0;
    m_ActorFlags.m_bMovementEnabled = 1;
    m_ActorFlags.m_bHasThrownPetrolBomb = 0;
    m_ActorFlags.m_b24 = 0;
    m_ActorFlags.m_b25 = 0;
    m_ActorFlags.m_b26 = 0;
    m_ActorFlags.m_bHasCollided = 0;
    m_ActorFlags.m_bBlinded = 0;
    m_ActorFlags.m_b29 = 0;
    m_ActorFlags.m_bCannotDie = 0;
    m_CollisionRadius = 0.5f;
    m_Unk784 = 0;
    m_AvoidLastSteer = 0.0f;
    m_HeadTrackMode = 0;
    m_CombatCircleStopTime = 0.0f;
    m_SubGroupAnimIndex = 0;
    m_UpdateTime = 0.0f;
    m_NotAllowedToGetUpTimer = 0.0f;
    m_LastHitSFX = 0;
    m_NudgeCount = 0;
    m_CollidedWith = 0;
    m_Unk90c[2] = 0;
    m_Unk90c[1] = 0;
    m_Unk90c[0] = 0;
    m_WalkAnim = 0;
    m_Unk818 = 0;
    m_Unk81c = 0;
    m_Unk82c = 0;
    m_Unk830 = 0;
    m_pRagDoll = 0;
    m_pHeadPhysics = 0;
    m_HitBy = -1;
    m_CollisionSphereRadius = 1.0f;
    m_BlockCounter = 0.0f;
    m_InvinsibleCounter = 0.0f;
    m_ConsecutiveHitCounter = 0;
    m_Momentum.SetZero();
    m_Unk8e0[0] = 1.0f;
    m_Unk8e0[1] = 1.0f;
    m_Unk8e0[2] = 1.0f;
    m_TargetRotYDelta = 0.0f;
    m_StateFlags.m_bMoving27 = 0;
    m_TurnStartRotY = 0.0f;
    m_Unk864 = (float)cos(DegToRad(100.0f));
    m_Unk868 = (float)cos(DegToRad(50.0f));
    m_Unk86c = 5.0f;
    m_Unk8f4 = (float)EXBaseApp::NominalGameFrameRate() * 0.75f;
    m_Unk8f8 = 0.0f;
    m_PelvisPosCache.Set(0.0f, 0.0f, 0.0f, 0.0f);
    m_PelvisPosFrame = 9999999;
    m_HeadPosCache.Set(0.0f, 0.0f, 0.0f, 0.0f);
    m_HeadPosFrame = 9999999;
    m_ChestPosCache.Set(0.0f, 0.0f, 0.0f, 0.0f);
    m_ChestPosFrame = 9999999;
    m_AimingPos.SetZero();
    m_Unk898 = 0.0f;
    m_RandomDir.SetZero();
    m_RandomDirTimer = 0.0f;
}

// Drops everything the actor owns: sounds, carried objects, the changing skin, effects, lost
// limbs, the skin geometry references and its actor slot.
Actor::~Actor()
{
    m_Count--;
    m_ActorFlags2.m_bBeingDestructed = 1;
    m_SFXTag.RemoveAll(1);
    m_SM.FlushStack();
    if (ActorCarryable::GetPtrFromID(m_CarryLeft)) ActorCarryable::GetPtrFromID(m_CarryLeft)->Delete();
    if (ActorCarryable::GetPtrFromID(m_CarryRight)) ActorCarryable::GetPtrFromID(m_CarryRight)->Delete();
    m_CarryLeft = 0;
    m_CarryRight = 0;
    DeleteChangeSkin();
    EffectDeleteAll();
    for (int i = 0; i < 4; i++) DeleteLostLimb((ActorLostLimb)i);
    if (m_StateFlags.m_bRagDoll) gRagDollCount--;
    if (gRagDollCount < 0) gRagDollCount = 0;
    m_TexSwapInfo.StopAllSwaps();
    ActorSkin *pSkin = AS_GetPtrFromIndex(m_SkinIndex);
    GeoSectionManager::Del(pSkin->m_File, pSkin->m_Hash);
    GeoSectionManager::Del(pSkin->m_ChangeFile, pSkin->m_ChangeHash);
    GeoSectionManager::Del(pSkin->m_SkelFile, pSkin->m_SkelHash);
    ActorSpecial_Unregister(this);
    m_SpeechTag.RemoveAll(1);
    ActorBase *pBase = GetActorBasePtr(m_RefID);
    if (pBase) {
        pBase->m_pActor = 0;
        pBase->m_bInWorld = 0;
    }
}

// Whether a move by delta (in the actor's local space) stays on the nav mesh: the delta is
// rotated by the facing (yaw only on a 2D location) and tried on a copy of the location.
signed char Actor::CanMoveRelative(const EXVector &delta)
{
    EXMatrix m;
    if (!m_OrgLoc.Is3D()) {
        float ry = GetRotation().y;
        m.SetIdentity();
        m.SetRotation(EXVector(0.0f, ry, 0.0f, 0.0f), 4);
    } else {
        EXQuaternion q(GetRotation());
        EXMatrix t;
        q.GetRotation(t);
        m = t;
    }
    EXVector d = (const EXMaths::mat_44 &)m * delta;
    u32 flags = 0;
    XNavLocation loc(m_OrgLoc);
    loc.Move(d, &flags);
    return !(flags & 2);
}

// Fills the HUD scan page: 0 = description and armour, 1 = sub group and profile text, 2 = the
// carried weapon. Each page can run the skin's script for it.
signed char Actor::GetScanInfo(int type, ScanInfo *pInfo)
{
    ActorSkin *pSkin = AS_GetPtrFromIndex(m_SkinIndex);
    switch (type) {
    case 0: {
        int h = pSkin->m_ScanScript[0];
        if (h > 0x4000000 && h < 0x40007d9) pInfo->m_a0 = h;
        else pInfo->m_a0 = 0;
        int text;
        if (AP_GetPtrFromIndex(m_ProfileIndex)->m_ScanDescription)
            text = AP_GetPtrFromIndex(m_ProfileIndex)->m_ScanDescription;
        else
            text = ArmourProfile::GetArmourDataFromIndex(m_ArmourIndex)->m_Description;
        pInfo->m_Text[0][0] = text;
        pInfo->m_Text[1][0] = 0x510007d3;
        // Retail tests the address of the title (likely an array name used without [0]), so the
        // "No Data" fallback never runs.
        if (!pInfo->m_Text[0][0] && !&pInfo->m_Text[1][0]) pInfo->m_Text[0][0] = 0x51000116;
        break;
    }
    case 1: {
        int h = pSkin->m_ScanScript[1];
        if (h > 0x4000000 && h < 0x40007d9) pInfo->m_a0 = h;
        else pInfo->m_a0 = 0;
        pInfo->m_Text[1][0] = SubGroupAnims::GetSubGroupAnims(m_SubGroupAnimIndex).m_Anims[0];
        pInfo->m_Text[1][1] = AP_GetPtrFromIndex(m_ProfileIndex)->m_ScanText84;
        pInfo->m_Text[3][0] = m_ScanText;
        break;
    }
    case 2: {
        int h = pSkin->m_ScanScript[2];
        if (h > 0x4000000 && h < 0x40007d9) pInfo->m_a0 = h;
        else pInfo->m_a0 = 0;
        pInfo->m_Text[0][0] = AP_GetPtrFromIndex(m_ProfileIndex)->m_ScanText88;
        WeaponProfile *pWeapon = GetWeaponProfile(0);
        pInfo->m_ModelFile = 0x100003c;
        if (pWeapon->m_CarryEntity) {
            pInfo->m_ModelEntity = pWeapon->m_CarryEntity;
            pInfo->m_Text[1][0] = ActorCarryable::FindTextForEntity(pWeapon->m_CarryEntity);
        } else if (ActorCarryable::GetPtrFromID(m_CarryRight)) {
            pInfo->m_ModelEntity = ActorCarryable::GetPtrFromID(m_CarryRight)->m_EntityHash;
            pInfo->m_Text[1][0] = ActorCarryable::GetPtrFromID(m_CarryRight)->m_Description;
        } else if (ActorCarryable::GetPtrFromID(m_CarryLeft)) {
            pInfo->m_ModelEntity = ActorCarryable::GetPtrFromID(m_CarryLeft)->m_EntityHash;
            pInfo->m_Text[1][0] = ActorCarryable::GetPtrFromID(m_CarryLeft)->m_Description;
        } else {
            pInfo->m_Text[1][0] = 0x510000f5;
        }
        if (!pInfo->m_Text[1][0]) pInfo->m_Text[1][0] = 0x5100005a;
        break;
    }
    default:
        pInfo->m_a0 = 0;
        break;
    }
    pInfo->m_id90 = m_RefID;
    pInfo->m_a4 = m_Health / AP_GetPtrFromIndex(m_ProfileIndex)->m_Health;
    s8 r = pInfo->m_a0 > 0;
    pInfo->m_ac = r;
    pInfo->m_pCallback = ScanCallback;
    return r;
}

// HUD scan callback. On open (b set), page 1 tints the scanned actor's brain in its affiliation
// colour and plays the brain overlay script. Otherwise pages 0 and 2 refresh the health text,
// and page 2 drops its "complete" mark once the actor holds a different weapon.
signed char Actor::ScanCallback(signed char b, FEObject *pObj, ScanInfo *pInfo)
{
    if (b) {
        switch (pInfo->m_0) {
        case 1: {
            Actor *pActor = GetActorPtr(pInfo->m_id90);
            if (!pActor) break;
            // TODO name: +0x20 of the sub group row is set to 1 while scanned.
            *(float *)&SubGroupAnims::GetSubGroupAnims(*pActor).m_Anims[7] = 1.0f;
            if (AP_GetPtrFromIndex(pActor->m_ProfileIndex)->m_SubGroup == 0x4f00002b)
                pActor->m_DefaultContextIcon = 0x4000417;
            SetGlobalAffiliation(AP_GetPtrFromIndex(pActor->m_ProfileIndex)->m_SubGroup);
            ex::RGBA c = pActor->GetBrainScanColour();
            ex::RGBA from;
            from.m_R = c.m_R;
            from.m_G = c.m_G;
            from.m_B = c.m_B;
            from.m_A = 0;
            FEObject *pBrain = pObj->Object(0, 0)->Object(0);
            pBrain->ScriptAdd(0x5f5f5f6e69617262, (FEObjType::Flags)4, 0x1000025, 0x40005f3, (EXW::TextAlign)4);
            pBrain->Script(0x5f5f5f6e69617262)->SetMathmos(FEMathmos(from, c, 30.0f, (FEWaveform::Flags)1, 1));
            pBrain->Script(0x5f5f5f6e69617262)->SetAttachedTo(0);
            pBrain->Script(0x5f5f5f6e69617262)->Play(0, 0);
            break;
        }
        }
        return 1;
    }
    switch (pInfo->m_0) {
    case 0:
    case 2: {
        Actor *pActor = GetActorPtr(pInfo->m_id90);
        if (!pActor) break;
        float health = pActor->m_Health / AP_GetPtrFromIndex(pActor->m_ProfileIndex)->m_Health;
        pInfo->m_a4 = health;
        if (health <= kActorSmallStep) pInfo->m_text.Format("Critical");
        else if (health < 0.5f) pInfo->m_text.Format("Wounded");
        else if (health < 1.0f) pInfo->m_text.Format("Hurt");
        else pInfo->m_text.Format("Healthy");
        if (pInfo->m_0 != 2) break;
        int entity = 0;
        WeaponProfile *pWeapon = pActor->GetWeaponProfile(0);
        if (pWeapon->m_CarryEntity) entity = pWeapon->m_CarryEntity;
        else if (ActorCarryable::GetPtrFromID(pActor->m_CarryRight))
            entity = ActorCarryable::GetPtrFromID(pActor->m_CarryRight)->m_EntityHash;
        else if (ActorCarryable::GetPtrFromID(pActor->m_CarryLeft))
            entity = ActorCarryable::GetPtrFromID(pActor->m_CarryLeft)->m_EntityHash;
        if (entity != (int)pInfo->m_ModelEntity) pInfo->m_ac = 0;
        break;
    }
    }
    return 1;
}

// Where to aim at a target (1 = the player, else an actor id). A cloaked player is aimed at
// around the last seen position, scattered along m_RandomDir more the farther away it is,
// unless the actor's sub group can see through the cloak.
EXVector Actor::GetAimPosForTarget(int target)
{
    if (target) {
        if (target == 1) {
            if (Player_IsCloaked() && !SubGroupAnims::GetSubGroupAnims(*this).m_Anims[8]) {
                const EXVector &seen = gAIControl.LastSeenPlayerPos();
                EXVector pos(seen.x, seen.y + 1.0f, seen.z, seen.w);
                EXVector d = pos - GetFeetPos();
                float dist = d.LengthXYZ();
                float spread;
                if (dist < 20.0f) {
                    spread = dist;
                    if (1.0f > spread) spread = 1.0f;
                    spread *= 0.4f;
                } else {
                    spread = 8.0f;
                }
                return pos + m_RandomDir * spread;
            }
            if (XItemHandler_Player::Instance()) return XItemHandler_Player::Instance()->GetChestPos();
        } else {
            Actor *pActor = GetActorPtr(target);
            if (pActor) return pActor->GetChestPos();
        }
    }
    return EXVector(0.0f, 0.0f, 0.0f, 0.0f);
}

// Body i of a body group, or 0 past the end (inlined in Actor.obj with a branch).
static __forceinline EX6DParticleBase *Actor_GetBody(EX6DBodyGroup *p, int i)
{
    if (i < p->BodyCount()) return p->Body(i);
    return 0;
}

// Pushes every rag doll body along dir with a random share of the force (capped at 20); the
// shares are scaled so they average to one.
void Actor::ApplyRagDollForceAll(const EXVector &dir, float force)
{
    if (!m_StateFlags.m_bRagDoll) return;
    EXVector n = dir;
    n.Normalise();
    if (force > 20.0f) force = 20.0f;
    EX6DAnim *p = (EX6DAnim *)m_pItem->m_pItemPhysics;
    if (!p->GetRuntimeClass()->IsDerivedFrom(EXRUNTIME_CLASS(EX6DAnim))) return;
    int count = p->BodyCount();
    float total = 0.0f;
    float share[16];
    int i;
    for (i = 0; i < count; i++) {
        share[i] = EXRandf(3.4f) - 0.70000005f;
        total += share[i];
    }
    if (total == 0.0f) return;
    float scale = 1.0f / (total / (float)count);
    n = n * force;
    for (i = 0; i < count; i++) {
        float f = share[i] * scale;
        EXVector v = n * f;
        Actor_GetBody(p, i)->AddWorldImpulse(v);
    }
}

// Collision against a quad given as four corners (p[0], p[1], p[2], p[3]), tested as the two
// triangles (0, 1, 2) and (1, 3, 2).
// EXTriangle::Set as retail inlines it here (our build otherwise calls it out of line).
static __forceinline void Actor_SetTriangle(EXTriangle &t, const EXVector &a, const EXVector &b, const EXVector &c)
{
    t.m_v[0].set(a);
    t.Edge0Set(b - a);
    t.Edge1Set(c - a);
}

int Actor::CheckCollision(const EXVector *p, float r, EXVector &out)
{
    EXTriangle t;
    Actor_SetTriangle(t, p[0], p[1], p[2]);
    int hit = CheckCollision(t, r, out);
    if (hit) return hit;
    Actor_SetTriangle(t, p[1], p[3], p[2]);
    return CheckCollision(t, r, out);
}

// Collision against a triangle (a blade swing). While blocking, the left hand (or its weapon)
// parries: the hit point is moved 0.75 towards the triangle and any contact counts. Otherwise
// each body part capsule is tested and the first hit returns that part's datum hash.
int Actor::CheckCollision(const EXTriangle &tri, float r, EXVector &out)
{
    if (m_BlockCounter > 0.0f) {
        EXVector hand;
        ActorCarryable *pCarry = ActorCarryable::GetPtrFromID(m_CarryLeft);
        if (!pCarry || !pCarry->GetWeaponCollisionPos(hand)) {
            EXDatumTemp d;
            memset(&d, 0, sizeof(d));
            if (m_pItem->GetAnimatorDatum(0x10000013, d, 0, 0)) {
                hand = d.m_position;
            } else {
                EXHashCodeFile::Debug_HashCodeName(GetProfile()->m_Hash);
                hand.Set(0.0f, 0.0f, 0.0f, 0.0f);
            }
        }
        EXVector p = hand;
        EXVector dir = tri.Origin() - hand;
        dir.Normalise();
        dir *= 0.75f;
        out = dir + hand;
        if (m_BlockCounter > 30.0f) return 1;
        if (EXSqrDistance(p, tri) < r * r + 0.5625f) return 1;
    }
    const int *pHash = s_CollisionDatums;
    int i = 0;
    do {
        int hash = *pHash;
        EXDatumTemp d;
        memset(&d, 0, sizeof(d));
        Actor_CalledGetDatum++;
        if (m_pItem->GetAnimatorDatum(hash, d, 0, 0)) {
            EXCapsule c;
            memset(&c, 0, sizeof(c));
            d.Get(c);
            float t = 0.0f;
            if (EXSqrt(EXSqrDistance(c.Segment(), tri, &t)) <= c.Radius() + r) {
                out = c.Origin() + c.Direction() * t;
                return s_CollisionDatums[i];
            }
        }
        pHash++;
        i++;
    } while (*pHash);
    return 0;
}

// Collision against a capsule (a thrown weapon, a disc). Blocking works as for triangles but
// the parry test is a 0.75 sphere at the left weapon. Otherwise the first body part capsule
// within reach returns its datum hash and the nearest point on it.
int Actor::CheckCollision(const EXCapsule &cap, EXVector &out)
{
    if (m_BlockCounter > 0.0f) {
        EXVector hand;
        ActorCarryable *pCarry = ActorCarryable::GetPtrFromID(m_CarryLeft);
        if (!pCarry || !pCarry->GetWeaponCollisionPos(hand)) {
            EXDatumTemp d;
            memset(&d, 0, sizeof(d));
            if (m_pItem->GetAnimatorDatum(0x10000013, d, 0, 0)) {
                hand = d.m_position;
            } else {
                EXHashCodeFile::Debug_HashCodeName(GetProfile()->m_Hash);
                hand.Set(0.0f, 0.0f, 0.0f, 0.0f);
            }
        }
        EXVector dir = (cap.Origin() + cap.Direction() * 0.5f) - hand;
        dir.Normalise();
        dir *= 0.75f;
        out = dir + hand;
        if (m_BlockCounter > 30.0f) return 1;
        EXSphere s;
        s.Set(GetWeaponPos(1), 0.75f);
        if (EXIntersection(s, cap)) return 1;
    }
    const int *pHash = s_CollisionDatums;
    int i = 0;
    do {
        int hash = *pHash;
        EXDatumTemp d;
        memset(&d, 0, sizeof(d));
        Actor_CalledGetDatum++;
        if (m_pItem->GetAnimatorDatum(hash, d, 0, 0)) {
            EXCapsule c;
            d.Get(c);
            EXVector o = c.Origin();
            if (isValidFloat(o)) {
                float t = 0.0f;
                float t2 = 0.0f;
                if (EXSqrt(EXSqrDistance(c.Segment(), cap.Segment(), &t, &t2)) <= cap.Radius() + c.Radius()) {
                    out = c.Origin() + c.Direction() * t;
                    return s_CollisionDatums[i];
                }
            }
        }
        pHash++;
        i++;
    } while (*pHash);
    return 0;
}

// Turns the actor towards rot (radians). A new turn of more than tolerance degrees starts the
// turn left/right animation for the actor's kind and sets the target facing; while a turn is
// playing (before 85% of it) the target is updated, never moving back past the current facing.
void Actor::SetTargetYRot(float rot, float tolerance)
{
    while (rot < 0.0f) rot += 6.2831855f;
    while (rot > 6.2831855f) rot -= 6.2831855f;
    if (m_StateFlags.m_bMoving27) {
        if (!(GetAnimFrame() < 0.85f)) return;
        rot = XNavNetwork::AngleDifference(m_TurnStartRotY, rot);
        if (rot > 3.1415927f) rot = 3.1415927f;
        else if (rot < -3.1415927f) rot = -3.1415927f;
        float cur = m_pItem->Rotation().y;
        float target = cur;
        if (m_StateFlags.m_b28) {
            if (!(0.0f >= rot)) {
                if (rot > XNavNetwork::AngleDifference(m_TurnStartRotY, cur)) {
                    target = m_TurnStartRotY + rot;
                } else {
                    m_TargetRotYDelta = cur;
                    return;
                }
            }
        } else {
            if (!(rot >= 0.0f)) {
                if (XNavNetwork::AngleDifference(m_TurnStartRotY, cur) > rot) {
                    target = m_TurnStartRotY + rot;
                } else {
                    m_TargetRotYDelta = cur;
                    return;
                }
            }
        }
        m_TargetRotYDelta = target;
        return;
    }
    float cur = m_pItem->Rotation().y;
    if (fabs(rot = XNavNetwork::AngleDifference(cur, rot)) < tolerance / 180.0f * 3.1415927f) return;
    m_TurnStartRotY = cur;
    if (rot > 0.0f) {
        if (m_ActorFlags.m_bUlysses) {
            SetAnimMode(0x9000240, 1);
        } else if (m_ActorFlags.m_bBoss) {
            switch (GetProfile()->m_SubGroup) {
            case 0x4f00000e:
            case 0x4f000013:
                SetAnimMode(0x90004cb, 1);
                break;
            case 0x4f00000d:
                SetAnimMode(0x900052a, 1);
                break;
            case 0x4f000010:
                SetAnimMode(0x90004d5, 1);
                break;
            case 0x4f000011:
                SetAnimMode(0x900053b, 1);
                break;
            case 0x4f00000f:
                SetAnimMode(0x9000312, 1);
                break;
            case 0x4f000017:
                SetAnimMode(0x900051d, 1);
                break;
            default:
                SetAnimMode(0x900051d, 1);
                break;
            }
        } else if (m_ActorFlags.m_bAiming) {
            switch (GetWeaponProfile(0)->m_AimType) {
            case 0x52000004:
                SetAnimMode(0x9000356, 1);
                break;
            case 0x52000005:
                SetAnimMode(0x9000358, 1);
                break;
            case 0x52000006:
                SetAnimMode(0x900035b, 1);
                break;
            case 0x52000000:
            case 0x52000007:
                SetAnimMode(0x9000306, 1);
                break;
            default:
                SetAnimMode(0x9000306, 1);
                break;
            }
        } else {
            SetAnimMode(0x9000306, 1);
        }
        m_StateFlags.m_b28 = 1;
    } else {
        if (m_ActorFlags.m_bUlysses) {
            SetAnimMode(0x900023f, 1);
        } else if (m_ActorFlags.m_bBoss) {
            switch (GetProfile()->m_SubGroup) {
            case 0x4f00000e:
            case 0x4f000013:
                SetAnimMode(0x90004ca, 1);
                break;
            case 0x4f000010:
                SetAnimMode(0x90004d4, 1);
                break;
            case 0x4f00000d:
                SetAnimMode(0x9000529, 1);
                break;
            case 0x4f000011:
                SetAnimMode(0x900053c, 1);
                break;
            case 0x4f00000f:
                SetAnimMode(0x9000311, 1);
                break;
            case 0x4f000017:
                SetAnimMode(0x900051c, 1);
                break;
            default:
                SetAnimMode(0x900051c, 1);
                break;
            }
        } else if (m_ActorFlags.m_bAiming) {
            switch (GetWeaponProfile(0)->m_AimType) {
            case 0x52000004:
                SetAnimMode(0x9000355, 1);
                break;
            case 0x52000005:
                SetAnimMode(0x9000359, 1);
                break;
            case 0x52000006:
                SetAnimMode(0x900035c, 1);
                break;
            case 0x52000000:
            case 0x52000007:
                SetAnimMode(0x9000307, 1);
                break;
            default:
                SetAnimMode(0x9000307, 1);
                break;
            }
        } else {
            SetAnimMode(0x9000307, 1);
        }
        m_StateFlags.m_b28 = 0;
    }
    if (rot > 3.1415927f) rot = 3.1415927f;
    else if (rot < -3.1415927f) rot = -3.1415927f;
    m_TargetRotYDelta = m_TurnStartRotY + rot;
    while (m_TargetRotYDelta < 0.0f) m_TargetRotYDelta += 6.2831855f;
    while (m_TargetRotYDelta > 6.2831855f) m_TargetRotYDelta -= 6.2831855f;
    m_StateFlags.m_bMoving14 = 0;
    m_StateFlags.m_bMoving27 = 1;
}

// EXVector::Normalise, forced inline as retail has it in GetGunDirection.
static __forceinline float Actor_Normalise(EXVector &v)
{
    float len = v.Length();
    if (len > EX_EPSILON_UNIT) v *= 1.0f / len;
    return len;
}

// Direction to fire the right hand gun at target: the muzzle's own direction, pulled towards
// the target by at most 0.2 (nothing under 5 units away, growing up to 10). The predator
// sniper gun always fires along its muzzle.
EXVector Actor::GetGunDirection(const EXVector &target)
{
    EXVector dir;
    dir.Set(1.0f, 0.0f, 0.0f, 0.0f);
    ActorCarryableGun *pGun = (ActorCarryableGun *)ActorCarryable::GetPtrFromID(m_CarryRight);
    if (!pGun->IsAGun()) return dir;
    // The sniper gun's datum pair is read into the same two locals the other way round.
    EXVector muzzle;
    EXVector pos;
    if (pGun->m_EntityHash == 0x2000184) {
        if (pGun->GetMuzzleDatumCorrect(muzzle, pos)) return pos;
        return EXVector(1.0f, 0.0f, 0.0f, 0.0f);
    }
    if (!pGun->GetMuzzleDatumCorrect(pos, muzzle)) return EXVector(1.0f, 0.0f, 0.0f, 0.0f);
    dir = target - pos;
    float dist = Actor_Normalise(dir);
    EXVector diff = dir - muzzle;
    float maxDev;
    if (dist >= 5.0f) {
        maxDev = 0.2f;
        if (dist < 10.0f) maxDev = (dist - 5.0f) * 0.04f;
    } else {
        maxDev = 0.0f;
    }
    if (maxDev * maxDev < diff.LengthXYZSquared()) {
        Actor_Normalise(diff);
        dir = diff * maxDev + muzzle;
        Actor_Normalise(dir);
    }
    return dir;
}

// Picks the aim (or fire) animation for the aiming target's direction (1/4 = down, 2/3 =
// left) from the weapon's aim class, or the boss's own set. Single shot weapons and finished
// fire animations go back to aiming.
void Actor::SetAimingAnim()
{
    WeaponProfile *pWeapon = GetWeaponProfile(0);
    if (!pWeapon) return;
    if (m_ActorFlags2.m_bFiringAnim && m_ActorFlags2.m_bAnimEnded) m_ActorFlags2.m_bFiringAnim = 0;
    if (pWeapon->m_FireType == 1) m_ActorFlags2.m_bFiringAnim = 0;
    if (m_ActorFlags2.m_bFiringAnim) {
        switch (m_AimingTarget) {
        case 1:
        case 4:
            if (m_ActorFlags.m_bBoss) {
                if (GetProfile()->m_SubGroup == 0x4f00000f) {
                    if (GetProfile()->m_Weapon == 0x20000de) SetAnimType(0x90003c6, 1, 0);
                    else SetAnimType(0x90003cb, 1, 0);
                    return;
                }
                if (GetSkin()->m_Hash == 0xd0000b9) {
                    SetAnimType(0x9000410, 1, 0);
                    return;
                }
                if (GetProfile()->m_SubGroup == 0x4f00000d) SetAnimType(0x90004e5, 1, 0);
                else SetAnimType(0x90003cb, 1, 0);
                return;
            }
            switch (pWeapon->m_AimType) {
            case 0x52000004:
                SetAnimType(0x900045e, 1, 0);
                return;
            case 0x52000005:
                SetAnimType(0x900044f, 1, 0);
                return;
            case 0x52000006:
                SetAnimType(0x9000467, 1, 0);
                return;
            case 0x52000007:
                SetAnimType(0x4e000001, 0, 1);
                return;
            }
            break;
        case 2:
        case 3:
            if (m_ActorFlags.m_bBoss) {
                if (GetProfile()->m_SubGroup == 0x4f00000f) {
                    if (GetProfile()->m_Weapon == 0x20000de) SetAnimType(0x90003c8, 1, 0);
                    else SetAnimType(0x90003c9, 1, 0);
                    return;
                }
                if (GetSkin()->m_Hash == 0xd0000b9) {
                    SetAnimType(0x9000416, 1, 0);
                    return;
                }
                if (GetProfile()->m_SubGroup == 0x4f00000d) SetAnimType(0x90004e6, 1, 0);
                else SetAnimType(0x90003c9, 1, 0);
                return;
            }
            switch (pWeapon->m_AimType) {
            case 0x52000004:
                SetAnimType(0x9000462, 1, 0);
                return;
            case 0x52000005:
                SetAnimType(0x9000450, 1, 0);
                return;
            case 0x52000006:
                SetAnimType(0x900046d, 1, 0);
                return;
            case 0x52000007:
                SetAnimType(0x4e000001, 0, 1);
                return;
            }
            break;
        }
    } else {
        switch (m_AimingTarget) {
        case 1:
        case 4:
            if (m_ActorFlags.m_bBoss) {
                if (GetProfile()->m_SubGroup == 0x4f00000f) {
                    SetAnimType(0x90003c6, 1, 0);
                    return;
                }
                if (GetSkin()->m_Hash == 0xd0000b9) {
                    SetAnimType(0x9000411, 1, 0);
                    return;
                }
                if (GetProfile()->m_SubGroup == 0x4f00000d) SetAnimType(0x90004e0, 1, 0);
                else SetAnimType(0x90003c6, 1, 0);
                return;
            }
            switch (pWeapon->m_AimType) {
            case 0x52000004:
                SetAnimType(0x9000460, 1, 0);
                return;
            case 0x52000005:
                SetAnimType(0x90003c0, 1, 0);
                return;
            case 0x52000006:
                SetAnimType(0x9000469, 1, 0);
                return;
            case 0x52000007:
                SetAnimType(0x4e000001, 0, 1);
                return;
            }
            break;
        case 2:
        case 3:
            if (m_ActorFlags.m_bBoss) {
                if (GetProfile()->m_SubGroup == 0x4f00000f) {
                    SetAnimType(0x90003c8, 1, 0);
                    return;
                }
                if (GetSkin()->m_Hash == 0xd0000b9) {
                    SetAnimType(0x9000417, 1, 0);
                    return;
                }
                if (GetProfile()->m_SubGroup == 0x4f00000d) SetAnimType(0x90004e1, 1, 0);
                else SetAnimType(0x90003c8, 1, 0);
                return;
            }
            switch (pWeapon->m_AimType) {
            case 0x52000004:
                SetAnimType(0x9000461, 1, 0);
                return;
            case 0x52000005:
                SetAnimType(0x90003c1, 1, 0);
                return;
            case 0x52000006:
                SetAnimType(0x900046a, 1, 0);
                return;
            case 0x52000007:
                SetAnimType(0x4e000001, 0, 1);
                return;
            }
            break;
        }
    }
}

extern unsigned int gCurrentMap_GeoHash;  // TODO stand-in (GameIsland.obj)
signed char STATE_CityDistractions(StateMachine &sm);  // ActorCity.obj

// Brain scan (vision mode) colour and scan text from the actor's status: green passive or
// panicking, red aggressive or violent, yellow hostile, orange hostile gangs. Bit 22 of the
// state flags marks the ones that pulse. bFade steps the shown colour towards the new one.
ex::RGBA Actor::UpdateBrainScanColour(signed char bFade)
{
    ActorProfile *pProfile = GetProfile();
    ActorSkin *pSkin = GetSkin();
    SubGroupAnims &sub = SubGroupAnims::GetSubGroupAnims(*this);
    ex::RGBA c;
    ActorStateFlags flags;
    if (pProfile->m_SubGroup == 0x4f000017) {
        flags = m_StateFlags;
        c = ex::RGBA(0x7f, 0x7f, 0x7f, 0x80);
        m_ScanText = 0x510000f4;
        flags.m_bBrainScan = 0;
    } else if ((flags = m_StateFlags).m_bCloaked || (sub.m_Anims[2] > 0 && sub.m_Anims[2] != pSkin->m_Hash)) {
        c = ex::RGBA(0, 0, 0, 0x80);
        m_ScanText = 0x510000f4;
        flags.m_bBrainScan = 0;
    } else {
        switch (m_Status) {
        case 0:
            c = ex::RGBA(0, 0, 0, 0x80);
            m_ScanText = 0;
            flags.m_bBrainScan = 0;
            break;
        case 1:
            if (m_ActorFlags2.m_bInPlayersHand) {
                c = ex::RGBA(0, 0x7f, 0, 0x80);
                flags.m_bBrainScan = 1;
                m_ScanText = 0x510007d7;
            } else {
                c = ex::RGBA(0, 0, 0, 0x80);
                flags.m_bBrainScan = 0;
                m_ScanText = 0;
            }
            break;
        case 4:
        case 12:
            c = ex::RGBA(0x7f, 0, 0, 0x80);
            m_ScanText = 0x510007d6;
            flags.m_bBrainScan = 0;
            break;
        case 13:
            c = ex::RGBA(0x7f, 0, 0, 0x80);
            flags.m_bBrainScan = 1;
            m_ScanText = 0x510007d8;
            break;
        case 5:
            c = ex::RGBA(0x7f, 0x40, 0, 0x80);
            m_ScanText = 0x510007d6;
            flags.m_bBrainScan = 0;
            break;
        case 7:
            c = ex::RGBA(0x7f, 0x7f, 0, 0x80);
            flags.m_bBrainScan = 1;
            m_ScanText = 0x510007d5;
            break;
        case 6:
        case 8:
            c = ex::RGBA(0x7f, 0x7f, 0, 0x80);
            m_ScanText = 0x510007d5;
            flags.m_bBrainScan = 0;
            break;
        case 9:
            c = ex::RGBA(0x7f, 0, 0, 0x80);
            flags.m_bBrainScan = 1;
            m_ScanText = 0x510007d5;
            break;
        case 10:
            c = ex::RGBA(0, 0, 0x7f, 0x80);
            m_ScanText = 0x510007d4;
            flags.m_bBrainScan = 0;
            break;
        case 3:
        case 14:
            c = ex::RGBA(0, 0x7f, 0, 0x80);
            flags.m_bBrainScan = 1;
            m_ScanText = 0x510007d7;
            break;
        default:
            switch (GetProfile()->m_Group) {
            case 0x4b000002:
            case 0x4b000003:
            case 0x4b000004:
            case 0x4b000005:
            case 0x4b000006:
            case 0x4b000007:
            case 0x4b000009:
            case 0x4b00000a:
            case 0x4b00000b:
            case 0x4b00000c:
                c = ex::RGBA(0x7f, 0x80, 0, 0x80);
                flags.m_bBrainScan = 0;
                m_ScanText = 0x510007d5;
                break;
            case 0x4b000001:
            case 0x4b000008:
                c = ex::RGBA(0, 0x7f, 0, 0x80);
                m_ScanText = 0x510007d4;
                flags.m_bBrainScan = 0;
                break;
            default:
                c = ex::RGBA(0, 0x7f, 0, 0x80);
                m_ScanText = 0x510007d4;
                flags.m_bBrainScan = 0;
                break;
            }
            break;
        }
    }
    m_StateFlags = flags;
    switch (pProfile->m_Hash) {
    case 0x470000ae:
    case 0x470000af:
    case 0x470000b0:
    case 0x470000b1:
    case 0x47000107:
    case 0x4700026e:
    case 0x470002ce:
    case 0x470002cf:
    case 0x470002d9:
    case 0x47000373:
    case 0x47000374:
    case 0x47000376:
    case 0x4700037a:
        c = ex::RGBA(0, 0, 0, 0x80);
        m_ScanText = 0x510000f4;
        m_StateFlags.m_bBrainScan = 0;
        break;
    case 0x4700035f:
        if (gCurrentMap_GeoHash == 0x1000124 || gCurrentMap_GeoHash == 0x100013b) {
            c = ex::RGBA(0, 0x7f, 0, 0x80);
            m_ScanText = 0x5100099f;
            m_StateFlags.m_bBrainScan = 0;
        }
        break;
    }
    if (pSkin->m_Hash == 0xd000104) {
        c = ex::RGBA(0, 0, 0, 0x80);
        m_StateFlags.m_bBrainScan = 0;
        m_ScanText = 0x510000f4;
    }
    if (!bFade || c == m_BrainScanColour) {
        m_BrainScanColour = c;
    } else {
        int step = (int)(EXBaseApp::GameFrameRateMul() * 2.0f);
        if (c.m_R > m_BrainScanColour.m_R + step) m_BrainScanColour.m_R += step;
        else if (c.m_R < m_BrainScanColour.m_R - step) m_BrainScanColour.m_R -= step;
        else m_BrainScanColour.m_R = c.m_R;
        if (c.m_G > m_BrainScanColour.m_G + step) m_BrainScanColour.m_G += step;
        else if (c.m_G < m_BrainScanColour.m_G - step) m_BrainScanColour.m_G -= step;
        else m_BrainScanColour.m_G = c.m_G;
        if (c.m_B > m_BrainScanColour.m_B + step) m_BrainScanColour.m_B += step;
        else if (c.m_B < m_BrainScanColour.m_B - step) m_BrainScanColour.m_B -= step;
        else m_BrainScanColour.m_B = c.m_B;
    }
    return GetBrainScanColour();
}

// Vision check against the player. The player can be seen when in line of sight within the
// attack range (or while a forced sighting runs); a cloaked player only when the actor can see
// through cloaks (sub group, a tagged player for missile soldiers, the override timer, touching
// distance) or, inside the profile's cloak distance, when the player moves.
void Actor::CheckSeePlayer()
{
    if (m_Health <= 0.0f) {
        m_StateFlags.m_bPlayerSeen = 0;
        m_ActorFlags2.m_bCouldPlayerBeSeen = 0;
        return;
    }
    m_StateFlags.m_bPlayerSeen = 0;
    m_ActorFlags2.m_bCouldPlayerBeSeen = 0;
    float range = GetProfile()->m_AttackRange;
    s8 bSeeCloak = 0;
    if (SubGroupAnims::GetSubGroupAnims(m_SubGroupAnimIndex).m_Anims[8]) bSeeCloak = 1;
    if (gAIControl.m_PlayerTaggedTimer > 0.0f && GetProfile()->m_SubGroup == 0x4f00002d) bSeeCloak = 1;
    if (gTimer_OverridePlayerCloak > 0.0f) bSeeCloak = 1;
    float touch = m_CollisionRadius + 0.54f;
    const EXMaths::vec_xyzw &d = GetFeetPos() - Player_GetFeetPos();
    EXVector w = d;
    if (w.y * w.y + w.z * w.z + w.x * w.x <= touch * touch) bSeeCloak = 1;
    if (!ActorOptionsManager::PlayerVisible()) return;
    EXVector head = Player_GetHeadPos();
    m_ActorFlags2.m_bCouldPlayerBeSeen = CanSeePoint(head, range) != 0;
    if (m_ForceSightingTimer > 0.0f) {
        m_ActorFlags2.m_bCouldPlayerBeSeen = 1;
        bSeeCloak = 1;
    }
    float cloakDist;
    switch (GetProfile()->m_NudgeType) {
    case 0x50000001:
        cloakDist = 4.0f;
        break;
    case 0x50000002:
        cloakDist = 4.0f;
        break;
    default:
        cloakDist = 1.5f;
        break;
    }
    const EXMaths::vec_xyzw &h = GetHeadPos() - head;
    EXVector hw = h;
    if ((hw.y * hw.y + hw.z * hw.z + hw.x * hw.x < cloakDist * cloakDist &&
         (bSeeCloak || (Player_IsCloaked() && XItemHandler_Player::Instance()->GetSpeed() > 0.02f))) ||
        bSeeCloak || !Player_IsCloaked()) {
        m_StateFlags.m_bPlayerSeen = m_ActorFlags2.m_bCouldPlayerBeSeen;
        if (m_StateFlags.m_bPlayerSeen) gAIControl.SetLastSeenPlayerPos(Player_GetFeetPos());
    }
}

// Starts a jump queued by a jump trigger: snaps the feet to the jump polygon, works out the
// landing spot and sets the take-off velocities (same ballistics as JumpToPos).
signed char Actor::ProcessTriggerJump()
{
    if (m_ActorFlags.m_bBeingJuggled || !m_NavRoute.m_Flags.m_bTriggerJump) return 0;
    EXVector from = GetFeetPos();
    m_OrgLoc.GetMesh()->SnapToPolygonBounds(from, m_NavRoute.m_JumpNode);
    m_OrgLoc.SetPosPoly(from, m_NavRoute.m_JumpNode, 0);
    m_NavRoute.m_JumpFromPos = from;
    EXVector to;
    m_NavRoute.CalcJumpToPos(m_OrgLoc.GetMesh(), from, &m_NavRoute.m_JumpFromNode, m_NavRoute.m_MoveVector, to);
    m_NavRoute.m_JumpToPos = to;
    EXVector d = to - from;
    EXMaths::vec_xyzw dv = d;
    float horiz = EXSqrt(dv.x * dv.x + dv.z * dv.z);
    float apex = horiz * 0.5f;
    float up = d.y;
    if (up >= 0.0f) up += 0.5f;
    float vUp;
    if (apex > fabs(up)) vUp = EXSqrt(apex * 0.0166f);
    else if (up > 0.0f) vUp = EXSqrt(up * 0.0166f);
    else vUp = 0.0f;
    float vDown = EXSqrt(vUp * vUp - d.y * 0.0166f);
    m_WorldVelocity.y = vUp;
    float yaw = (float)atan2(d.x, d.z);
    float speed = horiz / (((0.0f - vDown) - vUp) * 120.48193f);
    m_LocalVelocity.Set(0.0f, 0.0f, 0.0f - speed, 0.0f);
    m_NavRoute.m_TriggerJump = 0;
    m_NavRoute.m_fJumpTime = yaw;
    m_NavRoute.m_Flags.m_bTriggerJump = 0;
    m_NavRoute.m_Flags.m_b7 = 1;
    m_NavRoute.m_Flags.m_bJumpCancel = 0;
    return 1;
}

// Moves a ripped off head to pos (the player's hand). The held head item follows; the head
// physics chain gets body 0 placed and stopped, and bodies 1 and 2 are dragged along by the
// part of the move beyond 0.2.
void Actor::SetRippedHeadPosRot(const EXVector &pos, const EXQuaternion &rot)
{
    if (ms_pLimbInPlayersHand) {
        EXVector r;
        rot.GetRotation(r, 4);
        ms_pLimbInPlayersHand->SetPosition(pos);
    }
    if (!m_pHeadPhysics || m_pHeadPhysics->BodyCount() <= 0 || !m_pHeadPhysics->Body(0)) return;
    EXVector old = Actor_GetBody(m_pHeadPhysics, 0)->m_pos;
    Actor_GetBody(m_pHeadPhysics, 0)->m_pos = pos;
    ((EX6DRigidBody *)Actor_GetBody(m_pHeadPhysics, 0))->m_rot.Set(0.0f, 0.0f, 0.0f, 1.0f);
    Actor_GetBody(m_pHeadPhysics, 0)->m_velocity.SetZero();
    ((EX6DRigidBody *)Actor_GetBody(m_pHeadPhysics, 0))->m_angVel.SetZero();
    EXVector d = pos - old;
    float len = EXSqrt(d.y * d.y + d.z * d.z + d.w * d.w + d.x * d.x);
    if (len > 0.2f) {
        d *= (len - 0.2f) / len;
        Actor_GetBody(m_pHeadPhysics, 1)->m_pos = Actor_GetBody(m_pHeadPhysics, 1)->m_pos + d;
        Actor_GetBody(m_pHeadPhysics, 2)->m_pos = Actor_GetBody(m_pHeadPhysics, 2)->m_pos + d;
    }
}

// Kills the actor and blows off both arms and the head, each thrown outwards from the body
// (arms along the hand direction, the head roughly upwards) at a random 20..30 speed.
void Actor::ExplodeLimbs(const EXVector &pos, signed char bOverkill)
{
    if (!CanLooseLimbs()) return;
    SetHealth(0.0f, 0);
    if (bOverkill) m_ActorFlags.m_bOverkilled = 1;
    for (int i = 0; i < 4; i++) {
        if (i == 3) continue;
        EXVector dir;
        switch (i) {
        case 0: {
            EXDatumTemp d;
            memset(&d, 0, sizeof(d));
            m_pItem->GetAnimatorDatum(0x10000013, d, 0, 0);
            dir = d.m_position - GetFeetPos();
            break;
        }
        case 1: {
            int hash = GetSkin()->m_Hash == 0xd0000b9 ? 0x10000039 : 0x10000014;
            EXDatumTemp d;
            memset(&d, 0, sizeof(d));
            m_pItem->GetAnimatorDatum(hash, d, 0, 0);
            dir = d.m_position - GetFeetPos();
            break;
        }
        default: {
            float x = EXRandf() * 0.6f - 0.3f;
            float z = EXRandf() * 0.6f - 0.3f;
            dir.Set(x, 1.0f, z, 0.0f);
            break;
        }
        }
        dir.Normalise();
        dir.y = EXRandf() + 1.0f;
        dir.Normalise();
        dir *= EXRandf() * 10.0f + 20.0f;
        CreateLostLimb((ActorLostLimb)i, dir, 0, 0);
    }
}

// Line of sight from the actor's head to pos within range (0 = the profile's attack range).
// The point must be within the eye height band, inside the view cone (wide close up,
// narrowing with distance) unless the sub group sees all round or the actor is fighting the
// player close by, and two ray casts at a slightly jittered target must get through.
signed char Actor::CanSeePoint(const EXVector &pos, float range)
{
    float maxHeight = 5.5f;
    if (range == 0.0f) range = GetProfile()->m_AttackRange;
    if (m_ActorFlags.m_bInCombatWithPlayer) maxHeight = 10.0f;
    if (m_ActorFlags2.m_bLongRangeVision) {
        range *= 5.0f;
        maxHeight = 1000.0f;
    }
    if (GetProfile()->m_SeeAnyHeight == 1) maxHeight = 1000.0f;
    if (fabs(pos.y - GetHeadPos().y) > maxHeight) return 0;
    EXVector d = pos - GetHeadPos();
    EXVector w = d;
    float distSq = w.y * w.y + w.z * w.z + w.x * w.x;
    if (distSq > range * range) return 0;
    s8 bClose = 0;
    if (m_ActorFlags.m_bInCombatWithPlayer && 25.0f > distSq) bClose = 1;
    if (!SubGroupAnims::GetSubGroupAnims(m_SubGroupAnimIndex).m_Anims[9] && !bClose) {
        float cosLimit;
        if (m_Unk86c * m_Unk86c > distSq) {
            cosLimit = m_Unk864;
        } else {
            float t = (EXSqrt(distSq) - m_Unk86c) / (range - m_Unk86c);
            cosLimit = (1.0f - t) * m_Unk864 + m_Unk868 * t;
        }
        EXVector look = GetHeadDirection();
        EXMaths::vec_xyzw flat;
        flat.set(w.x, 0.0f, w.z, 0.0f);
        flat.Normalise();
        if (look.Dot(flat) <= cosLimit) return 0;
    }
    EXRayCastTemp ray;
    EXVector head = GetHeadPos();
    EXVector jitter;
    jitter.y = EXRandf() * 2.0f - 1.0f;
    jitter.x = EXRandf() * 2.0f - 1.0f;
    jitter.z = EXRandf() * 2.0f - 1.0f;
    jitter.w = 0.0f;
    EXVector target = pos + jitter * 0.2f;
    ray.Set(head, target - head, 0, 0, 0, 0);
    Actor_CalledRayCast++;
    ray.m_mask = 8;
    theItemEnv.DoRayCastToContoured(ray);
    if (ray.m_bHit) return 0;
    ray.Set(head, target - head, 0, 0, 0, 0);
    ray.m_mask = 8;
    Actor_CalledRayCast++;
    theItemEnv.DoRayCastToMap(ray, 0);
    return !ray.m_bHit;
}

// EXPlacementItem::SetRotation from euler angles, forced inline as in CreateLostLimb.
static __forceinline void Actor_SetRotation(XItem *pItem, const EXVector &r)
{
    pItem->m_rotation = EXVector(r.x, r.y, r.z, r.w);
    pItem->Invalidate();
}

// Severs a limb: counts the stat, drops what the arm carried, spawns the limb item from the
// skin's limb model (flung along dir, spinning) unless bNoItem, attaches a blood effect to the
// stump and swaps in the stump texture. A head ripped off by the player (bHeld) is handed to
// the player and, for skull rips, gets its own physics chain.
void Actor::CreateLostLimb(ActorLostLimb limb, const EXVector &dir, signed char bHeld, signed char bNoItem)
{
    if (limb < 0 || limb >= 4) return;
    if (m_LostLimbs[limb].m_Flags & 1) return;
    switch (limb) {
    case ALL_HEAD:
        m_SpeechTag.RemoveAll(1);
        XGetGameStats()->m_Unk4c[1]++;
        break;
    case ALL_LEFTARM:
    case ALL_RIGHTARM:
        XGetGameStats()->m_Unk4c[2]++;
        break;
    case ALL_BELT:
        XGetGameStats()->m_Unk4c[3]++;
        break;
    }
    m_LostLimbs[limb].m_bHeld = bHeld != 0;
    m_LostLimbs[limb].m_Life = bHeld ? 600.0f : 120.0f;
    switch (limb) {
    case ALL_LEFTARM:
        m_ActorFlags.m_bLeftArmMissing = 1;
        SetCarry(0, 1);
        break;
    case ALL_RIGHTARM:
        m_ActorFlags.m_bRightArmMissing = 1;
        SetCarry(0, 0);
        break;
    }
    ActorSkin *pSkin = GetSkin();
    int script;
    int hash;
    float side;
    switch (limb) {
    case ALL_LEFTARM:
        hash = pSkin->m_LeftArm;
        side = -1.0f;
        break;
    case ALL_RIGHTARM:
        hash = pSkin->m_RightArm;
        side = 1.0f;
        break;
    case ALL_HEAD:
        hash = pSkin->m_Head;
        side = 0.0f;
        break;
    default:
        goto pick;
    }
    {
        int file = pSkin->m_LimbFile;
        if (!hash || bNoItem) goto pick;
        m_LostLimbs[limb].m_Id = (int)XItem::CreateItem(0, 0, 0, EXVector(0.0f, 0.0f, 0.0f, 0.0f));
        if (!m_LostLimbs[limb].m_Id) goto pick;
        XItem *pItem = (XItem *)m_LostLimbs[limb].m_Id;
        pItem->RenderInfo().SetExtRenderInfoInl(&m_ExtRenderInfo);
        ((XItem *)m_LostLimbs[limb].m_Id)->RenderInfo().SetTexSwapInfoInl(&m_TexSwapInfo);
        m_LostLimbs[limb].m_Vel = (dir + EXVector(side, 1.0f, 0.0f, 0.0f)) * 0.005f;
        m_LostLimbs[limb].m_Alpha = 1.0f;
        float ry = EXRandf() * 0.4f - 0.2f;
        float rx = EXRandf() * 0.4f - 0.2f;
        float rz = EXRandf() * 0.4f - 0.2f;
        m_LostLimbs[limb].m_RotVel = EXVector(rx, ry, rz, 0.0f);
        ((XItem *)m_LostLimbs[limb].m_Id)->SetPosition(GetFeetPos());
        Actor_SetRotation((XItem *)m_LostLimbs[limb].m_Id, m_pItem->Rotation());
        ((XItem *)m_LostLimbs[limb].m_Id)->RenderInfo().SetExtRenderInfoInl(&m_ExtRenderInfo);
        EXDatumTemp d;
        memset(&d, 0, sizeof(d));
        Actor_CalledGetDatum++;
        if (m_pItem->GetAnimatorDatum(s_LimbInfo[limb].m_Datum, d, 0, 0)) {
            ((XItem *)m_LostLimbs[limb].m_Id)->SetPosition(d.Position());
            EXVector r;
            d.m_rotation.GetRotation(r, 4);
            Actor_SetRotation((XItem *)m_LostLimbs[limb].m_Id, r);
        }
        ((XItem *)m_LostLimbs[limb].m_Id)->SetMapOn(theItemEnv.GetMapOn(((XItem *)m_LostLimbs[limb].m_Id)->Position()));
        theItemEnv.AddItem((XItem *)m_LostLimbs[limb].m_Id, 0);
        EXItemAnimator_Entity *pEnt = EXItemAnimator_Entity::NewItem();
        EXGeoFile *pFile = EXGeoFile::GetGeoFile(file);
        if (pEnt && pFile) {
            pEnt->Initialise(pFile, hash);
            ((XItem *)m_LostLimbs[limb].m_Id)->AddAnimator(pEnt);
        }
        if (limb != ALL_HEAD) goto pick;
        if (bHeld && (ShouldBeSkullRipped() || m_ActorFlags.m_bUlysses)) {
            EXItemAnimator_AnimModifier *pMod = EXItemAnimator_AnimModifier::NewItem();
            EXGeoFile *pFx = EXGeoFile::GetGeoFile(0x1000016);
            if (pMod && pFx) {
                if (m_ActorFlags2.m_bPredator) pMod->Initialise(pFx, 0x3000030);
                else pMod->Initialise(pFx, 0x3000010);
                m_pHeadPhysics = new EX6DAnim;
                m_pHeadPhysics->m_pItem = (EXItem *)m_LostLimbs[ALL_HEAD].m_Id;
                ((XItem *)m_LostLimbs[ALL_HEAD].m_Id)->SetItemPhysics(m_pHeadPhysics);
                m_pHeadPhysics->m_flags.BitSet(4);
                m_pHeadPhysics->AutoRagdollInitialise(pMod, 3, 0, 0, 0.0f);
                EX6DAnim *p = m_pHeadPhysics;
                if (p->Body(0)) p->Body(0)->SetMass(100.0f);
                theItemEnv.m_p6DManager->AddBody(m_pHeadPhysics);
                ((XItem *)m_LostLimbs[ALL_HEAD].m_Id)->AddAnimator(pMod);
            }
        }
        goto jet;
    }
pick:
    if (limb <= ALL_RIGHTARM) {
        script = 0x4000329;
        goto blood;
    }
    if (limb == ALL_BELT) {
        script = 0x4000078;
        goto blood;
    }
jet:
    script = 0x4000396;
blood:
    m_LostLimbs[limb].m_pBlood = CreateBloodScript(s_LimbInfo[limb].m_Bone, script, (XItem *)m_LostLimbs[limb].m_Id);
    if (m_LostLimbs[limb].m_Flags & 2) {
        if (ms_iActorIDPlayerHasHead) {
            Actor *p = GetPtrFromID(ms_iActorIDPlayerHasHead);
            if (p) p->DeleteLostLimb(ALL_HEAD);
        }
        XItem *pHead = (XItem *)m_LostLimbs[limb].m_Id;
        int id = m_RefID;
        DeleteHeldHead();
        ms_iActorIDPlayerHasHead = id;
        ms_pLimbInPlayersHand = pHead;
    }
    m_LostLimbs[limb].m_Flags |= 1;
    ActorTextureSwap::ApplyLostLimbs(this);
}

// Puts a created actor into the world. Time table spawns that would appear within 10 of the
// player are moved to a spawn point out of sight (or deleted if there is none) and fade in.
// Then the skin's animator, render links, carried weapons, armour, collision size, kind flags,
// navigation flags, start state, cloak and brain scan colour are set up.
void Actor::AddToWorld()
{
    if (m_CreationFlags & 0x20) {
        EXVector d = XItemHandler_Player::Instance()->GetItem()->Position() - GetFeetPos();
        if (d.Length() < 10.0f) {
            XNavLocation loc;
            if (TimeTable_GenerateSpawnPos(XItemHandler_Player::Instance()->GetItem()->Position(), loc)) {
                SetPosition(loc.m_vPos, loc.m_nPolyHandle);
                if (m_CreationFlags & 0x10) m_SM.ExtGoto(0, STATE_CityDistractions, 0, 0, 0, 0);
            } else {
                Delete();
                return;
            }
        }
    }
    if (m_CreationFlags & 0x20) {
        m_Alpha = 0.0f;
        m_AliveRadius = 40.0f;
    } else {
        m_Alpha = 1.0f;
        m_AliveRadius = 1200.0f;
    }
    theItemEnv.AddItem(m_pItem, 0);
    ActorSkin *pSkin = GetSkin();
    if (pSkin->m_Height > 0.0f && 2.0f > pSkin->m_Height) {
        float s = pSkin->m_Height;
        m_pItem->SetScale(EXVector(s, s, s, 0.0f));
    }
    EXItemAnimator_AnimModifier *pAnim = new EXItemAnimator_AnimModifier;
    if (pAnim) {
        ActorSkin *p = GetSkin();
        AU_AssignSkinSkelAnim(pAnim, p->m_File, p->m_Hash, p->m_SkelFile, p->m_SkelHash, p->m_ChangeFile, p->m_ChangeHash);
        m_pItem->AddAnimator(pAnim);
        pAnim->InitControllerStack(0, 6, 1);
        if (m_Unk7a8) {
            EXItemCtrl *pCtrl = pAnim->Controller(0);
            pCtrl->m_ctrlFlags.BitSet(m_Unk7a8);
            ((EXItemCtrl_Anim *)pCtrl)->m_motionFlags = (u16)m_AnimMotionOverride;
        }
        pAnim->SetFrameRate60((float)EXBaseApp::NominalGameFrameRate());
        pAnim->RenderInfo().SetExtRenderInfoInl(&m_ExtRenderInfo2);
        m_StateFlags.m_b8 = 0;
        m_HeadTrackMode = 0;
    }
    SetAnimMode(GetSkin()->m_StartAnimMode, 0);
    m_pItem->RenderInfo().SetExtRenderInfoInl(&m_ExtRenderInfo);
    m_pItem->RenderInfo().SetTexSwapInfoInl(&m_TexSwapInfo);
    m_pAnim = (EXItemAnimator_Anim *)m_pItem->GetAnimator(EXRUNTIME_CLASS(EXItemAnimator_Anim));
    AU_SetItemARGB(m_pItem, 0.0f, 1.0f, 1.0f, 1.0f);
    AU_SetItemDraw(m_pItem, 0);
    int weapon = TranslateCarryHashCode(GetProfile()->m_Weapon);
    if (weapon) {
        int id = ActorCarryable::Create(0x100003c, weapon)->m_RefID;
        ActorCarryable *pOld = ActorCarryable::GetPtrFromID(m_CarryRight);
        if (pOld) pOld->SetOwner(0);
        ActorCarryable *pNew = ActorCarryable::GetPtrFromID(id);
        if (pNew) pNew->SetOwner(m_RefID);
        m_CarryRight = id;
        s8 bArmed = m_CarryLeft || id ? 1 : 0;
        m_Attrib &= ~0x10;
        if (bArmed) m_Attrib |= 0x10;
    }
    int weapon2 = TranslateCarryHashCode(GetProfile()->m_Weapon2);
    if (weapon2 && weapon2 != 0x200016e) SetCarry(ActorCarryable::Create(0x100003c, weapon2)->m_RefID, 1);
    if (GetProfile()->m_Armour) m_ArmourIndex = ArmourProfile::GetArmourDataIndex(GetProfile()->m_Armour);
    else m_ArmourIndex = ArmourProfile::GetArmourDataIndex(0);
    EXDatumTemp d;
    if (m_pItem->GetAnimatorDatum(0x10000002, d, 0, 0)) {
        m_CollisionRadius = d.Radius();
        if (m_CollisionRadius > ms_fMaxCollisionRadius) ms_fMaxCollisionRadius = m_CollisionRadius;
    }
    m_ActorFlags.m_bBoss = GetSkin()->m_SkelHash == 0xd000091;
    m_ActorFlags.m_bUlysses = GetProfile() ? GetProfile()->m_SubGroup == 0x4f000019 : 0;
    m_ActorFlags.m_bAlien = GetSkin()->m_SkelHash == 0xd000103;
    m_ActorFlags2.m_bPredator = GetSkin()->m_File == 0x10000c8;
    if (m_ActorFlags.m_bBoss) m_CollisionSphereRadius = 1.3f;
    else if (m_ActorFlags.m_bUlysses) m_CollisionSphereRadius = 2.0f;
    else if (m_ActorFlags.m_bAlien) m_CollisionSphereRadius = 1.5f;
    else m_CollisionSphereRadius = 1.0f;
    if ((m_ActorFlags.m_bBoss == 1 && !m_NavRoute.NoJump()) || gNavMeshHandler.IsDynamicNavigation()) {
        m_NavRoute.m_Flags.m_bDynamic = 1;
        m_NavRoute.m_NavIgnoreFlags = 8;
        if (!m_ActorFlags.m_bBoss || m_NavRoute.NoJump()) m_NavRoute.m_NavIgnoreFlags = 0x48;
    }
    if (m_SM.m_State == STATE_ActorInit) {
        if (m_ActorFlags.m_bAlien) {
            m_SM.Init(*this, ActorSpecial_Register(this));
        } else {
            float h = gCurrentMap_GeoHash != 0x100011a ? GetFeetPos().y - m_OrgLoc.m_vFloorPos.y : 0.0f;
            if (fabs(h) < 1.0f) m_SM.Init(*this, ActorSpecial_Register(this));
            else m_SM.Init(*this, STATE_FreeFall);
        }
    }
    SubGroupAnims &sub = SubGroupAnims::GetSubGroupAnims(*this);
    if (sub.m_Anims[6]) SetCloakDirect(1);
    m_ActorFlags.m_bVariableSpeed9 = sub.m_Anims[14] != 0;
    m_BrainScanColour = UpdateBrainScanColour(0);
    GetActorBasePtr(m_RefID)->m_bInWorld = 1;
    SetHide(m_StateFlags.m_bHide);
    ActorTextureSwap::Initialise(this);
    if (!GetSkin()->m_Type) {
        int walks[3];
        walks[0] = 0;
        walks[1] = 0x900026c;
        walks[2] = 0x9000303;
        m_WalkAnim = walks[EXRand32() % 3];
    }
    CalcMass();
    CalculateParameters();
    CheckForSkinChange(0.0f);
}

// A vehicle's footprint (four XZ corners) touched the actor. The nearest footprint edge within
// the collision radius gives the hit point and outward normal. A standing living actor hit
// side-on or by a slow or scripted vehicle is shoved out of the footprint; otherwise it is
// killed (rag doll or explosion death). Anyone else on screen just goes rag doll with a thud.
void Actor::HitByVehicle(Vehicle *pVehicle, EXFixedArray<EXVector2, 4> &box)
{
    s8 bHit = 0;
    float best = 99999.0f;
    EXVector hit;
    EXVector normal;
    for (int i = 0; i < 4; i++) {
        const EXVector2 &a = box[i];
        const EXVector2 &b = box[(i + 1) % 4];
        EXLineSeg seg;
        seg.m_Direction = EXVector(b.x - a.x, 0.0f, b.y - a.y, 0.0f);
        seg.m_Origin = EXVector(a.x, 0.0f, a.y, 0.0f);
        EXVector feet = GetFeetPos();
        feet.y = 0.0f;
        float t = 0.0f;
        float d = EXSqrt(EXSqrDistance(feet, seg, &t));
        if (d <= best && d <= m_CollisionRadius) {
            hit = seg.m_Origin + seg.m_Direction * t;
            bHit = 1;
            EXVector e = seg.m_Direction;
            e.Normalise();
            normal.Set(0.0f - e.z, 0.0f, e.x, 0.0f);
            best = d;
        }
    }
    if (!bHit) return;
    const EXVector3 &v = pVehicle->getMove().m_phys.m_velocity;
    EXVector vel(v.x, v.y, v.z, 0.0f);
    float speed = vel.Length();
    vel.Normalise();
    m_pHitByVehicle = pVehicle;
    m_HitByVehiclePos = pVehicle->m_pItem->Position();
    float facing = (float)fabs(vel.Dot(normal));
    if (!m_StateFlags.m_bLyingOnGround && m_Health > 0.0f && !m_ActorFlags2.m_bOnFire) {
        if (!(0.75f > facing) && !(0.001f >= speed) && !GetObjective()) {
            if (IsRagDollAllowed()) {
                EnableRagDoll();
                m_SM.ExtGosub(0, STATE_RagDollDeath, 0, 0, 0, 0);
            } else {
                SetRotY((float)atan2(normal.x, normal.z));
                SetAnimMode(0x900004c, 0);
                m_ActorFlags2.m_bIgnoreAnimChanges = 1;
                m_SM.ExtGosub(0, STATE_AnimDeath, 0, 0, 0, 0);
            }
            Speak(GetScreamSFX(), 0);
            m_StateFlags.m_bHitByCar = 1;
            return;
        }
        EXVector pos = hit + normal * m_CollisionRadius;
        m_pItem->SetPosition(pos);
        m_OrgLoc.SetPosPoly(pos, -1, 0);
        if (!SearchBrainStateStack(STATE_HitByVehicle)) m_SM.ExtGosub(0, STATE_HitByVehicle, 0, 0, 0, 0);
        return;
    }
    if (gRagDollCount >= 3) return;
    if (m_ActorFlags.m_bBoss || m_ActorFlags.m_bUlysses) return;
    if (!m_StateFlags.m_bOnScreen) return;
    EnableRagDoll();
    m_SM.ExtGosub(0, STATE_RagDollDeath, 0, 0, 0, 0);
    unsigned int sfx = RemapSFX(0x1af000de);
    if (sfx) {
        EXSoundHandle *h = m_SFXTag.StartSound(sfx, GetHeadPos(), 5.0f, 50.0f, -1, 0);
        if (h) h->SetTrackingSound(0);
        switch (sfx) {
        case 0x1af00104:
        case 0x1af00106:
        case 0x1af0010f:
        case 0x1af00135:
        case 0x1af0014f:
        case 0x1af0028c:
            h->SetLoopingSound(1);
            break;
        default:
            h->SetLoopingSound(0);
            break;
        }
    }
    unsigned int scream = RemapSFX(GetScreamSFX());
    if (scream) {
        if (m_SpeechTag.IsPlaying()) {
            if (m_SpeechTag.IsPlaying(scream)) goto done;
            m_SpeechTag.RemoveAll(1);
        }
        EXSoundHandle *h = m_SpeechTag.StartSound(scream, GetHeadPos(), -1, 0);
        if (h) h->SetLoopingSound(0);
    }
done:
    m_StateFlags.m_bHitByCar = 1;
}

// Aims the head and gun at pos. Lobbing weapons (the drone grenade launcher and fire type 3)
// aim 1000 units out along a raised line instead. The vertical and horizontal angles to the
// target (clamped to 45 degrees) drive the aiming target state: 1 and 4 vertical, 2 and 3
// horizontal, with m_AimingTime blending towards the angle or decaying while switching over.
void Actor::SetAimingPos(EXVector pos)
{
    m_StateFlags.m_bAimingPos = 1;
    m_AimingPos = pos;
    if (m_ActorFlags.m_bBoss || m_ActorFlags.m_bUlysses) return;
    WeaponProfile *pWeapon = GetWeaponProfile(0);
    if (!pWeapon) return;
    if (pWeapon->m_Id == 0x49000024 || pWeapon->m_FireType == 3) {
        EXVector dir = pos - m_pItem->m_position;
        dir.y = 0.0f;
        Actor_Normalise(dir);
        dir.y = 0.7f;
        Actor_Normalise(dir);
        pos = m_pItem->m_position + dir * 1000.0f;
    }
    EXVector hand = GetHandPos(0);
    EXVector dir = pos - hand;
    Actor_Normalise(dir);
    float v = qasinf(dir.y) * 57.29578f;
    if (v > 45.0f) v = 45.0f;
    else if (v < -45.0f) v = -45.0f;
    float r = m_pItem->m_rotation.y;
    EXVector facing(qsinf(r), 0.0f, qcosf(r), 0.0f);
    Actor_Normalise(facing);
    dir.y = 0.0f;
    Actor_Normalise(dir);
    float dot = facing.Dot(dir);
    if (dot > 1.0f) dot = 1.0f;
    else if (dot < -1.0f) dot = -1.0f;
    float h = qacosf(dot);
    if (facing.Dot(EXVector(dir.z, 0.0f, 0.0f - dir.x, dir.w)) > 0.0f) h = 0.0f - h;
    h *= 57.29578f;
    if (h > 45.0f) h = 45.0f;
    else if (h < -45.0f) h = -45.0f;
    float k = XApp::GameFrameRateMul() * 0.03f;
    switch (m_AimingTarget) {
    case 1:
        if (fabs(v) < 10.0f) {
            m_AimingTarget = 4;
            SetAimingAnim();
            return;
        }
        SpinToYRot(pos);
        m_AimingTime = (1.0f - k) * m_AimingTime + k * v;
        SetAimingAnim();
        return;
    case 2:
        if (fabs(v) > 10.0f) {
            m_AimingTarget = 3;
            SetAimingAnim();
            return;
        }
        m_AimingTime = (1.0f - k) * m_AimingTime + k * h;
        SetAimingAnim();
        return;
    case 3:
        if (fabs(v) < 10.0f) break;
        SpinToYRot(pos);
        m_AimingTime = (1.0f - k) * m_AimingTime;
        if (m_AimingTime < 5.0f) {
            m_AimingTarget = 1;
            SetAimingAnim();
            return;
        }
        SetAimingAnim();
        return;
    case 4:
        if (fabs(v) > 10.0f) {
            m_AimingTarget = 1;
            SetAimingAnim();
            return;
        }
        m_AimingTime = (1.0f - k) * m_AimingTime;
        if (m_AimingTime < 5.0f) break;
        SetAimingAnim();
        return;
    case 0:
        m_AimingTime = 0.0f;
        if (fabs(h) <= 22.5f && fabs(v) > 10.0f) {
            m_AimingTarget = 1;
            SetAimingAnim();
            return;
        }
        break;
    default:
        SetAimingAnim();
        return;
    }
    m_AimingTarget = 2;
    SetAimingAnim();
}

// A rag doll that slows down sharply has hit something: it damages hitable script objects it
// is touching, and a hard hit (or any object hit) makes a loud noise and a thud. A drone with
// a petrol bomb in its profile thrown well away from the player explodes. While moving, it
// also knocks into other actors near its middle: big skins (types 4, 7, 10) bounce it away,
// anyone else takes Dead_Bloke damage.
void Actor::DamageOtherActorsIfRagdoll()
{
    if (!m_StateFlags.m_bRagDoll) return;
    float speedSq = GetRagDollVelocity().LengthXYZSquared();
    float speed = speedSq >= 0.0f ? (float)sqrt(speedSq) : 0.0f;
    float decel = m_RagDollSpeed - speed;
    if (decel > 1.0f) {
        signed char bHit = 0;
        EXDatumTemp d;
        if (!m_pItem->GetAnimatorDatum(0xe000001, d, 0, 0)) return;
        EXVector pos = d.m_position;
        for (XItem *pI = gHitableList.GetFirst(); pI; pI = gHitableList.GetNext(pI)) {
            XItemHandler_ScriptRunner *pH = (XItemHandler_ScriptRunner *)pI->ItemHandler();
            EXVector diff = pos - pH->HitCentre();
            float r = pH->m_HitRadius + 0.8f;
            if (diff.LengthXYZSquared() < r * r) {
                pH->ApplyDamage(50.0f, 0x53000000);
                bHit = 1;
            }
        }
        if (decel > 1.5f) {
            EXVector diff = Player_GetFeetPos() - pos;
            if (diff.LengthXYZSquared() > 25.0f && m_ActorFlags.m_bHasBeenThrow && PetrolBombInProfile()) {
                WeaponProfile *pWeapon = WeaponProfile::GetWeaponData(0x49000025);
                XItemHandler_ScriptBase::Explosion_Create(pos, pWeapon->m_ExplosionScript,
                    (int)pWeapon->m_MineTriggerRadius, (int)pWeapon->GetBulletDamage(), 0, 0x49000025, 1, m_RefID, 0);
                SetHasBeenThrow(0);
            }
        } else if (!bHit) {
            goto done;
        }
        SendMsg_LoudNoise(m_pItem->m_position, 15.0f, this, 0, 0);
        XGetSoundManager()->StartSound(0x1af000af, m_pItem->m_position, -1, 0);
    }
done:
    m_RagDollSpeed = speed;
    AU_PlayerForceSightingInArea(GetPelvisPos(), 8.0f);
    if (speedSq < 0.01f) return;
    EXVector centre = (GetHeadPos() + m_pItem->m_position) * 0.5f;
    for (Actor *pA = GetFirst(centre, 4.0f, 0); pA; pA = pA->GetNext(centre, 4.0f, 0)) {
        if (pA == this || pA->IsDead() || !pA->IsHitable()) continue;
        EXVector other = (pA->GetHeadPos() + pA->m_pItem->m_position) * 0.5f;
        float rangeSq = 1.0f;
        if (pA->GetSkin()->m_Type == 7) rangeSq = 9.0f;
        EXVector diff = other - centre;
        if (diff.LengthXYZSquared() > rangeSq) continue;
        if (pA->SearchBrainStateStack(STATE_HitReactKnockedOver)) continue;
        int type = pA->GetSkin()->m_Type;
        if (type == 4 || type == 7 || type == 10) {
            EXVector away = centre - other;
            away.y = 0.0f;
            Actor_Normalise(away);
            EX6DAnim *p = (EX6DAnim *)m_pItem->m_pItemPhysics;
            Actor_GetBody(p, 0)->Velocity() = away * 5.0f;
        } else {
            EXVector dir = other - centre;
            WeaponProfile *pWeapon = WeaponProfile::GetWeaponData(0x49000039);
            if (!pWeapon) continue;
            if (pA->m_Unk898 <= 0.0f) {
                pA->PlaySFX(0x1af000ae, 0);
                pA->m_Unk898 = (float)EXBaseApp::NominalGameFrameRate() * 2.0f;
            }
            SendMsg_AttackedByActor(pA, pWeapon, centre, dir, pWeapon->GetBulletDamage(), this, 0);
        }
    }
}

// EXRand32() & 1; retail spills it to the stack before testing it (see ResetCivilianAttackVO).
static __forceinline int Actor_RandBit()
{
    const int &r = EXRand32() & 1;
    return r;
}

// One in ten calls, when the shared combat VO timer has run out and the camera is within 30
// units, picks a combat line for the actor's VO type. With a target the line is about that
// target (only human targets for most types) and the call returns 0; without one it is a
// general line (often a different one after recently hitting the predator) and returns 1.
// Many lines have a second take used half the time when an ally is within 10 units.
signed char Actor::TryToSpeakCombatVO(Actor *pTarget)
{
    if (EXRand32() % 10 != 0 || !gAIControl.CanSpeakCombatVO()) return 0;
    EXVector d = XGetCamera(0)->GetItem()->m_position - m_pItem->m_position;
    if (d.LengthXYZSquared() > 900.0f) return 0;
    int type = GetSkin()->m_VOType;
    if (type == 17) return 0;
    if (pTarget) {
        int target = pTarget->GetSkin()->m_VOType;
        switch (type) {
        case 0:
            switch (target) {
            case 1:
                if (AllyNear(10.0f, 0) && Actor_RandBit()) Speak(0x1af005a3, 0);
                else Speak(0x1af00447, 0);
                break;
            case 2:
                if (AllyNear(10.0f, 0) && Actor_RandBit()) Speak(0x1af005a2, 0);
                else Speak(0x1af00446, 0);
                break;
            default:
                return 0;
            }
            break;
        case 1:
            if (!IsHuman(target)) return 0;
            if (AllyNear(10.0f, 0) && Actor_RandBit()) Speak(0x1af005a4, 0);
            else Speak(0x1af0044e, 0);
            break;
        case 2:
            if (target != 0) return 0;
            if (AllyNear(10.0f, 0) && Actor_RandBit()) Speak(0x1af00597, 0);
            else Speak(0x1af003dd, 0);
            break;
        case 4:
            if (AllyNear(10.0f, 0) && Actor_RandBit()) Speak(0x1af005aa, 0);
            else Speak(0x1af004b1, 0);
            break;
        case 7:
            if (!IsHuman(target)) return 0;
            if (AllyNear(10.0f, 0) && Actor_RandBit()) Speak(0x1af005a7, 0);
            else Speak(0x1af004a4, 0);
            break;
        case 8:
            if (!IsHuman(target)) return 0;
            if (AllyNear(10.0f, 0) && Actor_RandBit()) Speak(0x1af0059b, 0);
            else Speak(0x1af00456, 0);
            break;
        case 9:
            if (!IsHuman(target)) return 0;
            Speak(0x1af003e4, 0);
            break;
        case 10:
            if (!IsHuman(target)) return 0;
            Speak(0x1af00497, 0);
            break;
        case 11:
            if (!IsHuman(target)) return 0;
            if (AllyNear(10.0f, 0) && Actor_RandBit()) Speak(0x1af0059e, 0);
            else Speak(0x1af003e0, 0);
            break;
        case 15:
            if (!IsHuman(target)) return 0;
            if (!AllyNear(10.0f, 0)) {
                gAIControl.SetSpokenCombatVO();
                return 0;
            }
            Speak(0x1af00453, 0);
            break;
        case 21:
            if (!IsHuman(target)) return 0;
            if (AllyNear(10.0f, 0) && Actor_RandBit()) Speak(0x1af00646, 0);
            else Speak(0x1af00647, 0);
            break;
        case 24:
            if (!IsHuman(target)) return 0;
            if (AllyNear(10.0f, 0) && Actor_RandBit()) Speak(0x1af006a0, 0);
            else Speak(0x1af0069f, 0);
            break;
        case 25:
            if (!IsHuman(target)) return 0;
            Speak(0x1af0065d, 0);
            break;
        case 26:
            if (!IsHuman(target)) return 0;
            if (AllyNear(10.0f, 0) && Actor_RandBit()) Speak(0x1af0066e, 0);
            else Speak(0x1af0066d, 0);
            break;
        case 27:
            if (target == 26) {
                if (AllyNear(10.0f, 0) && Actor_RandBit()) Speak(0x1af0068f, 0);
                else Speak(0x1af0068e, 0);
            } else {
                if (!IsHuman(target)) return 0;
                if (AllyNear(10.0f, 0) && Actor_RandBit()) Speak(0x1af0068d, 0);
                else Speak(0x1af0068c, 0);
            }
            break;
        case 28:
            if (target == 26) {
                if (AllyNear(10.0f, 0) && Actor_RandBit()) Speak(0x1af00689, 0);
                else Speak(0x1af00688, 0);
                break;
            }
            // Not marked as spoken.
            if (!IsHuman(target)) return 0;
            if (AllyNear(10.0f, 0) && Actor_RandBit()) Speak(0x1af00687, 0);
            else Speak(0x1af00686, 0);
            return 0;
        default:
            return 0;
        }
        gAIControl.SetSpokenCombatVO();
        return 0;
    }
    switch (type) {
    case 0:
        Speak(0x1af00448, 0);
        break;
    case 1:
        if (m_HitPredTimer > 0.0f) Speak(0x1af005a6, 0);
        else if (AllyNear(10.0f, 0) && Actor_RandBit()) Speak(0x1af005a5, 0);
        else Speak(0x1af0044f, 0);
        break;
    case 2:
        if (m_HitPredTimer > 0.0f) Speak(0x1af0059a, 0);
        else if (AllyNear(10.0f, 0) && Actor_RandBit()) Speak(0x1af00599, 0);
        else Speak(0x1af003de, 0);
        break;
    case 4:
        if (AllyNear(10.0f, 0) && Actor_RandBit()) Speak(0x1af005a9, 0);
        else Speak(0x1af004b0, 0);
        break;
    case 5:
        Speak(0x1af00496, 0);
        break;
    case 6:
        Speak(0x1af004af, 0);
        break;
    case 7:
        if (AllyNear(10.0f, 0) && Actor_RandBit()) Speak(0x1af005a8, 0);
        else Speak(0x1af004a5, 0);
        break;
    case 8:
        if (m_HitPredTimer > 0.0f) Speak(0x1af0059d, 0);
        else if (AllyNear(10.0f, 0) && Actor_RandBit()) Speak(0x1af0059c, 0);
        else Speak(0x1af00457, 0);
        break;
    case 9:
        Speak(0x1af003e4, 0);
        break;
    case 10:
        Speak(0x1af00497, 0);
        break;
    case 11:
        if (m_HitPredTimer > 0.0f) Speak(0x1af005a0, 0);
        else if (AllyNear(10.0f, 0) && Actor_RandBit()) Speak(0x1af0059f, 0);
        else Speak(0x1af003e1, 0);
        break;
    case 12:
        Speak(0x1af005f0, 0);
        break;
    case 15:
        if (!AllyNear(10.0f, 0)) goto spoken;
        Speak(0x1af00454, 0);
        break;
    case 16:
        if (m_HitPredTimer > 0.0f) Speak(0x1af005a1, 0);
        else Speak(0x1af003e7, 0);
        break;
    case 21:
        if (m_HitPredTimer > 0.0f) Speak(0x1af00649, 0);
        else {
            if (!AllyNear(10.0f, 0)) goto spoken;
            Speak(0x1af006d8, 0);
        }
        break;
    case 24:
        if (m_HitPredTimer > 0.0f) Speak(0x1af006a2, 0);
        else {
            if (!AllyNear(10.0f, 0)) goto spoken;
            ActorCarryable *pGun = ActorCarryable::GetPtrFromID(m_CarryRight);
            if (pGun && pGun->IsAGun()) Speak(0x1af006a1, 0);
            else Speak(0x1af0071e, 0);
        }
        break;
    case 25:
        Speak(0x1af0065e, 0);
        break;
    case 26:
        if (AllyNear(10.0f, 0) && Actor_RandBit()) Speak(0x1af00670, 0);
        else Speak(0x1af0066f, 0);
        break;
    case 27:
        if (AllyNear(10.0f, 0) && Actor_RandBit()) Speak(0x1af00691, 0);
        else Speak(0x1af00690, 0);
        break;
    case 28:
        if (AllyNear(10.0f, 0) && Actor_RandBit()) Speak(0x1af0068b, 0);
        else Speak(0x1af0068a, 0);
        break;
    case 29:
        if (gCurrentMap_GeoHash == 0x1000124) Speak(0x1af006f8, 0);
        else if (m_HitPredTimer <= 0.0f) Speak(0x1af006da, 0);
        else {
            if (!Actor_RandBit()) goto spoken;
            Speak(0x1af006db, 0);
        }
        break;
    default:
        return 0;
    }
spoken:
    gAIControl.SetSpokenCombatVO();
    return 1;
}

// A random entry of the n animation modes in a.
static __forceinline int Actor_PickAnim(const int *a, unsigned int n) { return a[EXRand32() % n]; }

// CurrentAnimLoops, forced inline as in UpdateAnimType.
static __forceinline signed char Actor_CurrentAnimLoops(Actor *p)
{
    EXItemCtrl *pCtrl = p->m_pAnim ? p->m_pAnim->m_stacks[0].FirstCtrl() : 0;
    if (!pCtrl) {
        p->m_ActorFlags2.m_bAnimEnded = 1;
        return 0;
    }
    return (signed char)(pCtrl->m_ctrlFlags.m_flags & 1);
}

// Picks the animation mode for the animation type (type, or the current one for -1, which
// only re-picks once a non-looping animation has ended). Choices depend on the skin type
// (5 predator-like, 7 Ulysses, 8 enforcers by sub-group, 10 Bruno, others civilians and
// soldiers by what they carry and their attributes), with random variations; injured
// sub-groups stagger instead. Skin type 4 then plays its queued animation once the previous
// one has finished.
void Actor::UpdateAnimType(int type)
{
    if (type != -1) m_AnimType = type;
    if (m_pAnim) m_pAnim->m_frameRateMult = AnimSpeedMult() * XApp::GlobalGameFrameRate();
    signed char bEnded = 0;
    if (!Actor_CurrentAnimLoops(this)) bEnded = m_ActorFlags2.m_bAnimEnded == 1;
    if (type == -1 && !bEnded) return;
    int skinType = GetSkin()->m_Type;
    signed char bInjured = *(signed char *)&GetSubGroupAnims().m_Anims[10];
    int subGroup = GetProfile()->m_SubGroup;
    int anim;
    switch (m_AnimType) {
    case 0x4e000001: // HT_AnimType_Stand
        if (bInjured) {
            int a[] = { 0x9000088 };
            anim = Actor_PickAnim(a, 1);
            break;
        } else {
            int anims[5];
            int n;
            switch (skinType) {
            case 5:
                if (subGroup == 0x4f000016) {
                    anims[0] = 0x9000522;
                    n = 1;
                } else {
                    anims[0] = 0x90003ff;
                    anims[1] = 0x9000402;
                    n = 2;
                }
                break;
            case 7:
                anims[0] = 0x90003a1;
                n = 1;
                break;
            case 8:
                if (subGroup == 0x4f00000e) {
                    anims[0] = 0x90002f1;
                    n = 1;
                } else if (subGroup == 0x4f000011) {
                    anims[0] = 0x900036b;
                    n = 1;
                } else if (subGroup == 0x4f000010) {
                    anims[0] = 0x90003ff;
                    anims[1] = 0x9000402;
                    n = 2;
                } else {
                    anims[0] = 0x900022a;
                    n = 1;
                }
                break;
            case 10:
                anims[0] = 0x90002f1;
                n = 1;
                break;
            default:
                if (m_Attrib & 2) {
                    anims[0] = 0x90000d4;
                    anims[1] = 0x9000178;
                    n = 2;
                    if (ActorCarryable::GetPtrFromID(m_CarryRight) && ActorCarryable::GetPtrFromID(m_CarryRight)->GetType() == CARRY_DRINK) {
                        anims[2] = 0x90000d3;
                        n = 3;
                    }
                } else if (m_Attrib & 4) {
                    anims[0] = 0x90000d0;
                    n = 1;
                } else if (!ActorCarryable::GetPtrFromID(m_CarryRight)) {
                    goto idle;
                } else {
                    switch (ActorCarryable::GetPtrFromID(m_CarryRight)->GetType()) {
                    case CARRY_FOOD:
                        anims[0] = 0x90000a1;
                        n = 1;
                        break;
                    case CARRY_WEAPON:
                    case CARRY_GUN: {
                        WeaponProfile *pWeapon = GetWeaponProfile(0);
                        if (!pWeapon) goto idle;
                        if (pWeapon->m_Id == 0x49000059) {
                            anims[0] = 0x9000002;
                            n = 1;
                        } else if (pWeapon->m_Id == 0x4900002a) {
                            anims[0] = 0x9000002;
                            n = 1;
                        } else {
                            switch (pWeapon->m_AimType) {
                            case 0x52000001:
                                anims[0] = 0x90002b6;
                                n = 1;
                                break;
                            case 0x52000002:
                                anims[0] = 0x900014a;
                                n = 1;
                                break;
                            case 0x52000003:
                                anims[0] = 0x90002b7;
                                n = 1;
                                break;
                            case 0x52000004:
                            case 0x52000007:
                                anims[0] = 0x9000002;
                                n = 1;
                                break;
                            case 0x52000005:
                                anims[0] = 0x900029a;
                                n = 1;
                                break;
                            case 0x52000006:
                                anims[0] = 0x9000481;
                                n = 1;
                                break;
                            default:
                                goto idle;
                            }
                        }
                        break;
                    }
                    case CARRY_DRINK:
                        anims[0] = 0x9000218;
                        n = 1;
                        break;
                    case CARRY_PHONE:
                        anims[0] = 0x9000068;
                        anims[1] = 0x9000069;
                        anims[2] = 0x900006a;
                        n = 3;
                        break;
                    case CARRY_CIGARETTE:
                        anims[0] = 0x9000066;
                        anims[1] = 0x9000067;
                        n = 2;
                        break;
                    default:
                    idle:
                        anims[0] = 0x9000065;
                        anims[1] = 0x9000002;
                        anims[2] = 0x9000002;
                        anims[3] = 0x9000002;
                        anims[4] = 0x9000002;
                        n = 5;
                        break;
                    }
                }
                break;
            }
            anim = Actor_PickAnim(anims, n);
        }
        break;
    case 0x4e000002: // HT_AnimType_Walk
        if (bInjured) {
            int a[] = { 0x90000c6, 0x900032b };
            anim = Actor_PickAnim(a, 2);
            break;
        }
        switch (skinType) {
        case 5:
            if (subGroup == 0x4f000016) anim = 0x9000426;
            else anim = 0x9000418;
            break;
        case 7:
            anim = 0x90001f5;
            break;
        case 8:
            switch (subGroup) {
            case 0x4f00000c: anim = 0x9000534; break;
            case 0x4f00000e: anim = 0x90002ea; break;
            case 0x4f00000f: anim = 0x900026a; break;
            case 0x4f000010: anim = 0x90004d7; break;
            case 0x4f000011: anim = 0x90004aa; break;
            default: anim = 0x900023c; break;
            }
            break;
        case 10:
            if (subGroup == 0x4f000028) anim = 0x90002e6;
            else anim = 0x90002ea;
            break;
        default:
            if (m_Attrib & 4) {
                anim = 0x9000019;
            } else if (m_Attrib & 2) {
                int a[] = { 0x9000019, 0x900001a };
                anim = Actor_PickAnim(a, 2);
            } else {
                int carry = ActorCarryable::GetPtrFromID(m_CarryRight) ? ActorCarryable::GetPtrFromID(m_CarryRight)->GetType() : CARRY_NONE;
                switch (carry) {
                case CARRY_PAPER: {
                    int a[] = { 0x9000016, 0x9000016, 0x9000016, 0x90000bf };
                    anim = Actor_PickAnim(a, 4);
                    break;
                }
                case CARRY_FOOD: {
                    int a[] = { 0x9000016, 0x9000016, 0x9000016, 0x90000c0 };
                    anim = Actor_PickAnim(a, 4);
                    break;
                }
                case CARRY_BOX:
                    anim = 0x900001c;
                    break;
                case CARRY_DRINK: {
                    int a[] = { 0x9000016, 0x9000016, 0x9000016, 0x90000c1 };
                    anim = Actor_PickAnim(a, 4);
                    break;
                }
                default:
                    anim = m_WalkAnim;
                    if (!anim) {
                        if (GetType() == 1) {
                            int a[] = { 0x900026d };
                            anim = Actor_PickAnim(a, 1);
                        } else {
                            anim = 0x9000016;
                        }
                    }
                    break;
                }
            }
            break;
        }
        break;
    case 0x4e000003: // HT_AnimType_Run
        if (bInjured) {
            int a[] = { 0x900013e };
            anim = Actor_PickAnim(a, 1);
            break;
        }
        switch (skinType) {
        case 5:
            if (subGroup == 0x4f000016) anim = 0x9000519;
            else anim = 0x9000418;
            break;
        case 7:
            anim = 0x9000245;
            break;
        case 8:
            switch (subGroup) {
            case 0x4f00000c: anim = 0x9000532; break;
            case 0x4f00000e: anim = 0x90002ea; break;
            case 0x4f00000f: anim = 0x900026a; break;
            case 0x4f000010: anim = 0x90004d0; break;
            case 0x4f000011: anim = 0x90004ab; break;
            case 0x4f000016:
            case 0x4f000021: anim = 0x9000519; break;
            default: anim = 0x900022b; break;
            }
            break;
        case 10:
            anim = 0x90002ea;
            break;
        default: {
            ActorCarryable *pCarry = ActorCarryable::GetPtrFromID(m_CarryRight);
            if (pCarry) {
                if (pCarry->GetType() == CARRY_BOX) {
                    anim = 0x9000102;
                    break;
                }
                if (pCarry->IsAGun()) {
                    int aim = pCarry->GetWeaponProfile()->m_AimType;
                    if (aim == 0x52000005 || aim == 0x52000006) {
                        anim = 0x900052e;
                        break;
                    }
                }
            }
            if (GetType() == 1) anim = 0x9000301;
            else anim = 0x900004b;
            break;
        }
        }
        break;
    case 0x4e000004: { // HT_AnimType_Attack_Idle
        if (bInjured) {
            int a[] = { 0x90001e3 };
            anim = Actor_PickAnim(a, 1);
            break;
        }
        WeaponProfile *pWeapon = GetWeaponProfile(0);
        switch (skinType) {
        case 5:
            if (subGroup == 0x4f000016) {
                int a[] = { 0x9000522 };
                anim = Actor_PickAnim(a, 1);
            } else {
                anim = 0x900022a;
            }
            break;
        case 7:
            anim = 0x90001f3;
            break;
        case 8:
            if (subGroup == 0x4f00000e) {
                anim = 0x90002f1;
            } else if (subGroup == 0x4f000011) {
                int a[] = { 0x900036b, 0x900036c, 0x900036d };
                anim = Actor_PickAnim(a, 3);
            } else {
                anim = 0x900022a;
            }
            break;
        case 10:
            anim = 0x90002f1;
            break;
        default:
            if (GetProfile()->m_SubGroup == 0x4f000005 && pWeapon && EXRand32() % 5 == 0) {
                int aim = pWeapon->m_AimType;
                if (aim == 0x52000000 || aim == 0x52000001 || aim == 0x52000002 || aim == 0x52000003 || aim == 0x52000007) {
                    int a[] = { 0x90002d2, 0x90002d3, 0x90002d4 };
                    anim = Actor_PickAnim(a, 3);
                } else {
                    int a[] = { 0x90002d2, 0x90002d4 };
                    anim = Actor_PickAnim(a, 2);
                }
                break;
            }
            if (!pWeapon) {
                int a[] = { 0x9000217 };
                anim = Actor_PickAnim(a, 1);
                break;
            }
            switch (pWeapon->m_AimType) {
            case 0x52000000: {
                int a[] = { 0x9000217, 0x9000217, 0x9000217 };
                anim = Actor_PickAnim(a, 3);
                break;
            }
            case 0x52000001: {
                int a[] = { 0x9000217 };
                anim = Actor_PickAnim(a, 1);
                break;
            }
            case 0x52000002: {
                int a[] = { 0x900014a, 0x900014a, 0x900014a };
                anim = Actor_PickAnim(a, 3);
                break;
            }
            case 0x52000003: {
                int a[] = { 0x9000217 };
                anim = Actor_PickAnim(a, 1);
                break;
            }
            case 0x52000004: {
                int a[] = { 0x9000106, 0x9000106, 0x9000106 };
                anim = Actor_PickAnim(a, 3);
                break;
            }
            case 0x52000005:
                anim = 0x9000274;
                break;
            case 0x52000006:
                anim = 0x900011a;
                break;
            case 0x52000007:
                anim = 0x9000217;
                break;
            default:
                goto done;
            }
            break;
        }
        break;
    }
    case 0x4e000007: // HT_AnimType_SideStepLeft
        switch (skinType) {
        case 5:
            if (subGroup == 0x4f000016) anim = 0x9000425;
            else anim = 0x9000419;
            break;
        case 7:
            anim = 0x9000053;
            break;
        case 8:
            switch (subGroup) {
            case 0x4f00000b:
            case 0x4f00000c:
            case 0x4f00000d: anim = 0x900023a; break;
            case 0x4f00000e: anim = 0x90002ec; break;
            case 0x4f000010: anim = 0x90004d2; break;
            case 0x4f000011: anim = 0x90004af; break;
            default: anim = 0x9000266; break;
            }
            break;
        case 10:
            anim = 0x90002ec;
            break;
        default:
            anim = 0x90001f0;
            break;
        }
        break;
    case 0x4e000008: // HT_AnimType_SideStepRight
        switch (skinType) {
        case 5:
            if (subGroup == 0x4f000016) anim = 0x9000424;
            else anim = 0x900041b;
            break;
        case 7:
            anim = 0x9000053;
            break;
        case 8:
            switch (subGroup) {
            case 0x4f00000b:
            case 0x4f00000c:
            case 0x4f00000d: anim = 0x900023b; break;
            case 0x4f00000e: anim = 0x90002ee; break;
            case 0x4f000010: anim = 0x90004d3; break;
            case 0x4f000011: anim = 0x90004ad; break;
            default: anim = 0x9000267; break;
            }
            break;
        case 10:
            anim = 0x90002ee;
            break;
        default:
            anim = 0x90001f1;
            break;
        }
        break;
    case 0x4e000009: // HT_AnimType_WalkBack
        switch (skinType) {
        case 5:
            anim = 0x900041a;
            break;
        case 7:
            anim = 0x9000053;
            break;
        case 8:
            switch (subGroup) {
            case 0x4f00000c: anim = 0x9000535; break;
            case 0x4f00000e: anim = 0x90002eb; break;
            case 0x4f000010: anim = 0x90004d8; break;
            case 0x4f000011: anim = 0x90004d9; break;
            default: anim = 0x9000238; break;
            }
            break;
        case 10:
            anim = 0x90002eb;
            break;
        default:
            anim = 0x90000ef;
            break;
        }
        break;
    case 0x4e00000a: // HT_AnimType_RunBack
        switch (skinType) {
        case 5:
            anim = 0x900041a;
            break;
        case 7:
            anim = 0x9000245;
            break;
        case 8:
            switch (subGroup) {
            case 0x4f00000c: anim = 0x9000533; break;
            case 0x4f00000e: anim = 0x90002eb; break;
            case 0x4f000010: anim = 0x90004d1; break;
            case 0x4f000011: anim = 0x90004d9; break;
            default: anim = 0x9000238; break;
            }
            break;
        case 10:
            anim = 0x90002eb;
            break;
        default:
            anim = 0x9000053;
            break;
        }
        break;
    case 0x4e00000b: // HT_AnimType_StrafeLeft
        switch (skinType) {
        case 5:
            if (subGroup == 0x4f000016) anim = 0x9000425;
            else anim = 0x9000419;
            break;
        case 7:
            anim = 0x9000053;
            break;
        case 8:
            switch (subGroup) {
            case 0x4f00000e: anim = 0x90002ec; break;
            case 0x4f00000f: anim = 0x9000266; break;
            case 0x4f000010: anim = 0x90004d2; break;
            case 0x4f000011: anim = 0x90004af; break;
            default: anim = 0x900023a; break;
            }
            break;
        case 10:
            anim = 0x90002ec;
            break;
        default:
            anim = 0x90000e7;
            break;
        }
        break;
    case 0x4e00000c: // HT_AnimType_StrafeRight
        switch (skinType) {
        case 5:
            if (subGroup == 0x4f000016) anim = 0x9000424;
            else anim = 0x900041b;
            break;
        case 7:
            anim = 0x9000053;
            break;
        case 8:
            switch (subGroup) {
            case 0x4f00000e: anim = 0x90002ee; break;
            case 0x4f00000f: anim = 0x9000267; break;
            case 0x4f000010: anim = 0x90004d3; break;
            case 0x4f000011: anim = 0x90004ad; break;
            default: anim = 0x900023b; break;
            }
            break;
        case 10:
            anim = 0x90002ee;
            break;
        default:
            anim = 0x90001f1;
            break;
        }
        break;
    case 0x4e00000d: { // panic run
        int a[] = { 0x900004b, 0x9000150, 0x9000151, 0x900013d };
        anim = Actor_PickAnim(a, 4);
        break;
    }
    default:
        // Any other value is an animation mode itself.
        anim = m_AnimType;
        break;
    }
    SetAnimMode(anim, 1);
done:
    if (bEnded && skinType == 4) {
        if (m_AnimMode == m_AnimModePrev && m_AnimModeNext) SetAnimMode(m_AnimModeNext, 1);
        m_AnimModeNext = 0;
        m_AnimModePrev = 0;
    }
}

// Animation script events: strikes, throws and item handling become messages to the brain,
// head and belt losses sever the limb (the boss/actor lose-head events hand the head to the
// player), plus rumble, blood pools, attack VO, Bruno's axe sparks and the predator landing
// blast. At the end of a script (-1) its stump blood scripts are released.
unsigned int Actor::DoScriptCmdEvent(GEOSCRIPT_EVENT *pEvent, EXItemAnimator *pAnim)
{
    switch (pEvent->m_Hash) {
    case 0x1600005e: // HT_ScriptEvents_Rumble
        ((XGamePad *)EXBaseGamePad::GetGamePad())->PlayRumble(pEvent->m_Param[0].u, pEvent->m_Param[1].f, 0xff);
        return 0;
    case 0x16000023: // HT_ScriptEvents_ActorDropItem
        SendMsg(MsgItem(MSG_DROP_ITEM), 0.0001f);
        return 0;
    case 0x16000024: // HT_ScriptEvents_ActorTakeItem
        SendMsg(MsgItem(MSG_TAKE_ITEM), 0.0001f);
        return 0;
    case 0x1600002d: // HT_ScriptEvents_ActorThrow
        SendMsg(MsgItem(MSG_THROW), 0.0001f);
        return 0;
    case 0x16000030: // HT_ScriptEvents_ActorLoseHead
    case 0x16000051: // HT_ScriptEvents_BossLooseHead
        CreateLostLimb(ALL_HEAD, EXVector(0.0f, 0.0f, 0.0f, 0.0f), 1, 0);
        return 0;
    case 0x16000034: // HT_ScriptEvents_ActorStrike
        SendMsg(MsgItem(MSG_STRIKE), 0.0001f);
        return 0;
    case 0x16000035: // HT_ScriptEvents_ActorReload
        SendMsg(MsgItem(MSG_RELOAD), 0.0001f);
        return 0;
    case 0x16000043: // HT_ScriptEvents_ActorBlock
        SendMsg(MsgItem(MSG_BLOCK), 0.0001f);
        return 0;
    case 0x16000044: // HT_ScriptEvents_Sparks
        if (GetProfile()->m_SubGroup == 0x4f000013) {
            EXDatumTemp d;
            memset(&d, 0, sizeof(d));
            if (m_pItem->GetAnimatorDatum(0x1000009c, d, 0, 0) && fabs(d.m_position.y - m_pItem->m_position.y) < 0.3f) {
                XItemHandler_ScriptBase::FX_CreateLink(0x400039b, m_RefID, 0x1000009c, 0x40, 0x1000016);
                return 0;
            }
        }
        break;
    case 0x1600004e: // HT_ScriptEvents_ActorStrikeLeft
        SendMsg(MsgItem(MSG_STRIKE_LEFT), 0.0001f);
        return 0;
    case 0x1600004f: // HT_ScriptEvents_ActorStrikeRight
        SendMsg(MsgItem(MSG_STRIKE_RIGHT), 0.0001f);
        return 0;
    case 0x16000057: // HT_ScriptEvents_AlienStrikeTeeth
        SendMsg(MsgItem(MSG_ALIEN_STRIKE_TEETH), 0.0001f);
        return 0;
    case 0x16000058: // HT_ScriptEvents_AlienStrikeTail
        SendMsg(MsgItem(MSG_ALIEN_STRIKE_TAIL), 0.0001f);
        return 0;
    case 0x16000059: // HT_ScriptEvents_AlienStrikeClaws
        SendMsg(MsgItem(MSG_ALIEN_STRIKE_CLAWS), 0.0001f);
        return 0;
    case 0x1600005f: { // HT_ScriptEvents_Rumble3D
        EXVector pos = pAnim->GetOwnerItem()->m_position;
        ((XGamePad *)EXBaseGamePad::GetGamePad())->PlayRumble3D(pEvent->m_Param[0].u, pos, pEvent->m_Param[2].f, pEvent->m_Param[3].f,
                                                               pEvent->m_Param[1].f, 0xff);
        return 0;
    }
    case 0x16000065:
        m_ActorFlags.m_b29 = 1;
        return 0;
    case 0x1600006f: // head off, no head item
        CreateLostLimb(ALL_HEAD, EXVector(0.0f, 0.0f, 0.0f, 0.0f), 0, 1);
        return 0;
    case 0x16000070: // head off
        CreateLostLimb(ALL_HEAD, EXVector(0.0f, 0.0f, 0.0f, 0.0f), 0, 0);
        return 0;
    case 0x16000071: // belt off
        CreateLostLimb(ALL_BELT, EXVector(0.0f, 0.0f, 0.0f, 0.0f), 0, 0);
        return 0;
    case 0x16000073: {
        EXVector pos = GetPelvisPos();
        Gore_BloodPool(pos, Gore_GetBloodGroup(this));
        return 0;
    }
    case 0x16000074: {
        EXVector pos = GetHeadPos();
        Gore_BloodPool(pos, Gore_GetBloodGroup(this));
        return 0;
    }
    case 0x16000078: {
        EXVector pos = m_pItem->m_position;
        XItemHandler_ScriptBase::Explosion_Create(pos, 0x40002d7, 6, 30, 0, 0, 1, m_RefID, 0);
        return 0;
    }
    case 0x1600007a:
        SendMsg(MsgItem(MSG_SCRIPT_EVENT_32), 0.0001f);
        return 0;
    case 0x1600007f:
        SendMsg(MsgItem(MSG_SCRIPT_EVENT_33), 0.0001f);
        return 0;
    case 0x16000080:
        if (!m_SpeechTag.IsPlaying()) SayAttackVO();
        return 0;
    case 0x16000081:
        if (!m_SpeechTag.IsPlaying()) SayAttackBigVO();
        return 0;
    case 0xffffffff:
        for (int i = 0; i < 4; i++) {
            if (pAnim == (EXItemAnimator *)m_LostLimbs[i].m_pBlood) {
                m_LostLimbs[i].m_pBlood->ItemFlags().BitSet(0x10);
                m_LostLimbs[i].m_pBlood = 0;
            } else if (pAnim == m_LostLimbs[i].m_pBlood2) {
                m_LostLimbs[i].m_pBlood2->ItemFlags().BitSet(0x10);
                m_LostLimbs[i].m_pBlood2 = 0;
            }
        }
        return 0;
    }
    return 0;
}

// Whether a left hand item stays visible: not the petrol bomb or weapons (melee or guns),
// except Bruno's shield. Retail's copy is a file static taking entity in eax with no callers
// left (SetHide has it inlined); ours is a plain function so it is emitted at all.
signed char Actor_ItemShouldBeShownInLeftHand(int entity)
{
    if (entity == 0x20000dc) return 0;
    if (entity == 0x2000139) return 1;
    CarryType t = ActorCarryable::FindCarryTypeForEntity(entity);
    if (t == CARRY_WEAPON || t == CARRY_GUN) return 0;
    return 1;
}

Actor *Actor::CreateObject() { return new Actor; }

signed char Actor::GotoPOI(int poi)
{
    POI_STRUCT *p = POI_Get(poi);
    return GotoPosPoly(p->m_Pos, p->m_Poly);
}

void Actor::SetActorIDPlayerHasHead(int id, XItem *pHead)
{
    DeleteHeldHead();
    ms_iActorIDPlayerHasHead = id;
    ms_pLimbInPlayersHand = pHead;
}

void Actor::SwapHands()
{
    int l = m_CarryLeft;
    m_CarryLeft = m_CarryRight;
    m_CarryRight = l;
    SetHide(m_StateFlags.m_bHide);
}

void Actor::SayBulletOuch()
{
    int sfx = GetHitSFX();
    if (!sfx) return;
    if (m_SpeechTag.IsPlaying(m_LastHitSFX)) return;
    m_LastHitSFX = sfx;
    Speak(sfx, 0);
}

void Actor::UpdateSFX()
{
    EXVector pos = GetHeadPos();
    m_SFXTag.UpdatePosition(pos);
    m_SpeechTag.UpdatePosition(pos);
}

ActorEffect::ActorEffect(Actor *pActor, unsigned int file, unsigned int hash, unsigned int datum, float time, signed char b,
                         unsigned __int64 id)
{
    Init(pActor, file, hash, datum, time, b, id);
}

void Actor::SetCloakDirect(signed char b)
{
    if (m_ActorFlags2.m_bBeingDestructed) return;
    if (!m_StateFlags.m_bCloaked && b) m_Unk848f = 0.5f;
    if (m_StateFlags.m_bCloaked != b) {
        m_StateFlags.m_bCloaked = b;
        ActorTextureSwap::Apply(this);
        unsigned int sfx;
        if (GetProfile()->m_SubGroup != 0x4f000021) sfx = (b != 0) + 0x1af003a5;
        else sfx = b ? 0x1af004fa : 0x1af004fb;
        if (sfx) XGetSoundManager()->StartSound(sfx, GetHeadPos(), -1, 0);
    }
    m_StateFlags.m_bCloaked = b != 0;
    m_pAnim->RenderItem()->RenderInfo().EnableRefractionRender(b);
    if (XGetPlayer() && XGetPlayer()->GetVisionMode() == 3) m_StateFlags.m_bTargetable = 1;
    else m_StateFlags.m_bTargetable = !b;
}

void Actor::SetCloak(signed char b)
{
    if (IsCloaked() == b) return;
    SparkCloak(2);
    SetCloakDirect(b);
}

void Actor::BreakCloak(int secs)
{
    if (!m_StateFlags.m_bCloaked) return;
    if (secs <= 0) secs = 9999;
    m_CloakBreakTimer = (float)(secs * 60);
    SparkCloak(2);
    SetCloakDirect(0);
}

float Actor_GetFadeDistance(Actor *pActor)
{
    float d = XGetCamera(0)->Zoom() * gActorFadeDist;
    if (pActor && d >= pActor->GetAliveRadius()) d = pActor->GetAliveRadius() - 5.0f;
    return d;
}

int Actor::SendMsg(int id, const MsgItem &msg, float delay)
{
    Actor *p = GetActorPtr(id);
    if (!p) return 0;
    return p->SendMsg(msg, delay);
}

void Actor::SetupDepthPtr(float *p, unsigned int n)
{
    if (!m_pAnim) return;
    EXItemCtrl *pCtrl = m_pAnim->Controller(n);
    if (pCtrl && pCtrl->IsKindOf(EXRUNTIME_CLASS(EXItemCtrl_Anim))) ((EXItemCtrl_Anim *)pCtrl)->m_frameDepth.SetPtr(p);
}

void Actor::SendMsg(const MsgItem &msg, const EXVector &pos, float radius, Actor *pFrom, float delay)
{
    for (Actor *p = GetFirst(pos, radius, 0); p; p = p->GetNext(pos, radius, 0))
        if (p != pFrom) p->SendMsg(msg, delay);
}

signed char Actor::EffectUpdate()
{
    ActorEffect *p = ActorEffect::FromLink(m_EffectList.Head());
    while (p) {
        ActorEffect *pNext = ActorEffect::FromLink(p->m_Link.Next());
        if (!p->DoUpdate()) delete p;
        p = pNext;
    }
    return 1;
}

void Actor::SetPosition(const EXVector &pos, int poly)
{
    m_pItem->SetPosition(pos);
    m_OrgLoc.SetPosPoly(pos, poly, 0);
}

signed char Actor::IsAnimVariableStart(int anim, Actor *pActor)
{
    if (pActor && !pActor->VariableSpeedAnims()) return 0;
    switch (anim) {
    case 0x9000017:
    case 0x900013d:
    case 0x900014e:
    case 0x900014f:
    case 0x9000150:
    case 0x9000151:
    case 0x4e000002:
    case 0x4e000003:
    case 0x4e00000d:
        return 1;
    }
    return 0;
}

signed char Actor::GetBonePos(unsigned int bone, EXVector &v)
{
    Actor_CalledGetDatum++;
    EXDatumTemp d;
    m_pItem->GetAnimatorDatum(bone, d, 0, 0);
    v = d.m_position;
    return 1;
}

// NEAR: 2 blocks differ, retail places the shared boss return right after the first test
int Actor::GetWarcrySFX()
{
    if (m_ActorFlags.m_bBoss) return 0x1af00337;
    switch (GetSkin()->m_VoiceType) {
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
        return 0x1af00337;
    case 2:
    case 3:
    case 4:
    case 5:
        return 0x1af00336;
    }
    return 0;
}

// NEAR: retail places the shared return 1 block right after the first test
signed char Actor::HasRagDollComeToRest()
{
    if (!m_StateFlags.m_bRagDoll) return 1;
    if (m_RagDollTime > (float)(int)(EXBaseApp::NominalGameFrameRate() * 10)) return 1;
    EX6DAnim *p = (EX6DAnim *)m_pItem->m_pItemPhysics;
    if (!p || !p->GetRuntimeClass()->IsDerivedFrom(EXRUNTIME_CLASS(EX6DAnim))) return 1;
    return !p->Active();
}

BloodType Actor::GetBloodType() const
{
    if (m_ActorFlags.m_bUlysses) return (BloodType)3;
    if (m_ActorFlags.m_bAlien) return (BloodType)2;
    unsigned int g = GetProfile()->m_SubGroup;
    if (g == 0x4f000014 || g == 0x4f000015 || g == 0x4f000016 || g == 0x4f000017) return (BloodType)1;
    return (BloodType)0;
}

void Actor::CatchDisc()
{
    ActorCarryable *p = ActorCarryable::GetPtrFromID(m_CarryRight);
    if (p) p->SetHide(0);
    m_ActorFlags.m_bDiscNotInHand = 0;
    ActorCarryableGun *pGun = (ActorCarryableGun *)ActorCarryable::GetPtrFromID(m_CarryRight);
    if (pGun && pGun->m_bGun) pGun->StopFiring();
}

void Actor::DeleteHeldHead()
{
    if (!ms_iActorIDPlayerHasHead || !ms_pLimbInPlayersHand) return;
    Actor *p = GetActorPtr(ms_iActorIDPlayerHasHead);
    if (!p) return;
    ms_iActorIDPlayerHasHead = 0;
    ms_pLimbInPlayersHand = 0;
    p->DeleteLostLimb((ActorLostLimb)2);
}

void Actor::DeleteChangeSkin()
{
    if (m_pChangeSkinItem) {
        m_pChangeSkinItem->ItemFlags().BitSet(0x10);
        m_pChangeSkinItem = 0;
    }
    ActorSkin *pSkin = AS_GetPtrFromIndex(m_NewSkinIndex);
    if (!pSkin) return;
    GeoSectionManager::Del(pSkin->m_File, pSkin->m_Hash);
    GeoSectionManager::Del(pSkin->m_ChangeFile, pSkin->m_ChangeHash);
    m_NewSkinIndex.m_Sheet = 0xffff;
    m_NewSkinIndex.m_Row = 0xffff;
}

signed char Actor::GotoPosPoly(const EXVector &pos, int poly)
{
    XNavLocation loc;
    loc.SetPosPoly(pos, poly, 0);
    return GotoLoc(loc);
}

// NEAR: register choice (ecx vs eax) for the returned EXVector temp
signed char Actor::GotoCustomerPOI(int poi, int idx)
{
    if (idx < 0) idx = POI_CustomerStandIndex(poi, m_RefID);
    return GotoPosPoly(POI_CustomerStandPos(poi, idx), POI_CustomerStandPoly(poi, idx));
}

// NEAR: register choice (ecx vs eax) for the returned EXVector temp
signed char Actor::GotoVendorPOI(int poi, int idx)
{
    if (idx < 0) idx = POI_VendorStandIndex(poi, m_RefID);
    return GotoPosPoly(POI_VendorStandPos(poi, idx), POI_VendorStandPoly(poi, idx));
}

signed char Actor::EffectCreate(unsigned int file, unsigned int hash, const EXVector &pos, float time, signed char b,
                                unsigned __int64 id)
{
    if (BeingDestructed()) return 0;
    new ActorEffect(this, file, hash, pos, time, b, id);
    return 1;
}

void Actor::SetHealth(float h, unsigned int by)
{
    if (CannotDie()) return;
    float old = m_Health;
    m_Health = h;
    if (0.0f >= h && old > 0.0f) {
        m_Health = 0.0f;
        if (by == 1) ActorSpecial_RegisterPlayerKill(this);
        if (!ShouldBeSkullRipped()) AwardDeathObjective();
    }
}

void Actor::ChangeHealth(float d, unsigned int by)
{
    if (CannotDie()) return;
    float old = m_Health;
    m_Health += d;
    if (0.0f >= m_Health && old > 0.0f) {
        m_Health = 0.0f;
        if (by == 1) ActorSpecial_RegisterPlayerKill(this);
        if (!ShouldBeSkullRipped()) AwardDeathObjective();
    }
}

unsigned int RemapSFXHashCode(unsigned int hash)
{
    if (gCurrentMap_GeoHash == 0x1000124) {
        switch (hash) {
        case 0x1af005d0: return 0x1af006f5;
        case 0x1af005d1: return 0x1af006f4;
        case 0x1af005d2: return 0x1af006f6;
        case 0x1af005ca: return 0x1af006f7;
        case 0x1af005d3: return 0x1af006f3;
        }
    }
    return hash;
}

signed char Actor::CarryingHiddenWeapon() const
{
    if (m_CarryLeft) {
        ActorCarryable *p = ActorCarryable::GetPtrFromID(m_CarryLeft);
        if (p) {
            CarryType t = ActorCarryable::FindCarryTypeForEntity(p->GetEntityHash());
            if (t == 9 || t == 6) {
                if (m_CarryRight) {
                    p = ActorCarryable::GetPtrFromID(m_CarryRight);
                    if (p) {
                        t = ActorCarryable::FindCarryTypeForEntity(p->GetEntityHash());
                        if (t == 9 || t == 6) return 0;
                    }
                }
                return 1;
            }
        }
    }
    return 0;
}

float Actor::GetGunYOffsetFromFeet(signed char bLeft)
{
    ActorCarryable *p;
    if (bLeft) p = ActorCarryable::GetPtrFromID(m_CarryLeft);
    else p = ActorCarryable::GetPtrFromID(m_CarryRight);
    ActorCarryableGun *pGun = (!p || !p->m_bGun) ? 0 : (ActorCarryableGun *)p;
    if (!pGun) return 0.0f;
    EXVector pos, dir;
    pGun->GetMuzzleDatum(pos, dir);
    return pos.y - GetFeetPos().y;
}

signed char Actor::WillThisDaze(float damage)
{
    if (!IsDrone()) return 0;
    float t = GetProfile()->m_Health * m_DazeMult;
    if (m_Health >= t && t > m_Health - damage) {
        m_DazeMult = m_DazeMult * 0.5f;
        return 1;
    }
    return 0;
}

void Actor::PreLoadInit()
{
    ActorBase *b = g_aActorList;
    for (int n = 128; n; n--, b++) {
        Actor *p = b->m_pActor;
        if (!p) continue;
        if (p->InWorld()) {
            p->m_pItem->ItemFlags().BitSet(0x10);
        } else if (p->m_pItem) {
            delete p->m_pItem;
            p->m_pItem = 0;
        }
    }
    memset(g_aActorList, 0, sizeof(g_aActorList));
    ResetGlobalAffiliation();
}

ActorEffect::~ActorEffect()
{
    if (m_pScript) {
        m_pScript->ItemFlags().BitSet(0x10);
    } else if (!m_pActor->BeingDestructed()) {
        EXGeoRefObject r;
        r.Set(EXGeoFile::GetGeoFile(m_File), m_Hash);
        m_pActor->GetAnim()->DeleteCloneSkin(r, 0);
    }
    m_pActor->EffectList()->Remove(&m_Link);
}

void Actor::SendMsgCapped(const MsgItem &msg, const EXVector &pos, float radius, Actor *pFrom, float delay, float lo,
                          float hi)
{
    for (Actor *p = GetFirst(pos, radius, 0); p; p = p->GetNext(pos, radius, 0)) {
        if (p == pFrom) continue;
        float dy = p->GetFeetPos().y - pos.y;
        if (!(dy > hi) && !(lo > dy)) p->SendMsg(msg, delay);
    }
}

void Actor::Speak(int sfx, signed char bForce)
{
    unsigned int hash = RemapSFX(sfx);
    if (!hash) return;
    if (m_SpeechTag.IsPlaying()) {
        if (!bForce && m_SpeechTag.IsPlaying(hash)) return;
        m_SpeechTag.RemoveAll(1);
    }
    EXSoundHandle *p = m_SpeechTag.StartSound(hash, GetHeadPos(), -1, 0);
    if (p) p->SetLoopingSound(0);
}

float Actor::GetAnimFrame()
{
    for (int i = 0; i < 3; i++) {
        EXItemCtrlStack stack = m_pAnim->ControllerStack(i);
        for (EXItemCtrl *p = stack.FirstCtrl(); p; p = p->Next())
            if (p->GetRuntimeClass() == EXRUNTIME_CLASS(EXItemCtrl_AnimScript))
                return ((EXItemCtrl_AnimScript *)p)->ScriptOffset();
    }
    return 0.0f;
}

signed char Actor::CheckRayIntersection(const EXRay &ray) const
{
    EXCapsule c;
    c.SetRadius(0.45f);
    c.Direction() = EXVector(0.0f, 1.8f, 0.0f, 0.0f);
    c.Origin() = GetFeetPos();
    return EXIntersection(ray, c);
}

// NEAR: retail unrolls 4x with a separate count-down register
void Actor::VehicleRemoved(Vehicle *pVehicle)
{
    for (ActorBase *b = g_aActorList; b < g_aActorList + 128; b++) {
        Actor *p = b->m_pActor;
        if (p && p->HitByVehiclePtr() == pVehicle) p->SetHitByVehiclePtr(0);
    }
}

signed char Actor::IsMafiaBossVulnerable(int i)
{
    if (m_EMPTimer > 0.0f) return 1;
    switch (i) {
    case 0:
    case 2:
    case 8:
    case 10:
    case 11:
    case 51:
    case 52:
    case 53:
    case 54:
        return 1;
    }
    return 0;
}

void Actor::DumpDebugInfo()
{
    ACTOR_TRACE(EXHashCodeFile::Debug_HashCodeName(GetProfile()->m_Hash));
    ActorCarryable *p = ActorCarryable::GetPtrFromID(m_CarryLeft);
    if (p) {
        ACTOR_TRACE(EXHashCodeFile::Debug_HashCodeName(GetProfile()->m_Hash));
        p->DumpDebugInfo();
    }
    p = ActorCarryable::GetPtrFromID(m_CarryRight);
    if (p) {
        ACTOR_TRACE(EXHashCodeFile::Debug_HashCodeName(GetProfile()->m_Hash));
        p->DumpDebugInfo();
    }
}

signed char Actor::AreThereFriendlyActorsNear(const EXVector &pos, float radius)
{
    ActorProfile *pMe = GetProfile();
    for (Actor *p = GetFirst(pos, radius, 0); p; p = p->GetNext(pos, radius, 0))
        if (!p->IsDead() && !AP_DoWeFight(pMe, p->GetProfile())) return 1;
    return 0;
}

void Actor::MsgQueueProcess()
{
    for (int i = 0; i < m_MsgQueueCount; i++) {
        if (!(0.0f >= m_MsgQueue[i].m_Delay)) {
            m_MsgQueue[i].m_Delay -= XApp::GameFrameRateMul();
            if (0.0f >= m_MsgQueue[i].m_Delay) {
                Actor *p = GetActorPtr(m_MsgQueue[i].m_ActorID);
                if (p) p->SendMsg(m_MsgQueue[i].m_Msg, 0.0f);
            }
        }
    }
}

signed char Actor::GotoPosSlow(const EXVector &pos)
{
    m_DestLoc.SetPosSlow(pos, 0);
    m_Unk08 = GetFeetPos();
    m_StateFlags.m_bMoving14 = 0;
    m_StateFlags.m_bMoving27 = 0;
    m_StateFlags.m_bMoving15 = 1;
    return CanReachZone(m_DestLoc.NavZone());
}

int Actor::CheckCollision(const EXLineSeg &seg, float r, EXVector &out)
{
    EXCapsule c;
    c.Segment() = seg;
    c.SetRadius(r);
    return CheckCollision(c, out);
}

// noinline: our build would otherwise inline it into DeleteHeldHead, retail calls it.
__declspec(noinline) void Actor::DeleteLostLimb(ActorLostLimb limb)
{
    if (limb < 0 || limb >= 4) return;
    if (ms_iActorIDPlayerHasHead == m_RefID) {
        DeleteHeldHead();
        ms_iActorIDPlayerHasHead = 0;
        ms_pLimbInPlayersHand = 0;
    }
    LostLimbStruct *l = &m_LostLimbs[limb];
    if (l->m_Id) AU_DeleteItem((EXItem *)l->m_Id);
    l->m_Id = 0;
    m_LostLimbs[limb].m_pBlood2 = 0;
}

// NEAR: retail interleaves the feet-position copy loads/stores
void Actor::CheckSeeBody(Actor *pBody)
{
    if (!pBody) return;
    EXVector pos = pBody->GetFeetPos();
    if (CanSeePoint(pos, 0.0f)) {
        m_StateFlags.m_bSeenBody = 1;
        m_SeenBodyID = pBody->m_RefID;
    }
}

// NEAR: esi/edi swapped
unsigned int Actor::RemapSFX(int sfx)
{
    unsigned int hash = sfx;
    if (sfx == 0x1af000ed) hash = GetDeathSFX();
    else if (sfx == 0x1af000e0) hash = GetScreamSFX();
    else if (sfx == 0x1af000eb) hash = GetHitSFX();
    else if (sfx == 0x1af000f2) hash = GetDazedSFX();
    ActorSkin *pSkin = GetSkin();
    if (pSkin && pSkin->m_Type == 1) {
        switch (hash) {
        case 0x1af0042f: return 0x1af0042e;
        case 0x1af00432: return 0x1af00431;
        }
    }
    return hash;
}

signed char Actor::CanLooseLimbs() const
{
    if (!IsDrone()) return 0;
    if (GetProfile()->m_SubGroup == 0x4f000033) return 0;
    switch (GetProfile()->m_Hash) {
    case 0x4700019b:
    case 0x470001f2:
    case 0x4700022f:
    case 0x4700033d:
    case 0x47000340:
    case 0x47000379:
    case 0x47000386:
        return 0;
    }
    switch (GetSkin()->m_Hash) {
    case 0xd000138:
    case 0xd00013b:
    case 0xd00016b:
        return 0;
    }
    return 1;
}

void Actor::SpinToYRot(const EXVector &pos)
{
    const EXMaths::vec_xyzw &d = pos - GetFeetPos();
    SpinToYRot(qatan2f(d.x, d.z));
}

signed char Actor::IsFacingYRot(const EXVector &pos, float tol)
{
    const EXMaths::vec_xyzw &d = pos - GetFeetPos();
    return IsFacingYRot(qatan2f(d.x, d.z), tol);
}

// GUESS: placement. Defined here so its dynamic initialiser gets retail's number (_$E20); the
// numbers before it are used up by the functions above.
ActorBase g_aActorList[128];

void Actor::SetTargetYRot(const EXVector &pos, float speed)
{
    const EXMaths::vec_xyzw &d = pos - GetFeetPos();
    SetTargetYRot(qatan2f(d.x, d.z), speed);
}

EXVector Actor::GetHandPos(signed char bLeft)
{
    int hash = GetHandDatumHash(bLeft);
    EXDatumTemp d;
    memset(&d, 0, sizeof(d));
    m_pItem->GetAnimatorDatum(hash, d, 0, 0);
    return d.m_position;
}

signed char Actor::GotoLoc(XNavLocation &loc)
{
    if (!loc.IsValid()) return 0;
    m_DestLoc = loc;
    m_Unk08 = GetFeetPos();
    m_StateFlags.m_bMoving14 = 0;
    m_StateFlags.m_bMoving27 = 0;
    m_StateFlags.m_bMoving15 = 1;
    return CanReachZone(m_DestLoc.NavZone());
}

void Actor::PlaySFX(int sfx, signed char bTrack)
{
    int hash = RemapSFX(sfx);
    if (!hash) return;
    EXSoundHandle *p = m_SFXTag.StartSound(hash, GetHeadPos(), 5.0f, 50.0f, -1, 0);
    if (p) p->SetTrackingSound(bTrack);
    switch (hash) {
    case 0x1af00104:
    case 0x1af00106:
    case 0x1af0010f:
    case 0x1af00135:
    case 0x1af0014f:
    case 0x1af0028c:
        p->SetLoopingSound(1);
        break;
    default:
        p->SetLoopingSound(0);
        break;
    }
}

// NEAR: add esp,8 scheduled one fmul later in retail
signed char Actor::IsFacingYRot(float rot, float tol)
{
    if (m_StateFlags.m_bMoving27) tol *= 0.25f;
    while (rot < 0.0f) rot += 6.2831855f;
    while (rot > 6.2831855f) rot -= 6.2831855f;
    if (qfabsf(XNavNetwork::AngleDifference(GetRotY(), rot)) < (tol / 180.0f) * 3.1415927f) return 1;
    return 0;
}

EXVector Actor::GetPelvisPosApprox()
{
    if (XGetGameWnd()->FrameCounter() == m_PelvisPosFrame) return m_PelvisPosCache;
    EXVector p = GetFeetPos();
    if (m_ActorFlags.m_bBoss) p.y += 1.2f;
    else p.y += 1.0f;
    return p;
}

EXVector Actor::GetHeadPosApprox()
{
    if (XGetGameWnd()->FrameCounter() == m_HeadPosFrame) return m_HeadPosCache;
    EXVector p = GetFeetPos();
    if (m_ActorFlags.m_bBoss) p.y += 2.0f;
    else p.y += 1.7f;
    return p;
}

EXVector Actor::GetChestPosApprox()
{
    if (XGetGameWnd()->FrameCounter() == m_HeadPosFrame) return m_HeadPosCache;
    EXVector p = GetFeetPos();
    if (m_ActorFlags.m_bBoss) p.y += 1.6f;
    else p.y += 1.35f;
    return p;
}

signed char Actor::ShouldBeSkullRipped()
{
    if (GetProfile()->m_SubGroup == 0x4f000016 && gCurrentMap_GeoHash == 0x100011c) return 0;
    switch (GetProfile()->m_SubGroup) {
    case 0x4f00000c:
    case 0x4f00000d:
    case 0x4f00000e:
    case 0x4f00000f:
    case 0x4f000012:
    case 0x4f000013:
    case 0x4f000014:
    case 0x4f000015:
    case 0x4f000016:
    case 0x4f00002e:
    case 0x4f00002f:
    case 0x4f000030:
    case 0x4f000031:
        return 1;
    case 0x4f000010:
        return GetSkin()->m_Hash == 0xd000096;
    }
    return 0;
}

void Actor::SetCarry(int id, signed char bLeft)
{
    ActorCarryable *p = GetCarryPtr(bLeft);
    if (p) p->SetOwner(0);
    p = ActorCarryable::GetPtrFromID(id);
    if (p) p->SetOwner(m_RefID);
    if (bLeft) m_CarryLeft = id;
    else m_CarryRight = id;
    SetAttrib(0x10, m_CarryLeft || m_CarryRight);
}

void Actor::SetHide(signed char b)
{
    m_StateFlags.m_bHide = b != 0;
    ActorCarryable *pLeft = ActorCarryable::GetPtrFromID(m_CarryLeft);
    if (pLeft) pLeft->SetHide(b);
    ActorCarryable *pRight = ActorCarryable::GetPtrFromID(m_CarryRight);
    if (pRight) pRight->SetHide(b);
    if (b) {
        m_pItem->ItemFlags().BitClr(1);
        return;
    }
    if (pLeft) {
        int hash = pLeft->GetEntityHash();
        CarryType t;
        if (hash == 0x20000dc ||
            (hash != 0x2000139 && ((t = ActorCarryable::FindCarryTypeForEntity(hash)) == 6 || t == 9)))
            pLeft->SetHide(1);
        else
            pLeft->SetHide(0);
    }
    m_pItem->SetHasRender();
}

// NEAR: retail interleaves the feet-position copy loads/stores
signed char Actor::AllyNear(float radius, signed char b)
{
    EXVector pos = GetFeetPos();
    int group = GetProfile()->m_Group;
    for (Actor *p = GetFirst(pos, radius, b); p; p = p->GetNext(pos, radius, b))
        if (p != this && p->GetProfile()->m_Group == group) return 1;
    return 0;
}

// NEAR: retail saves ebx/esi inside the notice branch
signed char Actor::DoNonCriticalUpdate()
{
    if (Actor_GetPause()) return 1;
    CheckSeePlayer();
    UpdateCheckFight();
    CheckForFriendsInCombat();
    if (ShouldBeNoticed()) {
        float r = 10.0f;
        if (GetProfile()->m_Hash == 0x470001d9) r = 40.0f;
        for (Actor *p = GetFirst(GetFeetPos(), r, 0); p; p = p->GetNext(GetFeetPos(), r, 0)) p->NoticeActor(this, 0);
    } else {
        NoticePlayer();
    }
    return 1;
}

inline void Actor::UpdateVisionMode()
{
    if (Affiliation() == GlobalAffiliation() && !(m_Health <= 0.0f) && !IsLimbLost(ALL_HEAD) && !m_Unk820)
        FEGetHud(1)->DrawAffiliation(this);
}

inline void Actor::UpdateCloak()
{
    if (!m_StateFlags.m_bCloaked) return;
    switch (XGetPlayer()->GetVisionMode()) {
    case 1:
    case 2:
        m_pAnim->RenderItem()->RenderInfo().EnableRefractionRender(0);
        m_StateFlags.m_bTargetable = 0;
        break;
    case 3:
        m_pAnim->RenderItem()->RenderInfo().EnableRefractionRender(0);
        m_StateFlags.m_bTargetable = 1;
        break;
    default:
        m_Unk848f = 0.4f;
        (*(u32 **)((char *)m_pAnim + 0x5c))[0x78 / 4] &= ~4;  // TODO EXItemAnimator +0x5c layout
        m_ExtRenderInfo2.m_RefractionStrength = m_Unk848f;
        m_pAnim->RenderItem()->RenderInfo().EnableRefractionRender(1);
        m_StateFlags.m_bTargetable = 0;
        break;
    }
    if (m_CloakBreakTimer > 0.0f) {
        m_CloakBreakTimer -= GetFrameRateMul();
        if (m_CloakBreakTimer <= 0.0f) SetCloak(1);
    }
}

// Retail inlined EXVector::operator- in DoUpdate; VC7.1 will not, so a local forced copy stands in.
static __forceinline EXMaths::vec_xyzw VSub(const EXMaths::vec_xyzw &a, const EXMaths::vec_xyzw &b)
{
    EXMaths::vec_xyzw r;
    r.x = a.x - b.x; r.y = a.y - b.y; r.z = a.z - b.z; r.w = a.w - b.w;
    return r;
}

// NEAR: structure complete; retail calls Normalise/frame helpers inline, frame is 4 bytes smaller and a few
// blocks are scheduled differently (stack slots of the camera delta).
signed char Actor::DoUpdate()
{
    if (NeedsDelete() || !m_pItem) return 1;
    m_pOptions = ActorOptionsManager::Options(this);
    CalculateParameters();
    if (!m_pAnim) return 1;
    EXItemCtrl *pCtrl = m_pAnim->m_stacks[0].FirstCtrl();
    if (Actor_GetPause()) {
        UpdateAlpha();
        pCtrl->CtrlFlags().BitClr(2);
        return 1;
    }
    pCtrl->CtrlFlags().BitSet(2);
    CheckHasAnimEnded();
    m_pAnim->SetFrameRate60(GetGlobalGameFrameRate() * 60.0f);
    m_ActorFlags.m_bInCombatWithPlayer = 0;
    EXMaths::vec_xyzw d = VSub(XGetCamera()->GetCameraPosition(), m_pItem->Position());
    if (m_StateFlags.m_bRagDoll || m_StateFlags.m_bAimingPos || m_StateFlags.m_bMoving27 || GetSkin()->m_Hash == 0xd0000b9)
        m_pAnim->SetLodDetails(0, 0);
    else if (!m_StateFlags.m_bOnScreen && !(Sq(10.0f) > d.x * d.x + d.y * d.y + d.z * d.z))
        m_pAnim->SetLodDetails(g_LodDetailsOffscreen, 1);
    else if (m_ActorFlags.m_bUlysses)
        m_pAnim->SetLodDetails(g_LodDetailsOnScreenUlysses, 4);
    else if (m_ActorFlags.m_bBoss)
        m_pAnim->SetLodDetails(g_LodDetailsOnScreenBoss, 4);
    else if (m_ActorFlags.m_bAlien)
        m_pAnim->SetLodDetails(g_LodDetailsOnScreenAlien, 4);
    else
        m_pAnim->SetLodDetails(g_LodDetailsOnScreenDrone, 4);
    m_StateFlags.m_b8 = 1;
    if (!Actor_GetPause()) {
        m_UpdateTime += XGetFrameLength();
        if (m_BlockCounter > 0.0f) m_BlockCounter -= XGetFrameRateMul();
        if (m_CombatCircleStopTime > 0.0f) m_CombatCircleStopTime -= XGetFrameRateMul();
        if (m_Unk898 > 0.0f) m_Unk898 -= XGetFrameRateMul();
        if (m_InvinsibleCounter > 0.0f) m_InvinsibleCounter -= XGetFrameRateMul();
        if (m_HitPredTimer > 0.0f) m_HitPredTimer -= XGetFrameRateMul();
        if (m_EMPTimer > 0.0f) m_EMPTimer -= XGetFrameRateMul();
        if (m_NotAllowedToGetUpTimer != 0.0f) m_NotAllowedToGetUpTimer -= XGetFrameRateMul();
        UpdateAlarmMeter();
        if (m_StateFlags.m_bExpandedFightDist) {
            if (m_FightDistTimer <= 0.0f) m_StateFlags.m_bExpandedFightDist = 0;
            else m_FightDistTimer -= GetFrameRateMul();
        }
        m_RandomDirTimer -= XGetFrameRateMul();
        if (m_RandomDirTimer <= 0.0f) {
            m_RandomDir.x = randf(-1.0f, 1.0f);
            m_RandomDir.y = 0.0f;
            m_RandomDir.z = randf(-1.0f, 1.0f);
            m_RandomDir.Normalise();
            m_RandomDirTimer = XGetNominalFrameRate() * randf(1.0f, 2.0f);
        }
        if (m_ActorFlags.m_bLeftArmMissing || m_ActorFlags.m_bRightArmMissing) {
            float dmg = -(GetProfile()->m_Health * 0.14285715f / XGetNominalFrameRate() * XGetFrameRateMul());
            if (m_ActorFlags.m_bLeftArmMissing && m_ActorFlags.m_bRightArmMissing) dmg *= 2.0f;
            ChangeHealth(dmg, 1);
        }
        if (m_ActorFlags.m_bBoss) {
            switch (GetProfile()->m_SubGroup) {
            case 0x4f00000e:
            case 0x4f000021:
                FEGetHud(1)->RegisterBossHit(m_RefID);
                break;
            case 0x4f000016:
                if (gCurrentMap_GeoHash == 0x100015b) FEGetHud(1)->RegisterBossHit(m_RefID);
                break;
            }
        }
        if (m_ActorFlags.m_bUlysses && GetProfile()->m_Hash == 0x47000329) FEGetHud(1)->RegisterBossHit(m_RefID);
        if (gCurrentMap_GeoHash == 0x1000124 && GetProfile()->m_SubGroup == 0x4f000021 && !m_ActorFlags2.m_bFleeToStatue &&
            GetProfile()->m_Health * 0.2f > m_Health) {
            m_InvinsibleCounter = 9999999.0f;
            m_StateFlags.m_bDontDoHitReaction = 1;
            m_ActorFlags2.m_bFleeToStatue = 1;
        }
        if (m_Health <= 0.0f && !m_ActorFlags.m_bUlysses) m_Unk8f4 -= XGetFrameRateMul();
        m_WatchFlags = 0xffffffff;
        SendMsg(MsgItem(MSG_EXT), 0.0f);
        if (!m_pItem) return 1;
        UpdateSeeingPlayer();
        UpdateSeeingBodies();
        UpdateFlagChannel();
        UpdateBossTests();
        UpdateShadow();
        UpdateCheckDeath();
        UpdateMovement();
        GetActorBasePtr(m_RefID)->m_Pos = m_pItem->Position();
        UpdateAnimType(-1);
        if (m_AimingTarget) SetupDepthPtr(&m_AimingTime, 0);
        UpdateCarry();
        UpdateLostLimbs();
        UpdateHeadTrack();
        UpdateCivilianAttackVO();
        UpdateSFX();
        UpdateChangeSkin();
        UpdateCloak();
        UpdateContextIcon();
        UpdateVisionMode();
        UpdateFallInOcean();
        UpdateLastHitTimer();
    }
    if (!m_ActorFlags.m_bAlien && Actor_ReachedNonCriticalUpdateFrame(m_NonCriticalFrame)) DoNonCriticalUpdate();
    UpdateDistFade();
    UpdateAlpha();
    EffectUpdate();
    if (!Actor_GetPause()) {
        m_HitCountdown -= GetFrameRateMul();
        DecrementDodgeTimer();
    }
    return 1;
}

void Actor::Update()
{
    float d1 = XGetCamera()->Zoom() * 10.0f;
    float d2 = XGetCamera()->Zoom() * 20.0f;
    float d3 = XGetCamera()->Zoom() * 50.0f;
    g_LodDetailsOnScreenUlysses[1].m_dist = d1;
    g_LodDetailsOnScreenUlysses[2].m_dist = d2;
    g_LodDetailsOnScreenUlysses[3].m_dist = d3;
    g_LodDetailsOnScreenBoss[1].m_dist = d1;
    g_LodDetailsOnScreenBoss[2].m_dist = d2;
    g_LodDetailsOnScreenBoss[3].m_dist = d3;
    g_LodDetailsOnScreenAlien[1].m_dist = d1;
    g_LodDetailsOnScreenAlien[2].m_dist = d2;
    g_LodDetailsOnScreenAlien[3].m_dist = d3;
    g_LodDetailsOnScreenDrone[1].m_dist = d1;
    g_LodDetailsOnScreenDrone[2].m_dist = d2;
    g_LodDetailsOnScreenDrone[3].m_dist = d3;
    gAlphaPulse += XGetFrameRateMul() * 0.1f;
    if (gAlphaPulse > 6.2831855f) gAlphaPulse = 0.0f;
    for (Actor *p = GetFirst(0); p; p = p->GetNext(0)) {
        if (p->NeedsDelete()) continue;
        if (!GetActorBasePtr(p->m_RefID)->m_bInWorld &&
            GeoSectionManager::IsLoaded(p->GetSkin()->m_File, p->GetSkin()->m_Hash) &&
            GeoSectionManager::IsLoaded(p->GetSkin()->m_ChangeFile, p->GetSkin()->m_ChangeHash) &&
            GeoSectionManager::IsLoaded(p->GetSkin()->m_SkelFile, p->GetSkin()->m_SkelHash))
            p->AddToWorld();
        if (g_bDumpActorInfo) p->DumpDebugInfo();
    }
    g_bDumpActorInfo = 0;
    if (gTimer_OverridePlayerCloak > 0.0f) gTimer_OverridePlayerCloak -= GetFrameRateMul();
    // The compiled-out trace calls of Actor.obj (XTrace.h): their real call sites are unknown (they emit no
    // code), they are referenced here only so their [fi] copies are emitted.
    TRBUILDTRACE();
    TRTRACE();
    TRTRACE_RAW();
    TRACE_MH();
    VCLTRACE();
    SW_TRACE();
    KG_TRACE();
    BUFFY_TRACE();
    GSM_TRACE();
    MsgQueueProcess();
}
int Actor::GetDeathSFX()
{
    if (m_ActorFlags.m_bUlysses) return 0;
    switch (GetSkin()->m_VoiceType) {
    case 1:
        return 0x1af000ed;
    case 2:
        return 0x1af000ee;
    case 3:
        return 0x1af004e8;
    case 4:
        return 0x1af004c2;
    case 7:
        return 0x1af004e4;
    case 8:
        return 0x1af005ef;
    case 9:
        return 0x1af004eb;
    case 10:
        return 0x1af004c4;
    case 11:
        return 0x1af004c6;
    case 15:
        return 0x1af004d0;
    case 16:
        return 0x1af005b7;
    case 13:
        return 0x1af005e9;
    case 5:
        return 0x1af006be;
    case 6:
        return 0x1af006cb;
    case 12:
        return 0x1af006d5;
    case 14:
        return 0x1af006b5;
    }
    return 0;
}

int Actor::GetScreamSFX()
{
    switch (GetSkin()->m_VoiceType) {
    case 1:
        return 0x1af000e0;
    case 2:
        return 0x1af000df;
    case 3:
        return 0x1af004ed;
    case 4:
        return 0x1af004ee;
    case 7:
        return 0x1af004e2;
    case 8:
        return 0x1af005ed;
    case 9:
        return 0x1af004e9;
    case 10:
        return 0x1af004d9;
    case 11:
        return 0x1af004dc;
    case 15:
        return 0x1af004de;
    case 16:
        return 0x1af005c2;
    case 13:
        return 0x1af005e7;
    case 5:
        return 0x1af006b9;
    case 6:
        return 0x1af006c6;
    case 12:
        return 0x1af006d2;
    case 14:
        return 0x1af006b2;
    }
    return 0;
}

int Actor::GetHitSFX()
{
    switch (GetSkin()->m_VoiceType) {
    case 1:
        return 0x1af000eb;
    case 2:
        return 0x1af000ec;
    case 3:
        return 0x1af004e7;
    case 4:
        return 0x1af004c0;
    case 7:
        return 0x1af004e3;
    case 8:
        return 0x1af005ee;
    case 9:
        return 0x1af004ea;
    case 10:
        return 0x1af004c3;
    case 11:
        return 0x1af004c5;
    case 15:
        return 0x1af004cf;
    case 16:
        return 0x1af005ba;
    case 13:
        return 0x1af005e8;
    case 5:
        return 0x1af006bd;
    case 6:
        return 0x1af006ca;
    case 12:
        return 0x1af006d4;
    case 14:
        return 0x1af006b4;
    }
    return 0;
}

int Actor::GetDazedSFX()
{
    switch (GetSkin()->m_VoiceType) {
    case 1:
        return 0x1af000f2;
    case 2:
        return 0x1af000f1;
    case 3:
        return 0x1af00504;
    case 4:
        return 0x1af0050c;
    case 7:
        return 0x1af005c8;
    case 8:
        return 0x1af005c7;
    case 9:
        return 0x1af005c6;
    case 10:
        return 0x1af005c5;
    case 11:
        return 0x1af005c4;
    case 15:
        return 0x1af005c3;
    case 16:
        return 0x1af005b8;
    case 13:
        return 0x1af005ea;
    case 5:
        return 0x1af006bf;
    case 6:
        return 0x1af006cc;
    case 12:
        return 0x1af006d0;
    case 14:
        return 0x1af006b0;
    }
    return 0;
}

int Actor::SendMsg(const MsgItem &msg, float delay)
{
    if (delay != 0.0f) {
        MsgQueueAdd(msg, this, delay);
        return 0;
    }
    m_SM.m_Unk9c = 0;
    m_SM.Run(msg);
    if (!m_SM.m_State) m_SM.Goto(0, AP_GetStateFromHash(GetProfile()->m_IdleState), 0, 0, 0, 0);
    return m_SM.m_Unk9c;
}

signed char Actor::CanReachZone(int zone)
{
    if (m_OrgLoc.NavZone() == zone) return 1;
    if (!m_NavRoute.IsDynamic() || !m_ActorFlags.m_bBoss || m_NavRoute.JumpDisabled()) return 0;
    XNavMesh *pMesh = m_OrgLoc.GetMesh();
    if (!pMesh || !pMesh->NavNetwork()->ZonesAreLinked(m_OrgLoc.NavZone(), zone, 0)) return 0;
    return 1;
}

signed char Actor::GotoPosDelta(const EXVector &delta)
{
    m_Unk08 = GetFeetPos();
    m_DestLoc = m_OrgLoc;
    u32 ignore = m_DestLoc.GetIgnore();
    m_DestLoc.Ignore(ignore | 0x8c8);
    m_DestLoc.Move(delta, 0);
    m_StateFlags.m_bMoving14 = 0;
    m_StateFlags.m_bMoving27 = 0;
    m_StateFlags.m_bMoving15 = 1;
    m_DestLoc.Ignore(ignore);
    return CanReachZone(m_DestLoc.NavZone());
}

signed char Actor::CheckBossDamageTriggers(float damage)
{
    ActorProfile *p = GetProfile();
    if (p && (m_WatchFlags & 4)) {
        if (p->m_LowHealthTrigger && !IsBitMsgOnStack(AW_BIT_LOW_HEALTH)) {
            float t = p->m_Health * p->m_GunSwapHealthPct * 0.01f;
            if (t < m_Health && m_Health - damage <= t) {
                m_StateFlags.m_b2 = 1;
                return 1;
            }
        }
        if (p->m_LargeDamageTrigger && !IsBitMsgOnStack(AW_BIT_LARGE_DAMAGE) &&
            p->m_Health * p->m_LargeDamagePct * 0.01f <= damage) {
            m_StateFlags.m_b1 = 1;
            return 1;
        }
    }
    return 0;
}

void Actor::ChangeSkin(unsigned int hash, unsigned int file, unsigned int item)
{
    if (m_pChangeSkinItem || !hash || hash == GetSkin()->m_Hash) return;
    AS_GetIndex(hash, m_NewSkinIndex);
    if (!AS_GetPtrFromIndex(m_NewSkinIndex)) return;
    m_ActorFlags2.m_bScriptRunControl = 0;
    m_pChangeSkinItem = EXItemAnimator_Script::NewItem();
    EXGeoFile *pFile = EXGeoFile::GetGeoFile(file);
    m_pChangeSkinItem->Initialise(pFile, item);
    m_pItem->AddAnimator(m_pChangeSkinItem);
    ActorSkin *pSkin = AS_GetPtrFromIndex(m_NewSkinIndex);
    GeoSectionManager::Add(pSkin->m_File, pSkin->m_Hash);
    GeoSectionManager::Add(pSkin->m_ChangeFile, pSkin->m_ChangeHash);
}

float Actor::SqDistToTarget()
{
    if (!m_StateFlags.m_bMoving15) return 0.0f;
    EXVector d = m_DestLoc.Pos() - GetFeetPos();
    EXVector w = d;
    if (fabs(w.y) < 0.5f) w.y = 0.0f;
    return w.x * w.x + w.y * w.y + w.z * w.z;
}

void Actor::NotifyCutscene(signed char bStart)
{
    if (!bStart) return;
    for (Actor *p = GetFirst(0); p; p = p->GetNext(0)) {
        p->m_SFXTag.RemoveAll(1);
        p->m_SFXTag.KillSoundPlayingLooping();
        ActorCarryable *pCarry = ActorCarryable::GetPtrFromID(p->m_CarryLeft);
        if (pCarry) pCarry->StopLoopingSFX();
        pCarry = ActorCarryable::GetPtrFromID(p->m_CarryRight);
        if (pCarry) pCarry->StopLoopingSFX();
    }
}

void Actor::CalcMass()
{
    if (m_ActorFlags.m_bUlysses) {
        m_Mass = 100.0f;
        m_ActorToPlayerColAdj = 0.5f;
        m_PlayerToActorColAdj = 1.0f;
    } else if (m_ActorFlags.m_bBoss) {
        m_Mass = 10.0f;
        m_ActorToPlayerColAdj = 1.0f;
        m_PlayerToActorColAdj = 1.0f;
    } else if (m_ActorFlags.m_bAlien) {
        m_Mass = 10.0f;
        m_ActorToPlayerColAdj = 1.0f;
        m_PlayerToActorColAdj = 0.0f;
    } else {
        m_Mass = 1.0f;
        m_ActorToPlayerColAdj = 1.0f;
        m_PlayerToActorColAdj = 0.7f;
    }
}

void Actor::MsgQueueAdd(const MsgItem &msg, Actor *pActor, float delay)
{
    if (!pActor) return;
    if (delay < 0.0f) delay = g_EXRandClass.Randf() * 0.5f + 0.25f;
    int i;
    for (i = 0; i < 128; i++) {
        if (!(m_MsgQueue[i].m_Delay > 0.0f)) break;
        if (m_MsgQueue[i].m_ActorID == pActor->m_RefID && m_MsgQueue[i].m_Msg.m_Type == msg.m_Type) return;
    }
    if (i >= 128) return;
    m_MsgQueue[i].m_Delay = delay * 60.0f;
    m_MsgQueue[i].m_ActorID = pActor->m_RefID;
    m_MsgQueue[i].m_Msg = msg;
    m_MsgQueueCount = max(m_MsgQueueCount, i + 1);
}

EXVector Actor::GetPelvisPos()
{
    unsigned int frame = XGetGameWnd()->FrameCounter();
    if (frame == m_PelvisPosFrame) return m_PelvisPosCache;
    if (!m_StateFlags.m_bOnScreen) return GetPelvisPosApprox();
    m_PelvisPosFrame = frame;
    EXDatumTemp d;
    if (!m_pItem->GetAnimatorDatum(0xe000001, d, 0, 0)) return EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    m_PelvisPosCache = d.m_position;
    return m_PelvisPosCache;
}

EXVector Actor::GetHeadPos()
{
    unsigned int frame = XGetGameWnd()->FrameCounter();
    if (frame == m_HeadPosFrame) return m_HeadPosCache;
    if (!m_StateFlags.m_bOnScreen) return GetHeadPosApprox();
    m_HeadPosFrame = frame;
    EXDatumTemp d;
    if (!m_pItem->GetAnimatorDatum(0xe000009, d, 0, 0)) return EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    m_HeadPosCache = d.m_position;
    return m_HeadPosCache;
}

float Actor::DistToTarget()
{
    if (!m_StateFlags.m_bMoving15) return 0.0f;
    EXVector d = m_DestLoc.Pos() - GetFeetPos();
    EXVector w = d;
    if (fabs(w.y) < 0.5f) w.y = 0.0f;
    return EXSqrt(w.x * w.x + w.y * w.y + w.z * w.z);
}

void Actor::UpdateChangeSkin()
{
    ActorSkin *pNew = AS_GetPtrFromIndex(m_NewSkinIndex);
    if (pNew && m_ActorFlags2.m_bScriptRunControl && GeoSectionManager::IsLoaded(pNew->m_File, pNew->m_Hash) &&
        GeoSectionManager::IsLoaded(pNew->m_ChangeFile, pNew->m_ChangeHash)) {
        AU_AssignSkin(m_pAnim, pNew->m_File, pNew->m_Hash);
        ActorSkin *pOld = AS_GetPtrFromIndex(m_SkinIndex);
        GeoSectionManager::Del(pOld->m_File, pOld->m_Hash);
        GeoSectionManager::Del(pOld->m_ChangeFile, pOld->m_ChangeHash);
        m_SkinIndex = m_NewSkinIndex;
        m_NewSkinIndex.m_Sheet = 0xffff;
        m_NewSkinIndex.m_Row = 0xffff;
        ActorTextureSwap::Initialise(this);
    }
    if ((m_Health <= 0.0f && !m_ActorFlags.m_bBoss) || (m_pChangeSkinItem && m_pChangeSkinItem->m_11a)) DeleteChangeSkin();
}

void Actor::SpinToYRot(float rot)
{
    while (rot < 0.0f) rot += 6.2831855f;
    while (rot > 6.2831855f) rot -= 6.2831855f;
    float cur = m_pItem->Rotation().y;
    float d = XNavNetwork::AngleDifference(cur, rot);
    float max = EXBaseApp::GameFrameRateMul() * 3.1415927f / (EXBaseApp::NominalGameFrameRate() * 1.2f);
    if (d > max) d = max;
    else if (d < 0.0f - max) d = 0.0f - max;
    m_TargetRotY = d + cur;
    m_StateFlags.m_bMoving27 = 0;
    m_StateFlags.m_bMoving14 = 1;
}

void Actor::DoScriptCmdSound(const EXItemAnimator *pAnim, const GEOSCRIPT_SOUND *pCmd, EXSoundScriptTemp &t)
{
    EXItem *pItem = (EXItem *)pAnim->GetBaseOwner();
    if (!pItem || !pItem->ItemHandler()) return;
    u32 i;
    if (pCmd->m_flags & 0x8000) {
        i = randi(0, max(0, pCmd->m_count - 1));
    } else {
        ActorSkin *pSkin = AS_GetPtrFromIndex(((Actor *)pItem->ItemHandler())->m_SkinIndex);
        i = 0;
        if (pSkin) i = max(0, pSkin->m_VoiceType - 1);
        i = (int)i < (int)pCmd->m_count ? i : 0;
    }
    t.m_Hash = RemapSFXHashCode(pCmd->m_hashes[i]);
}

// Script hooks for actor animators: script sounds pick the actor's voice set.
static EXAnimatorScriptCallbacks gActorScriptCallbacks(0, 0, 0, Actor::DoScriptCmdSound);

void Actor::SetAnimMode(int mode, signed char bForce)
{
    if (m_StateFlags.m_bRagDoll || m_ActorFlags2.m_bIgnoreAnimChanges) return;
    if (mode && (mode & 0xff000000) != 0x9000000) {
        ACTOR_TRACE("SetAnimMode() - not an anim mode %s", EXHashCodeFile::Debug_HashCodeName(mode));
        return;
    }
    if (!bForce && m_AnimMode == mode) return;
    if (m_pAnim) {
        m_pAnim->RegisterCallbacks(&gActorScriptCallbacks);
        if (!m_pAnim->ChangeToAnimMode(m_AnimMode, mode, 0x9000004) && mode)
            ACTOR_TRACE("SetAnimMode() - failed at %f %s", GetHeadPos(), EXHashCodeFile::Debug_HashCodeName(mode));
        u32 ctrl = m_Unk7a8;
        if (ctrl) {
            EXItemCtrl *p = m_pAnim->Controller(0);
            p->m_ctrlFlags.BitSet(ctrl);
            ((EXItemCtrl_Anim *)p)->m_motionFlags = (u16)m_AnimMotionOverride;
        }
    }
    m_AnimMode = mode;
}

void Actor::SetAnimType(int type, signed char bAiming, signed char bResetAim)
{
    ActorStateFlags sf = m_StateFlags;
    if (sf.m_bRagDoll) return;
    m_ActorFlags.m_bAiming = bAiming != 0;
    if (bResetAim) {
        m_AimingTime = 0.0f;
        m_AimingTarget = 0;
    }
    if (m_AnimType == type) return;
    if (IsAnimVariableSpeed(type, this)) sf.m_bAnimSpeedRand = 1;
    else sf.m_bAnimSpeedRand = 0;
    m_StateFlags = sf;
    UpdateAnimType(type);
    if (IsAnimVariableStart(type, this)) {
        float r = EXRandf();
        if (m_pAnim) m_pAnim->SetObjectFrame(GetAnimFrameCount() * r);
    }
}

signed char Actor::GetAnimDatum(int hash, EXVector &pos)
{
    if (!m_pAnim) return 0;
    EXDatumTemp d;
    memset(&d, 0, sizeof(d));
    if (!m_pAnim->GetFrameDatum(hash, d, 0)) return 0;
    pos = d.m_position + GetFeetPos();
    Actor_CalledGetDatum++;
    return 1;
}

signed char Actor::IsHeadshot(EXVector &pos, float radius)
{
    Actor_CalledGetDatum++;
    EXDatumTemp d;
    if (!m_pItem->GetAnimatorDatum(0x1000004a, d, 0, 0)) return 0;
    EXVector v = pos - d.m_position;
    EXVector w = v;
    if (w.y * w.y + w.z * w.z + w.x * w.x < radius * radius) return 1;
    return 0;
}

EXVector Actor::GetChestPos()
{
    unsigned int frame = XGetGameWnd()->FrameCounter();
    if (frame == m_ChestPosFrame) return m_ChestPosCache;
    if (!m_StateFlags.m_bOnScreen) return GetChestPosApprox();
    m_ChestPosFrame = frame;
    EXDatumTemp d;
    if (!m_pItem->GetAnimatorDatum(m_ActorFlags.m_bUlysses ? 0xe000003 : 0xe000005, d, 0, 0))
        return EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    m_ChestPosCache = d.m_position;
    return m_ChestPosCache;
}

EXVector Actor::GetWeaponPos(signed char bLeft)
{
    EXVector pos;
    ActorCarryable *pCarry = ActorCarryable::GetPtrFromID(bLeft ? m_CarryLeft : m_CarryRight);
    if (pCarry && pCarry->GetWeaponCollisionPos(pos)) return pos;
    EXDatumTemp d;
    memset(&d, 0, sizeof(d));
    int hash;
    if (!bLeft && GetSkin()->m_Hash == 0xd0000b9) hash = 0x10000039;
    else hash = 0x10000014 - (bLeft != 0);
    if (!m_pItem->GetAnimatorDatum(hash, d, 0, 0)) {
        ACTOR_TRACE("GetWeaponPos() - no weapon datum on %s", EXHashCodeFile::Debug_HashCodeName(GetProfile()->m_Hash));
        d.m_position = EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    }
    return d.m_position;
}

float Vec_AngleDifference(float a, float b);  // Vec.obj

signed char Actor::FriendlyActorBlockingLineOfSight(const EXRay &ray)
{
    ActorProfile *pMe = GetProfile();
    for (Actor *p = GetFirst(ray.Origin(), 100.0f, 0); p; p = p->GetNext(ray.Origin(), 100.0f, 0)) {
        if (p == this) continue;
        if (!AP_DoWeFight(pMe, p->GetProfile()) && p->CheckRayIntersection(ray)) return 1;
    }
    return 0;
}

void Actor::SendMsg(const MsgItem &msg, const EXVector &pos, const EXVector &dir, float radius, float angle,
                    Actor *pFrom, float delay)
{
    for (Actor *p = GetFirst(pos, radius, 0); p; p = p->GetNext(pos, radius, 0)) {
        const EXMaths::vec_xyzw &d = p->GetFeetPos() - pos;
        if (fabs(Vec_AngleDifference(qatan2f(d.x, d.z), qatan2f(dir.x, dir.z))) <= angle && p != pFrom)
            p->SendMsg(msg, delay);
    }
}

signed char Actor::EffectCreate(unsigned int file, unsigned int hash, unsigned int datum, float time, signed char b,
                                unsigned __int64 id)
{
    if (m_ActorFlags2.m_bBeingDestructed) return 0;
    if (hash > 0x4000000 && hash < 0x40007d9 && !datum) {
        new ActorEffect(this, file, hash, 0x1000000d, time, b, id);
        new ActorEffect(this, file, hash, 0x1000001f, time, b, id);
        return 1;
    }
    new ActorEffect(this, file, hash, datum, time, b, id);
    return 1;
}

void Actor::SayAttackVO()
{
    switch (GetSkin()->m_VOType) {
    case 4:
        Speak(0x1af004d3, 0);
        break;
    case 5:
        Speak(0x1af004c7, 0);
        break;
    case 6:
        Speak(0x1af004cb, 0);
        break;
    case 12:
        Speak(0x1af004d1, 0);
        break;
    case 26:
        Speak(0x1af004d2, 0);
        break;
    case 16:
        Speak(0x1af00075, 0);
        break;
    case 30:
        Speak(0x1af00161, 0);
        break;
    case 31:
        Speak(0x1af00537, 0);
        break;
    case 29:
        if (gCurrentMap_GeoHash == 0x1000124) Speak(0x1af006f1, 0);
        else Speak(0x1af005ce, 0);
        break;
    }
}

void Actor::SayAttackBigVO()
{
    switch (GetSkin()->m_VOType) {
    case 4:
        Speak(0x1af004d4, 0);
        break;
    case 5:
        Speak(0x1af004c8, 0);
        break;
    case 6:
        Speak(0x1af004cc, 0);
        break;
    case 12:
        Speak(0x1af00519, 0);
        break;
    case 26:
        Speak(0x1af0051b, 0);
        break;
    case 16:
        Speak(0x1af00449, 0);
        break;
    case 30:
        Speak(0x1af00518, 0);
        break;
    case 31:
        Speak(0x1af00538, 0);
        break;
    case 29:
        if (gCurrentMap_GeoHash == 0x1000124) Speak(0x1af006f2, 0);
        else Speak(0x1af005cf, 0);
        break;
    }
}

void Actor::PushState(StateFn state, unsigned int flags)
{
    if (m_StackCount >= 10) {
        ACTOR_TRACE("PushState() - stack overflow %s", EXHashCodeFile::Debug_HashCodeName(GetProfile()->m_Hash));
        return;
    }
    memcpy(m_Stack[m_StackCount].m_LocalData, m_SM.m_LocalData, sizeof(m_SM.m_LocalData));
    m_Stack[m_StackCount].m_AnimMode = m_AnimMode;
    m_Stack[m_StackCount].m_AnimType = m_AnimType;
    m_Stack[m_StackCount].m_State = state;
    m_Stack[m_StackCount].m_bGotoPos = m_StateFlags.m_bMoving15;
    m_Stack[m_StackCount].m_DestPos = (const EXVector3 &)m_DestLoc.Pos();
    m_Stack[m_StackCount].m_DestPoly = m_DestLoc.GetPolyHandle();
    m_Stack[m_StackCount].m_MsgBits = flags;
    m_StackCount++;
}

StateFn Actor::PopState()
{
    if (m_StackCount < 1) return 0;
    float rot = m_pItem->Rotation().y;
    m_StackCount--;
    m_StateFlags.m_bMoving27 = 0;
    m_StateFlags.m_bMoving14 = 1;
    m_TargetRotY = rot;
    memcpy(m_SM.m_LocalData, m_Stack[m_StackCount].m_LocalData, sizeof(m_SM.m_LocalData));
    if (!m_ActorFlags2.m_bBeingDestructed) SetAnimMode(m_Stack[m_StackCount].m_AnimMode, 0);
    ActorStack &s = m_Stack[m_StackCount];
    m_AnimType = s.m_AnimType;
    m_StateFlags.m_bMoving15 = s.m_bGotoPos;
    if (m_StateFlags.m_bMoving15)
        m_DestLoc.SetPosPoly(EXVector(s.m_DestPos, 0.0f), s.m_DestPoly, 0);
    return m_Stack[m_StackCount].m_State;
}

float Actor::DistFromSource()
{
    const EXMaths::vec_xyzw &d = m_Unk08 - GetFeetPos();
    return EXSqrt(d.x * d.x + d.z * d.z);
}

void Actor::SayNudgedVO()
{
    switch (GetSkin()->m_VOType) {
    case 1:
        Speak(0x1af00534, 0);
        break;
    case 4:
        Speak(0x1af00536, 0);
        break;
    case 7:
        Speak(0x1af00535, 0);
        break;
    case 8:
        Speak(0x1af00533, 0);
        break;
    case 9:
        Speak(0x1af00531, 0);
        break;
    case 10:
        Speak(0x1af00530, 0);
        break;
    case 11:
        Speak(0x1af0052f, 0);
        break;
    case 12:
        Speak(0x1af005f2, 0);
        break;
    case 15:
        Speak(0x1af00532, 0);
        break;
    case 21:
        Speak(0x1af00692, 0);
        break;
    case 24:
        Speak(0x1af006a5, 0);
        break;
    case 25:
        Speak(0x1af00693, 0);
        break;
    }
}

signed char Actor::CarryingMixedWeapons() const
{
    if (!m_CarryLeft) {
        if (m_CarryRight && GetProfile()->m_Weapon2 == 0x200016e) {
            ActorCarryable *p = ActorCarryable::GetPtrFromID(m_CarryRight);
            if (p && ActorCarryable::FindCarryTypeForEntity(p->m_EntityHash) == 9) return 1;
        }
        return 0;
    }
    if (!m_CarryRight) {
        if (GetProfile()->m_Weapon2 != 0x200016e) return 0;
        ActorCarryable *p = ActorCarryable::GetPtrFromID(m_CarryLeft);
        if (!p) return 0;
        return ActorCarryable::FindCarryTypeForEntity(p->m_EntityHash) == 9;
    }
    ActorCarryable *pL = ActorCarryable::GetPtrFromID(m_CarryLeft);
    ActorCarryable *pR = ActorCarryable::GetPtrFromID(m_CarryRight);
    if (!pL || !pR) return 0;
    int l = pL->m_EntityHash;
    int r = pR->m_EntityHash;
    if (l == 0x2000139 || l == 0x20000dc) return 0;
    int tl = ActorCarryable::FindCarryTypeForEntity(l);
    int tr = ActorCarryable::FindCarryTypeForEntity(r);
    if (tl == 9) return tr == 6;
    if (tl == 6) return tr == 9;
    return 0;
}

void Actor::UpdateCivilianAttackVO()
{
    if (!SubGroupAnims::GetSubGroupAnims(m_SubGroupAnimIndex).m_Anims[16] || m_Health <= 0.0f) return;
    if (m_CivilianAttackVOTimer > 0.0f) m_CivilianAttackVOTimer -= XApp::GameFrameRateMul();
    if (m_SpeechTag.IsPlaying() || m_CivilianAttackVOTimer <= 0.0f) return;
    if (m_CivilianAttackVOTimer >= EXBaseApp::NominalGameFrameRate() * 3.0f) return;
    switch (GetSkin()->m_VOType) {
    case 19:
        Speak(0x1af0062f, 0);
        break;
    case 20:
        Speak(0x1af00630, 0);
        break;
    case 22:
        Speak(0x1af006a6, 0);
        break;
    case 23:
        Speak(0x1af0064f, 0);
        break;
    default:
        if (GetSkin()->m_Type == 1) Speak(0x1af0042a, 0);
        else Speak(0x1af0042b, 0);
        break;
    }
    m_CivilianAttackVOTimer = 0.0f;
}

signed char Actor::IsPlayerStoodOnMe() const
{
    float r = m_CollisionRadius + 0.5f;
    const EXMaths::vec_xyzw &d = GetFeetPos() - Player_GetFeetPos();
    if (d.z * d.z + d.x * d.x <= r * r && fabs(d.y) < 2.0f) return 1;
    return 0;
}

WeaponProfile *Actor::GetWeaponProfile(signed char bLeft)
{
    ActorCarryable *p = bLeft ? ActorCarryable::GetPtrFromID(m_CarryLeft) : ActorCarryable::GetPtrFromID(m_CarryRight);
    if (p) return p->GetWeaponProfile();
    switch (GetProfile()->m_SubGroup) {
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
    }
    if (m_ActorFlags.m_bBoss) return WeaponProfile::GetWeaponData(0x49000036);
    return WeaponProfile::GetWeaponData(0x49000034);
}

void ActorEffect::Init(Actor *pActor, unsigned int file, unsigned int hash, unsigned int datum, float time, signed char b,
                       unsigned __int64 id)
{
    m_pActor = pActor;
    m_File = file;
    m_Hash = hash;
    m_Datum = datum;
    m_pScript = 0;
    m_bTimer = time > 0.0f;
    m_Timer = time * 60.0f;
    m_bTimed = 0;
    m_bStopOnScriptEnd = b != 0;
    m_ID = id;
    if (hash > 0x4000000 && hash < 0x40007d9) {
        EXGeoFile *pFile = EXGeoFile::GetGeoFile(file);
        m_pScript = new EXItemAnimator_Script;
        m_pScript->Initialise(pFile, m_Hash);
        m_pScript->RenderInfo().m_renderFlags.BitSet(8);
        m_pScript->m_renderFlags.BitSet(1);
        m_pActor->GetItem()->AddAnimator(m_pScript);
    } else {
        EXGeoRefObject r;
        r.Set(EXGeoFile::GetGeoFile(file), hash);
        pActor->GetAnim()->AddCloneSkin(r);
    }
    m_pActor->EffectList()->AddTail(&m_Link);
}

#define ACTOR_SUBGROUP_NOGRAB \
    case 0x4f000006: case 0x4f00000a: case 0x4f00000b: case 0x4f00000c: case 0x4f00000d: \
    case 0x4f00000e: case 0x4f00000f: case 0x4f000010: case 0x4f000011: case 0x4f000012: \
    case 0x4f000013: case 0x4f000014: case 0x4f000015: case 0x4f000016: case 0x4f000017: \
    case 0x4f000019: case 0x4f000033

signed char Actor::CanBeJuggled() const
{
    if (m_StateFlags.m_bLyingOnGround) return 0;
    if (IsDead()) return 0;
    switch (GetSkin()->m_SkelHash) {
    case 0xd00008d: case 0xd000091: case 0xd000103: case 0xd000104:
        return 0;
    }
    switch (GetProfile()->m_SubGroup) {
    ACTOR_SUBGROUP_NOGRAB:
        return 0;
    }
    switch (GetProfile()->m_Hash) {
    case 0x4700019b: case 0x470001f2: case 0x4700022f: case 0x4700033d:
    case 0x47000340: case 0x47000379: case 0x47000386:
        return 0;
    }
    switch (GetSkin()->m_Hash) {
    case 0xd000138: case 0xd00013b: case 0xd00016b:
        return 0;
    }
    if (SearchBrainStateStack(STATE_Barge)) return 0;
    return m_ActorFlags.m_bCanBeJuggled == 1;
}

signed char Actor::CanBeStealthKilled() const
{
    if (m_StateFlags.m_bLyingOnGround) return 0;
    if (IsDead()) return 0;
    switch (GetSkin()->m_SkelHash) {
    case 0xd00008d: case 0xd000091: case 0xd000103: case 0xd000104:
        return 0;
    }
    switch (GetProfile()->m_SubGroup) {
    ACTOR_SUBGROUP_NOGRAB:
        return 0;
    }
    switch (GetProfile()->m_Hash) {
    case 0x4700019b: case 0x470001f2: case 0x4700022f:
    case 0x47000340: case 0x47000379: case 0x47000386:
        return 0;
    }
    switch (m_AnimMode) {
    case 0x9000176: case 0x9000177: case 0x9000190: case 0x9000494: case 0x4700033d:
        return 0;
    }
    switch (GetSkin()->m_Hash) {
    case 0xd000138: case 0xd00013b: case 0xd00016b:
        return 0;
    }
    return 1;
}

signed char Actor::CanBeGrabbed() const
{
    if (m_StateFlags.m_bLyingOnGround) return 0;
    if (IsDead()) return 0;
    switch (GetSkin()->m_SkelHash) {
    case 0xd00008d: case 0xd000091: case 0xd000103: case 0xd000104:
        break;
    default:
        switch (GetProfile()->m_SubGroup) {
        ACTOR_SUBGROUP_NOGRAB:
            break;
        default:
            switch (GetProfile()->m_Hash) {
            case 0x4700019b: case 0x470001f2: case 0x4700022f: case 0x4700033d:
            case 0x47000340: case 0x47000379: case 0x47000386:
                break;
            default:
                switch (GetSkin()->m_Hash) {
                case 0xd000138: case 0xd00013b: case 0xd00016b:
                    break;
                default:
                    return 1;
                }
            }
        }
    }
    return 0;
}

signed char Actor::CanBeFinishedOff() const
{
    if (m_StateFlags.m_bLyingOnGround) return 0;
    if (IsDead()) return 0;
    switch (GetSkin()->m_SkelHash) {
    case 0xd00008d: case 0xd000091: case 0xd000103: case 0xd000104:
        break;
    default:
        switch (GetProfile()->m_SubGroup) {
        ACTOR_SUBGROUP_NOGRAB:
            break;
        default:
            switch (GetProfile()->m_Hash) {
            case 0x4700019b: case 0x470001f2: case 0x4700022f: case 0x4700033d:
            case 0x47000340: case 0x47000379: case 0x47000386:
                break;
            default:
                switch (GetSkin()->m_Hash) {
                case 0xd000138: case 0xd00013b: case 0xd00016b:
                    break;
                default:
                    return 1;
                }
            }
        }
    }
    return 0;
}

void Actor::PlayActorWeaponImpactSFX(int id, const EXVector &pos, WeaponProfile *wp, signed char bFixed)
{
    if (!wp) return;
    Actor *a = GetPtrFromID(id);
    if (a && a->m_ActorFlags.m_bUlysses) {
        if (wp->m_ClipSize && !bFixed) id = 0x1af0033c;
        else id = 0x1af0028d;
    } else {
        id = bFixed ? wp->m_SfxHitFixed : wp->m_SfxHit;
        if (!id) return;
    }
    XGetSoundManager()->StartSound(id, pos, -1, 0);
}

EXVector Actor::GetHeadDirection()
{
    m_HeadPosFrame = XGetGameWnd()->FrameCounter();
    EXDatumTemp d;
    if (!m_pItem->GetAnimatorDatum(0xe000009, d, 0, 0)) return EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    m_HeadPosCache = d.m_position;
    EXVector r;
    d.m_rotation.GetRotation(r, 4);
    EXVector o;
    o.y = 0.0f; o.w = 0.0f;
    o.x = (float)sin(r.y); o.z = (float)cos(r.y);
    return o;
}

void Actor::MafiaBossSheild(int secs)
{
    ActorEffect *p = EffectGet(0x5f5f6b726170735fULL);
    if (p) {
        p->SetTimer(secs);
        return;
    }
    EffectCreate(0x10000e3, 0xd000177, 0, (float)secs, 0, 0x5f5f6b726170735fULL);
}

signed char ActorEffect::DoUpdate()
{
    if (m_bTimer) {
        m_Timer -= XApp::GameFrameRateMul();
        if (m_Timer <= 0.0f) return 0;
    }
    signed char bDraw = 0;
    if (m_pScript) {
        bDraw = m_pScript->m_itemFlags & 1;
        u32 bHide = m_pActor->m_StateFlags.m_bHide;
        if (bHide && bDraw) {
            m_pScript->m_itemFlags.BitClr(1);
            m_pScript->EnableParticleSystems(0);
        } else if (bHide != 1 && !bDraw) {
            m_pScript->m_itemFlags.BitSet(1);
            m_pScript->m_validMtx = 0;
            m_pScript->EnableParticleSystems(1);
        }
    }
    if (m_pScript && bDraw && m_pActor->m_StateFlags.m_bOnScreen) {
        if (m_Datum) {
            Actor_CalledGetDatum++;
            EXDatumTemp d;
            memset(&d, 0, sizeof(d));
            if (!m_pActor->GetItem()->GetAnimatorDatum(m_Datum, d, 4, 0)) return 0;
            m_pScript->SetPosition(d.m_position);
        }
        if (m_bStopOnScriptEnd && m_pScript->m_11a) return 0;
    }
    return 1;
}

// GetAnimatorDatum returns the node it found; +0xc is that node's world matrix.
struct ActorDatumNode {
    u8 m_00[0xc];
    const EXMatrix *m_pMatrix;  // 0x0c
};

EXItemAnimator_Script *Actor::CreateBloodScript(int datum, int hash, XItem *pItem)
{
    EXItemAnimator_Script *p = new EXItemAnimator_Script;
    if (!p) return 0;
    EXGeoFile *pFile = EXGeoFile::GetGeoFile(0x1000016);
    if (!pFile) {
        delete p;
        return 0;
    }
    p->Initialise(pFile, hash);
    p->m_position = EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    p->m_rotation = EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    p->m_validMtx = 0;
    p->RenderInfo().m_renderFlags.BitSet(0xe);
    if (datum) {
        Actor_CalledGetDatum++;
        EXDatumTemp d;
        ActorDatumNode *pNode = (ActorDatumNode *)m_pItem->GetAnimatorDatum(datum, d, 0, 0);
        if (pNode) {
            p->m_renderFlags.BitSet(9);
            p->m_pRemoteMatrix = pNode->m_pMatrix;
        }
        m_pItem->AddAnimator(p);
        return p;
    }
    p->m_renderFlags.BitSet(1);
    if (pItem) pItem->AddAnimator(p);
    return p;
}

signed char Actor::NearToRecentLastKnownLocation() const
{
    if (m_AlertRaiseTimer > 0.0f && GetProfile()->m_AlertTime > 0.0f) return 0;
    if (gAIControl.m_LastKnownLocationTimer < (float)EXBaseApp::NominalGameFrameRate() * 15.0f) {
        const EXMaths::vec_xyzw &d = GetFeetPos() - gAIControl.m_LastSeenPlayerPos;
        EXVector w = d;
        if (w.x * w.x + w.y * w.y + w.z * w.z < 25.0f) return 1;
    }
    return 0;
}

ActorEffect::ActorEffect(Actor *pActor, unsigned int file, unsigned int hash, const EXVector &pos, float time,
                         signed char b, unsigned __int64 id)
{
    Init(pActor, file, hash, 0, time, b, id);
    EXMatrix m = m_pActor->GetItem()->mtx();
    EXVector p = pos;
    EXMatrix inv = m.Inverse();
    p.ApplyXForm(inv);
    m_pScript->SetPosition(p);
}

static inline EX6DParticleBase *RagDollRootBody(EX6DAnim *p) { return p->BodyCount() > 0 ? p->Body(0) : 0; }

EXVector Actor::GetRagDollVelocity() const
{
    EX6DAnim *pAnim = (EX6DAnim *)m_pItem->m_pItemPhysics;
    if (RagDollRootBody(pAnim)) {
        m_pItem->SetPosition(RagDollRootBody(pAnim)->m_pos);
        m_pItem->SetRotation(((EX6DRigidBody *)RagDollRootBody(pAnim))->m_rot);
    }
    EXVector v;
    v.SetZero();
    if (!m_StateFlags.m_bRagDoll) return EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    if (RagDollRootBody(pAnim)) v = RagDollRootBody(pAnim)->m_velocity;
    return v;
}

float Actor::GetAnimVelocity()
{
    int i = m_AnimMode - 0x9000000;
    if (i < 0 || i >= 0x54b || !s_AnimVelocity[i].m_bValid) return 0.0f;
    const ActorAnimVelocity &a = s_AnimVelocity[i];
    return (EXVector(a.m_Vel, 0.0f) * m_pAnim->m_frameRateMult).LengthXZ();
}

void Actor::ProcessHit(const EXVector &pos, const EXVector &dir, float damage)
{
    int best = -1;
    float bestDist = 0.0f;
    for (int i = 0; i < 4; i++) {
        if (i == ALL_BELT || i == ALL_HEAD) continue;
        if (IsLimbLost((ActorLostLimb)i)) continue;
        EXDatumTemp d;
        memset(&d, 0, sizeof(d));
        Actor_CalledGetDatum++;
        m_pItem->GetAnimatorDatum(s_LimbInfo[i].m_Datum, d, 0, 0);
        EXVector v = d.m_position - pos;
        float dist = v.x * v.x + v.y * v.y + v.z * v.z;
        if (best == -1 || dist < bestDist) {
            best = i;
            bestDist = dist;
        }
    }
    if (best != -1) CreateLostLimb((ActorLostLimb)best, dir, 0, 0);
}

void Actor::LooseArmIfHit(const EXVector &pos, const EXVector &dir, float damage)
{
    if (!IsDrone()) return;
    if (!CanBeStealthKilled()) return;
    if (SubGroupAnims::GetSubGroupAnims(*this).m_Anims[13]) return;
    if (!m_ActorFlags.m_bCanLooseArmAndLive) return;
    if (m_ActorFlags.m_bLeftArmMissing || m_ActorFlags.m_bRightArmMissing) return;
    int best = -1;
    float bestDist = 0.0f;
    for (int i = 0; i < 2; i++) {
        if (IsLimbLost((ActorLostLimb)i)) continue;
        EXDatumTemp d;
        memset(&d, 0, sizeof(d));
        Actor_CalledGetDatum++;
        m_pItem->GetAnimatorDatum(s_LimbInfo[i].m_Datum, d, 0, 0);
        EXVector v = d.m_position - pos;
        float dist = v.x * v.x + v.y * v.y + v.z * v.z;
        if (best == -1 || dist < bestDist) {
            best = i;
            bestDist = dist;
        }
    }
    if (best != -1) CreateLostLimb((ActorLostLimb)best, dir, 0, 0);
}

signed char Actor::CheckForSkinChange(float damage)
{
    signed char r = 0;
SubGroupAnims &s = SubGroupAnims::GetSubGroupAnims(m_SubGroupAnimIndex);
    if (s.m_Anims[3] > 0 && m_Health > 0.0f && m_Health - damage <= (float)s.m_Anims[3] && !m_StateFlags.m_bCloaked &&
        !m_StateFlags.m_bBeingStealthKilled) {
        SetCloak(1);
        r = 1;
    }
    if (s.m_Anims[1] > 0 && s.m_Anims[2] > 0 && !m_ActorFlags2.m_b12 && m_Health - damage <= (float)s.m_Anims[1]) {
        if (GetProfile()->m_SubGroup == 0x4f00000f) {
            unsigned int skin = 0xd000100;
            switch (GetSkin()->m_Hash) {
            case 0xd000085: skin = 0xd000100; break;
            case 0xd0000a2: skin = 0xd000107; break;
            case 0xd0000a3: skin = 0xd000108; break;
            case 0xd0000a4: skin = 0xd000109; break;
            }
            ChangeSkin(skin, 0x1000016, 0x400070a);
        } else {
            ChangeSkin(s.m_Anims[2], 0x1000016, 0x40002f2);
        }
        m_ActorFlags2.m_b12 = 1;
        r = 1;
    }
    return r;
}

void Actor::UpdateThrownDistStat()
{
    if (!(fabs(m_ThrownPos.x) < 0.001f && fabs(m_ThrownPos.y) < 0.001f && fabs(m_ThrownPos.z) < 0.001f &&
          fabs(m_ThrownPos.w) < 0.001f)) {
        float dist = (GetFeetPos() - m_ThrownPos).LengthXYZ() + 0.5f;
        if (m_ActorFlags2.m_bHasBeenKicked) {
            if (dist > XGetGameStats()->LongestKick()) XGetGameStats()->LongestKick() = dist;
        } else if (m_ActorFlags.m_bHasBeenThrow) {
            if (dist > XGetGameStats()->LongestThrow()) XGetGameStats()->LongestThrow() = dist;
        }
    }
}

void Actor::EnableRagDoll()
{
    if (m_StateFlags.m_bRagDoll) return;
    m_StateFlags.m_bRagDoll = 1;
    m_RagDollTime = 0.0f;
    gRagDollCount++;
    SetCarry(0, 0);
    SetCarry(0, 1);
    EXItemAnimator_AnimModifier *pMod =
        (EXItemAnimator_AnimModifier *)m_pItem->GetAnimator(EXRUNTIME_CLASS(EXItemAnimator_AnimModifier));
    if (!m_pAnim) return;
    pMod->RemoveModifier(2);
    m_pRagDoll = new EX6DAnim;
    m_pRagDoll->SetItem(m_pItem);
    if (m_ActorFlags.m_bAlien) {
        m_pRagDoll->AutoRagdollInitialise(pMod, 0xf, 0, 0, 0.0f);
    } else {
        m_pRagDoll->m_flags.BitSet(1);
        m_pRagDoll->AutoRagdollInitialise(pMod, 0xb, 0, 0, 0.0f);
    }
    theItemEnv.m_p6DManager->AddBody(m_pRagDoll);
    m_pItem->SetItemPhysics(m_pRagDoll);
}

signed char Actor::IsAnimVariableSpeed(int anim, Actor *pActor)
{
    if (pActor && (!pActor->m_ActorFlags.m_bVariableSpeed9 || !pActor->m_ActorFlags.m_bVariableSpeedAnims)) return 0;
    switch (anim) {
    case 0x9000017: case 0x9000018: case 0x9000022: case 0x900004e: case 0x9000065: case 0x9000066:
    case 0x9000067: case 0x9000088: case 0x9000100: case 0x9000108: case 0x900013d: case 0x900014e:
    case 0x900014f: case 0x9000150: case 0x9000151: case 0x9000155: case 0x9000166: case 0x9000305:
    case 0x9000308: case 0x900031c: case 0x9000327: case 0x9000329: case 0x90003b6: case 0x90003b7:
    case 0x90003ff: case 0x9000402: case 0x900053a: case 0x4e000001: case 0x4e000002: case 0x4e000003:
        return 1;
    }
    return 0;
}

void Actor::SetHeadTrackMode(int mode)
{
    m_StateFlags.m_b8 = mode == 1;
    if (!mode) {
        m_HeadTrackMode = mode;
        return;
    }
    if (mode == m_HeadTrackMode) return;
    EXItemAnimator_AnimModifier *pMod =
        (EXItemAnimator_AnimModifier *)m_pItem->GetAnimator(EXRUNTIME_CLASS(EXItemAnimator_AnimModifier));
    if (!pMod) return;
    EXHeadTracking *pHT = pMod->HeadTracking();
    if (!pHT) {
        pMod->AddModifier(2);
        pHT = pMod->HeadTracking();
        if (!pHT) {
            m_HeadTrackMode = mode;
            return;
        }
        pHT->SetBaseHash(0xe000001);
        pHT->m_64 = EXVector(0.0f, 0.0f, 1.0f, 0.0f);
        pHT->m_14 = EXVector(0.0f, 0.0f, 1.0f, 0.0f);
    }
    switch (mode) {
    case 0:
        pHT->m_a3 = 1;
        break;
    case 1:
        pHT->SetNumBones(1);
        pHT->m_7c = 0.1f;
        pHT->m_80 = 0.1f;
        pHT->m_84 = 1.5f;
        pHT->m_88 = 1.5f;
        break;
    case 2: {
        pHT->SetNumBones(1);
        pHT->SetHash(0, 0xe000003);
        pHT->m_7c = DegToRad(5.0f);
        pHT->m_80 = DegToRad(5.0f);
        pHT->m_84 = DegToRad(185.0f);
        pHT->m_88 = DegToRad(185.0f);
        pHT->SetMaxAngularMove(DegToRad(5.0f));
        float map[1] = { 1.0f };
        pHT->SetCustomMapping(map);
        pMod->HeadTracking()->Flags().BitSet(0x80);
        break;
    }
    }
    m_HeadTrackMode = mode;
}

Actor *Actor::GetFirst(const EXVector &pos, float radius, signed char b3D)
{
    float r2 = radius * radius;
    ActorBase *p = g_aActorList;
    for (int i = 0; i < 128; i++, p++) {
        if (p->m_pActor && p->m_bInWorld) {
            float dsq = b3D ? (pos - p->m_Pos).LengthSquared() : (pos - p->m_Pos).LengthXZSquared();
            if (dsq <= r2) return p->m_pActor;
        }
    }
    return 0;
}

Actor *Actor::GetNext(const EXVector &pos, float radius, signed char b3D)
{
    float r2 = radius * radius;
    int i = (m_RefID & 0x7f) + 1;
    ActorBase *p = &g_aActorList[i];
    for (; i < 128; i++, p++) {
        if (p->m_pActor && p->m_bInWorld) {
            float dsq = b3D ? (pos - p->m_Pos).LengthSquared() : (pos - p->m_Pos).LengthXZSquared();
            if (dsq <= r2) return p->m_pActor;
        }
    }
    return 0;
}

void Actor::UpdateAlarmMeter()
{
    if (GetProfile()->m_AlertTime == 0.0f || m_Health <= 0.0f || m_StateFlags.m_bBeingStealthKilled ||
        m_ActorFlags2.m_bInPlayersHand || GetBrainState() == STATE_LookAroundAlerted)
        return;
    if (m_AlertDecayTimer > 0.0f) {
        m_AlertDecayTimer -= XApp::GameFrameRateMul();
        if (m_AlertDecayTimer < 0.0f) m_AlertDecayTimer = 0.0f;
    }
    if (m_AlertRaiseTimer > 0.0f) {
        m_AlertRaiseTimer -= XApp::GameFrameRateMul();
        if (m_AlertRaiseTimer < 0.0f) m_AlertRaiseTimer = 0.0f;
    }
    if (m_AlertDecayTimer == 0.0f) {
        if (m_AlertRaiseTimer == 0.0f) {
            Objective_NotifyPlayerSeen(this);
            m_AlertRaiseTimer = -1.0f;
            m_AlertDecayTimer = -1.0f;
        } else if (m_AlertRaiseTimer == -1.0f && IsBitMsgOnStack(AW_BIT_HAVE_SEEN_PLAYER)) {
            m_AlertRaiseTimer = (float)EXBaseApp::NominalGameFrameRate() * 4.0f;
        }
    } else if (m_AlertDecayTimer == -1.0f) {
        if (!IsBitMsgOnStack(AW_BIT_HAVE_SEEN_PLAYER))
            m_AlertDecayTimer = (float)EXBaseApp::NominalGameFrameRate() * 5.0f;
    } else if (m_AlertDecayTimer > 0.0f) {
        if (IsBitMsgOnStack(AW_BIT_HAVE_SEEN_PLAYER)) {
            m_AlertRaiseTimer = -1.0f;
            m_AlertDecayTimer = -1.0f;
        }
    }
}

void Actor::SparkCloak(int secs)
{
    ActorEffect *p = EffectGet(0x5f5f6b726170735fULL);
    if (m_AnimMode == 0x900006d || m_AnimMode == 0x90002d9) {
        if (p) p->SetTimer(0);
        return;
    }
    if (p) {
        p->SetTimer(secs);
        return;
    }
    if (m_ActorFlags.m_bBoss) EffectCreate(0x10000e3, 0xd00012f, 0, (float)secs, 0, 0x5f5f6b726170735fULL);
    else EffectCreate(0x10000e3, 0xd00012d, 0, (float)secs, 0, 0x5f5f6b726170735fULL);
}

void Actor::SayLuredVO()
{
    switch (GetSkin()->m_VOType) {
    case 1:
        if (AllyNear(10.0f, 0)) Speak(0x1af005ac, 0);
        else Speak(0x1af00450, 0);
        break;
    case 4:
        if (AllyNear(10.0f, 0)) Speak(0x1af004b2, 0);
        else Speak(0x1af004b2, 0);
        break;
    case 7:
        if (AllyNear(10.0f, 0)) Speak(0x1af005ad, 0);
        else Speak(0x1af004a6, 0);
        break;
    case 8:
        Speak(0x1af00458, 0);
        break;
    case 9:
        Speak(0x1af003e5, 0);
        break;
    case 10:
        Speak(0x1af00498, 0);
        break;
    case 11:
        Speak(0x1af003e2, 0);
        break;
    case 12:
        Speak(0x1af005f1, 0);
        break;
    case 15:
        if (AllyNear(10.0f, 0)) Speak(0x1af005ab, 0);
        else Speak(0x1af00455, 0);
        break;
    case 21:
        if (AllyNear(10.0f, 0)) Speak(0x1af0064c, 0);
        else Speak(0x1af0064b, 0);
        break;
    case 24:
        if (AllyNear(10.0f, 0)) Speak(0x1af006a4, 0);
        else Speak(0x1af006a3, 0);
        break;
    case 25:
        if (AllyNear(10.0f, 0)) Speak(0x1af00660, 0);
        else Speak(0x1af0065f, 0);
        break;
    }
}

void Actor::PostLoadInit()
{
    gTimer_OverridePlayerCloak = 0.0f;
    gActorPause = 0;
    gActorHide = 0;
    switch (gCurrentMap_GeoHash) {
    case 0x1000056:
    case 0x1000070:
    case 0x1000073:
    case 0x10000af:
        gActorFadeDist = 100.0f;
        break;
    case 0x1000029:
    case 0x10000cd:
        gActorFadeDist = 75.0f;
        break;
    case 0x10000b4:
    case 0x10000d5:
    case 0x10000d6:
    case 0x1000124:
    case 0x100013b:
        gActorFadeDist = 190.0f;
        break;
    case 0x5000022:
    case 0x500002b:
        gActorFadeDist = 150.0f;
        break;
    default:
        gActorFadeDist = 50.0f;
        break;
    }
    ms_fActorAlpha = FELevelManager::Instance(1)->LevelFromHash(gCurrentMap_GeoHash, 0)->m_Type == 2 ? 0.5f : 1.0f;
    memset(m_MsgQueue, 0, sizeof(m_MsgQueue));
    m_MsgQueueCount = 0;
}

void Actor::UpdateLostLimbs()
{
    for (int i = 0; i < 4; i++) {
        LostLimbStruct &l = m_LostLimbs[i];
        if (!l.m_Id || (l.m_Flags & 2)) continue;
        for (int j = 0; j < (int)EXBaseApp::GameFrameRateMul(); j++) {
            ((EXItem *)l.m_Id)->SetPosition(((EXItem *)l.m_Id)->Position() + l.m_Vel);
            l.m_Vel.y -= 0.0083f;
            if (l.m_Vel.y < -1.2f) l.m_Vel.y = -1.2f;
            EXItem *p = (EXItem *)l.m_Id;
            p->m_rotation = p->Rotation() + l.m_RotVel;
            p->m_validMtx = 0;
        }
    }
}

EXVector Actor::CalcJumpDelta()
{
    EXVector pos = GetFeetPos();
    EXVector to;
    m_NavRoute.CalcJumpToPos(gNavMeshHandler.GetPtr((u32)m_OrgLoc.m_nMesh), pos, &m_NavRoute.m_JumpFromNode,
                             m_NavRoute.m_MoveVector, to);
    return to - pos;
}

void Actor::CheckForFriendsInCombat()
{
    ActorProfile *p = GetProfile();
    if (p->m_FriendsInCombatCount <= 0 || !p->m_FriendsInCombatState || GetBrainState() != STATE_AttackPred) return;
    float r = 4.0f;
    if (m_ActorFlags.m_b20) r = 25.0f;
    EXVector f = GetFeetPos();
    EXVector d = Player_GetFeetPos() - f;
    if (d.LengthXZSquared() > 16.0f) return;
    int n = 0;
    for (Actor *a = GetFirst(f, r, 0); a; a = a->GetNext(f, r, 0))
        if (a != this && a->m_Health > 0.0f && a->SearchBrainStateStack(STATE_AttackPred)) n++;
    if (!m_ActorFlags.m_b20) {
        if (n >= p->m_FriendsInCombatCount) m_ActorFlags.m_b20 = 1;
    } else if (!n) {
        m_SM.Goto(0, AP_GetStateFromHash(p->m_FriendsInCombatState), 0, 0, 0, 0);
    }
}

void Actor::NoticeActor(Actor *pActor, signed char bForce)
{
    if (!pActor) return;
    Actor *pCur = GetPtrFromID(m_NoticedActor);
    if (!pCur || bForce) {
        m_NoticedActor = pActor->m_RefID;
    } else {
        float dNew = (GetFeetPos() - pActor->GetFeetPos()).LengthXYZ();
        float dCur = (GetFeetPos() - pCur->GetFeetPos()).LengthXYZ();
        if (dNew < dCur) m_NoticedActor = pActor->m_RefID;
    }
    if (!m_HeadTrackMode) SetHeadTrackMode(1);
}

void Actor::ApplyRagDollForce(const EXVector &pos, const EXVector &dir, float force, int body)
{
    if (!m_StateFlags.m_bRagDoll) return;
    EXVector3 n(dir.x, dir.y, dir.z);
    n.Normalise();
    if (force > 20.0f) force = 20.0f;
    if (n.y > 0.5f) force = ((1.0f - n.y) + 0.5f) * force;
    if (body < 0) body = EXRand32() % 11;
    EX6DAnim *p = (EX6DAnim *)m_pItem->m_pItemPhysics;
    if (!p->GetRuntimeClass()->IsDerivedFrom(EXRUNTIME_CLASS(EX6DAnim))) return;
    if (body < p->BodyCount() && p->Body(body))
        p->Body(body)->AddWorldImpulse(EXVector(n.x, n.y, n.z, 0.0f) * 8.0f * force);
}

void Actor::JumpToPos(const EXVector &pos)
{
    m_NavRoute.InitJump(XNavMeshHandler::PolyID(m_OrgLoc.m_nPolyHandle), m_OrgLoc.m_vPos,
                        XNavMeshHandler::PolyID(m_OrgLoc.m_nPolyHandle), pos, m_pItem->Rotation().y);
    EXVector d = m_NavRoute.JumpToPos() - m_NavRoute.JumpFromPos();
    EXMaths::vec_xyzw dv = d;
    float horiz = EXSqrt(dv.x * dv.x + dv.z * dv.z);
    float apex = horiz * 0.5f;
    float up = d.y;
    if (up >= 0.0f) up += 0.5f;
    float vUp;
    if (apex > fabs(up)) vUp = EXSqrt(apex * 0.0166f);
    else if (up > 0.0f) vUp = EXSqrt(up * 0.0166f);
    else vUp = 0.0f;
    float vDown = EXSqrt(vUp * vUp - d.y * 0.0166f);
    m_WorldVelocity.y = vUp;
    float yaw = (float)atan2(d.x, d.z);
    float speed = horiz / (((0.0f - vDown) - vUp) * 120.48193f);
    m_LocalVelocity.Set(0.0f, 0.0f, 0.0f - speed, 0.0f);
    m_NavRoute.m_fJumpTime = yaw;
}

void Actor::CalculateParameters()
{
    EXVector d = XGetPlayer()->GetItem()->Position() - GetFeetPos();
    m_SqDistFromPlayer = d.LengthXYZSquared();
    d = XGetCamera(0)->GetItem()->Position() - GetFeetPos();
    m_SqDistFromCamera = d.LengthXYZSquared();
    m_FadeDistance = Actor_GetFadeDistance(this);
    EXSphere s;
    s.m_centre = GetFeetPos();
    s.m_centre.y += 1.0f;
    s.m_radius = m_ActorFlags.m_bUlysses ? 3.0f : 1.0f;
    m_StateFlags.m_bOnScreen24 = XGetCamera(0)->BoundingVolumeOnScreen(s) != 0;
    float f = m_FadeDistance + 5.0f;
    m_StateFlags.m_bOnScreen = f * f > m_SqDistFromCamera ? m_StateFlags.m_bOnScreen24 : 0;
}

EXVector Actor::GetAnimDelta()
{
    int i = m_AnimMode - 0x9000000;
    if (i >= 0 && i < 0x54b && !m_StateFlags.m_bOnScreen && s_AnimVelocity[i].m_bValid) {
        const ActorAnimVelocity &a = s_AnimVelocity[i];
        return EXVector(a.m_Vel.x, a.m_Vel.y, a.m_Vel.z, 0.0f) * m_pAnim->m_frameRateMult;
    }
    EXMotionData md;
    m_pAnim->CalcMotionData(md, 3, m_AnimMotionOverride | 1);
    Actor_CalledCMD++;
    if (i >= 0 && i < 0x54b && !s_AnimVelocity[i].m_bValid) {
        EXMotionData total;
        if (!m_pAnim->CalcTotalMotion(total)) return EXVector(0.0f, 0.0f, 0.0f, 0.0f);
        float frames = GetAnimFrameCount();
        total.m_pos.x = (1.0f / frames) * total.m_pos.x;
        total.m_pos.z = (1.0f / frames) * total.m_pos.z;
        ActorAnimVelocity &a = s_AnimVelocity[i];
        a.m_Vel.x = total.m_pos.x;
        a.m_bValid = 1;
        a.m_Vel.y = 0.0f;
        a.m_Vel.z = total.m_pos.z;
        if (fabs(total.m_pos.x) < 0.01f) a.m_Vel.x = 0.0f;
        if (fabs(total.m_pos.z) < 0.01f) a.m_Vel.z = 0.0f;
    }
    return md.m_pos * AnimSpeedMult();
}

void Actor::NoticePlayer()
{
    if (GetBrainState() == STATE_StealthDeath) return;
    if (GetBrainState() == STATE_PlayerControlDeath) return;
    if (m_Health <= 0.0f) return;
    if (!IsDrone()) return;
    if (!m_ActorFlags2.m_bCouldPlayerBeSeen) {
        if (m_NudgeTimer > 0.0f) goto tick;
    } else if (!m_ActorFlags2.m_bNudged) {
        float r;
        switch (GetProfile()->m_NudgeType) {
        case 0x50000001: r = 5.5f; break;
        case 0x50000002: r = 8.0f; break;
        default: return;
        }
        EXVector ph = Player_GetHeadPos();
        if ((GetHeadPos() - ph).LengthXYZSquared() < r * r) {
            m_ActorFlags2.m_bNudged = 1;
            m_LookAtPlayerTimer = 0.0f;
            m_NudgeTimer = (float)EXBaseApp::NominalGameFrameRate() * (EXRandf() * 4.0f + 6.0f);
            if (!m_StateFlags.m_bPlayerSeen && !m_SpeechTag.IsPlaying()) SayNudgedVO();
        }
        goto done;
    } else {
        if (m_NudgeTimer > 0.0f) goto tick;
        EXVector ph = Player_GetHeadPos();
        if ((GetHeadPos() - ph).LengthXYZSquared() <= 100.0f) goto done;
    }
    m_ActorFlags2.m_bNudged = 0;
    goto done;
tick:
    if (!m_SpeechTag.IsPlaying()) m_NudgeTimer -= XApp::GameFrameRateMul();
done:
    if (m_ActorFlags2.m_bNudged && !m_HeadTrackMode) SetHeadTrackMode(1);
}

// Spawn an actor of a profile: pick one of its skins, create the item facing rotY, place it on
// the nav mesh, record the profile/skin/creation data, load the skin's geo sections and start the
// brain in STATE_ActorInit. The nav poly and the time table are not used.
Actor *Actor::Create(int profile, const EXVector &pos, int poly, float rotY, int createID, int flags,
                     ActorTimeTable *pTable)
{
    if (!DBG_bActorsAndVehicles) return 0;
    EXHashCodeFile::Debug_HashCodeName(profile);
    ActorProfileIndex pi;
    AP_GetIndex(profile, pi);
    if (pi.m_Row == 0xffff || pi.m_Sheet == 0xffff) return 0;
    int skin = AP_GetRandSkinHash(AP_GetPtrFromIndex(pi));
    ActorSkinIndex si;
    AS_GetIndex(skin, si);
    if (si.m_Row == 0xffff || si.m_Sheet == 0xffff) return 0;
    ActorSkin *pSkin = AS_GetPtr(skin);
    if (!pSkin) {
        pSkin = AS_GetIndexPtr(0);
        if (!pSkin) {
            EXHashCodeFile::Debug_HashCodeName(skin);
            return 0;
        }
    }
    XItem *pItem = XItem::CreateItem(0, 0, (EXRuntimeClass *)&Actor::classActor, EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    if (!pItem) return 0;
    pItem->SetClipType(2);
    pItem->m_rotation.Set(0.0f, rotY, 0.0f, 0.0f);
    pItem->Invalidate();
    pItem->SetMapOn(theItemEnv.GetMapOn(pItem->m_position));
    Actor *pActor = (Actor *)pItem->ItemHandler();
    if (!pActor) return 0;
    pActor->SetPosition(pos, -1);
    pActor->m_SubGroupAnimIndex = SubGroupAnims::GetSubGroupAnimsFromHash(AP_GetPtrFromIndex(pi)->m_SubGroup);
    pActor->m_DestLoc = pActor->m_OrgLoc;
    pActor->m_ProfileIndex = pi;
    pActor->m_CreateID = createID;
    pActor->m_SkinIndex = si;
    pActor->m_StartPos = pos;
    pActor->m_CreationFlags = flags;
    pActor->m_DefaultContextIcon = AP_GetPtrFromIndex(pActor->m_ProfileIndex)->m_ContextIcon;
    pActor->SetRotY(rotY);
    pActor->m_Health = AP_GetPtrFromIndex(pi)->m_Health;
    GeoSectionManager::Add(pSkin->m_File, pSkin->m_Hash);
    GeoSectionManager::Add(pSkin->m_ChangeFile, pSkin->m_ChangeHash);
    GeoSectionManager::Add(pSkin->m_SkelFile, pSkin->m_SkelHash);
    pActor->m_TargettedFlagChannel = 0;
    pActor->m_RadarType = 0;
    pActor->m_SM.Init(*pActor, STATE_ActorInit);
    return pActor;
}

// Rotates v about Y by ry (euler order 4), as the walkers turn their animation motion into world space.
static __forceinline EXVector Actor_RotateY(float ry, const EXVector &v)
{
    EXMatrix m;
    m.SetIdentity();
    m.SetRotation(EXVector(0.0f, ry, 0.0f, 0.0f), 4);
    EXVector out;
    D3DXVec4Transform(&out, &v, &m);
    return out;
}

// One flat light from above-front, a grey fill, and the given ambient colour.
static __forceinline void Actor_SetFlatLighting(EXExtRenderInfo &info, const EXVector &dir, const float *amb)
{
    info.m_Light[0].set(dir.x, dir.y, dir.z, dir.w);
    info.m_Light[1].set(0.0f, 0.0f, 0.0f, 0.0f);
    info.m_Light[2].set(0.0f, 0.0f, 0.0f, 0.0f);
    info.m_Light2[0].set(0.5f, 0.5f, 0.5f, 0.0f);
    info.m_Light2[1].set(0.0f, 0.0f, 0.0f, 0.0f);
    info.m_Light2[2].set(0.0f, 0.0f, 0.0f, 0.0f);
    info.m_Ambient.set(amb[0], amb[1], amb[2], 0.0f);
}

// Ground movement for ordinary walkers. Lights the actor from the nav mesh, then (unless it is standing
// still, when only root motion applies) falls under gravity, follows its route or heads straight for its
// destination, steers around other actors, turns towards the heading (or a scripted turn), moves by the
// animation's root motion, gets pushed off the player and slides along nav mesh walls. Jumps are carried
// by the route. Dead actors flagged for it are tilted to lie along the floor.
void Actor::UpdateMovement2D()
{
    bool bOnScreen = m_StateFlags.m_bOnScreen;
    bool bStill = m_Status == 1 || !m_ActorFlags.m_bMovementEnabled;
    bool bDead = m_Health <= 0.0f;
    bool bAlive = m_Health > 0.0f;
    bool bAvoid = (m_ActorFlags.m_bUlysses || bAlive) && bOnScreen;
    bool bPushPlayer = bAlive && m_StateFlags.m_bCanCollideWithPlayer && m_Unk8f4 > 0.0f &&
                       !m_StateFlags.m_bLyingOnGround && !m_StateFlags.m_bRagDoll && !m_ActorFlags.m_bHolographic;
    bool bAlignFloor = m_ActorFlags.m_b29 && bDead;

    if (bOnScreen) {
        EXVector lights[3];
        EXVector fills[3];
        memset(lights, 0, sizeof(lights));
        memset(fills, 0, sizeof(fills));
        XItemHandler_Player *pPlayer = XItemHandler_Player::Instance();
        if (pPlayer->m_Tint) {
            m_Unk8d4[0] = m_Unk8d4[1] = m_Unk8d4[2] = 1.0f;
        } else {
            float len = (float)sqrt(3.0);
            lights[0].set(-1.0f, -1.0f, -1.0f, 0.0f);
            if (len > 1e-5f) {
                float inv = 1.0f / len;
                lights[0].set(-1.0f * inv, -1.0f * inv, -1.0f * inv, 0.0f * inv);
            }
            m_Unk8d4[0] = m_Unk8d4[1] = m_Unk8d4[2] = 1.0f;
            if (bStill && XItemHandler_Player::Instance()) {
                // Standing actors take the player's ambient colour.
                EXVector amb = XItemHandler_Player::Instance()->m_AmbientColour;
                m_Unk8d4[0] = amb.x;
                m_Unk8d4[1] = amb.y;
                m_Unk8d4[2] = amb.z;
            } else if (m_StateFlags.m_bCloaked) {
                m_Unk8d4[0] = m_Unk8d4[1] = m_Unk8d4[2] = 1.0f;
            } else if (m_OrgLoc.IsValid()) {
                XNavMesh *pMesh = m_OrgLoc.GetMesh();
                if (pMesh) {
                    m_Unk8d4[0] = m_Unk8d4[1] = m_Unk8d4[2] = 1.0f;
                    pMesh->GetLighting(m_OrgLoc.m_vPos, m_OrgLoc.GetPolyID(), m_Unk8d4[0], m_Unk8d4[1], m_Unk8d4[2]);
                    for (int i = 0; i < 3; i++)
                        if (m_Unk8d4[i] < 0.01f) m_Unk8d4[i] = 0.01f;
                }
            }
        }
        fills[0].set(0.5f, 0.5f, 0.5f, 0.0f);
        Actor_SetFlatLighting(m_ExtRenderInfo, lights[0], m_Unk8d4);
        Actor_SetFlatLighting(m_ExtRenderInfo2, lights[0], m_Unk8d4);
    }

    if (bStill) {
        // Root motion only.
        EXVector delta = GetAnimDelta();
        float rotX = m_pItem->Rotation().x;
        float rotY = m_StateFlags.m_bMoving14 ? m_TargetRotY : m_pItem->Rotation().y;
        EXVector move = Actor_RotateY(rotY, delta);
        m_Unk18 = m_pItem->Position() + move;
        m_pItem->SetPosition(m_Unk18);
        m_OrgLoc.SetPosPoly(m_Unk18, -1, 0);
        m_pItem->m_rotation = EXVector(rotX, rotY, 0.0f, 0.0f);
        m_pItem->Invalidate();
        m_LocalVelocity.set(0.0f, 0.0f, 0.0f, 0.0f);
        return;
    }

    float mul = XApp::GameFrameRateMul();
    bool bNavValid = AU_IsNavMeshValid(m_pItem->Position()) != 0;
    EXVector pos = m_pItem->Position();
    pos.w = 0.0f;
    float heading = m_pItem->Rotation().y;
    if (fabs(pos.x - m_OrgLoc.m_vPos.x) >= 0.001f || fabs(pos.y - m_OrgLoc.m_vPos.y) >= 0.001f ||
        fabs(pos.z - m_OrgLoc.m_vPos.z) >= 0.001f || fabs(-m_OrgLoc.m_vPos.w) >= 0.001f) {
        m_pItem->SetPosition(pos);
        m_OrgLoc.SetPosPoly(pos, -1, 0);
    }

    // Gravity (a jump in progress is driven by the route).
    if (!m_NavRoute.m_Flags.m_bTriggerJump || m_NavRoute.m_Flags.m_b7 || bDead) {
        m_WorldVelocity.y -= mul * 0.0083f;
        if (m_WorldVelocity.y < -1.2f) m_WorldVelocity.y = -1.2f;
        pos.x += m_WorldVelocity.x * mul;
        pos.y += m_WorldVelocity.y * mul;
        pos.z += m_WorldVelocity.z * mul;
        pos.w += m_WorldVelocity.w * mul;
    } else {
        ProcessTriggerJump();
    }

    EXVector animDelta = GetAnimDelta();
    if (!bNavValid) {
        heading = (float)atan2(m_DestLoc.m_vPos.x - pos.x, m_DestLoc.m_vPos.z - pos.z);
    } else {
        m_StateFlags.m_bValidRoute = 1;
        if (m_StateFlags.m_bMoving15 && !m_StateFlags.m_bMoving14 && !m_StateFlags.m_bMoving27) {
            if (gNavMeshHandler.IsDynamicNavigation()) {
                m_NavRoute.m_Flags.m_bDynamic = 1;
                m_NavRoute.m_NavIgnoreFlags = CanJump() ? 8 : 0x48;
            }
            m_NavRoute.m_Flags.m_bNavigating = 1;
            EXVector &dir = m_NavRoute.CalcRoute(&m_OrgLoc, &m_DestLoc);
            if (m_NavRoute.m_Status != 0) {
                m_NavRoute.m_Flags.m_bNavigating = 0;
                m_StateFlags.m_bValidRoute = 0;
                return;
            }
            heading = (float)atan2(dir.x, dir.z);
        }
    }

    bool bJumping = m_NavRoute.m_Flags.m_b7;
    float rotY;
    if (bJumping) {
        // Face the landing point once over the take-off poly, then hold that heading.
        int poly = m_OrgLoc.GetPolyID();
        if (poly == m_NavRoute.m_JumpNode) {
            EXVector d = m_NavRoute.m_JumpToPos - m_OrgLoc.m_vPos;
            heading = (float)atan2(d.x, d.z);
            m_NavRoute.m_fJumpTime = heading;
        } else {
            heading = m_NavRoute.m_fJumpTime;
        }
        rotY = heading;
    } else {
        float rot = m_pItem->Rotation().y;
        float diff = AngleDiff(rot, heading);
        if (!bAvoid) {
            rotY = rot + diff;
        } else {
            CAvoidance avoid;
            m_pItem->SetPosition(pos);
            float steer = avoid.Steer(this, heading);
            pos.x += avoid.m_PushDelta.x;
            pos.y += avoid.m_PushDelta.y;
            pos.z += avoid.m_PushDelta.z;
            pos.w += avoid.m_PushDelta.w;
            if (avoid.m_PushDelta.x != 0.0f || avoid.m_PushDelta.y != 0.0f || avoid.m_PushDelta.z != 0.0f ||
                avoid.m_PushDelta.w != 0.0f) {
                m_Unk90c[0]++;
                m_ActorFlags.m_b24 = 1;
            } else if (m_ActorFlags.m_b24) {
                m_ActorFlags.m_b24 = 0;
                m_Unk90c[0] = 0;
            }
            float dSq = SqDistToTarget();
            if (m_StateFlags.m_bMoving15) {
                // Close in: turn straight to the heading; otherwise ease along the steered direction.
                if (dSq <= 4.0f)
                    rotY = rot + (dSq >= 0.25f ? mul * 0.075f * diff : diff);
                else
                    rotY = rot + mul * 0.075f * steer;
            } else {
                rotY = rot + mul * 0.075f * diff;
            }
        }

        if (m_StateFlags.m_bMoving14) {
            // Snap to a requested facing.
            rotY = m_TargetRotY;
            m_StateFlags.m_bMoving14 = 0;
        } else if (m_StateFlags.m_bMoving27) {
            // Turn-on-the-spot animation: blend the turn in over the middle half of the anim.
            if (m_ActorFlags2.m_bAnimEnded) {
                m_StateFlags.m_bMoving27 = 0;
            } else {
                float f = GetAnimFrame();
                if (f >= 0.25f && f < 0.75f) {
                    float k = 10.0f - (f - 0.25f) * 20.0f;
                    if (k <= 1.0f) k = 1.0f;
                    float cur = m_pItem->Rotation().y;
                    float r = XNavNetwork::AngleDifference(cur, m_TargetRotYDelta) / (k * mul) + cur;
                    if (r < 0.0f) {
                        do r += 6.2831855f; while (r < 0.0f);
                    } else {
                        while (r > 6.2831855f) r -= 6.2831855f;
                    }
                    rotY = r;
                } else if (f >= 0.75f) {
                    rotY = m_TargetRotYDelta;
                }
            }
        }
    }

    // Root motion, or the jump's carried velocity.
    bool bFlying = bJumping && !m_NavRoute.m_Flags.m_bJumpCancel;
    EXVector move;
    if (bFlying) {
        EXVector v(m_LocalVelocity.x * mul, m_LocalVelocity.y * mul, m_LocalVelocity.z * mul, m_LocalVelocity.w * mul);
        move = Actor_RotateY(rotY, v);
    } else {
        float div = XApp::GameFrameRateDiv();
        m_LocalVelocity.set(animDelta.x * div, animDelta.y * div, animDelta.z * div, animDelta.w * div);
        move = Actor_RotateY(rotY, animDelta);
    }
    if (!m_StateFlags.m_bMoving15 || bFlying || move.x * move.x + move.y * move.y + move.z * move.z < SqDistToTarget()) {
        pos.x += move.x;
        pos.y += move.y;
        pos.z += move.z;
    } else {
        // Would overshoot: settle halfway onto the destination and stop.
        pos.x += (m_DestLoc.m_vPos.x - pos.x) * 0.5f;
        pos.z += (m_DestLoc.m_vPos.z - pos.z) * 0.5f;
        m_StateFlags.m_bMoving15 = 0;
    }

    if (bPushPlayer && !bFlying) {
        EXVector feet = Player_GetFeetPos();
        EXVector d = pos - feet;
        float r = m_CollisionRadius + 0.49f;
        float dSq = d.x * d.x + d.y * d.y + d.z * d.z + d.w * d.w;
        if (dSq >= r * r) {
            m_ActorFlags.m_b25 = 0;
            m_Unk90c[1] = 0;
        } else {
            float dist = (float)sqrt(dSq);
            EXVector n(d.x, d.y, d.z, 0.0f);
            float nLenSq = n.x * n.x + n.y * n.y + n.z * n.z;
            if (nLenSq >= 0.0f) {
                float nLen = (float)sqrt(nLenSq);
                if (nLen > 1e-5f) {
                    float inv = 1.0f / nLen;
                    n.set(n.x * inv, n.y * inv, n.z * inv, 0.0f);
                }
            }
            float s = (r - dist) * m_ActorToPlayerColAdj;
            pos.x += n.x * s;
            pos.y += n.y * s;
            pos.z += n.z * s;
            pos.w += n.w * s;
            m_Unk90c[1]++;
            m_ActorFlags.m_b25 = 1;
        }
    }

    if (!bNavValid) {
        pos.y = 0.0f;
        m_WorldVelocity.y = 0.0f;
    } else {
        m_NavRoute.Collide(&m_OrgLoc, pos);
        if (m_NavRoute.m_Flags.m_bCollided) {
            if (bJumping) {
                if (!(m_OrgLoc.m_wSurfaceFlags & 0x40)) {
                    // Landed.
                    SendMsg(MsgItem((MsgType)0x26), 0.0f);
                    if (GetBrainState() != STATE_MonsterJump) m_NavRoute.m_Flags.m_b7 = 0;
                    m_NavRoute.m_Flags.m_bJumpCancel = 1;
                    m_WorldVelocity.y = 0.0f;
                } else {
                    // Hit a no-land surface mid jump: drift onto the landing point.
                    EXVector d = m_NavRoute.m_JumpToPos - m_OrgLoc.m_vPos;
                    if (d.x * d.x + d.y * d.y + d.z * d.z >= 0.04f) {
                        float lenSq = d.x * d.x + d.y * d.y + d.z * d.z + d.w * d.w;
                        EXVector n = d;
                        if (lenSq >= 0.0f) {
                            float len = (float)sqrt(lenSq);
                            if (len > 1e-5f) {
                                float inv = 1.0f / len;
                                n.set(d.x * inv, d.y * inv, d.z * inv, d.w * inv);
                            }
                        }
                        m_OrgLoc.Move(EXVector(n.x * 0.2f, n.y * 0.2f, n.z * 0.2f, n.w * 0.2f), 0);
                    } else {
                        m_OrgLoc.MoveTo(m_NavRoute.m_JumpToPos, 0);
                    }
                    pos = m_OrgLoc.m_vPos;
                }
            } else {
                m_WorldVelocity.y = 0.0f;
            }
        }
        if (m_NavRoute.m_TriggerJump && CanJump()) {
            if (GetBrainState() == STATE_MonsterJump)
                SendMsg(MsgItem((MsgType)0x25), 0.0f);
            else
                m_SM.ExtGosub(0, STATE_MonsterJump, 0, 0, 0, 0);
        }
        if (m_NavRoute.m_Flags.m_b2) {
            m_Unk90c[2]++;
            m_ActorFlags.m_b26 = 1;
        } else {
            m_ActorFlags.m_b26 = 0;
            m_Unk90c[2] = 0;
        }
    }

    if (m_ActorFlags.m_b24 || m_ActorFlags.m_b25 || m_ActorFlags.m_b26) {
        m_ActorFlags.m_bHasCollided = 1;
        m_CollidedWith++;
    } else {
        m_ActorFlags.m_bHasCollided = 0;
        m_CollidedWith = 0;
    }
    if (!bFlying) {
        float dx = pos.x - m_Unk18.x, dz = pos.z - m_Unk18.z;
        float dSq = dx * dx + dz * dz;
        float dist = dSq < 0.0f ? 0.0f : (float)sqrt(dSq);
        m_LocalVelocity.z = (dist * XApp::GameFrameRateDiv() + GetAnimVelocity()) * 0.5f;
    }

    if (bAlignFloor) {
        // Lay the body along the floor slope.
        EXQuaternion q;
        q.quat_xyzw::SetRotation(EXVector(m_pItem->Rotation().x, rotY, m_pItem->Rotation().z, m_pItem->Rotation().w), 4);
        float s = (float)sin(rotY), c = (float)cos(rotY);
        EXVector n = m_OrgLoc.m_vFloorNormal;
        EXVector fwd(s, 0.0f, c, 0.0f);
        EXVector side(fwd.y * n.z - fwd.z * n.y, fwd.z * n.x - fwd.x * n.z, fwd.x * n.y - fwd.y * n.x, 0.0f);
        EXVector target(pos.x + (n.y * side.z - n.z * side.y), pos.y + (n.z * side.x - n.x * side.z),
                        pos.z + (n.x * side.y - n.y * side.x), pos.w);
        EXMatrix m;
        m.SetIdentity();
        m.LookAt(pos, target, n, EXVector(0.0f, 0.0f, 1.0f, 0.0f));
        EXQuaternion q2;
        q2.SetRotation(m);
        EXQuaternion r(q.slerp(q2, 0.4f));
        EXVector rv;
        r.GetRotation(rv, 4);
        m_pItem->m_rotation = EXVector(rv.x, rotY, rv.z, rv.w);
        m_pItem->Invalidate();
        m_pItem->SetPosition(pos);
        m_Unk18 = pos;
        return;
    }
    m_pItem->SetPosition(pos);
    m_pItem->m_rotation = EXVector(0.0f, rotY, 0.0f, 0.0f);
    m_pItem->Invalidate();
    m_Unk18 = m_pItem->Position();
}

// Moves the actor for this frame (rag dolls just count their time) and keeps its map-on current when the
// player holds it or it is on screen.
void Actor::UpdateMovement()
{
    if (m_StateFlags.m_bRagDoll) {
        m_RagDollTime += XApp::GameFrameRateMul();
    } else if (m_NavRoute.m_b3D) {
        UpdateMovement3D();
    } else {
        UpdateMovement2D();
    }
    XItemHandler_Player *pPlayer = XItemHandler_Player::Instance();
    if (pPlayer->IsHoldingActor() && pPlayer->m_SpecialActorID == (unsigned int)m_RefID) {
        m_ActorFlags2.m_bInPlayersHand = 1;
        const EXVector &p = m_pItem->Position();
        m_pItem->SetMapOn(theItemEnv.GetMapOn(EXVector(p.x, p.y + 1.0f, p.z, p.w)));
        return;
    }
    if (m_StateFlags.m_bOnScreen) {
        m_ActorFlags2.m_bInPlayersHand = 0;
        const EXVector &p = m_pItem->Position();
        m_pItem->SetMapOn(theItemEnv.GetMapOn(EXVector(p.x, p.y + 1.0f, p.z, p.w)));
        return;
    }
    m_ActorFlags2.m_bInPlayersHand = 0;
}

// Keeps carried items in the actor's hands: each carried item is placed on its hand datum (left-hand items
// turned round, the piss bottle dropped a little). The captive predator's right hand instead aims its item
// at a target that eases towards the player's chest.
void Actor::UpdateCarry()
{
    bool bCaptive = GetSkin()->m_Hash == 0xd0000b9;  // HT_AnimSkin_Pred_Captive_V1
    if (!m_StateFlags.m_bAimingPos && !m_StateFlags.m_bMoving27 && !bCaptive && !m_StateFlags.m_bOnScreen)
        return;
    int ids[2];
    ids[0] = m_CarryLeft;
    ids[1] = m_CarryRight;
    u32 datums[2];
    datums[0] = 0x10000013;  // HT_AnimDatum_Left_Hand
    datums[1] = GetSkin()->m_Hash == 0xd0000b9 ? 0x10000039 : 0x10000014;  // HT_AnimDatum_Right_Hand
    for (int i = 0; i < 2; i++) {
        if (!ids[i]) continue;
        ActorCarryable *pCarry = ActorCarryable::GetPtrFromID(ids[i]);
        if (!pCarry) continue;
        if (pCarry->m_EntityHash == 0x20000c9) datums[i] = 0xe000003;  // HT_Entity_Carry_Piss
        EXDatumTemp d;
        memset(&d, 0, 0x30);
        if (!m_pItem->GetAnimatorDatum(datums[i], d, 4, 0)) continue;
        Actor_CalledGetDatum++;
        if (pCarry->m_EntityHash == 0x20000c9) d.m_position.y -= 0.1f;
        EXVector rot;
        d.m_rotation.GetRotation(rot, 4);
        if (i == 0) {
            rot.z += 3.1415927f;
            if (rot.z >= 6.2831855f) rot.z -= 6.2831855f;
        }
        if (GetSkin()->m_Hash == 0xd0000b9) {
            rot.x = rot.y = rot.z = 0.0f;
            static EXVector vSniperTarget;
            if (g_bInitSniperTarget) {
                vSniperTarget = XItemHandler_Player::Instance()->GetChestPos();
                g_bInitSniperTarget = 0;
            } else {
                float t = XApp::GameFrameRateMul() * 0.05f;
                if (t >= 1.0f) t = 1.0f;
                if (m_Health > 0.0f) {
                    EXVector chest = XItemHandler_Player::Instance()->GetChestPos();
                    float s = 1.0f - t;
                    vSniperTarget.set(chest.x * t + vSniperTarget.x * s, chest.y * t + vSniperTarget.y * s,
                                      chest.z * t + vSniperTarget.z * s, chest.w * t + vSniperTarget.w * s);
                }
            }
            u32 hand = GetSkin()->m_Hash == 0xd0000b9 ? 0x10000039 : 0x10000014;
            EXDatumTemp h;
            memset(&h, 0, 0x30);
            m_pItem->GetAnimatorDatum(hand, h, 0, 0);
            EXVector dir = vSniperTarget - h.m_position;
            float lenSq = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z + dir.w * dir.w;
            if (lenSq >= 0.0f) {
                float len = (float)sqrt(lenSq);
                if (len > 1e-5f) {
                    float inv = 1.0f / len;
                    dir.set(dir.x * inv, dir.y * inv, dir.z * inv, dir.w * inv);
                }
            }
            float pitch;
            if (dir.y < -1.0f) pitch = -1.5707964f;
            else if (dir.y > 1.0f) pitch = 1.5707964f;
            else pitch = (float)asin(dir.y);
            rot.x -= pitch;
            float flatSq = dir.x * dir.x + dir.z * dir.z + dir.w * dir.w;
            if (flatSq >= 0.0f) {
                float flat = (float)sqrt(flatSq);
                if (flat > 1e-5f) {
                    float inv = 1.0f / flat;
                    dir.x *= inv;
                    dir.z *= inv;
                }
            }
            rot.y = ((float)atan2(dir.x, dir.z) + rot.y) - m_pItem->Rotation().y;
            if (rot.y < 0.0f) rot.y += 6.2831855f;
        }
        pCarry->m_pAnimator->SetPosition(d.m_position);
        pCarry->m_pAnimator->m_rotation = EXVector(rot.x, rot.y, rot.z, rot.w);
        pCarry->m_pAnimator->Invalidate();
    }
}

// Pushes the actor's alpha and tint to its item, lost limbs and their blood. Holograms vanish in the vision
// modes; the brain-scan mode tints by brain-scan colour. Limbs left lying fade out once their life runs
// out and are deleted when invisible.
void Actor::UpdateAlpha()
{
    float alpha = m_Alpha * m_Alpha2 * HolographicMult();
    float itemAlpha = alpha;
    u8 tint = XItemHandler_Player::Instance()->m_Tint;
    if (tint == 2) {
        if (m_ActorFlags.m_bHolographic) itemAlpha = 0.0f;
        ex::RGBA c = UpdateBrainScanColour(1);
        AU_SetItemARGB(m_pItem, itemAlpha, c.m_R * (1.0f / 255.0f), c.m_G * (1.0f / 255.0f), c.m_B * (1.0f / 255.0f));
    } else {
        if (tint != 0 && m_ActorFlags.m_bHolographic) itemAlpha = 0.0f;
        AU_SetItemARGB(m_pItem, itemAlpha, 1.0f, 1.0f, 1.0f);
    }
    bool bDraw = itemAlpha > 0.0f && !gActorHide && !m_StateFlags.m_bHide &&
                 (m_StateFlags.m_bOnScreen || m_ActorFlags2.m_bInPlayersHand);
    AU_SetItemDraw(m_pItem, bDraw);

    float limbAlpha = alpha;
    for (int i = 0; i < 4; i++) {
        LostLimbStruct &limb = m_LostLimbs[i];
        if (limb.m_Id) {
            limb.m_Life -= XApp::GameFrameRateMul();
            if (limb.m_Life <= 0.0f) {
                limb.m_Alpha -= EXBaseApp::GameFrameRateMul() * 0.05f;
                if (limb.m_Alpha <= 0.0f) {
                    DeleteLostLimb((ActorLostLimb)i);
                    return;
                }
                limbAlpha = limb.m_Alpha;
            }
            bool bLimbDraw = limbAlpha > 0.0f && !gActorHide && !m_StateFlags.m_bHide;
            if (AU_SetItemDraw((EXItem *)limb.m_Id, bLimbDraw)) {
                if (limb.m_Flags & 2) {
                    // Held by the player: lit by the player's ambient, brightened.
                    EXVector amb = XItemHandler_Player::Instance()->m_AmbientColour;
                    float r = amb.x * 4.0f, g = amb.y * 4.0f, b = amb.z * 4.0f;
                    if (r >= 1.0f) r = 1.0f;
                    if (g >= 1.0f) g = 1.0f;
                    if (b >= 1.0f) b = 1.0f;
                    AU_SetItemARGB((EXItem *)limb.m_Id, limbAlpha, r, g, b);
                } else {
                    AU_SetItemARGB((EXItem *)limb.m_Id, limbAlpha, m_Unk8d4[0], m_Unk8d4[1], m_Unk8d4[2]);
                }
            }
        }
        if (limb.m_pBlood || limb.m_pBlood2) {
            float r = 0.0f, g = 0.0f, b = 0.0f;
            Gore_GetBloodColour(this, r, g, b);
            if (limb.m_pBlood) {
                bool bBloodDraw = itemAlpha > 0.0f && !gActorHide && !m_StateFlags.m_bHide;
                if (AU_SetAnimatorDraw((EXItemAnimator *)limb.m_pBlood, bBloodDraw))
                    AU_SetAnimatorARGB_Body((EXItemAnimator *)limb.m_pBlood, itemAlpha, r, g, b);
            }
            if (limb.m_pBlood2) {
                bool bBloodDraw = limbAlpha > 0.0f && !gActorHide && !m_StateFlags.m_bHide;
                if (AU_SetAnimatorDraw((EXItemAnimator *)limb.m_pBlood2, bBloodDraw))
                    AU_SetAnimatorARGB_Limb((EXItemAnimator *)limb.m_pBlood2, limbAlpha, r, g, b);
            }
        }
    }
}

extern unsigned int gCurrentMap_GeoHash;

// Chooses what the head looks at: a nudged actor watches the player's head for a couple of seconds (then
// waits 4-8 s before doing so again), otherwise it watches a noticed actor within 100 m. Mode 1 tracks
// that point, mode 2 the aiming position at chest height; anything else lets the head relax.
void Actor::UpdateHeadTrack()
{
    if (m_StateFlags.m_b8 && m_Status != 3) {
        if (m_ActorFlags2.m_bNudged) {
            if (m_LookAtPlayerTimer >= 0.0f) {
                m_HeadTrackPos = Player_GetHeadPos();
                m_StateFlags.m_bHeadTrack = 1;
                m_LookAtPlayerTimer += EXBaseApp::GameFrameRateMul();
                if (m_LookAtPlayerTimer > (float)(EXBaseApp::NominalGameFrameRate() * 2))
                    m_LookAtPlayerTimer = -((float)EXBaseApp::NominalGameFrameRate() * (g_EXRandClass.Randf() + 1.0f) * 4.0f);
            }
        } else if (m_NoticedActor) {
            Actor *pActor = GetPtrFromID(m_NoticedActor);
            if (!pActor || pActor->m_Status != 3) {
                m_NoticedActor = 0;
            } else {
                EXVector d = m_pItem->Position() - pActor->m_pItem->Position();
                if (d.y * d.y + d.z * d.z + d.x * d.x < 10000.0f) {
                    m_HeadTrackPos = pActor->GetHeadPos();
                    m_StateFlags.m_bHeadTrack = 1;
                }
            }
        }
    }
    // Count back up from the cool-down.
    if (m_LookAtPlayerTimer < 0.0f) m_LookAtPlayerTimer += EXBaseApp::GameFrameRateMul();

    bool bAiming = m_StateFlags.m_bAimingPos;
    if (!m_StateFlags.m_bOnScreen || m_StateFlags.m_bRagDoll) {
        m_StateFlags.m_bHeadTrack = 0;
        bAiming = false;
    }
    EXItemAnimator_AnimModifier *pMod =
        (EXItemAnimator_AnimModifier *)m_pItem->GetAnimator(EXRUNTIME_CLASS(EXItemAnimator_AnimModifier));
    if (pMod && pMod->m_pHeadTracking) {
        EXHeadTracking *pHT = pMod->m_pHeadTracking;
        bool bRelax = false;
        switch (m_HeadTrackMode) {
        case 0:
            bRelax = true;
            break;
        case 1:
            if (m_StateFlags.m_bHeadTrack) pHT->SetTarget(m_HeadTrackPos);
            else bRelax = true;
            break;
        case 2:
            if (bAiming) {
                EXVector chest = GetChestPos();
                pHT->SetTarget(EXVector(m_AimingPos.x, chest.y, m_AimingPos.z, m_AimingPos.w));
            } else {
                bRelax = true;
            }
            break;
        }
        if (bRelax) pHT->m_a3 = 1;
    }
    m_StateFlags.m_bHeadTrack = 0;
}

// Shows the context icon over the actor's head: the move the player can do on it while it is the camera
// target (stealth kill, grab, juggle, kill), the alert icon when it is about to raise the alarm, or its
// default icon. The icon is a script animator re-created whenever the icon changes.
void Actor::UpdateContextIcon()
{
    if (GetSkin()->m_Type == 4) return;
    EXItemAnimator_Script *pIcon = (EXItemAnimator_Script *)m_Unk820;
    if (!m_StateFlags.m_bCanRipBossHead && m_Health <= 0.0f) {
        if (pIcon) {
            pIcon->ItemFlags().BitSet(0x10);
            m_Unk820 = 0;
        }
        return;
    }
    if (pIcon) {
        if (m_ActorFlags2.m_bInPlayersHand || m_StateFlags.m_bRagDoll) {
            pIcon->ItemFlags().BitClr(1);
            return;
        }
        pIcon->ItemFlags().BitSet(1);
        pIcon->Invalidate();
    }

    int icon = m_DefaultContextIcon;
    if (m_AlertRaiseTimer > 0.0f && GetProfile()->m_AlertTime > 0.0f) {
        if (gCurrentMap_GeoHash != 0x1000073) m_DefaultContextIcon = 0;  // HT_File_1G_GWarfare
        icon = 0x40005f6;
    }
    if (XGetCamera(0)->m_TargetID == m_RefID) {
        unsigned int moves = XItemHandler_Player::Instance()->m_SpecialMoveAvailable;
        if (m_StateFlags.m_bCanRipBossHead) icon = 0x4000413;                      // HT_Script_ContextIcon_StealthKill
        else if ((moves & 1) && CanBeStealthKilled()) icon = 0x4000413;            // HT_Script_ContextIcon_StealthKill
        else if ((moves & 0x10) && m_ActorFlags.m_bUlysses) icon = 0x4000691;
        else if ((moves & 2) && CanBeGrabbed()) icon = 0x4000411;                  // HT_Script_ContextIcon_Grab
        else if ((moves & 4) && CanBeJuggled()) icon = 0x4000415;                  // HT_Script_ContextIcon_Kill_Up
        else if (moves & 8) icon = 0x4000416;                                      // HT_Script_ContextIcon_Kill_Down
    }
    bool bRipHead = m_StateFlags.m_bCanRipBossHead;
    if (bRipHead) icon = 0x4000605;

    if (m_Unk820) {
        if (m_Health <= 0.0f && !bRipHead) {
            ((EXItemAnimator_Script *)m_Unk820)->ItemFlags().BitClr(1);
            return;
        }
        // First person hides all but the alert, vocal, clan mark and head-rip icons; so do the death states.
        if ((XGetCamera(0)->IsFirstPerson() && icon != 0x40005f6 && icon != 0x4000414 && icon != 0x4000417 &&
             icon != 0x4000605) ||
            GetBrainState() == STATE_PlayerControlDeath || GetBrainState() == STATE_StealthDeath) {
            ((EXItemAnimator_Script *)m_Unk820)->ItemFlags().BitClr(1);
            return;
        }
    }
    if (icon != m_Unk824) {
        if (m_Unk820) {
            ((EXItemAnimator_Script *)m_Unk820)->ItemFlags().BitSet(0x10);
            m_Unk820 = 0;
        }
        if (icon) {
            EXGeoFile *pFile = EXGeoFile::GetGeoFile(0x1000016);  // HT_File_Pred_FX
            EXItemAnimator_Script *pNew = EXItemAnimator_Script::NewItem();
            m_Unk820 = (int)pNew;
            pNew->Initialise(pFile, icon);
            pNew->RenderFlags().BitSet(1);
            pNew->m_rotOrder = 4;
            pNew->Invalidate();
            pNew->RenderInfo().RenderFlagsRef().BitSet(8);
            m_pItem->AddAnimator(pNew);
        }
    }
    if (m_Unk820) {
        float above = m_ActorFlags.m_bUlysses ? 1.2f : 0.5f;
        float y = GetHeadPos().y - m_pItem->Position().y + above;
        ((EXItemAnimator_Script *)m_Unk820)->SetPosition(EXVector(0.0f, y, 0.0f, 0.0f));
    }
    m_Unk824 = icon;
}

// Boss triggers from the actor profile: Ulysses reacts to the predator riding his head; with the boss watch
// on, a distance trigger fires when the player, having come within half the trigger distance, leaves it;
// a time trigger fires every few seconds; and pending low-health and large-damage triggers fire once.
void Actor::UpdateBossTests()
{
    XItemHandler_Player *pPlayer = XItemHandler_Player::Instance();
    if (m_ActorFlags.m_bUlysses && m_Health > 0.0f && pPlayer->m_Mode == 0x14 &&
        pPlayer->m_SpecialActorID == (unsigned int)m_RefID && !SearchBrainStateStack(STATE_UlysesPredOnHead))
        m_SM.Gosub(0, STATE_UlysesPredOnHead, 0, 0, 0, 0);

    if (!(m_WatchFlags & 4)) return;
    ActorProfile *pProfile = GetProfile();
    if (pProfile->m_DistanceTrigger && !IsBitMsgOnStack(AW_BIT_DISTANCE)) {
        EXVector d = m_pItem->Position() - Player_GetFeetPos();
        float dSq = d.y * d.y + d.z * d.z + d.x * d.x;
        if (dSq <= pProfile->m_Dist) {
            if (dSq < pProfile->m_Dist * 0.25f) m_StateFlags.m_b0 = 1;
        } else if (m_StateFlags.m_b0) {
            StateFn state = AP_GetStateFromHash(pProfile->m_DistanceTrigger);
            if (!SearchBrainStateStack(state)) {
                m_SM.Gosub(AW_BIT_DISTANCE, state, 0, 0, 0, 0);
                m_StateFlags.m_b0 = 0;
                return;
            }
        }
    }
    if (pProfile->m_TimeTrigger && !IsBitMsgOnStack(AW_BIT_TIME)) {
        int t = pProfile->m_TriggerTime;
        if ((float)t < m_UpdateTime) {
            StateFn state = AP_GetStateFromHash(pProfile->m_TimeTrigger);
            if (!SearchBrainStateStack(state)) {
                m_SM.Gosub(AW_BIT_TIME, state, 0, 0, 0, 0);
                m_UpdateTime -= (float)t;
                return;
            }
        }
    }
    if (m_StateFlags.m_b2) {
        m_StateFlags.m_b2 = 0;
        if (pProfile->m_LowHealthTrigger && !IsBitMsgOnStack(AW_BIT_LOW_HEALTH)) {
            m_SM.Gosub(AW_BIT_LOW_HEALTH, AP_GetStateFromHash(pProfile->m_LowHealthTrigger), 0, 0, 0, 0);
            return;
        }
    }
    if (m_StateFlags.m_b1) {
        m_StateFlags.m_b1 = 0;
        if (pProfile->m_LargeDamageTrigger && !IsBitMsgOnStack(AW_BIT_LARGE_DAMAGE))
            m_SM.Gosub(AW_BIT_LARGE_DAMAGE, AP_GetStateFromHash(pProfile->m_LargeDamageTrigger), 0, 0, 0, 0);
    }
}

// Actors with the fight watch pick a fight with the nearest living actor in fight range whose profile they
// fight and whose nav zone they can reach (never as Ulysses, aliens or hunters).
void Actor::UpdateCheckFight()
{
    if (m_Status == 0 || m_Status == 1 || !(m_WatchFlags & 2) || m_ActorFlags.m_bUlysses || m_ActorFlags.m_bAlien ||
        GetProfile()->m_SubGroup == 0x4f000021)  // HT_ActorSubGroup_Hunter
        return;
    Actor *pBest = 0;
    float bestSq = 1e8f;
    for (Actor *p = GetFirst(m_pItem->Position(), GetFightDistance(), 0); p;
         p = p->GetNext(m_pItem->Position(), GetFightDistance(), 0)) {
        // Retail tests this actor's status here, not the candidate's.
        if (p->m_Status == 0 || m_Status == 1 || p->m_Health <= 0.0f) continue;
        StateFn st = p->GetBrainState();
        if (st == STATE_PlayerControlDeath || st == STATE_CaptivePredHanging) continue;
        if (!AP_DoWeFight(GetProfile(), p->GetProfile())) continue;
        if (!CanReachZone(p->m_OrgLoc.m_sNavZone)) continue;
        EXVector d = p->m_pItem->Position() - m_pItem->Position();
        float dSq = d.y * d.y + d.z * d.z + d.x * d.x;
        if (dSq < bestSq) {
            pBest = p;
            bestSq = dSq;
        }
    }
    if (pBest) m_SM.Gosub(0, STATE_FightActor, pBest->m_RefID, 0, 0, 0);
}

// A seeing actor (not blinded) that has spotted the player - or is forced to, as when the predator rides
// it - drops any search or nudge reaction, records where the player was and runs its seen-player state.
// While that state runs it keeps shouting to everyone within 10 m every 1-2 seconds.
void Actor::UpdateSeeingPlayer()
{
    if (m_ActorFlags.m_bBlinded) return;
    XItemHandler_Player *pPlayer = XItemHandler_Player::Instance();
    if (pPlayer->m_Mode == 0x14 && pPlayer->m_SpecialActorID == (unsigned int)m_RefID)
        m_ForceSightingTimer = (float)EXBaseApp::NominalGameFrameRate();
    if (m_ForceSightingTimer > 0.0f) {
        m_ForceSightingTimer -= EXBaseApp::GameFrameRateMul();
        m_StateFlags.m_bPlayerSeen = 1;
    } else if (!m_StateFlags.m_bPlayerSeen) {
        return;
    }
    int seenState = GetProfile()->m_UnderAttackState;
    if (!seenState) return;
    if (!IsBitMsgOnStack(AW_BIT_HAVE_SEEN_PLAYER)) {
        if (m_WatchFlags & 1) {
            while (SearchBrainStateStack(STATE_SearchForPredator)) m_SM.PopState();
            while (SearchBrainStateStack(STATE_NudgeCloaked)) m_SM.PopState();
            gAIControl.SetLastSeenPlayerPos(Player_GetFeetPos());
            StateFn state = AP_GetStateFromHash(seenState);
            m_SM.Gosub(GetMsgBitsForMsg(MSG_SOMEONE_SEEN_PREDATOR) | GetMsgBitsForMsg(MSG_SOMEONE_UNDER_ATTACK) |
                           AW_BIT_HAVE_SEEN_PLAYER,
                       state, 0, 0, 0, 0);
        }
    } else {
        if (m_SeenAlertTimer > 0.0f) m_SeenAlertTimer -= XApp::GameFrameRateMul();
        if (m_SeenAlertTimer <= 0.0f) {
            SendMsg(MsgItem(MSG_SOMEONE_SEEN_PREDATOR), m_pItem->Position(), 10.0f, this, -1.0f);
            m_SeenAlertTimer = (float)EXBaseApp::NominalGameFrameRate() * (g_EXRandClass.Randf() + 1.0f);
        }
    }
}

// Drop shadow: an entity under the feet (bigger for Ulysses), shown only while the actor stands on the
// floor, is not carried, cloaked or a rag doll, and the player is in normal vision.
void Actor::UpdateShadow()
{
    if (!m_StateFlags.m_bOnScreen) return;
    if (!m_pShadow) {
        m_pShadow = new EXItemAnimator_Entity;
        m_pShadow->Initialise(EXGeoFile::GetGeoFile(0x1000016), 0x2000141);  // HT_File_Pred_FX, HT_Entity_DroneShadow
        m_pShadow->RenderFlags().BitSet(1);
        m_pItem->AddAnimator(m_pShadow);
        if (m_ActorFlags.m_bUlysses) {
            m_pShadow->RenderFlags().BitSet(2);
            m_pShadow->SetScale(EXVector(3.0f, 1.0f, 3.0f, 0.0f));
        }
    }
    float y = m_pItem->Position().y;
    float floorY = m_OrgLoc.m_vFloorPos.y;
    if (!m_ActorFlags2.m_bInPlayersHand && !m_StateFlags.m_bRagDoll && !m_StateFlags.m_bCloaked &&
        fabs(y - floorY) <= 0.1f && !XItemHandler_Player::Instance()->m_Tint) {
        m_pShadow->ItemFlags().BitSet(1);
        m_pShadow->Invalidate();
        const EXVector &rot = m_pItem->Rotation();
        float rx = rot.x, ry = rot.y, rz = rot.z;
        m_pShadow->SetPosition(EXVector(0.0f, (floorY - y) + 0.05f, 0.0f, 0.0f));
        m_pShadow->m_rotation = EXVector(0.0f - rx, 0.0f - ry, 0.0f - rz, 0.0f);
        m_pShadow->Invalidate();
        return;
    }
    m_pShadow->ItemFlags().BitClr(1);
}

// Starts the death sequence once health runs out (unless the actor cannot die, is inactive, already a
// rag doll or already dying): jumping is switched off, actors due a skull rip get the boss death, the
// hunters 1 and 3, the savage predator and the Ronin only pretend to die (Hunter 2 is left out), bosses play a death anim, and everyone else dies by bullet.
void Actor::UpdateCheckDeath()
{
    if (m_Health > 0.0f || m_ActorFlags.m_bCannotDie) return;
    unsigned int status = m_Status;
    if (status == 0 || status == 0xe || status == 1 || m_StateFlags.m_bRagDoll) return;
    StateFn st = GetBrainState();
    if (st == STATE_NotReallyDead || st == STATE_RagDollDeath || st == STATE_AnimDeath || st == STATE_DeathBullet)
        return;
    m_NavRoute.m_Flags.m_bNoJump = 1;
    if (ShouldBeSkullRipped()) {
        if (GetProfileHash() == 0x47000128) FlagChannels[0x71] = 1;  // HT_ActorProfile_Servituer_Enforcer
        m_SM.Goto(0, STATE_DeathBoss, 0, 0, 0, 0);
        return;
    }
    switch (GetProfileHash()) {
    case 0x47000245:  // HT_ActorProfile_Empire_Hunter_1
    case 0x47000247:  // HT_ActorProfile_Empire_Hunter_3
    case 0x47000159:  // HT_ActorProfile_SavagePredator_01
    case 0x47000238:  // HT_ActorProfile_Penthouse_Ronin
    case 0x47000279:  // HT_ActorProfile_Penthouse_Ronin2
    case 0x4700027a:  // HT_ActorProfile_Penthouse_Ronin3
        m_NavRoute.m_Flags.m_bNoJump = 0;
        m_SM.Goto(0, STATE_NotReallyDead, 0, 0, 0, 0);
        return;
    }
    m_SpeechTag.RemoveAll(1);
    if (m_ActorFlags.m_bBoss) {
        int anim;
        if (SearchBrainStateStack(STATE_CaptivePredHanging)) {
            anim = 0x900048c;
        } else {
            int anims[2];
            anims[0] = 0x9000376;  // HT_AnimMode_BOSS_Death1
            anims[1] = 0x9000377;  // HT_AnimMode_BOSS_Death2
            anim = anims[g_EXRandClass.Rand32() & 1];
        }
        SetAnimMode(anim, 1);
        m_ActorFlags2.m_bIgnoreAnimChanges = 1;
        m_SM.Gosub(0, STATE_AnimDeath, 0, 0, 0, 0);
        return;
    }
    m_SM.Gosub(0, STATE_DeathBullet, 0, 0, 0, 0);
}

// Body awareness. A dead actor offers itself, one nearby actor (within 20 m) per frame, to be seen as a
// body. A living one that has seen a body in its own nav zone goes to raise help, if its sub-group has a
// help animation and it is not hostile to the dead one.
void Actor::UpdateSeeingBodies()
{
    if (m_pOptions->m_BodyAwareness <= 0 || m_ActorFlags.m_bHolographic) return;
    if (m_Health <= 0.0f) {
        Actor *pPrev = GetPtrFromID(m_SeenBodyID);
        Actor *p = pPrev ? pPrev->GetNext(m_pItem->Position(), 20.0f, 0) : GetFirst(m_pItem->Position(), 20.0f, 0);
        if (!p) {
            m_SeenBodyID = 0;
            m_StateFlags.m_bSeenBody = 0;
            return;
        }
        m_SeenBodyID = p->m_RefID;
        p->CheckSeeBody(this);
        m_StateFlags.m_bSeenBody = 0;
        return;
    }
    Actor *pBody = GetPtrFromID(m_SeenBodyID);
    if (pBody && SubGroupAnims::GetSubGroupAnims(*this).m_Anims[4] && !SearchBrainStateStack(STATE_SeenBody_Help) &&
        pBody->m_OrgLoc.m_sNavZone == m_OrgLoc.m_sNavZone && m_StateFlags.m_bSeenBody &&
        (m_pOptions->m_BodyAwareness == 2 || ((m_WatchFlags & 0x10) && (m_WatchFlags & 1)))) {
        if (!AP_DoWeFight(GetProfile(), pBody->GetProfile())) {
            m_SM.ExtGosub(0, STATE_SeenBody_Help, 0, 0, 0, 0);
            m_StateFlags.m_bSeenBody = 0;
        }
    }
}

s8 Within(EXVector p, EXGeoTriggerDatum d);  // Trigger.obj

// A rag doll that drops into a water volume (environment type 0x12) makes a splash on the surface, once.
void Actor::UpdateFallInOcean()
{
    if (!m_StateFlags.m_bRagDoll || m_ActorFlags.m_bFellInOcean) return;
    for (XItem *p = gEnvList.GetFirst(); p; p = gEnvList.GetNext(p)) {
        XItemHandler_Environment *h = (XItemHandler_Environment *)p->ItemHandler();
        if (h->m_Type != 0x12) continue;
        if (!Within(m_pItem->Position(), *(EXGeoTriggerDatum *)&h->m_Datum)) continue;
        const EXVector &pos = m_pItem->Position();
        EXVector splash(pos.x, h->m_Datum.m_Pos.y + h->m_Datum.m_08.y, pos.z, 0.0f);
        XItemHandler_ScriptBase::FX_Create(splash, 0x40006df, 0x40, 0x1000016);  // HT_File_Pred_FX
        m_ActorFlags.m_bFellInOcean = 1;
    }
}

// Profile flag channels: when one of the profile's channels is set, a boss (or any actor told to delete
// itself, unless the player is killing it) abandons what it is doing for the channel's state, once.
void Actor::UpdateFlagChannel()
{
    for (int i = 0; i < 5; i++) {
        int channel = GetProfile()->m_Channels[i].m_Channel;
        if (!channel || !FlagChannels[channel] || m_Unk948.m_Fired[i]) continue;
        int state = GetProfile()->m_Channels[i].m_State;
        if ((m_WatchFlags & 4) || (state == 0x4d000020 && GetBrainState() != STATE_PlayerControlDeath)) {  // HT_ActorState_DeleteNow
            StateFn fn = AP_GetStateFromHash(state);
            m_SM.FlushStack();
            m_SM.ExtGoto(0, fn, 0, 0, 0, 0);
            m_Unk948.m_Fired[i] = 1;
        }
    }
}

// Fades the actor in when within its fade distance of the camera and out beyond it (0.05 per frame).
void Actor::UpdateDistFade()
{
    m_Alpha += m_AlphaSpeed;
    if (m_AlphaSpeed > 0.0f) {
        if (m_Alpha > 1.0f) {
            m_Alpha = 1.0f;
            m_AlphaSpeed = 0.0f;
        }
        return;
    }
    if (m_Alpha < 0.0f) {
        m_Alpha = 0.0f;
        m_AlphaSpeed = 0.0f;
    }
    m_AlphaSpeed = m_SqDistFromCamera <= m_FadeDistance * m_FadeDistance ? 0.05f : -0.05f;
}

// Counts the actor if it stands inside the box and has the box's profile (or the box takes any).
signed char ActorOptions::Within(Actor *pActor)
{
    const EXVector &p = pActor->GetItem()->Position();
    if (fabs(p.x - m_Pos.x) > m_Extent.x || fabs(p.y - m_Pos.y) > m_Extent.y || fabs(p.z - m_Pos.z) > m_Extent.z)
        return 0;
    if (m_Profile == pActor->GetProfile()->m_Hash || m_Profile == 0x47000000) m_Count++;  // HT_ActorProfile_NONE
    return 1;
}

// End of frame: drives the box's flag channels from the actors counted inside it and from whether the
// player is inside (optionally only in a given vision mode), then clears the per-frame state.
void ActorOptions::PostProcess()
{
    if (m_OccupiedChannel && !FlagChannels[m_OccupiedChannel] && m_Count) FlagChannels[m_OccupiedChannel] = 1;
    if (m_CountChannel) FlagChannels[m_CountChannel] = m_Count != 0;
    m_Count = 0;
    XItemHandler_Player *pPlayer = XItemHandler_Player::Instance();
    const EXVector &pp = pPlayer->GetItem()->Position();
    bool bInside = fabs(pp.x - m_Pos.x) <= m_Extent.x && fabs(pp.y - m_Pos.y) <= m_Extent.y &&
                   fabs(pp.z - m_Pos.z) <= m_Extent.z;
    if (m_PlayerEnteredChannel && !FlagChannels[m_PlayerEnteredChannel] && bInside &&
        (!m_VisionMode || (unsigned int)XItemHandler_Player::Instance()->m_Tint == (unsigned int)m_VisionMode))
        FlagChannels[m_PlayerEnteredChannel] = 1;
    if (m_PlayerInChannel) {
        if (!m_VisionMode || (unsigned int)pPlayer->m_Tint == (unsigned int)m_VisionMode) {
            FlagChannels[m_PlayerInChannel] = bInside;
        } else {
            bInside = false;
            FlagChannels[m_PlayerInChannel] = 0;
        }
    }
    if (m_PlayerOutChannel) FlagChannels[m_PlayerOutChannel] = !bInside;
    if (m_MimicryChannel && m_bMimicryUsed && bInside) FlagChannels[m_MimicryChannel] = 1;
    m_bMimicryUsed = 0;
    if (bInside && (m_Flags & 1)) ActorOptionsManager::m_bPlayerVisible = 0;
}
