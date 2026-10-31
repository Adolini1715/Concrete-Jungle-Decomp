#ifndef EXSHADOWTEMP_H
#define EXSHADOWTEMP_H
#include "EXCollisionLayer.h"
#include "EXGeoEntity_FaceRef.h"

// One shadow receiver face (sizeof 0x4c).
struct EXShadowFace {
    EXVector m_v[3];            // +0x00
    float m_dist;               // +0x30
    EXGeoEntity_FaceRef m_face; // +0x34
};

// Shadow cast query (EXCollision.obj), sizeof 0x164.
struct EXShadowTemp {
    EXShadowTemp() {}
    void Set(const EXVector &pos, float height, float radius, unsigned int layer, unsigned int layerFlags);

    EXVector m_pos;             // +0x00
    u8 m_10[0x10];              // +0x10 TODO
    float m_height;             // +0x20
    float m_radius;             // +0x24
    EXCollisionLayer m_layer;   // +0x28
    EXShadowFace m_faces[4];    // +0x30
    u8 m_numFaces;              // +0x160
};
#endif
