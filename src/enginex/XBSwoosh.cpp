#include "EXSwooshSys.h"
#include "EXGeoSwoosh.h"
#include "EXMemory.h"
#include "EXGeoEntity_VtxDataC.h"
#include "EXGeoComplexRefObject.h"
#include "EXGeoFile.h"
#include "EXGeoHeader.h"
#include "EXTexture.h"
#include "EXBaseDisplay.h"
#include "EXDisplay.h"
#include "EXStdFuncs.h"
#include "XBBaseRenderState.h"
#include "XBBaseVertexShader.h"
#include "xbd3d8.h"

// Xbox swoosh (weapon / motion trail) renderer (XBSwoosh.obj). Each trail is a set of rows (one per
// hash point on the item) of segment vertices; consecutive rows are drawn as a triangle strip,
// optionally tessellated along the segments with a Catmull-Rom spline.

// Strip vertices for one pair of rows (up to 512 vertices).
EXGeoEntity_VtxDataC SwooshSplineResult[512];

EXSwooshSys::EXSwooshSys()
{
    m_cc = 0;
}

EXSwooshSys::~EXSwooshSys()
{
}

// Additive or alpha-blended, no Z write, no culling, the trail's texture on stage 0.
void EXSwooshSys::SetSwooshRenderStates()
{
    EXDisplay *pDisp = (EXDisplay *)EXBaseDisplay::GetDisplay();
    EXGeoHeader *pHdr = m_geoRef.GeoFile()->GeoHeader();
    if (!pHdr) return;
    EXGeoSwooshHeader *pSH = m_geoRef.GeoSwooshHeader();
    if (!pSH) return;
    EXGeoSwoosh *pSwoosh = pSH->Object(0);
    if (!pSwoosh) return;
    pDisp->SetDefaultRenderState();
    pDisp->UpdateTransforms();
    xb::D3DXMATRIX m;
    D3DXMatrixMultiply(&m, &EXBaseDisplay::WorldToView(), &EXBaseDisplay::ViewToScreen());
    D3DDevice_SetVertexShaderConstantFast(0, &m, 4);
    XBRenderState.SetRenderState(xb::D3DRS_ZWRITEENABLE, 0);
    XBRenderState.SetRenderState(xb::D3DRS_ZENABLE, 2);
    XBRenderState.SetRenderState(xb::D3DRS_ALPHABLENDENABLE, 1);
    XBRenderState.SetRenderState(xb::D3DRS_FOGENABLE, 0);
    XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 0x302);
    XBRenderState.SetRenderState(xb::D3DRS_CULLMODE, 0);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ALPHAOP, 5);
    XBRenderState.SetTextureStageState(1, xb::D3DTSS_COLOROP, 1);
    if (pSwoosh->BlendMode() & 8) {
        XBRenderState.SetRenderState(xb::D3DRS_BLENDOP, 0x8006);
        XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 1);
    } else if (pSwoosh->BlendMode() & 0x10) {
        XBRenderState.SetRenderState(xb::D3DRS_BLENDOP, 0x800b);
        XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 1);
    } else {
        XBRenderState.SetRenderState(xb::D3DRS_BLENDOP, 0x8006);
        XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 0x303);
    }
    XBVertexShader.SetVertexShader(7);
    XBRenderState.SetTexture(1, 0);
    EXTexture *pTex = &pHdr->TextureArray()[pSwoosh->TextureHash()];
    if (pTex) pTex->Select();
    else XBRenderState.SetTexture(0, 0);
}

// Catmull-Rom through p1..p2 at t (t2 = t*t, t3 = t2*t).
template <class T> __forceinline float SwooshSpline(const T &p0, const T &p1, const T &p2, const T &p3, float t, float t2, float t3)
{
    return ((p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2 + (p1 * 3.0f - p2 * 3.0f + p3 - p0) * t3 + (p2 - p0) * t +
            p1 * 2.0f) * 0.5f;
}

// Vertex at t along segment i of a row of n vertices (colour and alpha splined only when the
// swoosh has colour / alpha controllers).
EXGeoEntity_VtxDataC EXSwooshSys::EXSwooshSplinePos(SwooshVTX_Data *pData, int n, int i, float t)
{
    EXGeoSwooshHeader *pSH = m_geoRef.GeoSwooshHeader();
    EXGeoSwoosh *pSwoosh = pSH ? pSH->Object(0) : 0;
    int i0 = max(0, i - 1);
    int i1 = i;
    int i2 = i + 1;
    int i3 = min(n - 1, i + 2);
    float t2 = t * t;
    float t3 = t2 * t;
    EXVector pos(SwooshSpline(pData[i0].m_pos.x, pData[i1].m_pos.x, pData[i2].m_pos.x, pData[i3].m_pos.x, t, t2, t3),
                 SwooshSpline(pData[i0].m_pos.y, pData[i1].m_pos.y, pData[i2].m_pos.y, pData[i3].m_pos.y, t, t2, t3),
                 SwooshSpline(pData[i0].m_pos.z, pData[i1].m_pos.z, pData[i2].m_pos.z, pData[i3].m_pos.z, t, t2, t3), 1.0f);
    float u = SwooshSpline(pData[i0].m_u, pData[i1].m_u, pData[i2].m_u, pData[i3].m_u, t, t2, t3);
    float v = SwooshSpline(pData[i0].m_v, pData[i1].m_v, pData[i2].m_v, pData[i3].m_v, t, t2, t3);
    ex::RGBA c;
    if (pSwoosh->UseColourController()) {
        c.m_R = (u8)(int)SwooshSpline((int)pData[i0].m_colour.m_R, (int)pData[i1].m_colour.m_R, (int)pData[i2].m_colour.m_R, (int)pData[i3].m_colour.m_R, t, t2, t3);
        c.m_G = (u8)(int)SwooshSpline((int)pData[i0].m_colour.m_G, (int)pData[i1].m_colour.m_G, (int)pData[i2].m_colour.m_G, (int)pData[i3].m_colour.m_G, t, t2, t3);
        c.m_B = (u8)(int)SwooshSpline((int)pData[i0].m_colour.m_B, (int)pData[i1].m_colour.m_B, (int)pData[i2].m_colour.m_B, (int)pData[i3].m_colour.m_B, t, t2, t3);
    } else {
        c.m_R = pData->m_colour.m_R;
        c.m_G = pData->m_colour.m_G;
        c.m_B = pData->m_colour.m_B;
    }
    if (pSwoosh->UseAlphaController())
        c.m_A = (u8)(int)SwooshSpline((int)pData[i0].m_colour.m_A, (int)pData[i1].m_colour.m_A, (int)pData[i2].m_colour.m_A, (int)pData[i3].m_colour.m_A, t, t2, t3);
    else
        c.m_A = pData->m_colour.m_A;
    EXGeoEntity_VtxDataC out;
    *(EXVector *)&out = pos;
    out.m_Colour = c.argb();
    out.m_u = u;
    out.m_v = v;
    return out;
}

// One strip per pair of adjacent rows: the live segments as they are (tessellation 1), or each
// segment split into m_tessalation spline steps.
void EXSwooshSys::DrawTessalatedSwoosh()
{
    int n = (int)m_6c - 1;
    if (n < 4) return;
    EXGeoHeader *pHdr = m_geoRef.GeoFile()->GeoHeader();
    if (!pHdr) return;
    EXGeoSwooshHeader *pSH = m_geoRef.GeoSwooshHeader();
    if (!pSH) return;
    EXGeoSwoosh *pSwoosh = pSH->Object(0);
    if (!pSwoosh) return;
    SetSwooshRenderStates();
    for (unsigned int h = 0; h < pSwoosh->NumHashs() - 1; h++) {
        unsigned int count = 0;
        if (m_tessalation == 1) {
            SwooshVTX_Data *pA = m_pRows[h];
            SwooshVTX_Data *pB = m_pRows[h + 1];
            EXGeoEntity_VtxDataC *pOut = SwooshSplineResult;
            if (n) {
                count = n * 2;
                int s = n;
                do {
                    *(EXVector *)pOut = pA->m_pos;
                    pOut->m_u = pA->m_u;
                    pOut->m_v = pA->m_v;
                    pOut->m_Colour = pA->m_colour.argb();
                    pOut++;
                    *(EXVector *)pOut = pB->m_pos;
                    pOut->m_u = pB->m_u;
                    pOut->m_v = pB->m_v;
                    pOut->m_Colour = pB->m_colour.argb();
                    pOut++;
                    pA++;
                    pB++;
                } while (--s);
            }
        } else {
            SwooshVTX_Data *pA = m_pRows[h];
            SwooshVTX_Data *pB = m_pRows[h + 1];
            float f = (float)m_tessalation;
            float step = f != 0.0f ? 1.0f / f : 0.0f;
            for (unsigned int s = 0; s < (unsigned int)(n - 1); s++) {
                float t = 0.0f;
                for (unsigned int k = 0; k < m_tessalation; k++) {
                    SwooshSplineResult[count++] = EXSwooshSplinePos(pA, n, s, t);
                    SwooshSplineResult[count++] = EXSwooshSplinePos(pB, n, s, t);
                    t += step;
                }
            }
        }
        if (count > 2) D3DDevice_DrawVerticesUP(6, count, SwooshSplineResult, sizeof(EXGeoEntity_VtxDataC));
    }
    XBRenderState.SetRenderState(xb::D3DRS_BLENDOP, 0x8006);
    XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 0x302);
    XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 0x303);
    XBRenderState.SetRenderState(xb::D3DRS_CULLMODE, 0x901);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ALPHAOP, 4);
}

void EXSwooshSys::Draw(u32 pass)
{
    if (3.0f > m_6c) return;
    DrawTessalatedSwoosh();
}
