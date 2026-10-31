// Included from the bottom of EXGeoEntity.h (the include sits outside the guard so that either
// header can be included first).
#include "EXGeoEntity.h"
#ifndef EXGEONAVMESHBASEENTITY_H
#define EXGEONAVMESHBASEENTITY_H
#include "EXMemory.h"

class EXGeoNavMeshBaseEntity;

// Triangle index (20 bits) plus flags (12 bits).
class EXNavIndex {
public:
    void SetFlags(unsigned int f) { m_flags = f; }
    unsigned int GetFlags() const { return m_flags; }
    int GetIndex() const { return m_index; }
    signed char IsValid() const { return GetIndex() != 0xfffff; }
    u32 m_index : 20;
    u32 m_flags : 12;
};

// Nav triangle: three vertex indices and the A* node bookkeeping (sizeof 0x10).
class EXNavTriangle {
public:
    EXNavIndex m_v[3];  // +0x00
    u32 m_0c;           // +0x0c TODO: open/closed node
};

// Neighbour triangle across each edge (sizeof 0xc).
class EXNavConnection {
public:
    EXNavIndex m_edge[3];
};

// Precomputed centre-to-centre distance across each edge (sizeof 0xc).
class EXNavDistance {
public:
    float m_dist[3];
};

class EXNavIdentifierGroup {
public:
    unsigned int GetGroupTriTotal() const { return m_triTotal; }
    u32 m_triTotal : 24;
    u32 m_flags : 8;
};

// Path: doubly linked list of triangle nodes (EXGeoEntity_NavMesh.obj).
class ExNavTriNode {
public:
    ExNavTriNode(unsigned int tri)
    {
        m_pPrev = 0;
        m_pNext = 0;
        m_tri = tri;
    }
    ExNavTriNode *m_pPrev;  // +0
    ExNavTriNode *m_pNext;  // +4
    u32 m_tri;              // +8
};

class ExNavPathBase {
public:
    ExNavPathBase(EXGeoNavMeshBaseEntity *pMesh)
    {
        m_pMesh = pMesh;
        m_pPrev = 0;
        m_pNext = 0;
        m_pHead = 0;
        m_14 = 0;
        m_flags = 0;
    }
    ~ExNavPathBase()
    {
        ExNavTriNode *p = m_pHead;
        while (p) {
            ExNavTriNode *t = p;
            p = p->m_pNext;
            delete t;
        }
    }
    void AddTriToHead(unsigned int tri)
    {
        ExNavTriNode *n = new ExNavTriNode(tri);
        n->m_pNext = m_pHead;
        m_pHead = n;
        if (n->m_pNext) n->m_pNext->m_pPrev = n;
    }
    void SetFlag(unsigned int f) { m_flags |= f; }
    void Release();

    ExNavPathBase *m_pPrev;         // +0x00
    ExNavPathBase *m_pNext;         // +0x04
    EXGeoNavMeshBaseEntity *m_pMesh;// +0x08
    u32 m_flags;                    // +0x0c bit 1: in the mesh's issued list
    ExNavTriNode *m_pHead;          // +0x10
    u32 m_14;                       // +0x14 TODO
};

// A* search node (sizeof 0x1c).
class ExNavAStarNode {
public:
    ExNavAStarNode()
    {
        m_pPrev = 0;
        m_pNext = 0;
        m_pParent = 0;
        m_f = 0.0f;
        m_g = 0.0f;
        m_h = 0.0f;
        m_tri = -1;
    }
    ExNavAStarNode *m_pPrev;    // +0x00
    ExNavAStarNode *m_pNext;    // +0x04
    ExNavAStarNode *m_pParent;  // +0x08
    float m_f;                  // +0x0c total cost (list sort key)
    float m_g;                  // +0x10
    float m_h;                  // +0x14
    int m_tri;                  // +0x18
};

class ExNavAStarLinkedList {
public:
    ExNavAStarLinkedList() { m_pHead = 0; }
    void AddHead(ExNavAStarNode *n)
    {
        n->m_pNext = m_pHead;
        n->m_pPrev = 0;
        if (m_pHead) m_pHead->m_pPrev = n;
        m_pHead = n;
    }
    void Insert(ExNavAStarNode *n)
    {
        if (!m_pHead) {
            m_pHead = n;
            return;
        }
        ExNavAStarNode *p = m_pHead;
        ExNavAStarNode *last;
        do {
            if (p->m_f > n->m_f) {
                n->m_pPrev = p->m_pPrev;
                p->m_pPrev = n;
                n->m_pNext = p;
                if (n->m_pPrev) n->m_pPrev->m_pNext = n;
                else m_pHead = n;
                return;
            }
            last = p;
            p = p->m_pNext;
        } while (p);
        last->m_pNext = n;
        n->m_pPrev = last;
    }
    void Remove(const ExNavAStarNode *n)
    {
        if (n->m_pPrev) {
            n->m_pPrev->m_pNext = n->m_pNext;
        } else {
            m_pHead = n->m_pNext;
            if (m_pHead) m_pHead->m_pPrev = 0;
        }
        if (n->m_pNext) n->m_pNext->m_pPrev = n->m_pPrev;
    }
    ExNavAStarNode *Pop()
    {
        ExNavAStarNode *n = m_pHead;
        if (n) {
            m_pHead = n->m_pNext;
            if (m_pHead) m_pHead->m_pPrev = 0;
        }
        return n;
    }
    signed char NotEmpty() const { return m_pHead != 0; }
    void EmptyList(const EXGeoNavMeshBaseEntity *pMesh);

    ExNavAStarNode *m_pHead;
};

// Edge cost and A* heuristic (EXGeoEntity_NavMesh.obj).
class ExNavCostBase {
public:
    ExNavCostBase() {}
    virtual float CalculateCost(EXGeoNavMeshBaseEntity *pMesh, int from, int to, int edge);
    virtual float EstimateCost(EXGeoNavMeshBaseEntity *pMesh, int from, int to);
};

// Whether a triangle may be entered across an edge (EXGeoEntity_NavMesh.obj).
class ExNavValidationBase {
public:
    ExNavValidationBase() {}
    virtual signed char IsValid(EXGeoNavMeshBaseEntity *pMesh, int tri, int prev, int edge, int user);
};

extern ExNavCostBase g_ExNavDefaultAStarCost;
extern ExNavValidationBase g_ExNavDefaultValidation;

// Nav mesh entity (EXGeoEntity_NavMesh.obj), sizeof 0xb8. No runtime class of its own.
class EXGeoNavMeshBaseEntity : public EXGeoBaseEntity {
protected:
    virtual void OnCreate(EXGeoHeader *, EXGeoObjectHeader *);
    virtual void OnDelete(EXGeoHeader *, EXGeoObjectHeader *);
public:
    virtual unsigned int GetVertexCount() const { return m_VertexCount; }
    virtual ~EXGeoNavMeshBaseEntity();
    virtual int RayCast(const EXVector &, const EXVector &, float, int) const;
    virtual int RayCastNearest(const EXVector &, const EXVector &, float, int) const;
    // Overloaded virtuals: reverse declaration order.
    virtual signed char GetTriangle(const EXVector &p, const EXVector &d, int tri, EXVector &out);
    virtual int GetTriangle(const EXVector &p, const EXVector &d, int tri, EXVector &out, int depth, int user, int prev);
    virtual signed char IsInLineOfSight(const EXVector &, const EXVector &);
    EXGeoNavMeshBaseEntity();

    void AddToIssuedPaths(ExNavPathBase *p)
    {
        if (p) {
            p->m_pNext = m_pIssuedPaths;
            m_pIssuedPaths = p;
            if (p->m_pNext) p->m_pNext->m_pPrev = p;
            p->SetFlag(2);
        }
    }
    unsigned int GetGroupCount() const { return m_GroupCount; }
    void SetCostFunction(ExNavCostBase *p);
    void SetValidationFunction(ExNavValidationBase *p);
    EXVector3 *GetVertices() const { return m_Vertices; }
    EXNavTriangle *GetTriangles() const { return m_Triangles; }
    EXNavConnection *GetConnections() const { return m_Connections; }
    EXNavIdentifierGroup *GetGroups() const { return m_Groups; }
    EXVector3 *GetCentres() const { return m_Centres; }
    EXNavDistance *GetDistances() const { return m_Distances; }
    // NEAR ([fi] copy): ~210 bytes differ, retail keeps the centre-table copy path free of pushes.
    __declspec(noinline) EXVector3 GenerateCentre(int tri) const
    {
        EXVector3 *c = GetCentres();
        if (c) return c[tri];
        EXVector3 *v = GetVertices();
        EXNavTriangle *t = &GetTriangles()[tri];
        const EXVector3 &v2 = v[t->m_v[2].GetIndex()];
        const EXVector3 &v1 = v[t->m_v[1].GetIndex()];
        const EXVector3 &v0 = v[t->m_v[0].GetIndex()];
        return EXVector3((v2.x + v0.x + v1.x) * 0.33333334f, (v0.y + v1.y + v2.y) * 0.33333334f, (v0.z + v1.z + v2.z) * 0.33333334f);
    }
    ExNavPathBase *GeneratePathDepthFirst(int from, int depth, int to);
    ExNavPathBase *GeneratePathDepthFirst(const EXVector &a, const EXVector &b, int depth);
    ExNavPathBase *GeneratePathBredthFirst(int from, int depth, int to);
    ExNavPathBase *GeneratePathBredthFirst(const EXVector &a, const EXVector &b, int depth);
    ExNavPathBase *GeneratePathAStar(int from, int to);
    ExNavPathBase *GeneratePathAStar(const EXVector &a, const EXVector &b);

    ExNavValidationBase *m_pValidation;         // 0x54
    ExNavCostBase *m_pCost;                     // 0x58
    int m_startTri;                             // 0x5c
    int m_goalTri;                              // 0x60
    int m_user;                                 // 0x64 passed to the validation function
    ExNavPathBase *m_pIssuedPaths;              // 0x68
    u32 m_VertexCount;                          // 0x6c
    u32 m_TriangleCount;                        // 0x70
    u32 m_GroupCount;                           // 0x74
    ex::EXRelPtr<EXVector3> m_Vertices;         // 0x78
    ex::EXRelPtr<EXNavTriangle> m_Triangles;    // 0x7c
    ex::EXRelPtr<EXNavConnection> m_Connections;// 0x80
    ex::EXRelPtr<EXNavIdentifierGroup> m_Groups;// 0x84
    ex::EXRelPtr<EXVector3> m_Centres;          // 0x88
    u32 m_8c;                                   // 0x8c TODO
    ex::EXRelPtr<EXNavDistance> m_Distances;    // 0x90
    EXVector m_rayDir;                          // 0x94 down probe used to find a point's triangle
    float m_rayLength;                          // 0xa4
    u8 m_a8[0xb8 - 0xa8];
};
#endif
