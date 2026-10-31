#include "EXItemAnimator_Script.h"
#include "EXItemAnimator_Anim.h"
#include "EXItemAnimator_Sound.h"
#include "EXItemAnimator_Collision.h"
#include "EXItemHandler.h"
#include "EXGeoFile.h"
#include "EXGeoScript.h"
#include "EXDatum.h"
#include "EXRand.h"
#include "EXRayCastTemp.h"
#include "EXItemAnimator_DynLight.h"
#include "XRGBA.h"
#include "EXBaseRenderEnv.h"
#include "EXGeoHeader.h"
#include "EXStdFuncs.h"
#include "EXItemAnimator_Camera.h"
#include "EXBaseSoundManager.h"
#include "EXBaseApp.h"
#include "EXMemory.h"

// TODO stand-in: animator pool (EXItemAnimator.obj).
class EXItemAnimatorQueue {
public:
    static void Release(EXItemAnimator *p);
    static EXItemAnimator *Allocate(u32 hash, EXItemAnimator *pOwner);
};
// TODO stand-in: item environment (EXItemEnv.obj); slot 2 starts music.
class EXItemEnv {
public:
    virtual ~EXItemEnv();
    virtual void Initialise() = 0;
    virtual void StartMusic(u32 hash, u32 flags, int a, int b);
    float MinDistanceToItemViews(const EXVector &p);
    EXDynLight *NewPointDynLight(ex::XRGBA col, float a, float b);
    EXDynLight *NewDirPointDynLight(ex::XRGBA col, const EXVector &dir, float a, float b);
    static EXItemEnv *m_pTheItemEnv;
};
extern EXRandClass g_EXRandClass;

// Script object command: file (-1: the script's own) and object hash.
struct _GEOSCRIPT_GEOOBJECT {
    u32 m_fileHash;
    u32 m_hash;
};
// Script light command.
struct _GEOSCRIPT_LIGHT {
    u16 m_type;         // +0 0: point
    u16 m_02;
    u8 m_flags;         // +4 2: affects one zone
    u8 m_05[3];
    ex::XRGBA m_colour; // +8
    float m_0c;
    float m_10;
};
// Script anim command (shared with EXItemCtrl_Anim.obj).
struct EXScriptAnimMotion {
    u32 m_fileHash;     // +0 -1: the script's own file
    u32 m_hash;         // +4
    float m_depth;      // +8
    s16 m_start;        // +0xc
    s16 m_end;          // +0xe
};
struct _GEOSCRIPT_ANIM {
    u8 m_numMotions;    // +0
    u8 m_01;            // +1 bits 1-3: motion flags
    u8 m_curMotion;     // +2
    u8 m_numMorphs;     // +3
    u32 m_skinFileHash; // +4
    u32 m_skinHash;     // +8
    EXScriptAnimMotion m_motions[1];    // +0xc
};
// Level-of-detail switch table entry (script controller data).
struct EXScriptLodEntry {
    float m_dist;
    _GEOSCRIPT_GEOOBJECT m_object;  // +4
};
struct EXScriptLodTable {
    u32 m_count;
    EXScriptLodEntry m_entries[1];
};
// TODO stand-in: script controller block header (EXItemAnimator_ScriptControllers.obj).
struct EXGeoScriptControllerHeader {
    void *Data() const
    {
        const s16 *p = &m_dataOff;
        return (char *)p + *p;
    }
    s16 m_numKeys;
    s16 m_dataOff;
    s32 m_keysOff;
    u32 m_flags;        // +8
};

// IsKindOf expanded in place (retail inlines the base-class walk).
static __forceinline s8 ExIsKindOf(const EXItemAnimator *p, const EXRuntimeClass *pClass)
{
    const EXRuntimeClass *r = p->GetRuntimeClass();
    do {
        if (r == pClass) return 1;
        r = r->m_pBaseClass;
    } while (r);
    return 0;
}

// Event sent to the handler when a script reaches its end command.
struct EXScriptEndEvent {
    u8 m_type;
    u8 m_size;
    u8 m_02[0xa];
    u32 m_0c;
    u8 m_10;
    u8 m_11[0x94 - 0x11];
};

static inline u32 ExRandIndex(u32 n)
{
    if (n) return g_EXRandClass.Rand32() % n;
    return 0;
}

inline s8 operator==(const EXGeoBaseRefObject &a, const EXGeoBaseRefObject &b)
{
    return a.m_pGeoFile == b.m_pGeoFile && a.m_hashRef == b.m_hashRef;
}

u8 EXItemAnimator_Script::ms_ScriptNodeTypeTable[20] = { 7, 7, 0, 2, 5, 6, 4, 3, 7, 7, 1, 7, 7, 8, 0xa, 9, 7, 7, 7, 0 };

void EXScriptThread::Initialise(int a, int b, int thread)
{
    m_pPlacement = 0;
    m_scriptOffset = 0xffffffff;
    m_rootNode = 0;
    m_thread = (u8)thread;
    m_animatorType = -1;
    m_valid = 0;
    m_parentThread = 0xff;
    m_14 = (u16)a;
    m_16 = (u16)b;
    m_localFrameTime = 0.0f;
    m_04 = 0.0f;
}

// NEAR: retail loads pCmd->m_hash before the file compare's branch (register choice follows).
EXItemAnimator *EXItemAnimator_Script::CreateThreadObject(const EXGeoScript *, const EXScriptThread *pThread, const _GEOSCRIPT_GEOOBJECT *pCmd) const
{
    EXGeoFile *pFile;
    if (pCmd->m_fileHash != 0xffffffff) pFile = EXGeoFile::FailSafeLoadGeoFile(pCmd->m_fileHash, 0);
    else pFile = m_geoRef.m_pGeoFile;
    if (!pFile) return 0;
    if (!pFile->GeoHeader()) return 0;
    EXItemAnimator *p = pThread->Placement();
    if (!p || pFile != p->m_geoRef.m_pGeoFile || pCmd->m_hash != p->m_geoRef.m_hashRef) {
        p = EXItemAnimatorQueue::Allocate(pCmd->m_hash, p);
        if (!p) return 0;
        if (!p->Initialise(pFile, pCmd->m_hash)) {
            EXItemAnimatorQueue::Release(p);
            return 0;
        }
    }
    p->SetFrameRateMult(0.0f);
    p->DoItemUpdate();
    return p;
}

// NEAR: as CreateThreadObject.
EXItemAnimator *EXItemAnimator_Script::CreateThreadScript(const EXGeoScript *, const EXScriptThread *pThread, const _GEOSCRIPT_GEOOBJECT *pCmd)
{
    EXGeoFile *pFile;
    if (pCmd->m_fileHash != 0xffffffff) pFile = EXGeoFile::FailSafeLoadGeoFile(pCmd->m_fileHash, 0);
    else pFile = m_geoRef.m_pGeoFile;
    if (!pFile) return 0;
    if (!pFile->GeoHeader()) return 0;
    EXItemAnimator *p = pThread->Placement();
    if (!p || pFile != p->m_geoRef.m_pGeoFile || pCmd->m_hash != p->m_geoRef.m_hashRef) {
        p = EXItemAnimatorQueue::Allocate(pCmd->m_hash, p);
        if (!p) return 0;
        if (!p->Initialise(pFile, pCmd->m_hash)) {
            EXItemAnimatorQueue::Release(p);
            return 0;
        }
    }
    ((EXItemAnimator_Script *)p)->RegisterCallbacks(m_pScriptCallbacks);
    p->SetOwner(this);
    p->SetFrameRateMult(0.0f);
    p->DoItemUpdate();
    return p;
}

// NEAR: SetSoundDetails argument loads ordered differently (reverb byte loaded first here).
EXItemAnimator *EXItemAnimator_Script::CreateThreadSound(const EXGeoScript *, const EXScriptThread *pThread, const GEOSCRIPT_SOUND *pCmd) const
{
    if (!pCmd->m_count) return 0;
    EXItemAnimator *p = pThread->Placement();
    if (!p) {
        u32 i = (pCmd->m_flags & 0x8000) ? ExRandIndex(pCmd->m_count) : 0;
        u32 hash = pCmd->m_hashes[i];
        if (m_pScriptCallbacks && m_pScriptCallbacks->m_pSound) m_pScriptCallbacks->m_pSound(this, pCmd, &hash);
        u32 h = hash;
        switch ((h >> 24) & 0x7f) {
        case 0x1a:
        case 0x1c:
            p = EXItemAnimatorQueue::Allocate(0x1a000000, 0);
            if (!p) return 0;
            if (!p->Initialise(m_geoRef.m_pGeoFile, h)) {
                EXItemAnimatorQueue::Release(p);
                return 0;
            }
            ((EXItemAnimator_Sound *)p)->SetSoundDetails(pCmd->m_volume, pCmd->m_range, pCmd->m_reverb, pCmd->m_flags,
                                                          (pCmd->m_flags & 0x100) ? (((u32)this & 0xfffffff) | 0x20000000) : 0);
            break;
        case 0x1b:
            if (EXItemEnv::m_pTheItemEnv) EXItemEnv::m_pTheItemEnv->StartMusic(h, pCmd->m_flags, pCmd->m_17, pCmd->m_16);
            break;
        }
    }
    return p;
}

EXItemAnimator *EXItemAnimator_Script::CreateThreadTransform(const EXGeoScript *, const EXScriptThread *pThread) const
{
    EXItemAnimator *p = pThread->Placement();
    if (!p) {
        p = EXItemAnimatorQueue::Allocate(0xffffffff, p);
        if (!p) return 0;
    }
    p->m_geoRef.Set(m_geoRef.m_pGeoFile, 0);
    return p;
}

EXItemAnimator *EXItemAnimator_Script::CreateThreadCollision(const EXGeoScript *, const EXScriptThread *pThread, const GEOSCRIPT_COLLISION *pCmd) const
{
    EXItemAnimator *p = pThread->Placement();
    if (!p) {
        p = new EXItemAnimator_Collision;
        if (!p) return 0;
    }
    ((EXItemAnimator_Collision *)p)->SetFromScriptCmd(pCmd);
    p->m_geoRef.Set(m_geoRef.m_pGeoFile, 0);
    return p;
}

__forceinline int EXItemAnimator_Script::DoUpdate_ScriptCmdObject(GEOSCRIPT_CMD *pCmd)
{
    EXItemHandler *h = GetOwnerHandler();
    if (!h) return 0;
    return h->DoScriptCmdObject((GEOSCRIPT_BASECMD *)pCmd, this);
}

s8 EXItemAnimator_Script::DoGrabPoint(const EXMatrix &mtx, EXGrabPointTemp &t, u32 flags)
{
    if (!m_numThreads) return 0;
    const EXMatrix &m = CalcMatrix_InPlace(mtx, flags);
    int n = m_numThreads;
    ex::EXBit128 bits = m_threadBits;
    EXScriptThread *pThread = m_pThreads;
    s8 r = 0;
    for (int i = 0; i < n; i++, pThread++) {
        EXItemAnimator *p = pThread->m_pPlacement;
        if (bits.IsSet(i)) r |= p->DoGrabPoint(m, t, flags);
    }
    return r;
}

s8 EXItemAnimator_Script::DoGrabPoint(const EXMatrix &mtx, EXGrabPointArrayTemp &t, u32 flags)
{
    if (!m_numThreads) return 0;
    const EXMatrix &m = CalcMatrix_InPlace(mtx, flags);
    int n = m_numThreads;
    ex::EXBit128 bits = m_threadBits;
    EXScriptThread *pThread = m_pThreads;
    s8 r = 0;
    for (int i = 0; i < n; i++, pThread++) {
        EXItemAnimator *p = pThread->m_pPlacement;
        if (bits.IsSet(i)) r |= p->DoGrabPoint(m, t, flags);
    }
    return r;
}

// NEAR: register allocation as DoEntityScoop.
void EXItemAnimator_Script::DoRayCast(const EXMatrix &mtx, EXRayCastTemp &t, u32 flags)
{
    if (!m_numThreads) return;
    const EXMatrix &m = CalcMatrix_InPlace(mtx, flags);
    int n = m_numThreads;
    ex::EXBit128 bits = m_threadBits;
    s8 bAny = t.m_bAnyHit;
    EXScriptThread *pThread = m_pThreads;
    for (int i = 0; i < n; i++, pThread++) {
        EXItemAnimator *p = pThread->m_pPlacement;
        if (bits.IsSet(i)) {
            p->DoRayCast(m, t, flags);
            if (bAny && t.m_bHit) return;
        }
    }
}

void *EXItemAnimator_Script::GetDatum(u32 type, EXDatumTemp &d, u32 flags, void *&pCtx)
{
    ex::EXBit128 bits = m_threadBits;
    void *pRet = 0;
    EXScriptThread *pThread = m_pThreads;
    int n = m_numThreads;
    for (int i = 0; i < n; i++, pThread++) {
        EXItemAnimator *p = pThread->m_pPlacement;
        if (bits.IsSet(i)) {
            pRet = p->GetDatum(type, d, flags, pCtx);
            if (pRet) {
                if (!(flags & 4)) {
                    EXMatrix m;
                    CalcMatrix(m);
                    d.ApplyXForm(m);
                }
                return pRet;
            }
        }
    }
    return pRet;
}

void EXItemAnimator_Script::EnableParticleSystems(s8 b) const
{
    ex::EXBit128 bits = m_threadBits;
    EXScriptThread *pThread = m_pThreads;
    int n = m_numThreads;
    for (int i = 0; i < n; i++, pThread++) {
        EXItemAnimator *p = pThread->m_pPlacement;
        if (bits.IsSet(i)) p->EnableParticleSystems(b);
    }
}

void EXItemAnimator_Script::BuildEntityInfoArray(const EXMatrix &mtx, EXGeoEntity_InfoArray &arr, s8)
{
    u32 flags = 0;
    const EXMatrix &m = CalcMatrix_InPlace(mtx, flags);
    EXScriptThread *pThread = m_pThreads;
    for (int i = m_numThreads; i > 0; i--, pThread++) {
        EXItemAnimator *p = pThread->m_pPlacement;
        if (p) p->BuildEntityInfoArray(m, arr, 0);
    }
}

EXScriptThread *EXItemAnimator_Script::GetThreadFromItem(const EXBaseItem *pItem) const
{
    int n = m_numThreads;
    EXScriptThread *pThread = m_pThreads;
    for (int i = 0; i < n; i++, pThread++) {
        if (pThread->m_pPlacement == pItem) return pThread;
    }
    return 0;
}

// NEAR: register allocation (retail keeps the thread pointer out of esi; pushes edi inside the loop guard).
void EXItemAnimator_Script::DoEntityScoop(const EXMatrix &mtx, EXCollisionScoopTemp &t, u32 flags)
{
    const EXMatrix &m = CalcMatrix_InPlace(mtx, flags);
    int n = m_numThreads;
    ex::EXBit128 bits = m_threadBits;
    EXScriptThread *pThread = m_pThreads;
    for (int i = 0; i < n; i++, pThread++) {
        if (bits.IsSet(i)) pThread->m_pPlacement->DoEntityScoop(m, t, flags);
    }
}

void EXItemAnimator_Script::SetRenderState(u32 state, s8 bOn)
{
    EXScriptThread *pThread = m_pThreads;
    for (int i = m_numThreads; i > 0; i--, pThread++) {
        EXItemAnimator *p = pThread->m_pPlacement;
        if (p) p->SetRenderState(state, bOn);
    }
    EXItemAnimator::SetRenderState(state, bOn);
}

void EXItemAnimator_Script::ApplyFunction(u32 (__cdecl *pFunc)(EXItemAnimator *, void *), void *pData)
{
    EXScriptThread *pThread = m_pThreads;
    for (int i = m_numThreads; i > 0; i--, pThread++) {
        EXItemAnimator *p = pThread->m_pPlacement;
        if (p) p->ApplyFunction(pFunc, pData);
    }
    EXItemAnimator::ApplyFunction(pFunc, pData);
}

void EXItemAnimator_Script::SetLodMin(u32 lod)
{
    EXScriptThread *pThread = m_pThreads;
    for (int i = m_numThreads; i > 0; i--, pThread++) {
        EXItemAnimator *p = pThread->m_pPlacement;
        if (p) p->SetLodMin(lod);
    }
}

void EXItemAnimator_Script::SetLodMax(u32 lod)
{
    EXScriptThread *pThread = m_pThreads;
    for (int i = m_numThreads; i > 0; i--, pThread++) {
        EXItemAnimator *p = pThread->m_pPlacement;
        if (p) p->SetLodMax(lod);
    }
}

void EXItemAnimator_Script::ProcessLevelOfDetailControllers(const EXGeoScript *pScript, const EXScriptThread *pThread, const EXGeoScriptControllerHeader *pHdr, const EXScriptController *) const
{
    EXItemAnimator *p = pThread->Placement();
    if (!p) return;
    p->CalcMatrix_InPlaceParent();
    EXVector pos = m_mtx.GetTranslation();
    float d = EXItemEnv::m_pTheItemEnv->MinDistanceToItemViews(pos);
    const EXScriptLodTable *pTable = (const EXScriptLodTable *)pHdr->Data();
    for (int i = pTable->m_count - 1; i >= 0; i--) {
        const EXScriptLodEntry *e = &pTable->m_entries[i];
        if (d > e->m_dist) {
            switch ((e->m_object.m_hash >> 24) & 0x7f) {
            case 0:
            case 2:
            case 4:
                CreateThreadObject(pScript, pThread, &e->m_object);
                break;
            }
            return;
        }
    }
}

EXItemAnimator_Script::EXItemAnimator_Script()
{
    m_pThreads = 0;
    m_numThreads = 0;
    m_bRootControllers = 0;
    m_pJumpCounts = 0;
    m_pScriptCallbacks = 0;
    m_11a = 0;
    m_scriptOffset = 0;
    m_scriptStatus = 0;
    m_validCmds = 0;
    m_objectTime = -1.0f;
    m_threadBits.ClrAll();
    m_activeBits.ClrAll();
}

void EXItemAnimator_Script::UpdateThreads(const EXGeoScript *pScript)
{
    if (m_bRootControllers) m_pThreads[m_numThreads].UpdateThreadControllers(this, pScript, m_objectTime);
    int n = m_numThreads;
    EXScriptThread *pThread = m_pThreads;
    ex::EXBit128 bits = m_activeBits;
    for (int i = 0; i < n; i++, pThread++) {
        if (bits.IsSet(i)) {
            float t;
            if (pThread->HasParentThread()) t = m_pThreads[pThread->ParentThread()].LocalFrameTime();
            else t = m_objectTime;
            pThread->UpdateThreadControllers(this, pScript, t);
        }
    }
}

EXGeoCommonObject *EXItemAnimator_Script::GetGeoObject() const
{
    return (EXGeoCommonObject *)m_geoRef.GeoScript();
}

EXGeoScript *EXItemAnimator_Script::GetGeoScript() const
{
    return m_geoRef.GeoScript();
}

void EXItemAnimator_Script::CleanItemThread(EXScriptThread *pThread)
{
    if (pThread->IsRootNode()) return;
    EXItemAnimator *p = pThread->Placement();
    if (p) {
        if (m_pScriptCallbacks && m_pScriptCallbacks->m_pThreadEnd) m_pScriptCallbacks->m_pThreadEnd(this, pThread);
        EXBaseItem *pChild = p->FirstItem();
        while (pChild) {
            EXBaseItem *pNext = pChild->NextItem();
            EXScriptThread *t = GetThreadFromItem(pChild);
            if (t) CleanItemThread(t);
            pChild = pNext;
        }
        EXItemAnimator *pOwner = (EXItemAnimator *)p->GetOwner();
        if (ExIsKindOf(pOwner, EXRUNTIME_CLASS(EXItemAnimator_Script))) {
            if (pThread->HasParentThread()) pOwner->Nodes().Remove(&p->m_node);
            else m_threadNodes.Remove(&p->m_node);
        } else {
            pOwner->Nodes().Remove(&p->m_node);
        }
        p->SetOwner(0);
        pThread->SetPlacement(0);
        pThread->SetValid(0);
        EXItemAnimatorQueue::Release(p);
    }
    m_threadBits.Clr(pThread->Thread());
    m_activeBits.Clr(pThread->Thread());
}

void EXItemAnimator_Script::AddItemThread(EXItemAnimator *p, const GEOSCRIPT_CMD *pCmd)
{
    u32 thread = ((const u8 *)pCmd)[0xa];
    EXScriptThread *pThread = &m_pThreads[thread];
    if (pThread->Placement()) {
        if (pThread->Placement() == p) return;
        CleanItemThread(pThread);
    }
    u32 parent = ((const u8 *)pCmd)[0xb];
    if (parent != 0xff) {
        EXItemAnimator *pParent = m_pThreads[parent].Placement();
        if (!pParent) {
            parent = 0xff;
        } else {
            p->SetOwner(pParent);
            pParent->Nodes().AddHead(&p->m_node);
            goto done;
        }
    }
    if (m_threadNodes.IsMember(p ? &p->m_node : 0)) while (1) {}
    p->SetOwner(this);
    m_threadNodes.AddTail(&p->m_node);
done:
    p->m_itemFlags.m_flags |= 0x80;
    p->m_rotOrder = 0;
    p->m_validMtx = 0;
    pThread->InitialiseThread(p, parent);
    pThread->SetValid(1);
    m_threadBits.Set(thread);
}

// NEAR: register assignment (retail: pFile ebx, pCmd ebp, skin file edi).
EXItemAnimator *EXItemAnimator_Script::CreateThreadAnim(const EXGeoScript *, const EXScriptThread *pThread, const _GEOSCRIPT_ANIM *pCmd)
{
    EXGeoFile *pFile;
    if (pCmd->m_motions[0].m_fileHash != 0xffffffff) pFile = EXGeoFile::FailSafeLoadGeoFile(pCmd->m_motions[0].m_fileHash, 0);
    else pFile = m_geoRef.m_pGeoFile;
    EXGeoFile *pSkinFile;
    if (pCmd->m_skinFileHash != 0xffffffff) pSkinFile = EXGeoFile::FailSafeLoadGeoFile(pCmd->m_skinFileHash, 0);
    else pSkinFile = pFile;
    if (!pFile || !pFile->GeoHeader() || !pSkinFile || !pSkinFile->GeoHeader()) return 0;
    EXItemAnimator_Anim *p = (EXItemAnimator_Anim *)pThread->Placement();
    if (p) {
        if (pFile == p->m_geoRef.m_pGeoFile && pCmd->m_motions[0].m_hash == p->m_geoRef.m_hashRef) {
            p->ChangeTo(0, pCmd->m_motions[0].m_hash, 3);
            goto setup;
        }
    } else {
        p = (EXItemAnimator_Anim *)EXItemAnimatorQueue::Allocate(0x3000000, 0);
        if (!p) return 0;
    }
    {
        EXGeoRefObject skel(pFile, pCmd->m_motions[0].m_hash);
        EXGeoRefObject skin;
        if (pCmd->m_skinHash != 0xffffffff) skin = EXGeoRefObject(pSkinFile, pCmd->m_skinHash);
        else skin = skel;
        if (!p->Initialise(skel, skin)) {
            EXItemAnimatorQueue::Release(p);
            return 0;
        }
    }
setup:
    {
        EXItemCtrl_Anim *pCtrl = (EXItemCtrl_Anim *)p->ControllerStack(0).FirstCtrl();
        if (pCtrl) {
            pCtrl->m_ctrlFlags = (pCtrl->m_ctrlFlags & ~1) | 0x10100;
            pCtrl->SetFromScriptAnimCmd(pFile->m_pGeoHeader, pCmd);
            u8 f = pCmd->m_01 & 0xe;
            if (f) pCtrl->SetMotionFlags(f << 7);
        }
    }
    p->SetOwner(this);
    p->SetFrameRateMult(0.0f);
    p->DoItemUpdate();
    return p;
}

EXItemAnimator *EXItemAnimator_Script::CreateThreadLight(const EXGeoScript *, const EXScriptThread *pThread, const _GEOSCRIPT_LIGHT *pCmd) const
{
    EXItemEnv *pEnv = EXItemEnv::m_pTheItemEnv;
    if (!pEnv) return 0;
    EXItemAnimator *p = pThread->Placement();
    if (!p) {
        p = EXItemAnimatorQueue::Allocate(0x13000000, p);
        if (!p) return 0;
        p->m_renderFlags.m_flags |= 1;
        EXDynLight *pLight;
        if (pCmd->m_type == 0) {
            pLight = pEnv->NewPointDynLight(pCmd->m_colour, pCmd->m_0c, pCmd->m_10);
        } else {
            EXVector dir(0.0f, 0.0f, 1.0f, 0.0f);
            pLight = pEnv->NewDirPointDynLight(pCmd->m_colour, dir, pCmd->m_0c, pCmd->m_10);
        }
        pLight->SetAffectsOneZone(pCmd->m_flags & 2);
        ((EXItemAnimator_DynLight *)p)->AttachDynLight(pLight);
    }
    return p;
}

void EXItemAnimator_Script::UpdateThreadNodes()
{
    int n = m_numThreads;
    ex::EXBit128 bits = m_threadBits;
    for (int i = 0; i < n; i++) {
        if (bits.IsSet(i)) {
            EXScriptThread *pThread = &m_pThreads[i];
            EXItemAnimator *p = pThread->Placement();
            if (p && (p->m_itemFlags & 0x10)) CleanItemThread(pThread);
        }
    }
}

void EXItemAnimator_Script::UpdateThreads_EndThread(const EXGeoScript *pScript, const GEOSCRIPT_CMD *pCmd, EXScriptThread *pThread)
{
    if (m_scriptStatus == 3 && m_threadBits.IsSet(pThread->Thread())) {
        ex::EXRELOFFSET32 *pTable = pScript->ThreadControllerTable();
        ex::EXRELOFFSET32 *e = &pTable[pThread->CtrlIndex()];
        const EXGeoScriptControllerHeader *pHdr = (const EXGeoScriptControllerHeader *)(e->m_Offset + (char *)e);
        if (pHdr->m_flags & 0x200) pThread->UpdateThreadControllers(this, pScript, (float)((const s16 *)pCmd)[2]);
    }
    CleanItemThread(pThread);
}

u32 EXItemAnimator_Script::DoRender(const EXMatrix &mtx, const EXRenderInfo &info, u32 flags)
{
    u32 r = 0;
    if (EXBaseRenderEnv::RenderState() & 1) CalcMatrix_InPlace(mtx, flags);
    EXPlacementItem *pFirst = FirstNode();
    EXDListItem *pHead = m_threadNodes.m_pHead;
    EXItemAnimator *p = pHead ? (EXItemAnimator *)((char *)pHead - 4) : 0;
    if (p || pFirst) {
        flags |= 0x40;
        EXRenderInfo ri(m_renderInfo, info);
        const EXMatrix &m = m_mtx;
        for (; p; p = p->NextAnimator()) {
            if (p->m_itemFlags & 1) r |= p->DoRender(m, ri, flags);
        }
        r |= RenderNodes(m, ri, flags);
    }
    return r;
}

// NEAR: arr / thread pointer swap ebx and ebp.
void EXItemAnimator_Script::GetDatum(u32 type, EXDatumTempArray &arr, u32 flags)
{
    int start = arr.Size();
    ex::EXBit128 bits = m_threadBits;
    EXScriptThread *pThread = m_pThreads;
    int n = m_numThreads;
    for (int i = 0; i < n; i++, pThread++) {
        EXItemAnimator *p = pThread->m_pPlacement;
        if (bits.IsSet(i)) p->GetDatum(type, arr, flags);
    }
    if (!(flags & 4)) {
        int cnt = arr.Size() - start;
        if (cnt) {
            EXMatrix m;
            CalcMatrix(m);
            for (; cnt > 0; cnt--) arr[start++].ApplyXForm(m);
        }
    }
    if (flags & 0x80) GetDatum_ChildLink(type, arr, flags);
}

void EXItemAnimator_Script::GetGeoBoundsBox(EXBoundsBox &box)
{
    const EXGeoComplexRefObject &ref = m_geoRef;
    EXGeoScript *pScript = ref.GeoScript();
    if (!pScript) {
        box.SetZero();
        return;
    }
    box.Set(pScript->GetBoundsBox());
}

s8 EXItemAnimator_Script::_GetBoundsBox(EXBoundsBox &box, u32 flags)
{
    EXBoundsBox b;
    GetGeoBoundsBox(b);
    ApplyMatrixBoundsBox(b, box, flags);
    return 1;
}

// NEAR: register assignment (pScript ebx / count edi in retail) and epilogue layout.
EXItemAnimator *EXItemAnimator_Script::GetThreadAnimator(u32 hash) const
{
    EXGeoHeader *pHdr = m_geoRef.m_pGeoFile->GeoHeader();
    if (!pHdr) return 0;
    u32 idx = pHdr->GeoObjectHashIndex(hash);
    int n;
    int i;
    if (idx != 0xffffffff) {
        n = m_numThreads;
        for (i = 0; i < n; i++) {
            EXItemAnimator *p = m_pThreads[i].m_pPlacement;
            if (p && p->m_geoRef.m_hashRef == idx) return p;
        }
        return 0;
    }
    EXGeoScript *pScript = m_geoRef.GeoScript();
    n = m_numThreads;
    for (i = 0; i < n; i++) {
        EXScriptThread *t = &m_pThreads[i];
        if (t->m_pPlacement && pScript->Data(t->m_scriptOffset)->m_0c == hash) return t->m_pPlacement;
    }
    return 0;
}

float EXItemAnimator_Script::ObjectTime() const
{
    EXGeoScript *pScript = m_geoRef.GeoScript();
    if (!pScript) return -1.0f;
    return m_objectTime / pScript->FrameRate();
}

float EXItemAnimator_Script::ObjectFrame() const
{
    EXGeoScript *pScript = m_geoRef.GeoScript();
    if (!pScript) return -1.0f;
    return m_objectTime;
}

EXItemAnimator_Script *EXItemAnimator_Script::CreateObject()
{
    return new EXItemAnimator_Script;
}

void EXScriptThread::ResetThread(const EXItemAnimator_Script *pScript, const EXGeoScript *pGeo, const GEOSCRIPT_CMD *pCmd, s8 b)
{
    u16 ctrl = pCmd->m_Thread;
    m_localFrameTime = 0.0f;
    m_ctrlIndex = ctrl;
    float t = (float)(s16)pCmd->m_04;
    u32 parent = ((const u8 *)pCmd)[0xb];
    if (parent != 0xff) t -= pScript->m_pThreads[parent].m_04;
    m_04 = t;
    m_scriptOffset = pScript->m_scriptOffset;
    m_1c = 0xffffffff;
    memset((char *)this + m_14, 0, m_16 * 8);
    if (b) UpdateThreadControllers(pScript, pGeo, t);
}

void EXItemAnimator_Script::CleanInitialise()
{
    EXGeoScript *pScript = GetGeoScript();
    if (pScript && (pScript->ScriptFlags() & 8))
        EXBaseSoundManager::GetSoundManager()->SfxRemoveAllHandle(((u32)this & 0xfffffff) | 0x20000000, 1);
    for (int i = m_numThreads - 1; i >= 0; i--) {
        EXScriptThread *pThread = &m_pThreads[i];
        if (pThread->Placement()) CleanItemThread(pThread);
    }
    if (m_pThreads) {
        EXFree(m_pThreads);
        m_pThreads = 0;
    }
    m_numThreads = 0;
    m_threadBits.ClrAll();
    m_activeBits.ClrAll();
    m_geoRef.Set(0, 0);
}

// NEAR: this / result swap esi and edi.
EXItemAnimator *EXItemAnimator_Script::CreateThreadAnimator(const EXGeoScript *pGeo, EXScriptThread *pThread, const GEOSCRIPT_REFCMD *pCmd)
{
    EXItemAnimator *p;
    switch (pThread->AnimatorType()) {
    case 2:
    case 4:
    case 9:
        p = CreateThreadObject(pGeo, pThread, (const _GEOSCRIPT_GEOOBJECT *)((const u8 *)pCmd + 0x10));
        break;
    case 5:
        p = CreateThreadScript(pGeo, pThread, (const _GEOSCRIPT_GEOOBJECT *)((const u8 *)pCmd + 0x10));
        break;
    case 0:
        p = CreateThreadAnim(pGeo, pThread, (const _GEOSCRIPT_ANIM *)((const u8 *)pCmd + 0x10));
        break;
    case 3:
        p = CreateThreadLight(pGeo, pThread, (const _GEOSCRIPT_LIGHT *)((const u8 *)pCmd + 0x10));
        break;
    case 6:
        p = CreateThreadSound(pGeo, pThread, (const GEOSCRIPT_SOUND *)pCmd);
        break;
    case 7:
        p = CreateThreadTransform(pGeo, pThread);
        break;
    case 8:
        p = CreateThreadCollision(pGeo, pThread, (const GEOSCRIPT_COLLISION *)pCmd);
        break;
    default:
        return 0;
    }
    if (!p) {
        CleanItemThread(pThread);
        return p;
    }
    AddItemThread(p, (const GEOSCRIPT_CMD *)pCmd);
    return p;
}

int EXItemAnimator_Script::DoUpdate_ScriptRootNode(const EXGeoScript *pGeo, const GEOSCRIPT_CMD *pCmd)
{
    EXScriptThread *pThread = &m_pThreads[m_numThreads];
    m_rotOrder = 0;
    m_validMtx = 0;
    pThread->InitialiseThread(this, 0xff);
    pThread->SetRootNode();
    pThread->ResetThread(this, pGeo, pCmd, 1);
    return 1;
}

int EXItemAnimator_Script::DoUpdate_ScriptCamera(EXGeoScript *pGeo, GEOSCRIPT_CMD *pCmd, EXScriptThread *pThread)
{
    CleanItemThread(pThread);
    EXItemAnimator_Camera *p = new EXItemAnimator_Camera;
    if (!p) return 0;
    p->m_geoRef.Set(m_geoRef.m_pGeoFile, 0);
    p->SetFromScriptCmd((const GEOSCRIPT_CAMERA *)pCmd);
    AddItemThread(p, pCmd);
    m_activeBits.Set(((const u8 *)pCmd)[0xa]);
    pThread->ResetThread(this, pGeo, pCmd, 1);
    pThread->SetAnimatorType(1);
    if (m_pScriptCallbacks && m_pScriptCallbacks->m_pCamera) m_pScriptCallbacks->m_pCamera(this, pGeo, pCmd, p);
    return 1;
}

// NEAR: inlined ResetThread loads the parent byte before the frame here.
int EXItemAnimator_Script::DoUpdate_InitialiseThread(EXGeoScript *pGeo, const GEOSCRIPT_CMD *pCmd, EXScriptThread *pThread)
{
    pThread->ResetThread(this, pGeo, pCmd, 0);
    m_activeBits.Set(((const u8 *)pCmd)[0xa]);
    pThread->SetAnimatorType(ms_ScriptNodeTypeTable[pCmd->m_Type]);
    if (!m_pScriptCallbacks || !m_pScriptCallbacks->m_pCreateThread || !m_pScriptCallbacks->m_pCreateThread(this, pThread, pGeo, pCmd))
        CreateThreadAnimator(pGeo, pThread, (const GEOSCRIPT_REFCMD *)pCmd);
    return 1;
}

void EXItemAnimator_Script::SetScriptOffset(u32 off)
{
    const EXGeoComplexRefObject &ref = m_geoRef;
    const EXGeoScript *pScript = ref.GeoScript();
    if (!pScript) return;
    int frame = (int)m_objectTime;
    m_scriptOffset = off;
    if (off) m_objectTime = (float)(s16)pScript->Data(off)->m_04;
    else m_objectTime = 0.0f;
    ex::EXBit128 bits;
    bits.ClrAll();
    for (;; off += pScript->Data(off)->m_Size) {
        const GEOSCRIPT_CMD *pCmd = pScript->Data(off);
        if (m_validCmds & (1 << pCmd->m_Type)) {
            if ((s16)pCmd->m_04 != frame) break;
            switch (pCmd->m_Type) {
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 9:
            case 0xa:
            case 0xd:
            case 0xf: {
                u32 thread = ((const u8 *)pCmd)[0xa];
                EXScriptThread *pThread = &m_pThreads[thread];
                if (pThread->m_scriptOffset == off) {
                    bits.Set(thread);
                    pThread->ResetThread(this, pScript, pCmd, 1);
                }
                break;
            }
            case 0x12:
                goto done;
            }
        }
    }
done:
    int n = m_numThreads;
    for (int i = 0; i < n; i++) {
        if (!bits.IsSet(i)) {
            EXScriptThread *pThread = &m_pThreads[i];
            if (pThread->m_pPlacement) CleanItemThread(pThread);
        }
    }
}

// NEAR: return value kept in edi here; retail zeroes eax before the handler test.
__declspec(noinline) int EXItemAnimator_Script::DoUpdate_ScriptCmdEventCall(GEOSCRIPT_CMD *pCmd, int r)
{
    EXItemHandler *h = GetOwnerHandler();
    int ret = 0;
    if (h) {
        u32 off = m_scriptOffset;
        if (!r) r = h->DoScriptCmdEvent((GEOSCRIPT_EVENT *)pCmd, this);
        switch (r) {
        case 1:
            return 1;
        case 2:
            SetScriptOffset(0);
            return -1;
        case 3:
            m_scriptStatus = 3;
            return (off == m_scriptOffset) - 1;
        case 4:
            m_objectTime = (float)(s16)pCmd->m_04;
            m_scriptStatus = 0;
            break;
        }
        ret = (off == m_scriptOffset) - 1;
    }
    return ret;
}

int EXItemAnimator_Script::DoUpdate_ScriptCmdTimeJump(GEOSCRIPT_CMD *pCmd, const EXGeoScript *)
{
    if (((const u8 *)pCmd)[0xc] == 3) {
        EXItemHandler *h = GetOwnerHandler();
        if (h) {
            switch (h->DoScriptCmdRunControl((GEOSCRIPT_RUNCONTROL *)pCmd, this)) {
            case 0:
                return 0;
            case 1:
                return 1;
            case 2:
                SetScriptOffset(0);
                return -1;
            }
        }
    }
    int *pCount = &m_pJumpCounts[((const u8 *)pCmd)[0xd]];
    if (*pCount > 0) {
        if (--*pCount == 0) return 0;
    } else {
        *pCount = pCmd->m_10;
    }
    SetScriptFrame((float)pCmd->m_14);
    return -1;
}

void EXItemAnimator_Script::SetScriptStatus(u32 s)
{
    switch (s) {
    case 2:
        SetScriptOffset(0);
        m_scriptStatus = 0;
        break;
    default:
        m_scriptStatus = s;
        break;
    }
}

void EXItemAnimator_Script::SetObjectTime(float t)
{
    const EXGeoComplexRefObject &ref = m_geoRef;
    EXGeoScript *pScript = ref.GeoScript();
    if (pScript) SetScriptFrame(pScript->m_FrameRate * t);
}

void EXItemAnimator_Script::SetObjectFrame(float f)
{
    const EXGeoComplexRefObject &ref = m_geoRef;
    if (ref.GeoScript()) SetScriptFrame(f);
}

EXItemAnimator_Script::~EXItemAnimator_Script()
{
    CleanInitialise();
}

EXItemAnimator *EXItemAnimator_Script::ProcessThreadAnimator(EXGeoScript *pGeo, EXScriptThread *pThread)
{
    const GEOSCRIPT_CMD *pCmd = pGeo->Data(pThread->ScriptOffset());
    if (m_pScriptCallbacks && m_pScriptCallbacks->m_pCreateThread && m_pScriptCallbacks->m_pCreateThread(this, pThread, pGeo, pCmd)) return pThread->Placement();
    return CreateThreadAnimator(pGeo, pThread, (const GEOSCRIPT_REFCMD *)pCmd);
}

void EXItemAnimator_Script::DoUpdate_ScriptCmds(float dt)
{
    if (m_scriptStatus == 1) return;
    EXGeoScript *pScript = m_geoRef.GeoScript();
    if (!pScript) return;
    while (1) {
        GEOSCRIPT_CMD *pCmd = pScript->Data(m_scriptOffset);
        if (m_validCmds & (1 << pCmd->m_Type)) {
            if (m_scriptStatus != 3) {
                if ((s16)pCmd->m_04 > (int)m_objectTime) {
                    UpdateThreads(pScript);
                    m_objectTime += dt;
                    if (m_scriptStatus == 3) continue;
                    return;
                }
            } else {
                m_objectTime = (float)(s16)pCmd->m_04;
            }
            EXScriptThread *pThread = &m_pThreads[((const u8 *)pCmd)[0xa]];
            int r;
            switch (pCmd->m_Type) {
            case 0x10:
                r = DoUpdate_ScriptCmdTimeJump(pCmd, pScript);
                if (r > 0) return;
                if (r < 0) continue;
                break;
            case 5:
                if (m_scriptStatus == 3) break;
            case 2:
            case 3:
            case 4:
            case 6:
            case 7:
            case 8:
            case 9:
            case 0xd:
            case 0xf:
                DoUpdate_InitialiseThread(pScript, pCmd, pThread);
                break;
            case 0xa:
                DoUpdate_ScriptCamera(pScript, pCmd, pThread);
                break;
            case 1:
                DoUpdate_ScriptRootNode(pScript, pCmd);
                break;
            case 0xb:
                r = DoUpdate_ScriptCmdEventCall(pCmd, 0);
                if (r > 0) return;
                if (r < 0) continue;
                break;
            case 0xc:
                DoUpdate_ScriptCmdObject(pCmd);
                break;
            case 0x11: {
                for (int i = 0; i < ((const u8 *)pCmd)[8]; i++) UpdateThreads_EndThread(pScript, pCmd, &m_pThreads[((const u8 *)pCmd)[9 + i]]);
                break;
            }
            case 0x12: {
                EXItemHandler *h = GetOwnerHandler();
                if (h) {
                    EXScriptEndEvent ev;
                    ev.m_type = 0xb;
                    ev.m_size = 0xc;
                    ev.m_10 = 0;
                    ev.m_0c = 0xffffffff;
                    h->DoScriptCmdEvent((GEOSCRIPT_EVENT *)&ev, this);
                }
                m_11a = 1;
                return;
            }
            }
        }
        m_scriptOffset += pCmd->m_Size;
    }
}

void EXItemAnimator_Script::DoUpdate()
{
    DoUpdate_ScriptCmds(m_frameRateMult * EXBaseApp::EngineRenderFrameRateMul());
    UpdateThreadNodes();
    UpdateNodes();
}

void EXItemAnimator_Script::DoUpdate_ScriptCmdsReverse(float t)
{
    EXGeoScript *pScript = m_geoRef.GeoScript();
    if (!pScript) return;
    int frame = (int)m_objectTime;
    GEOSCRIPT_CMD *pCmd = pScript->Data(m_scriptOffset);
    while ((s16)pCmd->m_04 >= (int)t) {
        if ((s16)pCmd->m_04 < frame && (m_validCmds & (1 << pCmd->m_Type))) {
            int r;
            switch (pCmd->m_Type) {
            case 0x10:
                r = DoUpdate_ScriptCmdTimeJump(pCmd, pScript);
            check:
                if (r > 0) {
                    t = (float)(s16)pCmd->m_04;
                    goto end;
                }
                if (r < 0) return;
                break;
            case 0xb:
                r = DoUpdate_ScriptCmdEventCall(pCmd, 0);
                goto check;
            case 5: {
                EXScriptThread *pThread = &m_pThreads[((const u8 *)pCmd)[0xa]];
                if (pThread->m_pPlacement) CleanItemThread(pThread);
                DoUpdate_InitialiseThread(pScript, pCmd, pThread);
                EXItemAnimator *p = pThread->m_pPlacement;
                if (p) p->DoUpdate();
                break;
            }
            }
        }
        u8 back = ((const u8 *)pCmd)[2];
        if (!back) break;
        m_scriptOffset -= back;
        pCmd = pScript->Data(m_scriptOffset);
    }
end:
    SetScriptFrame(t);
}

void EXItemAnimator_Script::UpdateObjectTime(float dt)
{
    const EXGeoComplexRefObject &ref = m_geoRef;
    EXGeoScript *pScript = ref.GeoScript();
    if (!pScript) return;
    float t = pScript->m_FrameRate * dt;
    float d = t - m_objectTime;
    if (0.0f > d) {
        DoUpdate_ScriptCmdsReverse(t);
        return;
    }
    if (d > 0.0f) {
        m_objectTime += d;
        DoUpdate_ScriptCmds(0.0f);
        UpdateThreadNodes();
    }
}
