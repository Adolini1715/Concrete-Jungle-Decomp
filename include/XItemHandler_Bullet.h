#ifndef XITEMHANDLER_BULLET_H
#define XITEMHANDLER_BULLET_H
#include "EXItemHandler.h"
#include "EXSoundTagStandIn.h"
#include "EXVector.h"

class XItem;
class EXEffectsSys;
class WeaponProfile;
class EXItemAnimator_Entity;

// Projectile item handler (Bullet.obj). sizeof 0x80.
class XItemHandler_Bullet : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_Bullet)
public:
    virtual ~XItemHandler_Bullet();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    XItemHandler_Bullet();
    static XItemHandler_Bullet *CreateObject();
    signed char IsMine() { return m_Type == 2; }
    void SetCountdown(short n) { if (m_Countdown < 0) m_Countdown = n; }
    unsigned int WeaponHash() { return m_WeaponHash; }
    void SetHideChannel(short);
    void Explode();
    void Bounce(EXVector &);
    static signed char InstantCreate(WeaponProfile *, EXVector const &, EXVector const &, EXVector const &, unsigned int, signed char, float, float, EXVector *, signed char *);
    static XItem *Create2(EXVector &, EXVector &, EXVector &, unsigned short, unsigned int, signed char, unsigned int, int, float);
    static XItem *Create(EXVector &, EXVector &, EXVector &, unsigned short, signed char, unsigned int, int, float);
    static XItem *CreateFromHash(EXVector &, EXVector &, EXVector &, unsigned int, signed char, unsigned int, int, float);
protected:
    void Update_SmartDisk();
    void Update_Returning();
    void Update_StaticTimeout();
    void Update_TwoStage();
    float CalcDamage();
    float CalcRange();
    float CalcRadius();
    void Update_Gravity();
    void Update_Mine();
    void Update_Homing();
public:
    u16 m_PredWeapon;           // 0x08 (Explosion_Create argument)
    u16 m_Type;                 // 0x0a 2 = mine
    s16 m_Targets[4];           // 0x0c actor ids: the stuck/locked-on target first (smart disk: up to 4 camera locks)
    float m_Speed;              // 0x14
    EXVector m_Velocity;        // 0x18
    float m_Life;               // 0x28 distance flown (DoUpdate range check); stuck spears: time left (Update_StaticTimeout fades the last second)
    float m_PinTime;            // 0x2c spear stuck in a rag doll: distance dragged (lets go past 30)
    u8 m_Unk30;                 // 0x30 cleared when a spear sticks into an actor
    u8 m_Pad31[3];
    int m_Bounces;              // 0x34 smart disc ricochets off scenery (up to 5)
    u32 m_WeaponHash;           // 0x38
    s8 m_bFromPlayer;           // 0x3c fired by the player: his own mines ignore him
    u8 m_Pad3d[3];
    int m_OwnerID;              // 0x40 actor that fired it (a hunter's smart disc returns to him)
    EXVector m_StageTarget;     // 0x44 two-stage weapons: where the first stage heads

    EXSoundTag m_SoundTag;      // 0x54
    EXEffectsSys *m_pSwoosh;    // 0x5c
    float m_Range;              // 0x60 plasma bolts: distance to the locked target (100000 otherwise)
    s16 m_Countdown;            // 0x64
    u16 m_HideChannel;          // 0x66 hidden until this flag channel is set (SetHideChannel)
    int m_PathIndex;            // 0x68 hover mines: map path followed (-1 none)
    int m_PathNode;             // 0x6c current node
    float m_PathT;              // 0x70 spline parameter between m_PathNode and the next node
    int m_GeoSectionGeo;        // 0x74 GeoSectionManager::Del arguments
    int m_GeoSectionItem;       // 0x78
    EXItemAnimator_Entity *m_pTriggerBall;  // 0x7c armed mine: trigger radius sphere; spear: the head it carries off
};

// Clears all mines (Bullet.obj). sizeof 0xc.
class XItemHandler_Minesweeper : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_Minesweeper)
public:
    virtual ~XItemHandler_Minesweeper();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    XItemHandler_Minesweeper() {}
    static XItemHandler_Minesweeper *CreateObject();
    int m_FlagChannel;          // 0x08
};
#endif
