#include "XItemHandler_Water.h"
#include "XItem.h"
#include "EXTrigger.h"
#include "EXPFluid.h"
#include "EXItemAnimator_Entity.h"
#include "EXMemory.h"
#include "FEText.h"

EX_IMPLEMENT_DYNCREATE(XItemHandler_Water, EXItemHandler)

XItemHandler_Water::XItemHandler_Water() {}

XItemHandler_Water::~XItemHandler_Water() {}

s8 XItemHandler_Water::Initialise() { return 1; }

s8 XItemHandler_Water::DoUpdate() { return 1; }

u32 XItemHandler_Water::DoScriptCmdEvent(GEOSCRIPT_EVENT *pEvent, EXItemAnimator *pAnim) { return 0; }

void XItemHandler_Water::TakeAHit(EXVector &a, EXVector &b) {}

s8 XItemHandler_Water::WaterInit(EXTrigger *pTrigger)
{
    EXGeoFile *pFile = m_pItem->GeoFile();
    m_pEntity = new EXItemAnimator_Entity;
    if (m_pEntity) {
        m_pEntity->Initialise(pFile, pTrigger->m_Hash);
        m_pItem->AddAnimator(m_pEntity);
    }
    m_pItem->SetClipType(2);
    m_pItem->SetMapOn(theItemEnv.GetMapOn(pTrigger->m_Position));
    m_pItem->SetRefObject(pTrigger->m_Hash);
    m_pItem->SetItemPhysics((EXItemPhysics *)EXRUNTIME_CLASS(EXPFluid)->CreateObject());
    m_pFluid = (EXPFluid *)m_pItem->ItemPhysics();
    m_pItem->RenderInfo().SetExtRenderInfo(&m_ExtRenderInfo);
    m_pFluid->Initialise(pTrigger->m_Data[0], pTrigger->m_Data[1], pTrigger->DataF(2), pTrigger->DataF(3),
                         pTrigger->DataF(4), 0);
    m_pFluid->Hit(2, 2, -0.06f, 0.3f);
    return 1;
}
