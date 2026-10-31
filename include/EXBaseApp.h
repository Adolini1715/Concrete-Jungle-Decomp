#ifndef EXBASEAPP_H
#define EXBASEAPP_H
#include "types.h"
#include "EXString.h"
#include "EXArray.h"

// Engine application base (EXApp.obj). Frame-rate state is static.
class EXBaseApp {
public:
    enum AppState { AS_RUNNING = 0, AS_QUIT = 1, AS_RESTART = 2 };  // TODO names (Restart() stores 2)
    virtual ~EXBaseApp();
    virtual char *QueryDataRootDirectory() { return 0; }
    virtual s8 InitHardware() = 0;
    virtual s8 InitInstance();
    virtual s8 InitGame() = 0;
    virtual s8 ExitGame() = 0;
    virtual int ExitInstance();
    virtual s8 ShutdownHardware() = 0;
    virtual s8 InitDisplay();
    virtual s8 InitPads();
    virtual s8 InitKeyboard();
    virtual s8 InitMouse();
    virtual s8 InitSoundManager();
    virtual s8 InitMallocMemory();
    virtual s8 InitFileSystem();
    virtual s8 InitWatcher();
    virtual s8 InitDebugWndMgr();
    virtual void ShutdownDisplay();
    virtual void ShutdownPads();
    virtual void ShutdownKeyboard();
    virtual void ShutdownMouse();
    virtual void ShutdownSoundManager();
    virtual void ShutdownFileSystem();
    virtual void ShutdownItemSystem();
    virtual void ShutdownWatcher();
    virtual void ShutdownDebugWndMgr();
    virtual s8 MainUpdate() = 0;
    virtual void Update();
    virtual void Render();
    virtual void UpdateDisplay();
    virtual void UpdatePads();
    virtual void UpdateSoundManager();
    virtual void UpdateKeyboard();
    virtual void UpdateMouse();
    virtual void UpdateFileSystem();
    virtual void UpdateMemory();
    virtual void UpdateGame();
    virtual void EnableEscapeKey(s8) {}

    EXBaseApp();
    static void UpdateRenderFrameRate();
    static void UpdateFrameRate(unsigned int rate);
    static void UpdateFrameRate(float rate);
    static void SetRenderFrameScale(float scale);

    static EXBaseApp *GetApp() { return m_pTheApp; }
    static unsigned int NominalGameFrameRate() { return m_NominalGameFrameRate; }
    static unsigned int NominalEngineFrameRate() { return m_NominalEngineFrameRate; }
    static float FrameLength() { return m_FrameLength; }
    static float FrameRate() { return m_FrameRate; }
    static float GameFrameRateMul() { return m_GameFrameRateMul; }
    static float GameRenderFrameRateMul() { return m_GameRenderFrameRateMul; }
    static float EngineRenderFrameRateMul() { return m_EngineRenderFrameRateMul; }
    static float RenderFrameScale() { return m_RenderFrameScale; }
    static void SetFrameRateHoldFrames(unsigned int n) { m_FrameRateHoldFrames = n; }
    static unsigned int VideoFrameRate() { return m_VideoFrameRate; }
    static void SetVideoFrameRate(unsigned int r) { m_VideoFrameRate = r; }
    static void SetMinFrameRate(unsigned int r) { m_MinFrameRate = r; }
    static void SetMaxFrameRate(unsigned int r) { m_MaxFrameRate = r; }
    static unsigned int MaxFrameRate() { return m_MaxFrameRate; }
    void EnableGamePause(s8 b) { m_bGamePause = b; }
    EXStringArray &Args() { return m_Args; }
    void Restart() { m_AppState = AS_RESTART; }
    s8 FrameSkipIndicator() const { return m_bFrameSkip != 0; }
    const EXString &AppName() const { return m_AppName; }
    AppState GetAppState() const { return m_AppState; }
    void SetWideScreen(unsigned int b) { m_WideScreen = b; }
    void SetPal60(unsigned int b) { m_Pal60 = b; }
    void SetLanguage(unsigned int l) { m_Language = (u8)l; }
protected:
    EXString m_AppName;         // 0x04
    AppState m_AppState;        // 0x08
    EXStringArray m_Args;       // 0x0c
    u8 m_Language;              // 0x1c
    s8 m_Unk1d;                 // 0x1d TODO
    s8 m_Unk1e;                 // 0x1e TODO
    u16 m_Unk20;                // 0x20 TODO
    u16 m_Unk22;                // 0x22 TODO
    float m_Unk24;              // 0x24 TODO 1.0
    EXFixedArray<char, 33> m_DataRoot;  // 0x28 TODO name (ctor clears [0]; operator[]<int> emitted in EXApp.obj)
    unsigned int m_WideScreen;  // 0x4c
    unsigned int m_Pal60;       // 0x50
    u32 m_Unk54;                // 0x54 TODO
    s8 m_Unk58;                 // 0x58 TODO
    s8 m_bFrameSkip;            // 0x59
    s8 m_bGamePause;            // 0x5a
    u32 m_Unk5c;                // 0x5c TODO
    u32 m_Unk60;                // 0x60 TODO
    s8 m_Unk64;                 // 0x64 TODO

    static s8 m_UseVarRateScheduler;
    static unsigned int m_NominalGameFrameRate;
    static unsigned int m_NominalEngineFrameRate;
    static unsigned int m_VideoFrameRate;
    static unsigned int m_MinFrameRate;
    static unsigned int m_MaxFrameRate;
    static unsigned int m_FrameRateInt;
    static float m_FrameRate;
    static float m_FrameLength;
    static float m_GameFrameRateDiv;
    static float m_GameFrameRateMul;
    static float m_EngineFrameRateDiv;
    static float m_EngineFrameRateMul;
    static float m_RenderFrameScale;
    static float m_GameRenderFrameRateDiv;
    static float m_GameRenderFrameRateMul;
    static float m_EngineRenderFrameRateDiv;
    static float m_EngineRenderFrameRateMul;
    static unsigned int m_FrameRateHoldFrames;
    static EXBaseApp *m_pTheApp;
};
// Game-side frame-rate accessors ([fi] copies in Actor.obj, emitted right after the EXBaseApp ones).
inline unsigned int XGetNominalFrameRate() { return EXBaseApp::NominalGameFrameRate(); }
inline float XGetFrameLength() { return EXBaseApp::FrameLength(); }
inline float XGetFrameRateMul() { return EXBaseApp::GameFrameRateMul(); }
#endif
