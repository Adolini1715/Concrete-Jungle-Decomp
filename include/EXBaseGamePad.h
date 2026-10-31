#ifndef EXBASEGAMEPAD_H
#define EXBASEGAMEPAD_H
#include "types.h"
#include "EXArray.h"

namespace EXPad {
enum AnalogChan { ANALOG_0 };      // TODO values
enum DeadZoneMode { DEADZONE_0 };  // TODO values
}

// Button mask -> action bits.
struct EXPadActionMap {
    u32 m_Buttons;
    u32 m_Action;
};

// Platform-independent pad state for 8 pads (sizeof 0xb7c).
class EXBaseGamePad {
public:
    virtual ~EXBaseGamePad();
    static EXBaseGamePad *GetGamePad() { return m_pGamePad; }
    s8 ButtonDown(u8 pad, u32 mask) { return (m_Buttons[pad] & mask) ? 1 : 0; }
    float Analog(u8 pad, EXPad::AnalogChan chan) { return m_Analog[pad][chan]; }
    s8 ButtonReleased(u8 pad, u32 mask) { return (m_ButtonsReleased[pad] & mask) ? 1 : 0; }
    s8 ButtonPressed(u8 pad, u32 mask) { return (m_ButtonsPressed[pad] & mask) ? 1 : 0; }
    u32 Buttons(u8 pad) { return m_Buttons[pad]; }
    s8 RumbleEnabled(int pad) const { return m_RumbleEnabled.m_Data[pad]; }
    u32 Actions(u8 pad) { return m_Actions[pad]; }
    u32 ActionPressed(u8 pad) { return m_ActionsPressed[pad]; }
    u32 ActionReleased(u8 pad) { return m_ActionsReleased[pad]; }
    void SetDeadZoneMode(u8 pad, EXPad::DeadZoneMode mode) { m_DeadZoneMode[pad][1] = (u8)mode; m_DeadZoneMode[pad][0] = (u8)mode; }
    s8 ActionDown(u8 pad, u32 mask) { return (m_Actions[pad] & mask) ? 1 : 0; }
    s8 ActionUp(u8 pad, u32 mask);
    s8 ActionPressed(u8 pad, u32 mask) { return (m_ActionsPressed[pad] & mask) ? 1 : 0; }
    s8 ActionReleased(u8 pad, u32 mask) { return (m_ActionsReleased[pad] & mask) ? 1 : 0; }
    static s8 Init(EXBaseGamePad *pPad);
    static void Shutdown();
    EXBaseGamePad();
    void UpdateActions();
    void SetActionMap(u8 pad, u8 count, const EXPadActionMap *pMap);
    void StartRumbleAll(u8 pad, u8 strength, u32 time);
    void StartRumbleHi(u8 pad, u8 strength, u32 time);
    void StartRumbleLo(u8 pad, u8 strength, u32 time);
    void StopRumbleAll(u8 pad);
    void StopRumbleHi(u8 pad);
    void StopRumbleLo(u8 pad);
    void EnableAllRumble(s8 enable);
    void KillAllRumble();
protected:
    void RumbleUpdate(u8 pad);
    static EXBaseGamePad *m_pGamePad;

    u32 m_Buttons[8];                                   // 0x004
    u32 m_PrevButtons[8];                               // 0x024
    u32 m_ButtonsPressed[8];                            // 0x044
    u32 m_ButtonsReleased[8];                           // 0x064
    EXFixedArray<EXFixedArray<float, 16>, 8> m_Analog;  // 0x084
    EXFixedArray<EXFixedArray<u8, 2>, 8> m_DeadZoneMode;  // 0x284
    u32 m_Actions[8];                                   // 0x294
    u32 m_PrevActions[8];                               // 0x2b4
    u32 m_ActionsPressed[8];                            // 0x2d4
    u32 m_ActionsReleased[8];                           // 0x2f4
    EXFixedArray<EXFixedArray<EXPadActionMap, 32>, 8> m_ActionMap;  // 0x314
    EXFixedArray<u8, 8> m_NumActionMaps;                // 0xb14
    u32 m_RumbleHiTime[8];                              // 0xb1c frames left
    u32 m_RumbleLoTime[8];                              // 0xb3c
    EXFixedArray<u8, 8> m_RumbleHi;                     // 0xb5c
    EXFixedArray<u8, 8> m_RumbleLo;                     // 0xb64
    EXFixedArray<s8, 8> m_RumbleEnabled;                // 0xb6c
    EXFixedArray<u8, 8> m_Attached;                     // 0xb74
};
// Appended by the game-application agent (copy emitted in XGamePad.obj).
inline s8 EXBaseGamePad::ActionUp(u8 pad, u32 mask) { return (~m_Actions[pad] & mask) ? 1 : 0; }
#endif
