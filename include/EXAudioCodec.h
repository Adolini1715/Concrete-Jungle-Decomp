#ifndef EXAUDIOCODEC_H
#define EXAUDIOCODEC_H
#include "types.h"

class Voice;

// Decodes a voice's sample data into a float buffer and resamples it (sizeof 0x3c).
class EXAudioCodec {
public:
    virtual void Reset_() = 0;
    virtual void Decode_() = 0;
    EXAudioCodec();
    ~EXAudioCodec();
    float *Decode(Voice *pVoice, unsigned int n);
    float DoInterpMono(float *p, int pos)
    {
        int i = pos >> 10;
        pos &= 0x3ff;
        float v0 = p[i];
        float v1 = p[i + 1];
        return (v1 - v0) * ((float)pos * (1.0f / 1024)) + v0;
    }
    float DoInterpStereo(float *p, int pos)
    {
        int i = (pos >> 9) & ~1;
        pos &= 0x3ff;
        float v0 = p[i];
        float v1 = p[i + 2];
        return (v1 - v0) * ((float)pos * (1.0f / 1024)) + v0;
    }
    void ReleaseEnvMono(float *p);
    void ReleaseEnvStereo(float *p);
    void InterpMono(float *pOut);
    void InterpStereo(float *pOut);

    float *m_pBuffer;      // 0x04 decoded source samples
    Voice *m_pVoice;       // 0x08
    u32 m_NumSamples;      // 0x0c output samples
    u32 m_SrcSamples;      // 0x10 source samples needed
    float m_Unk14[6];      // 0x14
    float m_Gain;          // 0x2c
    int m_ADPCMPos;        // 0x30
    int m_Unk34;           // 0x34
    int m_Unk38;           // 0x38
};

class EXAudioCodec_PCM : public EXAudioCodec {
private:
    virtual void Decode_();
};

class EXAudioCodec_ADPCM : public EXAudioCodec {
public:
    virtual void Reset_()
    {
        m_ADPCMPos = 0;
        m_Unk38 = 0;
        m_Unk34 = 0;
    }
    virtual void Decode_();
    EXAudioCodec_ADPCM() {}
};
#endif
