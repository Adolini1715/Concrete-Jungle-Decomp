#include "EXCollisionScoopTemp.h"
#include "EXRayCastTemp.h"
#include "EXGrabPointTemp.h"
#include "EXShadowTemp.h"
#include "EXItemAnimator_Entity.h"
#include "EXParticleSys.h"
#include "EXParticleSysMgr.h"
#include "EXDatum.h"
#include "EXStdFuncs.h"
#include "EXMemory.h"

class EXGDiData;
class EXCScoopTemp;
class EXGeoGrabPoint;
struct EXGeoEntityDatums;

// TODO stand-in: geo extra-data table (EXGeoCommon area).
class EXGeoXtraData {
public:
    void *GetXtra(u8 type) const;
};

// TODO stand-in: base geo entity (EXGeoEntity area). Virtual slot order from the map.
class EXGeoBaseEntity {
public:
    virtual EXRuntimeClass *GetRuntimeClass() const;
    virtual void OnCreate(EXGeoHeader *, EXGeoObjectHeader *);
    virtual void OnDelete(EXGeoHeader *, EXGeoObjectHeader *);
    virtual void Draw(EXGDiData *, u32);
    virtual EXBoundsBox &GetBoundsBox();
    virtual void *GetVertexBuffer(EXGDiData *, s8);
    virtual void ReleaseVertexBuffer(EXGDiData *);
    virtual void AddCachedDraw(EXGDiData *, u32);
    virtual u32 ApplyFunction(u32 (__cdecl *)(EXGeoBaseEntity *, EXGDiData *), EXGDiData *);
    virtual u32 DoRayCast(EXRayCastTemp &, EXGDiData *, u32);
    virtual u32 DoShadowCast(EXShadowTemp &);
    virtual u32 DoEntityScoop_Sphere(EXCScoopTemp &, EXGDiData *, u32, u32);
    virtual u32 DoEntityScoop_Shape(EXCScoopTemp &, EXGDiData *, u32, u32);
    virtual u32 GetVertexCount() const;
    virtual void BuildEntityInfoArray(EXGeoEntity_InfoArray &, EXGDiData *);
    void *BaseXtraData(u8 type) const { return m_xtra.GetXtra(type); }
    u32 Flags() const { return m_flags; }
    // NEAR (stand-in): the emitted copy ends with `and al, 1` (ours `and eax, 1`).
    s8 HasCollision() const { return (s8)((m_flags >> 8) & 1); }
    EXGeoGrabPoint *GrabPoint() const { return (EXGeoGrabPoint *)BaseXtraData(6); }
    EXGeoEntityDatums *Datums() const { return (EXGeoEntityDatums *)BaseXtraData(5); }
    const EXBoundsBox &BoundsBox() const { return m_bounds; }
    union {
        u32 m_flags;        // +4
        struct {
            u32 m_bits0 : 8;
            u32 m_collision : 1;    // 0x100
        } m_bits;
    };
    u8 m_08[8];
    EXBoundsBox m_bounds;   // +0x10
    u8 m_30[0x10];
    EXGeoXtraData m_xtra;   // +0x40
};

struct EXGeoBaseDatum;
// TODO stand-in: entity datum table (geo file area); datums are 0x30 bytes, hash first.
struct EXGeoEntityDatum {
    u32 m_hash;
    u8 m_04[0x2c];
};
struct EXGeoEntityDatums {
    u8 m_00[5];
    u8 m_count;                                 // +5
    ex::EXRelPtr16<EXGeoEntityDatum> m_datums;  // +6
};

// TODO stand-in: entity object header (EXGeoFile area).
class EXGeoEntityHeader {
public:
    u32 m_hash;                 // +0
    u8 m_04[8];
    EXGeoBaseEntity *m_pEntity; // +0xc
};


// TODO stand-in: entity info array (EXGeoEntity area).
class EXGeoEntity_InfoArray {
public:
    EXGeoEntity_InfoArray();
    ~EXGeoEntity_InfoArray() { EXFree(m_pData); }
    u32 HashRef() const { return m_hashRef; }
    void SetMatrix(const EXMatrix &mtx) { m_mtx = mtx; }
    u32 GetRandomVertexArray(EXVector *pOut, int n, u32 flags);
    u8 m_00[0xc];
    void *m_pData;          // +0xc
    EXMatrix m_mtx;         // +0x10
    u8 m_50[4];
    u32 m_hashRef;          // +0x54
};
EX_IMPLEMENT_DYNCREATE(EXItemAnimator_Entity, EXItemAnimator)

void EXItemAnimator_Entity::CleanInitialise()
{
    m_render.RemoveGDiMemory();
    m_geoRef.Set(0, 0);
}

s8 EXItemAnimator_Entity::Initialise(EXGeoFile *pFile, u32 hash)
{
    CleanInitialise();
    m_128 = 0.0f;
    m_lodMin = 0;
    m_lodMax = 3;
    if (!pFile) return 0;
    // TODO layout: EXGeoFile +0x20 is the loaded geo header.
    EXGeoHeader *pHdr = *(EXGeoHeader **)((char *)pFile + 0x20);
    if (!pHdr) return 0;
    if (!m_render.Initialise(pHdr, hash)) return 0;
    m_geoRef.Set(pFile, hash);
    return 1;
}

void EXItemAnimator_Entity::DetatchParticleSys() const { m_effects.DetatchSystems(EXRUNTIME_CLASS(EXParticleSys)); }
void EXItemAnimator_Entity::DeleteParticleSys() const { m_effects.DeleteSystems(EXRUNTIME_CLASS(EXParticleSys)); }

EXParticleSys *EXItemAnimator_Entity::ParticleSys() const
{
    if (!m_effects.m_pHead) return 0;
    return (EXParticleSys *)m_effects.FindSystem(EXRUNTIME_CLASS(EXParticleSys));
}

void EXItemAnimator_Entity::EnableParticleSystems(s8 bEnable) const
{
    EXParticleSysMgr *pMgr = (EXParticleSysMgr *)m_effects.FindSystem(EXRUNTIME_CLASS(EXParticleSys));
    if (pMgr) {
        EXBaseParticleSys *pSys = (EXBaseParticleSys *)pMgr->ParticleSys(this);
        if (pSys) pSys->Enable(bEnable);
    }
}

EXItemAnimator_Entity::~EXItemAnimator_Entity()
{
    m_effects.DetatchSystems();
    CleanInitialise();
}

EXGeoCommonObject *EXItemAnimator_Entity::GetGeoObject() const
{
    EXGeoEntityHeader *pHdr = m_geoRef.GeoEntityHeader();
    return pHdr ? (EXGeoCommonObject *)pHdr->m_pEntity : 0;
}

void EXItemAnimator_Entity::SetLodMin(u32 lod)
{
    u32 lo = 0;
    u32 hi = 3;
    m_lodMin = (u8)limit(lod, lo, hi);
    m_lodMax = max(m_lodMin, m_lodMax);
}

void EXItemAnimator_Entity::SetLodMax(u32 lod)
{
    u32 lo = 0;
    u32 hi = 3;
    m_lodMax = (u8)limit(lod, lo, hi);
    m_lodMin = min(m_lodMin, m_lodMax);
}

s8 EXItemAnimator_Entity::DoGrabPoint(const EXMatrix &mtx, EXGrabPointTemp &t, u32 flags)
{
    EXGeoEntityHeader *pHdr = m_geoRef.GeoEntityHeader();
    EXGeoBaseEntity *pEnt = pHdr ? pHdr->m_pEntity : 0;
    EXGeoGrabPoint *pGrab = 0;
    if (pEnt && t.m_layer.CalcGrabpointEntity(pEnt)) pGrab = pEnt->GrabPoint();
    if (!pGrab) {
        Invalidate(flags);
        return 0;
    }
    const EXMatrix &m = CalcMatrix_InPlace(mtx, flags);
    s8 r = pGrab->DoGrabPoint(m, t);
    if (r) t.m_pAnimator = this;
    return r;
}

s8 EXItemAnimator_Entity::DoGrabPoint(const EXMatrix &mtx, EXGrabPointArrayTemp &t, u32 flags)
{
    EXGeoEntityHeader *pHdr = m_geoRef.GeoEntityHeader();
    EXGeoBaseEntity *pEnt = pHdr ? pHdr->m_pEntity : 0;
    EXGeoGrabPoint *pGrab = 0;
    if (pEnt && t.m_layer.CalcGrabpointEntity(pEnt)) pGrab = pEnt->GrabPoint();
    if (!pGrab) {
        Invalidate(flags);
        return 0;
    }
    const EXMatrix &m = CalcMatrix_InPlace(mtx, flags);
    return pGrab->DoGrabPoint(m, t);
}

void EXItemAnimator_Entity::DoShadowCast(EXShadowTemp &t)
{
    EXGeoEntityHeader *pHdr = m_geoRef.GeoEntityHeader();
    EXGeoBaseEntity *pEnt = pHdr ? pHdr->m_pEntity : 0;
    if (pEnt && pEnt->HasCollision() && t.m_layer.CalcCollisionEntity(pEnt)) pEnt->DoShadowCast(t);
}

void EXItemAnimator_Entity::GetGeoBoundsBox(EXBoundsBox &box)
{
    EXGeoEntityHeader *pHdr = m_geoRef.GeoEntityHeader();
    EXGeoBaseEntity *pEnt = pHdr ? pHdr->m_pEntity : 0;
    if (!pEnt) {
        box.SetZero();
        return;
    }
    box.Set(pEnt->BoundsBox());
}

s8 EXItemAnimator_Entity::_GetBoundsBox(EXBoundsBox &box, u32 flags)
{
    EXBoundsBox b;
    GetGeoBoundsBox(b);
    ApplyMatrixBoundsBox(b, box, flags);
    return 1;
}

void EXItemAnimator_Entity::BuildEntityInfoArray(const EXMatrix &mtx, EXGeoEntity_InfoArray &arr, s8 bCalc)
{
    EXGeoEntityHeader *pHdr = m_geoRef.GeoEntityHeader();
    if (!pHdr) return;
    if (arr.HashRef() && arr.HashRef() != pHdr->m_hash) return;
    EXGeoBaseEntity *pEnt = pHdr->m_pEntity;
    if (!pEnt) return;
    u32 flags = 0;
    const EXMatrix &m = bCalc ? CalcMatrix_InPlace(mtx, flags) : m_mtx;
    arr.SetMatrix(m);
    pEnt->BuildEntityInfoArray(arr, (EXGDiData *)GDiDataArray(0).m_00);
}

void EXItemAnimator_Entity::DoEntityScoop(const EXMatrix &mtx, EXCollisionScoopTemp &t, u32 flags)
{
    const EXMatrix &m = CalcMatrix_InPlace(mtx, flags);
    EXGeoEntityHeader *pHdr = m_geoRef.GeoEntityHeader();
    EXGeoBaseEntity *pEnt = pHdr ? pHdr->m_pEntity : 0;
    if (pEnt && pEnt->HasCollision() && t.m_layer.CalcCollisionEntity(pEnt)) {
        t.SetTemporaryVars(this, m, 0);
        pEnt->DoEntityScoop_Sphere(*(EXCScoopTemp *)&t, (EXGDiData *)GDiDataArray(0).m_00, 0, flags);
    }
    if (GetAnimator()) {
        const EXMatrix &m2 = CalcMatrix_InPlace(mtx, flags);
        DoEntityScoop_ChildLink(m2, t, flags);
    }
}

EXItemAnimator_Entity::EXItemAnimator_Entity()
{
    m_128 = 0.0f;
    m_lodMin = 0;
    m_lodMax = 3;
}

u32 EXItemAnimator_Entity::ParticlePosCallback(EXItemAnimator *pAnim, EXVector *pOut, u32 n)
{
    EXGeoEntity_InfoArray arr;
    pAnim->BuildEntityInfoArray(pAnim->m_mtx, arr, 0);
    return arr.GetRandomVertexArray(pOut, n, 0);
}

s8 EXItemAnimator_Entity::AttachParticleSys(EXGeoFile *pFile, u32 hash, const EXRuntimeClass *pClass)
{
    EXBaseParticleSys *pSys = (EXBaseParticleSys *)m_effects.AttachParticleSys(this, pFile, hash, pClass);
    if (!pSys) return 0;
    pSys->SetParticlePosCallback(ParticlePosCallback);
    return 1;
}

void *EXItemAnimator_Entity::GetDatum(u32 type, EXDatumTemp &d, u32 flags, void *&pCtx)
{
    EXGeoEntityHeader *pHdr = m_geoRef.GeoEntityHeader();
    EXGeoBaseEntity *pEnt = pHdr ? pHdr->m_pEntity : 0;
    if (!pEnt) return 0;
    EXGeoEntityDatums *pDatums = pEnt->Datums();
    if (!pDatums) return 0;
    int n = pDatums->m_count;
    for (int i = 0; i < n; i++) {
        u32 id = ((u32)this << 8) | i;
        if (!pCtx) {
            EXGeoEntityDatum *pDatum = &((EXGeoEntityDatum *)pDatums->m_datums)[i];
            if (pDatum->m_hash == type) {
                if (!(flags & 4)) {
                    EXMatrix m;
                    CalcMatrix(m);
                    d.Set((const EXGeoBaseDatum *)pDatum, EXVector(0.0f, 0.0f, 0.0f, 0.0f), m);
                } else {
                    d.Set((const EXGeoBaseDatum *)pDatum, EXVector(0.0f, 0.0f, 0.0f, 0.0f));
                }
                return (void *)id;
            }
        } else if (pCtx == (void *)id) {
            pCtx = 0;
        }
    }
    if (flags & 0x80) return GetDatum_ChildLink(type, d, flags, pCtx);
    return 0;
}

void EXItemAnimator_Entity::GetDatum(u32 type, EXDatumTempArray &arr, u32 flags)
{
    EXGeoEntityHeader *pHdr = m_geoRef.GeoEntityHeader();
    EXGeoBaseEntity *pEnt = pHdr ? pHdr->m_pEntity : 0;
    if (!pEnt) return;
    EXGeoEntityDatums *pDatums = pEnt->Datums();
    if (!pDatums) return;
    int n = pDatums->m_count;
    s8 bCalc = 1;
    EXMatrix m;
    for (int i = 0; i < n; i++) {
        EXGeoEntityDatum *pDatum = &((EXGeoEntityDatum *)pDatums->m_datums)[i];
        if (pDatum->m_hash == type) {
            int idx = arr.m_nSize;
            arr.SetSize(idx + 1, -1);
            EXDatumTemp &dt = arr.m_pData[idx];
            if (bCalc) {
                CalcMatrix(m);
                bCalc = 0;
            }
            dt.Set((const EXGeoBaseDatum *)pDatum, EXVector(0.0f, 0.0f, 0.0f, 0.0f), m);
        }
    }
    if (flags & 0x80) GetDatum_ChildLink(type, arr, flags);
}

void EXItemAnimator_Entity::DoRayCast(const EXMatrix &mtx, EXRayCastTemp &t, u32 flags)
{
    EXGeoEntityHeader *pHdr = m_geoRef.GeoEntityHeader();
    EXGeoBaseEntity *pEnt = pHdr ? pHdr->m_pEntity : 0;
    if (pEnt && pEnt->HasCollision() && t.m_layer.CalcCollisionEntity(pEnt)) {
        const EXMatrix &m = CalcMatrix_InPlace(mtx, flags);
        EXMatrix inv = m.Inverse3();
        t.m_localStart = (((const EXMaths::vec_xyzw &)t.m_start) - m.r[3]).get_w0();
        t.m_localStart *= inv;
        t.m_localDir = (const EXMaths::mat_44 &)inv * t.m_dir;
        t.CalcBoundsTemp();
        if (pEnt->DoRayCast(t, (EXGDiData *)GDiDataArray(0).m_00, 0)) t.SetValid(this);
        return;
    }
    Invalidate(flags);
}
