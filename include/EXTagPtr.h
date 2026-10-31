#ifndef EXTAGPTR_H
#define EXTAGPTR_H
#include "types.h"

namespace ex {
// A value that can be redirected to live elsewhere: m_p points at m_v by default.
// Copy is the implicit member-wise one (the pointer is copied as is).
template <class T> class EXTagPtr {
public:
    EXTagPtr() { m_p = &m_v; }
    ~EXTagPtr() {}
    void operator=(T v) { *m_p = v; }
    operator const T() const { return *m_p; }
    void SetPtr(T *p) { m_p = p; }
    void ResetPtr() { m_p = &m_v; }
    T *m_p;
    T m_v;
};
}
#endif
