#include "EXItemCtrl_AnimModifier.h"
#include "EX6DObject.h"
#include "EXItem.h"
#include "EXItemCtrl_SnapShot.h"
#include "EXItemAnimator_Anim.h"
#include "EXMemory.h"

// TODO stand-in: animation modifier animator (EXItemAnimator_AnimModifier.obj), sizeof 0x194.
class EXItemAnimator_AnimModifier : public EXItemAnimator_Anim {
public:
    void SetModCtrl(EXItemCtrl_AnimModifier *p) { m_pModCtrl = p; }
    u8 m_180[0xc];
    EXItemCtrl_AnimModifier *m_pModCtrl;    // +0x18c
    u8 m_190[4];
};

EX_IMPLEMENT_DYNCREATE(EXItemCtrl_AnimModifier, EXItemCtrl)

EXItemCtrl_AnimModifier::~EXItemCtrl_AnimModifier()
{
    m_bodies.Empty(1);
    if (m_pAnimMod) m_pAnimMod->SetModCtrl(0);
}

s8 EXItemCtrl_AnimModifier::UpdateCtrlOnStack(EXItemAnimator_Anim *pAnim, EXItemCtrlStack *, EXRotPosInfo *pInfo, int n)
{
    if (!m_bodies.m_nSize) return 0;
    if (!m_bodies.m_pData->m_pBody) return 0;
    if (!m_bodies.m_pData->m_pBody->m_pItem) return 0;
    EXMotionData *pMotion = pInfo->m_pMotion;
    EXMatrix m;
    m.SetRotation(m_bodies.m_pData->m_pBody->m_pItem->m_rotation, m_bodies.m_pData->m_pBody->m_pItem->m_rotOrder);
    const EXVector &pos = m_bodies.m_pData->m_pBody->m_pItem->m_position;
    m.r[3].set(pos.x, pos.y, pos.z, 1.0f);
    EXItemRender_SkinAnim *pSkin = pAnim->m_pSkin;
    if (!n) pSkin->InitBlendStack(pInfo, 1);
    if (!pSkin) return 0;
    RecursiveSearch(m, pSkin->m_pBones, pMotion);
    EXItemCtrlStack::ms_UpdateFlags |= 0xc000;
    return 1;
}

void EXItemCtrl_AnimModifier::ResetPositions(const EXItemAnimator_AnimModifier *pMod, EXItemCtrl_SnapShot *pSnap, EXItemCtrl_SnapShot *pVel, float t)
{
    EXItemRender_SkinAnim *pSkin = pMod->m_pSkin;
    if (!pSkin) return;
    EXMatrix m;
    if (!pSnap && m_bodies.m_pData->m_pBody->m_pItem) {
        m.SetRotation(m_bodies.m_pData->m_pBody->m_pItem->m_rotation, m_bodies.m_pData->m_pBody->m_pItem->m_rotOrder);
        const EXVector &pos = m_bodies.m_pData->m_pBody->m_pItem->m_position;
        m.r[3].set(pos.x, pos.y, pos.z, 1.0f);
    } else {
        m.SetIdentity();
    }
    EXItemRender_SkinAnimSection *pRoot = pSkin->m_pBones;
    RecursiveReset(m, pRoot, pSnap);
    if (pVel) {
        m.SetIdentity();
        RecursiveResetVelocities(m, pRoot, pVel, t);
    }
}

void EXItemCtrl_AnimModifier::AddBody(EX6DAnimInfo &info) { m_bodies.Add(info, 1); }

EXItemCtrl_AnimModifier::EXItemCtrl_AnimModifier() { m_pAnimMod = 0; }


s8 EXItemCtrl_AnimModifier::RecursiveSearch(EXMatrix m, const EXItemRender_SkinAnimSection *pSec, EXMotionData *pMotion)
{
    int n = m_bodies.m_nSize;
    for (int i = 0; i < n; i++) {
        if (m_bodies.m_pData[i].m_bone == pSec->m_bone) {
            EXMatrix r;
            r.set_rotate_quat(m_bodies.m_pData[i].m_pBody->m_rot);
            const EXVector &p = m_bodies.m_pData[i].m_pBody->m_pos;
            r.r[3].set(p.x, p.y, p.z, 1.0f);
            r *= m_bodies.m_pData[i].m_mtx;
            r = m.FastInverse() * r;
            pMotion[m_bodies.m_pData[i].m_bone].m_pos = r.GetTranslation();
            r.GetRotation(pMotion[m_bodies.m_pData[i].m_bone].m_rot);
        }
    }
    EXMatrix s;
    s.set_rotate_quat(pMotion[pSec->m_bone].m_rot);
    const EXVector &t = pMotion[pSec->m_bone].m_pos;
    s.r[3].set(t.x, t.y, t.z, 1.0f);
    m = m * s;
    for (const EXItemRender_SkinAnimSection *pChild = pSec->ChildLinks(); pChild; pChild = pChild->Next())
        RecursiveSearch(m, pChild, pMotion);
    return 1;
}

s8 EXItemCtrl_AnimModifier::RecursiveReset(EXMatrix m, const EXItemRender_SkinAnimSection *pSec, EXItemCtrl_SnapShot *pSnap)
{
    int n = m_bodies.m_nSize;
    EXMatrix s;
    if (pSnap) {
        s.set_rotate_quat(pSnap->MotionData()[pSec->m_bone].m_rot);
        const EXVector &t = pSnap->MotionData()[pSec->m_bone].m_pos;
        s.r[3].set(t.x, t.y, t.z, 1.0f);
    } else {
        s.set_rotate_quat(pSec->m_rot);
        s.r[3].set(pSec->m_pos.x, pSec->m_pos.y, pSec->m_pos.z, 1.0f);
    }
    m = m * s;
    for (int i = 0; i < n; i++) {
        if (m_bodies.m_pData[i].m_bone == pSec->m_bone) {
            EXMatrix r = m_bodies.m_pData[i].m_mtx;
            r = m * r.FastInverse();
            m_bodies.m_pData[i].m_pBody->m_rot.SetRotation(r);
            m_bodies.m_pData[i].m_pBody->m_pos = r.GetTranslation();
        }
    }
    for (const EXItemRender_SkinAnimSection *pChild = pSec->ChildLinks(); pChild; pChild = pChild->Next())
        RecursiveReset(m, pChild, pSnap);
    return 1;
}

// NEAR: 220 bytes; index operand order ([edx+ebx] vs [ebx+edx]) and first mulss operand order.
s8 EXItemCtrl_AnimModifier::RecursiveResetVelocities(EXMatrix m, const EXItemRender_SkinAnimSection *pSec, EXItemCtrl_SnapShot *pSnap, float t)
{
    int n = m_bodies.Size();
    EXMatrix s;
    s.set_rotate_quat(pSnap->MotionData()[pSec->m_bone].m_rot);
    const EXVector &p = pSnap->MotionData()[pSec->m_bone].m_pos;
    s.r[3].set(p.x, p.y, p.z, 1.0f);
    m = m * s;
    for (int i = 0; i < n; i++) {
        if (m_bodies[i].m_bone == pSec->m_bone) {
            EXMatrix r = m_bodies[i].m_mtx;
            r = m * r.FastInverse();
            EX6DRigidBody *pBody = m_bodies[i].m_pBody;
            pBody->m_velocity = (pBody->m_pos - r.GetTranslation()) * (1.0f / t);
            m_bodies[i].m_pBody->m_velocity.w = 0.0f;
        }
    }
    for (const EXItemRender_SkinAnimSection *pChild = pSec->ChildLinks(); pChild; pChild = pChild->Next())
        RecursiveResetVelocities(m, pChild, pSnap, t);
    return 1;
}
