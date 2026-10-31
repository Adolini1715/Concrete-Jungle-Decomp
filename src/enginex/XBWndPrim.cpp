#include "EXWnd.h"
#include "EXPolyController.h"
#include "EXPrimVert.h"
#include "LineController.h"
#include "EXMemHeap.h"
#include "EXMemory.h"

#include "EXTexture.h"
#include "EXDisplay.h"
#include "EXBaseDisplay.h"
#include "XBBaseRenderState.h"
#include "XBBaseVertexShader.h"
#include "RGBA.h"
#include "EXVector.h"

// Batched window polygons: each AddPoly copies a fan or strip (up to 29 vertices) into the next
// 0x334-byte record with the current texture, blend mode and flags; RenderPolys draws them all
// with fixed vertex shader 7 at the end of the frame.
struct XBPolyVertex {           // 0x1c
    EXVector m_Pos;             // 0x00
    u32 m_Color;                // 0x10 a r g b
    float m_U, m_V;             // 0x14
};
struct XBPoly {                 // 0x334
    u32 m_Flags;                // 0x00 controller flags | prim type (0 fan, 1 strip)
    u32 m_Count;                // 0x04
    XBPolyVertex m_Verts[29];   // 0x08
};
#define POLY(i) (((XBPoly *)m_pPolys)[i])

void *XBHeap1Alloc2(int size, int flags)
{
    EXMemHeap *pHeap = _SystemHeapList[1];
    return pHeap->_EXAlloc(size, flags);
}

EXPolyController::EXPolyController()
{
    m_NumPolys = 0;
    m_pPolys = 0;
    m_pPolyTexture = 0;
    SetTriStripBlendMode(0);
    SetTriStripZWriteMode(1);
    SetTriStripZReadMode(1);
    SetTriFanTexture(0);
    SetTriFanBlendMode(0);
    SetTriFanZWriteMode(1);
    SetTriFanZReadMode(1);
    SetPrimType(0);
    m_pPolys = (char *)XBHeap1Alloc2(0xcd000, 0);
    m_pPolyTexture = (EXTexture **)XBHeap1Alloc2(0x1000, 0);
    m_pPolyBlend = (unsigned int *)XBHeap1Alloc2(0x1000, 0);
    m_Flags[1] = 0;
    m_Flags[0] = 0;
}

EXPolyController::~EXPolyController()
{
    if (m_pPolys) {
        EXFree(m_pPolys);
        m_pPolys = 0;
    }
    if (m_pPolyTexture) {
        EXFree(m_pPolyTexture);
        m_pPolyTexture = 0;
    }
    if (m_pPolyBlend) {
        EXFree(m_pPolyBlend);
        m_pPolyBlend = 0;
    }
}

void EXPolyController::Reset()
{
    SetTriFanTexture(0);
    SetTriFanBlendMode(0);
    SetTriFanBackFaceCulling(0);
    SetTriFanZWriteMode(1);
    SetTriFanZReadMode(1);
    SetTriStripTexture(0);
    SetTriStripBlendMode(0);
    SetTriStripBackFaceCulling(0);
    SetTriStripZWriteMode(1);
    SetTriStripZReadMode(1);
    m_NumPolys = 0;
    SetPrimType(0);
}

void EXPolyController::AddPoly(unsigned int n, EXPrimVert *p)
{
    if (m_NumPolys >= 0x400) return;
    m_pPolyTexture[m_NumPolys] = m_pTexture[m_PrimType];
    m_pPolyBlend[m_NumPolys] = m_BlendMode[m_PrimType];
    POLY(m_NumPolys).m_Count = n;
    POLY(m_NumPolys).m_Flags = m_Flags[m_PrimType] | m_PrimType;
    for (unsigned int i = 0; i < n; i++) {
        POLY(m_NumPolys).m_Verts[i].m_Pos = EXVector(p[i].x, p[i].y, p[i].z, p[i].w);
        POLY(m_NumPolys).m_Verts[i].m_U = p[i].u;
        POLY(m_NumPolys).m_Verts[i].m_V = p[i].v;
        POLY(m_NumPolys).m_Verts[i].m_Color = ((const ex::RGBA *)&p[i].m_Color)->argb();
    }
    m_NumPolys++;
}

// Blend modes: 0 alpha, 2 additive, 3 subtractive. Texture scroll offsets go to c29.
void EXPolyController::RenderPolys()
{
    EXDisplay *pDisp = (EXDisplay *)EXBaseDisplay::GetDisplay();
    if (!pDisp || !pDisp->m_pDevice || !m_NumPolys) return;
    pDisp->SelectNullTexture();
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ALPHAOP, 5);
    XBRenderState.SetTexture(1, 0);
    XBRenderState.SetTextureStageState(1, xb::D3DTSS_COLOROP, 1);
    XBRenderState.SetRenderState(xb::D3DRS_BLENDOP, 0x8006);
    XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 0x302);
    XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 0x303);
    XBRenderState.SetRenderState(xb::D3DRS_ZWRITEENABLE, 0);
    XBRenderState.SetRenderState(xb::D3DRS_CULLMODE, 0);
    XBRenderState.SetRenderState(xb::D3DRS_ALPHATESTENABLE, 1);
    XBRenderState.SetRenderState(xb::D3DRS_ALPHABLENDENABLE, 1);
    unsigned int lastBlend = 0xffffffff;
    EXMaths::mat_44 m;
    D3DXMatrixMultiply(&m, &EXBaseDisplay::WorldToView(), &EXBaseDisplay::ViewToScreen());
    D3DDevice_SetVertexShaderConstantFast(0, &m, 4);
    XBVertexShader.SetVertexShader(7);
    XBRenderState.SetTexture(1, 0);
    float c[4];
    c[0] = 0.0f;
    c[1] = 0.0f;
    c[2] = 0.0f;
    c[3] = 0.0f;
    for (unsigned int i = 0; i < m_NumPolys; i++) {
        XBRenderState.SetRenderState(xb::D3DRS_CULLMODE, (s8)((POLY(i).m_Flags & 8) == 8) * 0x901);
        XBRenderState.SetRenderState(xb::D3DRS_ZWRITEENABLE, (s8)((POLY(i).m_Flags & 4) == 4));
        XBRenderState.SetRenderState(xb::D3DRS_ZFUNC, (POLY(i).m_Flags & 2) ? 0x203 : 0x207);
        if (m_pPolyTexture[i]) {
            c[0] = m_pPolyTexture[i]->UOffset();
            c[1] = m_pPolyTexture[i]->VOffset();
            m_pPolyTexture[i]->Select();
        } else {
            XBRenderState.SetTexture(0, ((EXDisplay *)EXBaseDisplay::GetDisplay())->NullTextureBlack());
        }
        D3DDevice_SetVertexShaderConstantFast(29, c, 1);
        if (lastBlend != m_pPolyBlend[i]) {
            switch (m_pPolyBlend[i]) {
            case 2:
                XBRenderState.SetRenderState(xb::D3DRS_BLENDOP, 0x8006);
                XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 0x302);
                XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 1);
                break;
            case 3:
                XBRenderState.SetRenderState(xb::D3DRS_BLENDOP, 0x800b);
                XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 0x302);
                XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 1);
                break;
            default:
                XBRenderState.SetRenderState(xb::D3DRS_BLENDOP, 0x8006);
                XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 0x302);
                XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 0x303);
                break;
            }
            lastBlend = m_pPolyBlend[i];
        }
        xb::D3DDevice::DrawPrimitiveUP((xb::D3DPRIMITIVETYPE)((POLY(i).m_Flags & 1) ? 6 : 7), POLY(i).m_Count - 2,
                                       POLY(i).m_Verts, sizeof(XBPolyVertex));
    }
    c[0] = 0.0f;
    c[1] = 0.0f;
    c[2] = 0.0f;
    c[3] = 0.0f;
    D3DDevice_SetVertexShaderConstantFast(29, c, 1);
    XBRenderState.SetRenderState(xb::D3DRS_BLENDOP, 0x8006);
    XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 0x302);
    XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 0x303);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ALPHAOP, 4);
    XBRenderState.SetRenderState(xb::D3DRS_ZWRITEENABLE, 1);
    XBRenderState.SetRenderState(xb::D3DRS_ZFUNC, 0x203);
    XBRenderState.SetRenderState(xb::D3DRS_CULLMODE, 0x901);
    Reset();
}

void EXWnd::DrawLines(unsigned int n, EXPrimVert *p)
{
    LineControl.AddLines(n, p);
}

void EXWnd::SetTriFanBackFaceCulling(s8 cull)
{
    g_ExPolyController.SetTriFanBackFaceCulling(cull);
}

void EXWnd::SetTriFanZMode(s8 write, s8 read)
{
    g_ExPolyController.SetTriFanZWriteMode(write);
    g_ExPolyController.SetTriFanZReadMode(read);
}

void EXWnd::SetTriFanBlendMode(unsigned int mode)
{
    g_ExPolyController.SetTriFanBlendMode(mode);
}

void EXWnd::SelectTriFanTexture(EXTexture *pTex)
{
    g_ExPolyController.SetTriFanTexture(pTex);
}

void EXWnd::SetTriStripBackFaceCulling(s8 cull)
{
    g_ExPolyController.SetTriStripBackFaceCulling(cull);
}

void EXWnd::SetTriStripZMode(s8 write, s8 read)
{
    g_ExPolyController.SetTriStripZWriteMode(write);
    g_ExPolyController.SetTriStripZReadMode(read);
}

void EXWnd::SetTriStripBlendMode(unsigned int mode)
{
    g_ExPolyController.SetTriStripBlendMode(mode);
}

void EXWnd::SelectTriStripTexture(EXTexture *pTex)
{
    g_ExPolyController.SetTriStripTexture(pTex);
}

static const float s_EPadInit = 0.0f;
static float s_EPad = s_EPadInit;
EXPolyController g_ExPolyController;

void EXWnd::DrawTriFan(unsigned int n, EXPrimVert *p)
{
    g_ExPolyController.AddPoly(n, p);
}

void EXWnd::DrawTriStrip(unsigned int n, EXPrimVert *p)
{
    g_ExPolyController.SetPrimType(1);
    g_ExPolyController.AddPoly(n, p);
    g_ExPolyController.SetPrimType(0);
}

void EXWnd::DrawLine(const EXPrimVert &a, const EXPrimVert &b)
{
    D3DV_L v[2];
    v[0].x = a.x;
    v[0].y = a.y;
    v[0].z = a.z;
    v[0].m_Color = a.m_Color;
    v[1].x = b.x;
    v[1].y = b.y;
    v[1].z = b.z;
    v[1].m_Color = b.m_Color;
    LineControl.AddLine(&v[0], &v[1]);
}
