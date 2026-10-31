#ifndef EXGEOCOLSPHERE_H
#define EXGEOCOLSPHERE_H
#include "types.h"
#include "EXRelPtr.h"

class EXVector;
class EXLineSeg;
struct EXBoundsSphere;

// Collision spheres of an entity (relptr at EXGeoBaseEntity +0x44): the spheres, then the node tree
// over them.
struct EXGeoColSphere_NodeList;
struct EXGeoColSphere_Table {
    EXBoundsSphere *Spheres() const { return m_Spheres; }
    EXGeoColSphere_NodeList *NodeList() { return (EXGeoColSphere_NodeList *)&m_NodeList; }
    ex::EXRelPtr<EXBoundsSphere> m_Spheres;     // 0x00
    u32 m_NodeList;                             // 0x04
};
s8 DoRayCast_CollisionSpheres(EXGeoColSphere_NodeList *pNodes, EXBoundsSphere *pSpheres, const EXLineSeg &l);
s8 DoCollision_CollisionSpheres(EXGeoColSphere_NodeList *pNodes, EXBoundsSphere *pSpheres, const EXVector &c, float r);
#endif
