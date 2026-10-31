#include "EXBaseApp.h"
#include "EXMemory.h"
#include "EXBaseDisplay.h"
#include "EXDisplay.h"
#include "EXBaseGamePad.h"
#include "EXGamePad.h"
#include "EXBaseSoundManager.h"
#include "EXSoundManager.h"
#include "EXKeyboard.h"
#include "EXMouse.h"
#include "EXFileSys.h"
#include "EXMemHeap.h"
#include "EXWatcher.h"
#include "EXDebugWnd.h"
#include "EXBaseRenderEnv.h"
#include "EXItemEnvStandIn.h"


EXFixedArray<const char *, 16> g_DebugMessagesStart = {
    "-- ENGINE : INFO :", "** ENGINE : WARNING :", "** ENGINE : ERROR :", "** ENGINE : FATAL :",
    "-- GAME : INFO :", "** GAME : WARNING :", "** GAME : ERROR :", "** GAME : FATAL :",
    "-EI:", "*EW:", "*EE:", "*EF:", "-GI:", "*GW:", "*GE:", "*GF:"};
EXFixedArray<const char *, 16> g_DebugMessagesEnd = {
    " --", " **", " **", " **", " --", " **", " **", " **", "", "", "", "", "", "", "", ""};
EXFixedArray<signed char, 2> g_bTraceInfo = {1, 1};
EXFixedArray<signed char, 2> g_bTraceShort = {1, 1};

s8 EXBaseApp::m_UseVarRateScheduler = 1;
unsigned int EXBaseApp::m_NominalGameFrameRate = 60;
unsigned int EXBaseApp::m_NominalEngineFrameRate = 60;
unsigned int EXBaseApp::m_VideoFrameRate = 60;
unsigned int EXBaseApp::m_MinFrameRate = 30;
unsigned int EXBaseApp::m_MaxFrameRate = 60;
unsigned int EXBaseApp::m_FrameRateInt = 60;
float EXBaseApp::m_FrameRate = 60.0f;
float EXBaseApp::m_FrameLength = 1.0f / 60.0f;
float EXBaseApp::m_GameFrameRateDiv = 1.0f;
float EXBaseApp::m_GameFrameRateMul = 1.0f;
float EXBaseApp::m_EngineFrameRateDiv = 1.0f;
float EXBaseApp::m_EngineFrameRateMul = 1.0f;
float EXBaseApp::m_RenderFrameScale = 1.0f;
float EXBaseApp::m_GameRenderFrameRateDiv = 1.0f;
float EXBaseApp::m_GameRenderFrameRateMul = 1.0f;
float EXBaseApp::m_EngineRenderFrameRateDiv = 1.0f;
float EXBaseApp::m_EngineRenderFrameRateMul = 1.0f;
unsigned int EXBaseApp::m_FrameRateHoldFrames;
EXBaseApp *EXBaseApp::m_pTheApp;

EXBaseApp::~EXBaseApp()
{
}

s8 EXBaseApp::InitInstance()
{
    if (!InitWatcher()) return 0;
    if (!InitMallocMemory()) return 0;
    if (!InitDisplay()) return 0;
    EXBaseDisplay::GetDisplay()->RegisterWatcherVars();
    if (!InitPads()) return 0;
    if (!InitKeyboard()) return 0;
    if (!InitMouse()) return 0;
    if (!InitSoundManager()) return 0;
    if (!InitFileSystem()) return 0;
    if (!InitDebugWndMgr()) return 0;
    return 1;
}

int EXBaseApp::ExitInstance()
{
    ShutdownItemSystem();
    ShutdownFileSystem();
    ShutdownSoundManager();
    ShutdownMouse();
    ShutdownKeyboard();
    ShutdownPads();
    ShutdownDisplay();
    ShutdownWatcher();
    ShutdownDebugWndMgr();
    return 0;
}

s8 EXBaseApp::InitDisplay()
{
    if (!EXBaseDisplay::Init(new EXDisplay)) return 0;
    if (!EXBaseDisplay::GetDisplay()->OpenDefault()) return 0;
    g_RenderEnv.RegisterWatcherVars();
    return 1;
}

s8 EXBaseApp::InitPads()
{
    if (!EXBaseGamePad::Init(new EXGamePad)) return 0;
    return ((EXGamePad *)EXBaseGamePad::GetGamePad())->OpenPads() != 0;
}

s8 EXBaseApp::InitSoundManager()
{
    if (!EXBaseSoundManager::Init(new EXSoundManager)) return 0;
    if (!EXBaseSoundManager::GetSoundManager()->OpenSoundManager()) return 0;
    EXBaseSoundManager::GetSoundManager()->RegisterWatcherVars();
    EXBaseSoundManager::GetSoundManager()->SetLoadingSpeed(1);
    return 1;
}

s8 EXBaseApp::InitKeyboard()
{
    EXKeyboard::Init(new EXKeyboard);
    return 1;
}

s8 EXBaseApp::InitMouse()
{
    EXMouse::Init(new EXMouse);
    return 1;
}

s8 EXBaseApp::InitFileSystem()
{
    if (!EXFileSys::Init(new EXFileSys)) return 0;
    EXFileSys::GetFileSys()->RegisterWatcherVars();
    return 1;
}

s8 EXBaseApp::InitMallocMemory()
{
    EXMemHeap::RegisterMallocVars();
    return 1;
}

s8 EXBaseApp::InitWatcher()
{
    return EXWatcher::Init(new EXWatcher) != 0;
}

s8 EXBaseApp::InitDebugWndMgr()
{
    return EXDebugWndMgr::Init(new EXDebugWndMgr);
}

void EXBaseApp::ShutdownDisplay() { EXBaseDisplay::Shutdown(); }
void EXBaseApp::ShutdownPads() { EXBaseGamePad::Shutdown(); }
void EXBaseApp::ShutdownItemSystem() { EXItemEnv::Shutdown(); }
void EXBaseApp::ShutdownSoundManager() { EXBaseSoundManager::Shutdown(); }
void EXBaseApp::ShutdownKeyboard() { EXKeyboard::Shutdown(); }
void EXBaseApp::ShutdownMouse() { EXMouse::Shutdown(); }
void EXBaseApp::ShutdownFileSystem() { EXFileSys::Shutdown(); }
void EXBaseApp::ShutdownWatcher() { EXWatcher::Shutdown(); }
void EXBaseApp::ShutdownDebugWndMgr() { EXDebugWndMgr::Shutdown(); }

void EXBaseApp::Update()
{
    UpdateKeyboard();
    UpdateMouse();
    m_Unk64 = 1;
    if (m_bGamePause) {
        m_Unk64 = (m_Unk5c == 0);
        if (!m_Unk64 && m_Unk5c == 2 && m_Unk60++ > 10) {
            m_Unk64 = 1;
            m_Unk60 = 0;
        }
    }
    if (m_Unk64) UpdatePads();
    UpdateMemory();
    UpdateFileSystem();
    UpdateDisplay();
    if (m_Unk64) UpdateGame();
    UpdateSoundManager();
    m_UseVarRateScheduler = 1;
}

void EXBaseApp::Render()
{
    EXBaseDisplay *pDisplay = EXBaseDisplay::GetDisplay();
    pDisplay->BeginDraw();
    pDisplay->RedrawWindows();
    pDisplay->EndDraw();
}

void EXBaseApp::UpdateDisplay()
{
    EXBaseRenderEnv::Update();
    EXBaseDisplay::GetDisplay()->UpdateWindows();
}

void EXBaseApp::UpdatePads()
{
    ((EXGamePad *)EXBaseGamePad::GetGamePad())->Update();
    EXBaseGamePad::GetGamePad()->UpdateActions();
}

void EXBaseApp::UpdateSoundManager() { EXBaseSoundManager::GetSoundManager()->Update(); }
void EXBaseApp::UpdateKeyboard() {}
void EXBaseApp::UpdateMouse()
{
    EXMouse *pMouse = XGetMouse();
    if (pMouse->ButtonDown(0) || pMouse->ButtonPressed(0)) return;
}
void EXBaseApp::UpdateGame() {}
void EXBaseApp::UpdateFileSystem() { EXFileSys::GetFileSys()->Update(); }
void EXBaseApp::UpdateMemory() { EXMemHeap::UpdateAllHeaps(0, 1); }

void EXBaseApp::UpdateRenderFrameRate()
{
    m_GameRenderFrameRateDiv = (1.0f / m_RenderFrameScale) * m_GameFrameRateDiv;
    m_GameRenderFrameRateMul = m_GameFrameRateMul * m_RenderFrameScale;
    m_EngineRenderFrameRateDiv = (1.0f / m_RenderFrameScale) * m_EngineFrameRateDiv;
    m_EngineRenderFrameRateMul = m_EngineFrameRateMul * m_RenderFrameScale;
}

EXBaseApp::EXBaseApp() : m_Args(0, -1)
{
    m_pTheApp = this;
    m_AppName = "Please Set App Name";
    m_AppState = AS_RUNNING;
    m_Unk5c = 0;
    m_Unk60 = 0;
    m_Unk58 = 0;
    m_bFrameSkip = 0;
    m_bGamePause = 0;
    m_Unk64 = 0;
    m_Unk1d = 0;
    m_Unk1e = 0;
    m_Unk20 = 0;
    m_Unk22 = 0;
    m_DataRoot[0] = 0;
    m_WideScreen = 0;
    m_Language = 0xff;
    m_Unk24 = 1.0f;
}

void EXBaseApp::UpdateFrameRate(unsigned int rate)
{
    m_FrameRateInt = rate;
    m_FrameRate = (float)rate;
    m_FrameLength = m_FrameRate != 0.0f ? 1.0f / m_FrameRate : 0.0f;
    m_GameFrameRateDiv = m_FrameRate / (float)m_NominalGameFrameRate;
    m_GameFrameRateMul = (float)m_NominalGameFrameRate * (1.0f / m_FrameRate);
    m_EngineFrameRateDiv = m_FrameRate / (float)m_NominalEngineFrameRate;
    m_EngineFrameRateMul = (float)m_NominalEngineFrameRate * (1.0f / m_FrameRate);
    UpdateRenderFrameRate();
}

void EXBaseApp::UpdateFrameRate(float rate)
{
    m_FrameRateInt = (unsigned int)(rate + 0.5f);
    m_FrameRate = rate;
    m_FrameLength = m_FrameRate != 0.0f ? 1.0f / m_FrameRate : 0.0f;
    m_GameFrameRateDiv = m_FrameRate / (float)m_NominalGameFrameRate;
    m_GameFrameRateMul = (float)m_NominalGameFrameRate * (1.0f / m_FrameRate);
    m_EngineFrameRateDiv = m_FrameRate / (float)m_NominalEngineFrameRate;
    m_EngineFrameRateMul = (float)m_NominalEngineFrameRate * (1.0f / m_FrameRate);
    UpdateRenderFrameRate();
}

void EXBaseApp::SetRenderFrameScale(float scale)
{
    m_RenderFrameScale = scale;
    UpdateRenderFrameRate();
}
