#ifndef EXITEMANIMATOR_ANIM_H
#define EXITEMANIMATOR_ANIM_H
#include "EXItemAnimator.h"
#include "EXItemCtrl.h"
#include "EXItemRender.h"
#include "EXItemRender_Anim.h"
#include "EXEffectsSys.h"
#include "EXArray.h"
#include "EXu32Array.h"
#include "EXBit128.h"
#include "EXStdFuncs.h"

class EXParticleSys;
class EXGeoAnimSkinHeader;
class EXGeoAnimModeHeader;
class EXGeoAnimSetHeader;
class EXGeoHeader;
struct EXDatumTemp;
struct EXRotPosInfo;
struct EXMotionData;
struct EXAnimatorScriptCallbacks;


// Animator for a skinned, animated entity: up to three controller stacks blended together.
// sizeof 0x180.
class EXItemAnimator_Anim : public EXItemAnimator {
    EX_DECLARE_DYNAMIC(EXItemAnimator_Anim)
public:
    virtual ~EXItemAnimator_Anim();
    EX_DECLARE_ITEMTYPE(EXItemAnimator_Anim)
    virtual void DoUpdate();
    virtual u32 DoRender(const EXMatrix &mtx, const EXRenderInfo &info, u32 flags);
    virtual s8 Initialise(EXGeoFile *pFile, u32 hash);
    virtual void SetObjectTime(float t);
    virtual void UpdateObjectTime(float dt);
    virtual void EnableParticleSystems(s8 bEnable) const;
    virtual void *GetDatum(u32 type, EXDatumTemp &d, u32 flags, void *&pCtx);
    virtual void GetDatum(u32 type, EXDatumTempArray &arr, u32 flags);
    virtual void *GetNearestDatum(const EXVector &p, EXDatumTemp &d, u32 flags);
    virtual void DoEntityScoop(const EXMatrix &mtx, EXCollisionScoopTemp &t, u32 flags);
    virtual void SetRenderState(u32 state, s8 bOn);
    virtual void SetLodMin(u32 lod);
    virtual void SetLodMax(u32 lod);
    virtual void BuildEntityInfoArray(const EXMatrix &mtx, EXGeoEntity_InfoArray &arr, s8 b);
    virtual EXGeoCommonObject *GetGeoObject() const;
protected:
    virtual s8 _GetBoundsBox(EXBoundsBox &box, u32 flags);
public:
    virtual u32 ApplyCtrlFunction(u32 (__cdecl *pFunc)(EXItemCtrl *, void *), void *pData);

    EXItemAnimator_Anim();
    static EXItemAnimator_Anim *CreateObject();
    static EXItemAnimator_Anim *NewItem() { return new EXItemAnimator_Anim; }
    EXItemCtrlStack &ControllerStack(u32 i) { return m_stacks[i]; }
    EXItemCtrl *Controller(u32 i) const
    {
        EXDListItem *p = m_stacks[i].m_list.m_pHead;
        return p ? (EXItemCtrl *)((char *)p - 4) : 0;
    }
    int ControllerStackCount() const { return m_numStacks; }
    EXItemRender_SkinAnim *RenderItem() const { return m_pSkin; }
    void RegisterCallbacks(const EXAnimatorScriptCallbacks *p) { m_pScriptCallbacks = p; }
    const EXAnimatorScriptCallbacks *ScriptCallbacks() const { return m_pScriptCallbacks; }
    void EnableStackControllerObjects(s8 b) { m_stackCtrlObjects = b; }
    u32 LodDetailsFlags() const { return m_lodFlags; }
    EXu32Array *SnapShotDatums() { return m_pSnapShotDatums; }

    void DeleteStack(u32 i);
    void DetatchParticleSys() const;
    void DeleteParticleSys() const;
    EXParticleSys *ParticleSys() const;
    void *GetSkelDatum(u32 type, EXDatumTemp &d, u32 flags, void *&pCtx);
    void *GetSkelDatum(const EXVector &p, EXDatumTemp &d, u32 flags);
    void GetSkelDatum(u32 type, EXDatumTempArray &arr, u32 flags);
    EXItemRender_SkinAnimSection *GetAnimBoneSection(u32 a, u32 b);
    void SetLodDetails(EXAnimLodDetails *p, u32 n);
    void SetSkinRefObject(const EXGeoRefObject &r);
    void SetSkinRefObject(EXGeoFile *pFile, u32 hash);
    void SetSkinRefObject(u32 hash);
    void SetShadowRefObject(const EXGeoComplexRefObject &r);
    void SetCollisionRefObject(const EXGeoComplexRefObject &r);
    void SetReflectionRefObject(const EXGeoComplexRefObject &r);
    const EXGeoComplexRefObject &SkelRefObject() const;
    const EXGeoComplexRefObject &SkinRefObject() const;
    const EXGeoComplexRefObject &ShadowRefObject() const;
    const EXGeoComplexRefObject &ReflectionRefObject() const;
    EXItemRender_SkinAnim *AddCloneSkin(const EXGeoRefObject &r);
    void DeleteCloneSkin(EXItemRender_SkinAnim *p, s8 b);
    void DeleteCloneSkin(const EXGeoRefObject &r, s8 b);
    EXItemRender_SkinAnim *FindCloneSkin(const EXGeoRefObject &r) const;
    void UpdateSkinInformation(ex::EXBit128 &bits);
    void BuildSkel(u32 hash);
    void InitControllerStack(u32 i, u32 size, u32 flags);
    EXItemCtrl *AddController(u32 i, EXItemCtrl *p);
    EXItemCtrl *NewController(u32 i, u32 hash, u32 flags);
    EXItemCtrl *ChangeTo(u32 i, u32 hash, u32 flags);
    EXItemCtrl *ConvertToSnapShot(u32 i);
    EXItemCtrl *ConvertToSnapShotMulti(u32 mask);
    EXItemCtrl *ConvertStackToSnapShot(u32 i);
    void UpdateControllers();
    void BlendStacks(const EXItemCtrlStack *pStack) const;
    s8 CalcTotalMotion(EXMotionData &m) const;
    u8 CalcMotionData(EXMotionData &m, u32 a, u32 b) const;
    void *GetFrameDatum_CtrlStack(const EXItemCtrlStack *pStack, u32 id, EXDatumTemp &d, u32 flags);
    void *GetFrameDatum(u32 id, EXDatumTemp &d, u32 flags);
    void GetGeoBoundsBox(EXBoundsBox &box);
    u32 GetCtrlAnimFlags();
    s8 RemoveSnapShotDatumReference(u32 id);
    void AddSnapShotDatumReference(u32 id);
    s8 Initialise(const EXGeoRefObject &skel, const EXGeoRefObject &skin);
    EXItemCtrl *BlendTo(u32 i, u32 hash, u32 flags, u32 frames);
    void BlendToAnimSet(const EXGeoAnimSetHeader *pSet, u32 a, u32 b, u32 c);
    void ChangeToAnimSet(const EXGeoAnimSetHeader *pSet, u32 a);
    s8 CheckAnimMode(u32 hash) const;
    u32 ChangeToAnimMode(const EXGeoAnimModeHeader *pMode, int n, const EXGeoHeader *pHdr);
    u32 ChangeToAnimMode(u32 a, u32 b, u32 c);
    void UpdateRenderHierarchy();
    static u32 ParticlePosCallback(EXItemAnimator *pAnim, EXVector *pOut, u32 n);
    void SetObjectFrame(float f);
    s8 AssignSkelSkinAnim(const EXGeoRefObject &skel, const EXGeoRefObject &skin, const EXGeoRefObject &anim);
    s8 AssignSkelSkinAnim(int a, int b, int c, int d, int e, int f);
    u32 SetFrameDepth(float f);
    s8 AttachParticleSys(EXGeoFile *pFile, u32 hash, const EXRuntimeClass *pClass);
protected:
    void CleanInitialise();
private:
    void SetItemRender(EXItemRender_SkinAnim *p);
    void CalculateLodDetails();
    static u32 GetCtrlAnimFlags_Func(EXItemCtrl *p, void *pData);
    void DeleteUnusedStacks(const EXGeoAnimSetHeader *pSet);
    void DoRender_ControllerObjects(const EXMatrix &mtx, const EXRenderInfo &info, u32 flags);
    u8 CalcMotionData_CtrlStacks(EXMotionData *pData, u32 n, const u32 *pIds, int m) const;
    static u32 SetFrameDepth_Func(EXItemCtrl *p, void *pData);
    void ForceUpdate();
public:
    EXItemRender_SkinAnim *m_pSkin;                         // +0x104
    const EXAnimatorScriptCallbacks *m_pScriptCallbacks;    // +0x108
    EXAnimLodDetails *m_pLodDetails;                        // +0x10c
    EXu32Array *m_pSnapShotDatums;                          // +0x110
    EXEffectsSysTag m_effects;                              // +0x114
    u8 m_numStacks;                                         // +0x11c
    s8 m_stackCtrlObjects;                                  // +0x11d
    u8 m_numLods;                                           // +0x11e
    u8 m_lodFlags;                                          // +0x11f
    EXItemCtrlStack m_stacks[3];                            // +0x120
};
#endif
