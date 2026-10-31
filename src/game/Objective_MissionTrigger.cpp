#include "XItemHandler_MissionTrigger.h"
#include "XItemHandler_Objective.h"
#include "EXTrigger.h"
#include "EXGeoTrigger.h"
#include "EXGeoFile.h"
#include "EXGeoHeader.h"
#include "EXItemAnimator.h"
#include "XItem.h"
#include "EXMemory.h"
#include "FEText.h"
#include "XApp.h"
#include "Actor.h"
#include "XItemHandler_Player.h"
#include "ActorOptions.h"
#include "XGameWnd.h"
#include "EXCamera.h"
#include "XItemHandler_Camera.h"
#include "EXDisplay.h"

extern unsigned short Mission_State;
extern signed char Mission_AlarmMeterActive;
extern signed char Mission_AlarmMeterHidden;
extern float Mission_AlarmLevel;
extern float Mission_PlayerSeenTimer;
extern int Mission_PlayerSeenInterval;
extern int Mission_EscapeRadius;
extern float Mission_AlarmResetDelay;
extern float Mission_AlarmResetSpeed;
extern float Mission_AlarmResetTimer;
extern float Mission_AlarmLevelLastAdd;
extern signed char gTriggerCollision;
extern XGameWnd *gpGameWnd[];
extern unsigned int gSystemCameraUser;
signed char TrafficMissionInit(unsigned int mission);

extern unsigned int gCurrentMap_GeoHash;
extern unsigned int gTriggerFile_GeoHash;
unsigned int TriggerInit(EXTrigger *pTrigger, EXGeoTrigger *pGeo, EXGeoMap *pMap, EXMapOn mapOn, unsigned short index);

EX_IMPLEMENT_DYNCREATE_ROOT(XItemHandler_MissionTrigger)

XItemHandler_MissionTrigger::~XItemHandler_MissionTrigger() { gTrigList.Remove((XItem *)m_pItem); }

s8 XItemHandler_MissionTrigger::Initialise()
{
    if (!EXItemHandler::Initialise()) return 0;
    gTrigList.Add((XItem *)m_pItem);
    m_bFirstUpdate = 1;
    return 1;
}

void XItemHandler_MissionTrigger::LoadTriggerFile()
{
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(m_TriggerFile, 0, 0);
    gTriggerFile_GeoHash = m_TriggerFile;
    unsigned int saved = gCurrentMap_GeoHash;
    gCurrentMap_GeoHash = m_TriggerFile;
    EXItemAnimator *pMap = EXItemAnimator::CreateFrom(pFile, pFile->GeoHeader()->GeoObjectHashIndex(0x85000000));
    theItemEnv.InitTriggers((EXItemAnimator_Map *)pMap, TriggerInit);
    if (pMap) delete pMap;
    gCurrentMap_GeoHash = saved;
}

signed char IsTouchplate(EXItem *pItem)
{
    EXItemHandler *pHandler = pItem->ItemHandler();
    if (pHandler && pHandler->IsKindOf(EXRUNTIME_CLASS(XItemHandler_MissionTrigger))) return 1;
    return 0;
}

signed char IsValid(EXGeoTriggerDatum d) { return d.m_06 <= 2; }

s8 XItemHandler_MissionTrigger::DoUpdate()
{
    if (m_bFirstUpdate) {
        m_bFirstUpdate = 0;
        for (int i = 0; i < 8; i++) m_pLinks[i] = Mission_GetMissionItem(m_Links[i]);
    }
    switch (m_Type) {
    case 0:
        UpdateMissionTrigger();
        break;
    case 1:
        UpdateDestination();
        break;
    case 2:
        UpdateExit();
        break;
    }
    return 1;
}

// Point in trigger volume: 0 box (half sizes in m_08), 1 sphere (radius m_08.x), 2 vertical cylinder
// (half height m_08.x, radius m_08.y).
signed char Within(EXVector p, EXGeoTriggerDatum d)
{
    switch (d.m_06) {
    case 0:
        if (d.m_08.x < (float)fabs(p.x - d.m_Pos.x)) return 0;
        if (d.m_08.y < (float)fabs(p.y - d.m_Pos.y)) return 0;
        if (d.m_08.z < (float)fabs(p.z - d.m_Pos.z)) return 0;
        return 1;
    case 1: {
        EXVector v(p.x - d.m_Pos.x, p.y - d.m_Pos.y, p.z - d.m_Pos.z, p.w);
        if (d.m_08.x > v.LengthXYZ()) return 1;
        return 0;
    }
    case 2: {
        float dy = p.y - d.m_Pos.y;
        if (d.m_08.x < (float)fabs(dy)) return 0;
        EXVector v(p.x - d.m_Pos.x, dy, p.z - d.m_Pos.z, p.w);
        if (d.m_08.y > v.LengthXZ()) return 1;
        return 0;
    }
    }
    return 0;
}

// Destination: while its objective is active, sets the objective's 0x20 flag when the player is inside
// and 0x10 when one of the objective's actors is (with option 0x80 only while carrying such an actor).
void XItemHandler_MissionTrigger::UpdateDestination()
{
    XItemHandler_Objective *pObj;
    if (Mission_State == 1) {
        if (!m_pLinks[0]) return;
        pObj = (XItemHandler_Objective *)m_pLinks[0]->ItemHandler();
        if (pObj->m_State == 1) goto active;
    }
    if (m_pItem->ItemFlags().m_flags & 1) {
        m_pItem->ItemFlags().BitClr(1);
        m_pItem->UpdateGroups(&theItemEnv, 0);
    }
    return;
active:
    Draw3DText();
    if (m_Options & 0x80) {
        Actor *pHeld;
        if (!XItemHandler_Player::Instance()->IsHoldingActor() ||
            !(pHeld = Actor::GetPtrFromID(XItemHandler_Player::Instance()->m_SpecialActorID)) || !pObj->CaresAbout(pHeld)) {
            if (m_pItem->ItemFlags().m_flags & 1) {
                m_pItem->ItemFlags().BitClr(1);
                if (m_Options & 8) m_pItem->UpdateGroups(&theItemEnv, 0);
            }
            return;
        }
    }
    if (!(m_pItem->ItemFlags().m_flags & 1)) {
        m_pItem->ItemFlags().BitSet(1);
        m_pItem->m_validMtx = 0;
        if (m_Options & 8) m_pItem->UpdateGroups(&theItemEnv, 2);
    }
    if ((pObj->m_SuccessMask | pObj->m_FailMask) & 0x20) {
        EXVector p = gpPlayerItem[0]->Position();
        signed char bIn;
        if (IsValid(*(EXGeoTriggerDatum *)&m_Volume)) {
            bIn = Within(p, *(EXGeoTriggerDatum *)&m_Volume);
        } else {
            EXVector d = m_pItem->Position() - p;
            if ((float)m_Radius > d.LengthXYZ()) goto inside;
            bIn = m_bInside;
        }
        if (bIn)
        inside:
            pObj->m_Flags |= 0x20;
    }
    if ((pObj->m_SuccessMask | pObj->m_FailMask) & 0x10) {
        for (Actor *pActor = Actor::GetFirst(1); pActor; pActor = pActor->GetNext(1)) {
            if ((m_Options & 0x400) && Actor::GetPtrFromID(XItemHandler_Player::Instance()->m_SpecialActorID) == pActor) continue;
            if (0.0f >= pActor->m_Health) continue;
            if (pActor->m_CreateID != pObj->m_Self) continue;
            if (IsValid(*(EXGeoTriggerDatum *)&m_Volume)) {
                if (Within(pActor->GetItem()->Position(), *(EXGeoTriggerDatum *)&m_Volume)) pObj->m_Flags |= 0x10;
            } else {
                EXVector d = m_pItem->Position() - pActor->GetItem()->Position();
                if ((float)m_Radius > d.LengthXYZ()) pObj->m_Flags |= 0x10;
            }
        }
    }
}

void Debug_DrawCollShapes(EXGeoTriggerDatum d) { Within(gpPlayerItem[0]->Position(), d); }

// Mission start trigger: when the player enters (or at once with option 2) the mission starts: alarm
// settings, flag channels cleared, start text, the mission's trigger file loaded, linked objectives
// triggered. The trigger then deletes itself (option 4: hides).
void XItemHandler_MissionTrigger::UpdateMissionTrigger()
{
    if (theApp.m_bCutscene) return;
    if (!(m_Options & 2) && ((m_Options & 0x40) ? Mission_State != 1 : Mission_State != 3)) {
        if (m_pItem->ItemFlags().m_flags & 1) {
            m_pItem->ItemFlags().BitClr(1);
            m_pItem->UpdateGroups(&theItemEnv, 0);
        }
        return;
    }
    if (!(m_pItem->ItemFlags().m_flags & 1)) {
        m_pItem->ItemFlags().BitSet(1);
        m_pItem->m_validMtx = 0;
        if (m_Options & 8) m_pItem->UpdateGroups(&theItemEnv, 2);
    }
    EXVector p = gpPlayerItem[0]->Position();
    Draw3DText();
    if (gTriggerCollision && IsValid(*(EXGeoTriggerDatum *)&m_Volume)) Debug_DrawCollShapes(*(EXGeoTriggerDatum *)&m_Volume);
    if (IsValid(*(EXGeoTriggerDatum *)&m_Volume)) {
        if (Within(p, *(EXGeoTriggerDatum *)&m_Volume)) m_bInside = 1;
    } else {
        EXVector d = m_pItem->Position() - p;
        if ((float)m_Radius > d.LengthXYZ()) m_bInside = 1;
    }
    if (!(m_Options & 2) && !m_bInside) return;
    Mission_State = 1;
    Mission_AlarmMeterActive = (m_Options >> 5) & 1;
    memset(&FlagChannels, 0, sizeof(FlagChannels));
    Mission_AlarmMeterHidden = (m_Options >> 9) & 1;
    Mission_AlarmLevel = 0.0f;
    Mission_PlayerSeenTimer = 0.0f;
    Mission_PlayerSeenInterval = m_PlayerSeenInterval;
    Mission_EscapeRadius = m_EscapeRadius;
    Mission_AlarmResetDelay = m_AlarmResetDelay.f;
    Mission_AlarmResetSpeed = m_AlarmResetSpeed.f;
    Mission_AlarmResetTimer = 0.0f;
    Mission_AlarmLevelLastAdd = 0.0f;
    if (m_MissionHash == 0x45000006)
        Text_PrintObjective(0x51000034, ex::RGBA(0xffffffff), 4, -1, 0, 0);
    else if (m_Text2)
        Text_PrintObjective(m_Text2, ex::RGBA(0xffffffff), 4, -1, 0, 0);
    if (m_TriggerFile) LoadTriggerFile();
    TrafficMissionInit(m_MissionHash);
    for (int i = 0; i < 8; i++)
        if (m_pLinks[i]) ((XItemHandler_Objective *)m_pLinks[i]->ItemHandler())->TriggerObjective();
    if (m_Options & 4) {
        m_pItem->ItemFlags().BitClr(1);
        m_pItem->UpdateGroups(&theItemEnv, 0);
        m_bInside = 0;
        return;
    }
    m_pItem->ItemFlags().Set(0x10);
}

// Training exit: entering it asks whether to quit training.
void XItemHandler_MissionTrigger::UpdateExit()
{
    EXVector p = gpPlayerItem[0]->Position();
    Draw3DText();
    signed char bIn;
    if (IsValid(*(EXGeoTriggerDatum *)&m_Volume)) {
        bIn = Within(p, *(EXGeoTriggerDatum *)&m_Volume);
    } else {
        EXVector d = m_pItem->Position() - p;
        bIn = (float)m_Radius > d.LengthXYZ();
    }
    if (!bIn) {
        m_bInside = 0;
        return;
    }
    if (m_bInside) return;
    theApp.m_bSaveFlag = 1;
    theApp.CreateDialogYesNo(Text_Get(0x51000614), 0, 0x11f);
    m_bInside = 1;
}

// Floating label above the trigger while the player is within 10 m; fades in over the last two metres.
template <class T> inline const T &Draw3DMin(const T &a, const T &b) { return a < b ? a : b; }

void XItemHandler_MissionTrigger::Draw3DText()
{
    if (!m_Text) return;
    XItem *pPlayer = gpPlayerItem[0];
    EXItem *pItem = m_pItem;
    EXVector d = pItem->Position() - pPlayer->Position();
    float dist = d.LengthXYZ();
    if (dist >= 10.0f) return;
    EXVector pos = pItem->Position();
    pos.y += 3.0f;
    EXVector pos3 = pos;
    if (gSystemCameraUser != 1) {
        EXBaseCamera *pCam = (EXBaseCamera *)&gpGameWnd[0]->m_Camera;
        pCam->SetClipSize(640, 640, 0);
        ((EXBaseCamera *)&gpGameWnd[0]->m_Camera)->SetOrigin(((EXBaseCamera *)&gpGameWnd[0]->m_Camera)->Origin());
        XItemHandler_Camera *pHandler = XGetCamera(0);
        EXRectf r((float)gpGameWnd[0]->PrimRect().x, (float)gpGameWnd[0]->PrimRect().y, (float)gpGameWnd[0]->PrimRect().w, (float)gpGameWnd[0]->PrimRect().h);
        ((EXCommonCamera *)&gpGameWnd[0]->m_Camera)->SetVFOV(r, 0.7853982f / pHandler->Zoom());
        ((EXBaseCamera *)&gpGameWnd[0]->m_Camera)->UpdateDisplay(0);
        gSystemCameraUser = 1;
    }
    float fade = 10.0f - dist;
    float cap = 2.0f;
    float a = Draw3DMin(fade, cap) * 127.0f;
    ex::RGBA col;
    if (m_Type >= 0) {
        if (m_Type < 2) col = ex::RGBA(0x80, 0x60, 0x60, (u8)(int)a);
        else if (m_Type == 2) col = ex::RGBA(0x40, 0x40, 0x80, (u8)(int)a);
    }
    EXVector2 s = EXBaseDisplay::WorldToDisp(pos3);
    s.x -= ((EXBaseCamera *)&gpGameWnd[0]->m_Camera)->Origin().x;
    s.y -= ((EXBaseCamera *)&gpGameWnd[0]->m_Camera)->Origin().y;
    Text_Print((short)s.x, (short)s.y, gText.Get(m_Text), col, 0x700000c, (EXW::TextAlign)4, 1, 5, 7, 0);
}
