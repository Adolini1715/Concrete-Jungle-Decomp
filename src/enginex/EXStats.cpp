#include "EXStats.h"
#include "EXHistogram.h"
#include "EXMemory.h"

static const int s_FrameTimeBins[] = {
    0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80, 90, 100, 110, 120, 200, 500, 1000, 0x40000000
};

EXStats g_Stats;
EXHistogram g_CpuHistogram(s_FrameTimeBins);
EXHistogram g_GpuHistogram(s_FrameTimeBins);

EXBaseStats::EXBaseStats()
{
    m_FrameCount = 0xffffffff;
    m_GameTime = 0;
}

EXBaseStats::~EXBaseStats() {}

void EXBaseStats::FrameUpdate(s8)
{
    m_FrameCount++;
    m_GameTime += 0x10aaaa;
}

void EXHistogram::Reset()
{
    int i;
    m_NumCounts = 0;
    m_Total = 0;
    for (i = 0; i < m_NumBins; i++) m_pCounts[i] = 0;
    m_Index = 0;
    for (i = 0; i < 32; i++) m_Window[i] = 0;
}

void EXHistogram::Add(int v)
{
    if (!m_pBins || !m_pCounts || m_Paused) return;
    m_Window[m_Index] = v;
    m_Index++;
    if (m_Index >= 32) m_Index = 0;
    int n = m_NumBins;
    int lo = m_pBins[0];
    for (int i = 0; i < m_NumBins; i++) {
        int hi = m_pBins[i + 1];
        if (v >= lo && ((i < n - 1 && v < hi) || (i == n - 1 && v <= hi))) {
            m_pCounts[i]++;
            m_Total += v;
            m_NumCounts++;
            return;
        }
        lo = hi;
    }
}

void EXHistogram::BinData(int bin, int &lo, int &hi, int &count, int &percent) const
{
    lo = m_pBins[bin];
    hi = m_pBins[bin + 1];
    count = m_pCounts[bin];
    percent = 0;
    if (m_NumCounts) percent = (int)((float)m_pCounts[bin] / (float)m_NumCounts * 100.0f + 0.5f);
}

float EXHistogram::Average() const
{
    int n = m_NumCounts;
    if (n) return (float)m_Total / n;
    return 0.0f;
}

float EXHistogram::MovingAverage() const
{
    int sum = 0;
    for (int i = 0; i < 32; i++) sum += m_Window[i];
    return sum * 0.03125f;
}

EXHistogram::EXHistogram()
{
    m_NumBins = 0;
    m_pBins = 0;
    m_pCounts = 0;
    m_NumCounts = 0;
    UnPause();
}

EXHistogram::EXHistogram(const int *pBins)
{
    int i = 0;
    while (pBins[i] != 0x40000000) i++;
    m_NumBins = i - 1;
    m_pBins = (int *)_EXAlloc((m_NumBins + 1) * sizeof(int), 0);
    for (i = 0; i < m_NumBins + 1; i++) m_pBins[i] = pBins[i];
    m_pCounts = (int *)_EXAlloc(m_NumBins * sizeof(int), 0);
    Reset();
}

EXHistogram::~EXHistogram()
{
    if (m_pCounts) {
        EXFree(m_pCounts);
        m_pCounts = 0;
    }
    if (m_pBins) {
        EXFree(m_pBins);
        m_pBins = 0;
    }
}
