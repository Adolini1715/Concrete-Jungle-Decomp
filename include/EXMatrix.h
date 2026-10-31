#ifndef EXMATRIX_H
#define EXMATRIX_H
#include "EXVector.h"

class EXQuaternion;
class EXAngleAxis;

// The engine matrix: a mat_44 with the EXMatrix API on top.
class EXMatrix : public EXMaths::mat_44 {
public:
    EXMatrix() {}
    EXMatrix(const EXMaths::mat_44 &m) { *(mat_44 *)this = m; }
    EXMatrix &operator=(const EXMatrix &m) { *(mat_44 *)this = m; return *this; }
    operator const EXMaths::mat_44 &() const { return *this; }

    void SetIdentity() { set_identity(); }
    void SetZero() { set_zero(); }
    void SetRotation(const EXVector &a, u8 order) { set_rotate(a, order); }
    void SetRotation(const EXQuaternion &q);
    void SetRotation(const EXAngleAxis &aa);
    void SetRotationXYZ(const EXVector &a) { set_rotate_xyz(a); }
    void SetRotationYXZ(const EXVector &a) { set_rotate_yxz(a); }
    __forceinline void SetRotationZXY(const EXVector &a) { set_rotate_zxy(a); }  // [fi] in XBParticle.obj
    void SetScale(const EXVector &s) { set_scale(s); }
    void SetDir(const EXVector &v) { r[2].x = v.x; r[2].y = v.y; r[2].z = v.z; }
    void SetUp(const EXVector &v) { r[1].x = v.x; r[1].y = v.y; r[1].z = v.z; }
    void SetNorm(const EXVector &v) { r[0].x = v.x; r[0].y = v.y; r[0].z = v.z; }
    void SetTranslation(const EXVector &v) { r[3].set(v); r[3].w = 1.0f; }
    // NEAR (EXItemEnv.obj copy): retail stores r[3].w = 1 with the identity and copies only x, y, z.
    void SetTranslate(const EXVector &v)
    {
        SetIdentity();
        SetTranslation(v);
    }
    EXVector GetTranslation() const { return EXVector(r[3]); }
    EXVector GetDir() const { return EXVector(r[2].x, r[2].y, r[2].z, 0.0f); }
    EXVector GetUp() const { return EXVector(r[1].x, r[1].y, r[1].z, 0.0f); }
    EXVector GetNorm() const { return EXVector(r[0].x, r[0].y, r[0].z, 0.0f); }
    EXMatrix &SetFromRotX(float a) { set_rotate_x(a); return *this; }
    EXMatrix &SetFromRotY(float a) { set_rotate_y(a); return *this; }
    EXMatrix &SetFromRotZ(float a) { set_rotate_z(a); return *this; }
    static EXMatrix RotMatrixY(float a) { EXMatrix m; m.set_rotate_y(a); return m; }
    static EXMatrix MakeIdentity() { EXMatrix m; m.set_identity(); return m; }
    EXMatrix &operator*=(const EXMatrix &m) { *(mat_44 *)this = mat_44::operator*(m); return *this; }
    EXMatrix operator*(const EXMatrix &m) { return EXMatrix(mat_44::operator*(m)); }
    EXMatrix Inverse() const { return EXMatrix(inverse()); }
    EXMatrix Inverse3() const { return EXMatrix(inverse3()); }
    EXMatrix FastInverse() const { return EXMatrix(fastinverse()); }
    EXMatrix Transpose() const { return EXMatrix(transpose()); }

    void GetRotation(EXQuaternion &q) const;
    void GetRotation(EXAngleAxis &aa) const;
    void GetRotationXYZ(EXVector &a) const;
    void GetRotationXZY(EXVector &a) const;
    void GetRotationYXZ(EXVector &a) const;
    void GetRotationYZX(EXVector &a) const;
    void GetRotationZXY(EXVector &a) const;
    void GetRotationZYX(EXVector &a) const;
    void GetRotation(EXVector &a, u8 order) const;
    float Determinant() const;
    void GetScale(float &sx, float &sy, float &sz) const;
    void Decompose(EXVector &scale, EXVector &rot, u8 order) const;
    void AlignToPlane(const EXVector &n);
    void AlignToDir(const EXVector &a, const EXVector &b, const EXVector &c);
    void AlignToUp(const EXVector &a, const EXVector &b, const EXVector &c);
    void LookAt(const EXVector &a, const EXVector &b, const EXVector &c, const EXVector &d);
    void GetRotationTo(const EXVector &a, const EXVector &b);
    void Scale(const EXVector &s);
private:
    float _det2x2(float a, float b, float c, float d) const { return a * d - b * c; }
    float _det3x3(float a1, float a2, float a3, float b1, float b2, float b3, float c1, float c2, float c3) const
    {
        return a1 * _det2x2(b2, b3, c2, c3) - b1 * _det2x2(a2, a3, c2, c3) + c1 * _det2x2(a2, a3, b2, b3);
    }
};

// Scale matrix (EXMatrix.obj ctors). TODO layout: assumed to add no fields.
class EXScaleMatrix : public EXMatrix {
public:
    EXScaleMatrix(const EXVector &s);
    EXScaleMatrix(float s);
    EXScaleMatrix(float sx, float sy, float sz);
};

inline EXVector &EXVector::operator*=(const EXMatrix &m)
{
    *(EXMaths::vec_xyzw *)this = (const EXMaths::mat_44 &)m * *this;
    return *this;
}
inline EXVector &EXVector::ApplyXForm(const EXMatrix &m)
{
    w = 1.0f;
    *this *= m;
    w = 0.0f;
    return *this;
}

// ---- appended by the item-layer agent (verified as [fi] copies in EXItem.obj) ----
__forceinline void EXMatrix::Scale(const EXVector &v)
{
    EXMaths::mat_44 s;
    s.set_scale(v);
    *(EXMaths::mat_44 *)this = mat_44::operator*(s);
}
inline EXMatrix operator*(const EXMatrix &a, const EXMatrix &b) { return EXMatrix(a.EXMaths::mat_44::operator*(b)); }
#endif
