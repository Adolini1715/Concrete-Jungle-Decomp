#include "EXBaseRenderEnv.h"
#include "EXIntersection.h"
#include "EXItemCtrl_AnimModifier.h"
#include <float.h>
#include "EXCollisionScoopTemp.h"
#include "EX6DObject.h"
#include "EXItem.h"
#include "EXMatrix.h"
#include "EXPhysicsStandIn.h"
#include "EXMemory.h"
#include "EXGeoEntity.h"
#include "EXGeoHeader.h"
#include "EXGeoComplexRefObject.h"
#include "EXItemAnimator_AnimModifier.h"
#include "EXGeoAnim.h"
#include "EXGeoFile.h"
#include "EXGeoTrigger.h"
#include "EXGeoScript.h"
#include "EXInterObjectTemp.h"
#include "EXDatum.h"
#include "EXCollisionType.h"

class EXCScoopTemp;

// TODO stand-in: item environment (EXItemEnv.obj).
class EXItemEnv {
public:
    static EXItemEnv *TheItemEnv() { return m_pTheItemEnv; }
    EX6DManager *Get6DManager() const { return m_p6DManager; }
    void UpdateItems_InterObjCollision();
    void DoCollisionScoopToMap(EXCollisionScoopTemp &t) const;
    void DoCollisionScoopToContoured(EXCollisionScoopTemp &t, EXItem *pItem) const;
    u8 m_00[0x80];
    EX6DManager *m_p6DManager;  // +0x80
protected:
    static EXItemEnv *m_pTheItemEnv;
};

// TODO stand-in: inter-object collision scratch (collision area); only the fields filled here.
class EXInterCollisionTemp {
public:
    EXVector m_centres[1];      // +0x00
    u8 m_10[0x58 - 0x10];
    EXCollisionType *m_pObjects[0x14];  // +0x58
    float m_radii[0xf];         // +0xa8
    u8 m_numObjects[4];         // +0xe4
    u8 m_count;                 // +0xe8
};

// TODO stand-in: render object a damage area drives (its damage frame fields).
struct EX6DDamageTarget {
    u8 m_00[0x64];
    ex::EXFlag<u16> m_flags;    // +0x64
    u16 m_66;
    u8 m_68[8];
    s16 m_frame;                // +0x70
    s16 m_nextFrame;            // +0x72
    u8 m_74;
    u8 m_blend;                 // +0x75
};

EX_IMPLEMENT_DYNCREATE(EX6DObject, EXItemPhysicsExt)
EX_IMPLEMENT_DYNCREATE(EX6DParticleBase, EX6DObject)
EX_IMPLEMENT_DYNCREATE(EX6DRigidBody, EX6DParticleBase)
EX_IMPLEMENT_DYNCREATE(EX6DBodyGroup, EX6DObject)
EX_IMPLEMENT_DYNCREATE(EX6DRigid, EX6DRigidBody)
EX_IMPLEMENT_DYNCREATE(EX6DAnim, EX6DBodyGroup)

inline float SafeInverse(float f)
{
    if (f > 0.0f) return 1.0f / f;
    return 1e-30f;
}

EX6DDamageArea::EX6DDamageArea()
{
    m_08 = 1.0f;
    m_0c = 0.0f;
    m_10 = 2.0f;
    m_00 = 0;
    m_04 = 0;
    m_14 = 0.01f;
}

void EX6DCollisionBody::ResetVars()
{
    m_64 = 3.402823466e+38f;
    m_pRigidBody = 0;
    m_pParticle = 0;
    m_pObjects = 0;
    m_pObjectFlags = 0;
    m_pDamageModel = 0;
    m_68 = 0.0f;
    m_extraCollisions = 0;
    m_numObjects = 0;
}

// NEAR: ~100 bytes differ, the impulse scale multiplies y/z/w as f*v (retail keeps f live) and the tail order.
void EX6DParticleBase::ClearForces()
{
    m_force.Set(0.0f, m_gravityScale * m_mass * -9.81f, 0.0f, 0.0f);
    const float &s = m_impulse.LengthXYZ() * m_invMass;
    if (80.0f < s) {
        m_impulse *= (1.0f - 80.0f / s);
    } else {
        m_impulse.SetZero();
    }
    m_totalCollisions = 0.0f;
}

void EX6DParticleBase::FindCollisionBounds(EXCollisionScoopTemp &t, u32 flags)
{
    if (!CollisionBody()) return;
    if (!CollisionBody()->ExtraCollisions()) {
        t.m_centre = CollisionBody()->BoundsCentre();
        t.m_radius = CollisionBody()->BoundsRadius();
        return;
    }
    u32 n = CollisionBody()->ExtraCollisions();
    const EXVector &d = CollisionBody()->DeltaPos();
    EXVector off = d * ((1.0f - 1.0f / (float)(n + 1)) * 0.5f);
    float r = CollisionBody()->BoundsRadius();
    t.m_radius = off.LengthXYZ() + r;
    t.m_centre = CollisionBody()->BoundsCentre() - off;
}

void EX6DParticleBase::SetMass(float m)
{
    m_mass = m;
    m_invMass = SafeInverse(m);
}

void EX6DRigidBody::SetInertiaTweak(float f)
{
    float r = f / m_148;
    m_148 = f;
    float ix = SafeInverse(m_invInertia.x) * r;
    float iy = SafeInverse(m_invInertia.y) * r;
    float iz = SafeInverse(m_invInertia.z) * r;
    m_invInertia.x = SafeInverse(ix);
    m_invInertia.y = SafeInverse(iy);
    m_invInertia.z = SafeInverse(iz);
}

void EX6DRigidBody::ClearForces()
{
    m_torque.Set(0.0f, 0.0f, 0.0f, 0.0f);
    m_angImpulse.Set(0.0f, 0.0f, 0.0f, 0.0f);
    EX6DParticleBase::ClearForces();
}

void EX6DRigidBody::SyncWithItem()
{
    if (!m_pItem) return;
    m_pos = m_pItem->Position();
    m_rot.quat_xyzw::SetRotation(m_pItem->Rotation(), m_pItem->m_rotOrder);
    m_angVel.Set(0.0f, 0.0f, 0.0f, 0.0f);
    m_velocity.Set(0.0f, 0.0f, 0.0f, 0.0f);
}

void EX6DRigidBody::SetInertiaFromSides(float x, float y, float z)
{
    x *= 0.5f;
    y *= 0.5f;
    z *= 0.5f;
    float ix = (y * y + z * z) * m_148 * m_mass * (1.0f / 3.0f);
    float iy = (x * x + z * z) * m_148 * m_mass * (1.0f / 3.0f);
    float iz = (x * x + y * y) * m_148 * m_mass * (1.0f / 3.0f);
    m_invInertia.x = SafeInverse(ix);
    m_invInertia.y = SafeInverse(iy);
    m_invInertia.z = SafeInverse(iz);
}

void EX6DRigidBody::SetSphereInertia()
{
    if (!CollisionBody()) return;
    float r = CollisionBody()->CollisionObjects()->Radius();
    float i = m_148 * m_mass * r * r * 0.4f;
    m_invInertia.x = SafeInverse(i);
    m_invInertia.y = SafeInverse(i);
    m_invInertia.z = SafeInverse(i);
}

// NEAR: 13 bytes differ, the compiler picks a different operand order for the three inertia products.
void EX6DRigidBody::SetMass(float m)
{
    float inv = SafeInverse(m);
    m_invInertia.x = m_invInertia.x * m_mass * inv;
    m_invInertia.y = m_invInertia.y * m_mass * inv;
    m_invInertia.z = m_invInertia.z * m_mass * inv;
    m_mass = m;
    m_invMass = inv;
}

float EX6DBodyGroup::GetSampledMove() const
{
    if (!m_active) return 0.0f;
    float mx = 0.0f;
    for (u32 i = 0; i < m_numSamples; i++) {
        if (m_pSamples[i] > mx) mx = m_pSamples[i];
    }
    return mx;
}

void EX6DBodyGroup::InitDefaultDeactivation()
{
    m_8c = 0.6f;
    m_numSamples = 3;
    m_pSamples = (float *)_EXAlloc(m_numSamples * sizeof(float), 0);
    m_88 = m_8c;
    m_84 = 0.3f;
    m_sampleIndex = 0;
    for (u32 i = 0; i < m_numSamples; i++) m_pSamples[i] = 100.0f;
}

void EX6DRigid::SetActive(s8 b)
{
    m_active = b;
    if (b) m_physicsFlags &= ~0x20;
    else m_physicsFlags |= 0x20;
    if (b) {
        u32 i = m_sampleIndex + 1;
        if (i >= m_numSamples) i = 0;
        m_pSamples[i] = 100.0f;
    }
    if (m_pParent && m_pParent->m_active != b) m_pParent->SetActive(b);
}

float EX6DRigid::GetSampledMove() const
{
    if (!m_active) return 0.0f;
    float mx = 0.0f;
    for (u32 i = 0; i < m_numSamples; i++) {
        if (m_pSamples[i] > mx) mx = m_pSamples[i];
    }
    return mx;
}

void EX6DAngleConstraint::Set(EX6DRigidBody *a, EX6DRigidBody *b, const EXVector &vMin, const EXVector &vMax)
{
    m_pBodyA = a;
    m_pBodyB = b;
    m_a.Set(-3.402823466e+38f, -3.402823466e+38f, -3.402823466e+38f, 0.0f);
    m_b.Set(3.402823466e+38f, 3.402823466e+38f, 3.402823466e+38f, 0.0f);
    if (vMin.x > -6.2831855f) m_a.x = (float)sin(vMin.x * 0.5f);
    if (vMin.y > -6.2831855f) m_a.y = (float)sin(vMin.y * 0.5f);
    if (vMin.z > -6.2831855f) m_a.z = (float)sin(vMin.z * 0.5f);
    if (6.2831855f > vMax.x) m_b.x = (float)sin(vMax.x * 0.5f);
    if (6.2831855f > vMax.y) m_b.y = (float)sin(vMax.y * 0.5f);
    if (6.2831855f > vMax.z) m_b.z = (float)sin(vMax.z * 0.5f);
}

EXQuaternion EX6DAngleConstraint::Slerp6D(const EXQuaternion &a, const EXQuaternion &b, float t) const
{
    float d = a.y * b.y + a.z * b.z + b.w * a.w + a.x * b.x;
    float c;
    if (d >= 1.0f) c = 1.0f;
    else if (d <= -1.0f) c = -1.0f;
    else c = d;
    float angle = EXACos(c);
    float inv = 1.0f / (float)sin(angle);
    float ra = (float)sin((1.0f - t) * angle) * inv;
    float rb = (float)sin(t * angle) * inv;
    EXQuaternion r;
    r.x = a.x * ra + b.x * rb;
    r.y = a.y * ra + b.y * rb;
    r.z = a.z * ra + b.z * rb;
    r.w = b.w * rb + a.w * ra;
    return r;
}

void EX6DWorldCollisionConstraint::SatisfyConstraint(float t) { m_pBodyA->SatisfyWorldCollision(*this, t); }

EX6DParticleBase *EX6DConstraintType::BodyA()
{
    switch (m_type) {
    case 0: return PinConstraint().BodyA();
    case 1: return AngleConstraint().BodyA();
    case 2: return WorldCollisionConstraint().BodyA();
    case 3: return InterCollisionConstraint().BodyA();
    }
    return 0;
}

EX6DParticleBase *EX6DConstraintType::BodyB()
{
    switch (m_type) {
    case 0: return PinConstraint().BodyB();
    case 1: return AngleConstraint().BodyB();
    case 2: return 0;
    case 3: return InterCollisionConstraint().BodyB();
    }
    return 0;
}

// NEAR: 9 bytes differ, retail stores m_flags6D after m_autoUpdate (scheduling of the second Flags6D clear).
EX6DObject::EX6DObject()
{
    m_pManager = EXItemEnv::TheItemEnv()->Get6DManager();
    m_pCollisionItem = 0;
    m_pParent = 0;
    m_active = 0;
    m_autoUpdate = 1;
    m_50 = 10.0f;
    m_iterations = 3;
    m_flags6D = 0;
}

void EX6DDamageArea::Hit(float f)
{
    if (!m_00 && !m_04) return;
    if (m_0c >= m_08) return;
    if (!(f > m_10)) return;
    m_0c += (f - m_10) * m_14;
    if (m_0c > m_08) m_0c = m_08;
    if (m_00) {
        EX6DDamageTarget *p = (EX6DDamageTarget *)m_00;
        float v = m_0c;
        s16 i = (s16)v;
        p->m_frame = i;
        p->m_nextFrame = i + 1;
        p->m_blend = (u8)((v - (float)i) * 256.0f);
        ((EX6DDamageTarget *)m_00)->m_flags.BitSet(0x1000);
    } else if (m_04) {
        EX6DDamageTarget *p = (EX6DDamageTarget *)m_04;
        float v = m_0c;
        s16 i = (s16)v;
        p->m_frame = i;
        p->m_nextFrame = i + 1;
        p->m_blend = (u8)((v - (float)i) * 256.0f);
        ((EX6DDamageTarget *)m_04)->m_flags.BitSet(0x1000);
    }
}

EX6DDamageModel::~EX6DDamageModel()
{
    if (m_pMap) EXFree(m_pMap);
}

void EX6DDamageModel::Hit(u32 i, float f)
{
    if (i < m_count && m_pMap[i] < (u32)m_areas.Size()) m_areas[m_pMap[i]].Hit(f);
}

EX6DCollisionBody::EX6DCollisionBody(EX6DRigidBody *pBody, u32 n)
{
    ResetVars();
    m_pRigidBody = pBody;
    m_pObjects = (EXCollisionType *)_EXAlloc(n * 0x40, 0);
    m_pObjectFlags = (u32 *)_EXAlloc(n * sizeof(u32), 0);
    m_numObjects = n;
    m_maxContacts = 8;
}

EX6DCollisionBody::EX6DCollisionBody(EX6DParticleBase *pBody, u32 n)
{
    ResetVars();
    m_pParticle = pBody;
    m_pObjects = (EXCollisionType *)_EXAlloc(n * 0x40, 0);
    m_pObjectFlags = (u32 *)_EXAlloc(n * sizeof(u32), 0);
    m_numObjects = n;
    m_maxContacts = 8;
}

void EX6DCollisionBody::FillTempArray(EXInterCollisionTemp &t)
{
    t.m_pObjects[t.m_count] = m_pObjects;
    t.m_numObjects[t.m_count] = (u8)m_numObjects;
    t.m_centres[t.m_count] = m_boundsCentre;
    t.m_radii[t.m_count] = m_boundsRadius;
}

// NEAR: ~40 bytes differ, retail keeps the transformed position in memory (w store) before SetPosition.
void EX6DParticleBase::Syncronise()
{
    if (!m_pItem) return;
    if (m_pParentItem) {
        EXQuaternion rot(-m_pItem->Rotation().x, -m_pItem->Rotation().y, -m_pItem->Rotation().z, m_pItem->Rotation().w);
        EXMatrix m;
        m.SetRotation(m_pItem->Rotation(), 0);
        m.SetTranslation(m_pItem->Position());
        m = m.FastInverse();
        EXVector p = m.mat_44::operator*(EXVector(m_pos.x, m_pos.y, m_pos.z, 1.0f));
        p[3] = 0.0f;
        m_pParentItem->SetRotation(rot);
        m_pParentItem->SetPosition(p);
        m_pParentItem->Invalidate();
    } else {
        m_pItem->SetPosition(m_pos);
        m_pItem->Invalidate();
    }
}

void EX6DParticleBase::CollisionObject(u32 i, EXCollisionType &c)
{
    if (!m_pCollisionBody) return;
    c = m_pCollisionBody->CollisionObjects()[i];
    EXMatrix m;
    m.SetIdentity();
    m.SetTranslation(m_pos);
    c.ApplyXForm(m);
}

// EXVector::UnitVectorXYZ, which VC will not inline here.
static __forceinline EXVector UnitXYZ(const EXVector &f)
{
    EXMaths::vec_xyzw v = f;
    v.set_w0();
    float len = v.Length();
    if (len > EX_EPSILON_NORMALISE) {
        float inv = 1.0f / len;
        v.x = inv * v.x; v.y = inv * v.y; v.z = inv * v.z; v.w = inv * v.w;
    }
    return EXVector(v);
}

// NEAR: ~45 bytes differ, register allocation of the vec_xyzw copies and the force add order.
void EX6DRigidBody::AddWorldForceAtPoint(const EXVector &f, const EXVector &p)
{
    EXVector t = (p - m_pos).Cross(f);
    EXVector lt = EXQuaternion(-m_rot.x, -m_rot.y, -m_rot.z, m_rot.w).MultiplyVector(t);
    lt.set_w0();
    AddLocalTorque(lt);
    float a = (f * m_mass).LengthXYZ();
    float b = (m_invInertia * lt).LengthXYZ();
    float d = a - b;
    if (d < 0.0f) d = 0.0f;
    AddWorldForce(UnitXYZ(f) * (m_invMass * d));
}

// NEAR: ~45 bytes differ, as AddWorldForceAtPoint.
void EX6DRigidBody::AddWorldImpulseAtPoint(const EXVector &f, const EXVector &p)
{
    EXVector t = (p - m_pos).Cross(f);
    EXVector lt = EXQuaternion(-m_rot.x, -m_rot.y, -m_rot.z, m_rot.w).MultiplyVector(t);
    lt.set_w0();
    AddLocalAngImpulse(lt);
    float a = (f * m_mass).LengthXYZ();
    float b = (m_invInertia * lt).LengthXYZ();
    float d = a - b;
    if (d < 0.0f) d = 0.0f;
    AddWorldImpulse(UnitXYZ(f) * (m_invMass * d));
}

void EX6DRigidBody::DoCollision(EXItemPhysics *pOther, EXCScoop_FaceData *pData)
{
    if (!(m_flags6D & 2)) return;
    // TODO layout: face data +0x10 / +0x20 (collision area): the two contact points.
    const EXVector &a = *(const EXVector *)&pData->m_00[0x10];
    const EXVector &b = *(const EXVector *)&pData->m_00[0x20];
    EXVector n = UnitXYZ(a - b);
    SetActive(1);
    AddWorldImpulseAtPoint(n * -m_50 * pData->m_dist, b);
}

void EX6DRigidBody::CollisionObject(u32 i, EXCollisionType &c)
{
    if (!m_pCollisionBody) return;
    c = m_pCollisionBody->CollisionObjects()[i];
    EXMatrix m;
    m.set_rotate_quat(m_rot);
    m.SetTranslation(m_pos);
    c.ApplyXForm(m);
}

float EX6DBodyGroup::UpdatedBoundsRadius(const EXVector &c, EXVector &out)
{
    float r = 0.0f;
    int n = m_bodies.Size();
    for (int i = 0; i < n; i++) {
        if (m_bodies[i]->CollisionBody()) m_bodies[i]->CollisionBody()->CheckBounds(c, r, out);
    }
    return r;
}

void EX6DBodyGroup::ClearForces()
{
    int n = m_bodies.Size();
    for (int i = 0; i < n; i++) {
        EX6DParticleBase *p = m_bodies[i];
        if (p->m_active && !(p->m_physicsFlags & 0x20)) p->ClearForces();
    }
}

void EX6DBodyGroup::Syncronise()
{
    int n = m_bodies.Size();
    for (int i = 0; i < n; i++) {
        EX6DParticleBase *p = m_bodies[i];
        if (p->m_active && !(p->m_physicsFlags & 0x20) && p->m_autoUpdate) p->Syncronise();
    }
}

void EX6DBodyGroup::SetActive(s8 b)
{
    m_active = b;
    if (b) m_physicsFlags &= ~0x20;
    else m_physicsFlags |= 0x20;
    int n = m_bodies.Size();
    for (int i = 0; i < n; i++) {
        EX6DParticleBase *p = m_bodies[i];
        if (p->m_active != b) p->SetActive(b);
    }
}

void EX6DRigid::UpdateMovement(float t)
{
    m_174 -= t;
    if (0.0f > m_174) {
        float w = m_angVel.LengthXYZ();
        m_pSamples[m_sampleIndex] = m_velocity.LengthXYZSquared() + w * 0.3f;
        m_sampleIndex++;
        if (m_sampleIndex >= m_numSamples) m_sampleIndex = 0;
        m_174 = m_178;
        if (m_170 > GetSampledMove()) {
            SetActive(0);
            return;
        }
    }
    EX6DRigidBody::UpdateMovement(t);
}

void EX6DBodyGroup::UpdateMovement(float t)
{
    if (m_pSamples) {
        m_88 -= t;
        if (0.0f > m_88) {
            float mx = 0.0f;
            int n = m_bodies.Size();
            for (int i = 0; i < n; i++) {
                EX6DRigidBody *p = (EX6DRigidBody *)m_bodies[i];
                float w = p->m_angVel.LengthXYZ();
                float m = p->m_velocity.LengthXYZSquared() + w * 0.3f;
                if (m > mx) mx = m;
            }
            m_pSamples[m_sampleIndex] = mx;
            m_sampleIndex++;
            if (m_sampleIndex >= m_numSamples) m_sampleIndex = 0;
            m_88 = m_8c;
            if (m_84 > GetSampledMove()) {
                SetActive(0);
                return;
            }
        }
    }
    int n = m_bodies.Size();
    for (int i = 0; i < n; i++) {
        EX6DParticleBase *p = m_bodies[i];
        if (p->m_active && !(p->m_physicsFlags & 0x20)) p->UpdateMovement(t);
    }
}

float EX6DAnim::GetTotalCollisions() const
{
    if (!m_active) return 0.0f;
    float t = 0.0f;
    int n = m_bodies.Size();
    for (int i = 0; i < n; i++) t += m_bodies.GetAt(i)->TotalCollisions();
    return t;
}

// World point into a body's local space (the second, uninlined copy of the transform).
static __forceinline void PointToLocal(EXVector &pt, const EXVector &p, const EXMatrix &m)
{
    pt = p;
    pt[3] = 1.0f;
    EXMaths::vec_xyzw out;
    D3DXVec4Transform(&out, &pt, &m);
    (EXMaths::vec_xyzw &)pt = out;
    pt.w = 0.0f;
}

void EX6DPinConstraint::SetWorldSpace(EX6DRigidBody *a, EX6DParticleBase *b, const EXVector &p)
{
    m_pBodyA = a;
    m_pBodyB = b;
    if (!a) return;
    EXMatrix m;
    m.set_rotate_quat(m_pBodyA->Rotation());
    m.SetTranslation(m_pBodyA->Position());
    m = m.FastInverse();
    m_pointA = p;
    m_pointA.w = 1.0f;
    (EXMaths::vec_xyzw &)m_pointA = m.mat_44::operator*(m_pointA);
    m_pointA.w = 0.0f;
    if (!b) {
        m_pointB = p;
        return;
    }
    if (b->IsKindOf(EXRUNTIME_CLASS(EX6DRigidBody))) {
        EX6DRigidBody *pRigid = (EX6DRigidBody *)b;
        m.set_rotate_quat(pRigid->Rotation());
        m.SetTranslation(pRigid->Position());
        m = m.FastInverse();
        PointToLocal(m_pointB, p, m);
    } else {
        m_pointB.SetZero();
    }
}

void EX6DConstraintType::SatisfyConstraint(float t)
{
    switch (m_type) {
    case 0: PinConstraint().SatisfyConstraint(); break;
    case 1: AngleConstraint().SatisfyConstraint(); break;
    case 2: WorldCollisionConstraint().SatisfyConstraint(t); break;
    case 3: InterCollisionConstraint().SatisfyConstraint(t); break;
    }
}

void EX6DManager::Cleanup()
{
    m_constraints.Empty(1);
    m_collisions.Empty(1);
    m_bodies.Empty(1);
}

void EX6DManager::RemoveConstraints(const EX6DObject *p)
{
    int nc = m_constraints.Size();
    int nk = m_collisions.Size();
    for (int i = 0; i < nc; i++) {
        if (m_constraints[i].BodyA() == p || m_constraints[i].BodyB() == p) {
            m_constraints.RemoveAt(i, 1);
            i--;
            nc--;
        }
    }
    for (int j = 0; j < nk; j++) {
        if (m_collisions[j].BodyA() == p || m_collisions[j].BodyB() == p) {
            m_collisions.RemoveAt(j, 1);
            j--;
            nk--;
        }
    }
}

void EX6DManager::RemoveBody(const EX6DObject *p)
{
    for (int i = 0; i < m_bodies.Size(); i++) {
        if (m_bodies[i] == p) m_bodies.RemoveAt(i, 1);
    }
    RemoveConstraints(p);
    if (!m_bodies.Size()) Cleanup();
}

void EX6DManager::ClearAllForces()
{
    int n = m_bodies.Size();
    for (int i = 0; i < n; i++) {
        EX6DObject *p = m_bodies[i];
        if (p->m_active && !(p->m_physicsFlags & 0x20)) p->ClearForces();
    }
}

void EX6DManager::UpdateAllMovements(float t)
{
    int n = m_bodies.Size();
    for (int i = 0; i < n; i++) {
        EX6DObject *p = m_bodies[i];
        if (p->m_active && !(p->m_physicsFlags & 0x20)) p->UpdateMovement(t);
    }
}

void EX6DManager::SyncroniseAll()
{
    int n = m_bodies.Size();
    for (int i = 0; i < n; i++) {
        EX6DObject *p = m_bodies[i];
        if (p->m_active && !(p->m_physicsFlags & 0x20) && p->m_autoUpdate) p->Syncronise();
    }
}

void EX6DManager::DetectAllCollisions(float t)
{
    int n = m_bodies.Size();
    for (int i = 0; i < n; i++) {
        EX6DObject *p = m_bodies[i];
        if (p->m_active && !(p->m_physicsFlags & 0x20) && !(p->m_physicsFlags & 0x08)) p->DoEnvironmentCollisions(t);
    }
}

void EX6DManager::SatisfyConstraints(float t)
{
    int nc = m_constraints.Size();
    int nk = m_collisions.Size();
    for (u32 it = 0; it < 10; it++) {
        for (int i = 0; i < nc; i++) {
            if (it < m_constraints[i].Iterations()) m_constraints[i].SatisfyConstraint(t);
        }
        for (int j = 0; j < nk; j++) {
            if (it < m_collisions[j].Iterations()) m_collisions[j].SatisfyConstraint(t);
        }
    }
    m_collisions.Empty(0);
}

void EX6DManager::DoStep(float t)
{
    u32 steps = 1;
    float dt = t;
    if (dt > m_34) {
        do {
            dt *= 0.5f;
            steps += steps;
        } while (dt > m_34);
    }
    for (u32 i = 0; i < steps; i++) {
        UpdateAllMovements(dt);
        ClearAllForces();
        DetectAllCollisions(dt);
        EXItemEnv::TheItemEnv()->UpdateItems_InterObjCollision();
        SatisfyConstraints(dt);
        SyncroniseAll();
    }
}

EX6DObject::~EX6DObject()
{
    if (m_pManager) m_pManager->RemoveBody(this);
}

void EX6DDamageModel::AddDamageArea(EX6DDamageArea &a, const u32 *pObjs, u32 n)
{
    u32 idx = m_areas.Size();
    m_areas.Add(a, 1);
    for (u32 i = 0; i < n; i++) m_pMap[pObjs[i]] = idx;
}

// TODO layout: damage extra data of a geo entity (XtraData type 5), geo area.
struct EX6DDamageXtra {
    u8 m_00;
    u8 m_count;                         // +0x01
    ex::EXRelPtr16<u8> m_entries;       // +0x02 0x30-byte entries, hash first
};

void EX6DDamageModel::AddDamageArea(EX6DDamageArea &a, const EXGeoBaseEntity *pEnt, u32 ref)
{
    u32 idx = m_areas.Size();
    m_areas.Add(a, 1);
    u8 *pXtra = (u8 *)pEnt->m_XtraData.GetXtra(5);
    if (!pXtra) return;
    EX6DDamageXtra *p = (EX6DDamageXtra *)(pXtra + 0xc);
    if (!p) return;
    if (p->m_count < 1) return;
    if (p->m_count > 100) return;
    for (u32 i = 0; i < p->m_count; i++) {
        u8 *pEntries = p->m_entries;
        if (*(u32 *)(pEntries + i * 0x30) == ref) m_pMap[i] = idx;
    }
}

void EX6DDamageModel::AddDamageAreaAll(EX6DDamageArea &a)
{
    u32 idx = m_areas.Size();
    m_areas.Add(a, 1);
    for (u32 i = 0; i < m_count; i++) m_pMap[i] = idx;
}

EX6DCollisionBody::~EX6DCollisionBody()
{
    if (m_pObjects) EXFree(m_pObjects);
    if (m_pObjectFlags) EXFree(m_pObjectFlags);
    if (m_pDamageModel) delete m_pDamageModel;
}

// NEAR: 40 bytes differ, follows from the inlined EX6DObject constructor's store order.
EX6DParticleBase::EX6DParticleBase()
{
    m_pos.SetZero();
    m_velocity.SetZero();
    m_dampVelocity = 0.9999f;
    m_pCollisionBody = 0;
    m_pParentItem = 0;
    m_mass = 1.0f;
    m_invMass = 1.0f;
    m_c0 = 0.0f;
    m_interObjVelocityLoss = 0.0f;
    m_coefRestitution = 0.0f;
    m_interObjFriction = 1.0f;
    m_groundFriction = 0.5f;
    m_gravityScale = 1.0f;
    m_active = 1;
    m_totalCollisions = 0.0f;
    m_impulse.SetZero();
    m_force.SetZero();
}

s8 EX6DRigid::GetEnclosingCollisionObject(EXCollisionType &c)
{
    EX6DCollisionBody *pBody = m_pCollisionBody;
    if (!pBody) return 0;
    EXSphere sphere;
    sphere.Set(pBody->BoundsCentre(), pBody->BoundsRadius());
    c.Set(sphere);
    return 1;
}

EX6DManager::~EX6DManager() { Cleanup(); }

void EX6DManager::AddConstraint(EX6DConstraintType &c) { m_constraints.Add(c, 1); }
void EX6DManager::AddCollision(EX6DConstraintType &c) { m_collisions.Add(c, 1); }

EX6DParticleBase::~EX6DParticleBase()
{
    if (m_pCollisionBody) delete m_pCollisionBody;
}

EX6DRigidBody::EX6DRigidBody()
{
    m_rot.Set(0.0f, 0.0f, 0.0f, 1.0f);
    m_angVel.Set(0.0f, 0.0f, 0.0f, 0.0f);
    m_invInertia.Set(1.0f, 1.0f, 1.0f, 1.0f);
    m_dampAngular = 0.999f;
    m_140 = 0.0f;
    m_interObjAngularLoss = 0.0f;
    m_148 = 1.0f;
}

void EX6DBodyGroup::AddBody(EX6DParticleBase *p)
{
    m_bodies.Add(p);
    m_bodies[m_bodies.Size() - 1]->ClearForces();
    p->SetManager(m_pManager);
    p->SetParent(this);
    m_active = 1;
}

EX6DParticle::EX6DParticle()
{
    m_pCollisionBody = new EX6DCollisionBody(this, 1);
    EXSphere sphere;
    sphere.Set(EXVector(0.0f, 0.0f, 0.0f, 0.0f), 1.0f);
    m_pCollisionBody->CollisionObjects()[0].Set(sphere);
    m_pCollisionBody->CalcBounds(0xf);
    m_autoUpdate = 1;
}

EX6DParticle::~EX6DParticle() {}

EX6DAnim::~EX6DAnim()
{
    if (m_pRigids) delete[] m_pRigids;
    if (m_pSamples) EXFree(m_pSamples);
}

void EX6DManager::AddBody(EX6DObject *p)
{
    m_bodies.Add(p);
    m_bodies[m_bodies.Size() - 1]->ClearForces();
    p->SetManager(this);
}

EX6DDamageModel::EX6DDamageModel(u32 n)
{
    if (!n) m_pMap = 0;
    else m_pMap = (u32 *)_EXAlloc(n * sizeof(u32), 0);
    m_count = n;
    for (u32 i = 0; i < m_count; i++) m_pMap[i] = 0xffffffff;
}

void EX6DCollisionBody::AddDamageModel() { m_pDamageModel = new EX6DDamageModel(m_numObjects); }

// NEAR: ~130 bytes differ, the compiler duplicates the entity scoop call into both arms of the header null test.
void EX6DParticleBase::DoEnvironmentCollisions(float t)
{
    CalcDeltas(t);
    EXCollisionScoopTemp temp;
    temp.Initialise();
    temp.m_xform.SetIdentity();
    FindCollisionBounds(temp, 2);
    if (m_pCollisionItem) {
        EXPlacementItem *pItem = m_pCollisionItem;
        temp.SetTemporaryVars(pItem, pItem->mtx(), 0);
        EXGeoEntityHeader *pHdr = pItem->GeoRefObject().GeoEntityHeader();
        EXGeoEntity *pEnt = pHdr->Object(0);
        pEnt->DoEntityScoop_Sphere(*(EXCScoopTemp *)&temp, 0, 1, 0);
    } else {
        EXItemEnv::TheItemEnv()->DoCollisionScoopToMap(temp);
        EXItemEnv::TheItemEnv()->DoCollisionScoopToContoured(temp, m_pItem);
    }
    if (temp.m_numEntities) DoCollisionFaceCheck(temp, t);
}

// NEAR: ~60 bytes differ, as EX6DParticleBase::DoEnvironmentCollisions (scoop call duplicated per header test).
void EX6DBodyGroup::DoEnvironmentCollisions(float t)
{
    int n = m_bodies.Size();
    int i;
    for (i = 0; i < n; i++) {
        EX6DParticleBase *p = m_bodies[i];
        if (p->m_active && !(p->m_physicsFlags & 0x20)) p->CalcDeltas(t);
    }
    EXCollisionScoopTemp temp;
    temp.Initialise();
    temp.m_xform.SetIdentity();
    FindCollisionBounds(temp, 2);
    m_boundsCentre = temp.m_centre;
    m_boundsRadius = temp.m_radius;
    if (temp.m_radius > 50.0f) {
        SetActive(0);
        return;
    }
    if (m_pCollisionItem) {
        EXPlacementItem *pItem = m_pCollisionItem;
        temp.SetTemporaryVars(pItem, pItem->mtx(), 0);
        EXGeoEntityHeader *pHdr = pItem->GeoRefObject().GeoEntityHeader();
        EXGeoEntity *pEnt = pHdr->Object(0);
        pEnt->DoEntityScoop_Sphere(*(EXCScoopTemp *)&temp, 0, 1, 0);
    } else {
        EXItemEnv::TheItemEnv()->DoCollisionScoopToMap(temp);
        EXItemEnv::TheItemEnv()->DoCollisionScoopToContoured(temp, m_pItem);
    }
    if (temp.m_numEntities) {
        for (i = 0; i < n; i++) {
            EX6DParticleBase *p = m_bodies[i];
            if (p->m_active && !(p->m_physicsFlags & 0x20)) p->DoCollisionFaceCheck(temp, t);
        }
    }
}

EX6DRigidTest::EX6DRigidTest()
{
    m_pCollisionBody = new EX6DCollisionBody(this, 1);
    EXBox box;
    box.m_centre.SetZero();
    box.m_axis[0] = EXVector(1.0f, 0.0f, 0.0f, 0.0f);
    box.m_axis[1] = EXVector(0.0f, 1.0f, 0.0f, 0.0f);
    box.m_extent[0] = 1.0f;
    box.m_extent[1] = 1.0f;
    box.m_extent[2] = 1.0f;
    m_pCollisionBody->CollisionObjects()[0].Set(box);
    m_pCollisionBody->CalcBounds(0xf);
    SetInertiaFromSides(2.0f, 2.0f, 2.0f);
}

EX6DRigidTest::~EX6DRigidTest() {}

EX6DRigid::EX6DRigid(const EXGeoBaseEntity *pEnt, s8 b)
{
    m_15c.SetZero();
    m_14c.SetZero();
    m_pSamples = 0;
    Initialise(pEnt, b, 0);
}

EX6DRigid::~EX6DRigid()
{
    if (m_pSamples) EXFree(m_pSamples);
}

s8 EX6DAnim::Initialise(EXItemAnimator_AnimModifier *pMod)
{
    if (!pMod->ModCtrl()) pMod->Set6DAnim(this);
    EXGeoHeader *pHdr = pMod->SkelRefObject().GeoFile()->GeoHeader();
    EXGeoAnimSkin *pSkin = pHdr->GeoAnimSkin(pMod->SkelRefObject().HashRef());
    const EXGeoAnimSkin_BonePhysicItem *pPhys = pSkin->BonePhysics();
    m_a0 = 0;
    m_pRigids = new EX6DRigidTest[pSkin->BonePhysicsCount()];
    if (!pMod->m_pSkin) return 0;
    EXMatrix m;
    m.SetIdentity();
    RecursiveInit(m, pMod->m_pSkin->SectionArray(), pPhys, -1, pSkin->BonePhysicsCount());
    return 1;
}

EX6DAnim::EX6DAnim()
{
    m_pRigids = 0;
    m_a0 = 0;
    m_flags = 0;
}

EX6DManager::EX6DManager()
{
    m_constraints.SetGrowBy(16);
    m_bodies.SetGrowBy(16);
    m_collisions.SetGrowBy(64);
    m_interReactivateDist = 0.01f;
    m_34 = 0.08f;
}

// Integrate one step: position from velocity and force, then velocity from force, damping and the
// (capped) impulse.
void EX6DParticleBase::UpdateMovement(float t)
{
    m_lastPos = m_pos;
    m_pos = m_pos + m_velocity * t + m_force * 0.5f * m_invMass * t * t;
    EXVector imp = m_impulse;
    float s = imp.LengthXYZ() * m_invMass;
    if (s > 80.0f) imp = imp * (80.0f / s);
    m_velocity = m_velocity * m_dampVelocity + m_force * m_invMass * t + imp * m_invMass;
}

// Push the particle out of a world contact along its normal (scaled by the constraint strength),
// then remove the velocity into the surface: friction on the tangential part, restitution on the
// normal part.
void EX6DParticleBase::SatisfyWorldCollision(EX6DWorldCollisionConstraint &c, float t)
{
    if (c.Ref() > CollisionBody()->NumObjects() - 1) return;
    EXCollisionType obj = CollisionBody()->CollisionObjects()[c.Ref()];
    switch (obj.Type()) {
    case 0:
        obj.Sphere().Centre() = m_pos + obj.Sphere().Centre();
        break;
    case 1:
        obj.Capsule().Segment().Origin() = m_pos + obj.Capsule().Segment().Origin();
        break;
    case 2:
        obj.Lozenge().Rectangle().Origin() = m_pos + obj.Lozenge().Rectangle().Origin();
        break;
    default:
        return;
    }
    EXVector nearest;
    obj.NearestTo(c.Point(), nearest);
    EXVector proj = c.Normal() * (nearest - c.Point()).Dot(c.Normal());
    float sq = proj.LengthXYZSquared();
    if (sq < 0.0f) sq = 0.0f;
    float pen;
    if (proj.Dot(c.Normal()) >= 0.0f) {
        if (obj.Radius() * obj.Radius() < sq) return;
        pen = obj.Radius() - EXSqrt(sq);
    } else {
        pen = obj.Radius() + EXSqrt(sq);
    }
    m_pos += c.Normal() * (c.Strength() * pen);
    float vn = m_velocity.Dot(c.Normal());
    if (vn < 0.0f) {
        m_totalCollisions -= vn;
        m_velocity -= (m_velocity - c.Normal() * vn) * m_groundFriction;
        m_velocity -= c.Normal() * vn * (m_coefRestitution + 1.0f);
    }
}

// Vector helpers retail expands inline in the big contact solvers (VC's inline budget runs out
// there otherwise).
static __forceinline float S6_Dot(const EXVector &a, const EXVector &b) { return b.w * a.w + b.z * a.z + b.y * a.y + b.x * a.x; }
static __forceinline float S6_Sqrt(float f) { return f >= 0.0f ? (float)sqrt(f) : 0.0f; }
static __forceinline EXVector S6_Cross(const EXVector &a, const EXVector &b)
{
    return EXVector(a.y * b.z - b.y * a.z, b.x * a.z - a.x * b.z, b.y * a.x - a.y * b.x, 0.0f);
}
static __forceinline EXVector S6_Mul(const EXVector &a, const EXVector &b)
{
    return EXVector(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
}
static __forceinline EXVector S6_Scale(const EXVector &a, float f) { return EXVector(a.x * f, a.y * f, a.z * f, a.w * f); }
static __forceinline EXVector S6_Add(const EXVector &a, const EXVector &b)
{
    return EXVector(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}
static __forceinline EXVector S6_Sub(const EXVector &a, const EXVector &b)
{
    return EXVector(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}
// Effective inverse mass term of an impulse along local direction d at local arm r.
static __forceinline float S6_AngularTerm(const EXVector &invInertia, const EXVector &r, const EXVector &d)
{
    float k = S6_Dot(d, S6_Mul(invInertia, S6_Cross(S6_Cross(r, d), r)));
    if (k < 0.0f) k = 0.0f - k;
    return k;
}

// Resolve a world contact for a rigid body. The penetration along the contact normal comes from the
// body point (bodies without flag 1) or from the touched collision object: a sphere by its centre's
// height over the plane, capsules and boxes by casting the normal through them (capsules more than
// 20 deep are ignored). The body is pushed out with the share of the effective mass at the contact
// (linear plus the angular term about the contact arm), and while closing on the surface the point
// velocity loses its normal part (restitution) and part of its tangential part (friction) through
// an impulse at the contact. The push also turns the body (halved again without flag 1); a hit
// counts towards the damage model and may bleed off linear (m_c0) and angular (m_140) speed.
void EX6DRigidBody::SatisfyWorldCollision(EX6DWorldCollisionConstraint &c, float t)
{
    EXQuaternion inv(0.0f - m_rot.x, 0.0f - m_rot.y, 0.0f - m_rot.z, m_rot.w);
    if (!CollisionBody()) return;
    if (!m_active) return;
    EXMatrix m;
    m.set_rotate_quat(m_rot);
    m.SetTranslation(m_pos);
    float pen;
    if (!(m_flags6D & 1)) {
        EXVector bp(c.m_bodyPoint.x, c.m_bodyPoint.y, c.m_bodyPoint.z, 1.0f);
        EXMaths::vec_xyzw o;
        D3DXVec4Transform(&o, &bp, &m);
        EXVector wp(o.x, o.y, o.z, 0.0f);
        pen = 0.0f - S6_Dot(S6_Sub(wp, c.m_point), c.m_normal);
    } else {
        if (c.m_ref > CollisionBody()->NumObjects() - 1) return;
        EXCollisionType obj = CollisionBody()->m_pObjects[c.m_ref];
        obj.ApplyXForm(m);
        if (obj.m_type == EXCollisionType::COL_SPHERE) {
            EXVector proj = S6_Scale(c.m_normal, S6_Dot(S6_Sub(obj.Origin(), c.m_point), c.m_normal));
            float sq = proj.y * proj.y + proj.z * proj.z + proj.x * proj.x;
            if (sq < 0.0f) sq = 0.0f;
            if (S6_Dot(proj, c.m_normal) >= 0.0f) {
                if (obj.Radius() * obj.Radius() < sq) return;
                pen = obj.Radius() - S6_Sqrt(sq);
            } else {
                pen = obj.Radius() + S6_Sqrt(sq);
            }
            goto resolve;
        }
        if (obj.m_type == EXCollisionType::COL_CAPSULE) {
            EXCapsule cap = obj.Capsule();
            float hits[2];
            int n = EXFindIntersection(c.m_point, c.m_normal, cap, hits);
            if (!n) return;
            int k = 0;
            if (n == 2 && hits[1] < hits[0]) k = 1;
            pen = 0.0f - hits[k];
            if (pen > 20.0f) return;
        } else if (obj.m_type == EXCollisionType::COL_BOX) {
            EXBox box = obj.Box();
            float hits[2];
            u32 n = EXFindIntersection(c.m_point, c.m_normal, box, hits);
            if (!n) return;
            int k = 0;
            if (n == 2 && hits[1] < hits[0]) k = 1;
            pen = 0.0f - hits[k];
        } else {
            return;
        }
    }
    if (pen < 0.0f) return;
resolve:
    {
        EXVector push = S6_Scale(c.m_normal, 0.0f - pen);
        EXVector arm = inv.MultiplyVector(S6_Sub(c.m_point, m_pos));
        float imp;
        if (!(m_flags6D & 1)) {
            EXVector ld = inv.MultiplyVector(UnitXYZ(push));
            imp = -1.0f / (m_invMass + S6_AngularTerm(m_invInertia, arm, ld));
        } else {
            imp = -1.0f / m_invMass;
        }
        float s = m_invMass * imp;
        m_pos.x = s * push.x + m_pos.x;
        m_pos.y = m_pos.y + s * push.y;
        m_pos.z = m_pos.z + s * push.z;
        m_pos.w = m_pos.w + s * push.w;
        EXVector vp = S6_Add(m_rot.MultiplyVector(S6_Cross(m_angVel, arm)), m_velocity);
        push = S6_Scale(push, imp);
        EXVector lp = inv.MultiplyVector(push);
        float vn = S6_Dot(vp, c.m_normal);
        s8 bHit = 0;
        float rate = 0.5f;
        if (vn < 0.0f) {
            float fric = 1.0f - m_groundFriction;
            float rest = 0.0f - m_coefRestitution * vn;
            bHit = 1;
            EXVector dv = S6_Sub(vp, S6_Add(S6_Scale(S6_Sub(vp, S6_Scale(c.m_normal, vn)), fric),
                                             S6_Scale(c.m_normal, rest)));
            EXVector ld = inv.MultiplyVector(UnitXYZ(dv));
            float imp2 = -1.0f / (m_invMass + S6_AngularTerm(m_invInertia, arm, ld));
            float s2 = m_invMass * imp2;
            m_velocity.x = s2 * dv.x + m_velocity.x;
            m_velocity.y = m_velocity.y + s2 * dv.y;
            m_velocity.z = m_velocity.z + s2 * dv.z;
            m_velocity.w = m_velocity.w + s2 * dv.w;
            dv = S6_Scale(dv, imp2);
            EXVector w = S6_Mul(m_invInertia, S6_Cross(arm, inv.MultiplyVector(dv)));
            if (!(m_flags6D & 1)) w = S6_Scale(w, 0.5f);
            m_angVel.x += w.x;
            m_angVel.y += w.y;
            m_angVel.z += w.z;
        }
        EXVector turn = S6_Mul(S6_Scale(m_invInertia, rate), S6_Cross(arm, lp));
        if (!(m_flags6D & 1)) turn = S6_Scale(turn, rate);
        EXAngleAxis aa;
        aa.x = turn.x;
        aa.y = turn.y;
        aa.z = turn.z;
        aa.w = S6_Sqrt(turn.x * turn.x + turn.y * turn.y + turn.z * turn.z);
        float len = S6_Sqrt(turn.x * turn.x + turn.y * turn.y + turn.z * turn.z);
        if (len > EX_EPSILON_UNIT) {
            float il = 1.0f / len;
            aa.x = il * aa.x;
            aa.y = aa.y * il;
            aa.z = aa.z * il;
        }
        EXQuaternion dq;
        dq.SetRotation(aa);
        m_rot = m_rot * dq;
        if (bHit) {
            m_totalCollisions -= vn;
            if (m_pCollisionBody && m_pCollisionBody->DamageModel())
                m_pCollisionBody->DamageModel()->Hit(c.m_ref, 0.0f - vn);
            if (m_c0 > EX_EPSILON_NORMALISE) {
                float f = 0.0f - vn * m_c0;
                if (f > 1.0f) f = 1.0f;
                f = 1.0f - f;
                m_velocity.x *= f; m_velocity.y *= f; m_velocity.z *= f; m_velocity.w *= f;
            }
            if (m_140 > EX_EPSILON_NORMALISE) {
                float f = 0.0f - m_140 * vn;
                if (f > 1.0f) f = 1.0f;
                f = 1.0f - f;
                m_angVel.x *= f; m_angVel.y *= f; m_angVel.z *= f; m_angVel.w *= f;
            }
        }
    }
}

// Separate two touching rigid bodies' collision objects: each moves half its share of the
// penetration (by effective mass at the contact), and while they close on each other the relative
// contact velocity loses its normal part (the larger restitution) and part of its tangential part
// (the smaller friction) through an impulse; the push and that impulse also turn both bodies. A hit
// counts towards both damage models and bleeds off speed by the mass ratio.
void EX6DInterCollisionConstraint::SatisfyConstraint(float t)
{
    if (!m_pBodyB->CollisionBody() || !m_pBodyA->CollisionBody()) return;
    EX6DRigidBody *pA = m_pBodyA;
    EX6DRigidBody *pB = m_pBodyB;
    EXQuaternion invA(0.0f - pA->m_rot.x, 0.0f - pA->m_rot.y, 0.0f - pA->m_rot.z, pA->m_rot.w);
    EXQuaternion invB(0.0f - pB->m_rot.x, 0.0f - pB->m_rot.y, 0.0f - pB->m_rot.z, pB->m_rot.w);
    EXMatrix mA;
    mA.set_rotate_quat(m_pBodyA->m_rot);
    mA.r[3].x = m_pBodyA->m_pos.x;
    mA.r[3].y = m_pBodyA->m_pos.y;
    mA.r[3].z = m_pBodyA->m_pos.z;
    mA.r[3].w = 1.0f;
    EXMatrix mB;
    mB.set_rotate_quat(m_pBodyB->m_rot);
    mB.r[3].x = m_pBodyB->m_pos.x;
    mB.r[3].y = m_pBodyB->m_pos.y;
    mB.r[3].z = m_pBodyB->m_pos.z;
    mB.r[3].w = 1.0f;
    EXCollisionType a = m_pBodyA->CollisionBody()->m_pObjects[m_objA];
    a.ApplyXForm(mA);
    EXCollisionType b = m_pBodyB->CollisionBody()->m_pObjects[m_objB];
    b.ApplyXForm(mB);
    EXVector pa, pb;
    float depth;
    if (!a.Intersection(b, pa, pb, depth)) return;
    EXVector n = UnitXYZ(S6_Sub(pb, pa));
    depth = 0.0f - depth;
    EXVector mid = S6_Scale(S6_Add(pa, pb), 0.5f);
    EXVector push = S6_Scale(n, depth);
    EXVector rA = invA.MultiplyVector(S6_Sub(mid, m_pBodyA->m_pos));
    EXVector rB = invB.MultiplyVector(S6_Sub(mid, m_pBodyB->m_pos));
    EXVector vA = S6_Add(m_pBodyA->m_rot.MultiplyVector(S6_Cross(m_pBodyA->m_angVel, rA)), m_pBodyA->m_velocity);
    EXVector vB = S6_Add(m_pBodyB->m_rot.MultiplyVector(S6_Cross(m_pBodyB->m_angVel, rB)), m_pBodyB->m_velocity);
    EXVector vrel = S6_Sub(vA, vB);
    EXVector dir = UnitXYZ(push);
    EXVector dA = invA.MultiplyVector(dir);
    EXVector dB = invB.MultiplyVector(dir);
    float kA = S6_AngularTerm(m_pBodyA->m_invInertia, rA, dA);
    float kB = S6_AngularTerm(m_pBodyB->m_invInertia, rB, dB);
    float imp = -1.0f / (m_pBodyB->m_invMass + kB + kA + m_pBodyA->m_invMass);
    float sA = imp * m_pBodyA->m_invMass * 0.5f;
    m_pBodyA->m_pos = S6_Add(m_pBodyA->m_pos, S6_Scale(push, sA));
    float sB = m_pBodyB->m_invMass * imp * 0.5f;
    m_pBodyB->m_pos = S6_Sub(m_pBodyB->m_pos, S6_Scale(push, sB));
    EXVector lA = invA.MultiplyVector(S6_Scale(push, imp));
    EXVector lB = invB.MultiplyVector(S6_Scale(push, 0.0f - imp));
    float vn = S6_Dot(vrel, n);
    EXVector dv(0.0f, 0.0f, 0.0f, 0.0f);
    float imp2;
    if (vn <= 0.0f) {
        float fric = m_pBodyB->m_interObjFriction <= m_pBodyA->m_interObjFriction ? m_pBodyB->m_interObjFriction
                                                                                  : m_pBodyA->m_interObjFriction;
        fric = 1.0f - fric;
        float rest = m_pBodyA->m_coefRestitution <= m_pBodyB->m_coefRestitution ? m_pBodyB->m_coefRestitution
                                                                                : m_pBodyA->m_coefRestitution;
        rest = 0.0f - vn * rest;
        EXVector tgt = S6_Add(S6_Scale(S6_Sub(vrel, S6_Scale(n, vn)), fric), S6_Scale(n, rest));
        dv = S6_Sub(vrel, tgt);
        EXVector d2 = UnitXYZ(dv);
        EXVector dA2 = invA.MultiplyVector(d2);
        EXVector dB2 = invB.MultiplyVector(d2);
        float kA2 = S6_AngularTerm(m_pBodyA->m_invInertia, rA, dA2);
        float kB2 = S6_AngularTerm(m_pBodyB->m_invInertia, rB, dB2);
        imp2 = -1.0f / (m_pBodyB->m_invMass + kB2 + kA2 + m_pBodyA->m_invMass);
        m_pBodyA->m_velocity = S6_Add(m_pBodyA->m_velocity, S6_Scale(dv, imp2 * m_pBodyA->m_invMass));
        m_pBodyB->m_velocity = S6_Sub(m_pBodyB->m_velocity, S6_Scale(dv, m_pBodyB->m_invMass * imp2));
    }
    EXVector wA = S6_Mul(S6_Scale(m_pBodyA->m_invInertia, 0.5f), S6_Cross(rA, lA));
    EXAngleAxis aaA;
    aaA.x = wA.x;
    aaA.y = wA.y;
    aaA.z = wA.z;
    aaA.w = S6_Sqrt(wA.y * wA.y + wA.z * wA.z + wA.x * wA.x);
    float lenA = S6_Sqrt(wA.y * wA.y + wA.z * wA.z + wA.x * wA.x);
    if (lenA > EX_EPSILON_UNIT) {
        float il = 1.0f / lenA;
        aaA.x *= il; aaA.y *= il; aaA.z *= il;
    }
    EXQuaternion qA;
    qA.SetRotation(aaA);
    EXVector wB = S6_Mul(S6_Scale(m_pBodyB->m_invInertia, 0.5f), S6_Cross(rB, lB));
    EXAngleAxis aaB;
    aaB.x = wB.x;
    aaB.y = wB.y;
    aaB.z = wB.z;
    aaB.w = S6_Sqrt(wB.y * wB.y + wB.z * wB.z + wB.x * wB.x);
    float lenB = S6_Sqrt(wB.y * wB.y + wB.z * wB.z + wB.x * wB.x);
    if (lenB > EX_EPSILON_UNIT) {
        float il = 1.0f / lenB;
        aaB.x *= il; aaB.y *= il; aaB.z *= il;
    }
    EXQuaternion qB;
    qB.SetRotation(aaB);
    m_pBodyA->m_rot = m_pBodyA->m_rot * qA;
    m_pBodyB->m_rot = m_pBodyB->m_rot * qB;
    if (vn <= 0.0f) {
        EXVector j = S6_Scale(dv, imp2);
        EXVector tA = S6_Mul(m_pBodyA->m_invInertia, S6_Cross(rA, invA.MultiplyVector(j)));
        m_pBodyA->m_angVel.x += tA.x;
        m_pBodyA->m_angVel.y += tA.y;
        m_pBodyA->m_angVel.z += tA.z;
        EXVector tB = S6_Mul(m_pBodyB->m_invInertia, S6_Cross(rB, invB.MultiplyVector(j)));
        m_pBodyB->m_angVel.x -= tB.x;
        m_pBodyB->m_angVel.y -= tB.y;
        m_pBodyB->m_angVel.z -= tB.z;
        m_pBodyA->m_totalCollisions -= vn;
        m_pBodyB->m_totalCollisions -= vn;
        if (m_pBodyA->m_pCollisionBody && m_pBodyA->m_pCollisionBody->m_pDamageModel)
            m_pBodyA->DamageModel()->Hit(m_objA, 0.0f - vn);
        if (m_pBodyB->m_pCollisionBody && m_pBodyB->m_pCollisionBody->m_pDamageModel)
            m_pBodyB->DamageModel()->Hit(m_objB, 0.0f - vn);
        float ra = m_pBodyA->m_invMass * m_pBodyB->m_mass;
        if (ra > 1.0f) ra = 1.0f;
        if (m_pBodyA->m_interObjVelocityLoss > EX_EPSILON_NORMALISE) {
            float f = 0.0f - ra * m_pBodyA->m_interObjVelocityLoss * vn;
            if (f > 1.0f) f = 1.0f;
            m_pBodyA->m_velocity = S6_Scale(m_pBodyA->m_velocity, 1.0f - f);
        }
        if (m_pBodyA->m_interObjAngularLoss > EX_EPSILON_NORMALISE) {
            float f = 0.0f - ra * m_pBodyA->m_interObjAngularLoss * vn;
            if (f > 1.0f) f = 1.0f;
            m_pBodyA->m_angVel = S6_Scale(m_pBodyA->m_angVel, 1.0f - f);
        }
        float rb = m_pBodyA->m_mass * m_pBodyB->m_invMass;
        if (rb > 1.0f) rb = 1.0f;
        if (m_pBodyB->m_interObjVelocityLoss > EX_EPSILON_NORMALISE) {
            float f = 0.0f - rb * vn * m_pBodyB->m_interObjVelocityLoss;
            if (f > 1.0f) f = 1.0f;
            m_pBodyB->m_velocity = S6_Scale(m_pBodyB->m_velocity, 1.0f - f);
        }
        if (m_pBodyB->m_interObjAngularLoss > EX_EPSILON_NORMALISE) {
            float f = 0.0f - m_pBodyB->m_interObjAngularLoss * rb * vn;
            if (f > 1.0f) f = 1.0f;
            m_pBodyB->m_angVel = S6_Scale(m_pBodyB->m_angVel, 1.0f - f);
        }
    }
}

// Turn a body by the rotation vector w (angle |w| about w) on the right of its orientation.
static __forceinline void S6_Turn(EX6DRigidBody *p, const EXVector &w)
{
    EXAngleAxis aa;
    aa.x = w.x;
    aa.y = w.y;
    aa.z = w.z;
    aa.w = S6_Sqrt(w.y * w.y + w.z * w.z + w.x * w.x);
    float len = S6_Sqrt(w.y * w.y + w.z * w.z + w.x * w.x);
    if (len > EX_EPSILON_UNIT) {
        float il = 1.0f / len;
        aa.x *= il; aa.y *= il; aa.z *= il;
    }
    EXQuaternion q;
    q.SetRotation(aa);
    p->m_rot = p->m_rot * q;
}

// Hold body A's point (m_pointA, body space) on the anchor: a world point (no body B), body B's
// point (m_pointB, body space of a rigid B) or a particle B's position. The bodies are pulled
// together by m_strength of the effective-mass share of the gap, the relative velocity at the pin
// is removed by an impulse, and the pull turns rigid bodies too. Sleeping pairs only wake once the
// gap passes 0.1. A particle B moves by its mass share instead of an inverse mass.
void EX6DPinConstraint::SatisfyConstraint()
{
    EX6DRigidBody *pA = m_pBodyA;
    if (!m_pBodyB) {
        if (!pA->m_active) return;
        EXVector wa = S6_Add(pA->m_rot.MultiplyVector(m_pointA), pA->m_pos);
        EXQuaternion inv(0.0f - m_pBodyA->m_rot.x, 0.0f - m_pBodyA->m_rot.y, 0.0f - m_pBodyA->m_rot.z, m_pBodyA->m_rot.w);
        EXVector d = S6_Sub(wa, m_pointB);
        EXVector ld = inv.MultiplyVector(UnitXYZ(d));
        float imp = -1.0f / (m_pBodyA->m_invMass + S6_AngularTerm(m_pBodyA->m_invInertia, m_pointA, ld));
        m_pBodyA->m_pos = S6_Add(m_pBodyA->m_pos, S6_Scale(d, m_pBodyA->m_invMass * m_strength * imp));
        EXVector lj = inv.MultiplyVector(S6_Scale(d, imp));
        EXVector vp = S6_Add(m_pBodyA->m_rot.MultiplyVector(S6_Cross(m_pBodyA->m_angVel, m_pointA)), m_pBodyA->m_velocity);
        EXVector lv = inv.MultiplyVector(UnitXYZ(vp));
        float imp2 = -1.0f / (m_pBodyA->m_invMass + S6_AngularTerm(m_pBodyA->m_invInertia, m_pointA, lv));
        m_pBodyA->m_velocity = S6_Add(m_pBodyA->m_velocity, S6_Scale(vp, m_pBodyA->m_invMass * imp2));
        S6_Turn(m_pBodyA, S6_Mul(S6_Scale(m_pBodyA->m_invInertia, m_strength), S6_Cross(m_pointA, lj)));
        EXVector w = S6_Mul(m_pBodyA->m_invInertia, S6_Cross(m_pointA, inv.MultiplyVector(S6_Scale(vp, imp2))));
        m_pBodyA->m_angVel.x += w.x;
        m_pBodyA->m_angVel.y += w.y;
        m_pBodyA->m_angVel.z += w.z;
        return;
    }
    if (!pA->m_active && !m_pBodyB->m_active) return;
    if (m_pBodyB->IsKindOf(EXRUNTIME_CLASS(EX6DRigidBody))) {
        EX6DRigidBody *pB = (EX6DRigidBody *)m_pBodyB;
        EXVector wa = S6_Add(m_pBodyA->m_rot.MultiplyVector(m_pointA), m_pBodyA->m_pos);
        EXVector wb = S6_Add(pB->m_rot.MultiplyVector(m_pointB), pB->m_pos);
        EXQuaternion invA(0.0f - m_pBodyA->m_rot.x, 0.0f - m_pBodyA->m_rot.y, 0.0f - m_pBodyA->m_rot.z, m_pBodyA->m_rot.w);
        EXQuaternion invB(0.0f - pB->m_rot.x, 0.0f - pB->m_rot.y, 0.0f - pB->m_rot.z, pB->m_rot.w);
        EXVector d = S6_Sub(wa, wb);
        if (!m_pBodyA->m_active || !pB->m_active) {
            if (d.x * d.x + d.y * d.y + d.z * d.z <= 0.01f) return;
            m_pBodyA->SetActive(1);
            pB->SetActive(1);
        }
        EXVector dir = UnitXYZ(d);
        EXVector dA = invA.MultiplyVector(dir);
        EXVector dB = invB.MultiplyVector(dir);
        float kA = S6_AngularTerm(m_pBodyA->m_invInertia, m_pointA, dA);
        float kB = S6_AngularTerm(pB->m_invInertia, m_pointB, dB);
        float imp = -1.0f / (kB + kA + m_pBodyA->m_invMass + pB->m_invMass);
        m_pBodyA->m_pos = S6_Add(m_pBodyA->m_pos, S6_Scale(d, imp * m_pBodyA->m_invMass * m_strength));
        pB->m_pos = S6_Sub(pB->m_pos, S6_Scale(d, imp * pB->m_invMass * m_strength));
        EXVector la = invA.MultiplyVector(S6_Scale(d, imp));
        EXVector lb = invB.MultiplyVector(S6_Scale(d, 0.0f - imp));
        EXVector va = S6_Add(m_pBodyA->m_rot.MultiplyVector(S6_Cross(m_pBodyA->m_angVel, m_pointA)), m_pBodyA->m_velocity);
        EXVector vb = S6_Add(pB->m_rot.MultiplyVector(S6_Cross(pB->m_angVel, m_pointB)), pB->m_velocity);
        EXVector vrel = S6_Sub(va, vb);
        EXVector dv = UnitXYZ(vrel);
        EXVector dA2 = invA.MultiplyVector(dv);
        EXVector dB2 = invB.MultiplyVector(dv);
        float kA2 = S6_AngularTerm(m_pBodyA->m_invInertia, m_pointA, dA2);
        float kB2 = S6_AngularTerm(pB->m_invInertia, m_pointB, dB2);
        float imp2 = -1.0f / (m_pBodyA->m_invMass + kB2 + kA2 + pB->m_invMass);
        m_pBodyA->m_velocity = S6_Add(m_pBodyA->m_velocity, S6_Scale(vrel, m_pBodyA->m_invMass * imp2));
        pB->m_velocity = S6_Sub(pB->m_velocity, S6_Scale(vrel, imp2 * pB->m_invMass));
        S6_Turn(m_pBodyA, S6_Mul(S6_Scale(m_pBodyA->m_invInertia, m_strength), S6_Cross(m_pointA, la)));
        S6_Turn(pB, S6_Mul(S6_Scale(pB->m_invInertia, m_strength), S6_Cross(m_pointB, lb)));
        invA.Set(0.0f - m_pBodyA->m_rot.x, 0.0f - m_pBodyA->m_rot.y, 0.0f - m_pBodyA->m_rot.z, m_pBodyA->m_rot.w);
        invB.Set(0.0f - pB->m_rot.x, 0.0f - pB->m_rot.y, 0.0f - pB->m_rot.z, pB->m_rot.w);
        EXVector j = S6_Scale(vrel, imp2);
        EXVector wA = S6_Mul(m_pBodyA->m_invInertia, S6_Cross(m_pointA, invA.MultiplyVector(j)));
        m_pBodyA->m_angVel.x += wA.x;
        m_pBodyA->m_angVel.y += wA.y;
        m_pBodyA->m_angVel.z += wA.z;
        EXVector wB = S6_Mul(pB->m_invInertia, S6_Cross(m_pointB, invB.MultiplyVector(j)));
        pB->m_angVel.x -= wB.x;
        pB->m_angVel.y -= wB.y;
        pB->m_angVel.z -= wB.z;
        return;
    }
    EXVector wa = S6_Add(m_pBodyA->m_rot.MultiplyVector(m_pointA), m_pBodyA->m_pos);
    EXQuaternion inv(0.0f - m_pBodyA->m_rot.x, 0.0f - m_pBodyA->m_rot.y, 0.0f - m_pBodyA->m_rot.z, m_pBodyA->m_rot.w);
    EX6DParticleBase *pB = m_pBodyB;
    EXVector d = S6_Sub(wa, pB->m_pos);
    if (!m_pBodyA->m_active || !pB->m_active) {
        if (d.x * d.x + d.y * d.y + d.z * d.z <= 0.01f) return;
        m_pBodyA->SetActive(1);
        m_pBodyB->SetActive(1);
    }
    EXVector ld = inv.MultiplyVector(UnitXYZ(d));
    float imp = -1.0f / (m_pBodyB->m_invMass + S6_AngularTerm(m_pBodyA->m_invInertia, m_pointA, ld) + m_pBodyA->m_invMass);
    m_pBodyA->m_pos = S6_Add(m_pBodyA->m_pos, S6_Scale(d, imp * m_pBodyA->m_invMass * m_strength));
    float share = m_pBodyA->m_mass / (m_pBodyB->m_mass + m_pBodyA->m_mass);
    m_pBodyB->m_pos = S6_Add(m_pBodyB->m_pos, S6_Scale(d, share * m_strength));
    EXVector lj = inv.MultiplyVector(S6_Scale(d, imp));
    EXVector vrel = S6_Sub(S6_Add(m_pBodyA->m_rot.MultiplyVector(S6_Cross(m_pBodyA->m_angVel, m_pointA)), m_pBodyA->m_velocity),
                           m_pBodyB->m_velocity);
    EXVector lv = inv.MultiplyVector(UnitXYZ(vrel));
    float imp2 = -1.0f / (m_pBodyB->m_invMass + S6_AngularTerm(m_pBodyA->m_invInertia, m_pointA, lv) + m_pBodyA->m_invMass);
    m_pBodyA->m_velocity = S6_Add(m_pBodyA->m_velocity, S6_Scale(vrel, imp2 * m_pBodyA->m_invMass));
    m_pBodyB->m_velocity = S6_Add(m_pBodyB->m_velocity, S6_Scale(vrel, share));
    S6_Turn(m_pBodyA, S6_Mul(S6_Scale(m_pBodyA->m_invInertia, m_strength), S6_Cross(m_pointA, lj)));
    EXVector w = S6_Mul(m_pBodyA->m_invInertia, S6_Cross(m_pointA, inv.MultiplyVector(S6_Scale(vrel, imp2))));
    m_pBodyA->m_angVel.x += w.x;
    m_pBodyA->m_angVel.y += w.y;
    m_pBodyA->m_angVel.z += w.z;
}

static __forceinline EXVector Rag_UnitVector7(const EXVector &a)
{
    EXMaths::vec_xyzw v = a;
    float len = v.Length();
    if (len > EX_EPSILON_NORMALISE) {
        float inv = 1.0f / len;
        v.x = inv * v.x; v.y = inv * v.y; v.z = inv * v.z; v.w = inv * v.w;
    }
    return EXVector(v);
}

// A kept contact while picking the deepest ones (DoCollisionFaceCheck).
struct S6_Contact {
    float m_depth;
    u32 m_index;    // face (per face mode) or collision object (per object mode)
    u32 m_face;     // per object mode: the last face touching the object
};

// Insert depth/index into the list of the n deepest contacts (deepest first).
static __forceinline void S6_InsertContact(S6_Contact *best, u32 n, float depth, u32 index)
{
    for (u32 k = 0; k < n; k++) {
        if (depth > best[k].m_depth) {
            for (u32 s = n - 1; s > k; s--) {
                if (best[s - 1].m_depth > -1.0f) {
                    best[s].m_depth = best[s - 1].m_depth;
                    best[s].m_index = best[s - 1].m_index;
                    best[s].m_face = best[s - 1].m_face;
                }
            }
            best[k].m_depth = depth;
            best[k].m_index = index;
            return;
        }
    }
}

// Direction of v's x/y/z (w 0), unit when longer than EX_EPSILON_UNIT; returns the length.
static __forceinline float S6_DirXYZ(const EXVector &v, EXVector &dir)
{
    float len = S6_Sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    dir.Set(v.x, v.y, v.z, 0.0f);
    if (len > EX_EPSILON_UNIT) {
        float inv = 1.0f / len;
        dir.Set(v.x * inv, v.y * inv, v.z * inv, inv * 0.0f);
    }
    return len;
}

static inline void S6_DebugSphere(const EXVector &, float) {}

// Gather the map faces touching the body's collision objects and turn the deepest into world
// collision constraints (at most m_maxContacts, up to 8). Fast bodies sweep back along the step's
// movement (m_extraCollisions sub positions): particles stop at the first position with contacts,
// rigid bodies collect them all, objects already hit being switched off (type + 10) for the later
// positions. For bodies without flag 1 each face contact's body point is kept in body space. Bodies
// with flag 1 and a sphere first object instead take one contact per collision object, its normal
// the depth weighted sum of the separating directions of the object's faces (faces pointing the
// other way reflected), placed on the object's surface.
void EX6DCollisionBody::DoCollisionFaceCheck(EXCollisionScoopTemp &t, float f)
{
    t.m_numFaces = 0;
    t.m_numObjects = m_numObjects;
    t.m_pObjects = m_pObjects;
    s8 bPerFace = 0;
    if (m_pObjects[0].m_type != EXCollisionType::COL_SPHERE) bPerFace = 1;
    if (!(m_pRigidBody->m_flags6D & 1)) bPerFace = 1;
    EXMatrix m;
    if (m_pRigidBody) {
        m.set_rotate_quat(m_pRigidBody->m_rot);
        m.r[3].x = m_pRigidBody->m_pos.x;
        m.r[3].y = m_pRigidBody->m_pos.y;
        m.r[3].z = m_pRigidBody->m_pos.z;
    } else {
        m.SetIdentity();
        m.r[3].x = m_pParticle->m_pos.x;
        m.r[3].y = m_pParticle->m_pos.y;
        m.r[3].z = m_pParticle->m_pos.z;
    }
    m.r[3].w = 1.0f;
    EXMatrix inv;
    if (m_extraCollisions) {
        if (m_pRigidBody) {
            u32 done = 0;
            for (u32 i = 0; i <= m_extraCollisions; i++) {
                float k = 1.0f - (float)(i + 1) / (float)(m_extraCollisions + 1);
                EXVector pos = S6_Sub(m_pRigidBody->m_pos, S6_Scale(m_deltaPos, k));
                EXAngleAxis aa;
                aa.set(m_30.x, m_30.y, m_30.z, 0.0f - m_30.w * k);
                EXQuaternion q;
                q.SetRotation(aa);
                q = m_pRigidBody->m_rot * q;
                t.m_mtx.set_rotate_quat(q);
                t.m_mtx.r[3] = pos;
                t.m_mtx.r[3].w = 1.0f;
                t.m_centre = m_10;
                t.m_centre.w = 1.0f;
                EXMaths::vec_xyzw c;
                D3DXVec4Transform(&c, &t.m_centre, &t.m_mtx);
                (EXMaths::vec_xyzw &)t.m_centre = c;
                t.m_centre.w = 0.0f;
                t.m_radius = m_boundsRadius;
                t.CalculateFaceArray();
                if (!t.m_numFaces) continue;
                if (!(m_pRigidBody->m_flags6D & 1)) inv = t.m_mtx.FastInverse();
                for (u32 j = done; j < t.m_numFaces; j++) {
                    EXCScoop_FaceData &fd = t.m_pFaces[j];
                    if (!(m_pRigidBody->m_flags6D & 1)) {
                        EXVector dir;
                        float len = S6_DirXYZ(S6_Sub(fd.P0(), fd.P1()), dir);
                        if (!(fd.FaceFlags() & 0x100) && S6_Dot(fd.Normal(), dir) < 0.0f) fd.m_dist = -1.0f;
                        if (len > 1.1920929e-7f) fd.Normal() = dir;
                        fd.P0() = S6_Sub(fd.P1(), S6_Scale(fd.Normal(), fd.m_dist));
                        fd.m_dist = (float)m_extraCollisions * (1.0f - k) * m_64 + fd.m_dist;
                        fd.P0().w = 1.0f;
                        EXMaths::vec_xyzw o;
                        D3DXVec4Transform(&o, &fd.P0(), &inv);
                        (EXMaths::vec_xyzw &)fd.P0() = o;
                        fd.P0().w = 0.0f;
                    } else {
                        EXCollisionType &o = t.m_pObjects[fd.m_3e & 0x7f];
                        if (o.m_type < 10) o.m_type = (EXCollisionType::ColType)(m_pObjects[fd.m_3e & 0x7f].m_type + 10);
                    }
                }
                done = t.m_numFaces;
            }
            for (u32 n = 0; n < m_numObjects; n++)
                if (m_pObjects[n].m_type > 9) m_pObjects[n].m_type = (EXCollisionType::ColType)(m_pObjects[n].m_type - 10);
        } else {
            for (u32 i = 0; i <= m_extraCollisions; i++) {
                float k = 1.0f - (float)(i + 1) / (float)(m_extraCollisions + 1);
                EXVector pos = S6_Sub(m_pParticle->m_pos, S6_Scale(m_deltaPos, k));
                t.m_mtx.SetIdentity();
                t.m_mtx.r[3] = pos;
                t.m_mtx.r[3].w = 1.0f;
                t.m_centre = S6_Add(pos, m_10);
                t.m_radius = m_boundsRadius;
                t.CalculateFaceArray();
                if (t.m_numFaces) break;
            }
        }
    } else {
        t.m_mtx = m;
        t.m_centre = m_boundsCentre;
        t.m_radius = m_boundsRadius;
        t.CalculateFaceArray();
        if (!(m_pRigidBody->m_flags6D & 1) && t.m_numFaces) {
            inv = t.m_mtx.FastInverse();
            for (u32 i = 0; i < t.m_numFaces; i++) {
                EXCScoop_FaceData &fd = t.m_pFaces[i];
                EXVector dir;
                float len = S6_DirXYZ(S6_Sub(fd.P0(), fd.P1()), dir);
                if (len > 1.1920929e-7f) fd.Normal() = dir;
                fd.P0() = S6_Sub(fd.P1(), S6_Scale(fd.Normal(), fd.m_dist));
                fd.P0().w = 1.0f;
                EXMaths::vec_xyzw o;
                D3DXVec4Transform(&o, &fd.P0(), &inv);
                (EXMaths::vec_xyzw &)fd.P0() = o;
                fd.P0().w = 0.0f;
            }
        }
    }
    if (EXBaseRenderEnv::RenderScoopCollision()) {
        for (u32 n = 0; n < m_numObjects; n++) S6_DebugSphere(t.m_pObjects[n].Origin(), t.m_pObjects[n].Radius());
    }
    if (!t.m_numFaces) return;
    S6_Contact best[8];
    EX6DParticleBase *pBody = m_pRigidBody ? (EX6DParticleBase *)m_pRigidBody : m_pParticle;
    if (bPerFace) {
        u32 maxC = m_maxContacts;
        if (maxC > 8) maxC = 8;
        for (u32 b = 0; b < maxC; b++) best[b].m_depth = -3.4028235e38f;
        u32 count = 0;
        for (u32 j = 0; j < t.m_numFaces; j++) {
            if (t.m_pFaces[j].m_dist > 0.0f) {
                count++;
                S6_InsertContact(best, maxC, t.m_pFaces[j].m_dist, j);
            }
        }
        if (!count) return;
        if (count > maxC) count = maxC;
        for (u32 c = 0; c < count; c++) {
            EX6DWorldCollisionConstraint wc;
            EXCScoop_FaceData &fd = t.m_pFaces[best[c].m_index];
            if (!(m_pRigidBody->m_flags6D & 1)) {
                wc.Set(pBody, fd.P1(), fd.Normal(), fd.m_3e & 0x7f);
                wc.SetBodyPoint(fd.P0());
            } else {
                EXVector dir;
                float len = S6_DirXYZ(S6_Sub(fd.P0(), fd.P1()), dir);
                if (len < 1.1920929e-7f) dir = fd.Normal();
                wc.Set(pBody, fd.P1(), dir, fd.m_3e & 0x7f);
            }
            EX6DConstraintType ct;
            ct.Set(wc);
            ct.SetIterations(pBody->m_iterations);
            pBody->m_pManager->m_collisions.Add(ct, 1);
        }
        if (!m_pRigidBody) {
            m_pParticle->DoMapCollisionCallBack(&t.m_pFaces[best[0].m_index]);
            return;
        }
        m_pRigidBody->DoMapCollisionCallBack(&t.m_pFaces[best[0].m_index]);
        return;
    }
    float *pObjDepth = (float *)m_pObjectFlags;
    for (u32 n = 0; n < m_numObjects; n++) pObjDepth[n] = -3.4028235e38f;
    for (u32 j = 0; j < t.m_numFaces; j++) {
        u32 o = t.m_pFaces[j].m_3e & 0x7f;
        if (t.m_pFaces[j].m_dist > pObjDepth[o]) pObjDepth[o] = t.m_pFaces[j].m_dist;
    }
    u32 maxC = m_maxContacts;
    if (maxC > 8) maxC = 8;
    for (u32 b = 0; b < maxC; b++) best[b].m_depth = -3.4028235e38f;
    u32 count = 0;
    for (u32 n2 = 0; n2 < m_numObjects; n2++) {
        if (pObjDepth[n2] > 0.0f) {
            count++;
            S6_InsertContact(best, maxC, pObjDepth[n2], n2);
        }
    }
    if (count > maxC) count = maxC;
    EXVector normals[8];
    for (u32 c = 0; c < count; c++) normals[c].Set(0.0f, 0.0f, 0.0f, 0.0f);
    for (u32 j = 0; j < t.m_numFaces; j++) {
        EXCScoop_FaceData &fd = t.m_pFaces[j];
        for (u32 c = 0; c < count; c++) {
            if ((fd.m_3e & 0x7f) != best[c].m_index) continue;
            EXVector d = Rag_UnitVector7(S6_Sub(fd.P0(), fd.P1()));
            EXVector n = fd.Normal();
            if (fd.m_3e & 0x80) n.Set(0.0f - n.x, 0.0f - n.y, 0.0f - n.z, 0.0f - n.w);
            float dot = S6_Dot(n, d);
            EXVector v = d;
            if (dot < -0.95f && !(fd.FaceFlags() & 0x100)) v = S6_Sub(d, S6_Scale(n, dot * 2.0f));
            v = S6_Scale(v, fd.m_dist);
            normals[c] = S6_Add(normals[c], v);
            best[c].m_face = j;
        }
    }
    if (best[0].m_face >= t.m_numFaces) return;
    pBody->DoMapCollisionCallBack(&t.m_pFaces[best[0].m_face]);
    EXMatrix bm;
    if (m_pRigidBody) {
        bm.set_rotate_quat(m_pRigidBody->m_rot);
        bm.r[3].x = m_pRigidBody->m_pos.x;
        bm.r[3].y = m_pRigidBody->m_pos.y;
        bm.r[3].z = m_pRigidBody->m_pos.z;
    } else {
        bm.SetIdentity();
        bm.r[3].x = m_pParticle->m_pos.x;
        bm.r[3].y = m_pParticle->m_pos.y;
        bm.r[3].z = m_pParticle->m_pos.z;
    }
    bm.r[3].w = 1.0f;
    for (u32 c2 = count; c2 != 0; c2--) {
        S6_Contact &bc = best[c2 - 1];
        EXVector &n = normals[c2 - 1];
        EXCScoop_FaceData &fd = t.m_pFaces[bc.m_face];
        n.Normalise();
        EXCollisionType &obj = m_pObjects[bc.m_index];
        EXVector point;
        if (obj.m_type == EXCollisionType::COL_CAPSULE) {
            EXCollisionType o = obj;
            o.ApplyXForm(bm);
            EXVector axis = o.Capsule().Direction();
            axis.Normalise();
            float d = S6_Dot(n, axis);
            if (d < -0.01f) {
                float r = o.Radius() - bc.m_depth;
                point = S6_Sub(S6_Add(o.Origin(), o.Capsule().Direction()), S6_Scale(n, r));
            } else if (d > 0.01f) {
                float r = o.Radius() - bc.m_depth;
                point = S6_Sub(o.Origin(), S6_Scale(n, r));
            } else {
                float r = o.Radius() - bc.m_depth;
                point = S6_Sub(fd.P0(), S6_Scale(n, r));
            }
        } else {
            float r = obj.Radius() - bc.m_depth;
            point = S6_Sub(fd.P0(), S6_Scale(n, r));
        }
        EX6DWorldCollisionConstraint wc;
        wc.Set(pBody, point, n, bc.m_index);
        EX6DConstraintType ct;
        ct.Set(wc);
        ct.SetIterations(pBody->m_iterations);
        pBody->m_pManager->m_collisions.Add(ct, 1);
    }
}

// Per-step movement of the body: the position delta, the rotation delta as an angle/axis (both
// decide how many extra sub-collisions are needed to avoid tunnelling), and the world bounds centre.
void EX6DCollisionBody::CalcDeltas()
{
    if (m_pRigidBody) {
        m_deltaPos = m_pRigidBody->m_pos - m_pRigidBody->m_lastPos;
        EXMaths::quat_xyzw dq;
        const EXQuaternion &lr = m_pRigidBody->m_lastRot;
        dq.set(-lr.x, -lr.y, -lr.z, lr.w);
        dq *= m_pRigidBody->m_rot;
        ((EXAngleAxis &)m_30).SetRotation(EXQuaternion(dq));
        float angle = m_30.w;
        if (angle < 0.0f) angle = -angle;
        m_extraCollisions = (u32)((angle * m_68 + m_deltaPos.LengthXYZ()) / (m_64 * 0.75f));
        EXMatrix m;
        m.set_rotate_quat(m_pRigidBody->m_rot);
        m.SetTranslation(m_pRigidBody->m_pos);
        m_boundsCentre.Set(m_10.x, m_10.y, m_10.z, 1.0f);
        EXMaths::vec_xyzw out;
        D3DXVec4Transform(&out, &m_boundsCentre, &m);
        (EXMaths::vec_xyzw &)m_boundsCentre = out;
        m_boundsCentre.w = 0.0f;
        return;
    }
    m_deltaPos = m_pParticle->m_pos - m_pParticle->m_lastPos;
    m_extraCollisions = (u32)(m_deltaPos.LengthXYZ() / (m_64 * 0.95f));
    m_boundsCentre = m_10 + m_pParticle->m_pos;
}

// Grow the bounding radius r about c to cover this body (and, when it moved far this step, its
// in-between positions); out receives the offset to the furthest one.
void EX6DCollisionBody::CheckBounds(const EXVector &c, float &r, EXVector &out)
{
    EXVector v(0.0f, 0.0f, 0.0f, 0.0f);
    if (m_extraCollisions) {
        if (m_pRigidBody) {
            for (u32 i = 0; i < m_extraCollisions; i++) {
                float k = 1.0f - (float)(i + 1) / (float)(m_extraCollisions + 1);
                EXVector pos = m_pRigidBody->m_pos - m_deltaPos * k;
                EXAngleAxis aa;
                aa.set(m_30.x, m_30.y, m_30.z, -(m_30.w * k));
                EXQuaternion q;
                q.SetRotation(aa);
                q = m_pRigidBody->m_rot * q;
                EXMatrix m;
                m.set_rotate_quat(q);
                m.SetTranslation(pos);
                v.Set(m_10.x, m_10.y, m_10.z, 1.0f);
                EXMaths::vec_xyzw o;
                D3DXVec4Transform(&o, &v, &m);
                v.Set(o.x - c.x, o.y - c.y, o.z - c.z, 0.0f - c.w);
                float dist = v.Length() + m_boundsRadius;
                if (dist > r) {
                    r = dist;
                    out = v;
                }
            }
        } else {
            float k = 1.0f - 1.0f / (float)(m_extraCollisions + 1);
            EXVector d = m_pParticle->m_pos - m_deltaPos * k - c;
            float dist = d.Length() + m_boundsRadius;
            if (dist > r) {
                r = dist;
                out = d;
            }
        }
    }
    EXVector d = m_boundsCentre - c;
    float dist = d.Length() + m_boundsRadius;
    if (dist > r) {
        r = dist;
        out = d;
    }
}

// Bounding sphere for the collision scoop: start at the body's bounds, then walk the centre towards
// the furthest point (halving the step each time) keeping the smallest radius found. The trial
// radius is not reset between steps. NEAR: retail leaves it uninitialised in the dead argument
// slot (an effectively zero denormal); we start it at 0.
void EX6DRigidBody::FindCollisionBounds(EXCollisionScoopTemp &t, u32 iters)
{
    if (!CollisionBody()) return;
    t.m_centre = CollisionBody()->BoundsCentre();
    if (!CollisionBody()->ExtraCollisions()) {
        t.m_radius = CollisionBody()->BoundsRadius();
        return;
    }
    t.m_radius = 0.0f;
    EXVector dir;
    CollisionBody()->CheckBounds(t.m_centre, t.m_radius, dir);
    float step = dir.Length() * 0.5f;
    dir.Normalise();
    EXVector best = t.m_centre;
    float r = 0.0f;
    for (u32 i = iters; i > 0; i--) {
        t.m_centre += dir * step;
        CollisionBody()->CheckBounds(t.m_centre, r, dir);
        if (r < t.m_radius) {
            t.m_radius = r;
            best = t.m_centre;
        }
        dir.Normalise();
        step *= 0.5f;
    }
    t.m_centre = best;
}

void EX6DBodyGroup::FindCollisionBounds(EXCollisionScoopTemp &t, u32 iters)
{
    if (!m_bodies.Size()) return;
    if (m_bodies[0]->CollisionBody())
        t.m_centre = m_bodies[0]->CollisionBody()->BoundsCentre();
    else
        t.m_centre.Set(0.0f, 0.0f, 0.0f, 0.0f);
    EXVector dir;
    t.m_radius = UpdatedBoundsRadius(t.m_centre, dir);
    float step = dir.Length() * 0.5f;
    dir.Normalise();
    EXVector best = t.m_centre;
    if (iters) {
        do {
            t.m_centre += dir * step;
            float r = UpdatedBoundsRadius(t.m_centre, dir);
            if (r < t.m_radius) {
                t.m_radius = r;
                best = t.m_centre;
            }
            dir.Normalise();
            step *= 0.5f;
        } while (--iters);
    }
    t.m_centre = best;
}

// Copy the simulated pose back to the item; for a body moving relative to a parent item the pose is
// first expressed in the item's own space and written to the parent.
void EX6DRigidBody::Syncronise()
{
    if (!m_pItem) return;
    if (m_pParentItem) {
        EXMaths::quat_xyzw ir;
        if (m_pItem->m_rotOrder == 0)
            ir.set(m_pItem->Rotation());
        else
            ir.SetRotation(m_pItem->Rotation(), m_pItem->m_rotOrder);
        ir.z = -ir.z;
        ir.y = -ir.y;
        ir.x = -ir.x;
        EXMatrix m;
        m.set_rotate(m_pItem->Rotation(), m_pItem->m_rotOrder);
        m.SetTranslation(m_pItem->Position());
        m = m.FastInverse();
        EXVector p = m.mat_44::operator*(EXVector(m_pos.x, m_pos.y, m_pos.z, 1.0f));
        p[3] = 0.0f;
        EXQuaternion r(ir * m_rot);
        if (m_pParentItem->m_rotOrder == 0) {
            m_pParentItem->m_rotation = EXVector(r.x, r.y, r.z, r.w);
        } else {
            EXVector a;
            EXQuaternion(r).GetRotation(a, m_pParentItem->m_rotOrder);
            m_pParentItem->m_rotation = a;
        }
        m_pParentItem->Invalidate();
        m_pParentItem->SetPosition(p);
        m_pParentItem->Invalidate();
        m_pItem->Invalidate();
        return;
    }
    m_pItem->SetPosition(m_pos);
    m_pItem->SetRotation(m_rot);
    m_pItem->Invalidate();
}

// A face hit on the group: push the body nearest the contact point away from the face.
void EX6DBodyGroup::DoCollision(EXItemPhysics *pOther, EXCScoop_FaceData *pData)
{
    if (!(m_flags6D & 2)) return;
    float bestD = 3.402823466e+38f;
    int best = 0;
    int n = m_bodies.Size();
    if (!n) return;
    for (int i = 0; i < n; i++) {
        float d = (m_bodies[i]->m_pos - pData->P1()).LengthXYZSquared();
        if (d < bestD) {
            best = i;
            bestD = d;
        }
    }
    EXVector dir = UnitXYZ(pData->P0() - pData->P1());
    if (!m_bodies[best]->IsKindOf(EXRUNTIME_CLASS(EX6DRigidBody))) return;
    SetActive(1);
    ((EX6DRigidBody *)m_bodies[best])->AddWorldImpulseAtPoint(dir * -m_50 * pData->m_dist, pData->P1());
}

// Integrate one step: linear as for a particle; the rotation by a second order step of the
// quaternion (angular velocity and the torque driven angular acceleration), then renormalised.
void EX6DRigidBody::UpdateMovement(float t)
{
    m_lastPos = m_pos;
    m_lastRot = m_rot;
    m_pos = m_velocity * t + m_pos + m_force * 0.5f * m_invMass * t * t;
    EXVector imp = m_impulse;
    float s = imp.LengthXYZ() * m_invMass;
    if (s > 80.0f) imp = imp * (80.0f / s);
    m_velocity = m_velocity * m_dampVelocity + m_force * m_invMass * t + imp * m_invMass;
    EXMaths::quat_xyzw w;
    w.set(m_angVel.x, m_angVel.y, m_angVel.z, 0.0f);
    EXMaths::quat_xyzw a;
    a.set(m_torque.x * m_invInertia.x, m_torque.y * m_invInertia.y, m_torque.z * m_invInertia.z,
          m_angVel.z * m_angVel.z * -2.0f - m_angVel.w * m_angVel.w * 2.0f - m_angVel.y * m_angVel.y * 2.0f -
              m_angVel.x * m_angVel.x * 2.0f);
    float ht = t * 0.5f;
    float qt = t * t * 0.25f;
    EXMaths::quat_xyzw dw = m_rot * w;
    EXMaths::quat_xyzw da = m_rot * a;
    m_rot.x = m_rot.x + dw.x * ht + da.x * qt;
    m_rot.y = m_rot.y + dw.y * ht + da.y * qt;
    m_rot.z = m_rot.z + dw.z * ht + da.z * qt;
    m_rot.w = m_rot.w + dw.w * ht + da.w * qt;
    m_angVel.x = m_dampAngular * m_angVel.x + m_invInertia.x * t * m_torque.x + m_invInertia.x * m_angImpulse.x;
    m_angVel.y = m_angImpulse.y * m_invInertia.y + m_torque.y * m_invInertia.y * t + m_angVel.y * m_dampAngular;
    m_angVel.z = m_angImpulse.z * m_invInertia.z + m_torque.z * m_invInertia.z * t + m_angVel.z * m_dampAngular;
    m_angVel.w = m_angImpulse.w * m_invInertia.w + m_torque.w * m_invInertia.w * t + m_angVel.w * m_dampAngular;
    m_rot.Normalise();
}

// Build the ragdoll bodies from the set-up info: one rigid body per info entry, laid out by walking
// the skin's bone hierarchy from the root (bind pose translation of the root bone).
s8 EX6DAnim::Initialise(EX6DAnimInitInfo *pInfo)
{
    EXItemAnimator_AnimModifier *pMod = pInfo->m_pMod;
    m_9c = (u32)pMod;
    EXItemRender_SkinAnim *pSkin = pMod->m_pSkin;
    EXItemRender_SkinAnimSection *pRoot = pSkin->SectionArray();
    if (!pRoot) return 0;
    delete[] m_pRigids;
    m_a4 = pInfo->m_bodies.Size();
    if (!m_a4) return 0;
    m_pRigids = new EX6DRigidBody[m_a4];
    m_a0 = 0;
    EXGeoHeader *pHdr = pSkin->SkelRefObject().GeoFile() ? pSkin->SkelRefObject().GeoFile()->GeoHeader() : 0;
    EXGeoBaseAnimSkin *pBase = pHdr->GeoAnimSkin(pSkin->SkelRefObject().HashRef());
    if (!pBase) return 0;
    EXMatrix m;
    m.SetIdentity();
    EXMatrix trans;
    trans.SetIdentity();
    trans.SetTranslation(pBase->RotDataRel()[pRoot->m_bone]);
    EXMatrix root = m * trans;
    s8 done = 0;
    RecurseInfoInit(pInfo, pRoot, m, root, pRoot, pBase, 0, done);
    pMod->ResetPositions(pInfo->m_pSnapShot, pInfo->m_pVelocity, pInfo->m_time);
    InitDefaultDeactivation();
    return 1;
}

// As Initialise(EX6DAnimInitInfo *) with the body layout taken from the skin's own ragdoll data.
s8 EX6DAnim::AutoRagdollInitialise(EXItemAnimator_AnimModifier *pMod, u32 n, EXItemCtrl_SnapShot *a,
                                   EXItemCtrl_SnapShot *b, float f)
{
    m_9c = (u32)pMod;
    EXItemRender_SkinAnim *pSkin = pMod->m_pSkin;
    EXItemRender_SkinAnimSection *pRoot = pSkin->SectionArray();
    if (!pRoot) return 0;
    delete[] m_pRigids;
    m_pRigids = new EX6DRigidBody[n];
    m_a4 = n;
    m_a0 = 0;
    EXGeoHeader *pHdr = pSkin->SkelRefObject().GeoFile() ? pSkin->SkelRefObject().GeoFile()->GeoHeader() : 0;
    EXGeoBaseAnimSkin *pBase = pHdr->GeoAnimSkin(pSkin->SkelRefObject().HashRef());
    if (!pBase) return 0;
    EXMatrix m;
    m.SetIdentity();
    EXMatrix trans;
    trans.SetIdentity();
    trans.SetTranslation(pBase->RotDataRel()[pRoot->m_bone]);
    EXMatrix root = m * trans;
    s8 done = 0;
    RecurseRagdollInit(pRoot, m, root, pRoot, pBase, 0, done);
    pMod->ResetPositions(a, b, f);
    if (!(m_flags & 0xc)) InitDefaultDeactivation();
    return 1;
}

// Walk the skin's section tree; each section that owns a bone physics item gets the next rigid
// body: mass, contact coefficients, a box inertia from the item's extents, and its centre and
// orientation placed by the item's pose in the entity frame.
s8 EX6DAnim::RecursiveInit(EXMatrix m, const EXItemRender_SkinAnimSection *pSec,
                           const EXGeoAnimSkin_BonePhysicItem *pPhys, int parent, u32 n)
{
    int idx = parent;
    EXMatrix sec;
    sec.set_rotate_quat(pSec->m_rot);
    sec.r[3].x = pSec->m_pos.x;
    sec.r[3].y = pSec->m_pos.y;
    sec.r[3].z = pSec->m_pos.z;
    sec.r[3].w = 1.0f;
    m = m * sec;
    EXItem *pItem = m_pItem;
    if (!pItem) return 0;
    EXMatrix itemMtx;
    itemMtx.set_rotate_xyz(pItem->m_rotation);
    itemMtx.SetTranslation(pItem->m_position);
    // Retail bug kept: the item pointer advances with the index, so the bone test reads item 2i.
    const EXGeoAnimSkin_BonePhysicItem *p = pPhys;
    for (u32 i = 0; i < n; i++, p++) {
        if (p[i].m_bone == pSec->m_bone) {
            m_a0++;
            idx = i;
            EX6DRigidBody *pBody = &m_pRigids[i];
            m_bodies.Add(pBody);
            m_bodies[m_bodies.Size() - 1]->ClearForces();
            pBody->SetManager(m_pManager);
            pBody->SetParent(this);
            m_active = 1;
            m_pRigids[i].SetMass(p->m_mass);
            m_pRigids[i].m_groundFriction = p->m_friction;
            m_pRigids[i].m_coefRestitution = p->m_restitution;
            EXVector c((p->m_min.x + p->m_max.x) * 0.5f, (p->m_max.y + p->m_min.y) * 0.5f,
                       (p->m_min.z + p->m_max.z) * 0.5f, 0.0f);
            c.ApplyXForm(p->m_mtx * itemMtx);
            m_pRigids[i].Position() = c;
            m_pRigids[i].SetInertiaFromSides(p->m_max.x - p->m_min.x, p->m_max.y - p->m_min.y,
                                             p->m_max.z - p->m_min.z);
            EXQuaternion q;
            p->m_mtx.GetRotation(q);
            m_pRigids[i].Rotation() = q;
            break;
        }
    }
    for (const EXItemRender_SkinAnimSection *pChild = pSec->ChildLinks(); pChild; pChild = pChild->Next())
        RecursiveInit(m, pChild, pPhys, idx, n);
    return 1;
}

// EXASin / EX6DConstraintType::Set copies that retail inlines into the
// (very large) ragdoll builders where VC would otherwise stop inlining.
static __forceinline float Rag_ASin(float f)
{
    if (f < -1.0f) return -1.5707964f;
    if (f > 1.0f) return 1.5707964f;
    return (float)asin(f);
}
static __forceinline void Rag_SetPin(EX6DConstraintType &ct, const EX6DPinConstraint &c)
{
    ct.PinConstraint() = c;
    ct.m_type = 0;
}

// As RecurseInfoInit for the built-in human ragdoll: bodies come from the hierarchy flags (0x18),
// boxed by bone length; flag 2 / 1 rigs use fixed per-body angle limits (bodies 7..10 pinned at two
// points), others limit each joint to +-0.3 (0.5 with flag 4) about its rest angles. With flag 2
// bones 0xd and 0x14 twist with bodies 4 and 6.
s8 EX6DAnim::RecurseRagdollInit(EXItemRender_SkinAnimSection *pSec, EXMatrix m, const EXMatrix &m2,
                                EXItemRender_SkinAnimSection *pRoot, EXGeoBaseAnimSkin *pSkin, u32 n, s8 &b)
{
    EXGeoAnimSkin_HierData *pHier = pSkin->HierData();
    EXMatrix trans;
    trans.SetIdentity();
    trans.SetTranslation(pSkin->RotDataRel()[pSec->m_bone]);
    m = m * trans;
    u32 idx = n;
    u16 bone = pSec->m_bone;
    u16 flags = pHier[bone].m_Flags;
    s8 bCreated = 0;
    if (m_flags & 2) {
        EX6DRigidBody *pTwist;
        if (bone == 0xd) pTwist = &m_pRigids[4];
        else if (bone == 0x14) pTwist = &m_pRigids[6];
        else goto noTwist;
        {
            EX6DAnimInfo info;
            info.m_pBody = pTwist;
            info.m_bone = bone;
            EXMatrix r;
            r.set_rotate_quat(pTwist->m_rot);
            r.SetTranslation(pTwist->m_pos);
            r = r.FastInverse();
            info.m_mtx = r * m;
            ((EXItemAnimator_AnimModifier *)m_9c)->Add6DModifier(info);
        }
    }
noTwist:
    if (flags & 0x18) {
        if (!b) {
            EXVector p = m2.r[3];
            EXVector c = m.r[3];
            EXVector d = c - p;
            float len = d.LengthXYZ();
            if (len < 0.1f) len = 0.1f;
            EXVector dir = d;
            dir.Normalise();
            EXVector centre = (p + c) * 0.5f;
            float dy = dir.Dot(EXVector(0.0f, 1.0f, 0.0f, 0.0f));
            EXVector ref;
            if (dy >= 0.7f || dy <= -0.7f) {
                ref.Set(0.0f, 1.0f, 0.0f, 0.0f);
                if (dy < 0.0f) ref.Set(0.0f, -1.0f, 0.0f, 0.0f);
            } else {
                ref.Set(1.0f, 0.0f, 0.0f, 0.0f);
                if (dir.Dot(EXVector(1.0f, 0.0f, 0.0f, 0.0f)) < 0.0f) ref.Set(-1.0f, 0.0f, 0.0f, 0.0f);
            }
            EXVector axis = ref.Cross(dir);
            axis.Normalise();
            float angle = EXACos(ref.Dot(dir));
            EXVector v = axis * (float)sin(angle * 0.5f);
            EXQuaternion q(v.x, v.y, v.z, (float)cos(angle * 0.5f));
            AddBody(&m_pRigids[m_a0]);
            m_pRigids[m_a0].Position() = centre;
            m_pRigids[m_a0].Rotation() = q;
            m_pRigids[m_a0].SetInertiaFromSides(len, len, len);
            m_pRigids[m_a0].Flags6D().BitSet(1);
            m_pRigids[m_a0].SetItem(m_pItem);
            m_pRigids[m_a0].SetParent(this);
            m_pRigids[m_a0].SetAutoUpdate(0);
            if (m_flags & 4) {
                m_pRigids[m_a0].SetInertiaTweak(5.0f);
                m_pRigids[m_a0].SetDampAngular(0.01f);
                m_pRigids[m_a0].SetDampVelocity(0.01f);
            }
            if (!(m_flags & 0xc)) {
                m_pRigids[m_a0].SetCollisionBody(new EX6DCollisionBody(&m_pRigids[m_a0], 1));
                float radius = len * 0.4f;
                if (radius < 0.15f) radius = 0.15f;
                EXCollisionType &o = m_pRigids[m_a0].CollisionBody()->m_pObjects[0];
                o.Sphere().m_centre.Set(0.0f, 0.0f, 0.0f, 0.0f);
                o.Sphere().m_radius = radius;
                o.m_type = EXCollisionType::COL_SPHERE;
                m_pRigids[m_a0].CollisionBody()->m_maxContacts = 1;
                m_pRigids[m_a0].CollisionBody()->CalcBounds(0xf);
                m_pRigids[m_a0].SetSphereInertia();
                m_pRigids[m_a0].SetInertiaTweak(1.6f / radius);
            }
            if (m_flags & 3) {
                if (m_a0 < 2 || m_a0 == 8 || m_a0 == 10) m_pRigids[m_a0].SetDampVelocity(0.0f);
                else m_pRigids[m_a0].SetDampVelocity(0.01f);
            }
            if (n != m_a0) {
                EX6DPinConstraint pin;
                if (!(m_flags & 3) || m_a0 < 7) {
                    pin.SetWorldSpace(&m_pRigids[n], &m_pRigids[m_a0], p);
                } else {
                    EX6DPinConstraint pin0;
                    pin0.SetWorldSpace(&m_pRigids[n], &m_pRigids[m_a0], EXVector(p.x - 0.1f, p.y, p.z, p.w));
                    pin0.SetStrength(1.0f);
                    EX6DConstraintType ct0;
                    Rag_SetPin(ct0, pin0);
                    m_pManager->m_constraints.Add(ct0, 1);
                    pin.SetWorldSpace(&m_pRigids[n], &m_pRigids[m_a0], EXVector(p.x + 0.1f, p.y, p.z, p.w));
                }
                pin.SetStrength(1.0f);
                EX6DConstraintType ct;
                Rag_SetPin(ct, pin);
                m_pManager->m_constraints.Add(ct, 1);
                u32 i = m_a0;
                EXVector lo, hi;
                if (m_flags & 2) {
                    if (i == 0 || i == 1) {
                        lo.Set(-0.3f, -0.3f, -0.3f, 0.0f); hi.Set(0.3f, 0.3f, 0.3f, 0.0f);
                    } else if (i == 2) {
                        lo.Set(-0.6f, -0.3f, -0.6f, 0.0f); hi.Set(0.6f, 0.3f, 0.6f, 0.0f);
                    } else if (i == 3) {
                        lo.Set(-1.1f, -1.1f, -0.1f, 0.0f); hi.Set(1.1f, 1.1f, 1.1f, 0.0f);
                    } else if (i == 4) {
                        lo.Set(-0.3f, -0.1f, -0.3f, 0.0f); hi.Set(0.3f, 1.3f, 0.3f, 0.0f);
                    } else if (i == 5) {
                        lo.Set(-1.1f, -1.1f, -1.1f, 0.0f); hi.Set(1.1f, 1.1f, 0.1f, 0.0f);
                    } else if (i == 6) {
                        lo.Set(-0.3f, -1.3f, -0.3f, 0.0f); hi.Set(0.3f, 0.1f, 0.3f, 0.0f);
                    } else if (i == 7) {
                        lo.Set(-0.8f, -9.02f, -9.02f, 0.0f); hi.Set(0.3f, 9.02f, 9.02f, 0.0f);
                    } else if (i == 8) {
                        lo.Set(0.1f, -9.02f, -9.02f, 0.0f); hi.Set(1.4f, 9.02f, 9.02f, 0.0f);
                    } else if (i == 9) {
                        lo.Set(-0.8f, -9.02f, -9.02f, 0.0f); hi.Set(0.3f, 9.02f, 9.02f, 0.0f);
                    } else if (i == 10) {
                        lo.Set(0.1f, -9.02f, -9.02f, 0.0f); hi.Set(1.4f, 9.02f, 9.02f, 0.0f);
                    } else {
                        lo.Set(-0.3f, -0.3f, -0.3f, 0.0f); hi.Set(0.3f, 0.3f, 0.3f, 0.0f);
                    }
                } else if (m_flags & 1) {
                    if (i == 0 || i == 1) {
                        lo.Set(-0.3f, -0.3f, -0.3f, 0.0f); hi.Set(0.3f, 0.3f, 0.3f, 0.0f);
                    } else if (i == 2) {
                        lo.Set(-0.6f, -0.3f, -0.6f, 0.0f); hi.Set(0.6f, 0.3f, 0.6f, 0.0f);
                    } else if (i == 3) {
                        lo.Set(0.0f, -1.1f, -0.7f, 0.0f); hi.Set(1.2f, 1.1f, 0.9f, 0.0f);
                    } else if (i == 4) {
                        lo.Set(-0.3f, 0.0f, -0.3f, 0.0f); hi.Set(0.3f, 1.8f, 0.3f, 0.0f);
                    } else if (i == 5) {
                        lo.Set(-1.2f, -1.1f, -0.9f, 0.0f); hi.Set(0.0f, 1.1f, 0.7f, 0.0f);
                    } else if (i == 6) {
                        lo.Set(-0.3f, -1.8f, -0.3f, 0.0f); hi.Set(0.3f, 0.0f, 0.3f, 0.0f);
                    } else if (i == 7 || i == 9) {
                        lo.Set(-0.8f, -9.02f, -9.02f, 0.0f); hi.Set(0.3f, 9.02f, 9.02f, 0.0f);
                    } else if (i == 8 || i == 10) {
                        lo.Set(0.1f, -9.02f, -9.02f, 0.0f); hi.Set(1.5f, 9.02f, 9.02f, 0.0f);
                    } else {
                        lo.Set(-0.3f, -0.3f, -0.3f, 0.0f); hi.Set(0.3f, 0.3f, 0.3f, 0.0f);
                    }
                } else {
                    const EXQuaternion &pr = m_pRigids[n].m_rot;
                    EXQuaternion inv(0.0f - pr.x, 0.0f - pr.y, 0.0f - pr.z, pr.w);
                    EXQuaternion rel = inv * m_pRigids[i].m_rot;
                    float ax = Rag_ASin(rel.x) * 2.0f;
                    float ay = Rag_ASin(rel.y) * 2.0f;
                    float az = Rag_ASin(rel.z) * 2.0f;
                    float range = 0.3f;
                    if (m_flags & 4) range = 0.5f;
                    lo.Set(ax - range, ay - range, az - range, 0.0f);
                    hi.Set(range + ax, range + ay, range + az, 0.0f);
                }
                EX6DAngleConstraint ac;
                ac.Set(&m_pRigids[n], &m_pRigids[i], lo, hi);
                ac.SetStrength(1.0f);
                EX6DConstraintType ct2;
                ct2.Set(ac);
                m_pManager->m_constraints.Add(ct2, 1);
            }
            EX6DAnimInfo info;
            info.m_pBody = &m_pRigids[m_a0];
            info.m_bone = pRoot->m_bone;
            EXMatrix r;
            r.set_rotate_quat(q);
            r.SetTranslation(centre);
            r = r.FastInverse();
            info.m_mtx = r * m2;
            idx = m_a0;
            ((EXItemAnimator_AnimModifier *)m_9c)->Add6DModifier(info);
            m_a0++;
            b = 1;
        }
        bCreated = 1;
    }
    s8 bChild = 0;
    for (EXItemRender_SkinAnimSection *pChild = pSec->ChildLinks(); pChild; pChild = pChild->Next()) {
        if (bCreated)
            RecurseRagdollInit(pChild, m, m, pSec, pSkin, idx, bChild);
        else
            RecurseRagdollInit(pChild, m, m2, pRoot, pSkin, n, b);
    }
    return 1;
}

// Walk the skin's section tree with the set-up info: bones flagged in the hierarchy (0x18) start a
// new body, a sphere placed between the parent joint (m2's origin) and this joint (m's origin) and
// turned from the nearest of +-x / +-y onto the bone. Each body is pinned to its parent body at the
// joint (twice, 0.2 apart in x, when asked), optionally angle limited, and drives the bones down to
// the next body through an anim modifier; bones listed as twists are driven by their own body.
s8 EX6DAnim::RecurseInfoInit(EX6DAnimInitInfo *pInfo, EXItemRender_SkinAnimSection *pSec, EXMatrix m,
                             const EXMatrix &m2, EXItemRender_SkinAnimSection *pRoot, EXGeoBaseAnimSkin *pSkin,
                             u32 n, s8 &b)
{
    EXGeoAnimSkin_HierData *pHier = pSkin->HierData();
    EXMatrix trans = EXMatrix::MakeIdentity();
    trans.SetTranslation(pSkin->RotDataRel()[pSec->m_bone]);
    m = m * trans;
    u32 idx = n;
    u16 flags = pHier[pSec->m_bone].m_Flags;
    s8 bCreated = 0;
    for (int i = 0; i < pInfo->m_twists.Size(); i++) {
        if (pSec->m_bone == pInfo->m_twists[i].m_bone) {
            EX6DAnimInfo info;
            info.m_pBody = &m_pRigids[pInfo->m_twists[i].m_body];
            info.m_bone = pSec->m_bone;
            EXMatrix r;
            r.set_rotate_quat(info.m_pBody->m_rot);
            r.SetTranslation(info.m_pBody->m_pos);
            r = r.FastInverse();
            info.m_mtx = r * m;
            ((EXItemAnimator_AnimModifier *)m_9c)->Add6DModifier(info);
        }
    }
    if (flags & 0x18) {
        if (!b) {
            EXVector p = m2.r[3];
            EXVector c = m.r[3];
            EXVector dir = c - p;
            dir.Normalise();
            EXVector centre = (c + p) * 0.5f;
            float dy = dir.Dot(EXVector(0.0f, 1.0f, 0.0f, 0.0f));
            EXVector ref;
            if (dy >= 0.7f || dy <= -0.7f) {
                ref.Set(0.0f, 1.0f, 0.0f, 0.0f);
                if (dy < 0.0f) ref.Set(0.0f, -1.0f, 0.0f, 0.0f);
            } else {
                ref.Set(1.0f, 0.0f, 0.0f, 0.0f);
                if (dir.Dot(EXVector(1.0f, 0.0f, 0.0f, 0.0f)) < 0.0f) ref.Set(-1.0f, 0.0f, 0.0f, 0.0f);
            }
            EXVector axis = ref.Cross(dir);
            axis.Normalise();
            float angle = EXACos(ref.Dot(dir));
            EXVector v = axis * (float)sin(angle * 0.5f);
            EXQuaternion q(v.x, v.y, v.z, (float)cos(angle * 0.5f));
            EX6DRigidBody *pBody = &m_pRigids[m_a0];
            m_bodies.Add(pBody);
            m_bodies[m_bodies.Size() - 1]->ClearForces();
            pBody->SetManager(m_pManager);
            pBody->SetParent(this);
            m_active = 1;
            m_pRigids[m_a0].Position() = centre;
            m_pRigids[m_a0].Rotation() = q;
            m_pRigids[m_a0].Flags6D().BitSet(1);
            m_pRigids[m_a0].SetItem(m_pItem);
            m_pRigids[m_a0].SetParent(this);
            m_pRigids[m_a0].SetAutoUpdate(0);
            m_pRigids[m_a0].SetCollisionBody(new EX6DCollisionBody(&m_pRigids[m_a0], 1));
            float radius = pInfo->m_bodies[m_a0].m_radius;
            if (radius < 0.15f) radius = 0.15f;
            EXCollisionType &o = m_pRigids[m_a0].CollisionBody()->m_pObjects[0];
            o.Sphere().m_centre.Set(0.0f, 0.0f, 0.0f, 0.0f);
            o.Sphere().m_radius = radius;
            o.m_type = EXCollisionType::COL_SPHERE;
            m_pRigids[m_a0].CollisionBody()->m_maxContacts = 1;
            m_pRigids[m_a0].CollisionBody()->CalcBounds(0xf);
            m_pRigids[m_a0].SetSphereInertia();
            m_pRigids[m_a0].SetInertiaTweak(1.6f / radius);
            m_pRigids[m_a0].SetDampVelocity(pInfo->m_bodies[m_a0].m_damping);
            m_pRigids[m_a0].SetCoefRestitution(pInfo->m_bodies[m_a0].m_restitution);
            if (n != m_a0) {
                if (pInfo->m_bodies[m_a0].m_bDoublePin) {
                    EX6DPinConstraint pin;
                    pin.SetWorldSpace(&m_pRigids[n], &m_pRigids[m_a0], EXVector(p.x - 0.1f, p.y, p.z, p.w));
                    pin.SetStrength(pInfo->m_bodies[m_a0].m_pinStrength);
                    EX6DConstraintType ct;
                    ct.Set(pin);
                    m_pManager->m_constraints.Add(ct, 1);
                    p = EXVector(p.x + 0.1f, p.y, p.z, p.w);
                }
                EX6DPinConstraint pin;
                pin.SetWorldSpace(&m_pRigids[n], &m_pRigids[m_a0], p);
                pin.SetStrength(pInfo->m_bodies[m_a0].m_pinStrength);
                EX6DConstraintType ct;
                ct.Set(pin);
                m_pManager->m_constraints.Add(ct, 1);
                if (pInfo->m_bodies[m_a0].m_bAngleLimit) {
                    EX6DAngleConstraint ac;
                    ac.Set(&m_pRigids[n], &m_pRigids[m_a0], pInfo->m_bodies[m_a0].m_angleA,
                           pInfo->m_bodies[m_a0].m_angleB);
                    ac.SetStrength(pInfo->m_bodies[m_a0].m_angleStrength);
                    EX6DConstraintType ct2;
                    ct2.Set(ac);
                    m_pManager->m_constraints.Add(ct2, 1);
                }
            }
            EX6DAnimInfo info;
            info.m_pBody = &m_pRigids[m_a0];
            info.m_bone = pRoot->m_bone;
            EXMatrix r;
            r.set_rotate_quat(q);
            r.SetTranslation(centre);
            r = r.FastInverse();
            info.m_mtx = r * m2;
            idx = m_a0;
            ((EXItemAnimator_AnimModifier *)m_9c)->Add6DModifier(info);
            m_a0++;
            b = 1;
        }
        bCreated = 1;
    }
    s8 bChild = 0;
    for (EXItemRender_SkinAnimSection *pChild = pSec->ChildLinks(); pChild; pChild = pChild->Next()) {
        if (bCreated)
            RecurseInfoInit(pInfo, pChild, m, m, pSec, pSkin, idx, bChild);
        else
            RecurseInfoInit(pInfo, pChild, m, m2, pRoot, pSkin, n, b);
    }
    return 1;
}

// Build the collision body from the entity's collision datums (boxes, spheres and capsules), size
// the inertia from their combined bounds unless told not to, and set up deactivation sampling.
void EX6DRigid::Initialise(const EXGeoBaseEntity *pEnt, s8 b, u32 flags)
{
    if (!pEnt) return;
    m_autoUpdate = b;
    m_pCollisionBody = 0;
    const EXGeoCollisionXtra *pXtra = (const EXGeoCollisionXtra *)pEnt->m_XtraData.GetXtra(5);
    if (!pXtra) return;
    const EXGeoCollisionDatums *pSet = pXtra->CollisionDatums();
    if (!pSet) return;
    u32 n = pSet->NumDatums();
    if (!n || n < 1 || n > 1000) return;
    m_pCollisionBody = new EX6DCollisionBody(this, n + flags);
    EXVector vmin(0.0f, 0.0f, 0.0f, 0.0f);
    EXVector vmax(0.0f, 0.0f, 0.0f, 0.0f);
    for (u32 i = 0; i < n; i++) {
        const EXGeoBaseDatum *pD = &pSet->Datums()[i];
        EXDatumTemp d;
        EXVector origin(0.0f, 0.0f, 0.0f, 0.0f);
        EXMatrix m;
        m.SetIdentity();
        d.Set(pD, origin, m);
        switch (pD->m_06) {
        case 0: {
            EXBox box;
            d.Get(box);
            EXBoundsBox bb;
            box.GetBoundsBox(bb);
            for (u32 j = 0; j < 3; j++) {
                if (bb.m_vctMin[j] < vmin[j]) vmin[j] = bb.m_vctMin[j];
                if (bb.m_vctMax[j] > vmax[j]) vmax[j] = bb.m_vctMax[j];
            }
            if (m_pCollisionBody) {
                EXCollisionType &o = m_pCollisionBody->m_pObjects[i];
                EXBox &ob = o.Box();
                ob.m_centre = box.m_centre;
                ob.m_axis[0] = box.m_axis[0];
                ob.m_axis[1] = box.m_axis[1];
                ob.m_extent[0] = box.m_extent[0];
                ob.m_extent[1] = box.m_extent[1];
                ob.m_extent[2] = box.m_extent[2];
                o.m_type = EXCollisionType::COL_BOX;
            }
            break;
        }
        case 1: {
            EXSphere s;
            d.Get(s);
            if (s.m_centre.x - s.m_radius < vmin.x) vmin.x = s.m_centre.x - s.m_radius;
            if (s.m_centre.y - s.m_radius < vmin.y) vmin.y = s.m_centre.y - s.m_radius;
            if (s.m_centre.z - s.m_radius < vmin.z) vmin.z = s.m_centre.z - s.m_radius;
            if (s.m_centre.x + s.m_radius > vmax.x) vmax.x = s.m_centre.x + s.m_radius;
            if (s.m_centre.y + s.m_radius > vmax.y) vmax.y = s.m_centre.y + s.m_radius;
            if (s.m_centre.z + s.m_radius > vmax.z) vmax.z = s.m_centre.z + s.m_radius;
            if (m_pCollisionBody) {
                EXCollisionType &o = m_pCollisionBody->m_pObjects[i];
                *(EXSphere *)o.m_shape = s;
                o.m_type = EXCollisionType::COL_SPHERE;
            }
            break;
        }
        case 3: {
            EXCapsule c;
            d.Get(c);
            const EXVector &o = c.Origin();
            float r = c.Radius();
            if (o.x - r < vmin.x) vmin.x = o.x - r;
            if (o.y - r < vmin.y) vmin.y = o.y - r;
            if (o.z - r < vmin.z) vmin.z = o.z - r;
            if (o.x + r > vmax.x) vmax.x = o.x + r;
            if (o.y + r > vmax.y) vmax.y = o.y + r;
            if (o.z + r > vmax.z) vmax.z = o.z + r;
            EXVector e = c.Origin() + c.Direction();
            if (e.x - r < vmin.x) vmin.x = e.x - r;
            if (e.y - r < vmin.y) vmin.y = e.y - r;
            if (e.z - r < vmin.z) vmin.z = e.z - r;
            if (e.x + r > vmax.x) vmax.x = e.x + r;
            if (e.y + r > vmax.y) vmax.y = e.y + r;
            if (e.z + r > vmax.z) vmax.z = e.z + r;
            if (m_pCollisionBody) {
                EXCollisionType &o = m_pCollisionBody->m_pObjects[i];
                EXCapsule &oc = o.Capsule();
                oc.Origin() = c.Origin();
                oc.Direction() = c.Direction();
                oc.Radius() = c.Radius();
                o.m_type = EXCollisionType::COL_CAPSULE;
            }
            break;
        }
        default:
            return;
        }
    }
    if (!flags) {
        m_pCollisionBody->CalcBounds(0xf);
        SetInertiaFromSides(vmax.x - vmin.x, vmax.y - vmin.y, vmax.z - vmin.z);
    }
    m_178 = 0.6f;
    m_numSamples = 3;
    m_pSamples = (float *)_EXAlloc(m_numSamples * sizeof(float), 0);
    m_174 = m_178;
    m_170 = 0.3f;
    m_sampleIndex = 0;
    for (u32 k = 0; k < m_numSamples; k++) m_pSamples[k] = 100.0f;
    SyncWithItem();
}

// As Initialise(entity) for a script-built object: every entity placed by the script contributes
// its collision datums, moved by the placement's rotation and position.
void EX6DRigid::Initialise(const EXGeoScript *pScript, s8 b, u32 flags)
{
    if (!pScript || !m_pItem) return;
    EXGeoHeader *pHdr = m_pItem->m_geoRef.GeoFile()->GeoHeader();
    if (!pHdr) return;
    m_pCollisionBody = 0;
    if (m_pSamples) EXFree(m_pSamples);
    m_pSamples = 0;
    // Count the datums first.
    u32 total = 0;
    int ofs = 0;
    const GEOSCRIPT_CMD *pCmd;
    // Retail quirk kept: an entity with an empty datum list does not advance the offset.
    do {
        pCmd = pScript->Data(ofs);
        if (pCmd->m_Type == 3) {
            EXGeoBaseEntity *pEnt = pHdr->GeoEntity(pCmd->m_14);
            if (!pEnt) {
            next:
                ofs += pCmd->m_Size;
                continue;
            }
            const EXGeoCollisionXtra *pXtra = (const EXGeoCollisionXtra *)pEnt->m_XtraData.GetXtra(5);
            if (pXtra) {
                const EXGeoCollisionDatums *pSet = pXtra->CollisionDatums();
                if (!pSet) {
                    ofs += pCmd->m_Size;
                    continue;
                }
                u32 n = pSet->NumDatums();
                if (!n) continue;
                if (n < 1 || n > 1000) goto next;
                total += n;
            }
        }
        ofs += pCmd->m_Size;
    } while (pCmd->m_Type != 0x12);
    m_autoUpdate = b;
    m_pCollisionBody = new EX6DCollisionBody(this, total + flags);
    ofs = 0;
    EXVector vmin(0.0f, 0.0f, 0.0f, 0.0f);
    EXVector vmax(0.0f, 0.0f, 0.0f, 0.0f);
    u32 base = 0;
    do {
        pCmd = pScript->Data(ofs);
        if (pCmd->m_Type == 3) {
            EXGeoBaseEntity *pEnt = pHdr->GeoEntity(pCmd->m_14);
            if (!pEnt) {
                ofs += pCmd->m_Size;
                continue;
            }
            const EXGeoCollisionXtra *pXtra = (const EXGeoCollisionXtra *)pEnt->m_XtraData.GetXtra(5);
            if (pXtra) {
                const EXGeoCollisionDatums *pSet = pXtra->CollisionDatums();
                if (!pSet) {
                    ofs += pCmd->m_Size;
                    continue;
                }
                u32 n = pSet->NumDatums();
                if (!n) continue;
                if (n < 1 || n > 1000) {
                    ofs += pCmd->m_Size;
                    continue;
                }
                {
                    {
                        {
                            EXScriptDecodeStruct dec;
                            pScript->DecodeScriptCmd(pCmd, &dec, 0, 1);
                            EXMatrix mtx;
                            mtx.set_rotate_quat(dec.m_rot);
                            mtx.r[3].x = dec.m_pos.x;
                            mtx.r[3].y = dec.m_pos.y;
                            mtx.r[3].z = dec.m_pos.z;
                            mtx.r[3].w = 1.0f;
                            for (u32 i = 0; i < n; i++) {
                                const EXGeoBaseDatum *pD = &pSet->Datums()[i];
                                EXDatumTemp d;
                                EXVector origin(0.0f, 0.0f, 0.0f, 0.0f);
                                EXMatrix m;
                                m.SetIdentity();
                                d.Set(pD, origin, m);
                                switch (pD->m_06) {
                                case 0: {
                                    EXBox box;
                                    d.Get(box);
                                    EXBoundsBox bb;
                                    box.GetBoundsBox(bb);
                                    for (u32 j = 0; j < 3; j++) {
                                        if (bb.m_vctMin[j] < vmin[j]) vmin[j] = bb.m_vctMin[j];
                                        if (bb.m_vctMax[j] > vmax[j]) vmax[j] = bb.m_vctMax[j];
                                    }
                                    if (m_pCollisionBody) {
                                        EXCollisionType &o = m_pCollisionBody->m_pObjects[base + i];
                                        EXBox &ob = o.Box();
                                        ob.m_centre = box.m_centre;
                                        ob.m_axis[0] = box.m_axis[0];
                                        ob.m_axis[1] = box.m_axis[1];
                                        ob.m_extent[0] = box.m_extent[0];
                                        ob.m_extent[1] = box.m_extent[1];
                                        ob.m_extent[2] = box.m_extent[2];
                                        o.m_type = EXCollisionType::COL_BOX;
                                    }
                                    break;
                                }
                                case 1: {
                                    EXSphere s;
                                    d.Get(s);
                                    if (s.m_centre.x - s.m_radius < vmin.x) vmin.x = s.m_centre.x - s.m_radius;
                                    if (s.m_centre.y - s.m_radius < vmin.y) vmin.y = s.m_centre.y - s.m_radius;
                                    if (s.m_centre.z - s.m_radius < vmin.z) vmin.z = s.m_centre.z - s.m_radius;
                                    if (s.m_centre.x + s.m_radius > vmax.x) vmax.x = s.m_centre.x + s.m_radius;
                                    if (s.m_centre.y + s.m_radius > vmax.y) vmax.y = s.m_centre.y + s.m_radius;
                                    if (s.m_centre.z + s.m_radius > vmax.z) vmax.z = s.m_centre.z + s.m_radius;
                                    if (m_pCollisionBody) {
                                        EXCollisionType &o = m_pCollisionBody->m_pObjects[base + i];
                                        *(EXSphere *)o.m_shape = s;
                                        o.m_type = EXCollisionType::COL_SPHERE;
                                    }
                                    break;
                                }
                                case 3: {
                                    EXCapsule c;
                                    d.Get(c);
                                    const EXVector &o = c.Origin();
                                    float r = c.Radius();
                                    if (o.x - r < vmin.x) vmin.x = o.x - r;
                                    if (o.y - r < vmin.y) vmin.y = o.y - r;
                                    if (o.z - r < vmin.z) vmin.z = o.z - r;
                                    if (o.x + r > vmax.x) vmax.x = o.x + r;
                                    if (o.y + r > vmax.y) vmax.y = o.y + r;
                                    if (o.z + r > vmax.z) vmax.z = o.z + r;
                                    EXVector e = c.Origin() + c.Direction();
                                    if (e.x - r < vmin.x) vmin.x = e.x - r;
                                    if (e.y - r < vmin.y) vmin.y = e.y - r;
                                    if (e.z - r < vmin.z) vmin.z = e.z - r;
                                    if (e.x + r > vmax.x) vmax.x = e.x + r;
                                    if (e.y + r > vmax.y) vmax.y = e.y + r;
                                    if (e.z + r > vmax.z) vmax.z = e.z + r;
                                    if (m_pCollisionBody) {
                                        EXCollisionType &ot = m_pCollisionBody->m_pObjects[base + i];
                                        EXCapsule &oc = ot.Capsule();
                                        oc.Origin() = c.Origin();
                                        oc.Direction() = c.Direction();
                                        oc.Radius() = c.Radius();
                                        ot.m_type = EXCollisionType::COL_CAPSULE;
                                    }
                                    break;
                                }
                                default:
                                    return;
                                }
                                if (m_pCollisionBody) m_pCollisionBody->m_pObjects[base + i].ApplyXForm(mtx);
                            }
                            base += n;
                        }
                    }
                }
            }
        }
        ofs += pCmd->m_Size;
    } while (pCmd->m_Type != 0x12);
    if (!flags) {
        m_pCollisionBody->CalcBounds(0xf);
        SetInertiaFromSides(vmax.x - vmin.x, vmax.y - vmin.y, vmax.z - vmin.z);
    }
    m_178 = 0.6f;
    m_numSamples = 3;
    m_pSamples = (float *)_EXAlloc(m_numSamples * sizeof(float), 0);
    m_174 = m_178;
    m_170 = 0.3f;
    m_sampleIndex = 0;
    for (u32 k = 0; k < m_numSamples; k++) m_pSamples[k] = 100.0f;
    SyncWithItem();
}

// Turn the contacts found against another rigid body into constraints: deep enough contacts wake
// both bodies; against a sleeping body each contact becomes a world collision pinned in the
// sleeper's frame, and between two awake bodies an inter-collision constraint.
void EX6DRigidBody::InterObjectScoopCallback(EXInterObjectTemp &t)
{
    if (t.m_which) return;
    if (!t.m_pB->IsKindOf(EXRUNTIME_CLASS(EX6DRigidBody))) return;
    EX6DRigidBody *pOther = (EX6DRigidBody *)t.m_pB;
    float depth = 0.0f;
    u32 i;
    for (i = 0; i < t.m_numContacts; i++)
        if (t.m_pContacts[i].m_depth > depth) depth = t.m_pContacts[i].m_depth;
    if (depth > m_pManager->InterReactivateDist()) {
        EX6DObject::SetActive(1);
        pOther->EX6DObject::SetActive(1);
    }
    if (!m_active) {
        EXMatrix m;
        m.set_rotate_quat(pOther->m_rot);
        m.SetTranslation(pOther->m_pos);
        m = m.FastInverse();
        for (i = 0; i < t.m_numContacts; i++) {
            EXVector n = (t.m_pContacts[i].m_a - t.m_pContacts[i].m_b).UnitVectorXYZ();
            EX6DWorldCollisionConstraint wc;
            wc.Set(pOther, t.m_pContacts[i].m_a, n, t.m_pContacts[i].m_idxB);
            EXVector bp = t.m_pContacts[i].m_b;
            bp.ApplyXForm(m);
            wc.SetBodyPoint(bp);
            EX6DConstraintType ct;
            ct.Set(wc);
            ct.SetIterations(pOther->m_iterations);
            pOther->m_pManager->m_collisions.Add(ct, 1);
        }
    } else if (!pOther->m_active) {
        EXMatrix m;
        m.set_rotate_quat(m_rot);
        m.SetTranslation(m_pos);
        m = m.FastInverse();
        for (i = 0; i < t.m_numContacts; i++) {
            EXVector n = (t.m_pContacts[i].m_b - t.m_pContacts[i].m_a).UnitVectorXYZ();
            EX6DWorldCollisionConstraint wc;
            wc.Set(this, t.m_pContacts[i].m_b, n, t.m_pContacts[i].m_idxA);
            EXVector bp = t.m_pContacts[i].m_a;
            bp.ApplyXForm(m);
            wc.SetBodyPoint(bp);
            EX6DConstraintType ct;
            ct.Set(wc);
            ct.SetIterations(m_iterations);
            m_pManager->m_collisions.Add(ct, 1);
        }
    } else {
        for (i = 0; i < t.m_numContacts; i++) {
            EX6DInterCollisionConstraint ic;
            ic.Set(this, pOther, t.m_pContacts[i].m_idxA, t.m_pContacts[i].m_idxB);
            EX6DConstraintType ct;
            ct.Set(ic);
            u8 it = m_iterations;
            if (pOther->m_iterations < it) it = pOther->m_iterations;
            ct.SetIterations(it);
            m_pManager->m_collisions.Add(ct, 1);
        }
    }
}

// Bounds of the collision objects in body space (m_10), that centre in world space, the bounding
// radius, the smallest object radius (m_64) and the farthest object reach from the body origin
// (m_68). Bodies flagged with 1 measure the reach to the outer surface, others to object centres.
void EX6DCollisionBody::CalcBounds(u32 flags)
{
    EXMatrix m;
    if (m_pRigidBody) {
        m.set_rotate_quat(m_pRigidBody->m_rot);
        m.SetTranslation(m_pRigidBody->m_pos);
    } else {
        m.SetIdentity();
        m.SetTranslation(m_pParticle->m_pos);
    }
    EXCollisionScoopTemp temp;
    temp.Initialise();
    temp.m_numObjects = m_numObjects;
    temp.m_pObjects = m_pObjects;
    temp.m_xform.SetIdentity();
    temp.CalcBounds(temp.m_xform, (u8)flags);
    m_10 = temp.m_centre;
    m_boundsCentre = m_10;
    m_boundsCentre.ApplyXForm(m);
    m_boundsRadius = temp.m_radius;
    float d = 0.0f;
    for (u32 i = 0; i < m_numObjects; i++) {
        if (m_pObjects[i].Radius() < m_64) m_64 = m_pObjects[i].Radius();
        if (!(m_pRigidBody->m_flags6D & 1)) {
            EXCollisionType &o = m_pObjects[i];
            int type = o.Type();
            if (type == EXCollisionType::COL_SPHERE) {
                d = o.Origin().LengthXYZ();
            } else if (type == EXCollisionType::COL_CAPSULE) {
                d = o.Origin().LengthXYZ();
                EXVector e = m_pObjects[i].Capsule().Direction() + m_pObjects[i].Capsule().Origin();
                float b = e.LengthXYZ();
                if (b > d) d = b;
            } else if (type == EXCollisionType::COL_BOX) {
                d = o.Box().Centre().LengthXYZ();
            }
        } else {
            EXCollisionType &o = m_pObjects[i];
            int type = o.Type();
            if (type == EXCollisionType::COL_SPHERE) {
                d = o.Origin().LengthXYZ() + m_pObjects[i].Radius();
            } else if (type == EXCollisionType::COL_CAPSULE) {
                float a = o.Origin().LengthXYZ() + m_pObjects[i].Radius();
                EXVector e = m_pObjects[i].Capsule().Origin() + m_pObjects[i].Capsule().Direction();
                float b = e.LengthXYZ() + m_pObjects[i].Radius();
                d = a;
                if (a < b) d = b;
            } else if (type == EXCollisionType::COL_BOX) {
                EXVector c = o.Box().Centre();
                EXVector corner = o.Box().Centre();
                const float *ext = o.Box().Extents();
                for (u32 j = 0; j < 3; j++, ext++) {
                    if (o.Box().Axis(j).Dot(c) > 0.0f) {
                        float f = *ext;
                        corner += o.Box().Axis(j) * f;
                    } else {
                        float f = *ext;
                        corner -= o.Box().Axis(j) * f;
                    }
                }
                d = corner.LengthXYZ();
            }
        }
        if (d > m_68) m_68 = d;
    }
}

// Keep the rotation of body B relative to body A inside the limits m_a..m_b (sines of the half
// angles). An out-of-range relative rotation is pulled back by slerping both bodies part of the
// way, shared by inverse inertia, and the relative angular velocity is removed in the same ratio.
void EX6DAngleConstraint::SatisfyConstraint()
{
    if (!m_pBodyA->m_active && !m_pBodyB->m_active) return;
    EXQuaternion qa(-m_pBodyA->m_rot.x, -m_pBodyA->m_rot.y, -m_pBodyA->m_rot.z, m_pBodyA->m_rot.w);
    EXQuaternion qb(-m_pBodyB->m_rot.x, -m_pBodyB->m_rot.y, -m_pBodyB->m_rot.z, m_pBodyB->m_rot.w);
    EXQuaternion rel = qa * m_pBodyB->m_rot;
    if (rel.w < 0.0f) rel = -rel;
    s8 clamped = 0;
    EXQuaternion c = rel;
    if (m_a.x > c.x) {
        c.x = m_a.x;
        clamped = 1;
    }
    if (c.x > m_b.x) {
        c.x = m_b.x;
        clamped = 1;
    }
    if (m_a.y > c.y) {
        c.y = m_a.y;
        clamped = 1;
    }
    if (c.y > m_b.y) {
        c.y = m_b.y;
        clamped = 1;
    }
    if (m_a.z > c.z) {
        c.z = m_a.z;
        clamped = 1;
    }
    if (c.z > m_b.z) {
        c.z = m_b.z;
        clamped = 1;
    }
    if (!clamped) return;
    float t = 1.0f - EXSqrt(c.DotXYZ(c));
    if (FLT_EPSILON > t) t = FLT_EPSILON;
    float wc = EXSqrt(t);
    EXQuaternion cq(-c.x, -c.y, -c.z, wc);
    t = 1.0f - EXSqrt(rel.DotXYZ(rel));
    if (FLT_EPSILON > t) t = FLT_EPSILON;
    rel.w = EXSqrt(t);
    EXQuaternion corr = cq * rel;
    if (!m_pBodyA->m_active || !m_pBodyB->m_active) {
        if (corr.w >= 0.9f) return;
        m_pBodyA->SetActive(1);
        m_pBodyB->SetActive(1);
    }
    float ia = m_pBodyA->m_invInertia.LengthXYZ();
    float ib = m_pBodyB->m_invInertia.LengthXYZ();
    float sum = ib + ia;
    float sa = m_strength * ia / sum;
    float sb = m_strength * ib / sum;
    corr.Normalise();
    EXQuaternion id(0.0f, 0.0f, 0.0f, 1.0f);
    m_pBodyA->m_rot = m_pBodyA->m_rot * id.slerp(corr, sa);
    corr.x = -corr.x;
    corr.y = -corr.y;
    corr.z = -corr.z;
    m_pBodyB->m_rot = m_pBodyB->m_rot * id.slerp(corr, sb);
    EXVector wa = m_pBodyA->m_rot.MultiplyVector(EXMaths::vec_xyzw(m_pBodyA->m_angVel));
    EXVector wb = m_pBodyB->m_rot.MultiplyVector(EXMaths::vec_xyzw(m_pBodyB->m_angVel));
    EXVector d = (wa - wb) * -1.0f;
    EXVector va = qa.MultiplyVector(EXMaths::vec_xyzw(d));
    EXVector vb = qb.MultiplyVector(EXMaths::vec_xyzw(d));
    m_pBodyA->m_angVel = m_pBodyA->m_angVel + va * sa;
    m_pBodyB->m_angVel = m_pBodyB->m_angVel - vb * sb;
}
