#ifndef ACTORLOCATION_H
#define ACTORLOCATION_H
#include "types.h"
#include "EXVector.h"
#include "StateMachine.h"

// Points of interest (ActorLocation.obj). Layouts recovered from the POI_* accessors.
class EXTrigger;
class XGeoNavMeshEntity;
#include "EXList.h"
struct LocCount {
    // POI_GetActorCount mask bits.
    enum Flags {
        BOUND = 1,          // count binds with m_bBound set
        UNBOUND = 2,        // count binds whatever m_bBound says
        VENDORS = 4,
        CUSTOMERS = 8
    };
};
// One stand slot of a location (sizeof 0x2c). Vendors use slots 0..3, customers 4.. .
struct POI_BIND {
    POI_BIND() {}
    EXVector m_Pos;         // 0x00
    EXVector m_Offset;      // 0x10 stand datum position in the HT_File_StandPatterns pattern (w 0)
    int m_ID;               // 0x20 bound actor id
    int m_Poly;             // 0x24 nav poly of m_Pos
    s8 m_bBound;            // 0x28 actor has arrived
    s8 m_bVendor;           // 0x29 vendor slot (POI_ExtractStandPos)
    u8 m_Pad2a[2];
};
struct POI_STRUCT {
    POI_STRUCT() {}
    POI_BIND m_Binds[12];   // 0x000 vendors [0..3], customers [4..]
    EXVector m_PlacedPos;   // 0x210 trigger position before floor snapping (POI_SetupNavMesh)
    EXVector m_Pos;         // 0x220
    EXVector m_Dir;         // 0x230 (sin yrot, 0, cos yrot, 0)
    EXODListItem m_Link;    // 0x240 group list link
    int m_ID;               // 0x24c
    float m_YRot;           // 0x250 facing (ActorMission.obj STATE_Distraction_PoolTable)
    int m_nVendors;         // 0x254
    int m_nCustomers;       // 0x258
    int m_Zone;             // 0x25c nav zone of m_Poly
    int m_UseCount;         // 0x260 times picked by POI_GetRandomType/Flag
    int m_Type;             // 0x264 location type hash (POI_GetNearestType)
    int m_Priority;         // 0x268 LocationAttribs m_Priority (default 100)
    int m_LocID;            // 0x26c location id (POI_ResolveLocation)
    int m_LocType;          // 0x270 LocationAttribs index, -1 none
    int m_ComState;         // 0x274 POI_GetComData/SetComData
    int m_ComData[3];       // 0x278
    u32 m_VolumeData;       // 0x284 data 9 of the type 10 environment volume holding the point (POI_PostLoadInit);
                            //       spawning needs 0 or the current TimeTable_Get()
    int m_nBound;           // 0x288 slots with their actor in place (last POI_Update)
    int m_FlagChannel;      // 0x28c set while anyone is in place; set by POI_ForceState (trigger data 3)
    int m_EmptyChannel;     // 0x290 set when the place empties (fewer than 2 left) (trigger data 5)
    int m_ForceChannel;     // 0x294 POI_ForceFlagChannel
    int m_VendorState;      // 0x298 anim state (0x4d000000 none)
    int m_CustomerState;    // 0x29c
    int m_bBindsEnabled;    // 0x2a0
    int m_PatternIter;      // 0x2a4
    int m_VendorProfile;    // 0x2a8 actor profile spawned as vendor (trigger data 4; 0x47000000 none)
    int m_VendorSpawnID;    // 0x2ac id of the spawned vendor
    int m_CustomerProfile;  // 0x2b0 actor profile spawned as customers (trigger data 11)
    int m_CustomerSpawnID[4]; // 0x2b4 ids of the spawned customers
    int m_MaxCustomerSpawn; // 0x2c4 (trigger data 12)
    int m_Unk2c8;           // 0x2c8 TODO
    int m_QueueDist;        // 0x2cc >0: STATE_GotoLocation queues at a 0x4200009b POI within this distance (ActorUtil.obj)
    int m_SpawnChannel;     // 0x2d0 spawning needs this flag channel set (trigger data 14)
    int m_BlockChannel;     // 0x2d4 spawning needs this flag channel clear (trigger data 15)
    int m_Poly;             // 0x2d8
    class EXItemAnimator_Script *m_pFX; // 0x2dc HT_File_Pred_FX script on m_pItem
    class XItem *m_pItem;   // 0x2e0 display item (trigger entity / effect)
    const char *m_pName;    // 0x2e4 LocationAttribs name ("LocX" none)
    u8 m_Unk2e8;            // 0x2e8 TODO
    s8 m_bPlayerNear;       // 0x2e9 player within 40 units (x/z); spawns only on arrival
    u8 m_Pad2ea[2];
};
// Nav-zone groups of locations (sizeof 0xc); [0] holds the unassigned ones.
struct POI_GROUP {
    POI_GROUP() {}
    int m_Zone;             // 0x00
    EXODList m_List;        // 0x04
};
// LocationAttribs row (sizeof 0x18).
struct LocationTable {
    int m_Unk0;             // 0x00
    int m_Hash;             // 0x04
    int m_Priority;         // 0x08 POI m_Priority
    int m_Pattern;          // 0x0c default stand pattern
    const char *m_pName;    // 0x10
    int m_Flags;            // 0x14
};
// FSAttribs row: anim for a forced POI state.
struct FSTable {
    int m_Anim;
    int m_State;
};
extern LocationTable LocationAttribs[];
extern FSTable FSAttribs[];
extern POI_GROUP POI_Group[];

void POI_ReturnToHubLife(int);
int POI_AnimFromState(int);
int POI_GetNavZoneIndex(int, signed char);
POI_STRUCT *POI_Get(int);
int POI_GetLocFlags(int);
signed char POI_VendorIsThere(int);
int POI_VendorGetID(int, int);
int POI_VendorStandIndex(int, int);
EXVector POI_VendorStandPos(int, int);
int POI_VendorStandPoly(int, int);
signed char POI_CustomerIsThere(int);
int POI_CustomerStandIndex(int, int);
EXVector POI_CustomerStandPos(int, int);
int POI_CustomerStandPoly(int, int);
int POI_CustomerGetID(int, int);
POI_BIND *POI_PatternIteratorNext(int);
int POI_GetComData(int poi, int *a, int *b, int *c);
void POI_SetComData(int, int, int, int, int);
void POI_SendMsg(int, int, MsgItem const &);
void POI_EnableBinds(int, signed char);
int POI_GetActorCount(int, int);
signed char POI_IsFull(int, LocCount::Flags);
signed char POI_ActorPresent(int, int);
signed char POI_IsVendor(int, int);
signed char POI_ForceState(int, int, int);
signed char POI_ForceFlagChannel(int, int);
int POI_GetOtherActorID(int, int);
void POI_AnimAquire(int, signed char);
void POI_AnimRelease(int, signed char);
signed char POI_AnimReady(int, signed char);
void AU_DelaySpawnActor(EXTrigger *);
int POI_GetNext(int, int);
signed char POI_VendorUnbind(int, int);
signed char POI_CustomerUnbind(int, int);
signed char POI_Unbind(int, int);
void POI_OnDeLoadNavMesh(XGeoNavMeshEntity *);
void POI_MissionEnd();
void POI_SetupNavMesh(int);
void POI_Update();
int POI_GetFirst(int);
void POI_Add(EXTrigger *, signed char);
signed char POI_VendorBind(int, int);
signed char POI_CustomerBind(int, int);
void POI_PatternIteratorBind(int, POI_BIND *, int);
void POI_OnLoadNavMesh(XGeoNavMeshEntity *);
void POI_PreLoadInit();
void POI_PostLoadInit();
int POI_GetNearestType(EXVector const &, int, int, int, signed char, signed char);
int POI_GetNearestEvadeBetweenTarget(EXVector const &, EXVector const &, int, int, signed char);
int POI_GetNearestFlag(EXVector const &, int, int, int, signed char, signed char, float);
int POI_GetNearestFlag_Level(EXVector const &, int, int, int, signed char, signed char, float);
int POI_GetNearestFlagWithinRadius(EXVector const &, EXVector const &, float, int, int, int, signed char, signed char);
int POI_UpdateNearestFlag(EXVector const &, int, int, int, int, signed char, signed char, float);
int POI_UpdateNearestFlag_Level(EXVector const &, int, int, int, int, signed char, signed char, float);
int POI_UpdateNearestFlagWithinRadius(EXVector const &, EXVector const &, float, int, int, int, int, signed char, signed char);
int POI_GetRandomFlag(int, int, int, signed char, signed char);
int POI_GetRandomType(int, int, int, signed char, signed char);
int POI_ResolveLocation(int, int, signed char, EXVector const *);

#endif
