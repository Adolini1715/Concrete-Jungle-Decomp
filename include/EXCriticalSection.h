#ifndef EXCRITICALSECTION_H
#define EXCRITICALSECTION_H
#include "XBCriticalSection.h"

class EXCriticalSection {
public:
    void Enter() { RtlEnterCriticalSection(&m_CS); }
    void Leave() { RtlLeaveCriticalSection(&m_CS); }
    EXCriticalSection() { RtlInitializeCriticalSection(&m_CS); }
    ~EXCriticalSection() {}
    XBCriticalSection m_CS;
};
#endif
