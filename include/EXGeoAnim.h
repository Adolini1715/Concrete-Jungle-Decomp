#ifndef EXGEOANIM_H
#define EXGEOANIM_H
#include <string.h>
#include "EXGeoCommonObject.h"
#include "EXBounds.h"
#include "EXGeoEntity.h"
#include "EXBit128.h"
#include "EXRelPtr.h"
#include "EXStdFuncs.h"

class EXGeoFile;
class EXu32Array;
struct EXGeoRefObject;
struct EXMotionData;
// One animated datum of an anim (sizeof 0x18); the table is a relptr to the compressed data, then
// the datums (EXGeoAnimDatums).
struct EXGeoAnimDatum {
    u32 m_Id;               // +0x00
    u16 m_Flags;            // +0x04 -> EXDatumTemp::m_2c
    u8 m_Type;              // +0x06 shape type
    u8 m_07;
    float m_Size[3];        // +0x08
    ex::EXRelPtr<u8> m_FrameFlags;  // +0x14 per frame flag bits, or 0
};
// Per bone hierarchy data (sizeof 8).
struct EXGeoAnimSkin_HierData {
    u32 m_00;
    u16 m_Flags;    // +4 0x18: the bone starts a ragdoll body (EX6DAnim::RecurseInfoInit)
    u16 m_06;
};
struct XBSkinWeightVB;
namespace xb { struct D3DVertexBuffer; }

// One morph target set of a skeleton (sizeof 0x10; appended by the XBGeoEntity agent): its
// deformers are the entity's deformer vertex buffers, their weights start at DeformerOffset() in
// the renderer's morph weights.
struct EXGeoAnimSkin_MorphList {
    unsigned int DeformerOffset() const { return m_DeformerOffset; }
    unsigned int MorphType() const { return m_MorphType; }
    unsigned int DeformerCount() const { return m_DeformerCount; }
    // Type 1: u16 key count, then one deformer index per key (frame-blended morph).
    unsigned int *XtraData() const { return *(const ex::EXRelPtr<unsigned int> *)&m_0c; }
    u8 m_00[6];
    u16 m_DeformerCount;    // 0x06
    u16 m_DeformerOffset;   // 0x08
    u16 m_MorphType;        // 0x0a
    u32 m_0c;
};
// One bone's physics description in a skin (sizeof 0x90): the rigid body pose in skin space,
// its mass and contact coefficients and the bone-space box it occupies.
struct EXGeoAnimSkin_BonePhysicItem {
    EXMatrix m_mtx;             // +0x00
    u16 m_bone;                 // +0x40 skeleton bone index
    u16 m_42;                   // +0x42 TODO
    float m_mass;               // +0x44
    float m_friction;           // +0x48
    float m_restitution;        // +0x4c
    u8 m_50[0x68 - 0x50];       // +0x50 TODO
    EXVector3 m_min;            // +0x68
    EXVector3 m_max;            // +0x74
    u8 m_80[0x90 - 0x80];       // +0x80 TODO
};
struct EXGeoAnimDatums {
    u32 m_Count;
    EXGeoAnimDatum *m_Datums[1];    // +4 (address only is used)
};

// One decoded frame in an anim's sequence memory (header 0x50 bytes, then per-bone data).
struct EXGeoSkinAnimFrame {
    void Init()
    {
        m_Decoded = 0;
        m_MorphDecoded = 0;
        m_Frame = -1;
        memset(&m_Bones, 0, sizeof(m_Bones));
    }
    u8 m_Decoded;           // 0x00
    u8 m_MorphDecoded;      // 0x01
    s16 m_Frame;            // 0x02 -1: not decoded yet
    u8 m_04[0xc];
    ex::EXBit128 m_Bones;   // 0x10 bones decoded so far
    EXMaths::vec_xyzw m_Pos;    // 0x20 root motion: position (base position added)
    EXMaths::vec_xyzw m_Rot;    // 0x30 root motion: rotation
    EXMaths::vec_xyzw m_Root;   // 0x40 root bone rotation output of the decoder
};

// Where a stream starts and which 128-frame block the requested frame falls in (XBGeoAnim.obj).
class EXAnimStreamInfo {
public:
    int m_Frame0;           // 0x00 frame decoded (interpolated decodes: the first of the pair)
    int m_Frame1;           // 0x04 second frame of an interpolated decode
    int m_NumFrames0;       // 0x08
    void *m_pSections;      // 0x0c selective section data (extra data 5) or 0
    u32 m_StartFrame;       // 0x10 first frame of the stream
    int m_NumFrames;        // 0x14 (m_NumFrames - 1) >> 7 seek entries per channel
    int m_SeekFrame;        // 0x18 >> 7: seek entry to start from (0: scan from the start)
};

// Animation (sizeof 0x9c).
class EXGeoBaseAnim : public EXGeoCommonObject {
    EX_DECLARE_DYNAMIC(EXGeoBaseAnim)
    EX_DECLARE_GEOOBJECT(EXGeoBaseAnim)
public:
    virtual void OnCreate(EXGeoHeader *, EXGeoObjectHeader *);
    virtual void OnDelete(EXGeoHeader *, EXGeoObjectHeader *);
    virtual EXBoundsBox &GetBoundsBox();
    EXGeoBaseAnim() {}
    u8 *MotionData() const { return (u8 *)ex::EXRelOffset(m_Motion); }
    u8 *MotionCompressedData() const { return (u8 *)ex::EXRelOffset(m_Motion); }
    u8 *MorphCompressedData() const { return m_MorphData; }
    ex::EXBit128 SelectiveSectionFlags() const;
    void MorphGeoRefObject(EXGeoRefObject &ref) const;
    void SkinGeoRefObject(EXGeoRefObject &ref) const;
    unsigned int FrameFlags(int frame) const;
    void GetTotalMotion(EXMotionData &m) const;
    EXGeoSkinAnimFrame *DecodeMorph(int frame, unsigned int flags) const;
    void DecompressMorph(int frame, float *pOut) const;
    EXGeoSkinAnimFrame *DecodeFrame(int frame, unsigned int strip, ex::EXBit128 bones) const;
    void DecompressMotion(int frame, EXMotionData &m, unsigned char, unsigned int flags) const;
    void DecompressMotionInterp(float frame, EXMotionData &m, unsigned char, unsigned int flags) const;
    signed char DecompressDatum(int frame, int datum, EXMotionData &m, unsigned int flags) const;
    signed char DecompressDatumInterp(float frame, int datum, EXMotionData &m, unsigned int flags) const;
    void StripMotionData(const EXMotionData &strip, EXVector &pos, EXQuaternion &rot, unsigned int flags,
                         signed char apply) const;
    void DecompressMotion_StripMotion(EXMotionData &m, unsigned char, unsigned int flags) const;
    // Accessors appended by the item agent (EXItemCtrl_Anim.obj / EXItemAnimator_Anim.obj copies).
    unsigned int NumFrames() const { return m_NumFrames; }
    unsigned int AnimMotionFlags() const { return m_AnimMotionFlags; }
    unsigned int Flags() const { return m_Flags; }
    unsigned short FrameRate() const { return m_FrameRate; }
    unsigned short AnimIndex() const { return m_AnimIndex; }
    unsigned int MorphGeoHash() const { return m_MorphFileHash; }
    unsigned int MorphHashRef() const { return m_MorphHash; }
    void *XtraData(int t) const { return m_XtraData.GetXtra((unsigned char)t); }
    signed char HasXtraData(int t) const { return m_XtraData.HasXtraData((unsigned char)t); }
    unsigned int DatumCount() const { return m_DatumCount; }
    EXMotionData *Positioner() const { return (EXMotionData *)m_XtraData.GetXtra(1); }
    EXGeoAnimDatum *AnimDatum() const { return (EXGeoAnimDatum *)AnimDatumInfo()->m_Datums; }
private:
    EXGeoAnimDatums *AnimDatumInfo() const { return m_DatumData; }
public:
private:
    // XBGeoAnim.obj
    void DecompressMorphData(const unsigned char *pData, int numChannels, int frame, ex::EXBit128 &flags, float *pOut,
                             const EXAnimStreamInfo &info, signed char b) const;
    void DecompressMorphData(const unsigned char *pData, int numChannels, float frame, ex::EXBit128 &flags,
                             float *pOut, const EXAnimStreamInfo &info, signed char b) const;
    void DecompressData(const unsigned char *pData, int numChannels, int frame, ex::EXBit128 &a, ex::EXBit128 &b,
                        EXMaths::vec_xyzw *pOutA, EXMaths::vec_xyzw *pOutB, const EXAnimStreamInfo &info,
                        signed char c) const;
    void DecompressData(const unsigned char *pData, int numChannels, float frame, ex::EXBit128 &a, ex::EXBit128 &b,
                        EXMaths::vec_xyzw *pOutA, EXMaths::vec_xyzw *pOutB, const EXAnimStreamInfo &info,
                        signed char c) const;
    EXGeoSkinAnimFrame *ValidSeqMemoryPtr(const EXGeoHeader *pHdr, int frame) const;
    EXGeoSkinAnimFrame *AllocSeqMemory(const EXGeoHeader *pHdr, int frame) const;
    void DecompressFrame(int frame, EXGeoSkinAnimFrame *pFrame, ex::EXBit128 &bones) const;
    void DecompressMotion_StripMotion2(EXGeoSkinAnimFrame *pFrame, unsigned int flags) const;
public:
    static unsigned int m_gOnCreateAnimSection;

    ex::EXRELOFFSET32 m_Motion;         // 0x04 motion data (in the anim header's second address)
    u32 m_DataSize;                     // 0x08
    u8 m_FrameRate;                     // 0x0c
    u8 m_FramesPerSeq;                  // 0x0d frames per sequence-memory block
    u16 m_NumFrames;                    // 0x0e
    u16 m_AnimMotionFlags;              // 0x10
    u16 m_Flags;                        // 0x12
    u8 m_14;                            // 0x14 } bone/stream counts (frame stride uses
    u8 m_15;                            // 0x15 }  (m_14 + m_15) * 4 + m_16 dwords)
    u16 m_NumMorphs;                    // 0x16
    u16 m_DatumCount;                   // 0x18
    u16 m_AnimIndex;                    // 0x1a this anim's index in the anim list
    EXGeoFile *m_pGeoFile;              // 0x1c
    u32 m_SkinFileHash;                 // 0x20 -1: skin in this file
    u32 m_SkinHash;                     // 0x24
    u32 m_MorphFileHash;                // 0x28
    u32 m_MorphHash;                    // 0x2c
    ex::EXRelPtr<EXGeoAnimDatums> m_DatumData;      // 0x30
    ex::EXRelPtr<u8> m_MorphData;       // 0x34
    EXGeoXtraData m_XtraData;           // 0x38 (3: selective section flags, 4: frame flags)
    u32 m_3c;
    EXBoundsBox m_BoundsBox;            // 0x40
    ex::EXBit128 m_RootMask;            // 0x60 channels of the root stream decoded with each frame
    u32 m_TotalRot[4];                  // 0x70 total motion: rotation (x is copied as a dword)
    float m_TotalPos[3];                // 0x80 total motion: position
    float m_BasePos[3];                 // 0x8c added to the decoded root position
    EXGeoSkinAnimFrame *m_SeqMemory[1]; // 0x98 one block per m_FramesPerSeq frames (trailing)
};

class EXGeoAnim : public EXGeoBaseAnim {
    EX_DECLARE_DYNAMIC(EXGeoAnim)
    EX_DECLARE_GEOOBJECT(EXGeoAnim)
public:
    EXGeoAnim() {}
};

// Bone table entry of a skin (appended by the item agent): section index and bone hash.
struct EXGeoAnimSkin_BoneTbl {
    u16 m_index;    // +0
    u16 m_hash;     // +2 0xffff ends the table
};

// Entity used by a skin (sizeof 0x14); the hash's low 24 bits index the entity list.
struct EXGeoSkinEntityRef {
    u32 m_00[3];
    u32 m_EntityHash;   // 0x0c
    u32 m_10;
};

// Skeleton / skin (sizeof 0x8c).
class EXGeoBaseAnimSkin : public EXGeoCommonObject {
    EX_DECLARE_DYNAMIC(EXGeoBaseAnimSkin)
    EX_DECLARE_GEOOBJECT(EXGeoBaseAnimSkin)
public:
    virtual EXBoundsBox &GetBoundsBox();
    EXGeoBaseAnimSkin() {}
    // Appended by the XBGeoEntity agent ([fi] in EXItemRender_Anim.obj): relptr at 0x88.
    EXGeoAnimSkin_MorphList *MorphList() const { return *(const ex::EXRelPtr<EXGeoAnimSkin_MorphList> *)&m_80[8]; }
    int Value(int i) const;
    void GetTextureHashList(const EXGeoHeader *pHdr, EXu32Array &list) const;
    u8 m_04[8];
    ex::EXRelPtr24<s32> m_Values;   // 0x0c value table, low 8 bits: which values are present
    EXBoundsBox m_BoundsBox;    // 0x10
    u8 m_30[0x38];
    EXRelArray<EXGeoSkinEntityRef> m_EntityGroups[3];  // 0x68 laid out back to back
    u8 m_80[0xc];
    // Appended by the item agent (shapes from the EXItemRender_Anim.obj copies).
    // TODO layout: +4 bone count, +0x30 checksum (16 bytes), +0x58/+0x5c relptrs to the bone tables
    // (by index / by hash; {u16 index, u16 hash} entries ending in hash 0xffff), +0x80 deformer count.
    u32 BoneCount() const { return *(const u32 *)&m_04[0]; }
    u32 DefomerCount() const { return *(const u32 *)&m_80[0]; }
    const u32 *CheckSum() const { return (const u32 *)&m_30[0]; }
    int ValidateCheckSum(const EXGeoBaseAnimSkin *p) const { return EXmemcmp(&m_30[0], &p->m_30[0], 0x10); }
    u32 SkinCount() const { return m_EntityGroups[0].Size(); }
    u32 LinkedSkinCount() const { return m_EntityGroups[1].Size(); }
    u32 EntityCount() const { return m_EntityGroups[2].Size(); }
    u32 TotalSkinCount() const { return EntityCount() + LinkedSkinCount() + SkinCount(); }
    u32 MorphCount() const { return *(const u32 *)&m_80[4]; }
    EXGeoAnimSkin_BoneTbl *AnimBoneTbl() const { return *(const ex::EXRelPtr<EXGeoAnimSkin_BoneTbl> *)&m_30[0x28]; }
    EXGeoAnimSkin_BoneTbl *AnimBoneHashTbl() const { return *(const ex::EXRelPtr<EXGeoAnimSkin_BoneTbl> *)&m_30[0x2c]; }
    // Appended by the physics agent (shapes from the EXPhysics6D.obj copies). TODO layout: +0x44/+0x48
    // relptrs to the rest rotations / hierarchy, +0x50 bone physics count, +0x54 relptr to the items.
    EXVector *RotDataRel() const { return *(const ex::EXRelPtr<EXVector> *)&m_30[0x14]; }
    EXGeoAnimSkin_HierData *HierData() const { return *(const ex::EXRelPtr<EXGeoAnimSkin_HierData> *)&m_30[0x18]; }
    u32 BonePhysicsCount() const { return *(const u32 *)&m_30[0x20]; }
    EXGeoAnimSkin_BonePhysicItem *BonePhysics() const { return *(const ex::EXRelPtr<EXGeoAnimSkin_BonePhysicItem> *)&m_30[0x24]; }
};

// Platform skin (sizeof 0x94).
class EXGeoAnimSkin : public EXGeoBaseAnimSkin {
    EX_DECLARE_DYNAMIC(EXGeoAnimSkin)
    EX_DECLARE_GEOOBJECT(EXGeoAnimSkin)
public:
    virtual void OnCreate(EXGeoHeader *, EXGeoObjectHeader *);
    virtual void OnDelete(EXGeoHeader *, EXGeoObjectHeader *);
    EXGeoAnimSkin() {}
    xb::D3DVertexBuffer *GetWeightVertexBuffer(unsigned int i);  // XBGeoAnim.obj (appended by the XBGeoEntity agent)
    XBSkinWeightVB *m_pWeightVB;    // 0x8c one per skinned entity piece (OnCreate)
    u32 m_90;
};
#endif
