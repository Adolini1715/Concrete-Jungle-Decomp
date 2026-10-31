#include "AudioMixer.h"
#include "EXAudioVoiceManager.h"
#include "EXAudioCodec.h"
#include "Voice.h"

extern "C" void *__cdecl memcpy(void *, const void *, unsigned int);


AudioMixer::AudioMixer()
{
    m_Active = 0;
    m_ReverbLevel = 0.0f;
    m_Frequency = 44100;
    m_BlockSize = 0x1000;
    m_BufferSize = 0x4000;
    m_NumSamples = 0;
    m_Reverb.setroomsize(0.75f);
    m_Reverb.setwidth(1.0f);
    m_Reverb.setdamp(0.8f);
    m_Reverb.setlowpass(0.5f);
    m_Reverb.setpredelay(0.1f);
    m_Reverb.update();
}

AudioMixer::~AudioMixer() {}

void AudioMixer::DecodeVoice(Voice *pVoice)
{
    if (pVoice->m_pCodec) {
        pVoice->m_pCodec->Decode(pVoice, m_NumSamples);
        if (pVoice->m_Channels == 1) {
            pVoice->m_pCodec->InterpMono(pVoice->m_pMixBuffer);
            if (pVoice->GetFlags() & 0x10) {
                pVoice->m_pCodec->ReleaseEnvMono(pVoice->m_pMixBuffer);
                pVoice->Reset_(1);
            } else if (pVoice->GetFlags()) {
            }
        } else {
            pVoice->m_pCodec->InterpStereo(pVoice->m_pMixBuffer);
            if (pVoice->GetFlags() & 0x10) {
                pVoice->m_pCodec->ReleaseEnvStereo(pVoice->m_pMixBuffer);
                pVoice->Reset_(1);
            } else if (pVoice->GetFlags()) {
            }
        }
    }
    pVoice->m_pMixRead = pVoice->m_pMixBuffer;
}

void AudioMixer::Mix(unsigned int numSamples)
{
    if (!m_Active) return;
    m_NumSamples = numSamples;
    m_NumMono = 0;
    m_NumStereo = 0;
    int i;
    for (i = 0; i < 32; i++) {
        Voice *pVoice = gpAudioVoice->GetVoice(i);
        int flags = pVoice->GetFlags();
        if (flags) {
            for (int j = 0; j < 6; j++) undenormalise(pVoice->m_TargetVol[j]);
            if (flags & 0xb0) {
                if (flags & 0x80) {
                    pVoice->SetFlags(0x41);
                } else if (pVoice->m_Channels == 1) {
                    m_MonoVoices[m_NumMono++] = pVoice;
                } else {
                    m_StereoVoices[m_NumStereo++] = pVoice;
                }
            } else if ((flags & 5) == 5) {
                pVoice->Update();
                if (pVoice->m_Channels == 1) m_MonoVoices[m_NumMono++] = pVoice;
                else m_StereoVoices[m_NumStereo++] = pVoice;
            }
        }
    }
    unsigned int k;
    for (k = 0; k < (unsigned int)m_NumMono; k++) DecodeVoice(m_MonoVoices[k]);
    for (k = 0; k < (unsigned int)m_NumStereo; k++) DecodeVoice(m_StereoVoices[k]);
    Mix_();
}

static inline short ClampSample(float x)
{
    if (x > 32767.0f) x = 32767.0f;
    else if (x < -32768.0f) x = -32768.0f;
    return (short)x;
}

// NEAR: retail keeps all five accumulators in memory; ours promotes some to registers.
void AudioMixer::Mix_FiveOne(short *p0, short *p1, short *p2, short *p3, short *p4)
{
    int i, j;
    for (i = 0; i < m_NumMono; i++) {
        Voice *pVoice = m_MonoVoices[i];
        if (pVoice->m_Snap) {
            pVoice->m_Snap = 0;
            memcpy(pVoice->m_CurVol, pVoice->m_TargetVol, sizeof(pVoice->m_CurVol));
        } else {
            for (j = 0; j < 6; j++) {
                pVoice->m_CurVol[j] = (pVoice->m_TargetVol[j] - pVoice->m_CurVol[j]) * 0.1f + pVoice->m_CurVol[j];
                undenormalise(pVoice->m_CurVol[j]);
            }
        }
    }
    for (int s = 0; s < (int)m_NumSamples; s++) {
        float acc[5];
        float rev;
        acc[0] = 0.0f;
        acc[1] = 0.0f;
        acc[2] = 0.0f;
        acc[3] = 0.0f;
        acc[4] = 0.0f;
        rev = 0.0f;
        for (i = 0; i < m_NumMono; i++) {
            Voice *pVoice = m_MonoVoices[i];
            float x = *pVoice->m_pMixRead++;
            acc[0] += pVoice->m_CurVol[0] * x;
            acc[1] += pVoice->m_CurVol[1] * x;
            acc[2] += pVoice->m_CurVol[2] * x;
            acc[3] += pVoice->m_CurVol[3] * x;
            acc[4] += pVoice->m_CurVol[4] * x;
            rev += pVoice->m_CurVol[5] * x;
        }
        for (i = 0; i < m_NumStereo; i++) {
            Voice *pVoice = m_StereoVoices[i];
            float l = *pVoice->m_pMixRead++;
            float r = *pVoice->m_pMixRead++;
            acc[0] += pVoice->m_TargetVol[0] * l;
            acc[1] += pVoice->m_TargetVol[1] * r;
            acc[2] += pVoice->m_TargetVol[2] * l;
            acc[3] += pVoice->m_TargetVol[3] * r;
            acc[4] += (r + l) * pVoice->m_TargetVol[4];
        }
        float revL, revR;
        m_Reverb.process(rev, revL, revR);
        revL = m_ReverbLevel * revL;
        revR = m_ReverbLevel * revR;
        *p0++ = ClampSample(revL + acc[0]);
        *p1++ = ClampSample(revR + acc[1]);
        *p2++ = ClampSample(revL + acc[2]);
        *p3++ = ClampSample(revR + acc[3]);
        *p4++ = ClampSample((revL + revR) * 0.5f + acc[4]);
    }
    m_WritePos = (m_WritePos + m_NumSamples * 2) & 0x1ffe;
}
