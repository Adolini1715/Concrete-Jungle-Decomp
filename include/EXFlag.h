#ifndef EXFLAG_H
#define EXFLAG_H
#include "types.h"

namespace ex {
// Bit-flag wrapper used all over the engine (u8/u16/u32 instances in the map).
// Shapes from the map's emitted copies (Actor.obj, App.obj, EXItem.obj ...).
template <class T> class EXFlag {
public:
    EXFlag() { m_flags = 0; }
    operator T() const { return m_flags; }
    T operator=(T f) { m_flags = f; return f; }
    void Set(T f) { m_flags = f; }
    void BitSet(T b) { m_flags |= b; }
    void BitClr(T b) { m_flags &= ~b; }
    T BitTest(T b) const { return m_flags & b; }
    s8 BitTestBool(T b) const { return (m_flags & b) != 0; }
    void BitModify(T set, T clr) { m_flags = (m_flags & ~clr) | set; }
    // Ternary assignment, not if/else: gives the movzx/or/mov word shape (verified on
    // EXPlacementItem::GetExtRenderInfo / SetTexSwapInfo).
    template <class V> void BitState(T b, const V v) { m_flags = v ? (m_flags | b) : (m_flags & ~b); }
    T m_flags;
};
}
#endif
