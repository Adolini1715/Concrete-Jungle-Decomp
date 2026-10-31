#ifndef XITEMHANDLER_LIGHTSET_H
#define XITEMHANDLER_LIGHTSET_H
#include "EXItemHandler.h"

// Item that runs the level's light set script (NightAndDay.obj). sizeof 8.
class XItemHandler_LightSet : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_LightSet)
protected:
    XItemHandler_LightSet();
    virtual ~XItemHandler_LightSet();
    virtual u32 DoScriptCmdEvent(GEOSCRIPT_EVENT *pEvent, EXItemAnimator *pAnim);
public:
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    static XItemHandler_LightSet *CreateObject();
    static void StartLightSetScript();
};

// Skins 6..9 are the AVP predators (thermal vision uses the AVP palette).
inline signed char IsSkinAVP(int skin) { return skin >= 6 && skin <= 9; }
#endif
