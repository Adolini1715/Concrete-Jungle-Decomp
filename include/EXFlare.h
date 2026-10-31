#ifndef EXFLARE_H
#define EXFLARE_H
#include "types.h"
#include "RGBA.h"
#include "XRGBA.h"

class EXTexture;

// One lens flare element (sizeof 0x14).
class EXFlare {
public:
    EXFlare() { m_Texture = 0; }
    // Inline in NightAndDay.obj.
    EXFlare(float a, float b, ex::XRGBA colour, float pos, EXTexture *tex)
    {
        m_Alpha = a;
        m_Size = b;
        m_Colour = ex::RGBA(colour.m_c);
        m_Pos = pos;
        m_Texture = (unsigned int)tex;
    }
    float m_Alpha;          // 0x00 |1 - 2 * pos|
    float m_Size;           // 0x04
    ex::RGBA m_Colour;      // 0x08
    float m_Pos;            // 0x0c position along the sun-to-centre line (0..1)
    unsigned int m_Texture; // 0x10
};
#endif
