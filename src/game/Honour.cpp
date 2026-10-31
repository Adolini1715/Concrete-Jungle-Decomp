#include "types.h"
#include "FEText.h"
#include "GameData.h"

// TODO stand-in: actor profiles (ActorProfile.obj, not this area).
struct ActorProfileIndex { u32 m_Index; };
struct ActorProfile {
    u8 m_Unk0[0x64];    // TODO layout
    int m_Honour;       // 0x64
};
ActorProfile *AP_GetPtrFromIndex(const ActorProfileIndex &idx);
void Objective_FlagCondition(unsigned int cond);

// TODO stand-in: Actor (Actor.obj, not this area).
class Actor {
public:
    int GetHitBy() { return m_HitBy; }
    u8 m_Unk0[0x778];               // TODO layout
    ActorProfileIndex m_Profile;    // 0x778
    u8 m_Unk77c[0x844 - 0x77c];
    int m_HitBy;                    // 0x844
};

void Honour_ScoreKill(Actor *pActor)
{
    if (pActor->GetHitBy() == -1) return;
    int honour = AP_GetPtrFromIndex(pActor->m_Profile)->m_Honour;
    if (honour >= 0) GameData::Instance(1)->Stats()->m_Kills++;
    else GameData::Instance(1)->Stats()->m_KillsDishonour++;
    if (honour < 0) Objective_FlagCondition(0x40);
}
