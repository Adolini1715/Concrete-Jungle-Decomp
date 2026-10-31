#ifndef EXITEMANIMATOR_H
#define EXITEMANIMATOR_H
#include "EXPlacementItem.h"

class EXItem;
class EXItemHandler;
struct EXDatumTemp;
class EXDatumTempArray;
struct EXGrabPointTemp;
struct EXGrabPointArrayTemp;
struct EXRayCastTemp;
struct EXShadowTemp;
class EXCollisionScoopTemp;
class EXGeoEntity_InfoArray;
class EXGeoCommonObject;

// Base of all item animators (entity, anim, camera, sound, particle ...). sizeof 0x104.
// Animators hang off an EXItem (or another animator) through EXBaseItem's node list.
class EXItemAnimator : public EXPlacementItem {
    EX_DECLARE_DYNAMIC(EXItemAnimator)
public:
    // Creation classes, indexed by the hash type (hash >> 24 & 0x7f); see GetAnimatorCF.
    enum AnimatorCF {
        ANIMCF_NONE = -1,
        ANIMCF_SCRIPT = 0,      // type 4
        ANIMCF_ANIM = 1,        // type 3
        ANIMCF_ENTITY = 2,      // type 2
        ANIMCF_MAP = 3,         // type 5
        ANIMCF_PARTICLE = 4,    // type 0x11
        ANIMCF_SOUND = 5,       // types 0x1a, 0x1b
        ANIMCF_DYNLIGHT = 6,    // type 0x13
        ANIMCF_FORCEFEEDBACK = 7, // type 0x24
        ANIMCF_COUNT = 8
    };
    virtual ~EXItemAnimator();
    EX_DECLARE_ITEMTYPE(EXItemAnimator)
    virtual void DoUpdate();
    virtual u32 DoRender(const EXMatrix &mtx, const EXRenderInfo &info, u32 flags);
    virtual s8 Initialise(EXGeoFile *, u32) { return 1; }
    virtual float ObjectTime() const;
    virtual void SetObjectTime(float t);
    virtual void UpdateObjectTime(float dt);
    virtual void EnableParticleSystems(s8) const {}
    // MSVC puts overloaded virtuals in reverse declaration order: slot 11 is the array form.
    virtual void *GetDatum(u32 type, EXDatumTemp &d, u32 flags, void *&pCtx);
    virtual void GetDatum(u32 type, EXDatumTempArray &arr, u32 flags);
    virtual void *GetNearestDatum(const EXVector &p, EXDatumTemp &d, u32 flags);
    virtual s8 DoGrabPoint(const EXMatrix &, EXGrabPointTemp &, u32) { return 0; }
    virtual s8 DoGrabPoint(const EXMatrix &, EXGrabPointArrayTemp &, u32) { return 0; }
    virtual void DoRayCast(const EXMatrix &, EXRayCastTemp &, u32) {}
    virtual void DoShadowCast(EXShadowTemp &) {}
    virtual void DoEntityScoop(const EXMatrix &, EXCollisionScoopTemp &, u32) {}
    virtual void SetRenderState(u32 state, s8 bOn);
    virtual void SetLodMin(u32) {}
    virtual void SetLodMax(u32) {}
    virtual void ApplyFunction(u32 (__cdecl *pFunc)(EXItemAnimator *, void *), void *pData);
    virtual void BuildEntityInfoArray(const EXMatrix &mtx, EXGeoEntity_InfoArray &arr, s8 b);
    virtual EXGeoCommonObject *GetGeoObject() const;
protected:
    virtual s8 _GetBoundsBox(EXBoundsBox &box, u32 flags);
public:
    EXItemAnimator();
    static EXItemAnimator *CreateObject();
    void SetFrameRateMult(float f) { m_frameRateMult = f; }
    float FrameRateMult() const { return m_frameRateMult; }
    void EnableAllocatorCreated(s8 b) { m_allocatorCreated = b; }
    s8 AllocatorCreated() const { return m_allocatorCreated; }
    EXItemAnimator *GetAnimator() const
    {
        EXDListItem *p = m_nodes.Head();
        return p ? (EXItemAnimator *)((char *)p - 4) : 0;
    }
    // Next sibling in the owner's node list (not in the map: always inlined).
    EXItemAnimator *NextAnimator() const
    {
        EXDListItem *p = m_node.m_pNext;
        return p ? (EXItemAnimator *)((char *)p - 4) : 0;
    }
    // Case bodies in return-value order (jump-table layout); always inlined, copy still emitted.
    static __forceinline AnimatorCF GetAnimatorCF(u32 hash)
    {
        AnimatorCF cf = ANIMCF_NONE;
        switch ((hash >> 24) & 0x7f) {
        case 4: cf = ANIMCF_SCRIPT; break;
        case 3: cf = ANIMCF_ANIM; break;
        case 2: cf = ANIMCF_ENTITY; break;
        case 5: cf = ANIMCF_MAP; break;
        case 0x11: cf = ANIMCF_PARTICLE; break;
        case 0x1a:
        case 0x1b: cf = ANIMCF_SOUND; break;
        case 0x13: cf = ANIMCF_DYNLIGHT; break;
        case 0x24: cf = ANIMCF_FORCEFEEDBACK; break;
        }
        return cf;
    }
    static s8 SetCreateFrom(AnimatorCF cf, EXRuntimeClass *pClass, void *pData);
    static EXItemAnimator *CreateFrom(EXGeoFile *pFile, u32 hash);
    static EXRuntimeClass *RuntimeClassFrom(u32 hash);
    EXItemHandler *GetOwnerHandler() const;
    EXItem *GetOwnerItem() const;
    EXPlacementItem *GetPositionItem() const;
    void AddAnimator(EXItemAnimator *p);
    void RemoveAnimator(EXItemAnimator *p);
    void DeleteAnimator(EXItemAnimator *p);
    EXItemAnimator *GetAnimator(const EXRuntimeClass *pClass) const;
    EXItemAnimator *GetAnimator(u32 hash) const;
    void *GetDatum_ChildLink(u32 type, EXDatumTemp &d, u32 flags, void *&pCtx) const;
    void GetDatum_ChildLink(u32 type, EXDatumTempArray &arr, u32 flags) const;
    void *GetNearestDatum_ChildLink(const EXVector &p, EXDatumTemp &d, u32 flags) const;
    s8 DoGrabPoint_ChildLink(const EXMatrix &mtx, EXGrabPointTemp &t, u32 flags) const;
    void DoRayCast_ChildLink(const EXMatrix &mtx, EXRayCastTemp &t, u32 flags) const;
    void DoShadowCast_ChildLink(EXShadowTemp &t) const;
    void DoEntityScoop_ChildLink(const EXMatrix &mtx, EXCollisionScoopTemp &t, u32 flags) const;
    s8 GetBoundsBox(EXBoundsBox &box, u32 flags);
    void UpdateBoundsBox(s8 b);
    void SetFrameRate60(float f);
    float FrameRate60() const;
    void DoItemUpdate();
    u32 DoItemRender(const EXMatrix &mtx, const EXRenderInfo &info);
protected:
    void ApplyMatrixBoundsBox(EXBoundsBox &a, EXBoundsBox &b, u32 flags) const;
public:
    static void *m_pNullDatumPtr;
    float m_frameRateMult;      // +0xf8
    float m_objectTime;         // +0xfc
    s8 m_allocatorCreated;      // +0x100
};
#endif
