#ifndef EXPLACEMENTITEM_H
#define EXPLACEMENTITEM_H
#include "EXBaseItem.h"
#include "EXRuntimeClass.h"
#include "EXFlag.h"
#include "EXMatrix.h"
#include "EXQuaternion.h"
#include "EXBounds.h"
#include "EXRenderInfo.h"
#include "EXGeoComplexRefObject.h"

class EXGeoFile;

// TypeName()/Size() overrides every concrete item class carries (header inlines).
#define EX_DECLARE_ITEMTYPE(cls) \
    virtual const char *TypeName() const { return #cls; } \
    virtual int Size() const { return sizeof(cls); }
class EXExtRenderInfo;
class EXTexSwapInfo;

// An item with a placement (position/rotation/scale, cached matrix, bounds, render info).
// sizeof 0xf8. Item flag bits (m_itemFlags): 1 has render, 2 update, 4 post update (EXItem),
// 8 updated this frame, 0x10 delete when 0x80 clear.
// Render flag bits (m_renderFlags): 1 has rotation, 2 has scale, 4/8 remote matrix (8: also the
// parent), 0x10 item, 0x20 pre-rendered. m_ownFlags: 1 owns ext render info, 2 owns tex swap info.
class EXPlacementItem : public EXBaseItem {
    EX_DECLARE_DYNAMIC(EXPlacementItem)
public:
    virtual ~EXPlacementItem();
    EX_DECLARE_ITEMTYPE(EXPlacementItem)
    virtual void DoUpdate() {}
    virtual u32 DoRender(const EXMatrix &mtx, const EXRenderInfo &info, u32 flags);
    EXPlacementItem();
    static EXPlacementItem *CreateObject();

    void Invalidate() { m_validMtx = 0; }
    void Invalidate(u32 flags);
    const EXMatrix &mtx() const { return m_mtx; }
    void SetRemoteMatrix(const EXMatrix *pMtx) { m_pRemoteMatrix = pMtx; }
    void SetRemoteMatrix(const EXMatrix &mtx) { m_pRemoteMatrix = &mtx; }
    void SetMatrix(const EXMatrix &mtx) { m_mtx = mtx; m_validMtx = 0; }
    void SetPosition(const EXVector &v) { m_position = v; m_validMtx = 0; }
    void SetRotation(const EXQuaternion &q) { m_rotation = EXVector(q.x, q.y, q.z, q.w); m_validMtx = 0; }  // shape verified (Actor.obj copy, Trigger.obj)
    void SetScale(const EXVector &v) { m_scale = v; m_validMtx = 0; }  // shape verified (Actor.obj copy)
    const EXVector &Position() const { return m_position; }
    const EXVector &Rotation() const { return m_rotation; }
    const EXVector &Scale() const { return m_scale; }
    void SetRotOrder(u8 order) { m_rotOrder = order; m_validMtx = 0; }
    u8 RotOrder() const { return m_rotOrder; }
    ex::EXFlag<u8> &RenderFlags() { return m_renderFlags; }
    ex::EXFlag<u16> &ItemFlags() { return m_itemFlags; }
    const ex::EXFlag<u16> &ItemFlags() const { return m_itemFlags; }
    EXRenderInfo &RenderInfo() { return m_renderInfo; }
    void SetHasRender() { m_itemFlags.m_flags |= 1; m_validMtx = 0; }
    void ClrHasRender() { m_itemFlags.m_flags &= ~1; }
    EXGeoFile *GeoFile() const { return m_geoRef.m_pGeoFile; }
    u32 RefObject() const { return m_geoRef.m_hashRef; }
    void SetRefObject(u32 hash) { m_geoRef.SetHashRef(hash); }
    void SetGeoFile(EXGeoFile *pFile) { m_geoRef.SetGeoFile(pFile); }
    EXGeoComplexRefObject &GeoRefObject() { return m_geoRef; }
    s8 ValidMtx() const { return m_validMtx; }
    u8 ValidBounds() const { return m_validBounds; }
    s8 HasMtx() const { return m_hasMtx; }
    const EXBoundsBox &BoundsBox() const { return m_boundsBox; }
    // First child item (not in the map: always inlined). The ternary shapes matter for the
    // register allocation of inlined RenderNodes calls.
    EXPlacementItem *FirstNode() const
    {
        return m_nodes.Head() ? (EXPlacementItem *)((char *)m_nodes.Head() - 4) : 0;
    }
    u32 RenderNodes(const EXMatrix &mtx, const EXRenderInfo &info, u32 flags) const
    {
        EXPlacementItem *pChild = FirstNode();
        return pChild ? _RenderNodes(mtx, info, flags, pChild) : 0;
    }
    void UpdateNodes()
    {
        EXPlacementItem *pChild = FirstNode();
        if (pChild) _UpdateNodes(pChild);
    }
    EXExtRenderInfo *GetExtRenderInfo();
    EXTexSwapInfo *SetTexSwapInfo();
    void CalcMatrix(EXMatrix &out) const;
    const EXMatrix &CalcMatrix_InPlace(const EXMatrix &parent, u32 &flags);
    const EXMatrix &CalcMatrix_InPlaceParent();
protected:
    void _UpdateNodes(EXPlacementItem *pItem);
    static u32 _RenderNodes(const EXMatrix &mtx, const EXRenderInfo &info, u32 flags, EXPlacementItem *pItem);
    void CalcMatrix_Vap(const EXMatrix &parent, EXMatrix &out, u32 flags) const;
private:
    void CalcMatrix_Scale(EXMaths::mat_44 &m) const;
    void CalcMatrix_Remote(EXMaths::mat_44 &m, const EXMatrix &parent) const;
public:
    EXGeoComplexRefObject m_geoRef;     // +0x1c
    EXBoundsBox m_boundsBox;            // +0x2c
    EXRenderInfo m_renderInfo;          // +0x4c
    ex::EXFlag<u8> m_renderFlags;       // +0x7c
    ex::EXFlag<u8> m_ownFlags;          // +0x7d
    s8 m_validMtx;                      // +0x7e
    u8 m_7f_0 : 1;                      // +0x7f bit 0 TODO
    u8 m_validBounds : 1;               //       bit 1
    u8 m_hasMtx : 1;                    //       bit 2
    u8 m_rotOrder;                      // +0x80
    const EXMatrix *m_pRemoteMatrix;    // +0x84
    EXMatrix m_mtx;                     // +0x88
    EXVector m_position;                // +0xc8
    EXVector m_rotation;                // +0xd8 (quaternion)
    EXVector m_scale;                   // +0xe8
};
#endif
