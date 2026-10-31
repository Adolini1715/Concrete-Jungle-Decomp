#ifndef EXGRABPOINTTEMP_H
#define EXGRABPOINTTEMP_H
#include "EXVector.h"
#include "EXCollisionLayer.h"
#include "EXMemory.h"
#include "EXRelPtr.h"

class EXItem;
class EXItemAnimator;

// Segment header of an EXSegArray1 (the first element slot of each segment holds it).
class EXSegArrayHeader1 {
public:
    void SetNextSegmentHeader(EXSegArrayHeader1 *p) { m_pNext = p; }
    void SetPrevSegmentHeader(EXSegArrayHeader1 *p) { m_pPrev = p; }
    EXSegArrayHeader1 *m_pNext;     // +0
    EXSegArrayHeader1 *m_pPrev;     // +4
};

// Segmented array of N elements per segment; slot 0 of a segment is its header.
template <class T, int N> class EXSegArray1 {
public:
    EXSegArray1() { m_count = 0; }
    ~EXSegArray1()
    {
        if (m_count) {
            EXSegArrayHeader1 *p = m_pHead;
            while (p) {
                EXSegArrayHeader1 *n = p->m_pNext;
                EXFree(p);
                p = n;
            }
            m_count = 0;
        }
    }
    T *AddTail()
    {
        if (!(m_count & (N - 1))) AddNewTailSegment();
        u32 i = m_count++;
        return (T *)m_pTail + (i & (N - 1)) + 1;
    }
protected:
    EXSegArrayHeader1 *SegBaseAdrToSegHdrAdr(void *p) const { return (EXSegArrayHeader1 *)p; }
    void AddNewTailSegment()
    {
        EXSegArrayHeader1 *p = SegBaseAdrToSegHdrAdr(_EXAlloc((N + 1) * sizeof(T), 0));
        p->SetNextSegmentHeader(0);
        if (!m_count) {
            p->SetPrevSegmentHeader(0);
            m_pHead = p;
            m_pTail = p;
        } else {
            p->SetPrevSegmentHeader(m_pTail);
            m_pTail->SetNextSegmentHeader(p);
            m_pTail = p;
        }
    }
public:
    EXSegArrayHeader1 *m_pHead;     // +0
    EXSegArrayHeader1 *m_pTail;     // +4
    u32 m_count;                    // +8
};

// One grab point found by an array query (sizeof 0x9c): the EXGrabPointTemp result fields.
struct EXGrabPointData {
    u32 m_Type;         // +0x00 as EXGrabPointTemp::m_24
    u16 m_Data;         // +0x04 as m_28
    u16 m_06;
    u32 m_Corner;       // +0x08 as m_2c
    EXVector m_Pos;     // +0x0c hit (polygons: retail stores the last line hit here, see DoGrabPoint)
    EXVector m_Normal;  // +0x1c
    EXVector m_EdgePos; // +0x2c
    EXVector m_EdgeEnd; // +0x3c
    EXVector m_EdgeUp;  // +0x4c
    float m_Matrix[16]; // +0x5c
};

// Grab point query (EXGeoGrabpoint.obj), sizeof 0xc0 (stack frames in Player.obj).
struct EXGrabPointTemp {
    enum GRABFITYPE { GRABFI_0 = 0 };   // TODO values
    EXGrabPointTemp() {}
    void Set(const EXVector &pos, float radius, GRABFITYPE type);
    EXItem *GetItem() const;

    EXVector m_pos;                 // +0x00
    GRABFITYPE m_type;              // +0x10
    float m_radius;                 // +0x14
    EXCollisionLayer m_layer;       // +0x18
    EXItemAnimator *m_pAnimator;    // +0x20
    u32 m_24;                       // +0x24 TODO
    u16 m_28;                       // +0x28 TODO flags (0x100 tested by Player.obj CheckPerchLand)
    u16 m_2a;                       // +0x2a TODO
    u32 m_2c;                       // +0x2c TODO
    EXVector m_30;                  // +0x30 hit position (Player.obj GetGrabPoint)
    EXVector m_Normal;              // +0x40 surface normal at the hit (x, z give the wall facing: DoJump)
    EXVector m_EdgePos;             // +0x50 edge start (line) / triangle vertex 0 (DoGrabPoint)
    EXVector m_EdgeEnd;             // +0x60 edge end / triangle vertex 1
    EXVector m_EdgeUp;              // +0x70 edge start + (0, 1, 0) / triangle vertex 2
    float m_Matrix[16];             // +0x80 the entity matrix the hit was found with
};
// m_24 results of EXGeoGrabPoint::DoGrabPoint: 1 on a ledge line, 3 at a line end flagged as a corner
// (m_2c 0 start, 1 end), 2 on a grab polygon, 4 on a flagged polygon edge (m_2c the edge).

// Grab point query collecting every hit (EXGeoGrabpoint.obj), sizeof 0x2c.
struct EXGrabPointArrayTemp {
    enum GRABFITYPE { GRABFI_0 = 0 };   // TODO values
    EXGrabPointArrayTemp() {}
    ~EXGrabPointArrayTemp() {}
    void Set(const EXVector &pos, float radius, GRABFITYPE type);

    EXVector m_pos;                                 // +0x00
    GRABFITYPE m_type;                              // +0x10
    float m_radius;                                 // +0x14
    EXCollisionLayer m_layer;                       // +0x18
    EXSegArray1<EXGrabPointData, 32> m_points;      // +0x20
};

// Grab data in the .edb (EXGeoBaseEntity xtra data 6). Line strips: a header vertex whose m_Count is the
// number of further vertices; each vertex record is 0x10 bytes.
struct EXGeoGrabPoint_LineVert {
    u8 m_Count;         // +0 header only: vertices after this one
    u8 m_Flags;         // +1 1: the start is a corner, 2: the end is a corner
    u16 m_Data;         // +2 copied to EXGrabPointTemp::m_28
    float x, y, z;      // +4
};
// Grab polygons: a fan of m_Count vertices (0x10 apart) after a 0x10 header holding the normal.
struct EXGeoGrabPoint_TriVert {
    float x, y, z, w;
};
struct EXGeoGrabPoint_Tri {
    u8 m_Count;         // +0 vertices
    u8 m_Flags;         // +1 bit n: edge n of the polygon is grabbable
    u16 m_Data;         // +2 copied to EXGrabPointTemp::m_28
    float m_Normal[3];  // +4
    EXGeoGrabPoint_TriVert m_v[1];  // +0x10
};

class EXMatrix;
struct EXBoundsBox;
class EXGeoGrabPoint {
public:
    void GetBoundsBox(EXBoundsBox &b) const;
    s8 DoGrabPoint(const EXMatrix &m, EXGrabPointTemp &t) const;
    s8 DoGrabPoint(const EXMatrix &m, EXGrabPointArrayTemp &t) const;

    int m_nLines;                                   // +0x00
    ex::EXRelPtr<EXGeoGrabPoint_LineVert> m_Lines;  // +0x04
    int m_nTris;                                    // +0x08
    ex::EXRelPtr<EXGeoGrabPoint_Tri> m_Tris;        // +0x0c
    u16 m_bLines;                                   // +0x10 nonzero: the line strips are live
};
#endif
