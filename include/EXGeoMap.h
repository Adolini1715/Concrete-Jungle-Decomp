#ifndef EXGEOMAP_H
#define EXGEOMAP_H
#include "EXGeoCommonObject.h"
#include "EXBounds.h"
#include "EXRelPtr.h"
#include "EXGeoTrigger.h"
#include "EXGeoCommonArray.h"
#include "EXArray.h"
#include "EXVector.h"
#include "RGBA.h"
#include "EXGeoFace.h"

struct EXGeoPathNode;

namespace ex { class RGBA; }
class EXVector;
class EXLineSeg;
struct EXGeoIdentifier;
// A map light picked for an item (EXGeoMap::CalcLightsV, sizeof 0x2c).
struct EXLightInfo {
    EXVector m_Position;        // 0x00 light position, w 0
    EXVector m_Colour;          // 0x10 colour (0-255) x strength / 128; w strength
    u32 m_Hash;                 // 0x20 light hash
    u32 m_Flags;                // 0x24 light flags
    s8 m_Valid;                 // 0x28
    s8 m_29;
};
class EXMapZoneInfoTable;
class EXMapZoneInfo;
class EXRenderInfo;
struct EXViewPortal;
class EXMatrix;

// NEAR (FindIndex, 11 bytes): retail saves ebp/esi/edi before the hash-count test and keeps a
// separate return -1 block inside that frame.
// Map node list: {count, sorted-hash count, relptr to elements}; each element starts with its
// hash. FindIndex accepts a direct index hash or binary-searches the sorted hashes.
template <class T> class EXGeoNodeArray {
public:
    int Size() const { return m_Size; }
    T *Data() const { return m_Data; }
    T *Data() { return m_Data; }
    template <class I> const T &operator[](I i) const { return Data()[i]; }
    template <class I> T &operator[](I i) { return Data()[i]; }
    int FindIndex(unsigned int hash) const
    {
        int i = EXGeoHashIndex(hash);
        if (i >= 0) return i < Size() ? i : -1;
        if (m_HashSize > 0) {
            int lo = 0;
            int hi = m_HashSize - 1;
            const T *pData = Data();
            int mid = (lo + hi + 1) / 2;
            while (*(const u32 *)&pData[mid] != hash) {
                if (mid == lo) return -1;
                int round;
                if (*(const u32 *)&pData[mid] > hash) {
                    hi = mid;
                    round = 0;
                } else {
                    lo = mid;
                    round = 1;
                }
                mid = (round + lo + hi) / 2;
            }
            return mid;
        }
        return -1;
    }
    s16 m_Size;
    s16 m_HashSize;
    ex::EXRelPtr<T> m_Data;
};

// Node payloads; only the size (list stride) and leading hash are known so far.
class EXGeoBasePath {
public:
    int Value(int i) const;
    unsigned int Flags() const { return *(const u32 *)&m_04[0x10 - 4]; }  // 0x10 (ActorCombat.obj [fi]; 0x4000 = looping)
    const EXVector3 &Position() const { return *(const EXVector3 *)&m_04[0]; }  // 0x04 (Camera.obj SetCamSpline)
    // Path type (0x1c: 0 air traffic, 1 patrol) and its bounds box (0x20); both live in EXGeoPath's tail.
    unsigned short Type() const { return *(const u16 *)((const u8 *)this + 0x1c); }  // FX_AirTrafficHandler.obj [fi]
    const EXBoundsBox &BoundsBox() const { return *(const EXBoundsBox *)((const u8 *)this + 0x20); }
    u32 m_HashCode;
    u8 m_04[0x14];
    ex::EXRelPtr24<s32> m_Values;   // 0x18 value table, low 8 bits: which values are present
};
class EXGeoPath : public EXGeoBasePath {
public:
    void Debug_DrawPath(ex::RGBA c) const;
    void Debug_DrawLinearPath(ex::RGBA c) const;
    int GetPathNode(const EXVector &p) const;  // nearest node (Bullet.obj Create2)
    int NumLinks() const { return *(const int *)&m_1c[0x40 - 0x1c]; }  // 0x40 (Traffic.obj)
    int NumNodes() const { return *(const int *)&m_1c[0x48 - 0x1c]; }  // 0x48 (ActorCombat.obj)
    // Node i (sizeof 0x28) through the self-relative node offset at 0x4c (ActorCombat.obj [fi]).
    EXGeoPathNode *Node(int i) const
    {
        return (EXGeoPathNode *)((char *)(EXGeoPathNode *)*(const ex::EXRelPtr<EXGeoPathNode> *)&m_1c[0x4c - 0x1c] +
                                 i * 0x28);
    }
    u8 m_1c[0x34];
};
// The map's path list (EXItemEnv::GetPaths), elements 0x50.
class EXGeoPathArray : public EXGeoNodeArray<EXGeoPath> {
};
class EXGeoSplitPath {
public:
    EXGeoPath *GetPath(int i) const;
    void Debug_DrawPath(ex::RGBA c) const;
    u8 m_00[0x4c];
    ex::EXRelPtr<ex::EXRelPtr<EXGeoPath> > m_Paths;     // 0x4c
};
// Map light (0x30).
class EXGeoLight {
public:
    void Debug_DrawLight(ex::RGBA c) const;
    unsigned int HashRef() const { return m_HashCode; }
    const EXVector3 &Position() const { return m_Position; }
    unsigned int Flags() const { return m_Flags; }
    ex::RGBA Colour() const { return m_Colour; }
    float Attenuate(const EXVector &p, const EXVector &n, signed char b) const;
private:
    float Attenuate_DirectionAttenuator(const EXVector &p, const EXVector &n, const EXVector &d) const;
    float Attenuate_ParallelAttenuator(const EXVector &p, const EXVector &n, const EXVector &d) const;
    float Attenuate_BeamAttenuator(const EXVector &p, const EXVector &n) const;
    float Attenuate_LinearAttenuator(const EXVector &p, const EXVector &n) const;
public:
    u32 m_HashCode;
    EXVector3 m_Position;       // 0x04
    u32 m_Flags;                // 0x10 bit 0: on
    EXVector3 m_Direction;      // 0x14 beam / parallel direction
    u16 m_Attenuators;          // 0x20 1 linear, 2 direction (when asked), 4 beam, 8 parallel
    u16 m_BeamAngle;            // 0x22 degrees
    u32 m_Colour;               // 0x24 ex::RGBA
    float m_Range;              // 0x28
    float m_Falloff;            // 0x2c fraction of the range at full strength
};
struct EXGeoCamera {
    const EXVector3 &Position() const { return m_Position; }
    u32 m_HashCode;
    EXVector3 m_Position;       // 0x04
    u32 m_10;
    EXVector3 m_Target;         // 0x14 look-at (SetCamFixed mode 7)
    float m_FocalLength;        // 0x20
    float m_24;
    float m_ApertureHeight;     // 0x28
};
struct EXGeoSpecial {
    const EXVector3 &Position() const { return m_Position; }
    u32 m_HashCode;
    EXVector3 m_Position;       // 0x04
    u8 m_10[0x3c];
};
struct EXGeoSound {
    u32 m_HashCode;             // -1: none
    float m_Pos[3];             // 0x04
    u32 m_Flags;                // 0x10 music start flags (bit 7 cleared)
    u32 m_Sound;                // 0x14 bits 24-30 the kind: 0x1a effect, 0x1b music, 0x1c reverb depth
    u32 m_18;
    u8 m_Volume;                // 0x1c effect volume / reverb depth
    u8 m_1d;                    // 0x1d music start parameters
    u8 m_1e;
    u8 m_1f;                    // copied from the sound details (+0x0b)
    float m_MinDist;            // 0x20 TODO names; 0 in the file: take both from the sound details
    float m_MaxDist;            // 0x24
    int m_Zone;                 // 0x28 zone whose soundability gates it (unless flag 0x40)
};
// Sky reference of a map (only the hash is known).
struct EXGeoSky {
    u32 m_HashCode;
};
// The portal's face (EXGeoFace::m_Flags 0x10000: only the owning zone's side culls by depth).
typedef EXGeoFace EXGeoPortalTarget;
// Portal between zones (sizeof 0x44).
struct EXGeoPortal {
    float DistanceTo(const EXVector &p, EXVector &nearest) const;
    float RayCast(const EXLineSeg &l, EXVector &hit) const;
    unsigned int AccumalateVerticies(EXVector &sum) const;
    EXVector Centre() const;
    EXGeoFace *GeoFace() const { return m_Target; }
    u32 m_00;           // 0x00 low word: the zone the portal belongs to
    u16 m_Visited;      // 0x04 cleared by InitPortals; bit 0: open
    u8 m_06[6];
    float m_MinDepth;   // 0x0c >= 1: cull the portal when nearer than this (EXItemEnv.obj CheckPortalToView)
    ex::EXRelPtr<EXGeoFace> m_Target;           // 0x10
    EXFixedArray<EXVector3, 4> m_Verts;         // 0x14
};

// Zone-to-zone link through portals (8 bytes). A group is a head entry plus m_Extra entries;
// the head names the zone, every entry of the group names one portal.
struct EXGeoPortalInfo {
    u16 m_00;           // (head entry) the zone itself
    s16 m_Zone;         // 0x02 (head entry) -1: none
    s16 m_Portal;       // 0x04
    u8 m_Extra;         // 0x06 (head entry) entries after the head
    u8 m_07;
};
typedef EXGeoPortalInfo EXGeoZonePortalRef;

// BSP node (0x20): plane, then child indices (<= 0: leaf, -index is the zone).
struct EXGeoBspNode {
    float m_Plane[4];
    s16 m_Child[2];     // 0x10 [0]: in front, [1]: behind
    u8 m_14[0xc];
};
class EXGeoBspTree {
public:
    // What is swept from a to b: a point, a sphere of radius r, or a box (half extents in a
    // file static set by the box WhichNodes).
    enum Shape { SHAPE_POINT = 0, SHAPE_SPHERE = 1, SHAPE_BOX = 2 };
    int WhichNode(EXMaths::vec_xyzw p) const;
    int WhichNodes(const EXMaths::vec_xyzw &a, const EXMaths::vec_xyzw &b, short *pList) const;
    int WhichNodes(const EXMaths::vec_xyzw &a, const EXMaths::vec_xyzw &b, float r, short *pList) const;
    int WhichNodes(const EXMaths::vec_xyzw &a, const EXMaths::vec_xyzw &b, const EXMaths::vec_xyzw &boxMin,
                   const EXMaths::vec_xyzw &boxMax, short *pList) const;
protected:
    int CheckNodes(EXMaths::vec_xyzw a, EXMaths::vec_xyzw b, short *pList, Shape shape, float r) const;
public:
    EXGeoBspNode m_Nodes[1];
};

// Placed entity record of a map (sizeof 0x3c). TODO layout: names of the u16 pairs.
struct EXGeoPlacement {
    EXMatrix CalcMatrix() const;
    u32 m_hash;             // +0
    float m_pos[3];         // +0x04
    u32 m_10;               // +0x10 TODO
    float m_rot[3];         // +0x14 euler angles (rotation order 6)
    float m_scale[3];       // +0x20
    u16 m_2c, m_2e;         // +0x2c (EXMapPlacement engine flags, zone)
    u32 m_entity;           // +0x30
    u16 m_34, m_36;         // +0x34 (light set, group)
    u32 m_XtraData;         // +0x38 EXGeoXtraData
};
class EXGeoPlacementArray : public EXRelArray<EXGeoPlacement> {
};
struct EXGeoCluster;
struct EXGeoPlacementGroup;

// Placement tree of a zone: each node has either child nodes (with their bounds) or, as a leaf,
// a list of placement indices. Disabled nodes and items (flag 8 clear) are skipped.
struct EXGeoPlacementTree;
struct EXGeoPlacementTreeChild {
    ex::EXRelPtr<EXGeoPlacementTree> m_pNode;   // 0x00
    EXBoundsBox3 m_Box;                         // 0x04
};
struct EXGeoPlacementTreeItem {
    u16 m_Index;    // 0x00 placement index
    u8 m_Flags;     // 0x02 8: enabled
    u8 m_03;
};
struct EXGeoPlacementTree {
    u8 m_NumChildren;   // 0x00 0 for a leaf
    u8 m_Flags;         // 0x01 8: enabled
    u16 m_NumItems;     // 0x02 leaf only
    // 0x04: the children, or for a leaf the items.
    EXGeoPlacementTreeChild &Child(int i) { return ((EXGeoPlacementTreeChild *)(this + 1))[i]; }
    EXGeoPlacementTreeItem &Item(int i) { return ((EXGeoPlacementTreeItem *)(this + 1))[i]; }
};
class EXu32Array;
struct EXRayCastTemp;
void DoRayCast_PlacementTree(EXGeoPlacementTree *pTree, const EXRayCastTemp &t, EXu32Array &out);
void DoCollision_PlacementTree(EXGeoPlacementTree *pTree, const EXVector &p, float r, EXu32Array &out);
struct EXGeoPlacementInfo {
    EXGeoPlacementTree *PlacementTree() const { return m_Tree; }
    int NumPlacements() const { return m_NumPlacements; }  // EXGeoMap.obj
    unsigned short *PlacementList() const { return m_Placements; }
    int m_NumPlacements;                        // 0x00 (EXItemAnimator_Map.obj)
    ex::EXRelPtr<u16> m_Placements;             // 0x04 indices into the map's placement array
    ex::EXRelPtr<EXGeoPlacementTree> m_Tree;    // 0x08
};
struct EXGeoZoneISound {
    u8 m_Bit;       // 0x00 bit in the interior sound mask
    u8 m_01;
    u16 m_Sound;    // 0x02 index into EXGeoMap::m_Sounds
};
// Map zone (0x84).
class EXGeoZone {
public:
    EXGeoZone() {}
    unsigned int HashRef() const { return m_HashRef; }
    void SetSection(unsigned int s) { m_Section = s; }
    // Accessors emitted in EXItemEnv.obj.
    unsigned int SubZoneHashRef() const { return m_00; }
    unsigned int Section() const { return m_Section; }
    int NumPortals() const { return m_NumPortalRefs; }
    const EXGeoPortalInfo *PortalInfo(unsigned int i) const { return &((const EXGeoPortalInfo *)m_PortalRefs)[i]; }
    unsigned int *ZoneSectionList() { return (unsigned int *)&m_30[0]; }
    unsigned int *OcclusionList() { return (unsigned int *)&m_30[0x40 - 0x30]; }
    const EXBoundsBox3 &BoundsBox() const { return *(const EXBoundsBox3 *)&m_30[0x60 - 0x30]; }
    EXGeoPlacementInfo *PlacementInfo() const { return *(const ex::EXRelPtr<EXGeoPlacementInfo> *)&m_20; }
    unsigned int ISoundMask() const { return *(const unsigned int *)&m_30[0x78 - 0x30]; }
    // Interior sounds of the zone: {mask bit, index into the map's sounds} (EXMapZoneInfoTable::AddISound).
    int NumISounds() const { return *(const int *)&m_30[0x7c - 0x30]; }
    const EXGeoZoneISound *ISounds() const { return *(const ex::EXRelPtr<EXGeoZoneISound> *)&m_30[0x80 - 0x30]; }
    EXGeoIdentifier *Identifier() const { return m_Identifier; }
    int NumLights() const { return m_LightList.Size(); }
    unsigned short Lights(unsigned int i) const { return m_LightList[i]; }
    u32 m_00;
    ex::EXRelPtr<EXGeoIdentifier> m_Identifier;     // 0x04
    EXGeoNodeArray<unsigned short> m_LightList;     // 0x08
    EXGeoNodeArray<unsigned short> m_SoundList;     // 0x10 indices into EXGeoMap::m_Sounds (count read as a short)
    int m_NumPortalRefs;                            // 0x18
    ex::EXRelPtr<EXGeoZonePortalRef> m_PortalRefs;  // 0x1c groups: head entry + m_Extra more
    u32 m_20;                   // relptr to the zone's placements (item layer stand-in)
    u32 m_24;
    u32 m_HashRef;              // 0x28
    u32 m_Section;              // 0x2c
    u8 m_30[0x84 - 0x30];
};
// Map: zones, placements, lights, paths (sizeof 0x10c).
class EXGeoMap : public EXGeoCommonObject {
    EX_DECLARE_DYNAMIC(EXGeoMap)
    EX_DECLARE_GEOOBJECT(EXGeoMap)
public:
    virtual void OnCreate(EXGeoHeader *, EXGeoObjectHeader *);
    virtual void OnDelete(EXGeoHeader *, EXGeoObjectHeader *);
    EXGeoMap() {}
    unsigned int NumZones() const { return m_NumZones; }
    EXGeoZone *MapZone(int i) { return &m_Zones[i]; }
    EXRelArray<EXGeoSky> &Skys() { return m_Skys; }
    EXGeoBspTree *BspTree() const { return m_BspTree; }
    EXRelArray<EXGeoCluster> &Clusters() { return m_Clusters; }  // EXItemAnimator_Map.obj
    EXGeoPlacementArray &Placements() { return m_Placements; }  // EXItemAnimator_Map.obj
    EXRelArray<EXGeoPlacementGroup> &PlacementGroups() { return m_PlacementGroups; }
    void Initialise_PlacementTree(EXGeoPlacementTree *pTree, EXGeoHeader *pHdr, EXBoundsBox &box);  // EXItemEnv.obj call
    void Debug_DrawLights();
    void Debug_DrawMapZones(ex::RGBA c);
    void Debug_DrawClusters(EXMapZoneInfoTable *pTable) const;
    int WhichZone(unsigned int hash) const;
    int WhichZone(const EXVector &p) const { return BspTree()->WhichNode(*(const EXMaths::vec_xyzw *)&p); }  // EXItemEnv.obj
    // The zones a segment from a to b passes through (EXItemAnimator_Map.obj copy).
    int WhichZones(const EXVector &a, const EXVector &b, short *pList) const
    {
        EXMaths::vec_xyzw vb = b;
        EXMaths::vec_xyzw va = a;
        return BspTree()->WhichNodes(va, vb, pList);
    }
    EXGeoIdentifier *GetZoneIdentifier(int zone) const;
    int GetPathIndex(unsigned int hash) const;
    int GetCameraIndex(unsigned int hash) const;
    int GetSpecialIndex(unsigned int hash) const;
    int GetLightIndex(unsigned int hash) const;
    EXGeoPath *GetPath(unsigned int hash) const;
    EXGeoCamera *GetCamera(unsigned int hash) const;
    EXGeoSpecial *GetSpecial(unsigned int hash) const;
    EXGeoLight *GetLight(unsigned int hash) const;
    int GetCameraIndex(const EXVector &v) const;
    int GetSpecialIndex(const EXVector &v) const;
    int GetLightIndex(const EXVector &v) const;
    EXGeoCamera *GetCamera(const EXVector &v) const;
    EXGeoSpecial *GetSpecial(const EXVector &v) const;
    EXGeoLight *GetLight(const EXVector &v) const;
    EXGeoPortal *GetPortal(int i) const;
    int GetPortalIndex(const EXVector &v) const;
    EXGeoPortal *GetPortal(const EXVector &v) const;
    int GetPathIndex(const EXVector &v, unsigned int flags) const;
    EXGeoPath *GetPath(const EXVector &v, unsigned int flags) const;
    void InitPortals() const;
    EXBoundsBox BoundsBox(const EXGeoHeader *pHdr) const;
    unsigned int GetMapZoneList(const EXVector &p, float r, unsigned int *pList) const;
    void DrawZone(EXMapZoneInfoTable *pTable, EXMapZoneInfo *pZone, const EXRenderInfo &info);  // EXItemEnv.obj
    void DrawPortals(const EXMapZoneInfo *pZone, const EXRenderInfo &info);
    void DrawZone_XtraRender(const EXGeoPlacement *pPlace, float frames) const;
    static int ClipPortal(const EXViewPortal &vp, const EXBoundsBox &box, const EXMatrix &m);
    unsigned int GetMapZoneList(const EXVector &p, const EXBoundsBox &box, unsigned int *pList) const;
    EXVector CalcLightsV(const EXVector &p, const EXVector &n, signed char b, int zone);  // appended by the item agent
    EXVector CalcLightsV(EXLightInfo &info, const EXVector &p, const EXVector &n, signed char b, int zone);  // EXItemEnv.obj
    EXVector CalcLightsV(EXLightInfo *pInfo, int num, const EXVector &p, const EXVector &n, signed char b, int zone);
    ex::RGBA CalcLights(const EXVector &p, const EXVector &n, signed char b, int zone);
    // Appended by the effects agent (inlines emitted in Trigger.obj).
    EXGeoTriggerType *TriggerTypes(int i) const { return &((EXGeoTriggerInfo *)m_TriggerInfo)->m_Types[i]; }
    EXGeoTrigScriptHeader *TriggerScripts(int i) const { return &((EXGeoTriggerInfo *)m_TriggerInfo)->m_Scripts[i]; }  // EXItemEnv.obj
    EXGeoTriggerDatum *TriggerCollisions(int i) const { return &((EXGeoTriggerInfo *)m_TriggerInfo)->m_Collisions[i]; }

    ex::EXRelPtr<EXGeoBspTree> m_BspTree;       // 0x04
    EXGeoNodeArray<EXGeoPath> m_Paths;          // 0x08
    EXGeoNodeArray<EXGeoLight> m_Lights;        // 0x10
    EXGeoNodeArray<EXGeoCamera> m_Cameras;      // 0x18
    EXGeoNodeArray<EXGeoSpecial> m_Specials;    // 0x20
    EXRelArray<EXGeoCluster> m_Clusters;        // 0x28
    EXGeoNodeArray<EXGeoSound> m_Sounds;        // 0x30
    EXRelArray<EXGeoPortal> m_Portals;          // 0x38
    EXRelArray<EXGeoSky> m_Skys;                // 0x40
    EXGeoPlacementArray m_Placements;           // 0x48
    EXRelArray<EXGeoPlacementGroup> m_PlacementGroups;  // 0x50
    ex::EXRelPtr<EXGeoTriggerInfo> m_TriggerInfo;   // 0x58 (appended by the effects agent; Trigger.obj inlines)
    u8 m_5c[0x6c - 0x5c];
    EXBoundsBox3 m_BoundsBox;                   // 0x6c
    u32 m_NumZones;             // 0x84
    EXGeoZone m_Zones[1];       // 0x88
};

#endif
