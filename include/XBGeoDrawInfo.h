#ifndef XBGEODRAWINFO_H
#define XBGEODRAWINFO_H
#include "types.h"


// Anything owning D3D memory; registers itself with the display (sizeof 0xc).
class EXD3DResource {
public:
    EXD3DResource();
    virtual ~EXD3DResource();
    virtual void ReleaseD3DResource() = 0;
    void *m_pNext;          // 0x04 display resource list
    void *m_pPrev;          // 0x08
};

// Per-entity draw data (sizeof 0x68).
class EXGDiData : public EXD3DResource {
public:
    virtual ~EXGDiData() {}
    virtual void ReleaseD3DResource();
    EXGDiData();
    void Init();
    void Release();
    signed char HasModifiedVerts() { return m_Flags & 1; }  // [fi] copy in EXCollision.obj
    void Clear()
    {
        m_64 = 0;
        m_Flags = 0;
        m_IndexBuffer = 0;
        m_VertexBuffer = 0;
        m_pTexRemap = 0;
        m_54 = 0;
        m_58 = 0;
        m_5c = 0;
        m_1c = 0;
        m_20 = 0;
        m_24 = 0;
        m_CurrentVB = 0;
    }
    // m_Flags: 1 vertices rewritten this frame (CPU skinned/deformed: draw from m_CurrentVB),
    // 4 vertex buffer locked, 8 skinned (EXCachedGeoEntity::m_pSkinData applies),
    // 0x80 alpha op MODULATE2X, 0x100 the three dynamic vertex buffers exist.
    u32 m_Flags;            // 0x0c
    u32 m_IndexBuffer;      // 0x10 xb::D3DIndexBuffer * bound by DrawDirectX
    u32 m_VertexBuffer;     // 0x14 xb::D3DVertexBuffer * of the static vertices
    void *m_pTexRemap;      // 0x18 change number, then one EXTexture * per entity texture (BuildTextureRemap)
    unsigned int *m_1c;     // 0x1c dynamic vertex buffers (triple buffered, headers at 0x30/0x3c/0x48)
    unsigned int *m_20;
    unsigned int *m_24;
    u32 m_CurrentVB;        // 0x28 the dynamic buffer written this frame (stream 0)
    u8 m_2c[0x54 - 0x2c];   // 0x2c next buffer, then the three buffer headers
    void *m_54;             // 0x54 allocated vertex copies
    void *m_58;
    void *m_5c;
    u32 m_CurrentIndex;     // 0x60 which of the three buffers is next (0..2)
    u32 m_64;
};

#ifndef EXGDIDATAARRAY_DEFINED
#define EXGDIDATAARRAY_DEFINED
class EXGDiDataArray {
public:
    EXGDiDataArray() { m_pData = 0; m_Size = 0; }
    ~EXGDiDataArray();
    EXGDiData *Data() const { return m_pData; }
    int Size() const { return m_Size; }
    void Init(int n);
    void RemoveAll();
    EXGDiData *m_pData;
    int m_Size;
};
#endif

class EXGDiSkinData {
public:
    void Release();
    u8 m_00[0x14];
};

class EXGDiSkinDataArray {
public:
    EXGDiSkinDataArray() { m_pData = 0; m_pSkinData = 0; m_Size = 0; }
    ~EXGDiSkinDataArray();
    EXGDiSkinData *SkinData() { return m_pSkinData; }
    EXGDiData *Data() const { return m_pData; }
    int Size() const { return m_Size; }
    void Init(int n);
    void RemoveAll();
    EXGDiData *m_pData;             // 0x00
    EXGDiSkinData *m_pSkinData;     // 0x04
    int m_Size;                     // 0x08
};

// Instance stream buffer (sizeof 0x18).
class EXGDiInstanceData : public EXD3DResource {
public:
    virtual ~EXGDiInstanceData();
    virtual void ReleaseD3DResource();
    EXGDiInstanceData();
    void Release();
    void *m_pResource;      // 0x0c D3D resource
    u32 m_10;
    u32 m_14;
};
#endif
