#include "revmodel.h"

extern "C" void *__cdecl memset(void *, int, unsigned int);

// Tuning constants (const floats are referenced from memory when the other operand is variable).
static const float fixedgain = 0.06f;
static const float scalewet = 3.0f;
static const float scaledry = 2.0f;
static const float scaledamp = 0.4f;
static const float scaleroom = 0.28f;
static const float offsetroom = 0.7f;
static const float initialroom = 0.5f;
static const float initialdamp = 0.9f;
static const float initialdry = 0.0f;
static const float initialwidth = 1.0f;
static float initialwet = 1 / scalewet;

DSP_allpass::DSP_allpass()
{
    m_BufIdx = 0;
}

void DSP_allpass::setbuffer(float *pBuf, int size)
{
    m_pBuffer = pBuf;
    m_BufSize = size;
}

void DSP_allpass::mute()
{
    for (int i = 0; i < m_BufSize; i++) m_pBuffer[i] = 0.0f;
}

DSP_comb::DSP_comb()
{
    m_FilterStore = 0.0f;
    m_BufIdx = 0;
}

void DSP_comb::setbuffer(float *pBuf, int size)
{
    m_pBuffer = pBuf;
    m_BufSize = size;
}

void DSP_comb::mute()
{
    for (int i = 0; i < m_BufSize; i++) m_pBuffer[i] = 0.0f;
}

void DSP_comb::setdamp(float val)
{
    m_Damp1 = val;
    m_Damp2 = 1.0f - val;
}

float DSP_comb::getdamp()
{
    return m_Damp1;
}

void DSP_comb::setfeedback(float val)
{
    m_Feedback = val;
}

float DSP_comb::getfeedback()
{
    return m_Feedback;
}

void revmodel::mute()
{
    m_CombL[0].mute();
    m_CombL[2].mute();
    m_CombL[4].mute();
    m_CombL[6].mute();
    m_AllpassL[0].mute();
    m_AllpassR[0].mute();
    memset(m_PreDelay, 0, sizeof(m_PreDelay));
    m_LowPassL = 0.0f;
    m_LowPassR = 0.0f;
}

void revmodel::update()
{
    m_Wet1 = m_Wet * ((m_Width + 1.0f) / 2);
    m_Wet2 = m_Wet * ((1.0f - m_Width) / 2);
    m_RoomSize1 = m_RoomSize;
    m_Damp1 = m_Damp;
    m_Gain = fixedgain;
    m_CombL[0].setfeedback(m_RoomSize1);
    m_CombL[0].setdamp(m_Damp1);
    m_CombL[2].setfeedback(m_RoomSize1);
    m_CombL[2].setdamp(m_Damp1);
    m_CombL[4].setfeedback(m_RoomSize1);
    m_CombL[4].setdamp(m_Damp1);
    m_CombL[6].setfeedback(m_RoomSize1);
    m_CombL[6].setdamp(m_Damp1);
}

void revmodel::setroomsize(float value)
{
    m_RoomSize = value * scaleroom + offsetroom;
}

float revmodel::getroomsize()
{
    return (m_RoomSize - offsetroom) * (1 / scaleroom);
}

void revmodel::setdamp(float value)
{
    m_Damp = value * scaledamp;
}

void revmodel::setlowpass(float value)
{
    m_LowPass = value;
}

void revmodel::setpredelay(float value)
{
    m_PreDelayLen = (int)(value * 4096.0f);
    if (m_PreDelayLen == 0x1000) m_PreDelayLen = 0xfff;
}

float revmodel::getdamp()
{
    return m_Damp * (1 / scaledamp);
}

void revmodel::setwet(float value)
{
    m_Wet = value * scalewet;
}

float revmodel::getwet()
{
    return m_Wet * (1 / scalewet);
}

void revmodel::setdry(float value)
{
    m_Dry = value * scaledry;
}

float revmodel::getdry()
{
    return m_Dry * (1 / scaledry);
}

void revmodel::setwidth(float value)
{
    m_Width = value;
}

float revmodel::getwidth()
{
    return m_Width;
}

revmodel::revmodel()
{
    m_CombL[0].setbuffer(m_BufCombL1, 1116);
    m_CombL[2].setbuffer(m_BufCombL3, 1277);
    m_CombL[4].setbuffer(m_BufCombL5, 1422);
    m_CombL[6].setbuffer(m_BufCombL7, 1557);
    m_AllpassL[0].setbuffer(m_BufAllpassL1, 556);
    m_AllpassR[0].setbuffer(m_BufAllpassR1, 579);
    setwet(initialwet);
    setroomsize(initialroom);
    setdry(initialdry);
    setdamp(initialdamp);
    setwidth(initialwidth);
    setlowpass(0.85f);
    update();
    m_PreDelayLen = 0;
    m_PreDelayIdx = 0;
    mute();
}
