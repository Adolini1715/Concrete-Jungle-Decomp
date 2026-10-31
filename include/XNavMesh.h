#ifndef XNAVMESH_H
#define XNAVMESH_H
#include "types.h"
#include "EXVector.h"
#include "XGeoNavMeshEntity.h"
#include "XNavNetwork.h"

// Game nav mesh (NavMesh.obj): a chain of geometry sections (one per loaded nav-mesh entity)
// plus the nav network built over them.

class XNavLocation;
class XNavRoute;
class XNavSubMap;
class EXPlane;
struct NavTraverse;

// One loaded nav-mesh entity's polygon and vertex range (sizeof 0x48).
struct XNavGeomSection {
    __forceinline XGeoNavMeshEntity *Mesh() { return m_pMesh; }
    XNavGeomSection *SectionFromPolyID(int id)
    {
        if (id >= m_wPolyOffset && id < m_wPolyOffset + m_wPolys) return this;
        if (!m_pNext) return 0;
        return m_pNext->SectionFromPolyID(id);
    }
    EXGeoNavMeshEntity_PolyList *Poly(int id)
    {
        if (m_wPolys && id >= m_wPolyOffset && id < m_wPolyOffset + m_wPolys) {
            if (m_pMesh) return &m_pMesh->Poly(id - m_wPolyOffset);
            return 0;
        }
        if (m_pNext) return m_pNext->Poly(id);
        return 0;
    }
    // TODO body: inline emitted in ActorUtil.obj (0x85ef0).
    XNavGeomSection *SectionNearestPos(const EXVector &p, float &dist, XNavGeomSection *&best);
    __forceinline void SetMesh(XGeoNavMeshEntity *p) { m_pMesh = p; }
    XNavGeomSection *SectionFromMeshHash(u32 hash)
    {
        if (hash == m_dwHash) return this;
        if (m_pNext) return m_pNext->SectionFromMeshHash(hash);
        return 0;
    }
    XNavGeomSection *SectionFromPos(const EXVector &p)
    {
        if (!(m_vMin.x > p.x) && !(p.x >= m_vMax.x) && !(m_vMin.z > p.z) && !(p.z >= m_vMax.z)) return this;
        if (!m_pNext) return 0;
        return m_pNext->SectionFromPos(p);
    }
    void AddSection(XNavGeomSection *s)
    {
        if (m_pNext) {
            m_pNext->AddSection(s);
        } else {
            m_pNext = s;
            s->m_pNext = 0;
        }
    }
    void SetMeshLighting()
    {
        if (m_pMesh) m_pMesh->SetMeshLighting();
        if (m_pNext) m_pNext->SetMeshLighting();
    }
    __forceinline XNavGeomSection() {}
    const EXVector3 *VertexPos(int i)
    {
        if (i >= m_wVertOffset && i < m_wVertOffset + m_wVerts) {
            if (m_pMesh) return &m_pMesh->VertexPos(i - m_wVertOffset);
            return 0;
        }
        if (m_pNext) return m_pNext->VertexPos(i);
        return 0;
    }

    XGeoNavMeshEntity *m_pMesh;     // +0x00
    XNavGeomSection *m_pNext;       // +0x04
    u32 m_08;                       // +0x08
    u32 m_dwHash;                   // +0x0c
    u16 m_wPolys;                   // +0x10
    u16 m_wPolyOffset;              // +0x12
    u16 m_wVerts;                   // +0x14
    u16 m_wVertOffset;              // +0x16
    EXVector m_vMin;                // +0x18
    EXVector m_vMid;                // +0x28
    EXVector m_vMax;                // +0x38
};

class XNavMesh {
public:
    __forceinline XNavNetwork *NavNetwork() { return m_pNetwork; }
    XNavGeomSection *SectionFromPolyID(int id) { return m_Section.SectionFromPolyID(id); }
    XNavGeomSection *SectionNearestPos(const EXVector &p)
    {
        float dist = 3.4028235e38f;
        XNavGeomSection *best = 0;
        return m_Section.SectionNearestPos(p, dist, best);
    }
    __forceinline const u32 PolyCount() { return m_wPolyCount; }
    // [fi] in Actor.obj: lighting of the polygon, 0 when the polygon has no mesh.
    s8 GetLighting(const EXVector &p, int poly, float &r, float &g, float &b)
    {
        XNavGeomSection *pSec = SectionFromPolyID(poly);
        XGeoNavMeshEntity *pMesh = pSec ? pSec->Mesh() : 0;
        return pMesh ? pMesh->GetLighting(p, poly, r, g, b) : 0;
    }
    __forceinline XNavGeomSection *SectionBase() { return &m_Section; }
    __forceinline EXGeoNavMeshEntity_PolyList *Poly(int i) { return m_Section.Poly(i); }
    __forceinline u32 PolyFlags(int i)
    {
        if (i >= 0 && i < m_wPolyCount) return Poly(i)->m_wFlags;
        return 0;
    }
    __forceinline u16 MeshCount() { return m_wMeshCount; }
    __forceinline s16 PolyZone(int i)
    {
        if (i < 0) return -1;
        EXGeoNavMeshEntity_PolyList *p = Poly(i);
        return !p ? 0xffff : p->m_wZone | 0x8000;
    }
    __forceinline s8 Is3D() { return m_b3D; }
    XNavGeomSection *SectionFromPos(const EXVector &p) { return m_Section.SectionFromPos(p); }
    void Zero();
    void Init();
    void RefreshNavMesh(XGeoNavMeshEntity *);
    void DelNavMesh(XGeoNavMeshEntity *);
    s8 UpdatePtrs(XGeoNavMeshEntity *, XGeoNavMeshEntity *);
    s8 PolyVerts(int, EXVector *, int *);
    s8 PolyNormal(int, EXVector &);
    s8 PolyPlane(int, EXPlane &);
    float GetFloorHeight(const EXVector &);
    float GetHeight(const EXVector &, int, int, EXVector *);
    s8 SnapToPolygon(EXVector &, int);
    s8 SnapToPolygonPlane(EXVector &, int);
    s8 SnapToPolygonBounds(EXVector &, int);
    EXVector RandomPosition(u16);
    void SetMeshLighting();
    s8 ValidateMesh();
    s8 ClampPointToPoly(int, const EXVector &, EXVector &);
    void FloodPolyFlags(u16, u16, u16, u16);
    XNavMesh();
    void CleanUp();
    void AddNavMesh(XGeoNavMeshEntity *, int, XNavSubMap *);
    s8 LocatePolygon3D(const EXVector &, int *, int *, u32);
    s8 LocatePolygon3DRay(const EXVector &, const EXVector &, int *, u32, float *, EXVector *);
    int NearestPolygon(const EXVector &, EXVector *, u32);
    s8 PosIsValid(const EXVector &, int &);
    s8 PointInPolygon(const EXVector &p, int poly)
    {
        XNavGeomSection *s = SectionFromPolyID(poly);
        if (s && s->m_pMesh) return s->m_pMesh->PointInPolygon(p, poly);
        return 0;
    }
    s8 PointInPolygon3D(const EXVector &p, int poly)
    {
        XNavGeomSection *s = SectionNearestPos(p);
        if (s && s->m_pMesh) return s->m_pMesh->PointInPolygon3D(p, poly);
        return 0;
    }
    s8 LineExitsPoly(const EXVector &, const EXVector &, int, int &, int &, EXVector &, EXVector &, EXVector &, int);
    s8 LineExitsPolyIE(const EXVector &, const EXVector &, int, int &, int &, EXVector &, EXVector &, EXVector &, int);
    s8 LineExitsPolyIE2(EXVector &, EXVector &, XNavNode *, int, int &, int &, EXVector &, EXVector &, EXVector &, int);
    s8 LineExitsPolyIE3(EXVector &, EXVector &, XNavNode *, int, int &, int &, EXVector &, EXVector &, EXVector &, int);
    s8 LineExitsPoly3D(const EXVector &, const EXVector &, int, int &, int &, EXVector &, EXVector &, EXVector &);
    s8 ValidateLine(const EXVector &, const EXVector &, int, EXVector &, int &, u32);
    s8 ValidateLine(XNavLocation &, const EXVector &, int &, EXVector &, u32);
    s8 ValidateLine(XNavLocation &, XNavLocation &, XNavLocation &, u32);
    ~XNavMesh();
    s8 LocatePolygon(const EXVector &, int *, int *, u32);
    s8 ValidatePos(const EXVector &, u32, EXVector &, int *);
    s8 TraversePolys(const EXVector &, int &, EXVector &, u32, u32 *, XNavRoute *);
    s8 ValidatePos(const EXVector &, EXVector &, int *);
    __forceinline u32 Handle() { return m_dwHandle; }
    __forceinline void SetHandle(u32 h) { m_dwHandle = h; }
    __forceinline const EXVector3 *VertexPos(int i) { return m_Section.VertexPos(i); }
private:
    int TraversePolys(NavTraverse &);
public:
    u32 m_dwHandle;                 // +0x00
    u16 m_wMeshCount;               // +0x04
    u16 m_wPolyCount;               // +0x06
    u16 m_w8;                       // +0x08
    XNavGeomSection m_Section;      // +0x0c
    XNavNetwork *m_pNetwork;        // +0x54
    s8 m_b3D;                       // +0x58
    s8 m_bDynamic;                  // +0x59 the mesh entity is dynamic
    u8 m_Pad5a[2];
    EXVector m_vMin;                // +0x5c
    EXVector m_vMid;                // +0x6c
    EXVector m_vMax;                // +0x7c
    u16 m_wNodeInfoCount;           // +0x8c master mesh: per node zone table (low 12 bits)
    u16 *m_pNodeInfo;               // +0x90
    u16 m_wLinkInfoCount;           // +0x94 master mesh: links between sub maps
    XNavLinkInfo *m_pLinkInfo;      // +0x98
    u16 m_wGeomSections;            // +0x9c one per sub map
    XNavGeomSection *m_pGeomSections;   // +0xa0
    u16 m_wNetSections;             // +0xa4
    XNavSection *m_pNetSections;    // +0xa8
    u16 m_wZoneBase;                // +0xac grid sector << 12 (single mesh maps)
};
#endif
