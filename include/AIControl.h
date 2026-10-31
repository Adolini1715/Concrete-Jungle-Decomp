#ifndef AICONTROL_H
#define AICONTROL_H
#include "types.h"
#include "EXVector.h"
#include "XNavStandIn.h"
#include "XNavLocation.h"
#include "XNavMeshHandler.h"

class XGeoNavMeshEntity;
template <class T, class A> class EXArray;

// Global AI state (AIControl.obj, gAIControl, sizeof 0x30c): the player's nav location, combat
// music, zone sight pairs, actor-system init/shutdown.
class AIControl {
public:
    XNavMeshHandler &NavMeshHdlr() { return gNavMeshHandler; }
    const EXVector &LastSeenPlayerPos() { return m_LastSeenPlayerPos; }
    float GetLastKnownLocationTimer() const { return m_LastKnownLocationTimer; }
    void IncActorsInCombatCount() { m_ActorsInCombatCount++; }
    signed char IsPlayerTagged() const { return m_PlayerTaggedTimer > 0.0f; }
    signed char CanSpeakCombatVO() const { return m_CombatVOTimer <= 0.0f; }
    void SetLastSeenPlayerPos(const EXVector &pos)
    {
        m_LastSeenPlayerPos = pos;
        m_PlayerNavLocLastSeen = m_PlayerNavLoc;
        m_LastKnownLocationTimer = 0.0f;
    }
    XNavLocation &PlayerNavLoc() { return m_PlayerNavLoc; }
    float DistOffNaviMeshLastSeen() { return m_DistOffNaviMeshLastSeen; }
    XNavLocation &PlayerNavLocLastSeen() { return m_PlayerNavLocLastSeen; }
    signed char IsGrenadeAllowed() const { return m_GrenadeTime > m_GrenadeNextTime; }
    void GrenadeThrown(int frames);  // header inline in retail ([fi] in ActorCombat.obj); defined inline there
    int GetJumpToZoneArray(int zone, EXArray<int, int> &a);
    __forceinline void DoingHTMLReport(signed char b) { m_bHTMLReport = b; }
    __forceinline int &NavCacheSync() { return m_NavCacheSync; }
    AIControl();
    ~AIControl();
    signed char BootInit();
    signed char PreLoadInit();
    signed char PostLoadInit();
    signed char UnloadLevel();
    signed char Shutdown();
    signed char OnSectionLoad(unsigned int hash);
    signed char OnSectionDeload(unsigned int hash);
    void UpdateCombatMusic();
    signed char HandleMemCompaction();
    void NavMeshAdded(XGeoNavMeshEntity *pMesh);
    void NavMeshDeleting(XGeoNavMeshEntity *pMesh);
    signed char CheckZoneSight(int a, int b) const;
    void SetSpokenCombatVO();
    signed char Update();
    signed char GetDistantPos(const EXVector &pos, float dist, EXVector &out, int *pPoly);
    signed char PositionIsValid(const EXVector &pos);
    void FindSightLinkerPaths();
    signed char LevelPreRun();
    void SetCombatMusicHash(int hash) { m_CombatMusicHash = hash; }
    signed char CombatMusicPlaying() { return m_bCombatMusicPlaying; }
    __forceinline signed char IsDoingHTMLReport() { return m_bHTMLReport; }

    XNavRoute m_Route;                  // 0x000 TODO base class or member
    u8 m_Unk11c[0x13c - 0x11c];         // TODO layout
    EXVector m_LastSeenPlayerPos;       // 0x13c
    u8 m_Unk14c[0x15c - 0x14c];
    XNavLocation m_PlayerNavLoc;        // 0x15c
    XNavLocation m_PlayerNavLocLastSeen;// 0x1e4
    float m_Unk26c;                     // 0x26c
    float m_DistOffNaviMeshLastSeen;    // 0x270
    float m_LastKnownLocationTimer;     // 0x274
    float m_PlayerTaggedTimer;          // 0x278
    int m_ActorsInCombatCount;          // 0x27c
    float m_GrenadeTime;                // 0x280
    float m_GrenadeNextTime;            // 0x284
    int m_NavCacheSync;                 // 0x288
    s8 m_bCombatMusicPlaying;           // 0x28c
    float m_CombatMusicTimer;           // 0x290
    float m_CombatMusicStopTimer;       // 0x294
    int m_CombatMusicHash;              // 0x298
    int m_PrevMusicHash;                // 0x29c
    s8 m_bHTMLReport;                   // 0x2a0
    s8 m_bLevelLoading;                 // 0x2a1 TODO name (blocks HandleMemCompaction)
    int m_ZoneSight[12][2];             // 0x2a4 zone pairs that can see each other (-1 = none)
    int m_ZoneSightCount;               // 0x304
    float m_CombatVOTimer;              // 0x308 (sizeof 0x30c: CFormation::Update's static follows)
};
extern AIControl gAIControl;
#endif
