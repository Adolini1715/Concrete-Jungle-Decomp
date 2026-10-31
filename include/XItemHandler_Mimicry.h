#ifndef XITEMHANDLER_MIMICRY_H
#define XITEMHANDLER_MIMICRY_H
#include "EXItemHandler.h"
#include "EXSoundTagStandIn.h"

class XItemList;

// Vocal mimicry pickup (FX_VocalMimicry.obj). sizeof 0x20.
class XItemHandler_Mimicry : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_Mimicry)
public:
    virtual ~XItemHandler_Mimicry();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    XItemHandler_Mimicry() {}
    static XItemHandler_Mimicry *CreateObject();
    unsigned int Play();
    u32 m_Unk08;            // 0x08 TODO
    int m_ShowFlagChannel;  // 0x0c becomes visible when set
    int m_HideFlagChannel;  // 0x10 deleted when set
    int m_PlayFlagChannel;  // 0x14 set by Play()
    unsigned int m_Sound;   // 0x18 returned by Play()
    int m_bActive;          // 0x1c
};

// Voice-over trigger (FX_VocalMimicry.obj). sizeof 0x1c.
class XItemHandler_VoiceOver : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_VoiceOver)
public:
    virtual ~XItemHandler_VoiceOver();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    XItemHandler_VoiceOver() {}
    static XItemHandler_VoiceOver *CreateObject();
    void Stop();
    int m_StartFlagChannel; // 0x08 starts the sound when set
    int m_StopFlagChannel;  // 0x0c deletes the item when set
    unsigned int m_Sound;   // 0x10
    EXSoundTag m_SoundTag;  // 0x14
};

extern XItemList gMimicryList;
extern XItemList gVoiceOverList;
void StopAllVoiceOvers();
#endif
