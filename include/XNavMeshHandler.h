#ifndef XNAVMESHHANDLER_H
#define XNAVMESHHANDLER_H
#include "types.h"
#include "EXVector.h"
#include "EXArray.h"
#include "XNavMesh.h"

// Nav mesh manager (NavMeshHdlr.obj; header NavMeshHdlr.h in the Aug 2004 build).

class XNavLocation;

struct XNavSubMapData {
    __forceinline XNavSubMapData() {}
    u32 m_dwHashCode;   // +0x00
    u16 m_wPolyOffset;  // +0x04
    u16 m_wVertOffset;  // +0x06
    u16 m_wNodeOffset;  // +0x08
    u16 m_wLinkOffset;  // +0x0a
    u16 m_wPolyCount;   // +0x0c
    u16 m_wVertCount;   // +0x0e
    u16 m_wNodeCount;   // +0x10
    u16 m_wLinkCount;   // +0x12
    EXVector m_vBoundsMin;  // +0x14
    EXVector m_vBoundsMax;  // +0x24
    EXVector m_vBoundsMid;  // +0x34
};

class XNavSubMap {
public:
    __forceinline u32 HashCode() { return m_Data.m_dwHashCode; }
    __forceinline u16 NodeOffset() { return m_Data.m_wNodeOffset; }
    __forceinline u16 LinkOffset() { return m_Data.m_wLinkOffset; }
    __forceinline u16 PolyOffset() { return m_Data.m_wPolyOffset; }
    __forceinline u16 VertOffset() { return m_Data.m_wVertOffset; }
    __forceinline u16 NodeCount() { return m_Data.m_wNodeCount; }
    __forceinline u16 LinkCount() { return m_Data.m_wLinkCount; }
    __forceinline u16 PolyCount() { return m_Data.m_wPolyCount; }
    __forceinline u16 VertCount() { return m_Data.m_wVertCount; }
    __forceinline EXVector &BoundsMin() { return m_Data.m_vBoundsMin; }
    __forceinline EXVector &BoundsMid() { return m_Data.m_vBoundsMid; }
    __forceinline EXVector &BoundsMax() { return m_Data.m_vBoundsMax; }
    __forceinline s8 IsInit() { return m_bInit; }
    __forceinline void SetInit(s8 b) { m_bInit = b; }
    __forceinline void SetSector(u16 s) { m_wSector = s; }
    __forceinline XNavSubMapData *DataPtr() { return &m_Data; }
    __forceinline XNavSubMap() {}

    u16 m_wSector;          // +0x00
    s8 m_bInit;             // +0x02
    XNavSubMapData m_Data;  // +0x04
};

class XNavGridSector {
public:
    __forceinline void SetSubMap(XNavSubMap *p) { m_pSubMap = p; }
    __forceinline XGeoNavMeshEntity *Mesh() { return m_pMesh; }
    __forceinline void SetMesh(XGeoNavMeshEntity *p)
    {
        m_pMesh = p;
        if (p) p->SetSectorPtr(this);
    }

    XNavSubMap *m_pSubMap;          // +0x00
    XGeoNavMeshEntity *m_pMesh;     // +0x04
};

// Deferred block/open request (sizeof 0x20).
struct XNavMeshCmd {
    __forceinline XNavMeshCmd() {}
    EXVector m_vPos;    // +0x00
    u16 m_wIndex;       // +0x10
    u16 m_wType;        // +0x12 1: set blocked, 2: set open
    u8 m_14[0x20 - 0x14];
};

// Geometry file holding a nav mesh (sizeof 0xc).
struct XNavGeoFile {
    u32 m_00;                       // +0x00
    u32 m_dwHashCode;               // +0x04
    XGeoNavMeshEntity *m_pEntity;   // +0x08
};

class XNavMeshHandler {
public:
    __forceinline s8 IsDynamicNavigation() { return m_bDynamic; }
    __forceinline XNavMesh *GetPtr(u32 h)
    {
        if ((int)h > 0 && (int)h <= m_nMeshes) return &m_pMeshes[h - 1];
        return 0;
    }
    static int PolyID(int h)
    {
        int id = h & 0xffff;
        return id == 0xffff ? -1 : id;
    }
    __forceinline XNavMesh *Single() { return m_pMeshes; }
    __forceinline XNavMesh *Single3D() { return &m_pMeshes[1]; }
    __forceinline XNavMesh *Master() { return m_pMeshes; }
    __forceinline int GridSectorForPoly(int h)
    {
        int s = (u32)h >> 24;
        if (s == 0xff) return -1;
        return s;
    }
    // The mesh holding poly handle h (inline emitted in ActorTimeTable.obj). Body after XNavMesh.
    XNavMesh *GetPtr(int h);
    XNavMesh *GetPtr(const EXVector &p, s8 b3D)
    {
        switch (m_nType) {
        case 0:
            return 0;
        case 1:
            if (!b3D) {
                if (m_pMeshes[0].MeshCount()) return m_pMeshes;
                if (m_pMeshes[1].MeshCount() <= 0) return m_pMeshes;
            }
            return &m_pMeshes[1];
        case 2:
        {
            int i = GridSectorForPos(p);
            return m_pMeshes[i].SectionBase()->Mesh() ? &m_pMeshes[i] : 0;
        }
        case 3:
        {
            XNavGeomSection *s = Master()->SectionNearestPos(p);
            if (s && s->m_pMesh) return Master();
            break;
        }
        }
        return 0;
    }
    __forceinline static int MakePolyHandle(XNavMesh *, int poly, int)
    {
        return (poly & 0xffff) | 0xff000000;
    }
    __forceinline int Type() { return m_nType; }
    __forceinline void SetType(int t) { m_nType = t; }
    __forceinline void SetSubMap(int i, XNavSubMap *p) { m_Sectors[i].SetSubMap(p); }
    __forceinline static int PolyTri(int h)
    {
        int t = (u8)(h >> 16);
        if (t == 0xff) return -1;
        return t;
    }
    s8 PostLoadInit();
    int GridSectorForPos(const EXVector &p);
    s8 CalcAStar(XNavLocation &a, XNavLocation &b, u32 flags);
    s8 Init();
    s8 BootInit();
    s8 PreLoadInit();
    s8 UnloadLevel();
    s8 UpdatePtrs(XGeoNavMeshEntity *, XGeoNavMeshEntity *);
    s8 HandleMemCompaction();
    void RefreshMaps();
    void RemoveNavMesh(XGeoNavMeshEntity *);
    ~XNavMeshHandler();
    int SetUpRoute(XNavLocation &, XNavLocation &, float &, s8);
    s8 GetRoutePos(XNavLocation &, XNavLocation &, float, EXVector &, int &);
    s8 SetBlocked(const EXVector &);
    s8 SetOpen(const EXVector &);
    s8 AddNavMesh(XGeoNavMeshEntity *, u32);
    void UpdateCmds();
    s8 Update();
    XNavMeshHandler();
    __forceinline void AddZones(u16 n) { m_wZones += n; }
private:
    XNavMeshCmd *NewCmd();
    XGeoNavMeshEntity *Get(int, int);
    s8 DoCmd_SetBlocked(const EXVector &);
    s8 DoCmd_SetOpen(const EXVector &);
public:
    s8 m_b0;                                // +0x00
    s8 m_bDynamic;                          // +0x01
    u16 m_w2;                               // +0x02
    int m_nType;                            // +0x04
    int m_nGeoFiles;                        // +0x08
    XNavGeoFile m_GeoFiles[12];             // +0x0c
    int m_nMeshes;                          // +0x9c
    XNavMesh *m_pMeshes;                    // +0xa0
    EXPtrArray<XGeoNavMeshEntity> m_Entities;   // +0xa4
    EXPtrArray<XGeoNavMeshEntity> m_Entities3D; // +0xb4
    XGeoNavMeshEntity *m_pMaster;           // +0xc4
    XNavGridSector m_Sectors[12];           // +0xc8
    u16 m_wZones;                           // +0x128
    u16 m_wSubMaps;                         // +0x12a
    XNavSubMap *m_pSubMaps;                 // +0x12c
    u32 m_130;                              // +0x130
    u32 m_134;                              // +0x134
    int m_nCmds;                            // +0x138
    XNavMeshCmd *m_pCmds;                   // +0x13c
};

extern XNavMeshHandler gNavMeshHandler;

inline XNavMesh *XNavMeshHandler::GetPtr(int h)
{
    switch (m_nType) {
    case 1: {
        // The 2D mesh first, then the 3D one.
        if (m_pMeshes[0].SectionFromPolyID(PolyID(h))) return &m_pMeshes[0];
        return m_pMeshes[1].SectionFromPolyID(PolyID(h)) ? &m_pMeshes[1] : 0;
    }
    case 2: {
        int sector = GridSectorForPoly(h);
        return m_pMeshes[sector].SectionBase()->Mesh() ? &m_pMeshes[sector] : 0;
    }
    case 3: {
        XNavGeomSection *pSec = Master()->SectionBase()->SectionFromPolyID(PolyID(h));
        if (!pSec) return 0;
        return pSec->Mesh() ? Master() : 0;
    }
    }
    return 0;
}
#endif
