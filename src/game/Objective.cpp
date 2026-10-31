#include "XItemHandler_Objective.h"
#include "XItemHandler_MissionTrigger.h"
#include "XItem.h"
#include "XApp.h"
#include "Actor.h"
#include "ActorProfile.h"
#include "ActorUtil.h"
#include "EXArray.h"
#include "EXString.h"
#include "EXBaseSoundManager.h"
#include "EXMemory.h"
#include "FEText.h"
#include "FELevels.h"
#include "FEPause.h"
#include "XItemHandler_Player.h"
#include "XItemHandler_Reinforcements.h"
#include "XAppStandIn.h"
#include "EXGeoFile.h"
#include "EXHashCodeFile.h"
#include "XItemHandler_Camera.h"
#include "XItemHandler_LightSet.h"
#include "FEMainMenu.h"
#include "EXBaseDisplay.h"
#include "EXBaseWnd.h"

extern unsigned int gCurrentMap_GeoHash;
extern unsigned int gTriggerFile_GeoHash;
extern signed char gKillCounts;
char *HashToCharStr(unsigned int hash, char *pBuf);
void Stats_InitPageValues(unsigned int level);
void POI_MissionEnd();
double __cdecl fmod(double, double);

class Vehicle;
extern "C" int __cdecl sprintf(char *, const char *, ...);
void AirTraffic_CreateFromProfile(unsigned int a, unsigned int b, int c);
void TrafficObjectiveTriggered(XItemHandler_Objective *pObjective);
void TrafficObjectiveActorSpawned(int id, XItemHandler_Objective *pObjective);
void Mission_Success();

template <class T> inline const T &ObjMin(const T &a, const T &b) { return a > b ? b : a; }
template <class T> inline const T &ObjMax(const T &a, const T &b) { return b >= a ? b : a; }
template <class T> inline const T &ObjMinGe(const T &a, const T &b) { return a >= b ? b : a; }

int Mission_PlayerSeenInterval = 60;
float Mission_TotalTime;
float Mission_AlarmResetSpeed;
float Mission_RitualCompleteTime;
char TimeString[8];
float Mission_TimeLeft;
unsigned short Prev_Mission_State;
float Mission_AlarmResetTimer;
float Mission_AlarmLevelLastAdd;
int ListenState;
unsigned short Mission_State;
int Mission_EscapeRadius;
float Mission_AlarmResetDelay;
int HC_AliensFans;
int ListenID;
float Mission_AlarmLevel;
EXFixedArray<unsigned char, 290> FlagChannels;
unsigned int FailMessages[16];
unsigned int BonusFailMessages[16];
float Mission_StateTimer;
float Mission_PlayerSeenTimer;
signed char Mission_AlarmMeterActive;
signed char Mission_Return;
signed char Mission_AlarmMeterHidden;
signed char Cheat_CompleteObjectives;
signed char Cheat_IgnoreObjectives;
// $E7..$E10: four folded initialisers precede Mission_StatusString in retail (TODO: identify them).
static const float s_ObjDummyInit0 = 0.0f;
static float s_ObjDummy0 = s_ObjDummyInit0;
static const float s_ObjDummyInit1 = 0.0f;
static float s_ObjDummy1 = s_ObjDummyInit1;
EXString Mission_StatusString;
static char s_ObjectiveText[48];
XItemList gTrigList;
EXVector Mission_RestartRotation;
EXVector Mission_RestartPosition;

void Mission_FixupObjectiveReferences(XItem *pItem)
{
    XItemHandler_Objective *pObj = (XItemHandler_Objective *)pItem->ItemHandler();
    for (XItem *p = gTrigList.GetFirst(); p; p = gTrigList.GetNext(p)) {
        if (p == pItem) continue;
        XItemHandler_Objective *pOther = (XItemHandler_Objective *)p->ItemHandler();
        if (!pOther->IsKindOf(EXRUNTIME_CLASS(XItemHandler_Objective))) continue;
        for (int i = 0; i < 8; i++) {
            if (pOther->m_Links[i] == pObj->m_Id) {
                pObj->m_pRefs[pObj->m_RefCount] = p;
                pObj->m_RefCount++;
            }
        }
    }
}

XItem *Mission_GetMissionItem(int id)
{
    for (XItem *p = gTrigList.GetFirst(); p; p = gTrigList.GetNext(p)) {
        EXItemHandler *pHandler = p->ItemHandler();
        if (pHandler->IsKindOf(EXRUNTIME_CLASS(XItemHandler_Objective)) && ((XItemHandler_Objective *)pHandler)->m_Id == id)
            return p;
        if (pHandler->IsKindOf(EXRUNTIME_CLASS(XItemHandler_MissionTrigger)) &&
            ((XItemHandler_MissionTrigger *)pHandler)->m_Id == id)
            return p;
    }
    return 0;
}

EXString Mission_GetStatusString() { return Mission_StatusString; }

void Mission_ResetObjectives(unsigned char state)
{
    for (XItem *p = gTrigList.GetFirst(); p; p = gTrigList.GetNext(p)) {
        if (p->ItemHandler()->IsKindOf(EXRUNTIME_CLASS(XItemHandler_Objective)))
            ((XItemHandler_Objective *)p->ItemHandler())->m_State = state;
    }
}

void Mission_ResetBonusObjectives(unsigned char state)
{
    for (XItem *p = gTrigList.GetFirst(); p; p = gTrigList.GetNext(p)) {
        if (p->ItemHandler()->IsKindOf(EXRUNTIME_CLASS(XItemHandler_Objective))) {
            XItemHandler_Objective *pObj = (XItemHandler_Objective *)p->ItemHandler();
            if (pObj->m_ObjFlags & 0x40) pObj->m_State = state;
        }
    }
}

void Mission_BonusSuccess()
{
    if (Mission_State == 1) Mission_ResetBonusObjectives(2);
}

void Objective_FlagCondition(unsigned int cond)
{
    if (Mission_State != 1) return;
    for (XItem *p = gTrigList.GetFirst(); p; p = gTrigList.GetNext(p)) {
        XItemHandler_Objective *pObj = (XItemHandler_Objective *)p->ItemHandler();
        if (pObj->IsKindOf(EXRUNTIME_CLASS(XItemHandler_Objective)) && pObj->m_State == 1) pObj->SetFlags(cond);
    }
}

unsigned short Objective_GetMissionState() { return Mission_State; }

void AlarmSound()
{
    if (!Mission_AlarmMeterActive) return;
    if (Mission_AlarmMeterHidden) return;
    if (25.0f > Mission_AlarmLevel) EXBaseSoundManager::GetSoundManager()->StartSound(0x1af0061e);
    else if (50.0f > Mission_AlarmLevel) EXBaseSoundManager::GetSoundManager()->StartSound(0x1af0061f);
    else if (75.0f > Mission_AlarmLevel) EXBaseSoundManager::GetSoundManager()->StartSound(0x1af0061d);
    else EXBaseSoundManager::GetSoundManager()->StartSound(0x1af0061b);
}

void Objective_NotifyPlayerSeen(Vehicle *pVehicle) { Objective_FlagCondition(0x1000); }

void Objective_NotifyVehicleDestroyed(Vehicle *pVehicle) { Objective_FlagCondition(0x2000); }

void Objective_NotifyActorStartTalking(Actor *pActor)
{
    ListenID = pActor->m_RefID;
    if (!ListenState) ListenState = 1;
}

void Objective_NotifyActorStopTalking(Actor *pActor)
{
    ListenID = 0;
    if (ListenState == 1) Objective_FlagCondition(0x4000);
    ListenState = 0;
}

void Objective_AddToActiveTimers(float t)
{
    for (XItem *p = gTrigList.GetFirst(); p; p = gTrigList.GetNext(p)) {
        XItemHandler_Objective *pObj = (XItemHandler_Objective *)p->ItemHandler();
        if (pObj->IsKindOf(EXRUNTIME_CLASS(XItemHandler_Objective)) && pObj->m_State == 1 &&
            ((pObj->m_SuccessMask | pObj->m_FailMask) & 2))
            pObj->m_Timer -= t;
    }
}

EX_IMPLEMENT_DYNCREATE_ROOT(XItemHandler_Objective)

XItemHandler_Objective::~XItemHandler_Objective() { gTrigList.Remove((XItem *)m_pItem); }

s8 XItemHandler_Objective::Initialise()
{
    if (!EXItemHandler::Initialise()) return 0;
    gTrigList.Add((XItem *)m_pItem);
    m_RefCount = 0;
    m_DestroyCount = 0;
    m_Destroyed = 0;
    m_State = 3;
    m_Self = (int)this;
    m_bFirstUpdate = 1;
    return 1;
}

void XItemHandler_Objective::ProcessActorDeath(Actor *pActor)
{
    if (m_Killed >= m_Count) return;
    if (m_SpawnCount >= m_Count) return;
    m_SpawnCount++;
    if (!m_Profile) return;
    if (!m_SpawnPoint) return;
    ActorProfile *pProfile = AP_GetPtr(m_Profile);
    if (!pProfile) return;
    if (pProfile->m_SubGroup == 0x4f000026) {
        AirTraffic_CreateFromProfile(m_SpawnPoint, m_Profile, m_Self);
        return;
    }
    int id = AU_SpawnActor(m_SpawnPoint, m_Profile, m_Self, 0x10);
    if (!id) return;
    Actor *pNew = Actor::GetPtrFromID(id);
    pNew->SetTargettedFlagChannel(m_TargetFlagChannel);
    pNew->m_RadarType = m_Unk84;
}

void TriggerDebugText(const EXVector &pos, char *pText) {}

void Mission_Initialise()
{
    Mission_State = 3;
    Prev_Mission_State = 3;
    Mission_StateTimer = -99.9f;
    Mission_PlayerSeenTimer = 0.0f;
    Mission_Return = 0;
    memset(&FlagChannels, 0, sizeof(FlagChannels));
    Mission_AlarmLevel = 0.0f;
    Mission_AlarmMeterActive = 0;
    Mission_StatusString.Empty();
    Mission_AlarmMeterHidden = 0;
    ListenID = 0;
    ListenState = 0;
    HC_AliensFans = 0;
    Mission_TotalTime = 0.0f;
    Mission_TimeLeft = 0.0f;
    Mission_AlarmLevelLastAdd = 0.0f;
}

void Objective_AddAlarmFame(int fame, int unused)
{
    if (!Mission_AlarmMeterActive) return;
    if (Mission_PlayerSeenTimer > 0.0f) {
        if (!((float)fame > Mission_AlarmLevelLastAdd)) return;
        Mission_AlarmLevel -= Mission_AlarmLevelLastAdd;
    }
    if (fame <= 0) return;
    Mission_PlayerSeenTimer = (float)Mission_PlayerSeenInterval;
    Mission_AlarmLevel = (float)fame + Mission_AlarmLevel;
    Mission_AlarmLevel = ObjMinGe(Mission_AlarmLevel, 100.0f);
    Mission_AlarmResetTimer = Mission_AlarmResetDelay;
    Mission_AlarmLevelLastAdd = (float)fame;
    AlarmSound();
}

void Objective_NotifyPlayerSeenByAirVehicle(EXItem *pItem)
{
    ActorProfile *pProfile = AP_GetPtr(*(int *)((char *)pItem->ItemHandler() + 0x13c));  // TODO stand-in: air vehicle handler profile
    Objective_AddAlarmFame((int)pProfile->m_AlertTime, (int)pProfile->m_Unk74);
}

// NEAR: register/prologue shape differs (retail saves ebp/esi/edi up front; RGBA argument built in place).
void XItemHandler_Objective::TriggerObjective()
{
    if (m_Hash == 0xaceface) {
        Mission_Return = 0;
        Mission_Success();
        return;
    }
    if (m_Hash == 0x1cef00d) {
        Mission_Return = 1;
        Mission_Success();
        return;
    }
    if (m_Hash == 0xac1dcafe) {
        if (Mission_State == 1) Mission_ResetBonusObjectives(2);
        m_State = 2;
        return;
    }
    if (m_Text > 0x51000000) Text_PrintObjective(m_Text, ex::RGBA(0xffffffff), 4, -1, 0, 0);
    m_State = 1;
    m_Flags = 0;
    m_Killed = 0;
    m_Timer = 0.0f;
    TrafficObjectiveTriggered(this);
    if (!m_Profile) return;
    if (!m_SpawnPoint) return;
    for (short i = 0; i < m_SpawnCount; i++) {
        unsigned int flags = 0x210;
        if (m_ObjFlags & 2) flags = 0x10210;
        ActorProfile *pProfile = AP_GetPtr(m_Profile);
        if (!pProfile) continue;
        if (pProfile->m_SubGroup == 0x4f000026) {
            AirTraffic_CreateFromProfile(m_SpawnPoint, m_Profile, m_Self);
        } else {
            m_ActorId = AU_SpawnActor(m_SpawnPoint, m_Profile, m_Self, flags);
            if (m_ActorId) {
                TrafficObjectiveActorSpawned(m_ActorId, this);
                Actor *pNew = Actor::GetPtrFromID(m_ActorId);
                pNew->SetTargettedFlagChannel(m_TargetFlagChannel);
                pNew->m_RadarType = m_Unk84;
            }
        }
    }
}

// NEAR: retail constructs the RGBA argument in its stack slot (same as ActorSpecial_RegisterPlayerKill).
void XItemHandler_Objective::ProcessActorKilled(Actor *pActor)
{
    int n = ++m_Killed;
    if (m_ObjFlags & 0x400) {
        if (pActor && (*(u32 *)&pActor->m_StateFlags & 0x40000000)) m_Flags |= 8;
    } else if (n >= m_Count) {
        m_Flags |= 8;
    }
    if (m_ObjFlags & 0x80) {
        char buf[32];
        int tmp36 = ObjMin(m_Killed, m_Count);
sprintf(buf, "%d of %d killed", tmp36, m_Count);
        int tmp33 = ex::RGBA(0xffffffff);
Text_PrintMsg(EXString(buf), tmp33, 6, 4, 0, 0);
    }
}

// NEAR: as ProcessActorKilled (RGBA argument built in place).
void XItemHandler_Objective::ProcessObjectDestroyed()
{
    int n = ++m_Destroyed;
    if (n == m_DestroyCount) m_Flags |= 0x80;
    if (!(m_ObjFlags & 0x2000)) {
        int tmp69 = ObjMin(m_Destroyed, m_DestroyCount);
sprintf(s_ObjectiveText, "%d of %d destroyed", tmp69, m_DestroyCount);
        int tmp33 = ex::RGBA(0xffffffff);
Text_PrintMsg(EXString(s_ObjectiveText), tmp33, 6, 6, 0, 0);
    }
}

// NEAR: retail reloads pActor in each branch; ours hoists the load above the flag test.
__declspec(noinline) s8 XItemHandler_Objective::CaresAbout(Actor *pActor)
{
    s8 r = 0;
    if (m_ObjFlags & 4) {
        if (m_Profile == AP_GetPtrFromIndex(pActor->m_ProfileIndex)->m_Hash) r = 1;
    } else if (m_Self == pActor->GetCreateID()) {
        r = 1;
    }
    return r;
}

void Objective_NotifyActorJustKilled(Actor *pActor)
{
    if (Mission_State != 1) return;
    for (XItem *p = gTrigList.GetFirst(); p; p = gTrigList.GetNext(p)) {
        XItemHandler_Objective *pObj = (XItemHandler_Objective *)p->ItemHandler();
        if (pObj->IsKindOf(EXRUNTIME_CLASS(XItemHandler_Objective)) && pObj->m_State == 1 && pObj->CaresAbout(pActor))
            pObj->ProcessActorKilled(pActor);
    }
}

void Objective_NotifyActorDeath(Actor *pActor)
{
    if (Mission_State != 1) return;
    for (XItem *p = gTrigList.GetFirst(); p; p = gTrigList.GetNext(p)) {
        XItemHandler_Objective *pObj = (XItemHandler_Objective *)p->ItemHandler();
        if (pObj->IsKindOf(EXRUNTIME_CLASS(XItemHandler_Objective)) && pObj->m_State == 1 && pObj->CaresAbout(pActor))
            pObj->ProcessActorDeath(pActor);
    }
}

// Any reinforcement point within 15 m of the actor sends its men.
void ReinforceMe(Actor *pActor)
{
    for (XItem *p = gReinforceList.GetFirst(); p; p = gReinforceList.GetNext(p)) {
        EXVector d = pActor->GetItem()->m_position - p->m_position;
        if (15.0f > d.LengthXYZ()) ((XItemHandler_Reinforcements *)p->ItemHandler())->SpawnActors();
    }
}

// An actor saw the predator: objectives watching it get flag 1, the alarm meter rises by the profile's
// fame, and nearby reinforcements are called. Not while the player holds this actor.
void Objective_NotifyPlayerSeen(Actor *pActor)
{
    if (Mission_State != 1) return;
    if (XItemHandler_Player::Instance()->IsHoldingActor() && XItemHandler_Player::Instance()->m_SpecialActorID == pActor->m_RefID)
        return;
    for (XItem *p = gTrigList.GetFirst(); p; p = gTrigList.GetNext(p)) {
        XItemHandler_Objective *pObj = (XItemHandler_Objective *)p->ItemHandler();
        if (pObj->IsKindOf(EXRUNTIME_CLASS(XItemHandler_Objective)) && pObj->m_State == 1 && pObj->CaresAbout(pActor))
            pObj->SetFlags(1);
    }
    ActorProfile *pProfile = AP_GetPtrFromIndex(pActor->m_ProfileIndex);
    Objective_AddAlarmFame((int)pProfile->m_AlertTime, (int)AP_GetPtrFromIndex(pActor->m_ProfileIndex)->m_Unk74);
    ReinforceMe(pActor);
}

void Objective_NotifyAlarm(Actor *pActor)
{
    if (Mission_State != 1) return;
    for (XItem *p = gTrigList.GetFirst(); p; p = gTrigList.GetNext(p)) {
        XItemHandler_Objective *pObj = (XItemHandler_Objective *)p->ItemHandler();
        if (pObj->IsKindOf(EXRUNTIME_CLASS(XItemHandler_Objective)) && pObj->m_State == 1 && pObj->CaresAbout(pActor))
            pObj->SetFlags(0x100);
    }
    ReinforceMe(pActor);
}


void Objective_NotifyAirVehicleDestroyed(EXItem *pItem)
{
    if (Mission_State != 1) return;
    EXItemHandler *pAir = pItem->ItemHandler();
    for (XItem *p = gTrigList.GetFirst(); p; p = gTrigList.GetNext(p)) {
        XItemHandler_Objective *pObj = (XItemHandler_Objective *)p->ItemHandler();
        if (pObj->IsKindOf(EXRUNTIME_CLASS(XItemHandler_Objective)) && pObj->m_State == 1 &&
            pObj->m_Self == *(int *)((char *)pAir + 0x12c)) {  // TODO stand-in: air vehicle handler owner id
            pObj->ProcessActorDeath(0);
            pObj->ProcessActorKilled(0);
        }
    }
}

void Objective_NotifyActorDeleted(Actor *pActor)
{
    if (Mission_State != 1) return;
    for (XItem *p = gTrigList.GetFirst(); p; p = gTrigList.GetNext(p)) {
        XItemHandler_Objective *pObj = (XItemHandler_Objective *)p->ItemHandler();
        if (pObj->IsKindOf(EXRUNTIME_CLASS(XItemHandler_Objective)) && pObj->m_State == 1 && pObj->CaresAbout(pActor))
            pObj->SetFlags(4);
    }
}

// Assault-course music: start the course theme, and when the mission ends play the fail or success sting.
void Mission_AssaultCourseMusic()
{
    if (!theItemEnv.MusicHash()) theItemEnv.StartMusic(0x1b000023, 0, 1, 1);
    if (Mission_State == 1 || Prev_Mission_State != 1) return;
    switch (Mission_State) {
    case 0:
        theItemEnv.StartMusic(0x1b000030, 0, 1, 1);
        break;
    case 2:
        theItemEnv.StartMusic(0x1b000031, 0, 1, 1);
        break;
    default:
        EXBaseSoundManager::GetSoundManager()->FadeMusicDown(0x32, 0);
        break;
    }
}

// A bonus objective failed: show the first matching bonus-fail message in red, reset the bonus objectives.
void Mission_BonusFail(unsigned short reason)
{
    if (Mission_State != 1) return;
    for (int i = 0; i < 16; i++) {
        if ((reason & (1 << i)) && BonusFailMessages[i]) {
            if (BonusFailMessages[i]) Text_PrintStatus(Text_Get(BonusFailMessages[i]), ex::RGBA(0x7f, 0, 0, 0x80), 4, -1, 0, 0);
            break;
        }
    }
    Mission_ResetBonusObjectives(0);
}

// The mission failed (reason bits: 1 prey alerted, 2 time expired, 4 prey escaped, 8 prey killed, 0x1000
// alerted; FailMessages[] override per bit). Mission_State 0 for 5.1 s, then the app shows the failure.
void Mission_Fail(unsigned short reason)
{
    if (Mission_State != 1) return;
    Text_DeleteMessages(2);
    if (FELevelManager::Instance(1)->IsRitual(gCurrentMap_GeoHash))
        Mission_StatusString = Text_Get(0x510006b7);
    else
        Mission_StatusString = Text_Get(0x5100032c);
    Text_PrintStatus(Mission_StatusString, ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 5, -1, 1, 0);
    Mission_State = 0;
    Mission_StateTimer = 5.1f;
    EXString why;
    signed char bFound = 0;
    for (int i = 0; i < 16; i++) {
        if ((reason & (1 << i)) && FailMessages[i]) {
            why = Text_Get(FailMessages[i]);
            bFound = 1;
        }
    }
    if (!bFound) {
        if (reason & 1) why = Text_Get(0x5100032e);
        else if (reason & 4) why = Text_Get(0x5100032f);
        else if (reason & 2) why = Text_Get(0x51000330);
        else if (reason & 8) why = Text_Get(0x51000331);
        else if (reason & 0x1000) why = Text_Get(0x5100032e);
    }
    Mission_StatusString += "\n\n" + why;
    Mission_ResetObjectives(3);
    theApp.SetMissionFailed();
}

// The mission succeeded (player alive). Rituals report the completion time; Mission_State 2 for 5.1 s.
void Mission_Success()
{
    if (Mission_State != 1) return;
    if (0.0f >= XItemHandler_Player::Instance()->GetHealth()) return;
    Text_DeleteMessages(2);
    if (FELevelManager::Instance(1)->IsRitual(gCurrentMap_GeoHash)) {
        float t = Mission_TotalTime;
        Mission_RitualCompleteTime = t;
        EXString time;
        int mins = (int)(t * (1.0f / 60.0f));
        int secs = (int)fmod(t, 60.0);
        int tenths = (int)((t - (float)(short)t) * 10.0f) % 10;
        time.Format(Text_Get(0x51000513), mins, secs, tenths);
        Mission_StatusString = Text_Get(0x51000512);
        FEObjectives::Instance(1)->Add(Mission_StatusString, time, 0x10);
        Mission_StatusString += "\n\n" + time;
    } else {
        Mission_StatusString = Text_Get(FELevelManager::Instance(1)->IsTraining(gCurrentMap_GeoHash) ? 0x51000611 : 0x5100032a);
        FEObjectives::Instance(1)->Add(Mission_StatusString, 0x10);
    }
    Mission_State = 2;
    Mission_StateTimer = 5.1f;
    Mission_ResetObjectives(3);
    theApp.SetMissionComplete();
}

// Per-frame mission bookkeeping: ritual timer, the end-of-mission countdown (unloads the trigger file's
// items), the alarm meter decay, the listening objective, and the Aliens level's coolant messages.
void Mission_Update()
{
    if (FELevelManager::Instance(1)->IsRitual(gCurrentMap_GeoHash)) Mission_AssaultCourseMusic();
    if (theApp.GameStateCurrent()->m_Counter > 5 && !theApp.m_bCutscene && theApp.GameStateCurrent()->m_State == 9 && Mission_State == 1)
        Mission_TotalTime += EXBaseApp::FrameLength();
    if (theApp.GameStateCurrent()->m_State == 9 && Mission_StateTimer > 0.0f) {
        Mission_StateTimer -= EXBaseApp::FrameLength();
        if (0.0f >= Mission_StateTimer) {
            Mission_StateTimer = -99.9f;
            Mission_State = 3;
            if (gTriggerFile_GeoHash) {
                EXDListItem *pHead = theItemEnv.m_pItemLists[0].m_pHead;
                for (XItem *p = pHead ? (XItem *)((char *)pHead - 4) : 0; p; p = (XItem *)p->NextItem())
                    if (p->m_MapHash == gTriggerFile_GeoHash) p->ItemFlags().Set(0x10);
                POI_MissionEnd();
                EXGeoFile::DeLoadGeoFile(gTriggerFile_GeoHash, 0);
                gTriggerFile_GeoHash = 0;
            }
            if (Mission_Return) Mission_Return = 0;
        }
    }
    if (theApp.GameStateCurrent()->m_State == 9) {
        if (Mission_PlayerSeenTimer > 0.0f) {
            Mission_PlayerSeenTimer -= XApp::FrameLength();
            if (0.0f >= Mission_PlayerSeenTimer) {
                Mission_PlayerSeenTimer = 0.0f;
                Mission_AlarmLevelLastAdd = 0.0f;
            }
        }
        if (Mission_AlarmResetTimer > 0.0f) {
            Mission_AlarmResetTimer -= XApp::FrameLength();
        } else if (Mission_AlarmResetSpeed > 0.0f) {
            Mission_AlarmLevel -= XApp::FrameLength() / Mission_AlarmResetSpeed * 100.0f;
            if (0.0f > Mission_AlarmLevel) Mission_AlarmLevel = 0.0f;
        }
    }
    if (ListenState == 1 && ListenID > 0) {
        if (XGetCamera(0)->IsFirstPerson() && XGetCamera(0)->m_TargetID == ListenID)
            ListenState = 1;
        else
            ListenState = 2;
    }
    if (gCurrentMap_GeoHash == 0x10000f2) {
        int n = 0;
        if (FlagChannels[1]) n = 1;
        if (FlagChannels[2]) n++;
        if (FlagChannels[3]) n++;
        if (FlagChannels[4]) n++;
        if (n > HC_AliensFans) {
            HC_AliensFans = n;
            switch (n) {
            case 1:
                Text_PrintObjective(0x510002bf, ex::RGBA(0xffffffff), 4, -1, 0, 0);
                FlagChannels[0x1f] = 1;
                break;
            case 2:
                Text_PrintObjective(0x510002c0, ex::RGBA(0xffffffff), 4, -1, 0, 0);
                FlagChannels[0x20] = 1;
                break;
            case 3:
                Text_PrintObjective(0x510002c1, ex::RGBA(0xffffffff), 4, -1, 0, 0);
                FlagChannels[0x21] = 1;
                break;
            case 4:
                Text_PrintObjective(0x510002c2, ex::RGBA(0xffffffff), 4, -1, 0, 0);
                FlagChannels[0x22] = 1;
                break;
            }
        }
    }
    Prev_Mission_State = Mission_State;
}

// An objective waits (state 3) until every objective linking to it has succeeded, then triggers; while
// active (state 1) it collects m_Flags conditions (time limit, prey escaped, alarm level, flag channel) and
// succeeds when all of m_SuccessMask are set (rewards: flag channel, alarm, light set, stats page) or fails
// the mission (or the bonus) when any of m_FailMask is.
s8 XItemHandler_Objective::DoUpdate()
{
    if (m_bFirstUpdate) {
        m_bFirstUpdate = 0;
        for (int i = 0; i < 8; i++) m_pLinkItems[i] = Mission_GetMissionItem(m_Links[i]);
        Mission_FixupObjectiveReferences((XItem *)m_pItem);
    }
    if (Mission_State != 1) return 1;
    if (0.0f >= XItemHandler_Player::Instance()->GetHealth()) return 1;
    if (m_State == 3) {
        if (!(m_ObjFlags & 0x10)) {
            if (m_RefCount <= 0) return 1;
            int n = 0;
            for (int i = 0; i < m_RefCount; i++)
                if (((XItemHandler_Objective *)m_pRefs[i]->ItemHandler())->m_State == 2) n++;
            if (n != m_RefCount) return 1;
        }
        TriggerObjective();
        return 1;
    }
    if (m_State != 1) {
        if (!(m_ObjFlags & 0x1000) || !m_FlagChannel || FlagChannels[m_FlagChannel]) return 1;
        m_Flags &= ~0x800;
        m_State = 1;
        return 1;
    }
    if (gKillCounts && m_SpawnPoint && m_Profile) EXBaseDisplay::ActiveWnd()->DbgPrint("  Kill Count : %d\n", m_Killed);
    if (theApp.GameStateCurrent()->m_State == 9 && theApp.GameStateCurrent()->m_Counter > 5 && !theApp.m_bCutscene &&
        ((m_FailMask | m_SuccessMask) & 2)) {
        m_Timer += XApp::FrameLength();
        if (!(m_ObjFlags & 1)) {
            float left = (float)m_TimeLimit - m_Timer;
            float zero = 0.0f;
            Mission_TimeLeft = ObjMax(left, zero);
            int mins = (int)(Mission_TimeLeft * (1.0f / 60.0f));
            int secs = (int)fmod(Mission_TimeLeft, 60.0);
            int tenths = (int)((Mission_TimeLeft - (float)(short)Mission_TimeLeft) * 10.0f) % 10;
            sprintf(TimeString, "%02d:%02d.%01d", mins, secs, tenths);
            if (Mission_TimeLeft > 0.0f) {
                if (mins <= 0 && secs <= 10)
                    Text_PrintTimer(EXString(TimeString), ex::RGBA(0xff, 0xff, 0, (u8)(int)((float)tenths * 0.1f * 128.0f)));
                else
                    Text_PrintTimer(EXString(TimeString), ex::RGBA(0x7f, 0x7f, 0, 0x80));
            }
        }
        if (m_Timer > (float)m_TimeLimit) m_Flags |= 2;
    }
    u16 masks = m_SuccessMask | m_FailMask;
    if ((masks & 0x400) && Mission_AlarmLevel >= (float)m_AlarmFame) m_Flags |= 0x400;
    if ((masks & 0x800) && FlagChannels[m_FlagChannel]) m_Flags |= 0x800;
    if (masks & 4) {
        Actor *pActor;
        for (pActor = Actor::GetFirst(0); pActor; pActor = pActor->GetNext(0)) {
            if (!CaresAbout(pActor)) continue;
            EXVector d = pActor->GetItem()->m_position - gpPlayerItem[0]->m_position;
            float distSq = d.LengthXZSquared();
            if ((float)(Mission_EscapeRadius * Mission_EscapeRadius) > distSq) {
                if (m_ObjFlags & 0x800) {
                    float warn = (float)Mission_EscapeRadius * 0.7f;
                    if (distSq > warn * warn) Text_PrintMsg(Text_Get(0x5100017d), ex::RGBA(0xffffffff), 1, 5, 0, 0);
                }
                goto checked;
            }
        }
        m_Flags |= 4;
    }
checked:
    if ((m_SuccessMask && (m_Flags & m_SuccessMask) == m_SuccessMask) || (m_ObjFlags & 8)) {
        m_State = 2;
        if (!((m_FailMask | m_SuccessMask) & 0x800) && m_FlagChannel > 0 && m_FlagChannel < 0x122) FlagChannels[m_FlagChannel] = 1;
        if (!((m_SuccessMask | m_FailMask) & 0x400)) {
            if (m_AlarmFame < -100) Mission_AlarmMeterActive = 0;
            Mission_AlarmLevel = (float)m_AlarmFame + Mission_AlarmLevel;
            float hi = 100.0f, lo = 0.0f;
            Mission_AlarmLevel = ObjMax(ObjMinGe(Mission_AlarmLevel, hi), lo);
            Mission_AlarmResetTimer = Mission_AlarmResetDelay;
            if (m_AlarmFame > 0) AlarmSound();
        }
        if (m_ObjFlags & 0x20) XItemHandler_LightSet::StartLightSetScript();
        if (m_ObjFlags & 0x100) Objective_AddToActiveTimers((float)m_TimeLimit);
        if (m_ObjFlags & 0x200) EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00152);
        HashToCharStr(FELevelManager::Instance(1)->TextFromLevel(m_StatsLevel), 0);
        EXHashCodeFile::Debug_HashCodeName(m_StatsLevel);
        if (m_StatsLevel) {
            if (FlagChannels[0x11d]) {
                Stats_InitPageValues(m_StatsLevel);
                FEMainMenu::DisplayStats(m_StatsLevel, 0, 0, 0);
                theApp.GameStatePush((GameState::Flags)0x20, XApp::Frontend3DFile());
            } else if (FlagChannels[0x11b]) {
                Stats_InitPageValues(m_StatsLevel);
                FEMainMenu::DisplayStats(m_StatsLevel, 0, 0, 1);
                theApp.GameStatePush((GameState::Flags)0x20, m_StatsLevel);
            }
        }
        if (m_CompleteText > 0x51000000) FEObjectives::Instance(1)->MarkComplete(m_CompleteText);
    }
    u16 fail = m_Flags & m_FailMask;
    if (fail) {
        m_State = 0;
        if (m_ObjFlags & 0x40) {
            Mission_BonusFail(fail);
            return 1;
        }
        Mission_Fail(fail);
    }
    return 1;
}
