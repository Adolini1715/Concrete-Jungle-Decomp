#ifndef XITEMHANDLER_TESTCLOTH_H
#define XITEMHANDLER_TESTCLOTH_H
#include "EXItemHandler.h"
#include "EXExtRenderInfoStandIn.h"
#include "EXTexSwapInfo.h"

class EXVector;
class EXTrigger;
class EXGeoEntity;
class EXItemAnimator_Entity;
class XItemList;

// Cloth / flag item driven by EXPCloth physics (TestCloth.obj). sizeof 0x2a4.
class XItemHandler_TestCloth : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_TestCloth)
public:
    virtual ~XItemHandler_TestCloth();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
protected:
    virtual u32 DoScriptCmdEvent(GEOSCRIPT_EVENT *pEvent, EXItemAnimator *pAnim);
public:
    XItemHandler_TestCloth();
    void TakeAHit(EXVector &, EXVector &);
    void BloodStain();
    static XItemHandler_TestCloth *CreateObject();
    signed char ClothInit(unsigned int &hash);
    signed char ClothOrWaterInit(EXTrigger *pTrig);
    EXItemAnimator_Entity *m_pEntity;   // 0x08
    EXGeoEntity *m_pGeoEntity;          // 0x0c
    EXExtRenderInfo m_ExtRenderInfo;    // 0x10
    EXTexSwapInfo m_TexSwap;            // 0x90
    u8 m_Unka4[0x2a4 - 0x90 - sizeof(EXTexSwapInfo)];  // TODO layout
};

// Rope item (TestCloth.obj). sizeof 0x14.
class XItemHandler_TestRope : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_TestRope)
public:
    virtual ~XItemHandler_TestRope();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
protected:
    virtual u32 DoScriptCmdEvent(GEOSCRIPT_EVENT *pEvent, EXItemAnimator *pAnim);
public:
    XItemHandler_TestRope();
    signed char RopeInit(unsigned int &hash);
    void ForceModeChange(unsigned int mode);
    void TakeAHit(EXVector &, EXVector &);
    static XItemHandler_TestRope *CreateObject();
    s8 m_Unk8;                          // 0x08
    u8 m_Unk9[0x14 - 0x9];              // TODO layout
};

extern XItemList gClothList;
#endif
