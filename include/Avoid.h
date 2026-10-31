#ifndef AVOID_H
#define AVOID_H
#include "types.h"
#include "EXVector.h"

class Actor;

// Local avoidance (Avoid.obj): swept spheres for two actors, steering around each other.

// A moving sphere (sizeof 0x28).
class CAvoidSphere {
public:
    virtual ~CAvoidSphere();
    EXVector &MoveVec() { return m_MoveVec; }
    EXVector &Pos() { return m_Pos; }
    float &Radius() { return m_Radius; }
    CAvoidSphere();
    CAvoidSphere(const EXVector &pos, const EXVector &prev, float radius);
    EXVector m_Pos;         // 0x04
    EXVector m_MoveVec;     // 0x14
    float m_Radius;         // 0x24
};

// An actor as an obstacle (sizeof 0x58).
class CObstacle {
public:
    unsigned int &Flags() { return m_Flags; }
    EXVector &Pos() { return m_Sphere.m_Pos; }
    float &Ang() { return m_Ang; }
    float &DestAng() { return m_DestAng; }
    EXVector &MoveVec() { return m_Sphere.m_MoveVec; }
    float &Radius() { return m_Sphere.m_Radius; }
    CAvoidSphere &Sphere() { return m_Sphere; }
    float &Velocity() { return m_Velocity; }
    EXVector &ImpactPos() { return m_ImpactPos; }
    float &Bottom() { return m_Bottom; }
    float &Top() { return m_Top; }
    float &Mass() { return m_Mass; }
    void SetBlockBits(unsigned short b) { m_BlockBits |= b; }
    unsigned short BlockBits() { return m_BlockBits; }
    CObstacle();
    ~CObstacle();
    void Init(const EXVector &pos, const float &ang, float velocity, float radius, float mass);
    EXVector m_ImpactPos;   // 0x00
    unsigned int m_Flags;   // 0x10
    CAvoidSphere m_Sphere;  // 0x14
    float m_Ang;            // 0x3c
    float m_DestAng;        // 0x40
    float m_Bottom;         // 0x44
    float m_Top;            // 0x48
    float m_Mass;           // 0x4c
    float m_Velocity;       // 0x50
    u16 m_BlockBits;        // 0x54
};

class ImpactInfo {
public:
    ImpactInfo() {}
};

// sizeof TODO (>= 0x138).
class CAvoidance {
public:
    EXVector &PushDelta() { return m_PushDelta; }
    CObstacle &ObsA() { return m_ObsA; }
    CObstacle &ObsB() { return m_ObsB; }
    float AngleChange() { return m_AngleChange; }
    ~CAvoidance();
    int Evaluate(EXVector &v, float &f);
    static EXVector RHTriangleCPos(const EXVector &a, const EXVector &b, float c, float &d, float &e, float &f);
    static void UpdatePerFrame();
    CAvoidance();
    int Collide(EXVector &v, float &f);
    float Steer(Actor *pActor, float f);
private:
    signed char QuadraticFormula(float a, float b, float c, float &r0, float &r1);
    bool const SphereSphereSweep(CAvoidSphere *a, CAvoidSphere *b, float &t0, float &t1);
public:
    CObstacle m_ObsA;       // 0x00
    CObstacle m_ObsB;       // 0x58
    u8 m_Unkb0[0xe8 - 0xb0];    // TODO layout
    float m_fTouchDist;     // 0xe8 summed radii less a little: closer than this is touching
    float m_fVelShare;      // 0xec A's share of the combined speed
    float m_AngleChange;    // 0xf0
    float m_Unkf4;          // 0xf4 1.0 per Steer pass TODO
    float m_fUrgency;       // 0xf8 1 when the hit is now, towards 0 at the 2000 frame horizon
    float m_fHitTime;       // 0xfc frames until the circles touch
    float m_fClosing;       // 0x100 closing speed along A's heading
    float m_fImpactDist;    // 0x104 gap between the two impact positions
    EXVector m_ImpactDir;   // 0x108 unit ground vector from A's impact position to B's
    u8 m_Unk118[4];
    float m_fAngA;          // 0x11c heading to B relative to A's facing
    u8 m_Unk120[8];
    EXVector m_PushDelta;   // 0x128
};
#endif
