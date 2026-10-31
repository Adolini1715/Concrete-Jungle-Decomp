#ifndef XBMIXER_H
#define XBMIXER_H
#include "AudioMixer.h"

// Xbox mixer front ends (XBMixer.obj). TODO layout beyond AudioMixer.
class XBMixer : public AudioMixer {
public:
    XBMixer() {}
    ~XBMixer() {}
    unsigned int GetPlayPos();
};

class XBMixer_Five_One : public AudioMixer {
public:
    virtual int Init();
    virtual void Shutdown();
    XBMixer_Five_One() {}
protected:
    virtual void Mix_();
    void CreateSpeakerChannel(int, bool, int);
};

extern XBMixer *gpMixer;
#endif
