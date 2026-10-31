#include "EXBounds.h"

using namespace EXMaths;

s8 MXBoundsBox::_Intersect(const vec_xyzw &p, float r) const
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

s8 MXBoundsBox::_Intersect(const MXBoundsBox &b) const
{
    if (b.m_vctMin.x > m_vctMax.x) return 0;
    if (b.m_vctMin.y > m_vctMax.y) return 0;
    if (b.m_vctMin.z > m_vctMax.z) return 0;
    if (m_vctMin.x > b.m_vctMax.x) return 0;
    if (m_vctMin.y > b.m_vctMax.y) return 0;
    if (m_vctMin.z > b.m_vctMax.z) return 0;
    return 1;
}

// Segment o .. o + d against the box: inside test first, then the six slab planes (t in [0, 1]).
// NEAR: ~3 bytes of real difference (the original keeps min.x - o.x in xmm2 and multiplies with a
// movaps copy of 1/d.x); everything after is the same code shifted, plus the z slab's register order.
s8 MXBoundsBox::_Intersect(const vec_xyzw &o, const vec_xyzw &d) const
{
    float dx = m_vctMin.x - o.x;
    if (!(dx > 0.0f) && !(m_vctMin.y - o.y > 0.0f) && !(m_vctMin.z - o.z > 0.0f) &&
        !(0.0f > m_vctMax.x - o.x) && !(0.0f > m_vctMax.y - o.y) && !(0.0f > m_vctMax.z - o.z))
        return 1;
#define SLAB(A, B, C)                                                                        \
    if (!(EX_EPSILON_UNIT > (float)fabs(d.A))) {                                             \
        float inv = 1.0f / d.A;                                                              \
        float t = inv * (m_vctMin.A - o.A);                                                  \
        if (!(0.0f > t) && !(t > 1.0f)) {                                                    \
            float b = d.B * t + o.B;                                                         \
            float c = d.C * t + o.C;                                                         \
            if (!(m_vctMin.C > c) && !(c > m_vctMax.C) && !(m_vctMin.B > b) && !(b > m_vctMax.B)) \
                return 1;                                                                    \
        }                                                                                    \
        t = inv * (m_vctMax.A - o.A);                                                        \
        if (!(0.0f > t) && !(t > 1.0f)) {                                                    \
            float b = d.B * t + o.B;                                                         \
            float c = d.C * t + o.C;                                                         \
            if (!(m_vctMin.C > c) && !(c > m_vctMax.C) && !(m_vctMin.B > b) && !(b > m_vctMax.B)) \
                return 1;                                                                    \
        }                                                                                    \
    }
    SLAB(x, y, z)
    SLAB(y, x, z)
    SLAB(z, x, y)
#undef SLAB
    return 0;
}

// Transform the box by m (Arvo): start from the translation and add, per input axis, the smaller
// and larger of that row scaled by the old min and max.
void MXBoundsBox::_ApplyXForm(const mat_44 &m)
{
    vec_xyzw mn = m_vctMin;
    vec_xyzw mx = m_vctMax;
    vec_xyzw nmin = m.r[3];
    vec_xyzw nmax = m.r[3];
#define AXIS(i, c)                                                       \
    {                                                                    \
        vec_xyzw s0, s1;                                                 \
        s0.set(mn.c, mn.c, mn.c, 0.0f);                                  \
        s1.set(mx.c, mx.c, mx.c, 0.0f);                                  \
        vec_xyzw a = s0 * m.r[i];                                \
        vec_xyzw b = s1 * m.r[i];                                \
        nmin += a.Min(b);                                                \
        nmax += a.Max(b);                                                \
    }
    AXIS(0, x)
    AXIS(1, y)
    AXIS(2, z)
#undef AXIS
    nmin.set_w0();
    nmax.set_w0();
    m_vctMin = nmin;
    m_vctMax = nmax;
}
