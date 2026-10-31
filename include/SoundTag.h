#ifndef SOUNDTAG_H
#define SOUNDTAG_H
#include "EXSoundTagStandIn.h"

// Game-side sound tag (header inlines emitted in ActorCarry.obj). The out-of-line ~SoundTag is the
// inlined EXSoundTag destructor (RemoveAll(0)).
class SoundTag : public EXSoundTag {
public:
    SoundTag() {}
    ~SoundTag() {}
    signed char IsPlayingLooping(unsigned int hash);
};
#endif
