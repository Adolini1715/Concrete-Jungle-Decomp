#ifndef XITEMHANDLER_GRAFFITI_H
#define XITEMHANDLER_GRAFFITI_H
#include "EXItemHandler.h"
#include "EXTexSwapInfo.h"

class XItem;
class XItemList;
class EXVector;

// Graffiti marker (FX_Graffiti.obj). sizeof 0x1c.
class XItemHandler_Graffiti : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_Graffiti)
public:
    virtual ~XItemHandler_Graffiti();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    XItemHandler_Graffiti() {}
    static XItemHandler_Graffiti *CreateObject();
    EXTexSwapInfo m_TexSwapInfo;    // 0x08
};

extern XItemList gGraffitiList;
XItem *Graffiti_GetNearest(EXVector &pos);
#endif
