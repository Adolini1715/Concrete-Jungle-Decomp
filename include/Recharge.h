#ifndef RECHARGE_H
#define RECHARGE_H
#include "EXItemHandler.h"
#include "FEObjectScript.h"
#include "EXExtRenderInfoStandIn.h"

class XItem;
class EXVector;

// Energy recharge point (Recharge.obj). sizeof 0xb8. State: 0 idle, 1 recharging, 2 used up.
class XItemHandler_RechargePoint : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_RechargePoint)
public:
    virtual ~XItemHandler_RechargePoint();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    XItemHandler_RechargePoint() {}
    static XItemHandler_RechargePoint *CreateObject();
    void StartRecharge();
    void EndRecharge();
    s8 m_bRecharged;                // 0x08 energy given for this use
    u8 m_Unk9[3];                   // (EXTexSwapInfo stand-in has byte alignment)
    EXTexSwapInfo m_TexSwapInfo;    // 0x0c
    EXExtRenderInfo m_ExtRenderInfo;// 0x20
    unsigned int m_IdleScript;      // 0xa0
    unsigned int m_StartScript;     // 0xa4
    unsigned int m_EmptyScript;     // 0xa8 script hash when it runs out (> 0x4000000)
    int m_EmptyFlagChannel;         // 0xac set when it runs out
    u32 m_Unkb0;                    // 0xb0 TODO
    int m_State;                    // 0xb4
};

// Beacon (Recharge.obj). sizeof 0xac.
class XItemHandler_Beacon : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_Beacon)
public:
    virtual ~XItemHandler_Beacon();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    XItemHandler_Beacon() {}
    static XItemHandler_Beacon *CreateObject();
    void Activate();
    EXTexSwapInfo m_TexSwapInfo;    // 0x08
    EXExtRenderInfo m_ExtRenderInfo;// 0x1c
    int m_ActivateFlagChannel;      // 0x9c set on activation
    unsigned int m_ActivateScript;  // 0xa0
    int m_ShowFlagChannel;          // 0xa4 beacon becomes visible when set
    int m_bActivated;               // 0xa8
};

XItem *Recharge_GetNearest(const EXVector &pos);
XItem *Beacon_GetNearest(const EXVector &pos);
#endif
