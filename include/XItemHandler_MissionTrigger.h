#ifndef XITEMHANDLER_MISSIONTRIGGER_H
#define XITEMHANDLER_MISSIONTRIGGER_H
#include "EXItemHandler.h"
#include "EXGeoTrigger.h"

class XItem;
class EXItem;
struct EXGeoTriggerDatum;
class EXVector;

// Mission trigger / destination / exit volume (Objective_MissionTrigger.obj). sizeof 0xb0.
class XItemHandler_MissionTrigger : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_MissionTrigger)
public:
    virtual ~XItemHandler_MissionTrigger();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    XItemHandler_MissionTrigger() {}
    static XItemHandler_MissionTrigger *CreateObject();
protected:
    void LoadTriggerFile();
    void Draw3DText();
    void UpdateDestination();
    void UpdateMissionTrigger();
    void UpdateExit();
public:
    s8 m_bFirstUpdate;          // 0x08
    u8 m_Unk09[3];
    int m_Id;                   // 0x0c trigger id (Mission_GetMissionItem)
    int m_Links[8];             // 0x10 linked trigger ids
    XItem *m_pLinks[8];         // 0x30 resolved on the first update
    EXGeoBaseDatum m_Volume;    // 0x50 trigger collision
    short m_Type;               // 0x80 0 mission trigger, 1 destination, 2 exit
    s8 m_bInside;               // 0x82 player has been inside (latched)
    u8 m_Unk83;
    unsigned int m_MissionHash; // 0x84 mission trigger: data 0 (HT_Mission_*, TrafficMissionInit)
    u32 m_Unk88;                // 0x88 data 1
    int m_Radius;               // 0x8c used when the volume is not a box/sphere/cylinder (default 3, 0 with option 8)
    int m_PlayerSeenInterval;   // 0x90 data 3 -> Mission_PlayerSeenInterval
    unsigned int m_Text2;       // 0x94 data 4 (0x51000001..0x510009b8)
    unsigned int m_Text;        // 0x98 objective text (0x51000001..0x510009b8)
    unsigned int m_TriggerFile; // 0x9c geo file holding the triggers to load (0x1000000: none)
    int m_EscapeRadius;         // 0xa0 -> Mission_EscapeRadius (default 50)
    union { u32 u; float f; } m_AlarmResetDelay;  // 0xa4 -> Mission_AlarmResetDelay
    union { u32 u; float f; } m_AlarmResetSpeed;  // 0xa8 -> Mission_AlarmResetSpeed
    u32 m_Options;              // 0xac trigger m_Unk4c: 2 start without the volume, 4 stay after starting, 8
                                //      update groups, 0x20 alarm meter, 0x40 only while a mission runs (else
                                //      only before one), 0x80 (destination) any carried prey, 0x200 hide meter,
                                //      0x400 (destination) not the held actor
};

signed char IsTouchplate(EXItem *pItem);
#endif
