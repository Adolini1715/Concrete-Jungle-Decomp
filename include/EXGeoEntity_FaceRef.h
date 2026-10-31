#ifndef EXGEOENTITY_FACEREF_H
#define EXGEOENTITY_FACEREF_H
#include "EXVector.h"
#include "EXArray.h"
#include "RGBA.h"

class EXGeoEntity;
class EXGeoFile;
class EXGDiData;
class EXTexture;
class EXGeoTexture;
// A face's three vertex indices in the entity's index data (EXGeoEntity::Index() + face tex index).
struct EXGeoEntity_TriStripVert {
    u16 m_v[3];
};

// One collision face: three vertex indices and the face flags (sizeof 10).
struct EXGeoEntity_FaceCollisionFace {
    unsigned int vI0() const { return m_v[0]; }
    unsigned int vI1() const { return m_v[1]; }
    unsigned int vI2() const { return m_v[2]; }
    u16 m_v[3];     // +0
    u16 m_flags;    // +6
    u16 m_tex;      // +8 TODO
};

// Collision face group: 4-byte header then the faces.
struct EXGeoEntity_FaceCollisionGroup {
    u16 m_texture;                                  // +0 texture index
    u16 m_02;                                       // +2 TODO
    EXGeoEntity_FaceCollisionFace m_faces[1];       // +4
};

// An entity's collision faces (EXGeoEntity::FaceCollision): a header with the group count, then
// the groups, each followed directly by the next.
struct EXGeoEntity_FaceCollision {
    u16 m_00;
    u16 m_GroupCount;   // 0x02
    u32 m_04;
    EXGeoEntity_FaceCollisionGroup m_Groups[1];  // 0x08
};

// Reference to a hit face (XBCollision.obj), sizeof 0x18.
class EXGeoEntity_FaceRef {
public:
    EXGeoEntity_FaceRef() { m_pEntity = 0; }
    void Initialise()
    {
        m_pEntity = 0;
        m_pGeoFile = 0;
    }
    void _set(EXGeoEntity *pEnt, EXGDiData *pGDi, EXGeoEntity_FaceCollisionGroup *pGroup, EXGeoEntity_FaceCollisionFace *pFace)
    {
        m_pEntity = pEnt;
        m_pGDi = pGDi;
        m_pGroup = pGroup;
        m_pFace = pFace;
    }
    void Set(EXGeoEntity *pEnt, EXGDiData *pGDi, EXGeoEntity_FaceCollisionGroup *pGroup, EXGeoEntity_FaceCollisionFace *pFace);
    void SetCheckCode();
    EXGeoEntity *Entity() const;
    EXGeoFile *GeoFile() const;
    unsigned int GetFaceFlags() const;
    unsigned int GetTextureIndex() const;
    EXTexture *Texture() const;
    EXGeoTexture *GeoTexture() const;
    ex::RGBA GetTextureColour() const;
    EXVector GetTextureColourV() const;
    EXVector GetFaceSingleColourV() const;
    EXVector GetFaceVertexColourV(const EXVector &p) const;
    EXVector GetFaceColourV(unsigned int flags) const;
    EXVector GetFaceColourV(unsigned int flags, const EXVector &p) const;
    ex::RGBA GetFaceColour(unsigned int flags) const;
    ex::RGBA GetFaceColour(unsigned int flags, const EXVector &p) const;
    signed char GetFaceVertexInfo(EXFixedArray<EXVector, 3> &v, EXFixedArray<EXVector2, 3> &uv) const;
    signed char GetFaceUVInfo(EXVector2 *const uv) const;
private:
    ex::RGBA MakeRGBA(const EXVector &c) const;
    EXGeoEntity_TriStripVert *GetTriStripVert() const;
public:
    EXGeoFile *m_pGeoFile;                      // +0x00 file the entity lives in
    u32 m_checkCode;                            // +0x04 the file's check code when the ref was made
    EXGeoEntity *m_pEntity;                     // +0x08
    EXGDiData *m_pGDi;                          // +0x0c
    EXGeoEntity_FaceCollisionGroup *m_pGroup;   // +0x10
    EXGeoEntity_FaceCollisionFace *m_pFace;     // +0x14
};
#endif
