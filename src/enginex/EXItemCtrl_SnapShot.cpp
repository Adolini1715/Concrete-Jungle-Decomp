#include "EXItemCtrl_SnapShot.h"
#include "EXItemAnimator_Anim.h"
#include "EXStdFuncs.h"
#include "EXMemory.h"

void EXItemCtrl_SnapShot::Clone(const EXItemCtrl_SnapShot *p)
{
    m_numBones = p->m_numBones;
    m_numMorphs = p->m_numMorphs;
    m_numDatums = p->m_numDatums;
    u32 size = DataSize();
    if (size) {
        m_pMotionData = (EXMotionData *)_EXAlloc(size, 0);
        if (!m_pMotionData) for (;;) {}
        m_pMorph = (float *)(m_pMotionData + m_numBones);
        m_pDatums = (EXSnapShotDatum *)(m_pMorph + m_numMorphs);
        memcpy(m_pMotionData, p->m_pMotionData, size);
    }
}

s8 EXItemCtrl_SnapShot::UpdateCtrlOnStack(EXItemAnimator_Anim *pAnim, EXItemCtrlStack *, EXRotPosInfo *pInfo, int)
{
    EXItemRender_SkinAnim *pSkin = pAnim->m_pSkin;
    if (!pSkin) return 0;
    EXMotionData *pSrc = m_pMotionData;
    if (!pSrc) return 0;
    memcpy(pInfo->m_pMotion, pSrc, pSkin->m_numBones * sizeof(EXMotionData));
    memcpy(pInfo->m_pMorph, m_pMorph, pSkin->m_numMorphs * sizeof(float));
    pInfo->m_bits.SetAll();
    return 1;
}

EXItemCtrl_SnapShot::EXItemCtrl_SnapShot() : EXItemCtrl(0xffffffff)
{
    m_pMotionData = 0;
    m_pDatums = 0;
    m_numBones = 0;
    m_numMorphs = 0;
    m_numDatums = 0;
}

EXItemCtrl_SnapShot::~EXItemCtrl_SnapShot()
{
    if (m_pMotionData) EXFree(m_pMotionData);
}

// NEAR: 4 bytes, ecx/edx swapped in the DataSize() arithmetic.
s8 EXItemCtrl_SnapShot::Initialise(EXItemAnimator_Anim *pAnim)
{
    EXItemRender_SkinAnim *pSkin = pAnim->m_pSkin;
    if (!pSkin) return 0;
    EXu32Array *pIds = pAnim->m_pSnapShotDatums;
    m_numBones = (u16)pSkin->m_numBones;
    m_numMorphs = (u16)pSkin->m_numMorphs;
    m_numDatums = (u16)(pIds ? pIds->m_nSize : 0);
    m_pMotionData = (EXMotionData *)_EXAlloc(DataSize(), 0);
    if (!m_pMotionData) for (;;) {}
    m_pMorph = (float *)(m_pMotionData + m_numBones);
    m_pDatums = (EXSnapShotDatum *)(m_pMorph + m_numMorphs);
    return 1;
}

void EXItemCtrl_SnapShot::Create(EXItemAnimator_Anim *pAnim)
{
    pAnim->DoUpdate();
    if (!Initialise(pAnim)) return;
    EXItemRender_SkinAnimSection *pBone = pAnim->m_pSkin->m_pBones;
    EXMotionData *pMotion = m_pMotionData;
    int i;
    for (i = 0; i < m_numBones; i++) {
        pMotion[i].m_pos = pBone[i].m_pos;
        pMotion[i].m_rot = pBone[i].m_rot;
    }
    float *pMorph = m_pMorph;
    for (i = 0; i < m_numMorphs; i++) pMorph[i] = 0.0f;
    EXSnapShotDatum *pDatum = m_pDatums;
    EXu32Array *pIds = pAnim->m_pSnapShotDatums;
    for (i = 0; i < m_numDatums; i++) {
        u32 id = pIds->m_pData[i];
        if (!pAnim->GetFrameDatum(id, m_pDatums[i].m_datum, 1)) id = 0xffffffff;
        pDatum[i].m_id = id;
    }
}

void EXItemCtrl_SnapShot::Create(EXItemAnimator_Anim *pAnim, EXItemCtrlStack *pStack, const EXItemCtrl *pLast)
{
    if (!Initialise(pAnim)) return;
    EXRotPosInfo info;
    info.m_pMotion = m_pMotionData;
    info.m_pMorph = m_pMorph;
    info.m_bits.SetAll();
    int n = 0;
    EXDListItem *pNode = pStack->m_list.Tail();
    EXItemCtrl *pCtrl = pNode ? (EXItemCtrl *)((char *)pNode - 4) : 0;
    while (pCtrl) {
        if (pCtrl->UpdateCtrlOnStack(pAnim, pStack, &info, n)) n++;
        if (pCtrl == pLast) break;
        pNode = pCtrl->m_pPrev;
        pCtrl = pNode ? (EXItemCtrl *)((char *)pNode - 4) : 0;
    }
    int i;
    if (!n) {
        for (i = 0; i < m_numBones; i++) {
            m_pMotionData[i].m_pos.SetZero();
            m_pMotionData[i].m_rot.set_zero1();
        }
        memset(m_pMorph, 0, m_numMorphs * sizeof(float));
    }
    EXSnapShotDatum *pDatum = m_pDatums;
    EXu32Array *pIds = pAnim->m_pSnapShotDatums;
    for (i = 0; i < m_numDatums; i++) {
        u32 id = pIds->m_pData[i];
        if (!pAnim->GetFrameDatum_CtrlStack(pStack, id, m_pDatums[i].m_datum, 1)) id = 0xffffffff;
        pDatum[i].m_id = id;
    }
}

EXItemCtrl_SnapShot::EXItemCtrl_SnapShot(EXItemAnimator_Anim *pAnim) : EXItemCtrl(0xffffffff)
{
    m_pMotionData = 0;
    m_pMorph = 0;
    m_pDatums = 0;
    m_numBones = 0;
    m_numMorphs = 0;
    m_numDatums = 0;
    Create(pAnim);
}

EXItemCtrl_SnapShot::EXItemCtrl_SnapShot(EXItemAnimator_Anim *pAnim, EXItemCtrlStack *pStack) : EXItemCtrl(0xffffffff)
{
    m_pMotionData = 0;
    m_pMorph = 0;
    m_pDatums = 0;
    m_numBones = 0;
    m_numMorphs = 0;
    m_numDatums = 0;
    Create(pAnim, pStack, 0);
}

void *EXItemCtrl_SnapShot::GetFrameDatum(EXItemAnimator_Anim *, u32 id, EXDatumTemp &d, u32)
{
    for (int i = 0; i < m_numDatums; i++) {
        if (m_pDatums[i].m_id == id) {
            d = m_pDatums[i].m_datum;
            return &m_pDatums[i];
        }
    }
    return 0;
}


// Hamilton product a * b (textbook form; the quaternion multiply inline, not declared in EXMaths.h).
static inline EXMaths::quat_xyzw QuatMul(const EXMaths::quat_xyzw &a, const EXMaths::quat_xyzw &b)
{
    EXMaths::quat_xyzw r;
    r.x = a.x * b.w + a.w * b.x + a.y * b.z - a.z * b.y;
    r.y = a.y * b.w + a.w * b.y + a.z * b.x - a.x * b.z;
    r.z = a.z * b.w + a.w * b.z + a.x * b.y - a.y * b.x;
    r.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
    return r;
}

void EXItemCtrl_SnapShot::ApplyXForm(const EXVector &pos, const EXQuaternion &rot)
{
    EXMotionData *p = m_pMotionData;
    if (!p) return;
    p->m_pos = pos + p->m_pos;
    p->m_rot = QuatMul(rot, p->m_rot);
}

u8 EXItemCtrl_SnapShot::CalcMotionData(EXMotionData *pData, const EXCalcMotionTemp &t)
{
    int n = (t.m_flags & 0x80) ? 1 : 2;
    for (int i = 0; i < n; i++) {
        pData[i].m_pos = m_pMotionData->m_pos;
        pData[i].m_rot = m_pMotionData->m_rot;
    }
    return 4;
}
