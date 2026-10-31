#ifndef XITEMHANDLER_PLAYER_H
#define XITEMHANDLER_PLAYER_H
#include "EXItemHandler.h"
#include "EXVector.h"
#include "EXQuaternion.h"
#include "EXBit128.h"
#include "EXSoundTagStandIn.h"
#include "EXExtRenderInfoStandIn.h"
#include "EXArray.h"

class XItem;
class EXItem;
class Actor;
class Vehicle;
class EXItemAnimator_Entity;
class EXItemAnimator_Anim;
class EXSwooshSys;
class EXLineSeg;
class EXPlane;
struct EXGrabPointTemp;
struct EXGrabPointArrayTemp;

// TODO stand-in: texture swap table (EXRenderEnv.obj, engine render area). sizeof 0x14.
#include "EXTexSwapInfo.h"

// The player: the Predator (Player.obj). sizeof 0xd90. One instance (m_pInstance).
// m_Mode (UpdatePlayer's dispatch): 0 idle, 1 move, 2 crouch, 3 and 9 jump, 4 clamber,
// 6 hit (attack), 7 special hit, 8 land, 0xa wall jump, 0xc hang, 0xe skid, 0x10 pre-jump,
// 0x11 monkey bars, 0x12 perch, 0x13 TOS (button mash), 0x14 Ulysses, 0x15 car ride,
// 0x16 grab, 0x17 react (hit/knockdown), 0x18 gecko (wall walk); 5, 0xb, 0xd, 0xf no handler. Special upgrade slots
// (m_SpecialUpgrades): 0 health, 1 energy, 2 armour, 3 ammo, 4 ?.
class XItemHandler_Player : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_Player)
public:
    virtual ~XItemHandler_Player();
    virtual signed char Initialise();
    virtual signed char DoUpdate();
    unsigned int GetMode() { return m_Mode; }
    unsigned int GetSpecialActorID() { return m_SpecialActorID; }
    unsigned char GetVisionMode() { return m_Tint; }
    EXVector GetAmbientColour() { return m_AmbientColour; }
    unsigned int GetSpecialMoveAvailable() { return m_SpecialMoveAvailable; }
    static XItemHandler_Player *GetInstance() { return m_pInstance; }
    static XItemHandler_Player *Instance() { return m_pInstance; }
    float GetSpeed() { return m_Speed.LengthXZ(); }  // matches the Actor.obj copy (0x1a540)
    void GetDelta(EXVector &v) { v = m_Delta; }
    signed char IsHanging()
    {
        if (m_Mode == 0xc || m_Mode == 4) return 1;
        return 0;
    }
    float GetHealth() { return m_Health; }
    float GetCloakEnergy() { return m_Energy; }
    void SetButtonLock(unsigned int m) { m_ButtonLock = m; }
    void SetPlayerHasControl(signed char b) { m_bPlayerHasControl = b; }
    float GetPlasmaCharge() { return m_PlasmaCharge; }
    int GetWeaponUpgrade(int i)
    {
        if (i >= 0 && i < 8) return m_Unkcf0[i];
        return 0;
    }
    signed char GetHelmet() { return m_bHelmet; }
    unsigned int GetSubMode() { return m_SubMode; }
    EXQuaternion GetGeckoAngle() { return m_GeckoAngle; }
    EXVector GetGeckoPoint() { return m_GeckoPoint; }
    void SetButtonMask(unsigned int m) { m_ButtonMask = m; }
    void SetAnalogMask(unsigned int m) { m_AnalogMask = m; }
    signed char GetScanMode() { return m_bScanMode; }
    signed char CanPullUp() { return !((m_StateFlags >> 4) & 1); }  // [fi] copy in Camera.obj
    __forceinline void GetLedgeTarget(EXVector &v, float &f)  // [fi] copy in Camera.obj
    {
        v = m_LedgeTarget;
        f = m_LedgeAngle;
    }
    signed char HasMoveFlag(unsigned int f) { return (m_MoveFlags & f) == f; }
    float GetCrouchTimer() { return m_CrouchTimer; }
    int GetCurrentWeapon(int slot) { return m_CurrentWeapon[slot]; }
    signed char IsAttackingBehind() { return m_AttackState > 3; }
    EXItemAnimator_Entity *GetWeaponItem(int i) { return m_pWeaponItem[i]; }
    void SetHealth(float f) { m_Health = f; }
    float GetMaxHealth()
    {
        switch (m_SpecialUpgrades[0]) {
        case 1: return 133.0f;
        case 2: return 166.0f;
        case 3: return 200.0f;
        }
        return 100.0f;
    }
    float SparkTimer() { return m_SparkTimer; }
    int GetAmmo(int i) { return m_Ammo[i]; }
    void SetSparkTimer(float f) { m_SparkTimer = f; }
    short GetSelectedSkin() const { return m_SelectedSkin; }
    void SetSelectedSkin(short s) { m_SelectedSkin = s; }
    int *GetWeaponUpgradeArray() { return m_Unkcf0; }
    unsigned int VisionModeFlagsGet() { return m_VisionModeFlags; }
    int GetSpecialUpgrade(int i)
    {
        if (i >= 0 && i < 5) return m_SpecialUpgrades[i];
        return 0;
    }
    signed char IsDead()
    {
        if (m_Health <= 0.0f) return 1;
        return 0;
    }
    void SetCloakEnergy(float f) { m_Energy = f; }
    void VisionModeFlagsSet(unsigned int f) { m_VisionModeFlags = f; }
    void VisionModeFlagsAdd(unsigned int f) { m_VisionModeFlags |= f; }
    void VisionModeFlagsSub(unsigned int f) { m_VisionModeFlags &= ~f; }
    void SetWet(unsigned int w) { m_Wet = (u8)w; }
    void SetForceTrackCam(signed char b) { m_bForceTrackCam = b; }
    void SetAmmo(int weapon, int n) { m_Ammo[weapon] = (short)n; }
    int *GetSpecialUpgradeArray() { return m_SpecialUpgrades; }
    void SetPlayerIndex(unsigned int i) { m_PlayerIndex = i; }
    signed char IsHidden() { return m_bHidden; }
    void SetAmbientColour(EXVector v)
    {
        m_AmbientColour.x = v.x;
        m_AmbientColour.y = v.y;
        m_AmbientColour.z = v.z;
        m_AmbientColour.w = v.w;
    }
    EXExtRenderInfo *GetPlayerExtRenderInfo() { return &m_PlayerExtRenderInfo; }
    EXExtRenderInfo *GetWeaponExtRenderInfo(int i) { return &m_WeaponExtRenderInfo[i]; }
    void SetAnalogLock(unsigned int m) { m_AnalogLock = m; }
    void GetLookAt(EXVector&);
    float GetBaseHeight();
    void PlayNegativeSound(int);
    void UseEnergy(float);
    float GetPlasmaChargeEnergy();
    XItem* GetStandOn();
    void SetHelmet(signed char);
    void RagDollHeldDrone(int);
    signed char IsHoldingActor();
    void ForceModeChange(unsigned int);
    EXVector GetChestPos();
    EXVector GetPelvisPos();
    EXVector GetMidFeetPos();
    EXVector GetTopHeadPos();
    signed char GetCloak();
    void SetCloakDirect(signed char);
    void BreakCloak(int);
    void SetVisionMode(unsigned char);
    signed char GetAttackState(int&, int&);
    signed char OpenToAttack(EXVector&);
    signed char CanAssignWeapon();
    void SetFloorType(int);
    void VehicleRemoved(Vehicle*);
    void PickedUpWeapon(int);
    float GetMaxEnergy()
    {
        switch (m_SpecialUpgrades[1]) {
        case 1: return 133.0f;
        case 2: return 166.0f;
        case 3: return 200.0f;
        }
        return 100.0f;
    }
    int GetMaxAmmo() { return m_SpecialUpgrades[3] * 4 + 4; }
    float GetMaxArmour()
    {
        switch (m_SpecialUpgrades[2]) {
        case 1: return 0.95f;
        case 2: return 0.9f;
        case 3: return 0.8f;
        }
        return 1.0f;
    }
    void InitPlayerBodyWeapons();
    int GetMaxAmmo(int);
    signed char AddAmmo(int, int);
    int GetFaveWeapon();
    unsigned int GetCurrentSkinHash();
    signed char CheckCollision(EXLineSeg&, EXVector&, float&);
    void Resurrect();
    void SparkSkin(float);
    void EMPPlayer();
    void HandleVisionModeTextureSwaps(unsigned char);
    void CommitVisionMode(unsigned char);
    void UpdateVisionMode();
    void LevelWeaponUpgradeLoad(int);
    void SetWeaponUpgrade(int, int);
    void InGameWeaponUpgradeAdd(int);
    void SetSpecialUpgrade(int, int);
    void AddSpecialUpgrade(int, int);
    XItemHandler_Player();
    void RefreshAllWeapons();
    void Hide(signed char, signed char);
    void SetDiskInHand(signed char);
    void SetCurrentWeapon(int, int);
    void HandleDamageSkins();
    void ForceSafeWeapon();
    void SetCloak(signed char);
    void UpdateCloak();
    static XItemHandler_Player* CreateObject();
    void PlayerPostUpdate();
    void SetupMove(unsigned int, unsigned int, float);
    void TeleportPlayer(EXVector&, float);
    void KillPlayer();
    void SendPredMsg(int, EXVector&, float);
    void SendPredMsg(int, float);
    void SendPredMsg(int, unsigned int);
    void SendPredMsg(int, EXVector&, EXVector&, float);
    XItem *GetSpecialItem() { return m_pSpecialItem; }
    XItem *JustOffCar() { return m_pJustOffCar; }
protected:
    virtual signed char DoPostUpdate();
    virtual unsigned int DoScriptCmdEvent(GEOSCRIPT_EVENT*, EXItemAnimator*);
    virtual unsigned int DoScriptCmdObject(GEOSCRIPT_BASECMD*, EXItemAnimator*);
    signed char AnimEnded()
    {
        if (m_bAnimEnded) {
            m_bAnimEnded = 0;
            return 1;
        }
        return 0;
    }
    void UnlockHand() { m_HandLock = -1.0f; }
    void DeadStop(signed char);
    void ButtonUsed(int);
    Actor* GetLockonActor();
    void SendAttackMessage(int, int);
    int GetPlasmaChargeSpeed(float&);
    int GetWeaponSection(int);
    void LandedOnVehicle();
    void SetInertia(float);
    int GetUpgradedSkin();
    void LoadPredatorSkin();
    void DoEquip();
    signed char CheckForActorSpace(EXVector&, float, float);
    signed char MoveCheckForSticky();
    float CalculateMSpeed(float);
    void ResetAngleStore();
    void GetHandLockPos(int, EXVector&);
    void GetGrabVector(int, EXVector&);
    void DoClimb();
    void ResetCombo();
    int GetComboAvailable();
    void AdvanceCombo(int, signed char);
    Actor* GetLockOnActor();
    Actor* GetTargetActor();
    signed char CheckForThrowSpace(float);
    signed char CheckForStandingSpace(EXVector&, float);
    signed char CheckForFinishMoveSpace(signed char, float, float, float, float);
    void SafeBreakoutHeadRip();
    void RollActorRagDoll(Actor*, int);
    signed char UBBing();
    signed char HasLanded();
    signed char MonkeyCheckPullUp(EXGrabPointTemp&);
    signed char HandleShimmyAngle(EXVector&, float, signed char, float&);
    signed char GetGrabPoint(int);
    signed char CheckForWallSlam();
    signed char GetGeckoValidPoint();
    void CheckModeChange(unsigned int);
    void PlaySpecialFXScript(int);
    void AdjustTargetAdjustment();
    void SetTargetAdjustment(EXVector, float, int);
    void CheckForVehicleHit();
    signed char GetTargetPos(EXVector&, int);
    int CheckForTakeDownScenery();
    EXSwooshSys* CreateSwoosh(unsigned int, EXItem*, EXItemAnimator_Entity*);
    void DeleteSwoosh(unsigned int, EXItemAnimator_Entity*);
    void DeleteAllSwoosh();
    void AddWeaponLoad(unsigned int);
    void LoadUpgradeWeaponGFX(int, int);
    void GetPadData();
    signed char HoldingHeavyWeapon();
    int HoldingWeapon(int);
    void UpdatePlayerBodyWeapons();
    void SetHandWeaponPosRot(int);
    void SendVMMessages(int);
    void UpdateSectionFiles();
    EXItem* GetPlatformItem(int&);
    void CalculateTargetJump();
    void PostUpdatePlayer();
    void DebugDrawPredCollision();
    int WeaponIdToSlot(int);
    void SetDefaultAmmo();
    void UpdateWeaponUpgradeGFX(int);
    void UpdateWeaponGFX(int, float);
    void SetWeapon(int);
    void InitPlasmaCasterGFX();
    void InitWristBladeGFX();
    signed char MoveCheckForLedge();
    signed char CheckForClamberSpace();
    signed char MoveCheckForActor();
    void MoveJump();
    void LockHand(int, int, signed char);
    void HandleGrabAdjustment();
    void ResetWeaponDown();
    void ResetAttack();
    void SafeBreakoutAll(int);
    void CalculateHitType(int, signed char&, signed char&);
    void CalculateHitMove(int);
    void CheckChainButtonPress();
    signed char CheckPerchLand(EXGrabPointTemp&);
    signed char CheckPerchPlaneLand(EXGrabPointTemp&);
    signed char CheckForActorStomp();
    signed char MonkeyCheckAhead();
    signed char MonkeyCheckAround(EXVector&);
    signed char PerchCheckAhead(EXGrabPointTemp&);
    signed char CheckDeadFall();
    int GetGeckoFormQuart(EXQuaternion&, EXPlane&, signed char, float, float);
    EXVector GetGeckoAdjustPoint(EXQuaternion&);
    void GetGeckoAngle(float&);
    void SetupDepthPtr(float*, unsigned int);
    void CheckForCharacterHit(int);
    signed char CheckTargetThrowAngle();
    void ShootWeapon(int, signed char);
    void FootStepEvent(signed char);
    int GetCardRandom();
    signed char CheckForStealthKill(signed char);
    signed char CheckForGrabMove(signed char);
    signed char CheckForGroundAttack();
    signed char CheckForFinishingMove(signed char);
    signed char DoFinishingMove(signed char);
    void EnableCloak(signed char);
    void SetWeaponsToHeatRender(unsigned char);
    void InitUpgrades();
    void LoadWeaponsAsync();
    void InitClassVars();
    void SetupMoveBlend(unsigned int, unsigned int, float);
    void UpdatePlayerWeapons();
    void UseWeaponJump(int);
    void UpdateDroneToGrabHand();
    signed char DoGrabMove(signed char);
    void RestorePreviousWeapon();
    signed char CheckForCombo();
    signed char GetSegmentOn(EXGrabPointArrayTemp&, EXVector&, EXVector&, EXVector&);
    signed char GetSegmentNext(EXGrabPointArrayTemp&, EXVector&, signed char, EXVector&, EXVector&);
    signed char GetSegmentSpace(EXGrabPointArrayTemp&, EXVector&, signed char, float, float, float);
    signed char CheckForShimmySpace(signed char);
    void ReactStumble(EXVector&);
    void ReactExplode(EXVector&, float);
    signed char CheckForJump(signed char);
    void DoClamber();
    void DoTOS();
    void DoUlysses();
    void DoCarRide();
    signed char CheckForIdleBreakOut(int);
    signed char CheckForStop(int);
    signed char CheckForFall();
    void DoMonkey();
    void DoPerch();
    signed char CheckLineSegSpace(EXGrabPointTemp&, float&);
    signed char CheckForShimmy(signed char);
    void DoHang();
    signed char CheckWallMove();
    void DoPreJump();
    void DoGeckoJumpOff(signed char);
    signed char CheckForTOS();
    signed char CheckForBeaconPoint();
    signed char CheckForRechargePoint();
    signed char DoStealthKill(signed char);
    signed char DoGroundAttack(int);
    void ReactWeaponPickup(int, signed char);
    void UseWeapon(int);
    void HandleWeaponButtons();
    signed char MoveNormal(int);
    void MoveStrafe();
    void DoReact();
    void DoGrab();
    void DoHitSpecial();
    void DoHit();
    signed char CheckForFlipGrab();
    void DoIdle();
    void DoMove();
    void DoCrouch();
    void DoGecko();
    void InflictDamage(float, int);
    void DoSkid();
    void DoLand();
    void InflictDamageFromFall(float);
    void CheckForLand();
    void DoWallJump();
    void InflictDamage(float, EXVector&, EXVector&, int);
    int CheckGrabPoint(int);
    void DoJump();
    signed char UpdatePlayer();
protected:
    static XItemHandler_Player *m_pInstance;
public:
    EXItemAnimator_Anim *m_pAnim;               // 0x08
    ex::EXBit128 m_SectionBits;                 // 0x0c upper-body sections for the blend stack (SetupMoveBlend)
    EXExtRenderInfo m_PlayerExtRenderInfo;      // 0x1c
    EXTexSwapInfo m_PlayerTexSwap;              // 0x9c
    unsigned int m_UpdateCount;                 // 0xb0 UpdatePlayer calls
    unsigned int m_PlayerIndex;                 // 0xb4
    u8 m_PadIndex;                              // 0xb8 gamepad slot (GetPadData)
    u8 m_Unkb9[0xbc - 0xb9];
    int m_SkinBC;                               // 0xbc skin set (PRED_Skins index)
    int m_SkinC0;                               // 0xc0 skin variant
    int m_SkinC4;                               // 0xc4
    union {
        short m_SelectedSkin;                   // 0xc8
        unsigned short m_Unkc8;                 // (name used by GameData.cpp)
    };
    u8 m_Unkca[0xcc - 0xca];
    EXSoundTag m_SoundTag;                      // 0xcc
    u8 m_Unkd4;
    s8 m_bScanMode;                             // 0xd5
    u8 m_Unkd6[0xd8 - 0xd6];
    float m_StickAngle;                         // 0xd8 stick direction relative to the camera (GetPadData)
    float m_StickMag;                           // 0xdc analogue stick magnitude (CheckForIdleBreakOut)
    unsigned int m_ButtonMask;                  // 0xe0
    unsigned int m_AnalogMask;                  // 0xe4
    unsigned int m_ButtonLock;                  // 0xe8
    unsigned int m_AnalogLock;                  // 0xec
    unsigned int m_ButtonsPressed;              // 0xf0
    unsigned int m_ButtonsHeld;                 // 0xf4
    unsigned int m_ButtonsReleased;             // 0xf8
    EXVector m_Pad;                             // 0xfc analogue stick (x: CheckForShimmy)
    float m_PadAngle;                           // 0x10c stick direction (CalculateHitMove)
    float m_PrevStickAngle;                     // 0x110 m_StickAngle last frame (GetPadData)
    float m_CamAngle;                           // 0x114 camera yaw the stick is relative to (GetPadData)
    float m_TOSCharge;                          // 0x118 button-mash charge (DoTOS)
    float m_TOSPress;                           // 0x11c recent press timer (DoTOS)
    float m_HangYaw;                            // 0x120 item yaw when a ledge hang (mode 0x12) began (DoJump)
    int m_ThrowWeapon;                          // 0x124 weapon thrown by the PredatorShoot event (param 1)
    float m_HitMoveScale;                       // 0x128 root-motion scale in mode 6 (UpdatePlayer)
    u8 m_HoldState;                             // 0x12c 1: holding an actor
    u8 m_Unk12d[0x130 - 0x12d];
    float m_DeadFallTimer;                      // 0x130
    struct SwingTrail {
        EXVector m_Tip;                         // weapon tip (blades: start datum) this hit check
        EXVector m_TipPrev;                     // last check's (or this one's, when not consecutive)
        EXVector m_Base;                        // blades: second datum
        EXVector m_BasePrev;
    } m_Swing[2];                               // 0x134 per hand (CheckForCharacterHit)
    unsigned int m_SwingCount[2];               // 0x1b4 m_UpdateCount at the last check per hand
    float m_WeaponDownT[4];                     // 0x1bc
    float m_WeaponDown;                         // 0x1cc
    unsigned int m_HitEventFlags;               // 0x1d0 PredatorHit event bits (DoScriptCmdEvent)
    s8 m_bAttacking;                            // 0x1d4
    u8 m_Unk1d5[0x1d8 - 0x1d5];
    int m_AttackHit;                            // 0x1d8
    int m_AttackState;                          // 0x1dc
    float m_ReactTimer;                         // 0x1e0 > 0: cannot be stumbled (ReactStumble)
    int m_Combo;                                // 0x1e4 combo found by GetComboAvailable (-1: none)
    float m_ComboTimer;                         // 0x1e8
    float m_Unk1ec;                             // 0x1ec timer, counts down (UpdatePlayer)
    float m_PlasmaCharge;                       // 0x1f0
    float m_PlasmaChargeTime;                   // 0x1f4
    int m_PlasmaSoundIdx;                       // 0x1f8 -1: none (DoUpdate)
    EXItem *m_pHeadItem;                        // 0x1fc ripped head carried (PlayerPostUpdate)
    unsigned int m_PlasmaSound;                 // 0x200 charging sound hash in m_SoundTag (DoUpdate)
    unsigned int m_VMSound;                     // 0x204 last vocal mimicry sound (SendVMMessages)
    u8 m_bDamageStandOn;                        // 0x208 damage the vehicle stood on by 25 next post-update
    s8 m_bUnk209;                               // 0x209 easy difficulty: energy refills slowly up to 20 (InitClassVars, UpdatePlayer)
    u8 m_Unk20a[0x20c - 0x20a];
    unsigned int m_Mode;                        // 0x20c
    unsigned int m_SubMode;                     // 0x210
    unsigned int m_Unk214;                      // 0x214 (DoMove: 1 skips the run setup)
    float m_MSpeed;                             // 0x218
    float m_Unk21c;                             // 0x21c depth value for (1, 6) (SetupMove)
    float m_ShimmyDir;                          // 0x220 0: left, 1: right
    float m_Unk224;                             // 0x224 set to 1 with SetupMoveBlend (UseWeaponJump)
    float m_Unk228;                             // 0x228 depth value for perch/grab moves (SetupMove)
    float m_CrouchTimer;                        // 0x22c
    float m_Unk230;                             // 0x230 depth value for (1, 3) (SetupMove)
    int m_Unk234;                               // 0x234
    s8 m_bAnimEnded;                            // 0x238
    u8 m_Unk239[0x23c - 0x239];
    unsigned int m_AnimEndParam;                // 0x23c PRED_End_Anim event parameter
    unsigned int m_MoveFlags;                   // 0x240
    float m_AnimSpeed;                          // 0x244 animation rate scale (mode 0x18/2, DoUpdate)
    float m_MoveBlend;                          // 0x248 blend of controller stack 1 (SetupMoveBlend)
    float m_Unk24c;                             // 0x24c
    float m_ReactRecover;                       // 0x250 > 0: react cut short (DoReact)
    s8 m_bIdleHitReact;                         // 0x254 play the idle hit reaction (DoIdle)
    u8 m_Unk255[0x258 - 0x255];
    int m_PickupWeapon;                         // 0x258
    float m_PickupTimer;                        // 0x25c
    float m_HitAngle;                           // 0x260 hit direction relative to facing (InflictDamage)
    s8 m_bForceModeChange;                      // 0x264
    u8 m_Unk265[0x270 - 0x265];
    EXVector m_RootMotionStart;                 // 0x270 mode 4: rotated anim motion when the move began (UpdatePlayer)
    EXVector m_RootPosStart;                    // 0x280 mode 4: item position when the move began
    int m_HandWeapon[2];                        // 0x290 weapon id held per hand
    unsigned int m_CurrentWeapon[4];            // 0x298 per slot
    int m_AttackType;                           // 0x2a8
    int m_CurrentSlot;                          // 0x2ac
    int m_PrevWeapon;                           // 0x2b0 -1: none (RestorePreviousWeapon)
    u8 m_Unk2b4[0x2b8 - 0x2b4];
    EXVector m_Unk2b8;                          // 0x2b8 zeroed with m_Speed (DeadStop)
    EXVector m_LastPos;                         // 0x2c8 position at the last PostUpdatePlayer
    EXVector m_Delta;                           // 0x2d8
    EXVector m_Speed;                           // 0x2e8
    float m_MSpeedTarget;                       // 0x2f8
    float m_IdleTime;                           // 0x2fc idle animation timer (DoIdle)
    int m_ClamberSide;                          // 0x300 toggled 0/1 per clamber step (DoClamber)
    u8 m_Unk304[0x308 - 0x304];
    float m_Unk308;                             // 0x308 (DoGeckoJumpOff)
    s8 m_bPlayerHasControl;                     // 0x30c
    u8 m_Unk30d[0x310 - 0x30d];
    unsigned int m_PredMsgFlags;                // 0x310
    s8 m_bTargetAdjust;                         // 0x314
    u8 m_Unk315[3];
    float m_TargetAdjustType;                   // 0x318
    float m_TargetAdjustTime;                   // 0x31c
    float m_TargetAngle;                        // 0x320 per-frame yaw step (AdjustTargetAdjustment)
    EXVector m_TargetAdjust;                    // 0x324
    EXVector m_TargetDelta;                     // 0x334 per-frame move step (AdjustTargetAdjustment)
    float m_Unk344;                             // 0x344 set to 12 on a jump (CheckForJump)
    float m_JumpBaseY;                          // 0x348 item height at the jump
    float m_Unk34c;                             // 0x34c wall-jump facing (yaw + pi) (SetupMove 0xa/2)
    float m_Unk350;                             // 0x350 yaw when the wall jump began
    float m_FallTimer;                          // 0x354 time off the ground (CheckForFall)
    float m_Unk358;                             // 0x358 (DoWallJump: wait while > 0)
    float m_Unk35c;                             // 0x35c (DoGeckoJumpOff sets 1)
    float m_Unk360;                             // 0x360 (DoClamber sets 10)
    s8 m_bUnk364;                               // 0x364 (DoClamber sets 1)
    s8 m_bTurnToTarget;                         // 0x365 one-shot turn toward the target (DoHitSpecial)
    u8 m_Unk366[0x368 - 0x366];
    float m_TurnTime;                           // 0x368 PRED_Special 11: turn duration (frames)
    float m_TurnRate;                           // 0x36c radians per frame
    s8 m_bUnk370;                               // 0x370 wall jump (0xa/2): turn in the air at 0.09 instead of not at all (MoveJump)
    u8 m_Unk371[0x374 - 0x371];
    int m_LockHandIdx;                          // 0x374
    EXVector m_HandLockPos;                     // 0x378
    float m_HandLock;                           // 0x388 -1: unlocked
    s8 m_bHandLockFlag;                         // 0x38c
    u8 m_Unk38d[0x390 - 0x38d];
    unsigned int m_Unk390;                      // 0x390 cleared every update (DoUpdate)
    float m_SkidTurn;                           // 0x394 yaw rate while skidding (DoSkid)
    struct PadHistory { float m_Angle; unsigned int m_Update; };
    PadHistory m_PadHistory[4];                 // 0x398 last stick angles with their m_UpdateCount (MoveNormal: skid on a fast reversal)
    s8 m_bJumpTarget;                           // 0x3b8 a platform target exists (CalculateTargetJump)
    s8 m_bJumpTargetNear;                       // 0x3b9 target within 25 (XZ)
    u8 m_Unk3ba[0x3bc - 0x3ba];
    XItem *m_pJustOffCar;                       // 0x3bc
    EXVector m_JumpTarget;                      // 0x3c0 (CalculateTargetJump)
    s8 m_bLedgeTarget;                          // 0x3d0 m_LedgeTarget is set: the jump homes onto the ledge (MoveJump)
    u8 m_Unk3d1[0x3d4 - 0x3d1];
    EXVector m_LedgeTarget;                     // 0x3d4
    float m_LedgeAngle;                         // 0x3e4
    int m_JumpTargetType;                       // 0x3e8 1 vehicle, 2 actor (pounce) (DoJump)
    void *m_pTrackTarget;                       // 0x3ec lock-on target being tracked (GetTargetPos)
    EXVector m_AimPos;                          // 0x3f0 aim point at the tracked target (UseWeapon)
    float m_TrackTime;                          // 0x400 time on the current target
    EXVector m_TrackLastPos;                    // 0x404 target position last frame
    EXQuaternion m_GeckoAngle;                  // 0x414
    EXVector m_GeckoPoint;                      // 0x424
    EXVector m_GeckoNormal;                     // 0x434 wall normal at m_GeckoPoint (CheckWallMove)
    EXVector m_GeckoPlane;                      // 0x444 plane through the wall point (A, B, C, D); a hand lock keeps m_GeckoPoint on it (DoPostUpdate)
    float m_GeckoRoll;                          // 0x454 crawl heading on the wall (DoGecko, GetGeckoAngle)
    EXQuaternion m_GeckoStartAngle;             // 0x458 pose when the crawl began (DoGecko sub 0)
    EXQuaternion m_GeckoValidAngle;             // 0x468 (GetGeckoValidPoint) copied to m_GeckoAngle; blend target
    EXVector m_GeckoStartPos;                   // 0x478 item position when the crawl began
    EXVector m_GeckoTargetPos;                  // 0x488 wall point being blended to
    float m_GeckoBlend;                         // 0x498 0..1 onto the wall (+0.2 a frame)
    u8 m_Unk49c[0x4a0 - 0x49c];
    s8 m_bCannonUp;                             // 0x4a0 shoulder cannon deployed (UpdatePlayerWeapons)
    s8 m_bCannonRaised;                         // 0x4a1 raising or up
    u8 m_Unk4a2[2];
    int m_CannonState;                          // 0x4a4 -1 idle, 0 raising, 10 lowering
    EXQuaternion m_CannonAim;                   // 0x4a8 barrel aim (slerped)
    EXQuaternion m_CannonBase;                  // 0x4b8 mount pose (slerped)
    float m_CannonTimer;                        // 0x4c8 raise/lower time left
    s8 m_bDiskInHand;                           // 0x4cc
    u8 m_Unk4cd[0x4d0 - 0x4cd];
    int m_nWeaponLoads;                         // 0x4d0
    s8 m_bWeaponsLoaded;                        // 0x4d4 (LoadWeaponsAsync)
    u8 m_Unk4d5[0x4d8 - 0x4d5];
    EXItemAnimator_Entity *m_pWeaponItem[12];   // 0x4d8
    EXFixedArray<EXExtRenderInfo, 12> m_WeaponExtRenderInfo;  // 0x508
    EXFixedArray<EXTexSwapInfo, 12> m_WeaponTexSwap;          // 0xb08
    unsigned int m_SpecialActorID;              // 0xbf8
    XItem *m_pSpecialItem;                      // 0xbfc
    XItem *m_pStandOn;                          // 0xc00 vehicle stood on (mode 0x15)
    int m_GroundAttackID;                       // 0xc04 actor ref for a ground attack (CheckForGroundAttack)
    unsigned int m_SpecialMoveAvailable;        // 0xc08
    EXVector m_ClamberPos;                      // 0xc0c
    unsigned int m_StateFlags;                  // 0xc1c TODO bits (0x10: cannot pull up)
    EXVector m_GrabStep;                        // 0xc20 grab offset per unit of m_Unk360 (HandleGrabAdjustment)
    EXVector m_GrabBase;                        // 0xc30 item position at m_Unk360 = 0
    s8 m_bUnkc40;                               // 0xc40 cleared with m_HoldState
    u8 m_Unkc41[0xc44 - 0xc41];
    int m_Cards[4];                             // 0xc44 shuffled deck (GetCardRandom)
    int m_CardIdx;                              // 0xc54
    u8 m_Unkc58[0xc5c - 0xc58];
    float m_Health;                             // 0xc5c
    float m_Energy;                             // 0xc60 cloak energy
    float m_SavedEnergy;                        // 0xc64 energy before an EMP
    s8 m_bCloak;                                // 0xc68
    u8 m_Unkc69[0xc6c - 0xc69];
    int m_CloakState;                           // 0xc6c
    u8 m_Tint;                                  // 0xc70 vision mode (HUD tint index)
    u8 m_Unkc71[0xc74 - 0xc71];
    float m_SparkTimer;                         // 0xc74
    float m_EMPTimer;                           // 0xc78
    s8 m_bSparkSkin;                            // 0xc7c
    u8 m_Unkc7d[0xc80 - 0xc7d];
    float m_CloakBreakTimer;                    // 0xc80 (UpdateCloak)
    int m_CloakBreak;                           // 0xc84
    s8 m_bCloakBroken;                          // 0xc88
    u8 m_Unkc89[0xc8c - 0xc89];
    float m_CloakRefraction;                    // 0xc8c refraction strength * 4 (EnableCloak)
    float m_RefractionBoost;                    // 0xc90 > 0: cloak refraction pulled toward 4 (UpdatePlayer)
    unsigned int m_VisionModeFlags;             // 0xc94
    s8 m_bHelmet;                               // 0xc98
    u8 m_Unkc99;
    short m_Ammo[14];                           // 0xc9a per weapon
    u8 m_Unkcb6[0xcb8 - 0xcb6];
    int m_WeaponUse[14];                        // 0xcb8 per weapon (GetFaveWeapon)
    int m_Unkcf0[8];                            // 0xcf0 weapon upgrade levels (GetWeaponUpgradeArray)
    int m_SpecialUpgrades[5];                   // 0xd10
    float m_BaseHeight;                         // 0xd24
    float m_BaseLevel;                          // 0xd28 eased foot level while clambering (UpdatePlayerWeapons)
    int m_BaseState;                            // 0xd2c 1: m_BaseLevel valid
    s8 m_bForceTrackCam;                        // 0xd30
    u8 m_Unkd31[0xd34 - 0xd31];
    float m_Unkd34;                             // 0xd34 -1 at start (InitClassVars)
    u32 m_Unkd38;                               // 0xd38
    EXVector m_Unkd3c;                          // 0xd3c
    EXVector m_ShimmyMid;                       // 0xd4c midpoint of the hands' ledge points (HandleShimmyAngle)
    s8 m_bShimmyMid;                            // 0xd5c m_ShimmyMid is set
    u8 m_Unkd5d[0xd60 - 0xd5d];
    EXVector m_AmbientColour;                   // 0xd60
    int m_FloorType;                            // 0xd70 (SetFloorType)
    EXVector m_FloorNormal;                     // 0xd74 (CheckForFall)
    u8 m_Wet;                                   // 0xd84
    s8 m_bVisionFade;                           // 0xd85 fading (UpdateVisionMode)
    u8 m_Unkd86[0xd88 - 0xd86];
    float m_VisionFade;                         // 0xd88 1: fully visible
    s8 m_bHidden;                               // 0xd8c
    u8 m_Unkd8d[0xd90 - 0xd8d];
};
inline XItemHandler_Player *XGetPlayer() { return XItemHandler_Player::GetInstance(); }
#endif
