#ifndef ACTORTIMETABLE_H
#define ACTORTIMETABLE_H
#include "types.h"
#include "EXVector.h"
#include "XNavStandIn.h"

class Actor;

// One row of a time table worksheet (sizeof 0xc): from m_Time (hours) on, m_Density actors.
struct TimeTableEntry {
    float m_Time;       // 0x00
    int m_Density;      // 0x04 also the profile's weight in GetRandomProfile
    int m_Profile;      // 0x08
};
// The current time table (TODO names of the first two fields: reset to 0 and 8.0).
struct TimeTableHeader {
    s8 m_Unk0;
    u8 m_01[3];
    float m_Unk4;
    int m_Count;                // 0x08 worksheet rows
    TimeTableEntry *m_pEntries; // 0x0c the worksheet (AU_GetWorkSheet)
};
struct ActorTimeTable;

// A cached pedestrian spawn position (sizeof 0x9c).
struct tagSpawnPos {
    XNavLocation m_Loc;     // 0x00
    float m_88;             // 0x88 TODO
    s8 m_bValid;            // 0x8c
    float m_90;             // 0x90
    float m_94;             // 0x94
    float m_98;             // 0x98
};
extern int SpawnIndex;
extern tagSpawnPos SpawnPos[8];

void TimeTable_RefreshSpreadsheet();
void TimeTable_Set(unsigned int sheet);
int TimeTable_Get();
int TimeTable_GetDensity(TimeTableHeader *pTable);
int TimeTable_GetAvailableActorCount();
int TimeTable_GetRandomProfile(TimeTableHeader *pTable);
float TimeTable_GetTime();
void TimeTable_SkipHour();
void TimeTable_PostLoadInit();
void TimeTable_LevelPreRun();
void TimeTable_TaggedPersonHere(int a, int b);
Actor *TimeTable_CreateActor(unsigned int profile, EXVector pos, int poly, float rotY, unsigned int flags);
Actor *TimeTable_CreateActor(unsigned int profile, EXVector pos, EXVector from, float rotY, unsigned int flags);
void TimeTable_UpdateSpawnPos();
signed char TimeTable_GenerateSpawnPos(EXVector pos, XNavLocation &loc);
void TimeTable_Update();
#endif
