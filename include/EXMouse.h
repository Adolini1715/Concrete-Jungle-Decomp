#ifndef EXMOUSE_H
#define EXMOUSE_H
#include "types.h"
#include "EXRect.h"

// Mouse state (sizeof 8, allocated by EXBaseApp::InitMouse). No mouse on the retail Xbox
// build: the button queries return 0.
class EXMouse {
public:
    EXMouse() {}
    static EXMouse *GetMouse() { return m_pMouse; }
    static s8 Init(EXMouse *p) { m_pMouse = p; return 1; }
    static void Shutdown() {}
    s8 ButtonDown(unsigned int b) { return 0; }
    s8 ButtonPressed(unsigned int b) { return 0; }
    const EXPoint &Pos() const { return m_Pos; }
    EXPoint m_Pos;  // display coordinates
protected:
    static EXMouse *m_pMouse;
};
inline EXMouse *XGetMouse() { return EXMouse::GetMouse(); }
#endif
