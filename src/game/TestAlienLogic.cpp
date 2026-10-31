#include "Actor.h"
#include "ActorProfile.h"
#include "ActorUtil.h"
#include "StateMachine.h"
#include "AIControl.h"
#include "EXRand.h"
#include "FEText.h"
#include "FX_Gore.h"
#include "SubGroupAnims.h"
#include "WeaponTable.h"
#include "XItemHandler_Player.h"
#include "XNavLocation.h"
#include "XNavMesh.h"
#include "XNavRoute.h"
#include "EXMaths.h"

struct _AlienAnimItem {
    int m_From;     // current anim mode
    int m_To;       // mode to pass through
    int m_Anim;     // requested mode
};

_AlienAnimItem AlienAnimTable[] = {
    {0x9000342, 0x900036f, 0x9000335},
    {0x900036f, 0x900036f, 0x9000335},
    {0x9000335, 0x9000373, 0x9000342},
    {0x9000373, 0x9000373, 0x9000342},
    {0x9000095, 0x900039a, 0x9000342},
    {0x9000095, 0x900039b, 0x9000335},
    {-1, -1, -1},
};

signed char STATE_Alien_Attack(StateMachine &sm);
signed char STATE_Alien_CeilingHang(StateMachine &sm);
signed char STATE_Alien_Sonic_Hit(StateMachine &sm);
signed char AU_HandlePlayerAttacks_Alien(StateMachine &sm, StructMsg_AttackedByPredator *pMsg);

int AlienAnim(Actor &actor, int anim)
{
    int mode = actor.m_AnimMode;
    if (anim == actor.GetAnimModeNext()) return mode;
    for (_AlienAnimItem *p = AlienAnimTable; p->m_From >= 0; p++) {
        if (mode == p->m_From && anim == p->m_Anim) {
            actor.SetAnimModeNext(p->m_To);
            return mode;
        }
        if (mode == p->m_To && anim == p->m_Anim) {
            actor.SetAnimModeNext(anim);
            return mode;
        }
    }
    actor.SetAnimModeNext(0);
    return anim;
}

signed char STATE_TestAlien_Global(StateMachine &sm) { return 0; }

signed char Alien_HandlePlayerAttacks(StateMachine &sm, StructMsg_AttackedByPredator *pMsg)
{
    sm.GetActorOwner().m_HitBy = pMsg->m_Attack;
    if (sm.GetActorOwner().GetSkelHash() == 0xd000103) AU_HandlePlayerAttacks_Alien(sm, pMsg);
    return 1;
}

// Can the alien leap onto the surface at target? Only between surfaces facing (almost) opposite ways,
// no more than 20 away, and with a ray to it that travels at least 2.
signed char AlienCalcJumpPos(Actor &actor, XNavLocation &target, EXVector &out, int &poly)
{
    XNavLocation &org = actor.OrgLoc();
    EXVector &n = org.FloorNormal();
    EXVector &tn = target.FloorNormal();
    if (n.x * tn.x + n.y * tn.y + n.z * tn.z + n.w * tn.w > -0.7f) return 0;
    EXVector from = org.m_vPos;
    EXVector delta = target.m_vPos - from;
    float len2 = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z + delta.w * delta.w;
    float dist = 0.0f;
    if (len2 >= 0.0f) {
        dist = EXSqrt(len2);
        if (dist > 20.0f) return 0;
    }
    float hitDist = 0.0f;
    if (org.GetMesh()->LocatePolygon3DRay(from, delta, &poly, 0, &hitDist, &out)) {
        EXVector d = out - from;
        float l2 = d.x * d.x + d.y * d.y + d.z * d.z + d.w * d.w;
        if (l2 >= 0.0f && EXSqrt(l2) >= 2.0f) return 1;
    }
    return 0;
}

// Common hit handling: damage, flinch timer, strike message. 0 when this killed it.
signed char Alien_TakeHit(StateMachine &sm, StructMsg_AttackedByPredator *pMsg)
{
    Actor &a = sm.GetActorOwner();
    a.CheckBossDamageTriggers(pMsg->m_Damage);
    a.ChangeHealth(0.0f - pMsg->m_Damage, 1);
    a.m_HitCountdown = 10.0f;
    if (a.m_Health > 0.0f) {
        a.SendMsg(MsgItem(MSG_STRIKE), 0.0f);
        return 1;
    }
    return 0;
}

signed char STATE_Alien_Fall(StateMachine &sm)
{
    sm.GetActorOwner().m_WatchFlags = 4;
    switch (sm.m_Msg.m_Type) {
    case MSG_NONE:
        sm.GetActorOwner().IsMsgOnStack(MSG_NONE);
        return 1;
    case MSG_ENTER:
        sm.GetActorOwner().m_StateFlags.m_bCanRipBossHead = 0;
        sm.m_LocalData[0] = 0;
        sm.GetActorOwner().SetNav3D(1);
        return 1;
    case MSG_EXIT:
        sm.GetActorOwner().m_AnimMotionOverride &= ~0x200u;
        return 1;
    case MSG_EXT:
        if (sm.m_LocalData[0] == 0) {
            sm.GetActorOwner().m_AnimMotionOverride |= 0x200;
            sm.GetActorOwner().SetAnimType(AlienAnim(sm.GetActorOwner(), 0x900042a), 0, 1);
            sm.GetActorOwner().NavRoute().m_Flags2.m_b1 = 1;
            sm.GetActorOwner().NavRoute().m_Flags.m_b5 = 0;
            sm.m_LocalData[0]++;
            return 1;
        }
        if (sm.m_LocalData[0] == 2 && sm.GetActorOwner().HasAnimEnded()) {
            SM_GOTO(STATE_Alien_Attack, 0, 0, 0, 0);
            return 1;
        }
        return 0;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            Alien_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case (MsgType)0x26:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x26)) {
            sm.GetActorOwner().m_AnimMotionOverride &= ~0x200u;
            sm.GetActorOwner().SetAnimType(AlienAnim(sm.GetActorOwner(), 0x90004dd), 0, 1);
            sm.m_LocalData[0] = 2;
            return 1;
        }
        return 0;
    }
    return 0;
}

signed char STATE_Alien_EMP_Hit(StateMachine &sm)
{
    sm.GetActorOwner().m_WatchFlags = 4;
    switch (sm.m_Msg.m_Type) {
    case MSG_NONE:
        sm.GetActorOwner().IsMsgOnStack(MSG_NONE);
        return 1;
    case MSG_ENTER:
        sm.GetActorOwner().m_StateFlags.m_bCanRipBossHead = 0;
        sm.m_LocalData[0] = 0;
        sm.GetActorOwner().SetNav3D(1);
        sm.GetActorOwner().SetAnimType(AlienAnim(sm.GetActorOwner(), 0x900042b), 0, 1);
        sm.m_LocalData[1] = EXRand(600) + 600;
        return 1;
    case MSG_EXT:
        if (sm.m_LocalData[0] == 0 && --(int &)sm.m_LocalData[1] < 0) {
            SM_GOTO(STATE_Alien_Attack, 0, 0, 0, 0);
            return 1;
        }
        return 0;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            Alien_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

// Launches the alien along a ballistic arc from where it stands to target (landing in poly).
signed char AlienTriggerJump(Actor &actor, EXVector &target, int poly)
{
    XNavRoute &nav = actor.NavRoute();
    EXVector &pos = actor.OrgLoc().m_vPos;
    float dx = target.x - pos.x;
    float dy = target.y - pos.y;
    float dz = target.z - pos.z;
    float dw = target.w - pos.w;
    float horiz2 = dz * dz + dx * dx;
    float horiz = horiz2 < 0.0f ? 0.0f : EXSqrt(horiz2);
    float apex = horiz * 0.5f;
    float up = dy;
    if (up >= 0.0f) up += 0.5f;
    float vUp;
    if (apex <= (float)fabs(up) && (apex = up, up <= 0.0f)) vUp = 0.0f;
    else if (apex * 0.0166f < 0.0f) vUp = 0.0f;
    else vUp = EXSqrt(apex * 0.0166f);
    float t = vUp * vUp - dy * 0.0166f;
    float vDown = t < 0.0f ? 0.0f : EXSqrt(t);
    actor.WorldVelocity().y = vUp;
    float l2 = dz * dz + dw * dw + dx * dx;
    if (l2 >= 0.0f) {
        float l = EXSqrt(l2);
        if (l > 1e-5f) {
            float inv = 1.0f / l;
            dx *= inv;
            dz *= inv;
        }
    }
    float speed = 0.0f - horiz / (((0.0f - vDown) - vUp) * 120.48193f);
    actor.WorldVelocity().x = speed * dx;
    actor.WorldVelocity().z = speed * dz;
    nav.m_Flags.m_bTriggerJump = 0;
    nav.m_Flags.m_b7 = 1;
    nav.m_Flags.m_bJumpCancel = 0;
    nav.m_JumpFromPos = pos;
    nav.m_JumpToPoly = (u16)poly;
    nav.m_JumpToPos = target;
    return 1;
}

signed char AU_HandlePlayerAttacks_Alien(StateMachine &sm, StructMsg_AttackedByPredator *pMsg)
{
    Actor &a = sm.GetActorOwner();
    SubGroupAnims &sub = SubGroupAnims::GetSubGroupAnims(a);
    int type = pMsg->m_Attack;
    pMsg->m_Damage = *(float *)&sub.m_Anims[7] * pMsg->m_Damage;
    int dir[4];
    memcpy(dir, &pMsg->m_Dir, sizeof(dir));
    bool bWall = false;
    if (type == 8 || type == 9 || type == 7) {
        while (a.SearchBrainStateStack(STATE_Alien_EMP_Hit)) sm.PopState();
        while (a.SearchBrainStateStack(STATE_Alien_Sonic_Hit)) sm.PopState();
        a.m_AnimMotionOverride &= ~0x200u;
        bWall = a.OrgLoc().FloorNormal().y < 0.7f;
    }
    switch (type) {
    case 1: case 3: case 4: case 0x37: case 0x38: case 0x39: case 0x3b: case 0x3c: case 0x3d: {
        Gore_CreateActorBlood(&a, 0, 0, type, pMsg->m_Pos, pMsg->m_Dir, 1);
        EXVector d = a.GetFeetPos() - Player_GetFeetPos();
        if (d.y * d.y + d.z * d.z + d.w * d.w + d.x * d.x < 4.0f) {
            WeaponProfile *pW = WeaponProfile::GetWeaponData(0x49000065);
            XItemHandler_Player::GetInstance()->SendPredMsg(2, pW->GetBulletDamage());
        }
    }
    // fall through
    case 0: case 2: case 10: case 11: case 0x33: case 0x34: case 0x35:
        Alien_TakeHit(sm, pMsg);
        return 1;
    case 5: case 6:
        Gore_CreateActorBlood(&a, 0, 0, type, pMsg->m_Pos, pMsg->m_Dir, 1);
        Alien_TakeHit(sm, pMsg);
        return 1;
    case 7:
        Gore_CreateActorBlood(&a, 0, 0, 7, pMsg->m_Pos, pMsg->m_Dir, 1);
        if (!Alien_TakeHit(sm, pMsg)) return 1;
        if (!bWall) return 1;
        SM_GOTO(STATE_Alien_Fall, 0, 0, 0, 0);
        return 1;
    case 8:
        if (!bWall) {
            SM_GOSUB(STATE_Alien_EMP_Hit, (int)dir, 1, 0, 0);
            return 1;
        }
        break;
    case 9:
        if (!bWall) {
            SM_GOSUB(STATE_Alien_Sonic_Hit, (int)dir, 1, 0, 0);
            return 1;
        }
        break;
    default:
        return 1;
    }
    SM_GOTO(STATE_DeathBullet, 0, 0, 0, 0);
    return 1;
}

// Weapon-less melee/tail strikes: damage the player when within radius (squared radius r2) of where
// the alien's attack lands (head, pelvis or feet depending on what the player is doing).
static void Alien_StrikePlayer(StateMachine &sm, int weapon, float r2)
{
    WeaponProfile *pW = WeaponProfile::GetWeaponData(weapon);
    EXVector p = gAIControl.PlayerNavLoc().m_v00;
    XItemHandler_Player *pPlayer = XItemHandler_Player::GetInstance();
    if (pPlayer) {
        unsigned int mode = pPlayer->GetMode();
        if (mode == 4 || mode == 0xc) p = Player_GetHeadPos();
        else if (mode == 0x18) p = Player_GetPelvisPos();
        else p = Player_GetFeetPos();
    }
    EXVector d = sm.GetActorOwner().GetFeetPos() - p;
    if (d.y * d.y + d.z * d.z + d.w * d.w + d.x * d.x < r2) Player_DoDamage(pW->GetBulletDamage());
}

static float Alien_DestDist3(Actor &a)
{
    float dx = a.m_DestLoc.m_vPos.x - a.GetFeetPos().x;
    float dy = a.m_DestLoc.m_vPos.y - a.GetFeetPos().y;
    float dz = a.m_DestLoc.m_vPos.z - a.GetFeetPos().z;
    return dx * dx + dy * dy + dz * dz;
}

// Main alien AI. Locals: [0..3] jump target, [4] jump poly, [5] phase, [6] strafe side (0 L, 1 R),
// [7] strafe timer, [8] speak timer, [9] struck flag, [10] phase to return to after a one-shot anim.
// Phases: 0 idle, 1 chase, 2 strafe, 3 (unused) lunge, 4 wait for anim, 5 attack, 6 back off,
// 7 crouch for jump, 8 wait then launch.
signed char STATE_Alien_Attack(StateMachine &sm)
{
    Actor &a = sm.GetActorOwner();
    unsigned int *L = sm.m_LocalData;
    XNavLocation &pnl = gAIControl.PlayerNavLoc();
    bool bFar = 2.0f <= pnl.m_fMeshDistance;
    a.m_WatchFlags = 4;
    switch (sm.m_Msg.m_Type) {
    case (MsgType)0x26:
        if (!a.IsMsgOnStack((MsgType)0x26)) {
            a.m_AnimMotionOverride &= ~0x200u;
            a.SetAnimType(AlienAnim(a, 0x90004dd), 0, 1);
            L[10] = 1;
            L[5] = 4;
            return 1;
        }
        return 1;
    case (MsgType)0x2e:
        if (!a.IsMsgOnStack((MsgType)0x2e)) Alien_StrikePlayer(sm, 0x49000064, 4.0f);
        return 1;
    case (MsgType)0x2f:
        if (!a.IsMsgOnStack((MsgType)0x2f)) Alien_StrikePlayer(sm, 0x49000067, 6.25f);
        return 1;
    case (MsgType)0x30:
        if (!a.IsMsgOnStack((MsgType)0x30)) Alien_StrikePlayer(sm, 0x49000068, 4.0f);
        return 1;
    case MSG_STRIKE:
        if (!a.IsMsgOnStack(MSG_STRIKE)) L[9] = 1;
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!a.IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            Alien_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_NONE:
        a.IsMsgOnStack(MSG_NONE);
        return 1;
    case MSG_ENTER:
        a.m_StateFlags.m_bCanRipBossHead = 0;
        L[9] = 0;
        L[6] = 0;
        L[5] = 0;
        L[10] = 1;
        L[7] = g_EXRandClass.Rand32() % 180 + 180;
        L[8] = g_EXRandClass.Rand32() % 180 + 180;
        a.SetNav3D(1);
        a.SetAnimType(AlienAnim(a, 0x9000318), 0, 1);
        a.GotoPosPoly(pnl.m_vFloorPos, pnl.m_nPolyHandle);
        if (L[5] == 2) a.SetRotY(EXRandf() * 6.2831855f - 3.1415927f);
        return 1;
    case MSG_EXT:
        break;
    default:
        return 0;
    }

    if (--(int &)L[8] == 0) {
        a.Speak(0x1af0053f, 0);
        L[8] = g_EXRandClass.Rand32() % 180 + 60;
    }
    bool bAlone = !a.AllyNear(20.0f, 1);
    a.GotoPosPoly(pnl.m_vFloorPos, pnl.m_nPolyHandle);

    switch (L[5]) {
    case 0:
        if (a.IsValidRoute()) L[5] = 1;
        a.SetAnimType(AlienAnim(a, 0x9000318), 0, 1);
        if (L[9]) goto struck;
        return 1;
    case 1: {
        if (L[9]) goto struck;
        if (!a.IsValidRoute()) L[5] = 0;
        float d2 = Alien_DestDist3(a);
        int anim;
        if (d2 <= 64.0f) {
            anim = 0x9000095;
        } else {
            if (g_EXRandClass.Rand32() % 30 == 0 &&
                AlienCalcJumpPos(a, pnl, (EXVector &)L[0], (int &)L[4])) {
                L[5] = 7;
                return 1;
            }
            anim = 0x9000508;
        }
        a.SetAnimType(AlienAnim(a, anim), 0, 1);
        if (1.0f <= d2) {
            if ((bFar && d2 < 100.0f) ||
                (36.0f < d2 && d2 < 100.0f && g_EXRandClass.Rand32() % 100 < 5)) {
                L[5] = 2;
                L[6] = g_EXRandClass.Rand32() & 1;
                return 1;
            }
            if (9.0f <= d2) return 1;
            unsigned int r = g_EXRandClass.Rand32();
            L[5] = 5;
            if (r % 100 < 70) {
                L[6] = g_EXRandClass.Rand32() & 1;
                anim = 0x900038f;
            } else {
                unsigned int r2 = g_EXRandClass.Rand32();
                L[6] = r2 & 1;
                anim = (r2 & 1) == 0 ? 0x9000387 : 0x9000388;
            }
            a.SetAnimType(AlienAnim(a, anim), 0, 1);
            return 1;
        }
        L[5] = 6;
        return 1;
    }
    case 2: {
        if (L[9]) {
            L[9] = 0;
            int anim;
            if (L[6] == 0) anim = (g_EXRandClass.Rand32() & 1) == 0 ? 0x90004ed : 0x90004ef;
            else anim = (g_EXRandClass.Rand32() & 1) == 0 ? 0x90004ec : 0x90004ee;
            a.SetAnimType(AlienAnim(a, anim), 0, 1);
            L[10] = 2;
            L[5] = 4;
            return 1;
        }
        int mode = a.m_AnimMode;
        if (L[6] == 0) {
            if (a.CanStrafeLeft(2.0f)) {
                a.SetAnimType(AlienAnim(a, 0x9000342), 0, 1);
            } else {
                int anim = (mode == 0x9000342 || mode == 0x9000387) ? 0x900036f : 0x900039b;
                a.SetAnimType(AlienAnim(a, anim), 0, 1);
                L[6] = 1;
            }
        } else {
            if (a.CanStrafeRight(2.0f)) {
                a.SetAnimType(AlienAnim(a, 0x9000335), 0, 1);
            } else {
                int anim = (mode == 0x9000335 || mode == 0x9000388) ? 0x9000373 : 0x900039a;
                a.SetAnimType(AlienAnim(a, anim), 0, 1);
                L[6] = 0;
            }
        }
        a.InvalidateGotoPos();
        float d2 = Alien_DestDist3(a);
        if (d2 < 1.0f) {
            L[5] = 6;
            return 1;
        }
        if (36.0f < d2) {
            L[5] = 1;
            return 1;
        }
        if (--(int &)L[7] >= 0) return 1;
        if (g_EXRandClass.Rand32() % 100 < (unsigned int)((bAlone ? 0x28 : 0) + 0x32)) {
            L[5] = 1;
            goto retime;
        }
        goto reside;
    }
    case 3: {
        a.SetAnimType(AlienAnim(a, L[6] == 0 ? 0x9000351 : 0x9000352), 0, 1);
        float dx = a.m_DestLoc.m_vPos.x - a.GetFeetPos().x;
        float dy = a.m_DestLoc.m_vPos.y - a.GetFeetPos().y;
        float dz = a.m_DestLoc.m_vPos.z - a.GetFeetPos().z;
        float dw = a.m_DestLoc.m_vPos.w - a.GetFeetPos().w;
        float l2 = dx * dx + dy * dy + dz * dz + dw * dw;
        if (l2 >= 0.0f) {
            float l = EXSqrt(l2);
            if (l > 1e-5f) {
                float inv = 1.0f / l;
                dx *= inv;
                dy *= inv;
                dz *= inv;
                dw *= inv;
            }
        }
        EXVector t;
        t.x = dx * 10.0f + a.GetFeetPos().x;
        t.y = a.GetFeetPos().y + dy * 10.0f;
        t.z = a.GetFeetPos().z + dz * 10.0f;
        t.w = a.GetFeetPos().w + dw * 10.0f;
        a.GotoPosSlow(t);
        a.InvalidateGotoPos();
        L[10] = 1;
        L[5] = 4;
        return 1;
    }
    case 4:
        if (a.HasAnimEnded()) L[5] = L[10];
        return 1;
    case 5:
        a.InvalidateGotoPos();
        if (a.HasAnimEnded()) {
            L[5] = 2;
            if (bAlone) L[7] = g_EXRandClass.Rand32() % 15;
            else L[7] = g_EXRandClass.Rand32() % 60 + 30;
            a.SetAnimType(AlienAnim(a, L[6] == 0 ? 0x9000342 : 0x9000335), 0, 1);
        }
        return 1;
    case 6:
        if (L[9]) goto struck6;
        a.ClrAnimMode();
        a.SetAnimType(AlienAnim(a, 0x9000392), 0, 1);
        a.InvalidateGotoPos();
        if (Alien_DestDist3(a) <= 12.25f) return 1;
        L[5] = 2;
        goto reside;
    case 7:
        a.ClrAnimMode();
        a.SetAnimType(AlienAnim(a, 0x90004c4), 0, 1);
        L[5] = 8;
        return 1;
    case 8:
        if (a.HasAnimEnded()) {
            AlienTriggerJump(a, (EXVector &)L[0], (int)L[4]);
            a.SetAnimType(AlienAnim(a, 0x900042a), 0, 1);
            L[5] = 9;
        }
        return 1;
    }
    return 1;

struck:
    L[9] = 0;
    a.SetAnimType(AlienAnim(a, 0x90004bc), 0, 1);
    L[10] = 0;
    L[5] = 4;
    return 1;
struck6:
    L[9] = 0;
    a.SetAnimType(AlienAnim(a, 0x90004bc), 0, 1);
    L[10] = 6;
    L[5] = 4;
    return 1;
reside:
    L[6] = g_EXRandClass.Rand32() & 1;
retime:
    if (bAlone) L[7] = g_EXRandClass.Rand32() % 15;
    else L[7] = g_EXRandClass.Rand32() % 60 + 30;
    return 1;
}

// Pushed over the attack state when the alien flinches. The wasted Rand32 in the strafe case is
// retail's: the anim is always the same.
signed char STATE_Alien_HitReact(StateMachine &sm)
{
    Actor &a = sm.GetActorOwner();
    switch (sm.m_Msg.m_Type) {
    case MSG_NONE:
        a.IsMsgOnStack(MSG_NONE);
        return 1;
    case MSG_ENTER: {
        a.InvalidateGotoPos();
        sm.m_LocalData[0] = 0;
        a.SetNav3D(1);
        int anim = 0x90004bc;
        switch (a.m_AnimMode) {
        case 0x900031b: case 0x900032d: case 0x9000335: case 0x9000340: case 0x9000341: case 0x9000342:
            g_EXRandClass.Rand32();
            anim = 0x90004ed;
            break;
        }
        a.SetAnimType(AlienAnim(a, anim), 0, 1);
        sm.m_LocalData[1] = 600;
        return 1;
    }
    case MSG_EXT:
        a.InvalidateGotoPos();
        if (a.HasAnimEnded() || (sm.m_LocalData[0] == 0 && --(int &)sm.m_LocalData[1] < 0))
            sm.Return(0, 0, 0, 0);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!a.IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            Alien_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case (MsgType)0x2e:
        a.IsMsgOnStack((MsgType)0x2e);
        return 1;
    case (MsgType)0x2f:
        a.IsMsgOnStack((MsgType)0x2f);
        return 1;
    case (MsgType)0x30:
        a.IsMsgOnStack((MsgType)0x30);
        return 1;
    }
    return 0;
}

// Mid-air after a leap: replays the jump anim until the landing message, then lets it finish.
signed char STATE_Alien_Jumping(StateMachine &sm)
{
    Actor &a = sm.GetActorOwner();
    a.m_WatchFlags = 4;
    switch (sm.m_Msg.m_Type) {
    case MSG_NONE:
        a.IsMsgOnStack(MSG_NONE);
        return 1;
    case MSG_ENTER:
        sm.m_LocalData[0] = 0;
        a.SetAnimType(AlienAnim(a, 0x900042a), 0, 1);
        a.NavRoute().m_Flags2.m_b1 = 1;
        a.GotoPosPoly(gAIControl.PlayerNavLoc().m_vFloorPos, gAIControl.PlayerNavLoc().m_nPolyHandle);
        return 1;
    case MSG_EXT:
        if (sm.m_LocalData[0] == 0) a.SetAnimType(AlienAnim(a, 0x900042a), 0, 1);
        if (sm.m_LocalData[0] == 1 && a.HasAnimEnded()) sm.Return(0, 0, 0, 0);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!a.IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            Alien_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case (MsgType)0x26:
        if (!a.IsMsgOnStack((MsgType)0x26)) {
            a.SetAnimType(AlienAnim(a, 0x90004dd), 0, 1);
            sm.m_LocalData[0] = 1;
        }
        return 1;
    }
    return 0;
}

// Hanging from the ceiling: waits for the player to come within 15, hangs a while, drops, lands,
// then turns into a normal attacker. Locals: [0] phase, [1] hang timer.
signed char STATE_Alien_CeilingHang(StateMachine &sm)
{
    Actor &a = sm.GetActorOwner();
    unsigned int *L = sm.m_LocalData;
    a.m_WatchFlags = 4;
    switch (sm.m_Msg.m_Type) {
    case MSG_NONE:
        a.IsMsgOnStack(MSG_NONE);
        return 1;
    case MSG_ENTER:
        a.m_StateFlags.m_bCanRipBossHead = 0;
        L[0] = 0;
        a.SetNav3D(1);
        a.NavRoute().m_Flags.m_b5 = 1;
        a.SetAnimType(AlienAnim(a, 0x9000423), 0, 1);
        a.OrgLoc().m_vPos = a.OrgLoc().m_vFloorPos;
        a.GotoPosPoly(gAIControl.PlayerNavLoc().m_vFloorPos, gAIControl.PlayerNavLoc().m_nPolyHandle);
        return 1;
    case MSG_EXIT:
        a.m_AnimMotionOverride &= ~0x200u;
        return 1;
    case MSG_EXT:
        switch (L[0]) {
        case 0: {
            EXVector d = a.OrgLoc().m_vPos - gAIControl.PlayerNavLoc().m_vPos;
            float d2 = d.y * d.y + d.z * d.z + d.w * d.w + d.x * d.x;
            if (d2 < 0.0f || EXSqrt(d2) < 15.0f) {
                L[1] = g_EXRandClass.Rand32() % 120 + 60;
                L[0]++;
            }
            return 1;
        }
        case 1:
            if (--(int &)L[1] < 0) {
                a.m_AnimMotionOverride |= 0x200;
                a.SetAnimType(AlienAnim(a, 0x900041e), 0, 1);
                a.NavRoute().m_Flags2.m_b1 = 1;
                a.NavRoute().m_Flags.m_b5 = 0;
                L[0]++;
            }
            return 1;
        case 2:
            if (a.HasAnimEnded()) {
                a.SetAnimType(AlienAnim(a, 0x900042a), 0, 1);
                L[0]++;
            }
            return 1;
        case 4:
            if (a.HasAnimEnded()) L[0]++;
            return 1;
        case 5:
            SM_GOTO(STATE_Alien_Attack, 0, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!a.IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            Alien_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case (MsgType)0x26:
        if (!a.IsMsgOnStack((MsgType)0x26)) {
            a.m_AnimMotionOverride &= ~0x200u;
            a.SetAnimType(AlienAnim(a, 0x90004dd), 0, 1);
            L[0] = 4;
        }
        return 1;
    }
    return 0;
}

signed char STATE_Alien_Attack2(StateMachine &sm)
{
    sm.GetActorOwner().m_WatchFlags = 4;
    if (sm.m_Msg.m_Type != MSG_ENTER) return 0;
    sm.Goto(GetMsgBitsForMsg(sm.m_Msg.m_Type), STATE_Alien_Attack, 0, 0, 0, 0);
    return 1;
}

signed char STATE_Alien_Attack3(StateMachine &sm)
{
    sm.GetActorOwner().m_WatchFlags = 4;
    if (sm.m_Msg.m_Type != MSG_ENTER) return 0;
    sm.Goto(GetMsgBitsForMsg(sm.m_Msg.m_Type), STATE_Alien_Attack, 0, 0, 0, 0);
    return 1;
}

signed char STATE_Alien_Attack4(StateMachine &sm)
{
    sm.GetActorOwner().m_WatchFlags = 4;
    if (sm.m_Msg.m_Type != MSG_ENTER) return 0;
    sm.Goto(GetMsgBitsForMsg(sm.m_Msg.m_Type), STATE_Alien_Attack, 0, 0, 0, 0);
    return 1;
}

signed char STATE_Alien_Attack5(StateMachine &sm)
{
    sm.GetActorOwner().m_WatchFlags = 4;
    if (sm.m_Msg.m_Type != MSG_ENTER) return 0;
    sm.Goto(GetMsgBitsForMsg(sm.m_Msg.m_Type), STATE_Alien_Attack, 0, 0, 0, 0);
    return 1;
}

signed char STATE_Alien_CeilingHang2(StateMachine &sm)
{
    sm.GetActorOwner().m_WatchFlags = 4;
    if (sm.m_Msg.m_Type != MSG_ENTER) return 0;
    sm.Goto(GetMsgBitsForMsg(sm.m_Msg.m_Type), STATE_Alien_CeilingHang, 0, 0, 0, 0);
    return 1;
}

signed char STATE_Alien_CeilingHang3(StateMachine &sm)
{
    sm.GetActorOwner().m_WatchFlags = 4;
    if (sm.m_Msg.m_Type != MSG_ENTER) return 0;
    sm.Goto(GetMsgBitsForMsg(sm.m_Msg.m_Type), STATE_Alien_CeilingHang, 0, 0, 0, 0);
    return 1;
}

// NEAR: retail clears m_bCanRipBossHead with a direct and-to-memory; ours loads/ands/stores
signed char STATE_Alien_HoleDweller(StateMachine &sm)
{
    sm.GetActorOwner().m_WatchFlags = 4;
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        sm.GetActorOwner().m_StateFlags.m_bCanRipBossHead = 0;
        sm.GetLocalData()[0] = 0;
        sm.GetActorOwner().SetNav3D(1);
        sm.GetActorOwner().SetAnimType(AlienAnim(sm.GetActorOwner(), 0x900042b), 0, 1);
        sm.GetActorOwner().GotoPosPoly(gAIControl.PlayerNavLoc().FloorPos(), gAIControl.PlayerNavLoc().GetPolyHandle());
        return 1;
    case MSG_EXT:
        return 1;
    case MSG_NONE:
        if (sm.GetActorOwner().IsMsgOnStack(MSG_NONE)) {}
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            Alien_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

signed char STATE_Alien_HoleDweller2(StateMachine &sm)
{
    sm.GetActorOwner().m_WatchFlags = 4;
    if (sm.m_Msg.m_Type != MSG_ENTER) return 0;
    sm.Goto(GetMsgBitsForMsg(sm.m_Msg.m_Type), STATE_Alien_HoleDweller, 0, 0, 0, 0);
    return 1;
}

signed char STATE_Alien_HoleDweller3(StateMachine &sm)
{
    sm.GetActorOwner().m_WatchFlags = 4;
    if (sm.m_Msg.m_Type != MSG_ENTER) return 0;
    sm.Goto(GetMsgBitsForMsg(sm.m_Msg.m_Type), STATE_Alien_HoleDweller, 0, 0, 0, 0);
    return 1;
}

// NEAR: same and-to-memory issue as HoleDweller; MSG_EXT Goto reloads m_Msg.m_Type in retail
signed char STATE_Alien_Sonic_Hit(StateMachine &sm)
{
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        sm.GetActorOwner().m_StateFlags.m_bCanRipBossHead = 0;
        sm.m_LocalData[0] = 0;
        sm.GetActorOwner().SetNav3D(1);
        sm.GetActorOwner().SetAnimType(AlienAnim(sm.GetActorOwner(), 0x900042b), 0, 1);
        sm.m_LocalData[1] = EXRand(600) + 600;
        return 1;
    case MSG_EXT:
        if (sm.m_LocalData[0] == 0 && --(int &)sm.m_LocalData[1] < 0) {
            sm.Goto(GetMsgBitsForMsg(sm.m_Msg.m_Type), STATE_Alien_Attack, 0, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_NONE:
        if (sm.GetActorOwner().IsMsgOnStack(MSG_NONE)) {}
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            Alien_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

signed char STATE_TestAlien_Init(StateMachine &sm)
{
    sm.GetActorOwner().m_WatchFlags = 4;
    if (sm.m_Msg.m_Type != MSG_ENTER) return 0;
    sm.Goto(GetMsgBitsForMsg(sm.m_Msg.m_Type), STATE_Alien_Attack, 0, 0, 0, 0);
    return 1;
}
