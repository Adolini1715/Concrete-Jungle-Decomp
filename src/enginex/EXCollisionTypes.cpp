#include <math.h>
#include "EXCollisionType.h"
#include "EXCylinder.h"
#include "EXMatrix.h"
#include "EXBounds.h"

#define EX_FLOAT_MAX 3.4028235e38f

void EXSphere::GetBoundsBox(EXBoundsBox &b) const
{
    b.Set(Centre(), Radius());
}

void EXCapsule::ApplyXForm(const EXMatrix &m)
{
    m_seg.ApplyXForm(m);
}

float EXBox::Radius() const
{
    float r = EX_FLOAT_MAX;
    for (int i = 0; i < 3; i++) {
        if (r > m_extent[i]) r = m_extent[i];
    }
    return r;
}

// NEAR: scheduling only (retail interleaves the x and y sums and keeps centre.y in a register).
void EXBox2::ComputeVertices(EXFixedArray<EXVector2, 4> &v) const
{
    EXVector2 a0 = m_axis[0] * m_extent[0];
    EXVector2 a1 = m_axis[1] * m_extent[1];
    v[0].x = m_centre.x - a0.x - a1.x;
    v[0].y = m_centre.y - a0.y - a1.y;
    v[1].x = m_centre.x + a0.x - a1.x;
    v[1].y = m_centre.y + a0.y - a1.y;
    v[2].x = m_centre.x + a0.x + a1.x;
    v[2].y = m_centre.y + a0.y + a1.y;
    v[3].x = m_centre.x - a0.x + a1.x;
    v[3].y = m_centre.y - a0.y + a1.y;
}

// NEAR: 6 bytes differ, mulss operand order in the inlined third-axis cross product.
void EXBox::ComputeVertices(EXFixedArray<EXMaths::vec_xyzw, 8> &v) const
{
    EXVector a0 = m_axis[0] * m_extent[0];
    EXVector a1 = m_axis[1] * m_extent[1];
    EXVector a2 = Axis(2) * m_extent[2];
    v[0] = Centre() - a0 - a1 - a2;
    v[1] = Centre() + a0 - a1 - a2;
    v[2] = Centre() + a0 + a1 - a2;
    v[3] = Centre() - a0 + a1 - a2;
    v[4] = Centre() - a0 - a1 + a2;
    v[5] = Centre() + a0 - a1 + a2;
    v[6] = Centre() + a0 + a1 + a2;
    v[7] = Centre() - a0 + a1 + a2;
}

void EXSphere::ApplyXForm(const EXMatrix &m)
{
    m_centre.ApplyXForm(m);
}

// NEAR: ~280 bytes differ, retail packs the named vectors into fewer stack slots (r reuses p1's temp).
void EXCapsule::GetBoundsBox(EXBoundsBox &b) const
{
    EXMaths::vec_xyzw p0, p1, r;
    p0 = Origin();
    p1 = Origin() + Direction();
    r = EXVector(Radius(), Radius(), Radius(), 0.0f);
    EXMaths::vec_xyzw mn = p0.Min(p1);
    EXMaths::vec_xyzw mx = p0.Max(p1);
    b.Set(mn - r, mx + r);
}

void EXLozenge::ApplyXForm(const EXMatrix &m)
{
    Origin().ApplyXForm(m);
    Edge0() *= m;
    Edge1() *= m;
}

// NEAR: 12 bytes differ, addss operand order in one vertex sum (VC commutes it).
void EXLozenge::GetBoundsBox(EXBoundsBox &b) const
{
    EXMaths::vec_xyzw v[4];
    v[0] = Origin();
    v[1] = Edge0() + Origin();
    v[2] = Origin() + Edge1();
    v[3] = Edge1() + (Origin() + Edge0());
    EXMaths::vec_xyzw mn = v[0];
    EXMaths::vec_xyzw mx = v[0];
    for (int i = 1; i < 4; i++) {
        mn = mn.Min(v[i]);
        mx = mx.Max(v[i]);
    }
    EXMaths::vec_xyzw r = EXVector(Radius(), Radius(), Radius(), 0.0f);
    b.Set(mn - r, mx + r);
}

void EXCylinder::ApplyXForm(const EXMatrix &m)
{
    m_centre.ApplyXForm(m);
    m_dir *= m;
}

// NEAR: ~260 bytes differ, stack slot packing as in EXCapsule::GetBoundsBox.
void EXCylinder::GetBoundsBox(EXBoundsBox &b) const
{
    EXMaths::vec_xyzw p0, p1, r;
    p0 = Centre() - Direction();
    p1 = Centre() + Direction();
    r = EXVector(Radius(), Radius(), Radius(), 0.0f);
    EXMaths::vec_xyzw mn = p0.Min(p1);
    EXMaths::vec_xyzw mx = p0.Max(p1);
    b.Set(mn - r, mx + r);
}

void EXBox::ApplyXForm(const EXMatrix &m)
{
    m_centre.ApplyXForm(m);
    for (int i = 0; i < 2; i++) m_axis[i] *= m;
}

void EXBox::GetBoundsBox(EXBoundsBox &b) const
{
    EXFixedArray<EXMaths::vec_xyzw, 8> v;
    ComputeVertices(v);
    EXMaths::vec_xyzw mx = v[0];
    EXMaths::vec_xyzw mn = v[0];
    for (int i = 1; i < 8; i++) {
        mn = mn.Min(v[i]);
        mx = mx.Max(v[i]);
    }
    b.Set(mn, mx);
}

// Oriented box that bounds the lozenge's rectangle: axes along the two edges, centre at the
// rectangle's middle, half extents of (edge + 2 radius) / 2 and the radius along the normal.
void EXBox::Set(const EXLozenge &l)
{
    const EXVector &o = l.Origin();
    const EXVector &e0 = l.Edge0();
    const EXVector &e1 = l.Edge1();
    float radius = l.Radius();

    m_axis[0] = e0;
    m_axis[0].Normalise();
    m_axis[1] = e1;
    m_axis[1].Normalise();

    m_centre.x = o.x + (e1.x + e0.x) * 0.5f;
    m_centre.y = o.y + (e1.y + e0.y) * 0.5f;
    m_centre.z = o.z + (e1.z + e0.z) * 0.5f;
    m_centre.w = o.w + (e1.w + e0.w) * 0.5f;

    float dx = e0.x + 2.0f * radius * m_axis[0].x;
    float dy = e0.y + 2.0f * radius * m_axis[0].y;
    float dz = e0.z + 2.0f * radius * m_axis[0].z;
    float s = dx * dx + dz * dz + dy * dy;
    m_extent[0] = (s < 0.0f ? 0.0f : sqrtf(s)) * 0.5f;

    dx = e1.x + 2.0f * radius * m_axis[1].x;
    dy = e1.y + 2.0f * radius * m_axis[1].y;
    dz = e1.z + 2.0f * radius * m_axis[1].z;
    s = dx * dx + dz * dz + dy * dy;
    m_extent[1] = (s < 0.0f ? 0.0f : sqrtf(s)) * 0.5f;

    m_extent[2] = radius;
}
