#include "EXGeoEntity.h"
#include "XBGeoEntity.h"
#include "XBGeoDrawInfo.h"
#include "EXMemory.h"
#include "EXMemHeap.h"
#include "XBBaseVertexShader.h"
#include "EXCachedGeoEntity.h"
#include "EXTexSwapInfo.h"
#include "EXTexture.h"
#include "EXGeoHeader.h"
#include "XBBaseRenderState.h"
#include "EXDisplay.h"
#include "EXWnd.h"
#include "EXBaseRenderEnv.h"
#include "XBShadow.h"
#include "XBBaseEfx.h"
#include "XBBaseBenchMark.h"
#include "EXDynLight.h"
#include "EXShadowTemp.h"
#include "EXGeoAnim.h"
#include "EXItemRender_Anim.h"
#include "EXGeoComplexRefObject.h"
#include "EXGeoFile.h"
#include "EXStdFuncs.h"
#include "EXPlane.h"
#include "EXRayCastTemp.h"
#include <math.h>

// EXGeoEntity's Xbox fields (see XBGeoEntity.h).
#define XB ((XBGeoEntityFields *)this)

extern "C" xb::D3DXMATRIX *__stdcall D3DXMatrixTranspose(xb::D3DXMATRIX *pOut, const xb::D3DXMATRIX *pM);
extern "C" xb::D3DXVECTOR3 *__stdcall D3DXVec3TransformNormal(xb::D3DXVECTOR3 *pOut, const xb::D3DXVECTOR3 *pV,
                                                              const xb::D3DXMATRIX *pM);
// Vertex shader register (c-96 based) of each bone's 3x4 matrix (XBVertexShader.obj).
extern unsigned int RemapB2VS[30];
// Subdivision surfaces (XBSubdivisionSystem.obj).
void UpdateOrigin();
extern unsigned int uiSRRLod;
extern signed char bLayerOn[];

signed char g_uiUseSubdivisionSurface;

// Vertex shader manager fields past the shader tables (see XBBaseVertexShader.h).
#define XBMorphStreams (XBVertexShader.m_1df4)      // streams blended by c44 (2 vertex sets, or the skin's deformers)
#define XBWorldScale (XBVertexShader.m_FogScale)    // depth scale of the shaders (fade distances, 1 / it in c30.w)

// Select a texture for stage 0 and set its U/V clamping.
__forceinline void XBSetTexture(EXTexture *pTex)
{
    pTex->Select();
    if (pTex->Flags() & 0x100000)
        XBRenderState.SetTextureStageState(0, xb::D3DTSS_ADDRESSU, 3);
    else
        XBRenderState.SetTextureStageState(0, xb::D3DTSS_ADDRESSU, 1);
    if (pTex->Flags() & 0x200000)
        XBRenderState.SetTextureStageState(0, xb::D3DTSS_ADDRESSV, 3);
    else
        XBRenderState.SetTextureStageState(0, xb::D3DTSS_ADDRESSV, 1);
}

// Shadow casters: grey of the shadow density, opaque.
__forceinline void SetShadowGlobalDiffuseAmbient(float *p)
{
    float f = 1.0f - EXBaseRenderEnv::ShadowDensity();
    p[0] = f;
    p[1] = f;
    p[2] = f;
    p[3] = 1.0f;
}

// Lit strips use the lighting bits; transparent or unlit strips drop them.
__forceinline void SetUpStripLighting(unsigned int &bits, EXGeoEntity_TriStrip *pStrip)
{
    if (!pStrip->m_TransType && !(pStrip->m_Flags & 0x200))
        XBVertexShader.SetBit(bits);
    else
        XBVertexShader.ClearBit(bits);
}

__forceinline void SetUpEnvironmentMapping(EXTexture *pTex, xb::_D3DMATRIX *pObjectToWorld, xb::_D3DMATRIX *pWorldToView)
{
    if (!(pTex->Flags() & 0x10000)) {
        XBVertexShader.ClearBit(0x100);
    } else {
        XBVertexShader.SetBit(0x100);
        XBVertexShader.SetEnvironmentMatrix(pObjectToWorld, pWorldToView, pTex->Flags());
    }
}

void *XBHeap1Alloc(int size, int flags)
{
    EXMemHeap *h = _SystemHeapList[1];
    return h->_EXAlloc(size, flags);
}

unsigned int EXGeoEntity::GetVertexCount() const { return VertexCount(); }

void *EXGeoEntity::GetPhysicalVertexBuffer()
{
    xb::D3DVertexBuffer *p = XB->m_pVB;
    return D3DVertexBuffer_Lock2(p, 0);
}

void EXGeoEntity::RegisterPhysVertexBuffer(void *pHdr, void *pData)
{
    xb::D3DVertexBuffer *p = (xb::D3DVertexBuffer *)pHdr;
    p->m_Common = 1;
    p->m_Data = (u32)pData & 0x7fffffff;
    p->m_Lock = 0;
}

static __forceinline void XBClearStreams()
{
    D3DDevice_SetStreamSource(0, 0, 0);
    D3DDevice_SetStreamSource(1, 0, 0);
    D3DDevice_SetStreamSource(2, 0, 0);
    D3DDevice_SetStreamSource(3, 0, 0);
    D3DDevice_SetStreamSource(4, 0, 0);
    D3DDevice_SetStreamSource(5, 0, 0);
    D3DDevice_SetStreamSource(6, 0, 0);
    D3DDevice_SetStreamSource(7, 0, 0);
    D3DDevice_SetStreamSource(8, 0, 0);
    D3DDevice_SetStreamSource(9, 0, 0);
}

void EXGeoEntity::ReleasePhysicalVertexBuffer(unsigned int *p)
{
    XBClearStreams();
    D3DResource_BlockUntilNotBusy((xb::D3DResource *)p);
}

void EXGeoEntity::RegisterPhysIndexBuffer(unsigned int pHdr, void *pData)
{
    xb::D3DIndexBuffer *p = (xb::D3DIndexBuffer *)pHdr;
    p->m_Common = 0x10001;
    p->m_Data = (u32)pData;
    p->m_Lock = 0;
}

void EXGeoEntity::ReleasePhysicalIndexBuffer()
{
    D3DDevice_SetIndices(0, 0);
    D3DResource_BlockUntilNotBusy(&XB->m_IB);
    XB->m_pIB = 0;
}

void EXGeoEntity::XBSetEntityDynamicLighting() {}

void EXGeoEntity::DrawDirectXShadow(EXCachedGeoEntity *, EXGeoHeader *, EXGDiData *) {}

void EXGeoEntity::ReleaseVertexBuffer(EXGDiData *pData) { pData->m_Flags &= ~4; }

unsigned int EXGeoEntity::FaceCount() const
{
    unsigned int n = 0;
    for (unsigned int i = 0; i < TriStripCount(); i++) n += TriStrip()[i].m_FaceCount;
    return n;
}

void *EXGeoEntity::CreatePhysicalVertexBuffer(void *pHdr, void *pData, unsigned int *pOut)
{
    RegisterPhysVertexBuffer(pHdr, pData);
    *pOut = (unsigned int)pHdr;
    return pHdr;
}

void *EXGeoEntity::CreatePhysicalVertexBuffer(int)
{
    return CreatePhysicalVertexBuffer(&XB->m_VB, XB->m_Vtx, (unsigned int *)&XB->m_pVB);
}

xb::D3DVertexBuffer *EXGeoEntity::GetVertexBuffer()
{
    CreatePhysicalVertexBuffer(&XB->m_VB, XB->m_Vtx, (unsigned int *)&XB->m_pVB);
    return XB->m_pVB;
}

void EXGeoEntity::ReleasePhysicalVertexBuffer()
{
    XBClearStreams();
    D3DResource_BlockUntilNotBusy(GetVertexBuffer());
    XB->m_pVB = 0;
}

void EXGeoEntity::CreatePhysicalIndexBuffer()
{
    RegisterPhysIndexBuffer((unsigned int)&XB->m_IB, XB->m_Index);
    XB->m_pIB = &XB->m_IB;
}

void *EXGeoEntity::XtraData(unsigned char t) const
{
    if ((1 << t) & XB->m_XtraTypes) {
        int i = 0;
        while ((XB->m_Xtra[i].m_Value & 0xff) != t) i++;
        return ex::EXRelOffset(XB->m_Xtra[i]);
    }
    return 0;
}

void EXGeoEntity::CreateRGBAVertexBuffers()
{
    u32 *pSets = (u32 *)XtraData(5);
    int n = 1;
    if (pSets) n = *pSets;
    XB->m_pRGBAVB = (XBVertexBufferRef *)XBHeap1Alloc((n + 1) * sizeof(XBVertexBufferRef), 0);
    memset(XB->m_pRGBAVB, 0, (n + 1) * sizeof(XBVertexBufferRef));
    u32 *pRgba = XB->m_Rgba;
    for (int i = 0; i < n; i++) {
        if (i > 0) pRgba = ((ex::EXRelPtr<u32> *)(pSets + 1))[i];
        CreatePhysicalVertexBuffer(&XB->m_pRGBAVB[i].m_VB, pRgba, (unsigned int *)&XB->m_pRGBAVB[i].m_pVB);
    }
}

void EXGeoEntity::CreateDeformerVertexBuffers()
{
    if (!XB->m_DeformerCount) return;
    XB->m_pDefVB = (XBVertexBufferRef *)XBHeap1Alloc(XB->m_DeformerCount * sizeof(XBVertexBufferRef), 0);
    memset(XB->m_pDefVB, 0, XB->m_DeformerCount * sizeof(XBVertexBufferRef));
    ex::EXRELOFFSET32 *pOff = (ex::EXRELOFFSET32 *)XtraData(2);
    for (unsigned int i = 0; i < XB->m_DeformerCount; i++, pOff++)
        CreatePhysicalVertexBuffer(&XB->m_pDefVB[i].m_VB, ex::EXRelOffset(*pOff), (unsigned int *)&XB->m_pDefVB[i].m_pVB);
}

void EXGeoEntity::CreateDeformerVertexSetBuffers()
{
    u32 *pSets = (u32 *)XtraData(4);
    if (!pSets || !*pSets) return;
    XB->m_pDefVB = (XBVertexBufferRef *)XBHeap1Alloc(*pSets * sizeof(XBVertexBufferRef), 0);
    memset(XB->m_pDefVB, 0, *pSets * sizeof(XBVertexBufferRef));
    ex::EXRELOFFSET32 *pOff = (ex::EXRELOFFSET32 *)(pSets + 1);
    for (unsigned int i = 0; i < *pSets; i++, pOff++)
        CreatePhysicalVertexBuffer(&XB->m_pDefVB[i].m_VB, ex::EXRelOffset(*pOff), (unsigned int *)&XB->m_pDefVB[i].m_pVB);
}

// NEAR: register allocation of the relative-offset load differs (same pattern in GetXYZVertexBuffer,
// CreateDeformerVertexBuffers, CreateDeformerVertexSetBuffers, ReleaseDeformerVertexBuffers).
xb::D3DVertexBuffer *EXGeoEntity::GetDefVertexBuffer(unsigned int i)
{
    ex::EXRELOFFSET32 *pOff = (ex::EXRELOFFSET32 *)XtraData(2);
    pOff += i;
    CreatePhysicalVertexBuffer(&XB->m_pDefVB[i].m_VB, ex::EXRelOffset(*pOff), (unsigned int *)&XB->m_pDefVB[i].m_pVB);
    return XB->m_pDefVB[i].m_pVB;
}

xb::D3DVertexBuffer *EXGeoEntity::GetXYZVertexBuffer(unsigned int i)
{
    ex::EXRELOFFSET32 *pOff = (ex::EXRELOFFSET32 *)((u32 *)XtraData(4) + 1);
    CreatePhysicalVertexBuffer(&XB->m_pDefVB[i].m_VB, ex::EXRelOffset(pOff[i]), (unsigned int *)&XB->m_pDefVB[i].m_pVB);
    return XB->m_pDefVB[i].m_pVB;
}

xb::D3DVertexBuffer *EXGeoEntity::GetRGBAVertexBuffer(unsigned int i)
{
    u32 *pSets = (u32 *)XtraData(5);
    u32 *pRgba;
    if (i && pSets)
        pRgba = ((ex::EXRelPtr<u32> *)(pSets + 1))[i];
    else
        pRgba = XB->m_Rgba;
    CreatePhysicalVertexBuffer(&XB->m_pRGBAVB[i].m_VB, pRgba, (unsigned int *)&XB->m_pRGBAVB[i].m_pVB);
    return XB->m_pRGBAVB[i].m_pVB;
}

xb::D3DIndexBuffer *EXGeoEntity::GetIndexBuffer()
{
    D3DDevice_SetIndices(0, 0);
    CreatePhysicalIndexBuffer();
    return XB->m_pIB;
}

void EXGeoEntity::ReleaseRGBAVertexBuffers()
{
    if (!XB->m_pRGBAVB) return;
    u32 *pSets = (u32 *)XtraData(5);
    int n = 1;
    if (pSets) n = *pSets;
    for (int i = 0; i < n; i++) ReleasePhysicalVertexBuffer((unsigned int *)GetRGBAVertexBuffer(i));
    EXFree(XB->m_pRGBAVB);
}

void EXGeoEntity::ReleaseDeformerVertexBuffers()
{
    if (!XB->m_pDefVB) return;
    for (unsigned int i = 0; i < XB->m_DeformerCount; i++) ReleasePhysicalVertexBuffer((unsigned int *)GetDefVertexBuffer(i));
    EXFree(XB->m_pDefVB);
}

void EXGeoEntity::OnCreate(EXGeoHeader *pHdr, EXGeoObjectHeader *pObj)
{
    EXGeoBaseEntity::OnCreate(pHdr, pObj);
    CreatePhysicalVertexBuffer(0);
    CreatePhysicalIndexBuffer();
    CreateDeformerVertexBuffers();
    CreateDeformerVertexSetBuffers();
    CreateRGBAVertexBuffers();
    XB->m_pSubDiv = 0;
    XB->m_8a = 0xff;
}

void EXGeoEntity::OnDelete(EXGeoHeader *pHdr, EXGeoObjectHeader *pObj)
{
    if (XB->m_70) {
        EXFree(XB->m_70);
        XB->m_70 = 0;
    }
    EXGeoBaseEntity::OnDelete(pHdr, pObj);
    ReleasePhysicalVertexBuffer();
    ReleasePhysicalIndexBuffer();
    ReleaseDeformerVertexBuffers();
    ReleaseRGBAVertexBuffers();
    if (XB->m_pSubDiv) {
        delete XB->m_pSubDiv;  // TODO: ExSubDivisionMesh dtor is a [fi] in this object
        XB->m_pSubDiv = 0;
    }
}

void EXGeoEntity::SetShadowVertexShader(EXGDiData *pData)
{
    EXGDiSkinData *pSkin = ms_pGDiSkinData;
    if (pSkin && *(XBSkinBlock **)pSkin) {
        if ((*(XBSkinBlock **)pSkin)->m_MorphIndex >= 0) {
            XBVertexShader.SetVertexShader(9);
        } else if (!(pData->m_Flags & 1) && !(m_Flags & 0x2000)) {
            XBVertexShader.SetVertexShader(8);
        } else {
            XBVertexShader.SetVertexShader(10);
        }
    } else {
        XBVertexShader.SetVertexShader(10);
    }
}

void EXGeoEntity::SetRefractionVertexShader(EXGDiData *pData)
{
    EXGDiSkinData *pSkin = ms_pGDiSkinData;
    if (pSkin && *(XBSkinBlock **)pSkin) {
        if ((*(XBSkinBlock **)pSkin)->m_MorphIndex >= 0) {
            XBVertexShader.SetVertexShader(15);
        } else if (!(pData->m_Flags & 1) && !(m_Flags & 0x2000)) {
            XBVertexShader.SetVertexShader(13);
        } else {
            XBVertexShader.SetVertexShader(14);
        }
    } else {
        XBVertexShader.SetVertexShader(14);
    }
}

void EXGeoEntity::SetHeatVertexShader(EXGDiData *pData)
{
    EXGDiSkinData *pSkin = ms_pGDiSkinData;
    if (pSkin && *(XBSkinBlock **)pSkin) {
        if ((*(XBSkinBlock **)pSkin)->m_MorphIndex >= 0) {
            XBVertexShader.SetVertexShader(18);
        } else if (!(pData->m_Flags & 1) && !(m_Flags & 0x2000)) {
            XBVertexShader.SetVertexShader(16);
        } else {
            XBVertexShader.SetVertexShader(17);
        }
    } else {
        XBVertexShader.SetVertexShader(17);
    }
}

// NEAR: 25 bytes differ, scheduling of the buffer-header stores and the Lock2 argument register.
void *EXGeoEntity::GetVertexBuffer(EXGDiData *pData, signed char)
{
    if (!(pData->m_Flags & 0x100)) {
        pData->m_54 = XBHeap1Alloc(VertexCount() * sizeof(EXGeoEntity_VtxData), 1);
        pData->m_58 = XBHeap1Alloc(VertexCount() * sizeof(EXGeoEntity_VtxData), 1);
        pData->m_5c = XBHeap1Alloc(VertexCount() * sizeof(EXGeoEntity_VtxData), 1);
        memcpy(pData->m_54, XB->m_Vtx, VertexCount() * sizeof(EXGeoEntity_VtxData));
        memcpy(pData->m_58, XB->m_Vtx, VertexCount() * sizeof(EXGeoEntity_VtxData));
        memcpy(pData->m_5c, XB->m_Vtx, VertexCount() * sizeof(EXGeoEntity_VtxData));
        CreatePhysicalVertexBuffer(&pData->m_2c[0x30 - 0x2c], pData->m_54, (unsigned int *)&pData->m_1c);
        CreatePhysicalVertexBuffer(&pData->m_2c[0x3c - 0x2c], pData->m_58, (unsigned int *)&pData->m_20);
        CreatePhysicalVertexBuffer(&pData->m_2c[0x48 - 0x2c], pData->m_5c, (unsigned int *)&pData->m_24);
        pData->m_CurrentIndex = 0;
        pData->m_Flags |= 0x100;
    }
    pData->m_Flags |= 1;
    void *p;
    if (pData->m_CurrentIndex == 0) {
        p = D3DVertexBuffer_Lock2((xb::D3DVertexBuffer *)pData->m_1c, 0);
        pData->m_CurrentVB = (u32)pData->m_1c;
        *(unsigned int **)pData->m_2c = pData->m_20;
        return p;
    }
    if (pData->m_CurrentIndex == 1) {
        p = D3DVertexBuffer_Lock2((xb::D3DVertexBuffer *)pData->m_20, 0);
        pData->m_CurrentVB = (u32)pData->m_20;
        *(unsigned int **)pData->m_2c = pData->m_24;
        return p;
    }
    if (pData->m_CurrentIndex == 2) {
        p = D3DVertexBuffer_Lock2((xb::D3DVertexBuffer *)pData->m_24, 0);
        pData->m_CurrentVB = (u32)pData->m_24;
        *(unsigned int **)pData->m_2c = pData->m_1c;
        return p;
    }
    return 0;
}

void EXGeoEntity::BuildTextureRemap(EXCachedGeoEntity *pCached)
{
    EXGeoHeader *pHdr = pCached->m_pHeader;
    EXGDiData *pData = pCached->m_pGDiData;
    EXTexSwapInfo *pSwap = pCached->m_RenderInfo.TexSwapInfo();
    u32 *pRemap = (u32 *)pData->m_pTexRemap;
    if (!pSwap) {
        if (pRemap) return;
    } else if (pRemap && pSwap->ChangeNum() == pRemap[0]) {
        return;
    }
    EXTextureArray *pTextures = pHdr->m_pTextureArray;
    EXGeoEntity_TextureList *pList = TextureList();
    if (!pRemap) pData->m_pTexRemap = _EXAlloc(pList->m_Count * 4 + 4, 0);
    if (pSwap) {
        if (pSwap->SwapCount()) {
            ((u32 *)pData->m_pTexRemap)[0] = pSwap->ChangeNum();
            for (unsigned int i = 0; i < pList->m_Count; i++)
                ((u32 *)pData->m_pTexRemap)[i + 1] = (u32)pSwap->SwappedTexture(&(*pTextures)[pList->m_Textures[i]]);
            return;
        }
        ((u32 *)pData->m_pTexRemap)[0] = pSwap->ChangeNum();
    }
    for (unsigned int i = 0; i < pList->m_Count; i++)
        ((u32 *)pData->m_pTexRemap)[i + 1] = (u32)&(*pTextures)[pList->m_Textures[i]];
}

// Vertex shader lighting: the item's three directional lights (when it has extended render info),
// then the dynamic lights gathered by Draw, split into point lights and directional point lights.
// The bits the strips may use go to bits (SetUpStripLighting turns them off for unlit strips).
__forceinline void EXGeoEntity::SetUpLighting(EXCachedGeoEntity *pCached, unsigned int &bits)
{
    if ((m_Flags & 2) && pCached->m_RenderInfo.ExtRenderInfo()) {
        XBVertexShader.SetBit(4);
        EXVector dir[3];
        EXVector col[3];
        dir[0] = pCached->m_RenderInfo.LightVector(0);
        dir[1] = pCached->m_RenderInfo.LightVector(1);
        dir[2] = pCached->m_RenderInfo.LightVector(2);
        col[0] = pCached->m_RenderInfo.LightColor(0);
        col[1] = pCached->m_RenderInfo.LightColor(1);
        col[2] = pCached->m_RenderInfo.LightColor(2);
        XBVertexShader.SetDirLights(dir, col, (xb::D3DXMATRIX *)&pCached->m_ObjectToWorld);
    }
    EXMaths::vec_xyzw posCol[4];
    EXMaths::vec_xyzw pos[4];
    float range[4];
    EXMaths::vec_xyzw dirCol[4];
    EXMaths::vec_xyzw dirPos[4];
    EXMaths::vec_xyzw dirDir[4];
    unsigned int nPos = 0;
    unsigned int nDir = 0;
    for (unsigned int i = 0; i < pCached->m_DynLightCount; i++) {
        const EXDynLight *pLight = (const EXDynLight *)pCached->m_DynLights[i];
        if (pLight->m_outerRadius != 0.0f) {
            if (!pLight->m_directional) {
                pos[nPos].set(pLight->m_pos);
                posCol[nPos].set(pLight->m_colorF);
                if (pCached->m_DynLightRGBMultEnabled) posCol[nPos] *= pCached->m_DynLightRGBMult;
                range[nPos] = pLight->m_outerRadius;
                nPos++;
            } else {
                dirPos[nDir].set(pLight->m_pos);
                dirCol[nDir].set(pLight->m_colorF);
                if (pCached->m_DynLightRGBMultEnabled) dirCol[nDir] *= pCached->m_DynLightRGBMult;
                dirDir[nDir].set(pLight->m_dir);
                nDir++;
            }
        }
    }
    if (nPos) {
        XBVertexShader.SetBit(8);
        bits |= 8;
        if (nPos >= 3) {
            XBVertexShader.SetBit(0x10);
            bits |= 0x10;
        }
        XBVertexShader.SetPosLights(nPos, (EXVector *)pos, (EXVector *)posCol, range, (xb::D3DXMATRIX *)&pCached->m_ObjectToWorld);
    }
    if (nDir) {
        XBVertexShader.SetBit(0x20);
        bits |= 0x20;
        if (nDir >= 3) {
            XBVertexShader.SetBit(0x40);
            bits |= 0x40;
        }
        XBVertexShader.SetPosDirLights(nDir, (EXVector *)dirPos, (EXVector *)dirCol, (xb::D3DXMATRIX *)&pCached->m_ObjectToWorld,
                                       (EXVector *)dirDir);
    }
}

// Shadow map pass: no textures, plain shader. Receivers: stage 1 samples the shadow map through
// the shadow UV matrix (vertex shader bit 0x80); others: stages 1 and 2 off.
__forceinline void EXGeoEntity::SetUpShadows(EXCachedGeoEntity *pCached)
{
    if (pCached->m_ShadowCaster) {
        XBRenderState.SetTexture(2, 0);
        XBRenderState.SetTexture(1, 0);
        XBRenderState.SetTexture(0, 0);
        XBVertexShader.ClearBitField();
        XBRenderState.SetTextureStageState(1, xb::D3DTSS_COLOROP, 1);
    } else if (m_Flags & 0x64000) {
        XBRenderState.SetTextureStageState(1, xb::D3DTSS_COLOROP, 4);
        XBRenderState.SetTexture(1, XBShadowController.ShadowTexture());
        XBShadowController.XBGenerateShadowUVMatrix((xb::_D3DMATRIX *)&pCached->m_ObjectToWorld);
        XBVertexShader.SetBit(pCached->m_ShadowReceiver << 7);
        XBRenderState.SetTextureStageState(1, xb::D3DTSS_COLOROP, pCached->m_ShadowReceiver * 3 + 1);
    } else {
        XBRenderState.SetTexture(1, 0);
        XBRenderState.SetTexture(2, 0);
    }
}

// Vertex shader c11 (diffuse) and c12 (ambient) from the render environment: RGBA mult and the
// extended info's ambient colour when it is not black.
__forceinline void SetGlobalDiffuseAmbient(float *pDiffuse, float *pAmbient)
{
    if (EXBaseRenderEnv::AlphaMultEnabled() && !EXBaseRenderEnv::DistanceFade()) pDiffuse[3] = EXBaseRenderEnv::RGBAMult().w;
    if (EXBaseRenderEnv::RGBMultEnabled()) {
        pDiffuse[0] = EXBaseRenderEnv::RGBAMult().x;
        pDiffuse[1] = EXBaseRenderEnv::RGBAMult().y;
        pDiffuse[2] = EXBaseRenderEnv::RGBAMult().z;
    }
    if (EXBaseRenderEnv::RenderInfo().ExtRenderInfo()) {
        const EXVector &a = EXBaseRenderEnv::AmbientColor();
        if (a.x + a.y + a.z + a.w > 1e-5f) {
            for (int i = 0; i < 4; i++) pAmbient[i] = a[i];
        }
    }
}

// Refraction and heat shaders: the camera direction in object space (c40) and the refraction
// parameters (c30: 0.5, strength, 2, 1 / world scale).
__forceinline void CalculateCameraToObject(EXCachedGeoEntity *pCached)
{
    EXVector v;
    EXVector c;
    xb::D3DXMATRIX m;
    D3DXMatrixTranspose(&m, (xb::D3DXMATRIX *)&pCached->m_ObjectToWorld);
    D3DXVec3TransformNormal((xb::D3DXVECTOR3 *)&v, (xb::D3DXVECTOR3 *)((EXDisplay *)EXBaseDisplay::GetDisplay())->GetCameraPositionNormalised(), &m);
    float inv = 1.0f / (float)sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    v.x *= inv;
    v.y *= inv;
    v.z *= inv;
    v.w = 0.0f;
    D3DDevice_SetVertexShaderConstantFast(40, &v, 1);
    c.x = 0.5f;
    c.y = 0.04f;
    c.z = 2.0f;
    c.w = 1.0f / XBWorldScale;
    if (pCached->m_RenderInfo.RefractionRender()) {
        c.y = pCached->m_RenderInfo.RefractionStrength();
        D3DDevice_SetVertexShaderConstantFast(30, &c, 1);
    }
}

// Blend states of a strip: 0 normal (blend only when something needs alpha, otherwise alpha test
// at 0x96), 1 additive, 2 subtractive (both unfogged).
__forceinline void SetUpTransType(EXCachedGeoEntity *pCached, EXGeoEntity_TriStrip *pStrip)
{
    XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 0x302);
    if (!(pStrip->m_TransType & 3)) {
        XBRenderState.SetRenderState(xb::D3DRS_BLENDOP, 0x8006);
        XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 0x303);
        XBRenderState.SetRenderState(xb::D3DRS_FOGCOLOR, pCached->m_Fog.m_color.argb());
        int stripAlpha = pStrip->m_Flags & 8;  // strip flag 8: the texture has alpha
        int alpha = (pCached->m_RenderInfo.AlphaMultEnabled() || pCached->m_DistanceFade || (pCached->m_pEntity->m_Flags & 0xc0000000)) ? 1 : 0;
        if (!stripAlpha && !alpha) {
            XBRenderState.SetRenderState(xb::D3DRS_ALPHABLENDENABLE, 0);
            XBRenderState.SetRenderState(xb::D3DRS_ALPHAREF, 0x96);
        } else {
            XBRenderState.SetRenderState(xb::D3DRS_ALPHABLENDENABLE, 1);
            XBRenderState.SetRenderState(xb::D3DRS_ALPHAREF, 0);
        }
    } else if (pStrip->m_TransType == 1) {
        XBRenderState.SetRenderState(xb::D3DRS_ALPHABLENDENABLE, 1);
        XBRenderState.SetRenderState(xb::D3DRS_BLENDOP, 0x8006);
        XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 1);
        XBRenderState.SetRenderState(xb::D3DRS_FOGCOLOR, 0);
        XBRenderState.SetRenderState(xb::D3DRS_ALPHAREF, 0);
    } else if (pStrip->m_TransType == 2) {
        XBRenderState.SetRenderState(xb::D3DRS_ALPHABLENDENABLE, 1);
        XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 1);
        XBRenderState.SetRenderState(xb::D3DRS_BLENDOP, 0x800b);
        XBRenderState.SetRenderState(xb::D3DRS_FOGCOLOR, 0);
        XBRenderState.SetRenderState(xb::D3DRS_ALPHAREF, 0);
    }
}

// Refraction, heat and shadow passes blend with source alpha (shadow casters with flag 0x10000 do
// not) and pick their fixed vertex shader.
__forceinline void EXGeoEntity::SetUpSpecialFX(EXCachedGeoEntity *pCached, signed char &blend)
{
    if (pCached->m_RenderInfo.RefractionRender() || pCached->m_RenderInfo.HeatRender() || pCached->m_ShadowCaster) {
        blend = 1;
        if (pCached->m_ShadowCaster && (m_Flags & 0x10000)) blend = 0;
        XBRenderState.SetRenderState(xb::D3DRS_BLENDOP, 0x8006);
        XBRenderState.SetRenderState(xb::D3DRS_SRCBLEND, 0x302);
        XBRenderState.SetRenderState(xb::D3DRS_DESTBLEND, 0x303);
    }
    if (pCached->m_RenderInfo.RefractionRender()) {
        SetRefractionVertexShader(pCached->m_pGDiData);
        XBRenderState.SetTexture(0, pCached->m_RenderInfo.ExtRenderInfo() && pCached->m_RenderInfo.RefractionBlur()
                                        ? XBEfx.m_pRefractionBlur : XBEfx.m_pBlitSrc);
        XBRenderState.SetRenderState(xb::D3DRS_COLORWRITEENABLE, 0x10101);
    }
    if (pCached->m_RenderInfo.HeatRender()) SetHeatVertexShader(pCached->m_pGDiData);
    if (pCached->m_ShadowCaster) SetShadowVertexShader(pCached->m_pGDiData);
}

// Vertex shader skinning (CPU-skinned or deformed vertices skip it): stream 2 = bone weights,
// with a morph target set the deformer buffers on streams 3.. (weights in c44), and each bone's
// matrix, moved so that it turns about the bone's rest position, as 3 rows at RemapB2VS[bone].
void EXGeoEntity::CalculateVSSkin(EXGeoHeader *, EXGDiData *pData)
{
    if (pData->m_Flags & 1) return;
    if (m_Flags & 0x2000) return;
    XBGDiSkinFields *pSkin = (XBGDiSkinFields *)ms_pGDiSkinData;
    if (!pSkin || !pSkin->m_pEntitySkins) return;
    XBVertexShader.SetBit(2);
    pData->m_Flags |= 8;
    XBEntitySkin *pEntSkin = *(ex::EXRelPtr<XBEntitySkin> *)&((XBGDiSkinFields *)ms_pGDiSkinData)->m_pEntitySkins[GDiIndex()];
    EXGeoAnimSkinHeader *pSkelHdr = ((XBGDiSkinFields *)ms_pGDiSkinData)->m_pRender->m_skelRef.GeoAnimSkinHeader();
    if (!pSkelHdr) return;
    EXGeoAnimSkin *pSkel = pSkelHdr->m_Addr[0].m_pObject;
    if (!pSkel) return;
    EXGeoAnimSkinHeader *pDrawHdr = ((XBGDiSkinFields *)ms_pGDiSkinData)->m_pRender->m_pSkins[((XBGDiSkinFields *)ms_pGDiSkinData)->m_Skin]->m_drawSkinRef.GeoAnimSkinHeader();
    if (!pDrawHdr) return;
    EXGeoAnimSkin *pDrawSkin = pDrawHdr->m_Addr[0].m_pObject;
    if (!pDrawSkin) return;
    EXVector *pRest = *(ex::EXRelPtr<EXVector> *)&pSkel->m_30[0x10];
    xb::D3DVertexBuffer *pWeights = pDrawSkin->GetWeightVertexBuffer(((XBGDiSkinFields *)ms_pGDiSkinData)->m_WeightLod);
    if (g_uiUseSubdivisionSurface && XB->m_pSubDiv->GetLoD(uiSRRLod)->GetWeightsStream())
        pWeights = XB->m_pSubDiv->GetLoD(uiSRRLod)->GetWeightsStream();
    if (!pWeights) return;
    XBGDiSkinFields *pSkinData = (XBGDiSkinFields *)ms_pGDiSkinData;
    if (pSkinData->m_pSkinList->m_MorphIndex >= 0) {
        EXGeoAnimSkin_MorphList *pMorph = &pSkel->MorphList()[pSkinData->m_pSkinList->m_MorphIndex];
        unsigned int offset = pMorph->DeformerOffset();
        float weights[26];
        memset(weights, 0, sizeof(weights));
        for (int s = 0; s < 8; s++) XBVertexShader.TurnOffStream(2, s + 6, s + 6);
        int n = 0;
        unsigned int stream = 3;
        for (unsigned int i = 0; i < pMorph->m_DeformerCount; i++) {
            if (GetDefVertexBuffer(i)) {
                float w = ((float *)((XBGDiSkinFields *)ms_pGDiSkinData)->m_pRender->MorphDeformers())[offset + i];
                if (w > 0.0001f && stream < 11) {
                    weights[n] = w;
                    XBVertexShader.TurnOnStream(0x42, 2, stream + 3, stream, 0, 0, 0);
                    XBVertexShader.SetVertexStream(stream, GetDefVertexBuffer(i), 0x10, 0);
                    n++;
                    stream++;
                }
            }
        }
        XBMorphStreams = n;
        D3DDevice_SetVertexShaderConstantFast(44, weights, 2);
        XBVertexShader.SetBit(1);
    }
    XBVertexShader.SetVertexStream(2, pWeights, 0x14, 0);
    u8 *pBones = pEntSkin->m_Bones;
    EXMatrix *pMtx = ((XBGDiSkinFields *)ms_pGDiSkinData)->m_pRender->MtxArray();
    u8 *pRemap = ((XBGDiSkinFields *)ms_pGDiSkinData)->m_pRender->RemapTable();
    for (int b = 0; b < pEntSkin->m_BoneCount; b++) {
        unsigned int bone = pBones[b];
        if (pRemap) bone = pRemap[bone];
        EXMaths::mat_44 m = *(EXMaths::mat_44 *)&pMtx[bone];
        xb::D3DXVECTOR4 p;
        D3DXVec4Transform(&p, (xb::D3DXVECTOR4 *)&pRest[bone], (xb::D3DXMATRIX *)&pMtx[bone]);
        m.r[3].x -= p.x;
        m.r[3].y -= p.y;
        m.r[3].z -= p.z;
        D3DXMatrixTranspose((xb::D3DXMATRIX *)&m, (xb::D3DXMATRIX *)&m);
        D3DDevice_SetVertexShaderConstantFast((int)RemapB2VS[b] - 96, &m, 3);
    }
}

// Vertex streams: 0 positions/normals (the dynamic buffer when the CPU rewrote them), 2 and 3 the two
// vertex sets being morphed (weights in c44), 1 the vertex colours (two light sets blended by c68,
// the second set on stream nextStream), then skinning streams (CalculateVSSkin). Returns whether the
// second colour stream is on.
__forceinline signed char EXGeoEntity::SetUpVertexStreams(EXCachedGeoEntity *pCached, unsigned int &nextStream, unsigned int &rgbaInput)
{
    EXGeoHeader *pHdr = pCached->m_pHeader;
    EXGDiData *pData = pCached->m_pGDiData;
    signed char blendRGBA = 0;
    if ((pData->m_Flags & 1) || (m_Flags & 0x2000)) {
        if (pData->m_Flags & 0x100) {
            XBVertexShader.SetVertexStream(0, (void *)pData->m_CurrentVB, 0x18, 0);
            if (!pCached->m_ShadowCaster) pData->m_CurrentIndex++;
            if (pData->m_CurrentIndex == 3) pData->m_CurrentIndex = 0;
        } else {
            XBVertexShader.SetVertexStream(0, (void *)pData->m_VertexBuffer, 0x18, 0);
        }
        if (XB->m_pSubDiv) XBVertexShader.SetVertexStream(0, (void *)pData->m_VertexBuffer, 0x18, 0);
    } else {
        XBVertexShader.SetVertexStream(0, (void *)pData->m_VertexBuffer, 0x18, 0);
    }
    xb::D3DDevice::SetIndices((void *)pData->m_IndexBuffer, 0);
    EXVector one;
    one.x = 1.0f;
    one.y = 1.0f;
    one.z = 1.0f;
    one.w = 1.0f;
    D3DDevice_SetVertexShaderConstantFast(74, &one, 1);
    float weights[8];
    for (int i = 0; i < 8; i++) {
        weights[i] = 0.0f;
        XBVertexShader.TurnOffStream(0, i + 6, i + 6);
    }
    D3DDevice_SetVertexShaderConstantFast(44, weights, 2);
    XBMorphStreams = 0;
    if (XB->m_XtraTypes & 0x10) {
        int last = *(u32 *)XtraData(4) - 1;
        int a = pCached->m_RenderInfo.VertexSet1();
        a = a >= last ? last : (a > 0 ? a : 0);
        int b = pCached->m_RenderInfo.VertexSet2();
        b = b >= last ? last : (b > 0 ? b : 0);
        xb::D3DVertexBuffer *pA = GetXYZVertexBuffer(a);
        xb::D3DVertexBuffer *pB = GetXYZVertexBuffer(b);
        weights[0] = 1.0f - pCached->m_RenderInfo.VertexSetBlend();
        XBVertexShader.TurnOnStream(0x42, 0, 6, 2, 0, 0, 0);
        XBVertexShader.SetVertexStream(2, pA, 0x10, 0);
        weights[1] = pCached->m_RenderInfo.VertexSetBlend();
        XBVertexShader.TurnOnStream(0x42, 0, 7, 3, 0, 0, 0);
        XBVertexShader.SetVertexStream(3, pB, 0x10, 0);
        XBMorphStreams = 2;
        D3DDevice_SetVertexShaderConstantFast(44, weights, 2);
        XBVertexShader.SetBit(1);
        EXVector zero;
        zero.x = 0.0f;
        zero.y = 0.0f;
        zero.z = 0.0f;
        zero.w = 0.0f;
        D3DDevice_SetVertexShaderConstantFast(74, &zero, 1);
    }
    if (ms_pGDiSkinData) CalculateVSSkin(pHdr, pData);
    u32 *pSets = (u32 *)XtraData(5);
    int nSets = 1;
    if (pSets) nSets = *pSets;
    float blend = pCached->m_RenderInfo.LightSetBlend();
    int last = nSets - 1;
    unsigned int skinInputs = 0;
    unsigned int input = 0;
    int a = pCached->m_RenderInfo.LightSet1();
    a = a >= last ? last : (a > 0 ? a : 0);
    int b = pCached->m_RenderInfo.LightSet2();
    b = b >= last ? last : (b > 0 ? b : 0);
    EXVector c;
    c.x = blend;
    c.y = 0.0f;
    c.z = 0.0f;
    c.w = 0.0f;
    D3DDevice_SetVertexShaderConstantFast(68, &c, 1);
    xb::D3DVertexBuffer *pA = GetRGBAVertexBuffer(a);
    xb::D3DVertexBuffer *pB = GetRGBAVertexBuffer(b);
    if (nSets > 1 && blend > 0.0f && blend < 1.0f) blendRGBA = 1;
    if (blend >= 1.0f || blend <= 0.0f) {
        pA = GetRGBAVertexBuffer(a);
        pB = GetRGBAVertexBuffer(a);
    }
    if (g_uiUseSubdivisionSurface && XB->m_pSubDiv->GetLoD(uiSRRLod)->GetRGBAStream()) {
        pA = XB->m_pSubDiv->GetLoD(uiSRRLod)->GetRGBAStream();
        pB = XB->m_pSubDiv->GetLoD(uiSRRLod)->GetRGBAStream();
    }
    XBVertexShader.SetVertexStream(1, pA, 4, 0);
    nextStream = 2;
    if (XB->m_XtraTypes & 0x10) nextStream = XBMorphStreams + 2;
    if (ms_pGDiSkinData) {
        nextStream = 3;
        input = 1;
        if (!(pData->m_Flags & 1) && !(m_Flags & 0x2000)) skinInputs = 1;
        if ((*(XBSkinBlock **)ms_pGDiSkinData)->m_MorphIndex >= 0 && XBMorphStreams) {
            nextStream = XBMorphStreams + 3;
            if (!(pData->m_Flags & 1) && !(m_Flags & 0x2000)) {
                skinInputs = 2;
                input = 2;
            }
        }
    }
    if (blendRGBA) {
        rgbaInput = input;
        XBVertexShader.TurnOnStream(0x40, rgbaInput, 0xe, nextStream, 0, 0, 0);
        XBVertexShader.SetVertexStream(nextStream, pB, 4, 0);
    }
    XBVertexShader.RewireVertexShaderInputs(skinInputs);
    return blendRGBA;
}

// Snapshot the transforms and render environment into an EXCachedGeoEntity and queue it on the
// active window: the shadow-map pass (shadow and light lists), refraction, heat, or by render order
// (0-2 opaque/alpha, 3 before and 4 after the refraction pass, 5 unsorted without Z).
void EXGeoEntity::Draw(EXGDiData *pData, unsigned int)
{
    if (m_Flags & 0x400) return;
    if (!EXBaseDisplay::GeoHeader()) return;
    if (!pData) return;
    if (!VertexCount()) return;
    if (!TriStripCount()) return;
    EXCachedGeoEntity e;
    e.m_pHeader = EXBaseDisplay::GeoHeader();
    e.m_pGDiData = pData;
    e.m_pEntity = this;
    e.m_ObjectToWorld = EXBaseDisplay::ObjectToWorld();
    e.m_WorldToView = EXBaseDisplay::WorldToView();
    e.m_ObjectToScreen = EXBaseDisplay::ObjectToScreen();
    e.m_ZReadDisabled = EXBaseRenderEnv::ZReadDisabled();
    e.m_ZWriteDisabled = EXBaseRenderEnv::ZWriteDisabled();
    e.m_DistanceFade = 0;
    e.m_ShadowCaster = 0;
    e.m_ShadowReceiver = 0;
    e.m_RenderInfo = EXBaseRenderEnv::RenderInfo();
    e.m_ReflectionRender = EXBaseRenderEnv::ReflectionRender();
    e.m_FadeLayerMask = EXBaseRenderEnv::DistanceFadeLayerMask();
    e.m_pSkinData = ms_pGDiSkinData;
    e.m_FadeOffset = XBWorldScale * -1000.0f;
    e.m_Fog.m_enabled = 0;
    e.m_FadeScale = 1.0f;
    s8 fade = (1 - (EXBaseRenderEnv::DistanceFadeLayerMask() & 1)) * EXBaseRenderEnv::DistanceFade();
    e.m_Diffuse[0] = 1.0f;
    e.m_Diffuse[1] = 1.0f;
    e.m_Diffuse[2] = 1.0f;
    e.m_Diffuse[3] = 1.0f;
    e.m_Ambient[0] = 1.0f;
    e.m_Ambient[1] = 1.0f;
    e.m_Ambient[2] = 1.0f;
    e.m_Ambient[3] = 1.0f;
    if (e.m_RenderInfo.HeatRender() && !(m_Flags & 1)) e.m_RenderInfo.EnableHeatRender(0);
    if (EXBaseRenderEnv::ShadowRender()) {
        if (EXBaseRenderEnv::ShadowDrawDisabled()) return;
        e.m_ShadowCaster = 1;
        e.m_DynLightCount = 0;
        D3DXMatrixMultiply(&e.m_ObjectToClip, &EXBaseDisplay::ObjectToWorld(), &XBShadowController.m_WorldToShadowClip);
        if (m_Flags & 0x88000) {
            if (!(m_Flags & 0x10000)) SetShadowGlobalDiffuseAmbient(e.m_Diffuse);
            if (m_Flags & 0x8000) EXBaseDisplay::ActiveWnd()->AddLightGeoEntity(e);
            if (m_Flags & 0x80000) EXBaseDisplay::ActiveWnd()->AddIdShadowGeoEntity(e);
            return;
        }
        if (m_Flags & 0x40000000) return;
        s8 alpha = ((m_Flags & 0xc0000000) || EXBaseRenderEnv::AlphaMultEnabled()) ? 1 : 0;
        if (!(EXBaseDisplay::ActiveWnd()->m_Flags.m_flags & 0x40000)) return;
        if (alpha) return;
        g_uiShadowEntities++;
        SetShadowGlobalDiffuseAmbient(e.m_Diffuse);
        EXBaseDisplay::ActiveWnd()->AddShadowGeoEntity(e);
        return;
    }
    if (e.m_RenderInfo.RefractionRender() && e.m_RenderInfo.ExtRenderInfo() && e.m_RenderInfo.RefractionBlur())
        EXBaseDisplay::ActiveWnd()->m_EfxFlags |= 2;
    if (m_Flags & 0x4000) {
        xb::D3DXMATRIX tmp;
        D3DXMatrixMultiply(&tmp, &EXBaseDisplay::ObjectToWorld(), &EXBaseDisplay::WorldToShadowCull());
        EXMaths::mat_44 m = *(EXMaths::mat_44 *)&tmp;
        if (EXDisplay::CullBox((const float *)&m_BoundsBox, m)) e.m_ShadowReceiver = 1;
        g_uiShadowedEntities += e.m_ShadowReceiver;
    }
    SetGlobalDiffuseAmbient(e.m_Diffuse, e.m_Ambient);
    if (EXBaseRenderEnv::DistanceFade() && EXBaseRenderEnv::DistanceFadeStart() != EXBaseRenderEnv::DistanceFadeEnd()) {
        e.m_FadeOffset = EXBaseRenderEnv::DistanceFadeEnd() * XBWorldScale;
        e.m_FadeScale = 1.0f / (EXBaseRenderEnv::DistanceFadeStart() * XBWorldScale - EXBaseRenderEnv::DistanceFadeEnd() * XBWorldScale);
        e.m_DistanceFade = 1;
    }
    D3DXMatrixMultiply(&e.m_ObjectToClip, &EXBaseDisplay::ObjectToWorld(), &((EXDisplay *)EXBaseDisplay::GetDisplay())->m_ViewProjection);
    if (EXBaseRenderEnv::RenderFogInfo().m_enabled && !(m_Flags & 0x10)) {
        e.m_Fog.m_near = EXBaseRenderEnv::FogNear();
        e.m_Fog.m_far = EXBaseRenderEnv::FogFar();
        e.m_Fog.m_min = EXBaseRenderEnv::FogMin();
        e.m_Fog.m_enabled = EXBaseRenderEnv::RenderFogInfo().m_enabled;
        e.m_Fog.m_color = EXBaseRenderEnv::FogColor();
        e.m_Fog.m_max = EXBaseRenderEnv::FogMax();
    }
    EXBaseRenderEnv::SetDynLights(m_BoundsBox);
    e.m_DynLightCount = EXBaseRenderEnv::DynLightCount();
    for (unsigned int i = 0; i < e.m_DynLightCount; i++) e.m_DynLights[i] = EXBaseRenderEnv::DynLight(i);
    e.m_DynLightRGBMultEnabled = EXBaseRenderEnv::DynLightRGBMultEnabled();
    if (e.m_DynLightRGBMultEnabled) e.m_DynLightRGBMult = EXBaseRenderEnv::DynLightRGBMult();
    if (EXBaseRenderEnv::RefractionRender()) {
        e.m_Fog.m_enabled = 0;
        EXBaseDisplay::ActiveWnd()->AddRefractionGeoEntity(e);
        return;
    }
    if (e.m_RenderInfo.HeatRender()) {
        e.m_Fog.m_enabled = 0;
        e.m_DistanceFade = 0;
        EXBaseDisplay::ActiveWnd()->AddHeatGeoEntity(e);
        return;
    }
    unsigned char order = RenderOrder();
    if (order == 5) {
        e.m_RenderInfo.DisableZWrite(1);
        e.m_RenderInfo.DisableZRead(1);
        EXBaseDisplay::ActiveWnd()->AddAlphaFogGeoEntity(e);
    } else if (order == 0 || order == 1 || order == 2) {
        if ((m_Flags & 0xe0000000) || EXBaseRenderEnv::AlphaMultEnabled() || fade)
            EXBaseDisplay::ActiveWnd()->AddAlphaGeoEntity(e);
        else
            EXBaseDisplay::ActiveWnd()->AddNormalGeoEntity(e);
    } else {
        switch (order) {
        case 3:
            if ((m_Flags & 0xe0000000) || EXBaseRenderEnv::AlphaMultEnabled() || fade)
                EXBaseDisplay::ActiveWnd()->AddPreRefractionAlphaGeoEntity(e);
            else
                EXBaseDisplay::ActiveWnd()->AddPreRefractionGeoEntity(e);
            break;
        case 4:
            if ((m_Flags & 0xe0000000) || EXBaseRenderEnv::AlphaMultEnabled() || fade)
                EXBaseDisplay::ActiveWnd()->AddPostRefractionAlphaGeoEntity(e);
            else
                EXBaseDisplay::ActiveWnd()->AddPostRefractionGeoEntity(e);
            break;
        }
    }
}

// Draw one queued entity. Per entity: Z/cull/blend defaults, fog, vertex and index buffers (or the
// subdivision LoD's), lighting or the refraction/heat camera vector, c0-c3 = object to clip, shadow
// map sampling, vertex streams. Then each strip with its texture, fade, environment map, lighting
// bits and blend mode, drawn as an indexed triangle strip from the running index offset.
void EXGeoEntity::DrawDirectX(EXCachedGeoEntity *pCached)
{
    UpdateOrigin();
    ((EXDisplay *)EXBaseDisplay::GetDisplay())->SetCurrentDifferedEntity(pCached);
    EXGDiData *pData = pCached->m_pGDiData;
    EXGeoHeader *pHdr = pCached->m_pHeader;
    if ((pData->m_Flags & 8) && pCached->m_pSkinData)
        ms_pGDiSkinData = pCached->m_pSkinData;
    else
        ms_pGDiSkinData = 0;
    if (pData->m_Flags & 1) ((EXDisplay *)EXBaseDisplay::GetDisplay())->InvalidateWriteCache();
    XBVertexShader.SetGlobalDiffuse(pCached->m_Diffuse);
    XBVertexShader.SetGlobalAmbient(pCached->m_Ambient);
    EXVector fade;
    fade.z = pCached->m_FadeOffset;
    fade.w = pCached->m_FadeScale;
    D3DDevice_SetVertexShaderConstantFast(75, &fade, 1);
    EXTextureArray *pTextures = pHdr->m_pTextureArray;
    unsigned int bits = 0;
    unsigned int rgbaInput = 0;
    unsigned int index = 0;
    s8 zAlways = ((m_Flags & 0x1000) || pCached->m_ZReadDisabled || pCached->m_ShadowCaster) ? 1 : 0;
    XBRenderState.SetRenderState(xb::D3DRS_ZFUNC, zAlways * 4 + 0x203);
    int cull = 0x901 - pCached->m_ReflectionRender;
    XBRenderState.SetRenderState(xb::D3DRS_CULLMODE, cull);
    XBRenderState.SetRenderState(xb::D3DRS_ZENABLE, 2);
    s8 alpha = ((m_Flags & 0xc0000000) || pCached->m_RenderInfo.AlphaMultEnabled()) ? 1 : 0;
    XBRenderState.SetRenderState(xb::D3DRS_ALPHABLENDENABLE, alpha);
    XBRenderState.SetRenderState(xb::D3DRS_ALPHAREF, 0);
    s8 zWrite = (((m_Flags & 0x800) || alpha || pCached->m_ZWriteDisabled || pCached->m_ShadowCaster) && !(m_Flags & 8)) ? 0 : 1;
    XBRenderState.SetRenderState(xb::D3DRS_ZWRITEENABLE, zWrite);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ALPHAOP, ((pData->m_Flags >> 7) & 1) + 4);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_COLOROP, 5);
    XBVertexShader.ClearAllStreams();
    XBVertexShader.TurnOffStream(0, 0xe, 2);
    XBVertexShader.TurnOffStream(1, 0xe, 2);
    XBVertexShader.ClearBitField();
    XBVertexShader.SetFog(pCached->m_Fog.m_enabled, pCached->m_Fog.m_color, pCached->m_Fog.m_min, pCached->m_Fog.m_max,
                          pCached->m_Fog.m_near, pCached->m_Fog.m_far);
    pData->m_VertexBuffer = (u32)GetVertexBuffer();
    pData->m_IndexBuffer = (u32)GetIndexBuffer();
    g_uiUseSubdivisionSurface = 0;
    XBRenderState.SetRenderState(xb::D3DRS_FILLMODE, 0x1b02);
    EXGeoEntity_TriStrip *pStrips = TriStrip();
    if (!(XB->m_XtraTypes & 0x10) && (m_Flags & 0x10000000) && uiSRRLod) {
        if (!XB->m_pSubDiv) XB->m_pSubDiv = new ExSubDivisionMesh;
        XB->m_pSubDiv->BuidLoD(uiSRRLod, this, ms_pGDiSkinData, pData, *(const EXMatrix *)&pCached->m_ObjectToWorld, 0, 0);
        g_uiUseSubdivisionSurface = 1;
        if (XB->m_pSubDiv->GetLoD(uiSRRLod)) {
            if (bLayerOn[4] == 1) XBRenderState.SetRenderState(xb::D3DRS_FILLMODE, 0x1b01);
            if (XB->m_pSubDiv->GetLoD(uiSRRLod)->GetVertexStream())
                pData->m_VertexBuffer = (u32)XB->m_pSubDiv->GetLoD(uiSRRLod)->GetVertexStream();
            if (XB->m_pSubDiv->GetLoD(uiSRRLod)->GetIndexStream())
                pData->m_IndexBuffer = (u32)XB->m_pSubDiv->GetLoD(uiSRRLod)->GetIndexStream();
        }
    }
    if (pCached->m_RenderInfo.RefractionRender() || pCached->m_RenderInfo.HeatRender())
        CalculateCameraToObject(pCached);
    else if (!pCached->m_ShadowCaster)
        SetUpLighting(pCached, bits);
    D3DDevice_SetVertexShaderConstantFast(0, &pCached->m_ObjectToClip, 4);
    SetUpShadows(pCached);
    if (m_Flags & 1) BuildTextureRemap(pCached);
    unsigned int nextStream;
    s8 blendRGBA = SetUpVertexStreams(pCached, nextStream, rgbaInput);
    s8 blend = 0;
    SetUpSpecialFX(pCached, blend);
    if (pCached->m_ShadowCaster && blend) {
        // Alpha-tested shadow casters: one texture for the whole entity.
        (*pTextures)[0].Select();
        for (unsigned int i = 0; i < TriStripCount(); i++) {
            EXGeoEntity_TriStrip *pStrip = &pStrips[i];
            if (!(pStrip->m_Flags & 0x10) && !(pStrip->m_Flags & 0x800) && !(pStrip->m_TransType & 3)) {
                if (pStrip->m_Flags & 0x40) XBRenderState.SetRenderState(xb::D3DRS_CULLMODE, 0);
                if (pStrip->m_Flags & 0x80) XBRenderState.SetRenderState(xb::D3DRS_CULLMODE, cull);
                xb::D3DDevice::DrawIndexedPrimitive(xb::D3DPT_TRIANGLESTRIP, 0, 0, index, pStrip->m_FaceCount);
                if (pStrip->m_Flags & 0xc0) XBRenderState.SetRenderState(xb::D3DRS_CULLMODE, cull);
            }
            index += pStrip->m_FaceCount + 2;
        }
    } else if (!blend) {
        for (unsigned int i = 0; i < TriStripCount(); i++) {
            EXGeoEntity_TriStrip *pStrip = &pStrips[i];
            if (pStrip->m_Flags & 0x10) {
                index += pStrip->m_FaceCount + 2;
                continue;
            }
            // Distance fade (c75.zw): strips on a layer in the mask keep the far "no fade" values.
            unsigned int keep = (pCached->m_FadeLayerMask & (1 << pStrip->m_Layer)) >> pStrip->m_Layer;
            EXVector f;
            f.z = XBWorldScale * -1000.0f * keep + (1 - keep) * pCached->m_FadeOffset;
            f.w = (1 - keep) * pCached->m_FadeScale + keep;
            D3DDevice_SetVertexShaderConstantFast(75, &f, 1);
            short tex = (short)pStrip->m_Texture;
            EXTexture *pTex = &(*pTextures)[tex];
            if (m_Flags & 1) pTex = ((EXTexture **)pData->m_pTexRemap)[tex + 1];
            XBSetTexture(pTex);
            EXVector uv;
            uv.x = pTex->UOffset();
            uv.y = pTex->VOffset();
            uv.z = 0.0f;
            uv.w = 0.0f;
            D3DDevice_SetVertexShaderConstantFast(29, &uv, 1);
            SetUpEnvironmentMapping(pTex, (xb::_D3DMATRIX *)&pCached->m_ObjectToWorld, (xb::_D3DMATRIX *)&pCached->m_WorldToView);
            SetUpStripLighting(bits, pStrip);
            SetUpTransType(pCached, pStrip);
            if (pStrip->m_Flags & 0x400) {
                XBVertexShader.ClearBit(0x80);
                XBRenderState.SetTextureStageState(1, xb::D3DTSS_COLOROP, 1);
            } else if (m_Flags & 0x4000) {
                XBVertexShader.SetBit(pCached->m_ShadowReceiver << 7);
                XBRenderState.SetTextureStageState(1, xb::D3DTSS_COLOROP, pCached->m_ShadowReceiver * 3 + 1);
            }
            XBVertexShader.SetVertexShader();  // retail inlines this one
            if (pStrip->m_Flags & 0x40) XBRenderState.SetRenderState(xb::D3DRS_CULLMODE, 0);
            if (pStrip->m_Flags & 0x80) XBRenderState.SetRenderState(xb::D3DRS_CULLMODE, cull);
            if (!g_uiUseSubdivisionSurface)
                xb::D3DDevice::DrawIndexedPrimitive(xb::D3DPT_TRIANGLESTRIP, 0, 0, index, pStrip->m_FaceCount);
            else
                XB->m_pSubDiv->Render(uiSRRLod, i);
            if (pStrip->m_Flags & 0xc0) XBRenderState.SetRenderState(xb::D3DRS_CULLMODE, cull);
            index += pStrip->m_FaceCount + 2;
        }
    } else {
        // Refraction and heat: opaque strips only, heat with their own textures.
        for (unsigned int i = 0; i < TriStripCount(); i++) {
            EXGeoEntity_TriStrip *pStrip = &pStrips[i];
            if (!(pStrip->m_Flags & 0x10) && !(pStrip->m_TransType & 3)) {
                if (pCached->m_RenderInfo.HeatRender()) XBSetTexture(((EXTexture **)pData->m_pTexRemap)[pStrip->m_Texture + 1]);
                if (pStrip->m_Flags & 0x40) XBRenderState.SetRenderState(xb::D3DRS_CULLMODE, 0);
                xb::D3DDevice::DrawIndexedPrimitive(xb::D3DPT_TRIANGLESTRIP, 0, 0, index, pStrip->m_FaceCount);
                if (pStrip->m_Flags & 0xc0) XBRenderState.SetRenderState(xb::D3DRS_CULLMODE, cull);
            }
            index += pStrip->m_FaceCount + 2;
        }
    }
    if (!pCached->m_ShadowCaster) pData->m_Flags &= ~1;
    XBRenderState.SetRenderState(xb::D3DRS_ALPHABLENDENABLE, 1);
    XBRenderState.SetRenderState(xb::D3DRS_ALPHAREF, 0);
    XBRenderState.SetRenderState(xb::D3DRS_CULLMODE, 0x901);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_COLOROP, 5);
    XBRenderState.SetTextureStageState(0, xb::D3DTSS_ALPHAOP, 4);
    XBRenderState.SetRenderState(xb::D3DRS_ZWRITEENABLE, 1);
    XBRenderState.SetRenderState(xb::D3DRS_ZFUNC, 0x203);
    XBRenderState.SetRenderState(xb::D3DRS_FOGENABLE, 0);
    XBRenderState.SetTexture(1, 0);
    XBRenderState.SetTexture(2, 0);
    XBRenderState.SetTextureStageState(1, xb::D3DTSS_COLOROP, 1);
    xb::D3DDevice::SetVertexShaderInputDirect(0, 0, 0);
    if (blendRGBA) XBVertexShader.TurnOffStream(rgbaInput, 0xe, nextStream);
}

// Component-wise min/max of two vectors (the map's copies are in XGeoEntity_NavMesh.obj; not in a
// shared header yet because an EXVector overload would capture other units' EXMaths::Min calls).
__forceinline EXVector Min(const EXVector &a, const EXVector &b) { return EXVector(EXMaths::Min(a, b)); }
__forceinline EXVector Max(const EXVector &a, const EXVector &b) { return EXVector(a.Max(b)); }

typedef EXGeoEntity_FaceCollision XBFaceCollisionHeader;  // EXGeoEntity_FaceRef.h

// Vertical shadow ray: find the collision faces straight below and above t.m_pos (within
// t.m_height..t.m_radius in y) and keep the nearest ones in t.m_faces (0 floor below, 1 floor
// below flagged 1 (see-through), 3 ceiling above, 2 ceiling above flagged 1). Returns the slots set.
unsigned int EXGeoEntity::DoShadowCast(EXShadowTemp &t)
{
    if (m_BoundsBox.m_vctMin.x > t.m_pos.x) return 0;
    if (m_BoundsBox.m_vctMin.z > t.m_pos.z) return 0;
    if (m_BoundsBox.m_vctMin.y > t.m_radius) return 0;
    if (t.m_pos.x > m_BoundsBox.m_vctMax.x) return 0;
    if (t.m_pos.z > m_BoundsBox.m_vctMax.z) return 0;
    if (t.m_height > m_BoundsBox.m_vctMax.y) return 0;
    XBFaceCollisionHeader *pFC = (XBFaceCollisionHeader *)FaceCollision();
    if (!pFC) return 0;
    EXGeoEntity_FaceNormals *pNormals = GetFaceNormalsBuffer(0);
    if (!pNormals) return 0;
    EXGeoEntity_VtxData *pVtx = Vtx();
    EXVector v[3];
    EXVector pos = t.m_pos;
    unsigned int result = 0;
    const float *pN = (const float *)pNormals;
    EXGeoEntity_FaceCollisionGroup *pGroup = pFC->m_Groups;
    int nGroups = pFC->m_GroupCount;
    for (int g = 0; g < nGroups; g++) {
        EXGeoEntity_FaceCollisionFace *pFace = pGroup->m_faces;
        int nFaces = pGroup->m_02;
        for (int i = 0; i < nFaces; i++, pFace++, pN += 4) {
            v[0] = *(EXVector *)&pVtx[pFace->m_v[0]];
            v[1] = *(EXVector *)&pVtx[pFace->m_v[1]];
            v[2] = *(EXVector *)&pVtx[pFace->m_v[2]];
            EXVector mn = Min(Min(v[0], v[1]), v[2]);
            if (mn.x > pos.x) continue;
            if (mn.z > pos.z) continue;
            if (mn.y > t.m_radius) continue;
            EXVector mx = Max(Max(v[0], v[1]), v[2]);
            if (pos.x > mx.x) continue;
            if (pos.z > mx.z) continue;
            if (t.m_height > mx.y) continue;
            if (!pos.PointInTriY(v)) continue;
            // Height of the face's plane at pos.
            float inv = pN[1];
            if (inv != 0.0f) inv = 1.0f / inv;
            float y = pN[2] * inv * (v[0].z - pos.z) + inv * pN[0] * (v[0].x - pos.x) + v[0].y;
            if (y < t.m_height) continue;
            if (t.m_radius < y) continue;
            int slot;
            if (pos.y >= y) {
                if (pFace->m_flags & 1) {
                    if (!(y > t.m_faces[1].m_dist)) continue;
                    slot = 1;
                } else {
                    if (y > t.m_faces[1].m_dist) t.m_numFaces &= ~2;
                    t.m_height = y;
                    slot = 0;
                }
            } else {
                if (pFace->m_flags & 1) {
                    if (!(t.m_faces[2].m_dist > y)) continue;
                    slot = 2;
                } else {
                    if (t.m_faces[2].m_dist > y) t.m_numFaces &= ~4;
                    t.m_radius = y;
                    slot = 3;
                }
            }
            t.m_numFaces |= 1 << slot;
            result |= 1 << slot;
            EXShadowFace &f = t.m_faces[slot];
            f.m_dist = y;
            f.m_v[0] = v[0];
            f.m_v[0].w = 0.0f;
            f.m_v[1] = v[1];
            f.m_v[1].w = 0.0f;
            f.m_v[2] = v[2];
            f.m_v[2].w = 0.0f;
            f.m_face.Set(this, 0, pGroup, pFace);
        }
        pGroup = (EXGeoEntity_FaceCollisionGroup *)pFace;
    }
    return result;
}

#include "EXGeoColSphere.h"

// Ray against the collision faces (in the entity's space: t.m_localStart/m_localDir). Bit 0 of flags
// skips the bounds and collision sphere rejection. Keeps the nearest hit (or the first one when
// t.m_bAnyHit) in t; returns whether this entity was hit.
unsigned int EXGeoEntity::DoRayCast(EXRayCastTemp &t, EXGDiData *pData, unsigned int flags)
{
    if (!(flags & 1)) {
        if (!m_BoundsBox._Intersect(t.m_localBounds)) return 0;
        EXGeoColSphere_Table *pTab = *(ex::EXRelPtr<EXGeoColSphere_Table> *)&m_44;
        if (pTab && !DoRayCast_CollisionSpheres(pTab->NodeList(), pTab->Spheres(), (const EXLineSeg &)t)) return 0;
    }
    XBFaceCollisionHeader *pFC = (XBFaceCollisionHeader *)FaceCollision();
    if (!pFC) return 0;
    const EXPlane *pN = (const EXPlane *)GetFaceNormalsBuffer(pData);
    if (!pN) return 0;
    EXVector v[3];
    s8 bRelease;
    EXGeoEntity_VtxData *pVtx;
    if (pData && (pData->m_Flags & 1)) {
        bRelease = 1;
        pVtx = (EXGeoEntity_VtxData *)GetVertexBuffer(pData, 1);
    } else {
        bRelease = 0;
        pVtx = Vtx();
    }
    EXVector start(t.m_localStart.x, t.m_localStart.y, t.m_localStart.z, 0.0f);
    EXVector dir(t.m_localDir.x, t.m_localDir.y, t.m_localDir.z, 0.0f);
    s8 bAny = t.m_bAnyHit;
    s8 bHit = 0;
    EXGeoEntity_FaceCollisionGroup *pGroup = pFC->m_Groups;
    EXGeoEntity_FaceCollisionFace *pFace;
    for (int g = pFC->m_00; g > 0; g--) {
        pFace = pGroup->m_faces;
        for (int i = pGroup->m_02; i > 0; i--, pFace++, pN++) {
            float denom = dir.DotXYZ(*(const EXVector *)pN);
            if (denom == 0.0f) continue;
            float d = (start.DotXYZ(*(const EXVector *)pN) + pN->D) * (-1.0f / denom);
            if (d < 0.0f) continue;
            if (t.m_t < d) continue;
            if (t.m_flags && !(pFace->m_flags & t.m_flags)) continue;
            if (t.m_mask && (pFace->m_flags & t.m_mask)) continue;
            v[0] = *(EXVector *)&pVtx[pFace->m_v[0]];
            v[1] = *(EXVector *)&pVtx[pFace->m_v[1]];
            v[2] = *(EXVector *)&pVtx[pFace->m_v[2]];
            EXVector n(pN->A, pN->B, pN->C, 0.0f);
            EXVector p(dir.x * d + start.x, dir.y * d + start.y, dir.z * d + start.z, 1.0f);
            if ((p - v[0]).Dot((v[0] - v[2]).Cross(n)) > 0.0f) continue;
            if ((p - v[1]).Dot((v[1] - v[0]).Cross(n)) > 0.0f) continue;
            if ((p - v[2]).Dot((v[2] - v[1]).Cross(n)) > 0.0f) continue;
            t.m_t = d;
            t.m_normal = *(const EXVector *)pN;
            t.m_face._set(this, pData, pGroup, pFace);
            bHit = 1;
            if (bAny) goto found;
        }
        pGroup = (EXGeoEntity_FaceCollisionGroup *)pFace;
    }
    if (bRelease) ReleaseVertexBuffer(pData);
    if (!bHit) return bHit;
found:
    t.m_face.SetCheckCode();
    t.CalcBounds();
    t.m_bHit |= bHit;
    return bHit;
}

// One gathered piece: this entity with the array's current matrix.
void EXGeoEntity::BuildEntityInfoArray(EXGeoEntity_InfoArray &arr, EXGDiData *pData)
{
    EXGeoEntity_Info info;
    info.Set(this, pData, *(const EXMatrix *)arr.m_10);
    arr.Add(info, 1);
}

// The plane through a, b, c (the EXPlane constructor, inlined here).
static __forceinline void XBFacePlane(EXPlane &p, const EXVector &a, const EXVector &b, const EXVector &c)
{
    EXVector n = (b - a).Cross(c - b);
    p.A = n.x; p.B = n.y; p.C = n.z; p.D = n.w;
    float len = EXSqrt(p.A * p.A + p.B * p.B + p.C * p.C);
    if ((float)fabs(len) != 0.0f) {
        float inv = 1.0f / len;
        p.A = inv * p.A; p.B *= inv; p.C *= inv; p.D *= inv;
        p.D = 0.0f - (p.A * a.x + p.B * a.y + p.C * a.z);
    } else {
        p.A = 1.0f; p.B = 0.0f; p.C = 0.0f; p.D = 0.0f;
    }
}

// Plane of each collision face, cached in a heap block at m_70 (kept 0x20 frames). Rebuilt each
// call when the vertices move (skinned, or rewritten this frame), from the rewritten vertices.
EXGeoEntity_FaceNormals *EXGeoEntity::GetFaceNormalsBuffer(EXGDiData *pData)
{
    s8 bMoving = pData ? ((pData->m_Flags & 8) ? 1 : (s8)(pData->m_Flags & 1)) : 0;
    XBFaceCollisionHeader *pFC = (XBFaceCollisionHeader *)FaceCollision();
    void **ppNormals = &XB->m_70;
    if (!*ppNormals) {
        int one = 1;
        int n = pFC->m_04;
        if (!_SystemHeapList[1]->_EXCacheDel(max(one, n) * sizeof(EXPlane), 8, 0, ppNormals, 8)) return 0;
    } else if (!bMoving) {
        EXAllocSetTime(*ppNormals, 0x20, 0);
        return (EXGeoEntity_FaceNormals *)*ppNormals;
    }
    EXGeoEntity_VtxData *pVtx;
    if (pData && (pData->m_Flags & 1))
        pVtx = (EXGeoEntity_VtxData *)GetVertexBuffer(pData, 1);
    else
        pVtx = Vtx();
    EXPlane *pPlane = (EXPlane *)*ppNormals;
    EXGeoEntity_FaceCollisionGroup *pGroup = pFC->m_Groups;
    for (int g = 0; g < pFC->m_GroupCount; g++) {
        EXGeoEntity_FaceCollisionFace *pFace = pGroup->m_faces;
        for (int i = 0; i < pGroup->m_02; i++, pFace++, pPlane++) {
            EXGeoEntity_VtxData &v0 = pVtx[pFace->m_v[0]];
            EXGeoEntity_VtxData &v1 = pVtx[pFace->m_v[1]];
            EXGeoEntity_VtxData &v2 = pVtx[pFace->m_v[2]];
            XBFacePlane(*pPlane, EXVector(v0.m_x, v0.m_y, v0.m_z, 0.0f), EXVector(v1.m_x, v1.m_y, v1.m_z, 0.0f),
                        EXVector(v2.m_x, v2.m_y, v2.m_z, 0.0f));
        }
        pGroup = (EXGeoEntity_FaceCollisionGroup *)pFace;
    }
    EXAllocSetTime(*ppNormals, 0x20, 0);
    return (EXGeoEntity_FaceNormals *)*ppNormals;
}

// Morph target set of a skinned entity, into pOut (the entity's dynamic vertices). Type 0 adds each
// deformer weighted by the renderer's morph weights; type 1 blends between the two key deformers
// either side of the frame held in the first morph weight.
EXGeoEntity_VtxData *EXGeoEntity::ApplySkinMorph(EXGeoAnimSkin_MorphList *pMorph, EXItemRender_SkinAnim *pRender, void *pOut)
{
    unsigned int offset = pMorph->DeformerOffset();
    int n = VertexCount();
    ex::EXRELOFFSET32 *pDefs = (ex::EXRELOFFSET32 *)XtraData(2);
    EXGeoEntity_VtxData *pSrc = Vtx();
    EXGeoEntity_VtxData *pDst = (EXGeoEntity_VtxData *)pOut;
    switch (pMorph->MorphType()) {
    case 0:
        memcpy(pDst, pSrc, n * sizeof(EXGeoEntity_VtxData));
        for (unsigned int i = 0; i < pMorph->DeformerCount(); i++) {
            float w = ((float *)pRender->MorphDeformers())[offset + i];
            if (w > 0.0001f) {
                EXVector *pD = (EXVector *)ex::EXRelOffset(pDefs[i]);
                for (int v = 0; v < n; v++) {
                    EXVector d = pD[v] * w;
                    EXGeoEntity_VtxData &o = pDst[v];
                    o.m_x += d.x;
                    o.m_y += d.y;
                    o.m_z += d.z;
                }
            }
        }
        break;
    case 1: {
        float f = *(float *)pRender->MorphDeformers();
        u16 *pKeys = (u16 *)pMorph->XtraData();
        int last = pKeys[0] - 1;
        unsigned int a = pKeys[1 + limit((int)f, 0, last)];
        unsigned int b = pKeys[1 + limit((int)f + 1, 0, last)];
        f = (float)fmod((double)f, 1.0);
        EXVector *pA = (EXVector *)ex::EXRelOffset(pDefs[a]);
        EXVector *pB = (EXVector *)ex::EXRelOffset(pDefs[b]);
        for (int v = 0; v < n; v++) {
            EXVector p = pA[v].Interp(pB[v], f);
            EXGeoEntity_VtxData &d = pDst[v];
            d.m_x = p.x;
            d.m_y = p.y;
            d.m_z = p.z;
            d.m_Normal = pSrc[v].m_Normal;
        }
        break;
    }
    }
    return pDst;
}

extern "C" xb::D3DXVECTOR4 *__stdcall D3DXVec3Transform(xb::D3DXVECTOR4 *pOut, const xb::D3DXVECTOR3 *pV, const xb::D3DXMATRIX *pM);
// Palette entry of each vertex shader matrix register (XBVertexShader.obj).
extern unsigned int RemapVS2B[159];
// CPU skinning: each palette entry's bone matrix, moved to turn about the bone's rest position.
EXMatrix TempMtxStack[128];

// Adds (or, for the first weight, sets) one bone's weighted transform of the vertex.
#define SKIN_WEIGHT(j)                                                                                  \
    if (pW->m_Weight[j] != 0.0f) {                                                                      \
        EXMatrix *pM = &TempMtxStack[RemapVS2B[pW->m_Bone[j]]];                                         \
        D3DXVec3Transform((xb::D3DXVECTOR4 *)&t, (xb::D3DXVECTOR3 *)pSrc, (xb::D3DXMATRIX *)pM);        \
        t.x *= pW->m_Weight[j];                                                                    \
        t.y *= pW->m_Weight[j];                                                                    \
        t.z *= pW->m_Weight[j];                                                                    \
        t.w *= pW->m_Weight[j];                                                                    \
        acc.x += t.x;                                                                                   \
        acc.y += t.y;                                                                                   \
        acc.z += t.z;                                                                                   \
        acc.w += t.w;                                                                                       \
    }

// CPU skinning of the active skin's entities that need their vertices rewritten (morphed, or flagged
// 0x2000): positions blended over up to four bones, the normal turned by the first bone only.
void EXItemRender_SkinAnim::ApplySkin(s8)
{
    EXGeoAnimSkinHeader *pSkelHdr = m_skelRef.GeoAnimSkinHeader();
    if (!pSkelHdr) return;
    EXGeoAnimSkin *pSkel = pSkelHdr->m_Addr[0].m_pObject;
    if (!pSkel) return;
    EXGeoAnimSkinHeader *pSkinHdr = m_pActiveSkin->m_drawSkinRef.GeoAnimSkinHeader();
    if (!pSkinHdr) return;
    EXGeoAnimSkin *pSkin = pSkinHdr->m_Addr[0].m_pObject;
    if (!pSkin) return;
    EXGeoEntityHeader *pEnts = m_pActiveSkin->m_drawSkinRef.GeoFile()->GeoHeader()->GeoEntityHeader(0);
    if (!pEnts) return;
    EXGeoAnimSkin_SkinList *pList = (EXGeoAnimSkin_SkinList *)pSkin->m_EntityGroups[0].Data();
    EXVector *pRest = *(ex::EXRelPtr<EXVector> *)&pSkel->m_30[0x10];
    EXGDiData *pGDi = m_pActiveSkin->m_gdi.m_pData;
    XBGDiSkinFields *pGDiSkin = (XBGDiSkinFields *)m_pActiveSkin->m_gdi.m_pSkinData;
    pGDi->m_Flags |= 8;
    int numLists = pSkin->LinkedSkinCount() + pSkin->SkinCount();
    unsigned int skinIndex = 0;
    for (int l = 0; l < numLists; l++, pList++) {
        EXGeoAnimSkin_Data *pData = pList->SkinData();
        EXGeoEntity *pEnt = (&pEnts[pList->EntityIndex()])->Object(0);
        s8 bSplit = pEnt->IsKindOf(&EXGeoSplitEntity::classEXGeoSplitEntity);
        for (int k = 0; k < (int)pList->SkinPartCount(); k++) {
            pGDi->m_Flags |= 8;
            pGDiSkin->m_WeightLod = skinIndex++;
            pGDiSkin->m_pSkinList = (XBSkinBlock *)pList;
            pGDiSkin->m_pRender = this;
            pGDiSkin->m_pEntitySkins = (s32 *)pList->SkinData();
            pGDiSkin->m_Skin = m_activeSkin;
            EXGeoEntity *pE = pEnt;
            if (bSplit) pE = ((ex::EXRelPtr<EXGeoEntity> *)((u8 *)pEnt + 0x5c))[k];
            if (pList->MorphIndex() >= 0 && pSkin->MorphList()[pList->MorphIndex()].MorphType() == 1) pGDi->m_Flags |= 1;
            if ((pGDi->m_Flags & 1) || (pE->m_Flags & 0x2000)) {
                EXGeoEntity_VtxData *pVtx = (EXGeoEntity_VtxData *)pE->GetVertexBuffer(pGDi, 1);
                EXGeoAnimSkin_NewData *pES = (EXGeoAnimSkin_NewData *)pData[pE->m_GDiIndex].SkinData();
                EXGeoEntity_VtxData *pSrc = pE->Vtx();
                u8 *pBones = pES->MtxPalette();
                EXMatrix *pMtx = m_pMtx;
                u8 *pRemap = RemapTable();
                if (pList->MorphIndex() >= 0) {
                    pE->ApplySkinMorph(&pSkin->MorphList()[pList->MorphIndex()], this, pVtx);
                    pGDi->m_Flags |= 1;
                }
                for (int b = 0; b < pES->MtxPaletteSize(); b++) {
                    unsigned int bone = pBones[b];
                    if (pRemap) bone = pRemap[bone];
                    TempMtxStack[b] = pMtx[bone];
                    EXVector p = (const EXMaths::mat_44 &)pMtx[bone] * (const EXMaths::vec_xyzw &)pRest[bone];
                    TempMtxStack[b].r[3] -= p;
                }
                if (pList->MorphIndex() >= 0) pSrc = pVtx;
                XBWeightData *pW = (XBWeightData *)pES->WeightTable();
                for (unsigned int v = 0; v < pE->VertexCount(); v++, pW++, pSrc++, pVtx++) {
                    EXMaths::vec_xyzw acc, t;
                    for (int j = 0; j < 4; j++) {
                        if (j == 0) {
                            if (pW->m_Weight[0] != 0.0f) {
                                EXMatrix *pM = &TempMtxStack[RemapVS2B[pW->m_Bone[0]]];
                                D3DXVec3Transform((xb::D3DXVECTOR4 *)&t, (xb::D3DXVECTOR3 *)pSrc, (xb::D3DXMATRIX *)pM);
                                t.x *= pW->m_Weight[0];
                                t.y *= pW->m_Weight[0];
                                t.z *= pW->m_Weight[0];
                                t.w *= pW->m_Weight[0];
                                acc.x = t.x;
                                acc.y = t.y;
                                acc.z = t.z;
                                acc.w = t.w;
                                int n = pSrc->m_Normal;
                                EXVector3 nIn, nOut;
                                nIn.x = (float)((n << 21) >> 21) * (1.0f / 1023.0f);
                                nIn.y = (float)((n << 10) >> 21) * (1.0f / 1023.0f);
                                nIn.z = (float)(n >> 22) * (1.0f / 511.0f);
                                D3DXVec3TransformNormal((xb::D3DXVECTOR3 *)&nOut, (xb::D3DXVECTOR3 *)&nIn, (xb::D3DXMATRIX *)pM);
                                *(u32 *)&pVtx->m_Normal = ((((short)(nOut.y * 1023.0f) & 0x7ff) | ((short)(nOut.z * 511.0f) << 11)) << 11) | ((short)(nOut.x * 1023.0f) & 0x7ff);
                            }
                        } else if (j == 1) {
                            SKIN_WEIGHT(1)
                        } else if (j == 2) {
                            SKIN_WEIGHT(2)
                        } else if (j == 3) {
                            SKIN_WEIGHT(3)
                        }
                    }
                    pVtx->m_x = acc.x;
                    pVtx->m_y = acc.y;
                    pVtx->m_z = acc.z;
                }
                pE->ReleaseVertexBuffer(pGDi);
            }
            pGDi++;
            pGDiSkin++;
        }
    }
    if (m_08) ((void (__cdecl *)(int, EXItemRender_SkinAnim *))m_08)(0, this);
}
