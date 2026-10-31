#include "XGeoNavMeshEntity.h"
#include "XNavMeshHandler.h"
#include "EXGeoHeader.h"
#include "EXPlane.h"
#include "XItem.h"
#include "EXRayCastTemp.h"
#include "EXMemory.h"
#include "FEText.h"

#include "AIControl.h"

EXVector gZeroVector(0.0f, 0.0f, 0.0f, 0.0f);

EX_IMPLEMENT_DYNAMIC(XGeoNavMeshEntity, EXGeoBaseEntity)
EX_IMPLEMENT_GEOOBJECT(XGeoNavMeshEntity, 0x605)

void XGeoNavMeshEntity::DoBackup() {}

void XGeoNavMeshEntity::DoRestore() {}

s8 XGeoNavMeshEntity::DoCheck() { return 1; }

s8 XGeoNavMeshEntity::PostLoadInit() { return 1; }

s8 XGeoNavMeshEntity::PolyPlane(int poly, EXPlane &p)
{
    if (poly < 0 || poly >= (int)m_dwPolyCount) return 0;
    float *f = PolyBounds(poly)->m_Plane;
    p.set(f[0], f[1], f[2], f[3]);
    return 1;
}

EXVector XGeoNavMeshEntity::RandomPosition(u16 zone)
{
    if (m_pNavNetwork) return m_pNavNetwork->RandomPosition(zone);
    return gZeroVector;
}

s8 XGeoNavMeshEntity::ValidateMesh()
{
    u32 n = m_dwPolyCount;
    return n == m_pNavNetwork->NodeCount();
}

// NEAR: 5 bytes: order of the first two triangle stores.
void XGeoNavMeshEntity::TrisFromPoly(int poly, EXGeoNavMeshEntity_PolyList *p, int &n, EXGeoNavMeshEntity_TriList *t)
{
    u16 *tri = (u16 *)t;
    tri[0] = (u16)poly;
    tri[1] = p->m_wVerts[0];
    tri[0] = (u16)poly;
    tri[2] = p->m_wVerts[1];
    tri[0] = (u16)poly;
    tri[3] = p->m_wVerts[2];
    tri += 4;
    n++;
    for (int i = 3; i < p->m_wNumVerts; i++) {
        tri[0] = (u16)poly;
        tri[1] = p->m_wVerts[0];
        tri[2] = p->m_wVerts[i - 1];
        tri[3] = p->m_wVerts[i];
        tri += 4;
        n++;
    }
}

void XGeoNavMeshEntity::OnDelete(EXGeoHeader *, EXGeoObjectHeader *)
{
    if (gAIControl.IsDoingHTMLReport()) return;
    gNavMeshHandler.RemoveNavMesh(this);
    if (m_pPolyBoundsAlloc) {
        EXFree(m_pPolyBoundsAlloc);
        m_pPolyBoundsAlloc = 0;
    }
    if (m_pBoundsSectionsAlloc) {
        EXFree(m_pBoundsSectionsAlloc);
        m_pBoundsSectionsAlloc = 0;
    }
    if (m_pZonesAlloc) {
        EXFree(m_pZonesAlloc);
        m_pZonesAlloc = 0;
    }
    if (m_pNavNetwork) {
        delete m_pNavNetwork;
        m_pNavNetwork = 0;
    }
}

void XGeoNavMeshEntity::DoSubMapOffsets()
{
    if (!m_wVertOffset) return;
    for (u16 i = 0; i < m_dwPolyCount; i++) {
        EXGeoNavMeshEntity_PolyList &p = Poly(i);
        for (u16 j = 0; j < p.m_wNumVerts; j++) p.m_wVerts[j] += m_wVertOffset;
    }
}

// NEAR: argument evaluation order of the rel-ptr conversions passed to Loaded.
s8 XGeoNavMeshEntity::UpdatePtrs()
{
    if (!m_pPolyBoundsAlloc && m_pPolyBounds) m_pPolyBounds = m_PolyBounds;
    if (!m_pBoundsSectionsAlloc && m_pBoundsSections) m_pBoundsSections = m_BoundsSections;
    if (!m_pZonesAlloc && m_pZones) m_pZones = m_Zones;
    if (!m_pNavNetwork->Loaded(m_dwHashCode, m_wNodes, m_Nodes, m_wLinks, m_Links, m_NavData, 0, m_dwVersion, 0, m_NodeData,
                               m_wRanges, m_RangeData))
        while (1) {}
    return 1;
}

// NEAR: vertex base reload/pointer stepping in the copy loop differs.
s8 XGeoNavMeshEntity::PolyVerts(int poly, EXVector *v, int *n)
{
    EXGeoNavMeshEntity_PolyList &p = Poly(poly);
    int count = p.m_wNumVerts;
    if (n) *n = count;
    if (!v) return 0;
    for (int i = 0; i < count; i++) {
        EXVector3 &pos = VertexPos(p.m_wVerts[i]);
        v[i].x = pos.x;
        v[i].y = pos.y;
        v[i].z = pos.z;
        v[i].w = 0.0f;
        v[i].w = 0.0f;
    }
    return 1;
}

s8 XGeoNavMeshEntity::PolyRGBAs(int poly, u32 *c, int *n)
{
    EXGeoNavMeshEntity_PolyList &p = Poly(poly);
    int count = p.m_wNumVerts;
    if (n) *n = count;
    if (!c) return 0;
    for (int i = 0; i < count; i++) c[i] = GetVertexRGBA(p.m_wVerts[i]);
    return 1;
}

s8 XGeoNavMeshEntity::PolyNormal(int poly, EXVector &n)
{
    if (poly < 0 || poly >= (int)m_dwPolyCount) return 0;
    float *f = PolyBounds(poly)->m_Plane;
    n = EXVector(f[0], f[1], f[2], 0.0f);
    return 1;
}

// NEAR: plane locals/x87 ordering of the inlined HeightTo differ.
float XGeoNavMeshEntity::GetHeight(const EXVector &p, int poly, int, EXVector *pNormal)
{
    if (poly < 0 || poly >= (int)m_dwPolyCount) return 0.0f;
    float *f = PolyBounds(poly)->m_Plane;
    EXPlane plane(f[0], f[1], f[2], f[3]);
    if (pNormal) {
        pNormal->x = plane.A;
        pNormal->y = plane.B;
        pNormal->z = plane.C;
        pNormal->w = 0.0f;
    }
    EXVector3 &v = PolyVertexPos(poly, 0);
    EXVector vtx(v.x, v.y, v.z, 0.0f);
    return plane.HeightTo(vtx, p);
}

s8 XGeoNavMeshEntity::PosIsValid(const EXVector &p, int &poly) { return LocatePolygon(p, &poly, 0, 0); }

float XGeoNavMeshEntity::GetFloorHeight(const EXVector &p)
{
    int poly = -1;
    int tri = 0;
    LocatePolygon(p, &poly, &tri, 0);
    if (poly < 0) return p.y;
    return GetHeight(p, poly, tri, 0);
}

s8 XGeoNavMeshEntity::ValidatePos(const EXVector &p, EXVector &out, int *pPoly) { return ValidatePos(p, 0, out, pPoly); }

// NEAR: retail aligns the stack for the XNavMeshData copy (and esp,-8).
void XGeoNavMeshEntity::OnCreate(EXGeoHeader *pHeader, EXGeoObjectHeader *)
{
    if (gAIControl.IsDoingHTMLReport()) return;
    XNavMeshData data = *(XNavMeshData *)&m_dwVersion;
    if (!InitLoadedData(&data)) while (1) {}
    gNavMeshHandler.AddNavMesh(this, pHeader->HashCode());
}

// 2D (x/z) point in polygon: bounding box from the cached extreme vertices, then a crossing count
// along +x.
s8 XGeoNavMeshEntity::PointInPolygon(const EXVector &p, int poly)
{
    if (poly < 0) return 0;
    EXGeoNavMeshEntity_PolyList &pl = Poly(poly);
    float dz[16];
    float dx[16];
    int i;
    for (i = 0; i < pl.m_wNumVerts; i++) {
        dx[i] = VertexPos(pl.m_wVerts[i]).x - p.x;
        dz[i] = VertexPos(pl.m_wVerts[i]).z - p.z;
    }
    int count = pl.m_wNumVerts;
    if (dx[pl.m_MinVerts >> 4] <= 0.0f && dx[pl.m_MaxVerts >> 4] >= 0.0f && dz[pl.m_MinVerts & 0xf] <= 0.0f
        && dz[pl.m_MaxVerts & 0xf] >= 0.0f) {
        int crossings = 0;
        int j = 1;
        for (i = 0; i < count; i++, j++) {
            if (j == count) j = 0;
            float zi = dz[i];
            float zj = dz[j];
            if ((zi < 0.0f && zj >= 0.0f) || (zi >= 0.0f && zj < 0.0f)) {
                float xi = dx[i];
                float xj = dx[j];
                if (xi >= 0.0f && xj >= 0.0f) {
                    crossings++;
                } else if (xi >= 0.0f || xj >= 0.0f) {
                    float ddz = zj - zi;
                    if (ddz != 0.0f && xi - (xj - xi) / ddz * zi > 0.0f) crossings++;
                }
            }
        }
        return crossings & 1;
    }
    return 0;
}

// EXPlane(a, b, c) as inlined here (the out-of-line copy is in EXPlane.obj).
static __forceinline void Nav_PlaneFrom3(EXPlane &pl, const EXVector &a, const EXVector &b, const EXVector &c)
{
    EXVector ba = b - a;
    EXVector cb = c - b;
    EXVector n = ba.Cross(cb);
    pl.A = n.x; pl.B = n.y; pl.C = n.z; pl.D = n.w;
    float len = EXSqrt(pl.A * pl.A + pl.B * pl.B + pl.C * pl.C);
    if ((float)fabs(len) != 0.0f) {
        float inv = 1.0f / len;
        pl.A = inv * pl.A; pl.B *= inv; pl.C *= inv; pl.D *= inv;
        pl.D = 0.0f - (pl.A * a.x + pl.B * a.y + pl.C * a.z);
    } else {
        pl.A = 1.0f; pl.B = 0.0f; pl.C = 0.0f; pl.D = 0.0f;
    }
}

static __forceinline float Nav_PlaneDot(const EXPlane &pl, EXVector v) { return ((const EXMaths::vec_xyzw &)pl).Dot(v); }

// 3D point in polygon: inside every edge plane standing along the polygon normal.
s8 XGeoNavMeshEntity::PointInPolygon3D(const EXVector &p, int poly)
{
    int n = 0;
    EXVector v[16];
    if (poly < 0 || !PolyVerts(poly, v, &n)) return 0;
    EXVector pt = p;
    pt.w = 1.0f;
    EXVector normal;
    PolyNormal(poly, normal);
    for (int i = 0, j = 1; i < n; i++, j++) {
        if (j >= n) j = 0;
        EXPlane pl;
        Nav_PlaneFrom3(pl, v[i], v[j], v[j] - normal);
        if (Nav_PlaneDot(pl, pt) < 0.0f) return 0;
    }
    return 1;
}

// Polygon containing p (x/z), the cached one first; when several overlap, the one whose surface is
// nearest p in height. Disabled polygons (flag 0x40) are skipped.
s8 XGeoNavMeshEntity::LocatePolygon(const EXVector &p, int *pPoly, int *pTri, u32)
{
    if (*pPoly >= 0 && (u32)*pPoly < m_dwPolyCount && PointInPolygon(p, *pPoly)) {
        if (pTri) *pTri = 0;
        return 1;
    }
    float bestDiff = 0.0f;
    float diff = 0.0f;
    int best = -1;
    bool bHaveBest = false;
    u32 count = m_dwPolyCount;
    _XNavPolyBounds *b = m_pPolyBounds;
    for (u32 i = 0; i < count; i++, b++) {
        if (b->m_MinX > p.x || p.x > b->m_MaxX || b->m_MinZ > p.z || p.z > b->m_MaxZ) continue;
        if (Poly(i).m_wFlags & 0x40) continue;
        if (!PointInPolygon(p, i)) continue;
        if (best >= 0) {
            if (!bHaveBest) {
                bestDiff = (float)fabs(GetHeight(p, best, 0, 0) - p.y);
                bHaveBest = true;
            }
            diff = (float)fabs(GetHeight(p, i, 0, 0) - p.y);
            if (!(bestDiff > diff)) continue;
        }
        best = i;
        bestDiff = diff;
    }
    if (best < 0) return 0;
    *pPoly = best;
    if (pTri) *pTri = 0;
    return 1;
}

static inline float Nav_PlaneDist(const float *pl, EXVector v) { return pl[1] * v.y + pl[2] * v.z + pl[3] * v.w + v.x * pl[0]; }

// 3D polygon containing p: searched per bounds section, the nearest polygon plane wins. Polygons
// with any of the given flags are skipped.
s8 XGeoNavMeshEntity::LocatePolygon3D(const EXVector &p, int *pPoly, int *pTri, u32 flags)
{
    float bestDist = 0.0f;
    _XNavPolyBounds *pBounds = m_pPolyBounds;
    _XNavBoundsSection *pSection = m_pBoundsSections;
    EXVector pt(p.x, p.y, p.z, 1.0f);
    int best = -1;
    for (int s = 0; s < m_wBoundsSections; s++, pSection++) {
        if (!pSection->m_Bounds._Intersect(p, 1.0f)) continue;
        if (pSection->m_wFirstPoly != 0xffff) {
            EXGeoNavMeshEntity_PolyList *pl = &Poly(pSection->m_wFirstPoly);
            for (int j = 0; j < pSection->m_wPolyCount; j++, pl++) {
                if (flags && (flags & pl->m_wFlags)) continue;
                int poly = pSection->m_wFirstPoly + j;
                float d = Nav_PlaneDist(pBounds[poly].m_Plane, pt);
                if ((best < 0 || !(fabs(d) > fabs(bestDist))) && PointInPolygon3D(p, poly)) {
                    best = poly;
                    bestDist = d;
                }
            }
        } else {
            for (int j = 0; j < pSection->m_wPolyCount; j++) {
                int poly = pSection->m_pPolyList[j];
                if (flags && (flags & Poly(poly).m_wFlags)) continue;
                float d = Nav_PlaneDist(pBounds[poly].m_Plane, pt);
                if ((best < 0 || !(fabs(d) > fabs(bestDist))) && PointInPolygon3D(p, poly)) {
                    best = poly;
                    bestDist = d;
                }
            }
        }
    }
    if (best < 0) return 0;
    *pPoly = best;
    if (pTri) *pTri = 0;
    return 1;
}

// Nearest polygon hit by the ray from along dir (bounds sections around from). Section polygon lists
// fall back to the nearest-plane point test.
s8 XGeoNavMeshEntity::LocatePolygon3DRay(const EXVector &from, const EXVector &d, int *pPoly, u32 flags, float *pT,
                                         EXVector *pHit)
{
    float bestT = 10000.0f;
    _XNavPolyBounds *pBounds = m_pPolyBounds;
    EXVector pt(from.x, from.y, from.z, 1.0f);
    EXVector dir = d;
    int best = -1;
    float len = EXSqrt(dir.Dot(dir));
    if (len > EX_EPSILON_UNIT) dir *= 1.0f / len;
    EXVector tmp = from;
    if (pHit)
        *pHit = from;
    else
        pHit = &tmp;
    _XNavBoundsSection *pSection = m_pBoundsSections;
    for (int s = 0; s < m_wBoundsSections; s++, pSection++) {
        if (!pSection->m_Bounds._Intersect(from, 1.0f)) continue;
        if (pSection->m_wFirstPoly != 0xffff) {
            EXGeoNavMeshEntity_PolyList *pl = &Poly(pSection->m_wFirstPoly);
            for (int j = 0; j < pSection->m_wPolyCount; j++, pl++) {
                if (flags && (flags & pl->m_wFlags)) continue;
                int poly = pSection->m_wFirstPoly + j;
                float *pf = pBounds[poly].m_Plane;
                float denom = dir.x * pf[0] + dir.z * pf[2] + dir.w * pf[3] + dir.y * pf[1];
                if (fabs(denom) >= 1e-10f) {
                    float t = 0.0f - (from.y * pf[1] + pf[2] * from.z + pf[0] * from.x + pf[3]) / denom;
                    if (pf[2] * dir.z + pf[1] * dir.y + pf[0] * dir.x + dir.w * 0.0f <= 0.0f && t >= 0.0f
                        && (best < 0 || t <= bestT)) {
                        *pHit = from + dir * t;
                        if (PointInPolygon3D(*pHit, poly)) {
                            best = poly;
                            bestT = t;
                        }
                    }
                }
            }
        } else {
            for (int j = 0; j < pSection->m_wPolyCount; j++) {
                int poly = pSection->m_pPolyList[j];
                if (flags && (flags & Poly(poly).m_wFlags)) continue;
                float dist = Nav_PlaneDist(pBounds[poly].m_Plane, pt);
                if ((best < 0 || !(fabs(dist) > fabs(bestT))) && PointInPolygon3D(from, poly)) {
                    best = poly;
                    bestT = dist;
                }
            }
        }
    }
    if (best < 0) return 0;
    if (pPoly) *pPoly = best;
    if (pT) *pT = bestT;
    return 1;
}

// Squared distance from p to the nearest point on the polygon's edges (1e6 when none), that point in pNearest.
float XGeoNavMeshEntity::SquaredDistanceToPolygon(int poly, const EXVector &p, EXVector *pNearest)
{
    float best = 1000000.0f;
    int n;
    EXVector v[16];
    EXVector nearest;
    if (!PolyVerts(poly, v, &n)) return best;
    for (int i = 0, j = 1; i < n; i++, j++) {
        if (j >= n) j = 0;
        EXVector e = v[j] - v[i];
        EXVector w = p - v[i];
        float t = (w.x * e.x + w.y * e.y + w.z * e.z) / (e.x * e.x + e.y * e.y + e.z * e.z);
        EXVector q;
        if (t < 0.0f)
            q = v[i];
        else if (t > 1.0f)
            q = v[j];
        else
            q = v[i] + e * t;
        EXVector dd = p - q;
        float d = dd.x * dd.x + dd.y * dd.y + dd.z * dd.z;
        if (d < best) {
            best = d;
            nearest = q;
        }
    }
    if (best != 1000000.0f && pNearest) *pNearest = nearest;
    return best;
}

// Polygon whose edges come nearest p (x/z culling by zone and node radius). Without flags only
// polygons with exterior edges count, and only those edges, seen from their outer side. pOut gets
// the nearest edge point nudged 2cm towards the polygon centre.
int XGeoNavMeshEntity::NearestPolygon(const EXVector &p, EXVector *pOut, u32 flags)
{
    _XNavZone *pZones = m_pZones;
    XNavNode *pBestNode = 0;
    int best = -1;
    float bestDist = 1e8f;
    float bestDistSq = 1e8f;
    EXVector bestPt;
    for (int z = 0; z < m_wZones; z++) {
        EXVector d = EXVector(pZones[z].m_Pos.x, pZones[z].m_Pos.y, pZones[z].m_Pos.z, 0.0f) - p;
        float dist = EXSqrt(d.x * d.x + d.z * d.z) - pZones[z].m_Radius;
        pZones[z].m_Dist = dist;
        pZones[z].m_DistSq = dist * dist;
    }
    int count = m_dwPolyCount;
    XNavNode *pNode = m_pNavNetwork->NavSection().Node(m_wPolyOffset);
    EXGeoNavMeshEntity_PolyList *pl = &Poly(0);
    for (int i = 0; i < count; i++, pl++, pNode++) {
        if (flags & pl->m_wFlags) continue;
        if (!flags && !pNode->m_wXEdges) continue;
        if (pZones[pl->m_wZone].m_Dist > bestDist) continue;
        EXVector d = EXVector(pNode->m_Pos.x, pNode->m_Pos.y, pNode->m_Pos.z, 0.0f) - p;
        if (EXSqrt(d.x * d.x + d.z * d.z) - pNode->m_fRadius > bestDist) continue;
        int n;
        EXVector v[16];
        u8 usage[16];
        PolyVerts(i, v, &n);
        pNode->GetEdgeUsage(usage);
        for (int e = 0, j = 1; e < n; e++, j++) {
            if (!flags && usage[e]) continue;
            int k = j;
            if (k >= n) k = 0;
            EXVector edge = v[k] - v[e];
            EXVector w = p - v[e];
            if ((0.0f - edge.z) * w.x + w.z * edge.x < 0.0f) continue;
            float lenSq = edge.Dot(edge);
            float t;
            if (fabs(lenSq) >= 0.001f)
                t = (w.x * edge.x + w.z * edge.z) / lenSq;
            else if (fabs(edge.y) >= 0.001f)
                t = w.y * edge.y / edge.y;
            else
                t = 0.0f;
            EXVector q;
            if (t < 0.0f)
                q = v[e];
            else if (t > 1.0f)
                q = v[k];
            else
                q = v[e] + edge * t;
            EXVector dq = p - q;
            float dsq = dq.x * dq.x + dq.y * dq.y + dq.z * dq.z;
            if (dsq < bestDistSq) {
                best = i;
                bestPt = q;
                pBestNode = pNode;
                bestDistSq = dsq;
                bestDist = EXSqrt(dsq);
            }
        }
    }
    if (pBestNode) {
        EXVector dir(pBestNode->m_Pos.x - bestPt.x, 0.0f, pBestNode->m_Pos.z - bestPt.z, 0.0f);
        dir.NormaliseXYZ();
        *pOut = bestPt + dir * 0.02f;
    }
    return best;
}

// Nearest polygon in 3D: picks the bounds section nearest p (with a usable polygon), then the
// polygon there whose plane (when p projects inside it) or edges come closest. pOut gets the point
// found, nudged 1mm towards the polygon centre when it is on an edge.
int XGeoNavMeshEntity::NearestPolygon3D(const EXVector &p, EXVector *pOut, u32 flags)
{
    EXVector pt(p.x, p.y, p.z, 1.0f);
    _XNavPolyBounds *pBounds = m_pPolyBounds;
    _XNavBoundsSection *pSection = m_pBoundsSections;
    int nSections = m_wBoundsSections;
    int bestSection = -1;
    float bestDist = 0.0f;
    for (int s = 0; s < nSections; s++, pSection++) {
        float dx = 0.0f, dy = 0.0f, dz = 0.0f;
        if (pSection->m_Bounds.m_vctMin.x <= p.x) {
            if (p.x > pSection->m_Bounds.m_vctMax.x) dx = p.x - pSection->m_Bounds.m_vctMax.x;
        } else {
            dx = pSection->m_Bounds.m_vctMin.x - p.x;
        }
        if (pSection->m_Bounds.m_vctMin.y <= p.y) {
            if (p.y > pSection->m_Bounds.m_vctMax.y) dy = p.y - pSection->m_Bounds.m_vctMax.y;
        } else {
            dy = pSection->m_Bounds.m_vctMin.y - p.y;
        }
        if (pSection->m_Bounds.m_vctMin.z <= p.z) {
            if (p.z > pSection->m_Bounds.m_vctMax.z) dz = p.z - pSection->m_Bounds.m_vctMax.z;
        } else {
            dz = pSection->m_Bounds.m_vctMin.z - p.z;
        }
        float d = dz * dz + dy * dy + dx * dx;
        if ((bestSection < 0 || d < bestDist) && pSection->m_wFirstPoly != 0xffff) {
            EXGeoNavMeshEntity_PolyList *pl = &Poly(pSection->m_wFirstPoly);
            for (int j = 0; j < pSection->m_wPolyCount; j++, pl++) {
                if (!flags || !(flags & pl->m_wFlags)) {
                    bestSection = s;
                    bestDist = d;
                    break;
                }
            }
        }
    }
    EXVector nearest(0.0f, 0.0f, 0.0f, 0.0f);
    EXVector result(0.0f, 0.0f, 0.0f, 0.0f);
    int best = -1;
    if (bestSection >= 0) {
        _XNavBoundsSection *bs = &m_pBoundsSections[bestSection];
        if (bs->m_wFirstPoly == 0xffff) return -1;
        EXGeoNavMeshEntity_PolyList *pl = &Poly(bs->m_wFirstPoly);
        for (int j = 0; j < bs->m_wPolyCount; j++, pl++) {
            int poly;
            if (bs->m_wFirstPoly == 0xffff) {
                poly = bs->m_pPolyList[j];
                pl = &Poly(poly);
            } else {
                poly = bs->m_wFirstPoly + j;
            }
            if (flags && (flags & pl->m_wFlags)) continue;
            if (best >= 0) {
                XNavSection &sec = m_pNavNetwork->NavSection();
                XNavNode *pNode = sec.Node(poly);
                EXVector d = EXVector(pNode->m_Pos.x, pNode->m_Pos.y, pNode->m_Pos.z, 0.0f) - p;
                float dsq = d.LengthSquared();
                if (fabs(bestDist) < dsq - sec.Node(poly)->m_fRadius * sec.Node(poly)->m_fRadius) continue;
            }
            float *pf = pBounds[poly].m_Plane;
            float dist = Nav_PlaneDist(pf, pt);
            float distSq = dist * dist;
            if (best >= 0 && fabs(distSq) > fabs(bestDist)) continue;
            if (PointInPolygon3D(p, poly)) {
                result = p - EXVector(pf[0], pf[1], pf[2], 0.0f) * dist;
                best = poly;
                bestDist = distSq;
            } else {
                float sq = SquaredDistanceToPolygon(poly, p, &nearest);
                if (best < 0 || !(fabs(sq) > fabs(bestDist))) {
                    XNavNode *pNode = m_pNavNetwork->NavSection().Node(poly);
                    EXVector dir = EXVector(pNode->m_Pos.x, pNode->m_Pos.y, pNode->m_Pos.z, 0.0f) - nearest;
                    dir.Normalise();
                    result = nearest + dir * 0.001f;
                    best = poly;
                    bestDist = sq;
                    nearest = result;
                }
            }
        }
        if (best >= 0 && pOut) *pOut = result;
    }
    return best;
}

// Pulls p inside the polygon's edges (edge planes standing along the polygon normal, or straight
// up for 2D meshes), nudges it 1mm towards the centre and puts it on the polygon surface.
s8 XGeoNavMeshEntity::SnapToPolygon(EXVector &p, int poly)
{
    float up[3] = {0.0f, 1.0f, 0.0f};
    if (m_b3D && poly >= 0 && poly < (int)m_dwPolyCount) {
        float *f = PolyBounds(poly)->m_Plane;
        up[0] = f[0];
        up[1] = f[1];
        up[2] = f[2];
    }
    p.w = 1.0f;
    int n;
    EXVector v[16];
    PolyVerts(poly, v, &n);
    for (int i = 0, j = 1; i < n; i++, j++) {
        if (j >= n) j = 0;
        EXVector c(v[j].x + up[0], v[j].y + up[1], v[j].z + up[2], v[j].w);
        EXPlane pl;
        Nav_PlaneFrom3(pl, v[i], v[j], c);
        float d = pl.A * p.x + p.w * pl.D + p.z * pl.C + p.y * pl.B;
        if (d > 0.0f) p -= EXVector(pl.A, pl.B, pl.C, 0.0f) * d;
    }
    XNavNode *pNode = m_pNavNetwork->NavSection().Node(poly);
    EXVector dir = EXVector(pNode->m_Pos.x, pNode->m_Pos.y, pNode->m_Pos.z, 0.0f) - p;
    dir.Normalise();
    p += dir * 0.001f;
    float A, B, C, D;
    if (!m_b3D) {
        if (poly >= 0 && poly < (int)m_dwPolyCount) {
            float *f = PolyBounds(poly)->m_Plane;
            A = f[0];
            B = f[1];
            C = f[2];
        }
        p.w = 1.0f;
        float inv = B;
        if (B != 0.0f) inv = 1.0f / B;
        p.y = (v[0].x - p.x) * A * inv + (v[0].z - p.z) * C * inv + v[0].y;
        p.w = 0.0f;
        return 1;
    }
    if (poly >= 0 && poly < (int)m_dwPolyCount) {
        float *f = PolyBounds(poly)->m_Plane;
        D = f[3];
        A = f[0];
        B = f[1];
        C = f[2];
    }
    p.w = 1.0f;
    float d = p.w * D + p.z * C + p.y * B + A * p.x;
    p.w = 0.0f;
    p.x -= A * d;
    p.y -= B * d;
    p.z -= C * d;
    p.w = 0.0f;
    return 1;
}

// Projects p onto the polygon plane (3D meshes), or (2D) offsets its height by the plane height there.
s8 XGeoNavMeshEntity::SnapToPolygonPlane(EXVector &p, int poly)
{
    if (poly < 0) return 0;
    EXPlane pl;
    if (Is3D()) {
        if (poly < (int)m_dwPolyCount) {
            float *f = PolyBounds(poly)->m_Plane;
            pl.set(f[0], f[1], f[2], f[3]);
        }
        p.w = 1.0f;
        EXMaths::vec_xyzw q = p;
        float d = q.Dot((const EXMaths::vec_xyzw &)pl);
        p -= EXVector(pl.A, pl.B, pl.C, 0.0f) * d;
        p.w = 0.0f;
        return 1;
    }
    if (poly < (int)m_dwPolyCount) {
        float *f = PolyBounds(poly)->m_Plane;
        pl.A = f[0];
        pl.B = f[1];
        pl.C = f[2];
    }
    p.w = 1.0f;
    EXVector3 &v = PolyVertexPos(poly, 0);
    if (pl.B != 0.0f) pl.B = 1.0f / pl.B;
    p.y -= (v.z - p.z) * pl.C * pl.B + (v.x - p.x) * pl.A * pl.B + v.y;
    p.w = 0.0f;
    return 1;
}

// Pushes p back inside the polygon through each edge plane it is outside of (each edge once; when two
// edges push it, snaps to their shared vertex), then 5mm towards the polygon centre.
s8 XGeoNavMeshEntity::SnapToPolygonBounds(EXVector &p, int poly)
{
    float upx = 0.0f, upy = 1.0f, upz = 0.0f;
    if (m_b3D && poly >= 0 && poly < (int)m_dwPolyCount) {
        float *f = PolyBounds(poly)->m_Plane;
        upy = f[1];
        upz = f[2];
        upx = f[0];
    }
    p.w = 1.0f;
    int n;
    EXVector v[16];
    PolyVerts(poly, v, &n);
    int count = n;
    s8 done[16];
    memset(done, 0, sizeof(done));
    EXPlane planes[16];
    int i, j;
    for (i = 0, j = 1; i < n; i++, j++) {
        if (i >= n) i = 0;
        if (j >= n) j = 0;
        EXVector c(v[j].x + upx, v[j].y + upy, v[j].z + upz, v[j].w);
        Nav_PlaneFrom3(planes[i], v[i], v[j], c);
    }
    int hits = 0;
    int hitEdges[16];
    int k;
    for (k = 0, i = 0, j = 1; k < n; k++, i++, j++) {
        if (i >= count) i = 0;
        if (j >= count) j = 0;
        float d = Nav_PlaneDist(&planes[i].A, p);
        if (d >= 0.0f && !done[i]) {
            p -= EXVector(planes[i].A, planes[i].B, planes[i].C, 0.0f) * d;
            hitEdges[hits++] = j;
            if (hits == 1) n += count - 1;
            done[i] = 1;
        }
    }
    if (hits > 1) {
        if (hitEdges[0] == hitEdges[1] + 1)
            p = v[hitEdges[1]];
        else
            p = v[hitEdges[0]];
    }
    p.w = 0.0f;
    XNavNode *pNode = m_pNavNetwork->Node((u16)poly);
    EXVector dir(pNode->m_Pos.x - p.x, pNode->m_Pos.y - p.y, pNode->m_Pos.z - p.z, 0.0f);
    if (!m_b3D) dir.y = 0.0f;
    dir.NormaliseXYZ();
    p += dir * 0.005f;
    return 1;
}

// Old meshes (version < 0x18): vertex colours from the map face below each vertex (grey when none).
void XGeoNavMeshEntity::SetMeshLighting()
{
    if (m_dwVersion >= 0x18) return;
    for (u32 i = 0; i < m_dwVertCount; i++) {
        EXRayCastTemp t;
        EXVector3 &v = VertexPos(i);
        EXVector from(v.x, v.y + 0.5f, v.z, 0.0f);
        EXVector3 &v2 = VertexPos(i);
        EXVector dir = EXVector(v2.x, v2.y - 4.0f, v2.z, 0.0f) - from;
        t.Set(from, dir, 0, 0, 0, 0);
        t.m_layer.m_flags |= 0x400;
        theItemEnv.DoRayCastToMap(t, 0);
        if (!t.m_bHit) {
            SetVertexRGBA(i, 0x808080);
        } else {
            t.Intersect();
            EXVector3 &v3 = VertexPos(i);
            EXVector c = t.m_face.GetFaceColourV(2, EXVector(v3.x, v3.y, v3.z, 0.0f));
            u8 r = (u8)(c.x * 255.0f);
            u8 g = (u8)(c.y * 255.0f);
            u8 b = (u8)(c.z * 255.0f);
            SetVertexRGBA(i, ((r << 8 | g) << 8) | b);
        }
    }
}

static __forceinline float Nav_Bary(const EXVector &a, const EXVector &b, const EXVector &n, const EXVector &corner,
                                    const EXVector &pt)
{
    EXPlane pl;
    Nav_PlaneFrom3(pl, a, b, b + n);
    float d = ((const EXMaths::vec_xyzw &)pl).Dot(corner);
    if (d != 0.0f) return ((const EXMaths::vec_xyzw &)pl).Dot(pt) / d;
    return 0.0f;
}

// Vertex lighting at p: barycentric blend of the colours of the fan triangle (v0, va, vb) holding p.
s8 XGeoNavMeshEntity::GetLighting(const EXVector &p, int poly, float &r, float &g, float &b)
{
    int n;
    EXVector v[16];
    u32 c[16];
    PolyVerts(poly, v, &n);
    if (!PolyRGBAs(poly, c, &n)) return 0;
    EXVector normal;
    PolyNormal(poly, normal);
    EXVector pt(p.x, p.y, p.z, 1.0f);
    int a = 1, bb = 2;
    if (n > 3) {
        for (int i = 1, j = 2; i < n; i++, j++) {
            if (j >= n) j = 0;
            EXPlane pl;
            Nav_PlaneFrom3(pl, v[0], v[j], v[j] + normal);
            if (((const EXMaths::vec_xyzw &)pl).Dot(pt) >= 0.0f) {
                a = i;
                bb = j;
                break;
            }
        }
    }
    u32 c0 = c[0], ca = c[a], cb = c[bb];
    v[0].w = 1.0f;
    v[a].w = 1.0f;
    v[bb].w = 1.0f;
    r = g = b = 0.0f;
    float w = Nav_Bary(v[bb], v[a], normal, v[0], pt);
    r = (c0 & 0xff) * (1.0f / 255.0f) * w;
    g += (c0 >> 8 & 0xff) * (1.0f / 255.0f) * w;
    b += (c0 >> 16 & 0xff) * (1.0f / 255.0f) * w;
    w = Nav_Bary(v[0], v[bb], normal, v[a], pt);
    r += (ca & 0xff) * (1.0f / 255.0f) * w;
    g += (ca >> 8 & 0xff) * (1.0f / 255.0f) * w;
    b += (ca >> 16 & 0xff) * (1.0f / 255.0f) * w;
    w = Nav_Bary(v[a], v[0], normal, v[bb], pt);
    r += (cb & 0xff) * (1.0f / 255.0f) * w;
    g += (cb >> 8 & 0xff) * (1.0f / 255.0f) * w;
    b += (cb >> 16 & 0xff) * (1.0f / 255.0f) * w;
    return 1;
}

// Nearest point to p on the polygon's edges.
s8 XGeoNavMeshEntity::ClampPointToPoly(int poly, const EXVector &p, EXVector &out)
{
    EXVector q = p;
    int n = Poly(poly).m_wNumVerts;
    float best = 1000000.0f;
    if (!n) return 0;
    for (int i = 0, j = 1; i < n; i++, j++) {
        int k = j;
        if (k >= n) k = 0;
        float t;
        EXVector3 &vk = PolyVertexPos(poly, k);
        EXVector b(vk.x, vk.y, vk.z, 0.0f);
        EXVector3 &vi = PolyVertexPos(poly, i);
        EXVector a(vi.x, vi.y, vi.z, 0.0f);
        EXVector np = XNavNetwork::NearestPointOnLineSegment(a, b, q, &t);
        EXVector d = q - np;
        float dist = d.Length();
        if (dist < best) {
            out = np;
            best = dist;
        }
    }
    return best != 1000000.0f;
}

// p is valid when inside a polygon whose node has none of the flags (8 always). Otherwise out gets
// the nearest point on an allowed polygon (or the nearest valid node's polygon) and 0 is returned.
s8 XGeoNavMeshEntity::ValidatePos(const EXVector &p, u32 flags, EXVector &out, int *pPoly)
{
    u32 mask = flags | 8;
    int poly = -1;
    if (pPoly) poly = *pPoly;
    LocatePolygon(p, &poly, 0, 0);
    if (poly >= 0 && !(mask & m_pNavNetwork->NavSection().Node(poly)->m_wTypeFlags)) {
        if (pPoly) *pPoly = poly;
        out = p;
        return 1;
    }
    poly = NearestPolygon(p, &out, mask);
    if (poly < 0) {
        out = p;
        return 0;
    }
    if (!(mask & m_pNavNetwork->NavSection().Node(poly)->m_wTypeFlags)) {
        if (pPoly) *pPoly = poly;
    } else {
        XNavNode *pNode = m_pNavNetwork->NearestValidNode(poly, EXVector3(p.x, p.y, p.z), 32000.0f, mask);
        if (!pNode) {
            out = p;
            if (pPoly) *pPoly = -1;
        } else {
            ClampPointToPoly(pNode->m_wNodeID, p, out);
            if (pPoly) *pPoly = pNode->m_wNodeID;
        }
    }
    return 0;
}

// Edge of poly that the x/z segment a-b leaves through (other than towards node 'from'): edge index,
// neighbour node, edge end points and the crossing point (height interpolated along the edge).
s8 XGeoNavMeshEntity::LineExitsPoly(const EXVector &a, const EXVector &b, int poly, int &node, int &edge, EXVector &e0,
                                    EXVector &e1, EXVector &hit, int from)
{
    if (poly < 0) return 0;
    EXVector d = a - b;
    if (d.LengthXZ() == 0.0f) return 0;
    int n = Poly(poly).m_wNumVerts;
    for (int i = 0; i < n; i++) {
        int j = i + 1;
        if (j >= n) j = 0;
        EXVector3 &vi = PolyVertexPos(poly, i);
        e0.x = vi.x;
        e0.y = vi.y;
        e0.z = vi.z;
        e0.w = 0.0f;
        EXVector3 &vj = PolyVertexPos(poly, j);
        e1.x = vj.x;
        e1.y = vj.y;
        e1.z = vj.z;
        e1.w = 0.0f;
        if (XNavNetwork::LineSegsIntersect2D(a, b, e0, e1, &hit)) {
            edge = i;
            node = m_pNavNetwork->ConnectedNode(poly, i, 0);
            if (node < 0 || node != from) {
                float t;
                if (fabs(e1.x - e0.x) <= fabs(e1.z - e0.z))
                    t = (hit.z - e0.z) / (e1.z - e0.z);
                else
                    t = (hit.x - e0.x) / (e1.x - e0.x);
                hit.y = (e1.y - e0.y) * t + e0.y;
                return 1;
            }
        }
    }
    return 0;
}

// As LineExitsPoly, ignoring one edge (the one entered through) instead of a neighbour node.
s8 XGeoNavMeshEntity::LineExitsPolyIE(const EXVector &a, const EXVector &b, int poly, int &node, int &edge, EXVector &e0,
                                      EXVector &e1, EXVector &hit, int ignoreEdge)
{
    if (poly < 0) return 0;
    EXVector d = a - b;
    if (d.LengthXZ() == 0.0f) return 0;
    int n = Poly(poly).m_wNumVerts;
    for (int i = 0; i < n; i++) {
        int j = i + 1;
        if (j >= n) j = 0;
        if (i == ignoreEdge) continue;
        EXVector3 &vi = PolyVertexPos(poly, i);
        e0.x = vi.x;
        e0.y = vi.y;
        e0.z = vi.z;
        e0.w = 0.0f;
        EXVector3 &vj = PolyVertexPos(poly, j);
        e1.x = vj.x;
        e1.y = vj.y;
        e1.z = vj.z;
        e1.w = 0.0f;
        if (XNavNetwork::LineSegsIntersect2D(a, b, e0, e1, &hit)) {
            edge = i;
            node = m_pNavNetwork->ConnectedNode(poly, i, 0);
            float t;
            if (fabs(e1.x - e0.x) <= fabs(e1.z - e0.z))
                t = (hit.z - e0.z) / (e1.z - e0.z);
            else
                t = (hit.x - e0.x) / (e1.x - e0.x);
            hit.y = (e1.y - e0.y) * t + e0.y;
            return 1;
        }
    }
    return 0;
}

// Edge of poly crossed by the segment a-b (x/z), its neighbour node and end points.
s8 XGeoNavMeshEntity::LineExitsPoly3D(const EXVector &a, const EXVector &b, int poly, int &node, EXVector &e0,
                                      EXVector &e1, EXVector &hit)
{
    int n = Poly(poly).m_wNumVerts;
    for (int i = 0; i < n; i++) {
        int j = i + 1;
        if (j >= n) j = 0;
        EXVector3 &vi = PolyVertexPos(poly, i);
        e0.x = vi.x;
        e0.y = vi.y;
        e0.z = vi.z;
        e0.w = 0.0f;
        EXVector3 &vj = PolyVertexPos(poly, j);
        e1.x = vj.x;
        e1.y = vj.y;
        e1.z = vj.z;
        e1.w = 0.0f;
        if (XNavNetwork::LineSegsIntersect2D(a, b, e0, e1, &hit)) {
            node = m_pNavNetwork->ConnectedNode(poly, i, 0);
            return 1;
        }
    }
    return 0;
}

// Walks the segment from a towards b across the mesh. Returns 1 when b is reached; otherwise out
// gets the point where the walk left the mesh (or hit a blocked node, type flag 8) at a's height.
s8 XGeoNavMeshEntity::ValidateLine(const EXVector &a, const EXVector &b, int poly, EXVector &out, int &outPoly)
{
    out = b;
    if (!LocatePolygon(a, &poly, 0, 0)) {
        out = a;
        outPoly = poly;
        return 0;
    }
    EXVector from = a;
    int steps = 0;
    int node = -1;
    int edge;
    EXVector e0, e1, hit;
    s8 bExit = LineExitsPoly(from, b, poly, node, edge, e0, e1, hit, -1);
    while (bExit && steps++ <= 10000) {
        if (node < 0 || (m_pNavNetwork->NavSection().Node(node)->m_wTypeFlags & 8)) {
            outPoly = poly;
            out = hit;
            out.y = a.y;
            return 0;
        }
        from = hit;
        int prev = poly;
        poly = node;
        bExit = LineExitsPoly(from, b, poly, node, edge, e0, e1, hit, prev);
    }
    outPoly = poly;
    out = b;
    return 1;
}

#define NAV_ASSERT(c) \
    if (!(c)) while (1) {}

static __forceinline s8 Nav_IsFinite(float f) { return fabs(f - f) < 0.001f; }

// Sets the mesh up after loading: converts version 0x16 data (copy in pData), checks the header,
// creates the nav network, builds the polygon planes (0x16), zones and node radii, and (3D meshes)
// the bounds sections.
s8 XGeoNavMeshEntity::InitLoadedData(XNavMeshData *pData)
{
    if (m_dwVersion == 0x16 && pData) {
        u32 *d = pData->m_Data;
        m_dwVersion = d[0];
        m_dwHashCode = d[0xd];
        *(u32 *)((u8 *)this + 0x9c) = d[0xe];
        m_vBoundsMin.x = *(float *)&d[0xf];
        m_vBoundsMin.y = *(float *)&d[0x10];
        m_vBoundsMin.z = *(float *)&d[0x11];
        m_vBoundsMin.w = 0.0f;
        m_vBoundsMid.x = *(float *)&d[0x13];
        m_vBoundsMid.y = *(float *)&d[0x14];
        m_vBoundsMid.z = *(float *)&d[0x15];
        m_vBoundsMid.w = 0.0f;
        m_vBoundsMax.x = *(float *)&d[0x17];
        m_vBoundsMax.y = *(float *)&d[0x18];
        m_vBoundsMax.z = *(float *)&d[0x19];
        m_vBoundsMax.w = 0.0f;
        *(u32 *)&m_Polys = d[1];
        *(u32 *)&m_Verts = d[2];
        *(u32 *)&m_Nodes = d[3];
        *(u32 *)&m_Links = d[4];
        *(u32 *)&m_NavData = d[5];
        *(u32 *)&m_NodeData = d[6];
        *(u32 *)&m_RangeData = d[7];
        *(u32 *)&m_SubMapData = d[8];
        *(u32 *)&m_LinkInfo = d[9];
        *(u32 *)&m_NodeInfo = d[10];
        m_dwPolyCount = d[0x1b];
        m_dwVertCount = d[0x1c];
        m_d8 = d[0x1d];
        m_dwLinkInfoCount = d[0x1e];
        m_dwNodeInfoCount = d[0x1f];
        m_pNavNetwork = 0;
        m_pSector = 0;
        m_f8 = 0;
        m_pPolyBounds = 0;
        m_pBoundsSections = 0;
        m_pZones = 0;
        u16 *w = (u16 *)d;
        m_wID = w[0x4a];
        m_wZones = w[0x4b];
        m_wNodes = w[0x4c];
        m_wLinks = w[0x4d];
        m_wRanges = w[0x4e];
        m_wSubMaps = 0;
        m_w11c = 0;
        m_wNodeOffset = 0;
        m_wLinkOffset = 0;
        m_wPolyOffset = 0;
        m_wVertOffset = 0;
        m_w126 = w[0x55];
        m_w128 = w[0x56];
        m_wGridSector = w[0x57];
        m_wBoundsSections = 0;
        NAV_ASSERT(d[0x5c] == 0x13371337);
    }
    NAV_ASSERT(m_dwVersion == 0x18 || m_dwVersion == 0x17);
    NAV_ASSERT(Nav_IsFinite(m_vBoundsMin.x) && Nav_IsFinite(m_vBoundsMin.y) && Nav_IsFinite(m_vBoundsMin.z)
               && Nav_IsFinite(m_vBoundsMin.w));
    NAV_ASSERT(Nav_IsFinite(m_vBoundsMid.x) && Nav_IsFinite(m_vBoundsMid.y) && Nav_IsFinite(m_vBoundsMid.z)
               && Nav_IsFinite(m_vBoundsMid.w));
    NAV_ASSERT(Nav_IsFinite(m_vBoundsMax.x) && Nav_IsFinite(m_vBoundsMax.y) && Nav_IsFinite(m_vBoundsMax.z)
               && Nav_IsFinite(m_vBoundsMax.w));
    NAV_ASSERT(!m_pPolyBoundsAlloc);
    NAV_ASSERT(!m_pBoundsSectionsAlloc);
    NAV_ASSERT(!m_pZonesAlloc);
    NAV_ASSERT(m_dwPolyCount);
    NAV_ASSERT(m_dwVertCount);
    NAV_ASSERT(m_d8 < 0x7d000);
    NAV_ASSERT(!m_pNavNetwork);
    NAV_ASSERT(!m_pSector);
    NAV_ASSERT(!m_f8);
    NAV_ASSERT(!m_wID);
    NAV_ASSERT(m_wZones < 500);
    NAV_ASSERT(m_wNodes == m_dwPolyCount);
    NAV_ASSERT(m_wLinks < 10000);
    NAV_ASSERT(!m_w126);
    NAV_ASSERT(!m_w128);
    NAV_ASSERT(!m_wGridSector);
    NAV_ASSERT(!((u32)this & 0xf));
    NAV_ASSERT(m_1c4 == 0x13371337);
    m_pNavNetwork = new XNavNetwork(0);
    NAV_ASSERT(m_pNavNetwork);
    u32 navFlags = XNavNetwork::GetNavDataFlags(*(u32 *)((u8 *)this + 0x9c));
    m_pNavNetwork->SetNavDataFlags(navFlags);
    m_pNavNetwork->SetNavDataFileFlags(navFlags);
    EXGeoNavMeshEntity_PolyList *pPolys = m_Polys;
    XNavNode *pNodes = m_Nodes;
    XNavLink *pLinks = m_Links;
    u8 *pNavData = m_NavData;
    XNavNodeData *pNodeData = m_NodeData;
    XNavRangeData *pRangeData = m_RangeData;
    if (m_dwVersion > 0x16) {
        m_pPolyBounds = m_PolyBounds;
        m_pZones = m_Zones;
    }
    if (m_dwVersion == 0x16) {
        _XNavPolyBounds *pBounds = new _XNavPolyBounds[m_dwPolyCount];
        m_pPolyBoundsAlloc = pBounds;
        m_pPolyBounds = pBounds;
        float *pOld = (float *)(_XNavPolyBounds *)m_PolyBounds;
        EXVector v[16];
        for (int i = 0; i < (int)m_dwPolyCount; i++, pPolys++, pBounds++) {
            NAV_ASSERT(pPolys->m_wNumVerts <= 16);
            int n = 0;
            PolyVerts(i, v, &n);
            int k;
            int i1 = 1, i2 = 2;
            for (k = 0; k < n; k++, i1++, i2++) {
                if (i1 >= n) i1 = 0;
                if (i2 >= n) i2 = 0;
                EXVector e1 = v[k] - v[i1];
                EXVector e2 = v[i1] - v[i2];
                EXVector u1 = e1;
                u1.Normalise();
                EXVector u2 = e2;
                u2.Normalise();
                if (u2.x * u1.x + u2.w * u1.w + u2.z * u1.z + u2.y * u1.y < 0.9998f) break;
            }
            EXPlane pl;
            if (k == n)
                Nav_PlaneFrom3(pl, v[0], v[1], v[2]);
            else
                Nav_PlaneFrom3(pl, v[k], v[i1], v[i2]);
            pBounds->m_MinX = pOld[0];
            pBounds->m_MinZ = pOld[1];
            pBounds->m_MaxX = pOld[2];
            pBounds->m_MaxZ = pOld[3];
            pOld += 4;
            pBounds->m_Plane[0] = pl.A;
            pBounds->m_Plane[1] = pl.B;
            pBounds->m_Plane[2] = pl.C;
            pBounds->m_Plane[3] = pl.D;
        }
    }
    XNavStats stats;
    memset(&stats, 0, sizeof(stats));
    stats.m_10 = m_vBoundsMin;
    stats.m_30 = m_vBoundsMid;
    stats.m_20 = m_vBoundsMax;
    m_pNavNetwork->SetNumZones(m_wZones);
    if (!m_pNavNetwork->Loaded(m_dwHashCode, m_wNodes, pNodes, m_wLinks, pLinks, pNavData, &stats, m_dwVersion, 0,
                               pNodeData, m_wRanges, pRangeData))
        while (1) {}
    if (!m_bDynamic) {
        for (int i = 0; i < (int)m_dwPolyCount; i++) {
            u16 zone = Poly(i).m_wZone & 0xfff;
            m_pNavNetwork->NavSection().Node(i)->m_wZone = zone;
            Poly(i).m_wZone = zone;
        }
    } else {
        if (m_pZonesAlloc) {
            EXFree(m_pZonesAlloc);
            m_pZonesAlloc = 0;
        }
        m_pNavNetwork->BuildZones();
        int nZones = m_pNavNetwork->NumZones();
        m_wZones = nZones;
        _XNavZone *pZones = new _XNavZone[nZones];
        m_pZones = pZones;
        m_pZonesAlloc = pZones;
        int z;
        for (z = 0; z < nZones; z++) {
            m_pZones[z].m_Min.x = 10000000.0f;
            m_pZones[z].m_Min.y = 10000000.0f;
            m_pZones[z].m_Min.z = 10000000.0f;
            m_pZones[z].m_Max.x = -10000000.0f;
            m_pZones[z].m_Max.y = -10000000.0f;
            m_pZones[z].m_Max.z = -10000000.0f;
            m_pZones[z].m_wPolys = 0;
            m_pZones[z].m_w32 = 0;
        }
        for (int i = 0; i < m_pNavNetwork->NodeCount(); i++) {
            EXGeoNavMeshEntity_PolyList &pl = Poly(i);
            XNavNode *pNode = m_pNavNetwork->NavSection().Node(i);
            pNode->m_wNumEdges = pl.m_wNumVerts;
            u8 usage[16];
            pNode->GetEdgeUsage(usage);
            u16 zone = pNode->m_wZone;
            int xEdges = 0;
            float maxSq = -1000.0f;
            int far = -1;
            if (zone == 0xffff) {
                pNode->m_wZone = 0;
                zone = 0;
            }
            _XNavZone &zn = m_pZones[zone];
            zn.m_wPolys++;
            pl.m_wZone = zone;
            for (int k = 0; k < pNode->m_wNumEdges; k++) {
                if (!usage[k]) xEdges++;
                EXVector3 &v = VertexPos(pl.m_wVerts[k]);
                float x = v.x, y = v.y, z2 = v.z;
                zn.m_Min.x = x > zn.m_Min.x ? zn.m_Min.x : x;
                zn.m_Min.y = y > zn.m_Min.y ? zn.m_Min.y : y;
                zn.m_Min.z = z2 > zn.m_Min.z ? zn.m_Min.z : z2;
                zn.m_Max.x = x < zn.m_Max.x ? zn.m_Max.x : x;
                zn.m_Max.y = y < zn.m_Max.y ? zn.m_Max.y : y;
                zn.m_Max.z = z2 < zn.m_Max.z ? zn.m_Max.z : z2;
                float dx = x - pNode->m_Pos.x;
                float dz = z2 - pNode->m_Pos.z;
                float dsq = dz * dz + dx * dx;
                if (far < 0 || maxSq < dsq) {
                    far = k;
                    maxSq = dsq;
                }
            }
            if (maxSq <= 0.0f) maxSq = 1.0f;
            pNode->m_fRadius = (float)sqrt(maxSq);
            pNode->m_wXEdges = xEdges;
        }
        for (z = 0; z < nZones; z++) {
            _XNavZone &zn = m_pZones[z];
            zn.m_Pos.x = (zn.m_Min.x + zn.m_Max.x) * 0.5f;
            zn.m_Pos.y = (zn.m_Min.y + zn.m_Max.y) * 0.5f;
            zn.m_Pos.z = (zn.m_Min.z + zn.m_Max.z) * 0.5f;
            float dz = zn.m_Max.z - zn.m_Pos.z;
            float dx = zn.m_Max.x - zn.m_Pos.x;
            zn.m_Radius = EXSqrt(dx * dx + dz * dz);
        }
    }
    gNavMeshHandler.AddZones(m_pNavNetwork->NumZones());
    m_pBoundsSections = 0;
    if (m_b3D) {
        if (m_dwVersion == 0x16) {
            int nSections = 0;
            _XNavBoundsSection *pSections = 0;
            m_pNavNetwork->ClearNodeTypeFlags(0x2000);
            int size = 0;
            for (int i = 0; i < (int)m_dwPolyCount; i++) {
                XNavNode *pNode = m_pNavNetwork->NavSection().Node(i);
                if (pNode->m_wTypeFlags & 0x2000) continue;
                m_pNavNetwork->UnselectAll();
                int nSel = m_pNavNetwork->SelectFlagged(i, 0x8000);
                if (nSel <= 0) continue;
                nSections++;
                size += sizeof(_XNavBoundsSection);
                if (!pSections)
                    pSections = new _XNavBoundsSection[nSections];
                else
                    pSections = (_XNavBoundsSection *)_EXRealloc(pSections, size);
                _XNavBoundsSection &sec = pSections[nSections - 1];
                u16 *pList = (u16 *)_EXAlloc(nSel * 2, 0);
                sec.m_pPolyList = pList;
                EXVector mn(100000.0f, 100000.0f, 100000.0f, 0.0f);
                EXVector mx(-100000.0f, -100000.0f, -100000.0f, 0.0f);
                for (int j = 0; j < (int)m_dwPolyCount; j++) {
                    XNavNode *pn = m_pNavNetwork->NavSection().Node(j);
                    if (!(pn->m_wTypeFlags & 0x4000)) continue;
                    *pList++ = j;
                    pn->m_wTypeFlags |= 0x2000;
                    EXVector v[16];
                    int n;
                    if (PolyVerts(j, v, &n) && n > 0) {
                        for (int k = 0; k < n; k++) {
                            if (v[k].w <= mn.w) mn.w = v[k].w;
                            if (v[k].z <= mn.z) mn.z = v[k].z;
                            if (v[k].y <= mn.y) mn.y = v[k].y;
                            if (v[k].x <= mn.x) mn.x = v[k].x;
                            if (mx.w <= v[k].w) mx.w = v[k].w;
                            if (mx.z <= v[k].z) mx.z = v[k].z;
                            if (mx.y <= v[k].y) mx.y = v[k].y;
                            if (mx.x <= v[k].x) mx.x = v[k].x;
                        }
                    }
                }
                sec.m_wPolyCount = nSel;
                sec.m_wFirstPoly = 0xffff;
                sec.m_Centre = (mx + mn) * 0.5f;
                EXVector r = mx - sec.m_Centre;
                sec.m_Radius = r.Length();
                sec.m_Bounds.m_vctMin.set(mn.x, mn.y, mn.z, 0.0f);
                sec.m_Bounds.m_vctMax.set(mx.x, mx.y, mx.z, 0.0f);
            }
            m_pBoundsSectionsAlloc = pSections;
            m_pBoundsSections = pSections;
            m_wBoundsSections = nSections;
            return 1;
        }
        m_pBoundsSections = m_BoundsSections;
    }
    return 1;
}
