#include "Actor.h"
#include "ActorUtil.h"
#include "StateMachine.h"
#include "EXRand.h"
#include "EXBaseApp.h"
#include "XApp.h"
#include "GeoSectionManager.h"
#include "ActorLocation.h"
#include "FEText.h"
#include "FEHud.h"
#include "XItemHandler_Camera.h"
#include "EXSoundTagStandIn.h"
#include "EXGeoMap.h"
#include "EXGeoPathNode.h"
#include "ActorOptions.h"
#include "WeaponTable.h"
#include "ActorCombat.h"
#include "ActorCarryable.h"
#include "ActorProfile.h"

void Objective_NotifyPlayerSeen(Actor *pActor);  // TODO stand-in (Objective.obj)
void Objective_NotifyActorStartTalking(Actor *pActor);
void Objective_NotifyActorStopTalking(Actor *pActor);
void Objective_NotifyAlarm(Actor *pActor);
XItemHandler_Camera *XGetCamera(unsigned int i);
extern unsigned int gCurrentMap_GeoHash;

// "Open sesame" dialogue per level (sizeof 0x28): the lines an informant speaks before opening up.
class _SesameDataItem {
public:
    _SesameDataItem()
    {
        m_Map = 0;
        m_nLines = 0;
        m_14 = 0;
        m_StartLine = 0;
        m_Deadline = 0;
        m_TimeOut = 0;
        m_Lines[0] = 0;
        m_Lines[1] = 0;
        m_Lines[2] = 0;
        m_Lines[3] = 0;
    }
    _SesameDataItem(unsigned int map, int startLine, int deadline, int timeOut, unsigned int l0, unsigned int l1,
                    unsigned int l2, unsigned int l3)
    {
        m_StartLine = startLine;
        m_Deadline = deadline;
        m_TimeOut = timeOut;
        m_Lines[0] = l0;
        m_Lines[1] = l1;
        m_Map = map;
        m_Lines[2] = l2;
        m_nLines = 0;
        m_14 = 0;
        m_Lines[3] = l3;
        if (m_Lines[0]) m_nLines = 1;
        if (m_Lines[1]) m_nLines = 2;
        if (m_Lines[2]) m_nLines = 3;
        if (m_Lines[3]) m_nLines = 4;
    }
    unsigned int m_Map;     // 0x00 level geo hash (0: default)
    int m_nLines;           // 0x04
    int m_TimeOut;          // 0x08 seconds before talking starts unprompted (0: never)
    int m_StartLine;        // 0x0c 1-based line on which the conversation counts as started
    int m_Deadline;         // 0x10 seconds put on the HUD timer when the player started it
    int m_14;               // 0x14
    unsigned int m_Lines[4];// 0x18 speech sounds
};
class _SesameData {
public:
    _SesameData()
    {
        m_Items[m_Count++] = _SesameDataItem(0, 4, 2, 0, 0x1af0015a, 0x1af0015a, 0x1af0015a, 0x1af0015a);
        m_Items[m_Count++] = _SesameDataItem(0x100014e, 4, 4, 0, 0x1af005d8, 0x1af005d6, 0x1af005d9, 0x1af005d7);  // HT_File_DeadMan
        m_Items[m_Count++] = _SesameDataItem(0x1000073, 1, 2, 5, 0x1af005fd, 0, 0, 0);  // HT_File_1G_GWarfare
        m_Items[m_Count++] = _SesameDataItem(0x10000b4, 1, 7, 0, 0x1af00642, 0x1af00643, 0x1af00644, 0);  // HT_File_Labs
    }
    _SesameDataItem *Get(unsigned int)
    {
        for (int i = 0; i < m_Count; i++)
            if (m_Items[i].m_Map == gCurrentMap_GeoHash) return &m_Items[i];
        return &m_Items[0];
    }
    int m_Count;
    _SesameDataItem m_Items[32];
};
static _SesameData s_Sesame;
static EXSoundTag s_SesameSound;

signed char STATE_Perv(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    sm.GetActorOwner().m_WatchFlags = 0;
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_ATTACKED_BY_PREDATOR:
        if (sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR)) return 1;
        Objective_NotifyPlayerSeen(&sm.GetActorOwner());
        return 1;
    case MSG_EXT:
        if (stage == 0 || sm.GetActorOwner().HasAnimEnded()) {
            stage = 1;
            switch (EXRand32() & 1) {
            case 0:
                sm.GetActorOwner().SetAnimType(0x90003b0, 0, 1);
                break;
            case 1:
                sm.GetActorOwner().SetAnimType(0x90003ae, 0, 1);
                break;
            }
        }
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR)) return 1;
        Objective_NotifyPlayerSeen(&sm.GetActorOwner());
        return 1;
    case MSG_INCOMING_ATTACK:
        if (sm.GetActorOwner().IsMsgOnStack(MSG_INCOMING_ATTACK)) return 1;
        if (sm.GetActorOwner().GetFeetPos().y <= ((StructMsg_IncomingAttack *)sm.m_Msg.m_Data[0])->m_Pos.y)
            Objective_NotifyPlayerSeen(&sm.GetActorOwner());
        return 1;
    case MSG_ATTRACT:
        if (sm.GetActorOwner().IsMsgOnStack(MSG_ATTRACT)) return 1;
        if (!sm.GetActorOwner().Holographic()) AU_HandleAttractNoise(sm);
        Objective_NotifyPlayerSeen(&sm.GetActorOwner());
        return 1;
    }
    return 0;
}

signed char STATE_Bouncer(StateMachine &sm)
{
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_DanceGirl(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    sm.GetActorOwner().m_WatchFlags = 0;
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (stage == 0 || sm.GetActorOwner().HasAnimEnded()) {
            stage = 1;
            switch (EXRand32() % 3) {
            case 0:
                sm.GetActorOwner().SetAnimType(0x90003b1, 0, 1);
                break;
            case 1:
                sm.GetActorOwner().SetAnimType(0x90003af, 0, 1);
                break;
            case 2:
                sm.GetActorOwner().SetAnimType(0x90000db, 0, 1);
                break;
            }
        }
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_DeathMetal(StateMachine &sm)
{
    int i = 0;
    EXVector &pos = sm.GetVectorRef(i);
    int &stage = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    sm.GetActorOwner().m_WatchFlags = 5;
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        sm.GetActorOwner().SetPositionEx(pos);
        if (stage == 0 || sm.GetActorOwner().HasAnimEnded()) {
            stage = 1;
            switch (EXRand32() & 1) {
            case 0:
                sm.GetActorOwner().SetAnimType(0x90003b3, 0, 1);
                break;
            case 1:
                sm.GetActorOwner().SetAnimType(0x90003b3, 0, 1);
                break;
            }
        }
        return 1;
    case MSG_RETURN:
        pos = sm.GetActorOwner().GetFeetPos();
        return 1;
    case MSG_ENTER:
        pos = sm.GetActorOwner().GetFeetPos();
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_KickDispenser(StateMachine &sm)
{
    sm.GetActorOwner().SetStatus(0xb);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        sm.GetActorOwner().SetAnimType(0x90003ee, 0, 1);
        return 1;
    case MSG_EXIT:
        GeoSectionManager::Del(0x100000f, 0x800000f);
        return 1;
    case MSG_ENTER:
        GeoSectionManager::Add(0x100000f, 0x800000f);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_Distraction_Bar(StateMachine &sm)
{
    int i = 0;
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        timer -= EXBaseApp::GameFrameRateMul();
        if (!(0.0f > timer)) return 1;
    case MSG_ENTER:
        sm.Return(0, 0, 0, 0);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_MissionJunkie(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        switch (stage) {
        case 0:
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            timer -= XApp::GameFrameRateMul();
            if (0.0f >= timer) {
                stage = 1;
                timer = 1800.0f;
                sm.GetActorOwner().SetAnimType(0x9000104, 0, 1);
            }
            break;
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) stage = 0;
            break;
        }
        return 1;
    case MSG_EXIT:
        GeoSectionManager::Del(0x100000f, 0x800000e);
        return 1;
    case MSG_ENTER:
        stage = 0;
        timer = 1800.0f;
        sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        GeoSectionManager::Add(0x100000f, 0x800000e);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// NEAR: retail shares the ATTRACT block's return-1 epilogue for every other return 1 (and swaps ecx/edx)
signed char STATE_MissionSuicide(StateMachine &sm)
{
    sm.GetActorOwner().SetStatus(0xb);
    if (sm.m_Msg.m_Type == MSG_ATTRACT) {
        sm.Return(0, 0, 0, 0);
        return 1;
    }
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        {
            float a = sm.GetActorOwner().GetRotY() * 0.159155f;
            SM_GOTO(STATE_RagdollJump, (int)(a * 360.0f), 0, 0, 0);
        }
        return 1;
    case MSG_EXIT:
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// NEAR: the 0x2b/0x2c pair lays out differently (retail: 0x2b falls into the Gosub, 0x2c keeps its own return 1)
signed char STATE_MissionPoliceIdle(StateMachine &sm)
{
    sm.GetActorOwner().SetStatus(0xa);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        return 1;
    case MSG_ENTER:
        sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        return 1;
    AU_MSG_MUGGING AU_MSG_SOMEONE_UNDER_ATTACK AU_MSG_ATTACKED_BY_ACTOR AU_MSG_LOUD_NOISE AU_MSG_FIGHT_ME
    AU_MSG_DRIVE_BY AU_MSG_INCOMING_ATTACK AU_MSG_ATTRACT
    case (MsgType)0x2b:  // TODO name: arrest request (data = actor ids)
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x2b)) return 1;
    apprehend:
        SM_GOSUB(STATE_Police_ApprehendActor, sm.m_Msg.m_Data[0], sm.m_Msg.m_Data[1], 0, 0);
        return 1;
    case (MsgType)0x2c:  // TODO name
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x2c)) return 1;
        goto apprehend;
    AU_MSG_SOMEONE_SEEN_PREDATOR AU_MSG_ATTACKED_BY_PREDATOR
    }
    return 0;
}

signed char STATE_Distraction_Stage(StateMachine &sm)
{
    int i = 0;
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        timer -= EXBaseApp::GameFrameRateMul();
        if (0.0f > timer) {
            int anims[3];
            anims[0] = 0x900004e;
            anims[1] = 0x900004e;
            anims[2] = 0x90000db;
            int idx = EXRand32() % 3;
            sm.GetActorOwner().SetAnimType(anims[idx], 0, 1);
            unsigned int rate = EXBaseApp::NominalGameFrameRate();
            timer = (g_EXRandClass.Randf() * 10.0f + 5.0f) * rate;
        }
        return 1;
    case MSG_ENTER:
        return 1;
    case MSG_EXIT:
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// NEAR: block layout of the shared Return(0,0,0,0) path (RETURN case vs EXT timeout) differs
signed char STATE_Distraction_PoolTable(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        if (sm.m_Msg.m_Data[0]) break;
    finish:
        sm.Return(0, 0, 0, 0);
        return 1;
    case MSG_ENTER: {
        Actor &actor = sm.GetActorOwner();
        poi = POI_GetNearestType(actor.GetFeetPos(), 0x4200001d, 0, actor.GetNavZone(), actor.CanJump(), 0);
        timer = (g_EXRandClass.Randf() + 1.0f) * 300.0f;
        SM_GOSUB(STATE_GotoLocation, poi, 0, 0, 0);
        return 1;
    }
    case MSG_EXT:
        timer -= XApp::GameFrameRateMul();
        if (0.0f >= timer) goto finish;
        sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
        if (sm.GetActorOwner().IsFacingYRot(POI_Get(poi)->m_YRot, 5.0f))
            sm.GetActorOwner().SetAnimType(0x9000022, 0, 1);
        sm.GetActorOwner().SetTargetYRot(POI_Get(poi)->m_YRot, 5.0f);
        return 1;
    case MSG_EXIT:
        return 1;
    AU_DEFAULT_MSGS
    default:
        return 0;
    }
    return 1;
}

// Collection mission: wait at the spot (walking back if moved more than 1 away) until told to walk to
// the van (MSG_GOTO_VAN, the reply is this actor), then stand once the collection is done.
signed char STATE_MissionCollectionWaitForVan(StateMachine &sm)
{
    int i = 0;
    EXVector &home = sm.GetVectorRef(i);
    EXVector &van = sm.GetVectorRef(i);
    int &poly = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        switch (stage) {
        case 0: {
            EXVector d = sm.GetActorOwner().GetFeetPos() - home;
            if (d.y * d.y + d.z * d.z + d.x * d.x > 1.0f) {
                sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);  // HT_AnimType_Walk
                sm.GetActorOwner().GotoPosPoly(home, poly);
                return 1;
            }
        }
        // fall through
        case 3:
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);  // HT_AnimType_Stand
            return 1;
        case 1:
            stage = 2;
            sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
            sm.GetActorOwner().GotoPosSlow(van);
            return 1;
        }
        return 1;
    case MSG_ENTER:
        stage = 0;
        home = sm.GetActorOwner().GetFeetPos();
        poly = sm.GetActorOwner().GetNavPolyHandle();
        sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        return 1;
    case MSG_EXIT:
        return 1;
    case MSG_GOTO_VAN:
        if (sm.GetActorOwner().IsMsgOnStack(MSG_GOTO_VAN)) return 1;
        stage = 1;
        van.x = (float)sm.m_Msg.m_Data[0];
        van.y = (float)sm.m_Msg.m_Data[1];
        van.z = (float)sm.m_Msg.m_Data[2];
        sm.m_Unk9c = sm.GetActorOwner().m_RefID;
        return 1;
    case MSG_VAN_DONE:
        if (sm.GetActorOwner().IsMsgOnStack(MSG_VAN_DONE)) return 1;
        sm.m_Unk9c = sm.GetActorOwner().m_RefID;
        stage = 3;
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// Informant: stands until the player locks on and greets (camera target + HUD 2), or the level's
// timeout runs out, then speaks the level's lines one after another; from the start line on the
// conversation counts (and a player started one puts the deadline on the HUD timer); after the last
// line the context icon goes and the flag channel from the trigger data is forced.
signed char STATE_MissionOpenSesame(StateMachine &sm)
{
    int i = 0;
    int &line = sm.GetIntRef(i);
    _SesameDataItem *&pData = *(_SesameDataItem **)&sm.GetIntRef(i);
    int &channel = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    int &bPlayer = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    sm.GetActorOwner().m_WatchFlags = 5;
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT: {
        _SesameDataItem *p = pData;
        switch (stage) {
        case 0:
            if (XGetCamera(0)->m_TargetID == sm.GetActorOwner().m_RefID && FEHud::Instance(1)->m_LockState == 2) {
                stage = 1;
                bPlayer = 1;
            }
            if (p->m_TimeOut > 0) {
                timer = timer - XApp::GameFrameRateMul();
                if (timer <= 0.0f) {
                    stage = 1;
                    bPlayer = 0;
                }
            }
            return 1;
        case 1:
            if (s_SesameSound.IsPlaying()) return 1;
            if (line == p->m_StartLine - 1) {
                Objective_NotifyActorStartTalking(&sm.GetActorOwner());
                if (bPlayer) {
                    FEHud::Instance(1)->m_Unkd20 = (float)(p->m_Deadline * 60);
                    FEHud::Instance(1)->m_Unkd24 = 1;
                }
            } else if (line >= p->m_nLines) {
                sm.GetActorOwner().SetDefaultContextIcon(0, 1);
                Objective_NotifyActorStopTalking(&sm.GetActorOwner());
                POI_ForceFlagChannel(channel, sm.GetActorOwner().m_RefID);
                stage = 2;
            }
            if (line < p->m_nLines) {
                s_SesameSound.StartSound(p->m_Lines[line], sm.GetActorOwner().GetHeadPos(), -1, 0);
                line++;
            }
            return 1;
        }
        return 1;
    }
    case MSG_ENTER:
        sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);  // HT_AnimType_Stand
        channel = sm.m_Msg.m_Data[0];
        pData = s_Sesame.Get(0);
        stage = 0;
        timer = (float)(pData->m_TimeOut * 60);
        return 1;
    case MSG_EXIT:
    case MSG_SUSPEND:
        s_SesameSound.RemoveAll(1);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// Informant met at a POI: wait for the partner (two actors bound), turn to face it and chat; an
// informant with the vocal context icon speaks the level's lines once the player locks on and
// greets, which starts the conversation timer and finally forces the POI's flag channel.
signed char STATE_FS_OpenSesame(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    int &other = sm.GetIntRef(i);
    int &bVocal = sm.GetIntRef(i);
    int &line = sm.GetIntRef(i);
    _SesameDataItem *&pData = *(_SesameDataItem **)&sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    sm.GetActorOwner().m_WatchFlags = 5;
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT: {
        Actor *pOther = Actor::GetPtrFromID(other);
        _SesameDataItem *p = pData;
        switch (stage) {
        case 0:
            if (POI_GetActorCount(poi, 0xd) == 2) {
                other = POI_GetOtherActorID(poi, sm.GetActorOwner().m_RefID);
                stage = 1;
            }
            break;
        case 1:
            if (!pOther) {
                sm.Return(0, 0, 0, 0);
                return 1;
            }
            if (sm.GetActorOwner().IsFacingYRot(pOther->GetFeetPos(), 5.0f)) {
                stage = 2;
                // HT_AnimMode_GEN_Stand_Chat1b, or the next one for the lower id of the pair
                sm.GetActorOwner().SetAnimType(0x90000e1 + (other < sm.GetActorOwner().m_RefID), 0, 1);
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(pOther->GetFeetPos(), 5.0f);
            return 1;
        case 2:
            if (bVocal && XGetCamera(0)->m_TargetID == sm.GetActorOwner().m_RefID && FEHud::Instance(1)->m_LockState == 2)
                stage = 3;
            break;
        case 3:
            if (s_SesameSound.IsPlaying()) break;
            if (line == p->m_StartLine - 1) {
                Objective_NotifyActorStartTalking(&sm.GetActorOwner());
                FEHud::Instance(1)->m_Unkd20 = (float)(p->m_Deadline * 60);
                FEHud::Instance(1)->m_Unkd24 = 1;
            } else if (line >= p->m_nLines) {
                sm.GetActorOwner().SetDefaultContextIcon(0, 1);
                Objective_NotifyActorStopTalking(&sm.GetActorOwner());
                POI_ForceFlagChannel(poi, sm.GetActorOwner().m_RefID);
                stage = 4;
            }
            if (line < p->m_nLines) {
                s_SesameSound.StartSound(p->m_Lines[line], sm.GetActorOwner().GetHeadPos(), -1, 0);
                line++;
            }
            break;
        }
        return 1;
    }
    case MSG_EXIT:
        if (bVocal) s_SesameSound.RemoveAll(1);
        POI_CustomerUnbind(poi, sm.GetActorOwner().m_RefID);
        POI_VendorUnbind(poi, sm.GetActorOwner().m_RefID);
        return 1;
    case MSG_ENTER:
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        pData = s_Sesame.Get(0);
        sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);  // HT_AnimType_Stand
        sm.GetActorOwner().InvalidateGotoPos();
        if (sm.GetActorOwner().m_DefaultContextIcon == 0x4000414) bVocal = 1;  // HT_Script_ContextIcon_Vocal
        return 1;
    case MSG_SUSPEND:
        if (bVocal) s_SesameSound.RemoveAll(1);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// Distraction runner: run the nearest path node by node (raising each node's flag channel on
// arrival) and stand at its end; sent at someone (local 3), keep stabbing at them, stopping
// after each blow with a one in five chance.
signed char STATE_Distraction_Path_Runner(StateMachine &sm)
{
    int i = 0;
    int &node = sm.GetIntRef(i);
    int &path = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    int &victim = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT: {
        EXGeoPath *pPath = AU_GetPathFromIndex(path);
        switch (stage) {
        case 0: {
            const EXVector3 &p = pPath->Node(node)->Position();
            sm.GetActorOwner().GotoPosSlow(AU_SnapFloorPos(EXVector(p.x, p.y, p.z, 0.0f), 0, 1));
            sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);  // HT_AnimType_Run
            stage++;
            return 1;
        }
        case 1:
            sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
            if (sm.GetActorOwner().SqDistToTarget() < 1.0f) {
                if (pPath->Node(node)->Channel()) FlagChannels[pPath->Node(node)->Channel()] = 1;
                node++;
                stage++;
            }
            return 1;
        case 2:
            stage = node < pPath->NumNodes() ? 0 : 3;
            return 1;
        case 3:
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);  // HT_AnimType_Stand
            return 1;
        case 4: {
            Actor *pVictim = Actor::GetPtrFromID(victim);
            if (!pVictim) {
                stage = 0;
                return 1;
            }
            sm.GetActorOwner().SpinToYRot(pVictim->GetHeadPos());
            int anims[8];
            anims[0] = 0x900003f;  // HT_AnimMode_STABW_LATTACK1..4, RATTACK1..4
            anims[1] = 0x9000040;
            anims[2] = 0x9000041;
            anims[3] = 0x9000042;
            anims[4] = 0x9000043;
            anims[5] = 0x9000044;
            anims[6] = 0x9000045;
            anims[7] = 0x9000046;
            sm.GetActorOwner().SetAnimType(anims[g_EXRandClass.Rand32() & 7], 0, 1);
            EXVector zero(0.0f, 0.0f, 0.0f, 0.0f);
            SendMsg_AttackedByActor(pVictim, WeaponProfile::GetWeaponData(0x49000000), zero, zero, 0.0f,
                                    &sm.GetActorOwner(), 0);  // HT_Weaponry_Predator_Fists
            stage++;
            return 1;
        }
        case 5:
            if (sm.GetActorOwner().HasAnimEnded()) stage = g_EXRandClass.Rand32() % 5 ? 4 : 0;
            return 1;
        }
        return 1;
    }
    case MSG_ENTER:
        path = AU_FindClosestPathIndex(sm.GetActorOwner().GetFeetPos(), node, 0);
        if (path == -1) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// Distraction guard: walk the nearest path in a loop, pausing at nodes flagged 0x10000 (300 ticks)
// and/or 0x20000 (600 more), raising each node's flag channel on arrival. Variable-speed anims are
// off while in this state; a guard that has seen the player raises the alarm.
signed char STATE_Distraction_Path_Guard(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &node = sm.GetIntRef(i);
    int &path = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        stage = 0;
        path = AU_FindClosestPathIndex(sm.GetActorOwner().GetFeetPos(), node, 0);
        if (path == -1) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        sm.GetActorOwner().SetVariableSpeedAnims(0);
        sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);  // HT_AnimType_Walk
        return 1;
    case MSG_EXIT:
        sm.GetActorOwner().SetVariableSpeedAnims(1);
        return 1;
    case MSG_EXT: {
        if (sm.GetActorOwner().WasPlayerSeen()) Objective_NotifyAlarm(&sm.GetActorOwner());
        EXGeoPath *pPath = AU_GetPathFromIndex(path);
        switch (stage) {
        case 0: {
            const EXVector3 &p = pPath->Node(node)->Position();
            sm.GetActorOwner().GotoPosSlow(AU_SnapFloorPos(EXVector(p.x, p.y, p.z, 0.0f), 0, 1));
            sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
            stage = 1;
            return 1;
        }
        case 1:
            sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
            if (sm.GetActorOwner().SqDistToTarget() < 1.0f) {
                timer = 0.0f;
                if (pPath->Node(node)->Flags() & 0x10000) {
                    timer = 300.0f;
                    stage = 2;
                }
                if (pPath->Node(node)->Flags() & 0x20000) {
                    timer += 600.0f;
                    stage = 2;
                }
                if (pPath->Node(node)->Channel()) FlagChannels[pPath->Node(node)->Channel()] = 1;
                if (timer == 0.0f) stage = 3;
                node++;
            }
            return 1;
        case 2:
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);  // HT_AnimType_Stand
            timer -= XApp::GameFrameRateMul();
            if (timer <= 0.0f) stage = 3;
            return 1;
        case 3:
            if (node >= pPath->NumNodes()) node = 0;
            stage = 0;
            return 1;
        }
        return 1;
    }
    AU_DEFAULT_MSGS
    }
    return 0;
}

// Copycat killer's victim search: the nearest live actor within radius of pos (in the XZ plane),
// on the killer's nav zone, not on a 0x400 surface, not itself or its last victim, and of another
// actor subgroup.
static __forceinline void Copycat_FindVictim(StateMachine &sm, const EXVector &pos, float radius, int &target,
                                             int last)
{
    float best = 9999999.0f;
    for (Actor *p = Actor::GetFirst(pos, radius, 0); p; p = p->GetNext(pos, radius, 0)) {
        Actor &me = sm.GetActorOwner();
        if (p->m_OrgLoc.m_sNavZone != me.m_OrgLoc.m_sNavZone) continue;
        if (p->m_OrgLoc.SurfaceFlags() & 0x400) continue;
        if (p->m_RefID == me.m_RefID || p->m_RefID == last) continue;
        if (!(p->GetHealth() > 0.0f)) continue;
        if (AP_GetPtrFromIndex(p->m_ProfileIndex)->m_SubGroup == AP_GetPtrFromIndex(me.m_ProfileIndex)->m_SubGroup)
            continue;
        EXVector d = sm.GetActorOwner().GetFeetPos() - p->GetFeetPos();
        float dsq = d.LengthXZSquared();
        if (best - 1.0f > dsq) {
            best = dsq;
            target = p->m_RefID;
        }
    }
}

// Copycat killer (a mission actor): holds a service blade and waits at its start point. Every
// 300 ticks it picks the nearest victim within 15 units of home (60 ticks between failed looks),
// runs at them re-picking within 5 units each second, stabs them with wristblade damage and
// shouts an alarm (message 0xb, 10 units), then goes after the next one.
signed char STATE_MissionCopycatKiller(StateMachine &sm)
{
    int i = 0;
    EXVector &home = sm.GetVectorRef(i);
    int &homePoly = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    int &target = sm.GetIntRef(i);
    int &last = sm.GetIntRef(i);
    int &carry = sm.GetIntRef(i);
    int &weapon = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    float &homeRot = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(8);
    sm.GetActorOwner().SetWatch(7);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        stage = 2;
        home = sm.GetActorOwner().GetFeetPos();
        homePoly = sm.GetActorOwner().m_OrgLoc.GetPolyHandle();
        homeRot = sm.GetActorOwner().GetRotY();
        weapon = 0x2000159;  // HT_Entity_Weapon_Serv_Blade
        if (sm.GetActorOwner().GetCarryPtr(0)) sm.GetActorOwner().GetCarryPtr(0)->Delete();
        sm.GetActorOwner().SetCarry(0, 0);
        carry = ActorCarryable::Create(0x100003c, weapon)->GetRefID();  // HT_File_Actor_Carry
        sm.GetActorOwner().SetCarry(carry, 0);
        sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);  // HT_AnimType_Stand
        return 1;
    case MSG_EXIT:
        return 1;
    case MSG_EXT:
        switch (stage) {
        case 1:
            sm.GetActorOwner().GotoPosPoly(home, homePoly);
            if (sm.GetActorOwner().SqDistToTarget() < 0.01f) {
                stage = 2;
                timer = 300.0f;
                sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                sm.GetActorOwner().SetRotY(homeRot);
                return 1;
            }
            sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);  // HT_AnimType_Walk
            return 1;
        case 2:
            target = 0;
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            timer -= XApp::GameFrameRateMul();
            if (timer <= 0.0f) {
                Copycat_FindVictim(sm, home, 15.0f, target, last);
                if (target) {
                    stage = 3;
                    return 1;
                }
                timer = 60.0f;
            }
            return 1;
        case 3: {
            timer -= XApp::GameFrameRateMul();
            if (timer <= 0.0f) {
                Copycat_FindVictim(sm, sm.GetActorOwner().GetFeetPos(), 5.0f, target, last);
                timer = 60.0f;
            }
            Actor *pVictim = Actor::GetPtrFromID(target);
            if (!pVictim) {
                stage = 1;
                return 1;
            }
            sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);  // HT_AnimType_Run
            sm.GetActorOwner().GotoLoc(pVictim->m_OrgLoc);
            if (sm.GetActorOwner().SqDistToTarget() < 1.0f) {
                stage = 4;
                last = target;
                sm.GetActorOwner().SetAnimType(GetNormalStrikeAnim(sm, WeaponProfile::GetWeaponDataFromEntity(weapon)), 0, 1);
                EXVector zero(0.0f, 0.0f, 0.0f, 0.0f);
                SendMsg_AttackedByActor(pVictim, WeaponProfile::GetWeaponData(0x49000001), zero, zero, 1000.0f,
                                        &sm.GetActorOwner(), 0);  // HT_Weaponry_Predator_Wristblades
                Actor::SendMsg(MsgItem((MsgType)0xb), sm.GetActorOwner().GetHeadPos(), 10.0f, &sm.GetActorOwner(), 0.0f);
                return 1;
            }
            return 1;
        }
        case 4:
            sm.GetActorOwner().SetStatus(9);
            if (sm.GetActorOwner().HasAnimEnded()) stage = 2;
            return 1;
        }
        return 1;
    AU_MSG_MUGGING
    case MSG_ATTACKED_BY_ACTOR:
        if (sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR)) return 1;
        AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    AU_MSG_SIMPLE(MSG_LOUD_NOISE, AU_HandleLoudNoise)
    AU_MSG_FIGHT_ME
    AU_MSG_DRIVE_BY
    AU_MSG_ATTRACT
    AU_MSG_SIMPLE(MSG_SOMEONE_SEEN_PREDATOR, AU_HandleSomeoneSeenPredator)
    case MSG_ATTACKED_BY_PREDATOR:
        if (sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR)) return 1;
        AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}
