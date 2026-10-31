#include "StateMachine.h"
#include "Actor.h"
#include "ActorSpecial.h"
#include "FEText.h"

const unsigned int AW_BIT_HAVE_SEEN_PLAYER = 0x1000;
const unsigned int AW_BIT_HAVE_SEEN_CLOAKED_PLAYER = 0x2000;
const unsigned int AW_BIT_DAZED = 0x4000;
const unsigned int AW_BIT_DISTANCE = 0x8000;
const unsigned int AW_BIT_LOW_HEALTH = 0x10000;
const unsigned int AW_BIT_LARGE_DAMAGE = 0x20000;
const unsigned int AW_BIT_TIME = 0x40000;

MsgBits g_aMsgBits[] = {
    { (MsgType)0xb, 1 },
    { (MsgType)0xc, 2 },
    { (MsgType)0xd, 4 },
    { (MsgType)0x16, 8 },
    { (MsgType)0x17, 0x10 },
};
int g_iNumMsgBits = sizeof(g_aMsgBits) / sizeof(g_aMsgBits[0]);
signed char ignorereturns;

unsigned int GetMsgBitsForMsg(MsgType msg)
{
    MsgBits *p = g_aMsgBits;
    for (int i = 0; i < g_iNumMsgBits; i++, p++)
        if (p->m_Msg == msg) return p->m_Bits;
    return 0;
}

// NEAR: push edi scheduled three stores earlier than retail.
StateMachine::StateMachine()
{
    m_pActor = 0;
    m_State = 0;
    m_bGosub = 0;
    m_bGoto = 0;
    m_bReturn = 0;
    m_bExt = 0;
    m_Msg.m_Type = MSG_NONE;
    m_Msg.m_Data[0] = 0;
    m_Msg.m_Data[1] = 0;
    m_Msg.m_Data[2] = 0;
    m_Msg.m_Data[3] = 0;
    memset(m_LocalData, 0, sizeof(m_LocalData));
}

StateMachine::~StateMachine() {}

void StateMachine::PopState() { m_State = m_pActor->PopState(); }

void StateMachine::Run(const MsgItem &msg)
{
    m_Unk9c = 0;
    m_Msg = msg;
    if (!m_bExt && m_State) m_State(*this);
    m_bExt = 0;
    for (int n = 0; (m_bGosub || m_bReturn || m_bGoto) && n <= 20; n++) {
        if (m_bReturn) {
            m_Msg = MsgItem(MSG_EXIT, 0, 0, 0, 0);
            m_bReturn = 0;
            ignorereturns = 1;
            if (m_State) m_State(*this);
            ignorereturns = 0;
            m_State = m_pActor->PopState();
            m_Msg = MsgItem(MSG_RETURN, m_Ret[0], m_Ret[1], m_Ret[2], m_Ret[3]);
            if (m_State) m_State(*this);
        }
        if (m_bGosub) {
            m_Msg = MsgItem(MSG_SUSPEND, 0, 0, 0, 0);
            m_bGosub = 0;
            ignorereturns = 1;
            if (m_State) m_State(*this);
            ignorereturns = 0;
            m_pActor->PushState(m_State, m_PushFlags);
            memset(m_LocalData, 0, sizeof(m_LocalData));
            m_State = m_NextState;
            m_Msg = MsgItem(MSG_ENTER, m_Args[0], m_Args[1], m_Args[2], m_Args[3]);
            if (m_State) m_State(*this);
        }
        if (m_bGoto) {
            m_Msg = MsgItem(MSG_EXIT, 0, 0, 0, 0);
            m_bGoto = 0;
            ignorereturns = 1;
            if (m_State) m_State(*this);
            ignorereturns = 0;
            memset(m_LocalData, 0, sizeof(m_LocalData));
            m_State = m_NextState;
            m_Msg = MsgItem(MSG_ENTER, m_Args[0], m_Args[1], m_Args[2], m_Args[3]);
            if (m_State) m_State(*this);
        }
    }
}

void StateMachine::Gosub(unsigned int flags, StateFn state, int a, int b, int c, int d)
{
    if (!state) return;
    m_NextState = state;
    m_bGosub = 1;
    m_bReturn = 0;
    m_bGoto = 0;
    m_bExt = 0;
    m_Args[0] = a;
    m_Args[1] = b;
    m_Args[2] = c;
    m_Args[3] = d;
    m_PushFlags = flags;
}

void StateMachine::Return(int a, int b, int c, int d)
{
    if (ignorereturns) return;
    m_bReturn = 1;
    m_bGosub = 0;
    m_bGoto = 0;
    m_bExt = 0;
    m_Ret[0] = a;
    m_Ret[1] = b;
    m_Ret[2] = c;
    m_Ret[3] = d;
}

void StateMachine::Goto(unsigned int flags, StateFn state, int a, int b, int c, int d)
{
    if (!state) return;
    m_NextState = state;
    m_bGoto = 1;
    m_bGosub = 0;
    m_bReturn = 0;
    m_bExt = 0;
    m_Args[0] = a;
    m_Args[1] = b;
    m_Args[2] = c;
    m_Args[3] = d;
    m_PushFlags = flags;
}

void StateMachine::ExtGosub(unsigned int flags, StateFn state, int a, int b, int c, int d)
{
    if (ActorSpecial_IsSpecial(m_pActor)) return;
    Gosub(flags, state, a, b, c, d);
    m_bExt = 1;
    m_Msg = MsgItem(MSG_EXT, 0, 0, 0, 0);
    Run(m_Msg);
}

void StateMachine::ExtGoto(unsigned int flags, StateFn state, int a, int b, int c, int d)
{
    if (ActorSpecial_IsSpecial(m_pActor)) return;
    Goto(flags, state, a, b, c, d);
    m_bExt = 1;
    m_Msg = MsgItem(MSG_EXT, 0, 0, 0, 0);
    Run(m_Msg);
}

void StateMachine::Init(Actor &actor, StateFn state)
{
    m_pActor = &actor;
    m_State = state;
    Run(MsgItem(MSG_ENTER, 0, 0, 0, 0));
}

void StateMachine::FlushStack()
{
    while (m_State) {
        m_Msg = MsgItem(MSG_EXIT, 0, 0, 0, 0);
        m_State(*this);
        m_State = m_pActor->PopState();
    }
}
