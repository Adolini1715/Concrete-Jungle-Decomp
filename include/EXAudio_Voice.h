#ifndef EXAUDIO_VOICE_H
#define EXAUDIO_VOICE_H
#include "types.h"

class SFXItem;

// EuroSound voice handle (EXAudioBase.obj), sizeof 0x10: a platform sample voice, a stream, or a sub-SFX.
class EXAudio_Voice {
public:
    EXAudio_Voice();
    ~EXAudio_Voice();
    void ES_AudioKeyOn(int);
    void ES_AudioKeyOff();
    void ES_UnLockVoiceHandle();
    int ES_AudioIsPlaying();
    int ES_psiFindVoiceHandle();
    int ES_IsVoiceHandleStreamed();
    int ES_IsVoiceHandleSubSFX();
    int ES_GetStreamShutdownVolume();
    void ES_AudioPause();
    void ES_AudioUnPause();
    void ES_AudioSetVolume(int);
    int AudioHasEnded(SFXItem *);

    int m_Used;      // 0x00
    int m_SubSFX;    // 0x04 m_Handle is the object of a sub-SFX
    int m_Streamed;  // 0x08 m_Handle is a stream
    int m_Handle;    // 0x0c psi voice handle, stream or sub-SFX object
};
#endif
