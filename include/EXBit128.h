#ifndef EXBIT128_H
#define EXBIT128_H
#include "types.h"

namespace ex {
// 128-bit set (bone/section masks). TODO: Set/Clr/IsSet/IsAnySet/&= shapes.
struct EXBit128 {
    void SetAll() { m_bits[0] = 0xffffffff; m_bits[1] = 0xffffffff; m_bits[2] = 0xffffffff; m_bits[3] = 0xffffffff; }
    void ClrAll() { m_bits[0] = 0; m_bits[1] = 0; m_bits[2] = 0; m_bits[3] = 0; }
    void Set(const EXBit128 &b)
    {
        m_bits[0] = b.m_bits[0];
        m_bits[1] = b.m_bits[1];
        m_bits[2] = b.m_bits[2];
        m_bits[3] = b.m_bits[3];
    }
    // Appended by the effects agent (shapes of the EXItemAnimator_Script.obj copies).
    void Set(int n) { m_bits[n / 32] |= 1 << (n & 31); }
    void Clr(int n) { m_bits[n / 32] &= ~(1 << (n & 31)); }
    // Set bits [first, last), or bit first when first == last (AIControl.obj [fi] copy; body by the
    // actor agent).
    void Set(int first, int last)
    {
        if (first == last) {
            m_bits[first / 32] |= 1 << (first & 31);
            return;
        }
        u32 a = 0xffffffff;
        u32 b = 0xffffffff;
        for (int i = 0; i < 4; i++) {
            if (first < 0) a = 0;
            else if (first < 32) a = (1 << first) - 1;
            if (last < 0) b = 0;
            else if (last < 32) b = (1 << last) - 1;
            m_bits[i] |= b ^ a;
            first -= 32;
            last -= 32;
        }
    }
    u32 IsSet(int n) const { return (m_bits[n / 32] >> (n & 31)) & 1; }
    // Emitted in EXItemAnimator_Anim.obj.
    u32 IsAnySet() const
    {
        for (int i = 0; i < 4; i++)
            if (m_bits[i]) return 1;
        return 0;
    }
    EXBit128 &operator&=(const EXBit128 &b)
    {
        m_bits[0] &= b.m_bits[0];
        m_bits[1] &= b.m_bits[1];
        m_bits[2] &= b.m_bits[2];
        m_bits[3] &= b.m_bits[3];
        return *this;
    }
    // Appended by the item agent (EXItemRender_Anim.obj copies).
    EXBit128 &operator|=(const EXBit128 &b)
    {
        m_bits[0] |= b.m_bits[0];
        m_bits[1] |= b.m_bits[1];
        m_bits[2] |= b.m_bits[2];
        m_bits[3] |= b.m_bits[3];
        return *this;
    }
    EXBit128 &operator^=(const EXBit128 &b)
    {
        m_bits[0] ^= b.m_bits[0];
        m_bits[1] ^= b.m_bits[1];
        m_bits[2] ^= b.m_bits[2];
        m_bits[3] ^= b.m_bits[3];
        return *this;
    }
    u32 m_bits[4];
};
}
#endif
