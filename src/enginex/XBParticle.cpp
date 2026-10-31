#include "EXParticleSys.h"
#include "EXParticle.h"
#include "EXDisplay.h"
#include "EXGeoFile.h"
#include "EXMemory.h"
#include "XBBaseRenderState.h"
#include "XBBaseVertexShader.h"
#include "EXMatrix.h"
#include "EXTexture.h"
#include "EXGeoParticle.h"
#include "EXGeoEntity.h"
#include "EXGeoEntity_VtxDataC.h"
#include "EXBaseRenderEnv.h"

extern "C" void *__stdcall D3DXMatrixTranspose(void *pOut, const void *pM);

EX_IMPLEMENT_DYNCREATE(EXParticleSys, EXBaseParticleSys)

EXParticleSys::EXParticleSys()
{
}

EXParticleSys::~EXParticleSys()
{
}

int EXParticleSys::SortByTexture(const void *a, const void *b)
{
    u32 ta = ((const EXParticle *)a)->m_texture;
    u32 tb = ((const EXParticle *)b)->m_texture;
    if (tb > ta) return -1;
    return tb < ta ? 1 : 0;
}

// Alpha blended, no depth write, no culling; system flag 8 adds, 0x10 subtracts.
void EXParticleSys::SetParticleRenderStates(xb::D3DDevice *)
{
    XBRenderState.SetRenderState(xb::D3DRS_ZWRITEENABLE, 0);
    XBRenderState.SetRenderState(xb::D3DRS_ALPHAREF, 0);
    XBRenderState.SetRenderState(xb::D3DRS_CULLMODE, 0);
    XBRenderState.SetRenderState(xb::D3DRS_ALPHATESTENABLE, 1);
    XBRenderState.SetRenderState(xb::D3DRS_ALPHABLENDENABLE, 1);
    XBRenderState.SetRenderState(xb::D3DRS_BLENDOP, 0x8006);
    XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 0x302);
    XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 0x303);
    if (m_systemFlags & 8) XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 1);
    if (m_systemFlags & 0x10) {
        XBRenderState.SetRenderState(xb::D3DRS_BLENDOP, 0x800b);
        XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 1);
    }
}

void EXParticleSys::Debug_Draw(u32 pass)
{
}

// EXMatrix::SetFromRotZ, which VC7.1 will not inline into the big draw functions (retail did).
static __forceinline void XBRotZ(EXMatrix &m, float a)
{
    float s = (float)sin(a);
    float c = (float)cos(a);
    m.r[0].set(c, s, 0.0f, 0.0f);
    m.r[1].set(-s, c, 0.0f, 0.0f);
    m.r[2].set(0.0f, 0.0f, 1.0f, 0.0f);
    m.r[3].set(0.0f, 0.0f, 0.0f, 1.0f);
}

// Camera-facing quads. Shader 3 (sprite) gets c0 = world to clip, c25 = billboard rotation (the
// camera's angles times m_158, transposed); per particle c40 = position and c36 = the particle's own
// rotation (transposed). Each quad is a 4-vertex triangle strip of EXGeoEntity_VtxDataC corners
// (+-half size, 0), normal 1, colour, uv. With one texture (m_numTextures == 0) the first particle's
// texture is selected once and every quad goes straight into the push buffer; otherwise the texture
// is switched whenever it changes (particles are sorted by texture) and DrawVerticesUP is used.
void EXParticleSys::DrawQuadParticles(xb::D3DDevice *pDev, EXGeoHeader *pHdr)
{
    EXParticle *pPart = m_pParticles;
    int last = m_maxParticles - 1;
    if (!pPart) return;
    EXGeoParticle *pGeo = m_geoRef.GeoParticle();
    if (!pGeo) return;

    // Wrapped in a struct: a bare array of a class with a constructor calls the vector constructor iterator.
    struct { EXGeoEntity_VtxDataC v[4]; } quad;
    EXGeoEntity_VtxDataC *verts = quad.v;
    verts[0].m_u = 0.0f;
    verts[0].m_v = 0.0f;
    verts[0].m_Colour = 0;
    verts[1].m_u = 0.0f;
    verts[1].m_v = 1.0f;
    verts[1].m_Colour = 0;
    verts[2].m_u = 1.0f;
    verts[2].m_v = 0.0f;
    verts[2].m_Colour = 0;
    verts[3].m_u = 1.0f;
    verts[3].m_v = 1.0f;
    verts[3].m_Colour = 0;

    EXMatrix bill = m_pWorldToView->fastinverse();
    bill.r[3].set(m_7c.x, m_7c.y, m_7c.z, 1.0f);
    EXVector angles;
    bill.GetRotation(angles, 2);
    float f = (float)m_158;
    angles.x *= f;
    angles.y *= f;
    angles.z *= f;
    bill.SetRotationZXY(angles);
    float fOne = 1.0f;
    float fZero = 0.0f;

    xb::D3DXMATRIX m;
    D3DXMatrixMultiply(&m, &EXBaseDisplay::WorldToView(), &EXBaseDisplay::ViewToScreen());
    D3DDevice_SetVertexShaderConstantFast(0, &m, 4);
    D3DXMatrixTranspose(&m, &bill);
    D3DDevice_SetVertexShaderConstantFast(25, &m, 4);

    XBVertexShader.SetVertexShader(3);
    XBRenderState.SetRenderState(xb::D3DRS_ZWRITEENABLE, 0);
    XBRenderState.SetTexture(1, 0);
    XBRenderState.SetTextureStageState(1, xb::D3DTSS_COLOROP, 1);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ALPHAOP, 5);
    XBVertexShader.SetFog(m_258, m_fogColour, m_fogStart, m_fogEnd, m_fogNear, m_fogFar);

    if (!pGeo->m_textures.Size()) {
        EXTexture *pTex = &pHdr->TextureArray()[pPart->m_texture];
        if (pTex) pTex->Select();
        else XBRenderState.SetTexture(0, 0);
        for (int i = last; i >= 0; i--, pPart++) {
            if (!pPart->IsActive()) continue;
            EXVector half = pPart->Scale() * 0.5f;
            float hx = half.x;
            float hy = half.y;
            // A quad is skipped only when both half sizes are zero (the other path skips on either).
            if (hx != 0.0f || hy != 0.0f) {
                if (m_systemFlags & 0x1c06) {
                    EXMatrix rot;
                    if (pPart->m_4c.x == 0.0f && pPart->m_4c.y == 0.0f) {
                        XBRotZ(rot, pPart->m_4c.z);
                        D3DXMatrixTranspose(&m, &rot);
                    } else {
                        rot.SetRotationZXY(pPart->m_4c);
                        D3DXMatrixTranspose(&m, &rot);
                    }
                } else {
                    D3DXMatrixIdentity(&m);
                }
                EXVector pos = pPart->Pos();
                u32 col = pPart->m_colour.argb();
                if (m_bLocalSpace == 1) {
                    EXMaths::vec_xyzw t;
                    D3DXVec4Transform(&t, &pos, &m_1f8);
                    pos = t;
                    pos += m_lastEmitPos;
                }
                xb::DWORD *p;
                xb::D3DDevice::BeginPush(64, &p);
                p[0] = 0x41ea4;             // SET_TRANSFORM_CONSTANT_LOAD
                p[1] = 0x88;                // c40
                p[2] = 0x100b80;            // SET_TRANSFORM_CONSTANT, 4 dwords
                *(EXMaths::vec_xyzw *)&p[3] = pos;
                p += 7;
                p[0] = 0x41ea4;
                p[1] = 0x84;                // c36
                p[2] = 0x400b80;            // 16 dwords
                *(xb::D3DXMATRIX *)&p[3] = m;
                p += 19;
                p[0] = 0x417fc;             // SET_BEGIN_END
                p[1] = 6;                   // triangle strip
                p[2] = 0x40701818;          // INLINE_ARRAY, 28 dwords
                p += 3;
                // Corners as raw dwords; the negative half sizes get the sign bit set.
                p[0] = *(u32 *)&hx | 0x80000000;
                p[1] = *(u32 *)&hy | 0x80000000;
                p[2] = 0;
                p[3] = 1;
                p[4] = col;
                p[5] = *(u32 *)&fZero;
                p[6] = *(u32 *)&fOne;
                p += 7;
                p[0] = *(u32 *)&hx | 0x80000000;
                p[1] = *(u32 *)&hy;
                p[2] = 0;
                p[3] = 1;
                p[4] = col;
                p[5] = *(u32 *)&fZero;
                p[6] = *(u32 *)&fZero;
                p += 7;
                p[0] = *(u32 *)&hx;
                p[1] = *(u32 *)&hy | 0x80000000;
                p[2] = 0;
                p[3] = 1;
                p[4] = col;
                p[5] = *(u32 *)&fOne;
                p[6] = *(u32 *)&fOne;
                p += 7;
                p[0] = *(u32 *)&hx;
                p[1] = *(u32 *)&hy;
                p[2] = 0;
                p[3] = 1;
                p[4] = col;
                p[5] = *(u32 *)&fOne;
                p[6] = *(u32 *)&fZero;
                p += 7;
                p[0] = 0x417fc;
                p[1] = 0;                   // end
                p += 2;
                xb::D3DDevice::EndPush(p);
            }
        }
    } else {
        u32 curTex = (u32)-1;
        for (int i = last; i >= 0; i--, pPart++) {
            if (!pPart->IsActive()) continue;
            EXVector half = pPart->Scale() * 0.5f;
            if (half.x != 0.0f && half.y != 0.0f) {
                EXVector pos = pPart->Pos();
                if (pPart->m_texture != curTex) {
                    curTex = pPart->m_texture;
                    EXTexture *pTex = &pHdr->TextureArray()[curTex];
                    // Retail reads the D3D texture through the null pointer here.
                    if (pTex) pTex->Select();
                    else XBRenderState.SetTexture(0, pTex->m_pD3DTexture);
                }
                if (m_systemFlags & 0x1c06) {
                    EXMatrix rot;
                    if (pPart->m_4c.x == 0.0f && pPart->m_4c.y == 0.0f) {
                        XBRotZ(rot, pPart->m_4c.z);
                        D3DXMatrixTranspose(&m, &rot);
                    } else {
                        rot.SetRotationZXY(pPart->m_4c);
                        D3DXMatrixTranspose(&m, &rot);
                    }
                } else {
                    D3DXMatrixIdentity(&m);
                }
                verts[0].m_x = -half.x;
                verts[0].m_y = -half.y;
                verts[0].m_z = 0.0f;
                verts[0].m_Normal = 1;
                verts[1].m_x = -half.x;
                verts[1].m_y = half.y;
                verts[1].m_z = 0.0f;
                verts[1].m_Normal = 1;
                verts[2].m_x = half.x;
                verts[2].m_y = -half.y;
                verts[2].m_z = 0.0f;
                verts[2].m_Normal = 1;
                verts[3].m_x = half.x;
                verts[3].m_y = half.y;
                verts[3].m_z = 0.0f;
                verts[3].m_Normal = 1;
                verts[0].m_Colour = verts[1].m_Colour = verts[2].m_Colour = verts[3].m_Colour = pPart->m_colour.argb();
                pos = pPart->Pos();
                if (m_bLocalSpace == 1) {
                    EXMaths::vec_xyzw t;
                    D3DXVec4Transform(&t, &pos, &m_1f8);
                    pos = t;
                    pos += m_lastEmitPos;
                }
                D3DDevice_SetVertexShaderConstantFast(40, &pos, 1);
                D3DDevice_SetVertexShaderConstantFast(36, &m, 4);
                D3DDevice_DrawVerticesUP(6, 4, verts, sizeof(EXGeoEntity_VtxDataC));
            }
        }
    }
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ALPHAOP, 4);
    XBRenderState.SetRenderState(xb::D3DRS_ZWRITEENABLE, 1);
    XBRenderState.SetRenderState(xb::D3DRS_FOGENABLE, 0);
}

// By-value EXVector operators VC7.1 will not inline here (retail did).
static __forceinline EXMaths::vec_xyzw XBSubV(const EXMaths::vec_xyzw &a, const EXMaths::vec_xyzw &b)
{
    EXMaths::vec_xyzw r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    r.w = a.w - b.w;
    return r;
}
static __forceinline EXVector XBSub(const EXVector &a, const EXVector &b) { return EXVector(XBSubV(a, b)); }
static __forceinline EXMaths::vec_xyzw XBNeg(const EXMaths::vec_xyzw &a)
{
    EXMaths::vec_xyzw r;
    r.x = 0.0f - a.x;
    r.y = 0.0f - a.y;
    r.z = 0.0f - a.z;
    r.w = 0.0f - a.w;
    return r;
}

// Particles drawn as geo entities (m_texture holds the entity hash of the first particle; every
// particle draws that entity). Per particle the render info colour multiplier is the particle's m_5c
// (rgb / 128, a / 64) and the object matrix is built from scale, rotation and position:
// - local space: scale, then rotation m_1c (EXGeoParticle bit 0x46.0) or m_4c, at the particle
//   position, then the system rotation m_10c at the emitter position m_lastEmitPos;
// - world space: rotation facing the motion (m_pos - m_2c) or -m_4c, then the local matrix's
//   rotation, scaled first, at the particle position.
// GDi data k (one per drawn particle) gets flag 0x80 (alpha op MODULATE2X). The last particle slot
// is never drawn (the loop runs m_maxParticles - 1 times, retail).
void EXParticleSys::DrawEntityParticles(xb::D3DDevice *pDev, EXGeoHeader *pHdr)
{
    EXBaseDisplay *pDisp = EXBaseDisplay::GetDisplay();
    EXParticle *pPart = m_pParticles;
    int n = m_maxParticles - 1;
    EXGeoEntity *pEnt = pHdr->GeoEntity(pPart->m_texture);
    if (!pEnt) return;
    if (n <= 0) return;
    u32 saved[sizeof(EXRenderInfo) / 4];
    memcpy(saved, &EXBaseRenderEnv::RenderInfo(), sizeof(EXRenderInfo));
    EXMatrix localRot = m_1f8;
    localRot.r[3].set(0.0f, 0.0f, 0.0f, 1.0f);
    EXRenderInfo info;
    EXGeoParticle *pGeo = m_geoRef.GeoParticle();
    int k = 0;
    for (; n; n--, pPart++) {
        if (!pPart->IsActive()) continue;
        info.SetRGBAMult(EXVector(pPart->m_5c.x * (1.0f / 128.0f), pPart->m_5c.y * (1.0f / 128.0f),
                                  pPart->m_5c.z * (1.0f / 128.0f), pPart->m_5c.w * (1.0f / 64.0f)));
        info.EnableRGBMult(1);
        info.EnableAlphaMult(1);
        EXBaseRenderEnv::SetRenderInfo(info);
        EXVector scale(pPart->m_scale.x, pPart->m_scale.y, pPart->m_scale.z, 1.0f);
        EXMatrix m;
        if (m_bLocalSpace == 1) {
            EXMatrix rot;
            if (pGeo->m_flags46 & 1)
                rot.SetRotationZXY(pPart->m_1c);
            else
                rot.SetRotationZXY(pPart->m_4c);
            EXMatrix s;
            s.SetScale(scale);
            rot *= s;
            rot.SetTranslation(pPart->m_pos);
            m.SetRotationZXY(m_10c);
            m.SetTranslation(m_lastEmitPos);
            m *= rot;
        } else {
            if (pGeo->m_flags46 & 1) {
                EXVector dir = XBSub(pPart->m_pos, pPart->m_2c);
                m.AlignToDir(dir, EXVector(0.0f, 1.0f, 0.0f, 0.0f), EXVector(0.0f, 0.0f, 1.0f, 0.0f));
            } else {
                m.set_rotate_zxy(XBNeg(pPart->m_4c));
            }
            m = localRot * m;
            EXMatrix s;
            s.SetScale(scale);
            m *= s;
            m.SetTranslation(pPart->m_pos);
        }
        EXBaseDisplay::SetGeoHeader(pHdr);
        EXBaseDisplay::SetObjectToWorld(m);
        pDisp->UpdateTransforms();
        m_pGDiArrays[k].m_pData->m_Flags |= 0x80;
        pEnt->Draw(m_pGDiArrays[k].m_pData, 0);
        k++;
    }
    EXBaseRenderEnv::SetRenderInfo(*(EXRenderInfo *)saved);
}

void EXParticleSys::Draw(u32 pass)
{
    EXDisplay *pDisp = (EXDisplay *)EXBaseDisplay::GetDisplay();
    xb::D3DDevice *pDev = pDisp->Device();
    EXGeoHeader *pHdr = m_geoRef.GeoFile()->GeoHeader();
    if (!pHdr) return;
    if (!m_numActive) return;
    if (!m_64) {
        if (pass == 1) {
            pDisp->UpdateTransforms();
            SetParticleRenderStates(pDev);
            DrawQuadParticles(pDev, pHdr);
        }
    } else if (pass == 0) {
        DrawEntityParticles(pDev, pHdr);
    }
    pDisp->SetDefaultRenderState();
}
