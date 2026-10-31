#ifndef EXGEOTEXTURE_H
#define EXGEOTEXTURE_H
#include "types.h"
#include "EXRelPtr.h"
#include "RGBA.h"

class EXGeoHeader;
class EXGeoFile;
class EXGeoTextureHeader;

// Texture description shared by file textures and runtime textures (0x20).
class EXBaseGeoTexture {
public:
    unsigned short Width() const { return m_Width; }
    unsigned short Height() const { return m_Height; }
    unsigned short Depth() const { return m_Depth; }
    unsigned short GameFlags() const { return m_GameFlags; }
    short ScrollU() const { return m_ScrollU; }
    short ScrollV() const { return m_ScrollV; }
    unsigned int FrameCount() const { return m_FrameCount; }
    unsigned int ImageCount() const { return m_ImageCount; }
    unsigned int FrameRate() const { return m_FrameRate; }
    unsigned int RegionsCount() const { return m_RegionsCount; }
    unsigned int MipCount() const { return m_MipCount; }
    unsigned int Format() const { return m_Format; }
    float MipBias() const { return m_MipBias; }
    ex::RGBA Colour() const { return m_Colour; }
    void SetRuntimeValues(unsigned short w, unsigned short h, unsigned char fmt, unsigned short gameFlags, short su, short sv);

    u16 m_Width;            // 0x00
    u16 m_Height;           // 0x02
    u16 m_Depth;            // 0x04
    u16 m_GameFlags;        // 0x06
    s16 m_ScrollU;          // 0x08
    s16 m_ScrollV;          // 0x0a
    u8 m_FrameCount;        // 0x0c
    u8 m_ImageCount;        // 0x0d
    u8 m_FrameRate;         // 0x0e
    u8 m_0f;                // 0x0f
    u8 m_ValueFlags;        // 0x10 which optional values are present
    u8 m_RegionsCount;      // 0x11
    u8 m_MipCount;          // 0x12
    u8 m_Format;            // 0x13
    float m_MipBias;        // 0x14
    ex::RGBA m_Colour;      // 0x18
    u32 m_GeoFileHash;      // 0x1c -1: data is in this file, else the file holding the base texture
};

// Sub-rectangle of a texture (0x18).
struct EXGeoTexture_Region {
    int Value(int i) const;
    u32 m_HashCode;                 // 0x00
    u8 m_ValueFlags;                // 0x04
    u8 m_05;
    ex::EXRelPtr16<int> m_Values;   // 0x06
    u8 m_08[0x10];
};

// File texture. For an external texture (m_GeoFileHash != -1) the tail holds the reference
// to the base texture instead of frame data.
class EXGeoTexture : public EXBaseGeoTexture {
public:
    EXGeoTexture *BaseTexture()
    {
        if (m_GeoFileHash == 0xffffffff) return this;
        return _BaseTexture();
    }
    unsigned int GetSize() const { return m_Size; }
    unsigned char *Frame(int i) const { return ((ex::EXRelPtr<u8> *)&m_TextureRef)[i]; }
    unsigned char *AnimSeqData() const { return m_AnimSeq; }
    EXGeoTexture_Region *RegionsData() const { return m_Regions; }
    int Value(int i) const;
    EXGeoTexture_Region *Region(unsigned int hash) const;
private:
    EXGeoTexture *_BaseTexture();
public:
    ex::EXRelPtr16<u8> m_AnimSeq;                   // 0x20
    ex::EXRelPtr16<int> m_Values;                   // 0x22
    u16 m_24;
    ex::EXRelPtr16<EXGeoTexture_Region> m_Regions;  // 0x26
    u32 m_Size;                                     // 0x28
    // Own texture: relptrs to the frames from 0x2c. External texture: the reference below.
    u32 m_TextureRef;                               // 0x2c hash, or index once resolved
    u32 m_CheckCode;                                // 0x30 file check code when resolved
    EXGeoFile *m_pGeoFile;                          // 0x34
    EXGeoTexture *m_pBaseTexture;                   // 0x38
};

// Material: texture hash resolved to its header.
class EXGeoMaterial {
public:
    void Initialise(const EXGeoHeader *pHdr);
    u32 m_00;
    u32 m_TextureHash;                      // 0x04
    EXGeoTextureHeader *m_pTextureHeader;   // 0x08
};
#endif
