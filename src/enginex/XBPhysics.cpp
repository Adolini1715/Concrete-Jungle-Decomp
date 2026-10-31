#include "EXPCloth.h"
#include "EXMemory.h"
#include "EXCollisionType.h"
#include "EXCapsule.h"
#include "EXDistance.h"
#include "EXGeoEntity.h"
#include "EXGeoEntity_FaceRef.h"
#include <math.h>

// NEAR: ~250 bytes differ, register allocation around the inline rcpss block and the fixed-end branches.
// Shared edge solver: moves both ends (or the free one, twice as far) toward the rest length.
__forceinline void XbSatisfyConstraints(EXPParticle *a, EXPParticle *b, float lenSq, float rigidity, float dx, float dy, float dz, float distSq)
{
    float half = 0.5f;
    float l = lenSq;
    distSq = l + distSq;
    float r = rigidity;
    __asm {
        rcpss xmm0, distSq
        mulss xmm0, l
        subss xmm0, half
        mulss xmm0, r
        movss r, xmm0
    }
    u8 fa = (u8)(a->m_flags & 1);
    u8 fb = (u8)(b->m_flags & 1);
    dx *= r;
    dy *= r;
    dz *= r;
    if (fa | fb) {
        dx *= 2.0f;
        dy *= 2.0f;
        dz *= 2.0f;
        if (fa) {
            b->m_pos.x += dx;
            b->m_pos.y += dy;
            b->m_pos.z += dz;
        } else {
            a->m_pos.x -= dx;
            a->m_pos.y -= dy;
            a->m_pos.z -= dz;
        }
    } else {
        a->m_pos.x -= dx;
        a->m_pos.y -= dy;
        a->m_pos.z -= dz;
        b->m_pos.x += dx;
        b->m_pos.y += dy;
        b->m_pos.z += dz;
    }
}

// NEAR: ~300 bytes differ, follows from XbSatisfyConstraints (retail keeps the flag bytes and deltas in registers).
void EXPClothEdge::SatisfyConstraints()
{
    u8 fb = (u8)(m_p2->m_flags & 1);
    u8 fa = (u8)(m_p1->m_flags & 1);
    if (fa & fb) return;
    float dx = m_p2->m_pos.x - m_p1->m_pos.x;
    float dy = m_p2->m_pos.y - m_p1->m_pos.y;
    float dz = m_p2->m_pos.z - m_p1->m_pos.z;
    XbSatisfyConstraints(m_p1, m_p2, m_lengthSq, m_rigidity, dx, dy, dz, dx * dx + dy * dy + dz * dz);
}

// Variable-length edge: free between its minimum and maximum length (squared), pulled back to the
// nearer limit when outside it. Inactive edges are skipped. Unlike the plain edge there is no
// early out when both ends are fixed (the second end then moves).
void EXPVarClothEdge::SatisfyConstraints()
{
    if (!m_bActive) return;
    float dx = m_p2->m_pos.x - m_p1->m_pos.x;
    float dy = m_p2->m_pos.y - m_p1->m_pos.y;
    float dz = m_p2->m_pos.z - m_p1->m_pos.z;
    float distSq = dz * dz + dy * dy + dx * dx;
    float target = m_lengthSq;
    if (distSq > m_maxSq) target = m_maxSq;
    if (distSq < m_minSq) target = m_minSq;
    XbSatisfyConstraints(m_p1, m_p2, target, m_rigidity, dx, dy, dz, distSq);
}

// Pushes the particle out of a sphere or capsule to its surface, along the line from the centre
// (or the nearest point on the capsule's segment). With bOld the old position moves too, so the
// push adds no velocity. Other shapes are ignored.
void EXPParticle::KeepAwayFrom(EXCollisionType &c, s8 bOld)
{
    if (c.m_type == EXCollisionType::COL_SPHERE) {
        EXVector d = m_pos - c.Origin();
        float distSq = d.x * d.x + d.y * d.y + d.z * d.z;
        if (distSq >= c.Radius() * c.Radius()) return;
        d.w = 0.0f;
        float dist = distSq >= 0.0f ? sqrtf(distSq) : 0.0f;
        if (dist > EX_EPSILON_UNIT) {
            float inv = 1.0f / dist;
            d.x *= inv;
            d.y *= inv;
            d.z *= inv;
        }
        float push = c.Radius() - dist;
        m_pos.x += d.x * push;
        m_pos.y += d.y * push;
        m_pos.z += d.z * push;
        m_pos.w += d.w * push;
        if (bOld) m_oldPos = m_pos;
    } else if (c.m_type == EXCollisionType::COL_CAPSULE) {
        float t;
        float distSq = EXSqrDistance(m_pos, c.Capsule().Segment(), &t);
        float dist = distSq < 0.0f ? 0.0f : sqrtf(distSq);
        if (dist >= c.Radius()) return;
        EXVector along = c.Capsule().Direction() * t;
        EXVector d = m_pos - (c.Origin() + along);
        d.w = 0.0f;
        float len = d.x * d.x + d.y * d.y + d.z * d.z;
        if (len >= 0.0f) {
            len = sqrtf(len);
            if (len > EX_EPSILON_UNIT) {
                float inv = 1.0f / len;
                d.x *= inv;
                d.y *= inv;
                d.z *= inv;
            }
        }
        float push = c.Radius() - dist;
        m_pos.x += d.x * push;
        m_pos.y += d.y * push;
        m_pos.z += d.z * push;
        m_pos.w += d.w * push;
        if (bOld) m_oldPos = m_pos;
    }
}

// Per-particle normals for cloth drawn from an entity: each collision face's unit normal (e1 x e2;
// (1,0,0) for a degenerate face) is added to its three vertices, then each sum is divided by the
// number of faces that touched it. m_6c holds one EXVector per particle (w is the face count
// while summing, 0 after). Retail walks the first face group once per group instead of stepping
// through the groups, so only the first group contributes (repeated); kept.
void EXPCloth::FindNormals(const EXGeoEntity *pEnt, EXGeoEntity_VtxData *pVtx)
{
    EXVector *pNormals = (EXVector *)m_6c;
    if (!pNormals) return;
    memset(pNormals, 0, m_numParticles * sizeof(EXVector));
    const EXGeoEntity_FaceCollision *pFC = pEnt->FaceCollision();
    if (!pFC) return;
    const u8 *pV = (const u8 *)pVtx;
    for (int g = pFC->m_GroupCount; g > 0; g--) {
        const EXGeoEntity_FaceCollisionGroup *pGroup = pFC->m_Groups;
        const EXGeoEntity_FaceCollisionFace *pFace = pGroup->m_faces;
        for (int n = pGroup->m_02; n > 0; n--, pFace++) {
            const EXVector &v0 = *(const EXVector *)(pV + pFace->m_v[0] * 0x18);
            const EXVector &v1 = *(const EXVector *)(pV + pFace->m_v[1] * 0x18);
            const EXVector &v2 = *(const EXVector *)(pV + pFace->m_v[2] * 0x18);
            float e1x = v1.x - v0.x, e1y = v1.y - v0.y, e1z = v1.z - v0.z;
            float e2x = v2.x - v0.x, e2y = v2.y - v0.y, e2z = v2.z - v0.z;
            float nx = e2z * e1y - e2y * e1z;
            float ny = e1z * e2x - e2z * e1x;
            float nz = e2y * e1x - e1y * e2x;
            float lenSq = ny * ny + nz * nz + nx * nx;
            float len;
            if (lenSq < 0.0f || (len = sqrtf(lenSq)) == 0.0f) {
                nx = 1.0f;
                ny = 0.0f;
                nz = 0.0f;
            } else {
                float inv = 1.0f / len;
                nx *= inv;
                ny *= inv;
                nz *= inv;
            }
            for (int k = 0; k < 3; k++) {
                EXVector &acc = pNormals[pFace->m_v[k]];
                acc.x += nx;
                acc.y += ny;
                acc.z += nz;
                acc.w += 1.0f;
            }
        }
    }
    for (u32 i = 0; i < m_numParticles; i++) {
        if (pNormals[i].w > 0.1f) pNormals[i] *= 1.0f / pNormals[i].w;
        pNormals[i].w = 0.0f;
    }
}
