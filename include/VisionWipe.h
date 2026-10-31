#ifndef VISIONWIPE_H
#define VISIONWIPE_H
#include "FEWnd.h"
#include "EXSoundTagStandIn.h"

// Screen wipe played when the vision mode (or camera view) changes (VisionWipe.obj). sizeof 0x408.
// Modes below 4 are vision modes, 100 first person, 101 third person.
class VisionWipe : public FEWnd {
public:
    virtual ~VisionWipe() { m_pInstance = 0; }
    virtual signed char Update();
    signed char IsWiping() { return m_bWiping; }
    static VisionWipe *Instance(signed char create);
    void Start(unsigned char mode);
    void KillWipe();

    s8 m_bWiping;       // 0x3fc
    s8 m_bCommitted;    // 0x3fd mode applied (half way)
    s8 m_bFlip;         // 0x3fe random direction
    u8 m_Unk3ff;
    float m_Timer;      // 0x400
    u8 m_Mode;          // 0x404

private:
    VisionWipe();
    static VisionWipe *m_pInstance;
    static EXSoundTag m_SoundTag;
};
inline VisionWipe *XGetVisionWipe(signed char create) { return VisionWipe::Instance(create); }  // [fi] App.obj
#endif
