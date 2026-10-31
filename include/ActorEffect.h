#ifndef ACTOREFFECT_H
#define ACTOREFFECT_H
#include "types.h"
#include "EXList.h"
#include "EXVector.h"

class Actor;
class EXItemAnimator_Script;

// A script effect attached to an actor (sizeof 0x38), listed in Actor::m_EffectList.
class ActorEffect {
public:
    virtual ~ActorEffect();
    unsigned __int64 ID() { return m_ID; }
    void SetTimer(int secs);
    static void PostLoadInit();
    void Init(Actor *pActor, unsigned int file, unsigned int hash, unsigned int datum, float time, signed char b,
              unsigned __int64 id);
    signed char DoUpdate();
    ActorEffect(Actor *pActor, unsigned int file, unsigned int hash, unsigned int datum, float time, signed char b,
                unsigned __int64 id);
    ActorEffect(Actor *pActor, unsigned int file, unsigned int hash, const EXVector &pos, float time, signed char b,
                unsigned __int64 id);
    static ActorEffect *FromLink(EXDListItem *p) { return p ? (ActorEffect *)((char *)p - 8) : 0; }

    EXDListItem m_Link;             // 0x08 (the u64 member aligns the first member to 8)
    Actor *m_pActor;                // 0x10
    u32 m_Unk14;
    unsigned __int64 m_ID;          // 0x18
    unsigned int m_File;            // 0x20 geo file hash
    unsigned int m_Hash;            // 0x24 script (> 0x4000000) or clone skin
    unsigned int m_Datum;           // 0x28 datum to follow (0 = none)
    float m_Timer;                  // 0x2c frames
    EXItemAnimator_Script *m_pScript;   // 0x30
    u32 m_bTimed : 1;               // 0x34 bit 0 TODO
    u32 m_bTimer : 1;               //      bit 1
    u32 m_bStopOnScriptEnd : 1;     //      bit 2
    u32 m_b3 : 29;
};
#endif
