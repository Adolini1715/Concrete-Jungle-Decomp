#ifndef FETOPMOST_H
#define FETOPMOST_H
#include "FEWnd.h"

// Topmost front-end windows (FETopmost.obj): loading screen, dialog host and cutscene overlay.

// Loading screen with a streamed background texture (sizeof 0x408).
class FELoading : public FEWnd {
public:
    virtual ~FELoading();
    virtual char *DebugName() { return "WndLoading"; }
    unsigned int SetTextureGetText(unsigned int tex, unsigned int level);
    static FELoading *Instance(signed char create);

    unsigned int m_Texture;  // 0x3fc geo section of the loading texture
    unsigned int m_Text;     // 0x400 hint text id
    int m_HintCount;         // 0x404

private:
    FELoading();
    static FELoading *m_pInstance;
};

// Window that hosts modal dialogs (sizeof 0x3fc).
class FEDialog : public FEWnd {
public:
    virtual ~FEDialog();
    virtual char *DebugName() { return "WndDialog"; }
    static FEDialog *Instance(signed char create);

private:
    FEDialog();
    static FEDialog *m_pInstance;
};

// Mimicry (Predator vision-mimic overlay) helpers (FETopmost.obj).
class FEMimicry {
public:
    static void Create(FEObject *p, int n, unsigned int a, unsigned int b);
    static void Delete(FEObject *p);
    static signed char Update(FEObject *p, signed char b);
    static FEMathmos m_WhiteMathmos;  // 0x48e0e4
};

// Cutscene overlay: subtitles, letterbox, credits, mimicry (sizeof 0x468). TODO: members.
class FECutscene : public FEWnd {
public:
    virtual ~FECutscene();
    virtual signed char Draw();
    virtual char *DebugName() { return "WndCutscene"; }
    static FECutscene *Instance(signed char create);
    void SetSubtitle(unsigned int hash, unsigned int text, signed char a, signed char b, float time);
    void CreditsStart(float delay);
    void CreditsStop();
    void CreditsUpdate();
    void HUDScriptOn(unsigned int hash, float scale, EXVector pos);
    void HUDScriptOff(unsigned int hash);
    signed char CreditsRunning() { return m_CreditsLine >= 0; }
    // [fi] copies in CameraScript.obj.
    void SetCutsceneGeoHash(unsigned int h) { m_HudFile = h; }
    void SetLetterBox(signed char b) { m_Unk400[0] = b; }
    void OpenMimicry() { m_Unk410[1] = 1; }
    void CloseMimicry() { m_Unk410[1] = 0; }

    unsigned int m_HudFile;      // 0x3fc
    u8 m_Unk400[4];              // 0x400
    unsigned int m_SubtitleHash; // 0x404
    signed char m_SubtitleA;     // 0x408
    signed char m_SubtitleB;     // 0x409
    float m_SubtitleTime;        // 0x40c
    u8 m_Unk410[4];              // 0x410
    int m_CreditsLine;           // 0x414 < 0: credits not rolling
    float m_CreditsTimer;        // 0x418
    float m_CreditsDelay;        // 0x41c
    EXRect m_LetterTop;          // 0x420 letterbox bars
    EXRect m_LetterBottom;       // 0x430
    FEMathmos m_BarMathmos;      // 0x440

private:
    FECutscene();
    static FECutscene *m_pInstance;
    static FEMathmos m_WhiteMathmos;  // 0x48e110
};
// [fi] copies in App.obj.
inline FELoading *FEGetLoading(signed char create) { return FELoading::Instance(create); }
inline FECutscene *FEGetCutscene(signed char create) { return FECutscene::Instance(create); }
inline FEDialog *FEGetDialog(signed char create) { return FEDialog::Instance(create); }
#endif
