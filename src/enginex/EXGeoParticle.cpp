#include "EXGeoParticle.h"

EXBoundsBox &EXGeoParticle::GetBoundsBox()
{
    if ((u32)&m_BoundsBox & 0xf) for (;;) {}
    return m_BoundsBox;
}
