#include "EXMemCard.h"
#include "EXMemory.h"
#include "EXBaseDisplay.h"
#include "EXBaseGamePad.h"
#include "EXArray.h"
#include <string.h>
#include <stdio.h>

EXMemCard g_MemCard;

EXBaseMemCard::~EXBaseMemCard()
{
    if (m_pDlg) delete m_pDlg;
}

void EXBaseMemCard::StartMultiCardOp()
{
    m_bMultiCardOp = 1;
}

void EXBaseMemCard::EndMultiCardOp()
{
    m_bDlgLocked = 0;
    m_bMultiCardOp = 0;
}

s8 EXBaseMemCard::Init(const EXSaveInfo &info)
{
    return 1;
}

int EXBaseMemCard::CardChanged(signed char &b)
{
    b = 0;
    return 1;
}

int EXBaseMemCard::CheckSavePossible(unsigned int n)
{
    return 1;
}

int EXBaseMemCard::CheckLoadPossible(unsigned int n)
{
    return 1;
}

int EXBaseMemCard::SavesPresent(unsigned char &mask, unsigned int n)
{
    mask = 0;
    return 1;
}

int EXBaseMemCard::Save(unsigned char slot, void *p, unsigned int n)
{
    return -1;
}

int EXBaseMemCard::Load(unsigned char slot, void *p, unsigned int a, unsigned int b)
{
    return -1;
}

signed char EXBaseMemCard::CheckHardDiskSpace()
{
    return -1;
}

void EXBaseMemCard::ShowDlg(unsigned int type, unsigned int sel)
{
    if (m_bDlgLocked) return;
    if (m_pDlg) return;
    m_DlgData.m_Type = type;
    m_DlgData.m_Selected = (u16)sel;
    m_pDlg = (EXMemCardDlg *)m_pDlgClass->CreateObject();
    m_pDlg->ShowDlg(m_DlgData);
}

s8 EXBaseMemCard::DlgVisible() const
{
    return m_pDlg != 0;
}

void EXBaseMemCard::HideDlg()
{
    if (m_bDlgLocked) return;
    if (!m_pDlg) return;
    m_pDlg->HideDlg();
    if (m_pDlg) delete m_pDlg;
    m_pDlg = 0;
}

unsigned int EXBaseMemCard::DlgResult()
{
    if (!m_pDlg) return 5;
    return m_pDlg->DlgResult();
}

s8 EXMemCardDlgWnd::Draw()
{
    m_Frame++;
    return 1;
}

EX_IMPLEMENT_DYNCREATE_ROOT(EXMemCardDlg)

EXMemCardDlg::EXMemCardDlg()
{
    m_pWnd = 0;
}

EXMemCardDlg::~EXMemCardDlg()
{
    if (m_pWnd) EXBaseDisplay::GetDisplay()->DelWnd(m_pWnd);
}

void EXMemCardDlg::HideDlg()
{
    if (m_pWnd) EXBaseDisplay::GetDisplay()->DelWnd(m_pWnd);
    m_pWnd = 0;
}

unsigned int EXMemCardDlg::DlgResult()
{
    if (m_pWnd) return m_pWnd->m_Result;
    return 5;
}

EXBaseMemCard::EXBaseMemCard()
{
    m_pDlgClass = EXRUNTIME_CLASS(EXMemCardDlg);
    m_pDlg = 0;
    m_DlgData.m_Message = (unsigned int)-1;
    m_DlgData.m_ButtonHash[0] = (unsigned int)-1;
    m_DlgData.m_pText = "No Message";
    m_DlgData.m_pButton[0] = "OK";
    m_DlgData.m_NumButtons = 1;
    m_DlgData.m_Selected = 0;
    m_DlgData.m_pSlotName = "1";
    m_DlgData.m_MemRequired = 1;
    m_bMultiCardOp = 0;
    m_bDlgLocked = 0;
}

const char *EXBaseMemCard::Message(int code) const
{
    EXFixedArray<MemCrdMsgData_tag, 10> msgs;
    msgs[0].m_Code = 0;
    msgs[0].m_pText = "Waiting on completion";
    msgs[1].m_Code = 1;
    msgs[1].m_pText = "Operation completed successfully";
    msgs[2].m_Code = 2;
    msgs[2].m_pText = "Request ignored due to in-progress operation";
    msgs[3].m_Code = -1;
    msgs[3].m_pText = "No memory card present";
    msgs[4].m_Code = -2;
    msgs[4].m_pText = "No save data present";
    msgs[5].m_Code = -3;
    msgs[5].m_pText = "No space on card";
    msgs[6].m_Code = -4;
    msgs[6].m_pText = "Card operation failed";
    msgs[7].m_Code = -5;
    msgs[7].m_pText = "Card was changed since last card operation";
    msgs[8].m_Code = -6;
    msgs[8].m_pText = "Card operation aborted at user request";
    msgs[9].m_Code = -7;
    msgs[9].m_pText = "Card operation failed due to unspecified error";
    for (unsigned int i = 0; i < msgs.Size(); i++) {
        if (code == msgs[i].m_Code) return msgs[i].m_pText;
    }
    return "Unknown Message Code";
}

void EXBaseMemCard::SetDlgHashCodes(unsigned int msg, unsigned int n, unsigned int b0, unsigned int b1, unsigned int b2, unsigned int b3)
{
    if (m_bDlgLocked) return;
    m_DlgData.m_Message = msg;
    m_DlgData.m_NumButtons = (u16)n;
    m_DlgData.m_ButtonHash[0] = b0;
    m_DlgData.m_ButtonHash[1] = b1;
    m_DlgData.m_ButtonHash[2] = b2;
    m_DlgData.m_ButtonHash[3] = b3;
}

void EXBaseMemCard::SetDlgMessages(const char *pMsg, unsigned int n, const char *b0, const char *b1, const char *b2, const char *b3)
{
    if (m_bDlgLocked) return;
    m_DlgData.m_pText = (char *)pMsg;
    m_DlgData.m_NumButtons = (u16)n;
    m_DlgData.m_pButton[0] = b0;
    m_DlgData.m_pButton[1] = b1;
    m_DlgData.m_pButton[2] = b2;
    m_DlgData.m_pButton[3] = b3;
}

s8 EXMemCardDlgWnd::Update()
{
    switch (m_Type) {
    case 1:
        if (m_Result == 0) {
            if (EXBaseGamePad::GetGamePad()->ButtonPressed(0, 0x2000) && m_NumButtons > 0) m_Result = 1;
            if (EXBaseGamePad::GetGamePad()->ButtonPressed(0, 0x8000) && m_NumButtons > 1) m_Result = 2;
            if (EXBaseGamePad::GetGamePad()->ButtonPressed(0, 0x4000) && m_NumButtons > 2) m_Result = 3;
            if (EXBaseGamePad::GetGamePad()->ButtonPressed(0, 0x1000) && m_NumButtons > 3) m_Result = 4;
        }
        break;
    case 2:
        if (m_Result == 0) {
            if (EXBaseGamePad::GetGamePad()->ButtonPressed(0, 0x2000)) m_Result = m_Selected + 1;
            if (EXBaseGamePad::GetGamePad()->ButtonPressed(0, 1)) {
                m_Selected = m_Selected - 1 > 0 ? m_Selected - 1 : 0;
            }
            if (EXBaseGamePad::GetGamePad()->ButtonPressed(0, 2)) {
                int s = m_Selected + 1;
                int n = m_NumButtons - 1;
                m_Selected = s < n ? s : n;
            }
        }
        break;
    }
    return 1;
}

void EXMemCardDlg::ShowDlg(const EXMemCardDlgData &d)
{
    u16 w = EXBaseDisplay::GetDisplay()->Width();
    u16 h = EXBaseDisplay::GetDisplay()->Height();
    m_pWnd = new EXMemCardDlgWnd;
    m_pWnd->m_Flags = 0x4cca;
    m_pWnd->SetRect(EXRect(w / 2 - 0x78, h / 2 - 0x5a, 0xf0, 0xb4));
    m_pWnd->m_BkColor = ex::RGBA(0, 0, 0x60, 0x40);
    m_pWnd->SetOriginToCentre();
    m_pWnd->m_Result = 0;
    m_pWnd->m_Type = d.m_Type;
    m_pWnd->m_Selected = d.m_Selected;
    m_pWnd->m_NumButtons = d.m_NumButtons;
    EXBaseDisplay::GetDisplay()->AddWnd(m_pWnd);

    EXFixedArray<char, 350> text;
    const char *src = d.m_pText;
    const char *end = src + strlen(src) + 1;
    char *out = text;
    while (src < end) {
        if (*src == '(') {
            if (!strncmp(src, "(mcSLOT)", 8)) {
                strcpy(out, d.m_pSlotName);
                out += strlen(d.m_pSlotName);
                src += 8;
            } else if (!strncmp(src, "(mcNAME)", 8)) {
                strcpy(out, "MyGame");
                out += strlen("MyGame");
                src += 8;
            } else if (!strncmp(src, "(mcOK)", 6)) {
                strcpy(out, "OK");
                out += strlen("OK");
                src += 6;
            } else if (!strncmp(src, "(mcKB)", 6)) {
                char num[20];
                sprintf(num, "%d", d.m_MemRequired);
                strcpy(out, num);
                out += strlen(num);
                src += 6;
            } else {
                *out++ = *src++;
            }
        } else {
            *out++ = *src++;
        }
    }

    char *pEnd = text + strlen(text) + 1;
    u16 width = m_pWnd->m_DbgWidth;
    char *p = text;
    u16 row = 0;
    char *line = p;
    char *space = 0;
    for (; p < pEnd; p++) {
        if (*p == ' ' || *p == '\t') space = p;
        if (p - line >= width) {
            if (space) {
                *space = 0;
                m_pWnd->SetDbgPos(0, row++);
                m_pWnd->DbgPrint(line);
                line = space + 1;
                space = 0;
            } else {
                p[-1] = 0;
                m_pWnd->SetDbgPos(0, row++);
                m_pWnd->DbgPrint(line);
                line = p;
            }
        }
    }
    if (line < pEnd) {
        m_pWnd->SetDbgPos(0, row);
        m_pWnd->DbgPrint(line);
    }

    if (d.m_Type == 2) {
        u16 y = m_pWnd->m_DbgHeight - d.m_NumButtons;
        for (u16 i = 0; i < d.m_NumButtons; i++) {
            if (d.m_pButton[i]) {
                m_pWnd->SetDbgPos(m_pWnd->m_DbgWidth / 2 - strlen(d.m_pButton[i]) / 2, y++);
                m_pWnd->DbgPrint(d.m_pButton[i]);
            } else {
                y++;
            }
        }
    } else if (d.m_Type == 1) {
        u16 y = m_pWnd->m_DbgHeight - d.m_NumButtons;
        for (u16 i = 0; i < d.m_NumButtons; i++) {
            if (d.m_pButton[i]) {
                m_pWnd->SetDbgPos(1, y++);
                m_pWnd->DbgPrint("%c: %s", "ABXY"[i], d.m_pButton[i]);
            } else {
                y++;
            }
        }
    }
}
