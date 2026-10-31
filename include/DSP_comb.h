#ifndef DSP_COMB_H
#define DSP_COMB_H
#include "types.h"
#ifndef undenormalise
#define undenormalise(s) if (((*(unsigned int *)&(s)) & 0x7f800000) == 0) (s) = 0.0f
#endif

// Lowpass-feedback comb filter of the reverb (sizeof 0x1c).
class DSP_comb {
public:
    float process(float input)
    {
        float output = m_pBuffer[m_BufIdx];
        undenormalise(output);
        m_FilterStore = m_Damp1 * m_FilterStore + m_Damp2 * output;
        undenormalise(m_FilterStore);
        m_pBuffer[m_BufIdx] = m_FilterStore * m_Feedback + input;
        if (++m_BufIdx >= m_BufSize) m_BufIdx = 0;
        return output;
    }
    DSP_comb();
    void setbuffer(float *pBuf, int size);
    void mute();
    void setdamp(float val);
    float getdamp();
    void setfeedback(float val);
    float getfeedback();

    float m_Feedback;     // 0x00
    float m_FilterStore;  // 0x04
    float m_Damp1;        // 0x08
    float m_Damp2;        // 0x0c
    float *m_pBuffer;     // 0x10
    int m_BufSize;        // 0x14
    int m_BufIdx;         // 0x18
};
#endif
