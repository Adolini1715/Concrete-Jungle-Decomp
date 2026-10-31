#ifndef EXGEOENTITY_H
#define EXGEOENTITY_H
#include "EXGeoCommonObject.h"
#include "EXBounds.h"
#include "EXRelPtr.h"
#include "EXMatrix.h"

class EXGeoFile;
class EXGeoEntity;
class EXCScoopTemp;
class EXGDiSkinData;
class EXGeoGrabPoint;
class EXInstanceEnv;
class EXVector;
struct EXRayCastTemp;
struct EXShadowTemp;
struct EXGeoEntityDatums;
struct EXGeoEntity_VtxData;
struct EXGeoEntity_FaceCollision;
struct EXGeoColSphere_Table;
struct EXGeoEntity_FaceNormals;     // one plane normal (16 bytes) per collision face
struct EXGeoEntity_IndexData;
struct EXGeoEntity_TriStrip;     // XBGeoEntity.h
struct EXGeoEntity_FaceInfo;
struct EXGeoAnimSkin_MorphList;
struct EXGeoTreeEntity_Node;
struct EXNodeListTemp;
class EXItemRender_SkinAnim;
class EXCachedGeoEntity;
namespace xb { struct D3DVertexBuffer; struct D3DIndexBuffer; }
// Texture indices (into the header's texture list) used by an entity.
struct EXGeoEntity_TextureList {
    u16 m_Count;
    u16 m_Textures[1];
};

// One renderable piece gathered by BuildEntityInfoArray (sizeof 0x50).
class EXGeoBaseEntity;
class EXMatrix;
struct EXGeoEntity_Info {
    void Set(EXGeoBaseEntity *pEnt, EXGDiData *pData, const EXMatrix &mtx);
    EXMatrix m_Matrix;          // 0x00 piece to world
    float m_Weight;             // 0x40 cumulative random-usage weight (CalcRandomUsageWeights)
    u16 m_VertexCount;          // 0x44
    u16 m_Usage;                // 0x46 random vertices to take from this piece (CalcRandomUsage)
    EXGeoEntity *m_pEntity;     // 0x48
    EXGDiData *m_pData;         // 0x4c
};

// Gathered entity pieces plus gather flags (sizeof 0x58; ctor lives in the entity code).
class EXGeoEntity_InfoArray : public EXDataArray<EXGeoEntity_Info, const EXGeoEntity_Info &> {
public:
    EXGeoEntity_InfoArray();
    ~EXGeoEntity_InfoArray() {}
    void SetFlags(unsigned int f) { m_Flags = f; }
    const EXMatrix &Matrix() const { return *(const EXMatrix *)m_10; }
    void SetMatrix(const EXMatrix &m) { *(EXMatrix *)m_10 = m; }
    unsigned int Flags() const { return m_Flags; }
    float CalcRandomUsageWeights();
    void CalcRandomUsage(int n);
    unsigned int GetRandomVertexArray(EXVector *pOut, int n, unsigned int flags);
    u8 m_10[0x40];
    u32 m_Flags;                // 0x50
    u32 m_54;                   // TODO: sizeof 0x58 from the stack frame of its users
};

// Map zone entity node (0x14).
struct EXGeoMapZoneEntity_Node {
    EXGeoMapZoneEntity_Node() {}
    u32 m_RefIndex;     // 0x00 index into the header's ref-pointer list (EXItemEnv.obj ProcessMapZones)
    float m_x, m_y, m_z;    // 0x04 sub-zone centre
    float m_Radius;     // 0x10
};

// Optional extra data: 8-bit type mask plus relptr (24 bits) to a table of EXRELOFFSET24x8, one
// per set bit in mask order.
class EXGeoXtraData {
public:
    unsigned short XtraTypes() const { return m_Value & 0xff; }
    signed char HasXtraData(unsigned char t) const { return (s8)((1 << t) & m_Value); }
    void *GetXtra(unsigned char t) const;
    s32 m_Value;
};

// Collision shapes of an entity (extra data type 5): a datum list 0xc bytes into the record.
// TODO names of the leading fields.
struct EXGeoBaseDatum;
struct EXGeoCollisionDatums {
    u32 NumDatums() const { return m_count; }
    const EXGeoBaseDatum *Datums() const { return m_datums; }
    u8 m_00;                                // +0x00 TODO
    u8 m_count;                             // +0x01
    ex::EXRelPtr16<EXGeoBaseDatum> m_datums;// +0x02 0x30 bytes each
};
struct EXGeoCollisionXtra {
    const EXGeoCollisionDatums *CollisionDatums() const { return &m_datums; }
    u8 m_00[0xc];                           // +0x00 TODO
    EXGeoCollisionDatums m_datums;          // +0x0c
};

// Bounding sphere (0x10).
struct EXBoundsSphere {
    float Radius() const { return m_r; }
    void SetCentre(float x, float y, float z) { m_x = x; m_y = y; m_z = z; }  // [fi] copy in Camera.obj
    void SetRadius(float r) { m_r = r; }
    float m_x, m_y, m_z, m_r;
};

// Renderable mesh base (sizeof 0x54).
class EXGeoBaseEntity : public EXGeoCommonObject {
    EX_DECLARE_DYNAMIC(EXGeoBaseEntity)
    EX_DECLARE_GEOOBJECT(EXGeoBaseEntity)
    friend class EXGeoSplitEntity;
public:
    virtual void OnCreate(EXGeoHeader *, EXGeoObjectHeader *);
    virtual void OnDelete(EXGeoHeader *, EXGeoObjectHeader *);
    virtual void Draw(EXGDiData *, unsigned int);
    virtual EXBoundsBox &GetBoundsBox();
    virtual void *GetVertexBuffer(EXGDiData *, signed char);
    virtual void ReleaseVertexBuffer(EXGDiData *);
    virtual void AddCachedDraw(EXGDiData *, unsigned int);
    virtual unsigned int ApplyFunction(unsigned int (__cdecl *)(EXGeoBaseEntity *, EXGDiData *), EXGDiData *);
    virtual unsigned int DoRayCast(EXRayCastTemp &, EXGDiData *, unsigned int);
    virtual unsigned int DoShadowCast(EXShadowTemp &);
    virtual unsigned int DoEntityScoop_Sphere(EXCScoopTemp &, EXGDiData *, unsigned int, unsigned int);
    virtual unsigned int DoEntityScoop_Shape(EXCScoopTemp &, EXGDiData *, unsigned int, unsigned int);
    virtual unsigned int GetVertexCount() const;
    virtual void BuildEntityInfoArray(EXGeoEntity_InfoArray &, EXGDiData *);
protected:
    virtual unsigned short InitialiseGDiCount(unsigned short &);
public:
    EXGeoBaseEntity() {}
    unsigned int Flags() const { return m_Flags; }
    int SortValue() const { return m_SortValue; }
    unsigned char RenderOrder() const { return m_RenderOrder; }
    signed char HasCollision() const { return (s8)((m_Flags >> 8) & 1); }
    const EXBoundsBox &BoundsBox() const { return m_BoundsBox; }
    const EXBoundsSphere &BoundsSphere() const { return m_BoundsSphere; }
    EXGeoFile *GeoFile() const { return m_pGeoFile; }
    unsigned int GDiIndex() const { return m_GDiIndex; }
    float SurfaceArea() const { return m_SurfaceArea; }
    // appended by the collision agent ([fi] copies in EXCollision.obj)
    signed char HasBaseXtraData(unsigned char t) const { return m_XtraData.HasXtraData(t); }
    signed char HasCollisionEntity(int i) const { return HasBaseXtraData((unsigned char)i); }
    EXGeoBaseEntity *Collision(int i) const { return (EXGeoBaseEntity *)m_XtraData.GetXtra((unsigned char)i); }
    // appended by the item agent ([fi] copy in EXItemRender_Anim.obj; CalcGDiCount in EXGeoEntity.obj)
    unsigned short GDiCount()
    {
        if (m_GDiCount) return m_GDiCount;
        return CalcGDiCount();
    }
protected:
    unsigned short CalcGDiCount();
public:
    unsigned int GetFaceCount();
    int Value(int i) const;
protected:
    static EXGDiSkinData *ms_pGDiSkinData;  // appended by the XBGeoEntity agent
    friend class EXItemRender_SkinAnim;     // RenderSkin sets the skin data for each piece
public:

    // Entity flags as the Xbox renderer reads them: 1 texture remap (tex swap; heat render needs it),
    // 2 lit by the item's directional lights, 8 always write Z, 0x10 unfogged, 0x400 never drawn,
    // 0x800 no Z write, 0x1000 no Z test, 0x2000 CPU-written vertices, 0x4000 receives shadows,
    // 0x8000 drawn into the light map, 0x10000 plain (no texture alpha, no density tint) in the shadow
    // pass, 0x80000 drawn in the shadow id pass (EXWnd::RenderSortedShadowArray),
    // 0x10000000 subdivision surface, 0x40000000 casts no shadow, 0xc0000000 alpha blended,
    // 0x20000000 sorted with the alpha entities.
    u32 m_Flags;                    // 0x04
    s16 m_SortValue;                // 0x08 alpha sort bucket: < 0 last, 0 by depth, > 0 first
    u8 m_RenderOrder;               // 0x0a 1/3: drawn first, 2/4: drawn last (EXWnd::SortAndRenderEnitityArray)
    u8 m_0b;
    float m_SurfaceArea;            // 0x0c
    EXBoundsBox m_BoundsBox;        // 0x10
    EXBoundsSphere m_BoundsSphere;  // 0x30
    EXGeoXtraData m_XtraData;       // 0x40
    struct EXGeoColSphere_Table *CollisionSpheres() const { return *(const ex::EXRelPtr<EXGeoColSphere_Table> *)&m_44; }  // [fi] XBCollision.obj
    u32 m_44;
    u32 m_48;
    EXGeoFile *m_pGeoFile;          // 0x4c
    u16 m_GDiCount;                 // 0x50
    u16 m_GDiIndex;                 // 0x52
};

// ---- appended by the physics agent: vertex as the physics code sees it (stride 0x18) ----
#ifndef EXGEOENTITY_VTXDATA_DEFINED
#define EXGEOENTITY_VTXDATA_DEFINED
struct EXGeoEntity_VtxData {
    float m_x, m_y, m_z;    // 0x00 position
    s32 m_Normal;           // 0x0c packed normal (SetVertexNormal)
    u32 m_10;               // TODO
    u32 m_14;               // TODO
};
#endif

// Single mesh (sizeof 0xbc).
class EXGeoEntity : public EXGeoBaseEntity {
    EX_DECLARE_DYNAMIC(EXGeoEntity)
    EX_DECLARE_GEOOBJECT(EXGeoEntity)
public:
    virtual void OnCreate(EXGeoHeader *, EXGeoObjectHeader *);
    virtual void OnDelete(EXGeoHeader *, EXGeoObjectHeader *);
    virtual void Draw(EXGDiData *, unsigned int);
    virtual void *GetVertexBuffer(EXGDiData *, signed char);
    virtual void ReleaseVertexBuffer(EXGDiData *);
    virtual unsigned int DoRayCast(EXRayCastTemp &, EXGDiData *, unsigned int);
    virtual unsigned int DoShadowCast(EXShadowTemp &);
    virtual unsigned int GetVertexCount() const;
    virtual void BuildEntityInfoArray(EXGeoEntity_InfoArray &, EXGDiData *);
    EXGeoEntity() {}
    static void ReleasePhysicalVertexBuffer(unsigned int *);
    EXGeoEntity_TextureList *TextureList() const { return m_TextureList; }

    ex::EXRelPtr<EXGeoEntity_TextureList> m_TextureList;  // 0x54 (EXGeoAnim.obj copy of TextureList())
    u8 m_58[0xbc - 0x58];
    // ---- appended by the physics agent (EXPhysicsSoft.obj / EXPhysicsFluid.obj copies) ----
    // Extra data mask (16 bits at +0x88) and lookup (XtraData lives in the geo entity code).
    void *XtraData(unsigned char t) const;
    signed char HasXtraData(unsigned char t) const { return (s8)(*(const u16 *)&m_58[0x88 - 0x58] & (1 << t)); }
    // ---- appended by the collision agent (EXCollision.obj / XBCollision.obj copies): relptrs ----
    EXGeoEntity_VtxData *Vtx() const { return *(const ex::EXRelPtr<EXGeoEntity_VtxData> *)&m_58[0x5c - 0x58]; }
    unsigned int *Rgba() const { return *(const ex::EXRelPtr<unsigned int> *)&m_58[0x60 - 0x58]; }
    EXGeoEntity_FaceCollision *FaceCollision() const { return *(const ex::EXRelPtr<EXGeoEntity_FaceCollision> *)&m_58[0x68 - 0x58]; }
    EXGeoEntity_IndexData *Index() const { return *(const ex::EXRelPtr<EXGeoEntity_IndexData> *)&m_58[0x6c - 0x58]; }
    // Normal packed 11:11:10 (x, y signed *1023 in the low 22 bits, z *511 in the top 10).
    // Signed 11:11:10 fields scaled by 1023, 1023 and 511.
    EXVector3 GetVertexNormal(const EXGeoEntity_VtxData &v) const
    {
        s32 n = v.m_Normal;
        s32 x = (n << 21) >> 21;
        s32 y = (n << 10) >> 21;
        s32 z = n >> 22;
        return EXVector3((float)x * (1.0f / 1023.0f), (float)y * (1.0f / 1023.0f), (float)z * (1.0f / 511.0f));
    }
    void SetVertexNormal(EXGeoEntity_VtxData &v, const EXVector3 &n)
    {
        *(u32 *)&v.m_Normal = ((((int)(n.z * 511.0f) << 11) | ((int)(n.y * 1023.0f) & 0x7ff)) << 11) | ((int)(n.x * 1023.0f) & 0x7ff);
    }
    // ---- appended by the XBGeoEntity agent (XBGeoEntity.obj; field layout in XBGeoEntity.h) ----
    unsigned int TriStripCount() const { return *(const u32 *)&m_58[0x74 - 0x58]; }
    unsigned int VertexCount() const { return *(const u32 *)&m_58[0x78 - 0x58]; }
    // Shadow object id of the entity this frame (EXWnd::RenderSortedShadowArray; the shadow shader's key).
    unsigned short &ShadowObjectId() { return *(u16 *)&m_58[0x8a - 0x58]; }
    EXGeoEntity_TriStrip *TriStrip() const { return *(const ex::EXRelPtr<EXGeoEntity_TriStrip> *)&m_58[0x58 - 0x58]; }
    EXGeoEntity_FaceInfo *FaceInfo() const { return *(const ex::EXRelPtr<EXGeoEntity_FaceInfo> *)&m_58[0x64 - 0x58]; }
    void *GetPhysicalVertexBuffer();
    void RegisterPhysVertexBuffer(void *, void *);
    void RegisterPhysIndexBuffer(unsigned int, void *);
    void ReleasePhysicalIndexBuffer();
    void SetShadowVertexShader(EXGDiData *);
    void SetRefractionVertexShader(EXGDiData *);
    void SetHeatVertexShader(EXGDiData *);
    void XBSetEntityDynamicLighting();
    void DrawDirectXShadow(EXCachedGeoEntity *, EXGeoHeader *, EXGDiData *);
    unsigned int FaceCount() const;
    void *CreatePhysicalVertexBuffer(void *, void *, unsigned int *);
    void *CreatePhysicalVertexBuffer(int);
    xb::D3DVertexBuffer *GetVertexBuffer();
    void ReleasePhysicalVertexBuffer();
    void CreatePhysicalIndexBuffer();
    void BuildTextureRemap(EXCachedGeoEntity *);
    void CreateRGBAVertexBuffers();
    void CreateDeformerVertexBuffers();
    void CreateDeformerVertexSetBuffers();
    xb::D3DVertexBuffer *GetDefVertexBuffer(unsigned int);
    xb::D3DVertexBuffer *GetXYZVertexBuffer(unsigned int);
    xb::D3DVertexBuffer *GetRGBAVertexBuffer(unsigned int);
    xb::D3DIndexBuffer *GetIndexBuffer();
    void ReleaseRGBAVertexBuffers();
    void ReleaseDeformerVertexBuffers();
    void DrawDirectX(EXCachedGeoEntity *);
    // DrawDirectX stages (header inlines in XBGeoEntity.obj).
    void SetUpLighting(EXCachedGeoEntity *, unsigned int &);
    void SetUpShadows(EXCachedGeoEntity *);
    void SetUpSpecialFX(EXCachedGeoEntity *, signed char &);
    signed char SetUpVertexStreams(EXCachedGeoEntity *, unsigned int &, unsigned int &);
    void CalculateVSSkin(EXGeoHeader *, EXGDiData *);
    EXGeoEntity_FaceNormals *GetFaceNormalsBuffer(EXGDiData *);
    EXGeoEntity_VtxData *ApplySkinMorph(EXGeoAnimSkin_MorphList *, EXItemRender_SkinAnim *, void *);
    unsigned int DegenerateCount() const { return *(const u32 *)&m_58[0x7c - 0x58]; }
    void Create(EXGeoHeader *pHdr, EXGeoObjectHeader *pObj)
    {
        _EXGeoCommonObjectDef::Create(this, *(u32 *)this);
        OnCreate(pHdr, pObj);
    }
    unsigned int GetRandomVertexArray(EXGeoEntity_Info *pInfo, EXVector *pOut, int n, unsigned int flags);
};

// Mesh split into a node tree (sizeof 0x60).
class EXGeoSplitEntity : public EXGeoBaseEntity {
    EX_DECLARE_DYNAMIC(EXGeoSplitEntity)
    EX_DECLARE_GEOOBJECT(EXGeoSplitEntity)
public:
    virtual void OnCreate(EXGeoHeader *, EXGeoObjectHeader *);
    virtual void OnDelete(EXGeoHeader *, EXGeoObjectHeader *);
    virtual void Draw(EXGDiData *, unsigned int);
    virtual unsigned int ApplyFunction(unsigned int (__cdecl *)(EXGeoBaseEntity *, EXGDiData *), EXGDiData *);
    virtual unsigned int DoRayCast(EXRayCastTemp &, EXGDiData *, unsigned int);
    virtual unsigned int DoShadowCast(EXShadowTemp &);
    virtual unsigned int DoEntityScoop_Sphere(EXCScoopTemp &, EXGDiData *, unsigned int, unsigned int);
    virtual unsigned int DoEntityScoop_Shape(EXCScoopTemp &, EXGDiData *, unsigned int, unsigned int);
    virtual unsigned int GetVertexCount() const;
    virtual void BuildEntityInfoArray(EXGeoEntity_InfoArray &, EXGDiData *);
protected:
    virtual void AddCachedDraw(EXGDiData *, unsigned int);
    virtual unsigned short InitialiseGDiCount(unsigned short &);
public:
    EXGeoSplitEntity() {}
    void Create(EXGeoHeader *pHdr, EXGeoObjectHeader *pObj)
    {
        _EXGeoCommonObjectDef::Create(this, *(u32 *)this);
        OnCreate(pHdr, pObj);
    }
    EXGeoBaseEntity *Entity(unsigned int i) const { return ((ex::EXRelPtr<EXGeoBaseEntity> *)&m_54[8])[i]; }
    EXGeoTreeEntity_Node *Tree() const { return *(const ex::EXRelPtr<EXGeoTreeEntity_Node> *)&m_54[4]; }
private:
    void TreeDraw(const EXGeoTreeEntity_Node *pNode, EXGDiData *pData, signed char cull, unsigned int flags);
    void OnCreateNodes(EXGeoTreeEntity_Node *pNode, EXGeoHeader *pHdr, EXGeoObjectHeader *pObj);
    void DoShadowCast_GetNodeList(EXGeoTreeEntity_Node *pNode, EXShadowTemp &t, EXNodeListTemp *pList);
public:
    u8 m_54[0xc];
    // ---- appended by the physics agent (EXPhysicsSoft.obj copy): piece count at +0x54 ----
    unsigned int EntityCount() const { return *(const u32 *)m_54; }
};

// Instanced mesh (sizeof 0x54).
class EXGeoInstanceEntity : public EXGeoBaseEntity {
    EX_DECLARE_DYNAMIC(EXGeoInstanceEntity)
    EX_DECLARE_GEOOBJECT(EXGeoInstanceEntity)
public:
    EXGeoInstanceEntity() {}
    void Render(EXInstanceEnv *) {}
};

// Map zone collision/render split (sizeof 0x2d8).
class EXGeoMapZoneEntity : public EXGeoBaseEntity {
    EX_DECLARE_DYNAMIC(EXGeoMapZoneEntity)
    EX_DECLARE_GEOOBJECT(EXGeoMapZoneEntity)
public:
    virtual unsigned int DoRayCast(EXRayCastTemp &, EXGDiData *, unsigned int);
    virtual unsigned int DoShadowCast(EXShadowTemp &);
    virtual unsigned int DoEntityScoop_Sphere(EXCScoopTemp &, EXGDiData *, unsigned int, unsigned int);
    virtual unsigned int DoEntityScoop_Shape(EXCScoopTemp &, EXGDiData *, unsigned int, unsigned int);
    virtual void OnDelete(EXGeoHeader *);
    EXGeoMapZoneEntity() {}
    unsigned int NumSubZones() const { return m_NumSubZones; }
    unsigned int ZonesInSubFiles() const { return m_ZonesInSubFiles; }
    EXGeoMapZoneEntity_Node *SubZoneEntity(int i) { return &m_Nodes[i]; }

    u8 m_NumSubZones;       // 0x54
    u8 m_ZonesInSubFiles;   // 0x55
    u16 m_56;
    EXGeoMapZoneEntity_Node m_Nodes[32];  // 0x58

};

// Nav mesh entity base (collision area).
#include "EXGeoNavMeshBaseEntity.h"

class EXGeoNavMeshEntity : public EXGeoNavMeshBaseEntity {
    EX_DECLARE_DYNAMIC(EXGeoNavMeshEntity)
    EX_DECLARE_GEOOBJECT(EXGeoNavMeshEntity)
public:
    EXGeoNavMeshEntity() {}
};
#endif
