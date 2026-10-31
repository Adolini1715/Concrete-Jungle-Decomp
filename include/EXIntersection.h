#ifndef EXINTERSECTION_H
#define EXINTERSECTION_H
#include "EXVector.h"

class EXLine;
class EXRay;
class EXLineSeg;
class EXSphere;
class EXCapsule;
class EXLozenge;
class EXBox;
class EXTriangle;

// Boolean overlap tests (EXIntersection.obj).
signed char EXIntersection(const EXLine &l, const EXCapsule &c);
signed char EXIntersection(const EXLineSeg &l, const EXCapsule &c);
signed char EXIntersection(const EXRay &l, const EXCapsule &c);
signed char EXIntersection(const EXCapsule &a, const EXCapsule &b);
signed char EXIntersection(const EXCapsule &a, const EXLozenge &b);
signed char EXIntersection(const EXLozenge &a, const EXLozenge &b);
signed char EXIntersection(const EXRay &l, const EXLozenge &z);
signed char EXIntersection(const EXLine &l, const EXLozenge &z);
signed char EXIntersection(const EXLineSeg &l, const EXLozenge &z);
signed char EXIntersection(const EXLine &l, const EXSphere &s);
signed char EXIntersection(const EXLineSeg &l, const EXSphere &s);
signed char EXIntersection(const EXRay &l, const EXSphere &s);
signed char EXIntersection(const EXSphere &a, const EXSphere &b);
signed char EXIntersection(const EXSphere &s, const EXCapsule &c);
signed char EXIntersection(const EXSphere &s, const EXLozenge &z);
signed char EXIntersection(const EXLineSeg &l, const EXBox &b);

// Intersection points / contact queries (EXIntersection.obj).
unsigned int EXFindIntersection(const EXLineSeg &l, const EXSphere &s, EXVector *const pts);
unsigned int EXFindIntersection(const EXRay &l, const EXSphere &s, EXVector *const pts);
unsigned int EXFindIntersection(const EXLine &l, const EXSphere &s, EXVector *const pts);
signed char EXFindIntersection(const EXRay &l, const EXSphere &s, EXVector &p);
int EXFindIntersection(const EXVector &o, const EXVector &d, const EXCapsule &c, float *const t);
unsigned int EXFindIntersection(const EXLineSeg &l, const EXCapsule &c, EXVector *const pts);
signed char EXFindIntersection(const EXSphere &a, const EXSphere &b, float dt, const EXVector &va, const EXVector &vb,
                               float &t, EXVector &p);
unsigned int EXFindIntersection(const EXLineSeg &l, const EXBox &b, EXVector *const pts);
unsigned int EXFindIntersection(const EXVector &o, const EXVector &d, const EXBox &b, float *const t);
signed char EXFindIntersection(const EXSphere &a, const EXSphere &b, EXVector &p, EXVector &n, float &d);
signed char EXFindIntersection(const EXSphere &a, const EXCapsule &b, EXVector &p, EXVector &n, float &d);
signed char EXFindIntersection(const EXSphere &a, const EXLozenge &b, EXVector &p, EXVector &n, float &d);
signed char EXFindIntersection(const EXCapsule &a, const EXCapsule &b, EXVector &p, EXVector &n, float &d);
signed char EXFindIntersection(const EXCapsule &a, const EXLozenge &b, EXVector &p, EXVector &n, float &d);
signed char EXFindIntersection(const EXLozenge &a, const EXLozenge &b, EXVector &p, EXVector &n, float &d);
signed char EXFindIntersection(const EXSphere &a, const EXBox &b, EXVector &p, EXVector &n, float &d);
signed char EXFindIntersection(const EXBox &a, const EXBox &b, EXVector &p, EXVector &n, float &d);
unsigned int EXFindIntersection(const EXLineSeg &l, const EXLozenge &z, EXVector *const pts);
signed char EXFindIntersection(const EXCapsule &a, const EXBox &b, EXVector &p, EXVector &n, float &d);
signed char EXFindIntersection(const EXBox &a, const EXTriangle &b, EXVector &p, EXVector &n, float &d);

// Box/triangle separating-axis helpers (EXIntersection.obj, external linkage).
// Best separating axis found so far by the box/triangle test: the unit axis, the overlap depth
// along it and which candidate axis it was.
struct EXSeparatingAxisTemp {
    EXSeparatingAxisTemp() {}
    EXVector m_Axis;   // +0x00
    float m_Depth;     // +0x10
    int m_AxisId;      // +0x14
    int m_FailAxis;    // +0x18 the axis that separated (-1 when none did)
};
signed char SeparatingAxisTestNormal(float a, float b, const EXVector &n, int axis, EXSeparatingAxisTemp &t);
signed char SeparatingAxisTestFace(float a, float b, float c, float d, float e, EXVector n, int axis, EXSeparatingAxisTemp &t);
signed char SeparatingAxisTestEdge(float a, float b, float c, float d, EXVector n, int axis, EXSeparatingAxisTemp &t);
signed char TestSeparatingAxes(const EXBox &b, const EXTriangle &t, EXSeparatingAxisTemp &s);
void ClipPolyToPlane(EXVector *const in, int n, EXVector *const out, int &nOut, const EXVector &plane);
float FindContactPoint(const EXBox &b, const EXTriangle &t, EXSeparatingAxisTemp &s, EXVector &p);
#endif
