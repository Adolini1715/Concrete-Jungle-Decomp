#ifndef EXRUNTIMECLASS_H
#define EXRUNTIMECLASS_H
#include "types.h"

// MFC-style runtime class records: {base, name, sizeof, factory}. The records sit in .rdata.
struct EXRuntimeClass {
    const EXRuntimeClass *m_pBaseClass;
    const char *m_pszClassName;
    int m_nObjectSize;
    void *(__cdecl *m_pfnCreateObject)();
    // Verified against the FX_Water.obj copy (jmp [ecx+0xc]).
    void *CreateObject() const { return m_pfnCreateObject(); }
    s8 IsDerivedFrom(const EXRuntimeClass *pBase) const
    {
        const EXRuntimeClass *p = this;
        do {
            if (p == pBase) return 1;
            p = p->m_pBaseClass;
        } while (p);
        return 0;
    }
};

#define EXRUNTIME_CLASS(cls) ((EXRuntimeClass *)&cls::class##cls)

// In the class body. Tested so far on EXItemHandler; adjust as more classes match.
#define EX_DECLARE_DYNAMIC(cls) \
protected: \
    static EXRuntimeClass *_GetBaseClass(); \
public: \
    static const EXRuntimeClass class##cls; \
    virtual EXRuntimeClass *GetRuntimeClass() const { return EXRUNTIME_CLASS(cls); } \
    s8 IsKindOf(const EXRuntimeClass *p) const { return GetRuntimeClass()->IsDerivedFrom(p); }

// A root class names itself as its _GetBaseClass result but has a null m_pBaseClass.
#define EX_IMPLEMENT_DYNAMIC_ROOT(cls) \
    EXRuntimeClass *cls::_GetBaseClass() { return EXRUNTIME_CLASS(cls); } \
    const EXRuntimeClass cls::class##cls = { 0, #cls, sizeof(cls), 0 };

#define EX_IMPLEMENT_DYNAMIC(cls, base) \
    EXRuntimeClass *cls::_GetBaseClass() { return EXRUNTIME_CLASS(base); } \
    const EXRuntimeClass cls::class##cls = { EXRUNTIME_CLASS(base), #cls, sizeof(cls), 0 };

// Variant with a factory: CreateObject() { return new cls; }
#define EX_IMPLEMENT_DYNCREATE(cls, base) \
    cls *cls::CreateObject() { return new cls; } \
    EXRuntimeClass *cls::_GetBaseClass() { return EXRUNTIME_CLASS(base); } \
    const EXRuntimeClass cls::class##cls = { EXRUNTIME_CLASS(base), #cls, sizeof(cls), (void *(__cdecl *)())cls::CreateObject };
// Root class with a factory (EXFile).
#define EX_IMPLEMENT_DYNCREATE_ROOT(cls) \
    cls *cls::CreateObject() { return new cls; } \
    EXRuntimeClass *cls::_GetBaseClass() { return EXRUNTIME_CLASS(cls); } \
    const EXRuntimeClass cls::class##cls = { 0, #cls, sizeof(cls), (void *(__cdecl *)())cls::CreateObject };
#endif
