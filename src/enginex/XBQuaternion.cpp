#include "EXMaths.h"

namespace EXMaths {

// Rotation orders: 0 quaternion, 1 angle-axis, 2 XYZ, 3 XZY, 4 YXZ, 5 YZX, 6 ZXY, 7 ZYX.
void quat_xyzw::SetRotation(const vec_xyzw &a, u8 order)
{
    switch (order) {
    case 1: set_rotate_axis(a); break;
    case 2: set_rotate_xyz(a); break;
    case 3: set_rotate_xzy(a); break;
    case 4: set_rotate_yxz(a); break;
    case 5: set_rotate_yzx(a); break;
    case 6: set_rotate_zxy(a); break;
    case 7: set_rotate_zyx(a); break;
    default: set(a); break;
    }
}

} // namespace EXMaths


namespace EXMaths {

// NEAR: ~98 bytes differ, the original moves the "neg = 0" branch out of line and swaps the two
// product temporaries' stack slots.
quat_xyzw quat_xyzw::slerp(const quat_xyzw &q, float t) const
{
    float cosom = Dot(q);
    int neg;
    if (cosom < 0.0f) {
        neg = 1;
        cosom = 0.0f - cosom;
    } else {
        neg = 0;
    }
    float sinom = EXSqrt((float)fabs(1.0f - cosom * cosom));
    float s0, s1;
    if (1.0f - cosom > 0.01f) {
        float omega = (float)atan2(sinom, cosom);
        float inv = sinom != 0.0f ? 1.0f / sinom : 0.0f;
        s0 = (float)sin(omega - omega * t) * inv;
        s1 = (float)sin(omega * t) * inv;
    } else {
        s0 = 1.0f - t;
        s1 = t;
    }
    if (neg) s0 = 0.0f - s0;
    quat_xyzw r;
    r.set(*this * s0 + q * s1);
    return r;
}

} // namespace EXMaths

namespace EXMaths {

// Approximate slerp: a cubic correction of the interpolation factor from the cosine between the two.
quat_xyzw quat_xyzw::qslerp(const quat_xyzw &q, float t) const
{
    float d = Dot(q);
    float k = (d * 0.31462f - 0.798723f) * d + 0.506927f;
    quat_xyzw r;
    if (t > 0.5f) {
        float u = 1.0f - t;
        float f = (((u * 2.0f - 3.0f) * u + 1.0f) * k + 1.0f) * u;
        r.set(q.Interp(*this, f));
    } else {
        float f = (((t * 2.0f - 3.0f) * t + 1.0f) * k + 1.0f) * t;
        r.set(Interp(q, f));
    }
    return r;
}

} // namespace EXMaths

namespace EXMaths {

// NEAR: ~208 bytes differ, same math; the original evaluates qy*2v.z before 2v.y*qz (register order).
vec_xyzw quat_xyzw::MultiplyVector(const vec_xyzw &v) const
{
    float vx = v.x * 2.0f;
    float vy = v.y * 2.0f;
    float vz = v.z * 2.0f;
    float tx = y * vz - vy * z;
    float ty = vx * z - x * vz;
    float tz = x * vy - y * vx;
    vec_xyzw r;
    r.x = y * tz - ty * z + w * tx + v.x;
    r.y = tx * z - x * tz + w * ty + v.y;
    r.z = x * ty - y * tx + w * tz + v.z;
    r.w = 0.0f;
    return r;
}

} // namespace EXMaths
