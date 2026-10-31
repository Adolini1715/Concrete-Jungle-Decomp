#ifndef TRACERFIRE_H
#define TRACERFIRE_H
#include "types.h"
#include "EXVector.h"
#include "RGBA.h"

class EXGeoFile;

// Bullet tracer (ActorCarry.obj). sizeof 0x48. Pool of 200 in TracerFireManager.
class TracerFire {
public:
    signed char IsFree() const { return m_bFree == 1; }
    signed char DrawMe() const { return m_bDrawMe == 1; }
    void SetFree(signed char b) { m_bFree = b != 0; }
    void Reset();
    void Init(EXVector start, const EXVector &dir, int weaponIndex, int d);
    void Update();
    void Draw(EXGeoFile *pFile) const;
    TracerFire();
protected:
    void DrawSection(EXVector *p, int a, int b, int c, int d, ex::RGBA col) const;
public:
    EXVector m_Start;           // 0x00
    EXVector m_End;             // 0x10
    float m_Unk20;              // 0x20
    ex::RGBA m_Colour;          // 0x24
    float m_Width;              // 0x28 weapon m_TracerWidth
    float m_Dist;               // 0x2c distance travelled
    float m_Length;             // 0x30 weapon m_TracerLength
    float m_Speed;              // 0x34 weapon projectile speed
    float m_MaxDist;            // 0x38 range + length
    int m_WeaponIndex;          // 0x3c
    int m_Unk40;                // 0x40 TODO (Init param 4)
    u32 m_bFree : 1;            // 0x44 bit 0
    u32 m_bDrawMe : 1;          //      bit 1
    u32 m_bUnk4 : 1;            //      bit 2 TODO (cleared by Init)
};

// Static tracer pool (ActorCarry.obj).
class TracerFireManager {
public:
    static TracerFire *GetFreeTracer();
    static void FreeTracer(TracerFire *p);
    static void DrawTracers();
protected:
    static TracerFire ms_aoTracers[200];
    static unsigned int ms_ulAllocated;
};
#endif
