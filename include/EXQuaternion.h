#ifndef EXQUATERNION_H
#define EXQUATERNION_H
#include "EXMatrix.h"

// Axis (x, y, z) and angle (w).
class EXAngleAxis : public EXMaths::vec_xyzw {
public:
    EXAngleAxis() {}
    void SetRotation(const EXVector &a, u8 order);
    void SetRotation(const EXQuaternion &q);
    void SetRotation(const EXMatrix &m);
};

// The engine quaternion: quat_xyzw with the EX API on top.
class EXQuaternion : public EXMaths::quat_xyzw {
public:
    EXQuaternion() {}
    EXQuaternion(float x_, float y_, float z_, float w_) { x = x_; y = y_; z = z_; w = w_; }
    EXQuaternion(const EXMaths::vec_xyzw &v) { set(v); }
    EXQuaternion(const EXQuaternion &q) { set(q); }
    EXQuaternion(const EXMatrix &m) { SetRotation(m); }
    EXQuaternion &operator=(const EXQuaternion &q) { set(q); return *this; }
    void Set(float x_, float y_, float z_, float w_) { x = x_; y = y_; z = z_; w = w_; }
    void SetRotation(const EXAngleAxis &aa);
    void SetRotation(const EXMatrix &m);
    void FromSpherical(float a, float b, float c);
    EXQuaternion Inverse() const;
    void MakeQuaternion(EXVector a, EXVector b);
    float AngleDiff(const EXQuaternion &q) const;
    EXQuaternion SlerpBy(const EXQuaternion &q, float t, float &out) const;
    void GetRotation(EXAngleAxis &aa) const;
    void GetRotation(EXMatrix &m) const;
    void GetRotation(EXVector &a, u8 order) const;
};
#endif
