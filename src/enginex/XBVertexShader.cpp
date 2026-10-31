#include "XBBaseVertexShader.h"
#include "XBBaseRenderState.h"
#include "EXVector.h"
#include "RGBA.h"
#include "xbd3d8.h"
#include <string.h>
#include "EXStdFuncs.h"

extern "C" double __cdecl cos(double);
extern "C" double __cdecl sqrt(double);
#pragma intrinsic(cos, sqrt)
inline float sqrt(float f) { return (float)sqrt((double)f); }

// Xbox vertex shaders: the 512 bit-combination programs (lighting, fog, skinning, texture
// generation variants picked by XBBaseVertexShader::m_Bits) and the fixed ones (sprites, lines,
// shadow, heat, refraction, occluder), their input layouts, and the light and fog constants.
//
// Vertex constants (shader registers): c4/c5/c42 directional light directions (negated, view
// space), c6/c7/c43 their colours; c9-c10 environment map matrix; c11 global diffuse, c12 global
// ambient; c13 fog (scale, offset); c14/c15, c18/c19, c21/c22, c32/c33 point lights (position,
// colour), c17 their inverse ranges; c52-c67 spot lights (SetPosDirLights).

extern "C" void *__stdcall D3DXMatrixMultiply(void *pOut, const void *pM1, const void *pM2);
extern "C" xb::D3DXMATRIX *__stdcall D3DXMatrixTranspose(xb::D3DXMATRIX *pOut, const xb::D3DXMATRIX *pM);
extern "C" xb::D3DXVECTOR3 *__stdcall D3DXVec3TransformNormal(xb::D3DXVECTOR3 *pOut, const xb::D3DXVECTOR3 *pV,
                                                              const xb::D3DXMATRIX *pM);

#include "XBVertexShaderData.inc"

// Bit-combination shader index -> bit pattern, and its inverse (built at start up).
unsigned int RemapB2VS[30] = {0,  3,  6,  9,  12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42,
                              45, 48, 51, 54, 60, 63, 66, 69, 72, 75, 78, 81, 84, 87, 90};
unsigned int RemapVS2B[159];

XBBaseVertexShader XBVertexShader;

// The shader index remap is also built during static initialisation.
struct XBVertexShaderRemapInit {
    XBVertexShaderRemapInit()
    {
        for (int i = 0; i < 30; i++) RemapVS2B[RemapB2VS[i]] = i;
    }
};
static XBVertexShaderRemapInit s_RemapInit;

XBBaseVertexShader::XBBaseVertexShader()
{
    m_GlobalDiffuse[0] = 1.0f;
    m_GlobalDiffuse[1] = 1.0f;
    m_GlobalDiffuse[2] = 1.0f;
    m_GlobalDiffuse[3] = 1.0f;
    m_Current = -1;
    for (int i = 0; i < 30; i++) RemapVS2B[RemapB2VS[i]] = i;
    m_ScreenZ = 0.001f;
    m_1e10 = 1000.0f;
}

// Vertex declaration tokens (XDK D3DVSD_* macros) and input types.
#define D3DVSD_STREAM(n) (0x20000000 | (n))
#define D3DVSD_REG(r, t) (0x40000000 | ((t) << 16) | (r))
#define D3DVSD_END() 0xffffffff
enum {
    D3DVSDT_NONE = 0x02,
    D3DVSDT_NORMPACKED3 = 0x16,
    D3DVSDT_FLOAT2 = 0x22,
    D3DVSDT_FLOAT3 = 0x32,
    D3DVSDT_D3DCOLOR = 0x40,
    D3DVSDT_FLOAT4 = 0x42,
};

static __forceinline void ClearAttributes(XBVertexAttributeFormat *pAttr)
{
    memset(pAttr, 0, sizeof(XBVertexAttributeFormat) * 16);
    for (int i = 0; i < 16; i++) pAttr[i].m_Format = D3DVSDT_NONE;
}
static __forceinline void SetAttribute(XBVertexAttributeFormat *pAttr, int i, int format, int stream, int offset)
{
    pAttr[i].m_StreamIndex = stream;
    pAttr[i].m_TessellationSource = 0;
    pAttr[i].m_TessellationType = 0;
    pAttr[i].m_Offset = offset;
    pAttr[i].m_Format = format;
}

// Creates every vertex shader. The bit-combination shaders take one of three declarations by
// their low bits (none: inputs come from the layout in m_Attr through RewireVertexShaderInputs;
// bit 0 skinned; bit 1 normal mapped). Layouts 0-8 describe the fixed vertex formats: stream 0
// position (+0), packed normal (+12), uv; stream 1 diffuse; stream 2 specular and a FLOAT4.
void XBBaseVertexShader::DeclareAllVertexShaders()
{
    xb::DWORD dwDeclNone[] = {D3DVSD_END()};
    XBVertexAttributeFormat *pAttr;
    pAttr = m_Attr[0];
    ClearAttributes(pAttr);
    SetAttribute(pAttr, 0, D3DVSDT_FLOAT3, 0, 0);
    SetAttribute(pAttr, 1, D3DVSDT_NORMPACKED3, 0, 12);
    SetAttribute(pAttr, 2, D3DVSDT_D3DCOLOR, 1, 0);
    SetAttribute(pAttr, 3, D3DVSDT_FLOAT2, 0, 16);
    SetAttribute(pAttr, 4, D3DVSDT_NONE, 0, 0);
    SetAttribute(pAttr, 5, D3DVSDT_NONE, 0, 0);

    xb::DWORD dwDeclNormal[] = {D3DVSD_STREAM(0),
                            D3DVSD_REG(0, D3DVSDT_FLOAT3),
                            D3DVSD_REG(1, D3DVSDT_NORMPACKED3),
                            D3DVSD_REG(3, D3DVSDT_FLOAT2),
                            D3DVSD_STREAM(1),
                            D3DVSD_REG(2, D3DVSDT_D3DCOLOR),
                            D3DVSD_STREAM(2),
                            D3DVSD_REG(5, D3DVSDT_D3DCOLOR),
                            D3DVSD_REG(4, D3DVSDT_FLOAT4),
                            D3DVSD_END()};
    pAttr = m_Attr[1];
    ClearAttributes(pAttr);
    SetAttribute(pAttr, 0, D3DVSDT_FLOAT3, 0, 0);
    SetAttribute(pAttr, 1, D3DVSDT_NORMPACKED3, 0, 12);
    SetAttribute(pAttr, 3, D3DVSDT_FLOAT2, 0, 16);
    SetAttribute(pAttr, 2, D3DVSDT_D3DCOLOR, 1, 0);
    SetAttribute(pAttr, 4, D3DVSDT_FLOAT4, 2, 4);
    SetAttribute(pAttr, 5, D3DVSDT_D3DCOLOR, 2, 0);

    xb::DWORD dwDeclShadow[] = {D3DVSD_STREAM(0), D3DVSD_REG(0, D3DVSDT_FLOAT3), D3DVSD_STREAM(2),
                            D3DVSD_REG(5, D3DVSDT_D3DCOLOR), D3DVSD_END()};
    pAttr = m_Attr[8];
    ClearAttributes(pAttr);
    SetAttribute(pAttr, 0, D3DVSDT_FLOAT3, 0, 0);
    SetAttribute(pAttr, 5, D3DVSDT_D3DCOLOR, 2, 0);

    xb::DWORD dwDeclSkin[] = {D3DVSD_STREAM(0),
                          D3DVSD_REG(0, D3DVSDT_FLOAT3),
                          D3DVSD_REG(1, D3DVSDT_NORMPACKED3),
                          D3DVSD_REG(3, D3DVSDT_FLOAT2),
                          D3DVSD_STREAM(1),
                          D3DVSD_REG(2, D3DVSDT_D3DCOLOR),
                          D3DVSD_STREAM(2),
                          D3DVSD_REG(5, D3DVSDT_D3DCOLOR),
                          D3DVSD_STREAM(3),
                          D3DVSD_REG(6, D3DVSDT_FLOAT4),
                          D3DVSD_STREAM(4),
                          D3DVSD_REG(7, D3DVSDT_FLOAT4),
                          D3DVSD_STREAM(5),
                          D3DVSD_REG(8, D3DVSDT_FLOAT4),
                          D3DVSD_STREAM(6),
                          D3DVSD_REG(9, D3DVSDT_FLOAT4),
                          D3DVSD_STREAM(7),
                          D3DVSD_REG(10, D3DVSDT_FLOAT4),
                          D3DVSD_STREAM(8),
                          D3DVSD_REG(11, D3DVSDT_FLOAT4),
                          D3DVSD_STREAM(9),
                          D3DVSD_REG(12, D3DVSDT_FLOAT4),
                          D3DVSD_STREAM(10),
                          D3DVSD_REG(13, D3DVSDT_FLOAT4),
                          D3DVSD_END()};
    pAttr = m_Attr[2];
    ClearAttributes(pAttr);
    SetAttribute(pAttr, 0, D3DVSDT_FLOAT3, 0, 0);
    SetAttribute(pAttr, 1, D3DVSDT_NORMPACKED3, 0, 12);
    SetAttribute(pAttr, 3, D3DVSDT_FLOAT2, 0, 16);
    SetAttribute(pAttr, 2, D3DVSDT_D3DCOLOR, 1, 0);
    SetAttribute(pAttr, 5, D3DVSDT_D3DCOLOR, 2, 0);
    SetAttribute(pAttr, 4, D3DVSDT_FLOAT4, 2, 4);
    SetAttribute(pAttr, 6, D3DVSDT_NONE, 0, 0);
    SetAttribute(pAttr, 7, D3DVSDT_NONE, 0, 0);
    SetAttribute(pAttr, 8, D3DVSDT_NONE, 0, 0);
    SetAttribute(pAttr, 9, D3DVSDT_NONE, 0, 0);
    SetAttribute(pAttr, 10, D3DVSDT_NONE, 0, 0);
    SetAttribute(pAttr, 11, D3DVSDT_NONE, 0, 0);
    SetAttribute(pAttr, 12, D3DVSDT_NONE, 0, 0);
    SetAttribute(pAttr, 13, D3DVSDT_NONE, 0, 0);

    xb::DWORD dwDeclSprite[] = {D3DVSD_STREAM(0), D3DVSD_REG(0, D3DVSDT_FLOAT3), D3DVSD_REG(1, D3DVSDT_NORMPACKED3),
                            D3DVSD_REG(2, D3DVSDT_D3DCOLOR), D3DVSD_REG(3, D3DVSDT_FLOAT2), D3DVSD_END()};
    pAttr = m_Attr[3];
    ClearAttributes(pAttr);
    SetAttribute(pAttr, 0, D3DVSDT_FLOAT3, 0, 0);
    SetAttribute(pAttr, 1, D3DVSDT_NORMPACKED3, 0, 12);
    SetAttribute(pAttr, 5, D3DVSDT_D3DCOLOR, 0, 16);
    SetAttribute(pAttr, 3, D3DVSDT_FLOAT2, 0, 20);

    xb::DWORD dwDeclLine[] = {D3DVSD_STREAM(0), D3DVSD_REG(0, D3DVSDT_FLOAT3), D3DVSD_REG(2, D3DVSDT_D3DCOLOR),
                          D3DVSD_END()};
    pAttr = m_Attr[4];
    ClearAttributes(pAttr);
    SetAttribute(pAttr, 0, D3DVSDT_FLOAT3, 0, 0);
    SetAttribute(pAttr, 5, D3DVSDT_D3DCOLOR, 0, 12);

    xb::DWORD dwDeclSprite2D[] = {D3DVSD_STREAM(0), D3DVSD_REG(0, D3DVSDT_FLOAT3), D3DVSD_REG(2, D3DVSDT_D3DCOLOR),
                              D3DVSD_REG(3, D3DVSDT_FLOAT2), D3DVSD_END()};
    pAttr = m_Attr[6];
    ClearAttributes(pAttr);
    SetAttribute(pAttr, 0, D3DVSDT_FLOAT3, 0, 0);
    SetAttribute(pAttr, 5, D3DVSDT_D3DCOLOR, 0, 12);
    SetAttribute(pAttr, 3, D3DVSDT_FLOAT2, 0, 16);

    xb::DWORD dwDeclSwoosh[] = {D3DVSD_STREAM(0), D3DVSD_REG(0, D3DVSDT_FLOAT3), D3DVSD_REG(1, D3DVSDT_NORMPACKED3),
                            D3DVSD_REG(2, D3DVSDT_D3DCOLOR), D3DVSD_REG(3, D3DVSDT_FLOAT2), D3DVSD_END()};
    pAttr = m_Attr[7];
    ClearAttributes(pAttr);
    SetAttribute(pAttr, 0, D3DVSDT_FLOAT3, 0, 0);
    SetAttribute(pAttr, 1, D3DVSDT_NORMPACKED3, 0, 12);
    SetAttribute(pAttr, 5, D3DVSDT_D3DCOLOR, 0, 16);
    SetAttribute(pAttr, 3, D3DVSDT_FLOAT2, 0, 20);

    xb::DWORD dwDeclSprite2DTex3[] = {D3DVSD_STREAM(0),
                                  D3DVSD_REG(0, D3DVSDT_FLOAT3),
                                  D3DVSD_REG(2, D3DVSDT_D3DCOLOR),
                                  D3DVSD_REG(3, D3DVSDT_FLOAT2),
                                  D3DVSD_REG(4, D3DVSDT_FLOAT2),
                                  D3DVSD_REG(5, D3DVSDT_FLOAT2),
                                  D3DVSD_END()};
    xb::DWORD dwDeclSprite2Dw[] = {D3DVSD_STREAM(0), D3DVSD_REG(0, D3DVSDT_FLOAT4), D3DVSD_REG(2, D3DVSDT_D3DCOLOR),
                               D3DVSD_REG(3, D3DVSDT_FLOAT2), D3DVSD_END()};

#include "XBVertexShaderCreate.inc"

    float c8[4];
    c8[0] = 0.0f;
    c8[1] = 1.0f;
    c8[2] = -96.0f;
    c8[3] = 256.0f;
    D3DDevice_SetVertexShaderConstantFast(8, c8, 1);
    float c30[4];
    c30[0] = 0.5f;
    c30[1] = 0.25f;
    c30[2] = 2.0f;
    c30[3] = 12.0f;
    D3DDevice_SetVertexShaderConstantFast(30, c30, 1);
    float c74[4];
    c74[0] = c74[1] = c74[2] = c74[3] = 1.0f;
    D3DDevice_SetVertexShaderConstantFast(74, c74, 1);

    // The fixed shader table (SetVertexShader(n)); the shadow, heat and refraction programs are
    // loaded rather than selected.
    m_FixedHandles[3] = m_FixedShaders[0];
    m_FixedHandles[4] = m_FixedShaders[1];
    m_FixedHandles[5] = m_FixedShaders[2];
    m_FixedHandles[6] = m_FixedShaders[3];
    m_FixedHandles[7] = m_FixedShaders[4];
    m_FixedHandles[8] = m_FixedShaders[5];
    m_FixedHandles[9] = m_FixedShaders[6];
    m_FixedHandles[10] = m_FixedShaders[7];
    m_FixedHandles[16] = m_FixedShaders[8];
    m_FixedHandles[11] = m_FixedShaders[9];
    m_FixedHandles[12] = m_FixedShaders[10];
    m_FixedHandles[13] = m_FixedShaders[11];
    m_FixedHandles[15] = m_FixedShaders[12];
    m_FixedHandles[14] = m_FixedShaders[13];
    m_FixedHandles[18] = m_FixedShaders[14];
    m_FixedHandles[17] = m_FixedShaders[15];
    m_FixedHandles[19] = m_FixedShaders[16];
    m_FixedLoad[3] = 0;
    m_FixedLoad[4] = 0;
    m_FixedLoad[5] = 0;
    m_FixedLoad[6] = 0;
    m_FixedLoad[7] = 0;
    m_FixedLoad[11] = 0;
    m_FixedLoad[12] = 0;
    m_FixedLoad[8] = 1;
    m_FixedLoad[9] = 1;
    m_FixedLoad[10] = 1;
    m_FixedLoad[16] = 1;
    m_FixedLoad[13] = 1;
    m_FixedLoad[15] = 1;
    m_FixedLoad[14] = 1;
    m_FixedLoad[18] = 1;
    m_FixedLoad[17] = 1;
    m_FixedLoad[19] = 1;
    m_FixedCurrent = -1;
}

// Sphere map texture generation: the camera's normal matrix scaled into 0..1 texture space.
// Flag 0x40000 drops U, 0x80000 drops V.
void XBBaseVertexShader::SetEnvironmentMatrix(void *pA, void *pB, int flags)
{
    xb::D3DXMATRIX m;
    D3DXMatrixMultiply(&m, pA, pB);
    m._11 *= 0.5f;
    m._21 *= -0.5f;
    m._31 *= 0.5f;
    m._41 = 0.0f;
    m._12 *= 0.5f;
    m._22 *= -0.5f;
    m._32 *= 0.5f;
    m._42 = 0.0f;
    if (flags & 0x40000) {
        m._11 = 0.0f;
        m._21 = 0.0f;
        m._31 = 0.0f;
    }
    if (flags & 0x80000) {
        m._12 = 0.0f;
        m._22 = 0.0f;
        m._32 = 0.0f;
    }
    D3DXMatrixTranspose(&m, &m);
    D3DDevice_SetVertexShaderConstantFast(9, &m, 2);
}

void XBBaseVertexShader::SetEnvironmentIdentityMatrix()
{
    xb::D3DXMATRIX m;
    memset(&m, 0, sizeof(m));
    m._11 = 1.0f;
    m._22 = 1.0f;
    D3DDevice_SetVertexShaderConstantFast(9, &m, 2);
}

void XBBaseVertexShader::SetDirLight(int, const EXVector *, const EXVector *, xb::D3DXMATRIX *) {}

// Three directional lights: directions into view space (negated), colours with w = 1.
void XBBaseVertexShader::SetDirLights(EXVector *pDir, EXVector *pCol, xb::D3DXMATRIX *pMat)
{
    xb::D3DXMATRIX mt;
    D3DXMatrixTranspose(&mt, pMat);
    for (int i = 0; i < 3; i++) {
        xb::D3DXVECTOR3 d(pDir[i].x, pDir[i].y, pDir[i].z);
        xb::D3DXVECTOR3 c(pCol[i].x, pCol[i].y, pCol[i].z);
        xb::D3DXVECTOR3 t;
        D3DXVec3TransformNormal(&t, &d, &mt);
        xb::D3DXVECTOR4 dir(0.0f - t.x, 0.0f - t.y, 0.0f - t.z, 1.0f);
        xb::D3DXVECTOR4 col(c.x, c.y, c.z, 1.0f);
        if (i == 0) {
            D3DDevice_SetVertexShaderConstantFast(6, &col, 1);
            D3DDevice_SetVertexShaderConstantFast(4, &dir, 1);
        } else if (i == 1) {
            D3DDevice_SetVertexShaderConstantFast(7, &col, 1);
            D3DDevice_SetVertexShaderConstantFast(5, &dir, 1);
        } else if (i == 2) {
            D3DDevice_SetVertexShaderConstantFast(43, &col, 1);
            D3DDevice_SetVertexShaderConstantFast(42, &dir, 1);
        }
    }
}

void XBBaseVertexShader::SetPosLight(int, const EXVector *, const EXVector *, float, xb::D3DXMATRIX *) {}

// Up to four point lights: positions relative to the camera (pMat's translation) in view space,
// colours, and 1 / range of each in c17. Setting light 0 (or 2) also clears light 1 (or 3).
void XBBaseVertexShader::SetPosLights(int n, EXVector *pPos, EXVector *pCol, float *pRange, xb::D3DXMATRIX *pMat)
{
    xb::D3DXVECTOR4 zero(0.0f, 0.0f, 0.0f, 0.0f);
    xb::D3DXVECTOR4 invRange;
    invRange.x = invRange.y = invRange.z = invRange.w = 0.0f;
    xb::D3DXMATRIX mt;
    D3DXMatrixTranspose(&mt, pMat);
    for (int i = 0; i < n; i++) {
        (&invRange.x)[i] = 1.0f / pRange[i];
        xb::D3DXVECTOR3 v(pPos[i].x - pMat->_41, pPos[i].y - pMat->_42, pPos[i].z - pMat->_43);
        D3DXVec3TransformNormal(&v, &v, &mt);
        xb::D3DXVECTOR4 pos(v.x, v.y, v.z, 0.0f);
        xb::D3DXVECTOR4 col(pCol[i].x, pCol[i].y, pCol[i].z, 0.0f);
        if (i == 0) {
            D3DDevice_SetVertexShaderConstantFast(14, &pos, 1);
            D3DDevice_SetVertexShaderConstantFast(15, &col, 1);
            D3DDevice_SetVertexShaderConstantFast(18, &zero, 1);
            D3DDevice_SetVertexShaderConstantFast(19, &zero, 1);
        } else if (i == 1) {
            D3DDevice_SetVertexShaderConstantFast(18, &pos, 1);
            D3DDevice_SetVertexShaderConstantFast(19, &col, 1);
        } else if (i == 2) {
            D3DDevice_SetVertexShaderConstantFast(21, &pos, 1);
            D3DDevice_SetVertexShaderConstantFast(22, &col, 1);
            D3DDevice_SetVertexShaderConstantFast(32, &zero, 1);
            D3DDevice_SetVertexShaderConstantFast(33, &zero, 1);
        } else if (i == 3) {
            D3DDevice_SetVertexShaderConstantFast(32, &pos, 1);
            D3DDevice_SetVertexShaderConstantFast(33, &col, 1);
        }
    }
    D3DDevice_SetVertexShaderConstantFast(17, &invRange, 1);
}

void XBBaseVertexShader::SetPosDirLight(int, const EXVector *, const EXVector *, float, xb::D3DXMATRIX *, EXVector *) {}

// Up to four spot lights with fixed 15 / 30 degree cones: position (view space, w 0) in c55 / c58
// / c61 / c64, direction (w 1) in c56 / c59 / c62 / c65, colour in c57 / c60 / c63 / c66;
// c52 and c67 hold each light's cone cosines, c54 1 / (cos inner - cos outer), c53 1 / |direction|.
// Setting light 0 (or 2) also clears light 1 (or 3).
void XBBaseVertexShader::SetPosDirLights(int n, const EXVector *pPos, const EXVector *pCol, xb::D3DXMATRIX *pMat,
                                         EXVector *pDir)
{
    xb::D3DXVECTOR4 zero(0.0f, 0.0f, 0.0f, 0.0f);
    xb::D3DXMATRIX mt;
    D3DXMatrixTranspose(&mt, pMat);
    xb::D3DXVECTOR4 invCone, cones01, cones23, invLen;
    invCone.x = invCone.y = invCone.z = invCone.w = 0.0f;
    cones01.x = cones01.y = cones01.z = cones01.w = 0.0f;
    cones23.x = cones23.y = cones23.z = cones23.w = 0.0f;
    invLen.x = invLen.y = invLen.z = invLen.w = 0.0f;
    for (int i = 0; i < n; i++) {
        float cosIn = cos(0.2617994f);
        float cosOut = cos(0.5235988f);
        xb::D3DXVECTOR3 v(pPos[i].x - pMat->_41, pPos[i].y - pMat->_42, pPos[i].z - pMat->_43);
        D3DXVec3TransformNormal(&v, &v, &mt);
        xb::D3DXVECTOR3 d(pDir[i].x, pDir[i].y, pDir[i].z);
        xb::D3DXVECTOR3 t;
        D3DXVec3TransformNormal(&t, &d, &mt);
        xb::D3DXVECTOR4 dir(t.x, t.y, t.z, 1.0f);
        xb::D3DXVECTOR4 pos(v.x, v.y, v.z, 0.0f);
        xb::D3DXVECTOR4 col(pCol[i].x, pCol[i].y, pCol[i].z, 0.0f);
        if (i == 0) {
            cones01.x = cosIn;
            cones01.y = cosOut;
        } else if (i == 1) {
            cones01.z = cosIn;
            cones01.w = cosOut;
        } else if (i == 2) {
            cones23.x = cosIn;
            cones23.y = cosOut;
        } else if (i == 3) {
            cones23.z = cosIn;
            cones23.w = cosOut;
        }
        (&invCone.x)[i] = 1.0f / (cosIn - cosOut);
        if (i == 0) {
            D3DDevice_SetVertexShaderConstantFast(55, &pos, 1);
            D3DDevice_SetVertexShaderConstantFast(57, &col, 1);
            D3DDevice_SetVertexShaderConstantFast(56, &dir, 1);
            invLen.x = 1.0f / sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
            D3DDevice_SetVertexShaderConstantFast(58, &zero, 1);
            D3DDevice_SetVertexShaderConstantFast(60, &zero, 1);
            D3DDevice_SetVertexShaderConstantFast(59, &zero, 1);
        } else if (i == 1) {
            D3DDevice_SetVertexShaderConstantFast(58, &pos, 1);
            D3DDevice_SetVertexShaderConstantFast(60, &col, 1);
            D3DDevice_SetVertexShaderConstantFast(59, &dir, 1);
            invLen.y = 1.0f / sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
        } else if (i == 2) {
            D3DDevice_SetVertexShaderConstantFast(61, &pos, 1);
            D3DDevice_SetVertexShaderConstantFast(63, &col, 1);
            D3DDevice_SetVertexShaderConstantFast(62, &dir, 1);
            invLen.z = 1.0f / sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
            D3DDevice_SetVertexShaderConstantFast(64, &zero, 1);
            D3DDevice_SetVertexShaderConstantFast(66, &zero, 1);
            D3DDevice_SetVertexShaderConstantFast(65, &zero, 1);
        } else if (i == 3) {
            D3DDevice_SetVertexShaderConstantFast(64, &pos, 1);
            D3DDevice_SetVertexShaderConstantFast(66, &col, 1);
            D3DDevice_SetVertexShaderConstantFast(65, &dir, 1);
            invLen.w = 1.0f / sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
        }
    }
    D3DDevice_SetVertexShaderConstantFast(54, &invCone, 1);
    D3DDevice_SetVertexShaderConstantFast(53, &invLen, 1);
    D3DDevice_SetVertexShaderConstantFast(52, &cones01, 1);
    D3DDevice_SetVertexShaderConstantFast(67, &cones23, 1);
}

void XBBaseVertexShader::ClearAllStreams()
{
    for (int i = 0; i < 16; i++) {
        m_Streams[i].m_pVB = 0;
        m_Streams[i].m_Stride = 0;
        m_Streams[i].m_Index = 0;
        m_Streams[i].m_On = 0;
    }
}

void XBBaseVertexShader::SetVertexStream(int stream, void *pVB, int stride, int)
{
    if (pVB == (void *)0x1a || pVB == (void *)0x9a) __debugbreak();
    m_Streams[stream].m_Stride = stride;
    m_Streams[stream].m_Index = stream;
    m_Streams[stream].m_pVB = pVB;
    m_Streams[stream].m_pData = pVB;
    m_Streams[stream].m_On = pVB != 0;
}

void XBBaseVertexShader::TurnOnStream(int format, int shader, int attr, int stream, int offset, int tessSource,
                                      int tessType)
{
    XBVertexShader.m_Attr[shader][attr].m_Format = format;
    XBVertexShader.m_Attr[shader][attr].m_StreamIndex = stream;
    XBVertexShader.m_Attr[shader][attr].m_Offset = offset;
    XBVertexShader.m_Attr[shader][attr].m_TessellationSource = (u8)tessSource;
    XBVertexShader.m_Attr[shader][attr].m_TessellationType = (u8)tessType;
}

void XBBaseVertexShader::TurnOffStream(int shader, int attr, int stream)
{
    XBVertexShader.m_Attr[shader][attr].m_Format = 2;
    XBVertexShader.m_Attr[shader][attr].m_StreamIndex = 0;
    m_Streams[stream].m_pVB = 0;
    m_Streams[stream].m_On = 0;
}

struct XBStreamInput {      // D3DSTREAM_INPUT
    void *m_pVB;
    unsigned long m_Stride;
    unsigned long m_Offset;
};

// Binds the bound streams to the fixed shader's attribute layout (layout 0 with two streams or
// fewer).
void XBBaseVertexShader::RewireVertexShaderInputs(int shader)
{
    XBStreamInput in[16];
    int n = 0;
    for (int i = 0; i < 16; i++) {
        if (m_Streams[i].m_pVB) {
            in[i].m_Offset = 0;
            in[i].m_Stride = m_Streams[i].m_Stride;
            in[i].m_pVB = m_Streams[i].m_pVB;
            n++;
        } else {
            in[i].m_pVB = 0;
        }
    }
    if (n <= 2) shader = 0;
    D3DDevice_SetVertexShaderInputDirect((xb::_D3DVERTEXATTRIBUTEFORMAT *)XBVertexShader.m_Attr[shader], n,
                                         (const xb::_D3DSTREAM_INPUT *)in);
}

// Linear fog between start and end (both 0..1 of the view distance), written to c13 as the
// scale and offset the shaders apply to view depth.
void XBBaseVertexShader::SetFog(signed char on, ex::RGBA col, float start, float end, float zNear, float zFar)
{
    XBRenderState.SetRenderState(xb::D3DRS_FOGENABLE, on);
    if (!on) return;
    XBRenderState.SetRenderState(xb::D3DRS_FOGCOLOR, col.argb());
    float c[4];
    c[2] = 0.0f;
    c[3] = 0.0f;
    float s = 1.0f - limit(start, 0.0f, 1.0f);
    float e = 1.0f - limit(end, 0.0f, 1.0f);
    c[0] = (e - s) * zNear * zFar / (zFar - zNear) * m_FogScale;
    c[1] = ((s - e) * (zNear + zFar) / (zFar - zNear) + (e + s)) * 0.5f;
    D3DDevice_SetVertexShaderConstantFast(13, c, 1);
}
