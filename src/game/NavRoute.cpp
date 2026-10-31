#include "XNavRoute.h"
#include "XNavMeshHandler.h"
#include "XNavMesh.h"
#include "XNavNetwork.h"
#include "EXMatrix.h"
#include "EXRand.h"
#include "EXMemory.h"
#include "FEText.h"
#include "AIControl.h"
#include "EXLine.h"
#include "EXQuaternion.h"

s8 gbDrawNavPaths;

XNavRoute::XNavRoute()
{
    m_bInit = 0;
    m_v60.SetZero();
    m_v70.SetZero();
    m_Flags.m_bDynamic = 0;
    m_e8 = 0;
    m_c4 = -1;
    m_b3D = 0;
    m_Status = 1;
    m_fcc = g_EXRandClass.Randf() * 0.5f;
    m_Flags.m_bNoJump = 0;
    m_e0 = -1;
    m_dc = -1;
    m_e4 = -1;
    m_NavIgnoreFlags = 8;
    m_d8 = 1;
}

XNavRoute::~XNavRoute() {}

void XNavRoute::Init(signed char b3D, signed char bForce)
{
    if (!bForce && m_bInit && b3D == m_b3D) return;
    m_v00.SetZero();
    m_MoveToPoint.SetZero();
    m_v20.SetZero();
    m_MoveVector.SetZero();
    m_v40.SetZero();
    m_v50.SetZero();
    m_v60.SetZero();
    m_v70.SetZero();
    m_v80.SetZero();
    m_v90.SetZero();
    m_JumpFromPos.SetZero();
    m_JumpToPos.SetZero();
    m_b3D = b3D;
    m_bInit = 1;
    m_c1 = 0;
    m_Status = 0;
    m_c4 = 0;
    m_fcc = 0.0f;
    m_d0 = -1;
    m_d4 = -1;
    m_e8 = 0;
    m_JumpFromNode = 0xffff;
    m_JumpNode = 0xffff;
    m_JumpToPoly = 0xffff;
    m_fJumpTime = 0.0f;
    m_nNodes = 0;
    for (int i = 0; i < 6; i++) {
        m_Nodes[i] = 0xffff;
        m_Links[i] = 0xffff;
    }
    m_Flags.m_bNavigating = 0;
    m_Flags.m_bCollided = 0;
    m_Flags.m_b2 = 0;
    m_Flags.m_b3 = 0;
    m_Flags.m_bDynamic = 0;
    m_Flags.m_b5 = 0;
    m_Flags.m_bNoJump = 0;
    m_Flags.m_b7 = 0;
    m_Flags.m_bTriggerJump = 0;
    m_Flags.m_bJumpCancel = 0;
    m_e0 = -1;
    m_dc = -1;
    m_TriggerJump = 0;
    m_Flags2.m_b0 = 0;
    m_Flags2.m_b1 = 0;
}

int XNavRoute::GetRouteNodes(XNavLocation *pFrom, XNavLocation *pTo, XNavMesh *pMesh)
{
    m_nNodes = 0;
    if (!pFrom || !pTo) return 2;
    if (pFrom->GetPolyID() == pTo->GetPolyID()) {
        m_nNodes = 0;
        return 3;
    }
    if (m_Flags.m_bDynamic) {
        if (!gNavMeshHandler.CalcAStar(*pFrom, *pTo, m_NavIgnoreFlags)) return 1;
    }
    int node = pFrom->GetPolyID();
    int link;
    m_nNodes = 0;
    link = m_d4 = -1;
    for (; m_nNodes < 6;) {
        if (m_nNodes > 0 && m_d4 < 0) m_d4 = node;
        if (node == pTo->GetPolyID()) {
            m_Nodes[m_nNodes] = (u16)node;
            m_Links[m_nNodes] = (u16)link;
            m_nNodes++;
            return 0;
        }
        int prev = node;
        if (m_Flags.m_bDynamic)
            node = pMesh->NavNetwork()->GetNextNode(node, pTo->GetPolyID(), &link);
        else
            node = pMesh->NavNetwork()->GetBestNode(node, pTo->GetPolyID(), &link);
        if (node < 0) return 1;
        m_Nodes[m_nNodes] = (u16)prev;
        m_Links[m_nNodes] = (u16)link;
        m_nNodes++;
    }
    return 0;
}

signed char XNavRoute::CalcJourneyPos(const EXVector &from, const EXVector &to, int &fromPoly, int &toPoly,
                                      float speed, float dt, EXVector &out, int &outPoly, float &, int)
{
    XNavLocation lFrom(from, fromPoly, -1);
    XNavLocation lTo(to, toPoly, -1);
    if (lFrom.Validate(0, 0) && lTo.Validate(0, 0)) {
        float dist = 0.0f;
        if (gNavMeshHandler.SetUpRoute(lFrom, lTo, dist, 0) > 0)
            gNavMeshHandler.GetRoutePos(lFrom, lTo, speed * dt, out, outPoly);
        return 1;
    }
    return 0;
}

// NEAR: retail calls XNavLocation::GetPolyID out of line for pTo (header has it __forceinline),
// which also swaps edi/ebx.
signed char XNavRoute::RouteQuickTest(XNavLocation *pFrom, XNavLocation *pTo)
{
    if (!pFrom->Validate(0, 0)) return 0;
    if (!pTo->Validate(0, 0)) return 0;
    XNavMesh *pMesh = pFrom->GetMesh();
    if (pMesh && pMesh == pTo->GetMesh()) {
        int poly = pFrom->GetPolyID();
        if (poly >= 0 && pTo->GetPolyID() >= 0) {
            if (pFrom->NavZone() == pTo->NavZone()) return 1;
            if (m_Flags.m_bDynamic && !(m_NavIgnoreFlags & 0x40) &&
                pMesh->NavNetwork()->ZonesAreLinked(pFrom->NavZone(), pTo->NavZone(), 0))
                return 1;
        }
    }
    return 0;
}

void XNavRoute::InitJump(int fromNode, const EXVector &from, int toPoly, const EXVector &to, float t)
{
    if (m_Flags.m_bNoJump || m_Flags.m_b7) return;
    m_Flags.m_bTriggerJump = 0;
    m_Flags.m_bJumpCancel = 0;
    m_Flags.m_b7 = 1;
    m_JumpFromNode = (u16)fromNode;
    m_JumpNode = (u16)fromNode;
    m_JumpToPoly = (u16)toPoly;
    m_TriggerJump = 0;
    SetJumpFromPos(from);
    SetJumpToPos(to);
    m_fJumpTime = t;
}

void XNavRoute::EndJump()
{
    m_Flags.m_b7 = 0;
    m_Flags.m_bTriggerJump = 0;
    m_Flags.m_bJumpCancel = 0;
}

// NEAR: retail inlines only the first XNavNetwork::NodeCount and calls the later three out of line.
int XNavRoute::TraversePolyCallback(NavTraverse &t)
{
    int node = t.m_nNode;
    if (node >= 0) {
        XNavNetwork *pNet = t.m_pMesh->NavNetwork();
        if (node < pNet->NodeCount()) {
            u16 type = pNet->NodeTypeFlags((u16)t.m_nNode);
            if (!(type & 8) && !(type & t.m_dwIgnore)) {
                s8 isNext = 0;
                if (IsNavigating() && m_nNodes >= 2 && node == m_Nodes[1]) isNext = 1;
                if (IsNavigating() && m_nNodes >= 3 && node == m_Nodes[1]) {
                    u16 next = m_Nodes[2];
                    if (next != 0xffff && (type & 0x40)) {
                        if (!m_Flags.m_bNoJump && !m_TriggerJump && (!m_Flags.m_b7 || m_Flags.m_bJumpCancel)) {
                            m_TriggerJump = 1;
                            int n = t.m_nFromNode;
                            if (n >= t.m_pMesh->NavNetwork()->NodeCount()) n = -1;
                            m_JumpFromNode = (u16)n;
                            n = t.m_nNode;
                            if (n >= t.m_pMesh->NavNetwork()->NodeCount()) n = -1;
                            m_JumpNode = (u16)n;
                            if (next < t.m_pMesh->NavNetwork()->NodeCount())
                                m_JumpToPoly = next;
                            else
                                m_JumpToPoly = 0xffff;
                        }
                        return -1;
                    }
                }
                if (!(type & 0x40)) {
                    if (isNext) return 2;
                    if (m_Flags.m_b7 && node == m_JumpToPoly) return 2;
                    return (type & 0x800) == 0;
                }
            }
        }
    }
    return 0;
}

void XNavRoute::RecoverFromBrokenJump(XNavLocation &loc)
{
    EXVector p = m_JumpToPos;
    loc.SetPosPoly(p, m_JumpToPoly, 0);
}

// NEAR: ~20 bytes differ, retail sets the two result bits with xor-merge chains; ours merges them with and/or.
signed char XNavRoute::Collide(XNavLocation *pLoc, EXVector &v)
{
    if (pLoc && pLoc->IsValid() && pLoc->GetMesh()) {
        m_Flags.m_bCollided = 0;
        u32 flags = (m_Flags2.m_b0 << 4) | (m_Flags.m_b7 << 3);
        m_c1 = 1;
        pLoc->SetRoute(this);
        pLoc->Ignore(8);
        pLoc->MoveTo(v, &flags);
        v = pLoc->Pos();
        m_Flags.m_bCollided = flags >> 2;
        m_Flags.m_b2 = flags >> 1;
        return m_Flags.m_bCollided;
    }
    return 0;
}

EXVector XNavRoute::RandomPosition(XNavLocation *pFrom)
{
    return pFrom->GetMesh()->RandomPosition(pFrom->m_sNavZone);
}

// Pull the target point back inside a portal (an edge shared by two polygons) so the walk
// crosses it with "margin" to spare. Returns 1 when the target was clamped to an end of the
// portal, 0 when it was only moved onto the portal (or the lines never meet).
signed char XNavRoute::ClampToPortal(const EXVector &from, EXVector &target, EXVector &moveVec, EXVector &angle, EXVector *portal,
                                     signed char, float margin)
{
    EXVector push(portal[1].x - portal[0].x, portal[1].y - portal[0].y, portal[1].z - portal[0].z, portal[1].w - portal[0].w);
    if (push.x * push.x + push.z * push.z <= 9.0f) {
        push.Set(0.0f, 0.0f, 0.0f, 0.0f);
    } else {
        float lenSq = push.x * push.x + push.y * push.y + push.z * push.z + push.w * push.w;
        if (lenSq >= 0.0f) {
            float len = (float)sqrt(lenSq);
            if (len > 1e-5f) {
                float inv = 1.0f / len;
                push.x *= inv;
                push.y *= inv;
                push.z *= inv;
                push.w *= inv;
            }
        }
        margin = margin * 3.0f;
        push.x *= margin;
        push.y *= margin;
        push.z *= margin;
        push.w *= margin;
    }
    EXLineSeg walk(from, EXVector(target.x - from.x, target.y - from.y, target.z - from.z, target.w - from.w));
    EXLineSeg edge(portal[0], EXVector(portal[1].x - portal[0].x, portal[1].y - portal[0].y, portal[1].z - portal[0].z,
                                        portal[1].w - portal[0].w));
    EXLineSeg nearest;
    float sWalk;
    float tEdge;
    if (!XNavNetwork::ShortestLineSegBetween2LineSegs(walk, edge, nearest, sWalk, tEdge)) return 0;
    signed char clamped = 1;
    if (tEdge < 0.0f) {
        target.x = portal[0].x + push.x;
        target.z = portal[0].z + push.z;
        target.y = portal[0].y + push.y;
        target.w = portal[0].w + push.w;
    } else if (tEdge > 1.0f) {
        target.x = portal[1].x - push.x;
        target.z = portal[1].z - push.z;
        target.y = portal[1].y - push.y;
        target.w = portal[1].w - push.w;
    } else {
        target.x = nearest.m_Direction.x + nearest.m_Origin.x;
        target.z = nearest.m_Direction.z + nearest.m_Origin.z;
        target.y = nearest.m_Direction.y + nearest.m_Origin.y;
        target.w = nearest.m_Direction.w + nearest.m_Origin.w;
        clamped = 0;
    }
    float dx = target.x - from.x;
    float dz = target.z - from.z;
    float yaw = (float)atan2(dx, dz);
    moveVec.x = dx;
    moveVec.y = target.y - from.y;
    moveVec.z = dz;
    moveVec.w = target.w - from.w;
    angle.y = yaw;
    return clamped;
}


float XNavRoute::GetFloorHeight(XNavMesh *pMesh, const EXVector &p, int poly, int tri)
{
    if (!pMesh) {
        pMesh = gNavMeshHandler.GetPtr(p, 0);
        if (!pMesh) return p.y;
    }
    if (poly < 0) {
        pMesh->LocatePolygon(p, &poly, &tri, 0);
        if (poly < 0) return p.y;
    }
    return pMesh->GetHeight(p, poly, tri, 0);
}

signed char XNavRoute::RouteQuickTest(XNavLocation *pFrom, const EXVector &to, int poly)
{
    XNavLocation loc;
    loc.SetPosPoly(to, poly, 0);
    signed char r = RouteQuickTest(pFrom, &loc);
    return r;
}

// Work out where to jump to when the route crosses from pRoute[1] to pRoute[2] by a jump link:
// the point where the heading crosses the shared edge (or its middle), nudged 0.2 into the
// destination polygon. Returns 1 when the nodes share a link, 0 when it fell back to the
// destination node (or the move target).
signed char XNavRoute::CalcJumpToPos(XNavMesh *pMesh, const EXVector &from, u16 *pRoute, const EXVector &, EXVector &out)
{
    XNavNetwork *pNet = pMesh->NavNetwork();
    XNavNode *nodeA = pNet->m_Section.Node(pNet->m_Section.m_wNodeOffset + pRoute[1]);
    XNavNode *nodeB = pNet->m_Section.Node(pNet->m_Section.m_wNodeOffset + pRoute[2]);
    for (int i = 0; i < 16; i++) {
        XNavLink *link = 0;
        XNavNode *other = pNet->ConnectedNodePtr(nodeA, (u16)i, &link);
        if (nodeB != other) continue;
        if (link && pRoute[1] != 0xffff && pRoute[2] != 0xffff) {
            EXVector portal[2];
            const EXVector3 *v0 = pMesh->VertexPos(link->m_wEdgeVert[0]);
            portal[0] = EXVector(v0->x, v0->y, v0->z, 0.0f);
            const EXVector3 *v1 = pMesh->VertexPos(link->m_wEdgeVert[1]);
            portal[1] = EXVector(v1->x, v1->y, v1->z, 0.0f);
            EXVector target(from.x + m_MoveVector.x * 100.0f, from.y + m_MoveVector.y * 100.0f,
                            from.z + m_MoveVector.z * 100.0f, from.w + m_MoveVector.w * 100.0f);
            ClampToPortal(from, target, m_MoveVector, m_v20, portal, 0, m_fcc);
            EXVector hit;
            if (XNavNetwork::LineSegsIntersect2D(from, target, portal[0], portal[1], &hit)) {
                out = hit;
                if (pRoute[2] != 0xffff) out.y = pMesh->NavNetwork()->m_Section.Node(pRoute[2])->m_Pos.y;
            } else {
                EXVector mid((portal[1].x + portal[0].x) * 0.5f, (portal[1].y + portal[0].y) * 0.5f,
                             (portal[1].z + portal[0].z) * 0.5f, (portal[1].w + portal[0].w) * 0.5f);
                out = mid;
                if (pRoute[2] != 0xffff) out.y = pMesh->NavNetwork()->m_Section.Node(pRoute[2])->m_Pos.y;
            }
            if (pRoute[2] != 0xffff) {
                float dx = portal[1].x - portal[0].x;
                float nx = 0.0f - (portal[1].z - portal[0].z);
                float len2 = dx * dx + nx * nx;
                float w = 0.0f;
                if (len2 >= 0.0f) {
                    float len = (float)sqrt(len2);
                    if (len > 1e-5f) {
                        float inv = 1.0f / len;
                        nx *= inv;
                        w = inv * 0.0f;
                        dx *= inv;
                    }
                }
                out.x += nx * 0.2f;
                out.y += w * 0.2f;
                out.z += dx * 0.2f;
                out.w += w * 0.2f;
                pMesh->SnapToPolygon(out, pRoute[2]);
            }
            return 1;
        }
        break;
    }
    if (pRoute[2] != 0xffff) {
        XNavNode *dest = pMesh->NavNetwork()->m_Section.Node(pRoute[2]);
        out = EXVector(dest->m_Pos.x, dest->m_Pos.y, dest->m_Pos.z, 0.0f);
        return 0;
    }
    out = m_MoveToPoint;
    return 0;
}


// Orient an object on the floor: the matrix looks along the heading in "rot" but lies in the
// plane with normal "normal".
static void OrientOnFloor(const EXVector &pos, const EXVector &rot, const EXVector &normal, EXMatrix &m, EXVector &outRot,
                          EXQuaternion &q)
{
    float sx = (float)sin(rot.x), cx = (float)cos(rot.x);
    float sy = (float)sin(rot.y), cy = (float)cos(rot.y);
    float sz = (float)sin(rot.z), cz = (float)cos(rot.z);
    m.r[0].set(sz * sy * sx + cz * cy, sz * cx, sz * cy * sx - cz * sy, 0.0f);
    m.r[1].set(cz * sy * sx - sz * cy, cz * cx, cz * cy * sx + sz * sy, 0.0f);
    m.r[2].set(sy * cx, 0.0f - sx, cy * cx, 0.0f);
    m.r[3].set(0.0f, 0.0f, 0.0f, 1.0f);
    EXVector ahead(pos.x + m.r[2].x, pos.y + m.r[2].y, pos.z + m.r[2].z, pos.w + m.r[2].w);
    m.SetIdentity();
    m.LookAt(pos, ahead, normal, EXVector(0.0f, 0.0f, 1.0f, 0.0f));
    m.GetRotationYXZ(outRot);
    q.SetRotation(m);
}

// Put a position and Euler rotation on the floor of the nav mesh under it: the snapped
// position, the rotation that lies flat on the polygon, optionally the poly handle, the
// orientation matrix and the quaternion. Returns 0 if there is no floor there.
signed char XNavRoute::FloorInfo(const EXVector &pos, const EXVector &rot, EXVector &outPos, EXVector &outRot, int *pPoly,
                                 EXMatrix *pMat, EXQuaternion *pQuat)
{
    outPos = pos;
    outRot = rot;
    int poly = -1;
    int tri = -1;
    XNavMesh *pMesh = gNavMeshHandler.GetPtr(pos, 0);
    if (!pMesh) return 0;
    EXMatrix m;
    EXQuaternion q;
    EXVector normal;
    if (pMesh->MeshCount() == 0) {
        pMesh = gNavMeshHandler.GetPtr(pos, 1);
        if (!pMesh || pMesh->MeshCount() == 0) return 0;
        pMesh->LocatePolygon(outPos, &poly, &tri, 0);
        if (poly < 0) return 0;
        if (pPoly) *pPoly = (poly & 0xffff) | 0xff000000;
        pMesh->SnapToPolygon(outPos, poly);
        if (!pMesh->PolyNormal(poly, normal)) return 0;
    } else {
        pMesh->ValidatePos(pos, outPos, &poly);
        pMesh->LocatePolygon(outPos, &poly, &tri, 0);
        outPos.y = pMesh->GetHeight(outPos, poly, tri, 0);
        if (poly < 0) return 0;
        if (pPoly) *pPoly = (poly & 0xffff) | 0xff000000;
        if (!pMesh->PolyNormal(poly, normal)) return 0;
    }
    OrientOnFloor(outPos, rot, normal, m, outRot, q);
    if (pQuat) *pQuat = q;
    if (pMat) *pMat = m;
    return 1;
}


// Route one step of a ground journey: find the chain of polygons from pFrom to pTo (cached
// until either end changes polygon), then aim m_MoveVector at the next corner.
// Returns 1 when no route is needed or possible, 0 when m_MoveVector is valid.
int XNavRoute::CalcRouteEx(XNavLocation *pFrom, XNavLocation *pTo)
{
    if (m_b3D) return CalcRoute3DEx(pFrom, pTo);
    if (!m_c1) m_v80 = pFrom->m_vPos;
    if (!RouteQuickTest(pFrom, pTo)) return 1;
    XNavMesh *pMesh = pFrom->GetMesh();
    int fromPoly = pFrom->GetPolyID();
    if (m_dc >= 0 && m_e0 >= 0 && fromPoly == m_dc && pTo->GetPolyID() == m_e0 && m_e4 == gAIControl.m_NavCacheSync) {
    } else {
        m_d8 = GetRouteNodes(pFrom, pTo, pMesh);
        m_dc = fromPoly;
        m_e0 = pTo->GetPolyID();
        m_e4 = gAIControl.m_NavCacheSync;
    }
    if (m_d8 == 1) return 1;
    m_v00 = pTo->m_vPos;
    m_MoveToPoint = pTo->m_vPos;
    m_MoveVector = m_v00 - pFrom->m_vPos;
    m_v20.y = (float)atan2(m_MoveVector.x, m_MoveVector.z);
    m_d4 = pFrom->GetPolyID();
    if (pFrom->GetPolyID() != pTo->GetPolyID()) {
        EXVector portal[2];
        portal[0] = pTo->m_vPos;
        portal[1] = portal[0];
        for (int i = m_nNodes - 2; i >= 0; i--) {
            pMesh->NavNetwork()->GetLinkPortal(m_Nodes[i], m_Links[i], portal);
            bool bSet = ClampToPortal(pFrom->m_vPos, m_MoveToPoint, m_MoveVector, m_v20, portal, 0, m_fcc) != 0;
            if (!bSet) {
                u16 flags = m_Nodes[i] < pMesh->PolyCount() ? (u16)pMesh->PolyFlags(m_Nodes[i]) : 0;
                bSet = (flags & 0x40) != 0;
            }
            if (bSet) {
                m_v40 = portal[0];
                m_v50 = portal[1];
            }
        }
    }
    m_MoveVector.y = 0.0f;
    float lenSq = m_MoveVector.x * m_MoveVector.x + m_MoveVector.y * m_MoveVector.y + m_MoveVector.z * m_MoveVector.z +
                  m_MoveVector.w * m_MoveVector.w;
    if (lenSq >= 0.0f) {
        float len = (float)sqrt(lenSq);
        if (len > 1e-5f) {
            float inv = 1.0f / len;
            m_MoveVector.x *= inv;
            m_MoveVector.y *= inv;
            m_MoveVector.z *= inv;
            m_MoveVector.w *= inv;
        }
    }
    return 0;
}


// NEAR: stack frame 0x10 larger than retail (temporaries not shared), same control flow.
int XNavRoute::Update(XNavLocation *pFrom, XNavLocation *pTo, float dist)
{
    int node = pFrom->GetPolyID();
    EXVector pos = pFrom->Pos();
    if (pFrom->GetPolyID() == pTo->GetPolyID()) return 3;
    XNavMesh *pMesh = gNavMeshHandler.GetPtr(pFrom->Pos(), 0);
    if (!pMesh) return 1;
    int next;
    do {
        int link;
        next = pMesh->NavNetwork()->GetBestNode(node, pTo->GetPolyID(), &link);
        if (next < 0) return 1;
        EXVector3 *np = pMesh->NavNetwork()->NodePos((u16)next);
        EXVector p(np->x, np->y, np->z, 0.0f);
        EXVector d = p - pos;
        float len = d.LengthXYZ();
        if (len >= dist) {
            d *= dist / len;
            pFrom->m_vPos = d + pos;
            pFrom->SetPoly(pMesh, node, -1);
            return 0;
        }
        pos = p;
        node = next;
    } while (next != pTo->GetPolyID());
    return 3;
}

signed char XNavRoute::GetFleePosition(const EXVector &from, const EXVector &away, float dist, EXVector &out)
{
    XNavMesh *pMesh = gNavMeshHandler.GetPtr(from, 0);
    if (!pMesh) return 0;
    return pMesh->NavNetwork()->GetFleePosition(from, away, dist, out, 0);
}

EXVector &XNavRoute::CalcRoute(XNavLocation *pFrom, XNavLocation *pTo)
{
    if (m_b3D) return CalcRoute3D(pFrom, pTo);
    int r = CalcRouteEx(pFrom, pTo);
    m_Status = (s8)r;
    switch (r) {
    case 0:
        return m_MoveVector;
    case 1:
        m_MoveVector.x = 0.0f;
        m_MoveVector.z = 0.0f;
        m_MoveVector.y = 0.0f;
        return m_MoveVector;
    default:
        m_MoveVector.x = 0.0f;
        m_MoveVector.z = 0.0f;
        m_MoveVector.y = 0.0f;
        return m_MoveVector;
    }
}
