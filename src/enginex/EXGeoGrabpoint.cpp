#include "EXGrabPointTemp.h"
#include "EXScratchPad.h"
#include "EXLine.h"
#include "EXTriangle.h"
#include "EXBounds.h"
#include "EXMatrix.h"
#include "EXStdFuncs.h"
#include "EXItemAnimator.h"
#include "EXItem.h"

void EXGrabPointTemp::Set(const EXVector &pos, float radius, GRABFITYPE type)
{
    m_pos = pos;
    m_radius = radius;
    m_type = type;
    m_24 = 0;
    m_2c = 0;
    m_layer.Set(0, 0x400);
    m_pAnimator = 0;
}

EXItem *EXGrabPointTemp::GetItem() const
{
    if (m_pAnimator) {
        EXPlacementItem *p = (EXPlacementItem *)m_pAnimator->GetBaseOwner();
        if (p && p->IsKindOf(EXRUNTIME_CLASS(EXItem))) return (EXItem *)p;
    }
    return 0;
}

void EXGrabPointArrayTemp::Set(const EXVector &pos, float radius, GRABFITYPE type)
{
    m_pos = pos;
    m_radius = radius;
    m_type = type;
    m_layer.Set(0, 0x400);
}

// NEAR (see check): retail never advances through the vertices here: every line strip adds its header
// vertex m_Count + 1 times and every polygon its first vertex m_Count - 2 times (kept as found).
void EXGeoGrabPoint::GetBoundsBox(EXBoundsBox &b) const
{
    b.m_vctMin.x = 1000000.0f;
    b.m_vctMin.y = 1000000.0f;
    b.m_vctMin.z = 1000000.0f;
    b.m_vctMax.x = -1000000.0f;
    b.m_vctMax.y = -1000000.0f;
    b.m_vctMax.z = -1000000.0f;
    b.m_vctMin.w = 0.0f;
    b.m_vctMax.w = 0.0f;
    const EXGeoGrabPoint_LineVert *v = m_Lines;
    for (int i = 0; i < m_nLines; i++) {
        for (int j = 0; j < (int)(v->m_Count + 1); j++) {
            EXMaths::vec_xyzw &lo = b.m_vctMin;
            float w = lo.w;
            if (w >= 0.0f) w = 0.0f;
            float z = v->z;
            if (z > lo.z) z = lo.z;
            float y = v->y;
            if (y > lo.y) y = lo.y;
            float x = v->x;
            if (x > lo.x) x = lo.x;
            lo.x = x;
            lo.y = y;
            lo.z = z;
            lo.w = w;
            EXMaths::vec_xyzw &hi = b.m_vctMax;
            w = hi.w;
            if (w <= 0.0f) w = 0.0f;
            z = v->z;
            if (z < hi.z) z = hi.z;
            y = v->y;
            if (y < hi.y) y = hi.y;
            x = v->x;
            if (x < hi.x) x = hi.x;
            hi.x = x;
            hi.y = y;
            hi.z = z;
            hi.w = w;
        }
    }
    for (int i = 0; i < m_nTris; i++) {
        const EXGeoGrabPoint_Tri *t = m_Tris;
        const EXGeoGrabPoint_TriVert *p = t->m_v;
        for (int j = 0; j < t->m_Count - 2; j++, p++) {
            EXMaths::vec_xyzw &lo = b.m_vctMin;
            float w = lo.w;
            if (w >= 0.0f) w = 0.0f;
            float z = p->z;
            if (z > lo.z) z = lo.z;
            float y = p->y;
            if (y > lo.y) y = lo.y;
            float x = p->x;
            if (x > lo.x) x = lo.x;
            lo.x = x;
            lo.y = y;
            lo.z = z;
            lo.w = w;
            EXMaths::vec_xyzw &hi = b.m_vctMax;
            w = hi.w;
            if (w <= 0.0f) w = 0.0f;
            z = p->z;
            if (z < hi.z) z = hi.z;
            y = p->y;
            if (y < hi.y) y = hi.y;
            x = p->x;
            if (x < hi.x) x = hi.x;
            hi.x = x;
            hi.y = y;
            hi.z = z;
            hi.w = w;
        }
    }
}

static __forceinline EXVector GrabXForm(float x, float y, float z, const EXMatrix &m)
{
    EXVector in(x, y, z, 1.0f), out;
    D3DXVec4Transform(&out, &in, &m);
    return out;
}

// NEAR (see check): the nearest grab point of one entity within t.m_radius of t.m_pos. Ledge lines
// (type bit 1) are tested segment by segment; grab polygons (type bit 2) triangle by triangle as fans.
// Fills the hit, the edge or triangle, the matrix and the result type (see m_24 above), and on a hit
// shrinks t.m_radius to the hit distance so later entities must be nearer.
s8 EXGeoGrabPoint::DoGrabPoint(const EXMatrix &m, EXGrabPointTemp &t) const
{
    EXVector p = t.m_pos;
    const EXGeoGrabPoint_LineVert *pBest = 0;
    float best = t.m_radius * t.m_radius;
    s8 found = 0;
    EXVector hit(0.0f, 0.0f, 0.0f, 0.0f);
    if ((t.m_type & 1) && m_bLines) {
        EXScratchPad sp;
        const EXGeoGrabPoint_LineVert *v = m_Lines;
        int pos = 0;
        for (int i = 0; i < m_nLines; i++) {
            EXVector a = GrabXForm(v->x, v->y, v->z, m);
            a.w = 0.0f;
            const EXGeoGrabPoint_LineVert *e = v + 1;
            for (int n = v->m_Count; n; n--, e++) {
                EXVector b = GrabXForm(e->x, e->y, e->z, m);
                b.w = 0.0f;
                EXLineSeg seg(a, b - a);
                int r = seg.NearestTo(p, hit);
                EXVector d = hit - p;
                float d2 = d.x * d.x + d.y * d.y + d.z * d.z;
                if (d2 < best) {
                    t.m_30 = hit;
                    t.m_EdgePos = a;
                    t.m_EdgeEnd = b;
                    t.m_EdgeUp = EXVector(a.x, a.y + 1.0f, a.z, a.w);
                    pBest = e;
                    pos = r;
                    best = d2;
                }
                a = b;
            }
            v = e;
        }
        if (pBest) {
            t.m_Normal = t.m_EdgeEnd - t.m_EdgePos;
            float l = t.m_Normal.x * t.m_Normal.x + t.m_Normal.y * t.m_Normal.y + t.m_Normal.z * t.m_Normal.z + t.m_Normal.w * t.m_Normal.w;
            if (l >= 0.0f && (l = (float)sqrt(l)) > 1e-5f) {
                l = 1.0f / l;
                t.m_Normal.x = l * t.m_Normal.x;
                t.m_Normal.y *= l;
                t.m_Normal.z *= l;
                t.m_Normal.w *= l;
            }
            struct Mtx { float f[16]; };
            *(Mtx *)t.m_Matrix = *(const Mtx *)&m;
            t.m_28 = pBest->m_Data;
            t.m_24 = 1;
            if (pos == EXLineSeg::LINE_START) {
                if (pBest->m_Flags & 1) {
                    t.m_2c = 0;
                    t.m_24 = 3;
                }
            } else if (pos == EXLineSeg::LINE_END && (pBest->m_Flags & 2)) {
                t.m_2c = 1;
                t.m_24 = 3;
            }
            found = 1;
        }
    }
    if (t.m_type & 2) {
        const EXGeoGrabPoint_Tri *r = m_Tris;
        const EXGeoGrabPoint_Tri *pBestTri = 0;
        int bestK = 0;
        for (int i = 0; i < m_nTris; i++) {
            EXVector v0 = GrabXForm(r->m_v[0].x, r->m_v[0].y, r->m_v[0].z, m);
            EXVector v1 = GrabXForm(r->m_v[1].x, r->m_v[1].y, r->m_v[1].z, m);
            v0.w = 0.0f;
            v1.w = 0.0f;
            int n = r->m_Count - 2;
            int k = 0;
            for (; k < n; k++) {
                EXVector v2 = GrabXForm(r->m_v[k + 2].x, r->m_v[k + 2].y, r->m_v[k + 2].z, m);
                v2.w = 0.0f;
                EXVector e0 = v1 - v0;
                EXVector e1 = v2 - v0;
                EXTriangle tri;
                tri.m_v[0] = v0;
                tri.m_v[1] = e0;
                tri.m_v[2] = e1;
                float s, u;
                float d = tri.MinPointSquared(p, s, u);
                if (d < best) {
                    EXVector h = v0 + e0 * s + e1 * u;
                    t.m_30 = h;
                    t.m_30.w = 0.0f;
                    t.m_EdgePos = v0;
                    t.m_EdgeEnd = v1;
                    t.m_EdgeUp = v2;
                    pBestTri = r;
                    bestK = k;
                    best = d;
                }
                v1 = v2;
            }
            r = (const EXGeoGrabPoint_Tri *)((const u8 *)r + (k + 3) * 0x10);
        }
        if (pBestTri) {
            EXVector nrm(pBestTri->m_Normal[0], pBestTri->m_Normal[1], pBestTri->m_Normal[2], 0.0f);
            t.m_Normal = nrm;
            D3DXVec4Transform(&t.m_Normal, &t.m_Normal, &m);
            struct Mtx { float f[16]; };
            *(Mtx *)t.m_Matrix = *(const Mtx *)&m;
            t.m_28 = pBestTri->m_Data;
            t.m_24 = 2;
            int cnt = pBestTri->m_Count;
            int fl = pBestTri->m_Flags;
            if (bestK == 0 && (fl & 1) && EXLineSeg(t.m_EdgePos, t.m_EdgeEnd - t.m_EdgePos).PtOnLine(t.m_30)) {
                t.m_2c = 0;
                t.m_24 = 4;
            } else if ((fl & (1 << (bestK + 1))) && EXLineSeg(t.m_EdgeEnd, t.m_EdgeUp - t.m_EdgeEnd).PtOnLine(t.m_30)) {
                t.m_2c = 1;
                t.m_24 = 4;
            } else if (bestK == cnt - 3 && (fl & (1 << (cnt - 1))) && EXLineSeg(t.m_EdgeUp, t.m_EdgePos - t.m_EdgeUp).PtOnLine(t.m_30)) {
                t.m_2c = 2;
                t.m_24 = 4;
            }
            found = 1;
        }
    }
    if (!found) return 0;
    t.m_radius = best >= 0.0f ? (float)sqrt(best) : 0.0f;
    return found;
}

// NEAR (see check): as the single query, but every ledge segment and grab triangle within t.m_radius is
// appended to t.m_points (the radius is not narrowed). Retail bug kept: a polygon entry's position is the
// last line hit (or zero), never the point on the triangle.
s8 EXGeoGrabPoint::DoGrabPoint(const EXMatrix &m, EXGrabPointArrayTemp &t) const
{
    EXVector p = t.m_pos;
    EXVector hit(0.0f, 0.0f, 0.0f, 0.0f);
    float r2 = t.m_radius * t.m_radius;
    s8 found = 0;
    if ((t.m_type & 1) && m_bLines) {
        EXScratchPad sp;
        const EXGeoGrabPoint_LineVert *v = m_Lines;
        for (int i = 0; i < m_nLines; i++) {
            EXVector a = GrabXForm(v->x, v->y, v->z, m);
            a.w = 0.0f;
            int n = v->m_Count;
            const EXGeoGrabPoint_LineVert *e = v + 1;
            for (int k = 0; k < n; k++, e++) {
                EXVector b = GrabXForm(e->x, e->y, e->z, m);
                b.w = 0.0f;
                EXLineSeg seg(a, b - a);
                int r = seg.NearestTo(p, hit);
                EXVector d = hit - p;
                if (d.x * d.x + d.y * d.y + d.z * d.z < r2) {
                    EXGrabPointData *pD = t.m_points.AddTail();
                    if (!pD) return found;
                    pD->m_EdgePos = a;
                    pD->m_EdgeEnd = b;
                    pD->m_EdgeUp = EXVector(a.x, a.y + 1.0f, a.z, a.w);
                    pD->m_Pos = hit;
                    EXVector dir = b - a;
                    float l = dir.w * dir.w + dir.z * dir.z + dir.y * dir.y + dir.x * dir.x;
                    if (l >= 0.0f && (l = (float)sqrt(l)) > 1e-7f) {
                        l = 1.0f / l;
                        dir.x *= l;
                        dir.y *= l;
                        dir.z *= l;
                        dir.w *= l;
                    }
                    pD->m_Normal = dir;
                    struct Mtx { float f[16]; };
                    *(Mtx *)pD->m_Matrix = *(const Mtx *)&m;
                    pD->m_Data = e->m_Data;
                    pD->m_Type = 1;
                    if (r == EXLineSeg::LINE_START) {
                        if (e->m_Flags & 1) {
                            pD->m_Corner = 0;
                            pD->m_Type = 3;
                        }
                    } else if (r == EXLineSeg::LINE_END && (e->m_Flags & 2)) {
                        pD->m_Corner = 1;
                        pD->m_Type = 3;
                    }
                    found = 1;
                }
                a = b;
            }
            v = e;
        }
    }
    if (t.m_type & 2) {
        const EXGeoGrabPoint_Tri *r = m_Tris;
        for (int i = 0; i < m_nTris; i++) {
            EXVector v0 = GrabXForm(r->m_v[0].x, r->m_v[0].y, r->m_v[0].z, m);
            EXVector v1 = GrabXForm(r->m_v[1].x, r->m_v[1].y, r->m_v[1].z, m);
            int k = 0;
            for (; k < r->m_Count - 2; k++) {
                EXVector v2 = GrabXForm(r->m_v[k + 2].x, r->m_v[k + 2].y, r->m_v[k + 2].z, m);
                EXTriangle tri;
                tri.m_v[0] = v0;
                tri.m_v[1] = v1 - v0;
                tri.m_v[2] = v2 - v0;
                float s, u;
                if (tri.MinPointSquared(p, s, u) < r2) {
                    EXGrabPointData *pD = t.m_points.AddTail();
                    if (!pD) return found;
                    pD->m_EdgePos = EXVector(v0.x, v0.y, v0.z, 0.0f);
                    pD->m_EdgeEnd = v1;
                    pD->m_EdgeUp = EXVector(v2.x, v2.y, v2.z, 0.0f);
                    pD->m_Pos = hit;
                    pD->m_Normal = EXVector(r->m_Normal[0], r->m_Normal[1], r->m_Normal[2], 0.0f);
                    D3DXVec4Transform(&pD->m_Normal, &pD->m_Normal, &m);
                    struct Mtx { float f[16]; };
                    *(Mtx *)pD->m_Matrix = *(const Mtx *)&m;
                    pD->m_Data = r->m_Data;
                    pD->m_Type = 2;
                    int cnt = r->m_Count;
                    int fl = r->m_Flags;
                    if (k == 0 && (fl & 1) && EXLineSeg(pD->m_EdgePos, v1 - pD->m_EdgePos).PtOnLine(pD->m_Pos)) {
                        pD->m_Corner = 0;
                        pD->m_Type = 4;
                    } else if ((fl & (1 << (k + 1))) && EXLineSeg(v1, v2 - v1).PtOnLine(pD->m_Pos)) {
                        pD->m_Corner = 1;
                        pD->m_Type = 4;
                    } else if (k == cnt - 3 && (fl & (1 << (cnt - 1))) &&
                               EXLineSeg(pD->m_EdgeUp, pD->m_EdgePos - pD->m_EdgeUp).PtOnLine(pD->m_Pos)) {
                        pD->m_Corner = 2;
                        pD->m_Type = 4;
                    }
                    found = 1;
                }
                v1 = v2;
            }
            r = (const EXGeoGrabPoint_Tri *)((const u8 *)r + (k + 3) * 0x10);
        }
    }
    return found;
}
