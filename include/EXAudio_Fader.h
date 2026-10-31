#ifndef EXAUDIO_FADER_H
#define EXAUDIO_FADER_H
#include "types.h"

// Volume fader used for music, SFX and sound banks (EXAudioBase.obj), sizeof 0x10.
class EXAudio_Fader {
public:
    EXAudio_Fader();
    ~EXAudio_Fader();
    void FaderClear();
    void FaderEndUpdate(int);
    void FaderUpdate(int);

    int m_State;    // 0x00 0 idle, 1 fading up, else fading down (-1 on the music fader: cutscene hold)
    int m_Volume;   // 0x04 0..10000000
    int m_Speed;    // 0x08 per tick
    u8 m_Stop;      // 0x0c at the end of a fade down: stop (1) or pause (0)
    u8 m_Music;     // 0x0d fades the music rather than the SFX
    u8 m_Bank;      // 0x0e fades a sound bank out before releasing it
    u8 m_Wait;      // 0x0f wait while the music fader holds (-1)
};
#endif
