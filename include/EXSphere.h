#ifndef EXSPHERE_H
#define EXSPHERE_H
#include "EXVector.h"

class EXMatrix;
struct EXBoundsBox;

// Sphere collision shape (EXCollisionTypes.obj), sizeof 0x14.
class EXSphere {
public:
    EXSphere() {}
    void Set(const EXVector &c, float r)
    {
        m_centre = c;
        m_radius = r;
    }
    EXVector &Centre() { return m_centre; }
    const EXVector &Centre() const { return m_centre; }
    float &Radius() { return m_radius; }
    const float &Radius() const { return m_radius; }
    void SetRadius(float r) { m_radius = r; }
    void SetCentre(const EXVector &c) { m_centre = c; }
    void GetBoundsBox(EXBoundsBox &b) const;
    void ApplyXForm(const EXMatrix &m);

    EXVector m_centre;  // +0x00
    float m_radius;     // +0x10
};
#endif
