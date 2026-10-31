#include "Avoid.h"
#include "Actor.h"
#include "XNavLocation.h"
#include "EXMemory.h"
#include "FEText.h"

// NEAR (retail [fi] copy): retail computes sin before the dist == 1 test (single fsin).
__forceinline void SetVector_RotateY(EXVector &v, float ang, float dist)
{
    if (dist == 1.0f) {
        v.x = (float)sin(ang);
        v.z = (float)cos(ang);
    } else {
        v.x = (float)sin(ang) * dist;
        v.z = (float)cos(ang) * dist;
    }
    v.w = 0.0f;
    v.y = 0.0f;
}

CAvoidSphere::CAvoidSphere() {}

CAvoidSphere::CAvoidSphere(const EXVector &pos, const EXVector &prev, float radius)
{
    m_Pos = pos;
    m_MoveVec = pos - prev;
    m_Radius = radius;
}

CAvoidSphere::~CAvoidSphere() {}

CObstacle::CObstacle() {}

CObstacle::~CObstacle() {}

// NEAR: SetVector_RotateY shape and store scheduling differ.
void CObstacle::Init(const EXVector &pos, const float &ang, float velocity, float radius, float mass)
{
    m_Sphere.m_Pos = pos;
    m_Bottom = pos.y;
    m_Top = pos.y + 1.5f;
    m_Mass = mass;
    m_Ang = ang;
    m_Velocity = velocity;
    m_Sphere.m_Radius = radius;
    m_BlockBits = 0;
    if (velocity > 0.005f)
        SetVector_RotateY(m_Sphere.m_MoveVec, ang, velocity);
    else
        m_Sphere.m_MoveVec.Set(0.0f, 0.0f, 0.0f, 0.0f);
    m_Sphere.m_MoveVec.y = 0.0f;
    m_Sphere.m_Pos.y = 0.0f;
}

CAvoidance::~CAvoidance() {}

void CAvoidance::UpdatePerFrame() {}

CAvoidance::CAvoidance()
{
    m_PushDelta.w = 0.0f;
    m_PushDelta.z = 0.0f;
    m_PushDelta.y = 0.0f;
    m_PushDelta.x = 0.0f;
}

// Solve a*t^2 + b*t + c = 0. A double root is reported as 0, 0 (retail does this); no real
// roots returns 0.
signed char CAvoidance::QuadraticFormula(float a, float b, float c, float &r0, float &r1)
{
    float disc = b * b - a * c * 4.0f;
    if (disc == 0.0f) {
        r0 = 0.0f;
        r1 = 0.0f;
        return 1;
    }
    if (disc > 0.0f) {
        float inv = 1.0f / (a * 2.0f);
        r0 = ((float)sqrt(disc) - b) * inv;
        r1 = ((0.0f - b) - (float)sqrt(disc)) * inv;
        return 1;
    }
    return 0;
}

// Time window [t0, t1] (in frames of movement) during which the two circles overlap in the
// ground plane. Already overlapping gives 0, 0. Returns 0 when they never touch.
bool const CAvoidance::SphereSphereSweep(CAvoidSphere *a, CAvoidSphere *b, float &t0, float &t1)
{
    float dx = b->m_Pos.x - a->m_Pos.x;
    float dz = b->m_Pos.z - a->m_Pos.z;
    float r = a->m_Radius + b->m_Radius;
    float vx = b->m_MoveVec.x - a->m_MoveVec.x;
    float vz = b->m_MoveVec.z - a->m_MoveVec.z;
    r = r * r;
    float dist2 = dz * dz + dx * dx;
    if (dist2 <= r) {
        t0 = 0.0f;
        t1 = 0.0f;
        return 1;
    }
    if (QuadraticFormula(vz * vz + vx * vx, (vz * dz + vx * dx) * 2.0f, dist2 - r, t0, t1)) {
        if (t1 <= t0 && t0 != t1) {
            t0 = t1;
            t1 = t1;
        }
        return 1;
    }
    return 0;
}

// ArcCos that clamps out of range input: below -1 is pi, above 1 is 0, NaN is 0.
static __forceinline float SafeAcos(float v)
{
    if (v < -1.0f) return 3.1415927f;
    if (v > 1.0f) return 0.0f;
    float a = (float)acos(v);
    if (a != a) return 0.0f;
    return a;
}

// Right-angle triangle on a circle: from point "from" a tangent just touches the circle of
// radius "radius" round "centre". Returns the tangent point; ang0 is the heading to the
// centre minus the tangent angle, ang1 the heading from the centre to the tangent point,
// ang2 the heading from "from" to the centre.
EXVector CAvoidance::RHTriangleCPos(const EXVector &from, const EXVector &centre, float radius, float &ang0, float &ang1,
                                    float &ang2)
{
    float dx = centre.x - from.x;
    float dz = centre.z - from.z;
    float dy = centre.y - from.y;
    float dw = centre.w - from.w;
    float d2 = dy * dy + dz * dz + dw * dw + dx * dx;
    float dist = d2 < 0.0f ? 0.0f : (float)sqrt(d2);
    float t2 = dist * dist - radius * radius;
    float tangent = t2 < 0.0f ? 0.0f : (float)sqrt(t2);
    float inv = 1.0f / dist;
    float alpha = SafeAcos(inv * tangent);
    float beta = SafeAcos(inv * radius);
    double yaw = atan2(dx, dz);
    ang2 = (float)yaw;
    ang1 = (float)((yaw - 3.1415927f) + beta);
    ang0 = ang2 - alpha;
    double s = sin(ang1);
    double c = cos(ang1);
    EXVector r;
    r.x = (float)(s * radius + centre.x);
    r.y = centre.y;
    r.z = (float)(c * radius) + centre.z;
    r.w = centre.w;
    return r;
}

// Overlap test and push-apart between the two obstacles. "delta" gets the ground-plane vector
// from B to A and "dist" its length. Returns -1 when they are touching (and PushDelta has been
// updated), 0 otherwise.
int CAvoidance::Collide(EXVector &delta, float &dist)
{
    float dx = m_ObsA.m_Sphere.m_Pos.x - m_ObsB.m_Sphere.m_Pos.x;
    float dz = m_ObsA.m_Sphere.m_Pos.z - m_ObsB.m_Sphere.m_Pos.z;
    delta.z = dz;
    float lenSq = dz * dz + dx * dx;
    delta.x = dx;
    delta.w = 0.0f;
    delta.y = 0.0f;
    dist = lenSq < 0.0f ? 0.0f : (float)sqrt(lenSq);
    if (dist <= 64.0f && m_ObsA.m_Bottom <= m_ObsB.m_Top && m_ObsB.m_Bottom <= m_ObsA.m_Top) {
        m_fTouchDist = (m_ObsB.m_Sphere.m_Radius + m_ObsA.m_Sphere.m_Radius) - 0.2f;
        if (dist < m_fTouchDist + 1.0f) {
            EXVector away(0.0f - delta.x, 0.0f - delta.y, 0.0f - delta.z, 0.0f - delta.w);
            m_ObsA.m_BlockBits |= XNavLocation::CalcBlockBits(away);
        }
        if (dist < m_fTouchDist) {
            float share = (m_ObsB.m_Mass / (m_ObsA.m_Mass + m_ObsB.m_Mass)) * 2.0f;
            if (share > 1.0f) share = 1.0f;
            float l2 = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z + delta.w * delta.w;
            if (l2 >= 0.0f) {
                float l = (float)sqrt(l2);
                if (l > 1e-5f) {
                    float inv = 1.0f / l;
                    delta.x *= inv;
                    delta.y *= inv;
                    delta.z *= inv;
                    delta.w *= inv;
                }
            }
            if (m_ObsA.m_Mass == m_ObsB.m_Mass && m_ObsA.m_Velocity < m_ObsB.m_Velocity) return -1;
            float f = (m_fTouchDist - dist) / m_fTouchDist;
            float py = delta.y * f;
            float pz = f * delta.z;
            float pw = delta.w * f;
            m_PushDelta.x = delta.x * f * share + m_PushDelta.x;
            m_PushDelta.y = py * share + m_PushDelta.y;
            m_PushDelta.z = pz * share + m_PushDelta.z;
            m_PushDelta.w = pw * share + m_PushDelta.w;
            return -1;
        }
        return 0;
    }
    return 0;
}

float AngleDiff(float a, float b);

static __forceinline float WrapPolar(float a)
{
    if (a < 0.0f || a >= 6.2831855f) {
        a = (float)fmod(a, 6.2831855f);
        if (a < 0.0f) a += 6.2831855f;
    }
    return a;
}

// Will A hit B if both carry on? Fills in the hit time, the impact positions and the
// direction between them, then sets AngleChange to how far A should turn (positive turns
// one way, negative the other) to miss. Returns 0 when there is nothing to avoid (no hit,
// too far off, or A is already facing away from the way it would turn), 1 otherwise.
int CAvoidance::Evaluate(EXVector &, float &)
{
    float tMax;
    if (!SphereSphereSweep(&m_ObsA.m_Sphere, &m_ObsB.m_Sphere, m_fHitTime, tMax)) return 0;
    if (m_fHitTime < 0.0f) return 0;
    if (!(m_ObsB.m_Flags & 2))
        m_fClosing = (float)((double)m_ObsA.m_Velocity - cos(AngleDiff(m_ObsB.m_Ang, m_ObsA.m_Ang)) * m_ObsB.m_Velocity);
    else
        m_fClosing = m_ObsA.m_Velocity;
    float horizon = 2000.0f;
    float dist = 0.0f;
    if (m_fClosing * horizon <= m_fHitTime) return 0;
    if (m_fHitTime != 0.0f) {
        float t = m_fHitTime;
        m_ObsA.m_ImpactPos.x = m_ObsA.m_Sphere.m_MoveVec.x * t + m_ObsA.m_Sphere.m_Pos.x;
        m_ObsA.m_ImpactPos.y = m_ObsA.m_Sphere.m_Pos.y + m_ObsA.m_Sphere.m_MoveVec.y * t;
        m_ObsA.m_ImpactPos.z = m_ObsA.m_Sphere.m_Pos.z + m_ObsA.m_Sphere.m_MoveVec.z * t;
        m_ObsA.m_ImpactPos.w = m_ObsA.m_Sphere.m_Pos.w + m_ObsA.m_Sphere.m_MoveVec.w * t;
        if (!(m_ObsB.m_Flags & 2)) {
            m_ObsB.m_ImpactPos.x = m_ObsB.m_Sphere.m_MoveVec.x * t + m_ObsB.m_Sphere.m_Pos.x;
            m_ObsB.m_ImpactPos.y = m_ObsB.m_Sphere.m_Pos.y + m_ObsB.m_Sphere.m_MoveVec.y * t;
            m_ObsB.m_ImpactPos.z = m_ObsB.m_Sphere.m_Pos.z + m_ObsB.m_Sphere.m_MoveVec.z * t;
            m_ObsB.m_ImpactPos.w = m_ObsB.m_Sphere.m_Pos.w + m_ObsB.m_Sphere.m_MoveVec.w * t;
        } else {
            m_ObsB.m_ImpactPos = m_ObsB.m_Sphere.m_Pos;
        }
    } else {
        m_ObsA.m_ImpactPos = m_ObsA.m_Sphere.m_Pos;
        m_ObsB.m_ImpactPos = m_ObsB.m_Sphere.m_Pos;
    }
    m_ImpactDir.x = m_ObsB.m_ImpactPos.x - m_ObsA.m_ImpactPos.x;
    m_ImpactDir.y = m_ObsB.m_ImpactPos.y - m_ObsA.m_ImpactPos.y;
    m_ImpactDir.z = m_ObsB.m_ImpactPos.z - m_ObsA.m_ImpactPos.z;
    m_ImpactDir.w = m_ObsB.m_ImpactPos.w - m_ObsA.m_ImpactPos.w;
    m_ImpactDir.y = 0.0f;
    float lenSq = m_ImpactDir.x * m_ImpactDir.x + 0.0f + m_ImpactDir.z * m_ImpactDir.z + m_ImpactDir.w * m_ImpactDir.w;
    if (lenSq >= 0.0f) {
        dist = (float)sqrt(lenSq);
        if (dist > 1e-5f) {
            float inv = 1.0f / dist;
            m_ImpactDir.x *= inv;
            m_ImpactDir.y *= inv;
            m_ImpactDir.z *= inv;
            m_ImpactDir.w *= inv;
        }
    }
    m_fImpactDist = dist;
    m_fVelShare = m_ObsA.m_Velocity / (m_ObsA.m_Velocity + m_ObsB.m_Velocity);
    float urgency = 1.0f - (1.0f / (m_fClosing * horizon)) * m_fHitTime;
    m_AngleChange = m_fVelShare * urgency * 1.5707964f;
    m_fUrgency = urgency;
    float angA = AngleDiff(m_ObsA.m_Ang, (float)atan2(m_ImpactDir.x, m_ImpactDir.z));
    float angB = AngleDiff(m_ObsB.m_Ang, (float)atan2(0.0f - m_ImpactDir.x, 0.0f - m_ImpactDir.z));
    m_fAngA = angA;
    angA = WrapPolar(angA);
    angB = WrapPolar(angB);
    int quadA = (int)(angA * -0.63661975f);
    if (quadA == 0) {
        if (m_ObsB.m_Velocity <= 0.0f || (int)(angB * -0.63661975f) != 0) {
            m_AngleChange = 0.0f - m_AngleChange;
            return 1;
        }
    } else if (quadA == -1 || quadA == -2) {
        m_AngleChange = 0.0f;
        return 0;
    }
    return 1;
}

// Work out how much "pActor" should veer from its wanted heading destAng to avoid every other
// actor within 64 units. Each threat gives a turn (see Evaluate); turns the same way keep the
// larger, turns opposed to each other add. The result blends with the actor's own turn
// towards destAng, is clamped to about +-90 degrees and remembered for next frame.
float CAvoidance::Steer(Actor *pActor, float destAng)
{
    float best = 0.0f;
    int count = 0;
    float vel = pActor->GetAnimVelocity();
    float ang = pActor->m_AvoidLastSteer * 0.6f + pActor->GetRotation().y;
    m_ObsA.Init(pActor->GetFeetPos(), ang, vel, pActor->m_CollisionRadius + 0.1f, pActor->m_Mass);
    m_ObsA.m_DestAng = destAng;
    m_PushDelta.w = 0.0f;
    m_PushDelta.z = 0.0f;
    m_PushDelta.y = 0.0f;
    m_PushDelta.x = 0.0f;
    for (Actor *pOther = Actor::GetFirst(pActor->GetFeetPos(), 64.0f, 0); pOther;
         pOther = pOther->GetNext(pActor->GetFeetPos(), 64.0f, 0)) {
        if (pOther == pActor) continue;
        if (!((pOther->m_Health > 0.0f || pOther->m_StateFlags.m_bCanRipBossHead) || pOther->m_ActorFlags.m_bUlysses)) continue;
        if (pActor->m_MeetActorID == pOther->m_RefID) continue;
        m_ObsA.m_Flags = pOther->m_RefID < pActor->m_RefID;
        float otherVel = pOther->GetAnimVelocity();
        float mass = pOther->m_Mass;
        if (pOther->m_Status == 1) mass = 1000.0f;
        float otherAng = pOther->GetRotation().y;
        m_ObsB.Init(pOther->GetFeetPos(), otherAng, otherVel, pOther->m_CollisionRadius + 0.1f, mass);
        m_AngleChange = 0.0f;
        m_Unkf4 = 1.0f;
        EXVector delta;
        float dist;
        Collide(delta, dist);
        if (vel >= 0.01f && Evaluate(delta, dist)) {
            float change = m_AngleChange;
            EXVector dir((float)sin(destAng), 0.0f, (float)cos(destAng), 0.0f);
            int side = pOther->m_OrgLoc.NegotiateDir(dir, pActor->m_OrgLoc.m_wObjBlockBits | pActor->m_OrgLoc.m_wGeoBlockBits);
            float diff = AngleDiff((float)atan2(dir.x, dir.z), destAng);
            if (fabs(diff) <= 1.4f && ((side > 0 && change < 0.0f) || (side < 0 && change > 0.0f))) change = 0.0f - change;
            float merged = change;
            if (change >= 0.0f || best > 0.0f) {
                if (change <= 0.0f || best < 0.0f)
                    merged = change + best;
                else if (change <= best)
                    continue;
            } else if (best <= change) {
                continue;
            }
            count++;
            best = merged;
        }
    }
    pActor->m_OrgLoc.m_wObjBlockBits = 0;
    pActor->m_OrgLoc.m_wObjBlockBits |= m_ObsA.m_BlockBits;
    float own = AngleDiff(pActor->GetRotation().y, destAng);
    float steer = (float)((double)pActor->m_AvoidLastSteer * 0.6f + own);
    float result = steer;
    if (count > 0) {
        result = best;
        if (best >= 0.0f || steer >= 0.0f) {
            if (best <= 0.0f || steer <= 0.0f)
                result = steer + best;
            else if (steer > best)
                result = steer;
        } else if (!(best <= steer)) {
            result = steer;
        }
    }
    if (result < -1.57f) result = -1.57f;
    else if (result > 1.57f) result = 1.57f;
    pActor->m_AvoidLastSteer = result;
    return result;
}
