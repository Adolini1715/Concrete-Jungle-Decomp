#ifndef XDEBUG_H
#define XDEBUG_H
#include "types.h"
#include "RGBA.h"

class EXVector;
class EXDebugWnd;

// Game debug windows (Debug.obj). sizeof 0x2c. Retail builds leave the window code empty.
class XDebug {
public:
    XDebug();
    ~XDebug();
    void InitDebugWindows();
    void PostLoadDebugWindows();
    float GetAverageFrameRate();
    float GetCurrentFrameRate();
    void UpdateDebugWindows();
    void ClearDebugWindows();
    void CreateMarker(const EXVector &pos, ex::RGBA col, float size);
    static XDebug *GetDebug() { return m_pDebug; }
    EXDebugWnd *HelpWnd() { return m_pHelpWnd; }
    EXDebugWnd *ProfileWnd() { return m_pProfileWnd; }
    EXDebugWnd *PlayerWnd() { return m_pPlayerWnd; }
    EXDebugWnd *Player2Wnd() { return m_pPlayer2Wnd; }
    EXDebugWnd *CameraWnd() { return m_pCameraWnd; }
    EXDebugWnd *AIWnd() { return m_pAIWnd; }
    EXDebugWnd *InventoryWnd() { return m_pInventoryWnd; }
    EXDebugWnd *TesterWnd() { return m_pTesterWnd; }
    EXDebugWnd *PerformanceWnd() { return m_pPerformanceWnd; }
    EXDebugWnd *FlagChannelWnd() { return m_pFlagChannelWnd; }
protected:
    static XDebug *m_pDebug;
    EXDebugWnd *m_pHelpWnd;         // 0x00 (accessor order assumed = member order)
    EXDebugWnd *m_pProfileWnd;      // 0x04
    EXDebugWnd *m_pPlayerWnd;       // 0x08
    EXDebugWnd *m_pPlayer2Wnd;      // 0x0c
    EXDebugWnd *m_pCameraWnd;       // 0x10
    EXDebugWnd *m_pAIWnd;           // 0x14
    EXDebugWnd *m_pInventoryWnd;    // 0x18
    EXDebugWnd *m_pTesterWnd;       // 0x1c
    EXDebugWnd *m_pPerformanceWnd;  // 0x20
    EXDebugWnd *m_pFlagChannelWnd;  // 0x24
    s8 m_bMarkers;                  // 0x28 TODO name
};
extern XDebug *g_Debug;
extern XDebug theDebug;
#endif
