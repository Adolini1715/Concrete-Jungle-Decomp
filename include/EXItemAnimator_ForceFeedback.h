#ifndef EXITEMANIMATOR_FORCEFEEDBACK_H
#define EXITEMANIMATOR_FORCEFEEDBACK_H
#include "EXItemAnimator.h"

// Force-feedback placeholder animator: an EXItemAnimator with its own type (EXItemAnimator.obj).
class EXItemAnimator_ForceFeedback : public EXItemAnimator {
    EX_DECLARE_DYNAMIC(EXItemAnimator_ForceFeedback)
public:
    EX_DECLARE_ITEMTYPE(EXItemAnimator_ForceFeedback)
    EXItemAnimator_ForceFeedback() {}
    static EXItemAnimator_ForceFeedback *CreateObject();
    static EXItemAnimator_ForceFeedback *NewItem() { return new EXItemAnimator_ForceFeedback; }
};
#endif
