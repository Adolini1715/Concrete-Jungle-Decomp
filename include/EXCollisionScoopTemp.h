#ifndef EXCOLLISIONSCOOPTEMP_H
#define EXCOLLISIONSCOOPTEMP_H
#include "EXMatrix.h"
#include "EXArray.h"
#include "EXCollisionLayer.h"
#include "EXGeoEntity_FaceRef.h"

class EXCollisionType;
class EXPlacementItem;
class EXItemPhysics;
struct EXGeoEntity_VtxData;

// One face gathered by a collision scoop (EXCollision.obj), sizeof 0x44.
class EXCScoop_FaceData {
public:
    EXCScoop_FaceData() {}
    unsigned short FaceFlags() const { return m_pGroup->m_faces[m_index].m_flags; }
    const EXVector &vtx0(EXGeoEntity_VtxData *pVtx) const { return *(const EXVector *)((u8 *)pVtx + m_pGroup->m_faces[m_index].vI0() * 0x18); }
    const EXVector &vtx1(EXGeoEntity_VtxData *pVtx) const { return *(const EXVector *)((u8 *)pVtx + m_pGroup->m_faces[m_index].vI1() * 0x18); }
    const EXVector &vtx2(EXGeoEntity_VtxData *pVtx) const { return *(const EXVector *)((u8 *)pVtx + m_pGroup->m_faces[m_index].vI2() * 0x18); }
    EXVector &Normal() { return *(EXVector *)&m_00[0]; }    // face normal (flipped when m_3e bit 7)
    EXVector &P0() { return *(EXVector *)&m_00[0x10]; }     // shape-side point
    EXVector &P1() { return *(EXVector *)&m_00[0x20]; }     // face-side point
    unsigned int idx0() { return m_pGroup->m_faces[m_index].m_v[0]; }
    unsigned int idx1() { return m_pGroup->m_faces[m_index].m_v[1]; }
    unsigned int idx2() { return m_pGroup->m_faces[m_index].m_v[2]; }

    u8 m_00[0x30];                              // +0x00 TODO
    EXGeoEntity_FaceCollisionGroup *m_pGroup;   // +0x30
    u32 m_34;                                   // +0x34 TODO
    float m_dist;                               // +0x38
    u16 m_index;                                // +0x3c
    u8 m_3e;                                    // +0x3e TODO
    u8 m_flags;                                 // +0x3f
    u8 m_sort;                                  // +0x40 index of the face in sorted order
    u8 m_41[3];
};

// One entity (platform) gathered by a collision scoop, sizeof 0x50.
struct EXCScoop_EntityInfo {
    EXCScoop_EntityInfo() {}
    EXMatrix m_mtx;     // +0x00
    u32 m_40;           // +0x40 TODO
    u32 m_44;           // +0x44 TODO
    u32 m_id;           // +0x48
    s16 m_sub;          // +0x4c
    u8 m_flags;         // +0x4e bit 1: matrix has scale (full inverse)
    u8 m_4f;
};

// Collision scoop query and result (EXCollision.obj), sizeof 0x148.
class EXCollisionScoopTemp {
public:
    EXCollisionScoopTemp() {}
    void SetTemporaryVars(EXPlacementItem *pItem, const EXMatrix &mtx, int n)
    {
        m_pItem = pItem;
        m_mtx = mtx;
        m_2c = n;
    }
    void ApplyXForm(const EXMatrix &m);
    void CalculateFaceArray();
    void HandleClothCollision(EXItemPhysics &phys, void (*fn)(EXCScoop_FaceData *, EXItemPhysics &));
    void QuickSort(unsigned int lo, unsigned int hi);
    float UpdateRadius(EXVector &c, const EXMatrix &m);
    void Initialise();
    void CalculateFaceTypes(signed char b);
    void CalculateCollisionResponse(EXItemPhysics &phys, EXVector &v, unsigned short &n, unsigned int flags,
                                    void (*fn)(EXCScoop_FaceData *, EXItemPhysics &));
    void CalculateDualCollisionResponse(EXItemPhysics &phys, EXVector &a, EXVector &b, unsigned short &n,
                                        unsigned int flags, void (*fn)(EXCScoop_FaceData *, EXItemPhysics &));
    void CalcBounds(const EXMatrix &m, unsigned char flags);
    void DoCollisionPlatform(EXCScoop_EntityInfo &e);

    EXCScoop_EntityInfo *m_pEntities;   // +0x00
    u32 m_numEntities;                  // +0x04
    u32 m_maxEntities;                  // +0x08
    EXVector m_centre;                  // +0x0c
    float m_radius;                     // +0x1c
    EXCollisionType *m_pObjects;        // +0x20
    u32 m_numObjects;                   // +0x24
    EXPlacementItem *m_pItem;           // +0x28
    int m_2c;                           // +0x2c
    EXMatrix m_mtx;                     // +0x30 current platform matrix
    EXCScoop_FaceData *m_pFaces;        // +0x70
    u32 m_numFaces;                     // +0x74
    u32 m_maxFaces;                     // +0x78
    EXMatrix m_invMtx;                  // +0x7c
    EXMatrix m_invXform;                // +0xbc
    u32 m_flags;                        // +0xfc
    EXMatrix m_xform;                   // +0x100
    EXCollisionLayer m_layer;           // +0x140
};

extern EXFixedArray<EXCScoop_FaceData, 512> ScoopFaceData;
extern EXFixedArray<EXCScoop_EntityInfo, 64> ScoopEntityData;
#endif
