#ifndef EX6DOBJECT_H
#define EX6DOBJECT_H
#include "EXItemPhysics.h"
#include "EXVector.h"
#include "EXQuaternion.h"
#include "EXArray.h"
#include "EXFlag.h"

class EX6DManager;
class EX6DObject;
class EX6DParticleBase;
class EX6DRigidBody;
class EX6DCollisionBody;
class EX6DDamageModel;
class EX6DWorldCollisionConstraint;
class EXPlacementItem;
class EXCollisionScoopTemp;
class EXInterCollisionTemp;
class EXInterObjectTemp;
class EXGeoBaseEntity;
class EXGeoScript;
class EXItemAnimator_AnimModifier;
class EXItemCtrl_SnapShot;
class EXItemRender_SkinAnimSection;
class EXGeoBaseAnimSkin;
class EXMatrix;
struct EXGeoAnimSkin_BonePhysicItem;
struct EX6DAnimInitInfo;
struct EX6DConstraintType;

// ---------------------------------------------------------------- damage
// One damage zone (sizeof 0x18).
class EX6DDamageArea {
public:
    EX6DDamageArea();
    void Hit(float f);

    u32 m_00;                   // +0x00
    u32 m_04;                   // +0x04
    float m_08;                 // +0x08
    float m_0c;                 // +0x0c
    float m_10;                 // +0x10
    float m_14;                 // +0x14
};

// Collision object -> damage area map (sizeof 0x18).
class EX6DDamageModel {
public:
    ~EX6DDamageModel();
    void Hit(u32 i, float f);
    void AddDamageArea(EX6DDamageArea &a, const u32 *pObjs, u32 n);
    void AddDamageArea(EX6DDamageArea &a, const EXGeoBaseEntity *pEnt, u32 n);
    void AddDamageAreaAll(EX6DDamageArea &a);
    EX6DDamageModel(u32 n);

    EXDataArray<EX6DDamageArea, EX6DDamageArea &> m_areas;  // +0x00
    u32 *m_pMap;                // +0x10 area index per collision object (-1: none)
    u32 m_count;                // +0x14
};

// ---------------------------------------------------------------- collision body
// Collision shapes of a body (sizeof 0x6c).
class EX6DCollisionBody {
public:
    EX6DDamageModel *DamageModel() { return m_pDamageModel; }
    EXCollisionType *CollisionObjects() { return m_pObjects; }
    u32 NumObjects() const { return m_numObjects; }
    u32 ExtraCollisions() const { return m_extraCollisions; }
    const EXVector &BoundsCentre() const { return m_boundsCentre; }
    const EXVector &DeltaPos() const { return m_deltaPos; }
    float BoundsRadius() const { return m_boundsRadius; }
    void SetMaxContacts(u32 n) { m_maxContacts = n; }
    EX6DCollisionBody(EX6DRigidBody *pBody, u32 n);
    EX6DCollisionBody(EX6DParticleBase *pBody, u32 n);
    void CalcDeltas();
    void FillTempArray(EXInterCollisionTemp &t);
    void CheckBounds(const EXVector &v, float &f, EXVector &out);
    ~EX6DCollisionBody();
    void CalcBounds(u32 flags);
    void DoCollisionFaceCheck(EXCollisionScoopTemp &t, float f);
    void AddDamageModel();
protected:
    void ResetVars();
public:
    EXVector m_boundsCentre;            // +0x00
    EXVector m_10;                      // +0x10
    EXVector m_deltaPos;                // +0x20
    EXVector m_30;                      // +0x30
    EX6DRigidBody *m_pRigidBody;        // +0x40
    EX6DParticleBase *m_pParticle;      // +0x44
    EX6DDamageModel *m_pDamageModel;    // +0x48
    EXCollisionType *m_pObjects;        // +0x4c 0x40 bytes each
    u32 *m_pObjectFlags;                // +0x50
    u32 m_numObjects;                   // +0x54
    u32 m_extraCollisions;              // +0x58
    u32 m_maxContacts;                  // +0x5c
    float m_boundsRadius;               // +0x60
    float m_64;                         // +0x64
    float m_68;                         // +0x68
};

// ---------------------------------------------------------------- constraints
class EX6DPinConstraint {
public:
    EX6DPinConstraint()
    {
        m_pBodyA = 0;
        m_pBodyB = 0;
        m_strength = 0.7f;
    }
    void SetStrength(float f) { m_strength = f; }
    EX6DRigidBody *BodyA() { return m_pBodyA; }
    EX6DParticleBase *BodyB() { return m_pBodyB; }
    EX6DPinConstraint &operator=(const EX6DPinConstraint &c)
    {
        m_pointA = c.m_pointA;
        m_pointB = c.m_pointB;
        m_pBodyA = c.m_pBodyA;
        m_pBodyB = c.m_pBodyB;
        m_strength = c.m_strength;
        return *this;
    }
    void SetWorldSpace(EX6DRigidBody *a, EX6DParticleBase *b, const EXVector &p);
    void SatisfyConstraint();

    EXVector m_pointA;              // +0x00
    EXVector m_pointB;              // +0x10
    EX6DRigidBody *m_pBodyA;        // +0x20
    EX6DParticleBase *m_pBodyB;     // +0x24
    float m_strength;               // +0x28
};

class EX6DAngleConstraint {
public:
    void SetStrength(float f) { m_strength = f; }
    EX6DRigidBody *BodyA() { return m_pBodyA; }
    EX6DRigidBody *BodyB() { return m_pBodyB; }
    void Set(EX6DRigidBody *a, EX6DRigidBody *b, const EXVector &v1, const EXVector &v2);
    EXQuaternion Slerp6D(const EXQuaternion &a, const EXQuaternion &b, float t) const;
    void SatisfyConstraint();
    EX6DAngleConstraint()
    {
        m_pBodyA = 0;
        m_pBodyB = 0;
        m_strength = 0.7f;
    }
    EX6DAngleConstraint &operator=(const EX6DAngleConstraint &c)
    {
        m_a = c.m_a;
        m_b = c.m_b;
        m_pBodyA = c.m_pBodyA;
        m_pBodyB = c.m_pBodyB;
        m_strength = c.m_strength;
        return *this;
    }

    EXVector m_a;                   // +0x00
    EXVector m_b;                   // +0x10
    EX6DRigidBody *m_pBodyA;        // +0x20
    EX6DRigidBody *m_pBodyB;        // +0x24
    float m_strength;               // +0x28
};

class EX6DWorldCollisionConstraint {
public:
    EX6DWorldCollisionConstraint()
    {
        m_strength = 0.5f;
        m_pBodyA = 0;
        m_bodyPoint.SetZero();
    }
    const EXVector &Point() const { return m_point; }
    const EXVector &BodyPoint() const { return m_bodyPoint; }
    const EXVector &Normal() const { return m_normal; }
    float Strength() const { return m_strength; }
    void SetBodyPoint(const EXVector &v) { m_bodyPoint = v; }
    u32 Ref() const { return m_ref; }
    void Set(EX6DParticleBase *p, const EXVector &pt, const EXVector &n, u32 ref)
    {
        m_pBodyA = p;
        m_point = pt;
        m_normal = n;
        m_ref = ref;
    }
    EX6DParticleBase *BodyA() { return m_pBodyA; }
    EX6DWorldCollisionConstraint &operator=(const EX6DWorldCollisionConstraint &c)
    {
        m_point = c.m_point;
        m_bodyPoint = c.m_bodyPoint;
        m_normal = c.m_normal;
        m_pBodyA = c.m_pBodyA;
        m_ref = c.m_ref;
        m_strength = c.m_strength;
        return *this;
    }
    void SatisfyConstraint(float t);

    EXVector m_point;               // +0x00
    EXVector m_bodyPoint;           // +0x10
    EXVector m_normal;              // +0x20
    EX6DParticleBase *m_pBodyA;     // +0x30
    u32 m_ref;                      // +0x34
    float m_strength;               // +0x38
};

class EX6DInterCollisionConstraint {
public:
    EX6DInterCollisionConstraint()
    {
        m_pBodyA = 0;
        m_pBodyB = 0;
    }
    void Set(EX6DRigidBody *a, EX6DRigidBody *b, u32 ia, u32 ib)
    {
        m_pBodyA = a;
        m_pBodyB = b;
        m_objA = ia;
        m_objB = ib;
    }
    EX6DRigidBody *BodyA() { return m_pBodyA; }
    EX6DRigidBody *BodyB() { return m_pBodyB; }
    void SatisfyConstraint(float t);

    EX6DRigidBody *m_pBodyA;        // +0x00
    EX6DRigidBody *m_pBodyB;        // +0x04
    u32 m_objA;                     // +0x08
    u32 m_objB;                     // +0x0c
};

// Tagged constraint storage (sizeof 0x44): 0 pin, 1 angle, 2 world collision, 3 inter collision.
struct EX6DConstraintType {
    EX6DConstraintType() { m_iterations = 3; }
    void SetIterations(u8 n) { m_iterations = n; }
    u8 Iterations() const { return m_iterations; }
    void Set(const EX6DInterCollisionConstraint &c)
    {
        InterCollisionConstraint() = c;
        m_type = 3;
    }
    EX6DPinConstraint &PinConstraint() { return *(EX6DPinConstraint *)m_data; }
    EX6DAngleConstraint &AngleConstraint() { return *(EX6DAngleConstraint *)m_data; }
    EX6DWorldCollisionConstraint &WorldCollisionConstraint() { return *(EX6DWorldCollisionConstraint *)m_data; }
    EX6DInterCollisionConstraint &InterCollisionConstraint() { return *(EX6DInterCollisionConstraint *)m_data; }
    EX6DParticleBase *BodyA();
    EX6DParticleBase *BodyB();
    void Set(const EX6DPinConstraint &c)
    {
        PinConstraint() = c;
        m_type = 0;
    }
    void Set(const EX6DWorldCollisionConstraint &c)
    {
        WorldCollisionConstraint() = c;
        m_type = 2;
    }
    void SatisfyConstraint(float t);
    void Set(const EX6DAngleConstraint &c)
    {
        AngleConstraint() = c;
        m_type = 1;
    }

    u32 m_data[0xf];                // +0x00
    int m_type;                     // +0x3c
    u8 m_iterations;                // +0x40
};

// ---------------------------------------------------------------- objects
// Base of every 6D physics object (sizeof 0x5c).
class EX6DObject : public EXItemPhysicsExt {
    EX_DECLARE_DYNAMIC(EX6DObject)
public:
    virtual ~EX6DObject();
    virtual void SetCollisionItem(EXPlacementItem *p) { m_pCollisionItem = p; }
    virtual void UpdateMovement(float) {}
    virtual void DoEnvironmentCollisions(float) {}
    virtual void ClearForces() {}
    virtual void Syncronise() {}
    virtual void SetActive(s8 b)
    {
        m_active = b;
        if (b) m_physicsFlags &= ~0x20;
        else m_physicsFlags |= 0x20;
        if (m_pParent && m_pParent->m_active != b) m_pParent->SetActive(b);
    }
    virtual EX6DCollisionBody *CollisionBody() { return 0; }
    s8 Active() const { return m_active; }
    s8 AutoUpdate() const { return m_autoUpdate; }
    void SetAutoUpdate(s8 b) { m_autoUpdate = b; }
    void SetManager(EX6DManager *p) { m_pManager = p; }
    EX6DManager *Manager() { return m_pManager; }
    void SetParent(EX6DObject *p) { m_pParent = p; }
    EXPlacementItem *CollisionItem() { return m_pCollisionItem; }
    ex::EXFlag<u32> &Flags6D() { return m_flags6D; }
    u8 Iterations() const { return m_iterations; }
    EX6DObject();
    static EX6DObject *CreateObject();

    EX6DObject *m_pParent;              // +0x44
    EXPlacementItem *m_pCollisionItem;  // +0x48
    EX6DManager *m_pManager;            // +0x4c
    float m_50;                         // +0x50
    ex::EXFlag<u32> m_flags6D;          // +0x54
    u8 m_iterations;                    // +0x58
    s8 m_active;                        // +0x59
    s8 m_autoUpdate;                    // +0x5a
};

// Point mass (sizeof 0xdc).
class EX6DParticleBase : public EX6DObject {
    EX_DECLARE_DYNAMIC(EX6DParticleBase)
public:
    virtual ~EX6DParticleBase();
    virtual u32 NumCollisionObjects() { return m_pCollisionBody ? m_pCollisionBody->NumObjects() : 0; }
    virtual void CollisionObject(u32 i, EXCollisionType &c);
    virtual EXVector BoundsCentre() const
    {
        if (m_pCollisionBody) return m_pCollisionBody->BoundsCentre();
        return EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    }
    virtual float BoundsRadius() const
    {
        if (m_pCollisionBody) return m_pCollisionBody->BoundsRadius();
        return 0.0f;
    }
    virtual EXVector GetVelocity() const { return m_velocity; }
    virtual s8 IsObjectStationary() { return m_active == 0; }
    virtual void UpdateMovement(float t);
    virtual void DoEnvironmentCollisions(float t);
    virtual void ClearForces();
    virtual void Syncronise();
    virtual EX6DCollisionBody *CollisionBody() { return m_pCollisionBody; }
protected:
    virtual void FindCollisionBounds(EXCollisionScoopTemp &t, u32 flags);
public:
    virtual void SetMass(float m);
    virtual void SatisfyWorldCollision(EX6DWorldCollisionConstraint &c, float t);
    EXVector &Position() { return m_pos; }
    EXVector &Velocity() { return m_velocity; }
    void AddWorldImpulse(const EXVector &v)
    {
        m_impulse += v;
        if (!m_active) SetActive(1);
    }
    EXVector &LastPosition() { return m_lastPos; }
    float Mass() const { return m_mass; }
    float InvMass() const { return m_invMass; }
    float CoefRestitution() const { return m_coefRestitution; }
    void SetCoefRestitution(float f) { m_coefRestitution = f; }
    float GroundFriction() const { return m_groundFriction; }
    void SetGroundFriction(float f) { m_groundFriction = f; }
    void SetDampVelocity(float f) { m_dampVelocity = 1.0f - f; }
    float InterObjCollisionVelocityLoss() const { return m_interObjVelocityLoss; }
    float InterObjFriction() const { return m_interObjFriction; }
    float TotalCollisions() const { return m_totalCollisions; }
    void SetTotalCollisions(float f) { m_totalCollisions = f; }
    void AddWorldForce(const EXVector &v)
    {
        m_force += v;
        if (!m_active) SetActive(1);
    }
    // [fi] in EXPhysics6D.obj, called out of line by the inter collision solver.
    __declspec(noinline) EX6DDamageModel *DamageModel()
    {
        if (!m_pCollisionBody) return 0;
        return m_pCollisionBody->DamageModel();
    }
    void CalcDeltas(float)
    {
        if (m_pCollisionBody) m_pCollisionBody->CalcDeltas();
    }
    EX6DParticleBase();
    static EX6DParticleBase *CreateObject();
    void SetCollisionBody(EX6DCollisionBody *p)
    {
        if (m_pCollisionBody) delete m_pCollisionBody;
        m_pCollisionBody = p;
    }
    void DoCollisionFaceCheck(EXCollisionScoopTemp &t, float f)
    {
        if (m_pCollisionBody) m_pCollisionBody->DoCollisionFaceCheck(t, f);
    }

    EXVector m_pos;                     // +0x5c
    EXVector m_lastPos;                 // +0x6c
    EXVector m_velocity;                // +0x7c
    EXVector m_force;                   // +0x8c
    EXVector m_impulse;                 // +0x9c
    EX6DCollisionBody *m_pCollisionBody;  // +0xac
    EXPlacementItem *m_pParentItem;     // +0xb0 item the body moves relative to (TODO name)
    float m_mass;                       // +0xb4
    float m_invMass;                    // +0xb8
    float m_dampVelocity;               // +0xbc
    float m_c0;                         // +0xc0
    float m_interObjVelocityLoss;       // +0xc4
    float m_coefRestitution;            // +0xc8
    float m_groundFriction;             // +0xcc
    float m_interObjFriction;           // +0xd0
    float m_gravityScale;               // +0xd4
    float m_totalCollisions;            // +0xd8
};

// Rigid body (sizeof 0x14c).
class EX6DRigidBody : public EX6DParticleBase {
    EX_DECLARE_DYNAMIC(EX6DRigidBody)
public:
    virtual ~EX6DRigidBody() {}
    virtual void CollisionObject(u32 i, EXCollisionType &c);
    virtual void InterObjectScoopCallback(EXInterObjectTemp &t);
    virtual void DoCollision(EXItemPhysics *pOther, EXCScoop_FaceData *pData);
    virtual void UpdateMovement(float t);
    virtual void ClearForces();
    virtual void Syncronise();
protected:
    virtual void FindCollisionBounds(EXCollisionScoopTemp &t, u32 flags);
public:
    virtual void SetMass(float m);
    virtual void SatisfyWorldCollision(EX6DWorldCollisionConstraint &c, float t);
    EXQuaternion &Rotation() { return m_rot; }
    EXVector &AngularVelocity() { return m_angVel; }
    EXQuaternion &LastRotation() { return m_lastRot; }
    EXVector &InvInertia() { return m_invInertia; }
    void AddLocalTorque(const EXVector &v) { m_torque += v; }
    void AddLocalAngImpulse(const EXVector &v) { m_angImpulse += v; }
    void SetDampAngular(float f) { m_dampAngular = 1.0f - f; }
    float InterObjCollisionAngularLoss() const { return m_interObjAngularLoss; }
    void SetInertiaTweak(float f);
    void SyncWithItem();
    void SetInertiaFromSides(float x, float y, float z);
    void SetSphereInertia();
    void UpdateLastOrientation();
    void AddWorldForceAtPoint(const EXVector &f, const EXVector &p);
    void AddWorldImpulseAtPoint(const EXVector &f, const EXVector &p);
    EX6DRigidBody();
    static EX6DRigidBody *CreateObject();

    EXVector m_invInertia;              // +0xdc
    EXQuaternion m_rot;                 // +0xec
    EXQuaternion m_lastRot;             // +0xfc
    EXVector m_angVel;                  // +0x10c
    EXVector m_torque;                  // +0x11c
    EXVector m_angImpulse;              // +0x12c
    float m_dampAngular;                // +0x13c
    float m_140;                        // +0x140
    float m_interObjAngularLoss;        // +0x144
    float m_148;                        // +0x148
};

// Particle with a unit sphere collision body.
class EX6DParticle : public EX6DParticleBase {
public:
    virtual ~EX6DParticle();
    EX6DParticle();
};

class EX6DRigidTest : public EX6DRigidBody {
public:
    virtual ~EX6DRigidTest();
    EX6DRigidTest();
};

// Rigid body built from a geo entity or script (sizeof 0x184).
class EX6DRigid : public EX6DRigidBody {
    EX_DECLARE_DYNAMIC(EX6DRigid)
public:
    virtual ~EX6DRigid();
    virtual s8 GetEnclosingCollisionObject(EXCollisionType &c);
    virtual void UpdateMovement(float t);
    virtual void SetActive(s8 b);
    float GetSampledMove() const;
    void Initialise(const EXGeoBaseEntity *pEnt, s8 b, u32 flags);
    void Initialise(const EXGeoScript *pScript, s8 b, u32 flags);
    EX6DRigid(const EXGeoBaseEntity *pEnt, s8 b);
    EX6DRigid()
    {
        m_pSamples = 0;
        m_15c.SetZero();
        m_14c.SetZero();
    }
    static EX6DRigid *CreateObject();

    EXVector m_14c;                     // +0x14c
    EXVector m_15c;                     // +0x15c
    float *m_pSamples;                  // +0x16c recent movement samples (deactivation)
    float m_170;                        // +0x170 deactivation threshold
    float m_174;                        // +0x174 time to next sample
    float m_178;                        // +0x178 sample interval
    u32 m_sampleIndex;                  // +0x17c
    u32 m_numSamples;                   // +0x180
};

// Group of bodies moved as one object (sizeof 0x98).
class EX6DBodyGroup : public EX6DObject {
    EX_DECLARE_DYNAMIC(EX6DBodyGroup)
public:
    virtual EXVector BoundsCentre() const { return m_boundsCentre; }
    virtual float BoundsRadius() const { return m_boundsRadius; }
    virtual void DoCollision(EXItemPhysics *pOther, EXCScoop_FaceData *pData);
    virtual void UpdateMovement(float t);
    virtual void DoEnvironmentCollisions(float t);
    virtual void ClearForces();
    virtual void Syncronise();
    virtual void SetActive(s8 b);
    EX6DParticleBase *Body(int i)  // Actor.obj copy
    {
        if (i < BodyCount()) return m_bodies[i];
        return 0;
    }
    int BodyCount() const { return m_bodies.Size(); }
    float GetSampledMove() const;
    void InitDefaultDeactivation();
    void AddBody(EX6DParticleBase *p);
    // NEAR (18 bytes): follows from the inlined EX6DObject constructor's store order.
    EX6DBodyGroup()
    {
        m_boundsCentre.SetZero();
        m_pSamples = 0;
        m_numSamples = 0;
        m_boundsRadius = 0.0f;
    }
    static EX6DBodyGroup *CreateObject();
protected:
    float UpdatedBoundsRadius(const EXVector &v, EXVector &out);
    void FindCollisionBounds(EXCollisionScoopTemp &t, u32 flags);
public:
    EXPtrArray<EX6DParticleBase> m_bodies;  // +0x5c
    EXVector m_boundsCentre;            // +0x6c
    float m_boundsRadius;               // +0x7c
    float *m_pSamples;                  // +0x80 recent movement samples (deactivation)
    float m_84;                         // +0x84
    float m_88;                         // +0x88
    float m_8c;                         // +0x8c
    u32 m_sampleIndex;                  // +0x90
    u32 m_numSamples;                   // +0x94
};

// Ragdoll / animated body group (sizeof 0xac).
class EX6DAnim : public EX6DBodyGroup {
    EX_DECLARE_DYNAMIC(EX6DAnim)
public:
    virtual ~EX6DAnim();
    ex::EXFlag<u32> &Flags() { return m_flags; }
    float GetTotalCollisions() const;
    s8 Initialise(EXItemAnimator_AnimModifier *pMod);
    s8 Initialise(EX6DAnimInitInfo *pInfo);
    s8 AutoRagdollInitialise(EXItemAnimator_AnimModifier *pMod, u32 n, EXItemCtrl_SnapShot *a, EXItemCtrl_SnapShot *b, float f);
    EX6DAnim();
    static EX6DAnim *CreateObject();
protected:
    s8 RecursiveInit(EXMatrix m, const EXItemRender_SkinAnimSection *pSec, const EXGeoAnimSkin_BonePhysicItem *pPhys, int parent, u32 n);
    s8 RecurseRagdollInit(EXItemRender_SkinAnimSection *pSec, EXMatrix m, const EXMatrix &m2, EXItemRender_SkinAnimSection *pRoot, EXGeoBaseAnimSkin *pSkin, u32 n, s8 &b);
    s8 RecurseInfoInit(EX6DAnimInitInfo *pInfo, EXItemRender_SkinAnimSection *pSec, EXMatrix m, const EXMatrix &m2, EXItemRender_SkinAnimSection *pRoot, EXGeoBaseAnimSkin *pSkin, u32 n, s8 &b);
public:

    EX6DRigidBody *m_pRigids;           // +0x98
    u32 m_9c;                           // +0x9c
    u32 m_a0;                           // +0xa0
    u32 m_a4;                           // +0xa4
    ex::EXFlag<u32> m_flags;            // +0xa8
};

// Ragdoll set-up data (EX6DAnim::Initialise): one entry per body, in the order RecurseInfoInit
// creates them.
struct EX6DAnimBodyInfo {
    u32 m_00;                       // +0x00
    float m_restitution;            // +0x04
    float m_radius;                 // +0x08 collision sphere radius (at least 0.15)
    float m_damping;                // +0x0c velocity damping
    s8 m_bDoublePin;                // +0x10 joint pinned at two points 0.2 apart in x (no twist about x)
    s8 m_bAngleLimit;               // +0x11 add an angle constraint
    u8 m_12[2];
    float m_pinStrength;            // +0x14
    float m_angleStrength;          // +0x18
    EXVector m_angleA;              // +0x1c angle constraint limits
    EXVector m_angleB;              // +0x2c
};
// A twist link: the skin bone and the body it drives.
struct EX6DTwistInfo {
    u32 m_bone;                     // +0x00
    u32 m_body;                     // +0x04
};
struct EX6DAnimInitInfo {
    EXItemAnimator_AnimModifier *m_pMod;                        // +0x00
    EXItemCtrl_SnapShot *m_pSnapShot;                           // +0x04
    EXItemCtrl_SnapShot *m_pVelocity;                           // +0x08
    float m_time;                                               // +0x0c
    EXDataArray<EX6DAnimBodyInfo, EX6DAnimBodyInfo &> m_bodies; // +0x10
    EXDataArray<EX6DTwistInfo, EX6DTwistInfo &> m_twists;       // +0x20
};

// ---------------------------------------------------------------- manager
class EX6DManager {
public:
    float InterReactivateDist() const { return m_interReactivateDist; }
    void Cleanup();
    void RemoveConstraints(const EX6DObject *p);
    void RemoveBody(const EX6DObject *p);
    void SatisfyConstraints(float t);
    void DoStep(float t);
    ~EX6DManager();
    void AddConstraint(EX6DConstraintType &c);
    void AddCollision(EX6DConstraintType &c);
    void AddBody(EX6DObject *p);
    EX6DManager();
    ex::EXFlag<u32> &Flags() { return m_flags; }
protected:
    void ClearAllForces();
    void UpdateAllMovements(float t);
    void SyncroniseAll();
    void DetectAllCollisions(float t);
public:
    EXPtrArray<EX6DObject> m_bodies;                                        // +0x00
    EXDataArray<EX6DConstraintType, EX6DConstraintType &> m_constraints;    // +0x10
    EXDataArray<EX6DConstraintType, EX6DConstraintType &> m_collisions;     // +0x20
    float m_interReactivateDist;    // +0x30
    float m_34;                     // +0x34
    ex::EXFlag<u32> m_flags;        // +0x38
};
#endif
