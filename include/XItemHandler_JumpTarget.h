#ifndef XITEMHANDLER_JUMPTARGET_H
#define XITEMHANDLER_JUMPTARGET_H
#include "EXItemHandler.h"

class XItemList;

// Jump target marker (FX_JumpTarget.obj). sizeof 8.
class XItemHandler_JumpTarget : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_JumpTarget)
public:
    virtual ~XItemHandler_JumpTarget();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    XItemHandler_JumpTarget() {}
    static XItemHandler_JumpTarget *CreateObject();
};

extern XItemList gJumpTargetList;
#endif
