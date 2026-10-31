#ifndef EXVECTOR_H
#define EXVECTOR_H
#include "types.h"
#include "EXMaths.h"

class EXMatrix;
#define EX_EPSILON_UNIT 1e-5f

// 3-float vector (no w). Copy ctor/assignment are the implicit ones (dword copies).
class EXVector3 {
public:
    EXVector3() {}
    EXVector3(float x_, float y_, float z_) { x = x_; y = y_; z = z_; }
    void Set(float x_, float y_, float z_) { x = x_; y = y_; z = z_; }
    EXVector3 &operator+=(const EXVector3 &v) { x = v.x + x; y = v.y + y; z = v.z + z; return *this; }
    EXVector3 &operator-=(const EXVector3 &v) { x = x - v.x; y = y - v.y; z = z - v.z; return *this; }
    float Length() const { return EXSqrt(x * x + y * y + z * z); }
    float Normalise()
    {
        float len = Length();
        if (len > EX_EPSILON_UNIT) {
            float inv = 1.0f / len;
            x *= inv; y *= inv; z *= inv;
        }
        return len;
    }
    // NEAR (VehicleMove.obj inline): 65 bytes differ, register allocation / sum order.
    EXVector3 UnitVector() const
    {
        float xx = x, yy = y, zz = z;
        float len = EXSqrt(xx * xx + yy * yy + zz * zz);
        if (len > EX_EPSILON_UNIT) {
            float inv = 1.0f / len;
            xx *= inv; yy *= inv; zz *= inv;
        }
        return EXVector3(xx, yy, zz);
    }
    float x, y, z;
};
// Binary EXVector3 operators ([fi] copies in Camera.obj).
inline EXVector3 operator-(const EXVector3 &a, const EXVector3 &b) { return EXVector3(a.x - b.x, a.y - b.y, a.z - b.z); }
inline EXVector3 operator*(const EXVector3 &a, float f) { return EXVector3(a.x * f, a.y * f, a.z * f); }

class EXVector2 {
public:
    EXVector2() {}
    EXVector2(float x_, float y_) { x = x_; y = y_; }
    EXVector2(const EXVector2 &v) { x = v.x; y = v.y; }
    EXVector2 &operator=(const EXVector2 &v) { x = v.x; y = v.y; return *this; }
    void Set(float x_, float y_) { x = x_; y = y_; }
    void SetZero() { y = 0.0f; x = 0.0f; }
    EXVector2 &operator+=(const EXVector2 &v) { x = v.x + x; y = v.y + y; return *this; }
    EXVector2 &operator-=(const EXVector2 &v) { x = x - v.x; y = y - v.y; return *this; }
    EXVector2 &operator*=(float f) { x *= f; y *= f; return *this; }
    float LengthSquared() const { return x * x + y * y; }
    float Length() const { return EXSqrt(x * x + y * y); }
    float Normalise()
    {
        float len = Length();
        if (len > EX_EPSILON_UNIT) {
            float inv = 1.0f / len;
            x *= inv; y *= inv;
        }
        return len;
    }
    float x, y;
};

// Binary EXVector2 operators (emitted in EXItemCtrl.obj, FEHud.obj, FEObject.obj).
inline EXVector2 operator+(const EXVector2 &a, const EXVector2 &b) { return EXVector2(a.x + b.x, a.y + b.y); }
inline EXVector2 operator-(const EXVector2 &a, const EXVector2 &b) { return EXVector2(a.x - b.x, a.y - b.y); }
inline EXVector2 operator*(const EXVector2 &a, float f) { return EXVector2(a.x * f, a.y * f); }
// Component-wise min/max (emitted in EXItemEnv.obj).
inline EXVector2 Min(const EXVector2 &a, const EXVector2 &b) { return EXVector2(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y); }
inline EXVector2 Max(const EXVector2 &a, const EXVector2 &b) { return EXVector2(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y); }

// The engine's general vector: a vec_xyzw with the EXVector API on top.
class EXVector : public EXMaths::vec_xyzw {
public:
    EXVector() {}
    EXVector(float x_, float y_, float z_, float w_) { x = x_; y = y_; z = z_; w = w_; }
    EXVector(const EXVector &v) { set(v); }
    EXVector(const EXMaths::vec_xyzw &v) { set(v); }
    EXVector(const EXVector3 &v, float w_) { Set(v, w_); }
    EXVector &operator=(const EXVector &v) { set(v); return *this; }
    operator const EXMaths::vec_xyzw() const { return *this; }

    void Set(float x_, float y_, float z_, float w_) { x = x_; y = y_; z = z_; w = w_; }
    void Set(const EXVector3 &v, float w_) { set(v.x, v.y, v.z, w_); }
    void SetZero() { set_zero(); }
    void set_x(float f) { x = f; }
    void set_y(float f) { y = f; }
    void set_z(float f) { z = f; }
    void set_w(float f) { w = f; }
    float get_x() const { return x; }
    float get_y() const { return y; }
    float get_z() const { return z; }
    float get_w() const { return w; }
    float &operator[](int i) { return (&x)[i]; }
    float operator[](int i) const { return (&x)[i]; }

    EXVector &operator+=(const EXVector &v) { vec_xyzw::operator+=(v); return *this; }
    EXVector &operator-=(const EXVector &v) { vec_xyzw::operator-=(v); return *this; }
    EXVector &operator*=(const EXVector &v) { vec_xyzw::operator*=(v); return *this; }
    EXVector &operator*=(float f) { vec_xyzw::operator*=(f); return *this; }
    EXVector &operator/=(float f) { vec_xyzw::operator/=(f); return *this; }
    EXVector &operator*=(const EXMatrix &m);
    EXVector &ApplyXForm(const EXMatrix &m);
    EXVector operator-() const { return EXVector(vec_xyzw::operator-()); }
    // Not in the map: never emitted out of line, so forced inline (VC otherwise gives up on a second use).
    __forceinline EXVector operator+(const EXVector &v) const { return EXVector(vec_xyzw::operator+(v)); }
    __forceinline EXVector operator-(const EXVector &v) const { return EXVector(vec_xyzw::operator-(v)); }
    __forceinline EXVector operator*(float f) const { return EXVector(vec_xyzw::operator*(f)); }

    float Dot(const EXVector &v) const { return vec_xyzw::Dot(v); }
    float DotXYZ(const EXVector &v) const { return vec_xyzw::DotXYZ(v); }
    EXVector Cross(const EXVector &v) const { return EXVector(vec_xyzw::Cross(v)); }
    float Length() const { return vec_xyzw::Length(); }
    float LengthSquared() const { return vec_xyzw::LengthSquared(); }
    float LengthXYZ() const { return vec_xyzw::LengthXYZ(); }
    float LengthXYZSquared() const { return vec_xyzw::LengthXYZSquared(); }
    float LengthXZ() const
    {
        EXMaths::vec_xyzw v = *this;
        return EXSqrt(v.x * v.x + v.z * v.z);
    }
    float LengthXZSquared() const
    {
        EXMaths::vec_xyzw v = *this;
        return v.x * v.x + v.z * v.z;
    }
    float LengthXY() const
    {
        EXMaths::vec_xyzw v = *this;
        return EXSqrt(v.x * v.x + v.y * v.y);
    }
    EXVector UnitVector() const
    {
        EXMaths::vec_xyzw v = *this;
        float len = v.Length();
        if (len > EX_EPSILON_NORMALISE) {
            float inv = 1.0f / len;
            v.x = inv * v.x; v.y = inv * v.y; v.z = inv * v.z; v.w = inv * v.w;
        }
        return EXVector(v);
    }
    EXVector UnitVectorXYZ() const
    {
        EXMaths::vec_xyzw v = *this;
        v.set_w0();
        float len = v.Length();
        if (len > EX_EPSILON_NORMALISE) {
            float inv = 1.0f / len;
            v.x = inv * v.x; v.y = inv * v.y; v.z = inv * v.z; v.w = inv * v.w;
        }
        return EXVector(v);
    }
    float Normalise()
    {
        float len = Length();
        if (len > EX_EPSILON_UNIT) *this *= 1.0f / len;
        return len;
    }
    float NormaliseXYZ()
    {
        float len = LengthXYZ();
        if (len > EX_EPSILON_UNIT) {
            float inv = 1.0f / len;
            x *= inv; y *= inv; z *= inv;
        }
        return len;
    }
    u32 ClipFlag() const { return vec_xyzw::ClipFlag(); }
    void Trace(const char *name) const { vec_xyzw::Trace(name); }

    static void GenerateOrthonormalBasis(EXVector &a, EXVector &b, EXVector &c, s8 bNormalise);
};

// Component-wise product (copy emitted in EXPhysics6D.obj).
inline EXVector operator*(const EXVector &a, const EXVector &b) { return EXVector(a.EXMaths::vec_xyzw::operator*(b)); }

// Emitted in FEObject.obj (0xf9520): the member comparison, cdecl.
inline s8 operator==(const EXVector &a, const EXVector &b)
{
    if (EXIsEqual(a.x, b.x) && EXIsEqual(a.y, b.y) && EXIsEqual(a.z, b.z) && EXIsEqual(a.w, b.w)) return 1;
    return 0;
}
// Lead targeting (EXMaths.obj): where to aim from 'from' with projectile speed 'speed' to hit a target at
// 'pos' moving along unit 'dir' at 'tspeed'; the aim point goes to out.
s8 EXGetPursuitAngles(const EXVector &from, float speed, const EXVector &pos, const EXVector &dir, float tspeed, EXVector &out);
// NaN/infinity checks (EXMaths.obj).
s8 isValidFloat(const EXVector &v);
#endif
