#ifndef EXOBJECT_H
#define EXOBJECT_H
#include "types.h"
#include "EXRuntimeClass.h"

// Root of the runtime-class hierarchy (sizeof 4: vtable pointer only).
// TODO layout: the rest of the virtual interface is not recovered yet.
class EXObject {
    EX_DECLARE_DYNAMIC(EXObject)
};

s8 EXIsValidAddress(const void *p);
void EXAssertValidObject(const EXObject *p);
#endif
