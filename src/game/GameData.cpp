#include "FEText.h"
#include "EXMemory.h"
#include "EXBaseDisplay.h"
#include "EXBaseGamePad.h"
#include "EXGamePad.h"
#include "FEWnd.h"
#include "FESound.h"
#include "XAppStandIn.h"
#include "FEHud.h"
#include "FELevels.h"
#include "XGamePad.h"
#include "EXBaseSoundManager.h"
#include "FEMainMenu.h"
#include "FEManager.h"
#include "FELayout.h"
#include "FEObject.h"
#include "FEListener.h"
#include "FEMathmos.h"
#include "XApp.h"
#include "FE.h"
#include <string.h>
extern "C" int __cdecl sprintf(char *, const char *, ...);

// TODO: GameDialogWnd::Update is not done yet.
char *HashToCharStr(unsigned int hash, char *pBuf);
extern "C" int __cdecl strncmp(const char *, const char *, unsigned int);

extern unsigned int gCurrentMap_GeoHash;
extern signed char MemCard_bLoading;

int GameControllerRemovedSys::m_Countdown = 8;
signed char GameControllerRemovedSys::m_bValid[4] = {1, 1, 1, 1};
unsigned int GameDialogWnd::m_NumInstances;
unsigned char GameDialogWnd::ms_LastPad;
int GameControllerRemovedSys::m_Controller;
GCRS::Flags GameControllerRemovedSys::m_Flags;
GameData *GameData::m_pInstance;
GameDialog GameControllerRemovedSys::m_Dialog;
static char s_DlgText[400];

EX_IMPLEMENT_DYNCREATE_ROOT(GameDialog)

GameData *GameData::Instance(signed char create)
{
    if (!m_pInstance && create) m_pInstance = new GameData;
    return m_pInstance;
}

signed char GameDialogWnd::Draw()
{
    if (GameControllerRemovedSys::Wnd() && this != GameControllerRemovedSys::Wnd()) return 1;
    return FEWnd::Draw();
}

void GameDialog::ShowDlg(const EXMemCardDlgData &d)
{
    ShowDlg(d, (unsigned int)-1, 0);
}

static unsigned int s_DefaultButtons[4] = {0x2000, 0x8000, 0x4000, 0x1000};

void GameDialog::ShowDlg(const EXMemCardDlgData &data, unsigned int sndMask, unsigned int *pButtons)
{
    EXMemCardDlgData &d = const_cast<EXMemCardDlgData &>(data);
    char buf[72];
    m_pWnd = new GameDialogWnd;
    m_pWnd->m_pOwner = this;
    if (!pButtons) pButtons = s_DefaultButtons;
    m_pButtonArray = pButtons;
    m_SndMask = sndMask;
    if (FEMainMenu::Instance() && FEMainMenu::Instance()->State() == 0xc && d.m_Message == 0x22010006)
        d.m_NumButtons = 2;
    m_pWnd->m_Result = 0;
    m_pWnd->m_pButtons = (void *)d.m_Type;
    m_pWnd->m_Unk404 = d.m_Selected;
    m_pWnd->m_Unk400 = d.m_NumButtons;
    if (m_pWnd->m_Unk404 >= m_pWnd->m_Unk400) m_pWnd->m_Unk404 = 0;
    char *msg = d.m_pText;
    if (d.m_Message != -1) {
        unsigned int h = ConvertMessageHashCode(d.m_Message);
        if (h == -1) return;
        if (h) {
            switch (h) {
            case 0x5100021b:
            case 0x5100021d:
                m_pWnd->m_Unk404 = 1;
                d.m_ButtonHash[0] = 0x5100021a;
                d.m_ButtonHash[1] = 0x51000217;
                d.m_pButton[0] = 0;
                d.m_pButton[1] = 0;
                break;
            case 0x5100030a:
                m_pWnd->m_Unk404 = 1;
                break;
            }
            msg = HashToCharStr(h, s_DlgText);
            if (d.m_Type == 0) {
                switch (h) {
                case 0x5100024e:
                case 0x5100021e:
                case 0x5100022b:
                case 0x51000237:
                case 0x510002f8:
                case 0x510002fb:
                case 0x51000328:
                    sprintf(msg, "%s\nPress Any button", msg);
                    break;
                }
            }
        }
    }
    int len = strlen(msg);
    m_pWnd->m_Origin = EXVector2((float)m_pWnd->m_Rect.w * 0.5f, (float)m_pWnd->m_Rect.h * 0.5f);
    if (len && d.m_NumButtons) {
        signed char bAny = 0;
        for (unsigned int i = 0; i < d.m_NumButtons; i++) {
            if (bAny) break;
            if (d.m_pButton[i] || d.m_ButtonHash[i]) bAny = 1;
        }
        if (bAny && d.m_Type && msg[len - 1] != '\n' && msg[len - 2] != '\n') {
            msg[len] = '\n';
            msg[len + 1] = '\n';
            msg[len + 2] = 0;
        }
    }
    memset(m_Unk10, 0, 400);
    char *src = msg;
    char *dst = (char *)m_Unk10;
    char *end = msg + strlen(msg) + 1;
    while (src < end) {
        if (*src == '(') {
            if (!strncmp(src, "(mcSLOT)", 8)) {
                strcpy(dst, d.m_pSlotName);
                dst += strlen(d.m_pSlotName);
                src += 8;
            } else if (!strncmp(src, "(mcNAME)", 8)) {
                strcpy(dst, "MyGame");
                dst += 6;
                src += 8;
            } else if (!strncmp(src, "(mcOK)", 6)) {
                strcpy(dst, "OK");
                dst += 2;
                src += 6;
            } else if (!strncmp(src, "(mcBLOCKS)", 10)) {
                sprintf(buf, "%d", d.m_MemRequired);
                strcpy(dst, buf);
                dst += strlen(buf);
                src += 10;
            } else {
                *dst = *src;
                dst++;
                src++;
            }
        } else {
            *dst = *src;
            dst++;
            src++;
        }
    }
    FE::_Init(m_pWnd->SetObject(new FELayoutLayered(2, 0, 0, 0, 0, (EXW::TextAlign)4)));
    FELayoutFlexGrid *pGrid = new FELayoutFlexGrid(d.m_NumButtons ? d.m_NumButtons + 2 : 1, 1, (EXW::TextAlign)4);
    FE::_Push(FE::_Peek()->ObjectAdd(1, pGrid));
    FE::_Peek()->ListenerAdd(new FEListener(0x5f5f5f5f5f76616e, d.m_NumButtons ? 0x401 : 0, 0, (FEPad::Flags)0xf));
    FE::_SetMarker(0);
    FEObject *pText = new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 0xfdffffff, EXString((char *)m_Unk10), FE_Blank,
                                   (EXW::TextAlign)3, 0, 0, 0, 0, (EXW::TextAlign)4);
    FE::_Push(FE::_Peek()->ObjectAdd(0, 0, pText)->LockTextRect(0x168, 0, 0, 0));
    FE::_Peek()->StateSub(8);
    FE::_Peek()->StateAdd(0x40000);
    FE::_Pop();
    FEListener *pDlgListener = FE::_Marker(0)->Listener(0x5f5f5f5f5f76616e);
    m_pListener = pDlgListener;
    FEListener::SetDialogListener(pDlgListener);
    if (d.m_NumButtons) {
        FEObject *pSpacer = new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 0xfdffffff, FE_Blank, FE_Blank,
                                         (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4);
        FE::_Push(FE::_Peek()->ObjectAdd(1, 0, pSpacer)->LockTextRect(0x168, 0, 0, 0));
        FE::_Peek()->StateSub(8);
        FE::_Pop();
        if (d.m_Type == 2 || d.m_Type == 1) {
            for (unsigned short i = 0; i < d.m_NumButtons; i++) {
                const char *&b = d.m_pButton[i];
                if (b) {
                    if (!strcmp(b, "RETRY")) b = "Retry";
                    else if (!strcmp(b, "CANCEL")) b = "Cancel";
                    else if (!strcmp(b, "YES")) b = "Yes";
                    else if (!strcmp(b, "NO")) b = "No";
                    else if (!strcmp(b, "OKAY")) b = "Okay";
                    else if (!strcmp(b, "CONTINUE")) b = "Continue";
                }
            }
        }
        if (d.m_ButtonHash[0] == 0x51000216 && (d.m_NumButtons == 1 || d.m_Type == 1)) d.m_ButtonHash[0] = 0x5100011a;
        if (d.m_Type == 2) {
            for (unsigned short i = 0; i < d.m_NumButtons; i++) {
                signed char bPress = 0;
                if (!d.m_pButton[i] && d.m_ButtonHash[i]) {
                    unsigned int h = ConvertMessageHashCode(d.m_ButtonHash[i]);
                    d.m_pButton[i] = HashToCharStr(h, buf);
                    if (h == 0x5100011a) bPress = 1;
                }
                if (d.m_pButton[i]) {
                    char *label = (char *)m_Unk10 + 400 + i * 0x40;
                    strcpy(label, d.m_pButton[i]);
                    ex::RGBA col;
                    if (bPress) col = ex::RGBA(0x7f, 0x7f, 0x7f, 0x80);
                    else col = ex::RGBA(0x7f, 0, 0, 0x80);
                    FEObject *pBtn = new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, (unsigned __int64)i, EXString(label),
                                                  FE_Blank, (EXW::TextAlign)3, 0, 0, 0, 0, (EXW::TextAlign)4);
                    FE::_Peek()->ObjectAdd(i + 2, 0, pBtn)->LockTextRect(0, 0, 0, 0)->SetMathmos(
                        FEObjColour::OC_FOCUS, FEMathmos(col, ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 60.0f, (FEWaveform::Flags)1, 0), 1);
                }
            }
            FE::_Marker(0)->Listener(0x5f5f5f5f5f76616e)->SetFocus(m_pWnd->m_pObject->Object(1)->Object(m_pWnd->m_Unk404 + 2, 0), 1);
        } else if (d.m_Type == 1) {
            unsigned int n = 0;
            for (unsigned short i = 0; i < d.m_NumButtons; i++) {
                if (d.m_pButton[i]) {
                    char *label = (char *)m_Unk10 + 400 + i * 0x40;
                    strcpy(label, d.m_pButton[i]);
                    FEObject *pBtn = new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, (unsigned __int64)i, EXString(label),
                                                  EXString(""), (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4);
                    FE::_Peek()->ObjectAdd(i + 2, 0, pBtn)->LockTextRect(0x168, 0, 0, 0);
                    n++;
                }
            }
            if (!n)
                FE::_Marker(0)->Listener(0x5f5f5f5f5f76616e)->SubListenFlags(0x401);
            else
                FE::_Marker(0)->Listener(0x5f5f5f5f5f76616e)->SetFocus(m_pWnd->m_pObject->Object(1)->Object(2, 0), 1);
        }
    }
    FE::_Pop();
    FEObject *pBg = new FEObject(FEObjType::OT_SPRITE, 0x1000025, 0x6000057, 0xfdffffff, FE_Blank, FE_Blank,
                                 (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4);
    FE::_Push(FE::_Peek()->ObjectAdd(0, pBg));
    FE::_Peek()->m_BorderWidth = 0x10;
    FE::_Peek()->SetRect(FE::_Marker(0)->Rect());
    FEObject *pObj = FE::_Peek();
    pObj->m_Rect.x -= 0x10;
    pObj->m_Rect.y -= 0x10;
    pObj->m_Rect.w += 0x20;
    pObj->m_Rect.h += 0x20;
    pObj->StateAdd(2);
    pObj->Resize();
    FE::_Pop();
    FE::_Pop();
    if (gCurrentMap_GeoHash != XApp::Frontend3DFile()) {
        if (theApp.m_bCutscene)
            FESound::FadeDown(1);
        else
            FESound::FadeDown(10);
    }
    m_Data = d;
}

signed char GameDialogWnd::Update()
{
    unsigned int snd = 0;
    if (!(State() & 0x40) || (GameControllerRemovedSys::Wnd() && this != GameControllerRemovedSys::Wnd()))
        return FEWnd::Update();
    if (m_Unk400) HandleInput();
    if (GameControllerRemovedSys::Wnd()) BringToTop();
    switch ((unsigned int)m_pButtons) {
    case 2: {
        if (GameData::Instance(1)->Transfer() == 1) {
            m_Result = 1;
            return FEWnd::Update();
        }
        FEListener *pL = m_pObject->Object(1)->Listener(0x5f5f5f5f5f76616e);
        FEObject *pFocus = pL->Focus();
        if (m_Result || !FEListener::QueryButton(0x2000, (FEPad::Flags)0xf, 1)) return FEWnd::Update();
        ms_LastPad = FEListener::LastPad();
        m_Result = (unsigned int)pFocus->m_Data + 1;
        EXString label(pFocus->m_Text);
        bool bBack = label == EXString("Cancel") || label == EXString("CANCEL") || label == EXString("No") ||
                     label == EXString("NO");
        snd = (unsigned int)!bBack * 4 + 0x1af00158;
        break;
    }
    case 1: {
        if (m_Result) return FEWnd::Update();
        unsigned int *pB = m_pOwner->m_pButtonArray;
        if (FEListener::QueryButton(pB[0] & 0xfffffff, (FEPad::Flags)0xf, 1) && m_Unk400) {
            ms_LastPad = FEListener::LastPad();
            m_Result = 1;
            snd = (~(pB[0] >> 0x1d) & 4) | 0x1af00158;
        }
        if (FEListener::QueryButton(pB[1] & 0xfffffff, (FEPad::Flags)0xf, 1) && m_Unk400 > 1) {
            ms_LastPad = FEListener::LastPad();
            m_Result = 2;
            snd = (~(pB[1] >> 0x1d) & 4) | 0x1af00158;
        }
        if (FEListener::QueryButton(pB[2] & 0xfffffff, (FEPad::Flags)0xf, 1) && m_Unk400 > 2) {
            ms_LastPad = FEListener::LastPad();
            m_Result = 3;
            snd = (~(pB[2] >> 0x1d) & 4) | 0x1af00158;
        }
        if (FEListener::QueryButton(pB[3] & 0xfffffff, (FEPad::Flags)0xf, 1) && m_Unk400 > 3) {
            ms_LastPad = FEListener::LastPad();
            m_Result = 4;
            snd = (~(pB[3] >> 0x1d) & 4) | 0x1af00158;
        }
        break;
    }
    default:
        return FEWnd::Update();
    }
    if (snd && (m_pOwner->m_SndMask & (1 << (m_Result & 0x1f)))) EXBaseSoundManager::GetSoundManager()->StartSound(snd);
    return FEWnd::Update();
}

void GameDialog::HideDlg()
{
    if (m_pWnd) {
        EXBaseDisplay::GetDisplay()->DelWnd(m_pWnd);
        if (gCurrentMap_GeoHash != XApp::Frontend3DFile()) FESound::FadeUp(10);
    }
    FEListener::SetDialogListener(0);
    m_pWnd = 0;
}

unsigned int GameDialog::DlgResult()
{
    if (m_pWnd) return m_pWnd->m_Result;
    return 5;
}

void GameDialog::HideDlg_NoDestroy()
{
    if (m_pWnd) m_pWnd->StateSub(0x40);
}

void GameDialog::ShowDlg_NoCreate()
{
    if (m_pWnd) {
        FEListener::SetDialogListener(m_pListener);
        m_pWnd->StateAdd(0x40);
        m_pWnd->BringToTop();
    }
}

unsigned int GameDialog::ConvertMessageHashCode(unsigned int hash)
{
    switch (hash) {
    case 0x22020001: return 0x51000214;
    case 0x22020002: return 0x51000215;
    case 0x22020004: return 0x510002e9;
    case 0x22020005: return 0x510002ea;
    case 0x22020006: return 0x510002e7;
    case 0x22020009: return 0x51000240;
    case 0x22020003: return 0x51000820;
    case 0x2202000a: return 0x5100024a;
    case 0x22020008: return 0x51000237;
    case 0x22020007: return MemCard_bLoading ? 0x51000825 : 0x510002e6;
    case 0x5100023a: return 0x51000328;
    }
    return hash;
}

void GearCarryOver::WeaponArrayRestore()
{
    if (m_bValid) {
        if (m_Ammo[0] > 0) FEHud::Instance(1)->WeaponArrayAdd(8, (FEWeaponCat::Flags)4);
        if (m_Ammo[1] > 0) FEHud::Instance(1)->WeaponArrayAdd(9, (FEWeaponCat::Flags)4);
        if (m_Ammo[2] > 0) FEHud::Instance(1)->WeaponArrayAdd(10, (FEWeaponCat::Flags)4);
        if (m_Ammo[3] > 0) FEHud::Instance(1)->WeaponArrayAdd(11, (FEWeaponCat::Flags)4);
        if (m_Ammo[4] > 0) FEHud::Instance(1)->WeaponArrayAdd(5, (FEWeaponCat::Flags)4);
        if (m_Ammo[5] > 0) FEHud::Instance(1)->WeaponArrayAdd(12, (FEWeaponCat::Flags)4);
    }
}

void GameData::SavePost()
{
    SettingsInvalidateGlobal();
    SettingsInvalidateLevel();
}

void GameData::LoadPre()
{
}

void BigSaveStatus::SetStatus(unsigned int s)
{
    m_Status = s;
}

// NEAR: VC keeps 0 in a spare register (ebx/esi pushed) where orig uses immediates.
void GameControllerRemovedSys::UpdateControllers()
{
    for (int i = 0; i < 4; i++) {
        if (i < 4)
            m_bValid[i] = ((EXGamePad *)EXBaseGamePad::GetGamePad())->PadAttached(i) != 0;
        else
            m_bValid[i] = 0;
    }
}

signed char GameControllerRemovedSys::ReconnectCheck()
{
    XGameState state;
    UpdateControllers();
    if (m_bValid[m_Controller] && (m_Flags == 0 || EXBaseGamePad::GetGamePad()->ButtonReleased((u8)m_Controller, 0x40000))) {
        EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00159);
        if (theApp.GameStateCurrent()->m_State == 0x16) {
            theApp.m_GameStates.Pop(state);
            theApp.m_bGameStateChanged = 1;
        }
        m_Dialog.HideDlg();
        theApp.SetDialogOpen(0);
        return 1;
    }
    return 0;
}

signed char GameControllerRemovedSys::DisconnectCheck(GCRS::Flags flags)
{
    EXMemCardDlgData data;
    char text[0x140];
    UpdateControllers();
    if (!m_bValid[m_Controller]) {
        switch (flags) {
        case 2:
            if (!theApp.GetCutscene() && theApp.GameStateCounter() < 10) return 0;
        case 3:
        case 4:
        case 5:
            EXBaseSoundManager::GetSoundManager()->StartSound(0x1af0015b);
            sprintf(text, "Please reconnect the controller to controller port %d and press START to continue.", m_Controller + 1);
        default:
            m_Flags = flags;
            if (gCurrentMap_GeoHash != XApp::Frontend3DFile()) theApp.GameStatePush((GameState::Flags)0x16, 0);
            {
                GameDialog *pDlg = &m_Dialog;
                memset(&data, 0, sizeof(data));
                data.m_Message = (unsigned int)-1;
                data.m_pText = text;
                pDlg->ShowDlg(data, (unsigned int)-1, 0);
            }
            theApp.SetDialogOpen(1);
            return 1;
        case 0:
        case 1:
            return 1;
        }
    }
    return 0;
}

GameData::GameData()
{
    m_Transfer = GDTransfer::GDT_NONE;
    m_SavesMask = 0;
    m_Slot = 0;
    m_SaveInfo.m_Version = 1;
    m_SaveInfo.m_Size = sizeof(BigSave);
    strcpy(m_SaveInfo.m_Name, "Predator");
    strcpy(m_SaveInfo.m_Folder, "Saved Games");
    m_MemCard.Init(m_SaveInfo);
    m_MemCard.SetMemCardDlgClass((EXRuntimeClass *)EXRUNTIME_CLASS(GameDialog));
}

// NEAR: only the rep movsd setup order differs (orig loads the count before the destination).
void GameData::SettingsStore()
{
    m_Values.m_bLevelValid = 0;
    m_Values.m_bGlobalValid = 1;
    m_Values.m_Unkc = gCurrentMap_GeoHash;
    m_Values.m_bInvertX3rd = XGetCamera(0)->GetInvertX();
    m_Values.m_bInvertY3rd = XGetCamera(0)->GetInvertY();
    m_Values.m_bInvertX1st = XGetCamera(0)->GetInvertFPX();
    m_Values.m_bInvertY1st = XGetCamera(0)->GetInvertFPY();
    m_Values.m_FirstPersonView = XGetCamera(0)->GetFPFace();
    m_Values.m_MusicVolume = EXBaseSoundManager::GetSoundManager()->MusicVolume();
    m_Values.m_MovieVolume = EXBaseSoundManager::GetSoundManager()->SfxVolume();
    m_Values.m_ControllerConfig = ((XGamePad *)EXBaseGamePad::GetGamePad())->GetActions();
    m_Values.m_Unk1c[3] = EXBaseGamePad::GetGamePad()->RumbleEnabled(GameControllerRemovedSys::Controller());
    m_Values.m_Unk1c[1] = (s8)EXBaseSoundManager::GetSoundManager()->GetMode();
    m_Values.m_Unk1c[7] = EXBaseDisplay::GetDisplay()->VSyncEnabled();
    m_Values.m_Unk5c = CameraDistance;
    if (XItemHandler_Player::Instance()) {
        if (!theApp.IsPaused()) {
            m_Values.m_Unk58 = XItemHandler_Player::Instance()->m_Unkc8;
            FELevelManager *pMgr = FELevelManager::Instance(1);
            int idx = pMgr->IndexFromLevel(gCurrentMap_GeoHash);
            if (idx == 0 || (idx > 12 && idx <= 15)) {
                m_Values.m_Unk24[4] = XItemHandler_Player::Instance()->GetSpecialUpgradeArray()[4];
            } else {
                memcpy(m_Values.m_Unk24, XItemHandler_Player::Instance()->GetSpecialUpgradeArray(), sizeof(m_Values.m_Unk24));
                memcpy(m_Values.m_Unk38, XItemHandler_Player::Instance()->m_Unkcf0, sizeof(m_Values.m_Unk38));
            }
        }
        if (XItemHandler_Player::Instance() && !theApp.IsPaused()) {
            m_Values.m_bLevelValid = 1;
            m_Values.m_Unk64[0] = XItemHandler_Player::Instance()->m_Health;
            m_Values.m_Unk64[1] = XItemHandler_Player::Instance()->m_Energy;
            m_Values.m_Unk64[2] = 0.0f;
            m_Values.m_Unk70[0] = XItemHandler_Player::Instance()->m_CurrentWeapon[0];
            m_Values.m_Unk70[1] = XItemHandler_Player::Instance()->m_CurrentWeapon[1];
            m_Values.m_Unk70[2] = XItemHandler_Player::Instance()->m_CurrentWeapon[2];
            m_Values.m_Unk70[3] = XItemHandler_Player::Instance()->m_CurrentWeapon[3];
            for (int i = 0; i < 14; i++) ((short *)m_Values.m_Unk80)[i] = XItemHandler_Player::Instance()->m_Ammo[i];
        }
    }
}

// NEAR: bFull and the selected valid flag land in dl/bl swapped (orig keeps bFull in dl, copies via si).
inline void GearCarryOver::Restore(signed char bFull)
{
    XItemHandler_Player *p = XItemHandler_Player::Instance();
    if (!p) return;
    signed char valid = bFull ? m_bValidFull : m_bValid;
    if (valid) {
        p->SetAmmo(8, m_Ammo[0]);
        p->SetAmmo(9, m_Ammo[1]);
        p->SetAmmo(10, m_Ammo[2]);
        p->SetAmmo(11, m_Ammo[3]);
        p->SetAmmo(5, m_Ammo[4]);
        p->SetAmmo(12, m_Ammo[5]);
        if (bFull) {
            p->m_Health = m_Health;
            p->m_Energy = m_Energy;
        }
    }
}

// NEAR: only the rep movsd setup order differs (as in SettingsStore).
void GameData::SettingsRestore(signed char bLevelOnly)
{
    if (m_Values.m_bGlobalValid) {
        if (!bLevelOnly) {
            XItemHandler_Camera *pCam = XGetCamera(0);
            pCam->SetInvertX(m_Values.m_bInvertX3rd);
            pCam = XGetCamera(0);
            pCam->SetInvertY(m_Values.m_bInvertY3rd);
            pCam = XGetCamera(0);
            pCam->SetInvertFPX(m_Values.m_bInvertX1st);
            pCam = XGetCamera(0);
            pCam->SetInvertFPY(m_Values.m_bInvertY1st);
            pCam = XGetCamera(0);
            pCam->SetFPFace(m_Values.m_FirstPersonView);
            EXBaseSoundManager::GetSoundManager()->SetMusicVolume(m_Values.m_MusicVolume);
            EXBaseSoundManager::GetSoundManager()->SetSfxVolume(m_Values.m_MovieVolume);
            ((XGamePad *)EXBaseGamePad::GetGamePad())->SetActions(m_Values.m_ControllerConfig);
            EXBaseGamePad::GetGamePad()->EnableAllRumble(m_Values.m_Unk1c[3]);
            EXBaseDisplay::GetDisplay()->EnableVSync(m_Values.m_Unk1c[7]);
            CameraDistance = m_Values.m_Unk5c;
        }
        if (XItemHandler_Player::Instance() && !theApp.IsPaused()) {
            XItemHandler_Player::Instance()->m_Unkc8 = m_Values.m_Unk58;
            switch (gCurrentMap_GeoHash) {
            case 0x1000056:
            case 0x100006a:
            case 0x1000070:
            case 0x100016f:
            case 0x1000170:
                XItemHandler_Player::Instance()->SetSpecialUpgrade(4, m_Values.m_Unk24[4]);
                break;
            default:
                memcpy(XItemHandler_Player::Instance()->GetSpecialUpgradeArray(), m_Values.m_Unk24, sizeof(m_Values.m_Unk24));
                memcpy(XItemHandler_Player::Instance()->m_Unkcf0, m_Values.m_Unk38, sizeof(m_Values.m_Unk38));
                break;
            }
            if (bLevelOnly) {
                FELevelManager *pMgr = FELevelManager::Instance(1);
                int idx = pMgr->IndexFromLevel(gCurrentMap_GeoHash);
                if (idx >= 1 && idx < (int)FELevelManager::Instance(1)->m_Unk4) {
                    XGetGameValues()->m_LevelRecords[idx - 1].Restore(0);
                } else {
                    int rec;
                    switch (gCurrentMap_GeoHash) {
                    case 0x1000186: rec = 0xc; break;
                    case 0x1000185: rec = 0x16; break;
                    case 0x1000196:
                        rec = 0xa;
                        XGetGameValues()->m_LevelRecords[10].m_bValidFull = 1;
                        break;
                    case 0x1000163: rec = 0x11; break;
                    default: goto done;
                    }
                    XGetGameValues()->m_LevelRecords[rec].Restore(1);
                }
            }
        }
    }
done:
    if (!theApp.IsPaused()) m_Stats.Reset();
    if (!bLevelOnly && m_Values.m_bLevelValid && XItemHandler_Player::Instance() && !theApp.IsPaused()) {
        XItemHandler_Player::Instance()->m_Health = m_Values.m_Unk64[0];
        XItemHandler_Player::Instance()->m_Energy = m_Values.m_Unk64[1];
        for (int i = 0; i < 4; i++) XItemHandler_Player::Instance()->SetCurrentWeapon(i, m_Values.m_Unk70[i]);
        for (int j = 0; j < 14; j++) XItemHandler_Player::Instance()->m_Ammo[j] = ((short *)m_Values.m_Unk80)[j];
    }
}

void GameData::SavePre()
{
    SettingsStore();
}

unsigned int BigSave::Status()
{
    return GameData::Instance(1)->GetBigSaveStatus()->Status();
}

void BigSave::SetStatus(unsigned int s)
{
    GameData::Instance(1)->GetBigSaveStatus()->SetStatus(s);
}

void BigSave::SetStatusAndReset(unsigned int s)
{
    memset(this, 0, sizeof(*this));
    m_Magic = 0x89ab001d;
    GameData::Instance(1)->GetBigSaveStatus()->SetStatus(s);
}

void GameData::LoadPost()
{
    if (theApp.GameStateCurrent()->m_State != 2) SettingsRestore(0);
}

signed char GameData::DataTransfer()
{
    int r = 1;
    switch (m_Transfer) {
    case GDTransfer::GDT_CHECKPRESENT: r = m_MemCard.SavesPresent(m_SavesMask, 0); break;
    case GDTransfer::GDT_CHECKSAVE: r = m_MemCard.CheckSavePossible(0); break;
    case GDTransfer::GDT_SAVE: r = m_MemCard.Save(m_Slot, &m_Values, 0); break;
    case GDTransfer::GDT_LOAD: r = m_MemCard.Load(m_Slot, &m_Values, 0, 0); break;
    }
    m_Result = r;
    if (r != 0 && m_Transfer != GDTransfer::GDT_NONE) {
        switch (m_Transfer) {
        case GDTransfer::GDT_CHECKPRESENT:
            if (r == 1) {
                m_Transfer = ((u8)(1 << m_Slot) & m_SavesMask) ? GDTransfer::GDT_LOAD : GDTransfer::GDT_NONE;
                break;
            }
            m_Transfer = GDTransfer::GDT_NONE;
            break;
        case GDTransfer::GDT_CHECKSAVE:
            m_Transfer = r == 1 ? GDTransfer::GDT_SAVE : GDTransfer::GDT_NONE;
            break;
        case GDTransfer::GDT_SAVE:
            if (r == 1) SavePost();
            m_Transfer = GDTransfer::GDT_NONE;
            break;
        case GDTransfer::GDT_LOAD:
            if (r == 1) LoadPost();
            m_Transfer = GDTransfer::GDT_NONE;
            break;
        default:
            m_Transfer = GDTransfer::GDT_NONE;
            break;
        }
    }
    return m_Transfer != GDTransfer::GDT_NONE;
}

void GameData::Save(unsigned char slot)
{
    if (m_Transfer == GDTransfer::GDT_NONE) {
        m_Slot = slot;
        SettingsStore();
        m_Transfer = GDTransfer::GDT_CHECKSAVE;
        DataTransfer();
    }
}

void GameData::Load(unsigned char slot)
{
    if (m_Transfer == GDTransfer::GDT_NONE) {
        m_Slot = slot;
        m_Transfer = GDTransfer::GDT_CHECKPRESENT;
        DataTransfer();
    }
}
