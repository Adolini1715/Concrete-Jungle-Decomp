#ifndef EXRECTANGLE_H
#define EXRECTANGLE_H
#include "EXVector.h"

// Parallelogram: origin plus two edge vectors (EXCollisionTypes.obj users), sizeof 0x30.
class EXRectangle {
public:
    EXVector &Origin() { return m_origin; }
    const EXVector &Origin() const { return m_origin; }
    EXVector &Edge0() { return m_edge0; }
    const EXVector &Edge0() const { return m_edge0; }
    EXVector &Edge1() { return m_edge1; }
    const EXVector &Edge1() const { return m_edge1; }

    EXVector m_origin;  // +0x00
    EXVector m_edge0;   // +0x10
    EXVector m_edge1;   // +0x20
};
#endif
