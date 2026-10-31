#ifndef EXEFFECTSSYS_H
#define EXEFFECTSSYS_H
#include "types.h"
#include "EXRuntimeClass.h"
#include "EXList.h"
#include "EXGeoComplexRefObject.h"
#include "EXRenderInfo.h"

class EXItemAnimator;
class EXGeoFile;
class EXEffectsSysMgr;

// Base of every effects system (particles, swooshes). sizeof 0x54 (runtime-class record).
// The EXLDListItem base sits after the vptr and links the system into its owner's EXEffectsSysTag.
class EXEffectsSys : public EXLDListItem {
    EX_DECLARE_DYNAMIC(EXEffectsSys)
public:
    virtual ~EXEffectsSys();
    virtual void UpdateRenderInfo(const EXRenderInfo &info);
    EXEffectsSys();
    const EXGeoComplexRefObject &GeoRefObject() const { return m_geoRef; }
    EXEffectsSysMgr *SystemMgr() const { return m_pMgr; }
    void Remove() { EXLDListItem::Remove(); }

    EXGeoComplexRefObject m_geoRef;     // +0x10
    EXRenderInfo m_renderInfo;          // +0x20
    EXEffectsSysMgr *m_pMgr;            // +0x50
};

// Owner of a family of systems (EXParticleSysMgr, EXSwooshSysMgr). The base virtuals are asserts.
class EXEffectsSysMgr {
public:
    EXEffectsSysMgr() {}
    virtual ~EXEffectsSysMgr() {}
    virtual s8 DetatchSystem(const EXEffectsSys *pSys);
    virtual s8 DeleteSystem(const EXEffectsSys *pSys);
    virtual void RemoveSystem(const EXEffectsSys *pSys);
};

// List of effects systems attached to an animator (EXLDList of the systems' EXLDListItem bases).
class EXEffectsSysTag : public EXLDList {
public:
    EXEffectsSysTag() {}
    ~EXEffectsSysTag();
    EXEffectsSys *First() const { return static_cast<EXEffectsSys *>(m_pHead); }
    static EXEffectsSys *Next(const EXEffectsSys *p) { return static_cast<EXEffectsSys *>(p->m_pNext); }
    void Add(EXEffectsSys *pSys) { AddHead(pSys); }
    void DetatchSystems(const EXRuntimeClass *pClass) const;
    void DetatchSystems() const;
    void DeleteSystems(const EXRuntimeClass *pClass) const;
    void DeleteSystems() const;
    EXEffectsSys *FindSystem(const EXRuntimeClass *pClass) const;
    void UpdateRenderInfo(const EXRenderInfo &info) const;
    EXEffectsSys *AttachParticleSys(EXItemAnimator *pAnim, EXGeoFile *pFile, u32 hash, const EXRuntimeClass *pClass);
};
#endif
