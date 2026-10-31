#ifndef EXHISTOGRAM_H
#define EXHISTOGRAM_H
#include "types.h"

// Binned value counter with a 32-entry moving window. Bin tables end with 0x40000000. sizeof 0x98.
class EXHistogram {
public:
    void Reset();
    void Add(int v);
    void BinData(int bin, int &lo, int &hi, int &count, int &percent) const;
    float Average() const;
    float MovingAverage() const;
    void UnPause() { m_Paused = 0; }
    EXHistogram();
    EXHistogram(const int *pBins);
    ~EXHistogram();
    int NumBins() const { return m_NumBins; }
    int NumCounts() const { return m_NumCounts; }
private:
    s16 m_NumBins;     // 0x00
    s16 m_Index;       // 0x02 moving window position
    int *m_pBins;      // 0x04 NumBins + 1 boundaries
    int *m_pCounts;    // 0x08
    int m_NumCounts;   // 0x0c
    int m_Window[32];  // 0x10
    int m_Total;       // 0x90
    s8 m_Paused;       // 0x94
};

extern EXHistogram g_CpuHistogram;
extern EXHistogram g_GpuHistogram;
#endif
