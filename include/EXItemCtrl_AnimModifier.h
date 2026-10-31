#ifndef EXITEMCTRL_ANIMMODIFIER_H
#define EXITEMCTRL_ANIMMODIFIER_H
#include "EXItemCtrl.h"
#include "EXArray.h"
#include "EXMatrix.h"

class EXItemAnimator_AnimModifier;
class EXItemCtrl_SnapShot;
class EXItemRender_SkinAnimSection;
struct EXMotionData;
class EX6DRigidBody;

// One rigid body driving a bone (sizeof 0x48). TODO layout: name of the matrix.
struct EX6DAnimInfo {
    EXMatrix m_mtx;             // +0
    EX6DRigidBody *m_pBody;     // +0x40 physics body (orientation +0xec, position +0x5c)
    u32 m_bone;                 // +0x44 skeleton bone index
};

// Controller overriding bones with rigid-body (6D) motion (sizeof 0x5c).
class EXItemCtrl_AnimModifier : public EXItemCtrl {
    EX_DECLARE_DYNAMIC(EXItemCtrl_AnimModifier)
public:
    virtual ~EXItemCtrl_AnimModifier();
    virtual s8 UpdateCtrlOnStack(EXItemAnimator_Anim *pAnim, EXItemCtrlStack *pStack, EXRotPosInfo *pInfo, int n);
    EXItemCtrl_AnimModifier();
    static EXItemCtrl_AnimModifier *CreateObject();
    void SetAnimMod(EXItemAnimator_AnimModifier *p) { m_pAnimMod = p; }
    void ResetPositions(const EXItemAnimator_AnimModifier *pMod, EXItemCtrl_SnapShot *pSnap, EXItemCtrl_SnapShot *pVel, float t);
    void AddBody(EX6DAnimInfo &info);
protected:
    s8 RecursiveSearch(EXMatrix m, const EXItemRender_SkinAnimSection *pSec, EXMotionData *pMotion);
    s8 RecursiveReset(EXMatrix m, const EXItemRender_SkinAnimSection *pSec, EXItemCtrl_SnapShot *pSnap);
    s8 RecursiveResetVelocities(EXMatrix m, const EXItemRender_SkinAnimSection *pSec, EXItemCtrl_SnapShot *pSnap, float t);
public:
    EXDataArray<EX6DAnimInfo, EX6DAnimInfo &> m_bodies;    // +0x40
    u32 m_50;                                               // +0x50 TODO
    EXItemAnimator_AnimModifier *m_pAnimMod;                // +0x54
    u32 m_58;                                               // +0x58 TODO
};
#endif
