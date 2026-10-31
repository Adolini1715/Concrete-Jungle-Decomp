#include "FEText.h"
#include "EXMemory.h"
#include "EXStdFuncs.h"
#include "EXBaseDisplay.h"
#include "EXBaseGamePad.h"
#include "XAppStandIn.h"
#include "FEManager.h"
#include "FEObject.h"
#include "FEListener.h"
#include "FELevels.h"

extern signed char DBG_bSkipMoviesAndLegal;
extern signed char bBootLoading;
extern unsigned int gSystemCameraUser;
class XGameWnd;
extern XGameWnd **gpGameWnd;
void xbMovieSetVolume(int vol);

unsigned int AttractDelay = 28;
static signed char s_FirstStartup = 1;
static unsigned int s_AttractButtons[16] = { 0x1000,   0x2000,   0x4000, 0x8000, 0x10000, 0x20000, 0x100000, 0x200000,
                                             0x400000, 0x800000, 1,      2,      4,       8,       0x40000,  0x80000 };
FEManager *FEManager::m_pInstance;
static int s_CreditsLine;
static float s_CreditsTimer;

signed char Manager_Language(FEObject *pObj, FEObjEvent::Flags ev) { return 0; }

signed char Manager_RunCredits(FEObject *pObj, FEObjEvent::Flags ev)
{
    switch (ev) {
    case 0x13:
        s_CreditsTimer = 0.0f;
        s_CreditsLine = 0;
        return 1;
    case 0x10:
        if (pObj->Data32() == 0xfdffffff) break;
        pObj->KeyframeAdd(EXPoint(0, EXBaseDisplay::GetDisplay()->Height()), 0.0f);
        return 1;
    case 0x14:
        for (int i = 0; i < 10;) pObj->Object(++i)->KeyframeClear();
        return 1;
    case 0x15:
        s_CreditsTimer = EXBaseApp::GameFrameRateMul() + s_CreditsTimer;
        if (s_CreditsTimer - 30.0f >= 0.0f) {
            pObj->Object(s_CreditsLine + 1)->SetText(EXString("Credits Test"), 0, 0);
            pObj->Object(s_CreditsLine + 1)->KeyframeClear();
            pObj->Object(s_CreditsLine + 1)->KeyframeAdd(EXPoint(0, EXBaseDisplay::GetDisplay()->Height()), 0.0f);
            pObj->Object(s_CreditsLine + 1)->KeyframeAdd(EXPoint(0, -pObj->Rect().h),
                                                         (float)EXBaseDisplay::GetDisplay()->Height());
            s_CreditsLine = (s_CreditsLine + 1) % 10;
            s_CreditsTimer = 0.0f;
        }
        return 1;
    case 0x17:
        return 1;
    }
    return 0;
}

FEManager::FEManager()
{
    m_Attract = 0;
    m_MoviePlaying = 0;
    m_PageState = 0;
    m_PageSubstate = 0;
    m_AttractTime = 0.0f;
    m_PageStateTimer = 0.0f;
    m_PageSubstateTimer = 0.0f;
}

FEManager *FEManager::Instance(signed char create)
{
    if (!m_pInstance && create) m_pInstance = new FEManager;
    return m_pInstance;
}

signed char FEManager::Draw()
{
    gSystemCameraUser = 0;
    return FEWnd::Draw();
}

// NEAR: retail tail-merges the four MovieOpen cases into the case 0 block
signed char Manager_Startup(FEObject *pObj, FEObjEvent::Flags ev)
{
    FEManager *pMgr = FEManager::Instance(1);
    unsigned int state = pMgr->PageState();
    switch (ev) {
    case 0x15:
        if (!pMgr->PageSubstate()) {
            pMgr->SetPageSubstate(1);
            char *pMovie;
            switch (state) {
            case 0: pMovie = "sierra"; break;
            case 1: pMovie = "viv"; break;
            case 2: pMovie = "fox"; break;
            case 3: pMovie = "ecom"; break;
            case 4:
                pMgr->MovieClose();
                pObj->m_Flags |= 0x40;
                return 1;
            case 5:
                s_FirstStartup = 0;
                pMgr->Object()->PageSelect(0x5f5f5f7472617473ui64, 0, 0);
                return 1;
            default: return 1;
            }
            pMgr->MovieOpen(pMovie);
            if (!state) xbMovieSetVolume(0);
            return 1;
        }
        if (!state) {
            if (pMgr->PageSubstate() == 1 && pMgr->PageSubstateTimerSecs() > 0.5f)
                xbMovieSetVolume(XGetGameValues()->m_MovieVolume);
        } else if (state == 4) {
            if (pMgr->PageSubstate() == 1 && pMgr->PageSubstateTimerSecs() > 1.5f) pMgr->SetPageSubstate(2);
        }
        if (!(1.0f >= pMgr->PageStateTimerSecs())) {
            if (state == 4) {
                if (5.0f > pMgr->PageStateTimerSecs()) return 0;
                goto next;
            }
            if (FEListener::QueryButton(0x42000, (FEPad::Flags)0xf, 1)) goto next;
        }
        return 1;
    case 0x13:
        state = (unsigned int)-1;
        if (DBG_bSkipMoviesAndLegal) state = 3;
        bBootLoading = 0;
    case 0x1c:
    next:
        pMgr->MovieClose();
        pMgr->SetPageState(state + 1);
        return 0;
    }
    return 0;
}

void FEManager::ContentUpdate()
{
    TickPage();
    if (m_MoviePlaying) {
        if (m_MovieFinished) {
            m_MoviePlaying = 0;
            m_pObject->ChildSel()->DoEventCallback((FEObjEvent::Flags)0x1c);
        } else {
            FEListener::LastInputReset();
        }
    }
    if (DBG_bSkipMoviesAndLegal && !m_Attract) FEListener::LastInputReset();
    if (!m_Attract && theApp.Time() > FEListener::LastInput() + (float)(AttractDelay * 60)) {
        m_pObject->DoEventCallback((FEObjEvent::Flags)0x26);
        MovieOpen("attrct");
        m_Attract = 1;
        m_AttractTime = 0.0f;
        StateSub(0x40);
        FEListener::EnableInput();
        AttractDelay = 28;
    }
    MovieUpdate();
    if (m_Attract) {
        m_AttractTime += XApp::GameFrameRateMul();
        EXBaseGamePad *pPad = EXBaseGamePad::GetGamePad();
        signed char pressed = 0;
        for (unsigned int i = 0; !pressed && i < 16; i++)
            for (unsigned int j = 0; !pressed && j < 4; j++)
                if (pPad->ButtonReleased(j, s_AttractButtons[i])) pressed = 1;
        if (m_MovieFinished || pressed) {
            m_Attract = 0;
            MovieClose();
            FEListener::LastInputReset();
            m_pObject->DoEventCallback((FEObjEvent::Flags)0x27);
            StateAdd(0x40);
        }
    } else {
        HandleInput();
    }
}

void FEManager::ContentDestroy()
{
    if (!m_ContentDestroyed) {
        ((FEObject *)gpGameWnd)->m_Flags &= ~2;  // TODO stand-in: XGameWnd state flags at 0x70
        FEListener::RemapModeSet((FERemap::Flags)0);
        m_ContentDestroyed = 1;
    }
}

signed char Manager_SelectMap(FEObject *pObj, FEObjEvent::Flags ev)
{
    if (ev != 0x1a) return 0;
    EXString s = gText.Get(0x51000838);
    if (!(s == pObj->m_Text)) {
        unsigned int level = pObj->Data32();
        if (FELevelManager::Instance(1)->LevelFromHash(level, 0)->IsRitual())
            GameData::Instance(1)->m_Values.UnlockRitual(FELevelManager::Instance(1)->RitualBitIndexFromLevelHash(level));
        theApp.GameStatePush((GameState::Flags)0x22, level);
    }
    return 1;
}

#include <stdio.h>
#include "EXHashCodeFile.h"
#include "EXItemEnv.h"
#include "EXBaseSoundManager.h"
static char s_SoundTestText[0x80];

// Sound test page: left/right step through sound hashes and play each; selecting stops them all.
signed char Manager_SoundTest(FEObject *pObj, FEObjEvent::Flags ev)
{
    unsigned int hash = pObj->Data32();
    if (EXItemEnv::GetItemEnv() && EXItemEnv::GetItemEnv()->ViewArray()[0]) {
        // keep the listener at the origin, looking down +Z
        EXVector pos(0.0f, 0.0f, 0.0f, 0.0f);
        EXVector vel(0.0f, 0.0f, 0.0f, 0.0f);
        EXVector front(0.0f, 0.0f, 1.0f, 0.0f);
        EXVector top(0.0f, 1.0f, 0.0f, 0.0f);
        EXVector right(1.0f, 0.0f, 0.0f, 0.0f);
        EXItemEnv::GetItemEnv()->ViewArray()[0]->UpdateSoundListener(pos, vel, front, top, right);
    }
    switch (ev) {
    case 3:
        EXBaseSoundManager::GetSoundManager()->SfxRemoveAll(0);
        return 1;
    case 0xd:
    case 0xe: {
        pObj->SetData((unsigned __int64)0);
        hash += (ev == 0xe) ? 1 : -1;
        pObj->SetData((unsigned __int64)hash);
        int n = 0;
        const char *pName = EXHashCodeFile::Debug_HashCodeName(hash);
        if (pName) n = sprintf(s_SoundTestText, "%s : ", pName);
        sprintf(s_SoundTestText + n, "0x%x", hash);
        pObj->SetText(EXString(s_SoundTestText), 0, 0);
    }
    // fall through
    case 0x1a: {
        EXBaseSoundManager::GetSoundManager()->SfxRemoveAll(0);
        EXVector pos(0.0f, 0.0f, 0.0f, 0.0f);
        EXBaseSoundManager::GetSoundManager()->StartSound(hash, pos, 0.0f, 0.0f, -1, 0);
        return 1;
    }
    case 0x13:
        FEManager::Instance(1)->StateSub(0x8000000);
        return 1;
    }
    return 0;
}

#include "XItemHandler_CameraScript.h"
#include "XItemHandler_Player.h"
#include "FEMainMenu.h"
#include "FEObjectScript.h"
extern XItemHandler_CameraScript *gLastCamScript;  // TODO stand-in (CameraScript.obj)
static FEObject *s_pPressStartObj;  // the "press start" text, set up by ContentCreate

// "Press start" title screen: waits, flashes the prompt, and drops into the front-end 3D file.
signed char Manager_PressStart(FEObject *pObj, FEObjEvent::Flags ev)
{
    FEManager *pMgr = FEManager::Instance(1);
    switch (ev) {
    case 0x13:
        pMgr->SetPageState(0);
        return 0;
    case 0x17:
        if (pMgr->PageState() == 0) {
            if (3.0f > pMgr->PageStateTimerSecs()) return 0;
            theApp.GameStateFlush((GameState::Flags)0x20, XApp::Frontend3DFile());
        }
        return 1;
    case 0x15:
        if (pMgr->PageState() == 1 && pMgr->PageStateTimerSecs() >= 5.0f) {
            pObj->Object(2)->SetMathmos((FEObjColour::Flags)2,
                                        FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), ex::RGBA(0x7f, 0x7f, 0x7f, 0x40), 90.0f,
                                                  (FEWaveform::Flags)1, 0),
                                        1);
            pMgr->SetPageState(1);
            return 1;
        }
        break;
    case 0x27:
        s_pPressStartObj->Script(0)->SetFrame(150.0f);
        s_pPressStartObj->Script(0)->m_State |= 0x80000;
        break;
    }
    return 0;
}

// Delay on the title/main-menu transition: waits for the attract camera, then fades into the main menu.
signed char Manager_MainDelay(FEObject *pObj, FEObjEvent::Flags ev)
{
    FEManager *pMgr = FEManager::Instance(1);
    switch (ev) {
    case 0x13:
        XItemHandler_Player::Instance()->ForceModeChange(0x90004a7);
        return 0;
    case 0x16:
    case 0x17:
        return 0;
    case 0x15:
        break;
    default:
        return 1;
    }
    if (FEMainMenu::IsDisplayStats() && gLastCamScript && gLastCamScript->GetItem()->RefObject() != 0x40004ef)
        gLastCamScript->FinishScript(XApp::Frontend3DFile(), 0x40004ef);  // HT_Script_Front_End_Loop1
    unsigned int sub = pMgr->PageSubstate();
    if (sub == 0) {
        if (FEMainMenu::IsDisplayStats()) {
            if (pMgr->PageSubstateTimerSecs() < 1.0f) return 1;
            pMgr->SetPageSubstate(2);
            pMgr->Object()->PageSelect(0x756e654d6e69614dui64, 0, 0);  // "MainMenu"
            return 1;
        }
        if (pMgr->PageSubstateTimerSecs() >= 9.0f) {
            pMgr->SetPageSubstate(1);
            FEObject *p = pObj->Object(0);
            p->m_Flags |= 0x40;
            p->Script(0)->Play(0, 0);
        } else if (!FEMainMenu::IsDisplayStats() && gLastCamScript &&
                   gLastCamScript->GetItem()->RefObject() == 0x40004ee &&  // HT_Script_Front_End
                   FEListener::QueryButton(0x42000, 0xf, 0)) {
            theApp.ApplyFadeOutIn(2.0f, ex::RGBA(0, 0, 0, 0x80), 0, 1);
            pMgr->SetPageSubstate(2);
        }
    } else if (sub == 2) {
        if (pMgr->PageSubstateTimerSecs() >= 1.0f) {
            gLastCamScript->FinishScript(XApp::Frontend3DFile(), 0x40004ef);
            theApp.ApplyFadeOut(0.0f, ex::RGBA(0, 0, 0, 0x80), 0);
            pMgr->SetPageSubstate(3);
        }
    } else if (sub == 3) {
        theApp.ApplyFadeOut(0.0f, ex::RGBA(0, 0, 0, 0x80), 0);
        if (pMgr->PageSubstateTimerSecs() >= 0.1f) {
            theApp.ApplyFadeIn(1.0f, ex::RGBA(0, 0, 0, 0x80), 0);
            pMgr->Object()->PageSelect(0x756e654d6e69614dui64, 0, 0);
        }
    }
    if (pMgr->PageSubstate() == 1 && pMgr->PageSubstateTimerSecs() >= 2.0f) {
        pMgr->SetPageSubstate(2);
        pMgr->Object()->PageSelect(0x756e654d6e69614dui64, 0, 0);
    }
    return 1;
}

#include "FELayout.h"
#include "FE.h"
#include "FEMainMenu.h"
extern unsigned int gPrevMap_GeoHash;
extern unsigned int gCurrentMap_GeoHash;
#define FE_FRONT_END_FILE 0x100014c  // HT_File_Front_End

// Builds every page of the front-end root: startup/legal, language, press start, level select,
// main-menu delay and the main menu itself, then selects the first one.
void FEManager::ContentCreate()
{
    ((FEObject *)gpGameWnd)->m_Flags |= 2;  // TODO stand-in: XGameWnd state flags at 0x70
    StateAdd(0x60);
    m_Attract = 0;
    m_AttractTime = 0.0f;
    m_MoviePlaying = 0;
    FEListener::LastInputReset();
    FEListener::RemapModeSet((FERemap::Flags)1);
    SetObject(new FELayoutPaged(6, 0, 0, 0, 0, 0, (EXW::TextAlign)4));
    FE::_Init(m_pObject);

    // page 0: startup movies and legal text
    FE::_Push(FE::_Peek()->ObjectAdd(0, new FELayoutLayered(1, 0, 0, 0, 0, (EXW::TextAlign)4)));
    FE::_Peek()->SetData(0x5f70757472617473ui64);  // "startup_"
    FE::_Peek()->SetEventCallback(Manager_Startup, 1);
    FE::_Peek()->SetMathmos((FEObjColour::Flags)2, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80)), 1);
    FE::_Peek()->m_Flags &= ~0x40;
    FE::_Push(FE::_Peek()->ObjectAdd(0, new FELayoutLayered(3, 0, 0, 0, 0, (EXW::TextAlign)4)));
    FE::_Peek()
        ->ObjectAdd(0, new FEObject(FEObjType::OT_SCRIPT2 /* 1 */, 0x100001e, 0x6000234, 0xfdffffff, FE_Blank, FE_Blank,
                                    (EXW::TextAlign)4, 0, 0, 0x280, 0x1e0, (EXW::TextAlign)1))
        ->SetMathmos((FEObjColour::Flags)3, FEMathmos(ex::RGBA(0, 0, 0, 0x80)), 1);
    {
        FEObject *pText = FE::_Peek()->ObjectAdd(
            1, new FEObject((FEObjType::Flags)2, 0x100003a, 0x700000a, 0xfdffffff,
                            (gText.Get(0x5100013d) + gText.Get(0x5100013f)) + "\n" + gText.Get(0x51000815), FE_Blank,
                            (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        pText->m_Flags |= 0x20000;
        pText->MakeSafeRect();
    }
    FE::_Pop();
    FE::_Pop();

    // page 1: language select
    FE::_Push(FE::_Peek()->ObjectAdd(1, new FELayoutFlexGrid(2, 1, (EXW::TextAlign)4)));
    FE::_Peek()->SetData(0x65676175676e616cui64);  // "language"
    FE::_Pop();

    // page 2: press start
    FE::_Push(FE::_Peek()->ObjectAdd(2, new FELayoutLayered(3, 0, 0, 0, 0, (EXW::TextAlign)4)));
    FE::_Peek()->SetData(0x5f5f5f7472617473ui64);  // "start___"
    FE::_Peek()->SetEventCallback(Manager_PressStart, 1);
    FE::_Push(FE::_Peek()->ObjectAdd(1, new FEObject((FEObjType::Flags)4, 0x10000e5, 0x4000388, 0xfdffffff, FE_Blank, FE_Blank,
                                                     (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4)));
    s_pPressStartObj = FE::_Peek();
    FE::_Peek()->Script(0)->SetScale(2.45f);
    FE::_Peek()->Script(0)->Play(0, 0);
    FE::_Peek()->MakeSafeRect();
    FE::_Peek()->SetOrigin(EXPoint(-9, 30), (EXW::TextAlign)0);
    FE::_Peek()->m_Flags |= 0x80000302;
    FE::_Pop();
    FE::_Push(FE::_Peek()->ObjectAdd(2, new FEObject((FEObjType::Flags)2, 0x100003a, 0x7000004, 0xfdffffff, gText.Get(0x5100017e),
                                                     FE_Blank, (EXW::TextAlign)5, 0, 0, 0, 0, (EXW::TextAlign)4)));
    FE::_Peek()->SetMathmos((FEObjColour::Flags)2,
                            FEMathmos(ex::RGBA(0, 0, 0, 0), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 600.0f, (FEWaveform::Flags)1, 1), 1);
    FE::_Peek()->MakeSafeRect();
    FE::_Pop();
    FE::_Pop();

    // page 3: level select
    FE::_Push(FE::_Peek()->ObjectAdd(3, new FELayoutLayered(4, 0, 0, 0, 0, (EXW::TextAlign)4)));
    FE::_Peek()->SetData(0x5f5f5f6c6576656cui64);  // "level___"
    if (gCurrentMap_GeoHash != FE_FRONT_END_FILE) {
        FE::_Push(FE::_Peek()->ObjectAdd(0, new FEObject((FEObjType::Flags)1, 0x100001e, 0x600007b, 0xfdffffff, FE_Blank, FE_Blank,
                                                         (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4)));
        FE::_Pop();
    }
    FE::_Push(FE::_Peek()->ObjectAdd(1, new FEObject((FEObjType::Flags)2, 0x100003a, 0x700000a, 0xfdffffff,
                                                     EXString("Built Jan 10 2005 17:46:57"), FE_Blank, (EXW::TextAlign)1, 0, 0,
                                                     0, 0, (EXW::TextAlign)4)));
    FE::_Peek()->MakeSafeRect();
    FE::_Pop();
    FE::_Push(FE::_Peek()->ObjectAdd(2, new FELayoutScrollGrid(FELevelManager::Instance(1)->m_Count, 1, 0, 0, 0x15e, 0xfa,
                                                               (EXW::TextAlign)2)));
    FE::_Peek()->SetData(0x5f5f5f5f5f70616dui64);  // "map_____"
    for (int i = 0; i < FELevelManager::Instance(1)->m_Count; i++) {
        unsigned int level = FELevelManager::Instance(1)->m_Levels[i].m_Level;
        unsigned int text = FELevelManager::Instance(1)->m_Levels[i].m_Text;
        if (text) {
            FE::_Peek()->ObjectAdd(i, 0,
                                   new FEObject((FEObjType::Flags)2, 0x100003a, 0x7000004, level, gText.Get(text), FE_Blank,
                                                (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        } else {
            FEObject *pLocked = new FEObject((FEObjType::Flags)2, 0x100003a, 0x7000004, level, EXString("Locked"), FE_Blank,
                                             (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4);
            FE::_Peek()->ObjectAdd(i, 0, pLocked);
            pLocked->m_Flags &= ~8;
        }
    }
    FE::_Peek()->SetVisibleCount(5, 1);
    FE::_Peek()->ListenerAdd(new FEListener(0x5f5f5f5f5f76616eui64, 0x401, 0, (FEPad::Flags)0xf));  // "nav_____"
    FE::_Peek()->ListenerChildAdd(FEListener(0x5f5f5f5f5f6e7462ui64, 0x8000, Manager_SelectMap, (FEPad::Flags)0xf));  // "btn_____"
    FE::_Peek()->Listener(0x5f5f5f5f5f76616eui64)->SetFocusByData(FELevelManager::Instance(1)->BaseFromLevel(0x51000188), 0);
    FE::_Peek()->Listener(0x5f5f5f5f5f76616eui64)->SetFocusByData(FELevelManager::Instance(1)->BaseFromLevel(gPrevMap_GeoHash), 0);
    {
        FEObject *pGrid = FE::_Peek();
        pGrid->SetMathmos((FEObjColour::Flags)0,
                          FEMathmos(ex::RGBA(0x7f, 0, 0, 0x80), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 60.0f, (FEWaveform::Flags)1, 0), 1);
        FE::_Peek()->SetMathmos((FEObjColour::Flags)1, FEMathmos(ex::RGBA(0x40, 0, 0, 0x7f)), 1);
    }
    FE::_Pop();
    FE::_Push(FE::_Peek()->ObjectAdd(3, new FEObject((FEObjType::Flags)2, 0x100003a, 0x7000004, 0xfdffffff,
                                                     EXString("~V Scroll  ~A Select"), FE_Blank, (EXW::TextAlign)5, 0, 0, 0, 0,
                                                     (EXW::TextAlign)4)));
    FE::_Peek()->MakeSafeRect();
    FE::_Pop();
    FE::_Pop();

    // page 4: delay before the main menu
    FE::_Push(FE::_Peek()->ObjectAdd(4, new FELayoutLayered(1, 0, 0, 0, 0, (EXW::TextAlign)4)));
    FE::_Peek()->SetData(0x616c65446e69614dui64);  // "MainDela"
    FE::_Peek()->SetEventCallback(Manager_MainDelay, 1);
    FE::_Peek()->MakeSafeRect();
    if (!FEMainMenu::IsDisplayStats()) {
        FE::_Push(FE::_Peek()->ObjectAdd(0, new FEObject((FEObjType::Flags)4, 0x10000e5, 0x40004f9, 0xfdffffff, FE_Blank, FE_Blank,
                                                         (EXW::TextAlign)3, 0, 0, 0, 0, (EXW::TextAlign)3)));
        FE::_Peek()->Script(0)->SetScale(1.5f);
        FE::_Peek()->Script(0)->m_State |= 0x80000;
        FE::_Peek()->m_Flags &= ~0x48;
        FE::_Peek()->SetOrigin(EXPoint(-70, -565), (EXW::TextAlign)0);
        FE::_Peek()->m_Flags |= 0x80000302;
        FE::_Pop();
    }
    FE::_Pop();

    // page 5: the main menu
    FE::_Push(FE::_Peek()->ObjectAdd(5, new FEMainMenu));
    ((FEMainMenu *)FE::_Peek())->Init();
    FE::_Pop();
    FE::_Pop();

    if (gCurrentMap_GeoHash == FE_FRONT_END_FILE)
        m_pObject->PageSelect(0x616c65446e69614dui64, 0, 1);
    else
        m_pObject->PageSelect(0x5f70757472617473ui64, 0, 0);
}
