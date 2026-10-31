#ifndef EXTRIANGLE_H
#define EXTRIANGLE_H
#include "EXMatrix.h"
#include "EXArray.h"
#include "EXVector.h"

struct EXBoundsBox;

// Three vertices (EXVector.obj). TODO layout: only the vertex array is known.
class EXTriangle {
public:
    EXTriangle() {}
    // [fi] in XBCollision.obj / Actor.obj (shapes read from the disassembly, not checked there).
    EXTriangle(const EXVector &a, const EXVector &b, const EXVector &c) { Set(a, b, c); }
    void Set(const EXVector &a, const EXVector &b, const EXVector &c)
    {
        m_v[0].set(a);
        Edge0Set(b - a);
        Edge1Set(c - a);
    }
    void ApplyXForm(const EXMatrix &m);
    void GetBoundsBox(EXBoundsBox &b) const;
    float MinPointSquared(const EXVector &p, float &s, float &t) const;
    EXVector UnitNormal() const;
    // Edge setters and DistanceTo ([fi] in EXGeoMap.obj). m_v: origin, edge 0, edge 1.
    const EXVector &Origin() const { return (const EXVector &)m_v[0]; }
    const EXVector &Edge(int i) const  // [fi] copy in EXCollision.obj
    {
        if (i == 0) return (const EXVector &)m_v[1];
        return (const EXVector &)m_v[2];
    }
    const EXVector &Edge0() const { return (const EXVector &)m_v[1]; }  // [fi] copy in EXDistance.obj
    const EXVector &Edge1() const { return (const EXVector &)m_v[2]; }  // [fi] copy in EXDistance.obj
    void Edge0Set(const EXVector &v) { m_v[1].set(v); }
    void Edge1Set(const EXVector &v) { m_v[2].set(v); }
    void EdgeSet(int i, const EXVector &v)
    {
        if (i == 0) Edge0Set(v);
        else Edge1Set(v);
    }
    __forceinline float DistanceTo(const EXVector &p, float maxDist, EXVector &nearest) const
    {
        float t, s;
        float d = MinPointSquared(p, s, t);
        if (d > Sqr(maxDist)) return maxDist + 1.0f;
        EXVector r = (m_v[1] * s + m_v[2] * t) + m_v[0];
        nearest.Set(r.x, r.y, r.z, 0.0f);
        return EXSqrt(d);
    }
    EXFixedArray<EXMaths::vec_xyzw, 3> m_v;
};
#endif
