#ifndef ACTOR_H
#define ACTOR_H
#include "types.h"
#include "EXItemHandler.h"
#include "EXItem.h"
#include "EXVector.h"
#include "EXList.h"
#include "EXBaseApp.h"
#include "EXRand.h"
#include "RGBA.h"
#include "StateMachine.h"
#include "ActorProfile.h"
#include "SubGroupAnims.h"
#include "XNavStandIn.h"
#include "EXExtRenderInfoStandIn.h"
#include "EXSoundTagStandIn.h"

class XItem;
class EXItem;
class EXItemAnimator;
class EXItemAnimator_Anim;
class EXItemAnimator_Script;
class EXItemAnimator_Entity;
class EXQuaternion;
class EXRay;
class EXTriangle;
class EXCapsule;
class EXLineSeg;
class EXVector2;
class Vehicle;
class ActorCarryable;
class ActorCarryableGun;
class WeaponProfile;
class XItemHandler_Objective;
class ActorEffect;
class EX6DAnim;
class EX6DBodyGroup;
class ActorOptions;
class ActorTextureSwapItem;
class FEObject;
struct ActorTimeTable;
struct ScanInfo;
struct GEOSCRIPT_SOUND;
struct EXSoundScriptTemp;
template <class T, int N> class EXFixedArray;

// TODO stand-in: texture swap table (EXRenderEnv.obj, engine render area). sizeof 0x14.
#include "EXTexSwapInfo.h"

// Limbs (texture swaps: HT_Texture_Swap_LeftArm, _RightArm, _Head, _Belt).
enum ActorLostLimb {
    ALL_LEFTARM = 0,
    ALL_RIGHTARM = 1,
    ALL_HEAD = 2,
    ALL_BELT = 3        // TODO name
};
enum BloodType {
    BLOOD_RED = 0       // TODO names
};

// A delayed message (sizeof 0x1c).
struct QueuedMsgItem {
    QueuedMsgItem() {}
    float m_Delay;      // 0x00 TODO
    int m_ActorID;      // 0x04 TODO
    MsgItem m_Msg;      // 0x08
};

// Slot of the actor list (g_aActorList, 128 slots indexed by id & 0x7f; sizeof 0x18).
class ActorBase {
public:
    ActorBase()
    {
        m_pActor = 0;
        m_bInWorld = 0;
    }
    EXVector m_Pos;     // 0x00 feet position (DoUpdate)
    Actor *m_pActor;    // 0x10
    s8 m_bInWorld;      // 0x14
};
extern ActorBase g_aActorList[128];
// Actor.obj lod tables (4 EXAnimLodDetails each; Offscreen has 1).
struct EXAnimLodDetails;
extern EXAnimLodDetails g_LodDetailsOnScreenUlysses[];
extern EXAnimLodDetails g_LodDetailsOnScreenBoss[];
extern EXAnimLodDetails g_LodDetailsOnScreenAlien[];
extern EXAnimLodDetails g_LodDetailsOnScreenDrone[];
extern EXAnimLodDetails g_LodDetailsOffscreen[];
inline ActorBase *GetActorBasePtr(int id) { return &g_aActorList[id & 0x7f]; }
extern float gActorFadeDist;
extern signed char gActorPause;
extern signed char gActorHide;
inline void Actor_SetPause(signed char b) { gActorPause = b; }
inline signed char Actor_GetPause() { return gActorPause; }
inline void Actor_SetHide(signed char b) { gActorHide = b; }
inline signed char Actor_GetHide() { return gActorHide; }
inline void Actor_SetFadeDistance(float d) { gActorFadeDist = d; }
inline void __cdecl ACTOR_TRACE(...) {}
// TODO owning header: small game maths helpers ([fi] copies in Actor.obj right after ACTOR_TRACE;
// Sq(int) is a separate [fi] in ActorSpecial.obj). Need EXRand.h.
inline float Sq(float x) { return x * x; }
inline int randi(int lo, int hi) { return EXRand(lo, hi + 1); }
__forceinline float randf(float lo, float hi) { return EXRandf(lo, hi); }

// Actor::m_StateFlags (+0x93c).
struct ActorStateFlags {
    u32 m_b0 : 1;
    u32 m_b1 : 1;
    u32 m_b2 : 1;
    u32 m_b3 : 1;
    u32 m_bDontDoHitReaction : 1;   // 4
    u32 m_bLyingOnGround : 1;       // 5
    u32 m_bTargetable : 1;          // 6
    u32 m_bExpandedFightDist : 1;   // 7 GetFightDistance x4
    u32 m_b8 : 1;                   // 8 set by DoUpdate; SetHeadTrackMode sets it to mode == 1
    u32 m_bCloaked : 1;             // 9
    u32 m_bLyingFaceDown : 1;       // 10
    u32 m_bCanCollideWithPlayer : 1;// 11
    u32 m_bCanRipBossHead : 1;      // 12
    u32 m_bStartBossHeadRipped : 1; // 13
    u32 m_bMoving14 : 1;            // 14 (IsMoving)
    u32 m_bMoving15 : 1;            // 15 (IsMoving)
    u32 m_bValidRoute : 1;          // 16
    u32 m_bRagDoll : 1;             // 17
    u32 m_bHeadTrack : 1;           // 18
    u32 m_bAimingPos : 1;           // 19 m_AimingPos set (SetAimingPos)
    u32 m_bHide : 1;                // 20
    u32 m_bPlayerSeen : 1;          // 21
    u32 m_bBrainScan : 1;           // 22
    u32 m_bHitByCar : 1;            // 23
    u32 m_bOnScreen24 : 1;          // 24
    u32 m_bOnScreen : 1;            // 25
    u32 m_bSeenBody : 1;            // 26
    u32 m_bMoving27 : 1;            // 27 (IsMoving)
    u32 m_b28 : 1;
    u32 m_bDontDragMeSpear : 1;     // 29
    u32 m_bBeingStealthKilled : 1;  // 30
    u32 m_bAnimSpeedRand : 1;       // 31 AnimSpeedMult applies m_AnimSpeedRand
};
// Actor::m_ActorFlags (+0x940).
struct ActorFlags {
    u32 m_bCanBeJuggled : 1;        // 0
    u32 m_bBeingJuggled : 1;        // 1
    u32 m_bHolographic : 1;         // 2
    u32 m_bLeftArmMissing : 1;      // 3
    u32 m_bRightArmMissing : 1;     // 4
    u32 m_bInCombatWithPlayer : 1;  // 5
    u32 m_bCanLooseArmAndLive : 1;  // 6
    u32 m_bOverkilled : 1;          // 7
    u32 m_bDiscNotInHand : 1;       // 8
    u32 m_bVariableSpeed9 : 1;      // 9
    u32 m_bHasBeenThrow : 1;        // 10
    u32 m_bDeathObjective : 1;      // 11 death objective awarded
    u32 m_bGoingToStaggerAndDie : 1;// 12
    u32 m_bVariableSpeedAnims : 1;  // 13
    u32 m_bMovementEnabled : 1;     // 14
    u32 m_bBoss : 1;                // 15
    u32 m_bUlysses : 1;             // 16
    u32 m_bAlien : 1;               // 17
    u32 m_bHasThrownPetrolBomb : 1; // 18
    u32 m_bPlayerHolds : 1;         // 19
    u32 m_b20 : 1;
    u32 m_bFellInOcean : 1;         // 21 splash spawned (UpdateFallInOcean)
    u32 m_b22 : 1;
    u32 m_b23 : 1;
    u32 m_b24 : 1;
    u32 m_b25 : 1;
    u32 m_b26 : 1;
    u32 m_bHasCollided : 1;         // 27
    u32 m_bBlinded : 1;             // 28
    u32 m_b29 : 1;
    u32 m_bAiming : 1;              // 30
    u32 m_bCannotDie : 1;           // 31
};
// Actor::m_ActorFlags2 (+0x944).
struct ActorFlags2 {
    u32 m_bInPlayersHand : 1;       // 0
    u32 m_bOnFire : 1;              // 1
    u32 m_bCouldPlayerBeSeen : 1;   // 2
    u32 m_bNudged : 1;              // 3 player came close (NoticePlayer)
    u32 m_bFiringAnim : 1;          // 4
    u32 m_bHanging : 1;             // 5
    u32 m_bLyingBum : 1;            // 6
    u32 m_bDazed : 1;               // 7
    u32 m_bScriptRunControl : 1;    // 8 TODO
    u32 m_bBeingDestructed : 1;     // 9
    u32 m_bPredator : 1;            // 10
    u32 m_bHasBeenKicked : 1;       // 11
    u32 m_b12 : 1;
    u32 m_bLongRangeVision : 1;     // 13
    u32 m_bAnimEnded : 1;           // 14
    u32 m_bFleeToStatue : 1;        // 15
    u32 m_bIgnoreAnimChanges : 1;   // 16
    u32 m_b17 : 15;
};

// POI stand points (ActorLocation.obj; also declared in ActorLocation.h), used by CanGetToVendorPOI.
int POI_VendorStandIndex(int, int);
EXVector POI_VendorStandPos(int, int);
int POI_VendorStandPoly(int, int);

// Actor+0x948: one "already fired" flag per profile flag channel (UpdateFlagChannel), cleared by the
// constructor as an int and a byte (sizeof 8).
struct ActorUnk948 {
    void Clear()
    {
        m_00 = 0;
        m_04 = 0;
    }
    union {
        struct {
            int m_00;
            s8 m_04;
        };
        s8 m_Fired[5];
    };
};

// The NPC item handler: humans, aliens, police (Actor.obj, sizeof 0xcb0). Brain = m_SM.
class Actor : public EXItemHandler {
    EX_DECLARE_DYNAMIC(Actor)
public:
    // One pushed state of the brain stack (sizeof 0x80).
    struct ActorStack {
        ActorStack() {}
        EXVector3 m_DestPos;        // 0x00 m_DestLoc position when pushed
        int m_DestPoly;             // 0x0c m_DestLoc poly handle
        int m_AnimMode;             // 0x10
        int m_AnimType;             // 0x14
        u32 m_Unk18;
        union {
            u32 m_Flags;            // 0x1c stack flags; bits 30-31 are not message bits
            struct {
                u32 m_MsgBits : 30;
                u32 m_bGotoPos : 1; // 30 m_StateFlags.m_bMoving15 when pushed
                u32 m_b31 : 1;
            };
        };
        unsigned int m_LocalData[20];  // 0x20 the state's locals
        StateFn m_State;    // 0x70
        u8 m_Unk74[0x80 - 0x74];
    };
    // A lost limb (sizeof 0x38).
    struct LostLimbStruct {
        LostLimbStruct() {}
        int m_Id;           // 0x00 non-zero: limb item exists (EXItem *)
        EXItemAnimator_Script *m_pBlood;  // 0x04 blood script at the stump (CreateLostLimb)
        EXItemAnimator_Script *m_pBlood2; // 0x08 second stump script (released at script end)
        EXVector m_Vel;     // 0x0c per-frame move (UpdateLostLimbs; y falls to -1.2)
        EXVector m_RotVel;  // 0x1c per-frame rotation
        float m_Life;       // 0x2c frames the limb stays (120; 600 when held)
        float m_Alpha;      // 0x30
        union {
            u32 m_Flags;    // 0x34 bit 0: lost, bit 1: held by the player / at rest
            struct {
                u32 m_bLost : 1;
                u32 m_bHeld : 1;
            };
        };
    };

    virtual ~Actor();
    virtual signed char Initialise();
    virtual signed char DoUpdate();
    virtual unsigned int DoScriptCmdEvent(GEOSCRIPT_EVENT *pEvent, EXItemAnimator *pAnim);
    virtual unsigned int DoScriptCmdRunControl(GEOSCRIPT_RUNCONTROL *pCmd, EXItemAnimator *pAnim);

    signed char BeingDestructed() { return m_ActorFlags2.m_bBeingDestructed == 1; }
    int GetRefID() { return m_RefID; }
    StateFn GetBrainState() { return m_SM.m_State; }
    signed char IsLimbLost(ActorLostLimb limb) { return (signed char)(m_LostLimbs[limb].m_Flags & 1); }
    float AnimSpeedMult()
    {
        return m_StateFlags.m_bAnimSpeedRand ? m_AnimSpeedMult * m_AnimSpeedRand : m_AnimSpeedMult;
    }
    int GetAnimMode() const { return m_AnimMode; }
    ex::RGBA GetBrainScanColour()
    {
        if (m_StateFlags.m_bBrainScan == 1) {
            float p = ((float)sin(gAlphaPulse) + 1.0f) * 0.5f;
            return ex::RGBA((u8)(m_BrainScanColour.m_R * p), (u8)(m_BrainScanColour.m_G * p),
                            (u8)(m_BrainScanColour.m_B * p), 0x80);
        }
        return m_BrainScanColour;
    }
    EXVector &LocalVelocity() { return m_LocalVelocity; }
    EXVector &WorldVelocity() { return m_WorldVelocity; }
    const float GetRotY() { return m_pItem->Rotation().y; }
    const EXVector &GetFeetPos() const { return m_pItem->Position(); }
    const EXVector &GetRotation() const { return m_pItem->Rotation(); }
    XNavRoute &NavRoute() { return m_NavRoute; }
    void SetAttrib(unsigned int attrib, signed char on)
    {
        m_Attrib &= ~attrib;
        if (on) m_Attrib |= attrib;
    }
    float GetHealth() const { return m_Health; }
    signed char GetHide() const { return m_StateFlags.m_bHide == 1; }
    signed char IsDead() const { return m_Health <= 0.0f; }
    EXTexSwapInfo *TexSwapInfo() { return &m_TexSwapInfo; }
    XNavLocation &OrgLoc() { return m_OrgLoc; }
    int GetNavZone() { return m_OrgLoc.NavZone(); }
    void DisableJumping(signed char b) { m_NavRoute.m_Flags.m_bNoJump = b; }
    ActorProfile *GetProfile() const { return AP_GetPtrFromIndex(m_ProfileIndex); }
    ActorSkin *GetSkin() const { return AS_GetPtrFromIndex(m_SkinIndex); }
    void SetWatch(unsigned int w) { m_WatchFlags = w; }
    signed char WasPlayerSeen() { return m_StateFlags.m_bPlayerSeen == 1; }
    StateMachine *GetSM() { return &m_SM; }
    int GetType() { return GetSkin()->m_Type; }
    float GetAliveRadius() { return m_AliveRadius; }
    void SetOverkilled() { m_ActorFlags.m_bOverkilled = 1; }
    void DecrementDodgeTimer()
    {
        m_DodgeTimer = m_DodgeTimer > 0.0f ? m_DodgeTimer - EXBaseApp::GameFrameRateMul() : 0.0f;
    }
    EXDList *EffectList() { return &m_EffectList; }
    signed char IsSpecial() { return GetProfile()->m_SubGroup == 0x4f00002b; }
    ActorOptions *Options() { return m_pOptions; }
    float HolographicMult() { return m_ActorFlags.m_bHolographic ? 0.75f : ms_fActorAlpha; }
    signed char Holographic() { return m_ActorFlags.m_bHolographic == 1; }
    signed char VariableSpeedAnims() { return m_ActorFlags.m_bVariableSpeed9 && m_ActorFlags.m_bVariableSpeedAnims; }
    signed char ShouldBeNoticed() { return m_Status == 3; }
    static int GetActorIDPlayerHasHead() { return ms_iActorIDPlayerHasHead; }
    EXItemAnimator_Anim *GetAnim() { return m_pAnim; }
    signed char IsAiming() { return m_ActorFlags.m_bAiming == 1; }
    void SetInPlayersHand(signed char b) { m_ActorFlags2.m_bInPlayersHand = b != 0; }
    signed char InPlayersHand() { return m_ActorFlags2.m_bInPlayersHand == 1; }
    signed char OnFire() { return m_ActorFlags2.m_bOnFire == 1; }
    void SetHitByVehiclePtr(Vehicle *p) { m_pHitByVehicle = p; }
    Vehicle *HitByVehiclePtr() { return m_pHitByVehicle; }
    unsigned int Affiliation() { return GetProfile()->m_SubGroup; }
    static void SetGlobalAffiliation(unsigned int a) { m_gAffiliation = a; }
    static void ResetGlobalAffiliation() { m_gAffiliation = 0xffffffff; }
    static unsigned int GlobalAffiliation() { return m_gAffiliation; }
    float SqDistFromCamera() { return m_SqDistFromCamera; }
    float FadeDistance() { return m_FadeDistance; }
    signed char IsOnScreen(signed char b)
    {
        if (b) return m_StateFlags.m_bOnScreen == 1;
        return m_StateFlags.m_bOnScreen24 == 1;
    }
    SubGroupAnims &GetSubGroupAnims() const { return SubGroupAnims::GetSubGroupAnims(m_SubGroupAnimIndex); }
    int GetSkelHash() const { return GetSkin()->m_SkelHash; }
    float GetCollisionRadius() const { return m_CollisionRadius; }
    signed char IsBoss() const { return m_ActorFlags.m_bBoss == 1; }
    signed char IsUlysses() const { return m_ActorFlags.m_bUlysses == 1; }
    signed char IsAlien() const { return m_ActorFlags.m_bAlien == 1; }
    signed char IsPredator() const { return m_ActorFlags2.m_bPredator == 1; }
    signed char IsDrone() const { return !m_ActorFlags.m_bBoss && !m_ActorFlags.m_bUlysses && !m_ActorFlags.m_bAlien; }
    signed char IsLyingOnGround() const { return m_StateFlags.m_bLyingOnGround == 1; }
    void SetIsTargetable(signed char b) { m_StateFlags.m_bTargetable = b != 0; }
    signed char IsCloaked() const { return m_StateFlags.m_bCloaked == 1; }
    void SetIsDiscNotInHand(signed char b) { m_ActorFlags.m_bDiscNotInHand = b != 0; }
    signed char CanRipBossHead() const { return m_StateFlags.m_bCanRipBossHead == 1; }
    void SetDontDoHitReaction(signed char b) { m_StateFlags.m_bDontDoHitReaction = b != 0; }
    signed char LongRangeVision() const { return m_ActorFlags2.m_bLongRangeVision == 1; }
    signed char IsBeingStealthKilled() const { return m_StateFlags.m_bBeingStealthKilled == 1; }
    signed char HasBeenThrow() const { return m_ActorFlags.m_bHasBeenThrow == 1; }
    void SetHasBeenThrow(signed char b)
    {
        m_ActorFlags.m_bHasBeenThrow = b != 0;
        m_ThrownPos = GetFeetPos();
    }
    signed char CanLooseArmAndLive() const { return m_ActorFlags.m_bCanLooseArmAndLive == 1; }
    void SetIgnoreAnimChanges(signed char b) { m_ActorFlags2.m_bIgnoreAnimChanges = b != 0; }
    signed char CannotDie() const { return m_ActorFlags.m_bCannotDie == 1; }
    signed char IsBlinded() const { return m_ActorFlags.m_bBlinded == 1; }
    signed char IsLeftArmMissing() const { return m_ActorFlags.m_bLeftArmMissing == 1; }
    signed char IsRightArmMissing() const { return m_ActorFlags.m_bRightArmMissing == 1; }
    signed char HasThrownPetrolBomb() const { return m_ActorFlags.m_bHasThrownPetrolBomb == 1; }
    signed char FleeToStatue() const { return m_ActorFlags2.m_bFleeToStatue == 1; }
    void SetFleeToStatue(signed char b) { m_ActorFlags2.m_bFleeToStatue = b != 0; }
    signed char BeingJuggled() const { return m_ActorFlags.m_bBeingJuggled == 1; }
    signed char IsRagDoll() const { return m_StateFlags.m_bRagDoll == 1; }
    void ClearConsecutiveHitCounter() { m_ConsecutiveHitCounter = 0; }
    void IncConsecutiveHitCounter() { m_ConsecutiveHitCounter++; }
    void SetRadarType(unsigned char t) { m_RadarType = t; }
    void SetInvinsibleCounter(int n) { m_InvinsibleCounter = (float)n; }
    void TurnOffAiming()
    {
        m_AimingTime = 0.0f;
        m_AimingTarget = 0;
    }
    float GetActorToPlayerColAdj() const { return m_ActorToPlayerColAdj; }
    signed char HasHitPredRecently() const { return m_HitPredTimer > 0.0f; }
    signed char AboutToRaiseAlertLevel() const { return m_AlertRaiseTimer > 0.0f && GetProfile()->m_AlertTime > 0.0f; }
    float GetEMPTimer() { return m_EMPTimer; }
    static Actor *GetPtrFromID(int id);
    signed char InWorld() { return GetActorBasePtr(m_RefID)->m_bInWorld; }
    static void MsgQueueInit();
    void SetPosition(const EXVector &pos, int poly);
    void SetPositionEx(const EXVector &pos);
    signed char SearchBrainStateStack(StateFn state) const;
    signed char IsBitMsgOnStack(unsigned int bits) const;
    void ClearBitMsgOnStack(unsigned int bits);
    signed char IsMsgOnStack(MsgType msg) const;
    static void BootInit();
    static void PostLoadInit();
    float DistToTarget();
    float SqDistToTarget();
    void InvalidateGotoPosOnly();
    void InvalidateGotoPos();
    void InvalidateGotoPosButNotTargetYRot();
    signed char CanChangeSkinEver() const;
    signed char IsRagDollAllowed();
    float GetAnimFrame();
    ex::RGBA UpdateBrainScanColour(signed char b);
    signed char InitTriggerJump();
    EXVector CalcJumpDelta();
    signed char AtYRot(float rot, float tol);
    static Actor *GetFirst(signed char b);
    Actor *GetNext(signed char b);
    void SetRotY(float rot);
    void SpinToYRot(float rot);
    signed char IsFacingYRot(float rot, float tol);
    void NoAnimBlend();
    float GetAnimFrameCount();
    void SetAnimFrame(float f);
    void CommitAnimSpeedMult();
    static signed char IsAnimVariableSpeed(int anim, Actor *pActor);
    static signed char IsAnimVariableStart(int anim, Actor *pActor);
    signed char GetAnimDatum(int hash, EXVector &v);
    signed char GetBonePos(unsigned int bone, EXVector &v);
    signed char IsHeadshot(EXVector &v, float f);
    signed char HasLostLimb() const;
    signed char HasLostLimb(int limb) const;
    void UpdateLostLimbs();
    void SetHeadTrack(const EXVector &v);
    signed char ShouldBeSkullRipped();
    void AwardDeathObjective();
    void SetHealth(float h, unsigned int from);
    void ChangeHealth(float d, unsigned int from);
    signed char IsTargetable() const;
    signed char CheckBossDamageTriggers(float d);
    void SetDefaultContextIcon(int icon, signed char b);
    EXVector GetHeadDirection();
    void SetIsInCombatWithPlayer(signed char b);
    signed char CheckRayIntersection(const EXRay &ray) const;
    static void OverridePlayerCloak();
    void InstantAlarm();
    void CancelAlarmMeter();
    void UpdateAlarmMeter();
    signed char HasAnimEnded();
    void SetAlpha(float a);
    void __CarryFlush(int i);
    signed char CarryingHiddenWeapon() const;
    signed char CarryingMixedWeapons() const;
    void SetCarry(int hash, signed char left);
    ActorCarryable *GetCarryPtr(signed char left);
    ActorCarryableGun *GetGunPtr(signed char left);
    WeaponProfile *GetWeaponProfile(signed char left);
    void SpeakStop();
    signed char IsSpeaking();
    int GetDeathSFX();
    int GetScreamSFX();
    int GetHitSFX();
    int GetDazedSFX();
    int GetWarcrySFX();
    unsigned int RemapSFX(int sfx);
    void StopSFX(unsigned int sfx);
    void StopLoopingSFX(unsigned int sfx);
    void StopAllSFX();
    void StopAllLoopingSFX();
    signed char IsSFXPlaying(unsigned int sfx) const;
    unsigned int GetTotalStackFlags() const;
    void PushState(StateFn state, unsigned int flags);
    signed char HasRagDollComeToRest();
    void GrabRagdoll();
    void LetGoRagdoll();
    void UpdateRagdollGrabPoint();
    void SetHideCarryRight(signed char b);
    void SetHideCarryLeft(signed char b);
    XItemHandler_Objective *GetObjective();
    static Actor *Create(int profile, const EXVector &pos, int poly, float rotY, int a, int b, ActorTimeTable *pTable);
    static void NotifyCutscene(signed char b);
    static void VehicleRemoved(Vehicle *pVehicle);
    void CalculateParameters();
    void ChangeSkin(unsigned int a, unsigned int b, unsigned int c);
    ActorEffect *EffectGet(unsigned __int64 id);
    signed char EffectDeleteAll();
    signed char CanBeJuggled() const;
    signed char CanLooseLimbs() const;
    signed char CanBeStealthKilled() const;
    signed char NearToRecentLastKnownLocation() const;
    signed char CanBeGrabbed() const;
    signed char CanBeFinishedOff() const;
    float GetGunYOffsetFromFeet(signed char left);
    int GetHandDatumHash(signed char left);
    EXVector GetWeaponPos(signed char left);
    signed char IsCollisionPhysRadiusCheckValid();
    signed char CanLeapAbout() const;
    signed char DecideIsBoss() const;
    signed char HasBossBar() const;
    signed char DecideIsUlysses() const;
    signed char DecideIsAlien() const;
    signed char DecideIsPredator() const;
    signed char IsDiveAllowed() const;
    signed char IsDodgeAllowed() const;
    signed char IsCarryingGun();
    signed char PetrolBombInProfile() const;
    EXVector GetPelvisPosApprox();
    EXVector GetHeadPosApprox();
    EXVector GetChestPosApprox();
    BloodType GetBloodType() const;
    EXVector GetGunDirection(const EXVector &target);
    void SetFiringAnim(signed char b);
    void CalcMass();
    void ResetCivilianAttackVO();
    signed char CanCollideWithPlayer() const;
    void ResetLastHitTimer();
    void ClearLastHitTimer();
    signed char WillThisDaze(float d);
    void PlayActorWeaponImpactSFX(int a, const EXVector &pos, WeaponProfile *pWeapon, signed char b);
    void ExpandFightDist();
    float GetFightDistance() const;
    signed char IsMafiaBossVulnerable(int i);
    void CatchDisc();
    // NEAR: retail masks with `and al, 1` (a bitfield read?), this gives `and eax, 1`
    signed char NeedsDelete() { return (signed char)((m_pItem->ItemFlags() >> 4) & 1); }
    static Actor *GetActorPtr(int id)
    {
        if (!id) return 0;
        Actor *p = GetActorBasePtr(id)->m_pActor;
        if (!p) return 0;
        return p->m_RefID != id ? 0 : p;
    }
    signed char IsHitable() const
    {
        if (m_HitCountdown > 0.0f) return 0;
        if (IsDead() && !CanRipBossHead()) return 0;
        if (m_ActorFlags.m_bHolographic) return 0;
        return 1;
    }
    signed char CanJump() { return m_ActorFlags.m_bBoss && !m_NavRoute.NoJump(); }
    int GetProfileHash() const { return GetProfile()->m_Hash; }
    static signed char ScanCallback(signed char b, FEObject *pObj, ScanInfo *pInfo);
    Actor();
    void UpdateThrownDistStat();
    static void MsgQueueAdd(const MsgItem &msg, Actor *pActor, float delay);
    int SendMsg(const MsgItem &msg, float delay);
    static int SendMsg(int id, const MsgItem &msg, float delay);
    float DistFromSource();
    EXVector GetAnimDelta();
    float GetAnimVelocity();
    void JumpToPos(const EXVector &pos);
    signed char ProcessTriggerJump();
    static Actor *GetFirst(const EXVector &pos, float radius, signed char b);
    Actor *GetNext(const EXVector &pos, float radius, signed char b);
    void SpinToYRot(const EXVector &pos);
    signed char IsFacingYRot(const EXVector &pos, float tol);
    int CheckCollision(const EXTriangle &tri, float r, EXVector &out);
    int CheckCollision(const EXCapsule &cap, EXVector &out);
    int CheckCollision(const EXVector *p, float r, EXVector &out);
    signed char FriendlyActorBlockingLineOfSight(const EXRay &ray);
    EXVector GetHandPos(signed char left);
    static void DeleteHeldHead();
    void SetRippedHeadPosRot(const EXVector &pos, const EXQuaternion &rot);
    void SetHeadTrackMode(int mode);
    signed char CanReachZone(int zone);
    signed char BossFlees();
    void DumpDebugInfo();
    void CheckForFriendsInCombat();
    static void DoScriptCmdSound(const EXItemAnimator *pAnim, const GEOSCRIPT_SOUND *pCmd, EXSoundScriptTemp &t);
    signed char CurrentAnimLoops();
    void CheckHasAnimEnded();
    unsigned int RemapVocalMimicrySoundHash(unsigned int hash);
    void Delete();
    void EnableRagDoll();
    EXVector GetRagDollVelocity() const;
    void ApplyRagDollForce(const EXVector &pos, const EXVector &force, float f, int i);
    void ApplyRagDollForceAll(const EXVector &force, float f);
    void SetHide(signed char b);
    static void PreLoadInit();
    void DeleteChangeSkin();
    signed char AreThereFriendlyActorsNear(const EXVector &pos, float radius);
    signed char IsPlayerStoodOnMe() const;
    signed char CanMoveRelative(const EXVector &delta);
    signed char CanStrafeLeft(float d);
    signed char CanStrafeRight(float d);
    signed char CanMoveBack(float d);
    signed char CanMoveForward(float d);
    EXVector GetPelvisPos();
    EXVector GetHeadPos();
    EXVector GetChestPos();
    EXVector GetAimPosForTarget(int id);
    void SetupDepthPtr(float *p, unsigned int n);
    signed char AllyNear(float radius, signed char b);
    void UpdateLastHitTimer();
    static Actor *CreateObject();
    signed char GetScanInfo(int i, ScanInfo *pInfo);
    void NoticeActor(Actor *pActor, signed char b);
    static void MsgQueueProcess();
    static void SendMsg(const MsgItem &msg, const EXVector &pos, float radius, Actor *pFrom, float delay);
    static void SendMsgCapped(const MsgItem &msg, const EXVector &pos, float radius, Actor *pFrom, float delay, float a, float b);
    static void SendMsg(const MsgItem &msg, const EXVector &a, const EXVector &b, float f, float g, Actor *pFrom, float delay);
    signed char GotoPosSlow(const EXVector &pos);
    signed char GotoPosDelta(const EXVector &delta);
    signed char GotoLoc(XNavLocation &loc);
    signed char GotoPosPoly(const EXVector &pos, int poly);
    signed char GotoPOI(int poi);
    signed char GotoCustomerPOI(int poi, int i);
    signed char GotoVendorPOI(int poi, int i);
    void SetCloakDirect(signed char b);
    void SetAnimType(int type, signed char a, signed char b);
    int CheckCollision(const EXLineSeg &seg, float r, EXVector &out);
    static void SetActorIDPlayerHasHead(int id, XItem *pHead);
    void DeleteLostLimb(ActorLostLimb limb);
    signed char CanSeePoint(const EXVector &pos, float f);
    void CheckSeeBody(Actor *pBody);
    void SwapHands();
    void PlaySFX(int sfx, signed char b);
    void Speak(int sfx, signed char b);
    void SayBulletOuch();
    void UpdateSFX();
    StateFn PopState();
    void UpdateChangeSkin();
    signed char EffectUpdate();
    void HitByVehicle(Vehicle *pVehicle, EXFixedArray<EXVector2, 4> &box);
    void DamageOtherActorsIfRagdoll();
    void SetAimingAnim();
    void UpdateCivilianAttackVO();
    signed char TryToSpeakCombatVO(Actor *pActor);
    void SetTargetYRot(float rot, float speed);
    void CreateLostLimb(ActorLostLimb limb, const EXVector &dir, signed char a, signed char b);
    void SayAttackVO();
    void SayAttackBigVO();
    void SayNudgedVO();
    void SayLuredVO();
    signed char EffectCreate(unsigned int a, unsigned int b, const EXVector &pos, float t, signed char c, unsigned __int64 id);
    signed char EffectCreate(unsigned int a, unsigned int b, unsigned int c, float t, signed char d, unsigned __int64 id);
    void SetAimingPos(EXVector pos);
    void NoticePlayer();
    void SparkCloak(int i);
    void MafiaBossSheild(int i);
    void SetTargetYRot(const EXVector &pos, float speed);
    void ProcessHit(const EXVector &pos, const EXVector &dir, float d);
    void LooseArmIfHit(const EXVector &pos, const EXVector &dir, float d);
    void ExplodeLimbs(const EXVector &pos, signed char b);
    signed char DoNonCriticalUpdate();
    void SetCloak(signed char b);
    void BreakCloak(int i);
    signed char CheckForSkinChange(float f);
    void AddToWorld();
    static void Update();
    // Inlines emitted in other objects.
    signed char SeenBody() { return m_StateFlags.m_bSeenBody == 1; }
    void SetFiredBullet();
    void SetAnimSpeedMult(float m) { m_AnimSpeedMult = m; }
    EXVector RandomPosition() { return m_NavRoute.RandomPosition(&m_OrgLoc); }
    void SetMeetActorID(int id) { m_MeetActorID = id; }
    unsigned int GetAttrib() { return m_Attrib; }
    void SetStatus(unsigned int s) { m_Status = s; }
    void SetIsLyingBum(signed char b) { m_ActorFlags2.m_bLyingBum = b != 0; }
    signed char CanGetToPosSlow(const EXVector &pos) { return m_NavRoute.RouteQuickTest(&m_OrgLoc, pos, -1); }
    signed char CanGetToPosPoly(const EXVector &pos, int poly)
    {
        XNavLocation loc;
        loc.SetPosPoly(pos, poly, 0);
        return m_NavRoute.RouteQuickTest(&m_OrgLoc, &loc);
    }
    signed char CanGetToLocation(XNavLocation &loc) { return m_NavRoute.RouteQuickTest(&m_OrgLoc, &loc); }
    // NEAR: 2 bytes differ (lea of the stand-pos temporary uses ecx, retail eax)
    signed char CanGetToVendorPOI(int poi, int i)
    {
        if (i < 0) i = POI_VendorStandIndex(poi, m_RefID);
        return CanGetToPosPoly(POI_VendorStandPos(poi, i), POI_VendorStandPoly(poi, i));
    }
    void ResetHitCountdown() { m_HitCountdown = 10.0f; }
    int GetNavPolyHandle() { return m_OrgLoc.GetPolyHandle(); }
    unsigned short GetSurfaceFlags() { return m_OrgLoc.SurfaceFlags(); }
    signed char InSafeArea() { return (m_OrgLoc.SurfaceFlags() >> 10) & 1; }
    void EnableMovement(signed char b) { m_ActorFlags.m_bMovementEnabled = b != 0; }
    void PlayerForceSighting() { m_ForceSightingTimer = EXBaseApp::NominalGameFrameRate() * 3.0f; }
    void SetVariableSpeedAnims(signed char b) { m_ActorFlags.m_bVariableSpeedAnims = b != 0; }
    float SqDistFromPlayer() { return m_SqDistFromPlayer; }
    void StopCombatCircleAdjustment(float t) { m_CombatCircleStopTime = t; }
    signed char IsDiscNotInHand() const { return m_ActorFlags.m_bDiscNotInHand == 1; }
    signed char IsDazed() const { return m_ActorFlags2.m_bDazed == 1; }
    signed char DontDoHitReaction() const { return m_StateFlags.m_bDontDoHitReaction == 1; }
    void SetLongRangeVision(signed char b) { m_ActorFlags2.m_bLongRangeVision = b != 0; }
    void SetCanLooseArmAndLive(signed char b) { m_ActorFlags.m_bCanLooseArmAndLive = b != 0; }
    signed char IsLyingBum() const { return m_ActorFlags2.m_bLyingBum == 1; }
    signed char IsGoingToStaggerAndDie() const { return m_ActorFlags.m_bGoingToStaggerAndDie == 1; }
    void SetHasThrownPetrolBomb(signed char b) { m_ActorFlags.m_bHasThrownPetrolBomb = b != 0; }
    signed char IsHanging() const { return m_ActorFlags2.m_bHanging == 1; }
    void SetIsHanging(signed char b) { m_ActorFlags2.m_bHanging = b != 0; }
    int GetConsecutiveHitCounter() const { return m_ConsecutiveHitCounter; }
    signed char IsDestInSameNavZone() { return m_OrgLoc.NavZone() == m_DestLoc.NavZone(); }
    void ZeroMomentum() { m_Momentum.SetZero(); }
    void SetBlockCounter(int n) { m_BlockCounter = (float)n; }
    void SetHasHitPredRecently() { m_HitPredTimer = EXBaseApp::NominalGameFrameRate() * 5.0f; }
    float GetCombatTimeout() const { return (m_RefID % 24 * 0.25f + 6.0f) * EXBaseApp::NominalGameFrameRate(); }
    unsigned int GetStatus() { return m_Status; }
    unsigned int CreationFlagsGet() { return m_CreationFlags; }
    void SetBoundPOI(int poi) { m_BoundPOI = poi; }
    int BoundPOI() { return m_BoundPOI; }
    signed char IsMoving() const { return m_StateFlags.m_bMoving14 || m_StateFlags.m_bMoving15 || m_StateFlags.m_bMoving27; }
    int DefaultContextIcon() { return m_DefaultContextIcon; }
    void SetLastCityPOI(int poi) { m_LastCityPOI = poi; }
    int LastCityPOI() { return m_LastCityPOI; }
    void CreationFlagsAdd(unsigned int f) { m_CreationFlags |= f; }
    void SetAliveRadius(float r) { m_AliveRadius = r; }
    ActorTextureSwapItem *TextureSwapItem() { return m_pTextureSwapItem; }
    void SetTextureSwapItem(ActorTextureSwapItem *p) { m_pTextureSwapItem = p; }
    float GetAliveRadiusSq() { return m_AliveRadius * m_AliveRadius; }
    void SetHolographic() { m_ActorFlags.m_bHolographic = 1; }
    int GetAnimType() { return m_AnimType; }
    void JumpHitCeiling()
    {
        if (m_WorldVelocity.y > 0.0f) m_WorldVelocity.y = 0.0f;
    }
    EXVector GetGotoPos() { return m_DestLoc.Pos(); }
    EXVector GetNavLocation() { return m_OrgLoc.Pos(); }
    EXVector GetNavFloorPos() { return m_OrgLoc.FloorPos(); }
    int SeenBodyID() { return m_SeenBodyID; }
    signed char Overkilled() { return m_ActorFlags.m_bOverkilled == 1; }
    signed char CanDodge() { return m_DodgeTimer <= 0.0f; }
    void PerformDodge() { m_DodgeTimer = (float)EXBaseApp::NominalGameFrameRate(); }
    signed char HasCollided(int *pWith)
    {
        if (pWith) *pWith = m_CollidedWith;
        return m_ActorFlags.m_bHasCollided == 1;
    }
    void SetOnFire() { m_ActorFlags2.m_bOnFire = 1; }
    EXVector HitByVehiclePos() { return m_HitByVehiclePos; }
    signed char HitByCar() { return m_StateFlags.m_bHitByCar == 1; }
    void SetHitBy(int id) { m_HitBy = id; }
    void SetIsLyingOnGround(signed char b)
    {
        m_StateFlags.m_bLyingOnGround = b != 0;
        m_NotAllowedToGetUpTimer = 0.0f;
    }
    signed char NotAllowedToGetUp() const { return m_NotAllowedToGetUpTimer > 0.0f; }
    signed char IsLyingFaceDown() const { return m_StateFlags.m_bLyingFaceDown == 1; }
    void SetIsLyingFaceDown(signed char b) { m_StateFlags.m_bLyingFaceDown = b != 0; }
    void SetIsDazed(signed char b) { m_ActorFlags2.m_bDazed = b != 0; }
    void SetCanCollideWithPlayer(signed char b) { m_StateFlags.m_bCanCollideWithPlayer = b != 0; }
    void SetCanRipBossHead(signed char b) { m_StateFlags.m_bCanRipBossHead = b != 0; }
    signed char StartBossHeadRipped() const { return m_StateFlags.m_bStartBossHeadRipped == 1; }
    void SetStartBossHeadRipped(signed char b) { m_StateFlags.m_bStartBossHeadRipped = b != 0; }
    void SetIsBeingStealthKilled(signed char b) { m_StateFlags.m_bBeingStealthKilled = b != 0; }
    void SetCannotDie(signed char b) { m_ActorFlags.m_bCannotDie = b != 0; }
    void SetIsBlinded(signed char b) { m_ActorFlags.m_bBlinded = b != 0; }
    void SetGoingToStaggerAndDie(signed char b) { m_ActorFlags.m_bGoingToStaggerAndDie = b != 0; }
    void SetCanBeJuggled(signed char b) { m_ActorFlags.m_bCanBeJuggled = b != 0; }
    void SetBeingJuggled(signed char b) { m_ActorFlags.m_bBeingJuggled = b != 0; }
    void SetDontDragMeSpear(signed char b) { m_StateFlags.m_bDontDragMeSpear = b != 0; }
    void SetPlayerHolds(signed char b) { m_ActorFlags.m_bPlayerHolds = b != 0; }
    signed char IsInvinsible() const { return m_InvinsibleCounter > 0.0f; }
    unsigned int GetWatchFlags() { return m_WatchFlags; }
    unsigned int GetNudgeCount() const { return m_NudgeCount; }
    void IncNudgeCount() { m_NudgeCount++; }
    void SetEMPTimer(float t) { m_EMPTimer = t; }
    float GetCombatCircleStopTime() const { return m_CombatCircleStopTime; }
    float &AvoidLastSteer() { return m_AvoidLastSteer; }
    int GetMeetActorID() { return m_MeetActorID; }
    float GetMass() const { return m_Mass; }
    signed char DontDragMeSpear() const { return m_StateFlags.m_bDontDragMeSpear == 1; }
    signed char IsBlocking() const { return m_BlockCounter > 0.0f; }
    float GetCollisionSphereRadius() const { return m_CollisionSphereRadius; }
    int GetTargettedFlagChannel() const { return m_TargettedFlagChannel; }
    signed char IsSolid() { return m_Alpha == 1.0f; }
    signed char CouldPlayerBeSeen() { return m_ActorFlags2.m_bCouldPlayerBeSeen == 1; }
    signed char IsThreat() const { return GetProfile()->m_Honour > 0; }
    unsigned char GetRadarType() { return m_RadarType; }
    float CheckFiredBullet() { return m_FiredBulletTime; }
    int GetHitBy() { return m_HitBy; }
    int GetCreateID() { return m_CreateID; }
    void SetTargettedFlagChannel(int c) { m_TargettedFlagChannel = c; }
    void SetRotation(const EXVector &rot);
    static float GetMaxCollisionRadius() { return ms_fMaxCollisionRadius; }
    void SetNotAllowedToGetUp() { m_NotAllowedToGetUpTimer = EXBaseApp::NominalGameFrameRate() * 3.0f; }
    void SetHasBeenKicked(signed char b)
    {
        m_ActorFlags2.m_bHasBeenKicked = b != 0;
        m_ThrownPos = GetFeetPos();
    }
    float GetPlayerToActorColAdj() const { return m_PlayerToActorColAdj; }
    __forceinline signed char IsNegativeOrZeroThreat() const { return GetProfile()->m_Honour <= 0; }
    int GetAnimModeNext() { return m_AnimModeNext; }
    void SetAnimModeNext(int m)
    {
        m_AnimModeNext = m;
        m_AnimModePrev = m_AnimMode;
    }
    void ClrAnimMode() { m_AnimMode = 0; }
    unsigned int &AnimMotionOverride() { return m_AnimMotionOverride; }
    void SetNav3D(signed char b)
    {
        m_OrgLoc.Set3D(b);
        m_DestLoc.Set3D(b);
        m_NavRoute.Init(b, 0);
    }
    signed char IsValidRoute() const { return m_StateFlags.m_bValidRoute == 1; }

protected:
    void UpdateDistFade();
    void DeleteBloodScript(XItem *pItem, EXItemAnimator_Script *pScript);
    void UpdateBossTests();
    void UpdateVisionMode();
    void UpdateMovement2D();
    void UpdateMovement();
    void UpdateCarry();
    EXItemAnimator_Script *CreateBloodScript(int datum, int hash, XItem *pItem);
    void UpdateFlagChannel();
    void UpdateCheckFight();
    void UpdateShadow();
    void SetAnimMode(int mode, signed char b);
    void UpdateAnimType(int type);
    void UpdateHeadTrack();
    void UpdateCheckDeath();
    void UpdateContextIcon();
    void CheckSeePlayer();
    void UpdateSeeingPlayer();
    void UpdateSeeingBodies();
    void UpdateAlpha();
    void UpdateFallInOcean();
    void UpdateCloak();
    void UpdateMovement3D();

    static float ms_fActorAlpha;
    static float ms_fMaxCollisionRadius;
    static int m_Count;
    static unsigned int m_gAffiliation;
    static int m_MsgQueueCount;
    static int ms_iActorIDPlayerHasHead;
    static int gRagDollCount;
    static float gAlphaPulse;
    static XItem *ms_pLimbInPlayersHand;
    static QueuedMsgItem m_MsgQueue[128];

public:
    EXVector m_Unk08;                   // 0x008
    EXVector m_Unk18;                   // 0x018
    EXVector m_HeadTrackPos;            // 0x028
    EXVector m_Momentum;                // 0x038
    u8 m_Unk48[0x58 - 0x48];            // TODO layout
    EXVector m_StartPos;                // 0x058 position given to Create
    EXVector m_PelvisPosCache;          // 0x068 GetPelvisPos result, valid for frame m_PelvisPosFrame
    EXVector m_HeadPosCache;            // 0x078 GetHeadPos result, valid for frame m_HeadPosFrame
    EXVector m_ChestPosCache;           // 0x088 GetChestPos result, valid for frame m_ChestPosFrame
    EXVector m_AimingPos;               // 0x098 last SetAimingPos target
    EXVector m_ThrownPos;               // 0x0a8 feet position when thrown/kicked
    EXVector m_RandomDir;               // 0x0b8 TODO name: random unit XZ direction (DoUpdate)
    EXVector m_LocalVelocity;           // 0x0c8
    EXVector m_WorldVelocity;           // 0x0d8
    StateMachine m_SM;                  // 0x0e8 the brain
    ActorStack m_Stack[10];             // 0x188 pushed states
    LostLimbStruct m_LostLimbs[4];      // 0x688
    u32 m_CreationFlags;                // 0x768
    float m_AliveRadius;                // 0x76c
    int m_RefID;                        // 0x770 id; slot = id & 0x7f
    int m_CreateID;                     // 0x774
    ActorProfileIndex m_ProfileIndex;   // 0x778
    ActorSkinIndex m_SkinIndex;         // 0x77c
    ActorSkinIndex m_NewSkinIndex;      // 0x780 TODO
    int m_Unk784;
    int m_StackCount;                   // 0x788
    float m_Alpha2;                     // 0x78c TODO (SetAlpha, clamped 0..1)
    float m_TargetRotY;                 // 0x790
    int m_AnimMode;                     // 0x794
    int m_AnimType;                     // 0x798
    int m_AnimModePrev;                 // 0x79c
    int m_AnimModeNext;                 // 0x7a0
    unsigned int m_AnimMotionOverride;  // 0x7a4
    u32 m_Unk7a8;
    int m_CarryLeft;                    // 0x7ac ActorCarryable ids
    int m_CarryRight;                   // 0x7b0
    EXItemAnimator_Entity *m_pShadow;   // 0x7b4 drop shadow under the feet (UpdateShadow)
    EXItemAnimator_Script *m_pChangeSkinItem;  // 0x7b8 the new skin while it loads
    EXItemAnimator_Anim *m_pAnim;       // 0x7bc
    int m_MeetActorID;                  // 0x7c0
    float m_RagDollTime;                // 0x7c4 frames in rag doll
    float m_RagDollSpeed;               // 0x7c8 last frame's rag doll speed (DamageOtherActorsIfRagdoll)
    float m_Alpha;                      // 0x7cc
    float m_AlphaSpeed;                 // 0x7d0
    float m_HitCountdown;               // 0x7d4
    int m_BoundPOI;                     // 0x7d8
    float m_Health;                     // 0x7dc
    unsigned int m_Attrib;              // 0x7e0
    unsigned int m_Status;              // 0x7e4
    float m_LookAtPlayerTimer;          // 0x7e8 nudged: frames spent watching the player (negative: cool-down)
    unsigned int m_WatchFlags;          // 0x7ec
    int m_NonCriticalFrame;             // 0x7f0
    float m_SqDistFromPlayer;           // 0x7f4
    float m_SqDistFromCamera;           // 0x7f8
    float m_FadeDistance;               // 0x7fc
    int m_SeenBodyID;                   // 0x800
    EXDList m_EffectList;               // 0x804 ActorEffects (EXDListItem at +8)
    int m_TargettedFlagChannel;         // 0x80c
    int m_SubGroupAnimIndex;            // 0x810
    float m_ForceSightingTimer;         // 0x814
    int m_Unk818;                       // 0x818
    int m_Unk81c;                       // 0x81c
    int m_Unk820;
    int m_Unk824;
    int m_DefaultContextIcon;           // 0x828
    int m_Unk82c;                       // 0x82c
    int m_Unk830;                       // 0x830
    float m_UpdateTime;                 // 0x834 seconds updated (DoUpdate)
    float m_FiredBulletTime;            // 0x838
    EX6DAnim *m_pRagDoll;               // 0x83c
    EX6DAnim *m_pHeadPhysics;           // 0x840 ripped off head chain (body 0 held, 1 and 2 follow)
    int m_HitBy;                        // 0x844
    float m_Unk848f;                    // 0x848 TODO name: cloak refraction strength
    float m_CombatCircleStopTime;       // 0x84c
    float m_CollisionRadius;            // 0x850
    float m_CollisionSphereRadius;      // 0x854
    float m_BlockCounter;               // 0x858
    float m_InvinsibleCounter;          // 0x85c
    int m_ConsecutiveHitCounter;        // 0x860
    float m_Unk864;                     // 0x864 cos(1.7466)
    float m_Unk868;                     // 0x868 cos(0.8733)
    float m_Unk86c;                     // 0x86c 5
    float m_AnimSpeedRand;              // 0x870
    float m_AnimSpeedMult;              // 0x874
    float m_DodgeTimer;                 // 0x878
    ActorTextureSwapItem *m_pTextureSwapItem;  // 0x87c
    ActorOptions *m_pOptions;           // 0x880
    unsigned int m_PelvisPosFrame;      // 0x884
    unsigned int m_HeadPosFrame;        // 0x888
    unsigned int m_ChestPosFrame;       // 0x88c
    int m_NoticedActor;                 // 0x890 actor id the head tracks (NoticeActor)
    int m_HeadTrackMode;                // 0x894 (SetHeadTrackMode)
    float m_Unk898;                     // 0x898 TODO countdown (DoUpdate)
    int m_WalkAnim;                     // 0x89c civilian walk anim mode (0 = default)
    float m_AlertRaiseTimer;            // 0x8a0
    float m_AlertDecayTimer;            // 0x8a4 (UpdateAlarmMeter; -1 = idle)
    int m_AimingTarget;                 // 0x8a8
    float m_AimingTime;                 // 0x8ac
    float m_Mass;                       // 0x8b0
    float m_ActorToPlayerColAdj;        // 0x8b4
    float m_PlayerToActorColAdj;        // 0x8b8
    float m_NotAllowedToGetUpTimer;     // 0x8bc
    float m_SeenAlertTimer;             // 0x8c0 frames until the next MSG_SOMEONE_SEEN_PREDATOR shout
    int m_LastHitSFX;                   // 0x8c4
    float m_NudgeTimer;                 // 0x8c8 frames before the nudge state can clear (NoticePlayer)
    float m_HitPredTimer;               // 0x8cc
    float m_DazeMult;                   // 0x8d0 daze threshold (x profile health), halves per daze
    float m_Unk8d4[3];
    float m_Unk8e0[3];                  // 0x8e0 all 1
    float m_TargetRotYDelta;            // 0x8ec TODO (AtYRot)
    float m_TurnStartRotY;              // 0x8f0 facing when the turn animation started (SetTargetYRot)
    float m_Unk8f4;                     // 0x8f4 CanCollideWithPlayer
    float m_Unk8f8;                     // 0x8f8
    float m_CivilianAttackVOTimer;      // 0x8fc
    float m_EMPTimer;                   // 0x900
    float m_AvoidLastSteer;             // 0x904
    unsigned int m_NudgeCount;          // 0x908
    u16 m_Unk90c[3];                    // 0x90c
    u16 m_CollidedWith;                 // 0x912
    float m_CloakBreakTimer;            // 0x914
    float m_LastHitTimer;               // 0x918
    int m_ArmourIndex;                  // 0x91c ArmourProfile index
    float m_RandomDirTimer;             // 0x920 frames until m_RandomDir changes
    float m_FightDistTimer;             // 0x924
    Vehicle *m_pHitByVehicle;           // 0x928
    EXVector m_HitByVehiclePos;         // 0x92c
    ActorStateFlags m_StateFlags;       // 0x93c
    ActorFlags m_ActorFlags;            // 0x940
    ActorFlags2 m_ActorFlags2;          // 0x944
    ActorUnk948 m_Unk948;               // 0x948
    EXSoundTag m_SFXTag;                // 0x950
    EXSoundTag m_SpeechTag;             // 0x958
    XNavRoute m_NavRoute;               // 0x960 (+0x110 flags: bit 6 jumping disabled)
    XNavLocation m_OrgLoc;              // 0xa7c where the actor is
    XNavLocation m_DestLoc;             // 0xb04 where it is going
    EXExtRenderInfo m_ExtRenderInfo;    // 0xb8c
    EXExtRenderInfo m_ExtRenderInfo2;   // 0xc0c cloak (refraction)
    EXTexSwapInfo m_TexSwapInfo;        // 0xc8c
    unsigned char m_RadarType;          // 0xca0
    u8 m_Unkca1[3];
    ex::RGBA m_BrainScanColour;         // 0xca4
    int m_LastCityPOI;                  // 0xca8
    u32 m_ScanText;                     // 0xcac scan page 1 text
};

#endif
