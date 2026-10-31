#include "EXAudioCodec.h"
#include "Voice.h"
#include "EXMemory.h"

void ADPCM_Decrunch(const void *pSrc, void *pDst, int bytes, int channels, int interleaved);

static float s_DecodeBuffer[0x2000];

#define ADPCM_BYTES_TO_SAMPLES(b) (((b) * 0xe0) >> 7)
#define ADPCM_SAMPLES_TO_BYTES(s) (((s) * 0x80) / 0xe0)

template <class T> inline const T &Min(const T &a, const T &b) { return b < a ? b : a; }

EXAudioCodec::EXAudioCodec()
{
    m_pBuffer = s_DecodeBuffer;
    m_Unk14[0] = 0.0f;
    m_Unk14[1] = 0.0f;
    m_Unk14[2] = 0.0f;
    m_Unk14[3] = 0.0f;
    m_Unk14[4] = 0.0f;
    m_Unk14[5] = 0.0f;
}

EXAudioCodec::~EXAudioCodec() {}

float *EXAudioCodec::Decode(Voice *pVoice, unsigned int n)
{
    m_NumSamples = n;
    m_pVoice = pVoice;
    m_SrcSamples = pVoice->m_Pitch * n;
    m_SrcSamples = ((pVoice->m_Frac + m_SrcSamples) >> 10) + 3;
    m_Gain = pVoice->m_VolumeB * pVoice->m_VolumeA;
    Decode_();
    return m_pBuffer;
}

void EXAudioCodec::ReleaseEnvMono(float *p)
{
    for (int i = 0; i <= 80; i++) {
        p[m_NumSamples - i - 1] = (float)i * p[m_NumSamples - i - 1] * 0.0125f;
    }
}

void EXAudioCodec::ReleaseEnvStereo(float *p)
{
    for (int i = 0; i <= 80; i++) {
        p[(m_NumSamples - i) * 2 - 2] = (float)i * p[(m_NumSamples - i) * 2 - 2] * 0.0125f;
        p[(m_NumSamples - i) * 2 - 1] = (float)i * p[(m_NumSamples - i) * 2 - 1] * 0.0125f;
    }
}

void EXAudioCodec::InterpMono(float *pOut)
{
    u32 i;
    int pos = m_pVoice->m_Frac;
    if (m_pVoice->m_Pitch == 0x400) {
        for (i = 0; i < m_NumSamples; i++) {
            *pOut++ = m_pBuffer[i];
            pos += m_pVoice->m_Pitch;
        }
    } else {
        for (i = 0; i < m_NumSamples; i++) {
            *pOut++ = DoInterpMono(m_pBuffer, pos);
            pos += m_pVoice->m_Pitch;
        }
    }
    m_pVoice->m_Frac = pos & 0x3ff;
}

void EXAudioCodec::InterpStereo(float *pOut)
{
    u32 i;
    int pos = m_pVoice->m_Frac;
    if (m_pVoice->m_Pitch == 0x400) {
        for (i = 0; i < m_NumSamples; i++) {
            *pOut++ = m_pBuffer[i * 2];
            *pOut++ = m_pBuffer[i * 2 + 1];
            pos += m_pVoice->m_Pitch;
        }
    } else {
        for (i = 0; i < m_NumSamples; i++) {
            *pOut++ = DoInterpStereo(m_pBuffer, pos);
            *pOut++ = DoInterpStereo(m_pBuffer + 1, pos);
            pos += m_pVoice->m_Pitch;
        }
    }
    m_pVoice->m_Frac = pos & 0x3ff;
}

void EXAudioCodec_PCM::Decode_()
{
    u32 back = m_pVoice->m_Channels * 3 * sizeof(float);
    float *pOut = m_pBuffer;
    for (u32 i = 0; i < m_SrcSamples * m_pVoice->m_Channels; i++) {
        if (m_pVoice->m_Pos < m_pVoice->m_Length) {
            *pOut++ = ((float *)m_pVoice->m_pData)[m_pVoice->m_Pos];
            m_pVoice->m_Pos += sizeof(float);
        } else if (m_pVoice->GetFlags() & 8) {
            m_pVoice->m_Pos = m_pVoice->m_LoopStart;
            *pOut++ = ((float *)m_pVoice->m_pData)[m_pVoice->m_Pos];
            m_pVoice->m_Pos += sizeof(float);
        } else {
            m_pVoice->SetFlags(0x21);
            *pOut++ = 0.0f;
        }
    }
    if (m_pVoice->m_Pos < back) m_pVoice->m_Pos = m_pVoice->m_Length - back;
    else m_pVoice->m_Pos -= back;
}

// NEAR: same size, ebx/edi swapped (total vs output pointer) and the min() compare order.
void EXAudioCodec_ADPCM::Decode_()
{
    char *pData = m_pVoice->m_pData;
    int total = ADPCM_BYTES_TO_SAMPLES(m_pVoice->m_Length) / m_pVoice->m_Channels;
    int pos = m_ADPCMPos;
    float *pOut = m_pBuffer;
    int done = 0;
    int need = m_SrcSamples;
    if (need > 0) {
        do {
            int a = need - done;
            int b = total - pos;
            int n = Min(a, b);
            if (n <= 0) break;
            int blk = pos / 0x38;
            int rem = pos % 0x38;
            ADPCM_Decrunch(pData + ADPCM_SAMPLES_TO_BYTES(m_pVoice->m_Channels * blk * 0x38), pOut - m_pVoice->m_Channels * rem,
                           (n + rem + 0x37) / 0x38 * m_pVoice->m_Channels * 0x38 * (int)sizeof(float), m_pVoice->m_Channels, 1);
            done += n;
            pOut += m_pVoice->m_Channels * n;
            pos += n;
            if (pos >= total) {
                if (m_pVoice->GetFlags() & 8) {
                    pos = (int)(ADPCM_BYTES_TO_SAMPLES(m_pVoice->m_LoopStart) / m_pVoice->m_Channels) / 0x38 * 0x38;
                } else {
                    m_pVoice->SetFlags(0x81);
                    if (done < need) {
                        int left = need - done;
                        if (m_pVoice->m_Channels == 2) {
                            u32 l = ((u32 *)pOut)[-2];
                            u32 r = ((u32 *)pOut)[-1];
                            do {
                                *(u32 *)pOut = l;
                                pOut++;
                                *(u32 *)pOut = r;
                                pOut++;
                            } while (--left);
                        } else {
                            u32 v = ((u32 *)pOut)[-1];
                            do {
                                *(u32 *)pOut = v;
                                pOut++;
                            } while (--left);
                        }
                    }
                    pos = 0;
                    goto out;
                }
            }
        } while (done < need);
    }
    if (pos < 0) pos += ADPCM_BYTES_TO_SAMPLES(m_pVoice->m_Length) / m_pVoice->m_Channels;
out:
    pos -= 3;
    m_ADPCMPos = pos;
    m_pVoice->m_Pos = ADPCM_SAMPLES_TO_BYTES(m_pVoice->m_Channels * pos);
}
