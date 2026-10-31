#ifndef ACTORSPECIAL_H
#define ACTORSPECIAL_H
#include "types.h"
#include "StateMachine.h"

class Actor;

// A special (scripted) actor slot (ActorSpecial.obj, sizeof 0x20). When an actor of m_Profile is
// created and the slot is enabled and free, it takes the slot with m_Chance percent.
struct ActorSpecial {
    int m_Hash;             // 0x00
    s8 m_bEnabled;          // 0x04
    int m_ActorID;          // 0x08 actor in the slot (0 = free)
    int m_Chance;           // 0x0c percent
    int m_Profile;          // 0x10 profile hash
    StateFn m_State;        // 0x14 start state
    int m_Unk18;            // 0x18 TODO
    unsigned int m_Text;    // 0x1c objective text printed on a player kill
};
extern ActorSpecial SpecialList[10];
extern int SpecialCount;

int Sq(int x);
signed char ActorSpecial_Add(int hash, int chance, int profile, StateFn state, int a, unsigned int text);
signed char ActorSpecial_RegisterPlayerKill(Actor *pActor);
signed char ActorSpecial_Unregister(Actor *pActor);
ActorSpecial *ActorSpecial_GetEntry(int hash);
signed char ActorSpecial_IsSpecial(Actor *pActor);
signed char Validate_KillerHooker(Actor *a, Actor *b);
signed char Validate_YuppieMurderer(Actor *a, Actor *b);
signed char Validate_SerialKiller(Actor *a, Actor *b);
signed char Validate_Zealot(Actor *a, Actor *b);
signed char ActorSpecial_Enable(int hash, signed char b);
StateFn ActorSpecial_Register(Actor *pActor);
signed char ActorSpecial_PostLoadInit();
#endif
