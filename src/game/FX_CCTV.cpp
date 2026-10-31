#include "XItemHandler_CCTV.h"
#include "XItemHandler_Objective.h"
#include "XItemHandler_Player.h"
#include "XItem.h"
#include "ScanInfo.h"
#include "FEHud.h"
#include "EXMemory.h"
#include "FEText.h"
#include <math.h>

float Vec_AngleDifference(float a, float b);

XItemList gCCTVList;
signed char GotDatum;
FERadarInfo RadInfo;

EX_IMPLEMENT_DYNCREATE_ROOT(XItemHandler_CCTV)

XItemHandler_CCTV::~XItemHandler_CCTV()
{
    gCCTVList.Remove((XItem *)m_pItem);
    m_SoundTag.RemoveAll(1);
}

// NEAR: retail stores m_bActive as an immediate and loads al = 1 afterwards.
s8 XItemHandler_CCTV::Initialise()
{
    if (!EXItemHandler::Initialise()) return 0;
    gCCTVList.Add((XItem *)m_pItem);
    GotDatum = 0;
    m_bActive = 1;
m_bAlarm = 0;
    return 1;
}

signed char XItemHandler_CCTV::ScanCallback(signed char b, FEObject *pObj, ScanInfo *pInfo)
{
    XItemHandler_CCTV *pCCTV = (XItemHandler_CCTV *)pInfo->m_pItem->ItemHandler();
    pInfo->m_a4 = pCCTV->m_State == 3 ? 0.0f : 1.0f;
    if (pCCTV->m_State != 3) pInfo->m_text.Format("Active");
    else pInfo->m_text.Format("Disabled");
    return 1;
}

void Draw_CameraViewcones() {}

signed char XItemHandler_CCTV::CanSeePoint(EXVector &p)
{
    EXVector d = p - m_pItem->Position();
    if (d.LengthXYZ() > m_Range) return 0;
    if (!((float)fabs(Vec_AngleDifference((float)atan2(d.x, d.z), m_pItem->Rotation().y)) < m_HAngle)) return 0;
    EXMaths::vec_xyzw v = d;
    float h = EXSqrt(v.x * v.x + v.z * v.z);
    if ((float)fabs(Vec_AngleDifference(-(float)atan2(d.y, h), m_pItem->Rotation().x)) < m_VAngle) return 1;
    return 0;
}

signed char XItemHandler_CCTV::CanSeePredator()
{
    if (m_bCloakHides && XItemHandler_Player::Instance()->GetCloak()) return 0;
    EXVector p = gpPlayerItem[0]->Position();
    p.y += 1.5f;
    return CanSeePoint(p);
}

// NEAR: TODO (1824 bytes: camera sweep, alarm, objective flags). Stub.
s8 XItemHandler_CCTV::DoUpdate() { return 1; }

// NEAR: retail reads the option bit with movzx (we get xor/mov al) and stores m_Text[0][1] before m_Text[1][0].
signed char XItemHandler_CCTV::GetScanInfo(int type, ScanInfo *pInfo)
{
    switch (type) {
    case 0:
        pInfo->m_a0 = 0x40005ea;
        pInfo->m_Text[0][0] = 0x5100053d;
        pInfo->m_Text[1][0] = 0x510007d3;
        break;
    case 2:
        pInfo->m_a0 = 0x4000793;
        pInfo->m_Text[0][0] = 0x5100053c;
        pInfo->m_Text[0][1] = m_bCloakHides | 0x5100053e;
        pInfo->m_Text[1][0] = 0x51000557;
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
