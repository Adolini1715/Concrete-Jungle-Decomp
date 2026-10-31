#ifndef XGAMEWND_H
#define XGAMEWND_H
#include "EXWnd.h"
#include "EXCamera.h"

class EXGeoFile;
class EXMenu;
class EXCamera;

// The in-game 3D window (GameWnd.obj). sizeof 0x408 (NewGameWnd allocation). TODO layout
class XGameWnd : public EXWnd {
public:
    XGameWnd();
    virtual ~XGameWnd();
    virtual s8 InitialUpdate();
    virtual s8 Update();
    virtual void DrawShadow();
    virtual s8 DrawWindow();
    virtual s8 Draw();
    virtual s8 MenuSelect(unsigned int id);
    // GameWnd.obj
    void WaitForIslandInit();
    signed char WaitForIsland();
    void SetupCamera();
    void Debug_TestEngineFunctions();
    void LevelPreLoad();
    EXGeoFile *FileLoad(unsigned int hash, int *pLen);
    void LevelPreRun();
    void LevelDeinit(signed char b);
    EXGeoFile *LevelLoad(unsigned int hash, int *pLen);
    void LevelPostLoad();
    void UpdateLevelMenu();
    unsigned int FrameCounter() { return m_Unk384; }  // appended by the actor agent (Actor.obj copy)
    EXCamera *GetCamera() { return &m_Camera; }
    EXMenu *getMenu() const { return m_pMenu; }  // [fi] copy in CameraScript.obj  // [fi] copy in Camera.obj
    EXCamera m_Camera;           // 0x29c
    EXVector m_Unk354;           // 0x354 cleared in the ctor
    EXMenu *m_pLevelMenu;        // 0x364 level select debug menu (UpdateLevelMenu)
    u8 m_Unk368[0x370 - 0x368];  // TODO layout
    EXWnd *m_pBarWnd[2];     // 0x370 mem/perf bar windows (shown when m_bShowBars)
    EXWnd *m_pDisplayWnd;    // 0x378 (shown when m_bShowDisplay)
    s8 m_bShowBars;              // 0x37c
    s8 m_bShowDisplay;           // 0x37d
    u16 m_Unk37e;                // 0x37e TODO
    u32 m_Unk380;                // 0x380 0 = traffic triggers not yet initialised (LevelPreLoad)
    u32 m_Unk384;                // 0x384 game time/frame stamp (EVD::Dial999)
    u8 m_Unk388[0x390 - 0x388];  // 0x388 TODO (ctor: 0, 0, 1, 1, 0)
    EXProjector m_Projector;     // 0x390
    u8 m_Unk3f8[0x400 - 0x3f8];  // TODO layout
    int m_Unk400;                // 0x400 island index (-1 = no island; GameIsland.obj)
    u32 m_Unk404;                // 0x404 cleared by Island_Initialise
    // GameIsland.obj (appended by the misc agent)
    signed char GetIslandSectionInfo(EXVector &, unsigned int &);
    void UpdateDistrictLoading();
protected:
    void Island_Initialise();
    void UpdateIslandRadar();
};
#endif
