#ifndef MEMCARDSTANDIN_H
#define MEMCARDSTANDIN_H
#include "types.h"
#include "EXRuntimeClass.h"
#include "FEWnd.h"
#include "EXMemCard.h"

// TODO stand-in: memory-card dialog (EXMemCard.obj, engine) and the game's version (GameData.obj).
// Only what FELang.obj's memory-card flow touches.
// Dialog window (GameData.obj). TODO layout past 0x408; the vtable (77 slots) is in GameData.obj.
class GameDialog;
class GameDialogWnd : public FEWnd {
public:
    GameDialogWnd()
    {
        m_NumInstances++;
        BringToTop();
    }
    virtual ~GameDialogWnd() { m_NumInstances--; }
    virtual signed char Update();
    virtual signed char Draw();
    void SetOwnerDlg(GameDialog *p) { m_pOwner = p; }
    GameDialog *OwnerDlg() { return m_pOwner; }
    void *m_pButtons;      // 0x3fc
    u32 m_Unk400;
    u32 m_Unk404;
    unsigned int m_Result; // 0x408 1-based button chosen, 0 while open
    GameDialog *m_pOwner;  // 0x40c
    static unsigned int NumInstances() { return m_NumInstances; }
    static unsigned char LastPad() { return ms_LastPad; }
    static void SetLastPad(unsigned char p) { ms_LastPad = p; }
    static unsigned int m_NumInstances;
    static unsigned char ms_LastPad;
};
class GameDialog : public EXMemCardDlg {
    EX_DECLARE_DYNAMIC(GameDialog)
public:
    static GameDialog *CreateObject();
    virtual ~GameDialog() {}
    virtual void ShowDlg(const EXMemCardDlgData &d);
    virtual void HideDlg();
    virtual unsigned int DlgResult();
    virtual void ShowDlg(const EXMemCardDlgData &d, unsigned int n, unsigned int *p);
    GameDialog() { m_pWnd = 0; }
    GameDialogWnd *Wnd() const { return m_pWnd; }
    EXMemCardDlgData &Data() { return m_Data; }
    void HideDlg_NoDestroy();
    void ShowDlg_NoCreate();
    unsigned int ConvertMessageHashCode(unsigned int hash);
    u32 m_Unk8;                 // TODO layout
    GameDialogWnd *m_pWnd;      // 0x00c
    u8 m_Unk10[0x2a0 - 0x10];   // TODO layout
    class FEListener *m_pListener;  // 0x2a0 installed as the dialog listener while shown
    EXMemCardDlgData m_Data;    // 0x2a4
    unsigned int m_SndMask;     // 0x2dc results that play no sound (bit per result)
    unsigned int *m_pButtonArray;  // 0x2e0
    static unsigned int CalcSndMask_Result(unsigned int r) { return 1 << r; }
    signed char IsSndMasked_Result(unsigned int r) { return (m_SndMask & (1 << r)) == 0; }
    unsigned int *ButtonArrayPtr() { return m_pButtonArray; }
};
#endif
