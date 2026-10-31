#ifndef EXLOZENGE_H
#define EXLOZENGE_H
#include "EXRectangle.h"

class EXMatrix;
struct EXBoundsBox;

// Swept sphere over a rectangle (EXCollisionTypes.obj), sizeof 0x34.
class EXLozenge {
public:
    EXVector &Origin() { return m_rect.m_origin; }
    const EXVector &Origin() const { return m_rect.m_origin; }
    EXVector &Edge0() { return m_rect.m_edge0; }
    const EXVector &Edge0() const { return m_rect.m_edge0; }
    EXVector &Edge1() { return m_rect.m_edge1; }
    const EXVector &Edge1() const { return m_rect.m_edge1; }
    float &Radius() { return m_radius; }
    const float &Radius() const { return m_radius; }
    EXRectangle &Rectangle() { return m_rect; }
    const EXRectangle &Rectangle() const { return m_rect; }
    void ApplyXForm(const EXMatrix &m);
    void GetBoundsBox(EXBoundsBox &b) const;

    EXRectangle m_rect; // +0x00
    float m_radius;     // +0x30
};
#endif
