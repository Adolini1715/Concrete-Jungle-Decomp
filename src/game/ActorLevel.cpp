#include "ActorUtil.h"
#include "Actor.h"
#include "ActorLocation.h"
#include "ActorCarryable.h"
#include "XApp.h"
#include "EXRand.h"
#include "FEText.h"

signed char STATE_AL_Idle(StateMachine &sm)
{
    int i = 0;
    EXVector &pos = sm.GetVectorRef(i);
    int &poi = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        poi = sm.m_Msg.m_Data[0];
        pos = sm.GetActorOwner().GetFeetPos();
        return 1;
    case MSG_EXT:
        sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_AL_Chat(StateMachine &sm)
{
    int i = 0;
    EXVector &pos = sm.GetVectorRef(i);
    int &poi = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (POI_GetActorCount(poi, 0xd) >= 2) {
            if (POI_GetComData(poi, 0, 0, 0) > 0 && POI_GetComData(poi, 0, 0, 0) != sm.GetActorOwner().GetRefID()) {
                if (Actor::GetPtrFromID(POI_GetComData(poi, 0, 0, 0)))
                    sm.GetActorOwner().SetAnimType(0x90000e4, 0, 1);
            } else if (POI_ActorPresent(poi, sm.GetActorOwner().GetRefID())) {
                POI_SetComData(poi, sm.GetActorOwner().GetRefID(), 0, 0, 0);
                sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
                sm.GetActorOwner().SetAnimType(0x90000e3, 0, 1);
            }
        } else {
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        }
        return 1;
    case MSG_EXIT:
        if (POI_GetComData(poi, 0, 0, 0) == sm.GetActorOwner().GetRefID()) POI_SetComData(poi, 0, 0, 0, 0);
        return 1;
    case MSG_ENTER:
        poi = sm.m_Msg.m_Data[0];
        pos = sm.GetActorOwner().GetFeetPos();
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_AL_Courting(StateMachine &sm)
{
    int i = 0;
    EXVector &pos = sm.GetVectorRef(i);
    int &poi = sm.GetIntRef(i);
    int &carry = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (POI_GetActorCount(poi, 0xd) >= 2) {
            if (POI_GetComData(poi, 0, 0, 0) > 0 && POI_GetComData(poi, 0, 0, 0) != sm.GetActorOwner().GetRefID()) {
                if (Actor::GetPtrFromID(POI_GetComData(poi, 0, 0, 0)))
                    sm.GetActorOwner().SetAnimType(0x9000020, 0, 1);
            } else if (POI_ActorPresent(poi, sm.GetActorOwner().GetRefID())) {
                if (!carry) {
                    carry = ActorCarryable::Create(0x100003c, 0x20000b4)->GetRefID();
                    sm.GetActorOwner().SetCarry(carry, 1);
                }
                POI_SetComData(poi, sm.GetActorOwner().GetRefID(), 0, 0, 0);
                sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
                sm.GetActorOwner().SetAnimType(0x9000021, 0, 1);
            }
        } else {
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        }
        return 1;
    case MSG_EXIT:
        if (POI_GetComData(poi, 0, 0, 0) == sm.GetActorOwner().GetRefID()) {
            if (sm.GetActorOwner().GetCarryPtr(1)) sm.GetActorOwner().GetCarryPtr(1)->Delete();
            sm.GetActorOwner().SetCarry(0, 1);
            POI_SetComData(poi, 0, 0, 0, 0);
        }
        return 1;
    case MSG_ENTER:
        poi = sm.m_Msg.m_Data[0];
        pos = sm.GetActorOwner().GetFeetPos();
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_AL_Loiter(StateMachine &sm)
{
    int i = 0;
    EXVector &pos = sm.GetVectorRef(i);
    int &poi = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (POI_GetActorCount(poi, 0xd) >= 2) {
            if (POI_GetComData(poi, 0, 0, 0) > 0 && POI_GetComData(poi, 0, 0, 0) != sm.GetActorOwner().GetRefID()) {
                if (Actor::GetPtrFromID(POI_GetComData(poi, 0, 0, 0)))
                    sm.GetActorOwner().SetAnimType(0x90000e4, 0, 1);
            } else if (POI_ActorPresent(poi, sm.GetActorOwner().GetRefID())) {
                POI_SetComData(poi, sm.GetActorOwner().GetRefID(), 0, 0, 0);
                sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
                sm.GetActorOwner().SetAnimType(0x90000e3, 0, 1);
                if (sm.GetActorOwner().HasAnimEnded()) POI_SetComData(poi, 0, 0, 0, 0);
            }
        } else {
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        }
        return 1;
    case MSG_EXIT:
        if (POI_GetComData(poi, 0, 0, 0) == sm.GetActorOwner().GetRefID()) POI_SetComData(poi, 0, 0, 0, 0);
        return 1;
    case MSG_ENTER:
        poi = sm.m_Msg.m_Data[0];
        pos = sm.GetActorOwner().GetFeetPos();
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_AL_Piss(StateMachine &sm)
{
    int i = 0;
    EXVector &pos = sm.GetVectorRef(i);
    int &poi = sm.GetIntRef(i);
    int &bTimer = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (!bTimer) {
            bTimer = 1;
            timer = (g_EXRandClass.Randf() + 1.0f) * 600.0f;
        }
        timer -= XApp::GameFrameRateMul();
        if (timer <= 0.0f) {
            sm.GetActorOwner().Speak(0x1af004b4, 0);
            bTimer = 0;
        }
        sm.GetActorOwner().SetAnimType(0x9000107, 0, 1);
        return 1;
    case MSG_ENTER:
        poi = sm.m_Msg.m_Data[0];
        pos = sm.GetActorOwner().GetFeetPos();
        sm.GetActorOwner().SetCarry(ActorCarryable::Create(0x100003c, 0x20000c9)->GetRefID(), 1);
        return 1;
    case MSG_EXIT:
    case MSG_SUSPEND:
        if (sm.GetActorOwner().GetCarryPtr(1)) sm.GetActorOwner().GetCarryPtr(1)->Delete();
        sm.GetActorOwner().SetCarry(0, 1);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_AL_SleepDoorway(StateMachine &sm)
{
    int i = 0;
    EXVector &pos = sm.GetVectorRef(i);
    int &poi = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        switch (stage) {
        case 0:
            stage = 1;
            sm.GetActorOwner().SetAnimType(0x9000176, 0, 1);
            break;
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().SetAnimType(0x9000177, 0, 1);
                sm.GetActorOwner().SetIsLyingBum(1);
            }
            break;
        }
        return 1;
    case MSG_EXIT:
        sm.GetActorOwner().SetIsLyingBum(0);
        return 1;
    case MSG_ENTER:
        poi = sm.m_Msg.m_Data[0];
        pos = sm.GetActorOwner().GetFeetPos();
        stage = 0;
        return 1;
    AU_MSG_MUGGING AU_MSG_SOMEONE_UNDER_ATTACK AU_MSG_ATTACKED_BY_ACTOR AU_MSG_LOUD_NOISE AU_MSG_FIGHT_ME
    AU_MSG_DRIVE_BY AU_MSG_INCOMING_ATTACK AU_MSG_ATTRACT
    case MSG_DEATH_BULLET:
        if (sm.GetActorOwner().IsMsgOnStack(MSG_DEATH_BULLET)) return 1;
        Actor::SendMsg(MsgItem(MSG_SOMEONE_UNDER_ATTACK), sm.GetActorOwner().GetFeetPos(), 10.0f,
                       Actor::GetPtrFromID(sm.m_Msg.m_Data[0]), 0.0f);
        SM_GOSUB(STATE_DeathBullet, 0, 0, 0, 0);
        return 1;
    AU_MSG_SOMEONE_SEEN_PREDATOR AU_MSG_ATTACKED_BY_PREDATOR
    }
    return 0;
}

signed char STATE_AL_WindowShop(StateMachine &sm)
{
    int i = 0;
    EXVector &pos = sm.GetVectorRef(i);
    int &poi = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        sm.GetActorOwner().SetAnimType(0x90000c2, 0, 1);
        return 1;
    case MSG_ENTER:
        poi = sm.m_Msg.m_Data[0];
        pos = sm.GetActorOwner().GetFeetPos();
        sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_AL_Gamble(StateMachine &sm)
{
    int i = 0;
    EXVector &pos = sm.GetVectorRef(i);
    int &poi = sm.GetIntRef(i);
    int &bNewAnim = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (POI_GetActorCount(poi, 0xd) >= 2) {
            if (POI_GetComData(poi, 0, 0, 0) > 0 && POI_GetComData(poi, 0, 0, 0) != sm.GetActorOwner().GetRefID()) {
                if (bNewAnim || sm.GetActorOwner().HasAnimEnded()) {
                    bNewAnim = 0;
                    int anims[2] = { 0x9000028, 0x9000029 };
                    int r = g_EXRandClass.Rand32() & 1;
                    sm.GetActorOwner().SetAnimType(anims[r], 0, 1);
                }
                Actor *pOther = Actor::GetPtrFromID(POI_GetComData(poi, 0, 0, 0));
                if (pOther) sm.GetActorOwner().SpinToYRot(pOther->GetHeadPos());
            } else if (POI_ActorPresent(poi, sm.GetActorOwner().GetRefID())) {
                POI_SetComData(poi, sm.GetActorOwner().GetRefID(), 0, 0, 0);
                if (bNewAnim || sm.GetActorOwner().HasAnimEnded()) {
                    bNewAnim = 0;
                    sm.GetActorOwner().SetAnimType(0x900004f, 0, 1);
                }
                sm.GetActorOwner().SpinToYRot(POI_Get(poi)->m_Pos);
            }
        } else {
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        }
        return 1;
    case MSG_EXIT:
        if (POI_GetComData(poi, 0, 0, 0) == sm.GetActorOwner().GetRefID()) POI_SetComData(poi, 0, 0, 0, 0);
        return 1;
    case MSG_ENTER:
        poi = sm.m_Msg.m_Data[0];
        pos = sm.GetActorOwner().GetFeetPos();
        bNewAnim = 1;
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}
