#ifndef EXBASERENDERENV_H
#define EXBASERENDERENV_H
#include "types.h"
#include "EXVector.h"
#include "EXRenderFogInfo.h"
#include "EXRenderInfo.h"

struct EXDynLightData;
struct EXBoundsBox;
class EXLightManager;
class EXRenderInfo;

// TODO layout: stand-in for the render environment (EXRenderEnv.obj, render area). Only the
// static members and inline accessors the item layer uses are declared; names from the map.
class EXBaseRenderEnv {
    friend class EXGeoMap;  // DrawZone sets render-info flags directly
public:
    EXBaseRenderEnv();
    virtual ~EXBaseRenderEnv();
    virtual void CalcFogCoefficients() {}
    virtual void RegisterWatcherVars();
    // Appended by the map agent (inlines emitted in EXGeoMap.obj).
    static EXLightManager *LightManager() { return ms_pLightManager; }
    static void SetLightSet(float f) { ms_RenderInfo.SetLightSet(f); }
    static void EnableVertexSet(s8 b)
    {
        if (b) ms_RenderInfo.m_renderFlags.BitSet(0x1000);
        else ms_RenderInfo.m_renderFlags.BitClr(0x1000);
    }
    static void Update();  // EXBaseApp::UpdateDisplay (appended by the app-layer agent)
    static void SetRenderInfo(const EXRenderInfo &info);  // appended by the item agent (EXItemRender_Anim.obj call)
    static void SetLightManager(EXLightManager *p) { ms_pLightManager = p; }  // appended by the item agent
    static u32 RenderState() { return ms_RenderState; }
    static u32 SetRenderState(u32 s) { u32 old = ms_RenderState; ms_RenderState = s; return old; }
    static s8 RenderCameras() { return ms_RenderCameras; }
    static s8 ShadowRender() { return ms_ShadowRender; }
    static void SetShadowRender(s8 s) { ms_ShadowRender = s; }  // emitted in EXWnd.obj (0x21ed10)
    static s8 WideScreen() { return ms_WideScreen; }
    static void SetWideScreen(s8 b) { ms_WideScreen = b; }  // emitted in FEHud.obj (front-end agent)
    // Appended by the effects agent (dynamic lights; inlines emitted in EXLightManager.obj / XBGeoEntity.obj).
    static void SetDynLightCount(u32 n) { ms_DynLightCount = n; }
    static void SetDynLight(u32 i, EXDynLightData *p) { ms_DynLights[i] = p; }
    static EXDynLightData *DynLight(u32 i) { return ms_DynLights[i]; }
    static void EnableDynLightRGBMult(s8 b) { ms_DynLightRGBMultEnable = b; }
    static void SetDynLightRGBMult(const EXVector &v) { ms_DynLightRGBMult = (const EXMaths::vec_xyzw)v; }
    // Appended by the item agent (inlines emitted in EXItemEnv.obj).
    static void SetReflectionRender(s8 b) { ms_ReflectionRender = b; }
    static float ReflectionHeight() { return ms_ReflectionHeight; }
    static void SetFogDistance(float n, float f)
    {
        ms_RenderFogInfo.m_near = n;
        ms_RenderFogInfo.m_far = f;
    }
    static void SetFogMinMax(float mn, float mx)
    {
        ms_RenderFogInfo.m_min = mn;
        ms_RenderFogInfo.m_max = mx;
    }
    static void SetFogColor(ex::RGBA c) { ms_RenderFogInfo.m_color = c; }
    static void SetFogEnabled(s8 b) { ms_RenderFogInfo.m_enabled = b; }
    static s8 FogEnabled() { return ms_RenderFogInfo.m_enabled; }
    static s8 OverrideFogEnabled() { return ms_OverrideFogInfo.m_enabled; }
    static void SetOverrideFogEnabled(s8 b) { ms_OverrideFogInfo.m_enabled = b; }
    // Fog forced over the level's (vision modes, NightAndDay.obj).
    static void SetOverrideFog(ex::RGBA c, float n, float f, float mn, float mx, s8 en)
    {
        ms_OverrideFogInfo.m_near = n;
        ms_OverrideFogInfo.m_far = f;
        ms_OverrideFogInfo.m_min = mn;
        ms_OverrideFogInfo.m_enabled = en;
        ms_OverrideFogInfo.m_color = c;
        ms_OverrideFogInfo.m_max = mx;
    }
    static void SetRenderFogInfo(const EXRenderFogInfo &info) { ms_RenderFogInfo = info; }
    static EXRenderFogInfo &RenderOverrideFogInfo() { return ms_OverrideFogInfo; }  // [fi] copy in CameraScript.obj
    static void EnableRGBAMult(s8 b)  // [fi] copy in CameraScript.obj
    {
        if (b) ms_GRenderInfo.m_renderFlags.BitSet(6);
        else ms_GRenderInfo.m_renderFlags.BitClr(6);
    }
    static const EXRenderFogInfo &RenderFogInfo() { return ms_RenderFogInfo; }
    // Appended by the XBGeoEntity agent (copies in XBGeoEntity.obj).
    static const EXRenderInfo &RenderInfo() { return ms_RenderInfo; }
    static s8 DistanceFade() { return ms_DistanceFade; }
    // Distance fade of a skin being drawn (set and cleared around EXItemRender_SkinAnim::RenderSkin).
    static void SetDistanceFade(float start, float end, u8 layers)
    {
        ms_DistanceFade = 1;
        ms_DistanceFadeStart = start;
        ms_DistanceFadeEnd = end;
        ms_DistanceFadeLayerMask = layers;
    }
    static void ClearDistanceFade()
    {
        ms_DistanceFade = 0;
        ms_RenderInfo.SetVertexSet(0, 1, 0.0f);
        ms_DistanceFadeLayerMask = 0;
    }
    static u8 DistanceFadeLayerMask() { return ms_DistanceFadeLayerMask; }
    static float DistanceFadeStart() { return ms_DistanceFadeStart; }
    static float DistanceFadeEnd() { return ms_DistanceFadeEnd; }
    static s8 ReflectionRender() { return ms_ReflectionRender; }
    static float FogNear() { return ms_RenderFogInfo.m_near; }
    static float FogFar() { return ms_RenderFogInfo.m_far; }
    static float FogMin() { return ms_RenderFogInfo.m_min; }
    static float FogMax() { return ms_RenderFogInfo.m_max; }
    static ex::RGBA FogColor() { return ms_RenderFogInfo.m_color; }
    static float ShadowDensity() { return (s16)ms_RenderInfo.m_shadowDensity * (1.0f / 32767.0f); }
    static const EXVector &RGBAMult() { return ms_RenderInfo.RGBAMult(); }
    static const EXVector &AmbientColor() { return ms_RenderInfo.AmbientColor(); }
    static u32 DynLightCount() { return ms_DynLightCount; }
    static const EXVector &DynLightRGBMult() { return *(const EXVector *)&ms_DynLightRGBMult; }
    static s8 DynLightRGBMultEnabled() { return ms_DynLightRGBMultEnable; }
    static s8 ZReadDisabled() { return ms_RenderInfo.ZReadDisabled(); }
    static s8 ZWriteDisabled() { return ms_RenderInfo.ZWriteDisabled(); }
    static s8 RGBMultEnabled() { return ms_RenderInfo.RGBMultEnabled(); }
    static s8 AlphaMultEnabled() { return ms_RenderInfo.AlphaMultEnabled(); }
    static s8 ShadowDrawDisabled() { return ms_RenderInfo.ShadowDrawDisabled(); }
    static s8 RefractionRender() { return ms_RenderInfo.RefractionRender(); }
    static void SetDynLights(const EXBoundsBox &box);
    static EXRenderInfo &GRenderInfo() { return ms_GRenderInfo; }  // appended (CameraScript.obj dtor clears its mult bits)
    static s8 RenderScoopCollision() { return ms_RenderScoopCollision; }  // EXPhysics6D.obj
    static s8 RenderPaths() { return ms_RenderPaths; }  // FX_AirTrafficHandler.obj [fi]
protected:
    static EXRenderInfo ms_RenderInfo;
    static EXRenderInfo ms_GRenderInfo;
    static EXRenderFogInfo ms_OverrideFogInfo;
    static u16 ms_RefractTexHeight;
    static u16 ms_RefractTexWidth;
    static s8 ms_RenderPortals;
    static s8 ms_RenderLights;
    static s8 ms_RenderSounds;
    static s8 ms_RenderBspGrid;
    static s8 ms_RenderGrabPoints;
    static s8 ms_RenderMapZones;
    static s8 ms_RenderEntityBounds;
    static s8 ms_RenderScoopCollision;
    static s8 ms_RenderPaths;
    static s8 ms_RenderAnimBones;
    static s8 ms_RenderDynLights;
    static s8 ms_RenderClipBounds;
    static s8 ms_RenderNavMesh;
    static s8 ms_RenderNavConnections;
    static s8 ms_RenderNavPaths;
    static u32 ms_ContrastModifier;
    static u32 ms_RenderDrawType;
    static u32 ms_RenderFlashingBackground;
    static u32 ms_RenderItemDir;
    static s8 ms_DistanceFade;
    static u8 ms_DistanceFadeLayerMask;
    static float ms_DistanceFadeStart;
    static float ms_DistanceFadeEnd;
    static EXRenderFogInfo ms_RenderFogInfo;
    static s8 ms_ReflectionRender;
    static float ms_ReflectionHeight;
    static EXLightManager *ms_pLightManager;
    static EXDynLightData *ms_DynLights[4];
    static u32 ms_DynLightCount;
    static s8 ms_DynLightRGBMultEnable;
    static EXMaths::vec_xyzw ms_DynLightRGBMult;
    static u32 ms_RenderState;
    static s8 ms_WideScreen;
    static s8 ms_ShadowRender;
    static s8 ms_RenderCameras;
};

// Platform render environment (XBRenderEnv.obj): adds the fog watcher items.
class EXRenderEnv : public EXBaseRenderEnv {
public:
    EXRenderEnv();
    virtual ~EXRenderEnv();
    virtual void RegisterWatcherVars();
};
extern EXRenderEnv g_RenderEnv;
#endif
