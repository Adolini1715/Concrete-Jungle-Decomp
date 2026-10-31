#ifndef STATEMACHINE_H
#define STATEMACHINE_H
#include "types.h"

class Actor;
class EXVector;

// Actor message types. The 0xf000000x values are the state machine's own lifecycle messages.
enum MsgType {
    MSG_NONE = 0,
    MSG_ATTACKED_BY_PREDATOR = 3,   // data[0] = StructMsg_AttackedByPredator * (AU_HandlePlayerAttacks)
    MSG_MUGGING = 5,                // AU_HandleMuggingAttacks (Gosub STATE_BeingMugged)
    MSG_REQUEST_ITEM = 6,           // customer asks a vendor for its item; the reply (sm.m_Unk9c) is the carry id
    MSG_PLAY_ANIM = 8,              // data[0] = anim, data[1] = sender id (vendor and customer pairs)
    MSG_ROBBERY = 0x10,             // a vendor is being robbed (STATE_FS_Vendor Gosubs STATE_RobberyVictim)
    MSG_EXTORTION = 0x11,           // a vendor is being extorted (STATE_ExtortionVictim)
    MSG_ROBBERY_TAKE = 0x12,        // TODO name: robber takes the till; data[0] = robber id, reply = cash carry id
    MSG_DROP_ITEM = 9,              // anim script events (Actor::DoScriptCmdEvent)
    MSG_TAKE_ITEM = 0xa,
    MSG_SOMEONE_UNDER_ATTACK = 0xb, // AU_HandleSomeoneUnderAttack
    MSG_SOMEONE_SEEN_PREDATOR = 0xc,// AU_HandleSomeoneSeenPredator
    MSG_LOUD_NOISE = 0xd,           // SendMsg_LoudNoise / AU_HandleLoudNoise
    MSG_ATTACKED_BY_ACTOR = 0xe,    // data[0] = StructMsg_AttackedByActor * (AU_HandleActorAttacks)
    MSG_ATTRACT = 0x16,             // SendMsg_Attract / AU_HandleAttractNoise
    MSG_REPEL = 0x17,               // SendMsg_Repel
    MSG_FIGHT_ME = 0x18,            // data[0] = actor id (AU_HandleFightMe, Gosub STATE_FightActor)
    MSG_THROW = 0x19,
    MSG_INCOMING_ATTACK = 0x1d,     // data[0] = StructMsg_IncomingAttack * (AU_HandleIncomingAttack)
    MSG_STRIKE = 0x1e,
    MSG_STRIKE_LEFT = 0x1f,
    MSG_STRIKE_RIGHT = 0x20,
    MSG_GOTO_VAN = 0x21,           // collection mission: walk to the van; data = int x, y, z (STATE_MissionCollectionWaitForVan)
    MSG_VAN_DONE = 0x22,           // collection mission: collected, stand
    MSG_RELOAD = 0x23,
    MSG_DEATH_BULLET = 0x24,        // TODO name: STATE_AL_SleepDoorway Gosubs STATE_DeathBullet (data[0] = killer id)
    MSG_DRIVE_BY = 0x27,            // AU_HandleDriveBy
    MSG_BLOCK = 0x2d,
    MSG_ALIEN_STRIKE_CLAWS = 0x2e,
    MSG_ALIEN_STRIKE_TAIL = 0x2f,
    MSG_ALIEN_STRIKE_TEETH = 0x30,
    MSG_SCRIPT_EVENT_32 = 0x32,     // TODO name: script event 0x1600007a
    MSG_SCRIPT_EVENT_33 = 0x33,     // TODO name: script event 0x1600007f
    MSG_ENTER = 0xf0000001,     // state entered (data = Gosub/Goto arguments)
    MSG_EXIT = 0xf0000002,      // state left (Goto, or unwound by Return/FlushStack)
    MSG_RETURN = 0xf0000003,    // a Gosub'd state returned (data = Return values)
    MSG_EXT = 0xf0000004,       // ExtGosub/ExtGoto kick
    MSG_SUSPEND = 0xf0000005    // state suspended by a Gosub
};

// A message delivered to an actor's state (sizeof 0x14).
struct MsgItem {
    MsgItem(MsgType type = MSG_NONE, int a = 0, int b = 0, int c = 0, int d = 0)
    {
        m_Type = type;
        m_Data[0] = a;
        m_Data[1] = b;
        m_Data[2] = c;
        m_Data[3] = d;
    }
    MsgType m_Type;     // 0x00
    int m_Data[4];      // 0x04
};

// Message type -> watch bits (g_aMsgBits, ActorStateMachine.obj).
struct MsgBits {
    MsgType m_Msg;
    unsigned int m_Bits;
};
extern MsgBits g_aMsgBits[];
extern int g_iNumMsgBits;
extern signed char ignorereturns;
unsigned int GetMsgBitsForMsg(MsgType msg);

// Alert watch bits.
extern const unsigned int AW_BIT_HAVE_SEEN_PLAYER;
extern const unsigned int AW_BIT_HAVE_SEEN_CLOAKED_PLAYER;
extern const unsigned int AW_BIT_DAZED;
extern const unsigned int AW_BIT_DISTANCE;
extern const unsigned int AW_BIT_LOW_HEALTH;
extern const unsigned int AW_BIT_LARGE_DAMAGE;
extern const unsigned int AW_BIT_TIME;

class StateMachine;
typedef signed char (__cdecl *StateFn)(StateMachine &);

// An actor's brain: a stack-based state machine of plain state functions (ActorStateMachine.obj).
// Embedded in Actor at +0xe8. States keep their locals in m_LocalData (cleared on entry).
class StateMachine {
public:
    unsigned int *GetLocalData() { return m_LocalData; }
    StateFn GetState() const { return m_State; }
    Actor &GetActorOwner() { return *m_pActor; }
    int &GetIntRef(int &i) { return (int &)m_LocalData[i++]; }
    float &GetFloatRef(int &i) { return (float &)m_LocalData[i++]; }
    EXVector &GetVectorRef(int &i)
    {
        if (i % 4) i = (i / 4) * 3 + 4;  // TODO verify (ActorCity.obj)
        EXVector &v = (EXVector &)m_LocalData[i];
        i += 4;
        return v;
    }
    StateMachine();
    ~StateMachine();
    void PopState();
    void Run(const MsgItem &msg);
    void Gosub(unsigned int flags, StateFn state, int a, int b, int c, int d);
    void Return(int a, int b, int c, int d);
    void Goto(unsigned int flags, StateFn state, int a, int b, int c, int d);
    void ExtGosub(unsigned int flags, StateFn state, int a, int b, int c, int d);
    void ExtGoto(unsigned int flags, StateFn state, int a, int b, int c, int d);
    void Init(Actor &actor, StateFn state);
    void FlushStack();

    unsigned int m_LocalData[20];   // 0x00 the current state's locals
    Actor *m_pActor;                // 0x50
    s8 m_bGosub;                    // 0x54 pending Gosub
    s8 m_bReturn;                   // 0x55 pending Return
    s8 m_bGoto;                     // 0x56 pending Goto
    s8 m_bExt;                      // 0x57 Run must not re-run the current state
    StateFn m_State;                // 0x58 current state
    StateFn m_NextState;            // 0x5c Gosub/Goto target
    unsigned int m_PushFlags;       // 0x60 stack flags pushed with the suspended state
    int m_Args[4];                  // 0x64 Gosub/Goto arguments (MSG_ENTER data)
    int m_Ret[4];                   // 0x74 Return values (MSG_RETURN data)
    u32 m_Unk84;                    // 0x84 TODO
    MsgItem m_Msg;                  // 0x88 message being processed
    int m_Unk9c;                    // 0x9c cleared by Run
};
#endif
