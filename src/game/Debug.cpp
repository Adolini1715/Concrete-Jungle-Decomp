#include "XDebug.h"
#include "FEText.h"

unsigned int g_uiFrameRate[8];
XDebug *g_Debug;
XDebug *XDebug::m_pDebug;
unsigned int g_uiTotalFrames;
unsigned int g_uiFirstFrameFlag;
unsigned long g_ulTotalFrameRate;
unsigned long g_VBlankCount;
unsigned long g_ulCurrentFrameRate;

XDebug::XDebug()
{
    m_pDebug = this;
    m_pHelpWnd = 0;
    m_pProfileWnd = 0;
    m_pPlayerWnd = 0;
    m_pPlayer2Wnd = 0;
    m_pCameraWnd = 0;
    m_pAIWnd = 0;
    m_pInventoryWnd = 0;
    m_pTesterWnd = 0;
    m_pPerformanceWnd = 0;
    m_pFlagChannelWnd = 0;
    m_bMarkers = 0;
    g_Debug = this;
}
XDebug theDebug;

XDebug::~XDebug() {}
void XDebug::InitDebugWindows() {}
void XDebug::PostLoadDebugWindows()
{
    g_uiFrameRate[0] = 0;
    g_uiFrameRate[1] = 0;
    g_uiFrameRate[2] = 0;
    g_uiFrameRate[3] = 0;
    g_uiFrameRate[4] = 0;
    g_uiFrameRate[5] = 0;
    g_uiFrameRate[6] = 0;
    g_uiFrameRate[7] = 0;
    g_uiTotalFrames = 0;
    g_uiFirstFrameFlag = 0;
    g_ulTotalFrameRate = 0;
}
float XDebug::GetAverageFrameRate() { return (float)g_ulTotalFrameRate / (float)g_uiTotalFrames; }
float XDebug::GetCurrentFrameRate() { return (float)g_ulCurrentFrameRate; }
void XDebug::UpdateDebugWindows() {}
void XDebug::ClearDebugWindows() {}
void XDebug::CreateMarker(const EXVector &, ex::RGBA, float) {}
