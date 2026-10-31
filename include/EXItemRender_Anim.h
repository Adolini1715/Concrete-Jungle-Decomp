#ifndef EXITEMRENDER_ANIM_H
#define EXITEMRENDER_ANIM_H
#include "EXItemRender.h"
#include "EXGeoComplexRefObject.h"
#include "EXRenderInfo.h"
#include "EXArray.h"
#include "EXBit128.h"
#include "EXVector.h"
#include "EXQuaternion.h"
#include "EXStdFuncs.h"
#include "XBGeoDrawInfo.h"
#include "EXFlag.h"
#include "EXList.h"

class EXGeoFile;
class EXGeoHeader;
class EXGeoAnimSkin;
class EXGeoAnimSkinHeader;
class EXGeoBaseEntity;
class EXGDiData;
class EXMatrix;
class EXCollisionScoopTemp;
class EXGeoEntity_InfoArray;
class EXDatumTempArray;
struct EXDatumTemp;
struct EXRotPosInfo;
// Skeleton datum (datum shape, then the bone it hangs off and the bone chain from the root down to
// it). TODO layout of the leading datum part.
struct EXGeoAnimSkelDatum {
    u8 m_00[0x30];
    u8 m_Bone;          // +0x30
    u8 m_NumChain;      // +0x31
    u8 m_Chain[1];      // +0x32 root first; the last entry is m_Bone's section
};
struct EXItemRender_SkinMorph;
struct EXGeoSkinAnimFrame;
class EXGeoBaseAnim;
// Decoded frames handed from an animation controller to the skin's hierarchy builders
// (EXItemCtrl_Anim::UpdateCtrlOnStack): up to two motions (frame depth blend), each with the frame
// pair to interpolate between and the matching morph frames, and the skin the anim was made for.
struct EXItemRender_SkinAnimInfo {
    EXGeoSkinAnimFrame *m_pFrame[2][2];         // +0x00 [motion][frame, next frame]
    EXGeoBaseAnim *m_pAnim[2][2];               // +0x10
    EXGeoSkinAnimFrame *m_pMorphFrame[2][2];    // +0x20
    EXGeoBaseAnim *m_pMorphAnim[2][2];          // +0x30
    float m_frac[2];                            // +0x40 position between the frame pair
    float m_blend;                              // +0x48 controller blend
    float m_depth;                              // +0x4c blend between the two motions
    EXGeoComplexRefObject m_animSkin;           // +0x50 the anim's own skin, when not the item's
    const EXGeoComplexRefObject *m_pAnimSkin;   // +0x60 skin the anim's rest pose comes from
};
class EXItemRender_SkinAnimSection;
// Stack entry for the iterative hierarchy walk of CalcSectionMatrix.
struct EXItemRender_SkinAnimSectionTemp {
    EXItemRender_SkinAnimSection *m_pSec;
};

// TODO layout: plain geo reference {file, hash} (EXGeoFile area); sizeof 8.
struct EXGeoBaseRefObject {
    // Debug names (EXGeoFile.obj; appended by the geometry-layer agent).
    const char *DebugRefName() const;
    const char *DebugGeoName() const;
    EXGeoFile *m_pGeoFile;  // +0
    u32 m_hashRef;          // +4
};
struct EXGeoRefObject : public EXGeoBaseRefObject {
    EXGeoRefObject() {}
    EXGeoRefObject(EXGeoFile *pFile, u32 hash) { m_pGeoFile = pFile; m_hashRef = hash; }
    void SetHashRef(u32 hash) { m_hashRef = hash; }
    void Set(EXGeoFile *pFile, u32 hash)
    {
        m_pGeoFile = pFile;
        m_hashRef = hash;
    }
};

// Level-of-detail table entry (sizeof 0x18): switch distance, detail flags, skins used.
struct EXAnimLodDetails {
    float m_dist;               // +0
    u8 m_flags;                 // +4
    u8 m_05[3];
    ex::EXBit128 m_skinBits;    // +8 skin parts drawn at this lod
};

// Skeleton section (sizeof 0x48): a list item in its parent's child list (top-level sections in
// EXItemRender_SkinAnim::m_sections).
class EXItemRender_SkinAnimSection : public EXDListItem {
public:
    const EXVector &Position() const { return m_pos; }
    const EXQuaternion &Rotation() const { return m_rot; }
    u32 SectionIndex() const { return m_bone; }
    u32 ParentIndex() const { return m_parent; }
    EXItemRender_SkinAnimSection *Next() const { return (EXItemRender_SkinAnimSection *)m_pNext; }
    EXItemRender_SkinAnimSection *ChildLinks() const { return (EXItemRender_SkinAnimSection *)m_children.m_pHead; }
    // Copied through a POD temporary: retail loads all four dwords before storing.
    void SetPosition(const EXVector &v)
    {
        EXMaths::vec_xyzw t = (const EXMaths::vec_xyzw &)v;
        (EXMaths::vec_xyzw &)m_pos = t;
    }
    void SetRotation(const EXQuaternion &q) { m_rot = q; }
    void Initialise();
    u16 m_bone;                             // +8
    u16 m_parent;                           // +0xa 0xffff: root
    EXMatrix *m_pMtx;                       // +0xc
    EXDList m_children;                     // +0x10
    EXVector m_pos;                         // +0x18
    EXQuaternion m_rot;                     // +0x28
    ex::EXBit128 m_bits;                    // +0x38 this section and its skin parts
};

// One drawn skin of a skinned renderer (sizeof 0x58).
struct EXItemRender_SkinInfo {
    EXItemRender_SkinInfo();
    ~EXItemRender_SkinInfo() {}
    void Initialise();
    void RemoveSkin();
    ex::EXBit128 m_00;
    ex::EXBit128 m_usedBits;            // +0x10 skin parts present
    EXGDiSkinDataArray m_gdi;           // +0x20
    EXGeoComplexRefObject m_skinRef;    // +0x2c lod skin
    EXGeoComplexRefObject m_drawSkinRef;// +0x3c
    u8 *m_pRemap;                       // +0x4c section remap table
    u32 m_frame;                        // +0x50 g_Stats frame of the last update
    u8 m_54;
};

// Skinned-animation renderer (sizeof 0x150).
class EXItemRender_SkinAnim : public EXItemRender {
    EX_DECLARE_DYNAMIC(EXItemRender_SkinAnim)
public:
    virtual ~EXItemRender_SkinAnim();
    EXRenderInfo &RenderInfo() { return m_renderInfo; }
    const EXAnimLodDetails *ActiveLodDetails() const { return m_pActiveLod; }
    const EXGeoComplexRefObject &SkelRefObject() const { return m_skelRef; }
    const EXGeoComplexRefObject &SkinRefObject() const { return m_refs[3]; }
    const EXGeoComplexRefObject &ShadowRefObject() const { return m_refs[1]; }
    const EXGeoComplexRefObject &ReflectionRefObject() const { return m_refs[2]; }
    int SectionCount() const { return m_numBones; }
    int DefomerCount() const { return m_numMorphs; }
    void SetActiveLodDetails(EXAnimLodDetails *p) { m_pActiveLod = p; }
    EXItemRender_SkinAnimSection *SectionArray() const { return m_pBones; }
    EXItemRender_SkinMorph *MorphDeformers() const { return m_pMorphs; }
    EXMatrix *MtxArray() const { return m_pMtx; }
    const EXGeoComplexRefObject &DrawSkinRefObject(int i) const { return m_pSkins[i]->m_drawSkinRef; }
    u8 *RemapTable() const;
    ex::EXBit128 CalculateSkinUsageBits(u32 flags) const;
    EXItemRender_SkinAnimSection *GetSectionParent(const EXItemRender_SkinAnimSection *p) const;
    void DeleteCloneSkin(EXItemRender_SkinAnim *p, s8 b);
    EXItemRender_SkinAnim *FindCloneSkin(const EXGeoRefObject &r) const;
    void SetLodMin(u32 lod);
    void SetLodMax(u32 lod);
    void BuildHierData_ModifyMorph(const EXRotPosInfo *pInfo, float f);
    void InitBlendStack(const EXRotPosInfo *pInfo, s8 b);
    void SetSkinRefObject(const EXGeoBaseRefObject &r);
    void SetSkinRefObject(EXGeoFile *pFile, u32 hash);
    void SetSkinRefObject(u32 hash);
    void SetShadowRefObject(const EXGeoBaseRefObject &r);
    void SetCollisionRefObject(const EXGeoBaseRefObject &r);
    void SetReflectionRefObject(const EXGeoBaseRefObject &r);
    void Invalidate();
    void SetUpdateSkin();
    void SetUpdateNoRender();
    void CalcChildMatrix(EXItemRender_SkinAnimSection *pSec, EXMatrix *pMtx);
    EXItemRender_SkinAnimSection *GetAnimBoneSection(u32 hash, u32 flags) const;
    EXItemRender_SkinAnimSection *GetAnimBoneSection(const EXVector &p, u32 flags) const;
    s8 Create(const EXGeoRefObject &r);
    void ApplyFunction(u32 (__cdecl *pFunc)(EXGeoBaseEntity *, EXGDiData *));
    EXGeoBaseEntity *FindAttachedEntity(u32 hash, EXGDiData *&pData, s8 b);
    void SetRenderState(u32 state, s8 bOn);
    void RenderSkin(const EXMatrix &mtx);
    void BuildHierData_Morph(const EXRotPosInfo *pInfo, EXItemRender_SkinAnimInfo *pAnim, const EXGeoAnimSkin *pSkin, s8 b);
    void BuildHierData2_Morph(const EXRotPosInfo *pInfo, EXItemRender_SkinAnimInfo *pAnim);
    void UpdateRenderEnd();
    void GetDatum_SkelDatum(u32 type, EXDatumTempArray &arr, u32 flags);
    void *GetDatum_SkelDatum(u32 type, EXDatumTemp &d, void *&pCtx, u32 flags);
    void *GetDatum_AnimBoneDatum(u32 type, EXDatumTemp &d, u32 flags) const;
    void *GetDatum_AnimBoneDatum(const EXVector &p, EXDatumTemp &d, u32 flags) const;
    void BuildEntityInfoArray(const EXMatrix &mtx, EXGeoEntity_InfoArray &arr) const;
    s8 BuildHierData_Motion(const EXRotPosInfo *pInfo, EXItemRender_SkinAnimInfo *pAnim, ex::EXBit128 bits);
    s8 BuildHierData2_Motion(const EXRotPosInfo *pInfo, EXItemRender_SkinAnimInfo *pAnim, ex::EXBit128 bits);
    const EXGeoAnimSkinHeader *UpdateSkinInformation(int n, const EXVector &p);
    void ListSections(EXPtrArray<void> &arr, EXItemRender_SkinAnimSection *pSec) const;
    void DoEntityScoop(const EXMatrix &mtx, EXCollisionScoopTemp &t, u32 flags);
    EXItemRender_SkinAnim();
    void DoRender(const EXMatrix &mtx, const EXRenderInfo &info);
    EXItemRender_SkinAnim *AddCloneSkin(const EXGeoRefObject &r);
    static EXItemRender_SkinAnim *CreateObject();
    EXMatrix *TempMtxArray(int i) const;
private:
    void RemoveSkin(int i) const;
    EXMatrix GetDatum_CalcMatrixSP(const EXGeoAnimSkelDatum *pDatum, u32 n, EXMatrix *pMtx, const ex::EXBit128 *pBits) const;
    EXMatrix GetDatum_CalcMatrix(const EXGeoAnimSkelDatum *pDatum, u32 n);
    void UpdateSkinTables();
    void CalcSectionMatrix(EXItemRender_SkinAnimSection *pSec, EXMatrix *pMtx, EXItemRender_SkinAnimSectionTemp *pTemp);
    void BuildRemapTable(const EXGeoAnimSkin *pA, const EXGeoAnimSkin *pB, u8 *pTable, ex::EXBit128 &bits, s8 b) const;
    void BuildSkinRemapTable(const EXGeoHeader *pHdr, const EXGeoAnimSkinHeader *pSkinHdr);
    EXMatrix GetDatum_CalcMatrix(EXItemRender_SkinAnimSection *pSec, u32 n) const;
    EXItemRender_SkinInfo *SetActiveSkin(int i, int n);
    void CalcSectionMatrix(EXItemRender_SkinAnimSection *pSec, EXMatrix *pMtx);
    s8 CreateClone(const EXGeoRefObject &r, const EXItemRender_SkinAnim *pSrc);
    void BuildSkinGDiArray(const EXGeoHeader *pHdr, const EXGeoAnimSkinHeader *pSkinHdr);
    void BuildSkinRenderStateFlags();
    s8 AssignSkin(int i);
    s8 BuildSkin(int i);
    void ApplySkin(s8 b);
    void CalcMatrix();
public:
    ex::EXFlag<u8> m_updateSkin;            // +0xc SetUpdateSkin: 0xf
    ex::EXFlag<u8> m_updateNoRender;        // +0xd SetUpdateNoRender: 0xf
    ex::EXFlag<u8> m_0e;
    ex::EXFlag<u8> m_refChanged;            // +0xf 1 collision, 2 shadow, 4 reflection, 8 skin
    u8 m_10;
    s8 m_bOwnsSections;                     // +0x11 clones share the source's sections
    u8 m_lodMin;                            // +0x12
    u8 m_lodMax;                            // +0x13
    float m_lodDist;                        // +0x14
    u8 m_refTable[4];                       // +0x18 per ref: index of the ref it shares, 3 = none
    EXGeoComplexRefObject m_skelRef;        // +0x1c
    EXItemRender_SkinAnimSection *m_pBones; // +0x2c section array (root first)
    u32 m_numBones;                         // +0x30
    EXDList m_sections;                     // +0x34 top-level sections
    EXMatrix *m_pMtx;                       // +0x3c
    EXAnimLodDetails *m_pActiveLod;         // +0x40
    u32 m_numMorphs;                        // +0x44
    EXItemRender_SkinMorph *m_pMorphs;      // +0x48
    EXItemRender_SkinAnim *m_pClone;        // +0x4c next clone skin
    int m_activeSkin;                       // +0x50
    EXItemRender_SkinInfo *m_pActiveSkin;   // +0x54
    EXGeoComplexRefObject m_refs[4];        // +0x58 collision, shadow, reflection, skin
    EXItemRender_SkinInfo *m_pSkins[4];     // +0x98 one per ref; [3] is m_skin
    EXItemRender_SkinInfo m_skin;           // +0xa8
    EXRenderInfo m_renderInfo;              // +0x100
    EXDataArray<u32, u32> m_renderStates;   // +0x130 render states switched off
    ex::EXBit128 m_tempMtxBits;             // +0x140 sections with a temp matrix
};
#endif
