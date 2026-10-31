#include <math.h>
#include "EXItemAnimator_AnimModifier.h"
#include "EXItemCtrl_AnimModifier.h"
#include "EXGeoFile.h"
#include "EXMemory.h"

EX_IMPLEMENT_DYNCREATE_ROOT(EXSpineModifier)
EX_IMPLEMENT_DYNCREATE_ROOT(EXHeadTracking)

EXSpineModifier::~EXSpineModifier()
{
    if (m_numSteps) {
        EXFree(m_pSteps);
        EXFree(m_pTimes);
    }
}

s8 EXSpineModifier::OverRides() const
{
    if (0.001f > m_lastStep) return 0;
    if (m_stepDot > 0.998f) return 0;
    return 1;
}

EXHeadTracking::~EXHeadTracking()
{
    if (m_numBones) delete[] m_pBones;
}

void EXHeadTracking::SetNumBones(u32 n)
{
    if (!n) return;
    m_numBones = n;
    delete[] m_pBones;
    m_pBones = new EXHeadTrackingBone[m_numBones];
    m_a2 = 1;
    if (m_numBones < 5) {
        m_pBones[0].m_hash = 0xe000006;
        if (m_numBones > 2) m_pBones[1].m_hash = 0xe000007;
        if (m_numBones > 3) m_pBones[2].m_hash = 0xe000008;
        m_pBones[m_numBones - 1].m_hash = 0xe000009;
    } else if (m_numBones < 10) {
        for (u32 i = 0; i < m_numBones; i++) m_pBones[m_numBones - i - 1].m_hash = 0xe000009 - i;
    }
}

void EXHeadTracking::SetCustomMapping(const float *p)
{
    for (u32 i = 0; i < m_numBones; i++) m_pBones[i].m_weight = p[i];
    m_a1 = 0;
}

EX_IMPLEMENT_DYNCREATE(EXItemAnimator_AnimModifier, EXItemAnimator_Anim)

EXItemAnimator_AnimModifier::EXItemAnimator_AnimModifier()
{
    m_modifiers = 0;
    m_pSpine = 0;
    m_pHeadTracking = 0;
    m_pFlipInfo = 0;
    m_pModCtrl = 0;
    m_190 = 3;
}

s8 EXItemAnimator_AnimModifier::_GetBoundsBox(EXBoundsBox &box, u32 flags)
{
    return EXItemAnimator_Anim::_GetBoundsBox(box, flags);
}

EXItemCtrl *EXItemAnimator_AnimModifier::ConvertToAnimModifierMulti(u32 mask)
{
    EXItemCtrl_AnimModifier *p = new EXItemCtrl_AnimModifier;
    s8 bFirst = 1;
    for (int i = 0; i < m_numStacks; i++) {
        m_stacks[i].KillStack(0);
        if ((mask & (1 << i)) && bFirst) {
            bFirst = 0;
            m_stacks[i].m_list.AddHead(p);
        }
    }
    return p;
}

// NEAR: constant loads / member stores scheduled differently (same instructions).
EXSpineModifier::EXSpineModifier()
{
    m_44 = 0;
    m_45 = 0;
    m_numSteps = 0x80;
    m_pSteps = new EXFixedArray<EXVector, 128>;
    m_pTimes = new EXFixedArray<float, 128>;
    m_14 = 5;
    m_18 = 5;
    m_28 = 0.1f;
    m_3c = 0.7f;
    m_24 = 0.0f;
    m_20 = 0.0f;
    m_curStep = 1;
    m_46 = 0;
    m_47 = 0;
    m_34 = 0.0f;
    m_38 = 1.0f;
    m_40 = 1.7f;
    m_stepLength = 1.7f;
    float t = 1.87f / m_numSteps;
    for (u32 i = 0; i < m_numSteps; i++) {
        (*m_pSteps)[i].Set(0.0f, 0.0f, 1.0f, 0.0f);
        (*m_pTimes)[i] = t;
    }
}

// NEAR: retail finishes the x87 threshold (u32 count conversion in its own slot) before the dot product.
void EXSpineModifier::NewStep(const EXVector &dir, float t)
{
    u32 n = m_numSteps;
    const float &thresh = m_stepLength * 0.25f / n;
    const EXVector &s = (*m_pSteps)[m_curStep];
    m_stepDot = s.x * dir.x + s.y * dir.y + s.z * dir.z + s.w * dir.w;
    if (t > thresh) {
        if (++m_curStep >= n) m_curStep = 0;
        (*m_pSteps)[m_curStep] = dir;
        (*m_pTimes)[m_curStep] = t;
        m_lastStep = t;
    } else {
        m_lastStep = 0.0f;
    }
}

// NEAR: last eight member stores scheduled in a different order.
EXHeadTracking::EXHeadTracking()
{
    m_forwards.Set(0.0f, 0.0f, -1.0f, 0.0f);
    m_a0 = 0;
    m_14.Set(0.0f, 0.0f, -1.0f, 0.0f);
    m_64.Set(0.0f, 0.0f, -1.0f, 0.0f);
    m_mtx.SetIdentity();
    m_8c = 0;
    m_numBones = 1;
    m_pBones = new EXHeadTrackingBone[m_numBones];
    m_90 = 0x3c;
    m_a2 = 1;
    m_pBones[0].m_hash = 0xe000009;
    m_maxAngularMove = 0.1f;
    m_baseHash = 0xe000000;
    m_a1 = 1;
    m_7c = 1.0f;
    m_80 = 1.0f;
    m_84 = 2.0f;
    m_88 = 2.0f;
    m_a3 = 0;
    m_a4 = 0;
}

s8 EXItemAnimator_AnimModifier::Add6DModifier(EX6DAnimInfo &info)
{
    if (!m_pModCtrl) {
        m_pModCtrl = (EXItemCtrl_AnimModifier *)ConvertToAnimModifierMulti(1);
        m_pModCtrl->SetAnimMod(this);
    }
    m_pModCtrl->AddBody(info);
    m_b6D = 1;
    return 1;
}

s8 EXItemAnimator_AnimModifier::Set6DAnim(const EX6DAnim *)
{
    if (!m_pModCtrl) {
        m_pModCtrl = (EXItemCtrl_AnimModifier *)ConvertToAnimModifierMulti(1);
        m_pModCtrl->SetAnimMod(this);
    }
    m_b6D = 1;
    return 1;
}

void EXItemAnimator_AnimModifier::ResetPositions(EXItemCtrl_SnapShot *pSnap, EXItemCtrl_SnapShot *pVel, float t)
{
    if (!m_pModCtrl) {
        m_pModCtrl = (EXItemCtrl_AnimModifier *)ConvertToAnimModifierMulti(1);
        m_pModCtrl->SetAnimMod(this);
    }
    m_pModCtrl->ResetPositions(this, pSnap, pVel, t);
}

EXItemAnimator_AnimModifier *EXItemAnimator_AnimModifier::CreateModifierFrom(EXGeoFile *pFile, u32 hash)
{
    if (!pFile) return 0;
    if (!pFile->GeoHeader()) return 0;
    EXItemAnimator_AnimModifier *p = NewItem();
    if (p) {
        if (p->Initialise(pFile, hash)) return p;
        delete p;
    }
    return 0;
}

void EXItemAnimator_AnimModifier::SwapRootBone(const EXItemRender_SkinAnim *pSkin, u32 bone) const
{
    if (!pSkin) return;
    EXItemRender_SkinAnimSection *pSec = pSkin->GetAnimBoneSection(bone, 0);
    if (!pSec) return;
    EXQuaternion q = pSec->Rotation();
    q.y = -q.y;
    q.z = -q.z;
    pSec->SetRotation(q);
    EXVector p = pSec->Position();
    p.x = -p.x;
    pSec->SetPosition(p);
}

void EXItemAnimator_AnimModifier::SwapBone(const EXItemRender_SkinAnim *pSkin, u32 bone) const
{
    if (!pSkin) return;
    EXItemRender_SkinAnimSection *pSec = pSkin->GetAnimBoneSection(bone, 0);
    if (!pSec) return;
    EXQuaternion q = pSec->Rotation();
    q.y = -q.y;
    q.z = -q.z;
    pSec->SetRotation(q);
}

void EXItemAnimator_AnimModifier::SwapLeftRightBones(const EXItemRender_SkinAnim *pSkin, u32 a, u32 b) const
{
    if (!pSkin) return;
    EXItemRender_SkinAnimSection *pA = pSkin->GetAnimBoneSection(a, 0);
    EXItemRender_SkinAnimSection *pB = pSkin->GetAnimBoneSection(b, 0);
    if (!pA || !pB) return;
    EXQuaternion qa = pA->Rotation();
    qa.y = -qa.y;
    qa.z = -qa.z;
    EXQuaternion qb = pB->Rotation();
    qb.y = -qb.y;
    qb.z = -qb.z;
    pA->SetRotation(qb);
    pB->SetRotation(qa);
}

void EXItemAnimator_AnimModifier::Flip()
{
    EXFlipInfo *pInfo = m_pFlipInfo;
    if (!pInfo) return;
    int n = pInfo->CentreBones().Size();
    EXItemRender_SkinAnim *pSkin = m_pSkin;
    if (n > 0) SwapRootBone(pSkin, pInfo->CentreBones()[0]);
    int i;
    for (i = 1; i < n; i++) SwapBone(pSkin, m_pFlipInfo->CentreBones()[i]);
    int m = m_pFlipInfo->SwapBones().Size();
    for (i = 0; i < m; i++) {
        EXSwapBoneInfo &s = m_pFlipInfo->SwapBones()[i];
        SwapLeftRightBones(pSkin, s.m_a, s.m_b);
    }
}

void EXFlipInfo::AddRootBone(u32 bone)
{
    m_centreBones.InsertAt(0, bone, 1);
}

void EXFlipInfo::AddCentreBone(u32 bone)
{
    m_centreBones.Add(bone, 1);
}

void EXFlipInfo::AddSwapBone(u32 a, u32 b)
{
    EXSwapBoneInfo &s = m_swapBones.AddSpace(1);
    s.m_a = a;
    s.m_b = b;
}

void EXItemAnimator_AnimModifier::DoUpdate()
{
    EXItemRender_SkinAnim *pSkin = m_pSkin;
    EXItemAnimator_Anim::DoUpdate();
    s8 bOver = 0;
    if ((m_modifiers & 1) && m_pSpine) {
        m_pSpine->Update(pSkin);
        bOver = m_pSpine->OverRides();
    }
    if ((m_modifiers & 2) && m_pHeadTracking) m_pHeadTracking->Update(pSkin, bOver);
    if (m_modifiers & 4) Flip();
}

void EXItemAnimator_AnimModifier::CheckBone(u32 bone)
{
    if (!m_pSkin) return;
    EXItemRender_SkinAnimSection *pSec = m_pSkin->GetAnimBoneSection(bone, 0);
    if (!pSec) return;
    if (pSec->SectionIndex() == 0) m_pFlipInfo->AddRootBone(bone);
    else m_pFlipInfo->AddCentreBone(bone);
}

void EXItemAnimator_AnimModifier::CheckSwapBone(u32 a, u32 b)
{
    EXItemRender_SkinAnim *pSkin = m_pSkin;
    if (pSkin && pSkin->GetAnimBoneSection(a, 0) && pSkin->GetAnimBoneSection(b, 0)) m_pFlipInfo->AddSwapBone(a, b);
}

void EXItemAnimator_AnimModifier::RemoveModifier(u8 m)
{
    if (m & 8) m |= 4;
    if ((m & 1) && m_pSpine) {
        delete m_pSpine;
        m_pSpine = 0;
    }
    if ((m & 2) && m_pHeadTracking) {
        delete m_pHeadTracking;
        m_pHeadTracking = 0;
    }
    if ((m & 4) && m_pFlipInfo) {
        delete m_pFlipInfo;
        m_pFlipInfo = 0;
    }
    m_modifiers &= ~m;
}

EXItemAnimator_AnimModifier::~EXItemAnimator_AnimModifier()
{
    if (m_pSpine) delete m_pSpine;
    if (m_pHeadTracking) delete m_pHeadTracking;
    if (m_pFlipInfo) delete m_pFlipInfo;
}

void EXItemAnimator_AnimModifier::AddModifier(u8 m)
{
    if (m & 8) m |= 4;
    m_modifiers |= m;
    if ((m & 1) && !m_pSpine) m_pSpine = new EXSpineModifier;
    if ((m & 2) && !m_pHeadTracking) m_pHeadTracking = new EXHeadTracking;
    if ((m & 4) && !m_pFlipInfo) {
        m_pFlipInfo = new EXFlipInfo;
        if (m & 8) SetupAutoFlip();
    }
}

// Rotate a section by `angle` about an axis taken from the accumulated matrix (local `axis` in the
// section's frame); mtx accumulates the section's rotation first.
static __forceinline void RotateSectionAbout(EXItemRender_SkinAnimSection *pSec, float angle, EXMatrix &mtx, float ax, float ay,
                               float az, bool accumulate)
{
    EXMaths::mat_44 rot;
    EXMaths::mat_44 tmp;
    rot.set_rotate_quat(pSec->m_rot);
    D3DXMatrixMultiply(&tmp, &rot, &mtx);
    *(EXMaths::mat_44 *)&mtx = tmp;
    EXMaths::vec_xyzw v;
    v.x = ax;
    v.y = ay;
    v.z = az;
    v.w = 0.0f;
    EXMaths::mat_44 inv = mtx.fastinverse();
    v.w = 1.0f;
    EXMaths::vec_xyzw axis;
    D3DXVec4Transform(&axis, &v, &inv);
    EXMaths::vec_xyzw aa;
    aa.x = axis.x;
    aa.y = axis.y;
    aa.z = axis.z;
    aa.w = angle;
    EXMaths::mat_44 r;
    r.set_rotate_axis(aa);
    EXMaths::mat_44 cur;
    cur.set_rotate_quat(pSec->m_rot);
    D3DXMatrixMultiply(&tmp, &r, &cur);
    cur = tmp;
    if (accumulate) {
        D3DXMatrixMultiply(&tmp, &r, &mtx);
        *(EXMaths::mat_44 *)&mtx = tmp;
    }
    EXQuaternion q;
    q.SetRotation(*(const EXMatrix *)&cur);
    pSec->m_rot.x = q.x;
    pSec->m_rot.y = q.y;
    pSec->m_rot.z = q.z;
    pSec->m_rot.w = q.w;
}

// Bend a leg section about its local x axis.
void EXSpineModifier::AdjustLeg(EXItemRender_SkinAnimSection *pSec, float angle, EXMatrix &mtx) const
{
    RotateSectionAbout(pSec, angle, mtx, 1.0f, 0.0f, 0.0f, false);
}

// Bend a chain of spine sections about their local y axis, down to section n.
void EXSpineModifier::RecursivelyRotate(EXItemRender_SkinAnimSection *pSec, u32 n, float angle, EXMatrix &mtx)
{
    RotateSectionAbout(pSec, angle, mtx, 0.0f, 1.0f, 0.0f, true);
    if (pSec->m_bone < n && pSec->ChildLinks()) RecursivelyRotate(pSec->ChildLinks(), n, angle, mtx);
}

// Normalise with the 1e-5 length threshold these modifiers use.
static inline void NormaliseV(EXMaths::vec_xyzw &v)
{
    float l = v.y * v.y + v.w * v.w + v.z * v.z + v.x * v.x;
    if (l >= 0.0f) {
        l = (float)sqrt(l);
        if (l > 1e-5f) {
            float inv = 1.0f / l;
            v.x = inv * v.x;
            v.y = v.y * inv;
            v.z = v.z * inv;
            v.w = v.w * inv;
        }
    }
}

// Direction of travel averaged over the steps taken between `from` and `to` time units ago,
// and the signed angle (about y) from v to it, kept continuous through +-pi by `flipped` and
// limited to +-3pi/4. v is replaced by the averaged direction.
float EXSpineModifier::GetAngle(EXVector &v, float from, float to, s8 &flipped)
{
    u32 i = m_curStep;
    float *pTimes = &(*m_pTimes)[0];
    float t = pTimes[i];
    bool started = false;
    u32 n = 0;
    EXMaths::vec_xyzw acc;
    acc.x = 0.0f;
    acc.y = 0.0f;
    acc.z = 0.0f;
    acc.w = 0.0f;
    bool complete = true;
    while (t < to) {
        if (++n >= m_numSteps) {
            complete = false;
            break;
        }
        if (t > from) {
            const EXVector &step = (*m_pSteps)[i];
            float w;
            if (started)
                w = pTimes[i];
            else {
                w = ((t - from) / pTimes[i]) * pTimes[i];
                started = true;
            }
            acc.x = step.x * w + acc.x;
            acc.y = step.y * w + acc.y;
            acc.z = step.z * w + acc.z;
            acc.w = step.w * w + acc.w;
        }
        if (i == 0) i = m_numSteps;
        i--;
        t = pTimes[i] + t;
    }
    if (complete && n < m_numSteps) {
        const EXVector &step = (*m_pSteps)[i];
        float w = ((pTimes[i] - (t - to)) / pTimes[i]) * pTimes[i];
        acc.x = step.x * w + acc.x;
        acc.y = step.y * w + acc.y;
        acc.z = step.z * w + acc.z;
        acc.w = step.w * w + acc.w;
    }
    NormaliseV(acc);
    float dot = v.y * acc.y + v.w * acc.w + v.x * acc.x + v.z * acc.z;
    if (dot >= 1.0f) dot = 1.0f;
    else if (dot <= -1.0f) dot = -1.0f;
    float angle = EXACos(dot);
    if (dot > 0.99999f) angle = 0.0f;
    // y of v x acc
    float side = ((v.x * acc.y - v.y * acc.x) + 0.0f + (v.y * acc.z - v.z * acc.y)) * 0.0f + (v.z * acc.x - v.x * acc.z);
    if (side > 0.0f) angle = 0.0f - angle;
    if (angle >= 0.0f) {
        if (flipped) {
            if (angle < 1.5707964f) flipped = 0;
            else angle -= 6.2831855f;
        }
    } else if (!flipped) {
        if (angle > -1.5707964f) flipped = 1;
        else angle += 6.2831855f;
    }
    if (angle > 2.3561945f) angle = 2.3561945f;
    else if (angle < -2.3561945f) angle = -2.3561945f;
    v.x = acc.x;
    v.y = acc.y;
    v.z = acc.z;
    v.w = acc.w;
    return angle;
}

// Current yaw (b) and pitch (a) of the tracked direction, from m_14 and m_64 against the
// flattened m_14; flags 0x10 / 0x20 negate them.
void EXHeadTracking::ReturnAngles(float &a, float &b) const
{
    EXMaths::vec_xyzw flat;
    flat.x = m_14.x;
    flat.y = 0.0f;
    flat.z = m_14.z;
    flat.w = m_14.w;
    NormaliseV(flat);
    float d = m_14.w * flat.w + m_14.z * flat.z + m_14.y * flat.y + m_14.x * flat.x;
    b = EXACos(d);
    if (m_flags & 0x10) b = 0.0f - b;
    d = m_64.w * flat.w + m_64.z * flat.z + m_64.y * flat.y + m_64.x * flat.x;
    a = EXACos(d);
    if (m_flags & 0x20) a = 0.0f - a;
}

// Twist the spine towards the recent direction of travel: the hips / root by the near-term angle
// (unless m_47), the spine chain and the upper chain spread over m_18 / m_14 sections, and the
// legs counter-rotated when m_46. Skipped at LODs with flag 0x80.
void EXSpineModifier::Update(const EXItemRender_SkinAnim *pSkin)
{
    if (pSkin->m_pActiveLod && (s8)pSkin->m_pActiveLod->m_flags < 0) return;
    EXVector v = (*m_pSteps)[m_curStep];
    float a1 = GetAngle(v, m_34, m_38, *(s8 *)&m_44);
    if (m_47) a1 = 0.0f - a1;
    float lim = m_20 + m_28;
    if (a1 > lim || (lim = m_20 - m_28, a1 < lim)) a1 = lim;
    m_20 = a1;
    float a2 = -GetAngle(v, m_3c, m_40, *(s8 *)&m_45);
    lim = m_24 + m_28;
    if (a2 > lim || (lim = m_24 - m_28, a2 < lim)) a2 = lim;
    m_24 = a2;
    EXItemRender_SkinAnimSection *pRoot = pSkin->GetAnimBoneSection(0xe000001, 0);
    EXItemRender_SkinAnimSection *pSpine = pSkin->GetAnimBoneSection(0xe000002, 0);
    EXItemRender_SkinAnimSection *pUpper = pSkin->GetAnimBoneSection(0xe00001c, 0);
    if (!pRoot && !(pRoot = pSkin->GetAnimBoneSection(0xe000000, 0))) return;
    if (!pSpine || !pUpper) return;
    EXMatrix mtx;
    mtx.set_rotate_quat(pRoot->m_rot);
    if (!m_47) {
        EXMaths::vec_xyzw up;
        up.x = 0.0f;
        up.y = 1.0f;
        up.z = 0.0f;
        up.w = 0.0f;
        EXMaths::mat_44 inv = mtx.fastinverse();
        up.w = 1.0f;
        EXMaths::vec_xyzw axis;
        D3DXVec4Transform(&axis, &up, &inv);
        EXMaths::vec_xyzw aa;
        aa.x = axis.x;
        aa.y = axis.y;
        aa.z = axis.z;
        aa.w = 0.0f - a1;
        EXMaths::mat_44 r, tmp;
        r.set_rotate_axis(aa);
        D3DXMatrixMultiply(&tmp, &r, &mtx);
        *(EXMaths::mat_44 *)&mtx = tmp;
        EXQuaternion q;
        q.SetRotation(mtx);
        pRoot->m_rot.x = q.x;
        pRoot->m_rot.y = q.y;
        pRoot->m_rot.z = q.z;
        pRoot->m_rot.w = q.w;
    }
    EXMatrix saved = mtx;
    if (m_18) {
        a1 = a1 / (float)m_18;
        RecursivelyRotate(pSpine, (pSpine->m_bone - 1) + m_18, a1, mtx);
    }
    if (m_14) {
        mtx = saved;
        RecursivelyRotate(pUpper, (pUpper->m_bone - 1) + m_14, a2 / (float)m_14, saved);
    }
    if (m_46) {
        EXItemRender_SkinAnimSection *pLeg0 = pSkin->GetAnimBoneSection(0xe00000f, 0);
        EXItemRender_SkinAnimSection *pLeg1 = pSkin->GetAnimBoneSection(0xe000013, 0);
        EXItemRender_SkinAnimSection *pLeg2 = pSkin->GetAnimBoneSection(0xe000016, 0);
        EXItemRender_SkinAnimSection *pLeg3 = pSkin->GetAnimBoneSection(0xe000019, 0);
        AdjustLeg(pLeg0, 0.0f - a1, mtx);
        AdjustLeg(pLeg1, a1, mtx);
        AdjustLeg(pLeg2, a1 * 0.5f, mtx);
        AdjustLeg(pLeg3, a1 * -0.5f, mtx);
    }
}

// Walk the hierarchy from pSec accumulating matrices; each tracked bone turns by its weight times
// `angle` about axis (given in the base frame). When the last tracked bone is reached the
// accumulated matrix is kept in m_mtx.
void EXHeadTracking::FindAndRotate(EXItemRender_SkinAnimSection *pSec, float angle, EXVector &axis, EXMatrix mtx, u32 &n)
{
    if (!pSec) return;
    EXMaths::mat_44 local, tmp;
    local.set_rotate_quat(pSec->m_rot);
    local.r[3].x = pSec->m_pos.x;
    local.r[3].y = pSec->m_pos.y;
    local.r[3].z = pSec->m_pos.z;
    local.r[3].w = 1.0f;
    D3DXMatrixMultiply(&tmp, &local, &mtx);
    *(EXMaths::mat_44 *)&mtx = tmp;
    for (u32 i = 0; i < m_numBones; i++) {
        if (pSec->m_bone != m_pBones[i].m_08) continue;
        n++;
        EXMaths::vec_xyzw v;
        v.x = axis.x;
        v.y = axis.y;
        v.z = axis.z;
        v.w = axis.w;
        EXMaths::mat_44 inv = mtx.fastinverse();
        v.w = 1.0f;
        EXMaths::vec_xyzw a;
        D3DXVec4Transform(&a, &v, &inv);
        EXMaths::vec_xyzw aa;
        aa.x = a.x;
        aa.y = a.y;
        aa.z = a.z;
        aa.w = m_pBones[i].m_weight * angle;
        EXMaths::mat_44 r;
        r.set_rotate_axis(aa);
        EXMaths::mat_44 cur;
        cur.set_rotate_quat(pSec->m_rot);
        D3DXMatrixMultiply(&tmp, &r, &cur);
        cur = tmp;
        D3DXMatrixMultiply(&tmp, &r, &mtx);
        *(EXMaths::mat_44 *)&mtx = tmp;
        EXQuaternion q;
        q.SetRotation(*(const EXMatrix *)&cur);
        pSec->m_rot.x = q.x;
        pSec->m_rot.y = q.y;
        pSec->m_rot.z = q.z;
        pSec->m_rot.w = q.w;
        if (n == m_numBones) m_mtx = mtx;
    }
    for (EXItemRender_SkinAnimSection *p = pSec->ChildLinks(); p && n < m_numBones; p = p->Next())
        FindAndRotate(p, angle, axis, mtx, n);
}

static inline void NormaliseXYZ0(EXMaths::vec_xyzw &v)
{
    float l = v.z * v.z + v.x * v.x + v.y * v.y;
    v.w = 0.0f;
    if (l >= 0.0f) {
        l = (float)sqrt(l);
        if (l > 1e-5f) {
            float inv = 1.0f / l;
            v.x = inv * v.x;
            v.y = v.y * inv;
            v.z = v.z * inv;
            v.w = inv * 0.0f;
        }
    }
}

static inline float ClampedACos(float d)
{
    if (d >= 1.0f) d = 1.0f;
    else if (d <= -1.0f) d = -1.0f;
    return EXACos(d);
}

// Turn the tracked direction m_14 towards the look target (m_forwards in world space, relative to
// the head base) by at most m_maxAngularMove per update (half the remaining angle when closer), keep
// it within the yaw / pitch limits about the rest direction m_64 (m_7c..m_88: left, right, down,
// up) and rotate the head bones to it. `reset` snaps back to the rest direction.
void EXHeadTracking::Update(const EXItemRender_SkinAnim *pSkin, s8 reset)
{
    if (!m_a0) return;
    const EXAnimLodDetails *pLod = pSkin->m_pActiveLod;
    if (pLod) {
        if (pLod->m_flags & 0x80) return;
        if (pLod->m_flags & 0x10) m_a3 = 1;
    }
    EXMaths::vec_xyzw target;
    target.x = m_forwards.x;
    u32 savedFlags = m_flags;
    target.y = m_forwards.y;
    target.z = m_forwards.z;
    target.w = m_forwards.w;
    EXPlacementItem *pOwner = (EXPlacementItem *)pSkin->GetBaseOwner();
    if (!pOwner) return;
    EXMaths::mat_44 inv;
    EXMaths::vec_xyzw local;
    if (!pOwner->m_validMtx) {
        EXMaths::mat_44 m;
        m.set_rotate(pOwner->m_rotation, pOwner->m_rotOrder);
        m.r[3].x = pOwner->m_position.x;
        m.r[3].y = pOwner->m_position.y;
        m.r[3].z = pOwner->m_position.z;
        m.r[3].w = 1.0f;
        inv = m.fastinverse();
    } else
        inv = pOwner->m_mtx.fastinverse();
    target.w = 1.0f;
    D3DXVec4Transform(&local, &target, &inv);
    EXMaths::vec_xyzw d;
    d.w = 0.0f;
    EXMaths::vec_xyzw base;
    base.x = 0.0f;
    base.y = 0.0f;
    base.z = 0.0f;
    d.x = local.x;
    d.y = local.y;
    d.z = local.z;
    if (!(m_flags & 0x80)) {
        base.w = 1.0f;
        EXMaths::vec_xyzw b;
        D3DXVec4Transform(&b, &base, &m_mtx);
        base.x = b.x;
        base.y = b.y;
        base.z = b.z;
    }
    d.x = d.x - base.x;
    d.y = d.y - base.y;
    d.z = d.z - base.z;
    if (m_flags & 0x80) d.y = 0.0f;
    if (reset) {
        d.x = m_64.x;
        d.y = m_64.y;
        d.z = m_64.z;
        m_8c = m_90;
    }
    if (m_8c) {
        d.x = m_64.x;
        d.y = m_64.y;
        d.z = m_64.z;
        m_8c--;
    }
    if (m_a3) {
        d.x = m_64.x;
        d.y = m_64.y;
        d.z = m_64.z;
    }
    NormaliseXYZ0(d);
    // Step from the current direction towards the target.
    EXMaths::vec_xyzw axis;
    axis.x = d.z * m_14.y - d.y * m_14.z;
    axis.y = m_14.z * d.x - d.z * m_14.x;
    axis.z = m_14.x * d.y - m_14.y * d.x;
    axis.w = 0.0f;
    float ang = ClampedACos(m_14.w * d.w + m_14.z * d.z + m_14.y * d.y + m_14.x * d.x);
    if (ang > 0.01f) {
        axis.w = ang <= m_maxAngularMove ? ang * 0.5f : m_maxAngularMove;
        EXMaths::mat_44 r;
        r.set_rotate_axis(axis);
        EXMaths::vec_xyzw cur;
        cur.x = m_14.x;
        cur.y = m_14.y;
        cur.z = m_14.z;
        cur.w = 1.0f;
        EXMaths::vec_xyzw out;
        D3DXVec4Transform(&out, &cur, &r);
        d.x = out.x;
        d.y = out.y;
        d.z = out.z;
        NormaliseXYZ0(d);
    }
    if (m_flags & 0x80) {
        d.y = 0.0f;
        float l = d.w * d.w + d.z * d.z + d.x * d.x;
        if (l >= 0.0f) {
            l = (float)sqrt(l);
            if (l > 1e-5f) {
                float k = 1.0f / l;
                d.x = k * d.x;
                d.y = k * 0.0f;
                d.z = d.z * k;
                d.w = d.w * k;
            }
        }
    }
    // Angle and turn axis from the rest direction.
    EXMaths::vec_xyzw rest;
    rest.y = m_64.y;
    rest.z = m_64.z;
    rest.w = m_64.w;
    rest.x = m_64.x;
    EXMaths::vec_xyzw turn;
    turn.x = rest.y * d.z - rest.z * d.y;
    turn.y = rest.z * d.x - d.z * rest.x;
    turn.z = d.y * rest.x - rest.y * d.x;
    turn.w = 0.0f;
    float rot = ClampedACos(rest.x * d.x + rest.w * d.w + rest.z * d.z + rest.y * d.y);
    NormaliseXYZ0(turn);
    m_flags = m_flags & ~0x3f;
    bool left = m_64.z * turn.x <= 0.0f;
    if (left) m_flags.BitSet(0x10);
    int vert;
    float up;
    if (turn.y >= 0.0f) {
        vert = 3;
        up = turn.y;
    } else {
        vert = 2;
        m_flags.BitSet(0x20);
        up = 0.0f - turn.y;
    }
    if (m_a1) {
        if (m_numBones < 2)
            m_pBones[0].m_weight = 1.0f;
        else {
            float k = up;
            if (up < 0.9f && m_64.z * turn.x > 0.0f) k = 1.0f;
            for (u32 i = 0; i < m_numBones - 1; i++) m_pBones[i].m_weight = k / (float)m_numBones;
            u32 last = m_numBones - 1;
            m_pBones[last].m_weight = ((float)m_numBones - (float)last * k) / (float)m_numBones;
        }
    }
    if (m_a3 && rot < 0.001f && rot > -0.001f) {
        m_a0 = 0;
        m_a3 = 0;
        return;
    }
    const float *pLimits = &m_7c;
    float limit = (1.0f - up) * pLimits[left ? 1 : 0] + pLimits[vert] * up;
    if (rot <= limit)
        m_a4 = 0;
    else if (!m_a4 || (m_flags & 0x40)) {
        float av = turn.y;
        if (turn.y < 0.0f) av = 0.0f - turn.y;
        float ah = turn.x;
        if (turn.x < 0.0f) ah = 0.0f - av;   // retail negates the vertical part here
        rot = limit;
        if (left) {
            if (vert != 2) m_flags.BitSet(av < ah ? 2 : 8);
            else m_flags.BitSet(av < ah ? 2 : 4);
        } else {
            if (vert != 2) m_flags.BitSet(av < ah ? 1 : 8);
            else m_flags.BitSet(av < ah ? 1 : 4);
        }
        m_a4 = 1;
    } else {
        // Already at the limit: hold the previous direction.
        d.y = m_14.y;
        d.z = m_14.z;
        d.w = m_14.w;
        d.x = m_14.x;
        turn.x = rest.y * d.z - rest.z * d.y;
        turn.y = rest.z * d.x - d.z * rest.x;
        turn.z = d.y * rest.x - rest.y * d.x;
        turn.w = 0.0f;
        rot = ClampedACos(rest.x * d.x + rest.w * d.w + rest.z * d.z + rest.y * d.y);
        NormaliseXYZ0(turn);
        m_flags = savedFlags;
        m_a4 = 1;
    }
    m_14.y = d.y;
    m_14.x = d.x;
    m_14.z = d.z;
    m_14.w = d.w;
    EXItemRender_SkinAnimSection *pBase = pSkin->GetAnimBoneSection(m_baseHash, 0);
    if (!pBase && !(pBase = pSkin->GetAnimBoneSection(0xe000000, 0))) {
        m_a0 = 0;
        return;
    }
    if (m_a2) {
        for (u32 i = 0; i < m_numBones; i++) {
            EXItemRender_SkinAnimSection *pSec = pSkin->GetAnimBoneSection(m_pBones[i].m_hash, 0);
            if (!pSec) return;
            m_pBones[i].m_08 = pSec->m_bone;
        }
        m_a2 = 0;
    }
    EXMatrix identity;
    identity.set_identity();
    u32 n = 0;
    FindAndRotate(pBase, rot, *(EXVector *)&turn, identity, n);
}

// Register the standard skeleton for automatic mirroring: the centre / root bones and the left /
// right pairs present in the skin.
void EXItemAnimator_AnimModifier::SetupAutoFlip()
{
    CheckBone(0xe000000);
    CheckBone(0xe000001);
    CheckBone(0xe000002);
    CheckBone(0xe000003);
    CheckBone(0xe000004);
    CheckBone(0xe000005);
    CheckBone(0xe000006);
    CheckBone(0xe000007);
    CheckBone(0xe000008);
    CheckBone(0xe000009);
    CheckBone(0xe00000a);
    CheckBone(0xe00000b);
    CheckBone(0xe00000c);
    CheckBone(0xe00000d);
    CheckBone(0xe00001c);
    CheckBone(0xe00001d);
    CheckBone(0xe00001e);
    CheckBone(0xe000020);
    CheckBone(0xe000021);
    CheckSwapBone(0xe00000e, 0xe000012);
    CheckSwapBone(0xe00000f, 0xe000013);
    CheckSwapBone(0xe000010, 0xe000014);
    CheckSwapBone(0xe000011, 0xe000015);
    CheckSwapBone(0xe000016, 0xe000019);
    CheckSwapBone(0xe000017, 0xe00001a);
    CheckSwapBone(0xe000018, 0xe00001b);
    CheckSwapBone(0xe000022, 0xe000023);
    CheckSwapBone(0xe000024, 0xe000025);
    CheckSwapBone(0xe000026, 0xe000027);
    CheckSwapBone(0xe000028, 0xe000029);
    CheckSwapBone(0xe000030, 0xe00003f);
    CheckSwapBone(0xe000031, 0xe000040);
    CheckSwapBone(0xe000032, 0xe000041);
    CheckSwapBone(0xe000033, 0xe000042);
    CheckSwapBone(0xe000034, 0xe000043);
    CheckSwapBone(0xe000035, 0xe000044);
    CheckSwapBone(0xe000036, 0xe000045);
    CheckSwapBone(0xe000037, 0xe000046);
    CheckSwapBone(0xe000038, 0xe000047);
    CheckSwapBone(0xe000039, 0xe000048);
    CheckSwapBone(0xe00003a, 0xe000049);
    CheckSwapBone(0xe00003b, 0xe00004a);
    CheckSwapBone(0xe00003c, 0xe00004b);
    CheckSwapBone(0xe00003d, 0xe00004c);
    CheckSwapBone(0xe00003e, 0xe00004d);
    CheckSwapBone(0xe000050, 0xe00005f);
    CheckSwapBone(0xe000051, 0xe000060);
    CheckSwapBone(0xe000052, 0xe000061);
    CheckSwapBone(0xe000053, 0xe000062);
    CheckSwapBone(0xe000054, 0xe000063);
    CheckSwapBone(0xe000055, 0xe000064);
    CheckSwapBone(0xe000056, 0xe000065);
    CheckSwapBone(0xe000057, 0xe000066);
    CheckSwapBone(0xe000058, 0xe000067);
    CheckSwapBone(0xe000059, 0xe000068);
    CheckSwapBone(0xe00005a, 0xe000069);
    CheckSwapBone(0xe00005b, 0xe00006a);
    CheckSwapBone(0xe00005c, 0xe00006b);
    CheckSwapBone(0xe00005d, 0xe00006c);
    CheckSwapBone(0xe00005e, 0xe00006d);
}
