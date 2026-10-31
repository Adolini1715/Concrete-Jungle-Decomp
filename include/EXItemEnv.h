#ifndef EXITEMENV_H
#define EXITEMENV_H
#include "types.h"
#include "EXArray.h"
#include "EXList.h"
#include "EXRenderInfo.h"
#include "EXEffectsSystems.h"
#include "EXMapOn.h"
#include "EXItemRender_Anim.h"
#include "EXCollisionType.h"

// Real layout of the item environment (EXItemEnv.obj). Only EXItemEnv.cpp uses this header so far;
// other units still use EXItemEnvStandIn.h (same mangled names, opaque layout).

class EXItem;
class EXItemAnimator;
class EXItemAnimator_Map;
class EXItemEnvView;
class EXLightManager;
class EXDynLight;
class EXCloth2Manager;
class EXSoundListener;
class EXMapZoneInfo;
struct EXViewPortal;
struct EXGeoPortalInfo;
class EXMapPlacement;
class EXGeoMap;
class EXGeoPath;
class EXGeoLight;
struct EXGeoCamera;
struct EXGeoSpecial;
struct EXGeoPortal;
struct EXGeoIdentifier;
struct EXGeoTrigScriptHeader;
struct EXGeoTriggerDatum;
class EX6DManager;
class EXGeoPathArray;
class EXGeoTrigHeaderArray;
class EXGeoFile;
class EXGeoHeader;
class EXMatrix;
class EXVector;
class EXCamera;
class EXBaseItem;
struct EXShadowTemp;
struct EXGrabPointTemp;
struct EXGrabPointArrayTemp;
struct EXRayCastTemp;
class EXCollisionScoopTemp;
struct EXLightInfo;
namespace ex {
struct XRGBA;
}

// Per-frame validity: m_flags is cleared the first time it is checked in a new frame (sizeof 8).
class EXFrameValidFlags {
private:
    s8 CheckIsValidFrame();
public:
    u32 m_flags;    // +0
    u32 m_frame;    // +4 g_Stats frame of the last check
};

// A loaded map: geo reference, user value and the animator (sizeof 0x10).
class EXItemMapRef {
public:
    void Initialise(const EXGeoRefObject &r, int n);
    void DeleteMap();
    const EXGeoRefObject &RefObject() const { return m_ref; }
    int MapId() const { return m_08; }
    EXItemAnimator_Map *ItemMap() const { return m_pMap; }
    void SetItemMap(EXItemAnimator_Map *p) { m_pMap = p; }
    EXGeoRefObject m_ref;           // +0
    int m_08;
    EXItemAnimator_Map *m_pMap;     // +0xc
};

// A registered view of the environment: camera, visible / soundable zone lists, sound listener
// (sizeof 0x6c).
class EXItemEnvView {
public:
    EXItemEnvView(s8 b);
    ~EXItemEnvView();
    void UpdateSoundListener(const EXVector &pos, const EXVector &vel, const EXVector &front, const EXVector &top,
                             const EXVector &right);
    void UpdateSoundListener(const EXVector &pos, const EXVector &vel, const EXMatrix &mtx);
    void UpdateCameraPosition(const EXCamera &cam);
    EXPtrArray<void> &ZonePtrArray(int i) { return m_lists[i]; }
    const EXVector &CameraPosition() const { return m_camPos; }
    EXDataArray<EXVector, EXVector &> &ZoneLoadInfo() { return m_4c; }
    EXDataArray<EXCollisionType, EXCollisionType &> &SubZoneLoadInfo() { return m_5c; }
    EXVector m_camPos;                      // +0
    EXVector m_camDir;                      // +0x10
    EXPtrArray<void> m_lists[2];            // +0x20 visible, soundable zones
    EXSoundListener *m_pSoundListener;      // +0x40
    EXBaseItem *m_p44;                      // +0x44 TODO type (deleted through slot 0)
    u8 m_48;
    EXDataArray<EXVector, EXVector &> m_4c;                 // +0x4c zone load info
    EXDataArray<EXCollisionType, EXCollisionType &> m_5c;   // +0x5c sub-zone load info
};

// The scene environment: maps, item lists and groups, views, lights (sizeof 0xa4).
class EXItemEnv {
public:
    virtual ~EXItemEnv();
    virtual void Initialise() = 0;
    virtual void StartMusic(u32 hash, u32 flags, int a, int b);
    virtual void UpdateItems_UpdateCode();
protected:
    virtual EXItemAnimator *CreateSky(EXItemAnimator_Map *pMap, u32 hash, const EXMapZoneInfo *pZone) const;
    virtual void ProcessSky(EXItemAnimator *pSky) const;
    virtual void RenderSky(EXItemAnimator *pSky, const EXMatrix &mtx, const EXRenderInfo &info, const EXMapZoneInfo *pZone) const;
public:
    EXItemEnv(u32 a, u32 b);
    static EXItemEnv *GetItemEnv() { return m_pTheItemEnv; }
    const EXPtrArray<EXItemAnimator_Map> &ActiveMapArray() const { return m_activeMaps; }
    const EXDataArray<EXItemMapRef, const EXItemMapRef &> &MapRefArray() const { return m_mapRefs; }
    unsigned int ViewArraySize() const { return m_views.Size(); }
    EXItemEnvView *const *ViewArray() const { return (EXItemEnvView *const *)m_views.GetData(); }
    EXDList &ItemList(int i) { return m_pItemLists[i]; }
    EXODList &ItemGroup(int i) { return m_pItemGroups[i]; }
    u8 NumItemGroupLists() const { return m_numItemGroups; }
    EXRenderInfo &RenderInfo() { return m_renderInfo; }
    static void Shutdown();
    void UpdateClothManager();
    void AddItem(EXItem *p, u32 list);
    void AddItem(EXItem *p, u32 list, u32 sort);
    EXItemAnimator_Map *FindMap(EXGeoFile *pFile, u32 hash);
    EXItemAnimator_Map *GetMapItem(s8 map) const;
    EXItemAnimator_Map *GetMapItem(EXMapOn m) const
    {
        s8 map = (s8)(m.m_v >> 8);
        if (map > 0) return m_mapRefs.GetData()[map].m_pMap;
        EXItemAnimator_Map *const *pp = (EXItemAnimator_Map *const *)m_activeMaps.GetData();
        if (!pp) return 0;
        return *pp;
    }
    EXGeoMap *GetGeoMap(EXMapOn m) const;  // inline, defined in EXItemEnv.cpp
    EXMapOn GetMapOn(const EXVector &p) const;
    EXMapOn GetMapOn(u32 hash) const;
    EXGeoPath *GetPath(const EXVector &p, u32 flags) const;
    EXGeoPath *GetPath(int hash) const;  // appended by the vehicles agent (Traffic.obj)
    EXGeoCamera *GetCamera(int i) const;
    EXGeoSpecial *GetSpecial(int i) const;
    EXGeoLight *GetLight(int i) const;
    EXGeoTrigScriptHeader *GetTriggerScript(EXMapOn m, int i) const;
    EXGeoTriggerDatum *GetTriggerCollision(EXMapOn m, int i) const;
    int GetPathIndex(u32 hash) const;
    int GetPathIndex(const EXVector &p, u32 flags) const;
    EXMapPlacement *GetMapPlacement(u32 hash) const;
    EXGeoCamera *GetCamera(const EXVector &p) const;
    int GetCameraIndex(u32 hash) const;
    int GetCameraIndex(const EXVector &p) const;
    EXGeoSpecial *GetSpecial(const EXVector &p) const;
    int GetSpecialIndex(u32 hash) const;
    int GetSpecialIndex(const EXVector &p) const;
    EXGeoPortal *GetPortal(const EXVector &p) const;
    int GetPortalIndex(const EXVector &p) const;
    EXGeoLight *GetLight(const EXVector &p) const;
    int GetLightIndex(u32 hash) const;
    int GetLightIndex(const EXVector &p) const;
    EXGeoPathArray *GetPaths(EXMapOn m) const;
    EXGeoTrigHeaderArray *GetTriggerArray(EXMapOn m) const;
    s8 IsZoneVisible(EXMapOn m) const;
    s8 IsZoneVisible(const EXVector &p, float r) const;
    s8 IsZoneSoundable(EXMapOn m) const;
    s8 IsZoneLoaded(EXMapOn m) const;
    s8 IsPortalVisible(EXMapOn m, short portal) const;
    s8 IsPortalVisible(int i) const;
    EXCloth2Manager *ClothManager();
    void RemoveAllNonPersistentItems();
    float MinDistanceToItemViews(const EXVector &p);
    void UpdateItems_LastFrame();
    void UpdateItems_Collision() const;
    EXGeoPortal *GetPortal(int i) const;
    void ResetEnvViews();
    void UpdateItems();
    EXGeoIdentifier *GetZoneIdentifier(EXMapOn m) const;
    void ProcessEnvViews();
    EXItemEnvView *RegisterEnvView(s8 b);
    void ReleaseEnvView(EXItemEnvView *pView);
    void Render(EXItemEnvView *pView);
    void UpdateEnvView(EXItemEnvView *pView, EXCamera &cam, const EXMatrix &mtx);
    void RenderShadows() const;
    void RenderReflections() const;
    EXVector CalcLightsV(const EXVector &p, const EXVector &n, s8 b, EXMapOn m) const;
    EXVector CalcLightsV(EXLightInfo &info, const EXVector &p, const EXVector &n, s8 b, EXMapOn m) const;
    EXVector CalcLightsV(EXLightInfo *pInfo, int num, const EXVector &p, const EXVector &n, s8 b, EXMapOn m) const;
    ex::RGBA CalcLights(const EXVector &p, const EXVector &n, s8 b, EXMapOn m) const;
    void RemoveItem(EXItem *p);
    void RemoveAllAddedItems();
    void UpdateItems_Physics() const;
    void UpdateItems_InterObjCollision();
    EXDynLight *NewDynLight();
    EXDynLight *NewPointDynLight(ex::XRGBA c, const EXVector &pos, float inner, float outer);
    EXDynLight *NewPointDynLight(ex::XRGBA c, float inner, float outer);
    EXDynLight *NewDirPointDynLight(ex::XRGBA c, const EXVector &pos, const EXVector &dir, float inner, float outer);
    EXDynLight *NewDirPointDynLight(ex::XRGBA c, const EXVector &dir, float inner, float outer);
    void RemoveDynLight(EXDynLight *p) const;
    void RemoveAllDynLights() const;
    void SetDynLightRGBMult(const EXVector &v);
    void SetDynLightRGBMult(float r, float g, float b);
    void EnableDynLightRGBMult(s8 b);
    EXItemAnimator_Map *GetMapItem(const EXVector &p) const;
    EXGeoMap *GetGeoMap(const EXVector &p) const;
    EXMapPlacement *GetMapPlacement(const EXVector &p) const;
    void DoShadowCastToMap(EXShadowTemp &t) const;
    void DoCollisionScoopToContoured(EXCollisionScoopTemp &t, EXItem *pItem) const;
    void DoGrabPointToContoured(EXGrabPointTemp &t) const;
    void DoGrabPointToContoured(EXGrabPointArrayTemp &t) const;
    void DoRayCastToContoured(EXRayCastTemp &t) const;
    void DoCollisionScoopToMap(EXCollisionScoopTemp &t) const;
    void DoGrabPointToMap(EXGrabPointTemp &t) const;
    void DoGrabPointToMap(EXGrabPointArrayTemp &t) const;
    void DoRayCastToMap(EXRayCastTemp &t, u32 flags) const;
    void RemoveAllMaps();
    void RemoveMap(EXGeoFile *pFile, u32 hash);
    EXMapOn AddGeoMap(const EXGeoHeader *pHdr, const EXGeoMap *pMap);
    EXItemAnimator_Map *AddMap(EXGeoFile *pFile, u32 hash, EXItemAnimator_Map *pMap);
protected:
    int CheckPortalToView(const EXGeoPortal *pPortal, const EXGeoPortalInfo *pInfo, const EXViewPortal &vp, EXViewPortal &out) const;
    void RecursePortal(EXItemAnimator_Map *pMap, EXMapZoneInfo *pZone, const EXViewPortal &vp, int depth);
    void RecurseSfxPortals(EXItemAnimator_Map *pMap, EXMapZoneInfo *pZone, int depth, u32 mask);
    static u32 GeoMapOnIndex(u32 index, u32 map) { return (map << 16) | index; }
    static u32 GeoMapOnCode(u32 code) { return code & 0xff00ffff; }
    static u32 GeoMapOnMapId(u32 code) { return (u8)(code >> 16); }
    void ProcessSounds();
    EXMapOn GetConnectingMapOn(const EXItemAnimator_Map *pFrom, const EXGeoPortal *pPortal) const;
    EXGeoMap *GetGeoMap(s8 map) const;
    void UpdateDynLights();
    void ProcessMapZones(s8 b);
    void RenderItems();
    void RenderMaps(EXItemEnvView *pView);
    void RenderSkys(const EXItemEnvView *pView, EXMapZoneInfo *pZone) const;
    void DecodeActiveList(EXItemEnvView *pView, EXMapZoneInfo *pZone, int list);
    void BuildUsageList(EXItemEnvView *pView, int list);
    void CalculatePortalVisability(const EXItemEnvView *pView);
    void CalculatePortalSoundability();
    void UpdateView(EXItemEnvView *pView);
    s8 IsViewVisible(EXMapOn m) const;
    s8 IsViewSoundable(EXMapOn m) const;
    void ProcessSkys();
    void InitialiseViewTables();
    void ResetViewTables();
    void ProcessViewTables();
    void ResetEnvViewTables();
public:
    EXDataArray<EXItemMapRef, const EXItemMapRef &> m_mapRefs;  // +0x04
    EXPtrArray<EXItemAnimator_Map> m_activeMaps;                // +0x14
    EXPtrArray<EXItemEnvView> m_views;                          // +0x24
    EXPtrArray<void> m_34;                                      // +0x34
    EXODList m_44;                                              // +0x44 zones to decode
    EXRenderInfo m_renderInfo;                                  // +0x4c
    EXLightManager *m_pLightManager;                            // +0x7c
    EX6DManager *m_p6DManager;                                  // +0x80
    EXCloth2Manager *m_pClothManager;                           // +0x84
    EXEffectsSystems m_effects;                                 // +0x88
    EXDList *m_pItemLists;                                      // +0x90 m_numItemLists lists
    EXODList *m_pItemGroups;                                    // +0x94 m_numItemGroups groups
    u8 m_98;
    u8 m_numItemLists;                                          // +0x99
    u8 m_numItemGroups;                                         // +0x9a
    u8 m_9b;
    u32 m_9c;
    s32 m_a0;
    static EXVector m_MapSoundPosition;
protected:
    static EXItemEnv *m_pTheItemEnv;
};
#endif
