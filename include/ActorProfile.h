#ifndef ACTORPROFILE_H
#define ACTORPROFILE_H
#include "types.h"
#include "StateMachine.h"

class Actor;

// Index of a profile: worksheet (0 global, 1 current map) and row. 0xffff = none.
struct ActorProfileIndex {
    u16 m_Sheet;
    u16 m_Row;
};
struct ActorSkinIndex {
    u16 m_Sheet;
    u16 m_Row;
};

// Flag channel reference in a profile (0 <= m_Channel < 290).
struct ActorProfileChannel {
    int m_Channel;  // flag channel watched
    int m_State;    // state hash to switch to when it is set
};

// One row of the actor profile worksheets (HT_SpreadSheet 0x14000003, sizeof 0x14c).
struct ActorProfile {
    int m_Hash;                         // 0x00 profile hash
    int m_Group;                        // 0x04 HT_ActorGroup_* (0x4b000001.. 12 groups)
    int m_SubGroup;                     // 0x08 HT_ActorSubGroup_*
    int m_StartAnim;                    // 0x0c non-zero: start in STATE_PlayStartAnim
    int m_StartState;                   // 0x10 HT_ActorState_*
    int m_IdleState;                    // 0x14 default HT_ActorState_DefaultIdle
    int m_UnderAttackState;             // 0x18 state hash Gosub'd on MSG_SOMEONE_UNDER_ATTACK (0: ignore)
    int m_LoudNoiseState;               // 0x1c state hash Gosub'd on MSG_LOUD_NOISE
    int m_AttractState;                 // 0x20 state hash Gosub'd on MSG_ATTRACT (0: ignore)
    int m_FriendsInCombatState;         // 0x24 state hash Goto'd when no friends are left fighting (CheckForFriendsInCombat)
    int m_FriendsInCombatCount;         // 0x28 friends fighting the predator before the actor joins in
    int m_DistanceTrigger;              // 0x2c boss: state hash Gosub'd when the player walks away beyond m_Dist
    float m_Dist;                       // 0x30 squared on load
    int m_LowHealthTrigger;             // 0x34 boss: message when health drops below m_GunSwapHealthPct
    float m_GunSwapHealthPct;           // 0x38 percent of m_Health (ActorCarry.obj ChangeMode)
    int m_TimeTrigger;                  // 0x3c boss: state hash Gosub'd every m_TriggerTime seconds
    int m_TriggerTime;                  // 0x40 seconds
    int m_LargeDamageTrigger;           // 0x44 boss: message on one hit of at least m_LargeDamagePct
    float m_LargeDamagePct;             // 0x48 percent of m_Health
    int m_NudgeType;                    // 0x4c 0x50000001: notice the player within 5.5, 0x50000002: within 8
    u32 m_SeeAnyHeight;                 // 0x50 1: no eye height limit (Actor::CanSeePoint)
    int m_Armour;                       // 0x54 ArmourProfile id (ActorUtil.obj AU_HandlePlayerAttacks)
    float m_Health;                     // 0x58 starting health
    float m_AttackRange;                // 0x5c TODO name: distance that starts an attack (ActorCombat.obj LookAroundAlerted)
    int m_DodgeChance;                  // 0x60 percent (ActorUtil.obj AU_HandleIncomingAttack: Rand32() % 101 <= it)
    int m_Honour;                       // 0x64
    float m_FightDist;                  // 0x68
    float m_FormationRadius;            // 0x6c combat-circle wobble radius (CFormation::Update) TODO name
    float m_AlertTime;                  // 0x70
    float m_Unk74;                      // 0x74 alarm fame (Objective_NotifyPlayerSeenByAirVehicle)
    int m_Weapon;                       // 0x78 HT_Entity_Weapon_* (fist = none)
    int m_Weapon2;                      // 0x7c
    int m_ScanDescription;              // 0x80 scan text (0 = the armour's)
    int m_ScanText84;                   // 0x84 TODO name: scan page 1 text
    int m_ScanText88;                   // 0x88 TODO name: scan page 2 text
    int m_ContextIcon;                  // 0x8c default context icon of the actor
    int m_LastSkin;                     // 0x90 index into m_Skins
    int m_Skins[6];                     // 0x94 skin hashes (0 = unused)
    ActorProfileChannel m_Channels[5];  // 0xac
    int m_POIWeights[30];               // 0xd4 weights of the FSAttribs POI types
};

// One row of the actor skin worksheet (HT_SpreadSheet 0x14000005, sizeof 0x54).
struct ActorSkin {
    int m_Hash;             // 0x00
    unsigned int m_File;    // 0x04 texture file hash
    float m_Height;         // 0x08 TODO name (VehicleMove scales it by 1.8)
    int m_SkelHash;         // 0x0c
    int m_SkelFile;         // 0x10 geo file of m_SkelHash (Actor::Update)
    int m_ChangeHash;       // 0x14 second geo section {0x1c file, 0x14 item} freed by DeleteChangeSkin
    int m_StartAnimMode;    // 0x18 anim mode set when added to the world
    int m_ChangeFile;       // 0x1c
    int m_Head;             // 0x20 severed limb models in m_LimbFile (0 = none)
    int m_LeftArm;          // 0x24
    int m_RightArm;         // 0x28
    int m_CanLoseBelt;      // 0x2c TODO name: non-zero lets AR_HitStomach blow off ALL_BELT
    int m_LimbFile;         // 0x30 geo file of the severed limb models
    int m_VoiceType;        // 0x34 selects warcry/death/hit SFX sets
    int m_Type;             // 0x38
    int m_VOType;           // 0x3c selects attack/nudge VO sets
    u8 m_Unk40[0x44 - 0x40];
    int m_BloodType;        // 0x44 (FX_Gore.obj Gore_GetBloodGroup)
    int m_ScanScript[3];    // 0x48 script per scan page (HT_Script_*)
};

StateFn AP_GetStateFromHash(int hash);
ActorProfile *AP_GetPtr(int hash);
void AP_GetIndex(int hash, ActorProfileIndex &idx);
void AS_GetIndex(int hash, ActorSkinIndex &idx);
ActorProfile *AP_GetPtrFromIndex(const ActorProfileIndex &idx);
ActorSkin *AS_GetPtrFromIndex(const ActorSkinIndex &idx);
StateFn AP_GetStartState(ActorProfile *p);
signed char AP_DoWeFight(const ActorProfile *a, const ActorProfile *b);
int AP_GetRandSkinHash(ActorProfile *p);
void AP_RefreshPointer();
void AP_PostLoadInit();
ActorSkin *AS_GetPtr(int hash);
ActorSkin *AS_GetIndexPtr(int i);
void AS_RefreshPointer();
void AS_PostLoadInit();
void AP_GetRandPOI(Actor *pActor, int &poi, int &b);
#endif
