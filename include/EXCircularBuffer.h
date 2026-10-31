#ifndef EXCIRCULARBUFFER_H
#define EXCIRCULARBUFFER_H
#include "types.h"
#include <string.h>

// Fixed-size ring buffer (only instance: EXCircularBuffer<char, 2048> in EXWidget.obj, the text console).
template <class T, int N> class EXCircularBuffer {
public:
    EXCircularBuffer(s8 bOverwrite)
    {
        Empty();
        m_bOverwrite = bOverwrite;
    }
    ~EXCircularBuffer() {}
    void Empty()
    {
        m_Tail = 0;
        m_Head = 0;
        m_Count = 0;
        memset(m_Data, 0, sizeof(m_Data));
    }
    // -1: full and not overwriting; 1: overwrote the oldest element; 0: appended.
    int Put(const T &v)
    {
        if (m_Count == N && !m_bOverwrite) return -1;
        m_Data[m_Tail] = v;
        if (++m_Tail >= N) m_Tail = 0;
        if (m_Count == N) {
            m_Head = m_Tail;
            return 1;
        }
        m_Count++;
        return 0;
    }
    T Get()
    {
        if (m_Count == 0) return 0;
        m_Count--;
        u32 h = m_Head;
        if (++m_Head >= N) m_Head = 0;
        return m_Data[h];
    }
    u32 Size() const { return m_Count; }
    s8 IsEmpty() const { return m_Count == 0; }
    T &operator[](u32 i)
    {
        u32 n = m_Head + i;
        if (n >= N) n -= N;
        return m_Data[n];
    }
    T m_Data[N];       // 0x000
    u32 m_Head;        // N+0x0 read position
    u32 m_Tail;        // N+0x4 write position
    u32 m_Count;       // N+0x8
    s8 m_bOverwrite;   // N+0xc
};
#endif
