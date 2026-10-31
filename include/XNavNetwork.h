#ifndef XNAVNETWORK_H
#define XNAVNETWORK_H
#include "types.h"
#include "EXVector.h"
#include "EXMemory.h"

// Game navigation network (NavNetwork.obj): nodes (one per nav-mesh polygon), links (shared
// polygon edges), sections (one per loaded sub-map) and the A* node heap.

class XNavMesh;
class XNavNode;
class XNavLink;
class EXLineSeg;
template <class T, class ARG> class EXArray;
struct XNavLinkInfo;

// Per-node link entry (sizeof 4).
struct XNavNodeLink {
    u16 m_wLinkID;  // +0
    u16 m_wEdge;    // +2
};

// Best-link table directory entry (sizeof 8).
struct XNavNodeData {
    u32 m_dwOffset;     // +0 byte offset into the nav data
    u16 m_wRange;       // +4 first range
    u16 m_wRanges;      // +6 range count
};

// Node range (sizeof 6).
struct XNavRangeData {
    u16 m_wStartNode;   // +0
    u16 m_wSize;        // +2
    u16 m_wAltNode;     // +4
};

// Working range (sizeof 8).
struct XNavRangeWorking {
    u16 m_wLo;      // +0
    u16 m_wHi;      // +2
    u16 m_wAlt;     // +4
    u16 m_w6;       // +6
};

// List of working ranges per node (sizeof 0xc). TODO name
struct XNavRangeWorkingList {
    XNavRangeWorking *m_pRanges;    // +0
    u32 m_dwCount;                  // +4
    u32 m_08;                       // +8
};

// Zone connection (sizeof 6).
struct _XNavZone_Link {
    u16 m_wType;    // +0
    u16 m_wZoneA;   // +2
    u16 m_wZoneB;   // +4
};

// Statistics block stored with the nav data (sizeof 0x40).
struct XNavStats {
    u16 m_w0;           // +0x00 node count
    u16 m_wLinks;       // +0x02 link count
    u32 m_04;           // +0x04
    u32 m_08;           // +0x08
    u32 m_0c;           // +0x0c
    EXVector m_10;      // +0x10
    EXVector m_20;      // +0x20
    EXVector m_30;      // +0x30
};

// Network node: one per nav-mesh polygon (sizeof 0x40).
class XNavNode {
public:
    __forceinline void SetRouteParent(XNavNode *p) { m_pRouteParent = p; }
    __forceinline void SetZone(u16 z) { m_wZone = z; }
    __forceinline EXVector3 &Pos() { return m_Pos; }
    __forceinline u16 NodeID() { return m_wNodeID; }
    __forceinline u16 TypeFlags() { return m_wTypeFlags; }
    __forceinline XNavNode *RouteParent() { return m_pRouteParent; }
    __forceinline void SetPos(const EXVector3 &p) { m_Pos = p; }
    __forceinline void SetNodeID(u16 id) { m_wNodeID = id; }
    __forceinline void SetFlags(u16 f) { m_wFlags = f; }
    __forceinline void SetFlag(u16 f) { m_wFlags |= f; }
    __forceinline void ClrFlag(u16 f) { m_wFlags &= ~f; }
    __forceinline void SetTypeFlags(u16 f) { m_wTypeFlags = f; }
    __forceinline void SetupLinks(XNavNodeLink *p, u16 n)
    {
        m_pLinks = p;
        m_wLinkCount = n;
    }
    __forceinline void SetRouteLink(u16 l) { m_wRouteLink = l; }
    __forceinline void SetCostToHere(float f) { m_fCostToHere = f; }
    __forceinline void SetTotalCost(float f) { m_fTotalCost = f; }
    __forceinline void SetDynamicCost(float f) { m_fDynamicCost = f; }
    __forceinline void SetNumEdges(u16 n) { m_wNumEdges = n; }
    __forceinline void SetXEdges(u16 n) { m_wXEdges = n; }
    __forceinline void SetRadius(float f) { m_fRadius = f; }
    __forceinline u16 Flags() { return m_wFlags; }
    __forceinline int LinkCount() { return m_wLinkCount; }
    __forceinline u16 GetLinkID(u16 i)
    {
        return !m_pLinks ? 0xffff : m_pLinks[i].m_wLinkID;
    }
    __forceinline u16 GetLinkEdge(u16 i)
    {
        return !m_pLinks ? 0xffff : m_pLinks[i].m_wEdge;
    }
    // Retail inlines this in some callers only: those use a file-local __forceinline copy (NavNetwork.cpp).
    inline int IndexForLinkID(u16 id)
    {
        for (int i = 0; i < LinkCount(); i++)
            if (GetLinkID((u16)i) == id) return i;
        return -1;
    }
    __forceinline XNavNode *Next() { return m_pNext; }
    __forceinline float CostToHere() { return m_fCostToHere; }
    __forceinline u16 Zone() { return m_wZone; }
    __forceinline u16 &NumEdges() { return m_wNumEdges; }
    __forceinline void OffsetLinkIDs(u16 off)
    {
        for (u16 i = 0; i < m_wLinkCount; i++) m_pLinks[i].m_wLinkID += off;
    }
    XNavNode();
    ~XNavNode();
    void AddLink(u16, int);
    void SetNext(XNavNode *p);
    u16 GetLinkCountForEdge(u16 e);
    u16 GetLinkForEdge(u16 e, u16 n);
    void GetEdgeUsage(u8 *p);
    float CostScale();
    __forceinline s8 IsSelected() { return (m_wTypeFlags >> 14) & 1; }
    __forceinline void Select(s8 b) { m_wTypeFlags = b ? (m_wTypeFlags | 0x4000) : (m_wTypeFlags & ~0x4000); }
    __forceinline float Radius() { return m_fRadius; }
    __forceinline u16 &XEdges() { return m_wXEdges; }

    EXVector3 m_Pos;            // +0x00
    float m_fRadius;            // +0x0c
    u16 m_wNodeID;              // +0x10
    u16 m_wFlags;               // +0x12 bits 0-1: heap state, 2: unvisited
    u16 m_wTypeFlags;           // +0x14 bit 14: selected
    u16 m_wLinkCount;           // +0x16
    u16 m_wRouteLink;           // +0x18
    u16 m_wZone;                // +0x1a
    u16 m_wNumEdges;            // +0x1c
    u16 m_wXEdges;              // +0x1e
    XNavNodeLink *m_pLinks;     // +0x20
    XNavNode *m_pNext;          // +0x24 heap list
    XNavNode *m_pRouteParent;   // +0x28
    float m_fDynamicCost;       // +0x2c
    float m_fCostToHere;        // +0x30
    float m_fTotalCost;         // +0x34
    u32 m_38;                   // +0x38
    u32 m_3c;                   // +0x3c
};

// Network link: a polygon edge shared by two nodes (sizeof 0x14).
class XNavLink {
public:
    __forceinline void SetLinkID(u16 id) { m_wLinkID = id; }
    __forceinline void SetNodeAB(u16 a, u16 b)
    {
        m_wNodeA = a;
        m_wNodeB = b;
    }
    __forceinline void SetEdge(u16 a, u16 b)
    {
        m_wEdgeVert[0] = a;
        m_wEdgeVert[1] = b;
    }
    __forceinline void SetLength(float f) { m_fLength = f; }
    __forceinline u16 &LinkID() { return m_wLinkID; }
    __forceinline u16 &TypeFlags() { return m_wTypeFlags; }
    __forceinline u16 &NodeA() { return m_wNodeA; }
    __forceinline u16 &NodeB() { return m_wNodeB; }
    __forceinline u16 &EdgeVert(u16 i) { return m_wEdgeVert[i]; }
    __forceinline float &Length() { return m_fLength; }
    __forceinline float Cost(u16) { return m_fLength; }
    XNavLink();
    ~XNavLink();

    u16 m_wLinkID;      // +0x00
    u16 m_wTypeFlags;   // +0x02 bit 0: removed
    u16 m_wNodeA;       // +0x04
    u16 m_wNodeB;       // +0x06
    u16 m_wEdgeVert[2]; // +0x08
    float m_fLength;    // +0x0c
    u32 m_10;           // +0x10
};

// Node/link block of one loaded sub-map; sections are chained (sizeof 0x24).
struct XNavSection {
    XNavSection *SectionFromHash(u32 hash)
    {
        if (hash == m_dwHash) return this;
        if (m_pNext) return m_pNext->SectionFromHash(hash);
        return 0;
    }
    XNavNode *Node(u16 i)
    {
        if (i >= m_wNodeOffset && i < m_wNodeOffset + m_wNodes) {
            if (!m_pNodes) return 0;
            return &m_pNodes[i - m_wNodeOffset];
        }
        if (!m_pNext) return 0;
        return m_pNext->Node(i);
    }
    void AddSection(XNavSection *s)
    {
        if (m_pNext) {
            m_pNext->AddSection(s);
        } else {
            m_pNext = s;
            s->m_pNext = 0;
        }
    }
    void SetSection(XNavSection *s)
    {
        m_wNodeOffset = s->m_wNodeOffset;
        m_wNodes = s->m_wNodes;
        m_wNodeC = s->m_wNodeC;
        m_pNodes = s->m_pNodes;
        m_wLinkOffset = s->m_wLinkOffset;
        m_wLinks = s->m_wLinks;
        m_wLink12 = s->m_wLink12;
        m_pLinks = s->m_pLinks;
    }
    u16 NodeCount(u16 n)
    {
        n += m_wNodes;
        if (m_pNext) return m_pNext->NodeCount(n);
        return n;
    }
    u16 LinkCount(u16 n)
    {
        n += m_wLinks;
        if (m_pNext) return m_pNext->LinkCount(n);
        return n;
    }
    XNavLink *Link(u16 i)
    {
        if (i >= m_wLinkOffset && i < m_wLinkOffset + m_wLinks) {
            if (!m_pLinks) return 0;
            return &m_pLinks[i - m_wLinkOffset];
        }
        if (m_pNext) return m_pNext->Link(i);
        return 0;
    }

    u32 m_dwHash;           // +0x00
    XNavNode *m_pNodes;     // +0x04
    u16 m_wNodeOffset;      // +0x08
    u16 m_wNodes;           // +0x0a
    u16 m_wNodeC;           // +0x0c TODO
    u16 m_wLinkOffset;      // +0x0e
    u16 m_wLinks;           // +0x10
    u16 m_wLink12;          // +0x12 TODO
    XNavLink *m_pLinks;     // +0x14
    XNavNodeLink *m_pNodeLinks; // +0x18 link entries of all nodes (stored after the nodes)
    u32 m_1c;               // +0x1c
    XNavSection *m_pNext;   // +0x20
};

// Open list for the A* search: nodes sorted by cost, chained through XNavNode::m_pNext.
class XNavNodeHeap {
public:
    __forceinline XNavNode *Top() { return m_Head.m_pNext; }
    XNavNodeHeap(u16 flag);
    ~XNavNodeHeap();
    void Push(XNavNode *n);
    XNavNode *Pop();
    void Add(XNavNode *n);
    void Remove(XNavNode *n);
    void Adjust(XNavNode *n);
private:
    void InsertAfter(XNavNode *n, XNavNode *after);
public:
    u16 m_wFlag;        // +0x00 flag set on member nodes
    int m_nCount;       // +0x04
    XNavNode m_Head;    // +0x08 sentinel
};

class XNavNetwork {
public:
    __forceinline u8 *NavData() { return m_pNavData; }
    __forceinline XNavNodeData *NavNodeData() { return m_pNavNodeData; }
    __forceinline XNavRangeData *NavRanges() { return m_pNavRanges; }
    __forceinline u16 NavRangesCount() { return m_wNavRanges; }
    __forceinline u16 NavZoneLinksCount() { return m_wNavZoneLinks; }
    __forceinline _XNavZone_Link *NavZoneLinks() { return m_pNavZoneLinks; }
    __forceinline void SetNavData(u8 *p) { m_pNavData = p; }
    __forceinline void SetNavNodeData(XNavNodeData *p) { m_pNavNodeData = p; }
    __forceinline void SetNavRanges(XNavRangeData *p) { m_pNavRanges = p; }
    __forceinline void SetNavRangesCount(u16 n) { m_wNavRanges = n; }
    __forceinline void SetNavZoneLinks(u16 n, _XNavZone_Link *p)
    {
        m_wNavZoneLinks = n;
        m_pNavZoneLinks = p;
    }
    __forceinline void SetNavMesh(XNavMesh *p) { m_pNavMesh = p; }
    __forceinline XNavSection &NavSection() { return m_Section; }
    __forceinline void AddSection(XNavSection *s) { m_Section.AddSection(s); }
    __forceinline void SetSection(XNavSection *s) { m_Section.SetSection(s); }
    __forceinline XNavNode *Node(u16 i) { return m_Section.Node(i); }
    __forceinline XNavNode *PolyNode(u16 i) { return m_Section.Node(m_Section.m_wNodeOffset + i); }
    __forceinline EXVector3 *NodePos(u16 i) { return &m_Section.Node(i)->m_Pos; }
    __forceinline u16 NodeTypeFlags(u16 i) { return m_Section.Node(i)->m_wTypeFlags; }
    __forceinline u16 NumZones() { return m_wNumZones; }
    __forceinline void SetNumZones(u16 n) { m_wNumZones = n; }
    __forceinline u16 &NodeOffset() { return m_Section.m_wNodeOffset; }
    __forceinline u16 &LinkOffset() { return m_Section.m_wLinkOffset; }
    __forceinline u8 *NavData(int i) { return m_pNavData + m_pNavNodeData[i].m_dwOffset; }
    __forceinline XNavRangeData *NavRanges(int i)
    {
        if (m_pNavRanges) return &m_pNavRanges[m_pNavNodeData[i].m_wRange];
        return 0;
    }
    __forceinline int NavRangesCount(int i)
    {
        if (m_pNavNodeData) return m_pNavNodeData[i].m_wRanges;
        return 0;
    }
    __forceinline XNavLink *Link(u16 i) { return m_Section.Link(i); }
    __forceinline u16 NodeCount() { return m_Section.NodeCount(0); }
    __forceinline u16 LinkCount() { return m_Section.LinkCount(0); }
    __forceinline void SetLinkLen(u16 i, float f) { m_Section.Link(i)->m_fLength = f; }
    XNavLink *NodeLinkPtr(XNavNode *n, u16 i)
    {
        if (n && i != 15) {
            u16 id = n->GetLinkID(i);
            if (id != 0xffff) return m_Section.Link(id);
        }
        return 0;
    }
    u16 LinkedNodeID(XNavNode *n, XNavLink *l)
    {
        if (l->m_wNodeA == 0xffff || l->m_wNodeB == 0xffff) return 0xffff;
        if (l->m_wNodeA == n->m_wNodeID) return l->m_wNodeB;
        return l->m_wNodeA;
    }
    __forceinline XNavNode *LinkedNode(XNavNode *n, XNavLink *l) { return m_Section.Node(LinkedNodeID(n, l)); }
    __forceinline static u16 GetRangeStartNode(void *p) { return ((XNavRangeData *)p)->m_wStartNode; }
    __forceinline static u16 GetRangeSize(void *p) { return ((XNavRangeData *)p)->m_wSize; }
    __forceinline static u16 GetRangeAltNode(void *p) { return ((XNavRangeData *)p)->m_wAltNode; }
    void DeleteNodes();
    void DeleteLinks();
    static int OldNavDataNodeSize(u16, u16 nodes, u16, u32);
    static int OldNavDataTotalSize(u16 nodes, u16, u32);
    void RemoveLink(XNavLink *l);
    void SetBestLinkOld(u8 *p, int nodes, int from, int to, int link);
    int GetBestLinkOld(u8 *p, int nodes, int from, int to);
    XNavRangeWorking *GetNavRangeWorking(int node, int from);
    int GetBestLinkTemp(int from, int to);
    void SetBestLinkRanged(int from, int to, int link);
    int GetBestLinkRanged(int from, int to);
    int GetBestNode(int from, int to, int *pLink);
    s8 DeallocateData();
    EXVector3 *PositionForNode(int i);
    float GetLinkCostRatio(const EXVector3 *a, const EXVector3 *b, const EXVector &c, const EXVector &d);
    EXVector RandomPosition(u16 zone);
    XNavNode *NearestNodeToNode(XNavNode *n);
    XNavNode *ConnectedNodePtr(XNavNode *n, u16 i, XNavLink **ppLink);
    s8 ValidateNetwork();
    s8 ZonesAreLinked(int a, int b, u16 type);
    int SetupRoute(int from, int to, float &cost, s8);
    s8 GetRoute(int from, int to, u16 *pRoute);
    static u32 GetNavDataFlags(u32 f);
    void DoSubMapOffsets(u16 polyOffset, u16 vertOffset);
    static float MakeAnglePolar(float a);
    static float AngleDifference(float a, float b);
    static EXVector NearestPointOnLineSegment(const EXVector &, const EXVector &, const EXVector &, float *);
    static EXVector NearestPointOnLineSegment3D(const EXVector &, const EXVector &, const EXVector &, float *);
    static s8 LineSegsIntersect2D2(const EXVector &, const EXVector &, const EXVector &, const EXVector &, EXVector *, float *, float *);
    static s8 LineSegsIntersect2D(const EXVector &, const EXVector &, const EXVector &, const EXVector &, EXVector *);
    static s8 LineSegsNearIntersect2D(const EXVector &, const EXVector &, const EXVector &, const EXVector &, EXVector *, float &);
    static s8 ShortestLineSegBetween2LineSegs(const EXLineSeg &, const EXLineSeg &, EXLineSeg &, float &, float &);
    void ClearNodeTypeFlags(u16 f);
    XNavLink *LinkPtrForNodes(XNavNode *a, XNavNode *b, int *pIdx)
    {
        XNavLink *l;
        for (int i = 0; i < 16; i++) {
            if (b == ConnectedNodePtr(a, (u16)i, &l)) {
                if (pIdx) *pIdx = i;
                return l;
            }
        }
        return 0;
    }
    XNavNetwork(XNavMesh *pMesh);
    ~XNavNetwork();
    s8 Loaded(u32 hash, u16 nodes, XNavNode *pNodes, u16 links, XNavLink *pLinks, u8 *pNavData, XNavStats *pStats, u32 flags,
              u16 zones, XNavNodeData *pNodeData, u16 ranges, XNavRangeData *pRanges);
    void RemoveLink(XNavNode *a, XNavNode *b);
    s8 GetFleePosition(const EXVector &, const EXVector &, float, EXVector &, int *);
    int NodeSpan(u16, const EXVector &, const EXVector &);
    void FloodNodeFlags(u16, u16, u16, u16);
    int GetNextNode(int from, int to, int *pLink);
    float CalcLinkCost(XNavLink *l, XNavNode *n, const EXVector &v);
    s8 AllocateData(u8 *p);
    s8 GetLinkPortal(XNavLink *l, u16 node, EXVector *pPortal);
    s8 GetLinkPortal(u16 a, u16 b, EXVector *pPortal);
    int NumConnectedNodes(int node, u16 flags);
    int ConnectedNode(int node, u16 i, u16 flags);
    s8 FindConnectedNode(int node, u16 other, u16 flags);
    s8 GetRoutePos(int from, int to, float dist, EXVector &pos, int &node);
    void InstallMasterLinks(XNavLinkInfo *p);
    static float DistanceToLineSegment(const EXVector &, const EXVector &, const EXVector &);
    void UnselectAll();
    int SelectFlagged(u16, u16);
    void RemoveBadLinks();
    u16 AddNode(EXVector3 &p);
    u16 AddLink(u16, u16, int, int, u16, u16, EXVector *);
    XNavNode *NearestValidNode(u16, const EXVector3 &, float, u32);
    s8 CalcAStar(int from, int to, u32 flags);
    int BuildZones();
    int GetJumpToZoneArray(int zone, EXArray<int, int> &arr);
    __forceinline void SetNavDataFlags(u32 f) { m_dwFlags = f; }
    __forceinline void SetNavDataFileFlags(u32 f) { m_dwFileFlags = f; }
private:
    void PrepareNodes(float cost);
    void SetNodeCosts(float cost);
    void ClearNodeHeapFlags();
    void EmitDistancesFromNode(u16 node, float dist);
public:
    u32 m_dwVersion;                // +0x00
    u16 m_w4;                       // +0x04
    XNavSection m_Section;          // +0x08
    u32 m_2c;                       // +0x2c
    u8 *m_pNavData;                 // +0x30
    u8 *m_pNavDataAlloc;            // +0x34
    u16 m_wNavRanges;               // +0x38
    XNavRangeData *m_pNavRanges;    // +0x3c
    XNavRangeData *m_pNavRangesAlloc;   // +0x40
    XNavNodeData *m_pNavNodeData;   // +0x44
    void *m_p48;                    // +0x48
    _XNavZone_Link *m_pNavZoneLinks;    // +0x4c
    u16 m_wNavZoneLinks;            // +0x50
    void *m_p54;                    // +0x54
    s8 **m_ppBestLinkTemp;          // +0x58
    XNavRangeWorkingList *m_pRangeWorking;  // +0x5c
    u16 m_wNumZones;                // +0x60
    u16 m_w62;                      // +0x62
    int m_64;                       // +0x64
    XNavStats m_Stats;              // +0x68
    int m_nRouteNodes;              // +0xa8
    int m_nRouteFrom;               // +0xac
    int m_nRouteTo;                 // +0xb0
    float m_fRouteCost;             // +0xb4
    u32 m_dwFlags;                  // +0xb8
    u32 m_dwFileFlags;              // +0xbc
    u32 m_c0;                       // +0xc0
    XNavMesh *m_pNavMesh;           // +0xc4
};
#endif
