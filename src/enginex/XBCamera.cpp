#include "EXCamera.h"
#include "EXBaseDisplay.h"
#include "EXDisplay.h"
#include "EXBaseRenderEnv.h"
#include "XBBaseVertexShader.h"
#include "XBBaseEfx.h"
#include "XBShadow.h"
#include "xbd3d8.h"

extern "C" double __cdecl sin(double);
extern "C" double __cdecl cos(double);
extern "C" double __cdecl sqrt(double);
#pragma intrinsic(sin, cos, sqrt)

// Xbox camera and projector (XBCamera.obj): builds the view, projection, cull and screen
// matrices for the display from the camera's position, target and lens, and sets up the
// W-buffer depth range the vertex shaders write (24-bit W, 16777215 at the far plane).

EXCamera::EXCamera()
{
    m_54 = 0.0f;
    m_58 = 1.0f;
}

EXCamera::~EXCamera() {}

EXProjector::EXProjector()
{
    m_54 = 0.0f;
    m_58 = 1.0f;
    m_bPerspective = 1;
}

EXProjector::~EXProjector() {}

void EXProjector::UpdateDisplayProjection() {}

// W buffer: scale the projection so that w runs 0..16777215 over 0..far, tell D3D the depth
// clip planes in that range, and keep the fog scale and the depth used for screen quads.
void EXCamera::SetupWBufferProjection()
{
    float zFar = m_ZFar;
    float zNear = m_ZNear;
    EXMaths::mat_44 scale;
    scale.r[3].w = 0.0f;
    scale.r[3].z = 0.0f;
    scale.r[3].y = 0.0f;
    scale.r[3].x = 0.0f;
    scale.r[2].w = 0.0f;
    scale.r[2].y = 0.0f;
    scale.r[2].x = 0.0f;
    scale.r[1].w = 0.0f;
    scale.r[1].z = 0.0f;
    scale.r[1].x = 0.0f;
    scale.r[0].w = 0.0f;
    scale.r[0].z = 0.0f;
    float inv = 1.0f / zFar;
    float w = inv * 16777215.0f;
    scale.r[0].y = 0.0f;
    scale.r[3].w = w;
    scale.r[0].x = w;
    scale.r[1].y = w;
    scale.r[2].z = w;
    D3DXMatrixMultiply(&m_Projection, &scale, &m_Projection);
    const float *p = (const float *)&m_Projection;
    XBVertexShader.m_FogScale = scale.r[3].w;
    float b = (p[15] - p[14]) / (p[10] - p[11]) * p[11] + p[15];
    float a = p[15] - p[14] / p[10] * p[11];
    D3DDevice_SetDepthClipPlanes(zNear * a * 16777215.0f, 16777215.0f, 1);
    XBVertexShader.m_ScreenZ = a + 1.0f;
    XBVertexShader.m_1e10 = b - 1.0f;
}

// Vertex shader viewport constants: c72 = 1, c-38 = (h/2, -h/2, 16777215, 1),
// c-37 = (origin + view offset + 0.53125 pixel centre).
void EXCamera::SetupViewport()
{
    float c72[4];
    c72[0] = 1.0f;
    c72[1] = 1.0f;
    c72[2] = 1.0f;
    c72[3] = 1.0f;
    float scale[4];
    scale[0] = ViewSpaceSize().h * 0.5f;
    scale[1] = -(ViewSpaceSize().h * 0.5f);
    scale[2] = 16777215.0f;
    scale[3] = 1.0f;
    float offset[4];
    offset[0] = m_Origin.x + ViewSpaceSize().x + 0.53125f;
    offset[1] = ViewSpaceSize().y + m_Origin.y + 0.53125f;
    offset[2] = 0.0f;
    offset[3] = 0.0f;
    D3DDevice_SetVertexShaderConstantFast(72, c72, 1);
    D3DDevice_SetVertexShaderConstantFast(-38, scale, 1);
    D3DDevice_SetVertexShaderConstantFast(-37, offset, 1);
}

// Shadow / light projector: the shadow controller builds its view and projection, which
// become the display's world-to-view and view-to-cull for the shadow pass.
void EXProjector::UpdateDisplay(unsigned int)
{
    if (m_debugFlags & 0x8000) DebugMessage("Projector UpdateDisplay()");
    XBShadowController.SetPerspective(m_bPerspective);
    XBShadowController.XBGenerateShadowMatrices(this, m_ZNear, m_ZFar);
    EXMaths::mat_44 view = XBShadowController.m_ShadowView;
    EXBaseDisplay::SetWorldToView(view);
    EXMaths::mat_44 proj = XBShadowController.m_ShadowProj;
    EXBaseDisplay::SetViewToCull(proj);
    EXMaths::vec_xyzw one;
    one.x = 1.0f;
    one.y = 1.0f;
    one.z = 1.0f;
    one.w = 1.0f;
    EXBaseDisplay::SetViewToCullScale(EXVector(proj * one));
    EXBaseDisplay::GetDisplay()->UpdateTransforms();
    EXBaseDisplay::SetWorldToShadowCull(proj * view);
}

// Camera: look-at view matrix (z toward the target, up from the world-up vector with
// fallbacks when they are parallel), D3D projection from the vertical field of view and the
// display aspect (x0.75 with aspect correction and again for wide screen), W buffer, viewport,
// the screen-coordinate and cull matrices, and the eye / target for the post effects.
void EXCamera::UpdateDisplay(unsigned int)
{
    if (m_debugFlags & 0x8000) DebugMessage("UpdateDisplay()");
    float half = VFOV() * 0.5f;
    float yZoom = (float)cos(half) / (float)sin(half);
    float aspect = 0.75f;
    if (!m_AspectCorrection) aspect = 1.0f;
    if (EXBaseRenderEnv::WideScreen()) aspect *= 0.75f;
    EXBaseDisplay *pDisp = EXBaseDisplay::GetDisplay();
    float xZoom = (float)pDisp->Width() / (float)pDisp->Height() * aspect * yZoom;
    float vh0 = ViewSpaceSize().h;
    float vh1 = ViewSpaceSize().h;

    // Unit camera position (relative to the target) for the environment map.
    EXVector eye = EXVector(-m_Target) + m_Position;
    eye.w = 0.0f;
    eye.Normalise();
    ((EXDisplay *)EXBaseDisplay::GetDisplay())->SetCameraPosition(m_Position);
    ((EXDisplay *)EXBaseDisplay::GetDisplay())->SetCameraPositionNormalised(eye);
    XBEfx.m_Eye.x = m_Position.x;
    XBEfx.m_Eye.y = m_Position.y;
    XBEfx.m_Eye.z = m_Position.z;
    XBEfx.m_At.x = m_Target.x;
    XBEfx.m_At.y = m_Target.y;
    XBEfx.m_At.z = m_Target.z;

    // Forward.
    EXVector f = m_Target - m_Position;
    float len = f.Length();
    if (len < 1.0e-6f) {
        f.x = 0.0f;
        f.y = 0.0f;
        f.z = 1.0f;
        f.w = 0.0f;
        len = 1.0f;
    }
    f *= 1.0f / len;

    // Up: world up without its forward part; fall back to y, then z.
    EXVector u = m_WorldUp - f * f.Dot(m_WorldUp);
    len = u.Length();
    if (len < 1.0e-6f) {
        u = EXVector(0.0f, 1.0f, 0.0f, 0.0f) - f * f.y;
        len = u.Length();
        if (len < 1.0e-6f) {
            u = EXVector(0.0f, 0.0f, 1.0f, 0.0f) - f * f.z;
            len = u.Length();
            if (len < 1.0e-6f) return;
        }
    }
    u *= 1.0f / len;
    EXVector s = u.Cross(f);

    EXMaths::mat_44 view;
    view.r[0].x = s.x;
    view.r[0].y = u.x;
    view.r[0].z = f.x;
    view.r[0].w = 0.0f;
    view.r[1].x = s.y;
    view.r[1].y = u.y;
    view.r[1].z = f.y;
    view.r[1].w = 0.0f;
    view.r[2].x = s.z;
    view.r[2].y = u.z;
    view.r[2].z = f.z;
    view.r[2].w = 0.0f;
    view.r[3].x = 0.0f - m_Position.Dot(s);
    view.r[3].y = 0.0f - m_Position.Dot(u);
    view.r[3].z = 0.0f - m_Position.Dot(f);
    view.r[3].w = 1.0f;
    EXBaseDisplay::SetWorldToView(view);
    view.r[0].x = s.x;
    view.r[0].y = s.y;
    view.r[0].z = s.z;
    view.r[0].w = 0.0f;
    view.r[1].x = u.x;
    view.r[1].y = u.y;
    view.r[1].z = u.z;
    view.r[1].w = 0.0f;
    view.r[2].x = f.x;
    view.r[2].y = f.y;
    view.r[2].z = f.z;
    view.r[2].w = 0.0f;
    view.r[3].x = 0.0f;
    view.r[3].y = 0.0f;
    view.r[3].z = 0.0f;
    view.r[3].w = 1.0f;
    EXBaseDisplay::SetObjectToVap(view);

    // Projection (D3D style, z 0..1).
    float cx = m_Origin.x + ViewSpaceSize().x;
    float cy = ViewSpaceSize().y + m_Origin.y;
    float *p = (float *)&m_Projection;
    float q = 1.0f / (m_ZFar - m_ZNear);
    float zq = m_ZFar * q;
    float sw = (m_ZFar * m_54 - m_58 * m_ZNear) * q;
    float zOffset = 0.0f - zq * m_ZNear;
    p[1] = 0.0f;
    p[2] = 0.0f;
    p[3] = 0.0f;
    p[0] = xZoom;
    float sz = (m_58 - m_54) * zq * m_ZNear;
    p[4] = 0.0f;
    p[5] = yZoom;
    p[6] = 0.0f;
    p[7] = 0.0f;
    p[8] = 0.0f;
    p[9] = 0.0f;
    p[10] = zq;
    p[11] = 1.0f;
    p[12] = 0.0f;
    p[13] = 0.0f;
    p[14] = zOffset;
    p[15] = 0.0f;
    ((EXDisplay *)EXBaseDisplay::GetDisplay())->SetNormalProjection(m_Projection);
    SetupWBufferProjection();
    SetupViewport();

    // View to screen coordinates (pixels; z and w as the W buffer writes them).
    EXMaths::mat_44 scr;
    scr.r[0].x = vh1 * 0.5f * xZoom;
    scr.r[0].y = 0.0f;
    scr.r[0].z = 0.0f;
    scr.r[0].w = 0.0f;
    scr.r[1].x = 0.0f;
    scr.r[1].y = 0.0f - vh0 * 0.5f * yZoom;
    scr.r[1].z = 0.0f;
    scr.r[1].w = 0.0f;
    scr.r[2].x = cx;
    scr.r[2].y = cy;
    scr.r[2].z = sw;
    scr.r[2].w = 1.0f;
    scr.r[3].x = 0.0f;
    scr.r[3].y = 0.0f;
    scr.r[3].z = sz;
    scr.r[3].w = 0.0f;
    EXBaseDisplay::SetViewToScreen(m_Projection);
    EXBaseDisplay::SetViewToScreenCoords(scr);

    // View to cull space (GL style, z -1..1, x scaled to the 512-wide guard band).
    EXMaths::mat_44 cull;
    const EXRectf &vss = ViewSpaceSize();
    cull.r[0].x = 512.0f / (float)pDisp->Width() * (vss.h / ViewSpaceSize().w) * xZoom;
    cull.r[0].y = 0.0f;
    cull.r[0].z = 0.0f;
    cull.r[0].w = 0.0f;
    EXMaths::vec_xyzw one;
    cull.r[1].x = 0.0f;
    cull.r[1].y = yZoom;
    cull.r[1].z = 0.0f;
    cull.r[1].w = 0.0f;
    cull.r[2].x = 0.0f;
    cull.r[2].y = 0.0f;
    cull.r[2].z = (m_ZFar + m_ZNear) / (m_ZFar - m_ZNear);
    cull.r[2].w = 1.0f;
    cull.r[3].x = 0.0f;
    cull.r[3].y = 0.0f;
    cull.r[3].z = 0.0f - m_ZFar * m_ZNear * 2.0f / (m_ZFar - m_ZNear);
    cull.r[3].w = 0.0f;
    one.x = 1.0f;
    EXBaseDisplay::SetViewToCull(cull);
    one.y = 1.0f;
    one.z = 1.0f;
    one.w = 1.0f;
    EXBaseDisplay::SetViewToCullScale(EXVector(cull * one));
    EXBaseDisplay::GetDisplay()->UpdateTransforms();
}
