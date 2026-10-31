#ifndef EXDISPLAY_H
#define EXDISPLAY_H
#include "EXBaseDisplay.h"

namespace xb {
struct D3DDevice;
struct D3DSurface;
// XDK D3DPRESENT_PARAMETERS (sizeof 0x44).
struct _D3DPRESENT_PARAMETERS_ {
    u32 BackBufferWidth;                // 0x00
    u32 BackBufferHeight;               // 0x04
    u32 BackBufferFormat;               // 0x08
    u32 BackBufferCount;                // 0x0c
    u32 MultiSampleType;                // 0x10
    u32 SwapEffect;                     // 0x14
    void *hDeviceWindow;                // 0x18
    u32 Windowed;                       // 0x1c
    u32 EnableAutoDepthStencil;         // 0x20
    u32 AutoDepthStencilFormat;         // 0x24
    u32 Flags;                          // 0x28
    u32 FullScreen_RefreshRateInHz;     // 0x2c
    u32 FullScreen_PresentationInterval;  // 0x30
    D3DSurface *BufferSurfaces[3];      // 0x34
    D3DSurface *DepthStencilSurface;    // 0x40
};
}
struct EXBoundsSphere;
class EXMatrix;
class EXD3DResource;
class EXCachedGeoEntity;
namespace xb { struct D3DTexture; }

#include "XBRenderTargetManager.h"

// Xbox display (XBDisplay.obj, sizeof 0x9a0).
class EXDisplay : public EXBaseDisplay {
public:
    virtual ~EXDisplay();
    virtual s8 OpenDefault();
    virtual s8 Open(const EXPresentParams &params);
    virtual s8 ReOpen(const EXPresentParams &params);
    virtual s8 Close();
    virtual void SetDefaultRenderState();
    virtual void UpdateRenderState() {}  // App.obj copy
    virtual void UpdateTransforms();
    virtual s8 BeginDraw();
    virtual s8 EndDraw();
    virtual s8 Present();
    virtual EXString CaptureToFile();
    virtual void FlushAllWindowMem();
    virtual void WaitDrawSync();
    EXDisplay();
    xb::D3DDevice *Device() { return m_pDevice; }
    void AddD3DResource(EXD3DResource *p);
    s8 SetRenderTarget(xb::D3DSurface *pColour, xb::D3DSurface *pDepth);
    s8 SetViewPort(const EXRect *pRect, const EXRect *pClip);
    xb::D3DTexture *NullTexture();
    const EXCachedGeoEntity *GetCurrentDeferredEntity() { return m_pCurrentDeferredEntity; }
    // Appended by the XBGeoEntity agent (copies in XBGeoEntity.obj).
    void SetCurrentDifferedEntity(EXCachedGeoEntity *p) { m_pCurrentDeferredEntity = p; }
    EXVector *GetCameraPositionNormalised() { return (EXVector *)m_CameraPosNormalised; }
    // Emitted in XBCamera.obj.
    void SetNormalProjection(EXMatrix &m) { m_NormalProjection = *(const EXMaths::mat_44 *)&m; }
    void SetCameraPosition(EXVector &v) { *(EXVector *)&m_934 = v; }
    void SetCameraPositionNormalised(EXVector &v) { *(EXVector *)m_CameraPosNormalised = v; }

    // Write back the CPU cache once per frame before the GPU reads vertices the CPU just wrote.
    void InvalidateWriteCache()
    {
        if (!m_InvalidateWriteCache) {
            __asm wbinvd
            m_InvalidateWriteCache = 1;
        }
    }
    xb::D3DTexture *NullTextureBlack();
    float HalfWidth() { return m_HalfWidth; }
    float HalfHeight() { return m_HalfHeight; }
    XBRenderTargetManager *GetRenderTargetManager() { return &m_RenderTargets; }
    EXMatrix *GetNormalProjection() { return (EXMatrix *)&m_NormalProjection; }
    void IncVerticalBlanks()
    {
        m_VerticalBlanks++;
        m_TotalVerticalBlanks++;
    }
    void SelectDebugTexture();
    void SelectNullTexture();
    void SelectNullTextureBlack();
    void ReleaseD3DResources();
    void SetDefaultXBoxRenderState();
    void RenderDebugChar(unsigned char c, const EXVector &pos, const ex::RGBA &col);
protected:
    s8 SetUp(const EXPresentParams &params);
    s8 InitNullTexture();
    s8 InitNullTextureBlack();
    s8 InitDebugFont();
    static s8 ReleaseD3DResource(EXDListItem *pItem);
    s8 SetViewPortInline(const EXRect *pRect, const EXRect *pClip);
    static s8 ms_UpdWorldToScreen;
public:
    void RemD3DResource(EXD3DResource *p);
    // Appended by the app-layer agent (copies emitted in XBApp.obj).
    const xb::_D3DPRESENT_PARAMETERS_ *GetPresentParameters() { return &m_PresentParams; }
    void EnableInvalidateWriteCache() { m_InvalidateWriteCache = 0; }
    unsigned int GetAndResetVerticalBlanks()
    {
        unsigned int n = m_VerticalBlanks;
        m_VerticalBlanks = 0;
        return n;
    }
    unsigned int GetVerticalBlanks() { return m_VerticalBlanks; }
    static int CullBox(const float *pBox, const EXMaths::mat_44 &m);  // appended by the item agent (EXItemEnv.obj call)
    static int CullSphere(const EXBoundsSphere &s, const EXMaths::mat_44 &m);  // map agent (EXGeoMap.obj call)

    u32 m_InvalidateWriteCache;                  // 0x54 cache already written back this frame (cleared by EnableInvalidateWriteCache)
    xb::_D3DPRESENT_PARAMETERS_ m_PresentParams;  // 0x58
    xb::D3DDevice *m_pDevice;                     // 0x9c
    xb::D3DTexture *m_pDebugFont;                 // 0xa0 128x256 bitmap font (InitDebugFont)
    xb::D3DTexture *m_pNullTexture;               // 0xa4 16x16 opaque white
    xb::D3DTexture *m_pNullTextureBlack;          // 0xa8 16x16 opaque black
    float m_DebugFontU[256];                      // 0xac texel origin of each character
    float m_DebugFontV[256];                      // 0x4ac
    EXDList m_D3DResources;                       // 0x8ac EXD3DResource list (released on Close)
    EXMaths::mat_44 m_NormalProjection;           // 0x8b4 identity until the game sets one
    EXMaths::mat_44 m_ViewProjection;             // 0x8f4 ms_mtxArray[2] * WorldToView (UpdateTransforms)
    EXMaths::vec_xyzw m_934;                      // 0x934 TODO use; zeroed with set_zero (w first) in the ctor
    float m_CameraPosNormalised[4];               // 0x944 unit camera position (DrawDirectX takes it to object space)
    EXRect m_TargetRect;                          // 0x954 current render target (0, 0, w, h); scissors clamp to it
    s8 m_GammaSet;                                // 0x964 a tint gamma ramp is loaded
    u8 m_965[3];
    float m_HalfWidth;                            // 0x968 half the frame buffer size (SetUp); the expansion blur works at this size
    float m_HalfHeight;                           // 0x96c
    float m_ScreenSpaceOffsetX;                   // 0x970 wanted D3D screen-space offset (applied in BeginDraw)
    float m_ScreenSpaceOffsetY;                   // 0x974
    const EXCachedGeoEntity *m_pCurrentDeferredEntity;  // 0x978 entity being drawn (texture selection reads it)
    u32 m_VideoMemUsed;                           // 0x97c frame buffers (w * h * 12) + 2 MB push buffer + textures
    volatile unsigned int m_VerticalBlanks;       // 0x980 counted by the vblank callback
    volatile unsigned int m_TotalVerticalBlanks;  // 0x984
    unsigned int m_FrameRate;                     // 0x988 EXBaseApp::MaxFrameRate the presentation interval was set for
    XBRenderTargetManager m_RenderTargets;        // 0x98c

};
#endif
