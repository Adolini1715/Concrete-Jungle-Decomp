#ifndef REVMODEL_H
#define REVMODEL_H
#include "types.h"
#include "DSP_comb.h"
#include "DSP_allpass.h"

// Reverb model: predelay line, combs and allpasses (only every other comb and the first
// allpass of each side are wired up). sizeof at least 0xff80. TODO layout after 0xf674.
class revmodel {
public:
    // NEAR (emitted in EXAudioMixer.obj): retail computes the gain multiply in SSE, ours in x87.
    void process(float input, float &outL, float &outR)
    {
        m_PreDelay[m_PreDelayIdx] = input;
        input = m_Gain * m_PreDelay[(m_PreDelayIdx - m_PreDelayLen) & 0xfff];
        m_PreDelayIdx = (m_PreDelayIdx + 1) & 0xfff;
        float accL = m_CombL[0].process(input);
        accL += m_CombL[2].process(input);
        float accR = m_CombL[4].process(input);
        accL *= 0.5f;
        accR += m_CombL[6].process(input);
        accR *= 0.5f;
        accR = m_AllpassR[0].process(accR);
        accL = m_AllpassL[0].process(accL);
        outL = m_Wet1 * accL + m_Wet2 * accR;
        outR = m_Wet2 * accL + m_Wet1 * accR;
    }
    void mute();
    void update();
    void setroomsize(float value);
    float getroomsize();
    void setdamp(float value);
    void setlowpass(float value);
    void setpredelay(float value);
    float getdamp();
    void setwet(float value);
    float getwet();
    void setdry(float value);
    float getdry();
    void setwidth(float value);
    float getwidth();
    revmodel();

    float m_Gain;              // 0x00
    float m_RoomSize;          // 0x04
    float m_RoomSize1;         // 0x08
    float m_Damp;              // 0x0c
    float m_Damp1;             // 0x10
    float m_Wet;               // 0x14
    float m_Wet1;              // 0x18
    float m_Wet2;              // 0x1c
    float m_Dry;               // 0x20
    float m_Width;             // 0x24
    float m_Unk28;             // 0x28
    float m_LowPassL;          // 0x2c filter state
    float m_LowPassR;          // 0x30 filter state
    float m_Unk34;             // 0x34
    float m_LowPass;           // 0x38
    float m_PreDelay[0x1000];  // 0x3c
    int m_PreDelayIdx;         // 0x403c
    int m_PreDelayLen;         // 0x4040
    DSP_comb m_CombL[8];       // 0x4044
    DSP_allpass m_AllpassL[4]; // 0x4124
    DSP_allpass m_AllpassR[4]; // 0x4154
    float m_BufCombL1[1116];   // 0x4184
    float m_BufCombL2[1188];
    float m_BufCombL3[1277];   // 0x6584
    float m_BufCombL4[1356];
    float m_BufCombL5[1422];   // 0x8ea8
    float m_BufCombL6[1491];
    float m_BufCombL7[1557];   // 0xbc2c
    float m_BufCombL8[1617];
    float m_BufAllpassL1[556]; // 0xedc4
    float m_BufAllpassR1[579]; // 0xf674 TODO: size unverified
};
#endif
