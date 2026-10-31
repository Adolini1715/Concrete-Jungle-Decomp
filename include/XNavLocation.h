#ifndef XNAVLOCATION_H
#define XNAVLOCATION_H
#include "types.h"
#include "EXVector.h"
#include "XNavMeshHandler.h"

// A position on the nav mesh (NavMeshHdlr.obj): world position, floor contact, polygon handle.

class XNavRoute;

class XNavLocation {
public:
    __forceinline s8 IsValid() { return m_bValid; }
    __forceinline s8 Is3D() { return m_b3D; }
    __forceinline void Ignore(u32 f) { m_dwIgnore = f; }
    __forceinline u32 GetIgnore() { return m_dwIgnore; }
    __forceinline int NavZone() { return m_sNavZone; }
    __forceinline const EXVector &Pos() { return m_vPos; }
    __forceinline const EXVector &FloorPos() { return m_vFloorPos; }
    void SnapToFloor() { m_vPos = m_vFloorPos; }  // [fi] copy in ActorLocation.obj
    __forceinline EXVector &FloorNormal() { return m_vFloorNormal; }
    __forceinline int GetPolyHandle() { return m_nPolyHandle; }
    __forceinline XNavMesh *GetMesh() { return gNavMeshHandler.GetPtr((u32)m_nMesh); }
    __forceinline int GetPolyID() { return XNavMeshHandler::PolyID(m_nPolyHandle); }
    __forceinline u16 SurfaceFlags() { return m_wSurfaceFlags; }
    // NEAR ([fi] copy in NavMesh.obj): retail keeps separate -1 stores for the null mesh / bad poly paths.
    __declspec(noinline) void SetPoly(XNavMesh *pMesh, int poly, int tri)
    {
        m_nPolyHandle = XNavMeshHandler::MakePolyHandle(pMesh, poly, tri);
        if (pMesh) {
            EXGeoNavMeshEntity_PolyList *p;
            if (poly < 0 || (p = pMesh->Poly(poly)) == 0) m_sNavZone = -1;
            else m_sNavZone = (u16)(p->m_wZone | 0x8000);
        } else {
            m_sNavZone = -1;
        }
    }
    __forceinline float MeshDistance() { return m_fMeshDistance; }
    __forceinline int GetPolyTri() { return XNavMeshHandler::PolyTri(m_nPolyHandle); }
    __forceinline u16 GeoBlockBits() { return m_wGeoBlockBits; }
    void SetPushRadius(float r) { m_70 = r; }  // appended by the ActorUtil agent (ActorUtil.obj [fi]; stores +0x70)
    void SetRot(const EXVector &r) { m_v20 = r; }  // appended by the actor agent (AIControl.obj [fi])
    // appended by the nav agent (NavRoute.obj [fi], Actor.obj [fi])
    __forceinline void SetRoute(XNavRoute *r) { m_60 = (u32)r; }
    void MoveTo(const EXVector &p, u32 *pFlags)
    {
        if (IsValid()) {
            Move(p - m_vPos, pFlags);
            m_v00 = p;
        } else {
            SetPosPoly(p, -1, 0);
        }
    }
    ~XNavLocation();
    void Reset();
    static u16 CalcBlockBits(const EXVector &);
    int NegotiateDir(const EXVector &, u16);
    void FloodPolyFlags(u16, u16, u16);
    XNavLocation();
    XNavLocation(EXVector, int, int);
    s8 Validate(s8, s8);
    void Set3D(s8);
    s8 SelectPolyByRaycast(EXVector &);
    void SetPosSlow(const EXVector &, s8);
    void SetPosPoly(const EXVector &, int, s8);
    void Move(const EXVector &, u32 *);
private:
    __forceinline void SetPoly(int h) { m_nPolyHandle = h; }
    void UpdateFloorInfo(XNavMesh *, int);
public:
    EXVector m_v00;             // +0x00
    EXVector m_vPos;            // +0x10
    EXVector m_v20;             // +0x20
    EXVector m_vFloorPos;       // +0x30
    EXVector m_vFloorNormal;    // +0x40
    EXVector m_v50;             // +0x50
    u32 m_60;                   // +0x60
    int m_nMesh;                // +0x64 1-based mesh handle
    int m_nPolyHandle;          // +0x68 poly id (16 bits), tri (8 bits), 0xff
    float m_fPushRadius;        // +0x6c
    float m_70;                 // +0x70
    float m_fMeshDistance;      // +0x74
    u32 m_dwIgnore;             // +0x78
    u16 m_wSurfaceFlags;        // +0x7c
    u16 m_wGeoBlockBits;        // +0x7e
    u16 m_wObjBlockBits;        // +0x80
    s16 m_sNavZone;             // +0x82
    u32 m_bValid : 1;           // +0x84
    u32 m_b3D : 1;
    u32 m_b2 : 1;
    u32 m_b3 : 1;               // only 4 bits (implicit operator=, AIControl.obj copy)
};
#endif
