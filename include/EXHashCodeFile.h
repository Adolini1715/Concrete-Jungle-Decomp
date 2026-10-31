#ifndef EXHASHCODEFILE_H
#define EXHASHCODEFILE_H
#include "types.h"

namespace EXHT {
extern const char *HashCodeNotFoundMessage;
}

// Hash-code name table (debug name lookup); the retail build stubs everything out.
class EXHashCodeFile {
public:
    static s8 LoadFile(const char *pName);
    static const char *Debug_HashCodeName(u32 hash);
    static u32 Debug_HashRef(const char *pName);
};
#endif
