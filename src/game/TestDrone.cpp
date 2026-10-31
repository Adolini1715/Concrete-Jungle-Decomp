#include "XItemHandler_TestDrone.h"
#include "XItem.h"
#include "EXItemAnimator_Anim.h"
#include "EXMemory.h"
#include "FEText.h"

EX_IMPLEMENT_DYNCREATE(XItemHandler_LostLimb, EXItemHandler)

signed char XItemHandler_LostLimb::Initialise()
{
    m_Vel.x = 0.01f;
    m_Vel.y = 0.01f;
    m_Vel.z = 0.01f;
    m_Life = 100;
    return 1;
}

EX_IMPLEMENT_DYNCREATE(XItemHandler_TestDrone, EXItemHandler)

XItemHandler_TestDrone::~XItemHandler_TestDrone() { gTestDroneList.Remove((XItem *)m_pItem); }

unsigned int XItemHandler_TestDrone::DoScriptCmdEvent(GEOSCRIPT_EVENT *pEvent, EXItemAnimator *pAnim) { return 0; }

void XItemHandler_TestDrone::ProcessHit()
{
    if (m_bHit) {
        m_Unk6c = 2;
        m_bHit = 0;
    }
}

void XItemHandler_TestDrone::CheckModeChange(unsigned int mode)
{
    EXItemAnimator_Anim *pAnim = (EXItemAnimator_Anim *)m_pItem->GetAnimator();
    unsigned int cur = ((XItem *)m_pItem)->m_Unk160;
    if (mode != cur) {
        pAnim->ChangeToAnimMode(cur, mode, 0x9000004);
        ((XItem *)m_pItem)->m_Unk160 = mode;
    }
}

void XItemHandler_TestDrone::ForceModeChange(unsigned int mode)
{
    EXItemAnimator_Anim *pAnim = (EXItemAnimator_Anim *)m_pItem->GetAnimator();
    pAnim->ChangeToAnimMode(((XItem *)m_pItem)->m_Unk160, mode, 0x9000004);
    ((XItem *)m_pItem)->m_Unk160 = mode;
}

signed char XItemHandler_LostLimb::DoUpdate()
{
    m_Vel.y -= 0.003f;
    m_pItem->SetPosition(m_Vel + m_pItem->Position());
    if (--m_Life <= 0) m_pItem->ItemFlags().BitSet(0x10);
    return 1;
}

XItemHandler_TestDrone::XItemHandler_TestDrone()
{
    m_Unk8 = 0;
    m_bHit = 0;
}

void XItemHandler_TestDrone::MoveNormal() {}

void XItemHandler_TestDrone::TakeAHit(const TestHit &hit)
{
    if (m_bHit && m_Hit.m_Force >= hit.m_Force) return;
    m_bHit = 1;
    m_Hit = hit;
}

// TODO: FindHitLimb, SetupDepthPtr, SetupIdle, SetupMove, DoIdle, DoMove not written; Initialise and
// DoUpdate are stubs for the vtable.
signed char XItemHandler_TestDrone::Initialise() { return 1; }
signed char XItemHandler_TestDrone::DoUpdate() { return 1; }
