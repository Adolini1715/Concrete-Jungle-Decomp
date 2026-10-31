#ifndef AUDIOMIXER_H
#define AUDIOMIXER_H
#include "types.h"
#include "EXCriticalSection.h"
#include "revmodel.h"

class Voice;

// Software mixer: decodes the active voices and mixes them with reverb (sizeof 0x12150).
class AudioMixer {
public:
    virtual int Init() = 0;
    virtual void Shutdown() = 0;
protected:
    virtual void Mix_() = 0;
public:
    void SetMasterReverb(int level) { m_ReverbLevel = level * 0.01f; }
    int GetPrimaryFreq() { return m_Frequency; }
    AudioMixer();
    ~AudioMixer();
    void DecodeVoice(Voice *pVoice);
    void Mix(unsigned int numSamples);
    void Mix_FiveOne(short *p0, short *p1, short *p2, short *p3, short *p4);

    int m_Frequency;               // 0x04
    int m_BufferSize;              // 0x08
    int m_BlockSize;               // 0x0c
    int m_Active;                  // 0x10
    int m_WritePos;                // 0x14
    unsigned int m_NumSamples;     // 0x18
    EXCriticalSection m_Lock;      // 0x1c
    revmodel m_Reverb;             // 0x38
    u8 m_Unkffb8[0x208c];          // 0xffb8 TODO layout
    float m_ReverbLevel;           // 0x12044
    Voice *m_MonoVoices[32];       // 0x12048
    int m_NumMono;                 // 0x120c8
    Voice *m_StereoVoices[32];     // 0x120cc
    int m_NumStereo;               // 0x1214c
};
#endif
