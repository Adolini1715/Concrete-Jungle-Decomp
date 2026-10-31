#ifndef EXPLANE_H
#define EXPLANE_H
#include "EXLine.h"

// Plane A*x + B*y + C*z + D = 0 (EXPlane.obj). Inline shapes from Camera/Player/NavMesh copies.
class EXPlane {
public:
    enum PLANE_POS { PLANE_PARALLEL = 0, PLANE_COINCIDENT = 1, PLANE_INTERSECT = 2 }; // TODO: values unverified
    EXPlane() {}
    EXPlane(float a, float b, float c, float d) { A = a; B = b; C = c; D = d; }
    EXPlane(const EXPlane &p) { A = p.A; B = p.B; C = p.C; D = p.D; }
    EXPlane(const EXVector &a, const EXVector &b, const EXVector &c);
    EXPlane(const EXVector &p, const EXVector &n);
    EXPlane &operator=(const EXPlane &p) { set(p.A, p.B, p.C, p.D); return *this; }
    void set(float a, float b, float c, float d) { A = a; B = b; C = c; D = d; }
    float get_A() const { return A; }
    float get_B() const { return B; }
    float get_C() const { return C; }
    float get_D() const { return D; }
    EXVector Normal() const { return EXVector(A, B, C, 0.0f); }
    EXVector Point() const { return Normal() * D; }
    // appended by the nav agent ([fi] copy in XGeoEntity_NavMesh.obj): plane height at q, through v.
    float HeightTo(const EXVector &v, const EXVector &q) const
    {
        float b = B;
        float inv = b;
        if (b != 0.0f) inv = 1.0f / b;
        return (v.z - q.z) * (inv * C) + (v.x - q.x) * (inv * A) + v.y;
    }
    // appended by the player agent ([fi] copy in Player.obj): signed distance of v from a unit-normal plane.
    float DistanceToNormalised(const EXVector &v) const
    {
        EXMaths::vec_xyzw n = *(const EXMaths::vec_xyzw *)this;
        n.w = 0.0f;
        return n.Dot(v) + D;
    }
    s8 Intersection(float &t, const EXLineSeg &l) const;
    PLANE_POS Intersection(const EXPlane &p, EXLine &l) const;

    float A, B, C, D;
protected:
    static float m_rCalcTol;
};
#endif
