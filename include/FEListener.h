#ifndef FELISTENER_H
#define FELISTENER_H
#include "types.h"
#include "EXList.h"
#include "FEFlags.h"
#include "FEMathmos.h"

class FEObject;

// ID of the standard navigation listener ("nav_____").
#define FE_NAV_LISTENER 0x5f5f5f5f5f76616eui64

typedef signed char (__cdecl *FEEventCallback)(FEObject *, FEObjEvent::Flags);

// Key repeat timing (sizeof 0x20; FENavigator.obj).
class FEKeyRepeater {
public:
    FEKeyRepeater(unsigned int delay = FE_DEFAULT, unsigned int rate = FE_DEFAULT, unsigned int minRate = FE_DEFAULT,
                  unsigned int accel = FE_DEFAULT);
    signed char QueryButton(unsigned int button, FEPad::Flags pad);
    float m_PressTime;        // 0x00 game-state time of the first press
    unsigned int m_Delay;     // 0x04 frames before repeating
    unsigned int m_Rate;      // 0x08 initial repeat interval
    unsigned int m_CurRate;   // 0x0c
    unsigned int m_LastRepeat; // 0x10 game-state time of the last repeat
    unsigned int m_MinRate;   // 0x14
    unsigned int m_Accel;     // 0x18 repeats per speed-up step
    unsigned int m_Count;     // 0x1c
};

// Button remap entry (FENavigator.obj); list item at 0, input/output at 8/0xc.
class FEListenerRemap : public EXDListItem {
public:
    FEListenerRemap(unsigned int in, unsigned int out) { m_Input = in; m_Output = out; }
    unsigned int Input() { return m_Input; }
    unsigned int Output() { return m_Output; }
    unsigned int m_Input;   // 0x08
    unsigned int m_Output;  // 0x0c
};

// Input focus handler attached to an FEObject (FENavigator.obj). The EXDListItem base links it into
// the owner's listener list; it sits at +8 (vptr plus an 8-byte member). sizeof 0x68.
class FEListener : public EXDListItem {
public:
    virtual ~FEListener();
    FEListener(unsigned __int64 id, unsigned int flags, FEEventCallback pCallback, FEPad::Flags pad);
    FEListener(FEListener *p);

    static unsigned char LastPad() { return m_LastPad; }
    static void SetPadOverride(FEPad::Flags pad) { m_PadOverride = pad; }
    static FEListener *DialogListener() { return m_pDialogListener; }
    static void SetDialogListener(FEListener *p) { m_pDialogListener = p; }
    FEObject *Focus() { return m_pFocus; }
    void AddListenFlags(unsigned int f) { m_ListenFlags |= f; }
    void SubListenFlags(unsigned int f) { m_ListenFlags &= ~f; }
    void SetCallback(FEEventCallback p) { m_pCallback = p; }
    unsigned int UpdateTimeStamp() const { return m_UpdateTimeStamp; }
    FEObject *Owner() const { return m_pOwner; }
    void Enable() { m_Disabled = 0; }
    void Disable() { m_Disabled = 1; }
    static float LastInput() { return m_LastInput; }
    static void EnableInput() { ms_bInputDisabled = 0; }
    static signed char InputDisabled() { return ms_bInputDisabled; }
    signed char Disabled() { return m_Disabled; }
    unsigned __int64 ID() { return m_Id; }
    void SetOwner(FEObject *p) { m_pOwner = p; }
    void RestoreDefaultFocus() { SetFocus(m_pDefaultFocus, 0); }
    void SetHelpObject(FEObject *p) { m_pHelpObject = p; }

    void SetFocus(FEObject *p, signed char b);
    void SetDefaultFocus(FEObject *p);
    void SetCursor(FEObject *p);
    static void RemapAdd(unsigned int in, unsigned int out);
    static void RemapClear();
    static void RemapModeSet(FERemap::Flags mode);
    static void LastInputReset();
    static signed char QueryButtons(FEPad::Flags pad);
    static signed char IsOrthogonal(FEObjEvent::Flags ev, FEObject *a, FEObject *b);
    static void Initialise();
    static void LastInputUpdate();
    static signed char QueryButton(unsigned int b, FEPad::Flags pad, signed char r);
    static signed char QueryButton(unsigned int b, int pad, signed char r);
    static FEDirection::Flags QueryDirection(FEPad::Flags pad);
    static unsigned int GetChildren(FEObjEvent::Flags ev, FEObject *a, FEObject *b, FEObject **pList,
                                    unsigned int &n, unsigned int max, signed char r);
    signed char Update();
    static FEObject *FindObjectByData(FEObject *pRoot, unsigned __int64 data);
    static signed char IsParentOf(FEObject *a, FEObject *b);
    static signed char IsChildOf(FEObject *a, FEObject *b);
    signed char SetFocusByData(unsigned __int64 data, signed char b);
    signed char SetDefaultFocusByData(unsigned __int64 data);

    unsigned __int64 m_Id;          // 0x10
    FEObject *m_pFocus;             // 0x18
    FEObject *m_pOwner;             // 0x1c
    FEObject *m_pCursor;            // 0x20
    FEObject *m_pHelpObject;        // 0x24
    FEObject *m_pDefaultFocus;      // 0x28
    unsigned int m_ListenFlags;     // 0x2c
    FEPad::Flags m_Pad;             // 0x30
    FEMathmos m_CursorMathmos;      // 0x34
    unsigned int m_UpdateTimeStamp; // 0x5c
    signed char m_Disabled;         // 0x60
    FEEventCallback m_pCallback;    // 0x64

private:
    static FEPad::Flags m_PadOverride;
    static unsigned char m_LastPad;
    static float m_LastInput;
    static unsigned int m_ButtonMask[4];
    static unsigned int m_PrevButtonMask[4];
    static signed char ms_bInputDisabled;
    static FEListener *m_pDialogListener;
    static FEKeyRepeater m_KeyRepeater;
    static EXDList m_Remap;
};
#endif
