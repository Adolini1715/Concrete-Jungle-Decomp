#ifndef EXLINE_H
#define EXLINE_H
#include "EXVector.h"

class EXMatrix;

// Infinite line: origin + t * direction.
class EXLine {
public:
    EXLine() {}
    EXLine(const EXVector &o, const EXVector &d) : m_Origin(o), m_Direction(d) {}
    EXVector &Origin() { return m_Origin; }
    const EXVector &Origin() const { return m_Origin; }
    EXVector &Direction() { return m_Direction; }
    const EXVector &Direction() const { return m_Direction; }
    void ApplyXForm(const EXMatrix &m);
    EXVector m_Origin;
    EXVector m_Direction;
};

// Half line from the origin.
class EXRay {
public:
    EXRay() {}
    EXRay(const EXVector &o, const EXVector &d) : m_Origin(o), m_Direction(d) {}
    EXVector &Origin() { return m_Origin; }
    const EXVector &Origin() const { return m_Origin; }
    EXVector &Direction() { return m_Direction; }
    const EXVector &Direction() const { return m_Direction; }
    void Reverse() { m_Direction = -m_Direction; }
    void ApplyXForm(const EXMatrix &m);
    EXVector m_Origin;
    EXVector m_Direction;
};

// Segment from origin to origin + direction.
class EXLineSeg {
public:
    enum LINE_POS { LINE_START = 1, LINE_MIDDLE = 2, LINE_END = 3 };
    EXLineSeg() {}
    EXLineSeg(const EXVector &o, const EXVector &d) : m_Origin(o), m_Direction(d) {}
    EXLineSeg(const EXLineSeg &l) : m_Origin(l.m_Origin), m_Direction(l.m_Direction) {}
    EXLineSeg &operator=(const EXLineSeg &l) { m_Origin = l.m_Origin; m_Direction = l.m_Direction; return *this; }
    EXVector &Origin() { return m_Origin; }
    const EXVector &Origin() const { return m_Origin; }
    EXVector &Direction() { return m_Direction; }
    const EXVector &Direction() const { return m_Direction; }
    LINE_POS NearestTo(const EXVector &p, EXVector &out) const;
    s8 PtOnLine(const EXVector &p) const;
    void ApplyXForm(const EXMatrix &m);
    EXVector m_Origin;
    EXVector m_Direction;
};
#endif
