#include "ActorProfile.h"
#include "Actor.h"
#include "ActorUtil.h"
#include "EXHashCodeFile.h"
#include "EXRand.h"
#include "EXTrace.h"
#include "EXApp.h"
#include "FEText.h"

// TODO stand-ins (ActorUtil.obj, ActorLocation.obj: not written yet).
extern unsigned int gCurrentMap_GeoHash;
int POI_GetNearestType(const EXVector &pos, int type, int last, int zone, signed char a, signed char b);
int POI_GetNearestFlag(const EXVector &pos, int flag, int last, int zone, signed char a, signed char b, float r);
struct FSPOIType {
    int m_Type;         // POI type (0: use m_Flag)
    int m_Flag;
    int m_Unk8;
    int m_Unkc;
};
struct FSTable {        // TODO stand-in layout (ActorLocation.obj)
    u8 m_Unk0[0xa8];
    FSPOIType m_POITypes[30];   // 0xa8
    s8 m_Fight[12][12];         // 0x288 [group b][group a]
    u8 m_Unk318[8];
};
extern FSTable FSAttribs[];

signed char STATE_AL_Chat(StateMachine &sm);
signed char STATE_AL_Courting(StateMachine &sm);
signed char STATE_AL_Gamble(StateMachine &sm);
signed char STATE_AL_Idle(StateMachine &sm);
signed char STATE_AL_Loiter(StateMachine &sm);
signed char STATE_AL_Piss(StateMachine &sm);
signed char STATE_AL_SleepDoorway(StateMachine &sm);
signed char STATE_AL_WindowShop(StateMachine &sm);
signed char STATE_Alien_Attack(StateMachine &sm);
signed char STATE_Alien_Attack2(StateMachine &sm);
signed char STATE_Alien_Attack3(StateMachine &sm);
signed char STATE_Alien_Attack4(StateMachine &sm);
signed char STATE_Alien_Attack5(StateMachine &sm);
signed char STATE_Alien_CeilingHang(StateMachine &sm);
signed char STATE_Alien_CeilingHang2(StateMachine &sm);
signed char STATE_Alien_CeilingHang3(StateMachine &sm);
signed char STATE_Alien_HoleDweller(StateMachine &sm);
signed char STATE_Alien_HoleDweller2(StateMachine &sm);
signed char STATE_Alien_HoleDweller3(StateMachine &sm);
signed char STATE_AttackPlayerForever(StateMachine &sm);
signed char STATE_AttackPlayerOnSight(StateMachine &sm);
signed char STATE_AttackPred(StateMachine &sm);
signed char STATE_AttackPredBruno_UseLoc(StateMachine &sm);
signed char STATE_BOSS_RatPredEvadePlayer(StateMachine &sm);
signed char STATE_BlazinSquad(StateMachine &sm);
signed char STATE_BlendAnimTest(StateMachine &sm);
signed char STATE_Bouncer(StateMachine &sm);
signed char STATE_BrutePredAttack(StateMachine &sm);
signed char STATE_Brute_FleeToLoc18(StateMachine &sm);
signed char STATE_BurnTheDanceFloor(StateMachine &sm);
signed char STATE_CaptivePredHanging(StateMachine &sm);
signed char STATE_CaptivePredRelease(StateMachine &sm);
signed char STATE_ChangeSkinThenAttackPred(StateMachine &sm);
signed char STATE_ChangeWeaponMode(StateMachine &sm);
signed char STATE_ChargeAttack(StateMachine &sm);
signed char STATE_CityDistractions(StateMachine &sm);
signed char STATE_CryForever(StateMachine &sm);
signed char STATE_DanceGirl(StateMachine &sm);
signed char STATE_DeathBullet(StateMachine &sm);
signed char STATE_DeathMetal(StateMachine &sm);
signed char STATE_DefaultIdle(StateMachine &sm);
signed char STATE_DeleteNow(StateMachine &sm);
signed char STATE_Distraction_Path_Guard(StateMachine &sm);
signed char STATE_Distraction_Path_Runner(StateMachine &sm);
signed char STATE_Distraction_Stage(StateMachine &sm);
signed char STATE_DropMine(StateMachine &sm);
signed char STATE_ElecticShock(StateMachine &sm);
signed char STATE_ElectroSquad(StateMachine &sm);
signed char STATE_EvadePlayer(StateMachine &sm);
signed char STATE_FS_ATM(StateMachine &sm);
signed char STATE_FS_BangOnDoor(StateMachine &sm);
signed char STATE_FS_BankRobbery(StateMachine &sm);
signed char STATE_FS_BankRobberyAccomplice(StateMachine &sm);
signed char STATE_FS_BreakAndEnter(StateMachine &sm);
signed char STATE_FS_BreakAndEnterAccomplice(StateMachine &sm);
signed char STATE_FS_BuyDrugs(StateMachine &sm);
signed char STATE_FS_BuyHooker(StateMachine &sm);
signed char STATE_FS_BuyHookerAlt(StateMachine &sm);
signed char STATE_FS_BuyVendor(StateMachine &sm);
signed char STATE_FS_ChangeSkin(StateMachine &sm);
signed char STATE_FS_Chat(StateMachine &sm);
signed char STATE_FS_CheckpointCharlie(StateMachine &sm);
signed char STATE_FS_Courting(StateMachine &sm);
signed char STATE_FS_Cower(StateMachine &sm);
signed char STATE_FS_CrowdCircle(StateMachine &sm);
signed char STATE_FS_Delete(StateMachine &sm);
signed char STATE_FS_DriveBy(StateMachine &sm);
signed char STATE_FS_DrugDealer(StateMachine &sm);
signed char STATE_FS_Extortion(StateMachine &sm);
signed char STATE_FS_ExtortionAccomplice(StateMachine &sm);
signed char STATE_FS_FactoryWorker(StateMachine &sm);
signed char STATE_FS_FightClub(StateMachine &sm);
signed char STATE_FS_Gamble(StateMachine &sm);
signed char STATE_FS_GoDrinking(StateMachine &sm);
signed char STATE_FS_GoShopping(StateMachine &sm);
signed char STATE_FS_Graffiti(StateMachine &sm);
signed char STATE_FS_Hooker(StateMachine &sm);
signed char STATE_FS_HookerAlt(StateMachine &sm);
signed char STATE_FS_Hustler(StateMachine &sm);
signed char STATE_FS_IdlingProstitutes(StateMachine &sm);
signed char STATE_FS_Kill(StateMachine &sm);
signed char STATE_FS_Loiter(StateMachine &sm);
signed char STATE_FS_LyingDownInstant(StateMachine &sm);
signed char STATE_FS_MimicryMe(StateMachine &sm);
signed char STATE_FS_Mugger(StateMachine &sm);
signed char STATE_FS_OpenSesame(StateMachine &sm);
signed char STATE_FS_PanicStand(StateMachine &sm);
signed char STATE_FS_Pimp(StateMachine &sm);
signed char STATE_FS_Piss(StateMachine &sm);
signed char STATE_FS_Pray(StateMachine &sm);
signed char STATE_FS_Pray_Ignore(StateMachine &sm);
signed char STATE_FS_Reprimand(StateMachine &sm);
signed char STATE_FS_ShadyDeal(StateMachine &sm);
signed char STATE_FS_SleepDoorway(StateMachine &sm);
signed char STATE_FS_SmokingBreak(StateMachine &sm);
signed char STATE_FS_StoreRobbery(StateMachine &sm);
signed char STATE_FS_StoreRobberyAccomplice(StateMachine &sm);
signed char STATE_FS_ThrowGrenadeForwards(StateMachine &sm);
signed char STATE_FS_TrampGather(StateMachine &sm);
signed char STATE_FS_Trashcan(StateMachine &sm);
signed char STATE_FS_Vandalism(StateMachine &sm);
signed char STATE_FS_VandalismAccomplice(StateMachine &sm);
signed char STATE_FS_VendingMachine(StateMachine &sm);
signed char STATE_FS_Vendor(StateMachine &sm);
signed char STATE_FS_WindowShop(StateMachine &sm);
signed char STATE_FindHeal(StateMachine &sm);
signed char STATE_GotoLocationViaEvadePoints1(StateMachine &sm);
signed char STATE_GotoLocationViaEvadePoints10(StateMachine &sm);
signed char STATE_GotoLocationViaEvadePoints11(StateMachine &sm);
signed char STATE_GotoLocationViaEvadePoints12(StateMachine &sm);
signed char STATE_GotoLocationViaEvadePoints13(StateMachine &sm);
signed char STATE_GotoLocationViaEvadePoints14(StateMachine &sm);
signed char STATE_GotoLocationViaEvadePoints15(StateMachine &sm);
signed char STATE_GotoLocationViaEvadePoints16(StateMachine &sm);
signed char STATE_GotoLocationViaEvadePoints17(StateMachine &sm);
signed char STATE_GotoLocationViaEvadePoints18(StateMachine &sm);
signed char STATE_GotoLocationViaEvadePoints19(StateMachine &sm);
signed char STATE_GotoLocationViaEvadePoints2(StateMachine &sm);
signed char STATE_GotoLocationViaEvadePoints20(StateMachine &sm);
signed char STATE_GotoLocationViaEvadePoints3(StateMachine &sm);
signed char STATE_GotoLocationViaEvadePoints4(StateMachine &sm);
signed char STATE_GotoLocationViaEvadePoints5(StateMachine &sm);
signed char STATE_GotoLocationViaEvadePoints6(StateMachine &sm);
signed char STATE_GotoLocationViaEvadePoints7(StateMachine &sm);
signed char STATE_GotoLocationViaEvadePoints8(StateMachine &sm);
signed char STATE_GotoLocationViaEvadePoints9(StateMachine &sm);
signed char STATE_HerIndoors(StateMachine &sm);
signed char STATE_HoldItemTest(StateMachine &sm);
signed char STATE_Hunter1_PredAttack(StateMachine &sm);
signed char STATE_Hunter2(StateMachine &sm);
signed char STATE_Hunter_Sniper_1(StateMachine &sm);
signed char STATE_KickDispenser(StateMachine &sm);
signed char STATE_LookAroundAlerted(StateMachine &sm);
signed char STATE_MafiaBoss(StateMachine &sm);
signed char STATE_MeleeTest(StateMachine &sm);
signed char STATE_MissionCollectionWaitForVan(StateMachine &sm);
signed char STATE_MissionCopycatKiller(StateMachine &sm);
signed char STATE_MissionJunkie(StateMachine &sm);
signed char STATE_MissionOpenSesame(StateMachine &sm);
signed char STATE_MissionPoliceIdle(StateMachine &sm);
signed char STATE_MissionSuicide(StateMachine &sm);
signed char STATE_MonsterSquad(StateMachine &sm);
signed char STATE_MonsterSquad_Freeze(StateMachine &sm);
signed char STATE_MonsterSquad_Missiles(StateMachine &sm);
signed char STATE_MonsterSquad_Plasma(StateMachine &sm);
signed char STATE_MonsterSquad_Sniper(StateMachine &sm);
signed char STATE_Mosh(StateMachine &sm);
signed char STATE_MotherInTank(StateMachine &sm);
signed char STATE_Panic(StateMachine &sm);
signed char STATE_PanicBlind(StateMachine &sm);
signed char STATE_PanicDropWeapon(StateMachine &sm);
signed char STATE_PanicExtreme(StateMachine &sm);
signed char STATE_PanicStand(StateMachine &sm);
signed char STATE_PanicStandForever(StateMachine &sm);
signed char STATE_PanicThenAttackPred(StateMachine &sm);
signed char STATE_Perv(StateMachine &sm);
signed char STATE_PlayStartAnim(StateMachine &sm);
signed char STATE_Police_Patrol(StateMachine &sm);
signed char STATE_PredatorFollowThePlayer(StateMachine &sm);
signed char STATE_QuickEvadePlayer(StateMachine &sm);
signed char STATE_QuickEvadePlayerWithGunButt(StateMachine &sm);
signed char STATE_RoninPredAttack(StateMachine &sm);
signed char STATE_RunToLocation1(StateMachine &sm);
signed char STATE_RunToLocation10(StateMachine &sm);
signed char STATE_RunToLocation11(StateMachine &sm);
signed char STATE_RunToLocation12(StateMachine &sm);
signed char STATE_RunToLocation13(StateMachine &sm);
signed char STATE_RunToLocation14(StateMachine &sm);
signed char STATE_RunToLocation15(StateMachine &sm);
signed char STATE_RunToLocation16(StateMachine &sm);
signed char STATE_RunToLocation17(StateMachine &sm);
signed char STATE_RunToLocation18(StateMachine &sm);
signed char STATE_RunToLocation19(StateMachine &sm);
signed char STATE_RunToLocation2(StateMachine &sm);
signed char STATE_RunToLocation20(StateMachine &sm);
signed char STATE_RunToLocation3(StateMachine &sm);
signed char STATE_RunToLocation4(StateMachine &sm);
signed char STATE_RunToLocation5(StateMachine &sm);
signed char STATE_RunToLocation6(StateMachine &sm);
signed char STATE_RunToLocation7(StateMachine &sm);
signed char STATE_RunToLocation8(StateMachine &sm);
signed char STATE_RunToLocation9(StateMachine &sm);
signed char STATE_RunToLocationIgnore1(StateMachine &sm);
signed char STATE_RunToLocationIgnore10(StateMachine &sm);
signed char STATE_RunToLocationIgnore11(StateMachine &sm);
signed char STATE_RunToLocationIgnore12(StateMachine &sm);
signed char STATE_RunToLocationIgnore13(StateMachine &sm);
signed char STATE_RunToLocationIgnore14(StateMachine &sm);
signed char STATE_RunToLocationIgnore15(StateMachine &sm);
signed char STATE_RunToLocationIgnore16(StateMachine &sm);
signed char STATE_RunToLocationIgnore17(StateMachine &sm);
signed char STATE_RunToLocationIgnore18(StateMachine &sm);
signed char STATE_RunToLocationIgnore19(StateMachine &sm);
signed char STATE_RunToLocationIgnore2(StateMachine &sm);
signed char STATE_RunToLocationIgnore20(StateMachine &sm);
signed char STATE_RunToLocationIgnore3(StateMachine &sm);
signed char STATE_RunToLocationIgnore4(StateMachine &sm);
signed char STATE_RunToLocationIgnore5(StateMachine &sm);
signed char STATE_RunToLocationIgnore6(StateMachine &sm);
signed char STATE_RunToLocationIgnore7(StateMachine &sm);
signed char STATE_RunToLocationIgnore8(StateMachine &sm);
signed char STATE_RunToLocationIgnore9(StateMachine &sm);
signed char STATE_RussianBossPredAttack(StateMachine &sm);
signed char STATE_SearchForPredator(StateMachine &sm);
signed char STATE_SeenBody_Help(StateMachine &sm);
signed char STATE_SeenBody_Panic(StateMachine &sm);
signed char STATE_ServBossPredAttack(StateMachine &sm);
signed char STATE_ShootAtCar(StateMachine &sm);
signed char STATE_ShootForwards(StateMachine &sm);
signed char STATE_SniperMove(StateMachine &sm);
signed char STATE_SniperStand(StateMachine &sm);
signed char STATE_Special_OldBloke(StateMachine &sm);
signed char STATE_Special_TrampBasher(StateMachine &sm);
signed char STATE_Special_YuppieMurderer(StateMachine &sm);
signed char STATE_StandAttractedToPlayer(StateMachine &sm);
signed char STATE_StandExpandedFightDist(StateMachine &sm);
signed char STATE_StandIdle_Invincible(StateMachine &sm);
signed char STATE_TestAlien_Init(StateMachine &sm);
signed char STATE_TestAnim(StateMachine &sm);
signed char STATE_TestBackoff(StateMachine &sm);
signed char STATE_TestDiscWeapon(StateMachine &sm);
signed char STATE_TestLooping(StateMachine &sm);
signed char STATE_TurningTest(StateMachine &sm);
signed char STATE_UlyssesLookAtPlayer(StateMachine &sm);
signed char STATE_UlyssesPoweredDown(StateMachine &sm);
signed char STATE_UlyssesPoweredDownAttack(StateMachine &sm);
signed char STATE_UlyssesProtectArea(StateMachine &sm);
signed char STATE_UlyssesStandAlert(StateMachine &sm);
signed char STATE_Ulysses_Follow_Path(StateMachine &sm);
signed char STATE_Ulysses_Stand_And_Shoot(StateMachine &sm);
signed char STATE_WalkAttractedToPlayer(StateMachine &sm);
signed char STATE_WalkToLocation1(StateMachine &sm);
signed char STATE_WalkToLocation10(StateMachine &sm);
signed char STATE_WalkToLocation11(StateMachine &sm);
signed char STATE_WalkToLocation12(StateMachine &sm);
signed char STATE_WalkToLocation13(StateMachine &sm);
signed char STATE_WalkToLocation14(StateMachine &sm);
signed char STATE_WalkToLocation15(StateMachine &sm);
signed char STATE_WalkToLocation16(StateMachine &sm);
signed char STATE_WalkToLocation17(StateMachine &sm);
signed char STATE_WalkToLocation18(StateMachine &sm);
signed char STATE_WalkToLocation19(StateMachine &sm);
signed char STATE_WalkToLocation2(StateMachine &sm);
signed char STATE_WalkToLocation20(StateMachine &sm);
signed char STATE_WalkToLocation3(StateMachine &sm);
signed char STATE_WalkToLocation4(StateMachine &sm);
signed char STATE_WalkToLocation5(StateMachine &sm);
signed char STATE_WalkToLocation6(StateMachine &sm);
signed char STATE_WalkToLocation7(StateMachine &sm);
signed char STATE_WalkToLocation8(StateMachine &sm);
signed char STATE_WalkToLocation9(StateMachine &sm);
signed char STATE_WalkToLocationCarry1(StateMachine &sm);
signed char STATE_WalkToLocationCarry10(StateMachine &sm);
signed char STATE_WalkToLocationCarry11(StateMachine &sm);
signed char STATE_WalkToLocationCarry12(StateMachine &sm);
signed char STATE_WalkToLocationCarry13(StateMachine &sm);
signed char STATE_WalkToLocationCarry14(StateMachine &sm);
signed char STATE_WalkToLocationCarry15(StateMachine &sm);
signed char STATE_WalkToLocationCarry16(StateMachine &sm);
signed char STATE_WalkToLocationCarry17(StateMachine &sm);
signed char STATE_WalkToLocationCarry18(StateMachine &sm);
signed char STATE_WalkToLocationCarry19(StateMachine &sm);
signed char STATE_WalkToLocationCarry2(StateMachine &sm);
signed char STATE_WalkToLocationCarry20(StateMachine &sm);
signed char STATE_WalkToLocationCarry3(StateMachine &sm);
signed char STATE_WalkToLocationCarry4(StateMachine &sm);
signed char STATE_WalkToLocationCarry5(StateMachine &sm);
signed char STATE_WalkToLocationCarry6(StateMachine &sm);
signed char STATE_WalkToLocationCarry7(StateMachine &sm);
signed char STATE_WalkToLocationCarry8(StateMachine &sm);
signed char STATE_WalkToLocationCarry9(StateMachine &sm);
signed char STATE_WalkToLocationIgnore1(StateMachine &sm);
signed char STATE_WalkToLocationIgnore10(StateMachine &sm);
signed char STATE_WalkToLocationIgnore11(StateMachine &sm);
signed char STATE_WalkToLocationIgnore12(StateMachine &sm);
signed char STATE_WalkToLocationIgnore13(StateMachine &sm);
signed char STATE_WalkToLocationIgnore14(StateMachine &sm);
signed char STATE_WalkToLocationIgnore15(StateMachine &sm);
signed char STATE_WalkToLocationIgnore16(StateMachine &sm);
signed char STATE_WalkToLocationIgnore17(StateMachine &sm);
signed char STATE_WalkToLocationIgnore18(StateMachine &sm);
signed char STATE_WalkToLocationIgnore19(StateMachine &sm);
signed char STATE_WalkToLocationIgnore2(StateMachine &sm);
signed char STATE_WalkToLocationIgnore20(StateMachine &sm);
signed char STATE_WalkToLocationIgnore3(StateMachine &sm);
signed char STATE_WalkToLocationIgnore4(StateMachine &sm);
signed char STATE_WalkToLocationIgnore5(StateMachine &sm);
signed char STATE_WalkToLocationIgnore6(StateMachine &sm);
signed char STATE_WalkToLocationIgnore7(StateMachine &sm);
signed char STATE_WalkToLocationIgnore8(StateMachine &sm);
signed char STATE_WalkToLocationIgnore9(StateMachine &sm);
signed char STATE_WalkToProstitution(StateMachine &sm);
signed char STATE_WanderTest(StateMachine &sm);
signed char STATE_WeaponTest(StateMachine &sm);
signed char STATE_WheresDaBeef(StateMachine &sm);
signed char STATE_ZombieAttack(StateMachine &sm);
signed char STATE_ZombieBoss(StateMachine &sm);
signed char STATE_Ulysses_Follow_Path_And_Kick_Ass(StateMachine &sm);
signed char STATE_PlayStartAnim(StateMachine &sm);

static int s_APCount[2];
static ActorProfile *s_APTable[2];
static int s_ASCount[1];
static ActorSkin *s_ASTable[1];

StateFn AP_GetStateFromHash(int hash)
{
    switch (hash) {
    case 0x4d000000:
        return 0;
    case 0x4d000002:
        return STATE_CityDistractions;
    case 0x4d000008:
        return STATE_Bouncer;
    case 0x4d000009:
        return STATE_DanceGirl;
    case 0x4d000164:
        return STATE_Perv;
    case 0x4d000173:
        return STATE_DeathMetal;
    case 0x4d000175:
        return STATE_KickDispenser;
    case 0x4d00000b:
        return STATE_Distraction_Path_Guard;
    case 0x4d00000c:
        return STATE_Distraction_Path_Runner;
    case 0x4d00000f:
        return STATE_Distraction_Stage;
    case 0x4d000182:
        return STATE_PanicDropWeapon;
    case 0x4d0000cd:
        return STATE_PanicBlind;
    case 0x4d0000e7:
        return STATE_SeenBody_Panic;
    case 0x4d0000e8:
        return STATE_SeenBody_Help;
    case 0x4d000111:
        return STATE_BlazinSquad;
    case 0x4d000184:
        return STATE_BurnTheDanceFloor;
    case 0x4d00012f:
        return STATE_ElectroSquad;
    case 0x4d000020:
        return STATE_DeleteNow;
    case 0x4d000021:
        return STATE_DeathBullet;
    case 0x4d000032:
    case 0x4d000033:
        return STATE_StandAttractedToPlayer;
    case 0x4d000034:
    case 0x4d000035:
        return STATE_WalkAttractedToPlayer;
    case 0x4d000023:
    case 0x4d000031:
        return STATE_PanicStand;
    case 0x4d000165:
        return STATE_CryForever;
    case 0x4d000101:
        return STATE_ChangeSkinThenAttackPred;
    case 0x4d0000c5:
        return STATE_PanicThenAttackPred;
    case 0x4d000038:
        return STATE_AttackPred;
    case 0x4d000037:
        return STATE_AttackPlayerForever;
    case 0x4d00010f:
        return STATE_FS_ThrowGrenadeForwards;
    case 0x4d000185:
        return STATE_ShootAtCar;
    case 0x4d000186:
        return STATE_UlyssesLookAtPlayer;
    case 0x4d000188:
        return STATE_CaptivePredHanging;
    case 0x4d000189:
        return STATE_CaptivePredRelease;
    case 0x4d00018b:
        return STATE_ShootForwards;
    case 0x4d00018d:
        return STATE_ZombieAttack;
    case 0x4d000191:
        return STATE_StandExpandedFightDist;
    case 0x4d000192:
        return STATE_WheresDaBeef;
    case 0x4d000193:
        return STATE_Mosh;
    case 0x4d000194:
        return STATE_Special_OldBloke;
    case 0x4d000195:
        return STATE_RussianBossPredAttack;
    case 0x4d000196:
        return STATE_Ulysses_Follow_Path_And_Kick_Ass;
    case 0x4d000197:
        return STATE_PanicStandForever;
    case 0x4d000198:
        return STATE_RoninPredAttack;
    case 0x4d0001a3:
        return STATE_Hunter1_PredAttack;
    case 0x4d0001a4:
        return STATE_HoldItemTest;
    case 0x4d0001a5:
        return STATE_HerIndoors;
    case 0x4d0001a6:
        return STATE_MotherInTank;
    case 0x4d0001a7:
        return STATE_MafiaBoss;
    case 0x4d0001ac:
        return STATE_MonsterSquad;
    case 0x4d0001a8:
        return STATE_MonsterSquad_Sniper;
    case 0x4d0001a9:
        return STATE_MonsterSquad_Plasma;
    case 0x4d0001aa:
        return STATE_MonsterSquad_Missiles;
    case 0x4d0001ab:
        return STATE_MonsterSquad_Freeze;
    case 0x4d0001ad:
        return STATE_ElecticShock;
    case 0x4d0001ae:
        return STATE_WanderTest;
    case 0x4d0001af:
        return STATE_Hunter2;
    case 0x4d0001b0:
        return STATE_Ulysses_Stand_And_Shoot;
    case 0x4d0001b1:
        return STATE_ZombieBoss;
    case 0x4d0001b2:
        return STATE_AttackPlayerOnSight;
    case 0x4d0001b3:
        return STATE_LookAroundAlerted;
    case 0x4d0001b4:
        return STATE_Brute_FleeToLoc18;
    case 0x4d0001b5:
        return STATE_TestLooping;
    case 0x4d0001b6:
        return STATE_TestBackoff;
    case 0x4d000081:
    case 0x4d000082:
        return STATE_FS_Chat;
    case 0x4d000083:
        return STATE_FS_Courting;
    case 0x4d000084:
        return STATE_FS_DrugDealer;
    case 0x4d000085:
        return STATE_FS_BuyDrugs;
    case 0x4d000086:
        return STATE_FS_Hustler;
    case 0x4d000087:
        return STATE_FS_Gamble;
    case 0x4d000088:
        return STATE_FS_Hooker;
    case 0x4d000089:
        return STATE_FS_BuyHooker;
    case 0x4d00018e:
        return STATE_FS_HookerAlt;
    case 0x4d00018f:
        return STATE_FS_BuyHookerAlt;
    case 0x4d00008a:
        return STATE_FS_Pimp;
    case 0x4d00008b:
        return STATE_FS_Mugger;
    case 0x4d00008c:
        return STATE_FS_Vendor;
    case 0x4d00008d:
        return STATE_FS_BuyVendor;
    case 0x4d00008e:
        return STATE_FS_SleepDoorway;
    case 0x4d000090:
        return STATE_FS_TrampGather;
    case 0x4d000091:
        return STATE_FS_Trashcan;
    case 0x4d000092:
        return STATE_FS_Loiter;
    case 0x4d000093:
        return STATE_FS_VendingMachine;
    case 0x4d000094:
        return STATE_FS_WindowShop;
    case 0x4d000095:
        return STATE_FS_GoDrinking;
    case 0x4d000096:
        return STATE_FS_GoShopping;
    case 0x4d0000c6:
        return STATE_FS_Piss;
    case 0x4d00016a:
        return STATE_FS_ATM;
    case 0x4d000174:
        return STATE_FS_Graffiti;
    case 0x4d000027:
    case 0x4d000097:
        return STATE_SniperStand;
    case 0x4d000028:
    case 0x4d000098:
        return STATE_SniperMove;
    case 0x4d000099:
        return STATE_FS_Delete;
    case 0x4d000128:
        return STATE_FS_Kill;
    case 0x4d00009a:
        return STATE_FS_Cower;
    case 0x4d000180:
        return STATE_FS_Pray;
    case 0x4d00018a:
        return STATE_FS_Pray_Ignore;
    case 0x4d000011:
    case 0x4d00009b:
        return STATE_Panic;
    case 0x4d000110:
        return STATE_FS_PanicStand;
    case 0x4d000022:
    case 0x4d00009c:
        return STATE_PanicExtreme;
    case 0x4d0000c8:
        return STATE_EvadePlayer;
    case 0x4d00015c:
        return STATE_QuickEvadePlayer;
    case 0x4d00015f:
        return STATE_QuickEvadePlayerWithGunButt;
    case 0x4d000160:
        return STATE_ChangeWeaponMode;
    case 0x4d000162:
        return STATE_ChargeAttack;
    case 0x4d0000c9:
        return STATE_FindHeal;
    case 0x4d0000d0:
        return STATE_FS_DriveBy;
    case 0x4d0000d1:
        return STATE_FS_ChangeSkin;
    case 0x4d000113:
        return STATE_FS_BankRobbery;
    case 0x4d000114:
        return STATE_FS_BankRobberyAccomplice;
    case 0x4d00016b:
        return STATE_FS_StoreRobbery;
    case 0x4d00016c:
        return STATE_FS_StoreRobberyAccomplice;
    case 0x4d00016d:
        return STATE_FS_Extortion;
    case 0x4d00016e:
        return STATE_FS_ExtortionAccomplice;
    case 0x4d00016f:
        return STATE_FS_BreakAndEnter;
    case 0x4d000170:
        return STATE_FS_BreakAndEnterAccomplice;
    case 0x4d000171:
        return STATE_FS_Vandalism;
    case 0x4d000172:
        return STATE_FS_VandalismAccomplice;
    case 0x4d000129:
        return STATE_FS_MimicryMe;
    case 0x4d00012b:
        return STATE_FS_FightClub;
    case 0x4d000166:
        return STATE_FS_CrowdCircle;
    case 0x4d00012d:
        return STATE_FS_SmokingBreak;
    case 0x4d00012e:
        return STATE_FS_Reprimand;
    case 0x4d000161:
        return STATE_FS_FactoryWorker;
    case 0x4d000167:
        return STATE_FS_CheckpointCharlie;
    case 0x4d000168:
        return STATE_FS_IdlingProstitutes;
    case 0x4d000169:
        return STATE_FS_ShadyDeal;
    case 0x4d00017e:
        return STATE_FS_LyingDownInstant;
    case 0x4d00018c:
        return STATE_FS_BangOnDoor;
    case 0x4d0000fd:
        return STATE_BOSS_RatPredEvadePlayer;
    case 0x4d000105:
        return STATE_AL_Chat;
    case 0x4d000106:
        return STATE_AL_Courting;
    case 0x4d000107:
        return STATE_AL_Gamble;
    case 0x4d000108:
        return STATE_AL_Idle;
    case 0x4d000109:
        return STATE_AL_Loiter;
    case 0x4d00010a:
        return STATE_AL_Piss;
    case 0x4d00010b:
        return STATE_AL_SleepDoorway;
    case 0x4d00010d:
        return STATE_AL_WindowShop;
    case 0x4d00010e:
        return STATE_Special_TrampBasher;
    case 0x4d000116:
        return STATE_Special_YuppieMurderer;
    case 0x4d000190:
        return STATE_WalkToProstitution;
    case 0x4d00009d:
        return STATE_WalkToLocation1;
    case 0x4d00009e:
        return STATE_WalkToLocation2;
    case 0x4d00009f:
        return STATE_WalkToLocation3;
    case 0x4d0000a0:
        return STATE_WalkToLocation4;
    case 0x4d0000a1:
        return STATE_WalkToLocation5;
    case 0x4d0000a2:
        return STATE_WalkToLocation6;
    case 0x4d0000a3:
        return STATE_WalkToLocation7;
    case 0x4d0000a4:
        return STATE_WalkToLocation8;
    case 0x4d0000a5:
        return STATE_WalkToLocation9;
    case 0x4d0000a6:
        return STATE_WalkToLocation10;
    case 0x4d0000a7:
        return STATE_WalkToLocation11;
    case 0x4d0000a8:
        return STATE_WalkToLocation12;
    case 0x4d0000a9:
        return STATE_WalkToLocation13;
    case 0x4d0000aa:
        return STATE_WalkToLocation14;
    case 0x4d0000ab:
        return STATE_WalkToLocation15;
    case 0x4d0000ac:
        return STATE_WalkToLocation16;
    case 0x4d0000ad:
        return STATE_WalkToLocation17;
    case 0x4d0000ae:
        return STATE_WalkToLocation18;
    case 0x4d0000af:
        return STATE_WalkToLocation19;
    case 0x4d0000b0:
        return STATE_WalkToLocation20;
    case 0x4d0000e9:
        return STATE_WalkToLocationIgnore1;
    case 0x4d0000ea:
        return STATE_WalkToLocationIgnore2;
    case 0x4d0000eb:
        return STATE_WalkToLocationIgnore3;
    case 0x4d0000ec:
        return STATE_WalkToLocationIgnore4;
    case 0x4d0000ed:
        return STATE_WalkToLocationIgnore5;
    case 0x4d0000ee:
        return STATE_WalkToLocationIgnore6;
    case 0x4d0000ef:
        return STATE_WalkToLocationIgnore7;
    case 0x4d0000f0:
        return STATE_WalkToLocationIgnore8;
    case 0x4d0000f1:
        return STATE_WalkToLocationIgnore9;
    case 0x4d0000f2:
        return STATE_WalkToLocationIgnore10;
    case 0x4d0000f3:
        return STATE_WalkToLocationIgnore11;
    case 0x4d0000f4:
        return STATE_WalkToLocationIgnore12;
    case 0x4d0000f5:
        return STATE_WalkToLocationIgnore13;
    case 0x4d0000f6:
        return STATE_WalkToLocationIgnore14;
    case 0x4d0000f7:
        return STATE_WalkToLocationIgnore15;
    case 0x4d0000f8:
        return STATE_WalkToLocationIgnore16;
    case 0x4d0000f9:
        return STATE_WalkToLocationIgnore17;
    case 0x4d0000fa:
        return STATE_WalkToLocationIgnore18;
    case 0x4d0000fb:
        return STATE_WalkToLocationIgnore19;
    case 0x4d0000fc:
        return STATE_WalkToLocationIgnore20;
    case 0x4d0000b1:
        return STATE_RunToLocation1;
    case 0x4d0000b2:
        return STATE_RunToLocation2;
    case 0x4d0000b3:
        return STATE_RunToLocation3;
    case 0x4d0000b4:
        return STATE_RunToLocation4;
    case 0x4d0000b5:
        return STATE_RunToLocation5;
    case 0x4d0000b6:
        return STATE_RunToLocation6;
    case 0x4d0000b7:
        return STATE_RunToLocation7;
    case 0x4d0000b8:
        return STATE_RunToLocation8;
    case 0x4d0000b9:
        return STATE_RunToLocation9;
    case 0x4d0000ba:
        return STATE_RunToLocation10;
    case 0x4d0000bb:
        return STATE_RunToLocation11;
    case 0x4d0000bc:
        return STATE_RunToLocation12;
    case 0x4d0000bd:
        return STATE_RunToLocation13;
    case 0x4d0000be:
        return STATE_RunToLocation14;
    case 0x4d0000bf:
        return STATE_RunToLocation15;
    case 0x4d0000c0:
        return STATE_RunToLocation16;
    case 0x4d0000c1:
        return STATE_RunToLocation17;
    case 0x4d0000c2:
        return STATE_RunToLocation18;
    case 0x4d0000c3:
        return STATE_RunToLocation19;
    case 0x4d0000c4:
        return STATE_RunToLocation20;
    case 0x4d0000d2:
        return STATE_RunToLocationIgnore1;
    case 0x4d0000d3:
        return STATE_RunToLocationIgnore2;
    case 0x4d0000d4:
        return STATE_RunToLocationIgnore3;
    case 0x4d0000d5:
        return STATE_RunToLocationIgnore4;
    case 0x4d0000d6:
        return STATE_RunToLocationIgnore5;
    case 0x4d0000d7:
        return STATE_RunToLocationIgnore6;
    case 0x4d0000d8:
        return STATE_RunToLocationIgnore7;
    case 0x4d0000d9:
        return STATE_RunToLocationIgnore8;
    case 0x4d0000da:
        return STATE_RunToLocationIgnore9;
    case 0x4d0000db:
        return STATE_RunToLocationIgnore10;
    case 0x4d0000dc:
        return STATE_RunToLocationIgnore11;
    case 0x4d0000dd:
        return STATE_RunToLocationIgnore12;
    case 0x4d0000de:
        return STATE_RunToLocationIgnore13;
    case 0x4d0000df:
        return STATE_RunToLocationIgnore14;
    case 0x4d0000e0:
        return STATE_RunToLocationIgnore15;
    case 0x4d0000e1:
        return STATE_RunToLocationIgnore16;
    case 0x4d0000e2:
        return STATE_RunToLocationIgnore17;
    case 0x4d0000e3:
        return STATE_RunToLocationIgnore18;
    case 0x4d0000e4:
        return STATE_RunToLocationIgnore19;
    case 0x4d0000e5:
        return STATE_RunToLocationIgnore20;
    case 0x4d000130:
        return STATE_WalkToLocationCarry1;
    case 0x4d000131:
        return STATE_WalkToLocationCarry2;
    case 0x4d000132:
        return STATE_WalkToLocationCarry3;
    case 0x4d000133:
        return STATE_WalkToLocationCarry4;
    case 0x4d000134:
        return STATE_WalkToLocationCarry5;
    case 0x4d00014d:
        return STATE_WalkToLocationCarry6;
    case 0x4d00014e:
        return STATE_WalkToLocationCarry7;
    case 0x4d00014f:
        return STATE_WalkToLocationCarry8;
    case 0x4d000150:
        return STATE_WalkToLocationCarry9;
    case 0x4d000151:
        return STATE_WalkToLocationCarry10;
    case 0x4d000152:
        return STATE_WalkToLocationCarry11;
    case 0x4d000153:
        return STATE_WalkToLocationCarry12;
    case 0x4d000154:
        return STATE_WalkToLocationCarry13;
    case 0x4d000155:
        return STATE_WalkToLocationCarry14;
    case 0x4d000156:
        return STATE_WalkToLocationCarry15;
    case 0x4d000157:
        return STATE_WalkToLocationCarry16;
    case 0x4d000158:
        return STATE_WalkToLocationCarry17;
    case 0x4d000159:
        return STATE_WalkToLocationCarry18;
    case 0x4d00015a:
        return STATE_WalkToLocationCarry19;
    case 0x4d00015b:
        return STATE_WalkToLocationCarry20;
    case 0x4d000056:
        return STATE_MissionCollectionWaitForVan;
    case 0x4d0000ca:
        return STATE_MissionCopycatKiller;
    case 0x4d0000ff:
        return STATE_MissionJunkie;
    case 0x4d000103:
        return STATE_Police_Patrol;
    case 0x4d000123:
        return STATE_MissionSuicide;
    case 0x4d000126:
        return STATE_MissionOpenSesame;
    case 0x4d00017f:
        return STATE_FS_OpenSesame;
    case 0x4d000127:
        return STATE_MissionPoliceIdle;
    case 0x4d0000e6:
        return STATE_Ulysses_Follow_Path;
    case 0x4d0000fe:
        return STATE_TestAnim;
    case 0x4d000102:
        return STATE_BlendAnimTest;
    case 0x4d00002f:
    case 0x4d000104:
        return STATE_DefaultIdle;
    case 0x4d000187:
        return STATE_StandIdle_Invincible;
    case 0x4d000112:
        return STATE_AttackPredBruno_UseLoc;
    case 0x4d000121:
        return STATE_UlyssesProtectArea;
    case 0x4d000122:
        return STATE_UlyssesPoweredDown;
    case 0x4d000125:
        return STATE_UlyssesPoweredDownAttack;
    case 0x4d00012a:
        return STATE_WeaponTest;
    case 0x4d000135:
        return STATE_MeleeTest;
    case 0x4d000136:
        return STATE_TurningTest;
    case 0x4d000137:
        return STATE_BrutePredAttack;
    case 0x4d000138:
        return STATE_DropMine;
    case 0x4d00015d:
        return STATE_TestDiscWeapon;
    case 0x4d00015e:
        return STATE_ServBossPredAttack;
    case 0x4d000163:
        return STATE_Hunter_Sniper_1;
    case 0x4d000176:
        return STATE_SearchForPredator;
    case 0x4d00017c:
        return STATE_UlyssesStandAlert;
    case 0x4d000183:
        return STATE_PredatorFollowThePlayer;
    case 0x4d000139:
        return STATE_GotoLocationViaEvadePoints1;
    case 0x4d00013a:
        return STATE_GotoLocationViaEvadePoints2;
    case 0x4d00013b:
        return STATE_GotoLocationViaEvadePoints3;
    case 0x4d00013c:
        return STATE_GotoLocationViaEvadePoints4;
    case 0x4d00013d:
        return STATE_GotoLocationViaEvadePoints5;
    case 0x4d00013e:
        return STATE_GotoLocationViaEvadePoints6;
    case 0x4d00013f:
        return STATE_GotoLocationViaEvadePoints7;
    case 0x4d000140:
        return STATE_GotoLocationViaEvadePoints8;
    case 0x4d000141:
        return STATE_GotoLocationViaEvadePoints9;
    case 0x4d000142:
        return STATE_GotoLocationViaEvadePoints10;
    case 0x4d000143:
        return STATE_GotoLocationViaEvadePoints11;
    case 0x4d000144:
        return STATE_GotoLocationViaEvadePoints12;
    case 0x4d000145:
        return STATE_GotoLocationViaEvadePoints13;
    case 0x4d000146:
        return STATE_GotoLocationViaEvadePoints14;
    case 0x4d000147:
        return STATE_GotoLocationViaEvadePoints15;
    case 0x4d000148:
        return STATE_GotoLocationViaEvadePoints16;
    case 0x4d000149:
        return STATE_GotoLocationViaEvadePoints17;
    case 0x4d00014a:
        return STATE_GotoLocationViaEvadePoints18;
    case 0x4d00014b:
        return STATE_GotoLocationViaEvadePoints19;
    case 0x4d00014c:
        return STATE_GotoLocationViaEvadePoints20;
    case 0x4d000115:
        return STATE_TestAlien_Init;
    case 0x4d00017a:
        return STATE_Alien_Attack;
    case 0x4d000199:
        return STATE_Alien_Attack2;
    case 0x4d00019a:
        return STATE_Alien_Attack3;
    case 0x4d00019b:
        return STATE_Alien_Attack4;
    case 0x4d00019c:
        return STATE_Alien_Attack5;
    case 0x4d000179:
        return STATE_Alien_CeilingHang;
    case 0x4d00019d:
        return STATE_Alien_CeilingHang2;
    case 0x4d00019e:
        return STATE_Alien_CeilingHang3;
    case 0x4d00017b:
        return STATE_Alien_HoleDweller;
    case 0x4d0001a1:
        return STATE_Alien_HoleDweller2;
    case 0x4d0001a2:
        return STATE_Alien_HoleDweller3;
    default:
        if (hash) ex::TRACE("AP_GetStateFromHash() - Unknown state requested %s - 0x%08x", EXHashCodeFile::Debug_HashCodeName(hash), hash);
        return 0;
    }
}

ActorProfile *AP_GetPtr(int hash)
{
    for (int s = 1; s >= 0; s--)
        for (int i = 0; i < s_APCount[s]; i++)
            if (s_APTable[s][i].m_Hash == hash) return &s_APTable[s][i];
    ex::TRACE("AP_GetPtr() - Unknown profile requested %s", EXHashCodeFile::Debug_HashCodeName(hash));
    return 0;
}

void AP_GetIndex(int hash, ActorProfileIndex &idx)
{
    idx.m_Sheet = 0xffff;
    idx.m_Row = 0xffff;
    for (int s = 1; s >= 0; s--)
        for (int i = 0; i < s_APCount[s]; i++)
            if (s_APTable[s][i].m_Hash == hash) {
                idx.m_Sheet = s;
                idx.m_Row = i;
                return;
            }
}

void AS_GetIndex(int hash, ActorSkinIndex &idx)
{
    idx.m_Sheet = 0xffff;
    idx.m_Row = 0xffff;
    for (int s = 0; s >= 0; s--)
        for (int i = 0; i < s_ASCount[s]; i++)
            if (s_ASTable[s][i].m_Hash == hash) {
                idx.m_Sheet = s;
                idx.m_Row = i;
                return;
            }
}

ActorProfile *AP_GetPtrFromIndex(const ActorProfileIndex &idx)
{
    if (idx.m_Sheet != 0xffff && idx.m_Row != 0xffff) return &s_APTable[idx.m_Sheet][idx.m_Row];
    return 0;
}

ActorSkin *AS_GetPtrFromIndex(const ActorSkinIndex &idx)
{
    if (idx.m_Sheet != 0xffff && idx.m_Row != 0xffff) return &s_ASTable[idx.m_Sheet][idx.m_Row];
    return 0;
}

StateFn AP_GetStartState(ActorProfile *p)
{
    if (p->m_StartAnim) return STATE_PlayStartAnim;
    return AP_GetStateFromHash(p->m_StartState);
}

signed char AP_DoWeFight(const ActorProfile *a, const ActorProfile *b)
{
    int ga = a->m_Group - 0x4b000001;
    int gb = b->m_Group - 0x4b000001;
    if (ga >= 0 && ga < 12 && gb >= 0 && gb < 12) return FSAttribs[0].m_Fight[gb][ga];
    return 0;
}

int AP_GetRandSkinHash(ActorProfile *p)
{
    s8 bStarted = 0;
    int i = p->m_LastSkin;
    while (!bStarted || !p->m_Skins[i]) {
        bStarted = 1;
        i = (i + 1) % 6;
    }
    p->m_LastSkin = i;
    return p->m_Skins[i];
}

void AP_RefreshPointer()
{
    s_APCount[0] = 0;
    s_APTable[0] = (ActorProfile *)AU_GetWorkSheet(0x100004c, 0x14000003, 0, s_APCount[0]);
    s_APCount[1] = 0;
    s_APTable[1] = (ActorProfile *)AU_GetWorkSheet(gCurrentMap_GeoHash, 0x14000003, 0, s_APCount[1]);
}

void AP_PostLoadInit()
{
    s_APCount[0] = 0;
    s_APTable[0] = (ActorProfile *)AU_GetWorkSheet(0x100004c, 0x14000003, 0, s_APCount[0]);
    s_APCount[1] = 0;
    s_APTable[1] = (ActorProfile *)AU_GetWorkSheet(gCurrentMap_GeoHash, 0x14000003, 0, s_APCount[1]);
    for (int s = 0; s < 2; s++) {
        for (int i = 0; i < s_APCount[s]; i++) {
            if (!s_APTable[s][i].m_IdleState) s_APTable[s][i].m_IdleState = 0x4d00002f;
            if (s_APTable[s][i].m_Weapon == 0x200016e) s_APTable[s][i].m_Weapon = 0;
            for (int c = 0; c < 5; c++) {
                ActorProfileChannel &ch = s_APTable[s][i].m_Channels[c];
                if (ch.m_Channel < 0 || ch.m_Channel >= 290) {
                    ex::TRACE("bad flag channel %s", EXHashCodeFile::Debug_HashCodeName(s_APTable[s][i].m_Hash));
                    ex::EXDebugBreak();
                }
            }
            s_APTable[s][i].m_LastSkin = -1;
            s_APTable[s][i].m_Dist = s_APTable[s][i].m_Dist * s_APTable[s][i].m_Dist;
        }
    }
}

ActorSkin *AS_GetPtr(int hash)
{
    for (int s = 0; s < 1; s++)
        for (int i = 0; i < s_ASCount[s]; i++)
            if (s_ASTable[s][i].m_Hash == hash) return &s_ASTable[s][i];
    return 0;
}

ActorSkin *AS_GetIndexPtr(int i)
{
    if (i < 0) return 0;
    int base = 0;
    for (int s = 0; s < 1; s++) {
        if (i <= s_ASCount[s] + base) return &s_ASTable[s][i - base];
        base += s_ASCount[s];
    }
    return 0;
}

void AS_RefreshPointer()
{
    s_ASCount[0] = 0;
    s_ASTable[0] = (ActorSkin *)AU_GetWorkSheet(0x100004c, 0x14000005, 0, s_ASCount[0]);
}

void AS_PostLoadInit()
{
    s_ASCount[0] = 0;
    s_ASTable[0] = (ActorSkin *)AU_GetWorkSheet(0x100004c, 0x14000005, 0, s_ASCount[0]);
}

void AP_GetRandPOI(Actor *pActor, int &poi, int &b)
{
    ActorProfile *p = AP_GetPtrFromIndex(pActor->m_ProfileIndex);
    poi = 0;
    b = 0;
    int total = 0;
    int i;
    for (i = 0; i < 30; i++) total += p->m_POIWeights[i];
    if (total < 1) return;
    int r = EXRand(total) + 1;
    int sum;
    for (i = 0, sum = 0; i < 30; i++) {
        sum += p->m_POIWeights[i];
        if (r <= sum) goto found;
    }
    return;
found:
    if (FSAttribs[0].m_POITypes[i].m_Type)
        poi = POI_GetNearestType(pActor->GetFeetPos(), FSAttribs[0].m_POITypes[i].m_Type, pActor->LastCityPOI(), pActor->GetNavZone(), pActor->CanJump(), 1);
    else
        poi = POI_GetNearestFlag(pActor->GetFeetPos(), FSAttribs[0].m_POITypes[i].m_Flag, pActor->LastCityPOI(), pActor->GetNavZone(), pActor->CanJump(), 1, -1.0f);
    b = FSAttribs[0].m_POITypes[i].m_Unk8;
    pActor->SetLastCityPOI(poi);
}
