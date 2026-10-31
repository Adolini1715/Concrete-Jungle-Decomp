#ifndef FEMANAGER_H
#define FEMANAGER_H
#include "FEWnd.h"
#include "XAppStandIn.h"

// Front-end root window: boot movies/legal pages, attract mode and the page state machine
// (FEManager.obj). Singleton, sizeof 0x414.
class FEManager : public FEWnd {
public:
    virtual ~FEManager()
    {
        ContentDestroy();
        m_pInstance = 0;
    }
    virtual signed char Draw();
    virtual void MovieUpdate() { EXWnd::MovieUpdate(); }
    virtual void MovieClose()
    {
        m_MoviePlaying = 0;
        EXWnd::MovieClose();
    }
    virtual void ContentCreate();
    virtual void ContentUpdate();
    virtual void ContentDestroy();
    virtual char *DebugName() { return "WndManager"; }

    static FEManager *Instance(signed char create);

    unsigned int PageState() { return m_PageState; }
    void SetPageState(unsigned int s)
    {
        m_PageState = s;
        m_PageStateTimer = 0.0f;
        m_PageSubstate = 0;
        m_PageSubstateTimer = 0.0f;
    }
    unsigned int PageSubstate() { return m_PageSubstate; }
    void SetPageSubstate(unsigned int s)
    {
        m_PageSubstate = s;
        m_PageSubstateTimer = 0.0f;
    }
    float PageStateTimerSecs() { return m_PageStateTimer; }
    float PageSubstateTimerSecs() { return m_PageSubstateTimer; }
    signed char AttractMode() const { return m_Attract; }
    float AttractModeTime() const
    {
        if (m_Attract) return m_AttractTime;
        return 0.0f;
    }
    void MovieOpen(char *pName)
    {
        m_MoviePlaying = 1;
        FEBaseWnd::MovieOpen(pName);
    }
    void TickPage()
    {
        float dt = XApp::FrameLength();
        m_PageStateTimer += dt;
        m_PageSubstateTimer += dt;
    }

    signed char m_Attract;          // 0x3fc attract-mode movie running
    signed char m_MoviePlaying;     // 0x3fd
    float m_AttractTime;            // 0x400
    unsigned int m_PageState;       // 0x404
    unsigned int m_PageSubstate;    // 0x408
    float m_PageStateTimer;         // 0x40c seconds in the current page state
    float m_PageSubstateTimer;      // 0x410

private:
    FEManager();
    static FEManager *m_pInstance;
};
inline FEManager *FEGetManager(signed char create) { return FEManager::Instance(create); }  // [fi] App.obj
#endif
