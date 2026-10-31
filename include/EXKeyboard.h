#ifndef EXKEYBOARD_H
#define EXKEYBOARD_H
#include "types.h"

// Keyboard state (sizeof 0x100, allocated by EXBaseApp::InitKeyboard). No keyboard on the
// retail Xbox build: Init/Shutdown only keep the pointer. TODO layout
class EXKeyboard {
public:
    EXKeyboard() {}
    static EXKeyboard *GetKeyboard() { return m_pKeyboard; }
    static s8 Init(EXKeyboard *p) { m_pKeyboard = p; return 1; }
    static void Shutdown() {}
    s8 ButtonDown(u8 key);
    s8 ButtonPressed(u8 key);
    u8 m_Data[0x100];  // TODO layout
protected:
    static EXKeyboard *m_pKeyboard;
};
inline EXKeyboard *XGetKeyboard() { return EXKeyboard::GetKeyboard(); }
#endif
