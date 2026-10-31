#ifndef EXBOX_H
#define EXBOX_H
#include "EXVector.h"
#include "EXArray.h"

class EXMatrix;
class EXLozenge;
struct EXBoundsBox;

// Oriented box (EXCollisionTypes.obj), sizeof 0x3c: centre, two stored unit axes (the third is
// their cross product) and the three half extents.
class EXBox {
public:
    EXBox() {}
    EXVector &Centre() { return m_centre; }
    const EXVector &Centre() const { return m_centre; }
    float &Extent(int i) { return m_extent[i]; }
    const float &Extent(int i) const { return m_extent[i]; }
    const float *Extents() const { return m_extent; }
    const EXVector Axis(int i) const
    {
        return i < 2 ? m_axis[i] : m_axis[0].Cross(m_axis[1]);
    }
    void SetAxis(int i, const EXVector &v)
    {
        if (i < 2) m_axis[i] = v;
    }
    float Radius() const;
    void Set(const EXLozenge &l);
    void ComputeVertices(EXFixedArray<EXMaths::vec_xyzw, 8> &v) const;
    void ApplyXForm(const EXMatrix &m);
    void GetBoundsBox(EXBoundsBox &b) const;

    EXVector m_centre;      // +0x00
    EXVector m_axis[2];     // +0x10
    float m_extent[3];      // +0x30
};

// 2D oriented box (EXCollisionTypes.obj), sizeof 0x20.
class EXBox2 {
public:
    void ComputeVertices(EXFixedArray<EXVector2, 4> &v) const;

    EXVector2 m_centre;     // +0x00
    EXVector2 m_axis[2];    // +0x08
    float m_extent[2];      // +0x18
};
#endif
