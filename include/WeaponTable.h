#ifndef WEAPONTABLE_H
#define WEAPONTABLE_H
#include "types.h"

// One row of the weapons worksheet (sizeof 0xe8). Init() converts the timing columns from seconds
// (floats in the sheet) to frames in place.
class WeaponProfile {
public:
    void LockWeapon() { m_LockTimer = (float)m_LockFrames; }
    static void Init();
    static void RefreshBasePointer();
    static WeaponProfile *GetWeaponDataFromEntity(int entity);
    static WeaponProfile *GetWeaponData(int id);
    static int GetWeaponDataIndexFromEntity(int entity);
    static int GetWeaponDataIndex(int id);
    static WeaponProfile *GetWeaponDataFromIndex(int index);
    static int GetWeaponryHashFromPredWeapon(int index);
    float GetMeleeDamage() const;
    float GetBulletDamage() const;
    float GetDuration() const;
    float GetWait() const;
    static void Update();
    signed char IsPredatorProjectileWeapon() const;
    int GetSpecialAnimHash() const;
    float GetMineTriggerRadius() const;
    static int GetWeaponryHashFromPredWeapon_Safe(int index);

    int m_Id;                   // 0x00 HT_Weaponry_*
    u32 m_MapHash;              // 0x04 0 = any map
    int m_Entity;               // 0x08
    int m_CarryEntity;          // 0x0c entity shown in the hand (0 = use the carried object; Actor::GetScanInfo)
    int m_AimType;              // 0x10 HT_ aim class 0x52000004..7 (ActorCombat.obj Actor_GetAimAnimsSet)
    int m_SfxFire;              // 0x14 shot sound (ActorCombat.obj RussianBossPredAttack) TODO name
    u32 m_ReloadSound;          // 0x18 (ActorCarry.obj Reload)
    u32 m_ImpactSound;          // 0x1c tracer scenery hit (ActorCarry.obj TracerFire::Update)
    int m_SfxHitFixed;          // 0x20 melee hit sound when m_MeleeDamage > 0 (ActorCombat.obj StrikeCharacter)
    int m_SfxHit;               // 0x24 melee hit sound otherwise
    u32 m_SfxFireEnd;           // 0x28 played when a burst or beam ends (ActorCarry.obj UpdateFiring)
    u32 m_FlightSound;          // 0x2c looped while the projectile flies (Bullet.obj Create2)
    u32 m_SfxFireStart;         // 0x30 beam spin-up (ActorCarry.obj UpdateFiring)
    int m_DurationMin;          // 0x34 frames (seconds in the sheet)
    int m_DurationMax;          // 0x38
    int m_WaitMin;              // 0x3c
    int m_WaitMax;              // 0x40
    int m_Rate;                 // 0x44 frames per shot (shots per second in the sheet)
    float m_RangeMin;           // 0x48 preferred minimum distance (ActorCombat.obj MonsterSquad_Freeze; MafiaBoss POI radius)
    float m_RangeMax;           // 0x4c preferred maximum distance (ActorCombat.obj MonsterSquad_Freeze)
    int m_ClipSize;             // 0x50 (ActorCarry.obj Reload; Actor.obj PlayActorWeaponImpactSFX reads a dword)
    int m_Burst;                // 0x54 rounds per burst (at least 1)
    int m_IgnoreArmour;         // 0x58 TODO name: non-zero skips the ArmourProfile scale (ActorUtil.obj AU_HandleActorAttacks)
    float m_MeleeDamage;        // 0x5c fixed damage when > 0
    float m_DamageMin;          // 0x60
    float m_DamageMax;          // 0x64
    float m_ThrowSpeed;         // 0x68 launch speed for thrown weapons (ActorCombat.obj ThrowGrenade)
    u8 m_ExplosionType;         // 0x6c (Bullet.obj Explode)
    u8 m_Unk6d[0x70 - 0x6d];
    float m_MineTriggerRadius;  // 0x70
    float m_AttackRadius;       // 0x74 (Player.obj SendAttackMessage)
    int m_FireType;             // 0x78 0 = instant bullets with tracers, 1 = beam (also single shot, ActorCarry.obj StartFiring), else projectiles
    int m_IgnoresCloak;         // 0x7c homing rounds track the cloaked player itself, not where he was last seen (Bullet.obj Update_Homing)
    float m_TracerLength;       // 0x80 tracer streak length; tracers stop this far short of the range (ActorCarry.obj TracerFire::Init)
    int m_TracerCount;          // 0x84 tracers a gun keeps in flight (ActorCarryableGun ctor)
    int m_TracerR;              // 0x88 tracer colour
    int m_TracerG;              // 0x8c
    int m_TracerB;              // 0x90
    int m_TracerA;              // 0x94
    float m_TracerWidth;        // 0x98 (TracerFire m_Width)
    float m_Range;              // 0x9c (Bullet.obj CalcRange)
    float m_Accuracy;           // 0xa0 percent; 100 = no spread (ActorCombat.obj ThrowGrenade)
    int m_Tracers;              // 0xa4 tracer texture hash; nonzero: draws tracers (ActorCarry.obj CreateTracer, TracerFire::Draw)
    int m_NumStrikes;           // 0xa8 melee combo length (ActorCombat.obj GetStrikeAnim: last strike uses a special anim)
    int m_SpecialAnims[3];      // 0xac
    int m_NumSpecialAnims;      // 0xb8
    float m_MeleeRange;         // 0xbc strike reach (ActorCombat.obj STATE_QuickEvadePlayerWithGunButt)
    float m_DodgeRadius;        // 0xc0 incoming attacks beyond this are ignored (ActorCombat.obj Actor_CheckForAttack)
    float m_NoiseRadius;        // 0xc4 loud noise radius on impact (Bullet.obj DoUpdate)
    int m_DamageClass;          // 0xc8 HT_DamageClass_Health / _Energy / _HealthEnergy (melee: push / stun / both, ActorCombat.obj StrikeCharacter)
    u32 m_ProjectileEntity;     // 0xcc projectile animator (Bullet.obj Create2); two-stage rounds replace it when stage two starts
    u32 m_Stage2Animator;       // 0xd0 its replacement, created from the same geo file
    u32 m_ExplosionScript;      // 0xd4 (Bullet.obj Explode)
    u32 m_Swoosh;               // 0xd8 projectile trail swoosh (Bullet.obj Create2)
    u32 m_MuzzleScript;         // 0xdc muzzle flash effect on the right hand (Player.obj ShootWeapon)
    int m_LockFrames;           // 0xe0 frames (seconds in the sheet)
    float m_LockTimer;          // 0xe4
private:
    static WeaponProfile *ms_pWeaponTable;
    static int ms_iWeaponTableCount;
};

// One row of the armour worksheet (sizeof 0x38): percentages turned into multipliers.
class ArmourProfile {
public:
    static void Init();
    static void RefreshBasePointer();
    static ArmourProfile *GetArmourData(int id);
    static int GetArmourDataIndex(int id);
    static ArmourProfile *GetArmourDataFromIndex(int index);

    int m_Id;               // 0x00
    u32 m_Description;      // 0x04 scan text
    float m_Damage[12];     // 0x08 (100 - percent) / 100
private:
    static ArmourProfile *ms_pArmourTable;
    static int ms_iArmourTableCount;
};
#endif
