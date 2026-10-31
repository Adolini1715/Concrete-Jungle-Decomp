#include "XItemHandler_Turret.h"
#include "XItemHandler_Player.h"
#include "XItem.h"
#include "Actor.h"
#include "ActorProfile.h"
#include "ScanInfo.h"
#include "TracerFire.h"
#include "EXItemAnimator_Script.h"
#include "EXGeoFile.h"
#include "EXDatum.h"
#include "EXRand.h"
#include "EXMemory.h"
#include "FEText.h"
#include <math.h>

float Vec_AngleDifference(float a, float b);

XItemList gTurretList;

EX_IMPLEMENT_DYNCREATE_ROOT(XItemHandler_Turret)

XItemHandler_Turret::~XItemHandler_Turret()
{
    gTurretList.Remove((XItem *)m_pItem);
    for (int i = 0; i < 8; i++) {
        if (m_pTracers[i]) m_pTracers[i]->Reset();
    }
}

signed char XItemHandler_Turret::ScanCallback(signed char b, FEObject *pObj, ScanInfo *pInfo)
{
    XItemHandler_Turret *pTurret = (XItemHandler_Turret *)pInfo->m_pItem->ItemHandler();
    pInfo->m_a4 = pTurret->m_State == 3 ? 0.0f : 1.0f;
    if (pTurret->m_State != 3) pInfo->m_text.Format("Active");
    else pInfo->m_text.Format("Disabled");
    return 1;
}

void Draw_TurretViewcones() {}

s8 XItemHandler_Turret::Initialise()
{
    if (!EXItemHandler::Initialise()) return 0;
    gTurretList.Add((XItem *)m_pItem);
    m_Unk2c = 0;
    for (int i = 0; i < 8; i++) m_pTracers[i] = 0;
    m_Unk30 = 0.0f;
    m_Unk34 = 0;
    m_Unk130 = 0.0f;
    m_pItem->RenderInfo().SetTexSwapInfo(&m_TexSwapInfo);
    m_pItem->RenderInfo().SetExtRenderInfo(&m_ExtRenderInfo);
    m_Unk35 = 0;
    m_pMuzzleScript = 0;
    return 1;
}

signed char XItemHandler_Turret::CanSeePoint(EXVector &p)
{
    EXVector d = p - m_pItem->Position();
    if (d.LengthXYZ() > m_Range) return 0;
    if (!((float)fabs(Vec_AngleDifference((float)atan2(d.x, d.z), m_pItem->Rotation().y)) < m_HAngle)) return 0;
    EXMaths::vec_xyzw v = d;
    float h = EXSqrt(v.x * v.x + v.z * v.z);
    if ((float)fabs(Vec_AngleDifference(-(float)atan2(d.y, h), m_pItem->Rotation().x)) < m_VAngle) return 1;
    return 0;
}

void XItemHandler_Turret::DeleteMuzzleScript()
{
    if (m_pMuzzleScript) {
        m_pMuzzleScript->ItemFlags().m_flags |= 0x10;
        m_pMuzzleScript = 0;
    }
}

// NEAR: 2 bytes differ, retail reads the option byte with movzx and stores m_Text[1][0] later
signed char XItemHandler_Turret::GetScanInfo(int type, ScanInfo *pInfo)
{
    switch (type) {
    case 0:
        pInfo->m_a0 = 0x40005e9;
        pInfo->m_Text[0][0] = 0x5100053d;
        pInfo->m_Text[1][0] = 0x510007d3;
        break;
    case 2:
        pInfo->m_a0 = 0x4000794;
        pInfo->m_Text[0][0] = 0x5100053b;
        pInfo->m_Text[0][1] = ((m_Options >> 2) & 1) | 0x5100053e;
        pInfo->m_Text[1][0] = 0x510000e7;
        pInfo->m_Text[0][2] = m_bOption0 ? 0x51000909 : 0x5100090a;
        break;
    default:
        pInfo->m_a0 = 0;
        break;
    }
    pInfo->m_a4 = m_State == 3 ? 0.0f : 1.0f;
    s8 r = pInfo->m_a0 > 0;
    pInfo->m_ac = r;
    pInfo->m_pCallback = ScanCallback;
    return r;
}

int XItemHandler_Turret::CanSeePredator()
{
    if (!m_bCloakHides || !XItemHandler_Player::Instance()->GetCloak()) {
        EXVector p = gpPlayerItem[0]->Position();
        if (CanSeePoint(p)) return 1;
        p.y += 1.0f;
        if (CanSeePoint(p)) return 1;
        p = XItemHandler_Player::Instance()->GetTopHeadPos();
        if (CanSeePoint(p)) return 1;
    }
    if (m_TargetSubGroup) {
        for (Actor *a = Actor::GetFirst(m_pItem->m_position, m_Range, 0); a;
             a = a->GetNext(m_pItem->m_position, m_Range, 0)) {
            if (a->GetProfile()->m_SubGroup == m_TargetSubGroup && !(0.0f >= a->m_Health)) {
                EXVector c = a->GetChestPosApprox();
                if (CanSeePoint(c)) return a->m_RefID;
            }
        }
    }
    return 0;
}

void XItemHandler_Turret::PlayMuzzleScript()
{
    DeleteMuzzleScript();
    m_pMuzzleScript = new EXItemAnimator_Script;
    if (!m_pMuzzleScript) return;
    EXGeoFile *pFile = EXGeoFile::GetGeoFile(0x1000016);
    m_pMuzzleScript->Initialise(pFile, 0x4000179);
    ex::EXFlag<u8> &f = m_pMuzzleScript->RenderFlags();
    f.Set(f.m_flags | 3);
    float s = g_EXRandClass.Randf() * 2.0f + 1.0f;
    m_pMuzzleScript->SetScale(EXVector(s, s, s, 0.0f));
    m_pItem->AddAnimator(m_pMuzzleScript);
    EXDatumTemp d;
    if (m_pItem->GetAnimatorDatum(0x100000ea, d, 4, 0)) {
        m_pMuzzleScript->SetPosition(d.m_position);
        EXVector rot;
        d.m_rotation.GetRotation(rot, 4);
        m_pMuzzleScript->SetRotation(EXQuaternion(rot.x, rot.y, rot.z, rot.w));
    } else {
        m_pMuzzleScript->SetPosition(m_pItem->Position());
    }
}

void XItemHandler_Turret::UpdateMuzzleScript()
{
    if (!m_pMuzzleScript) return;
    if (XItemHandler_Player::Instance()->m_Tint == 1) m_pMuzzleScript->RenderInfo().m_renderFlags.BitSet(8);
    else m_pMuzzleScript->RenderInfo().m_renderFlags.BitClr(8);
    if (m_pMuzzleScript->m_11a) DeleteMuzzleScript();
}

// TODO: 3568-byte DoUpdate not written yet (virtual: kept as a stub for the vtable).
s8 XItemHandler_Turret::DoUpdate() { return 1; }
