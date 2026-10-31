#ifndef EXBASEDISPLAY_H
#define EXBASEDISPLAY_H
#include "types.h"
#include "EXList.h"
#include "EXRect.h"
#include "EXString.h"
#include "EXVector.h"
#include "EXMaths.h"
#include "EXFlag.h"
#include "RGBA.h"

class EXWnd;
class EXTexture;
class EXTextureArray;
class EXGeoHeader;
struct _EXResolution;
template <class T, class ARG> class EXDataArray;

// Display mode request (sizeof 0x18). TODO names: XDisplay::OpenDefault fills
// fb w/h/bpp, back-buffer w/h/bpp, depth 0x18, 1, 0. A class: retail mangles it with V.
class EXPresentParams {
public:
    EXPresentParams() { m_VSync = 1; }
    u16 m_FBWidth;    // 0x00
    u16 m_FBHeight;   // 0x02
    u16 m_Unk04;      // 0x04 bpp (0x20)
    u16 m_Width;      // 0x06
    u16 m_Height;     // 0x08
    u16 m_Unk0a;      // 0x0a bpp (0x20)
    u16 m_Unk0c;      // 0x0c depth bits (0x18)
    u16 m_Unk0e;      // 0x0e TODO
    u32 m_Unk10;      // 0x10 TODO
    s8 m_VSync;       // 0x14
    u8 m_Pad15[3];
};

// Platform-independent display: owns the top-level window list and the transform cache
// (EXDisplay.obj). EXDisplay (XBDisplay.obj) is the Xbox implementation.
class EXBaseDisplay {
public:
    virtual ~EXBaseDisplay();
    virtual s8 OpenDefault() = 0;
    virtual s8 Open(const EXPresentParams &params) = 0;
    virtual s8 ReOpen(const EXPresentParams &params) = 0;
    virtual s8 Close() = 0;
    virtual void SetColorAdjustment(float, float, float) {}
    virtual void SetDefaultRenderState() = 0;
    virtual void UpdateRenderState() = 0;
    virtual void UpdateTransforms() = 0;
    virtual s8 BeginDraw() = 0;
    virtual s8 EndDraw() = 0;
    virtual s8 Present() = 0;
    virtual EXString CaptureToFile() = 0;
    virtual void FlushAllWindowMem();
    virtual void DeleteAllWindowMem();
    virtual void WaitDrawSync() = 0;
    virtual void RegisterWatcherVars();
    virtual EXDataArray<_EXResolution, _EXResolution> *GetAvailableResolutions() { return 0; }

    static EXBaseDisplay *GetDisplay() { return m_pDisplay; }
    static const EXMaths::vec_xyzw &ViewToCullScale() { return ms_VtxViewToCull; }
    static EXWnd *ActiveWnd() { return ms_pActiveWnd; }
    static void SetActiveWnd(EXWnd *pWnd) { ms_pActiveWnd = pWnd; }
    static EXPoint &Offset() { return ms_Offset; }
    static EXGeoHeader *GeoHeader() { return ms_pGeoHeader; }
    static void SetGeoHeader(EXGeoHeader *p) { ms_pGeoHeader = p; }
    static EXPoint DispToPrim(const EXPoint &p)
    {
        EXPoint r;
        r.x = p.x + ms_Offset.x;
        r.y = p.y + ms_Offset.y;
        return r;
    }
    static EXPoint PrimToDisp(const EXPoint &p)
    {
        EXPoint r;
        r.x = p.x - ms_Offset.x;
        r.y = p.y - ms_Offset.y;
        return r;
    }
    unsigned short Width() const { return m_Params.m_Width; }
    unsigned short Height() const { return m_Params.m_Height; }
    const EXPresentParams &Params() const { return m_Params; }
    void EnableVSync(s8 v) { m_Params.m_VSync = v; }
    s8 VSyncEnabled() const { return m_Params.m_VSync; }
    void SetCaptureDir(const EXString &dir) { m_CaptureDir = dir; }
    void DoCaptureToFile() { m_DoCapture = 1; }
    EXRect SafeFrame() const { return m_SafeFrame; }
    EXWnd *TopWnd() const { return m_Wnds.Tail() ? (EXWnd *)((char *)m_Wnds.Tail() - 4) : 0; }
    EXWnd *BottomWnd() const { return m_Wnds.Head() ? (EXWnd *)((char *)m_Wnds.Head() - 4) : 0; }
    EXWnd *FocusWnd() { return m_pFocusWnd; }
    EXWnd *MouseWnd() { return m_pMouseWnd; }
    void SetFocusWnd(EXWnd *pWnd) { m_pFocusWnd = pWnd; }
    void SetMouseWnd(EXWnd *pWnd) { m_pMouseWnd = pWnd; }
    static const EXMaths::mat_44 &WorldToScreen() { return ms_mtxArray[8]; }
    // Appended by the effects agent (addresses from the map's emitted copies).
    static const EXMaths::mat_44 &ObjectToWorld() { return ms_mtxArray[4]; }
    // Appended by the item agent (addresses from the map's emitted copies; EXItemEnv.obj).
    static const EXMaths::mat_44 &ReflectMatrix() { return ms_mtxArray[5]; }
    static void SetReflectMatrix(const EXMaths::mat_44 &m) { ms_mtxArray[5] = m; }
    static const EXMaths::mat_44 &ObjectToCull() { return ms_mtxArray[10]; }
    static const EXMaths::mat_44 &ObjectToVap() { return ms_mtxArray[14]; }  // map agent (0x49e038)
    static const EXMaths::mat_44 &ViewToCull() { return ms_mtxArray[1]; }
    static const EXMaths::mat_44 &ObjectToScreen() { return ms_mtxArray[11]; }
    static EXVector PrimToDisp(const EXVector &p) { return EXVector(p.x - ms_Offset.x, p.y - ms_Offset.y, p.z, 0.0f); }
    // Appended by the item agent (EXItemEnv.obj RenderMaps; emitted in EXItemAnimator_Entity.obj).
    static void SetObjectToWorld(const EXMaths::mat_44 &m)
    {
        ms_mtxArray[4] = ms_mtxArray[5] * m;
        ms_UpdObjectToWorld = 1;
    }
    // Setters emitted in XBCamera.obj.
    static void SetWorldToView(const EXMaths::mat_44 &m)
    {
        ms_mtxArray[3] = m;
        ms_UpdWorldToView = 1;
    }
    static void SetViewToCull(const EXMaths::mat_44 &m)
    {
        ms_mtxArray[1] = m;
        ms_UpdViewToCull = 1;
    }
    static void SetViewToScreen(const EXMaths::mat_44 &m)
    {
        ms_mtxArray[2] = m;
        ms_UpdViewToScreen = 1;
    }
    static void SetViewToScreenCoords(const EXMaths::mat_44 &m)
    {
        ms_mtxArray[12] = m;
        ms_UpdViewToScreen = 1;
    }
    static void SetWorldToShadowCull(const EXMaths::mat_44 &m)
    {
        ms_mtxArray[16] = m;
        ms_UpdWorldToShadowCull = 1;
    }
    static void SetObjectToVap(const EXMaths::mat_44 &m) { ms_mtxArray[14] = m; }
    static void SetViewToCullScale(const EXMaths::vec_xyzw &v) { ms_VtxViewToCull = v; }
    static const EXMaths::mat_44 &WorldToView() { return ms_mtxArray[3]; }
    static const EXMaths::mat_44 &ViewToScreen() { return ms_mtxArray[2]; }  // XBParticle.obj (inlined)
    static const EXMaths::mat_44 &ViewToScreenCoords() { return ms_mtxArray[12]; }
    static const EXMaths::mat_44 &WorldToShadowCull() { return ms_mtxArray[16]; }  // XBGeoEntity.obj copy

    static s8 Init(EXBaseDisplay *pDisplay);
    static void Shutdown();
    void RemoveWnd(EXWnd *pWnd);
    s8 IsWndVisible(EXWnd *pWnd) const;
    static EXVector2 WorldToDisp(const EXVector &v);
    EXTexture *NewRuntimeTexture(unsigned short w, unsigned short h, unsigned char fmt, unsigned int flags);
    void DelRuntimeTexture(EXTexture *pTex);
    EXBaseDisplay();
    EXWnd *AddWnd(EXWnd *pWnd);
    void DelWnd(EXWnd *pWnd);
    void WndToTop(EXWnd *pWnd);
    void WndToBottom(EXWnd *pWnd);
    void RedrawWindows();
    EXWnd *WindowUnder(const EXPoint &p, EXWnd *pIgnore) const;
    void UpdateWindows();
protected:
    static s8 CalcPrimRectWindow(EXDListItem *pItem);
    static s8 FlushWindowMem(EXDListItem *pItem);
    static s8 DeleteWindowMem(EXDListItem *pItem);
    static s8 RedrawWindow(EXDListItem *pItem);
    static s8 UpdateWindow(EXDListItem *pItem);
    void SetupSafeFrame();

    static EXMaths::vec_xyzw ms_VtxViewToCull;
    static s8 ms_UpdWorldToShadowCull;
    static s8 ms_UpdClipToScreen;
    static s8 ms_UpdViewToScreen;
    static s8 ms_UpdViewToCull;
    static s8 ms_UpdViewToClip;
    static s8 ms_UpdWorldToView;
    static s8 ms_UpdObjectToWorld;
    static EXMaths::mat_44 ms_mtxArray[17];
    static float ms_TintBlue;
    static float ms_TintGreen;
    static float ms_TintRed;
    static EXGeoHeader *ms_pGeoHeader;
    static EXWnd *ms_pActiveWnd;
    static s8 m_bPerformanceGraphsActivated;
    static EXBaseDisplay *m_pDisplay;
    static EXPoint ms_Offset;
    static ex::EXFlag<unsigned int> ms_Flags;
    static ex::RGBA ms_BkColor;

public:
    EXPresentParams m_Params;          // 0x04
    EXDList m_Wnds;                    // 0x1c top-level windows, back to front
    EXWnd *m_pFocusWnd;                // 0x24
    EXWnd *m_pMouseWnd;                // 0x28
    EXString m_CaptureDir;             // 0x2c
    s8 m_DoCapture;                    // 0x30
    s8 m_CaptureContinuous;            // 0x31
    s8 m_Unk32;                        // 0x32
    s8 m_Unk33;                        // 0x33
    s8 m_Unk34;                        // 0x34
    s8 m_SafeFrameEnabled;             // 0x35
    s8 m_Unk36;                        // 0x36
    EXRect m_SafeFrame;                // 0x38
    EXTextureArray *m_pRuntimeTextures;  // 0x48
    u32 m_Unk4c;                       // 0x4c
    int m_NumRuntimeTextures;          // 0x50
};
#endif
