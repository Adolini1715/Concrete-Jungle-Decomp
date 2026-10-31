#include "XBShadow.h"
#include "EXCamera.h"
#include "EXBaseDisplay.h"
#include "EXDisplay.h"
#include "XBBaseVertexShader.h"
#include "xbd3d8.h"

// Projected shadows (XBShadow.obj). Shadow casters are drawn from the projector into a 256x256
// render target; receivers sample it through m_WorldToShadowUV (vertex shader c25..c27).

long D3dCreateTexture(unsigned int w, unsigned int h, unsigned int levels, unsigned long usage, xb::D3DFORMAT fmt,
                      unsigned long pool, xb::D3DTexture **ppTex);
extern "C" xb::D3DXMATRIX *__stdcall D3DXMatrixTranspose(xb::D3DXMATRIX *pOut, const xb::D3DXMATRIX *pM);
extern "C" xb::D3DXVECTOR3 *__stdcall D3DXVec3TransformNormal(xb::D3DXVECTOR3 *pOut, const xb::D3DXVECTOR3 *pV,
                                                              const xb::D3DXMATRIX *pM);
extern "C" xb::D3DXMATRIX *__stdcall D3DXMatrixLookAtLH(xb::D3DXMATRIX *pOut, const xb::D3DXVECTOR3 *pEye,
                                                        const xb::D3DXVECTOR3 *pAt, const xb::D3DXVECTOR3 *pUp);
extern "C" xb::D3DXMATRIX *__stdcall D3DXMatrixPerspectiveFovLH(xb::D3DXMATRIX *pOut, float fovy, float aspect,
                                                                float zn, float zf);
extern "C" xb::D3DXMATRIX *__stdcall D3DXMatrixOrthoLH(xb::D3DXMATRIX *pOut, float w, float h, float zn, float zf);

XBShadow XBShadowController;
float g_shadowfov = 8.0f;
float g_shadowpersfov = 22.5f;

void XBShadow::Init()
{
    m_Perspective = 1;
    m_pShadowTexture = 0;
    m_pLightTexture = 0;
    m_pShadowMap = ((EXDisplay *)EXBaseDisplay::GetDisplay())->NullTexture();
    m_pLightMap = ((EXDisplay *)EXBaseDisplay::GetDisplay())->NullTextureBlack();
}

// Switch rendering to the shadow map (creating it the first time) and clear it to white.
void XBShadow::XBStartShadowRender()
{
    if (!m_pShadowTexture && D3dCreateTexture(0x100, 0x100, 1, 1, xb::D3DFMT_R5G6B5, 0, &m_pShadowTexture)) {
        m_pShadowTexture = ((EXDisplay *)EXBaseDisplay::GetDisplay())->NullTexture();
        return;
    }
    m_pShadowMap = m_pShadowTexture;
    m_pOldDepth = D3DDevice_GetDepthStencilSurface2();
    m_pOldTarget = D3DDevice_GetRenderTarget2();
    xb::D3DSurface *pSurf = m_pShadowSurface = D3DTexture_GetSurfaceLevel2(m_pShadowMap, 0);
    if (pSurf) {
        ((EXDisplay *)EXBaseDisplay::GetDisplay())->SetRenderTarget(pSurf, 0);
        D3DDevice_Clear(0, 0, 0xf0, 0xffffffff, 0.0f, 0);
    }
}

void XBShadow::XBEndShadowRender()
{
    ((EXDisplay *)EXBaseDisplay::GetDisplay())->SetRenderTarget(m_pOldTarget, m_pOldDepth);
    D3DResource_Release(m_pOldTarget);
    D3DResource_Release(m_pOldDepth);
    D3DResource_Release(m_pShadowSurface);
}

// The light map: the display's black null texture, cleared to opaque black.
void XBShadow::XBStartLightRender()
{
    m_pLightMap = m_pLightTexture = ((EXDisplay *)EXBaseDisplay::GetDisplay())->NullTextureBlack();
    m_pOldDepth = D3DDevice_GetDepthStencilSurface2();
    m_pOldTarget = D3DDevice_GetRenderTarget2();
    xb::D3DSurface *pSurf = m_pLightSurface = D3DTexture_GetSurfaceLevel2(m_pLightMap, 0);
    if (pSurf) {
        ((EXDisplay *)EXBaseDisplay::GetDisplay())->SetRenderTarget(pSurf, 0);
        D3DDevice_Clear(0, 0, 0xf0, 0xff000000, 0.0f, 0);
    }
}

void XBShadow::XBEndLightRender()
{
    ((EXDisplay *)EXBaseDisplay::GetDisplay())->SetRenderTarget(m_pOldTarget, m_pOldDepth);
    D3DResource_Release(m_pOldTarget);
    D3DResource_Release(m_pOldDepth);
    D3DResource_Release(m_pLightSurface);
}

// The direction shadows are cast in, in object space (c73: minus the projector's view z axis).
void XBShadow::XBGenerateShadowProjectionVector(xb::_D3DMATRIX *pObjectToWorld)
{
    xb::D3DXMATRIX m;
    D3DXMatrixTranspose(&m, (xb::D3DXMATRIX *)pObjectToWorld);
    xb::D3DXVECTOR3 dir;
    dir.x = m_ShadowView.r[0].z;
    dir.y = m_ShadowView.r[1].z;
    dir.z = m_ShadowView.r[2].z;
    xb::D3DXVECTOR3 out;
    D3DXVec3TransformNormal(&out, &dir, &m);
    EXMaths::vec_xyzw v;
    v.x = 0.0f - out.x;
    v.y = 0.0f - out.y;
    v.z = 0.0f - out.z;
    v.w = 1.0f;
    D3DDevice_SetVertexShaderConstantFast(73, &v, 1);
}

// W buffer for the shadow pass: w runs 0..16777215 over 0..far (as EXCamera's).
void XBShadow::SetupWBufferProjection(xb::_D3DMATRIX *pProj, float, float zFar)
{
    EXMaths::mat_44 scale;
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
    scale.r[0].y = 0.0f;
    float w = 16777215.0f / zFar;
    scale.r[3].w = w;
    scale.r[0].x = w;
    scale.r[1].y = w;
    scale.r[2].z = w;
    D3DXMatrixMultiply(pProj, &scale, pProj);
    XBVertexShader.m_FogScale = scale.r[3].w;
}

void XBShadow::XBGenerateShadowUVMatrix(xb::_D3DMATRIX *pObjectToWorld)
{
    xb::D3DXMATRIX m;
    D3DXMatrixMultiply(&m, pObjectToWorld, &m_WorldToShadowUV);
    D3DXMatrixTranspose(&m, &m);
    D3DDevice_SetVertexShaderConstantFast(25, &m, 3);
}

void XBShadow::RenderDebug(float, float)
{
    EXMaths::mat_44 view = m_ShadowView;
    EXMaths::mat_44 inv = view.inverse();
}

// View, projection and clip matrices of the shadow projector, and the matrix from world space to
// the shadow map's texture coordinates.
void XBShadow::XBGenerateShadowMatrices(EXProjector *pProj, float, float)
{
    D3DXMatrixLookAtLH((xb::D3DXMATRIX *)&m_ShadowView, (xb::D3DXVECTOR3 *)&pProj->m_Position,
                       (xb::D3DXVECTOR3 *)&pProj->ProjectAt(), (xb::D3DXVECTOR3 *)&pProj->m_WorldUp);
    if (m_Perspective == 1)
        D3DXMatrixPerspectiveFovLH((xb::D3DXMATRIX *)&m_ShadowProj, g_shadowpersfov * 0.017453292f, 1.0f, 0.0001f, 1000.0f);
    else
        D3DXMatrixOrthoLH((xb::D3DXMATRIX *)&m_ShadowProj, g_shadowfov, g_shadowfov, 0.0001f, 1000.0f);
    EXMaths::mat_44 scale;
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
    scale.r[0].y = 0.0f;
    scale.r[3].w = 16777.215f;
    scale.r[0].x = 16777.215f;
    scale.r[1].y = 16777.215f;
    scale.r[2].z = 16777.215f;
    D3DXMatrixMultiply(&m_ShadowProj, &scale, &m_ShadowProj);
    XBVertexShader.m_FogScale = scale.r[3].w;
    D3DXMatrixMultiply(&m_WorldToShadowClip, &m_ShadowView, &m_ShadowProj);
    xb::D3DXMATRIX uv(*(const xb::_D3DMATRIX *)&m_ShadowProj);
    if (!m_Perspective) {
        uv._42 = 0.0f;
        float s = 1.0f / (g_shadowfov * 0.5f);
        uv._41 = 0.0f;
        uv._32 = 0.0f;
        uv._31 = 0.0f;
        uv._24 = 0.0f;
        uv._23 = 0.0f;
        uv._21 = 0.0f;
        uv._14 = 0.0f;
        uv._13 = 0.0f;
        uv._12 = 0.0f;
        uv._11 = s;
        uv._22 = s;
        uv._33 = 0.0f;
        uv._34 = 1.0f;
        uv._43 = 1.0f;
        uv._44 = 0.0f;
    }
    EXMaths::mat_44 bias;
    bias.r[3].z = 0.0f;
    bias.r[3].y = 0.0f;
    bias.r[3].x = 0.0f;
    bias.r[2].w = 0.0f;
    bias.r[1].w = 0.0f;
    bias.r[1].z = 0.0f;
    bias.r[1].x = 0.0f;
    bias.r[0].w = 0.0f;
    bias.r[0].z = 0.0f;
    bias.r[0].y = 0.0f;
    bias.r[3].w = 1.0f;
    bias.r[2].z = 1.0f;
    bias.r[0].x = 0.5f;
    bias.r[1].y = -0.5f;
    bias.r[2].x = 0.5f;
    bias.r[2].y = 0.5f;
    D3DXMatrixMultiply(&uv, &uv, &bias);
    D3DXMatrixMultiply(&m_WorldToShadowUV, &m_ShadowView, &uv);
}
