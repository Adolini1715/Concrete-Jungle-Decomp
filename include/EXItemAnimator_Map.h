#ifndef EXITEMANIMATOR_MAP_H
#define EXITEMANIMATOR_MAP_H
#include "EXGeoRenderEnv.h"
#include "EXItemAnimator.h"
#include "EXItemRender_Entity.h"
#include "EXSoundTagStandIn.h"
#include "EXMapOn.h"
#include "EXArray.h"
#include "EXu32Array.h"
#include "EXFlag.h"
#include "EXBit128.h"
#include "EXCluster.h"
#include <string.h>

class EXGeoHeader;
class EXGeoZone;
class EXGeoMap;
struct EXGeoPortal;
struct EXGeoPlacement;
struct EXGeoSound;


// Interior sound slot of a map zone table (sizeof 0x30). TODO layout: +0x10/+0x14.
class EXISoundTag : public EXSoundTag {
public:
    EXISoundTag() { m_status = 0; }
    const EXVector &Position() const { return m_pos; }
    void SetMapOn(EXMapOn m) { m_mapOn = m; }
    EXMapOn MapOn() const { return m_mapOn; }
    u32 Time() const { return m_time; }
    void SetTime(int t) { m_time = t; }
    u32 Mask() const { return m_mask; }
    void SetMask(u32 m) { m_mask = m; }
    u32 Recursion() const { return m_recursion; }
    void SetRecursion(u32 r) { m_recursion = r; }
    u32 Status() const { return m_status; }
    void SetStatus(u32 s) { m_status = s; }
    u32 SfxHash() const { return m_sfxHash; }
    void SetSfxHash(u32 h) { m_sfxHash = h; }
    EXVector m_pos;         // +8
    EXMapOn m_mapOn;        // +0x18
    u16 m_1a;
    u32 m_time;             // +0x1c
    u32 m_recursion;        // +0x20
    u32 m_mask;             // +0x24
    u32 m_status;           // +0x28 (2 tracking, 3 stop)
    u32 m_sfxHash;          // +0x2c
};

// A placed entity in a map (sizeof 0x9c). TODO layout: +0x9a.
class EXMapPlacement {
public:
    EXMapPlacement();
    ~EXMapPlacement();
    void Initialise(const EXGeoHeader *pHdr, const EXGeoPlacement *pPlace, int zone);
    void InitialiseGDiMemory(const EXGeoHeader *pHdr);
    void Initialise_BoundsBox(const EXGeoHeader *pHdr);
    void RemoveGDiMemory();
    void DisablePlacement(s8 b);
    s8 GetDatum(u32 type, EXDatumTemp &d, u32 flags);
    void DoEntityScoop(EXCollisionScoopTemp &t, u32 flags);
    u32 HashRef() const { return m_hashRef; }
    // Accessors emitted in EXGeoMap.obj.
    const EXGDiDataArray &GDiDataArray(int i) const { return m_render.m_gdi[i]; }
    const EXBoundsBox &BoundsBox() const { return m_bounds; }
    const EXMatrix &mtx() const { return m_mtx; }
    u32 ObjectRef() const { return m_entity; }
    ex::EXFlag<u16> &EngineFlags() { return *(ex::EXFlag<u16> *)&m_90; }
    u16 Zone() const { return m_92; }
    u16 LightSet() const { return m_94; }
    s16 Group() const { return m_96; }
    EXVector Position() const { return EXVector(m_mtx.r[3].x, m_mtx.r[3].y, m_mtx.r[3].z, m_mtx.r[3].w); }
    EXMatrix m_mtx;                 // +0
    EXBoundsBox m_bounds;           // +0x40
    EXItemRender_Entity m_render;   // +0x60
    u32 m_entity;                   // +0x80 current entity hash (-1: disabled)
    u32 m_baseEntity;               // +0x84
    EXGeoFile *m_pGeoFile;          // +0x88
    u32 m_hashRef;                  // +0x8c
    u16 m_90, m_92, m_94, m_96;     // +0x90
    u16 m_zone;                     // +0x98
    u16 m_9a;
};

// TODO layout: per-view zone list head at the start of EXMapZoneInfo (self-referencing +8).
struct EXZoneViewList {
    void Reset()
    {
        m_0c = 0;
        m_10 = 0;
        m_04 = 0;
        m_00 = 0;
        m_08 = this;
    }
    u32 m_00;
    u32 m_04;
    void *m_08;
    u32 m_0c;
    u32 m_10;
};

// Per-sub-zone render data (sizeof 8).
class EXMapSubZoneInfo : public EXGDiDataArray {
public:
    EXGDiDataArray &GDiArray() { return *this; }  // EXItemEnv.obj
};
class EXMapSubZoneInfoArray : public EXArray<EXMapSubZoneInfo, const EXMapSubZoneInfo &> {
public:
    ex::EXBit128 &LoadedState() { return m_loaded; }  // EXItemEnv.obj
    ex::EXBit128 m_loaded;          // +0x14 which sub-zone sections are loaded
};

// Screen-space window a zone is seen through (sizeof 0x18). TODO names of the last two fields.
struct EXViewPortal {
    EXViewPortal() {}
    EXViewPortal &operator=(const EXViewPortal &o)
    {
        m_x0 = o.m_x0;
        m_y0 = o.m_y0;
        m_x1 = o.m_x1;
        m_y1 = o.m_y1;
        m_10 = o.m_10;
        m_14 = o.m_14;
        return *this;
    }
    float m_x0;     // +0x00
    float m_y0;     // +0x04
    float m_x1;     // +0x08 display width for the root portal
    float m_y1;     // +0x0c display height
    float m_10;
    float m_14;
};

// One zone of a map (sizeof 0x88). TODO layout.
class EXMapZoneInfo {
public:
    EXMapZoneInfo();
    ~EXMapZoneInfo();
    s8 SetZoneLoaded(const EXGeoHeader *pHdr, s8 b);
    void UpdateZoneLoaded(const EXGeoHeader *pHdr);
    void SetLoaded(s8 b) { m_loaded = b; }
    void SetSoundable(s8 b) { m_soundable = b; }
    void SetMapOn(EXMapOn m) { m_mapOn = m; }
    // Accessors emitted in EXItemEnv.obj.
    s8 IsVisible() const { return m_view; }
    s8 IsLoaded() const { return m_loaded; }
    s8 IsSoundable() const { return m_soundable; }
    void SetVisible(s8 b) { m_view = b; }
    s8 IsViewVisible() const { return m_viewNext; }
    s8 IsViewSoundable() const { return m_soundNext; }
    void SetViewVisible(s8 b) { m_viewNext = b; }
    void SetViewSoundable(s8 b) { m_soundNext = b; }
    EXMapOn MapOn() const { return m_mapOn; }
    EXODList &ChildList() { return *(EXODList *)&m_viewList.m_0c; }
    EXODListItem &Link() { return *(EXODListItem *)this; }
    EXMapSubZoneInfoArray &MapSubZoneInfoArray() { return m_subZones; }
    EXGeoRenderEnv &DrawRender() { return m_drawRender; }
    EXViewPortal &ViewPortal() { return m_viewPortal; }
    void SetViewPortal(const EXViewPortal &vp) { m_viewPortal = vp; }
    EXZoneViewList m_viewList;      // +0
    EXGeoRenderEnv m_baseRender;    // +0x14 the zone's own fog (from its identifier)
    EXGeoRenderEnv m_drawRender;    // +0x2c fog the zone is drawn with
    EXMapSubZoneInfoArray m_subZones;   // +0x44 (0x24 bytes incl. the loaded-state bits)
    EXMapOn m_mapOn;                // +0x68
    s8 m_loaded;                    // +0x6a
    s8 m_view;                      // +0x6b
    s8 m_soundable;                 // +0x6c
    s8 m_viewNext;                  // +0x6d
    u8 m_soundNext;                 // +0x6e
    u8 m_6f;
    EXViewPortal m_viewPortal;      // +0x70
};

class EXZoneInfoArray : public EXArray<EXMapZoneInfo, const EXMapZoneInfo &> {
};
// TODO layout: placement group (sizeof unknown, only the first float is constructed).
struct EXPlacementGroup {
    EXPlacementGroup() { m_LightSet = 0.0f; }
    float m_LightSet;       // light-set offset added to member placements' light sets
};
class EXPlacementGroupArray : public EXArray<EXPlacementGroup, const EXPlacementGroup &> {
};
// TODO stand-in: objects owned by the zone table (deleted through a virtual destructor).
class EXMapOwnedStandIn {
public:
    virtual ~EXMapOwnedStandIn();
};
class EXMapPlacementArray : public EXArray<EXMapPlacement, const EXMapPlacement &> {
};

// 256 zone-occlusion bits.
struct EXOcclusionBits {
    void ClrAll() { memset(m_bits, 0, sizeof(m_bits)); }
    u32 m_bits[8];
};

// Zone/placement table of a map (sizeof 0x218). TODO layout: +0..0x73 (zone, placement and
// group arrays, clusters, sound handles).
class EXMapZoneInfoTable {
public:
    EXMapZoneInfoTable();
    ~EXMapZoneInfoTable();
    void Initialise(EXGeoFile *pFile, u32 hash);
    s8 InitialiseMapZonePlacements(const EXGeoHeader *pHdr, const EXGeoZone *pZone, int zone);
    s8 UpdateMapZoneLoaded(EXGeoHeader *pHdr, EXGeoMap *pGeo);  // EXItemEnv.obj call
    void ViewInitialise();
    void ViewReset();
    void ViewProcess();
    int GetMapPlacementIndex(u32 hash) const;
    int GetMapPlacementIndex(const EXVector &p) const;
    EXMapPlacement *GetMapPlacement(u32 hash) const;
    EXMapPlacement *GetMapPlacement(const EXVector &p) const;
    void UpdateTrackingSounds(u32 mask);
    s8 UpdateISound(const EXGeoSound *pSnd, EXISoundTag &tag, EXVector pos, s8 bSmooth) const;
    void AddISound(EXGeoMap *pGeo, const EXGeoPortal *pPortal, EXMapOn m, u32 mask, int n);
    void UpdateZoneSounds(const EXGeoMap *pGeo, const EXGeoZone *pZone);
    void UpdateSounds();
    void SetOcclusionFlags(const u32 *pFlags, s8 bInit);
    void ClrOcclusionFlags();
    // Accessors emitted in EXGeoMap.obj.
    EXClusterArray &ClusterArray() { return m_clusters; }
    EXu32Array &PlacementUsedArray() { return *(EXu32Array *)&m_placementUsed; }
    EXMapPlacementArray &PlacementArray() { return m_placements; }
    EXPlacementGroupArray &PlacementGroupArray() { return m_groups; }
    // Accessors emitted in EXItemEnv.obj.
    EXZoneInfoArray &ZoneInfoArray() { return m_zones; }
    EXPtrArray<EXItemAnimator> &SkyPlacementInfoArray() { return *(EXPtrArray<EXItemAnimator> *)&m_owned; }
    EXMapZoneInfo *MapZoneInfo(int i) { return &m_zones[i]; }
    s8 IsZoneVisible(int i) const { return m_zones[i].IsVisible(); }
    s8 IsZoneSoundable(int i) const { return m_zones[i].IsSoundable(); }
    s8 IsZoneLoaded(int i) const { return m_zones[i].IsLoaded(); }
    s8 IsViewVisible(int i) const { return m_zones[i].IsViewVisible(); }
    s8 IsViewSoundable(int i) const { return m_zones[i].IsViewSoundable(); }
    void SetViewVisible(int i, s8 b) { MapZoneInfo(i)->SetViewVisible(b); }
    s8 IsOcclusionFlagSet(int i) const { return (m_occlusion.m_bits[i / 32] & (1 << (i & 31))) != 0; }
private:
    void InitPlacementUsedArray();
public:
    EXGeoFile *m_pGeoFile;              // +0
    u32 m_mapHash;                      // +4
    EXZoneInfoArray m_zones;            // +8
    EXMapPlacementArray m_placements;   // +0x1c
    EXDataArray<u32, u32> m_placementUsed;  // +0x30
    EXClusterArray m_clusters;          // +0x40
    EXPlacementGroupArray m_groups;     // +0x48
    EXPtrArray<EXMapOwnedStandIn> m_owned;  // +0x5c
    EXSoundTag m_sounds;                // +0x6c
    EXISoundTag m_iSounds[8];       // +0x74
    EXOcclusionBits m_occlusion;    // +0x1f4
    u8 m_occlusionState;            // +0x214 0 none, 1 first set, 2 combined
    u8 m_215[3];
};

// TODO stand-in: member at +0x32c whose destructor resets it to -1.
struct EXMapHandleStandIn {
    ~EXMapHandleStandIn() { m_v = 0xffffffff; }
    u32 m_v;
};

// Animator rendering and colliding a geo map (sizeof 0x330).
class EXItemAnimator_Map : public EXItemAnimator {
    EX_DECLARE_DYNAMIC(EXItemAnimator_Map)
public:
    virtual ~EXItemAnimator_Map();
    EX_DECLARE_ITEMTYPE(EXItemAnimator_Map)
    virtual void DoUpdate();
    virtual u32 DoRender(const EXMatrix &mtx, const EXRenderInfo &info, u32 flags);
    virtual s8 Initialise(EXGeoFile *pFile, u32 hash);
    virtual s8 DoGrabPoint(const EXMatrix &mtx, EXGrabPointTemp &t, u32 flags);
    virtual s8 DoGrabPoint(const EXMatrix &mtx, EXGrabPointArrayTemp &t, u32 flags);
    virtual void DoRayCast(const EXMatrix &mtx, EXRayCastTemp &t, u32 flags);
    virtual void DoShadowCast(EXShadowTemp &t);
    virtual void DoEntityScoop(const EXMatrix &mtx, EXCollisionScoopTemp &t, u32 flags);
protected:
    virtual s8 _GetBoundsBox(EXBoundsBox &box, u32 flags);
public:
    EXItemAnimator_Map() {}
    static EXItemAnimator_Map *CreateObject();
    static EXItemAnimator_Map *NewItem() { return new EXItemAnimator_Map; }
    void GetGeoBoundsBox(EXBoundsBox &box);
    // Accessors emitted in EXItemEnv.obj.
    void SetMapId(int id) { m_32c.m_v = id; }
    int MapId() const { return m_32c.m_v; }
    EXMapZoneInfoTable *MapZoneInfoTable() { return &m_zoneTable; }
    EXMapZoneInfo *MapZoneInfo(int i) { return &m_zoneTable.m_zones[i]; }
    s8 IsZoneLoaded(int i) const { return m_zoneTable.m_zones[i].m_loaded; }
    s8 IsZoneVisible(int i) const { return m_zoneTable.m_zones[i].m_view; }
    s8 IsZoneSoundable(int i) const { return m_zoneTable.m_zones[i].m_soundable; }

    EXMapZoneInfoTable m_zoneTable;         // +0x104
    EXPtrArray<EXItemAnimator> m_animators; // +0x31c TODO name
    EXMapHandleStandIn m_32c;               // +0x32c TODO
};
#endif
