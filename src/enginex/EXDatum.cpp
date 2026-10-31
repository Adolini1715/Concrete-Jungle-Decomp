#include <math.h>
#include "EXDatum.h"
#include "EXMatrix.h"

#include "EXSphere.h"
#include "EXCylinder.h"
#include "EXLozenge.h"
#include "EXCapsule.h"
#include "EXBox.h"
// TODO stand-in: datum record in a geo file (geo area).
struct EXGeoBaseDatum {
    u32 m_00;
    u16 m_flags;            // +4
    u8 m_type;              // +6
    u8 m_07;
    EXVector3 m_size;       // +8
    EXVector3 m_pos;        // +0x14
    EXMaths::vec_xyzw m_rot;    // +0x20
};

void EXDatumTemp::Set(const EXGeoBaseDatum *pDatum, const EXVector &origin)
{
    m_position = EXVector(pDatum->m_pos, 1.0f) - origin;
    m_rotation = EXQuaternion(pDatum->m_rot);
    m_2c = pDatum->m_flags;
    m_type = pDatum->m_type;
    m_size.x = pDatum->m_size.x;
    m_size.y = pDatum->m_size.y;
    m_size.z = pDatum->m_size.z;
}

// Rotation and uniform scale (the largest axis scale) of a transform; near-unit scale takes the
// direct matrix-to-quaternion path.
static void RotationAndScale(const EXMatrix &m, EXQuaternion &q, float &scale)
{
    float sx, sy, sz;
    m.GetScale(sx, sy, sz);
    if (fabsf(sx - 1.0f) < 0.001f && fabsf(sy - 1.0f) < 0.001f && fabsf(sz - 1.0f) < 0.001f) {
        q.SetRotation(m);
        scale = 1.0f;
        return;
    }
    EXScaleMatrix sm(sx, sy, sz);
    EXMatrix rot = m * sm.Inverse3();
    EXVector angles;
    rot.GetRotation(angles, 2);
    q.EXMaths::quat_xyzw::SetRotation(angles, 2);
    scale = sx;
    if (sx < sy) {
        scale = sy;
        if (sy < sz) scale = sz;
    } else if (sz > sx) {
        scale = sz;
    }
}

// Hamilton product a * b.
static EXQuaternion QuatMul(const EXQuaternion &a, const EXQuaternion &b)
{
    return EXQuaternion(a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
                        a.w * b.y + a.y * b.w + a.z * b.x - a.x * b.z,
                        a.w * b.z + a.z * b.w + a.x * b.y - a.y * b.x,
                        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z);
}

void EXDatumTemp::Set(const EXGeoBaseDatum *pDatum, const EXVector &origin, const EXMatrix &mtx)
{
    EXVector rel(pDatum->m_pos.x - origin.x, pDatum->m_pos.y - origin.y, pDatum->m_pos.z - origin.z, 1.0f - origin.w);
    rel *= mtx;
    m_position = rel;
    EXQuaternion q;
    float scale;
    RotationAndScale(mtx, q, scale);
    m_rotation = QuatMul(q, EXQuaternion(pDatum->m_rot));
    m_2c = pDatum->m_flags;
    m_type = pDatum->m_type;
    m_size.x = pDatum->m_size.x * scale;
    m_size.y = pDatum->m_size.y * scale;
    m_size.z = pDatum->m_size.z * scale;
}

void EXDatumTemp::ApplyXForm(const EXMatrix &mtx)
{
    m_position.w = 1.0f;
    m_position *= mtx;
    m_position.w = 0.0f;
    EXQuaternion q;
    float scale;
    RotationAndScale(mtx, q, scale);
    m_rotation = QuatMul(q, m_rotation);
    m_size.x *= scale;
    m_size.y *= scale;
    m_size.z *= scale;
}

void EXDatumTemp::Get(EXSphere &s) const
{
    s.m_centre = m_position;
    s.m_radius = m_size.x;
}

void EXDatumTemp::Get(EXCylinder &c) const
{
    EXMatrix m;
    m.set_rotate_quat(m_rotation);
    EXMaths::vec_xyzw d = (const EXMaths::mat_44 &)m * EXVector(0.0f, m_size.x, 0.0f, 0.0f);
    c.Centre() = m_position;
    c.Direction() = EXVector(d);
    c.Radius() = m_size.y;
}

void EXDatumTemp::Get(EXCapsule &c) const
{
    EXMatrix m;
    m.set_rotate_quat(m_rotation);
    EXVector d = EXVector((const EXMaths::mat_44 &)m * EXVector(0.0f, m_size.x, 0.0f, 0.0f));
    c.Origin() = m_position - d;
    c.Direction() = d * 2.0f;
    c.Radius() = m_size.y;
}

void EXDatumTemp::Get(EXBox &b) const
{
    EXMatrix m;
    m.set_rotate_quat(m_rotation);
    b.m_centre = m_position;
    b.m_axis[0] = EXVector((const EXMaths::mat_44 &)m * EXVector(1.0f, 0.0f, 0.0f, 0.0f));
    b.m_axis[1] = EXVector((const EXMaths::mat_44 &)m * EXVector(0.0f, 1.0f, 0.0f, 0.0f));
    b.m_extent[0] = m_size.x;
    b.m_extent[1] = m_size.y;
    b.m_extent[2] = m_size.z;
}

void EXDatumTemp::Get(EXLozenge &l) const
{
    EXMatrix m;
    m.set_rotate_quat(m_rotation);
    EXVector a = EXVector((const EXMaths::mat_44 &)m * EXVector(m_size.x, 0.0f, 0.0f, 0.0f));
    EXVector b = EXVector((const EXMaths::mat_44 &)m * EXVector(0.0f, 0.0f, m_size.y, 0.0f));
    l.Origin() = (m_position - a) - b;
    l.Edge0() = a * 2.0f;
    l.Edge1() = b * 2.0f;
    l.Radius() = m_size.z;
}

void EXDatumTemp::Get(EXVector &v) const { v = m_position; }
