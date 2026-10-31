#ifndef XBGEOENTITY_H
#define XBGEOENTITY_H
#include "types.h"
#include "EXRelPtr.h"
#include "EXMemory.h"
#include "EXArray.h"

// Xbox side of EXGeoEntity (XBGeoEntity.obj). Layout recovered from the code; names from the map.

#include "xbd3d8.h"

class EXGeoEntity;
class EXGDiSkinData;
class EXGDiData;
class EXMatrix;

// A vertex buffer header plus the pointer the renderer binds (16 bytes; arrays at +0xac / +0xb0).
struct XBVertexBufferRef {
    xb::D3DVertexBuffer *m_pVB;     // 0x00
    xb::D3DVertexBuffer m_VB;       // 0x04
};

// One triangle strip (0x14): m_FaceCount + 2 indices from the running offset into the index buffer.
struct EXGeoEntity_TriStrip {
    u32 m_FaceCount;    // 0x00
    s32 m_Texture;      // 0x04 index into the entity's texture list (the low 16 bits; heat render reads it whole)
    u32 m_08;
    // 8 alpha texture (blend instead of alpha test), 0x10 hidden, 0x40 two-sided, 0x80 reapply the entity's cull mode, 0x200 unlit, 0x400 no shadow,
    // 0x800 left out of the shadow map.
    u16 m_Flags;        // 0x0c
    u8 m_TransType;     // 0x0e 0 normal (alpha test, blend if needed), 1 additive, 2 subtractive
    u8 m_Layer;         // 0x0f bit tested against EXCachedGeoEntity::m_FadeLayerMask
    u32 m_10;
};
struct EXGeoEntity_FaceInfo;
// TODO stand-in: what EXGDiSkinData's first dword points at (skinning state; +0x10 matrix palette index?).
struct XBSkinBlock {
    u32 m_00[4];
    int m_MorphIndex;   // 0x10 >= 0: a morph target set is blended in (EXGeoAnimSkin_SkinList::MorphIndex)
};
class EXItemRender_SkinAnim;
namespace ex {
// TODO layout: 128-bit table entry (only its address is used here).
struct u128 {
    u32 m_v[4];
};
}
// Skinning data of one entity of a skin: bones used (the matrix palette, one bone index byte each)
// and the per-vertex weight table (XBWeightData, 0x14 per vertex).
struct EXGeoAnimSkin_NewData {
    int MtxPaletteSize() const { return m_PaletteSize; }
    unsigned char *MtxPalette() const { return m_Palette; }
    ex::u128 *WeightTable() const { return m_Weights; }
    int m_PaletteSize;                  // 0x00
    ex::EXRelPtr<u8> m_Palette;         // 0x04
    ex::EXRelPtr<ex::u128> m_Weights;   // 0x08
};
// Per entity GDi index, a relptr to its EXGeoAnimSkin_NewData.
struct EXGeoAnimSkin_Data {
    unsigned int *SkinData() const { return *(const ex::EXRelPtr<unsigned int> *)&m_00; }
    s32 m_00;
};
// One skinned entity of a skin (sizeof 0x14; the skin's entity groups, EXGeoBaseAnimSkin +0x68).
struct EXGeoAnimSkin_SkinList {
    EXGeoAnimSkin_Data *SkinData() const { return m_SkinData; }
    unsigned int SkinPartCount() const { return m_SkinPartCount; }
    unsigned int EntityIndex() const { return m_EntityIndex & 0xffffff; }
    int SectionIndex() const { return m_SectionIndex; }
    int MorphIndex() const { return m_MorphIndex; }
    ex::EXRelPtr<EXGeoAnimSkin_Data> m_SkinData;  // 0x00
    u32 m_SkinPartCount;                // 0x04 entities (the pieces of a split entity)
    s32 m_SectionIndex;                 // 0x08
    u32 m_EntityIndex;                  // 0x0c low 24 bits: index in the geo header's entity list
    s32 m_MorphIndex;                   // 0x10 morph set (EXGeoBaseAnimSkin::MorphList index), -1: none
};
// Bone weights of one vertex (sizeof 0x14): up to four palette entries (RemapVS2B), their weights.
struct XBWeightData {
    u8 m_Bone[4];       // 0x00
    float m_Weight[4];  // 0x04
};
// EXGDiSkinData as the Xbox entity code reads it (sizeof 0x14).
struct XBGDiSkinFields {
    XBSkinBlock *m_pSkinList;           // 0x00
    EXItemRender_SkinAnim *m_pRender;   // 0x04 owning skinned renderer (matrices, morph weights, skins)
    s32 *m_pEntitySkins;                // 0x08 relptr per entity GDi index to an XBEntitySkin
    u32 m_WeightLod;                    // 0x0c EXGeoAnimSkin::GetWeightVertexBuffer index
    u32 m_Skin;                         // 0x10 which of the renderer's skins (m_pSkins index)
};
// Bones one skinned entity uses: count, then a relptr to one bone index byte per palette entry.
struct XBEntitySkin {
    int m_BoneCount;                    // 0x00
    ex::EXRelPtr<u8> m_Bones;           // 0x04
};
// TODO stand-in: subdivision-surface mesh and its levels of detail (inlines emitted in XBGeoEntity.obj;
// built by XBSubdivisionSystem.obj). One level (sizeof 0x58): double-buffered streams picked by m_Current.
struct ExSubDivisionPacket {
    u32 m_Start;    // 0x00 first index
    u32 m_Count;    // 0x04 triangles
};
class ExSubDivisionLoD {
public:
    xb::D3DVertexBuffer *GetVertexStream() { return m_pVertices[m_Current]; }
    xb::D3DIndexBuffer *GetIndexStream() { return m_pIndices; }
    xb::D3DVertexBuffer *GetRGBAStream() { return m_pRGBA[m_Current]; }
    xb::D3DVertexBuffer *GetWeightsStream() { return m_pWeights[m_Current]; }
    // Triangle list of the strip i of the source entity.
    void Render(unsigned int i)
    {
        xb::D3DDevice::DrawIndexedPrimitive(xb::D3DPT_TRIANGLELIST, 0, 0, m_pPackets[i].m_Start, m_pPackets[i].m_Count);
    }
    xb::D3DVertexBuffer *m_pVertices[2];    // 0x00
    xb::D3DVertexBuffer *m_pRGBA[2];        // 0x08
    xb::D3DVertexBuffer *m_pWeights[2];     // 0x10
    xb::D3DIndexBuffer *m_pIndices;         // 0x18
    u8 m_1c[0x30 - 0x1c];
    ExSubDivisionPacket *m_pPackets;        // 0x30 one per strip
    u8 m_34[0x49 - 0x34];
    u8 m_Current;                           // 0x49
    u8 m_4a[0x54 - 0x4a];
    s8 m_Valid;                             // 0x54
    u8 m_55[3];
};
class ExSubDivisionMesh {
public:
    ExSubDivisionMesh()
    {
        m_00 = 0;
        m_08 = 0;
        m_04 = 0;
        m_LoDs.Init(0, 1);
    }
    ExSubDivisionLoD *GetLoD(unsigned int i)
    {
        if (i <= (unsigned int)m_LoDs.AllocSize() && m_LoDs.ElementAt(i).m_Valid) return &m_LoDs.ElementAt(i);
        return 0;
    }
    void Render(unsigned int lod, unsigned int i) { GetLoD(lod)->Render(i); }
    void BuidLoD(unsigned int lod, const EXGeoEntity *pEnt, const EXGDiSkinData *pSkin, const EXGDiData *pData,
                 const EXMatrix &m, signed char a, signed char b);
    ~ExSubDivisionMesh()
    {
        if (m_00) EXFree(m_00);
        if (m_08) EXFree(m_08);
        m_00 = 0;
        m_08 = 0;
        m_LoDs.Empty(1);
    }
    void *m_00;                                             // 0x00
    u32 m_04;
    void *m_08;                                             // 0x08
    EXDataArray<ExSubDivisionLoD, ExSubDivisionLoD &> m_LoDs;  // 0x0c
};

// EXGeoEntity fields from 0x58 as the Xbox code uses them (the class header keeps them as bytes).
struct XBGeoEntityFields {
    u8 m_00[0x58];
    ex::EXRelPtr<EXGeoEntity_TriStrip> m_TriStrip;  // 0x58
    ex::EXRelPtr<u8> m_Vtx;                         // 0x5c
    ex::EXRelPtr<u32> m_Rgba;                       // 0x60
    ex::EXRelPtr<EXGeoEntity_FaceInfo> m_FaceInfo;  // 0x64
    u32 m_68;                                       // 0x68 face collision relptr
    ex::EXRelPtr<u8> m_Index;                       // 0x6c
    void *m_70;                                     // 0x70 allocated (freed in OnDelete)
    u32 m_TriStripCount;                            // 0x74
    u32 m_VertexCount;                              // 0x78
    u32 m_7c;
    u32 m_80;
    u32 m_DeformerCount;                            // 0x84
    u16 m_XtraTypes;                                // 0x88
    u16 m_8a;                                       // 0x8a (0xff after OnCreate)
    xb::D3DVertexBuffer *m_pVB;                     // 0x8c
    xb::D3DVertexBuffer m_VB;                       // 0x90
    xb::D3DIndexBuffer *m_pIB;                      // 0x9c
    xb::D3DIndexBuffer m_IB;                        // 0xa0
    XBVertexBufferRef *m_pDefVB;                    // 0xac
    XBVertexBufferRef *m_pRGBAVB;                   // 0xb0
    ExSubDivisionMesh *m_pSubDiv;                   // 0xb4
    ex::EXRELOFFSET24x8 m_Xtra[1];                  // 0xb8 one per extra-data type, low byte = type
};
#endif
