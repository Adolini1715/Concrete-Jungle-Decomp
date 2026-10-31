#include "XBBaseBenchMark.h"
#include "EXHistogram.h"

extern "C" int __stdcall QueryPerformanceFrequency(s64 *pFreq);

// Initialised from a const float: the front end treats that as a dynamic initialiser (it uses up
// $E1/$E2, making the benchmark object's initialiser $E3) before folding it to zero data.
static const float s_FrameRateDivideInit = 0.0f;
float FrameRateDivide = s_FrameRateDivideInit;
u32 g_uiShadowEntities;
u32 g_uiShadowedEntities;
XBBaseBenchMark XBBenchMark;

XBBaseBenchMark::XBBaseBenchMark()
{
    QueryPerformanceFrequency(&m_Frequency);
    m_FrameRate = 60;
    m_CPUCount = 0;
    m_GPUCount = 0;
    m_CPUAccum = 0.0f;
    m_AvgFrames = 120;
    m_CPUAverage = 0.0f;
    m_GPUAverage = 0.0f;
    m_GPUAccum = 0.0f;
    m_LastFrame = 0;
    m_ThisFrame = 0;
    m_Unk64 = 0;
}

s64 XBBaseBenchMark::GetCPUTime()
{
    s64 t = 0;
    __asm {
        rdtsc
        mov dword ptr [t], eax
        mov dword ptr [t + 4], edx
    }
    return t;
}

float XBBaseBenchMark::ClksToSecs(s64 clks)
{
    return (float)clks / (float)m_Frequency;
}

void StopGPUTimer(int)
{
    XBBenchMark.m_GPUTime = XBBenchMark.GetCPUTime() - XBBenchMark.m_GPUStart;
}

void StartGPUTimer(int)
{
    XBBenchMark.m_GPUStart = XBBenchMark.GetCPUTime();
}

void XBBaseBenchMark::StartCPUTimer()
{
    m_CPUStart = GetCPUTime();
}

void XBBaseBenchMark::StopCPUTimer()
{
    m_CPUTime = GetCPUTime() - m_CPUStart;
}

float XBBaseBenchMark::ConvertTimeToMs(s64 t)
{
    return (float)t / (float)m_Frequency;
}

// NEAR: tail differs: retail rounds s to a float stack temp and does the accumulate in SSE
// (movss/addss); ours keeps s on the x87 stack. Same for DisplayGPUTimer.
void XBBaseBenchMark::DisplayCPUTimer()
{
    m_ThisFrame = GetCPUTime();
    if (m_LastFrame) m_FrameRate = (int)(1.0f / ClksToSecs(m_ThisFrame - m_LastFrame));
    m_LastFrame = GetCPUTime();
    if (m_CPUCount == m_AvgFrames) {
        m_CPUAverage = m_CPUAccum / m_AvgFrames;
        m_CPUAccum = 0.0f;
        m_CPUCount = 0;
    }
    m_CPUCount++;
    float s = ClksToSecs(m_CPUTime);
    m_CPUSecs = s;
    m_CPUAccum += s;
    g_CpuHistogram.Add((int)(1.0f / s));
}

// NEAR: see DisplayCPUTimer.
void XBBaseBenchMark::DisplayGPUTimer()
{
    if (m_GPUCount == m_AvgFrames) {
        m_GPUAverage = m_GPUAccum / m_AvgFrames;
        m_GPUAccum = 0.0f;
        m_GPUCount = 0;
    }
    m_GPUCount++;
    float s = ClksToSecs(m_GPUTime);
    m_GPUSecs = s;
    m_GPUAccum += s;
    g_GpuHistogram.Add((int)(1.0f / s));
}
