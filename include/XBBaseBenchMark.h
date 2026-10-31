#ifndef XBBASEBENCHMARK_H
#define XBBASEBENCHMARK_H
#include "types.h"

// rdtsc-based CPU/GPU frame timers feeding g_CpuHistogram / g_GpuHistogram. sizeof 0x70.
class XBBaseBenchMark {
public:
    XBBaseBenchMark();
    s64 GetCPUTime();
    float ClksToSecs(s64 clks);
    void StartCPUTimer();
    void StopCPUTimer();
    float ConvertTimeToMs(s64 t);
    void DisplayCPUTimer();
    void DisplayGPUTimer();

    s64 m_LastFrame;    // 0x00
    s64 m_ThisFrame;    // 0x08
    s64 m_CPUStart;     // 0x10
    s64 m_CPUTime;      // 0x18
    s64 m_GPUStart;     // 0x20
    s64 m_GPUTime;      // 0x28
    s64 m_Frequency;    // 0x30 QueryPerformanceFrequency
    float m_GPUSecs;    // 0x38
    float m_CPUSecs;    // 0x3c
    int m_FrameRate;    // 0x40
    u32 m_Unk44;        // 0x44 TODO
    int m_AvgFrames;    // 0x48
    float m_CPUAccum;   // 0x4c
    float m_CPUAverage; // 0x50
    float m_GPUAccum;   // 0x54
    float m_GPUAverage; // 0x58
    int m_CPUCount;     // 0x5c
    int m_GPUCount;     // 0x60
    u32 m_Unk64;        // 0x64 TODO
    u32 m_Unk68[2];     // 0x68 TODO
};

void StartGPUTimer(int n);
void StopGPUTimer(int n);

extern XBBaseBenchMark XBBenchMark;
extern float FrameRateDivide;
extern u32 g_uiShadowEntities;
extern u32 g_uiShadowedEntities;
#endif
