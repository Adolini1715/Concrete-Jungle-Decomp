#include "EXQuaternion.h"
#include "EXArray.h"

using namespace EXMaths;

// NEAR: 8 bytes differ, the zero path loads a double 0.0 where the original stores the float 0 and fsqrt's it.
__forceinline float qrecipsqrtf(float f)
{
    if (f != 0.0f) return EXSqrt(1.0f / f);
    return EXSqrt(0.0f);
}

void EXQuaternion::SetRotation(const EXAngleAxis &aa)
{
    float half = aa.w * 0.5f;
    vec_xyzw v = aa * (float)sin(half);
    set(v.x, v.y, v.z, (float)cos(half));
}

void EXQuaternion::SetRotation(const EXMatrix &m)
{
    float tr = m.r[0][0] + m.r[1][1] + m.r[2][2];
    if (tr > 0.0f) {
        float s = tr + 1.0f;
        float sq = EXSqrt(s);
        float f = 0.5f / sq;
        Set(m.r[1][2] - m.r[2][1], m.r[2][0] - m.r[0][2], m.r[0][1] - m.r[1][0], s);
        *this *= f;
    } else {
        u32 i;
        if (m.r[0].x >= m.r[1].y) {
            if (m.r[0].x >= m.r[2].z) i = 0;
            else i = 2;
        } else {
            if (m.r[1].y >= m.r[2].z) i = 1;
            else i = 2;
        }
        u32 j = ((i * 11 + 11) >> 3) & 3;
        u32 k = ((i * 11 - 10) >> 3) & 3;
        float s = m.r[i][i] - m.r[j][j] - m.r[k][k] + 1.0f;
        float sq = EXSqrt(s);
        float f;
        if (sq != 0.0f) f = 0.5f / sq;
        else f = 0.0f;
        EXFixedArray<float, 3> q;
        q[i] = s;
        q[j] = m.r[j][i] + m.r[i][j];
        q[k] = m.r[k][i] + m.r[i][k];
        float ww = m.r[j][k] - m.r[k][j];
        set(q[0u], q[1u], q[2u], ww);
        if (ww < 0.0f) f = -f;
        *this *= f;
    }
}

void EXQuaternion::FromSpherical(float a, float b, float c)
{
    float cb = (float)cos(b);
    float cc = (float)cos(c);
    x = (float)cos(a) * cc * cb;
    y = (float)sin(a) * cc * cb;
    z = (float)sin(b) * cc;
    w = (float)sin(c);
}

// NEAR: ~120 bytes differ, the original routes the result through two extra temporaries.
EXQuaternion EXQuaternion::Inverse() const
{
    float len2 = LengthSquared();
    if (len2 > 0.0f) {
        float inv = 1.0f / len2;
        float ninv = 0.0f - inv;
        vec_xyzw r;
        r.set(x * ninv, y * ninv, z * ninv, w * inv);
        return EXQuaternion(r.x, r.y, r.z, r.w);
    }
    return EXQuaternion(0.0f, 0.0f, 0.0f, 1.0f);
}

// NEAR: 3 bytes short, the original gives the acos result its own stack slot (push ecx frame).
float EXQuaternion::AngleDiff(const EXQuaternion &q) const
{
    float d = Dot(q);
    float res;
    if ((float)fabs(d) > 1.0f) res = 0.0f;
    else res = EXACos(d) * 2.0f;
    return res;
}

// NEAR: ~200 bytes differ, the original compares the AngleDiff result with SSE (we use fcompi) and
// normalises in w, z, x, y order.
EXQuaternion EXQuaternion::SlerpBy(const EXQuaternion &q, float t, float &out) const
{
    out = AngleDiff(q);
    float d = out;
    float r = (d > 0.0f ? 1.0f / d : 1.0f) * t;
    vec_xyzw v = slerp(q, limit(r, 0.0f, 1.0f));
    float len = EXSqrt(v.w * v.w + v.z * v.z + v.y * v.y + v.x * v.x);
    if (len > EX_EPSILON_NORMALISE) v *= 1.0f / len;
    return EXQuaternion(v);
}

// NEAR: ~265 bytes differ, qrecipsqrtf's zero path and the clamp's register reuse of w.
void EXQuaternion::GetRotation(EXAngleAxis &aa) const
{
    vec_xyzw v = *this;
    v.w = 0.0f;
    float len2 = v.LengthXYZSquared();
    if (len2 > 1.4210855e-14f) {
        float f = qrecipsqrtf(len2);
        aa.set(v * f);
        float c = w;
        aa.w = EXACos(limit(c, -1.0f, 1.0f)) * 2.0f;
    } else {
        aa.set(1.0f, 0.0f, 0.0f, 0.0f);
    }
}

// NEAR: ~390 bytes differ, same math; register assignment of 2q (the original loads 2q.z late).
void EXQuaternion::GetRotation(EXMatrix &m) const
{
    vec_xyzw v2 = vec_xyzw(*this) * 2.0f;
    v2.w = 0.0f;
    vec_xyzw wv = v2 * w;
    vec_xyzw xv = v2 * x;
    vec_xyzw yv = v2 * y;
    float zz = z * v2.z;
    m.r[0].set(1.0f - (yv.y + zz), xv.y + wv.z, xv.z - wv.y, 0.0f);
    m.r[1].set(xv.y - wv.z, 1.0f - (xv.x + zz), yv.z + wv.x, 0.0f);
    m.r[2].set(xv.z + wv.y, yv.z - wv.x, 1.0f - (xv.x + yv.y), 0.0f);
    m.r[3].set_zero1();
}

void EXQuaternion::GetRotation(EXVector &a, u8 order) const
{
    EXMatrix m;
    GetRotation(m);
    m.GetRotation(a, order);
}

void EXAngleAxis::SetRotation(const EXVector &a, u8 order)
{
    EXQuaternion q;
    q.quat_xyzw::SetRotation(a, order);
    q.GetRotation(*this);
}

void EXAngleAxis::SetRotation(const EXQuaternion &q)
{
    q.GetRotation(*this);
}

// NEAR: 12 bytes differ, the original does not reload ecx (&q) before the second call.
void EXAngleAxis::SetRotation(const EXMatrix &m)
{
    EXQuaternion q;
    q.SetRotation(m);
    q.GetRotation(*this);
}

