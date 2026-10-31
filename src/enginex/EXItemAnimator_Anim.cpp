#include <math.h>
#include "EXCollisionScoopTemp.h"
#include "EXItemAnimator_Anim.h"
#include "EXParticleSys.h"
#include "EXItemCtrl_SnapShot.h"
#include "EXDatum.h"
#include "EXStdFuncs.h"
#include "EXMemory.h"
#include "EXScratchPad.h"
#include "EXBaseRenderEnv.h"
#include "EXDisplay.h"
#include "EXItemHandler.h"

// TODO stand-in: item environment (EXItemEnv.obj).
class EXItemEnv {
public:
    float MinDistanceToItemViews(const EXVector &p);
    static EXItemEnv *m_pTheItemEnv;
};
// TODO stand-in: unnamed .data dword at 0x39a088 (just after EXItemCtrl::ms_BlendSpline) that
// gates the LOD calculation; probably the item-view count.
extern u32 g_LodViewsStandIn;


// TODO stand-in: common geo object (geo area); slot 4 returns its bounds.
class EXGeoCommonObjectStandIn {
public:
    virtual EXRuntimeClass *GetRuntimeClass() const;
    virtual void OnCreate(EXGeoHeader *, void *);
    virtual void OnDelete(EXGeoHeader *, void *);
    virtual void Draw(void *, u32);
    virtual EXBoundsBox &GetBoundsBox();
};

// TODO stand-in: anim-set table header (geo anim area). Entries are variable length: u16 stack,
// s16 item count, 4 bytes, then count 12-byte items.
struct EXGeoAnimSetItem {
    u32 m_hash;
    u16 m_flags;        // +4
    u16 m_frames;       // +6
    u32 m_spline;       // +8
};
struct EXGeoAnimSet {
    u16 m_stack;
    s16 m_count;
    float m_blend;                  // +4
    EXGeoAnimSetItem m_items[1];    // +8
    EXGeoAnimSet *Next() const { return (EXGeoAnimSet *)((u8 *)this + 8 + m_count * 12); }
};
class EXGeoAnimSetHeader {
public:
    EXGeoAnimSet *FirstSet() const
    {
        if (!this) return 0;
        return m_pFirst;
    }
    u8 m_00[0xc];
    EXGeoAnimSet *m_pFirst;     // +0xc
    u32 m_numSets;              // +0x10
};

int __cdecl EXGeoCommonArray_FindIndex(u32 hash, void *pArray, u32 stride);

namespace ex {
// TODO stand-in: 32-bit self-relative pointer (geo file area).
template <class T> class EXRelPtr {
public:
    operator T *() const
    {
        if (!m_off) return 0;
        return (T *)((char *)this + m_off);
    }
    s32 m_off;
};
}

// TODO stand-in: hashed object table in a geo header (geo area): count, then a relative pointer
// to sizeof(T)-byte entries.
template <class T> class EXGeoCommonArray {
public:
    int FindIndex(u32 hash) const { return EXGeoCommonArray_FindIndex(hash, (void *)this, sizeof(T)); }
    T *Find(u32 hash) const
    {
        int i = FindIndex(hash);
        if (i < 0) return 0;
        return &((T *)m_data)[i];
    }
    u32 m_count;
    ex::EXRelPtr<T> m_data;     // +4
};

// TODO stand-ins: geo animation headers (geo anim area).
class EXGeoAnimHeader {
public:
    u32 SkinNum() const { return m_skinNum; }
    u8 m_00[0xc];
    void *m_pAnim;          // +0xc
    u8 m_10[0xc];
    u32 m_skinNum;          // +0x1c
};
class EXGeoAnimSkinHeader {
public:
    u32 BaseSkinNum() const { return m_baseSkinNum; }
    u8 m_00[0x10];
    u32 m_baseSkinNum;      // +0x10
    u8 m_14[0x10];
};
class EXGeoAnimModeHeader {
public:
    u8 m_00[0x10];
    u32 m_numModes;         // +0x10
};
// TODO stand-in: geo file header (geo area); only the animation tables are declared.
class EXGeoHeader {
public:
    EXGeoAnimHeader *GeoAnimHeader(u32 hash) const;
    EXGeoAnimHeader *FindGeoAnimHeader(u32 hash) const { return m_anims.Find(hash); }
    EXGeoAnimSkinHeader *GeoAnimSkinHeader(u32 hash) const { return m_skins.Find(hash); }
    int GeoAnimModeIndex(u32 hash) const { return m_modes.FindIndex(hash); }
    EXGeoAnimModeHeader *GeoAnimModeHeader(u32 hash) const { return m_modes.Find(hash); }
    EXGeoAnimSetHeader *GeoAnimSetHeader(u32 hash) const { return m_sets.Find(hash); }
    u8 m_00[0x58];
    EXGeoCommonArray<EXGeoAnimHeader> m_anims;      // +0x58
    EXGeoCommonArray<EXGeoAnimSkinHeader> m_skins;  // +0x60
    u8 m_68[0x10];
    EXGeoCommonArray<EXGeoAnimModeHeader> m_modes;  // +0x78
    EXGeoCommonArray<EXGeoAnimSetHeader> m_sets;    // +0x80
};
// TODO stand-in: geo file (EXGeoFile.obj).
class EXGeoFile {
public:
    static EXGeoFile *LoadGeoFile(u32 hash, s8 b, u32 flags);
    EXGeoHeader *Header() const { return m_pHeader; }
    u8 m_00[0x20];
    EXGeoHeader *m_pHeader;     // +0x20
};

// TODO stand-in: entity info array (EXGeoEntity area).
class EXGeoEntity_InfoArray {
public:
    EXGeoEntity_InfoArray();
    ~EXGeoEntity_InfoArray() { EXFree(m_pData); }
    u32 GetRandomVertexArray(EXVector *pOut, int n, u32 flags);
    u8 m_00[0xc];
    void *m_pData;          // +0xc
    EXMatrix m_mtx;         // +0x10
    u8 m_50[8];
};
// TODO stand-in: application timing (EXBaseApp.obj); really a protected static.
class EXBaseApp {
public:
    static float m_EngineRenderFrameRateMul;
};
// TODO stand-in: script animator (EXItemAnimator_Script.obj), sizeof 0x148.
class EXItemAnimator_Script : public EXItemAnimator {
public:
    u8 m_104[0x44];
};

// TODO stand-in: geo animation (EXGeoAnim.obj); the real class is in EXGeoAnim.h, which clashes with
// the geo stand-ins above.
class EXGeoBaseAnim {
public:
    void DecompressMotion(int frame, EXMotionData &m, unsigned char, unsigned int flags) const;
    u8 m_00[0xe];
    u16 m_NumFrames;    // +0xe
};

float AngleDiff(float a, float b);  // EXMaths.obj

EXFixedArray<EXRotPosInfo, 2> PDBaseInfo;

EX_IMPLEMENT_DYNCREATE(EXItemAnimator_Anim, EXItemAnimator)

void EXItemAnimator_Anim::CleanInitialise()
{
    for (int i = 0; i < m_numStacks; i++) m_stacks[i].KillStack(0);
    if (m_pSkin) delete m_pSkin;
    m_geoRef.Set(0, 0);
}

void EXItemAnimator_Anim::SetItemRender(EXItemRender_SkinAnim *p)
{
    m_pSkin = p;
    p->SetOwner(this);
}

void EXItemAnimator_Anim::DeleteStack(u32 i) { m_stacks[i].KillStack(0); }
void EXItemAnimator_Anim::DetatchParticleSys() const { m_effects.DetatchSystems(EXRUNTIME_CLASS(EXParticleSys)); }
void EXItemAnimator_Anim::DeleteParticleSys() const { m_effects.DeleteSystems(EXRUNTIME_CLASS(EXParticleSys)); }

EXParticleSys *EXItemAnimator_Anim::ParticleSys() const
{
    if (!m_effects.m_pHead) return 0;
    return (EXParticleSys *)m_effects.FindSystem(EXRUNTIME_CLASS(EXParticleSys));
}

void EXItemAnimator_Anim::EnableParticleSystems(s8 bEnable) const
{
    EXParticleSys *p = ParticleSys();
    if (p) p->m_bEnabled = bEnable;
}

void *EXItemAnimator_Anim::GetSkelDatum(u32 type, EXDatumTemp &d, u32 flags, void *&pCtx)
{
    void *p = 0;
    if (m_pSkin) {
        u32 t = type & 0x7f000000;
        if (t == 0x0e000000) p = m_pSkin->GetDatum_AnimBoneDatum(type, d, flags);
        else if (t == 0x10000000) p = m_pSkin->GetDatum_SkelDatum(type, d, pCtx, flags);
        if (p && !(flags & 4)) {
            EXMatrix m;
            CalcMatrix(m);
            d.ApplyXForm(m);
        }
    }
    return p;
}

void *EXItemAnimator_Anim::GetSkelDatum(const EXVector &pos, EXDatumTemp &d, u32 flags)
{
    void *p = 0;
    if (m_pSkin) {
        p = m_pSkin->GetDatum_AnimBoneDatum(pos, d, flags);
        if (p && !(flags & 4)) {
            EXMatrix m;
            CalcMatrix(m);
            d.ApplyXForm(m);
        }
    }
    return p;
}

EXItemRender_SkinAnimSection *EXItemAnimator_Anim::GetAnimBoneSection(u32 a, u32 b)
{
    if (!m_pSkin) return 0;
    return m_pSkin->GetAnimBoneSection(a, b);
}

void EXItemAnimator_Anim::SetRenderState(u32 state, s8 bOn)
{
    u32 t = state & 0x7f000000;
    if ((t == 0x02000000 || t == 0x0e000000) && m_pSkin) m_pSkin->SetRenderState(state, bOn);
    EXItemAnimator::SetRenderState(state, bOn);
}

void EXItemAnimator_Anim::BuildEntityInfoArray(const EXMatrix &mtx, EXGeoEntity_InfoArray &arr, s8 b)
{
    if (!m_pSkin) return;
    u32 f = 0;
    const EXMatrix &m = b ? CalcMatrix_InPlace(mtx, f) : m_mtx;
    m_pSkin->BuildEntityInfoArray(m, arr);
}

void EXItemAnimator_Anim::SetLodMin(u32 lod) { m_pSkin->SetLodMin(lod); }
void EXItemAnimator_Anim::SetLodMax(u32 lod) { m_pSkin->SetLodMax(lod); }

void EXItemAnimator_Anim::SetLodDetails(EXAnimLodDetails *p, u32 n)
{
    m_numLods = (u8)n;
    m_pLodDetails = p;
    m_lodFlags = p ? p->m_flags : 0;
    if (m_pSkin) m_pSkin->SetActiveLodDetails(p);
}

void EXItemAnimator_Anim::CalculateLodDetails()
{
    if (!m_pLodDetails || !g_LodViewsStandIn) {
        m_lodFlags = 0;
        return;
    }
    EXVector pos = m_mtx.GetTranslation();
    float d = EXItemEnv::m_pTheItemEnv->MinDistanceToItemViews(pos);
    int i;
    for (i = m_numLods - 1; i > 0; i--)
        if (d >= m_pLodDetails[i].m_dist) break;
    EXAnimLodDetails *p = &m_pLodDetails[i];
    m_lodFlags = p->m_flags;
    if (m_pSkin) m_pSkin->SetActiveLodDetails(p);
}

void EXItemAnimator_Anim::SetSkinRefObject(const EXGeoRefObject &r) { m_pSkin->SetSkinRefObject(r); }
void EXItemAnimator_Anim::SetSkinRefObject(EXGeoFile *pFile, u32 hash) { m_pSkin->SetSkinRefObject(pFile, hash); }
void EXItemAnimator_Anim::SetSkinRefObject(u32 hash) { m_pSkin->SetSkinRefObject(hash); }
void EXItemAnimator_Anim::SetShadowRefObject(const EXGeoComplexRefObject &r) { m_pSkin->SetShadowRefObject((const EXGeoBaseRefObject &)r); }
void EXItemAnimator_Anim::SetCollisionRefObject(const EXGeoComplexRefObject &r) { m_pSkin->SetCollisionRefObject((const EXGeoBaseRefObject &)r); }
void EXItemAnimator_Anim::SetReflectionRefObject(const EXGeoComplexRefObject &r) { m_pSkin->SetReflectionRefObject((const EXGeoBaseRefObject &)r); }
const EXGeoComplexRefObject &EXItemAnimator_Anim::SkelRefObject() const { return m_pSkin->SkelRefObject(); }
const EXGeoComplexRefObject &EXItemAnimator_Anim::SkinRefObject() const { return m_pSkin->SkinRefObject(); }
const EXGeoComplexRefObject &EXItemAnimator_Anim::ShadowRefObject() const { return m_pSkin->ShadowRefObject(); }
const EXGeoComplexRefObject &EXItemAnimator_Anim::ReflectionRefObject() const { return m_pSkin->ReflectionRefObject(); }
EXItemRender_SkinAnim *EXItemAnimator_Anim::AddCloneSkin(const EXGeoRefObject &r) { return m_pSkin->AddCloneSkin(r); }
void EXItemAnimator_Anim::DeleteCloneSkin(EXItemRender_SkinAnim *p, s8 b) { m_pSkin->DeleteCloneSkin(p, b); }

void EXItemAnimator_Anim::DeleteCloneSkin(const EXGeoRefObject &r, s8 b)
{
    EXItemRender_SkinAnim *p = m_pSkin->FindCloneSkin(r);
    if (p) m_pSkin->DeleteCloneSkin(p, b);
}

EXItemRender_SkinAnim *EXItemAnimator_Anim::FindCloneSkin(const EXGeoRefObject &r) const { return m_pSkin->FindCloneSkin(r); }

void EXItemAnimator_Anim::DoEntityScoop(const EXMatrix &mtx, EXCollisionScoopTemp &t, u32 flags)
{
    const EXMatrix &m = CalcMatrix_InPlace(mtx, flags);
    EXBoundsBox box = m_boundsBox;
    box._ApplyXForm(m);
    if (box.Intersect(t.m_centre, t.m_radius)) m_pSkin->DoEntityScoop(m, t, flags);
}

u32 EXItemAnimator_Anim::ApplyCtrlFunction(u32 (__cdecl *pFunc)(EXItemCtrl *, void *), void *pData)
{
    u32 r = 0;
    for (int i = 0; i < m_numStacks; i++) {
        for (EXItemCtrl *p = m_stacks[i].FirstCtrl(); p; p = p->Next())
            r |= pFunc(p, pData);
    }
    return r;
}

u32 EXItemAnimator_Anim::GetCtrlAnimFlags_Func(EXItemCtrl *p, void *)
{
    if (p->IsKindOf(EXRUNTIME_CLASS(EXItemCtrl_Anim))) return ((EXItemCtrl_Anim *)p)->GetCtrlAnimFlags();
    return 0;
}

void EXItemAnimator_Anim::UpdateSkinInformation(ex::EXBit128 &bits)
{
    if (m_pSkin) {
        EXVector pos = m_mtx.GetTranslation();
        m_pSkin->UpdateSkinInformation(3, pos);
        m_pSkin->UpdateSkinInformation(1, pos);
        bits = m_pSkin->CalculateSkinUsageBits(1);
        if (bits.IsAnySet()) return;
    }
    bits.SetAll();
}

EXItemAnimator_Anim::EXItemAnimator_Anim()
{
    m_pSkin = 0;
    m_pScriptCallbacks = 0;
    m_pSnapShotDatums = 0;
    m_frameRateMult = 1.0f;
    m_pLodDetails = 0;
    m_numLods = 0;
    m_lodFlags = 0;
    m_stackCtrlObjects = 0;
    m_numStacks = 1;
    for (int i = 0; i < 3; i++) m_stacks[i].SetStackSize(2);
}

EXGeoCommonObject *EXItemAnimator_Anim::GetGeoObject() const
{
    return (EXGeoCommonObject *)(m_pSkin ? m_pSkin->SkelRefObject() : m_geoRef).GeoAnim();
}

void EXItemAnimator_Anim::BuildSkel(u32 hash)
{
    // Copying through a temporary keeps the original's (dead) store of the unchanged file pointer.
    EXGeoRefObject skel(GeoFile(), hash);
    EXGeoComplexRefObject &ref = GeoRefObject();
    ref.m_pGeoFile = skel.m_pGeoFile;
    ref.m_hashRef = skel.m_hashRef;
    ref.m_08 = -1;
    EXGeoFile *pFile = skel.m_pGeoFile;
    EXGeoAnimHeader *h = ref.GeoAnimHeader();
    if (!h) return;
    EXGeoRefObject r(pFile, *(u32 *)((char *)h + 0x1c));
    EXItemRender_SkinAnim *p = new EXItemRender_SkinAnim;
    if (!p) for (;;) {}
    SetItemRender(p);
    SetLodDetails(0, 0);
    p->Create(r);
}

void EXItemAnimator_Anim::InitControllerStack(u32 i, u32 size, u32 flags)
{
    m_stacks[i].SetFlags((u16)flags);
    m_stacks[i].SetStackSize((u8)size);
}

EXItemCtrl *EXItemAnimator_Anim::AddController(u32 i, EXItemCtrl *p)
{
    m_numStacks = max<u32>(m_numStacks, i + 1);
    m_stacks[i].m_list.AddHead(p);
    return p;
}

EXItemCtrl *EXItemAnimator_Anim::NewController(u32 i, u32 hash, u32 flags)
{
    EXGeoFile *pFile = m_geoRef.GeoFile();
    if (!pFile) return 0;
    if (!*(u32 *)((char *)pFile + 0x20)) return 0;
    EXItemCtrl *p = 0;
    switch ((hash >> 24) & 0x7f) {
    case 3:
        if (m_geoRef.HashRef() == 0xffffffff) BuildSkel(hash);
        p = new EXItemCtrl_Anim(pFile, hash);
        break;
    case 4:
        p = new EXItemCtrl_AnimScript(pFile, hash);
        break;
    }
    if (p) {
        p->CtrlFlags() = flags;
        p->SetCtrlStack(&m_stacks[i]);
        m_geoRef.Set(pFile, hash);
        p->Initialise(this);
    }
    return p;
}

EXItemCtrl *EXItemAnimator_Anim::ChangeTo(u32 i, u32 hash, u32 flags)
{
    EXItemCtrl *p = NewController(i, hash, flags);
    if (p) {
        EXItemCtrlStack *pStack = &m_stacks[i];
        pStack->KillStack(0);
        pStack->m_list.AddHead(p);
        m_numStacks = max<u32>(m_numStacks, i + 1);
    }
    return p;
}

EXItemCtrl *EXItemAnimator_Anim::ConvertToSnapShot(u32 i)
{
    EXItemCtrl_SnapShot *p = new EXItemCtrl_SnapShot(this);
    for (int j = 0; j < m_numStacks; j++) m_stacks[j].KillStack(0);
    if (p) {
        EXItemCtrlStack *pStack = &m_stacks[i];
        pStack->m_list.AddHead(p);
        m_numStacks = max<u32>(m_numStacks, i + 1);
    }
    return p;
}

EXItemCtrl *EXItemAnimator_Anim::ConvertToSnapShotMulti(u32 mask)
{
    EXItemCtrl_SnapShot *pSnap = mask ? new EXItemCtrl_SnapShot(this) : 0;
    s8 bFirst = 1;
    for (int i = 0; i < m_numStacks; i++) {
        m_stacks[i].KillStack(0);
        if (mask & (1 << i)) {
            if (bFirst) {
                bFirst = 0;
                m_stacks[i].m_list.AddHead(pSnap);
            } else {
                EXItemCtrl_SnapShot *p = new EXItemCtrl_SnapShot;
                p->Clone(pSnap);
                m_stacks[i].m_list.AddHead(p);
            }
            m_numStacks = max<u32>(m_numStacks, i + 1);
        }
    }
    return pSnap;
}

EXItemCtrl *EXItemAnimator_Anim::ConvertStackToSnapShot(u32 i)
{
    EXItemCtrlStack *pStack = &m_stacks[i];
    EXItemCtrl_SnapShot *p = new EXItemCtrl_SnapShot(this, pStack);
    if (p) {
        pStack->KillStack(0);
        pStack->m_list.AddHead(p);
        m_numStacks = max<u32>(m_numStacks, i + 1);
    }
    return p;
}

void EXItemAnimator_Anim::DeleteUnusedStacks(const EXGeoAnimSetHeader *pSet)
{
    for (int i = 0; i < m_numStacks; i++) {
        const EXGeoAnimSet *e = pSet->FirstSet();
        for (u32 j = 0; j < pSet->m_numSets; j++) {
            if (i == e->m_stack) goto next;
            e = e->Next();
        }
        if (!m_stacks[i].Flags().BitTest(0x800)) m_stacks[i].KillStack(0);
    next:;
    }
}

void EXItemAnimator_Anim::GetSkelDatum(u32 type, EXDatumTempArray &arr, u32 flags)
{
    if (!m_pSkin) return;
    int n0 = arr.Size();
    if ((type & 0x7f000000) == 0x10000000) m_pSkin->GetDatum_SkelDatum(type, arr, flags);
    if (!(flags & 4)) {
        int n = arr.Size() - n0;
        if (n) {
            EXMatrix m;
            CalcMatrix(m);
            for (int i = 0; i < n; i++) arr[n0 + i].ApplyXForm(m);
        }
    }
}

void EXItemAnimator_Anim::GetDatum(u32 type, EXDatumTempArray &arr, u32 flags)
{
    GetSkelDatum(type, arr, flags);
    if (flags & 0x80) GetDatum_ChildLink(type, arr, flags);
}

void *EXItemAnimator_Anim::GetDatum(u32 type, EXDatumTemp &d, u32 flags, void *&pCtx)
{
    void *p = (flags & 0x10) ? GetFrameDatum(type, d, flags) : GetSkelDatum(type, d, flags, pCtx);
    if (!p && (flags & 0x80)) p = GetDatum_ChildLink(type, d, flags, pCtx);
    return p;
}

void *EXItemAnimator_Anim::GetNearestDatum(const EXVector &pos, EXDatumTemp &d, u32 flags)
{
    void *p = GetSkelDatum(pos, d, flags);
    if (!p && (flags & 0x80)) p = GetNearestDatum_ChildLink(pos, d, flags);
    return p;
}

void EXItemAnimator_Anim::GetGeoBoundsBox(EXBoundsBox &box)
{
    EXGeoCommonObjectStandIn *p = (EXGeoCommonObjectStandIn *)GetGeoObject();
    if (!p) {
        box.SetZero();
        return;
    }
    box.Set(p->GetBoundsBox());
}

s8 EXItemAnimator_Anim::_GetBoundsBox(EXBoundsBox &box, u32 flags)
{
    EXBoundsBox b;
    b.SetMaxRange();
    s8 bFound = 0;
    for (int i = 0; i < m_numStacks; i++) {
        if (m_stacks[i].Blend() < 0.001f) continue;
        EXItemCtrl *p = m_stacks[i].FirstCtrl();
        if (p) {
            p->GetBoundsBox(b, 1);
            bFound = 1;
        }
    }
    if (!bFound) b.SetZero();
    ApplyMatrixBoundsBox(b, box, flags);
    return bFound;
}

u32 EXItemAnimator_Anim::GetCtrlAnimFlags() { return ApplyCtrlFunction(GetCtrlAnimFlags_Func, 0); }

u32 EXItemAnimator_Anim::SetFrameDepth_Func(EXItemCtrl *p, void *pData)
{
    if (p->IsKindOf(EXRUNTIME_CLASS(EXItemCtrl_Anim))) {
        float f = *(float *)pData;
        ((EXItemCtrl_Anim *)p)->FrameDepth().ResetPtr();
        ((EXItemCtrl_Anim *)p)->SetFrameDepth(f);
        return 0;
    }
    return 0;
}

u32 EXItemAnimator_Anim::SetFrameDepth(float f) { return ApplyCtrlFunction(SetFrameDepth_Func, &f); }

s8 EXItemAnimator_Anim::RemoveSnapShotDatumReference(u32 id)
{
    if (m_pSnapShotDatums) {
        for (int i = 0; i < m_pSnapShotDatums->Size(); i++) {
            if (m_pSnapShotDatums->ElementAt(i) == id) {
                m_pSnapShotDatums->RemoveAt(i, 1);
                return 1;
            }
        }
    }
    return 0;
}

EXItemAnimator_Anim::~EXItemAnimator_Anim()
{
    m_effects.DetatchSystems();
    if (m_pSnapShotDatums) {
        delete m_pSnapShotDatums;
        m_pSnapShotDatums = 0;
    }
    CleanInitialise();
}

void EXItemAnimator_Anim::ForceUpdate()
{
    for (int i = 0; i < m_numStacks; i++) {
        EXItemCtrlStack &stack = m_stacks[i];
        stack.UpdateStack();
        for (EXItemCtrl *p = stack.LastCtrl(); p; p = p->Prev()) p->Update(this);
    }
    UpdateRenderHierarchy();
}

u32 EXItemAnimator_Anim::ParticlePosCallback(EXItemAnimator *pAnim, EXVector *pOut, u32 n)
{
    EXGeoEntity_InfoArray arr;
    pAnim->BuildEntityInfoArray(pAnim->m_mtx, arr, 0);
    return arr.GetRandomVertexArray(pOut, n, 0);
}

void EXItemAnimator_Anim::SetObjectFrame(float f)
{
    for (int i = 0; i < m_numStacks; i++)
        for (EXItemCtrl *p = m_stacks[i].FirstCtrl(); p; p = p->Next()) p->SetObjectFrame(f);
    ForceUpdate();
}

void EXItemAnimator_Anim::SetObjectTime(float t)
{
    for (int i = 0; i < m_numStacks; i++)
        for (EXItemCtrl *p = m_stacks[i].FirstCtrl(); p; p = p->Next()) p->SetObjectTime(t, 0);
    ForceUpdate();
}

void EXItemAnimator_Anim::UpdateObjectTime(float dt)
{
    s8 old = EXItemCtrl::SetForceUpdateObjectTime(1);
    for (int i = 0; i < m_numStacks; i++)
        for (EXItemCtrl *p = m_stacks[i].FirstCtrl(); p; p = p->Next()) p->SetObjectTime(dt, 1);
    ForceUpdate();
    EXItemCtrl::SetForceUpdateObjectTime(old);
}

s8 EXItemAnimator_Anim::AssignSkelSkinAnim(const EXGeoRefObject &a, const EXGeoRefObject &b, const EXGeoRefObject &c)
{
    Initialise(c, b);
    m_pSkin->SetSkinRefObject(a);
    return 1;
}

// NEAR: 44 bytes; the failure block is laid out after the success call instead of before it.
s8 EXItemAnimator_Anim::Initialise(EXGeoFile *pFile, u32 hash)
{
    if (!pFile) return 0;
    if (!pFile->Header()) return 0;
    const EXGeoCommonArray<EXGeoAnimHeader> &anims = pFile->Header()->m_anims;
    int i = anims.FindIndex(hash);
    EXGeoAnimHeader *h = i < 0 ? 0 : &((EXGeoAnimHeader *)anims.m_data)[i];
    if (h && h->m_pAnim) {
        EXGeoRefObject anim(pFile, hash);
        EXGeoRefObject skin(pFile, hash);
        return Initialise(anim, skin);
    }
    return 0;
}

// NEAR: 80 bytes; the shared return-0 block lands at the end instead of after the first test.
u32 EXItemAnimator_Anim::ChangeToAnimMode(u32 mode, u32 a, u32 b)
{
    EXGeoFile *pFile = m_geoRef.GeoFile();
    EXGeoHeader *pHdr;
    if (!pFile || !(pHdr = pFile->Header())) return 0;
    int idx = pHdr->GeoAnimModeIndex(mode);
    if (idx < 0) return 0;
    u32 r;
    const EXGeoAnimModeHeader *m = pHdr->GeoAnimModeHeader(a);
    if (m && (r = ChangeToAnimMode(m, idx, pHdr)) != 0) return r;
    m = pHdr->GeoAnimModeHeader(b);
    if (m && (r = ChangeToAnimMode(m, idx, pHdr)) != 0) return r;
    return 0;
}

void EXItemAnimator_Anim::DoUpdate()
{
    CalculateLodDetails();
    UpdateControllers();
    UpdateRenderHierarchy();
    UpdateNodes();
}

s8 EXItemAnimator_Anim::AttachParticleSys(EXGeoFile *pFile, u32 hash, const EXRuntimeClass *pClass)
{
    EXBaseParticleSys *pSys = (EXBaseParticleSys *)m_effects.AttachParticleSys(this, pFile, hash, pClass);
    if (!pSys) return 0;
    pSys->SetParticlePosCallback(ParticlePosCallback);
    return 1;
}

s8 EXItemAnimator_Anim::AssignSkelSkinAnim(int a, int b, int c, int d, int e, int f)
{
    EXGeoFile *pA = EXGeoFile::LoadGeoFile(a, 1, 0);
    EXGeoFile *pC = EXGeoFile::LoadGeoFile(c, 1, 0);
    EXGeoFile *pE = EXGeoFile::LoadGeoFile(e, 1, 0);
    if (!pC || !pA || !pE) return 0;
    EXGeoRefObject skin(pC, d);
    Initialise(EXGeoRefObject(pE, f), EXGeoRefObject(pA, b));
    m_pSkin->SetSkinRefObject(skin);
    return 1;
}

void EXItemAnimator_Anim::AddSnapShotDatumReference(u32 id)
{
    if (!m_pSnapShotDatums) m_pSnapShotDatums = new EXu32Array;
    m_pSnapShotDatums->Add(id);
}

EXItemCtrl *EXItemAnimator_Anim::BlendTo(u32 i, u32 hash, u32 flags, u32 frames)
{
    EXItemCtrl *p = NewController(i, hash, flags);
    if (p) {
        if (flags & 4) {
            EXItemCtrl *pCur = Controller(i);
            if (pCur) p->SetFrame(pCur);
        }
        if (flags & 8) ConvertStackToSnapShot(i);
        AddController(i, p);
        p->SetBlend(0.0f);
        p->SetBlendRateFrames(frames);
    }
    return p;
}

s8 EXItemAnimator_Anim::CheckAnimMode(u32 hash) const
{
    EXGeoFile *pFile = m_geoRef.GeoFile();
    if (!pFile) return 0;
    if (!pFile->Header()) return 0;
    return pFile->Header()->GeoAnimModeIndex(hash) >= 0;
}

// NEAR: 260 bytes; header/param registers swapped (ebp/edi), hash copied through eax in the original.
s8 EXItemAnimator_Anim::Initialise(const EXGeoRefObject &anim, const EXGeoRefObject &skinRef)
{
    CleanInitialise();
    EXGeoHeader *pHdr;
    EXGeoRefObject skel = skinRef;
    if (!skel.m_pGeoFile || !(pHdr = skel.m_pGeoFile->Header())) return 0;
    if ((skel.m_hashRef & 0x7f000000) != 0x0d000000) {
        EXGeoAnimHeader *h = pHdr->GeoAnimHeader(skel.m_hashRef);
        if (!h) return 0;
        EXGeoAnimSkinHeader *s = pHdr->GeoAnimSkinHeader(h->SkinNum());
        if (!s) return 0;
        skel.m_hashRef = s->BaseSkinNum();
    }
    EXItemRender_SkinAnim *p = new EXItemRender_SkinAnim;
    if (!p) return 0;
    if (!p->Create(skel)) {
        delete p;
        return 0;
    }
    SetItemRender(p);
    SetLodDetails(0, 0);
    EXGeoRefObject skin = skinRef;
    if ((skin.m_hashRef & 0x7f000000) != 0x0d000000) {
        if (!skin.m_pGeoFile || !(pHdr = skin.m_pGeoFile->Header())) return 0;
        EXGeoAnimHeader *h = pHdr->GeoAnimHeader(skin.m_hashRef);
        if (!h) return 0;
        skin.m_hashRef = h->SkinNum();
    }
    p->SetSkinRefObject(skin);
    m_geoRef.Set(anim);
    ChangeTo(0, anim.m_hashRef, 3);
    SetObjectTime(0.0f);
    return 1;
}

void EXItemAnimator_Anim::UpdateControllers()
{
    float dt = (float)fabs(EXBaseApp::m_EngineRenderFrameRateMul * m_frameRateMult);
    for (int i = 0; i < m_numStacks; i++) {
        EXItemCtrlStack &stack = m_stacks[i];
        stack.UpdateStack();
        for (EXItemCtrl *p = stack.LastCtrl(); p; p = p->Prev()) {
            p->UpdateBlendFracs(dt);
            p->Update(this);
        }
    }
    m_itemFlags.BitClr(8);
}

void EXItemAnimator_Anim::DoRender_ControllerObjects(const EXMatrix &mtx, const EXRenderInfo &info, u32 flags)
{
    m_stackCtrlObjects = 0;
    for (int i = 0; i < m_numStacks; i++) {
        EXItemCtrlStack &stack = m_stacks[i];
        stack.UpdateStack();
        for (EXItemCtrl *p = stack.FirstCtrl(); p; p = p->Next()) {
            if (p->m_ctrlFlags & 0x100000) {
                EXItemAnimator_Script *pScript = ((EXItemCtrl_AnimScript *)p)->ScriptAnimator();
                if (pScript) {
                    pScript->DoRender(mtx, info, flags);
                    if (p->m_ctrlFlags & 0x400000) {
                        pScript->m_renderInfo.RenderFlagsRef().BitState(2, 0.99f > *p->m_blend.m_p);
                        pScript->m_renderInfo.m_rgbaMult.Set(1.0f, 1.0f, 1.0f, *p->m_blend.m_p);
                    }
                    m_stackCtrlObjects = 1;
                }
            }
            if (p->m_ctrlFlags & 0x200000) {
                const EXSoundTag &tag = ((EXItemCtrl_Anim *)p)->SoundTag();
                if (tag.m_pHead) {
                    tag.UpdatePosition(mtx.GetTranslation());
                    m_stackCtrlObjects = 1;
                }
            }
        }
    }
}

// NEAR: 246 bytes; pInfo kept as an absolute pointer in the original (ours becomes an offset), register choice.
void EXItemAnimator_Anim::UpdateRenderHierarchy()
{
    EXItemRender_SkinAnim *pSkin = m_pSkin;
    EXScratchPad pad;
    EXMotionData *pMotion = (EXMotionData *)pad.Claim(0x2200);
    PDBaseInfo[0].m_pMotion = pMotion;
    PDBaseInfo[0].m_pMorph = (float *)(pMotion + 0x100);
    PDBaseInfo[1].m_pMotion = pMotion + 0x80;
    PDBaseInfo[1].m_pMorph = PDBaseInfo[0].m_pMorph + 0x40;
    EXItemCtrlStack::ms_UpdateFlags = 0;
    EXRotPosInfo *pInfo = PDBaseInfo;
    int n = 0;
    for (int i = 0; i < m_numStacks; i++) {
        EXItemCtrlStack &stack = m_stacks[i];
        if (stack.Blend() < 0.001f) continue;
        EXItemCtrl *pCtrl = stack.LastCtrl();
        if (pCtrl) {
            pSkin->InitBlendStack(pInfo, 0);
            EXItemCtrlStack::ms_BoneUsedBits.ClrAll();
            EXItemCtrlStack::ms_DeformerUsedBits.ClrAll();
            int k = 0;
            for (; pCtrl; pCtrl = pCtrl->Prev())
                if (pCtrl->UpdateCtrlOnStack(this, &stack, pInfo, k)) k++;
            if (n && (EXItemCtrlStack::ms_UpdateFlags & 0x8000)) BlendStacks(&stack);
            n++;
        }
        EXItemCtrlStack::ms_UpdateFlags &= ~0x8000;
        if (n) pInfo = &PDBaseInfo[1];
    }
    if (EXItemCtrlStack::ms_UpdateFlags & 0x4000) pSkin->UpdateRenderEnd();
}

void EXItemAnimator_Anim::BlendToAnimSet(const EXGeoAnimSetHeader *pSet, u32 frames, u32 flags, u32 mode)
{
    const EXGeoAnimSet *e = pSet->FirstSet();
    u32 acc = 0;
    for (u32 i = 0; i < pSet->m_numSets; i++) {
        u32 s = e->m_stack;
        if (s == 0) m_stacks[0].SetBlend(1.0f);
        else if (e->m_blend == 0.0f) m_stacks[s].SetBlend(0.0f);
        for (int j = 0; j < e->m_count; j++) {
            const EXGeoAnimSetItem &it = e->m_items[j];
            u32 f = frames;
            if (mode == 0xc000003) f = it.m_frames + f;
            else if (!f) f = it.m_frames;
            u32 fl = flags;
            if (!fl) fl = it.m_flags;
            acc |= fl;
            EXItemCtrl *p = BlendTo(s, it.m_hash, fl, f);
            if (p) p->SetBlendSpline(it.m_spline);
        }
        e = e->Next();
    }
    if (!(acc & 0x20)) DeleteUnusedStacks(pSet);
}

// NEAR: 61 bytes; after the inner loop the original reloads the set pointer from its stack slot
// (ours reuses the register copy), which shifts the tail.
void EXItemAnimator_Anim::ChangeToAnimSet(const EXGeoAnimSetHeader *pSet, u32 flags)
{
    const EXGeoAnimSet *e = pSet->FirstSet();
    u32 acc = 0;
    for (u32 i = 0; i < pSet->m_numSets; i++) {
        u32 s = e->m_stack;
        float b = 1.0f;
        if (s) b = e->m_blend;
        m_stacks[s].SetBlend(b);
        for (int j = 0; j < e->m_count; j++) {
            const EXGeoAnimSetItem &it = e->m_items[j];
            u32 fl = flags;
            if (!fl) fl = it.m_flags;
            acc |= fl;
            ChangeTo(s, it.m_hash, fl);
        }
        e = e->Next();
    }
    if (!(acc & 0x20)) DeleteUnusedStacks(pSet);
}

// Render the skin (frustum culled against the bounds box when drawing), the effects, the stack
// controller objects and the child nodes. Returns 1 if the skin was drawn, 2 otherwise, plus the
// children's result.
u32 EXItemAnimator_Anim::DoRender(const EXMatrix &mtx, const EXRenderInfo &info, u32 flags)
{
    u32 result = 2;
    if (EXBaseRenderEnv::RenderState() & 1) {
        CalcMatrix_InPlace(mtx, flags);
        UpdateBoundsBox(0);
    }
    EXRenderInfo ri(m_renderInfo, info);
    EXBaseDisplay *pDisplay = EXBaseDisplay::GetDisplay();
    if (m_pSkin) {
        bool draw = true;
        if (EXBaseRenderEnv::RenderState() & 2) {
            EXBaseDisplay::SetObjectToWorld(m_mtx);
            pDisplay->UpdateTransforms();
            draw = EXDisplay::CullBox((const float *)&m_boundsBox, EXBaseDisplay::ObjectToCull()) != 0;
        }
        if (draw) {
            m_pSkin->DoRender(m_mtx, ri);
            result = 1;
        }
        if ((EXBaseRenderEnv::RenderState() & 2) && m_effects.m_pHead) m_effects.UpdateRenderInfo(ri);
        if (m_stackCtrlObjects) DoRender_ControllerObjects(m_mtx, ri, flags);
    }
    return RenderNodes(m_mtx, ri, flags) | result;
}

// Datum of the current frame from one controller stack: the first controller that has it, with
// each later controller's result blended in by its blend weight.
void *EXItemAnimator_Anim::GetFrameDatum_CtrlStack(const EXItemCtrlStack *pStack, u32 id, EXDatumTemp &d, u32 flags)
{
    void *pResult = 0;
    EXDatumTemp tmp;
    for (EXItemCtrl *pCtrl = pStack->FirstCtrl(); pCtrl; pCtrl = pCtrl->Next()) {
        void *p = pCtrl->GetFrameDatum(this, id, pResult ? tmp : d, flags);
        if (!p) continue;
        if (!pResult) {
            pResult = p;
            continue;
        }
        float t = pCtrl->m_blend;
        d.m_position.x = (tmp.m_position.x - d.m_position.x) * t + d.m_position.x;
        d.m_position.y = (tmp.m_position.y - d.m_position.y) * t + d.m_position.y;
        d.m_position.z = (tmp.m_position.z - d.m_position.z) * t + d.m_position.z;
        d.m_position.w = (tmp.m_position.w - d.m_position.w) * t + d.m_position.w;
        EXMaths::quat_xyzw q = d.m_rotation.slerp(tmp.m_rotation, t);
        d.m_rotation.x = q.x;
        d.m_rotation.y = q.y;
        d.m_rotation.z = q.z;
        d.m_rotation.w = q.w;
    }
    return pResult;
}

// Datum of the current frame blended over the controller stacks (stacks with a blend below 0.001
// are skipped), in world space unless flags bit 4.
void *EXItemAnimator_Anim::GetFrameDatum(u32 id, EXDatumTemp &d, u32 flags)
{
    void *pResult = 0;
    EXDatumTemp tmp;
    if (!m_numStacks) return 0;
    for (int i = 0; i < m_numStacks; i++) {
        float blend = m_stacks[i].m_blend;
        if (!(blend >= 0.001f)) continue;
        void *p = GetFrameDatum_CtrlStack(&m_stacks[i], id, pResult ? tmp : d, flags);
        if (!p) continue;
        if (!pResult) {
            pResult = p;
            continue;
        }
        float t = m_stacks[i].m_blend;
        d.m_position.x = (tmp.m_position.x - d.m_position.x) * t + d.m_position.x;
        d.m_position.y = (tmp.m_position.y - d.m_position.y) * t + d.m_position.y;
        d.m_position.z = (tmp.m_position.z - d.m_position.z) * t + d.m_position.z;
        d.m_position.w = (tmp.m_position.w - d.m_position.w) * t + d.m_position.w;
        EXMaths::quat_xyzw q = d.m_rotation.slerp(tmp.m_rotation, t);
        d.m_rotation.x = q.x;
        d.m_rotation.y = q.y;
        d.m_rotation.z = q.z;
        d.m_rotation.w = q.w;
    }
    if (pResult && !(flags & 4)) {
        EXMatrix m;
        CalcMatrix(m);
        d.ApplyXForm(m);
    }
    return pResult;
}

// One function of an animation mode (EXGeoAnimMode data, sizeof 0x10).
struct EXGeoAnimModeFunction {
    u32 m_Type;         // 0x00 0xc000002 blend to set, 3 blend (frame count checked), 4 change to set,
                        //      5 snapshot stacks, 6 handler function
    u32 m_SetHash;      // 0x04 anim set
    u16 m_Arg;          // 0x08 flags / stack mask
    u16 m_Flags;        // 0x0a bit 0: m_Frames is valid
    int m_Frames;       // 0x0c blend frames
};
// The functions of one mode: count, then relative pointers to the functions.
struct EXGeoAnimModeFunctionList {
    u32 m_Count;
    ex::EXRelPtr<EXGeoAnimModeFunction> m_Funcs[1];
};
// Mode table entry: mode index and its function list.
struct EXGeoAnimModeEntry {
    int m_Mode;
    ex::EXRelPtr<EXGeoAnimModeFunctionList> m_pList;
};

// Run the functions of mode `mode` in this anim mode table; returns the table's hash, or 0 if it
// has no such mode.
u32 EXItemAnimator_Anim::ChangeToAnimMode(const EXGeoAnimModeHeader *pModes, int mode, const EXGeoHeader *pHdr)
{
    const EXGeoAnimModeEntry *pEntries = *(EXGeoAnimModeEntry *const *)&pModes->m_00[0xc];
    for (u32 i = 0; i < pModes->m_numModes; i++) {
        if (pEntries[i].m_Mode != mode) continue;
        EXGeoAnimModeFunctionList *pList = pEntries[i].m_pList;
        for (u32 f = 0; f < pList->m_Count; f++) {
            EXGeoAnimModeFunction *pFunc = pList->m_Funcs[f];
            switch (pFunc->m_Type) {
            case 0xc000002:
                BlendToAnimSet(pHdr->GeoAnimSetHeader(pFunc->m_SetHash), (pFunc->m_Flags & 1) ? pFunc->m_Frames : 0,
                               pFunc->m_Arg, pFunc->m_Type);
                break;
            case 0xc000003: {
                const EXGeoAnimSetHeader *pSet = pHdr->GeoAnimSetHeader(pFunc->m_SetHash);
                u32 frames = ((pFunc->m_Flags & 1) && pFunc->m_Frames >= 1) ? pFunc->m_Frames : 0;
                BlendToAnimSet(pSet, frames, pFunc->m_Arg, pFunc->m_Type);
                break;
            }
            case 0xc000004:
                ChangeToAnimSet(pHdr->GeoAnimSetHeader(pFunc->m_SetHash), pFunc->m_Arg);
                break;
            case 0xc000005:
                ConvertToSnapShotMulti(pFunc->m_Arg);
                break;
            case 0xc000006: {
                EXItemHandler *pHandler = GetOwnerHandler();
                if (pHandler) pHandler->DoAnimModeFunction(pHdr->GeoAnimSetHeader(pFunc->m_SetHash), pFunc, this);
                break;
            }
            }
        }
        return *(const u32 *)pModes->m_00;
    }
    return 0;
}

// Motion of the requested bones blended over the controller stacks: the first stack whose first
// controller produces a motion sets it, later ones are blended in by their stack blend. Stacks
// below 0.001 blend or without section 0 are skipped. Returns the first controller's result
// (8: no motion).
u8 EXItemAnimator_Anim::CalcMotionData_CtrlStacks(EXMotionData *pData, u32 flags, const u32 *pIds, int n) const
{
    EXCalcMotionTemp t;
    t.m_flags = flags;
    t.m_lod = m_lodFlags;
    t.m_rate = m_frameRateMult;
    t.m_count = n;
    for (int i = 0; i < n; i++) {
        t.m_ids[i] = pIds[i];
        pData[i].m_pos.x = 0.0f;
        pData[i].m_pos.y = 0.0f;
        pData[i].m_pos.z = 0.0f;
        pData[i].m_pos.w = 0.0f;
        pData[i].m_rot.x = 0.0f;
        pData[i].m_rot.y = 0.0f;
        pData[i].m_rot.z = 0.0f;
        pData[i].m_rot.w = 1.0f;
    }
    int found = 0;
    u8 result = 8;
    EXMotionData tmp[2];
    for (int s = 0; s < m_numStacks; s++) {
        const EXItemCtrlStack &stack = m_stacks[s];
        if (!(stack.m_blend >= 0.001f) || !(stack.m_sectionBits.m_bits[0] & 1)) continue;
        EXItemCtrl *pCtrl = stack.FirstCtrl();
        if (!pCtrl) continue;
        if (!found) {
            result = pCtrl->CalcMotionData(pData, t);
            if (result != 8) found++;
        } else if (pCtrl->CalcMotionData(tmp, t) != 8) {
            float b = stack.m_blend;
            for (int i = 0; i < n; i++) {
                pData[i].m_pos.x = (tmp[i].m_pos.x - pData[i].m_pos.x) * b + pData[i].m_pos.x;
                pData[i].m_pos.y = (tmp[i].m_pos.y - pData[i].m_pos.y) * b + pData[i].m_pos.y;
                pData[i].m_pos.z = (tmp[i].m_pos.z - pData[i].m_pos.z) * b + pData[i].m_pos.z;
                pData[i].m_pos.w = (tmp[i].m_pos.w - pData[i].m_pos.w) * b + pData[i].m_pos.w;
                EXMaths::quat_xyzw q = pData[i].m_rot.slerp(tmp[i].m_rot, b);
                pData[i].m_rot.x = q.x;
                pData[i].m_rot.y = q.y;
                pData[i].m_rot.z = q.z;
                pData[i].m_rot.w = q.w;
            }
            found++;
        }
    }
    return result;
}

static inline void BlendMotion(EXMotionData &a, const EXMotionData &b, float t)
{
    EXVector p;
    p.x = (b.m_pos.x - a.m_pos.x) * t + a.m_pos.x;
    p.y = (b.m_pos.y - a.m_pos.y) * t + a.m_pos.y;
    p.z = (b.m_pos.z - a.m_pos.z) * t + a.m_pos.z;
    p.w = (b.m_pos.w - a.m_pos.w) * t + a.m_pos.w;
    a.m_pos.x = p.x;
    a.m_pos.y = p.y;
    a.m_pos.z = p.z;
    a.m_pos.w = p.w;
    EXMaths::quat_xyzw q = a.m_rot.slerp(b.m_rot, t);
    a.m_rot.x = q.x;
    a.m_rot.y = q.y;
    a.m_rot.z = q.z;
    a.m_rot.w = q.w;
}

// Blend the pose just built for pStack (PDBaseInfo[1]) into the accumulated pose (PDBaseInfo[0]) by
// the stack's blend: the bones the stack touched (only those in the active LOD's skin, or in the
// stack's selective sections) and the deformers it touched.
void EXItemAnimator_Anim::BlendStacks(const EXItemCtrlStack *pStack) const
{
    EXItemRender_SkinAnim *pSkin = m_pSkin;
    float t = pStack->m_blend;
    int numBones = pSkin->m_numBones;
    ex::EXBit128 bones;
    bones.Set(EXItemCtrlStack::ms_BoneUsedBits);
    for (int w = 0; w < 4; w++) {
        if (!bones.m_bits[w]) continue;
        u8 selective = (pStack->m_flags >> 1) & 1;
        if (m_lodFlags & 0x80) {
            const EXAnimLodDetails *pLod = pSkin->m_pActiveLod;
            bones.m_bits[0] = EXItemCtrlStack::ms_BoneUsedBits.m_bits[0] & pLod->m_skinBits.m_bits[0];
            bones.m_bits[1] = EXItemCtrlStack::ms_BoneUsedBits.m_bits[1] & pLod->m_skinBits.m_bits[1];
            bones.m_bits[2] = EXItemCtrlStack::ms_BoneUsedBits.m_bits[2] & pLod->m_skinBits.m_bits[2];
            bones.m_bits[3] = EXItemCtrlStack::ms_BoneUsedBits.m_bits[3] & pLod->m_skinBits.m_bits[3];
            selective = 1;
        }
        EXMotionData *pDst = PDBaseInfo[0].m_pMotion;
        const EXMotionData *pSrc = PDBaseInfo[1].m_pMotion;
        if (!selective) {
            for (int i = 0; i < numBones; i++) BlendMotion(pDst[i], pSrc[i], t);
        } else {
            for (int i = 0; i < numBones; i++)
                if (bones.IsSet(i)) BlendMotion(pDst[i], pSrc[i], t);
        }
        break;
    }
    float *pDst = PDBaseInfo[0].m_pMorph;
    const float *pSrc = PDBaseInfo[1].m_pMorph;
    int numMorphs = pSkin->m_numMorphs;
    for (int i = 0; i < numMorphs; i++)
        if (EXItemCtrlStack::ms_DeformerUsedBits.IsSet(i)) pDst[i] = (pSrc[i] - pDst[i]) * t + pDst[i];
}

// Hamilton product a * b.
static inline EXMaths::quat_xyzw QuatMul(const EXMaths::quat_xyzw &a, const EXMaths::quat_xyzw &b)
{
    EXMaths::quat_xyzw r;
    r.x = a.x * b.w + a.w * b.x + a.y * b.z - a.z * b.y;
    r.y = a.y * b.w + a.w * b.y + a.z * b.x - a.x * b.z;
    r.z = a.z * b.w + a.w * b.z + a.x * b.y - a.y * b.x;
    r.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
    return r;
}

static inline EXMaths::quat_xyzw QuatConj(const EXMaths::quat_xyzw &q)
{
    EXMaths::quat_xyzw r;
    r.x = 0.0f - q.x;
    r.y = 0.0f - q.y;
    r.z = 0.0f - q.z;
    r.w = q.w;
    return r;
}

// Motion between two bones / datums of the current pose: 0..2 one id (root / ...), 3 ids 2 -> 3,
// 4 ids 1 -> 2, 5 the same with flag 0x40. Flags: 1 rotation as euler differences, 2 position in the
// first motion's frame, 4 scaled by the animator scale. Returns the controllers' result (8: none).
u8 EXItemAnimator_Anim::CalcMotionData(EXMotionData &m, u32 type, u32 flags) const
{
    static const u32 s_Ids0[2] = { 2, 0 };
    static const u32 s_Ids1[2] = { 1, 0 };
    static const u32 s_Ids2[2] = { 3, 0 };
    static const u32 s_Ids3[2] = { 2, 3 };
    static const u32 s_Ids4[2] = { 1, 2 };
    EXMotionData pair[2];
    u8 result;
    int ref;
    switch (type) {
    case 0:
        return CalcMotionData_CtrlStacks(&m, flags | 0x80, s_Ids0, 1);
    case 1:
        return CalcMotionData_CtrlStacks(&m, flags | 0x80, s_Ids1, 1);
    case 2:
        return CalcMotionData_CtrlStacks(&m, flags | 0x80, s_Ids2, 1);
    case 3:
        result = CalcMotionData_CtrlStacks(pair, flags, s_Ids3, 2);
        ref = 0;
        break;
    case 5:
        flags |= 0x40;
        // fall through
    case 4:
        result = CalcMotionData_CtrlStacks(pair, flags, s_Ids4, 2);
        ref = 1;
        break;
    default:
        m.m_pos.w = 0.0f;
        m.m_pos.z = 0.0f;
        m.m_pos.y = 0.0f;
        m.m_pos.x = 0.0f;
        m.m_rot.z = 0.0f;
        m.m_rot.y = 0.0f;
        m.m_rot.x = 0.0f;
        m.m_rot.w = 1.0f;
        return 8;
    }
    EXVector d;
    d.x = pair[1].m_pos.x - pair[0].m_pos.x;
    d.y = pair[1].m_pos.y - pair[0].m_pos.y;
    d.z = pair[1].m_pos.z - pair[0].m_pos.z;
    d.w = pair[1].m_pos.w - pair[0].m_pos.w;
    m.m_pos.x = d.x;
    m.m_pos.y = d.y;
    m.m_pos.z = d.z;
    m.m_pos.w = d.w;
    const EXQuaternion &q0 = pair[0].m_rot;
    const EXQuaternion &q1 = pair[1].m_rot;
    if (fabsf(q0.x - q1.x) >= 0.001f || fabsf(q0.y - q1.y) >= 0.001f || fabsf(q0.z - q1.z) >= 0.001f ||
        fabsf(q0.w - q1.w) >= 0.001f) {
        if (!(flags & 1)) {
            EXMaths::quat_xyzw r = QuatMul(q1, QuatConj(q0));
            m.m_rot.x = r.x;
            m.m_rot.y = r.y;
            m.m_rot.z = r.z;
            m.m_rot.w = r.w;
        } else {
            EXVector a0, a1;
            q0.GetRotation(a0, 4);
            q1.GetRotation(a1, 4);
            EXVector diff;
            diff.z = AngleDiff(a0.z, a1.z);
            diff.y = AngleDiff(a0.y, a1.y);
            diff.x = AngleDiff(a0.x, a1.x);
            diff.w = 0.0f;
            m.m_rot.EXMaths::quat_xyzw::SetRotation(diff, 4);
        }
    } else {
        m.m_rot.z = 0.0f;
        m.m_rot.y = 0.0f;
        m.m_rot.x = 0.0f;
        m.m_rot.w = 1.0f;
    }
    if ((flags & 4) && (m_renderFlags & 2)) {
        m.m_pos.x = m.m_pos.x * m_scale.x;
        m.m_pos.y = m_scale.y * m.m_pos.y;
        m.m_pos.z = m_scale.z * m.m_pos.z;
        m.m_pos.w = m_scale.w * m.m_pos.w;
    }
    if (flags & 2) {
        // Into the frame of the reference motion.
        const EXQuaternion &q = pair[ref].m_rot;
        if (fabsf(q.x) >= 0.001f || fabsf(q.y) >= 0.001f || fabsf(q.z) >= 0.001f || fabsf(q.w - 1.0f) >= 0.001f) {
            EXMaths::quat_xyzw inv = QuatConj(q);
            EXMaths::mat_44 mtx;
            mtx.set_rotate_quat(inv);
            EXMaths::vec_xyzw v;
            D3DXVec4Transform(&v, &m.m_pos, &mtx);
            m.m_pos.x = v.x;
            m.m_pos.y = v.y;
            m.m_pos.z = v.z;
            m.m_pos.w = v.w;
        }
    }
    return result;
}

// Root motion over the whole of the current animations: last frame minus first frame of the first
// EXItemCtrl_Anim of each active stack, blended by the stack blends. Returns 0 if an animation was
// not available.
s8 EXItemAnimator_Anim::CalcTotalMotion(EXMotionData &m) const
{
    m.m_pos.w = 0.0f;
    m.m_pos.z = 0.0f;
    m.m_pos.y = 0.0f;
    m.m_pos.x = 0.0f;
    m.m_rot.z = 0.0f;
    m.m_rot.y = 0.0f;
    m.m_rot.x = 0.0f;
    m.m_rot.w = 1.0f;
    int n = 0;
    s8 ok = 1;
    EXMotionData start, end, start2, end2;
    for (int s = 0; s < m_numStacks; s++) {
        const EXItemCtrlStack &stack = m_stacks[s];
        if (!(stack.m_blend >= 0.001f) || !(stack.m_sectionBits.m_bits[0] & 1)) continue;
        EXItemCtrl *pCtrl = stack.FirstCtrl();
        if (!pCtrl || !pCtrl->IsKindOf(EXRUNTIME_CLASS(EXItemCtrl_Anim))) continue;
        EXItemCtrl_Anim *pAnimCtrl = (EXItemCtrl_Anim *)pCtrl;
        EXGeoFile *pFile = m_geoRef.GeoFile();
        EXGeoHeader *pHdr = pFile ? pFile->Header() : 0;
        EXAnimMotionMorph &info = pAnimCtrl->m_pMotionInfo[pAnimCtrl->m_curMotion];
        EXGeoBaseAnim *pGeoAnim = info.GeoAnim(info.GeoHeader(pHdr));
        if (!pGeoAnim) {
            ok = 0;
            continue;
        }
        pGeoAnim->DecompressMotion(0, n ? start2 : start, 4, 0x3f00);
        pGeoAnim->DecompressMotion(pGeoAnim->m_NumFrames - 1, n ? end2 : end, 4, 0x3f00);
        if (n) {
            float t = stack.m_blend;
            BlendMotion(start, start2, t);
            BlendMotion(end, end2, t);
        }
        n++;
    }
    if (n > 0) {
        m.m_pos.x = end.m_pos.x - start.m_pos.x;
        m.m_pos.y = end.m_pos.y - start.m_pos.y;
        m.m_pos.z = end.m_pos.z - start.m_pos.z;
        m.m_pos.w = end.m_pos.w - start.m_pos.w;
        EXMaths::quat_xyzw r = QuatMul(end.m_rot, QuatConj(start.m_rot));
        m.m_rot.x = r.x;
        m.m_rot.y = r.y;
        m.m_rot.z = r.z;
        m.m_rot.w = r.w;
    }
    return ok;
}
