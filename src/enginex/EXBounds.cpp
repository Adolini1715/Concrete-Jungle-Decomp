#include "EXBounds.h"

using namespace EXMaths;

s8 EXBoundsBox3::Intersect(const EXVector &p, float r) const
{
    if (m_vctMin.x - p.x > r) return 0;
    if (m_vctMin.y - p.y > r) return 0;
    if (m_vctMin.z - p.z > r) return 0;
    float nr = 0.0f - r;
    if (nr > m_vctMax.x - p.x) return 0;
    if (nr > m_vctMax.y - p.y) return 0;
    if (nr > m_vctMax.z - p.z) return 0;
    return 1;
}

void EXBoundsBox::Set(const EXVector &c, float r)
{
    SetMin(c - EXVector(r, r, r, 0.0f));
    SetMax(c + EXVector(r, r, r, 0.0f));
}

float EXBoundsBox::CentreAndRadius(EXVector &c) const
{
    c = EXVector(m_vctMin + m_vctMax) * 0.5f;
    EXVector d = m_vctMax - m_vctMin;
    return d.LengthXYZ() * 0.5f;
}

float EXBoundsBox::CentreAndRadiusSquared(EXVector &c) const
{
    c = EXVector(m_vctMin + m_vctMax) * 0.5f;
    EXVector d = m_vctMax - m_vctMin;
    return (d.x * d.x + d.y * d.y + d.z * d.z) * 0.5f;
}

float EXBoundsBox::Volume() const
{
    EXVector d = m_vctMax - m_vctMin;
    return max(d.x, 0.001f) * max(d.y, 0.001f) * max(d.z, 0.001f);
}

void EXBoundsBox3::Set(const EXBoundsBox &b)
{
    m_vctMin.Set(b.m_vctMin.x, b.m_vctMin.y, b.m_vctMin.z);
    m_vctMax.Set(b.m_vctMax.x, b.m_vctMax.y, b.m_vctMax.z);
}

s8 EXBoundsBox3::Intersect(const EXBoundsBox &b) const
{
    if (b.m_vctMin.x > m_vctMax.x) return 0;
    if (b.m_vctMin.y > m_vctMax.y) return 0;
    if (b.m_vctMin.z > m_vctMax.z) return 0;
    if (m_vctMin.x > b.m_vctMax.x) return 0;
    if (m_vctMin.y > b.m_vctMax.y) return 0;
    if (m_vctMin.z > b.m_vctMax.z) return 0;
    return 1;
}

void EXBoundsBox::Accumulate(const EXBoundsBox &b)
{
    m_vctMin = m_vctMin.Min(b.m_vctMin);
    m_vctMax = m_vctMax.Max(b.m_vctMax);
}

// NEAR: ~130 bytes differ, register allocation in the z-slab section only (x/y slabs match).
s8 EXBoundsBox3::Intersect(const EXVector &o, const EXVector &d) const
{
    float dx = m_vctMin.x - o.x;
    if (!(dx > 0.0f || m_vctMin.y - o.y > 0.0f || m_vctMin.z - o.z > 0.0f ||
          0.0f > m_vctMax.x - o.x || 0.0f > m_vctMax.y - o.y || 0.0f > m_vctMax.z - o.z)) return 1;
    float t, a, b, inv;
    if (!((float)fabs(d.x) < 1e-5f)) {
        inv = 1.0f / d.x;
        t = inv * dx;
        if (!(0.0f > t || t > 1.0f)) {
            a = d.y * t + o.y; b = d.z * t + o.z;
            if (!(m_vctMin.z > b || b > m_vctMax.z || m_vctMin.y > a || a > m_vctMax.y)) return 1;
        }
        t = (m_vctMax.x - o.x) * inv;
        if (!(0.0f > t || t > 1.0f)) {
            a = d.y * t + o.y; b = d.z * t + o.z;
            if (!(m_vctMin.z > b || b > m_vctMax.z || m_vctMin.y > a || a > m_vctMax.y)) return 1;
        }
    }
    if (!((float)fabs(d.y) < 1e-5f)) {
        inv = 1.0f / d.y;
        t = (m_vctMin.y - o.y) * inv;
        if (!(0.0f > t || t > 1.0f)) {
            a = d.x * t + o.x; b = d.z * t + o.z;
            if (!(m_vctMin.z > b || b > m_vctMax.z || m_vctMin.x > a || a > m_vctMax.x)) return 1;
        }
        t = (m_vctMax.y - o.y) * inv;
        if (!(0.0f > t || t > 1.0f)) {
            a = d.x * t + o.x; b = d.z * t + o.z;
            if (!(m_vctMin.z > b || b > m_vctMax.z || m_vctMin.x > a || a > m_vctMax.x)) return 1;
        }
    }
    if (!((float)fabs(d.z) < 1e-5f)) {
        inv = 1.0f / d.z;
        t = (m_vctMin.z - o.z) * inv;
        if (!(0.0f > t || t > 1.0f)) {
            a = d.x * t + o.x; b = d.y * t + o.y;
            if (!(m_vctMin.y > b || b > m_vctMax.y || m_vctMin.x > a || a > m_vctMax.x)) return 1;
        }
        t = (m_vctMax.z - o.z) * inv;
        if (!(0.0f > t || t > 1.0f)) {
            a = d.x * t + o.x; b = d.y * t + o.y;
            if (!(m_vctMin.y > b || b > m_vctMax.y || m_vctMin.x > a || a > m_vctMax.x)) return 1;
        }
    }
    return 0;
}
