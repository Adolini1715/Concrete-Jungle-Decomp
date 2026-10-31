#ifndef ACTORUTIL_H
#define ACTORUTIL_H
#include "types.h"
#include "EXVector.h"
#include "StateMachine.h"

// ActorUtil.obj: actor helpers (AU_*), player accessors, message senders and the shared AI states.
class Actor;
class EXGeoPath;
class EXItem;
class EXItemAnimator;
class EXItemAnimator_Anim;
class EXQuaternion;
class EXTrigger;
class WeaponProfile;
class XNavLocation;
struct EXDatumTemp;

// MSG_ATTACKED_BY_PREDATOR payload (sizeof 0x2c).
struct StructMsg_AttackedByPredator {
    StructMsg_AttackedByPredator() {}
    int m_Attack;           // 0x00 attack type
    EXVector m_Pos;         // 0x04
    EXVector m_Dir;         // 0x14
    float m_Damage;         // 0x24
    int m_Unk28;            // 0x28 TODO
};
// MSG_ATTACKED_BY_ACTOR payload (sizeof 0x30).
struct StructMsg_AttackedByActor {
    StructMsg_AttackedByActor() {}
    WeaponProfile *m_pWeapon;   // 0x00
    EXVector m_Pos;             // 0x04
    EXVector m_Dir;             // 0x14
    float m_Damage;             // 0x24
    s8 m_Unk28;                 // 0x28 TODO
    Actor *m_pAttacker;         // 0x2c
};
// MSG_INCOMING_ATTACK payload (sizeof 0x2c).
struct StructMsg_IncomingAttack {
    StructMsg_IncomingAttack() {}
    int m_Weapon;           // 0x00 weapon id
    int m_Unk4;             // 0x04 TODO
    EXVector m_Pos;         // 0x08
    EXVector m_Dir;         // 0x18
    float m_Unk28;          // 0x28 TODO
};

// Hit reaction picked by AR_ReactFromHit (the state it Gosubs). TODO names: read from the jump table.
enum KnockOver {
    KO_HITREACT_FLAG = 0,       // STATE_HitReact(dir, 1)
    KO_BARGE = 1,               // STATE_Barge
    KO_DAZED = 2,               // STATE_Dazed (ignored when already dazed)
    KO_ZOMBIEDAZED = 3,         // STATE_ZombieDazed (also skips the DontDoHitReaction/invincible tests)
    KO_ELECTRIC = 4,            // STATE_ElecticShock
    KO_HITREACT = 5,            // STATE_HitReact
    KO_JUGGLE_75 = 6,           // 3 in 4: STATE_HitReactKnockedOver when juggleable, else STATE_HitReact
    KO_JUGGLE_50 = 7,           // 1 in 2
    KO_JUGGLE_25 = 8,           // 1 in 4
    KO_JUGGLE = 9,              // always when juggleable
    KO_KNOCKBACKWARD = 10,      // STATE_KnockBackwardAndLive
    KO_NUDGE = 11,              // STATE_NudgeCloaked
    KO_KNOCKBACK_SERV = 12,     // STATE_KnockBackServ(dir, 1)
    KO_KNOCKBACK_RUSSIAN = 13,
    KO_KNOCKBACK_HUNTER1 = 14,
    KO_KNOCKBACK_MONSTERSQUAD = 15,
    KO_KNOCKBACK_ZOMBIE = 16,
    KO_MINE = 17,               // STATE_HitByUnknownMine
    KO_NONE = 18                // no reaction
};

signed char AU_IsOnScreen(EXItem *, EXVector const &, float);
signed char AU_ActorGroupWithinRange(int, int, EXVector const &, float);
EXGeoPath *AU_GetPathFromIndex(int);
void Player_DoDamage(float);
void SendMsg_LoudNoise(EXVector const &, float, Actor *, signed char, signed char);
void SendMsg_Attract(Actor *, EXVector const &);
void SendMsg_Repel(Actor *, EXVector const &);
void AU_AssignSkin(EXItemAnimator_Anim *, int, int);
signed char AU_AssignSkinSkelAnim(EXItemAnimator_Anim *, int, int, int, int, int, int);
int AU_SpawnActor(int, int, int, unsigned int);
void AU_DrawCross(EXVector const &, float, int);
int AU_NavPolyForPos(EXVector const &);
EXVector AU_SnapFloorPos(EXVector const &, int *, signed char);
EXVector AU_SnapRelativeFloorPos(EXVector const &, EXVector const &, int *);
EXVector AU_SnapRelativeToLoc(XNavLocation &, EXVector const &, int *);
EXVector Player_GetFeetPos();
EXVector Player_GetPelvisPos();
EXVector Player_GetHeadPos();
EXVector Player_GetRot();
signed char Player_IsCloaked();
int Player_GetVisionMode();
signed char AU_VecAngleInside(EXVector const &, EXVector const &, EXVector const &, float);
signed char AU_HandleMuggingAttacks(StateMachine &);
signed char AU_DoesAttackCreateLoudNoise(StructMsg_AttackedByPredator *);
signed char AU_DoesAttackOverrideCloak(StructMsg_AttackedByPredator *);
signed char AU_DoesAttackGiveActorSight(StateMachine &, StructMsg_AttackedByPredator *);
void AU_AdjustDamageForArmour(StateMachine &, StructMsg_AttackedByPredator *);
void AU_PlayerForceSightingInArea(EXVector const &, float);
signed char AU_ShouldSearchForPredator();
signed char AU_HandleLoudNoise(StateMachine &);
signed char AU_PlayerObstructs(StateMachine &, EXVector, float);
signed char STATE_KnockBackwardAndLive(StateMachine &);
signed char STATE_HitReactLyingOnGround(StateMachine &);
signed char STATE_HitReactKnockedOver(StateMachine &);
signed char STATE_Barge(StateMachine &);
signed char STATE_Dazed(StateMachine &);
signed char STATE_Special_OldBloke(StateMachine &);
signed char STATE_DeathBoss(StateMachine &);
signed char STATE_DeleteNow(StateMachine &);
signed char STATE_FS_Delete(StateMachine &);
signed char STATE_FS_Kill(StateMachine &);
signed char STATE_FS_StandStill(StateMachine &);
signed char STATE_FS_ChangeSkin(StateMachine &);
signed char STATE_ActorInit(StateMachine &);
signed char AU_HandleAttractNoise(StateMachine &);
signed char AU_HandleFightMe(StateMachine &);
signed char AU_HandleDriveBy(StateMachine &);
signed char STATE_AttackPlayerForever(StateMachine &);
signed char STATE_TestAnim(StateMachine &);
signed char AU_SetItemDraw(EXItem *, signed char);
signed char AU_SetAnimatorDraw(EXItemAnimator *, signed char);
signed char AU_GetItemDraw(EXItem *);
void AU_SetItemARGB(EXItem *, float, float, float, float);
void AU_SetAnimatorARGB_Body(EXItemAnimator *, float, float, float, float);
void AU_SetAnimatorARGB_Limb(EXItemAnimator *, float, float, float, float);
void AU_DeleteItem(EXItem *);
EXVector AU_GetPosAtDistFromPlayer(float, int *);
int AU_FindClosestPathIndex(EXVector const &, int &, unsigned int);
void SendMsg_AttackedByPredator(Actor *, int, EXVector const &, EXVector const &, float, int);
void SendMsg_AttackedByActor(Actor *, WeaponProfile *, EXVector const &, EXVector const &, float, Actor *, signed char);
void SendMsg_IncomingAttack(Actor *, int, int, EXVector const &, EXVector const &, float);
int AU_SpawnActor(EXTrigger *, int);
int AU_SpawnActorNearPlayer(int, int);
signed char AU_IsNavMeshValid(EXVector const &);
int AU_GetNavZone(int);
EXQuaternion AU_InterpQuat(EXQuaternion &, EXQuaternion &, float);
EXVector AU_EnsurePointOutOfView(EXVector &, EXVector &);
EXDatumTemp Player_GetHandDatum();
float AU_GetFloorHeight(EXVector const &);
signed char STATE_MonsterJump(StateMachine &);
signed char STATE_ElecticShock(StateMachine &);
signed char STATE_NotReallyDead(StateMachine &);
signed char STATE_FadeOutDead(StateMachine &);
signed char STATE_PickUpDroppedItem(StateMachine &);
signed char STATE_GotoLocationQueue(StateMachine &);
signed char STATE_MonsterGotoLocation(StateMachine &);
void SendMsg_HealingRoar(EXVector const &, float);
void AU_BlockLoadGeoSubSection(int, int);
void AU_BlockDeloadGeoSubSection(int, int);
void *AU_GetWorkSheet(int, int, int, int &);
signed char STATE_RagDollDeath(StateMachine &);
signed char STATE_AnimDeath(StateMachine &);
signed char STATE_DeathBullet(StateMachine &);
signed char STATE_DeathJuggleSlice(StateMachine &);
signed char STATE_GotoLocation(StateMachine &);
signed char STATE_MonsterTest(StateMachine &);
signed char AR_ReleaseFromPredator(StateMachine &);
signed char AU_HandleIncomingAttack(StateMachine &, StructMsg_IncomingAttack *);
signed char STATE_KnockBackwardAndDie(StateMachine &);
signed char STATE_HitReact(StateMachine &);
signed char STATE_HeavyFinish(StateMachine &);
signed char STATE_StealthDeathInit(StateMachine &);
signed char STATE_StealthDeath(StateMachine &);
signed char STATE_PlayerControlDeath(StateMachine &);
signed char AU_HandlePlayerAttacks_Ulysses(StateMachine &, StructMsg_AttackedByPredator *);
signed char AU_HandlePlayerAttacks_Boss(StateMachine &, StructMsg_AttackedByPredator *);
signed char AU_HandleActorAttacks(StateMachine &, StructMsg_AttackedByActor *);
signed char STATE_FS_LyingDownInstant(StateMachine &);
int AU_FindRunAwayPoint(StateMachine &, int, int, int, signed char);
int AU_FindHealPoint(StateMachine &, int, int, signed char);
signed char STATE_SearchForPredator(StateMachine &);
signed char AU_HandleSomeoneUnderAttack(StateMachine &);
signed char STATE_RagdollJump(StateMachine &);
signed char STATE_BlazinSquad(StateMachine &);
signed char AU_HandlePlayerAttacks(StateMachine &, StructMsg_AttackedByPredator *);
signed char STATE_HitByVehicle(StateMachine &);
signed char STATE_NudgeCloaked(StateMachine &);
signed char STATE_QuickEvadePlayer(StateMachine &);
signed char STATE_BurnTheDanceFloor(StateMachine &);
signed char STATE_ElectroSquad(StateMachine &);
signed char STATE_RagDollGrabbed(StateMachine &);
signed char AU_HandleSomeoneSeenPredator(StateMachine &);
signed char STATE_FindPred(StateMachine &);
signed char STATE_EvadePlayer(StateMachine &);
signed char STATE_BOSS_RatPredEvadePlayer(StateMachine &);
signed char STATE_FindHeal(StateMachine &);
signed char STATE_PanicExtreme(StateMachine &);
signed char STATE_FS_BangOnDoor(StateMachine &);
signed char STATE_PanicBlind(StateMachine &);
signed char STATE_SeenBody_Panic(StateMachine &);
signed char STATE_FS_Cower(StateMachine &);
signed char STATE_FS_Pray(StateMachine &);
signed char STATE_FS_Pray_Ignore(StateMachine &);
signed char STATE_CustomerQueue(StateMachine &);
signed char STATE_StandStill(StateMachine &);
signed char STATE_GotoLocationNearestType(StateMachine &);
signed char STATE_GotoLocationNearestFlag(StateMachine &);
signed char STATE_CrimePrepare(StateMachine &);
signed char STATE_CrimePrepareAccomplice(StateMachine &);
signed char STATE_GotoPosition(StateMachine &);
signed char STATE_POIForceState(StateMachine &);
signed char STATE_MissionEnd(StateMachine &);
signed char STATE_FreeFall(StateMachine &);
signed char STATE_RunToDestination(StateMachine &);
signed char STATE_RunToDestination1(StateMachine &);
signed char STATE_RunToDestination2(StateMachine &);
signed char STATE_RunToDestination3(StateMachine &);
signed char STATE_RunToDestination4(StateMachine &);
signed char STATE_RunToDestination5(StateMachine &);
signed char STATE_RunToDestination6(StateMachine &);
signed char STATE_RunToDestination7(StateMachine &);
signed char STATE_RunToDestination8(StateMachine &);
signed char STATE_RunToDestination9(StateMachine &);
signed char STATE_RunToDestination10(StateMachine &);
signed char STATE_StandAttractedToPlayer(StateMachine &);
signed char STATE_WalkAttractedToPlayer(StateMachine &);
signed char STATE_CryForever(StateMachine &);
signed char STATE_PanicStand(StateMachine &);
signed char STATE_PanicStandForever(StateMachine &);
signed char STATE_RunDestSnipeStand(StateMachine &);
signed char STATE_RunDestSnipeStand1(StateMachine &);
signed char STATE_RunDestSnipeStand2(StateMachine &);
signed char STATE_RunDestSnipeStand3(StateMachine &);
signed char STATE_RunDestSnipeStand4(StateMachine &);
signed char STATE_RunDestSnipeStand5(StateMachine &);
signed char STATE_RunDestSnipeStand6(StateMachine &);
signed char STATE_RunDestSnipeStand7(StateMachine &);
signed char STATE_RunDestSnipeStand8(StateMachine &);
signed char STATE_RunDestSnipeStand9(StateMachine &);
signed char STATE_RunDestSnipeStand10(StateMachine &);
signed char STATE_RunDestSnipeStand11(StateMachine &);
signed char STATE_RunDestSnipeStand12(StateMachine &);
signed char STATE_RunDestSnipeStand13(StateMachine &);
signed char STATE_RunDestSnipeStand14(StateMachine &);
signed char STATE_RunDestSnipeStand15(StateMachine &);
signed char STATE_RunDestSnipeStand16(StateMachine &);
signed char STATE_RunDestSnipeStand17(StateMachine &);
signed char STATE_RunDestSnipeStand18(StateMachine &);
signed char STATE_RunDestSnipeStand19(StateMachine &);
signed char STATE_RunDestSnipeStand20(StateMachine &);
signed char STATE_RunDestSnipeStand21(StateMachine &);
signed char STATE_RunDestSnipeStand22(StateMachine &);
signed char STATE_RunDestSnipeStand23(StateMachine &);
signed char STATE_RunDestSnipeStand24(StateMachine &);
signed char STATE_RunDestSnipeStand25(StateMachine &);
signed char STATE_RunDestSnipeMove(StateMachine &);
signed char STATE_RunDestSnipeMove1(StateMachine &);
signed char STATE_RunDestSnipeMove2(StateMachine &);
signed char STATE_RunDestSnipeMove3(StateMachine &);
signed char STATE_RunDestSnipeMove4(StateMachine &);
signed char STATE_RunDestSnipeMove5(StateMachine &);
signed char STATE_RunDestSnipeMove6(StateMachine &);
signed char STATE_RunDestSnipeMove7(StateMachine &);
signed char STATE_RunDestSnipeMove8(StateMachine &);
signed char STATE_RunDestSnipeMove9(StateMachine &);
signed char STATE_RunDestSnipeMove10(StateMachine &);
signed char STATE_RunDestSnipeMove11(StateMachine &);
signed char STATE_RunDestSnipeMove12(StateMachine &);
signed char STATE_RunDestSnipeMove13(StateMachine &);
signed char STATE_RunDestSnipeMove14(StateMachine &);
signed char STATE_RunDestSnipeMove15(StateMachine &);
signed char STATE_RunDestSnipeMove16(StateMachine &);
signed char STATE_RunDestSnipeMove17(StateMachine &);
signed char STATE_RunDestSnipeMove18(StateMachine &);
signed char STATE_RunDestSnipeMove19(StateMachine &);
signed char STATE_RunDestSnipeMove20(StateMachine &);
signed char STATE_RunDestSnipeMove21(StateMachine &);
signed char STATE_RunDestSnipeMove22(StateMachine &);
signed char STATE_RunDestSnipeMove23(StateMachine &);
signed char STATE_RunDestSnipeMove24(StateMachine &);
signed char STATE_RunDestSnipeMove25(StateMachine &);
signed char STATE_RunDestCower(StateMachine &);
signed char STATE_RunDestCower1(StateMachine &);
signed char STATE_RunDestCower2(StateMachine &);
signed char STATE_RunDestDelete(StateMachine &);
signed char STATE_RunDestDelete1(StateMachine &);
signed char STATE_RunDestDelete2(StateMachine &);
signed char STATE_RunDestDelete3(StateMachine &);
signed char STATE_RunDestDelete4(StateMachine &);
signed char STATE_RunDestDelete5(StateMachine &);
signed char STATE_RunDestDelete6(StateMachine &);
signed char STATE_RunDestDelete7(StateMachine &);
signed char STATE_RunDestDelete8(StateMachine &);
signed char STATE_RunDestDelete9(StateMachine &);
signed char STATE_RunDestDelete10(StateMachine &);
signed char STATE_WalkToLocation(StateMachine &);
signed char STATE_WalkToProstitution(StateMachine &);
signed char STATE_WalkToLocationIgnore(StateMachine &);
signed char STATE_RunToLocation(StateMachine &);
signed char STATE_RunToLocationIgnore(StateMachine &);
signed char STATE_WalkToLocationCarry(StateMachine &);
signed char STATE_WalkToLocation1(StateMachine &);
signed char STATE_WalkToLocation2(StateMachine &);
signed char STATE_WalkToLocation3(StateMachine &);
signed char STATE_WalkToLocation4(StateMachine &);
signed char STATE_WalkToLocation5(StateMachine &);
signed char STATE_WalkToLocation6(StateMachine &);
signed char STATE_WalkToLocation7(StateMachine &);
signed char STATE_WalkToLocation8(StateMachine &);
signed char STATE_WalkToLocation9(StateMachine &);
signed char STATE_WalkToLocation10(StateMachine &);
signed char STATE_WalkToLocation11(StateMachine &);
signed char STATE_WalkToLocation12(StateMachine &);
signed char STATE_WalkToLocation13(StateMachine &);
signed char STATE_WalkToLocation14(StateMachine &);
signed char STATE_WalkToLocation15(StateMachine &);
signed char STATE_WalkToLocation16(StateMachine &);
signed char STATE_WalkToLocation17(StateMachine &);
signed char STATE_WalkToLocation18(StateMachine &);
signed char STATE_WalkToLocation19(StateMachine &);
signed char STATE_WalkToLocation20(StateMachine &);
signed char STATE_WalkToLocationIgnore1(StateMachine &);
signed char STATE_WalkToLocationIgnore2(StateMachine &);
signed char STATE_WalkToLocationIgnore3(StateMachine &);
signed char STATE_WalkToLocationIgnore4(StateMachine &);
signed char STATE_WalkToLocationIgnore5(StateMachine &);
signed char STATE_WalkToLocationIgnore6(StateMachine &);
signed char STATE_WalkToLocationIgnore7(StateMachine &);
signed char STATE_WalkToLocationIgnore8(StateMachine &);
signed char STATE_WalkToLocationIgnore9(StateMachine &);
signed char STATE_WalkToLocationIgnore10(StateMachine &);
signed char STATE_WalkToLocationIgnore11(StateMachine &);
signed char STATE_WalkToLocationIgnore12(StateMachine &);
signed char STATE_WalkToLocationIgnore13(StateMachine &);
signed char STATE_WalkToLocationIgnore14(StateMachine &);
signed char STATE_WalkToLocationIgnore15(StateMachine &);
signed char STATE_WalkToLocationIgnore16(StateMachine &);
signed char STATE_WalkToLocationIgnore17(StateMachine &);
signed char STATE_WalkToLocationIgnore18(StateMachine &);
signed char STATE_WalkToLocationIgnore19(StateMachine &);
signed char STATE_WalkToLocationIgnore20(StateMachine &);
signed char STATE_RunToLocation1(StateMachine &);
signed char STATE_RunToLocation2(StateMachine &);
signed char STATE_RunToLocation3(StateMachine &);
signed char STATE_RunToLocation4(StateMachine &);
signed char STATE_RunToLocation5(StateMachine &);
signed char STATE_RunToLocation6(StateMachine &);
signed char STATE_RunToLocation7(StateMachine &);
signed char STATE_RunToLocation8(StateMachine &);
signed char STATE_RunToLocation9(StateMachine &);
signed char STATE_RunToLocation10(StateMachine &);
signed char STATE_RunToLocation11(StateMachine &);
signed char STATE_RunToLocation12(StateMachine &);
signed char STATE_RunToLocation13(StateMachine &);
signed char STATE_RunToLocation14(StateMachine &);
signed char STATE_RunToLocation15(StateMachine &);
signed char STATE_RunToLocation16(StateMachine &);
signed char STATE_RunToLocation17(StateMachine &);
signed char STATE_RunToLocation18(StateMachine &);
signed char STATE_RunToLocation19(StateMachine &);
signed char STATE_RunToLocation20(StateMachine &);
signed char STATE_RunToLocationIgnore1(StateMachine &);
signed char STATE_RunToLocationIgnore2(StateMachine &);
signed char STATE_RunToLocationIgnore3(StateMachine &);
signed char STATE_RunToLocationIgnore4(StateMachine &);
signed char STATE_RunToLocationIgnore5(StateMachine &);
signed char STATE_RunToLocationIgnore6(StateMachine &);
signed char STATE_RunToLocationIgnore7(StateMachine &);
signed char STATE_RunToLocationIgnore8(StateMachine &);
signed char STATE_RunToLocationIgnore9(StateMachine &);
signed char STATE_RunToLocationIgnore10(StateMachine &);
signed char STATE_RunToLocationIgnore11(StateMachine &);
signed char STATE_RunToLocationIgnore12(StateMachine &);
signed char STATE_RunToLocationIgnore13(StateMachine &);
signed char STATE_RunToLocationIgnore14(StateMachine &);
signed char STATE_RunToLocationIgnore15(StateMachine &);
signed char STATE_RunToLocationIgnore16(StateMachine &);
signed char STATE_RunToLocationIgnore17(StateMachine &);
signed char STATE_RunToLocationIgnore18(StateMachine &);
signed char STATE_RunToLocationIgnore19(StateMachine &);
signed char STATE_RunToLocationIgnore20(StateMachine &);
signed char STATE_WalkToLocationCarry1(StateMachine &);
signed char STATE_WalkToLocationCarry2(StateMachine &);
signed char STATE_WalkToLocationCarry3(StateMachine &);
signed char STATE_WalkToLocationCarry4(StateMachine &);
signed char STATE_WalkToLocationCarry5(StateMachine &);
signed char STATE_WalkToLocationCarry6(StateMachine &);
signed char STATE_WalkToLocationCarry7(StateMachine &);
signed char STATE_WalkToLocationCarry8(StateMachine &);
signed char STATE_WalkToLocationCarry9(StateMachine &);
signed char STATE_WalkToLocationCarry10(StateMachine &);
signed char STATE_WalkToLocationCarry11(StateMachine &);
signed char STATE_WalkToLocationCarry12(StateMachine &);
signed char STATE_WalkToLocationCarry13(StateMachine &);
signed char STATE_WalkToLocationCarry14(StateMachine &);
signed char STATE_WalkToLocationCarry15(StateMachine &);
signed char STATE_WalkToLocationCarry16(StateMachine &);
signed char STATE_WalkToLocationCarry17(StateMachine &);
signed char STATE_WalkToLocationCarry18(StateMachine &);
signed char STATE_WalkToLocationCarry19(StateMachine &);
signed char STATE_WalkToLocationCarry20(StateMachine &);
signed char STATE_Police_ApprehendActor(StateMachine &);
void AU_SpawnDebugActor(int);
signed char AU_EnsureAtTarget(StateMachine &, EXVector, float);
signed char STATE_Mosh(StateMachine &);
signed char STATE_AttackPlayerOnSight(StateMachine &);
signed char STATE_DefaultIdle(StateMachine &);
signed char STATE_PlayStartAnim(StateMachine &);
signed char STATE_PanicDropWeapon(StateMachine &);
signed char STATE_Panic(StateMachine &);
signed char STATE_SeenBody_Help(StateMachine &);
signed char STATE_FS_PanicStand(StateMachine &);
signed char STATE_Police_Patrol(StateMachine &);
signed char STATE_Police_PatrolAccomplice(StateMachine &);
signed char STATE_RobberyVictim(StateMachine &);
signed char STATE_ExtortionVictim(StateMachine &);

// ---- State-function helpers (the YACAAV states: signed char STATE_x(StateMachine &sm)) ----
// A state switches on sm.m_Msg.m_Type; MSG_ENTER etc. are the lifecycle messages. Gosub/Goto take the
// current message's watch bits.
#define SM_GOTO(state, a, b, c, d) sm.Goto(GetMsgBitsForMsg(sm.m_Msg.m_Type), state, a, b, c, d)
#define SM_GOSUB(state, a, b, c, d) sm.Gosub(GetMsgBitsForMsg(sm.m_Msg.m_Type), state, a, b, c, d)

// Standard message cases. Retail's return-1 epilogue sharing pins the shapes: the payload messages
// test with &&, the others return early when the message is already on the stack. VC jumps to the
// LAST return-1 of a kind in source order, so ATTACKED_BY_PREDATOR goes last and SEEN_PREDATOR
// second last (see AU_DEFAULT_MSGS).
#define AU_MSG_ATTACKED_BY_PREDATOR \
    case MSG_ATTACKED_BY_PREDATOR: \
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR) && !sm.GetActorOwner().Holographic()) \
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]); \
        return 1;
#define AU_MSG_ATTACKED_BY_ACTOR \
    case MSG_ATTACKED_BY_ACTOR: \
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR) && !sm.GetActorOwner().Holographic()) \
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]); \
        return 1;
#define AU_MSG_INCOMING_ATTACK \
    case MSG_INCOMING_ATTACK: \
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_INCOMING_ATTACK) && !sm.GetActorOwner().Holographic()) \
            AU_HandleIncomingAttack(sm, (StructMsg_IncomingAttack *)sm.m_Msg.m_Data[0]); \
        return 1;
#define AU_MSG_SIMPLE(msg, fn) \
    case msg: \
        if (sm.GetActorOwner().IsMsgOnStack(msg)) return 1; \
        fn(sm); \
        return 1;
#define AU_MSG_SIMPLE_NOHOLO(msg, fn) \
    case msg: \
        if (sm.GetActorOwner().IsMsgOnStack(msg)) return 1; \
        if (!sm.GetActorOwner().Holographic()) fn(sm); \
        return 1;
#define AU_MSG_MUGGING AU_MSG_SIMPLE(MSG_MUGGING, AU_HandleMuggingAttacks)
#define AU_MSG_SOMEONE_UNDER_ATTACK AU_MSG_SIMPLE(MSG_SOMEONE_UNDER_ATTACK, AU_HandleSomeoneUnderAttack)
#define AU_MSG_FIGHT_ME AU_MSG_SIMPLE(MSG_FIGHT_ME, AU_HandleFightMe)
#define AU_MSG_DRIVE_BY AU_MSG_SIMPLE(MSG_DRIVE_BY, AU_HandleDriveBy)
#define AU_MSG_SOMEONE_SEEN_PREDATOR AU_MSG_SIMPLE_NOHOLO(MSG_SOMEONE_SEEN_PREDATOR, AU_HandleSomeoneSeenPredator)
#define AU_MSG_LOUD_NOISE AU_MSG_SIMPLE_NOHOLO(MSG_LOUD_NOISE, AU_HandleLoudNoise)
#define AU_MSG_ATTRACT AU_MSG_SIMPLE_NOHOLO(MSG_ATTRACT, AU_HandleAttractNoise)

// The standard set (jump-table cases 0xe, 0xd, 0x18, 0x27, 0x1d, 0x16 in this source order).
#define AU_DEFAULT_MSGS \
    AU_MSG_MUGGING AU_MSG_SOMEONE_UNDER_ATTACK AU_MSG_ATTACKED_BY_ACTOR AU_MSG_LOUD_NOISE AU_MSG_FIGHT_ME \
    AU_MSG_DRIVE_BY AU_MSG_INCOMING_ATTACK AU_MSG_ATTRACT AU_MSG_SOMEONE_SEEN_PREDATOR AU_MSG_ATTACKED_BY_PREDATOR

#endif
