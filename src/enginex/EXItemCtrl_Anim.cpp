#include <math.h>
#include "EXItemAnimator_Anim.h"
#include "EXItemAnimator_Script.h"
#include "EXItemCtrl_SnapShot.h"
#include "EXGeoAnim.h"
#include "EXGeoHeader.h"
#include "EXGeoFile.h"
#include "EXGeoScript.h"
#include "EXDatum.h"
#include "EXStdFuncs.h"
#include "EXMemory.h"
#include "EXBaseApp.h"
#include "EXSoundHandle.h"
#include "EXBaseSoundManager.h"
#include "EXItemEnv.h"
#include "EXRand.h"
#include "EXItemHandler.h"

// TODO stand-in: animator pool (EXItemAnimator.obj).
class EXItemAnimatorQueue {
public:
    static void Release(EXItemAnimator *p);
    static EXItemAnimator *Allocate(u32 hash, EXItemAnimator *pOwner);
};

// Script anim command (geo script area): motions, then morphs.
struct EXScriptAnimMotion {
    u32 m_fileHash;     // +0 -1: the script's own file
    u32 m_hash;         // +4
    float m_depth;      // +8
    s16 m_start;        // +0xc
    s16 m_end;          // +0xe
};
struct EXScriptAnimMorph {
    u32 m_fileHash;     // +0
    u32 m_hash;         // +4
    s16 m_start;        // +8
    s16 m_end;          // +0xa
};
struct _GEOSCRIPT_ANIM {
    u8 m_numMotions;    // +0
    u8 m_01;
    u8 m_curMotion;     // +2
    u8 m_numMorphs;     // +3
    u8 m_04[8];
    EXScriptAnimMotion m_motions[1];    // +0xc, then m_numMorphs EXScriptAnimMorph
};

// TODO stand-in: platform nominal engine rate (EXBaseApp area).
inline unsigned int XGetENominalFrameRate() { return EXBaseApp::NominalEngineFrameRate(); }

// Anim sound event (anim xtra data 2, after a u32 count; sizeof 0x2c).
struct EXAnimSoundEvent {
    u8 m_00[0x14];
    u32 m_hash;         // +0x14
    u8 m_18[4];
    u8 m_reverb;        // +0x1c
    u8 m_1d[3];
    float m_vol;        // +0x20
    float m_range;      // +0x24
    u32 m_frame;        // +0x28
};
struct EXAnimSoundEvents {
    u32 m_count;
    EXAnimSoundEvent m_events[1];
};

// TODO stand-in: unnamed .data at 0x39a090 (default script callbacks).
extern const EXAnimatorScriptCallbacks g_DefaultScriptCallbacksStandIn;

// IsKindOf expanded in place (retail inlines the base-class walk here).
static __forceinline s8 ExIsKindOf(const EXItemAnimator *p, const EXRuntimeClass *pClass)
{
    const EXRuntimeClass *r = p->GetRuntimeClass();
    do {
        if (r == pClass) return 1;
        r = r->m_pBaseClass;
    } while (r);
    return 0;
}

static inline int ExFloatSign(const float &f)
{
    if (f >= 0.0f) {
        if (f == 0.0f) return 0;
        return 1;
    }
    return -1;
}

EXGeoHeader *EXAnimBaseInfo::GeoHeader(EXGeoHeader *pHdr) const
{
    if (m_ref.m_pGeoFile) return m_ref.m_pGeoFile->m_pGeoHeader;
    return pHdr;
}

void EXItemCtrl_Anim::SetFrame(float f)
{
    m_frame = f;
    m_prevFrame = f;
}

void EXItemCtrl_Anim::SetFrameRateMult(float f)
{
    int d = ExFloatSign(f);
    int o = ExFloatSign(m_frameRateMult);
    m_frameRateMult = f;
    if (o != d) {
        m_direction = (s8)d;
        m_prevFrame = m_frame;
    }
}

void EXItemCtrl_AnimScript::SetFrameRateMult(float f)
{
    int d = ExFloatSign(f);
    int o = ExFloatSign(m_frameRateMult);
    m_frameRateMult = f;
    if (o != d) {
        m_direction = (s8)d;
        m_prevFrame = m_frame;
        m_prevScriptOffset = m_scriptOffset;
    }
}

void EXItemCtrl_AnimScript::SetFrame(float f)
{
    m_scriptOffset = f;
    m_prevScriptOffset = f;
}

EXItemCtrl_Anim::EXItemCtrl_Anim(EXGeoFile *pFile, u32 hash) : EXItemCtrl(hash)
{
    m_geoRef.Set(pFile, hash);
    m_motion.Set(pFile, hash, 0.0f, 0, 0);
    m_pMotionInfo = &m_motion;
    m_numMotions = 1;
    m_bAllocated = 0;
    m_curMotion = 0;
    m_6e = 0;
    m_numFrames = 0;
    m_numMorphs = 0;
    FrameDepth() = 0.0f;
    m_direction = 1;
    m_motionFlags = 0;
    m_prevFrame = -1.0f;
}

EXItemCtrl_Anim::~EXItemCtrl_Anim()
{
    if (m_bAllocated && m_pMotionInfo) EXFree(m_pMotionInfo);
}

EXItemCtrl_Anim *EXItemCtrl_Anim::operator=(const EXItemCtrl_Anim *p)
{
    *(EXItemCtrl *)this = *p;
    m_frameDepth = p->m_frameDepth;
    m_numMotions = p->m_numMotions;
    m_numMorphs = p->m_numMorphs;
    m_bAllocated = p->m_bAllocated;
    m_motionFlags = p->m_motionFlags;
    m_prevFrame = p->m_prevFrame;
    if (p->m_bAllocated) {
        u32 size = (p->m_numMorphs + p->m_numMotions) * sizeof(EXAnimMotionMorph);
        m_pMotionInfo = (EXAnimMotionMorph *)_EXAlloc(size, 0);
        if (!m_pMotionInfo) while (1) {}
        memcpy(m_pMotionInfo, p->m_pMotionInfo, size);
    } else {
        m_pMotionInfo = &m_motion;
        *m_pMotionInfo = *p->m_pMotionInfo;
    }
    return this;
}

// NEAR: prologue only: retail pushes all four registers before the n <= 1 early-out.
u32 EXItemCtrl_Anim::CalcFrameDepth(float &depth) const
{
    int n = m_numMotions;
    int i = 0;
    if (n <= 1) {
        depth = 0.0f;
        return 0;
    }
    for (int j = 0; j < n; j++) {
        if (m_pMotionInfo[j].m_depth > depth) goto found;
        i = j;
    }
    depth = 0.0f;
    return i;
found:
    float range = m_pMotionInfo[i + 1].m_depth - m_pMotionInfo[i].m_depth;
    float eps = 1.1920929e-7f;
    depth = (depth - m_pMotionInfo[i].m_depth) / max(eps, range);
    depth = limit(depth, 0.0f, 1.0f);
    if (depth >= 0.999f) {
        depth = 0.0f;
        i++;
    }
    if (m_ctrlFlags.m_flags & 0x80) depth = 0.0f;
    if (i >= n - 1) return n - 1;
    return i > 0 ? i : 0;
}

u32 EXItemCtrl_Anim::CalcMotionFlags(const EXGeoAnim *pGeoAnim, u32 flags) const
{
    if (!(flags & 0x8000)) {
        if (m_ctrlFlags.m_flags & 0x100) return m_motionFlags & 0x7f00;
        flags = pGeoAnim->AnimMotionFlags();
    }
    return flags & 0x7f00;
}

void EXItemCtrl_Anim::UpdateAnimSound(const EXItemAnimator_Anim *pAnim, int dir, const EXAnimMotionMorph *pMotion, const EXGeoAnim *pGeoAnim)
{
    const EXAnimSoundEvents *pEvents = (const EXAnimSoundEvents *)pGeoAnim->XtraData(2);
    if (!pEvents) return;
    for (u32 i = 0; i < pEvents->m_count; i++) {
        const EXAnimSoundEvent *pEv = &pEvents->m_events[i];
        int n = pMotion->NumFrames();
        if (n == 0) n = pGeoAnim->m_NumFrames;
        float fn = (float)n;
        float cur = limit(m_frame, 0.0f, 1.0f) * fn + (float)pMotion->StartFrame();
        float prev = limit(m_prevFrame, 0.0f, 1.0f) * fn + (float)pMotion->StartFrame();
        float ev = (float)pEv->m_frame;
        s8 bPlay = 0;
        if (dir > 0) {
            if (prev > cur) bPlay = ev > prev || cur >= ev;
            else bPlay = ev > prev && cur >= ev;
        } else if (dir < 0) {
            if (cur > prev) bPlay = prev > ev || ev >= cur;
            else bPlay = prev > ev && ev >= cur;
        }
        if (bPlay) {
            EXVector pos = pAnim->m_mtx.GetTranslation();
            EXSoundHandle *h = m_soundTag.StartSound(pEv->m_hash, pos, pEv->m_vol, pEv->m_range, pEv->m_reverb, 0);
            if (h) h->m_Handle = 0;
        }
    }
}

EXItemCtrl_AnimScript::EXItemCtrl_AnimScript(EXGeoFile *pFile, u32 hash) : EXItemCtrl_Anim(pFile, hash)
{
    m_frame = 0.0f;
    m_scriptOffset = 0.0f;
    m_prevScriptOffset = -1.0f;
    m_88 = 0xffffffff;
    m_motion.Set(0, 0xffffffff, 0.0f, 0, 0);
    m_pScriptAnimator = 0;
}

EXItemCtrl_AnimScript::~EXItemCtrl_AnimScript()
{
    if (m_pScriptAnimator) EXItemAnimatorQueue::Release(m_pScriptAnimator);
}

void EXItemCtrl_AnimScript::SetObjectTime(float t, s8)
{
    m_scriptOffset = 0.0f;
    m_prevScriptOffset = 0.0f;
    EXGeoScriptHeader *pHdr = m_geoRef.GeoScriptHeader();
    if (pHdr) {
        EXGeoScript *pScript = pHdr->Object(0);
        if (pScript) {
            float f = t * pScript->FrameRate() / pScript->NumFrames();
            m_scriptOffset = f;
            m_prevScriptOffset = f;
        }
    }
}

void EXItemCtrl_AnimScript::SetObjectFrame(float f)
{
    m_scriptOffset = 0.0f;
    m_prevScriptOffset = 0.0f;
    EXGeoScriptHeader *pHdr = m_geoRef.GeoScriptHeader();
    if (pHdr) {
        EXGeoScript *pScript = pHdr->Object(0);
        if (pScript) {
            float o = f / pScript->NumFrames();
            m_scriptOffset = o;
            m_prevScriptOffset = o;
        }
    }
}

void EXItemCtrl_AnimScript::UpdateScript_ProcessSceneScript(EXItemAnimator_Anim *pAnim, const EXGeoScript *pScript, float f, u32 flags)
{
    if (!m_pScriptAnimator) {
        m_pScriptAnimator = (EXItemAnimator_Script *)EXItemAnimatorQueue::Allocate(0x4000000, 0);
        if (m_pScriptAnimator) {
            u32 hash = pAnim->m_geoRef.m_hashRef;
            EXGeoFile *pFile = pAnim->m_geoRef.m_pGeoFile;
            if (!m_pScriptAnimator->Initialise(pFile, hash)) {
                EXItemAnimatorQueue::Release(m_pScriptAnimator);
                m_pScriptAnimator = 0;
            }
        }
        if (m_pScriptAnimator) {
            m_pScriptAnimator->ValidCmds() = flags;
            m_pScriptAnimator->SetOwner(pAnim);
            m_ctrlFlags.BitSet(0x100000);
            if (pScript->ScriptFlags() & 2) m_ctrlFlags.BitSet(0x400000);
            pAnim->EnableStackControllerObjects(1);
            const EXAnimatorScriptCallbacks *pCallbacks = pAnim->ScriptCallbacks();
            if (pCallbacks) m_pScriptAnimator->RegisterCallbacks(pCallbacks);
            else m_pScriptAnimator->RegisterCallbacks(&g_DefaultScriptCallbacksStandIn);
        }
    }
    if (m_pScriptAnimator) m_pScriptAnimator->SetObjectFrame(f);
}

u32 EXItemCtrl_AnimScript::AnimScript_CmdCreate(EXItemAnimator_Script *pScript, EXScriptThread *pThread, EXGeoScript *pGeo, GEOSCRIPT_REFCMD *pCmd)
{
    EXItemAnimator *p = pScript->CreateThreadAnimator(pGeo, pThread, pCmd);
    if (!p) return 0;
    if ((((u32 *)pCmd)[3] & 0x7f000000) == 0xe000000 && !pThread->HasParentThread()) {
        EXItemAnimator *pOwner = (EXItemAnimator *)pScript->GetOwner();
        if (pOwner && ExIsKindOf(pOwner, EXRUNTIME_CLASS(EXItemAnimator_Anim))) {
            EXItemRender_SkinAnimSection *pSec = ((EXItemAnimator_Anim *)pOwner)->GetAnimBoneSection(((u32 *)pCmd)[3], 0);
            if (pSec) {
                p->SetRemoteMatrix(*(const EXMatrix *)((u32 *)pSec)[3]);
                p->m_renderFlags.BitSet(9);
            }
        }
    }
    return 1;
}

u32 EXItemCtrl_AnimScript::NumAnimsInScript()
{
    EXGeoScriptHeader *pHdr = m_geoRef.GeoScriptHeader();
    EXGeoScript *pScript = pHdr->Object(0);
    if (!pScript) return 0;
    u32 n = 0;
    int off = 0;
    while (1) {
        const GEOSCRIPT_CMD *pCmd = pScript->Data(off);
        switch (pCmd->m_Type) {
        case 2:
            n++;
            break;
        case 0x12:
            return n;
        }
        off += pCmd->m_Size;
    }
}

u32 EXAnimBaseInfo::CalcAnimRef(const EXGeoHeader *pHdr)
{
    m_animHash = m_ref.m_hashRef;
    if (m_animHash != 0xffffffff && (m_animHash & 0x80000000)) {
        int i = pHdr->AnimList().FindIndex(m_animHash, sizeof(EXGeoAnimHeader));
        if (i != -1) m_animHash = (m_animHash & 0xff000000) | i | 0x80000000;
    }
    return m_animHash;
}

EXGeoBaseAnim *EXAnimBaseInfo::GeoAnim(const EXGeoHeader *pHdr)
{
    if (m_ref.m_pGeoFile) return m_ref.GeoAnim();
    if (pHdr) {
        u32 hash = m_animHash;
        if (hash == 0xffffffff) hash = CalcAnimRef(pHdr);
        return pHdr->GeoAnim(hash);
    }
    return 0;
}

void EXItemCtrl_Anim::SetObjectTime(float t, s8 b)
{
    if (b) m_prevFrame = m_frame;
    else m_frame = 0.0f;
    EXGeoHeader *pHdr = m_geoRef.m_pGeoFile->GeoHeader();
    if (pHdr) {
        EXAnimMotionMorph *pM = &m_pMotionInfo[m_curMotion];
        EXGeoBaseAnim *pAnim = pM->GeoAnim(pHdr);
        if (pAnim) {
            float n = (float)pM->NumFrames();
            if (n <= 0.0f) n = (float)pAnim->m_NumFrames;
            m_frame = (float)pAnim->FrameRate() / n * t;
        }
    }
    if (!b) m_prevFrame = m_frame;
}

void EXItemCtrl_Anim::SetObjectFrame(float f)
{
    m_frame = 0.0f;
    EXGeoHeader *pHdr = m_geoRef.m_pGeoFile->GeoHeader();
    if (pHdr) {
        EXAnimMotionMorph *pM = &m_pMotionInfo[m_curMotion];
        EXGeoBaseAnim *pAnim = pM->GeoAnim(pHdr);
        if (pAnim) {
            float n = (float)pM->NumFrames();
            if (n <= 0.0f) n = (float)pAnim->m_NumFrames;
            m_frame = f / n;
        }
    }
    m_prevFrame = m_frame;
}

// NEAR: register allocation in the inlined GeoAnim tail (frames/pHdr swap ebx/ebp).
void EXItemCtrl_Anim::SetFromScriptAnimCmd(const EXGeoHeader *pHdr, const _GEOSCRIPT_ANIM *pCmd)
{
    int n = 0;
    if (m_bAllocated && m_pMotionInfo) EXFree(m_pMotionInfo);
    int total = pCmd->m_numMotions + pCmd->m_numMorphs;
    if (total <= 1) {
        m_pMotionInfo = &m_motion;
        m_bAllocated = 0;
    } else {
        m_pMotionInfo = (EXAnimMotionMorph *)_EXAlloc(total * sizeof(EXAnimMotionMorph), 0);
        if (!m_pMotionInfo) while (1) {}
        m_bAllocated = 1;
    }
    m_numMotions = 0;
    int i;
    for (i = 0; i < pCmd->m_numMotions; i++) {
        const EXScriptAnimMotion *pM = &pCmd->m_motions[i];
        EXGeoFile *pFile = pHdr->GeoFile();
        if (pM->m_fileHash != 0xffffffff) {
            pFile = EXGeoFile::GetGeoFile(pM->m_fileHash);
            if (!pFile) continue;
        }
        m_pMotionInfo[n++].Set(pFile, pM->m_hash, pM->m_depth, pM->m_start, pM->m_end);
        m_numMotions++;
    }
    m_numMorphs = 0;
    const EXScriptAnimMorph *pMorphs = (const EXScriptAnimMorph *)&pCmd->m_motions[pCmd->m_numMotions];
    for (i = 0; i < pCmd->m_numMorphs; i++) {
        const EXScriptAnimMorph *pM = &pMorphs[i];
        EXGeoFile *pFile = pHdr->GeoFile();
        if (pM->m_fileHash != 0xffffffff) {
            pFile = EXGeoFile::GetGeoFile(pM->m_fileHash);
            if (!pFile) continue;
        }
        m_pMotionInfo[n++].Set(pFile, pM->m_hash, 0.0f, pM->m_start, pM->m_end);
        m_numMorphs++;
    }
    int last = m_numMotions;
    int cur = pCmd->m_curMotion;
    m_curMotion = (u16)min(cur, last);
    EXAnimMotionMorph &m = MotionFrameInfo(-1);
    int frames = m.NumFrames();
    if (frames <= 0) {
        EXGeoBaseAnim *pAnim = m.GeoAnim(pHdr);
        if (pAnim) {
            m_numFrames = pAnim->m_NumFrames;
            return;
        }
    }
    m_numFrames = (u16)frames;
}

// NEAR: blend block reads pAnim's frame rate before frames in retail (xmm allocation differs).
s8 EXItemCtrl_Anim::CalcFrameInfo(float &frames, float &rate) const
{
    frames = 0.0f;
    rate = m_frameRateMult;
    EXGeoHeader *pHdr = m_geoRef.m_pGeoFile->GeoHeader();
    if (pHdr) {
        float depth = m_frameDepth;
        u32 i = CalcFrameDepth(depth);
        EXAnimMotionMorph *pM = &m_pMotionInfo[i];
        EXGeoBaseAnim *pAnim = pM->GeoAnim(pHdr);
        if (pAnim) {
            frames = (float)pM->NumFrames();
            if (frames <= 0.0f) frames = (float)pAnim->m_NumFrames;
            rate = (float)pAnim->FrameRate();
            if (depth != 0.0f) {
                EXAnimMotionMorph *pN = &m_pMotionInfo[i + 1];
                EXGeoBaseAnim *pAnim2 = pN->GeoAnim(pHdr);
                if (pAnim2) {
                    float n = (float)pN->NumFrames();
                    if (n <= 0.0f) n = (float)pAnim2->m_NumFrames;
                    float f = frames;
                    float d = depth;
                    frames = (n - f) * d + f;
                    float r = rate;
                    rate = ((float)pAnim->FrameRate() - r) * d + r;
                }
            }
            rate = rate / XGetENominalFrameRate() * m_frameRateMult;
            return 1;
        }
    }
    return 0;
}

float EXItemCtrl_Anim::CalcFrameRate() const
{
    float rate = 1.0f;
    EXGeoHeader *pHdr = m_geoRef.m_pGeoFile->GeoHeader();
    if (pHdr) {
        float depth = m_frameDepth;
        u32 i = CalcFrameDepth(depth);
        EXAnimMotionMorph *pM = &m_pMotionInfo[i];
        EXGeoBaseAnim *pAnim = pM->GeoAnim(pHdr);
        if (pAnim) {
            rate = (float)pAnim->FrameRate();
            if (depth != 0.0f) {
                EXAnimMotionMorph *pN = &m_pMotionInfo[i + 1];
                EXGeoBaseAnim *pAnim2 = pN->GeoAnim(pHdr);
                if (pAnim2) rate = (float)pAnim2->FrameRate() * depth + (1.0f - depth) * rate;
            }
            float nom = (float)XGetENominalFrameRate();
            float inv = nom != 0.0f ? 1.0f / nom : 0.0f;
            rate = inv * rate;
        }
    }
    return rate;
}

void EXItemCtrl_Anim::UpdateAnimSounds(const EXItemAnimator_Anim *pAnim, float dt)
{
    float d = ForceUpdateObjectTime() ? dt : m_frame - m_prevFrame;
    int dir = ExFloatSign(d);
    if (!dir) return;
    EXGeoHeader *pHdr = m_geoRef.m_pGeoFile->GeoHeader();
    if (pHdr) {
        float depth = m_frameDepth;
        u32 i = CalcFrameDepth(depth);
        EXAnimMotionMorph *pM = &m_pMotionInfo[i];
        EXGeoBaseAnim *pGeoAnim = pM->GeoAnim(pHdr);
        if (pGeoAnim && pGeoAnim->HasXtraData(2)) UpdateAnimSound(pAnim, dir, pM, (const EXGeoAnim *)pGeoAnim);
    }
}

u32 EXItemCtrl_Anim::GetCtrlAnimFlags() const
{
    u32 flags = 0;
    EXGeoHeader *pHdr = m_geoRef.m_pGeoFile->GeoHeader();
    if (pHdr) {
        for (int i = 0; i < m_numMotions; i++) {
            EXAnimMotionMorph *p = &m_pMotionInfo[i];
            EXGeoBaseAnim *pAnim = p->GeoAnim(pHdr);
            if (pAnim) flags |= pAnim->Flags();
        }
    }
    return flags;
}

// TODO stand-ins: script controller evaluation (EXItemAnimator_ScriptControllers.obj).
struct EXGeoScriptControllerHeader;
struct EXScriptController {
    u32 m_00;
    u32 m_key;          // +4 current key
    static float UpdateThread_TimeControllers(const EXGeoScriptControllerHeader *pHdr, EXScriptController *pCtrl, float t, u32 flags);
};
// Thread controller block of a script thread: which time controllers it has, their parameter, then
// the controllers (8 bytes each).
struct EXGeoScriptThreadControllers {
    u32 m_00;
    u32 m_Flags;        // +4 bit 0, 1: time controllers present
    u32 m_Param;        // +8
    u8 m_Ctrls[0x10];   // +0xc
};

// Script anim command as stored in the script: command header, then the anim data.
struct GEOSCRIPT_ANIM {
    u8 m_00[0x10];
    _GEOSCRIPT_ANIM m_anim;     // +0x10
};

void EXItemCtrl_AnimScript::UpdateScript_Animation(const EXItemAnimator_Anim *pAnim, const EXGeoHeader *pHdr, GEOSCRIPT_ANIM *pCmd, u32 n)
{
    if (m_pMotionInfo->AnimRef() != pCmd->m_anim.m_motions[0].m_hash) {
        m_88 = n;
        SetFromScriptAnimCmd(pHdr, &pCmd->m_anim);
    }
    m_curMotion = pCmd->m_anim.m_curMotion;
    m_ctrlFlags.BitState(0x80, pCmd->m_anim.m_01 & 1);
}

void EXItemCtrl_Anim::UpdateFrame(const EXItemAnimator_Anim *pAnim)
{
    float rate;
    float frames;
    if (!CalcFrameInfo(frames, rate)) return;
    u32 flags = m_ctrlFlags;
    float dt = pAnim->m_frameRateMult * EXBaseApp::EngineRenderFrameRateMul();
    if (!(flags & 0x10000)) dt *= rate;
    if (dt != 0.0f) {
        int d = dt >= 0.0f ? 1 : -1;
        if (d != m_direction) m_direction = (s8)d;
    }
    s8 bEnd = 0;
    if (!ForceUpdateObjectTime()) m_prevFrame = m_frame;
    float f = dt / frames + m_frame;
    if (f >= 0.999f) {
        if (flags & 1) {
            f -= 1.0f;
            m_ctrlFlags.BitSet(0x400);
        } else {
            m_ctrlFlags.BitSet(0x20000);
            f = 0.999f;
        }
        bEnd = (m_ctrlFlags >> 4) & 1;
    } else if (0.0f > f) {
        if (flags & 1) {
            f += 1.0f;
            m_ctrlFlags.BitSet(0x400);
        } else {
            f = 0.0f;
        }
        bEnd = (m_ctrlFlags >> 4) & 1;
    }
    m_frame = limit(f, 0.0f, 0.999f);
    if (bEnd) m_ctrlFlags.BitSet(0x40000);
    UpdateAnimSounds(pAnim, dt);
}

void EXItemCtrl_Anim::Update(EXItemAnimator_Anim *pAnim)
{
    if (!(pAnim->m_itemFlags & 8) && (m_ctrlFlags & 2)) UpdateFrame(pAnim);
}

void EXItemCtrl_AnimScript::UpdateFrame(EXItemAnimator_Anim *pAnim)
{
    EXGeoScript *pScript = m_geoRef.GeoScriptHeader()->Object(0);
    if (!pScript) return;
    float rate;
    float frames;
    if (!CalcFrameInfo(frames, rate)) return;
    u32 flags = m_ctrlFlags;
    float dt = pAnim->m_frameRateMult * EXBaseApp::EngineRenderFrameRateMul();
    if (!(flags & 0x10000)) dt *= rate;
    if (!m_numFrames) return;
    u32 nf = pScript->NumFrames();
    if (nf > 1) m_scriptOffset += m_numFrames / frames / ((float)(int)nf - 1.0f) * dt;
    s8 bEnd = 0;
    if (m_scriptOffset >= 1.0f) {
        if (flags & 1) {
            m_scriptOffset -= 1.0f;
            m_ctrlFlags.BitSet(0x400);
        } else {
            if (!(flags & 0x200)) m_scriptOffset = 1.0f;
            m_ctrlFlags.BitSet(0x20000);
        }
        bEnd = (m_ctrlFlags >> 4) & 1;
    } else if (0.0f > m_scriptOffset) {
        if (flags & 1) {
            m_scriptOffset += 1.0f;
            m_ctrlFlags.BitSet(0x400);
        } else if (!(flags & 0x200)) {
            m_scriptOffset = 0.0f;
        }
        bEnd = (m_ctrlFlags >> 4) & 1;
    }
    if (bEnd) {
        m_ctrlFlags.BitSet(0x40000);
        UpdateScript(pAnim, 0);
    }
}

s8 EXItemCtrl_AnimScript::Initialise(EXItemAnimator_Anim *pAnim)
{
    UpdateScript(pAnim, 1);
    return 1;
}

void EXItemCtrl_AnimScript::Update(EXItemAnimator_Anim *pAnim)
{
    if (!(pAnim->m_itemFlags & 8)) {
        UpdateScript(pAnim, 0);
        if (m_ctrlFlags & 2) UpdateFrame(pAnim);
    }
}

// Frame position in [0, len] from the normalised frame, snapped to the nearest half frame when within
// 0.01 of it.
static inline float SnapHalfFrame(float frame, int len)
{
    float f = frame;
    if (f >= 1.0f) f = 1.0f;
    else if (!(f > 0.0f)) f = 0.0f;
    float v = (float)len * f;
    float r = (float)fmod((double)v, 0.5);
    if (r < 0.01f) return v - r;
    if (r > 0.49f) return (v - r) + 0.5f;
    return v;
}

// Decode the morph frames of the current morph track and blend them into the stack's deformers.
s8 EXItemCtrl_Anim::UpdateMorphStack(const EXRotPosInfo *pInfo, const EXItemAnimator_Anim *pAnim, int n)
{
    EXItemRender_SkinAnimInfo info;
    info.m_blend = n ? GetSplineBlend() : 1.0f;
    info.m_depth = 0.0f;
    EXGeoHeader *pHdr = m_geoRef.m_pGeoFile ? m_geoRef.m_pGeoFile->m_pGeoHeader : 0;
    if (!pHdr) return 0;
    int len = m_numFrames;
    float frame = SnapHalfFrame(m_frame, len);
    int f = (int)frame;
    EXAnimMotionMorph *pMorph = &m_pMotionInfo[m_numMotions];
    for (int i = 0; i < m_numMorphs; i++, pMorph++) {
        int start = pMorph->m_startFrame;
        int end = pMorph->m_endFrame;
        if (f < start || f > end) continue;
        EXGeoHeader *pMorphHdr = pMorph->m_ref.m_pGeoFile ? pMorph->m_ref.m_pGeoFile->m_pGeoHeader : pHdr;
        u32 hash = pMorph->m_animHash;
        if (hash == 0xffffffff) hash = pMorph->m_ref.m_hashRef;
        EXGeoAnimHeader *pAnimHdr = pMorphHdr->AnimList().Find(hash);
        if (!pAnimHdr) continue;
        EXGeoBaseAnim *pGeoAnim = (EXGeoBaseAnim *)pAnimHdr->m_Addr[0].m_pObject;
        if (!pGeoAnim) continue;
        float frac = frame - (float)start;
        info.m_frac[0] = frac - (float)(int)frac;
        EXGeoSkinAnimFrame *pFrame = pGeoAnim->DecodeMorph(f, 0);
        if (!pFrame) return 0;
        info.m_pMorphFrame[0][0] = pFrame;
        info.m_pMorphAnim[0][0] = pGeoAnim;
        if (info.m_frac[0] > 0.001f && len > 1) {
            int g = f + 1;
            if (g >= end) g = (m_ctrlFlags & 1) ? g - len : end - 1;
            pFrame = pGeoAnim->DecodeMorph(g, 0);
            if (!pFrame) return 0;
        }
        info.m_pMorphFrame[0][1] = pFrame;
        info.m_pMorphAnim[0][1] = pGeoAnim;
        if (pAnim->m_pSkin) pAnim->m_pSkin->BuildHierData_Morph(pInfo, &info, 0, 1);
    }
    return 1;
}

// Decode the frames this controller contributes (one motion, or two when the frame depth blends
// between neighbouring motions), with their morphs, and build the stack's pose from them.
s8 EXItemCtrl_Anim::UpdateCtrlOnStack(EXItemAnimator_Anim *pAnim, EXItemCtrlStack *pStack, EXRotPosInfo *pInfo, int n)
{
    EXItemRender_SkinAnimInfo info;
    if (m_pMotionInfo->m_ref.m_hashRef == 0xffffffff || pAnim->m_geoRef.m_hashRef == 0xffffffff) return 0;
    EXItemRender_SkinAnim *pSkin = pAnim->m_pSkin;
    if (!pSkin) return 0;
    ex::EXBit128 bits;
    bits.Set(pStack->m_sectionBits);
    if ((s8)pAnim->m_lodFlags < 0) bits &= pSkin->m_pActiveLod->m_skinBits;
    info.m_blend = n ? GetSplineBlend() : 1.0f;
    info.m_depth = *m_frameDepth.m_p;
    u32 first = CalcFrameDepth(info.m_depth);
    EXGeoFile *pFile = m_geoRef.m_pGeoFile;
    if (!pFile) return 0;
    EXGeoHeader *pHdr = pFile->m_pGeoHeader;
    if (!pHdr) return 0;
    u32 lod = pAnim->m_lodFlags;
    int count;
    if (lod & 4) {
        info.m_depth = 0.0f;
        count = 1;
    } else
        count = info.m_depth > 0.001f ? 2 : 1;
    for (int k = 0; k < count; k++) {
        EXAnimMotionMorph *pMotion = &m_pMotionInfo[first + k];
        EXGeoAnimHeader *pAnimHdr = pMotion->m_ref.GeoAnimHeader();
        if (!pAnimHdr || !pAnimHdr->m_Addr[0].m_pObject) {
            GetRuntimeClass();   // debug report of the missing anim compiled out
            return 0;
        }
        if (pAnimHdr->m_1c == 0xffffffff)
            info.m_pAnimSkin = &pSkin->m_refs[3];
        else {
            info.m_animSkin.m_08 = -1;
            info.m_animSkin.m_pGeoFile = pFile;
            info.m_animSkin.m_hashRef = pAnimHdr->m_1c;
            info.m_pAnimSkin = &info.m_animSkin;
        }
        EXGeoBaseAnim *pGeoAnim = (EXGeoBaseAnim *)pAnimHdr->m_Addr[0].m_pObject;
        int start = pMotion->m_startFrame;
        int len = pMotion->m_endFrame - start;
        if (!len) len = pGeoAnim->m_NumFrames;
        int end = start + len;
        float frame = SnapHalfFrame(m_frame, len);
        int f = (int)((float)start + frame);
        if (f >= end) f = end - 1;
        info.m_frac[k] = (lod & 2) ? 0.0f : ((float)start + frame) - (float)f;
        u32 flags = ((m_ctrlFlags & 0x100) ? m_motionFlags : pGeoAnim->m_AnimMotionFlags) & 0x7f00;
        EXGeoSkinAnimFrame *pFrame = pGeoAnim->DecodeFrame(f, flags, bits);
        if (!pFrame || !pFrame->m_Decoded) return 0;
        info.m_pFrame[k][0] = pFrame;
        info.m_pAnim[k][0] = pGeoAnim;
        int g = f;
        if (info.m_frac[k] > 0.001f && len > 1) {
            g = f + 1;
            if (g >= end) g = (m_ctrlFlags & 1) ? g - len : end - 1;
            pFrame = pGeoAnim->DecodeFrame(g, flags, bits);
            if (!pFrame || !pFrame->m_Decoded) return 0;
        }
        info.m_pFrame[k][1] = pFrame;
        info.m_pAnim[k][1] = pGeoAnim;
        // Morphs: a separate morph anim, or the frames themselves.
        EXGeoBaseAnim *pMorphAnim = 0;
        EXGeoSkinAnimFrame *pMorph0 = 0;
        if (pGeoAnim->m_MorphHash != 0xffffffff &&
            ((short)pGeoAnim->m_MorphHash != (short)pGeoAnim->m_AnimIndex || pGeoAnim->m_MorphFileHash != 0xffffffff)) {
            EXGeoHeader *pMorphHdr;
            if (pGeoAnim->m_MorphFileHash == 0xffffffff)
                pMorphHdr = pMotion->GeoHeader(pHdr);
            else {
                EXGeoFile *pMorphFile = EXGeoFile::GetGeoFile(pGeoAnim->m_MorphFileHash);
                pMorphHdr = pMorphFile ? pMorphFile->m_pGeoHeader : 0;
                if (!pMorphFile) pMorphHdr = 0;
            }
            if (pMorphHdr && (pMorphAnim = (EXGeoBaseAnim *)pMorphHdr->GeoAnim(pGeoAnim->m_MorphHash)) != 0)
                pMorph0 = pMorphAnim->DecodeMorph(f, flags);
        }
        if (pMorph0) {
            info.m_pMorphFrame[k][0] = pMorph0;
            info.m_pMorphAnim[k][0] = pMorphAnim;
            info.m_pMorphFrame[k][1] = f != g ? pMorphAnim->DecodeMorph(g, flags) : pMorph0;
            info.m_pMorphAnim[k][1] = pMorphAnim;
        } else {
            for (int i = 0; i < 2; i++) {
                info.m_pMorphFrame[k][i] = info.m_pFrame[k][i];
                info.m_pMorphAnim[k][i] = info.m_pAnim[k][i];
            }
        }
    }
    s8 r = count == 1 ? pSkin->BuildHierData_Motion(pInfo, &info, bits) : pSkin->BuildHierData2_Motion(pInfo, &info, bits);
    if (r) EXItemCtrlStack::ms_UpdateFlags |= 0xc000;
    if (m_numMorphs) UpdateMorphStack(pInfo, pAnim, n);
    if (lod & 8) pSkin->BuildHierData_ModifyMorph(pInfo, 0.0f);
    return r;
}

// Bounds of the current frame: each motion's anim bounds moved by its root motion, blended by the
// frame depth when two motions are active. Flags bit 1: accumulate into box.
void EXItemCtrl_Anim::GetBoundsBox(EXBoundsBox &box, u32 flags)
{
    MXBoundsBox bounds[2];
    int found = 0;
    float depth;
    EXGeoFile *pFile = m_geoRef.m_pGeoFile;
    EXGeoHeader *pHdr;
    if (pFile && (pHdr = pFile->m_pGeoHeader) != 0) {
        depth = *m_frameDepth.m_p;
        u32 first = CalcFrameDepth(depth);
        int count = depth <= 0.001f ? 1 : 2;
        MXBoundsBox *pOut = bounds;
        for (int k = 0; k < count; k++, pOut++) {
            EXAnimMotionMorph *pMotion = &m_pMotionInfo[first + k];
            EXGeoBaseAnim *pGeoAnim = pMotion->GeoAnim(pHdr);
            if (!pGeoAnim) continue;
            int start = pMotion->m_startFrame;
            int len = pMotion->m_endFrame - start;
            if (!len) len = pGeoAnim->m_NumFrames;
            float frame = SnapHalfFrame(m_frame, len);
            u32 motionFlags = (m_ctrlFlags & 0x100) ? m_motionFlags : pGeoAnim->m_AnimMotionFlags;
            EXMotionData m;
            pGeoAnim->DecompressMotion((int)((float)start + frame), m, 8, 0x3f00 - (motionFlags & 0x3f00));
            if (motionFlags & 0x4000) {
                const EXMotionData *pStrip = (const EXMotionData *)pGeoAnim->m_XtraData.GetXtra(1);
                if (pStrip) pGeoAnim->StripMotionData(*pStrip, m.m_pos, m.m_rot, motionFlags & 0x7f00, 1);
            }
            EXMaths::mat_44 mtx;
            mtx.set_rotate_quat(m.m_rot);
            mtx.r[3].x = m.m_pos.x;
            mtx.r[3].y = m.m_pos.y;
            mtx.r[3].z = m.m_pos.z;
            mtx.r[3].w = 1.0f;
            const EXBoundsBox &b = pGeoAnim->GetBoundsBox();
            pOut->m_vctMin = b.m_vctMin;
            pOut->m_vctMax = b.m_vctMax;
            pOut->_ApplyXForm(mtx);
            found++;
        }
    }
    if (found == 2) {
        float t = depth;
        bounds[0].m_vctMax.x = (bounds[1].m_vctMax.x - bounds[0].m_vctMax.x) * t + bounds[0].m_vctMax.x;
        bounds[0].m_vctMax.y = (bounds[1].m_vctMax.y - bounds[0].m_vctMax.y) * t + bounds[0].m_vctMax.y;
        bounds[0].m_vctMax.z = (bounds[1].m_vctMax.z - bounds[0].m_vctMax.z) * t + bounds[0].m_vctMax.z;
        bounds[0].m_vctMax.w = (bounds[1].m_vctMax.w - bounds[0].m_vctMax.w) * t + bounds[0].m_vctMax.w;
        bounds[0].m_vctMin.x = (bounds[1].m_vctMin.x - bounds[0].m_vctMin.x) * t + bounds[0].m_vctMin.x;
        bounds[0].m_vctMin.y = (bounds[1].m_vctMin.y - bounds[0].m_vctMin.y) * t + bounds[0].m_vctMin.y;
        bounds[0].m_vctMin.z = (bounds[1].m_vctMin.z - bounds[0].m_vctMin.z) * t + bounds[0].m_vctMin.z;
        bounds[0].m_vctMin.w = (bounds[1].m_vctMin.w - bounds[0].m_vctMin.w) * t + bounds[0].m_vctMin.w;
    } else if (found != 1) {
        bounds[0].m_vctMin.set_zero();
        bounds[0].m_vctMax.set_zero();
    }
    if (!(flags & 1)) {
        box.m_vctMin = bounds[0].m_vctMin;
        box.m_vctMax = bounds[0].m_vctMax;
    } else
        box.Accumulate(*(const EXBoundsBox *)&bounds[0]);
}

// Datum `id` at the current frame from the anim's datum tracks (or, with flags bit 2, from the
// skeleton), blended by the frame depth over two motions. Flags bit 1: apply the motion flags
// (bit 15 set: use flags' own 0x7f00 bits). Returns the datum record found, or 0.
void *EXItemCtrl_Anim::GetFrameDatum(EXItemAnimator_Anim *pAnim, u32 id, EXDatumTemp &d, u32 flags)
{
    EXGeoFile *pFile = m_geoRef.m_pGeoFile;
    EXGeoHeader *pHdr;
    if (!pFile || !(pHdr = pFile->m_pGeoHeader)) return 0;
    float depth = *m_frameDepth.m_p;
    void *pResult = 0;
    bool found = false;
    u32 first = CalcFrameDepth(depth);
    u32 lod = pAnim->m_lodFlags;
    int count;
    if (lod & 4) {
        depth = 0.0f;
        count = 1;
    } else
        count = depth > 0.001f ? 2 : 1;
    EXMotionData motion[2];
    int n = 0;
    u16 dFlags;
    u8 dType;
    u8 dFrameFlags;
    float dSize[3];
    for (int k = 0; k < count; k++) {
        EXAnimMotionMorph *pMotion = &m_pMotionInfo[first + k];
        EXGeoBaseAnim *pGeoAnim = pMotion->GeoAnim(pHdr);
        if (!pGeoAnim) return 0;
        u32 numDatums = pGeoAnim->m_DatumCount;
        EXGeoAnimDatums *pTable = pGeoAnim->m_DatumData;
        EXGeoAnimDatum *pDatum = (EXGeoAnimDatum *)((u8 *)pTable + 4);
        u32 i;
        for (i = 0; i < numDatums; i++, pDatum++)
            if (pDatum->m_Id == id) break;
        if (i < numDatums) {
            int start = pMotion->m_startFrame;
            int len = pMotion->m_endFrame - start;
            if (!len) len = pGeoAnim->m_NumFrames;
            int end = len + start;
            float frame = (float)start + SnapHalfFrame(m_frame, len);
            u32 motionFlags = 0;
            int f = (int)frame;
            if (flags & 1) {
                motionFlags = flags;
                if (!(flags & 0x8000)) motionFlags = (m_ctrlFlags & 0x100) ? m_motionFlags : pGeoAnim->m_AnimMotionFlags;
                motionFlags &= 0x7f00;
            }
            s8 ok;
            if ((lod & 2) || frame - (float)f <= 0.0f || f + 1 >= end)
                ok = pGeoAnim->DecompressDatum(f, i, motion[n], motionFlags);
            else
                ok = pGeoAnim->DecompressDatumInterp(frame, i, motion[n], motionFlags);
            if (ok) {
                n++;
                if (!pResult) {
                    dType = pDatum->m_Type;
                    dFlags = pDatum->m_Flags;
                    dFrameFlags = 0;
                    dSize[0] = pDatum->m_Size[0];
                    dSize[1] = pDatum->m_Size[1];
                    dSize[2] = pDatum->m_Size[2];
                    found = true;
                }
                pResult = pDatum;
                u8 *pFrameFlags = pDatum->m_FrameFlags;
                if (pFrameFlags) dFrameFlags |= pFrameFlags[f];
            }
            continue;
        }
        if (flags & 2) {
            void *pCtx = 0;
            EXDatumTemp skel;
            void *p = pAnim->GetSkelDatum(id, skel, 4, pCtx);
            if (p) {
                motion[n].m_pos = skel.m_position;
                motion[n].m_rot = skel.m_rotation;
                n++;
                found = true;
                pResult = p;
            }
        }
    }
    if (!found) return pResult;
    if (n == 1) {
        d.m_position = motion[0].m_pos;
        d.m_rotation = motion[0].m_rot;
    } else {
        EXVector p;
        p.x = (motion[1].m_pos.x - motion[0].m_pos.x) * depth + motion[0].m_pos.x;
        p.y = (motion[1].m_pos.y - motion[0].m_pos.y) * depth + motion[0].m_pos.y;
        p.z = (motion[1].m_pos.z - motion[0].m_pos.z) * depth + motion[0].m_pos.z;
        p.w = (motion[1].m_pos.w - motion[0].m_pos.w) * depth + motion[0].m_pos.w;
        d.m_position = p;
        EXMaths::quat_xyzw q = motion[0].m_rot.slerp(motion[1].m_rot, depth);
        d.m_rotation.x = q.x;
        d.m_rotation.y = q.y;
        d.m_rotation.z = q.z;
        d.m_rotation.w = q.w;
    }
    d.m_position.w = 1.0f;
    d.m_type = dType;
    d.m_2c = dFlags;
    d.m_2e = dFrameFlags;
    d.m_size.x = dSize[0];
    d.m_size.y = dSize[1];
    d.m_size.z = dSize[2];
    return pResult;
}

// Hamilton product a * b.
static inline EXMaths::quat_xyzw QuatMulHamilton(const EXMaths::quat_xyzw &a, const EXMaths::quat_xyzw &b)
{
    EXMaths::quat_xyzw r;
    r.x = a.x * b.w + a.w * b.x + a.y * b.z - a.z * b.y;
    r.y = a.y * b.w + a.w * b.y + a.z * b.x - a.x * b.z;
    r.z = a.z * b.w + a.w * b.z + a.x * b.y - a.y * b.x;
    r.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
    return r;
}

static inline void SetIdentityMotion(EXMotionData &m)
{
    m.m_pos.y = 0.0f;
    m.m_pos.x = 0.0f;
    m.m_pos.z = 0.0f;
    m.m_pos.w = 0.0f;
    m.m_rot.w = 0.0f;
    m.m_rot.z = 0.0f;
    m.m_rot.y = 0.0f;
    m.m_rot.w = 1.0f;
    m.m_rot.x = 0.0f;
}

// Root motion of this controller for up to two motion ids of the request: 1 the previous step,
// 2 the current frame, 3 the next step (the step is the anim frame rate scaled by the request's
// rate). Past the last frame the motion continues from the anim's total motion (looping anims) or
// eases out. Blended over the two frame depth motions. Returns 4, or 8 if an anim is missing.
u8 EXItemCtrl_Anim::CalcMotionData(EXMotionData *pData, const EXCalcMotionTemp &t)
{
    int nMotions = 2 - ((t.m_flags & 0x80) != 0);
    int filled = 0;
    for (int i = 0; i < nMotions; i++) {
        pData[i].m_pos.x = 0.0f;
        pData[i].m_pos.y = 0.0f;
        pData[i].m_pos.z = 0.0f;
        pData[i].m_pos.w = 0.0f;
        pData[i].m_rot.x = 0.0f;
        pData[i].m_rot.y = 0.0f;
        pData[i].m_rot.z = 0.0f;
        pData[i].m_rot.w = 1.0f;
        filled = nMotions;
    }
    EXGeoFile *pFile = m_geoRef.m_pGeoFile;
    EXGeoHeader *pHdr;
    if (!pFile || !(pHdr = pFile->m_pGeoHeader)) return 4;
    float depth = *m_frameDepth.m_p;
    u32 first = CalcFrameDepth(depth);
    int count;
    if (t.m_lod & 4) {
        depth = 0.0f;
        count = 1;
    } else
        count = depth > 0.001f ? 2 : 1;
    EXMotionData tmp[2][2];     // [motion id][frame depth motion]
    for (int k = 0; k < count; k++) {
        EXAnimMotionMorph *pMotion = &m_pMotionInfo[first + k];
        EXGeoAnimHeader *pAnimHdr;
        if (!pMotion->m_ref.m_pGeoFile) {
            u32 hash = pMotion->m_animHash;
            if (hash == 0xffffffff) {
                hash = pMotion->m_ref.m_hashRef;
                pMotion->m_animHash = hash;
                if (hash != 0xffffffff && (hash & 0x80000000)) {
                    int i = pHdr->AnimList().FindIndex(hash, sizeof(EXGeoAnimHeader));
                    if (i != -1) pMotion->m_animHash = (pMotion->m_animHash & 0xff000000) | i | 0x80000000;
                }
                hash = pMotion->m_animHash;
            }
            int i = pHdr->AnimList().FindIndex(hash, sizeof(EXGeoAnimHeader));
            if (i < 0) return 8;
            pAnimHdr = &pHdr->AnimList().Data()[i];
        } else
            pAnimHdr = pMotion->m_ref.GeoAnimHeader();
        EXGeoBaseAnim *pGeoAnim;
        if (!pAnimHdr || !(pGeoAnim = (EXGeoBaseAnim *)pAnimHdr->m_Addr[0].m_pObject)) return 8;
        u32 mflags = t.m_flags;
        if (!(mflags & 0x8000)) mflags = (m_ctrlFlags & 0x100) ? m_motionFlags : pGeoAnim->m_AnimMotionFlags;
        mflags &= 0x7f00;
        int start = pMotion->m_startFrame;
        int len = pMotion->m_endFrame - start;
        if (!len) len = pGeoAnim->m_NumFrames;
        int end = start + len;
        float last = (float)(end - 1);
        float flen = (float)len;
        float cur = SnapHalfFrame(m_frame, len);
        float step = ((float)pGeoAnim->m_FrameRate / (float)XGetENominalFrameRate()) * t.m_rate;
        float stepLimit = last - step;
        if (!(t.m_flags & 0x80)) step = EXBaseApp::EngineRenderFrameRateMul() * step;
        float ease = 0.0f;
        filled = 0;
        for (int j = 0; j < nMotions; j++, filled++) {
            u32 id = t.m_ids[j];
            if (!id) break;
            u32 loop = m_ctrlFlags & 1;
            if (loop && (t.m_flags & 0x40)) id++;
            EXMotionData &m = tmp[j][k];
            if (!mflags) {
                SetIdentityMotion(m);
                continue;
            }
            float f = cur;
            if (id == 3) {
                ease = step;
                f = cur + step;
                stepLimit = last;
            } else if (id == 1) {
                ease = step;
                f = cur - step;
                if (f < 0.0f) {
                    SetIdentityMotion(m);
                    f = 0.0f;
                }
            }
            float base = (float)start;
            float absF = base + f;
            if (absF < 0.0f) {
                // Before the start: ease the motion out to identity.
                pGeoAnim->DecompressMotionInterp(absF, m, 4, mflags);
                EXVector p;
                p.x = (0.0f - m.m_pos.x) * ease + m.m_pos.x;
                p.y = (0.0f - m.m_pos.y) * ease + m.m_pos.y;
                p.z = (0.0f - m.m_pos.z) * ease + m.m_pos.z;
                p.w = (0.0f - m.m_pos.w) * ease + m.m_pos.w;
                m.m_pos = p;
                EXQuaternion identity(0.0f, 0.0f, 0.0f, 1.0f);
                EXMaths::quat_xyzw q = m.m_rot.slerp(identity, ease);
                m.m_rot.x = q.x;
                m.m_rot.y = q.y;
                m.m_rot.z = q.z;
                m.m_rot.w = q.w;
            } else {
                if (!loop) {
                    if (absF < last) {
                        if (absF > stepLimit) absF = stepLimit;
                    } else if (absF > last)
                        absF = last;
                }
                if (absF > last) {
                    float fend = (float)end;
                    EXMotionData total, edge;
                    pGeoAnim->GetTotalMotion(total);
                    pGeoAnim->DecompressMotion_StripMotion(total, 4, mflags);
                    if (absF < fend) {
                        // Between the last frame and the end: from the last frame towards the total.
                        pGeoAnim->DecompressMotion(end - 1, edge, 4, mflags);
                        float u = fend - absF;
                        edge.m_pos.w = 0.0f;
                        EXVector p;
                        p.x = (edge.m_pos.x - total.m_pos.x) * u + total.m_pos.x;
                        p.y = (edge.m_pos.y - total.m_pos.y) * u + total.m_pos.y;
                        p.z = (edge.m_pos.z - total.m_pos.z) * u + total.m_pos.z;
                        p.w = (0.0f - total.m_pos.w) * u + total.m_pos.w;
                        m.m_pos = p;
                        EXMaths::quat_xyzw q = total.m_rot.slerp(edge.m_rot, u);
                        m.m_rot.x = q.x;
                        m.m_rot.y = q.y;
                        m.m_rot.z = q.z;
                        m.m_rot.w = q.w;
                    } else {
                        // Wrapped: the motion of a full loop, relative to the first frame, plus the
                        // wrapped frame.
                        pGeoAnim->DecompressMotion(0, edge, 4, mflags);
                        total.m_pos.x = total.m_pos.x - edge.m_pos.x;
                        total.m_pos.y = total.m_pos.y - edge.m_pos.y;
                        total.m_pos.z = total.m_pos.z - edge.m_pos.z;
                        total.m_pos.w = total.m_pos.w - edge.m_pos.w;
                        EXMaths::quat_xyzw inv;
                        inv.x = 0.0f - edge.m_rot.x;
                        inv.y = 0.0f - edge.m_rot.y;
                        inv.z = 0.0f - edge.m_rot.z;
                        inv.w = edge.m_rot.w;
                        EXMaths::quat_xyzw r = QuatMulHamilton(total.m_rot, inv);
                        total.m_rot.x = r.x;
                        total.m_rot.y = r.y;
                        total.m_rot.z = r.z;
                        total.m_rot.w = r.w;
                        float wrapped = flen != 0.0f ? (float)fmod((double)absF, (double)flen) : 0.0f;
                        float frame = base + wrapped;
                        pGeoAnim->DecompressMotionInterp(frame, m, 4, mflags);
                        int loops = (int)(absF - (float)(int)absF);   // retail: always below 1
                        if (loops < 1) loops = 1;
                        while (loops-- > 0) {
                            m.m_pos.x = total.m_pos.x + m.m_pos.x;
                            m.m_pos.y = total.m_pos.y + m.m_pos.y;
                            m.m_pos.z = total.m_pos.z + m.m_pos.z;
                            m.m_pos.w = total.m_pos.w + m.m_pos.w;
                            EXMaths::quat_xyzw q = QuatMulHamilton(m.m_rot, total.m_rot);
                            m.m_rot.x = q.x;
                            m.m_rot.y = q.y;
                            m.m_rot.z = q.z;
                            m.m_rot.w = q.w;
                        }
                    }
                } else
                    pGeoAnim->DecompressMotionInterp(absF, m, 4, mflags);
            }
            m.m_pos.w = 0.0f;
        }
    }
    for (int j = 0; j < filled; j++) {
        if (count == 1) {
            pData[j].m_pos = tmp[j][0].m_pos;
            pData[j].m_rot = tmp[j][0].m_rot;
        } else {
            EXVector p;
            p.x = (tmp[j][1].m_pos.x - tmp[j][0].m_pos.x) * depth + tmp[j][0].m_pos.x;
            p.y = (tmp[j][1].m_pos.y - tmp[j][0].m_pos.y) * depth + tmp[j][0].m_pos.y;
            p.z = (tmp[j][1].m_pos.z - tmp[j][0].m_pos.z) * depth + tmp[j][0].m_pos.z;
            p.w = (tmp[j][1].m_pos.w - tmp[j][0].m_pos.w) * depth + tmp[j][0].m_pos.w;
            pData[j].m_pos = p;
            EXMaths::quat_xyzw q = tmp[j][0].m_rot.slerp(tmp[j][1].m_rot, depth);
            pData[j].m_rot.x = q.x;
            pData[j].m_rot.y = q.y;
            pData[j].m_rot.z = q.z;
            pData[j].m_rot.w = q.w;
        }
    }
    return 4;
}

// Advance the script time by its object controllers (unless `init`), then find the anim command
// active at the current script frame, switch to it and set this controller's frame from the
// script time (through the command's time controllers and the anim / script frame rate ratio).
void EXItemCtrl_AnimScript::UpdateScript(EXItemAnimator_Anim *pAnim, s8 init)
{
    EXGeoScriptHeader *pScriptHdr = m_geoRef.GeoScriptHeader();
    if (!pScriptHdr) return;
    EXGeoScript *pScript = pScriptHdr->Object(0);
    if (!pScript) return;
    float rate = pAnim->m_frameRateMult * m_frameRateMult * EXBaseApp::EngineRenderFrameRateMul() * CalcFrameRate();
    if (rate != 0.0f) {
        s8 dir = rate < 0.0f ? -1 : 1;
        if (dir != m_direction) {
            m_direction = dir;
            m_prevFrame = m_frame;
            m_prevScriptOffset = m_scriptOffset;
        }
    }
    int numFrames = pScript->m_NumFrames;
    float pos;
    int tries = 0;
    do {
        float o = m_scriptOffset;
        if (!(o < 1.0f)) o = 1.0f;
        else if (!(o > 0.0f)) o = 0.0f;
        pos = o * (float)numFrames;
    } while (!init && (pScript->m_UsedControllerTypes & 0x11cf8) &&
             !UpdateScript_ObjectsEngine(pAnim, pScript, rate, m_prevScriptOffset * (float)numFrames, pos) && ++tries < 2);

    // The last anim command at or before the current frame.
    u32 off = 0;
    int animOff = -1;
    float animFrame = -1.0f;
    bool done = false;
    do {
        const GEOSCRIPT_CMD *pCmd = pScript->Data(off);
        float frame = (float)(short)pCmd->m_04;
        if (pCmd->m_Type == 2) {
            if (pos < frame) done = true;
            else {
                animOff = off;
                animFrame = frame;
            }
        } else if (pCmd->m_Type == 0x12)
            done = true;
        off += pCmd->m_Size;
    } while (!done);

    if (animOff >= 0) {
        EXGeoHeader *pHdr = m_geoRef.m_pGeoFile ? m_geoRef.m_pGeoFile->m_pGeoHeader : 0;
        GEOSCRIPT_ANIM *pCmd = (GEOSCRIPT_ANIM *)pScript->Data(animOff);
        if (m_88 != (u32)animOff) UpdateScript_Animation(pAnim, pHdr, pCmd, animOff);
        EXGeoBaseAnim *pGeoAnim = m_pMotionInfo[m_curMotion].GeoAnim(pHdr);
        float t = pos - animFrame;
        float frames = (float)m_numFrames;
        ex::EXRELOFFSET32 *pTable = pScript->m_ThreadControllerTable;
        ex::EXRELOFFSET32 *pEntry = &pTable[((GEOSCRIPT_CMD *)pCmd)->m_Thread];
        EXGeoScriptThreadControllers *pCtrls = (EXGeoScriptThreadControllers *)ex::EXRelOffset(*pEntry);
        u32 ctrlFlags = pCtrls->m_Flags;
        if (ctrlFlags & 3) {
            EXScriptController state[2];
            state[0].m_00 = 0;
            state[0].m_key = 0;
            const u8 *pHdrCtrl = pCtrls->m_Ctrls;
            u32 param = pCtrls->m_Param;
            state[1].m_00 = 0;
            state[1].m_key = 0;
            if (ctrlFlags & 1) {
                t = EXScriptController::UpdateThread_TimeControllers((const EXGeoScriptControllerHeader *)pHdrCtrl, &state[0], t, param);
                pHdrCtrl += 8;
            }
            if (ctrlFlags & 2)
                t = EXScriptController::UpdateThread_TimeControllers((const EXGeoScriptControllerHeader *)pHdrCtrl, &state[1], t, param);
        }
        if (pGeoAnim) {
            float scriptRate = pScript->m_FrameRate;
            float animRate = (float)pGeoAnim->m_FrameRate;
            if (scriptRate != animRate) t = (animRate / scriptRate) * t;
        }
        u32 flags = m_ctrlFlags;
        m_prevFrame = m_frame;
        float f;
        if (!(flags & 1)) {
            f = frames;
            if (t < frames) {
                f = t;
                if (t <= 0.0f) f = 0.0f;
            }
        } else
            f = frames != 0.0f ? (float)fmod((double)t, (double)frames) : 0.0f;
        m_frame = f;
        m_frame = f / frames;
        if (flags & 2) UpdateAnimSounds(pAnim, rate);
    }
    if (!init) {
        if (m_scriptOffset >= 1.0f)
            m_prevScriptOffset = 1.0f;
        else
            m_prevScriptOffset = m_scriptOffset > 0.0f ? m_scriptOffset : 0.0f;
    }
}

// Script sound command (type 5) fields read here.
struct EXScriptSoundCmd {
    u8 m_Type;          // +0x00
    u8 m_Size;          // +0x01
    u16 m_02;
    s16 m_Frame;        // +0x04
    u16 m_06;
    u16 m_08;
    u8 m_Channel;       // +0x0a
    u8 m_0b[5];
    u32 m_Flags;        // +0x10 bit 15: pick a random sound
    u8 m_NumSounds;     // +0x14
    u8 m_Reverb;        // +0x15
    u8 m_16;            // +0x16 music: second argument
    u8 m_17;            // +0x17 music: first argument
    float m_Volume;     // +0x18
    float m_Range;      // +0x1c 0: the sound's own maximum distance
    u32 m_Sounds[1];    // +0x20
};

// Run the object / event / sound commands of the scene script crossed between frames prev and cur
// (in the direction of rate), keep the script's sound channels playing, and hand the remaining
// controller types to the script animator. Returns 0 if a command jumped the script time.
s8 EXItemCtrl_AnimScript::UpdateScript_ObjectsEngine(EXItemAnimator_Anim *pAnim, const EXGeoScript *pScript, float rate,
                                                     float prev, float cur)
{
    struct SoundChannel {
        const EXScriptSoundCmd *m_pCmd;
        u32 m_Offset;
        s8 m_Crossed;
    };
    u16 scriptFlags = pScript->m_ScriptFlags;
    EXItemHandler *pHandler = 0;
    u32 active[4] = { 0, 0, 0, 0 };
    SoundChannel channels[128];
    u32 off = 0;
    bool done = false;
    do {
        GEOSCRIPT_CMD *pCmd = pScript->Data(off);
        float frame = (float)(short)pCmd->m_04;
        s8 crossed = 0;
        if (rate > 0.001f) {
            if (prev <= cur) {
                if (prev < frame && frame <= cur) crossed = 1;
            } else if (prev < frame || frame <= cur)
                crossed = 1;
        } else if (rate < -0.001f) {
            if (cur <= prev) {
                if (frame < prev && !(frame < cur)) crossed = 1;
            } else if (!(prev <= frame) || !(frame < cur))
                crossed = 1;
        }
        switch (pCmd->m_Type) {
        case 3:
        case 4:
        case 6:
        case 0xc:
            if (crossed && !(scriptFlags & 5) && (pHandler || (pHandler = pAnim->GetOwnerHandler()) != 0))
                pHandler->DoScriptCmdObject((GEOSCRIPT_BASECMD *)pCmd, pAnim);
            break;
        case 5:
            if (!(scriptFlags & 4) && frame <= cur) {
                u8 ch = ((EXScriptSoundCmd *)pCmd)->m_Channel;
                channels[ch].m_Crossed = crossed;
                channels[ch].m_pCmd = (EXScriptSoundCmd *)pCmd;
                channels[ch].m_Offset = off;
                active[ch >> 5] |= 1 << (ch & 31);
            }
            break;
        case 0xb: {
            const u8 *p = (const u8 *)pCmd;
            if (!(m_ctrlFlags & 0x40) &&
                (((p[0x12] & 1) && frame <= cur && cur < ((float)*(const u16 *)(p + 6) + frame) - 1.0f) || crossed) &&
                (pHandler || (pHandler = pAnim->GetOwnerHandler()) != 0))
                pHandler->DoScriptCmdEvent((GEOSCRIPT_EVENT *)pCmd, pAnim);
            break;
        }
        case 0x10:
            if (crossed) {
                SetObjectFrame((float)pCmd->m_14);
                return 0;
            }
            break;
        case 0x11:
            if (frame <= cur) {
                const u8 *p = (const u8 *)pCmd;
                for (int i = 0; i < p[8]; i++) {
                    u8 ch = p[9 + i];
                    active[ch >> 5] &= ~(1 << (ch & 31));
                }
            }
            break;
        case 0x12:
            done = true;
            break;
        }
        off += pCmd->m_Size;
    } while (!done);

    u32 mask;
    if (scriptFlags & 1)
        mask = 0x6f6fa;
    else if (scriptFlags & 4)
        mask = 0x20;
    else {
        EXBaseSoundManager *pMgr = EXBaseSoundManager::GetSoundManager();
        if (!pMgr) return 1;
        EXSoundHandle *pHandle = m_soundTag.m_pHead;
        EXSoundTag *pTag = &m_soundTag;
        int w;
        for (w = 0; w < 4; w++)
            if (active[w]) break;
        if (w == 4 && !pHandle) return 1;
        EXVector pos;
        pos.x = pAnim->m_mtx.r[3].x;
        pos.y = pAnim->m_mtx.r[3].y;
        pos.z = pAnim->m_mtx.r[3].z;
        pos.w = pAnim->m_mtx.r[3].w;
        // Sounds already playing on a channel keep going while the same command is active there.
        for (; pHandle; pHandle = pHandle->Next()) {
            u32 ch = pHandle->m_Handle & 0xff;
            if (!ch) continue;
            int i = ch - 1;
            if (!((active[i / 32] >> (i & 31)) & 1))
                pHandle->m_Unk28 = 1;
            else {
                if (channels[i].m_Offset != ((pHandle->m_Handle >> 8) & 0xfffff)) pHandle->m_Unk28 = 1;
                active[i / 32] &= ~(1 << (i & 31));
            }
        }
        u32 numChannels = pScript->m_NumThreads;
        for (u32 ch = 0; ch < numChannels; ch++) {
            if (!((active[ch >> 5] >> (ch & 31)) & 1)) continue;
            const EXScriptSoundCmd *pCmd = channels[ch].m_pCmd;
            u8 n = pCmd->m_NumSounds;
            if (!n) continue;
            u32 pick = ((s8)(pCmd->m_Flags >> 8) < 0 && n) ? g_EXRandClass.Rand32() % n : 0;
            u32 hash = pCmd->m_Sounds[pick];
            const EXAnimatorScriptCallbacks *pCallbacks = pAnim->m_pScriptCallbacks;
            if (pCallbacks && pCallbacks->m_pSound)
                pCallbacks->m_pSound((const EXItemAnimator_Script *)pAnim, (const GEOSCRIPT_SOUND *)pCmd, &hash);
            u32 kind = (hash >> 24) & 0x7f;
            if (kind == 0x1a) {
                const EXSoundDetails &details = pMgr->SoundDetails(hash);
                if (!details.m_bLooping && !channels[ch].m_Crossed) continue;
                if (details.m_b3D) {
                    float range = pCmd->m_Range;
                    if (range == 0.0f) range = (float)details.m_MaxDist;
                    if (!pMgr->SfxIsValidRange(pos, range)) continue;
                }
                EXSoundHandle *pNew = pTag->StartSound(hash, pos, pCmd->m_Volume, pCmd->m_Range, pCmd->m_Reverb, 0);
                if (pNew) pNew->m_Handle = ((channels[ch].m_Offset | 0x100000) << 8) | (ch + 1);
            } else if (kind == 0x1b) {
                if (EXItemEnv::GetItemEnv()) EXItemEnv::GetItemEnv()->StartMusic(hash, pCmd->m_Flags, pCmd->m_17, pCmd->m_16);
            } else if (kind == 0x1c)
                pMgr->SfxSetReverbDepth(pCmd->m_Reverb);
        }
        if (pTag->m_pHead) {
            m_ctrlFlags.BitSet(0x200000);
            pAnim->EnableStackControllerObjects(1);
        }
        return 1;
    }
    if (pScript->m_UsedControllerTypes & mask) UpdateScript_ProcessSceneScript(pAnim, pScript, cur, mask);
    return 1;
}
