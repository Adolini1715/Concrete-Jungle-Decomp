#ifndef EXGEOTRIGGER_H
#define EXGEOTRIGGER_H
#include "types.h"
#include "EXVector.h"
#include "EXRelPtr.h"

// TODO stand-in: map trigger records as stored in the .edb (engine geo area). Layouts from EXTrigger::Init.

// Trigger type table entry (16 bytes).
struct EXGeoTriggerType {
    u32 m_Hash;
    u8 m_04[0x0c];
};

// Trigger position: a vector with a user copy constructor, which makes datum copies memberwise
// (Within() callers in NightAndDay.obj copy the volume field by field).
class EXGeoTriggerPos : public EXVector3 {
public:
    EXGeoTriggerPos() {}
    EXGeoTriggerPos(const EXGeoTriggerPos &v) { x = v.x; y = v.y; z = v.z; }
    EXGeoTriggerPos &operator=(const EXGeoTriggerPos &v) { x = v.x; y = v.y; z = v.z; return *this; }
};

// Datum record of a geo file (0x30 bytes). Its implicit copy constructor is memberwise
// (emitted in Actor.obj) because of m_Pos.
struct EXGeoBaseDatum {
    // Memberwise, but never inlined in retail (called from Trigger.obj; copy emitted in ActorLocation.obj).
    __declspec(noinline) EXGeoBaseDatum &operator=(const EXGeoBaseDatum &d)
    {
        m_Type = d.m_Type;
        m_04 = d.m_04;
        m_06 = d.m_06;
        m_07 = d.m_07;
        m_08 = d.m_08;
        m_Pos = d.m_Pos;
        m_20 = d.m_20;
        return *this;
    }
    u32 m_Type;             // 0x00
    u16 m_04;               // 0x04
    u8 m_06;                // 0x06
    u8 m_07;                // 0x07
    EXVector3 m_08;         // 0x08
    EXGeoTriggerPos m_Pos;  // 0x14 relative to the trigger
    EXMaths::vec_xyzw m_20; // 0x20
};

// Trigger collision volume.
struct EXGeoTriggerDatum : public EXGeoBaseDatum {};

// The same 0x30-byte record as copied by the mission trigger creators (Trigger.obj): its implicit
// assignment is inlined there, unlike EXGeoBaseDatum's. TODO real name.
struct EXGeoTriggerVolume {
    u32 m_Type;             // 0x00
    u16 m_04;               // 0x04
    u8 m_06;                // 0x06
    u8 m_07;                // 0x07
    EXVector3 m_08;         // 0x08
    EXGeoTriggerPos m_Pos;  // 0x14
    EXMaths::vec_xyzw m_20; // 0x20
};

// Trigger tables of a map (at EXGeoMap +0x58).
// TODO layout: trigger script header (8 bytes, EXGeoMap::TriggerScripts index scale).
struct EXGeoTrigScriptHeader {
    u32 m_00;
    u32 m_04;
};
struct EXGeoTriggerInfo {
    u8 m_00[0x08];
    ex::EXRelPtr<EXGeoTrigScriptHeader> m_Scripts;  // 0x08 (EXItemEnv.obj)
    ex::EXRelPtr<EXGeoTriggerType> m_Types;         // 0x0c
    ex::EXRelPtr<EXGeoTriggerDatum> m_Collisions;   // 0x10
};

// One placed trigger; optional fields follow at m_Data as flagged by m_Flags.
struct EXGeoTrigger {
    u16 m_TypeIndex;        // 0x00
    u16 m_02;
    u32 m_04;               // 0x04
    u32 m_Flags;            // 0x08
    EXVector3 m_Pos;        // 0x0c
    EXVector3 m_Rot;        // 0x18
    EXVector3 m_Scale;      // 0x24
    u32 m_Data[1];          // 0x30
};
#endif
