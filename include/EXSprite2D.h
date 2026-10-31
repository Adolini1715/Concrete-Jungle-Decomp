#ifndef EXSPRITE2D_H
#define EXSPRITE2D_H
#include "types.h"
#include "RGBA.h"

// 2D sprites drawn through EXWnd::_DrawSprite2D (layouts from XBWndSprite.obj).
struct EXSprite2D {
    ex::RGBA m_Color;   // 0x00 alpha clamped to 0x80
    short x, y;         // 0x04 window relative
    u16 w, h;           // 0x08
    float u0, v0;       // 0x0c
    float u1, v1;       // 0x14
};

struct EXRotSprite2D : public EXSprite2D {
    float m_Unk1c;      // 0x1c
    float m_Angle;      // 0x20 negated for the renderer
};

struct EXFlexiSprite2D {
    struct Vert {
        ex::RGBA m_Color;  // 0x0
        short x, y;        // 0x4 window relative
        float u, v;        // 0x8
    };
    Vert m_Verts[4];
};
#endif
