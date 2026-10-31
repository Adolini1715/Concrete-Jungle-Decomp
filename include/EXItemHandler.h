#ifndef EXITEMHANDLER_H
#define EXITEMHANDLER_H
#include "types.h"
#include "EXRuntimeClass.h"

class EXItem;
class EXItemAnimator;
class EXGeoAnimSetHeader;
struct EXGeoAnimModeFunction;
struct GEOSCRIPT_EVENT;
struct GEOSCRIPT_RUNCONTROL;
struct GEOSCRIPT_BASECMD;

// Abstract base for item behaviours (sizeof 8). The destructor is pure virtual with a body,
// which is why vtable slot 1 is __purecall.
class EXItemHandler {
    EX_DECLARE_DYNAMIC(EXItemHandler)
public:
    virtual ~EXItemHandler() = 0;
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    virtual s8 DoPostUpdate();
    virtual u32 DoScriptCmdEvent(GEOSCRIPT_EVENT *pEvent, EXItemAnimator *pAnim);
    virtual u32 DoScriptCmdObject(GEOSCRIPT_BASECMD *pCmd, EXItemAnimator *pAnim);
    virtual u32 DoScriptCmdRunControl(GEOSCRIPT_RUNCONTROL *pCmd, EXItemAnimator *pAnim);
    virtual u32 DoAnimModeFunction(EXGeoAnimSetHeader *pSet, EXGeoAnimModeFunction *pFunc, EXItemAnimator *pAnim);
    EXItemHandler();
    EXItem *GetItem() const { return m_pItem; }
    void SetItem(EXItem *pItem);
protected:
    EXItem *m_pItem;
};
// Header inline (emitted in EXItem.obj).
inline void EXItemHandler::SetItem(EXItem *pItem) { m_pItem = pItem; }
#endif
