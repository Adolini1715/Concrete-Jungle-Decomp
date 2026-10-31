#include "EXIntersection.h"
#include "EXDistance.h"
#include "EXCollisionType.h"
#include "EXTriangle.h"
#include "EXMatrix.h"
#include <float.h>
#include <string.h>

signed char EXIntersection(const EXLine &l, const EXCapsule &c)
{
    float d = EXSqrDistance(l, c.Segment());
    if (d <= c.Radius() * c.Radius()) return 1;
    return 0;
}

signed char EXIntersection(const EXLineSeg &l, const EXCapsule &c)
{
    float d = EXSqrDistance(l, c.Segment());
    if (d <= c.Radius() * c.Radius()) return 1;
    return 0;
}

signed char EXIntersection(const EXRay &l, const EXCapsule &c)
{
    float d = EXSqrDistance(l, c.Segment());
    if (d <= c.Radius() * c.Radius()) return 1;
    return 0;
}

signed char EXIntersection(const EXCapsule &a, const EXCapsule &b)
{
    float r = a.Radius() + b.Radius();
    if (EXSqrDistance(a.Segment(), b.Segment()) <= r * r) return 1;
    return 0;
}

signed char EXIntersection(const EXCapsule &a, const EXLozenge &b)
{
    float r = a.Radius() + b.Radius();
    if (EXSqrDistance(a.Segment(), b.Rectangle()) <= r * r) return 1;
    return 0;
}

signed char EXIntersection(const EXLozenge &a, const EXLozenge &b)
{
    float r = a.Radius() + b.Radius();
    if (EXSqrDistance(a.Rectangle(), b.Rectangle()) <= r * r) return 1;
    return 0;
}

signed char EXIntersection(const EXRay &l, const EXLozenge &z)
{
    float d = EXSqrDistance(l, z.Rectangle());
    if (d <= z.Radius() * z.Radius()) return 1;
    return 0;
}

signed char EXIntersection(const EXLine &l, const EXLozenge &z)
{
    float d = EXSqrDistance(l, z.Rectangle());
    if (d <= z.Radius() * z.Radius()) return 1;
    return 0;
}

signed char EXIntersection(const EXLineSeg &l, const EXLozenge &z)
{
    float d = EXSqrDistance(l, z.Rectangle());
    if (d <= z.Radius() * z.Radius()) return 1;
    return 0;
}

signed char EXIntersection(const EXLine &l, const EXSphere &s)
{
    float d = EXSqrDistance((EXMaths::vec_xyzw)s.Centre(), l);
    if (d <= s.Radius() * s.Radius()) return 1;
    return 0;
}

signed char EXIntersection(const EXLineSeg &l, const EXSphere &s)
{
    float d = EXSqrDistance((EXMaths::vec_xyzw)s.Centre(), l);
    if (d <= s.Radius() * s.Radius()) return 1;
    return 0;
}

signed char EXIntersection(const EXRay &l, const EXSphere &s)
{
    float d = EXSqrDistance((EXMaths::vec_xyzw)s.Centre(), l);
    if (d <= s.Radius() * s.Radius()) return 1;
    return 0;
}

signed char EXIntersection(const EXSphere &a, const EXSphere &b)
{
    float r = a.Radius() + b.Radius();
    EXMaths::vec_xyzw d = a.Centre() - b.Centre();
    if (EXVector(d).LengthXYZSquared() <= r * r) return 1;
    return 0;
}

signed char EXIntersection(const EXSphere &s, const EXCapsule &c)
{
    float r = s.Radius() + c.Radius();
    if (EXSqrDistance((EXMaths::vec_xyzw)s.Centre(), c.Segment()) <= r * r) return 1;
    return 0;
}

signed char EXIntersection(const EXSphere &s, const EXLozenge &z)
{
    float r = z.Radius() + s.Radius();
    if (EXSqrDistance((EXMaths::vec_xyzw)s.Centre(), z.Rectangle()) <= r * r) return 1;
    return 0;
}

// Liang-Barsky clip of the parameter interval [t0, t1] against one slab side.
static signed char Clip(float denom, float numer, float &t0, float &t1)
{
    if (denom > 0.0f) {
        if (numer > denom * t1) return 0;
        if (numer > denom * t0) t0 = numer / denom;
        return 1;
    } else if (denom < 0.0f) {
        if (numer > denom * t0) return 0;
        if (numer > denom * t1) t1 = numer / denom;
        return 1;
    } else {
        return numer <= 0.0f;
    }
}

// Clip the quad p (four 2D points) against the rectangle |x| < h[0], |y| < h[1], one side at a
// time, writing the clipped polygon (at most eight points) to ret. Returns the point count.
static int EXIntersectionRectQuad(const float *const h, float *const p, float *const ret)
{
    int nq = 4, nr = 0;
    float buffer[16];
    float *q = p;
    float *r = ret;
    for (int dir = 0; dir <= 1; dir++) {
        for (int sign = -1; sign <= 1; sign += 2) {
            float *pq = q;
            float *pr = r;
            nr = 0;
            for (int i = nq; i > 0; i--) {
                if (sign * pq[dir] < h[dir]) {
                    // Inside this side: keep the point.
                    pr[0] = pq[0];
                    pr[1] = pq[1];
                    pr += 2;
                    nr++;
                    if (nr & 8) {
                        q = r;
                        goto done;
                    }
                }
                float *next = i > 1 ? pq + 2 : q;
                if ((sign * pq[dir] < h[dir]) ^ (sign * next[dir] < h[dir])) {
                    // The edge to the next point crosses this side: add the crossing.
                    pr[1 - dir] = (next[1 - dir] - pq[1 - dir]) / (next[dir] - pq[dir]) * (sign * h[dir] - pq[dir]) +
                                  pq[1 - dir];
                    pr[dir] = sign * h[dir];
                    pr += 2;
                    nr++;
                    if (nr & 8) {
                        q = r;
                        goto done;
                    }
                }
                pq += 2;
            }
            q = r;
            r = q == ret ? buffer : ret;
            nq = nr;
        }
    }
done:
    if (q != ret) memcpy(ret, q, nr * 2 * sizeof(float));
    return nr;
}

// Box/triangle separating axis tests. Each projects onto one candidate axis: a and b (or the
// triangle extremes) against the box radius; on overlap the shallowest axis so far is kept.
signed char SeparatingAxisTestNormal(float a, float b, const EXVector &n, int axis, EXSeparatingAxisTemp &t)
{
    if (a + b < 0.0f) return 0;
    float len = n.Length();
    if (len > 0.0f) {
        float inv = 1.0f / len;
        if (inv * (a + b) < t.m_Depth) {
            t.m_Axis.x = -n.x * inv;
            t.m_Axis.y = -n.y * inv;
            t.m_Axis.z = -n.z * inv;
            t.m_AxisId = axis;
            t.m_Depth = inv * (a + b);
        }
    }
    return 1;
}

signed char SeparatingAxisTestFace(float p0, float p1, float p2, float r, float unused, EXVector n, int axis,
                                   EXSeparatingAxisTemp &t)
{
    float mn = p0 < p1 ? (p0 < p2 ? p0 : p2) : (p1 < p2 ? p1 : p2);
    float mx = p0 > p1 ? (p0 > p2 ? p0 : p2) : (p1 > p2 ? p1 : p2);
    float hi = mx + r;
    float lo = r - mn;
    if (lo < 0.0f || hi < 0.0f) return 0;
    if (hi < lo) {
        n.x = -n.x;
        n.y = -n.y;
        n.z = -n.z;
        lo = hi;
    }
    if (lo < t.m_Depth) {
        t.m_Axis.x = n.x;
        t.m_Axis.y = n.y;
        t.m_Axis.z = n.z;
        t.m_AxisId = axis;
        t.m_Depth = lo;
    }
    return 1;
}

signed char SeparatingAxisTestEdge(float p0, float p1, float r, float unused, EXVector n, int axis,
                                   EXSeparatingAxisTemp &t)
{
    float mn, mx;
    if (p0 < p1) {
        mn = p0;
        mx = p1;
    } else {
        mn = p1;
        mx = p0;
    }
    float lo = r - mn;
    float hi = mx + r;
    if (lo < 0.0f || hi < 0.0f) return 0;
    if (hi < lo) {
        n.x = -n.x;
        n.y = -n.y;
        n.z = -n.z;
        lo = hi;
    }
    float len = n.Length();
    if (len > 0.0f) {
        float inv = 1.0f / len;
        // Edge axes are biased against (x1.5) in favour of face axes.
        if (inv * lo * 1.5f < t.m_Depth) {
            t.m_Axis.x = n.x * inv;
            t.m_Axis.y = n.y * inv;
            t.m_Axis.z = n.z * inv;
            t.m_AxisId = axis;
            t.m_Depth = inv * lo;
        }
    }
    return 1;
}

signed char EXFindIntersection(const EXBox &box, const EXTriangle &tri, EXVector &n, EXVector &p, float &d)
{
    EXSeparatingAxisTemp t;
    if (!TestSeparatingAxes(box, tri, t)) {
        d = -FLT_MAX;
        return 0;
    }
    if (t.m_AxisId == 0) {
        d = -FLT_MAX;
        return 0;
    }
    d = FindContactPoint(box, tri, t, p);
    n = p - t.m_Axis;
    return 1;
}

// Line against an origin-centred box with the given half extents: clip the parameter interval
// [t0, t1] by the six slab planes. True when something is left and the interval was clipped.
static signed char FindIntersection(const EXMaths::vec_xyzw &o, const EXMaths::vec_xyzw &d, const float *const ext,
                                    float &t0, float &t1)
{
    float save0 = t0, save1 = t1;
    signed char notClipped = Clip(+d.x, -o.x - ext[0], t0, t1) && Clip(-d.x, +o.x - ext[0], t0, t1) &&
                             Clip(+d.y, -o.y - ext[1], t0, t1) && Clip(-d.y, +o.y - ext[1], t0, t1) &&
                             Clip(+d.z, -o.z - ext[2], t0, t1) && Clip(-d.z, +o.z - ext[2], t0, t1);
    return notClipped && (t0 != save0 || t1 != save1);
}

signed char EXFindIntersection(const EXRay &ray, const EXSphere &s, EXVector &p)
{
    EXVector pts[2];
    unsigned int n = EXFindIntersection(ray, s, pts);
    if (n == 0) return 0;
    int i = 0;
    if (n == 2) {
        EXVector d0 = pts[0] - ray.Origin();
        EXVector d1 = pts[1] - ray.Origin();
        if (d1.LengthXYZSquared() < d0.LengthXYZSquared()) i = 1;
    }
    p = pts[i];
    return 1;
}

// Sutherland-Hodgman: clip the polygon in[n] against the plane (keeping the side where
// p.n + w >= 0) into out, counting the result in nOut.
void ClipPolyToPlane(EXVector *const in, int n, EXVector *const out, int &nOut, const EXVector &plane)
{
    nOut = 0;
    for (int i = 0, j = n - 1; i < n; j = i, i++) {
        const EXVector &a = in[j];
        const EXVector &b = in[i];
        float da = a.x * plane.x + a.y * plane.y + a.z * plane.z + plane.w;
        float db = b.x * plane.x + b.y * plane.y + b.z * plane.z + plane.w;
        if (da >= 0.0f) {
            out[nOut].x = a.x;
            out[nOut].y = a.y;
            out[nOut].z = a.z;
            nOut++;
        }
        if ((da > 0.0f && db < 0.0f) || (da < 0.0f && db > 0.0f)) {
            float den = da - db;
            out[nOut].x = a.x - (a.x - b.x) * da / den;
            out[nOut].y = a.y - (a.y - b.y) * da / den;
            out[nOut].z = a.z - (a.z - b.z) * da / den;
            nOut++;
        }
    }
}

// Line/ray/segment against sphere: roots of |o + t d - c|^2 = r^2, clipped to the primitive's
// parameter range. Returns the number of points written.
#define EXSPHERE_QUADRATIC(L, S)                                                                   \
    EXVector diff = L.Origin() - S.Centre();                                                       \
    float a = L.Direction().LengthXYZSquared();                                                    \
    float b = diff.DotXYZ(L.Direction());                                                          \
    float c = diff.LengthXYZSquared() - S.Radius() * S.Radius();                                   \
    float discr = b * b - c * a;

unsigned int EXFindIntersection(const EXLineSeg &l, const EXSphere &s, EXVector *const pts)
{
    EXSPHERE_QUADRATIC(l, s)
    if (discr < 0.0f) return 0;
    if (discr > 0.0f) {
        float root = EXSqrt(discr);
        float inv = a != 0.0f ? 1.0f / a : 0.0f;
        float t0 = (-b - root) * inv;
        float t1 = (root - b) * inv;
        if (t0 > 1.0f) return 0;
        if (t1 < 0.0f) return 0;
        if (t0 < 0.0f) {
            pts[0] = l.Origin() + l.Direction() * t1;
            return 1;
        }
        pts[0] = l.Origin() + l.Direction() * t0;
        if (t1 <= 1.0f) {
            pts[1] = l.Origin() + l.Direction() * t1;
            return 2;
        }
        return 1;
    }
    float t = -(a != 0.0f ? b / a : 0.0f);
    if (t < 0.0f) return 0;
    if (t > 1.0f) return 0;
    pts[0] = l.Origin() + l.Direction() * t;
    return 1;
}

unsigned int EXFindIntersection(const EXRay &l, const EXSphere &s, EXVector *const pts)
{
    EXSPHERE_QUADRATIC(l, s)
    if (discr < 0.0f) return 0;
    if (discr > 0.0f) {
        float root = EXSqrt(discr);
        float inv = a != 0.0f ? 1.0f / a : 0.0f;
        float t0 = (-b - root) * inv;
        float t1 = (root - b) * inv;
        if (t0 >= 0.0f) {
            pts[0] = l.Origin() + l.Direction() * t0;
            pts[1] = l.Origin() + l.Direction() * t1;
            return 2;
        }
        if (t1 < 0.0f) return 0;
        pts[0] = l.Origin() + l.Direction() * t1;
        return 1;
    }
    float t = -(a != 0.0f ? b / a : 0.0f);
    if (t < 0.0f) return 0;
    pts[0] = l.Origin() + l.Direction() * t;
    return 1;
}

unsigned int EXFindIntersection(const EXLine &l, const EXSphere &s, EXVector *const pts)
{
    EXSPHERE_QUADRATIC(l, s)
    if (discr < 0.0f) return 0;
    if (discr > 0.0f) {
        float inv = a != 0.0f ? 1.0f / a : 0.0f;
        float t0 = (-b - EXSqrt(discr)) * inv;
        pts[0] = l.Origin() + l.Direction() * t0;
        float t1 = (EXSqrt(discr) - b) * inv;
        pts[1] = l.Origin() + l.Direction() * t1;
        return 2;
    }
    float t = -(a != 0.0f ? b / a : 0.0f);
    pts[0] = l.Origin() + l.Direction() * t;
    return 1;
}

// Overlapping spheres: the deepest point of each inside the other along the centre line, and the
// penetration depth in d (-9999 when apart).
signed char EXFindIntersection(const EXSphere &a, const EXSphere &b, EXVector &pa, EXVector &pb, float &d)
{
    EXVector diff = a.Centre() - b.Centre();
    d = diff.LengthXYZSquared();
    float r = b.Radius() + a.Radius();
    if (r * r < d) {
        d = -9999.0f;
        return 0;
    }
    if (d < 0.0f) d = 0.0f;
    d = EXSqrt(d);
    EXVector n = diff * (1.0f / d);
    pa = a.Centre() - n * a.Radius();
    pb = b.Centre() + n * b.Radius();
    d = r - d;
    return 1;
}

// Line (origin o, direction dir) against box: the entry and exit parameters in t[0], t[1].
unsigned int EXFindIntersection(const EXVector &o, const EXVector &dir, const EXBox &box, float *const t)
{
    EXVector axis2 = box.Axis(2);
    EXVector diff = o - box.Centre();
    EXVector pnt(diff.DotXYZ(box.Axis(0)), diff.DotXYZ(box.Axis(1)), diff.DotXYZ(axis2), 0.0f);
    EXVector d(dir.DotXYZ(box.Axis(0)), dir.DotXYZ(box.Axis(1)), dir.DotXYZ(axis2), 0.0f);
    float t0 = -FLT_MAX, t1 = FLT_MAX;
    signed char hit = FindIntersection(pnt, d, box.Extents(), t0, t1);
    t[0] = t0;
    t[1] = t1;
    return hit ? 2 : 0;
}

// Segment against box, separating axis test: the three box axes, then the segment direction crossed
// with each box axis.
signed char EXIntersection(const EXLineSeg &seg, const EXBox &box)
{
    EXVector w = seg.Direction() * 0.5f;
    EXVector axis2 = box.Axis(2);
    EXVector diff = seg.Origin() + w - box.Centre();
    float aw0 = (float)fabs(w.DotXYZ(box.Axis(0)));
    if ((float)fabs(diff.DotXYZ(box.Axis(0))) > aw0 + box.Extent(0)) return 0;
    float aw1 = (float)fabs(w.DotXYZ(box.Axis(1)));
    if ((float)fabs(diff.DotXYZ(box.Axis(1))) > aw1 + box.Extent(1)) return 0;
    float aw2 = (float)fabs(w.DotXYZ(axis2));
    if ((float)fabs(diff.DotXYZ(axis2)) > aw2 + box.Extent(2)) return 0;
    EXVector wxd = w.Cross(diff);
    if ((float)fabs(wxd.DotXYZ(box.Axis(0))) > aw1 * box.Extent(2) + aw2 * box.Extent(1)) return 0;
    if ((float)fabs(wxd.DotXYZ(box.Axis(1))) > aw0 * box.Extent(2) + aw2 * box.Extent(0)) return 0;
    if ((float)fabs(wxd.DotXYZ(axis2)) > aw0 * box.Extent(1) + aw1 * box.Extent(0)) return 0;
    return 1;
}

// Segment against box: where the segment enters and/or leaves the box.
unsigned int EXFindIntersection(const EXLineSeg &seg, const EXBox &box, EXVector *const pts)
{
    EXVector axis2 = box.Axis(2);
    EXVector diff = seg.Origin() - box.Centre();
    EXVector pnt(diff.DotXYZ(box.Axis(0)), diff.DotXYZ(box.Axis(1)), diff.DotXYZ(axis2), 0.0f);
    EXVector d(seg.Direction().DotXYZ(box.Axis(0)), seg.Direction().DotXYZ(box.Axis(1)),
               seg.Direction().DotXYZ(axis2), 0.0f);
    float t0 = 0.0f, t1 = 1.0f;
    if (!FindIntersection(pnt, d, box.Extents(), t0, t1)) return 0;
    if (t0 > 0.0f) {
        if (t1 < 1.0f) {
            pts[0] = seg.Origin() + seg.Direction() * t0;
            pts[1] = seg.Origin() + seg.Direction() * t1;
            return 2;
        }
        pts[0] = seg.Origin() + seg.Direction() * t0;
        return 1;
    }
    if (t1 < 1.0f) {
        pts[0] = seg.Origin() + seg.Direction() * t1;
        return 1;
    }
    return 0;
}

// Contact between swept primitives: closest points of the cores, pushed out to each surface along
// the line between them; d becomes the penetration depth (-9999 when apart).
#define EXCONTACT_DEPTH(R)                                                                         \
    if ((R) * (R) < d) {                                                                           \
        d = -9999.0f;                                                                              \
        return 0;                                                                                  \
    }                                                                                              \
    if (d < 0.0f) d = 0.0f;                                                                        \
    d = EXSqrt(d);

signed char EXFindIntersection(const EXSphere &a, const EXCapsule &b, EXVector &pa, EXVector &pb, float &d)
{
    float t;
    d = EXSqrDistance((EXMaths::vec_xyzw)a.Centre(), b.Segment(), &t);
    float r = b.Radius() + a.Radius();
    EXCONTACT_DEPTH(r)
    pb = b.Segment().Origin() + b.Segment().Direction() * t;
    EXVector n = (a.Centre() - pb) * (1.0f / d);
    pa = a.Centre() - n * a.Radius();
    pb += n * b.Radius();
    d = r - d;
    return 1;
}

signed char EXFindIntersection(const EXSphere &a, const EXLozenge &b, EXVector &pa, EXVector &pb, float &d)
{
    float s, t;
    d = EXSqrDistance((EXMaths::vec_xyzw)a.Centre(), b.Rectangle(), &s, &t);
    float r = a.Radius() + b.Radius();
    EXCONTACT_DEPTH(r)
    pb = b.Rectangle().Origin() + b.Rectangle().Edge0() * s + b.Rectangle().Edge1() * t;
    EXVector n = (a.Centre() - pb) * (1.0f / d);
    pa = a.Centre() - n * a.Radius();
    pb += n * b.Radius();
    d = r - d;
    return 1;
}

signed char EXFindIntersection(const EXCapsule &a, const EXCapsule &b, EXVector &pa, EXVector &pb, float &d)
{
    float s, t;
    d = EXSqrDistance(a.Segment(), b.Segment(), &s, &t);
    float r = b.Radius() + a.Radius();
    EXCONTACT_DEPTH(r)
    pa = a.Segment().Origin() + a.Segment().Direction() * s;
    pb = b.Segment().Origin() + b.Segment().Direction() * t;
    EXVector n = (pa - pb) * (1.0f / d);
    pa -= n * a.Radius();
    pb += n * b.Radius();
    d = r - d;
    return 1;
}

signed char EXFindIntersection(const EXCapsule &a, const EXLozenge &b, EXVector &pa, EXVector &pb, float &d)
{
    float r0, s, t;
    d = EXSqrDistance(a.Segment(), b.Rectangle(), &r0, &s, &t);
    float r = b.Radius() + a.Radius();
    EXCONTACT_DEPTH(r)
    pa = a.Segment().Origin() + a.Segment().Direction() * r0;
    pb = b.Rectangle().Origin() + b.Rectangle().Edge0() * s + b.Rectangle().Edge1() * t;
    EXVector n = (pa - pb) * (1.0f / d);
    pa -= n * a.Radius();
    pb += n * b.Radius();
    d = r - d;
    return 1;
}

signed char EXFindIntersection(const EXLozenge &a, const EXLozenge &b, EXVector &pa, EXVector &pb, float &d)
{
    float s0, t0, s1, t1;
    d = EXSqrDistance(a.Rectangle(), b.Rectangle(), &s0, &t0, &s1, &t1);
    float r = b.Radius() + a.Radius();
    EXCONTACT_DEPTH(r)
    pa = a.Rectangle().Origin() + a.Rectangle().Edge0() * s0 + a.Rectangle().Edge1() * t0;
    pb = b.Rectangle().Origin() + b.Rectangle().Edge0() * s1 + b.Rectangle().Edge1() * t1;
    EXVector n = (pa - pb) * (1.0f / d);
    pa -= n * a.Radius();
    pb += n * b.Radius();
    d = r - d;
    return 1;
}

// Line (origin o, direction dir) against capsule, in capsule space (u, v across the axis, w along
// it): the cylinder wall, then the bottom and top hemispheres. Returns how many parameters were
// written to t (at most two).
int EXFindIntersection(const EXVector &o, const EXVector &dir, const EXCapsule &c, float *const t)
{
    EXVector u, v, w = c.Segment().Direction();
    float wlen = w.Normalise();
    EXVector::GenerateOrthonormalBasis(u, v, w, 1);
    EXVector3 d(dir.DotXYZ(u), dir.DotXYZ(v), dir.DotXYZ(w));
    float dlen = d.Normalise();
    float invD = dlen != 0.0f ? 1.0f / dlen : 0.0f;
    EXVector diff = o - c.Segment().Origin();
    EXVector3 p(u.DotXYZ(diff), v.DotXYZ(diff), w.DotXYZ(diff));
    float rr = c.Radius() * c.Radius();
    float a, b, cc, discr, root, s, h;

    if ((float)fabs(d.z) >= 1.0f - FLT_EPSILON || dlen < 1e-7f) {
        // Moving along the axis (or not at all).
        float axis = dir.DotXYZ(c.Segment().Direction());
        discr = rr - p.x * p.x - p.y * p.y;
        if (axis < 0.0f && discr >= 0.0f) {
            t[0] = (p.z + EXSqrt(discr)) * invD;
            t[1] = -((wlen - p.z + EXSqrt(discr)) * invD);
            return 2;
        }
        if (axis > 0.0f && discr >= 0.0f) {
            t[0] = -((p.z + EXSqrt(discr)) * invD);
            t[1] = (wlen - p.z + EXSqrt(discr)) * invD;
            return 2;
        }
        return 0;
    }

    // Infinite cylinder.
    b = d.y * p.y + d.x * p.x;
    cc = p.y * p.y + p.x * p.x - rr;
    a = d.y * d.y + d.x * d.x;
    discr = b * b - cc * a;
    if (discr < 0.0f) return 0;
    int n = 0;
    if (discr > 0.0f) {
        float inv = a != 0.0f ? 1.0f / a : 0.0f;
        s = (-b - EXSqrt(discr)) * inv;
        h = s * d.z + p.z;
        if (0.0f <= h && h <= wlen) t[n++] = s * invD;
        s = (EXSqrt(discr) - b) * inv;
        h = s * d.z + p.z;
        if (0.0f <= h && h <= wlen) t[n++] = s * invD;
        if (n == 2) return 2;
    } else {
        s = a != 0.0f ? b / a : 0.0f;
        h = -s * d.z + p.z;
        if (0.0f <= h && h <= wlen) {
            t[0] = -s * invD;
            return 1;
        }
    }

    // Bottom hemisphere.
    b = p.z * d.z + b;
    cc = p.z * p.z + cc;
    discr = b * b - cc;
    if (discr > 0.0f) {
        root = EXSqrt(discr);
        s = -b - root;
        if (s * d.z + p.z <= 0.0f) {
            t[n++] = s * invD;
            if (n == 2) return 2;
        }
        s = root - b;
        if (s * d.z + p.z <= 0.0f) {
            t[n++] = s * invD;
            if (n == 2) return 2;
        }
    } else if (discr == 0.0f) {
        if (-b * d.z + p.z <= 0.0f) {
            t[n++] = -b * invD;
            if (n == 2) return 2;
        }
    }

    // Top hemisphere.
    b = b - wlen * d.z;
    cc = (wlen - p.z * 2.0f) * wlen + cc;
    discr = b * b - cc;
    if (discr > 0.0f) {
        root = EXSqrt(discr);
        s = -b - root;
        if (s * d.z + p.z >= wlen) {
            t[n++] = s * invD;
            if (n == 2) return 2;
        }
        s = root - b;
        if (s * d.z + p.z >= wlen) {
            t[n++] = s * invD;
            if (n == 2) return 2;
        }
    } else if (discr == 0.0f) {
        if (-b * d.z + p.z >= wlen) {
            t[n++] = -b * invD;
            if (n == 2) return 2;
        }
    }
    return n;
}

// Segment against capsule: the line hits that fall within the segment.
unsigned int EXFindIntersection(const EXLineSeg &seg, const EXCapsule &c, EXVector *const pts)
{
    float t[2];
    int n = EXFindIntersection(seg.Origin(), seg.Direction(), c, t);
    int count = 0;
    for (int i = 0; i < n; i++) {
        if (0.0f <= t[i] && t[i] <= 1.0f) pts[count++] = seg.Origin() + seg.Direction() * t[i];
    }
    return count;
}

// Moving spheres over [0, dt] with velocities va, vb: the first contact time t and point p (the
// centres' midpoint when they already overlap).
signed char EXFindIntersection(const EXSphere &a, const EXSphere &b, float dt, const EXVector &va,
                               const EXVector &vb, float &t, EXVector &p)
{
    EXVector vdiff = vb - va;
    float av = vdiff.LengthXYZSquared();
    EXVector cdiff = b.Centre() - a.Centre();
    float c = cdiff.LengthXYZSquared();
    float rsum = b.Radius() + a.Radius();
    float rr = rsum * rsum;
    float bv;
    if (av <= 0.0f || (bv = cdiff.DotXYZ(vdiff)) > 0.0f) {
        if (rr < c) return 0;
    } else {
        if (bv < -av * dt && rr < (bv * 2.0f + av * dt) * dt + c) return 0;
        float discr = bv * bv - (c - rr) * av;
        if (discr < 0.0f) return 0;
        if (c - rr > 0.0f) {
            t = -(EXSqrt(discr) + bv) / av;
            if (t < 0.0f) t = 0.0f;
            else if (t > dt) t = dt;
            float k = rsum != 0.0f ? a.Radius() / rsum : 0.0f;
            p = a.Centre() + va * t + (cdiff + vdiff * t) * k;
            return 1;
        }
    }
    t = 0.0f;
    p = (a.Centre() + b.Centre()) * 0.5f;
    return 1;
}

// Sphere against box: with the centre outside the box, the closest box point; inside, the
// shallowest face. pa is the deepest sphere point, pb the box contact, d the depth.
signed char EXFindIntersection(const EXSphere &s, const EXBox &box, EXVector &pa, EXVector &pb, float &d)
{
    EXVector axis2 = box.Axis(2);
    EXVector diff = s.Centre() - box.Centre();
    EXVector p(box.m_axis[0].Dot(diff), box.m_axis[1].Dot(diff), axis2.Dot(diff), diff.w);
    signed char outside = 0;
    if (p.x < -box.Extent(0)) {
        p.x = -box.Extent(0);
        outside = 1;
    }
    if (p.x > box.Extent(0)) {
        p.x = box.Extent(0);
        outside = 1;
    }
    if (p.y < -box.Extent(1)) {
        p.y = -box.Extent(1);
        outside = 1;
    }
    if (p.y > box.Extent(1)) {
        p.y = box.Extent(1);
        outside = 1;
    }
    if (p.z < -box.Extent(2)) {
        p.z = -box.Extent(2);
        outside = 1;
    }
    if (p.z > box.Extent(2)) {
        p.z = box.Extent(2);
        outside = 1;
    }
    if (!outside) {
        // Centre inside: push out through the nearest face.
        int axis = 0;
        d = box.Extent(0) - (float)fabs(p.x);
        if (box.Extent(1) - (float)fabs(p.y) < d) {
            d = box.Extent(1) - (float)fabs(p.y);
            axis = 1;
        }
        if (box.Extent(2) - (float)fabs(p.z) < d) {
            d = box.Extent(2) - (float)fabs(p.z);
            axis = 2;
        }
        EXVector n = p[axis] > 0.0f ? box.Axis(axis) : -box.Axis(axis);
        pa = s.Centre() - n * s.Radius();
        pb = s.Centre() + n * d;
        return 1;
    }
    EXVector q = box.m_axis[0] * p.x + box.m_axis[1] * p.y + axis2 * p.z;
    EXVector n = diff - q;
    d = n.Normalise();
    d = s.Radius() - d;
    if (d < 0.0f) return 0;
    pb = q + box.Centre();
    pa = s.Centre() - n * s.Radius();
    return 1;
}

// Capsule against box: closest points of the capsule segment and the box; pa is pushed out to the
// capsule surface towards the box, d is the penetration depth.
signed char EXFindIntersection(const EXCapsule &c, const EXBox &box, EXVector &pa, EXVector &pb, float &d)
{
    float t, b0, b1, b2;
    float sq = EXSqrDistance(c.Segment(), box, &t, &b0, &b1, &b2);
    if (c.Radius() * c.Radius() < sq) {
        d = -9999.0f;
        return 0;
    }
    float len = sq >= 0.0f ? EXSqrt(sq) : 0.0f;
    d = c.Radius() - len;
    if (d < 0.0f) d = 0.0f;
    pb = box.Centre() + box.m_axis[0] * b0 + box.m_axis[1] * b1 + box.Axis(2) * b2;
    pa = c.Segment().Origin() + c.Segment().Direction() * t;
    EXMaths::vec_xyzw n;
    n.set(pb.x - pa.x, pb.y - pa.y, pb.z - pa.z, 0.0f);
    n.Normalise();
    pa = pa + EXVector(n) * c.Radius();
    return 1;
}

// Box against box: separating axis test over the 15 candidate axes (the 3 + 3 face normals and the
// 9 edge cross products; edge axes are biased by 1.05 in favour of faces). The shallowest axis
// gives the contact: for an edge pair, the closest points of the two edges; for a face, the
// incident face of the other box clipped to the reference face and averaged by depth. pa and pb
// are the contact points on a and b, d the penetration depth.
#define BOXBOX_FACE(EXPR1, EXPR2, NORM, CODE)                                                      \
    s2 = (float)fabs(EXPR1) - (EXPR2);                                                             \
    if (s2 > 0.0f) return 0;                                                                       \
    if (s2 > s) {                                                                                  \
        s = s2;                                                                                    \
        normR = NORM;                                                                              \
        useR = 1;                                                                                  \
        invert = (EXPR1) < 0.0f;                                                                   \
        code = CODE;                                                                               \
    }
#define BOXBOX_EDGE(EXPR1, EXPR2, N1, N2, N3, CODE)                                                \
    e1 = EXPR1;                                                                                    \
    s2 = (float)fabs(e1) - (EXPR2);                                                                \
    if (s2 > 0.0f) return 0;                                                                       \
    l = EXSqrt((N1) * (N1) + (N2) * (N2) + (N3) * (N3));                                           \
    if (l > 0.0f) {                                                                                \
        float inv = 1.0f / l;                                                                      \
        s2 *= inv;                                                                                 \
        if (s2 * 1.05f > s) {                                                                      \
            s = s2;                                                                                \
            useR = 0;                                                                              \
            normC.x = (N1) * inv;                                                                  \
            normC.y = (N2) * inv;                                                                  \
            normC.z = (N3) * inv;                                                                  \
            invert = e1 < 0.0f;                                                                    \
            code = CODE;                                                                           \
        }                                                                                          \
    }

signed char EXFindIntersection(const EXBox &a, const EXBox &b, EXVector &pa, EXVector &pb, float &d)
{
    EXVector a2 = a.Axis(2);
    EXVector p = b.Centre() - a.Centre();
    EXVector b2 = b.Axis(2);
    EXVector normR(0.0f, 0.0f, 0.0f, 0.0f);
    float pp[3];
    int i, j;
    for (i = 0; i < 2; i++) pp[i] = a.Axis(i).Dot(p);
    pp[2] = a2.Dot(p);
    const float *A = a.Extents();
    const float *B = b.Extents();
    float R11 = a.m_axis[0].Dot(b.m_axis[0]);
    float R12 = a.m_axis[0].Dot(b.m_axis[1]);
    float R13 = a.m_axis[0].Dot(b2);
    float R21 = a.m_axis[1].Dot(b.m_axis[0]);
    float R22 = a.m_axis[1].Dot(b.m_axis[1]);
    float R23 = a.m_axis[1].Dot(b2);
    float R31 = a.Axis(2).Dot(b.m_axis[0]);
    float R32 = a.Axis(2).Dot(b.m_axis[1]);
    float R33 = a.Axis(2).Dot(b2);
    float Q11 = (float)fabs(R11), Q12 = (float)fabs(R12), Q13 = (float)fabs(R13);
    float Q21 = (float)fabs(R21), Q22 = (float)fabs(R22), Q23 = (float)fabs(R23);
    float Q31 = (float)fabs(R31), Q32 = (float)fabs(R32), Q33 = (float)fabs(R33);
    float s = -FLT_MAX, s2, e1, l;
    EXVector normC;
    signed char useR = 0, invert = 0;
    int code = 0;

    // Face normals of a, then of b.
    BOXBOX_FACE(pp[0], B[1] * Q12 + B[2] * Q13 + B[0] * Q11 + A[0], a.m_axis[0], 1)
    BOXBOX_FACE(pp[1], B[1] * Q22 + B[2] * Q23 + B[0] * Q21 + A[1], a.m_axis[1], 2)
    BOXBOX_FACE(pp[2], B[1] * Q32 + B[2] * Q33 + B[0] * Q31 + A[2], a2, 3)
    BOXBOX_FACE(b.m_axis[0].Dot(p), A[1] * Q21 + A[0] * Q11 + A[2] * Q31 + B[0], b.m_axis[0], 4)
    BOXBOX_FACE(b.m_axis[1].Dot(p), A[1] * Q22 + A[0] * Q12 + A[2] * Q32 + B[1], b.m_axis[1], 5)
    BOXBOX_FACE(b.Axis(2).Dot(p), A[1] * Q23 + A[0] * Q13 + A[2] * Q33 + B[2], b.Axis(2), 6)

    // Edge pairs: a's edge i crossed with b's edge j.
    BOXBOX_EDGE(pp[2] * R21 - pp[1] * R31, B[2] * Q12 + B[1] * Q13 + A[1] * Q31 + A[2] * Q21, 0.0f, -R31, R21, 7)
    BOXBOX_EDGE(pp[2] * R22 - pp[1] * R32, B[2] * Q11 + B[0] * Q13 + A[1] * Q32 + A[2] * Q22, 0.0f, -R32, R22, 8)
    BOXBOX_EDGE(pp[2] * R23 - pp[1] * R33, B[1] * Q11 + B[0] * Q12 + A[1] * Q33 + A[2] * Q23, 0.0f, -R33, R23, 9)
    BOXBOX_EDGE(pp[0] * R31 - pp[2] * R11, B[2] * Q22 + B[1] * Q23 + A[2] * Q11 + A[0] * Q31, R31, 0.0f, -R11, 10)
    BOXBOX_EDGE(pp[0] * R32 - pp[2] * R12, B[2] * Q21 + B[0] * Q23 + A[2] * Q12 + A[0] * Q32, R32, 0.0f, -R12, 11)
    BOXBOX_EDGE(pp[0] * R33 - pp[2] * R13, B[1] * Q21 + B[0] * Q22 + A[2] * Q13 + A[0] * Q33, R33, 0.0f, -R13, 12)
    BOXBOX_EDGE(pp[1] * R11 - pp[0] * R21, B[2] * Q32 + B[1] * Q33 + A[1] * Q21 + A[0] * Q11, -R21, R11, 0.0f, 13)
    BOXBOX_EDGE(pp[1] * R12 - pp[0] * R22, B[2] * Q31 + B[0] * Q33 + A[1] * Q22 + A[0] * Q12, -R22, R12, 0.0f, 14)
    BOXBOX_EDGE(pp[1] * R13 - pp[0] * R23, B[1] * Q31 + B[0] * Q32 + A[1] * Q23 + A[0] * Q13, -R23, R13, 0.0f, 15)

    if (!code) return 0;

    // World space contact normal (from a towards b).
    EXVector normal;
    if (useR) {
        normal = normR;
    } else {
        normal.Set(0.0f, 0.0f, 0.0f, 0.0f);
        for (i = 0; i < 2; i++) normal += a.Axis(i) * normC[i];
        normal += a2 * normC.z;
    }
    if (invert) {
        normal.x = -normal.x;
        normal.y = -normal.y;
        normal.z = -normal.z;
    }
    float depth = -s;

    if (code > 6) {
        // Edge against edge: walk to each box's edge on the contact side, then take the closest
        // points of the two edge lines.
        const float *ext = a.Extents();
        pa = a.Centre();
        for (j = 0; j < 3; j++, ext++) {
            float sign = a.Axis(j).Dot(normal) > 0.0f ? 1.0f : -1.0f;
            pa += a.Axis(j) * (sign * *ext);
        }
        ext = b.Extents();
        pb = b.Centre();
        for (j = 0; j < 3; j++, ext++) {
            float sign = b.Axis(j).Dot(normal) > 0.0f ? -1.0f : 1.0f;
            pb += b.Axis(j) * (*ext * sign);
        }
        EXVector ua = a.Axis((code - 7) / 3);
        EXVector ub = b.Axis((code - 7) % 3);
        float alpha, beta;
        EXDistance(EXLine(pa, ua), EXLine(pb, ub), &alpha, &beta);
        pa += ua * alpha;
        pb += ub * beta;
        d = depth;
        return 1;
    }

    // Face contact: a reference face on one box (the one whose normal won) and the incident face
    // of the other.
    const EXBox *ra = &a, *rb = &b;
    EXVector normal2 = normal;
    if (code > 3) {
        normal2.x = -normal.x;
        normal2.y = -normal.y;
        normal2.z = -normal.z;
        ra = &b;
        rb = &a;
    }
    EXVector nr;
    for (i = 0; i < 3; i++) nr[i] = rb->Axis(i).Dot(normal2);
    EXVector anr((float)fabs(nr.x), (float)fabs(nr.y), (float)fabs(nr.z), nr.w);
    int lanr, a1, a3;
    if (anr.y > anr.x) {
        a1 = 0;
        if (anr.y > anr.z) {
            lanr = 1;
            a3 = 2;
        } else {
            a3 = 1;
            lanr = 2;
        }
    } else if (anr.x > anr.z) {
        lanr = 0;
        a1 = 1;
        a3 = 2;
    } else {
        a1 = 0;
        a3 = 1;
        lanr = 2;
    }
    // Centre of the incident face relative to the reference box.
    EXVector center;
    if (nr[lanr] < 0.0f)
        center = rb->Centre() - ra->Centre() + rb->Axis(lanr) * rb->Extent(lanr);
    else
        center = rb->Centre() - ra->Centre() - rb->Axis(lanr) * rb->Extent(lanr);
    int codeN = code <= 3 ? code - 1 : code - 4;
    int code1, code2 = 2;
    if (codeN == 0) {
        code1 = 1;
    } else {
        code1 = 0;
        if (codeN != 1) code2 = 1;
    }
    // Incident face corners in the reference face's 2D frame.
    float c1 = ra->Axis(code1).Dot(center);
    float c2 = ra->Axis(code2).Dot(center);
    float m11 = rb->Axis(a1).Dot(ra->Axis(code1));
    float m12 = rb->Axis(a3).Dot(ra->Axis(code1));
    float m21 = rb->Axis(a1).Dot(ra->Axis(code2));
    float m22 = rb->Axis(a3).Dot(ra->Axis(code2));
    float k1 = m11 * rb->Extent(a1);
    float k2 = m21 * rb->Extent(a1);
    float k3 = m12 * rb->Extent(a3);
    float k4 = m22 * rb->Extent(a3);
    float quad[8];
    quad[0] = c1 - k1 - k3;
    quad[1] = c2 - k2 - k4;
    quad[2] = c1 - k1 + k3;
    quad[3] = c2 - k2 + k4;
    quad[4] = c1 + k1 + k3;
    quad[5] = c2 + k2 + k4;
    quad[6] = c1 + k1 - k3;
    quad[7] = c2 + k2 - k4;
    float rect[2];
    rect[0] = ra->Extent(code1);
    rect[1] = ra->Extent(code2);
    float ret[16];
    int n = EXIntersectionRectQuad(rect, quad, ret);
    if (n > 0) {
        // Back to 3D, keeping the penetrating points.
        float det1 = 1.0f / (m22 * m11 - m21 * m12);
        m11 *= det1;
        m21 *= det1;
        m12 *= det1;
        m22 *= det1;
        float point[3 * 8];
        float dep[8];
        int cnum = 0;
        for (j = 0; j < n; j++) {
            float q1 = m22 * (ret[j * 2] - c1) - m12 * (ret[j * 2 + 1] - c2);
            float q2 = m11 * (ret[j * 2 + 1] - c2) - m21 * (ret[j * 2] - c1);
            for (i = 0; i < 3; i++) point[cnum * 3 + i] = rb->Axis(a3)[i] * q2 + center[i] + q1 * rb->Axis(a1)[i];
            dep[cnum] = ra->Extent(codeN) - normal2.Dot(EXVector(point[cnum * 3], point[cnum * 3 + 1],
                                                                 point[cnum * 3 + 2], 0.0f));
            if (dep[cnum] >= 0.0f) {
                ret[cnum * 2] = ret[j * 2];
                ret[cnum * 2 + 1] = ret[j * 2 + 1];
                cnum++;
            }
        }
        if (cnum > 0) {
            // Depth weighted average of the contact points.
            float sum = 0.0f;
            EXVector acc(0.0f, 0.0f, 0.0f, 0.0f);
            for (j = 0; j < cnum; j++) {
                sum += dep[j];
                acc += EXVector(point[j * 3], point[j * 3 + 1], point[j * 3 + 2], 0.0f) * dep[j];
            }
            if (sum >= FLT_EPSILON) {
                acc = acc * (1.0f / sum);
                d = ra->Extent(codeN) - normal2.Dot(acc);
                EXVector c = ra->Centre() + acc;
                if (code < 4) {
                    pa = c + normal2 * d;
                    pb = c;
                } else {
                    pb = c + normal2 * d;
                    pa = c;
                }
                return 1;
            }
        }
    }
    return 0;
}

// Box against triangle separating axis test over 13 axes: the triangle normal (1), the box axes
// (2-4) and each box axis crossed with each triangle edge (5-13). On overlap t holds the
// shallowest axis; on separation t.m_FailAxis says which axis separated.
#define TRI_EDGE_AXIS(AI, E, PSIGN, AJ, EJ, AK, EK, ID)                                            \
    {                                                                                              \
        EXVector L = box.Axis(AI).Cross(E);                                                        \
        float p0 = L.Dot(diff);                                                                    \
        if (!SeparatingAxisTestEdge(p0, p0 PSIGN box.Axis(AI).Dot(n),                              \
                                    (float)fabs(box.Axis(AJ).Dot(E)) * box.Extent(EJ) +            \
                                        (float)fabs(box.Axis(AK).Dot(E)) * box.Extent(EK),         \
                                    n.Dot(L) * invLen, L, ID, t)) {                                \
            t.m_FailAxis = ID;                                                                     \
            return 0;                                                                              \
        }                                                                                          \
    }

signed char TestSeparatingAxes(const EXBox &box, const EXTriangle &tri, EXSeparatingAxisTemp &t)
{
    t.m_Depth = FLT_MAX;
    t.m_AxisId = 0;
    t.m_FailAxis = -1;
    EXVector e0 = tri.Edge0();
    EXVector e1 = tri.Edge1();
    EXVector e2 = e1 - e0;
    EXVector a2 = box.Axis(2);
    EXVector n = e0.Cross(e1);
    EXVector diff = tri.Origin() - box.Centre();
    float len = n.Length();
    // Triangle normal.
    float an2 = a2.Dot(n);
    if (!SeparatingAxisTestNormal(diff.Dot(n),
                                  (float)fabs(an2) * box.Extent(2) + (float)fabs(box.m_axis[1].Dot(n)) * box.Extent(1) +
                                      (float)fabs(box.m_axis[0].Dot(n)) * box.Extent(0),
                                  n, 1, t)) {
        t.m_FailAxis = 1;
        return 0;
    }
    float invLen = 1.0f / len;
    // Box face normals.
    float p0 = box.m_axis[0].Dot(diff);
    if (!SeparatingAxisTestFace(p0, box.m_axis[0].Dot(e0) + p0, box.m_axis[0].Dot(e1) + p0, box.Extent(0),
                                box.m_axis[0].Dot(n) * invLen, box.m_axis[0], 2, t)) {
        t.m_FailAxis = 2;
        return 0;
    }
    p0 = box.m_axis[1].Dot(diff);
    if (!SeparatingAxisTestFace(p0, box.m_axis[1].Dot(e0) + p0, box.m_axis[1].Dot(e1) + p0, box.Extent(1),
                                n.Dot(box.m_axis[1]) * invLen, box.m_axis[1], 3, t)) {
        t.m_FailAxis = 3;
        return 0;
    }
    p0 = a2.Dot(diff);
    if (!SeparatingAxisTestFace(p0, a2.Dot(e0) + p0, a2.Dot(e1) + p0, box.Extent(2), an2 * invLen, a2, 4, t)) {
        t.m_FailAxis = 4;
        return 0;
    }
    // Box axes crossed with the triangle edges.
    TRI_EDGE_AXIS(0, e0, +, 1, 2, 2, 1, 5)
    TRI_EDGE_AXIS(0, e1, -, 1, 2, 2, 1, 6)
    TRI_EDGE_AXIS(0, e2, -, 1, 2, 2, 1, 7)
    TRI_EDGE_AXIS(1, e0, +, 0, 2, 2, 0, 8)
    TRI_EDGE_AXIS(1, e1, -, 0, 2, 2, 0, 9)
    TRI_EDGE_AXIS(1, e2, -, 0, 2, 2, 0, 10)
    TRI_EDGE_AXIS(2, e0, +, 0, 1, 1, 0, 11)
    TRI_EDGE_AXIS(2, e1, -, 0, 1, 1, 0, 12)
    TRI_EDGE_AXIS(2, e2, -, 0, 1, 1, 0, 13)
    return 1;
}

// Contact point and depth for an overlapping box and triangle, given the shallowest separating
// axis found by TestSeparatingAxes. Edge axes take the closest points of the two edges; a box face
// clips the triangle to the box slab; the triangle face clips the most aligned box face to the
// triangle prism. Face contacts average the clipped points weighted by depth. -FLT_MAX on failure.
float FindContactPoint(const EXBox &box, const EXTriangle &tri, EXSeparatingAxisTemp &t, EXVector &p)
{
    EXVector v0 = tri.Origin();
    EXVector v1 = tri.Origin() + tri.Edge0();
    EXVector v2 = tri.Origin() + tri.Edge1();
    int i;
    t.m_Axis.w = 0.0f;

    if (t.m_AxisId > 4) {
        // Box edge against triangle edge.
        EXVector bp = box.Centre();
        const float *ext = box.Extents();
        for (i = 0; i < 3; i++, ext++) {
            const EXVector a = box.Axis(i);
            float sign = a.Dot(t.m_Axis) > 0.0f ? 1.0f : -1.0f;
            bp.y += *ext * a.y * sign;
            bp.x += *ext * a.x * sign;
            bp.z += *ext * a.z * sign;
        }
        EXVector o, e;
        int edge = (t.m_AxisId - 5) % 3;
        if (edge == 0) {
            o = v0;
            e = tri.Edge0();
        } else if (edge == 1) {
            o = v2;
            e = tri.Edge1();
        } else {
            o = v1;
            e = tri.Edge1() - tri.Edge0();
        }
        e.w = 0.0f;
        e.Normalise();
        EXVector a = box.Axis((t.m_AxisId - 5) / 3);
        a.w = 0.0f;
        a.Normalise();
        float s, u;
        EXSqrDistance(EXLine(bp, a), EXLine(o, e), &s, &u);
        p = o + e * u;
        return t.m_Depth;
    }

    if (t.m_AxisId != 1) {
        // Box face: clip the triangle to the slab of the box face and its four neighbours.
        EXVector n = t.m_Axis;
        int i0, i1 = 2;
        if (t.m_AxisId == 2) {
            i0 = 1;
        } else {
            i0 = 0;
            if (t.m_AxisId != 3) i1 = 1;
        }
        EXVector tv[3];
        tv[0] = v0 - box.Centre();
        tv[1] = v1 - box.Centre();
        tv[2] = v2 - box.Centre();
        EXVector polyA[9], polyB[9];
        for (i = 0; i < 9; i++) {
            polyA[i].x = 0.0f;
            polyA[i].y = 0.0f;
            polyA[i].z = 0.0f;
            polyB[i].x = 0.0f;
            polyB[i].y = 0.0f;
            polyB[i].z = 0.0f;
        }
        int nA, nB;
        EXVector plane(-n.x, -n.y, -n.z, box.Extent(t.m_AxisId - 2));
        ClipPolyToPlane(tv, 3, polyA, nA, plane);
        plane = box.Axis(i0);
        plane.w = box.Extent(i0);
        ClipPolyToPlane(polyA, nA, polyB, nB, plane);
        plane = -box.Axis(i0);
        plane.w = box.Extent(i0);
        ClipPolyToPlane(polyB, nB, polyA, nA, plane);
        plane = box.Axis(i1);
        plane.w = box.Extent(i1);
        ClipPolyToPlane(polyA, nA, polyB, nB, plane);
        plane = -box.Axis(i1);
        plane.w = box.Extent(i1);
        ClipPolyToPlane(polyB, nB, polyA, nA, plane);
        if (nA > 0) {
            n.w = 0.0f;
            float sum = 0.0f;
            EXVector acc(0.0f, 0.0f, 0.0f, 0.0f);
            for (i = 0; i < nA; i++) {
                float dep = -(polyA[i].Dot(n) - box.Extent(t.m_AxisId - 2));
                if (dep >= 0.0f) {
                    acc += polyA[i] * dep;
                    sum += dep;
                }
            }
            if (sum >= FLT_EPSILON) {
                acc = acc * (1.0f / sum);
                float depth = -(acc.Dot(n) - box.Extent(t.m_AxisId - 2));
                p = acc + box.Centre();
                return depth;
            }
        }
        return -FLT_MAX;
    }

    // Triangle face: clip the box face most opposed to the normal to the triangle's prism.
    EXVector nb(-t.m_Axis.x, -t.m_Axis.y, -t.m_Axis.z, 0.0f);
    float proj[4];
    proj[0] = proj[1] = proj[2] = proj[3] = 0.0f;
    for (i = 0; i < 3; i++) proj[i] = box.Axis(i).Dot(nb);
    float ax = (float)fabs(proj[0]), ay = (float)fabs(proj[1]), az = (float)fabs(proj[2]);
    int lanr, i3, i4;
    if (ay > ax) {
        i3 = 0;
        if (ay > az) {
            lanr = 1;
            i4 = 2;
        } else {
            i4 = 1;
            lanr = 2;
        }
    } else if (ax > az) {
        lanr = 0;
        i3 = 1;
        i4 = 2;
    } else {
        i3 = 0;
        i4 = 1;
        lanr = 2;
    }
    EXVector fa = box.Axis(lanr);
    EXVector fc;
    if (proj[lanr] <= 0.0f) {
        fc.x = fa.x * box.Extent(lanr) + (box.Centre().x - v0.x);
        fc.y = (box.Centre().y - v0.y) + fa.y * box.Extent(lanr);
        fc.z = (box.Centre().z - v0.z) + fa.z * box.Extent(lanr);
    } else {
        fc.x = (box.Centre().x - v0.x) - fa.x * box.Extent(lanr);
        fc.y = (box.Centre().y - v0.y) - fa.y * box.Extent(lanr);
        fc.z = (box.Centre().z - v0.z) - fa.z * box.Extent(lanr);
    }
    EXVector b1 = box.Axis(i3);
    EXVector b2 = box.Axis(i4);
    b1.x *= box.Extent(i3);
    b2.x = box.Extent(i4) * b2.x;
    EXVector quad[4];
    quad[0].x = (fc.x + b1.x) - b2.x;
    quad[2].x = (fc.x - b1.x) + b2.x;
    quad[3].x = fc.x + b2.x + b1.x;
    quad[1].x = (fc.x - b1.x) - b2.x;
    b2.y = box.Extent(i4) * b2.y;
    b1.y = box.Extent(i3) * b1.y;
    quad[0].y = (fc.y + b1.y) - b2.y;
    quad[3].y = fc.y + b2.y + b1.y;
    b1.z = box.Extent(i3) * b1.z;
    quad[2].y = (fc.y - b1.y) + b2.y;
    quad[1].y = (fc.y - b1.y) - b2.y;
    b2.z = box.Extent(i4) * b2.z;
    quad[0].z = (fc.z + b1.z) - b2.z;
    quad[1].z = (fc.z - b1.z) - b2.z;
    quad[2].z = (fc.z - b1.z) + b2.z;
    quad[3].z = fc.z + b2.z + b1.z;
    int nA = 0, nB = 0;
    EXVector polyA[9], polyB[9];
    for (i = 0; i < 9; i++) {
        polyB[i].x = 0.0f;
        polyB[i].y = 0.0f;
        polyB[i].z = 0.0f;
        polyA[i].x = 0.0f;
        polyA[i].y = 0.0f;
        polyA[i].z = 0.0f;
    }
    EXVector n = tri.Edge0().Cross(tri.Edge1());
    // Below the triangle plane.
    EXVector plane(-n.x, -n.y, -n.z, 0.0f);
    plane.NormaliseXYZ();
    plane.w = 0.0f;
    ClipPolyToPlane(quad, 4, polyB, nB, plane);
    // Inside each triangle edge.
    EXVector e = v1 - v0;
    plane = n.Cross(e);
    plane.NormaliseXYZ();
    plane.w = 0.0f;
    ClipPolyToPlane(polyB, nB, polyA, nA, plane);
    e = v2 - v1;
    plane = n.Cross(e);
    plane.w = 0.0f;
    plane.Normalise();
    plane.w = (v0 - v2).Dot(plane);
    ClipPolyToPlane(polyA, nA, polyB, nB, plane);
    e = v0 - v2;
    plane = n.Cross(e);
    plane.NormaliseXYZ();
    plane.w = 0.0f;
    ClipPolyToPlane(polyB, nB, polyA, nA, plane);
    if (nA > 0) {
        float sum = 0.0f;
        EXVector acc(0.0f, 0.0f, 0.0f, 0.0f);
        for (i = 0; i < nA; i++) {
            float dep = -nb.Dot(polyA[i]);
            if (dep >= 0.0f) {
                acc += polyA[i] * dep;
                sum += dep;
            }
        }
        if (sum >= FLT_EPSILON) {
            acc = acc * (1.0f / sum);
            float depth = -nb.Dot(acc);
            p = v0 + nb * depth + acc;
            return depth;
        }
    }
    return -FLT_MAX;
}

// Segment against lozenge: hits on the four edge capsules and on the core box; returns the nearest
// and farthest hit from the segment origin (2), or the count when there are none.
unsigned int EXFindIntersection(const EXLineSeg &seg, const EXLozenge &z, EXVector *const pts)
{
    const EXRectangle &r = z.Rectangle();
    EXVector c1 = r.Edge0() + r.Origin();
    EXVector c0 = r.Origin();
    EXVector c2 = r.Edge1() + r.Origin();
    EXVector c3 = r.Edge0() + r.Origin() + r.Edge1();
    EXVector hits[10];
    EXCapsule cap;
    cap.Set(c0, c2 - c0, z.Radius());
    int n = EXFindIntersection(seg, cap, hits);
    cap.Set(c0, c1 - c0, z.Radius());
    n += EXFindIntersection(seg, cap, hits + n);
    cap.Set(c1, c3 - c1, z.Radius());
    n += EXFindIntersection(seg, cap, hits + n);
    cap.Set(c2, c3 - c2, z.Radius());
    n += EXFindIntersection(seg, cap, hits + n);
    // The flat core: a box of half height radius around the rectangle.
    EXBox box;
    EXVector e0 = r.Edge0().UnitVector();
    EXVector e1 = r.Edge1().UnitVector();
    EXVector off = e0.Cross(e1) * z.Radius();
    EXVector lo = c0 + off;
    box.Centre() = lo.Interp(c3 - off, 0.5f);
    box.SetAxis(0, e0);
    box.SetAxis(1, e1);
    box.Extent(0) = EXSqrt((c0 - c1).LengthXYZSquared()) * 0.5f;
    box.Extent(1) = EXSqrt((c0 - c2).LengthXYZSquared()) * 0.5f;
    box.Extent(2) = z.Radius();
    n += EXFindIntersection(seg, box, hits + n);
    if (n < 1) return n;
    int iMin = 0, iMax = 0;
    float dMin = 10000000.0f, dMax = -10000000.0f;
    for (int i = 0; i < n; i++) {
        float d = (hits[i] - seg.Origin()).LengthXYZSquared();
        if (d < dMin) {
            dMin = d;
            iMin = i;
        }
        if (d > dMax) {
            iMax = i;
            dMax = d;
        }
    }
    pts[0] = hits[iMin];
    pts[1] = hits[iMax];
    return 2;
}
