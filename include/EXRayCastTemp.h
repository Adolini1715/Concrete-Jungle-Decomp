#ifndef EXRAYCASTTEMP_H
#define EXRAYCASTTEMP_H
#include "EXBounds.h"
#include "EXCollisionLayer.h"
#include "EXGeoEntity_FaceRef.h"

class EXItemAnimator;

// Ray cast query and result (EXCollision.obj), sizeof 0xc4.
struct EXRayCastTemp {
    EXRayCastTemp() {}
    __declspec(noinline) EXVector Intersect() const { return m_dir * m_t + m_start; }
    void FindAnyHit() { m_bAnyHit = 1; }
    void FindClosestHit() { m_bAnyHit = 0; }
    void SetValid(EXItemAnimator *pAnim);
    void CalcBounds();
    void CalcBoundsTemp();
    void Set(const EXVector &start, const EXVector &dir, unsigned int layer, unsigned int layerFlags,
             unsigned int flags, unsigned int mask);

    EXVector m_localStart;          // +0x00 ray in the space of the item being tested
    EXVector m_localDir;            // +0x10
    EXVector m_start;               // +0x20
    EXVector m_dir;                 // +0x30
    EXBoundsBox m_bounds;           // +0x40
    EXBoundsBox m_localBounds;      // +0x60
    EXVector m_normal;              // +0x80 hit face normal (local to the hit item; w unused)
    float m_t;                      // +0x90 hit parameter along m_dir
    EXGeoEntity_FaceRef m_face;     // +0x94
    EXItemAnimator *m_pAnimator;    // +0xac
    EXCollisionLayer m_layer;       // +0xb0
    u32 m_flags;                    // +0xb8
    u32 m_mask;                     // +0xbc
    s8 m_bHit;                      // +0xc0
    s8 m_bAnyHit;                   // +0xc1
};
#endif
