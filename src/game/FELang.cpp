#include "FEText.h"
#include "EXMemory.h"
#include "XAppStandIn.h"
#include "MemCardStandIn.h"
#include "FELang.h"
#include "FEListener.h"
#include "EXArray.h"

extern unsigned int gSystemCameraUser;

int MemCard_LastTraceFlow;
int MemCard_Flow;
int MemCard_LastRetVal;
int MemCard_RetVal;
signed char MemCard_bLoading;
signed char MemCard_bAutoLoad;
signed char MemCard_bAutoSave;
signed char MemCard_bOnLoadGameScreen;
unsigned char MemCard_SavesFoundBits;
signed char MemCard_bNotFormatted;
signed char MemCard_bBootChecking = 1;
int Stats_FaveWeapon;
static unsigned int s_MemCardSlot;
FELang *FELang::m_pInstance;
GameDialog MemCard_ExtraDialog;
EXFixedArray<int, 3> MemCard_ExtraDialog_OutFlow;

GameDialog &MemCardExtraDialog() { return MemCard_ExtraDialog; }

void SetMemCardFlow(int flow)
{
    MemCard_Flow = flow;
    MemCard_RetVal = 0;
    MemCard_LastTraceFlow = -999;
    MemCard_LastRetVal = -999;
}

int MemCardFlow() { return MemCard_Flow; }

unsigned int MemCardDlgResult() { return MemCard_ExtraDialog.DlgResult(); }

signed char MemCardDlgIsOn() { return MemCard_ExtraDialog.Wnd() != 0; }

signed char MemCardBusy()
{
    if (MemCard_RetVal == 0 && MemCard_LastRetVal != -999) return 1;
    return 0;
}

unsigned int MemCardDlgMessageHash()
{
    if (!MemCard_ExtraDialog.Wnd()) return 0xffffffff;
    return MemCard_ExtraDialog.Data().m_Message;
}

void MemCard_FlowTrace(char *pText, signed char b)
{
    if (MemCard_Flow != MemCard_LastTraceFlow) MemCard_LastTraceFlow = MemCard_Flow;
}

void MemCard_Trace(char *pText) {}

void MemCard_RetTrace(char *pText)
{
    if (MemCard_RetVal != MemCard_LastRetVal) MemCard_LastRetVal = MemCard_RetVal;
}

void MemCard_TraceState(char *pText)
{
    if (MemCard_Flow != MemCard_LastTraceFlow) MemCard_LastTraceFlow = MemCard_Flow;
}

void Hide_MemCardExtraDialog_NoDestroy() { MemCard_ExtraDialog.HideDlg_NoDestroy(); }

void Show_MemCardExtraDialog_NoCreate() { MemCard_ExtraDialog.ShowDlg_NoCreate(); }

FELang *FELang::Instance(signed char create)
{
    if (!m_pInstance && create) m_pInstance = new FELang;
    return m_pInstance;
}

void FELang::Destroy()
{
    if (this) {
        ContentDestroy();
        delete this;
    }
}

void FELang::ContentCreate() {}

void FELang::ContentUpdate()
{
    TickPage();
    FEListener::LastInputReset();
    HandleInput();
}

void FELang::ContentDestroy()
{
    if (!ContentDestroyed()) {
        FEListener::RemapModeSet((FERemap::Flags)0);
        FEWnd::ContentDestroy();
        theApp.m_Unke54 = 0;
    }
}

signed char FELang::Draw()
{
    gSystemCameraUser = 0;
    return FEWnd::Draw();
}

void MemCard_SaveInit()
{
    MemCard_bAutoLoad = 0;
    MemCard_bLoading = 0;
    MemCard_bOnLoadGameScreen = 0;
    MemCard_bAutoSave = 0;
}

void ResetMemCardSaveFoundBits() { MemCard_SavesFoundBits = 0; }

signed char MemCard_HasBeenRemoved() { return 0; }

void MemCardSetSlot(unsigned int slot) { s_MemCardSlot = slot; }

unsigned int MemCardSlot() { return s_MemCardSlot; }

void MemCardEnableAutoSave(signed char b) { MemCard_bAutoSave = b; }

signed char MemCardAutoSaveEnabled() { return MemCard_bAutoSave; }

unsigned int MemCardSavesFound() { return MemCard_SavesFoundBits; }

signed char MemCard_BootChecks(signed char b)
{
    MemCard_bBootChecking = 0;
    return 0;
}

void MemCard_LoadInit()
{
    MemCard_bAutoLoad = 1;
    MemCard_bLoading = 1;
    MemCard_bOnLoadGameScreen = 1;
    MemCard_bAutoSave = 0;
}

void TraceToFile(const char *pFmt, ...) {}

void InitTraceFile() {}

int Hide_MemCardExtraDialog(signed char reset)
{
    int flow = MemCard_ExtraDialog_OutFlow[0];
    if (MemCard_ExtraDialog.Wnd()) {
        if (MemCard_ExtraDialog.Wnd()->m_pButtons) {
            if (MemCard_ExtraDialog.Wnd()->m_Result == 2) flow = MemCard_ExtraDialog_OutFlow[1];
            else if (MemCard_ExtraDialog.Wnd()->m_Result == 3) flow = MemCard_ExtraDialog_OutFlow[2];
        }
        MemCard_ExtraDialog.HideDlg();
    }
    if (reset) SetMemCardFlow(flow);
    return flow;
}

int MemCardDlgUpdate(signed char reset)
{
    if (!MemCard_ExtraDialog.Wnd()) return 0;
    if (!MemCard_ExtraDialog.Wnd()->m_Result) return 0;
    return Hide_MemCardExtraDialog(reset);
}

void MemCardDlgHide(signed char reset) { Hide_MemCardExtraDialog(reset); }

#include "GameData.h"
#include "XItemHandler_Player.h"
#include "EXWString.h"

static char s_HashToCharStrBuf[0x100];
static char s_WideToCharStrBuf[0x100];

// Copies a localised string into pBuf (or a shared static buffer when pBuf is null).
char *HashToCharStr(unsigned int hash, char *pBuf)
{
    if (!pBuf) pBuf = s_HashToCharStrBuf;
    EXString s = gText.Get(hash);
    char *src = s.GetBuffer(0);
    memcpy(pBuf, src, s.Length() + 1);
    s.ReleaseBuffer();
    return pBuf;
}

// Narrows a wide string to 8-bit, mapping the curly apostrophe and the 0xac glyph.
char *WideToCharStr(EXWString &ws, char *pBuf)
{
    if (!pBuf) pBuf = s_WideToCharStrBuf;
    unsigned short *w = ws.GetBuffer(0);
    int n = 0;
    while (*w) {
        unsigned short c = *w++;
        if (c == 0x2019) c = 0x27;
        else if (c == 0xac) c = 0xa0;
        pBuf[n++] = (char)c;
    }
    ws.ReleaseBuffer();
    pBuf[n] = 0;
    return pBuf;
}

void Stats_InitPageValues(unsigned int level)
{
    int fave = XItemHandler_Player::Instance()->GetFaveWeapon();
    GameData::Instance(1)->Stats()->m_Unk48 = fave;
    Stats_FaveWeapon = fave;
}

#include <stdio.h>

BigSave MemCard_LoadSpace_BigSave;
// Save-game attribute flags passed to the card (set when a save already exists).
unsigned int MemCard_SaveFlags;
unsigned int MemCard_LoadSize;

int MemCard_CheckSavePossible(signed char start)
{
    if (start) {
        if (GameData::Instance(1)->MemCard().XBIsSaveGamePresent(0, 0)) MemCard_SaveFlags = 0x10000;
        MemCard_LastTraceFlow = -999;
        MemCard_LastRetVal = -999;
        MemCard_Flow = 3;
        MemCard_RetVal = 0;
        MemCard_bNotFormatted = 0;
        return 0;
    }
    int ret = MemCard_RetVal;
    int prev;
    do {
        prev = ret;
        if (MemCard_Flow != MemCard_LastTraceFlow) MemCard_LastTraceFlow = MemCard_Flow;
        if (MemCard_Flow == 3 && ret == 0) {
            unsigned int flags = MemCard_SaveFlags | 0x8010;
            if (MemCard_bBootChecking) flags |= 8;
            ret = GameData::Instance(1)->MemCard().CheckSavePossible(flags);
            MemCard_RetVal = ret;
            if (ret != MemCard_LastRetVal) MemCard_LastRetVal = ret;
        }
    } while (prev == 0 && ret != 0);
    if (ret != MemCard_LastRetVal) MemCard_LastRetVal = ret;
    return ret;
}

int MemCard_CardChanged(signed char start)
{
    BigSave *bs = GameData::Instance(1)->GetBigSave();
    if (start) {
        MemCard_LastTraceFlow = -999;
        MemCard_LastRetVal = -999;
        MemCard_Flow = 2;
        MemCard_RetVal = 0;
        return 0;
    }
    int ret = MemCard_RetVal;
    int prev;
    do {
        prev = ret;
        if (MemCard_Flow != MemCard_LastTraceFlow) MemCard_LastTraceFlow = MemCard_Flow;
        if (MemCard_Flow == 2) {
            switch (ret) {
            case 0: {
                signed char changed = 0;
                ret = GameData::Instance(1)->MemCard().CardChanged(changed);
                MemCard_RetVal = ret;
                if (ret != MemCard_LastRetVal) MemCard_LastRetVal = ret;
                if (changed) {
                    bs->SetStatusAndReset(0);
                    return -5;
                }
                break;
            }
            case 1: break;
            default: {
                char msg[0x20];
                sprintf(msg, "default: %d", ret);
            }
            case -1:
                bs->SetStatusAndReset(0);
                ret = MemCard_RetVal;
                break;
            }
        }
    } while (prev == 0 && ret != 0);
    if (ret != MemCard_LastRetVal) MemCard_LastRetVal = ret;
    return ret;
}

#include "FEMainMenu.h"
#include "FEManager.h"

unsigned int MemCard_DialogDefaultOpt;
static char s_ExtraDialogText[0x100];
static unsigned int s_ContinueButtons[2] = {0x2000, 0};

static u32 MemCard_RequiredKB() { return (u32)((float)(GameData::Instance(1)->SaveInfo().m_Size + 0xff) * 0.00390625f); }

// Opens the extra dialog on (up to three) result-dependent flows. numButtons==0 with type 1 shows the four face buttons.
void Show_MemCardExtraDialog(char *text, int flow0, unsigned int msg, unsigned int custom, unsigned int numButtons, unsigned int btn0,
                             unsigned int btn1, int flow1, unsigned int btn2, int flow2, int n, unsigned int *pArr)
{
    MemCard_ExtraDialog.HideDlg();
    if (flow1 == -1) flow1 = flow0;
    EXMemCardDlgData d;
    memset(&d, 0, sizeof(d));
    u32 type = custom;
    u16 count = (u16)numButtons;
    if (custom) {
        type = 2;
        if (numButtons > 0) d.m_ButtonHash[0] = btn0;
        if (numButtons > 1) d.m_ButtonHash[1] = btn1;
        if (numButtons > 2) d.m_ButtonHash[2] = btn2;
        d.m_Selected = (u16)MemCard_DialogDefaultOpt;
    }
    if (numButtons == 0) {
        type = 1;
        count = 4;
        d.m_ButtonHash[0] = d.m_ButtonHash[1] = d.m_ButtonHash[2] = d.m_ButtonHash[3] = 0;
    }
    d.m_NumButtons = count;
    MemCard_DialogDefaultOpt = 0;
    d.m_Type = type;
    if (text && type == 0) {
        sprintf(s_ExtraDialogText, "%s\nPress Any Button", text);
        text = s_ExtraDialogText;
    }
    d.m_pText = text;
    d.m_Message = msg;
    d.m_pSlotName = "1";
    d.m_MemRequired = MemCard_RequiredKB();
    MemCard_ExtraDialog.ShowDlg(d, n, pArr);
    MemCard_RetVal = 0;
    MemCard_ExtraDialog_OutFlow[0] = flow0;
    MemCard_ExtraDialog_OutFlow[1] = flow1;
    MemCard_ExtraDialog_OutFlow[2] = flow2;
    MemCard_Flow = 200;
    MemCard_LastTraceFlow = -999;
    MemCard_LastRetVal = -999;
}

// Simple one-message dialog with the four face buttons; any result goes to flow.
void Show_MemCardExtraDialog(unsigned int msg, int flow)
{
    MemCard_ExtraDialog.HideDlg();
    EXMemCardDlgData d;
    memset(&d, 0, sizeof(d));
    d.m_Type = 1;
    d.m_Message = msg;
    d.m_ButtonHash[0] = d.m_ButtonHash[1] = d.m_ButtonHash[2] = d.m_ButtonHash[3] = 0;
    MemCard_DialogDefaultOpt = 0;
    d.m_NumButtons = 4;
    d.m_pText = 0;
    d.m_pSlotName = "1";
    d.m_MemRequired = MemCard_RequiredKB();
    MemCard_ExtraDialog.ShowDlg(d, 0xffffffff, 0);
    MemCard_ExtraDialog_OutFlow[0] = flow;
    MemCard_ExtraDialog_OutFlow[1] = flow;
    MemCard_RetVal = 0;
    MemCard_ExtraDialog_OutFlow[2] = -1;
    MemCard_Flow = 200;
    MemCard_LastTraceFlow = -999;
    MemCard_LastRetVal = -999;
}

// Pops one of the stock Yes/No/Retry/Continue... dialogs; 'type' picks the button set.
void Show_MemCardExtraDialog_Query(unsigned int msg, int a, int b, int c, unsigned int type, int e, int f, unsigned int *p)
{
    unsigned int ebx = 0, ecx = 0x51000214 /* Yes */, esi = 0x51000215 /* No */;
    unsigned int count = 2;
    bool inRetryPage = false;
    if (FEMainMenu::Instance() && FEMainMenu::Instance()->m_pManager->m_PageState == 0xc) inRetryPage = true;
    switch (type) {
    case 1: ecx = 0x51000217; esi = 0x51000219; break;                        // Retry / Cancel
    case 2: ecx = 0x51000217; esi = 0x5100021a; break;                        // Retry / Continue into game
    case 3: ecx = 0x51000837; esi = 0x5100082e; ebx = 0x51000219; count = 3; break;  // Save / Continue without saving / Cancel
    case 4: ecx = 0x510002fd; esi = 0x51000219; break;                        // Continue / Cancel
    case 5: ecx = 0x51000216; esi = 0; count = 1; break;                      // OK
    case 6: ecx = 0x510002fd; esi = 0; count = 1; break;                      // Continue
    case 7: ecx = 0x51000219; esi = 0; count = 1; break;                      // Cancel
    case 9: ecx = 0x51000217; esi = 0x51000219;                               // Retry / Cancel / ...
    case 8:
        ebx = 0x5100082e;                                                     // ... Continue without saving
        count = 3;
        if (inRetryPage) {
            count = 2;
            b = 4;
        }
        break;
    case 10: count = 0; break;
    case 11:
    case 12: count = 1; ecx = 0x5100011a; p = s_ContinueButtons; break;       // Press ~E to continue.
    case 13: ebx = 0x510002fd; count = 3; esi = 0x51000219; ecx = 0x51000217; break;
    }
    MemCard_DialogDefaultOpt = c;
    Show_MemCardExtraDialog(0, a, msg, 2, count, ecx, esi, b, ebx, e, f, p);
}

int MemCard_Load(signed char start)
{
    BigSave *bs = GameData::Instance(1)->GetBigSave();
    if (start) {
        MemCard_LoadSize = GameData::Instance(1)->SaveInfo().m_Size;
        MemCard_RetVal = 0;
        MemCard_LastTraceFlow = -999;
        MemCard_LastRetVal = -999;
        MemCard_Flow = 6;
        return 0;
    }
    int ret = MemCard_RetVal;
    int prev;
    do {
        prev = ret;
        if (MemCard_Flow != MemCard_LastTraceFlow) MemCard_LastTraceFlow = MemCard_Flow;
        if (MemCard_Flow == 6) {
            switch (ret) {
            case 0:
                ret = 1;
                MemCard_RetVal = ret;
                break;
            case 1:
                if (MemCard_LoadSpace_BigSave.m_Magic == 0x89ab001d) {
                    unsigned int slot = s_MemCardSlot;
                    if ((signed char)MemCard_LoadSpace_BigSave.m_SaveFound[slot] != 0) {
                        bs->m_Saves[slot] = MemCard_LoadSpace_BigSave.m_Saves[slot];
                        GameData::Instance(1)->m_Values = bs->m_Saves[s_MemCardSlot];
                        MemCard_bAutoSave = 1;
                        ret = MemCard_RetVal;
                        break;
                    }
                    bs->SetStatusAndReset(0);
                }
                ret = -4;
                MemCard_RetVal = ret;
                MemCard_bAutoSave = 0;
                break;
            case -1:
                bs->SetStatusAndReset(0);
                ret = MemCard_RetVal;
                MemCard_bAutoSave = 0;
                break;
            case -7:
                ret = -4;
                MemCard_RetVal = ret;
                MemCard_bAutoSave = 0;
                break;
            case -6:
            case -4:
            case -2:
                MemCard_bAutoSave = 0;
                break;
            }
        }
    } while (prev == 0 && ret != 0);
    if (ret != MemCard_LastRetVal) MemCard_LastRetVal = ret;
    return ret;
}

static unsigned int s_SaveParam;

int MemCard_Save(signed char start, unsigned int param)
{
    BigSave *bs = GameData::Instance(1)->GetBigSave();
    if (start) {
        s_SaveParam = param;
        bs->m_Saves[s_MemCardSlot] = GameData::Instance(1)->m_Values;
        *(u8 *)&bs->m_SaveFound[s_MemCardSlot] = 1;
        GameData *gd = GameData::Instance(1);
        if (gd->m_Values.m_Rituals != 0 && gd->m_RitualTimes.m_Unk4[0] == 0) {
            bs->m_RitualTimes = GameData::Instance(1)->m_RitualTimes;
            *(u8 *)&bs->m_SaveFound[6] = 1;
        }
        MemCard_LastTraceFlow = -999;
        MemCard_LastRetVal = -999;
        MemCard_Flow = 8;
        MemCard_RetVal = 0;
        return 0;
    }
    int ret = MemCard_RetVal;
    int prev;
    do {
        prev = ret;
        if (MemCard_Flow != MemCard_LastTraceFlow) MemCard_LastTraceFlow = MemCard_Flow;
        if (MemCard_Flow == 8) {
            switch (ret) {
            case 0: {
                unsigned int flags = s_SaveParam;
                if (!(MemCard_SavesFoundBits & (1 << s_MemCardSlot))) flags = 0x20;
                if (MemCard_bAutoSave) flags = 0x20;
                ret = GameData::Instance(1)->MemCard().Save(0, bs, flags | 0x38000);
                MemCard_RetVal = ret;
                if (ret != MemCard_LastRetVal) MemCard_LastRetVal = ret;
                break;
            }
            case 1:
                MemCard_SavesFoundBits |= (unsigned char)(1 << s_MemCardSlot);
                MemCard_LoadSpace_BigSave = *bs;
                bs->SetStatus(1);
                ret = MemCard_RetVal;
                break;
            case -7:
            case -5:
            case -4:
            case -1:
                bs->SetStatusAndReset(0);
                ret = MemCard_RetVal;
                break;
            }
        } else if (MemCard_Flow == 200) {
            if (MemCard_ExtraDialog.Wnd() && MemCard_ExtraDialog.Wnd()->m_Result) Hide_MemCardExtraDialog(1);
            ret = 0;
            MemCard_RetVal = 0;
        }
    } while (prev == 0 && ret != 0);
    if (ret != MemCard_LastRetVal) MemCard_LastRetVal = ret;
    return ret;
}

static unsigned int s_CheckSavesParam;
static unsigned char s_SavesMask;
static signed char s_bFirstCardCheck;
signed char MemCard_bBootCheck2 = 1;

int MemCard_CheckSavesPresent(signed char start, unsigned int param)
{
    BigSave *bs = GameData::Instance(1)->GetBigSave();
    if (start) {
        s_CheckSavesParam = param;
        MemCard_LastTraceFlow = -999;
        MemCard_LastRetVal = -999;
        MemCard_SavesFoundBits = 0;
        s_SavesMask = 0;
        s_bFirstCardCheck = 1;
        MemCard_Flow = 2;
        MemCard_RetVal = 0;
        MemCard_bNotFormatted = 0;
        return 0;
    }
    int ret = MemCard_RetVal;
    int prev;
    do {
        prev = ret;
        if (MemCard_Flow != MemCard_LastTraceFlow) MemCard_LastTraceFlow = MemCard_Flow;
        int flow = MemCard_Flow;
        if (flow == 0x1f4) {
            MemCard_LoadSize = GameData::Instance(1)->SaveInfo().m_Size;
            MemCard_Flow = 6;
            ret = 0;
            MemCard_RetVal = 0;
            MemCard_LastTraceFlow = -999;
            MemCard_LastRetVal = -999;
            flow = 6;
        }
        if (flow == 6) {
            bool fail = true;
            switch (ret) {
            case -6:
                bs->SetStatusAndReset(0);
                ret = MemCard_RetVal;
                MemCard_bAutoSave = 0;
                MemCard_SavesFoundBits = 0;
                fail = false;
                break;
            case 0:
                ret = GameData::Instance(1)->MemCard().Load(0, &MemCard_LoadSpace_BigSave, s_CheckSavesParam | 0x100803, MemCard_LoadSize);
                fail = false;
                if (ret != MemCard_LastRetVal) MemCard_LastRetVal = ret;
                MemCard_RetVal = ret;
                break;
            case 1:
                if (MemCard_LoadSpace_BigSave.m_Magic == 0x89ab001d) {
                    bs->SetStatus(1);
                    *bs = MemCard_LoadSpace_BigSave;
                    if ((signed char)bs->m_SaveFound[6] && bs->m_RitualTimes.m_Magic == 0xfedc000a) {
                        FEMainMenu *mm = FEMainMenu::Instance();
                        if (mm) {
                            unsigned int st = mm->m_pManager->m_PageState;
                            if ((st == 2 || st == 3 || st == 1) && (signed char)MemCard_LoadSpace_BigSave.m_SaveFound[6] &&
                                MemCard_LoadSpace_BigSave.m_RitualTimes.m_Magic == 0xfedc000a) {
                                bs->m_RitualTimes = MemCard_LoadSpace_BigSave.m_RitualTimes;
                                *(u8 *)&bs->m_SaveFound[6] = 1;
                                GameData::Instance(1)->m_RitualTimes = MemCard_LoadSpace_BigSave.m_RitualTimes;
                            }
                        }
                    } else {
                        bs->m_RitualTimes.Reset();
                        *(u8 *)&bs->m_SaveFound[6] = 0;
                    }
                    MemCard_SavesFoundBits = (unsigned char)bs->CalcSavesFound();
                    ret = MemCard_RetVal;
                    fail = false;
                    break;
                }
                MemCard_RetVal = -4;
                break;
            }
            if (fail) {
                bs->SetStatusAndReset(0);
                ret = -4;
                MemCard_RetVal = ret;
                MemCard_bAutoSave = 0;
                MemCard_SavesFoundBits = 0;
            }
        } else if (flow == 2) {
            int r = MemCard_CardChanged(s_bFirstCardCheck);
            s_bFirstCardCheck = 0;
            bool reset = false;
            switch (r) {
            case -1:
                bs->SetStatusAndReset(0);
                ret = MemCard_RetVal;
                if (r) MemCard_bBootCheck2 = 0;
                break;
            case 0:
                ret = MemCard_RetVal;
                break;
            case 1:
                if (!MemCard_bBootCheck2) {
                    MemCard_LoadSpace_BigSave = *bs;
                    if (GameData::Instance(1)->GetBigSaveStatus()->m_Status == 1) {
                        MemCard_SavesFoundBits = (unsigned char)bs->CalcSavesFound();
                        return 1;
                    }
                }
                reset = true;
                break;
            default:
                reset = true;
                break;
            }
            if (reset) {
                bs->SetStatusAndReset(0);
                MemCard_Flow = 5;
                ret = 0;
                MemCard_RetVal = 0;
                MemCard_LastTraceFlow = -999;
                MemCard_LastRetVal = -999;
                if (r) MemCard_bBootCheck2 = 0;
            }
        } else if (flow == 5) {
            bool notFormatted = false, bad = false;
            switch (ret) {
            case -9: MemCard_bNotFormatted = 1; goto done5;
            case -7: case -6: case -4: case -2:
                MemCard_RetVal = -2;
                goto done5;
            case 0:
                ret = GameData::Instance(1)->MemCard().SavesPresent(s_SavesMask, 0x58000);
                MemCard_RetVal = ret;
                if (ret != MemCard_LastRetVal) MemCard_LastRetVal = ret;
                break;
            case 1:
                if (s_SavesMask & 1) {
                    SetMemCardFlow(0x1f4);
                    ret = MemCard_RetVal;
                } else {
                    MemCard_RetVal = -2;
                    goto done5;
                }
                break;
            default:
            done5:
                if (bs->Status() == 1) bs->SetStatusAndReset(0);
                ret = MemCard_RetVal;
                MemCard_SavesFoundBits = 0;
                break;
            }
        }
    } while (prev == 0 && ret != 0);
    if (ret != MemCard_LastRetVal) MemCard_LastRetVal = ret;
    return ret;
}
