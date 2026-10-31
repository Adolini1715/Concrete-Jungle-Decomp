#ifndef EXGEOCOMPLEXREFOBJECT_H
#define EXGEOCOMPLEXREFOBJECT_H
#include "types.h"

class EXGeoFile;
class EXGeoEntity;
class EXGeoEntityHeader;
class EXGeoObjectHeader;
class EXGeoCommonObject;
class EXGeoAnim;
class EXGeoAnimHeader;
class EXGeoMapHeader;
class EXGeoScript;
class EXGeoScriptHeader;
class EXGeoAnimSkinHeader;
class EXGeoAnimSkin;
class EXGeoParticleHeader;
class EXGeoParticle;
class EXGeoSwooshHeader;
class EXGeoSwoosh;
struct EXGeoBaseRefObject;

// Reference to an object inside a geo file: file, hash, cached index (sizeof 0x10).
// Inline shapes verified from the item objects; the out-of-line members live in EXGeoFile.obj.
struct EXGeoComplexRefObject {
    EXGeoComplexRefObject() { m_08 = -1; m_0c = 0; }
    void Set(EXGeoFile *pFile, u32 hash) { m_pGeoFile = pFile; m_hashRef = hash; m_08 = -1; }
    void Set(const EXGeoBaseRefObject &r);
    void Set(const EXGeoComplexRefObject &r);
    void SetGeoFile(EXGeoFile *pFile);
    void SetHashRef(u32 hash);
    EXGeoFile *GeoFile() const { return m_pGeoFile; }
    u32 HashRef() const { return m_hashRef; }
    EXGeoEntityHeader *GeoEntityHeader() const;
    EXGeoObjectHeader *GeoObjectHeader() const;
    EXGeoAnimSkinHeader *GeoAnimSkinHeader() const;
    EXGeoScriptHeader *GeoScriptHeader() const;
    EXGeoParticleHeader *GeoParticleHeader() const;
    EXGeoSwooshHeader *GeoSwooshHeader() const;
    // TODO layout: EXGeoObjectHeader (geo area) holds its object pointer at +0xc.
    EXGeoCommonObject *GeoObject() const
    {
        EXGeoObjectHeader *h = GeoObjectHeader();
        return h ? *(EXGeoCommonObject **)((char *)h + 0xc) : 0;
    }
    EXGeoAnimHeader *GeoAnimHeader() const;
    EXGeoMapHeader *GeoMapHeader() const;
    // TODO layout: EXGeoAnimHeader (geo area) holds its anim pointer at +0xc.
    EXGeoAnim *GeoAnim() const
    {
        EXGeoAnimHeader *h = GeoAnimHeader();
        return h ? *(EXGeoAnim **)((char *)h + 0xc) : 0;
    }
    // Non-const form (EXItemCtrl_Anim.obj copy; appended by the item agent).
    EXGeoAnim *GeoAnim()
    {
        EXGeoAnimHeader *h = GeoAnimHeader();
        return h ? *(EXGeoAnim **)((char *)h + 0xc) : 0;
    }
    // Script object (EXItemAnimator_Script.obj copies; appended by the item agent).
    EXGeoScript *GeoScript()
    {
        EXGeoScriptHeader *h = GeoScriptHeader();
        return h ? *(EXGeoScript **)((char *)h + 0xc) : 0;
    }
    EXGeoScript *GeoScript() const
    {
        EXGeoScriptHeader *h = GeoScriptHeader();
        return h ? *(EXGeoScript **)((char *)h + 0xc) : 0;
    }
    // Skin object (EXItemRender_Anim.obj copies; appended by the item agent).
    EXGeoAnimSkin *GeoAnimSkin()
    {
        EXGeoAnimSkinHeader *h = GeoAnimSkinHeader();
        return h ? *(EXGeoAnimSkin **)((char *)h + 0xc) : 0;
    }
    EXGeoAnimSkin *GeoAnimSkin() const
    {
        EXGeoAnimSkinHeader *h = GeoAnimSkinHeader();
        return h ? *(EXGeoAnimSkin **)((char *)h + 0xc) : 0;
    }
protected:
    void Invalidate() { m_08 = -1; }
public:
    // Cache-refreshing accessors (EXGeoFile.obj). Callers that want the const versions (retail
    // binds e.g. EXItemAnimator_Map's GeoMapHeader calls to the const one) use a const reference.
    EXGeoEntityHeader *GeoEntityHeader(const EXGeoFile *pFile, unsigned int checkCode);
    EXGeoEntityHeader *GeoEntityHeader();
    EXGeoObjectHeader *GeoObjectHeader(const EXGeoFile *pFile, unsigned int checkCode);
    EXGeoObjectHeader *GeoObjectHeader();
    EXGeoAnimSkinHeader *GeoAnimSkinHeader(const EXGeoFile *pFile, unsigned int checkCode);
    EXGeoAnimSkinHeader *GeoAnimSkinHeader();
    EXGeoScriptHeader *GeoScriptHeader(const EXGeoFile *pFile, unsigned int checkCode);
    EXGeoScriptHeader *GeoScriptHeader();
    EXGeoParticleHeader *GeoParticleHeader(const EXGeoFile *pFile, unsigned int checkCode);
    EXGeoParticleHeader *GeoParticleHeader();
    EXGeoSwooshHeader *GeoSwooshHeader(const EXGeoFile *pFile, unsigned int checkCode);
    EXGeoSwooshHeader *GeoSwooshHeader();
    // Appended by the effects agent (EXSwoosh.obj copy). TODO layout: header holds the object at +0xc.
    EXGeoSwoosh *GeoSwoosh()
    {
        EXGeoSwooshHeader *h = GeoSwooshHeader();
        return h ? *(EXGeoSwoosh **)((char *)h + 0xc) : 0;
    }
    // Particle definition (XBParticle.obj copy). TODO layout: header holds the object at +0xc.
    EXGeoParticle *GeoParticle()
    {
        EXGeoParticleHeader *h = GeoParticleHeader();
        return h ? *(EXGeoParticle **)((char *)h + 0xc) : 0;
    }
    EXGeoMapHeader *GeoMapHeader(const EXGeoFile *pFile, unsigned int checkCode);
    EXGeoMapHeader *GeoMapHeader();
    EXGeoAnimHeader *GeoAnimHeader(const EXGeoFile *pFile, unsigned int checkCode);
    EXGeoAnimHeader *GeoAnimHeader();
    EXGeoFile *m_pGeoFile;  // +0
    u32 m_hashRef;          // +4
    s32 m_08;               // +8 file check code the cache belongs to, -1 = invalid
    union {                 // +0xc cached list element
        u32 m_0c;
        void *m_pHeader;
        EXGeoEntityHeader *m_pEntityHeader;
        EXGeoObjectHeader *m_pObjectHeader;
        EXGeoAnimHeader *m_pAnimHeader;
        EXGeoAnimSkinHeader *m_pAnimSkinHeader;
        EXGeoScriptHeader *m_pScriptHeader;
        EXGeoParticleHeader *m_pParticleHeader;
        EXGeoSwooshHeader *m_pSwooshHeader;
        EXGeoMapHeader *m_pMapHeader;
    };
};
#endif
