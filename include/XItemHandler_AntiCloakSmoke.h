#ifndef XITEMHANDLER_ANTICLOAKSMOKE_H
#define XITEMHANDLER_ANTICLOAKSMOKE_H
#include "EXItemHandler.h"

class EXVector;
class EXItemAnimator_Script;

// Anti-cloak smoke grenade cloud (ActorCarry.obj). sizeof 0xc.
class XItemHandler_AntiCloakSmoke : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_AntiCloakSmoke)
public:
    virtual ~XItemHandler_AntiCloakSmoke();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    XItemHandler_AntiCloakSmoke() {}
    static XItemHandler_AntiCloakSmoke *CreateObject();
    EXItemAnimator_Script *m_pScript;  // 0x08 smoke cloud script
};

void CreateAntiCloakSmoke(const EXVector &pos);
#endif
