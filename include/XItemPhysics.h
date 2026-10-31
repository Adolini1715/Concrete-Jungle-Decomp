#ifndef XITEMPHYSICS_H
#define XITEMPHYSICS_H
#include "EXItemPhysics.h"
#include "EXCollisionType.h"

class XItem;
class Vehicle;
class EXCapsule;
class EXSphere;
class EXCScoop_FaceData;
class EXInterCollisionTemp;

// Game item physics (XItemPhysics.obj): simple point-mass integration plus a capsule collision object.
// sizeof 0x184. Collision flag bits (m_collisionFlags): 1 on ground (inter-object), 4 game-flag face hit.
class XItemPhysics : public EXItemPhysics {
    EX_DECLARE_DYNAMIC(XItemPhysics)
public:
    virtual s8 DoUpdate();
    virtual s8 Initialise();
    virtual void DoCollisions();
    virtual s8 InterObjCollisions(EXInterCollisionTemp &t);
    virtual void InterObjCallback(EXInterCollisionTemp &t);

    void SetDeltaMove(const EXVector &v) { m_DeltaMove = v; }
    void AddDeltaMove(const EXVector &v) { m_DeltaMove += v; }
    EXVector &DeltaMove() { return m_DeltaMove; }
    EXVector &CollisionResponse() { return m_CollisionResponse; }
    void SetPFDeltaMove(const EXVector &v) { m_PFDeltaMove = v; }
    void AddPFDeltaMove(const EXVector &v) { m_PFDeltaMove += v; }
    EXVector &PFDeltaMove() { return m_PFDeltaMove; }
    void SetVertWallReaction(s8 b) { m_bVertWallReaction = b; }
    void SetIgnoreCars(s8 b) { m_bIgnoreCars = b; }
    void SetThrust(float f) { m_DeltaMove.y = f; }
    float GetThrust() const { return m_DeltaMove.y; }
    void SetYAccel(float f) { m_Accel.y = f; }
    EXVector &GetFloorNorm() { return m_FloorNorm; }
    void SetFriction(float f) { m_Friction = f; }
    void SetPreviousPosition(EXVector v)
    {
        m_PrevPos.x = v.x;
        m_PrevPos.y = v.y;
        m_PrevPos.z = v.z;
        m_PrevPos.w = v.w;
    }
    void ResetHitByVehicle()
    {
        m_pHitVehicle = 0;
        m_pHitFlyingVehicle = 0;
    }
    void HitByFlyingVehicle(XItem *pItem, EXVector &v)
    {
        if (m_pHitFlyingVehicle) return;
        m_pHitFlyingVehicle = pItem;
        m_FlyingVehicleVel = v;
    }
    XItem *GetItem() const { return (XItem *)m_pItem; }

    void SetPause(s8 b);
    void EnableCollision(s8 b);
    void EnablePhysics(s8 b);
    void SetCollisionHeightAndWidth(float h);
    void SetCollisionHeightAndWidth(float h, float w);
    float RealSpeed();
    float RealFallSpeed();
    float GetThrustToAttainHeight(float h);
    void SetThrustToAttainHeight(float h);
    void SetCollisionObject(float a, float b, float c, float d);
    void CheckGameFlags(EXCScoop_FaceData *pFace);
    void HitByVehicle(Vehicle *pVehicle, EXCScoop_FaceData *pFace);
    XItemPhysics();
    static void FaceCollisionCallBack(EXCScoop_FaceData *pFace, EXItemPhysics &phys);
    s8 CollideCapsule(EXCapsule &c, EXVector &out);
    s8 CollideSphereClose(EXSphere &s, EXVector &a, EXVector &b);
    s8 CollideSphereVertWall(EXSphere &s, s8 &b1, EXVector &a, EXVector &b, s8 &b2, EXVector &c, EXVector &d);
    static XItemPhysics *CreateObject();

    EXVector m_PrevPos;             // +0x10 item position at the start of DoUpdate
    EXVector m_PrevRot;             // +0x20 item rotation at the start of DoUpdate
    u16 m_PrevCollisionFlags;       // +0x30 collision flags of the last update
    float m_Gravity;                // +0x34
    float m_Friction;               // +0x38 horizontal velocity damping
    float m_3c;                     // +0x3c TODO
    float m_40;                     // +0x40 TODO
    u32 m_44;                       // +0x44 TODO
    s8 m_bCollision;                // +0x48
    s8 m_bPhysics;                  // +0x49
    s8 m_bPause;                    // +0x4a
    s8 m_bVertWallReaction;         // +0x4b
    s8 m_bIgnoreCars;               // +0x4c
    EXVector m_DeltaMove;           // +0x50 velocity (y: thrust)
    EXVector m_Accel;               // +0x60
    EXVector m_RotDelta;            // +0x70 added to the rotation every update
    EXVector m_CollisionResponse;   // +0x80
    EXVector m_PFDeltaMove;         // +0x90 one-frame position delta
    EXVector m_PFRotDelta;          // +0xa0 one-frame rotation delta
    u8 m_b0[0x10];                  // +0xb0 TODO
    EXCollisionType m_ColObject;    // +0xc0 capsule
    EXVector m_GameFlagPos;         // +0x100 face data of the last game-flag hit
    EXVector m_GameFlagNorm;        // +0x110
    EXVector m_FloorNorm;           // +0x120
    float m_130;                    // +0x130 TODO
    float m_ImpactSpeed;            // +0x134 vertical speed at the last ground contact (negative: falling; Player CheckForLand)
    float m_FrameRateDiv;           // +0x138
    u8 m_13c[8];                    // +0x13c TODO
    float m_ColWidth;               // +0x144
    float m_ColHeight;              // +0x148
    EXVector m_ColOffset;           // +0x14c
    Vehicle *m_pHitVehicle;         // +0x15c
    EXVector m_HitVehicleVel;       // +0x160
    XItem *m_pHitFlyingVehicle;     // +0x170
    EXVector m_FlyingVehicleVel;    // +0x174
};
#endif
