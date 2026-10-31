#include "EXBaseRenderEnv.h"
#include "EXWatcher.h"

// Xbox render environment (XBRenderEnv.obj): the fog override watcher items.

EXRenderEnv::EXRenderEnv() {}

EXRenderEnv::~EXRenderEnv() {}

void EXRenderEnv::RegisterWatcherVars()
{
    EXBaseRenderEnv::RegisterWatcherVars();
    ms_OverrideFogInfo.m_min = 0.8f;
    ms_OverrideFogInfo.m_max = 0.0f;
    EXWatcher *w = EXWatcher::GetWatcher();
    unsigned int h = w->AddItem("Near Fog", "Fog", EXWATCH::DT_7, &ms_OverrideFogInfo.m_max, 1);
    w->EnableItemEdit(h, 0.0f, 1.0f, 0.01f, 1);
    h = w->AddItem("Far Fog", "Fog", EXWATCH::DT_7, &ms_OverrideFogInfo.m_min, 1);
    w->EnableItemEdit(h, 0.0f, 1.0f, 0.01f, 1);
}
