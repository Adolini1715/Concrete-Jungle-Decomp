#include "FEText.h"
#include "EXMemory.h"
#include "FEPause.h"
#include "FEHashCodes.h"
#include "FEObject.h"
#include "FEListener.h"
#include "FELevels.h"
#include "FEHud.h"
#include "GameData.h"
#include "XItemHandler_Player.h"
#include "EXBaseSoundManager.h"
#include "EXSoundTagStandIn.h"
#include "FEMainMenu.h"
#include "XApp.h"
#include "FE.h"
#include "FELayout.h"
#include "FEObjectScript.h"
#include "EXApp.h"

extern unsigned int gCurrentMap_GeoHash;
extern unsigned int gPrevMap_GeoHash;

FEObjectives *FEObjectives::m_pInstance;
FEPause *FEPause::m_pInstance;

void FEObjectives::Initialise()
{
    for (unsigned int i = 0; i < (unsigned int)m_nCount; i++) {
        m_Objectives[i].m_Short.Empty();
        m_Objectives[i].m_Msg.Empty();
    }
    memset(m_Objectives, 0, sizeof(m_Objectives));
    m_nCount = 0;
}

void FEObjectives::_Add(unsigned int shortHash, EXString shortMsg, unsigned int msgHash, EXString msg, int type)
{
    if ((unsigned int)m_nCount < 32) {
        if (shortHash || shortMsg.Length()) {
            if (type & 0x28) {
                FELevelManager *pMgr = FELevelManager::Instance(1);
                if (pMgr->LevelFromHash(gCurrentMap_GeoHash, 0)->m_Type == 1) type = 1;
            }
            if (shortHash)
                m_Objectives[m_nCount].m_ShortHash = shortHash;
            else
                m_Objectives[m_nCount].m_Short = shortMsg;
            if (msgHash)
                m_Objectives[m_nCount].m_MsgHash = msgHash;
            else
                m_Objectives[m_nCount].m_Msg = msg;
            m_Objectives[m_nCount].m_Type = type;
            m_Objectives[m_nCount].m_bRead = 0;
            m_Objectives[m_nCount].m_bComplete = 0;
            m_nCount++;
            return;
        }
    }
}

int FEObjectives::MsgType(int i)
{
    if (i >= m_nCount) return 0x40;
    return m_Objectives[i].m_Type;
}

signed char FEObjectives::IsRead(int i)
{
    if (i >= m_nCount) return 0;
    return m_Objectives[i].m_bRead;
}

void FEObjectives::MarkRead(int i)
{
    if (i < m_nCount) m_Objectives[i].m_bRead = 1;
}

signed char FEObjectives::IsComplete(int i)
{
    if (i >= m_nCount) return 0;
    return m_Objectives[i].m_bComplete;
}

void FEObjectives::MarkComplete(unsigned int hash)
{
    for (int i = 0; i < m_nCount; i++) {
        if ((m_Objectives[i].m_ShortHash == hash || m_Objectives[i].m_MsgHash == hash) &&
            !m_Objectives[i].m_bComplete) {
            m_Objectives[i].m_bComplete = 1;
            return;
        }
    }
}

signed char FEObjectives::NewObjective(int type)
{
    for (int i = 0; i < m_nCount; i++) {
        if ((m_Objectives[i].m_Type & type) && !m_Objectives[i].m_bRead) return 1;
    }
    return 0;
}

signed char Pause_Controller(FEObject *pObj, FEObjEvent::Flags ev)
{
    if (ev != 0x13) return 0;
    pObj->Listener(0x5f5f5f5f5f76616eull)->RestoreDefaultFocus();
    return 1;
}

struct ControllerLabel {
    unsigned int m_Action;
    EXW::TextAlign m_Align;
};
static ControllerLabel s_ControllerLabels[18] = {
    {0x100000d7, (EXW::TextAlign)2}, {0x100000d8, (EXW::TextAlign)2}, {0x100000d9, (EXW::TextAlign)2},
    {0x100000da, (EXW::TextAlign)2}, {0x100000db, (EXW::TextAlign)7}, {0x100000dc, (EXW::TextAlign)2},
    {0x100000dd, (EXW::TextAlign)7}, {0x100000de, (EXW::TextAlign)2}, {0x100000df, (EXW::TextAlign)2},
    {0x100000e0, (EXW::TextAlign)2}, {0x100000e1, (EXW::TextAlign)7}, {0x100000e2, (EXW::TextAlign)7},
    {0x100000e3, (EXW::TextAlign)5}, {0x100000e4, (EXW::TextAlign)7}, {0x100000e5, (EXW::TextAlign)7},
    {0x100000e6, (EXW::TextAlign)7}, {0x100000e7, (EXW::TextAlign)7}, {0x100000e8, (EXW::TextAlign)2},
};

// NEAR: retail loads the table entry fields in a different order.
signed char Pause_DrawController(FEObject *pObj, FEObjEvent::Flags ev)
{
    if (ev != 0x18) return 0;
    if (pObj->m_Data == 0x5f63696870617267ull) {
        int cfg = GameData::Instance(1)->m_Values.m_ControllerConfig;
        ((XGamePad *)EXBaseGamePad::GetGamePad())->SetActions(cfg);
        for (const ControllerLabel *l = s_ControllerLabels; (int)l < (int)(s_ControllerLabels + 18); l++)
            pObj->DrawText((FERenderPass::Flags)2, 0x100003a, 0x700000a, ex::RGBA(0x5f, 0x5f, 0x5f, 0x80), 0,
                           l->m_Action,
                           ((XGamePad *)EXBaseGamePad::GetGamePad())->GetControllerString(l->m_Action),
                           l->m_Align, 0);
    }
    return 1;
}

__forceinline FEPause *FEPause::Instance(signed char create)
{
    if (!m_pInstance && create) {
        m_pInstance = new FEPause;
        FEWnd::FixupZOrder();
    }
    return m_pInstance;
}

void FEPause::ContentUpdate()
{
    m_Mathmos3.Update();
    m_Mathmos.Update();
    m_Mathmos2.Update();
    FEWnd::ContentUpdate();
}

__forceinline FEObjectives *FEObjectives::Instance(signed char create)
{
    if (!m_pInstance && create) m_pInstance = new FEObjectives;
    return m_pInstance;
}

void FEObjectives::Add(EXString msg, int type) { _Add(0, msg, 0, msg, type); }

void FEObjectives::Add(EXString shortMsg, EXString msg, int type) { _Add(0, shortMsg, 0, msg, type); }

void FEObjectives::Add(unsigned int hash, int type) { _Add(hash, FE_Blank, hash, FE_Blank, type); }

void FEObjectives::Add(unsigned int shortHash, unsigned int msgHash, int type)
{
    _Add(shortHash, FE_Blank, msgHash, FE_Blank, type);
}

EXString FEObjectives::Msg(int i)
{
    if (i >= m_nCount) return gText.m_Empty;
    FEObjective &o = m_Objectives[i];
    if (o.m_MsgHash) return gText.Get(o.m_MsgHash);
    return o.m_Msg;
}

EXString FEObjectives::ShortMsg(int i)
{
    if (i >= m_nCount) return gText.m_Empty;
    FEObjective &o = m_Objectives[i];
    if (o.m_ShortHash) return gText.Get(o.m_ShortHash);
    return o.m_Short;
}

signed char Pause_ExitStats(FEObject *pObj, FEObjEvent::Flags ev)
{
    if (ev != 0x18) return 0;
    if (pObj->m_Data == 0x5f5f5f7374617473ull) {
        if ((FEPause::Instance(1)->Object()->Object(2)->m_Flags & 0x40) && !GameControllerRemovedSys::Wnd()) {
            if (!FEListener::QueryButton(0x8000, (FEPad::Flags)0xf, 1) && !FEListener::QueryButton(0x80000, (FEPad::Flags)0xf, 1)) return 1;
            EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00159);
            FEPause::Instance(1)->Object()->Object(1)->StateAdd(0x10000);
            FEPause::Instance(1)->Object()->Object(2)->StateSub(0x10040);
        }
    }
    return 1;
}

#pragma auto_inline(off)
signed char Pause_ViewWeapon(FEObject *pObj, FEObjEvent::Flags ev)
{
    static EXString s_Desc;
    if (pObj) {
    if (ev == 9) {
      if (pObj->m_Data != 0xfdffffffull) {
        FEWeapon w = FEHud::Instance(1)->GetWeaponPreset((unsigned int)pObj->m_Data, 0, 0);
        FEObject *pPage = FEPause::Instance(1)->Object()->Object(1)->Object(2, 0)->Page(0x70616577736e6f5full, 0);
        int lvl;
        if (w.m_Hash < 1000) {
            if ((int)w.m_Hash < 0 || (int)w.m_Hash >= 8) lvl = 0;
            else lvl = XItemHandler_Player::Instance()->m_Unkcf0[w.m_Hash];
            lvl++;
        } else if (w.m_Hash - 1000 == 2) {
            lvl = XItemHandler_Player::Instance()->GetSpecialUpgrade(w.m_Hash - 1000);
        } else {
            lvl = XItemHandler_Player::Instance()->GetSpecialUpgrade(w.m_Hash - 1000) + 1;
        }
        pPage->Object(0, 0)->Object(0)->Object(1, 0)->Object(0, 0)->Object(0, 0)->SetTexture(HT_File_HUD, w.m_Icon);
        FEObject *pName = pPage->Object(0, 0)->Object(0)->Object(1, 0)->Object(1, 0);
        pName->Object(0)->SetText(gText.Get(w.m_Text), 0, 0);
        s_Desc.Empty();
        if (w.m_Unk25 && lvl > 0) {
            s_Desc.Format(Text_Get(0x51000606), lvl);
        }
        pPage->Object(0, 0)->Object(0)->Object(1, 0)->Object(0, 0)->Object(1, 0)->SetText(s_Desc, 0, 0);
        if (w.m_Unk14 && lvl > 1) {
            FEObject *p2 = pPage->Object(0, 0)->Object(0)->Object(1, 0)->Object(1, 0);
            p2->Object(0)->AddText("\n\n" + Text_Get(w.m_Unk14), 0, 0);
        }
        if (w.m_Unk18 && lvl > 2) {
            FEObject *p3 = pPage->Object(0, 0)->Object(0)->Object(1, 0)->Object(1, 0);
            p3->Object(0)->AddText("\n\n" + Text_Get(w.m_Unk18), 0, 0);
        }
      }
      return 1;
    }
    if (ev == 0x13) {
        FEListener *pL = pObj->Listener(0x5f5f5f5f5f76616eull);
        pL->SetFocus(pL->m_pDefaultFocus, 0);
        return 1;
    }
    }
    return 0;
}
#pragma auto_inline(on)

signed char Pause_ChangeVolume(FEObject *pObj, FEObjEvent::Flags ev)
{
    static EXSoundTag s_Tag;
    switch (ev) {
    case 0x10:
        if (!(pObj->m_Flags & 4)) return 1;
    case 9:
        if (XApp::Frontend3DFile() != gCurrentMap_GeoHash && pObj->m_Data == 0x5f5f5f5f5f786673ull) {
            if (!s_Tag.IsPlaying()) s_Tag.StartSound(0x1af004f9);
        }
        if (XApp::Frontend3DFile() == gCurrentMap_GeoHash && FEMainMenu::Instance())
            FEMainMenu::Instance()->m_pHelpText->SetText(pObj->m_HelpText, 0, 0);
        return 1;
    case 10:
    case 0x11:
        if (pObj->m_Data == 0x5f5f5f5f5f786673ull) {
            if (s_Tag.IsPlaying()) s_Tag.RemoveAll(1);
        }
        return 1;
    case 0x19:
        break;
    default:
        return 0;
    }
    int v = pObj->Value();
    if (v > 0 && v < 100) v += 1 - (int)((100 - v) * -0.1f);
    if (pObj->m_Data == 0x5f5f5f5f5f786673ull) {
        GameData::Instance(1)->m_Values.m_MovieVolume = v;
        EXBaseSoundManager::GetSoundManager()->SetSfxVolume(v);
        return 1;
    }
    if (pObj->m_Data == 0x5f5f5f636973756dull) {
        XGetGameValues()->m_MusicVolume = v;
        EXBaseSoundManager::GetSoundManager()->SetMusicVolume(v);
    }
    return 1;
}

signed char Pause_Options(FEObject *pObj, FEObjEvent::Flags ev)
{
    static EXSoundTag s_Tag;
    switch (ev) {
    case 0x13:
        pObj->Listener(0x5f5f5f5f5f76616eull)->RestoreDefaultFocus();
        return 1;
    case 0x10:
        if (!(pObj->m_Flags & 4)) return 1;
    case 9:
        if (pObj->m_Data == 0x5f5f5f5f5f786673ull) {
            if (!s_Tag.IsPlaying()) s_Tag.StartSound(0x1af004f9);
        }
        if (gCurrentMap_GeoHash == XApp::Frontend3DFile()) {
            if (pObj->m_Data == 0x5f5f5f636973756dull) {
                EXBaseSoundManager::GetSoundManager()->FadeMusicUp(1);
                EXBaseSoundManager::GetSoundManager()->StartMusic(0x1b000022, 1, 1, 0, 1);
            }
            if (FEMainMenu::Instance()) FEMainMenu::Instance()->NewFocus(pObj, 0x10);
        }
        return 1;
    case 10:
    case 0x11:
        if (pObj->m_Data == 0x5f5f5f5f5f786673ull) {
            if (s_Tag.IsPlaying()) s_Tag.RemoveAll(1);
        }
        if (gCurrentMap_GeoHash == XApp::Frontend3DFile() && pObj->m_Data == 0x5f5f5f636973756dull)
            EXBaseSoundManager::GetSoundManager()->FadeMusicDown(1, 0);
        return 1;
    case 0x19:
        break;
    default:
        return 0;
    }
    int v = pObj->Value();
    if (v > 0 && v < 100) v += 1 - (int)((100 - v) * -0.1f);
    if (pObj->m_Data == 0x5f5f5f5f5f786673ull) {
        EXBaseSoundManager::GetSoundManager()->SetSfxVolume(v);
        return 1;
    }
    if (pObj->m_Data == 0x5f5f5f636973756dull) EXBaseSoundManager::GetSoundManager()->SetMusicVolume(v);
    return 1;
}

signed char Pause_ViewHint(FEObject *pObj, FEObjEvent::Flags ev)
{
    if (!pObj || (ev != 9 && ev != 0x10) || pObj->m_Data == 0xfdffffffull) return 0;
    int idx = (int)pObj->m_Data;
    pObj->SetText(FEObjectives::Instance(1)->ShortMsg(idx), 0, 0);
    if (pObj->m_Flags & 4) {
        FEObject *pText = FEPause::Instance(1)->Object()->Object(1)->Object(2, 0)->Page(0x5f5f5f73746e6968ull, 0)->Object(0, 0)->Object(0);
        pText->Object(1, 0)->SetText(FEObjectives::Instance(1)->Msg((int)pObj->m_Data), 0, 0);
        FEObjectives *po = FEObjectives::Instance(1);
        if ((int)pObj->m_Data < po->m_nCount) po->m_Objectives[(int)pObj->m_Data].m_bRead = 1;
        pObj->SetRemoteMathmos((FEObjColour::Flags)1, &FEPause::Instance(1)->m_Mathmos, 1);
        Text_DeleteUnique(8);
    }
    return 1;
}

signed char Pause_ViewObjective(FEObject *pObj, FEObjEvent::Flags ev)
{
    if (!pObj || (ev != 9 && ev != 0x10) || pObj->m_Data == 0xfdffffffull) return 0;
    FEObjectives *po = FEObjectives::Instance(1);
    int idx = (int)pObj->m_Data;
    const char *mark = (idx < po->m_nCount && po->m_Objectives[idx].m_bComplete) ? "~& " : "~' ";
    pObj->SetText(EXString(mark), 0, 0);
    int d2 = (int)pObj->m_Data;
    pObj->SetText(pObj->m_Text + FEObjectives::Instance(1)->ShortMsg(d2), 0, 0);
    if (pObj->m_Flags & 4) {
        FEObject *pText = FEPause::Instance(1)->Object()->Object(1)->Object(2, 0)->Page(0x5f5f5f5f5f6a626full, 0)->Object(0, 0)->Object(0);
        pText->Object(1, 0)->SetText(FEObjectives::Instance(1)->Msg((int)pObj->m_Data), 0, 0);
        FEObjectives *po = FEObjectives::Instance(1);
        if ((int)pObj->m_Data < po->m_nCount) po->m_Objectives[(int)pObj->m_Data].m_bRead = 1;
        pObj->SetRemoteMathmos((FEObjColour::Flags)1, &FEPause::Instance(1)->m_Mathmos, 1);
    }
    return 1;
}

signed char Pause_SelectMap(FEObject *pObj, FEObjEvent::Flags ev)
{
    switch (ev) {
    case 0x1a: {
        EXString t = gText.Get(0x51000838);
        if (!(t == pObj->m_Text)) {
            if ((unsigned int)pObj->m_Data == 0xabbaabbau) {
                EXBaseSoundManager::GetSoundManager()->StartSound(0x1af0015b);
                FEPause::Instance(1)->Object()->Object(1)->StateSub(0x10000);
                FEPause::Instance(1)->Object()->Object(2)->StateAdd(0x10040);
            } else {
                theApp.GameStatePush((GameState::Flags)0x1b, (unsigned int)pObj->m_Data);
                return 1;
            }
        }
        return 1;
    }
    case 0x13:
        pObj->Listener(0x5f5f5f5f5f76616eull)->RestoreDefaultFocus();
        return 1;
    default:
        return 0;
    }
}


void FEPause::ContentDestroy()
{
    if (m_ContentDestroyed) return;
    GameData::Instance(1)->SettingsRestore(0);
    if (FEHud::Instance(0)) {
        FEHud::Instance(1)->m_DisablePlayer = 10;
        XItemHandler_Player::Instance()->m_ButtonLock = 0xffffff5f;
        FEHud *pHud = FEHud::Instance(1);
        pHud->m_StateStack.Pop(pHud->m_State);
    }
    EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00159);
    EXBaseSoundManager::GetSoundManager()->SetSfxVolume(GameData::Instance(1)->Values()->m_MovieVolume);
    FEListener::RemapModeSet((FERemap::Flags)0);
    m_DeleteNeeded = 1;
    m_ContentDestroyed = 1;
}


// Level upgrade table (FEMainMenu.obj).
unsigned int GetLevelUpgrade_BonusTasks(unsigned int level);
unsigned int GetLevelUpgrade_WeaponUpgrades(unsigned int level);
unsigned int GetLevelUpgrade_WeaponUpgradesBaseIndex(unsigned int level);
unsigned int GetLevelUpgrade_AttributeUpgrades(unsigned int level);
unsigned int GetLevelUpgrade_Costumes(unsigned int level);
unsigned int GetLevelUpgrade_RewardIndex(unsigned int level);

// One "label  value" row of the level stats list: adds the row and its label, then pushes the (empty) value
// text so the caller can fill it in. The caller pops twice.
static FEObject *Pause_StatRow(int row, const EXString &label)
{
    FE::_Push(FE::_Peek()->ObjectAdd(row, 0, new FELayoutFlexGrid(1, 2, (EXW::TextAlign)4)));
    FE::_Peek()->m_Flags |= 8;
    FE::_Push(FE::_Peek()->ObjectAdd(0, 0, new FEObject((FEObjType::Flags)2, 0x100003a, 0x7000004, 0xfdffffff, label, FE_Blank,
                                                        (EXW::TextAlign)7, 0, 0, 0x145, 0x1e, (EXW::TextAlign)4)));
    FE::_Pop();
    FE::_Push(FE::_Peek()->ObjectAdd(0, 1, new FEObject((FEObjType::Flags)2, 0x100003a, 0x7000004, 0xfdffffff, FE_Blank, FE_Blank,
                                                        (EXW::TextAlign)7, 0, 0, 0x4b, 0x1e, (EXW::TextAlign)4)));
    return FE::_Peek();
}

// An integer stat row.
static void Pause_StatInt(int row, unsigned int label, int value)
{
    Pause_StatRow(row, gText.Get(label));
    FE::_Peek()->m_Text.Format("%d", value);
    FE::_Peek()->m_Flags |= 0x4000;
    FE::_Pop();
    FE::_Pop();
}

// An empty spacer row.
static void Pause_StatGap(int row)
{
    FE::_Push(FE::_Peek()->ObjectAdd(row, 0, new FEObject((FEObjType::Flags)0, 0, 0, 0xfdffffff, FE_Blank, FE_Blank,
                                                          (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4)));
    FE::_Peek()->m_Flags &= ~8;
    FE::_Pop();
}

// The help line under a page: a tinted strip with button prompts (text hashes, 0-terminated), optionally
// with their text rects locked. Added to the page at (row, 0).
static void Pause_HelpBar(ex::RGBA tint, int row, const unsigned int *pText, signed char bLock)
{
    int n = 0;
    while (pText[n]) n++;
    FE::_Push(FE::_Peek()->ObjectAdd(row, 0, new FELayoutLayered(2, 0, 0, 0x1d0, 0x18, (EXW::TextAlign)4)));
    FE::_Push(FE::_Peek()->ObjectAdd(0, new FEObject((FEObjType::Flags)1, 0x1000025, 0x6000054, 0xfdffffff, FE_Blank, FE_Blank,
                                                     (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4)));  // HT_Texture_FEWhite
    FE::_Peek()->SetMathmos((FEObjColour::Flags)1, FEMathmos(tint), 1);
    FE::_Pop();
    FE::_Push(FE::_Peek()->ObjectAdd(1, new FELayoutFixedGrid(1, n, 0, 0, 0, 0, (EXW::TextAlign)4)));
    for (int i = 0; i < n; i++) {
        FEObject *p = FE::_Peek()->ObjectAdd(0, i, new FEObject((FEObjType::Flags)2, 0x100003a, 0x700000c, 0xfdffffff, gText.Get(pText[i]),
                                                                FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        if (bLock) p->LockTextRect(0, 0, 0, 0);
    }
    FE::_Pop();
    FE::_Pop();
}

static const unsigned int s_HelpMoveScrollSelect[] = {0x51000472, 0x51000473, 0x51000476, 0};  // ~L/~R Move, ~V Scroll, ~E Select
static const unsigned int s_HelpMoveScroll[] = {0x51000472, 0x51000473, 0};                     // ~L/~R Move, ~V Scroll

// An options row: the label (which takes marker 1 for the control to link its colour to) at (0, 0) of a
// two-cell row; the caller adds the control at (0, 1).
static void Pause_OptionRow(int row)
{
    FE::_Push(FE::_Peek()->ObjectAdd(row, 0, new FELayoutFixedGrid(1, 2, 0, 0, 0, 0, (EXW::TextAlign)4)));
}
static void Pause_OptionLabel(const EXString &label)
{
    FE::_Push(FE::_Peek()->ObjectAdd(0, 0, new FEObject((FEObjType::Flags)2, 0x100003a, 0x7000004, 0, label, FE_Blank, (EXW::TextAlign)2,
                                                        0, 0, 0, 0, (EXW::TextAlign)4)));
    FE::_SetMarker(1);
    FE::_Peek()->m_Flags &= ~8;
    FE::_Pop();
}

// An on/off option bound to a settings byte.
static void Pause_OptionCheck(int row, const EXString &label, signed char bInit, s8 *pValue, signed char bFirst)
{
    Pause_OptionRow(row);
    Pause_OptionLabel(label);
    FE::_Push(FE::_Peek()->ObjectAdd(0, 1, new FELayoutCheckbox(bInit, 0, 0, 0, 0, (EXW::TextAlign)4)));
    if (bFirst) FE::_SetMarker(0);
    FE::_Marker(1)->MathmosLink(FE::_Peek());
    FE::_Peek()->m_HelpText = gText.Get(0x510004d7);  // ~E Toggle
    FE::_Peek()->Bind((FEDataType::Flags)1, pValue);
    FE::_Pop();
    FE::_Pop();
}

// A multiple-choice option (text hashes, 0-terminated; each choice's data is its index) bound to a
// settings byte. 'pData' optionally overrides the choice data.
static void Pause_OptionChoice(int row, const EXString &label, const unsigned int *pChoices, const int *pData, s8 *pValue)
{
    int n = 0;
    while (pChoices[n]) n++;
    Pause_OptionRow(row);
    FE::_Peek()->m_HelpText = gText.Get(0x51000474);  // ~H Change
    Pause_OptionLabel(label);
    FE::_Push(FE::_Peek()->ObjectAdd(0, 1, new FELayoutScrollGrid(1, n, 0, 0, 0, 0, (EXW::TextAlign)4)));
    FE::_Marker(1)->MathmosLink(FE::_Peek());
    for (int i = 0; i < n; i++)
        FE::_Peek()->ObjectAdd(0, i, new FEObject((FEObjType::Flags)2, 0x100003a, 0x7000004, pData ? pData[i] : i, gText.Get(pChoices[i]),
                                                  FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
    FE::_Peek()->SetVisibleCount(1, 1);
    FE::_Peek()->Bind((FEDataType::Flags)1, pValue);
    FE::_Pop();
    FE::_Pop();
}

// A 0-100 volume slider.
static void Pause_OptionVolume(int row, const EXString &label, unsigned __int64 data, int *pValue)
{
    Pause_OptionRow(row);
    Pause_OptionLabel(label);
    FE::_Push(FE::_Peek()->ObjectAdd(0, 1, new FELayoutSlider(0, 100, 0, 0, 0, 0, (EXW::TextAlign)4)));
    FE::_Marker(1)->MathmosLink(FE::_Peek());
    FE::_Peek()->m_Data = data;
    FE::_Peek()->SetRange(0, 100);
    FE::_Peek()->SetDelta(5);
    FE::_Peek()->m_Flags &= ~0x8000000;
    FE::_Peek()->m_HelpText = gText.Get(0x51000474);  // ~H Change
    FE::_Peek()->Bind((FEDataType::Flags)3, pValue);
    FE::_Pop();
    FE::_Pop();
}

// The help bar of an options page: Move, Scroll and a third prompt that becomes the focused item's help
// (Marker 0's listener shows the focused item's help text there). Pops the page as well.
static void Pause_HelpBarDynamic(ex::RGBA tint, unsigned int last)
{
    FE::_Push(FE::_Peek()->ObjectAdd(1, 0, new FELayoutLayered(2, 0, 0, 0x1d0, 0x18, (EXW::TextAlign)4)));
    FE::_Push(FE::_Peek()->ObjectAdd(0, new FEObject((FEObjType::Flags)1, 0x1000025, 0x6000054, 0xfdffffff, FE_Blank, FE_Blank,
                                                     (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4)));
    FE::_Peek()->SetMathmos((FEObjColour::Flags)1, FEMathmos(tint), 1);
    FE::_Pop();
    FE::_Push(FE::_Peek()->ObjectAdd(1, new FELayoutFixedGrid(1, 3, 0, 0, 0, 0, (EXW::TextAlign)4)));
    FE::_Peek()->ObjectAdd(0, 0, new FEObject((FEObjType::Flags)2, 0x100003a, 0x700000c, 0xfdffffff, gText.Get(0x51000472), FE_Blank,
                                              (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
    FE::_Peek()->ObjectAdd(0, 1, new FEObject((FEObjType::Flags)2, 0x100003a, 0x700000c, 0xfdffffff, gText.Get(0x51000473), FE_Blank,
                                              (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
    FE::_Push(FE::_Peek()->ObjectAdd(0, 2, new FEObject((FEObjType::Flags)2, 0x100003a, 0x700000c, 0xfdffffff, gText.Get(last), FE_Blank,
                                                        (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4)));
    FE::_Marker(0)->Listener(0x5f5f5f5f5f76616eull)->SetHelpObject(FE::_Peek());
    FE::_Pop();
    FE::_Pop();
    FE::_Pop();
    FE::_Pop();
}

// Builds the pause menu: a level stats page (only on story levels) shown from the level menu, and the main
// pause screen, a tab bar (level, objectives, hints, options, controller, weapons) over paged contents.
void FEPause::ContentCreate()
{
    ex::RGBA tint = FEHud::TintColour(0);
    tint.m_A = 0x40;
    if (FEHud::Instance(0)) {
        FEHud::Instance(1)->StatePush();
        FEHud::Instance(1)->StateSub(0x40);
    }
    StateAdd(0x60);
    EnvironmentStateAdd(2);
    GameData::Instance(1)->SettingsStore();
    EXBaseSoundManager::GetSoundManager()->StartSound(0x1af0015b);
    FEListener::RemapModeSet((FERemap::Flags)2);
    m_Mathmos3 = FEMathmos::DefaultFadeUp();
    m_Mathmos = FEMathmos(ex::RGBA(0x40, 0x40, 0x40, 0x80));
    m_Mathmos2 = FEMathmos(ex::RGBA(0x40, 0x40, 0x40, 0x80), ex::RGBA(0x40, 0x40, 0x40, 0x40), 60.0f, (FEWaveform::Flags)1, 0);
    m_Unk474 = 0;

    FE::_Init(SetObject(new FELayoutLayered(3, 0, 0, 0, 0, (EXW::TextAlign)4)));
    FE::_Peek()->SetRemoteMathmos((FEObjColour::Flags)2, &m_Mathmos3, 1);
    FE::_Peek()->SetMathmos((FEObjColour::Flags)0, FEMathmos(ex::RGBA(0xff, 0xff, 0xff, 0x80)), 1);
    FE::_Peek()->SetMathmos((FEObjColour::Flags)1, FEMathmos(ex::RGBA(0x40, 0x40, 0x40, 0x80)), 1);
    FE::_Peek()->SetMathmos((FEObjColour::Flags)3, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80)), 1);

    // 0: animated backdrop
    FE::_Push(FE::_Peek()->ObjectAdd(0, new FEObject((FEObjType::Flags)4, 0x1000025, 0x4000606, 0xfdffffff, FE_Blank, FE_Blank,
                                                     (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4)));
    FE::_Peek()->SetMathmos((FEObjColour::Flags)1, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80)), 1);
    FE::_Peek()->Script(0)->SetSize(0x1e0, 0x17c, 0);
    FE::_Peek()->Script(0)->Play(0, 0);
    FE::_Pop();

    // 2: level stats (story levels only)
    FE::_Push(FE::_Peek()->ObjectAdd(2, new FELayoutLayered(2, 0, 0, 0x1f4, 0x140, (EXW::TextAlign)4)));
    int level = FELevelManager::Instance(1)->IndexFromLevel(gCurrentMap_GeoHash);
    GameData *pData = GameData::Instance(1);
    if (level >= 0 && level < 27) {
        m_Unk474 = 1;
        unsigned int reward = GetLevelUpgrade_RewardIndex(level);
        int gotTasks = 0, gotAttribs = 0, gotCostumes = 0;
        if (reward && GameData::Instance(1)->m_Values.RewardAwarded(reward)) {
            gotTasks = 1;
            if (GetLevelUpgrade_AttributeUpgrades(level)) gotAttribs = 1;
            if (GetLevelUpgrade_Costumes(level)) gotCostumes = 1;
        }
        unsigned int nWeapons = GetLevelUpgrade_WeaponUpgrades(level);
        unsigned int base = GetLevelUpgrade_WeaponUpgradesBaseIndex(level);
        int gotWeapons = 0;
        if (base)
            for (unsigned int i = 0; i < nWeapons; i++)
                if (GameData::Instance(1)->m_Values.RewardAwarded(base + i)) gotWeapons++;

        FE::_Push(FE::_Peek()->ObjectAdd(0, new FEObject((FEObjType::Flags)1, 0x1000025, 0x6000057, 0xfdffffff, FE_Blank, FE_Blank,
                                                         (EXW::TextAlign)4, 0, 0, 0x1ae, 0x14a, (EXW::TextAlign)4)));  // HT_Texture_FEDialog
        FE::_Peek()->Inflate(0x10, 8);
        FE::_Peek()->m_BorderWidth = 0x10;
        FE::_Pop();
        FE::_Push(FE::_Peek()->ObjectAdd(1, new FELayoutFlexGrid(3, 1, (EXW::TextAlign)4)));
        FE::_Push(FE::_Peek()->ObjectAdd(0, 0, new FEObject((FEObjType::Flags)2, 0x100003a, 0x7000004, 0xfdffffff, gText.Get(0x510008d3),
                                                            FE_Blank, (EXW::TextAlign)3, 0, 0, 0x190, 0x1e, (EXW::TextAlign)4)));
        FE::_Pop();
        FE::_Push(FE::_Peek()->ObjectAdd(1, 0, new FELayoutScrollGrid(XItemHandler_Player::Instance()->GetFaveWeapon() >= 0 ? 0x23 : 0x21, 1, 0, 0,
                                                                      0x190, 0x109, (EXW::TextAlign)4)));
        ((FELayoutScrollGrid *)FE::_Peek())->m_UpDownArrowY = -7;
        FE::_Peek()->SetVisibleCount(10, 1);

        Pause_StatRow(0, gText.Get(0x5100088f));  // Bonus tasks completed
        FE::_Peek()->m_Text.Format("%d/%d", gotTasks, GetLevelUpgrade_BonusTasks(level));
        FE::_Peek()->m_Flags |= 0x4000;
        FE::_Pop();
        FE::_Pop();
        Pause_StatRow(1, gText.Get(0x51000890));  // Weapon upgrades received
        FE::_Peek()->m_Text.Format("%d/%d", gotWeapons, GetLevelUpgrade_WeaponUpgrades(level));
        FE::_Peek()->m_Flags |= 0x4000;
        FE::_Pop();
        FE::_Pop();
        Pause_StatRow(2, gText.Get(0x51000891));  // Attribute upgrades received
        FE::_Peek()->m_Text.Format("%d/%d", gotAttribs, GetLevelUpgrade_AttributeUpgrades(level));
        FE::_Peek()->m_Flags |= 0x4000;
        FE::_Pop();
        FE::_Pop();
        Pause_StatRow(3, gText.Get(0x51000892));  // Alternate costumes received
        FE::_Peek()->m_Text.Format("%d/%d", gotCostumes, GetLevelUpgrade_Costumes(level));
        FE::_Peek()->m_Flags |= 0x4000;
        FE::_Pop();
        FE::_Pop();
        Pause_StatGap(4);
        GameStats &stats = pData->m_Stats;
        Pause_StatInt(5, 0x51000893, stats.m_Kills);           // Honorable kills
        Pause_StatInt(6, 0x51000894, stats.m_KillsDishonour);  // Dishonorable kills
        Pause_StatGap(7);
        Pause_StatInt(8, 0x51000895, stats.m_Unk10[0]);    // Executions
        Pause_StatInt(9, 0x51000896, stats.m_Unk10[1]);    // Backs broken
        Pause_StatInt(10, 0x51000897, stats.m_Unk10[2]);   // Skulls ripped
        Pause_StatInt(11, 0x51000898, stats.m_Unk10[3]);   // Throats sliced
        Pause_StatInt(12, 0x51000899, stats.m_Unk10[4]);   // Necks snapped
        Pause_StatInt(13, 0x5100089a, stats.m_Unk10[5]);   // Heads crushed
        Pause_StatGap(14);
        Pause_StatInt(15, 0x5100089b, stats.m_Unk10[6]);   // Grapple kills
        Pause_StatRow(16, gText.Get(0x5100089c));          // Furthest distance opponent thrown
        FE::_Peek()->m_Text.Format("%.2fm", ex::fd(stats.LongestThrow()));
        FE::_Peek()->m_Flags |= 0x4000;
        FE::_Pop();
        FE::_Pop();
        Pause_StatRow(17, gText.Get(0x5100089d));          // Furthest distance opponent kicked
        FE::_Peek()->m_Text.Format("%.2fm", ex::fd(stats.LongestKick()));
        FE::_Peek()->m_Flags |= 0x4000;
        FE::_Pop();
        FE::_Pop();
        Pause_StatInt(18, 0x5100089e, stats.m_Unk10[9]);   // Ground kills
        Pause_StatInt(19, 0x5100089f, stats.m_Unk10[10]);  // Railing kills
        Pause_StatInt(20, 0x510008a0, stats.m_Unk10[11]);  // Wall kills
        Pause_StatGap(21);
        Pause_StatInt(22, 0x510008a1, stats.m_Unk10[12]);  // Opponents impaled
        Pause_StatInt(23, 0x510008a2, stats.m_Unk10[13]);  // Juggle kills
        Pause_StatGap(24);
        int row = 0x19;
        if (XItemHandler_Player::Instance()->GetFaveWeapon() >= 0) {
            // Favorite weapon: label plus the weapon's icon.
            FE::_Push(FE::_Peek()->ObjectAdd(0x19, 0, new FELayoutFlexGrid(1, 3, (EXW::TextAlign)4)));
            FE::_Peek()->m_Flags |= 8;
            FE::_Push(FE::_Peek()->ObjectAdd(0, 0, new FEObject((FEObjType::Flags)2, 0x100003a, 0x7000004, 0xfdffffff, Text_Get(0x510008a3),
                                                                FE_Blank, (EXW::TextAlign)7, 0, 0, 0x145, 0x1e, (EXW::TextAlign)4)));
            FE::_Pop();
            FE::_Push(FE::_Peek()->ObjectAdd(0, 1, new FEObject((FEObjType::Flags)0, 0, 0, 0xfdffffff, FE_Blank, FE_Blank,
                                                                (EXW::TextAlign)4, 0, 0, 0x2d, 0x1e, (EXW::TextAlign)4)));
            FE::_Pop();
            FEHud *pHud = FEHud::Instance(1);
            FEWeapon w = pHud->GetWeaponPreset(XItemHandler_Player::Instance()->GetFaveWeapon(), -1, 0);
            FE::_Push(FE::_Peek()->ObjectAdd(0, 2, new FEObject((FEObjType::Flags)1, 0x1000025, w.m_Icon, 0xfdffffff, FE_Blank, FE_Blank,
                                                                (EXW::TextAlign)7, 0, 0, 0x1e, 0x1e, (EXW::TextAlign)4)));
            FE::_Peek()->m_Flags |= 0x4000;
            FE::_Pop();
            FE::_Pop();
            Pause_StatGap(0x1a);
            row = 0x1b;
        }
        Pause_StatInt(row++, 0x510008a4, stats.m_Unk4c[0]);  // Headshots
        Pause_StatInt(row++, 0x510008a5, stats.m_Unk4c[1]);  // Decapitations
        Pause_StatInt(row++, 0x510008a6, stats.m_Unk4c[2]);  // Arms severed
        Pause_StatInt(row++, 0x510008a7, stats.m_Unk4c[3]);  // Bodies split
        Pause_StatInt(row++, 0x510008a8, stats.m_Unk4c[4]);  // Opponents burnt to death
        Pause_StatGap(row++);
        Pause_StatInt(row++, 0x510008a9, stats.m_Unk4c[5]);  // Healing charges used
        Pause_StatInt(row, 0x510008aa, stats.m_Recharges);   // Recharge points used

        FE::_Peek()->ListenerAdd(new FEListener(0x5f5f5f5f5f76616eull, 0x401, 0, (FEPad::Flags)0xf));  // "nav_____"
        FE::_Peek()->Listener(0x5f5f5f5f5f76616eull)->SetFocus(FE::_Peek()->Object(0, 0), 1);
        FE::_Peek()->m_Data = 0x5f5f5f7374617473ull;  // "stats___"
        FE::_Peek()->SetEventCallback(Pause_ExitStats, 1);
        FE::_Pop();
        FE::_Push(FE::_Peek()->ObjectAdd(2, 0, new FEObject((FEObjType::Flags)2, 0x100003a, 0x7000004, 0xfdffffff, gText.Get(0x510008d4),
                                                            FE_Blank, (EXW::TextAlign)5, 0, 0, 0x190, 0x1e, (EXW::TextAlign)4)));  // ~V Scroll  ~F Back
        FE::_Pop();
        FE::_Pop();
        FE::_Peek()->m_Flags &= ~0x10040;
    }
    FE::_Pop();

    // 1: the pause screen proper: tab bar over the paged contents
    FE::_Push(FE::_Peek()->ObjectAdd(1, new FELayoutFlexGrid(3, 1, (EXW::TextAlign)4)));
    FE::_Push(FE::_Peek()->ObjectAdd(0, 0, new FEObject((FEObjType::Flags)0, 0, 0, 0xfdffffff, FE_Blank, FE_Blank, (EXW::TextAlign)4, 0, 0,
                                                        0, 0, (EXW::TextAlign)4)));
    FE::_Pop();
    FE::_Push(FE::_Peek()->ObjectAdd(1, 0, new FELayoutLayered(2, 0, 0, 0x1d0, 0x18, (EXW::TextAlign)4)));
    FE::_Push(FE::_Peek()->ObjectAdd(0, new FEObject((FEObjType::Flags)1, 0x1000025, 0x6000054, 0xfdffffff, FE_Blank, FE_Blank,
                                                     (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4)));  // HT_Texture_FEWhite
    FE::_Peek()->SetMathmos((FEObjColour::Flags)1, FEMathmos(tint), 1);
    FE::_Pop();
    FE::_Push(FE::_Peek()->ObjectAdd(1, new FELayoutFlexGrid(1, 6, (EXW::TextAlign)4)));
    {
        static const struct {
            unsigned __int64 m_Data;
            unsigned int m_Text;
        } tabs[6] = {
            {0x5f5f5f6c6576656cull, 0x510001f0},  // "level___"  Quit
            {0x5f5f5f5f5f6a626full, 0x510001f1},  // "obj_____"  Objectives
            {0x5f5f5f73746e6968ull, 0x510001f2},  // "hints___"  Hints
            {0x5f736e6f6974706full, 0x510001f3},  // "options_"  Options
            {0x736c6f72746e6f63ull, 0x510001f7},  // "controls"  Controller
            {0x5f736e6f70616577ull, 0x510004c3},  // "weapons_"  Weapons
        };
        for (int i = 0; i < 6; i++)
            FE::_Peek()
                ->ObjectAdd(0, i, new FEObject((FEObjType::Flags)2, 0x100003a, 0x700000c, tabs[i].m_Data, gText.Get(tabs[i].m_Text),
                                               FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4))
                ->LockTextRect(0, 0, 5, 0)
                ->StateAdd(0x8000);
    }
    FE::_Peek()->ListenerAdd(new FEListener(0x5f5f5f76616e726cull, 0x4000, 0, (FEPad::Flags)0xf));  // "lrnav___"
    // Open on the level tab in story levels, else on objectives (hints if there is a new one).
    unsigned __int64 tab;
    if (FELevelManager::Instance(1)->LevelFromHash(gCurrentMap_GeoHash, 0)->m_Type == 1)
        tab = 0x5f5f5f6c6576656cull;
    else if (FEObjectives::Instance(1)->NewObjective(0x28))
        tab = 0x5f5f5f73746e6968ull;
    else
        tab = 0x5f5f5f5f5f6a626full;
    FE::_Peek()->Listener(0x5f5f5f76616e726cull)->SetFocusByData(tab, 0);
    FE::_Pop();
    FE::_Pop();

    FE::_Push(FE::_Peek()->ObjectAdd(2, 0, new FELayoutPaged(7, m_pObject->FindChildListener(0x5f5f5f76616e726cull), 0, 0, 0x1d0, 0x13a,
                                                             (EXW::TextAlign)4)));

    // page: level (level stats, restart, quit)
    FE::_Push(FE::_Peek()->ObjectAdd(0, new FELayoutFlexGrid(2, 1, (EXW::TextAlign)4)));
    FE::_Peek()->m_Data = 0x5f5f5f6c6576656cull;  // "level___"
    unsigned int map = gCurrentMap_GeoHash;
    if (FELevelManager::Instance(1)->IndexFromLevel(gCurrentMap_GeoHash) == -1) map = gPrevMap_GeoHash;
    FE::_Push(FE::_Peek()->ObjectAdd(0, 0, new FELayoutScrollGrid(4, 1, 0, 0, 0x1d0, 0x122, (EXW::TextAlign)4)));
    FE::_Peek()->m_Rect.x += 8;
    FE::_Peek()->m_Rect.w -= 0x10;
    FE::_Peek()->m_Flags |= 2;
    FE::_Peek()->Resize();
    FE::_Push(FE::_Peek()->ObjectAdd(0, 0, new FEObject((FEObjType::Flags)2, 0x100003a, 0x7000004, 0xabbaabba, EXString("Level Stats"),
                                                        FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4)));
    if (!m_Unk474) {
        // No stats on this level: grey the entry out.
        FE::_Peek()->m_Flags &= ~8;
        FE::_Peek()->SetMathmos((FEObjColour::Flags)1, FEMathmos(ex::RGBA(0x10, 0x10, 0x10, 0x80)), 1);
    }
    FE::_Pop();
    FE::_Peek()->ObjectAdd(1, 0, new FEObject((FEObjType::Flags)2, 0x100003a, 0x7000004, (__int64)(int)map, EXString("Restart Level"),
                                              FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
    FE::_Peek()->ObjectAdd(2, 0, new FEObject((FEObjType::Flags)2, 0x100003a, 0x7000004, XApp::Frontend2DFile(), EXString("Quit Level"),
                                              FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
    FE::_Peek()->ObjectAdd(3, 0, new FEObject((FEObjType::Flags)2, 0x100003a, 0x7000004, XApp::Frontend3DFile(),
                                              EXString("Quit to Main Menu"), FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
    FE::_Peek()->ListenerAdd(new FEListener(0x5f5f5f5f5f76616eull, 0x401, 0, (FEPad::Flags)0xf));                  // "nav_____"
    FE::_Peek()->ListenerChildAdd(FEListener(0x5f5f5f5f5f6e7462ull, 0x8000, Pause_SelectMap, (FEPad::Flags)0xf));  // "btn_____"
    FE::_Peek()->Listener(0x5f5f5f5f5f76616eull)->SetDefaultFocusByData((__int64)(int)map);
    FE::_Peek()->SetEventCallback(Pause_SelectMap, 1);
    FE::_Pop();
    Pause_HelpBar(tint, 1, s_HelpMoveScrollSelect, 1);
    FE::_Pop();

    // page: objectives (list of short texts, the selected one's full text below)
    FE::_Push(FE::_Peek()->ObjectAdd(1, new FELayoutFlexGrid(2, 1, (EXW::TextAlign)4)));
    FE::_Peek()->m_Data = 0x5f5f5f5f5f6a626full;  // "obj_____"
    int nObjectives = 0;
    for (int i = FEObjectives::Instance(1)->m_nCount - 1; i >= 0; i--)
        if (FEObjectives::Instance(1)->MsgType(i) & 0x11) nObjectives++;
    FE::_Push(FE::_Peek()->ObjectAdd(0, 0, new FELayoutLayered(2, 0, 0, 0x1d0, 0x122, (EXW::TextAlign)4)));
    FE::_Push(FE::_Peek()->ObjectAdd(0, new FELayoutFixedGrid(2, 1, 0, 0, 0x1d0, 0x122, (EXW::TextAlign)4)));
    FE::_Push(FE::_Peek()->ObjectAdd(0, 0, new FELayoutScrollGrid(nObjectives >= 1 ? nObjectives : 1, 1, 0, 0, 0, 0, (EXW::TextAlign)4)));
    FE::_Peek()->SetVisibleCount(7, 1);
    FE::_SetMarker(2);
    FE::_Peek()->m_Rect.x += 8;
    FE::_Peek()->m_Rect.w -= 0x10;
    FE::_Peek()->m_Flags |= 2;
    FE::_Peek()->Resize();
    int row = -1, focus = -1;
    if (nObjectives > 0) {
        for (int i = 0; i < FEObjectives::Instance(1)->m_nCount; i++) {
            if (!(FEObjectives::Instance(1)->m_Objectives[i].m_Type & 0x11)) continue;
            row++;
            // Focus the first unread objective.
            if (focus < 0 && !FEObjectives::Instance(1)->IsRead(i)) focus = row;
            const char *mark = FEObjectives::Instance(1)->IsComplete(i) ? "~& " : "~' ";
            FE::_Push(FE::_Peek()->ObjectAdd(row, 0, new FEObject((FEObjType::Flags)2, 0x100003a, 0x7000004, (__int64)i, EXString(mark),
                                                                  FE_Blank, (EXW::TextAlign)2, 0, 0, 0, 0, (EXW::TextAlign)4)));
            EXString msg = FEObjectives::Instance(1)->ShortMsg(i);
            FE::_Peek()->SetText(FE::_Peek()->m_Text + msg, 0, 0);
            FE::_Pop();
        }
    } else {
        row = 0;
        FE::_Push(FE::_Peek()->ObjectAdd(0, 0, new FEObject((FEObjType::Flags)2, 0x100003a, 0x7000004, 0xfdffffff, gText.Get(0x510001f5),
                                                            FE_Blank, (EXW::TextAlign)2, 0, 0, 0, 0, (EXW::TextAlign)4)));  // None
        FE::_Pop();
    }
    FE::_Peek()->ListenerAdd(new FEListener(0x5f5f5f5f5f76616eull, 0x401, 0, (FEPad::Flags)0xf));  // "nav_____"
    FE::_Peek()->Listener(0x5f5f5f5f5f76616eull)->SetDefaultFocus(FE::_Peek()->Object(focus >= 0 ? focus : row, 0));
    FE::_Peek()->SetEventCallback(Pause_ViewObjective, 1);
    FE::_Pop();
    FE::_Push(FE::_Peek()->ObjectAdd(1, 0, new FEObject((FEObjType::Flags)2, 0x100003a, 0x700000c, 0xfdffffff, FE_Blank, FE_Blank,
                                                        (EXW::TextAlign)0, 0, 0, 0, 0, (EXW::TextAlign)4)));
    FE::_Peek()->m_Rect.x += 8;
    FE::_Peek()->m_Rect.y += 8;
    FE::_Peek()->m_Rect.w -= 0x10;
    FE::_Peek()->m_Rect.h -= 0x10;
    FE::_Peek()->m_Flags |= 2;
    FE::_Peek()->Resize();
    FE::_Peek()->m_Flags |= 0x20000;
    Pause_ViewObjective(FE::_Marker(2)->Object(0, 0), (FEObjEvent::Flags)9);  // show the focused objective's text
    FE::_Pop();
    FE::_Pop();

    // divider between the list and the text
    FE::_Push(FE::_Peek()->ObjectAdd(1, new FEObject((FEObjType::Flags)1, 0x1000025, 0x6000054, 0xfdffffff, FE_Blank, FE_Blank,
                                                     (EXW::TextAlign)4, 0, 0, 0x1d8, 2, (EXW::TextAlign)4)));
    FE::_Peek()->SetMathmos((FEObjColour::Flags)1, FEMathmos(FEHud::TintColour(0)), 1);
    FE::_Pop();
    FE::_Pop();
    Pause_HelpBar(tint, 1, s_HelpMoveScroll, 0);
    FE::_Pop();

    // page: hints (unread ones pulse)
    FE::_Push(FE::_Peek()->ObjectAdd(2, new FELayoutFlexGrid(2, 1, (EXW::TextAlign)4)));
    FE::_Peek()->m_Data = 0x5f5f5f73746e6968ull;  // "hints___"
    int nHints = 0;
    for (int i = FEObjectives::Instance(1)->m_nCount - 1; i >= 0; i--)
        if (FEObjectives::Instance(1)->MsgType(i) & 0x28) nHints++;
    FE::_Push(FE::_Peek()->ObjectAdd(0, 0, new FELayoutLayered(2, 0, 0, 0x1d0, 0x122, (EXW::TextAlign)4)));
    FE::_Push(FE::_Peek()->ObjectAdd(0, new FELayoutFixedGrid(2, 1, 0, 0, 0x1d0, 0x122, (EXW::TextAlign)4)));
    FE::_Push(FE::_Peek()->ObjectAdd(0, 0, new FELayoutScrollGrid(nHints >= 1 ? nHints : 1, 1, 0, 0, 0, 0, (EXW::TextAlign)4)));
    FE::_Peek()->SetVisibleCount(7, 1);
    FE::_SetMarker(2);
    FE::_Peek()->m_Rect.x += 8;
    FE::_Peek()->m_Rect.w -= 0x10;
    FE::_Peek()->m_Flags |= 2;
    FE::_Peek()->Resize();
    row = -1;
    if (nHints > 0) {
        for (int i = 0; i < FEObjectives::Instance(1)->m_nCount; i++) {
            if (!(FEObjectives::Instance(1)->m_Objectives[i].m_Type & 0x28)) continue;
            row++;
            FE::_Push(FE::_Peek()->ObjectAdd(row, 0, new FEObject((FEObjType::Flags)2, 0x100003a, 0x7000004, (__int64)i,
                                                                  FEObjectives::Instance(1)->ShortMsg(i), FE_Blank, (EXW::TextAlign)2,
                                                                  0, 0, 0, 0, (EXW::TextAlign)4)));
            FE::_Peek()->SetRemoteMathmos((FEObjColour::Flags)1, FEObjectives::Instance(1)->IsRead(i) ? &m_Mathmos : &m_Mathmos2, 1);
            FE::_Pop();
        }
    } else {
        row = 0;
        FE::_Push(FE::_Peek()->ObjectAdd(0, 0, new FEObject((FEObjType::Flags)2, 0x100003a, 0x7000004, 0xfdffffff, gText.Get(0x510001f5),
                                                            FE_Blank, (EXW::TextAlign)2, 0, 0, 0, 0, (EXW::TextAlign)4)));  // None
        FE::_Pop();
    }
    FE::_Peek()->ListenerAdd(new FEListener(0x5f5f5f5f5f76616eull, 0x401, 0, (FEPad::Flags)0xf));  // "nav_____"
    FE::_Peek()->Listener(0x5f5f5f5f5f76616eull)->SetDefaultFocus(FE::_Peek()->Object(row, 0));
    FE::_Peek()->SetEventCallback(Pause_ViewHint, 1);
    FE::_Pop();
    FE::_Push(FE::_Peek()->ObjectAdd(1, 0, new FEObject((FEObjType::Flags)2, 0x100003a, 0x700000c, 0xfdffffff, FE_Blank, FE_Blank,
                                                        (EXW::TextAlign)0, 0, 0, 0, 0, (EXW::TextAlign)4)));
    FE::_Peek()->m_Rect.x += 8;
    FE::_Peek()->m_Rect.y += 8;
    FE::_Peek()->m_Rect.w -= 0x10;
    FE::_Peek()->m_Rect.h -= 0x10;
    FE::_Peek()->m_Flags |= 2;
    FE::_Peek()->Resize();
    FE::_Peek()->m_Flags |= 0x20000;
    Pause_ViewHint(FE::_Marker(2)->Object(0, 0), (FEObjEvent::Flags)9);
    FE::_Pop();
    FE::_Pop();
    FE::_Push(FE::_Peek()->ObjectAdd(1, new FEObject((FEObjType::Flags)1, 0x1000025, 0x6000054, 0xfdffffff, FE_Blank, FE_Blank,
                                                     (EXW::TextAlign)4, 0, 0, 0x1d8, 2, (EXW::TextAlign)4)));
    FE::_Peek()->SetMathmos((FEObjColour::Flags)1, FEMathmos(FEHud::TintColour(0)), 1);
    FE::_Pop();
    FE::_Pop();
    Pause_HelpBar(tint, 1, s_HelpMoveScroll, 0);
    FE::_Pop();

    // page: options
    FE::_Push(FE::_Peek()->ObjectAdd(3, new FELayoutFlexGrid(2, 1, (EXW::TextAlign)4)));
    FE::_Peek()->m_Data = 0x5f736e6f6974706full;  // "options_"
    FE::_Push(FE::_Peek()->ObjectAdd(0, 0, new FELayoutScrollGrid(12, 1, 0, 0, 0x1d0, 0x122, (EXW::TextAlign)4)));
    FE::_Peek()->m_Rect.x += 8;
    FE::_Peek()->m_Rect.w -= 0x10;
    FE::_Peek()->m_Flags |= 2;
    FE::_Peek()->Resize();
    {
        GameValues *pV = &GameData::Instance(1)->m_Values;
        static const unsigned int viewChoices[] = {0x510006b9, 0x510006b8, 0};              // Camera View, Predator View
        static const unsigned int camChoices[] = {0x51000207, 0x51000208, 0x510002c8, 0};    // Auto, Manual, Track
        static const unsigned int markerChoices[] = {0x510002b7, 0x510002ba, 0};            // Always, 1st Person
        static const int markerData[] = {1, 0};
        Pause_OptionCheck(0, gText.Get(0x510001f8), 1, &pV->m_bInvertX3rd, 1);  // 3rd Person Invert Horz.
        Pause_OptionCheck(1, gText.Get(0x510001f9), 1, &pV->m_bInvertY3rd, 0);  // 3rd Person Invert Vert.
        Pause_OptionCheck(2, gText.Get(0x51000470), 0, &pV->m_bInvertX1st, 0);  // 1st Person Invert Horz.
        Pause_OptionCheck(3, gText.Get(0x51000471), 0, &pV->m_bInvertY1st, 0);  // 1st Person Invert Vert.
        Pause_OptionChoice(4, gText.Get(0x510005e2), viewChoices, 0, &pV->m_FirstPersonView);  // Enter 1st Person
        Pause_OptionChoice(5, gText.Get(0x51000204), camChoices, 0, &pV->m_CameraMode);        // Camera Mode
        Pause_OptionCheck(6, EXString("Action Camera"), 0, &pV->m_bActionCamera, 0);
        Pause_OptionChoice(7, gText.Get(0x510002b6), markerChoices, markerData, (s8 *)&pV->m_Unk1c[6]);  // HUD Waypoint Markers
        Pause_OptionCheck(8, gText.Get(0x510001de), 0, (s8 *)&pV->m_Unk1c[2], 0);  // Show Subtitles
        Pause_OptionVolume(9, gText.Get(0x510001fc), 0x5f5f5f636973756dull, &pV->m_MusicVolume);  // Music Volume, "music___"
        Pause_OptionVolume(10, gText.Get(0x510001fb), 0x5f5f5f5f5f786673ull, &pV->m_MovieVolume);  // SFX Volume, "sfx_____"
        Pause_OptionCheck(11, EXString("Auto-equip Weapon"), 0, &pV->m_bAutoEquip, 0);
    }
    FE::_Peek()->ListenerAdd(new FEListener(0x5f5f5f5f5f76616eull, 0xc03, 0, (FEPad::Flags)0xf));  // "nav_____"
    FE::_Peek()->Listener(0x5f5f5f5f5f76616eull)->SetDefaultFocus(FE::_Marker(0));
    FE::_Peek()->SetEventCallback(Pause_Options, 1);
    FE::_SetMarker(0);
    FE::_Pop();
    Pause_HelpBarDynamic(tint, 0x510004d7);  // ~L/~R Move, ~V Scroll, ~E Toggle (follows the focus)

    // page: controller (style, vibration, lean, and the pad picture labelled for the chosen style)
    FE::_Push(FE::_Peek()->ObjectAdd(4, new FELayoutFlexGrid(2, 1, (EXW::TextAlign)4)));
    FE::_Peek()->m_Data = 0x736c6f72746e6f63ull;  // "controls"
    FE::_Push(FE::_Peek()->ObjectAdd(0, 0, new FELayoutFlexGrid(2, 1, (EXW::TextAlign)4)));
    FE::_Push(FE::_Peek()->ObjectAdd(0, 0, new FELayoutFixedGrid(3, 1, 0, 0, 0x1d0, 0x48, (EXW::TextAlign)4)));
    FE::_Peek()->m_Rect.x += 8;
    FE::_Peek()->m_Rect.w -= 0x10;
    FE::_Peek()->m_Flags |= 2;
    FE::_Peek()->Resize();
    {
        GameValues *pV = &GameData::Instance(1)->m_Values;
        // Control Style: each choice carries the "~H Change" help itself.
        Pause_OptionRow(0);
        Pause_OptionLabel(gText.Get(0x510005e3));  // Control Style
        FE::_Push(FE::_Peek()->ObjectAdd(0, 1, new FELayoutScrollGrid(1, 5, 0, 0, 0, 0, (EXW::TextAlign)4)));
        FE::_Marker(1)->MathmosLink(FE::_Peek());
        FE::_SetMarker(0);
        static const unsigned int styles[] = {0x510001fe, 0x510001ff, 0x51000335, 0x510003e8, 0x510003e9};  // 1 - Predator .. 5 - Reaper
        for (int i = 0; i < 5; i++)
            FE::_Peek()
                ->ObjectAdd(0, i, new FEObject((FEObjType::Flags)2, 0x100003a, 0x7000004, i, gText.Get(styles[i]), FE_Blank,
                                               (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4))
                ->m_HelpText = gText.Get(0x51000474);  // ~H Change
        FE::_Peek()->SetVisibleCount(1, 1);
        FE::_Peek()->Bind((FEDataType::Flags)3, &pV->m_ControllerConfig);
        FE::_Peek()->SetEventCallback(Pause_DrawController, 1);
        FE::_Pop();
        FE::_Pop();
        Pause_OptionCheck(1, gText.Get(0x510001fd), 0, (s8 *)&pV->m_Unk1c[3], 0);  // Vibration
        static const unsigned int leanChoices[] = {0, 0x510007fe, 0x510007ff};  // ("Off"), ~M, ~O
        Pause_OptionRow(2);
        FE::_Peek()->m_HelpText = gText.Get(0x51000474);  // ~H Change
        Pause_OptionLabel(gText.Get(0x510007fd));          // 1st Person Lean
        FE::_Push(FE::_Peek()->ObjectAdd(0, 1, new FELayoutScrollGrid(1, 3, 0, 0, 0, 0, (EXW::TextAlign)4)));
        FE::_Marker(1)->MathmosLink(FE::_Peek());
        FE::_Peek()->ObjectAdd(0, 0, new FEObject((FEObjType::Flags)2, 0x100003a, 0x7000004, 0, EXString("Off"), FE_Blank, (EXW::TextAlign)4,
                                                  0, 0, 0, 0, (EXW::TextAlign)4));
        FE::_Peek()->ObjectAdd(0, 1, new FEObject((FEObjType::Flags)2, 0x100003a, 0x7000004, 1, gText.Get(leanChoices[1]), FE_Blank,
                                                  (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        FE::_Peek()->ObjectAdd(0, 2, new FEObject((FEObjType::Flags)2, 0x100003a, 0x7000004, 2, gText.Get(leanChoices[2]), FE_Blank,
                                                  (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        FE::_Peek()->SetVisibleCount(1, 1);
        FE::_Peek()->Bind((FEDataType::Flags)2, &pV->m_ButtonConfig);
        FE::_Pop();
        FE::_Pop();
    }
    FE::_Pop();
    FE::_Push(FE::_Peek()->ObjectAdd(1, 0, new FEObject((FEObjType::Flags)3, 0x1000025, 0x20001e3 /* HT_Entity_XB_controller */, 0xfdffffff,
                                                        FE_Blank, FE_Blank, (EXW::TextAlign)4, 0, 0, 0xc8, 0xda, (EXW::TextAlign)4)));
    FE::_Peek()->m_Flags &= ~8;
    FE::_Peek()->m_Data = 0x5f63696870617267ull;  // "graphic_"
    FE::_Peek()->Script(0)->SetSize(300, 300, 0);
    FE::_Peek()->Script(0)->TweakPosition(EXVector(0.0f, 0.0f, -1.0f, 0.0f));
    FE::_Peek()->SetMathmos((FEObjColour::Flags)1, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80)), 1);
    FE::_Peek()->SetMathmos((FEObjColour::Flags)2, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80)), 1);
    FE::_Peek()->SetEventCallback(Pause_DrawController, 1);
    FE::_Pop();
    FE::_Peek()->ListenerAdd(new FEListener(0x5f5f5f5f5f76616eull, 0xc03, 0, (FEPad::Flags)0xf));  // "nav_____"
    FE::_Peek()->Listener(0x5f5f5f5f5f76616eull)->SetDefaultFocus(FE::_Marker(0));
    FE::_Peek()->SetEventCallback(Pause_Controller, 0);
    FE::_Pop();
    Pause_HelpBarDynamic(tint, 0x51000474);  // ~L/~R Move, ~V Scroll, ~H Change (follows the focus)

    // page: weapons (the weapons the player has, then owned vision modes and the always-present items,
    // with the selected one's icon, name and description)
    FE::_Push(FE::_Peek()->ObjectAdd(5, new FELayoutFlexGrid(2, 1, (EXW::TextAlign)4)));
    FE::_Peek()->m_Data = 0x5f736e6f70616577ull;  // "weapons_"
    {
        int nItems = 0;
        unsigned int nPresets = 0;
        static const unsigned int visionBits[3] = {1, 4, 2};
        FEHud::Instance(1)->GetWeaponPreset(0, 0, &nPresets);
        for (unsigned int i = 0; i < nPresets; i++) {
            FEWeapon w = FEHud::Instance(1)->GetWeaponPreset(0, i, 0);
            if (w.m_Cat != 5 && FEHud::Instance(1)->IsWeaponAvailable(w.m_Hash) && w.m_Text2 > 0x51000000 && w.m_Text2 < 0x510009b9)
                nItems++;
        }
        for (int k = 0; k < 3; k++)
            if (visionBits[k] & XItemHandler_Player::Instance()->m_VisionModeFlags) nItems++;
        nItems += 4;
        FE::_Push(FE::_Peek()->ObjectAdd(0, 0, new FELayoutLayered(2, 0, 0, 0x1d0, 0x122, (EXW::TextAlign)4)));
        FE::_Push(FE::_Peek()->ObjectAdd(0, new FELayoutFixedGrid(2, 1, 0, 0, 0x1d0, 0x122, (EXW::TextAlign)4)));
        FE::_Push(FE::_Peek()->ObjectAdd(0, 0, new FELayoutScrollGrid(nItems, 1, 0, 0, 0, 0, (EXW::TextAlign)4)));
        FE::_SetMarker(2);
        FE::_Peek()->SetVisibleCount(7, 1);
        FE::_Peek()->m_Rect.x += 8;
        FE::_Peek()->m_Rect.w -= 0x10;
        FE::_Peek()->m_Flags |= 2;
        FE::_Peek()->Resize();
        FE::_Peek()->SetSmoothScroll(0);
        int row = -1;
        // carried weapons
        for (unsigned int i = 0; i < nPresets; i++) {
            FEWeapon w = FEHud::Instance(1)->GetWeaponPreset(0, i, 0);
            if (w.m_Cat == 5 || !FEHud::Instance(1)->IsWeaponAvailable(w.m_Hash) || w.m_Text2 <= 0x51000000 || w.m_Text2 >= 0x510009b9)
                continue;
            row++;
            FE::_Push(FE::_Peek()->ObjectAdd(row, 0, new FEObject((FEObjType::Flags)2, 0x100003a, 0x7000004, i, gText.Get(w.m_Text), FE_Blank,
                                                                  (EXW::TextAlign)2, 0, 0, 0, 0, (EXW::TextAlign)4)));
            FE::_Pop();
        }
        // vision modes the player has
        for (int k = 0; k < 3; k++) {
            if (!(visionBits[k] & XItemHandler_Player::Instance()->m_VisionModeFlags)) continue;
            FEWeapon w = FEHud::Instance(1)->GetWeaponPreset(0, k, 0);
            if (w.m_Text2 <= 0x51000000 || w.m_Text2 >= 0x510009b9) continue;
            row++;
            FE::_Push(FE::_Peek()->ObjectAdd(row, 0, new FEObject((FEObjType::Flags)2, 0x100003a, 0x7000004, k, gText.Get(w.m_Text), FE_Blank,
                                                                  (EXW::TextAlign)2, 0, 0, 0, 0, (EXW::TextAlign)4)));
            FE::_Pop();
        }
        // the always-present items
        for (int k = 3; k < 7; k++) {
            FEWeapon w = FEHud::Instance(1)->GetWeaponPreset(0, k, 0);
            if (w.m_Text2 <= 0x51000000 || w.m_Text2 >= 0x510009b9) continue;
            row++;
            FE::_Push(FE::_Peek()->ObjectAdd(row, 0, new FEObject((FEObjType::Flags)2, 0x100003a, 0x7000004, k, gText.Get(w.m_Text), FE_Blank,
                                                                  (EXW::TextAlign)2, 0, 0, 0, 0, (EXW::TextAlign)4)));
            FE::_Pop();
        }
    }
    FE::_Peek()->ListenerAdd(new FEListener(0x5f5f5f5f5f76616eull, 0x401, 0, (FEPad::Flags)0xf));  // "nav_____"
    FE::_Peek()->Listener(0x5f5f5f5f5f76616eull)->SetDefaultFocus(FE::_Peek()->Object(0, 0));
    FE::_Peek()->SetEventCallback(Pause_ViewWeapon, 1);
    FE::_Pop();
    FE::_Push(FE::_Peek()->ObjectAdd(1, 0, new FELayoutFlexGrid(1, 2, (EXW::TextAlign)4)));
    FE::_Push(FE::_Peek()->ObjectAdd(0, 0, new FELayoutFixedGrid(2, 1, 0, 0, 0x48, 0x91, (EXW::TextAlign)4)));
    FE::_Push(FE::_Peek()->ObjectAdd(0, 0, new FEObject((FEObjType::Flags)1, 0, 0, 0xfdffffff, FE_Blank, FE_Blank, (EXW::TextAlign)4, 0, 0,
                                                        0, 0, (EXW::TextAlign)4)));  // icon
    FE::_Peek()->m_Rect.x += 8;
    FE::_Peek()->m_Rect.w -= 0x10;
    FE::_Peek()->m_Flags |= 2;
    FE::_Peek()->Resize();
    FE::_Peek()->m_BlendMode = 2;
    FE::_Peek()->SetMathmos((FEObjColour::Flags)1, FEMathmos(FEHud::TintColour(0)), 1);
    FE::_Pop();
    FE::_Push(FE::_Peek()->ObjectAdd(1, 0, new FEObject((FEObjType::Flags)2, 0x100003a, 0x700000c, 0xfdffffff, FE_Blank, FE_Blank,
                                                        (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4)));  // name
    FE::_Peek()->m_Flags |= 2;
    FE::_Peek()->Resize();
    FE::_Pop();
    FE::_Pop();
    FE::_Push(FE::_Peek()->ObjectAdd(0, 1, new FELayoutLayered(1, 0, 0, 0x178, 0x91, (EXW::TextAlign)4)));
    FE::_Push(FE::_Peek()->ObjectAdd(0, new FEObject((FEObjType::Flags)2, 0x100003a, 0x700000c, 0xfdffffff, FE_Blank, FE_Blank,
                                                     (EXW::TextAlign)0, 0, 0, 0, 0, (EXW::TextAlign)4)));  // description
    FE::_Peek()->m_Flags |= 2;
    FE::_Peek()->Resize();
    FE::_Peek()->m_Flags |= 0x20000;
    FE::_Pop();
    FE::_Pop();
    Pause_ViewWeapon(FE::_Marker(2)->Object(0, 0), (FEObjEvent::Flags)9);
    FE::_Pop();
    FE::_Pop();
    FE::_Push(FE::_Peek()->ObjectAdd(1, new FEObject((FEObjType::Flags)1, 0x1000025, 0x6000054, 0xfdffffff, FE_Blank, FE_Blank,
                                                     (EXW::TextAlign)4, 0, 0, 0x1d8, 2, (EXW::TextAlign)4)));
    FE::_Peek()->SetMathmos((FEObjColour::Flags)1, FEMathmos(FEHud::TintColour(0)), 1);
    FE::_Pop();
    FE::_Pop();
    Pause_HelpBar(tint, 1, s_HelpMoveScroll, 1);
    FE::_Pop();

    // page: an empty last page
    FE::_Push(FE::_Peek()->ObjectAdd(6, new FELayoutLayered(1, 0, 0, 0, 0, (EXW::TextAlign)4)));
    FE::_Peek()->m_Data = 0x5f5f5f5f7473616cull;  // "last____"
    FE::_Pop();
    FE::_Pop();
    FE::_Pop();
    FE::_Pop();
}
