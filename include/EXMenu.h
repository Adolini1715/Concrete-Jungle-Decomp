#ifndef EXMENU_H
#define EXMENU_H
#include "types.h"
#include "EXList.h"
#include "EXString.h"
#include "EXWnd.h"
#include "EXAllocator.h"
#include "EXNew.h"

class EXMenu;
class EXMenuWnd;

// Menu entry (sizeof 0x18), pooled in EXMenu::m_MenuItemAllocator.
class EXMenuItem {
public:
    EXMenuItem() {}
    ~EXMenuItem();
    EXODListItem m_Link;     // 0x00 in EXMenu::m_Items (object = this)
    EXString m_Text;         // 0x0c
    unsigned int m_ID;       // 0x10 passed to MenuSelect
    EXMenu *m_pSubMenu;      // 0x14 owned
};

// Debug pull-down / pop-up menu (sizeof 0x1c).
class EXMenu {
    friend class EXMenuWnd;
public:
    virtual ~EXMenu();
    EXMenu();
    void SetTopLevel(s8 b) { m_TopLevel = b; }
    s8 IsOpen() const { return m_pMenuWnd != 0; }
    s8 TopLevel() const { return m_TopLevel; }
    EXMenuWnd *MenuWnd() { return m_pMenuWnd; }
    s8 CloseButtonVisible() const { return m_CloseButtonVisible; }
    s8 PushPinVisible() const { return m_PushPinVisible; }
    void SetPushPinIn(s8 b) { m_PushPinIn = b; }
    s8 PushPinIn() const { return m_PushPinIn; }
    EXWnd *Wnd() { return m_pWnd; }
    void ShowCloseButton(s8 b) { m_CloseButtonVisible = b; }
    void ShowPushPin(s8 b) { m_PushPinVisible = b; }
    EXMenuItem *ItemByIndex(unsigned int index) const;
    EXMenuItem *ItemByID(unsigned int id, s8 recurse) const;
    s8 DelItemByIndex(unsigned int index);
    void Close();
    s8 DelItemByID(unsigned int id, s8 recurse);
    s8 DelAllItems();
    EXMenu *AddSubMenu(const EXString &text, unsigned int id);
    void AddItem(const EXString &text, unsigned int id);
    void Open(EXWnd *pWnd, int x, int y);
protected:
    EXODListItem *FirstListItem() const { return m_Items.Head(); }
    static EXClassAllocator<EXMenuItem> m_MenuItemAllocator;
public:
    s8 m_TopLevel;             // 0x04 menu bar (child of m_pWnd) rather than pop-up
    EXODList m_Items;          // 0x08
    EXWnd *m_pWnd;             // 0x10 owner window
    EXMenuWnd *m_pMenuWnd;     // 0x14 open window
    s8 m_CloseButtonVisible;   // 0x18
    s8 m_PushPinVisible;       // 0x19
    s8 m_PushPinIn;            // 0x1a
};

// Window showing an open menu (sizeof 0x2dc).
class EXMenuWnd : public EXWnd {
public:
    virtual ~EXMenuWnd();
    virtual s8 Update();
    virtual s8 Draw();
    virtual void ButtonDown(unsigned int b);
    virtual void ButtonUp(unsigned int b);
    virtual void MouseMove(int x, int y);
    EXMenuWnd(EXMenu *pMenu);
    void SetScrollMenu(s8) { m_ScrollMenu = 1; }
    void CloseSubMenu();
protected:
    void ProcessItem(const EXMenuItem *pItem, const EXRect &r);
public:
    EXWnd *m_pPrevFocusWnd;    // 0x29c
    EXWnd *m_pPrevMouseWnd;    // 0x2a0
    EXMenu *m_pMenu;           // 0x2a4
    EXDataArray<EXRect, EXRect &> m_ItemRects;  // 0x2a8 prim rects of the items
    EXMenu *m_pSubMenu;        // 0x2b8 open sub menu
    EXRect m_HiRect;           // 0x2bc highlighted item, window coordinates
    s8 m_ScrollMenu;           // 0x2cc
    int m_Unk2d0;              // 0x2d0
    unsigned int m_NumItems;   // 0x2d4 items plus pin/close buttons
    u16 m_Unk2d8;              // 0x2d8
};
#endif
