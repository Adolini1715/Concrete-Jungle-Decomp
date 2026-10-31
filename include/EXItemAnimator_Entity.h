#ifndef EXITEMANIMATOR_ENTITY_H
#define EXITEMANIMATOR_ENTITY_H
#include "EXItemAnimator.h"
#include "EXItemRender_Entity.h"
#include "EXEffectsSys.h"

class EXParticleSys;

// Animator rendering a static geo entity (sizeof 0x134).
class EXItemAnimator_Entity : public EXItemAnimator {
    EX_DECLARE_DYNAMIC(EXItemAnimator_Entity)
public:
    virtual ~EXItemAnimator_Entity();
    EX_DECLARE_ITEMTYPE(EXItemAnimator_Entity)
    virtual u32 DoRender(const EXMatrix &mtx, const EXRenderInfo &info, u32 flags);
    virtual s8 Initialise(EXGeoFile *pFile, u32 hash);
    virtual void EnableParticleSystems(s8 bEnable) const;
    virtual void *GetDatum(u32 type, EXDatumTemp &d, u32 flags, void *&pCtx);
    virtual void GetDatum(u32 type, EXDatumTempArray &arr, u32 flags);
    virtual s8 DoGrabPoint(const EXMatrix &mtx, EXGrabPointTemp &t, u32 flags);
    virtual s8 DoGrabPoint(const EXMatrix &mtx, EXGrabPointArrayTemp &t, u32 flags);
    virtual void DoRayCast(const EXMatrix &mtx, EXRayCastTemp &t, u32 flags);
    virtual void DoShadowCast(EXShadowTemp &t);
    virtual void DoEntityScoop(const EXMatrix &mtx, EXCollisionScoopTemp &t, u32 flags);
    virtual void SetLodMin(u32 lod);
    virtual void SetLodMax(u32 lod);
    virtual void BuildEntityInfoArray(const EXMatrix &mtx, EXGeoEntity_InfoArray &arr, s8 b);
    virtual EXGeoCommonObject *GetGeoObject() const;
protected:
    virtual s8 _GetBoundsBox(EXBoundsBox &box, u32 flags);
public:
    EXItemAnimator_Entity();
    static EXItemAnimator_Entity *CreateObject();
    static EXItemAnimator_Entity *NewItem() { return new EXItemAnimator_Entity; }
    const EXGDiDataArray &GDiDataArray(int i) const { return m_render.GDiDataArray(i); }
    void DetatchParticleSys() const;
    void DeleteParticleSys() const;
    EXParticleSys *ParticleSys() const;
    void GetGeoBoundsBox(EXBoundsBox &box);
    static u32 ParticlePosCallback(EXItemAnimator *pAnim, EXVector *pOut, u32 n);
    s8 AttachParticleSys(EXGeoFile *pFile, u32 hash, const EXRuntimeClass *pClass);
protected:
    void CleanInitialise();
public:
    EXItemRender_Entity m_render;   // +0x104
    u8 m_lodMin;                    // +0x124
    u8 m_lodMax;                    // +0x125
    float m_128;                    // +0x128 TODO name
    EXEffectsSysTag m_effects;      // +0x12c
};
#endif
