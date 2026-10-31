#ifndef EXGEOCOMMONOBJECT_H
#define EXGEOCOMMONOBJECT_H
#include "types.h"
#include "EXRuntimeClass.h"
#include "EXArray.h"
#include "EXNew.h"

class EXGeoHeader;
class EXGeoObjectHeader;
class EXGDiData;
struct EXBoundsBox;

// Factory record for objects that live inside geo files: {placement-new function, type id}.
// The file stores the type id in the object's first dword; _EXGeoCommonObjectDef::Create finds the
// record and runs the factory over it, which writes the vptr.
typedef void (__cdecl *EXGeoCommonObjectCreateFn)(void *p, int index);
struct _EXGEOCOMMONOBJECTDEF {
    EXGeoCommonObjectCreateFn m_pfnCreate;  // called with the def's index as a second argument
    u32 m_Type;
};

class _EXGeoCommonObjectDefArray : public EXDataArray<_EXGEOCOMMONOBJECTDEF, const _EXGEOCOMMONOBJECTDEF &> {
public:
    _EXGeoCommonObjectDefArray() { Init(0, 0); }
    ~_EXGeoCommonObjectDefArray() {}
};

class _EXGeoCommonObjectDef {
public:
    _EXGeoCommonObjectDef(_EXGEOCOMMONOBJECTDEF def);
    ~_EXGeoCommonObjectDef();
    static void Create(void *p, unsigned int type);
    static _EXGeoCommonObjectDefArray *m_gpDefList;
};

// In the class body of a geo object class (after EX_DECLARE_DYNAMIC).
#define EX_DECLARE_GEOOBJECT(cls) \
public: \
    static void _GCO_N_##cls(void *p);

// In EXGeoCommon.cpp: placement factory and its registration.
#define EX_IMPLEMENT_GEOOBJECT(cls, type) \
    void cls::_GCO_N_##cls(void *p) { new (p) cls; } \
    _EXGEOCOMMONOBJECTDEF _GCO_L_##cls = { (EXGeoCommonObjectCreateFn)cls::_GCO_N_##cls, type }; \
    _EXGeoCommonObjectDef _GCO_D_##cls(_GCO_L_##cls);

// Base of every polymorphic object stored in a geo file (sizeof 4: only the vptr is added).
class EXGeoCommonObject {
    EX_DECLARE_DYNAMIC(EXGeoCommonObject)
    EX_DECLARE_GEOOBJECT(EXGeoCommonObject)
public:
    virtual void OnCreate(EXGeoHeader *, EXGeoObjectHeader *);
    virtual void OnDelete(EXGeoHeader *, EXGeoObjectHeader *);
    virtual void Draw(EXGDiData *, unsigned int);
    virtual EXBoundsBox &GetBoundsBox();
    EXGeoCommonObject() {}
    void Create(EXGeoHeader *pHdr, EXGeoObjectHeader *pObjHdr)
    {
        _EXGeoCommonObjectDef::Create(this, *(u32 *)this);
        OnCreate(pHdr, pObjHdr);
    }
};
#endif
