#include "Recharge.h"
#include "XItem.h"
#include "EXItemAnimator.h"
#include "EXMemory.h"
#include "EXArray.h"
#include "FEText.h"
#include "GameData.h"
#include "XAppStandIn.h"

// TODO stand-ins: other areas.
class EXItemAnimator_Script : public EXItemAnimator {
public:
    EXItemAnimator_Script();
    u8 m_Unk104[0x148 - 0x104];  // TODO layout
};
extern EXFixedArray<unsigned char, 290> FlagChannels;

XItemList gRechargeList;
XItemList gBeaconList;

EX_IMPLEMENT_DYNCREATE_ROOT(XItemHandler_RechargePoint)

XItemHandler_RechargePoint::~XItemHandler_RechargePoint() { gRechargeList.Remove((XItem *)m_pItem); }

void XItemHandler_RechargePoint::StartRecharge()
{
    if (m_State == 2) return;
    m_pItem->DeleteAnimator(m_pItem->GetAnimator());
    EXItemAnimator_Script *pScript = new EXItemAnimator_Script;
    pScript->Initialise(m_pItem->GeoFile(), m_StartScript);
    m_pItem->AddAnimator(pScript);
    m_State = 1;
}

// NEAR: copy of the difference vector is scheduled differently (w store first).
XItem *Recharge_GetNearest(const EXVector &pos)
{
    float best = 999999.9f;
    XItem *p = gRechargeList.GetFirst();
    XItem *pBest = 0;
    for (; p; p = gRechargeList.GetNext(p)) {
        if (((XItemHandler_RechargePoint *)p->ItemHandler())->m_State == 0) {
            EXVector d = p->Position() - pos;
            float dist = d.LengthXYZSquared();
            if (best > dist) {
                best = dist;
                pBest = p;
            }
        }
    }
    return pBest;
}

EX_IMPLEMENT_DYNCREATE_ROOT(XItemHandler_Beacon)

XItemHandler_Beacon::~XItemHandler_Beacon() { gBeaconList.Remove((XItem *)m_pItem); }

// NEAR: as Recharge_GetNearest.
XItem *Beacon_GetNearest(const EXVector &pos)
{
    float best = 999999.9f;
    XItem *p = gBeaconList.GetFirst();
    XItem *pBest = 0;
    for (; p; p = gBeaconList.GetNext(p)) {
        XItemHandler_Beacon *pBeacon = (XItemHandler_Beacon *)p->ItemHandler();
        if (!pBeacon->m_bActivated && !pBeacon->m_ShowFlagChannel) {
            EXVector d = p->Position() - pos;
            float dist = d.LengthXYZSquared();
            if (best > dist) {
                best = dist;
                pBest = p;
            }
        }
    }
    return pBest;
}

s8 XItemHandler_RechargePoint::Initialise()
{
    if (!EXItemHandler::Initialise()) return 0;
    gRechargeList.Add((XItem *)m_pItem);
    m_pItem->RenderInfo().SetExtRenderInfo(&m_ExtRenderInfo);
    m_pItem->RenderInfo().SetTexSwapInfo(&m_TexSwapInfo);
    m_bRecharged = 0;
    return 1;
}

void XItemHandler_RechargePoint::EndRecharge()
{
    if (m_State == 2) return;
    unsigned int script;
    if (m_EmptyScript > 0x4000000) {
        script = m_EmptyScript;
        m_State = 2;
        if (m_EmptyFlagChannel) FlagChannels[m_EmptyFlagChannel] = 1;
    } else {
        script = m_IdleScript;
        m_State = 0;
    }
    m_pItem->DeleteAnimator(m_pItem->GetAnimator());
    EXItemAnimator_Script *pScript = new EXItemAnimator_Script;
    pScript->Initialise(m_pItem->GeoFile(), script);
    m_pItem->AddAnimator(pScript);
}

s8 XItemHandler_RechargePoint::DoUpdate()
{
    if (m_State != 2 && m_State == 1 && !m_bRecharged) {
        m_bRecharged = 1;
        XItemHandler_Player *pPlayer = XItemHandler_Player::Instance();
        pPlayer->m_Energy = pPlayer->GetMaxEnergy();
        GameData::Instance(1)->Stats()->m_Recharges++;
    }
    return 1;
}

s8 XItemHandler_Beacon::Initialise()
{
    if (!EXItemHandler::Initialise()) return 0;
    gBeaconList.Add((XItem *)m_pItem);
    m_pItem->RenderInfo().SetExtRenderInfo(&m_ExtRenderInfo);
    m_pItem->RenderInfo().SetTexSwapInfo(&m_TexSwapInfo);
    m_bActivated = 0;
    return 1;
}

void XItemHandler_Beacon::Activate()
{
    if (m_bActivated) return;
    m_pItem->DeleteAnimator(m_pItem->GetAnimator());
    EXItemAnimator_Script *pScript = new EXItemAnimator_Script;
    pScript->Initialise(m_pItem->GeoFile(), m_ActivateScript);
    m_pItem->AddAnimator(pScript);
    m_pItem->RenderInfo().EnableRefractionRender(0);
    m_pItem->RenderInfo().RenderFlagsRef().BitClr(8);
    FlagChannels[m_ActivateFlagChannel] = 1;
    m_bActivated = 1;
}

s8 XItemHandler_Beacon::DoUpdate()
{
    if (m_ShowFlagChannel && FlagChannels[m_ShowFlagChannel]) {
        m_ShowFlagChannel = 0;
        m_pItem->SetHasRender();
        m_pItem->UpdateGroups(&theItemEnv, 2);
    }
    return 1;
}
