#ifndef EXBASETRIGGER_H
#define EXBASETRIGGER_H
#include "types.h"
#include "EXList.h"

// Abstract trigger base (dtor body in EXTrigger.obj, inline ctor emitted in Trigger.obj). sizeof 0xc.
// The list link is a non-polymorphic base after the vptr (operator= in ActorLocation.obj null-checks it).
class EXBaseTrigger : public EXDListItem {
public:
    EXBaseTrigger() {}
    virtual ~EXBaseTrigger() = 0;
};
#endif
