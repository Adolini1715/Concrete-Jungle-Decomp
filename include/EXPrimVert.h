#ifndef EXPRIMVERT_H
#define EXPRIMVERT_H
#include "types.h"

// Primitive vertex handed to EXWnd::DrawLine / DrawTriFan / DrawTriStrip (sizeof 0x20).
struct EXPrimVert {
    float x, y, z, w;   // 0x00
    float u, v;         // 0x10
    u32 m_Unk18;        // 0x18 TODO
    u32 m_Color;        // 0x1c ex::RGBA bytes r, g, b, a
};
#endif
