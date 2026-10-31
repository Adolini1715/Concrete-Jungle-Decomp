#include "ScriptRunner.h"
#include "XItem.h"
#include "EXItemAnimator.h"
#include "EXMemory.h"
#include "EXBaseSoundManager.h"
#include "FEText.h"
#include "GameData.h"
#include "XAppStandIn.h"
#include "XItemHandler_ScriptBase.h"
#include "XItemHandler_Objective.h"
#include "XItemHandler_Player.h"
#include "XItemHandler_Camera.h"
#include "EXItemAnimator_Script.h"
#include "EXGeoFile.h"
#include "EXDatum.h"
#include "EXHashCodeFile.h"
#include "XGamePad.h"
#include "XNavMeshHandler.h"
#include "AIControl.h"
#include "ActorOptions.h"
#include "ActorUtil.h"

// Script event as the animator passes it to DoScriptCmdEvent (only the fields used here).
struct GEOSCRIPT_EVENT {
    u32 m_Unk0[3];
    unsigned int m_Hash;    // 0x0c event hash (HT_ScriptEvents_*), -1 at the end of the script
    u32 m_Unk10;
    union Param {
        unsigned int u;
        float f;
        signed char b;
    } m_Param[4];           // 0x14
};

extern unsigned short Mission_State;



XItemList gDoorTestList;
XItemList gHitableList;

EX_IMPLEMENT_DYNCREATE_ROOT(XItemHandler_ScriptRunner)

void XItemHandler_ScriptRunner::DoWeNeedToBeRemovedFromAnyList(EXItem *pItem)
{
    if ((m_ListFlags & 1) > 0) {
        m_ListFlags &= ~1;
        gDoorTestList.Remove((XItem *)pItem);
    }
    if ((m_ListFlags & 2) > 0) {
        m_ListFlags &= ~2;
        gHitableList.Remove((XItem *)pItem);
    }
}

// NEAR: as Recharge_GetNearest (copy of the difference vector scheduled differently).
XItem *DoorTest_GetNearest(const EXVector &pos, float maxDist)
{
    XItem *p = gDoorTestList.GetFirst();
    XItem *pBest = 0;
    for (; p; p = gDoorTestList.GetNext(p)) {
        XItemHandler_ScriptRunner *pHandler = (XItemHandler_ScriptRunner *)p->ItemHandler();
        if (pHandler && pHandler->m_DoorState == 1) {
            EXVector d = p->Position() - pos;
            float dist = d.LengthXYZ();
            if (maxDist > dist) {
                maxDist = dist;
                pBest = p;
            }
        }
    }
    return pBest;
}

void XItemHandler_ScriptRunner::ApplyDamage(float damage, unsigned int type)
{
    if (type == m_DamageType && m_bHitable && 0.0f >= m_HitTimer) {
        m_Health -= damage;
        m_HitTimer = 6.0f;
    }
}

void XItemHandler_ScriptRunner::DoImpactFX(EXVector &pos)
{
    unsigned int fx[7] = {0x400064e, 0x400064f, 0x4000650, 0x4000651, 0x4000653, 0x4000652, 0x4000692};
    unsigned int snd[7] = {0x1af00594, 0x1af005b2, 0x1af005b4, 0x1af005b1, 0x1af005af, 0x1af005b0, 0x1af005b3};
    XItemHandler_ScriptBase::FX_Create(pos, fx[m_ImpactType], 0x40, 0x1000016);
    EXBaseSoundManager::GetSoundManager()->StartSound(snd[m_ImpactType], pos, -1, 0);
}

unsigned int XItemHandler_ScriptRunner::RestoreState()
{
    return --m_SaveCount ? 0 : 4;
}

unsigned int XItemHandler_ScriptRunner::SaveState()
{
    m_StateMarkers++;
    return 0;
}

XItemHandler_ScriptRunner::~XItemHandler_ScriptRunner()
{
    DoWeNeedToBeRemovedFromAnyList(m_pItem);
}

// NEAR: VC hoists the m_pItem render-info address above the member stores (orig computes it after
// the float stores).
s8 XItemHandler_ScriptRunner::Initialise()
{
    if (!EXItemHandler::Initialise()) return 0;
    m_State = 2;
    m_DoorState = 0;
    m_SaveCount = 0;
    m_ListFlags = 0;
    m_Health = 0.0f;
    m_HitTimer = 0.0f;
    m_HitRadius = 0.0f;
    m_HitCentre[0] = 0.0f;
    m_HitCentre[1] = 0.0f;
    m_HitCentre[2] = 0.0f;
    m_HitCentre[3] = 0.0f;
    m_bHitable = 0;
    m_bMissionDone = 0;
    m_StateMarkers = 0;
    m_bFindMissionItem = 1;
    m_NextScript = 0;
    m_pItem->RenderInfo().SetExtRenderInfo(&m_ExtRenderInfo);
    m_pItem->RenderInfo().SetTexSwapInfo(&m_TexSwapInfo);
    return 1;
}

// Runs the placement's script while the camera is within m_Range of it (and its zone is visible), stops
// it when it leaves, swaps scripts on SR_SwitchScript and removes the item when the script ends.
s8 XItemHandler_ScriptRunner::DoUpdate()
{
    if (m_bFindMissionItem) {
        m_bFindMissionItem = 0;
        m_pMissionItem = Mission_GetMissionItem(m_MissionId);
        if (m_pMissionItem) ((XItemHandler_Objective *)m_pMissionItem->ItemHandler())->m_DestroyCount++;
    }
    if (m_bMissionDone && Mission_State == 3) m_pItem->ItemFlags().Set(0x10);
    if (m_NextScript) {
        m_pItem->DeleteAnimator(m_pItem->GetAnimator(m_ScriptHash));
        m_ScriptHash = m_NextScript;
        m_NextScript = 0;
        EXGeoFile *pGeo = EXGeoFile::LoadGeoFile(m_GeoHash, 0, 0);
        EXItemAnimator_Script *pAnim = new EXItemAnimator_Script;
        pAnim->Initialise(pGeo, m_ScriptHash);
        m_pItem->AddAnimator(pAnim);
        m_StateMarkers = 0;
        m_SaveCount = 0;
        pAnim->m_itemFlags.BitSet(1);
        pAnim->m_validMtx = 0;
        return 1;
    }
    if (m_DeleteChannel && FlagChannels[m_DeleteChannel]) m_pItem->ItemFlags().Set(0x10);
    if (m_HitTimer > 0.0f) m_HitTimer -= XApp::GameFrameRateMul();
    switch (m_State) {
    case 3:
        m_State = 4;
        m_pItem->DeleteAnimator(m_pItem->GetAnimator(m_ScriptHash));
        if (m_Flags) m_pItem->UpdateGroups(&theItemEnv, 0);
        DoWeNeedToBeRemovedFromAnyList(m_pItem);
        return 1;
    case 2:
        if (!(m_Flags2 & 2) && !theItemEnv.IsZoneVisible(m_pItem->MapOn())) break;
        if (m_Range.f != 0.0f) {
            EXVector cam = XGetCamera(0)->m_CameraPos; EXVector d = m_pItem->Position() - cam;
            if (EXSqrt(d.x * d.x + d.y * d.y + d.z * d.z) > m_Range.f) break;
        }
        {
            m_State = 1;
            EXGeoFile *pGeo = EXGeoFile::LoadGeoFile(m_GeoHash, 0, 0);
            EXItemAnimator_Script *pAnim = EXItemAnimator_Script::NewItem();
            pAnim->Initialise(pGeo, m_ScriptHash);
            m_pItem->AddAnimator(pAnim);
        }
        if (m_Flags) m_pItem->UpdateGroups(&theItemEnv, 3);
        if (m_Flags & 2) {
            gDoorTestList.Add((XItem *)m_pItem);
            m_ListFlags |= 1;
            m_DoorState = 1;
        }
        if (m_Flags & 4) {
            gHitableList.Add((XItem *)m_pItem);
            m_ListFlags |= 2;
        }
        break;
    case 1:
        if ((m_Flags2 & 2) || theItemEnv.IsZoneVisible(m_pItem->MapOn())) {
            if (m_Range.f == 0.0f) break;
            EXVector cam = XGetCamera(0)->m_CameraPos; EXVector d = m_pItem->Position() - cam;
            if (!(EXSqrt(d.x * d.x + d.y * d.y + d.z * d.z) > m_Range.f)) break;
        }
        m_State = 2;
        m_pItem->DeleteAnimator(m_pItem->GetAnimator(m_ScriptHash));
        DoWeNeedToBeRemovedFromAnyList(m_pItem);
        if (m_Flags & 1) m_pItem->UpdateGroups(&theItemEnv, 0);
        break;
    }
    return 1;
}

// The SR_ script commands. Return values go back to the script animator: 0 carry on, 1 wait here
// (the command repeats next frame), 2 jump, 3 take the alternate branch, 4 end of the alternate branch.
u32 XItemHandler_ScriptRunner::DoScriptCmdEvent(GEOSCRIPT_EVENT *pEvent, EXItemAnimator *pAnim)
{
    XItemHandler_Player *pPlayer;
    int ch;
    unsigned int hash = pEvent->m_Hash;
    switch (hash) {
    case 0x1600002a:    // SR_StateMarker
        if (m_SaveCount) return RestoreState();
        return SaveState();
    case 0x1600002b:    // SR_SetAlternateState
        if (m_StateMarkers) {
            m_SaveCount = m_StateMarkers;
            return 3;
        }
        break;
    case 0x1600002c:    // SR_Wait
        return 1;
    case 0x1600002f:    // SR_DoorTestStart
        if (m_SaveCount) break;
        pPlayer = (XItemHandler_Player *)gpPlayerItem[0]->m_pItemHandler;
        if (pPlayer->m_Mode != 0x13 || pPlayer->m_pSpecialItem != m_pItem) return 1;
        if (pPlayer->m_SubMode == 2) break;
        if (pPlayer->m_SubMode == 3) {
            DoWeNeedToBeRemovedFromAnyList(m_pItem);
            m_SaveCount = 1;
            return 3;
        }
        return 1;
    case 0x1600002e:    // SR_DoorTestLoop
        if (m_SaveCount) {
            DoWeNeedToBeRemovedFromAnyList(m_pItem);
            break;
        }
        pPlayer = (XItemHandler_Player *)gpPlayerItem[0]->m_pItemHandler;
        if (pPlayer->m_Mode != 0x13 || pPlayer->m_pSpecialItem != m_pItem) return 1;
        if (pPlayer->m_SubMode == 3) {
            m_SaveCount = 1;
            DoWeNeedToBeRemovedFromAnyList(m_pItem);
            return 3;
        }
        if (pPlayer->m_SubMode != 2) return 1;
        return 2;
    case 0x16000031:    // SR_DoorTest
        if (m_SaveCount) break;
        pPlayer = (XItemHandler_Player *)gpPlayerItem[0]->m_pItemHandler;
        if (pPlayer->m_Mode != 0x13 || pPlayer->m_pSpecialItem != m_pItem || pPlayer->m_SubMode != 3) break;
        DoWeNeedToBeRemovedFromAnyList(m_pItem);
        m_SaveCount = 1;
        return 3;
    case 0x16000032:    // SR_WaitForHit: wait until the health runs out
        m_bHitable = 1;
        if (m_SaveCount) break;
        if (!(0.0f > m_Health)) return 1;
        if (!m_pMissionItem) break;
        goto destroyed;
    case 0x16000033: {  // SR_SetFlags: health, and the hit sphere from the HitCollision datum
        if (m_SaveCount) break;
        m_Health = pEvent->m_Param[0].f;
        EXDatumTemp d;
        if (m_pItem->GetAnimator()->GetDatum(0x10000003, d, 0, EXItemAnimator::m_pNullDatumPtr)) {
            m_HitRadius = d.Radius();
            EXVector c = m_pItem->Position() + d.m_position;
            m_HitCentre[0] = c.x;
            m_HitCentre[1] = c.y;
            m_HitCentre[2] = c.z;
            m_HitCentre[3] = c.w;
        } else {
            m_HitRadius = 0.5f;
            m_HitCentre[0] = m_pItem->Position().x;
            m_HitCentre[1] = m_pItem->Position().y;
            m_HitCentre[2] = m_pItem->Position().z;
            m_HitCentre[3] = m_pItem->Position().w;
        }
        break;
    }
    case 0x16000037:    // SR_WaitForFlagChannel
        if (m_SaveCount) break;
        ch = (int)pEvent->m_Param[0].f;
        if (ch >= 0 && ch < 0x122 && FlagChannels[ch]) break;
        return 1;
    case 0x16000038:    // SR_SetFlagChannel
        if (m_SaveCount) break;
        ch = (int)pEvent->m_Param[0].f;
        if (ch >= 0 && ch < 0x122) FlagChannels[ch] = 1;
        break;
    case 0x1600003e:    // SR_WaitForMission
        if (m_SaveCount) break;
        if (!m_pMissionItem || ((XItemHandler_Objective *)m_pMissionItem->m_pItemHandler)->m_State != 1) return 1;
        m_bMissionDone = 1;
        break;
    case 0x1600005e:    // Rumble
        if (m_SaveCount) break;
        ((XGamePad *)EXBaseGamePad::GetGamePad())->PlayRumble(pEvent->m_Param[0].u, pEvent->m_Param[1].f, 0xff);
        break;
    case 0x16000056:    // SR_ClearFlagChannel
        if (m_SaveCount) break;
        ch = (int)pEvent->m_Param[0].f;
        if (ch >= 0 && ch < 0x122) FlagChannels[ch] = 0;
        break;
    case 0x16000046: {  // SR_Explosion
        if (m_SaveCount) break;
        u8 f = (u8)pEvent->m_Param[3].b;
        XItemHandler_ScriptBase::Explosion_Create(m_pItem->m_position, pEvent->m_Param[0].u, (int)pEvent->m_Param[1].f,
                                                  (int)pEvent->m_Param[2].f, 0, 0, (s8)(f & 2), 0, (s8)(f & 1));
        break;
    }
    case 0x16000050:    // SR_SwitchScript (parameter 1: only while this flag channel is set)
        ch = (int)pEvent->m_Param[1].f;
        if (ch && !FlagChannels[ch]) break;
        m_NextScript = pEvent->m_Param[0].u;
        break;
    case 0x16000052:    // SR_DoorTestState
        m_DoorState = pEvent->m_Param[0].u;
        break;
    case 0x1600005a:    // LoudNoise
        if (m_SaveCount) break;
        SendMsg_LoudNoise(m_pItem->m_position, pEvent->m_Param[0].f, 0, 1, 0);
        break;
    case 0x1600005f: {  // Rumble3D
        if (m_SaveCount) break;
        EXVector pos = pAnim->GetOwnerItem()->m_position;
        ((XGamePad *)EXBaseGamePad::GetGamePad())->PlayRumble3D(pEvent->m_Param[0].u, pos, pEvent->m_Param[2].f, pEvent->m_Param[3].f,
                                                              pEvent->m_Param[1].f, 0xff);
        break;
    }
    case 0x16000063:    // wait until the health drops below the parameter
        m_bHitable = 1;
        if (m_SaveCount) break;
        if (!(pEvent->m_Param[0].f > m_Health)) return 1;
        if (!m_pMissionItem || !(0.0f > m_Health)) break;
    destroyed:
        ((XItemHandler_Objective *)m_pMissionItem->m_pItemHandler)->ProcessObjectDestroyed();
        m_pMissionItem = 0;
        break;
    case 0x16000060:    // SR_Cloak
        m_pItem->RenderInfo().EnableRefractionRender(pEvent->m_Param[0].b);
        break;
    case 0x1600006c:    // block the nav mesh here
        gNavMeshHandler.SetBlocked(m_pItem->m_position);
        gAIControl.m_NavCacheSync++;
        break;
    case 0x1600006d:    // open the nav mesh here
        gNavMeshHandler.SetOpen(m_pItem->m_position);
        gAIControl.m_NavCacheSync++;
        break;
    case 0xffffffff:    // end of script
        if (pAnim == m_pItem->GetAnimator()) m_State = 3;
        break;
    default:
        EXHashCodeFile::Debug_HashCodeName(hash);
        break;
    }
    return 0;
}
