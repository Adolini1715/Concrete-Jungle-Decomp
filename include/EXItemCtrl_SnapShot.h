#ifndef EXITEMCTRL_SNAPSHOT_H
#define EXITEMCTRL_SNAPSHOT_H
#include "EXItemCtrl.h"
#include "EXQuaternion.h"
#include "EXDatum.h"

// TODO layout: per-bone motion (anim area).
struct EXMotionData {
    EXVector m_pos;         // +0
    EXQuaternion m_rot;     // +0x10
};
// TODO layout: controller-stack evaluation target (anim area).
struct EXRotPosInfo {
    ex::EXBit128 m_bits;        // +0
    EXMotionData *m_pMotion;    // +0x10
    float *m_pMorph;            // +0x14
};
// A datum captured in a snapshot (sizeof 0x34).
struct EXSnapShotDatum {
    EXDatumTemp m_datum;
    u32 m_id;
};

// Controller holding a captured pose (sizeof 0x54).
class EXItemCtrl_SnapShot : public EXItemCtrl {
    EX_DECLARE_DYNAMIC(EXItemCtrl_SnapShot)
public:
    virtual ~EXItemCtrl_SnapShot();
    virtual s8 UpdateCtrlOnStack(EXItemAnimator_Anim *pAnim, EXItemCtrlStack *pStack, EXRotPosInfo *pInfo, int n);
    virtual u8 CalcMotionData(EXMotionData *pData, const EXCalcMotionTemp &t);
    virtual void *GetFrameDatum(EXItemAnimator_Anim *pAnim, u32 type, EXDatumTemp &d, u32 flags);
private:
    virtual s8 Initialise(EXItemAnimator_Anim *pAnim);
public:
    EXItemCtrl_SnapShot();
    EXItemCtrl_SnapShot(EXItemAnimator_Anim *pAnim);
    EXItemCtrl_SnapShot(EXItemAnimator_Anim *pAnim, EXItemCtrlStack *pStack);
    static EXItemCtrl_SnapShot *CreateObject();
    EXMotionData *MotionData() { return m_pMotionData; }
    void Clone(const EXItemCtrl_SnapShot *p);
    void Create(EXItemAnimator_Anim *pAnim);
    void Create(EXItemAnimator_Anim *pAnim, EXItemCtrlStack *pStack, const EXItemCtrl *pCtrl);
    void ApplyXForm(const EXVector &pos, const EXQuaternion &rot);

    u32 DataSize() const { return m_numBones * sizeof(EXMotionData) + m_numMorphs * sizeof(float) + m_numDatums * sizeof(EXSnapShotDatum); }

    u16 m_numBones;                 // +0x40
    u16 m_numMorphs;                // +0x42
    u16 m_numDatums;                // +0x44
    u16 m_46;
    EXMotionData *m_pMotionData;    // +0x48 one allocation: motion, morph weights, datums
    float *m_pMorph;                // +0x4c
    EXSnapShotDatum *m_pDatums;     // +0x50
};
#endif
