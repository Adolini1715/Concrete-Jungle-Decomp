#include <math.h>
#include "EXMatrix.h"
#include "EXQuaternion.h"

using namespace EXMaths;

void EXMatrix::GetRotation(EXQuaternion &q) const
{
    q.SetRotation(*this);
}

void EXMatrix::GetRotation(EXAngleAxis &aa) const
{
    aa.SetRotation(*this);
}

EXScaleMatrix::EXScaleMatrix(const EXVector &s)
{
    r[0].set(s.x, 0.0f, 0.0f, 0.0f);
    r[1].set(0.0f, s.y, 0.0f, 0.0f);
    r[2].set(0.0f, 0.0f, s.z, 0.0f);
    r[3].set_zero1();
}

EXScaleMatrix::EXScaleMatrix(float f)
{
    r[0].set(f, 0.0f, 0.0f, 0.0f);
    r[1].set(0.0f, f, 0.0f, 0.0f);
    r[2].set(0.0f, 0.0f, f, 0.0f);
    r[3].set_zero1();
}

EXScaleMatrix::EXScaleMatrix(float sx, float sy, float sz)
{
    r[0].set(sx, 0.0f, 0.0f, 0.0f);
    r[1].set(0.0f, sy, 0.0f, 0.0f);
    r[2].set(0.0f, 0.0f, sz, 0.0f);
    r[3].set_zero1();
}

// Rotation orders: 0 quaternion, 1 angle-axis, 2 XYZ, 3 XZY, 4 YXZ, 5 YZX, 6 ZXY, 7 ZYX.
void EXMatrix::GetRotation(EXVector &a, u8 order) const
{
    switch (order) {
    case 3: GetRotationXZY(a); break;
    case 6: GetRotationZXY(a); break;
    case 7: GetRotationZYX(a); break;
    case 5: GetRotationYZX(a); break;
    case 4: GetRotationYXZ(a); break;
    case 0: GetRotation(*(EXQuaternion *)&a); break;
    case 1: GetRotation(*(EXAngleAxis *)&a); break;
    default: GetRotationXYZ(a); break;
    }
}

void EXMatrix::GetRotationXYZ(EXVector &a) const
{
    float x, y, z;
    if (r[2].x < 1.0f) {
        if (r[2].x > -1.0f) {
            x = (float)atan2(-r[2].y, r[2].z);
            y = EXASin(r[2].x);
            z = (float)atan2(-r[1].x, r[0].x);
        } else {
            x = -(float)atan2(r[0].y, r[1].y);
            y = -1.5707964f;
            z = 0.0f;
        }
    } else {
        x = (float)atan2(r[0].y, r[1].y);
        y = 1.5707964f;
        z = 0.0f;
    }
    a.Set(x, y, z, 0.0f);
}

void EXMatrix::GetRotationXZY(EXVector &a) const
{
    float x, y, z;
    if (r[1].x < 1.0f) {
        if (r[1].x > -1.0f) {
            x = (float)atan2(r[1].z, r[1].y);
            z = EXASin(-r[1].x);
            y = (float)atan2(r[2].x, r[0].x);
        } else {
            x = (float)atan2(r[0].z, r[2].z);
            z = 1.5707964f;
            y = 0.0f;
        }
    } else {
        x = (float)atan2(-r[0].z, r[2].z);
        z = -1.5707964f;
        y = 0.0f;
    }
    a.Set(x, y, z, 0.0f);
}

void EXMatrix::GetRotationYXZ(EXVector &a) const
{
    float x, y, z;
    if (r[2].y < 1.0f) {
        if (r[2].y > -1.0f) {
            y = (float)atan2(r[2].x, r[2].z);
            x = EXASin(-r[2].y);
            z = (float)atan2(r[0].y, r[1].y);
        } else {
            y = (float)atan2(r[1].x, r[0].x);
            x = 1.5707964f;
            z = 0.0f;
        }
    } else {
        y = (float)atan2(-r[1].x, r[0].x);
        x = -1.5707964f;
        z = 0.0f;
    }
    a.Set(x, y, z, 0.0f);
}

void EXMatrix::GetRotationYZX(EXVector &a) const
{
    float x, y, z;
    if (r[0].y < 1.0f) {
        if (r[0].y > -1.0f) {
            y = (float)atan2(-r[0].z, r[0].x);
            z = EXASin(r[0].y);
            x = (float)atan2(-r[2].y, r[1].y);
        } else {
            y = -(float)atan2(r[1].z, r[2].z);
            z = -1.5707964f;
            x = 0.0f;
        }
    } else {
        y = (float)atan2(r[1].z, r[2].z);
        z = 1.5707964f;
        x = 0.0f;
    }
    a.Set(x, y, z, 0.0f);
}

void EXMatrix::GetRotationZXY(EXVector &a) const
{
    float x, y, z;
    if (r[1].z < 1.0f) {
        if (r[1].z > -1.0f) {
            z = (float)atan2(-r[1].x, r[1].y);
            x = EXASin(r[1].z);
            y = (float)atan2(-r[0].z, r[2].z);
        } else {
            z = -(float)atan2(r[2].x, r[0].x);
            x = -1.5707964f;
            y = 0.0f;
        }
    } else {
        z = (float)atan2(r[2].x, r[0].x);
        x = 1.5707964f;
        y = 0.0f;
    }
    a.Set(x, y, z, 0.0f);
}

void EXMatrix::GetRotationZYX(EXVector &a) const
{
    float x, y, z;
    if (r[0].z < 1.0f) {
        if (r[0].z > -1.0f) {
            z = (float)atan2(r[0].y, r[0].x);
            y = EXASin(-r[0].z);
            x = (float)atan2(r[1].z, r[2].z);
        } else {
            z = -(float)atan2(r[1].x, r[2].x);
            y = 1.5707964f;
            x = 0.0f;
        }
    } else {
        z = (float)atan2(-r[1].x, -r[2].x);
        y = -1.5707964f;
        x = 0.0f;
    }
    a.Set(x, y, z, 0.0f);
}

void EXMatrix::GetScale(float &sx, float &sy, float &sz) const
{
    sx = r[0].LengthXYZ();
    sy = r[1].LengthXYZ();
    sz = r[2].LengthXYZ();
}

EXMatrix InterpolateR(const EXMatrix &a, const EXMatrix &b, float t)
{
    EXQuaternion qa(a);
    EXQuaternion qb(b);
    EXQuaternion q = qb.slerp(qa, t);
    EXMatrix m;
    q.GetRotation(m);
    return m;
}

EXMatrix Interpolate(const EXMatrix &a, const EXMatrix &b, float t)
{
    EXMatrix m;
    m = InterpolateR(a, b, t);
    m.r[3] = a.r[3].Interp(b.r[3], t);
    return m;
}

namespace {
const float kNormEps = 1.0e-5f;
const float kParallel = 0.9997f;

// Normalise in place (length over all given components); leaves tiny vectors untouched.
inline void Norm3(float &x, float &y, float &z)
{
    float l2 = x * x + y * y + z * z;
    if (l2 >= 0.0f) {
        float l = sqrtf(l2);
        if (l > kNormEps) {
            float inv = 1.0f / l;
            x *= inv; y *= inv; z *= inv;
        }
    }
}
inline void Norm4(float &x, float &y, float &z, float &w)
{
    float l2 = w * w + z * z + y * y + x * x;
    if (l2 >= 0.0f) {
        float l = sqrtf(l2);
        if (l > kNormEps) {
            float inv = 1.0f / l;
            x *= inv; y *= inv; z *= inv; w *= inv;
        }
    }
}
}

float EXMatrix::Determinant() const
{
    float a = r[3].w * r[2].z - r[3].z * r[2].w;
    float b = r[3].w * r[2].y - r[3].y * r[2].w;
    float c = r[3].z * r[2].y - r[3].y * r[2].z;
    float d = r[3].w * r[2].x - r[3].x * r[2].w;
    float e = r[3].z * r[2].x - r[3].x * r[2].z;
    float f = r[3].y * r[2].x - r[3].x * r[2].y;
    return (f * r[1].w + (b * r[1].x - d * r[1].y)) * r[0].z
         + ((c * r[1].w + (a * r[1].y - b * r[1].z)) * r[0].x
          - (e * r[1].w + (a * r[1].x - d * r[1].z)) * r[0].y)
         - (f * r[1].z + (c * r[1].x - e * r[1].y)) * r[0].w;
}

// Build an orthonormal basis with +Z along 'dir'; X comes from 'up' x Z (or 'alt' x Z when 'up' is parallel).
void EXMatrix::AlignToDir(const EXVector &dir, const EXVector &up, const EXVector &alt)
{
    float zx = dir.x, zy = dir.y, zz = dir.z, zw = dir.w;
    Norm4(zx, zy, zz, zw);
    float xx, xy, xz;
    const EXVector &ref = (fabsf(zx * up.x + zy * up.y + zz * up.z + zw * up.w) <= kParallel) ? up : alt;
    xx = ref.y * zz - ref.z * zy;
    xy = ref.z * zx - ref.x * zz;
    xz = ref.x * zy - ref.y * zx;
    Norm3(xx, xy, xz);
    float yx = xz * zy - xy * zz;
    float yy = zz * xx - xz * zx;
    float yz = xy * zx - xx * zy;
    Norm3(yx, yy, yz);
    r[0].x = xx; r[0].y = xy; r[0].z = xz;
    r[1].x = yx; r[1].y = yy; r[1].z = yz;
    r[2].x = zx; r[2].y = zy; r[2].z = zz;
}

// As AlignToDir, but the first vector becomes the up axis (Y).
void EXMatrix::AlignToUp(const EXVector &upv, const EXVector &dir, const EXVector &alt)
{
    float yx = upv.x, yy = upv.y, yz = upv.z, yw = upv.w;
    Norm4(yx, yy, yz, yw);
    const EXVector &ref = (fabsf(yx * dir.x + yy * dir.y + yz * dir.z + yw * dir.w) <= kParallel) ? dir : alt;
    float xx = ref.z * yy - ref.y * yz;
    float xy = ref.x * yz - ref.z * yx;
    float xz = ref.y * yx - ref.x * yy;
    Norm3(xx, xy, xz);
    float zx = xy * yz - xz * yy;
    float zy = xz * yx - xx * yz;
    float zz = xx * yy - xy * yx;
    Norm3(zx, zy, zz);
    r[0].x = xx; r[0].y = xy; r[0].z = xz;
    r[1].x = yx; r[1].y = yy; r[1].z = yz;
    r[2].x = zx; r[2].y = zy; r[2].z = zz;
}

// Make the matrix's up axis the plane normal, keeping the old direction (projected) where possible.
void EXMatrix::AlignToPlane(const EXVector &n)
{
    float nx = n.x, ny = n.y, nz = n.z, nw = n.w;
    Norm4(nx, ny, nz, nw);
    float dx = r[2].x, dy = r[2].y, dz = r[2].z, dw = 0.0f;
    {
        float l2 = dz * dz + dy * dy + dx * dx;
        if (l2 >= 0.0f) {
            float l = sqrtf(l2);
            if (l > kNormEps) {
                float inv = 1.0f / l;
                dx *= inv; dy *= inv; dz *= inv; dw = inv * 0.0f;
            }
        }
    }
    float dot = dx * nx + dy * ny + dz * nz + dw * nw;
    float xx, xy, xz, zx, zy, zz;
    if (fabsf(dot) <= kParallel) {
        dx -= nx * dot;
        dy -= ny * dot;
        dz -= nz * dot;
        dw -= nw * dot;
        float l2 = dw * dw + dz * dz + dy * dy + dx * dx;
        if (l2 >= 0.0f) {
            float l = sqrtf(l2);
            if (l > kNormEps) {
                float inv = 1.0f / l;
                dx *= inv; dy *= inv; dz *= inv;
            }
        }
        xx = dz * ny - dy * nz;
        xy = nz * dx - dz * nx;
        xz = dy * nx - ny * dx;
        zx = dx; zy = dy; zz = dz;
    } else {
        float ux = r[1].x, uy = r[1].y, uz = r[1].z;
        Norm3(ux, uy, uz);
        xx = uz * ny - uy * nz;
        xy = nz * ux - uz * nx;
        xz = uy * nx - ny * ux;
        zx = xz * ny - xy * nz;
        zy = nz * xx - xz * nx;
        zz = xy * nx - xx * ny;
    }
    r[0].x = xx; r[0].y = xy; r[0].z = xz;
    r[1].x = nx; r[1].y = ny; r[1].z = nz;
    r[2].x = zx; r[2].y = zy; r[2].z = zz;
}

void EXMatrix::LookAt(const EXVector &eye, const EXVector &target, const EXVector &up, const EXVector &alt)
{
    EXVector dir(target.x - eye.x, target.y - eye.y, target.z - eye.z, target.w - eye.w);
    AlignToDir(dir, up, alt);
    r[3].x = eye.x; r[3].y = eye.y; r[3].z = eye.z; r[3].w = eye.w;
}

// Rotation taking unit vector 'from' onto 'to' (Moller-Hughes); falls back to a reflection pair
// through the least-aligned axis when they are (anti)parallel.
void EXMatrix::GetRotationTo(const EXVector &from, const EXVector &to)
{
    float vx = to.z * from.y - from.z * to.y;
    float vy = from.z * to.x - to.z * from.x;
    float vz = to.y * from.x - to.x * from.y;
    float e = from.z * to.z + from.w * to.w + to.y * from.y + from.x * to.x;
    if (fabsf(e) <= 0.99999988f) {
        float h = 1.0f / (e + 1.0f);
        float hx = vx * h;
        r[0].x = vx * hx + e;
        r[1].x = vy * hx - vz;
        r[0].y = vz + vy * hx;
        r[0].z = vz * hx - vy;
        r[1].y = vy * vy * h + e;
        float yz = vy * vz * h;
        r[1].z = vx + yz;
        r[2].x = vy + vz * hx;
        r[2].y = yz - vx;
        r[2].z = vz * vz * h + e;
        return;
    }
    float ax[3] = { 0.0f, 0.0f, 0.0f };
    if (fabsf(from.y) <= fabsf(from.x)) {
        if (fabsf(from.z) <= fabsf(from.y)) ax[2] = 1.0f;
        else ax[1] = 1.0f;
    } else {
        if (fabsf(from.x) < fabsf(from.z)) ax[0] = 1.0f;
        else ax[2] = 1.0f;
    }
    float u[3] = { ax[0] - from.x, ax[1] - from.y, ax[2] - from.z };
    float v[3] = { ax[0] - to.x, ax[1] - to.y, ax[2] - to.z };
    float uw = 0.0f - from.w, vw = 0.0f - to.w;
    float c1 = 2.0f / (u[1] * u[1] + u[2] * u[2] + uw * uw + u[0] * u[0]);
    float c2 = 2.0f / (v[1] * v[1] + v[2] * v[2] + vw * vw + v[0] * v[0]);
    float c3 = (v[1] * u[1] + v[2] * u[2] + vw * uw + v[0] * u[0]) * c2 * c1;
    for (int i = 0; i < 3; i++) {
        float *row = &r[i].x;
        for (int j = 0; j < 3; j++) {
            row[j] = v[i] * u[j] * c3 - (v[i] * v[j] * c2 + u[j] * c1 * u[i]);
        }
        row[i] += 1.0f;
    }
}

// Split into scale (out param 1) and rotation angles (out param 2); negative axes are folded into the scale.
void EXMatrix::Decompose(EXVector &scale, EXVector &rot, u8 order) const
{
    GetScale(scale.x, scale.y, scale.z);
    EXMatrix S;
    S.SetIdentity();
    S.r[0].x = scale.x;
    S.r[1].y = scale.y;
    S.r[2].z = scale.z;
    EXMatrix rm = *this * S.Inverse();
    rm.GetRotation(rot, order);

    EXMatrix R;
    R.SetRotation(rot, order);
    EXMatrix chk = R.Inverse() * *this;
    if (chk.r[0].x < 0.0f) scale.x *= -1.0f;
    if (chk.r[1].y < 0.0f) scale.y *= -1.0f;
    if (chk.r[2].z < 0.0f) scale.z *= -1.0f;
}
