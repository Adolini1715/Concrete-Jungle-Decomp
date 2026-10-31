#include "EXRayCastTemp.h"
#include "EXItemAnimator.h"
#include "EXItemAnimator_ForceFeedback.h"
#include "EXItemAnimator_Map.h"
#include "EXItemAnimator_Entity.h"
#include "EXItemAnimator_Anim.h"
#include "EXItemAnimator_Particle.h"
#include "EXItemAnimator_Sound.h"
#include "EXItemAnimator_DynLight.h"
#include "EXItem.h"
#include "EXDatum.h"
#include "EXBaseRenderEnv.h"
#include "EXBaseSoundManager.h"
#include "EXMemory.h"

inline void *__cdecl operator new(unsigned int, void *p) { return p; }

// TODO stand-in: application frame rates (EXApp.obj, app area).
class EXBaseApp {
public:
    static u32 NominalEngineFrameRate() { return m_NominalEngineFrameRate; }
protected:
    static u32 m_NominalEngineFrameRate;
};

// TODO stand-in: script animator (EXItemAnimator_Script.obj, script area), sizeof 0x148.
class EXItemAnimator_Script : public EXItemAnimator {
public:
    static const EXRuntimeClass classEXItemAnimator_Script;
    EXItemAnimator_Script();
    static EXItemAnimator_Script *NewItem() { return new EXItemAnimator_Script; }
    u8 m_104[0x44];
};



// TODO layout: storage unit of the animator allocator (size of the largest pooled animator).
class _EXItemAnimatorAllocator {
public:
    u8 m_data[0x180];
};

// Pool the allocator-created animators come from.
class EXItemAnimatorQueue : public EXAllocator<_EXItemAnimatorAllocator> {
public:
    EXItemAnimatorQueue(u32 n) : EXAllocator<_EXItemAnimatorAllocator>(n) {}
    ~EXItemAnimatorQueue() {}
    static void Release(EXItemAnimator *p);
    static EXItemAnimator *Allocate(u32 hash, EXItemAnimator *p);
};

EX_IMPLEMENT_DYNCREATE(EXItemAnimator, EXPlacementItem)
EX_IMPLEMENT_DYNCREATE(EXItemAnimator_ForceFeedback, EXItemAnimator)

EXItemAnimator::EXItemAnimator()
{
    m_frameRateMult = 1.0f;
    m_objectTime = 0.0f;
    m_allocatorCreated = 0;
}

EXItemAnimator::~EXItemAnimator()
{
    if (m_allocatorCreated) for (;;) {}
}

struct EXAnimatorCreateFrom {
    EXRuntimeClass *m_pClass;
    void *m_pCreate;
};
static EXAnimatorCreateFrom s_CreateFrom[EXItemAnimator::ANIMCF_COUNT] = {
    { (EXRuntimeClass *)&EXItemAnimator_Script::classEXItemAnimator_Script, (void *)EXItemAnimator_Script::NewItem },
    { EXRUNTIME_CLASS(EXItemAnimator_Anim), (void *)EXItemAnimator_Anim::NewItem },
    { EXRUNTIME_CLASS(EXItemAnimator_Entity), (void *)EXItemAnimator_Entity::NewItem },
    { EXRUNTIME_CLASS(EXItemAnimator_Map), (void *)EXItemAnimator_Map::NewItem },
    { EXRUNTIME_CLASS(EXItemAnimator_Particle), (void *)EXItemAnimator_Particle::NewItem },
    { EXRUNTIME_CLASS(EXItemAnimator_Sound), (void *)EXItemAnimator_Sound::NewItem },
    { EXRUNTIME_CLASS(EXItemAnimator_DynLight), (void *)EXItemAnimator_DynLight::NewItem },
    { EXRUNTIME_CLASS(EXItemAnimator_ForceFeedback), (void *)EXItemAnimator_ForceFeedback::NewItem },
};

s8 EXItemAnimator::SetCreateFrom(AnimatorCF cf, EXRuntimeClass *pClass, void *pCreate)
{
    if (cf == ANIMCF_NONE) return 0;
    s_CreateFrom[cf].m_pClass = pClass;
    s_CreateFrom[cf].m_pCreate = pCreate;
    return 1;
}

EXItemAnimator *EXItemAnimator::CreateFrom(EXGeoFile *pFile, u32 hash)
{
    if (!pFile) return 0;
    if (!*(void **)((char *)pFile + 0x20)) return 0;
    AnimatorCF cf = GetAnimatorCF(hash);
    if (cf != ANIMCF_NONE && s_CreateFrom[cf].m_pCreate) {
        EXItemAnimator *p = ((EXItemAnimator *(*)())s_CreateFrom[cf].m_pCreate)();
        if (p) {
            if (p->Initialise(pFile, hash)) return p;
            delete p;
        }
    }
    return 0;
}

EXRuntimeClass *EXItemAnimator::RuntimeClassFrom(u32 hash)
{
    AnimatorCF cf = GetAnimatorCF(hash);
    if (cf == ANIMCF_NONE) return 0;
    return s_CreateFrom[cf].m_pClass;
}

EXItemHandler *EXItemAnimator::GetOwnerHandler() const
{
    EXItemHandler *pHandler = 0;
    const EXPlacementItem *p = this;
again:
    p = (const EXPlacementItem *)p->m_pOwner;
    if (p) {
        if (!p->IsKindOf(EXRUNTIME_CLASS(EXItem))) goto again;
        pHandler = ((const EXItem *)p)->ItemHandler();
        if (!pHandler) goto again;
    }
    return pHandler;
}

EXItem *EXItemAnimator::GetOwnerItem() const
{
    const EXPlacementItem *p = this;
    do {
        p = (const EXPlacementItem *)p->m_pOwner;
    } while (p && !p->IsKindOf(EXRUNTIME_CLASS(EXItem)));
    return (EXItem *)p;
}

EXPlacementItem *EXItemAnimator::GetPositionItem() const
{
    const EXPlacementItem *p = this;
    while (p && !p->ValidMtx()) p = (const EXPlacementItem *)p->m_pOwner;
    return (EXPlacementItem *)p;
}

void EXItemAnimator::AddAnimator(EXItemAnimator *p)
{
    if (p == this) for (;;) {}
    p->SetOwner(this);
    p->Invalidate();
    m_nodes.AddTail(&p->m_node);
}

void EXItemAnimator::RemoveAnimator(EXItemAnimator *p)
{
    if (p) m_nodes.Remove(&p->m_node);
}

void EXItemAnimator::DeleteAnimator(EXItemAnimator *p)
{
    m_nodes.Remove(p ? &p->m_node : 0);
    delete p;
}

EXItemAnimator *EXItemAnimator::GetAnimator(const EXRuntimeClass *pClass) const
{
    if (!pClass) return GetAnimator();
    for (EXItemAnimator *p = GetAnimator(); p; p = p->NextAnimator())
        if (p->IsKindOf(pClass)) return p;
    return 0;
}

EXItemAnimator *EXItemAnimator::GetAnimator(u32 hash) const
{
    for (EXItemAnimator *p = GetAnimator(); p; p = p->NextAnimator())
        if (p->m_geoRef.m_hashRef == hash) return p;
    return 0;
}

void *EXItemAnimator::GetDatum_ChildLink(u32 type, EXDatumTemp &d, u32 flags, void *&pCtx) const
{
    for (EXItemAnimator *p = GetAnimator(); p; p = p->NextAnimator()) {
        void *r = p->GetDatum(type, d, flags, pCtx);
        if (r) return r;
    }
    return 0;
}

void EXItemAnimator::GetDatum_ChildLink(u32 type, EXDatumTempArray &arr, u32 flags) const
{
    for (EXItemAnimator *p = GetAnimator(); p; p = p->NextAnimator()) p->GetDatum(type, arr, flags);
}

void *EXItemAnimator::GetNearestDatum_ChildLink(const EXVector &pos, EXDatumTemp &d, u32 flags) const
{
    for (EXItemAnimator *p = GetAnimator(); p; p = p->NextAnimator()) {
        void *r = p->GetNearestDatum(pos, d, flags);
        if (r) return r;
    }
    return 0;
}

s8 EXItemAnimator::DoGrabPoint_ChildLink(const EXMatrix &mtx, EXGrabPointTemp &t, u32 flags) const
{
    s8 r = 0;
    for (EXItemAnimator *p = GetAnimator(); p; p = p->NextAnimator()) r |= p->DoGrabPoint(mtx, t, flags);
    return r;
}

void EXItemAnimator::DoRayCast_ChildLink(const EXMatrix &mtx, EXRayCastTemp &t, u32 flags) const
{
    EXItemAnimator *p = GetAnimator();
    s8 bStop = t.m_bAnyHit;
    for (; p; p = p->NextAnimator()) {
        p->DoRayCast(mtx, t, flags);
        if (bStop && t.m_bHit) break;
    }
}

void EXItemAnimator::DoShadowCast_ChildLink(EXShadowTemp &t) const
{
    for (EXItemAnimator *p = GetAnimator(); p; p = p->NextAnimator()) p->DoShadowCast(t);
}

void EXItemAnimator::DoEntityScoop_ChildLink(const EXMatrix &mtx, EXCollisionScoopTemp &t, u32 flags) const
{
    for (EXItemAnimator *p = GetAnimator(); p; p = p->NextAnimator()) p->DoEntityScoop(mtx, t, flags);
}

s8 EXItemAnimator::GetBoundsBox(EXBoundsBox &box, u32 flags)
{
    s8 r = _GetBoundsBox(box, flags);
    for (EXItemAnimator *p = GetAnimator(); p; p = p->NextAnimator()) r |= p->GetBoundsBox(box, 1);
    return r;
}

void EXItemAnimator::UpdateBoundsBox(s8 bRecurse)
{
    _GetBoundsBox(m_boundsBox, 2);
    for (EXItemAnimator *p = GetAnimator(); p; p = p->NextAnimator()) {
        if (bRecurse) UpdateBoundsBox(bRecurse);
        EXMatrix m;
        p->CalcMatrix(m);
        EXBoundsBox box = p->m_boundsBox;
        box._ApplyXForm(m);
        m_boundsBox.Accumulate(box);
    }
    m_validBounds = 1;
}

void EXItemAnimator::ApplyMatrixBoundsBox(EXBoundsBox &box, EXBoundsBox &out, u32 flags) const
{
    if (!(flags & 2)) {
        EXMatrix m;
        CalcMatrix(m);
        box._ApplyXForm(m);
    }
    if (flags & 1) out.Accumulate(box);
    else out.Set(box);
}

void EXItemAnimator::SetFrameRate60(float f) { m_frameRateMult = f / (float)EXBaseApp::NominalEngineFrameRate(); }
float EXItemAnimator::FrameRate60() const { return (float)EXBaseApp::NominalEngineFrameRate() * m_frameRateMult; }
void EXItemAnimator::SetObjectTime(float t) { m_objectTime = (float)EXBaseApp::NominalEngineFrameRate() * t; }
float EXItemAnimator::ObjectTime() const { return m_objectTime / (float)EXBaseApp::NominalEngineFrameRate(); }
void EXItemAnimator::UpdateObjectTime(float t) { SetObjectTime(t); }
void EXItemAnimator::BuildEntityInfoArray(const EXMatrix &, EXGeoEntity_InfoArray &, s8) {}

void EXItemAnimator::DoUpdate() { UpdateNodes(); }

u32 EXItemAnimator::DoRender(const EXMatrix &mtx, const EXRenderInfo &info, u32 flags)
{
    if (EXBaseRenderEnv::RenderState() & 1) CalcMatrix_InPlace(mtx, flags);
    return RenderNodes(m_mtx, info, flags);
}

void EXItemAnimator::ApplyFunction(u32 (__cdecl *pFunc)(EXItemAnimator *, void *), void *pData)
{
    pFunc(this, pData);
    for (EXItemAnimator *p = GetAnimator(); p; p = p->NextAnimator()) p->ApplyFunction(pFunc, pData);
}

void *EXItemAnimator::GetDatum(u32 type, EXDatumTemp &d, u32 flags, void *&pCtx)
{
    if (flags & 0x80) return GetDatum_ChildLink(type, d, flags, pCtx);
    return 0;
}

void *EXItemAnimator::GetNearestDatum(const EXVector &pos, EXDatumTemp &d, u32 flags)
{
    if (flags & 0x80) GetNearestDatum_ChildLink(pos, d, flags);
    return 0;
}

EXGeoCommonObject *EXItemAnimator::GetGeoObject() const { return m_geoRef.GeoObject(); }

s8 EXItemAnimator::_GetBoundsBox(EXBoundsBox &box, u32 flags)
{
    if (!(flags & 1)) box.SetZero();
    return 0;
}

void EXItemAnimator::SetRenderState(u32 state, s8 bOn)
{
    if (state == m_geoRef.m_hashRef) {
        if (bOn) SetHasRender();
        else ClrHasRender();
    }
}

void EXItemAnimator::DoItemUpdate()
{
    if (m_itemFlags.m_flags & 2) {
        DoUpdate();
        m_itemFlags.m_flags &= ~8;
        UpdateBoundsBox(0);
    }
}

u32 EXItemAnimator::DoItemRender(const EXMatrix &mtx, const EXRenderInfo &info)
{
    if (m_itemFlags.BitTest(1)) {
        u32 r = DoRender(mtx, info, 0);
        if (m_itemFlags.BitTest(8)) m_itemFlags.BitClr(8);
        return r;
    }
    return 0;
}

void EXItemAnimator::GetDatum(u32 type, EXDatumTempArray &arr, u32 flags)
{
    EXDatumTemp d;
    if (GetDatum(type, d, flags, m_pNullDatumPtr)) arr.Add(d, 1);
}

static const float s_FrameRateInit = 60.0f;
static float s_FrameRate = s_FrameRateInit;
EXItemAnimatorQueue AnimatorAllocatorQueue(16);

void EXItemAnimatorQueue::Release(EXItemAnimator *p)
{
    if (p->AllocatorCreated()) {
        p->EnableAllocatorCreated(0);
        p->~EXItemAnimator();
        AnimatorAllocatorQueue.Remove((_EXItemAnimatorAllocator *)p);
    } else {
        delete p;
    }
}

EXItemAnimator *EXItemAnimatorQueue::Allocate(u32 hash, EXItemAnimator *p)
{
    if (!p) p = (EXItemAnimator *)AnimatorAllocatorQueue.Get();
    switch ((hash >> 24) & 0x7f) {
    case 4: new (p) EXItemAnimator_Script; break;
    case 3: new (p) EXItemAnimator_Anim; break;
    case 2: new (p) EXItemAnimator_Entity; break;
    case 0x11: new (p) EXItemAnimator_Particle; break;
    case 0x1a:
    case 0x1b: new (p) EXItemAnimator_Sound; break;
    case 0x13: new (p) EXItemAnimator_DynLight; break;
    case 0x24: new (p) EXItemAnimator_ForceFeedback; break;
    default: new (p) EXItemAnimator; break;
    }
    p->EnableAllocatorCreated(1);
    return p;
}

void *EXItemAnimator::m_pNullDatumPtr;
