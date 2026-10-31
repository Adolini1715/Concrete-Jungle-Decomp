#ifndef EXSTATS_H
#define EXSTATS_H
#include "EXBaseStats.h"

// Xbox statistics: rdtsc timing (733 MHz) and GetTickCount uptime. sizeof 0x30 (g_Stats).
class EXStats : public EXBaseStats {
public:
    virtual ~EXStats();
    EXStats();
    u32 UpTime();
    static float TicksToMillisecs(u64 ticks);
    // Not inlined in retail (callers call it); noinline reproduces that.
    static __declspec(noinline) u64 __stdcall Tick() { __asm rdtsc }
    void ResetDisplayLine();
    u16 DisplayLine();
protected:
    u32 m_TicksPerLine; // 0x18
    u32 m_Unk1C;        // 0x1c TODO
    u64 m_LineStart;    // 0x20
    u32 m_StartTime;    // 0x28 GetTickCount() at construction
    u32 m_Unk2C;        // 0x2c TODO
};

extern EXStats g_Stats;
#endif
