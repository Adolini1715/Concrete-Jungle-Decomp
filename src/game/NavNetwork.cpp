#include "XNavNetwork.h"
#include "XNavMesh.h"
#include "EXMemory.h"
#include "FEText.h"
#include "EXRand.h"
#include "EXArray.h"
#include "EXLine.h"

void *_EXRealloc(void *p, u32 size);

// XNavNode::IndexForLinkID as retail inlines it into some callers.
static __forceinline int IndexForLinkIDInl(XNavNode *n, u16 id)
{
    for (int i = 0; i < n->LinkCount(); i++)
        if (n->GetLinkID((u16)i) == id) return i;
    return -1;
}

void XNavNetwork::DeleteNodes() {}

void XNavNetwork::DeleteLinks() {}

int XNavNetwork::OldNavDataNodeSize(u16, u16 nodes, u16, u32) { return (nodes >> 1) + 1; }

int XNavNetwork::OldNavDataTotalSize(u16 nodes, u16, u32) { return ((nodes >> 1) + 1) * nodes; }

void XNavNetwork::RemoveLink(XNavLink *l) { l->TypeFlags() |= 1; }

void XNavNetwork::PrepareNodes(float cost)
{
    for (int i = 0; i < NodeCount(); i++) {
        XNavNode *n = Node((u16)i);
        if (n) {
            n->SetCostToHere(cost);
            n->SetTotalCost(cost);
            n->SetFlags((n->Flags() & 0xfffc) | 4);
        }
    }
}

void XNavNetwork::SetNodeCosts(float cost)
{
    for (int i = 0; i < NodeCount(); i++) {
        XNavNode *n = Node((u16)i);
        if (n) {
            n->SetCostToHere(cost);
            n->SetTotalCost(cost);
        }
    }
}

void XNavNetwork::SetBestLinkOld(u8 *p, int nodes, int from, int to, int link)
{
    u8 *b = &p[((nodes / 2) + 1) * from + (to >> 1)];
    int shift = (to & 1) << 2;
    *b = (*b & ~(0xf << shift)) | ((link & 0xf) << shift);
}

int XNavNetwork::GetBestLinkOld(u8 *p, int nodes, int from, int to)
{
    return (p[((nodes / 2) + 1) * from + (to >> 1)] & (u8)(0xf << ((to & 1) << 2))) >> (u8)((to & 1) << 2);
}

XNavRangeWorking *XNavNetwork::GetNavRangeWorking(int node, int from)
{
    XNavRangeWorkingList *l = &m_pRangeWorking[from];
    XNavRangeWorking *r = l->m_pRanges;
    for (u32 i = 0; i < l->m_dwCount; r++, i++) {
        if (node >= r->m_wLo && node <= r->m_wHi) return r;
    }
    return 0;
}

int XNavNetwork::GetBestLinkTemp(int from, int to)
{
restart:
    XNavRangeWorkingList *l = &m_pRangeWorking[to];
    XNavRangeWorking *r = l->m_pRanges;
    for (u32 i = 0; i < l->m_dwCount; r++, i++) {
        if (from >= r->m_wLo && from <= r->m_wHi) {
            to = r->m_wAlt;
            goto restart;
        }
    }
    return m_ppBestLinkTemp[to][from];
}

// NEAR: 1 byte differs, register copy source for the shift (mov ecx, edi vs eax).
void XNavNetwork::SetBestLinkRanged(int from, int to, int link)
{
    u8 *b = m_pNavData + (m_pNavNodeData[to].m_dwOffset + (from >> 1));
    int shift = (from & 1) << 2;
    *b = (*b & ~(0xf << shift)) | ((link & 0xf) << shift);
}

int XNavNetwork::GetBestLinkRanged(int from, int to)
{
    int idx;
restart:
    XNavRangeData *r = NavRanges(to);
    idx = from;
    for (int i = 0; i < NavRangesCount(to); i++) {
        u16 start = GetRangeStartNode(r);
        u16 size = GetRangeSize(r);
        u16 alt = GetRangeAltNode(r);
        if (from < start) break;
        if (from < start + size) {
            to = alt;
            goto restart;
        }
        idx -= size;
        r++;
    }
    return (NavData(to)[idx >> 1] & (u8)(0xf << ((idx & 1) << 2))) >> (u8)((idx & 1) << 2);
}

int XNavNetwork::GetBestNode(int from, int to, int *pLink)
{
    if (pLink) *pLink = -1;
    if (from == to) return from;
    int link = GetBestLinkRanged(from, to);
    if (link < 0 || link == 15) return -1;
    XNavLink *l = NodeLinkPtr(Node((u16)from), (u16)link);
    if (!l || (l->m_wTypeFlags & 1)) return -1;
    u16 id = LinkedNodeID(Node((u16)from), l);
    if (pLink) *pLink = link;
    return id;
}

s8 XNavNetwork::DeallocateData() { return 1; }

EXVector3 *XNavNetwork::PositionForNode(int i) { return NodePos((u16)i); }

float XNavNetwork::GetLinkCostRatio(const EXVector3 *a, const EXVector3 *b, const EXVector &c, const EXVector &d)
{
    float dx = d.x - c.x;
    float dz = d.z - c.z;
    float denom = (b->x - a->x) * dz - (b->z - a->z) * dx;
    if ((float)fabs(denom) > 1.1754944e-37f) {
        float t = ((a->z - c.z) * dx - (a->x - c.x) * dz) / denom;
        if (t < 0.0f) t = 0.0f;
        else if (t > 1.0f) t = 1.0f;
        return t;
    }
    return 0.5f;
}

// NEAR: first Node() lookup inlined with separate null returns in retail.
EXVector XNavNetwork::RandomPosition(u16 zone)
{
    zone &= 0xfff;
    u16 i;
    do {
        i = (u16)((EXRand() & 0x7fff) % NodeCount());
    } while ((m_Section.Node(i)->m_wTypeFlags & 8) || (zone != 0xfff && Node(i)->m_wZone != zone) || i == 0xffff);
    EXVector3 &p = Node(i)->m_Pos;
    return EXVector(p.x, p.y, p.z, 0.0f);
}

XNavNode *XNavNetwork::NearestNodeToNode(XNavNode *n)
{
    u16 bestLink = 0xffff;
    float best = 10000000.0f;
    for (u16 i = 0; i < n->m_wLinkCount; i++) {
        u16 id = n->GetLinkID(i);
        if (id != 0xffff && best > Link(id)->Length()) {
            bestLink = id;
            best = Link(id)->Length();
        }
    }
    if (bestLink == 0xffff) return 0;
    return LinkedNode(n, Link(bestLink));
}

XNavNode *XNavNetwork::ConnectedNodePtr(XNavNode *pNode, u16 i, XNavLink **ppLink)
{
    if (i >= pNode->m_wLinkCount || !pNode->m_pLinks) return 0;
    u32 li = pNode->m_pLinks[i].m_wLinkID;
    if ((u16)li == 0xffff) return 0;
    XNavLink *l = m_Section.Link((u16)li);
    *ppLink = l;
    if (l->m_wNodeA == 0xffff || l->m_wNodeB == 0xffff) return 0;
    if (l->m_wNodeA == pNode->m_wNodeID) return m_Section.Node(l->m_wNodeB);
    return m_Section.Node(l->m_wNodeA);
}

// NEAR: 9 bytes differ, this/count swap ebx/ebp.
s8 XNavNetwork::ValidateNetwork()
{
    for (int i = 0; i < NodeCount(); i++) {
        for (int j = 0; j < NodeCount(); j++) {
            if (i == j) continue;
            int next = i;
            int count = 0;
            int n;
            do {
                int link;
                n = next;
                next = GetBestNode(n, j, &link);
                if (next == j || next < 0) break;
                if (n == next) return 0;
                if (count++ > 10000) return 0;
            } while (n != j);
        }
    }
    return 1;
}

s8 XNavNetwork::ZonesAreLinked(int a, int b, u16 type)
{
    if (!m_pNavZoneLinks || !m_wNavZoneLinks) return 0;
    a &= 0xfff;
    b &= 0xfff;
    _XNavZone_Link *l = m_pNavZoneLinks;
    for (int i = 0; i < m_wNavZoneLinks; i++, l++) {
        if (l->m_wType == type && l->m_wZoneA == a && l->m_wZoneB == b) return 1;
        if (l->m_wZoneA == b && l->m_wZoneB == a) return 1;
    }
    return 0;
}

int XNavNetwork::SetupRoute(int from, int to, float &cost, s8)
{
    if (from < 0 || from >= NodeCount() || to < 0 || to >= NodeCount()) return -1;
    m_nRouteTo = to;
    m_nRouteNodes = -1;
    m_nRouteFrom = from;
    m_fRouteCost = 0.0f;
    cost = 0.0f;
    Node((u16)from)->m_pRouteParent = 0;
    int link = 0;
    int count = 0;
    int cur = from;
    while (cur != to) {
        int next = GetBestNode(cur, to, &link);
        if (next < 0) return -1;
        Node((u16)next)->m_pRouteParent = Node((u16)cur);
        link = Node((u16)cur)->GetLinkID((u16)link);
        cost += Link((u16)link)->Cost(0);
        count++;
        cur = next;
    }
    m_nRouteNodes = count;
    m_fRouteCost = cost;
    return count;
}

s8 XNavNetwork::GetRoute(int from, int to, u16 *pRoute)
{
    if (from == m_nRouteFrom && to == m_nRouteTo) {
        if (!pRoute) return 0;
        XNavNode *n = Node((u16)to);
        int i = m_nRouteNodes;
        while (n) {
            pRoute[--i] = n->m_wNodeID;
            n = n->m_pRouteParent;
        }
        return 1;
    }
    return 0;
}

u32 XNavNetwork::GetNavDataFlags(u32 f)
{
    return ((((f >> 1) & 0x10000000) | (f & 0x8000000)) >> 1 | (f & 0x10000000)) >> 26;
}

// NEAR: 8 bytes differ, base/index order in the link field adds.
void XNavNetwork::DoSubMapOffsets(u16 polyOffset, u16 vertOffset)
{
    u16 linkOff = m_Section.m_wLinkOffset;
u16 nodeOff = m_Section.m_wNodeOffset;
    if (0 != nodeOff || linkOff) {
        for (u16 i = 0; (int)i < m_Section.m_wNodes; ++i) {
            XNavNode *n = &m_Section.m_pNodes[i];
            n->m_wNodeID += nodeOff;
            n->OffsetLinkIDs(linkOff);
        }
    }
    if ((nodeOff != 0 || linkOff || vertOffset) == 0) return;
for (u16 i = 0; i < m_Section.m_wLinks; i++) {
            m_Section.m_pLinks[i].LinkID() += linkOff;
            m_Section.m_pLinks[i].NodeA() += nodeOff;
            m_Section.m_pLinks[i].NodeB() += nodeOff;
            m_Section.m_pLinks[i].EdgeVert(0) += vertOffset;
            m_Section.m_pLinks[i].EdgeVert(1) += vertOffset;
        }
}

float XNavNetwork::MakeAnglePolar(float a)
{
    if (a >= 0.0f && a < 6.2831855f) return a;
    float r = (float)fmod(a, 6.2831855f);
    if (r >= 0.0f) return r;
    return r + 6.2831855f;
}

float XNavNetwork::AngleDifference(float a, float b)
{
    float pb = MakeAnglePolar(b);
    float pa = MakeAnglePolar(a);
    float d = pb - pa;
    if ((float)fabs(d) < 3.1415927f) return d;
    if (d < 0.0f) return d + 6.2831855f;
    return d - 6.2831855f;
}

XNavNode::XNavNode()
{
    m_Pos.x = 0.0f;
    m_Pos.y = 0.0f;
    m_Pos.z = 0.0f;
    m_fRadius = 0.0f;
    m_wNodeID = 0xffff;
    m_wFlags = 0;
    m_wTypeFlags = 0;
    m_wLinkCount = 0;
    m_pLinks = 0;
    m_wRouteLink = 0xffff;
    m_wZone = 0xffff;
    m_wNumEdges = 0xffff;
    m_wXEdges = 0xffff;
    m_pNext = 0;
    m_pRouteParent = 0;
    m_fCostToHere = 0.0f;
    m_fTotalCost = 0.0f;
    m_fDynamicCost = 0.0f;
}

XNavNode::~XNavNode() {}

void XNavNode::AddLink(u16, int) {}

void XNavNode::SetNext(XNavNode *p) { m_pNext = p; }

u16 XNavNode::GetLinkCountForEdge(u16 e)
{
    u16 n = 0;
    for (int i = 0; i < LinkCount(); i++)
        if (GetLinkEdge((u16)i) == e) n++;
    return n;
}

u16 XNavNode::GetLinkForEdge(u16 e, u16 n)
{
    for (int i = 0; i < LinkCount(); i++) {
        if (GetLinkEdge((u16)i) == e) {
            if (!n) return GetLinkID((u16)i);
            n--;
        }
    }
    return 0xffff;
}

void XNavNode::GetEdgeUsage(u8 *p)
{
    memset(p, 0, 16);
    int n = LinkCount();
    for (int i = 0; i < n; i++) {
        u16 e = GetLinkEdge((u16)i);
        p[e] = 1;
    }
}

float XNavNode::CostScale()
{
    u16 f = m_wTypeFlags;
    if (f & 3) return 51.0f;
    if (f & 4) return 5.0f;
    if (f & 8) return 1001.0f;
    return 1.0f;
}

XNavLink::XNavLink()
{
    m_wLinkID = 0xffff;
    m_wTypeFlags = 0;
    m_wNodeA = 0xffff;
    m_wNodeB = 0xffff;
    m_wEdgeVert[0] = 0xffff;
    m_wEdgeVert[1] = 0xffff;
    m_fLength = 0.0f;
}

XNavLink::~XNavLink() {}

XNavNodeHeap::XNavNodeHeap(u16 flag)
{
    m_wFlag = flag;
    m_Head.SetNext(0);
    m_nCount = 0;
}

XNavNodeHeap::~XNavNodeHeap() {}

void XNavNodeHeap::InsertAfter(XNavNode *n, XNavNode *after)
{
    XNavNode *next = after->m_pNext;
    after->m_pNext = n;
    n->m_pNext = next;
    n->SetFlag(m_wFlag);
    m_nCount++;
}

void XNavNodeHeap::Push(XNavNode *n)
{
    XNavNode *p = m_Head.m_pNext;
    XNavNode *prev = &m_Head;
    for (; p; p = p->m_pNext) {
        if (p->m_fCostToHere > n->m_fCostToHere) {
            InsertAfter(n, prev);
            return;
        }
        prev = p;
    }
    InsertAfter(n, prev);
}

XNavNode *XNavNodeHeap::Pop()
{
    XNavNode *n = m_Head.m_pNext;
    if (n) {
        m_Head.m_pNext = n->m_pNext;
        n->m_pNext = 0;
        n->ClrFlag(m_wFlag);
        m_nCount--;
    }
    return n;
}

void XNavNodeHeap::Add(XNavNode *n)
{
    XNavNode *next = m_Head.m_pNext;
    m_Head.m_pNext = n;
    n->m_pNext = next;
    n->m_wFlags |= m_wFlag;
    m_nCount++;
}

void XNavNodeHeap::Remove(XNavNode *n)
{
    XNavNode *p = m_Head.m_pNext;
    XNavNode *prev = &m_Head;
    for (; p; p = p->m_pNext) {
        if (p == n) {
            prev->m_pNext = n->m_pNext;
            n->m_pNext = 0;
            n->m_wFlags &= ~m_wFlag;
            m_nCount--;
            return;
        }
        prev = p;
    }
}

void XNavNodeHeap::Adjust(XNavNode *n)
{
    Remove(n);
    Push(n);
}

void XNavNetwork::ClearNodeTypeFlags(u16 f)
{
    for (int i = 0; i < NodeCount(); i++) {
        XNavNode *n = Node((u16)i);
        n->m_wTypeFlags &= ~f;
    }
}

XNavNetwork::XNavNetwork(XNavMesh *pMesh)
{
    m_dwVersion = 0;
    m_w4 = 0;
    memset(&m_Section, 0, sizeof(m_Section));
    m_2c = 0;
    m_pNavData = 0;
    m_pNavDataAlloc = 0;
    m_wNavRanges = 0;
    m_pNavRanges = 0;
    m_pNavRangesAlloc = 0;
    m_pNavNodeData = 0;
    m_p48 = 0;
    m_ppBestLinkTemp = 0;
    m_pRangeWorking = 0;
    SetNavZoneLinks(0, 0);
    m_p54 = 0;
    m_wNumZones = 0;
    m_w62 = 0;
    m_64 = -1;
    memset(&m_Stats, 0, sizeof(m_Stats));
    m_nRouteNodes = 0;
    m_nRouteFrom = 0;
    m_nRouteTo = 0;
    m_dwFlags = 0;
    m_dwFileFlags = 0;
    m_c0 = 0;
    m_pNavMesh = pMesh;
    m_fRouteCost = 0.0f;
}

XNavNetwork::~XNavNetwork()
{
    if (m_pNavRangesAlloc) {
        EXFree(m_pNavRangesAlloc);
        m_pNavRangesAlloc = 0;
    }
    if (m_p48) {
        EXFree(m_p48);
        m_p48 = 0;
    }
    if (m_pNavDataAlloc) {
        EXFree(m_pNavDataAlloc);
        m_pNavDataAlloc = 0;
    }
    if (m_p54) {
        EXFree(m_p54);
        m_p54 = 0;
    }
}

// Node layout of old nav data (sizeof 0x70), converted in place by Loaded().
struct XNavNodeOld {
    EXVector3 m_Pos;            // +0x00
    float m_fRadius;            // +0x0c
    u16 m_wNodeID;              // +0x10
    u16 m_wFlags;               // +0x12
    u16 m_wTypeFlags;           // +0x14
    u16 m_wLinkCount;           // +0x16
    u16 m_wLinkIDs[16];         // +0x18
    u16 m_wEdges[16];           // +0x38
    u16 m_wRouteLink;           // +0x58
    u16 m_wZone;                // +0x5a
    u16 m_wNumEdges;            // +0x5c
    u16 m_wXEdges;              // +0x5e
    XNavNode *m_pNext;          // +0x60
    XNavNode *m_pRouteParent;   // +0x64
    float m_fCostToHere;        // +0x68
    float m_fTotalCost;         // +0x6c
};

// NEAR: strength-reduced pointer biases and register choice in the conversion loops differ.
s8 XNavNetwork::Loaded(u32 hash, u16 nodes, XNavNode *pNodes, u16 links, XNavLink *pLinks, u8 *pNavData, XNavStats *pStats,
                       u32 flags, u16 zones, XNavNodeData *pNodeData, u16 ranges, XNavRangeData *pRanges)
{
    m_dwVersion = flags;
    if (pStats) m_Stats = *pStats;
    m_Stats.m_wLinks = links;
    m_Section.m_wLinks = links;
    m_Section.m_pLinks = pLinks;
    m_Section.m_dwHash = hash;
    m_pNavNodeData = pNodeData;
    m_pNavData = pNavData;
    m_wNavRanges = ranges;
    m_pNavRanges = pRanges;
    m_Stats.m_w0 = nodes;
    m_Section.m_wNodes = nodes;
    m_Section.m_pNodes = pNodes;
    m_w62 = zones;
    if (!m_Section.m_pNodeLinks) {
        XNavNodeOld *old = (XNavNodeOld *)pNodes;
        int total = 0;
        for (int i = 0; i < m_Section.m_wNodes; i++) total += old[i].m_wLinkCount;
        if (total <= 0) {
            m_Section.m_pNodeLinks = 0;
        } else {
            u32 size = total * sizeof(XNavNodeLink);
            XNavNodeLink *tmp = (XNavNodeLink *)_EXAlloc(size, 0);
            m_Section.m_pNodeLinks = tmp;
            int base = 0;
            for (int i = 0; i < m_Section.m_wNodes; i++) {
                XNavNodeOld *o = &old[i];
                XNavNode *n = &pNodes[i];
                o->m_wFlags &= 0x0fff;
                XNavNodeLink *l = &tmp[base];
                XNavNodeLink *d = l;
                for (int j = 0; j < o->m_wLinkCount; j++, d++) {
                    d->m_wLinkID = o->m_wLinkIDs[j];
                    d->m_wEdge = o->m_wEdges[j];
                }
                base += o->m_wLinkCount;
                n->m_Pos.x = o->m_Pos.x;
                n->m_Pos.y = o->m_Pos.y;
                n->m_Pos.z = o->m_Pos.z;
                n->SetRadius(o->m_fRadius);
                n->SetNodeID(o->m_wNodeID);
                n->SetFlags(o->m_wFlags);
                n->SetTypeFlags(o->m_wTypeFlags);
                n->SetupLinks(l, o->m_wLinkCount);
                n->SetRouteLink(o->m_wRouteLink);
                n->SetZone(o->m_wZone);
                n->SetNumEdges(o->m_wNumEdges);
                n->SetXEdges(o->m_wXEdges);
                n->m_pNext = o->m_pNext;
                n->SetRouteParent(o->m_pRouteParent);
                n->SetCostToHere(o->m_fCostToHere);
                n->SetTotalCost(o->m_fTotalCost);
                n->SetDynamicCost(0.0f);
            }
            tmp = m_Section.m_pNodeLinks;
            m_Section.m_pNodeLinks = (XNavNodeLink *)&m_Section.m_pNodes[m_Section.m_wNodes];
            memcpy(m_Section.m_pNodeLinks, tmp, size);
            if (tmp) EXFree(tmp);
        }
    }
    if (m_Section.m_pNodeLinks) {
        XNavNode *n = m_Section.m_pNodes;
        XNavNodeLink *p = (XNavNodeLink *)&n[m_Section.m_wNodes];
        m_Section.m_pNodeLinks = p;
        int off = 0;
        for (int i = 0; i < m_Section.m_wNodes; i++) {
            n[i].m_pLinks = &p[off];
            off += n[i].m_wLinkCount;
        }
    }
    if (m_dwVersion < 0x12 && m_wNavRanges) {
        m_pNavRangesAlloc = (XNavRangeData *)_EXAlloc(m_wNavRanges * sizeof(XNavRangeData), 0);
        u32 *src = (u32 *)m_pNavRanges;
        for (int i = 0; i < m_wNavRanges; i++) {
            m_pNavRangesAlloc[i].m_wStartNode = (u16)((src[i] >> 20) & 0x3ff);
            m_pNavRangesAlloc[i].m_wSize = (u16)((src[i] >> 10) & 0x3ff);
            m_pNavRangesAlloc[i].m_wAltNode = (u16)(src[i] & 0x3ff);
        }
        m_pNavRanges = m_pNavRangesAlloc;
    }
    return 1;
}

void XNavNetwork::RemoveLink(XNavNode *a, XNavNode *b) { RemoveLink(LinkPtrForNodes(a, b, 0)); }

void XNavNetwork::ClearNodeHeapFlags()
{
    for (int i = 0; i < NodeCount(); i++) {
        XNavNode *n = Node((u16)i);
        if (n) {
            n->m_wRouteLink = 0xff;
            n->m_pRouteParent = 0;
            n->m_pNext = 0;
            n->SetFlags((n->Flags() & 0xfffc) | 4);
        }
    }
}

void XNavNetwork::FloodNodeFlags(u16 node, u16 mask, u16 clr, u16 set)
{
    set &= clr;
    ClearNodeHeapFlags();
    SetNodeCosts(32000000.0f);
    XNavNodeHeap open(1);
    XNavNodeHeap closed(2);
    XNavNode *start = Node(node);
    EXGeoNavMeshEntity_PolyList *poly = m_pNavMesh->Poly(node);
    if (!poly) return;
    start->SetCostToHere(0.0f);
    start->SetTotalCost(0.0f);
    if (mask & (start->TypeFlags() == 0)) return;
    u16 f = (start->TypeFlags() & ~clr) | set;
    start->SetTypeFlags(f);
    poly->SetFlags(f);
    open.Push(start);
    while (open.Top()) {
        XNavNode *cur = open.Pop();
        for (int i = 0; i < cur->LinkCount(); i++) {
            XNavLink *l = NodeLinkPtr(cur, (u16)i);
            if (!l || (l->m_wTypeFlags & 1)) continue;
            XNavNode *n = LinkedNode(cur, l);
            if (n->Flags() & 3) continue;
            EXGeoNavMeshEntity_PolyList *p = m_pNavMesh->Poly(n->NodeID());
            if (!p) continue;
            u16 tf = n->TypeFlags();
            if (!(tf & mask)) continue;
            u16 nf = (tf & ~clr) | set;
            n->SetTypeFlags(nf);
            p->SetFlags(nf);
            n->SetRouteParent(cur);
            n->SetRouteLink((u16)n->IndexForLinkID(l->m_wLinkID));
            open.Push(n);
        }
        closed.Add(cur);
    }
}

int XNavNetwork::GetNextNode(int from, int to, int *pLink)
{
    if (pLink) *pLink = -1;
    if (from == to) return from;
    XNavNode *n = Node((u16)from);
    XNavNode *next = n->Next();
    int idx = -1;
    if (!LinkPtrForNodes(n, next, &idx)) return -1;
    if (pLink) *pLink = idx;
    return next->m_wNodeID;
}

float XNavNetwork::CalcLinkCost(XNavLink *l, XNavNode *n, const EXVector &v)
{
    float scale = n->CostScale();
    EXVector p(n->m_Pos.x, n->m_Pos.y, n->m_Pos.z, 0.0f);
    float len = l->m_fLength;
    EXVector d = v - p;
    return d.Length() + len * scale;
}

s8 XNavNetwork::AllocateData(u8 *) { return 1; }

s8 XNavNetwork::GetLinkPortal(XNavLink *l, u16 node, EXVector *pPortal)
{
    const EXVector3 *a;
    const EXVector3 *b;
    if (l->m_wNodeA == node) {
        a = m_pNavMesh->VertexPos(l->m_wEdgeVert[0]);
        b = m_pNavMesh->VertexPos(l->m_wEdgeVert[1]);
    } else if (l->m_wNodeB == node) {
        a = m_pNavMesh->VertexPos(l->m_wEdgeVert[1]);
        b = m_pNavMesh->VertexPos(l->m_wEdgeVert[0]);
    } else {
        return 0;
    }
    if (!a || !b) return 0;
    pPortal[0] = EXVector(a->x, a->y, a->z, 0.0f);
    pPortal[1] = EXVector(b->x, b->y, b->z, 0.0f);
    return 1;
}

s8 XNavNetwork::GetLinkPortal(u16 node, u16 link, EXVector *pPortal)
{
    if (link == 0xffff) return 0;
    u16 id = Node(node)->GetLinkID(link);
    if (id == 0xffff) return 0;
    XNavLink *l = Link(id);
    return GetLinkPortal(l, node, pPortal);
}

int XNavNetwork::NumConnectedNodes(int node, u16 e) { return Node((u16)node)->GetLinkCountForEdge(e); }

int XNavNetwork::ConnectedNode(int node, u16 e, u16 n)
{
    XNavNode *p = Node((u16)node);
    u16 l = p->GetLinkForEdge(e, n);
    if (l == 0xffff) return -1;
    XNavNode *o = LinkedNode(p, Link(l));
    if (!o) return -1;
    return o->m_wNodeID;
}

s8 XNavNetwork::FindConnectedNode(int node, u16 e, u16 other)
{
    XNavNode *p = Node((u16)node);
    int count = p->GetLinkCountForEdge(e);
    for (int i = 0; i < count; i++) {
        u16 l = p->GetLinkForEdge(e, (u16)i);
        if (l == 0xffff) return 0;
        XNavNode *o = LinkedNode(p, Link(l));
        if (!o) return 0;
        if (o->m_wNodeID == other) return 1;
    }
    return 0;
}

// Retail inlines ConnectedNodePtr into GetRoutePos.
static __forceinline XNavNode *ConnectedNodePtrInl(XNavNetwork *pNet, XNavNode *pNode, u16 i, XNavLink **ppLink)
{
    if (i >= pNode->m_wLinkCount || !pNode->m_pLinks) return 0;
    u32 li = pNode->m_pLinks[i].m_wLinkID;
    if ((u16)li == 0xffff) return 0;
    XNavLink *l = pNet->m_Section.Link((u16)li);
    *ppLink = l;
    if (l->m_wNodeA == 0xffff || l->m_wNodeB == 0xffff) return 0;
    if (l->m_wNodeA == pNode->m_wNodeID) return pNet->m_Section.Node(l->m_wNodeB);
    return pNet->m_Section.Node(l->m_wNodeA);
}

// NEAR: loop/branch layout and SSE scheduling of the interpolation differ.
s8 XNavNetwork::GetRoutePos(int from, int to, float dist, EXVector &pos, int &node)
{
    float remain = m_fRouteCost;
    XNavNode *n = Node((u16)to);
    if (n) {
        XNavNode *parent = n->m_pRouteParent;
        while (parent) {
            XNavLink *l;
            int i;
            for (i = 0; i < 16; i++)
                if (parent == ConnectedNodePtrInl(this, n, (u16)i, &l)) goto found;
            return 0;
        found:
            if (!l) return 0;
            float r = remain - l->m_fLength;
            if (dist > r) {
                dist = (float)fabs(remain - l->m_fLength - dist) / l->m_fLength;
                float dx = (n->m_Pos.x - parent->m_Pos.x) * dist;
                float dy = (n->m_Pos.y - parent->m_Pos.y) * dist;
                float dz = (n->m_Pos.z - parent->m_Pos.z) * dist;
                pos.x = parent->m_Pos.x + dx;
                pos.y = parent->m_Pos.y + dy;
                pos.z = parent->m_Pos.z + dz;
                pos.w = 0.0f;
                node = n->m_wNodeID;
                return 1;
            }
            remain = r;
            n = parent;
            parent = n->m_pRouteParent;
        }
    }
    return 0;
}

void XNavNetwork::InstallMasterLinks(XNavLinkInfo *)
{
    for (u16 i = 0; i < m_Section.m_wLinks; i++) {
        XNavLink *l = &m_Section.m_pLinks[i];
        XNavNode *a = Node(l->m_wNodeA);
        if (a) IndexForLinkIDInl(a, l->m_wLinkID);
        XNavNode *b = Node(l->m_wNodeB);
        if (b) IndexForLinkIDInl(b, l->m_wLinkID);
    }
}

float XNavNetwork::DistanceToLineSegment(const EXVector &a, const EXVector &b, const EXVector &p)
{
    EXVector n = NearestPointOnLineSegment(a, b, p, 0);
    return (n - p).LengthXZ();
}

void XNavNetwork::UnselectAll()
{
    for (int i = 0; i < NodeCount(); i++) Node((u16)i)->Select(0);
}

int XNavNetwork::SelectFlagged(u16 node, u16 mask)
{
    ClearNodeHeapFlags();
    SetNodeCosts(32000000.0f);
    XNavNodeHeap open(1);
    XNavNodeHeap closed(2);
    XNavNode *start = Node(node);
    int count = 1;
    u16 want = start->TypeFlags() & mask;
    start->Select(1);
    open.Add(start);
    while (open.Top()) {
        XNavNode *cur = open.Pop();
        for (int i = 0; i < cur->LinkCount(); i++) {
            XNavLink *l = NodeLinkPtr(cur, (u16)i);
            if (!l || (l->m_wTypeFlags & 1)) continue;
            XNavNode *n = LinkedNode(cur, l);
            if (n->Flags() & 3) continue;
            if ((n->TypeFlags() & mask) != want) continue;
            if (!n->IsSelected()) {
                count++;
                n->Select(1);
            }
            open.Push(n);
        }
        closed.Add(cur);
    }
    return count;
}

// NEAR: register allocation of the node copy (esi/ecx) and push order differ.
u16 XNavNetwork::AddNode(EXVector3 &p)
{
    if (m_Section.m_wNodes >= m_Section.m_wNodeC) {
        XNavNode *old = m_Section.m_pNodes;
        m_Section.m_pNodes = new XNavNode[m_Section.m_wNodeC + 32];
        memcpy(m_Section.m_pNodes, old, m_Section.m_wNodeC * sizeof(XNavNode));
        if (old) delete[] old;
        m_Section.m_wNodeC += 32;
    }
    u16 id = m_Section.m_wNodes++;
    XNavNode *n = &m_Section.m_pNodes[id];
    n->SetPos(p);
    n->SetNodeID(id);
    return id;
}

u16 XNavNetwork::AddLink(u16 a, u16 b, int, int, u16 v0, u16 v1, EXVector *pVerts)
{
    EXVector *p0 = &pVerts[v0];
    EXVector *p1 = &pVerts[v1];
    for (u16 i = 0; i < LinkCount(); i++) {
        if ((a == Link(i)->NodeA() && b == Link(i)->NodeB()) || (b == Link(i)->NodeA() && a == Link(i)->NodeB())) return i;
    }
    if (m_Section.m_wLinks >= m_Section.m_wLink12) {
        XNavLink *old = m_Section.m_pLinks;
        m_Section.m_pLinks = new XNavLink[m_Section.m_wLink12 + 32];
        memcpy(m_Section.m_pLinks, old, m_Section.m_wLink12 * sizeof(XNavLink));
        if (old) delete[] old;
        m_Section.m_wLink12 += 32;
    }
    u16 id = m_Section.m_wLinks++;
    XNavLink *l = Link(id);
    l->SetLinkID(id);
    l->SetNodeAB(a, b);
    l->SetEdge(v0, v1);
    XNavNode *nb = Node(b);
    XNavNode *na = Node(a);
    EXVector3 d(nb->m_Pos.x - na->m_Pos.x, nb->m_Pos.y - na->m_Pos.y, nb->m_Pos.z - na->m_Pos.z);
    float len = d.Length();
    if (p0 && p1) {
        float t = GetLinkCostRatio(&Node(a)->m_Pos, &Node(b)->m_Pos, *p0, *p1);
        float ca = Node(a)->CostScale() * len;
        float cb = Node(b)->CostScale() * len;
        len = cb * (1.0f - t) + ca * t;
        SetLinkLen(id, len);
        return id;
    }
    SetLinkLen(id, len);
    return id;
}

void XNavNetwork::EmitDistancesFromNode(u16 node, float)
{
    ClearNodeHeapFlags();
    SetNodeCosts(32000000.0f);
    XNavNodeHeap open(1);
    XNavNodeHeap closed(2);
    XNavNode *start = Node(node);
    start->SetCostToHere(0.0f);
    start->SetTotalCost(0.0f);
    EXVector startPos(start->m_Pos.x, start->m_Pos.y, start->m_Pos.z, 0.0f);
    open.Add(start);
    while (open.Top()) {
        XNavNode *cur = open.Pop();
        for (int i = 0; i < cur->LinkCount(); i++) {
            XNavLink *l = NodeLinkPtr(cur, (u16)i);
            if (!l || (l->m_wTypeFlags & 1)) continue;
            XNavNode *n = LinkedNode(cur, l);
            float g = CalcLinkCost(l, cur, startPos) + cur->CostToHere();
            u16 f = n->Flags();
            if ((f & 3) && g >= n->CostToHere()) continue;
            n->SetRouteParent(cur);
            n->SetCostToHere(g);
            n->SetTotalCost(g);
            n->SetRouteLink((u16)IndexForLinkIDInl(n, l->m_wLinkID));
            if (f & 2) closed.Remove(n);
            if (n->Flags() & 1) open.Adjust(n);
            else open.Push(n);
        }
        closed.Add(cur);
    }
}

s8 XNavNetwork::CalcAStar(int from, int to, u32 flags)
{
    if (from < 0 || to < 0) return 0;
    XNavNode *start = Node((u16)from);
    if (!start) return 0;
    start->SetRouteParent(0);
    if (from == to) return 1;
    PrepareNodes(32000000.0f);
    XNavNodeHeap open(1);
    XNavNodeHeap closed(2);
    XNavLink *l = 0;
    XNavNode *target = Node((u16)to);
    if (!target) return 0;
    EXVector startPos(start->m_Pos.x, start->m_Pos.y, start->m_Pos.z, 0.0f);
    open.Push(start);
    start->SetCostToHere(0.0f);
    start->SetTotalCost(0.0f);
    XNavNode *cur;
    while (open.Top()) {
        cur = open.Pop();
        for (int i = 0; i < cur->LinkCount(); i++) {
            XNavNode *n = ConnectedNodePtr(cur, (u16)i, &l);
            if (l && (l->m_wTypeFlags & 1)) continue;
            if (!n) continue;
            if (n->TypeFlags() & flags) continue;
            EXVector3 d(target->m_Pos.x - n->m_Pos.x, target->m_Pos.y - n->m_Pos.y, target->m_Pos.z - n->m_Pos.z);
            float h = d.Length() * 2.0f;
            float g = CalcLinkCost(l, cur, startPos) + cur->CostToHere();
            if (n->NodeID() == to) {
                n->SetCostToHere(g);
                n->SetRouteParent(cur);
                n->SetTotalCost(g + h);
                n->SetRouteLink((u16)i);
                for (XNavNode *p = cur; p; p = p->RouteParent()) {
                    p->m_pNext = n;
                    n = p;
                }
                return 1;
            }
            u16 f = n->Flags();
            if ((f & 3) && g >= n->CostToHere()) continue;
            n->SetCostToHere(g);
            n->SetRouteParent(cur);
            n->SetTotalCost(g + h);
            n->SetRouteLink((u16)i);
            if (f & 2) closed.Remove(n);
            if (n->Flags() & 1) open.Adjust(n);
            else open.Push(n);
        }
        closed.Add(cur);
    }
    return 0;
}

int XNavNetwork::BuildZones()
{
    for (int i = 0; i < NodeCount(); i++) Node((u16)i)->SetZone(0xffff);
    u16 zone = m_w62;
    u16 zoneNodes[2000];
    for (int i = 0; i < NodeCount(); i++) {
        if (m_Section.Node((u16)i)->Zone() != 0xffff) continue;
        u16 z = zone++;
        zoneNodes[z - m_w62] = (u16)i;
        m_Section.Node((u16)i)->SetZone(z);
        if (m_Section.Node((u16)i)->TypeFlags() & 0x48) continue;
        for (int j = i + 1; j < NodeCount(); j++) {
            if (i == j) continue;
            if (m_Section.Node((u16)j)->Zone() != 0xffff) continue;
            if (!CalcAStar(i, j, 0x48)) continue;
            Node((u16)j)->SetZone(z);
        }
    }
    m_wNumZones = zone - m_w62;
    if (!m_wNumZones) {
        m_p54 = 0;
        m_pNavZoneLinks = 0;
        return 1;
    }
    u32 n = m_wNumZones;
    m_pNavZoneLinks = (_XNavZone_Link *)_EXAlloc(((n >> 1) + 1) * n * sizeof(_XNavZone_Link), 0);
    m_p54 = m_pNavZoneLinks;
    m_wNavZoneLinks = 0;
    for (int a = 0; a < m_wNumZones; a++) {
        for (int b = a + 1; b < m_wNumZones; b++) {
            if (CalcAStar(zoneNodes[a], zoneNodes[b], 8)) {
                m_pNavZoneLinks[m_wNavZoneLinks].m_wType = 0;
                m_pNavZoneLinks[m_wNavZoneLinks].m_wZoneA = (u16)a;
                m_pNavZoneLinks[m_wNavZoneLinks].m_wZoneB = (u16)b;
                m_wNavZoneLinks++;
            }
        }
    }
    if (m_wNavZoneLinks) {
        m_p54 = _EXRealloc(m_p54, m_wNavZoneLinks * sizeof(_XNavZone_Link));
        m_pNavZoneLinks = (_XNavZone_Link *)m_p54;
        return 1;
    }
    EXFree(m_p54);
    m_pNavZoneLinks = 0;
    m_p54 = 0;
    return 1;
}

// NEAR: retail calls EXArray::SetSize(1) out of line; VC7.1 inlines it here.
int XNavNetwork::GetJumpToZoneArray(int zone, EXArray<int, int> &arr)
{
    arr.SetSize(0);
    arr.SetSize(1);
    arr.m_pData[0] = zone;
    if (zone == -1) return 1;
    zone &= 0xfff;
    for (int i = 0; i < m_wNavZoneLinks; i++) {
        _XNavZone_Link *l = &m_pNavZoneLinks[i];
        if (l->m_wZoneA == zone) {
            int n = arr.m_nSize;
            arr.SetSize(n + 1);
            arr.m_pData[n] = (s16)(l->m_wZoneB | 0x8000);
        } else if (l->m_wZoneB == zone) {
            int n = arr.m_nSize;
            arr.SetSize(n + 1);
            arr.m_pData[n] = (s16)(l->m_wZoneA | 0x8000);
        }
    }
    return arr.m_nSize;
}

// Closest point to p on the segment a-b, measured in the ground plane (x, z); *t gets the fraction
// along the segment. A flat-looking segment (nearly no x/z span) is handled by its height instead.
EXVector XNavNetwork::NearestPointOnLineSegment(const EXVector &a, const EXVector &b, const EXVector &p, float *t)
{
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float dz = b.z - a.z;
    float bw = b.w;
    float aw = a.w;
    float len2 = dx * dx + dz * dz;
    float f;
    if (0.001f <= (len2 < 0.0f ? -len2 : len2)) {
        f = ((p.z - a.z) * dz + (p.x - a.x) * dx) / len2;
    } else {
        if ((dy < 0.0f ? -dy : dy) < 0.001f) {
            f = 0.0f;
            goto inside;
        }
        f = ((p.y - a.y) * dy) / dy;
    }
    if (f < 0.0f) return a;
    if (1.0f < f) return b;
inside:
    if (t) *t = f;
    return EXVector(a.x + dx * f, dy * f + a.y, a.z + dz * f, a.w + (bw - aw) * f);
}

// Same shape as the 2D version (retail has an identical body).
EXVector XNavNetwork::NearestPointOnLineSegment3D(const EXVector &a, const EXVector &b, const EXVector &p, float *t)
{
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float dz = b.z - a.z;
    float bw = b.w;
    float aw = a.w;
    float len2 = dx * dx + dz * dz;
    float f;
    if (0.001f <= (len2 < 0.0f ? -len2 : len2)) {
        f = ((p.z - a.z) * dz + (p.x - a.x) * dx) / len2;
    } else {
        if ((dy < 0.0f ? -dy : dy) < 0.001f) {
            f = 0.0f;
            goto inside;
        }
        f = ((p.y - a.y) * dy) / dy;
    }
    if (f < 0.0f) return a;
    if (1.0f < f) return b;
inside:
    if (t) *t = f;
    return EXVector(a.x + dx * f, dy * f + a.y, a.z + dz * f, a.w + (bw - aw) * f);
}

// Whether segments p1-p2 and p3-p4 cross in the ground plane; the crossing goes to out.
s8 XNavNetwork::LineSegsIntersect2D(const EXVector &p1, const EXVector &p2, const EXVector &p3, const EXVector &p4,
                                    EXVector *out)
{
    float ax = p2.x - p1.x;
    float bx = p3.x - p4.x;
    float hi, lo;
    if (0.0f <= ax) {
        hi = p2.x;
        lo = p1.x;
    } else {
        lo = p2.x;
        hi = p1.x;
    }
    if (bx <= 0.0f) {
        if (hi < p3.x) return 0;
        if (p4.x < lo) return 0;
    } else {
        if (hi < p4.x) return 0;
        if (p3.x < lo) return 0;
    }
    float az = p2.z - p1.z;
    float bz = p3.z - p4.z;
    if (0.0f <= az) {
        hi = p2.z;
        lo = p1.z;
    } else {
        lo = p2.z;
        hi = p1.z;
    }
    if (bz <= 0.0f) {
        if (hi < p3.z) return 0;
        if (p4.z < lo) return 0;
    } else {
        if (hi < p4.z) return 0;
        if (p3.z < lo) return 0;
    }
    float num0 = (p1.x - p3.x) * bz - (p1.z - p3.z) * bx;
    float d = az * bx - bz * ax;
    if (d <= 0.0f) {
        if (0.0f < num0) return 0;
        if (num0 < d) return 0;
    } else {
        if (num0 < 0.0f) return 0;
        if (d < num0) return 0;
    }
    float num1 = (p1.z - p3.z) * ax - (p1.x - p3.x) * az;
    if (d <= 0.0f) {
        if (0.0f < num1) return 0;
        if (num1 < d) return 0;
    } else {
        if (num1 < 0.0f) return 0;
        if (d < num1) return 0;
    }
    if ((d < 0.0f ? -d : d) < 0.0001f) return 0;
    d = 1.0f / d;
    out->x = d * num0 * ax + p1.x;
    out->y = 0.0f;
    out->z = d * num0 * az + p1.z;
    out->w = 0.0f;
    return 1;
}

// As LineSegsIntersect2D but a miss of up to tol still counts as a crossing.
s8 XNavNetwork::LineSegsNearIntersect2D(const EXVector &p1, const EXVector &p2, const EXVector &p3, const EXVector &p4,
                                        EXVector *out, float &tol)
{
    float ax = p2.x - p1.x;
    float bx = p3.x - p4.x;
    float negTol = 0.0f - tol;
    float hi, lo;
    if (negTol <= ax) {
        hi = p2.x;
        lo = p1.x;
    } else {
        lo = p2.x;
        hi = p1.x;
    }
    if (bx <= tol) {
        if (hi < p3.x) return 0;
        if (p4.x < lo) return 0;
    } else {
        if (hi < p4.x) return 0;
        if (p3.x < lo) return 0;
    }
    float az = p2.z - p1.z;
    float bz = p3.z - p4.z;
    if (negTol <= az) {
        hi = p2.z;
        lo = p1.z;
    } else {
        lo = p2.z;
        hi = p1.z;
    }
    if (bz <= tol) {
        if (hi < p3.z) return 0;
        if (p4.z < lo) return 0;
    } else {
        if (hi < p4.z) return 0;
        if (p3.z < lo) return 0;
    }
    float num0 = (p1.x - p3.x) * bz - (p1.z - p3.z) * bx;
    float d = az * bx - bz * ax;
    if (d <= 0.0f) {
        if (tol < num0) return 0;
        if (num0 < d) return 0;
    } else {
        if (num0 < negTol) return 0;
        if (d < num0) return 0;
    }
    float num1 = (p1.z - p3.z) * ax - (p1.x - p3.x) * az;
    if (d <= 0.0f) {
        if (tol < num1) return 0;
        if (num1 < d) return 0;
    } else {
        if (num1 < negTol) return 0;
        if (d < num1) return 0;
    }
    if ((d < 0.0f ? -d : d) < 0.0001f) return 0;
    d = 1.0f / d;
    out->x = d * num0 * ax + p1.x;
    out->y = 0.0f;
    out->z = d * num0 * az + p1.z;
    out->w = 0.0f;
    return 1;
}

// Ground-plane intersection of the lines through p1-p2 and p3-p4, with the fractions along each
// (s along p1-p2, t along p3-p4). Without output pointers it only answers whether the segments
// cross; parallel lines fail with huge fractions.
s8 XNavNetwork::LineSegsIntersect2D2(const EXVector &p1, const EXVector &p2, const EXVector &p3, const EXVector &p4,
                                     EXVector *out, float *pS, float *pT)
{
    float big = 3.4028235e38f;
    float one = 1.0f;
    float d = (p4.z - p3.z) * (p2.x - p1.x) - (p2.z - p1.z) * (p4.x - p3.x);
    if ((d < 0.0f ? -d : d) < 0.00001f) {
        if (pT) *pT = big;
        if (pS) *pS = big;
        if (out) {
            out->x = 0.0f;
            out->y = 0.0f;
            out->z = 0.0f;
            out->w = 0.0f;
        }
        return 0;
    }
    float inv = 1.0f / d;
    float s = ((p1.z - p3.z) * (p4.x - p3.x) - (p4.z - p3.z) * (p1.x - p3.x)) * inv;
    if (pS) {
        *pS = s;
    } else if (!pT) {
        if (s < 0.0f) return 0;
        if (1.0f < s) return 0;
    }
    float t = ((p1.z - p3.z) * (p2.x - p1.x) - (p2.z - p1.z) * (p1.x - p3.x)) * inv;
    if (pT) {
        *pT = t;
    } else if (!pS) {
        if (t < 0.0f) return 0;
        if (one < t) return 0;
    }
    if (out) {
        out->x = p1.x + (p2.x - p1.x) * s;
        out->y = p1.y + (p2.y - p1.y) * s;
        out->z = (p2.z - p1.z) * s + p1.z;
        out->w = p1.w + (p2.w - p1.w) * s;
    }
    if (0.0f <= s && s <= one && 0.0f <= t && t <= one) return 1;
    return 0;
}

// Closest points between two lines (each an origin plus direction): out runs from the point on
// line a to the point on line b, with the fractions along each. Fails for (nearly) parallel or
// degenerate lines.
s8 XNavNetwork::ShortestLineSegBetween2LineSegs(const EXLineSeg &a, const EXLineSeg &b, EXLineSeg &out, float &s,
                                                float &t)
{
    float eps = 1.1920929e-7f;
    float dax = a.m_Direction.x, day = a.m_Direction.y, daz = a.m_Direction.z, daw = a.m_Direction.w;
    float dbx = b.m_Direction.x, dby = b.m_Direction.y, dbz = b.m_Direction.z, dbw = b.m_Direction.w;
    float ox = a.m_Origin.x - b.m_Origin.x;
    float oy = a.m_Origin.y - b.m_Origin.y;
    float oz = a.m_Origin.z - b.m_Origin.z;
    if ((eps <= (dbx < 0.0f ? -dbx : dbx)) || (eps <= (dby < 0.0f ? -dby : dby)) || (eps <= (dbz < 0.0f ? -dbz : dbz))) {
        if ((eps <= (dax < 0.0f ? -dax : dax)) || (eps <= (day < 0.0f ? -day : day)) ||
            (eps <= (daz < 0.0f ? -daz : daz))) {
            float obDot = oz * dbz + oy * dby + ox * dbx;
            float abDot = daz * dbz + day * dby + dax * dbx;
            float bbDot = dbz * dbz + dby * dby + dbx * dbx;
            float det = (daz * daz + day * day + dax * dax) * bbDot - abDot * abDot;
            if (eps <= (det < 0.0f ? -det : det)) {
                float sa = (abDot * obDot - (oz * daz + oy * day + ox * dax) * bbDot) / det;
                s = sa;
                float sb = (sa * abDot + obDot) / bbDot;
                t = sb;
                float f = s;
                float px = f * dax + a.m_Origin.x;
                float py = f * day + a.m_Origin.y;
                float pz = f * daz + a.m_Origin.z;
                float pw = daw * f + a.m_Origin.w;
                out.m_Origin.x = px;
                out.m_Origin.y = py;
                out.m_Origin.z = pz;
                out.m_Origin.w = pw;
                out.m_Direction.x = (sb * dbx + b.m_Origin.x) - px;
                out.m_Direction.y = (sb * dby + b.m_Origin.y) - py;
                out.m_Direction.z = (sb * dbz + b.m_Origin.z) - pz;
                out.m_Direction.w = (dbw * sb + b.m_Origin.w) - pw;
                return 1;
            }
        }
    }
    return 0;
}

// Searches outwards from a node over the nodes whose polygons touch the start polygon (sharing a
// vertex with it), nearest first, for the polygon that contains the point p. Returns its node id,
// or -1.
int XNavNetwork::NodeSpan(u16 startNode, const EXVector &, const EXVector &p)
{
    ClearNodeHeapFlags();
    SetNodeCosts(32000000.0f);
    XNavNode head;
    XNavNode *open = 0;
    XNavNode *done = 0;
    XNavNode *start = Node(startNode);
    EXGeoNavMeshEntity_PolyList *startPoly = m_pNavMesh->m_Section.Poly(startNode);
    if (!startPoly) return -1;
    start->m_wFlags |= 1;
    start->m_fCostToHere = 0.0f;
    start->m_fTotalCost = 0.0f;
    start->m_pNext = 0;
    open = start;
    do {
        XNavNode *cur = open;
        if (cur) {
            open = cur->m_pNext;
            cur->m_pNext = 0;
            cur->m_wFlags &= ~1;
        }
        for (int i = 0; i < cur->m_wLinkCount; i++) {
            if ((u16)i == 15 || !cur->m_pLinks) continue;
            u16 linkID = cur->m_pLinks[i].m_wLinkID;
            if (linkID == 0xffff) continue;
            XNavLink *link = m_Section.Link(linkID);
            if (!link || (link->m_wTypeFlags & 1)) continue;
            u16 other = link->m_wNodeA;
            if (other == 0xffff || link->m_wNodeB == 0xffff) other = 0xffff;
            else if (other == cur->m_wNodeID) other = link->m_wNodeB;
            XNavNode *nbr = m_Section.Node(other);
            if (nbr->m_wFlags & 3) continue;
            EXGeoNavMeshEntity_PolyList *poly = m_pNavMesh->m_Section.Poly(nbr->m_wNodeID);
            if (!poly) continue;
            bool bShares = false;
            for (int j = 0; j < poly->m_wNumVerts; j++) {
                for (int k = 0; k < startPoly->m_wNumVerts; k++) {
                    if (poly->m_wVerts[j] == startPoly->m_wVerts[k]) {
                        if (m_pNavMesh->PointInPolygon(p, nbr->m_wNodeID)) return nbr->m_wNodeID;
                        bShares = true;
                    }
                }
            }
            if (!bShares) continue;
            nbr->m_pRouteParent = cur;
            int idx;
            for (idx = 0; idx < nbr->m_wLinkCount; idx++) {
                u16 id = nbr->m_pLinks ? nbr->m_pLinks[idx].m_wLinkID : 0xffff;
                if (id == link->m_wLinkID) break;
            }
            if (idx >= nbr->m_wLinkCount) idx = -1;
            nbr->m_wRouteLink = (u16)idx;
            head.m_pNext = open;
            XNavNode *prev = &head;
            XNavNode *it;
            for (it = open; it; it = it->m_pNext) {
                if (nbr->m_fCostToHere < it->m_fCostToHere) {
                    XNavNode *next = prev->m_pNext;
                    prev->m_pNext = nbr;
                    nbr->m_pNext = next;
                    nbr->m_wFlags |= 1;
                    break;
                }
                prev = it;
            }
            if (!it) {
                XNavNode *next = prev->m_pNext;
                prev->m_pNext = nbr;
                nbr->m_wFlags |= 1;
                nbr->m_pNext = next;
            }
            open = head.m_pNext;
        }
        cur->m_wFlags |= 2;
        cur->m_pNext = done;
        done = cur;
    } while (open);
    return -1;
}

static const u32 s_NVNEps = 0x02200000;

XNavNode *XNavNetwork::NearestValidNode(u16 startID, const EXVector3 &pos, float, u32 mask)
{
    XNavNode *start = m_Section.Node(startID);
    if (!(mask & start->m_wTypeFlags)) return start;
    PrepareNodes(32000000.0f);
    XNavNode head;
    int nOpen = 0;
    start->m_fCostToHere = 0.0f;
    start->m_fTotalCost = 0.0f;
    head.m_pNext = 0;
    for (int i = 0; i < start->m_wLinkCount; i++) {
        XNavLink *link = 0;
        XNavNode *nbr = ConnectedNodePtr(start, (u16)i, &link);
        if (link && (link->m_wTypeFlags & 1)) continue;
        if (!nbr) continue;
        EXVector portal[2];
        float t = 0.5f;
        if (GetLinkPortal(link, start->m_wNodeID, portal)) {
            float den = (nbr->m_Pos.x - pos.x) * (portal[1].z - portal[0].z) - (nbr->m_Pos.z - pos.z) * (portal[1].x - portal[0].x);
            if (*(float *)&s_NVNEps < (float)fabs(den)) {
                t = ((pos.z - portal[0].z) * (portal[1].x - portal[0].x) - (pos.x - portal[0].x) * (portal[1].z - portal[0].z)) / den;
                if (t < 0.0f) t = 0.0f;
                else if (t > 1.0f) t = 1.0f;
            }
        }
        u16 tf = nbr->m_wTypeFlags;
        float scale;
        if (tf & 3) scale = 50.0f;
        else if (tf & 4) scale = 5.0f;
        else if (tf & 8) scale = 1000.0f;
        else scale = 1.0f;
        float cost = link->m_fLength * scale * t;
        nbr->m_pRouteParent = start;
        nbr->m_fCostToHere = cost;
        nbr->m_fTotalCost = cost;
        nbr->m_wRouteLink = (u16)i;
        XNavNode *prev = &head;
        XNavNode *it;
        for (it = head.m_pNext; it; it = it->m_pNext) {
            if (cost < it->m_fCostToHere) break;
            prev = it;
        }
        nbr->m_pNext = prev->m_pNext;
        prev->m_pNext = nbr;
        nbr->m_wFlags |= 1;
        nOpen++;
    }
    start->m_wFlags |= 2;
    start->m_pNext = 0;
    XNavNode doneHead;
    doneHead.m_pNext = start;
    XNavNode *done = start;
    for (;;) {
        XNavNode *cur = head.m_pNext;
        if (!cur) return 0;
        head.m_pNext = cur->m_pNext;
        cur->m_pNext = 0;
        cur->m_wFlags &= ~1;
        nOpen--;
        if (!(mask & cur->m_wTypeFlags)) return cur;
        for (u16 i = 0; (int)i < cur->m_wLinkCount; i++) {
            if (i >= cur->m_wLinkCount || !cur->m_pLinks) continue;
            u16 linkID = cur->m_pLinks[i].m_wLinkID;
            if (linkID == 0xffff) continue;
            XNavLink *link = m_Section.Link(linkID);
            u16 a = link->m_wNodeA;
            XNavNode *nbr;
            if (a == 0xffff || link->m_wNodeB == 0xffff) nbr = 0;
            else if (a == cur->m_wNodeID) nbr = m_Section.Node(link->m_wNodeB);
            else nbr = m_Section.Node(a);
            if ((link->m_wTypeFlags & 1) || !nbr) continue;
            EXVector p(pos.x, pos.y, pos.z, 0.0f);
            float nc = CalcLinkCost(link, cur, p) + cur->m_fCostToHere;
            if ((nbr->m_wFlags & 3) && !(nc < nbr->m_fCostToHere)) continue;
            nbr->m_pRouteParent = cur;
            nbr->m_fCostToHere = nc;
            nbr->m_fTotalCost = nc;
            nbr->m_wRouteLink = i;
            if (nbr->m_wFlags & 2) {
                XNavNode *pv = &doneHead;
                for (XNavNode *d = done; d; d = d->m_pNext) {
                    if (d == nbr) {
                        pv->m_pNext = nbr->m_pNext;
                        nbr->m_wFlags &= ~2;
                        nbr->m_pNext = 0;
                        break;
                    }
                    pv = d;
                }
                done = doneHead.m_pNext;
            }
            if (nbr->m_wFlags & 1) {
                XNavNode *pv = &head;
                for (XNavNode *o = head.m_pNext; o; o = o->m_pNext) {
                    if (o == nbr) {
                        pv->m_pNext = nbr->m_pNext;
                        nbr->m_wFlags &= ~1;
                        nOpen--;
                        nbr->m_pNext = 0;
                        break;
                    }
                    pv = o;
                }
            }
            XNavNode *pv = &head;
            XNavNode *o;
            for (o = head.m_pNext; o; o = o->m_pNext) {
                if (nc < o->m_fCostToHere) break;
                pv = o;
            }
            nbr->m_pNext = pv->m_pNext;
            pv->m_pNext = nbr;
            nbr->m_wFlags |= 1;
            nOpen++;
        }
        cur->m_wFlags |= 2;
        cur->m_pNext = doneHead.m_pNext;
        doneHead.m_pNext = cur;
        done = cur;
    }
}

void XNavNetwork::RemoveBadLinks()
{
    int count = NodeCount();
    for (int ni = 0; ni < count; ni++) {
        XNavNode *node = Node((u16)ni);
        if (!(node->m_wTypeFlags & 0x40)) continue;
        for (int e = 0; e < node->m_wNumEdges; e++) {
            int used = 0;
            for (int k = 0; k < node->m_wLinkCount; k++)
                if (node->GetLinkEdge((u16)k) == (u16)e) used++;
            if (used < 2) continue;
            for (int j = 0; j < used; j++) {
                u16 linkID = node->GetLinkForEdge((u16)e, (u16)j);
                if (linkID == 0xffff) continue;
                XNavLink *edgeLink = m_Section.Link(linkID);
                XNavNode *other = m_Section.Node(LinkedNodeID(node, edgeLink));
                if (!other) continue;
                XNavLink *link = LinkPtrForNodes(node, other, 0);
                if (!link) continue;
                if (!(other->m_wTypeFlags & 0x40)) {
                    link->m_wTypeFlags |= 1;
                    continue;
                }
                EXVector portal[2];
                GetLinkPortal(link, node->m_wNodeID, portal);
                portal[0].y = 0.0f;
                EXVector d = portal[1] - portal[0];
                d.y = 0.0f;
                d.Normalise();
                EXVector a(portal[0].x - node->m_Pos.x, 0.0f, portal[0].z - node->m_Pos.z, 0.0f);
                a.Normalise();
                EXVector b(portal[0].x - other->m_Pos.x, 0.0f, portal[0].z - other->m_Pos.z, 0.0f);
                b.Normalise();
                float ca = a.z * d.x + a.x * -d.z;
                float cb = b.z * d.x + b.x * -d.z;
                if ((ca <= 0.0f && cb <= 0.0f) || (ca >= 0.0f && cb >= 0.0f))
                    link->m_wTypeFlags |= 1;
            }
        }
    }
}

static inline float FleeLen(float x, float y, float z, float w)
{
    float s = x * x + y * y + z * z + w * w;
    return s < 0.0f ? 0.0f : (float)sqrt(s);
}

s8 XNavNetwork::GetFleePosition(const EXVector &pos, const EXVector &threat, float maxDist, EXVector &out, int *pNode)
{
    PrepareNodes(32000000.0f);
    XNavNode head;
    head.m_pNext = 0;
    int nodeIdx = -1;
    EXVector tmp;
    m_pNavMesh->ValidatePos(pos, tmp, &nodeIdx);
    if (nodeIdx < 0) return 0;
    float bx = (pos.x - threat.x) * 2.0f;
    float by = (pos.y - threat.y) * 2.0f;
    float bz = (pos.z - threat.z) * 2.0f;
    float bw = (pos.w - threat.w) * 2.0f;
    XNavNode *start = m_Section.Node((u16)nodeIdx);
    float sd = FleeLen(start->m_Pos.x - threat.x, start->m_Pos.y - threat.y, start->m_Pos.z - threat.z, 0.0f - threat.w);
    start->m_fCostToHere = maxDist - sd;
    start->m_fTotalCost = maxDist - sd;
    for (int i = 0; i < start->m_wLinkCount; i++) {
        XNavLink *link = 0;
        XNavNode *nbr = ConnectedNodePtr(start, (u16)i, &link);
        if (link && (link->m_wTypeFlags & 1)) continue;
        if (!nbr || (nbr->m_wTypeFlags & 8)) continue;
        float d = FleeLen(nbr->m_Pos.x - threat.x + bx, nbr->m_Pos.y - threat.y + by, nbr->m_Pos.z - threat.z + bz, 0.0f - threat.w + bw);
        float cost = maxDist - d;
        nbr->m_fCostToHere = cost;
        nbr->m_fTotalCost = cost;
        XNavNode *prev = &head;
        for (XNavNode *it = head.m_pNext; it; it = it->m_pNext) {
            if (cost < it->m_fCostToHere) break;
            prev = it;
        }
        nbr->m_pNext = prev->m_pNext;
        prev->m_pNext = nbr;
        nbr->m_wFlags |= 1;
    }
    start->m_wFlags |= 2;
    start->m_pNext = 0;
    XNavNode *done = start;
    for (;;) {
        XNavNode *cur = head.m_pNext;
        if (!cur) return 0;
        head.m_pNext = cur->m_pNext;
        cur->m_pNext = 0;
        cur->m_wFlags &= ~1;
        float d = FleeLen(cur->m_Pos.x - threat.x, cur->m_Pos.y - threat.y, cur->m_Pos.z - threat.z, 0.0f - threat.w);
        if (maxDist < d) {
            out = EXVector(cur->m_Pos.x, cur->m_Pos.y, cur->m_Pos.z, 0.0f);
            if (pNode) *pNode = cur->m_wNodeID | 0xff000000;
            return 1;
        }
        for (u16 i = 0; (int)i < cur->m_wLinkCount; i++) {
            XNavLink *link = 0;
            XNavNode *nbr = 0;
            if (cur->m_pLinks) {
                u16 linkID = cur->m_pLinks[i].m_wLinkID;
                if (linkID != 0xffff) {
                    link = m_Section.Link(linkID);
                    u16 a = link->m_wNodeA;
                    if (a != 0xffff && link->m_wNodeB != 0xffff)
                        nbr = (a == cur->m_wNodeID) ? m_Section.Node(link->m_wNodeB) : m_Section.Node(a);
                }
            }
            if (link && (link->m_wTypeFlags & 1)) continue;
            if (!nbr || (nbr->m_wTypeFlags & 8) || (nbr->m_wFlags & 3)) continue;
            float nd = FleeLen(nbr->m_Pos.x - threat.x + bx, nbr->m_Pos.y - threat.y + by, nbr->m_Pos.z - threat.z + bz, 0.0f - threat.w + bw);
            float cost = maxDist - nd;
            nbr->m_fCostToHere = cost;
            nbr->m_fTotalCost = cost;
            XNavNode *prev = &head;
            for (XNavNode *it = head.m_pNext; it; it = it->m_pNext) {
                if (cost < it->m_fCostToHere) break;
                prev = it;
            }
            nbr->m_pNext = prev->m_pNext;
            prev->m_pNext = nbr;
            nbr->m_wFlags |= 1;
        }
        cur->m_wFlags |= 2;
        cur->m_pNext = done;
        done = cur;
    }
}
