#ifndef DSP_ALLPASS_H
#define DSP_ALLPASS_H
#include "types.h"
#ifndef undenormalise
#define undenormalise(s) if (((*(unsigned int *)&(s)) & 0x7f800000) == 0) (s) = 0.0f
#endif

// Allpass filter of the reverb (sizeof 0xc; the feedback is a constant).
class DSP_allpass {
public:
    float process(float input)
    {
        float bufout = m_pBuffer[m_BufIdx];
        undenormalise(bufout);
        float output = bufout - input;
        m_pBuffer[m_BufIdx] = bufout * 0.5f + input;
        if (++m_BufIdx >= m_BufSize) m_BufIdx = 0;
        return output;
    }
    DSP_allpass();
    void setbuffer(float *pBuf, int size);
    void mute();

    float *m_pBuffer;  // 0x00
    int m_BufSize;     // 0x04
    int m_BufIdx;      // 0x08
};
#endif
