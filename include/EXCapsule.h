#ifndef EXCAPSULE_H
#define EXCAPSULE_H
#include "EXLine.h"

class EXMatrix;
struct EXBoundsBox;

// Swept sphere along a segment (EXCollisionTypes.obj), sizeof 0x24.
class EXCapsule {
public:
    EXCapsule() {}
    EXVector &Origin() { return m_seg.m_Origin; }
    const EXVector &Origin() const { return m_seg.m_Origin; }
    EXVector &Direction() { return m_seg.m_Direction; }
    const EXVector &Direction() const { return m_seg.m_Direction; }
    float &Radius() { return m_radius; }
    const float &Radius() const { return m_radius; }
    void SetRadius(float r) { m_radius = r; }
    EXLineSeg &Segment() { return m_seg; }
    const EXLineSeg &Segment() const { return m_seg; }
    void Set(const EXVector &o, const EXVector &d, float r)
    {
        m_seg.m_Origin = o;
        m_seg.m_Direction = d;
        m_radius = r;
    }
    void ApplyXForm(const EXMatrix &m);
    void GetBoundsBox(EXBoundsBox &b) const;

    EXLineSeg m_seg;    // +0x00
    float m_radius;     // +0x20
};
#endif
