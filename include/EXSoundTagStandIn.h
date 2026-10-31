#ifndef EXSOUNDTAGSTANDIN_H
#define EXSOUNDTAGSTANDIN_H
#include "EXVector.h"

class EXSoundHandle;

// TODO stand-in: EXSoundTag belongs to the audio area (EXSoundManager.obj); replace with the real
// header when it lands. Layout {head, tail} of EXSoundHandle links; inline ctor/dtor verified.
#ifndef EXSOUNDTAG_DEFINED
#define EXSOUNDTAG_DEFINED
class EXSoundTag {
public:
    EXSoundTag() { m_pHead = 0; m_pTail = 0; }
    ~EXSoundTag() { RemoveAll(0); }
    void RemoveAll(s8 b) const;
    EXSoundHandle *AddTrackingSound(u32 hash, const EXVector &pos, float vol, float range);
    EXSoundHandle *StartSound(u32 hash, const EXVector &pos, float vol, float range, int reverb, int flags);
    void UpdatePosition(const EXVector &pos) const;
    // Appended by the front-end agent (verified call sites in FEScript.obj).
    EXSoundHandle *StartSound(u32 hash, const EXVector &pos, int vol, int flags);
    void SetVolume(u32 vol) const;
    void KillSoundPlayingLooping() const;
    // Appended by the actor agent (verified call sites in Actor.obj).
    void KillSoundPlayingLooping(unsigned int hash) const;
    signed char IsPlaying() const;
    signed char IsPlaying(unsigned int hash) const;
    void Remove(unsigned int hash, s8 b) const;
    void Remove(EXSoundHandle *pHandle, s8 force) const;
    // Appended by the effects agent (verified call site in FX_VocalMimicry.obj).
    EXSoundHandle *StartSound(unsigned int hash);
    // Appended by the audio agent (EXSoundManager.obj).
    void SetPitch(int pitch) const;
    void SetVolume(unsigned int hash, unsigned int vol) const;
    void SetPitch(unsigned int hash, int pitch) const;
    EXSoundHandle *m_pHead;
    EXSoundHandle *m_pTail;
};
#endif
#endif
