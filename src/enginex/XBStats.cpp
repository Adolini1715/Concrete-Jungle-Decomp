#include "EXStats.h"
#include "EXMemory.h"

extern "C" u32 __stdcall GetTickCount();

EXStats::EXStats()
{
    m_StartTime = GetTickCount();
}

EXStats::~EXStats() {}

u32 EXStats::UpTime()
{
    return GetTickCount() - m_StartTime;
}

float EXStats::TicksToMillisecs(u64 ticks)
{
    return (s64)(ticks * 3 / 2200) * 0.001;
}

void EXStats::ResetDisplayLine()
{
    m_LineStart = Tick();
}

u16 EXStats::DisplayLine()
{
    return (u16)((Tick() - m_LineStart) * 256 / m_TicksPerLine);
}

