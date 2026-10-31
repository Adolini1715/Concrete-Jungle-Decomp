#include "EXCollisionScoopTemp.h"
#include "EXItemAnimator_Anim.h"
#include "EXItemRender_Anim.h"
#include "EXItemCtrl_SnapShot.h"
#include "EXDatum.h"
#include "EXMatrix.h"
#include "EXGeoEntity.h"
#include "EXGeoHeader.h"
#include "EXGeoFile.h"
#include "EXGeoAnim.h"
#include "EXStats.h"
#include "EXBaseRenderEnv.h"
#include "EXBaseDisplay.h"
#include "EXScratchPad.h"
#include "EXStdFuncs.h"
#include "EXMemory.h"

// TODO stand-in: item environment (EXItemEnv.obj).
class EXItemEnv {
public:
    float MinDistanceToItemViews(const EXVector &p);
    static EXItemEnv *m_pTheItemEnv;
};

// Section hierarchy entry of a skin (8 bytes): parent section first. TODO layout (geo area):
// relptr at EXGeoBaseAnimSkin +0x48.
struct EXGeoAnimSkin_Hier {
    u16 m_parent;   // 0xffff: root
    u16 m_02;
    u32 m_04;
};
static inline const EXGeoAnimSkin_Hier *ExSkinHier(const EXGeoBaseAnimSkin *p)
{
    s32 o = *(const s32 *)((const char *)p + 0x48);
    return o ? (const EXGeoAnimSkin_Hier *)((const char *)p + o + 0x48) : 0;
}

inline s8 operator==(const EXGeoBaseRefObject &a, const EXGeoBaseRefObject &b)
{
    return (a.m_pGeoFile == b.m_pGeoFile && a.m_hashRef == b.m_hashRef) ? 1 : 0;
}

inline s8 operator!=(const EXGeoBaseRefObject &a, const EXGeoBaseRefObject &b)
{
    return (a.m_pGeoFile != b.m_pGeoFile || a.m_hashRef != b.m_hashRef) ? 1 : 0;
}

EXRuntimeClass *EXItemRender_SkinAnim::_GetBaseClass() { return EXRUNTIME_CLASS(EXItemRender); }
const EXRuntimeClass EXItemRender_SkinAnim::classEXItemRender_SkinAnim = {
    EXRUNTIME_CLASS(EXItemRender), "EXItemRender_SkinAnim", sizeof(EXItemRender_SkinAnim),
    (void *(__cdecl *)())EXItemRender_SkinAnim::CreateObject};

void EXItemRender_SkinAnimSection::Initialise()
{
    Init();
    m_children.Init();
    m_bits.ClrAll();
    m_pos.z = 0.0f;
    m_pos.y = 0.0f;
    m_pos.x = 0.0f;
    m_pos.w = 1.0f;
    m_rot.z = 0.0f;
    m_rot.y = 0.0f;
    m_rot.x = 0.0f;
    m_rot.w = 1.0f;
}

void EXItemRender_SkinInfo::Initialise()
{
    m_pRemap = 0;
    m_usedBits.ClrAll();
    m_00.ClrAll();
    m_drawSkinRef.Set(0, 0);
    m_skinRef.Set(0, 0);
    m_frame = g_Stats.FrameCount() - 1;
    m_54 = 0;
}

void EXItemRender_SkinInfo::RemoveSkin()
{
    if (m_pRemap) {
        EXFree(m_pRemap);
        m_pRemap = 0;
    }
    m_gdi.RemoveAll();
}

void EXItemRender_SkinAnim::RemoveSkin(int i) const
{
    EXItemRender_SkinInfo *p = m_pSkins[i];
    if (p) p->RemoveSkin();
}

void EXItemRender_SkinAnim::UpdateSkinTables()
{
    for (int i = 2; i >= 0; i--) {
        int t = i;
        if (!m_refs[i].m_pGeoFile) {
            t = 3;
        } else {
            for (int j = i + 1; j < 3; j++) {
                if (*(const EXGeoBaseRefObject *)&m_refs[i] == *(const EXGeoBaseRefObject *)&m_refs[j]) {
                    t = j;
                    break;
                }
            }
        }
        m_refTable[i] = t;
    }
}

u8 *EXItemRender_SkinAnim::RemapTable() const
{
    EXItemRender_SkinInfo *p = m_pSkins[m_activeSkin];
    if (!p) return 0;
    return p->m_pRemap;
}

ex::EXBit128 EXItemRender_SkinAnim::CalculateSkinUsageBits(u32 flags) const
{
    ex::EXBit128 bits;
    bits.ClrAll();
    EXItemRender_SkinInfo *const *pp = m_pSkins;
    for (int n = 4; n; n--, pp++) {
        if (*pp) bits |= (*pp)->m_usedBits;
    }
    if (flags && m_pOwner && (((const EXItemAnimator_Anim *)m_pOwner)->LodDetailsFlags() & 0x80))
        bits &= m_pActiveLod->m_skinBits;
    if (m_pClone) bits |= m_pClone->CalculateSkinUsageBits(flags);
    return bits;
}

EXItemRender_SkinAnimSection *EXItemRender_SkinAnim::GetSectionParent(const EXItemRender_SkinAnimSection *p) const
{
    u32 i = p->ParentIndex();
    if (i == 0xffff) return 0;
    return &m_pBones[i];
}

void EXItemRender_SkinAnim::DeleteCloneSkin(EXItemRender_SkinAnim *p, s8 b)
{
    if (!m_pClone) return;
    if (m_pClone != p) {
        m_pClone->DeleteCloneSkin(p, b);
        return;
    }
    if (b) {
        if (m_pClone) {
            delete m_pClone;
            m_pClone = 0;
        }
    } else {
        EXItemRender_SkinAnim *pNext = m_pClone->m_pClone;
        m_pClone->m_pClone = 0;
        if (m_pClone) {
            delete m_pClone;
            m_pClone = 0;
        }
        m_pClone = pNext;
    }
}

EXItemRender_SkinAnim *EXItemRender_SkinAnim::FindCloneSkin(const EXGeoRefObject &r) const
{
    const EXItemRender_SkinAnim *pSkin = this;
    while (pSkin->m_pClone) {
        const EXGeoComplexRefObject &ref = pSkin->m_pClone->m_refs[3];
        if (ref.m_pGeoFile == r.m_pGeoFile && ref.m_hashRef == r.m_hashRef) return pSkin->m_pClone;
        pSkin = pSkin->m_pClone;
    }
    return 0;
}

EXItemRender_SkinInfo::EXItemRender_SkinInfo()
{
    Initialise();
}

EXItemRender_SkinAnim::~EXItemRender_SkinAnim()
{
    for (int i = 0; i < 4; i++) {
        RemoveSkin(i);
        if (i != 3 && m_pSkins[i]) {
            delete m_pSkins[i];
            m_pSkins[i] = 0;
        }
    }
    if (m_pClone) {
        delete m_pClone;
        m_pClone = 0;
    }
    if (m_bOwnsSections && m_pBones) delete[] m_pBones;
}

void EXItemRender_SkinAnim::SetLodMin(u32 lod)
{
    m_lodMin = (u8)limit<u32>(lod, 0, 10);
    m_lodMax = max(m_lodMin, m_lodMax);
}

void EXItemRender_SkinAnim::SetLodMax(u32 lod)
{
    m_lodMax = (u8)limit<u32>(lod, 0, 10);
    m_lodMin = min(m_lodMin, m_lodMax);
}

void EXItemRender_SkinAnim::BuildRemapTable(const EXGeoAnimSkin *pA, const EXGeoAnimSkin *pB, u8 *pTable, ex::EXBit128 &bits, s8 b) const
{
    const EXGeoAnimSkin_BoneTbl *pTb = pB->AnimBoneTbl();
    const EXGeoAnimSkin_Hier *pHb = ExSkinHier(pB);
    const EXGeoAnimSkin_BoneTbl *pTa = pA->AnimBoneTbl();
    const EXGeoAnimSkin_Hier *pHa = ExSkinHier(pA);
    int n = pA->BoneCount();
    memset(pTable, 0xff, n);
    bits.ClrAll();
    while (pTa->m_hash != 0xffff) {
        if (pTa->m_hash == pTb->m_hash) {
            u32 idx = pTb->m_index;
            pTable[pTa->m_index] = idx;
            do {
                bits.Set(idx);
                idx = pHb[idx].m_parent;
            } while (idx != 0xffff);
            pTa++;
            pTb++;
        } else if (pTa->m_hash > pTb->m_hash) {
            pTb++;
        } else {
            pTa++;
        }
    }
    if (b) {
        for (int i = 0; i < n; i++) {
            if (pTable[i] == 0xff) {
                u32 p = pHa[i].m_parent;
                pTable[i] = p != 0xffff ? pTable[p] : 0;
            }
        }
    }
}

void EXItemRender_SkinAnim::BuildSkinRemapTable(const EXGeoHeader *pHdr, const EXGeoAnimSkinHeader *pSkinHdr)
{
    if (!pSkinHdr) return;
    const EXGeoAnimSkin *pSkin = pSkinHdr->m_Addr[0].m_pObject;
    if (!pSkin) return;
    if (*(const EXGeoBaseRefObject *)&m_skelRef != EXGeoRefObject(m_pActiveSkin->m_drawSkinRef.m_pGeoFile, pSkinHdr->m_10)) {
        EXGeoAnimSkin *pSkel = m_skelRef.GeoAnimSkin();
        if (!pSkel) return;
        int tmp74 = pSkin->ValidateCheckSum(pSkel);
if (tmp74) {
            m_pActiveSkin->m_pRemap = (u8 *)_EXAllocPure(pSkin->BoneCount(), 0);
            if (!m_pActiveSkin->m_pRemap) return;
            BuildRemapTable(pSkin, pSkel, m_pActiveSkin->m_pRemap, m_pActiveSkin->m_usedBits, 1);
            return;
        }
    }
    m_pActiveSkin->m_usedBits.Set(0, pSkin->BoneCount());
}

void EXItemRender_SkinAnim::BuildHierData_ModifyMorph(const EXRotPosInfo *pInfo, float f)
{
    EXGeoAnimSkin *pSkin = m_skelRef.GeoAnimSkin();
    if (!pSkin) return;
    int n = pSkin->DefomerCount();
    if (!n) return;
    float *p = pInfo->m_pMorph;
    for (int i = 0; i < n; i++) p[i] *= f;
}

void EXItemRender_SkinAnim::SetSkinRefObject(const EXGeoBaseRefObject &r)
{
    m_refs[3].Set(r);
    m_refChanged.BitSet(8);
    UpdateSkinTables();
}

void EXItemRender_SkinAnim::SetSkinRefObject(EXGeoFile *pFile, u32 hash)
{
    m_refs[3].Set(pFile, hash);
    m_refChanged.BitSet(8);
    UpdateSkinTables();
}

void EXItemRender_SkinAnim::SetSkinRefObject(u32 hash)
{
    m_refs[3].SetHashRef(hash);
    m_refChanged.BitSet(8);
    UpdateSkinTables();
}

void EXItemRender_SkinAnim::SetShadowRefObject(const EXGeoBaseRefObject &r)
{
    m_refs[1].Set(r);
    m_refChanged.BitSet(2);
    UpdateSkinTables();
}

void EXItemRender_SkinAnim::SetCollisionRefObject(const EXGeoBaseRefObject &r)
{
    m_refs[0].Set(r);
    m_refChanged.BitSet(1);
    UpdateSkinTables();
}

void EXItemRender_SkinAnim::SetReflectionRefObject(const EXGeoBaseRefObject &r)
{
    m_refs[2].Set(r);
    m_refChanged.BitSet(4);
    UpdateSkinTables();
}

EXItemRender_SkinInfo *EXItemRender_SkinAnim::SetActiveSkin(int i, int)
{
    if (!m_pSkins[i]) m_pSkins[i] = new EXItemRender_SkinInfo;
    m_activeSkin = i;
    m_pActiveSkin = m_pSkins[i];
    return m_pActiveSkin;
}

void EXItemRender_SkinAnim::Invalidate()
{
    m_10 = 0;
    m_0e = 0;
}

void EXItemRender_SkinAnim::SetUpdateSkin()
{
    m_updateSkin.BitSet(0xf);
}

void EXItemRender_SkinAnim::SetUpdateNoRender()
{
    m_updateNoRender.BitSet(0xf);
}

void EXItemRender_SkinAnim::CalcChildMatrix(EXItemRender_SkinAnimSection *pSec, EXMatrix *pMtx)
{
    EXItemRender_SkinAnimSection *pChild = pSec->ChildLinks();
    if (pChild) {
        EXScratchPad pad;
        CalcSectionMatrix(pChild, pMtx, (EXItemRender_SkinAnimSectionTemp *)pad.Claim(0x100));
    }
}

void EXItemRender_SkinAnim::CalcSectionMatrix(EXItemRender_SkinAnimSection *pSec, EXMatrix *pMtx)
{
    EXScratchPad pad;
    EXItemRender_SkinAnimSectionTemp *pTemp = (EXItemRender_SkinAnimSectionTemp *)pad.Claim(0x100);
    CalcSectionMatrix(pSec, pMtx, pTemp);
}

EXItemRender_SkinAnimSection *EXItemRender_SkinAnim::GetAnimBoneSection(u32 hash, u32 flags) const
{
    const EXGeoComplexRefObject &ref = (flags & 0x20) ? m_refs[3] : m_skelRef;
    EXGeoAnimSkin *pSkin = ref.GeoAnimSkin();
    if (!pSkin) return 0;
    const EXGeoAnimSkin_BoneTbl *t = pSkin->AnimBoneHashTbl();
    while (t->m_hash != 0xffff) {
        if (t->m_hash == (u16)hash) return &m_pBones[t->m_index];
        t++;
    }
    return 0;
}

s8 EXItemRender_SkinAnim::CreateClone(const EXGeoRefObject &r, const EXItemRender_SkinAnim *pSrc)
{
    m_bOwnsSections = 0;
    EXGeoComplexRefObject ref;
    ref.Set(r);
    if (!ref.GeoAnimSkin()) return 0;
    m_numBones = pSrc->m_numBones;
    m_numMorphs = pSrc->m_numMorphs;
    m_skelRef = pSrc->m_skelRef;
    m_pBones = pSrc->m_pBones;
    m_pMtx = pSrc->m_pMtx;
    m_pMorphs = pSrc->m_pMorphs;
    SetSkinRefObject(r);
    Invalidate();
    return 1;
}

s8 EXItemRender_SkinAnim::Create(const EXGeoRefObject &r)
{
    m_skelRef.Set(r);
    EXGeoAnimSkin *pSkin = m_skelRef.GeoAnimSkin();
    if (!pSkin) return 0;
    m_numBones = pSkin->BoneCount();
    m_numMorphs = pSkin->DefomerCount();
    EXItemRender_SkinAnimSection *pBones = (EXItemRender_SkinAnimSection *)_EXAlloc(
        m_numBones * sizeof(EXMatrix) + m_numMorphs * sizeof(float) + m_numBones * sizeof(EXItemRender_SkinAnimSection), 2);
    if (!pBones) for (;;) {}
    m_pBones = pBones;
    m_pMtx = (EXMatrix *)&pBones[m_numBones];
    m_pMorphs = (EXItemRender_SkinMorph *)&m_pMtx[m_numBones];
    const EXGeoAnimSkin_Hier *pHier = ExSkinHier(pSkin);
    EXItemRender_SkinAnimSection *pSec = pBones;
    for (int i = 0; i < (int)m_numBones; i++, pSec++, pHier++) {
        pSec->Initialise();
        if (pHier->m_parent == 0xffff) m_sections.AddTail(pSec);
        else pBones[pHier->m_parent].m_children.AddTail(pSec);
        pSec->m_bits.Set(i, pHier->m_02);
        pSec->m_pMtx = &m_pMtx[i];
        m_pMtx[i].SetIdentity();
        pSec->m_bone = i;
        pSec->m_parent = pHier->m_parent;
    }
    m_refChanged.BitSet(0xf);
    Invalidate();
    memset(m_pMorphs, 0, m_numMorphs * sizeof(float));
    return 1;
}

void EXItemRender_SkinAnim::ApplyFunction(u32 (__cdecl *pFunc)(EXGeoBaseEntity *, EXGDiData *))
{
    EXGeoComplexRefObject &ref = m_pSkins[3]->m_drawSkinRef;
    EXGeoFile *pFile = ref.m_pGeoFile;
    if (!pFile) return;
    EXGeoHeader *pHdr = pFile->GeoHeader();
    if (!pHdr) return;
    EXGeoAnimSkin *pSkin = ref.GeoAnimSkin();
    if (!pSkin) return;
    const EXGeoEntityHeader *pEnt = pHdr->GeoEntityHeader(0);
    EXGDiData *pData = m_skin.m_gdi.Data();
    int n = 0;
    const EXGeoSkinEntityRef *pList = pSkin->m_EntityGroups[0].Data();
    for (u32 i = 0; i < pSkin->TotalSkinCount(); i++) {
        EXGeoBaseEntity *e = pEnt[pList[i].m_EntityHash & 0xffffff].Object(0);
        e->ApplyFunction(pFunc, &pData[n]);
        n += e->GDiCount();
    }
}

EXGeoBaseEntity *EXItemRender_SkinAnim::FindAttachedEntity(u32 hash, EXGDiData *&pData, s8 b)
{
    EXGeoComplexRefObject &ref = b ? m_pSkins[3]->m_drawSkinRef : m_refs[3];
    EXGeoAnimSkin *pSkin = ref.GeoAnimSkin();
    if (!pSkin) return 0;
    EXGeoHeader *pHdr = ref.m_pGeoFile->GeoHeader();
    if (!pHdr) return 0;
    const EXGeoEntityHeader *pEnt = pHdr->GeoEntityHeader(0);
    if (!pEnt) return 0;
    EXGDiData *pD = m_skin.m_gdi.Data();
    int n = 0;
    const EXGeoSkinEntityRef *pList = pSkin->m_EntityGroups[0].Data();
    for (u32 i = 0; i < pSkin->TotalSkinCount(); i++) {
        EXGeoBaseEntity *e = pEnt[pList[i].m_EntityHash & 0xffffff].Object(0);
        if (pList[i].m_00[2] == hash) {
            if (pD) pData = &pD[n];
            return e;
        }
        n += e->GDiCount();
    }
    return 0;
}

void EXItemRender_SkinAnim::BuildSkinGDiArray(const EXGeoHeader *pHdr, const EXGeoAnimSkinHeader *pSkinHdr)
{
    if (!pSkinHdr) return;
    const EXGeoAnimSkin *pSkin = pSkinHdr->m_Addr[0].m_pObject;
    if (!pSkin) return;
    const EXGeoSkinEntityRef *pList = pSkin->m_EntityGroups[0].Data();
    if (!pList) return;
    const EXGeoEntityHeader *pEnt = pHdr->GeoEntityHeader(0);
    int n = 0;
    for (u32 i = 0; i < pSkin->TotalSkinCount(); i++) {
        EXGeoBaseEntity *e = pEnt[pList[i].m_EntityHash & 0xffffff].Object(0);
        n += e->GDiCount();
    }
    m_pActiveSkin->m_gdi.Init(n);
}

void EXItemRender_SkinAnim::SetRenderState(u32 state, s8 bOn)
{
    for (int i = 0; i < m_renderStates.Size(); i++) {
        if (m_renderStates[i] == state) {
            if (bOn) {
                m_renderStates.RemoveAt(i);
                m_updateNoRender.BitSet(0xf);
            }
            return;
        }
    }
    if (!bOn) {
        m_renderStates.Add(state);
        m_updateNoRender.BitSet(0xf);
    }
}

void *EXItemRender_SkinAnim::GetDatum_AnimBoneDatum(u32 type, EXDatumTemp &d, u32 flags) const
{
    EXItemRender_SkinAnimSection *pSec = GetAnimBoneSection(type, flags);
    if (pSec) {
        EXMatrix m = GetDatum_CalcMatrix(pSec, flags);
        d.m_position = m.r[3];
        d.m_rotation.SetRotation(m);
        d.m_type = 1;
        d.m_2c = 0;
        d.m_2e = 0;
        d.m_size.Set(0.0f, 0.0f, 0.0f);
    }
    return pSec;
}

void *EXItemRender_SkinAnim::GetDatum_AnimBoneDatum(const EXVector &p, EXDatumTemp &d, u32 flags) const
{
    EXItemRender_SkinAnimSection *pSec = GetAnimBoneSection(p, flags);
    if (pSec) {
        d.m_position = pSec->m_pMtx->r[3];
        d.m_rotation.SetRotation(*pSec->m_pMtx);
        d.m_type = 1;
        d.m_2c = 0;
        d.m_2e = 0;
        d.m_size.Set(0.0f, 0.0f, 0.0f);
    }
    return pSec;
}

s8 EXItemRender_SkinAnim::AssignSkin(int i)
{
    EXItemRender_SkinInfo *pInfo = m_pSkins[i];
    s8 result = 0;
    s8 changed = (*(const EXGeoBaseRefObject *)&pInfo->m_skinRef != *(const EXGeoBaseRefObject *)&pInfo->m_drawSkinRef) |
                 (m_updateSkin & (u8)(1 << m_activeSkin));
    if (changed) {
        pInfo->RemoveSkin();
        pInfo->m_drawSkinRef.Set(pInfo->m_skinRef);
        EXGeoAnimSkinHeader *pHdr = pInfo->m_drawSkinRef.GeoAnimSkinHeader();
        if (!pHdr) return 0;
        const EXGeoHeader *pGeo = pInfo->m_drawSkinRef.m_pGeoFile->GeoHeader();
        BuildSkinRemapTable(pGeo, pHdr);
        BuildSkinGDiArray(pGeo, pHdr);
        result = 1;
        m_updateSkin.BitClr((u8)(1 << m_activeSkin));
        if (m_10) {
            ex::EXBit128 used = CalculateSkinUsageBits(1);
            ex::EXBit128 t = m_tempMtxBits;
            t &= used;
            t ^= used;
            m_10 = !t.IsAnySet();
        }
    }
    if ((m_updateNoRender & (u8)(1 << m_activeSkin)) | changed) {
        BuildSkinRenderStateFlags();
        m_updateNoRender.BitClr((u8)(1 << m_activeSkin));
    }
    return result;
}

const EXGeoAnimSkinHeader *EXItemRender_SkinAnim::UpdateSkinInformation(int n, const EXVector &p)
{
    EXItemRender_SkinInfo *pInfo = m_pSkins[n];
    if (!pInfo) return 0;
    if (pInfo->m_frame != g_Stats.FrameCount()) {
        float d = EXItemEnv::m_pTheItemEnv->MinDistanceToItemViews(p);
        m_lodDist = d;
        const EXGeoAnimSkinHeader *pHdr = EXGeoAnimSkinHeader::CalcAnimSkinLod(m_refs[n], pInfo->m_54, m_lodMin, m_lodMax, d);
        if (!pHdr || !pHdr->m_Addr[0].m_pObject) return 0;
        pInfo->m_frame = g_Stats.FrameCount();
        EXGeoRefObject r(m_refs[n].m_pGeoFile, pHdr->HashCode());
        if (*(const EXGeoBaseRefObject *)&pInfo->m_skinRef != r) pInfo->m_skinRef.Set(r);
        SetActiveSkin(n, 0);
        AssignSkin(n);
        return pHdr;
    }
    return pInfo->m_skinRef.GeoAnimSkinHeader();
}

void EXItemRender_SkinAnim::ListSections(EXPtrArray<void> &arr, EXItemRender_SkinAnimSection *pSec) const
{
    EXItemRender_SkinAnimSection *p = pSec ? pSec->ChildLinks() : (EXItemRender_SkinAnimSection *)m_sections.m_pHead;
    for (; p; p = p->Next()) {
        arr.Add(p);
        ListSections(arr, p);
    }
}

s8 EXItemRender_SkinAnim::BuildSkin(int i)
{
    if (!SetActiveSkin(i, 0)) return 0;
    EXGeoComplexRefObject &ref = m_refs[i];
    EXGeoAnimSkinHeader *pHdr = ref.GeoAnimSkinHeader();
    if (!pHdr || !pHdr->m_Addr[0].m_pObject) return 0;
    m_pSkins[i]->m_skinRef.Set(ref.m_pGeoFile, pHdr->m_HashCode);
    AssignSkin(i);
    if (!m_10) CalcMatrix();
    else EXBaseRenderEnv::SetRenderState(2);
    ApplySkin(1);
    if (m_10) m_10 = 1;
    return 1;
}

EXItemRender_SkinAnim::EXItemRender_SkinAnim()
{
    m_numBones = 0;
    m_lodMin = 0;
    m_lodMax = 10;
    m_lodDist = 0.0f;
    m_pBones = 0;
    m_pMtx = 0;
    m_pMorphs = 0;
    m_pActiveLod = 0;
    m_pOwner = 0;
    m_10 = 0;
    m_bOwnsSections = 1;
    m_pActiveSkin = 0;
    m_activeSkin = 3;
    m_pClone = 0;
    m_skelRef.Set(0, 0xffffffff);
    for (int i = 0; i < 4; i++) {
        m_refs[i].Set(0, 0xffffffff);
        m_refTable[i] = 3;
        m_pSkins[i] = 0;
    }
    m_updateNoRender = 0;
    m_0e = 0;
    m_refChanged = 0;
    m_skin.Initialise();
    m_pSkins[3] = &m_skin;
    m_tempMtxBits.ClrAll();
    m_renderStates.SetGrowBy(8);
}

void EXItemRender_SkinAnim::DoRender(const EXMatrix &mtx, const EXRenderInfo &info)
{
    EXRenderInfo ri(m_renderInfo, info);
    EXBaseRenderEnv::SetRenderInfo(ri);
    if (EXBaseRenderEnv::RenderState() & 1) m_0e = 0;
    int n = 3;
    if (EXBaseRenderEnv::RenderState() & 4) n = 1;
    else if (EXBaseRenderEnv::RenderState() & 8) n = 2;
    if (m_refChanged.BitTest((u8)(1 << n))) {
        int s = m_refTable[n];
        if (SetActiveSkin(s, 0)) {
            EXGeoHeader *pHdr = m_refs[s].m_pGeoFile->GeoHeader();
            if (pHdr) {
                EXBaseDisplay::SetGeoHeader(pHdr);
                EXVector pos = mtx.r[3];
                if (UpdateSkinInformation(m_activeSkin, pos)) {
                    if (!m_10 && m_bOwnsSections) CalcMatrix();
                    if (EXBaseRenderEnv::RenderState() & 2) {
                        if (!m_0e.BitTest((u8)(1 << m_activeSkin))) ApplySkin(0);
                        RenderSkin(mtx);
                    }
                    if (m_10) m_10 = 1;
                }
            }
        }
    }
    if (m_pClone) m_pClone->DoRender(mtx, info);
}

EXItemRender_SkinAnim *EXItemRender_SkinAnim::AddCloneSkin(const EXGeoRefObject &r)
{
    if (m_pClone) return m_pClone->AddCloneSkin(r);
    m_pClone = new EXItemRender_SkinAnim;
    if (m_pClone && !m_pClone->CreateClone(r, this)) {
        if (m_pClone) {
            delete m_pClone;
            m_pClone = 0;
        }
        return 0;
    }
    return m_pClone;
}

EXItemRender_SkinAnim *EXItemRender_SkinAnim::CreateObject()
{
    return new EXItemRender_SkinAnim;
}

// Local matrix of a section: its rotation and position.
static inline void SectionLocalMatrix(EXMaths::mat_44 &m, const EXItemRender_SkinAnimSection *pSec)
{
    m.set_rotate_quat(pSec->m_rot);
    m.r[3].x = pSec->m_pos.x;
    m.r[3].y = pSec->m_pos.y;
    m.r[3].z = pSec->m_pos.z;
    m.r[3].w = pSec->m_pos.w;
}

// Skin-space matrix of a section: its cached matrix when this frame's matrices are valid for it,
// otherwise the product of the local matrices up its parent chain.
EXMatrix EXItemRender_SkinAnim::GetDatum_CalcMatrix(EXItemRender_SkinAnimSection *pSec, u32 flags) const
{
    if (((flags & 8) || m_10) && m_tempMtxBits.IsSet(pSec->m_bone)) return *pSec->m_pMtx;
    EXItemRender_SkinAnimSection *chain[32];
    int n = -1;
    do {
        chain[++n] = pSec;
        pSec = pSec->m_parent == 0xffff ? 0 : &m_pBones[pSec->m_parent];
        if (n > 31) for (;;) {}
    } while (pSec);
    EXMaths::mat_44 m;
    m.set_identity();
    for (; n >= 0; n--) {
        EXMaths::mat_44 local, tmp;
        SectionLocalMatrix(local, chain[n]);
        D3DXMatrixMultiply(&tmp, &local, &m);
        m = tmp;
    }
    return *(EXMatrix *)&m;
}

// Skin-space matrix of a skeleton datum's bone: from the deepest valid cached matrix along its
// chain (flags 8: the bone's own), multiplying in the remaining local matrices.
EXMatrix EXItemRender_SkinAnim::GetDatum_CalcMatrix(const EXGeoAnimSkelDatum *pDatum, u32 flags)
{
    if ((flags & 8) && m_tempMtxBits.IsSet(pDatum->m_Bone)) return m_pMtx[pDatum->m_Bone];
    EXMaths::mat_44 m;
    int start = 0;
    bool found = false;
    if (m_10) {
        for (int i = pDatum->m_NumChain; i - 1 >= 1; i--) {
            u8 bone = pDatum->m_Chain[i - 1];
            if (m_tempMtxBits.IsSet(bone)) {
                m = m_pMtx[bone];
                start = i;
                found = true;
                break;
            }
        }
    }
    if (!found) m.set_identity();
    for (int i = start; i < pDatum->m_NumChain; i++) {
        EXMaths::mat_44 local, tmp;
        SectionLocalMatrix(local, &m_pBones[pDatum->m_Chain[i]]);
        D3DXMatrixMultiply(&tmp, &local, &m);
        m = tmp;
    }
    return *(EXMatrix *)&m;
}

// The same against a caller's matrix cache (pMtx, valid where pBits is set); the result is stored
// back in the cache slot of the chain's last bone.
EXMatrix EXItemRender_SkinAnim::GetDatum_CalcMatrixSP(const EXGeoAnimSkelDatum *pDatum, u32, EXMatrix *pMtx,
                                                      const ex::EXBit128 *pBits) const
{
    int bone = -1;
    int start;
    EXMaths::mat_44 m;
    int i = pDatum->m_NumChain;
    for (;;) {
        start = i;
        i = start - 1;
        if (i < 0) {
            m.set_identity();
            start = 0;
            break;
        }
        bone = pDatum->m_Chain[i];
        if (pBits->IsSet(bone)) {
            m = pMtx[bone];
            break;
        }
    }
    for (; start < pDatum->m_NumChain; start++) {
        bone = pDatum->m_Chain[start];
        EXMaths::mat_44 local, tmp;
        SectionLocalMatrix(local, &m_pBones[bone]);
        D3DXMatrixMultiply(&tmp, &local, &m);
        m = tmp;
    }
    if (bone >= 0) *(EXMaths::mat_44 *)&pMtx[bone] = m;
    return *(EXMatrix *)&m;
}

// Build the matrices of the sections under pSec that the visible skin parts use (depth first, pTemp
// is the walk stack), marking them valid in m_tempMtxBits.
void EXItemRender_SkinAnim::CalcSectionMatrix(EXItemRender_SkinAnimSection *pSec, EXMatrix *pMtx,
                                              EXItemRender_SkinAnimSectionTemp *pTemp)
{
    int depth = 0;
    ex::EXBit128 usage = CalculateSkinUsageBits(m_tempMtxBits.IsAnySet());
    ex::EXBit128 done;
    done.Set(m_tempMtxBits);
    for (;;) {
        pTemp[depth].m_pSec = pSec;
        int w;
        for (w = 0; w < 4; w++)
            if (usage.m_bits[w] & pSec->m_bits.m_bits[w]) break;
        EXItemRender_SkinAnimSection *pNext = 0;
        if (w < 4) {
            u32 bone = pSec->m_bone;
            pNext = pSec->ChildLinks();
            u16 parent = pSec->m_parent;
            EXMaths::mat_44 &m = *(EXMaths::mat_44 *)&pMtx[bone];
            SectionLocalMatrix(m, pSec);
            if (parent != 0xffff) {
                EXMaths::mat_44 local = m;
                EXMaths::mat_44 tmp;
                D3DXMatrixMultiply(&tmp, &local, &pMtx[parent]);
                m = tmp;
            }
            done.Set(bone);
            if (pNext) depth++;
        }
        if (!pNext) {
            while (!(pNext = pTemp[depth].m_pSec->Next())) {
                if (--depth < 0) {
                    m_tempMtxBits.Set(done);
                    return;
                }
            }
        }
        pSec = pNext;
    }
}

// Mark the skin parts whose render state was switched off (m_renderStates): entities by entity
// hash (class 0x02000000), or the parts hanging off a bone by bone hash (class 0x0e000000).
void EXItemRender_SkinAnim::BuildSkinRenderStateFlags()
{
    m_pActiveSkin->m_00.ClrAll();
    if (!m_renderStates.Size()) return;
    EXGeoFile *pFile = m_pActiveSkin->m_drawSkinRef.m_pGeoFile;
    if (!pFile) return;
    EXGeoHeader *pHdr = pFile->m_pGeoHeader;
    if (!pHdr) return;
    EXGeoAnimSkinHeader *pSkinHdr = m_pActiveSkin->m_drawSkinRef.GeoAnimSkinHeader();
    if (!pSkinHdr) return;
    EXGeoBaseAnimSkin *pSkin = (EXGeoBaseAnimSkin *)pSkinHdr->m_Addr[0].m_pObject;
    if (!pSkin) return;
    const EXGeoSkinEntityRef *pRefs = pSkin->m_EntityGroups[0].Data();
    const EXGeoSkinEntityRef *pEntityRefs = pSkin->m_EntityGroups[2].Data();
    const EXGeoEntityHeader *pEntities = pHdr->GeoEntityHeader(0);
    int numSkins = pSkin->m_EntityGroups[0].Size();
    int numEntities = pSkin->m_EntityGroups[2].Size();
    int numLinked = pSkin->m_EntityGroups[1].Size();
    int total = numSkins + numLinked + numEntities;
    for (int i = 0; i < m_renderStates.Size(); i++) {
        u32 hash = m_renderStates[i];
        if (!hash) continue;
        if ((hash & 0x7f000000) == 0x2000000) {
            // Every part (the groups are laid out back to back) drawing this entity.
            for (int k = 0; k < total; k++)
                if (pEntities[pRefs[k].m_EntityHash & 0xffffff].m_HashCode == hash) m_pActiveSkin->m_00.Set(k);
        } else if ((hash & 0x7f000000) == 0xe000000) {
            const EXGeoAnimSkin_BoneTbl *pBone = pSkin->AnimBoneHashTbl();
            for (; pBone->m_hash != 0xffff; pBone++) {
                if (pBone->m_hash != (u16)hash) continue;
                u16 bone = pBone->m_index;
                for (int k = 0; k < numEntities; k++)
                    if (pEntityRefs[k].m_00[2] == bone) m_pActiveSkin->m_00.Set(numSkins + numLinked + k);
                break;
            }
        }
    }
}

// Reset a blend stack: (with `full`) every bone to its rest position with no rotation, and all
// morph weights to zero.
void EXItemRender_SkinAnim::InitBlendStack(const EXRotPosInfo *pInfo, s8 full)
{
    EXGeoAnimSkinHeader *pSkinHdr = m_skelRef.GeoAnimSkinHeader();
    if (!pSkinHdr) return;
    EXGeoBaseAnimSkin *pSkin = (EXGeoBaseAnimSkin *)pSkinHdr->m_Addr[0].m_pObject;
    if (!pSkin) return;
    if (full) {
        EXMotionData *pMotion = pInfo->m_pMotion;
        for (u32 i = 0; i < m_numBones; i++) {
            const EXVector &rest = pSkin->RotDataRel()[i];
            pMotion[i].m_pos.x = rest.x;
            pMotion[i].m_pos.y = rest.y;
            pMotion[i].m_pos.z = rest.z;
            pMotion[i].m_pos.w = rest.w;
            pMotion[i].m_rot.z = 0.0f;
            pMotion[i].m_rot.y = 0.0f;
            pMotion[i].m_rot.x = 0.0f;
            pMotion[i].m_rot.w = 1.0f;
        }
    }
    float *pMorph;
    if (m_numMorphs && (pMorph = pInfo->m_pMorph) != 0)
        for (u32 i = m_numMorphs; i; i--) *pMorph++ = 0.0f;
}

// Skeleton datum table of a skin (relptr at EXGeoBaseAnimSkin +0x64): count, then 8-byte entries
// {id, number of following entries to skip when the id does not match, relptr to the datum}.
struct EXGeoAnimSkelDatumEntry {
    u32 m_Id;
    u16 m_Skip;
    s16 m_Datum;    // self-relative (from this field)
    const EXGeoAnimSkelDatum *Datum() const
    {
        if (!m_Datum) return 0;
        return (const EXGeoAnimSkelDatum *)((const u8 *)&m_Datum + m_Datum);
    }
};
struct EXGeoAnimSkelDatumTable {
    u8 m_00[5];
    u8 m_Count;     // +5
    s16 m_Entries;  // +6 self-relative
    const EXGeoAnimSkelDatumEntry *Entries() const
    {
        if (!m_Entries) return 0;
        return (const EXGeoAnimSkelDatumEntry *)((const u8 *)&m_Entries + m_Entries);
    }
};
static inline const EXGeoAnimSkelDatumTable *ExSkelDatums(const EXGeoBaseAnimSkin *p)
{
    return *(const ex::EXRelPtr<EXGeoAnimSkelDatumTable> *)&p->m_30[0x34];
}
static inline const EXVector *ExSkelScales(const EXGeoBaseAnimSkin *p)
{
    return *(const ex::EXRelPtr<EXVector> *)&p->m_30[0x10];
}

// Find skeleton datum `id` (the next one after pCtx when iterating) and place it in d. Flags:
// 0x20 search the skin rather than the skeleton, 0x40 unplaced (identity, no scale), 8 use the
// cached section matrices. Returns the iteration key, or 0.
void *EXItemRender_SkinAnim::GetDatum_SkelDatum(u32 id, EXDatumTemp &d, void *&pCtx, u32 flags)
{
    const EXGeoComplexRefObject &ref = (flags & 0x20) ? m_refs[3] : m_skelRef;
    EXGeoAnimSkinHeader *pSkinHdr = ref.GeoAnimSkinHeader();
    if (!pSkinHdr) return 0;
    EXGeoBaseAnimSkin *pSkin = (EXGeoBaseAnimSkin *)pSkinHdr->m_Addr[0].m_pObject;
    const EXGeoAnimSkelDatumTable *pTable;
    if (!pSkin || !(pTable = ExSkelDatums(pSkin))) return 0;
    u32 count = pTable->m_Count;
    if (!count) return 0;
    const EXGeoAnimSkelDatumEntry *pEntries = pTable->Entries();
    for (u32 i = 0; i < count; i++) {
        u32 key = ((u32)this << 8) | i;
        if (pCtx) {
            if ((u32)pCtx == key) pCtx = 0;
            continue;
        }
        if (pEntries[i].m_Id != id) {
            i += pEntries[i].m_Skip;
            continue;
        }
        const EXGeoAnimSkelDatum *pDatum = pEntries[i].Datum();
        if (flags & 0x40) {
            EXMatrix identity;
            identity.set_identity();
            EXVector zero(0.0f, 0.0f, 0.0f, 0.0f);
            d.Set((const EXGeoBaseDatum *)pDatum, zero, identity);
            return (void *)key;
        }
        EXMatrix mtx = GetDatum_CalcMatrix(pDatum, flags);
        d.Set((const EXGeoBaseDatum *)pDatum, ExSkelScales(pSkin)[pDatum->m_Bone], mtx);
        return (void *)key;
    }
    return 0;
}

// Every skeleton datum `id`, appended to arr (matrices built once through a scratch cache unless
// the section matrices are current).
void EXItemRender_SkinAnim::GetDatum_SkelDatum(u32 id, EXDatumTempArray &arr, u32 flags)
{
    const EXGeoComplexRefObject &ref = (flags & 0x20) ? m_refs[3] : m_skelRef;
    EXGeoAnimSkinHeader *pSkinHdr = ref.GeoAnimSkinHeader();
    if (!pSkinHdr) return;
    EXGeoBaseAnimSkin *pSkin = (EXGeoBaseAnimSkin *)pSkinHdr->m_Addr[0].m_pObject;
    const EXGeoAnimSkelDatumTable *pTable;
    if (!pSkin || !(pTable = ExSkelDatums(pSkin))) return;
    u32 count = pTable->m_Count;
    if (!count) return;
    ex::EXBit128 *pCache = 0;
    EXScratchPad pad;
    if (!m_10 && !(flags & 8) && (pCache = (ex::EXBit128 *)pad.Claim(0x2010)) != 0) pCache->ClrAll();
    const EXGeoAnimSkelDatumEntry *pEntries = pTable->Entries();
    for (u32 i = 0; i < count; i++) {
        if (pEntries[i].m_Id != id) {
            i += pEntries[i].m_Skip;
            continue;
        }
        const EXGeoAnimSkelDatum *pDatum = pEntries[i].Datum();
        int n = arr.Size();
        arr.SetSize(n + 1, -1);
        EXDatumTemp &d = arr[n];
        if (!(flags & 0x40)) {
            EXMatrix mtx = pCache ? GetDatum_CalcMatrixSP(pDatum, flags, (EXMatrix *)(pCache + 1), pCache)
                                  : GetDatum_CalcMatrix(pDatum, flags);
            d.Set((const EXGeoBaseDatum *)pDatum, ExSkelScales(pSkin)[pDatum->m_Bone], mtx);
        } else {
            EXMatrix identity;
            identity.set_identity();
            EXVector zero(0.0f, 0.0f, 0.0f, 0.0f);
            d.Set((const EXGeoBaseDatum *)pDatum, zero, identity);
        }
    }
    pad.Release();
}

// Collision scoop against the skin's pieces with collision: skins and linked skins in the item's
// frame (flag 1), bone-attached entities in their section's frame.
void EXItemRender_SkinAnim::DoEntityScoop(const EXMatrix &mtx, EXCollisionScoopTemp &t, u32 flags)
{
    if (!BuildSkin(0)) return;
    int active = m_activeSkin;
    EXGeoAnimSkinHeader *pSkinHdr = m_refs[active].GeoAnimSkinHeader();
    if (!pSkinHdr) return;
    EXGeoBaseAnimSkin *pSkin = (EXGeoBaseAnimSkin *)pSkinHdr->m_Addr[0].m_pObject;
    EXGDiData *pGDi;
    if (!pSkin || !(pGDi = m_pActiveSkin->m_gdi.m_pData)) return;
    EXGeoFile *pFile = m_refs[active].m_pGeoFile;
    EXGeoHeader *pHdr = pFile ? pFile->m_pGeoHeader : 0;
    const EXGeoEntityHeader *pEntities = pHdr->GeoEntityHeader(0);
    const EXGeoSkinEntityRef *pRefs = pSkin->m_EntityGroups[0].Data();
    EXItemRender_SkinAnimSection *pBones = m_pBones;
    const u8 *pRemap = m_pActiveSkin->m_pRemap;
    int numSkinned = pSkin->m_EntityGroups[1].Size() + pSkin->m_EntityGroups[0].Size();
    int total = pSkin->m_EntityGroups[2].Size() + numSkinned;
    int gdi = 0;
    for (int i = 0; i < total; i++) {
        const EXGeoEntityHeader *pEntHdr = &pEntities[pRefs[i].m_EntityHash & 0xffffff];
        EXGeoBaseEntity *pEntity = pEntHdr ? (EXGeoBaseEntity *)pEntHdr->m_Addr[0].m_pObject : 0;
        if (!m_pActiveSkin->m_00.IsSet(i) && pEntity->HasCollision()) {
            u32 skinned;
            if (i < numSkinned) {
                t.SetTemporaryVars((EXPlacementItem *)m_pOwner, mtx, 0);
                skinned = 1;
            } else {
                u32 bone = pRefs[i].m_00[2];
                if (pRemap) bone = pRemap[bone];
                EXMaths::mat_44 m;
                D3DXMatrixMultiply(&m, pBones[bone].m_pMtx, &mtx);
                t.SetTemporaryVars((EXPlacementItem *)m_pOwner, *(const EXMatrix *)&m, 0);
                skinned = 0;
            }
            pEntity->DoEntityScoop_Sphere(*(EXCScoopTemp *)&t, &pGDi[gdi], skinned, flags);
        }
        gdi += pEntity->GDiCount();
    }
}

// Gather the visible pieces of the drawn skin (optionally only entity m_54) for random vertex
// picking: skins in the item frame, bone entities in their section frames.
void EXItemRender_SkinAnim::BuildEntityInfoArray(const EXMatrix &mtx, EXGeoEntity_InfoArray &arr) const
{
    EXItemRender_SkinInfo *pInfo = m_pSkins[3];
    arr.SetMatrix(mtx);
    EXGeoAnimSkinHeader *pSkinHdr = pInfo->m_drawSkinRef.GeoAnimSkinHeader();
    if (!pSkinHdr) return;
    EXGeoBaseAnimSkin *pSkin = (EXGeoBaseAnimSkin *)pSkinHdr->m_Addr[0].m_pObject;
    if (!pSkin) return;
    EXGeoFile *pFile = pInfo->m_drawSkinRef.m_pGeoFile;
    EXGeoHeader *pHdr = pFile ? pFile->m_pGeoHeader : 0;
    const EXGeoSkinEntityRef *pRefs = pSkin->m_EntityGroups[0].Data();
    const EXGeoEntityHeader *pEntities = pHdr->EntityList().Find(0);
    EXGDiData *pGDi = pInfo->m_gdi.m_pData;
    u32 filter = arr.m_54;
    int numSkinned = pSkin->m_EntityGroups[1].Size() + pSkin->m_EntityGroups[0].Size();
    int gdi = 0;
    int i;
    for (i = 0; i < numSkinned; i++) {
        const EXGeoEntityHeader *pEntHdr = &pEntities[pRefs[i].m_EntityHash & 0xffffff];
        EXGeoBaseEntity *pEntity = pEntHdr ? (EXGeoBaseEntity *)pEntHdr->m_Addr[0].m_pObject : 0;
        if ((!filter || filter == pEntHdr->m_HashCode) && !pInfo->m_00.IsSet(i)) pEntity->BuildEntityInfoArray(arr, &pGDi[gdi]);
        gdi += pEntity->GDiCount();
    }
    if (pInfo->m_gdi.m_Size < gdi) for (;;) {}
    EXItemRender_SkinAnimSection *pBones = m_pBones;
    const u8 *pRemap = pInfo->m_pRemap;
    const EXGeoSkinEntityRef *pEntityRefs = pSkin->m_EntityGroups[2].Data();
    int n = pSkin->m_EntityGroups[2].Size();
    for (int k = 0; k < n; k++, i++) {
        const EXGeoEntityHeader *pEntHdr = &pEntities[pEntityRefs[k].m_EntityHash & 0xffffff];
        EXGeoBaseEntity *pEntity = pEntHdr ? (EXGeoBaseEntity *)pEntHdr->m_Addr[0].m_pObject : 0;
        if ((!filter || filter == pEntHdr->m_HashCode) && !pInfo->m_00.IsSet(i)) {
            u32 bone = pEntityRefs[k].m_00[2];
            if (pRemap) bone = pRemap[bone];
            EXMaths::mat_44 m;
            D3DXMatrixMultiply(&m, pBones[bone].m_pMtx, &mtx);
            arr.SetMatrix(*(const EXMatrix *)&m);
            pEntity->BuildEntityInfoArray(arr, &pGDi[gdi]);
        }
        gdi += pEntity->GDiCount();
    }
    if (pInfo->m_gdi.m_Size < gdi) for (;;) {}
}

// The section whose matrix origin is nearest p (within 1000 units). Flags 0x20: count the skin's
// bones rather than the skeleton's.
EXItemRender_SkinAnimSection *EXItemRender_SkinAnim::GetAnimBoneSection(const EXVector &p, u32 flags) const
{
    const EXGeoComplexRefObject &ref = (flags & 0x20) ? m_refs[3] : m_skelRef;
    EXGeoAnimSkinHeader *pSkinHdr = ref.GeoAnimSkinHeader();
    if (!pSkinHdr || !pSkinHdr->m_Addr[0].m_pObject) return 0;
    u32 n = ((EXGeoBaseAnimSkin *)pSkinHdr->m_Addr[0].m_pObject)->BoneCount();
    EXItemRender_SkinAnimSection *pBest = 0;
    float best = 1000000.0f;
    for (u32 i = 0; i < n; i++) {
        const EXMatrix *m = m_pBones[i].m_pMtx;
        float dx = m->r[3].x - p.x;
        float dy = m->r[3].y - p.y;
        float dz = m->r[3].z - p.z;
        float d = dy * dy + dz * dz + dx * dx;
        if (d < best) {
            pBest = &m_pBones[i];
            best = d;
        }
    }
    return pBest;
}

extern EXFixedArray<EXRotPosInfo, 2> PDBaseInfo;   // EXItemAnimator_Anim.obj blend stacks

// Copy the blended pose (PDBaseInfo[0]) into the sections (only the active LOD's skin parts at LODs
// with flag 0x80) and the morph weights.
void EXItemRender_SkinAnim::UpdateRenderEnd()
{
    EXItemRender_SkinAnimSection *pBones = m_pBones;
    if (!pBones) return;
    const EXMotionData *pMotion = PDBaseInfo[0].m_pMotion;
    int n = m_numBones;
    const EXAnimLodDetails *pLod = m_pActiveLod;
    if (!pLod || !(pLod->m_flags & 0x80)) {
        for (int i = 0; i < n; i++) {
            pBones[i].m_pos.x = pMotion[i].m_pos.x;
            pBones[i].m_pos.y = pMotion[i].m_pos.y;
            pBones[i].m_pos.z = pMotion[i].m_pos.z;
            pBones[i].m_pos.w = pMotion[i].m_pos.w;
            pBones[i].m_rot.x = pMotion[i].m_rot.x;
            pBones[i].m_rot.y = pMotion[i].m_rot.y;
            pBones[i].m_rot.z = pMotion[i].m_rot.z;
            pBones[i].m_rot.w = pMotion[i].m_rot.w;
        }
    } else {
        for (int i = 0; i < n; i++) {
            if (!pLod->m_skinBits.IsSet(i)) continue;
            pBones[i].m_pos.x = pMotion[i].m_pos.x;
            pBones[i].m_pos.y = pMotion[i].m_pos.y;
            pBones[i].m_pos.z = pMotion[i].m_pos.z;
            pBones[i].m_pos.w = pMotion[i].m_pos.w;
            pBones[i].m_rot.x = pMotion[i].m_rot.x;
            pBones[i].m_rot.y = pMotion[i].m_rot.y;
            pBones[i].m_rot.z = pMotion[i].m_rot.z;
            pBones[i].m_rot.w = pMotion[i].m_rot.w;
        }
    }
    u32 morphs = m_numMorphs;
    if (morphs) {
        const float *pSrc = PDBaseInfo[0].m_pMorph;
        float *pDst = (float *)m_pMorphs;
        for (; morphs; morphs--) *pDst++ = *pSrc++;
    }
    m_10 = 0;
    m_0e = 0;
}

// Blend the decoded morph frames (interpolated by the frame fraction) into the stack's deformer
// weights by the controller blend, for the morph lists the anims animate.
void EXItemRender_SkinAnim::BuildHierData_Morph(const EXRotPosInfo *pInfo, EXItemRender_SkinAnimInfo *pAnim,
                                                const EXGeoAnimSkin *pSkin, s8)
{
    if (!pSkin) {
        EXGeoAnimSkinHeader *pSkinHdr = m_skelRef.GeoAnimSkinHeader();
        if (!pSkinHdr) return;
        pSkin = (const EXGeoAnimSkin *)pSkinHdr->m_Addr[0].m_pObject;
        if (!pSkin || !pSkin->DefomerCount()) return;
    }
    float frac = pAnim->m_frac[0];
    float keep = 1.0f - pAnim->m_blend;
    u32 mask = 0xffffffff;
    const float *pFrame[2];
    int count = frac <= 0.0f ? 1 : 2;
    for (int k = 0; k < count; k++) {
        const EXGeoBaseAnim *pGeoAnim = pAnim->m_pMorphAnim[0][k];
        mask &= pGeoAnim->m_3c;
        pFrame[k] = (const float *)((const u8 *)pAnim->m_pMorphFrame[0][k] + (pGeoAnim->m_15 + 5 + pGeoAnim->m_14) * 16);
    }
    float *pOut = pInfo->m_pMorph;
    if (!pOut) return;
    const EXGeoAnimSkin_MorphList *pLists = pSkin->MorphList();
    int off = 0;
    for (u32 l = 0; l < pSkin->MorphCount(); l++) {
        u32 n = pLists[l].m_DeformerCount;
        if (!(mask & (1 << l))) {
            off += n;
            continue;
        }
        EXItemCtrlStack::ms_DeformerUsedBits.Set(off, off + n);
        for (u32 j = 0; j < n; j++, off++) {
            float v = pFrame[0][off];
            if (frac != 0.0f) v = (pFrame[1][off] - v) * frac + v;
            if (keep != 0.0f) v = (pOut[off] - v) * keep + v;
            pOut[off] = v;
        }
    }
}

// Two-motion form of BuildHierData_Morph: each motion's morph frames interpolated by its frame
// fraction, the motions blended by the frame depth, then into the stack by the controller blend.
void EXItemRender_SkinAnim::BuildHierData2_Morph(const EXRotPosInfo *pInfo, EXItemRender_SkinAnimInfo *pAnim)
{
    EXGeoAnimSkinHeader *pSkinHdr = m_skelRef.GeoAnimSkinHeader();
    if (!pSkinHdr) return;
    const EXGeoAnimSkin *pSkin = (const EXGeoAnimSkin *)pSkinHdr->m_Addr[0].m_pObject;
    if (!pSkin || !pSkin->DefomerCount()) return;
    u32 mask = 0xffffffff;
    const float *pFrame[2][2];
    for (int m = 0; m < 2; m++) {
        int count = pAnim->m_frac[m] <= 0.0f ? 1 : 2;
        for (int k = 0; k < count; k++) {
            const EXGeoBaseAnim *pGeoAnim = pAnim->m_pMorphAnim[m][k];
            mask &= pGeoAnim->m_3c;
            pFrame[m][k] = (const float *)((const u8 *)pAnim->m_pMorphFrame[m][k] + (pGeoAnim->m_14 + 5 + pGeoAnim->m_15) * 16);
        }
        if (count == 1) pFrame[m][1] = pFrame[m][0];
    }
    float *pOut = pInfo->m_pMorph;
    if (!pOut) return;
    float keep = 1.0f - pAnim->m_blend;
    float depth = pAnim->m_depth;
    const EXGeoAnimSkin_MorphList *pLists = pSkin->MorphList();
    int off = 0;
    for (u32 l = 0; l < pSkin->MorphCount(); l++) {
        u32 n = pLists[l].m_DeformerCount;
        if (!((1 << l) & mask)) {
            off += n;
            continue;
        }
        EXItemCtrlStack::ms_DeformerUsedBits.Set(off, n + off);
        for (u32 j = 0; j < n; j++, off++) {
            float v = pFrame[0][0][off];
            if (pAnim->m_frac[0] != 0.0f) v = (pFrame[0][1][off] - v) * pAnim->m_frac[0] + v;
            if (depth != 0.0f) {
                float w = pFrame[1][0][off];
                if (pAnim->m_frac[1] != 0.0f) w = (pFrame[1][1][off] - w) * pAnim->m_frac[1] + w;
                v = (w - v) * depth + v;
            }
            if (keep != 0.0f) v = (pOut[off] - v) * keep + v;
            pOut[off] = v;
        }
    }
}

// Per-bone data of a decoded frame: rotations from +0x40, then the positions of the bones whose
// position is animated (EXGeoBaseAnim::m_RootMask).
static inline const EXMaths::quat_xyzw *FrameBoneRot(const EXGeoSkinAnimFrame *pFrame)
{
    return (const EXMaths::quat_xyzw *)((const u8 *)pFrame + 0x40);
}
static inline const EXMaths::vec_xyzw *FrameBonePos(const EXGeoSkinAnimFrame *pFrame, int numBones)
{
    return (const EXMaths::vec_xyzw *)((const u8 *)pFrame + (numBones + 4) * 16);
}

// Build the stack's pose from one motion: per bone (in bits) the frame pair's rotation slerped by
// the frame fraction, and the position (rest position for bones not animated; the root's height
// rescaled from the anim's skin to the skeleton), each blended over the stack by the controller
// blend. Bones outside bits get no rotation.
s8 EXItemRender_SkinAnim::BuildHierData_Motion(const EXRotPosInfo *pInfo, EXItemRender_SkinAnimInfo *pAnim, ex::EXBit128 bits)
{
    EXGeoAnimSkinHeader *pHdr = pAnim->m_pAnimSkin->GeoAnimSkinHeader();
    if (!pHdr) return 0;
    const EXGeoBaseAnimSkin *pAnimSkin = (const EXGeoBaseAnimSkin *)pHdr->m_Addr[0].m_pObject;
    if (!pAnimSkin) return 0;
    pHdr = m_skelRef.GeoAnimSkinHeader();
    if (!pHdr) return 0;
    const EXGeoBaseAnimSkin *pSkel = (const EXGeoBaseAnimSkin *)pHdr->m_Addr[0].m_pObject;
    if (!pSkel) return 0;
    const EXGeoBaseAnim *pGeoAnim = pAnim->m_pAnim[0][0];
    u32 n = pGeoAnim->m_14;
    if (pSkel->BoneCount() < n) n = pSkel->BoneCount();
    if (n) {
        float frac = pAnim->m_frac[0];
        EXMotionData *pOut = pInfo->m_pMotion;
        float keep = 1.0f - pAnim->m_blend;
        const EXMaths::quat_xyzw *pRot0 = FrameBoneRot(pAnim->m_pFrame[0][0]);
        const EXMaths::quat_xyzw *pRot1 = FrameBoneRot(pAnim->m_pFrame[0][1]);
        for (int i = 0; i < (int)n; i++) {
            EXMaths::quat_xyzw q;
            if (!bits.IsSet(i)) {
                q.x = 0.0f;
                q.y = 0.0f;
                q.z = 0.0f;
                q.w = 1.0f;
            } else {
                q = pRot0[i];
                if (frac != 0.0f) q = q.slerp(pRot1[i], frac);
                if (keep != 0.0f) q = q.slerp(pOut[i].m_rot, keep);
            }
            pOut[i].m_rot.x = q.x;
            pOut[i].m_rot.y = q.y;
            pOut[i].m_rot.z = q.z;
            pOut[i].m_rot.w = q.w;
        }
        const EXMaths::vec_xyzw *pPos0 = FrameBonePos(pAnim->m_pFrame[0][0], n);
        const EXMaths::vec_xyzw *pPos1 = FrameBonePos(pAnim->m_pFrame[0][1], n);
        ex::EXBit128 posMask;
        posMask.Set(pGeoAnim->m_RootMask);
        const EXVector *pSkelRest = pSkel->RotDataRel();
        const EXVector *pAnimRest = pAnimSkin->RotDataRel();
        float scale = 1.0f;
        if (pAnimRest[0].y != 0.0f) scale = pSkelRest[0].y / pAnimRest[0].y;
        int k = 0;
        for (int i = 0; i < (int)n; i++) {
            EXMaths::vec_xyzw p;
            if (posMask.IsSet(i) && bits.IsSet(i)) {
                p = pPos0[k];
                if (frac != 0.0f) p = p.Interp(pPos1[k], frac);
                if (i == 0) p.y = p.y * scale;
                k++;
            } else {
                if (posMask.IsSet(i)) k++;
                p = pSkelRest[i];
            }
            if (keep != 0.0f) p = p.Interp(pOut[i].m_pos, keep);
            pOut[i].m_pos.x = p.x;
            pOut[i].m_pos.y = p.y;
            pOut[i].m_pos.z = p.z;
            pOut[i].m_pos.w = p.w;
        }
        EXItemCtrlStack::ms_BoneUsedBits |= bits;
    }
    if (pSkel->DefomerCount()) BuildHierData_Morph(pInfo, pAnim, (const EXGeoAnimSkin *)pSkel, 0);
    return 1;
}

// Two-motion form of BuildHierData_Motion: each motion's frame pair interpolated by its fraction,
// the two motions blended by the frame depth, then into the stack by the controller blend. Bones
// not animated by a motion take the anim skin's rest position for it.
s8 EXItemRender_SkinAnim::BuildHierData2_Motion(const EXRotPosInfo *pInfo, EXItemRender_SkinAnimInfo *pAnim, ex::EXBit128 bits)
{
    EXGeoAnimSkinHeader *pHdr = pAnim->m_pAnimSkin->GeoAnimSkinHeader();
    if (!pHdr) return 0;
    const EXGeoBaseAnimSkin *pAnimSkin = (const EXGeoBaseAnimSkin *)pHdr->m_Addr[0].m_pObject;
    if (!pAnimSkin) return 0;
    int n = pAnim->m_pAnim[0][0]->m_14;
    if (n) {
        float keep = 1.0f - pAnim->m_blend;
        float depth = pAnim->m_depth;
        EXMotionData *pOut = pInfo->m_pMotion;
        const EXMaths::quat_xyzw *pRot00 = FrameBoneRot(pAnim->m_pFrame[0][0]);
        const EXMaths::quat_xyzw *pRot01 = FrameBoneRot(pAnim->m_pFrame[0][1]);
        const EXMaths::quat_xyzw *pRot10 = FrameBoneRot(pAnim->m_pFrame[1][0]);
        const EXMaths::quat_xyzw *pRot11 = FrameBoneRot(pAnim->m_pFrame[1][1]);
        for (int i = 0; i < n; i++) {
            EXMaths::quat_xyzw q;
            if (!bits.IsSet(i)) {
                q.x = 0.0f;
                q.y = 0.0f;
                q.z = 0.0f;
                q.w = 1.0f;
            } else {
                EXMaths::quat_xyzw qa = pAnim->m_frac[0] != 0.0f ? pRot00[i].slerp(pRot01[i], pAnim->m_frac[0]) : pRot00[i];
                EXMaths::quat_xyzw qb = pAnim->m_frac[1] != 0.0f ? pRot10[i].slerp(pRot11[i], pAnim->m_frac[1]) : pRot10[i];
                q = qa.slerp(qb, depth);
                if (keep != 0.0f) q = q.slerp(pOut[i].m_rot, keep);
            }
            pOut[i].m_rot.x = q.x;
            pOut[i].m_rot.y = q.y;
            pOut[i].m_rot.z = q.z;
            pOut[i].m_rot.w = q.w;
        }
        const EXMaths::vec_xyzw *pPos00 = FrameBonePos(pAnim->m_pFrame[0][0], n);
        const EXMaths::vec_xyzw *pPos01 = FrameBonePos(pAnim->m_pFrame[0][1], n);
        const EXMaths::vec_xyzw *pPos10 = FrameBonePos(pAnim->m_pFrame[1][0], n);
        const EXMaths::vec_xyzw *pPos11 = FrameBonePos(pAnim->m_pFrame[1][1], n);
        const EXVector *pRest = pAnimSkin->RotDataRel();
        ex::EXBit128 mask0, mask1;
        mask0.Set(pAnim->m_pAnim[0][0]->m_RootMask);
        mask1.Set(pAnim->m_pAnim[1][0]->m_RootMask);
        int k0 = 0, k1 = 0;
        for (int i = 0; i < n; i++) {
            EXMaths::vec_xyzw p;
            bool m0 = mask0.IsSet(i) != 0;
            bool m1 = mask1.IsSet(i) != 0;
            if (!m0 && !m1)
                p = pRest[i];
            else {
                EXMaths::vec_xyzw pa, pb;
                if (m0 && bits.IsSet(i)) {
                    pa = pPos00[k0];
                    if (pAnim->m_frac[0] != 0.0f) pa = pa.Interp(pPos01[k0], pAnim->m_frac[0]);
                } else
                    pa = pRest[i];
                if (m0) k0++;
                if (m1 && bits.IsSet(i)) {
                    pb = pPos10[k1];
                    if (pAnim->m_frac[1] != 0.0f) pb = pb.Interp(pPos11[k1], pAnim->m_frac[1]);
                } else
                    pb = pRest[i];
                if (m1) k1++;
                p = pa.Interp(pb, depth);
            }
            if (keep != 0.0f) p = p.Interp(pOut[i].m_pos, keep);
            pOut[i].m_pos.x = p.x;
            pOut[i].m_pos.y = p.y;
            pOut[i].m_pos.z = p.z;
            pOut[i].m_pos.w = p.w;
        }
        EXItemCtrlStack::ms_BoneUsedBits |= bits;
    }
    BuildHierData2_Morph(pInfo, pAnim);
    return 1;
}

// Draw the active skin: the skinned pieces (split entities draw each sub piece) with the skin
// matrices, then the rigid bone-attached entities with their section matrices. Skins with a fade
// distance fade out over the last stretch before their LOD switch distance.
void EXItemRender_SkinAnim::RenderSkin(const EXMatrix &mtx)
{
    EXItemRender_SkinInfo *pInfo = m_pActiveSkin;
    EXGeoAnimSkinHeader *pSkinHdr = pInfo->m_drawSkinRef.GeoAnimSkinHeader();
    if (!pSkinHdr) return;
    EXGeoBaseAnimSkin *pSkin = (EXGeoBaseAnimSkin *)pSkinHdr->m_Addr[0].m_pObject;
    if (!pSkin) return;
    EXGDiData *pGDi = pInfo->m_gdi.m_pData;
    if (!pGDi) return;
    float fadeDist = *(const float *)&pSkinHdr->m_1c[0];
    s8 fade;
    if (fadeDist > 0.0f) {
        fade = 1;
        const EXBoundsBox &b = pSkin->GetBoundsBox();
        EXMaths::vec_xyzw size;
        size.x = b.m_vctMax.x - b.m_vctMin.x;
        size.y = b.m_vctMax.y - b.m_vctMin.y;
        size.z = b.m_vctMax.z - b.m_vctMin.z;
        size.w = b.m_vctMax.w - b.m_vctMin.w;
        float l = size.z * size.z + size.w * size.w + size.y * size.y + size.x * size.x;
        float r = (l >= 0.0f ? (float)sqrt(l) : 0.0f) * 0.5f;
        if (m_lodDist > fadeDist - r) {
            float end = pSkinHdr->m_LodDistance - r;
            EXBaseRenderEnv::SetDistanceFade(fadeDist >= end ? end : fadeDist, end, pSkinHdr->m_1c[4]);
        }
    } else
        fade = 0;
    EXGeoFile *pFile = pInfo->m_drawSkinRef.m_pGeoFile;
    const EXGeoEntityHeader *pEntities = (pFile ? pFile->m_pGeoHeader : 0)->GeoEntityHeader(0);
    const EXGeoSkinEntityRef *pRefs = pSkin->m_EntityGroups[0].Data();
    EXGDiSkinData *pSkinData = pInfo->m_gdi.m_pSkinData;
    int numSkinned = pSkin->m_EntityGroups[0].Size() + pSkin->m_EntityGroups[1].Size();
    int gdi = 0;
    int i;
    for (i = 0; i < numSkinned; i++) {
        const EXGeoEntityHeader *pEntHdr = &pEntities[pRefs[i].m_EntityHash & 0xffffff];
        EXGeoBaseEntity *pEntity = pEntHdr ? (EXGeoBaseEntity *)pEntHdr->m_Addr[0].m_pObject : 0;
        if (!pInfo->m_00.IsSet(i)) {
            s8 split = pEntity->IsKindOf(EXRUNTIME_CLASS(EXGeoSplitEntity));
            EXGDiSkinData *pSD = (EXGDiSkinData *)((u8 *)pSkinData + gdi * 0x14);
            EXGDiData *pData = (EXGDiData *)((u8 *)pGDi + gdi * 0x68);
            const ex::EXRelPtr<EXGeoBaseEntity> *pSub = (const ex::EXRelPtr<EXGeoBaseEntity> *)((const u8 *)pEntity + 0x5c);
            for (u32 p = 0; p < pRefs[i].m_00[1]; p++, pSub++) {
                EXGeoBaseEntity *pDraw = split ? (EXGeoBaseEntity *)*pSub : pEntity;
                EXGeoBaseEntity::ms_pGDiSkinData = pSkinData ? pSD : 0;
                pDraw->Draw(pData, 1);
                pSD = (EXGDiSkinData *)((u8 *)pSD + 0x14);
                pData = (EXGDiData *)((u8 *)pData + 0x68);
            }
        }
        gdi += pEntity->GDiCount();
    }
    EXBaseDisplay *pDisplay = EXBaseDisplay::GetDisplay();
    EXGeoBaseEntity::ms_pGDiSkinData = 0;
    int n = pSkin->m_EntityGroups[2].Size();
    if (n) {
        const u8 *pRemap = pInfo->m_pRemap;
        EXItemRender_SkinAnimSection *pBones = m_pBones;
        const EXGeoSkinEntityRef *pEntityRefs = pSkin->m_EntityGroups[2].Data();
        for (int k = 0; k < n; k++, i++) {
            const EXGeoEntityHeader *pEntHdr = &pEntities[pEntityRefs[k].m_EntityHash & 0xffffff];
            EXGeoBaseEntity *pEntity = pEntHdr ? (EXGeoBaseEntity *)pEntHdr->m_Addr[0].m_pObject : 0;
            if (!pInfo->m_00.IsSet(i)) {
                u32 bone = pEntityRefs[k].m_00[2];
                if (pRemap) bone = pRemap[bone];
                EXMaths::mat_44 m;
                D3DXMatrixMultiply(&m, pBones[bone].m_pMtx, &mtx);
                EXBaseDisplay::SetObjectToWorld(m);
                pDisplay->UpdateTransforms();
                pEntity->Draw((EXGDiData *)((u8 *)pGDi + gdi * 0x68), 0);
            }
            gdi += pEntity->GDiCount();
        }
        if (m_pClone) {
            EXBaseDisplay::SetObjectToWorld(mtx);
            pDisplay->UpdateTransforms();
        }
    }
    if (fade) EXBaseRenderEnv::ClearDistanceFade();
}
