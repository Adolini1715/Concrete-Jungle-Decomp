#ifndef XBD3D8_H
#define XBD3D8_H
#include "types.h"

// The part of the Xbox XDK's d3d8.h the engine's XB* layer uses, recovered
// from the code. The engine compiled the XDK headers inside `namespace xb`
// (the map's mangled names say so); the flat C API (D3DDevice_*, D3DTexture_*)
// is extern "C".
//
// XDK inlines are __forceinline: every call is inlined and each object still
// emits a copy (the map's [fi] / [fs] entries). The C-API inlines are static,
// so their emitted copies get VC 7.1's custom calling convention (arguments in
// esi/edi, no stack cleanup), as seen in every XB*.obj.

namespace xb {

typedef unsigned long DWORD;
typedef unsigned int UINT;
typedef int INT;
typedef long HRESULT;

enum _D3DFORMAT {
    D3DFMT_A1R5G5B5 = 0x02,         // swizzled
    D3DFMT_X1R5G5B5 = 0x03,
    D3DFMT_A4R4G4B4 = 0x04,
    D3DFMT_R5G6B5 = 0x05,
    D3DFMT_A8R8G8B8 = 0x06,
    D3DFMT_X8R8G8B8 = 0x07,
    D3DFMT_P8 = 0x0b,
    D3DFMT_DXT1 = 0x0c,
    D3DFMT_DXT2 = 0x0e,
    D3DFMT_LIN_A8R8G8B8 = 0x12,
    D3DFMT_LIN_X8R8G8B8 = 0x1e,
    D3DFMT_FORCE_DWORD = 0x7fffffff
};
typedef _D3DFORMAT D3DFORMAT;

enum _D3DRESOURCETYPE { D3DRTYPE_FORCE_DWORD = 0x7fffffff };
enum _D3DPRIMITIVETYPE {
    D3DPT_POINTLIST = 1,
    D3DPT_LINELIST = 2,
    D3DPT_LINELOOP = 3,
    D3DPT_LINESTRIP = 4,
    D3DPT_TRIANGLELIST = 5,
    D3DPT_TRIANGLESTRIP = 6,
    D3DPT_TRIANGLEFAN = 7,
    D3DPT_QUADLIST = 8,
    D3DPT_QUADSTRIP = 9,
    D3DPT_POLYGON = 10,
    D3DPT_FORCE_DWORD = 0x7fffffff
};
typedef _D3DPRIMITIVETYPE D3DPRIMITIVETYPE;
enum _D3DDEVTYPE { D3DDEVTYPE_HAL = 1, D3DDEVTYPE_REF = 2, D3DDEVTYPE_SW = 3, D3DDEVTYPE_FORCE_DWORD = 0x7fffffff };
typedef _D3DDEVTYPE D3DDEVTYPE;
struct _D3DVERTEXATTRIBUTEFORMAT;
struct _D3DSTREAM_INPUT;
enum _D3DMULTISAMPLE_TYPE { D3DMULTISAMPLE_FORCE_DWORD = 0x7fffffff };

enum _D3DRENDERSTATETYPE {
    D3DRS_PSALPHAINPUTS0 = 0x00,
    D3DRS_PSALPHAINPUTS1 = 0x01,
    D3DRS_PSALPHAINPUTS2 = 0x02,
    D3DRS_PSALPHAINPUTS3 = 0x03,
    D3DRS_PSALPHAINPUTS4 = 0x04,
    D3DRS_PSALPHAINPUTS5 = 0x05,
    D3DRS_PSALPHAINPUTS6 = 0x06,
    D3DRS_PSALPHAINPUTS7 = 0x07,
    D3DRS_PSFINALCOMBINERINPUTSABCD = 0x08,
    D3DRS_PSFINALCOMBINERINPUTSEFG = 0x09,
    D3DRS_PSCONSTANT0_0 = 0x0a,
    D3DRS_PSCONSTANT0_1 = 0x0b,
    D3DRS_PSCONSTANT0_2 = 0x0c,
    D3DRS_PSCONSTANT0_3 = 0x0d,
    D3DRS_PSCONSTANT0_4 = 0x0e,
    D3DRS_PSCONSTANT0_5 = 0x0f,
    D3DRS_PSCONSTANT0_6 = 0x10,
    D3DRS_PSCONSTANT0_7 = 0x11,
    D3DRS_PSCONSTANT1_0 = 0x12,
    D3DRS_PSCONSTANT1_1 = 0x13,
    D3DRS_PSCONSTANT1_2 = 0x14,
    D3DRS_PSCONSTANT1_3 = 0x15,
    D3DRS_PSCONSTANT1_4 = 0x16,
    D3DRS_PSCONSTANT1_5 = 0x17,
    D3DRS_PSCONSTANT1_6 = 0x18,
    D3DRS_PSCONSTANT1_7 = 0x19,
    D3DRS_PSALPHAOUTPUTS0 = 0x1a,
    D3DRS_PSALPHAOUTPUTS1 = 0x1b,
    D3DRS_PSALPHAOUTPUTS2 = 0x1c,
    D3DRS_PSALPHAOUTPUTS3 = 0x1d,
    D3DRS_PSALPHAOUTPUTS4 = 0x1e,
    D3DRS_PSALPHAOUTPUTS5 = 0x1f,
    D3DRS_PSALPHAOUTPUTS6 = 0x20,
    D3DRS_PSALPHAOUTPUTS7 = 0x21,
    D3DRS_PSRGBINPUTS0 = 0x22,
    D3DRS_PSRGBINPUTS1 = 0x23,
    D3DRS_PSRGBINPUTS2 = 0x24,
    D3DRS_PSRGBINPUTS3 = 0x25,
    D3DRS_PSRGBINPUTS4 = 0x26,
    D3DRS_PSRGBINPUTS5 = 0x27,
    D3DRS_PSRGBINPUTS6 = 0x28,
    D3DRS_PSRGBINPUTS7 = 0x29,
    D3DRS_PSCOMPAREMODE = 0x2a,
    D3DRS_PSFINALCOMBINERCONSTANT0 = 0x2b,
    D3DRS_PSFINALCOMBINERCONSTANT1 = 0x2c,
    D3DRS_PSRGBOUTPUTS0 = 0x2d,
    D3DRS_PSRGBOUTPUTS1 = 0x2e,
    D3DRS_PSRGBOUTPUTS2 = 0x2f,
    D3DRS_PSRGBOUTPUTS3 = 0x30,
    D3DRS_PSRGBOUTPUTS4 = 0x31,
    D3DRS_PSRGBOUTPUTS5 = 0x32,
    D3DRS_PSRGBOUTPUTS6 = 0x33,
    D3DRS_PSRGBOUTPUTS7 = 0x34,
    D3DRS_PSCOMBINERCOUNT = 0x35,
    D3DRS_PS_RESERVED = 0x36,
    D3DRS_PSDOTMAPPING = 0x37,
    D3DRS_PSINPUTTEXTURE = 0x38,
    D3DRS_ZFUNC = 0x39,
    D3DRS_ALPHAFUNC = 0x3a,
    D3DRS_ALPHABLENDENABLE = 0x3b,
    D3DRS_ALPHATESTENABLE = 0x3c,
    D3DRS_ALPHAREF = 0x3d,
    D3DRS_SRCBLEND = 0x3e,
    D3DRS_DESTBLEND = 0x3f,
    D3DRS_ZWRITEENABLE = 0x40,
    D3DRS_DITHERENABLE = 0x41,
    D3DRS_SHADEMODE = 0x42,
    D3DRS_COLORWRITEENABLE = 0x43,
    D3DRS_STENCILZFAIL = 0x44,
    D3DRS_STENCILPASS = 0x45,
    D3DRS_STENCILFUNC = 0x46,
    D3DRS_STENCILREF = 0x47,
    D3DRS_STENCILMASK = 0x48,
    D3DRS_STENCILWRITEMASK = 0x49,
    D3DRS_BLENDOP = 0x4a,
    D3DRS_BLENDCOLOR = 0x4b,
    D3DRS_SWATHWIDTH = 0x4c,
    D3DRS_POLYGONOFFSETZSLOPESCALE = 0x4d,
    D3DRS_POLYGONOFFSETZOFFSET = 0x4e,
    D3DRS_POINTOFFSETENABLE = 0x4f,
    D3DRS_WIREFRAMEOFFSETENABLE = 0x50,
    D3DRS_SOLIDOFFSETENABLE = 0x51,
    D3DRS_DEPTHCLIPCONTROL = 0x52,
    D3DRS_STIPPLEENABLE = 0x53,
    D3DRS_SIMPLE_MAX = 0x5c,                        // below: D3DDevice_SetRenderState_Simple
    D3DRS_FOGENABLE = 0x5c,
    D3DRS_FOGTABLEMODE = 0x5d,
    D3DRS_FOGSTART = 0x5e,
    D3DRS_FOGEND = 0x5f,
    D3DRS_FOGDENSITY = 0x60,
    D3DRS_RANGEFOGENABLE = 0x61,
    D3DRS_WRAP0 = 0x62,
    D3DRS_WRAP1 = 0x63,
    D3DRS_WRAP2 = 0x64,
    D3DRS_WRAP3 = 0x65,
    D3DRS_LIGHTING = 0x66,
    D3DRS_SPECULARENABLE = 0x67,
    D3DRS_LOCALVIEWER = 0x68,
    D3DRS_COLORVERTEX = 0x69,
    D3DRS_BACKSPECULARMATERIALSOURCE = 0x6a,
    D3DRS_BACKDIFFUSEMATERIALSOURCE = 0x6b,
    D3DRS_BACKAMBIENTMATERIALSOURCE = 0x6c,
    D3DRS_BACKEMISSIVEMATERIALSOURCE = 0x6d,
    D3DRS_SPECULARMATERIALSOURCE = 0x6e,
    D3DRS_DIFFUSEMATERIALSOURCE = 0x6f,
    D3DRS_AMBIENTMATERIALSOURCE = 0x70,
    D3DRS_EMISSIVEMATERIALSOURCE = 0x71,
    D3DRS_BACKAMBIENT = 0x72,
    D3DRS_AMBIENT = 0x73,
    D3DRS_POINTSIZE = 0x74,
    D3DRS_POINTSIZE_MIN = 0x75,
    D3DRS_POINTSPRITEENABLE = 0x76,
    D3DRS_POINTSCALEENABLE = 0x77,
    D3DRS_POINTSCALE_A = 0x78,
    D3DRS_POINTSCALE_B = 0x79,
    D3DRS_POINTSCALE_C = 0x7a,
    D3DRS_POINTSIZE_MAX = 0x7b,
    D3DRS_PATCHEDGESTYLE = 0x7c,
    D3DRS_PATCHSEGMENTS = 0x7d,
    D3DRS_SWAPFILTER = 0x7e,
    D3DRS_PRESENTATIONINTERVAL = 0x7f,
    D3DRS_DEFERRED_MAX = 0x88,                      // below: deferred, dirty flags
    D3DRS_PSTEXTUREMODES = 0x88,
    D3DRS_VERTEXBLEND = 0x89,
    D3DRS_FOGCOLOR = 0x8a,
    D3DRS_FILLMODE = 0x8b,
    D3DRS_BACKFILLMODE = 0x8c,
    D3DRS_TWOSIDEDLIGHTING = 0x8d,
    D3DRS_NORMALIZENORMALS = 0x8e,
    D3DRS_ZENABLE = 0x8f,
    D3DRS_STENCILENABLE = 0x90,
    D3DRS_STENCILFAIL = 0x91,
    D3DRS_FRONTFACE = 0x92,
    D3DRS_CULLMODE = 0x93,
    D3DRS_TEXTUREFACTOR = 0x94,
    D3DRS_ZBIAS = 0x95,
    D3DRS_LOGICOP = 0x96,
    D3DRS_EDGEANTIALIAS = 0x97,
    D3DRS_MULTISAMPLEANTIALIAS = 0x98,
    D3DRS_MULTISAMPLEMASK = 0x99,
    D3DRS_MULTISAMPLEMODE = 0x9a,
    D3DRS_MULTISAMPLERENDERTARGETMODE = 0x9b,
    D3DRS_SHADOWFUNC = 0x9c,
    D3DRS_LINEWIDTH = 0x9d,
    D3DRS_SAMPLEALPHA = 0x9e,
    D3DRS_DXT1NOISEENABLE = 0x9f,
    D3DRS_YUVENABLE = 0xa0,
    D3DRS_OCCLUSIONCULLENABLE = 0xa1,
    D3DRS_STENCILCULLENABLE = 0xa2,
    D3DRS_ROPZCMPALWAYSREAD = 0xa3,
    D3DRS_ROPZREAD = 0xa4,
    D3DRS_DONOTCULLUNCOMPRESSED = 0xa5,
    D3DRS_FORCE_DWORD = 0x7fffffff
};
typedef _D3DRENDERSTATETYPE D3DRENDERSTATETYPE;

enum _D3DTEXTURESTAGESTATETYPE {
    D3DTSS_ADDRESSU = 0x00,
    D3DTSS_ADDRESSV = 0x01,
    D3DTSS_ADDRESSW = 0x02,
    D3DTSS_MAGFILTER = 0x03,
    D3DTSS_MINFILTER = 0x04,
    D3DTSS_MIPFILTER = 0x05,
    D3DTSS_MIPMAPLODBIAS = 0x06,
    D3DTSS_MAXMIPLEVEL = 0x07,
    D3DTSS_MAXANISOTROPY = 0x08,
    D3DTSS_COLORKEYOP = 0x09,
    D3DTSS_COLORSIGN = 0x0a,
    D3DTSS_ALPHAKILL = 0x0b,
    D3DTSS_DEFERRED_TEXTURE_STATE_MAX = 0x0c,
    D3DTSS_COLOROP = 0x0c,
    D3DTSS_COLORARG0 = 0x0d,
    D3DTSS_COLORARG1 = 0x0e,
    D3DTSS_COLORARG2 = 0x0f,
    D3DTSS_ALPHAOP = 0x10,
    D3DTSS_ALPHAARG0 = 0x11,
    D3DTSS_ALPHAARG1 = 0x12,
    D3DTSS_ALPHAARG2 = 0x13,
    D3DTSS_RESULTARG = 0x14,
    D3DTSS_TEXTURETRANSFORMFLAGS = 0x15,
    D3DTSS_DEFERRED_MAX = 0x16,
    D3DTSS_BUMPENVMAT00 = 0x16,
    D3DTSS_BUMPENVMAT01 = 0x17,
    D3DTSS_BUMPENVMAT11 = 0x18,
    D3DTSS_BUMPENVMAT10 = 0x19,
    D3DTSS_BUMPENVLSCALE = 0x1a,
    D3DTSS_BUMPENVLOFFSET = 0x1b,
    D3DTSS_TEXCOORDINDEX = 0x1c,
    D3DTSS_BORDERCOLOR = 0x1d,
    D3DTSS_COLORKEYCOLOR = 0x1e,
    D3DTSS_FORCE_DWORD = 0x7fffffff
};
typedef _D3DTEXTURESTAGESTATETYPE D3DTEXTURESTAGESTATETYPE;

struct _D3DSURFACE_DESC {
    D3DFORMAT Format;
    _D3DRESOURCETYPE Type;
    DWORD Usage;
    UINT Size;
    _D3DMULTISAMPLE_TYPE MultiSampleType;
    UINT Width;
    UINT Height;
};
typedef _D3DSURFACE_DESC D3DSURFACE_DESC;

struct _D3DLOCKED_RECT {
    INT Pitch;
    void *pBits;
};
typedef _D3DLOCKED_RECT D3DLOCKED_RECT;

struct tagRECT { long left, top, right, bottom; };

struct D3DXVECTOR3 {
    D3DXVECTOR3() {}
    D3DXVECTOR3(float fx, float fy, float fz) { x = fx; y = fy; z = fz; }
    D3DXVECTOR3 operator-(const D3DXVECTOR3 &v) const { return D3DXVECTOR3(x - v.x, y - v.y, z - v.z); }
    D3DXVECTOR3 operator+(const D3DXVECTOR3 &v) const { return D3DXVECTOR3(x + v.x, y + v.y, z + v.z); }
    friend D3DXVECTOR3 operator*(float f, const D3DXVECTOR3 &v);
    float x, y, z;
};
inline D3DXVECTOR3 operator*(float f, const D3DXVECTOR3 &v) { return D3DXVECTOR3(f * v.x, f * v.y, f * v.z); }
struct D3DXVECTOR4 {
    D3DXVECTOR4() {}
    __forceinline D3DXVECTOR4(float fx, float fy, float fz, float fw) { x = fx; y = fy; z = fz; w = fw; }
    float x, y, z, w;
};
struct _D3DMATRIX {
    float m[4][4];
};
struct D3DXMATRIX {
    D3DXMATRIX() {}
    D3DXMATRIX(const _D3DMATRIX &mat) { *(_D3DMATRIX *)this = mat; }
    union {
        struct {
            float _11, _12, _13, _14;
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
            float _41, _42, _43, _44;
        };
        float m[4][4];
    };
};
inline D3DXMATRIX *D3DXMatrixIdentity(D3DXMATRIX *pOut)
{
    pOut->m[0][1] = pOut->m[0][2] = pOut->m[0][3] =
    pOut->m[1][0] = pOut->m[1][2] = pOut->m[1][3] =
    pOut->m[2][0] = pOut->m[2][1] = pOut->m[2][3] =
    pOut->m[3][0] = pOut->m[3][1] = pOut->m[3][2] = 0.0f;
    pOut->m[0][0] = pOut->m[1][1] = pOut->m[2][2] = pOut->m[3][3] = 1.0f;
    return pOut;
}

struct D3DResource {
    u32 m_Common;   // 0x00 refcount | type bits (vertex buffer 1, index buffer 0x10001)
    u32 m_Data;     // 0x04 physical address
    u32 m_Lock;     // 0x08
    DWORD __stdcall Release();
    void __stdcall BlockUntilNotBusy();
    void __stdcall Register(void *pBase);
};
struct D3DVertexBuffer : public D3DResource {};
struct D3DIndexBuffer : public D3DResource {};
struct D3DPalette : public D3DResource {};
struct D3DPixelContainer : public D3DResource {
    DWORD Format;   // 0x0c
    DWORD Size;     // 0x10
};
struct D3DBaseTexture : public D3DPixelContainer {};
struct D3DTexture;
struct D3DSurface : public D3DPixelContainer {
    HRESULT __stdcall GetDesc(D3DSURFACE_DESC *pDesc);
    D3DBaseTexture *Parent;     // 0x14
};
struct D3DDevice;
struct _D3DPixelShaderDef;

}   // namespace xb

extern "C" {
extern xb::DWORD D3D__DirtyFlags;
extern xb::DWORD D3D__RenderState[];
// The XDK header defines its lookup tables (selectany: the map gives them to Actor.obj, the first
// object linked), so the inlines fold them to constants. Values read from the XBE.
__declspec(selectany) extern const xb::DWORD D3DDIRTYFROMTEXTURESTATE[22] = {
    0x0000000f, 0x0000000f, 0x0000000f, 0x0000000f, 0x0000000f, 0x0000000f, 0x0000000f, 0x0000000f,
    0x0000000f, 0x0000000f, 0x0000000f, 0x0000000f, 0x00000800, 0x00000800, 0x00000800, 0x00000800,
    0x00000800, 0x00000800, 0x00000800, 0x00000800, 0x00000800, 0x00000400,
};
__declspec(selectany) extern const xb::DWORD D3DDIRTYFROMRENDERSTATE[48] = {
    0x00002000, 0x00002000, 0x00002000, 0x00002000, 0x00002000, 0x00002000, 0x0000000f, 0x0000000f,
    0x0000000f, 0x0000000f, 0x00001200, 0x00003000, 0x00001000, 0x00001000, 0x00001000, 0x00001000,
    0x00001000, 0x00001000, 0x00001000, 0x00001000, 0x00001000, 0x00001000, 0x00001000, 0x00001000,
    0x00000100, 0x00000100, 0x00000900, 0x00000100, 0x00000100, 0x00000100, 0x00000100, 0x00000100,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00081b00, 0x00081b40, 0x00081b80, 0x00081bc0,
};
__declspec(selectany) extern const xb::DWORD D3DSIMPLERENDERSTATEENCODE[92] = {
    0x00040260, 0x00040264, 0x00040268, 0x0004026c, 0x00040270, 0x00040274, 0x00040278, 0x0004027c,
    0x00040288, 0x0004028c, 0x00040a60, 0x00040a64, 0x00040a68, 0x00040a6c, 0x00040a70, 0x00040a74,
    0x00040a78, 0x00040a7c, 0x00040a80, 0x00040a84, 0x00040a88, 0x00040a8c, 0x00040a90, 0x00040a94,
    0x00040a98, 0x00040a9c, 0x00040aa0, 0x00040aa4, 0x00040aa8, 0x00040aac, 0x00040ab0, 0x00040ab4,
    0x00040ab8, 0x00040abc, 0x00040ac0, 0x00040ac4, 0x00040ac8, 0x00040acc, 0x00040ad0, 0x00040ad4,
    0x00040ad8, 0x00040adc, 0x000417f8, 0x00041e20, 0x00041e24, 0x00041e40, 0x00041e44, 0x00041e48,
    0x00041e4c, 0x00041e50, 0x00041e54, 0x00041e58, 0x00041e5c, 0x00041e60, 0x00041d90, 0x00041e74,
    0x00041e78, 0x00040354, 0x0004033c, 0x00040304, 0x00040300, 0x00040340, 0x00040344, 0x00040348,
    0x0004035c, 0x00040310, 0x0004037c, 0x00040358, 0x00040374, 0x00040378, 0x00040364, 0x00040368,
    0x0004036c, 0x00040360, 0x00040350, 0x0004034c, 0x000409f8, 0x00040384, 0x00040388, 0x00040330,
    0x00040334, 0x00040338, 0x00041d78, 0x0004147c, 0x00041d90, 0x00041d90, 0x00041d90, 0x00041d90,
    0x00041d90, 0x00041d90, 0x00041d90, 0x00041d90,
};

// Vertex count of n primitives: [type][0] * n + [type][1].
__declspec(selectany) extern const xb::UINT D3DPRIMITIVETOVERTEXCOUNT[11][2] = {
    {0, 0}, {1, 0}, {2, 0}, {1, 1}, {1, 1}, {3, 0}, {1, 2}, {1, 2}, {4, 0}, {2, 2}, {0, 0},
};
extern unsigned short *D3D__IndexData;
void __stdcall D3DDevice_DrawIndexedVertices(xb::D3DPRIMITIVETYPE PrimitiveType, xb::UINT VertexCount,
                                             const unsigned short *pIndexData);
xb::HRESULT __stdcall D3DDevice_SetVertexShaderInputDirect(xb::_D3DVERTEXATTRIBUTEFORMAT *pVAF, xb::UINT StreamCount,
                                                           const xb::_D3DSTREAM_INPUT *pStreamInputs);
xb::DWORD __stdcall D3DResource_Release(xb::D3DResource *pThis);
xb::D3DSurface *__stdcall D3DDevice_GetBackBuffer2(xb::INT BackBuffer);
xb::D3DVertexBuffer *__stdcall D3DDevice_CreateVertexBuffer2(xb::UINT Length);
xb::D3DSurface *__stdcall D3DDevice_GetRenderTarget2();
xb::D3DSurface *__stdcall D3DDevice_GetDepthStencilSurface2();
xb::D3DSurface *__stdcall D3DTexture_GetSurfaceLevel2(xb::D3DTexture *pThis, xb::UINT Level);
void __stdcall D3DTexture_GetLevelDesc(xb::D3DTexture *pThis, xb::UINT Level, xb::D3DSURFACE_DESC *pDesc);
void __stdcall D3DTexture_LockRect(xb::D3DTexture *pThis, xb::UINT Level, xb::D3DLOCKED_RECT *pLockedRect,
                                   const xb::tagRECT *pRect, xb::DWORD Flags);
xb::HRESULT __stdcall D3DDevice_Clear(xb::DWORD Count, const void *pRects, xb::DWORD Flags, xb::DWORD Color,
                                      float Z, xb::DWORD Stencil);

static __forceinline void __stdcall D3DDevice_BeginScene() {}
static __forceinline void __stdcall D3DDevice_EndScene() {}
static __forceinline void D3DDevice_GetRenderState(xb::D3DRENDERSTATETYPE State, xb::DWORD *pValue)
{
    *pValue = D3D__RenderState[State];
}

void __fastcall D3DDevice_SetRenderState_Simple(xb::DWORD Method, xb::DWORD Value);
void __stdcall D3DDevice_SetRenderState_PSTextureModes(xb::DWORD Value);
void __stdcall D3DDevice_SetRenderState_VertexBlend(xb::DWORD Value);
void __stdcall D3DDevice_SetRenderState_FogColor(xb::DWORD Value);
void __stdcall D3DDevice_SetRenderState_FillMode(xb::DWORD Value);
void __stdcall D3DDevice_SetRenderState_BackFillMode(xb::DWORD Value);
void __stdcall D3DDevice_SetRenderState_TwoSidedLighting(xb::DWORD Value);
void __stdcall D3DDevice_SetRenderState_NormalizeNormals(xb::DWORD Value);
void __stdcall D3DDevice_SetRenderState_ZEnable(xb::DWORD Value);
void __stdcall D3DDevice_SetRenderState_StencilEnable(xb::DWORD Value);
void __stdcall D3DDevice_SetRenderState_StencilFail(xb::DWORD Value);
void __stdcall D3DDevice_SetRenderState_FrontFace(xb::DWORD Value);
void __stdcall D3DDevice_SetRenderState_CullMode(xb::DWORD Value);
void __stdcall D3DDevice_SetRenderState_TextureFactor(xb::DWORD Value);
void __stdcall D3DDevice_SetRenderState_ZBias(xb::DWORD Value);
void __stdcall D3DDevice_SetRenderState_LogicOp(xb::DWORD Value);
void __stdcall D3DDevice_SetRenderState_EdgeAntiAlias(xb::DWORD Value);
void __stdcall D3DDevice_SetRenderState_MultiSampleAntiAlias(xb::DWORD Value);
void __stdcall D3DDevice_SetRenderState_MultiSampleMask(xb::DWORD Value);
void __stdcall D3DDevice_SetRenderState_MultiSampleMode(xb::DWORD Value);
void __stdcall D3DDevice_SetRenderState_MultiSampleRenderTargetMode(xb::DWORD Value);
void __stdcall D3DDevice_SetRenderState_ShadowFunc(xb::DWORD Value);
void __stdcall D3DDevice_SetRenderState_LineWidth(xb::DWORD Value);
void __stdcall D3DDevice_SetRenderState_SampleAlpha(xb::DWORD Value);
void __stdcall D3DDevice_SetRenderState_Dxt1NoiseEnable(xb::DWORD Value);
void __stdcall D3DDevice_SetRenderState_YuvEnable(xb::DWORD Value);
void __stdcall D3DDevice_SetRenderState_OcclusionCullEnable(xb::DWORD Value);
void __stdcall D3DDevice_SetRenderState_StencilCullEnable(xb::DWORD Value);
void __stdcall D3DDevice_SetRenderState_RopZCmpAlwaysRead(xb::DWORD Value);
void __stdcall D3DDevice_SetRenderState_RopZRead(xb::DWORD Value);
void __stdcall D3DDevice_SetRenderState_DoNotCullUncompressed(xb::DWORD Value);

void __fastcall D3DDevice_SetVertexShaderConstant1Fast(xb::INT Register, const void *pConstantData);
void __fastcall D3DDevice_SetVertexShaderConstantNotInlineFast(xb::INT Register, const void *pConstantData,
                                                               xb::DWORD DwordCount);
// Register is -96..95 as the shader sees it (c0 is register 96 of the hardware's 192).
static __forceinline void D3DDevice_SetVertexShaderConstantFast(xb::INT Register, const void *pConstantData,
                                                                xb::DWORD ConstantCount)
{
    Register += 96;
    if (ConstantCount == 1)
        D3DDevice_SetVertexShaderConstant1Fast(Register, pConstantData);
    else
        D3DDevice_SetVertexShaderConstantNotInlineFast(Register, pConstantData, ConstantCount * 4);
}
void __stdcall D3DDevice_DrawVerticesUP(xb::DWORD PrimitiveType, xb::UINT VertexCount, const void *pVertexStreamZeroData,
                                        xb::UINT VertexStreamZeroStride);
// Raw push buffer access (XBParticle.obj): BeginPush reserves Count dwords and returns the write pointer.
xb::DWORD *__stdcall D3DDevice_BeginPush(xb::DWORD Count);
void __stdcall D3DDevice_EndPush(xb::DWORD *pPush);

static __forceinline void D3DDevice_SetRenderState(xb::D3DRENDERSTATETYPE State, xb::DWORD Value)
{
    if (State < xb::D3DRS_SIMPLE_MAX) {
        D3DDevice_SetRenderState_Simple(D3DSIMPLERENDERSTATEENCODE[State], Value);
        D3D__RenderState[State] = Value;
    } else if (State < xb::D3DRS_DEFERRED_MAX) {
        D3D__DirtyFlags |= D3DDIRTYFROMRENDERSTATE[State - xb::D3DRS_SIMPLE_MAX];
        D3D__RenderState[State] = Value;
    }
    else if (State == xb::D3DRS_PSTEXTUREMODES) D3DDevice_SetRenderState_PSTextureModes(Value);
    else if (State == xb::D3DRS_VERTEXBLEND) D3DDevice_SetRenderState_VertexBlend(Value);
    else if (State == xb::D3DRS_FOGCOLOR) D3DDevice_SetRenderState_FogColor(Value);
    else if (State == xb::D3DRS_FILLMODE) D3DDevice_SetRenderState_FillMode(Value);
    else if (State == xb::D3DRS_BACKFILLMODE) D3DDevice_SetRenderState_BackFillMode(Value);
    else if (State == xb::D3DRS_TWOSIDEDLIGHTING) D3DDevice_SetRenderState_TwoSidedLighting(Value);
    else if (State == xb::D3DRS_NORMALIZENORMALS) D3DDevice_SetRenderState_NormalizeNormals(Value);
    else if (State == xb::D3DRS_ZENABLE) D3DDevice_SetRenderState_ZEnable(Value);
    else if (State == xb::D3DRS_STENCILENABLE) D3DDevice_SetRenderState_StencilEnable(Value);
    else if (State == xb::D3DRS_STENCILFAIL) D3DDevice_SetRenderState_StencilFail(Value);
    else if (State == xb::D3DRS_CULLMODE) D3DDevice_SetRenderState_CullMode(Value);
    else if (State == xb::D3DRS_FRONTFACE) D3DDevice_SetRenderState_FrontFace(Value);
    else if (State == xb::D3DRS_TEXTUREFACTOR) D3DDevice_SetRenderState_TextureFactor(Value);
    else if (State == xb::D3DRS_ZBIAS) D3DDevice_SetRenderState_ZBias(Value);
    else if (State == xb::D3DRS_LOGICOP) D3DDevice_SetRenderState_LogicOp(Value);
    else if (State == xb::D3DRS_EDGEANTIALIAS) D3DDevice_SetRenderState_EdgeAntiAlias(Value);
    else if (State == xb::D3DRS_MULTISAMPLEANTIALIAS) D3DDevice_SetRenderState_MultiSampleAntiAlias(Value);
    else if (State == xb::D3DRS_MULTISAMPLEMASK) D3DDevice_SetRenderState_MultiSampleMask(Value);
    else if (State == xb::D3DRS_MULTISAMPLEMODE) D3DDevice_SetRenderState_MultiSampleMode(Value);
    else if (State == xb::D3DRS_MULTISAMPLERENDERTARGETMODE) D3DDevice_SetRenderState_MultiSampleRenderTargetMode(Value);
    else if (State == xb::D3DRS_SHADOWFUNC) D3DDevice_SetRenderState_ShadowFunc(Value);
    else if (State == xb::D3DRS_LINEWIDTH) D3DDevice_SetRenderState_LineWidth(Value);
    else if (State == xb::D3DRS_SAMPLEALPHA) D3DDevice_SetRenderState_SampleAlpha(Value);
    else if (State == xb::D3DRS_DXT1NOISEENABLE) D3DDevice_SetRenderState_Dxt1NoiseEnable(Value);
    else if (State == xb::D3DRS_YUVENABLE) D3DDevice_SetRenderState_YuvEnable(Value);
    else if (State == xb::D3DRS_OCCLUSIONCULLENABLE) D3DDevice_SetRenderState_OcclusionCullEnable(Value);
    else if (State == xb::D3DRS_STENCILCULLENABLE) D3DDevice_SetRenderState_StencilCullEnable(Value);
    else if (State == xb::D3DRS_ROPZCMPALWAYSREAD) D3DDevice_SetRenderState_RopZCmpAlwaysRead(Value);
    else if (State == xb::D3DRS_ROPZREAD) D3DDevice_SetRenderState_RopZRead(Value);
    else if (State == xb::D3DRS_DONOTCULLUNCOMPRESSED) D3DDevice_SetRenderState_DoNotCullUncompressed(Value);
}

extern xb::DWORD D3D__TextureState[4][32];
void __stdcall D3DDevice_SetTextureState_TexCoordIndex(xb::DWORD Stage, xb::DWORD Value);
void __stdcall D3DDevice_SetTextureState_BorderColor(xb::DWORD Stage, xb::DWORD Value);
void __stdcall D3DDevice_SetTextureState_ColorKeyColor(xb::DWORD Stage, xb::DWORD Value);
void __stdcall D3DDevice_SetTextureState_BumpEnv(xb::DWORD Stage, xb::D3DTEXTURESTAGESTATETYPE Type, xb::DWORD Value);
xb::HRESULT __stdcall D3DDevice_SetTexture(xb::DWORD Stage, xb::D3DBaseTexture *pTexture);

static __forceinline void D3DDevice_GetTextureStageState(xb::DWORD Stage, xb::D3DTEXTURESTAGESTATETYPE Type, xb::DWORD *pValue)
{
    *pValue = D3D__TextureState[Stage][Type];
}

static __forceinline void D3DDevice_SetTextureStageState(xb::DWORD Stage, xb::D3DTEXTURESTAGESTATETYPE Type, xb::DWORD Value)
{
    if (Type < xb::D3DTSS_DEFERRED_TEXTURE_STATE_MAX) {
        D3D__DirtyFlags |= (1 << Stage);
        D3D__TextureState[Stage][Type] = Value;
    } else if (Type < xb::D3DTSS_DEFERRED_MAX) {
        D3D__DirtyFlags |= D3DDIRTYFROMTEXTURESTATE[Type];
        D3D__TextureState[Stage][Type] = Value;
    }
    else if (Type == xb::D3DTSS_TEXCOORDINDEX) D3DDevice_SetTextureState_TexCoordIndex(Stage, Value);
    else if (Type == xb::D3DTSS_BORDERCOLOR) D3DDevice_SetTextureState_BorderColor(Stage, Value);
    else if (Type == xb::D3DTSS_COLORKEYCOLOR) D3DDevice_SetTextureState_ColorKeyColor(Stage, Value);
    else if (Type <= xb::D3DTSS_BUMPENVLOFFSET) D3DDevice_SetTextureState_BumpEnv(Stage, Type, Value);
}


// Device, presentation and resource entry points (D3D8.lib).
namespace xb {
struct _D3DVBLANKDATA { DWORD VBlank; DWORD Swap; DWORD Flags; };
struct _D3DGAMMARAMP { unsigned char red[256]; unsigned char green[256]; unsigned char blue[256]; };
struct _D3DRECT { long x1, y1, x2, y2; };
typedef void (__cdecl *D3DVBLANKCALLBACK)(_D3DVBLANKDATA *pData);
}
xb::HRESULT __stdcall Direct3D_CreateDevice(xb::UINT Adapter, xb::DWORD DeviceType, void *hFocusWindow,
                                            xb::DWORD BehaviorFlags, void *pPresentationParameters,
                                            xb::D3DDevice **ppReturnedDeviceInterface);
void __stdcall Direct3D_SetPushBufferSize(xb::DWORD PushBufferSize, xb::DWORD KickOffSize);
xb::DWORD __stdcall D3DDevice_Release();
xb::HRESULT __stdcall D3DDevice_Reset(void *pPresentationParameters);
void __stdcall D3DDevice_Swap(xb::DWORD Flags);
void __stdcall D3DDevice_SetSoftDisplayFilter(xb::INT Enable);
void __stdcall D3DDevice_SetGammaRamp(xb::DWORD Flags, const xb::_D3DGAMMARAMP *pRamp);
void __stdcall D3DDevice_SetRenderTarget(xb::D3DSurface *pRenderTarget, xb::D3DSurface *pNewZStencil);
long __stdcall D3DDevice_SetVertexShader(unsigned long Handle);
long __stdcall D3DDevice_LoadVertexShader(unsigned long Handle, unsigned long Address);
void __stdcall D3DDevice_SetShaderConstantMode(xb::DWORD Mode);
void __stdcall D3DDevice_BlockUntilIdle();
void __stdcall D3DDevice_SetVerticalBlankCallback(xb::D3DVBLANKCALLBACK pCallback);
void __stdcall D3DDevice_BlockUntilVerticalBlank();
void __stdcall D3DDevice_SetScissors(xb::DWORD Count, xb::INT Exclusive, const xb::_D3DRECT *pRects);
void __stdcall D3DDevice_SetScreenSpaceOffset(float x, float y);
void __stdcall D3DDevice_GetScreenSpaceOffset(float *pX, float *pY);
void __stdcall D3DDevice_SetDepthClipPlanes(float Near, float Far, xb::DWORD Flags);
void __stdcall D3DDevice_SetStreamSource(xb::UINT StreamNumber, void *pStreamData, xb::UINT Stride);
void __stdcall D3DDevice_SetIndices(void *pIndexData, xb::UINT BaseVertexIndex);
void __stdcall D3DResource_BlockUntilNotBusy(xb::D3DResource *pThis);
void __stdcall D3DResource_Register(xb::D3DResource *pThis, void *pBase);
void *__stdcall D3DVertexBuffer_Lock2(xb::D3DVertexBuffer *pThis, xb::DWORD Flags);
void __stdcall D3DDevice_SetPalette(xb::DWORD Stage, void *pPalette);
void __stdcall D3DSurface_GetDesc(xb::D3DSurface *pThis, xb::D3DSURFACE_DESC *pDesc);
void __stdcall D3DDevice_SetPixelShader(xb::DWORD Handle);
void __stdcall D3DDevice_CreatePixelShader(const xb::_D3DPixelShaderDef *pPSDef, xb::DWORD *pHandle);
xb::HRESULT __stdcall D3DDevice_CreateVertexShader(const xb::DWORD *pDeclaration, const xb::DWORD *pFunction,
                                                   xb::DWORD *pHandle, xb::DWORD Usage);
void __stdcall D3DDevice_SetPixelShaderConstant(xb::DWORD Register, const void *pConstantData, xb::DWORD ConstantCount);
xb::D3DTexture *__stdcall D3DDevice_CreateTexture2(xb::UINT Width, xb::UINT Height, xb::UINT Depth, xb::UINT Levels,
                                                   xb::DWORD Usage, xb::D3DFORMAT Format, xb::DWORD RType);

static __forceinline void __stdcall D3DTexture_UnlockRect(xb::D3DTexture *pThis, xb::UINT Level) {}
}   // extern "C"

namespace xb {
// Copies emitted in xbMovie.obj.
__forceinline void __stdcall D3DResource::BlockUntilNotBusy() { D3DResource_BlockUntilNotBusy(this); }
__forceinline void __stdcall D3DResource::Register(void *pBase) { D3DResource_Register(this, pBase); }
struct _D3DPRESENT_PARAMETERS_;
struct D3DTexture : public D3DBaseTexture {
    __forceinline HRESULT __stdcall GetSurfaceLevel(UINT Level, D3DSurface **ppSurfaceLevel)
    {
        D3DSurface *p = D3DTexture_GetSurfaceLevel2(this, Level);
        *ppSurfaceLevel = p;
        return p ? 0 : 0x8007000e;      // E_OUTOFMEMORY
    }
    __forceinline HRESULT __stdcall LockRect(UINT Level, D3DLOCKED_RECT *pLockedRect, const tagRECT *pRect, DWORD Flags)
    {
        D3DTexture_LockRect(this, Level, pLockedRect, pRect, Flags);
        return 0;
    }
    __forceinline HRESULT __stdcall UnlockRect(UINT Level)
    {
        D3DTexture_UnlockRect(this, Level);
        return 0;
    }
    __forceinline HRESULT __stdcall GetLevelDesc(UINT Level, D3DSURFACE_DESC *pDesc)
    {
        D3DTexture_GetLevelDesc(this, Level, pDesc);
        return 0;
    }
};
struct D3DDevice {
    static __forceinline HRESULT __stdcall CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, DWORD Pool,
                                                             D3DVertexBuffer **ppVertexBuffer)
    {
        *ppVertexBuffer = D3DDevice_CreateVertexBuffer2(Length);
        return *ppVertexBuffer ? 0 : 0x8007000e;
    }
    static __forceinline HRESULT __stdcall GetBackBuffer(INT BackBuffer, DWORD Type, D3DSurface **ppBackBuffer)
    {
        *ppBackBuffer = D3DDevice_GetBackBuffer2(BackBuffer);
        return 0;
    }
    static __forceinline DWORD __stdcall Release() { return D3DDevice_Release(); }
    static __forceinline HRESULT __stdcall Reset(_D3DPRESENT_PARAMETERS_ *pPP) { return D3DDevice_Reset(pPP); }
    static __forceinline HRESULT __stdcall Present(const tagRECT *, const tagRECT *, void *, void *)
    {
        D3DDevice_Swap(0);
        return 0;
    }
    static __forceinline void __stdcall SetSoftDisplayFilter(INT Enable) { D3DDevice_SetSoftDisplayFilter(Enable); }
    static __forceinline void __stdcall SetGammaRamp(DWORD Flags, const _D3DGAMMARAMP *pRamp) { D3DDevice_SetGammaRamp(Flags, pRamp); }
    static __forceinline HRESULT __stdcall SetRenderTarget(D3DSurface *pRenderTarget, D3DSurface *pNewZStencil)
    {
        D3DDevice_SetRenderTarget(pRenderTarget, pNewZStencil);
        return 0;
    }
    static __forceinline HRESULT __stdcall GetRenderTarget(D3DSurface **ppRenderTarget)
    {
        *ppRenderTarget = D3DDevice_GetRenderTarget2();
        return 0;
    }
    static __forceinline HRESULT __stdcall GetDepthStencilSurface(D3DSurface **ppZStencilSurface)
    {
        D3DSurface *p = D3DDevice_GetDepthStencilSurface2();
        *ppZStencilSurface = p;
        return p ? 0 : 0x88760866;      // D3DERR_NOTFOUND
    }
    static __forceinline HRESULT __stdcall BeginScene()
    {
        D3DDevice_BeginScene();
        return 0;
    }
    static __forceinline HRESULT __stdcall EndScene()
    {
        D3DDevice_EndScene();
        return 0;
    }
    static __forceinline HRESULT __stdcall Clear(DWORD Count, const _D3DRECT *pRects, DWORD Flags, DWORD Color, float Z,
                                                 DWORD Stencil)
    {
        D3DDevice_Clear(Count, pRects, Flags, Color, Z, Stencil);
        return 0;
    }
    static __forceinline HRESULT __stdcall SetRenderState(D3DRENDERSTATETYPE State, DWORD Value)
    {
        D3DDevice_SetRenderState(State, Value);
        return 0;
    }
    static __forceinline HRESULT __stdcall GetRenderState(D3DRENDERSTATETYPE State, DWORD *pValue)
    {
        D3DDevice_GetRenderState(State, pValue);
        return 0;
    }
    static __forceinline HRESULT __stdcall SetTexture(DWORD Stage, D3DBaseTexture *pTexture)
    {
        D3DDevice_SetTexture(Stage, pTexture);
        return 0;
    }
    static __forceinline HRESULT __stdcall GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD *pValue)
    {
        D3DDevice_GetTextureStageState(Stage, Type, pValue);
        return 0;
    }
    static __forceinline HRESULT __stdcall SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value)
    {
        D3DDevice_SetTextureStageState(Stage, Type, Value);
        return 0;
    }
    static __forceinline HRESULT __stdcall DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount,
                                                           const void *pVertexStreamZeroData, UINT VertexStreamZeroStride)
    {
        D3DDevice_DrawVerticesUP(PrimitiveType,
                                 D3DPRIMITIVETOVERTEXCOUNT[PrimitiveType][0] * PrimitiveCount + D3DPRIMITIVETOVERTEXCOUNT[PrimitiveType][1],
                                 pVertexStreamZeroData, VertexStreamZeroStride);
        return 0;
    }
    static __forceinline HRESULT __stdcall LoadVertexShader(DWORD Handle, DWORD Address)
    {
        D3DDevice_LoadVertexShader(Handle, Address);
        return 0;
    }
    static __forceinline HRESULT __stdcall SetVertexShaderConstantFast(INT Register, const void *pConstantData, DWORD ConstantCount)
    {
        D3DDevice_SetVertexShaderConstantFast(Register, pConstantData, ConstantCount);
        return 0;
    }
    // Typed overloads (the XDK signatures; retail mangles these). The void * forms above stay for existing callers.
    static __forceinline HRESULT __stdcall SetStreamSource(UINT StreamNumber, D3DVertexBuffer *pStreamData, UINT Stride)
    {
        D3DDevice_SetStreamSource(StreamNumber, pStreamData, Stride);
        return 0;
    }
    static __forceinline HRESULT __stdcall SetIndices(D3DIndexBuffer *pIndexData, UINT BaseVertexIndex)
    {
        D3DDevice_SetIndices(pIndexData, BaseVertexIndex);
        return 0;
    }
    static __forceinline HRESULT __stdcall SetVertexShader(DWORD Handle)
    {
        D3DDevice_SetVertexShader(Handle);
        return 0;
    }
    static HRESULT __stdcall CreateVertexShader(const DWORD *pDeclaration, const DWORD *pFunction, DWORD *pHandle,
                                                DWORD Usage)
    {
        return D3DDevice_CreateVertexShader(pDeclaration, pFunction, pHandle, Usage);
    }
    static HRESULT __stdcall CreatePixelShader(const _D3DPixelShaderDef *pPSDef, DWORD *pHandle)
    {
        D3DDevice_CreatePixelShader(pPSDef, pHandle);
        return 0;
    }
    static HRESULT __stdcall SetPixelShader(DWORD Handle)
    {
        D3DDevice_SetPixelShader(Handle);
        return 0;
    }
    static HRESULT __stdcall SetPixelShaderConstant(DWORD Register, const void *pConstantData, DWORD ConstantCount)
    {
        D3DDevice_SetPixelShaderConstant(Register, pConstantData, ConstantCount);
        return 0;
    }
    static HRESULT __stdcall CreateTexture(UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format,
                                           DWORD Pool, D3DTexture **ppTexture)
    {
        *ppTexture = D3DDevice_CreateTexture2(Width, Height, 1, Levels, Usage, Format, 3);
        return *ppTexture ? 0 : 0x8007000e;
    }
    static __forceinline HRESULT __stdcall SetShaderConstantMode(DWORD Mode)
    {
        D3DDevice_SetShaderConstantMode(Mode);
        return 0;
    }
    static __forceinline void __stdcall BlockUntilIdle() { D3DDevice_BlockUntilIdle(); }
    static __forceinline void __stdcall SetVerticalBlankCallback(D3DVBLANKCALLBACK pCallback)
    {
        D3DDevice_SetVerticalBlankCallback(pCallback);
    }
    static __forceinline void __stdcall BlockUntilVerticalBlank() { D3DDevice_BlockUntilVerticalBlank(); }
    static __forceinline HRESULT __stdcall SetScissors(DWORD Count, INT Exclusive, const _D3DRECT *pRects)
    {
        D3DDevice_SetScissors(Count, Exclusive, pRects);
        return 0;
    }
    static __forceinline HRESULT __stdcall SetScreenSpaceOffset(float x, float y)
    {
        D3DDevice_SetScreenSpaceOffset(x, y);
        return 0;
    }
    static __forceinline HRESULT __stdcall GetScreenSpaceOffset(float *pX, float *pY)
    {
        D3DDevice_GetScreenSpaceOffset(pX, pY);
        return 0;
    }
    static __forceinline void __stdcall SetDepthClipPlanes(float Near, float Far, DWORD Flags)
    {
        D3DDevice_SetDepthClipPlanes(Near, Far, Flags);
    }
    static __forceinline HRESULT __stdcall SetStreamSource(UINT StreamNumber, void *pStreamData, UINT Stride)
    {
        D3DDevice_SetStreamSource(StreamNumber, pStreamData, Stride);
        return 0;
    }
    static __forceinline HRESULT __stdcall SetIndices(void *pIndexData, UINT BaseVertexIndex)
    {
        D3DDevice_SetIndices(pIndexData, BaseVertexIndex);
        return 0;
    }
    static __forceinline HRESULT __stdcall DrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT MinIndex,
                                                                UINT NumIndices, UINT StartIndex, UINT PrimitiveCount)
    {
        D3DDevice_DrawIndexedVertices(PrimitiveType,
                                      D3DPRIMITIVETOVERTEXCOUNT[PrimitiveType][0] * PrimitiveCount + D3DPRIMITIVETOVERTEXCOUNT[PrimitiveType][1],
                                      D3D__IndexData + StartIndex);
        return 0;
    }
    static __forceinline HRESULT __stdcall SetVertexShaderInputDirect(_D3DVERTEXATTRIBUTEFORMAT *pVAF, UINT StreamCount,
                                                                      const _D3DSTREAM_INPUT *pStreamInputs)
    {
        D3DDevice_SetVertexShaderInputDirect(pVAF, StreamCount, pStreamInputs);
        return 0;
    }
    static __forceinline HRESULT __stdcall SetPalette(DWORD Stage, void *pPalette)
    {
        D3DDevice_SetPalette(Stage, pPalette);
        return 0;
    }
    // Emitted in XBParticle.obj.
    static __forceinline HRESULT __stdcall BeginPush(DWORD Count, DWORD **ppPush)
    {
        *ppPush = D3DDevice_BeginPush(Count);
        return 0;
    }
    static __forceinline HRESULT __stdcall EndPush(DWORD *pPush)
    {
        D3DDevice_EndPush(pPush);
        return 0;
    }
};
__forceinline DWORD __stdcall D3DResource::Release() { return D3DResource_Release(this); }
__forceinline HRESULT __stdcall D3DSurface::GetDesc(D3DSURFACE_DESC *pDesc)
{
    D3DSurface_GetDesc(this, pDesc);
    return 0;
}
struct Direct3D {
    static __forceinline HRESULT __stdcall CreateDevice(UINT Adapter, D3DDEVTYPE DeviceType, void *hFocusWindow,
                                                        DWORD BehaviorFlags, _D3DPRESENT_PARAMETERS_ *pPP,
                                                        D3DDevice **ppDevice)
    {
        return Direct3D_CreateDevice(Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPP, ppDevice);
    }
    static __forceinline HRESULT __stdcall CreateDevice(UINT Adapter, DWORD DeviceType, void *hFocusWindow,
                                                        DWORD BehaviorFlags, _D3DPRESENT_PARAMETERS_ *pPP,
                                                        D3DDevice **ppDevice)
    {
        return Direct3D_CreateDevice(Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPP, ppDevice);
    }
    static __forceinline HRESULT __stdcall SetPushBufferSize(DWORD PushBufferSize, DWORD KickOffSize)
    {
        Direct3D_SetPushBufferSize(PushBufferSize, KickOffSize);
        return 0;
    }
};
}
#endif
