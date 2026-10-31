#ifndef EXGEOPATHNODE_H
#define EXGEOPATHNODE_H
#include "types.h"

// TODO layout: geo path node/link records (EXGeoMap area). Only what Traffic.obj needs: the
// traffic node/link classes derive from these (sizes 0x28 / 0x1c come from the derived classes).
class EXVector3;
struct EXGeoPathNode {
    EXGeoPathNode() {}
    // Position at 0x00, flags at 0x1c (ActorCombat.obj [fi]); TODO layout: real members
    const EXVector3 &Position() const { return *(const EXVector3 *)this; }
    unsigned int Flags() const { return *(const unsigned int *)((const char *)this + 0x1c); }
    // Flag channel raised on arrival (distraction paths, ActorMission.obj).
    unsigned short Channel() const { return *(const unsigned short *)((const char *)this + 0x14); }
};
struct EXGeoPathLink {
};

#endif
