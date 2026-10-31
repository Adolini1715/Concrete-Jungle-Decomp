#ifndef EXITEMANIMATOR_ANIMMODIFIER_H
#define EXITEMANIMATOR_ANIMMODIFIER_H
#include "EXItemAnimator_Anim.h"
#include "EXArray.h"
#include "EXQuaternion.h"

class EXItemCtrl_AnimModifier;
class EXItemCtrl_SnapShot;
class EX6DAnim;
struct EX6DAnimInfo;

// Procedural spine bend from the recent step directions (sizeof 0x48).
class EXSpineModifier {
    EX_DECLARE_DYNAMIC(EXSpineModifier)
public:
    virtual ~EXSpineModifier();
    EXSpineModifier();
    static EXSpineModifier *CreateObject();
    s8 OverRides() const;
    void NewStep(const EXVector &dir, float t);
    void Update(const EXItemRender_SkinAnim *pSkin);
protected:
    void AdjustLeg(EXItemRender_SkinAnimSection *pSec, float f, EXMatrix &m) const;
    void RecursivelyRotate(EXItemRender_SkinAnimSection *pSec, u32 n, float f, EXMatrix &m);
    float GetAngle(EXVector &v, float a, float b, s8 &flag);
public:
    EXFixedArray<EXVector, 128> *m_pSteps;  // +4 step directions (ring buffer)
    EXFixedArray<float, 128> *m_pTimes;     // +8 step times
    u32 m_numSteps;                         // +0xc
    u32 m_curStep;                          // +0x10
    u32 m_14;
    u32 m_18;
    float m_stepLength;                     // +0x1c
    float m_20;
    float m_24;
    float m_28;
    float m_lastStep;                       // +0x2c
    float m_stepDot;                        // +0x30
    float m_34;
    float m_38;
    float m_3c;
    float m_40;
    u8 m_44;
    u8 m_45;
    u8 m_46;
    u8 m_47;
};

// One tracked bone (sizeof 0xc).
struct EXHeadTrackingBone {
    float m_weight;     // +0
    u32 m_hash;         // +4
    u32 m_08;
};

// Head/neck look-at modifier (sizeof 0xa8).
class EXHeadTracking {
    EX_DECLARE_DYNAMIC(EXHeadTracking)
public:
    enum HTDirection { HT_0, HT_1, HT_2, HT_3 };
    virtual ~EXHeadTracking();
    EXHeadTracking();
    static EXHeadTracking *CreateObject();
    ex::EXFlag<u32> &Flags() { return m_flags; }
    void SetNumBones(u32 n);
    void SetCustomMapping(const float *p);
    void SetHash(u32 i, u32 hash)
    {
        if (i < m_numBones) m_pBones[i].m_hash = hash;
    }
    void SetBaseHash(u32 hash) { m_baseHash = hash; }
    void SetMaxAngularMove(float f) { m_maxAngularMove = f; }
    // Look at v (m_a3 set instead lets the head return to rest).
    void SetTarget(const EXVector &v) { m_forwards = v; m_a0 = 1; m_a3 = 0; }
    void Update(const EXItemRender_SkinAnim *pSkin, s8 b);
    void ReturnAngles(float &a, float &b) const;
protected:
    void FindAndRotate(EXItemRender_SkinAnimSection *pSec, float f, EXVector &v, EXMatrix m, u32 &n);
public:
    EXVector m_forwards;            // +4 look-at target (SetTarget)
    EXVector m_14;
    EXMatrix m_mtx;                 // +0x24
    EXVector m_64;
    EXHeadTrackingBone *m_pBones;   // +0x74
    float m_maxAngularMove;         // +0x78
    float m_7c;
    float m_80;
    float m_84;
    float m_88;
    u32 m_8c;
    u32 m_90;
    u32 m_baseHash;                 // +0x94
    ex::EXFlag<u32> m_flags;        // +0x98
    u32 m_numBones;                 // +0x9c
    u8 m_a0;
    u8 m_a1;
    u8 m_a2;
    u8 m_a3;
    u8 m_a4;
};

struct EXSwapBoneInfo {
    u32 m_a;
    u32 m_b;
};

// Bones mirrored by a flip: centre bones (the first is the root) and left/right pairs (sizeof 0x20).
class EXFlipInfo {
public:
    EXFlipInfo() {}
    EXDataArray<u32, u32 &> &CentreBones() { return m_centreBones; }
    EXDataArray<EXSwapBoneInfo, EXSwapBoneInfo &> &SwapBones() { return m_swapBones; }
    void AddRootBone(u32 bone);
    void AddCentreBone(u32 bone);
    void AddSwapBone(u32 a, u32 b);
    EXDataArray<u32, u32 &> m_centreBones;                  // +0
    EXDataArray<EXSwapBoneInfo, EXSwapBoneInfo &> m_swapBones; // +0x10
};

// Anim animator with procedural modifiers (spine, head tracking, flip, 6D bodies). sizeof 0x194.
class EXItemAnimator_AnimModifier : public EXItemAnimator_Anim {
    EX_DECLARE_DYNAMIC(EXItemAnimator_AnimModifier)
public:
    virtual ~EXItemAnimator_AnimModifier();
    EX_DECLARE_ITEMTYPE(EXItemAnimator_AnimModifier)
    virtual void DoUpdate();
    virtual s8 _GetBoundsBox(EXBoundsBox &box, u32 flags);

    EXItemAnimator_AnimModifier();
    static EXItemAnimator_AnimModifier *CreateObject();
    // Called, not inlined, by CreateModifierFrom (copy emitted in Actor.obj).
    static __declspec(noinline) EXItemAnimator_AnimModifier *NewItem() { return new EXItemAnimator_AnimModifier; }
    EXHeadTracking *HeadTracking() { return m_pHeadTracking; }
    EXFlipInfo *FlipInfo() { return m_pFlipInfo; }
    EXItemCtrl_AnimModifier *ModCtrl() { return m_pModCtrl; }
    void SetModCtrl(EXItemCtrl_AnimModifier *p) { m_pModCtrl = p; }
    EXItemCtrl *ConvertToAnimModifierMulti(u32 mask);
    s8 Add6DModifier(EX6DAnimInfo &info);
    s8 Set6DAnim(const EX6DAnim *p);
    void ResetPositions(EXItemCtrl_SnapShot *pSnap, EXItemCtrl_SnapShot *pVel, float t);
    static EXItemAnimator_AnimModifier *CreateModifierFrom(EXGeoFile *pFile, u32 hash);
    void Flip();
    void SetupAutoFlip();
    void RemoveModifier(u8 m);
    void AddModifier(u8 m);
protected:
    void SwapRootBone(const EXItemRender_SkinAnim *pSkin, u32 bone) const;
    void SwapBone(const EXItemRender_SkinAnim *pSkin, u32 bone) const;
    void SwapLeftRightBones(const EXItemRender_SkinAnim *pSkin, u32 a, u32 b) const;
    void CheckBone(u32 bone);
    void CheckSwapBone(u32 a, u32 b);
public:
    EXSpineModifier *m_pSpine;              // +0x180
    EXHeadTracking *m_pHeadTracking;        // +0x184
    EXFlipInfo *m_pFlipInfo;                // +0x188
    EXItemCtrl_AnimModifier *m_pModCtrl;    // +0x18c
    u8 m_190;
    s8 m_b6D;                               // +0x191
    u8 m_modifiers;                         // +0x192 1 spine, 2 head tracking, 4 flip, 8 auto flip
};
#endif
