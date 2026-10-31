#ifndef XITEMHANDLER_H
#define XITEMHANDLER_H
#include "EXItemHandler.h"

// Base of the game's item handlers (sizeof 8).
class XItemHandler : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler)
public:
    virtual ~XItemHandler();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    virtual u32 DoScriptCmdObject(GEOSCRIPT_BASECMD *pCmd, EXItemAnimator *pAnim);
    XItemHandler();
    static XItemHandler *CreateObject();
};
#endif
