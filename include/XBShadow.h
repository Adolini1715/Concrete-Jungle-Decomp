#ifndef XBSHADOW_H
#define XBSHADOW_H
#include "types.h"
#include "EXDisplay.h"

namespace xb { struct D3DTexture; struct D3DSurface; struct _D3DMATRIX; }
class EXProjector;

// Shadow and light map renderer (XBShadow.obj; the global XBShadowController at 0x4ce0f8).
// The shadow map is a 256x256 R5G6B5 render target drawn from the projector (cleared white, casters
// drawn dark); the light map shares the display's black null texture.
class XBShadow {
public:
    void Init();
    void XBGenerateShadowProjectionVector(xb::_D3DMATRIX *pObjectToWorld);
    void SetupWBufferProjection(xb::_D3DMATRIX *pProj, float zNear, float zFar);
    void RenderDebug(float x, float y);
    void XBStartShadowRender();
    void XBEndShadowRender();
    void XBStartLightRender();
    void XBEndLightRender();
    // No shadows or lights this frame: sample a constant texture instead.
    xb::D3DTexture *DisableShadowMap() { return m_pShadowMap = ((EXDisplay *)EXBaseDisplay::GetDisplay())->NullTexture(); }
    xb::D3DTexture *DisableLightMap() { return m_pLightMap = ((EXDisplay *)EXBaseDisplay::GetDisplay())->NullTextureBlack(); }

    // Appended by the XBGeoEntity agent.
    xb::D3DTexture *ShadowTexture() const { return m_pShadowMap; }
    void XBGenerateShadowUVMatrix(xb::_D3DMATRIX *pObjectToWorld);

    xb::D3DSurface *m_pOldTarget;   // 0x00 render target and depth buffer to restore
    xb::D3DSurface *m_pOldDepth;    // 0x04
    xb::D3DSurface *m_pShadowSurface;   // 0x08
    xb::D3DSurface *m_pLightSurface;    // 0x0c
    xb::D3DTexture *m_pShadowTexture;   // 0x10 created on the first shadow render
    u32 m_14;
    u32 m_18;
    xb::D3DTexture *m_pLightTexture;    // 0x1c
    xb::D3DTexture *m_pShadowMap;   // 0x20
    xb::D3DTexture *m_pLightMap;    // 0x24
    void SetPerspective(signed char b) { m_Perspective = b; }   // XBCamera.obj
    void XBGenerateShadowMatrices(EXProjector *pProj, float zNear, float zFar);

    s8 m_Perspective;               // 0x28
    u8 m_29[3];
    EXMaths::mat_44 m_ShadowView;   // 0x2c world to projector view (EXProjector::UpdateDisplay)
    EXMaths::mat_44 m_ShadowProj;   // 0x6c projector view to clip
    EXMaths::mat_44 m_WorldToShadowClip;  // 0xac shadow casters are drawn with ObjectToWorld * this
    EXMaths::mat_44 m_WorldToShadowUV;    // 0xec world to shadow map texture coordinates
    u8 m_12c[0x40];
};
extern float g_shadowfov;       // orthographic shadow width (world units)
extern float g_shadowpersfov;   // perspective shadow field of view (degrees)
extern XBShadow XBShadowController;
extern int XBRenderedShadows;
extern unsigned int XBRenderingShadows;
extern unsigned int XBShadowObjectId;
extern unsigned int XBRenderingLights;
#endif
