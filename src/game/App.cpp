#include "FEText.h"
#include "EVD.h"
#include "VisionWipe.h"
#include "XAppStandIn.h"
#include "EXMemory.h"
#include "EXMemHeap.h"
#include "EXDisplay.h"
#include "EXStateSaver.h"
#include "FEWnd.h"
#include "FETopmost.h"
#include "XGamePad.h"
#include "FELevels.h"
#include "XGameWnd.h"
#include "FEHud.h"
#include "FERadar.h"
#include "FEManager.h"
#include "GeoSectionManager.h"
#include "FEPause.h"
#include "XItem.h"
#include "EXItemPhysics.h"
#include "EXPCloth.h"
#include "EXFileSys.h"
#include "EXSoundManager.h"
#include "FESound.h"
#include "XObjViewWnd.h"
#include "XObjViewWnd2.h"

// UpdateGame (the game-state machine) and the inlines only it references are at the end of the file.

void InitTraceFile();
void TrafficHandleMemCompaction();

#include "AIControl.h"
void TrafficDeinit();

#include "EXBaseRenderEnv.h"

// Display with the game's default mode selection (vtable in App.obj).
class XDisplay : public EXDisplay {
public:
    XDisplay() {}
private:
    virtual s8 OpenDefault();
};


signed char bBootLoading = 1;
unsigned int _SDataHeapSize = 0x800000;
unsigned int _LDataHeapSize = 0x2400000;
char *_SoundRootDir = "d:\\Predator\\Binary\\_bin_XB";
unsigned int uiProductID = 0x4212a901;
char szFingerprint[0x100] = "c 2004 Vivendi Universal Games";
unsigned int uiChecksum = 0xa24;
float XApp::m_GlobalGameFrameRate = 1.0f;
unsigned int XApp::m_FrameRateInt = 60;
float XApp::m_FrameRate = 60.0f;
float XApp::m_FrameLength = 1.0f / 60.0f;
float XApp::m_GameFrameRateDiv = 1.0f;
float XApp::m_GameFrameRateMul = 1.0f;
EXFILE FileListdef = {"d:\\Predator\\Binary\\_bin_XB\\BuildData\\Filelist.bin", 0, 0};
signed char bIDontKnowWhyTheRadarCrashesTheFrameAfterPauseButThisFlagShouldGetRoundItHopefully;
unsigned int gSystemCameraUser;
int gNumPlayers;
signed char KillMeImAllWrong;
XApp theApp;

class EXHeapSizes;
void _adjust_heap_sizes(EXHeapSizes &sizes)
{
}

s8 XApp::InitPads()
{
    return XGamePad::Init(new XGamePad) != 0;
}

s8 XApp::InitInstance()
{
    {
        EXString file("d:\\Predator\\Albert\\Predator.ini");
        EXStateSaver::SetStateFile(file);
    }
    return EXBaseApp::InitInstance();
}

inline XGameState *XApp::GameStateCurrent()
{
    XGameState *p = m_GameStates.GetBaseItem();
    XGameState *q;
    while ((q = m_GameStates.GetNextItem()) != 0) p = q;
    return p;
}

void XApp::PlayMovie(char *pName, unsigned int result)
{
    if (strlen(pName) <= 15) {
        strcpy(m_MovieName, pName);
        if (!result && IsRunning()) result = Frontend3DFile();
        m_MovieResult = result;
        m_MovieTime = 0.0f;
        GameStatePush((GameState::Flags)0x29, 0);
    }
}

void XApp::LoadingScreenOpen(unsigned int level)
{
    CancelFade();
    m_Unkcc4.Empty();
    m_LoadingTexture = FELevelManager::Instance(1)->LoadingTextureFromLevel(level);
    FELoading *pLoading = FELoading::Instance(1);
    m_Unkcc4 = Text_Get(pLoading->SetTextureGetText(level, m_LoadingTexture));
    pLoading = FELoading::Instance(1);
    pLoading->SetTextureGetText(level, m_LoadingTexture);
    LoadingScreenDraw();
}

inline int XApp::LoadingScreenBarWidth()
{
    return EXBaseDisplay::GetDisplay()->SafeFrame().w - 0x40;
}

void XApp::LoadingScreenDraw()
{
    int barW = LoadingScreenBarWidth();
    EXRect sf = EXBaseDisplay::GetDisplay()->SafeFrame();
    EXRect r(sf.x + 0x48, sf.y + sf.h - 0x60, barW, 0x60);
    FEGetLoading(1)->DrawSprite((FERenderPass::Flags)4, 0x100004e, 0x6000054, ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 3,
                                       EXRect(0, 0, EXBaseDisplay::GetDisplay()->Width(), EXBaseDisplay::GetDisplay()->Height()),
                                       EXVector2(0.0f, 0.0f), EXVector2(1.0f, 1.0f));
    FEGetLoading(1)->DrawSprite((FERenderPass::Flags)4, 0x100004e, 0x6000054, ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 3,
                                       EXRect(0, 0, EXBaseDisplay::GetDisplay()->Width(), EXBaseDisplay::GetDisplay()->Height()),
                                       EXVector2(0.0f, 0.0f), EXVector2(1.0f, 1.0f));
    if (bBootLoading) {
        FEGetLoading(1)->DrawSprite((FERenderPass::Flags)4, 0x100004e, 0x6000054, ex::RGBA(0, 0, 0, 0x80), 0,
                                           EXRect(0, 0, EXBaseDisplay::GetDisplay()->Width(), EXBaseDisplay::GetDisplay()->Height()),
                                           EXVector2(0.0f, 0.0f), EXVector2(1.0f, 1.0f));
    } else {
        FEGetLoading(1)->DrawSprite((FERenderPass::Flags)4, 0x100004e, m_LoadingTexture, ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 0,
                                           EXRect(0, 0, EXBaseDisplay::GetDisplay()->Width(), EXBaseDisplay::GetDisplay()->Height()),
                                           EXVector2(0.0f, 0.0f), EXVector2(1.0f, 1.0f));
    }
    if (m_Unkcc4.Length() > 0) {
        FEGetLoading(1)->DrawSprite((FERenderPass::Flags)4, 0x100004e, 0x6000054, ex::RGBA(0, 0, 0, 0x40), 0, r,
                                    EXVector2(0.0f, 0.0f), EXVector2(1.0f, 1.0f));
        r.Deflate(8, 0);
        FEGetLoading(1)->DrawText((FERenderPass::Flags)4, 0x100004e, 0x7000004, ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 0, r, m_Unkcc4,
                                  (EXW::TextAlign)2, 0);
    }
    FEGetLoading(1)->DrawSprite((FERenderPass::Flags)4, 0x100004e, 0x600028e, ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 0,
                                       EXRect(sf.x, sf.y + sf.h - 0x50, 0x40, 0x40), EXVector2(0.0f, 0.0f), EXVector2(1.0f, 1.0f));
    FEGetLoading(1)->DrawSprite((FERenderPass::Flags)4, 0x100004e, 0x6000310, ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 2,
                                       EXRect(sf.x, sf.y + sf.h - 0x70, 0x40, 0x10), EXVector2(0.0f, 0.0f), EXVector2(1.0f, 1.0f));
}

void XApp::LoadingCutsceneDraw()
{
    EXRect sf = EXBaseDisplay::GetDisplay()->SafeFrame();
    FEGetLoading(1)->DrawSprite((FERenderPass::Flags)4, 0x1000025, 0x6000310, ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 2,
                                       EXRect(sf.x, sf.y + sf.h - 0x70, 0x40, 0x10), EXVector2(0.0f, 0.0f), EXVector2(1.0f, 1.0f));
}

void XApp::LoadingScreenClose()
{
    if (FELoading::Instance(0)) delete FELoading::Instance(0);
}

void XApp::HandleMemCompaction(unsigned int n)
{
    if (theApp.LevelLoaded()) {
        TrafficHandleMemCompaction();
        gAIControl.HandleMemCompaction();
        gText.HandleMemCompaction();
    }
}

unsigned int XApp::Frontend2DFile()
{
    return 0x100001e;
}

unsigned int XApp::Frontend3DFile()
{
    return 0x100014c;
}

void XApp::MovieOpen()
{
    m_pMovieWnd = new FEWnd;
    m_pMovieWnd->MovieOpen(m_MovieName);
    m_bMoviePlaying = 1;
}

void XApp::CancelFade()
{
    if (m_FadeActive && FELoading::Instance(0)) {
        FELoading *p = FELoading::Instance(0);
        if (p) delete p;
    }
    m_FadeActive = 0;
    m_bHalfwayFade = 0;
    m_bFinishedFade = 1;
    m_FadeTime = 0.0f;
    m_FadeColour = ex::RGBA(0x7f, 0x7f, 0x7f, 0x80);
    m_bFadePaused = 0;
}

inline float qrecipf(float f)
{
    if (f != 0.0f) return 1.0f / f;
    return 0.0f;
}

void XApp::UpdateGameFrameRate()
{
    float rate = EXBaseApp::FrameRate() / m_GlobalGameFrameRate;
    m_FrameRateInt = (unsigned int)(rate + 0.5f);
    m_FrameRate = rate;
    m_FrameLength = qrecipf(rate);
    m_GameFrameRateDiv = rate * (1.0f / 60.0f);
    m_GameFrameRateMul = 60.0f / rate;
}

static int NewGameWnd(int x, int y, int w, int h)
{
    XGameWnd *pWnd = new XGameWnd;
    pWnd->m_Flags = 0x34201d;
    pWnd->SetRect(x, y, w, h);
    pWnd->SetBkColor(ex::RGBA(0, 0, 0, 0x80));
    pWnd->SetOriginToCentre();
    EXBaseDisplay::GetDisplay()->AddWnd(pWnd);
    return 0;
}

void XApp::DoManualClothUpdate()
{
    EXDListItem *pHead = theItemEnv.m_pItemLists[0].m_pHead;
    for (XItem *p = pHead ? (XItem *)((char *)pHead - 4) : 0; p; p = (XItem *)p->NextItem()) {
        // TODO: retail calls XItemPhysics::IsKindOf here; including XItemPhysics.h shifts this unit's $E
        // numbering by two (EXCollisionType::Set(const EXBox &) generates EXBox::operator=), so the
        // engine base view is used.
        if (p->ItemPhysics() && ((EXItemPhysics *)p->ItemPhysics())->IsKindOf(EXRUNTIME_CLASS(EXPCloth))) {
            ((EXItemPhysics *)p->ItemPhysics())->SetPhysicsFlagsOn(0x20);
            ((EXItemPhysics *)p->ItemPhysics())->DoUpdate();
            ((EXItemPhysics *)p->ItemPhysics())->SetPhysicsFlagsOff(0x20);
        }
    }
}

s8 XDisplay::OpenDefault()
{
    EXPresentParams params;
    EXStateSaver saver("RenderMode");
    {
        EXString key("FBWidth");
        params.m_FBWidth = (u16)saver.Restore(key, 0x280);
    }
    {
        EXString key("FBHeight");
        params.m_FBHeight = (u16)saver.Restore(key, 0x1e0);
    }
    params.m_Unk04 = 0x20;
    params.m_Width = params.m_FBWidth;
    params.m_Height = params.m_FBHeight;
    params.m_Unk0a = 0x20;
    params.m_Unk0c = 0x18;
    params.m_Unk0e = 1;
    params.m_Unk10 = 0;
    return Open(params);
}

signed char XApp::MovieUpdate()
{
    m_pMovieWnd->MovieUpdate();
    m_MovieTime += FrameLength();
    return m_pMovieWnd->HasMovieFinished();
}

unsigned int XApp::MovieClose()
{
    m_pMovieWnd->MovieClose();
    if (m_pMovieWnd) delete m_pMovieWnd;
    m_pMovieWnd = 0;
    m_bMoviePlaying = 0;
    return m_MovieResult;
}

extern signed char MemCard_bAutoSave;
void InitWatcherEffects();
// Game accessors ([fi] copies in App.obj).
inline EXDisplay *XGetDisplay() { return (EXDisplay *)EXBaseDisplay::GetDisplay(); }
inline EXFileSys *XGetFileSys() { return EXFileSys::GetFileSys(); }

// NEAR: ~30 bytes differ in the frame-rate/checksum tail: retail stores m_MinFrameRate first and keeps the
// fingerprint sum in edx (register allocation/scheduling).
s8 XApp::InitGame()
{
    {
        EXString dir("d:\\Predator\\Albert\\");
        XGetDisplay()->SetCaptureDir(dir);
    }
    XGetFileSys()->AddFileList(FileListdef);
    FESound::FadeUp(1);
    XGetSoundManager()->FadeMusicUp(1);
    XGetSoundManager()->DeloadSoundBank(0xffffffff);
    XGetWorld()->Initialise();
    EnableGamePause(1);
    SetPauseEnabled(1);
    int viewer = 0;
    EXStringArray &args = XGetApp()->Args();
    for (int i = 0; i < args.Size(); i++) {
        if (args[i] == EXString("viewer")) viewer = 1;
        if (args[i] == EXString("viewer2")) viewer = 2;
    }
    gNumPlayers = 1;
    m_DialogYes = -1;
    m_DialogNo = -1;
    m_DialogType = 0;
    m_Unke28 = 0;
    m_Unkcad = 0;
    m_Time = 0.0f;
    m_FadeActive = 0;
    m_bFinishedFade = 0;
    m_Unkccb = 0;
    m_MissionTimer = 0.0f;
    m_FadeTime = 0.0f;
    m_FadeColour = ex::RGBA(0x7f, 0x7f, 0x7f, 0x80);
    m_Unkcd4 = 0;
    m_bFadePaused = 0;
    m_bLevelLoaded = 0;
    m_PauseMusic = -1;
    m_LoadingTexture = 0x600024d;
    m_bDeathMusic = 0;
    m_bSaveFlag = 0;
    unsigned short w = XGetDisplay()->Width();
    unsigned short h = XGetDisplay()->Height();
    if (viewer == 1) {
        EXWnd *pWnd = new EXWnd;
        pWnd->m_Flags = 0x2002;
        pWnd->SetRect(EXRect(0, 0, w, h));
        pWnd->SetBkColor(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80));
        pWnd->SetOriginToCentre();
        XGetDisplay()->AddWnd(pWnd);
        XObjViewWnd *pView = new XObjViewWnd;
        pView->SetRect(EXRect(0, 0x10, w, h - 0x10));
        pView->SetBkColor(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80));
        pView->SetOriginToCentre();
        XGetDisplay()->AddWnd(pView);
    } else if (viewer == 2) {
        EXWnd *pWnd = new EXWnd;
        pWnd->m_Flags = 0x2002;
        pWnd->SetRect(EXRect(0, 0, w, h));
        pWnd->SetBkColor(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80));
        pWnd->SetOriginToCentre();
        XGetDisplay()->AddWnd(pWnd);
        XObjViewWnd2 *pView = new XObjViewWnd2;
        pView->SetRect(EXRect(0, 0x10, w, h - 0x10));
        pView->SetBkColor(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80));
        pView->SetOriginToCentre();
        XGetDisplay()->AddWnd(pView);
    } else {
        gAIControl.BootInit();
        m_Unke2c = 0;
        switch (gNumPlayers) {
        case 2:
            NewGameWnd(0, 0, w, h / 2);
            NewGameWnd(0, h / 2, w, h / 2);
            break;
        case 3:
            NewGameWnd(0, 0, w / 2, h / 2);
            NewGameWnd(w / 2, 0, w / 2, h / 2);
            NewGameWnd(0, h / 2, w / 2, h / 2);
            break;
        case 4:
            NewGameWnd(0, 0, w / 2, h / 2);
            NewGameWnd(w / 2, 0, w / 2, h / 2);
            NewGameWnd(0, h / 2, w / 2, h / 2);
            NewGameWnd(w / 2, h / 2, w / 2, h / 2);
            break;
        default:
            NewGameWnd(0, 0, w, h);
            break;
        }
        MemCard_bAutoSave = 0;
        m_NextLevelHash = 0;
        InitWatcherEffects();
        EXBaseApp::SetMinFrameRate(EXBaseApp::VideoFrameRate() / 4);
        EXBaseApp::SetFrameRateHoldFrames(EXBaseApp::VideoFrameRate() / 2 / 3);
        EXBaseApp::SetMaxFrameRate(EXBaseApp::VideoFrameRate() / 2);
        unsigned int sum = 0;
        for (char *p = szFingerprint; *p; p++) sum += *p;
        if (sum != uiChecksum) KillMeImAllWrong = 1;
    }
    XGetSoundManager()->LoadSoundBank(0x1ae00001);
    return 1;
}

s8 XApp::InitDisplay()
{
    if (!EXBaseDisplay::Init(new XDisplay)) return 0;
    if (!EXBaseDisplay::GetDisplay()->OpenDefault()) return 0;
    g_RenderEnv.RegisterWatcherVars();
    return 1;
}

s8 XApp::ExitGame()
{
    Text_Shutdown();
    gText.Shutdown();
    TrafficDeinit();
    gAIControl.Shutdown();
    if (FEGetHud(0)) delete FEGetHud(0);
    if (FEGetRadar(0)) delete FEGetRadar(0);
    if (FEGetManager(0)) delete FEGetManager(0);
    if (FEGetPause(0)) delete FEGetPause(0);
    if (FEGetCutscene(0)) delete FEGetCutscene(0);
    if (FEGetDialog(0)) delete FEGetDialog(0);
    if (EVD::Instance(0)) delete EVD::Instance(0);
    if (XGetVisionWipe(0)) delete XGetVisionWipe(0);
    if (FEGetObjectives(0)) delete FEGetObjectives(0);
    if (FEGetLevels(0)) delete FEGetLevels(0);
    if (XGetGameData(0)) delete XGetGameData(0);
    if (FEGetLoading(0)) delete FEGetLoading(0);
    GeoSectionManager::Shutdown();
    return 1;
}

XApp::~XApp()
{
}

XApp::XApp() : m_GameStates(0x80)
{
    m_AppName = "Predator";
    EXMemHeap::EnableAutoHeapCompact(1);
    EXMemHeap::SetCompactCallbacks(0, HandleMemCompaction);
    SetNextLevelHash(0);
    m_bIntroCutscene = 0;
    m_bMoviePlaying = 0;
    m_MovieName[0] = 0;
    m_pMovieWnd = 0;
    m_MovieResult = 0;
    InitTraceFile();
}

// ---- Game-state machine (XApp::UpdateGame) and the inlines only it references. ----
#include "FE.h"
#include "FEObject.h"
#include "FELayout.h"
#include "FELang.h"
#include "FEMainMenu.h"
#include "FEListener.h"
#include "EXHashCodeFile.h"
#include "EXGeoFile.h"
#include "EXParticleSysMgr.h"
#include "EXSwooshSysMgr.h"

signed char MemCard_BootChecks(signed char b);
char *HashToCharStr(unsigned int hash, char *pBuf);
EXString Mission_GetStatusString();
void TrafficUpdate();
void AirTraffic_Update();
void NightAndDay_Update();
void LoadSoundBank();
void Trigger_PostLoad();
extern XGameWnd *gpGameWnd[4];  // TODO stand-in (GameWnd.obj)
extern unsigned int gCurrentMap_GeoHash;
extern EXFixedArray<unsigned char, 290> FlagChannels;

// Flag channels the end-of-mission dialogs answer through.
namespace SpecialFlagChannels {
enum { SFC_NEXT = 0x11c, SFC_RETRY = 0x11e, SFC_QUIT = 0x11f, SFC_EXIT = 0x120, SFC_COUNT = 0x122 };
}

inline void FTRACE(...) {}
inline void DrawPauseText() {}
namespace ex {
inline unsigned int FToU32(float f) { return (unsigned int)f; }
}
inline EXMemCard &XGetMemCard() { return GameData::Instance(1)->MemCard(); }

inline void FEHud::DisablePlayer(unsigned int buttons, int frames)
{
    m_DisablePlayer = frames;
    XItemHandler_Player::Instance()->SetButtonLock(buttons);
}

inline signed char XApp::IsFrontend3D() const { return gCurrentMap_GeoHash == 0x100014c; }

inline void XApp::ApplyFadeIn(float time, ex::RGBA colour, signed char b)
{
    FEWnd::FixupZOrder();
    m_Unkcd4 = b;
    m_FadeActive = 1;
    m_bFinishedFade = 0;
    m_bHalfwayFade = 0;
    m_Unkccb = 0;
    m_FadeMathmos = FEMathmos(colour, ex::RGBA(0, 0, 0, 0), (float)EXBaseApp::NominalGameFrameRate() * time,
                              FEWaveform::WF_LINEAR, 1);
}

inline void XApp::ApplyFadeOut(float time, ex::RGBA colour, signed char b)
{
    FEWnd::FixupZOrder();
    m_Unkcd4 = b;
    m_FadeActive = 1;
    m_bFinishedFade = 0;
    m_bHalfwayFade = 0;
    m_Unkccb = 0;
    m_FadeMathmos = FEMathmos(ex::RGBA(0, 0, 0, 0), colour, (float)EXBaseApp::NominalGameFrameRate() * time,
                              FEWaveform::WF_LINEAR, 1);
}

inline void XApp::GameStateUpdateCurrent()
{
    GameStateCurrent()->Update();
    m_bGameStateChanged = 0;
}

inline void XApp::GameStateFlush(GameState::Flags state, unsigned int data)
{
    XGameState s;
    while (m_GameStates.Remove(&s)) {
    }
    GameStatePush(state, data);
}

inline void XApp::CreateDialogOk(EXString text, int ok)
{
    GameStatePush((GameState::Flags)0x10, 0);
    m_DialogText = text;
    m_DialogYes = m_DialogNo = ok;
    m_DialogType = 3;
}

inline void XApp::CreateDialogYesNo(EXString text, int yes, int no)
{
    GameStatePush((GameState::Flags)0x10, 0);
    m_DialogText = text;
    m_DialogYes = yes;
    m_DialogNo = no;
    m_DialogCancel = -1;
    m_DialogType = 1;
}

inline void XApp::CreateDialogYesNoCancel(EXString text, int yes, int no, int cancel)
{
    GameStatePush((GameState::Flags)0x10, 0);
    m_DialogText = text;
    m_DialogYes = yes;
    m_DialogNo = no;
    m_DialogCancel = cancel;
    m_DialogType = 2;
}

#define FE_TEXT(text, align) \
    new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 0xfdffffff, text, FE_Blank, align, 0, 0, 0, 0, (EXW::TextAlign)4)
#define FE_FRAME() \
    new FEObject(FEObjType::OT_SPRITE, 0x1000025, 0x6000057, 0xfdffffff, FE_Blank, FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0, \
                 (EXW::TextAlign)4)
#define FE_ICON() \
    new FEObject(FEObjType::OT_NONE, 0, 0, 0xfdffffff, FE_Blank, FE_Blank, (EXW::TextAlign)4, 0, 0, 0x10, 0x10, \
                 (EXW::TextAlign)4)

#define SET_DIALOG_CHANNEL(ch) \
    if ((ch) > 0 && (ch) < SpecialFlagChannels::SFC_COUNT) FlagChannels[ch] = 1

// NEAR: structurally complete (same states, calls and inline/out-of-line split; 9360 vs 9312 bytes).
// Differences: register allocation (VC keeps 0 in edi, retail keeps this in esi), a larger frame, and the
// FEMathmos argument of SetMathmos is built in a temporary and copied (retail constructs it in place).
// Retail also tail-merges the dialog builders' SetMarker/Pop and FileLoad sequences.
void XApp::UpdateGame()
{
    if (!gpGameWnd[0]) return;
    EXBaseDisplay::SetActiveWnd(gpGameWnd[0]);
    if (MemCard_BootChecks(0)) return;
    UpdateGameFrameRate();
    m_Time += EXBaseApp::GameFrameRateMul();
    XGetMemCard().Update();
    FEListener::LastInputUpdate();
    XGameState *pState = GameStateCurrent();
    switch (pState->State()) {
    case 2:
        GameControllerRemovedSys::DisconnectCheck((GCRS::Flags)1);
        switch (pState->SubState()) {
        case 3:
            FTRACE("Next level %s (%s)\n", EXHashCodeFile::Debug_HashCodeName(m_NextLevelHash),
                   Text_Get(FELevelManager::Instance(1)->TextFromLevel(m_NextLevelHash)));
            if (!m_NextLevelHash) {
                pState->SetSubState(7);
                goto manager;
            }
            pState->SubStateInc();
        case 4:
            FELang::Instance(1)->ContentCreate();
            pState->SubStateInc();
        case 5:
            FELang::Instance(1)->ContentUpdate();
            break;
        case 6:
            FELang::Instance(1)->Destroy();
            pState->SubStateInc();
        case 7:
        manager:
            FEManager::Instance(1)->ContentCreate();
            pState->SubStateInc();
        case 8:
            FEManager::Instance(1)->ContentUpdate();
            break;
        }
        break;
    case 9:
        if (IsFrontend3D()) {
            GameControllerRemovedSys::UpdateControllers();
        } else if (!FlagChannels[SpecialFlagChannels::SFC_RETRY] && !FlagChannels[SpecialFlagChannels::SFC_EXIT] &&
                   !FlagChannels[SpecialFlagChannels::SFC_QUIT] && !FlagChannels[SpecialFlagChannels::SFC_NEXT]) {
            if (GameControllerRemovedSys::DisconnectCheck((GCRS::Flags)2)) break;
        }
        switch (pState->SubState()) {
        case 10:
            m_GivenPlayerControl = 0;
            if (!m_bIntroCutscene) {
                FEHud::Instance(1)->ContentCreate();
                FEHud::Instance(1)->ContentUpdate();
            }
            ApplyFadeIn(2.0f, ex::RGBA(0, 0, 0, 0x88), 0);
            FESound::FadeUp(0x14);
            XGetSoundManager()->FadeMusicUp(0x14);
            SetMissionInProgress();
            pState->SubStateInc();
        case 11:
            if (!m_GivenPlayerControl && FinishedFade()) {
                m_GivenPlayerControl = 1;
                if (!GetCutscene()) XItemHandler_Player::Instance()->SetPlayerHasControl(1);
            }
            if (PauseEnabled() && !IsFrontend3D()) {
                if (FEListener::QueryButton(0x40000, GameControllerRemovedSys::Controller(), 1)) {
                    GameStatePush((GameState::Flags)0xc, 0);
                    EXMemHeap::ForceCompactHeaps();
                    break;
                }
                m_LevelPlayTimer += GetFrameLength();
            }
            if (FlagChannels[SpecialFlagChannels::SFC_RETRY]) {
                GameStatePush((GameState::Flags)0x20, gCurrentMap_GeoHash);
            } else if (FlagChannels[SpecialFlagChannels::SFC_NEXT]) {
                FEMainMenu::DisplayStats(gCurrentMap_GeoHash, !m_bSaveFlag, 7, 0);
                GameStatePush((GameState::Flags)0x20, 0x100014c);
            } else if (FlagChannels[SpecialFlagChannels::SFC_QUIT]) {
                FEMainMenu::DisplayStats(gCurrentMap_GeoHash, !m_bSaveFlag, 4, 0);
                GameStatePush((GameState::Flags)0x20, 0x100014c);
            } else if (FlagChannels[SpecialFlagChannels::SFC_EXIT]) {
                GameStatePush((GameState::Flags)0x20, 0x100014c);
            } else if (!theApp.GetCutscene() && (IsMissionComplete() || IsMissionFailed() || IsMissionDead())) {
                m_MissionTimer += GameFrameRateMul();
                if (IsMissionComplete()) {
                    FESound::FadeDown(10);
                    SetSaveFlag(1);
                    if (FELevelManager::Instance(1)->IsRitual(gCurrentMap_GeoHash)) {
                        if (FEMainMenu::IngameIsRitualTimeBeaten())
                            CreateDialogYesNo(Mission_GetStatusString() + "\n\n" + Text_Get(0x51000965),
                                              SpecialFlagChannels::SFC_RETRY, SpecialFlagChannels::SFC_QUIT);
                        else
                            CreateDialogYesNo(Mission_GetStatusString() + "\n\n" + Text_Get(0x51000610),
                                              SpecialFlagChannels::SFC_RETRY, SpecialFlagChannels::SFC_QUIT);
                    } else {
                        CreateDialogOk(Mission_GetStatusString(), SpecialFlagChannels::SFC_QUIT);
                    }
                } else if (IsMissionFailed() && MissionTimer() >= 300.0f) {
                    FESound::FadeDown(10);
                    SetSaveFlag(0);
                    if (FELevelManager::Instance(1)->IsRitual(gCurrentMap_GeoHash))
                        CreateDialogYesNoCancel(Mission_GetStatusString() + "\n\n" + Text_Get(0x5100060f),
                                                SpecialFlagChannels::SFC_RETRY, SpecialFlagChannels::SFC_QUIT,
                                                SpecialFlagChannels::SFC_NEXT);
                    else
                        CreateDialogYesNo(Mission_GetStatusString() + "\n\n" + Text_Get(0x510002e5),
                                          SpecialFlagChannels::SFC_RETRY, SpecialFlagChannels::SFC_QUIT);
                } else if (IsMissionDead()) {
                    FESound::FadeDown(10);
                    SetSaveFlag(0);
                    if (FELevelManager::Instance(1)->IsRitual(gCurrentMap_GeoHash))
                        CreateDialogYesNoCancel(Text_Get(0x51000955) + "\n\n" + Text_Get(0x5100060f),
                                                SpecialFlagChannels::SFC_RETRY, SpecialFlagChannels::SFC_QUIT,
                                                SpecialFlagChannels::SFC_NEXT);
                    else if (FELevelManager::Instance(1)->IsTraining(gCurrentMap_GeoHash))
                        CreateDialogYesNo(Text_Get(0x51000964) + "\n\n" + Text_Get(0x510002e5),
                                          SpecialFlagChannels::SFC_RETRY, SpecialFlagChannels::SFC_QUIT);
                    else
                        CreateDialogYesNo(Text_Get(0x51000346) + "\n\n" + Text_Get(0x510002e5),
                                          SpecialFlagChannels::SFC_RETRY, SpecialFlagChannels::SFC_QUIT);
                }
            } else if (IsMissionInProgress()) {
                m_MissionTimer += GameFrameRateMul();
            }
            gpGameWnd[0]->UpdateDistrictLoading();
            theItemEnv.UpdateItems_InterObjCollision();
            if (!m_bIntroCutscene) {
                TrafficUpdate();
                AirTraffic_Update();
            }
            XGetWorld()->UpdateItems_UpdateCode();
            theItemEnv.UpdateItems_Physics();
            theItemEnv.UpdateItems_Collision();
            if (!m_bIntroCutscene) {
                gAIControl.Update();
                XItemHandler_Player::Instance()->PlayerPostUpdate();
                XGetCamera(0)->CameraPostUpdate();
                FEHud::Instance(1)->MaskUpdate();
            }
            if (!m_bCutscene) ((XGamePad *)EXBaseGamePad::GetGamePad())->UpdateRumble();
            NightAndDay_Update();
            if (XItemHandler_Player::Instance()->GetHealth() <= 0.0f && !m_bDeathMusic) {
                XGetWorld()->StartMusic(0x1b000042, 0x14, 2, 2);
                m_bDeathMusic = 1;
            }
            if (!m_bIntroCutscene && !m_bCutscene) {
                if (!XGetGameValues()->m_Unk1c[4] && !FELevelManager::Instance(1)->IsRitual(gCurrentMap_GeoHash) &&
                    gCurrentMap_GeoHash != 0x10000b4 && XItemHandler_Player::Instance()->GetHealth() <= 50.0f) {
                    Text_PrintMsg(Text_Get(0x5100033a), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 2, -1, 0, 0);
                    XGetGameValues()->m_Unk1c[4] = 1;
                } else if (!XGetGameValues()->m_Unk1c[5] &&
                           !FELevelManager::Instance(1)->IsTraining(gCurrentMap_GeoHash) &&
                           gCurrentMap_GeoHash != 0x100006a &&
                           XItemHandler_Player::Instance()->GetCloakEnergy() <= 30.0f) {
                    Text_PrintMsg(Text_Get(0x510009b3), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 2, -1, 0, 0);
                    XGetGameValues()->m_Unk1c[5] = 1;
                }
            }
            if (!m_bIntroCutscene) FEHud::Instance(1)->ContentUpdate();
            if (IsFrontend3D()) FEManager::Instance(1)->ContentUpdate();
            bIDontKnowWhyTheRadarCrashesTheFrameAfterPauseButThisFlagShouldGetRoundItHopefully = 0;
            break;
        }
        break;
    case 0xc:
        if (GameControllerRemovedSys::DisconnectCheck((GCRS::Flags)3)) break;
        switch (pState->SubState()) {
        case 0xd:
            DrawPauseText();
            FEPause::Instance(1)->ContentCreate();
            pState->SubStateInc();
        case 0xe:
            FEPause::Instance(1)->ContentUpdate();
            if (!FEListener::QueryButton(0x40000, GameControllerRemovedSys::Controller(), 1)) break;
            pState->SubStateInc();
        case 0xf:
            FEPause::Instance(1)->ContentDestroy();
            GameStatePop();
            bIDontKnowWhyTheRadarCrashesTheFrameAfterPauseButThisFlagShouldGetRoundItHopefully = 1;
            break;
        }
        break;
    case 0x10: {
        FEObject *pButtons = FEDialog::Instance(1)->m_pObject->Object(0);
        switch (pState->SubState()) {
        case 0x11: {
            ((XGamePad *)EXBaseGamePad::GetGamePad())->StopAllRumble();
            XGetSoundManager()->StartSound(0x1af0015b);
            FESound::FadeDown(10);
            FEListener::RemapModeSet((FERemap::Flags)1);
            FE::_Init(FEDialog::Instance(1)->m_pObject);
            FE::_Push(FE::_Peek()->ObjectAdd(0, new FELayoutLayered(2, 0, 0, 0, 0, (EXW::TextAlign)4)));
            switch (m_DialogType) {
            case 1:
            case 2:
                FE::_Push(FE::_Peek()->ObjectAdd(1, FE_TEXT(m_DialogText, (EXW::TextAlign)4)));
                FE::_Peek()->LockTextRect(0x168, 0, 0, 0);
                FE::_Peek()->StateAdd(0x40000);
                FE::_Peek()->SetMathmos(FEObjColour::OC_NORMAL, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80)), 1);
                FE::_SetMarker(0);
                FE::_Pop();
                break;
            case 3:
                FE::_Push(FE::_Peek()->ObjectAdd(1, new FELayoutFlexGrid(3, 1, (EXW::TextAlign)4)));
                if (m_bDialogLeftAlign)
                    FE::_Peek()->ObjectAdd(0, 0, FE_TEXT(m_DialogText, (EXW::TextAlign)2))->LockTextRect(0x168, 0, 0, 0)->StateAdd(0x40000);
                else
                    FE::_Peek()->ObjectAdd(0, 0, FE_TEXT(m_DialogText, (EXW::TextAlign)4))->LockTextRect(0x168, 0, 0, 0)->StateAdd(0x40000);
                FE::_Peek()->ObjectAdd(1, 0, FE_ICON());
                FE::_Peek()->ObjectAdd(2, 0, FE_TEXT(Text_Get(0x5100011a), (EXW::TextAlign)4))->LockTextRect(0x168, 0, 0, 0)->StateAdd(0x40000);
                FE::_SetMarker(0);
                FE::_Pop();
                break;
            case 4:
                FE::_Push(FE::_Peek()->ObjectAdd(1, new FELayoutFlexGrid(3, 1, (EXW::TextAlign)4)));
                FE::_Peek()->ObjectAdd(0, 0, FE_TEXT(m_DialogText, (EXW::TextAlign)4))->LockTextRect(0x168, 0, 0, 0)->StateAdd(0x40000);
                FE::_Peek()->ObjectAdd(1, 0, FE_ICON());
                FE::_Peek()->ObjectAdd(2, 0, FE_TEXT(Text_Get(0x510004b5), (EXW::TextAlign)4))->LockTextRect(0x168, 0, 0, 0)->StateAdd(0x40000);
                FE::_SetMarker(0);
                FE::_Pop();
                break;
            }
            FE::_Push(FE::_Peek()->ObjectAdd(0, FE_FRAME()));
            FE::_Peek()->SetBorderWidth(0x10);
            FE::_Peek()->SetRect(FE::_Marker(0)->Rect());
            FE::_Peek()->Inflate(0x10, 0x10);
            FE::_Peek()->StateAdd(2);
            FE::_Pop();
            FE::_Pop();
            FE::_Pop();
            m_DialogOpen = 1;
            pState->SubStateInc();
            m_bDialogLeftAlign = 0;
        }
        case 0x12:
            if (GameControllerRemovedSys::DisconnectCheck((GCRS::Flags)5)) {
                if (pButtons) pButtons->StateSub(0x40);
                break;
            }
            if (pButtons) pButtons->StateAdd(0x40);
            switch (m_DialogType) {
            case 3:
                if (!FEListener::QueryButton(0x2000, GameControllerRemovedSys::Controller(), 1)) break;
                XGetSoundManager()->StartSound(0x1af0015c);
                SET_DIALOG_CHANNEL(m_DialogYes);
                pState->SubStateInc();
                break;
            case 4:
                if (!FEListener::QueryButton(0x8000, GameControllerRemovedSys::Controller(), 1)) break;
                XGetSoundManager()->StartSound(0x1af0015a);
                SET_DIALOG_CHANNEL(m_DialogYes);
                pState->SubStateInc();
                break;
            case 1:
            case 2:
                if (FEListener::QueryButton(0x2000, GameControllerRemovedSys::Controller(), 1)) {
                    XGetSoundManager()->StartSound(0x1af0015c);
                    SET_DIALOG_CHANNEL(m_DialogYes);
                    pState->SubStateInc();
                } else if (FEListener::QueryButton(0x8000, GameControllerRemovedSys::Controller(), 1)) {
                    XGetSoundManager()->StartSound(0x1af0015c);
                    SET_DIALOG_CHANNEL(m_DialogNo);
                    pState->SubStateInc();
                } else if (m_DialogType == 2 &&
                           FEListener::QueryButton(0x4000, GameControllerRemovedSys::Controller(), 1)) {
                    XGetSoundManager()->StartSound(0x1af0015c);
                    SET_DIALOG_CHANNEL(m_DialogCancel);
                    pState->SubStateInc();
                }
                break;
            }
            break;
        case 0x13:
            FESound::FadeUp(10);
            XGetSoundManager()->StartSound(0x1af00159);
            FEDialog::Instance(1)->m_DeleteNeeded = 1;
            FEHud::Instance(1)->DisablePlayer(0xffffff7f, 10);
            m_DialogOpen = 0;
            FEListener::RemapClear();
            GameStatePop();
            break;
        }
        gpGameWnd[0]->UpdateDistrictLoading();
        FEHud::Instance(1)->ContentUpdate();
        break;
    }
    case 0x14:
        if (GameControllerRemovedSys::DisconnectCheck((GCRS::Flags)4)) break;
        if (FEListener::QueryButton(0x40000, GameControllerRemovedSys::Controller(), 1) &&
            !FEHud::Instance(1)->IsWeaponSelectorOpen()) {
            GameStatePush((GameState::Flags)0xc, 0);
            break;
        }
        gpGameWnd[0]->UpdateDistrictLoading();
        FEHud::Instance(1)->ContentUpdate();
        break;
    case 0x16:
        GameControllerRemovedSys::ReconnectCheck();
        if (!GameControllerRemovedSys::Status()) break;
        gpGameWnd[0]->UpdateDistrictLoading();
        FEHud::Instance(1)->ContentUpdate();
        break;
    case 0x18:
        switch (pState->SubState()) {
        case 0x19:
            ApplyFadeOut(5.0f, ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 1);
            pState->SubStateInc();
        case 0x1a:
            FEHud::Instance(1)->ContentUpdate();
            if (FinishedFade()) {
                GameStatePop();
                GameStatePush((GameState::Flags)0x20, 0x100014c);
            }
            break;
        }
        break;
    case 0x1b: {
        FEObject *pButtons = FEDialog::Instance(1)->m_pObject->Object(0);
        if (GameControllerRemovedSys::DisconnectCheck((GCRS::Flags)5)) {
            if (pButtons) pButtons->StateSub(0x40);
            break;
        }
        if (pButtons) pButtons->StateAdd(0x40);
        switch (pState->SubState()) {
        case 0x1c:
            XGetSoundManager()->StartSound(0x1af0015b);
            FE::_Init(FEDialog::Instance(1)->m_pObject);
            FE::_Push(FE::_Peek()->ObjectAdd(0, new FELayoutLayered(2, 0, 0, 0, 0, (EXW::TextAlign)4)));
            FE::_Push(FE::_Peek()->ObjectAdd(1, FE_TEXT(Text_Get(0x510001e3), (EXW::TextAlign)4)));
            FE::_Peek()->LockTextRect(0x168, 0, 0, 0);
            FE::_Peek()->SetMathmos(FEObjColour::OC_NORMAL, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80)), 1);
            FE::_SetMarker(0);
            FE::_Pop();
            FE::_Push(FE::_Peek()->ObjectAdd(0, FE_FRAME()));
            FE::_Peek()->SetBorderWidth(0x10);
            FE::_Peek()->SetRect(FE::_Marker(0)->Rect());
            FE::_Peek()->Inflate(0x10, 0x10);
            FE::_Peek()->StateAdd(2);
            FE::_Pop();
            FE::_Pop();
            FE::_Pop();
            pState->SubStateInc();
            break;
        case 0x1d:
            if (FEListener::QueryButton(0x2000, GameControllerRemovedSys::Controller(), 1) ||
                FEListener::QueryButton(0x40000, GameControllerRemovedSys::Controller(), 1)) {
                XGetSoundManager()->StartSound(0x1af0015c);
                XGetSoundManager()->StartSound(0x1af00159);
                unsigned int level = pState->Data();
                if (level == 0x100014c) {
                    FEMainMenu::DisplayStats(0, 1, 1, 1);
                } else if (level == 0x100001e) {
                    FEMainMenu::DisplayStats(gCurrentMap_GeoHash, 1, 0, 0);
                    level = 0x100014c;
                } else if (level == gCurrentMap_GeoHash) {
                    FEMainMenu::ResetUpgradesFromQuit();
                }
                GameStatePop();
                GameStatePush((GameState::Flags)0x20, level);
                LoadingScreenOpen(level);
                FEDialog::Instance(1)->m_DeleteNeeded = 1;
                FEPause::Instance(1)->m_DeleteNeeded = 1;
            } else if (FEListener::QueryButton(0x8000, GameControllerRemovedSys::Controller(), 1) ||
                       FEListener::QueryButton(0x80000, GameControllerRemovedSys::Controller(), 1)) {
                XGetSoundManager()->StartSound(0x1af0015c);
                XGetSoundManager()->StartSound(0x1af00159);
                GameStatePop();
                FEDialog::Instance(1)->m_DeleteNeeded = 1;
            }
            break;
        }
        gpGameWnd[0]->UpdateDistrictLoading();
        break;
    }
    case 0x1e: {
        GameData::Instance(1)->SettingsStore();
        unsigned int level = pState->Data();
        FTRACE("Load level %s (%s)\n", EXHashCodeFile::Debug_HashCodeName(level),
               HashToCharStr(FELevelManager::Instance(1)->TextFromLevel(level), 0));
        GameStatePop();
        GameStatePush((GameState::Flags)0x24, level);
        LoadingScreenOpen(level);
        break;
    }
    case 0x20:
        if (pState->Data() == gCurrentMap_GeoHash) {
            XGetGameValues()->m_Stats.m_Unk0[0] += ex::FToU32(theApp.LevelPlayTimer());
            FEMainMenu::ResetUpgradesFromQuit();
        }
    case 0x22: {
        FELang *pLang = FEGetLang(0);
        if (pLang) pLang->Destroy();
        m_Unke28 = pState->Data();
        FTRACE("Load level %s (%s)\n", EXHashCodeFile::Debug_HashCodeName(m_Unke28),
               HashToCharStr(FELevelManager::Instance(1)->TextFromLevel(m_Unke28), 0));
        if (pState->State() == 0x22) {
            GameControllerRemovedSys::ControllerSet(FEListener::LastPad());
            FEManager::Instance(1)->ContentDestroy();
        }
        GameData::Instance(1)->SettingsInvalidateLevel();
        unsigned int level = pState->Data();
        GameStatePop();
        GameStatePush((GameState::Flags)0x24, level);
        m_bIntroCutscene = FELevelManager::Instance(1)->IsIntroCutscene(level);
        LoadingScreenOpen(level);
        break;
    }
    case 0x24:
        GameControllerRemovedSys::UpdateControllers();
        switch (pState->SubState()) {
        case 0x25:
            gpGameWnd[0]->LevelDeinit(0);
            gpGameWnd[0]->LevelPreLoad();
            m_nLoadFiles = 0;
            m_Unkcb4 = 0;
            m_LoadBarPos = 0.0f;
            m_LoadTotal = 0;
            m_LoadDone = 0;
            m_LoadIndex = 0;
            m_bDeathMusic = 0;
#define LOAD_LEVEL(h)                                                                           \
    m_LoadFiles[m_nLoadFiles].m_pFile = gpGameWnd[0]->LevelLoad(h, &m_LoadFiles[m_nLoadFiles].m_Size); \
    m_nLoadFiles++;
#define LOAD_FILE(h)                                                                            \
    m_LoadFiles[m_nLoadFiles].m_pFile = gpGameWnd[0]->FileLoad(h, &m_LoadFiles[m_nLoadFiles].m_Size); \
    m_nLoadFiles++;
            if (pState->Data() == 0x100001e) {
                LOAD_LEVEL(pState->Data());
                LOAD_FILE(0x1000076);
                LOAD_FILE(0x100003a);
                LOAD_FILE(0x1000025);
                LOAD_FILE(0x100003b);
                LOAD_FILE(0x10000e5);
            } else {
                bBootLoading = 0;
                LOAD_LEVEL(pState->Data());
                LOAD_FILE(0x1000076);
                LOAD_FILE(0x100003a);
                LOAD_FILE(0x1000025);
                LOAD_FILE(0x100003b);
                LOAD_FILE(0x1000005);
                if (!m_bIntroCutscene) {
                    LOAD_FILE(0x1000024);
                    LOAD_FILE(0x1000016);
                    LOAD_FILE(0x1000013);
                    LOAD_FILE(0x100000f);
                    LOAD_FILE(0x100004c);
                    LOAD_FILE(0x10000e3);
                    LOAD_FILE(0x100004f);
                    LOAD_FILE(0x100001d);
                }
                if (pState->Data() == 0x100014c) {
                    LOAD_FILE(0x100001e);
                    LOAD_FILE(0x10000e5);
                }
            }
            for (int i = 0; i < m_nLoadFiles; i++) m_LoadTotal += m_LoadFiles[i].m_Size;
            pState->SubStateInc();
        case 0x26: {
            int i = m_LoadIndex;
            if (m_LoadFiles[i].m_pFile->IsLoaded()) {
                m_LoadBarPos = 0.0f;
                m_LoadDone += m_LoadFiles[i].m_Size;
                m_LoadIndex = i + 1;
            } else {
                float f = (float)m_LoadFiles[i].m_Size / (float)m_LoadTotal * (float)LoadingScreenBarWidth();
                m_LoadBarPos = min(m_LoadBarPos, f);
            }
            LoadingScreenDraw();
            if (m_LoadIndex < m_nLoadFiles) break;
            pState->SubStateInc();
        }
        case 0x27:
            SetCutscene(0);
            LoadingScreenDraw();
            LoadSoundBank();
            m_Unke2c++;
            Text_LoadLanguage();
            FESound::Init();
            FEListener::Initialise();
            if (gCurrentMap_GeoHash == 0x100001e) {
                LoadingScreenClose();
                FESound::FadeUp(1);
                XGetSoundManager()->FadeMusicUp(1);
                GameStateFlush((GameState::Flags)2, 0);
            } else {
                gpGameWnd[0]->LevelPostLoad();
                GameData::Instance(1)->SettingsRestore(0);
                gpGameWnd[0]->WaitForIslandInit();
                Trigger_PostLoad();
                if (IsFrontend3D())
                    FEManager::Instance(1)->ContentCreate();
                else
                    m_LevelPlayTimer = 0.0f;
                pState->SubStateInc();
            }
            XGetSoundManager()->SetMusicVolume(XGetGameValues()->m_MusicVolume);
            XGetSoundManager()->SetSfxVolume(XGetGameValues()->m_MovieVolume);
            FEListener::LastInputReset();
            break;
        case 0x28:
            LoadingScreenDraw();
            gpGameWnd[0]->UpdateDistrictLoading();
            if (gpGameWnd[0]->WaitForIsland()) {
                m_Unkcad = 0;
                gpGameWnd[0]->LevelPreRun();
                GameStateFlush((GameState::Flags)9, 0);
            }
            break;
        }
    case 0x29:
        switch (pState->SubState()) {
        case 0x2a:
            if (m_MovieResult) gpGameWnd[0]->LevelDeinit(1);
            MovieOpen();
            pState->SubStateInc();
        case 0x2b:
            FEListener::LastInputReset();
            if (!MovieUpdate()) {
                if (!(m_MovieTime >= 3.0f)) break;
                if (!FEListener::QueryButton(0x42000, (FEPad::Flags)0xf, 1)) break;
            }
            pState->SubStateInc();
        case 0x2c: {
            unsigned int result = MovieClose();
            if (!result) {
                GameStatePop();
            } else if (gCurrentMap_GeoHash == 0x100006a && result == FELevelManager::Instance(1)->LevelFromIndex(1)) {
                FEMainMenu::DisplayStats(result, 0, 0, 0);
            } else {
                GameStateFlush((GameState::Flags)0x20, result);
            }
            break;
        }
        }
        break;
    }

    GameState::Flags state = GameStateCurrent()->State();
    if ((state != 2 && state != 9 && state != 0x24) || GameStateCurrent()->State() != pState->State())
        DoManualClothUpdate();
    if (GameStateChanged()) {
        gpGameWnd[0]->ParticleSysMgr()->PauseAllSystems(GameStateCurrent()->State() != 9);
        gpGameWnd[0]->SwooshSysMgr()->PauseAllSystems(GameStateCurrent()->State() != 9);
        if (GameStateCurrent()->State() == 0xc && !m_Unkcad) {
            m_Unkcad = 1;
            m_PauseMusic = theItemEnv.MusicHash();
            FESound::FadeDown(m_bCutscene ? 1 : 10);
            XGetSoundManager()->StartMusic(0x1b000040, 10, 10, 1, 1);
        } else if (GameStateCurrent()->State() == 9 && m_Unkcad) {
            m_Unkcad = 0;
            FESound::FadeUp(10);
            XGetSoundManager()->StartMusic(m_PauseMusic ? m_PauseMusic : 0x1b00001e, 10, 10, 1, 0);
        }
        if (GameStateCurrent()->State() == 9) {
            for (int h = 0x1000000; h < 0x100019c; h++) {
                EXGeoFile *pFile = EXGeoFile::GetGeoFile(h);
                if (pFile && pFile->IsLoaded()) pFile->GeoHeader()->SetAnimTextureSpeedScale(1.0f);
            }
        } else {
            for (int h = 0x1000000; h < 0x100019c; h++) {
                if (h == 0x100003a || h == 0x100003b || h == 0x100001e || h == 0x100004e || h == 0x10000e5) continue;
                EXGeoFile *pFile = EXGeoFile::GetGeoFile(h);
                if (pFile && pFile->IsLoaded()) pFile->GeoHeader()->SetAnimTextureSpeedScale(0.0f);
            }
        }
    }
    if (m_FadeActive) {
        ex::RGBA colour;
        if (m_Unkcd4 && GameStateCurrent()->State() != 9)
            colour = m_FadeMathmos.Colour();
        else
            colour = m_FadeMathmos.Update();
        FEGetLoading(1)->DrawSprite((FERenderPass::Flags)4, 0x1000025, 0x6000054, colour, 0,
                                    EXRect(0, 0, EXBaseDisplay::GetDisplay()->Width(), EXBaseDisplay::GetDisplay()->Height()),
                                    EXVector2(0.0f, 0.0f), EXVector2(1.0f, 1.0f));
        if (m_FadeMathmos.IsFinished() && !m_bFadePaused) {
            if (m_Unkccb) {
                ApplyFadeIn(m_FadeTime * 0.5f, m_FadeColour, m_Unkcd4);
                m_bHalfwayFade = 1;
            } else {
                if (!m_bFinishedFade) {
                    m_bFinishedFade = 1;
                    m_FadeActive = 0;
                } else {
                    CancelFade();
                }
                FEGetLoading(1)->m_DeleteNeeded = 1;
            }
        }
    }
    if (GameStateCurrent()->State() == 0xc || GameStateCurrent()->State() == 0x14 ||
        GameStateCurrent()->State() == 0x10) {
        gpGameWnd[0]->SetFocusBlurDistance(0.0f);
        gpGameWnd[0]->SetFocusBlurFactor(10);
        gpGameWnd[0]->Effects().BitSet(4);
    }
    theItemEnv.UpdateItems_LastFrame();
    GameData::Instance(1)->DataTransfer();
    GeoSectionManager::Update();
    GameStateUpdateCurrent();
    theItemEnv.ProcessEnvViews();
}
