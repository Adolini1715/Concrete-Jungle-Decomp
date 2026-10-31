#ifndef XAPP_H
#define XAPP_H
#include "EXApp.h"
#include "EXStack.h"
#include "EXString.h"
#include "FEMathmos.h"
#include "RGBA.h"

// One entry of the game-state stack (sizeof 0x14).
namespace GameState {
enum Flags { GS_NONE = 0 };  // TODO names (2 = loading?, 9 = running, 0xc = paused, 0x16 = controller removed)
}
class XGameState {
public:
    XGameState()
    {
        m_State = GameState::GS_NONE;
        m_SubState = 1;
        m_Data = 0;
        m_Timer = 0.0f;
        m_Counter = 0;
    }
    XGameState(GameState::Flags state, unsigned int data)
    {
        m_State = state;
        m_SubState = state + 1;
        m_Data = data;
        m_Timer = 0.0f;
        m_Counter = 0;
    }
    float Timer() { return m_Timer; }
    GameState::Flags State() { return (GameState::Flags)m_State; }
    unsigned int SubState() { return m_SubState; }
    void SubStateInc() { m_SubState++; }
    unsigned int Data() { return m_Data; }
    void SetSubState(unsigned int s) { m_SubState = s; }
    void Update()
    {
        m_Timer += EXBaseApp::GameFrameRateMul();
        m_Counter++;
    }
    int m_State;         // 0x00 GameState::Flags
    unsigned int m_SubState; // 0x04 state + 1 when pushed
    unsigned int m_Data; // 0x08
    float m_Timer;       // 0x0c seconds in this state
    int m_Counter;       // 0x10 frames in this state
};

// The game application (App.obj). sizeof 0xe78 (the FEText.h header static follows theApp).
class XApp : public EXApp {
public:
    XApp();
    virtual ~XApp();
    virtual s8 InitInstance();
    virtual s8 InitGame();
    virtual s8 ExitGame();
    virtual s8 InitDisplay();
    virtual s8 InitPads();
    virtual void UpdateGame();

    static void HandleMemCompaction(unsigned int n);
    static unsigned int Frontend2DFile();
    static unsigned int Frontend3DFile();
    static void UpdateGameFrameRate();
    void LoadingScreenOpen(unsigned int hash);
    void LoadingScreenClose();
    void LoadingScreenDraw();
    int LoadingScreenBarWidth();  // inline in App.cpp
    void LoadingCutsceneDraw();
    void DoManualClothUpdate();
    void CancelFade();
    void MovieOpen();
    signed char MovieUpdate();
    unsigned int MovieClose();
    void PlayMovie(char *pName, unsigned int flags);
    // Fades out over half the time, then back in ([fi] in CameraScript.obj).
    void ApplyFadeOutIn(float time, ex::RGBA col, signed char a, signed char b)
    {
        m_bFadePaused = b;
        ApplyFadeOut(time * 0.5f, col, a);
        m_FadeColour = col;
        m_Unkccb = 1;
        m_FadeTime = time;
    }

    // Game-state stack.
    void GameStatePush(GameState::Flags state, unsigned int data)  // inlined in App.obj, FEHudVision.obj
    {
        XGameState s(state, data);
        m_GameStates.Push(s);
        m_bGameStateChanged = 1;
    }
    void GameStateFlush(GameState::Flags state, unsigned int data);
    void GameStatePop()  // inlined in App.obj, FEHudVision.obj
    {
        XGameState s;
        m_GameStates.Pop(s);
        m_bGameStateChanged = 1;
    }
    void GameStateUpdateCurrent();  // inline in App.cpp
    XGameState *GameStateCurrent();
    signed char GameStateChanged() { return m_bGameStateChanged; }
    float GameStateTimer() { return GameStateCurrent()->Timer(); }
    int GameStateCounter() { return GameStateCurrent()->m_Counter; }
    signed char IsRunning() { return GameStateCurrent()->m_State == 9; }
    signed char IsPaused() { return GameStateCurrent()->m_State == 0xc; }

    // Frame rate (the game keeps its own copies of the engine values).
    static float GlobalGameFrameRate() { return m_GlobalGameFrameRate; }
    static void SetGlobalGameFrameRate(float f) { m_GlobalGameFrameRate = f; }
    static float FrameRate() { return m_FrameRate; }
    static float FrameLength() { return m_FrameLength; }
    static float GameFrameRateDiv() { return m_GameFrameRateDiv; }
    static float GameFrameRateMul() { return m_GameFrameRateMul; }

    // Mission state.
    void SetMissionInProgress() { m_MissionState = 1; m_MissionTimer = 0.0f; }
    void SetMissionComplete() { m_MissionState = 2; m_MissionTimer = 0.0f; }
    void SetMissionFailed() { m_MissionState = 3; m_MissionTimer = 0.0f; }
    void SetMissionDead() { m_MissionState = 4; m_MissionTimer = 0.0f; }
    void PredIsDead() { m_MissionState = 4; m_MissionTimer = 0.0f; }
    void ResetMissionTimer() { m_MissionTimer = 0.0f; }
    signed char IsMissionInProgress() { return m_MissionState == 1; }
    signed char IsMissionComplete() { return m_MissionState == 2; }
    signed char IsMissionFailed() { return m_MissionState == 3; }
    signed char IsMissionDead() { return m_MissionState == 4; }
    float MissionTimer() { return m_MissionTimer; }

    // Dialogs (inline in App.cpp). The channels are flag channels set when the button is pressed.
    void CreateDialogOk(EXString text, int ok);
    void CreateDialogYesNo(EXString text, int yes, int no);
    void CreateDialogYesNoCancel(EXString text, int yes, int no, int cancel);

    // Fades.
    void ApplyFadeIn(float time, ex::RGBA colour, signed char b);   // inline in App.cpp
    void ApplyFadeOut(float time, ex::RGBA colour, signed char b);  // inline in App.cpp
    signed char IsFadeActive() { return m_FadeActive; }
    signed char FinishedFade()
    {
        signed char b = m_bFinishedFade;
        m_bFinishedFade = 0;
        return b;
    }
    signed char HalfwayFade()
    {
        signed char b = m_bHalfwayFade;
        m_bHalfwayFade = 0;
        return b;
    }
    void UnpauseFade() { m_bFadePaused = 0; }

    // Misc flags.
    signed char GetCutscene() { return m_bCutscene; }
    void SetCutscene(signed char b) { m_bCutscene = b; }
    signed char IsIntroCutscene() const { return m_bIntroCutscene; }
    signed char GivenPlayerControl() { return m_GivenPlayerControl; }
    signed char PlayingDeathMusic() { return m_bDeathMusic; }
    void SetPauseEnabled(signed char b) { m_bPauseEnabled = b; }
    signed char PauseEnabled() { return m_bPauseEnabled; }
    void SetSaveFlag(signed char b) { m_bSaveFlag = b; }
    signed char DialogOpen() { return m_DialogOpen; }
    void SetDialogOpen(signed char b) { m_DialogOpen = b; }
    void SetDialogLeftAlign() { m_bDialogLeftAlign = 1; }
    signed char LevelLoaded() { return m_bLevelLoaded; }
    signed char IsFrontend3D() const;  // inline in App.cpp
    void SetLevelLoaded(signed char b) { m_bLevelLoaded = b; }
    void SetNextLevelHash(unsigned int h) { m_NextLevelHash = h; }
    float LevelPlayTimer() const { return m_LevelPlayTimer; }
    float GlobalTimer() { return m_Time; }
    float Time() { return m_Time; }  // TODO: the FE units' name for GlobalTimer

    u32 m_Unkc8c;                         // 0xc8c TODO
    EXStack<XGameState> m_GameStates;     // 0xc90
    signed char m_bGameStateChanged;      // 0xcac
    signed char m_Unkcad;                 // 0xcad TODO (cleared by InitGame)
    unsigned int m_PauseMusic;            // 0xcb0 music hash to resume after the pause (-1 after InitGame)
    int m_Unkcb4;                         // 0xcb4 TODO (cleared when loading starts)
    float m_LoadBarPos;                   // 0xcb8 loading bar fill of the current file
    signed char m_bLevelLoaded;           // 0xcbc
    unsigned int m_LoadingTexture;        // 0xcc0 loading-screen texture hash
    EXString m_Unkcc4;                    // 0xcc4 loading-screen text
    signed char m_FadeActive;             // 0xcc8
    signed char m_bFinishedFade;          // 0xcc9
    signed char m_bHalfwayFade;           // 0xcca
    signed char m_Unkccb;                 // 0xccb TODO (cleared by InitGame)
    float m_FadeTime;                     // 0xccc TODO name
    ex::RGBA m_FadeColour;                // 0xcd0
    signed char m_Unkcd4;                 // 0xcd4 TODO (ApplyFade flag argument)
    signed char m_bFadePaused;            // 0xcd5
    FEMathmos m_FadeMathmos;              // 0xcd8
    signed char m_GivenPlayerControl;     // 0xd00
    signed char m_bDeathMusic;            // 0xd01
    int m_MissionState;                   // 0xd04 1 in progress, 2 complete, 3 failed, 4 dead
    float m_MissionTimer;                 // 0xd08
    signed char m_bSaveFlag;              // 0xd0c
    signed char m_bPauseEnabled;          // 0xd0d
    struct LoadFile {
        class EXGeoFile *m_pFile;
        int m_Size;
    };
    LoadFile m_LoadFiles[32];             // 0xd10 files queued by the loading state
    int m_nLoadFiles;                     // 0xe10
    int m_LoadIndex;                      // 0xe14 file being waited for
    int m_LoadTotal;                      // 0xe18 total size
    int m_LoadDone;                       // 0xe1c size loaded so far
    u8 m_Unke20[0xe28 - 0xe20];           // TODO layout
    u32 m_Unke28;                         // 0xe28 level being loaded (cleared by InitGame)
    u32 m_Unke2c;                         // 0xe2c TODO (cleared by InitGame)
    signed char m_bCutscene;              // 0xe30
    int m_DialogYes;                      // 0xe34 flag channel set by A (-1 after InitGame)
    int m_DialogNo;                       // 0xe38 flag channel set by B (-1 after InitGame)
    int m_DialogCancel;                   // 0xe3c flag channel set by X
    int m_DialogType;                     // 0xe40 1 yes/no, 2 yes/no/cancel, 3 ok (cleared by InitGame)
    EXString m_DialogText;                // 0xe44
    signed char m_DialogOpen;             // 0xe48
    signed char m_bDialogLeftAlign;       // 0xe49
    float m_Time;                         // 0xe4c global timer
    float m_LevelPlayTimer;               // 0xe50
    union {
        unsigned int m_NextLevelHash;     // 0xe54 cleared by FELang::ContentDestroy
        unsigned int m_Unke54;            // the FE units' name for it
    };
    signed char m_bIntroCutscene;         // 0xe58
    signed char m_bMoviePlaying;          // 0xe59
    char m_MovieName[0xe6c - 0xe5a];      // 0xe5a TODO size
    class FEWnd *m_pMovieWnd;             // 0xe6c
    unsigned int m_MovieResult;           // 0xe70 TODO name
    float m_MovieTime;                    // 0xe74
protected:
    static float m_GlobalGameFrameRate;
    static unsigned int m_FrameRateInt;
    static float m_FrameRate;
    static float m_FrameLength;
    static float m_GameFrameRateDiv;
    static float m_GameFrameRateMul;
};
extern XApp theApp;
inline EXApp *XGetApp() { return (EXApp *)EXBaseApp::GetApp(); }
// Game frame-rate accessors ([fi] copies in Actor.obj, emitted right after the XApp ones).
inline float GetFrameLength() { return XApp::FrameLength(); }
inline float GetGlobalGameFrameRate() { return XApp::GlobalGameFrameRate(); }
inline float GetFrameRateDiv() { return XApp::GameFrameRateDiv(); }
inline float GetFrameRateMul() { return XApp::GameFrameRateMul(); }
#endif
