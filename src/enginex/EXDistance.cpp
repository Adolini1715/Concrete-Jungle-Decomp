#include "EXDistance.h"
#include "EXArray.h"
#include "EXLine.h"
#include "EXCollisionType.h"
#include "EXTriangle.h"
#include <math.h>

// NEAR: 12 bytes differ, d.y/d.z register swap (xmm1/xmm2) and the matching x87 sum order.
float EXSqrDistance(const EXMaths::vec_xyzw &p, const EXLine &l, float *pt)
{
    EXMaths::vec_xyzw d = p - l.Origin();
    float t = ((EXMaths::vec_xyzw)l.Direction()).DotXYZ(d) / l.Direction().LengthXYZSquared();
    d -= (EXMaths::vec_xyzw)(l.Direction() * t);
    if (pt) *pt = t;
    return d.LengthXYZSquared();
}

// NEAR: 12 bytes differ, same d.y/d.z register swap as the EXLine version.
float EXSqrDistance(const EXMaths::vec_xyzw &p, const EXRay &l, float *pt)
{
    EXMaths::vec_xyzw d = p - l.Origin();
    float t = ((EXMaths::vec_xyzw)l.Direction()).DotXYZ(d);
    if (t <= 0.0f) {
        t = 0.0f;
    } else {
        t /= l.Direction().LengthXYZSquared();
        d -= (EXMaths::vec_xyzw)(l.Direction() * t);
    }
    if (pt) *pt = t;
    return d.LengthXYZSquared();
}

// NEAR: 25 bytes differ, same register swap as the EXLine version.
float EXSqrDistance(const EXMaths::vec_xyzw &p, const EXLineSeg &l, float *pt)
{
    EXMaths::vec_xyzw d = p - l.Origin();
    float t = ((EXMaths::vec_xyzw)l.Direction()).DotXYZ(d);
    if (t <= 0.0f) {
        t = 0.0f;
    } else {
        float len2 = l.Direction().LengthXYZSquared();
        if (t >= len2) {
            t = 1.0f;
            d -= (EXMaths::vec_xyzw)l.Direction();
        } else {
            t /= len2;
            d -= (EXMaths::vec_xyzw)(l.Direction() * t);
        }
    }
    if (pt) *pt = t;
    return d.LengthXYZSquared();
}

// Line/box helpers ([fs], EXDistance.obj). The line is in box space with every direction component
// made non-negative (the caller reflects them); pnt walks to the closest box point, pfL receives the
// line parameter and sqr accumulates the squared distance. LB_Face handles a line with all three
// direction components positive whose closest box feature lies on face i0.
static void LB_Face(int i0, int i1, int i2, EXMaths::vec_xyzw &pnt, const EXMaths::vec_xyzw &dir,
                    const EXBox &box, const EXMaths::vec_xyzw &PmE, float *pfL, float &sqr)
{
    EXMaths::vec_xyzw PpE;
    float lenSqr, tmp, delta, param, t;
    PpE[i1] = pnt[i1] + box.Extent(i1);
    PpE[i2] = pnt[i2] + box.Extent(i2);
    if (dir[i0] * PpE[i1] >= dir[i1] * PmE[i0]) {
        if (dir[i0] * PpE[i2] >= dir[i2] * PmE[i0]) {
            // The line meets the face: zero distance.
            if (pfL) {
                pnt[i0] = box.Extent(i0);
                float inv = 1.0f / dir[i0];
                pnt[i1] -= dir[i1] * PmE[i0] * inv;
                pnt[i2] -= dir[i2] * PmE[i0] * inv;
                *pfL = -PmE[i0] * inv;
            }
        } else {
            // Closest along the edge parallel to axis i1 (i2 at -extent).
            lenSqr = dir[i0] * dir[i0] + dir[i2] * dir[i2];
            tmp = lenSqr * PpE[i1] - dir[i1] * (dir[i0] * PmE[i0] + dir[i2] * PpE[i2]);
            if (tmp <= box.Extent(i1) * lenSqr * 2.0f) {
                t = tmp / lenSqr;
                lenSqr += dir[i1] * dir[i1];
                tmp = PpE[i1] - t;
                delta = dir[i0] * PmE[i0] + dir[i1] * tmp + dir[i2] * PpE[i2];
                param = (-1.0f / lenSqr) * delta;
                sqr += PmE[i0] * PmE[i0] + tmp * tmp + PpE[i2] * PpE[i2] + delta * param;
                if (pfL) {
                    *pfL = param;
                    pnt[i0] = box.Extent(i0);
                    pnt[i1] = t - box.Extent(i1);
                    pnt[i2] = -box.Extent(i2);
                }
            } else {
                lenSqr += dir[i1] * dir[i1];
                delta = dir[i0] * PmE[i0] + dir[i1] * PmE[i1] + dir[i2] * PpE[i2];
                param = (-1.0f / lenSqr) * delta;
                sqr += PmE[i0] * PmE[i0] + PmE[i1] * PmE[i1] + PpE[i2] * PpE[i2] + delta * param;
                if (pfL) {
                    *pfL = param;
                    pnt[i0] = box.Extent(i0);
                    pnt[i1] = box.Extent(i1);
                    pnt[i2] = -box.Extent(i2);
                }
            }
        }
    } else if (dir[i0] * PpE[i2] >= dir[i2] * PmE[i0]) {
        // Closest along the edge parallel to axis i2 (i1 at -extent).
        lenSqr = dir[i0] * dir[i0] + dir[i1] * dir[i1];
        tmp = lenSqr * PpE[i2] - dir[i2] * (dir[i0] * PmE[i0] + dir[i1] * PpE[i1]);
        if (tmp <= box.Extent(i2) * lenSqr * 2.0f) {
            t = tmp / lenSqr;
            lenSqr += dir[i2] * dir[i2];
            tmp = PpE[i2] - t;
            delta = dir[i0] * PmE[i0] + dir[i1] * PpE[i1] + dir[i2] * tmp;
            param = (-1.0f / lenSqr) * delta;
            sqr += PmE[i0] * PmE[i0] + PpE[i1] * PpE[i1] + tmp * tmp + delta * param;
            if (pfL) {
                *pfL = param;
                pnt[i0] = box.Extent(i0);
                pnt[i1] = -box.Extent(i1);
                pnt[i2] = t - box.Extent(i2);
            }
        } else {
            lenSqr += dir[i2] * dir[i2];
            delta = dir[i0] * PmE[i0] + dir[i1] * PpE[i1] + dir[i2] * PmE[i2];
            param = (-1.0f / lenSqr) * delta;
            sqr += PmE[i0] * PmE[i0] + PpE[i1] * PpE[i1] + PmE[i2] * PmE[i2] + delta * param;
            if (pfL) {
                *pfL = param;
                pnt[i0] = box.Extent(i0);
                pnt[i1] = -box.Extent(i1);
                pnt[i2] = box.Extent(i2);
            }
        }
    } else {
        lenSqr = dir[i0] * dir[i0] + dir[i2] * dir[i2];
        tmp = lenSqr * PpE[i1] - dir[i1] * (dir[i0] * PmE[i0] + dir[i2] * PpE[i2]);
        if (tmp >= 0.0f) {
            if (tmp <= box.Extent(i1) * lenSqr * 2.0f) {
                t = tmp / lenSqr;
                lenSqr += dir[i1] * dir[i1];
                tmp = PpE[i1] - t;
                delta = dir[i0] * PmE[i0] + dir[i1] * tmp + dir[i2] * PpE[i2];
                param = (-1.0f / lenSqr) * delta;
                sqr += PmE[i0] * PmE[i0] + tmp * tmp + PpE[i2] * PpE[i2] + delta * param;
                if (pfL) {
                    *pfL = param;
                    pnt[i0] = box.Extent(i0);
                    pnt[i1] = t - box.Extent(i1);
                    pnt[i2] = -box.Extent(i2);
                }
            } else {
                lenSqr += dir[i1] * dir[i1];
                delta = dir[i0] * PmE[i0] + dir[i1] * PmE[i1] + dir[i2] * PpE[i2];
                param = (-1.0f / lenSqr) * delta;
                sqr += PmE[i0] * PmE[i0] + PmE[i1] * PmE[i1] + PpE[i2] * PpE[i2] + delta * param;
                if (pfL) {
                    *pfL = param;
                    pnt[i0] = box.Extent(i0);
                    pnt[i1] = box.Extent(i1);
                    pnt[i2] = -box.Extent(i2);
                }
            }
        } else {
            lenSqr = dir[i0] * dir[i0] + dir[i1] * dir[i1];
            tmp = lenSqr * PpE[i2] - dir[i2] * (dir[i0] * PmE[i0] + dir[i1] * PpE[i1]);
            if (tmp >= 0.0f) {
                if (tmp <= box.Extent(i2) * lenSqr * 2.0f) {
                    t = tmp / lenSqr;
                    lenSqr += dir[i2] * dir[i2];
                    tmp = PpE[i2] - t;
                    delta = dir[i0] * PmE[i0] + dir[i1] * PpE[i1] + dir[i2] * tmp;
                    param = (-1.0f / lenSqr) * delta;
                    sqr += PmE[i0] * PmE[i0] + PpE[i1] * PpE[i1] + tmp * tmp + delta * param;
                    if (pfL) {
                        *pfL = param;
                        pnt[i0] = box.Extent(i0);
                        pnt[i1] = -box.Extent(i1);
                        pnt[i2] = t - box.Extent(i2);
                    }
                } else {
                    lenSqr += dir[i2] * dir[i2];
                    delta = dir[i0] * PmE[i0] + dir[i1] * PpE[i1] + dir[i2] * PmE[i2];
                    param = (-1.0f / lenSqr) * delta;
                    sqr += PmE[i0] * PmE[i0] + PpE[i1] * PpE[i1] + PmE[i2] * PmE[i2] + delta * param;
                    if (pfL) {
                        *pfL = param;
                        pnt[i0] = box.Extent(i0);
                        pnt[i1] = -box.Extent(i1);
                        pnt[i2] = box.Extent(i2);
                    }
                }
            } else {
                // Closest at the (-i1, -i2) corner.
                lenSqr += dir[i2] * dir[i2];
                delta = dir[i0] * PmE[i0] + dir[i1] * PpE[i1] + dir[i2] * PpE[i2];
                param = (-1.0f / lenSqr) * delta;
                sqr += PmE[i0] * PmE[i0] + PpE[i1] * PpE[i1] + PpE[i2] * PpE[i2] + delta * param;
                if (pfL) {
                    *pfL = param;
                    pnt[i0] = box.Extent(i0);
                    pnt[i1] = -box.Extent(i1);
                    pnt[i2] = -box.Extent(i2);
                }
            }
        }
    }
}

static void LB_CaseNoZeros(EXMaths::vec_xyzw &pnt, const EXMaths::vec_xyzw &dir, const EXBox &box,
                           float *pfL, float &sqr)
{
    EXVector PmE(pnt.x - box.Extent(0), pnt.y - box.Extent(1), pnt.z - box.Extent(2), 0.0f);
    if (dir.y * PmE.x >= dir.x * PmE.y) {
        if (dir.z * PmE.x >= dir.x * PmE.z)
            LB_Face(0, 1, 2, pnt, dir, box, PmE, pfL, sqr);
        else
            LB_Face(2, 0, 1, pnt, dir, box, PmE, pfL, sqr);
    } else {
        if (dir.z * PmE.y >= dir.y * PmE.z)
            LB_Face(1, 2, 0, pnt, dir, box, PmE, pfL, sqr);
        else
            LB_Face(2, 0, 1, pnt, dir, box, PmE, pfL, sqr);
    }
}

// Clamp one coordinate of pnt into the box slab, accumulating the squared overshoot.
#define LB_CLAMP(i)                                                                                \
    if (pnt[i] < -box.Extent(i)) {                                                                 \
        delta = pnt[i] + box.Extent(i);                                                            \
        sqr += delta * delta;                                                                      \
        pnt[i] = -box.Extent(i);                                                                   \
    } else if (pnt[i] > box.Extent(i)) {                                                           \
        delta = pnt[i] - box.Extent(i);                                                            \
        sqr += delta * delta;                                                                      \
        pnt[i] = box.Extent(i);                                                                    \
    }

static void LB_Case0(int i0, int i1, int i2, EXMaths::vec_xyzw &pnt, const EXMaths::vec_xyzw &dir,
                     const EXBox &box, float *pfL, float &sqr)
{
    float PmE0 = pnt[i0] - box.Extent(i0);
    float PmE1 = pnt[i1] - box.Extent(i1);
    float prod0 = dir[i1] * PmE0;
    float prod1 = dir[i0] * PmE1;
    float delta, inv;
    if (prod0 >= prod1) {
        pnt[i0] = box.Extent(i0);
        float PpE1 = pnt[i1] + box.Extent(i1);
        delta = prod0 - dir[i0] * PpE1;
        if (delta >= 0.0f) {
            inv = 1.0f / (dir[i0] * dir[i0] + dir[i1] * dir[i1]);
            sqr += delta * delta * inv;
            if (pfL) {
                pnt[i1] = -box.Extent(i1);
                *pfL = -(dir[i0] * PmE0 + dir[i1] * PpE1) * inv;
            }
        } else if (pfL) {
            inv = 1.0f / dir[i0];
            pnt[i1] -= prod0 * inv;
            *pfL = -(inv * PmE0);
        }
    } else {
        pnt[i1] = box.Extent(i1);
        float PpE0 = pnt[i0] + box.Extent(i0);
        delta = prod1 - dir[i1] * PpE0;
        if (delta >= 0.0f) {
            inv = 1.0f / (dir[i0] * dir[i0] + dir[i1] * dir[i1]);
            sqr += delta * delta * inv;
            if (pfL) {
                pnt[i0] = -box.Extent(i0);
                *pfL = -(dir[i0] * PpE0 + dir[i1] * PmE1) * inv;
            }
        } else if (pfL) {
            inv = 1.0f / dir[i1];
            pnt[i0] -= prod1 * inv;
            *pfL = -(inv * PmE1);
        }
    }
    LB_CLAMP(i2)
}

static void LB_Case00(int i0, int i1, int i2, EXMaths::vec_xyzw &pnt, const EXMaths::vec_xyzw &dir,
                      const EXBox &box, float *pfL, float &sqr)
{
    float delta;
    // Retail reads extent 0 here whatever i0 is (the i0 extent is what the geometry calls for).
    if (pfL) *pfL = (box.Extent(0) - pnt[i0]) / dir[i0];
    pnt[i0] = box.Extent(i0);
    LB_CLAMP(i1)
    LB_CLAMP(i2)
}

static void LB_Case000(EXMaths::vec_xyzw &pnt, const EXBox &box, float &sqr)
{
    float delta;
    LB_CLAMP(0)
    LB_CLAMP(1)
    LB_CLAMP(2)
}

float EXSqrDistance(const EXLine &line, const EXBox &box, float *pt, float *pb0, float *pb1, float *pb2)
{
    EXMaths::vec_xyzw diff = (const EXMaths::vec_xyzw &)line.Origin() - box.Centre();
    EXVector a2 = box.Axis(2);
    EXVector a1 = box.Axis(1);
    EXVector a0 = box.Axis(0);
    EXMaths::vec_xyzw pnt, dir;
    pnt.set(diff.DotXYZ(a0), diff.DotXYZ(a1), diff.DotXYZ(a2), 0.0f);
    dir.set(line.Direction().DotXYZ(a0), line.Direction().DotXYZ(a1), line.Direction().DotXYZ(a2), 0.0f);
    EXFixedArray<s8, 3> reflect;
    int i;
    for (i = 0; i < 3; i++) {
        if (dir[i] < 0.0f) {
            pnt[i] = -pnt[i];
            dir[i] = -dir[i];
            reflect[i] = 1;
        } else {
            reflect[i] = 0;
        }
    }
    float sqr = 0.0f;
    if (dir.x > 0.0f) {
        if (dir.y > 0.0f) {
            if (dir.z > 0.0f)
                LB_CaseNoZeros(pnt, dir, box, pt, sqr);
            else
                LB_Case0(0, 1, 2, pnt, dir, box, pt, sqr);
        } else if (dir.z > 0.0f) {
            LB_Case0(0, 2, 1, pnt, dir, box, pt, sqr);
        } else {
            LB_Case00(0, 1, 2, pnt, dir, box, pt, sqr);
        }
    } else if (dir.y > 0.0f) {
        if (dir.z > 0.0f)
            LB_Case0(1, 2, 0, pnt, dir, box, pt, sqr);
        else
            LB_Case00(1, 0, 2, pnt, dir, box, pt, sqr);
    } else if (dir.z > 0.0f) {
        LB_Case00(2, 0, 1, pnt, dir, box, pt, sqr);
    } else {
        LB_Case000(pnt, box, sqr);
        if (pt) *pt = 0.0f;
    }
    if (pt) {
        for (i = 0; i < 3; i++)
            if (reflect[i]) pnt[i] = -pnt[i];
        *pb0 = pnt.x;
        *pb1 = pnt.y;
        *pb2 = pnt.z;
    }
    return sqr;
}

// Line/line family (closest points on two parametric lines, then clamped for rays and segments).
// a00, a11: squared direction lengths; a01: minus their dot; b0, b1: the origin offset against each
// direction (b1 negated); c: the squared origin offset; det: |a00 a11 - a01^2| (parallel below
// 1e-5). All NEAR (see check).
#define EXDIST_SETUP(L0, L1)                                                                       \
    float dx = L0.Origin().x - L1.Origin().x;                                                      \
    float dy = L0.Origin().y - L1.Origin().y;                                                      \
    float dz = L0.Origin().z - L1.Origin().z;                                                      \
    const EXVector &u = L0.Direction();                                                            \
    const EXVector &v = L1.Direction();                                                            \
    float a01 = -(u.x * v.x + u.y * v.y + u.z * v.z);                                              \
    float a00 = u.x * u.x + u.y * u.y + u.z * u.z;                                                 \
    float a11 = v.x * v.x + v.y * v.y + v.z * v.z;                                                 \
    float det = (float)fabs(a11 * a00 - a01 * a01);                                                \
    float b0 = u.z * dz + u.y * dy + u.x * dx;

float EXSqrDistance(const EXLine &l0, const EXLine &l1, float *ps, float *pt)
{
    EXDIST_SETUP(l0, l1)
    float s, t, d;
    if (det < 1e-5f) {
        t = 0.0f;
        s = 0.0f - b0 / a00;
        d = s * b0;
    } else {
        float inv = 1.0f / det;
        float b1 = 0.0f - (v.z * dz + v.y * dy + v.x * dx);
        t = (b0 * a01 - b1 * a00) * inv;
        s = (b1 * a01 - b0 * a11) * inv;
        d = (t * a11 + s * a01 + b1 * 2.0f) * t + (t * a01 + s * a00 + b0 * 2.0f) * s;
    }
    d = d + dz * dz + dy * dy + dx * dx;
    if (ps) *ps = s;
    if (pt) *pt = t;
    return (float)fabs(d);
}

float EXSqrDistance(const EXLine &l0, const EXRay &l1, float *ps, float *pt)
{
    EXDIST_SETUP(l0, l1)
    float s, t, d;
    if (1e-5f <= det) {
        float b1 = 0.0f - (v.z * dz + v.y * dy + v.x * dx);
        float tn = b0 * a01 - b1 * a00;
        if (0.0f <= tn) {
            float inv = 1.0f / det;
            s = (b1 * a01 - b0 * a11) * inv;
            t = inv * tn;
            d = (s * a01 + t * a11 + b1 * 2.0f) * t + (s * a00 + t * a01 + b0 * 2.0f) * s;
            goto done;
        }
    }
    t = 0.0f;
    s = 0.0f - b0 / a00;
    d = s * b0;
done:
    d = d + dz * dz + dy * dy + dx * dx;
    if (ps) *ps = s;
    if (pt) *pt = t;
    return (float)fabs(d);
}

float EXSqrDistance(const EXLine &l0, const EXLineSeg &l1, float *ps, float *pt)
{
    EXDIST_SETUP(l0, l1)
    float c = dz * dz + dy * dy + dx * dx;
    float s, t, d;
    if (1e-5f <= det) {
        float b1 = 0.0f - (v.z * dz + v.y * dy + v.x * dx);
        float tn = b0 * a01 - b1 * a00;
        if (0.0f <= tn) {
            if (det < tn) {
                s = 0.0f - (b0 + a01) / a00;
                d = (b0 + a01) * s + b1 * 2.0f + c + a11;
                t = 1.0f;
            } else {
                s = (b1 * a01 - b0 * a11) * (1.0f / det);
                t = (1.0f / det) * tn;
                d = (s * a01 + t * a11 + b1 * 2.0f) * t + (s * a00 + t * a01 + b0 * 2.0f) * s + c;
            }
            goto done;
        }
    }
    t = 0.0f;
    s = 0.0f - b0 / a00;
    d = s * b0 + c;
done:
    if (ps) *ps = s;
    if (pt) *pt = t;
    return (float)fabs(d);
}

float EXSqrDistance(const EXRay &l0, const EXRay &l1, float *ps, float *pt)
{
    EXDIST_SETUP(l0, l1)
    float d = dz * dz + dy * dy + dx * dx;
    float s, t, k, m;
    if (det < 1e-5f) {
        if (a01 > 0.0f) {
            t = 0.0f;
            if (b0 >= 0.0f) {
                s = 0.0f;
                goto done;
            }
            s = 0.0f - b0 / a00;
            goto edge0;
        }
        if (b0 < 0.0f) {
            s = 0.0f - b0 / a00;
            goto edge0;
        }
        m = 0.0f - (v.z * dz + v.y * dy + v.x * dx);
        s = 0.0f;
        k = 0.0f - m / a11;
        t = k;
        d = k * m + d;
        goto done;
    } else {
        float b1 = 0.0f - (v.z * dz + v.y * dy + v.x * dx);
        float sn = b1 * a01 - b0 * a11;
        float tn = b0 * a01 - b1 * a00;
        if (sn < 0.0f) {
            if (tn >= 0.0f || b0 >= 0.0f) {
                s = 0.0f;
                if (b1 < 0.0f) {
                    t = 0.0f - b1 / a11;
                    d = t * b1 + d;
                } else {
                    t = 0.0f;
                }
                goto done;
            }
            s = 0.0f - b0 / a00;
        } else {
            if (tn >= 0.0f) {
                float inv = 1.0f / det;
                s = inv * sn;
                t = inv * tn;
                d = (t * a11 + s * a01 + b1 * 2.0f) * t + (t * a01 + s * a00 + b0 * 2.0f) * s + d;
                goto done;
            }
            t = 0.0f;
            if (b0 >= 0.0f) {
                s = 0.0f;
                goto done;
            }
            s = 0.0f - b0 / a00;
        }
    }
edge0:
    t = 0.0f;
    d = s * b0 + d;
done:
    if (ps) *ps = s;
    if (pt) *pt = t;
    return (float)fabs(d);
}

float EXSqrDistance(const EXRay &l0, const EXLineSeg &l1, float *ps, float *pt)
{
    EXDIST_SETUP(l0, l1)
    float d = dz * dz + dy * dy + dx * dx;
    float s, t = 1.0f, b1, sn, tn;
    if (det < 1e-5f) {
        if (a01 > 0.0f) goto neg_t;
        b1 = 0.0f - (v.z * dz + v.y * dy + v.x * dx);
        if (b0 + a01 < 0.0f) goto edge1;
        goto end1;
    }
    b1 = 0.0f - (v.z * dz + v.y * dy + v.x * dx);
    sn = b1 * a01 - b0 * a11;
    tn = b0 * a01 - b1 * a00;
    if (sn >= 0.0f) {
        if (tn >= 0.0f) {
            b1 = b1 * 2.0f;
            if (tn <= det) {
                float inv = 1.0f / det;
                s = inv * sn;
                t = inv * tn;
                d = (t * a11 + s * a01 + b1) * t + (t * a01 + s * a00 + b0 * 2.0f) * s + d;
                goto done;
            }
            if (0.0f - a01 <= b0) {
                d = b1 + d + a11;
                s = 0.0f;
                goto done;
            }
            goto edge1b;
        }
    neg_t:
        if (b0 >= 0.0f) {
            s = 0.0f;
            t = 0.0f;
            goto done;
        }
    edge0:
        s = 0.0f - b0 / a00;
        d = s * b0 + d;
        t = 0.0f;
        goto done;
    }
    if (tn > 0.0f) {
        if (det < tn && b0 + a01 < 0.0f) goto edge1;
    } else if (b0 < 0.0f) {
        goto edge0;
    }
    s = 0.0f;
    if (b1 >= 0.0f) {
        t = 0.0f;
        goto done;
    }
    if (0.0f - b1 < a11) {
        t = 0.0f - b1 / a11;
        d = t * b1 + d;
        goto done;
    }
end1:
    s = 0.0f;
    d = b1 * 2.0f + d + a11;
    goto done;
edge1:
    b1 = b1 * 2.0f;
edge1b:
    s = 0.0f - (b0 + a01) / a00;
    d = (b0 + a01) * s + b1 + d + a11;
done:
    if (ps) *ps = s;
    if (pt) *pt = t;
    return (float)fabs(d);
}

float EXSqrDistance(const EXLineSeg &l0, const EXLineSeg &l1, float *ps, float *pt)
{
    EXDIST_SETUP(l0, l1)
    float d = dz * dz + dy * dy + dx * dx;
    float t = 1.0f, s = 1.0f, b1, sn, tn, tmp;
    if (det < 1e-5f) {
        if (a01 > 0.0f) {
            if (b0 >= 0.0f) {
                t = 0.0f;
                s = 0.0f;
                goto done;
            }
            if (a00 < 0.0f - b0) {
                b1 = 0.0f - (v.z * dz + v.y * dy + v.x * dx);
                if (0.0f - (b0 + a00) < a01) {
                    t = 0.0f - (b0 + a00) / a01;
                    d = ((b1 + a01) * 2.0f + t * a11) * t + b0 * 2.0f + d + a00;
                } else {
                    d = (b1 + b0 + a01) * 2.0f + d + a11 + a00;
                }
                goto done;
            }
        } else {
            if (a00 <= 0.0f - b0) goto s1t0;
            if (b0 > 0.0f) {
                b1 = (0.0f - (v.z * dz + v.y * dy + v.x * dx)) * 2.0f;
                s = 0.0f;
                if (0.0f - a01 <= b0) {
                    d = b1 + d + a11;
                    goto done;
                }
                t = 0.0f - b0 / a01;
                tmp = (t * a11 + b1) * t;
                goto add;
            }
        }
    t0:
        s = 0.0f - b0 / a00;
        d = s * b0 + d;
        t = 0.0f;
        goto done;
    }
    b1 = 0.0f - (v.z * dz + v.y * dy + v.x * dx);
    sn = b1 * a01 - b0 * a11;
    tn = b0 * a01 - b1 * a00;
    if (sn < 0.0f) {
        if (tn < 0.0f) {
            if (b0 < 0.0f) {
                t = 0.0f;
                if (a00 <= 0.0f - b0) {
                    d = b0 * 2.0f + d + a00;
                    goto done;
                }
                goto t0;
            }
        } else if (det < tn && (tmp = b0 + a01) < 0.0f) {
            if (a00 <= 0.0f - tmp) {
                d = (tmp + b1) * 2.0f + d + a11 + a00;
                goto done;
            }
        t1:
            s = 0.0f - (b0 + a01) / a00;
            d = (b0 + a01) * s + b1 * 2.0f + d + a11;
            goto done;
        }
        s = 0.0f;
        if (b1 >= 0.0f) {
            t = 0.0f;
            goto done;
        }
        if (0.0f - b1 < a11) {
            t = 0.0f - b1 / a11;
            tmp = t * b1;
            goto add;
        }
    s0t1:
        d = b1 * 2.0f + d + a11;
        s = 0.0f;
        goto done;
    }
    if (sn <= det) {
        if (tn >= 0.0f) {
            if (tn <= det) {
                float inv = 1.0f / det;
                s = inv * sn;
                t = inv * tn;
                tmp = (t * a11 + s * a01 + b1 * 2.0f) * t + (t * a01 + s * a00 + b0 * 2.0f) * s;
                goto add;
            }
            tmp = b0 + a01;
            if (tmp < 0.0f) {
                if (a00 <= 0.0f - tmp) {
                    d = (tmp + b1) * 2.0f + d + a11 + a00;
                    goto done;
                }
                goto t1;
            }
            goto s0t1;
        }
        if (b0 >= 0.0f) {
            t = 0.0f;
            s = 0.0f;
            goto done;
        }
        if (0.0f - b0 < a00) goto t0;
    } else if (tn < 0.0f) {
        if (0.0f - b0 < a00) {
            if (b0 >= 0.0f) {
                t = 0.0f;
                s = 0.0f;
                goto done;
            }
            goto t0;
        }
        tmp = b1 + a01;
        if (tmp < 0.0f) {
            if (a11 <= 0.0f - tmp) {
                d = (tmp + b0) * 2.0f + d + a11 + a00;
                goto done;
            }
            goto s1;
        }
    } else if (det < tn) {
        tmp = b0 + a01;
        if (0.0f - tmp <= a00) {
            if (tmp >= 0.0f) {
                d = b1 * 2.0f + d + a11;
                s = 0.0f;
            } else {
                s = 0.0f - tmp / a00;
                d = tmp * s + b1 * 2.0f + d + a11;
            }
            goto done;
        }
        b1 = b1 + a01;
        if (b1 < 0.0f) {
            if (0.0f - b1 < a11) {
                t = 0.0f - b1 / a11;
                d = b1 * t + b0 * 2.0f + d + a00;
            } else {
                d = (b1 + b0) * 2.0f + d + a11 + a00;
            }
            goto done;
        }
    } else {
        tmp = b1 + a01;
        if (tmp < 0.0f) {
            if (a11 <= 0.0f - tmp) {
                d = (tmp + b0) * 2.0f + d + a11 + a00;
                goto done;
            }
        s1:
            t = 0.0f - (b1 + a01) / a11;
            d = (b1 + a01) * t + b0 * 2.0f + d + a00;
            goto done;
        }
    }
s1t0:
    d = b0 * 2.0f + d + a00;
    t = 0.0f;
    goto done;
add:
    d = tmp + d;
done:
    if (ps) *ps = s;
    if (pt) *pt = t;
    return (float)fabs(d);
}

float EXSqrDistance(const EXMaths::vec_xyzw &p, const EXRectangle &r, float *ps0, float *ps1)
{
    EXMaths::vec_xyzw diff = (const EXMaths::vec_xyzw &)r.Origin() - p;
    float a00 = r.Edge0().LengthXYZSquared();
    float a11 = r.Edge1().LengthXYZSquared();
    float b0 = ((EXMaths::vec_xyzw)r.Edge0()).DotXYZ(diff);
    float b1 = ((EXMaths::vec_xyzw)r.Edge1()).DotXYZ(diff);
    float dx = diff.x, dy = diff.y, dz = diff.z;
    float s1n = 0.0f - b1;
    float s0n = 0.0f - b0;
    float d = dz * dz + dy * dy + dx * dx;
    float s0, s1;
    if (s0n < 0.0f) {
        s0 = 0.0f;
    } else if (s0n <= a00) {
        s0 = s0n / a00;
        d = s0 * b0 + d;
    } else {
        s0 = 1.0f;
        d = b0 * 2.0f + d + a00;
    }
    if (s1n < 0.0f) {
        s1 = 0.0f;
    } else if (s1n <= a11) {
        s1 = s1n / a11;
        d = s1 * b1 + d;
    } else {
        s1 = 1.0f;
        d = b1 * 2.0f + d + a11;
    }
    if (ps0) *ps0 = s0;
    if (ps1) *ps1 = s1;
    return (float)fabs(d);
}

float EXSqrDistance(const EXMaths::vec_xyzw &p, const EXBox &b, float *pb0, float *pb1, float *pb2)
{
    EXMaths::vec_xyzw diff = p - b.Centre();
    EXVector a2 = b.Axis(2);
    EXVector a1 = b.Axis(1);
    EXVector a0 = b.Axis(0);
    float c0 = diff.DotXYZ(a0);
    float c1 = diff.DotXYZ(a1);
    float c2 = diff.DotXYZ(a2);
    float d = 0.0f, k;
    if (c0 < 0.0f - b.Extent(0)) {
        k = c0 + b.Extent(0);
        d += k * k;
        c0 = 0.0f - b.Extent(0);
    } else if (c0 > b.Extent(0)) {
        k = c0 - b.Extent(0);
        d += k * k;
        c0 = b.Extent(0);
    }
    if (c1 < 0.0f - b.Extent(1)) {
        k = c1 + b.Extent(1);
        d += k * k;
        c1 = 0.0f - b.Extent(1);
    } else if (c1 > b.Extent(1)) {
        k = c1 - b.Extent(1);
        d += k * k;
        c1 = b.Extent(1);
    }
    if (c2 < 0.0f - b.Extent(2)) {
        k = c2 + b.Extent(2);
        d += k * k;
        c2 = 0.0f - b.Extent(2);
    } else if (c2 > b.Extent(2)) {
        k = c2 - b.Extent(2);
        d += k * k;
        c2 = b.Extent(2);
    }
    if (pb0) {
        *pb0 = c0;
        *pb1 = c1;
        *pb2 = c2;
    }
    return d;
}

float EXSqrDistance(const EXMaths::vec_xyzw &p, const EXTriangle &tri, float *ps, float *pt)
{
    EXMaths::vec_xyzw diff = (const EXMaths::vec_xyzw &)tri.Origin() - p;
    float a00 = tri.Edge(0).LengthXYZSquared();
    float a01 = tri.Edge(1).DotXYZ(tri.Edge(0));
    float a11 = tri.Edge(1).LengthXYZSquared();
    float b0 = tri.Edge(0).DotXYZ(diff);
    float b1 = tri.Edge(1).DotXYZ(diff);
    float c = diff.LengthXYZSquared();
    float det = (float)fabs(a00 * a11 - a01 * a01);
    float s = a01 * b1 - a11 * b0;
    float t = a01 * b0 - a00 * b1;
    float d, num, den;
    if (s + t <= det) {
        if (s < 0.0f) {
            if (t < 0.0f) {
                if (b0 < 0.0f) {
                    t = 0.0f;
                    if (0.0f - b0 >= a00) {
                        s = 1.0f;
                        d = b0 * 2.0f + c + a00;
                    } else {
                        s = 0.0f - b0 / a00;
                        d = s * b0 + c;
                    }
                } else {
                    s = 0.0f;
                    if (b1 >= 0.0f) {
                        t = 0.0f;
                        d = c;
                    } else if (0.0f - b1 >= a11) {
                        t = 1.0f;
                        d = b1 * 2.0f + c + a11;
                    } else {
                        t = 0.0f - b1 / a11;
                        d = t * b1 + c;
                    }
                }
            } else {
                s = 0.0f;
                if (b1 >= 0.0f) {
                    t = 0.0f;
                    d = c;
                } else if (0.0f - b1 >= a11) {
                    t = 1.0f;
                    d = b1 * 2.0f + c + a11;
                } else {
                    t = 0.0f - b1 / a11;
                    d = t * b1 + c;
                }
            }
        } else if (t < 0.0f) {
            t = 0.0f;
            if (b0 >= 0.0f) {
                s = 0.0f;
                d = c;
            } else if (0.0f - b0 >= a00) {
                s = 1.0f;
                d = b0 * 2.0f + c + a00;
            } else {
                s = 0.0f - b0 / a00;
                d = s * b0 + c;
            }
        } else {
            float inv = det != 0.0f ? 1.0f / det : 0.0f;
            s *= inv;
            t *= inv;
            d = (t * a11 + s * a01 + b1 * 2.0f) * t + (t * a01 + s * a00 + b0 * 2.0f) * s + c;
        }
        goto done;
    }
    if (s < 0.0f) {
        float tmp0 = b0 + a01;
        float tmp1 = b1 + a11;
        if (tmp1 > tmp0) {
            num = tmp1 - tmp0;
            den = a00 - a01 * 2.0f + a11;
            if (num >= den) {
                s = 1.0f;
                t = 0.0f;
                d = b0 * 2.0f + c + a00;
            } else {
                s = num / den;
                t = 1.0f - s;
                d = (t * a11 + s * a01 + b1 * 2.0f) * t + (t * a01 + s * a00 + b0 * 2.0f) * s + c;
            }
        } else {
            s = 0.0f;
            if (tmp1 <= 0.0f) {
                t = 1.0f;
                d = b1 * 2.0f + c + a11;
            } else if (b1 >= 0.0f) {
                t = 0.0f;
                d = c;
            } else {
                t = 0.0f - b1 / a11;
                d = t * b1 + c;
            }
        }
    } else if (t < 0.0f) {
        float tmp0 = b1 + a01;
        float tmp1 = b0 + a00;
        if (tmp1 > tmp0) {
            num = tmp1 - tmp0;
            den = a00 - a01 * 2.0f + a11;
            if (num >= den) {
                t = 1.0f;
                s = 0.0f;
                d = b1 * 2.0f + c + a11;
            } else {
                t = num / den;
                s = 1.0f - t;
                d = (t * a11 + s * a01 + b1 * 2.0f) * t + (t * a01 + s * a00 + b0 * 2.0f) * s + c;
            }
        } else {
            t = 0.0f;
            if (tmp1 <= 0.0f) {
                s = 1.0f;
                d = b0 * 2.0f + c + a00;
            } else if (b0 >= 0.0f) {
                s = 0.0f;
                d = c;
            } else {
                s = 0.0f - b0 / a00;
                d = s * b0 + c;
            }
        }
    } else {
        num = b1 + a11 - a01 - b0;
        if (num <= 0.0f) {
            s = 0.0f;
            t = 1.0f;
            d = b1 * 2.0f + c + a11;
        } else {
            den = a00 - a01 * 2.0f + a11;
            if (num >= den) {
                s = 1.0f;
                t = 0.0f;
                d = b0 * 2.0f + c + a00;
            } else {
                s = num / den;
                t = 1.0f - s;
                d = (t * a11 + s * a01 + b1 * 2.0f) * t + (t * a01 + s * a00 + b0 * 2.0f) * s + c;
            }
        }
    }
done:
    if (ps) *ps = s;
    if (pt) *pt = t;
    return (float)fabs(d);
}

float EXSqrDistance(const EXLineSeg &seg, const EXBox &box, float *pt, float *pb0, float *pb1, float *pb2)
{
    EXLine line(seg.Origin(), seg.Direction());
    float lt, b0, b1, b2;
    float d = EXSqrDistance(line, box, &lt, &b0, &b1, &b2);
    float t;
    if (lt >= 0.0f) {
        if (lt <= 1.0f) {
            if (pt) {
                *pt = lt;
                *pb0 = b0;
                *pb1 = b1;
                *pb2 = b2;
            }
            return d;
        }
        t = 1.0f;
        line.Origin() = seg.Origin() + seg.Direction();
    } else {
        t = 0.0f;
        line.Origin() = seg.Origin();
    }
    d = EXSqrDistance(line.Origin(), box, pb0, pb1, pb2);
    if (pt) *pt = t;
    return d;
}

// Line against triangle: solve the 3x3 system for the closest line point r and triangle point
// (s, t), and when that point falls outside the triangle (or the line is parallel to its plane)
// fall back to the closest of the triangle edges.
float EXSqrDistance(const EXLine &line, const EXTriangle &tri, float *pr, float *ps, float *pt)
{
    EXVector diff = tri.Origin() - line.Origin();
    float a00 = line.Direction().LengthXYZSquared();
    float a01 = -line.Direction().DotXYZ(tri.Edge0());
    float a02 = -line.Direction().DotXYZ(tri.Edge1());
    float a11 = tri.Edge0().LengthXYZSquared();
    float a12 = tri.Edge0().DotXYZ(tri.Edge1());
    float a22 = tri.Edge1().LengthXYZSquared();
    float b0 = -diff.DotXYZ(line.Direction());
    float b1 = diff.DotXYZ(tri.Edge0());
    float b2 = diff.DotXYZ(tri.Edge1());
    float cof00 = a11 * a22 - a12 * a12;
    float cof01 = a02 * a12 - a01 * a22;
    float cof02 = a01 * a12 - a02 * a11;
    float det = a00 * cof00 + a01 * cof01 + a02 * cof02;
    EXLineSeg seg;
    float d, d0, r, s, t, r0, s0, t0;
    EXVector n = tri.Edge0().Cross(tri.Edge1());
    float nd = n.DotXYZ(line.Direction());
    if (nd * nd < n.LengthXYZSquared() * a00 * 1e-5f) {
        // Parallel to the triangle plane.
        seg.Origin() = tri.Origin();
        seg.Direction() = tri.Edge0();
        d = EXSqrDistance(line, seg, &r, &s);
        t = 0.0f;
        seg.Direction() = tri.Edge1();
        d0 = EXSqrDistance(line, seg, &r0, &t0);
        if (d0 < d) {
            d = d0;
            r = r0;
            s = 0.0f;
            t = t0;
        }
        seg.Origin() = tri.Origin() + tri.Edge0();
        seg.Direction() = tri.Edge1() - tri.Edge0();
        d0 = EXSqrDistance(line, seg, &r0, &t0);
        if (d0 < d) {
            d = d0;
            r = r0;
            s = 1.0f - t0;
            t = t0;
        }
    } else {
        float cof11 = a00 * a22 - a02 * a02;
        float cof12 = a02 * a01 - a00 * a12;
        float cof22 = a00 * a11 - a01 * a01;
        float inv = det != 0.0f ? 1.0f / det : 0.0f;
        float rhs0 = -b0 * inv;
        float rhs1 = -b1 * inv;
        float rhs2 = -b2 * inv;
        r = cof00 * rhs0 + cof01 * rhs1 + cof02 * rhs2;
        s = cof01 * rhs0 + cof11 * rhs1 + cof12 * rhs2;
        t = cof02 * rhs0 + cof12 * rhs1 + cof22 * rhs2;
        if (s + t <= 1.0f) {
            if (s < 0.0f) {
                seg.Origin() = tri.Origin();
                seg.Direction() = tri.Edge1();
                if (t < 0.0f) {
                    d = EXSqrDistance(line, seg, 0, &t);
                    s = 0.0f;
                    seg.Origin() = tri.Origin();
                    seg.Direction() = tri.Edge0();
                    d0 = EXSqrDistance(line, seg, 0, &s0);
                    if (d0 < d) {
                        d = d0;
                        s = s0;
                        t = 0.0f;
                    }
                } else {
                    d = EXSqrDistance(line, seg, 0, &t);
                    s = 0.0f;
                }
            } else if (t < 0.0f) {
                seg.Origin() = tri.Origin();
                seg.Direction() = tri.Edge0();
                d = EXSqrDistance(line, seg, 0, &s);
                t = 0.0f;
            } else {
                d = 0.0f;
            }
        } else {
            if (s >= 0.0f) {
                if (t >= 0.0f) {
                    seg.Origin() = tri.Origin() + tri.Edge0();
                    seg.Direction() = tri.Edge1() - tri.Edge0();
                    d = EXSqrDistance(line, seg, 0, &t);
                    s = 1.0f - t;
                    goto done;
                }
                seg.Origin() = tri.Origin();
                seg.Direction() = tri.Edge0();
                d = EXSqrDistance(line, seg, 0, &s);
                t = 0.0f;
            } else {
                seg.Origin() = tri.Origin();
                seg.Direction() = tri.Edge1();
                d = EXSqrDistance(line, seg, 0, &t);
                s = 0.0f;
            }
            seg.Origin() = tri.Origin() + tri.Edge0();
            seg.Direction() = tri.Edge1() - tri.Edge0();
            d0 = EXSqrDistance(line, seg, 0, &t0);
            if (d0 < d) {
                d = d0;
                t = t0;
                s = 1.0f - t0;
            }
        }
    }
done:
    if (pr) *pr = r;
    if (ps) *ps = s;
    if (pt) *pt = t;
    return (float)fabs(d);
}

// Line against rectangle: solve for the closest line point r and rectangle point (s, t); outside
// the unit square (or parallel) fall back to the nearest rectangle edges.
float EXSqrDistance(const EXLine &line, const EXRectangle &rect, float *pr, float *ps, float *pt)
{
    EXMaths::vec_xyzw diff = (const EXMaths::vec_xyzw &)rect.Origin() - line.Origin();
    float a00 = line.Direction().LengthXYZSquared();
    float a01 = -line.Direction().DotXYZ(rect.Edge0());
    float a02 = -line.Direction().DotXYZ(rect.Edge1());
    float a11 = rect.Edge0().LengthXYZSquared();
    float a22 = rect.Edge1().LengthXYZSquared();
    float cof01 = -a22 * a01;
    float cof02 = -a11 * a02;
    float det = a22 * a11 * a00 + cof01 * a01 + cof02 * a02;
    EXLineSeg seg;
    float d, d0, r, s, t, r0, s0, t0;
    if ((float)fabs(det) < 1e-5f) {
        // Parallel to the rectangle plane.
        seg.Origin() = rect.Origin();
        seg.Direction() = rect.Edge0();
        d = EXSqrDistance(line, seg, &r, &s);
        t = 0.0f;
        seg.Direction() = rect.Edge1();
        d0 = EXSqrDistance(line, seg, &r0, &t0);
        s0 = 0.0f;
        if (d0 < d) {
            d = d0;
            r = r0;
            s = s0;
            t = t0;
        }
        seg.Origin() = rect.Origin() + rect.Edge1();
        seg.Direction() = rect.Edge0();
        d0 = EXSqrDistance(line, seg, &r0, &s0);
        t0 = 1.0f;
        if (d0 < d) {
            d = d0;
            r = r0;
            s = s0;
            t = t0;
        }
        seg.Origin() = rect.Origin() + rect.Edge0();
        seg.Direction() = rect.Edge1();
        d0 = EXSqrDistance(line, seg, &r0, &t0);
        if (d0 < d) {
            d = d0;
            r = r0;
            s = 1.0f;
            t = t0;
        }
    } else {
        float cof12 = a02 * a01;
        float inv = 1.0f / det;
        float rhs0 = -(-diff.DotXYZ(line.Direction())) * inv;
        float rhs1 = -diff.DotXYZ(rect.Edge0()) * inv;
        float rhs2 = -diff.DotXYZ(rect.Edge1()) * inv;
        r = a11 * a22 * rhs0 + cof01 * rhs1 + cof02 * rhs2;
        s = cof01 * rhs0 + (a00 * a22 - a02 * a02) * rhs1 + cof12 * rhs2;
        t = cof02 * rhs0 + cof12 * rhs1 + (a00 * a11 - a01 * a01) * rhs2;
        if (s >= 0.0f) {
            if (s <= 1.0f) {
                if (t >= 0.0f) {
                    if (t <= 1.0f) {
                        d = 0.0f;
                    } else {
                        seg.Origin() = rect.Origin() + rect.Edge1();
                        seg.Direction() = rect.Edge0();
                        d = EXSqrDistance(line, seg, 0, &s);
                        t = 1.0f;
                    }
                } else {
                    seg.Origin() = rect.Origin();
                    seg.Direction() = rect.Edge0();
                    d = EXSqrDistance(line, seg, 0, &s);
                    t = 0.0f;
                }
            } else if (t >= 0.0f) {
                seg.Origin() = rect.Origin() + rect.Edge0();
                seg.Direction() = rect.Edge1();
                if (t <= 1.0f) {
                    d = EXSqrDistance(line, seg, 0, &t);
                    s = 1.0f;
                } else {
                    d = EXSqrDistance(line, seg, 0, &t);
                    s = 1.0f;
                    seg.Origin() = rect.Origin() + rect.Edge1();
                    seg.Direction() = rect.Edge0();
                    d0 = EXSqrDistance(line, seg, 0, &s0);
                    if (d0 < d) {
                        d = d0;
                        s = s0;
                        t = 1.0f;
                    }
                }
            } else {
                seg.Origin() = rect.Origin() + rect.Edge0();
                seg.Direction() = rect.Edge1();
                d = EXSqrDistance(line, seg, 0, &t);
                s = 1.0f;
                seg.Origin() = rect.Origin();
                seg.Direction() = rect.Edge0();
                d0 = EXSqrDistance(line, seg, 0, &s0);
                if (d0 < d) {
                    d = d0;
                    s = s0;
                    t = 0.0f;
                }
            }
        } else if (t >= 0.0f) {
            seg.Origin() = rect.Origin();
            seg.Direction() = rect.Edge1();
            if (t <= 1.0f) {
                d = EXSqrDistance(line, seg, 0, &t);
                s = 0.0f;
            } else {
                d = EXSqrDistance(line, seg, 0, &t);
                s = 0.0f;
                seg.Origin() = rect.Origin() + rect.Edge1();
                seg.Direction() = rect.Edge0();
                d0 = EXSqrDistance(line, seg, 0, &s0);
                if (d0 < d) {
                    d = d0;
                    s = s0;
                    t = 1.0f;
                }
            }
        } else {
            seg.Origin() = rect.Origin();
            seg.Direction() = rect.Edge1();
            d = EXSqrDistance(line, seg, 0, &t);
            s = 0.0f;
            seg.Origin() = rect.Origin();
            seg.Direction() = rect.Edge0();
            d0 = EXSqrDistance(line, seg, 0, &s0);
            if (d0 < d) {
                d = d0;
                s = s0;
                t = 0.0f;
            }
        }
    }
    if (pr) *pr = r;
    if (ps) *ps = s;
    if (pt) *pt = t;
    return (float)fabs(d);
}

// Triangle against triangle: the closest of each triangle's three edges against the other triangle.
float EXSqrDistance(const EXTriangle &tri0, const EXTriangle &tri1, float *ps0, float *pt0, float *ps1, float *pt1)
{
    float d0, d, t10, s10, t00, s00, t1, s1, t0, s0;
    EXLineSeg seg;
    seg.Origin() = tri0.Origin();
    seg.Direction() = tri0.Edge0();
    d = EXSqrDistance(seg, tri1, &s0, &s1, &t1);
    t0 = 0.0f;
    seg.Direction() = tri0.Edge1();
    d0 = EXSqrDistance(seg, tri1, &t00, &s10, &t10);
    if (d0 < d) {
        d = d0;
        s0 = 0.0f;
        t0 = t00;
        s1 = s10;
        t1 = t10;
    }
    seg.Origin() += tri0.Edge0();
    seg.Direction() = seg.Direction() - tri0.Edge0();
    d0 = EXSqrDistance(seg, tri1, &t00, &s10, &t10);
    s00 = 1.0f - t00;
    if (d0 < d) {
        d = d0;
        s0 = s00;
        t0 = t00;
        s1 = s10;
        t1 = t10;
    }
    seg.Origin() = tri1.Origin();
    seg.Direction() = tri1.Edge0();
    d0 = EXSqrDistance(seg, tri0, &s10, &s00, &t00);
    t10 = 0.0f;
    if (d0 < d) {
        d = d0;
        s0 = s00;
        t0 = t00;
        s1 = s10;
        t1 = t10;
    }
    seg.Direction() = tri1.Edge1();
    d0 = EXSqrDistance(seg, tri0, &t10, &s00, &t00);
    if (d0 < d) {
        d = d0;
        s0 = s00;
        t0 = t00;
        s1 = 0.0f;
        t1 = t10;
    }
    seg.Origin() += tri1.Edge0();
    seg.Direction() = seg.Direction() - tri1.Edge0();
    d0 = EXSqrDistance(seg, tri0, &t10, &s00, &t00);
    if (d0 < d) {
        d = d0;
        s0 = s00;
        t0 = t00;
        s1 = 1.0f - t10;
        t1 = t10;
    }
    if (ps0) *ps0 = s0;
    if (pt0) *pt0 = t0;
    if (ps1) *ps1 = s1;
    if (pt1) *pt1 = t1;
    return (float)fabs(d);
}

#define EXDIST_TAKE(S0, T0, S1, T1)                                                                \
    if (d0 < d) {                                                                                  \
        d = d0;                                                                                    \
        s0 = S0;                                                                                   \
        t0 = T0;                                                                                   \
        s1 = S1;                                                                                   \
        t1 = T1;                                                                                   \
    }

// Triangle against rectangle: each triangle edge against the rectangle, then each rectangle edge
// against the triangle.
float EXSqrDistance(const EXTriangle &tri, const EXRectangle &rect, float *ps0, float *pt0, float *ps1, float *pt1)
{
    float s0, t0, s1, t1, s00, t00, s10, t10, d, d0;
    EXLineSeg seg;
    seg.Origin() = tri.Origin();
    seg.Direction() = tri.Edge0();
    d = EXSqrDistance(seg, rect, &s0, &s1, &t1);
    t0 = 0.0f;
    seg.Direction() = tri.Edge1();
    d0 = EXSqrDistance(seg, rect, &t00, &s10, &t10);
    EXDIST_TAKE(0.0f, t00, s10, t10)
    seg.Origin() += tri.Edge0();
    seg.Direction() = seg.Direction() - tri.Edge0();
    d0 = EXSqrDistance(seg, rect, &t00, &s10, &t10);
    s00 = 1.0f - t00;
    EXDIST_TAKE(s00, t00, s10, t10)
    seg.Origin() = rect.Origin();
    seg.Direction() = rect.Edge0();
    d0 = EXSqrDistance(seg, tri, &s10, &s00, &t00);
    t10 = 0.0f;
    EXDIST_TAKE(s00, t00, s10, t10)
    seg.Direction() = rect.Edge1();
    d0 = EXSqrDistance(seg, tri, &t10, &s00, &t00);
    s10 = 0.0f;
    EXDIST_TAKE(s00, t00, s10, t10)
    seg.Origin() = rect.Origin() + rect.Edge1();
    seg.Direction() = rect.Edge0();
    d0 = EXSqrDistance(seg, tri, &s10, &s00, &t00);
    t10 = 1.0f;
    EXDIST_TAKE(s00, t00, s10, t10)
    seg.Origin() = rect.Origin() + rect.Edge0();
    seg.Direction() = rect.Edge1();
    d0 = EXSqrDistance(seg, tri, &t10, &s00, &t00);
    EXDIST_TAKE(s00, t00, 1.0f, t10)
    if (ps0) *ps0 = s0;
    if (pt0) *pt0 = t0;
    if (ps1) *ps1 = s1;
    if (pt1) *pt1 = t1;
    return (float)fabs(d);
}

// Rectangle against rectangle: each rectangle's four edges against the other rectangle.
float EXSqrDistance(const EXRectangle &rect0, const EXRectangle &rect1, float *ps0, float *pt0, float *ps1, float *pt1)
{
    float s0, t0, s1, t1, s00, t00, s10, t10, d, d0;
    EXLineSeg seg;
    seg.Origin() = rect0.Origin();
    seg.Direction() = rect0.Edge0();
    d = EXSqrDistance(seg, rect1, &s0, &s1, &t1);
    t0 = 0.0f;
    seg.Direction() = rect0.Edge1();
    d0 = EXSqrDistance(seg, rect1, &t00, &s10, &t10);
    EXDIST_TAKE(0.0f, t00, s10, t10)
    seg.Origin() = rect0.Origin() + rect0.Edge0();
    d0 = EXSqrDistance(seg, rect1, &t00, &s10, &t10);
    s00 = 1.0f;
    EXDIST_TAKE(s00, t00, s10, t10)
    seg.Origin() = rect0.Origin() + rect0.Edge1();
    seg.Direction() = rect0.Edge0();
    d0 = EXSqrDistance(seg, rect1, &s00, &s10, &t10);
    t00 = 1.0f;
    EXDIST_TAKE(s00, t00, s10, t10)
    seg.Origin() = rect1.Origin();
    seg.Direction() = rect1.Edge0();
    d0 = EXSqrDistance(seg, rect0, &s10, &s00, &t00);
    t10 = 0.0f;
    EXDIST_TAKE(s00, t00, s10, t10)
    seg.Direction() = rect1.Edge1();
    d0 = EXSqrDistance(seg, rect0, &t10, &s00, &t00);
    EXDIST_TAKE(s00, t00, 0.0f, t10)
    seg.Origin() = rect1.Origin() + rect1.Edge0();
    d0 = EXSqrDistance(seg, rect0, &t10, &s00, &t00);
    s10 = 1.0f;
    EXDIST_TAKE(s00, t00, s10, t10)
    seg.Origin() = rect1.Origin() + rect1.Edge1();
    seg.Direction() = rect1.Edge0();
    d0 = EXSqrDistance(seg, rect0, &s10, &s00, &t00);
    EXDIST_TAKE(s00, t00, s10, 1.0f)
    if (ps0) *ps0 = s0;
    if (pt0) *pt0 = t0;
    if (ps1) *ps1 = s1;
    if (pt1) *pt1 = t1;
    return (float)fabs(d);
}

// Rectangle edges as segments: the bottom (origin, edge 0), left (origin, edge 1), top (origin +
// edge 1, edge 0) and right (origin + edge 0, edge 1) sides.
#define RECT_SEG_B(R) seg.Origin() = R.Origin(); seg.Direction() = R.Edge0();
#define RECT_SEG_L(R) seg.Origin() = R.Origin(); seg.Direction() = R.Edge1();
#define RECT_SEG_T(R) seg.Origin() = R.Origin() + R.Edge1(); seg.Direction() = R.Edge0();
#define RECT_SEG_R(R) seg.Origin() = R.Edge0() + R.Origin(); seg.Direction() = R.Edge1();

// Ray against rectangle: the line solution when the closest line point is ahead of the ray origin,
// otherwise the nearest of the ray origin and the rectangle edges.
float EXSqrDistance(const EXRay &ray, const EXRectangle &rect, float *pr, float *ps, float *pt)
{
    EXMaths::vec_xyzw diff = (const EXMaths::vec_xyzw &)rect.Origin() - ray.Origin();
    float a00 = ray.Direction().LengthXYZSquared();
    float a01 = -ray.Direction().DotXYZ(rect.Edge0());
    float a02 = -ray.Direction().DotXYZ(rect.Edge1());
    float a11 = rect.Edge0().LengthXYZSquared();
    float a22 = rect.Edge1().LengthXYZSquared();
    float cof01 = -a22 * a01;
    float cof02 = -a11 * a02;
    float det = a22 * a11 * a00 + cof01 * a01 + cof02 * a02;
    EXLineSeg seg;
    float d, d0, r, s, t, r0, s0, t0;
    if ((float)fabs(det) < 1e-5f) {
        // Parallel to the rectangle plane.
        RECT_SEG_B(rect)
        d = EXSqrDistance(ray, seg, &r, &s);
        t = 0.0f;
        seg.Direction() = rect.Edge1();
        d0 = EXSqrDistance(ray, seg, &r0, &t0);
        s0 = 0.0f;
        if (d0 < d) {
            d = d0;
            r = r0;
            s = s0;
            t = t0;
        }
        RECT_SEG_T(rect)
        d0 = EXSqrDistance(ray, seg, &r0, &s0);
        t0 = 1.0f;
        if (d0 < d) {
            d = d0;
            r = r0;
            s = s0;
            t = t0;
        }
        seg.Origin() = rect.Origin() + rect.Edge0();
        seg.Direction() = rect.Edge1();
        d0 = EXSqrDistance(ray, seg, &r0, &t0);
        s0 = 1.0f;
        if (d0 < d) {
            d = d0;
            r = r0;
            s = s0;
            t = t0;
        }
        seg.Origin() = ray.Origin();
        d0 = EXSqrDistance(seg.Origin(), rect, &s0, &t0);
        if (d0 < d) {
            d = d0;
            r = 0.0f;
            s = s0;
            t = t0;
        }
        goto done;
    }
    {
        float cof12 = a02 * a01;
        float inv = 1.0f / det;
        float rhs0 = -(-diff.DotXYZ(ray.Direction())) * inv;
        float rhs1 = -diff.DotXYZ(rect.Edge0()) * inv;
        float rhs2 = -diff.DotXYZ(rect.Edge1()) * inv;
        r = a11 * a22 * rhs0 + cof01 * rhs1 + cof02 * rhs2;
        s = cof01 * rhs0 + (a00 * a22 - a02 * a02) * rhs1 + cof12 * rhs2;
        t = cof02 * rhs0 + cof12 * rhs1 + (a00 * a11 - a01 * a01) * rhs2;
    }
    if (r > 0.0f) {
        // Closest line point is on the ray: as for the line.
        if (s >= 0.0f) {
            if (s <= 1.0f) {
                if (t >= 0.0f) {
                    if (t <= 1.0f) {
                        d = 0.0f;
                    } else {
                        RECT_SEG_T(rect)
                        d = EXSqrDistance(ray, seg, 0, &s);
                        t = 1.0f;
                    }
                } else {
                    RECT_SEG_B(rect)
                    d = EXSqrDistance(ray, seg, 0, &s);
                    t = 0.0f;
                }
            } else if (t >= 0.0f) {
                RECT_SEG_R(rect)
                if (t <= 1.0f) {
                    d = EXSqrDistance(ray, seg, 0, &t);
                    s = 1.0f;
                } else {
                    d = EXSqrDistance(ray, seg, 0, &t);
                    s = 1.0f;
                    RECT_SEG_T(rect)
                    d0 = EXSqrDistance(ray, seg, 0, &s0);
                    if (d0 < d) {
                        d = d0;
                        s = s0;
                        t = 1.0f;
                    }
                }
            } else {
                RECT_SEG_R(rect)
                d = EXSqrDistance(ray, seg, 0, &t);
                s = 1.0f;
                RECT_SEG_B(rect)
                d0 = EXSqrDistance(ray, seg, 0, &s0);
                if (d0 < d) {
                    d = d0;
                    s = s0;
                    t = 0.0f;
                }
            }
        } else if (t >= 0.0f) {
            RECT_SEG_L(rect)
            if (t <= 1.0f) {
                d = EXSqrDistance(ray, seg, 0, &t);
                s = 0.0f;
            } else {
                d = EXSqrDistance(ray, seg, 0, &t);
                s = 0.0f;
                RECT_SEG_T(rect)
                d0 = EXSqrDistance(ray, seg, 0, &s0);
                if (d0 < d) {
                    d = d0;
                    s = s0;
                    t = 1.0f;
                }
            }
        } else {
            RECT_SEG_L(rect)
            d = EXSqrDistance(ray, seg, 0, &t);
            s = 0.0f;
            RECT_SEG_B(rect)
            d0 = EXSqrDistance(ray, seg, 0, &s0);
            if (d0 < d) {
                d = d0;
                s = s0;
                t = 0.0f;
            }
        }
        goto done;
    }
    // Closest line point is behind the ray origin.
    if (s < 0.0f) {
        if (t >= 0.0f) {
            RECT_SEG_L(rect)
            if (t <= 1.0f) {
                d = EXSqrDistance(ray, seg, &r, &t);
                s = 0.0f;
                seg.Origin() = ray.Origin();
                d0 = EXSqrDistance(seg.Origin(), rect, &s0, &t0);
                if (d0 < d) {
                    d = d0;
                    r = 0.0f;
                    s = s0;
                    t = t0;
                }
            } else {
                d = EXSqrDistance(ray, seg, &r, &t);
                s = 0.0f;
                RECT_SEG_T(rect)
                d0 = EXSqrDistance(ray, seg, &r0, &s0);
                t0 = 1.0f;
                if (d0 < d) {
                    d = d0;
                    r = r0;
                    s = s0;
                    t = t0;
                }
                seg.Origin() = ray.Origin();
                d0 = EXSqrDistance(seg.Origin(), rect, &s0, &t0);
                if (d0 < d) {
                    d = d0;
                    r = 0.0f;
                    s = s0;
                    t = t0;
                }
            }
        } else {
            RECT_SEG_L(rect)
            d = EXSqrDistance(ray, seg, &r, &t);
            s = 0.0f;
            RECT_SEG_B(rect)
            d0 = EXSqrDistance(ray, seg, &r0, &s0);
            t0 = 0.0f;
            if (d0 < d) {
                d = d0;
                r = r0;
                s = s0;
                t = t0;
            }
            seg.Origin() = ray.Origin();
            d0 = EXSqrDistance(seg.Origin(), rect, &s0, &t0);
            if (d0 < d) {
                d = d0;
                r = 0.0f;
                s = s0;
                t = t0;
            }
        }
    } else if (s > 1.0f) {
        if (t < 0.0f) {
            RECT_SEG_R(rect)
            d = EXSqrDistance(ray, seg, &r, &t);
            s = 1.0f;
            RECT_SEG_B(rect)
            d0 = EXSqrDistance(ray, seg, &r0, &s0);
            t0 = 0.0f;
            if (d0 < d) {
                d = d0;
                r = r0;
                s = s0;
                t = t0;
            }
            seg.Origin() = ray.Origin();
            d0 = EXSqrDistance(seg.Origin(), rect, &s0, &t0);
            if (d0 < d) {
                d = d0;
                r = 0.0f;
                s = s0;
                t = t0;
            }
        } else {
            seg.Origin() = rect.Edge0() + rect.Origin();
            if (t > 1.0f) {
                seg.Direction() = rect.Edge1();
                d = EXSqrDistance(ray, seg, &r, &t);
                s = 1.0f;
                RECT_SEG_T(rect)
                d0 = EXSqrDistance(ray, seg, &r0, &s0);
                t0 = 1.0f;
                if (d0 < d) {
                    d = d0;
                    r = r0;
                    s = s0;
                    t = t0;
                }
                seg.Origin() = ray.Origin();
                d0 = EXSqrDistance(seg.Origin(), rect, &s0, &t0);
                if (d0 < d) {
                    d = d0;
                    r = 0.0f;
                    s = s0;
                    t = t0;
                }
            } else {
                seg.Direction() = rect.Edge1();
                d = EXSqrDistance(ray, seg, &r, &t);
                s = 1.0f;
                seg.Origin() = ray.Origin();
                d0 = EXSqrDistance(seg.Origin(), rect, &s0, &t0);
                if (d0 < d) {
                    d = d0;
                    r = 0.0f;
                    s = s0;
                    t = t0;
                }
            }
        }
    } else if (t >= 0.0f) {
        if (t <= 1.0f) {
            seg.Origin() = ray.Origin();
            d = EXSqrDistance(seg.Origin(), rect, &s, &t);
            r = 0.0f;
        } else {
            RECT_SEG_T(rect)
            d = EXSqrDistance(ray, seg, &r, &s);
            t = 1.0f;
            seg.Origin() = ray.Origin();
            d0 = EXSqrDistance(seg.Origin(), rect, &s0, &t0);
            if (d0 < d) {
                d = d0;
                r = 0.0f;
                s = s0;
                t = t0;
            }
        }
    } else {
        RECT_SEG_B(rect)
        d = EXSqrDistance(ray, seg, &r, &s);
        t = 0.0f;
        seg.Origin() = ray.Origin();
        d0 = EXSqrDistance(seg.Origin(), rect, &s0, &t0);
        if (d0 < d) {
            d = d0;
            r = 0.0f;
            s = s0;
            t = t0;
        }
    }
done:
    if (pr) *pr = r;
    if (ps) *ps = s;
    if (pt) *pt = t;
    return (float)fabs(d);
}

// Triangle edges as segments: (origin, edge 0), (origin, edge 1) and the far edge
// (origin + edge 0, edge 1 - edge 0).
#define TRI_SEG_E0(T) edge.Origin() = T.Origin(); edge.Direction() = T.Edge0();
#define TRI_SEG_E1(T) edge.Origin() = T.Origin(); edge.Direction() = T.Edge1();
#define TRI_SEG_E2(T) edge.Origin() = T.Origin() + T.Edge0(); edge.Direction() = T.Edge1() - T.Edge0();

// Segment against triangle: the line/plane solution clamped to the segment and triangle; outside
// either, the nearest of the triangle edges and the segment end points.
float EXSqrDistance(const EXLineSeg &seg, const EXTriangle &tri, float *pr, float *ps, float *pt)
{
    EXVector diff = tri.Origin() - seg.Origin();
    float a00 = seg.Direction().LengthXYZSquared();
    float a01 = (-seg.Direction()).DotXYZ(tri.Edge0());
    float a02 = (-seg.Direction()).DotXYZ(tri.Edge1());
    float a11 = tri.Edge0().LengthXYZSquared();
    float a12 = tri.Edge0().DotXYZ(tri.Edge1());
    float a22 = tri.Edge1().LengthXYZSquared();
    float b0 = -seg.Direction().DotXYZ(diff);
    float b1 = tri.Edge0().DotXYZ(diff);
    float b2 = tri.Edge1().DotXYZ(diff);
    float cof00 = a11 * a22 - a12 * a12;
    float cof01 = a02 * a12 - a01 * a22;
    float cof02 = a01 * a12 - a02 * a11;
    float det = a00 * cof00 + a01 * cof01 + a02 * cof02;
    EXLineSeg edge;
    EXVector p;
    float d, d0, r, s, t, r0, s0, t0;
    EXVector n = tri.Edge0().Cross(tri.Edge1());
    float nd = n.DotXYZ(seg.Direction());
    if (nd * nd < n.LengthXYZSquared() * a00 * 1e-5f) {
        // Parallel to the triangle plane.
        TRI_SEG_E0(tri)
        d = EXSqrDistance(seg, edge, &r, &s);
        t = 0.0f;
        edge.Direction() = tri.Edge1();
        d0 = EXSqrDistance(seg, edge, &r0, &t0);
        if (d0 < d) {
            d = d0;
            r = r0;
            s = 0.0f;
            t = t0;
        }
        TRI_SEG_E2(tri)
        d0 = EXSqrDistance(seg, edge, &r0, &t0);
        s0 = 1.0f - t0;
        if (d0 < d) {
            d = d0;
            r = r0;
            s = s0;
            t = t0;
        }
        p = seg.Origin();
        d0 = EXSqrDistance(p, tri, &s0, &t0);
        if (d0 < d) {
            d = d0;
            r = 0.0f;
            s = s0;
            t = t0;
        }
        p = seg.Origin() + seg.Direction();
        d0 = EXSqrDistance(p, tri, &s0, &t0);
        if (d0 < d) {
            d = d0;
            r = 1.0f;
            s = s0;
            t = t0;
        }
        goto done;
    }
    {
        float cof11 = a00 * a22 - a02 * a02;
        float cof12 = a02 * a01 - a00 * a12;
        float cof22 = a00 * a11 - a01 * a01;
        float inv = det != 0.0f ? 1.0f / det : 0.0f;
        float rhs0 = -inv * b0;
        float rhs1 = -inv * b1;
        float rhs2 = -inv * b2;
        r = cof00 * rhs0 + cof01 * rhs1 + cof02 * rhs2;
        s = cof01 * rhs0 + cof11 * rhs1 + cof12 * rhs2;
        t = cof02 * rhs0 + cof12 * rhs1 + cof22 * rhs2;
    }
    if (r >= 0.0f) {
        if (r <= 1.0f) {
            if (s + t > 1.0f) {
                if (s >= 0.0f) {
                    if (t >= 0.0f) {
                        TRI_SEG_E2(tri)
                        d = EXSqrDistance(seg, edge, &r, &t);
                        s = 1.0f - t;
                        goto done;
                    }
                    TRI_SEG_E0(tri)
                    d = EXSqrDistance(seg, edge, &r, &s);
                    t = 0.0f;
                } else {
                    TRI_SEG_E1(tri)
                    d = EXSqrDistance(seg, edge, &r, &t);
                    s = 0.0f;
                }
                TRI_SEG_E2(tri)
                d0 = EXSqrDistance(seg, edge, &r0, &t0);
                if (d0 < d) {
                    d = d0;
                    r = r0;
                    s = 1.0f - t0;
                    t = t0;
                }
            } else if (s >= 0.0f) {
                if (t >= 0.0f) {
                    d = (t * a22 + s * a12 + r * a02 + b2 * 2.0f) * t +
                        (t * a12 + s * a11 + r * a01 + b1 * 2.0f) * s +
                        (t * a02 + s * a01 + r * a00 + b0 * 2.0f) * r + diff.LengthXYZSquared();
                } else {
                    TRI_SEG_E0(tri)
                    d = EXSqrDistance(seg, edge, &r, &s);
                    t = 0.0f;
                }
            } else {
                TRI_SEG_E1(tri)
                if (t >= 0.0f) {
                    d = EXSqrDistance(seg, edge, &r, &t);
                    s = 0.0f;
                } else {
                    d = EXSqrDistance(seg, edge, &r, &t);
                    s = 0.0f;
                    TRI_SEG_E0(tri)
                    d0 = EXSqrDistance(seg, edge, &r0, &s0);
                    if (d0 < d) {
                        d = d0;
                        r = r0;
                        s = s0;
                        t = 0.0f;
                    }
                }
            }
            goto done;
        }
        // Beyond the segment end.
        if (s + t > 1.0f) {
            if (s < 0.0f) {
                TRI_SEG_E1(tri)
                d = EXSqrDistance(seg, edge, &r, &t);
                s = 0.0f;
                TRI_SEG_E2(tri)
                d0 = EXSqrDistance(seg, edge, &r0, &t0);
                s0 = 1.0f - t0;
                if (d0 < d) {
                    d = d0;
                    r = r0;
                    s = s0;
                    t = t0;
                }
            } else if (t < 0.0f) {
                TRI_SEG_E0(tri)
                d = EXSqrDistance(seg, edge, &r, &s);
                t = 0.0f;
                TRI_SEG_E2(tri)
                d0 = EXSqrDistance(seg, edge, &r0, &t0);
                s0 = 1.0f - t0;
                if (d0 < d) {
                    d = d0;
                    r = r0;
                    s = s0;
                    t = t0;
                }
            } else {
                TRI_SEG_E2(tri)
                d = EXSqrDistance(seg, edge, &r, &t);
                s = 1.0f - t;
            }
        } else if (s >= 0.0f) {
            if (t >= 0.0f) {
                p = seg.Origin() + seg.Direction();
                d = EXSqrDistance(p, tri, &s, &t);
                r = 1.0f;
                goto done;
            }
            TRI_SEG_E0(tri)
            d = EXSqrDistance(seg, edge, &r, &s);
            t = 0.0f;
        } else {
            TRI_SEG_E1(tri)
            if (t < 0.0f) {
                d = EXSqrDistance(seg, edge, &r, &t);
                s = 0.0f;
                TRI_SEG_E0(tri)
                d0 = EXSqrDistance(seg, edge, &r0, &s0);
                t0 = 0.0f;
                if (d0 < d) {
                    d = d0;
                    r = r0;
                    s = s0;
                    t = t0;
                }
            } else {
                d = EXSqrDistance(seg, edge, &r, &t);
                s = 0.0f;
            }
        }
        p = seg.Origin() + seg.Direction();
        d0 = EXSqrDistance(p, tri, &s0, &t0);
        if (d0 < d) {
            d = d0;
            r = 1.0f;
            s = s0;
            t = t0;
        }
    } else {
        // Before the segment start.
        if (s + t > 1.0f) {
            if (s < 0.0f) {
                TRI_SEG_E1(tri)
                d = EXSqrDistance(seg, edge, &r, &t);
                s = 0.0f;
                TRI_SEG_E2(tri)
                d0 = EXSqrDistance(seg, edge, &r0, &t0);
                s0 = 1.0f - t0;
                if (d0 < d) {
                    d = d0;
                    r = r0;
                    s = s0;
                    t = t0;
                }
            } else if (t < 0.0f) {
                TRI_SEG_E0(tri)
                d = EXSqrDistance(seg, edge, &r, &s);
                t = 0.0f;
                TRI_SEG_E2(tri)
                d0 = EXSqrDistance(seg, edge, &r0, &t0);
                s0 = 1.0f - t0;
                if (d0 < d) {
                    d = d0;
                    r = r0;
                    s = s0;
                    t = t0;
                }
            } else {
                TRI_SEG_E2(tri)
                d = EXSqrDistance(seg, edge, &r, &t);
                s = 1.0f - t;
            }
        } else if (s >= 0.0f) {
            if (t >= 0.0f) {
                p = seg.Origin();
                d = EXSqrDistance(p, tri, &s, &t);
                r = 0.0f;
                goto done;
            }
            TRI_SEG_E0(tri)
            d = EXSqrDistance(seg, edge, &r, &s);
            t = 0.0f;
        } else {
            TRI_SEG_E1(tri)
            if (t < 0.0f) {
                d = EXSqrDistance(seg, edge, &r, &t);
                s = 0.0f;
                TRI_SEG_E0(tri)
                d0 = EXSqrDistance(seg, edge, &r0, &s0);
                t0 = 0.0f;
                if (d0 < d) {
                    d = d0;
                    r = r0;
                    s = s0;
                    t = t0;
                }
            } else {
                d = EXSqrDistance(seg, edge, &r, &t);
                s = 0.0f;
            }
        }
        p = seg.Origin();
        d0 = EXSqrDistance(p, tri, &s0, &t0);
        if (d0 < d) {
            d = d0;
            r = 0.0f;
            s = s0;
            t = t0;
        }
    }
done:
    if (pr) *pr = r;
    if (ps) *ps = s;
    if (pt) *pt = t;
    return (float)fabs(d);
}

// Segment against rectangle: the line solution clamped to the segment and the rectangle; outside
// either, the nearest of the rectangle edges and the segment end points.
float EXSqrDistance(const EXLineSeg &sg, const EXRectangle &rect, float *pr, float *ps, float *pt)
{
    EXVector diff = rect.Origin() - sg.Origin();
    float a00 = sg.Direction().LengthXYZSquared();
    float a01 = -sg.Direction().DotXYZ(rect.Edge0());
    float a02 = -sg.Direction().DotXYZ(rect.Edge1());
    float a11 = rect.Edge0().LengthXYZSquared();
    float a22 = rect.Edge1().LengthXYZSquared();
    float b0 = -sg.Direction().DotXYZ(diff);
    float b1 = rect.Edge0().DotXYZ(diff);
    float b2 = rect.Edge1().DotXYZ(diff);
    float cof00 = a22 * a11;
    float cof01 = -a22 * a01;
    float cof02 = -a11 * a02;
    float det = cof02 * a02 + cof01 * a01 + cof00 * a00;
    EXLineSeg seg;
    EXVector p;
    float d, d0, r, s, t, r0, s0, t0;
    if ((float)fabs(det) < 1e-5f) {
        // Parallel to the rectangle plane.
        RECT_SEG_B(rect)
        d = EXSqrDistance(sg, seg, &r, &s);
        t = 0.0f;
        seg.Direction() = rect.Edge1();
        d0 = EXSqrDistance(sg, seg, &r0, &t0);
        s0 = 0.0f;
        if (d0 < d) {
            d = d0;
            r = r0;
            s = s0;
            t = t0;
        }
        RECT_SEG_T(rect)
        d0 = EXSqrDistance(sg, seg, &r0, &s0);
        t0 = 1.0f;
        if (d0 < d) {
            d = d0;
            r = r0;
            s = s0;
            t = t0;
        }
        seg.Origin() = rect.Origin() + rect.Edge0();
        seg.Direction() = rect.Edge1();
        d0 = EXSqrDistance(sg, seg, &r0, &t0);
        s0 = 1.0f;
        if (d0 < d) {
            d = d0;
            r = r0;
            s = s0;
            t = t0;
        }
        p = sg.Origin();
        d0 = EXSqrDistance(p, rect, &s0, &t0);
        if (d0 < d) {
            d = d0;
            r = 0.0f;
            s = s0;
            t = t0;
        }
        p = sg.Origin() + sg.Direction();
        d0 = EXSqrDistance(p, rect, &s0, &t0);
        if (d0 < d) {
            d = d0;
            r = 1.0f;
            s = s0;
            t = t0;
        }
        goto done;
    }
    {
        float cof12 = a02 * a01;
        float inv = 1.0f / det;
        float rhs0 = -inv * b0;
        float rhs1 = -inv * b1;
        float rhs2 = -inv * b2;
        r = cof00 * rhs0 + cof01 * rhs1 + cof02 * rhs2;
        s = cof01 * rhs0 + (a00 * a22 - a02 * a02) * rhs1 + cof12 * rhs2;
        t = cof02 * rhs0 + cof12 * rhs1 + (a00 * a11 - a01 * a01) * rhs2;
    }
    if (r >= 0.0f) {
        if (r <= 1.0f) {
            if (s >= 0.0f) {
                if (s <= 1.0f) {
                    if (t >= 0.0f) {
                        if (t <= 1.0f) {
                            d = (t * a02 + s * a01 + r * a00 + b0 * 2.0f) * r +
                                (t * a22 + r * a02 + b2 * 2.0f) * t +
                                (s * a11 + r * a01 + b1 * 2.0f) * s + diff.LengthXYZSquared();
                        } else {
                            RECT_SEG_T(rect)
                            d = EXSqrDistance(sg, seg, &r, &s);
                            t = 1.0f;
                        }
                    } else {
                        RECT_SEG_B(rect)
                        d = EXSqrDistance(sg, seg, &r, &s);
                        t = 0.0f;
                    }
                } else if (t >= 0.0f) {
                    RECT_SEG_R(rect)
                    if (t <= 1.0f) {
                        d = EXSqrDistance(sg, seg, &r, &t);
                        s = 1.0f;
                    } else {
                        d = EXSqrDistance(sg, seg, &r, &t);
                        s = 1.0f;
                        RECT_SEG_T(rect)
                        d0 = EXSqrDistance(sg, seg, &r0, &s0);
                        if (d0 < d) {
                            d = d0;
                            r = r0;
                            s = s0;
                            t = 1.0f;
                        }
                    }
                } else {
                    RECT_SEG_R(rect)
                    d = EXSqrDistance(sg, seg, &r, &t);
                    s = 1.0f;
                    RECT_SEG_B(rect)
                    d0 = EXSqrDistance(sg, seg, &r0, &s0);
                    if (d0 < d) {
                        d = d0;
                        r = r0;
                        s = s0;
                        t = 0.0f;
                    }
                }
            } else if (t >= 0.0f) {
                RECT_SEG_L(rect)
                if (t <= 1.0f) {
                    d = EXSqrDistance(sg, seg, &r, &t);
                    s = 0.0f;
                } else {
                    d = EXSqrDistance(sg, seg, &r, &t);
                    s = 0.0f;
                    RECT_SEG_T(rect)
                    d0 = EXSqrDistance(sg, seg, &r0, &s0);
                    if (d0 < d) {
                        d = d0;
                        r = r0;
                        s = s0;
                        t = 1.0f;
                    }
                }
            } else {
                RECT_SEG_L(rect)
                d = EXSqrDistance(sg, seg, &r, &t);
                s = 0.0f;
                RECT_SEG_B(rect)
                d0 = EXSqrDistance(sg, seg, &r0, &s0);
                if (d0 < d) {
                    d = d0;
                    r = r0;
                    s = s0;
                    t = 0.0f;
                }
            }
            goto done;
        }
        // Beyond the segment end.
        if (s >= 0.0f) {
            if (s > 1.0f) {
                seg.Origin() = rect.Edge0() + rect.Origin();
                seg.Direction() = rect.Edge1();
                if (t < 0.0f) {
                    d = EXSqrDistance(sg, seg, &r, &t);
                    s = 1.0f;
                    RECT_SEG_B(rect)
                    d0 = EXSqrDistance(sg, seg, &r0, &s0);
                    t0 = 0.0f;
                    if (d0 < d) {
                        d = d0;
                        r = r0;
                        s = s0;
                        t = t0;
                    }
                } else if (t > 1.0f) {
                    d = EXSqrDistance(sg, seg, &r, &t);
                    s = 1.0f;
                    RECT_SEG_T(rect)
                    d0 = EXSqrDistance(sg, seg, &r0, &s0);
                    t0 = 1.0f;
                    if (d0 < d) {
                        d = d0;
                        r = r0;
                        s = s0;
                        t = t0;
                    }
                } else {
                    d = EXSqrDistance(sg, seg, &r, &t);
                    s = 1.0f;
                }
            } else if (t < 0.0f) {
                RECT_SEG_B(rect)
                d = EXSqrDistance(sg, seg, &r, &s);
                t = 0.0f;
            } else if (t <= 1.0f) {
                p = sg.Origin() + sg.Direction();
                d = EXSqrDistance(p, rect, &s, &t);
                r = 1.0f;
                goto done;
            } else {
                RECT_SEG_T(rect)
                d = EXSqrDistance(sg, seg, &r, &s);
                t = 1.0f;
            }
        } else {
            RECT_SEG_L(rect)
            if (t < 0.0f) {
                d = EXSqrDistance(sg, seg, &r, &t);
                s = 0.0f;
                RECT_SEG_B(rect)
                d0 = EXSqrDistance(sg, seg, &r0, &s0);
                t0 = 0.0f;
                if (d0 < d) {
                    d = d0;
                    r = r0;
                    s = s0;
                    t = t0;
                }
            } else if (t > 1.0f) {
                d = EXSqrDistance(sg, seg, &r, &t);
                s = 0.0f;
                RECT_SEG_T(rect)
                d0 = EXSqrDistance(sg, seg, &r0, &s0);
                t0 = 1.0f;
                if (d0 < d) {
                    d = d0;
                    r = r0;
                    s = s0;
                    t = t0;
                }
            } else {
                d = EXSqrDistance(sg, seg, &r, &t);
                s = 0.0f;
            }
        }
        p = sg.Origin() + sg.Direction();
        d0 = EXSqrDistance(p, rect, &s0, &t0);
        if (d0 < d) {
            d = d0;
            r = 1.0f;
            s = s0;
            t = t0;
        }
    } else {
        // Before the segment start.
        if (s >= 0.0f) {
            if (s > 1.0f) {
                seg.Origin() = rect.Edge0() + rect.Origin();
                seg.Direction() = rect.Edge1();
                if (t < 0.0f) {
                    d = EXSqrDistance(sg, seg, &r, &t);
                    s = 1.0f;
                    RECT_SEG_B(rect)
                    d0 = EXSqrDistance(sg, seg, &r0, &s0);
                    t0 = 0.0f;
                    if (d0 < d) {
                        d = d0;
                        r = r0;
                        s = s0;
                        t = t0;
                    }
                } else if (t > 1.0f) {
                    d = EXSqrDistance(sg, seg, &r, &t);
                    s = 1.0f;
                    RECT_SEG_T(rect)
                    d0 = EXSqrDistance(sg, seg, &r0, &s0);
                    t0 = 1.0f;
                    if (d0 < d) {
                        d = d0;
                        r = r0;
                        s = s0;
                        t = t0;
                    }
                } else {
                    d = EXSqrDistance(sg, seg, &r, &t);
                    s = 1.0f;
                }
            } else if (t < 0.0f) {
                RECT_SEG_B(rect)
                d = EXSqrDistance(sg, seg, &r, &s);
                t = 0.0f;
            } else if (t <= 1.0f) {
                p = sg.Origin();
                d = EXSqrDistance(p, rect, &s, &t);
                r = 0.0f;
                goto done;
            } else {
                RECT_SEG_T(rect)
                d = EXSqrDistance(sg, seg, &r, &s);
                t = 1.0f;
            }
        } else {
            RECT_SEG_L(rect)
            if (t < 0.0f) {
                d = EXSqrDistance(sg, seg, &r, &t);
                s = 0.0f;
                RECT_SEG_B(rect)
                d0 = EXSqrDistance(sg, seg, &r0, &s0);
                t0 = 0.0f;
                if (d0 < d) {
                    d = d0;
                    r = r0;
                    s = s0;
                    t = t0;
                }
            } else if (t > 1.0f) {
                d = EXSqrDistance(sg, seg, &r, &t);
                s = 0.0f;
                RECT_SEG_T(rect)
                d0 = EXSqrDistance(sg, seg, &r0, &s0);
                t0 = 1.0f;
                if (d0 < d) {
                    d = d0;
                    r = r0;
                    s = s0;
                    t = t0;
                }
            } else {
                d = EXSqrDistance(sg, seg, &r, &t);
                s = 0.0f;
            }
        }
        p = sg.Origin();
        d0 = EXSqrDistance(p, rect, &s0, &t0);
        if (d0 < d) {
            d = d0;
            r = 0.0f;
            s = s0;
            t = t0;
        }
    }
done:
    if (pr) *pr = r;
    if (ps) *ps = s;
    if (pt) *pt = t;
    return (float)fabs(d);
}
