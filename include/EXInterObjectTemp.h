#ifndef EXINTEROBJECTTEMP_H
#define EXINTEROBJECTTEMP_H
#include "EXArray.h"
#include "EXVector.h"

class EXItemPhysics;

// One object/object contact (EXCollision.obj), sizeof 0x30.
struct EXInterObject_ContactInfo {
    EXInterObject_ContactInfo() {}
    // Layout from EXItemEnv.obj UpdateItems_InterObjCollision (item agent).
    EXVector m_a;       // +0x00 contact point on the first object
    EXVector m_b;       // +0x10 contact point on the second object
    u32 m_idxA;         // +0x20 collision object index (first)
    u32 m_idxB;         // +0x24 collision object index (second)
    float m_depth;      // +0x28
    u32 m_2c;
};

// Object/object contact query (EXCollision.obj). TODO layout: only the contact array is known.
class EXInterObjectTemp {
public:
    void Initialise();

    EXItemPhysics *m_pA;                    // +0x00 (EXItemEnv.obj UpdateItems_InterObjCollision)
    EXItemPhysics *m_pB;                    // +0x04
    EXInterObject_ContactInfo *m_pContacts; // +0x08
    u32 m_maxContacts;                      // +0x0c
    u32 m_numContacts;                      // +0x10
    u8 m_which;                             // +0x14 0: callback for m_pA, 1: for m_pB
};

extern EXFixedArray<EXInterObject_ContactInfo, 64> InterContactData;
#endif
