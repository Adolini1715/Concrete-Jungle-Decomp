#include "XNavMesh.h"
#include "XNavLocation.h"
#include "EXMemory.h"
#include "FEText.h"
#include "EXPlane.h"
#include "XNavRoute.h"
#include "XNavNetwork.h"
#include "XNavMeshHandler.h"
#include "XGeoNavMeshEntity.h"

void XNavMesh::Zero()
{
    m_dwHandle = 0;
    m_wMeshCount = 0;
    m_w8 = 0;
    m_wPolyCount = 0;
    memset(&m_Section, 0, sizeof(m_Section));
    m_vMin.Set(3.4028235e38f, 3.4028235e38f, 3.4028235e38f, 0.0f);
    m_vMid.Set(0.0f, 0.0f, 0.0f, 0.0f);
    m_vMax.Set(-3.4028235e38f, -3.4028235e38f, -3.4028235e38f, 0.0f);
    m_b3D = 0;
    m_wNodeInfoCount = 0;
    m_pNodeInfo = 0;
    m_wLinkInfoCount = 0;
    m_pLinkInfo = 0;
    m_wGeomSections = 0;
    m_pGeomSections = 0;
    m_pNetwork = 0;
    m_wNetSections = 0;
    m_pNetSections = 0;
    m_wZoneBase = 0;
}

void XNavMesh::Init()
{
    Zero();
    m_pNetwork = new XNavNetwork(this);
}

void XNavMesh::RefreshNavMesh(XGeoNavMeshEntity *) {}

void XNavMesh::DelNavMesh(XGeoNavMeshEntity *pMesh)
{
    XNavGeomSection *s = m_Section.SectionFromMeshHash(pMesh->HashCode());
    if (s) s->m_pMesh = 0;
    if (m_pNetwork) {
        XNavSection *ns = m_pNetwork->m_Section.SectionFromHash(pMesh->NavNetwork()->m_Section.m_dwHash);
        if (ns) {
            ns->m_pNodes = 0;
            ns->m_pLinks = 0;
        }
    }
}

s8 XNavMesh::UpdatePtrs(XGeoNavMeshEntity *pOld, XGeoNavMeshEntity *pNew)
{
    XNavNetwork *src = pNew->NavNetwork();
    if (m_pNetwork) {
        m_pNetwork->SetNavData(src->NavData());
        m_pNetwork->SetNavNodeData(src->NavNodeData());
        m_pNetwork->SetNavRanges(src->NavRanges());
        m_pNetwork->SetNavRangesCount(src->NavRangesCount());
        m_pNetwork->SetSection(&src->NavSection());
    }
    for (XNavGeomSection *s = &m_Section; s; s = s->m_pNext)
        if (s->m_pMesh == pOld) s->m_pMesh = pNew;
    return 1;
}

s8 XNavMesh::PolyVerts(int poly, EXVector *v, int *n)
{
    XNavGeomSection *s = SectionFromPolyID(poly);
    if (s && s->m_pMesh) return s->m_pMesh->PolyVerts(poly - s->m_wPolyOffset, v, n);
    return 0;
}

s8 XNavMesh::PolyNormal(int poly, EXVector &n)
{
    XNavGeomSection *s = SectionFromPolyID(poly);
    if (s->m_pMesh) return s->m_pMesh->PolyNormal(poly - s->m_wPolyOffset, n);
    return 0;
}

s8 XNavMesh::PolyPlane(int poly, EXPlane &p)
{
    XNavGeomSection *s = SectionFromPolyID(poly);
    if (s->m_pMesh) return s->m_pMesh->PolyPlane(poly - s->m_wPolyOffset, p);
    return 0;
}

float XNavMesh::GetFloorHeight(const EXVector &p)
{
    XNavGeomSection *s = SectionFromPos(p);
    if (s && s->m_pMesh) return s->m_pMesh->GetFloorHeight(p);
    return p.y;
}

float XNavMesh::GetHeight(const EXVector &p, int poly, int tri, EXVector *pNormal)
{
    if (poly < 0) return 0.0f;
    XNavGeomSection *s = SectionFromPolyID(poly);
    if (s && s->m_pMesh) return s->m_pMesh->GetHeight(p, poly - s->m_wPolyOffset, tri, pNormal);
    return p.y;
}

s8 XNavMesh::SnapToPolygon(EXVector &p, int poly)
{
    XNavGeomSection *s = SectionFromPolyID(poly);
    if (!s) return 0;
    if (!s->m_pMesh) return 0;
    return s->m_pMesh->SnapToPolygon(p, poly);
}

s8 XNavMesh::SnapToPolygonPlane(EXVector &p, int poly)
{
    XNavGeomSection *s = SectionFromPolyID(poly);
    if (!s) return 0;
    if (!s->m_pMesh) return 0;
    return s->m_pMesh->SnapToPolygonPlane(p, poly);
}

s8 XNavMesh::SnapToPolygonBounds(EXVector &p, int poly)
{
    XNavGeomSection *s = SectionFromPolyID(poly);
    if (!s) return 0;
    if (!s->m_pMesh) return 0;
    return s->m_pMesh->SnapToPolygonBounds(p, poly);
}

EXVector XNavMesh::RandomPosition(u16 zone)
{
    if (m_pNetwork) return m_pNetwork->RandomPosition(zone);
    return gZeroVector;
}

void XNavMesh::SetMeshLighting() { m_Section.SetMeshLighting(); }

s8 XNavMesh::ValidateMesh() { return 1; }

s8 XNavMesh::ClampPointToPoly(int poly, const EXVector &p, EXVector &out)
{
    XGeoNavMeshEntity *m = SectionFromPolyID(poly)->m_pMesh;
    if (!m) return 0;
    return m->ClampPointToPoly(poly - m->PolyOffset(), p, out);
}

void XNavMesh::FloodPolyFlags(u16 a, u16 b, u16 c, u16 d) { m_pNetwork->FloodNodeFlags(a, b, c, d); }

XNavMesh::XNavMesh() { Zero(); }

void XNavMesh::CleanUp()
{
    if (m_pGeomSections) {
        EXFree(m_pGeomSections);
        m_pGeomSections = 0;
    }
    if (m_pNetwork) {
        delete m_pNetwork;
        m_pNetwork = 0;
    }
    Zero();
}

s8 XNavMesh::LocatePolygon3D(const EXVector &p, int *pPoly, int *pTri, u32 flags)
{
    XNavGeomSection *s = SectionNearestPos(p);
    if (!s->Mesh()) return 0;
    int poly = -1;
    if (pPoly) {
        poly = *pPoly;
        if (poly >= 0) poly -= s->m_wPolyOffset;
    }
    s8 r = s->Mesh()->LocatePolygon3D(p, &poly, pTri, flags);
    if (pPoly) {
        *pPoly = poly;
        if (poly >= 0) *pPoly = poly + s->m_wPolyOffset;
    }
    return r;
}

s8 XNavMesh::LocatePolygon3DRay(const EXVector &p, const EXVector &d, int *pPoly, u32 flags, float *pDist, EXVector *pOut)
{
    XNavGeomSection *s = SectionNearestPos(p);
    if (!s->m_pMesh) return 0;
    int poly = -1;
    if (pPoly) {
        poly = *pPoly;
        if (poly >= 0) poly -= s->m_wPolyOffset;
    }
    s8 r = s->Mesh()->LocatePolygon3DRay(p, d, &poly, flags, pDist, pOut);
    if (pPoly) {
        *pPoly = poly;
        if (poly >= 0) *pPoly = poly + s->m_wPolyOffset;
    }
    return r;
}

int XNavMesh::NearestPolygon(const EXVector &p, EXVector *pOut, u32 flags)
{
    XNavGeomSection *s = SectionNearestPos(p);
    if (!s) return -1;
    if (!s->m_pMesh) return -1;
    int poly;
    if (m_b3D) poly = s->m_pMesh->NearestPolygon3D(p, pOut, flags);
    else poly = s->m_pMesh->NearestPolygon(p, pOut, flags);
    return poly + s->m_wPolyOffset;
}

s8 XNavMesh::PosIsValid(const EXVector &p, int &poly)
{
    if (m_b3D) return PointInPolygon3D(p, poly);
    return PointInPolygon(p, poly);
}

s8 XNavMesh::ValidateLine(XNavLocation &loc, const EXVector &to, int &poly, EXVector &out, u32 flags)
{
    return ValidateLine(loc.Pos(), to, loc.GetPolyID(), out, poly, flags);
}

s8 XNavMesh::ValidateLine(XNavLocation &from, XNavLocation &to, XNavLocation &, u32 flags)
{
    int poly = to.GetPolyID();
    EXVector v;
    s8 r = ValidateLine(from.Pos(), to.Pos(), from.GetPolyID(), v, poly, flags);
    to.m_vPos = v;
    to.SetPoly(from.GetMesh(), poly, 0);
    return r;
}

XNavMesh::~XNavMesh() { CleanUp(); }

s8 XNavMesh::LocatePolygon(const EXVector &p, int *pPoly, int *pTri, u32 flags)
{
    if (m_b3D) return LocatePolygon3D(p, pPoly, pTri, flags);
    XNavGeomSection *s = SectionNearestPos(p);
    if (!s) return 0;
    if (!s->m_pMesh) return 0;
    int poly = -1;
    if (pPoly) {
        poly = *pPoly;
        if (poly >= 0) poly -= s->m_wPolyOffset;
    }
    s8 r = s->Mesh()->LocatePolygon(p, &poly, pTri, flags);
    if (pPoly) {
        *pPoly = poly;
        if (poly >= 0) *pPoly = poly + s->m_wPolyOffset;
    }
    return r;
}

s8 XNavMesh::ValidatePos(const EXVector &p, u32 flags, EXVector &out, int *pPoly)
{
    int poly = -1;
    if (pPoly) poly = *pPoly;
    if (LocatePolygon(p, &poly, 0, 0) && poly >= 0 && !(m_pNetwork->Node((u16)poly)->m_wTypeFlags & flags)) {
        if (pPoly) *pPoly = poly;
        out = p;
        return 1;
    }
    int np = NearestPolygon(p, &out, flags);
    if (np < 0) {
        out = p;
        return 0;
    }
    if (!(m_pNetwork->Node((u16)np)->m_wTypeFlags & flags)) {
        if (pPoly) *pPoly = np;
        return 0;
    }
    EXVector3 v(p.x, p.y, p.z);
    XNavNode *n = m_pNetwork->NearestValidNode((u16)np, v, 32000.0f, flags);
    if (!n) {
        out = p;
        if (pPoly) *pPoly = -1;
        return 0;
    }
    ClampPointToPoly(n->m_wNodeID, p, out);
    if (pPoly) *pPoly = n->m_wNodeID;
    return 0;
}

s8 XNavMesh::ValidatePos(const EXVector &p, EXVector &out, int *pPoly) { return ValidatePos(p, 0, out, pPoly); }

// Finds the edge of poly that the 2D segment a-b leaves through (skipping an edge into poly `from`):
// returns the edge index and the poly beyond it, the edge end points and the crossing point, its
// height taken along the edge.
s8 XNavMesh::LineExitsPoly(const EXVector &a, const EXVector &b, int poly, int &next, int &edge, EXVector &e0,
                           EXVector &e1, EXVector &hit, int from)
{
    if (poly < 0) return 0;
    EXVector d = a - b;
    float sq = d.z * d.z + d.x * d.x;
    if (sq < 0.0f) return 0;
    if ((float)sqrt(sq) == 0.0f) return 0;
    EXVector dd = d;
    XNavMesh *pThis = this;
    EXVector v[16];
    int n = 0;
    if (!PolyVerts(poly, v, &n)) return 0;
    for (int i = 0; i < n; i++) {
        int j = i + 1;
        if (j >= n) j = 0;
        e0 = v[i];
        e1 = v[j];
        if (XNavNetwork::LineSegsIntersect2D(a, b, e0, e1, &hit)) {
            edge = i;
            next = pThis->m_pNetwork->ConnectedNode(poly, (u16)i, 0);
            if (next < 0 || next != from) {
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

// As LineExitsPoly, but skips the edge the line came in by (inEdge) instead of a neighbouring poly.
s8 XNavMesh::LineExitsPolyIE(const EXVector &a, const EXVector &b, int poly, int &next, int &edge, EXVector &e0,
                             EXVector &e1, EXVector &hit, int inEdge)
{
    if (poly < 0) return 0;
    EXVector d = a - b;
    float sq = d.z * d.z + d.x * d.x;
    if (sq < 0.0f) return 0;
    if ((float)sqrt(sq) == 0.0f) return 0;
    EXVector dd = d;
    XNavMesh *pThis = this;
    EXVector v[16];
    int n = 0;
    if (!PolyVerts(poly, v, &n)) return 0;
    for (int i = 0; i < n; i++) {
        int j = i + 1;
        if (j >= n) j = 0;
        if (i == inEdge) continue;
        e0 = v[i];
        e1 = v[j];
        if (XNavNetwork::LineSegsIntersect2D(a, b, e0, e1, &hit)) {
            edge = i;
            next = pThis->m_pNetwork->ConnectedNode(poly, (u16)i, 0);
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

// d flattened onto the ground plane and normalised (left as is when too short).
static __forceinline EXVector Nav_FlatUnit(const EXVector &d)
{
    EXVector v(d.x, 0.0f, d.z, 0.0f);
    float sq = v.z * v.z + v.x * v.x;
    if (sq >= 0.0f && (float)sqrt(sq) > EX_EPSILON_UNIT) {
        float inv = 1.0f / (float)sqrt(sq);
        v.x *= inv;
        v.y *= inv;
        v.z *= inv;
        v.w *= inv;
    }
    return v;
}

// Moves p a millimetre towards the node centre when it sits on corner v (so a line that starts or
// ends exactly on a vertex still crosses one edge cleanly).
static __forceinline void Nav_NudgeOffCorner(EXVector &p, const EXVector3 &centre, const EXVector &v)
{
    if (fabs(p.x - v.x) < 0.0001f && fabs(p.z - v.z) < 0.0001f) {
        EXVector d(centre.x - v.x, centre.y - v.y, centre.z - v.z, 0.0f - v.w);
        EXVector n = Nav_FlatUnit(d) * 0.001f;
        p += n;
    }
}

// As LineExitsPolyIE, for a line inside node pNode's poly: end points lying on a poly corner are
// first nudged inwards, and a crossing only counts when a (or else b) is on the inside of the edge.
s8 XNavMesh::LineExitsPolyIE2(EXVector &a, EXVector &b, XNavNode *pNode, int poly, int &next, int &edge,
                              EXVector &e0, EXVector &e1, EXVector &hit, int inEdge)
{
    if (poly < 0) return 0;
    EXVector d = a - b;
    float sq = d.z * d.z + d.x * d.x;
    if (sq < 0.0f) return 0;
    if ((float)sqrt(sq) == 0.0f) return 0;
    XNavMesh *pThis = this;
    EXVector v[16];
    int n = 0;
    if (!PolyVerts(poly, v, &n)) return 0;
    for (int i = 0; i < n; i++) {
        int j = i + 1;
        if (j >= n) j = 0;
        if (i == inEdge) continue;
        e0 = v[i];
        e1 = v[j];
        Nav_NudgeOffCorner(a, pNode->Pos(), e0);
        Nav_NudgeOffCorner(a, pNode->Pos(), e1);
        Nav_NudgeOffCorner(b, pNode->Pos(), e0);
        Nav_NudgeOffCorner(b, pNode->Pos(), e1);
        if (!XNavNetwork::LineSegsIntersect2D(a, b, e0, e1, &hit)) continue;
        EXVector ed = Nav_FlatUnit(e1 - e0);
        EXVector perp(0.0f - ed.z, ed.y, ed.x, ed.w);
        float c = Nav_FlatUnit(a - e0).Dot(perp);
        if (!(c > 0.0f || c != 0.0f)) {
            c = Nav_FlatUnit(b - e0).Dot(perp);
            if (c == 0.0f || c < 0.0f) continue;
        }
        edge = i;
        next = pThis->m_pNetwork->ConnectedNode(poly, (u16)i, 0);
        float t;
        if (fabs(e1.x - e0.x) <= fabs(e1.z - e0.z))
            t = (hit.z - e0.z) / (e1.z - e0.z);
        else
            t = (hit.x - e0.x) / (e1.x - e0.x);
        hit.y = (e1.y - e0.y) * t + e0.y;
        return 1;
    }
    return 0;
}

// As LineExitsPolyIE2 without the corner nudging, for a line whose end b has left the poly: collects
// up to three crossings (the tables hold two; a third overruns them, as in retail) and returns the
// first that leads into another poly, else the first one. With no crossing, b is moved a little
// towards the node centre and snapped back into the poly.
s8 XNavMesh::LineExitsPolyIE3(EXVector &a, EXVector &b, XNavNode *pNode, int poly, int &next, int &edge,
                              EXVector &e0, EXVector &e1, EXVector &hit, int inEdge)
{
    int nHits = 0;
    if (poly < 0) return 0;
    EXVector d = a - b;
    float sq = d.z * d.z + d.x * d.x;
    if (sq < 0.0f) return 0;
    if ((float)sqrt(sq) == 0.0f) return 0;
    EXVector dd = d;
    XNavMesh *pThis = this;
    if (PointInPolygon(b, poly)) return 0;
    EXVector v[16];
    int n = 0;
    if (!PolyVerts(poly, v, &n)) return 0;
    int edges[2];
    float tol[2];
    int nexts[2];
    EXVector hits[2];
    EXVector e0s[2];
    EXVector e1s[2];
    for (int i = 0; i < n; i++) {
        int j = i + 1;
        if (j >= n) j = 0;
        if (i == inEdge) continue;
        e0 = v[i];
        e1 = v[j];
        float ta, tb;
        if (!XNavNetwork::LineSegsIntersect2D2(a, b, e0, e1, &hit, &ta, &tb)) continue;
        EXVector ed = Nav_FlatUnit(e1 - e0);
        EXVector perp(0.0f - ed.z, ed.y, ed.x, ed.w);
        float c = Nav_FlatUnit(a - e0).Dot(perp);
        if (!(c > 0.0f || c != 0.0f)) {
            c = Nav_FlatUnit(b - e0).Dot(perp);
            if (c == 0.0f || c < 0.0f) continue;
        }
        if (nHits >= 3) break;
        edge = i;
        nexts[nHits] = pThis->m_pNetwork->ConnectedNode(poly, (u16)i, 0);
        hits[nHits] = hit;
        float t;
        if (fabs(e1.x - e0.x) <= fabs(e1.z - e0.z))
            t = (hits[nHits].z - e0.z) / (e1.z - e0.z);
        else
            t = (hits[nHits].x - e0.x) / (e1.x - e0.x);
        e0s[nHits] = e0;
        e1s[nHits] = e1;
        tol[nHits] = 0.0001f;
        edges[nHits] = edge;
        hits[nHits].y = (e1.y - e0.y) * t + e0.y;
        nHits++;
    }
    if (nHits) {
        for (int k = 0; k < nHits; k++) {
            if (nexts[k] >= 0) {
                next = nexts[k];
                hit = hits[k];
                e0 = e0s[k];
                e1 = e1s[k];
                edge = edges[k];
                if (next == 0x44) next = 0x44;
                return 1;
            }
        }
        next = nexts[0];
        hit = hits[0];
        e0 = e0s[0];
        e1 = e1s[0];
        edge = edges[0];
        if (next == 0x44) next = 0x44;
        return 1;
    }
    XNavNode *pCentre = pThis->m_pNetwork->NavSection().Node((u16)poly);
    EXVector c(pCentre->Pos().x - b.x, pCentre->Pos().y - b.y, pCentre->Pos().z - b.z, 0.0f - b.w);
    b += Nav_FlatUnit(c) * 0.0001f;
    if (!PointInPolygon(b, poly)) SnapToPolygon(b, poly);
    return 0;
}

// The plane through a, b and c (EXPlane's three point constructor, inlined in retail).
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

// 3D form of LineExitsPoly for sloped polys: each edge is a wall plane standing along the poly
// normal; the segment a-b leaves through the first wall it crosses from the inside, the crossing
// snapped onto the poly plane and clamped to the edge (just past an end counts as that corner).
s8 XNavMesh::LineExitsPoly3D(const EXVector &a, const EXVector &b, int poly, int &next, int &edge, EXVector &e0,
                             EXVector &e1, EXVector &hit)
{
    EXVector v[16];
    int n = 0;
    if (poly < 0 || !PolyVerts(poly, v, &n)) return 0;
    float t = 0.0f;
    EXVector start(a.x, a.y, a.z, 1.0f);
    EXVector dir = b - a;
    EXPlane polyPlane;
    PolyPlane(poly, polyPlane);
    for (int i = 0; i < n; i++) {
        int j = i + 1;
        if (j >= n) j = 0;
        e0 = v[i];
        e1 = v[j];
        EXPlane pl;
        Nav_PlaneFrom3(pl, e0, e1, e1 - polyPlane.Normal());
        float denom = Nav_PlaneDot(pl, EXVector(dir.x, dir.y, dir.z, 0.0f));
        if ((float)fabs(denom) < 1e-10f) continue;
        t = 0.0f - (pl.A * start.x + pl.C * start.z + pl.B * start.y + pl.D) / denom;
        if (t < 0.0f || t > 1.0f) continue;
        if (Nav_PlaneDot(pl, start) < 0.0f) continue;
        hit = (b - a) * t + a;
        hit.w = 0.0f;
        SnapToPolygonPlane(hit, poly);
        hit = XNavNetwork::NearestPointOnLineSegment(e0, e1, hit, &t);
        if (t < 0.0f || t > 1.0f) {
            if (t < -1e-6f || t > 1.000001f) continue;
            if (t >= 0.0f) hit = e1;
            else hit = e0;
        }
        next = m_pNetwork->ConnectedNode(poly, (u16)i, 0);
        edge = i;
        return 1;
    }
    next = -1;
    edge = -1;
    return 0;
}

// Walks the 2D segment from-to across the mesh starting in poly. Returns 1 with out = to when it
// gets there (or gives up after 10000 polys); returns 0 with out just short of the blocking edge
// (at from's height) when the line leaves the mesh or enters a poly with any of the flags (or 8),
// and out = from when from is not in poly. outPoly is the last poly reached.
s8 XNavMesh::ValidateLine(const EXVector &from, const EXVector &to, int poly, EXVector &out, int &outPoly, u32 flags)
{
    out = to;
    s8 inside;
    if (m_b3D)
        inside = PointInPolygon3D(from, poly);
    else
        inside = PointInPolygon(from, poly);
    if (!inside) {
        out = from;
        outPoly = poly;
        return 0;
    }
    EXVector cur = from;
    int iter = 0;
    int next = -1;
    int edge;
    EXVector e0, e1, hit;
    s8 ok = LineExitsPoly(cur, to, poly, next, edge, e0, e1, hit, -1);
    for (;;) {
        if (!ok || iter > 10000) {
            outPoly = poly;
            out = to;
            return 1;
        }
        if (next < 0) {
            XNavNode *pNode = m_pNetwork->NavSection().Node((u16)poly);
            EXVector d(hit.x - pNode->Pos().x, hit.y - pNode->Pos().y, hit.z - pNode->Pos().z, hit.w);
            float len = d.Length();
            if (len > EX_EPSILON_UNIT) d *= 1.0f / len;
            outPoly = poly;
            out = hit - d * 0.02f;
            out.y = from.y;
            return 0;
        }
        if (((flags | 8) & m_pNetwork->NavSection().Node((u16)next)->Flags()) != 0) break;
        int prev = poly;
        poly = next;
        cur = hit;
        ok = LineExitsPoly(cur, to, poly, next, edge, e0, e1, hit, prev);
        iter++;
    }
    XNavNode *pNode = m_pNetwork->NavSection().Node((u16)poly);
    EXVector d(hit.x - pNode->Pos().x, hit.y - pNode->Pos().y, hit.z - pNode->Pos().z, hit.w);
    outPoly = poly;
    out = hit - d * 0.02f;
    out.y = from.y;
    return 0;
}

// d normalised in 3D (w scaled too), unchanged when too short.
static __forceinline void Nav_Unit3(EXVector &d)
{
    float sq = d.z * d.z + d.y * d.y + d.x * d.x;
    if (sq >= 0.0f) {
        float len = (float)sqrt(sq);
        if (len > EX_EPSILON_UNIT) {
            float inv = 1.0f / len;
            d.x *= inv;
            d.y *= inv;
            d.z *= inv;
            d.w *= inv;
        }
    }
}

// One step of a walk across the mesh (see NavTraverse): if the target is in the current poly the
// walk ends there; otherwise it crosses the exit edge into a passable neighbour (chosen by the
// route callback when there is one) and recurses. A wall is slid along once (target pushed back
// onto the wall's plane, then restored); after that the walk stops just inside it and records the
// block. Returns the poly the walk ends in.
int XNavMesh::TraversePolys(NavTraverse &t)
{
    int cur = t.m_nFromNode;
    if (++t.m_nDepth > 1000) return cur;
    t.m_44 = cur;
    t.m_4c = cur;
    t.m_Start = t.m_Pos;
    t.m_5c = t.m_nFromNode;
    s8 inside;
    if (!m_b3D) {
        SnapToPolygonBounds(t.m_Pos, t.m_nFromNode);
        if (!PointInPolygon(t.m_Pos, t.m_nFromNode)) {
            XNavNode *pN = m_pNetwork->NavSection().Node((u16)t.m_nFromNode);
            t.m_Pos = EXVector(pN->Pos().x, pN->Pos().y, pN->Pos().z, 0.0f);
        }
        inside = PointInPolygon(t.m_Target, t.m_nFromNode);
    } else if (!(t.m_dwFlags & 0x18)) {
        SnapToPolygon(t.m_Pos, t.m_nFromNode);
        SnapToPolygonPlane(t.m_Target, t.m_nFromNode);
        inside = PointInPolygon3D(t.m_Target, t.m_nFromNode);
    } else {
        SnapToPolygonBounds(t.m_Pos, t.m_nFromNode);
        inside = PointInPolygon3D(t.m_Target, t.m_nFromNode);
    }
    if (inside) {
        t.m_Pos = t.m_Target;
        return t.m_nFromNode;
    }
    XNavNode *pNode = m_pNetwork->PolyNode((u16)t.m_nFromNode);
    int edge;
    EXVector e0, e1, hit;
    s8 ok;
    if (!m_b3D)
        ok = LineExitsPolyIE3(t.m_Pos, t.m_Target, pNode, t.m_nFromNode, t.m_4c, edge, e0, e1, hit, -1);
    else
        ok = LineExitsPoly3D(t.m_Pos, t.m_Target, t.m_nFromNode, t.m_4c, edge, e0, e1, hit);
    if (!ok) {
        t.m_Pos = t.m_Target;
        return t.m_nFromNode;
    }
    t.m_4c = -1;
    int nLinks = m_pNetwork->NumConnectedNodes(t.m_nFromNode, (u16)edge);
    for (int k = 0; k < nLinks; k++) {
        int c = m_pNetwork->ConnectedNode(t.m_nFromNode, (u16)edge, (u16)k);
        t.m_nNode = c;
        if (c < 0 || pNode->RouteParent() == m_pNetwork->NavSection().Node((u16)c)) continue;
        if (!t.m_pRoute) {
            if (!(t.m_dwIgnore & t.m_pMesh->m_pNetwork->NavSection().Node((u16)t.m_nNode)->Flags())) t.m_4c = c;
        } else {
            int r = t.m_pRoute->TraversePolyCallback(t);
            if (r == -1) {
                t.m_4c = -1;
                break;
            }
            if (r == 1) {
                t.m_4c = t.m_nNode;
            } else if (r == 2) {
                t.m_4c = t.m_nNode;
                break;
            }
        }
    }
    if (t.m_4c >= 0) {
        XNavNode *pNext = m_pNetwork->NavSection().Node((u16)t.m_4c);
        pNext->SetRouteParent(pNode);
        pNode->SetNext(pNext);
        t.m_nFromNode = t.m_4c;
        if (m_b3D) {
            t.m_Pos = hit;
            XNavNode *pN = m_pNetwork->NavSection().Node((u16)t.m_4c);
            EXVector d(pN->Pos().x - hit.x, pN->Pos().y - hit.y, pN->Pos().z - hit.z, 0.0f);
            Nav_Unit3(d);
            t.m_Pos = d * 0.001f + hit;
        }
        t.m_dwFlags |= 1;
        return TraversePolys(t);
    }
    // A wall: stop just inside it.
    t.m_Pos.x = hit.x;
    if (!m_b3D) {
        t.m_Pos.z = hit.z;
        t.m_Pos.w = 0.0f;
    } else {
        t.m_Pos.y = hit.y;
        t.m_Pos.z = hit.z;
        t.m_Pos.w = hit.w;
    }
    XNavNode *pCur = m_pNetwork->NavSection().Node((u16)t.m_nFromNode);
    EXVector d(pCur->Pos().x - hit.x, pCur->Pos().y - t.m_Pos.y, pCur->Pos().z - hit.z, 0.0f);
    if (!m_b3D) d.y = 0.0f;
    Nav_Unit3(d);
    t.m_Pos += d * 0.001f;
    EXPlane wall;
    if (!m_b3D) {
        Nav_PlaneFrom3(wall, e0, e1, e1 + EXVector(0.0f, 1.0f, 0.0f, 0.0f));
    } else {
        EXVector n;
        PolyNormal(t.m_nFromNode, n);
        Nav_PlaneFrom3(wall, e0, e1, e1 + n);
    }
    t.m_Target.w = 1.0f;
    float tw = t.m_Target.w;
    t.m_Target.w = 0.0f;
    t.m_dwFlags |= 2;
    if (++t.m_nSlides < 2) {
        t.m_SavedTarget = t.m_Target;
        float dist = wall.D * tw + wall.C * t.m_Target.z + wall.B * t.m_Target.y + wall.A * t.m_Target.x + 0.001f;
        t.m_Target = t.m_Target - EXVector(wall.A, wall.B, wall.C, 0.0f) * dist;
        TraversePolys(t);
        t.m_Target = t.m_SavedTarget;
        return TraversePolys(t);
    }
    EXVector b(pCur->Pos().x - hit.x, pCur->Pos().y - hit.y, pCur->Pos().z - hit.z, 0.0f);
    if (!m_b3D) b.y = 0.0f;
    Nav_Unit3(b);
    t.m_nBlockEdge = edge;
    t.m_nBlockPoly = t.m_nFromNode;
    t.m_bBlocked = 1;
    t.m_BlockPos = b * 0.001f + hit;
    t.m_BlockE0 = e0;
    t.m_BlockE1 = e1;
    if (!m_b3D) {
        t.m_Target.x = t.m_BlockPos.x;
        t.m_Target.z = t.m_BlockPos.z;
        t.m_Target.w = 0.0f;
    } else {
        t.m_Target = t.m_BlockPos;
    }
    t.m_Pos = t.m_Target;
    return t.m_nFromNode;
}

// Walks from pos towards target starting in poly (see TraversePolys(NavTraverse &)): poly and
// target become where the walk ended. Returns whether it was stopped by a wall; pFlags (in and
// out) carries the walk flags.
s8 XNavMesh::TraversePolys(const EXVector &pos, int &poly, EXVector &target, u32 ignore, u32 *pFlags, XNavRoute *pRoute)
{
    NavTraverse t;
    t.m_Pos.x = pos.x;
    t.m_Pos.y = pos.y;
    t.m_Pos.z = pos.z;
    t.m_Target.x = target.x;
    t.m_nFromNode = poly;
    t.m_Target.y = target.y;
    t.m_Target.z = target.z;
    t.m_pRoute = pRoute;
    t.m_Target.w = 0.0f;
    t.m_Pos.w = 0.0f;
    t.m_64 = 0;
    t.m_dwIgnore = ignore;
    t.m_nSlides = 0;
    t.m_nDepth = 0;
    t.m_dwFlags = 0;
    if (pFlags) t.m_dwFlags = *pFlags;
    t.m_bBlocked = 0;
    t.m_pMesh = this;
    XNavNode *pNode = m_pNetwork->PolyNode((u16)t.m_nFromNode);
    if (!pNode) return 0;
    pNode->SetNext(0);
    pNode->SetRouteParent(0);
    poly = TraversePolys(t);
    target = t.m_Pos;
    if (pFlags) *pFlags |= t.m_dwFlags;
    return t.m_bBlocked;
}

// Registers a loaded nav mesh entity. nSubMaps > 0 (the master mesh of a streamed map) first builds a
// geometry and a network section per sub map. The network then takes the entity's data: a master
// mesh supplies the node and link info tables; a sub map hooks its nodes and links into its section,
// installs the master links and stamps every node and poly with its zone; a single mesh takes
// everything. Finally the entity is attached to its geometry section (the base section is also
// given the entity's counts and reset bounds).
void XNavMesh::AddNavMesh(XGeoNavMeshEntity *pMesh, int nSubMaps, XNavSubMap *pSubMaps)
{
    m_wMeshCount++;
    m_b3D = pMesh->m_b3D;
    m_bDynamic = pMesh->m_bDynamic;
    if (nSubMaps) {
        if (m_pGeomSections) {
            EXFree(m_pGeomSections);
            m_pGeomSections = 0;
        }
        if (m_pNetSections) {
            EXFree(m_pNetSections);
            m_pNetSections = 0;
        }
        XNavGeomSection *pGeom = new XNavGeomSection[nSubMaps];
        memset(pGeom, 0, nSubMaps * sizeof(XNavGeomSection));
        XNavSection *pNet = (XNavSection *)_EXAlloc(nSubMaps * sizeof(XNavSection), 0);
        memset(pNet, 0, nSubMaps * sizeof(XNavSection));
        m_wGeomSections = (u16)nSubMaps;
        m_pGeomSections = pGeom;
        m_wNetSections = (u16)nSubMaps;
        m_pNetSections = pNet;
        for (int i = 0; i < nSubMaps; i++) {
            XNavSubMap &sub = pSubMaps[i];
            XNavGeomSection &g = pGeom[i];
            g.m_dwHash = sub.HashCode();
            g.m_wPolys = sub.PolyCount();
            g.m_wPolyOffset = sub.PolyOffset();
            g.m_wVerts = sub.VertCount();
            g.m_wVertOffset = sub.VertOffset();
            g.m_vMin = sub.BoundsMin();
            g.m_vMid = sub.BoundsMid();
            g.m_vMax = sub.BoundsMax();
            m_wPolyCount += sub.PolyCount();
            m_w8 += sub.VertCount();
            m_Section.AddSection(&g);
            XNavSection &n = pNet[i];
            n.m_dwHash = sub.HashCode();
            n.m_wNodes = sub.NodeCount();
            n.m_wNodeOffset = sub.NodeOffset();
            n.m_wLinks = sub.LinkCount();
            n.m_wLinkOffset = sub.LinkOffset();
            m_pNetwork->AddSection(&n);
        }
    }
    XNavNetwork *pNet = pMesh->NavNetwork();
    if (pMesh->m_bMaster) {
        m_pNetwork->SetNavData(pNet->NavData());
        m_pNetwork->SetNavNodeData(pNet->NavNodeData());
        m_pNetwork->SetNavRanges(pNet->NavRanges());
        m_pNetwork->SetNavRangesCount(pNet->NavRangesCount());
        m_pNetwork->NavSection().SetSection(&pNet->NavSection());
        m_wNodeInfoCount = (u16)pMesh->NodeInfoCount();
        m_pNodeInfo = pMesh->NodeInfo();
        m_wLinkInfoCount = (u16)pMesh->LinkInfoCount();
        m_pLinkInfo = pMesh->LinkInfo();
    } else if (pMesh->m_bNoData) {
        XNavSection *pSect = m_pNetwork->NavSection().SectionFromHash(pNet->NavSection().m_dwHash);
        if (pSect) {
            pSect->m_pNodes = pNet->NavSection().m_pNodes;
            pSect->m_pLinks = pNet->NavSection().m_pLinks;
        }
        m_pNetwork->InstallMasterLinks(m_pLinkInfo);
        for (int i = 0; i < pNet->NavSection().m_wNodes; i++) {
            XNavNode &node = pNet->NavSection().m_pNodes[i];
            u16 zone = m_pNodeInfo[node.NodeID()] & 0xfff;
            node.SetZone(zone);
            EXGeoNavMeshEntity_PolyList *pPoly = &pMesh->Poly(i);
            if (pPoly) pPoly->m_wZone = zone;
        }
    } else {
        m_pNetwork->SetNavData(pNet->NavData());
        m_pNetwork->SetNavNodeData(pNet->NavNodeData());
        m_pNetwork->SetNavRanges(pNet->NavRanges());
        m_pNetwork->SetNavRangesCount(pNet->NavRangesCount());
        m_pNetwork->SetNavZoneLinks(pNet->NavZoneLinksCount(), pNet->NavZoneLinks());
        m_pNetwork->SetNumZones(pNet->NumZones());
        m_pNetwork->NavSection().SetSection(&pNet->NavSection());
        m_wNodeInfoCount = 0;
        m_pNodeInfo = 0;
        m_wLinkInfoCount = 0;
        m_pLinkInfo = 0;
        m_wZoneBase = pMesh->GridSector() << 12;
    }
    XNavGeomSection *g;
    u32 hash = pMesh->HashCode();
    if (!hash || hash == 0xffffffff || hash == m_Section.m_dwHash) {
        g = &m_Section;
    } else {
        if (!m_Section.m_pNext) goto done;
        g = m_Section.m_pNext->SectionFromMeshHash(hash);
    }
    if (g) {
        g->m_pMesh = pMesh;
        *(u8 *)&g->m_08 = 1;
        if (g == &m_Section) {
            g->m_dwHash = pMesh->HashCode();
            g->m_wPolys = (u16)pMesh->PolyCount();
            g->m_wPolyOffset = 0;
            g->m_wVerts = (u16)pMesh->VertCount();
            g->m_wVertOffset = 0;
            m_wPolyCount = (u16)pMesh->PolyCount();
            m_w8 = (u16)pMesh->VertCount();
            g->m_vMin.Set(10000.0f, 10000.0f, 10000.0f, 0.0f);
            g->m_vMid.Set(0.0f, 0.0f, 0.0f, 0.0f);
            g->m_vMax.Set(-10000.0f, -10000.0f, -10000.0f, 0.0f);
        }
    }
done:
    pMesh->NavNetwork()->SetNavMesh(this);
}
