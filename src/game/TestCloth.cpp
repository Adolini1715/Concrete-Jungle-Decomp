#include "XItemHandler_TestCloth.h"
#include "XItem.h"
#include "EXTrigger.h"
#include "EXGeoFile.h"
#include "EXPCloth.h"
#include "EXItemAnimator_Entity.h"
#include "EXItemAnimator_AnimModifier.h"
#include "EXMemory.h"
#include "FEText.h"

XItemList gClothList;

EX_IMPLEMENT_DYNCREATE(XItemHandler_TestCloth, EXItemHandler)

XItemHandler_TestCloth::XItemHandler_TestCloth() {}

XItemHandler_TestCloth::~XItemHandler_TestCloth()
{
    gClothList.Remove((XItem *)m_pItem);
}

u32 XItemHandler_TestCloth::DoScriptCmdEvent(GEOSCRIPT_EVENT *pEvent, EXItemAnimator *pAnim) { return 0; }

void XItemHandler_TestCloth::TakeAHit(EXVector &, EXVector &) {}

void XItemHandler_TestCloth::BloodStain()
{
    m_TexSwap.SetSwap(0x60002b9, m_pItem->GeoFile(), 0x60002ba);
}

EX_IMPLEMENT_DYNCREATE(XItemHandler_TestRope, EXItemHandler)

XItemHandler_TestRope::XItemHandler_TestRope() {}

XItemHandler_TestRope::~XItemHandler_TestRope() {}

s8 XItemHandler_TestRope::Initialise()
{
    m_pItem->SetRotOrder(0);
    return 1;
}

signed char XItemHandler_TestRope::RopeInit(unsigned int &hash)
{
    EXItemAnimator_AnimModifier *p = new EXItemAnimator_AnimModifier;
    if (!p) return 0;
    p->Initialise(m_pItem->GeoFile(), hash);
    m_pItem->AddAnimator(p);
    p->InitControllerStack(0, 6, 1);
    p->InitControllerStack(1, 6, 1);
    p->InitControllerStack(2, 6, 1);
    p->SetFrameRate60(60.0f);
    m_Unk8 = 0;
    return 1;
}

void XItemHandler_TestRope::ForceModeChange(unsigned int mode)
{
    XItem *pItem = (XItem *)m_pItem;
    ((EXItemAnimator_Anim *)pItem->GetAnimator())->ChangeToAnimMode(pItem->m_Unk160, mode, 0x9000004);
    ((XItem *)m_pItem)->m_Unk160 = mode;
}

s8 XItemHandler_TestRope::DoUpdate() { return 1; }

u32 XItemHandler_TestRope::DoScriptCmdEvent(GEOSCRIPT_EVENT *pEvent, EXItemAnimator *pAnim) { return 0; }

void XItemHandler_TestRope::TakeAHit(EXVector &, EXVector &) {}

s8 XItemHandler_TestCloth::Initialise()
{
    gClothList.Add((XItem *)m_pItem);
    m_pItem->RenderInfo().SetTexSwapInfo(&m_TexSwap);
    m_pItem->RenderInfo().SetExtRenderInfo(&m_ExtRenderInfo);
    return 1;
}

signed char XItemHandler_TestCloth::ClothInit(unsigned int &hash)
{
    EXGeoFile *pFile = m_pItem->GeoFile();
    m_pEntity = new EXItemAnimator_Entity;
    if (m_pEntity) {
        m_pEntity->Initialise(pFile, hash);
        m_pItem->AddAnimator(m_pEntity);
    }
    m_pItem->SetClipType(2);
    m_pGeoEntity = pFile->GeoHeader()->GeoEntity(hash);
    return 1;
}

signed char XItemHandler_TestCloth::ClothOrWaterInit(EXTrigger *pTrig)
{
    EXGeoFile *pFile = m_pItem->GeoFile();
    m_pEntity = new EXItemAnimator_Entity;
    if (m_pEntity) {
        m_pEntity->Initialise(pFile, pTrig->m_Hash);
        m_pItem->AddAnimator(m_pEntity);
    }
    m_pItem->SetClipType(2);
    m_pGeoEntity = pFile->GeoHeader()->GeoEntity(pTrig->m_Hash);
    m_pItem->SetItemPhysics((EXItemPhysics *)EXPCloth::classEXPCloth.CreateObject());
    ((EXPCloth *)m_pItem->ItemPhysics())->Initialise(m_pGeoEntity, 0);
    ((EXPCloth *)m_pItem->ItemPhysics())->SetIterations(pTrig->m_Data[0]);
    ((EXPCloth *)m_pItem->ItemPhysics())->SetGravCoef(pTrig->DataF(1));
    EXVector wind(pTrig->DataF(2), pTrig->DataF(3), pTrig->DataF(4), 0.0f);
    ((EXPCloth *)m_pItem->ItemPhysics())->SetWind(wind);
    ((EXPCloth *)m_pItem->ItemPhysics())->SetDrag(pTrig->DataF(5));
    return 1;
}
