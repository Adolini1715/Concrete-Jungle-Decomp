#ifndef EXCOLLISIONLAYER_H
#define EXCOLLISIONLAYER_H
#include "types.h"

class EXGeoBaseEntity;

// Which collision level of an entity a query uses (EXCollision.obj), sizeof 8. Flags: bit 0 = fall back
// to the render entity, bit n+1 = level n may stand in, 0x400 = always use the render entity.
class EXCollisionLayer {
public:
    void Set(unsigned int layer, unsigned int flags)
    {
        m_layer = layer;
        m_flags = flags;
    }
    signed char CalcCollisionEntity(EXGeoBaseEntity *&pEnt) const;
    signed char CalcGrabpointEntity(EXGeoBaseEntity *&pEnt) const;

    u32 m_layer;    // +0
    u32 m_flags;    // +4
};
#endif
