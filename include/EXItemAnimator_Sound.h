#ifndef EXITEMANIMATOR_SOUND_H
#define EXITEMANIMATOR_SOUND_H
#include "EXItemAnimator.h"

#include "EXSoundTagStandIn.h"

// Positional sound / reverb-zone animator (sizeof 0x120).
class EXItemAnimator_Sound : public EXItemAnimator {
    EX_DECLARE_DYNAMIC(EXItemAnimator_Sound)
public:
    EX_DECLARE_ITEMTYPE(EXItemAnimator_Sound)
    virtual void DoUpdate();
    virtual u32 DoRender(const EXMatrix &mtx, const EXRenderInfo &info, u32 flags);
    virtual s8 Initialise(EXGeoFile *pFile, u32 hash);
    virtual void SetObjectTime(float t);
    EXItemAnimator_Sound();
    static EXItemAnimator_Sound *CreateObject();
    static EXItemAnimator_Sound *NewItem() { return new EXItemAnimator_Sound; }
    void SetSoundDetails(float vol, float range, int reverb, u32 flags, u32 user);
    void SetSoundTimeout(u16 t) const;
private:
    void UpdateSound(s8 bRender);
public:
    EXSoundTag m_tag;       // +0x104
    float m_volume;         // +0x10c
    float m_range;          // +0x110
    u32 m_soundFlags;       // +0x114 (0x20 tracking)
    s8 m_reverb;            // +0x118
    s8 m_bStarted;          // +0x119
    u32 m_user;             // +0x11c copied to the sound handle (+0x18)
};
#endif
