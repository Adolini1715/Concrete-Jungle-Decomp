#ifndef AIFORMATION_H
#define AIFORMATION_H
#include "types.h"
#include "EXVector.h"
#include "XNavLocation.h"

class CFormation;

// One slot of a combat formation around the player (AIFormation.obj, sizeof 0x2c).
class CFormationItem {
public:
    int GetID() const { return m_ID; }
    signed char GetActive() const { return m_bActive; }
    signed char GetAction() const { return m_Mode == 1; }
    int GetMode() const { return m_Mode; }
    void SetID(int id) { m_ID = id; }
    void SetActive(signed char b) { m_bActive = b; }
    void SetFormation(CFormation *p) { m_pFormation = p; }
    void SetPos(EXVector &p) { m_Pos = p; }
    void SetMode(int m) { m_Mode = m; }
    void AddFlag(unsigned int f) { m_Flags |= f; }
    void RemoveFlag(unsigned int f) { m_Flags &= ~f; }
    void Initialise() {}
    CFormationItem();
    void Update();
    EXVector GetWorldAng() const;
    signed char AttackDistanceAngle(float &dist, float &ang);
    void CalcItemPos(int slot, float f);
    EXVector GetWorldPos() const;

    EXVector m_Pos;             // 0x00 offset from the formation origin
    CFormation *m_pFormation;   // 0x10
    int m_ID;                   // 0x14 actor id (-1 = free)
    int m_Mode;                 // 0x18 (1 = attacking)
    float m_Timer;              // 0x1c
    unsigned int m_Flags;       // 0x20 low 3 bits: ring (1 inner, 2 middle, 4 outer)
    float m_Angle;              // 0x24
    s8 m_bActive;               // 0x28
};

// Formation of up to 32 actors around a nav location (sizeof 0x648). One instance: g_Formations.
class CFormation {
public:
    const EXVector GetPos() { return m_NavLoc.m_vPos; }
    EXVector GetAng() const { return m_Ang; }
    XNavLocation &NavLoc() { return m_NavLoc; }
    ~CFormation() {}
    CFormation();
    int AllocSlot(int id);
    void Update();
    int ItemNumFromActorID(int id) const;
    CFormationItem *ItemPtrFromActorID(int id, signed char b);
    signed char GetAction(int id);
    signed char GetSlotDistAng(int id, float &dist, float &ang);
    void GetOrigin(EXVector &out);
    void GetOriginAndAngle(EXVector &pos, EXVector &ang);
    void Initialise();
    int GetAllocatedSlotCount() const;
    float GetDistSqClosestActor();
    signed char FreeSlot(int id);
    signed char GetSlotPosAndAngle(int id, EXVector &pos, EXVector &ang);
    signed char GetSlotPos(int id, EXVector &pos);

    CFormationItem m_Items[32];     // 0x000
    int m_Count;                    // 0x580
    XNavLocation m_NavLoc;          // 0x584
    EXVector m_Ang;                 // 0x60c
    EXVector m_v61c;                // 0x61c TODO name
    EXVector m_v62c;                // 0x62c TODO name
    u16 m_w63c;                     // 0x63c TODO name
    u16 m_w63e;                     // 0x63e
    int m_640;                      // 0x640 (FEText.h static follows at 0x644)
};
extern CFormation g_Formations;
extern int g_iCombatInnerCircle;
extern int g_iCombatMiddleCircle;
extern int g_iCombatOuterCircle;
#endif
