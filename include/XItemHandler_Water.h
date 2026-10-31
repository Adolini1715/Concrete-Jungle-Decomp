#ifndef XITEMHANDLER_WATER_H
#define XITEMHANDLER_WATER_H
#include "EXItemHandler.h"
#include "EXExtRenderInfoStandIn.h"

class EXPFluid;
class EXItemAnimator_Entity;
class EXTrigger;
class EXVector;

// Water surface (FX_Water.obj): an EXPFluid simulation on the item. sizeof 0xd4.
class XItemHandler_Water : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_Water)
public:
    virtual ~XItemHandler_Water();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
protected:
    virtual u32 DoScriptCmdEvent(GEOSCRIPT_EVENT *pEvent, EXItemAnimator *pAnim);
public:
    XItemHandler_Water();
    static XItemHandler_Water *CreateObject();
    s8 WaterInit(EXTrigger *pTrigger);
    void TakeAHit(EXVector &a, EXVector &b);
    EXPFluid *m_pFluid;                 // 0x08 the item's physics
    EXItemAnimator_Entity *m_pEntity;   // 0x0c
    u32 m_Unk10;                        // 0x10 TODO
    EXExtRenderInfo m_ExtRenderInfo;    // 0x14
    u8 m_Unk94[0xd4 - 0x94];            // 0x94 TODO
};
#endif
