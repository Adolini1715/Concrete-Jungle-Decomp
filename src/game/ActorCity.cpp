#include "Actor.h"
#include "ActorUtil.h"
#include "ActorLocation.h"
#include "StateMachine.h"
#include "EXRand.h"
#include "ActorProfile.h"
#include "XApp.h"
#include "ActorCarryable.h"
#include "FEText.h"
#include "EVD.h"
#include "XItemHandler_Graffiti.h"
#include "XItem.h"
#include "ActorCombat.h"
#include "XItemHandler_Player.h"
#include "SubGroupAnims.h"

// City NPC behaviour states (ActorCity.obj).

extern unsigned int gCurrentMap_GeoHash;

signed char STATE_FS_ChatExchange(StateMachine &sm)
{
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
    case MSG_EXT:
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_FS_DriveBy(StateMachine &sm)
{
    sm.GetActorOwner().SetStatus(3);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER: {
        Actor &actor = sm.GetActorOwner();
        actor.Speak(actor.GetScreamSFX(), 0);
        SM_GOSUB(STATE_DeathBullet, 0, 0, 0, 0);
        return 1;
    }
    case MSG_EXIT:
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_FS_IdlingProstitutes(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        return 1;
    case MSG_EXT:
        if (sm.GetActorOwner().HasAnimEnded()) {
            POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
            POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        }
        return 1;
    case MSG_RETURN:
        return 1;
    case MSG_EXIT:
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_FS_ShadyDeal(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        return 1;
    case MSG_EXT:
        if (sm.GetActorOwner().HasAnimEnded()) {
            POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
            POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        }
        return 1;
    case MSG_RETURN:
        return 1;
    case MSG_EXIT:
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// NEAR: 30 bytes, the 0x4e000001 else-arm is laid out inline instead of after the tail.
signed char STATE_FS_BuyHookerAlt(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(6);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (POI_GetActorCount(poi, LocCount::BOUND | LocCount::VENDORS) > 0)
            sm.GetActorOwner().SetAnimType(0x90000e3, 0, 1);
        else
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        if (sm.GetActorOwner().HasAnimEnded()) sm.Return(0, 0, 0, 0);
        return 1;
    case MSG_EXIT:
        POI_Unbind(poi, sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_ENTER:
        poi = sm.m_Msg.m_Data[0];
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_CityDistractions(StateMachine &sm)
{
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
    case MSG_RETURN: {
        int poi = 0;
        int type = 0;
        sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        AP_GetRandPOI(&sm.GetActorOwner(), poi, type);
        SM_GOSUB(STATE_GotoLocation, poi, type, 0, 0);
        return 1;
    }
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_FS_HookerAlt(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &bTimer = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(6);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (POI_GetActorCount(poi, LocCount::BOUND | LocCount::CUSTOMERS) > 0) {
            sm.GetActorOwner().SetAnimType(0x90000e4, 0, 1);
            return 1;
        }
        sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        if (!bTimer) {
            bTimer = 1;
            timer = (g_EXRandClass.Randf() + 1.0f) * 600.0f;
        }
        timer -= XApp::GameFrameRateMul();
        if (timer <= 0.0f) {
            sm.GetActorOwner().Speak(0x1af0049e, 0);
            bTimer = 0;
        }
        return 1;
    case MSG_ENTER:
        poi = sm.m_Msg.m_Data[0];
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_FS_Loiter(StateMachine &sm)
{
    int i = 0;
    int &bDone = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (!bDone) {
            timer -= XApp::GameFrameRateMul();
            if (timer <= 0.0f) {
                POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
                POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
            }
        }
        return 1;
    case MSG_EXIT:
        POI_Unbind(poi, sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_ENTER:
        bDone = 0;
        poi = sm.m_Msg.m_Data[0];
        timer = g_EXRandClass.Randf() * 600.0f + 300.0f;
        sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// NEAR: 5 bytes, retail spills the (EXRand32() & 1) result to the parameter slot before testing it.
signed char STATE_FS_GoDrinking(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        switch (stage) {
        case 0:
            stage = 1;
            timer = (g_EXRandClass.Randf() + 1.0f) * 600.0f;
            sm.GetActorOwner().SetHide(1);
            break;
        case 1:
            timer -= XApp::GameFrameRateMul();
            if (timer <= 0.0f) {
                int r = EXRand32() & 1;
                if (!r) sm.GetActorOwner().m_Attrib |= 2;
                POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
                POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
            }
            break;
        }
        return 1;
    case MSG_EXIT:
        sm.GetActorOwner().SetHide(0);
        return 1;
    case MSG_ENTER:
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_FS_GoShopping(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        switch (stage) {
        case 0:
            stage = 1;
            timer = (g_EXRandClass.Randf() + 1.0f) * 600.0f;
            sm.GetActorOwner().SetHide(1);
            break;
        case 1:
            timer -= XApp::GameFrameRateMul();
            if (timer <= 0.0f) {
                if (EXRand32() % 3) sm.GetActorOwner().SetCarry(ActorCarryable::Create(0x100003c, 0x20000c4)->GetRefID(), 0);
                POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
                POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
            }
            break;
        }
        return 1;
    case MSG_EXIT:
        sm.GetActorOwner().SetHide(0);
        return 1;
    case MSG_ENTER:
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_FS_WindowShop(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        switch (stage) {
        case 0:
            if (sm.GetActorOwner().IsFacingYRot(POI_Get(poi)->m_YRot, 5.0f)) {
                stage = 1;
                sm.GetActorOwner().SetAnimType(0x90000c2, 0, 1);
                timer = (g_EXRandClass.Randf() + 1.0f) * 300.0f;
                break;
            }
            sm.GetActorOwner().SetTargetYRot(POI_Get(poi)->m_YRot, 5.0f);
            break;
        case 1:
            timer -= XApp::GameFrameRateMul();
            if (timer <= 0.0f) {
                POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
                POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
                break;
            }
            break;
        }
        return 1;
    case MSG_EXIT:
        POI_CustomerUnbind(poi, sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_ENTER:
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        sm.GetActorOwner().InvalidateGotoPos();
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_FS_Piss(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        switch (stage) {
        case 0:
            stage = 2;
            sm.GetActorOwner().Speak(0x1af004b4, 0);
            sm.GetActorOwner().SetCarry(ActorCarryable::Create(0x100003c, 0x20000c9)->GetRefID(), 1);
            sm.GetActorOwner().SetAnimType(0x9000107, 0, 1);
            break;
        case 2:
            if (sm.GetActorOwner().HasAnimEnded()) {
                POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
                POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
            }
            break;
        }
        return 1;
    case MSG_EXIT:
        if (sm.GetActorOwner().GetCarryPtr(1)) sm.GetActorOwner().GetCarryPtr(1)->Delete();
        sm.GetActorOwner().SetCarry(0, 1);
        POI_Unbind(poi, sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_ENTER:
        if (sm.GetActorOwner().GetAttrib() & 0x10) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        return 1;
    case MSG_SUSPEND:
        if (sm.GetActorOwner().GetCarryPtr(1)) sm.GetActorOwner().GetCarryPtr(1)->Delete();
        sm.GetActorOwner().SetCarry(0, 1);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// NEAR: 22 bytes, register choice only (eax/ecx/edx) in the ENTER and default-message blocks.
signed char STATE_FS_CrowdCircle(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    int &bStarted = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        timer -= XApp::GameFrameRateMul();
        if (timer <= 0.0f) {
            POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
            POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        }
        if (sm.GetActorOwner().HasAnimEnded() || !bStarted) {
            bStarted = 1;
            int anims[4] = {0x900004e, 0x9000100, 0x900031c, 0x4e000001};
            int anim = anims[EXRand32() & 3];
            sm.GetActorOwner().SetAnimType(anim, 0, 1);
        }
        return 1;
    case MSG_ENTER:
        poi = sm.m_Msg.m_Data[0];
        timer = 1800.0f;
        return 1;
    case MSG_EXIT:
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_BeingMugged(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(3);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        switch (stage) {
        case 0:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().SetAnimType(0x90004b2, 0, 1);
                stage = 1;
            }
            break;
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().SetAnimType(0x90003b6, 0, 1);
                stage = 2;
            }
            break;
        case 2:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 3;
                SM_GOSUB(STATE_Panic, 0, 0, 0, 0);
            }
            break;
        case 3:
            POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
            break;
        }
        return 1;
    case MSG_ENTER:
        stage = 0;
        sm.GetActorOwner().SetCarry(0, 1);
        sm.GetActorOwner().SetCarry(0, 0);
        sm.GetActorOwner().SetAnimType(0x9000018, 0, 1);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// NEAR: retail cross-jumps the message-0x15 case into the MSG_RETURN ReturnToHubLife block in place.
signed char STATE_FS_BreakAndEnterAccomplice(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(6);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        if (!sm.m_Msg.m_Data[0]) {
        hub:
            POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        }
        return 1;
    case MSG_EXIT:
        if (sm.GetActorOwner().GetCarryPtr(1)) sm.GetActorOwner().GetCarryPtr(1)->Delete();
        sm.GetActorOwner().SetCarry(0, 1);
        if (sm.GetActorOwner().GetCarryPtr(0)) sm.GetActorOwner().GetCarryPtr(0)->Delete();
        sm.GetActorOwner().SetCarry(0, 0);
        sm.GetActorOwner().SetHide(0);
        POI_CustomerUnbind(poi, sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_ENTER:
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        SM_GOSUB(STATE_CrimePrepareAccomplice, poi, 0, 0, 0);
        return 1;
    case MSG_EXT:
        sm.GetActorOwner().SetStatus(7);
        switch (stage) {
        case 0:
            stage = 1;
            sm.GetActorOwner().SetAnimType(0x9000108, 0, 1);
            break;
        case 1: {
            int target = 0;
            if (POI_GetComData(poi, &target, 0, 0) == 5000) {
                stage = 2;
                SM_GOSUB(STATE_GotoLocation, target, 0x2310, 0, 0);
            }
            break;
        }
        case 2:
            sm.GetActorOwner().Delete();
            break;
        }
        return 1;
    AU_DEFAULT_MSGS
    case (MsgType)0x15:  // TODO name
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x15)) return 1;
        goto hub;
    }
    return 0;
}

// NEAR: 42 bytes, register rotation shifted by one from the customer SetAnimType on (eax/ecx/edx).
signed char STATE_FS_CheckpointCharlie(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &bVendor = sm.GetIntRef(i);
    int &bCustomer = sm.GetIntRef(i);
    int &bStarted = sm.GetIntRef(i);
    int &anim = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        return 1;
    case MSG_ENTER: {
        poi = sm.m_Msg.m_Data[0];
        timer = 1800.0f;
        bVendor = POI_VendorGetID(poi, 0) == sm.GetActorOwner().GetRefID() ? 1 : 0;
        bCustomer = POI_CustomerGetID(poi, 0) == sm.GetActorOwner().GetRefID() ? 1 : 0;
        sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        int anims[2] = {0x9000149, 0x4e000001};
        anim = anims[EXRand32() & 1];
        return 1;
    }
    case MSG_EXT:
        timer -= XApp::GameFrameRateMul();
        if (timer <= 0.0f) POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
        if (!bStarted || sm.GetActorOwner().HasAnimEnded()) {
            bStarted = 1;
            if (bVendor) {
                sm.GetActorOwner().SetAnimType(0x9000100, 0, 1);
                return 1;
            }
            if (bCustomer) {
                int anims[2] = {0x9000329, 0x9000327};
                int a = anims[EXRand32() & 1];
                sm.GetActorOwner().SetAnimType(a, 0, 1);
                return 1;
            }
            sm.GetActorOwner().SetAnimType(anim, 0, 1);
        }
        return 1;
    case MSG_EXIT:
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// NEAR: 4 bytes, retail spills (EXRand32() & 1) to the parameter slot before testing it (as GoDrinking).
signed char STATE_FS_Trashcan(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        stage = 0;
        return 1;
    case MSG_EXIT:
        POI_Unbind(poi, sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_ENTER:
        if (sm.GetActorOwner().GetAttrib() & 0x10) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        sm.GetActorOwner().InvalidateGotoPos();
        sm.GetActorOwner().Speak(0x1af004b3, 0);
        return 1;
    case MSG_EXT:
        switch (stage) {
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) {
                int r = EXRand32() & 1;
                if (r) {
                    int items[3] = {0x20000b6, 0x20000bb, 0x20000b5};
                    sm.GetActorOwner().SetCarry(ActorCarryable::Create(0x100003c, items[EXRand32() % 3])->GetRefID(), 0);
                }
                POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
                POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
                return 1;
            }
            break;
        case 0:
            if (sm.GetActorOwner().IsFacingYRot(POI_Get(poi)->m_Pos, 5.0f)) {
                stage = 1;
                sm.GetActorOwner().SetAnimType(0x90000e9, 0, 1);
            } else {
            sm.GetActorOwner().SetTargetYRot(POI_Get(poi)->m_Pos, 5.0f);
            }
            break;
        }
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// NEAR: retail keeps the 1-entry anim array in the param slot and indexes it with a zeroed eax (also pushed
// as SetAnimType's 2nd arg); VC7.1 folds anims[EXRand32() % 1] here, so the customer tail cross-jumps.
signed char STATE_FS_Reprimand(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &bFirst = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    int &bVendor = sm.GetIntRef(i);
    float &speakTimer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        return 1;
    case MSG_EXIT:
        if (sm.GetActorOwner().GetRefID() == POI_GetComData(poi, 0, 0, 0)) POI_SetComData(poi, 0, 0, 0, 0);
        if (gCurrentMap_GeoHash == 0x10000b4) sm.GetActorOwner().SpeakStop();
        return 1;
    case MSG_ENTER:
        poi = sm.m_Msg.m_Data[0];
        timer = 1800.0f;
        bFirst = 1;
        bVendor = POI_IsVendor(poi, sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_EXT:
        timer -= XApp::GameFrameRateMul();
        if (timer <= 0.0f) {
            POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
            POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        }
        if (POI_GetActorCount(poi, LocCount::BOUND | LocCount::VENDORS | LocCount::CUSTOMERS) >= 3) {
            if (bVendor) {
                if (gCurrentMap_GeoHash == 0x10000b4) {
                    speakTimer -= XApp::GameFrameRateMul();
                    if (speakTimer <= 0.0f && !sm.GetActorOwner().IsSpeaking()) {
                        speakTimer = 360.0f;
                        sm.GetActorOwner().Speak(0x1af00620, 0);
                    }
                }
                if (sm.GetActorOwner().HasAnimEnded() || bFirst) sm.GetActorOwner().SetAnimType(0x9000100, 0, 1);
                bFirst = 0;
                return 1;
            }
            if (sm.GetActorOwner().HasAnimEnded() || bFirst) {
                int anims[1] = {0x4e000001};
                int a = anims[EXRand32() % 1];
                sm.GetActorOwner().SetAnimType(a, 0, 1);
            }
            bFirst = 0;
            return 1;
        }
        sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_FS_SleepDoorway(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        switch (stage) {
        case 0:
            stage = 1;
            timer = 3600.0f;
            sm.GetActorOwner().SetAnimType(0x9000176, 0, 1);
            break;
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 2;
                sm.GetActorOwner().SetAnimType(0x9000177, 0, 1);
                sm.GetActorOwner().SetIsLyingBum(1);
            }
            break;
        case 2:
            timer -= XApp::GameFrameRateMul();
            if (timer <= 0.0f) {
                POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
                POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
            }
            break;
        }
        return 1;
    case MSG_EXIT:
        POI_VendorUnbind(poi, sm.GetActorOwner().GetRefID());
        sm.GetActorOwner().SetIsLyingBum(0);
        return 1;
    case MSG_ENTER:
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        sm.GetActorOwner().Speak(0x1af004b3, 0);
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

// NEAR: retail's inner-switch exits (stage 3/4 not ready, bad stage) jump back to the ENTER Gosub's
// return-1 block; ours go to the last one in EXT. Otherwise identical.
signed char STATE_FS_BankRobberyAccomplice(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    int &dest = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(6);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        if (!sm.m_Msg.m_Data[0]) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_EXIT:
        POI_Unbind(poi, sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_ENTER: {
        stage = 2;
        poi = sm.m_Msg.m_Data[0];
        timer = 1800.0f;
        Actor &actor = sm.GetActorOwner();
        dest = POI_GetNearestType(POI_Get(poi)->m_Pos, 0x42000001, 0, actor.GetNavZone(), actor.CanJump(), 0);
        if (!dest) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        SM_GOSUB(STATE_CrimePrepareAccomplice, poi, 0, 0, 0);
        return 1;
    }
    case MSG_EXT:
        switch (stage) {
        case 2:
            stage = 3;
            sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
            sm.GetActorOwner().GotoPOI(dest);
            break;
        case 3:
            if (sm.GetActorOwner().SqDistToTarget() < 0.01f) {
                stage = 4;
                sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                sm.GetActorOwner().SetHide(1);
            }
            break;
        case 4: {
            int target = 0;
            if (POI_GetComData(poi, &target, 0, 0) != 5000) {
                timer -= XApp::GameFrameRateMul();
                if (!(timer <= 0.0f)) break;
            }
            stage = 5;
            sm.GetActorOwner().SetHide(0);
            SM_GOSUB(STATE_GotoLocation, target, 0x2310, 0, 0);
            break;
        }
        case 5:
            sm.GetActorOwner().Delete();
            break;
        }
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// Retail inlined EXVector::operator- here; VC7.1 will not, so a local forced copy stands in.
static __forceinline EXMaths::vec_xyzw VSub(const EXMaths::vec_xyzw &a, const EXMaths::vec_xyzw &b)
{
    EXMaths::vec_xyzw r;
    r.x = a.x - b.x; r.y = a.y - b.y; r.z = a.z - b.z; r.w = a.w - b.w;
    return r;
}

// NEAR: retail emits the SOMEONE_SEEN_PREDATOR case (jump-table index 0 here) right after ATTACKED_BY_ACTOR
// and LOUD_NOISE/ATTRACT share its return; ours keeps macro order. Rest identical.
signed char STATE_FS_Hustler(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(6);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        if (!sm.m_Msg.m_Data[0]) {
            POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
            return 1;
        }
        stage = 0;
        return 1;
    case MSG_EXIT:
        POI_VendorUnbind(poi, sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_ENTER:
        if (sm.GetActorOwner().GetAttrib() & 6) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        return 1;
    case MSG_EXT:
        switch (stage) {
        case 0:
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            break;
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) stage = 0;
            break;
        }
        return 1;
    case MSG_PLAY_ANIM:  // TODO name: data = anim, customer id
        if (sm.GetActorOwner().IsMsgOnStack(MSG_PLAY_ANIM)) return 1;
        stage = 1;
        sm.GetActorOwner().SetAnimType(sm.m_Msg.m_Data[0], 0, 1);
        {
            EXVector dz = VSub(Actor::GetPtrFromID(sm.m_Msg.m_Data[1])->GetFeetPos(), sm.GetActorOwner().GetHeadPos());
            EXVector dx = VSub(Actor::GetPtrFromID(sm.m_Msg.m_Data[1])->GetFeetPos(), sm.GetActorOwner().GetHeadPos());
            sm.GetActorOwner().SetRotY((float)atan2(dx.x, dz.z));
        }
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// NEAR: retail keeps talkAnims[EXRand32() % 1] as a param-slot array load (VC7.1 folds it here, as in
// Reprimand), and loads the actor after the gatherAnims index.
signed char STATE_FS_TrampGather(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    int talkAnims[1] = {0x9000178};
    int gatherAnims[4] = {0x9000171, 0x9000172, 0x9000173, 0x9000174};
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        stage = 0;
        return 1;
    case MSG_EXIT:
        if (POI_Unbind(poi, sm.GetActorOwner().GetRefID())) POI_SetComData(poi, POI_GetComData(poi, 0, 0, 0) - 1, 0, 0, 0);
        return 1;
    case MSG_ENTER:
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        sm.GetActorOwner().InvalidateGotoPos();
        if (!poi) sm.Return(0, 0, 0, 0);
        return 1;
    case MSG_EXT:
        switch (stage) {
        case 0:
            stage = 1;
            timer = 1800.0f;
            POI_SetComData(poi, POI_GetComData(poi, 0, 0, 0) + 1, 0, 0, 0);
            break;
        case 1:
            timer -= XApp::GameFrameRateMul();
            if (timer <= 0.0f) {
                POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
                POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
            }
            if (sm.GetActorOwner().IsFacingYRot(POI_Get(poi)->m_Pos, 5.0f)) {
                stage = 2;
                if (POI_GetComData(poi, 0, 0, 0) >= 3) {
                    sm.GetActorOwner().SpeakStop();
                    int a = gatherAnims[EXRand32() & 3];
                    sm.GetActorOwner().SetAnimType(a, 0, 1);
                    break;
                }
                sm.GetActorOwner().Speak(0x1af004b3, 0);
                int a = talkAnims[EXRand32() % 1];
                sm.GetActorOwner().SetAnimType(a, 0, 1);
                break;
            }
            sm.GetActorOwner().SetTargetYRot(POI_Get(poi)->m_Pos, 5.0f);
            break;
        case 2:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 1;
                break;
            }
            break;
        }
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// NEAR: register rotation differs from the EXT block on (eax/ecx/edx), and VC hoists the actor load
// above the stage-1 null test (3 bytes).
signed char STATE_FS_Chat(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    int &other = sm.GetIntRef(i);
    EXVector &pos = sm.GetVectorRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        if (!sm.m_Msg.m_Data[0]) {
            POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
            return 1;
        }
        stage = 0;
        return 1;
    case MSG_EXIT:
        POI_CustomerUnbind(poi, sm.GetActorOwner().GetRefID());
        POI_VendorUnbind(poi, sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_ENTER:
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        pos = sm.GetActorOwner().GetFeetPos();
        sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        sm.GetActorOwner().InvalidateGotoPos();
        return 1;
    case MSG_EXT: {
        Actor *pOther = Actor::GetPtrFromID(other);
        switch (stage) {
        case 0:
            AU_EnsureAtTarget(sm, pos, 0.25f);
            if (POI_GetActorCount(poi, LocCount::BOUND | LocCount::VENDORS | LocCount::CUSTOMERS) >= 2) {
                other = POI_GetOtherActorID(poi, sm.GetActorOwner().GetRefID());
                stage = 1;
            }
            break;
        case 1:
            if (!pOther) {
                POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
                POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
                return 1;
            }
            if (sm.GetActorOwner().IsFacingYRot(pOther->GetFeetPos(), 5.0f)) {
                stage = 2;
                sm.GetActorOwner().SetAnimType(0x90000e1 + (other < sm.GetActorOwner().GetRefID()), 0, 1);
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(pOther->GetFeetPos(), 5.0f);
            return 1;
        case 2:
            if (!pOther) {
                sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
                POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
                return 1;
            }
            if (sm.GetActorOwner().HasAnimEnded()) {
                POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
                POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
                return 1;
            }
            break;
        }
        return 1;
    }
    AU_DEFAULT_MSGS
    case (MsgType)0x15:  // TODO name
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x15)) return 1;
        POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        return 1;
    }
    return 0;
}

// NEAR: same SOMEONE_SEEN_PREDATOR placement issue as Hustler (states with message 8 split the tree at 0xb
// and retail lays 0xc out right after ATTACKED_BY_ACTOR while keeping its return as the shared one).
signed char STATE_FS_SmokingBreak(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        stage = 0;
        return 1;
    case MSG_EXIT:
        if (POI_GetComData(poi, 0, 0, 0) == sm.GetActorOwner().GetRefID()) POI_SetComData(poi, 0, 0, 0, 0);
        return 1;
    case MSG_ENTER:
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        timer = 1800.0f;
        return 1;
    case MSG_EXT:
        timer -= XApp::GameFrameRateMul();
        if (timer <= 0.0f) {
            POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
            POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        }
        switch (stage) {
        case 0:
            if (POI_GetActorCount(poi, LocCount::BOUND | LocCount::VENDORS | LocCount::CUSTOMERS) >= 2) {
                stage = 1;
                if (POI_GetComData(poi, 0, 0, 0)) {
                    sm.GetActorOwner().SetAnimType(0x90000e4, 0, 1);
                    return 1;
                }
                POI_SetComData(poi, sm.GetActorOwner().GetRefID(), 0, 0, 0);
                sm.GetActorOwner().SetAnimType(0x90000e3, 0, 1);
            }
            break;
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 2;
                if (POI_GetComData(poi, 0, 0, 0) == sm.GetActorOwner().GetRefID()) {
                    sm.GetActorOwner().SetAnimType(0x9000066, 0, 1);
                    return 1;
                }
                sm.GetActorOwner().SetAnimType(0x900031c, 0, 1);
                return 1;
            }
            break;
        case 2:
            if (sm.GetActorOwner().HasAnimEnded()) {
                if (!(EXRand32() % 6)) {
                    sm.GetActorOwner().SetAnimType(0x90000e3, 0, 1);
                    POI_SendMsg(poi, sm.GetActorOwner().GetRefID(), MsgItem(MSG_PLAY_ANIM, 0x90000e4));
                    return 1;
                }
                sm.GetActorOwner().SetAnimType(0x9000067, 0, 1);
                return 1;
            }
            break;
        }
        return 1;
    case MSG_PLAY_ANIM:  // TODO name: play anim data[0]
        if (sm.GetActorOwner().IsMsgOnStack(MSG_PLAY_ANIM)) return 1;
        sm.GetActorOwner().SetAnimType(sm.m_Msg.m_Data[0], 0, 1);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_FS_Pimp(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(6);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        if (!sm.m_Msg.m_Data[0]) {
            POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
            return 1;
        }
        stage = 0;
        return 1;
    case MSG_EXIT:
        POI_CustomerUnbind(poi, sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_ENTER:
        if (sm.GetActorOwner().GetAttrib() & 0x16) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        sm.GetActorOwner().InvalidateGotoPos();
        return 1;
    case MSG_EXT:
        switch (stage) {
        case 0:
            if (sm.GetActorOwner().IsFacingYRot(POI_VendorStandPos(poi, 0), 5.0f)) {
                sm.GetActorOwner().SetAnimType(0x90000e3, 0, 1);
                stage = 1;
                break;
            }
            sm.GetActorOwner().SetTargetYRot(POI_VendorStandPos(poi, 0), 5.0f);
            break;
        case 1:
            if (!POI_VendorIsThere(poi)) {
                POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
                break;
            }
            if (sm.GetActorOwner().HasAnimEnded()) {
                if (!(EXRand32() % 3)) {
                    sm.GetActorOwner().SetAnimType(0x9000132, 0, 1);
                    stage = 2;
                    break;
                }
                sm.GetActorOwner().SetAnimType(0x90000a4, 0, 1);
                stage = 3;
                break;
            }
            POI_SetComData(poi, 1, 0, 0, 0);
            break;
        case 2:
        case 3:
            if (!POI_VendorIsThere(poi)) {
                POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
                POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
                break;
            }
            if (sm.GetActorOwner().HasAnimEnded()) {
                POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
                POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
                break;
            }
            if (stage == 2) {
                POI_SetComData(poi, 4, 0, 0, 0);
                break;
            }
            POI_SetComData(poi, 5, 0, 0, 0);
            break;
        }
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_FS_ATM(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        switch (stage) {
        case 0:
            stage = 1;
            SM_GOSUB(STATE_CustomerQueue, poi, 1, 0, 0);
            break;
        case 1: {
            stage = 2;
            EXVector dz = VSub(POI_Get(poi)->m_Pos, sm.GetActorOwner().GetHeadPos());
            EXVector dx = VSub(POI_Get(poi)->m_Pos, sm.GetActorOwner().GetHeadPos());
            sm.GetActorOwner().SetRotY((float)atan2(dx.x, dz.z));
            sm.GetActorOwner().SetAnimType(0x9000022, 0, 1);
            break;
        }
        case 2:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 3;
                timer = 120.0f;
                sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            }
            break;
        case 3:
            timer -= XApp::GameFrameRateMul();
            if (timer <= 0.0f) {
                POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
                POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
            }
            break;
        }
        return 1;
    case MSG_EXIT:
        POI_CustomerUnbind(poi, sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_ENTER:
        if (sm.GetActorOwner().GetAttrib() & 0x16) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        poi = sm.m_Msg.m_Data[0];
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_FS_VendingMachine(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        switch (stage) {
        case 0:
            stage = 1;
            SM_GOSUB(STATE_CustomerQueue, poi, 1, 0, 0);
            return 1;
        case 1:
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(POI_Get(poi)->m_YRot, 5.0f)) {
                stage = 2;
                sm.GetActorOwner().SetAnimType(0x9000022, 0, 1);
                break;
            }
            sm.GetActorOwner().SetTargetYRot(POI_Get(poi)->m_YRot, 5.0f);
            return 1;
        case 2:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 3;
                sm.GetActorOwner().SetAnimType(0x90000a4, 0, 1);
            }
            return 1;
        case 3:
            if (sm.GetActorOwner().HasAnimEnded()) {
                int items[5] = {0x20000b6, 0x20000b9, 0x20000c0, 0x20000c1, 0x20000c4};
                sm.GetActorOwner().SetCarry(ActorCarryable::Create(0x100003c, items[EXRand32() % 5])->GetRefID(), 0);
                POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
                POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
            }
            return 1;
        }
        return 1;
    case MSG_EXIT:
        POI_CustomerUnbind(poi, sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_ENTER:
        if (sm.GetActorOwner().GetAttrib() & 0x16) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_FS_BuyVendor(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    int &carry = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        switch (stage) {
        case 0:
            stage = 1;
            SM_GOSUB(STATE_CustomerQueue, poi, 0, 0, 0);
            return 1;
        case 1: {
            Actor *pVendor = Actor::GetPtrFromID(POI_VendorGetID(poi, 0));
            if (!pVendor) {
                sm.Return(0, 0, 0, 0);
                break;
            }
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(pVendor->GetFeetPos(), 5.0f)) {
                stage = 2;
                sm.GetActorOwner().SetAnimType(0x90000a2, 0, 1);
                carry = Actor::SendMsg(POI_VendorGetID(poi, 0), MsgItem((MsgType)6), 0.0f);
                if (!carry) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
                break;
            }
            sm.GetActorOwner().SetTargetYRot(pVendor->GetFeetPos(), 5.0f);
            return 1;
        }
        case 2:
            if (!Actor::GetPtrFromID(POI_VendorGetID(poi, 0))) {
                POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
                break;
            }
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 3;
                sm.GetActorOwner().SetAnimType(0x90000a4, 0, 1);
            }
            return 1;
        case 3:
            if (!Actor::GetPtrFromID(POI_VendorGetID(poi, 0))) {
                POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
                break;
            }
            if (sm.GetActorOwner().HasAnimEnded()) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
            return 1;
        }
        return 1;
    case MSG_EXIT:
        POI_CustomerUnbind(poi, sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_ENTER:
        if (sm.GetActorOwner().GetAttrib() & 0x16) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        return 1;
    case (MsgType)0xa:  // TODO name: vendor hands over the item
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0xa)) return 1;
        sm.GetActorOwner().SetCarry(carry, 0);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// NEAR: retail cross-jumps the stage-1 GotoLocation Gosub into the ENTER Gosub (keeps the earlier copy);
// ours keeps the later copy. Same tail-merge direction issue as BankRobberyAccomplice.
signed char STATE_FS_ExtortionAccomplice(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    int &dest = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(6);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        if (!sm.m_Msg.m_Data[0]) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_EXIT:
        POI_CustomerUnbind(poi, sm.GetActorOwner().GetRefID());
        sm.GetActorOwner().Delete();
        return 1;
    case MSG_ENTER: {
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        Actor &actor = sm.GetActorOwner();
        dest = POI_GetNearestFlag(POI_Get(poi)->m_Pos, 0x20, 0, actor.GetNavZone(), actor.CanJump(), 0, -1.0f);
        if (!dest) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        SM_GOSUB(STATE_CrimePrepareAccomplice, poi, 0, 0, 0);
        return 1;
    }
    case MSG_EXT:
        sm.GetActorOwner().SetStatus(7);
        switch (stage) {
        case 0:
            stage = 1;
            sm.GetActorOwner().SetAnimType(0x9000108, 0, 1);
            return 1;
        case 1: {
            int target = 0;
            if (POI_GetComData(poi, 0, 0, 0) == 4000) {
                sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
                if (sm.GetActorOwner().IsFacingYRot(POI_VendorStandPos(poi, 1), 5.0f)) {
                    sm.GetActorOwner().SetAnimType(0x900012b, 0, 1);
                    break;
                }
                sm.GetActorOwner().SetTargetYRot(POI_VendorStandPos(poi, 1), 5.0f);
                break;
            }
            if (POI_GetComData(poi, &target, 0, 0) == 5000) {
                stage = 2;
                SM_GOSUB(STATE_GotoLocation, target, 0x10, 0, 0);
            }
            return 1;
        }
        case 2:
            sm.GetActorOwner().Delete();
            break;
        }
        return 1;
    AU_DEFAULT_MSGS
    case (MsgType)0x15:  // TODO name
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x15)) return 1;
        POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        return 1;
    }
    return 0;
}

// Bank robber: prepares with the crime POI, runs to the nearest bank and hides inside for 600
// frames, then flags the POI (com data 5000 = done, with the home it flees to) and runs home
// to be deleted.
signed char STATE_FS_BankRobbery(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    int &bank = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(6);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        if (!sm.m_Msg.m_Data[0]) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_EXIT:
        POI_Unbind(bank, sm.GetActorOwner().GetRefID());
        POI_SetComData(bank, 0, 0, 0, 0);
        return 1;
    case MSG_ENTER: {
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        timer = 600.0f;
        Actor &actor = sm.GetActorOwner();
        bank = POI_GetNearestType(POI_Get(poi)->m_Pos, 0x42000001, 0, actor.GetNavZone(), actor.CanJump(), 0);
        if (!bank) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        SM_GOSUB(STATE_CrimePrepare, poi, 0, 0, 0);
        return 1;
    }
    case MSG_EXT:
        switch (stage) {
        case 0:
            stage = 1;
            sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
            sm.GetActorOwner().GotoPOI(bank);
            break;
        case 1:
            if (sm.GetActorOwner().SqDistToTarget() < 0.01f) {
                stage = 2;
                sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                sm.GetActorOwner().SetHide(1);
            }
            break;
        case 2: {
            timer -= XApp::GameFrameRateMul();
            if (!(timer <= 0.0f)) break;
            stage = 3;
            sm.GetActorOwner().SetHide(0);
            POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
            Actor &actor = sm.GetActorOwner();
            int home = POI_GetRandomType(0x42000004, 0, actor.GetNavZone(), actor.CanJump(), 0);
            POI_SetComData(poi, 5000, home, 0, 0);
            SM_GOSUB(STATE_GotoLocation, home, 0x2310, 0, 0);
            break;
        }
        case 3:
            sm.GetActorOwner().Delete();
            break;
        }
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// Drug dealer at its vendor POI: plays the deal animations the customer signals through the
// POI com data (1 chat, 2 chat with a line, 3 take, 4 hand over with a line), otherwise stays
// on its spot and mutters every 600 to 1200 frames. One of two voice sets. A customer's
// request is answered with a new syringe.
signed char STATE_FS_DrugDealer(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &carry = sm.GetIntRef(i);
    int &lastCom = sm.GetIntRef(i);
    EXVector &pos = sm.GetVectorRef(i);
    int &voice = sm.GetIntRef(i);
    int &bTimer = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(6);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT: {
        int com = POI_GetComData(poi, 0, 0, 0);
        switch (com) {
        case 1:
            sm.GetActorOwner().SetAnimType(0x90000e1, 0, 1);
            break;
        case 2:
            if (lastCom != com) sm.GetActorOwner().Speak(voice ? 0x1af00567 : 0x1af00440, 0);
            sm.GetActorOwner().SetAnimType(0x90000e2, 0, 1);
            break;
        case 3:
            sm.GetActorOwner().SetAnimType(0x90000a4, 0, 1);
            break;
        case 4:
            if (lastCom != com) sm.GetActorOwner().Speak(voice ? 0x1af00568 : 0x1af00441, 0);
            sm.GetActorOwner().SetAnimType(0x90000a2, 0, 1);
            break;
        default:
            AU_EnsureAtTarget(sm, pos, 0.25f);
            if (!bTimer) {
                bTimer = 1;
                timer = (EXRandf() + 1.0f) * 600.0f;
            }
            timer -= XApp::GameFrameRateMul();
            if (timer <= 0.0f) {
                sm.GetActorOwner().Speak(voice ? 0x1af00566 : 0x1af0043f, 0);
                bTimer = 0;
            }
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            break;
        }
        lastCom = com;
        POI_SetComData(poi, 0, 0, 0, 0);
        return 1;
    }
    case MSG_ENTER:
        if (sm.GetActorOwner().GetAttrib() & 0x16) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        sm.GetActorOwner().InvalidateGotoPos();
        poi = sm.m_Msg.m_Data[0];
        pos = sm.GetActorOwner().GetFeetPos();
        voice = EXRand32() & 1;
        return 1;
    case MSG_EXIT:
        POI_VendorUnbind(poi, sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_RETURN:
        if (!sm.m_Msg.m_Data[0]) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_REQUEST_ITEM:
        if (sm.GetActorOwner().IsMsgOnStack(MSG_REQUEST_ITEM)) return 1;
        carry = ActorCarryable::Create(0x100003c, 0x20000b2)->GetRefID();
        sm.GetActorOwner().SetCarry(carry, 0);
        sm.m_Unk9c = carry;
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// Graffiti tagger: turns to the nearest graffiti item to its POI, sprays it (the tag fades in
// while the anim plays, after a 70 frame wind-up) with a spray can in hand, calls in a crime
// report, then flags the POI (com data 10) and goes back to hub life.
signed char STATE_FS_Graffiti(StateMachine &sm)
{
    int i = 0;
    EXVector &pos = sm.GetVectorRef(i);
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    XItem *&pGraffiti = (XItem *&)sm.GetIntRef(i);
    float &alpha = sm.GetFloatRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(6);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        if (!sm.m_Msg.m_Data[0]) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_EXIT:
        POI_VendorUnbind(poi, sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_ENTER:
        if (!(sm.GetActorOwner().GetAttrib() & 0x16)) {
            stage = 0;
            poi = sm.m_Msg.m_Data[0];
            pos = POI_Get(poi)->m_Pos;
            pGraffiti = Graffiti_GetNearest(pos);
            if (pGraffiti) return 1;
        }
        sm.Return(0, 0, 0, 0);
        return 1;
    case MSG_EXT:
        sm.GetActorOwner().SetStatus(7);
        switch (stage) {
        case 0:
            if (sm.GetActorOwner().IsFacingYRot(pGraffiti->Position(), 5.0f)) {
                stage = 1;
                timer = 70.0f;
                sm.GetActorOwner().SetAnimType(0x9000101, 0, 1);
                sm.GetActorOwner().SetCarry(ActorCarryable::Create(0x100003c, 0x20000c6)->GetRefID(), 0);
                EVD::Instance(1)->Dial999(sm.GetActorOwner().GetFeetPos(), poi, sm.GetActorOwner().GetRefID(), CrimeType::VANDALISM);
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(pGraffiti->Position(), 5.0f);
            break;
        case 1:
            timer -= XApp::GameFrameRateMul();
            if (!(timer <= 0.0f)) break;
            stage = 2;
            return 1;
        case 2:
            alpha += XApp::GameFrameRateMul() * 0.005f;
            if (alpha >= 1.0f) alpha = 1.0f;
            pGraffiti->RenderInfo().m_rgbaMult.Set(1.0f, 1.0f, 1.0f, alpha);
            if (sm.GetActorOwner().HasAnimEnded()) {
                pGraffiti->RenderInfo().m_rgbaMult.Set(1.0f, 1.0f, 1.0f, 1.0f);
                POI_SetComData(poi, 10, 0, 0, 0);
                if (sm.GetActorOwner().GetCarryPtr(0)) sm.GetActorOwner().GetCarryPtr(0)->Delete();
                sm.GetActorOwner().SetCarry(0, 0);
                POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
                POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
                return 1;
            }
            break;
        }
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// Factory worker at a machine: works the machine (the ATM use anim) until it ends, then flags
// the POI and goes back to hub life. Interrupted, it walks back to where it stood (giving up
// the walk once within 2.5 units for 5 seconds), turns back to its old facing and resumes.
signed char STATE_FS_FactoryWorker(StateMachine &sm)
{
    int i = 0;
    EXVector &pos = sm.GetVectorRef(i);
    int &poly = sm.GetIntRef(i);
    float &rotY = sm.GetFloatRef(i);
    float &timer = sm.GetFloatRef(i);
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    sm.GetActorOwner().SetHeadTrackMode(1);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN: {
        EXVector d = sm.GetActorOwner().GetFeetPos() - pos;
        if (d.y * d.y + d.z * d.z + d.x * d.x > 1.0f) {
            stage = 1;
            timer = EXBaseApp::NominalGameFrameRate() * 5.0f;
            return 1;
        }
        stage = 2;
        return 1;
    }
    case MSG_ENTER:
        stage = 0;
        pos = sm.GetActorOwner().GetFeetPos();
        poly = sm.GetActorOwner().GetNavPolyHandle();
        rotY = sm.GetActorOwner().GetRotation().y;
        poi = sm.m_Msg.m_Data[0];
        sm.GetActorOwner().SetAnimType(0x9000022, 0, 1);
        return 1;
    case MSG_EXT:
        switch (stage) {
        case 0:
            sm.GetActorOwner().InvalidateGotoPos();
            sm.GetActorOwner().SetAnimType(0x9000022, 0, 1);
            if (sm.GetActorOwner().HasAnimEnded()) {
                POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
                POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
            }
            break;
        case 1:
            sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
            sm.GetActorOwner().GotoPosPoly(pos, poly);
            if (sm.GetActorOwner().SqDistToTarget() < 6.25f) {
                timer -= XApp::GameFrameRateMul();
                if (timer <= 0.0f || sm.GetActorOwner().SqDistToTarget() < 0.01f) {
                    stage = 2;
                    return 1;
                }
                break;
            }
            timer = EXBaseApp::NominalGameFrameRate() * 5.0f;
            return 1;
        case 2:
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(rotY, 5.0f)) {
                stage = 0;
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(rotY, 5.0f);
            return 1;
        }
        return 1;
    case MSG_EXIT:
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// Dice player at a gambling POI: waits for the vendor (the dice host), kneels facing its stand
// slot, throws (the host plays its idle), then wins or loses at random (the host gets the
// other outcome) and goes back to hub life.
signed char STATE_FS_Gamble(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT: {
        Actor *pHost = Actor::GetPtrFromID(POI_VendorGetID(poi, 0));
        switch (stage) {
        case 0:
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            if (POI_VendorIsThere(poi)) {
                stage = 1;
                return 1;
            }
            break;
        case 1:
            if (sm.GetActorOwner().IsFacingYRot(POI_CustomerStandPos(poi, POI_GetComData(poi, 0, 0, 0) - 1), 5.0f)) {
                stage = 2;
                POI_SetComData(poi, POI_CustomerStandIndex(poi, sm.GetActorOwner().GetRefID()), 0, 0, 0);
                sm.GetActorOwner().SetAnimType(0x9000027, 0, 1);
                if (pHost) pHost->SendMsg(MsgItem(MSG_PLAY_ANIM, 0x900004f, sm.GetActorOwner().GetRefID()), 0.0f);
            } else {
                sm.GetActorOwner().SetTargetYRot(POI_CustomerStandPos(poi, POI_GetComData(poi, 0, 0, 0) - 1), 5.0f);
            }
            break;
        case 2:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 3;
                int r = EXRand32() & 1;
                int anims[4];
                anims[0] = 0x9000028;
                anims[1] = 0x9000029;
                anims[2] = 0x900004d;
                anims[3] = 0x900004e;
                sm.GetActorOwner().SetAnimType(anims[r], 0, 1);
                if (pHost) pHost->SendMsg(MsgItem(MSG_PLAY_ANIM, anims[!r + 2], sm.GetActorOwner().GetRefID()), 0.0f);
            }
            break;
        case 3:
            if (sm.GetActorOwner().HasAnimEnded()) {
                POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
                POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
                POI_SetComData(poi, 0, 0, 0, 0);
                stage = 1;
            }
            break;
        }
        return 1;
    }
    case MSG_ENTER:
        if (sm.GetActorOwner().GetAttrib() & 6) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        sm.GetActorOwner().InvalidateGotoPos();
        return 1;
    case MSG_EXIT:
        POI_CustomerUnbind(poi, sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_RETURN:
        if (!sm.m_Msg.m_Data[0]) {
            POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
            return 1;
        }
        stage = 0;
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// Store robbery lookout: from the crime POI's nearest flag point it binds to the scene of crime
// as a customer and runs there (or just stands at the flag point if it cannot), then threatens
// facing the flag point until the robber signals (com data 5000) or 1800 frames pass, and
// leaves with the robber's destination to be deleted.
signed char STATE_FS_StoreRobberyAccomplice(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    int &store = sm.GetIntRef(i);
    int &flag = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(6);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        if (!sm.m_Msg.m_Data[0]) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_EXIT:
        POI_CustomerUnbind(poi, sm.GetActorOwner().GetRefID());
        POI_CustomerUnbind(store, sm.GetActorOwner().GetRefID());
        sm.GetActorOwner().Delete();
        return 1;
    case MSG_ENTER: {
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        Actor &actor = sm.GetActorOwner();
        flag = POI_GetNearestFlag(POI_Get(poi)->m_Pos, 0x20, 0, actor.GetNavZone(), actor.CanJump(), 0, -1.0f);
        if (!flag) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        Actor &a = sm.GetActorOwner();
        store = POI_GetNearestType(POI_Get(flag)->m_Pos, 0x42000033, 0, a.GetNavZone(), a.CanJump(), 0);
        if (!store) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        SM_GOSUB(STATE_CrimePrepareAccomplice, poi, 0, 0, 0);
        return 1;
    }
    case MSG_EXT:
        sm.GetActorOwner().SetStatus(7);
        switch (stage) {
        case 0:
            timer = 1800.0f;
            if (POI_CustomerBind(store, sm.GetActorOwner().GetRefID())) {
                stage = 1;
                sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
                sm.GetActorOwner().GotoCustomerPOI(store, -1);
                return 1;
            }
            if (sm.GetActorOwner().IsFacingYRot(POI_Get(flag)->m_Pos, 5.0f)) {
                stage = 2;
                sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(POI_Get(flag)->m_Pos, 5.0f);
            return 1;
        case 1:
            if (sm.GetActorOwner().SqDistToTarget() < 0.01f) {
                sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
                if (sm.GetActorOwner().IsFacingYRot(POI_Get(flag)->m_Pos, 5.0f)) {
                    stage = 2;
                    sm.GetActorOwner().SetAnimType(0x900012b, 0, 1);
                    return 1;
                }
                sm.GetActorOwner().SetTargetYRot(POI_Get(flag)->m_Pos, 5.0f);
                return 1;
            }
            break;
        case 2: {
            int target = 0;
            if (POI_GetComData(poi, &target, 0, 0) != 5000) {
                timer -= XApp::GameFrameRateMul();
                if (!(timer <= 0.0f)) break;
            }
            stage = 3;
            SM_GOSUB(STATE_GotoLocation, target, 0x2310, 0, 0);
            return 1;
        }
        case 3:
            sm.GetActorOwner().Delete();
            return 1;
        }
        return 1;
    AU_DEFAULT_MSGS
    case (MsgType)0x15:  // TODO name
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x15)) return 1;
        POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        return 1;
    }
    return 0;
}

// Vandal's accomplice: once the vandal signals (com data 3000) it faces the POI and smashes
// the truck with a baseball bat (three random swings) until the signal stops, then flags the
// POI and walks to the nearest home before going back to hub life.
signed char STATE_FS_VandalismAccomplice(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    int &bDone = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(6);
    int anims[3];
    anims[0] = 0x900014b;
    anims[1] = 0x900014c;
    anims[2] = 0x900014d;
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        if (!sm.m_Msg.m_Data[0]) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_EXIT:
        if (sm.GetActorOwner().GetCarryPtr(0)) sm.GetActorOwner().GetCarryPtr(0)->Delete();
        sm.GetActorOwner().SetCarry(0, 0);
        POI_CustomerUnbind(poi, sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_ENTER:
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        bDone = 0;
        SM_GOSUB(STATE_CrimePrepareAccomplice, poi, 0, 0, 0);
        return 1;
    case MSG_EXT:
        sm.GetActorOwner().SetStatus(7);
        switch (stage) {
        case 0:
            if (POI_GetComData(poi, 0, 0, 0) == 3000) {
                stage = 1;
                EXVector dz = VSub(POI_Get(poi)->m_Pos, sm.GetActorOwner().GetHeadPos());
                EXVector dx = VSub(POI_Get(poi)->m_Pos, sm.GetActorOwner().GetHeadPos());
                sm.GetActorOwner().SetRotY((float)atan2(dx.x, dz.z));
                sm.GetActorOwner().SetCarry(ActorCarryable::Create(0x100003c, 0x20000b8)->GetRefID(), 0);
                sm.GetActorOwner().SetAnimType(anims[EXRand32() % 3], 0, 1);
            }
            break;
        case 1:
            if (POI_GetComData(poi, 0, 0, 0) != 3000) bDone = 1;
            if (sm.GetActorOwner().HasAnimEnded()) {
                if (bDone) {
                    stage = 2;
                    Actor *pActor = &sm.GetActorOwner();
                    int home = POI_GetNearestType(pActor->GetFeetPos(), 0x42000004, 0, pActor->GetNavZone(), pActor->CanJump(), 0);
                    POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
                    SM_GOSUB(STATE_GotoLocation, home, 0x110, 0, 0);
                    return 1;
                }
                sm.GetActorOwner().SetAnimType(anims[EXRand32() % 3], 0, 1);
            }
            break;
        case 2:
            POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
            break;
        }
        return 1;
    AU_DEFAULT_MSGS
    case (MsgType)0x15:  // TODO name
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x15)) return 1;
        POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        return 1;
    }
    return 0;
}

// Burglar: prepares at the crime POI, walks to its nearest entry flag point and hides inside
// for 600 frames (a crime report goes out), then comes out with a swag bag, flags the POI with
// its getaway home (com data 5000) and runs there to be deleted. If killed on the job its
// accomplice is told (message 0x15).
signed char STATE_FS_BreakAndEnter(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &entry = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(6);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        if (!sm.m_Msg.m_Data[0]) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_EXIT:
        if (sm.GetActorOwner().GetCarryPtr(1)) sm.GetActorOwner().GetCarryPtr(1)->Delete();
        sm.GetActorOwner().SetCarry(0, 1);
        if (sm.GetActorOwner().GetCarryPtr(0)) sm.GetActorOwner().GetCarryPtr(0)->Delete();
        sm.GetActorOwner().SetCarry(0, 0);
        sm.GetActorOwner().SetHide(0);
        if (POI_VendorUnbind(poi, sm.GetActorOwner().GetRefID())) {
            POI_SetComData(poi, 0, 0, 0, 0);
            POI_SetComData(entry, 0, 0, 0, 0);
            if (sm.GetActorOwner().IsDead()) POI_SendMsg(poi, sm.GetActorOwner().GetRefID(), MsgItem((MsgType)0x15));
        }
        return 1;
    case MSG_ENTER: {
        if (sm.GetActorOwner().GetAttrib() & 0x16) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        Actor &actor = sm.GetActorOwner();
        entry = POI_GetNearestFlag(POI_Get(poi)->m_Pos, 0x100, 0, actor.GetNavZone(), actor.CanJump(), 0, -1.0f);
        if (!entry) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        SM_GOSUB(STATE_CrimePrepare, poi, 0, 0, 0);
        return 1;
    }
    case MSG_EXT:
        sm.GetActorOwner().SetStatus(7);
        switch (stage) {
        case 0:
            stage = 1;
            SM_GOSUB(STATE_GotoLocation, entry, 0x110, 0, 0);
            break;
        case 1:
            stage = 2;
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            sm.GetActorOwner().SetHide(1);
            timer = 600.0f;
            EVD::Instance(1)->Dial999(sm.GetActorOwner().GetFeetPos(), poi, sm.GetActorOwner().GetRefID(), CrimeType::NONE);
            break;
        case 2: {
            timer -= XApp::GameFrameRateMul();
            if (!(timer <= 0.0f)) break;
            stage = 3;
            sm.GetActorOwner().SetCarry(ActorCarryable::Create(0x100003c, 0x20000c7)->GetRefID(), 1);
            sm.GetActorOwner().SetHide(0);
            Actor &actor = sm.GetActorOwner();
            int home = POI_GetNearestType(actor.GetFeetPos(), 0x42000004, entry, actor.GetNavZone(), actor.CanJump(), 0);
            POI_SetComData(poi, 5000, home, 0, 0);
            POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
            SM_GOSUB(STATE_GotoLocation, home, 0x2310, 0, 0);
            break;
        }
        case 3:
            sm.GetActorOwner().Delete();
            break;
        }
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// Vandal: prepares at the crime POI, faces it and smashes the truck with a baseball bat (three
// random swings, com data 3000 calls the accomplice in, a crime report goes out) for 600
// frames, then flags the POI (com data 5000) and goes back to hub life. If killed on the job
// its accomplice is told (message 0x15).
signed char STATE_FS_Vandalism(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(6);
    int anims[3];
    anims[0] = 0x900014b;
    anims[1] = 0x900014c;
    anims[2] = 0x900014d;
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        if (!sm.m_Msg.m_Data[0]) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_EXIT:
        if (sm.GetActorOwner().GetCarryPtr(0)) sm.GetActorOwner().GetCarryPtr(0)->Delete();
        sm.GetActorOwner().SetCarry(0, 0);
        if (POI_VendorUnbind(poi, sm.GetActorOwner().GetRefID())) {
            POI_SetComData(poi, 0, 0, 0, 0);
            if (sm.GetActorOwner().IsDead()) POI_SendMsg(poi, sm.GetActorOwner().GetRefID(), MsgItem((MsgType)0x15));
        }
        return 1;
    case MSG_ENTER:
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        SM_GOSUB(STATE_CrimePrepare, poi, 0, 0, 0);
        return 1;
    case MSG_EXT:
        sm.GetActorOwner().SetStatus(7);
        switch (stage) {
        case 0: {
            stage = 1;
            timer = 600.0f;
            EXVector dz = VSub(POI_Get(poi)->m_Pos, sm.GetActorOwner().GetHeadPos());
            EXVector dx = VSub(POI_Get(poi)->m_Pos, sm.GetActorOwner().GetHeadPos());
            sm.GetActorOwner().SetRotY((float)atan2(dx.x, dz.z));
            sm.GetActorOwner().SetAnimType(anims[EXRand32() % 3], 0, 1);
            POI_SetComData(poi, 3000, 0, 0, 0);
            sm.GetActorOwner().SetCarry(ActorCarryable::Create(0x100003c, 0x20000b8)->GetRefID(), 0);
            EVD::Instance(1)->Dial999(sm.GetActorOwner().GetFeetPos(), poi, sm.GetActorOwner().GetRefID(), (CrimeType::Flags)2);
            break;
        }
        case 1:
            timer -= XApp::GameFrameRateMul();
            if (sm.GetActorOwner().HasAnimEnded()) {
                if (0.0f >= timer) {
                    POI_SetComData(poi, 5000, 0, 0, 0);
                    POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
                    POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
                } else {
                    sm.GetActorOwner().SetAnimType(anims[EXRand32() % 3], 0, 1);
                }
            }
            break;
        }
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// Drug buyer at a dealer's POI: waits for the dealer, chats, hands over money and takes the
// drugs (com data 1..4 drives the dealer's matching anims), then walks to the nearest loiter
// point, uses them and is left high (attribute 4).
signed char STATE_FS_BuyDrugs(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    int &loiter = sm.GetIntRef(i);
    int &carry = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        switch (stage) {
        case 0:
            if (!POI_VendorIsThere(poi)) {
                sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                return 1;
            }
            stage = 1;
            return 1;
        case 1:
            sm.GetActorOwner().Speak(0x1af0042f, 0);
            sm.GetActorOwner().SetAnimType(0x90000e3, 0, 1);
            stage = 2;
            return 1;
        case 2:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().SetAnimType(0x90000e4, 0, 1);
                stage = 3;
            }
            POI_SetComData(poi, 1, 0, 0, 0);
            return 1;
        case 3:
            if (!POI_VendorIsThere(poi)) {
                POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
                return 1;
            }
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().Speak(0x1af00432, 0);
                sm.GetActorOwner().SetAnimType(0x90000a2, 0, 1);
                stage = 4;
            }
            POI_SetComData(poi, 2, 0, 0, 0);
            return 1;
        case 4:
            if (!POI_VendorIsThere(poi)) break;
            if (sm.GetActorOwner().HasAnimEnded()) {
                carry = Actor::SendMsg(POI_VendorGetID(poi, 0), MsgItem(MSG_REQUEST_ITEM), 0.0f);
                sm.GetActorOwner().SetAnimType(0x90000a4, 0, 1);
                stage = 5;
            }
            POI_SetComData(poi, 3, 0, 0, 0);
            return 1;
        case 5:
            if (!POI_VendorIsThere(poi)) break;
            if (sm.GetActorOwner().HasAnimEnded()) {
                Actor &actor = sm.GetActorOwner();
                loiter = POI_GetNearestType(actor.GetFeetPos(), 0x42000005, 0, actor.GetNavZone(), actor.CanJump(), 0);
                POI_SetComData(poi, 0, 0, 0, 0);
                POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
                stage = 6;
                SM_GOSUB(STATE_GotoLocation, loiter, 0x101, 0, 0);
                return 1;
            }
            POI_SetComData(poi, 4, 0, 0, 0);
            return 1;
        case 6:
            sm.GetActorOwner().SetAnimType(0x9000104, 0, 1);
            stage = 7;
            return 1;
        case 7:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().SetAttrib(4, 1);
                POI_CustomerUnbind(loiter, sm.GetActorOwner().GetRefID());
                POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
            }
            return 1;
        default:
            return 1;
        }
        POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_ENTER:
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        if (sm.GetActorOwner().GetCarryPtr(1)) sm.GetActorOwner().GetCarryPtr(1)->Delete();
        sm.GetActorOwner().SetCarry(0, 1);
        if (sm.GetActorOwner().GetCarryPtr(0)) sm.GetActorOwner().GetCarryPtr(0)->Delete();
        sm.GetActorOwner().SetCarry(0, 0);
        sm.GetActorOwner().InvalidateGotoPos();
        return 1;
    case MSG_EXIT:
        POI_SetComData(poi, 0, 0, 0, 0);
        POI_CustomerUnbind(poi, sm.GetActorOwner().GetRefID());
        POI_CustomerUnbind(loiter, sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_RETURN:
        if (!sm.m_Msg.m_Data[0]) {
            POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
            return 1;
        }
        stage = 0;
        return 1;
    case MSG_TAKE_ITEM:
        if (sm.GetActorOwner().IsMsgOnStack(MSG_TAKE_ITEM)) return 1;
        sm.GetActorOwner().SetCarry(carry, 0);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// Shop vendor at its stand: keeps its spot until a customer asks for an item, which it makes
// from the stand type (burger, ice cream, a paper or comic, anything, a shop bag, else a can),
// turns to the customer slot, takes it out, hands it over and flags the POI. Robbery and
// extortion hand the vendor over to the victim states.
signed char STATE_FS_Vendor(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    int &carry = sm.GetIntRef(i);
    EXVector &pos = sm.GetVectorRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        if (!sm.m_Msg.m_Data[0]) {
            POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
            return 1;
        }
        stage = 0;
        return 1;
    case MSG_ENTER:
        if (sm.GetActorOwner().GetAttrib() & 0x16) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        pos = sm.GetActorOwner().GetFeetPos();
        return 1;
    case MSG_EXT:
        switch (stage) {
        case 0:
            AU_EnsureAtTarget(sm, pos, 0.25f);
            return 1;
        case 1:
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(POI_Get(poi)->m_Binds[4].m_Pos, 5.0f)) {
                stage = 2;
                sm.GetActorOwner().SetAnimType(0x90000a4, 0, 1);
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(POI_Get(poi)->m_Binds[4].m_Pos, 5.0f);
            return 1;
        case 2:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 3;
                if (ActorCarryable::GetPtrFromID(carry)) ActorCarryable::GetPtrFromID(carry)->SetHide(0);
                sm.GetActorOwner().SetCarry(carry, 0);
                sm.GetActorOwner().SetAnimType(0x90000a2, 0, 1);
            }
            return 1;
        case 3:
            if (sm.GetActorOwner().HasAnimEnded()) stage = 4;
            return 1;
        case 4:
            stage = 0;
            POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
            POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
            return 1;
        }
        break;
    case MSG_EXIT:
        POI_VendorUnbind(poi, sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_REQUEST_ITEM: {
        if (sm.GetActorOwner().IsMsgOnStack(MSG_REQUEST_ITEM)) return 1;
        stage = 1;
        int items[5];
        items[0] = 0x20000b6;
        items[1] = 0x20000b9;
        items[2] = 0x20000c0;
        items[3] = 0x20000c1;
        items[4] = 0x20000c4;
        int entity;
        switch (POI_Get(poi)->m_Type) {
        case 0x42000015: // HT_LocationType_Vendor_Food1_Burgers
            entity = 0x20000b5;
            break;
        case 0x42000016: // HT_LocationType_Vendor_Food2_IceCreams
            entity = 0x20000bc;
            break;
        case 0x42000017: // HT_LocationType_Vendor_News1_Papers
            entity = items[EXRand32() % 3 + 1];
            break;
        case 0x42000018: // HT_LocationType_Vendor_News2_Everything
            // Retail bug: one past the end of the five items.
            entity = items[EXRand32() % 6];
            break;
        case 0x42000019: // HT_LocationType_Vendor_Misc1_BrownBags
            entity = 0x20000c4;
            break;
        default:
            entity = 0x20000b6;
            break;
        }
        carry = ActorCarryable::Create(0x100003c, entity)->GetRefID();
        if (ActorCarryable::GetPtrFromID(carry)) ActorCarryable::GetPtrFromID(carry)->SetHide(1);
        sm.m_Unk9c = carry;
        return 1;
    }
    AU_DEFAULT_MSGS
    case MSG_ROBBERY:
        if (sm.GetActorOwner().IsMsgOnStack(MSG_ROBBERY)) return 1;
        stage = 0;
        POI_SendMsg(poi, sm.GetActorOwner().GetRefID(), MsgItem(MSG_SOMEONE_UNDER_ATTACK));
        SM_GOSUB(STATE_RobberyVictim, poi, 0, 0, 0);
        return 1;
    case MSG_EXTORTION:
        if (sm.GetActorOwner().IsMsgOnStack(MSG_EXTORTION)) return 1;
        stage = 0;
        POI_SendMsg(poi, sm.GetActorOwner().GetRefID(), MsgItem(MSG_SOMEONE_UNDER_ATTACK));
        SM_GOSUB(STATE_ExtortionVictim, poi, 0, 0, 0);
        return 1;
    }
    return 0;
}

// Fight club brawler: once at least two fighters are at the POI (always on the zombie base) it
// throws random punches, striking everyone within 2 units, and flinches when struck (at most
// every 120 frames). Knocked more than 0.5 off its spot it walks back and turns to its old
// facing. Leaves after 1800 frames. On the labs map the first fighter shouts to start it.
signed char STATE_FS_FightClub(StateMachine &sm)
{
    int i = 0;
    EXVector &pos = sm.GetVectorRef(i);
    int &poly = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    int &bFighting = sm.GetIntRef(i);
    float &flinchTimer = sm.GetFloatRef(i);
    float &rotY = sm.GetFloatRef(i);
    float &walkTimer = sm.GetFloatRef(i);
    int &stage = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    if (sm.m_Msg.m_Type == MSG_ATTRACT) return 1;
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        timer -= XApp::GameFrameRateMul();
        if (timer <= 0.0f) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        flinchTimer -= XApp::GameFrameRateMul();
        switch (stage) {
        case 0: {
            if (gCurrentMap_GeoHash != 0x1000082 && POI_GetActorCount(poi, 0xd) < 2) {
                sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                return 1;
            }
            if (!sm.GetActorOwner().HasAnimEnded() && bFighting) return 1;
            if (!bFighting) {
                POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
                bFighting = 1;
                if (!POI_GetComData(poi, 0, 0, 0) && gCurrentMap_GeoHash == 0x10000b4) {
                    POI_SetComData(poi, 1, 0, 0, 0);
                    sm.GetActorOwner().Speak(0x1af00645, 0);
                }
            }
            {
                int anims[8];
                anims[0] = 0x900012d;
                anims[1] = 0x900012e;
                anims[2] = 0x900012f;
                anims[3] = 0x9000130;
                anims[4] = 0x9000131;
                anims[5] = 0x9000132;
                anims[6] = 0x9000133;
                anims[7] = 0x9000134;
                sm.GetActorOwner().SetAnimType(anims[EXRand32() & 7], 0, 1);
            }
            EXVector d = sm.GetActorOwner().GetFeetPos() - pos;
            if (d.y * d.y + d.z * d.z + d.x * d.x > 0.25f) {
                stage = 1;
                return 1;
            }
            for (Actor *p = Actor::GetFirst(sm.GetActorOwner().GetFeetPos(), 2.0f, 0); p;
                 p = p->GetNext(sm.GetActorOwner().GetFeetPos(), 2.0f, 0))
                p->SendMsg(MsgItem(MSG_STRIKE), 0.0f);
            return 1;
        }
        case 1:
            sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
            sm.GetActorOwner().GotoPosPoly(pos, poly);
            if (sm.GetActorOwner().SqDistToTarget() <= 0.0025f) {
                walkTimer -= XApp::GameFrameRateMul();
                if (walkTimer <= 0.0f || sm.GetActorOwner().SqDistToTarget() < 0.01f) {
                    stage = 2;
                    return 1;
                }
                return 1;
            }
            walkTimer = EXBaseApp::NominalGameFrameRate() * 5.0f;
            return 1;
        case 2:
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(rotY, 45.0f)) {
                stage = 0;
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(rotY, 45.0f);
            return 1;
        }
        return 1;
    case MSG_EXIT:
        POI_SetComData(poi, 0, 0, 0, 0);
        return 1;
    case MSG_ENTER:
        poi = sm.m_Msg.m_Data[0];
        timer = 1800.0f;
        pos = sm.GetActorOwner().GetFeetPos();
        poly = sm.GetActorOwner().GetNavPolyHandle();
        rotY = sm.GetActorOwner().GetRotation().y;
        stage = 0;
        sm.GetActorOwner().SetCarry(0, 1);
        sm.GetActorOwner().SetCarry(0, 0);
        return 1;
    AU_DEFAULT_MSGS
    case MSG_STRIKE:
        if (sm.GetActorOwner().IsMsgOnStack(MSG_STRIKE)) return 1;
        if (flinchTimer <= 0.0f) sm.GetActorOwner().SetAnimType(0x900012c, 0, 1);
        flinchTimer = 120.0f;
        return 1;
    }
    return 0;
}

// Mimicry show-off: an armed actor faces away from its POI and fires into the air when it hears
// the predator's vocal mimicry (an attract noise sets it to 1 health). It turns to the POI,
// picks a side 90 degrees either way and sprays bullets 10 units ahead, sweeping between the
// two sides, then panics. Without a gun it goes back to hub life.
signed char STATE_FS_MimicryMe(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    float &rot = sm.GetFloatRef(i);
    float &rotA = sm.GetFloatRef(i);
    float &rotB = sm.GetFloatRef(i);
    EXVector &pos = sm.GetVectorRef(i);
    sm.GetActorOwner().SetStatus(3);
    sm.GetActorOwner().m_WatchFlags = 0;
    ActorCarryableGun *pGun = sm.GetActorOwner().GetGunPtr(0);
    if (!pGun) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER: {
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        EXVector d = sm.GetActorOwner().GetFeetPos() - POI_Get(poi)->m_Pos;
        rot = (float)atan2(d.x, d.z);
        pos = sm.GetActorOwner().GetFeetPos();
        pGun->SetWait(0.0f);
        pGun->RefillClipInstantly();
        return 1;
    }
    case MSG_EXT: {
        GunCombatAnims anims = Actor_GetAimAnimsSet(sm.GetActorOwner(), sm.GetActorOwner().GetWeaponProfile(0));
        switch (stage) {
        case 0:
            if (sm.GetActorOwner().IsFacingYRot(rot, 5.0f)) {
                stage = 1;
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(rot, 5.0f);
            return 1;
        case 1:
            sm.GetActorOwner().SetAnimType(sm.GetActorOwner().GetSubGroupAnims().m_Anims[5], 0, 1);
            return 1;
        case 2:
            if (sm.GetActorOwner().IsFacingYRot(POI_Get(poi)->m_Pos, 5.0f)) {
                stage = 3;
                rotA = sm.GetActorOwner().GetRotation().y - 1.5707964f;
                rotB = sm.GetActorOwner().GetRotation().y + 1.5707964f;
                if (EXRand32() & 1) rot = rotB;
                else rot = rotA;
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(POI_Get(poi)->m_Pos, 5.0f);
            return 1;
        case 3:
            if (pGun->CanFire() && pGun->StartFiring()) {
                sm.GetActorOwner().Speak(sm.GetActorOwner().GetWarcrySFX(), 0);
                stage = 4;
            }
            sm.GetActorOwner().SetAnimType(anims.m_Anims[1], 1, 1);
            return 1;
        case 4: {
            if (sm.GetActorOwner().IsFacingYRot(rot, 5.0f)) {
                if (fabs(rot - rotA) < 0.3926991f) rot = rotB;
                else rot = rotA;
            } else {
                sm.GetActorOwner().SpinToYRot(rot);
            }
            float r = sm.GetActorOwner().GetRotation().y;
            EXVector dir((float)sin(r) * 10.0f, 0.0f, (float)cos(r) * 10.0f, 0.0f);
            EXVector target = sm.GetActorOwner().GetFeetPos() + dir;
            target.y += 1.0f;
            if (!pGun->UpdateFiring(sm.GetActorOwner(), target, anims.m_Anims[10], 0)) stage = 5;
            return 1;
        }
        case 5:
            stage = 7;
            POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
            SM_GOTO(STATE_PanicExtreme, 0, 0, 0, 0);
            return 1;
        case 6:
            stage = 7;
            POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
            return 1;
        case 7:
            POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
            return 1;
        }
        return 1;
    }
    case MSG_EXIT:
    case MSG_SUSPEND:
        pGun->StopFiring();
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR)) return 1;
        AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTRACT: {
        if (sm.GetActorOwner().IsMsgOnStack(MSG_ATTRACT)) return 1;
        sm.GetActorOwner().SetHealth(1.0f, 2);
        stage = 2;
        EXVector d = XGetPlayer()->GetItem()->m_position - POI_Get(poi)->m_Pos;
        if (d.x * d.x + d.z * d.z < 4.0f) {
            stage = 2;
            return 1;
        }
        AU_HandleAttractNoise(sm);
        return 1;
    }
    case MSG_SOMEONE_SEEN_PREDATOR:
        if (sm.GetActorOwner().IsMsgOnStack(MSG_SOMEONE_SEEN_PREDATOR)) return 1;
        AU_HandleSomeoneSeenPredator(sm);
        return 1;
    case MSG_SOMEONE_UNDER_ATTACK:
        if (sm.GetActorOwner().IsMsgOnStack(MSG_SOMEONE_UNDER_ATTACK)) return 1;
        AU_HandleSomeoneUnderAttack(sm);
        return 1;
    }
    return 0;
}

// Store robber: prepares, binds to the scene of crime by the crime POI's flag point as its
// "vendor" and runs there with a revolver, turns to the shop's vendor slot and holds it up
// (the shopkeeper gets MSG_ROBBERY, a crime report goes out), takes the cash, then flags the
// POI with a getaway home (com data 5000) and runs there to be deleted. If killed on the job
// its accomplice is told (message 0x15).
signed char STATE_FS_StoreRobbery(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &store = sm.GetIntRef(i);
    int &shop = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    int &carry = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(6);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        if (!sm.m_Msg.m_Data[0]) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_EXIT:
        if (sm.GetActorOwner().GetCarryPtr(1)) sm.GetActorOwner().GetCarryPtr(1)->Delete();
        sm.GetActorOwner().SetCarry(0, 1);
        if (sm.GetActorOwner().GetCarryPtr(0)) sm.GetActorOwner().GetCarryPtr(0)->Delete();
        sm.GetActorOwner().SetCarry(0, 0);
        if (POI_VendorUnbind(poi, sm.GetActorOwner().GetRefID())) {
            POI_SetComData(poi, 0, 0, 0, 0);
            POI_SetComData(shop, 0, 0, 0, 0);
            if (sm.GetActorOwner().IsDead()) POI_SendMsg(poi, sm.GetActorOwner().GetRefID(), MsgItem((MsgType)0x15));
        }
        POI_VendorUnbind(store, sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_ENTER: {
        stage = 2;
        poi = sm.m_Msg.m_Data[0];
        Actor &actor = sm.GetActorOwner();
        shop = POI_GetNearestFlag(POI_Get(poi)->m_Pos, 0x20, 0, actor.GetNavZone(), actor.CanJump(), 0, -1.0f);
        if (!shop) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        Actor &a = sm.GetActorOwner();
        store = POI_GetNearestType(POI_Get(shop)->m_Pos, 0x42000033, 0, a.GetNavZone(), a.CanJump(), 0);
        if (!store) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        SM_GOSUB(STATE_CrimePrepare, poi, 0, 0, 0);
        return 1;
    }
    case MSG_EXT:
        sm.GetActorOwner().SetStatus(7);
        switch (stage) {
        case 2:
            if (POI_VendorBind(store, sm.GetActorOwner().GetRefID())) {
                stage = 3;
                sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
                sm.GetActorOwner().SetCarry(ActorCarryable::Create(0x100003c, 0x20000e2)->GetRefID(), 0);
                sm.GetActorOwner().GotoVendorPOI(store, -1);
                return 1;
            }
            POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
            break;
        case 3:
            if (sm.GetActorOwner().SqDistToTarget() < 0.01f) {
                sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
                if (sm.GetActorOwner().IsFacingYRot(POI_VendorStandPos(shop, 0), 5.0f)) {
                    stage = 4;
                    Actor::SendMsg(POI_VendorGetID(shop, 0), MsgItem(MSG_ROBBERY), 0.0f);
                    sm.GetActorOwner().SetAnimType(0x90000e2, 0, 1);
                    EVD::Instance(1)->Dial999(sm.GetActorOwner().GetFeetPos(), poi, sm.GetActorOwner().GetRefID(), (CrimeType::Flags)5);
                    return 1;
                }
                sm.GetActorOwner().SetTargetYRot(POI_VendorStandPos(shop, 0), 5.0f);
                return 1;
            }
            break;
        case 4:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 5;
                carry = Actor::SendMsg(POI_VendorGetID(shop, 0), MsgItem(MSG_ROBBERY_TAKE, sm.GetActorOwner().GetRefID()), 0.0f);
                sm.GetActorOwner().SetAnimType(0x90000a4, 0, 1);
            }
            break;
        case 5:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 6;
                Actor &actor = sm.GetActorOwner();
                int home = POI_GetRandomType(0x42000004, 0, actor.GetNavZone(), actor.CanJump(), 0);
                POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
                POI_SetComData(poi, 5000, home, 0, 0);
                SM_GOSUB(STATE_GotoLocation, home, 0x2310, 0, 0);
            }
            break;
        case 6:
            sm.GetActorOwner().Delete();
            break;
        }
        return 1;
    case MSG_TAKE_ITEM:
        if (sm.GetActorOwner().IsMsgOnStack(MSG_TAKE_ITEM)) return 1;
        sm.GetActorOwner().SetCarry(carry, 1);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// Hooker at her corner: plays the deal the john drives through the POI com data (1-5: chat,
// chat with a line, take, flinch, hand over) and otherwise keeps her spot, calling out every
// 600 to 1200 frames. When the john settles (com data 6, with the meeting POI) she walks
// there with him; once both arrive she flags the POI and is deleted. If the john is lost or
// calls it off (message 0x15) she gives up (vanishing if already near the meeting point).
signed char STATE_FS_Hooker(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    int &john = sm.GetIntRef(i);
    int &meet = sm.GetIntRef(i);
    int &lastCom = sm.GetIntRef(i);
    float &meetTimer = sm.GetFloatRef(i);
    int &bOff = sm.GetIntRef(i);
    EXVector &pos = sm.GetVectorRef(i);
    int &bTimer = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        if (!sm.m_Msg.m_Data[0]) {
            POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
            return 1;
        }
        stage = 0;
        return 1;
    case MSG_EXIT:
        sm.GetActorOwner().m_MeetActorID = 0;
        sm.GetActorOwner().SetHide(0);
        POI_VendorUnbind(poi, sm.GetActorOwner().GetRefID());
        if (john) {
            Actor *pJohn = Actor::GetPtrFromID(john);
            if (pJohn) pJohn->SendMsg(MsgItem((MsgType)0x15), 0.0f);
            john = 0;
        }
        return 1;
    case MSG_ENTER:
        if (sm.GetActorOwner().GetAttrib() & 0x10) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        pos = sm.GetActorOwner().GetFeetPos();
        sm.GetActorOwner().InvalidateGotoPos();
        return 1;
    case MSG_EXT:
        switch (stage) {
        case 0: {
            int com = POI_GetComData(poi, 0, 0, 0);
            switch (com) {
            case 1:
                sm.GetActorOwner().SetAnimType(0x90000e4, 0, 1);
                break;
            case 2:
                if (lastCom != com) sm.GetActorOwner().Speak(0x1af004a0, 0);
                sm.GetActorOwner().SetAnimType(0x90000e3, 0, 1);
                break;
            case 3:
                sm.GetActorOwner().SetAnimType(0x90000a4, 0, 1);
                break;
            case 4:
                sm.GetActorOwner().SetAnimType(0x900012a, 0, 1);
                break;
            case 5:
                sm.GetActorOwner().SetAnimType(0x90000a2, 0, 1);
                break;
            case 6:
                stage = 1;
                POI_GetComData(poi, &meet, 0, 0);
                john = POI_CustomerGetID(poi, 0);
                break;
            default:
                AU_EnsureAtTarget(sm, pos, 0.25f);
                if (!bTimer) {
                    bTimer = 1;
                    timer = (EXRandf() + 1.0f) * 600.0f;
                }
                timer -= XApp::GameFrameRateMul();
                if (timer <= 0.0f) {
                    sm.GetActorOwner().Speak(0x1af0049e, 0);
                    bTimer = 0;
                }
                sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                break;
            }
            lastCom = com;
            POI_SetComData(poi, 0, 0, 0, 0);
            return 1;
        }
        case 1:
            meetTimer = 1800.0f;
            stage = 2;
            sm.GetActorOwner().m_MeetActorID = john;
            POI_VendorUnbind(poi, sm.GetActorOwner().GetRefID());
            sm.GetActorOwner().GotoPOI(meet);
            sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
            return 1;
        case 2: {
            Actor *pJohn = Actor::GetPtrFromID(john);
            EXVector d = VSub(POI_Get(meet)->m_Pos, sm.GetActorOwner().GetFeetPos());
            float distSq = d.x * d.x + d.y * d.y + d.z * d.z;
            if ((!pJohn || bOff) && distSq < 9.0f) {
                sm.GetActorOwner().Delete();
                sm.Return(0, 0, 0, 0);
                return 1;
            }
            if (!pJohn || bOff) {
                sm.Return(0, 0, 0, 0);
                return 1;
            }
            EXVector j = VSub(pJohn->GetFeetPos(), sm.GetActorOwner().GetFeetPos());
            if (j.y * j.y + j.z * j.z + j.x * j.x > 9.0f) sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            else sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
            if (sm.GetActorOwner().SqDistToTarget() < 0.01f) {
                POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
                sm.GetActorOwner().Delete();
            }
            return 1;
        }
        }
        return 1;
    AU_DEFAULT_MSGS
    case (MsgType)0x15:  // TODO name: the john called it off
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x15)) return 1;
        john = 0;
        bOff = 1;
        return 1;
    }
    return 0;
}

// Squared xyz distance, written out inline (retail inlines it here).
static __forceinline float City_DistSq(const EXVector &a, const EXVector &b)
{
    EXVector d = a - b;
    return d.x * d.x + d.y * d.y + d.z * d.z;
}

// Robbed shopkeeper (Gosub'd from STATE_FS_Vendor on MSG_ROBBERY): its stand stops taking
// customers, it faces the scene of crime with its hands up, hands the robber a swag bag, runs
// out to the nearest "outside crime" point (the scene of crime when that is over 10 units
// away), turns to the robber to shout at him and walks back to its spot. Message 0x15 (the
// robber gone) skips to the walk back.
signed char STATE_RobberyVictim(StateMachine &sm)
{
    int i = 0;
    EXVector &pos = sm.GetVectorRef(i);
    int &poly = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    int &carry = sm.GetIntRef(i);
    int &crime = sm.GetIntRef(i);
    int &robber = sm.GetIntRef(i);
    int &outside = sm.GetIntRef(i);
    int &shop = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(3);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        if (!sm.m_Msg.m_Data[0]) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_ENTER: {
        stage = 0;
        Actor &actor = sm.GetActorOwner();
        crime = POI_GetNearestType(actor.GetFeetPos(), 0x42000033, 0, actor.GetNavZone(), actor.CanJump(), 0);
        Actor &a = sm.GetActorOwner();
        outside = POI_GetNearestType(a.GetFeetPos(), 0x42000034, 0, a.GetNavZone(), a.CanJump(), a.IsBoss());
        pos = sm.GetActorOwner().GetFeetPos();
        poly = sm.GetActorOwner().GetNavPolyHandle();
        shop = sm.m_Msg.m_Data[0];
        if (!outside || City_DistSq(pos, POI_Get(outside)->m_Pos) > 100.0f) outside = crime;
        if (sm.GetActorOwner().GetCarryPtr(1)) sm.GetActorOwner().GetCarryPtr(1)->Delete();
        sm.GetActorOwner().SetCarry(0, 1);
        if (sm.GetActorOwner().GetCarryPtr(0)) sm.GetActorOwner().GetCarryPtr(0)->Delete();
        sm.GetActorOwner().SetCarry(0, 0);
        POI_EnableBinds(shop, 0);
        sm.GetActorOwner().InvalidateGotoPos();
        return 1;
    }
    case MSG_EXT:
        switch (stage) {
        case 0: {
            EXVector dz = VSub(POI_Get(crime)->m_Pos, sm.GetActorOwner().GetHeadPos());
            EXVector dx = VSub(POI_Get(crime)->m_Pos, sm.GetActorOwner().GetHeadPos());
            sm.GetActorOwner().SetRotY((float)atan2(dx.x, dz.z));
            sm.GetActorOwner().SetAnimType(0x9000109, 0, 1);
            return 1;
        }
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 2;
                if (outside) {
                    SM_GOSUB(STATE_GotoLocation, outside, 0x302, 0, 0);
                    return 1;
                }
            }
            return 1;
        case 2: {
            Actor *pRobber = Actor::GetPtrFromID(robber);
            if (!pRobber || sm.GetActorOwner().IsFacingYRot(pRobber->GetFeetPos(), 5.0f)) {
                stage = 3;
                sm.GetActorOwner().SetAnimType(0x9000100, 0, 1);
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(pRobber->GetFeetPos(), 5.0f);
            return 1;
        }
        case 3:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 4;
                return 1;
            }
            return 1;
        case 4:
            stage = 5;
            sm.GetActorOwner().GotoPosPoly(pos, poly);
            sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
            return 1;
        case 5:
            if (sm.GetActorOwner().SqDistToTarget() < 0.01f) sm.Return(1, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_EXIT:
        POI_VendorUnbind(outside, sm.GetActorOwner().GetRefID());
        POI_EnableBinds(shop, 1);
        return 1;
    AU_DEFAULT_MSGS
    case MSG_ROBBERY_TAKE:
        if (sm.GetActorOwner().IsMsgOnStack(MSG_ROBBERY_TAKE)) return 1;
        stage = 1;
        robber = sm.m_Msg.m_Data[0];
        carry = ActorCarryable::Create(0x100003c, 0x20000c7)->GetRefID();
        sm.GetActorOwner().SetCarry(carry, 0);
        sm.GetActorOwner().SetAnimType(0x90000a2, 0, 1);
        sm.m_Unk9c = carry;
        return 1;
    case (MsgType)0x15:  // TODO name: the robber is gone
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x15)) return 1;
        stage = 4;
        return 1;
    }
    return 0;
}

// Extorted shopkeeper (Gosub'd from STATE_FS_Vendor on MSG_EXTORTION): its stand stops taking
// customers, it faces the scene of crime, then walks to the extortion point and waits there,
// turning to talk (or argue, one time in four, when it refuses) once the extorter is ready
// (com data 4000). It pays with a swag bag, or refuses and gets punched; when the extorter
// leaves it walks back to its spot.
signed char STATE_ExtortionVictim(StateMachine &sm)
{
    int i = 0;
    EXVector &pos = sm.GetVectorRef(i);
    int &poly = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    int &carry = sm.GetIntRef(i);
    int &crime = sm.GetIntRef(i);
    int &spot = sm.GetIntRef(i);
    int &extorter = sm.GetIntRef(i);
    int &bRefuse = sm.GetIntRef(i);
    int &shop = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(3);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        if (!sm.m_Msg.m_Data[0]) sm.Return(0, 0, 0, 0);
        return 1;
    case MSG_ENTER: {
        stage = 0;
        Actor &actor = sm.GetActorOwner();
        crime = POI_GetNearestType(actor.GetFeetPos(), 0x42000033, 0, actor.GetNavZone(), actor.CanJump(), 0);
        Actor &a = sm.GetActorOwner();
        spot = POI_GetNearestType(a.GetFeetPos(), 0x4200002f, 0, a.GetNavZone(), a.CanJump(), 0);
        pos = sm.GetActorOwner().GetFeetPos();
        poly = sm.GetActorOwner().GetNavPolyHandle();
        shop = sm.m_Msg.m_Data[0];
        const int &r = EXRand32() & 3;  // retail spills it
        bRefuse = r == 0;
        if (sm.GetActorOwner().GetCarryPtr(1)) sm.GetActorOwner().GetCarryPtr(1)->Delete();
        sm.GetActorOwner().SetCarry(0, 1);
        if (sm.GetActorOwner().GetCarryPtr(0)) sm.GetActorOwner().GetCarryPtr(0)->Delete();
        sm.GetActorOwner().SetCarry(0, 0);
        POI_EnableBinds(shop, 0);
        sm.GetActorOwner().InvalidateGotoPos();
        EXVector dz = VSub(POI_Get(crime)->m_Pos, sm.GetActorOwner().GetHeadPos());
        EXVector dx = VSub(POI_Get(crime)->m_Pos, sm.GetActorOwner().GetHeadPos());
        sm.GetActorOwner().SetRotY((float)atan2(dx.x, dz.z));
        return 1;
    }
    case MSG_EXT:
        switch (stage) {
        case 0:
            if (sm.GetActorOwner().HasAnimEnded()) {
                if (!POI_VendorBind(spot, sm.GetActorOwner().GetRefID())) {
                    sm.Return(0, 0, 0, 0);
                    return 1;
                }
                stage = 1;
                sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
                sm.GetActorOwner().GotoVendorPOI(spot, -1);
            }
            return 1;
        case 1:
            if (sm.GetActorOwner().SqDistToTarget() < 0.01f) {
                sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
                if (POI_GetComData(spot, 0, 0, 0) == 4000) {
                    if (sm.GetActorOwner().IsFacingYRot(POI_VendorStandPos(spot, 0), 5.0f)) {
                        stage = 2;
                        if (bRefuse) sm.GetActorOwner().SetAnimType(0x9000100, 0, 1);
                        else sm.GetActorOwner().SetAnimType(0x90000e4, 0, 1);
                        return 1;
                    }
                    sm.GetActorOwner().SetTargetYRot(POI_VendorStandPos(spot, 0), 5.0f);
                    return 1;
                }
                sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            }
            return 1;
        case 2:
            return 1;
        case 3:
            if (sm.GetActorOwner().HasAnimEnded()) stage = 5;
            return 1;
        case 4:
            if (POI_GetComData(spot, 0, 0, 0) != 4000) stage = 5;
            return 1;
        case 5:
            stage = 6;
            sm.GetActorOwner().GotoPosPoly(pos, poly);
            sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
            return 1;
        case 6:
            if (sm.GetActorOwner().SqDistToTarget() < 0.01f) sm.Return(1, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_EXIT:
        POI_VendorUnbind(spot, sm.GetActorOwner().GetRefID());
        POI_EnableBinds(shop, 1);
        return 1;
    AU_DEFAULT_MSGS
    case MSG_ROBBERY_TAKE:
        if (sm.GetActorOwner().IsMsgOnStack(MSG_ROBBERY_TAKE)) return 1;
        extorter = sm.m_Msg.m_Data[0];
        stage = 3;
        if (!bRefuse) {
            carry = ActorCarryable::Create(0x100003c, 0x20000c7)->GetRefID();
            sm.GetActorOwner().SetCarry(carry, 0);
            sm.GetActorOwner().SetAnimType(0x90000a2, 0, 1);
        } else {
            stage = 4;
            carry = 0;
            sm.GetActorOwner().SetAnimType(0x900012a, 0, 1);
        }
        sm.m_Unk9c = carry;
        return 1;
    case (MsgType)0x15:  // TODO name: the extorter is gone
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x15)) return 1;
        stage = 5;
        return 1;
    }
    return 0;
}

// Extortionist: prepares at the extortion POI (com data 3000 calls the lookout), walks to the
// scene of crime by the shop and summons the shopkeeper (MSG_EXTORTION), walks to its own
// stand at the extortion point and, once the keeper arrives (com data 4000), takes the money or
// punches a keeper who refuses, then flags the POI with a getaway home (com data 5000) and
// walks there to be deleted. If killed on the job its accomplice is told (message 0x15).
signed char STATE_FS_Extortion(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &shop = sm.GetIntRef(i);
    int &meet = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    int &carry = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(6);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        if (!sm.m_Msg.m_Data[0]) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_EXIT:
        if (sm.GetActorOwner().GetCarryPtr(1)) sm.GetActorOwner().GetCarryPtr(1)->Delete();
        sm.GetActorOwner().SetCarry(0, 1);
        if (sm.GetActorOwner().GetCarryPtr(0)) sm.GetActorOwner().GetCarryPtr(0)->Delete();
        sm.GetActorOwner().SetCarry(0, 0);
        if (POI_VendorUnbind(poi, sm.GetActorOwner().GetRefID())) {
            POI_SetComData(poi, 0, 0, 0, 0);
            POI_SetComData(shop, 0, 0, 0, 0);
            if (sm.GetActorOwner().IsDead()) POI_SendMsg(poi, sm.GetActorOwner().GetRefID(), MsgItem((MsgType)0x15));
        }
        POI_VendorUnbind(meet, sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_ENTER: {
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        Actor &actor = sm.GetActorOwner();
        shop = POI_GetNearestFlag(POI_Get(poi)->m_Pos, 0x20, 0, actor.GetNavZone(), actor.CanJump(), 0, -1.0f);
        if (!shop) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        Actor &a = sm.GetActorOwner();
        meet = POI_GetNearestType(POI_Get(shop)->m_Pos, 0x42000033, 0, a.GetNavZone(), a.CanJump(), 0);
        if (!meet) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        SM_GOSUB(STATE_CrimePrepare, poi, 0, 0, 0);
        return 1;
    }
    case MSG_EXT:
        sm.GetActorOwner().SetStatus(7);
        switch (stage) {
        case 0:
            stage = 1;
            POI_SetComData(poi, 3000, 0, 0, 0);
            sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
            sm.GetActorOwner().GotoPOI(meet);
            break;
        case 1:
            if (sm.GetActorOwner().SqDistToTarget() < 0.01f) {
                sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
                if (sm.GetActorOwner().IsFacingYRot(POI_VendorStandPos(shop, 0), 5.0f)) {
                    stage = 2;
                    Actor::SendMsg(POI_VendorGetID(shop, 0), MsgItem(MSG_EXTORTION), 0.0f);
                    sm.GetActorOwner().SetAnimType(0x90000e3, 0, 1);
                    break;
                }
                sm.GetActorOwner().SetTargetYRot(POI_VendorStandPos(shop, 0), 5.0f);
            }
            break;
        case 2:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 3;
                sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
                sm.GetActorOwner().GotoVendorPOI(poi, -1);
                EVD::Instance(1)->Dial999(sm.GetActorOwner().GetFeetPos(), poi, sm.GetActorOwner().GetRefID(), (CrimeType::Flags)3);
            }
            break;
        case 3:
            if (sm.GetActorOwner().SqDistToTarget() < 0.01f) {
                sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
                if (sm.GetActorOwner().IsFacingYRot(POI_VendorStandPos(poi, 1), 5.0f)) {
                    stage = 4;
                    POI_SetComData(poi, 4000, 0, 0, 0);
                    sm.GetActorOwner().SetAnimType(0x90000e3, 0, 1);
                    break;
                }
                sm.GetActorOwner().SetTargetYRot(POI_VendorStandPos(poi, 1), 5.0f);
            }
            break;
        case 4:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 5;
                carry = Actor::SendMsg(POI_VendorGetID(poi, 1), MsgItem(MSG_ROBBERY_TAKE, sm.GetActorOwner().GetRefID()), 0.0f);
                if (!carry) sm.GetActorOwner().SetAnimType(0x9000131, 0, 1);
                else sm.GetActorOwner().SetAnimType(0x90000a4, 0, 1);
            }
            break;
        case 5:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 6;
                Actor &actor = sm.GetActorOwner();
                int home = POI_GetRandomType(0x42000004, 0, actor.GetNavZone(), actor.CanJump(), 0);
                POI_SetComData(poi, 5000, home, 0, 0);
                POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
                SM_GOSUB(STATE_GotoLocation, home, 0x110, 0, 0);
            }
            break;
        case 6:
            sm.GetActorOwner().Delete();
            break;
        }
        return 1;
    case MSG_TAKE_ITEM:
        if (sm.GetActorOwner().IsMsgOnStack(MSG_TAKE_ITEM)) return 1;
        sm.GetActorOwner().SetCarry(carry, 1);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// Mugger: loiters suspiciously by the ATM nearest its POI. Every 3.5 seconds it looks for
// anyone within 3 units carrying a bag (type 8), and otherwise takes whoever is at the ATM
// once they are within 1 of it. When the victim then moves off (more than sqrt 10 on the
// ground) it runs them down, swings a crowbar (MSG_MUGGING to the victim) and escapes to a
// loiter point near a random spot. A vanished victim ends it.
signed char STATE_FS_Mugger(StateMachine &sm)
{
    int i = 0;
    EXVector &victimPos = sm.GetVectorRef(i);
    int &stage = sm.GetIntRef(i);
    int &atm = sm.GetIntRef(i);
    int &victim = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    float &runDistSq = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(7);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (victim && !Actor::GetPtrFromID(victim)) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        switch (stage) {
        case 0:
            if (atm && !sm.GetActorOwner().IsFacingYRot(POI_Get(atm)->m_Pos, 5.0f)) {
                sm.GetActorOwner().SetTargetYRot(POI_Get(atm)->m_Pos, 5.0f);
                return 1;
            }
            stage = 1;
            sm.GetActorOwner().SetAnimType(0x9000108, 0, 1);
            return 1;
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) sm.GetActorOwner().SetAnimType(0x9000108, 0, 1);
            timer -= EXBaseApp::GameFrameRateMul();
            if (timer < 0.0f) {
                timer = EXBaseApp::NominalGameFrameRate() * 3.5f;
                for (ActorCarryable *p = ActorCarryable::GetFirst(); p; p = p->GetNext()) {
                    if (p->GetType() != CARRY_BAG || !p->GetOwner()) continue;
                    if (City_DistSq(p->GetWorldPos(), sm.GetActorOwner().GetFeetPos()) > 9.0f) continue;
                    victim = p->GetOwner();
                    stage = 2;
                    victimPos = Actor::GetPtrFromID(victim)->GetFeetPos();
                }
                return 1;
            }
            if (!atm || !POI_CustomerGetID(atm, 0)) return 1;
            victim = POI_CustomerGetID(atm, 0);
            if (!Actor::GetPtrFromID(victim)) return 1;
            {
                EXVector pos = Actor::GetPtrFromID(victim)->GetFeetPos();
                if (City_DistSq(pos, POI_Get(atm)->m_Pos) < 1.0f) {
                    stage = 2;
                    victimPos = pos;
                }
            }
            return 1;
        case 2: {
            EXVector d = Actor::GetPtrFromID(victim)->GetFeetPos() - victimPos;
            if (d.x * d.x + d.z * d.z > runDistSq) stage = 3;
            return 1;
        }
        case 3:
            POI_CustomerUnbind(poi, sm.GetActorOwner().GetRefID());
            sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
            sm.GetActorOwner().GotoLoc(Actor::GetPtrFromID(victim)->OrgLoc());
            if (sm.GetActorOwner().SqDistToTarget() < 1.0f) {
                stage = 4;
                sm.GetActorOwner().SetAnimType(0x9000034, 0, 1);
                Actor::GetPtrFromID(victim)->SendMsg(MsgItem(MSG_MUGGING), 0.0f);
            }
            return 1;
        case 4:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 5;
                Actor &actor = sm.GetActorOwner();
                poi = POI_GetNearestType(actor.GetFeetPos() + actor.RandomPosition(), 0x42000005, 0, actor.GetNavZone(), actor.CanJump(), 0);
                POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
                SM_GOSUB(STATE_GotoLocation, poi, 0x310, 0, 0);
            }
            return 1;
        case 5:
            POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
            return 1;
        }
        return 1;
    case MSG_ENTER:
        if (sm.GetActorOwner().GetAttrib() & 0x16) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        if (poi) {
            Actor &actor = sm.GetActorOwner();
            atm = POI_GetNearestType(POI_Get(poi)->m_Pos, 0x42000009, 0, actor.GetNavZone(), actor.CanJump(), 0);
            if (atm) {
                victim = 0;
                runDistSq = 10.0f;
                sm.GetActorOwner().InvalidateGotoPos();
                return 1;
            }
        }
        sm.Return(0, 0, 0, 0);
        return 1;
    case MSG_EXIT:
        POI_Unbind(poi, sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_RETURN:
        if (!sm.m_Msg.m_Data[0]) sm.Return(0, 0, 0, 0);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// The smaller of a and b, by reference (retail's inline min).
static __forceinline const float &City_Min(const float &a, const float &b) { return b < a ? b : a; }

// Walks the john beside the hooker: side is +-0.7 along her right (her facing crossed with up),
// with the anim sped up (to at most double) the further he has to catch up.
static __forceinline void City_WalkBeside(Actor &actor, int hooker, float side)
{
    Actor *p = Actor::GetPtrFromID(hooker);
    if (!p) return;
    float r = p->GetRotation().y;
    EXVector facing((float)sin(r), 0.0f, (float)cos(r), 0.0f);
    EXVector target = facing.Cross(EXVector(0.0f, 1.0f, 0.0f, 0.0f)) * side + p->GetFeetPos();
    EXVector d = target - actor.GetFeetPos();
    float speed = d.LengthSquared() * 0.25f + 1.0f;
    float maxSpeed = 2.0f;
    actor.m_AnimSpeedMult = City_Min(speed, maxSpeed);
    actor.GotoPosSlow(target);
}

// The hooker's john: chats her up, pays and takes his change through the POI com data (1-3),
// then settles (com data 6 with the nearest home as the meeting point) and walks there at her
// side, turning to face her for the first 180 frames. Both are deleted on arrival. If she is
// lost or calls it off (message 0x15) he gives up (vanishing if already near the home).
signed char STATE_FS_BuyHooker(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    int &hooker = sm.GetIntRef(i);
    int &home = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    int &bOff = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        switch (stage) {
        case 0:
            sm.GetActorOwner().Speak(0x1af00430, 0);
            sm.GetActorOwner().SetAnimType(0x90000e3, 0, 1);
            stage = 1;
            return 1;
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().SetAnimType(0x90000e4, 0, 1);
                stage = 2;
                return 1;
            }
            POI_SetComData(poi, 1, 0, 0, 0);
            return 1;
        case 2:
            if (!POI_VendorIsThere(poi)) {
                POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
                return 1;
            }
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 3;
                sm.GetActorOwner().Speak(0x1af00433, 0);
                sm.GetActorOwner().SetAnimType(0x90000a2, 0, 1);
                return 1;
            }
            POI_SetComData(poi, 2, 0, 0, 0);
            return 1;
        case 3:
            if (!POI_VendorIsThere(poi)) {
                POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
                return 1;
            }
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 4;
                Actor &actor = sm.GetActorOwner();
                home = POI_GetNearestType(actor.GetFeetPos(), 0x42000004, 0, actor.GetNavZone(), actor.CanJump(), 0);
                hooker = POI_VendorGetID(poi, 0);
                if (!home) {
                    POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
                    POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
                }
                timer = 180.0f;
                POI_SetComData(poi, 6, home, 0, 0);
                return 1;
            }
            POI_SetComData(poi, 3, 0, 0, 0);
            return 1;
        case 4:
            stage = 5;
            sm.GetActorOwner().m_MeetActorID = hooker;
            POI_CustomerUnbind(poi, sm.GetActorOwner().GetRefID());
            return 1;
        case 5: {
            Actor *pHooker = Actor::GetPtrFromID(hooker);
            EXVector d = POI_Get(home)->m_Pos - sm.GetActorOwner().GetFeetPos();
            float distSq = d.y * d.y + d.z * d.z + d.x * d.x;
            if ((!pHooker || bOff) && distSq < 9.0f) sm.GetActorOwner().Delete();
            else if (!pHooker || bOff) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
            timer -= XApp::GameFrameRateMul();
            if (timer > 0.0f) {
                sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
                if (sm.GetActorOwner().IsFacingYRot(pHooker->GetHeadPos(), 5.0f)) {
                    sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                    return 1;
                }
                sm.GetActorOwner().SetTargetYRot(pHooker->GetHeadPos(), 5.0f);
                return 1;
            }
            sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
            if (distSq > 25.0f) {
                if (sm.GetActorOwner().CanMoveRelative(EXVector(-0.7f, 0.0f, 0.0f, 0.0f)))
                    City_WalkBeside(sm.GetActorOwner(), hooker, -0.7f);
                else
                    City_WalkBeside(sm.GetActorOwner(), hooker, 0.7f);
                return 1;
            }
            if (distSq < 0.01f) {
                sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                sm.GetActorOwner().Delete();
                return 1;
            }
            sm.GetActorOwner().m_AnimSpeedMult = 1.0f;
            sm.GetActorOwner().GotoPOI(home);
            return 1;
        }
        }
        return 1;
    case MSG_ENTER:
        if (sm.GetActorOwner().GetAttrib() & 0x10) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        sm.GetActorOwner().InvalidateGotoPos();
        return 1;
    case MSG_EXIT:
        sm.GetActorOwner().m_MeetActorID = 0;
        sm.GetActorOwner().SetHide(0);
        POI_CustomerUnbind(poi, sm.GetActorOwner().GetRefID());
        if (hooker) {
            Actor *p = Actor::GetPtrFromID(hooker);
            if (p) p->SendMsg(MsgItem((MsgType)0x15), 0.0f);
            hooker = 0;
        }
        return 1;
    case MSG_RETURN:
        if (!sm.m_Msg.m_Data[0]) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        stage = 0;
        return 1;
    AU_DEFAULT_MSGS
    case (MsgType)0x15:  // TODO name: the hooker called it off
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x15)) return 1;
        hooker = 0;
        bOff = 1;
        return 1;
    }
    return 0;
}

// Courting couple at a dating POI (the actor with the lower id leads): once both are there
// they chat; the leader offers flowers, which the other accepts half the time. Accepted, the
// leader walks to a nearby meeting point (waiting for the other to keep up) and both vanish
// there; the follower turns to face the leader for 180 frames, then walks at its side. Turned
// down, the follower drops the flowers and storms off, the leader leaves for another spot.
// The leader gives up after 3600 frames at the meeting point. A partner that vanishes or
// calls it off (message 0x15) ends it.
signed char STATE_FS_Courting(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    int &partner = sm.GetIntRef(i);
    int &bLead = sm.GetIntRef(i);
    int &flowers = sm.GetIntRef(i);
    int &bAccepted = sm.GetIntRef(i);
    int &meet = sm.GetIntRef(i);
    float &waitTimer = sm.GetFloatRef(i);
    float &faceTimer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        sm.GetActorOwner().InvalidateGotoPos();
        return 1;
    case MSG_EXIT:
        if (POI_CustomerUnbind(poi, sm.GetActorOwner().GetRefID())) POI_SetComData(poi, 0, 0, 0, 0);
        if (POI_VendorUnbind(poi, sm.GetActorOwner().GetRefID())) POI_SetComData(poi, 0, 0, 0, 0);
        sm.GetActorOwner().SetHide(0);
        if (partner) {
            Actor *p = Actor::GetPtrFromID(partner);
            partner = 0;
            if (p) p->SendMsg(MsgItem((MsgType)0x15), 0.0f);
            if (sm.GetActorOwner().GetCarryPtr(0)) sm.GetActorOwner().GetCarryPtr(0)->Delete();
            sm.GetActorOwner().SetCarry(0, 0);
        }
        return 1;
    case MSG_RETURN:
        if (!sm.m_Msg.m_Data[0]) {
            POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
            return 1;
        }
        stage = 0;
        return 1;
    case MSG_EXT: {
        if (!partner) {
            partner = POI_GetOtherActorID(poi, sm.GetActorOwner().GetRefID());
            bLead = partner > sm.GetActorOwner().GetRefID();
        }
        Actor *pPartner = Actor::GetPtrFromID(partner);
        if (partner && !pPartner) POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        switch (stage) {
        case 0:
            if (bLead && !flowers) {
                flowers = ActorCarryable::Create(0x100003c, 0x20000b4)->GetRefID();
                sm.GetActorOwner().SetCarry(flowers, 0);
            }
            if (POI_GetActorCount(poi, 0xd) == 2) stage = 1;
            return 1;
        case 1:
            stage = 2;
            if (bLead) sm.GetActorOwner().SetAnimType(0x9000021, 0, 1);
            else sm.GetActorOwner().SetAnimType(0x9000020, 0, 1);
            return 1;
        case 2:
            if (bLead && sm.GetActorOwner().HasAnimEnded()) {
                stage = 3;
                sm.GetActorOwner().SetAnimType(0x90000a2, 0, 1);
                bAccepted = pPartner->SendMsg(MsgItem(MSG_REQUEST_ITEM, flowers), 0.0f);
            }
            return 1;
        case 3:
            if (sm.GetActorOwner().HasAnimEnded()) {
                waitTimer = 3600.0f;
                faceTimer = 180.0f;
                if (bAccepted) {
                    stage = 4;
                    if (bLead) sm.GetActorOwner().SetAnimType(0x9000021, 0, 1);
                    else sm.GetActorOwner().SetAnimType(0x9000020, 0, 1);
                    return 1;
                }
                stage = 6;
                if (bLead) {
                    sm.GetActorOwner().SetAnimType(0x9000021, 0, 1);
                    return 1;
                }
                sm.GetActorOwner().SetCarry(0, 0);
                sm.GetActorOwner().SetAnimType(0x9000100, 0, 1);
            }
            return 1;
        case 4:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 5;
                Actor &actor = sm.GetActorOwner();
                meet = POI_GetNearestFlag(actor.GetFeetPos(), 0x200, poi, actor.GetNavZone(), actor.CanJump(), 0, -1.0f);
                POI_VendorUnbind(poi, sm.GetActorOwner().GetRefID());
                POI_CustomerUnbind(poi, sm.GetActorOwner().GetRefID());
                sm.GetActorOwner().GotoPOI(meet);
                sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
            }
            return 1;
        case 5:
            if (bLead) {
                EXVector d = pPartner->GetFeetPos() - sm.GetActorOwner().GetFeetPos();
                if (d.x * d.x + d.y * d.y + d.z * d.z > 9.0f) sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                else sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
                if (sm.GetActorOwner().SqDistToTarget() < 0.01f) {
                    sm.GetActorOwner().SetHide(1);
                    sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                    waitTimer -= XApp::GameFrameRateMul();
                    if (waitTimer <= 0.0f) {
                        pPartner->SendMsg(MsgItem((MsgType)0x15), 0.0f);
                        POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
                    }
                }
                return 1;
            }
            faceTimer -= XApp::GameFrameRateMul();
            if (faceTimer > 0.0f) {
                sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
                if (sm.GetActorOwner().IsFacingYRot(pPartner->GetHeadPos(), 5.0f)) {
                    sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                    return 1;
                }
                sm.GetActorOwner().SetTargetYRot(pPartner->GetHeadPos(), 5.0f);
                return 1;
            }
            sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
            {
                float distSq = City_DistSq(POI_Get(meet)->m_Pos, sm.GetActorOwner().GetFeetPos());
                if (distSq > 25.0f) {
                    if (sm.GetActorOwner().CanMoveRelative(EXVector(-0.7f, 0.0f, 0.0f, 0.0f)))
                        City_WalkBeside(sm.GetActorOwner(), partner, -0.7f);
                    else
                        City_WalkBeside(sm.GetActorOwner(), partner, 0.7f);
                    return 1;
                }
                if (distSq >= 0.01f) {
                    sm.GetActorOwner().GotoPOI(meet);
                    return 1;
                }
            }
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            sm.GetActorOwner().SetHide(1);
            if (sm.GetActorOwner().GetCarryPtr(0)) sm.GetActorOwner().GetCarryPtr(0)->Delete();
            sm.GetActorOwner().SetCarry(0, 0);
            sm.GetActorOwner().Delete();
            return 1;
        case 6:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 7;
                POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
                if (!bLead) {
                    POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
                    return 1;
                }
                Actor &actor = sm.GetActorOwner();
                meet = POI_GetNearestFlag(actor.GetFeetPos(), 0x400, poi, actor.GetNavZone(), actor.CanJump(), 0, -1.0f);
                if (POI_Get(meet)->m_Type == 0x4200001a) SM_GOSUB(STATE_GotoLocation, meet, 0x101, 0, 0);
                else SM_GOSUB(STATE_GotoLocation, meet, 0x110, 0, 0);
            }
            return 1;
        case 7:
            waitTimer -= XApp::GameFrameRateMul();
            if (waitTimer <= 0.0f) {
                POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
                POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
            }
            sm.GetActorOwner().SetHide(1);
            return 1;
        }
        return 1;
    }
    case MSG_REQUEST_ITEM: {
        if (sm.GetActorOwner().IsMsgOnStack(MSG_REQUEST_ITEM)) return 1;
        stage++;
        flowers = sm.m_Msg.m_Data[0];
        sm.GetActorOwner().SetAnimType(0x90000a4, 0, 1);
        unsigned int r = EXRand32();
        bAccepted = r & 1;
        sm.m_Unk9c = r & 1;
        return 1;
    }
    case MSG_TAKE_ITEM:
        if (sm.GetActorOwner().IsMsgOnStack(MSG_TAKE_ITEM)) return 1;
        if (sm.GetActorOwner().GetCarryPtr(0)) sm.GetActorOwner().GetCarryPtr(0)->Delete();
        sm.GetActorOwner().SetCarry(0, 0);
        sm.GetActorOwner().SetCarry(flowers, 0);
        return 1;
    AU_DEFAULT_MSGS
    case (MsgType)0x15:  // TODO name: the partner called it off
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x15)) return 1;
        partner = 0;
        if (sm.GetActorOwner().GetCarryPtr(0)) sm.GetActorOwner().GetCarryPtr(0)->Delete();
        sm.GetActorOwner().SetCarry(0, 0);
        POI_ReturnToHubLife(sm.GetActorOwner().GetRefID());
        return 1;
    }
    return 0;
}
