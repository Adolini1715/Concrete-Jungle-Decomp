#include "EXMaths.h"

#include "EXWnd.h"
#include "EXBaseDisplay.h"

namespace EXMaths {

void mat_44::Print(const char *name) const
{
    EXWnd *pWnd = EXBaseDisplay::ActiveWnd();
    if (pWnd) {
        pWnd->DbgPrint("m44 %s:\n", name);
        get_row0().Print(0);
        get_row1().Print(0);
        get_row2().Print(0);
        get_row3().Print(0);
    }
}

void mat_44::Trace(const char *name) const
{
    get_row0().Trace(0);
    get_row1().Trace(0);
    get_row2().Trace(0);
    get_row3().Trace(0);
}

mat_44 mat_44::transpose() const
{
    mat_44 t;
    t.r[0].x = r[0].x; t.r[0].y = r[1].x; t.r[0].z = r[2].x; t.r[0].w = r[3].x;
    t.r[1].x = r[0].y; t.r[1].y = r[1].y; t.r[1].z = r[2].y; t.r[1].w = r[3].y;
    t.r[2].x = r[0].z; t.r[2].y = r[1].z; t.r[2].z = r[2].z; t.r[2].w = r[3].z;
    t.r[3].x = r[0].w; t.r[3].y = r[1].w; t.r[3].z = r[2].w; t.r[3].w = r[3].w;
    return t;
}

mat_44 mat_44::transpose3() const
{
    mat_44 t;
    t.r[0].x = r[0].x; t.r[0].y = r[1].x; t.r[0].z = r[2].x; t.r[0].w = 0.0f;
    t.r[1].x = r[0].y; t.r[1].y = r[1].y; t.r[1].z = r[2].y; t.r[1].w = 0.0f;
    t.r[2].x = r[0].z; t.r[2].y = r[1].z; t.r[2].z = r[2].z; t.r[2].w = 0.0f;
    t.r[3].x = r[3].x; t.r[3].y = r[3].y; t.r[3].z = r[3].z; t.r[3].w = r[3].w;
    return t;
}

// Rotation part inverted by cofactors; an orthonormal rotation (|det| == 1) just transposes.
// NEAR: ~600 bytes differ, same math; x87 operand order of the determinant, the frame layout (det/t
// slots) and the cofactor scheduling differ.
// Full inverse by Cramer's rule (Intel's "Streaming SIMD Extensions - Inverse of 4x4 Matrix",
// scalar form): cofactors of the transpose from 2x2 products, then scaled by 1/det (0 when singular).
mat_44 mat_44::inverse() const
{
    mat_44 t = transpose();
    float src[16];
    for (int i = 0; i < 16; i++) src[i] = ((const float *)&t)[i];
    float tmp[12];
    float dst[16];
    tmp[0] = src[10] * src[15];
    tmp[1] = src[11] * src[14];
    tmp[2] = src[9] * src[15];
    tmp[3] = src[11] * src[13];
    tmp[4] = src[9] * src[14];
    tmp[5] = src[10] * src[13];
    tmp[6] = src[8] * src[15];
    tmp[7] = src[11] * src[12];
    tmp[8] = src[8] * src[14];
    tmp[9] = src[10] * src[12];
    tmp[10] = src[8] * src[13];
    tmp[11] = src[9] * src[12];
    dst[0] = tmp[0] * src[5] + tmp[3] * src[6] + tmp[4] * src[7];
    dst[0] -= tmp[1] * src[5] + tmp[2] * src[6] + tmp[5] * src[7];
    dst[1] = tmp[1] * src[4] + tmp[6] * src[6] + tmp[9] * src[7];
    dst[1] -= tmp[0] * src[4] + tmp[7] * src[6] + tmp[8] * src[7];
    dst[2] = tmp[2] * src[4] + tmp[7] * src[5] + tmp[10] * src[7];
    dst[2] -= tmp[3] * src[4] + tmp[6] * src[5] + tmp[11] * src[7];
    dst[3] = tmp[5] * src[4] + tmp[8] * src[5] + tmp[11] * src[6];
    dst[3] -= tmp[4] * src[4] + tmp[9] * src[5] + tmp[10] * src[6];
    dst[4] = tmp[1] * src[1] + tmp[2] * src[2] + tmp[5] * src[3];
    dst[4] -= tmp[0] * src[1] + tmp[3] * src[2] + tmp[4] * src[3];
    dst[5] = tmp[0] * src[0] + tmp[7] * src[2] + tmp[8] * src[3];
    dst[5] -= tmp[1] * src[0] + tmp[6] * src[2] + tmp[9] * src[3];
    dst[6] = tmp[3] * src[0] + tmp[6] * src[1] + tmp[11] * src[3];
    dst[6] -= tmp[2] * src[0] + tmp[7] * src[1] + tmp[10] * src[3];
    dst[7] = tmp[4] * src[0] + tmp[9] * src[1] + tmp[10] * src[2];
    dst[7] -= tmp[5] * src[0] + tmp[8] * src[1] + tmp[11] * src[2];
    tmp[0] = src[2] * src[7];
    tmp[1] = src[3] * src[6];
    tmp[2] = src[1] * src[7];
    tmp[3] = src[3] * src[5];
    tmp[4] = src[1] * src[6];
    tmp[5] = src[2] * src[5];
    tmp[6] = src[0] * src[7];
    tmp[7] = src[3] * src[4];
    tmp[8] = src[0] * src[6];
    tmp[9] = src[2] * src[4];
    tmp[10] = src[0] * src[5];
    tmp[11] = src[1] * src[4];
    dst[8] = tmp[0] * src[13] + tmp[3] * src[14] + tmp[4] * src[15];
    dst[8] -= tmp[1] * src[13] + tmp[2] * src[14] + tmp[5] * src[15];
    dst[9] = tmp[1] * src[12] + tmp[6] * src[14] + tmp[9] * src[15];
    dst[9] -= tmp[0] * src[12] + tmp[7] * src[14] + tmp[8] * src[15];
    dst[10] = tmp[2] * src[12] + tmp[7] * src[13] + tmp[10] * src[15];
    dst[10] -= tmp[3] * src[12] + tmp[6] * src[13] + tmp[11] * src[15];
    dst[11] = tmp[5] * src[12] + tmp[8] * src[13] + tmp[11] * src[14];
    dst[11] -= tmp[4] * src[12] + tmp[9] * src[13] + tmp[10] * src[14];
    dst[12] = tmp[2] * src[10] + tmp[5] * src[11] + tmp[1] * src[9];
    dst[12] -= tmp[4] * src[11] + tmp[0] * src[9] + tmp[3] * src[10];
    dst[13] = tmp[8] * src[11] + tmp[0] * src[8] + tmp[7] * src[10];
    dst[13] -= tmp[6] * src[10] + tmp[9] * src[11] + tmp[1] * src[8];
    dst[14] = tmp[6] * src[9] + tmp[11] * src[11] + tmp[3] * src[8];
    dst[14] -= tmp[10] * src[11] + tmp[2] * src[8] + tmp[7] * src[9];
    dst[15] = tmp[10] * src[10] + tmp[4] * src[8] + tmp[9] * src[9];
    dst[15] -= tmp[8] * src[9] + tmp[11] * src[10] + tmp[5] * src[8];
    float det = src[0] * dst[0] + src[1] * dst[1] + src[2] * dst[2] + src[3] * dst[3];
    if (det != 0.0f) det = 1.0f / det;
    mat_44 m;
    for (int j = 0; j < 16; j++) ((float *)&m)[j] = dst[j] * det;
    return m;
}

mat_44 mat_44::inverse3() const
{
    float det = (r[1].y * r[2].z - r[2].y * r[1].z) * r[0].x - (r[1].x * r[2].z - r[2].x * r[1].z) * r[0].y +
                (r[2].y * r[1].x - r[2].x * r[1].y) * r[0].z;
    mat_44 t;
    if ((float)fabs((float)fabs(det) - 1.0f) <= 1.1920929e-6f) {
        t.r[0].x = r[0].x; t.r[0].y = r[1].x; t.r[0].z = r[2].x; t.r[0].w = 0.0f;
        t.r[1].x = r[0].y; t.r[1].y = r[1].y; t.r[1].z = r[2].y; t.r[1].w = 0.0f;
        t.r[2].x = r[0].z; t.r[2].y = r[1].z; t.r[2].z = r[2].z; t.r[2].w = 0.0f;
        t.r[3].x = r[3].x; t.r[3].y = r[3].y; t.r[3].z = r[3].z; t.r[3].w = r[3].w;
    } else {
        float inv = det;
        if (det != 0.0f) inv = 1.0f / det;
        t.r[0].x = (r[2].z * r[1].y - r[2].y * r[1].z) * inv;
        t.r[0].y = -(r[2].z * r[0].y - r[0].z * r[2].y) * inv;
        t.r[0].z = (r[1].z * r[0].y - r[0].z * r[1].y) * inv;
        t.r[0].w = 0.0f * inv;
        t.r[1].x = -(r[2].z * r[1].x - r[2].x * r[1].z) * inv;
        t.r[1].y = (r[0].x * r[2].z - r[0].z * r[2].x) * inv;
        t.r[1].z = -(r[1].z * r[0].x - r[0].z * r[1].x) * inv;
        t.r[1].w = 0.0f * inv;
        t.r[2].x = (r[2].y * r[1].x - r[2].x * r[1].y) * inv;
        t.r[2].y = -(r[2].y * r[0].x - r[2].x * r[0].y) * inv;
        t.r[2].z = (r[0].x * r[1].y - r[1].x * r[0].y) * inv;
        t.r[2].w = 0.0f * inv;
        t.r[3] = r[3];
    }
    return t;
}

mat_44 mat_44::inverse3trans() const
{
    mat_44 t = inverse3();
    t.r[3].x = 0.0f - r[3].x;
    t.r[3].y = 0.0f - r[3].y;
    t.r[3].z = 0.0f - r[3].z;
    t.r[3].w = r[3].w;
    return t;
}

// NEAR: 210 bytes differ, same math; load/spill scheduling of the transposed rows differs.
mat_44 mat_44::fastinverse() const
{
    mat_44 t;
    t.r[0].x = r[0].x; t.r[0].y = r[1].x; t.r[0].z = r[2].x; t.r[0].w = 0.0f;
    t.r[1].x = r[0].y; t.r[1].y = r[1].y; t.r[1].z = r[2].y; t.r[1].w = 0.0f;
    t.r[2].x = r[0].z; t.r[2].y = r[1].z; t.r[2].z = r[2].z; t.r[2].w = 0.0f;
    t.r[3].x = 0.0f - r[3].x * t.r[0].x - r[3].y * t.r[1].x - r[3].z * t.r[2].x;
    t.r[3].y = 0.0f - r[3].x * t.r[0].y - r[3].y * t.r[1].y - r[3].z * t.r[2].y;
    t.r[3].z = 0.0f - r[3].x * t.r[0].z - r[3].y * t.r[1].z - r[3].z * t.r[2].z;
    t.r[3].w = 1.0f;
    return t;
}

// NEAR: 256 bytes differ, same math; product evaluation order / register allocation differs.
void mat_44::set_rotate_quat(const vec_xyzw &q)
{
    float x2 = q.x * 2.0f, y2 = q.y * 2.0f, z2 = q.z * 2.0f;
    float xx = q.x * x2, xy = q.x * y2, xz = q.x * z2;
    float yy = q.y * y2, yz = q.y * z2, zz = q.z * z2;
    float wx = q.w * x2, wy = q.w * y2, wz = q.w * z2;
    r[0].x = 1.0f - (zz + yy); r[0].y = xy + wz; r[0].z = xz - wy; r[0].w = 0.0f;
    r[1].x = xy - wz; r[1].y = 1.0f - (zz + xx); r[1].z = yz + wx; r[1].w = 0.0f;
    r[2].x = xz + wy; r[2].y = yz - wx; r[2].z = 1.0f - (yy + xx); r[2].w = 0.0f;
    r[3].x = 0.0f; r[3].y = 0.0f; r[3].z = 0.0f; r[3].w = 1.0f;
}

// NEAR: ~300 bytes differ, inherits the set_rotate_quat scheduling difference after the (matching) quat build.
void mat_44::set_rotate_axis(const vec_xyzw &aa)
{
    quat_xyzw q;
    q.set_rotate_axis(aa);
    set_rotate_quat(q);
}

__forceinline void mat_44::set_rotate_xzy(const vec_xyzw &a)
{
    float sx, cx, sy, cy, sz, cz;
    EXSinCos(a.x, sx, cx);
    EXSinCos(a.y, sy, cy);
    EXSinCos(a.z, sz, cz);
    r[0].set(cy * cz, sz * cy * cx + sy * sx, sz * cy * sx - sy * cx, 0.0f);
    r[1].set(-sz, cx * cz, sx * cz, 0.0f);
    r[2].set(cz * sy, sz * sy * cx - cy * sx, sz * sy * sx + cy * cx, 0.0f);
    r[3].set(0.0f, 0.0f, 0.0f, 1.0f);
}

__forceinline void mat_44::set_rotate_yxz(const vec_xyzw &a)
{
    float sx, cx, sy, cy, sz, cz;
    EXSinCos(a.x, sx, cx);
    EXSinCos(a.y, sy, cy);
    EXSinCos(a.z, sz, cz);
    r[0].set(cz * cy + sz * sx * sy, sz * cx, sz * sx * cy - cz * sy, 0.0f);
    r[1].set(cz * sx * sy - sz * cy, cz * cx, sz * sy + cz * sx * cy, 0.0f);
    r[2].set(cx * sy, -sx, cx * cy, 0.0f);
    r[3].set(0.0f, 0.0f, 0.0f, 1.0f);
}

__forceinline void mat_44::set_rotate_yzx(const vec_xyzw &a)
{
    float sx, cx, sy, cy, sz, cz;
    EXSinCos(a.x, sx, cx);
    EXSinCos(a.y, sy, cy);
    EXSinCos(a.z, sz, cz);
    r[0].set(cz * cy, sz, -(cz * sy), 0.0f);
    r[1].set(sy * sx - sz * cy * cx, cz * cx, sz * sy * cx + cy * sx, 0.0f);
    r[2].set(sz * cy * sx + sy * cx, -(cz * sx), cy * cx - sz * sy * sx, 0.0f);
    r[3].set(0.0f, 0.0f, 0.0f, 1.0f);
}

__forceinline void mat_44::set_rotate_zyx(const vec_xyzw &a)
{
    float sx, cx, sy, cy, sz, cz;
    EXSinCos(a.x, sx, cx);
    EXSinCos(a.y, sy, cy);
    EXSinCos(a.z, sz, cz);
    r[0].set(cz * cy, sz * cy, -sy, 0.0f);
    r[1].set(cz * sy * sx - sz * cx, sz * sy * sx + cz * cx, cy * sx, 0.0f);
    r[2].set(cz * sy * cx + sz * sx, sz * sy * cx - cz * sx, cy * cx, 0.0f);
    r[3].set(0.0f, 0.0f, 0.0f, 1.0f);
}

// Rotation orders: 1 angle-axis, 2 XYZ, 3 XZY, 4 YXZ, 5 YZX, 6 ZXY, 7 ZYX (others leave the matrix).
void mat_44::set_rotate(const vec_xyzw &a, u8 order)
{
    switch (order) {
    case 1: set_rotate_axis(a); break;
    case 2: set_rotate_xyz(a); break;
    case 3: set_rotate_xzy(a); break;
    case 4: set_rotate_yxz(a); break;
    case 5: set_rotate_yzx(a); break;
    case 6: set_rotate_zxy(a); break;
    case 7: set_rotate_zyx(a); break;
    }
}

} // namespace EXMaths
