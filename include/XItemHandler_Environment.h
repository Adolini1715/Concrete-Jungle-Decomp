#ifndef XITEMHANDLER_ENVIRONMENT_H
#define XITEMHANDLER_ENVIRONMENT_H
#include "EXItemHandler.h"
#include "EXVector.h"
#include "EXGeoTrigger.h"
#include "EXTexSwapInfo.h"
#include "EXExtRenderInfoStandIn.h"

class XItemList;

// Environment interaction item (teleports, slow-mo zones...) (FX_Environment.obj). sizeof 0x88.
class XItemHandler_Environment : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_Environment)
public:
    virtual ~XItemHandler_Environment();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    unsigned int Type() { return m_Type; }
    void AbilityEnable(unsigned int, signed char);
    XItemHandler_Environment() {}
    static XItemHandler_Environment *CreateObject();
    void SetCollided() { m_bCollided = 1; }
    EXGeoBaseDatum m_Datum;     // 0x08 trigger collision
    u32 m_Options;              // 0x38 trigger m_Unk4c
    int m_Link;                 // 0x3c trigger link 0
    s8 m_Unk40;                 // 0x40
    u8 m_Unk41[3];
    float m_Unk44;              // 0x44 starts at m_Unk5c
    unsigned int m_Type;        // 0x48 4 = slow motion, 6 = first person, 0xe/0x12 = teleports (trigger data 0)
    u32 m_Data1;                // 0x4c trigger data 1..3
    u32 m_Data2;                // 0x50
    u32 m_Data3;                // 0x54
    int m_Unk58;                // 0x58 flag channel gating the rain volume (type 2); hides the item (data 4)
    float m_Unk5c;              // 0x5c (float)data 5
    u32 m_Unk60;                // 0x60 data 6
    unsigned int m_Ability;     // 0x64 data 7 (AbilityEnable)
    int m_Unk68;                // 0x68 data 8 (1: enable the ability at creation)
    u32 m_Unk6c;                // 0x6c data 9
    int m_FlagChannel;          // 0x70 set on destruction (data 10)
    u32 m_Data11[4];            // 0x74 data 11..14
    s8 m_bCollided;             // 0x84
    s8 m_Unk85;                 // 0x85
    s8 m_bTeleportPending;      // 0x86 cleared by RemoveAllPendingTeleports
};

// Ammo pickup (FX_Environment.obj). sizeof 0xe0.
class XItemHandler_AmmoPickup : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_AmmoPickup)
public:
    virtual ~XItemHandler_AmmoPickup();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    __declspec(noinline) XItemHandler_AmmoPickup() {}
    static XItemHandler_AmmoPickup *CreateObject();
    s8 m_Unk8;                  // 0x08
    EXExtRenderInfo m_ExtRenderInfo;  // 0x0c
    EXTexSwapInfo m_TexSwap;    // 0x8c
    u32 m_Options;              // 0xa0 trigger m_Unk4c (bit 0: no glow)
    EXGeoBaseDatum m_Datum;     // 0xa4 trigger collision
    u32 m_Data[3];              // 0xd4 trigger data 0..2 (2: show flag channel)
};

// Pressure plate that sets a flag channel (FX_Environment.obj). sizeof 0x50.
class XItemHandler_Touchplate : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_Touchplate)
public:
    virtual ~XItemHandler_Touchplate();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    XItemHandler_Touchplate() {}
    static XItemHandler_Touchplate *CreateObject();
    float m_Timer;              // 0x08
    EXGeoBaseDatum m_Datum;     // 0x0c copied from the trigger collision
    int m_SetChannel;           // 0x3c
    int m_RequireChannel;       // 0x40
    int m_KillChannel;          // 0x44
    int m_OnTime;               // 0x48
    int m_OffTime;              // 0x4c
};

// Weapon upgrade pickup (FX_Environment.obj). sizeof 0x64.
class XItemHandler_Upgrades : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_Upgrades)
public:
    virtual ~XItemHandler_Upgrades();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    XItemHandler_Upgrades() {}
    static XItemHandler_Upgrades *CreateObject();
    s8 m_bActive;               // 0x08
    u8 m_Unk9[0xc - 0x9];
    EXGeoBaseDatum m_Datum;     // 0x0c copied from the trigger collision
    u32 m_Data[10];             // 0x3c trigger data 0..9 (m_Data[8] = level weapon upgrade)
};

// Laser beam trigger (FX_Environment.obj). sizeof 0x4c.
class XItemHandler_BeamTrigger : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_BeamTrigger)
public:
    virtual ~XItemHandler_BeamTrigger();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    XItemHandler_BeamTrigger() {}
    static XItemHandler_BeamTrigger *CreateObject();
    u8 m_Unk8[0x38 - 0x8];      // TODO layout
    u32 m_Options;              // 0x38 trigger m_Unk4c
    u32 m_Data[4];              // 0x3c trigger data 0..3
};

extern signed char Tethys;
extern XItemList gEnvList;
extern XItemList gPickupList;
signed char IsEnvironment(EXItem *pItem);
void RemoveAllPendingTeleports();
#endif
