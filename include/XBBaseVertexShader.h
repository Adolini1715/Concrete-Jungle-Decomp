#ifndef XBBASEVERTEXSHADER_H
#define XBBASEVERTEXSHADER_H
#include "types.h"
#include "xbd3d8.h"

extern "C" long __stdcall D3DDevice_LoadVertexShader(unsigned long handle, unsigned long address);
extern "C" long __stdcall D3DDevice_SetVertexShader(unsigned long handle);
extern "C" void __fastcall D3DDevice_SetVertexShaderConstant1Fast(int Register, const void *pConstantData);
namespace xb { struct D3DXMATRIX; }
namespace ex { class RGBA; }
class EXVector;

// Xbox vertex shader manager (XBVertexShader.obj, global XBVertexShader, sizeof 0x1e18). Two shader
// sets: 512 bit-combination shaders indexed by m_Bits (handles at +0, current at +0x804) and 20
// fixed shaders (handles at +0x1c50, current at +0x1c4c, "load instead of set" flags at +0x1ca0),
// each with its own vertex attribute layout (+0x84c). Stream bindings for RewireVertexShaderInputs
// at +0x1cb4.
struct XBVertexAttributeFormat {        // D3DVERTEXATTRIBUTEFORMAT (0x10)
    unsigned long m_StreamIndex;
    unsigned long m_Offset;
    unsigned long m_Format;             // 2: none
    u8 m_TessellationType;
    u8 m_TessellationSource;
    u8 m_pad[2];
};
struct XBVertexStream {                 // 0x14
    void *m_pVB;
    void *m_pData;
    int m_Stride;
    int m_Index;
    s8 m_On;
};
class XBBaseVertexShader {
public:
    // Emitted in XBDisplay.obj (inlined in XBGeoEntity.obj with constant n).
    __forceinline void SetVertexShader(int n)
    {
        if (m_FixedCurrent != n && n < 20) {
            if (m_FixedLoad[n] == 1)
                xb::D3DDevice::LoadVertexShader(m_FixedHandles[n], 0);
            else
                xb::D3DDevice::SetVertexShader(m_FixedHandles[n]);
            m_FixedCurrent = n;
        }
        m_Current = -1;
    }
    // Bit-combination shader: m_Bits picks the handle (emitted in XBGeoEntity.obj).
    void SetVertexShader()
    {
        if (m_Bits != m_Current) {
            D3DDevice_LoadVertexShader(m_Handles[m_Bits], 0);
            m_Current = m_Bits;
        }
        m_FixedCurrent = -1;
    }
    void ClearBitField() { m_Bits = 0; }
    void SetBit(unsigned long b) { m_Bits |= b; }
    void ClearBit(unsigned long b) { m_Bits &= ~b; }
    void SetGlobalDiffuse(float *p) { D3DDevice_SetVertexShaderConstant1Fast(11 + 96, p); }
    void SetFullGlobalDiffuse() { D3DDevice_SetVertexShaderConstant1Fast(11 + 96, m_GlobalDiffuse); }  // [fi] XBFont.obj
    void SetGlobalAmbient(float *p) { D3DDevice_SetVertexShaderConstant1Fast(12 + 96, p); }
    void SetScrollUVs(float *p) { D3DDevice_SetVertexShaderConstant1Fast(29 + 96, p); }   // c29 (u, v scroll)
    XBBaseVertexShader();
    void DeclareAllVertexShaders();
    void SetEnvironmentMatrix(void *a, void *b, int c);
    void SetEnvironmentIdentityMatrix();
    void SetDirLights(EXVector *pDir, EXVector *pCol, xb::D3DXMATRIX *pMat);
    void SetPosLights(int n, EXVector *pPos, EXVector *pCol, float *pRange, xb::D3DXMATRIX *pMat);
    void SetVertexStream(int stream, void *pVB, int stride, int a);
    void TurnOnStream(int a, int b, int c, int d, int e, int f, int g);
    void TurnOffStream(int a, int b, int c);
    void RewireVertexShaderInputs(int n);
    void SetFog(signed char on, ex::RGBA col, float a, float b, float c, float d);
    void SetDirLight(int i, const EXVector *pDir, const EXVector *pCol, xb::D3DXMATRIX *pMat);
    void SetPosLight(int i, const EXVector *pPos, const EXVector *pCol, float range, xb::D3DXMATRIX *pMat);
    void SetPosDirLight(int i, const EXVector *pPos, const EXVector *pDir, float range, xb::D3DXMATRIX *pMat,
                        EXVector *pCol);
    void SetPosDirLights(int n, const EXVector *pPos, const EXVector *pCol, xb::D3DXMATRIX *pMat, EXVector *pDir);
    void ClearAllStreams();
    unsigned long m_Handles[0x200];     // 0x000 indexed by m_Bits
    u32 m_Bits;                         // 0x800
    int m_Current;                      // 0x804
    unsigned long m_FixedShaders[17];   // 0x808 handles as created (sprite, line3d, line2d, ...)
    XBVertexAttributeFormat m_Attr[20][16];     // 0x84c input layout of each fixed shader
    int m_FixedCurrent;                 // 0x1c4c
    unsigned long m_FixedHandles[20];   // 0x1c50
    u8 m_FixedLoad[20];                 // 0x1ca0
    XBVertexStream m_Streams[16];       // 0x1cb4
    u32 m_1df4;
    float m_GlobalDiffuse[4];           // 0x1df8 vertex constant c11 (screen-space quads put it back after use)
    float m_FogScale;                   // 0x1e08 scales the fog constant (SetFog)
    float m_ScreenZ;                    // 0x1e0c depth of screen-space quads (100 is added); 0.001
    float m_1e10;                       // 0x1e10 1000
    u32 m_1e14;
};
extern XBBaseVertexShader XBVertexShader;
#endif
