#include "EXHashCodeFile.h"

const char *EXHT::HashCodeNotFoundMessage = "HashCode Not Found";

s8 EXHashCodeFile::LoadFile(const char *) { return 1; }
const char *EXHashCodeFile::Debug_HashCodeName(u32) { return EXHT::HashCodeNotFoundMessage; }
u32 EXHashCodeFile::Debug_HashRef(const char *) { return 0xffffffff; }
