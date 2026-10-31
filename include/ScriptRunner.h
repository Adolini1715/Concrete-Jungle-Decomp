#ifndef SCRIPTRUNNER_H
#define SCRIPTRUNNER_H
#include "EXItemHandler.h"
#include "FEObjectScript.h"
#include "EXExtRenderInfoStandIn.h"

class XItem;
class EXItem;
class EXVector;

// Scripted world object: doors, hitable props (ScriptRunner.obj). sizeof 0x104.
class XItemHandler_ScriptRunner : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_ScriptRunner)
public:
    virtual ~XItemHandler_ScriptRunner();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    virtual u32 DoScriptCmdEvent(GEOSCRIPT_EVENT *pEvent, EXItemAnimator *pAnim);
    virtual void ApplyDamage(float damage, unsigned int type);
    XItemHandler_ScriptRunner() {}
    static XItemHandler_ScriptRunner *CreateObject();
    void DoWeNeedToBeRemovedFromAnyList(EXItem *pItem);
    void DoImpactFX(EXVector &pos);
protected:
    unsigned int RestoreState();
    unsigned int SaveState();
public:
    s8 m_bHitable;              // 0x08 accepts damage (set by SR_WaitForHit)
    s8 m_bMissionDone;          // 0x09 mission objective reached (SR_WaitForMission); item deleted when the mission ends
    u8 m_Unka[2];
    u32 m_GeoHash;              // 0x0c geo file holding the script (trigger, else the current map)
    u32 m_ScriptHash;           // 0x10 running script (animator hash)
    u32 m_NextScript;           // 0x14 SR_SwitchScript: script to switch to on the next update
    unsigned int m_DamageType;  // 0x18 damage hash accepted by ApplyDamage
    int m_ImpactType;           // 0x1c index into the impact effect/sound tables
    int m_State;                // 0x20 1 running, 2 idle (out of range / zone hidden), 3 finished -> 4 removed
    int m_DoorState;            // 0x24 1: door the player can open (DoorTest_GetNearest)
    u32 m_Flags;                // 0x28 trigger data 0: 1/any update groups, 2 door test, 4 hitable
    int m_SaveCount;            // 0x2c alternate-state depth (SR_StateMarker / SR_SetAlternateState)
    u8 m_ListFlags;             // 0x30 1: in gDoorTestList, 2: in gHitableList
    float m_Health;             // 0x34
    float m_HitTimer;           // 0x38 seconds before the next hit counts
    s8 m_bFindMissionItem;      // 0x3c look up m_pMissionItem on the first update
    int m_MissionId;            // 0x40 trigger link 0 (Mission_GetMissionItem)
    XItem *m_pMissionItem;      // 0x44 objective told when this is destroyed
    u32 m_Flags2;               // 0x48 trigger +0x4c: 2 = run even when the zone is not visible
    int m_StateMarkers;         // 0x4c SR_StateMarker count (SaveState)
    float m_HitRadius;          // 0x50 hit-collision datum radius (0.5 without one)
    const EXVector &HitCentre() const { return *(const EXVector *)m_HitCentre; }
    float m_HitCentre[4];       // 0x54 hit-collision centre, world space
    EXTexSwapInfo m_TexSwapInfo;      // 0x64
    EXExtRenderInfo m_ExtRenderInfo;  // 0x78
    u32 m_TriggerFlags;         // 0xf8 trigger data 1 (0x18: item starts hidden)
    union { u32 u; float f; } m_Range;  // 0xfc trigger data 2: camera distance to run within (0 = any)
    int m_DeleteChannel;        // 0x100 trigger data 3: flag channel that deletes the item
};

XItem *DoorTest_GetNearest(const EXVector &pos, float maxDist);
#endif
