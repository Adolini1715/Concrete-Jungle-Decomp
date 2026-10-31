#ifndef EXITEM_H
#define EXITEM_H
#include "EXPlacementItem.h"
#include "EXMapOn.h"
#include "EXMemory.h"

class EXItemAnimator;
class EXItemHandler;
class EXItemPhysics;
class EXItemEnv;
class EXBaseTrigger;
struct EXDatumTemp;
class EXDatumTempArray;
struct EXShadowTemp;
class EXCollisionScoopTemp;
struct EXGrabPointTemp;
struct EXGrabPointArrayTemp;
struct EXRayCastTemp;

// A scene item: placement plus animators, handler, physics, trigger and list-group links.
// sizeof 0x160. Groups 0..4 are embedded, further groups (EXItemEnv count - 5) are allocated.
class EXItem : public EXPlacementItem {
    EX_DECLARE_DYNAMIC(EXItem)
public:
    virtual ~EXItem();
    EX_DECLARE_ITEMTYPE(EXItem)
    virtual void *GetAnimatorDatum(u32 type, EXDatumTemp &d, u32 flags, void *pCtx) const;  // slot 7
    virtual void GetAnimatorDatum(u32 type, EXDatumTempArray &arr, u32 flags) const;        // slot 6
    virtual s8 GetAnimatorBoundsBox(EXBoundsBox &box, u32 flags) const;
    virtual void AnimatorApplyFunction(u32 (__cdecl *pFunc)(EXItemAnimator *, void *), void *pData);
    virtual void DoItemUpdate();
    virtual void DoItemPhysics();
    virtual u32 DoItemRender(const EXMatrix &mtx, const EXRenderInfo &info);
    EXItem();
    static EXItem *CreateObject();
    static EXItem *NewItem() { return new EXItem; }
    EXItemHandler *ItemHandler() const { return m_pItemHandler; }
    EXItemPhysics *ItemPhysics() const { return m_pItemPhysics; }
    EXBaseTrigger *Trigger() const { return m_pTrigger; }
    EXItem *GetXItemOwner() const { return m_pXItemOwner; }
    void SetXItemOwner(EXItem *p) { m_pXItemOwner = p; }
    void SetMapOn(EXMapOn m) { m_mapOn = m; }
    EXMapOn MapOn() const { return m_mapOn; }
    s16 PortalOn() const { return m_portalOn; }
    void SetClipType(u32 t) { m_clipType = (u8)t; m_clipFlags = 0; }
    u8 ClipType() const { return m_clipType; }
    u8 ClipFlags() const { return m_clipFlags; }
    void SetClipFlags(u32 f) { m_clipFlags = (u8)f; }
    void SetListSortValue(u32 v) { m_listSortValue = (u8)v; }
    u32 ListSortValue() const { return m_listSortValue; }
    s8 BuildAnimatorBoundsBox(u32 flags) { return GetAnimatorBoundsBox(m_boundsBox, flags); }
    EXItemAnimator *GetAnimator() const
    {
        EXDListItem *p = m_animators.Head();
        return p ? (EXItemAnimator *)((char *)p - 4) : 0;
    }
    EXODListItem &ItemGroup(u32 i)
    {
        if (i < 5) return m_groups[i];
        return m_pExtraGroups[i - 5];
    }
    void UpdateGroups(EXItemEnv *pEnv, u32 flags);
    void UpdateGroupsTail(EXItemEnv *pEnv, u32 flags);
    void EnableParticleSystems(s8 b) const;
    void SetItemHandler(EXItemHandler *p);
    void SetItemPhysics(EXItemPhysics *p);
    void SetTrigger(EXBaseTrigger *p);
    void AddAnimator(EXItemAnimator *p);
    void RemoveAnimator(EXItemAnimator *p);
    void DeleteAnimator(EXItemAnimator *p);
    EXItemAnimator *GetAnimator(const EXRuntimeClass *pClass) const;
    EXItemAnimator *GetAnimator(u32 index) const;
    void DoItemShadowCast(EXMatrix &mtx, EXShadowTemp &t);
    void SetAnimatorRenderState(u32 state, s8 bOn) const;
    void SetAnimatorLodRange(int lo, int hi) const;
    void SetAnimatorMinLod(int lod) const;
    void SetAnimatorMaxLod(int lod) const;
    void DoItemUpdate_Animators();
    void DoItemPreRender();
    void DoItemCollisionScoop(const EXMatrix &mtx, const EXItem *pItem, EXCollisionScoopTemp &t);
    void DoItemCollisionScoop(const EXMatrix &mtx, EXCollisionScoopTemp &t);
    void DoItemGrabPoint(const EXMatrix &mtx, EXGrabPointTemp &t);
    void DoItemGrabPoint(const EXMatrix &mtx, EXGrabPointArrayTemp &t);
    void DoItemRayCast(const EXMatrix &mtx, EXRayCastTemp &t);

    u32 m_f8;                       // +0xf8 item-group membership bits (UpdateGroups)
    EXODListItem m_groups[5];       // +0xfc
    EXODListItem *m_pExtraGroups;   // +0x138
    EXDList m_animators;            // +0x13c
    EXItemHandler *m_pItemHandler;  // +0x144
    EXItemPhysics *m_pItemPhysics;  // +0x148
    EXBaseTrigger *m_pTrigger;      // +0x14c
    EXItem *m_pXItemOwner;          // +0x150
    EXMapOn m_mapOn;                // +0x154
    s16 m_portalOn;                 // +0x156
    u8 m_clipType;                  // +0x158
    u8 m_clipFlags;                 // +0x159
    u8 m_listSortValue;             // +0x15a
    u8 m_15b;                       // +0x15b TODO
    u32 m_15c;                      // +0x15c TODO
};
#endif
