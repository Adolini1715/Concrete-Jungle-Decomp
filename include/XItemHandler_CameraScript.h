#ifndef XITEMHANDLER_CAMERASCRIPT_H
#define XITEMHANDLER_CAMERASCRIPT_H
#include "XItemHandler.h"
#include "RGBA.h"
#include "EXArray.h"

class XItem;
class EXItemAnimator;
class EXItemAnimator_Anim;
class EXItemAnimator_Camera;

// Cutscene / scripted camera handler (CameraScript.obj). sizeof 0x98. TODO layout.
class XItemHandler_CameraScript : public XItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_CameraScript)
public:
    virtual ~XItemHandler_CameraScript();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    virtual u32 DoScriptCmdEvent(GEOSCRIPT_EVENT *pEvent, EXItemAnimator *pAnim);
    void HandleVisionModeTextureSwaps(unsigned char mode);
    void LightThreadItem(EXItemAnimator_Anim *pAnim);
    void UpdateLighting();
    XItemHandler_CameraScript();
    void Preload(signed char b);
    static XItemHandler_CameraScript *CreateObject();
    static XItem *Create(unsigned int a, unsigned int b);
    void FinishScript(unsigned int a, unsigned int b);
    unsigned int LastEventCmdHash() const { return m_Unk38; }
    void SetDisableUpdate() { m_Flags |= 0x8000; }
protected:
    EXItemAnimator *GetThreadItem(int i);
    static signed char m_bClearBitSet;
    static ex::RGBA m_BkColor;
public:
    // Window effects the script drives each frame (DoUpdate).
    s8 m_bMotionBlur;         // 0x08
    s8 m_bNoMotionBlur;       // 0x09 suppresses the motion blur for one frame
    s8 m_bFocusBlur;          // 0x0a
    s8 m_bNoise;              // 0x0b
    s8 m_bGlow;               // 0x0c fades out by 0.001 a frame
    s8 m_bNoPlayerControl;    // 0x0d
    u8 m_Unk0e[2];
    float m_MotionBlur;       // 0x10
    unsigned int m_FocusBlurFactor;  // 0x14
    float m_FocusBlurDistance;       // 0x18
    float m_Noise;            // 0x1c
    float m_Glow;             // 0x20
    unsigned int m_ScriptHash; // 0x24 cutscene script (Create)
    u32 m_Flags;              // 0x28 bit 15: camera handed back (FEMainMenu::InitCameraTransition)
    int m_FinishFlag;         // 0x2c flag channel set when the script finishes (FinishScript)
    unsigned int m_NextLevel; // 0x30 level to go to after the script (FinishScript)
    unsigned int m_FileHash;  // 0x34 geo file it came from; 0 for the current map (Create)
    int m_Unk38;              // 0x38 -1 once the script has finished (FEMainMenu::UpdateStateViewTrophies)
    unsigned int m_SavedVisionMode; // 0x3c player vision mode when the cutscene started (Create)
    unsigned int m_SkinOverride;    // 0x40 skin the cutscene predator wears instead of the player's (0: the player's)
    EXArray<unsigned int, unsigned int &> m_ThreadObject;  // 0x44 object reference each root thread placed
    EXArray<signed char, signed char &> m_ObjectCloaked;   // 0x58 per object reference (0x15000000 + i)
    EXArray<unsigned int, unsigned int &> m_StreamSound;   // 0x6c streamed sounds to precache
    EXArray<int, int &> m_StreamSoundFrame;                // 0x80 their start frames
    EXItemAnimator_Camera *m_pCamera; // 0x94 the script's camera; drives the game camera while set
};

// Trigger item that starts a cutscene (CameraScript.obj). sizeof 0x24. TODO layout.
class XItemHandler_CutsceneTrigger : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_CutsceneTrigger)
public:
    virtual ~XItemHandler_CutsceneTrigger();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    XItemHandler_CutsceneTrigger() {}
    static XItemHandler_CutsceneTrigger *CreateObject();
    u32 m_Options;            // 0x08 trigger m_Unk4c
    u32 m_Data[4];            // 0x0c trigger data 0..3 (data 2: map hash 0x1000001..0x100019b)
    unsigned int m_File;      // 0x1c cutscene geo file
    unsigned int m_Hash;      // 0x20 cutscene script
};
// Queue of cutscenes to load and play (CameraScript.obj, sizeof 0xa4); also fills the debug menu 0x50.
class CutscenePlayer {
public:
    CutscenePlayer() { Init(); }
    void Init();
    void Add(unsigned int file, unsigned int hash);
    void Play(unsigned int i);
    struct Entry { unsigned int file, hash; };
    Entry m_Queue[20];
    unsigned int m_Count;       // 0xa0
};
extern CutscenePlayer gCutscenePlayer;
#endif
