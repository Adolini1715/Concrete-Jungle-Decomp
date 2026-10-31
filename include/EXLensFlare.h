#ifndef EXLENSFLARE_H
#define EXLENSFLARE_H
#include "EXFlare.h"

// Lens flare: up to ten elements (inline ctor emitted in FX_LensFlare.obj). sizeof 0xe4.
class EXLensFlare {
public:
    EXLensFlare()
    {
        m_Unkd4 = 1.0f;
        m_Unkd8 = 0.0f;
    }
    int m_Count;            // 0x00
    EXFlare m_Flares[10];   // 0x04
    float m_Unkcc;          // 0xcc
    float m_Unkd0;          // 0xd0
    float m_Unkd4;          // 0xd4
    float m_Unkd8;          // 0xd8
    float m_Unkdc;          // 0xdc
    float m_Unke0;          // 0xe0
};
#endif
