#ifndef EXAUDIO_VOICEMANAGER_H
#define EXAUDIO_VOICEMANAGER_H
#include "types.h"
#include "EXAudio_Voice.h"

struct SampleHeaderData;

// Pool of 100 EuroSound voice handles (EXAudioBase.obj), sizeof 0x640.
class EXAudio_VoiceManager {
public:
    EXAudio_Voice *ES_FindFreeVoiceHandle();
    EXAudio_Voice *ES_RequestVoiceHandle(SampleHeaderData *, int, int, int, int, int, int);
    EXAudio_VoiceManager();
    ~EXAudio_VoiceManager();

    EXAudio_Voice m_Voices[100];  // 0x000
};
#endif
