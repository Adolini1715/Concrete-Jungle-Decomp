#include "EXGeoNavMeshBaseEntity.h"
#include "EXMemory.h"
#include "EXPlane.h"

ExNavCostBase g_ExNavDefaultAStarCost;
ExNavValidationBase g_ExNavDefaultValidation;
ExNavAStarLinkedList g_Open;
ExNavAStarLinkedList g_Close;

void EXGeoNavMeshBaseEntity::OnCreate(EXGeoHeader *, EXGeoObjectHeader *)
{
}

void EXGeoNavMeshBaseEntity::OnDelete(EXGeoHeader *, EXGeoObjectHeader *)
{
}

EXGeoNavMeshBaseEntity::~EXGeoNavMeshBaseEntity()
{
    ExNavPathBase *p = m_pIssuedPaths;
    while (p) {
        ExNavPathBase *t = p;
        p = p->m_pNext;
        delete t;
    }
}

signed char EXGeoNavMeshBaseEntity::IsInLineOfSight(const EXVector &, const EXVector &)
{
    return 0;
}

void EXGeoNavMeshBaseEntity::SetCostFunction(ExNavCostBase *p)
{
    if (!p) m_pCost = &g_ExNavDefaultAStarCost;
    else m_pCost = p;
}

void EXGeoNavMeshBaseEntity::SetValidationFunction(ExNavValidationBase *p)
{
    if (!p) m_pValidation = &g_ExNavDefaultValidation;
    else m_pValidation = p;
}

signed char ExNavValidationBase::IsValid(EXGeoNavMeshBaseEntity *pMesh, int tri, int prev, int edge, int user)
{
    return pMesh->GetConnections()[tri].m_edge[edge].GetIndex() != prev;
}

void ExNavPathBase::Release()
{
    if (m_flags & 2) {
        if (m_pPrev) m_pPrev->m_pNext = m_pNext;
        else m_pMesh->m_pIssuedPaths = m_pNext;
        if (m_pNext) m_pNext->m_pPrev = m_pPrev;
    }
    delete this;
}

EXGeoNavMeshBaseEntity::EXGeoNavMeshBaseEntity()
{
    m_pCost = &g_ExNavDefaultAStarCost;
    m_pValidation = &g_ExNavDefaultValidation;
    m_rayDir = EXVector(0.0f, -1.0f, 0.0f, 1.0f);
    m_rayLength = 100000.0f;
}

// NEAR: ~130 bytes differ, retail keeps tri in edi and the edge counter in the tri argument slot.
int EXGeoNavMeshBaseEntity::GetTriangle(const EXVector &p, const EXVector &d, int tri, EXVector &out, int depth, int user, int prev)
{
    if (GetTriangle(p, d, tri, out) == 1) return tri;
    if (--depth < 0) return -1;
    EXNavIndex *e = GetConnections()[tri].m_edge;
    for (u32 i = 0; i < 3; i++, e++) {
        if (!e->IsValid()) break;
        if (m_pValidation->IsValid(this, tri, prev, i, user)) {
            int r = GetTriangle(p, d, e->GetIndex(), out, depth, user, tri);
            if (r != -1) return r;
        }
    }
    return -1;
}

void ExNavAStarLinkedList::EmptyList(const EXGeoNavMeshBaseEntity *pMesh)
{
    EXNavTriangle *t = pMesh->GetTriangles();
    while (m_pHead) {
        ExNavAStarNode *n = m_pHead;
        m_pHead = n->m_pNext;
        t[n->m_tri].m_0c = 0;
        t[n->m_tri].m_v[0].SetFlags(0);
        delete n;
    }
}

// NEAR: ~80 bytes differ, scheduling of the m_goalTri load in the inlined AddTriToHead.
ExNavPathBase *EXGeoNavMeshBaseEntity::GeneratePathDepthFirst(int from, int depth, int to)
{
    if (from == m_goalTri) {
        ExNavPathBase *path = new ExNavPathBase(this);
        path->AddTriToHead(m_goalTri);
        return path;
    }
    if (!depth) return 0;
    EXNavIndex *e = GetConnections()[from].m_edge;
    for (u32 i = 0; i < 3; i++, e++) {
        if (!e->IsValid()) break;
        if (m_pValidation->IsValid(this, from, to, i, m_user)) {
            ExNavPathBase *path = GeneratePathDepthFirst(e->GetIndex(), depth - 1, from);
            if (path) {
                path->AddTriToHead(from);
                return path;
            }
        }
    }
    return 0;
}

ExNavPathBase *EXGeoNavMeshBaseEntity::GeneratePathDepthFirst(const EXVector &a, const EXVector &b, int depth)
{
    m_startTri = RayCast(a, m_rayDir, m_rayLength, -1);
    m_goalTri = RayCast(b, m_rayDir, m_rayLength, -1);
    if (m_goalTri == -1 || m_startTri == -1) return 0;
    ExNavPathBase *p = GeneratePathDepthFirst(m_startTri, depth, m_startTri);
    AddToIssuedPaths(p);
    return p;
}

ExNavPathBase *EXGeoNavMeshBaseEntity::GeneratePathBredthFirst(int from, int depth, int to)
{
    for (int d = 0; d <= depth; d++) {
        ExNavPathBase *p = GeneratePathDepthFirst(from, d, to);
        if (p) return p;
    }
    return 0;
}

ExNavPathBase *EXGeoNavMeshBaseEntity::GeneratePathBredthFirst(const EXVector &a, const EXVector &b, int depth)
{
    m_startTri = RayCast(a, m_rayDir, m_rayLength, -1);
    m_goalTri = RayCast(b, m_rayDir, m_rayLength, -1);
    if (m_goalTri == -1 || m_startTri == -1) return 0;
    ExNavPathBase *p = GeneratePathBredthFirst(m_startTri, depth, m_startTri);
    AddToIssuedPaths(p);
    return p;
}

float ExNavCostBase::CalculateCost(EXGeoNavMeshBaseEntity *pMesh, int from, int to, int edge)
{
    EXNavDistance *pDist = pMesh->GetDistances();
    if (pDist) return pDist[from].m_dist[edge];
    float dx, dy, dz;
    EXVector3 *c = pMesh->GetCentres();
    if (c) {
        const EXVector3 &f = c[from];
        const EXVector3 &t = c[to];
        dx = t.x - f.x;
        dy = t.y - f.y;
        dz = t.z - f.z;
    } else {
        EXVector3 b = pMesh->GenerateCentre(from);
        EXVector3 a = pMesh->GenerateCentre(to);
        dx = a.x - b.x;
        dy = a.y - b.y;
        dz = a.z - b.z;
    }
    return EXSqrt(dx * dx + dy * dy + dz * dz);
}

// NEAR: 14 bytes differ, the sqrt temp lands in the pMesh slot instead of the from slot.
float ExNavCostBase::EstimateCost(EXGeoNavMeshBaseEntity *pMesh, int from, int to)
{
    float dx, dy, dz;
    EXVector3 *c = pMesh->GetCentres();
    if (c) {
        const EXVector3 &f = c[from];
        const EXVector3 &t = c[to];
        dx = t.x - f.x;
        dy = t.y - f.y;
        dz = t.z - f.z;
    } else {
        EXVector3 b = pMesh->GenerateCentre(from);
        EXVector3 a = pMesh->GenerateCentre(to);
        dx = a.x - b.x;
        dy = a.y - b.y;
        dz = a.z - b.z;
    }
    float len2 = dx * dx + dy * dy + dz * dz;
    return EXSqrt(len2);
}

// A* over the triangle graph from triangle from to triangle to. The open list is kept sorted by f, each
// triangle remembers its live node and whether it is open (flag 1) or closed (flag 2). Returns the path
// start to goal, or 0 when the goal cannot be reached. Uses the default cost object, not m_pCost.
ExNavPathBase *EXGeoNavMeshBaseEntity::GeneratePathAStar(int from, int to)
{
    ExNavAStarNode *pStart = new ExNavAStarNode;
    EXNavConnection *conn = GetConnections();
    EXNavTriangle *tris = GetTriangles();
    pStart->m_g = 0.0f;
    pStart->m_h = g_ExNavDefaultAStarCost.EstimateCost(this, from, to);
    pStart->m_tri = from;
    pStart->m_f = pStart->m_h + pStart->m_g;
    tris[from].m_v[0].SetFlags(1);
    tris[from].m_0c = (u32)pStart;
    g_Open.Insert(pStart);

    for (;;) {
        if (!g_Open.m_pHead) {
            g_Open.EmptyList(this);
            g_Close.EmptyList(this);
            return 0;
        }
        ExNavAStarNode *cur = g_Open.Pop();
        tris[cur->m_tri].m_v[0].SetFlags(0);

        if (cur->m_tri == to) {
            ExNavPathBase *path = new ExNavPathBase(this);
            for (ExNavAStarNode *n = cur; n; n = n->m_pParent)
                path->AddTriToHead(n->m_tri);
            g_Open.EmptyList(this);
            g_Close.EmptyList(this);
            delete cur;
            return path;
        }

        int prev = cur->m_pParent ? cur->m_pParent->m_tri : -1;
        for (u32 e = 0; e < 3; e++) {
            if (!conn[cur->m_tri].m_edge[e].IsValid()) break;
            if (!m_pValidation->IsValid(this, cur->m_tri, prev, e, m_user)) continue;
            int next = conn[cur->m_tri].m_edge[e].GetIndex();
            float g = g_ExNavDefaultAStarCost.CalculateCost(this, cur->m_tri, next, e) + cur->m_g;
            EXNavTriangle *t = &tris[next];
            if (t->m_v[0].GetFlags() & 1) {
                ExNavAStarNode *old = (ExNavAStarNode *)t->m_0c;
                if (old->m_g < g) continue;
                t->m_v[0].SetFlags(t->m_v[0].GetFlags() & ~1u);
                g_Open.Remove(old);
                delete old;
            }
            if (t->m_v[0].GetFlags() & 2) {
                ExNavAStarNode *old = (ExNavAStarNode *)t->m_0c;
                if (old->m_g < g) continue;
                t->m_v[0].SetFlags(t->m_v[0].GetFlags() & ~2u);
                g_Close.Remove(old);
                delete old;
            }
            float h = g_ExNavDefaultAStarCost.EstimateCost(this, next, to);
            ExNavAStarNode *node = new ExNavAStarNode;
            node->m_g = g;
            node->m_h = h;
            node->m_tri = next;
            node->m_f = h + g;
            node->m_pParent = cur;
            t->m_v[0].SetFlags(t->m_v[0].GetFlags() | 1);
            t->m_0c = (u32)node;
            g_Open.Insert(node);
        }
        g_Close.AddHead(cur);
        tris[cur->m_tri].m_v[0].SetFlags(tris[cur->m_tri].m_v[0].GetFlags() | 2);
        tris[cur->m_tri].m_0c = (u32)cur;
    }
}

// Group g occupies [first, first + count) in the triangle table; -1 (or out of range) means every triangle.
static void NavMeshTriRange(const EXGeoNavMeshBaseEntity *pMesh, int group, u32 &first, u32 &end)
{
    first = 0;
    end = pMesh->m_TriangleCount;
    if (group >= 0 && (u32)group < pMesh->m_GroupCount) {
        const u32 *g = (const u32 *)pMesh->GetGroups() + group * 2;
        first = g[1] & 0xfffff;
        end = (g[0] & 0xffffff) + first;
    }
}

// Is hit on the inner side of the edge P-Q of the triangle plane? ((P - Q) x n) . (hit - P) <= 0.
static __forceinline bool NavEdgeInside(const EXVector &P, const EXVector &Q, const EXPlane &pl, float hx, float hy, float hz)
{
    float ex = P.x - Q.x, ey = P.y - Q.y, ez = P.z - Q.z;
    float px = ey * pl.C - ez * pl.B;
    float py = ez * pl.A - pl.C * ex;
    float pz = pl.B * ex - ey * pl.A;
    return px * (hx - P.x) + pz * (hz - P.z) + py * (hy - P.y) <= 0.0f;
}

// Distance along dir at which the ray meets triangle tri's plane, with the triangle's plane and corners.
// Returns false when the ray runs parallel to the plane.
static __forceinline bool NavRayPlane(const EXGeoNavMeshBaseEntity *pMesh, u32 tri, const EXVector &p, const EXVector &d,
                                      EXVector &v0, EXVector &v1, EXVector &v2, EXPlane &pl, float &t)
{
    EXVector3 *verts = pMesh->GetVertices();
    EXNavTriangle *tris = pMesh->GetTriangles();
    const EXVector3 &a = verts[tris[tri].m_v[0].GetIndex()];
    const EXVector3 &b = verts[tris[tri].m_v[1].GetIndex()];
    const EXVector3 &c = verts[tris[tri].m_v[2].GetIndex()];
    v0 = EXVector(a.x, a.y, a.z, 0.0f);
    v1 = EXVector(b.x, b.y, b.z, 0.0f);
    v2 = EXVector(c.x, c.y, c.z, 0.0f);
    pl = EXPlane(v0, v1, v2);
    float denom = pl.C * d.z + pl.B * d.y + pl.A * d.x + pl.D * d.w;
    if (!(1e-10f <= (float)fabs(denom))) return false;
    t = (pl.C * p.z + pl.B * p.y + pl.A * p.x + pl.D) * (-1.0f / denom);
    return true;
}

// First triangle (optionally within one group) that the ray p + t*d, 0 < t < maxDist, passes through;
// -1 when none.
int EXGeoNavMeshBaseEntity::RayCast(const EXVector &p, const EXVector &d, float maxDist, int group) const
{
    u32 first, end;
    NavMeshTriRange(this, group, first, end);
    for (u32 i = first; i < end; i++) {
        EXVector v0, v1, v2;
        EXPlane pl;
        float t;
        if (!NavRayPlane(this, i, p, d, v0, v1, v2, pl, t)) continue;
        if (!(0.0f < t && t < maxDist)) continue;
        float hx = d.x * t + p.x, hy = t * d.y + p.y, hz = t * d.z + p.z;
        if (NavEdgeInside(v0, v2, pl, hx, hy, hz) && NavEdgeInside(v1, v0, pl, hx, hy, hz)
            && NavEdgeInside(v2, v1, pl, hx, hy, hz))
            return (int)i;
    }
    return -1;
}

// Like RayCast, but returns the triangle hit closest to p (t from 0 up to maxDist).
int EXGeoNavMeshBaseEntity::RayCastNearest(const EXVector &p, const EXVector &d, float maxDist, int group) const
{
    u32 first, end;
    NavMeshTriRange(this, group, first, end);
    int best = -1;
    float bestT = maxDist;
    for (u32 i = first; i < end; i++) {
        EXVector v0, v1, v2;
        EXPlane pl;
        float t;
        if (!NavRayPlane(this, i, p, d, v0, v1, v2, pl, t)) continue;
        if (!(0.0f <= t && t < bestT)) continue;
        float hx = d.x * t + p.x, hy = t * d.y + p.y, hz = t * d.z + p.z;
        if (NavEdgeInside(v0, v2, pl, hx, hy, hz) && NavEdgeInside(v1, v0, pl, hx, hy, hz)
            && NavEdgeInside(v2, v1, pl, hx, hy, hz)) {
            best = (int)i;
            bestT = t;
        }
    }
    return best;
}

// Drops a vertical line from p onto triangle tri's plane; if the landing point is inside the triangle,
// writes it (w = 1) to out and returns 1. The direction argument is ignored by retail.
signed char EXGeoNavMeshBaseEntity::GetTriangle(const EXVector &p, const EXVector &, int tri, EXVector &out)
{
    if (tri == -1) return 0;
    EXVector down(0.0f, -1.0f, 0.0f, 0.0f);
    EXVector v0, v1, v2;
    EXPlane pl;
    float t;
    if (!NavRayPlane(this, (u32)tri, p, down, v0, v1, v2, pl, t)) return 0;
    float hx = t * 0.0f + p.x, hy = t * -1.0f + p.y, hz = t * 0.0f + p.z;
    if (NavEdgeInside(v0, v2, pl, hx, hy, hz) && NavEdgeInside(v1, v0, pl, hx, hy, hz)
        && NavEdgeInside(v2, v1, pl, hx, hy, hz)) {
        out.x = hx;
        out.y = hy;
        out.z = hz;
        out.w = 1.0f;
        return 1;
    }
    return 0;
}

ExNavPathBase *EXGeoNavMeshBaseEntity::GeneratePathAStar(const EXVector &a, const EXVector &b)
{
    m_startTri = RayCast(a, m_rayDir, m_rayLength, -1);
    m_goalTri = RayCast(b, m_rayDir, m_rayLength, -1);
    if (m_goalTri == -1 || m_startTri == -1) return 0;
    ExNavPathBase *p = GeneratePathAStar(m_startTri, m_goalTri);
    AddToIssuedPaths(p);
    return p;
}
