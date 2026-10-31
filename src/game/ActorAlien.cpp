#include "Actor.h"
#include "FEText.h"
#include "AIControl.h"
#include "Avoid.h"
#include "ActorUtil.h"
#include "XApp.h"
#include "XNavRoute.h"
#include "XNavLocation.h"
#include "XGeoNavMeshEntity.h"
#include "XItemHandler_Player.h"
#include "EXQuaternion.h"
#include "EXMatrix.h"
#include "EXItemAnimator_Anim.h"
#include "EXExtRenderInfoStandIn.h"
#include <math.h>

// TODO stand-in: motion sample filled by EXItemAnimator_Anim::CalcMotionData (translation first).
struct EXMotionData {
    float x, y, z, w;
    float m_Rest[4];
};

// Gives both of the actor's render infos the same flat light set and ambient colour.
static void Alien_SetLighting(EXExtRenderInfo &info, const EXVector &dir, const float *amb)
{
    info.m_Light[0].set(dir.x, dir.y, dir.z, dir.w);
    info.m_Light[1].set(dir.x, dir.y, dir.z, dir.w);
    info.m_Light[2].set(dir.x, dir.y, dir.z, dir.w);
    info.m_Light2[0].set(0.5f, 0.5f, 0.5f, 0.0f);
    info.m_Light2[1].set(0.0f, 0.0f, 0.0f, 0.0f);
    info.m_Light2[2].set(0.0f, 0.0f, 0.0f, 0.0f);
    info.m_Ambient.set(amb[0], amb[1], amb[2], 0.0f);
}


// Per-frame movement for actors that walk on any surface (aliens on walls and ceilings): lights the actor,
// works out where it wants to go (route, destination or the player), steers around others, pushes off
// the player, then moves it by animation motion or falling and stands it on the surface it lands on.
void Actor::UpdateMovement3D()
{
    bool bOnScreen = m_StateFlags.m_bOnScreen;
    bool bAirborne = (m_NavRoute.m_Flags.m_b7 && !m_NavRoute.m_Flags.m_bJumpCancel) || m_NavRoute.m_Flags2.m_b0;
    bool bDead = m_Health <= 0.0f;
    bool bRoute = m_StateFlags.m_bMoving15 && !m_StateFlags.m_bMoving14 && !m_StateFlags.m_bMoving27;
    bool bPlayerCollide = !bDead && CanCollideWithPlayer();
    if (!bDead && m_OrgLoc.m_vPos.y < -1000.0f)
        SetHealth(0.0f, 1);

    if (bOnScreen) {
        float len = (float)sqrt(3.0);
        EXVector dir(-1.0f, -1.0f, -1.0f, 0.0f);
        if (len > 1e-5f) {
            float inv = 1.0f / len;
            dir.set(-1.0f * inv, -1.0f * inv, -1.0f * inv, 0.0f * inv);
        }
        m_Unk8d4[0] = m_Unk8d4[1] = m_Unk8d4[2] = 1.0f;
        XNavMesh *pMesh = m_OrgLoc.IsValid() ? m_OrgLoc.GetMesh() : 0;
        if (!XItemHandler_Player::Instance()->m_Tint && pMesh) {
            int poly = m_OrgLoc.m_nPolyHandle & 0xffff;
            if (poly == 0xffff) poly = -1;
            m_Unk8d4[0] = m_Unk8d4[1] = m_Unk8d4[2] = 1.0f;
            pMesh->GetLighting(m_OrgLoc.m_vPos, poly, m_Unk8d4[0], m_Unk8d4[1], m_Unk8d4[2]);
            for (int i = 0; i < 3; i++)
                if (m_Unk8d4[i] < 0.01f) m_Unk8d4[i] = 0.01f;
        }
        Alien_SetLighting(m_ExtRenderInfo, dir, m_Unk8d4);
        Alien_SetLighting(m_ExtRenderInfo2, dir, m_Unk8d4);
    }

    EXQuaternion curRot;
    EXMatrix mat;
    mat.SetIdentity();
    curRot.SetRotation(mat);
    m_pItem->SetRotOrder(0);
    curRot = EXQuaternion(m_pItem->Rotation().x, m_pItem->Rotation().y, m_pItem->Rotation().z, m_pItem->Rotation().w);

    EXVector moveDir(0.0f, 0.0f, 1.0f, 0.0f);
    EXVector push(0.0f, 0.0f, 0.0f, 0.0f);
    EXVector delta(0.0f, 0.0f, 0.0f, 0.0f);

    if (m_NavRoute.m_Flags.m_bTriggerJump) {
        bAirborne = true;
        m_NavRoute.m_Flags.m_bTriggerJump = 0;
        m_NavRoute.m_Flags.m_bJumpCancel = 0;
        m_NavRoute.m_Flags.m_b7 = 1;
        bRoute = false;
        m_WorldVelocity.y = 0.0f;
    }
    if (bDead && !bAirborne && m_OrgLoc.m_vFloorNormal.y <= 0.75f) {
        m_NavRoute.m_Flags2.m_b1 = 0;
        m_NavRoute.m_Flags2.m_b0 = 1;
        m_WorldVelocity.x = m_OrgLoc.m_vFloorNormal.x * 0.1f;
        m_WorldVelocity.y = m_OrgLoc.m_vFloorNormal.y * 0.1f;
        m_WorldVelocity.z = m_OrgLoc.m_vFloorNormal.z * 0.1f;
        m_WorldVelocity.w = m_OrgLoc.m_vFloorNormal.w * 0.1f;
        bAirborne = true;
        bRoute = false;
        delta.y = -0.01f;
    }

    EXQuaternion newRot = curRot;
    bool bToGround = false;
    if (m_NavRoute.m_Flags2.m_b1) {
        if (m_OrgLoc.m_vFloorNormal.y > 0.0f) {
            SendMsg(MsgItem((MsgType)0x26), 0.0f);
            m_NavRoute.m_Flags2.m_b1 = 0;
        } else {
            delta.y -= 0.15f;
            bAirborne = true;
            m_NavRoute.m_Flags2.m_b1 = 0;
            m_NavRoute.m_Flags2.m_b0 = 1;
            m_WorldVelocity.y = 0.0f;
            bToGround = true;
        }
    }
    if (!bToGround && !bAirborne && !bDead) {
        if (!bRoute) {
            m_StateFlags.m_bValidRoute = 1;
            moveDir = m_DestLoc.m_vPos - m_OrgLoc.m_vPos;
            m_NavRoute.m_MoveVector = moveDir;
        } else {
            m_NavRoute.CalcRoute(&m_OrgLoc, &m_DestLoc);
            m_StateFlags.m_bValidRoute = m_NavRoute.m_Status == 0;
            moveDir = m_NavRoute.m_MoveToPoint - m_OrgLoc.m_vPos;
        }
    } else if (bDead) {
        m_StateFlags.m_bValidRoute = 1;
        EXQuaternion q(m_pItem->Rotation().x, m_pItem->Rotation().y, m_pItem->Rotation().z, m_pItem->Rotation().w);
        EXMatrix rot;
        q.GetRotation(rot);
        EXVector out;
        D3DXVec4Transform(&out, &moveDir, &rot);
        moveDir = out;
        m_NavRoute.m_MoveVector = moveDir;
    } else {
        m_StateFlags.m_bValidRoute = 1;
        EXVector d = gAIControl.m_PlayerNavLoc.m_vFloorPos - m_OrgLoc.m_vPos;
        float lenSq = d.x * d.x + d.y * d.y + d.z * d.z + d.w * d.w;
        moveDir = d;
        if (lenSq >= 0.0f) {
            float len = (float)sqrt(lenSq);
            if (len > 1e-5f) {
                float inv = 1.0f / len;
                moveDir.set(d.x * inv, d.y * inv, d.z * inv, d.w * inv);
            }
        }
        m_NavRoute.m_MoveVector = moveDir;
    }

    if (bOnScreen && !bDead) {
        CAvoidance avoid;
        SetPositionEx(m_OrgLoc.m_vPos);
        avoid.Steer(this, m_pItem->Rotation().y);
        push = avoid.m_PushDelta;
    }

    if (bPlayerCollide) {
        XItemHandler_Player *pPlayer = XItemHandler_Player::Instance();
        EXVector pp;
        if (pPlayer && (pPlayer->m_Mode == 4 || pPlayer->m_Mode == 0xc || pPlayer->m_Mode == 0x18))
            pp = Player_GetPelvisPos();
        else
            pp = Player_GetFeetPos();
        EXVector d((push.x + m_OrgLoc.m_vPos.x) - pp.x, (m_OrgLoc.m_vPos.y + push.y) - pp.y,
                   (m_OrgLoc.m_vPos.z + push.z) - pp.z, (m_OrgLoc.m_vPos.w + push.w) - pp.w);
        if (fabs(d.y) <= 0.5f) {
            float distSq = d.x * d.x + d.z * d.z;
            float dist = distSq < 0.0f ? 0.0f : (float)sqrt(distSq);
            float radius = m_CollisionRadius + 0.49f;
            if (dist <= radius) {
                float w;
                if (dist <= 0.01f) {
                    d.x = d.y = d.z = 0.0f;
                    w = 0.0f;
                } else {
                    float s = (radius - dist) / dist;
                    d.y = s * 0.0f;
                    d.x *= s;
                    d.z = s * d.z;
                    w = s * d.w;
                }
                float adj = m_ActorToPlayerColAdj;
                d.x *= adj;
                d.y = adj * d.y;
                d.w = adj * w;
                d.z = adj * d.z;
                push.x += d.x; push.y += d.y; push.z += d.z; push.w += d.w;
            }
        }
    }

    if (bDead || bAirborne) {
        curRot = EXQuaternion(m_pItem->Rotation().x, m_pItem->Rotation().y, m_pItem->Rotation().z, m_pItem->Rotation().w);
        curRot.GetRotation(mat);
        if (bAirborne)
            m_WorldVelocity.y -= XApp::GameFrameRateMul() * 0.0083333f;
        if (m_WorldVelocity.y < -1.2f)
            m_WorldVelocity.y = -1.2f;
        float dt = XApp::GameFrameRateMul();
        delta.x += m_WorldVelocity.x * dt;
        delta.y += m_WorldVelocity.y * dt;
        delta.z += m_WorldVelocity.z * dt;
        delta.w += m_WorldVelocity.w * dt;
    }

    EXVector unused(0.0f, 0.0f, 0.0f, 0.0f);
    EXVector outRot;
    if (m_NavRoute.StandOnSurface(unused, &m_OrgLoc, moveDir, outRot, &mat, &newRot)) {
        curRot = EXQuaternion(curRot.slerp(newRot, 0.5f));
        curRot.GetRotation(mat);
    }

    GetAnimDelta();
    EXMotionData motion;
    ((EXItemAnimator_Anim *)m_pItem->GetAnimator(EXRUNTIME_CLASS(EXItemAnimator_Anim)))->CalcMotionData(motion, 3, 1);
    float sqDist = SqDistToTarget();
    if (!bAirborne && !bDead) {
        if (!m_StateFlags.m_bMoving15 || motion.x * motion.x + motion.z * motion.z + motion.y * motion.y < sqDist) {
            EXVector local(motion.x, motion.y, motion.z, motion.w);
            EXVector world;
            D3DXVec4Transform(&world, &local, &mat);
            delta.x += world.x + push.x;
            delta.y += world.y + push.y;
            delta.z += world.z + push.z;
            delta.w += world.w + push.w;
        } else {
            delta = m_DestLoc.m_vPos - m_OrgLoc.m_vPos;
        }
    }

    EXVector newPos(delta.x + m_OrgLoc.m_vPos.x, m_OrgLoc.m_vPos.y + delta.y, m_OrgLoc.m_vPos.z + delta.z,
                    m_OrgLoc.m_vPos.w + delta.w);
    m_NavRoute.Collide(&m_OrgLoc, newPos);
    if (m_NavRoute.m_Flags.m_bCollided) {
        m_WorldVelocity.set(0.0f, 0.0f, 0.0f, 0.0f);
        bool bWasJumping = m_NavRoute.m_Flags.m_b7;
        m_NavRoute.m_Flags.m_b5 = 0;
        if (bWasJumping) {
            SendMsg(MsgItem((MsgType)0x26), 0.0f);
            bAirborne = false;
            m_NavRoute.m_Flags.m_b7 = 0;
            m_NavRoute.m_Flags.m_bJumpCancel = 1;
        }
        if (m_NavRoute.m_Flags2.m_b0) {
            SendMsg(MsgItem((MsgType)0x26), 0.0f);
            m_NavRoute.m_Flags2.m_b0 = 0;
            bAirborne = false;
            m_NavRoute.m_Flags.m_bJumpCancel = 1;
        }
    }

    SetPositionEx(m_OrgLoc.m_vPos);
    m_pItem->SetRotation(curRot);
    if (!bAirborne) {
        EXVector d = m_OrgLoc.m_vPos - m_Unk18;
        float distSq = d.x * d.x + d.z * d.z;
        float dist = distSq < 0.0f ? 0.0f : (float)sqrt(distSq);
        m_LocalVelocity.z = dist * XApp::GameFrameRateDiv() + GetAnimVelocity();
        m_LocalVelocity.z *= 0.5f;
    }
    m_Unk18 = m_OrgLoc.m_vPos;
}
