#ifndef EXRENDERFOGINFO_H
#define EXRENDERFOGINFO_H
#include "types.h"
#include "RGBA.h"

// Fog parameters of the render environment (EXBaseRenderEnv::ms_RenderFogInfo), sizeof 0x18.
// TODO layout: names from the EXBaseRenderEnv accessors (EXItemEnv.obj copies); +0x15..0x17 unknown.
class EXRenderFogInfo {
public:
    EXRenderFogInfo() { m_enabled = 0; }  // EXRenderEnv.obj copy
    // [fi] copies in CameraScript.obj.
    void SetFogDistance(float n, float f) { m_near = n; m_far = f; }
    void SetFogMinMax(float mn, float mx) { m_min = mn; m_max = mx; }
    void SetFogColor(ex::RGBA c) { m_color = c; }
    void SetFogEnabled(s8 b) { m_enabled = b; }
    ex::RGBA m_color;   // +0
    float m_near;       // +4
    float m_far;        // +8
    float m_min;        // +0xc
    float m_max;        // +0x10
    s8 m_enabled;       // +0x14
    u8 m_15[3];
};
#endif
