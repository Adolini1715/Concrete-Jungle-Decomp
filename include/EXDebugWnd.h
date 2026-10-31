#ifndef EXDEBUGWND_H
#define EXDEBUGWND_H
#include "types.h"
#include "EXWnd.h"
#include "EXString.h"
#include "EXArray.h"

class EXMenu;

// Toggleable debug text window registered with EXDebugWndMgr.
class EXDebugWnd : public EXWnd {
public:
    virtual ~EXDebugWnd();
    virtual s8 Update();
    virtual s8 Draw();
    virtual void Close();
    virtual void ButtonDown(unsigned int b);
    virtual void ButtonUp(unsigned int b);
    EXODListItem *DebugWndListItem() { return &m_ListItem; }
    const EXString &Title() const { return m_Title; }
    void SetTitle(const EXString &s) { m_Title = s; }
    unsigned char ShowKey() const { return m_ShowKey; }
    void SetShowKey(unsigned char k) { m_ShowKey = k; }
    s8 PrintWhenHidden() const { return m_PrintWhenHidden; }
    void SaveState();
protected:
    friend class EXDebugWndMgr;
    EXDebugWnd();
public:
    EXODListItem m_ListItem;   // 0x29c in EXDebugWndMgr::m_Wnds
    EXString m_Title;          // 0x2a8
    unsigned char m_ShowKey;   // 0x2ac
    s8 m_PrintWhenHidden;      // 0x2ad
};

class EXDebugWndMgr {
public:
    static EXDebugWndMgr *GetDebugWndMgr() { return m_pDebugWndMgr; }
    EXDebugWndMgr();
    ~EXDebugWndMgr();
    void RegisterMenu(EXMenu *pMenu, unsigned int id);
    s8 MenuSelect(unsigned int id) const;
    static s8 Init(EXDebugWndMgr *pMgr);
    static void Shutdown();
    EXDebugWnd *NewDebugWnd(const EXString &title, unsigned char key, const EXRect &r, ex::RGBA col, EXDebugWnd *pWnd);
    void DelDebugWnd(EXDebugWnd *pWnd);
    void Update() const;
protected:
    void UpdateMenu();
    static EXDebugWndMgr *m_pDebugWndMgr;
public:
    EXODList m_Wnds;           // 0x00
    EXMenu *m_pMenu;           // 0x08
    unsigned int m_MenuID;     // 0x0c first menu id
};

// Dialog button (sizeof 0x3c).
struct EXDialogButton {
    EXFixedArray<char, 30> m_Text;  // 0x00
    u16 m_Pad1e;
    int m_Value;               // 0x20 returned by GetReturn
    EXRect m_Rect;             // 0x24
    u32 m_Unk34;
    u32 m_Unk38;
};

class EXDebugDialogWnd : public EXWnd {
public:
    virtual ~EXDebugDialogWnd();
    virtual s8 Update();
    virtual s8 Draw();
    virtual void ButtonDown(unsigned int b);
    virtual void MouseMove(int x, int y);
    void SetText(const char *pText);
    s8 PointInButton(const EXDialogButton *pButton, const EXPoint &p) const;
    void Destroy();
    void Show();
    void Hide();
    s8 GetReturn(int *pRet);
    void AddButton(EXDialogButton &b);
    EXDebugDialogWnd();

    EXDataArray<EXDialogButton, EXDialogButton &> m_Buttons;  // 0x29c
    int m_Return;              // 0x2ac
    s8 m_HasReturn;            // 0x2b0
    int m_TextX;               // 0x2b4
    int m_TextY;               // 0x2b8
    s8 m_Destroy;              // 0x2bc
};
#endif
