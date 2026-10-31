#ifndef EXITEMCTRL_H
#define EXITEMCTRL_H
#include "types.h"
#include "EXRuntimeClass.h"
#include "EXList.h"
#include "EXFlag.h"
#include "EXTagPtr.h"
#include "EXBit128.h"
#include "EXVector.h"
#include "EXGeoComplexRefObject.h"
#include "EXSoundTagStandIn.h"

class EXItemAnimator_Anim;
class EXItemAnimator_Script;
class EXItemCtrlStack;
class EXGeoFile;
class EXGeoHeader;
class EXGeoAnim;
class EXGeoBaseAnim;
class EXGeoScript;
class EXScriptThread;
struct EXRotPosInfo;
struct EXMotionData;
// Motion evaluation parameters handed to the controllers (EXItemAnimator_Anim::CalcMotionData_CtrlStacks).
struct EXCalcMotionTemp {
    u32 m_flags;        // +0x00 0x80: root bone only (one motion), 0x40 ...
    u32 m_lod;          // +0x04 animator LOD flags (bit 2: ...)
    u32 m_ids[2];       // +0x08 bone / datum ids of the motions
    int m_count;        // +0x10 motions wanted
    float m_rate;       // +0x14 animator frame rate multiplier
};
struct EXDatumTemp;
struct EXBoundsBox;
struct _GEOSCRIPT_ANIM;
struct GEOSCRIPT_ANIM;
struct GEOSCRIPT_REFCMD;

// Animation reference: geo reference plus the resolved anim hash (sizeof 0x14).
class EXAnimBaseInfo {
public:
    EXAnimBaseInfo() {}
    void Set(EXGeoFile *pFile, u32 hash)
    {
        m_ref.Set(pFile, hash);
        m_animHash = 0xffffffff;
    }
    EXGeoComplexRefObject &RefObject() { return m_ref; }
    u32 AnimRef() const { return m_ref.m_hashRef; }
    u32 AnimHash() const
    {
        if (m_animHash != 0xffffffff) return m_animHash;
        return m_ref.m_hashRef;
    }
    EXGeoHeader *GeoHeader(EXGeoHeader *pHdr) const;
    EXGeoBaseAnim *GeoAnim(const EXGeoHeader *pHdr);
private:
    u32 CalcAnimRef(const EXGeoHeader *pHdr);
public:
    EXGeoComplexRefObject m_ref;    // +0
    u32 m_animHash;                 // +0x10 -1: not resolved yet
};

// One animation of a controller with its depth key and frame range (sizeof 0x1c).
class EXAnimMotionMorph : public EXAnimBaseInfo {
public:
    EXAnimMotionMorph() {}
    void Set(EXGeoFile *pFile, u32 hash, float depth, s16 start, s16 end)
    {
        m_ref.Set(pFile, hash);
        m_animHash = 0xffffffff;
        m_depth = depth;
        m_startFrame = start;
        m_endFrame = end;
    }
    int StartFrame() const { return m_startFrame; }
    int EndFrame() const { return m_endFrame; }
    int NumFrames() const { return m_endFrame - m_startFrame; }
    float Depth() const { return m_depth; }
    float m_depth;                  // +0x14
    s16 m_startFrame;               // +0x18
    s16 m_endFrame;                 // +0x1a
};

// An animation controller on an EXItemAnimator_Anim controller stack (sizeof 0x40).
// Linked into the stack's list through its EXDListItem base (at +4).
class EXItemCtrl : public EXDListItem {
    EX_DECLARE_DYNAMIC(EXItemCtrl)
public:
    virtual ~EXItemCtrl() {}
    virtual float GetFrame() const;
    virtual void SetFrame(float f);
    virtual void SetObjectTime(float t, s8 b);
    virtual void SetObjectFrame(float f);
    virtual void SetFrameRateMult(float f);
    virtual s8 Initialise(EXItemAnimator_Anim *pAnim);
    virtual void Update(EXItemAnimator_Anim *) {}
    virtual s8 UpdateCtrlOnStack(EXItemAnimator_Anim *pAnim, EXItemCtrlStack *pStack, EXRotPosInfo *pInfo, int n);
    virtual u8 CalcMotionData(EXMotionData *pData, const EXCalcMotionTemp &t);
    virtual void *GetFrameDatum(EXItemAnimator_Anim *pAnim, u32 type, EXDatumTemp &d, u32 flags);
    virtual void GetBoundsBox(EXBoundsBox &box, u32 flags);
    EXItemCtrl() {}
    EXItemCtrl(u32 hash);
    static EXItemCtrl *CreateObject();
    EXItemCtrl *operator=(const EXItemCtrl *p);
    ex::EXFlag<u32> &CtrlFlags() { return m_ctrlFlags; }
    float Frame() const { return m_frame; }
    ex::EXTagPtr<float> &Blend() { return m_blend; }
    void SetBlend(float f) { m_blend = f; }
    void SetBlendSpline(u32 s);
    void SetCtrlStack(EXItemCtrlStack *p) { m_pCtrlStack = p; }
    void SetFrame(const EXItemCtrl *p) { SetFrame(p->GetFrame()); }
    static s8 SetForceUpdateObjectTime(s8 b)
    {
        s8 old = ms_ForceUpdateObjectTime;
        ms_ForceUpdateObjectTime = b;
        return old;
    }
    static s8 ForceUpdateObjectTime() { return ms_ForceUpdateObjectTime; }
    void UpdateBlendFracs(float dt);
    void SetBlendRateFrames(u32 n);
    float GetSplineBlend() const;
    EXItemCtrl *Next() const { return m_pNext ? (EXItemCtrl *)((char *)m_pNext - 4) : 0; }
    EXItemCtrl *Prev() const { return m_pPrev ? (EXItemCtrl *)((char *)m_pPrev - 4) : 0; }

    EXGeoComplexRefObject m_geoRef;     // +0xc
    EXItemCtrlStack *m_pCtrlStack;      // +0x1c
    ex::EXFlag<u32> m_ctrlFlags;        // +0x20
    u32 m_blendSpline;                  // +0x24 four bytes: spline control points (x1, y1, x2, y2) * 100
    float m_frame;                      // +0x28
    float m_frameRateMult;              // +0x2c
    ex::EXTagPtr<float> m_blend;        // +0x30
    ex::EXTagPtr<float> m_blendRate;    // +0x38
private:
    static s8 ms_ForceUpdateObjectTime;
    static EXVector2 ms_BlendSpline[4];
};

// Stack of controllers blended together (sizeof 0x20). Flag bits: 1 flush, 2 selective sections.
class EXItemCtrlStack {
public:
    EXItemCtrlStack();
    void KillStack(EXItemCtrl *pFrom);
    void SetSelectiveSectionBits(const ex::EXBit128 &bits);
    void UpdateFlushStack();
    void UpdateStack();
    void SetStackSize(u8 n) { m_stackSize = n; }
    ex::EXFlag<u16> Flags() const { return m_flags; }
    void SetFlags(u16 f) { m_flags = f; }
    float Blend() const { return m_blend; }
    void SetBlend(float f) { m_blend = f; }
    const ex::EXBit128 &SelectiveSectionBits() const { return m_sectionBits; }
    EXItemCtrl *FirstCtrl() const { return m_list.Head() ? (EXItemCtrl *)((char *)m_list.Head() - 4) : 0; }
    EXItemCtrl *LastCtrl() const
    {
        EXDListItem *p = m_list.Tail();
        return p ? (EXItemCtrl *)((char *)p - 4) : 0;
    }

    EXDList m_list;                 // +0
    ex::EXFlag<u16> m_flags;        // +8
    u8 m_stackSize;                 // +0xa
    float m_blend;                  // +0xc
    ex::EXBit128 m_sectionBits;     // +0x10
    static ex::EXBit128 ms_DeformerUsedBits;
    static ex::EXBit128 ms_BoneUsedBits;
    static u32 ms_UpdateFlags;
};

// Controller playing an animation (sizeof 0x80; most code in EXItemCtrl_Anim.obj).
class EXItemCtrl_Anim : public EXItemCtrl {
    EX_DECLARE_DYNAMIC(EXItemCtrl_Anim)
public:
    virtual ~EXItemCtrl_Anim();
    virtual void SetFrame(float f);
    virtual void SetObjectTime(float t, s8 b);
    virtual void SetObjectFrame(float f);
    virtual void SetFrameRateMult(float f);
    virtual void Update(EXItemAnimator_Anim *pAnim);
    virtual s8 UpdateCtrlOnStack(EXItemAnimator_Anim *pAnim, EXItemCtrlStack *pStack, EXRotPosInfo *pInfo, int n);
    virtual u8 CalcMotionData(EXMotionData *pData, const EXCalcMotionTemp &t);
    virtual void *GetFrameDatum(EXItemAnimator_Anim *pAnim, u32 type, EXDatumTemp &d, u32 flags);
    virtual void GetBoundsBox(EXBoundsBox &box, u32 flags);
    EXItemCtrl_Anim() {}
    EXItemCtrl_Anim(EXGeoFile *pFile, u32 hash);
    static EXItemCtrl_Anim *CreateObject();
    const char *ClassName() const { return GetRuntimeClass()->m_pszClassName; }
    ex::EXTagPtr<float> &FrameDepth() { return m_frameDepth; }
    void SetFrameDepth(float f) { m_frameDepth = f; }
    void SetMotionFlags(u32 f) { m_motionFlags = (u16)f; }
    EXSoundTag &SoundTag() { return m_soundTag; }
    EXItemCtrl_Anim *operator=(const EXItemCtrl_Anim *p);
    u32 CalcFrameDepth(float &depth) const;
    void SetFromScriptAnimCmd(const EXGeoHeader *pHdr, const _GEOSCRIPT_ANIM *pCmd);
    s8 CalcFrameInfo(float &a, float &b) const;
    float CalcFrameRate() const;
    u32 GetCtrlAnimFlags() const;
    EXAnimMotionMorph &MotionFrameInfo(int i) const
    {
        if (i < 0) i = m_curMotion;
        return m_pMotionInfo[i];
    }
protected:
    void UpdateAnimSound(const EXItemAnimator_Anim *pAnim, int n, const EXAnimMotionMorph *pMotion, const EXGeoAnim *pGeoAnim);
    void UpdateAnimSounds(const EXItemAnimator_Anim *pAnim, float f);
private:
    u32 CalcMotionFlags(const EXGeoAnim *pGeoAnim, u32 flags) const;
    s8 UpdateMorphStack(const EXRotPosInfo *pInfo, const EXItemAnimator_Anim *pAnim, int n);
    void UpdateFrame(const EXItemAnimator_Anim *pAnim);
public:
    EXAnimMotionMorph m_motion;         // +0x40 used when there is a single motion
    EXAnimMotionMorph *m_pMotionInfo;   // +0x5c m_numMotions + m_numMorphs entries
    ex::EXTagPtr<float> m_frameDepth;   // +0x60
    float m_prevFrame;                  // +0x68
    u16 m_curMotion;                    // +0x6c
    u16 m_6e;
    u16 m_numFrames;                    // +0x70
    u16 m_motionFlags;                  // +0x72
    u8 m_numMotions;                    // +0x74
    u8 m_numMorphs;                     // +0x75
    s8 m_bAllocated;                    // +0x76 m_pMotionInfo is a heap block
    s8 m_direction;                     // +0x77 sign of the frame rate
    EXSoundTag m_soundTag;              // +0x78
};

// Animation controller driven by a geo script (sizeof 0x90).
class EXItemCtrl_AnimScript : public EXItemCtrl_Anim {
    EX_DECLARE_DYNAMIC(EXItemCtrl_AnimScript)
public:
    virtual void SetFrame(float f);
    virtual void SetObjectTime(float t, s8 b);
    virtual void SetObjectFrame(float f);
    virtual void SetFrameRateMult(float f);
    virtual s8 Initialise(EXItemAnimator_Anim *pAnim);
    virtual void Update(EXItemAnimator_Anim *pAnim);
    virtual ~EXItemCtrl_AnimScript();
    EXItemCtrl_AnimScript() {}
    EXItemCtrl_AnimScript(EXGeoFile *pFile, u32 hash);
    static EXItemCtrl_AnimScript *CreateObject();
    float ScriptOffset() const { return m_scriptOffset; }
    EXItemAnimator_Script *ScriptAnimator() const { return m_pScriptAnimator; }
    static u32 AnimScript_CmdCreate(EXItemAnimator_Script *pScript, EXScriptThread *pThread, EXGeoScript *pGeo, GEOSCRIPT_REFCMD *pCmd);
private:
    void UpdateScript_ProcessSceneScript(EXItemAnimator_Anim *pAnim, const EXGeoScript *pScript, float f, u32 flags);
    u32 NumAnimsInScript();
    void UpdateScript_Animation(const EXItemAnimator_Anim *pAnim, const EXGeoHeader *pHdr, GEOSCRIPT_ANIM *pCmd, u32 n);
    s8 UpdateScript_ObjectsEngine(EXItemAnimator_Anim *pAnim, const EXGeoScript *pScript, float a, float b, float c);
    void UpdateScript(EXItemAnimator_Anim *pAnim, s8 b);
    void UpdateFrame(EXItemAnimator_Anim *pAnim);
public:
    float m_scriptOffset;                   // +0x80
    float m_prevScriptOffset;               // +0x84
    u32 m_88;
    EXItemAnimator_Script *m_pScriptAnimator;   // +0x8c
};
#endif
