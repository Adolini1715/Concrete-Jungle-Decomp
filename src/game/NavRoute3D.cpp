#include "XNavRoute.h"
#include "XNavMeshHandler.h"
#include "XNavMesh.h"
#include "XNavNetwork.h"
#include "EXMatrix.h"
#include "EXQuaternion.h"
#include "XNavLocation.h"
#include "EXMemory.h"
#include "FEText.h"

// 3D version of CalcRouteEx: make sure both ends sit on a polygon of the 3D mesh, route
// between them, and aim m_MoveVector at the next portal corner. Returns 1 when there is no
// route, 0 when m_MoveVector is valid.
int XNavRoute::CalcRoute3DEx(XNavLocation *pFrom, XNavLocation *pTo)
{
    m_v80 = pFrom->m_vPos;
    XNavMesh *pMesh = gNavMeshHandler.GetPtr(pFrom->m_vPos, 1);
    if (!pMesh) return 1;
    if ((pFrom->m_nPolyHandle & 0xffff) == 0xffff) {
        m_v60 = pFrom->m_vPos;
        int poly = XNavMeshHandler::PolyID(pFrom->m_nPolyHandle);
        int tri = XNavMeshHandler::PolyTri(pFrom->m_nPolyHandle);
        pMesh->LocatePolygon3D(pFrom->m_vPos, &poly, &tri, 0);
        if (poly >= 0) pFrom->SetPoly(pMesh, poly, tri);
    }
    if ((pTo->m_nPolyHandle & 0xffff) == 0xffff) {
        int poly = -1;
        int tri = XNavMeshHandler::PolyTri(pTo->m_nPolyHandle);
        m_v70 = pTo->m_vPos;
        pMesh->LocatePolygon3D(pTo->m_vPos, &poly, &tri, 0);
        if (poly < 0) {
            EXVector fixed;
            pMesh->ValidatePos(pTo->m_vPos, fixed, &poly);
            pTo->SetPosPoly(fixed, poly, 0);
        }
        pTo->SetPoly(pMesh, poly, tri);
    }
    if (GetRouteNodes(pFrom, pTo, pMesh) == 1) return 1;
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
            if (ClampToPortal(pFrom->m_vPos, m_MoveToPoint, m_MoveVector, m_v20, portal, 1, 0.0f)) {
                m_v40 = portal[0];
                m_v50 = portal[1];
            }
        }
    }
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

static float s_fStandOnSurfaceTime;

// Orient a 3D mover on the polygon it is standing on: forward is "heading" flattened onto the
// polygon plane, up is the polygon normal (flipped when the route is flagged upside down).
// Writes the Euler rotation to outRot, and optionally the matrix and quaternion.
signed char XNavRoute::StandOnSurface(const EXVector &, XNavLocation *pLoc, const EXVector &heading, EXVector &outRot,
                                      EXMatrix *pMat, EXQuaternion *pQuat)
{
    s_fStandOnSurfaceTime += 0.1f;
    XNavMesh *pMesh = pLoc ? pLoc->GetMesh() : 0;
    if (!pMesh) return 0;
    EXVector h = heading;
    outRot.Set(0.0f, 0.0f, 0.0f, 0.0f);
    if (!m_b3D) return 0;
    int poly = pLoc->GetPolyID();
    if (poly == -1) return 0;
    EXVector n;
    if (!pMesh->PolyNormal(poly, n)) return 0;
    if (m_Flags.m_b5) {
        n.x = 0.0f - n.x;
        n.y = 0.0f - n.y;
        n.z = 0.0f - n.z;
        n.w = 0.0f - n.w;
    }
    float hl2 = h.w * h.w + h.z * h.z + h.y * h.y + h.x * h.x;
    if (hl2 >= 0.0f) {
        float hl = (float)sqrt(hl2);
        if (hl > 1e-5f) {
            float inv = 1.0f / hl;
            h.x *= inv;
            h.y *= inv;
            h.z *= inv;
        }
    }
    float cx = h.y * n.z - h.z * n.y;
    float cy = h.z * n.x - n.z * h.x;
    float cz = n.y * h.x - h.y * n.x;
    EXVector ahead(pLoc->m_vPos.x + (cz * n.y - cy * n.z), pLoc->m_vPos.y + (n.z * cx - cz * n.x),
                   pLoc->m_vPos.z + (cy * n.x - n.y * cx), pLoc->m_vPos.w + 0.0f);
    EXMatrix m;
    m.SetIdentity();
    m.LookAt(pLoc->m_vPos, ahead, n, EXVector(0.0f, 0.0f, 1.0f, 0.0f));
    m.GetRotationYXZ(outRot);
    EXQuaternion q;
    q.SetRotation(m);
    if (pQuat) *pQuat = q;
    if (pMat) *pMat = m;
    return 1;
}

static __forceinline float Len4(float x, float y, float z, float w)
{
    float l2 = x * x + y * y + z * z + w * w;
    return l2 < 0.0f ? 0.0f : (float)sqrt(l2);
}

// Build an orientation matrix whose forward row is "fwd" and whose up row is as close to
// "up" as possible while staying square to it. If up is parallel to fwd it tries world Y,
// then world Z; if none work the matrix is left alone.
void XNavRoute::LookAt2(const EXVector &fwdIn, const EXVector &up, EXMatrix &out)
{
    float fx = fwdIn.x, fy = fwdIn.y, fz = fwdIn.z, fw = fwdIn.w;
    float fl2 = fw * fw + fx * fx + fz * fz + fy * fy;
    if (fl2 >= 0.0f) {
        float fl = (float)sqrt(fl2);
        if (fl > 1e-5f) {
            float inv = 1.0f / fl;
            fy *= inv;
            fz *= inv;
            fx *= inv;
            fw *= inv;
        }
    }
    float d = fw * up.w + up.z * fz + fx * up.x + up.y * fy;
    float ux = up.x - fx * d;
    float uz = up.z - fz * d;
    float uy = up.y - fy * d;
    float uw = up.w - fw * d;
    float len = Len4(ux, uy, uz, uw);
    if (len < 1e-6f) {
        ux = 0.0f - fx * fy;
        uy = 1.0f - fy * fy;
        uz = 0.0f - fz * fy;
        uw = 0.0f - fw * fy;
        len = Len4(ux, uy, uz, uw);
        if (len < 1e-6f) {
            ux = 0.0f - fx * fz;
            uy = 0.0f - fz * fy;
            uz = 1.0f - fz * fz;
            uw = 0.0f - fw * fz;
            len = Len4(ux, uy, uz, uw);
            if (len < 1e-6f) return;
        }
    }
    float inv = 1.0f / len;
    uy *= inv;
    uz *= inv;
    ux *= inv;
    float rx = uy * fz - uz * fy;
    float ry = uz * fx - ux * fz;
    float rz = ux * fy - uy * fx;
    float rw = 0.0f;
    float rl2 = rx * rx + ry * ry + rz * rz + 0.0f;
    if (rl2 >= 0.0f) {
        float rl = (float)sqrt(rl2);
        if (rl > 1e-5f) {
            float rinv = 1.0f / rl;
            rx *= rinv;
            ry *= rinv;
            rz *= rinv;
            rw = rinv * 0.0f;
        }
    }
    out.SetIdentity();
    out.r[0].set(rx, ry, rz, rw);
    out.r[1].set(ux, uy, uz, uw * inv);
    out.r[2].set(fx, fy, fz, fw);
}

EXVector &XNavRoute::CalcRoute3D(XNavLocation *pFrom, XNavLocation *pTo)
{
    int r = CalcRoute3DEx(pFrom, pTo);
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
