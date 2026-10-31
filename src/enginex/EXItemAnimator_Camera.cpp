#include "EXItemAnimator_Camera.h"
#include "EXBaseRenderEnv.h"
#include "EXMemory.h"

// TODO stand-in: script camera command (geo script area).
struct GEOSCRIPT_CAMERA {
    u8 pad[0x14];
    float m_apertureWidth;  // +0x14
    float m_apertureHeight; // +0x18
    float m_focalLength;    // +0x1c
    float m_zoom;           // +0x20
};

EX_IMPLEMENT_DYNCREATE(EXItemAnimator_Camera, EXItemAnimator)

void EXItemAnimator_Camera::SetDefaults()
{
    SetFocalLength(1.0f);
    SetAperture(0.64f, 0.48f);
    SetZoom(1.0f);
    SetViewport(0, 0, 640, 480);
}

void EXItemAnimator_Camera::SetFromScriptCmd(const GEOSCRIPT_CAMERA *pCmd)
{
    m_focalLength = pCmd->m_focalLength;
    m_apertureWidth = pCmd->m_apertureWidth;
    m_apertureHeight = pCmd->m_apertureHeight;
    m_zoom = pCmd->m_zoom;
    SetViewport(0, 0, 640, 480);
}

void EXItemAnimator_Camera::UpdateCamera() const {}

u32 EXItemAnimator_Camera::DoRender(const EXMatrix &mtx, const EXRenderInfo &, u32 flags)
{
    if (EXBaseRenderEnv::RenderState() & 1) CalcMatrix_InPlace(mtx, flags);
    return 1;
}

void EXItemAnimator_Camera::SetObjectTime(float) { CalcMatrix_InPlaceParent(); }

EXItemAnimator_Camera::EXItemAnimator_Camera() { SetDefaults(); }

s8 EXItemAnimator_Camera::Initialise(EXGeoFile *, u32)
{
    SetDefaults();
    return 1;
}
