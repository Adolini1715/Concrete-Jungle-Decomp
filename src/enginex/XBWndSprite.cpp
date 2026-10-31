#include "EXWnd.h"
#include "EXSprite2D.h"
#include "EXPointf.h"
#include "EXStdFuncs.h"
#include "EXMemory.h"
#include "EXTexture.h"
#include "EXDisplay.h"
#include "EXBaseDisplay.h"
#include "XBBaseRenderState.h"
#include "XBBaseVertexShader.h"
#include <math.h>

// Sprite2D command records queued by AddSprite2DData (type in the first dword).
enum { SPR2D_TEXTURE = 1, SPR2D_CLIPRECT = 2, SPR2D_BLENDMODE = 3, SPR2D_SPRITE = 4, SPR2D_ROTSPRITE = 5,
       SPR2D_FLEXISPRITE = 6 };
struct Sprite2DTextureCmd {
    int m_Type;
    EXTexture *m_pTexture;
    s8 m_A : 1;     // filtered
    s8 m_B : 1;     // tiled
    u8 m_Pad[3];
    int m_Unkc;
};
struct Sprite2DClipCmd {
    int m_Type;
    short x, y;
    u16 w, h;
};
struct Sprite2DBlendCmd {
    int m_Type;
    unsigned int m_Mode;
};
struct Sprite2DCmd {
    int m_Type;
    EXSprite2D m_Sprite;
};
struct RotSprite2DCmd {
    int m_Type;
    EXRotSprite2D m_Sprite;
};
struct FlexiSprite2DCmd {
    int m_Type;
    EXFlexiSprite2D m_Sprite;
};

void EXWnd::_SelectSprite2DTexture(EXTexture *pTex, s8 a, s8 b)
{
    Sprite2DTextureCmd cmd;
    m_pSprite2DTexture = pTex;
    cmd.m_pTexture = pTex;
    cmd.m_A = a;
    cmd.m_B = b;
    cmd.m_Type = SPR2D_TEXTURE;
    AddSprite2DData(&cmd, sizeof(cmd));
}

void EXWnd::_SetSprite2DClipRect(short x, short y, unsigned short w, unsigned short h)
{
    Sprite2DClipCmd cmd;
    cmd.x = m_PrimRect.x + x;
    cmd.y = m_PrimRect.y + y;
    cmd.w = w;
    cmd.m_Type = SPR2D_CLIPRECT;
    cmd.h = h;
    AddSprite2DData(&cmd, sizeof(cmd));
}

void EXWnd::_SetSprite2DBlendMode(unsigned int mode)
{
    Sprite2DBlendCmd cmd;
    cmd.m_Type = SPR2D_BLENDMODE;
    cmd.m_Mode = mode;
    AddSprite2DData(&cmd, sizeof(cmd));
}

void EXWnd::_DrawSprite2D(EXFlexiSprite2D &s)
{
    FlexiSprite2DCmd cmd;
    cmd.m_Type = SPR2D_FLEXISPRITE;
    EXFlexiSprite2D::Vert *pSrc = s.m_Verts;
    cmd.m_Sprite.m_Verts[0] = *pSrc;
    cmd.m_Sprite.m_Verts[0].x += m_PrimRect.x;
    cmd.m_Sprite.m_Verts[0].y += m_PrimRect.y;
    pSrc++;
    cmd.m_Sprite.m_Verts[1] = *pSrc;
    cmd.m_Sprite.m_Verts[1].x += m_PrimRect.x;
    cmd.m_Sprite.m_Verts[1].y += m_PrimRect.y;
    pSrc++;
    cmd.m_Sprite.m_Verts[2] = *pSrc;
    cmd.m_Sprite.m_Verts[2].x += m_PrimRect.x;
    cmd.m_Sprite.m_Verts[2].y += m_PrimRect.y;
    pSrc++;
    cmd.m_Sprite.m_Verts[3] = *pSrc;
    cmd.m_Sprite.m_Verts[3].x += m_PrimRect.x;
    cmd.m_Sprite.m_Verts[3].y += m_PrimRect.y;
    AddSprite2DData(&cmd, sizeof(cmd));
}

EXPointf Translate(float x, float y, EXPointf p)
{
    EXPointf r;
    r.x = p.x + x;
    r.y = p.y + y;
    return r;
}

// NEAR: 34 bytes differ, SSE register allocation around fsin/fcos
EXPointf Rotate(float angle, EXPointf p)
{
    float s = (float)sin((double)angle);
    float sy = s * p.y;
    float sx = s * p.x;
    float c = (float)cos((double)angle);
    EXPointf r;
    r.x = c * p.x - sy;
    r.y = c * p.y + sx;
    return r;
}

// NEAR: 24 bytes differ, retail loads sin into xmm4 before fcos (register allocation)
EXPointf CalcRotation(float cx, float cy, float angle, EXPointf p)
{
    float dx = p.x - cx;
    float dy = p.y - cy;
    float s = (float)sin((double)angle);
    float c = (float)cos((double)angle);
    return EXPointf(c * dx - s * dy + cx, c * dy + s * dx + cy);
}

void EXWnd::_DrawSprite2D(EXSprite2D &s)
{
    Sprite2DCmd cmd;
    cmd.m_Type = SPR2D_SPRITE;
    cmd.m_Sprite.m_Color = s.m_Color;
    cmd.m_Sprite.m_Color.m_A = min(cmd.m_Sprite.m_Color.m_A, (u8)0x80);
    cmd.m_Sprite.x = m_PrimRect.x + s.x;
    cmd.m_Sprite.y = m_PrimRect.y + s.y;
    cmd.m_Sprite.w = s.w;
    cmd.m_Sprite.h = s.h;
    cmd.m_Sprite.u0 = s.u0;
    cmd.m_Sprite.v0 = s.v0;
    cmd.m_Sprite.u1 = s.u1;
    cmd.m_Sprite.v1 = s.v1;
    AddSprite2DData(&cmd, sizeof(cmd));
}

void EXWnd::_DrawSprite2D(EXRotSprite2D &s)
{
    RotSprite2DCmd cmd;
    cmd.m_Type = SPR2D_ROTSPRITE;
    cmd.m_Sprite.m_Color = s.m_Color;
    cmd.m_Sprite.m_Color.m_A = min(cmd.m_Sprite.m_Color.m_A, (u8)0x80);
    cmd.m_Sprite.x = m_PrimRect.x + s.x;
    cmd.m_Sprite.y = m_PrimRect.y + s.y;
    cmd.m_Sprite.w = s.w;
    cmd.m_Sprite.h = s.h;
    cmd.m_Sprite.u0 = s.u0;
    cmd.m_Sprite.v0 = s.v0;
    cmd.m_Sprite.u1 = s.u1;
    cmd.m_Sprite.v1 = s.v1;
    cmd.m_Sprite.m_Unk1c = s.m_Unk1c;
    cmd.m_Sprite.m_Angle = -s.m_Angle;
    AddSprite2DData(&cmd, sizeof(cmd));
}

// Sprite command block (EXWndSprite.obj): bytes used, then the records from +0x10.
struct XBSprite2DBlock {
    u32 m_Used;
    u32 m_Pad[3];
    u8 m_Data[0x1ff0];
};

// One screen-space quad, drawn as a 4-vertex fan with fixed vertex shader 6 (sprite2D).
struct XBSPRITE2D {
    float x, y, z;
    unsigned int diffuse;
    float u, v;
};
static XBSPRITE2D s_Sprite2DVerts[4];

// Plays back the window's queued sprite commands: texture (c29 = its scroll offsets, filter and
// tiling), clip rect (viewport = rect within the window's clip rect), blend mode (0 alpha, 1 opaque
// ONE/ZERO, 2 additive, 3 reverse subtract) and the three sprite kinds. Corners are offset by
// 0.53125 pixels; a rotated sprite turns its corners about the centre by one angle and its UVs about
// the UV centre by another. Restores the window viewport, clamped linear sampling and Z writes.
void EXWnd::DrawSprite2DData()
{
    float z = XBVertexShader.m_ScreenZ;
    s_Sprite2DVerts[0].z = z;
    s_Sprite2DVerts[1].z = z;
    s_Sprite2DVerts[2].z = z;
    s_Sprite2DVerts[3].z = z;
    XBVertexShader.SetVertexShader(6);
    float c[4];
    c[0] = 0.0f;
    c[1] = 0.0f;
    c[2] = 0.0f;
    c[3] = 0.0f;
    D3DDevice_SetVertexShaderConstant1Fast(29 + 96, c);
    XBRenderState.SetRenderState(xb::D3DRS_BLENDOP, 0x8006);
    XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 0x302);
    XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 0x303);
    XBRenderState.SetRenderState(xb::D3DRS_ALPHABLENDENABLE, 1);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ALPHAOP, 5);
    XBRenderState.SetTexture(0, ((EXDisplay *)EXBaseDisplay::GetDisplay())->NullTextureBlack());
    XBRenderState.SetRenderState(xb::D3DRS_ZWRITEENABLE, 0);
    for (u32 i = 0; i < m_Sprite2DBlocks.Size(); i++) {
        XBSprite2DBlock *pBlock = (XBSprite2DBlock *)m_Sprite2DBlocks[i];
        if (!pBlock) continue;
        u32 left = pBlock->m_Used;
        u8 *p = pBlock->m_Data;
        while (left > 0) {
            switch (*(int *)p) {
            case SPR2D_TEXTURE: {
                Sprite2DTextureCmd *pCmd = (Sprite2DTextureCmd *)p;
                if (pCmd->m_pTexture) {
                    c[0] = 0.0f;
                    c[1] = 0.0f;
                    c[0] = pCmd->m_pTexture->UOffset();
                    c[1] = pCmd->m_pTexture->VOffset();
                    D3DDevice_SetVertexShaderConstant1Fast(29 + 96, c);
                    pCmd->m_pTexture->Select();
                } else {
                    XBRenderState.SetTexture(0, ((EXDisplay *)EXBaseDisplay::GetDisplay())->NullTextureBlack());
                }
                pCmd->m_pTexture->SetFiltered(pCmd->m_A);
                pCmd->m_pTexture->SetTiling(pCmd->m_B);
                left -= sizeof(Sprite2DTextureCmd);
                p += sizeof(Sprite2DTextureCmd);
                break;
            }
            case SPR2D_CLIPRECT: {
                Sprite2DClipCmd *pCmd = (Sprite2DClipCmd *)p;
                EXRect r(pCmd->x, pCmd->y, pCmd->w, pCmd->h);
                r = IntersectRect(r, m_PrimClipRect);
                ((EXDisplay *)EXBaseDisplay::GetDisplay())->SetViewPort(&r, &r);
                left -= sizeof(Sprite2DClipCmd);
                p += sizeof(Sprite2DClipCmd);
                break;
            }
            case SPR2D_SPRITE: {
                EXSprite2D &s = ((Sprite2DCmd *)p)->m_Sprite;
                s_Sprite2DVerts[0].diffuse = s.m_Color.argb();
                s_Sprite2DVerts[1].diffuse = s.m_Color.argb();
                s_Sprite2DVerts[2].diffuse = s.m_Color.argb();
                s_Sprite2DVerts[3].diffuse = s.m_Color.argb();
                s_Sprite2DVerts[0].x = (float)s.x + 0.53125f;
                s_Sprite2DVerts[0].y = (float)s.y + 0.53125f;
                s_Sprite2DVerts[0].u = s.u0;
                s_Sprite2DVerts[0].v = s.v0;
                s_Sprite2DVerts[1].x = (float)s.w + (float)s.x + 0.53125f;
                s_Sprite2DVerts[1].y = (float)s.y + 0.53125f;
                s_Sprite2DVerts[1].u = s.u1;
                s_Sprite2DVerts[1].v = s.v0;
                s_Sprite2DVerts[2].x = (float)s.w + (float)s.x + 0.53125f;
                s_Sprite2DVerts[2].y = (float)s.y + (float)s.h + 0.53125f;
                s_Sprite2DVerts[2].u = s.u1;
                s_Sprite2DVerts[2].v = s.v1;
                s_Sprite2DVerts[3].x = (float)s.x + 0.53125f;
                s_Sprite2DVerts[3].y = (float)s.y + (float)s.h + 0.53125f;
                s_Sprite2DVerts[3].u = s.u0;
                s_Sprite2DVerts[3].v = s.v1;
                D3DDevice_DrawVerticesUP(7, 4, s_Sprite2DVerts, sizeof(XBSPRITE2D));
                left -= sizeof(Sprite2DCmd);
                p += sizeof(Sprite2DCmd);
                break;
            }
            case SPR2D_ROTSPRITE: {
                EXRotSprite2D &s = ((RotSprite2DCmd *)p)->m_Sprite;
                s_Sprite2DVerts[0].diffuse = s.m_Color.argb();
                s_Sprite2DVerts[1].diffuse = s.m_Color.argb();
                s_Sprite2DVerts[2].diffuse = s.m_Color.argb();
                s_Sprite2DVerts[3].diffuse = s.m_Color.argb();
                EXPointf tl((float)s.x, (float)s.y);
                EXPointf tr((float)s.w + (float)s.x, (float)s.y);
                EXPointf bl((float)s.x, (float)s.h + (float)s.y);
                EXPointf br((float)s.w + (float)s.x, (float)s.h + (float)s.y);
                EXPointf uvtl(s.u0, s.v0);
                EXPointf uvtr(s.u1, s.v0);
                EXPointf uvbl(s.u0, s.v1);
                EXPointf uvbr(s.u1, s.v1);
                float cu = (uvbr.x - uvtl.x) * 0.5f + uvtl.x;
                float cx = ((float)(s.x + s.w) - (float)s.x) * 0.5f + (float)s.x;
                float cy = ((float)(s.y + s.h) - (float)s.y) * 0.5f + (float)s.y;
                float cv = (uvbr.y - uvtl.y) * 0.5f + uvtl.y;
                if (s.m_Unk1c != 0.0f) {
                    tl = CalcRotation(cx, cy, s.m_Unk1c, tl);
                    tr = CalcRotation(cx, cy, s.m_Unk1c, tr);
                    bl = CalcRotation(cx, cy, s.m_Unk1c, bl);
                    br = CalcRotation(cx, cy, s.m_Unk1c, br);
                }
                if (s.m_Angle != 0.0f) {
                    uvtl = CalcRotation(cu, cv, s.m_Angle, uvtl);
                    uvtr = CalcRotation(cu, cv, s.m_Angle, uvtr);
                    uvbl = CalcRotation(cu, cv, s.m_Angle, uvbl);
                    uvbr = CalcRotation(cu, cv, s.m_Angle, uvbr);
                }
                s_Sprite2DVerts[0].x = tl.x + 0.53125f;
                s_Sprite2DVerts[0].y = tl.y + 0.53125f;
                s_Sprite2DVerts[0].u = uvtl.x;
                s_Sprite2DVerts[0].v = uvtl.y;
                s_Sprite2DVerts[1].x = tr.x + 0.53125f;
                s_Sprite2DVerts[1].y = tr.y + 0.53125f;
                s_Sprite2DVerts[1].u = uvtr.x;
                s_Sprite2DVerts[1].v = uvtr.y;
                s_Sprite2DVerts[2].x = br.x + 0.53125f;
                s_Sprite2DVerts[2].y = br.y + 0.53125f;
                s_Sprite2DVerts[2].u = uvbr.x;
                s_Sprite2DVerts[2].v = uvbr.y;
                s_Sprite2DVerts[3].x = bl.x + 0.53125f;
                s_Sprite2DVerts[3].y = bl.y + 0.53125f;
                s_Sprite2DVerts[3].u = uvbl.x;
                s_Sprite2DVerts[3].v = uvbl.y;
                D3DDevice_DrawVerticesUP(7, 4, s_Sprite2DVerts, sizeof(XBSPRITE2D));
                left -= sizeof(RotSprite2DCmd);
                p += sizeof(RotSprite2DCmd);
                break;
            }
            case SPR2D_FLEXISPRITE: {
                EXFlexiSprite2D &s = ((FlexiSprite2DCmd *)p)->m_Sprite;
                s_Sprite2DVerts[0].diffuse = s.m_Verts[0].m_Color.argb();
                s_Sprite2DVerts[1].diffuse = s.m_Verts[1].m_Color.argb();
                s_Sprite2DVerts[2].diffuse = s.m_Verts[2].m_Color.argb();
                s_Sprite2DVerts[3].diffuse = s.m_Verts[3].m_Color.argb();
                s_Sprite2DVerts[0].x = (float)s.m_Verts[0].x + 0.53125f;
                s_Sprite2DVerts[0].y = (float)s.m_Verts[0].y + 0.53125f;
                s_Sprite2DVerts[0].u = s.m_Verts[0].u;
                s_Sprite2DVerts[0].v = s.m_Verts[0].v;
                s_Sprite2DVerts[1].x = (float)s.m_Verts[1].x + 0.53125f;
                s_Sprite2DVerts[1].y = (float)s.m_Verts[1].y + 0.53125f;
                s_Sprite2DVerts[1].u = s.m_Verts[1].u;
                s_Sprite2DVerts[1].v = s.m_Verts[1].v;
                s_Sprite2DVerts[2].x = (float)s.m_Verts[2].x + 0.53125f;
                s_Sprite2DVerts[2].y = (float)s.m_Verts[2].y + 0.53125f;
                s_Sprite2DVerts[2].u = s.m_Verts[2].u;
                s_Sprite2DVerts[2].v = s.m_Verts[2].v;
                s_Sprite2DVerts[3].x = (float)s.m_Verts[3].x + 0.53125f;
                s_Sprite2DVerts[3].y = (float)s.m_Verts[3].y + 0.53125f;
                s_Sprite2DVerts[3].u = s.m_Verts[3].u;
                s_Sprite2DVerts[3].v = s.m_Verts[3].v;
                D3DDevice_DrawVerticesUP(7, 4, s_Sprite2DVerts, sizeof(XBSPRITE2D));
                left -= sizeof(FlexiSprite2DCmd);
                p += sizeof(FlexiSprite2DCmd);
                break;
            }
            case SPR2D_BLENDMODE: {
                Sprite2DBlendCmd *pCmd = (Sprite2DBlendCmd *)p;
                switch (pCmd->m_Mode) {
                case 1:
                    XBRenderState.SetRenderState(xb::D3DRS_BLENDOP, 0x8006);
                    XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 1);
                    XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 0);
                    break;
                case 2:
                    XBRenderState.SetRenderState(xb::D3DRS_BLENDOP, 0x8006);
                    XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 0x302);
                    XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 1);
                    break;
                case 3:
                    XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 0x302);
                    XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 1);
                    XBRenderState.SetRenderState(xb::D3DRS_BLENDOP, 0x800b);
                    break;
                default:
                    XBRenderState.SetRenderState(xb::D3DRS_BLENDOP, 0x8006);
                    XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 0x302);
                    XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 0x303);
                    break;
                }
                left -= sizeof(Sprite2DBlendCmd);
                p += sizeof(Sprite2DBlendCmd);
                break;
            }
            }
        }
    }
    c[0] = 0.0f;
    c[1] = 0.0f;
    c[2] = 0.0f;
    c[3] = 0.0f;
    D3DDevice_SetVertexShaderConstant1Fast(29 + 96, c);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ALPHAOP, 4);
    ((EXDisplay *)EXBaseDisplay::GetDisplay())->SetViewPort(&m_PrimRect, &m_PrimClipRect);
    XBRenderState.SetRenderState(xb::D3DRS_BLENDOP, 0x8006);
    XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 0x302);
    XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 0x303);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ADDRESSU, 3);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ADDRESSV, 3);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_MAGFILTER, 2);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_MINFILTER, 2);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_MIPFILTER, 2);
    XBRenderState.SetRenderState(xb::D3DRS_ZWRITEENABLE, 1);
}
