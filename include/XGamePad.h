#ifndef XGAMEPAD_H
#define XGAMEPAD_H
#include "types.h"
#include "EXGamePad.h"
#include "EXString.h"
#include "EXVector.h"


// On-screen button glyph (sizeof 0x10).
struct XButtonGraphic {
    u32 m_Texture;      // 0x00 texture hash
    u32 m_TextHash;     // 0x04 text hash for the glyph
    float m_Scale;      // 0x08
    s8 m_bAnalogue;     // 0x0c TODO name
    u8 m_Unkd;          // 0x0d
    u16 m_Unke;
};
// Active rumble effect (sizeof 0x38).
struct _ACTIVE_RUMBLES {
    _ACTIVE_RUMBLES() {}
    s8 m_bActive;           // 0x00
    s8 m_b3D;               // 0x01
    int m_Pad;              // 0x04
    unsigned int m_Id;      // 0x08 rumble hash (type 0x0c)
    u8 *m_pData;            // 0x0c (lo, hi) motor pairs per frame
    float m_Time;           // 0x10 frames played
    int m_Length;           // 0x14 frames
    float m_Mult;           // 0x18
    float m_Unk1c;          // 0x1c
    float m_MinDist;        // 0x20 full strength inside this
    float m_Range;          // 0x24 fade distance
    EXVector m_Pos;         // 0x28
};
struct _RUMBLEDATA {
    u8 *m_pData;
    int m_Length;           // frames (bytes / 2)
};

// Escape-code button (sizeof 0xc): "~<Trigger>" in text maps to a glyph and an action.
struct XButton {
    char Trigger;               // 0x00
    XButtonGraphic *Graphic;    // 0x04
    u32 Action;                 // 0x08
};

// Game pad with action configs, button glyphs and rumble (XGamePad.obj). sizeof 0xccc.
class XGamePad : public EXGamePad {
protected:
    virtual ~XGamePad();
public:
    s8 GetActionsChanged()
    {
        s8 b = m_bActionsChanged;
        m_bActionsChanged = 0;
        return b;
    }
    int GetActions() { return m_Config; }
    unsigned int NumConfigs() const { return 5; }
    void SetActions(int config);
    EXPad::AnalogChan GetAnalogueFromAction(unsigned int action);
    static s8 Init(XGamePad *pPad);
    EXString GetActionString(int action);
    XButton *LookupButton(char trigger);
    unsigned int SetMask(unsigned int mask);
    EXString GetControllerString(unsigned int action);
    unsigned int XActions(u8 pad);
    unsigned int XActionPressed(u8 pad);
    s8 XActionPressed(u8 pad, unsigned int action);
    unsigned int XActionReleased(u8 pad);
    s8 XActionReleased(u8 pad, unsigned int action);
    s8 XActionUp(u8 pad, unsigned int action);
    s8 XActionDown(u8 pad, unsigned int action);
    void InitRumbleManager();
    void StopAllRumble();
    int GetRumbleSlot();
    void StopRumbleID(unsigned int id);
    void UpdateRumble();
    s8 IsRumbleActive(unsigned int id, int pad);
    void AlterRumbleMult(int id, float mult);
    int PlayRumble(unsigned int id, float mult, u8 pad);
    void PlayRumble3D(unsigned int id, EXVector &pos, float a, float b, float c, u8 pad);
    XGamePad();
protected:
    int m_Config;           // 0xcc4 active action config (PadRemap index)
    s8 m_bActionsChanged;   // 0xcc8 read-and-clear by GetActionsChanged
};
inline EXGamePad *XGetGamePad() { return (EXGamePad *)EXBaseGamePad::GetGamePad(); }  // [fi] copy in Camera.obj
#endif
