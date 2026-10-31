#ifndef EXITEMANIMATOR_DYNLIGHT_H
#define EXITEMANIMATOR_DYNLIGHT_H
#include "EXItemAnimator.h"
#include "EXDynLight.h"

// Animator driving a dynamic light from its placement (sizeof 0x118).
class EXItemAnimator_DynLight : public EXItemAnimator {
    EX_DECLARE_DYNAMIC(EXItemAnimator_DynLight)
public:
    virtual ~EXItemAnimator_DynLight();
    EX_DECLARE_ITEMTYPE(EXItemAnimator_DynLight)
    virtual u32 DoRender(const EXMatrix &mtx, const EXRenderInfo &info, u32 flags);
    virtual s8 Initialise(EXGeoFile *pFile, u32 hash);
    EXItemAnimator_DynLight();
    static EXItemAnimator_DynLight *CreateObject();
    static EXItemAnimator_DynLight *NewItem() { return new EXItemAnimator_DynLight; }
    EXDynLight *DynLight() { return m_pDynLight; }
    void AttachDynLight(EXDynLight *pLight);
protected:
    void CleanInitialise();
public:
    EXDynLight *m_pDynLight;    // +0x104
    EXVector m_dir;             // +0x108 light direction in item space
};
#endif
