#include "EXPlane.h"

using namespace EXMaths;

float EXPlane::m_rCalcTol;

// NEAR: ~320 bytes differ, the original keeps one more EXVector temporary copy of the cross product.
EXPlane::EXPlane(const EXVector &a, const EXVector &b, const EXVector &c)
{
    EXVector n = (b - a).Cross(c - b);
    A = n.x; B = n.y; C = n.z; D = n.w;
    float len = EXSqrt(A * A + B * B + C * C);
    if ((float)fabs(len) != 0.0f) {
        float inv = 1.0f / len;
        A = inv * A; B *= inv; C *= inv; D *= inv;
        D = 0.0f - (A * a.x + B * a.y + C * a.z);
    } else {
        A = 1.0f; B = 0.0f; C = 0.0f; D = 0.0f;
    }
}

EXPlane::EXPlane(const EXVector &p, const EXVector &n)
{
    float len = n.Length();
    if ((float)fabs(len) != 0.0f) {
        EXVector v = n * (1.0f / len);
        A = v.x; B = v.y; C = v.z; D = v.w;
        D = 0.0f - (p.x * A + p.y * B + p.z * C);
    } else {
        A = 1.0f; B = 0.0f; C = 0.0f; D = 0.0f;
    }
}

s8 EXPlane::Intersection(float &t, const EXLineSeg &l) const
{
    const EXVector &d = l.m_Direction;
    float denom = d.x * A + d.y * B + d.z * C + d.w * D;
    if ((float)fabs(denom) < 1e-10f) return 0;
    const EXVector &o = l.m_Origin;
    t = 0.0f - (o.x * A + o.y * B + o.z * C + o.w * D) / denom;
    return 1;
}
