#ifndef XITEMHANDLER_REINFORCEMENTS_H
#define XITEMHANDLER_REINFORCEMENTS_H
#include "EXItemHandler.h"

class XItemList;

// Reinforcement spawner (Reinforcements.obj, sizeof 0x24): spawns m_Count actors of profile
// m_Profile at its item, one every 10 frames; repeats after m_RepeatDelay frames or deletes itself.
class XItemHandler_Reinforcements : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_Reinforcements)
public:
    virtual ~XItemHandler_Reinforcements();
    virtual signed char Initialise();
    virtual signed char DoUpdate();
    XItemHandler_Reinforcements() {}
    void SpawnActors();
    static XItemHandler_Reinforcements *CreateObject();

    s8 m_bSpawning;         // 0x08
    u8 m_Unk9[3];
    int m_Spawned;          // 0x0c
    int m_SpawnDelay;       // 0x10 frames to the next spawn
    int m_Profile;          // 0x14 actor profile hash
    int m_Count;            // 0x18
    int m_RepeatDelay;      // 0x1c
    int m_RepeatTimer;      // 0x20
};
extern XItemList gReinforceList;
#endif
