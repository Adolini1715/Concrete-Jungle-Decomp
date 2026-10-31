#include "EXGeoEntity_FaceRef.h"
#include "EXCollisionScoopTemp.h"
#include "EXGeoEntity.h"
#include "EXGeoFile.h"
#include "EXGeoHeader.h"
#include "EXTexture.h"
#include "EXTriangle.h"
#include "XBGeoDrawInfo.h"
#include "EXGeoColSphere.h"
#include "EXCollisionType.h"
#include <math.h>

void EXGeoEntity_FaceRef::Set(EXGeoEntity *pEnt, EXGDiData *pGDi, EXGeoEntity_FaceCollisionGroup *pGroup, EXGeoEntity_FaceCollisionFace *pFace)
{
    _set(pEnt, pGDi, pGroup, pFace);
    if (pEnt) {
        m_pGeoFile = pEnt->GeoFile();
        m_checkCode = m_pGeoFile->m_CheckCode;
    }
}

void EXGeoEntity_FaceRef::SetCheckCode()
{
    if (m_pEntity) {
        m_pGeoFile = ((EXGeoBaseEntity *)m_pEntity)->GeoFile();
        m_checkCode = m_pGeoFile->m_CheckCode;
    }
}

EXGeoEntity *EXGeoEntity_FaceRef::Entity() const
{
    EXGeoEntity *p = m_pEntity;
    if (p && m_pGeoFile->m_CheckCode != m_checkCode) p = 0;
    return p;
}

EXGeoFile *EXGeoEntity_FaceRef::GeoFile() const
{
    if (m_pEntity && m_pGeoFile->m_CheckCode == m_checkCode) return m_pGeoFile;
    return 0;
}

unsigned int EXGeoEntity_FaceRef::GetFaceFlags() const
{
    if (m_pGeoFile && m_pGeoFile->m_CheckCode == m_checkCode) return m_pFace->m_flags;
    return 0;
}

unsigned int EXGeoEntity_FaceRef::GetTextureIndex() const
{
    if (m_pGeoFile && m_pGeoFile->m_CheckCode == m_checkCode) return m_pGroup->m_texture;
    return 0;
}

// NEAR: ~70 bytes differ, retail reloads the check code for the inlined GetTextureIndex (no CSE).
EXTexture *EXGeoEntity_FaceRef::Texture() const
{
    EXGeoEntity *pEnt = Entity();
    if (!pEnt) return 0;
    EXGeoHeader *pHdr = ((EXGeoBaseEntity *)pEnt)->GeoFile()->GeoHeader();
    return &pHdr->TextureArray()[GetTextureIndex()];
}

EXGeoTexture *EXGeoEntity_FaceRef::GeoTexture() const
{
    EXTexture *t = Texture();
    if (!t) return 0;
    return t->GeoTexture();
}

static inline int ClampByte(int v, int lo)
{
    if (v >= 255) return 255;
    return v > lo ? v : lo;
}

// NEAR: register allocation only (retail: r in ebx, g in edx, b in ecx, a spilled).
ex::RGBA EXGeoEntity_FaceRef::MakeRGBA(const EXVector &c) const
{
    int r = ClampByte((int)c.x, 0);
    int g = ClampByte((int)c.y, 0);
    int b = ClampByte((int)c.z, 0);
    int a = ClampByte((int)c.w, 1);
    ex::RGBA col;
    col.m_B = (u8)b;
    col.m_R = (u8)r;
    col.m_G = (u8)g;
    col.m_A = (u8)a;
    return col;
}

EXGeoEntity_TriStripVert *EXGeoEntity_FaceRef::GetTriStripVert() const
{
    return (EXGeoEntity_TriStripVert *)((u16 *)m_pEntity->Index() + m_pFace->m_tex);
}

ex::RGBA EXGeoEntity_FaceRef::GetTextureColour() const
{
    EXTexture *t = Texture();
    if (t) return t->GeoTexture()->Colour();
    return ex::RGBA(0u);
}

// The triangle's unit normal (edge 0 x edge 1); straight up x when degenerate.
EXVector EXTriangle::UnitNormal() const
{
    const EXMaths::vec_xyzw &e0 = m_v[1];
    const EXMaths::vec_xyzw &e1 = m_v[2];
    float x = e0.y * e1.z - e1.y * e0.z;
    float y = e1.x * e0.z - e0.x * e1.z;
    float z = e1.y * e0.x - e1.x * e0.y;
    float l = x * x + y * y + z * z;
    if (l >= 0.0f) {
        l = (float)sqrt(l);
        if (l != 0.0f) {
            float inv = 1.0f / l;
            return EXVector(inv * x, inv * y, inv * z, inv * 0.0f);
        }
    }
    return EXVector(1.0f, 0.0f, 0.0f, 0.0f);
}

// The face texture's average colour, 0..1.
EXVector EXGeoEntity_FaceRef::GetTextureColourV() const
{
    EXTexture *t = Texture();
    unsigned int c = t ? t->GeoTexture()->Colour().argb() : 0;
    return EXVector((float)(c & 0xff) * (1.0f / 255.0f), (float)((c >> 8) & 0xff) * (1.0f / 255.0f),
                    (float)((c >> 16) & 0xff) * (1.0f / 255.0f), (float)(c >> 24) * (1.0f / 255.0f));
}

// The face's flat colour: the sum of its three vertex colours (each scaled by 1/390, so three full
// ones give about 2), each channel clamped to 1.
EXVector EXGeoEntity_FaceRef::GetFaceSingleColourV() const
{
    float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
    EXGeoEntity *pEnt = Entity();
    if (pEnt) {
        EXGeoEntity_TriStripVert *pTri = GetTriStripVert();
        const u8 *c = (const u8 *)pEnt->Rgba();
        const u8 *c0 = c + pTri->m_v[0] * 4;
        const u8 *c1 = c + pTri->m_v[1] * 4;
        const u8 *c2 = c + pTri->m_v[2] * 4;
        g = (float)c2[1] * 0.0025641026f + (float)c1[1] * 0.0025641026f + (float)c0[1] * 0.0025641026f;
        r = (float)c2[2] * 0.0025641026f + (float)c1[2] * 0.0025641026f + (float)c0[2] * 0.0025641026f;
        b = (float)c2[0] * 0.0025641026f + (float)c1[0] * 0.0025641026f + (float)c0[0] * 0.0025641026f;
        a = (float)c2[3] * 0.0025641026f + (float)c1[3] * 0.0025641026f + (float)c0[3] * 0.0025641026f;
        if (r > 1.0f) r = 1.0f;
        if (g > 1.0f) g = 1.0f;
        if (b > 1.0f) b = 1.0f;
        if (a > 1.0f) a = 1.0f;
    }
    return EXVector(r, g, b, a);
}

// The vertex colour at p, interpolated over the face (alpha 1).
EXVector EXGeoEntity_FaceRef::GetFaceVertexColourV(const EXVector &p) const
{
    EXGeoEntity *pEnt = Entity();
    if (!pEnt) return EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    signed char bLocked;
    EXGeoEntity_VtxData *pVtx;
    if (m_pGDi && m_pGDi->HasModifiedVerts()) {
        bLocked = 1;
        pVtx = (EXGeoEntity_VtxData *)m_pEntity->GetVertexBuffer(m_pGDi, 1);
    } else {
        bLocked = 0;
        pVtx = pEnt->Vtx();
    }
    EXGeoEntity_TriStripVert *pTri = GetTriStripVert();
    const u8 *c = (const u8 *)m_pEntity->Rgba();
    unsigned int i = pTri->m_v[0];
    EXVector p0(pVtx[i].m_x, pVtx[i].m_y, pVtx[i].m_z, 0.0f);
    EXVector c0((float)c[i * 4 + 2] * (1.0f / 255.0f), (float)c[i * 4 + 1] * (1.0f / 255.0f), (float)c[i * 4] * (1.0f / 255.0f), 0.0f);
    i = pTri->m_v[1];
    EXVector p1(pVtx[i].m_x, pVtx[i].m_y, pVtx[i].m_z, 0.0f);
    EXVector c1((float)c[i * 4 + 2] * (1.0f / 255.0f), (float)c[i * 4 + 1] * (1.0f / 255.0f), (float)c[i * 4] * (1.0f / 255.0f), 0.0f);
    i = pTri->m_v[2];
    EXVector p2(pVtx[i].m_x, pVtx[i].m_y, pVtx[i].m_z, 0.0f);
    EXVector c2((float)c[i * 4 + 2] * (1.0f / 255.0f), (float)c[i * 4 + 1] * (1.0f / 255.0f), (float)c[i * 4] * (1.0f / 255.0f), 0.0f);
    if (bLocked) m_pEntity->ReleaseVertexBuffer(m_pGDi);
    EXTriangle tri;
    tri.m_v[0] = p0;
    tri.m_v[1] = p1 - p0;
    tri.m_v[2] = p2 - p0;
    float s, t;
    tri.MinPointSquared(p, s, t);
    return EXVector((c1.x - c0.x) * s + c0.x + (c2.x - c0.x) * t, (c1.y - c0.y) * s + c0.y + (c2.y - c0.y) * t,
                    (c1.z - c0.z) * s + c0.z + (c2.z - c0.z) * t, s * 0.0f + 1.0f + t * 0.0f);
}

// The face colour for the material flags: 2 vertex colours (times the texture's with 1), else the
// texture's.
EXVector EXGeoEntity_FaceRef::GetFaceColourV(unsigned int flags) const
{
    EXVector c(0.0f, 0.0f, 0.0f, 0.0f);
    if (Entity()) {
        if (!(flags & 2)) {
            c = GetTextureColourV();
        } else {
            c = GetFaceSingleColourV();
            if (flags & 1) c *= GetTextureColourV();
        }
    }
    return c;
}

EXVector EXGeoEntity_FaceRef::GetFaceColourV(unsigned int flags, const EXVector &p) const
{
    EXVector c(0.0f, 0.0f, 0.0f, 0.0f);
    if (Entity()) {
        if (!(flags & 2)) {
            c = GetTextureColourV();
        } else {
            c = GetFaceVertexColourV(p);
            if (flags & 1) c *= GetTextureColourV();
        }
    }
    return c;
}

ex::RGBA EXGeoEntity_FaceRef::GetFaceColour(unsigned int flags) const
{
    return MakeRGBA(GetFaceColourV(flags) * 255.0f);
}

ex::RGBA EXGeoEntity_FaceRef::GetFaceColour(unsigned int flags, const EXVector &p) const
{
    return MakeRGBA(GetFaceColourV(flags, p) * 255.0f);
}

// The face's vertex positions (w 1) and UVs.
signed char EXGeoEntity_FaceRef::GetFaceVertexInfo(EXFixedArray<EXVector, 3> &v, EXFixedArray<EXVector2, 3> &uv) const
{
    EXGeoEntity *pEnt = Entity();
    if (!pEnt) return 0;
    signed char bLocked;
    EXGeoEntity_VtxData *pVtx;
    if (m_pGDi && m_pGDi->HasModifiedVerts()) {
        bLocked = 1;
        pVtx = (EXGeoEntity_VtxData *)m_pEntity->GetVertexBuffer(m_pGDi, 1);
    } else {
        bLocked = 0;
        pVtx = pEnt->Vtx();
    }
    EXGeoEntity_TriStripVert *pTri = GetTriStripVert();
    for (int k = 0; k < 3; k++) {
        EXGeoEntity_VtxData &d = pVtx[pTri->m_v[k]];
        v[k] = EXVector(d.m_x, d.m_y, d.m_z, 1.0f);
        uv[k] = *(EXVector2 *)&d.m_10;
    }
    if (bLocked) m_pEntity->ReleaseVertexBuffer(m_pGDi);
    return 1;
}

// The face's three UVs.
signed char EXGeoEntity_FaceRef::GetFaceUVInfo(EXVector2 *const uv) const
{
    EXGeoEntity *pEnt = Entity();
    if (!pEnt) return 0;
    EXGeoEntity_VtxData *pVtx = pEnt->Vtx();
    EXGeoEntity_TriStripVert *pTri = GetTriStripVert();
    uv[0] = *(EXVector2 *)&pVtx[pTri->m_v[0]].m_10;
    uv[1] = *(EXVector2 *)&pVtx[pTri->m_v[1]].m_10;
    uv[2] = *(EXVector2 *)&pVtx[pTri->m_v[2]].m_10;
    return 1;
}

static inline const float &ScoopMin(const float &a, const float &b) { return b < a ? b : a; }
static inline const float &ScoopMax(const float &a, const float &b) { return a < b ? b : a; }

// NEAR (see check). Gathers the faces of one platform entity that touch the scoop's shapes: sphere
// tree rejection, then per face an AABB-vs-scoop-sphere test in entity space and the exact shape test
// in scoop space. Each hit fills the next face slot (normal and contact points back in world space).
void EXCollisionScoopTemp::DoCollisionPlatform(EXCScoop_EntityInfo &e)
{
    if (m_numFaces >= m_maxFaces - 1) return;
    EXVector c = m_centre - *(const EXVector *)&e.m_mtx.r[3];
    c.w = 0.0f;
    EXVector lc;
    D3DXVec4Transform(&lc, &c, &m_invMtx);
    c = lc;
    EXGeoEntity *pEnt = (EXGeoEntity *)e.m_40;
    EXGeoColSphere_Table *pTab = pEnt->CollisionSpheres();
    if (pTab && !DoCollision_CollisionSpheres(pTab->NodeList(), pTab->Spheres(), c, m_radius)) return;
    u16 *pFC = (u16 *)pEnt->FaceCollision();
    if (!pFC) return;
    EXGDiData *pGDi = (EXGDiData *)e.m_44;
    signed char bLocked;
    EXGeoEntity_VtxData *pVtx;
    if (pGDi && pGDi->HasModifiedVerts()) {
        bLocked = 1;
        pVtx = (EXGeoEntity_VtxData *)pEnt->GetVertexBuffer(pGDi, 1);
    } else {
        bLocked = 0;
        pVtx = pEnt->Vtx();
    }
    EXMatrix m;
    D3DXMatrixMultiply(&m, &e.m_mtx, &m_invXform);
    EXMatrix toScoop = m;
    D3DXMatrixMultiply(&m, &m_xform, &m_invMtx);
    EXCScoop_FaceData *pOut = &m_pFaces[m_numFaces];
    int nGroups = *pFC;
    EXGeoEntity_FaceCollisionGroup *pGroup = (EXGeoEntity_FaceCollisionGroup *)(pFC + 4);
    float r = m_radius;
    if (nGroups) {
        do {
            EXGeoEntity_FaceCollisionFace *pFace = pGroup->m_faces;
            int n = pGroup->m_02;
            int total = n;
            if (n) {
                do {
                    EXVector v0 = *(const EXVector *)((u8 *)pVtx + pFace->m_v[0] * 0x18);
                    EXVector v1 = *(const EXVector *)((u8 *)pVtx + pFace->m_v[1] * 0x18);
                    EXVector v2 = *(const EXVector *)((u8 *)pVtx + pFace->m_v[2] * 0x18);
                    float lo;
                    float nr = 0.0f - r;
                    if ((lo = ScoopMin(ScoopMin(v0.x, v1.x), v2.x)) - c.x <= r &&
                        (lo = ScoopMin(ScoopMin(v0.y, v1.y), v2.y)) - c.y <= r &&
                        (lo = ScoopMin(ScoopMin(v0.z, v1.z), v2.z)) - c.z <= r &&
                        nr <= (lo = ScoopMax(ScoopMax(v0.x, v1.x), v2.x)) - c.x &&
                        nr <= (lo = ScoopMax(ScoopMax(v0.y, v1.y), v2.y)) - c.y &&
                        nr <= (lo = ScoopMax(ScoopMax(v0.z, v1.z), v2.z)) - c.z) {
                        v0.w = 1.0f;
                        v1.w = 1.0f;
                        v2.w = 1.0f;
                        EXVector t0, t1, t2;
                        D3DXVec4Transform(&t2, &v2, &toScoop);
                        D3DXVec4Transform(&t1, &v1, &toScoop);
                        D3DXVec4Transform(&t0, &v0, &toScoop);
                        EXTriangle tri;
                        tri.m_v[0] = t0;
                        tri.m_v[1] = t1 - t0;
                        tri.m_v[2] = t2 - t0;
                        for (u32 k = 0; k < m_numObjects; k++) {
                            if (!m_pObjects[k].Intersection(tri, pOut->m_dist, pOut->P0(), pOut->P1())) continue;
                            if (m_numFaces >= m_maxFaces) return;
                            m_numFaces++;
                            {
                                const EXMaths::vec_xyzw &e0 = tri.m_v[1];
                                const EXMaths::vec_xyzw &e1 = tri.m_v[2];
                                float nx = e0.y * e1.z - e1.y * e0.z;
                                float ny = e1.x * e0.z - e0.x * e1.z;
                                float nz = e1.y * e0.x - e1.x * e0.y;
                                float l = nx * nx + ny * ny + nz * nz;
                                EXVector nv(1.0f, 0.0f, 0.0f, 0.0f);
                                if (l >= 0.0f) {
                                    l = (float)sqrt(l);
                                    if (l != 0.0f) {
                                        float inv = 1.0f / l;
                                        nv = EXVector(inv * nx, inv * ny, inv * nz, inv * 0.0f);
                                    }
                                }
                                pOut->Normal() = nv;
                            }
                            EXVector t;
                            D3DXVec4Transform(&t, &pOut->Normal(), &m_xform);
                            pOut->Normal() = t;
                            pOut->P0().w = 1.0f;
                            pOut->P1().w = 1.0f;
                            D3DXVec4Transform(&t, &pOut->P0(), &m_xform);
                            pOut->P0() = t;
                            pOut->P0().w = 0.0f;
                            pOut->P1().w = 1.0f;
                            D3DXVec4Transform(&t, &pOut->P1(), &m_xform);
                            pOut->P1() = t;
                            pOut->P1().w = 0.0f;
                            pOut->m_34 = (u32)&e;
                            pOut->m_pGroup = pGroup;
                            pOut->m_index = (u16)(total - n);
                            pOut->m_3e = (u8)(k & 0x7f);
                            pOut++;
                        }
                    }
                    n--;
                    pFace++;
                } while (n > 0);
            }
            pGroup = (EXGeoEntity_FaceCollisionGroup *)pFace;
            nGroups--;
        } while (nGroups > 0);
    }
    if (bLocked) pEnt->ReleaseVertexBuffer(pGDi);
}
