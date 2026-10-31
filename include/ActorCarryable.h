#ifndef ACTORCARRYABLE_H
#define ACTORCARRYABLE_H
#include "types.h"
#include "EXList.h"
#include "EXVector.h"
#include "SoundTag.h"

class EXItemAnimator_Entity;
class EXItemAnimator_Script;
class EXExtRenderInfo;
class WeaponProfile;
class XItem;
class Actor;
class TracerFire;

enum CarryType {
    CARRY_NONE = 0,
    CARRY_PAPER = 1,
    CARRY_FOOD = 2,
    CARRY_BOX = 5,
    CARRY_WEAPON = 6,
    CARRY_DRINK = 7,
    CARRY_BAG = 8,      // TODO name: what muggers go for (STATE_FS_Mugger)
    CARRY_GUN = 9,      // created as an ActorCarryableGun
    CARRY_PHONE = 10,
    CARRY_CIGARETTE = 11
    // (Actor::UpdateAnimType)
};

// Item carried by an actor (ActorCarry.obj). sizeof 0x88. Up to 256 live, indexed by ref id & 0xff.
class ActorCarryable {
public:
    virtual ~ActorCarryable();
    virtual void SetOwner(int id);
    virtual signed char DoUpdate();
    virtual void StopLoopingSFX();
    virtual void DumpDebugInfo() const;
    int GetEntityHash() const { return m_EntityHash; }
    int GetRefID() const { return m_RefID; }
    CarryType GetType() const { return m_Type; }
    int GetDescription() const { return m_Description; }
    EXItemAnimator_Entity *GetAnimator() const { return m_pAnimator; }
    signed char IsAGun() const { return m_bGun; }
    static ActorCarryable *GetFirst()
    {
        EXDListItem *p = gActorCarryableList.Head();
        return p ? (ActorCarryable *)((char *)p - 4) : 0;
    }
    ActorCarryable *GetNext()
    {
        EXDListItem *p = m_Link.Next();
        return p ? (ActorCarryable *)((char *)p - 4) : 0;
    }
    signed char NeedsDelete()
    {
        if (m_bDelete) {
            if (m_DeleteTimer-- <= 0) return 1;
        }
        return 0;
    }
    static ActorCarryable *GetPtrFromID(int id);
    static void BootInit();
    void SetHide(signed char b);
    EXVector GetWorldPos();
    void Delete();
    static void Update();
    static void PreLoadInit();
    static CarryType FindCarryTypeForEntity(int entity);
    static unsigned int FindTextForEntity(int entity);
    WeaponProfile *GetWeaponProfile() const;
    ActorCarryable(int file, int entity);
    signed char GetWeaponCollisionPos(EXVector &pos) const;
    static ActorCarryable *Create(int file, int entity);
    int GetOwner() const { return m_Owner; }
protected:
    void UpdateFall();
    void UpdateAlpha();
    void UpdateDistFade();
    void UpdateTimeOut();
    static EXExtRenderInfo mExtRenderInfo;
    static EXDList gActorCarryableList;
public:
    EXDListItem m_Link;                 // 0x04 gActorCarryableList
    int m_RefID;                        // 0x0c
    s8 m_bDelete;                       // 0x10
    int m_DeleteTimer;                  // 0x14 frames left before delete
    s8 m_bLoaded;                       // 0x18 geometry loaded, animator initialised
    u8 m_Pad19[3];
    float m_Unk1c;                      // 0x1c TODO (0 at creation)
    float m_Unk20;                      // 0x20 TODO
    float m_Unk24;                      // 0x24 TODO (1 at creation)
    int m_Owner;                        // 0x28 actor id
    int m_Unk2c;                        // 0x2c TODO (150 at creation and on every change of owner)
    int m_Unk30;                        // 0x30 TODO (150 at creation)
    int m_EntityHash;                   // 0x34
    int m_GeoHash;                      // 0x38 (GeoSectionManager::Del with m_EntityHash)
    float m_FloorY;                     // 0x3c floor height under a dropped item (SetOwner)
    float m_Unk40;                      // 0x40 TODO (0 when dropped)
    CarryType m_Type;                   // 0x44
    s8 m_bHide;                         // 0x48
    s8 m_bFading;                       // 0x49
    u8 m_Unk4a[0x4c - 0x4a];
    float m_FadeTime;                   // 0x4c deleted once <= 0
    EXItemAnimator_Entity *m_pAnimator; // 0x50
    XItem *m_pItem;                     // 0x54
    u8 m_Unk58[0x78 - 0x58];
    int m_Unk78;                        // 0x78 HT_File_DRONE_Gen section held for this carryable (carry type table column 3)
    int m_Description;                  // 0x7c
    s8 m_bGun;                          // 0x80 an ActorCarryableGun
    int m_WeaponIndex;                  // 0x84 WeaponProfile index, -1 none
};

// A carried gun (ActorCarry.obj). sizeof 0xe4.
class ActorCarryableGun : public ActorCarryable {
public:
    virtual ~ActorCarryableGun();
    virtual void SetOwner(int id);
    virtual signed char DoUpdate();
    virtual void StopLoopingSFX();
    virtual void DumpDebugInfo() const;
    SoundTag *BeamSoundTag() { return &m_BeamSoundTag; }
    void ChangeProfile(int id);
    signed char GetMuzzleDatumCorrect(EXVector &pos, EXVector &dir);
    void GetMuzzleDatum(EXVector &pos, EXVector &dir);
    void DelayWait(float t);
    static void ResetBulletsPerFrame();
    void ChangeMode();
    void ExtChangeMode();
    void PauseFiring();
    void StopFiring();
    void UpdateWait();
    signed char WaitOver() const;
    signed char CanFire() const;
    signed char StartFiring();
    void Reload();
    void RefillClipInstantly();
    void SetInitialWait();
    EXVector GetMuzzleWorldPos();
    signed char IsSniper() const;
    int GetLocationType() const;
    ActorCarryableGun(int file, int entity);
    void DeleteSceneryImpactScript();
    void UpdateSceneryImpactScript();
    void UpdateTracers();
    int GetFreeTracer();
    void DeleteTracers();
    void DeleteMuzzleScript();
    signed char GetEndPoint(WeaponProfile *p, EXVector const &a, EXVector const &b, EXVector &out);
    void PlayMuzzleScript(int a);
    void PlaySceneryImpactScript(int a, EXVector const &pos);
    void CreateTracer(EXVector const &a, EXVector const &b);
    signed char Fire(Actor &actor, EXVector const &target, signed char b, int c);
    signed char UpdateFiring(Actor &actor, EXVector const &target, int a, int b);
    void SetWait(float t) { m_Wait = t; }
    void ForceRoundToEnd() { m_RoundTime = 0.0f; }
    static int ms_iInstantBulletsThisFrame;
protected:
    void UpdateMuzzleScript();
public:
    EXItemAnimator_Script *m_pMuzzleScript;     // 0x88
    EXItemAnimator_Script *m_pImpactScript;     // 0x8c
    XItem *m_pImpactScript2;                    // 0x90 world item holding m_pImpactScript
    int m_NumTracers;                           // 0x94
    TracerFire **m_ppTracers;                   // 0x98
    SoundTag m_SoundTag;                        // 0x9c
    SoundTag m_BeamSoundTag;                    // 0xa4
    s8 m_bFirstRound;                           // 0xac first beam round of a burst plays the effects
    u8 m_Padad[3];
    int m_Mode;                                 // 0xb0 4 = waiting, 7 = paused
    int m_PrevMode;                             // 0xb4
    EXVector m_FirePos;                         // 0xb8 owner position while firing a burst (he is held there)
    float m_PauseTime;                          // 0xc8
    float m_Unkcc;                              // 0xcc
    float m_RoundTime;                          // 0xd0
    float m_Wait;                               // 0xd4
    u32 m_bMuzzleLooping : 1;                   // 0xd8 bit 0: looping muzzle script started (freeze throwers)
    s16 m_Clip;                                 // 0xdc
    s16 m_BurstLeft;                            // 0xde rounds left in this burst
    int m_LocationType;                         // 0xe0
};
// Weapon entity hash a profile names to the one actually carried (ActorCarry.obj).
int TranslateCarryHashCode(int hash);
#endif
