#include "EXDebugWnd.h"
#include "EXMenu.h"
#include "EXBaseDisplay.h"
#include "EXStateSaver.h"
#include "EXStdFuncs.h"
#include "EXMemory.h"
#include "EXWatcher.h"
#include "EXFileSys.h"
#include "EXBaseGamePad.h"

// TODO stand-in: keyboard (EXKeyboard.obj); the retail ButtonDown is a stub.
class EXKeyboard {
public:
    s8 ButtonDown(unsigned char) { return 0; }
    static EXKeyboard *m_pKeyboard;
};
// TODO stand-in: mouse (EXMouse.obj); the cursor position comes first.
class EXMouse {
public:
    EXPoint m_Pos;
    static EXMouse *m_pMouse;
};

EXDebugWndMgr *EXDebugWndMgr::m_pDebugWndMgr;

EXDebugWnd::EXDebugWnd()
{
    m_PrintWhenHidden = 0;
}

void EXDebugWnd::SaveState()
{
    EXStateSaver saver("XB" + m_Title + "DebugWnd");
    saver.Save("X", m_Rect.x);
    saver.Save("Y", m_Rect.y);
    saver.Save("W", m_Rect.w);
    saver.Save("H", m_Rect.h);
    saver.Save("Visible", IsVisible());
}

s8 EXDebugWnd::Update()
{
    return 1;
}

void EXDebugWnd::ButtonUp(unsigned int b)
{
    if (b == 4) SaveState();
}

EXDebugWndMgr::EXDebugWndMgr()
{
    m_pMenu = 0;
    m_MenuID = 0;
}

EXDebugWndMgr::~EXDebugWndMgr()
{
    m_pMenu = 0;
    for (;;) {
        EXODListItem *p = m_Wnds.Head();
        if (!p) break;
        EXDebugWnd *pWnd = (EXDebugWnd *)p->Object();
        if (pWnd) delete pWnd;
    }
}

void EXDebugWndMgr::RegisterMenu(EXMenu *pMenu, unsigned int id)
{
    m_pMenu = pMenu;
    m_MenuID = id;
}

s8 EXDebugWndMgr::MenuSelect(unsigned int id) const
{
    int index = id - m_MenuID;
    if (index < 0) return 0;
    int i = 0;
    for (EXODListItem *p = m_Wnds.Head(); p; p = p->Next()) {
        EXDebugWnd *pWnd = (EXDebugWnd *)p->Object();
        if (pWnd) {
            if (index == i++) {
                if (!pWnd->IsVisible()) {
                    EXBaseDisplay::GetDisplay()->AddWnd(pWnd);
                    pWnd->SaveState();
                }
                return 1;
            }
        }
    }
    return 1;
}

void EXDebugWndMgr::UpdateMenu()
{
    if (m_pMenu) {
        unsigned int id = m_MenuID;
        m_pMenu->DelAllItems();
        for (EXODListItem *p = m_Wnds.Head(); p; p = p->Next()) {
            EXDebugWnd *pWnd = (EXDebugWnd *)p->Object();
            if (pWnd) {
                m_pMenu->AddItem(pWnd->Title(), id);
                id++;
            }
        }
    }
}

void EXDebugDialogWnd::SetText(const char *pText)
{
    DbgClear();
    SetDbgPos(0, 0);
    DbgPrint(pText);
    m_TextX = DbgCol();
    m_TextY = DbgRow();
}

s8 EXDebugDialogWnd::PointInButton(const EXDialogButton *pButton, const EXPoint &p) const
{
    return pButton->m_Rect.ContainsPoint(p);
}

void EXDebugDialogWnd::Destroy()
{
    m_Destroy = 1;
}

s8 EXDebugWnd::Draw()
{
    m_Flags.BitClr(0x20000);
    unsigned short width = DbgWidth();
    unsigned short col = DbgCol();
    unsigned short row = DbgRow();
    memset(m_pDbgText, 0, width * 2);
    SetDbgPos(0, 0);
    DbgPrint(m_Title);
    SetDbgPos(width - 2, 0);
    DbgPrint("%cX", 0x7f);
    SetDbgPos(col, row);
    return 1;
}

void EXDebugWnd::Close()
{
    EXBaseDisplay::GetDisplay()->RemoveWnd(this);
    if (!m_PrintWhenHidden) m_Flags.BitSet(0x20000);
    SaveState();
}

void EXDebugWnd::ButtonDown(unsigned int b)
{
    if (b == 1) {
        EXPoint p = EXBaseDisplay::DispToPrim(EXMouse::m_pMouse->m_Pos);
        EXRect r(m_PrimRect.x + m_DbgWidth * 12 - 0x12, m_PrimRect.y, 0x18, 0xc);
        if (r.ContainsPoint(p)) {
            EXBaseDisplay::GetDisplay()->RemoveWnd(this);
            SaveState();
        }
    }
}

void EXDebugWndMgr::Shutdown()
{
    if (m_pDebugWndMgr) delete m_pDebugWndMgr;
}

void EXDebugWndMgr::DelDebugWnd(EXDebugWnd *pWnd)
{
    m_Wnds.Remove(pWnd->DebugWndListItem());
    UpdateMenu();
}

void EXDebugWndMgr::Update() const
{
    if (EXKeyboard::m_pKeyboard) {
        for (EXODListItem *p = m_Wnds.Head(); p; p = p->Next()) {
            EXDebugWnd *pWnd = (EXDebugWnd *)p->Object();
            if (pWnd && EXKeyboard::m_pKeyboard->ButtonDown(pWnd->ShowKey())) {
                if (pWnd->IsVisible()) pWnd->Close();
                else EXBaseDisplay::GetDisplay()->AddWnd(pWnd);
            }
        }
    }
}

extern EXFixedArray<signed char, 2> g_bTraceInfo;
extern EXFixedArray<signed char, 2> g_bTraceShort;

s8 EXDebugWndMgr::Init(EXDebugWndMgr *pMgr)
{
    EXWatcher *pWatcher = EXWatcher::GetWatcher();
    m_pDebugWndMgr = pMgr;
    unsigned int trace = pWatcher->AddSection("TRACE Control", 0u, "-Engine-", 1);
    unsigned int section = trace;
    if (!section) section = pWatcher->SectionHandle("-Engine-", 0);
    section = pWatcher->AddSection("Engine", section, "-Engine-", 1);
    unsigned int h = pWatcher->AddItem("TRACE_INFO On", section, EXWATCH::DT_8, &g_bTraceInfo[0]);
    pWatcher->EnableItemEdit(h, 1);
    h = pWatcher->AddItem("Short Messages", section, EXWATCH::DT_8, &g_bTraceShort[0]);
    pWatcher->EnableItemEdit(h, 1);
    h = pWatcher->AddItem("File Messages", section, EXWATCH::DT_8, &EXFileSys::DebugLoadInfo());
    pWatcher->EnableItemEdit(h, 1);
    section = trace;
    if (!section) section = pWatcher->SectionHandle("-Engine-", 0);
    section = pWatcher->AddSection("Game", section, "-Engine-", 1);
    h = pWatcher->AddItem("TRACE_INFO On", section, EXWATCH::DT_8, &g_bTraceInfo[1]);
    pWatcher->EnableItemEdit(h, 1);
    h = pWatcher->AddItem("Short Messages", section, EXWATCH::DT_8, &g_bTraceShort[1]);
    pWatcher->EnableItemEdit(h, 1);
    return 1;
}

// Creates (or adopts 'pWnd' as) a debug window titled 'title', restoring its saved position/size/visibility.
EXDebugWnd *EXDebugWndMgr::NewDebugWnd(const EXString &title, unsigned char key, const EXRect &r, ex::RGBA col, EXDebugWnd *pWnd)
{
    EXStateSaver saver("XB" + title + "DebugWnd");
    if (!pWnd) pWnd = new EXDebugWnd;
    pWnd->m_Flags = 0x5dca;
    pWnd->m_BkColor = col;
    pWnd->m_Title = title;
    pWnd->m_ShowKey = key;
    EXRect rect = r;
    rect.x = saver.Restore("X", (unsigned int)r.x);
    rect.y = saver.Restore("Y", (unsigned int)r.y);
    rect.w = saver.Restore("W", (unsigned int)r.w);
    rect.h = saver.Restore("H", (unsigned int)r.h);
    pWnd->SetRect(rect);
    if (saver.Restore("Visible", 0u)) EXBaseDisplay::GetDisplay()->AddWnd(pWnd);
    pWnd->m_Flags = pWnd->m_Flags | 0x20000;
    m_Wnds.AddTail(pWnd->m_ListItem, pWnd);
    UpdateMenu();
    return pWnd;
}

EXDebugDialogWnd::EXDebugDialogWnd()
{
    m_Flags = 0x5d08;
    SetBkColor(ex::RGBA(0, 0x80, 0x28, 0x40));
    int w = EXBaseDisplay::GetDisplay()->Width();
    int h = EXBaseDisplay::GetDisplay()->Height();
    SetRect(EXRect(w / 4, h / 4, w / 2, h / 2));
    m_Return = 0;
    m_HasReturn = 0;
    m_TextX = 0;
    m_TextY = 0;
    EXBaseDisplay::GetDisplay()->AddWnd(this);
    m_Destroy = 0;
}

// Lay the button out after the previous one (or below the text) and print its caption.
void EXDebugDialogWnd::AddButton(EXDialogButton &b)
{
    char *pText = b.m_Text;
    int len = 0;
    while (pText[len]) len++;
    if (len == 0) {
        pText[0] = '?';
        pText[1] = '?';
        pText[2] = '?';
        pText[3] = 0;
    }
    int x, y;
    int n = m_Buttons.Size();
    if (n == 0) {
        x = 2;
        y = m_TextY + 2;
    } else {
        const EXDialogButton &prev = m_Buttons[n - 1];
        const char *pPrev = prev.m_Text;
        int prevLen = 0;
        while (pPrev[prevLen]) prevLen++;
        x = prev.m_Unk34 + 2 + prevLen;
        y = prev.m_Unk38;
    }
    b.m_Unk34 = x;
    b.m_Unk38 = y;
    len = 0;
    while (pText[len]) len++;
    b.m_Rect.y = y * 12 - 5;
    b.m_Rect.x = x * 12 - 5;
    b.m_Rect.w = len * 12 + 10;
    b.m_Rect.h = 0x16;
    SetDbgPos((unsigned short)x, (unsigned short)y);
    DbgPrint(pText);
    m_Buttons.Add(b, 1);
}

s8 EXDebugDialogWnd::Update()
{
    EXBaseGamePad *pPad = EXBaseGamePad::GetGamePad();
    if (!m_Destroy) {
        int n = m_Buttons.Size();
        if (!m_HasReturn && pPad && n) {
            if (pPad->ButtonPressed(0, 0x444)) {
                if (m_Return < 1) m_Return = 0;
                else m_Return = m_Return - 1;
            }
            if (pPad->ButtonPressed(0, 0x888)) {
                if (m_Return < n - 1) m_Return = m_Return + 1;
                else m_Return = n - 1;
            }
            if (pPad->ButtonPressed(0, 0x3f000) && m_Return >= 0 && m_Return < n) m_HasReturn = 1;
        }
        return 1;
    }
    m_Destroy = 0;
    delete this;
    return 0;
}

void EXDebugDialogWnd::ButtonDown(unsigned int b)
{
    if (!m_HasReturn && b == 1) {
        int px = EXMouse::m_pMouse->m_Pos.x + EXBaseDisplay::Offset().x - m_PrimRect.x;
        int py = EXMouse::m_pMouse->m_Pos.y + EXBaseDisplay::Offset().y - m_PrimRect.y;
        for (int i = 0; i < m_Buttons.Size(); i++) {
            const EXRect &r = m_Buttons[i].m_Rect;
            if (px < r.x || py < r.y || r.x + r.w - 1 < px || r.y + r.h - 1 < py) continue;
            m_HasReturn = 1;
            m_Return = i;
            return;
        }
    }
}

void EXDebugDialogWnd::MouseMove(int, int)
{
    if (!m_HasReturn) {
        int py = EXMouse::m_pMouse->m_Pos.y + EXBaseDisplay::Offset().y - m_PrimRect.y;
        int px = EXMouse::m_pMouse->m_Pos.x + EXBaseDisplay::Offset().x - m_PrimRect.x;
        for (int i = 0; i < m_Buttons.Size(); i++) {
            const EXRect &r = m_Buttons[i].m_Rect;
            if (px < r.x || py < r.y || r.x + r.w - 1 < px || r.y + r.h - 1 < py) continue;
            m_Return = i;
            return;
        }
    }
}

EXDebugDialogWnd::~EXDebugDialogWnd()
{
    m_Buttons.Empty(1);
    EXBaseDisplay::GetDisplay()->RemoveWnd(this);
}

void EXDebugDialogWnd::Show()
{
    m_Flags.BitSet(6);
    m_HasReturn = 0;
    m_Return = 0;
}

void EXDebugDialogWnd::Hide()
{
    m_Flags.BitClr(6);
    m_Buttons.Empty(1);
}

s8 EXDebugDialogWnd::GetReturn(int *pRet)
{
    if (m_HasReturn && pRet && m_Return >= 0 && m_Return <= m_Buttons.Size()) {
        *pRet = m_Buttons[m_Return].m_Value;
        return 1;
    }
    return 0;
}

s8 EXDebugDialogWnd::Draw()
{
    return 1;
}

EXDebugWnd::~EXDebugWnd()
{
    EXDebugWndMgr::GetDebugWndMgr()->DelDebugWnd(this);
}
