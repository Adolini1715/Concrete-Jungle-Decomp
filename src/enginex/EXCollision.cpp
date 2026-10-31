#include "EXCollisionType.h"
#include "EXCollisionScoopTemp.h"
#include "EXRayCastTemp.h"
#include "EXShadowTemp.h"
#include "EXInterObjectTemp.h"
#include "EXIntersection.h"
#include "EXDistance.h"
#include "EXTriangle.h"
#include "EXGeoEntity.h"
#include "EXMatrix.h"
#include "EXBounds.h"
#include "XBGeoDrawInfo.h"

EXFixedArray<EXCScoop_FaceData, 512> ScoopFaceData;
EXFixedArray<EXCScoop_EntityInfo, 64> ScoopEntityData;
EXFixedArray<EXInterObject_ContactInfo, 64> InterContactData;

void EXCollisionType::Debug_Draw(ex::RGBA c)
{
}

void EXCollisionType::GetBoundsBox(EXBoundsBox &b, unsigned int flags) const
{
    switch (m_type) {
    case COL_SPHERE:
        Sphere().GetBoundsBox(b);
        break;
    case COL_CAPSULE:
        Capsule().GetBoundsBox(b);
        break;
    case COL_LOZENGE:
        Lozenge().GetBoundsBox(b);
        break;
    case COL_BOX:
        Box().GetBoundsBox(b);
        break;
    }
}

void EXCollisionType::ApplyXForm(const EXMatrix &m)
{
    switch (m_type) {
    case COL_SPHERE:
        Sphere().ApplyXForm(m);
        break;
    case COL_CAPSULE:
        Capsule().ApplyXForm(m);
        break;
    case COL_LOZENGE:
        Lozenge().ApplyXForm(m);
        break;
    case COL_BOX:
        Box().ApplyXForm(m);
        break;
    }
}

EXVector EXCollisionType::Origin() const
{
    switch (m_type) {
    case COL_SPHERE:
        return Sphere().Centre();
    case COL_CAPSULE:
        return Capsule().Origin();
    case COL_LOZENGE:
        return Lozenge().Origin();
    }
    return EXVector(0.0f, 0.0f, 0.0f, 0.0f);
}

float EXCollisionType::Radius() const
{
    switch (m_type) {
    case COL_SPHERE:
        return Sphere().Radius();
    case COL_CAPSULE:
        return Capsule().Radius();
    case COL_LOZENGE:
        return Lozenge().Radius();
    case COL_BOX:
        return Box().Radius();
    }
    return 0.0f;
}

// NEAR (see check: retail keeps the core distance in its own slot and t in the dead triangle
// argument slot). Distance from this shape's surface to a triangle (negative radius clearance gives -9999: not
// touching). a gets the nearest point on the shape's core (centre, axis or rectangle), b the nearest
// point on the triangle, d the shape's radius less the core distance.
float EXCollisionType::Distance(const EXTriangle &tri, float &d, EXVector &a, EXVector &b) const
{
    float dist, s, t, u, rs, rt;
    switch (m_type) {
    case COL_SPHERE:
        dist = EXDistance((EXMaths::vec_xyzw)Sphere().Centre(), tri, &s, &t);
        d = Sphere().Radius() - dist;
        if (0.0f > d) return -9999.0f;
        a = Sphere().Centre();
        break;
    case COL_CAPSULE: {
        dist = EXDistance(Capsule().Segment(), tri, &u, &s, &t);
        d = Capsule().Radius() - dist;
        if (0.0f > d) return -9999.0f;
        a = Capsule().Direction() * u + Capsule().Origin();
        break;
    }
    case COL_LOZENGE: {
        dist = EXDistance(tri, Lozenge().Rectangle(), &s, &t, &rs, &rt);
        d = Lozenge().Radius() - dist;
        if (0.0f > d) return -9999.0f;
        a = Lozenge().Origin() + (Lozenge().Edge0() * rs + Lozenge().Edge1() * rt);
        break;
    }
    default:
        return -9999.0f;
    }
    b = (tri.Edge(0) * s + tri.Edge(1) * t) + tri.Origin();
    a.w = 0.0f;
    b.w = 0.0f;
    return dist;
}

// Whether this shape touches a triangle: as Distance, but returns 1 with the contact points (boxes go
// to EXFindIntersection).
signed char EXCollisionType::Intersection(const EXTriangle &tri, float &d, EXVector &a, EXVector &b) const
{
    float dist, s, t, u, rs, rt;
    switch (m_type) {
    case COL_SPHERE:
        dist = EXDistance((EXMaths::vec_xyzw)Sphere().Centre(), tri, &s, &t);
        d = Sphere().Radius() - dist;
        if (0.0f > d) return 0;
        a = Sphere().Centre();
        break;
    case COL_CAPSULE: {
        dist = EXDistance(Capsule().Segment(), tri, &u, &s, &t);
        d = Capsule().Radius() - dist;
        if (0.0f > d) return 0;
        a = Capsule().Direction() * u + Capsule().Origin();
        break;
    }
    case COL_LOZENGE: {
        dist = EXDistance(tri, Lozenge().Rectangle(), &s, &t, &rs, &rt);
        d = Lozenge().Radius() - dist;
        if (0.0f > d) return 0;
        a = Lozenge().Origin() + (Lozenge().Edge0() * rs + Lozenge().Edge1() * rt);
        break;
    }
    case COL_BOX:
        return EXFindIntersection(Box(), tri, a, b, d);
    default:
        return 0;
    }
    b = (tri.Edge(0) * s + tri.Edge(1) * t) + tri.Origin();
    a.w = 0.0f;
    b.w = 0.0f;
    return 1;
}

// NEAR (7 bytes: the operands of one addss swap). The point of this shape nearest p: the sphere's centre, the capsule axis's nearest point, or the
// nearest point of the lozenge's rectangle.
void EXCollisionType::NearestTo(const EXVector &p, EXVector &out) const
{
    switch (m_type) {
    case COL_SPHERE:
        out = Sphere().Centre();
        break;
    case COL_CAPSULE:
        Capsule().Segment().NearestTo(p, out);
        break;
    case COL_LOZENGE: {
        float s, t;
        EXDistance((EXMaths::vec_xyzw)p, Lozenge().Rectangle(), &s, &t);
        out = (Lozenge().Edge0() * s + Lozenge().Edge1() * t) + Lozenge().Origin();
        break;
    }
    }
}

// NEAR: 26 bytes differ, eax/edx swapped for the other shape in the lozenge and box cases.
signed char EXCollisionType::Intersection(const EXCollisionType &c, EXVector &a, EXVector &b, float &d) const
{
    switch (m_type) {
    case COL_SPHERE:
        if (c.m_type == COL_SPHERE) return EXFindIntersection(Sphere(), c.Sphere(), a, b, d);
        else if (c.m_type == COL_CAPSULE) return EXFindIntersection(Sphere(), c.Capsule(), a, b, d);
        else if (c.m_type == COL_LOZENGE) return EXFindIntersection(Sphere(), c.Lozenge(), a, b, d);
        else if (c.m_type == COL_BOX) return EXFindIntersection(Sphere(), c.Box(), a, b, d);
        break;
    case COL_CAPSULE:
        if (c.m_type == COL_SPHERE) return EXFindIntersection(c.Sphere(), Capsule(), b, a, d);
        else if (c.m_type == COL_CAPSULE) return EXFindIntersection(Capsule(), c.Capsule(), a, b, d);
        else if (c.m_type == COL_LOZENGE) return EXFindIntersection(Capsule(), c.Lozenge(), a, b, d);
        else if (c.m_type == COL_BOX) return EXFindIntersection(Capsule(), c.Box(), a, b, d);
        break;
    case COL_LOZENGE:
        if (c.m_type == COL_SPHERE) return EXFindIntersection(c.Sphere(), Lozenge(), b, a, d);
        else if (c.m_type == COL_CAPSULE) return EXFindIntersection(c.Capsule(), Lozenge(), b, a, d);
        else if (c.m_type == COL_LOZENGE) return EXFindIntersection(Lozenge(), c.Lozenge(), a, b, d);
        break;
    case COL_BOX:
        if (c.m_type == COL_SPHERE) return EXFindIntersection(c.Sphere(), Box(), b, a, d);
        else if (c.m_type == COL_CAPSULE) return EXFindIntersection(c.Capsule(), Box(), b, a, d);
        else if (c.m_type == COL_LOZENGE) return 0;
        else if (c.m_type == COL_BOX) return EXFindIntersection(Box(), c.Box(), a, b, d);
        break;
    }
    return 0;
}

// NEAR (16 calls to 17: MSVC merges the lozenge/capsule tail, c.Origin() call included, with the
// capsule/capsule one). Squared distance between the cores of two shapes (sphere centre, capsule axis, lozenge rectangle;
// boxes are not handled: 9999), with the nearest core points in a (this) and b (c).
float EXCollisionType::CentreSqrDistance(const EXCollisionType &c, EXVector &a, EXVector &b) const
{
    float d, s0, t0, s1, t1;
    switch (m_type) {
    case COL_SPHERE:
        if (c.m_type == COL_SPHERE) {
            a = Origin();
            b = c.Origin();
            EXVector t = a - b;
            d = t.x * t.x + t.y * t.y + t.z * t.z;
        } else if (c.m_type == COL_CAPSULE) {
            a = Origin();
            d = EXSqrDistance((EXMaths::vec_xyzw)a, c.Capsule().Segment(), &s1);
            b = c.Origin() + c.Capsule().Direction() * s1;
        } else if (c.m_type == COL_LOZENGE) {
            a = Sphere().Centre();
            d = EXSqrDistance((EXMaths::vec_xyzw)a, c.Lozenge().Rectangle(), &s1, &t1);
            b = c.Lozenge().Origin() + c.Lozenge().Edge0() * s1 + c.Lozenge().Edge1() * t1;
        } else {
            return 9999.0f;
        }
        break;
    case COL_CAPSULE:
        if (c.m_type == COL_SPHERE) {
            b = c.Sphere().Centre();
            d = EXSqrDistance((EXMaths::vec_xyzw)b, Capsule().Segment(), &s0);
            a = Origin() + Capsule().Direction() * s0;
        } else if (c.m_type == COL_CAPSULE) {
            d = EXSqrDistance(Capsule().Segment(), c.Capsule().Segment(), &s0, &s1);
            a = Origin() + Capsule().Direction() * s0;
            b = c.Origin() + c.Capsule().Direction() * s1;
        } else if (c.m_type == COL_LOZENGE) {
            d = EXSqrDistance(Capsule().Segment(), c.Lozenge().Rectangle(), &s0, &s1, &t1);
            a = Origin() + Capsule().Direction() * s0;
            b = c.Lozenge().Origin() + c.Lozenge().Edge0() * s1 + c.Lozenge().Edge1() * t1;
        } else {
            return 9999.0f;
        }
        break;
    case COL_LOZENGE:
        if (c.m_type == COL_SPHERE) {
            b = c.Sphere().Centre();
            d = EXSqrDistance((EXMaths::vec_xyzw)b, Lozenge().Rectangle(), &s0, &t0);
            a = Lozenge().Origin() + Lozenge().Edge0() * s0 + Lozenge().Edge1() * t0;
        } else if (c.m_type == COL_CAPSULE) {
            d = EXSqrDistance(c.Capsule().Segment(), Lozenge().Rectangle(), &s1, &s0, &t0);
            a = Lozenge().Origin() + Lozenge().Edge0() * s0 + Lozenge().Edge1() * t0;
            b = c.Origin() + c.Capsule().Direction() * s1;
        } else if (c.m_type == COL_LOZENGE) {
            d = EXSqrDistance(Lozenge().Rectangle(), c.Lozenge().Rectangle(), &s0, &t0, &s1, &t1);
            a = Lozenge().Origin() + Lozenge().Edge0() * s0 + Lozenge().Edge1() * t0;
            b = c.Lozenge().Origin() + c.Lozenge().Edge0() * s1 + c.Lozenge().Edge1() * t1;
        } else {
            return 9999.0f;
        }
        break;
    default:
        return 9999.0f;
    }
    a.w = 0.0f;
    b.w = 0.0f;
    return d;
}

void EXCollisionScoopTemp::ApplyXForm(const EXMatrix &m)
{
    for (u32 i = 0; i < m_numObjects; i++) m_pObjects[i].ApplyXForm(m);
}

// NEAR: 11 bytes differ, store scheduling of m_flags vs the -2 constant.
void EXCollisionScoopTemp::CalculateFaceArray()
{
    m_invXform = m_xform.FastInverse();
    int lastSub = -2;
m_flags |= 1;
    u32 lastId = 0;
    for (u32 i = 0; i < m_numEntities; i++) {
        EXCScoop_EntityInfo &e = m_pEntities[i];
        if (lastId != e.m_id || lastSub != e.m_sub) {
            lastId = e.m_id;
            lastSub = e.m_sub;
            m_mtx = e.m_mtx;
            m_invMtx = (e.m_flags & 2) ? m_mtx.Inverse() : m_mtx.FastInverse();
            m_flags |= 1;
        }
        DoCollisionPlatform(e);
    }
}

void EXCollisionScoopTemp::HandleClothCollision(EXItemPhysics &phys, void (*fn)(EXCScoop_FaceData *, EXItemPhysics &))
{
    for (u32 i = 0; i < m_numFaces; i++) {
        EXCScoop_FaceData *f = &m_pFaces[i];
        if (f->FaceFlags() & 0x400) {
            f->m_flags &= ~1;
            if (fn) fn(f, phys);
        }
    }
}

// NEAR: register allocation only (retail keeps this in ebx, hi in edi).
void EXCollisionScoopTemp::QuickSort(unsigned int lo, unsigned int hi)
{
    unsigned int l = lo;
    unsigned int h = hi;
    u32 pivot = m_pFaces[lo].m_sort;
    while (lo < hi) {
        while (m_pFaces[m_pFaces[hi].m_sort].m_dist <= m_pFaces[pivot].m_dist && lo < hi) hi--;
        if (lo != hi) {
            m_pFaces[lo].m_sort = m_pFaces[hi].m_sort;
            lo++;
        }
        while (m_pFaces[pivot].m_dist <= m_pFaces[m_pFaces[lo].m_sort].m_dist && lo < hi) lo++;
        if (lo != hi) {
            m_pFaces[hi].m_sort = m_pFaces[lo].m_sort;
            hi--;
        }
    }
    m_pFaces[lo].m_sort = pivot;
    if (l < lo) QuickSort(l, lo - 1);
    if (h > lo) QuickSort(lo + 1, h);
}

// NEAR (see check: calls match; retail keeps the offset in xmm1-4 across the loop). The scoop
// radius needed around m_centre for the shapes moved by m: per shape, the farthest point
// (sphere and capsule ends plus their radius, the box corner facing away from the centre; boxes are
// moved in place). dir gets the offset to the farthest one.
float EXCollisionScoopTemp::UpdateRadius(EXVector &dir, const EXMatrix &m)
{
    float best = 0.0f;
    EXVector v(0.0f, 0.0f, 0.0f, 0.0f);
    for (u32 i = 0; i < m_numObjects; i++) {
        EXCollisionType &o = m_pObjects[i];
        float r;
        if (o.Type() == EXCollisionType::COL_SPHERE) {
            v.Set(o.Sphere().Centre().x, o.Sphere().Centre().y, o.Sphere().Centre().z, 1.0f);
            EXVector t;
            D3DXVec4Transform(&t, &v, &m);
            v = t;
            v.w = 0.0f;
            v = v - m_centre;
            r = o.Sphere().Radius() + EXSqrt(v.Dot(v));
        } else if (o.Type() == EXCollisionType::COL_CAPSULE) {
            v.Set(o.Capsule().Origin().x, o.Capsule().Origin().y, o.Capsule().Origin().z, 1.0f);
            EXVector t;
            D3DXVec4Transform(&t, &v, &m);
            v = t;
            v.w = 0.0f;
            v = v - m_centre;
            float l0 = EXSqrt(v.Dot(v));
            EXVector e = o.Capsule().Origin() + o.Capsule().Direction();
            EXVector v1(e.x, e.y, e.z, 1.0f);
            EXVector t1;
            D3DXVec4Transform(&t1, &v1, &m);
            v1 = t1;
            v1.w = 0.0f;
            v1 = v1 - m_centre;
            float l1 = EXSqrt(v1.Dot(v1));
            if (l1 > l0) {
                v = v1;
                r = o.Capsule().Radius() + l1;
            } else {
                r = o.Capsule().Radius() + l0;
            }
        } else if (o.Type() == EXCollisionType::COL_BOX) {
            o.Box().ApplyXForm(m);
            EXVector d = o.Box().Centre() - m_centre;
            EXVector p = o.Box().Centre();
            for (int k = 0; k < 3; k++) {
                if (o.Box().Axis(k).Dot(d) > 0.0f) p += o.Box().Axis(k) * o.Box().Extent(k);
                else p -= o.Box().Axis(k) * o.Box().Extent(k);
            }
            v = p - m_centre;
            r = v.LengthXYZ();
        }
        if (r > best) {
            best = r;
            dir = v;
        }
    }
    return best;
}

void EXShadowTemp::Set(const EXVector &pos, float height, float radius, unsigned int layer, unsigned int layerFlags)
{
    m_pos = pos;
    m_height = height;
    m_radius = radius;
    m_numFaces = 0;
    m_layer.Set(layer, layerFlags);
    m_faces[0].m_dist = m_height;
    m_faces[1].m_dist = m_height;
    m_faces[3].m_dist = radius;
    m_faces[2].m_dist = radius;
    for (int i = 0; i < 4; i++) m_faces[i].m_face.Initialise();
}

void EXRayCastTemp::SetValid(EXItemAnimator *pAnim)
{
    m_pAnimator = pAnim;
    m_face.SetCheckCode();
}

// NEAR: ~50 bytes differ, retail computes &m_XtraData late and returns flags & 1 as a byte op.
signed char EXCollisionLayer::CalcCollisionEntity(EXGeoBaseEntity *&pEnt) const
{
    u32 flags = m_flags;
    if (flags & 0x400) return 1;
    EXGeoBaseEntity *p = pEnt;
    u32 layer = m_layer;
    if (p->HasCollisionEntity(layer)) goto found;
    while (layer > 0) {
        layer--;
        if ((flags & (2 << layer)) && p->HasCollisionEntity(layer)) goto found;
    }
    if (m_layer == 0) return 1;
    return (u8)flags & 1;
found:
    pEnt = p->Collision(layer);
    return 1;
}

// NEAR: ~140 bytes differ, retail shares the return-1 epilogue (mov al,1 before the first test).
signed char EXCollisionLayer::CalcGrabpointEntity(EXGeoBaseEntity *&pEnt) const
{
    if (m_flags & 0x400) return 1;
    u32 layer = m_layer;
    EXGeoBaseEntity *p = pEnt;
    if (p->HasCollisionEntity(layer)) {
        p = p->Collision(layer);
        if (p->Collision(6)) {
            pEnt = p;
            return 1;
        }
    }
    while (layer > 0) {
        layer--;
        if (m_flags & (2 << layer)) {
            p = pEnt;
            if (p->HasCollisionEntity(layer)) {
                p = p->Collision(layer);
                if (p->Collision(6)) {
                    pEnt = p;
                    return 1;
                }
            }
        }
    }
    if (m_flags & 1) return 1;
    if (m_layer == 0) return 1;
    return pEnt->Collision(6) != 0;
}

void EXCollisionScoopTemp::Initialise()
{
    m_pFaces = ScoopFaceData;
    m_maxFaces = 0x200;
    m_numFaces = 0;
    m_pEntities = ScoopEntityData;
    m_maxEntities = 0x40;
    m_numEntities = 0;
    m_layer.m_layer = 0;
    m_layer.m_flags = 0;
    m_numObjects = 0;
    m_pObjects = 0;
    m_flags = 0;
}

void EXInterObjectTemp::Initialise()
{
    m_pContacts = InterContactData;
    m_maxContacts = 0x40;
    m_numContacts = 0;
}

// NEAR: ~170 bytes differ, retail copies the min/max results through extra EXVector temporaries.
void EXRayCastTemp::CalcBounds()
{
    EXVector e = Intersect();
    m_bounds.Set(EXMaths::Min(m_start, e), EXMaths::Max(m_start, e));
}

// NEAR: as CalcBounds.
void EXRayCastTemp::CalcBoundsTemp()
{
    EXVector e = m_localStart + m_localDir * m_t;
    m_localBounds.Set(EXMaths::Min(m_localStart, e), EXMaths::Max(m_localStart, e));
}

void EXRayCastTemp::Set(const EXVector &start, const EXVector &dir, unsigned int layer, unsigned int layerFlags,
                        unsigned int flags, unsigned int mask)
{
    m_start = start;
    m_dir = dir;
    m_flags = flags;
    m_mask = mask;
    m_bHit = 0;
    m_pAnimator = 0;
    FindClosestHit();
    m_t = 1.0f;
    m_layer.Set(layer, layerFlags);
    CalcBounds();
}

// NEAR (see check: calls match). The scoop's bounding sphere in m's space. One shape: its own bound. Several: start at the first
// shape's origin and walk the centre towards the farthest point iters times (halving the step),
// keeping the smallest radius found.
void EXCollisionScoopTemp::CalcBounds(const EXMatrix &m, unsigned char iters)
{
    EXVector t;
    if (m_numObjects == 1) {
        EXCollisionType &o = m_pObjects[0];
        if (o.Type() == EXCollisionType::COL_SPHERE) {
            m_radius = o.Sphere().Radius();
            m_centre = o.Sphere().Centre();
        } else if (o.Type() == EXCollisionType::COL_CAPSULE) {
            m_radius = o.Capsule().Direction().LengthXYZ() * 0.5f + o.Capsule().Radius();
            m_centre = o.Capsule().Origin() + o.Capsule().Direction() * 0.5f;
        } else {
            m_radius = EXVector3(o.Box().Extent(0), o.Box().Extent(1), o.Box().Extent(2)).Length();
            m_centre = o.Box().Centre();
        }
        m_centre.w = 1.0f;
        D3DXVec4Transform(&t, &m_centre, &m);
        m_centre = t;
        m_centre.w = 0.0f;
        return;
    }
    m_centre = m_pObjects[0].Origin();
    m_centre.w = 1.0f;
    D3DXVec4Transform(&t, &m_centre, &m);
    m_centre = t;
    m_centre.w = 0.0f;
    EXVector dir;
    m_radius = UpdateRadius(dir, m);
    float step = dir.Length() * 0.5f;
    dir.Normalise();
    for (int i = 0; i < iters; i++) {
        m_centre += dir * step;
        float r = UpdateRadius(dir, m);
        if (m_radius > r) m_radius = r;
        dir.Normalise();
        step *= 0.5f;
    }
}

// NEAR (see check: calls match). Classifies the scooped faces (m_flags): 1 wall, 0x11 ceiling (normal below -0.8), 3 floor (above
// 0.8), plus 0x40 when the shape point lies over the floor triangle (in XZ). Faces seen from behind
// are dropped unless two-sided (face flag 0x100), whose normal is then turned round.
void EXCollisionScoopTemp::CalculateFaceTypes(signed char b)
{
    for (u32 i = 0; i < m_numFaces; i++) {
        EXCScoop_FaceData &f = m_pFaces[i];
        f.m_flags = 0;
        EXVector e = f.P1() - f.P0();
        e.NormaliseXYZ();
        EXVector n = f.Normal();
        if (f.m_3e & 0x80) n = -n;
        if (n.y < 0.8f && n.y > -0.8f) {
            e.y = 0.0f;
        } else {
            e.x = 0.0f;
            e.z = 0.0f;
        }
        if (n.DotXYZ(e) >= 0.0f) {
            if (!(f.FaceFlags() & 0x100)) continue;
            n = -n;
        }
        if (n.y > 0.8f) {
            EXCScoop_EntityInfo *pInfo = (EXCScoop_EntityInfo *)f.m_34;
            EXGeoEntity *pEnt = (EXGeoEntity *)pInfo->m_40;
            EXGDiData *pGDi = (EXGDiData *)pInfo->m_44;
            EXGeoEntity_VtxData *pVtx;
            signed char bLocked;
            if (pGDi && pGDi->HasModifiedVerts()) {
                bLocked = 1;
                pVtx = (EXGeoEntity_VtxData *)pEnt->GetVertexBuffer(pGDi, 1);
            } else {
                bLocked = 0;
                pVtx = pEnt->Vtx();
            }
            EXVector v0 = f.vtx0(pVtx);
            EXVector v1 = f.vtx1(pVtx);
            EXVector v2 = f.vtx2(pVtx);
            if (bLocked) pEnt->ReleaseVertexBuffer(pGDi);
            v0.w = 1.0f;
            v1.w = 1.0f;
            v2.w = 1.0f;
            EXMaths::vec_xyzw tri[3];
            EXVector t;
            D3DXVec4Transform(&t, &v0, &pInfo->m_mtx);
            tri[0] = t;
            D3DXVec4Transform(&t, &v1, &pInfo->m_mtx);
            tri[1] = t;
            D3DXVec4Transform(&t, &v2, &pInfo->m_mtx);
            tri[2] = t;
            if (f.P0().PointInTriY(tri)) f.m_flags |= 0x40;
            f.m_flags |= 3;
        } else if (-0.8f > n.y) {
            f.m_flags = 0x11;
        } else {
            f.m_flags = 1;
        }
    }
}

// NEAR (see check: calls match). Pushes the moving shapes (offset by v) out of the classified faces, iters passes: each touching
// face (dist > 0) pushes v back along the face normal on the first pass (along the contact
// direction after), walls only sideways (n |= 2), floors only up (n |= 1). Layer flags 0x1000 /
// 0x2000 skip walls / floors. fn is told about each face once (face flag 4); flag 8 faces only
// report.
void EXCollisionScoopTemp::CalculateCollisionResponse(EXItemPhysics &phys, EXVector &v, unsigned short &n, unsigned int iters,
                                                      void (*fn)(EXCScoop_FaceData *, EXItemPhysics &))
{
    signed char bFloors = !(m_layer.m_flags & 0x2000);
    signed char bWalls = !(m_layer.m_flags & 0x1000);
    if (!bWalls && !bFloors) return;
    for (u32 pass = 0; pass < iters; pass++) {
        for (u32 i = 0; i < m_numFaces; i++) {
            EXCScoop_FaceData &f = m_pFaces[i];
            signed char bUse = 0;
            if (bWalls && !(f.m_flags & 2)) bUse = 1;
            if (bFloors && (f.m_flags & 2)) bUse = 1;
            if (!(f.m_flags & 1) || !bUse) continue;
            EXCScoop_EntityInfo *pInfo = (EXCScoop_EntityInfo *)f.m_34;
            EXGeoEntity *pEnt = (EXGeoEntity *)pInfo->m_40;
            EXGDiData *pGDi = (EXGDiData *)pInfo->m_44;
            EXGeoEntity_VtxData *pVtx;
            signed char bLocked;
            if (pGDi && pGDi->HasModifiedVerts()) {
                bLocked = 1;
                pVtx = (EXGeoEntity_VtxData *)pEnt->GetVertexBuffer(pGDi, 1);
            } else {
                bLocked = 0;
                pVtx = pEnt->Vtx();
            }
            EXVector v0 = f.vtx0(pVtx);
            EXVector v1 = f.vtx1(pVtx);
            EXVector v2 = f.vtx2(pVtx);
            if (bLocked) pEnt->ReleaseVertexBuffer(pGDi);
            v0.w = 1.0f;
            v1.w = 1.0f;
            v2.w = 1.0f;
            EXVector t0, t1, t2;
            D3DXVec4Transform(&t2, &v2, &pInfo->m_mtx);
            D3DXVec4Transform(&t1, &v1, &pInfo->m_mtx);
            D3DXVec4Transform(&t0, &v0, &pInfo->m_mtx);
            EXTriangle tri;
            tri.m_v[1] = t1 - t0;
            tri.m_v[0] = t0;
            tri.m_v[2] = t2 - t0;
            EXCollisionType o = m_pObjects[f.m_3e & 0x7f];
            o.ApplyXForm(m_xform);
            if (o.Type() == EXCollisionType::COL_CAPSULE) o.Capsule().Origin() += v;
            else o.Sphere().Centre() += v;
            o.Distance(tri, f.m_dist, f.P0(), f.P1());
            EXVector d = (f.P1() - f.P0()).UnitVectorXYZ();
            float dist = f.m_dist;
            unsigned char flags = f.m_flags;
            float dot;
            if (!(flags & 0x40)) {
                dot = d.Dot(f.Normal());
                if (pass == 0) d = f.Normal();
                if (dot < 0.0f) {
                    if (pass == 0) d = -d;
                    dot = -dot;
                }
            } else {
                dot = 0.0f;
            }
            if (dist > 0.0f && (pass != 0 || dot > 0.99999f || (flags & 2))) {
                unsigned short old = n;
                d.x *= dist;
                d.y *= dist;
                d.z *= dist;
                if (flags & 0x20) d.y = 0.0f;
                if (!(flags & 2)) {
                    if (!(flags & 0x10)) n = old | 2;
                    if ((old & 1) && d.y > 0.0f) d.y = 0.0f;
                } else {
                    if (!(flags & 0x40) && dot <= 0.9999f) continue;
                    d.x = 0.0f;
                    d.z = 0.0f;
                    n = old | 1;
                }
                if (!(f.m_flags & 4)) {
                    if (fn) fn(&f, phys);
                    f.m_flags |= 4;
                }
                if (!(f.m_flags & 8)) {
                    v.x -= d.x;
                    v.y -= d.y;
                    v.z -= d.z;
                    v.w -= d.w * dist;
                } else {
                    n = old;
                }
            }
        }
    }
}

// NEAR (see check). As CalculateCollisionResponse for a shape whose wall and floor pushes go to
// separate vectors (a: walls, b: floors). The shapes move with the total push so far; layer flag
// 0x4000 walks the faces backwards.
void EXCollisionScoopTemp::CalculateDualCollisionResponse(EXItemPhysics &phys, EXVector &a, EXVector &b, unsigned short &n,
                                                          unsigned int iters, void (*fn)(EXCScoop_FaceData *, EXItemPhysics &))
{
    signed char bFloors = !(m_layer.m_flags & 0x2000);
    signed char bWalls = !(m_layer.m_flags & 0x1000);
    EXVector acc(0.0f, 0.0f, 0.0f, 0.0f);
    if (!bWalls && !bFloors) return;
    for (u32 pass = 0; pass < iters; pass++) {
        for (u32 i = 0; i < m_numFaces; i++) {
            u32 fi = i;
            if (m_layer.m_flags & 0x4000) fi = m_numFaces - i - 1;
            EXCScoop_FaceData &f = m_pFaces[fi];
            signed char bUse = 0;
            if (bWalls && !(f.m_flags & 2)) bUse = 1;
            if (bFloors && (f.m_flags & 2)) bUse = 1;
            if (!(f.m_flags & 1) || !bUse) continue;
            EXCScoop_EntityInfo *pInfo = (EXCScoop_EntityInfo *)f.m_34;
            EXGeoEntity *pEnt = (EXGeoEntity *)pInfo->m_40;
            EXGDiData *pGDi = (EXGDiData *)pInfo->m_44;
            EXGeoEntity_VtxData *pVtx;
            signed char bLocked;
            if (pGDi && pGDi->HasModifiedVerts()) {
                bLocked = 1;
                pVtx = (EXGeoEntity_VtxData *)pEnt->GetVertexBuffer(pGDi, 1);
            } else {
                bLocked = 0;
                pVtx = pEnt->Vtx();
            }
            EXVector v0 = f.vtx0(pVtx);
            EXVector v1 = f.vtx1(pVtx);
            EXVector v2 = f.vtx2(pVtx);
            if (bLocked) pEnt->ReleaseVertexBuffer(pGDi);
            v0.w = 1.0f;
            v1.w = 1.0f;
            v2.w = 1.0f;
            EXVector t0, t1, t2;
            D3DXVec4Transform(&t2, &v2, &pInfo->m_mtx);
            D3DXVec4Transform(&t1, &v1, &pInfo->m_mtx);
            D3DXVec4Transform(&t0, &v0, &pInfo->m_mtx);
            EXTriangle tri;
            tri.m_v[1] = t1 - t0;
            tri.m_v[0] = t0;
            tri.m_v[2] = t2 - t0;
            EXCollisionType o = m_pObjects[f.m_3e & 0x7f];
            o.ApplyXForm(m_xform);
            o.Sphere().Centre() += acc;
            o.Distance(tri, f.m_dist, f.P0(), f.P1());
            EXVector d = f.P1() - f.P0();
            unsigned char flags = f.m_flags;
            float dist = f.m_dist;
            float dot = 0.0f;
            if (!(flags & 0x40)) {
                d = d.UnitVectorXYZ();
                dot = f.Normal().Dot(d);
                if (pass == 0) {
                    d.x = f.Normal().x;
                    d.y = f.Normal().y;
                    d.z = f.Normal().z;
                }
                if (dot < 0.0f) {
                    if (pass == 0) {
                        d.x = 0.0f - d.x;
                        d.y = 0.0f - d.y;
                        d.z = 0.0f - d.z;
                    }
                    dot = 0.0f - dot;
                }
            }
            if (dist > 0.0f && (pass != 0 || dot > 0.9999f || (flags & 0x40))) {
                d = d.UnitVectorXYZ();
                unsigned short old = n;
                d *= dist;
                if (flags & 0x20) d.y = 0.0f;
                if (!(flags & 2)) {
                    if (!(flags & 0x10)) n = old | 2;
                    if ((old & 1) && d.y > 0.0f) d.y = 0.0f;
                } else {
                    if (!(flags & 0x40) && dot <= 0.9999f) continue;
                    n = old | 1;
                }
                if (!(f.m_flags & 4)) {
                    if (fn) fn(&f, phys);
                    f.m_flags |= 4;
                }
                if (!(f.m_flags & 8)) {
                    if (!(f.m_flags & 2)) {
                        a -= d;
                        acc.x -= d.x;
                        acc.z -= d.z;
                    } else {
                        b -= d;
                    }
                    acc.y -= d.y;
                    acc.w -= d.w;
                } else {
                    n = old;
                }
            }
        }
    }
}

