#include "EXMenu.h"
#include "EXBaseDisplay.h"
#include "EXMemory.h"

// TODO stand-in: mouse (EXMouse.obj); the cursor position comes first.
class EXMouse {
public:
    EXPoint m_Pos;
    static EXMouse *m_pMouse;
};

EXClassAllocator<EXMenuItem> EXMenu::m_MenuItemAllocator(16);

EXMenuItem::~EXMenuItem()
{
    if (m_pSubMenu) delete m_pSubMenu;
}

EXMenu::EXMenu()
{
    m_TopLevel = 1;
    m_pWnd = 0;
    m_pMenuWnd = 0;
    m_CloseButtonVisible = 0;
    m_PushPinVisible = 0;
    m_PushPinIn = 0;
}

EXMenuItem *EXMenu::ItemByIndex(unsigned int index) const
{
    unsigned int i = 0;
    for (EXODListItem *p = FirstListItem(); p; p = p->Next()) {
        EXMenuItem *pItem = (EXMenuItem *)p->Object();
        if (pItem) {
            if (i == index) return pItem;
            i++;
        }
    }
    return 0;
}

EXMenuItem *EXMenu::ItemByID(unsigned int id, s8 recurse) const
{
    for (EXODListItem *p = FirstListItem(); p; p = p->Next()) {
        EXMenuItem *pItem = (EXMenuItem *)p->Object();
        if (pItem) {
            if (pItem->m_ID == id) return pItem;
            if (recurse && pItem->m_pSubMenu) {
                EXMenuItem *pFound = pItem->m_pSubMenu->ItemByID(id, recurse);
                if (pFound) return pFound;
            }
        }
    }
    return 0;
}

s8 EXMenu::DelItemByIndex(unsigned int index)
{
    unsigned int i = 0;
    for (EXODListItem *p = m_Items.Head(); p; p = p->Next()) {
        if (p->Object()) {
            if (i == index) {
                m_Items.Remove(p);
                return 1;
            }
            i++;
        }
    }
    return 0;
}

void EXMenuWnd::MouseMove(int, int) {}

void EXMenuWnd::CloseSubMenu()
{
    if (m_pSubMenu) m_pSubMenu->Close();
    m_pSubMenu = 0;
}

// NEAR: register swap (retail keeps this in edi, the menu window in esi).
void EXMenu::Close()
{
    EXMenuWnd *pMenuWnd = m_pMenuWnd;
    if (pMenuWnd) {
        if (m_PushPinIn) {
            pMenuWnd->CloseSubMenu();
        } else if (m_TopLevel) {
            pMenuWnd->CloseSubMenu();
            m_pWnd->DelChild(m_pMenuWnd);
            m_pMenuWnd = 0;
        } else {
            m_pMenuWnd->CloseSubMenu();
            EXBaseDisplay::GetDisplay()->DelWnd(m_pMenuWnd);
            m_pMenuWnd = 0;
        }
    }
}

EXMenuWnd::~EXMenuWnd()
{
    if (m_pSubMenu) m_pSubMenu->Close();
    EXBaseDisplay::GetDisplay()->SetFocusWnd(0);
    EXBaseDisplay::GetDisplay()->SetMouseWnd(0);
}

// NEAR: inlined EXRect::ContainsPoint branch shape and register allocation.
s8 EXMenuWnd::Draw()
{
    if (EXBaseDisplay::GetDisplay()->WindowUnder(EXMouse::m_pMouse->m_Pos, 0) == this) {
        EXPoint p = EXBaseDisplay::DispToPrim(EXMouse::m_pMouse->m_Pos);
        m_HiRect.Set(0, 0, 0, 0);
        for (int i = 0; i < m_ItemRects.Size(); i++) {
            EXRect &r = m_ItemRects[i];
            if (r.ContainsPoint(p)) {
                m_HiRect = r;
                m_HiRect.Offset(-m_PrimRect.x, -m_PrimRect.y);
            }
        }
        return 1;
    }
    if (!m_pSubMenu) {
        m_HiRect.Set(0, 0, 0, 0);
        return 1;
    }
    int i = 0;
    for (EXODListItem *p = m_pMenu->FirstListItem(); p; p = p->Next(), i++) {
        EXMenuItem *pItem = (EXMenuItem *)p->Object();
        if (pItem && pItem->m_pSubMenu == m_pSubMenu) {
            m_HiRect = m_ItemRects[i];
            m_HiRect.Offset(-m_PrimRect.x, -m_PrimRect.y);
            return 1;
        }
    }
    return 1;
}

void EXMenuWnd::ButtonUp(unsigned int b)
{
    if (b == 2) m_pMenu->Wnd()->Menu()->Close();
}

EXMenu::~EXMenu()
{
    EXODListItem *p = m_Items.Head();
    while (p) {
        EXMenuItem *pItem = (EXMenuItem *)p->Object();
        EXODListItem *pNext = p->Next();
        m_MenuItemAllocator.Remove(pItem);
        p = pNext;
    }
    if (m_pMenuWnd) {
        delete m_pMenuWnd;
        m_pMenuWnd = 0;
    }
}

s8 EXMenu::DelItemByID(unsigned int id, s8 recurse)
{
    for (EXODListItem *p = m_Items.Head(); p; p = p->Next()) {
        EXMenuItem *pItem = (EXMenuItem *)p->Object();
        if (pItem) {
            if (pItem->m_ID == id) {
                m_Items.Remove(p);
                m_MenuItemAllocator.Remove(pItem);
                return 1;
            }
            if (recurse && pItem->m_pSubMenu && pItem->m_pSubMenu->DelItemByID(id, recurse)) return 1;
        }
    }
    return 0;
}

s8 EXMenu::DelAllItems()
{
    for (;;) {
        EXODListItem *p = m_Items.Head();
        if (!p) break;
        EXMenuItem *pItem = (EXMenuItem *)p->Object();
        m_Items.RemoveHead();
        if (pItem) m_MenuItemAllocator.Remove(pItem);
    }
    return 1;
}

EXMenu *EXMenu::AddSubMenu(const EXString &text, unsigned int id)
{
    EXMenuItem *pItem = m_MenuItemAllocator.Get();
    pItem->m_Text = text;
    pItem->m_ID = id;
    pItem->m_pSubMenu = new EXMenu;
    pItem->m_pSubMenu->m_TopLevel = 0;
    m_Items.AddTail(&pItem->m_Link, pItem);
    return pItem->m_pSubMenu;
}

void EXMenu::AddItem(const EXString &text, unsigned int id)
{
    EXMenuItem *pItem = m_MenuItemAllocator.Get();
    pItem->m_Text = text;
    pItem->m_ID = id;
    pItem->m_pSubMenu = 0;
    m_Items.AddTail(&pItem->m_Link, pItem);
}

// NEAR: register allocation (retail keeps &m_ItemRects in ebp for the inlined SetSize).
EXMenuWnd::EXMenuWnd(EXMenu *pMenu)
{
    m_pMenu = pMenu;
    m_pSubMenu = 0;
    m_HiRect.Set(0, 0, 0, 0);
    m_ScrollMenu = 0;
    m_Unk2d0 = 0;
    m_Unk2d8 = 0;
    m_pPrevFocusWnd = EXBaseDisplay::GetDisplay()->FocusWnd();
    EXBaseDisplay::GetDisplay()->SetFocusWnd(this);
    if (!m_pMenu->TopLevel()) {
        m_pPrevMouseWnd = EXBaseDisplay::GetDisplay()->MouseWnd();
        EXBaseDisplay::GetDisplay()->SetMouseWnd(this);
    } else {
        m_pPrevMouseWnd = 0;
    }
    m_NumItems = 0;
    unsigned int maxLen = 0;
    for (EXODListItem *p = m_pMenu->FirstListItem(); p; p = p->Next()) {
        EXMenuItem *pItem = (EXMenuItem *)p->Object();
        if (pItem) {
            m_NumItems++;
            if (pItem->m_pSubMenu) {
                if (pItem->m_Text.Length() + 1 > maxLen) maxLen = pItem->m_Text.Length() + 1;
            } else {
                if (pItem->m_Text.Length() > maxLen) maxLen = pItem->m_Text.Length();
            }
        }
    }
    if (!m_pMenu->TopLevel()) {
        m_Rect.w = (maxLen + 2) * 12;
        m_Rect.h = m_NumItems * 12;
    }
    if (m_pMenu->PushPinVisible()) m_NumItems++;
    if (m_pMenu->CloseButtonVisible()) m_NumItems++;
    m_ItemRects.SetSize(m_NumItems, -1);
}

// NEAR: register allocation in the pop-up placement.
void EXMenu::Open(EXWnd *pWnd, int x, int y)
{
    if (m_pMenuWnd) {
        Close();
        if (m_pMenuWnd) return;
    }
    m_pWnd = pWnd;
    m_pMenuWnd = new EXMenuWnd(this);
    m_pMenuWnd->m_Flags = 0x8000071e;
    EXRect r;
    if (m_TopLevel) {
        r.Set(0, 0, m_pWnd->m_Rect.w, 12);
        m_pMenuWnd->SetRect(r);
        m_pMenuWnd->SetBkColor(ex::RGBA(0x20, 0x20, 0x20, 0x7f));
        m_pWnd->AddChild(m_pMenuWnd);
        return;
    }
    r = m_pMenuWnd->m_Rect;
    EXBaseDisplay *pDisplay = EXBaseDisplay::GetDisplay();
    unsigned short dw = pDisplay->Width();
    unsigned short dh = pDisplay->Height();
    if (x + r.w > dw) r.x = dw - r.w;
    else r.x = x;
    if (r.h > dh - 0x60) {
        int top = 0x30;
        r.y = min(y, top);
        r.h = dh - 0x60;
        m_pMenuWnd->SetScrollMenu(1);
    } else if (y + r.h > dh) {
        r.y = dh - r.h;
    } else {
        r.y = y;
    }
    m_pMenuWnd->SetRect(r);
    m_pMenuWnd->SetBkColor(ex::RGBA(0x20, 0x20, 0x20, 0x7f));
    EXBaseDisplay::GetDisplay()->AddWnd(m_pMenuWnd);
}

// Draw the item captions, record each item's screen rect in m_ItemRects, and open / close the sub
// menu that the mouse is hovering over. Returns 0 when a hover changed the sub menu state.
s8 EXMenuWnd::Update()
{
    s8 ret = 1;
    if (m_Unk2d8 < 13) m_Unk2d8++;
    int mx = EXMouse::m_pMouse->m_Pos.x + EXBaseDisplay::Offset().x;
    int my = EXMouse::m_pMouse->m_Pos.y + EXBaseDisplay::Offset().y;
    unsigned int idx = 0;
    EXWnd *pUnder = EXBaseDisplay::GetDisplay()->WindowUnder(EXMouse::m_pMouse->m_Pos, 0);

    if (!m_pMenu->m_TopLevel) {
        EXRect r(m_PrimRect.x, m_PrimRect.y, m_PrimRect.w, 12);
        EXRect empty(0, 0, 0, 0);
        if (m_ScrollMenu) {
            SetDbgPos(1, 0);
            DbgPrint("%c", 0x87);
            SetDbgPos(m_DbgWidth - 2, 0);
            DbgPrint("%c", 0x87);
            r.y += 12;
            SetDbgPos(0, 1);
        }
        EXODListItem *pLink = m_pMenu->m_Items.Head();
        while (pLink) {
            EXMenuItem *pItem = (EXMenuItem *)pLink->Object();
            if (!pItem) {
                pLink = pLink->Next();
                continue;
            }
            if (!m_ScrollMenu || ((unsigned int)m_Unk2d0 <= idx && idx <= (unsigned int)(m_DbgHeight - 3) + m_Unk2d0)) {
                int row = (short)m_DbgY;
                DbgPrint("%s", (const char *)pItem->m_Text);
                if (pItem->m_pSubMenu) {
                    SetDbgPos(m_DbgWidth - 1, row);
                    DbgPrint("%c", 0x85);
                }
                SetDbgPos(0, row + 1);
                m_ItemRects.SetAtGrow(idx, r);
                idx++;
                if (pUnder == this && r.x <= mx && r.y <= my && mx <= r.w - 1 + r.x && my <= r.y + 11 && m_Unk2d8 > 12) {
                    EXMenu *pSub = m_pSubMenu;
                    if (!pItem->m_pSubMenu) {
                        if (pSub) pSub->Close();
                        ret = 0;
                    } else if (pItem->m_pSubMenu != pSub) {
                        if (pSub) pSub->Close();
                        ret = 0;
                        ProcessItem(pItem, r);
                    }
                }
                r.y += 12;
                pLink = pLink->Next();
            } else {
                pLink = pLink->Next();
                m_ItemRects.SetAtGrow(idx, empty);
                idx++;
            }
        }
        if (m_ScrollMenu) {
            SetDbgPos(1, m_DbgHeight - 1);
            DbgPrint("%c", 0x86);
            SetDbgPos(m_DbgWidth - 2, m_DbgHeight - 1);
            DbgPrint("%c", 0x86);
        }
    } else {
        EXODListItem *pLink = m_pMenu->m_Items.Head();
        int x = m_PrimRect.x + 6;
        EXRect r(x, m_PrimRect.y, 0, 13);
        for (; pLink; pLink = pLink->Next()) {
            EXMenuItem *pItem = (EXMenuItem *)pLink->Object();
            if (!pItem) continue;
            DbgPrint("%s", (const char *)pItem->m_Text);
            int w = (pItem->m_Text.Length() + 1) * 12;
            r.w = w;
            m_ItemRects.SetAtGrow(idx, r);
            idx++;
            if (pUnder == this && x <= mx && r.y <= my && mx <= w - 1 + x && my <= r.y + 12 && m_Unk2d8 > 12) {
                EXMenu *pSub = m_pSubMenu;
                if (!pItem->m_pSubMenu) {
                    if (pSub) pSub->Close();
                    ret = 0;
                } else if (pItem->m_pSubMenu != pSub) {
                    if (pSub) pSub->Close();
                    ret = 0;
                    ProcessItem(pItem, r);
                }
            }
            x += w;
            r.x = x;
        }
    }

    if (m_pMenu->m_TopLevel) {
        EXRect r(m_PrimRect.x, m_PrimRect.y, 0x18, 12);
        unsigned short col = m_DbgWidth - 1;
        if (m_pMenu->m_CloseButtonVisible) {
            SetDbgPos(col - 1, 0);
            DbgPrint("%cX", 0x7f);
            r.x = m_PrimRect.x - 6 + col * 12;
            m_ItemRects.SetAtGrow(idx, r);
            idx++;
        }
        if (m_pMenu->m_PushPinVisible) {
            SetDbgPos(col - 3, 0);
            DbgPrint("%c%c", 0x7f, m_pMenu->m_PushPinIn ? 0x81 : 0x80);
            r.x = m_PrimRect.x - 6 + (unsigned short)(col - 2) * 12;
            m_ItemRects.SetAtGrow(idx, r);
        }
    }
    return ret;
}

void EXMenuWnd::ButtonDown(unsigned int b)
{
    if (b == 1) {
        int mx = EXMouse::m_pMouse->m_Pos.x + EXBaseDisplay::Offset().x;
        int my = EXMouse::m_pMouse->m_Pos.y + EXBaseDisplay::Offset().y;
        if (mx < m_PrimRect.x || my < m_PrimRect.y || m_PrimRect.w - 1 + m_PrimRect.x < mx || m_PrimRect.h - 1 + m_PrimRect.y < my) {
            EXWnd *pPrev = m_pPrevMouseWnd;
            m_pMenu->Close();
            if (pPrev) pPrev->ButtonDown(1);
            return;
        }
        EXMenu *pMenu = m_pMenu;
        int k = 0;
        for (EXODListItem *pLink = pMenu->m_Items.Head(); pLink; pLink = pLink->Next()) {
            EXMenuItem *pItem = (EXMenuItem *)pLink->Object();
            if (!pItem) continue;
            EXRect r = m_ItemRects[k++];
            if (r.x <= mx && r.y <= my && mx <= r.x - 1 + r.w && my <= r.h - 1 + r.y) {
                ProcessItem(pItem, r);
                return;
            }
        }
        if (pMenu->m_TopLevel) {
            int y = m_PrimRect.y;
            if (pMenu->m_CloseButtonVisible) {
                int cx = m_PrimRect.x - 6 + (unsigned short)(m_DbgWidth - 1) * 12;
                if (cx <= mx && y <= my && mx <= cx + 0x17 && my <= y + 11) {
                    EXWnd *pWnd = pMenu->m_pWnd;
                    if (!pWnd->m_pParent) EXBaseDisplay::GetDisplay()->DelWnd(pWnd);
                    else pWnd->m_pParent->DelChild(pWnd);
                    return;
                }
            }
            if (pMenu->m_PushPinVisible) {
                int px = m_PrimRect.x - 6 + (unsigned short)(m_DbgWidth - 3) * 12;
                if (px <= mx && y <= my && mx <= px + 0x17 && my <= y + 11) {
                    pMenu->m_PushPinIn = pMenu->m_PushPinIn == 0;
                    return;
                }
            }
        }
    } else if (b == 0x20) {
        if (m_ScrollMenu && (unsigned int)m_Unk2d0 != 0) {
            unsigned int step = 5;
            if ((unsigned int)m_Unk2d0 < 5) step = m_Unk2d0;
            m_Unk2d0 -= step;
        }
    } else if (b == 0x40 && m_ScrollMenu) {
        unsigned int limit = (m_NumItems - m_DbgHeight) + 2;
        if ((unsigned int)m_Unk2d0 < limit) {
            unsigned int next = m_Unk2d0 + 5;
            if (limit < next) next = limit;
            m_Unk2d0 = next;
        }
    }
}

// NEAR: retail keeps this in esi and pushes edi late.
void EXMenuWnd::ProcessItem(const EXMenuItem *pItem, const EXRect &r)
{
    if (pItem->m_pSubMenu) {
        if (m_pSubMenu) m_pSubMenu->Close();
        EXPoint p = EXBaseDisplay::PrimToDisp(r.TopLeft());
        EXMenu *pMenu = m_pMenu;
        if (pMenu->TopLevel()) pItem->m_pSubMenu->Open(pMenu->Wnd(), p.x, p.y + r.h);
        else pItem->m_pSubMenu->Open(pMenu->Wnd(), p.x + r.w, p.y);
        m_pSubMenu = pItem->m_pSubMenu;
    } else {
        EXWnd *pWnd = m_pMenu->Wnd();
        pWnd->Menu()->Close();
        pWnd->MenuSelect(pItem->m_ID);
    }
}
