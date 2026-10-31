#include "EXCollisionScoopTemp.h"
#include "EXRayCastTemp.h"
#include "EXItem.h"
#include "EXItemAnimator.h"
#include "EXItemHandler.h"
#include "EXItemPhysics.h"
#include "EXDatum.h"
#include "EXQuaternion.h"
#include "EXBaseRenderEnv.h"
#include "EXStdFuncs.h"
#include "EXMemory.h"

// TODO stand-in: item environment (EXItemEnv.obj): group lists.
class EXItemEnv {
public:
    EXODList &ItemGroup(int i) { return m_pGroups[i]; }
    u8 NumItemGroupLists() const { return m_numGroups; }
    static EXItemEnv *TheItemEnv() { return m_pTheItemEnv; }
    u8 m_00[0x94];
    EXODList *m_pGroups;    // +0x94
    u8 m_98[2];
    u8 m_numGroups;         // +0x9a
protected:
    static EXItemEnv *m_pTheItemEnv;
};

// TODO stand-in: extended render parameters (render area), sizeof 0x80.
class EXExtRenderInfo {
public:
    EXExtRenderInfo()
    {
        memset(m_00, 0, sizeof(m_00));
        m_60.Set(0.5f, 0.5f, 0.5f, 1.0f);
        m_70 = 0.5f;
        m_74 = 0.08f;
        m_78 = 0;
        m_7c = 0.0f;
    }
    u32 m_00[0x18];
    EXVector m_60;
    float m_70, m_74;
    u32 m_78;
    float m_7c;
};

// TODO stand-in: texture swap table (render area), sizeof 0x14.
class EXTexSwapInfo {
public:
    EXTexSwapInfo();
    ~EXTexSwapInfo();
    u8 m_00[0x14];
};

// TODO stand-in: EXBaseDisplay::ObjectToVap() (display area, header owned elsewhere).
extern EXMaths::mat_44 g_ObjectToVapStandIn;

EX_IMPLEMENT_DYNCREATE_ROOT(EXPlacementItem)
EX_IMPLEMENT_DYNCREATE(EXItem, EXPlacementItem)

EXBaseItem::~EXBaseItem()
{
    EXBaseItem *p = FirstItem();
    while (p) {
        EXBaseItem *pNext = p->NextItem();
        delete p;
        p = pNext;
    }
    if (m_pOwner) m_node.Remove();
}

EXBaseItem *EXBaseItem::GetBaseOwner() const
{
    const EXBaseItem *p = this;
    while (p->m_pOwner) p = p->m_pOwner;
    return (EXBaseItem *)p;
}

void EXPlacementItem::Invalidate(u32 flags)
{
    if (flags & 0x80) m_validMtx = 0;
}

void EXPlacementItem::CalcMatrix_Scale(EXMaths::mat_44 &m) const { ((EXMatrix &)m).Scale(m_scale); }

void EXItem::UpdateGroups(EXItemEnv *pEnv, u32 flags)
{
    u32 changed = m_f8 ^ flags;
    int i = -1;
    while (changed) {
        u32 bit;
        do {
            bit = changed & 1;
            changed >>= 1;
            i++;
        } while (!bit);
        if (flags & (1 << i)) pEnv->ItemGroup(i).AddHead(&ItemGroup(i), this);
        else pEnv->ItemGroup(i).Remove(&ItemGroup(i));
    }
    m_f8 = flags;
}

void EXItem::UpdateGroupsTail(EXItemEnv *pEnv, u32 flags)
{
    u32 changed = m_f8 ^ flags;
    int i = -1;
    while (changed) {
        u32 bit;
        do {
            bit = changed & 1;
            changed >>= 1;
            i++;
        } while (!bit);
        if (flags & (1 << i)) pEnv->ItemGroup(i).AddTail(&ItemGroup(i), this);
        else pEnv->ItemGroup(i).Remove(&ItemGroup(i));
    }
    m_f8 = flags;
}

void EXItem::EnableParticleSystems(s8 b) const
{
    for (EXItemAnimator *p = GetAnimator(); p; p = p->NextAnimator()) p->EnableParticleSystems(b);
}

void EXItem::SetItemHandler(EXItemHandler *p)
{
    m_pItemHandler = p;
    if (p) p->SetItem(this);
}

void EXItem::SetItemPhysics(EXItemPhysics *p)
{
    m_pItemPhysics = p;
    if (p) p->SetItem(this);
}

void EXItem::SetTrigger(EXBaseTrigger *p) { m_pTrigger = p; }

void EXItem::AddAnimator(EXItemAnimator *p)
{
    if (p) {
        p->SetOwner(this);
        p->Invalidate();
        m_animators.AddTail(&p->m_node);
    }
}

void EXItem::RemoveAnimator(EXItemAnimator *p)
{
    if (p) m_animators.Remove(&p->m_node);
}

void EXItem::DeleteAnimator(EXItemAnimator *p)
{
    if (p) {
        m_animators.Remove(&p->m_node);
        delete p;
    }
}

EXItemAnimator *EXItem::GetAnimator(const EXRuntimeClass *pClass) const
{
    if (!pClass) return GetAnimator();
    EXItemAnimator *p;
    for (p = GetAnimator(); p; p = p->NextAnimator())
        if (p->IsKindOf(pClass)) break;
    return p;
}

EXItemAnimator *EXItem::GetAnimator(u32 hash) const
{
    EXItemAnimator *p;
    for (p = GetAnimator(); p; p = p->NextAnimator())
        if (p->m_geoRef.m_hashRef == hash) break;
    return p;
}

void EXItem::DoItemPhysics()
{
    EXItemPhysics *pPhysics = m_pItemPhysics;
    if (pPhysics && !(pPhysics->m_physicsFlags & 0x20)) pPhysics->DoUpdate();
}

void EXItem::DoItemShadowCast(EXMatrix &, EXShadowTemp &t)
{
    for (EXItemAnimator *p = GetAnimator(); p; p = p->NextAnimator()) p->DoShadowCast(t);
}

void EXItem::SetAnimatorRenderState(u32 state, s8 bOn) const
{
    for (EXItemAnimator *p = GetAnimator(); p; p = p->NextAnimator()) p->SetRenderState(state, bOn);
}

void EXItem::SetAnimatorLodRange(int lo, int hi) const
{
    for (EXItemAnimator *p = GetAnimator(); p; p = p->NextAnimator()) {
        p->SetLodMin(lo);
        p->SetLodMax(hi);
    }
}

void EXItem::SetAnimatorMinLod(int lod) const
{
    for (EXItemAnimator *p = GetAnimator(); p; p = p->NextAnimator()) p->SetLodMin(lod);
}

void EXItem::SetAnimatorMaxLod(int lod) const
{
    for (EXItemAnimator *p = GetAnimator(); p; p = p->NextAnimator()) p->SetLodMax(lod);
}

void EXItem::AnimatorApplyFunction(u32 (__cdecl *pFunc)(EXItemAnimator *, void *), void *pData)
{
    for (EXItemAnimator *p = GetAnimator(); p; p = p->NextAnimator()) p->ApplyFunction(pFunc, pData);
}

EXPlacementItem::EXPlacementItem()
{
    m_position.SetZero();
    m_rotation.set_zero();
    m_scale.set(1.0f, 1.0f, 1.0f, 1.0f);
    m_rotOrder = 4;
    m_geoRef.Set(0, 0);
    m_boundsBox.SetZero();
    m_mtx.SetIdentity();
    m_pRemoteMatrix = 0;
    m_ownFlags = 0;
    m_validMtx = 0;
    m_7f_0 = 0;
    m_validBounds = 0;
    m_hasMtx = 0;
    m_itemFlags = 0xb;
    m_renderFlags.Set(0x10);
}

EXPlacementItem::~EXPlacementItem()
{
    if (m_ownFlags.BitTest(1)) {
        if (m_renderInfo.ExtRenderInfo()) delete m_renderInfo.ExtRenderInfo();
    }
    if (m_ownFlags.BitTest(2)) delete m_renderInfo.TexSwapInfo();
}

void EXPlacementItem::_UpdateNodes(EXPlacementItem *p)
{
    while (p) {
        EXPlacementItem *pNext = (EXPlacementItem *)p->NextItem();
        u32 f = p->m_itemFlags.m_flags;
        if (f & 2) {
            p->DoUpdate();
            f = p->m_itemFlags.m_flags;
            if (f & 8) p->m_itemFlags.BitClr(8);
        }
        if ((f & 0x90) == 0x10) {
            m_nodes.Remove(&p->m_node);
            delete p;
        }
        p = pNext;
    }
}

u32 EXPlacementItem::_RenderNodes(const EXMatrix &mtx, const EXRenderInfo &info, u32 flags, EXPlacementItem *p)
{
    u32 r = 0;
    while (p) {
        if (p->m_itemFlags.BitTest(1)) r |= p->DoRender(mtx, info, flags);
        p = (EXPlacementItem *)p->NextItem();
    }
    return r;
}

EXExtRenderInfo *EXPlacementItem::GetExtRenderInfo()
{
    EXExtRenderInfo *p = m_renderInfo.m_pExtRenderInfo;
    if (!p) {
        p = new EXExtRenderInfo;
        m_renderInfo.m_pExtRenderInfo = p;
        m_renderInfo.RenderFlagsRef().BitState(0x40, p);
        m_ownFlags.m_flags |= 1;
    }
    return p;
}

EXTexSwapInfo *EXPlacementItem::SetTexSwapInfo()
{
    EXTexSwapInfo *p = m_renderInfo.m_pTexSwapInfo;
    if (!p) {
        p = new EXTexSwapInfo;
        m_renderInfo.m_pTexSwapInfo = p;
        m_renderInfo.RenderFlagsRef().BitState(0x80, p);
        m_ownFlags.m_flags |= 2;
    }
    return p;
}

void EXPlacementItem::CalcMatrix_Vap(const EXMatrix &parent, EXMatrix &out, u32 flags) const
{
    u32 bRot = m_renderFlags.m_flags & 1;
    *(EXMaths::mat_44 *)&out = g_ObjectToVapStandIn;
    EXMatrix rot;
    if (bRot) rot.SetRotation(m_rotation, m_rotOrder);
    if ((m_renderFlags.m_flags | flags) & 2) {
        float sx, sy, sz;
        parent.GetScale(sx, sy, sz);
        EXMatrix s;
        EXVector v(sx, sy, sz, 1.0f);
        s.SetScale(v);
        out *= s;
    }
    if (bRot) out *= rot;
    out.set_row3(parent.r[3]);
}

void EXPlacementItem::CalcMatrix(EXMatrix &m) const
{
    u32 f = m_renderFlags.m_flags;
    if (!(f & 1)) {
        m.SetIdentity();
        return;
    }
    m.SetRotation(m_rotation, m_rotOrder);
    if (f & 2) {
        if (!((float)fabs(m_scale.x - 1.0f) < EX_EPSILON_EQUAL && (float)fabs(m_scale.y - 1.0f) < EX_EPSILON_EQUAL &&
              (float)fabs(m_scale.z - 1.0f) < EX_EPSILON_EQUAL && (float)fabs(m_scale.w - 1.0f) < EX_EPSILON_EQUAL))
            m.Scale(m_scale);
    }
    EXMaths::vec_xyzw t = m_position;
    t.set_w1();
    m.set_row3(t);
    if (f & 0xc) m = *m_pRemoteMatrix * m;
}

void EXPlacementItem::CalcMatrix_Remote(EXMaths::mat_44 &m, const EXMatrix &parent) const
{
    m = *m_pRemoteMatrix * EXMatrix(m);
    if (m_renderFlags.m_flags & 8) m = parent * EXMatrix(m);
}

EXItem::EXItem()
{
    m_animators.Init();
    m_pItemHandler = 0;
    m_pItemPhysics = 0;
    m_pTrigger = 0;
    m_pXItemOwner = 0;
    m_itemFlags = 0xb;
    m_renderFlags.BitSet(0x13);
    m_mapOn.m_v = 0xffff;
    m_clipType = 0;
    m_clipFlags = 0;
    m_f8 = 0;
    for (int i = 0; i < 5; i++) ItemGroup(i).Init();
    u32 n = EXItemEnv::TheItemEnv()->NumItemGroupLists() - 5;
    m_pExtraGroups = 0;
    if (n) {
        m_pExtraGroups = new EXODListItem[n];
        for (u32 j = 0; j < n; j++) m_pExtraGroups[j].Init();
    }
}

EXItem::~EXItem()
{
    EXItemAnimator *p = GetAnimator();
    while (p) {
        EXItemAnimator *pNext = p->NextAnimator();
        delete p;
        p = pNext;
    }
    m_animators.Init();
    if (m_pItemPhysics) {
        delete m_pItemPhysics;
        m_pItemPhysics = 0;
    }
    if (m_pItemHandler) {
        delete m_pItemHandler;
        m_pItemHandler = 0;
    }
    if (m_pExtraGroups) {
        delete[] m_pExtraGroups;
        m_pExtraGroups = 0;
    }
}

void EXItem::DoItemUpdate_Animators()
{
    EXItemAnimator *p = GetAnimator();
    while (p) {
        u32 f = p->m_itemFlags.m_flags;
        if (f & 2) {
            p->DoUpdate();
            f = p->m_itemFlags.m_flags;
            if (f & 8) p->m_itemFlags.BitClr(8);
        }
        EXItemAnimator *pNext = p->NextAnimator();
        if ((f & 0x90) == 0x10) {
            m_animators.Remove(&p->m_node);
            delete p;
        } else {
            p->UpdateBoundsBox(0);
        }
        p = pNext;
    }
}

void EXItem::DoItemUpdate()
{
    if (m_itemFlags.BitTest(2)) {
        if (ItemHandler()) ItemHandler()->DoUpdate();
        DoItemUpdate_Animators();
        if (m_itemFlags.BitTest(4) && ItemHandler()) ItemHandler()->DoPostUpdate();
    }
    m_itemFlags.BitClr(8);
}

void EXItem::DoItemPreRender()
{
    static EXRenderInfo s_info;
    EXBaseRenderEnv::SetRenderState(1);
    EXMatrix m = EXMatrix::MakeIdentity();
    DoItemRender(m, s_info);
    m_renderFlags.m_flags |= 0x20;
}

void *EXItem::GetAnimatorDatum(u32 type, EXDatumTemp &d, u32 flags, void *pCtx) const
{
    for (EXItemAnimator *p = GetAnimator(); p; p = p->NextAnimator()) {
        void *r = p->GetDatum(type, d, flags, pCtx);
        if (r) {
            if (!(flags & 4)) {
                EXMatrix m;
                CalcMatrix(m);
                d.ApplyXForm(m);
            }
            return r;
        }
    }
    return 0;
}

// Hamilton product a * b (the quat_xyzw multiply inline, not declared in EXMaths.h yet).
static inline EXMaths::quat_xyzw QuatMul(const EXMaths::quat_xyzw &a, const EXMaths::quat_xyzw &b)
{
    EXMaths::quat_xyzw r;
    r.x = a.w * b.x + b.w * a.x + b.z * a.y - b.y * a.z;
    r.y = b.w * a.y + b.y * a.w + a.z * b.x - a.x * b.z;
    r.z = a.w * b.z + a.z * b.w + b.y * a.x - b.x * a.y;
    r.w = b.w * a.w - b.x * a.x - b.y * a.y - a.z * b.z;
    return r;
}

void EXItem::GetAnimatorDatum(u32 type, EXDatumTempArray &arr, u32 flags) const
{
    int start = arr.m_nSize;
    for (EXItemAnimator *p = GetAnimator(); p; p = p->NextAnimator()) p->GetDatum(type, arr, flags);
    if (flags & 4) return;
    int n = arr.m_nSize - start;
    if (!n) return;
    EXMatrix m;
    CalcMatrix(m);
    EXQuaternion q;
    q.SetRotation(m);
    for (int i = 0; i < n; i++) {
        EXDatumTemp &dt = arr.m_pData[start + i];
        dt.m_position.ApplyXForm(m);
        dt.m_rotation = QuatMul(q, dt.m_rotation);
    }
}

s8 EXItem::GetAnimatorBoundsBox(EXBoundsBox &box, u32 flags) const
{
    EXItemAnimator *p = GetAnimator();
    if (!p) {
        box.SetZero();
        return 0;
    }
    box.m_vctMin.set(1e6f, 1e6f, 1e6f, 0.0f);
    box.SetMax(EXVector(-1e6f, -1e6f, -1e6f, 0.0f));
    for (; p; p = p->NextAnimator()) {
        EXMatrix m;
        p->CalcMatrix(m);
        EXBoundsBox b = p->m_boundsBox;
        b._ApplyXForm(m);
        box.Accumulate(b);
    }
    if (!(flags & 2)) {
        EXMatrix m;
        CalcMatrix(m);
        box._ApplyXForm(m);
    }
    return 1;
}

const EXMatrix &EXPlacementItem::CalcMatrix_InPlace(const EXMatrix &parent, u32 &flags)
{
    u32 rf = m_renderFlags.m_flags;
    u32 changed = 0;
    if ((m_validMtx == 0) | (flags & 0x80) | (rf & 0xc)) {
        if (!(rf & 1)) {
            m_mtx = parent;
        } else {
            EXMatrix &m = m_mtx;
            m.SetRotation(m_rotation, m_rotOrder);
            if (rf & 2) {
                if (!((float)fabs(m_scale.x - 1.0f) < EX_EPSILON_EQUAL && (float)fabs(m_scale.y - 1.0f) < EX_EPSILON_EQUAL &&
                      (float)fabs(m_scale.z - 1.0f) < EX_EPSILON_EQUAL && (float)fabs(m_scale.w - 1.0f) < EX_EPSILON_EQUAL))
                    CalcMatrix_Scale(m);
            }
            EXMaths::vec_xyzw t = m_position;
            t.set_w1();
            m.set_row3(t);
            if (rf & 0xc) CalcMatrix_Remote(m, parent);
            else m = parent * EXMatrix(m);
        }
        changed = 0x80;
        m_validMtx = 1;
        m_hasMtx = 1;
    }
    flags |= (rf & 0xf) | changed;
    return m_mtx;
}

u32 EXItem::DoItemRender(const EXMatrix &mtx, const EXRenderInfo &info)
{
    u32 r = 0;
    u32 flags = 0;
    if (EXBaseRenderEnv::RenderState() & 1) CalcMatrix_InPlace(mtx, flags);
    const EXMatrix &m = m_mtx;
    if (m_itemFlags.BitTest(1)) {
        EXRenderInfo ri(m_renderInfo, info);
        for (EXItemAnimator *p = GetAnimator(); p; p = p->NextAnimator())
            if (p->m_itemFlags.BitTest(1)) r |= p->DoRender(m, ri, flags);
    }
    return r;
}

void EXItem::DoItemCollisionScoop(const EXMatrix &mtx, const EXItem *pItem, EXCollisionScoopTemp &t)
{
    if (!pItem->m_pItemPhysics) return;
    if ((t.m_layer.m_flags & 0x200) && m_pItemPhysics && (m_pItemPhysics->m_physicsFlags & 0x10)) return;
    if (m_pItemPhysics && (m_pItemPhysics->m_physicsFlags & 8)) return;
    u32 flags = 0;
    const EXMatrix &m = CalcMatrix_InPlace(mtx, flags);
    for (EXItemAnimator *p = GetAnimator(); p; p = p->NextAnimator()) {
        if (p->m_validBounds) {
            EXBoundsBox box = p->m_boundsBox;
            box._ApplyXForm(p->m_mtx);
            if (!box.Intersect(t.m_centre, t.m_radius)) {
                p->m_validMtx = 0;
                continue;
            }
        }
        p->DoEntityScoop(m, t, flags);
    }
}

void EXItem::DoItemCollisionScoop(const EXMatrix &mtx, EXCollisionScoopTemp &t)
{
    EXItemAnimator *p = GetAnimator();
    if (!p) return;
    u32 flags = 0;
    const EXMatrix &m = CalcMatrix_InPlace(mtx, flags);
    for (; p; p = p->NextAnimator()) p->DoEntityScoop(m, t, flags);
}

void EXItem::DoItemGrabPoint(const EXMatrix &mtx, EXGrabPointTemp &t)
{
    EXItemAnimator *p = GetAnimator();
    if (!p) return;
    u32 flags = 0;
    const EXMatrix &m = CalcMatrix_InPlace(mtx, flags);
    for (; p; p = p->NextAnimator()) p->DoGrabPoint(m, t, flags);
}

void EXItem::DoItemGrabPoint(const EXMatrix &mtx, EXGrabPointArrayTemp &t)
{
    EXItemAnimator *p = GetAnimator();
    if (!p) return;
    u32 flags = 0;
    const EXMatrix &m = CalcMatrix_InPlace(mtx, flags);
    for (; p; p = p->NextAnimator()) p->DoGrabPoint(m, t, flags);
}

void EXItem::DoItemRayCast(const EXMatrix &mtx, EXRayCastTemp &t)
{
    EXItemAnimator *p = GetAnimator();
    if (!p) return;
    u32 flags = 0;
    const EXMatrix &m = CalcMatrix_InPlace(mtx, flags);
    s8 bStop = t.m_bAnyHit;
    for (; p; p = p->NextAnimator()) {
        if (p->m_validBounds) {
            EXBoundsBox box = p->m_boundsBox;
            box._ApplyXForm(m);
            if (!box.Intersect(t.m_start, t.m_dir)) {
                p->m_validMtx = 0;
                continue;
            }
        }
        p->DoRayCast(m, t, flags);
        if (bStop && t.m_bHit) break;
    }
}

u32 EXPlacementItem::DoRender(const EXMatrix &mtx, const EXRenderInfo &, u32 flags)
{
    if (m_itemFlags.BitTest(1) && (EXBaseRenderEnv::RenderState() & 1)) CalcMatrix_InPlace(mtx, flags);
    return 0;
}

const EXMatrix &EXPlacementItem::CalcMatrix_InPlaceParent()
{
    EXFixedArray<EXPlacementItem *, 32> stack;
    int n = -1;
    EXPlacementItem *p = this;
    do {
        stack[++n] = p;
        p = (EXPlacementItem *)p->m_pOwner;
    } while (p);
    u32 flags = 0;
    EXMatrix m = EXMatrix::MakeIdentity();
    for (; n >= 0; n--) {
        EXPlacementItem *q = stack[n];
        s8 bValid = q->m_validMtx;
        m = q->CalcMatrix_InPlace(m, flags);
        if (!bValid) q->m_validMtx = 0;
    }
    return m;
}
