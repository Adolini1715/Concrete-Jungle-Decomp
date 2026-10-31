#include "EXVector.h"
#include "EXTriangle.h"
#include "EXBounds.h"
#include <math.h>

using namespace EXMaths;

// NEAR: ~250 bytes differ, the second Normalise loads u in y, z, w, x order where the original
// keeps x, y, z, w (register assignment), same as the first.
void EXVector::GenerateOrthonormalBasis(EXVector &u, EXVector &v, EXVector &w, s8 bNormalised)
{
    if (!bNormalised) w.Normalise();
    if ((float)fabs(w.x) >= (float)fabs(w.y) && (float)fabs(w.x) >= (float)fabs(w.z)) {
        u.x = -w.y;
        u.y = w.x;
        u.z = 0.0f;
    } else {
        u.x = 0.0f;
        u.y = w.z;
        u.z = -w.y;
    }
    u.Normalise();
    v = w.Cross(u);
}

// Only the first vertex gets the w = 1 / w = 0 point treatment; the others transform with their own w.
void EXTriangle::ApplyXForm(const EXMatrix &m)
{
    ((EXVector &)m_v[0]).ApplyXForm(m);
    (EXVector &)m_v[1] *= m;
    (EXVector &)m_v[2] *= m;
}

// Bounds of the three corners (origin, origin + edge 0, origin + edge 1), all four components.
void EXTriangle::GetBoundsBox(EXBoundsBox &b) const
{
    const float *o = &m_v[0].x;
    const float *e0 = &m_v[1].x;
    const float *e1 = &m_v[2].x;
    float mn[4], mx[4];
    for (int i = 0; i < 4; i++) mn[i] = mx[i] = o[i];
    for (int k = 0; k < 2; k++) {
        const float *e = k ? e1 : e0;
        for (int i = 0; i < 4; i++) {
            float v = o[i] + e[i];
            if (v <= mn[i]) mn[i] = v;
            if (v >= mx[i]) mx[i] = v;
        }
    }
    b.m_vctMin.set(mn[0], mn[1], mn[2], mn[3]);
    b.m_vctMax.set(mx[0], mx[1], mx[2], mx[3]);
}

// Closest point on the triangle to p: returns the squared distance and the edge parameters (s, t)
// of the closest point origin + s*edge0 + t*edge1 (region analysis of the quadratic in s, t).
float EXTriangle::MinPointSquared(const EXVector &p, float &outS, float &outT) const
{
    const float *o = &m_v[0].x;
    const float *e0 = &m_v[1].x;
    const float *e1 = &m_v[2].x;
    float dx = o[0] - p.x, dy = o[1] - p.y, dz = o[2] - p.z;
    float a = e0[2] * e0[2] + e0[1] * e0[1] + e0[0] * e0[0];
    float b = e0[2] * e1[2] + e0[1] * e1[1] + e0[0] * e1[0];
    float c = e1[2] * e1[2] + e1[1] * e1[1] + e1[0] * e1[0];
    float det = c * a - b * b;
    if (det <= 1.1920929e-7f) {
        outS = 0.0f;
        outT = 0.0f;
        return 1.8446743e19f;
    }
    float d = e0[1] * dy + e0[2] * dz + e0[0] * dx;
    float e = e1[1] * dy + e1[2] * dz + e1[0] * dx;
    float inv = 1.0f / det;
    float s = (e * b - d * c) * inv;
    float t = (d * b - e * a) * inv;
    float denom = a - 2.0f * b + c;
    if (s + t <= 1.0f) {
        if (s < 0.0f) {
            if (t < 0.0f) {
                if (d < 0.0f) {
                    t = 0.0f;
                    s = (-d >= a) ? 1.0f : -d / a;
                } else {
                    s = 0.0f;
                    t = (e >= 0.0f) ? 0.0f : ((-e >= c) ? 1.0f : -e / c);
                }
            } else {
                s = 0.0f;
                t = (e >= 0.0f) ? 0.0f : ((-e >= c) ? 1.0f : -e / c);
            }
        } else if (t < 0.0f) {
            t = 0.0f;
            s = (d >= 0.0f) ? 0.0f : ((-d >= a) ? 1.0f : -d / a);
        }
    } else if (s < 0.0f) {
        float t0 = b + d, t1 = c + e;
        if (t1 > t0) {
            float n = t1 - t0;
            s = (n >= denom) ? 1.0f : n / denom;
            t = 1.0f - s;
        } else {
            s = 0.0f;
            t = (t1 <= 0.0f) ? 1.0f : ((e >= 0.0f) ? 0.0f : -e / c);
        }
    } else if (t < 0.0f) {
        float t0 = b + e, t1 = a + d;
        if (t1 > t0) {
            float n = t1 - t0;
            t = (n >= denom) ? 1.0f : n / denom;
            s = 1.0f - t;
        } else {
            t = 0.0f;
            s = (t1 <= 0.0f) ? 1.0f : ((d >= 0.0f) ? 0.0f : -d / a);
        }
    } else {
        float n = (c + e) - (b + d);
        if (n <= 0.0f) s = 0.0f;
        else s = (n >= denom) ? 1.0f : n / denom;
        t = 1.0f - s;
    }
    outS = s;
    outT = t;
    return fabsf((2.0f * d + a * s + b * t) * s + (2.0f * e + b * s + c * t) * t + (dy * dy + dz * dz + dx * dx));
}
