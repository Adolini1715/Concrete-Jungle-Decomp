#include "EVD.h"
#include "Actor.h"
#include "StateMachine.h"
#include "ActorLocation.h"
#include "XGameWnd.h"
#include "EXMemory.h"
#include "FEText.h"

void AirTraffic_CreateEmergencyVehicle(EXVector &pos);
extern XGameWnd *gpGameWnd[4];  // TODO stand-in (GameWnd.obj)

EVD *EVD::m_pInstance;

void EVD::Initialise()
{
    m_Next = 0;
    memset(m_Calls, 0, sizeof(m_Calls));
}

void EVD::Update()
{
    for (int i = 0; i < 4; i++) {
        EVDCall &c = m_Calls[i];
        if (!c.m_bActive) continue;
        if (c.m_bDispatched) continue;
        c.m_bDispatched = 1;
        AirTraffic_CreateEmergencyVehicle(c.m_Pos);
        int id = 0;
        int next = 0;
        s8 vendor = 1;
        s8 ok = 1;
        for (Actor *a = Actor::GetFirst(c.m_Pos, 30.0f, 0); a; a = a->GetNext(c.m_Pos, 30.0f, 0)) {
            if (ok) {
                int tries = 0;
                for (int k = next; k < 4; k++) {
                    if (vendor) id = POI_VendorGetID(m_Calls[k].m_POI, k);
                    else id = POI_CustomerGetID(m_Calls[k].m_POI, k);
                    if (id > 0) {
                        next = k + 1;
                        if (next > 3) {
                            next = 0;
                            vendor = !vendor;
                        }
                        break;
                    }
                    if (k == 3) {
                        next = 0;
                        vendor = !vendor;
                        if (++tries >= 2) break;
                    }
                }
            }
            MsgItem msg((MsgType)0x2b, id);
            ok = a->SendMsg(msg, 0.0f) > 0;
        }
    }
}

void EVD::Dial999(EXVector pos, int poi, int a, CrimeType::Flags crime)
{
    m_Calls[m_Next].m_bActive = 1;
    m_Calls[m_Next].m_bDispatched = 0;
    m_Calls[m_Next].m_Pos = pos;
    m_Calls[m_Next].m_POI = poi;
    m_Calls[m_Next].m_Unk18 = a;
    m_Calls[m_Next].m_Crime = crime;
    m_Calls[m_Next].m_Time = gpGameWnd[0]->m_Unk384;
    m_Next = (m_Next + 1) % 4;
}

EVD::EVD()
{
    m_Next = 0;
    memset(m_Calls, 0, sizeof(m_Calls));
}

EVD *EVD::Instance(signed char create)
{
    if (!m_pInstance && create) m_pInstance = new EVD;
    return m_pInstance;
}
