#ifndef EXITEMPHYSICS_H
#define EXITEMPHYSICS_H
#include "types.h"
#include "EXRuntimeClass.h"
#include "EXVector.h"

class EXItem;
class EXPlacementItem;
class EXCScoop_FaceData;
class EXInterCollisionTemp;
class EXInterObjectTemp;
class EXCollisionType;

// Physics component of an item (sizeof 0x10). Most virtuals are header inlines
// (emitted in XItemPhysics.obj).
class EXItemPhysics {
public:
    virtual s8 DoUpdate();
    virtual s8 Initialise();
    EX_DECLARE_DYNAMIC(EXItemPhysics)
public:
    virtual ~EXItemPhysics() {}
    virtual void DoCollisions() {}
    virtual s8 DoInterObjCollisions(EXInterCollisionTemp &) { return 0; }
    virtual void DoInterObjCallback(EXInterCollisionTemp &) {}
    virtual u32 NumCollisionObjects() { return 0; }
    virtual void CollisionObject(u32, EXCollisionType &) {}
    virtual void InterObjectScoopCallback(EXInterObjectTemp &) {}
    virtual EXVector BoundsCentre() const { return EXVector(0.0f, 0.0f, 0.0f, 0.0f); }
    virtual float BoundsRadius() const { return 0.0f; }
    virtual void DoCollision(EXItemPhysics *, EXCScoop_FaceData *) {}
    virtual void DoMapCollisionCallBack(EXCScoop_FaceData *pData);
    virtual void SetVelocity(const EXVector &) {}
    virtual EXVector GetVelocity() const { return EXVector(0.0f, 0.0f, 0.0f, 0.0f); }
    EXItemPhysics();
    static EXItemPhysics *CreateObject();
    void SetItem(EXItem *pItem) { m_pItem = pItem; }
    EXItem *Item() { return m_pItem; }
    void SetPhysicsFlagsOn(u16 f) { m_physicsFlags |= f; }
    void SetPhysicsFlagsOff(u16 f) { m_physicsFlags &= ~f; }
    u32 PhysicsFlags() const { return m_physicsFlags; }
    u16 CollisionFlags() const { return m_collisionFlags; }
    u16 InterObjectGroup() const { return m_interObjectGroup; }

    EXItem *m_pItem;            // +4
    u16 m_physicsFlags;         // +8
    u16 m_collisionFlags;       // +0xa
    u16 m_interObjectGroup;     // +0xc
};

// Extended physics interface (sizeof 0x44).
class EXItemPhysicsExt : public EXItemPhysics {
    EX_DECLARE_DYNAMIC(EXItemPhysicsExt)
public:
    virtual ~EXItemPhysicsExt() {}
    virtual s8 DoPostUpdate() { return 1; }
    virtual float Weight() const { return 0.0f; }
    virtual void *GetPhysicsInterface() { return 0; }
    virtual void SetCollisionItem(EXPlacementItem *) {}
    virtual s8 GetEnclosingCollisionObject(EXCollisionType &) { return 0; }
    virtual s8 IsObjectStationary() { return 0; }
    EXItemPhysicsExt();
    static EXItemPhysicsExt *CreateObject();

    EXVector m_10;          // +0x10 TODO
    EXVector m_20;          // +0x20 TODO
    EXVector m_30;          // +0x30 TODO (not set by the constructor)
    void *m_40;             // +0x40 TODO
};
#endif
