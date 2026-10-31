#ifndef EXAUDIOVOICEMANAGER_H
#define EXAUDIOVOICEMANAGER_H
#include "types.h"
#include "Voice.h"
#include "SFXPoint3.h"

struct SFXPoint3;

// Owns the 32 software voices (sizeof 0xf534). TODO layout before 0xe004 and after the voices.
class EXAudioVoiceManager {
public:
    virtual void Init() = 0;
    virtual void Kill() = 0;
    virtual int GetTime() = 0;
    void MicSetPos(SFXPoint3 *, SFXPoint3 *, SFXPoint3 *, SFXPoint3 *);
    ~EXAudioVoiceManager();
    Voice *FindFreeVoice();
    int FxCreate(void *, int, int, int, bool, int, bool);
    void FxDelete(int);
    void CheckForStoppedVoices();
    void DEBUG_GetVoiceFlags(int *);
    void ResetResources();
    EXAudioVoiceManager();
    Voice *VoiceAlloc(int, int);
    Voice *GetVoice(int i) { return &m_Voices[i]; }

    SOUNDLIST_tag m_Sounds[0x800];  // 0x0004 sample slots (0 unused)
    Voice m_Voices[32];    // 0xe004
    s8 m_Active;           // 0xf504 set by Init, cleared by Kill
    int m_NextVoice;       // 0xf508 FindFreeVoice search start
    int m_NumPlaying;      // 0xf50c CheckForStoppedVoices count
    SFXPoint3 m_MicPos;    // 0xf510
    SFXPoint3 m_MicDir;    // 0xf51c
    SFXPoint3 m_MicUp;     // 0xf528
};

class XBAudioVoiceManager : public EXAudioVoiceManager {
public:
    virtual void Init();
    virtual void Kill();
    virtual int GetTime();
    XBAudioVoiceManager() {}
};

extern XBAudioVoiceManager *gpAudioVoice;
#endif
