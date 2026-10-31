#ifndef XRGBA_H
#define XRGBA_H
#include "types.h"

namespace ex {
// Packed 8-bit colour, r in the low byte. TODO: other members (widgets, lights use it).
struct XRGBA {
    XRGBA() {}
    XRGBA(u8 r_, u8 g_, u8 b_, u8 a_) { m_c = (((((a_ << 8) | b_) << 8) | g_) << 8) | r_; }
    // Byte view appended by the effects agent (EXDynLight::SetPointLight reads r/g/b/a through al/ah).
    union {
        u32 m_c;
        struct {
            u8 r, g, b, a;
        };
    };
};
}
#endif
