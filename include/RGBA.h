#ifndef RGBA_H
#define RGBA_H
#include "types.h"

namespace ex {
// 32-bit colour, bytes r, g, b, a in memory (sizeof 4). Emitted copies: Actor.obj (0x11000/0x11010/0x11030),
// ActorSpecial.obj, Trigger.obj, XBDisplay.obj.
class RGBA {
public:
    RGBA() {}
    RGBA(u8 r, u8 g, u8 b, u8 a) { m_R = r; m_G = g; m_B = b; m_A = a; }
    RGBA(unsigned int c) { *(unsigned int *)this = c; }
    operator unsigned int() const { return *(const unsigned int *)this; }
    void Set(u8 r, u8 g, u8 b, u8 a) { m_R = r; m_G = g; m_B = b; m_A = a; }
    unsigned int argb() const { return ((((m_A << 8) | m_R) << 8 | m_G) << 8) | m_B; }
    u8 m_R, m_G, m_B, m_A;
};
}
#endif
