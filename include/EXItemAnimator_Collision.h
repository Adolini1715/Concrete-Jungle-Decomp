#ifndef EXITEMANIMATOR_COLLISION_H
#define EXITEMANIMATOR_COLLISION_H
#include "EXItemAnimator.h"
#include "EXDatum.h"

struct GEOSCRIPT_COLLISION;

// Animator carrying one collision datum (sizeof 0x138).
class EXItemAnimator_Collision : public EXItemAnimator {
    EX_DECLARE_DYNAMIC(EXItemAnimator_Collision)
public:
    EX_DECLARE_ITEMTYPE(EXItemAnimator_Collision)
    virtual void SetObjectTime(float t);
    virtual void *GetDatum(u32 type, EXDatumTemp &d, u32 flags, void *&pCtx);
    virtual void GetDatum(u32 type, EXDatumTempArray &arr, u32 flags);
    EXItemAnimator_Collision();
    static EXItemAnimator_Collision *CreateObject();
    void SetFromScriptCmd(const GEOSCRIPT_COLLISION *pCmd);

    EXDatumTemp m_datum;    // +0x104
    s32 m_134;              // +0x134 TODO (from the script command +0xc)
};
#endif
