#ifndef EXGAMEPAD_H
#define EXGAMEPAD_H
#include "EXBaseGamePad.h"

// Xbox kernel input types, declared here (no XDK headers). The engine wraps them in namespace xb.
namespace xb {
struct _XINPUT_GAMEPAD {
    u16 wButtons;
    u8 bAnalogButtons[8];
    s16 sThumbLX;
    s16 sThumbLY;
    s16 sThumbRX;
    s16 sThumbRY;
};
struct _XINPUT_STATE {
    u32 dwPacketNumber;
    _XINPUT_GAMEPAD Gamepad;
};
#pragma pack(push, 1)
struct _XINPUT_FEEDBACK {
    u32 dwStatus;
    void *hEvent;
    u8 Reserved[58];
    u16 wLeftMotorSpeed;
    u16 wRightMotorSpeed;
};
#pragma pack(pop)
struct _XPP_DEVICE_TYPE;
}

// Xbox pads: 4 ports (sizeof 0xcc4).
class EXGamePad : public EXBaseGamePad {
public:
    virtual ~EXGamePad();
    s8 HasAnalogSticks(u8) { return 1; }
    s8 PadAttached(u8 pad) { return m_Attached[pad]; }
    s8 OpenPads();
    void SetInserted(int pad) { m_Attached.m_Data[pad] = 1; }
    void SetRemoved(int pad) { m_Attached.m_Data[pad] = 0; }
    EXGamePad();
    void SetPadRumble(u8 pad);
    void Update();
protected:
    void StorePadState(u8 pad, xb::_XINPUT_GAMEPAD *pState);

    const char *m_PortName[4];            // 0xb7c
    void *m_hPad[4];                      // 0xb8c
    u32 m_PacketNumber[4];                // 0xb9c
    xb::_XINPUT_FEEDBACK m_Feedback[4];   // 0xbac (0x46 bytes each)
};
#endif
