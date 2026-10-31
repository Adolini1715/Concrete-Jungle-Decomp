#ifndef EXAUDIO_SOUNDBANK_H
#define EXAUDIO_SOUNDBANK_H
#include "types.h"
#include "EXAudio_Fader.h"

// Loaded sound bank slot (EXAudioBase.obj), sizeof 0x48. TODO names.
class EXAudio_SoundBank {
public:
    struct Entry {
        int m_Hash;    // 0x0
        int m_Offset;  // 0x4 into m_pSFXData
    };
    EXAudio_SoundBank();
    ~EXAudio_SoundBank();

    int m_Unk00;            // 0x00
    int m_Unk04;            // 0x04
    int m_Unk08;            // 0x08
    char *m_pSFXData;       // 0x0c SFXParameters blocks
    int m_NumSFX;           // 0x10
    Entry *m_pSFXTable;     // 0x14
    int m_Loaded;           // 0x18
    int m_Unk1c;            // 0x1c
    int m_Hash;             // 0x20
    int m_Unk24;            // 0x24
    int m_Unk28;            // 0x28
    EXAudio_Fader m_Fader;  // 0x2c
    int m_Unk3c;            // 0x3c
    int m_Status;           // 0x40 2: release pending
    int m_Unk44;            // 0x44
};
#endif
