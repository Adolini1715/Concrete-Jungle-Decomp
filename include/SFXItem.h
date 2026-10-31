#ifndef SFXITEM_H
#define SFXITEM_H
#include "types.h"
#include "SFXPoint3.h"

class EXAudio_Voice;
struct SFXParameters;

// One playing (or pending) sound effect of the EuroSound core (EXAudioBase.obj), sizeof 0x39c.
// Lives in EXAudioBase::m_SFX and is linked into the active or free list. TODO names.
class SFXItem {
public:
    struct VoiceSlot {
        EXAudio_Voice *m_pVoice;  // 0x0
        int m_Volume;             // 0x4
        int m_Freq;               // 0x8
        int m_Pan;                // 0xc
    };
    ~SFXItem();
    void ClearVariables();
    int GetDist();
    int CountAllVoicesActive();
    int SFXGetImportance();
    void GetMyPos(int);
    SFXItem *GetRealParent();
    void Delete();
    int CombineVolumes(int);
    void CalculateDopplerShift();
    SFXItem();
    int ProcessCulling();
    void KeyOnPreCachedSFX();
    void Pause();
    void UnPause();
    void Remove();
    int SFXSetup();
    void UpdateEnding(SFXItem **);

    SFXItem *m_pNext;          // 0x000
    SFXItem *m_pPrev;          // 0x004
    const void *m_pObj;        // 0x008 owner object passed to SFXStart3D
    int m_Unk00c;              // 0x00c
    int m_Unk010;              // 0x010
    int m_Unk014;              // 0x014
    SFXItem *m_pParent;        // 0x018 set on sub-SFX
    int m_Unk01c;              // 0x01c
    int m_Hash;                // 0x020
    int m_Unk024;              // 0x024
    int m_Unk028;              // 0x028
    VoiceSlot m_Voices[20];    // 0x02c
    int m_Unk16c;              // 0x16c
    int m_Unk170[50];          // 0x170
    int m_Unk238[50];          // 0x238
    SFXParameters *m_pParams;  // 0x300
    int m_Bank;                // 0x304 sound bank slot
    int m_Unk308;              // 0x308
    SFXPoint3 m_Pos;           // 0x30c
    SFXPoint3 m_Unk318;        // 0x318
    SFXPoint3 m_Unk324;        // 0x324
    SFXPoint3 m_Unk330;        // 0x330
    SFXPoint3 m_Velocity;      // 0x33c
    float m_Doppler;           // 0x348 pitch shift (4096 per octave-ish log units)
    int m_VolumeOverRide;      // 0x34c
    int m_PitchOverRide;       // 0x350
    int m_Unk354;              // 0x354
    int m_Unk358;              // 0x358
    int m_Paused;              // 0x35c
    int m_Unk360;              // 0x360
    int m_Unk364;              // 0x364
    int m_Unk368;              // 0x368
    int m_Unk36c;              // 0x36c
    SFXPoint3 m_Unk370;        // 0x370
    int m_Unk37c;              // 0x37c
    int m_Unk380;              // 0x380
    int m_Unk384;              // 0x384
    int m_Unk388;              // 0x388
    int m_Unk38c;              // 0x38c
    int m_Pausable;            // 0x390 paused by a fade down
    int m_Remove;              // 0x394 remove on the next update
    int m_Unk398;              // 0x398
};
#endif
