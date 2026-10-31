#ifndef LINECONTROLLER_H
#define LINECONTROLLER_H
#include "types.h"

struct EXPrimVert;

// Line vertex (x, y, z, colour).
struct D3DV_L {
    float x, y, z;
    u32 m_Color;
};

// Batches debug lines (XBWndDebug.obj), sizeof 0x20008.
class LineController {
public:
    LineController();
    void AddLine(D3DV_L *a, D3DV_L *b);
    void AddLines(unsigned int n, EXPrimVert *p);
    void RenderLines();

    int m_NumLines;           // 0x00 max 0x1000
    int m_NumVerts;           // 0x04
    D3DV_L m_Verts[0x2000];   // 0x08 colours converted to ARGB
};

extern LineController LineControl;
#endif
