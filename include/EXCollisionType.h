#ifndef EXCOLLISIONTYPE_H
#define EXCOLLISIONTYPE_H
#include "EXSphere.h"
#include "EXCapsule.h"
#include "EXLozenge.h"
#include "EXBox.h"

class EXTriangle;
namespace ex { class RGBA; }

// Tagged collision shape (EXCollision.obj), sizeof 0x40: the shape overlays the first 0x3c bytes.
class EXCollisionType {
public:
    enum ColType { COL_SPHERE = 0, COL_CAPSULE = 1, COL_LOZENGE = 2, COL_BOX = 3 };
    void SetType(ColType t) { m_type = t; }
    ColType Type() const { return m_type; }
    EXSphere &Sphere() { return *(EXSphere *)this; }
    EXCapsule &Capsule() { return *(EXCapsule *)this; }
    EXLozenge &Lozenge() { return *(EXLozenge *)this; }
    EXBox &Box() { return *(EXBox *)this; }
    const EXSphere &Sphere() const { return *(const EXSphere *)this; }
    const EXCapsule &Capsule() const { return *(const EXCapsule *)this; }
    const EXLozenge &Lozenge() const { return *(const EXLozenge *)this; }
    const EXBox &Box() const { return *(const EXBox *)this; }
    void Set(const EXSphere &s)
    {
        Sphere() = s;
        m_type = COL_SPHERE;
    }
    void Set(const EXCapsule &c)
    {
        Capsule() = c;
        m_type = COL_CAPSULE;
    }
    void Set(const EXBox &b)
    {
        Box() = b;
        m_type = COL_BOX;
    }
    void Debug_Draw(ex::RGBA c);
    void GetBoundsBox(EXBoundsBox &b, unsigned int flags) const;
    void ApplyXForm(const EXMatrix &m);
    float Distance(const EXTriangle &t, float &d, EXVector &a, EXVector &b) const;
    signed char Intersection(const EXTriangle &t, float &d, EXVector &a, EXVector &b) const;
    void NearestTo(const EXVector &p, EXVector &out) const;
    EXVector Origin() const;
    float Radius() const;
    signed char Intersection(const EXCollisionType &c, EXVector &a, EXVector &b, float &d) const;
    float CentreSqrDistance(const EXCollisionType &c, EXVector &a, EXVector &b) const;

    float m_shape[15];  // +0x00 EXSphere / EXCapsule / EXLozenge / EXBox
    ColType m_type;     // +0x3c
};
#endif
