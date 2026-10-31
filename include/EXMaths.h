#ifndef EXMATHS_H
#define EXMATHS_H
#include "types.h"
#include "EXStdFuncs.h"

#define EX_EPSILON_EQUAL 0.001f
#define EX_EPSILON_NORMALISE 1e-7f

// Square root that clamps negative input to 0 (fsqrt on the float, x87).
inline float EXSqrt(float f) { return f >= 0.0f ? (float)sqrt(f) : 0.0f; }
inline float qatanf(float f) { return (float)atan(f); }  // [fi] copy in Camera.obj
inline void EXSinCos(float a, float &s, float &c) { s = (float)sin(a); c = (float)cos(a); }
// Clamped inverse trig: out-of-range input saturates to the end of the range.
inline float EXASin(float f)
{
    if (f < -1.0f) return -1.5707964f;
    if (f > 1.0f) return 1.5707964f;
    return (float)asin(f);
}
inline float EXACos(float f)
{
    if (f < -1.0f) return 3.1415927f;
    if (f > 1.0f) return 0.0f;
    return (float)acos(f);
}
inline s8 EXIsEqual(float a, float b) { return (float)fabs(a - b) < EX_EPSILON_EQUAL; }

// D3DX entry points (our own prototypes; D3DX is linked from the XDK libraries).
extern "C" {
void *__stdcall D3DXVec4Transform(void *pOut, const void *pV, const void *pM);
void *__stdcall D3DXMatrixMultiply(void *pOut, const void *pM1, const void *pM2);
}

namespace EXMaths {

// Low-level 4-float vector (POD). Results are built in a local and copied out (no NRVO in VC7.1).
// set(a, b, c, d) evaluates its arguments right to left, which is why the "+=" family loads w first.
class vec_xyzw {
public:
    void set(float x_, float y_, float z_, float w_) { x = x_; y = y_; z = z_; w = w_; }
    void set(const vec_xyzw &v) { set(v.x, v.y, v.z, v.w); }
    void set(float f) { w = f; z = f; y = f; x = f; }
    void set_zero() { w = 0.0f; z = 0.0f; y = 0.0f; x = 0.0f; }
    void set_zero1() { z = 0.0f; y = 0.0f; x = 0.0f; w = 1.0f; }
    void set_x(float f) { x = f; }
    void set_y(float f) { y = f; }
    void set_z(float f) { z = f; }
    void set_w(float f) { w = f; }
    void set_w0() { w = 0.0f; }
    void set_w1() { w = 1.0f; }
    float get_x() const { return x; }
    float get_y() const { return y; }
    float get_z() const { return z; }
    float get_w() const { return w; }
    vec_xyzw get_w0() const
    {
        vec_xyzw r;
        r.x = x; r.y = y; r.z = z; r.w = 0.0f;
        return r;
    }
    vec_xyzw get_w1() const
    {
        vec_xyzw r;
        r.x = x; r.y = y; r.z = z; r.w = 1.0f;
        return r;
    }
    float &operator[](int i) { return (&x)[i]; }
    float operator[](int i) const { return (&x)[i]; }

    vec_xyzw operator+(const vec_xyzw &v) const
    {
        vec_xyzw r;
        r.x = v.x + x; r.y = v.y + y; r.z = v.z + z; r.w = v.w + w;
        return r;
    }
    vec_xyzw operator-(const vec_xyzw &v) const
    {
        vec_xyzw r;
        r.x = x - v.x; r.y = y - v.y; r.z = z - v.z; r.w = w - v.w;
        return r;
    }
    vec_xyzw operator*(const vec_xyzw &v) const
    {
        vec_xyzw r;
        r.x = v.x * x; r.y = v.y * y; r.z = v.z * z; r.w = v.w * w;
        return r;
    }
    vec_xyzw operator*(float f) const
    {
        vec_xyzw r;
        r.x = x * f; r.y = y * f; r.z = z * f; r.w = w * f;
        return r;
    }
    vec_xyzw operator/(float f) const
    {
        float inv = 1.0f / f;
        vec_xyzw r;
        r.x = x * inv; r.y = y * inv; r.z = z * inv; r.w = w * inv;
        return r;
    }
    vec_xyzw operator-() const
    {
        vec_xyzw r;
        r.x = 0.0f - x; r.y = 0.0f - y; r.z = 0.0f - z; r.w = 0.0f - w;
        return r;
    }
    void operator+=(const vec_xyzw &v) { set(v.x + x, v.y + y, v.z + z, v.w + w); }
    void operator-=(const vec_xyzw &v) { set(x - v.x, y - v.y, z - v.z, w - v.w); }
    void operator*=(const vec_xyzw &v) { set(v.x * x, v.y * y, v.z * z, v.w * w); }
    void operator*=(float f) { x *= f; y *= f; z *= f; w *= f; }
    void operator/=(float f)
    {
        float inv = 1.0f / f;
        x *= inv; y *= inv; z *= inv; w *= inv;
    }
    s8 operator==(const vec_xyzw &v) const
    {
        if (EXIsEqual(x, v.x) && EXIsEqual(y, v.y) && EXIsEqual(z, v.z) && EXIsEqual(w, v.w)) return 1;
        return 0;
    }
    s8 operator!=(const vec_xyzw &v) const
    {
        if (EXIsEqual(x, v.x) && EXIsEqual(y, v.y) && EXIsEqual(z, v.z) && EXIsEqual(w, v.w)) return 0;
        return 1;
    }

    float Dot(const vec_xyzw &v) const { return v.w * w + v.z * z + v.y * y + v.x * x; }
    float DotXYZ(const vec_xyzw &v) const { return v.z * z + v.y * y + v.x * x; }
    vec_xyzw Cross(const vec_xyzw &v) const
    {
        vec_xyzw r;
        r.x = y * v.z - v.y * z;
        r.y = v.x * z - x * v.z;
        r.z = v.y * x - y * v.x;
        r.w = 0.0f;
        return r;
    }
    float LengthSquared() const { return x * x + y * y + z * z + w * w; }
    float Length() const { return EXSqrt(x * x + y * y + z * z + w * w); }
    float LengthXYZSquared() const { return x * x + y * y + z * z; }
    float LengthXYZ() const { return EXSqrt(x * x + y * y + z * z); }
    vec_xyzw &Normalise()
    {
        float len = Length();
        if (len > EX_EPSILON_NORMALISE) *this *= 1.0f / len;
        return *this;
    }
    vec_xyzw Interp(const vec_xyzw &v, float t) const
    {
        vec_xyzw r;
        r.x = (v.x - x) * t + x;
        r.y = (v.y - y) * t + y;
        r.z = (v.z - z) * t + z;
        r.w = (v.w - w) * t + w;
        return r;
    }
    vec_xyzw Min(const vec_xyzw &v) const
    {
        vec_xyzw r;
        r.set(v.x > x ? x : v.x, v.y > y ? y : v.y, v.z > z ? z : v.z, v.w > w ? w : v.w);
        return r;
    }
    vec_xyzw Max(const vec_xyzw &v) const
    {
        vec_xyzw r;
        r.set(v.x < x ? x : v.x, v.y < y ? y : v.y, v.z < z ? z : v.z, v.w < w ? w : v.w);
        return r;
    }
    u32 ClipFlag() const
    {
        float f = (float)fabs(w);
        float nf = 0.0f - f;
        u8 flags = 0;
        if (nf > x) flags = 2;
        else if (x > f) flags = 1;
        if (nf > y) flags |= 8;
        else if (y > f) flags |= 4;
        if (nf > z) flags |= 0x20;
        else if (z > f) flags |= 0x10;
        return flags;
    }
    signed char PointInTri(const vec_xyzw *const tri, const vec_xyzw &n) const;
    signed char PointInTriY(const vec_xyzw *const tri) const;
    void Print(const char *name) const;
    void Trace(const char *name) const;

    float x, y, z, w;
};

// 4x4 matrix as four row vectors (row-vector convention, translation in r[3]).
class mat_44 {
public:
    void set_identity()
    {
        r[0].x = 1.0f; r[0].y = 0.0f; r[0].z = 0.0f; r[0].w = 0.0f;
        r[1].x = 0.0f; r[1].y = 1.0f; r[1].z = 0.0f; r[1].w = 0.0f;
        r[2].x = 0.0f; r[2].y = 0.0f; r[2].z = 1.0f; r[2].w = 0.0f;
        r[3].x = 0.0f; r[3].y = 0.0f; r[3].z = 0.0f; r[3].w = 1.0f;
    }
    void set_zero() { r[0].set_zero(); r[1].set_zero(); r[2].set_zero(); r[3].set_zero(); }
    void set_scale(const vec_xyzw &s)
    {
        r[0].set(s.x, 0.0f, 0.0f, 0.0f);
        r[1].set(0.0f, s.y, 0.0f, 0.0f);
        r[2].set(0.0f, 0.0f, s.z, 0.0f);
        r[3].set(0.0f, 0.0f, 0.0f, s.w);
    }
    void set_rotate_x(float a)
    {
        double sd = sin(a);  // via a double: schedules the negation before the cos (orig x/y rotations)
        float s = (float)sd;
        float ms = -s;
        float c = (float)cos(a);
        r[0].set(1.0f, 0.0f, 0.0f, 0.0f);
        r[1].set(0.0f, c, s, 0.0f);
        r[2].set(0.0f, ms, c, 0.0f);
        r[3].set(0.0f, 0.0f, 0.0f, 1.0f);
    }
    void set_rotate_y(float a)
    {
        double sd = sin(a);  // via a double: schedules the negation before the cos (orig x/y rotations)
        float s = (float)sd;
        float ms = -s;
        float c = (float)cos(a);
        r[0].set(c, 0.0f, ms, 0.0f);
        r[1].set(0.0f, 1.0f, 0.0f, 0.0f);
        r[2].set(s, 0.0f, c, 0.0f);
        r[3].set(0.0f, 0.0f, 0.0f, 1.0f);
    }
    void set_rotate_z(float a)
    {
        float s = (float)sin(a);
        float c = (float)cos(a);
        r[0].set(c, s, 0.0f, 0.0f);
        r[1].set(-s, c, 0.0f, 0.0f);
        r[2].set(0.0f, 0.0f, 1.0f, 0.0f);
        r[3].set(0.0f, 0.0f, 0.0f, 1.0f);
    }
    __forceinline void set_rotate_xyz(const vec_xyzw &a)
    {
        float sx, cx, sy, cy, sz, cz;
        EXSinCos(a.x, sx, cx);
        EXSinCos(a.y, sy, cy);
        EXSinCos(a.z, sz, cz);
        r[0].x = cz * cy; r[0].y = cz * sy * sx + sz * cx; r[0].z = sz * sx - cz * sy * cx; r[0].w = 0.0f;
        r[1].x = -(sz * cy); r[1].y = cz * cx - sz * sy * sx; r[1].z = sz * sy * cx + cz * sx; r[1].w = 0.0f;
        r[2].x = sy; r[2].y = -(cy * sx); r[2].z = cy * cx; r[2].w = 0.0f;
        r[3].x = 0.0f; r[3].y = 0.0f; r[3].z = 0.0f; r[3].w = 1.0f;
    }
    void set_rotate_xzy(const vec_xyzw &a);
    void set_rotate_yxz(const vec_xyzw &a);
    void set_rotate_yzx(const vec_xyzw &a);
    // Inlined in XBParticle.obj (EXMatrix::SetRotationZXY copy).
    __forceinline void set_rotate_zxy(const vec_xyzw &a)
    {
        float sx, cx, sy, cy, sz, cz;
        EXSinCos(a.x, sx, cx);
        EXSinCos(a.y, sy, cy);
        EXSinCos(a.z, sz, cz);
        r[0].set(cz * cy - sz * sy * sx, cz * sy * sx + sz * cy, -(sy * cx), 0.0f);
        r[1].set(-(sz * cx), cz * cx, sx, 0.0f);
        r[2].set(sz * cy * sx + cz * sy, sz * sy - cz * cy * sx, cy * cx, 0.0f);
        r[3].set(0.0f, 0.0f, 0.0f, 1.0f);
    }
    void set_rotate_zyx(const vec_xyzw &a);
    void set_rotate_quat(const vec_xyzw &q);
    void set_rotate_axis(const vec_xyzw &aa);
    void set_rotate(const vec_xyzw &a, u8 order);
    void set_row0(const vec_xyzw &v) { r[0] = v; }
    void set_row1(const vec_xyzw &v) { r[1] = v; }
    void set_row2(const vec_xyzw &v) { r[2] = v; }
    void set_row3(const vec_xyzw &v) { r[3] = v; }
    vec_xyzw get_row0() const { return r[0]; }
    vec_xyzw get_row1() const { return r[1]; }
    vec_xyzw get_row2() const { return r[2]; }
    vec_xyzw get_row3() const { return r[3]; }
    vec_xyzw operator*(const vec_xyzw &v) const
    {
        vec_xyzw out;
        D3DXVec4Transform(&out, &v, this);
        return out;
    }
    mat_44 operator*(const mat_44 &m) const
    {
        mat_44 out;
        D3DXMatrixMultiply(&out, &m, this);
        return out;
    }
    mat_44 transpose() const;
    mat_44 transpose3() const;
    mat_44 inverse() const;
    mat_44 inverse3() const;
    mat_44 inverse3trans() const;
    mat_44 fastinverse() const;
    void Print(const char *name) const;
    void Trace(const char *name) const;

    vec_xyzw r[4];
};

// Quaternion (x, y, z, w) on top of vec_xyzw.
class quat_xyzw : public vec_xyzw {
public:
    // XBQuaternion.obj [fi] copy with no callers: forced inline.
    __forceinline quat_xyzw operator*(float f) const
    {
        quat_xyzw r;
        r.x = x * f; r.y = y * f; r.z = z * f; r.w = w * f;
        return r;
    }
    // Quaternion products ([fi] copies in Player.obj, shapes from them).
    __forceinline quat_xyzw &operator*=(const quat_xyzw &q)
    {
        vec_xyzw a = *this;
        x = a.y * q.z + a.x * q.w + a.w * q.x - q.y * a.z;
        y = a.y * q.w + a.w * q.y + a.z * q.x - q.z * a.x;
        z = a.w * q.z + a.z * q.w + q.y * a.x - q.x * a.y;
        w = a.w * q.w - q.x * a.x - q.y * a.y - q.z * a.z;
        return *this;
    }
    __forceinline quat_xyzw operator*(const quat_xyzw &q) const
    {
        quat_xyzw r;
        r.x = w * q.x + q.w * x + q.z * y - q.y * z;
        r.y = q.w * y + q.y * w + z * q.x - x * q.z;
        r.z = z * q.w + w * q.z + q.y * x - q.x * y;
        r.w = q.w * w - q.x * x - q.y * y - z * q.z;
        return r;
    }
    // Emitted as a [fi] copy in EXQuaternion.obj with no callers: forced inline.
    __forceinline quat_xyzw &operator*=(float f)
    {
        set(vec_xyzw(*this) * f);
        return *this;
    }
    __forceinline void set_rotate_axis(const vec_xyzw &aa)
    {
        float half = aa.w * 0.5f;
        vec_xyzw v = aa * (float)sin(half);
        set(v.x, v.y, v.z, (float)cos(half));
    }
    // Euler-angle builders (XBQuaternion.obj [fi] copies, all inlined into SetRotation): forced inline.
#define EX_QUAT_ROT(name, SY, SX, SZ, SW)                                              \
    __forceinline void name(const vec_xyzw &a)                                         \
    {                                                                                  \
        float sx, cx, sy, cy, sz, cz;                                                  \
        EXSinCos(a.x * 0.5f, sx, cx);                                                  \
        EXSinCos(a.y * 0.5f, sy, cy);                                                  \
        EXSinCos(a.z * 0.5f, sz, cz);                                                  \
        float sycx = sy * cx, sysx = sy * sx;                                          \
        float cysx = cy * sx, cycx = cy * cx;                                          \
        x = sycx * sz SX cysx * cz;                                                    \
        y = sycx * cz SY cysx * sz;                                                    \
        z = cycx * sz SZ sysx * cz;                                                    \
        w = cycx * cz SW sysx * sz;                                                    \
    }
    EX_QUAT_ROT(set_rotate_xyz, -, +, +, -)
    EX_QUAT_ROT(set_rotate_xzy, -, -, +, +)
    EX_QUAT_ROT(set_rotate_yxz, -, +, -, +)
    EX_QUAT_ROT(set_rotate_yzx, +, +, -, -)
    EX_QUAT_ROT(set_rotate_zxy, +, -, +, -)
    EX_QUAT_ROT(set_rotate_zyx, +, -, -, +)
#undef EX_QUAT_ROT
    void SetRotation(const vec_xyzw &a, u8 order);
    quat_xyzw slerp(const quat_xyzw &q, float t) const;
    quat_xyzw qslerp(const quat_xyzw &q, float t) const;
    vec_xyzw MultiplyVector(const vec_xyzw &v) const;
};

// [fi] copies in VehiclePos.obj (verified by the collision agent).
inline vec_xyzw Min(const vec_xyzw &a, const vec_xyzw &b)
{
    vec_xyzw r;
    r.set(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z, a.w < b.w ? a.w : b.w);
    return r;
}
inline vec_xyzw Max(const vec_xyzw &a, const vec_xyzw &b)
{
    vec_xyzw r;
    r.set(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z, a.w > b.w ? a.w : b.w);
    return r;
}

} // namespace EXMaths

// Square ([fi] in EXGeoMap.obj).
template <class T> inline T Sqr(T x) { return x * x; }
#endif
