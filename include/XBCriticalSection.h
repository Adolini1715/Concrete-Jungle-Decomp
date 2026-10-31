#ifndef XBCRITICALSECTION_H
#define XBCRITICALSECTION_H
#include "types.h"

// Kernel critical section (RTL_CRITICAL_SECTION on Xbox, 0x1c bytes); declarations are ours.
struct XBCriticalSection {
    u32 m_Data[7];
};
extern "C" {
__declspec(dllimport) void __stdcall RtlInitializeCriticalSection(XBCriticalSection *p);
__declspec(dllimport) void __stdcall RtlEnterCriticalSection(XBCriticalSection *p);
__declspec(dllimport) void __stdcall RtlLeaveCriticalSection(XBCriticalSection *p);
}
#endif
