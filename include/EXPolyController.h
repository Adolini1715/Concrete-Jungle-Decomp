#ifndef EXPOLYCONTROLLER_H
#define EXPOLYCONTROLLER_H
#include "types.h"

class EXTexture;
struct EXPrimVert;

// Batches EXWnd tri fans / strips for XBWndPrim.obj (sizeof 0x30). Index 0 = fan, 1 = strip.
// Flag bits in m_Flags[]: 2 z read, 4 z write, 8 back-face culling.
class EXPolyController {
public:
    void SetTriFanBackFaceCulling(s8 b) { if (b == 1) m_Flags[0] |= 8; else m_Flags[0] &= ~8; }
    void SetTriFanTexture(EXTexture *p) { m_pTexture[0] = p; }
    void SetTriFanZWriteMode(s8 b) { if (b == 1) m_Flags[0] |= 4; else m_Flags[0] &= ~4; }
    void SetTriFanZReadMode(s8 b) { if (b == 1) m_Flags[0] |= 2; else m_Flags[0] &= ~2; }
    void SetTriFanBlendMode(unsigned int m) { m_BlendMode[0] = m; }
    void SetTriStripBackFaceCulling(s8 b) { if (b == 1) m_Flags[1] |= 8; else m_Flags[1] &= ~8; }
    void SetTriStripTexture(EXTexture *p) { m_pTexture[1] = p; }
    void SetTriStripZWriteMode(s8 b) { if (b == 1) m_Flags[1] |= 4; else m_Flags[1] &= ~4; }
    void SetTriStripZReadMode(s8 b) { if (b == 1) m_Flags[1] |= 2; else m_Flags[1] &= ~2; }
    void SetTriStripBlendMode(unsigned int m) { m_BlendMode[1] = m; }
    void SetPrimType(unsigned int t) { m_PrimType = t; }
    EXPolyController();
    ~EXPolyController();
    void Reset();
    void AddPoly(unsigned int n, EXPrimVert *p);
    void RenderPolys();

    unsigned int m_NumPolys;     // 0x00 max 0x400
    char *m_pPolys;              // 0x04 0x400 x 0x334
    EXTexture **m_pPolyTexture;  // 0x08 per poly
    unsigned int *m_pPolyBlend;  // 0x0c per poly
    EXTexture *m_pTexture[2];    // 0x10
    unsigned int m_BlendMode[2]; // 0x18
    unsigned int m_PrimType;     // 0x20 0 fan, 1 strip
    unsigned int m_Flags[2];     // 0x24
    u32 m_Unk2c;                 // 0x2c
};

extern EXPolyController g_ExPolyController;
void *XBHeap1Alloc2(int size, int flags);
#endif
