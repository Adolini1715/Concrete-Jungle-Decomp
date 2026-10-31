#ifndef XITEMHANDLER_OBJECTIVE_H
#define XITEMHANDLER_OBJECTIVE_H
#include "EXItemHandler.h"
#include "EXVector.h"

class XItemList;
class Actor;
class XItem;

// Mission objective (Objective.obj). sizeof 0xdc.
// m_State: 3 idle, 1 triggered, 2 bonus reset. m_Flags (SetFlags): 8 kill count reached, 0x80 all destroyed.
class XItemHandler_Objective : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_Objective)
public:
    virtual ~XItemHandler_Objective();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    XItemHandler_Objective() {}
    static XItemHandler_Objective *CreateObject();
    void SetFlags(unsigned short f) { m_Flags |= f; }
    void ProcessActorDeath(Actor *pActor);
    void TriggerObjective();
    void ProcessActorKilled(Actor *pActor);
    void ProcessObjectDestroyed();
    s8 CaresAbout(Actor *pActor);

    s8 m_bFirstUpdate;          // 0x08
    u8 m_Unk09[3];
    int m_Id;                   // 0x0c trigger id (Mission_GetMissionItem)
    int m_Links[8];             // 0x10 linked trigger ids
    XItem *m_pLinkItems[8];     // 0x30 resolved m_Links (cleared at creation)
    u16 m_ObjFlags;             // 0x50 trigger options: 1 hide timer, 2, 4 profile match, 8 always succeed, 0x10 start
                                //      without links, 0x20 light set script, 0x40 bonus, 0x80 show kills, 0x100 add
                                //      time to active timers, 0x200 sound, 0x400, 0x800 escape warning, 0x1000 start
                                //      when m_FlagChannel clears, 0x2000 no destroy count text
    u16 m_Unk52;
    unsigned int m_Hash;        // 0x54 objective hash (0xaceface / 0x1cef00d / 0xac1dcafe special)
    u16 m_SuccessMask;          // 0x58 m_Flags bits that complete the objective (2 time limit reached, 4 prey
                                //      out of escape range, 0x400 alarm level, 0x800 flag channel set, ...)
    u16 m_FailMask;             // 0x5a m_Flags bits that fail it (passed to Mission_Fail / Mission_BonusFail)
    int m_Profile;              // 0x5c actor profile id to spawn / match
    int m_Count;                // 0x60 kills needed
    int m_TimeLimit;            // 0x64 seconds (timer shown unless m_ObjFlags & 1)
    int m_SpawnPoint;           // 0x68
    int m_SpawnCount;           // 0x6c
    int m_FlagChannel;          // 0x70 set on success; 0x800 condition; m_ObjFlags 0x1000 waits for it to clear
    int m_AlarmFame;            // 0x74 alarm added on success; 0x400 condition threshold (< -100 hides the meter)
    unsigned int m_StatsLevel;  // 0x78 level whose stats page opens on success (flag channels 0x11d / 0x11b)
    int m_TargetFlagChannel;    // 0x7c copied to spawned actors
    unsigned int m_Text;        // 0x80 objective text hash
    u8 m_Unk84;                 // 0x84 copied to spawned actors (radar type)
    u8 m_Unk85[3];
    unsigned int m_CompleteText; // 0x88 objective text marked complete in the pause screen
    XItem *m_pRefs[8];          // 0x8c items whose links name this objective
    int m_RefCount;             // 0xac
    int m_DestroyCount;         // 0xb0
    int m_Destroyed;            // 0xb4
    u8 m_State;                 // 0xb8
    u8 m_Unkb9;
    u16 m_Flags;                // 0xba
    int m_Killed;               // 0xbc
    float m_Timer;              // 0xc0
    int m_ActorId;              // 0xc4 last spawned actor
    int m_Self;                 // 0xc8 (int)this, matched against Actor +0x774 (spawn owner)
    EXVector m_Pos;             // 0xcc trigger position
};

extern XItemList gTrigList;
XItem *Mission_GetMissionItem(int id);
#endif
