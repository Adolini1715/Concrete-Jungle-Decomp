#ifndef EXRELPTR_H
#define EXRELPTR_H
#include "types.h"

// Self-relative pointers used by the in-memory .edb data: the stored value is the byte distance
// from the field itself to the target, 0 meaning null.
namespace ex {

template <class T> struct EXRelPtr {
    operator T *() const
    {
        if (!m_Offset) return 0;
        return (T *)((char *)this + m_Offset);
    }
    s32 m_Offset;
};

// 16-bit variant (EXGeoTexture regions, datums).
template <class T> struct EXRelPtr16 {
    operator T *() const
    {
        if (!m_Offset) return 0;
        return (T *)((char *)this + m_Offset);
    }
    s16 m_Offset;
};

// Offset in the top 24 bits, 8 bits of payload below (EXGeoXtraData, entity lods).
template <class T> struct EXRelPtr24 {
    operator T *() const
    {
        int o = m_Value >> 8;
        if (!o) return 0;
        return (T *)((char *)this + o);
    }
    s32 m_Value;
};

struct EXRELOFFSET32 {
    s32 Offset() const { return m_Offset; }
    s32 m_Offset;
};
struct EXRELOFFSET16 {
    s32 Offset() const { return m_Offset; }
    s16 m_Offset;
};
struct EXRELOFFSET24x8 {
    s32 Offset() const { return m_Value >> 8; }
    s32 m_Value;
};
// Offset from the start of the file (header).
struct EXABSOFFSET32 {
    u32 m_Offset;
};

template <class O> inline void *EXRelOffset(const O &o) { return (char *)&o + o.Offset(); }
template <class B, class O> inline void *EXAbsOffset(const B *pBase, const O &o) { return (char *)pBase + o.m_Offset; }
// Moves an absolute offset from one base to another: base + (a - b).
template <class T, class O> inline T *EXRelocOffset(const T *pBase, const O &a, const O &b)
{
    return (T *)((char *)pBase + (a.m_Offset - b.m_Offset));
}

}

// Counted array whose data follows through a relative pointer: {size, relptr}.
template <class T> class EXRelArray {
public:
    int Size() const { return m_Size; }
    T *Data() const { return m_Data; }
    T &operator[](int i) const { return Data()[i]; }
    int m_Size;
    ex::EXRelPtr<T> m_Data;
};
#endif
