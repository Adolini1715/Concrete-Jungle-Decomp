#ifndef EXU32ARRAY_H
#define EXU32ARRAY_H
#include "EXArray.h"

// u32 array (array area). Only the EXDataArray part is used. TODO layout: moved out of
// EXItemAnimator_Anim.h so the map code can use it without the animator headers.
class EXu32Array : public EXDataArray<u32, u32> {
};
#endif
