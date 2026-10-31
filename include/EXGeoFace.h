#ifndef EXGEOFACE_H
#define EXGEOFACE_H
#include "EXGeoCommonObject.h"

struct EXPrimVert;
// Face vertex as stored in the file; laid out like EXPrimVert (0x20) so it is drawn directly.
struct GEOFACE_VTX {
    u8 m_00[0x20];
};

// Single textured triangle fan placed in a map (sizeof 0x90, up to 4 vertices).
class EXGeoFace : public EXGeoCommonObject {
    EX_DECLARE_DYNAMIC(EXGeoFace)
    EX_DECLARE_GEOOBJECT(EXGeoFace)
public:
    virtual void Draw(EXGDiData *, unsigned int);
    EXGeoFace() {}
    unsigned int Flags() const { return m_Flags; }
    unsigned int VertexCount() const { return m_VertexCount; }
    unsigned int TextureRef() const { return m_TextureRef; }
    const GEOFACE_VTX *Vtx() const { return m_Vtx; }
    void Create(EXGeoHeader *pHdr, EXGeoObjectHeader *pObjHdr)
    {
        _EXGeoCommonObjectDef::Create(this, *(u32 *)this);
        OnCreate(pHdr, pObjHdr);
    }

    u32 m_TextureRef;       // 0x04
    u32 m_VertexCount;      // 0x08
    u32 m_Flags;            // 0x0c
    GEOFACE_VTX m_Vtx[4];   // 0x10
};
#endif
