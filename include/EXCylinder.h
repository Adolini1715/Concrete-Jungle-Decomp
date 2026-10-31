#ifndef EXCYLINDER_H
#define EXCYLINDER_H
#include "EXVector.h"

class EXMatrix;
struct EXBoundsBox;

// Cylinder: centre, half-axis vector, radius (EXCollisionTypes.obj), sizeof 0x24.
class EXCylinder {
public:
    EXVector &Centre() { return m_centre; }
    EXVector &Direction() { return m_dir; }
    float &Radius() { return m_radius; }
    const EXVector &Centre() const { return m_centre; }
    const EXVector &Direction() const { return m_dir; }
    const float &Radius() const { return m_radius; }
    void ApplyXForm(const EXMatrix &m);
    void GetBoundsBox(EXBoundsBox &b) const;

    EXVector m_centre;  // +0x00
    EXVector m_dir;     // +0x10
    float m_radius;     // +0x20
};
#endif
