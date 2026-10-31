#ifndef VOICE_H
#define VOICE_H
#include "types.h"

struct SFXPoint3;
class EXAudioCodec;

// Sound sample slot owned by EXAudioVoiceManager (sizeof 0x1c); index 0 is never used.
struct SOUNDLIST_tag {
    int m_Used;          // 0x00
    u32 m_Length;        // 0x04 bytes per channel
    u16 m_SampleRate;    // 0x08
    int m_Channels;      // 0x0c
    u32 m_LoopStart;     // 0x10
    char *m_pData;       // 0x14
    bool m_Loop;         // 0x18
    bool m_Owned;        // 0x19 m_pData came from psiAsyncCreateBuffer
};

// Software mixer voice (EXAudioVoice.obj), sizeof 0xa8. m_Flags is guarded by gpMixer's lock.
// Flags: 1 playing, 8 looping, 0x20 finished, 0x80 ADPCM end ...
class Voice {
public:
    void SetReverb(float level) { m_Reverb = level * 0.01f; }
    ~Voice();
    void NormalisePoint3(SFXPoint3 *);
    float DotP_Point3(SFXPoint3 *, SFXPoint3 *);
    int VoiceGetSound();
    void VoiceLock();
    void VoiceUnlock();
    bool IsVoiceLocked();
    void SetFlags(int);
    void AndFlags(int);
    void OrFlags(int);
    int GetFlags();
    void SetVolume(int);
    void SetFrequency_();
    void SetFrequency(int);
    void SetLowPassFilter(float);
    void SetLowPassFilterHRTF(float);
    void UpdateVolumes_51_();
    void UpdateVolumes_71_();
    void UpdateVolumes_Mono_();
    void UpdateVolumes_Stereo_();
    void SetPanLR(float);
    void SetPanFR(float);
    void SetPan(float, float);
    void GetWrite(unsigned int, unsigned int, void **, unsigned int *, void **, unsigned int *);
    void VoiceMemCpy(int, void *, int);
    int VoiceGetBytePos();
    void StreamPause();
    void StreamUnPause();
    void StreamStop();
    void VoiceSetVolume(float);
    void VoiceSet2dPan(float);
    void VoiceSetFreq(int);
    void Reset_(s8 full);
    bool VoiceIsPlaying();
    void VoiceSetMode(int);
    void VoiceSetPos(SFXPoint3 *);
    void VoiceSetRadius(float, float);
    void Play(int);
    void Pause();
    void Stop();
    void Calculate3dVolume();
    void UpdateVolumes();
    Voice();
    void VoicePlay(float);
    void VoiceStop(bool);
    void StreamPlay(float);
    void Update();
    void InitVoice(SOUNDLIST_tag *pSound, int mode);

    volatile int m_Flags;      // 0x00 volatile: every access is a separate load/store
    SOUNDLIST_tag *m_pSound;   // 0x04
    s8 m_Is3D;                 // 0x08
    float m_Volume;            // 0x0c 0..100
    float m_Pos3D[3];          // 0x10 VoiceSetPos
    float m_MinRadius;         // 0x1c
    float m_MaxRadius;         // 0x20
    u8 m_Unk24;                // 0x24
    bool m_Locked;             // 0x25
    int m_Channels;            // 0x28
    float *m_pMixBuffer;       // 0x2c 0x8000 bytes
    EXAudioCodec *m_pCodec;    // 0x30
    float *m_pMixRead;         // 0x34 read pointer into m_pMixBuffer
    u32 m_LoopStart;           // 0x38
    char *m_pData;             // 0x3c
    u32 m_Length;              // 0x40
    u32 m_Pos;                 // 0x44
    u32 m_Pitch;               // 0x48 10-bit fixed point step
    u32 m_Frac;                // 0x4c 10-bit fraction of the position
    u32 m_SampleRate;          // 0x50
    u32 m_BaseRate;            // 0x54
    float m_Gain;              // 0x58 SetVolume (0..1)
    float m_TargetVol[6];      // 0x5c per speaker (5.1)
    float m_CurVol[6];         // 0x74 ramped towards m_TargetVol
    u8 m_Snap;                 // 0x8c jump straight to the target volumes
    float m_PanLR;             // 0x90
    float m_PanFR;             // 0x94
    int m_Mode;                // 0x98
    float m_Reverb;            // 0x9c
    float m_VolumeA;           // 0xa0 SetLowPassFilter
    float m_VolumeB;           // 0xa4 SetLowPassFilterHRTF
};
#endif
