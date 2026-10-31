#ifndef EXDISTANCE_H
#define EXDISTANCE_H
#include "EXVector.h"

class EXLine;
class EXRay;
class EXLineSeg;
class EXBox;
class EXRectangle;
class EXTriangle;

// Squared distances between primitives (EXDistance.obj). The optional float pointers receive the
// parameters of the closest points.
float EXSqrDistance(const EXLine &a, const EXBox &b, float *t = 0, float *b0 = 0, float *b1 = 0, float *b2 = 0);
float EXSqrDistance(const EXLine &a, const EXLine &b, float *s = 0, float *t = 0);
float EXSqrDistance(const EXLine &a, const EXRay &b, float *s = 0, float *t = 0);
float EXSqrDistance(const EXLine &a, const EXLineSeg &b, float *s = 0, float *t = 0);
float EXSqrDistance(const EXRay &a, const EXRay &b, float *s = 0, float *t = 0);
float EXSqrDistance(const EXRay &a, const EXLineSeg &b, float *s = 0, float *t = 0);
float EXSqrDistance(const EXLineSeg &a, const EXLineSeg &b, float *s = 0, float *t = 0);
float EXSqrDistance(const EXLine &a, const EXRectangle &b, float *t = 0, float *s0 = 0, float *s1 = 0);
float EXSqrDistance(const EXLine &a, const EXTriangle &b, float *t = 0, float *s0 = 0, float *s1 = 0);
float EXSqrDistance(const EXMaths::vec_xyzw &p, const EXBox &b, float *b0 = 0, float *b1 = 0, float *b2 = 0);
float EXSqrDistance(const EXMaths::vec_xyzw &p, const EXLine &l, float *t = 0);
float EXSqrDistance(const EXMaths::vec_xyzw &p, const EXRay &l, float *t = 0);
float EXSqrDistance(const EXMaths::vec_xyzw &p, const EXLineSeg &l, float *t = 0);
float EXSqrDistance(const EXMaths::vec_xyzw &p, const EXRectangle &r, float *s0 = 0, float *s1 = 0);
float EXSqrDistance(const EXMaths::vec_xyzw &p, const EXTriangle &t, float *s0 = 0, float *s1 = 0);
float EXSqrDistance(const EXLineSeg &a, const EXBox &b, float *t = 0, float *b0 = 0, float *b1 = 0, float *b2 = 0);
float EXSqrDistance(const EXRay &a, const EXRectangle &b, float *t = 0, float *s0 = 0, float *s1 = 0);
float EXSqrDistance(const EXLineSeg &a, const EXRectangle &b, float *t = 0, float *s0 = 0, float *s1 = 0);
float EXSqrDistance(const EXLineSeg &a, const EXTriangle &b, float *t = 0, float *s0 = 0, float *s1 = 0);
float EXSqrDistance(const EXTriangle &a, const EXRectangle &b, float *s0 = 0, float *s1 = 0, float *t0 = 0, float *t1 = 0);
float EXSqrDistance(const EXRectangle &a, const EXRectangle &b, float *s0 = 0, float *s1 = 0, float *t0 = 0, float *t1 = 0);
float EXSqrDistance(const EXTriangle &a, const EXTriangle &b, float *s0 = 0, float *s1 = 0, float *t0 = 0, float *t1 = 0);

// Distances: clamped square roots of the above (inlines, emitted as [fi] copies where used).
inline float EXDistance(const EXLine &a, const EXLine &b, float *s = 0, float *t = 0) { return EXSqrt(EXSqrDistance(a, b, s, t)); }
inline float EXDistance(const EXLineSeg &a, const EXLineSeg &b, float *s = 0, float *t = 0) { return EXSqrt(EXSqrDistance(a, b, s, t)); }
inline float EXDistance(const EXLineSeg &a, const EXTriangle &b, float *t = 0, float *s0 = 0, float *s1 = 0) { return EXSqrt(EXSqrDistance(a, b, t, s0, s1)); }
inline float EXDistance(const EXMaths::vec_xyzw &p, const EXLineSeg &l, float *t = 0) { return EXSqrt(EXSqrDistance(p, l, t)); }
inline float EXDistance(const EXMaths::vec_xyzw &p, const EXRectangle &r, float *s0 = 0, float *s1 = 0) { return EXSqrt(EXSqrDistance(p, r, s0, s1)); }
inline float EXDistance(const EXMaths::vec_xyzw &p, const EXTriangle &t, float *s0 = 0, float *s1 = 0) { return EXSqrt(EXSqrDistance(p, t, s0, s1)); }
inline float EXDistance(const EXTriangle &a, const EXRectangle &b, float *s0 = 0, float *s1 = 0, float *t0 = 0, float *t1 = 0)
{
    return EXSqrt(EXSqrDistance(a, b, s0, s1, t0, t1));
}
#endif
