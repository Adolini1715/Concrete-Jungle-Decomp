#ifndef EXTRIGGER_H
#define EXTRIGGER_H
#include "EXBaseTrigger.h"
#include "EXRuntimeClass.h"
#include "EXVector.h"
#include "EXMapOn.h"
#include "RGBA.h"

struct EXGeoTrigger;
class EXGeoMap;

// Collision volume copied from the map's trigger collision table (0x30 bytes).
struct EXTriggerCollision {
    u32 m_Type;             // +0x00
    u16 m_Unk04;            // +0x04
    u8 m_Unk06;             // +0x06 (0xff when none)
    u8 m_Unk07;             // +0x07
    EXVector3 m_Unk08;      // +0x08
    EXVector3 m_Pos;        // +0x14 (trigger position added)
    EXMaths::vec_xyzw m_Unk20; // +0x20
};

// Placed trigger instance (Trigger.obj). sizeof 0xf0.
class EXTrigger : public EXBaseTrigger {
public:
    virtual ~EXTrigger();
    EX_DECLARE_DYNAMIC(EXTrigger)
public:
    EXTrigger();
    EXTrigger(EXGeoTrigger *pGeo, EXGeoMap *pMap, EXMapOn mapOn, unsigned short index);
    void Init(EXGeoTrigger *pGeo, EXGeoMap *pMap, EXMapOn mapOn, unsigned short index);
    static EXTrigger *CreateObject();
    s8 TrigFlag(unsigned int f) const { return (m_Flags & f) > 0; }
    float DataF(int i) const { return *(const float *)&m_Data[i]; }

    EXVector m_Position;            // 0x0c
    EXVector m_Rotation;            // 0x1c
    EXVector m_Scale;               // 0x2c
    EXMapOn m_MapOn;                // 0x3c
    u16 m_Unk3e;
    u32 m_Id;                       // 0x40 (map hash & 0xfff) << 16 | index
    u32 m_Type;                     // 0x44 trigger type hash
    u32 m_Flags;                    // 0x48 which optional fields are present
    u32 m_Unk4c;                    // 0x4c
    u32 m_Unk50;                    // 0x50
    u32 m_Hash;                     // 0x54
    u32 m_Data[16];                 // 0x58 flags 0x1..0x8000
    int m_Links[8];                 // 0x98 flags 0x10000.. (trigger index in map, -1 none)
    EXTriggerCollision m_Collision; // 0xb8 flag 0x8000000
    ex::RGBA m_Colour;              // 0xe8 flag 0x10000000 (default 80 80 80 ff)
    int m_NavPoly;                  // 0xec
};
#endif
