#include "EXCamera.h"
#include "EXString.h"
#include "EXStdFuncs.h"
#include "EXMemory.h"

namespace ex { double fd(float f); }
extern "C" int __cdecl vsprintf(char *, const char *, char *);

inline float qcotf(float a)
{
    float t = (float)tan(a);
    if (t != 0.0f) return 1.0f / t;
    return 0.0f;
}

inline float qacotf(float a)
{
    if (a != 0.0f) a = 1.0f / a;
    else a = 0.0f;
    return (float)atan(a);
}

float EXCommonCamera::HFOV() const { return (float)atan(m_viewSpaceSize.w / (m_focalLength + m_focalLength)) * 2.0f; }
float EXCommonCamera::VFOV() const { return m_vfov; }

float EXCommonCamera::HZoom() const
{
    float t = (float)tan(atan(m_viewSpaceSize.w / (m_focalLength + m_focalLength)));
    if (t != 0.0f) t = 1.0f / t;
    else t = 0.0f;
    return t;
}

float EXCommonCamera::VZoom() const
{
    float t = (float)tan(m_vfov * 0.5f);
    if (t != 0.0f) t = 1.0f / t;
    else t = 0.0f;
    return t;
}

float EXCommonCamera::FocalLength() const { return m_focalLength; }
const EXRectf &EXCommonCamera::ViewSpaceSize() const { return m_viewSpaceSize; }

void EXCommonCamera::DebugMessage(const char *pFmt, ...)
{
    char buf[0x200];
    vsprintf(buf, pFmt, (char *)(&pFmt + 1));
    if (m_pName) m_pName->IsEmpty();
}

void EXCommonCamera::SetName(const char *pName)
{
    if (!m_pName) {
        m_pName = new EXString;
        if (!m_pName) return;
    }
    *m_pName = pName;
    if (m_debugFlags & 0x200) DebugMessage("SetName(%s)", *(const char **)m_pName);
}

void EXCommonCamera::SetFocalLength(const EXRectf &r, float f)
{
    if (m_debugFlags & 0x4000)
        DebugMessage("SetFocalLength((%f,%f,%f,%f),%f)", ex::fd(r.x), ex::fd(r.y), ex::fd(r.w), ex::fd(r.h), ex::fd(f));
    SetVFOV(r, (float)atan(r.h / (f + f)) * 2.0f);
}

void EXCommonCamera::SetVFOV(const EXRectf &r, float fov)
{
    if (m_debugFlags & 0x800)
        DebugMessage("SetVFOV((%f,%f,%f,%f),%f)", ex::fd(r.x), ex::fd(r.y), ex::fd(r.w), ex::fd(r.h), ex::fd(fov));
    m_viewSpaceSize.x = r.x;
    m_viewSpaceSize.y = r.y;
    m_viewSpaceSize.w = r.w;
    m_viewSpaceSize.h = r.h;
    m_vfov = fov;
    m_focalLength = r.h * qcotf(fov * 0.5f) * 0.5f;
}

void EXCommonCamera::SetVZoom(const EXRectf &r, float zoom)
{
    if (m_debugFlags & 0x2000)
        DebugMessage("SetVZoom((%f,%f,%f,%f),%f)", ex::fd(r.x), ex::fd(r.y), ex::fd(r.w), ex::fd(r.h), ex::fd(zoom));
    SetVFOV(r, qacotf(zoom) * 2.0f);
}

void EXCommonCamera::SetHFOV(const EXRectf &r, float fov)
{
    if (m_debugFlags & 0x400)
        DebugMessage("SetHFOV((%f,%f,%f,%f),%f)", ex::fd(r.x), ex::fd(r.y), ex::fd(r.w), ex::fd(r.h), ex::fd(fov));
    SetFocalLength(r, r.w * qcotf(fov * 0.5f) * 0.5f);
}

void EXCommonCamera::SetHZoom(const EXRectf &r, float zoom)
{
    if (m_debugFlags & 0x1000)
        DebugMessage("SetHZoom((%f,%f,%f,%f),%f)", ex::fd(r.x), ex::fd(r.y), ex::fd(r.w), ex::fd(r.h), ex::fd(zoom));
    SetHFOV(r, qacotf(zoom) * 2.0f);
}

EXBaseCamera::EXBaseCamera()
{
    m_WorldUp.set(0.0f, 1.0f, 0.0f, 0.0f);
    SetPosition(EXVector(0.0f, 0.0f, -100.0f, 0.0f));
    SetTarget(EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    SetOrigin(EXVector2(50.0f, 50.0f));
    SetVFOV(EXRectf(0.0f, 0.0f, 100.0f, 100.0f), 1.0471976f);
    SetClipSize(200, 200, 0);
    SetZRange(0.001f, 1000.0f);
    SetAspectCorrection(1);
}

EXBaseCamera::~EXBaseCamera() {}

EXBaseProjector::EXBaseProjector()
{
    m_WorldUp.set(0.0f, 0.0f, 1.0f, 0.0f);
    SetPosition(EXVector(0.0f, 5.0f, 0.0f, 0.0f));
    SetTarget(EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    SetVFOV(EXRectf(0.0f, 0.0f, 255.0f, 255.0f), 1.0471976f);
    SetZRange(0.001f, 1000.0f);
}

EXBaseProjector::~EXBaseProjector() {}
