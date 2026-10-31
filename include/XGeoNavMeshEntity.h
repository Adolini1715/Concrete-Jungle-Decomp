#ifndef XGEONAVMESHENTITY_H
#define XGEONAVMESHENTITY_H
#include "EXGeoEntity.h"
#include "EXRelPtr.h"
#include "EXVector.h"
#include "EXBounds.h"
#include "XNavNetwork.h"

// Game nav-mesh entity (XGeoEntity_NavMesh.obj): polygon nav mesh loaded with the geometry,
// with the nav network data appended.

class XNavNetwork;
class XNavGridSector;
class EXPlane;
struct XNavLinkInfo;
struct XNavSubMapData;
struct XNavMeshData;

// Vertex (sizeof 0x10).
struct EXGeoNavMeshEntity_VtxList {
    EXVector3 m_Pos;    // +0x00
    u32 m_dwRGBA;       // +0x0c
};

// Polygon header followed by up to 15 vertex ids.
struct EXGeoNavMeshEntity_PolyList {
    __forceinline void SetFlags(u16 f) { m_wFlags = f; }
    __forceinline u16 NumVerts() { return m_wNumVerts; }
    __forceinline u16 VertID(int i) { return m_wVerts[i]; }
    __forceinline u16 Flags() { return m_wFlags; }
    __forceinline u16 Zone() { return m_wZone; }

    u16 m_wFlags;       // +0x00
    u16 m_wNumVerts;    // +0x02
    u16 m_wZone;        // +0x04
    u8 m_MinVerts;      // +0x06 bounds: vertex with the smallest x (high nibble) and z (low nibble)
    u8 m_MaxVerts;      // +0x07 vertex with the largest x (high nibble) and z (low nibble)
    u16 m_wVerts[16];   // +0x08
};

struct EXGeoNavMeshEntity_TriList;

struct _XNavPolyBounds {
    __forceinline _XNavPolyBounds() {}
    float m_MinX;       // +0x00 x/z bounds
    float m_MinZ;       // +0x04
    float m_MaxX;       // +0x08
    float m_MaxZ;       // +0x0c
    float m_Plane[4];   // +0x10 polygon plane A, B, C, D
};

// Zone bounding circle (sizeof 0x40); NearestPolygon caches the x/z distance to a query point.
struct _XNavZone {
    __forceinline _XNavZone() {}
    EXVector3 m_Pos;            // +0x00 centre of the bounds
    float m_Radius;             // +0x0c x/z radius
    EXVector3 m_Min;            // +0x10 bounds of the zone's polygon vertices
    float m_Dist;               // +0x1c x/z distance from the query point to the circle
    EXVector3 m_Max;            // +0x20
    float m_DistSq;             // +0x2c
    u16 m_wPolys;               // +0x30
    u16 m_w32;
    u8 m_34[0x40 - 0x34];
};

// A spatial section of the mesh (sizeof 0x40): its bounds and the polygons inside, either a
// contiguous run (m_wFirstPoly) or, when m_wFirstPoly is 0xffff, a list.
struct _XNavBoundsSection {
    __forceinline _XNavBoundsSection() {}
    MXBoundsBox m_Bounds;       // +0x00
    EXVector m_Centre;          // +0x20
    float m_Radius;             // +0x30
    u16 m_wFirstPoly;           // +0x34
    u16 m_wPolyCount;           // +0x36
    u16 *m_pPolyList;           // +0x38
    u32 m_3c;
};

// Copy of the entity's loaded data block (+0x54..+0x1c8) passed to InitLoadedData (sizeof 0x174).
struct XNavMeshData {
    u32 m_Data[0x5d];
};

class XGeoNavMeshEntity : public EXGeoBaseEntity {
    EX_DECLARE_DYNAMIC(XGeoNavMeshEntity)
    EX_DECLARE_GEOOBJECT(XGeoNavMeshEntity)
private:
    virtual void OnCreate(EXGeoHeader *, EXGeoObjectHeader *);
    virtual void OnDelete(EXGeoHeader *, EXGeoObjectHeader *);
public:
    __forceinline u16 GridSector() { return m_wGridSector; }
    __forceinline u16 SectorFromZone(u16) { return 0; }
    __forceinline const EXVector &BoundsMid() { return m_vBoundsMid; }
    XGeoNavMeshEntity() {}
private:
    __forceinline u32 HashCode() { return m_dwHashCode; }
    __forceinline s8 Is3D() { return m_b3D; }
    __forceinline s8 IsMaster() { return m_bMaster; }
    __forceinline s8 HasData() { return ~m_bNoData & 1; }
    __forceinline s8 IsDynamic() { return m_bDynamic; }
    __forceinline XNavNetwork *NavNetwork() { return m_pNavNetwork; }
    __forceinline u32 NodeInfoCount() { return m_dwNodeInfoCount; }
    __forceinline u32 LinkInfoCount() { return m_dwLinkInfoCount; }
    __forceinline const u32 PolyCount() { return m_dwPolyCount; }
    __forceinline const u32 VertCount() { return m_dwVertCount; }
    __forceinline u16 *NodeInfo() { return m_NodeInfo; }
    __forceinline XNavLinkInfo *LinkInfo() { return m_LinkInfo; }
    __forceinline void SetID(u16 id) { m_wID = id; }
    __forceinline s8 IsSingle() { return m_bSingle; }
    __forceinline void SetSectorPtr(XNavGridSector *p) { m_pSector = p; }
    __forceinline u16 SubMaps() { return m_wSubMaps; }
    __forceinline void SetGridSector(u16 s) { m_wGridSector = s; }
    __forceinline u16 &NodeOffset() { return m_wNodeOffset; }
    __forceinline u16 &LinkOffset() { return m_wLinkOffset; }
    __forceinline u16 &PolyOffset() { return m_wPolyOffset; }
    __forceinline u16 &VertOffset() { return m_wVertOffset; }
    __forceinline XNavSubMapData *SubMapData() { return m_SubMapData; }
    __forceinline u16 PolyVertexID(int poly, int i) { return ((EXGeoNavMeshEntity_PolyList *)m_Polys)[poly].m_wVerts[i]; }
    __forceinline u16 PolyVertexCount(int poly) { return ((EXGeoNavMeshEntity_PolyList *)m_Polys)[poly].m_wNumVerts; }
    __forceinline EXVector3 &PolyVertexPos(int poly, int i)
    {
        return ((EXGeoNavMeshEntity_VtxList *)m_Verts)[((EXGeoNavMeshEntity_PolyList *)m_Polys)[poly].m_wVerts[i]].m_Pos;
    }
    __forceinline u32 GetVertexRGBA(int i) { return ((EXGeoNavMeshEntity_VtxList *)m_Verts)[i].m_dwRGBA; }
    __forceinline void SetVertexRGBA(int i, u32 c) { ((EXGeoNavMeshEntity_VtxList *)m_Verts)[i].m_dwRGBA = c; }
    EXGeoNavMeshEntity_PolyList &Poly(int i) { return ((EXGeoNavMeshEntity_PolyList *)m_Polys)[i]; }
    __forceinline EXVector3 &VertexPos(int i) { return ((EXGeoNavMeshEntity_VtxList *)m_Verts)[i].m_Pos; }

    __forceinline _XNavPolyBounds *PolyBounds() { return m_pPolyBounds; }
    __forceinline _XNavBoundsSection *BoundsSections() { return m_pBoundsSections; }
    __forceinline _XNavPolyBounds *PolyBounds(int i) { return &m_pPolyBounds[i]; }
    void DoBackup();
    void DoRestore();
    s8 DoCheck();
    s8 PostLoadInit();
    s8 PolyPlane(int, EXPlane &);
    EXVector RandomPosition(u16);
    s8 ValidateMesh();
    void TrisFromPoly(int, EXGeoNavMeshEntity_PolyList *, int &, EXGeoNavMeshEntity_TriList *);
    void DoSubMapOffsets();
    s8 UpdatePtrs();
    s8 PolyVerts(int, EXVector *, int *);
    s8 PolyRGBAs(int, u32 *, int *);
    s8 PolyNormal(int, EXVector &);
    float GetHeight(const EXVector &, int, int, EXVector *);
    s8 PointInPolygon(const EXVector &, int);
    s8 PointInPolygon3D(const EXVector &, int);
    s8 LocatePolygon(const EXVector &, int *, int *, u32);
    s8 LocatePolygon3D(const EXVector &, int *, int *, u32);
    s8 LocatePolygon3DRay(const EXVector &, const EXVector &, int *, u32, float *, EXVector *);
    float SquaredDistanceToPolygon(int, const EXVector &, EXVector *);
    int NearestPolygon(const EXVector &, EXVector *, u32);
    int NearestPolygon3D(const EXVector &, EXVector *, u32);
    s8 SnapToPolygon(EXVector &, int);
    s8 SnapToPolygonPlane(EXVector &, int);
    s8 SnapToPolygonBounds(EXVector &, int);
    void SetMeshLighting();
    s8 GetLighting(const EXVector &, int, float &, float &, float &);
    s8 ClampPointToPoly(int, const EXVector &, EXVector &);
    s8 PosIsValid(const EXVector &, int &);
    s8 ValidatePos(const EXVector &, u32, EXVector &, int *);
    s8 LineExitsPoly(const EXVector &, const EXVector &, int, int &, int &, EXVector &, EXVector &, EXVector &, int);
    s8 LineExitsPolyIE(const EXVector &, const EXVector &, int, int &, int &, EXVector &, EXVector &, EXVector &, int);
    s8 LineExitsPoly3D(const EXVector &, const EXVector &, int, int &, EXVector &, EXVector &, EXVector &);
    s8 ValidateLine(const EXVector &, const EXVector &, int, EXVector &, int &);
    s8 InitLoadedData(XNavMeshData *);
    float GetFloorHeight(const EXVector &);
    s8 ValidatePos(const EXVector &, EXVector &, int *);

    friend struct XNavGeomSection;
    friend class XNavMesh;
    friend class XNavMeshHandler;
    friend class XNavNetwork;
    friend class XNavGridSector;
    friend class XNavLocation;
    friend class XNavRoute;
public:
    u32 m_dwVersion;                                // +0x54
    ex::EXRelPtr<EXGeoNavMeshEntity_PolyList> m_Polys;  // +0x58
    ex::EXRelPtr<EXGeoNavMeshEntity_VtxList> m_Verts;   // +0x5c
    ex::EXRelPtr<XNavNode> m_Nodes;                 // +0x60
    ex::EXRelPtr<XNavLink> m_Links;                 // +0x64
    ex::EXRelPtr<u8> m_NavData;                     // +0x68
    ex::EXRelPtr<XNavNodeData> m_NodeData;          // +0x6c
    ex::EXRelPtr<XNavRangeData> m_RangeData;        // +0x70
    ex::EXRelPtr<XNavSubMapData> m_SubMapData;      // +0x74
    ex::EXRelPtr<XNavLinkInfo> m_LinkInfo;          // +0x78
    ex::EXRelPtr<u16> m_NodeInfo;                   // +0x7c
    ex::EXRelPtr<_XNavPolyBounds> m_PolyBounds;     // +0x80
    ex::EXRelPtr<_XNavZone> m_Zones;                // +0x84
    ex::EXRelPtr<_XNavBoundsSection> m_BoundsSections;  // +0x88
    u8 m_8c[0x98 - 0x8c];
    u32 m_dwHashCode;                               // +0x98
    u32 m_dwFlags0 : 24;                            // +0x9c
    u32 m_bDynamic : 1;
    u32 m_b25 : 1;
    u32 m_b26 : 1;
    u32 m_bMaster : 1;
    u32 m_bNoData : 1;
    u32 m_b29 : 1;
    u32 m_bSingle : 1;
    u32 m_b3D : 1;
    EXVector m_vBoundsMin;                          // +0xa0
    EXVector m_vBoundsMid;                          // +0xb0
    EXVector m_vBoundsMax;                          // +0xc0
    u32 m_dwPolyCount;                              // +0xd0
    u32 m_dwVertCount;                              // +0xd4
    u32 m_d8;                                       // +0xd8
    u32 m_dwLinkInfoCount;                          // +0xdc
    u32 m_dwNodeInfoCount;                          // +0xe0
    u8 m_e4[0xf0 - 0xe4];
    XNavNetwork *m_pNavNetwork;                     // +0xf0
    XNavGridSector *m_pSector;                      // +0xf4
    u32 m_f8;                                       // +0xf8
    _XNavZone *m_pZones;                            // +0xfc
    _XNavPolyBounds *m_pPolyBounds;                 // +0x100
    _XNavBoundsSection *m_pBoundsSections;          // +0x104
    u8 m_108[0x110 - 0x108];
    u16 m_wID;                                      // +0x110
    u16 m_wZones;                                   // +0x112
    u16 m_wNodes;                                   // +0x114
    u16 m_wLinks;                                   // +0x116
    u16 m_wRanges;                                  // +0x118
    u16 m_wSubMaps;                                 // +0x11a
    u16 m_w11c;                                     // +0x11c
    u16 m_wNodeOffset;                              // +0x11e
    u16 m_wLinkOffset;                              // +0x120
    u16 m_wPolyOffset;                              // +0x122
    u16 m_wVertOffset;                              // +0x124
    u16 m_w126;                                     // +0x126
    u16 m_w128;                                     // +0x128
    u16 m_wGridSector;                              // +0x12a
    u16 m_wBoundsSections;                          // +0x12c
    u8 m_12e[0x1b8 - 0x12e];
    _XNavPolyBounds *m_pPolyBoundsAlloc;            // +0x1b8
    _XNavBoundsSection *m_pBoundsSectionsAlloc;     // +0x1bc
    _XNavZone *m_pZonesAlloc;                       // +0x1c0
    u32 m_1c4;                                      // +0x1c4
};
extern EXVector gZeroVector;
#endif
