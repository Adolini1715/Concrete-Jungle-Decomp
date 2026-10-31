#include "Actor.h"
#include "ActorUtil.h"
#include "ActorCombat.h"
#include "StateMachine.h"
#include "EXRand.h"
#include "ActorProfile.h"
#include "XApp.h"
#include "ActorCarryable.h"
#include "FEText.h"
#include "WeaponTable.h"
#include "AIControl.h"
#include "EXHashCodeFile.h"
#include "XItemHandler_Player.h"
#include "GeoSectionManager.h"
#include "EXQMath.h"
#include "XItemHandler_Bullet.h"
#include "XNavLocation.h"
#include "EXMatrix.h"
#include "EXStdFuncs.h"
#include "ActorLocation.h"
#include "EXBaseGamePad.h"
#include "AIFormation.h"
#include "XNavNetwork.h"
#include "SubGroupAnims.h"
#include "ActorOptions.h"
#include "EXDatum.h"
#include "XItemHandler_ScriptBase.h"
#include "EXIntersection.h"
#include "EXCapsule.h"
#include "EXSphere.h"
#include "EXGeoMap.h"
#include "EXGeoPathNode.h"
#include "EXLine.h"
#include "Vehicle.h"
#include "XGameWnd.h"
#include <math.h>

// Enemy combat AI states and helpers (ActorCombat.obj).

void CreateActorDroppedMine(Actor &actor);  // TODO stand-in: owning header
extern unsigned int gCurrentMap_GeoHash;
void Objective_NotifyAlarm(Actor *pActor);                     // TODO stand-in: Objective.obj
float GetRotation(EXVector &v);                                // TODO stand-in: Player.obj
float CalculateAngle(float, float, float, float, signed char);  // TODO stand-in: Player.obj

GunCombatAnims oGunAnimsHighAim = { { 0x0900020c, 0x09000274, 0x09000213, 0x09000212, 0x09000213, 0x09000212,
                                      0x09000211, 0x09000210, 0x0900020f, 0x0900020e, 0x0900020b } };
GunCombatAnims oGunAnimsLowAim = { { 0x09000113, 0x0900011a, 0x09000118, 0x09000116, 0x09000118, 0x09000116,
                                     0x0900011f, 0x0900011e, 0x09000121, 0x09000124, 0x09000115 } };
GunCombatAnims oGunAnimsHandAim = { { 0x090001ff, 0x09000106, 0x090001f8, 0x090001fa, 0x090001f8, 0x090001fa,
                                      0x090001fb, 0x090001fc, 0x090001fd, 0x090001fe, 0x090001f9 } };
GunCombatAnims oGunAnimsThrowing = { { 0, 0, 0x090001f0, 0x090001f1, 0x090000e7, 0x090000e6, 0x090000ef,
                                       0x4e000002, 0x090000ef, 0x4e000003, 0 } };
GunCombatAnims oGunAnimsBossAim = { { 0x0900026b, 0x4e000004, 0x4e000007, 0x4e000008, 0x4e00000b, 0x4e00000c,
                                      0x4e00000a, 0x4e000002, 0x4e00000a, 0x4e000003, 0x09000235 } };
static int s_aDiveAnims[8] = { 0x090001b7, 0x090001b6, 0x090001b7, 0x090001b6,
                               0x090001b7, 0x090001b6, 0x090001b8, 0x090001b9 };
static int s_aDodgeAnims[2] = { 0x090001b7, 0x090001b6 };
static int s_aStrikeOneArmA[4] = { 0x09000134, 0x0900012e, 0x0900012f, 0x09000130 };  // unarmed strikes
static int s_aStrikeOneArmB[4] = { 0x09000131, 0x09000132, 0x09000133, 0x0900012d };
static int s_aThrowAnims[1] = { 0x090001b5 };
static int s_aBlockAnims[1] = { 0x09000265 };  // non-Ronin block anim (STATE_Block)  // indexed EXRand32() % 1 by the grenade states
static int s_aHunter1Strikes[2] = { 0x0900021b, 0x0900021c };  // STATE_Hunter1_PredAttack close-range strike
float g_fZombieBossNoAttackTimer = 600.0f;
int g_ulZombieBossAttacker = -1;
unsigned int g_ulZombieBossID;
unsigned int g_ulZombieBossFrame;
float g_fZombieBossAttackTimer;
float g_fZombieBossSeenTimeout;
unsigned int g_ulLookAtAnim;

GunCombatAnims &Actor_GetAimAnimsSet(const Actor &actor, WeaponProfile *pWeapon)
{
    if (pWeapon) {
        if (actor.GetSkelHash() == 0xd000091) return oGunAnimsBossAim;
        switch (pWeapon->m_AimType) {
        case 0x52000007:
            return oGunAnimsThrowing;
        case 0x52000005:
            return oGunAnimsHighAim;
        case 0x52000004:
            return oGunAnimsHandAim;
        case 0x52000006:
            break;
        default:
            EXHashCodeFile::Debug_HashCodeName(pWeapon->m_Id);
            break;
        }
    }
    return oGunAnimsLowAim;
}

signed char IsOutsideFightDist(Actor &actor, Actor *pTarget, float mult)
{
    EXMaths::vec_xyzw d = actor.GetFeetPos().vec_xyzw::operator-(pTarget->GetFeetPos());
    float r = Sq(actor.GetFightDistance() * (mult + 1.0f));
    if (r < d.x * d.x + d.z * d.z + d.y * d.y) return 1;
    return 0;
}

// Retail inlines vec_xyzw::operator- in places where VC7.1 will not; a local forced copy stands in.
static __forceinline EXMaths::vec_xyzw VSub(const EXMaths::vec_xyzw &a, const EXMaths::vec_xyzw &b)
{
    EXMaths::vec_xyzw r;
    r.x = a.x - b.x; r.y = a.y - b.y; r.z = a.z - b.z; r.w = a.w - b.w;
    return r;
}

// EXVector::Normalise, which retail inlines into the grenade/aim code; VC7.1 will not.
static __forceinline float NormaliseInline(EXVector &v)
{
    float len = v.Length();
    if (len > EX_EPSILON_UNIT) v *= 1.0f / len;
    return len;
}

signed char STATE_RoninPredAttackBlock(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    float &blockTimer = sm.GetFloatRef(i);
    float &seenTimer = sm.GetFloatRef(i);
    float &elapsed = sm.GetFloatRef(i);
    float &animTimer = sm.GetFloatRef(i);
    EXVector &dir = sm.GetVectorRef(i);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT: {
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        blockTimer -= EXBaseApp::GameFrameRateMul();
        elapsed += EXBaseApp::GameFrameRateMul();
        if (elapsed > EXBaseApp::NominalGameFrameRate() * 2.5f) {
            sm.GetActorOwner().SetBlockCounter(EXBaseApp::NominalGameFrameRate());
            sm.Return((int)STATE_Block, 1, 0, 0);
            return 1;
        }
        signed char seen = Actor_CanSeeTarget(sm.GetActorOwner(), 1, seenTimer, sm.GetActorOwner().GetCombatTimeout());
        switch (stage) {
        case 0: {
            Actor &a = sm.GetActorOwner();
            a.SetAnimType(GetRoninBlockAnimation(a, dir), 0, 1);
            stage = 1;
            animTimer = EXBaseApp::NominalGameFrameRate() * 0.25f;
            return 1;
        }
        case 1:
            if (animTimer > 0.0f) animTimer -= XApp::GameFrameRateMul();
            sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
            sm.GetActorOwner().SetBlockCounter(40);
            if (0.0f >= blockTimer || !seen) {
                sm.Return(0, 0, 0, 0);
                return 1;
            }
            return 1;
        }
        return 1;
    }
    case MSG_EXIT:
        return 1;
    case MSG_ENTER:
        blockTimer = EXBaseApp::NominalGameFrameRate();
        stage = 0;
        elapsed = 0.0f;
        dir = ((StructMsg_IncomingAttack *)sm.m_Msg.m_Data[0])->m_Dir;
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_INCOMING_ATTACK:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_INCOMING_ATTACK)) {
            if (stage == 2) stage = 0;
            blockTimer = EXBaseApp::NominalGameFrameRate();
            StructMsg_IncomingAttack *pMsg = (StructMsg_IncomingAttack *)sm.m_Msg.m_Data[0];
            if (animTimer <= 0.0f) {
                Actor &a = sm.GetActorOwner();
                a.SetAnimType(GetRoninBlockAnimation(a, pMsg->m_Dir), 0, 1);
                animTimer = EXBaseApp::NominalGameFrameRate() * 0.25f;
            }
            if (pMsg->m_Unk4 != -1) AU_HandleIncomingAttack(sm, (StructMsg_IncomingAttack *)sm.m_Msg.m_Data[0]);
        }
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

static __forceinline EXMaths::vec_xyzw VAdd(const EXMaths::vec_xyzw &a, const EXMaths::vec_xyzw &b)
{
    EXMaths::vec_xyzw r;
    r.x = a.x + b.x; r.y = a.y + b.y; r.z = a.z + b.z; r.w = a.w + b.w;
    return r;
}

static __forceinline float ForceSqrt(float f) { return f >= 0.0f ? (float)sqrt(f) : 0.0f; }

// NEAR: 16 bytes, squared-length register choice and a reload of the 4.0f constant for the bias
int Actor_IsPredatorCloser(Actor &actor, int target)
{
    if (target == 1) return target;
    if (actor.GetProfile()->m_Group == 0x4b00000b) return target;
    float targetDistSq = 1e8f;
    Actor *p = Actor::GetPtrFromID(target);
    if (p) {
        EXMaths::vec_xyzw d = VSub(p->GetFeetPos(), actor.GetFeetPos());
        targetDistSq = d.LengthXYZSquared();
    }
    if (actor.WasPlayerSeen()) {
        float fd = actor.GetFightDistance();
        EXMaths::vec_xyzw d = VSub(gAIControl.LastSeenPlayerPos(), actor.GetFeetPos());
        float playerDistSq = d.LengthXYZSquared();
        if (fd * fd > playerDistSq) {
            if (4.0f > playerDistSq) return 1;
            float bias = 0.0f;
            if (gCurrentMap_GeoHash == 0x100006a) bias = 4.0f;
            if (ForceSqrt(playerDistSq) - bias < ForceSqrt(targetDistSq)) return 1;
        }
    }
    return target;
}

signed char Actor_TargetIsValid(int id)
{
    if (id == 1) return 1;
    Actor *p = Actor::GetPtrFromID(id);
    if (p && !p->IsDead()) return !p->InSafeArea();
    return 0;
}

EXVector Actor_GetTargetFeetPos(int id)
{
    if (!id) return EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    if (id == 1) return gAIControl.LastSeenPlayerPos();
    Actor *p = Actor::GetPtrFromID(id);
    if (!p) return EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    return p->GetFeetPos();
}

// NEAR: 16 bytes differ, register allocation (eax/ecx swapped)
signed char Actor_DelayedSightToTarget(Actor &actor, int &timer, int &seen, int delay)
{
    int now = actor.WasPlayerSeen();
    if (now != seen && !timer) {
        timer = delay;
        return (signed char)seen;
    }
    if (timer == 1) {
        seen = now;
        timer = 0;
    }
    return (signed char)seen;
}


// Retail inlines mat_44::set_rotate_xyz here; VC7.1 will not, so a local copy is forced in.
static __forceinline void SetRotationXYZInline(EXMaths::mat_44 &m, const EXMaths::vec_xyzw &a)
{
    float sx, cx, sy, cy, sz, cz;
    EXSinCos(a.x, sx, cx);
    EXSinCos(a.y, sy, cy);
    EXSinCos(a.z, sz, cz);
    m.r[0].x = cz * cy; m.r[0].y = cz * sy * sx + sz * cx; m.r[0].z = sz * sx - cz * sy * cx; m.r[0].w = 0.0f;
    m.r[1].x = -(sz * cy); m.r[1].y = cz * cx - sz * sy * sx; m.r[1].z = sz * sy * cx + cz * sx; m.r[1].w = 0.0f;
    m.r[2].x = sy; m.r[2].y = -(cy * sx); m.r[2].z = cy * cx; m.r[2].w = 0.0f;
    m.r[3].x = 0.0f; m.r[3].y = 0.0f; m.r[3].z = 0.0f; m.r[3].w = 1.0f;
}

// NEAR: written for behaviour; retail routes the segment end points and deltas through extra stack copies
signed char Actor_CheckForAttack(Actor &actor, StructMsg_IncomingAttack *pMsg)
{
    if (!pMsg) return 0;
    EXVector start = pMsg->m_Pos;
    EXVector end(pMsg->m_Pos.x + pMsg->m_Dir.x * 1000.0f, pMsg->m_Pos.y + pMsg->m_Dir.y * 1000.0f,
                 pMsg->m_Pos.z + pMsg->m_Dir.z * 1000.0f, pMsg->m_Pos.w + pMsg->m_Dir.w * 1000.0f);
    const EXVector &feet = actor.GetFeetPos();
    EXVector flat(feet.x, 0.0f, feet.z, feet.w);
    end.y = 0.0f;
    start.y = 0.0f;
    EXVector np = XNavNetwork::NearestPointOnLineSegment(start, end, flat, 0);
    EXMaths::vec_xyzw d = VSub(np, flat);
    if (d.y * d.y + d.z * d.z + d.x * d.x > 9.0f) return 0;
    EXMaths::vec_xyzw t = VSub(actor.GetFeetPos(), pMsg->m_Pos);
    WeaponProfile *pWeapon = WeaponProfile::GetWeaponData(pMsg->m_Weapon);
    if (t.y * t.y + t.z * t.z + t.x * t.x > pWeapon->m_DodgeRadius * pWeapon->m_DodgeRadius) return 0;
    float a = actor.GetRotation().y - 0.7853982f;
    float s = sin(a);
    float c = cos(a);
    if (t.z * c + t.x * s + t.y * 0.0f > 0.0f) return 0;
    if (t.z * -s + t.x * c + t.y * 0.0f > 0.0f) return 0;
    return 1;
}

// NEAR: written for behaviour; retail keeps extra vec_xyzw copies of both deltas
EXVector GetApproachPosition(int slot, const EXVector &target)
{
    EXVector slotPos;
    EXVector origin;
    g_Formations.GetSlotPos(slot, slotPos);
    g_Formations.GetOrigin(origin);
    EXVector a = VSub(slotPos, origin);
    EXVector b = VSub(target, origin);
    if (0.0f > b.z * a.z + b.x * a.x) {
        EXVector perp(-b.z, 0.0f, b.x, 1.0f);
        NormaliseInline(perp);
        float len = ForceSqrt(a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w);
        return EXVector(origin.x + len * perp.x, origin.y + len * perp.y, origin.z + len * perp.z, origin.w + len * perp.w);
    }
    return slotPos;
}

// NEAR: x87/SSE scheduling of the inlined rotation-matrix products only
void SetOffsetVector(EXVector &v)
{
    EXVector a;
    a.x = EXRandf() * 6.2831855f;
    a.y = EXRandf() * 6.2831855f;
    a.z = EXRandf() * 6.2831855f;
    EXMatrix m;
    SetRotationXYZInline(m, a);
    v = EXVector(0.0f, 0.0f, 1.0f, 1.0f);
    v *= m;
    if (v.y < 0.0f) v.y = -v.y;
    v.y = 0.0f;
    v *= 1.5f;
}

void ShrinkOffsetVector(EXVector &v)
{
    v *= 0.98f;
}

// NEAR: retail keeps two extra copies of the negated direction (stack 0x30), as the other knockbacks
signed char STATE_KnockBackHunter1(StateMachine &sm)
{
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        if (sm.GetActorOwner().HasAnimEnded()) {
            sm.Return((int)STATE_KnockBackHunter1, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_ENTER: {
        sm.GetActorOwner().ClearConsecutiveHitCounter();
        sm.GetActorOwner().SetDontDoHitReaction(1);
        sm.GetActorOwner().InvalidateGotoPos();
        EXVector dir = -*(EXVector *)sm.m_Msg.m_Data[0];
        sm.GetActorOwner().SetRotY(qatan2f(dir.x, dir.z));
        sm.GetActorOwner().SetAnimType(0x9000225, 0, 1);
        return 1;
    }
    }
    return 0;
}

signed char STATE_ChangeWeaponMode(StateMachine &sm)
{
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetStatus(0xd);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER: {
        ActorCarryableGun *pGun = sm.GetActorOwner().GetGunPtr(0);
        if (pGun && pGun->GetWeaponProfile()->m_Id == 0x49000041) pGun->m_LocationType = 0x8000;
        pGun = sm.GetActorOwner().GetGunPtr(1);
        if (pGun && pGun->GetWeaponProfile()->m_Id == 0x49000041) pGun->m_LocationType = 0x8000;
        sm.Return(0, 0, 0, 0);
        return 1;
    }
    }
    return 0;
}

signed char STATE_StandExpandedFightDist(StateMachine &sm)
{
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        sm.GetActorOwner().ExpandFightDist();
        sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// NEAR: ~140 bytes differ, retail keeps aim+off in xmm1-4 before reading the player delta (schedule)
EXVector GetDelayedTarget(Actor &actor, const EXVector &off)
{
    EXVector delta;
    EXVector aim = actor.GetAimPosForTarget(1);
    EXVector r(off.x + aim.x, off.y + aim.y, off.z + aim.z, off.w + aim.w);
    XGetPlayer()->GetDelta(delta);
    return EXVector(r.x - delta.x * 10.0f, r.y - delta.y * 10.0f, r.z - delta.z * 10.0f, r.w - delta.w * 10.0f);
}

// NEAR: written for behaviour (1856 bytes); register/stack-slot residue around the shape and delta copies
void StrikeCharacter(StateMachine &sm, signed char left, int &flags, WeaponProfile *pWeapon, signed char bFlag, int target)
{
    Actor *pTarget = 0;
    if (target != 1) {
        pTarget = Actor::GetPtrFromID(target);
        if (!pTarget) return;
    }
    if (left) {
        if (!(flags & 1)) return;
    } else {
        if (!(flags & 2)) return;
    }
    EXSphere sphere;
    sphere.Set(sm.GetActorOwner().GetWeaponPos(left), 0.4f);
    EXCapsule capsule;
    capsule.Set(target == 1 ? Player_GetFeetPos() : pTarget->GetFeetPos(), EXVector(0.0f, 1.5f, 0.0f, 0.0f), 0.5f);
    if (!EXIntersection(sphere, capsule)) return;
    if (pWeapon->m_Id == 0x4900002a && pWeapon->m_ExplosionScript) {
        EXDatumTemp datum;
        if (!sm.GetActorOwner().GetItem()->GetAnimatorDatum(0x1000009d, datum, 0, 0)) {
            EXHashCodeFile::Debug_HashCodeName(pWeapon->m_Id);
            return;
        }
        EXVector pos = datum.Position();
        XItemHandler_ScriptBase::FX_Create(pos, pWeapon->m_ExplosionScript, 0x40, 0x1000016);
    }
    float damage = pWeapon->GetMeleeDamage();
    if (sm.GetActorOwner().GetAnimMode() == 0x90002b8) damage *= 2.0f;
    if (target == 1) {
        int kind = 0;
        if (flags & 4)
            kind = 1;
        else if (flags & 8)
            kind = 2;
        else if (flags & 0x10)
            kind = 3;
        EXVector wpos = sm.GetActorOwner().GetWeaponPos(left);
        EXVector dir = VSub(Player_GetFeetPos(), wpos);
        int reaction = pWeapon->m_DamageClass;
        switch (kind) {
        case 1:
            if (reaction == 0x53000000 || reaction == 0x53000002)
                XItemHandler_Player::Instance()->SendPredMsg(0, wpos, dir, damage);
            if (reaction == 0x53000001 || reaction == 0x53000002) XItemHandler_Player::Instance()->SendPredMsg(6, damage);
            wpos = sm.GetActorOwner().GetFeetPos();
            XItemHandler_Player::Instance()->SendPredMsg(5, wpos, 0.1f);
            break;
        case 2:
            if (reaction == 0x53000000 || reaction == 0x53000002)
                XItemHandler_Player::Instance()->SendPredMsg(0, wpos, dir, damage);
            if (reaction == 0x53000001 || reaction == 0x53000002) XItemHandler_Player::Instance()->SendPredMsg(6, damage);
            wpos = sm.GetActorOwner().GetFeetPos();
            XItemHandler_Player::Instance()->SendPredMsg(5, wpos, 1.0f);
            break;
        case 3:
            XItemHandler_Player::Instance()->SendPredMsg(7, wpos, dir, damage);
            break;
        default:
            if (reaction == 0x53000000 || reaction == 0x53000002)
                XItemHandler_Player::Instance()->SendPredMsg(0, wpos, dir, damage);
            if (reaction == 0x53000001 || reaction == 0x53000002) XItemHandler_Player::Instance()->SendPredMsg(6, damage);
            break;
        }
        sm.GetActorOwner().SetHasHitPredRecently();
        sm.GetActorOwner().PlaySFX(pWeapon->m_MeleeDamage > 0.0f ? pWeapon->m_SfxHitFixed : pWeapon->m_SfxHit, 0);
    } else {
        EXVector dir = VSub(pTarget->GetFeetPos(), sm.GetActorOwner().GetFeetPos());
        SendMsg_AttackedByActor(pTarget, pWeapon, pTarget->GetFeetPos(), dir, damage, &sm.GetActorOwner(), bFlag);
        Actor *p = Actor::GetPtrFromID(target);
        if (p && p->IsUlysses())
            sm.GetActorOwner().PlaySFX(0x1af0028d, 0);
        else
            sm.GetActorOwner().PlaySFX(pWeapon->m_MeleeDamage > 0.0f ? pWeapon->m_SfxHitFixed : pWeapon->m_SfxHit, 0);
    }
    if (left)
        flags &= ~0x1d;
    else
        flags &= ~0x1e;
    if (target == 1 && sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f00000f) {
        EXVector feet = sm.GetActorOwner().GetFeetPos();
        EXSphere s2;
        s2.Set(feet, 2.0f);
        EXCapsule c2;
        c2.Set(Player_GetFeetPos(), EXVector(0.0f, 1.0f, 0.0f, 0.0f), 1.0f);
        if (EXIntersection(s2, c2)) XItemHandler_Player::Instance()->SendPredMsg(5, feet, 0.5f);
    }
}

// NEAR: written for behaviour; retail's attack-delta copy and side/facing products are scheduled differently
signed char STATE_Dodge(StateMachine &sm)
{
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (sm.GetActorOwner().HasAnimEnded()) {
            sm.GetActorOwner().SetDontDoHitReaction(0);
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_ENTER: {
        StructMsg_IncomingAttack *pMsg = (StructMsg_IncomingAttack *)sm.m_Msg.m_Data[0];
        if (!pMsg) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        EXMaths::vec_xyzw p = pMsg->m_Pos;
        p.y = 0.0f;
        EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), p);
        float s = sin(sm.GetActorOwner().GetRotation().y);
        float side = d.z * -pMsg->m_Dir.x + d.x * pMsg->m_Dir.z;
        float facing = cos(sm.GetActorOwner().GetRotation().y) * pMsg->m_Dir.z + s * pMsg->m_Dir.x;
        int right;
        if (side > 0.0f)
            right = facing > 0.0f ? 1 : 0;
        else
            right = facing > 0.0f ? 0 : 1;
        WeaponProfile *pWeapon = WeaponProfile::GetWeaponData(pMsg->m_Weapon);
        if (!pWeapon) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        int chance = 4;
        if (pWeapon->IsPredatorProjectileWeapon()) chance = 2;
        signed char canMove = 1;
        if (right == 0) {
            if (!sm.GetActorOwner().CanStrafeLeft(2.0f)) canMove = 0;
        } else if (right == 1) {
            if (!sm.GetActorOwner().CanStrafeRight(2.0f)) canMove = 0;
        }
        sm.GetActorOwner().SetDontDoHitReaction(1);
        if (right == 0) {
            if (sm.GetActorOwner().IsBoss()) {
                if (canMove && sm.GetActorOwner().CanLeapAbout()) {
                    sm.GetActorOwner().SetAnimType(0x9000232, 0, 1);
                    return 1;
                }
                sm.GetActorOwner().SetAnimType(0x9000374, 0, 1);
                return 1;
            }
            if (canMove && sm.GetActorOwner().IsDiveAllowed() && EXRand(chance) == 0) {
                sm.GetActorOwner().SetAnimType(0x90001b8, 0, 1);
                return 1;
            }
            int r = EXRand(2);
            if (r)
                sm.GetActorOwner().SetAnimType(0x90001b7, 0, 1);
            else
                sm.GetActorOwner().SetAnimType(0x4e00000b, 0, 1);
            return 1;
        }
        if (sm.GetActorOwner().IsBoss()) {
            if (canMove && sm.GetActorOwner().CanLeapAbout()) {
                sm.GetActorOwner().SetAnimType(0x9000233, 0, 1);
                return 1;
            }
            sm.GetActorOwner().SetAnimType(0x9000375, 0, 1);
            return 1;
        }
        if (canMove && sm.GetActorOwner().IsDiveAllowed() && EXRand(chance) == 0) {
            sm.GetActorOwner().SetAnimType(0x90001b9, 0, 1);
            return 1;
        }
        int r = EXRand(2);
        if (r)
            sm.GetActorOwner().SetAnimType(0x90001b6, 0, 1);
        else
            sm.GetActorOwner().SetAnimType(0x4e00000c, 0, 1);
        return 1;
    }
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

// NEAR: 115 bytes differ, retail keeps two extra copies of the negated direction (stack 0x30)
signed char STATE_KnockBackRussian(StateMachine &sm)
{
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        if (sm.GetActorOwner().HasAnimEnded()) {
            sm.GetActorOwner().SetDontDoHitReaction(0);
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_ENTER: {
        sm.GetActorOwner().ClearConsecutiveHitCounter();
        sm.GetActorOwner().SetDontDoHitReaction(1);
        EXVector dir = -*(EXVector *)sm.m_Msg.m_Data[0];
        sm.GetActorOwner().SetRotY(qatan2f(dir.x, dir.z));
        sm.GetActorOwner().SetAnimType(0x9000225, 0, 1);
        return 1;
    }
    }
    return 0;
}

// NEAR: 115 bytes differ, retail keeps two extra copies of the negated direction (stack 0x30)
signed char STATE_KnockBackZombie(StateMachine &sm)
{
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        if (sm.GetActorOwner().HasAnimEnded()) {
            sm.GetActorOwner().SetDontDoHitReaction(0);
            sm.Return((int)STATE_KnockBackZombie, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_ENTER: {
        sm.GetActorOwner().ClearConsecutiveHitCounter();
        sm.GetActorOwner().SetDontDoHitReaction(1);
        EXVector dir = -*(EXVector *)sm.m_Msg.m_Data[0];
        sm.GetActorOwner().SetRotY(qatan2f(dir.x, dir.z));
        sm.GetActorOwner().SetAnimType(0x9000530, 0, 1);
        return 1;
    }
    }
    return 0;
}

// NEAR: 115 bytes differ, retail keeps two extra copies of the negated direction (stack 0x30)
signed char STATE_KnockBackMonsterSquad(StateMachine &sm)
{
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        if (sm.GetActorOwner().HasAnimEnded()) {
            sm.GetActorOwner().SetDontDoHitReaction(0);
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_ENTER: {
        sm.GetActorOwner().ClearConsecutiveHitCounter();
        sm.GetActorOwner().SetDontDoHitReaction(1);
        EXVector dir = -*(EXVector *)sm.m_Msg.m_Data[0];
        sm.GetActorOwner().SetRotY(qatan2f(dir.x, dir.z));
        sm.GetActorOwner().SetAnimType(0x9000225, 0, 1);
        return 1;
    }
    }
    return 0;
}

signed char STATE_ZombieDazed(StateMachine &sm)
{
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_ATTACKED_BY_PREDATOR: {
        if (sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR)) return 1;
        StructMsg_AttackedByPredator *pMsg = (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0];
        if (!pMsg) return 1;
        if (pMsg->m_Attack != 9) {
            sm.GetLocalData()[0] = 1;
            AU_HandlePlayerAttacks(sm, pMsg);
            return 1;
        }
        sm.GetActorOwner().SetAnimType(0x900053a, 0, 1);
        return 1;
    }
    case MSG_EXT:
        if (sm.GetLocalData()[0] || sm.GetActorOwner().HasAnimEnded()) sm.Return(0, 0, 0, 0);
        return 1;
    case MSG_ENTER:
        sm.GetActorOwner().SetAnimType(0x900053a, 0, 1);
        return 1;
    }
    return 0;
}

// NEAR: retail spills the EXRand32()&3 result to a dead stack slot; feet-pos copy register order
signed char STATE_UlysesPredOnHead(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &canThrow = sm.GetIntRef(i);
    int &skin = sm.GetIntRef(i);
    int &count = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        Actor::OverridePlayerCloak();
        if (XItemHandler_Player::Instance()->GetMode() != 0x14 ||
            XItemHandler_Player::Instance()->GetSpecialActorID() != (unsigned int)sm.GetActorOwner().m_RefID)
            stage = 2;
        switch (stage) {
        case 0:
            switch (skin) {
            case 0xd000101:
                sm.GetActorOwner().SetAnimType(0x900028c, 0, 1);
                sm.GetActorOwner().SetDontDoHitReaction(1);
                canThrow = 1;
                stage = 1;
                return 1;
            case 0xd00011b:
                if (count > 0) {
                    sm.GetActorOwner().SetAnimType(0x9000255, 0, 1);
                    count--;
                    stage = 1;
                } else {
                    sm.GetActorOwner().SetAnimType(0x900028c, 0, 1);
                    sm.GetActorOwner().SetDontDoHitReaction(1);
                    canThrow = 1;
                    stage = 1;
                }
                return 1;
            default: {
                int r = EXRand(4);
                if (r) {
                    sm.GetActorOwner().SetAnimType(0x90001f3, 0, 1);
                    stage = 1;
                } else {
                    sm.GetActorOwner().SetAnimType(0x900028c, 0, 1);
                    sm.GetActorOwner().SetDontDoHitReaction(1);
                    canThrow = 1;
                    stage = 1;
                }
                return 1;
            }
            }
            return 1;
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 0;
                return 1;
            }
            return 1;
        case 2:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().SetDontDoHitReaction(0);
                sm.Return((int)STATE_UlysesPredOnHead, 0, 0, 0);
                return 1;
            }
            return 1;
        }
        return 1;
    case MSG_EXIT:
        sm.GetActorOwner().SetDontDoHitReaction(0);
        return 1;
    case MSG_ENTER:
        skin = sm.GetActorOwner().GetSkin()->m_Hash;
        stage = 1;
        canThrow = 0;
        count = EXRand32() % 3 + 2;
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x1e:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e) && canThrow &&
            XItemHandler_Player::Instance()->GetMode() == 0x14 &&
            XItemHandler_Player::Instance()->GetSpecialActorID() == (unsigned int)sm.GetActorOwner().m_RefID) {
            EXVector pos = sm.GetActorOwner().GetFeetPos();
            EXVector off(sin(sm.GetActorOwner().GetRotation().y) * 3.0f, 0.0f, cos(sm.GetActorOwner().GetRotation().y) * 3.0f, 0.0f);
            pos = VSub(pos, off);
            XItemHandler_Player::Instance()->SendPredMsg(5, pos, 0.5f);
            canThrow = 0;
            stage = 2;
        }
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

// NEAR: written for behaviour; SSE register/copy residue around the player-delta vector
EXVector GetUlyssesAimPos(StateMachine &sm)
{
    EXVector head = sm.GetActorOwner().GetHeadDirection();
    const EXVector &feet = sm.GetActorOwner().GetFeetPos();
    EXVector d = VSub(gAIControl.LastSeenPlayerPos(), feet);
    EXVector aim;
    if (16.0f > d.LengthXYZSquared()) {
        NormaliseInline(d);
        aim = EXVector(feet.x + d.x * 4.0f, feet.y + d.y * 4.0f, feet.z + d.z * 4.0f, feet.w + d.w * 4.0f);
    } else {
        aim = gAIControl.LastSeenPlayerPos();
    }
    EXVector side(-head.z, head.y, head.x, head.w);
    return EXVector(aim.x + side.x * 1.15f, aim.y + side.y * 1.15f, aim.z + side.z * 1.15f, aim.w + side.w * 1.15f);
}

// NEAR: written for behaviour; retail routes the offset/chest vectors through extra stack copies
signed char STATE_UlyssesStandAlert(StateMachine &sm)
{
    int i = 0;
    EXVector &aim = sm.GetVectorRef(i);
    float &angle = sm.GetFloatRef(i);
    float &timer = sm.GetFloatRef(i);
    float &aiming = sm.GetFloatRef(i);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
    case MSG_RETURN:
        angle = sm.GetActorOwner().GetRotation().y;
        timer = 0.0f;
        aiming = 0.0f;
        return 1;
    case MSG_EXT:
        if (timer > 0.0f) {
            timer -= XApp::GameFrameRateMul();
        } else {
            int r = EXRand(2);
            float a;
            if (r)
                a = EXRandf() + angle + 0.5f;
            else
                a = EXRandf() + angle - 1.5f;
            EXVector off(sin(a) * 5.0f, 0.0f, cos(a) * 5.0f, 0.0f);
            EXMaths::vec_xyzw p = VAdd(sm.GetActorOwner().GetChestPos(), off);
            aim = p;
            angle = a;
            aiming = 1.0f;
            timer = EXRandf(1.0f, 2.0f) * EXBaseApp::NominalGameFrameRate() * 3.0f;
        }
        if (aiming != 0.0f) {
            sm.GetActorOwner().SetHeadTrackMode(2);
            sm.GetActorOwner().SetAimingPos(aim);
            return 1;
        }
        sm.GetActorOwner().SetHeadTrackMode(0);
        return 1;
    case MSG_EXIT:
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

signed char STATE_UlyssesProtectArea(StateMachine &sm)
{
    sm.Return(0, 0, 0, 0);
    return 1;
}

signed char STATE_UlyssesPoweredDownAttack(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &count = sm.GetIntRef(i);
    sm.GetActorOwner().SetWatch(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        SM_GOTO(STATE_UlyssesStandAlert, 0, 0, 0, 0);
        return 1;
    case MSG_EXIT:
        sm.GetActorOwner().EnableMovement(1);
        sm.GetActorOwner().SetIsTargetable(1);
        return 1;
    case MSG_ENTER:
        sm.GetActorOwner().EnableMovement(0);
        sm.GetActorOwner().SetAnimType(0x9000409, 0, 1);
        stage = 0;
        AU_AssignSkin(sm.GetActorOwner().GetAnim(), 0x1000146, 0xd000068);
        return 1;
    case MSG_EXT:
        switch (stage) {
        case 0:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
                sm.GetActorOwner().EnableMovement(1);
                stage = 1;
                count = 1;
            }
            break;
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) {
                if (!count) {
                    sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                    SM_GOTO(STATE_UlyssesStandAlert, 0, 0, 0, 0);
                    return 1;
                }
                count--;
            }
            break;
        }
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

signed char STATE_UlyssesPoweredDown(StateMachine &sm)
{
    sm.GetActorOwner().SetWatch(4);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        sm.GetActorOwner().SetAnimType(0x900050a, 0, 1);
        sm.GetActorOwner().EnableMovement(0);
        sm.GetActorOwner().SetIsTargetable(0);
        GeoSectionManager::Add(0x1000146, 0xd000068);
        return 1;
    case MSG_RETURN:
        sm.GetActorOwner().SetAnimType(0x900050a, 0, 1);
        sm.GetActorOwner().EnableMovement(0);
        return 1;
    case MSG_EXT:
        sm.GetActorOwner().ResetHitCountdown();
        return 1;
    case MSG_EXIT:
        sm.GetActorOwner().EnableMovement(1);
        sm.GetActorOwner().SetIsTargetable(1);
        return 1;
    }
    return 0;
}

// NEAR: written for behaviour; look-target vector copies and facing products scheduled differently
signed char STATE_HitByUnknownMine(StateMachine &sm)
{
    int i = 0;
    EXVector &lookPos = sm.GetVectorRef(i);
    EXVector &mineDir = sm.GetVectorRef(i);
    int &stage = sm.GetIntRef(i);
    int &sawPlayer = sm.GetIntRef(i);
    int &done = sm.GetIntRef(i);
    sm.GetActorOwner().SetWatch(4);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXIT:
        return 1;
    case MSG_ENTER: {
        EXMaths::vec_xyzw d = VSub(Player_GetFeetPos(), sm.GetActorOwner().GetFeetPos());
        EXVector v(d.x, 0.0f, d.z, d.w);
        NormaliseInline(v);
        float r = EXRandf() * 20.0f - 10.0f;
        EXVector side(r * -v.z, r * v.y, r * v.x, r * v.w);
        const EXVector &feet = sm.GetActorOwner().GetFeetPos();
        EXVector ahead(feet.x + v.x * 10.0f, feet.y + v.y * 10.0f, feet.z + v.z * 10.0f, feet.w + v.w * 10.0f);
        lookPos = EXVector(ahead.x + side.x, ahead.y + side.y, ahead.z + side.z, ahead.w + side.w);
        stage = 0;
        sawPlayer = 0;
        mineDir = *(EXVector *)sm.m_Msg.m_Data[0];
        return 1;
    }
    case MSG_EXT:
        if (done) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        if (!sawPlayer && ActorOptionsManager::PlayerVisible() &&
            (!Player_IsCloaked() || SubGroupAnims::GetSubGroupAnims(sm.GetActorOwner().m_SubGroupAnimIndex).m_Anims[8])) {
            EXVector chest = XItemHandler_Player::Instance()->GetChestPos();
            chest.y = EXRandf() + chest.y - 0.5f;
            if (sm.GetActorOwner().CanSeePoint(chest, 0.0f)) sawPlayer = 1;
        }
        switch (stage) {
        case 0: {
            sm.GetActorOwner().SetDontDoHitReaction(1);
            stage = 1;
            float a = sm.GetActorOwner().GetRotation().y + 0.7853982f;
            float s = sin(a);
            float c = cos(a);
            float front = mineDir.x * s + mineDir.z * c + mineDir.y * 0.0f;
            float right = mineDir.x * -c + mineDir.z * s + mineDir.y * 0.0f;
            if (0.0f > front) {
                if (right > 0.0f)
                    sm.GetActorOwner().SetAnimType(0x90001bc, 0, 1);
                else
                    sm.GetActorOwner().SetAnimType(0x90001ba, 0, 1);
                return 1;
            }
            if (0.0f > right)
                sm.GetActorOwner().SetAnimType(0x90001bb, 0, 1);
            else
                sm.GetActorOwner().SetAnimType(0x90001bd, 0, 1);
            return 1;
        }
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 2;
                sm.GetActorOwner().SetDontDoHitReaction(0);
                return 1;
            }
            return 1;
        case 2:
            if (sm.GetActorOwner().IsFacingYRot(lookPos, 15.0f)) {
                if (g_ulLookAtAnim == 1) {
                    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
                    if (pWeapon && pWeapon->m_AimType != 0x52000005 && pWeapon->m_AimType != 0x52000006) g_ulLookAtAnim = 2;
                }
                switch (g_ulLookAtAnim) {
                case 0:
                    sm.GetActorOwner().SetAnimType(0x9000308, 0, 1);
                    break;
                case 1:
                    sm.GetActorOwner().SetAnimType(0x9000305, 0, 1);
                    break;
                case 2:
                    sm.GetActorOwner().SetAnimType(0x9000108, 0, 1);
                    break;
                }
                if (++g_ulLookAtAnim > 2) g_ulLookAtAnim = 0;
                stage = 3;
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(lookPos, 15.0f);
            return 1;
        case 3:
            if (sm.GetActorOwner().HasAnimEnded()) {
                if (!sawPlayer) {
                    sm.GetActorOwner().CancelAlarmMeter();
                    sm.Return(0, 0, 0, 0);
                    return 1;
                }
                done = 1;
                sm.GetActorOwner().PlayerForceSighting();
                sm.Return(0, 0, 0, 0);
                return 1;
            }
            return 1;
        }
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

// NEAR: written for behaviour; retail inlines the feet-pos delta as a 4-wide subtract plus copy
signed char STATE_CaptivePredHanging(StateMachine &sm)
{
    int i = 0;
    EXVector &pos = sm.GetVectorRef(i);
    sm.GetActorOwner().SetWatch(4);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN: {
        sm.GetActorOwner().SetAnimType(0x900048b, 0, 1);
        sm.GetActorOwner().EnableMovement(0);
        EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), pos);
        if (d.x * d.x + d.y * d.y + d.z * d.z > 0.0001f) {
            sm.GetActorOwner().SetPositionEx(pos);
            return 1;
        }
        return 1;
    }
    case MSG_EXIT:
        sm.GetActorOwner().EnableMovement(1);
        sm.GetActorOwner().SetIsTargetable(1);
        sm.GetActorOwner().SetIsHanging(0);
        return 1;
    case MSG_ENTER:
        sm.GetActorOwner().SetAnimType(0x900048b, 0, 1);
        sm.GetActorOwner().EnableMovement(0);
        sm.GetActorOwner().SetIsTargetable(0);
        sm.GetActorOwner().SetIsHanging(1);
        pos = sm.GetActorOwner().GetFeetPos();
        return 1;
    case MSG_EXT: {
        if (sm.GetActorOwner().HasAnimEnded()) {
            int r = EXRand(4);
            if (r)
                sm.GetActorOwner().SetAnimType(0x900048b, 0, 1);
            else
                sm.GetActorOwner().SetAnimType(0x900048a, 0, 1);
            return 1;
        }
        EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), pos);
        if (d.x * d.x + d.y * d.y + d.z * d.z > 0.0001f) {
            sm.GetActorOwner().SetPositionEx(pos);
            return 1;
        }
        return 1;
    }
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

signed char STATE_StandIdle_Invincible(StateMachine &sm)
{
    sm.GetActorOwner().SetWatch(4);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
    case MSG_RETURN:
        sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        sm.GetActorOwner().EnableMovement(0);
        return 1;
    case MSG_EXT:
        sm.GetActorOwner().ResetHitCountdown();
        return 1;
    case MSG_EXIT:
        sm.GetActorOwner().EnableMovement(1);
        return 1;
    }
    return 0;
}

signed char STATE_BlendAnimTest(StateMachine &sm)
{
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER: {
        int i = 0;
        sm.GetFloatRef(i) = 0.0f;
        sm.GetFloatRef(i) = 0.0f;
        return 1;
    }
    case MSG_EXT: {
        Actor &actor = sm.GetActorOwner();
        actor.SetAimingPos(actor.GetAimPosForTarget(1));
        return 1;
    }
    }
    return 0;
}

signed char STATE_TurningTest(StateMachine &sm)
{
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(1);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
        if (sm.GetActorOwner().IsFacingYRot(gAIControl.LastSeenPlayerPos(), 5.0f)) {
            sm.GetActorOwner().SetAnimType(0x4e000004, 0, 1);
            return 1;
        }
        sm.GetActorOwner().SetTargetYRot(gAIControl.LastSeenPlayerPos(), 5.0f);
        return 1;
    }
    return 0;
}

signed char STATE_DropMine(StateMachine &sm)
{
    int i = 0;
    int &dropped = sm.GetIntRef(i);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (sm.GetActorOwner().HasAnimEnded()) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_EXIT:
        if (!dropped) CreateActorDroppedMine(sm.GetActorOwner());
        return 1;
    case MSG_ENTER:
        sm.GetActorOwner().InvalidateGotoPos();
        if (sm.GetActorOwner().IsBoss()) {
            sm.GetActorOwner().SetAnimType(0x9000427, 0, 1);
            dropped = 0;
            return 1;
        }
        sm.GetActorOwner().SetAnimType(0x90000db, 0, 1);
        dropped = 0;
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case 9:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)9) && !dropped) {
            dropped = 1;
            CreateActorDroppedMine(sm.GetActorOwner());
        }
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

signed char STATE_TestDiscWeapon(StateMachine &sm)
{
    int i = 0;
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXIT:
        return 1;
    case MSG_ENTER:
        timer = EXBaseApp::NominalGameFrameRate() * 10.0f;
        return 1;
    case MSG_EXT:
        timer -= XApp::GameFrameRateMul();
        if (timer <= 0.0f) {
            if (sm.GetActorOwner().IsBoss())
                sm.GetActorOwner().SetAnimType(0x9000354, 0, 1);
            else
                sm.GetActorOwner().SetAnimType(0x90001b5, 0, 1);
            timer = EXBaseApp::NominalGameFrameRate() * 10.0f;
        }
        if (sm.GetActorOwner().HasAnimEnded()) {
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            return 1;
        }
        return 1;
    case 0x19:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x19)) {
            EXVector hand = sm.GetActorOwner().GetHandPos(0);
            EXVector dir(sin(sm.GetActorOwner().GetRotation().y), 0.0f, cos(sm.GetActorOwner().GetRotation().y), 0.0f);
            EXVector zero(0.0f, 0.0f, 0.0f, 0.0f);
            XItemHandler_Bullet::CreateFromHash(hand, zero, dir, 0x49000041, 0, sm.GetActorOwner().m_RefID, 1, 1.0f);
            sm.GetActorOwner().SetHideCarryRight(1);
        }
        return 1;
    }
    return 0;
}

// NEAR: written for behaviour; vector copy/schedule residue in the facing test
signed char STATE_ScaredOfPredRoar(StateMachine &sm)
{
    int i = 0;
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        timer -= XApp::GameFrameRateMul();
        if (timer < 0.0f) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_ENTER: {
        int r = EXRand(4);
        if (!r) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        if (sm.GetActorOwner().DontDoHitReaction()) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        if (sm.GetActorOwner().IsDazed()) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        if (sm.GetActorOwner().IsLyingOnGround()) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        if (sm.GetActorOwner().IsLyingBum()) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        if (sm.GetActorOwner().IsBoss() || sm.GetActorOwner().IsUlysses() || sm.GetActorOwner().IsAlien()) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        EXMaths::vec_xyzw d = VSub(Player_GetFeetPos(), sm.GetActorOwner().GetFeetPos());
        EXVector fwd(sin(sm.GetActorOwner().GetRotation().y), 0.0f, cos(sm.GetActorOwner().GetRotation().y), 0.0f);
        if (d.LengthXYZSquared() > 100.0f || 0.0f > fwd.DotXYZ(d)) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        sm.GetActorOwner().SetAnimType(0x900012c, 0, 1);
        timer = EXRandf(1.0f, 2.0f) * EXBaseApp::NominalGameFrameRate() * 1.5f;
        sm.GetActorOwner().SetAnimFrame(EXRandf());
        return 1;
    }
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

signed char STATE_WanderTest(StateMachine &sm)
{
    int i = 0;
    EXVector &start = sm.GetVectorRef(i);
    EXVector &target = sm.GetVectorRef(i);
    int &startPoly = sm.GetIntRef(i);
    int &targetPoly = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        target = start = sm.GetActorOwner().GetFeetPos();
        targetPoly = startPoly = sm.GetActorOwner().GetNavPolyHandle();
        return 1;
    case MSG_EXT:
        timer -= XApp::GameFrameRateMul();
        if (timer <= 0.0f) {
            XNavLocation loc;
            loc.SetPosPoly(start, startPoly, 0);
            float x = EXRandf() * 30.0f - 15.0f;
            EXVector off(x, 0.0f, EXRandf() * 30.0f - 15.0f, 0.0f);
            loc.m_dwIgnore = 0x848;
            loc.Move(off, 0);
            target = loc.m_vFloorPos;
            targetPoly = loc.GetPolyHandle();
            timer = EXBaseApp::NominalGameFrameRate() * 10.0f;
        }
        sm.GetActorOwner().GotoPosPoly(target, targetPoly);
        sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
        return 1;
    }
    return 0;
}

signed char STATE_HerIndoors(StateMachine &sm)
{
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        sm.GetActorOwner().SetDontDoHitReaction(1);
        sm.GetActorOwner().EnableMovement(0);
        sm.GetActorOwner().SetAnimType(0x9000507, 0, 1);
        return 1;
    }
    return 0;
}

signed char STATE_MotherInTank(StateMachine &sm)
{
    sm.GetActorOwner().SetWatch(4);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        sm.GetActorOwner().SetDontDoHitReaction(1);
        sm.GetActorOwner().SetIsTargetable(0);
        sm.GetActorOwner().EnableMovement(0);
        sm.GetActorOwner().SetAnimType(0x9000541, 0, 1);
        return 1;
    }
    return 0;
}

signed char STATE_TestBackoff(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT: {
        Actor &actor = sm.GetActorOwner();
        if (actor.SqDistFromPlayer() > 100.0f) {
            actor.SetAnimType(0x4e000001, 0, 1);
            return 1;
        }
        switch (stage) {
        case 0:
            actor.SetAnimType(0x4e000001, 0, 1);
            timer = EXBaseApp::NominalGameFrameRate() * 3.0f;
            stage++;
            return 1;
        case 1:
            timer -= XApp::GameFrameRateMul();
            if (timer <= 0.0f) {
                stage++;
                return 1;
            }
            sm.GetActorOwner().SpinToYRot(Player_GetFeetPos());
            return 1;
        case 2:
            actor.SetAnimType(0x4e000009, 0, 1);
            stage++;
            return 1;
        case 3:
            if (actor.HasAnimEnded()) {
                stage = 0;
                return 1;
            }
            sm.GetActorOwner().SpinToYRot(Player_GetFeetPos());
            return 1;
        }
        return 1;
    }
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

signed char STATE_TestCrash(StateMachine &sm)
{
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER: {
        int anim = sm.GetActorOwner().GetAnimMode();
        sm.GetActorOwner().SetAnimType(0x90000db, 0, 1);
        sm.GetActorOwner().SetAnimType(anim, 0, 1);
        sm.GetActorOwner().SetAnimFrame(1.0f);
        return 1;
    }
    case MSG_EXT:
        sm.Return(0, 0, 0, 0);
        return 1;
    }
    return 0;
}

signed char STATE_TestLooping(StateMachine &sm)
{
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        sm.GetActorOwner().SetAnimType(0x9000016, 0, 1);
        return 1;
    case MSG_EXT:
        sm.GetActorOwner().HasAnimEnded();
        return 1;
    }
    return 0;
}

// NEAR: retail keeps separate flag-test/return blocks on the null-weapon path; VC merges it into case 0x52000000
int GetNormalStrikeAnim(StateMachine &sm, WeaponProfile *pWeapon)
{
    static int s_aStrike54[8] = { 0x09000036, 0x09000037, 0x09000038, 0x09000039,
                                  0x0900003a, 0x0900003b, 0x0900003c, 0x0900003d };
    static int s_aStrike55[8] = { 0x090001ec, 0x090001eb, 0x090001ea, 0x090001e9,
                                  0x090001e8, 0x090001e7, 0x090001e6, 0x090001e4 };
    static int s_aStrike53[8] = { 0x0900003f, 0x09000040, 0x09000041, 0x09000042,
                                  0x09000043, 0x09000044, 0x09000045, 0x09000046 };
    if (!pWeapon) {
        int arm;
        if (sm.GetActorOwner().m_ActorFlags.m_bRightArmMissing)
            arm = 1;
        else if (sm.GetActorOwner().m_ActorFlags.m_bLeftArmMissing)
            arm = 0;
        else
            arm = EXRand32() & 1;
        if (arm) return s_aStrikeOneArmA[EXRand32() & 3];
        return s_aStrikeOneArmB[EXRand32() & 3];
    }
    switch (pWeapon->m_AimType) {
    case 0x52000000: {
        int arm;
        if (sm.GetActorOwner().m_ActorFlags.m_bRightArmMissing)
            arm = 1;
        else if (sm.GetActorOwner().m_ActorFlags.m_bLeftArmMissing)
            arm = 0;
        else
            arm = EXRand32() & 1;
        if (arm) return s_aStrikeOneArmA[EXRand32() & 3];
        return s_aStrikeOneArmB[EXRand32() & 3];
    }
    case 0x52000001:
        return s_aStrike53[EXRand32() & 7];
    case 0x52000002:
        return s_aStrike54[EXRand32() & 7];
    case 0x52000003:
        return s_aStrike55[EXRand32() & 7];
    case 0x52000004:
        if (sm.GetActorOwner().GetSkelHash() == 0xd000091)
            return (sm.GetActorOwner().GetProfile()->m_SubGroup != 0x4f000021) + 0x900025a;
        return 0x90002d1;
    case 0x52000005:
        return sm.GetActorOwner().GetSkelHash() == 0xd000091 ? 0x900025b : 0x90002a1;
    case 0x52000006:
        return sm.GetActorOwner().GetSkelHash() == 0xd000091 ? 0x900025b : 0x90002cc;
    }
    return 0;
}

// NEAR: 30 bytes, the 0x4f000013 strike chain shares the 0x4f00000e return tails the other way round
int GetStrikeAnim(StateMachine &sm, WeaponProfile *pWeapon, int strike, int *pFlags)
{
    static int s_aBoss0c[4] = { 0x0900025a, 0x0900025b, 0x0900025c, 0x0900025d };
    static int s_aBoss10[4] = { 0x09000261, 0x09000260, 0x0900025e, 0x0900025f };
    static int s_aBoss17[3] = { 0x09000437, 0x09000436, 0x09000435 };
    static int s_aBossDefault[2] = { 0x0900021b, 0x0900021c };
    *pFlags = 3;
    if (sm.GetActorOwner().IsBoss()) {
        switch (sm.GetActorOwner().GetProfile()->m_SubGroup) {
        case 0x4f00000c:
            return s_aBoss0c[EXRand32() & 3];
        case 0x4f000010:
            return s_aBoss10[EXRand32() & 3];
        case 0x4f000017:
            return s_aBoss17[EXRand32() % 3];
        case 0x4f00000e:
            switch (strike) {
            case 2:
                *pFlags |= 4;
                return 0x90002e9;
            case 3:
                *pFlags |= 8;
                return 0x90002b8;
            }
            return 0x90002e8;
        case 0x4f000013:
            switch (strike) {
            case 1:
                return 0x90002e8;
            case 2:
                *pFlags |= 4;
                return 0x90002e9;
            case 3:
                *pFlags |= 8;
                return 0x90002b8;
            }
            return 0x90002e7;
        case 0x4f000011:
            switch (strike) {
            case 1:
                return 0x900051f;
            case 2:
                return 0x900051e;
            case 3:
                *pFlags |= 8;
                return 0x9000520;
            }
            return 0x900051e;
        case 0x4f000021:
            *pFlags |= 0x10;
            return XItemHandler_Player::Instance()->IsHanging() ? 0x900038a : 0x90003fc;
        }
        return s_aBossDefault[EXRand32() & 1];
    }
    if (strike == pWeapon->m_NumStrikes - 1 && pWeapon->m_NumSpecialAnims) {
        *pFlags = 0x13;
        return pWeapon->GetSpecialAnimHash();
    }
    return GetNormalStrikeAnim(sm, pWeapon);
}

int Actor_GetDodgeAnim(StateMachine &sm)
{
    if (sm.GetActorOwner().IsDiveAllowed()) return s_aDiveAnims[EXRand32() & 7];
    return s_aDodgeAnims[EXRand32() & 1];
}

// NEAR: written for behaviour; vector-copy and sqrt-compare scheduling residue
int Actor_ChooseTarget(Actor &actor, signed char bArmed, float alertTime, int current)
{
    float fd = actor.GetFightDistance();
    float bestDistSq = fd * fd;
    int best = 0;
    signed char canTargetPlayer = 1;
    if (actor.GetProfile()->m_Group == 0x4b00000b) canTargetPlayer = 0;
    if (actor.GetProfile()->m_SubGroup == 0x4f000011 &&
        (gCurrentMap_GeoHash == 0x1000124 || gCurrentMap_GeoHash == 0x100013b)) {
    } else if (canTargetPlayer && (actor.WasPlayerSeen() || (bArmed && alertTime > 0.0f))) {
        EXMaths::vec_xyzw d = VSub(gAIControl.LastSeenPlayerPos(), actor.GetFeetPos());
        float distSq = d.LengthXYZSquared();
        if (bestDistSq > distSq) {
            if (bArmed) {
                if (16.0f > distSq) return 1;
            } else {
                if (4.0f > distSq) return 1;
            }
            bestDistSq = distSq;
            best = 1;
        }
    }
    EXVector pos = actor.GetFeetPos();
    for (Actor *p = Actor::GetFirst(pos, fd, 0); p; p = p->GetNext(pos, fd, 0)) {
        if (p->m_RefID == current) continue;
        if (0.0f >= p->GetHealth()) continue;
        if ((p->GetSurfaceFlags() >> 10) & 1) continue;
        if (p->m_SM.GetState() == STATE_PlayerControlDeath) continue;
        if (p->m_SM.GetState() == STATE_CaptivePredHanging) continue;
        if (p->m_StateFlags.m_bBeingStealthKilled) continue;
        if (p->IsHanging()) continue;
        if (!AP_DoWeFight(actor.GetProfile(), p->GetProfile())) continue;
        if (!gAIControl.CheckZoneSight(actor.GetNavZone(), p->GetNavZone())) continue;
        EXMaths::vec_xyzw d = VSub(pos, p->GetFeetPos());
        float distSq = d.LengthXYZSquared();
        if (bestDistSq > distSq) {
            if (bArmed && best == 1) {
                if (ForceSqrt(distSq) < ForceSqrt(bestDistSq) - 2.0f) {
                    best = p->m_RefID;
                    bestDistSq = distSq;
                }
            } else {
                best = p->m_RefID;
                bestDistSq = distSq;
            }
        }
    }
    if (!best) best = current;
    return best;
}

signed char Actor_CanSeeTarget(Actor &actor, int target, float &timer, float time)
{
    signed char seen;
    switch (target) {
    case 0:
        return 0;
    case 1:
        seen = actor.WasPlayerSeen();
        break;
    default:
        if (EXRand32() % 31 == 0) {
            EXVector pos = Actor_GetTargetFeetPos(target);
            pos.y += 1.0f;
            seen = actor.CanSeePoint(pos, 100.0f);
        } else {
            seen = timer > 0.0f;
        }
        break;
    }
    if (seen) timer = time;
    timer -= EXBaseApp::GameFrameRateMul();
    if (target == 1) {
        if (seen && !Player_IsCloaked()) gAIControl.SetLastSeenPlayerPos(Player_GetFeetPos());
        if (timer >= 0.0f) seen = 1;
    }
    return seen;
}

// NEAR: written for behaviour; retail routes the deltas through extra vec_xyzw copies and reuses
// parameter slots for the distance/angle temporaries
void ThrowGrenade(StateMachine &sm, EXVector target, int weapon, signed char left)
{
    WeaponProfile *pWeapon;
    if (weapon)
        pWeapon = WeaponProfile::GetWeaponData(weapon);
    else
        pWeapon = WeaponProfile::GetWeaponDataFromEntity(sm.GetActorOwner().GetProfile()->m_Weapon);
    if (!pWeapon || pWeapon->m_AimType != 0x52000007) {
        pWeapon = WeaponProfile::GetWeaponDataFromEntity(sm.GetActorOwner().GetProfile()->m_Weapon2);
        if (!pWeapon || pWeapon->m_AimType != 0x52000007) {
            EXHashCodeFile::Debug_HashCodeName(sm.GetActorOwner().GetProfile()->m_Hash);
            return;
        }
    }
    if (!sm.GetActorOwner().HasThrownPetrolBomb() && sm.GetActorOwner().GetProfile()->m_Weapon2 == 0x20000dc &&
        sm.GetActorOwner().m_StateFlags.m_bOnScreen)
        sm.GetActorOwner().SetHasThrownPetrolBomb(1);
    if (pWeapon->m_Id == 0x49000025) {
        EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), target);
        float dist = ForceSqrt(d.LengthXYZSquared());
        float scale = 1.0f;
        if (20.0f > dist) scale = dist * 0.05f;
        EXVector r;
        r.x = EXRandf() * 2.0f - 1.0f;
        r.y = 0.0f;
        r.z = EXRandf() * 2.0f - 1.0f;
        r.w = 0.0f;
        NormaliseInline(r);
        scale = (EXRandf() * 2.0f + 3.0f) * scale;
        target.x = target.x + scale * r.x;
        target.y = target.y + scale * r.y;
        target.z = target.z + scale * r.z;
        target.w = target.w + scale * r.w;
    }
    EXVector hand = sm.GetActorOwner().GetHandPos(left);
    EXVector zero(0.0f, 0.0f, 0.0f, 0.0f);
    EXVector d = VSub(target, hand);
    float rot = GetRotation(d);
    float speed = pWeapon->m_ThrowSpeed;
    float horiz = ForceSqrt(d.z * d.z + d.x * d.x);
    float angle = CalculateAngle(speed, horiz, d.y, 0.01f, 1);
    if (angle > 90.0f) angle = 0.7853982f;
    float spread = (100.0f - pWeapon->m_Accuracy) * 0.0015f;
    float pitch = EXRandf(-spread, spread) + angle;
    EXVector dir;
    dir.x = sin(rot) * cos(pitch);
    dir.z = cos(rot) * cos(pitch);
    dir.y = sin(pitch);
    dir.w = 0.0f;
    XItemHandler_Bullet::CreateFromHash(hand, zero, dir, pWeapon->m_Id, 0, sm.GetActorOwner().m_RefID, 0, 1.0f);
}

int GetServBlockAnimation(Actor &actor, EXVector &v)
{
    static int s_aServ2[3] = { 0x090004f6, 0x090004f7, 0x090004f8 };
    static int s_aServ1[3] = { 0x09000543, 0x09000544, 0x09000545 };
    int cur = actor.GetAnimMode();
    int anim;
    if (actor.GetProfile()->m_SubGroup == 0x4f000013) {
        do {
            anim = s_aServ1[EXRand32() % 3];
        } while (anim == cur);
        return anim;
    }
    do {
        anim = s_aServ2[EXRand32() % 3];
    } while (anim == cur);
    return anim;
}

int GetRoninBlockAnimation(Actor &actor, EXVector &v)
{
    static int s_aRonin[3] = { 0x09000362, 0x09000363, 0x09000364 };
    int cur = actor.GetAnimMode();
    int anim;
    do {
        anim = s_aRonin[EXRand32() % 3];
    } while (anim == cur);
    return anim;
}

// NEAR: written for behaviour; stage jump table, delta copies and register residue
signed char STATE_QuickEvadePlayerWithGunButt(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    int &strikeFlags = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(4);
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXIT:
        POI_VendorUnbind(poi, sm.GetActorOwner().m_RefID);
        return 1;
    case MSG_ENTER:
        poi = AU_FindRunAwayPoint(sm, 0x2000, poi, sm.GetActorOwner().GetNavZone(), 1);
        if (!poi) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        POI_VendorBind(poi, sm.GetActorOwner().m_RefID);
        sm.GetActorOwner().GotoPOI(poi);
        stage = 0;
        timer = EXBaseApp::NominalGameFrameRate() * 3.0f;
        if (sm.GetActorOwner().IsBoss()) return 1;
        EXHashCodeFile::Debug_HashCodeName(sm.GetActorOwner().GetProfileHash());
        sm.Return(0, 0, 0, 0);
        return 1;
    case MSG_EXT:
        sm.GetActorOwner().SetInvinsibleCounter(10);
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        if (timer > 0.0f) timer -= XApp::GameFrameRateMul();
        switch (stage) {
        case 0: {
            sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
            sm.GetActorOwner().GotoPOI(poi);
            if (0.0100000007f > sm.GetActorOwner().SqDistToTarget()) {
                sm.GetActorOwner().InvalidateGotoPos();
                sm.Return(0, 0, 0, 0);
                return 1;
            }
            if (0.0f >= timer) {
                EXMaths::vec_xyzw d = VSub(gAIControl.LastSeenPlayerPos(), sm.GetActorOwner().GetFeetPos());
                if (16.0f > d.LengthXYZSquared() && sm.GetActorOwner().CanGetToLocation(gAIControl.PlayerNavLocLastSeen()) &&
                    1.0f > gAIControl.DistOffNaviMeshLastSeen())
                    stage = 2;
            }
            return 1;
        }
        case 1:
            return 1;
        case 2: {
            sm.GetActorOwner().GotoLoc(gAIControl.PlayerNavLocLastSeen());
            sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
            if (!sm.GetActorOwner().IsDestInSameNavZone()) {
                stage = 0;
                return 1;
            }
            float sq = sm.GetActorOwner().SqDistToTarget();
            float reach = pWeapon->m_MeleeRange * 1.5f;
            if (reach * reach > sq) {
                sm.GetActorOwner().InvalidateGotoPos();
                stage = 3;
            }
            if (sq > 36.0f || gAIControl.DistOffNaviMeshLastSeen() > 1.1f) stage = 0;
            return 1;
        }
        case 3: {
            strikeFlags = 7;
            float r = EXRandf(3.0f, 5.0f);
            timer = r * EXBaseApp::NominalGameFrameRate();
            Actor &a = sm.GetActorOwner();
            a.SetAnimType(GetStrikeAnim(sm, pWeapon, 0, &strikeFlags), 0, 1);
            stage = 5;
            return 1;
        }
        case 4:
            return 1;
        case 5:
            if (sm.GetActorOwner().HasAnimEnded()) stage = 0;
            return 1;
        }
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x20:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x20)) StrikeCharacter(sm, 0, strikeFlags, pWeapon, 1, 1);
        return 1;
    case 0x1f:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1f)) StrikeCharacter(sm, 1, strikeFlags, pWeapon, 1, 1);
        return 1;
    case 0x1e:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e)) {
            StrikeCharacter(sm, 1, strikeFlags, pWeapon, 1, 1);
            StrikeCharacter(sm, 0, strikeFlags, pWeapon, 1, 1);
        }
        return 1;
    }
    return 0;
}


signed char STATE_HunterBlock(StateMachine &sm)
{
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_EXT:
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        if (sm.GetActorOwner().HasAnimEnded()) {
            sm.GetActorOwner().SetDontDoHitReaction(0);
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_ENTER: {
        sm.GetActorOwner().SetDontDoHitReaction(1);
        int anims[2] = { 0x9000232, 0x9000233 };
        int anim = anims[EXRand32() & 1];
        sm.GetActorOwner().SetAnimType(anim, 0, 1);
        return 1;
    }
    }
    return 0;
}

signed char STATE_RussianBlock(StateMachine &sm)
{
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_EXT:
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        if (sm.GetActorOwner().HasAnimEnded()) {
            sm.GetActorOwner().SetDontDoHitReaction(0);
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        sm.GetActorOwner().SetBlockCounter(40);
        return 1;
    case MSG_ENTER: {
        sm.GetActorOwner().SetDontDoHitReaction(1);
        StructMsg_IncomingAttack *pMsg = (StructMsg_IncomingAttack *)sm.m_Msg.m_Data[0];
        Actor &actor = sm.GetActorOwner();
        actor.SetAnimType(GetRoninBlockAnimation(actor, pMsg->m_Dir), 0, 1);
        return 1;
    }
    }
    return 0;
}

// NEAR: written for behaviour; compare-chain layout (EXIT/default) and register residue
signed char STATE_BlockServ(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &strikeFlags = sm.GetIntRef(i);
    float &blockTimer = sm.GetFloatRef(i);
    float &elapsed = sm.GetFloatRef(i);
    float &animTimer = sm.GetFloatRef(i);
    EXVector &dir = sm.GetVectorRef(i);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    sm.GetActorOwner().SetHeadTrackMode(0);
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        blockTimer = EXBaseApp::NominalGameFrameRate();
        stage = 0;
        elapsed = 0.0f;
        dir = ((StructMsg_IncomingAttack *)sm.m_Msg.m_Data[0])->m_Dir;
        sm.GetActorOwner().SetAnimType(0x90004f5, 0, 1);
        return 1;
    case MSG_EXT:
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        blockTimer -= EXBaseApp::GameFrameRateMul();
        elapsed += EXBaseApp::GameFrameRateMul();
        if (elapsed > EXBaseApp::NominalGameFrameRate() * 2.5f) {
            strikeFlags = 7;
            sm.GetActorOwner().SetAnimType(0x900025f, 0, 1);
            stage = 2;
        }
        switch (stage) {
        case 0:
            stage = 1;
            animTimer = EXBaseApp::NominalGameFrameRate() * 0.25f;
            return 1;
        case 1:
            if (sm.GetActorOwner().HasAnimEnded() && sm.GetActorOwner().GetAnimMode() == 0x90004f5) {
                Actor &a = sm.GetActorOwner();
                a.SetAnimType(GetServBlockAnimation(a, dir), 0, 1);
            }
            if (animTimer > 0.0f) animTimer -= XApp::GameFrameRateMul();
            sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
            sm.GetActorOwner().SetBlockCounter(40);
            if (0.0f >= blockTimer) {
                sm.Return(0, 0, 0, 0);
                return 1;
            }
            return 1;
        case 2:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.Return(0, 0, 0, 0);
                return 1;
            }
            return 1;
        }
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x1f:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1f)) StrikeCharacter(sm, 1, strikeFlags, pWeapon, 0, 1);
        return 1;
    case 0x20:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x20)) StrikeCharacter(sm, 0, strikeFlags, pWeapon, 0, 1);
        return 1;
    case 0x1e:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e)) {
            StrikeCharacter(sm, 1, strikeFlags, pWeapon, 0, 1);
            StrikeCharacter(sm, 0, strikeFlags, pWeapon, 0, 1);
        }
        return 1;
    case MSG_INCOMING_ATTACK:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_INCOMING_ATTACK)) {
            StructMsg_IncomingAttack *pMsg = (StructMsg_IncomingAttack *)sm.m_Msg.m_Data[0];
            if (pMsg->m_Unk4 != -1) {
                sm.GetActorOwner().SetBlockCounter(0);
                sm.Return(0, 0, 0, 0);
                return 1;
            }
            if (stage != 2) {
                blockTimer = EXBaseApp::NominalGameFrameRate();
                if (animTimer <= 0.0f) {
                    Actor &a = sm.GetActorOwner();
                    a.SetAnimType(GetServBlockAnimation(a, pMsg->m_Dir), 0, 1);
                    animTimer = EXBaseApp::NominalGameFrameRate() * 0.25f;
                }
            }
        }
        return 1;
    case MSG_EXIT:
        return 1;
    }
    return 0;
}

// NEAR: 7 bytes, register choice (eax/ecx/edx) in the incoming-attack case
signed char STATE_Block(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    float &blockTimer = sm.GetFloatRef(i);
    float &seenTimer = sm.GetFloatRef(i);
    float &elapsed = sm.GetFloatRef(i);
    float &animTimer = sm.GetFloatRef(i);
    EXVector &dir = sm.GetVectorRef(i);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT: {
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        blockTimer -= EXBaseApp::GameFrameRateMul();
        elapsed += EXBaseApp::GameFrameRateMul();
        if (elapsed > EXBaseApp::NominalGameFrameRate() * 2.5f) {
            sm.GetActorOwner().SetBlockCounter(EXBaseApp::NominalGameFrameRate());
            sm.Return((int)STATE_Block, 1, 0, 0);
            return 1;
        }
        signed char seen = Actor_CanSeeTarget(sm.GetActorOwner(), 1, seenTimer, sm.GetActorOwner().GetCombatTimeout());
        switch (stage) {
        case 0: {
            int sub = sm.GetActorOwner().GetProfile()->m_SubGroup;
            if (sub == 0x4f00000e || sub == 0x4f000011 || sub == 0x4f000021) {
                Actor &a = sm.GetActorOwner();
                a.SetAnimType(GetRoninBlockAnimation(a, dir), 0, 1);
            } else {
                sm.GetActorOwner().SetAnimType(s_aBlockAnims[EXRand(1)], 0, 1);
            }
            stage = 1;
            animTimer = EXBaseApp::NominalGameFrameRate() * 0.25f;
            return 1;
        }
        case 1:
            if (animTimer > 0.0f) animTimer -= XApp::GameFrameRateMul();
            sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
            {
                int sub = sm.GetActorOwner().GetProfile()->m_SubGroup;
                if (sub == 0x4f00000e || sub == 0x4f000011 || sub == 0x4f000021)
                    sm.GetActorOwner().SetBlockCounter(40);
                else
                    sm.GetActorOwner().SetBlockCounter(3);
            }
            if (0.0f >= blockTimer || !seen) {
                sm.Return(0, 0, 0, 0);
                return 1;
            }
            return 1;
        }
        return 1;
    }
    case MSG_EXIT:
        return 1;
    case MSG_ENTER:
        blockTimer = EXBaseApp::NominalGameFrameRate();
        stage = 0;
        elapsed = 0.0f;
        dir = ((StructMsg_IncomingAttack *)sm.m_Msg.m_Data[0])->m_Dir;
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_INCOMING_ATTACK:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_INCOMING_ATTACK)) {
            if (stage == 2) stage = 0;
            blockTimer = EXBaseApp::NominalGameFrameRate();
            int sub = sm.GetActorOwner().GetProfile()->m_SubGroup;
            if (animTimer <= 0.0f && (sub == 0x4f000011 || sub == 0x4f000021)) {
                Actor &a = sm.GetActorOwner();
                a.SetAnimType(GetRoninBlockAnimation(a, ((StructMsg_IncomingAttack *)sm.m_Msg.m_Data[0])->m_Dir), 0, 1);
                animTimer = EXBaseApp::NominalGameFrameRate() * 0.25f;
            }
        }
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

// NEAR: written for behaviour; retail keeps extra copies of the direction vectors (as the other knockbacks)
signed char STATE_KnockBackServ(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case 0x19:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x19)) {
            EXVector off(sin(sm.GetActorOwner().GetRotation().y), 0.0f, cos(sm.GetActorOwner().GetRotation().y), 0.0f);
            ThrowGrenade(sm, VAdd(off, sm.GetActorOwner().GetFeetPos()), 0, 1);
        }
        return 1;
    case MSG_EXT:
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        switch (stage) {
        case 0:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().SetAnimType(0x90004c9, 0, 1);
                stage = 1;
                return 1;
            }
            return 1;
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().SetDontDoHitReaction(0);
                sm.Return(0, 0, 0, 0);
                return 1;
            }
            return 1;
        }
        return 1;
    case MSG_ENTER: {
        sm.GetActorOwner().ClearConsecutiveHitCounter();
        sm.GetActorOwner().SetDontDoHitReaction(1);
        EXVector dir = -*(EXVector *)sm.m_Msg.m_Data[0];
        sm.GetActorOwner().SetRotY(qatan2f(dir.x, dir.z));
        sm.GetActorOwner().SetAnimType(0x9000225, 0, 1);
        return 1;
    }
    }
    return 0;
}

// Draw-weapon anim by aim class (bosses pick one of two). Cases 0x52000000/0x52000007 only widen the jump table.
#define PULL_OUT_WEAPON_ANIM() \
    if (sm.GetActorOwner().IsBoss()) { \
        int anims[2] = { 0x900037c, 0x9000379 }; \
        int anim = anims[EXRand32() & 1]; \
        sm.GetActorOwner().SetAnimType(anim, 0, 1); \
        return 1; \
    } \
    switch (sm.GetActorOwner().GetWeaponProfile(0)->m_AimType) { \
    case 0x52000005: \
        sm.GetActorOwner().SetAnimType(0x900035a, 0, 1); \
        return 1; \
    case 0x52000006: \
        sm.GetActorOwner().SetAnimType(0x9000357, 0, 1); \
        return 1; \
    case 0x52000000: \
        sm.GetActorOwner().SetAnimType(0x900035d, 0, 1); \
        return 1; \
    case 0x52000007: \
        sm.GetActorOwner().SetAnimType(0x900035d, 0, 1); \
        return 1; \
    } \
    sm.GetActorOwner().SetAnimType(0x900035d, 0, 1); \
    return 1;

#define PULL_OUT_WEAPON_ATTACKED \
    case MSG_ATTACKED_BY_PREDATOR: \
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR)) \
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]); \
        return 1; \
    case MSG_ATTACKED_BY_ACTOR: \
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR)) \
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]); \
        return 1;

// NEAR: 4 bytes, register choice for the m_Data[0] loads in the attacked cases (ecx/edx in retail)
signed char STATE_PullOutWeapon(StateMachine &sm)
{
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (sm.GetActorOwner().HasAnimEnded()) {
            sm.GetActorOwner().SwapHands();
            SM_GOTO(STATE_AttackPred, 0, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_ENTER:
        sm.GetActorOwner().SetCarry(0, 0);
        PULL_OUT_WEAPON_ANIM()
    PULL_OUT_WEAPON_ATTACKED
    }
    return 0;
}

// NEAR: 4 bytes, register choice for the m_Data[0] loads in the attacked cases (ecx/edx in retail)
signed char STATE_PullOutWeaponForActor(StateMachine &sm)
{
    sm.GetActorOwner().SetStatus(0xc);
    sm.GetActorOwner().SetWatch(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (sm.GetActorOwner().HasAnimEnded()) {
            sm.GetActorOwner().SwapHands();
            SM_GOTO(STATE_FightActor, 0, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_ENTER:
        sm.GetActorOwner().SetCarry(0, 0);
        PULL_OUT_WEAPON_ANIM()
    PULL_OUT_WEAPON_ATTACKED
    }
    return 0;
}

// NEAR: 4 bytes, register choice for the m_Data[0] loads in the attacked cases (ecx/edx in retail)
signed char STATE_SwapWeaponHands(StateMachine &sm)
{
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        if (sm.GetActorOwner().HasAnimEnded()) {
            sm.GetActorOwner().SwapHands();
            SM_GOTO(STATE_AttackPred, 0, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_ENTER:
        PULL_OUT_WEAPON_ANIM()
    PULL_OUT_WEAPON_ATTACKED
    }
    return 0;
}

// NEAR: 4 bytes, register choice for the m_Data[0] loads in the attacked cases (ecx/edx in retail)
signed char STATE_SwapWeaponHandsActorCombat(StateMachine &sm)
{
    sm.GetActorOwner().SetStatus(0xc);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        if (sm.GetActorOwner().HasAnimEnded()) {
            sm.GetActorOwner().SwapHands();
            SM_GOTO(STATE_FightActor, 0, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_ENTER:
        PULL_OUT_WEAPON_ANIM()
    PULL_OUT_WEAPON_ATTACKED
    }
    return 0;
}

// NEAR: written for behaviour; Hunter2_Goto-style ENTER plus delta-copy residue
signed char STATE_Brute_FleeToLoc18(StateMachine &sm)
{
    int i = 0;
    EXVector &pos = sm.GetVectorRef(i);
    int &poi = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    int &strikeFlags = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        sm.GetActorOwner().SetVariableSpeedAnims(1);
        sm.GetActorOwner().SetAnimSpeedMult(1.25f);
        sm.GetActorOwner().SetDontDoHitReaction(1);
        switch (stage) {
        case 0: {
            EXMaths::vec_xyzw d = VSub(pos, sm.GetActorOwner().GetFeetPos());
            if (0.0100000007f > d.LengthXYZSquared() && !POI_ForceState(poi, 0, sm.GetActorOwner().m_RefID)) {
                sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                return 1;
            }
            EXMaths::vec_xyzw p = VSub(Player_GetFeetPos(), sm.GetActorOwner().GetFeetPos());
            if (4.0f > p.y * p.y + p.z * p.z + p.x * p.x) {
                float rot = sm.GetActorOwner().GetRotation().y;
                if ((float)(cos(rot) * p.z + sin(rot) * p.x) > 0.0f) {
                    strikeFlags = 0xb;
                    sm.GetActorOwner().SetAnimType(0x9000384, 0, 1);
                    stage = 1;
                    return 1;
                }
            }
            sm.GetActorOwner().GotoVendorPOI(poi, -1);
            sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
            return 1;
        }
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 0;
                return 1;
            }
            sm.GetActorOwner().SpinToYRot(Player_GetFeetPos());
            return 1;
        }
        return 1;
    case MSG_EXIT:
        return 1;
    case MSG_ENTER:
        poi = POI_ResolveLocation(0x42000097, sm.GetActorOwner().GetNavZone(),
                                  sm.GetActorOwner().IsBoss() && !sm.GetActorOwner().m_NavRoute.JumpDisabled(),
                                  &sm.GetActorOwner().GetFeetPos());
        POI_VendorBind(poi, sm.GetActorOwner().m_RefID);
        pos = POI_VendorStandPos(poi, POI_VendorStandIndex(poi, sm.GetActorOwner().m_RefID));
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x20:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x20)) StrikeCharacter(sm, 0, strikeFlags, pWeapon, 0, 1);
        return 1;
    case 0x1f:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1f)) StrikeCharacter(sm, 1, strikeFlags, pWeapon, 0, 1);
        return 1;
    case 0x1e:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e)) {
            StrikeCharacter(sm, 1, strikeFlags, pWeapon, 0, 1);
            StrikeCharacter(sm, 0, strikeFlags, pWeapon, 0, 1);
        }
        return 1;
    }
    return 0;
}

// NEAR: 53 bytes, ENTER keeps the location hash in edi and evaluates the POI_ResolveLocation args in another order
signed char STATE_Hunter2_Goto(StateMachine &sm)
{
    int i = 0;
    EXVector &pos = sm.GetVectorRef(i);
    int &poi = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT: {
        sm.GetActorOwner().SetVariableSpeedAnims(1);
        sm.GetActorOwner().SetAnimSpeedMult(1.25f);
        sm.GetActorOwner().SetDontDoHitReaction(1);
        EXMaths::vec_xyzw d = VSub(pos, sm.GetActorOwner().GetFeetPos());
        if (4.0f > d.LengthXYZSquared()) {
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        sm.GetActorOwner().GotoVendorPOI(poi, -1);
        sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
        return 1;
    }
    case MSG_EXIT:
        return 1;
    case MSG_ENTER: {
        int loc;
        switch (sm.m_Msg.m_Data[0]) {
        case 2:
            loc = 0x42000087;
            break;
        case 3:
            loc = 0x42000088;
            break;
        case 4:
            loc = 0x42000089;
            break;
        case 6:
            loc = 0x4200008b;
            break;
        default:
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        poi = POI_ResolveLocation(loc, sm.GetActorOwner().GetNavZone(),
                                  sm.GetActorOwner().IsBoss() && !sm.GetActorOwner().m_NavRoute.JumpDisabled(),
                                  &sm.GetActorOwner().GetFeetPos());
        POI_VendorBind(poi, sm.GetActorOwner().m_RefID);
        pos = POI_VendorStandPos(poi, POI_VendorStandIndex(poi, sm.GetActorOwner().m_RefID));
        return 1;
    }
    }
    return 0;
}

// Hunter 2: one in eleven frames it turns on a nearby living 0x4f000011 actor within 3 m (stage 0x10).
#define HUNTER2_ATTACK_NEARBY() \
    { \
        EXVector here = sm.GetActorOwner().GetFeetPos(); \
        for (Actor *p = Actor::GetFirst(here, 3.0f, 0); p; p = p->GetNext(here, 3.0f, 0)) { \
            if (p->GetProfile()->m_SubGroup == 0x4f000011 && !p->IsDead() && EXRand32() % 11 == 0) { \
                targetID = p->m_RefID; \
                prevStage = stage; \
                stage = 0x10; \
                return 1; \
            } \
        } \
    }

// Hunter 2 (disc thrower): runs a scripted route of four goto points while the player is far or
// below, then fights from 0x8000 (0x4000 once fled) vantage POIs, throwing discs and striking.
// NEAR: logic read in full from the disassembly; POI distance copies and the nearby-actor loops differ
signed char STATE_Hunter2(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &gotoStep = sm.GetIntRef(i);
    int &strikeFlags = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    int &prevPoi = sm.GetIntRef(i);
    int &fled = sm.GetIntRef(i);
    int &swapNext = sm.GetIntRef(i);
    int &swapped = sm.GetIntRef(i);
    int &savedStage = sm.GetIntRef(i);
    int &prevStage = sm.GetIntRef(i);
    int &targetID = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    sm.GetActorOwner().SetHeadTrackMode(0);
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    ActorCarryableGun *pGun = sm.GetActorOwner().GetGunPtr(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXIT:
    case MSG_SUSPEND:
        if (pGun) pGun->StopFiring();
        return 1;
    case MSG_RETURN:
        if (sm.m_Msg.m_Data[0] == (int)STATE_KnockBackHunter1) {
            if (!savedStage) savedStage = stage;
            stage = 0xb;
            return 1;
        }
        if (stage == 8) {
            stage = 2;
            return 1;
        }
        if (stage == 1) stage = 0;
        return 1;
    case MSG_ENTER:
        gotoStep = 2;
        sm.GetActorOwner().m_ActorFlags.m_bDiscNotInHand = 0;
        stage = 0;
        return 1;
    case MSG_EXT: {
        if (!fled && stage != 0xf && sm.GetActorOwner().m_ActorFlags2.m_bFleeToStatue) {
            stage = 0xf;
            sm.GetActorOwner().m_InvinsibleCounter = 9999999.0f;
            sm.GetActorOwner().SetDontDoHitReaction(1);
            SM_GOSUB(STATE_RunToLocation7, 0, 0, 0, 0);
            return 1;
        }
        if (stage == 2 || stage == 5 || stage == 6) {
            int flag = fled ? 0x4000 : 0x8000;
            int old = poi;
            if (old) {
                EXVector player = Player_GetFeetPos();
                Actor &actor = sm.GetActorOwner();
                int np = POI_UpdateNearestFlag(player, flag, poi, prevPoi, actor.GetNavZone(), actor.CanJump(), 1, -1.0f);
                EXVector standOld = POI_VendorStandPos(poi, 0);
                EXVector standNew = POI_VendorStandPos(np, 0);
                EXMaths::vec_xyzw d1 = VSub(standOld, player);
                float len1 = ForceSqrt(d1.LengthXYZSquared());
                EXMaths::vec_xyzw d2 = VSub(standNew, player);
                float len2 = ForceSqrt(d2.LengthXYZSquared());
                if (len1 - 5.0f > len2) poi = np;
            } else {
                Actor &actor = sm.GetActorOwner();
                poi = POI_GetNearestFlag(Player_GetFeetPos(), flag, 0, actor.GetNavZone(), actor.CanJump(), 1, -1.0f);
            }
            if (old != poi) {
                if (old) POI_VendorUnbind(old, sm.GetActorOwner().m_RefID);
                prevPoi = old;
                if (poi) POI_VendorBind(poi, sm.GetActorOwner().m_RefID);
                if (stage == 2 || stage == 5) stage = 5;
            }
            if (!poi) {
                sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                return 1;
            }
        }
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        switch (stage) {
        case 0:
            if (225.0f > sm.GetActorOwner().m_SqDistFromPlayer || Player_GetFeetPos().y > 0.0f) {
                sm.GetActorOwner().SetDontDoHitReaction(1);
                sm.GetActorOwner().m_InvinsibleCounter = 9999999.0f;
                switch (gotoStep) {
                case 2:
                    gotoStep = 3;
                    SM_GOSUB(STATE_Hunter2_Goto, 2, 0, 0, 0);
                    return 1;
                case 3:
                    gotoStep = 4;
                    SM_GOSUB(STATE_Hunter2_Goto, 3, 0, 0, 0);
                    return 1;
                case 4:
                    gotoStep = 6;
                    SM_GOSUB(STATE_Hunter2_Goto, 4, 0, 0, 0);
                    return 1;
                case 6:
                    gotoStep = -1;
                    SM_GOSUB(STATE_Hunter2_Goto, 6, 0, 0, 0);
                    return 1;
                }
                sm.GetActorOwner().m_InvinsibleCounter = 0.0f;
                sm.GetActorOwner().SetDontDoHitReaction(0);
                stage = 2;
                return 1;
            }
            HUNTER2_ATTACK_NEARBY()
            sm.GetActorOwner().m_InvinsibleCounter = 0.0f;
            sm.GetActorOwner().SetDontDoHitReaction(0);
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (!sm.GetActorOwner().IsFacingYRot(Player_GetFeetPos(), 20.0f)) {
                sm.GetActorOwner().SetTargetYRot(Player_GetFeetPos(), 20.0f);
                return 1;
            }
            if (pGun) pGun->UpdateWait();
            if (!sm.GetActorOwner().m_ActorFlags.m_bDiscNotInHand && pGun && pGun->CanFire()) {
                pGun->StopFiring();
                sm.GetActorOwner().SetAnimType(0x9000354, 0, 1);
                stage = 1;
                return 1;
            }
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            return 1;
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) stage = 0;
            return 1;
        case 2: {
            HUNTER2_ATTACK_NEARBY()
            EXMaths::vec_xyzw d = VSub(Player_GetFeetPos(), sm.GetActorOwner().GetFeetPos());
            if (25.0f > d.x * d.x + d.y * d.y + d.z * d.z) {
                if (!swapNext) {
                    stage = 6;
                    return 1;
                }
                stage = 9;
                sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                return 1;
            }
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (!sm.GetActorOwner().IsFacingYRot(Player_GetFeetPos(), 20.0f)) {
                sm.GetActorOwner().SetTargetYRot(Player_GetFeetPos(), 20.0f);
                return 1;
            }
            if (pGun) pGun->UpdateWait();
            if (!sm.GetActorOwner().m_ActorFlags.m_bDiscNotInHand && pGun && pGun->CanFire()) {
                pGun->StopFiring();
                sm.GetActorOwner().SetAnimType(0x9000354, 0, 1);
                stage = 8;
                return 1;
            }
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            return 1;
        }
        case 3:
            if (sm.GetActorOwner().IsFacingYRot(Player_GetFeetPos(), 20.0f)) {
                strikeFlags = 0xa;
                sm.GetActorOwner().SetAnimType(0x90003e9, 0, 1);
                stage = 0xd;
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(Player_GetFeetPos(), 20.0f);
            return 1;
        case 5: {
            HUNTER2_ATTACK_NEARBY()
            EXVector stand = POI_VendorStandPos(poi, 0);
            EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), stand);
            if (0.25f > ForceSqrt(d.x * d.x + d.y * d.y + d.z * d.z)) {
                sm.GetActorOwner().InvalidateGotoPos();
                stage = 2;
                return 1;
            }
            if (sm.GetActorOwner().CanGetToVendorPOI(poi, 0)) {
                sm.GetActorOwner().GotoVendorPOI(poi, 0);
                sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
            }
            return 1;
        }
        case 6: {
            HUNTER2_ATTACK_NEARBY()
            sm.GetActorOwner().m_InvinsibleCounter = 0.0f;
            sm.GetActorOwner().SetDontDoHitReaction(0);
            EXVector stand = POI_VendorStandPos(poi, 0);
            EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), stand);
            if (ForceSqrt(d.x * d.x + d.y * d.y + d.z * d.z) > 15.0f) {
                if (swapped) {
                    swapNext = 0;
                    stage = 0xa;
                    return 1;
                }
                swapNext = 1;
                stage = 5;
                return 1;
            }
            EXMaths::vec_xyzw p = VSub(sm.GetActorOwner().GetFeetPos(), Player_GetFeetPos());
            if (1.5f > ForceSqrt(p.x * p.x + p.y * p.y + p.z * p.z)) {
                stage = 7;
                strikeFlags = 0x13;
                // Retail tests the swapped-hands flag here but both branches play the same strike set.
                sm.GetActorOwner().SetAnimType(s_aHunter1Strikes[EXRand32() & 1], 0, 1);
                return 1;
            }
            sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
            sm.GetActorOwner().GotoLoc(gAIControl.m_PlayerNavLoc);
            return 1;
        }
        case 7:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 6;
                return 1;
            }
            sm.GetActorOwner().SpinToYRot(Player_GetFeetPos());
            return 1;
        case 8:
            if (sm.GetActorOwner().HasAnimEnded()) stage = 2;
            return 1;
        case 9:
            if (sm.GetActorOwner().HasAnimEnded()) {
                swapped = 1;
                sm.GetActorOwner().SwapHands();
                stage = 6;
                return 1;
            }
            return 1;
        case 0xa:
            if (sm.GetActorOwner().HasAnimEnded()) {
                swapped = 0;
                sm.GetActorOwner().SwapHands();
                stage = 5;
                return 1;
            }
            return 1;
        case 0xb:
            sm.GetActorOwner().InvalidateGotoPos();
            sm.GetActorOwner().SetDontDoHitReaction(1);
            sm.GetActorOwner().SetCloak(1);
            sm.GetActorOwner().SetAnimType(0x90003e5, 0, 1);
            strikeFlags = 0x12;
            stage = 0xc;
            return 1;
        case 0xc:
            if (sm.GetActorOwner().HasAnimEnded()) {
                strikeFlags = 0;
                stage = 3;
                sm.GetActorOwner().SetDontDoHitReaction(0);
                return 1;
            }
            return 1;
        case 0xd:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().SetCloak(0);
                sm.GetActorOwner().SetDontDoHitReaction(0);
                sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                stage = savedStage;
                savedStage = 0;
                return 1;
            }
            return 1;
        case 0xe:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = savedStage;
                savedStage = 0;
                return 1;
            }
            return 1;
        case 0xf:
            sm.GetActorOwner().m_InvinsibleCounter = 0.0f;
            sm.GetActorOwner().SetDontDoHitReaction(0);
            stage = 2;
            fled = 1;
            return 1;
        case 0x10: {
            Actor *t = Actor::GetPtrFromID(targetID);
            if (t && !t->IsDead()) {
                EXMaths::vec_xyzw d = VSub(t->GetFeetPos(), sm.GetActorOwner().GetFeetPos());
                if (2.25f > d.x * d.x + d.y * d.y + d.z * d.z) {
                    strikeFlags = 0x13;
                    sm.GetActorOwner().SetAnimType(s_aHunter1Strikes[EXRand32() & 1], 0, 1);
                    stage = 0x11;
                    return 1;
                }
                sm.GetActorOwner().SetDontDoHitReaction(1);
                sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
                sm.GetActorOwner().GotoLoc(t->m_OrgLoc);
                return 1;
            }
            stage = prevStage;
            return 1;
        }
        case 0x11: {
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().SetDontDoHitReaction(0);
                stage = prevStage;
                return 1;
            }
            Actor *t = Actor::GetPtrFromID(targetID);
            if (t && !t->IsDead()) {
                sm.GetActorOwner().SetDontDoHitReaction(1);
                sm.GetActorOwner().SpinToYRot(t->GetFeetPos());
                return 1;
            }
            sm.GetActorOwner().SetDontDoHitReaction(0);
            return 1;
        }
        }
        return 1;
    }
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x1f:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1f))
            StrikeCharacter(sm, 1, strikeFlags, pWeapon, 0, stage == 0x11 ? targetID : 1);
        return 1;
    case 0x20:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x20))
            StrikeCharacter(sm, 0, strikeFlags, pWeapon, 0, stage == 0x11 ? targetID : 1);
        return 1;
    case 0x1e:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e)) {
            int who = stage == 0x11 ? targetID : 1;
            StrikeCharacter(sm, 1, strikeFlags, pWeapon, 0, who);
            StrikeCharacter(sm, 0, strikeFlags, pWeapon, 0, who);
        }
        return 1;
    case 0x19:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x19) && pWeapon->m_Id == 0x49000041) {
            EXVector hand = sm.GetActorOwner().GetHandPos(0);
            EXVector dir(sin(EXRandf() * 1.5707964f - 0.7853982f + sm.GetActorOwner().GetRotation().y), 0.0f,
                         cos(EXRandf() * 1.5707964f - 0.7853982f + sm.GetActorOwner().GetRotation().y), 0.0f);
            EXVector zero(0.0f, 0.0f, 0.0f, 0.0f);
            XItemHandler_Bullet::CreateFromHash(hand, zero, dir, 0x49000041, 0, sm.GetActorOwner().m_RefID, 1, 1.0f);
            sm.GetActorOwner().SetHideCarryRight(1);
            sm.GetActorOwner().m_ActorFlags.m_bDiscNotInHand = 1;
        }
        return 1;
    }
    return 0;
}
#undef HUNTER2_ATTACK_NEARBY


// NEAR: size equal; SSE register/schedule residue in the inlined Normalise and throw-position sums
signed char STATE_FS_ThrowGrenadeForwards(StateMachine &sm)
{
    int i = 0;
    int &data = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(4);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (sm.GetActorOwner().HasAnimEnded()) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_ENTER: {
        data = sm.m_Msg.m_Data[0];
        sm.GetActorOwner().SetAnimType(s_aThrowAnims[EXRand(1)], 0, 1);
        return 1;
    }
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x19:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x19)) {
            EXVector d(sin(sm.GetActorOwner().GetRotation().y), 0.0f, cos(sm.GetActorOwner().GetRotation().y), 0.0f);
            NormaliseInline(d);
            EXMaths::vec_xyzw t;
            t.x = d.x * 10.0f; t.y = d.y * 10.0f; t.z = d.z * 10.0f; t.w = d.w * 10.0f;
            const EXVector &p = sm.GetActorOwner().GetFeetPos();
            ThrowGrenade(sm, EXVector(p.x + t.x, p.y + t.y, p.z + t.z, p.w + t.w), 0, 0);
        }
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

// NEAR: written for behaviour; player-delta copies and the facing product are scheduled differently
signed char STATE_RunAtAndStrike(StateMachine &sm)
{
    int i = 0;
    float &timer = sm.GetFloatRef(i);
    int &stage = sm.GetIntRef(i);
    int &canStrike = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xd);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        switch (stage) {
        case 0: {
            timer -= XApp::GameFrameRateMul();
            if (!(0.0f >= timer)) {
                EXMaths::vec_xyzw d = VSub(Player_GetFeetPos(), sm.GetActorOwner().GetFeetPos());
                if (!(4.0f > d.LengthXYZSquared())) {
                    sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
                    sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
                    return 1;
                }
            }
            sm.GetActorOwner().SetAnimType(0x9000378, 0, 1);
            canStrike = 1;
            stage = 1;
            return 1;
        }
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) {
                SM_GOTO(STATE_QuickEvadePlayerWithGunButt, 0, 0, 0, 0);
                return 1;
            }
            return 1;
        }
        return 1;
    case MSG_EXIT:
        sm.GetActorOwner().SetCloak(0);
        sm.GetActorOwner().InvalidateGotoPos();
        return 1;
    case MSG_ENTER:
        timer = EXBaseApp::NominalGameFrameRate() * 3.0f;
        sm.GetActorOwner().SetCloak(1);
        return 1;
    case 0x1e:
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e)) return 1;
        goto hit;
    case 0x20:
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x20)) return 1;
        goto hit;
    case 0x1f:
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x1f)) return 1;
    hit:
        if (canStrike) {
            EXMaths::vec_xyzw d = VSub(Player_GetFeetPos(), sm.GetActorOwner().GetFeetPos());
            if (20.25f > d.LengthXYZSquared()) {
                EXMaths::vec_xyzw d2 = VSub(Player_GetFeetPos(), sm.GetActorOwner().GetFeetPos());
                float rot = sm.GetActorOwner().GetRotation().y;
                if ((float)(cos(rot) * d2.z + sin(rot) * d2.x) > 0.0f) {
                    EXVector feet = sm.GetActorOwner().GetFeetPos();
                    XItemHandler_Player::Instance()->SendPredMsg(7, feet, 20.0f);
                    canStrike = 0;
                }
            }
        }
        return 1;
    }
    return 0;
}

// NEAR: written for behaviour; lifecycle jump table and delta-copy residue
signed char STATE_ZombieAttack(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &strikeFlags = sm.GetIntRef(i);
    float &seenTimer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xd);
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_SUSPEND:
        sm.GetActorOwner().SetAnimSpeedMult(1.0f);
        return 1;
    case MSG_ENTER:
        seenTimer = sm.GetActorOwner().GetCombatTimeout();
        return 1;
    case MSG_EXT:
        if (!Actor_CanSeeTarget(sm.GetActorOwner(), 1, seenTimer, sm.GetActorOwner().GetCombatTimeout())) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        switch (stage) {
        case 0: {
            sm.GetActorOwner().GotoLoc(gAIControl.PlayerNavLocLastSeen());
            sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
            EXVector d = VSub(sm.GetActorOwner().GetFeetPos(), gAIControl.LastSeenPlayerPos());
            if (2.25f > d.LengthXZSquared()) {
                sm.GetActorOwner().InvalidateGotoPos();
                stage = 1;
            }
            return 1;
        }
        case 1: {
            int anim;
            int r = EXRand(2);
            if (r) {
                anim = s_aStrikeOneArmA[EXRand32() & 3];
                strikeFlags = 1;
            } else {
                anim = s_aStrikeOneArmB[EXRand32() & 3];
                strikeFlags = 2;
            }
            sm.GetActorOwner().SetAnimType(anim, 0, 1);
            sm.GetActorOwner().SetAnimSpeedMult(0.375f);
            stage = 2;
            return 1;
        }
        case 2:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().SetAnimSpeedMult(1.0f);
                stage = 0;
                return 1;
            }
            sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
            return 1;
        }
        return 1;
    case MSG_EXIT:
        sm.GetActorOwner().SetAnimSpeedMult(1.0f);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x1f:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1f)) StrikeCharacter(sm, 1, strikeFlags, pWeapon, 0, 1);
        return 1;
    case 0x20:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x20)) StrikeCharacter(sm, 0, strikeFlags, pWeapon, 0, 1);
        return 1;
    case 0x1e:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e)) {
            StrikeCharacter(sm, 1, strikeFlags, pWeapon, 0, 1);
            StrikeCharacter(sm, 0, strikeFlags, pWeapon, 0, 1);
        }
        return 1;
    }
    return 0;
}

// NEAR: written for behaviour; player-delta copies and x87/SSE scheduling residue
signed char STATE_RoninPredAttackLeapBack(StateMachine &sm)
{
    int i = 0;
    int &hit = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (sm.GetActorOwner().HasAnimEnded()) {
            sm.GetActorOwner().SetDontDoHitReaction(0);
            sm.Return((int)STATE_RoninPredAttackLeapBack, 0, 0, 0);
            return 1;
        }
        sm.GetActorOwner().SetInvinsibleCounter(5);
        return 1;
    case MSG_EXIT:
        sm.GetActorOwner().SetDontDoHitReaction(0);
        return 1;
    case MSG_ENTER:
        sm.GetActorOwner().SetDontDoHitReaction(1);
        sm.GetActorOwner().SetAnimType(0x9000234, 0, 1);
        hit = 0;
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x33:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x33) && !hit) {
            float s = sin(sm.GetActorOwner().GetRotation().y);
            float c = cos(sm.GetActorOwner().GetRotation().y);
            EXMaths::vec_xyzw d = VSub(Player_GetFeetPos(), sm.GetActorOwner().GetFeetPos());
            EXVector push(d.x, 0.0f, d.z, d.w);
            if ((float)(d.z * c + d.x * s + d.w * 0.0f) > 0.0f && 1.5f > (float)fabs(d.y)) {
                EXVector away = VSub(sm.GetActorOwner().GetFeetPos(), Player_GetFeetPos());
                if (4.0f > away.LengthXZSquared()) {
                    NormaliseInline(push);
                    EXVector feet = sm.GetActorOwner().GetFeetPos();
                    XItemHandler_Player::Instance()->SendPredMsg(0, feet, push, 10.0f);
                    XItemHandler_Player::Instance()->SendPredMsg(5, feet, 0.5f);
                    hit = 1;
                }
            }
        }
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

// Player is ahead of the actor (half-space test against the facing direction).
static __forceinline signed char Ronin_IsAhead(Actor &actor, const EXVector &pos)
{
    float ry = actor.GetRotation().y;
    EXVector fwd(sin(ry), 0.0f, cos(ry), 0.0f);
    EXMaths::vec_xyzw d = VSub(pos, actor.GetFeetPos());
    return d.x * fwd.x + d.y * fwd.y + d.z * fwd.z + d.w * fwd.w > 0.0f;
}

// Ronin boss: closes on the player's nav location, strikes in a 4-hit combo, leaps aside, throws
// fans of seven shuriken (0x4900004d) and grenades on request, cloaking after a back-flip.
// NEAR: logic read in full from the disassembly; shuriken fan vector math and spills differ
signed char STATE_RoninPredAttack(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &strikeFlags = sm.GetIntRef(i);
    int &combo = sm.GetIntRef(i);
    int &throwType = sm.GetIntRef(i);
    int &starsLeft = sm.GetIntRef(i);
    int &strafed = sm.GetIntRef(i);
    int &cloakNext = sm.GetIntRef(i);
    int &cloakCount = sm.GetIntRef(i);
    float &seenTimer = sm.GetFloatRef(i);
    float &flipTimer = sm.GetFloatRef(i);
    float &voTimer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    sm.GetActorOwner().SetHeadTrackMode(0);
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXIT:
        sm.GetActorOwner().SetDontDoHitReaction(0);
        return 1;
    case MSG_ENTER:
        combo = 0;
        stage = 0;
        flipTimer = (EXRandf() + 1.0f) * EXBaseApp::NominalGameFrameRate() * 2.0f;
        throwType = 0;
        return 1;
    case MSG_RETURN:
        if (sm.m_Msg.m_Data[0] == (int)STATE_RoninPredAttackLeapBack) {
            if (Ronin_IsAhead(sm.GetActorOwner(), gAIControl.LastSeenPlayerPos())) {
                EXMaths::vec_xyzw d = VSub(gAIControl.LastSeenPlayerPos(), sm.GetActorOwner().GetFeetPos());
                if (12.25f > d.x * d.x + d.y * d.y + d.z * d.z) {
                    strikeFlags = 0x13;
                    sm.GetActorOwner().SetAnimType(0x9000520, 0, 1);
                    sm.GetActorOwner().SetDontDoHitReaction(1);
                    stage = 4;
                    return 1;
                }
            }
            throwType = Player_IsCloaked() ? 2 : 0;
            stage = 8;
            cloakNext = 1;
        }
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_EXT: {
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        Actor &actor = sm.GetActorOwner();
        float timeout = actor.GetCombatTimeout();
        signed char seen = actor.WasPlayerSeen();
        if (seen) seenTimer = timeout;
        seenTimer -= EXBaseApp::GameFrameRateMul();
        if (seen && !Player_IsCloaked()) gAIControl.SetLastSeenPlayerPos(Player_GetFeetPos());
        if (flipTimer > 0.0f) flipTimer -= XApp::GameFrameRateMul();
        if (starsLeft) {
            // One shuriken of the fan per frame, spread sideways by (n - 4.5) * 0.05 and pitched at the player.
            EXVector zero(0.0f, 0.0f, 0.0f, 0.0f);
            EXVector hand = sm.GetActorOwner().GetHandPos(0);
            float ry = sm.GetActorOwner().GetRotation().y;
            EXVector fwd(sin(ry), 0.0f, cos(ry), 0.0f);
            EXMaths::vec_xyzw d = VSub(gAIControl.LastSeenPlayerPos(), sm.GetActorOwner().GetFeetPos());
            float sq = d.y * d.y + d.z * d.z + d.x * d.x;
            float slope = d.y / (sq >= 0.0f ? (float)sqrt(sq) : 0.0f);
            EXVector side(0.0f - cos(ry), 0.0f, 0.0f - -sin(ry), 0.0f);
            float k = (float)starsLeft - 4.5f;
            EXVector s(side.x * k * 0.05f, side.y * k * 0.05f, side.z * k * 0.05f, side.w * k * 0.05f);
            EXVector dir(fwd.x + s.x, s.y + slope, fwd.z + s.z, s.w);
            NormaliseInline(dir);
            starsLeft--;
            WeaponProfile *pStar = WeaponProfile::GetWeaponData(0x4900004d);
            if (pStar) sm.GetActorOwner().PlaySFX(pStar->m_SfxFire, 0);
            XItemHandler_Bullet::CreateFromHash(hand, zero, dir, 0x4900004d, 0, sm.GetActorOwner().m_RefID, 1, 1.0f);
        }
        switch (stage) {
        case 8:
            sm.GetActorOwner().SetAnimType(0x9000540, 0, 1);
            sm.GetActorOwner().InvalidateGotoPos();
            flipTimer = (EXRandf() + 1.0f) * EXBaseApp::NominalGameFrameRate() * 2.0f;
            stage = 9;
            strafed = 0;
            if (cloakCount > 0 && --cloakCount == 0) sm.GetActorOwner().SetCloak(0);
            return 1;
        case 9:
            if (sm.GetActorOwner().HasAnimEnded()) {
                if (cloakNext) {
                    cloakNext = 0;
                    cloakCount = 2;
                    sm.GetActorOwner().SetCloak(1);
                }
                if ((EXRand32() & 3) == 0) {
                    if (EXRand32() & 1) {
                        if (sm.GetActorOwner().CanStrafeLeft(1.5f)) {
                            sm.GetActorOwner().SetAnimType(0x9000232, 0, 1);
                            stage = 0xa;
                            strafed = 1;
                            return 1;
                        }
                    } else if (sm.GetActorOwner().CanStrafeRight(1.5f)) {
                        sm.GetActorOwner().SetAnimType(0x9000233, 0, 1);
                        stage = 0xa;
                        strafed = 1;
                        return 1;
                    }
                }
                stage = 2;
                return 1;
            }
            if (Ronin_IsAhead(sm.GetActorOwner(), gAIControl.LastSeenPlayerPos()))
                sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
            return 1;
        case 0: {
            if (0.0f > flipTimer && !starsLeft && Ronin_IsAhead(sm.GetActorOwner(), gAIControl.m_PlayerNavLocLastSeen.Pos())) {
                stage = 8;
                return 1;
            }
            EXMaths::vec_xyzw d = VSub(gAIControl.m_PlayerNavLocLastSeen.Pos(), sm.GetActorOwner().GetFeetPos());
            if (d.y * d.y + d.z * d.z + d.x * d.x > 16.0f) {
                if (gAIControl.m_DistOffNaviMeshLastSeen > 1.0f) {
                    sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                    return 1;
                }
                sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
                sm.GetActorOwner().GotoLoc(gAIControl.m_PlayerNavLocLastSeen);
                return 1;
            }
            sm.GetActorOwner().InvalidateGotoPos();
            stage = 2;
            strafed = 0;
            return 1;
        }
        case 1: {
            if (0.0f > flipTimer && !starsLeft && Ronin_IsAhead(sm.GetActorOwner(), gAIControl.LastSeenPlayerPos())) {
                stage = 8;
                return 1;
            }
            EXMaths::vec_xyzw d = VSub(gAIControl.m_PlayerNavLocLastSeen.Pos(), sm.GetActorOwner().GetFeetPos());
            float sq = d.y * d.y + d.z * d.z + d.x * d.x;
            if (sq > 256.0f) {
                stage = 0;
                return 1;
            }
            if (sq > 16.0f) {
                if (gAIControl.m_DistOffNaviMeshLastSeen > 1.0f) {
                    sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                    return 1;
                }
                sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
                sm.GetActorOwner().GotoLoc(gAIControl.m_PlayerNavLocLastSeen);
                return 1;
            }
            sm.GetActorOwner().InvalidateGotoPos();
            stage = 2;
            combo = 0;
            strafed = 0;
            return 1;
        }
        case 2: {
            EXMaths::vec_xyzw d = VSub(gAIControl.m_PlayerNavLocLastSeen.Pos(), sm.GetActorOwner().GetFeetPos());
            float sq = d.x * d.x + d.y * d.y + d.z * d.z;
            if (sq > 64.0f) {
                stage = 1;
                return 1;
            }
            if (!sm.GetActorOwner().IsFacingYRot(gAIControl.LastSeenPlayerPos(), 5.0f)) {
                sm.GetActorOwner().SetTargetYRot(gAIControl.LastSeenPlayerPos(), 5.0f);
                return 1;
            }
            if (!(0.0f < voTimer)) {
                if (sm.GetActorOwner().TryToSpeakCombatVO(0))
                    voTimer = (EXRandf() * 5.0f + 20.0f) * EXBaseApp::NominalGameFrameRate();
            } else {
                voTimer -= XApp::GameFrameRateMul();
            }
            switch (EXRand32() & 3) {
            case 0:
                if (!strafed && sm.GetActorOwner().CanStrafeLeft(1.5f)) {
                    if (EXRand32() & 1) {
                        sm.GetActorOwner().SetAnimType(0x4e000007, 0, 1);
                        stage = 5;
                    } else {
                        sm.GetActorOwner().SetAnimType(0x9000232, 0, 1);
                        stage = 0xa;
                    }
                    strafed = 1;
                }
                break;
            case 1:
                if (!strafed && sm.GetActorOwner().CanStrafeRight(1.5f)) {
                    if (EXRand32() & 1) {
                        sm.GetActorOwner().SetAnimType(0x4e000008, 0, 1);
                        stage = 5;
                    } else {
                        sm.GetActorOwner().SetAnimType(0x9000233, 0, 1);
                        stage = 0xa;
                    }
                    strafed = 1;
                }
                break;
            default:
                if (!XItemHandler_Player::Instance()->IsHanging() && !(gAIControl.m_DistOffNaviMeshLastSeen > 1.0f)) {
                    if (EXRand32() % 3 == 0 && sq > 16.0f) {
                        strikeFlags = 0xa;
                        sm.GetActorOwner().SetDontDoHitReaction(1);
                        sm.GetActorOwner().SetAnimType(0x90003e9, 0, 1);
                        stage = 5;
                        return 1;
                    }
                    strafed = 0;
                    stage = 3;
                    return 1;
                }
                if (0.0f > flipTimer && !starsLeft && Ronin_IsAhead(sm.GetActorOwner(), gAIControl.LastSeenPlayerPos())) {
                    EXVector n = VSub(gAIControl.LastSeenPlayerPos(), sm.GetActorOwner().GetFeetPos());
                    NormaliseInline(n);
                    if (0.95f > fabs(n.y)) {
                        stage = 8;
                        return 1;
                    }
                }
                break;
            }
            if (stage == 2) sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            return 1;
        }
        case 3: {
            EXMaths::vec_xyzw d = VSub(gAIControl.m_PlayerNavLocLastSeen.Pos(), sm.GetActorOwner().GetFeetPos());
            float sq = d.x * d.x + d.y * d.y + d.z * d.z;
            if (sq > 64.0f || gAIControl.m_DistOffNaviMeshLastSeen > 1.1f) {
                stage = 2;
                return 1;
            }
            if (2.25f > sq) {
                sm.GetActorOwner().InvalidateGotoPos();
                switch (combo) {
                case 0:
                case 1:
                case 2: {
                    int anims[3] = { 0x900051e, 0x900051f, 0x900051e };
                    strikeFlags = 0x13;
                    sm.GetActorOwner().SetAnimType(anims[combo], 0, 1);
                    break;
                }
                case 3:
                    strikeFlags = 0x13;
                    sm.GetActorOwner().SetAnimType(0x9000520, 0, 1);
                    break;
                }
                sm.GetActorOwner().SetDontDoHitReaction(1);
                if (++combo > 3) combo = 0;
                stage = 4;
                return 1;
            }
            sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
            sm.GetActorOwner().GotoLoc(gAIControl.m_PlayerNavLocLastSeen);
            return 1;
        }
        case 4:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().SetDontDoHitReaction(0);
                stage = (combo != 0) + 2;
                return 1;
            }
            sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
            return 1;
        case 5:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().SetDontDoHitReaction(0);
                stage = 2;
                return 1;
            }
            sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
            return 1;
        case 0xa:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().SetDontDoHitReaction(0);
                stage = 2;
                return 1;
            }
            sm.GetActorOwner().m_InvinsibleCounter = 5.0f;
            sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
            return 1;
        case 6:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 2;
                return 1;
            }
            return 1;
        case 7:
            if (sm.GetActorOwner().HasAnimEnded()) stage = 1;
            return 1;
        }
        return 1;
    }
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x1f:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1f)) StrikeCharacter(sm, 1, strikeFlags, pWeapon, 0, 1);
        return 1;
    case 0x20:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x20)) StrikeCharacter(sm, 0, strikeFlags, pWeapon, 0, 1);
        return 1;
    case 0x1e:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e)) {
            StrikeCharacter(sm, 1, strikeFlags, pWeapon, 0, 1);
            StrikeCharacter(sm, 0, strikeFlags, pWeapon, 0, 1);
        }
        return 1;
    case MSG_INCOMING_ATTACK:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_INCOMING_ATTACK) && stage != 0xa)
            AU_HandleIncomingAttack(sm, (StructMsg_IncomingAttack *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x19:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x19)) {
            switch (throwType) {
            case 0:
                starsLeft = 7;
                break;
            case 1:
                ThrowGrenade(sm, gAIControl.LastSeenPlayerPos(), 0x4900000e, 1);
                break;
            case 2:
                ThrowGrenade(sm, gAIControl.LastSeenPlayerPos(), 0x49000062, 1);
                break;
            }
            throwType = 0;
        }
        return 1;
    }
    return 0;
}

// Ulysses (two guns): heads for 0x20000 POIs near the player, fires the left gun at the player
// (pitch clamped to +-3 m) or the right gun when the player is far above/below, and every few
// approaches swings a close-range blow (0x90001f2) resolved by the strike messages.
// NEAR: logic read in full from the disassembly; aim vector copies and the shared strike tail differ
signed char STATE_NewUlysesAttackPred(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &fireMode = sm.GetIntRef(i);
    int &strikePending = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    int &arrived = sm.GetIntRef(i);
    float &seenTimer = sm.GetFloatRef(i);
    float &giveUpTimer = sm.GetFloatRef(i);
    int &strikeCount = sm.GetIntRef(i);
    float &strikeTimer = sm.GetFloatRef(i);
    int &shots = sm.GetIntRef(i);
    int &rightFiring = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    ActorCarryableGun *pLeft = sm.GetActorOwner().GetGunPtr(0);
    ActorCarryableGun *pRight = sm.GetActorOwner().GetGunPtr(1);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXIT:
        if (poi) {
            POI_VendorUnbind(poi, sm.GetActorOwner().m_RefID);
            poi = 0;
        }
        if (pLeft) pLeft->StopFiring();
        if (pRight) pRight->StopFiring();
        return 1;
    case MSG_ENTER:
        giveUpTimer = (EXRandf() * 2.0f + 4.0f) * EXBaseApp::NominalGameFrameRate();
        seenTimer = 0.0f;
        stage = 0;
        fireMode = 0;
        shots = 4;
        strikeCount = EXRand32() % 3 + 2;
        strikeTimer = (EXRandf() + 1.0f) * EXBaseApp::NominalGameFrameRate() * 2.0f;
        return 1;
    case MSG_EXT: {
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        Actor &actor = sm.GetActorOwner();
        float timeout = actor.GetCombatTimeout();
        signed char seen = actor.WasPlayerSeen();
        if (seen) seenTimer = timeout;
        seenTimer -= EXBaseApp::GameFrameRateMul();
        if (seen && !Player_IsCloaked()) gAIControl.SetLastSeenPlayerPos(Player_GetFeetPos());
        if (!(seenTimer < 0.0f)) seen = 1;
        giveUpTimer -= EXBaseApp::GameFrameRateMul();
        if (0.0f > seenTimer && 0.0f > giveUpTimer) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        if (seen) {
            sm.GetActorOwner().SetHeadTrackMode(2);
            sm.GetActorOwner().SetAimingPos(GetUlyssesAimPos(sm));
        } else {
            sm.GetActorOwner().SetHeadTrackMode(0);
        }
        switch (fireMode) {
        case 0:
            pLeft->UpdateWait();
            pRight->UpdateWait();
            if (seen) {
                if (gCurrentMap_GeoHash == 0x10000e1) shots = 0x63;
                if (stage == 0) {
                    if (shots) {
                        EXVector head = sm.GetActorOwner().GetHeadDirection();
                        EXMaths::vec_xyzw d = VSub(gAIControl.LastSeenPlayerPos(), sm.GetActorOwner().GetFeetPos());
                        if (!(fabs(d.y) > 3.5f)) {
                            EXVector n(d.x, d.y, d.z, d.w);
                            NormaliseInline(n);
                            if (head.z * n.z + head.x * n.x + head.y * n.y > 0.9f && pLeft->CanFire()) {
                                EXVector from = gAIControl.LastSeenPlayerPos();
                                from.y += 1.5f;
                                EXVector hp = sm.GetActorOwner().GetHeadPos();
                                EXRay ray;
                                ray.Direction() = VSub(from, hp);
                                ray.Origin() = hp;
                                if (!sm.GetActorOwner().FriendlyActorBlockingLineOfSight(ray) && pLeft->StartFiring()) {
                                    fireMode = 1;
                                    shots--;
                                }
                            }
                            break;
                        }
                    }
                    if (pRight->CanFire() && pRight->StartFiring()) {
                        sm.GetActorOwner().SetAnimType(0x9000438, 0, 1);
                        stage = 4;
                        fireMode = 2;
                        rightFiring = 1;
                    }
                }
            }
            break;
        case 1: {
            EXVector head = sm.GetActorOwner().GetHeadDirection();
            EXMaths::vec_xyzw d = VSub(gAIControl.LastSeenPlayerPos(), sm.GetActorOwner().GetFeetPos());
            EXVector n(d.x, d.y, d.z, d.w);
            float len = NormaliseInline(n);
            if (head.x * n.x + head.z * n.z + head.y * n.y > 0.7f) {
                EXVector hand = sm.GetActorOwner().GetHandPos(0);
                EXVector aim = sm.GetActorOwner().GetAimPosForTarget(1);
                float dy = hand.y - aim.y;
                dy = min(dy, 3.0f);
                dy = max(dy, -3.0f);
                float r = max(10.0f, len);
                EXVector reach(head.x * r, head.y * r, head.z * r, head.w * r);
                EXVector p = VAdd(sm.GetActorOwner().GetHandPos(0), reach);
                EXVector target(p.x, p.y - dy, p.z, p.w);
                if (!pLeft->UpdateFiring(sm.GetActorOwner(), target, 0, 1)) fireMode = 0;
            } else {
                pLeft->StopFiring();
                fireMode = 0;
            }
            break;
        }
        case 2:
            shots = 4;
            if (stage != 4) fireMode = 0;
            break;
        }
        if (sm.GetActorOwner().GetConsecutiveHitCounter() >= SubGroupAnims::GetSubGroupAnims(sm.GetActorOwner().m_SubGroupAnimIndex).m_Anims[11]) {
            sm.GetActorOwner().ClearConsecutiveHitCounter();
            sm.GetActorOwner().SetInvinsibleCounter(EXBaseApp::NominalGameFrameRate() * 2);
            stage = 2;
        }
        switch (stage) {
        case 0: {
            sm.GetActorOwner().TryToSpeakCombatVO(0);
            EXVector player = gAIControl.LastSeenPlayerPos();
            int playerPoly = gAIControl.m_PlayerNavLocLastSeen.GetPolyHandle();
            EXMaths::vec_xyzw d = VSub(player, sm.GetActorOwner().GetFeetPos());
            float sq = d.y * d.y + d.z * d.z + d.x * d.x;
            if (strikeCount > 0) {
                if (16.0f > sq) {
                    EXVector head = sm.GetActorOwner().GetHeadDirection();
                    EXVector n = VSub(gAIControl.LastSeenPlayerPos(), sm.GetActorOwner().GetFeetPos());
                    NormaliseInline(n);
                    if (head.x * n.x + head.z * n.z + head.y * n.y > 0.8f) {
                        stage = 2;
                        return 1;
                    }
                }
            } else {
                if (sq > 225.0f) {
                    strikeTimer = (EXRandf() + 1.0f) * EXBaseApp::NominalGameFrameRate() * 2.0f;
                    strikeCount = EXRand32() % 3 + 2;
                }
                if (strikeTimer > 0.0f) {
                    strikeTimer -= XApp::GameFrameRateMul();
                } else {
                    strikeTimer = (EXRandf() + 1.0f) * EXBaseApp::NominalGameFrameRate() * 2.0f;
                    strikeCount = EXRand32() % 3 + 2;
                }
            }
            {
                int old = poi;
                Actor &a = sm.GetActorOwner();
                if (old) {
                    if (!(EXRand32() & 1))
                        poi = POI_UpdateNearestFlag(player, 0x20000, old, 0, a.GetNavZone(), a.CanJump(), 1, -1.0f);
                } else {
                    poi = POI_GetNearestFlag(player, 0x20000, 0, a.GetNavZone(), a.CanJump(), 1, -1.0f);
                }
                if (old != poi) {
                    if (old) POI_VendorUnbind(old, sm.GetActorOwner().m_RefID);
                    if (poi) POI_VendorBind(poi, sm.GetActorOwner().m_RefID);
                }
            }
            if (!poi) {
                sm.GetActorOwner().GotoPosPoly(player, playerPoly);
                sm.GetActorOwner().SetAnimType(0x90003a1, 0, 1);
                return 1;
            }
            EXVector stand = POI_VendorStandPos(poi, 0);
            int poly = POI_VendorStandPoly(poi, 0);
            EXMaths::vec_xyzw s = VSub(sm.GetActorOwner().GetFeetPos(), stand);
            float ssq = s.x * s.x + s.y * s.y + s.z * s.z;
            if (16.0f > ssq || (arrived && 100.0f > ssq)) {
                arrived = 1;
                sm.GetActorOwner().SetAnimType(0x90003a1, 0, 1);
                return 1;
            }
            arrived = 0;
            sm.GetActorOwner().GotoPosPoly(stand, poly);
            sm.GetActorOwner().SetAnimType(0x900039f, 0, 1);
            return 1;
        }
        case 2:
            sm.GetActorOwner().SetDontDoHitReaction(1);
            sm.GetActorOwner().SetAnimType(0x90001f2, 0, 1);
            stage = 3;
            strikePending = 1;
            strikeCount--;
            if (pLeft) pLeft->StopFiring();
            if (pRight) pRight->StopFiring();
            return 1;
        case 3:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().m_InvinsibleCounter = 0.0f;
                sm.GetActorOwner().SetDontDoHitReaction(0);
                stage = 0;
                strikePending = 0;
                return 1;
            }
            return 1;
        case 4:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 0;
                fireMode = 0;
                return 1;
            }
            if (seen) {
                EXVector head = sm.GetActorOwner().GetHeadDirection();
                EXVector far(head.x * 1000.0f, head.y * 1000.0f, head.z * 1000.0f, head.w * 1000.0f);
                sm.GetActorOwner().SetAimingPos(VAdd(sm.GetActorOwner().GetHeadPos(), far));
            }
            return 1;
        }
        return 1;
    }
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_SUSPEND:
        pLeft->StopFiring();
        pRight->StopFiring();
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x1f:
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x1f)) return 1;
        goto strike;
    case 0x20:
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x20)) return 1;
        goto strike;
    case 0x1e:
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e)) return 1;
    strike:
        if (strikePending) {
            XItemHandler_Player *pPlayer = XItemHandler_Player::Instance();
            if (pPlayer->m_Mode == 0x14 && pPlayer->m_SpecialActorID == (unsigned int)sm.GetActorOwner().m_RefID) {
                // The player is grappling Ulysses: push the player back along Ulysses' facing.
                EXVector p = sm.GetActorOwner().GetFeetPos();
                float ry = sm.GetActorOwner().GetRotation().y;
                EXVector back(p.x - (float)sin(ry), p.y, p.z - (float)cos(ry), p.w);
                XItemHandler_Player::Instance()->SendPredMsg(5, back, 0.5f);
                strikePending = 0;
                return 1;
            }
            EXVector p = sm.GetActorOwner().GetFeetPos();
            EXSphere sphere;
            sphere.Centre() = p;
            sphere.Radius() = 2.0f;
            EXCapsule capsule;
            capsule.Origin() = Player_GetFeetPos();
            capsule.Direction() = EXVector(0.0f, 2.0f, 0.0f, 0.0f);
            capsule.Radius() = 1.0f;
            if (EXIntersection(sphere, capsule)) {
                Player_DoDamage(20.0f);
                XItemHandler_Player::Instance()->SendPredMsg(7, p, 1.0f);
            }
            strikePending = 0;
        }
        return 1;
    case 0x19:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x19) && rightFiring) {
            if (!pRight->UpdateFiring(sm.GetActorOwner(), sm.GetActorOwner().GetAimPosForTarget(1), 0, 1)) rightFiring = 0;
        }
        return 1;
    case 0x2a:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x2a)) {
            pLeft->ForceRoundToEnd();
            pRight->ForceRoundToEnd();
        }
        return 1;
    }
    return 0;
}

signed char STATE_UlysesAttackPred(StateMachine &sm)
{
    SM_GOTO(STATE_NewUlysesAttackPred, 0, 0, 0, 0);
    return 1;
}

signed char STATE_UlyssesLookAtPlayer(StateMachine &sm)
{
    int anims[17] = { 0x9000254, 0x9000251, 0x9000252, 0x9000253, 0x900028b, 0x900040b, 0x9000291, 0x900040a, 0x9000409,
                      0x90001f2, 0x900039f, 0x90003a1, 0x9000438, 0x90001f3, 0x900028c, 0x9000255, 0x90001f5 };
    int i = 0;
    int &idx = sm.GetIntRef(i);
    switch (sm.m_Msg.m_Type) {
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_EXT:
        sm.GetActorOwner().SetHealth(1000.0f, 2);
        gAIControl.SetLastSeenPlayerPos(Player_GetFeetPos());
        sm.GetActorOwner().SetHeadTrackMode(2);
        sm.GetActorOwner().SetAimingPos(GetUlyssesAimPos(sm));
        if (sm.GetActorOwner().HasAnimEnded()) {
            sm.GetActorOwner().SetAnimType(anims[idx], 0, 1);
            if (++idx == 17) idx = 0;
        }
        return 1;
    case MSG_ENTER:
        idx = 0;
        sm.GetActorOwner().SetAnimType(anims[idx], 0, 1);
        idx++;
        return 1;
    }
    return 0;
}

signed char STATE_UlyssesHitbyEMP(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetWatch(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        if (sm.m_Msg.m_Data[0] == (int)STATE_UlysesPredOnHead) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_EXIT:
        sm.GetActorOwner().SetDontDoHitReaction(0);
        return 1;
    case MSG_ENTER: {
        if (sm.GetActorOwner().GetSkin()->m_Hash == 0xd000101) {
            float t = EXRandf(1.0f, 2.0f) * 2.0f;
            timer = t * EXBaseApp::NominalGameFrameRate();
        } else {
            float t = EXRandf(6.0f, 10.0f);
            timer = t * EXBaseApp::NominalGameFrameRate();
        }
        stage = 0;
        sm.GetActorOwner().SetDontDoHitReaction(1);
        return 1;
    }
    case MSG_EXT:
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        switch (stage) {
        case 0:
            sm.GetActorOwner().SetAnimType(0x900028b, 0, 1);
            stage = 1;
            return 1;
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().SetAnimType(0x900040b, 0, 1);
                stage = 2;
                return 1;
            }
            return 1;
        case 2:
            timer -= EXBaseApp::GameFrameRateMul();
            if (timer <= 0.0f) {
                sm.GetActorOwner().SetAnimType(0x9000291, 0, 1);
                stage = 3;
                return 1;
            }
            return 1;
        case 3:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.Return(0, 0, 0, 0);
                return 1;
            }
            return 1;
        }
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR)) {
            StructMsg_AttackedByPredator *pMsg = (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0];
            if (pMsg->m_Attack == 8) {
                float r = EXRandf(6.0f, 10.0f);
                float t = r * EXBaseApp::NominalGameFrameRate();
                timer = max(timer, t);
                return 1;
            }
            AU_HandlePlayerAttacks(sm, pMsg);
        }
        return 1;
    }
    return 0;
}

signed char STATE_MonsterSquadHitbyEMP(StateMachine &sm)
{
    static int s_SpeechDelay;
    int i = 0;
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXIT:
        sm.GetActorOwner().SetDontDoHitReaction(0);
        return 1;
    case MSG_ENTER: {
        float t = EXRandf(5.0f, 7.0f);
        timer = t * EXBaseApp::NominalGameFrameRate();
        sm.GetActorOwner().SetDontDoHitReaction(1);
        sm.GetActorOwner().SetAnimType(0x9000257, 0, 1);
        if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f00000f) {
            sm.GetActorOwner().Speak(0x1af00580, 0);
            return 1;
        }
        return 1;
    }
    case MSG_EXT:
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        timer -= EXBaseApp::GameFrameRateMul();
        if (timer <= 0.0f) {
            if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f00000f) {
                if (s_SpeechDelay > 0) s_SpeechDelay--;
                if (!s_SpeechDelay && EXRand32() % 3 == 0) {
                    s_SpeechDelay = 2;
                    sm.GetActorOwner().Speak(0x1af004ac, 0);
                }
            }
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR)) {
            StructMsg_AttackedByPredator *pMsg = (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0];
            if (pMsg->m_Attack == 8) {
                float r = EXRandf(6.0f, 10.0f);
                float t = r * EXBaseApp::NominalGameFrameRate();
                timer = max(timer, t);
                return 1;
            }
            AU_HandlePlayerAttacks(sm, pMsg);
        }
        return 1;
    }
    return 0;
}

// NEAR: retail re-reads the pad buttons for the second test (shr/test form)
signed char STATE_HoldItemTest(StateMachine &sm)
{
    static int s_aItems[84] = {
        0x020000b4, 0x020000b5, 0x020000b6, 0x020000b7, 0x020000b9, 0x020000bb, 0x020000bc,
        0x020000bd, 0x020000c0, 0x020000c1, 0x020000c2, 0x020000c9, 0x020000c4, 0x020000c6,
        0x020000c7, 0x020000b1, 0x020000b2, 0x020001ce, 0x020001cd, 0x020001d4, 0x020000e5,
        0x020000e7, 0x020000e8, 0x020000e9, 0x020000e2, 0x020000e3, 0x020000e6, 0x020000e4,
        0x020000d3, 0x020000d6, 0x020000b8, 0x020000cb, 0x020000ce, 0x020000ba, 0x020000cf,
        0x020000db, 0x020000cd, 0x020000d2, 0x020000be, 0x020000ca, 0x02000237, 0x0200017e,
        0x02000236, 0x020000d4, 0x020000bf, 0x020000da, 0x020000dd, 0x020000d0, 0x020000e0,
        0x020000df, 0x020000de, 0x020000d8, 0x020000dc, 0x02000271, 0x0200024a, 0x020000d1,
        0x020000e1, 0x020000cc, 0x020000d9, 0x020000d5, 0x02000120, 0x02000139, 0x02000159,
        0x02000249, 0x0200016d, 0x02000184, 0x02000185, 0x02000187, 0x02000188, 0x02000189,
        0x02000186, 0x0200018e, 0x0200018c, 0x0200018d, 0x020001fc, 0x020001fe, 0x020001ff,
        0x02000202, 0x02000200, 0x02000201, 0x02000203, 0x02000204, 0x02000232, 0x02000231
    };
    static int s_nItems = 84;
    static int s_iItem;
    static int s_State;
    int i = 0;
    int &item = sm.GetIntRef(i);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT: {
        EXBaseGamePad *pPad = EXBaseGamePad::GetGamePad();
        if (pPad) {
            if (pPad->Buttons(0) & 1) {
                if (!s_State) {
                    if (++s_iItem >= s_nItems) s_iItem = 0;
                    s_State = 1;
                }
            } else if ((pPad->Buttons(0) >> 1) & 1) {
                if (!s_State) {
                    if (--s_iItem < 0) s_iItem = s_nItems - 1;
                    s_State = 1;
                }
            } else {
                s_State = 0;
                return 1;
            }
        }
        if (s_State == 1) {
            item = s_aItems[s_iItem];
            if (item)
                sm.GetActorOwner().SetCarry(ActorCarryable::Create(0x100003c, item)->GetRefID(), 0);
            else
                sm.GetActorOwner().SetCarry(0, 0);
            s_State = 2;
        }
        return 1;
    }
    case MSG_ENTER:
        if (sm.GetActorOwner().IsBoss())
            sm.GetActorOwner().SetAnimType(0x9000475, 0, 1);
        else
            sm.GetActorOwner().SetAnimType(0x9000468, 0, 1);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_StaggerAndDie(StateMachine &sm)
{
    int i = 0;
    float &rot = sm.GetFloatRef(i);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (!sm.GetActorOwner().IsFacingYRot(rot, 5.0f)) {
            sm.GetActorOwner().SpinToYRot(rot);
            return 1;
        }
        return 1;
    case MSG_ENTER: {
        if (!sm.GetActorOwner().IsLeftArmMissing() && !sm.GetActorOwner().IsRightArmMissing()) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        sm.GetActorOwner().InvalidateGotoPos();
        int anims[2] = { 0x900032b, 0x900013e };
        int idx = EXRand32() & 1;
        sm.GetActorOwner().SetAnimType(anims[idx], 0, 1);
        int left = EXRand32() & 1;
        Actor &actor = sm.GetActorOwner();
        if (left)
            rot = actor.GetRotation().y + randf(1.0f, 2.0f);
        else
            rot = actor.GetRotation().y + randf(-2.0f, -1.0f);
        return 1;
    }
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

// NEAR: 28 bytes, ebp/edi swap for the previous POI and the delta copy store order
signed char STATE_PredatorFollowThePlayer(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &close = sm.GetIntRef(i);
    int &prevPoi = sm.GetIntRef(i);
    int &flagType = sm.GetIntRef(i);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXIT:
        if (poi) POI_VendorUnbind(poi, sm.GetActorOwner().m_RefID);
        return 1;
    case MSG_ENTER:
        flagType = sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f000011 ? 0x2000 : 0x8000;
        return 1;
    case MSG_EXT: {
        int old;
        old = poi;
        if (old) {
            if (EXRand32() % 11 == 0)
                poi = POI_UpdateNearestFlag_Level(Player_GetFeetPos(), flagType, poi, prevPoi, sm.GetActorOwner().GetNavZone(),
                                                  sm.GetActorOwner().CanJump(), 1, -1.0f);
        } else {
            poi = POI_GetNearestFlag_Level(Player_GetFeetPos(), flagType, 0, sm.GetActorOwner().GetNavZone(),
                                           sm.GetActorOwner().CanJump(), 1, -1.0f);
        }
        if (old != poi) {
            if (old) POI_VendorUnbind(old, sm.GetActorOwner().m_RefID);
            prevPoi = old;
            if (poi) POI_VendorBind(poi, sm.GetActorOwner().m_RefID);
        }
        if (!poi) {
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(Player_GetFeetPos(), 5.0f)) {
                sm.GetActorOwner().InvalidateGotoPos();
                sm.GetActorOwner().SetAnimType(0x4e000001, 1, 1);
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(Player_GetFeetPos(), 5.0f);
            return 1;
        }
        EXVector stand = POI_VendorStandPos(poi, 0);
        int poly = POI_VendorStandPoly(poi, 0);
        if (close) {
            EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), stand);
            if (d.LengthXYZSquared() > 9.0f) close = 0;
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(stand, 5.0f)) {
                sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(stand, 5.0f);
            return 1;
        }
        EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), stand);
        if (4.0f > d.LengthXYZSquared()) {
            close = 1;
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(stand, 5.0f)) {
                sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(stand, 5.0f);
            return 1;
        }
        close = 0;
        sm.GetActorOwner().SetAnimType(0x4e000003, 1, 1);
        sm.GetActorOwner().GotoPosPoly(stand, poly);
        return 1;
    }
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

// Twenty identical wrappers: Goto STATE_GotoLocationViaEvadePoints with location hash 0x42000086 + n - 1.
#define EVADE_POINTS_STATE(n, loc) \
    signed char STATE_GotoLocationViaEvadePoints##n(StateMachine &sm) \
    { \
        sm.GetActorOwner().SetWatch(0); \
        switch (sm.m_Msg.m_Type) { \
        case MSG_ENTER: \
            SM_GOTO(STATE_GotoLocationViaEvadePoints, loc, 0, 0, 0); \
            return 1; \
        AU_DEFAULT_MSGS \
        } \
        return 0; \
    }
EVADE_POINTS_STATE(1, 0x42000086)
EVADE_POINTS_STATE(2, 0x42000087)
EVADE_POINTS_STATE(3, 0x42000088)
EVADE_POINTS_STATE(4, 0x42000089)
EVADE_POINTS_STATE(5, 0x4200008a)
EVADE_POINTS_STATE(6, 0x4200008b)
EVADE_POINTS_STATE(7, 0x4200008c)
EVADE_POINTS_STATE(8, 0x4200008d)
EVADE_POINTS_STATE(9, 0x4200008e)
EVADE_POINTS_STATE(10, 0x4200008f)
EVADE_POINTS_STATE(11, 0x42000090)
EVADE_POINTS_STATE(12, 0x42000091)
EVADE_POINTS_STATE(13, 0x42000092)
EVADE_POINTS_STATE(14, 0x42000093)
EVADE_POINTS_STATE(15, 0x42000094)
EVADE_POINTS_STATE(16, 0x42000095)
EVADE_POINTS_STATE(17, 0x42000096)
EVADE_POINTS_STATE(18, 0x42000097)
EVADE_POINTS_STATE(19, 0x42000098)
EVADE_POINTS_STATE(20, 0x42000099)

signed char STATE_CaptivePredRelease(StateMachine &sm)
{
    sm.GetActorOwner().SetWatch(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        sm.GetActorOwner().SetAnimType(0x9000379, 0, 1);
        sm.GetActorOwner().EnableMovement(1);
        sm.GetActorOwner().SetIsTargetable(1);
        sm.GetActorOwner().SetIsHanging(0);
        return 1;
    case MSG_EXT:
        if (sm.GetActorOwner().HasAnimEnded()) {
            SM_GOTO(STATE_PredatorFollowThePlayer, 0, 0, 0, 0);
            return 1;
        }
        return 1;
    }
    return 0;
}

// The open-fire tail that STATE_AttackPredArmed repeats after its stage 0 and stage 1 aiming code:
// throw a grenade if the weapon (or the profile's second weapon) is one, else start firing unless a
// friendly blocks the line of fire.
static __forceinline signed char AttackPredArmed_Fire(StateMachine &sm, int &stage, EXVector &offset,
                                                       WeaponProfile *pWeapon, ActorCarryableGun *pGun)
{
    if (pWeapon->m_AimType == 0x52000007) {
        if (gAIControl.m_GrenadeTime > gAIControl.m_GrenadeNextTime) {
            EXVector p = gAIControl.LastSeenPlayerPos();
            if (!sm.GetActorOwner().AreThereFriendlyActorsNear(p, 5.0f)) {
                gAIControl.GrenadeThrown(pWeapon->m_LockFrames);
                stage = 7;
                return 1;
            }
        }
        stage = 1;
        pGun->DelayWait((EXRandf() + 1.0f) * 0.5f * EXBaseApp::NominalGameFrameRate());
    } else {
        WeaponProfile *pGrenade = WeaponProfile::GetWeaponDataFromEntity(sm.GetActorOwner().GetProfile()->m_Weapon2);
        if (pGrenade && pGrenade->m_AimType == 0x52000007 && gAIControl.m_GrenadeTime > gAIControl.m_GrenadeNextTime) {
            EXVector p = gAIControl.LastSeenPlayerPos();
            if (!sm.GetActorOwner().AreThereFriendlyActorsNear(p, 5.0f)) {
                gAIControl.GrenadeThrown(pGrenade->m_LockFrames);
                stage = 7;
                return 1;
            }
        }
    }
    if (stage != 6) return 1;
    EXVector target = GetDelayedTarget(sm.GetActorOwner(), offset);
    EXVector muzzle = pGun->GetMuzzleWorldPos();
    EXRay ray;
    ray.Origin() = muzzle;
    ray.Direction() = VSub(target, muzzle);
    if (sm.GetActorOwner().FriendlyActorBlockingLineOfSight(ray)) {
        stage = 1;
        pGun->DelayWait((EXRandf() + 1.0f) * 0.5f * EXBaseApp::NominalGameFrameRate());
    }
    if (stage != 6) return 1;
    if (!pGun->StartFiring()) stage = 8;
    return 1;
}

// Melee-range dodge shot: close (3 m), random, and the player roughly ahead (cos 45 deg).
static __forceinline signed char AttackPredArmed_PlayerAhead(StateMachine &sm, const EXMaths::vec_xyzw &d)
{
    EXVector fwd(sin(sm.GetActorOwner().GetRotation().y), 0.0f, cos(sm.GetActorOwner().GetRotation().y), 0.0f);
    EXVector v(d.x, d.y, d.z, d.w);
    NormaliseInline(v);
    return v.DotXYZ(fwd) > 0.707f;
}

// NEAR: logic read in full from the disassembly; the duplicated fire tail is a local __forceinline
// helper here (retail code repeats it after stages 0 and 1), vector copies and spills differ
signed char STATE_AttackPredArmed(StateMachine &sm)
{
    int i = 0;
    EXVector &offset = sm.GetVectorRef(i);
    int &stage = sm.GetIntRef(i);
    int &moveMode = sm.GetIntRef(i);
    int &strikeFlags = sm.GetIntRef(i);
    float &seenTimer = sm.GetFloatRef(i);
    float &hitTimer = sm.GetFloatRef(i);
    float &giveUpTimer = sm.GetFloatRef(i);
    float &chooseTimer = sm.GetFloatRef(i);
    float &meleeTimer = sm.GetFloatRef(i);
    float &swapTimer = sm.GetFloatRef(i);
    float &moveTimer = sm.GetFloatRef(i);
    float &stuckTimer = sm.GetFloatRef(i);
    float &stuckYRot = sm.GetFloatRef(i);
    sm.GetActorOwner().SetHeadTrackMode(0);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    if (sm.GetActorOwner().IsRightArmMissing()) {
        SM_GOTO(STATE_AttackPred, 0, 0, 0, 0);
        return 1;
    }
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    ActorCarryableGun *pGun = sm.GetActorOwner().GetGunPtr(0);
    if (!pGun) {
        sm.Return(0, 0, 0, 0);
        return 1;
    }
    GunCombatAnims anims = Actor_GetAimAnimsSet(sm.GetActorOwner(), pWeapon);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT: {
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        Actor &actor = sm.GetActorOwner();
        float timeout = actor.GetCombatTimeout();
        signed char seen = actor.WasPlayerSeen();
        if (seen) seenTimer = timeout;
        seenTimer -= EXBaseApp::GameFrameRateMul();
        if (seen && !Player_IsCloaked()) gAIControl.SetLastSeenPlayerPos(Player_GetFeetPos());
        if (!(seenTimer < 0.0f)) seen = 1;
        giveUpTimer -= EXBaseApp::GameFrameRateMul();
        if (stage == 0x11 && !(seenTimer < sm.GetActorOwner().GetCombatTimeout() * 0.95f)) {
            stage = 1;
        } else if (seenTimer > 0.0f && sm.GetActorOwner().GetCombatTimeout() * 0.5f > seenTimer) {
            stage = 0x11;
        } else if (0.0f > seenTimer && 0.0f > giveUpTimer) {
            if (AU_ShouldSearchForPredator()) {
                SM_GOTO(STATE_SearchForPredator, 0, 0, 0, 0);
                return 1;
            }
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        chooseTimer -= EXBaseApp::GameFrameRateMul();
        if (!(0.0f < chooseTimer)) {
            int target = Actor_ChooseTarget(sm.GetActorOwner(), 1, seenTimer, 0);
            if (target && target != 1) {
                SM_GOTO(STATE_FightActor, target, 0, 0, 0);
                return 1;
            }
            chooseTimer = (EXRandf() + 3.0f) * EXBaseApp::NominalGameFrameRate();
        }
        if (hitTimer > 0.0f) hitTimer -= EXBaseApp::GameFrameRateMul();
        if (gCurrentMap_GeoHash == 0x1000070 || gCurrentMap_GeoHash == 0x10000a5 || gCurrentMap_GeoHash == 0x10000f3)
            meleeTimer = 9999.0f;
        if (meleeTimer > 0.0f) meleeTimer -= EXBaseApp::GameFrameRateMul();
        EXVector feet = sm.GetActorOwner().GetFeetPos();
        EXVector slotPos;
        g_Formations.GetSlotPos(sm.GetActorOwner().m_RefID, slotPos);
        EXVector origin;
        g_Formations.GetOrigin(origin);
        // Retail clears y and takes the 4-component length (x, z and w).
        EXMaths::vec_xyzw d = VSub(origin, feet);
        d.y = 0.0f;
        float distSq = d.x * d.x + d.y * d.y + d.z * d.z + d.w * d.w;
        float dist = distSq >= 0.0f ? (float)sqrt(distSq) : 0.0f;
        float swapRange = 3.0f;
        if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f000021) swapRange = 10.0f;
        swapTimer -= EXBaseApp::GameFrameRateMul();
        if (0.0f > swapTimer && swapRange > dist && EXRand32() % 0x79 == 0 && sm.GetActorOwner().CarryingMixedWeapons()) {
            SM_GOTO(STATE_SwapWeaponHands, 0, 0, 0, 0);
            return 1;
        }
        {
            int n = SubGroupAnims::GetSubGroupAnims(sm.GetActorOwner().m_SubGroupAnimIndex).m_Anims[11];
            if (n > 0 && sm.GetActorOwner().GetConsecutiveHitCounter() >= n) {
                sm.GetActorOwner().ClearConsecutiveHitCounter();
                sm.GetActorOwner().SetInvinsibleCounter(EXBaseApp::NominalGameFrameRate() * 2);
                stage = 3;
            }
        }
        signed char canLeap = sm.GetActorOwner().CanLeapAbout();
        switch (stage) {
        case 0x11: {
            EXVector p = gAIControl.LastSeenPlayerPos();
            if (sm.GetActorOwner().CanGetToLocation(gAIControl.m_PlayerNavLocLastSeen)) {
                EXMaths::vec_xyzw q = VSub(p, sm.GetActorOwner().GetFeetPos());
                if (q.z * q.z + q.x * q.x > 25.0f) {
                    sm.GetActorOwner().GotoLoc(gAIControl.m_PlayerNavLocLastSeen);
                    sm.GetActorOwner().SetAnimType(anims.m_Anims[9], 1, 1);
                    return 1;
                }
            }
            sm.GetActorOwner().InvalidateGotoPos();
            sm.GetActorOwner().SetAimingPos(sm.GetActorOwner().GetAimPosForTarget(1));
            seenTimer = 0.0f;
            return 1;
        }
        case 0: {
            EXVector approach = GetApproachPosition(sm.GetActorOwner().m_RefID, feet);
            if (sm.GetActorOwner().CanGetToPosSlow(approach)) {
                EXMaths::vec_xyzw a = VSub(feet, approach);
                if (a.y * a.y + a.z * a.z + a.x * a.x > 4.0f) {
                    sm.GetActorOwner().GotoPosSlow(approach);
                } else {
                    sm.GetActorOwner().InvalidateGotoPos();
                    stage = 1;
                }
                EXMaths::vec_xyzw o = VSub(feet, origin);
                if (!(o.y * o.y + o.z * o.z + o.x * o.x > Sq(pWeapon->m_RangeMax - 1.0f)) && seen) {
                    sm.GetActorOwner().InvalidateGotoPos();
                    stage = 1;
                    return 1;
                }
                sm.GetActorOwner().SetAnimType(anims.m_Anims[9], 1, 1);
                return 1;
            }
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (!sm.GetActorOwner().IsFacingYRot(GetDelayedTarget(sm.GetActorOwner(), offset), 22.0f)) {
                sm.GetActorOwner().SetTargetYRot(GetDelayedTarget(sm.GetActorOwner(), offset), 22.0f);
                return 1;
            }
            sm.GetActorOwner().SetAimingPos(sm.GetActorOwner().GetAimPosForTarget(1));
            pGun->UpdateWait();
            if (pGun->CanFire()) stage = 6;
            if (stage != 6) return 1;
            return AttackPredArmed_Fire(sm, stage, offset, pWeapon, pGun);
        }
        case 1: {
            sm.GetActorOwner().TryToSpeakCombatVO(0);
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            pGun->UpdateWait();
            signed char canFire = pGun->CanFire();
            float ang = (float)atan2(-d.x, -d.z);
            if (dist > pWeapon->m_RangeMax + 1.0f) {
                stage = 0;
                if (!sm.GetActorOwner().IsBoss()) SetOffsetVector(offset);
                return 1;
            }
            float slotDist;
            float slotAng;
            g_Formations.GetSlotDistAng(sm.GetActorOwner().m_RefID, slotDist, slotAng);
            float angDiff = XNavNetwork::AngleDifference(slotAng, ang);
            if (moveTimer > 0.0f) moveTimer -= EXBaseApp::GameFrameRateMul();
            switch (moveMode) {
            case 0: {
                if (!(0.0f < meleeTimer) && 3.0f > dist && (EXRand32() & 1) && AttackPredArmed_PlayerAhead(sm, d)) {
                    if (pWeapon->m_Id == 0x4900001d) {
                        pGun->ExtChangeMode();
                        pWeapon = pGun->GetWeaponProfile();
                        pGun->m_Wait = 0.0f;
                        pGun->StartFiring();
                        stage = 0xe;
                    } else {
                        stage = 2;
                    }
                }
                if (moveTimer > 0.0f) break;
                int blocked = 0;
                if (dist > pWeapon->m_RangeMax + 1.0f) {
                    if (sm.GetActorOwner().CanMoveForward(1.0f)) {
                        moveMode = 1;
                        break;
                    }
                    blocked = 1;
                }
                if (pWeapon->m_RangeMin - 1.0f > dist) {
                    if (sm.GetActorOwner().CanMoveBack(1.5f)) {
                        if (canLeap && EXRand32() % 3)
                            moveMode = 0xb;
                        else
                            moveMode = 2;
                        break;
                    }
                    blocked |= 2;
                }
                if (canLeap && fabs(angDiff) > 0.9425f) {
                    if (0.0f > angDiff) {
                        if (sm.GetActorOwner().CanStrafeLeft(1.5f)) moveMode = 9;
                    } else if (sm.GetActorOwner().CanStrafeRight(1.5f)) {
                        moveMode = 10;
                    }
                } else {
                    float absAng = (float)fabs(angDiff);
                    if (absAng > 1.5707964f) {
                        if (0.0f > angDiff) {
                            if (sm.GetActorOwner().CanStrafeLeft(1.5f)) {
                                moveMode = 5;
                                break;
                            }
                            blocked |= 4;
                        } else {
                            if (sm.GetActorOwner().CanStrafeRight(1.5f)) {
                                moveMode = 6;
                                break;
                            }
                            blocked |= 8;
                        }
                    } else if (absAng > 0.9425f) {
                        if (0.0f > angDiff) {
                            if (sm.GetActorOwner().CanStrafeLeft(1.5f)) {
                                moveMode = 7;
                                break;
                            }
                            blocked |= 4;
                        } else {
                            if (sm.GetActorOwner().CanStrafeRight(1.5f)) {
                                moveMode = 8;
                                break;
                            }
                            blocked |= 8;
                        }
                    }
                }
                if ((blocked & 2) && (blocked & 0xc)) {
                    stuckTimer += XApp::GameFrameRateMul();
                    if (stuckTimer > EXBaseApp::NominalGameFrameRate()) {
                        stuckTimer = (EXRandf() * 0.5f + 1.0f) * EXBaseApp::NominalGameFrameRate();
                        if (EXRand32() & 1)
                            stuckYRot = (EXRandf() + 1.0f) * 0.25f + sm.GetActorOwner().GetRotation().y;
                        else
                            stuckYRot = EXRandf() * 0.25f - 0.5f + sm.GetActorOwner().GetRotation().y;
                        stage = 0xd;
                    }
                } else {
                    stuckTimer = 0.0f;
                }
                break;
            }
            case 1:
                if (pWeapon->m_RangeMax - 1.0f > dist || !sm.GetActorOwner().CanMoveForward(1.0f)) moveMode = 0;
                break;
            case 2:
                if (!(0.0f < meleeTimer) && 3.0f > dist && (EXRand32() & 1) && AttackPredArmed_PlayerAhead(sm, d)) {
                    if (pWeapon->m_Id == 0x4900001d) {
                        pGun->ExtChangeMode();
                        pGun->GetWeaponProfile();
                        pGun->m_Wait = 0.0f;
                        pGun->StartFiring();
                        stage = 0xe;
                        return 1;
                    }
                    stage = 2;
                    return 1;
                }
                if (dist > pWeapon->m_RangeMin + 1.0f || !sm.GetActorOwner().CanMoveBack(1.0f)) moveMode = 0;
                break;
            case 5:
            case 7:
            case 9:
                if (dist > pWeapon->m_RangeMax + 1.0f && sm.GetActorOwner().CanMoveForward(1.0f))
                    moveMode = 1;
                else if (0.6283f > fabs(angDiff) || !sm.GetActorOwner().CanStrafeLeft(1.5f))
                    moveMode = 0;
                break;
            case 6:
            case 8:
            case 10:
                if (dist > pWeapon->m_RangeMax + 1.0f && sm.GetActorOwner().CanMoveForward(1.0f))
                    moveMode = 1;
                else if (0.6283f > fabs(angDiff) || !sm.GetActorOwner().CanStrafeRight(1.5f))
                    moveMode = 0;
                break;
            default:
                moveMode = 0;
                break;
            }
            if (moveMode) moveTimer = EXBaseApp::NominalGameFrameRate() * 0.25f;
            switch (moveMode) {
            case 0:
                if (sm.GetActorOwner().IsFacingYRot(GetDelayedTarget(sm.GetActorOwner(), offset), 5.0f)) {
                    sm.GetActorOwner().SetAimingPos(sm.GetActorOwner().GetAimPosForTarget(1));
                    if (canFire && sm.GetActorOwner().WasPlayerSeen()) stage = 6;
                } else {
                    sm.GetActorOwner().SetTargetYRot(GetDelayedTarget(sm.GetActorOwner(), offset), 5.0f);
                }
                break;
            case 1:
                sm.GetActorOwner().SetAnimType(anims.m_Anims[9], 1, 1);
                sm.GetActorOwner().SpinToYRot(GetDelayedTarget(sm.GetActorOwner(), offset));
                break;
            case 2:
                sm.GetActorOwner().SetAnimType(anims.m_Anims[8], 1, 1);
                sm.GetActorOwner().SpinToYRot(GetDelayedTarget(sm.GetActorOwner(), offset));
                if (canFire && sm.GetActorOwner().WasPlayerSeen()) stage = 6;
                break;
            case 11:
                sm.GetActorOwner().SetAnimType(0x9000234, 0, 1);
                stage = 5;
                return 1;
            case 9:
                sm.GetActorOwner().SetAnimType(0x9000232, 0, 1);
                stage = 0xf;
                return 1;
            case 10:
                sm.GetActorOwner().SetAnimType(0x9000233, 0, 1);
                stage = 0xf;
                return 1;
            case 5:
                sm.GetActorOwner().SetAnimType(anims.m_Anims[4], 1, 1);
                sm.GetActorOwner().SpinToYRot(GetDelayedTarget(sm.GetActorOwner(), offset));
                if (canFire && sm.GetActorOwner().WasPlayerSeen()) stage = 6;
                break;
            case 6:
                sm.GetActorOwner().SetAnimType(anims.m_Anims[5], 1, 1);
                sm.GetActorOwner().SpinToYRot(GetDelayedTarget(sm.GetActorOwner(), offset));
                if (canFire && sm.GetActorOwner().WasPlayerSeen()) stage = 6;
                break;
            case 7:
                sm.GetActorOwner().SetAnimType(anims.m_Anims[2], 1, 1);
                sm.GetActorOwner().SpinToYRot(GetDelayedTarget(sm.GetActorOwner(), offset));
                if (canFire && sm.GetActorOwner().WasPlayerSeen()) stage = 6;
                break;
            case 8:
                sm.GetActorOwner().SetAnimType(anims.m_Anims[3], 1, 1);
                sm.GetActorOwner().SpinToYRot(GetDelayedTarget(sm.GetActorOwner(), offset));
                if (canFire && sm.GetActorOwner().WasPlayerSeen()) stage = 6;
                break;
            }
            if (stage != 6) return 1;
            return AttackPredArmed_Fire(sm, stage, offset, pWeapon, pGun);
        }
        case 0xd:
            stuckTimer -= XApp::GameFrameRateMul();
            if (!(0.0f < stuckTimer)) {
                if (sm.GetActorOwner().IsFacingYRot(GetDelayedTarget(sm.GetActorOwner(), offset), 5.0f)) {
                    stage = 1;
                    stuckTimer = 0.0f;
                    return 1;
                }
                sm.GetActorOwner().SetTargetYRot(GetDelayedTarget(sm.GetActorOwner(), offset), 5.0f);
                return 1;
            }
            sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
            sm.GetActorOwner().SpinToYRot(stuckYRot);
            return 1;
        case 8:
            sm.GetActorOwner().SetAnimType(anims.m_Anims[0], 1, 1);
            stage = 5;
            return 1;
        case 7:
            sm.GetActorOwner().SetAnimType(s_aThrowAnims[EXRand32() % 1], 0, 1);
            stage = 5;
            return 1;
        case 5:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 1;
                return 1;
            }
            return 1;
        case 0xf:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().m_InvinsibleCounter = 0.0f;
                stage = 1;
                return 1;
            }
            sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
            return 1;
        case 6: {
            offset *= 0.98f;
            EXVector target = GetDelayedTarget(sm.GetActorOwner(), offset);
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(target, 22.0f)) {
                sm.GetActorOwner().SetAimingPos(sm.GetActorOwner().GetAimPosForTarget(1));
                if (!pGun->UpdateFiring(sm.GetActorOwner(), target, anims.m_Anims[10], 1)) {
                    stage = 1;
                    return 1;
                }
                return 1;
            }
            pGun->PauseFiring();
            pGun->StopLoopingSFX();
            sm.GetActorOwner().SetTargetYRot(target, 22.0f);
            return 1;
        }
        case 0xe: {
            offset *= 0.98f;
            EXVector target = GetDelayedTarget(sm.GetActorOwner(), offset);
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(target, 22.0f)) {
                sm.GetActorOwner().SetAimingPos(sm.GetActorOwner().GetAimPosForTarget(1));
                if (!pGun->UpdateFiring(sm.GetActorOwner(), target, anims.m_Anims[10], 1)) {
                    pGun->ExtChangeMode();
                    pGun->GetWeaponProfile();
                    stage = 1;
                    return 1;
                }
                return 1;
            }
            pGun->PauseFiring();
            pGun->StopLoopingSFX();
            sm.GetActorOwner().SetTargetYRot(target, 22.0f);
            return 1;
        }
        case 2: {
            float sq = sm.GetActorOwner().SqDistToTarget();
            if (pWeapon->m_MeleeRange * pWeapon->m_MeleeRange > sq) {
                stage = 3;
                return 1;
            }
            if (!sm.GetActorOwner().IsFacingYRot(gAIControl.LastSeenPlayerPos(), 5.0f))
                sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
            if (sq > 36.0f) {
                stage = 0;
                return 1;
            }
            sm.GetActorOwner().GotoPosSlow(origin);
            sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
            return 1;
        }
        case 3:
            sm.GetActorOwner().SetAnimType(GetNormalStrikeAnim(sm, pWeapon), 0, 1);
            stage = 5;
            strikeFlags = 3;
            if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f000021) strikeFlags |= 4;
            meleeTimer = (EXRandf() * 2.0f + 5.0f) * EXBaseApp::NominalGameFrameRate();
            pGun->ForceRoundToEnd();
            return 1;
        }
        return 1;
    }
    case MSG_EXIT:
        g_Formations.FreeSlot(sm.GetActorOwner().m_RefID);
        pGun->StopFiring();
        sm.GetActorOwner().SetCanLooseArmAndLive(0);
        if (sm.GetActorOwner().IsLeftArmMissing() || sm.GetActorOwner().IsRightArmMissing())
            sm.GetActorOwner().ChangeHealth(-sm.GetActorOwner().m_Health, 1);
        sm.GetActorOwner().SetHeadTrackMode(0);
        return 1;
    case MSG_ENTER:
        if (sm.GetActorOwner().IsBoss() && pWeapon->m_Id != 0x4900002d &&
            sm.GetActorOwner().GetProfile()->m_SubGroup != 0x4f000010) {
            SM_GOTO(STATE_AttackPredArmedVantage, 0, 0, 0, 0);
            return 1;
        }
        stage = 0;
        giveUpTimer = (EXRandf() * 2.0f + 4.0f) * EXBaseApp::NominalGameFrameRate();
        chooseTimer = (EXRandf() + 3.0f) * EXBaseApp::NominalGameFrameRate();
        swapTimer = (EXRandf() * 0.0f + 1.0f) * EXBaseApp::NominalGameFrameRate();
        if (g_Formations.AllocSlot(sm.GetActorOwner().m_RefID) < 0) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        seenTimer = sm.GetActorOwner().GetCombatTimeout();
        hitTimer = 0.0f;
        meleeTimer = 0.0f;
        moveTimer = 0.0f;
        stuckTimer = 0.0f;
        if (!sm.GetActorOwner().IsBoss()) SetOffsetVector(offset);
        pGun->SetInitialWait();
        sm.GetActorOwner().SetCanLooseArmAndLive(1);
        return 1;
    case MSG_SUSPEND:
        pGun->StopFiring();
        return 1;
    case 0x19:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x19)) ThrowGrenade(sm, gAIControl.LastSeenPlayerPos(), 0, 0);
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_INCOMING_ATTACK:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_INCOMING_ATTACK))
            AU_HandleIncomingAttack(sm, (StructMsg_IncomingAttack *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x25:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x25)) SM_GOSUB(STATE_MonsterJump, 0, 0, 0, 0);
        return 1;
    case 0x23:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x23)) pGun->Reload();
        return 1;
    case 0x2a:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x2a)) pGun->ForceRoundToEnd();
        return 1;
    case 0x1f:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1f)) StrikeCharacter(sm, 1, strikeFlags, pWeapon, 1, 1);
        return 1;
    case 0x20:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x20)) StrikeCharacter(sm, 0, strikeFlags, pWeapon, 1, 1);
        return 1;
    case 0x1e:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e)) {
            StrikeCharacter(sm, 1, strikeFlags, pWeapon, 1, 1);
            StrikeCharacter(sm, 0, strikeFlags, pWeapon, 1, 1);
        }
        return 1;
    }
    return 0;
}

// NEAR: logic read in full from the disassembly; delta-vector copies, slot-distance spills and the
// strafe-threshold block layout differ
signed char STATE_AttackPred(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &moveMode = sm.GetIntRef(i);
    int &combo = sm.GetIntRef(i);
    int &strikeFlags = sm.GetIntRef(i);
    int &strikesLeft = sm.GetIntRef(i);
    int &strikeIndex = sm.GetIntRef(i);
    int &leaping = sm.GetIntRef(i);
    float &seenTimer = sm.GetFloatRef(i);
    float &hitTimer = sm.GetFloatRef(i);
    float &giveUpTimer = sm.GetFloatRef(i);
    float &chooseTimer = sm.GetFloatRef(i);
    float &swapTimer = sm.GetFloatRef(i);
    float &moveTimer = sm.GetFloatRef(i);
    float &hangTimer = sm.GetFloatRef(i);
    float &leapTimer = sm.GetFloatRef(i);
    float &grenadeTimer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    sm.GetActorOwner().SetHeadTrackMode(0);
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    if (sm.GetActorOwner().IsRightArmMissing()) pWeapon = WeaponProfile::GetWeaponData(0x49000034);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        if ((sm.m_Msg.m_Data[0] == (int)STATE_Block && sm.m_Msg.m_Data[1] == 1) ||
            (sm.m_Msg.m_Data[0] == (int)STATE_KnockBack && sm.m_Msg.m_Data[1] == 1)) {
            stage = 3;
            strikesLeft = 4;
            strikeIndex = 0;
            return 1;
        }
        stage = 1;
        return 1;
    case MSG_EXIT:
        g_Formations.FreeSlot(sm.GetActorOwner().m_RefID);
        sm.GetActorOwner().SetCanLooseArmAndLive(0);
        if (!sm.GetActorOwner().IsGoingToStaggerAndDie() &&
            (sm.GetActorOwner().IsLeftArmMissing() || sm.GetActorOwner().IsRightArmMissing()))
            sm.GetActorOwner().ChangeHealth(-sm.GetActorOwner().m_Health, 1);
        return 1;
    case MSG_ENTER:
        if (sm.GetActorOwner().CarryingHiddenWeapon()) {
            SM_GOTO(STATE_PullOutWeapon, 0, 0, 0, 0);
            return 1;
        }
        stage = 0;
        giveUpTimer = (EXRandf() * 2.0f + 4.0f) * EXBaseApp::NominalGameFrameRate();
        chooseTimer = (EXRandf() + 3.0f) * EXBaseApp::NominalGameFrameRate();
        swapTimer = (EXRandf() * 0.0f + 1.0f) * EXBaseApp::NominalGameFrameRate();
        grenadeTimer = (EXRandf() + 1.0f) * 3.0f * EXBaseApp::NominalGameFrameRate();
        moveTimer = 0.0f;
        hangTimer = 0.0f;
        if (sm.GetActorOwner().IsUlysses()) {
            SM_GOTO(STATE_UlysesAttackPred, 0, 0, 0, 0);
            return 1;
        }
        if (sm.GetActorOwner().IsCarryingGun()) {
            SM_GOTO(STATE_AttackPredArmed, 0, 0, 0, 0);
            return 1;
        }
        if (g_Formations.AllocSlot(sm.GetActorOwner().m_RefID) < 0) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        seenTimer = sm.GetActorOwner().GetCombatTimeout();
        hitTimer = 0.0f;
        sm.GetActorOwner().SetCanLooseArmAndLive(1);
        return 1;
    case MSG_EXT: {
        if (sm.GetActorOwner().IsBoss() && sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f000013) {
            EXMaths::vec_xyzw d = VSub(gAIControl.LastSeenPlayerPos(), sm.GetActorOwner().GetFeetPos());
            if (d.z * d.z + d.x * d.x > 100.0f) {
                SM_GOTO(STATE_AttackPredBruno_UseLoc, 0, 0, 0, 0);
                return 1;
            }
        }
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        Actor &actor = sm.GetActorOwner();
        float timeout = actor.GetCombatTimeout();
        signed char seen = actor.WasPlayerSeen();
        if (seen) seenTimer = timeout;
        seenTimer -= EXBaseApp::GameFrameRateMul();
        if (seen && !Player_IsCloaked()) gAIControl.SetLastSeenPlayerPos(Player_GetFeetPos());
        if (!(seenTimer < 0.0f)) seen = 1;
        giveUpTimer -= EXBaseApp::GameFrameRateMul();
        if (0.0f > seenTimer && 0.0f > giveUpTimer) {
            if (AU_ShouldSearchForPredator()) {
                SM_GOTO(STATE_SearchForPredator, 0, 0, 0, 0);
                return 1;
            }
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        chooseTimer -= EXBaseApp::GameFrameRateMul();
        if (!(0.0f < chooseTimer)) {
            int target = Actor_ChooseTarget(sm.GetActorOwner(), 1, seenTimer, 0);
            if (target && target != 1) {
                SM_GOTO(STATE_FightActor, target, 0, 0, 0);
                return 1;
            }
            chooseTimer = (EXRandf() + 3.0f) * EXBaseApp::NominalGameFrameRate();
        }
        if (hitTimer > 0.0f) hitTimer -= EXBaseApp::GameFrameRateMul();
        if (leapTimer > 0.0f) leapTimer -= EXBaseApp::GameFrameRateMul();
        if (grenadeTimer > 0.0f) grenadeTimer -= EXBaseApp::GameFrameRateMul();
        EXVector feet = sm.GetActorOwner().GetFeetPos();
        signed char action = g_Formations.GetAction(sm.GetActorOwner().m_RefID);
        EXVector slotPos;
        g_Formations.GetSlotPos(sm.GetActorOwner().m_RefID, slotPos);
        EXVector origin;
        g_Formations.GetOrigin(origin);
        EXVector me = sm.GetActorOwner().GetFeetPos();
        if (XItemHandler_Player::Instance()->OpenToAttack(me)) action = 1;
        if (sm.GetActorOwner().IsBoss() && sm.GetActorOwner().GetProfileHash() == 0x47000245 &&
            XItemHandler_Player::Instance()->IsHanging()) {
            action = 1;
        } else if (fabs(sm.GetActorOwner().GetFeetPos().y - gAIControl.LastSeenPlayerPos().y) > 1.8f) {
            action = 0;
        }
        float slotDist;
        float slotAng;
        g_Formations.GetSlotDistAng(sm.GetActorOwner().m_RefID, slotDist, slotAng);
        EXMaths::vec_xyzw d = VSub(origin, feet);
        d.y = 0.0f;
        float distSq = d.x * d.x + d.z * d.z;
        signed char canLeap = 0;
        if (!(0.0f < leapTimer)) canLeap = sm.GetActorOwner().CanLeapAbout();
        if (XItemHandler_Player::Instance()->m_Mode == 0x18) {
            hangTimer += XApp::GameFrameRateMul();
            if (hangTimer > 3.0f) {
                sm.GetActorOwner().SetAnimType(0x4e000004, 0, 1);
                return 1;
            }
        } else {
            hangTimer = 0.0f;
        }
        swapTimer -= EXBaseApp::GameFrameRateMul();
        float swapRange = 6.0f;
        if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f000021) swapRange = 10.0f;
        if (0.0f > swapTimer && EXRand32() % 0x79 == 0 && distSq > swapRange * swapRange &&
            sm.GetActorOwner().CarryingMixedWeapons()) {
            SM_GOTO(STATE_SwapWeaponHands, 0, 0, 0, 0);
            return 1;
        }
        {
            int n = SubGroupAnims::GetSubGroupAnims(sm.GetActorOwner().m_SubGroupAnimIndex).m_Anims[11];
            if (n > 0 && sm.GetActorOwner().GetConsecutiveHitCounter() >= n) {
                sm.GetActorOwner().ClearConsecutiveHitCounter();
                sm.GetActorOwner().SetInvinsibleCounter(EXBaseApp::NominalGameFrameRate() * 2);
                stage = 0x12;
            }
        }
        switch (stage) {
        case 0: {
            EXVector approach = GetApproachPosition(sm.GetActorOwner().m_RefID, feet);
            if (!sm.GetActorOwner().CanGetToPosSlow(approach)) {
                sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
                if (sm.GetActorOwner().IsFacingYRot(gAIControl.LastSeenPlayerPos(), 5.0f)) {
                    sm.GetActorOwner().SetAnimType(0x4e000004, 0, 1);
                    return 1;
                }
                sm.GetActorOwner().SetTargetYRot(gAIControl.LastSeenPlayerPos(), 5.0f);
                return 1;
            }
            sm.GetActorOwner().GotoPosSlow(approach);
            EXMaths::vec_xyzw f = VSub(feet, origin);
            if (f.x * f.x + f.z * f.z > Sq(slotDist + 1.0f)) {
                if (0.0f > grenadeTimer) {
                    WeaponProfile *pGrenade = WeaponProfile::GetWeaponDataFromEntity(sm.GetActorOwner().GetProfile()->m_Weapon2);
                    if (pGrenade && pGrenade->m_AimType == 0x52000007) {
                        EXMaths::vec_xyzw g = VSub(feet, origin);
                        if (pGrenade->m_RangeMax * pGrenade->m_RangeMax >= g.z * g.z + g.x * g.x &&
                            gAIControl.m_GrenadeTime > gAIControl.m_GrenadeNextTime) {
                            EXVector target = gAIControl.LastSeenPlayerPos();
                            if (!sm.GetActorOwner().AreThereFriendlyActorsNear(target, 5.0f)) {
                                grenadeTimer = (EXRandf() + 1.0f) * 3.0f * EXBaseApp::NominalGameFrameRate();
                                gAIControl.GrenadeThrown(pGrenade->m_LockFrames);
                                stage = 7;
                            }
                        }
                    }
                }
                sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
                return 1;
            }
            sm.GetActorOwner().InvalidateGotoPos();
            stage = 1;
            return 1;
        }
        case 7:
            sm.GetActorOwner().SetAnimType(s_aThrowAnims[EXRand32() % 1], 0, 1);
            stage = 5;
            return 1;
        case 1: {
            sm.GetActorOwner().TryToSpeakCombatVO(0);
            if (distSq > Sq(slotDist + 3.0f)) {
                stage = 0;
                return 1;
            }
            float angDiff = XNavNetwork::AngleDifference(slotAng, (float)atan2(-d.x, -d.z));
            float absAng = (float)fabs(angDiff);
            if (moveTimer > 0.0f) moveTimer -= EXBaseApp::GameFrameRateMul();
            switch (moveMode) {
            case 0:
                if (moveTimer > 0.0f) break;
                if (distSq > Sq(slotDist + 4.5f) && sm.GetActorOwner().CanMoveForward(1.0f)) {
                    moveMode = 1;
                } else if (distSq > Sq(slotDist + 2.3f) && sm.GetActorOwner().CanMoveForward(1.0f)) {
                    moveMode = 3;
                } else if (slotDist * slotDist > distSq && sm.GetActorOwner().CanMoveBack(1.5f)) {
                    if (canLeap && 16.0f > distSq)
                        moveMode = EXRand32() % 3 ? 4 : 11;
                    else
                        moveMode = 4;
                } else if (canLeap && absAng > 0.3f) {
                    if (0.0f > angDiff) {
                        if (sm.GetActorOwner().CanStrafeLeft(2.5f)) moveMode = 9;
                    } else if (sm.GetActorOwner().CanStrafeRight(2.5f)) {
                        moveMode = 10;
                    }
                } else if (absAng > (distSq > 16.0f ? 0.6f : 2.0f)) {
                    if (0.0f > angDiff) {
                        if (sm.GetActorOwner().CanStrafeLeft(1.5f)) moveMode = 5;
                    } else if (sm.GetActorOwner().CanStrafeRight(1.5f)) {
                        moveMode = 6;
                    }
                } else if (absAng > 0.3f) {
                    if (0.0f > angDiff) {
                        if (sm.GetActorOwner().CanStrafeLeft(1.5f)) moveMode = 7;
                    } else if (sm.GetActorOwner().CanStrafeRight(1.5f)) {
                        moveMode = 8;
                    }
                }
                break;
            case 1:
            case 3:
                if (Sq(slotDist + 0.5f) > distSq || !sm.GetActorOwner().CanMoveForward(1.0f)) moveMode = 0;
                break;
            case 2:
            case 4:
                if (distSq > Sq(slotDist + 1.0f) || !sm.GetActorOwner().CanMoveBack(1.0f)) moveMode = 0;
                break;
            case 5:
            case 7:
            case 9:
                if (moveMode == 9 && !canLeap)
                    moveMode = 0;
                else if (distSq > Sq(slotDist + 2.0f) && sm.GetActorOwner().CanMoveForward(1.0f))
                    moveMode = 1;
                else if (0.2f > absAng || !sm.GetActorOwner().CanStrafeLeft(1.0f))
                    moveMode = 0;
                break;
            case 6:
            case 8:
            case 10:
                if (moveMode == 10 && !canLeap)
                    moveMode = 0;
                else if (distSq > Sq(slotDist + 2.0f) && sm.GetActorOwner().CanMoveForward(1.0f))
                    moveMode = 1;
                else if (0.2f > absAng || !sm.GetActorOwner().CanStrafeRight(1.0f))
                    moveMode = 0;
                break;
            default:
                moveMode = 0;
                break;
            }
            if (moveMode) moveTimer = EXBaseApp::NominalGameFrameRate() * 0.25f;
            if (action && seen) {
                switch (moveMode) {
                case 0:
                    if (!sm.GetActorOwner().IsFacingYRot(gAIControl.LastSeenPlayerPos(), 5.0f)) break;
                case 2:
                case 4:
                case 5:
                case 6:
                case 7:
                case 8:
                    if (sm.GetActorOwner().CanGetToLocation(gAIControl.m_PlayerNavLocLastSeen) &&
                        !(gAIControl.m_DistOffNaviMeshLastSeen > 1.0f)) {
                        stage = 2;
                        strikesLeft = pWeapon->m_NumStrikes;
                        strikeIndex = 0;
                        combo = 1;
                        if (sm.GetActorOwner().IsBoss()) {
                            strikesLeft = 1;
                            strikeIndex = 3;
                        }
                        return 1;
                    }
                    break;
                }
            }
            switch (moveMode) {
            case 0:
                sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
                if (sm.GetActorOwner().IsFacingYRot(gAIControl.LastSeenPlayerPos(), 5.0f)) {
                    sm.GetActorOwner().SetAnimType(0x4e000004, 0, 1);
                    return 1;
                }
                sm.GetActorOwner().SetTargetYRot(gAIControl.LastSeenPlayerPos(), 5.0f);
                return 1;
            case 11:
                sm.GetActorOwner().SetAnimType(0x9000234, 0, 1);
                stage = 5;
                return 1;
            case 9:
                sm.GetActorOwner().SetAnimType(0x9000232, 0, 1);
                stage = 0xf;
                leapTimer = (EXRandf() * 4.0f + 3.0f) * EXBaseApp::NominalGameFrameRate();
                return 1;
            case 10:
                sm.GetActorOwner().SetAnimType(0x9000233, 0, 1);
                stage = 0xf;
                leapTimer = (EXRandf() * 4.0f + 3.0f) * EXBaseApp::NominalGameFrameRate();
                return 1;
            case 1:
                sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
                sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
                return 1;
            case 2:
                sm.GetActorOwner().SetAnimType(0x4e00000a, 0, 1);
                sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
                return 1;
            case 3:
                sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
                sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
                return 1;
            case 4:
                sm.GetActorOwner().SetAnimType(0x4e000009, 0, 1);
                sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
                return 1;
            case 5:
                sm.GetActorOwner().SetAnimType(0x4e00000b, 0, 1);
                sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
                return 1;
            case 6:
                sm.GetActorOwner().SetAnimType(0x4e00000c, 0, 1);
                sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
                return 1;
            case 7:
                sm.GetActorOwner().SetAnimType(0x4e000007, 0, 1);
                sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
                return 1;
            case 8:
                sm.GetActorOwner().SetAnimType(0x4e000008, 0, 1);
                sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
                return 1;
            }
            return 1;
        }
        case 0xf:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 1;
                leaping = 0;
                sm.GetActorOwner().SetDontDoHitReaction(0);
                return 1;
            }
            sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
            leaping = 1;
            sm.GetActorOwner().SetDontDoHitReaction(1);
            return 1;
        case 5:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 1;
                return 1;
            }
            return 1;
        case 2: {
            sm.GetActorOwner().GotoLoc(gAIControl.m_PlayerNavLocLastSeen);
            sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
            EXMaths::vec_xyzw p = VSub(sm.GetActorOwner().GetFeetPos(), gAIControl.LastSeenPlayerPos());
            float reach = pWeapon->m_MeleeRange;
            float dist = p.z * p.z + p.x * p.x;
            if (sm.GetActorOwner().IsBoss()) reach *= 1.5f;
            if (reach * reach > dist) {
                sm.GetActorOwner().InvalidateGotoPos();
                stage = 3;
            }
            if (dist > 36.0f || gAIControl.m_DistOffNaviMeshLastSeen > 1.1f) {
                sm.GetActorOwner().InvalidateGotoPos();
                stage = 0;
                return 1;
            }
            return 1;
        }
        case 3:
            sm.GetActorOwner().SetAnimType(GetStrikeAnim(sm, pWeapon, strikeIndex, &strikeFlags), 0, 1);
            stage = 4;
            return 1;
        case 0x12:
            sm.GetActorOwner().SetAnimType(GetStrikeAnim(sm, pWeapon, strikeIndex, &strikeFlags), 0, 1);
            stage = 0x13;
            return 1;
        case 0x13:
            sm.GetActorOwner().SpinToYRot(Player_GetHeadPos());
        case 4:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().m_InvinsibleCounter = 0.0f;
                if ((strikeFlags & 2) && (strikeFlags & 1)) {
                    strikesLeft = 0;
                    strikeIndex = 0;
                    combo = 0;
                }
                strikeFlags = 0;
                strikesLeft--;
                strikeIndex++;
                if (strikesLeft > 0) {
                    stage = 2;
                    return 1;
                }
                if (!combo || EXRand32() % 3 == 0) {
                    stage = 1;
                    return 1;
                }
                stage = 2;
                strikeIndex = 0;
                strikesLeft = pWeapon->m_NumStrikes;
                if (sm.GetActorOwner().IsBoss()) {
                    strikesLeft = 1;
                    strikeIndex = 3;
                }
                return 1;
            }
            sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
            if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f000013) sm.GetActorOwner().m_InvinsibleCounter = 5.0f;
            return 1;
        }
        return 1;
    }
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR)) {
            if (!sm.GetActorOwner().IsBoss()) {
                strikesLeft = 0;
                strikeIndex = 0;
            }
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        }
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR)) {
            sm.GetActorOwner().StopCombatCircleAdjustment((EXRandf() * 2.0f + 4.0f) * EXBaseApp::NominalGameFrameRate());
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
            combo = 0;
            if (!sm.GetActorOwner().IsBoss()) {
                strikesLeft = 0;
                strikeIndex = 0;
            }
        }
        return 1;
    case 0x19:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x19)) ThrowGrenade(sm, gAIControl.LastSeenPlayerPos(), 0, 0);
        return 1;
    case 0x1f:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1f)) StrikeCharacter(sm, 1, strikeFlags, pWeapon, 0, 1);
        return 1;
    case 0x20:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x20)) StrikeCharacter(sm, 0, strikeFlags, pWeapon, 0, 1);
        return 1;
    case 0x1e:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e)) {
            StrikeCharacter(sm, 1, strikeFlags, pWeapon, 0, 1);
            StrikeCharacter(sm, 0, strikeFlags, pWeapon, 0, 1);
        }
        return 1;
    case 0x25:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x25)) SM_GOSUB(STATE_MonsterJump, 0, 0, 0, 0);
        return 1;
    case MSG_INCOMING_ATTACK:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_INCOMING_ATTACK)) {
            if (stage == 4 && sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f000013) return 1;
            AU_HandleIncomingAttack(sm, (StructMsg_IncomingAttack *)sm.m_Msg.m_Data[0]);
        }
        return 1;
    }
    return 0;
}

// NEAR: written for behaviour; retail keeps extra copies of the hit direction and head-pos deltas
signed char STATE_KnockBack(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &canExplode = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_EXT:
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        switch (stage) {
        case 0:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().SetBlockCounter(120);
                sm.GetActorOwner().SetDontDoHitReaction(0);
                if (sm.GetActorOwner().GetProfile()->m_SubGroup != 0x4f000021) {
                    sm.Return((int)STATE_KnockBack, 1, 0, 0);
                    return 1;
                }
                sm.GetActorOwner().SetAnimType(0x900021b, 0, 1);
                canExplode = 1;
                stage = 1;
                return 1;
            }
            return 1;
        case 1:
            sm.GetActorOwner().SetBlockCounter(120);
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().SetDontDoHitReaction(0);
                int anims[2] = { 0x900037c, 0x9000379 };
                int anim = anims[EXRand32() & 1];
                sm.GetActorOwner().SetAnimType(anim, 0, 1);
                stage = 2;
                return 1;
            }
            return 1;
        case 2:
            sm.GetActorOwner().SetBlockCounter(120);
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().SetDontDoHitReaction(0);
                SM_GOTO(STATE_RunAtAndStrike, 0, 0, 0, 0);
                return 1;
            }
            return 1;
        }
        return 1;
    case MSG_ENTER: {
        sm.GetActorOwner().ClearConsecutiveHitCounter();
        sm.GetActorOwner().SetDontDoHitReaction(1);
        EXVector dir = -*(EXVector *)sm.m_Msg.m_Data[0];
        sm.GetActorOwner().SetRotY(qatan2f(dir.x, dir.z));
        sm.GetActorOwner().SetRotY(qatan2f(VSub(gAIControl.LastSeenPlayerPos(), sm.GetActorOwner().GetHeadPos()).x,
                                           VSub(gAIControl.LastSeenPlayerPos(), sm.GetActorOwner().GetHeadPos()).z));
        if (sm.GetActorOwner().IsBoss())
            sm.GetActorOwner().SetAnimType(0x9000225, 0, 1);
        else
            sm.GetActorOwner().SetAnimType(0x90002e4, 0, 1);
        return 1;
    }
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x20:
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x20)) return 1;
        goto hit;
    case 0x1f:
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x1f)) return 1;
        goto hit;
    case 0x1e:
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e)) return 1;
    hit:
        if (canExplode) {
            EXVector feet = sm.GetActorOwner().GetFeetPos();
            XItemHandler_ScriptBase::Explosion_Create(feet, 0x40002d7, 5, 0x1e, 0, 0, 1, sm.GetActorOwner().m_RefID, 0);
            canExplode = 0;
        }
        return 1;
    }
    return 0;
}

signed char STATE_MonsterSquad(StateMachine &sm)
{
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        switch (sm.GetActorOwner().GetProfile()->m_Weapon) {
        case 0x20000d8:
            SM_GOTO(STATE_MonsterSquad_Sniper, 0, 0, 0, 0);
            return 1;
        case 0x20000de:
            SM_GOTO(STATE_MonsterSquad_Plasma, 0, 0, 0, 0);
            return 1;
        case 0x20000df:
            SM_GOTO(STATE_MonsterSquad_Missiles, 0, 0, 0, 0);
            return 1;
        case 0x20000e0:
            SM_GOTO(STATE_MonsterSquad_Freeze, 0, 0, 0, 0);
            return 1;
        }
        sm.Return(0, 0, 0, 0);
        return 1;
    }
    return 0;
}

signed char STATE_MeleeTest(StateMachine &sm)
{
    static int s_aWeapons[18] = { 0x020000cf, 0x02000231, 0x020000e7, 0x020000e8, 0x020000b8, 0x020000cb,
                                  0x020000ba, 0x020000cf, 0x020000bf, 0x02000159, 0x020000cd, 0x020000cc,
                                  0x020000d1, 0x020000d0, 0,          0x020000d2, 0x02000120, 0x02000189 };
    static int s_nWeapons = 18;
    static int s_iWeapon;
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(1);
    switch (sm.m_Msg.m_Type) {
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR)) {
            SM_GOTO(STATE_AttackPred, 0, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_EXT:
        sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        return 1;
    case MSG_ENTER: {
        int i = 0;
        int &weapon = sm.GetIntRef(i);
        weapon = s_aWeapons[s_iWeapon];
        if (weapon) sm.GetActorOwner().SetCarry(ActorCarryable::Create(0x100003c, weapon)->GetRefID(), 0);
        if (++s_iWeapon >= s_nWeapons) s_iWeapon = 0;
        return 1;
    }
    }
    return 0;
}

signed char STATE_ChangeSkinThenAttackPred(StateMachine &sm)
{
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        SM_GOTO(STATE_AttackPred, 0, 0, 0, 0);
        return 1;
    case MSG_ENTER:
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        SM_GOSUB(STATE_FS_ChangeSkin, 0, 0, 0, 0);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// NEAR: register rotation only (eax/ecx in the LoudNoise call, ecx/edx/eax in the attack cases)
signed char STATE_PanicThenAttackPred(StateMachine &sm)
{
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (sm.GetActorOwner().HasAnimEnded()) {
            SM_GOTO(STATE_AttackPred, 0, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_ENTER: {
        int anims[2] = { 0x900012c, 0x90004a1 };
        int anim = anims[EXRand32() & 1];
        sm.GetActorOwner().SetAnimType(anim, 0, 1);
        sm.GetActorOwner().Speak(0x1af000e1, 0);
        Actor &actor = sm.GetActorOwner();
        SendMsg_LoudNoise(actor.GetFeetPos(), 10.0f, &actor, 1, 0);
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        return 1;
    }
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

// ---------------------------------------------------------------------------------------------
// Large combat states (written for behaviour; see the NEAR/GUESS tags).

// NEAR: logic read in full; retail builds the fire target through extra EXVector copies (x87/SSE mix)
signed char STATE_WeaponTest(StateMachine &sm)
{
    static int s_aWeapons[28] = { 0x020000e5, 0x02000188, 0x020000e9, 0x020000e2, 0x020000e3, 0x020000e6, 0x020000e4,
                                  0x020000d3, 0x020000d6, 0x020000db, 0x020000be, 0x02000237, 0x02000236, 0x020000d4,
                                  0x020000da, 0x020000dd, 0x020000e0, 0x020000df, 0x020000de, 0x020000d8, 0x020000e1,
                                  0x020000d5, 0x020000d9, 0x02000185, 0x02000187, 0x0200018e, 0x02000184, 0x0200018c };
    static int s_nWeapons = 28;
    static int s_iWeapon;
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &weapon = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(1);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT: {
        GunCombatAnims anims = Actor_GetAimAnimsSet(sm.GetActorOwner(), sm.GetActorOwner().GetWeaponProfile(0));
        if (timer > 0.0f) timer -= XApp::GameFrameRateMul();
        ActorCarryableGun *pGun = sm.GetActorOwner().GetGunPtr(0);
        if (!pGun) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        switch (stage) {
        case 0:
            if (timer > 0.0f) {
                stage = 3;
                return 1;
            }
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            return 1;
        case 3:
            if (0.0f >= timer) {
                stage = 0;
                return 1;
            }
            pGun->UpdateWait();
            if (pGun->CanFire()) {
                if (pGun->StartFiring()) {
                    stage = 2;
                    return 1;
                }
                sm.GetActorOwner().SetAnimType(anims.m_Anims[0], 1, 1);
                stage = 1;
                return 1;
            }
            sm.GetActorOwner().SetAnimType(anims.m_Anims[1], 1, 1);
            return 1;
        case 2: {
            EXVector off((float)sin(sm.GetActorOwner().GetRotation().y) * 100.0f, 0.0f,
                         (float)cos(sm.GetActorOwner().GetRotation().y) * 100.0f, 0.0f);
            EXVector target = VAdd(sm.GetActorOwner().GetFeetPos(), off);
            target.y += 1.0f;
            if (pGun->UpdateFiring(sm.GetActorOwner(), target, anims.m_Anims[10], 1)) return 1;
            sm.GetActorOwner().SetAnimType(anims.m_Anims[1], 1, 1);
            stage = 3;
            return 1;
        }
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 3;
                return 1;
            }
            return 1;
        }
        return 1;
    }
    case MSG_ENTER:
        timer = 0.0f;
        weapon = s_aWeapons[s_iWeapon];
        sm.GetActorOwner().SetCarry(ActorCarryable::Create(0x100003c, weapon)->GetRefID(), 0);
        if (++s_iWeapon >= s_nWeapons) s_iWeapon = 0;
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            timer = EXBaseApp::NominalGameFrameRate() * 60.0f;
        return 1;
    case 0x23:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x23)) {
            ActorCarryableGun *pGun = sm.GetActorOwner().GetGunPtr(0);
            if (pGun) pGun->Reload();
        }
        return 1;
    }
    return 0;
}

// NEAR: logic read in full; retail keeps extra EXVector copies (0x48 frame) for the aim base and offset sums
signed char STATE_WheresDaBeef(StateMachine &sm)
{
    int i = 0;
    EXVector &base = sm.GetVectorRef(i);
    EXVector &off = sm.GetVectorRef(i);
    float &timer = sm.GetFloatRef(i);
    int &holding = sm.GetIntRef(i);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT: {
        if (holding) {
            timer -= XApp::GameFrameRateMul();
            if (0.0f >= timer) {
                off.x = EXRandf() * 14.0f - 7.0f;
                off.y = 0.0f;
                off.z = EXRandf() * 14.0f - 7.0f;
                off.w = 0.0f;
                timer = (EXRandf() + 0.5f) * EXBaseApp::NominalGameFrameRate();
            }
            sm.GetActorOwner().SetAimingPos(VAdd(off, base));
            return 1;
        }
        int anims[6];
        anims[0] = 0x9000217;
        anims[1] = 0x9000108;
        anims[2] = 0x90002d2;
        anims[3] = 0x90002d4;
        int n;
        if (sm.GetActorOwner().GetCarryPtr(0)) {
            anims[4] = 0x900014a;
            n = 5;
        } else {
            anims[4] = 0x900012b;
            anims[5] = 0x90002d3;
            n = 6;
        }
        if (sm.GetActorOwner().HasAnimEnded()) sm.GetActorOwner().SetAnimType(anims[n ? EXRand32() % n : 0], 0, 1);
        return 1;
    }
    case MSG_ENTER: {
        ActorCarryable *pCarry = sm.GetActorOwner().GetCarryPtr(0);
        if (pCarry) {
            holding = pCarry->m_bGun;
            if (holding) {
                EXVector dir((float)sin(sm.GetActorOwner().GetRotation().y) * 10.0f, 0.0f,
                             (float)cos(sm.GetActorOwner().GetRotation().y) * 10.0f, 0.0f);
                base = VAdd(sm.GetActorOwner().GetFeetPos(), dir);
                base.y += 1.4f;
                timer = 0.0f;
            }
        }
        return 1;
    }
    case MSG_EXIT:
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// NEAR: logic read in full; retail keeps extra EXVector copies (0xb8 frame) around the aim/ray sums
signed char STATE_Ulysses_Follow_Path_And_Kick_Ass(StateMachine &sm)
{
    int i = 0;
    EXVector &gotoPos = sm.GetVectorRef(i);
    EXVector &aim = sm.GetVectorRef(i);
    float &aimTimer = sm.GetFloatRef(i);
    int &stage = sm.GetIntRef(i);
    int &node = sm.GetIntRef(i);
    int &path = sm.GetIntRef(i);
    int &dir = sm.GetIntRef(i);
    int &fireStage = sm.GetIntRef(i);
    int &target = sm.GetIntRef(i);
    float &waitTimer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    sm.GetActorOwner().SetHeadTrackMode(0);
    ActorCarryableGun *pGun = sm.GetActorOwner().GetGunPtr(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT: {
        aimTimer -= XApp::GameFrameRateMul();
        if (0.0f >= aimTimer) {
            aimTimer = (EXRandf() * 2.0f + 1.0f) * EXBaseApp::NominalGameFrameRate();
            target = Actor_ChooseTarget(sm.GetActorOwner(), 0, 0.0f, 0);
            if (!target) {
                float rx = EXRandf() * 150.0f - 75.0f;
                float rz = EXRandf() * 150.0f - 75.0f;
                EXVector fwd((float)sin(sm.GetActorOwner().GetRotation().y) * 100.0f, 0.0f,
                             (float)cos(sm.GetActorOwner().GetRotation().y) * 100.0f, 0.0f);
                EXVector p = VAdd(sm.GetActorOwner().GetFeetPos(), fwd);
                p.x += rx;
                p.z += rz;
                aim = p;
            }
        }
        if (target) aim = sm.GetActorOwner().GetAimPosForTarget(target);
        sm.GetActorOwner().SetHeadTrackMode(2);
        sm.GetActorOwner().SetAimingPos(aim);
        EXGeoPath *pPath = AU_GetPathFromIndex(path);
        sm.GetActorOwner().TryToSpeakCombatVO(Actor::GetPtrFromID(target));
        switch (fireStage) {
        case 4: {
            pGun->UpdateWait();
            EXVector head = sm.GetActorOwner().GetHeadDirection();
            EXVector d = VSub(aim, sm.GetActorOwner().GetFeetPos());
            NormaliseInline(d);
            if (head.z * d.z + head.y * d.y + head.x * d.x > 0.9f && pGun->CanFire()) {
                EXVector from = aim;
                from.y += 1.5f;
                EXVector hp = sm.GetActorOwner().GetHeadPos();
                EXRay ray;
                ray.Direction() = VSub(from, hp);
                ray.Origin() = hp;
                if (!sm.GetActorOwner().FriendlyActorBlockingLineOfSight(ray) && pGun->StartFiring()) fireStage = 5;
            }
            break;
        }
        case 5:
            if (!pGun->UpdateFiring(sm.GetActorOwner(), aim, 0, 0)) fireStage = 4;
            break;
        }
        switch (stage) {
        case 0: {
            const EXVector3 &np = pPath->Node(node)->Position();
            sm.GetActorOwner().GotoPosSlow(AU_SnapFloorPos(EXVector(np.x, np.y, np.z, 0.0f), 0, 1));
            const EXVector3 &np2 = pPath->Node(node)->Position();
            gotoPos = AU_SnapFloorPos(EXVector(np2.x, np2.y, np2.z, 0.0f), 0, 1);
            sm.GetActorOwner().SetAnimType(0x900039f, 0, 1);
            stage = 1;
            return 1;
        }
        case 1:
            sm.GetActorOwner().SetAnimType(0x900039f, 0, 1);
            if (1.0f > sm.GetActorOwner().SqDistToTarget()) {
                waitTimer = 0.0f;
                if (pPath->Node(node)->Flags() & 0x10000) {
                    waitTimer = 300.0f;
                    stage = 2;
                }
                if (pPath->Node(node)->Flags() & 0x20000) {
                    waitTimer += 600.0f;
                    stage = 2;
                }
                if (waitTimer == 0.0f) stage = 3;
                node += dir;
                return 1;
            }
            return 1;
        case 2:
            sm.GetActorOwner().SetAnimType(0x90003a1, 0, 1);
            waitTimer -= XApp::GameFrameRateMul();
            if (0.0f >= waitTimer) {
                stage = 3;
                return 1;
            }
            return 1;
        case 3:
            if (node >= pPath->NumNodes()) {
                if (pPath->Flags() & 0x4000) {
                    node = 0;
                    stage = 0;
                    return 1;
                }
                dir = -1;
                node -= 2;
                stage = 0;
                return 1;
            }
            if (node < 0) {
                dir = 1;
                node += 2;
            }
            stage = 0;
            return 1;
        }
        return 1;
    }
    case MSG_ENTER:
        stage = 0;
        node = 0;
        path = AU_FindClosestPathIndex(sm.GetActorOwner().GetFeetPos(), node, 0x2000);
        dir = 1;
        fireStage = 4;
        aimTimer = 0.0f;
        if (path == -1) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
        return 1;
    case MSG_SUSPEND:
        if (pGun) pGun->StopFiring();
        return 1;
    case 0x2a:
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x2a)) return 1;
    case MSG_EXIT:
        if (pGun) pGun->ForceRoundToEnd();
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

// NEAR: logic read in full (same path walk as the _And_Kick_Ass variant, no shooting)
signed char STATE_Ulysses_Follow_Path(StateMachine &sm)
{
    int i = 0;
    EXVector &gotoPos = sm.GetVectorRef(i);
    int &stage = sm.GetIntRef(i);
    int &node = sm.GetIntRef(i);
    int &path = sm.GetIntRef(i);
    int &dir = sm.GetIntRef(i);
    float &waitTimer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR) && !sm.GetActorOwner().Holographic())
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_EXT: {
        if (sm.GetActorOwner().WasPlayerSeen()) Objective_NotifyAlarm(&sm.GetActorOwner());
        EXGeoPath *pPath = AU_GetPathFromIndex(path);
        switch (stage) {
        case 0: {
            const EXVector3 &np = pPath->Node(node)->Position();
            Actor &a = sm.GetActorOwner();
            a.GotoPosSlow(AU_SnapFloorPos(EXVector(np.x, np.y, np.z, 0.0f), 0, 1));
            const EXVector3 &np2 = pPath->Node(node)->Position();
            gotoPos = AU_SnapFloorPos(EXVector(np2.x, np2.y, np2.z, 0.0f), 0, 1);
            sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
            stage = 1;
            return 1;
        }
        case 1:
            sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
            if (1.0f > sm.GetActorOwner().SqDistToTarget()) {
                waitTimer = 0.0f;
                if (pPath->Node(node)->Flags() & 0x10000) {
                    waitTimer = 300.0f;
                    stage = 2;
                }
                if (pPath->Node(node)->Flags() & 0x20000) {
                    waitTimer += 600.0f;
                    stage = 2;
                }
                if (waitTimer == 0.0f) stage = 3;
                node += dir;
                return 1;
            }
            return 1;
        case 2:
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            waitTimer -= XApp::GameFrameRateMul();
            if (0.0f >= waitTimer) {
                stage = 3;
                return 1;
            }
            return 1;
        case 3:
            if (node >= pPath->NumNodes()) {
                if (pPath->Flags() & 0x4000) {
                    node = 0;
                    stage = 0;
                    return 1;
                }
                dir = -1;
                node -= 2;
                stage = 0;
                return 1;
            }
            if (node < 0) {
                dir = 1;
                node += 2;
            }
            stage = 0;
            return 1;
        }
        return 1;
    }
    case MSG_ENTER:
        stage = 0;
        node = 0;
        path = AU_FindClosestPathIndex(sm.GetActorOwner().GetFeetPos(), node, 0x2000);
        dir = 1;
        if (path == -1) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
        return 1;
    AU_MSG_MUGGING AU_MSG_SOMEONE_UNDER_ATTACK AU_MSG_ATTACKED_BY_ACTOR AU_MSG_LOUD_NOISE AU_MSG_FIGHT_ME
    AU_MSG_DRIVE_BY AU_MSG_INCOMING_ATTACK AU_MSG_ATTRACT AU_MSG_SOMEONE_SEEN_PREDATOR
    }
    return 0;
}

// NEAR: logic read in full; retail routes the aim/ray vectors through extra stack copies
signed char STATE_Ulysses_Stand_And_Shoot(StateMachine &sm)
{
    int i = 0;
    EXVector &aim = sm.GetVectorRef(i);
    float &aimTimer = sm.GetFloatRef(i);
    int &fireStage = sm.GetIntRef(i);
    int &target = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    sm.GetActorOwner().SetHeadTrackMode(0);
    ActorCarryableGun *pGun = sm.GetActorOwner().GetGunPtr(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        aimTimer -= XApp::GameFrameRateMul();
        if (0.0f >= aimTimer) {
            aimTimer = (EXRandf() * 2.0f + 1.0f) * EXBaseApp::NominalGameFrameRate();
            target = Actor_ChooseTarget(sm.GetActorOwner(), 0, 0.0f, 0);
            if (!target) {
                float rx = EXRandf() * 150.0f - 75.0f;
                float rz = EXRandf() * 150.0f - 75.0f;
                const EXVector &feet = sm.GetActorOwner().GetFeetPos();
                EXVector p(rx + feet.x, feet.y, rz + feet.z, feet.w);
                aim = p;
            }
        }
        if (target) aim = sm.GetActorOwner().GetAimPosForTarget(target);
        sm.GetActorOwner().SetHeadTrackMode(2);
        sm.GetActorOwner().SetAimingPos(aim);
        sm.GetActorOwner().SetAnimType(0x90003a1, 0, 1);
        sm.GetActorOwner().TryToSpeakCombatVO(Actor::GetPtrFromID(target));
        switch (fireStage) {
        case 4: {
            pGun->UpdateWait();
            EXVector head = sm.GetActorOwner().GetHeadDirection();
            EXVector d = VSub(aim, sm.GetActorOwner().GetFeetPos());
            NormaliseInline(d);
            if (head.z * d.z + head.y * d.y + head.x * d.x > 0.9f && target && pGun->CanFire()) {
                EXVector from = aim;
                from.y += 1.5f;
                EXVector hp = sm.GetActorOwner().GetHeadPos();
                EXRay ray;
                ray.Direction() = VSub(from, hp);
                ray.Origin() = hp;
                if (!sm.GetActorOwner().FriendlyActorBlockingLineOfSight(ray) && pGun->StartFiring()) {
                    fireStage = 5;
                    return 1;
                }
            }
            return 1;
        }
        case 5:
            if (!pGun->UpdateFiring(sm.GetActorOwner(), aim, 0, 0)) {
                fireStage = 4;
                return 1;
            }
            return 1;
        }
        return 1;
    case MSG_ENTER:
        fireStage = 4;
        aimTimer = 0.0f;
        sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
        return 1;
    case MSG_SUSPEND:
        if (pGun) pGun->StopFiring();
        return 1;
    case 0x2a:
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x2a)) return 1;
    case MSG_EXIT:
        if (pGun) pGun->ForceRoundToEnd();
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

// NEAR: logic read in full; register/schedule residue only expected
signed char STATE_ChargeAttack(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &flags = sm.GetIntRef(i);
    float &chargeTimer = sm.GetFloatRef(i);
    float &timer2 = sm.GetFloatRef(i);
    float &turnTimer = sm.GetFloatRef(i);
    float &seenTimer = sm.GetFloatRef(i);
    float &yRot = sm.GetFloatRef(i);
    sm.GetActorOwner().SetHeadTrackMode(0);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_EXT: {
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        if (chargeTimer > 0.0f) chargeTimer -= XApp::GameFrameRateMul();
        if (timer2 > 0.0f) timer2 -= XApp::GameFrameRateMul();
        signed char seen = sm.GetActorOwner().WasPlayerSeen();
        if (seen) seenTimer = 180.0f;
        seenTimer -= EXBaseApp::GameFrameRateMul();
        if (seen && !Player_IsCloaked()) gAIControl.SetLastSeenPlayerPos(Player_GetFeetPos());
        if (seenTimer >= 0.0f) seen = 1;
        switch (stage) {
        case 0:
            if (turnTimer > 0.0f) turnTimer -= XApp::GameFrameRateMul();
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (0.0f >= turnTimer || sm.GetActorOwner().IsFacingYRot(gAIControl.LastSeenPlayerPos(), 5.0f)) {
                sm.GetActorOwner().InvalidateGotoPos();
                chargeTimer = (EXRandf() + 3.5f) * EXBaseApp::NominalGameFrameRate();
                timer2 = (EXRandf() + 1.0f) * EXBaseApp::NominalGameFrameRate();
                stage = 1;
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(gAIControl.LastSeenPlayerPos(), 5.0f);
            return 1;
        case 1: {
            EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), gAIControl.LastSeenPlayerPos());
            if (6.25f > d.LengthXYZSquared() || 0.0f >= chargeTimer) {
                stage = 2;
                return 1;
            }
            sm.GetActorOwner().GotoLoc(gAIControl.PlayerNavLocLastSeen());
            sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
            return 1;
        }
        case 2:
            sm.GetActorOwner().InvalidateGotoPos();
            sm.GetActorOwner().SetAnimType(0x900025e, 0, 1);
            flags = 0x13;
            stage = 3;
            return 1;
        case 3:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().SetAnimType(0x900037c, 0, 1);
                stage = 4;
                return 1;
            }
            return 1;
        case 4:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 6;
                return 1;
            }
            return 1;
        case 5:
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(yRot, 5.0f)) {
                sm.GetActorOwner().InvalidateGotoPos();
                stage = 6;
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(yRot, 5.0f);
            return 1;
        case 6:
            if (seen) {
                stage = 0;
                chargeTimer = 0.0f;
                timer2 = 0.0f;
                turnTimer = (EXRandf() + 2.5f) * EXBaseApp::NominalGameFrameRate();
                return 1;
            }
            switch (EXRand32() % 7) {
            case 0:
                if (sm.GetActorOwner().CanMoveForward(1.5f)) {
                    sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
                    stage = 4;
                    return 1;
                }
                return 1;
            case 1:
                if (sm.GetActorOwner().CanMoveBack(1.5f)) {
                    sm.GetActorOwner().SetAnimType(0x4e000009, 0, 1);
                    stage = 4;
                    return 1;
                }
                return 1;
            case 2:
            case 3:
                if (sm.GetActorOwner().CanMoveForward(1.5f)) {
                    sm.GetActorOwner().SetAnimType(0x900037d, 0, 1);
                    stage = 4;
                    return 1;
                }
                return 1;
            default:
                yRot = EXRandf() * 3.1415927f;
                stage = 5;
                return 1;
            }
        }
        return 1;
    }
    case MSG_ENTER:
        chargeTimer = 0.0f;
        timer2 = 0.0f;
        turnTimer = (EXRandf() + 2.5f) * EXBaseApp::NominalGameFrameRate();
        seenTimer = 180.0f;
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x20:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x20)) StrikeCharacter(sm, 0, flags, pWeapon, 0, 1);
        return 1;
    case 0x1f:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1f)) StrikeCharacter(sm, 1, flags, pWeapon, 0, 1);
        return 1;
    case 0x1e:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e)) {
            StrikeCharacter(sm, 1, flags, pWeapon, 0, 1);
            StrikeCharacter(sm, 0, flags, pWeapon, 0, 1);
        }
        return 1;
    }
    return 0;
}

inline void AIControl::GrenadeThrown(int frames)
{
    m_GrenadeNextTime = frames / EXBaseApp::NominalGameFrameRate() + m_GrenadeTime;
}
// STATE_AttackPredArmedVantage restricted to POIs within 20 m of where the hunter started.
// NEAR: logic read in full from the disassembly; last-seen delta copies and spills differ
signed char STATE_Hunter_Sniper_1(StateMachine &sm)
{
    int i = 0;
    EXVector &offset = sm.GetVectorRef(i);
    EXVector &home = sm.GetVectorRef(i);
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    int &prevPoi = sm.GetIntRef(i);
    float &seenTimer = sm.GetFloatRef(i);
    float &giveUpTimer = sm.GetFloatRef(i);
    float &meleeTimer = sm.GetFloatRef(i);
    int &strikeFlags = sm.GetIntRef(i);
    float &fireTimer = sm.GetFloatRef(i);
    int &nextStage = sm.GetIntRef(i);
    int &moving = sm.GetIntRef(i);
    float &swapTimer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    sm.GetActorOwner().SetHeadTrackMode(0);
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    ActorCarryableGun *pGun = sm.GetActorOwner().GetGunPtr(0);
    if (!pGun) {
        sm.Return(0, 0, 0, 0);
        return 1;
    }
    GunCombatAnims anims = Actor_GetAimAnimsSet(sm.GetActorOwner(), pWeapon);
    int locType = pGun->GetLocationType();
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT: {
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        Actor &actor = sm.GetActorOwner();
        float timeout = actor.GetCombatTimeout();
        signed char seen = actor.WasPlayerSeen();
        if (seen) seenTimer = timeout;
        seenTimer -= EXBaseApp::GameFrameRateMul();
        if (seen && !Player_IsCloaked()) gAIControl.SetLastSeenPlayerPos(Player_GetFeetPos());
        if (!(seenTimer < 0.0f)) seen = 1;
        signed char seenOrClose = seen;
        if (meleeTimer > 0.0f) meleeTimer -= EXBaseApp::GameFrameRateMul();
        {
            int old = poi;
            if (!moving) {
                Actor &a = sm.GetActorOwner();
                if (old)
                    poi = POI_UpdateNearestFlagWithinRadius(gAIControl.LastSeenPlayerPos(), home, 20.0f, locType, old, prevPoi, a.GetNavZone(), a.CanJump(), 1);
                else
                    poi = POI_GetNearestFlagWithinRadius(gAIControl.LastSeenPlayerPos(), home, 20.0f, locType, 0, a.GetNavZone(), a.CanJump(), 1);
            }
            if (old != poi) {
                if (old) POI_VendorUnbind(old, sm.GetActorOwner().m_RefID);
                prevPoi = old;
                if (poi) POI_VendorBind(poi, sm.GetActorOwner().m_RefID);
                stage = 0;
            }
        }
        if (!poi) {
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(gAIControl.LastSeenPlayerPos(), 5.0f)) {
                sm.GetActorOwner().InvalidateGotoPos();
                sm.GetActorOwner().SetAimingPos(sm.GetActorOwner().GetAimPosForTarget(1));
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(gAIControl.LastSeenPlayerPos(), 5.0f);
            return 1;
        }
        EXVector stand = POI_VendorStandPos(poi, 0);
        int poly = POI_VendorStandPoly(poi, 0);
        {
            int n = SubGroupAnims::GetSubGroupAnims(sm.GetActorOwner().m_SubGroupAnimIndex).m_Anims[11];
            if (n > 0 && sm.GetActorOwner().GetConsecutiveHitCounter() >= n) {
                sm.GetActorOwner().ClearConsecutiveHitCounter();
                sm.GetActorOwner().SetInvinsibleCounter(EXBaseApp::NominalGameFrameRate() * 2);
                stage = 3;
            }
        }
        swapTimer -= EXBaseApp::GameFrameRateMul();
        float swapRange = 3.0f;
        if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f000021) swapRange = 7.0f;
        if (0.0f > swapTimer) {
            EXMaths::vec_xyzw d = VSub(gAIControl.LastSeenPlayerPos(), sm.GetActorOwner().GetFeetPos());
            if (swapRange * swapRange > d.x * d.x + d.z * d.z && EXRand32() % 0x79 == 0 &&
                sm.GetActorOwner().CarryingMixedWeapons()) {
                SM_GOTO(STATE_SwapWeaponHands, 0, 0, 0, 0);
                return 1;
            }
        }
        switch (stage) {
        case 0: {
            nextStage = 0;
            sm.GetActorOwner().GotoPosPoly(stand, poly);
            moving = 1;
            fireTimer -= XApp::GameFrameRateMul();
            EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), stand);
            if (4.0f > d.x * d.x + d.y * d.y + d.z * d.z) {
                moving = 0;
                stage = 1;
                if (!sm.GetActorOwner().IsBoss()) SetOffsetVector(offset);
                return 1;
            }
            if (!(0.0f < meleeTimer)) {
                EXMaths::vec_xyzw p = VSub(gAIControl.LastSeenPlayerPos(), sm.GetActorOwner().GetFeetPos());
                if (25.0f > p.z * p.z + p.x * p.x) {
                    stage = 2;
                    return 1;
                }
            }
            if (locType != 0x4000 && !(0.0f < fireTimer)) {
                pGun->UpdateWait();
                signed char canFire = pGun->CanFire();
                if (sm.GetActorOwner().m_ActorFlags.m_bDiscNotInHand && pWeapon->m_Id == 0x49000041) return 1;
                if (!canFire) return 1;
                if (!seen) return 1;
                fireTimer = (EXRandf() + 1.0f) * 5.0f * EXBaseApp::NominalGameFrameRate();
                stage = pGun->StartFiring() ? 6 : 8;
                return 1;
            }
            sm.GetActorOwner().SetAnimType(anims.m_Anims[9], 1, 1);
            return 1;
        }
        case 1: {
            nextStage = 1;
            pGun->UpdateWait();
            signed char canFire = pGun->CanFire();
            if (sm.GetActorOwner().m_ActorFlags.m_bDiscNotInHand && pWeapon->m_Id == 0x49000041) {
                canFire = 0;
            } else if (canFire && pWeapon->m_Id == 0x49000041) {
                EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), gAIControl.LastSeenPlayerPos());
                if (d.y * d.y + d.z * d.z + d.x * d.x > Sq(pWeapon->m_Range + 10.0f)) canFire = 0;
            }
            EXMaths::vec_xyzw c = VSub(sm.GetActorOwner().GetFeetPos(), Player_GetFeetPos());
            signed char ok = 4.0f > c.y * c.y + c.z * c.z + c.x * c.x ? 1 : seenOrClose;
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(gAIControl.LastSeenPlayerPos(), 22.0f)) {
                sm.GetActorOwner().InvalidateGotoPos();
                sm.GetActorOwner().SetAimingPos(sm.GetActorOwner().GetAimPosForTarget(1));
                if (canFire && ok) stage = pGun->StartFiring() ? 6 : 8;
            } else {
                sm.GetActorOwner().SetTargetYRot(gAIControl.LastSeenPlayerPos(), 22.0f);
            }
            if (!(0.0f < meleeTimer)) {
                EXMaths::vec_xyzw p = VSub(gAIControl.LastSeenPlayerPos(), sm.GetActorOwner().GetFeetPos());
                if (25.0f > p.z * p.z + p.x * p.x) {
                    stage = 2;
                    return 1;
                }
            }
            return 1;
        }
        case 8:
            sm.GetActorOwner().SetAnimType(anims.m_Anims[0], 1, 1);
            stage = 5;
            return 1;
        case 5:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().m_InvinsibleCounter = 0.0f;
                stage = nextStage;
                return 1;
            }
            return 1;
        case 6: {
            if (pWeapon->m_Id == 0x49000041) {
                sm.GetActorOwner().SetAnimType(0x9000354, 0, 1);
                stage = 5;
                pGun->StopFiring();
                return 1;
            }
            offset *= 0.98f;
            EXVector target = GetDelayedTarget(sm.GetActorOwner(), offset);
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(gAIControl.LastSeenPlayerPos(), 22.0f)) {
                sm.GetActorOwner().SetAimingPos(sm.GetActorOwner().GetAimPosForTarget(1));
                if (!pGun->UpdateFiring(sm.GetActorOwner(), target, 0x9000235, 1)) {
                    stage = 1;
                    return 1;
                }
                return 1;
            }
            pGun->StopLoopingSFX();
            sm.GetActorOwner().SetTargetYRot(gAIControl.LastSeenPlayerPos(), 22.0f);
            return 1;
        }
        case 2: {
            EXMaths::vec_xyzw d = VSub(gAIControl.LastSeenPlayerPos(), sm.GetActorOwner().GetFeetPos());
            float sq = d.z * d.z + d.x * d.x;
            if (pWeapon->m_MeleeRange * pWeapon->m_MeleeRange > sq) {
                stage = 3;
                return 1;
            }
            if (!sm.GetActorOwner().IsFacingYRot(gAIControl.LastSeenPlayerPos(), 5.0f))
                sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
            if (sq > 36.0f) {
                stage = 0;
                return 1;
            }
            sm.GetActorOwner().GotoLoc(gAIControl.m_PlayerNavLocLastSeen);
            sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
            return 1;
        }
        case 3:
            sm.GetActorOwner().SetAnimType(GetNormalStrikeAnim(sm, pWeapon), 0, 1);
            stage = 5;
            strikeFlags = 3;
            if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f000021) {
                sm.GetActorOwner().m_InvinsibleCounter = 100.0f;
                strikeFlags |= 4;
                meleeTimer = (EXRandf() * 0.5f + 1.0f) * EXBaseApp::NominalGameFrameRate();
            } else {
                meleeTimer = (EXRandf() * 2.0f + 3.0f) * EXBaseApp::NominalGameFrameRate();
            }
            pGun->ForceRoundToEnd();
            return 1;
        }
        return 1;
    }
    case MSG_EXIT:
        if (poi) POI_VendorUnbind(poi, sm.GetActorOwner().m_RefID);
        pGun->StopFiring();
        sm.GetActorOwner().SetHeadTrackMode(0);
        return 1;
    case MSG_ENTER:
        seenTimer = sm.GetActorOwner().GetCombatTimeout();
        stage = 0;
        giveUpTimer = (EXRandf() * 2.0f + 4.0f) * EXBaseApp::NominalGameFrameRate();
        poi = 0;
        meleeTimer = 0.0f;
        fireTimer = (EXRandf() + 1.0f) * 5.0f * EXBaseApp::NominalGameFrameRate();
        swapTimer = (EXRandf() * 0.0f + 1.0f) * EXBaseApp::NominalGameFrameRate();
        if (!sm.GetActorOwner().IsBoss()) SetOffsetVector(offset);
        pGun->SetInitialWait();
        sm.GetActorOwner().m_ActorFlags.m_bDiscNotInHand = 0;
        home = sm.GetActorOwner().GetFeetPos();
        return 1;
    case MSG_SUSPEND:
        stage = 0;
        pGun->StopFiring();
        return 1;
    case 0x19:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x19) && pWeapon->m_Id == 0x49000041) {
            // The x and z of the throw direction take separate random spreads (as retail).
            EXVector hand = sm.GetActorOwner().GetHandPos(0);
            EXVector dir(sin(EXRandf() * 1.5707964f - 0.7853982f + sm.GetActorOwner().GetRotation().y), 0.0f,
                         cos(EXRandf() * 1.5707964f - 0.7853982f + sm.GetActorOwner().GetRotation().y), 0.0f);
            EXVector zero(0.0f, 0.0f, 0.0f, 0.0f);
            XItemHandler_Bullet::CreateFromHash(hand, zero, dir, 0x49000041, 0, sm.GetActorOwner().m_RefID, 1, 1.0f);
            sm.GetActorOwner().SetHideCarryRight(1);
            sm.GetActorOwner().m_ActorFlags.m_bDiscNotInHand = 1;
        }
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_INCOMING_ATTACK:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_INCOMING_ATTACK))
            AU_HandleIncomingAttack(sm, (StructMsg_IncomingAttack *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x23:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x23)) pGun->Reload();
        return 1;
    case 0x2a:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x2a)) pGun->ForceRoundToEnd();
        return 1;
    case 0x25:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x25)) SM_GOSUB(STATE_MonsterJump, 0, 0, 0, 0);
        return 1;
    case 0x1f:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1f)) StrikeCharacter(sm, 1, strikeFlags, pWeapon, 1, 1);
        return 1;
    case 0x20:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x20)) StrikeCharacter(sm, 0, strikeFlags, pWeapon, 1, 1);
        return 1;
    case 0x1e:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e)) {
            StrikeCharacter(sm, 1, strikeFlags, pWeapon, 1, 1);
            StrikeCharacter(sm, 0, strikeFlags, pWeapon, 1, 1);
        }
        return 1;
    }
    return 0;
}

// Armed boss: holds the nearest vantage POI (gun location type) around the player's last seen
// position and fires from it; the disc thrower (0x49000041) throws a spread disc instead.
// NEAR: logic read in full from the disassembly; last-seen delta copies and spills differ
signed char STATE_AttackPredArmedVantage(StateMachine &sm)
{
    int i = 0;
    EXVector &offset = sm.GetVectorRef(i);
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    int &prevPoi = sm.GetIntRef(i);
    float &seenTimer = sm.GetFloatRef(i);
    float &giveUpTimer = sm.GetFloatRef(i);
    float &meleeTimer = sm.GetFloatRef(i);
    int &strikeFlags = sm.GetIntRef(i);
    float &fireTimer = sm.GetFloatRef(i);
    int &nextStage = sm.GetIntRef(i);
    int &moving = sm.GetIntRef(i);
    float &swapTimer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    sm.GetActorOwner().SetHeadTrackMode(0);
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    ActorCarryableGun *pGun = sm.GetActorOwner().GetGunPtr(0);
    if (!pGun) {
        sm.Return(0, 0, 0, 0);
        return 1;
    }
    GunCombatAnims anims = Actor_GetAimAnimsSet(sm.GetActorOwner(), pWeapon);
    int locType = pGun->GetLocationType();
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT: {
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        Actor &actor = sm.GetActorOwner();
        float timeout = actor.GetCombatTimeout();
        signed char seen = actor.WasPlayerSeen();
        if (seen) seenTimer = timeout;
        seenTimer -= EXBaseApp::GameFrameRateMul();
        if (seen && !Player_IsCloaked()) gAIControl.SetLastSeenPlayerPos(Player_GetFeetPos());
        if (!(seenTimer < 0.0f)) seen = 1;
        signed char seenOrClose = seen;
        if (meleeTimer > 0.0f) meleeTimer -= EXBaseApp::GameFrameRateMul();
        {
            int old = poi;
            if (!moving) {
                Actor &a = sm.GetActorOwner();
                if (old)
                    poi = POI_UpdateNearestFlag(gAIControl.LastSeenPlayerPos(), locType, old, prevPoi, a.GetNavZone(), a.CanJump(), 1, pWeapon->m_RangeMin);
                else
                    poi = POI_GetNearestFlag(gAIControl.LastSeenPlayerPos(), locType, 0, a.GetNavZone(), a.CanJump(), 1, pWeapon->m_RangeMin);
            }
            if (old != poi) {
                if (old) POI_VendorUnbind(old, sm.GetActorOwner().m_RefID);
                prevPoi = old;
                if (poi) POI_VendorBind(poi, sm.GetActorOwner().m_RefID);
                stage = 0;
            }
        }
        if (!poi) {
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(gAIControl.LastSeenPlayerPos(), 5.0f)) {
                sm.GetActorOwner().InvalidateGotoPos();
                sm.GetActorOwner().SetAimingPos(sm.GetActorOwner().GetAimPosForTarget(1));
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(gAIControl.LastSeenPlayerPos(), 5.0f);
            return 1;
        }
        EXVector stand = POI_VendorStandPos(poi, 0);
        int poly = POI_VendorStandPoly(poi, 0);
        {
            int n = SubGroupAnims::GetSubGroupAnims(sm.GetActorOwner().m_SubGroupAnimIndex).m_Anims[11];
            if (n > 0 && sm.GetActorOwner().GetConsecutiveHitCounter() >= n) {
                sm.GetActorOwner().ClearConsecutiveHitCounter();
                sm.GetActorOwner().SetInvinsibleCounter(EXBaseApp::NominalGameFrameRate() * 2);
                stage = 3;
            }
        }
        swapTimer -= EXBaseApp::GameFrameRateMul();
        float swapRange = 3.0f;
        if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f000021) swapRange = 7.0f;
        if (0.0f > swapTimer) {
            EXMaths::vec_xyzw d = VSub(gAIControl.LastSeenPlayerPos(), sm.GetActorOwner().GetFeetPos());
            if (swapRange * swapRange > d.x * d.x + d.z * d.z && EXRand32() % 0x79 == 0 &&
                sm.GetActorOwner().CarryingMixedWeapons()) {
                SM_GOTO(STATE_SwapWeaponHands, 0, 0, 0, 0);
                return 1;
            }
        }
        switch (stage) {
        case 0: {
            nextStage = 0;
            sm.GetActorOwner().GotoPosPoly(stand, poly);
            moving = 1;
            fireTimer -= XApp::GameFrameRateMul();
            EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), stand);
            if (4.0f > d.x * d.x + d.y * d.y + d.z * d.z) {
                moving = 0;
                stage = 1;
                if (!sm.GetActorOwner().IsBoss()) SetOffsetVector(offset);
                return 1;
            }
            if (!(0.0f < meleeTimer)) {
                EXMaths::vec_xyzw p = VSub(gAIControl.LastSeenPlayerPos(), sm.GetActorOwner().GetFeetPos());
                if (25.0f > p.z * p.z + p.x * p.x) {
                    stage = 2;
                    return 1;
                }
            }
            if (locType != 0x4000 && !(0.0f < fireTimer)) {
                pGun->UpdateWait();
                signed char canFire = pGun->CanFire();
                if (sm.GetActorOwner().m_ActorFlags.m_bDiscNotInHand && pWeapon->m_Id == 0x49000041) return 1;
                if (!canFire) return 1;
                if (!seen) return 1;
                fireTimer = (EXRandf() + 1.0f) * 5.0f * EXBaseApp::NominalGameFrameRate();
                stage = pGun->StartFiring() ? 6 : 8;
                return 1;
            }
            sm.GetActorOwner().SetAnimType(anims.m_Anims[9], 1, 1);
            return 1;
        }
        case 1: {
            nextStage = 1;
            pGun->UpdateWait();
            signed char canFire = pGun->CanFire();
            if (sm.GetActorOwner().m_ActorFlags.m_bDiscNotInHand && pWeapon->m_Id == 0x49000041) {
                canFire = 0;
            } else if (canFire && pWeapon->m_Id == 0x49000041) {
                EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), gAIControl.LastSeenPlayerPos());
                if (d.y * d.y + d.z * d.z + d.x * d.x > Sq(pWeapon->m_Range + 10.0f)) canFire = 0;
            }
            EXMaths::vec_xyzw c = VSub(sm.GetActorOwner().GetFeetPos(), Player_GetFeetPos());
            signed char ok = 4.0f > c.y * c.y + c.z * c.z + c.x * c.x ? 1 : seenOrClose;
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(gAIControl.LastSeenPlayerPos(), 22.0f)) {
                sm.GetActorOwner().InvalidateGotoPos();
                sm.GetActorOwner().SetAimingPos(sm.GetActorOwner().GetAimPosForTarget(1));
                if (canFire && ok) stage = pGun->StartFiring() ? 6 : 8;
            } else {
                sm.GetActorOwner().SetTargetYRot(gAIControl.LastSeenPlayerPos(), 22.0f);
            }
            if (!(0.0f < meleeTimer)) {
                EXMaths::vec_xyzw p = VSub(gAIControl.LastSeenPlayerPos(), sm.GetActorOwner().GetFeetPos());
                if (25.0f > p.z * p.z + p.x * p.x) {
                    stage = 2;
                    return 1;
                }
            }
            return 1;
        }
        case 8:
            sm.GetActorOwner().SetAnimType(anims.m_Anims[0], 1, 1);
            stage = 5;
            return 1;
        case 5:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().m_InvinsibleCounter = 0.0f;
                stage = nextStage;
                return 1;
            }
            return 1;
        case 6: {
            if (pWeapon->m_Id == 0x49000041) {
                sm.GetActorOwner().SetAnimType(0x9000354, 0, 1);
                stage = 5;
                pGun->StopFiring();
                return 1;
            }
            offset *= 0.98f;
            EXVector target = GetDelayedTarget(sm.GetActorOwner(), offset);
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(gAIControl.LastSeenPlayerPos(), 22.0f)) {
                sm.GetActorOwner().SetAimingPos(sm.GetActorOwner().GetAimPosForTarget(1));
                if (!pGun->UpdateFiring(sm.GetActorOwner(), target, 0x9000235, 1)) {
                    stage = 1;
                    return 1;
                }
                return 1;
            }
            pGun->StopLoopingSFX();
            sm.GetActorOwner().SetTargetYRot(gAIControl.LastSeenPlayerPos(), 22.0f);
            return 1;
        }
        case 2: {
            EXMaths::vec_xyzw d = VSub(gAIControl.LastSeenPlayerPos(), sm.GetActorOwner().GetFeetPos());
            float sq = d.z * d.z + d.x * d.x;
            if (pWeapon->m_MeleeRange * pWeapon->m_MeleeRange > sq) {
                stage = 3;
                return 1;
            }
            if (!sm.GetActorOwner().IsFacingYRot(gAIControl.LastSeenPlayerPos(), 5.0f))
                sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
            if (sq > 36.0f || !sm.GetActorOwner().CanGetToLocation(gAIControl.m_PlayerNavLocLastSeen)) {
                stage = 0;
                return 1;
            }
            sm.GetActorOwner().GotoLoc(gAIControl.m_PlayerNavLocLastSeen);
            sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
            return 1;
        }
        case 3:
            sm.GetActorOwner().SetAnimType(GetNormalStrikeAnim(sm, pWeapon), 0, 1);
            stage = 5;
            strikeFlags = 3;
            if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f000021) {
                sm.GetActorOwner().m_InvinsibleCounter = 100.0f;
                strikeFlags |= 4;
                meleeTimer = (EXRandf() * 0.5f + 1.0f) * EXBaseApp::NominalGameFrameRate();
            } else {
                meleeTimer = (EXRandf() * 2.0f + 3.0f) * EXBaseApp::NominalGameFrameRate();
            }
            pGun->ForceRoundToEnd();
            return 1;
        }
        return 1;
    }
    case MSG_EXIT:
        if (poi) POI_VendorUnbind(poi, sm.GetActorOwner().m_RefID);
        pGun->StopFiring();
        sm.GetActorOwner().SetHeadTrackMode(0);
        return 1;
    case MSG_ENTER:
        seenTimer = sm.GetActorOwner().GetCombatTimeout();
        stage = 0;
        giveUpTimer = (EXRandf() * 2.0f + 4.0f) * EXBaseApp::NominalGameFrameRate();
        poi = 0;
        meleeTimer = 0.0f;
        fireTimer = (EXRandf() + 1.0f) * 5.0f * EXBaseApp::NominalGameFrameRate();
        swapTimer = (EXRandf() * 0.0f + 1.0f) * EXBaseApp::NominalGameFrameRate();
        if (!sm.GetActorOwner().IsBoss()) SetOffsetVector(offset);
        pGun->SetInitialWait();
        sm.GetActorOwner().m_ActorFlags.m_bDiscNotInHand = 0;
        return 1;
    case MSG_SUSPEND:
        stage = 0;
        pGun->StopFiring();
        return 1;
    case 0x19:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x19) && pWeapon->m_Id == 0x49000041) {
            // The x and z of the throw direction take separate random spreads (as retail).
            EXVector hand = sm.GetActorOwner().GetHandPos(0);
            EXVector dir(sin(EXRandf() * 1.5707964f - 0.7853982f + sm.GetActorOwner().GetRotation().y), 0.0f,
                         cos(EXRandf() * 1.5707964f - 0.7853982f + sm.GetActorOwner().GetRotation().y), 0.0f);
            EXVector zero(0.0f, 0.0f, 0.0f, 0.0f);
            XItemHandler_Bullet::CreateFromHash(hand, zero, dir, 0x49000041, 0, sm.GetActorOwner().m_RefID, 1, 1.0f);
            sm.GetActorOwner().SetHideCarryRight(1);
            sm.GetActorOwner().m_ActorFlags.m_bDiscNotInHand = 1;
        }
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_INCOMING_ATTACK:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_INCOMING_ATTACK))
            AU_HandleIncomingAttack(sm, (StructMsg_IncomingAttack *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x23:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x23)) pGun->Reload();
        return 1;
    case 0x2a:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x2a)) pGun->ForceRoundToEnd();
        return 1;
    case 0x25:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x25)) SM_GOSUB(STATE_MonsterJump, 0, 0, 0, 0);
        return 1;
    case 0x1f:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1f)) StrikeCharacter(sm, 1, strikeFlags, pWeapon, 1, 1);
        return 1;
    case 0x20:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x20)) StrikeCharacter(sm, 0, strikeFlags, pWeapon, 1, 1);
        return 1;
    case 0x1e:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e)) {
            StrikeCharacter(sm, 1, strikeFlags, pWeapon, 1, 1);
            StrikeCharacter(sm, 0, strikeFlags, pWeapon, 1, 1);
        }
        return 1;
    }
    return 0;
}


// NEAR: logic read in full; car-position copies and the throw/fire tail are laid out differently
signed char STATE_ShootAtCar(StateMachine &sm)
{
    int i = 0;
    EXVector &target = sm.GetVectorRef(i);
    int &stage = sm.GetIntRef(i);
    int &seenCar = sm.GetIntRef(i);
    int &lostCar = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(4);
    if (lostCar)
        sm.GetActorOwner().SetWatch(7);
    else
        sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    ActorCarryableGun *pGun = sm.GetActorOwner().GetGunPtr(0);
    if (!pGun) {
        sm.Return(0, 0, 0, 0);
        return 1;
    }
    GunCombatAnims anims = Actor_GetAimAnimsSet(sm.GetActorOwner(), pWeapon);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        return 1;
    case MSG_EXIT:
        pGun->StopFiring();
        sm.GetActorOwner().SetHeadTrackMode(0);
        return 1;
    case MSG_EXT: {
        Vehicle *pCar = Vehicle::GetFirst();
        for (; pCar; pCar = pCar->GetNext())
            if (pCar->m_vehFlags & 0x10) goto found;
        goto lost;
    found: {
        EXVector carPos;
        pCar->m_currPos.get(carPos);
        EXMaths::vec_xyzw d = VSub(carPos, sm.GetActorOwner().GetFeetPos());
        if (40000.0f > d.LengthXYZSquared()) {
            EXVector eye(carPos.x, carPos.y + 2.0f, carPos.z, carPos.w);
            if (!sm.GetActorOwner().CanSeePoint(eye, 0.0f)) goto lost;
            seenCar = 1;
            sm.GetActorOwner().SetHeadTrackMode(0);
            switch (stage) {
            case 0:
                sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
                if (sm.GetActorOwner().IsFacingYRot(carPos, 22.0f)) {
                    sm.GetActorOwner().InvalidateGotoPos();
                    sm.GetActorOwner().SetAimingPos(carPos);
                    pGun->UpdateWait();
                    if (!pGun->CanFire()) return 1;
                    WeaponProfile *pThrow = pWeapon;
                    if (pThrow->m_AimType == 0x52000007) {
                        if (!gAIControl.IsGrenadeAllowed()) return 1;
                        if (sm.GetActorOwner().AreThereFriendlyActorsNear(carPos, 5.0f)) return 1;
                    } else {
                        pThrow = WeaponProfile::GetWeaponDataFromEntity(sm.GetActorOwner().GetProfile()->m_Weapon2);
                        if (!pThrow || pThrow->m_AimType != 0x52000007 || !gAIControl.IsGrenadeAllowed() ||
                            sm.GetActorOwner().AreThereFriendlyActorsNear(carPos, 5.0f)) {
                            stage = pGun->StartFiring() ? 2 : 4;
                            return 1;
                        }
                    }
                    gAIControl.GrenadeThrown(pThrow->m_LockFrames);
                    target = carPos;
                    stage = 6;
                    return 1;
                }
                sm.GetActorOwner().SetTargetYRot(carPos, 22.0f);
                return 1;
            case 2:
                sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
                if (sm.GetActorOwner().IsFacingYRot(carPos, 22.0f)) {
                    sm.GetActorOwner().SetAimingPos(carPos);
                    if (!pGun->UpdateFiring(sm.GetActorOwner(), carPos, anims.m_Anims[10], 0)) stage = 0;
                    return 1;
                }
                pGun->StopLoopingSFX();
                sm.GetActorOwner().SetTargetYRot(carPos, 22.0f);
                return 1;
            case 4:
                sm.GetActorOwner().SetAnimType(anims.m_Anims[0], 1, 1);
                stage = 5;
                return 1;
            case 5:
                if (sm.GetActorOwner().HasAnimEnded()) stage = 0;
                return 1;
            case 6:
                sm.GetActorOwner().SetAnimType(s_aThrowAnims[EXRand32() % 1], 0, 1);
                stage = 5;
                return 1;
            }
            return 1;
        }
        if (seenCar) lostCar = 1;
    }
    lost:
        sm.GetActorOwner().SetHeadTrackMode(0);
        sm.GetActorOwner().SetAnimType(0x4e000004, 0, 1);
        pGun->StopFiring();
        return 1;
    }
    case MSG_SUSPEND:
        pGun->StopFiring();
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    AU_MSG_MUGGING
    AU_MSG_SIMPLE(MSG_SOMEONE_SEEN_PREDATOR, AU_HandleSomeoneSeenPredator)
    AU_MSG_SIMPLE(MSG_LOUD_NOISE, AU_HandleLoudNoise)
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    AU_MSG_ATTRACT
    AU_MSG_FIGHT_ME
    case 0x19:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x19)) ThrowGrenade(sm, target, 0, 0);
        return 1;
    case MSG_INCOMING_ATTACK:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_INCOMING_ATTACK))
            AU_HandleIncomingAttack(sm, (StructMsg_IncomingAttack *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x23:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x23)) pGun->Reload();
        return 1;
    AU_MSG_DRIVE_BY
    case 0x2a:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x2a)) pGun->ForceRoundToEnd();
        return 1;
    }
    return 0;
}

// Mafia boss: walks to his location (0x42000092), then guns down the player or the nearest threat,
// backing off, side-stepping and sweeping his fire in a circle around the target.
// NEAR: logic read in full from the disassembly; aim-vector copies and spills differ
signed char STATE_MafiaBoss(StateMachine &sm)
{
    int i = 0;
    EXVector &aimOffset = sm.GetVectorRef(i);
    EXVector &stand = sm.GetVectorRef(i);
    float &offsetTimer = sm.GetFloatRef(i);
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    int &strikeFlags = sm.GetIntRef(i);
    int &target = sm.GetIntRef(i);
    int &sweep = sm.GetIntRef(i);
    int &run = sm.GetIntRef(i);
    float &chooseTimer = sm.GetFloatRef(i);
    float &sweepAngle = sm.GetFloatRef(i);
    float &seenTimer = sm.GetFloatRef(i);
    float &strafeTimer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    sm.GetActorOwner().SetHeadTrackMode(0);
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    ActorCarryableGun *pGun = sm.GetActorOwner().GetGunPtr(0);
    if (!pGun) {
        sm.Return(0, 0, 0, 0);
        return 1;
    }
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER: {
        stage = 0;
        target = 0;
        chooseTimer = 0.0f;
        offsetTimer = 0.0f;
        Actor &a = sm.GetActorOwner();
        int loc = POI_ResolveLocation(0x42000092, a.GetNavZone(), a.CanJump(), &a.GetFeetPos());
        POI_VendorBind(loc, sm.GetActorOwner().m_RefID);
        stand = POI_VendorStandPos(loc, POI_VendorStandIndex(loc, sm.GetActorOwner().m_RefID));
        return 1;
    }
    case MSG_EXT: {
        EXVector aimPos(0.0f, 0.0f, 0.0f, 0.0f);
        int poly = -1;
        if (stage) {
            Actor &actor = sm.GetActorOwner();
            float timeout = actor.GetCombatTimeout();
            signed char seen = actor.WasPlayerSeen();
            if (seen) seenTimer = timeout;
            seenTimer -= EXBaseApp::GameFrameRateMul();
            if (seen && !Player_IsCloaked()) gAIControl.SetLastSeenPlayerPos(Player_GetFeetPos());
            if (!(seenTimer >= 0.0f) && !seen) {
                if (target == 1) target = 0;
            } else if (target == 1) {
                goto player;
            }
            if (sm.GetActorOwner().WasPlayerSeen()) {
                float fd = sm.GetActorOwner().GetFightDistance();
                EXMaths::vec_xyzw d = VSub(gAIControl.LastSeenPlayerPos(), sm.GetActorOwner().GetFeetPos());
                if (fd * fd > d.x * d.x + d.y * d.y + d.z * d.z) target = 1;
            }
            chooseTimer -= EXBaseApp::GameFrameRateMul();
            if (!target) chooseTimer -= EXBaseApp::GameFrameRateMul();
            if (0.0f > chooseTimer && (target == 0 || target == 1)) {
                target = Actor_ChooseTarget(sm.GetActorOwner(), target == 1, EXBaseApp::NominalGameFrameRate() * 5.0f, 0);
                chooseTimer = (EXRandf() + 1.0f) * 2.0f * EXBaseApp::NominalGameFrameRate();
            }
            if (target == 1) {
            player:
                sm.GetActorOwner().SetIsInCombatWithPlayer(1);
                aimPos = gAIControl.LastSeenPlayerPos();
                poly = gAIControl.m_PlayerNavLocLastSeen.GetPolyHandle();
                sm.GetActorOwner().SetStatus(0xd);
            } else {
                Actor *p = Actor::GetPtrFromID(target);
                if (p && !p->IsDead()) {
                    aimPos = p->GetFeetPos();
                    poly = p->m_OrgLoc.GetPolyHandle();
                    sm.GetActorOwner().SetStatus(0xc);
                } else if (stage != 1) {
                    pGun->StopLoopingSFX();
                    pGun->StopFiring();
                    sweep = !sweep;
                    target = 0;
                    stage = 1;
                    sm.GetActorOwner().SetHeadTrackMode(0);
                    EXVector ahead(sin(sm.GetActorOwner().GetRotation().y) * 100.0f, 0.0f,
                                   cos(sm.GetActorOwner().GetRotation().y) * 100.0f, 0.0f);
                    EXVector p2 = VAdd(sm.GetActorOwner().GetFeetPos(), ahead);
                    sm.GetActorOwner().SetAimingPos(EXVector(p2.x, p2.y + 1.0f, p2.z, p2.w));
                    return 1;
                }
            }
        }
        switch (stage) {
        case 0: {
            sm.GetActorOwner().SetDontDoHitReaction(1);
            sm.GetActorOwner().m_InvinsibleCounter = 10.0f;
            EXMaths::vec_xyzw d = VSub(stand, sm.GetActorOwner().GetFeetPos());
            if (1.0f > d.x * d.x + d.y * d.y + d.z * d.z) {
                sm.GetActorOwner().SetDontDoHitReaction(0);
                stage = 1;
                return 1;
            }
            sm.GetActorOwner().GotoPosSlow(stand);
            sm.GetActorOwner().SetAnimType(0x9000504, 0, 1);
            return 1;
        }
        case 1: {
            if (!target) {
                EXVector ahead(sin(sm.GetActorOwner().GetRotation().y) * 10.0f, 0.0f,
                               cos(sm.GetActorOwner().GetRotation().y) * 10.0f, 0.0f);
                EXVector look = VAdd(sm.GetActorOwner().GetFeetPos(), ahead);
                look.y += 1.4f;
                offsetTimer -= XApp::GameFrameRateMul();
                if (!(0.0f < offsetTimer)) {
                    aimOffset.y = 0.0f;
                    aimOffset.x = EXRandf() * 14.0f - 7.0f;
                    aimOffset.w = 0.0f;
                    aimOffset.z = EXRandf() * 14.0f - 7.0f;
                    offsetTimer = (EXRandf() + 0.5f) * EXBaseApp::NominalGameFrameRate();
                }
                sm.GetActorOwner().SetAimingPos(VAdd(look, aimOffset));
                return 1;
            }
            EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), aimPos);
            float sq = d.x * d.x + d.y * d.y + d.z * d.z;
            float dist = sq >= 0.0f ? (float)sqrt(sq) : 0.0f;
            run = dist > 20.0f ? 1 : 0;
            stage = 2;
            return 1;
        }
        case 2: {
            sm.GetActorOwner().TryToSpeakCombatVO(Actor::GetPtrFromID(target));
            EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), aimPos);
            float sq = d.x * d.x + d.y * d.y + d.z * d.z;
            float dist = sq >= 0.0f ? (float)sqrt(sq) : 0.0f;
            if (pWeapon->m_RangeMax - 2.0f > dist) {
                sm.GetActorOwner().InvalidateGotoPos();
                stage = 3;
                return 1;
            }
            int anim = 0x9000502 + (run == 0) * 2;
            if (target == 1) {
                if (sm.GetActorOwner().CanGetToLocation(gAIControl.m_PlayerNavLocLastSeen)) {
                    sm.GetActorOwner().GotoLoc(gAIControl.m_PlayerNavLocLastSeen);
                    sm.GetActorOwner().SetAnimType(anim, 0, 1);
                }
                return 1;
            }
            if (sm.GetActorOwner().CanGetToPosPoly(aimPos, poly)) {
                sm.GetActorOwner().GotoPosPoly(aimPos, poly);
                sm.GetActorOwner().SetAnimType(anim, 0, 1);
            }
            return 1;
        }
        case 5: {
            EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), aimPos);
            float sq = d.x * d.x + d.y * d.y + d.z * d.z;
            float dist = sq >= 0.0f ? (float)sqrt(sq) : 0.0f;
            if (!sm.GetActorOwner().CanMoveBack(1.0f)) {
                sm.GetActorOwner().InvalidateGotoPos();
                stage = 3;
                return 1;
            }
            if (pWeapon->m_RangeMin + 2.0f > dist) {
                sm.GetActorOwner().SetAnimType(0x90004ff, 0, 1);
                sm.GetActorOwner().SpinToYRot(aimPos);
                return 1;
            }
            stage = 3;
            return 1;
        }
        case 3: {
            EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), aimPos);
            float sq = d.x * d.x + d.y * d.y + d.z * d.z;
            float dist = sq >= 0.0f ? (float)sqrt(sq) : 0.0f;
            if (pWeapon->m_RangeMin - 2.0f > dist && sm.GetActorOwner().CanMoveBack(2.0f)) {
                stage = 5;
                return 1;
            }
            if (!sm.GetActorOwner().IsFacingYRot(aimPos, 40.0f)) {
                sm.GetActorOwner().SetTargetYRot(aimPos, 40.0f);
                return 1;
            }
            sm.GetActorOwner().TryToSpeakCombatVO(Actor::GetPtrFromID(target));
            sm.GetActorOwner().SetAimingPos(sm.GetActorOwner().GetAimPosForTarget(target));
            pGun->UpdateWait();
            if (pGun->CanFire()) {
                if (pGun->StartFiring()) {
                    stage = 4;
                    return 1;
                }
                sm.GetActorOwner().SetAnimType(0x9000528, 0, 1);
                pGun->Reload();
                stage = 6;
                return 1;
            }
            if (strafeTimer > 0.0f) {
                strafeTimer -= XApp::GameFrameRateMul();
                return 1;
            }
            strafeTimer = (EXRandf() * 2.0f + 1.0f) * EXBaseApp::NominalGameFrameRate();
            if (EXRand32() & 1) {
                if (sm.GetActorOwner().CanStrafeLeft(2.0f)) {
                    sm.GetActorOwner().SetAnimType(0x90004fe, 0, 1);
                    stage = 6;
                    return 1;
                }
            } else if (sm.GetActorOwner().CanStrafeRight(2.0f)) {
                sm.GetActorOwner().SetAnimType(0x9000503, 0, 1);
                stage = 6;
                return 1;
            }
            return 1;
        }
        case 4: {
            EXVector a = sm.GetActorOwner().GetAimPosForTarget(target);
            if (!sm.GetActorOwner().IsFacingYRot(a, 40.0f)) {
                pGun->StopLoopingSFX();
                pGun->StopFiring();
                sm.GetActorOwner().SetTargetYRot(a, 40.0f);
                return 1;
            }
            if (sweep && fabs(a.y - sm.GetActorOwner().GetFeetPos().y) < 1.5f) {
                sweepAngle += XApp::GameFrameRateMul() * 0.05f;
                if (sweepAngle > 6.2831855f) {
                    do {
                        sweepAngle -= 6.2831855f;
                    } while (sweepAngle > 6.2831855f);
                }
                EXVector d = VSub(a, sm.GetActorOwner().GetFeetPos());
                d.y = 0.0f;
                NormaliseInline(d);
                EXVector p = VAdd(sm.GetActorOwner().GetFeetPos(), EXVector(d.x * 10.0f, d.y * 10.0f, d.z * 10.0f, d.w * 10.0f));
                a.x = p.x + (float)sin(sweepAngle) * 8.0f;
                a.y = p.y + 1.4f;
                a.z = p.z + (float)cos(sweepAngle) * 8.0f;
                a.w = p.w;
            }
            sm.GetActorOwner().SetAimingPos(a);
            if (!pGun->UpdateFiring(sm.GetActorOwner(), a, 1, target)) {
                stage = 3;
                sweep = !sweep;
                return 1;
            }
            return 1;
        }
        case 6:
            if (sm.GetActorOwner().HasAnimEnded()) stage = 3;
            return 1;
        }
        return 1;
    }
    case MSG_SUSPEND:
        pGun->StopLoopingSFX();
        pGun->StopFiring();
        return 1;
    case MSG_EXIT:
        pGun->StopLoopingSFX();
        pGun->StopFiring();
        if (poi) POI_VendorUnbind(poi, sm.GetActorOwner().m_RefID);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR)) {
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
            seenTimer = sm.GetActorOwner().GetCombatTimeout();
            target = 1;
            sm.GetActorOwner().m_ForceSightingTimer = EXBaseApp::NominalGameFrameRate() * 3.0f;
            chooseTimer = (EXRandf() + 1.0f) * 2.0f * EXBaseApp::NominalGameFrameRate();
        }
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR)) {
            if (!(0.0f < sm.GetActorOwner().m_EMPTimer)) sm.GetActorOwner().MafiaBossSheild(1);
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        }
        return 1;
    case 0x1f:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1f)) StrikeCharacter(sm, 1, strikeFlags, pWeapon, 0, 1);
        return 1;
    case 0x20:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x20)) StrikeCharacter(sm, 0, strikeFlags, pWeapon, 0, 1);
        return 1;
    case 0x1e:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e)) {
            StrikeCharacter(sm, 1, strikeFlags, pWeapon, 0, 1);
            StrikeCharacter(sm, 0, strikeFlags, pWeapon, 0, 1);
        }
        return 1;
    }
    return 0;
}

// NEAR: logic read in full; vector copies and the target-selection tail are scheduled differently
signed char STATE_MafiaBoss_NOT_GREAT(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    int &prevPoi = sm.GetIntRef(i);
    int &flags = sm.GetIntRef(i);
    int &target = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    ActorCarryableGun *pGun = sm.GetActorOwner().GetGunPtr(0);
    if (!pGun) {
        sm.Return(0, 0, 0, 0);
        return 1;
    }
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        stage = 0;
        target = 0;
        timer = 0.0f;
        return 1;
    case MSG_EXT: {
        EXVector aimPos(0.0f, 0.0f, 0.0f, 0.0f);
        target = Actor_IsPredatorCloser(sm.GetActorOwner(), target);
        timer -= EXBaseApp::GameFrameRateMul();
        if (!target) timer -= EXBaseApp::GameFrameRateMul();
        if (0.0f > timer) {
            if (target == 0 || target == 1) {
                target = Actor_ChooseTarget(sm.GetActorOwner(), target == 1, EXBaseApp::NominalGameFrameRate() * 5.0f, 0);
                timer = (EXRandf() + 1.0f) * 2.0f * EXBaseApp::NominalGameFrameRate();
            }
        }
        if (target == 1) {
            aimPos = gAIControl.LastSeenPlayerPos();
            sm.GetActorOwner().SetStatus(0xd);
        } else {
            Actor *p = Actor::GetPtrFromID(target);
            if (!p || p->IsDead()) {
                pGun->StopFiring();
                target = 0;
                sm.GetActorOwner().SetAnimType(0x9000475, 0, 1);
                sm.GetActorOwner().SetHeadTrackMode(0);
                return 1;
            }
            aimPos = p->GetFeetPos();
            sm.GetActorOwner().SetStatus(0xc);
        }
        if (target == 1) sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        int old = poi;
        Actor &actor = sm.GetActorOwner();
        if (old)
            poi = POI_UpdateNearestFlag(aimPos, 0x8000, old, prevPoi, actor.GetNavZone(), actor.CanJump(), 1, pWeapon->m_RangeMin);
        else
            poi = POI_GetNearestFlag(aimPos, 0x8000, 0, actor.GetNavZone(), actor.CanJump(), 1, pWeapon->m_RangeMin);
        if (old != poi) {
            if (old) POI_VendorUnbind(old, sm.GetActorOwner().m_RefID);
            prevPoi = old;
            if (poi) POI_VendorBind(poi, sm.GetActorOwner().m_RefID);
            stage = 1;
        }
        if (!poi) stage = 0;
        switch (stage) {
        case 0:
            sm.GetActorOwner().SetAnimType(0x9000475, 0, 1);
            return 1;
        case 1: {
            sm.GetActorOwner().TryToSpeakCombatVO(Actor::GetPtrFromID(target));
            EXVector stand = POI_VendorStandPos(poi, 0);
            EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), stand);
            if (0.0625f > d.LengthXYZSquared()) {
                sm.GetActorOwner().InvalidateGotoPos();
                stage = 2;
                return 1;
            }
            if (sm.GetActorOwner().CanGetToPosPoly(POI_VendorStandPos(poi, 0), POI_VendorStandPoly(poi, 0))) {
                sm.GetActorOwner().GotoVendorPOI(poi, 0);
                sm.GetActorOwner().SetAnimType(0x9000504, 0, 1);
                return 1;
            }
            return 1;
        }
        case 2:
            if (sm.GetActorOwner().IsFacingYRot(aimPos, 30.0f)) {
                sm.GetActorOwner().TryToSpeakCombatVO(Actor::GetPtrFromID(target));
                sm.GetActorOwner().SetAimingPos(sm.GetActorOwner().GetAimPosForTarget(target));
                pGun->UpdateWait();
                if (pGun->CanFire()) {
                    if (pGun->StartFiring()) {
                        stage = 3;
                        return 1;
                    }
                    pGun->Reload();
                    return 1;
                }
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(aimPos, 30.0f);
            return 1;
        case 3: {
            EXVector a = sm.GetActorOwner().GetAimPosForTarget(target);
            if (sm.GetActorOwner().IsFacingYRot(a, 30.0f)) {
                sm.GetActorOwner().SetAimingPos(a);
                if (!pGun->UpdateFiring(sm.GetActorOwner(), a, 1, target)) {
                    stage = 2;
                    return 1;
                }
                return 1;
            }
            pGun->StopLoopingSFX();
            sm.GetActorOwner().SetTargetYRot(a, 30.0f);
            return 1;
        }
        }
        return 1;
    }
    case MSG_SUSPEND:
        pGun->StopLoopingSFX();
        return 1;
    case MSG_EXIT:
        pGun->StopLoopingSFX();
        if (poi) POI_VendorUnbind(poi, sm.GetActorOwner().m_RefID);
        return 1;
    case MSG_RETURN:
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x1f:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1f)) StrikeCharacter(sm, 1, flags, pWeapon, 0, 1);
        return 1;
    case 0x20:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x20)) StrikeCharacter(sm, 0, flags, pWeapon, 0, 1);
        return 1;
    case 0x1e:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e)) {
            StrikeCharacter(sm, 1, flags, pWeapon, 0, 1);
            StrikeCharacter(sm, 0, flags, pWeapon, 0, 1);
        }
        return 1;
    }
    return 0;
}

// NEAR: logic read in full; player-delta copies and shared tails laid out differently
signed char STATE_BrutePredAttack_OLD(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &flags = sm.GetIntRef(i);
    float &seenTimer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetHeadTrackMode(0);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXIT:
        sm.GetActorOwner().SetDontDoHitReaction(0);
        return 1;
    case MSG_ENTER:
        seenTimer = sm.GetActorOwner().GetCombatTimeout();
        sm.GetActorOwner().SetDontDoHitReaction(1);
        return 1;
    case MSG_EXT: {
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        Actor &actor = sm.GetActorOwner();
        float timeout = actor.GetCombatTimeout();
        signed char seen = actor.WasPlayerSeen();
        if (seen) seenTimer = timeout;
        seenTimer -= EXBaseApp::GameFrameRateMul();
        if (seen && !Player_IsCloaked()) gAIControl.SetLastSeenPlayerPos(Player_GetFeetPos());
        if (seenTimer >= 0.0f) seen = 1;
        switch (stage) {
        case 0:
            if (seen && sm.GetActorOwner().CanGetToLocation(gAIControl.PlayerNavLocLastSeen())) {
                stage = 1;
                return 1;
            }
            switch (EXRand32() % 5) {
            case 0: {
                int anims[2] = { 0x900037c, 0x9000379 };
                sm.GetActorOwner().SetAnimType(anims[EXRand32() & 1], 0, 1);
                stage = 7;
                return 1;
            }
            case 1:
                sm.GetActorOwner().SetAnimType(0x900037d, 0, 1);
                stage = 7;
                return 1;
            }
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            stage = 7;
            return 1;
        case 1: {
            EXMaths::vec_xyzw d = VSub(gAIControl.LastSeenPlayerPos(), sm.GetActorOwner().GetFeetPos());
            if (d.LengthXYZSquared() > 16.0f) {
                if (sm.GetActorOwner().CanGetToLocation(gAIControl.PlayerNavLocLastSeen())) {
                    sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
                    sm.GetActorOwner().GotoLoc(gAIControl.PlayerNavLocLastSeen());
                    return 1;
                }
                sm.GetActorOwner().InvalidateGotoPos();
                stage = 0;
                return 1;
            }
            sm.GetActorOwner().InvalidateGotoPos();
            stage = 2;
            return 1;
        }
        case 2: {
            if (!seen) {
                stage = 0;
                return 1;
            }
            EXMaths::vec_xyzw d = VSub(gAIControl.LastSeenPlayerPos(), sm.GetActorOwner().GetFeetPos());
            if (d.LengthXYZSquared() > 36.0f) {
                stage = 1;
                return 1;
            }
            if (sm.GetActorOwner().IsFacingYRot(gAIControl.LastSeenPlayerPos(), 5.0f)) {
                switch (EXRand32() % 5) {
                case 0:
                    if (sm.GetActorOwner().CanStrafeLeft(1.5f)) {
                        sm.GetActorOwner().SetAnimType(0x4e00000b, 0, 1);
                        stage = 6;
                        return 1;
                    }
                    break;
                case 1:
                    if (sm.GetActorOwner().CanStrafeRight(1.5f)) {
                        sm.GetActorOwner().SetAnimType(0x4e00000c, 0, 1);
                        stage = 6;
                        return 1;
                    }
                    break;
                case 2: {
                    int anims[2] = { 0x900037c, 0x9000379 };
                    sm.GetActorOwner().SetAnimType(anims[EXRand32() & 1], 0, 1);
                    stage = 5;
                    return 1;
                }
                default:
                    if (sm.GetActorOwner().CanGetToLocation(gAIControl.PlayerNavLocLastSeen()) &&
                        1.0f > gAIControl.DistOffNaviMeshLastSeen()) {
                        stage = 3;
                        return 1;
                    }
                    break;
                }
                if (stage == 2) {
                    sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                    stage = 5;
                    return 1;
                }
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(gAIControl.LastSeenPlayerPos(), 5.0f);
            return 1;
        }
        case 3: {
            EXMaths::vec_xyzw d = VSub(gAIControl.LastSeenPlayerPos(), sm.GetActorOwner().GetFeetPos());
            float distSq = d.LengthXYZSquared();
            if (distSq > 36.0f || gAIControl.DistOffNaviMeshLastSeen() > 1.1f) {
                stage = 1;
                return 1;
            }
            if (2.25f > distSq) {
                sm.GetActorOwner().InvalidateGotoPos();
                int anims[2] = { 0x900025e, 0x900025f };
                sm.GetActorOwner().SetAnimType(anims[EXRand32() & 1], 0, 1);
                flags = 7;
                stage = 4;
                return 1;
            }
            sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
            sm.GetActorOwner().GotoLoc(gAIControl.PlayerNavLocLastSeen());
            return 1;
        }
        case 4:
        case 5:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 2;
                return 1;
            }
            return 1;
        case 6:
            if (!sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
                return 1;
            }
            stage = 2;
            return 1;
        case 7:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 0;
                return 1;
            }
            return 1;
        }
        return 1;
    }
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x20:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x20)) StrikeCharacter(sm, 0, flags, pWeapon, 0, 1);
        return 1;
    case 0x1f:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1f)) StrikeCharacter(sm, 1, flags, pWeapon, 0, 1);
        return 1;
    case 0x1e:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e)) {
            StrikeCharacter(sm, 1, flags, pWeapon, 0, 1);
            StrikeCharacter(sm, 0, flags, pWeapon, 0, 1);
        }
        return 1;
    }
    return 0;
}

// NEAR: logic read in full; delta copies and shared turn-to-player tails laid out differently
signed char STATE_AttackPredBruno_UseLoc(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    int &fleeing = sm.GetIntRef(i);
    int &fleeArrived = sm.GetIntRef(i);
    float &seenTimer = sm.GetFloatRef(i);
    float &fightTimer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT: {
        if (!fleeing) {
            ActorProfile *pProfile = sm.GetActorOwner().GetProfile();
            if (pProfile->m_Health * sm.GetActorOwner().GetProfile()->m_GunSwapHealthPct * 0.01f > sm.GetActorOwner().m_Health) {
                if (poi) POI_VendorUnbind(poi, sm.GetActorOwner().m_RefID);
                fleeing = 1;
                fleeArrived = 0;
            }
        }
        if (!fleeing || fleeArrived) {
            EXMaths::vec_xyzw d = VSub(gAIControl.LastSeenPlayerPos(), sm.GetActorOwner().GetFeetPos());
            if (25.0f > d.LengthXYZSquared()) {
                SM_GOTO(STATE_AttackPred, 0, 0, 0, 0);
                return 1;
            }
        }
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        Actor &actor = sm.GetActorOwner();
        float timeout = actor.GetCombatTimeout();
        signed char seen = actor.WasPlayerSeen();
        if (seen) seenTimer = timeout;
        seenTimer -= EXBaseApp::GameFrameRateMul();
        if (seen && !Player_IsCloaked()) gAIControl.SetLastSeenPlayerPos(Player_GetFeetPos());
        fightTimer -= EXBaseApp::GameFrameRateMul();
        if (0.0f > seenTimer && 0.0f > fightTimer) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        if (fleeing) {
            if (!poi) {
                poi = AU_FindRunAwayPoint(sm, 0x2000, 0, sm.GetActorOwner().GetNavZone(), 1);
                if (!poi) goto face;
                POI_VendorBind(poi, sm.GetActorOwner().m_RefID);
                sm.GetActorOwner().GotoVendorPOI(poi, 0);
                sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
                if (!poi) goto face;
            }
            if (0.01f > sm.GetActorOwner().SqDistToTarget()) {
                fleeArrived = 1;
                sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
                if (sm.GetActorOwner().IsFacingYRot(gAIControl.LastSeenPlayerPos(), 5.0f)) {
                    sm.GetActorOwner().InvalidateGotoPos();
                    sm.GetActorOwner().SetAnimType(0x4e000004, 0, 1);
                } else {
                    sm.GetActorOwner().SetTargetYRot(gAIControl.LastSeenPlayerPos(), 5.0f);
                }
                EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), gAIControl.LastSeenPlayerPos());
                if (100.0f > d.LengthXYZSquared()) {
                    POI_VendorUnbind(poi, sm.GetActorOwner().m_RefID);
                    POI_ForceFlagChannel(poi, sm.GetActorOwner().m_RefID);
                    poi = 0;
                }
            }
            return 1;
        }
        {
            int old = poi;
            Actor &a = sm.GetActorOwner();
            if (old)
                poi = POI_UpdateNearestFlag(gAIControl.LastSeenPlayerPos(), 0x8000, old, 0, a.GetNavZone(), a.CanJump(), 1, -1.0f);
            else
                poi = POI_GetNearestFlag(gAIControl.LastSeenPlayerPos(), 0x8000, 0, a.GetNavZone(), a.CanJump(), 1, -1.0f);
            if (old != poi) {
                if (old) POI_VendorUnbind(old, sm.GetActorOwner().m_RefID);
                if (poi) POI_VendorBind(poi, sm.GetActorOwner().m_RefID);
                stage = 0;
            }
        }
        if (!poi) {
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(gAIControl.LastSeenPlayerPos(), 5.0f)) {
                sm.GetActorOwner().InvalidateGotoPos();
                sm.GetActorOwner().SetAnimType(0x900022a, 0, 1);
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(gAIControl.LastSeenPlayerPos(), 5.0f);
            return 1;
        }
        {
            EXVector stand = POI_VendorStandPos(poi, 0);
            int poly = POI_VendorStandPoly(poi, 0);
            switch (stage) {
            case 0: {
                sm.GetActorOwner().GotoPosPoly(stand, poly);
                EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), stand);
                if (4.0f > d.LengthXYZSquared()) {
                    stage = 1;
                    return 1;
                }
                sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
                return 1;
            }
            case 1:
                goto face;
            }
        }
        return 1;
    face:
        sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
        if (sm.GetActorOwner().IsFacingYRot(gAIControl.LastSeenPlayerPos(), 5.0f)) {
            sm.GetActorOwner().InvalidateGotoPos();
            sm.GetActorOwner().SetAnimType(0x4e000004, 0, 1);
            return 1;
        }
        sm.GetActorOwner().SetTargetYRot(gAIControl.LastSeenPlayerPos(), 5.0f);
        return 1;
    }
    case MSG_EXIT:
        if (poi) POI_VendorUnbind(poi, sm.GetActorOwner().m_RefID);
        return 1;
    case MSG_ENTER:
        if (!sm.GetActorOwner().IsBoss()) {
            EXHashCodeFile::Debug_HashCodeName(sm.GetActorOwner().GetProfile()->m_Hash);
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        seenTimer = sm.GetActorOwner().GetCombatTimeout();
        stage = 0;
        fightTimer = (EXRandf() * 2.0f + 4.0f) * EXBaseApp::NominalGameFrameRate();
        poi = 0;
        fleeing = 0;
        fleeArrived = 0;
        return 1;
    case MSG_SUSPEND:
        stage = 0;
        return 1;
    case MSG_INCOMING_ATTACK:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_INCOMING_ATTACK))
            AU_HandleIncomingAttack(sm, (StructMsg_IncomingAttack *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

// Sniper message cases: ignored while the sniper has a target.
#define SNIPER_MSG(msg, fn) \
    case msg: \
        if (sm.GetActorOwner().IsMsgOnStack(msg)) return 1; \
        if (!target) fn(sm); \
        return 1;

// NEAR: logic read in full; aim/muzzle ray copies and target-selection schedule differ
signed char STATE_SniperStand(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &target = sm.GetIntRef(i);
    float &seenTimer = sm.GetFloatRef(i);
    float &chooseTimer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(4);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    ActorCarryableGun *pGun = sm.GetActorOwner().GetGunPtr(0);
    if (!pGun) {
        sm.Return(0, 0, 0, 0);
        return 1;
    }
    GunCombatAnims anims = Actor_GetAimAnimsSet(sm.GetActorOwner(), pWeapon);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        target = sm.m_Msg.m_Data[0];
        chooseTimer = (EXRandf() + 1.0f) * 2.0f * EXBaseApp::NominalGameFrameRate();
        return 1;
    case MSG_EXIT:
        pGun->StopFiring();
        sm.GetActorOwner().SetHeadTrackMode(0);
        return 1;
    case MSG_EXT: {
        target = Actor_IsPredatorCloser(sm.GetActorOwner(), target);
        chooseTimer -= EXBaseApp::GameFrameRateMul();
        if (!target) chooseTimer -= EXBaseApp::GameFrameRateMul();
        if (0.0f > chooseTimer) {
            target = Actor_ChooseTarget(sm.GetActorOwner(), target == 1, seenTimer, 0);
            chooseTimer = (EXRandf() + 1.0f) * 2.0f * EXBaseApp::NominalGameFrameRate();
        }
        if (target == 1) {
            sm.GetActorOwner().SetStatus(0xd);
        } else {
            Actor *p = Actor::GetPtrFromID(target);
            if (!p || p->IsDead()) {
                pGun->StopFiring();
                target = 0;
                sm.GetActorOwner().SetAnimType(0x4e000004, 0, 1);
                sm.GetActorOwner().SetHeadTrackMode(0);
                return 1;
            }
            sm.GetActorOwner().SetStatus(0xc);
        }
        if (target == 1) sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        EXVector aim = sm.GetActorOwner().GetAimPosForTarget(target);
        Actor &actor = sm.GetActorOwner();
        if (!Actor_CanSeeTarget(actor, target, seenTimer, actor.GetCombatTimeout())) {
            pGun->StopFiring();
            sm.GetActorOwner().SetHeadTrackMode(0);
            sm.GetActorOwner().SetAnimType(0x4e000004, 0, 1);
            return 1;
        }
        sm.GetActorOwner().SetHeadTrackMode(0);
        switch (stage) {
        case 0:
            sm.GetActorOwner().TryToSpeakCombatVO(Actor::GetPtrFromID(target));
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(aim, 22.0f)) {
                sm.GetActorOwner().SetAimingPos(aim);
                sm.GetActorOwner().InvalidateGotoPos();
                pGun->UpdateWait();
                if (!pGun->CanFire()) return 1;
                EXVector muzzle = pGun->GetMuzzleWorldPos();
                EXRay ray;
                ray.Origin() = muzzle;
                ray.Direction() = VSub(aim, muzzle);
                if (sm.GetActorOwner().FriendlyActorBlockingLineOfSight(ray)) return 1;
                stage = pGun->StartFiring() ? 2 : 4;
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(aim, 22.0f);
            return 1;
        case 2:
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(aim, 22.0f)) {
                sm.GetActorOwner().SetAimingPos(aim);
                if (!pGun->UpdateFiring(sm.GetActorOwner(), aim, anims.m_Anims[10], target)) {
                    sm.GetActorOwner().SetAnimType(anims.m_Anims[1], 1, 1);
                    stage = 0;
                    return 1;
                }
                return 1;
            }
            pGun->StopLoopingSFX();
            sm.GetActorOwner().SetTargetYRot(aim, 22.0f);
            return 1;
        case 4:
            sm.GetActorOwner().SetAnimType(anims.m_Anims[0], 1, 1);
            stage = 5;
            return 1;
        case 5:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 0;
                return 1;
            }
            return 1;
        }
        return 1;
    }
    case MSG_SUSPEND:
        pGun->StopFiring();
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    SNIPER_MSG(MSG_MUGGING, AU_HandleMuggingAttacks)
    SNIPER_MSG(MSG_SOMEONE_UNDER_ATTACK, AU_HandleSomeoneUnderAttack)
    SNIPER_MSG(MSG_SOMEONE_SEEN_PREDATOR, AU_HandleSomeoneSeenPredator)
    SNIPER_MSG(MSG_LOUD_NOISE, AU_HandleLoudNoise)
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    AU_MSG_ATTRACT
    SNIPER_MSG(MSG_FIGHT_ME, AU_HandleFightMe)
    case MSG_INCOMING_ATTACK:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_INCOMING_ATTACK))
            AU_HandleIncomingAttack(sm, (StructMsg_IncomingAttack *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x23:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x23)) pGun->Reload();
        return 1;
    SNIPER_MSG(MSG_DRIVE_BY, AU_HandleDriveBy)
    case 0x2a:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x2a)) pGun->ForceRoundToEnd();
        return 1;
    }
    return 0;
}

// Sniper that holds a home spot (within 8 m), side-stepping within 5 m of it while it shoots.
// NEAR: logic read in full from the disassembly; step-position vector copies and spills differ
signed char STATE_SniperMove(StateMachine &sm)
{
    int i = 0;
    EXVector &home = sm.GetVectorRef(i);
    int &homePoly = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    int &target = sm.GetIntRef(i);
    int &unused = sm.GetIntRef(i);
    float &seenTimer = sm.GetFloatRef(i);
    float &chooseTimer = sm.GetFloatRef(i);
    float &homeYRot = sm.GetFloatRef(i);
    float &lookYRot = sm.GetFloatRef(i);
    float &stepTimer = sm.GetFloatRef(i);
    float &lookTimer = sm.GetFloatRef(i);
    int &strikeFlags = sm.GetIntRef(i);
    float &meleeTimer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(4);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    ActorCarryableGun *pGun = sm.GetActorOwner().GetGunPtr(0);
    if (!pGun) {
        sm.Return(0, 0, 0, 0);
        return 1;
    }
    GunCombatAnims anims = Actor_GetAimAnimsSet(sm.GetActorOwner(), pWeapon);
    switch (sm.m_Msg.m_Type) {
    case MSG_SUSPEND:
        pGun->StopFiring();
        return 1;
    case MSG_ENTER:
        home = sm.GetActorOwner().GetFeetPos();
        homePoly = sm.GetActorOwner().m_OrgLoc.GetPolyHandle();
        homeYRot = lookYRot = sm.GetActorOwner().GetRotation().y;
        target = sm.m_Msg.m_Data[0];
        chooseTimer = (EXRandf() + 1.0f) * 2.0f * EXBaseApp::NominalGameFrameRate();
        unused = -1;
        stepTimer = (EXRandf() * 3.0f + 2.0f) * EXBaseApp::NominalGameFrameRate();
        lookTimer = (EXRandf() * 3.0f + 2.0f) * EXBaseApp::NominalGameFrameRate();
        meleeTimer = 0.0f;
        return 1;
    case MSG_RETURN:
        target = Actor_IsPredatorCloser(sm.GetActorOwner(), target);
        return 1;
    case MSG_EXT: {
        if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f00002d && gAIControl.m_PlayerTaggedTimer > 0.0f) target = 1;
        target = Actor_IsPredatorCloser(sm.GetActorOwner(), target);
        chooseTimer -= EXBaseApp::GameFrameRateMul();
        if (!target) chooseTimer -= EXBaseApp::GameFrameRateMul();
        if (0.0f > chooseTimer) {
            target = Actor_ChooseTarget(sm.GetActorOwner(), target == 1, seenTimer, 0);
            chooseTimer = (EXRandf() + 1.0f) * 2.0f * EXBaseApp::NominalGameFrameRate();
        }
        if (target == 1) {
            sm.GetActorOwner().SetStatus(0xd);
        } else {
            Actor *p = Actor::GetPtrFromID(target);
            if (!p || p->IsDead()) {
                pGun->StopFiring();
                target = 0;
                sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
                if (sm.GetActorOwner().IsFacingYRot(lookYRot, 5.0f)) {
                    sm.GetActorOwner().SetAnimType(0x4e000004, 0, 1);
                    lookTimer -= XApp::GameFrameRateMul();
                    if (0.0f > lookTimer) {
                        lookTimer = (EXRandf() + 1.0f) * EXBaseApp::NominalGameFrameRate() * 5.0f;
                        float rot = EXRandf() + homeYRot - 0.5f;
                        if (fabs(rot - lookYRot) > 0.25f) {
                            lookYRot = rot;
                            sm.GetActorOwner().SetHeadTrackMode(0);
                            return 1;
                        }
                    }
                } else {
                    sm.GetActorOwner().SetTargetYRot(lookYRot, 5.0f);
                }
                sm.GetActorOwner().SetHeadTrackMode(0);
                return 1;
            }
            sm.GetActorOwner().SetStatus(0xc);
        }
        if (target == 1) sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        if (gCurrentMap_GeoHash == 0x1000070 || gCurrentMap_GeoHash == 0x10000a5 || gCurrentMap_GeoHash == 0x10000f3)
            meleeTimer = 9999.0f;
        if (meleeTimer > 0.0f) meleeTimer -= XApp::GameFrameRateMul();
        if (!Actor_CanSeeTarget(sm.GetActorOwner(), target, seenTimer, EXBaseApp::NominalGameFrameRate() * 2.0f)) {
            sm.GetActorOwner().SetHeadTrackMode(0);
            sm.GetActorOwner().SetAnimType(0x4e000004, 0, 1);
            pGun->StopFiring();
            return 1;
        }
        sm.GetActorOwner().SetHeadTrackMode(0);
        switch (stage) {
        case 0: {
            sm.GetActorOwner().TryToSpeakCombatVO(Actor::GetPtrFromID(target));
            EXMaths::vec_xyzw h = VSub(home, sm.GetActorOwner().GetFeetPos());
            if (h.LengthXYZSquared() > 64.0f) {
                sm.GetActorOwner().GotoPosPoly(home, homePoly);
                stage = 7;
                return 1;
            }
            EXVector fwd(sin(sm.GetActorOwner().GetRotation().y), 0.0f, cos(sm.GetActorOwner().GetRotation().y), 0.0f);
            {
                float len = EXSqrt(fwd.x * fwd.x + fwd.z * fwd.z);
                if (len > EX_EPSILON_UNIT) fwd *= 1.0f / len;
            }
            EXVector ahead(fwd.x * 1.5f, fwd.y * 1.5f, fwd.z * 1.5f, fwd.w * 1.5f);
            EXVector side(-ahead.z, 0.0f, ahead.x, 0.0f);
            stepTimer -= XApp::GameFrameRateMul();
            EXMaths::vec_xyzw t = VSub(sm.GetActorOwner().GetFeetPos(), Actor_GetTargetFeetPos(target));
            float tSq = t.LengthXYZSquared();
            if (81.0f > tSq) {
                if (sm.GetActorOwner().IsFacingYRot(Actor_GetTargetFeetPos(target), 45.0f)) {
                    EXMaths::vec_xyzw b = VSub(VSub(sm.GetActorOwner().GetFeetPos(), ahead), home);
                    if (25.0f > b.LengthXYZSquared() && sm.GetActorOwner().CanMoveBack(1.5f)) {
                        sm.GetActorOwner().SetAnimType(anims.m_Anims[8], 1, 1);
                        stage = 8;
                        return 1;
                    }
                    switch (EXRand32() & 1) {
                    case 0: {
                        EXVector p = sm.GetActorOwner().GetFeetPos();
                        EXVector l(p.x + side.x, p.y, p.z + side.z, p.w);
                        EXMaths::vec_xyzw q = VSub(l, home);
                        if (25.0f > q.LengthXYZSquared() && sm.GetActorOwner().CanStrafeLeft(1.5f)) {
                            sm.GetActorOwner().SetAnimType(anims.m_Anims[4], 1, 1);
                            stage = 8;
                            return 1;
                        }
                        break;
                    }
                    case 1: {
                        EXVector p = sm.GetActorOwner().GetFeetPos();
                        EXVector r(p.x - side.x, p.y, p.z - side.z, p.w);
                        EXMaths::vec_xyzw q = VSub(r, home);
                        if (25.0f > q.LengthXYZSquared() && sm.GetActorOwner().CanStrafeRight(1.5f)) {
                            sm.GetActorOwner().SetAnimType(anims.m_Anims[5], 1, 1);
                            stage = 8;
                            return 1;
                        }
                        break;
                    }
                    }
                    if (4.0f > tSq && !(meleeTimer > 0.0f)) stage = 9;
                    pGun->UpdateWait();
                    pGun->UpdateWait();
                }
            } else if (0.0f > stepTimer) {
                switch (EXRand32() & 3) {
                case 0: {
                    EXMaths::vec_xyzw q = VSub(VAdd(sm.GetActorOwner().GetFeetPos(), ahead), home);
                    if (25.0f > q.LengthXYZSquared() && sm.GetActorOwner().CanMoveForward(1.5f)) {
                        sm.GetActorOwner().SetAnimType(anims.m_Anims[9], 1, 1);
                        stage = 8;
                        return 1;
                    }
                    break;
                }
                case 1: {
                    EXMaths::vec_xyzw q = VSub(VSub(sm.GetActorOwner().GetFeetPos(), ahead), home);
                    if (25.0f > q.LengthXYZSquared() && sm.GetActorOwner().CanMoveBack(1.5f)) {
                        sm.GetActorOwner().SetAnimType(anims.m_Anims[8], 1, 1);
                        stage = 8;
                        return 1;
                    }
                    break;
                }
                case 2: {
                    EXVector p = sm.GetActorOwner().GetFeetPos();
                    EXVector l(side.x + p.x, p.y, p.z + side.z, p.w);
                    EXMaths::vec_xyzw q = VSub(l, home);
                    if (25.0f > q.LengthXYZSquared() && sm.GetActorOwner().CanStrafeLeft(1.5f)) {
                        sm.GetActorOwner().SetAnimType(anims.m_Anims[4], 1, 1);
                        stage = 8;
                        return 1;
                    }
                    break;
                }
                case 3: {
                    EXVector p = sm.GetActorOwner().GetFeetPos();
                    EXVector r(p.x - side.x, p.y, p.z - side.z, p.w);
                    EXMaths::vec_xyzw q = VSub(r, home);
                    if (25.0f > q.LengthXYZSquared() && sm.GetActorOwner().CanStrafeRight(1.5f)) {
                        sm.GetActorOwner().SetAnimType(anims.m_Anims[5], 1, 1);
                        stage = 8;
                        return 1;
                    }
                    break;
                }
                }
            }
            if (sm.GetActorOwner().IsFacingYRot(Actor_GetTargetFeetPos(target), 45.0f)) {
                pGun->UpdateWait();
                if (pGun->CanFire()) {
                    EXVector muzzle = pGun->GetMuzzleWorldPos();
                    EXRay ray;
                    ray.Origin() = muzzle;
                    ray.Direction() = VSub(Actor_GetTargetFeetPos(target), muzzle);
                    if (!sm.GetActorOwner().FriendlyActorBlockingLineOfSight(ray)) {
                        if (pWeapon->m_AimType != 0x52000007) {
                            stage = pGun->StartFiring() ? 2 : 4;
                            return 1;
                        }
                        if (gAIControl.m_GrenadeTime > gAIControl.m_GrenadeNextTime &&
                            !sm.GetActorOwner().AreThereFriendlyActorsNear(Actor_GetTargetFeetPos(target), 5.0f)) {
                            gAIControl.GrenadeThrown(pWeapon->m_LockFrames);
                            stage = 6;
                            return 1;
                        }
                        stage = 0;
                        unused = -1;
                        pGun->DelayWait((EXRandf() + 1.0f) * 0.5f * EXBaseApp::NominalGameFrameRate());
                    }
                }
            }
            if (stage) return 1;
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(Actor_GetTargetFeetPos(target), 22.0f)) {
                sm.GetActorOwner().SetAimingPos(sm.GetActorOwner().GetAimPosForTarget(target));
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(Actor_GetTargetFeetPos(target), 22.0f);
            return 1;
        }
        case 2:
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(Actor_GetTargetFeetPos(target), 22.0f)) {
                sm.GetActorOwner().SetAimingPos(sm.GetActorOwner().GetAimPosForTarget(target));
                if (!pGun->UpdateFiring(sm.GetActorOwner(), sm.GetActorOwner().GetAimPosForTarget(target), anims.m_Anims[10], target)) {
                    stage = 0;
                    unused = -1;
                    return 1;
                }
                return 1;
            }
            pGun->StopLoopingSFX();
            sm.GetActorOwner().SetTargetYRot(Actor_GetTargetFeetPos(target), 22.0f);
            return 1;
        case 4:
            sm.GetActorOwner().SetAnimType(anims.m_Anims[0], 1, 1);
            stage = 5;
            return 1;
        case 5:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 0;
                unused = -1;
                return 1;
            }
            return 1;
        case 6:
            sm.GetActorOwner().SetAnimType(s_aThrowAnims[EXRand32() % 1], 0, 1);
            stage = 5;
            return 1;
        case 7: {
            EXMaths::vec_xyzw h = VSub(home, sm.GetActorOwner().GetFeetPos());
            if (h.LengthXYZSquared() > 16.0f) {
                sm.GetActorOwner().SetAnimType(anims.m_Anims[9], 1, 1);
                return 1;
            }
            stage = 0;
            sm.GetActorOwner().InvalidateGotoPos();
            return 1;
        }
        case 8:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stepTimer = (EXRandf() * 3.0f + 2.0f) * EXBaseApp::NominalGameFrameRate();
                stage = 0;
                unused = -1;
                return 1;
            }
            sm.GetActorOwner().SpinToYRot(Actor_GetTargetFeetPos(target));
            return 1;
        case 9:
            sm.GetActorOwner().SetAnimType(GetNormalStrikeAnim(sm, pWeapon), 0, 1);
            stage = 5;
            strikeFlags = 3;
            meleeTimer = (EXRandf() * 3.0f + 2.0f) * EXBaseApp::NominalGameFrameRate();
            pGun->ForceRoundToEnd();
            return 1;
        }
        return 1;
    }
    case MSG_EXIT:
        pGun->StopFiring();
        sm.GetActorOwner().SetHeadTrackMode(0);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    AU_MSG_ATTRACT
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    SNIPER_MSG(MSG_MUGGING, AU_HandleMuggingAttacks)
    SNIPER_MSG(MSG_SOMEONE_UNDER_ATTACK, AU_HandleSomeoneUnderAttack)
    SNIPER_MSG(MSG_SOMEONE_SEEN_PREDATOR, AU_HandleSomeoneSeenPredator)
    SNIPER_MSG(MSG_LOUD_NOISE, AU_HandleLoudNoise)
    SNIPER_MSG(MSG_FIGHT_ME, AU_HandleFightMe)
    SNIPER_MSG(MSG_DRIVE_BY, AU_HandleDriveBy)
    case 0x23:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x23)) pGun->Reload();
        return 1;
    case 0x2a:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x2a)) pGun->ForceRoundToEnd();
        return 1;
    case 0x19:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x19)) ThrowGrenade(sm, Actor_GetTargetFeetPos(target), 0, 0);
        return 1;
    case MSG_INCOMING_ATTACK:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_INCOMING_ATTACK))
            AU_HandleIncomingAttack(sm, (StructMsg_IncomingAttack *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x1f:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1f)) StrikeCharacter(sm, 1, strikeFlags, pWeapon, 1, 1);
        return 1;
    case 0x20:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x20)) StrikeCharacter(sm, 0, strikeFlags, pWeapon, 1, 1);
        return 1;
    case 0x1e:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e)) {
            StrikeCharacter(sm, 1, strikeFlags, pWeapon, 1, 1);
            StrikeCharacter(sm, 0, strikeFlags, pWeapon, 1, 1);
        }
        return 1;
    }
    return 0;
}

extern float gTimer_OverridePlayerCloak;  // TODO stand-in: Actor.obj

// Bruiser skin on map 0x100011d: sees through the cloak while the override timer runs, and raises the alarm.
static inline signed char IsSpecialLookSkin(Actor &actor)
{
    return actor.GetSkin()->m_Hash == 0xd000138 && gCurrentMap_GeoHash == 0x100011d;
}

// NEAR: logic read in full; look-position vector copies and the shared attack tail differ
signed char STATE_LookAroundAlerted(StateMachine &sm)
{
    int i = 0;
    EXVector &lookPos = sm.GetVectorRef(i);
    int &stage = sm.GetIntRef(i);
    int &seen = sm.GetIntRef(i);
    int &done = sm.GetIntRef(i);
    sm.GetActorOwner().SetWatch(4);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXIT:
        sm.GetActorOwner().SetLongRangeVision(0);
        return 1;
    case MSG_ENTER: {
        EXMaths::vec_xyzw d = VSub(Player_GetFeetPos(), sm.GetActorOwner().GetFeetPos());
        EXVector v(d.x, 0.0f, d.z, d.w);
        NormaliseInline(v);
        float r = EXRandf() * 20.0f - 10.0f;
        EXVector side(r * -v.z, r * v.y, r * v.x, r * v.w);
        EXVector ahead(v.x * 10.0f, v.y * 10.0f, v.z * 10.0f, v.w * 10.0f);
        lookPos = VAdd(VAdd(sm.GetActorOwner().GetFeetPos(), ahead), side);
        stage = 0;
        seen = 0;
        sm.GetActorOwner().SetLongRangeVision(1);
        return 1;
    }
    case MSG_EXT: {
        if (done) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), gAIControl.LastSeenPlayerPos());
        float distSq = d.LengthXYZSquared();
        float range = sm.GetActorOwner().GetProfile()->m_AttackRange;
        if (IsSpecialLookSkin(sm.GetActorOwner())) {
            range = 3.0f;
            EXMaths::vec_xyzw p = VSub(sm.GetActorOwner().GetFeetPos(), Player_GetFeetPos());
            distSq = p.LengthXYZSquared();
        }
        if (range * range > distSq) goto attack;
        if (!seen && ActorOptionsManager::PlayerVisible()) {
            signed char override = 0;
            if (sm.GetActorOwner().GetSkin()->m_Hash == 0xd000138 && gTimer_OverridePlayerCloak > 0.0f &&
                gCurrentMap_GeoHash == 0x100011d)
                override = 1;
            if (!Player_IsCloaked() || override) {
                EXVector chest = XItemHandler_Player::Instance()->GetChestPos();
                chest.y = EXRandf() + chest.y - 0.5f;
                if (sm.GetActorOwner().CanSeePoint(chest, 0.0f)) seen = 1;
            }
        }
        switch (stage) {
        case 0:
            if (sm.GetActorOwner().IsFacingYRot(lookPos, 15.0f)) {
                if (g_ulLookAtAnim == 1) {
                    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
                    if (pWeapon && pWeapon->m_AimType != 0x52000005 && pWeapon->m_AimType != 0x52000006) g_ulLookAtAnim = 2;
                }
                switch (g_ulLookAtAnim) {
                case 0:
                    sm.GetActorOwner().SetAnimType(0x9000308, 0, 1);
                    break;
                case 1:
                    sm.GetActorOwner().SetAnimType(0x9000305, 0, 1);
                    break;
                case 2:
                    sm.GetActorOwner().SetAnimType(0x9000108, 0, 1);
                    break;
                }
                if (++g_ulLookAtAnim > 2) g_ulLookAtAnim = 0;
                stage = 1;
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(lookPos, 15.0f);
            return 1;
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) {
                if (!seen) {
                    sm.GetActorOwner().CancelAlarmMeter();
                    sm.Return(0, 0, 0, 0);
                    return 1;
                }
                if (!(EXRand32() & 1)) goto attack;
            } else {
                if (!seen) return 1;
                if (!IsSpecialLookSkin(sm.GetActorOwner())) return 1;
            }
            sm.GetActorOwner().SetAnimType(0x900030a, 0, 1);
            stage = 2;
            return 1;
        case 2:
            if (sm.GetActorOwner().HasAnimEnded()) goto attack;
            sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
            return 1;
        }
        return 1;
    attack:
        done = 1;
        if (IsSpecialLookSkin(sm.GetActorOwner())) sm.GetActorOwner().InstantAlarm();
        SM_GOSUB(STATE_AttackPred, 0, 0, 0, 0);
        return 1;
    }
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

// NEAR: logic read in full; stand-position distance copies and sqrt temporaries laid out differently
signed char STATE_MonsterSquad_Sniper(StateMachine &sm)
{
    int i = 0;
    EXVector &aim = sm.GetVectorRef(i);
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    int &prevPoi = sm.GetIntRef(i);
    int &flags = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    sm.GetActorOwner().SetHeadTrackMode(0);
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    ActorCarryableGun *pGun = sm.GetActorOwner().GetGunPtr(0);
    if (!pGun) {
        sm.Return(0, 0, 0, 0);
        return 1;
    }
    switch (sm.m_Msg.m_Type) {
    case MSG_EXIT:
    case MSG_SUSPEND:
        pGun->StopFiring();
        return 1;
    case MSG_RETURN:
        return 1;
    case MSG_EXT: {
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        EXVector player = Player_GetFeetPos();
        if (stage == 0 || stage == 1) {
            int old = poi;
            Actor &actor = sm.GetActorOwner();
            if (old) {
                int np = POI_UpdateNearestFlag(player, 0x4000, old, prevPoi, actor.GetNavZone(), actor.CanJump(), 1, -1.0f);
                EXVector standOld = POI_VendorStandPos(poi, 0);
                EXVector standNew = POI_VendorStandPos(np, 0);
                EXMaths::vec_xyzw d1 = VSub(standOld, player);
                float len1 = ForceSqrt(d1.LengthXYZSquared());
                EXMaths::vec_xyzw d2 = VSub(standNew, player);
                float len2 = ForceSqrt(d2.LengthXYZSquared());
                if (len1 - 5.0f > len2) poi = np;
            } else {
                poi = POI_GetNearestFlag(player, 0x4000, 0, actor.GetNavZone(), actor.CanJump(), 1, -1.0f);
            }
            if (old != poi) {
                if (old) POI_VendorUnbind(old, sm.GetActorOwner().m_RefID);
                prevPoi = old;
                if (poi) POI_VendorBind(poi, sm.GetActorOwner().m_RefID);
                stage = 0;
            }
        }
        if (!poi) {
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            return 1;
        }
        EXVector stand = POI_VendorStandPos(poi, 0);
        if (stage == 0 || stage == 1) {
            EXMaths::vec_xyzw d = VSub(Player_GetFeetPos(), sm.GetActorOwner().GetFeetPos());
            if (4.0f > d.LengthXYZSquared()) stage = 4;
        }
        switch (stage) {
        case 0: {
            sm.GetActorOwner().TryToSpeakCombatVO(0);
            EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), stand);
            if (0.25f > ForceSqrt(d.LengthXYZSquared())) {
                sm.GetActorOwner().InvalidateGotoPos();
                stage = 1;
                return 1;
            }
            if (sm.GetActorOwner().CanGetToPosPoly(POI_VendorStandPos(poi, 0), POI_VendorStandPoly(poi, 0))) {
                sm.GetActorOwner().GotoVendorPOI(poi, 0);
                sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
                return 1;
            }
            return 1;
        }
        case 1:
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(player, 40.0f)) {
                sm.GetActorOwner().TryToSpeakCombatVO(0);
                aim = sm.GetActorOwner().GetAimPosForTarget(1);
                sm.GetActorOwner().SetAimingPos(aim);
                pGun->UpdateWait();
                if (pGun->CanFire()) {
                    if (pGun->StartFiring()) {
                        stage = 3;
                        return 1;
                    }
                    pGun->Reload();
                    return 1;
                }
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(player, 40.0f);
            return 1;
        case 2:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 1;
                return 1;
            }
            sm.GetActorOwner().SpinToYRot(Player_GetFeetPos());
            return 1;
        case 3:
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            aim = sm.GetActorOwner().GetAimPosForTarget(1);
            sm.GetActorOwner().SetAimingPos(aim);
            if (!pGun->UpdateFiring(sm.GetActorOwner(), aim, 1, 1)) {
                stage = 1;
                return 1;
            }
            return 1;
        case 4:
            sm.GetActorOwner().InvalidateGotoPos();
            flags = 0xb;
            sm.GetActorOwner().SetAnimType(0x900025b, 0, 1);
            stage = 5;
            return 1;
        case 5:
            if (sm.GetActorOwner().HasAnimEnded()) {
                pGun->m_Wait = 0.0f;
                stage = 1;
                return 1;
            }
            sm.GetActorOwner().SpinToYRot(Player_GetFeetPos());
            return 1;
        }
        return 1;
    }
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x1f:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1f)) StrikeCharacter(sm, 1, flags, pWeapon, 0, 1);
        return 1;
    case 0x20:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x20)) StrikeCharacter(sm, 0, flags, pWeapon, 0, 1);
        return 1;
    case 0x1e:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e)) {
            StrikeCharacter(sm, 1, flags, pWeapon, 0, 1);
            StrikeCharacter(sm, 0, flags, pWeapon, 0, 1);
        }
        return 1;
    }
    return 0;
}

// NEAR: logic read in full; aim/grenade target vectors built through extra copies in retail
signed char STATE_ShootForwards(StateMachine &sm)
{
    int i = 0;
    EXVector &aimPos = sm.GetVectorRef(i);
    EXVector &grenadeTarget = sm.GetVectorRef(i);
    EXVector &startPos = sm.GetVectorRef(i);
    int &startPoly = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    float &returnTimer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(4);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    ActorCarryableGun *pGun = sm.GetActorOwner().GetGunPtr(0);
    if (!pGun) {
        sm.Return(0, 0, 0, 0);
        return 1;
    }
    GunCombatAnims anims = Actor_GetAimAnimsSet(sm.GetActorOwner(), pWeapon);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXIT:
        pGun->StopFiring();
        return 1;
    case MSG_ENTER: {
        float s = (float)sin(sm.GetActorOwner().GetRotation().y);
        float c = (float)cos(sm.GetActorOwner().GetRotation().y);
        EXVector far(s * 1000.0f, 0.0f, c * 1000.0f, 0.0f);
        aimPos = VAdd(sm.GetActorOwner().GetFeetPos(), far);
        aimPos.y += 1.2f;
        EXVector near(s * 15.0f, 0.0f, c * 15.0f, 0.0f);
        grenadeTarget = VAdd(sm.GetActorOwner().GetFeetPos(), near);
        startPos = (EXMaths::vec_xyzw)sm.GetActorOwner().GetFeetPos();
        startPoly = sm.GetActorOwner().OrgLoc().GetPolyHandle();
        return 1;
    }
    case MSG_EXT: {
        EXMaths::vec_xyzw d = VSub(Player_GetFeetPos(), sm.GetActorOwner().GetFeetPos());
        if (2.0f > (float)fabs(d.y) && 225.0f > d.LengthXYZSquared()) {
            SM_GOSUB(STATE_AttackPred, 0, 0, 0, 0);
            return 1;
        }
        switch (stage) {
        case 0: {
            EXMaths::vec_xyzw h = VSub(startPos, sm.GetActorOwner().GetFeetPos());
            if (h.LengthXYZSquared() > 1.0f) {
                stage = 6;
                return 1;
            }
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(aimPos, 22.0f)) {
                sm.GetActorOwner().SetAimingPos(aimPos);
                sm.GetActorOwner().InvalidateGotoPos();
                pGun->UpdateWait();
                if (!pGun->CanFire()) return 1;
                if (pWeapon->m_AimType == 0x52000007) {
                    if (gAIControl.IsGrenadeAllowed()) {
                        gAIControl.GrenadeThrown(pWeapon->m_LockFrames);
                        sm.GetActorOwner().SetAnimType(s_aThrowAnims[EXRand32() % 1], 0, 1);
                        stage = 5;
                        return 1;
                    }
                    pGun->DelayWait((EXRandf() + 1.0f) * 0.5f * EXBaseApp::NominalGameFrameRate());
                    return 1;
                }
                stage = pGun->StartFiring() ? 2 : 4;
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(aimPos, 22.0f);
            return 1;
        }
        case 2:
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(aimPos, 22.0f)) {
                sm.GetActorOwner().SetAimingPos(aimPos);
                if (!pGun->UpdateFiring(sm.GetActorOwner(), aimPos, anims.m_Anims[10], 0)) {
                    sm.GetActorOwner().SetAnimType(anims.m_Anims[1], 1, 1);
                    stage = 0;
                    return 1;
                }
                return 1;
            }
            pGun->StopLoopingSFX();
            sm.GetActorOwner().SetTargetYRot(aimPos, 22.0f);
            return 1;
        case 4:
            sm.GetActorOwner().SetAnimType(anims.m_Anims[0], 1, 1);
            stage = 5;
            return 1;
        case 5:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 0;
                return 1;
            }
            return 1;
        case 6:
            sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
            sm.GetActorOwner().GotoPosPoly(startPos, startPoly);
            if (6.25f > sm.GetActorOwner().SqDistToTarget()) {
                returnTimer -= XApp::GameFrameRateMul();
                if (0.0f >= returnTimer || 0.01f > sm.GetActorOwner().SqDistToTarget()) {
                    stage = 0;
                    return 1;
                }
                return 1;
            }
            returnTimer = EXBaseApp::NominalGameFrameRate() * 5.0f;
            return 1;
        }
        return 1;
    }
    case MSG_SUSPEND:
        pGun->StopFiring();
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR)) {
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
            SM_GOSUB(STATE_AttackPred, 0, 0, 0, 0);
        }
        return 1;
    AU_MSG_ATTRACT
    case 0x23:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x23)) pGun->Reload();
        return 1;
    case 0x19:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x19)) ThrowGrenade(sm, grenadeTarget, 0, 0);
        return 1;
    case 0x2a:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x2a)) pGun->ForceRoundToEnd();
        return 1;
    case MSG_INCOMING_ATTACK:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_INCOMING_ATTACK))
            AU_HandleIncomingAttack(sm, (StructMsg_IncomingAttack *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

// NEAR: logic read in full; distance/sweep vector copies scheduled differently
signed char STATE_MonsterSquad_Freeze(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &sweep = sm.GetIntRef(i);
    float &strafeTimer = sm.GetFloatRef(i);
    float &sweepAngle = sm.GetFloatRef(i);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    sm.GetActorOwner().SetHeadTrackMode(0);
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    ActorCarryableGun *pGun = sm.GetActorOwner().GetGunPtr(0);
    if (!pGun) {
        sm.Return(0, 0, 0, 0);
        return 1;
    }
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        stage = 0;
        return 1;
    case MSG_EXIT:
    case MSG_SUSPEND:
        pGun->StopFiring();
        return 1;
    case MSG_RETURN:
        return 1;
    case MSG_EXT: {
        EXVector player = Player_GetFeetPos();
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        switch (stage) {
        case 0: {
            sm.GetActorOwner().TryToSpeakCombatVO(0);
            EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), player);
            if (pWeapon->m_RangeMax - 2.0f > ForceSqrt(d.LengthXYZSquared())) {
                sm.GetActorOwner().InvalidateGotoPos();
                stage = 1;
                return 1;
            }
            if (sm.GetActorOwner().CanGetToLocation(gAIControl.PlayerNavLoc())) {
                sm.GetActorOwner().GotoLoc(gAIControl.PlayerNavLoc());
                sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
                return 1;
            }
            return 1;
        }
        case 1: {
            EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), player);
            float dist = ForceSqrt(d.LengthXYZSquared());
            if (pWeapon->m_RangeMin - 2.0f > dist && sm.GetActorOwner().CanMoveBack(2.0f)) {
                stage = 4;
                return 1;
            }
            if (dist > pWeapon->m_RangeMax) {
                stage = 0;
                return 1;
            }
            if (sm.GetActorOwner().IsFacingYRot(player, 40.0f)) {
                sm.GetActorOwner().TryToSpeakCombatVO(0);
                sm.GetActorOwner().SetAimingPos(sm.GetActorOwner().GetAimPosForTarget(1));
                pGun->UpdateWait();
                if (pGun->CanFire()) {
                    if (pGun->StartFiring()) {
                        stage = 3;
                        return 1;
                    }
                    pGun->Reload();
                }
                if (strafeTimer > 0.0f) {
                    strafeTimer -= XApp::GameFrameRateMul();
                    return 1;
                }
                strafeTimer = (EXRandf() * 2.0f + 1.0f) * EXBaseApp::NominalGameFrameRate();
                if (EXRand32() & 1) {
                    if (sm.GetActorOwner().CanStrafeLeft(2.0f)) {
                        sm.GetActorOwner().SetAnimType(0x90004fe, 0, 1);
                        stage = 2;
                        return 1;
                    }
                    return 1;
                }
                if (sm.GetActorOwner().CanStrafeRight(2.0f)) {
                    sm.GetActorOwner().SetAnimType(0x9000503, 0, 1);
                    stage = 2;
                    return 1;
                }
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(player, 40.0f);
            return 1;
        }
        case 2:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 1;
                return 1;
            }
            sm.GetActorOwner().SpinToYRot(Player_GetFeetPos());
            return 1;
        case 3: {
            EXVector aim = sm.GetActorOwner().GetAimPosForTarget(1);
            if (sm.GetActorOwner().IsFacingYRot(aim, 40.0f)) {
                if (sweep && 1.5f > (float)fabs(aim.y - sm.GetActorOwner().GetFeetPos().y)) {
                    sweepAngle += XApp::GameFrameRateMul() * 0.05f;
                    if (sweepAngle > 6.2831855f) {
                        float a = sweepAngle;
                        do a -= 6.2831855f;
                        while (a > 6.2831855f);
                        sweepAngle = a;
                    }
                    EXMaths::vec_xyzw d = VSub(aim, sm.GetActorOwner().GetFeetPos());
                    EXVector v(d.x, 0.0f, d.z, d.w);
                    NormaliseInline(v);
                    EXVector p(v.x * 10.0f + sm.GetActorOwner().GetFeetPos().x, v.y * 10.0f + sm.GetActorOwner().GetFeetPos().y,
                               v.z * 10.0f + sm.GetActorOwner().GetFeetPos().z, v.w * 10.0f + sm.GetActorOwner().GetFeetPos().w);
                    p.y += 1.4f;
                    float s = (float)sin(sweepAngle) * 8.0f;
                    float c = (float)cos(sweepAngle) * 8.0f;
                    aim.x = p.x + s;
                    aim.y = p.y;
                    aim.z = p.z + c;
                    aim.w = p.w;
                }
                sm.GetActorOwner().SetAimingPos(aim);
                if (!pGun->UpdateFiring(sm.GetActorOwner(), aim, 1, 1)) {
                    stage = 1;
                    sweep = sweep == 0;
                    return 1;
                }
                return 1;
            }
            pGun->StopLoopingSFX();
            pGun->StopFiring();
            sm.GetActorOwner().SetTargetYRot(aim, 40.0f);
            return 1;
        }
        case 4: {
            EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), player);
            float dist = ForceSqrt(d.LengthXYZSquared());
            if (!sm.GetActorOwner().CanMoveBack(1.0f) || !(pWeapon->m_RangeMin + 2.0f > dist)) {
                stage = 1;
                return 1;
            }
            sm.GetActorOwner().SetAnimType(0x90004ff, 0, 1);
            sm.GetActorOwner().SpinToYRot(player);
            return 1;
        }
        }
        return 1;
    }
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

// NEAR: logic read in full; stand-position distance copies and sqrt temporaries laid out differently
signed char STATE_MonsterSquad_Missiles(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    int &prevPoi = sm.GetIntRef(i);
    int &holdPos = sm.GetIntRef(i);
    int &flags = sm.GetIntRef(i);
    float &moveTimer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    sm.GetActorOwner().SetHeadTrackMode(0);
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    ActorCarryableGun *pGun = sm.GetActorOwner().GetGunPtr(0);
    if (!pGun) {
        sm.Return(0, 0, 0, 0);
        return 1;
    }
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        stage = 0;
        moveTimer = (EXRandf() * 3.0f + 5.0f) * EXBaseApp::NominalGameFrameRate();
        return 1;
    case MSG_EXIT:
    case MSG_SUSPEND:
        pGun->StopFiring();
        return 1;
    case MSG_RETURN:
        return 1;
    case MSG_EXT: {
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        EXVector player = Player_GetFeetPos();
        {
            int old = poi;
            Actor &actor = sm.GetActorOwner();
            if (old) {
                int np = POI_UpdateNearestFlag(player, 0x8000, old, prevPoi, actor.GetNavZone(), actor.CanJump(), 1, -1.0f);
                EXVector standOld = POI_VendorStandPos(poi, 0);
                EXVector standNew = POI_VendorStandPos(np, 0);
                EXMaths::vec_xyzw d1 = VSub(standOld, player);
                float len1 = ForceSqrt(d1.LengthXYZSquared());
                EXMaths::vec_xyzw d2 = VSub(standNew, player);
                float len2 = ForceSqrt(d2.LengthXYZSquared());
                if (len1 - 5.0f > len2) poi = np;
            } else {
                poi = POI_GetNearestFlag(player, 0x8000, 0, actor.GetNavZone(), actor.CanJump(), 1, -1.0f);
            }
            if (old != poi) {
                if (old) POI_VendorUnbind(old, sm.GetActorOwner().m_RefID);
                prevPoi = old;
                if (poi) POI_VendorBind(poi, sm.GetActorOwner().m_RefID);
                holdPos = 0;
                stage = 0;
            }
        }
        if (!poi) {
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            return 1;
        }
        EXVector stand = POI_VendorStandPos(poi, 0);
        if ((stage == 0 || stage == 1) && !holdPos) {
            EXMaths::vec_xyzw d = VSub(Player_GetFeetPos(), sm.GetActorOwner().GetFeetPos());
            if (4.0f > d.LengthXYZSquared()) stage = 4;
        }
        switch (stage) {
        case 0: {
            moveTimer -= XApp::GameFrameRateMul();
            if (0.0f >= moveTimer) {
                pGun->m_Wait = 0.0f;
                holdPos = 1;
                moveTimer = (EXRandf() + 1.0f) * EXBaseApp::NominalGameFrameRate() * 3.0f;
                stage = 1;
                return 1;
            }
            sm.GetActorOwner().TryToSpeakCombatVO(0);
            EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), stand);
            if (0.25f > ForceSqrt(d.LengthXYZSquared())) {
                sm.GetActorOwner().InvalidateGotoPos();
                stage = 1;
                return 1;
            }
            if (sm.GetActorOwner().CanGetToPosPoly(POI_VendorStandPos(poi, 0), POI_VendorStandPoly(poi, 0))) {
                sm.GetActorOwner().GotoVendorPOI(poi, 0);
                sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
                return 1;
            }
            return 1;
        }
        case 1:
            if (sm.GetActorOwner().IsFacingYRot(player, 40.0f)) {
                sm.GetActorOwner().TryToSpeakCombatVO(0);
                sm.GetActorOwner().SetAimingPos(sm.GetActorOwner().GetAimPosForTarget(1));
                pGun->UpdateWait();
                if (pGun->CanFire()) {
                    if (pGun->StartFiring()) {
                        stage = 3;
                        return 1;
                    }
                    pGun->Reload();
                    return 1;
                }
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(player, 40.0f);
            return 1;
        case 2:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 1;
                return 1;
            }
            sm.GetActorOwner().SpinToYRot(Player_GetFeetPos());
            return 1;
        case 3: {
            EXVector aim = sm.GetActorOwner().GetAimPosForTarget(1);
            if (sm.GetActorOwner().IsFacingYRot(player, 40.0f)) {
                sm.GetActorOwner().SetAimingPos(aim);
                if (!pGun->UpdateFiring(sm.GetActorOwner(), aim, 1, 1)) {
                    if (holdPos) {
                        stage = 0;
                        holdPos = 0;
                        return 1;
                    }
                    stage = 1;
                    return 1;
                }
                return 1;
            }
            pGun->StopLoopingSFX();
            pGun->StopFiring();
            sm.GetActorOwner().SetTargetYRot(player, 40.0f);
            return 1;
        }
        case 4:
            flags = 0xb;
            sm.GetActorOwner().SetAnimType(0x900025b, 0, 1);
            stage = 5;
            return 1;
        case 5:
            if (sm.GetActorOwner().HasAnimEnded()) {
                pGun->m_Wait = 0.0f;
                holdPos = 1;
                stage = 1;
                return 1;
            }
            sm.GetActorOwner().SpinToYRot(Player_GetFeetPos());
            return 1;
        }
        return 1;
    }
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x1f:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1f)) StrikeCharacter(sm, 1, flags, pWeapon, 0, 1);
        return 1;
    case 0x20:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x20)) StrikeCharacter(sm, 0, flags, pWeapon, 0, 1);
        return 1;
    case 0x1e:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e)) {
            StrikeCharacter(sm, 1, flags, pWeapon, 0, 1);
            StrikeCharacter(sm, 0, flags, pWeapon, 0, 1);
        }
        return 1;
    }
    return 0;
}

// NEAR: logic read in full; target-position copies and the two sub-state switches laid out differently
signed char STATE_FightActor(StateMachine &sm)
{
    int i = 0;
    int &target = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    int &flags = sm.GetIntRef(i);
    int &move = sm.GetIntRef(i);
    int &strikesLeft = sm.GetIntRef(i);
    int &strike = sm.GetIntRef(i);
    float &chooseTimer = sm.GetFloatRef(i);
    float &closeTimer = sm.GetFloatRef(i);
    float &swapTimer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetStatus(0xc);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    sm.GetActorOwner().SetHeadTrackMode(0);
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        if (sm.GetActorOwner().CarryingHiddenWeapon()) {
            SM_GOTO(STATE_PullOutWeaponForActor, 0, 0, 0, 0);
            return 1;
        }
        stage = 0;
        chooseTimer = (EXRandf() + 3.0f) * EXBaseApp::NominalGameFrameRate();
        swapTimer = (EXRandf() * 0.0f + 1.0f) * EXBaseApp::NominalGameFrameRate();
        if (sm.GetActorOwner().IsCarryingGun()) {
            SM_GOTO(STATE_FightActorArmed, sm.m_Msg.m_Data[0], 0, 0, 0);
            return 1;
        }
        target = sm.m_Msg.m_Data[0];
        return 1;
    case MSG_EXIT:
        return 1;
    case MSG_EXT: {
        Actor *p = Actor::GetPtrFromID(target);
        if (p && p->GetBrainState() == STATE_PlayerControlDeath) target = 0;
        target = Actor_IsPredatorCloser(sm.GetActorOwner(), target);
        if (target == 1) {
            SM_GOTO(STATE_AttackPred, 0, 0, 0, 0);
            return 1;
        }
        chooseTimer -= EXBaseApp::GameFrameRateMul();
        if (!target) chooseTimer -= EXBaseApp::GameFrameRateMul();
        if (0.0f >= chooseTimer) {
            target = Actor_ChooseTarget(sm.GetActorOwner(), 0, 0.0f, 0);
            if (target == 1) {
                SM_GOTO(STATE_AttackPred, 0, 0, 0, 0);
                return 1;
            }
            chooseTimer = (EXRandf() + 1.0f) * 2.0f * EXBaseApp::NominalGameFrameRate();
        }
        Actor *pTarget = Actor::GetPtrFromID(target);
        if (!pTarget || pTarget->IsDead() || IsOutsideFightDist(sm.GetActorOwner(), pTarget, 0.1f)) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        EXVector tpos = (EXMaths::vec_xyzw)pTarget->GetFeetPos();
        int tpoly = pTarget->OrgLoc().GetPolyHandle();
        EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), tpos);
        float distSq = d.LengthXYZSquared();
        swapTimer -= EXBaseApp::GameFrameRateMul();
        if (0.0f > swapTimer) {
            if (EXRand32() % 121 == 0 && distSq > 36.0f && sm.GetActorOwner().CarryingMixedWeapons()) {
                SM_GOTO(STATE_SwapWeaponHandsActorCombat, 0, 0, 0, 0);
                return 1;
            }
        }
        switch (stage) {
        case 0:
            if (distSq > 3.24f) {
                sm.GetActorOwner().GotoPosPoly(tpos, tpoly);
                sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
                return 1;
            }
            sm.GetActorOwner().InvalidateGotoPos();
            stage = 1;
            move = 0;
            return 1;
        case 1:
            sm.GetActorOwner().TryToSpeakCombatVO(Actor::GetPtrFromID(target));
            sm.GetActorOwner().InvalidateGotoPos();
            if (distSq > 12.25f) {
                stage = 0;
                return 1;
            }
            switch (move) {
            case 0:
                if (0.25f > distSq && sm.GetActorOwner().CanMoveBack(1.0f)) {
                    move = 2;
                    break;
                }
                switch (EXRand32() % 61) {
                case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
                    stage = 2;
                    strikesLeft = pWeapon->m_NumStrikes;
                    strike = 0;
                    closeTimer = EXBaseApp::NominalGameFrameRate() * 4.0f;
                    break;
                case 8:
                    if (sm.GetActorOwner().CanStrafeLeft(1.5f)) move = 5;
                    break;
                case 9:
                    if (sm.GetActorOwner().CanStrafeRight(1.5f)) move = 6;
                    break;
                }
                break;
            case 2:
                if (distSq > 4.0f || !sm.GetActorOwner().CanMoveBack(1.0f)) move = 0;
                break;
            case 5:
                if (sm.GetActorOwner().HasAnimEnded() || !sm.GetActorOwner().CanStrafeLeft(1.0f)) move = 0;
                break;
            case 6:
                if (sm.GetActorOwner().HasAnimEnded() || !sm.GetActorOwner().CanStrafeRight(1.0f)) move = 0;
                break;
            default:
                move = 0;
                break;
            }
            switch (move) {
            case 0:
                sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
                if (sm.GetActorOwner().IsFacingYRot(tpos, 5.0f)) {
                    sm.GetActorOwner().InvalidateGotoPos();
                    sm.GetActorOwner().SetAnimType(0x4e000004, 0, 1);
                    return 1;
                }
                sm.GetActorOwner().SetTargetYRot(tpos, 5.0f);
                return 1;
            case 2:
                sm.GetActorOwner().InvalidateGotoPos();
                sm.GetActorOwner().SetAnimType(0x4e000009, 0, 1);
                sm.GetActorOwner().SpinToYRot(tpos);
                return 1;
            case 5:
                sm.GetActorOwner().InvalidateGotoPos();
                sm.GetActorOwner().SetAnimType(0x4e000007, 0, 1);
                sm.GetActorOwner().SpinToYRot(tpos);
                return 1;
            case 6:
                sm.GetActorOwner().InvalidateGotoPos();
                sm.GetActorOwner().SetAnimType(0x4e000008, 0, 1);
                sm.GetActorOwner().SpinToYRot(tpos);
                return 1;
            }
            return 1;
        case 2: {
            float reach = sm.GetActorOwner().GetSkelHash() == 0xd000091 ? 1.6f : 1.1f;
            if (reach * reach > distSq) {
                sm.GetActorOwner().ZeroMomentum();
                stage = 3;
                sm.GetActorOwner().InvalidateGotoPos();
                return 1;
            }
            closeTimer -= EXBaseApp::GameFrameRateMul();
            if (0.0f >= closeTimer || distSq > 16.0f) {
                stage = 0;
                sm.GetActorOwner().InvalidateGotoPos();
                return 1;
            }
            sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
            sm.GetActorOwner().SpinToYRot(tpos);
            return 1;
        }
        case 3: {
            sm.GetActorOwner().SpinToYRot(tpos);
            sm.GetActorOwner().SetAnimType(GetStrikeAnim(sm, pWeapon, strike, &flags), 0, 1);
            EXVector zero(0.0f, 0.0f, 0.0f, 0.0f);
            SendMsg_IncomingAttack(pTarget, pWeapon->m_Id, -1, sm.GetActorOwner().GetFeetPos(), zero, 0.0f);
            stage = 4;
            return 1;
        }
        case 4:
            sm.GetActorOwner().SpinToYRot(tpos);
            if (sm.GetActorOwner().HasAnimEnded()) {
                flags = 0;
                strikesLeft--;
                strike++;
                stage = (strikesLeft != 0) + 1;
                return 1;
            }
            return 1;
        case 5:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 1;
                return 1;
            }
            return 1;
        }
        return 1;
    }
    case MSG_SUSPEND:
        stage = 1;
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    AU_MSG_ATTRACT
    case MSG_INCOMING_ATTACK:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_INCOMING_ATTACK))
            AU_HandleIncomingAttack(sm, (StructMsg_IncomingAttack *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x1f:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1f)) StrikeCharacter(sm, 1, flags, pWeapon, 0, target);
        return 1;
    case 0x20:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x20)) StrikeCharacter(sm, 0, flags, pWeapon, 0, target);
        return 1;
    case 0x1e:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e)) {
            StrikeCharacter(sm, 1, flags, pWeapon, 0, target);
            StrikeCharacter(sm, 0, flags, pWeapon, 0, target);
        }
        return 1;
    }
    return 0;
}

// NEAR: logic read in full; player-delta copies and shared run/taunt tails laid out differently
signed char STATE_BrutePredAttack(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &flags = sm.GetIntRef(i);
    int &combo = sm.GetIntRef(i);
    int &unused = sm.GetIntRef(i);
    int &taunted = sm.GetIntRef(i);
    int &doTaunt = sm.GetIntRef(i);
    int &walk = sm.GetIntRef(i);
    int &hit = sm.GetIntRef(i);
    float &seenTimer = sm.GetFloatRef(i);
    float &roarTimer = sm.GetFloatRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    sm.GetActorOwner().SetHeadTrackMode(0);
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXIT:
        sm.GetActorOwner().SetDontDoHitReaction(0);
        return 1;
    case MSG_ENTER:
        combo = 0;
        stage = 0;
        roarTimer = (EXRandf() * 2.0f + 4.0f) * EXBaseApp::NominalGameFrameRate();
        timer = (EXRandf() + 2.0f) * EXBaseApp::NominalGameFrameRate();
        return 1;
    case MSG_EXT: {
        sm.GetActorOwner().SetDontDoHitReaction(1);
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        Actor &actor = sm.GetActorOwner();
        float timeout = actor.GetCombatTimeout();
        signed char seen = actor.WasPlayerSeen();
        if (seen) seenTimer = timeout;
        seenTimer -= EXBaseApp::GameFrameRateMul();
        if (seen && !Player_IsCloaked()) gAIControl.SetLastSeenPlayerPos(Player_GetFeetPos());
        gAIControl.SetLastSeenPlayerPos(Player_GetFeetPos());
        switch (stage) {
        case 0: {
            EXMaths::vec_xyzw d = VSub(gAIControl.PlayerNavLocLastSeen().FloorPos(), sm.GetActorOwner().GetFeetPos());
            if (d.LengthXYZSquared() > 16.0f) {
                sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
                sm.GetActorOwner().GotoLoc(gAIControl.PlayerNavLocLastSeen());
                return 1;
            }
            sm.GetActorOwner().InvalidateGotoPos();
            walk = 1;
            stage = 3;
            timer = (EXRandf() + 3.0f) * EXBaseApp::NominalGameFrameRate();
            unused = 0;
            taunted = 0;
            combo = 0;
            return 1;
        }
        case 1: {
            if (hit) {
                hit = 0;
                stage = 0;
            }
            if (roarTimer > 0.0f) roarTimer -= XApp::GameFrameRateMul();
            if (0.0f >= roarTimer) {
                sm.GetActorOwner().InvalidateGotoPos();
                sm.GetActorOwner().SetAnimType(0x9000379, 0, 1);
                roarTimer = (EXRandf() * 2.0f + 4.0f) * EXBaseApp::NominalGameFrameRate();
                stage = 7;
                return 1;
            }
            EXMaths::vec_xyzw d = VSub(gAIControl.PlayerNavLocLastSeen().FloorPos(), sm.GetActorOwner().GetFeetPos());
            float distSq = d.LengthXYZSquared();
            if (distSq > 256.0f) {
                stage = 0;
                return 1;
            }
            if (distSq > 16.0f) {
                sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
                sm.GetActorOwner().GotoLoc(gAIControl.PlayerNavLocLastSeen());
                return 1;
            }
            sm.GetActorOwner().InvalidateGotoPos();
            stage = 2;
            unused = 0;
            taunted = 0;
            combo = 0;
            return 1;
        }
        case 2: {
            EXMaths::vec_xyzw d = VSub(gAIControl.PlayerNavLocLastSeen().FloorPos(), sm.GetActorOwner().GetFeetPos());
            float distSq = d.LengthXYZSquared();
            if (distSq > 64.0f) {
                timer = (EXRandf() + 2.0f) * EXBaseApp::NominalGameFrameRate();
                hit = 0;
                stage = 1;
                return 1;
            }
            if (doTaunt) {
                doTaunt = 0;
                goto taunt;
            }
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(gAIControl.LastSeenPlayerPos(), 5.0f)) {
                timer = (EXRandf() + 2.0f) * EXBaseApp::NominalGameFrameRate();
                sm.GetActorOwner().TryToSpeakCombatVO(0);
                switch (EXRand32() & 7) {
                case 0:
                    taunted = 1;
                    stage = 6;
                    sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                    break;
                case 4:
                    if (!taunted) {
                        taunted = 1;
                        stage = 6;
                        sm.GetActorOwner().SetAnimType(0x9000379, 0, 1);
                    }
                    break;
                default:
                    if (36.0f > distSq && (XItemHandler_Player::Instance()->IsHanging() || gAIControl.DistOffNaviMeshLastSeen() > 1.0f))
                        goto taunt;
                    walk = 0;
                    stage = 3;
                    timer = (EXRandf() + 3.0f) * EXBaseApp::NominalGameFrameRate();
                    break;
                }
                if (stage == 2) {
                    sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                    return 1;
                }
                return 1;
            }
            timer -= XApp::GameFrameRateMul();
            if (0.0f >= timer) {
                timer = (EXRandf() + 2.0f) * EXBaseApp::NominalGameFrameRate();
                doTaunt = 1;
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(gAIControl.LastSeenPlayerPos(), 5.0f);
            return 1;
        }
        case 3: {
            timer -= XApp::GameFrameRateMul();
            if (0.0f >= timer) {
                sm.GetActorOwner().InvalidateGotoPos();
                timer = (EXRandf() + 2.0f) * EXBaseApp::NominalGameFrameRate();
                goto taunt;
            }
            EXMaths::vec_xyzw d = VSub(gAIControl.PlayerNavLocLastSeen().FloorPos(), sm.GetActorOwner().GetFeetPos());
            float distSq = d.LengthXYZSquared();
            if (distSq > 64.0f || gAIControl.DistOffNaviMeshLastSeen() > 1.1f) {
                stage = 2;
                return 1;
            }
            if (2.25f > distSq) {
                sm.GetActorOwner().InvalidateGotoPos();
                if (combo >= 0) {
                    if (combo <= 2) {
                        flags = 0x13;
                        int anims[3] = { 0x900051a, 0x900051b, 0x90003fc };
                        sm.GetActorOwner().SetAnimType(anims[combo], 0, 1);
                    } else if (combo == 3) {
                        if (EXRand32() & 1) {
                            flags = 0xa;
                            sm.GetActorOwner().SetAnimType(0x90003e6, 0, 1);
                        } else {
                            flags = 6;
                            sm.GetActorOwner().SetAnimType(0x9000521, 0, 1);
                        }
                    }
                }
                sm.GetActorOwner().SetDontDoHitReaction(1);
                if (++combo > 3) combo = 0;
                stage = 4;
                return 1;
            }
            sm.GetActorOwner().SetAnimType(walk ? 0x4e000003 : 0x4e000002, 0, 1);
            sm.GetActorOwner().GotoLoc(gAIControl.PlayerNavLocLastSeen());
            return 1;
        }
        case 4:
            if (sm.GetActorOwner().HasAnimEnded()) {
                if (!combo) {
                    stage = 2;
                    return 1;
                }
                walk = 0;
                timer = (EXRandf() + 3.0f) * EXBaseApp::NominalGameFrameRate();
                stage = 3;
                return 1;
            }
            sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
            return 1;
        case 5:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 2;
                return 1;
            }
            sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
            return 1;
        case 6:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 2;
                return 1;
            }
            return 1;
        case 7:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 1;
                return 1;
            }
            return 1;
        }
        return 1;
    taunt:
        sm.GetActorOwner().SetAnimType(0x90003e5, 0, 1);
        stage = 6;
        return 1;
    }
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR)) {
            StructMsg_AttackedByPredator *pMsg = (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0];
            if (pMsg->m_Attack >= 0x31 && pMsg->m_Attack <= 0x3e) {
                sm.GetActorOwner().SetDontDoHitReaction(0);
                combo = 3;
                doTaunt = 1;
                taunted = 1;
            }
            if (stage == 1) {
                hit = 1;
                AU_HandlePlayerAttacks(sm, pMsg);
                return 1;
            }
            if (stage == 3) walk = 1;
            AU_HandlePlayerAttacks(sm, pMsg);
        }
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x20:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x20)) StrikeCharacter(sm, 0, flags, pWeapon, 0, 1);
        return 1;
    case 0x1f:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1f)) StrikeCharacter(sm, 1, flags, pWeapon, 0, 1);
        return 1;
    case 0x1e:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e)) {
            StrikeCharacter(sm, 1, flags, pWeapon, 0, 1);
            StrikeCharacter(sm, 0, flags, pWeapon, 0, 1);
        }
        return 1;
    }
    return 0;
}

extern XGameWnd *gpGameWnd[4];  // TODO stand-in (GameWnd.obj)

// Patrol points 1..8 of the zombie bosses (retail expands this switch inline twice).
static __forceinline int ZombieBossPatrolLoc(int idx)
{
    int loc = 0;
    switch (idx) {
    case 1: loc = 0x42000086; break;
    case 2: loc = 0x42000087; break;
    case 3: loc = 0x42000088; break;
    case 4: loc = 0x42000089; break;
    case 5: loc = 0x4200008a; break;
    case 6: loc = 0x4200008b; break;
    case 7: loc = 0x4200008c; break;
    case 8: loc = 0x4200008d; break;
    }
    return loc;
}

// NEAR: logic read in full; player-delta copies and shared tails laid out differently
signed char STATE_ZombieBoss(StateMachine &sm)
{
    int i = 0;
    EXVector &patrolPos = sm.GetVectorRef(i);
    int &patrolPoly = sm.GetIntRef(i);
    int &patrolIdx = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    int &flags = sm.GetIntRef(i);
    int &newPoint = sm.GetIntRef(i);
    int &bossID = sm.GetIntRef(i);
    int &combo = sm.GetIntRef(i);
    float &chaseTimer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER: {
        stage = 0;
        bossID = g_ulZombieBossID % 3;
        g_ulZombieBossID++;
        patrolIdx = bossID * 2 + 1;
        int loc = ZombieBossPatrolLoc(patrolIdx);
        Actor &actor = sm.GetActorOwner();
        int poi = POI_ResolveLocation(loc, actor.GetNavZone(), actor.CanJump(), &actor.GetFeetPos());
        patrolPos = POI_Get(poi)->m_Pos;
        patrolPoly = POI_Get(poi)->m_Poly;
        return 1;
    }
    case MSG_EXIT:
        return 1;
    case MSG_RETURN:
        if (sm.m_Msg.m_Data[0] == (int)STATE_KnockBackZombie && EXRand32() % 3 == 0) {
            SM_GOSUB(STATE_ZombieDazed, 0, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_EXT: {
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        sm.GetActorOwner().TryToSpeakCombatVO(0);
        unsigned int frame = gpGameWnd[0]->m_Unk384;
        if (g_ulZombieBossFrame != frame) {
            g_ulZombieBossFrame = frame;
            if (g_fZombieBossNoAttackTimer > 0.0f) {
                g_fZombieBossNoAttackTimer -= XApp::GameFrameRateMul();
            } else {
                g_fZombieBossAttackTimer -= XApp::GameFrameRateMul();
                if (0.0f >= g_fZombieBossAttackTimer) {
                    if (++g_ulZombieBossAttacker >= 3) g_ulZombieBossAttacker = 0;
                    g_fZombieBossAttackTimer = EXBaseApp::NominalGameFrameRate() * 15.0f;
                }
            }
        }
        Actor &actor = sm.GetActorOwner();
        float timeout = actor.GetCombatTimeout();
        signed char seen = actor.WasPlayerSeen();
        if (seen) g_fZombieBossSeenTimeout = timeout;
        g_fZombieBossSeenTimeout -= EXBaseApp::GameFrameRateMul();
        if (seen && !Player_IsCloaked()) gAIControl.SetLastSeenPlayerPos(Player_GetFeetPos());
        if (g_fZombieBossSeenTimeout >= 0.0f) seen = 1;
        if (chaseTimer > 0.0f || !seen) {
            chaseTimer -= XApp::GameFrameRateMul();
            if (0.0f >= chaseTimer) {
                sm.GetActorOwner().SetInvinsibleCounter(EXBaseApp::NominalGameFrameRate() * 3);
                stage = 0;
            }
        }
        switch (stage) {
        case 0: {
            if (seen && g_ulZombieBossAttacker == bossID) goto attack;
            EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), patrolPos);
            if (9.0f > d.LengthXYZSquared()) {
                if (++patrolIdx > 8) patrolIdx = 1;
                int loc = ZombieBossPatrolLoc(patrolIdx);
                Actor &a = sm.GetActorOwner();
                int poi = POI_ResolveLocation(loc, a.GetNavZone(), a.CanJump(), &a.GetFeetPos());
                patrolPos = POI_Get(poi)->m_Pos;
                patrolPoly = POI_Get(poi)->m_Poly;
                newPoint = 1;
                return 1;
            }
            sm.GetActorOwner().GotoPosPoly(patrolPos, patrolPoly);
            if (newPoint) {
                newPoint = 0;
                sm.GetActorOwner().SetAnimType(0x900022e, 0, 1);
                return 1;
            }
            if (sm.GetActorOwner().GetAnimMode() == 0x900022e) {
                if (sm.GetActorOwner().HasAnimEnded()) {
                    sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
                    return 1;
                }
                return 1;
            }
            sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
            return 1;
        }
        case 1: {
            EXMaths::vec_xyzw d = VSub(gAIControl.PlayerNavLocLastSeen().FloorPos(), sm.GetActorOwner().GetFeetPos());
            if (d.LengthXYZSquared() > 16.0f) {
                sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
                sm.GetActorOwner().GotoLoc(gAIControl.PlayerNavLocLastSeen());
                return 1;
            }
            sm.GetActorOwner().InvalidateGotoPos();
            stage = 2;
            return 1;
        }
        case 2: {
            EXMaths::vec_xyzw d = VSub(gAIControl.PlayerNavLocLastSeen().FloorPos(), sm.GetActorOwner().GetFeetPos());
            if (d.LengthXYZSquared() > 64.0f) {
                stage = 1;
                return 1;
            }
            if (sm.GetActorOwner().IsFacingYRot(gAIControl.LastSeenPlayerPos(), 15.0f)) {
                switch (EXRand32() & 3) {
                case 0:
                    if (sm.GetActorOwner().CanStrafeLeft(1.5f)) {
                        stage = 3;
                        sm.GetActorOwner().SetDontDoHitReaction(1);
                        sm.GetActorOwner().SetAnimType(0x9000232, 0, 1);
                        return 1;
                    }
                    break;
                case 1:
                    if (sm.GetActorOwner().CanStrafeRight(1.5f)) {
                        stage = 3;
                        sm.GetActorOwner().SetDontDoHitReaction(1);
                        sm.GetActorOwner().SetAnimType(0x9000233, 0, 1);
                        return 1;
                    }
                    break;
                default:
                    combo = 0;
                    stage = 4;
                    break;
                }
                sm.GetActorOwner().InvalidateGotoPos();
                sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(gAIControl.LastSeenPlayerPos(), 15.0f);
            return 1;
        }
        case 3:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().SetDontDoHitReaction(0);
                stage = 2;
                return 1;
            }
            return 1;
        case 4: {
            EXMaths::vec_xyzw d = VSub(gAIControl.LastSeenPlayerPos(), sm.GetActorOwner().GetFeetPos());
            float distSq = d.LengthXYZSquared();
            if (distSq > 64.0f || gAIControl.DistOffNaviMeshLastSeen() > 1.1f) {
                stage = 2;
                return 1;
            }
            if (2.25f > distSq) {
                sm.GetActorOwner().InvalidateGotoPos();
                switch (combo) {
                case 0:
                    flags = 3;
                    sm.GetActorOwner().SetAnimType((EXRand32() & 1) ? 0x900054a : 0x9000548, 0, 1);
                    break;
                case 1:
                case 2:
                    flags = 3;
                    sm.GetActorOwner().SetAnimType(0x9000549, 0, 1);
                    break;
                case 3:
                    flags = 0x13;
                    sm.GetActorOwner().SetAnimType(0x9000384, 0, 1);
                    break;
                }
                sm.GetActorOwner().SetDontDoHitReaction(1);
                if (++combo > 3) combo = 0;
                stage = 5;
                return 1;
            }
            sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
            sm.GetActorOwner().GotoLoc(gAIControl.PlayerNavLocLastSeen());
            return 1;
        }
        case 5:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().SetDontDoHitReaction(0);
                stage = (combo != 0) * 2 + 2;
                return 1;
            }
            sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
            return 1;
        }
        return 1;
    attack:
        chaseTimer = EXBaseApp::NominalGameFrameRate() * 10.0f;
        stage = 1;
        sm.GetActorOwner().InvalidateGotoPos();
        return 1;
    }
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR)) {
            if (!stage) {
                chaseTimer = EXBaseApp::NominalGameFrameRate() * 10.0f;
                stage = 1;
                sm.GetActorOwner().InvalidateGotoPos();
                return 1;
            }
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        }
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x1f:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1f)) StrikeCharacter(sm, 1, flags, pWeapon, 0, 1);
        return 1;
    case 0x20:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x20)) StrikeCharacter(sm, 0, flags, pWeapon, 0, 1);
        return 1;
    case 0x1e:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e)) {
            StrikeCharacter(sm, 1, flags, pWeapon, 0, 1);
            StrikeCharacter(sm, 0, flags, pWeapon, 0, 1);
        }
        return 1;
    }
    return 0;
}

// NEAR: logic read in full; aim/target vector temporaries and shared turn tails laid out differently
signed char STATE_FightActorArmedVantage(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &flags = sm.GetIntRef(i);
    int &target = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    int &moving = sm.GetIntRef(i);
    int &prevPoi = sm.GetIntRef(i);
    float &chooseTimer = sm.GetFloatRef(i);
    float &timer1c = sm.GetFloatRef(i);
    float &timer20 = sm.GetFloatRef(i);
    float &switchTimer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xc);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    ActorCarryableGun *pGun = sm.GetActorOwner().GetGunPtr(0);
    if (!pGun) {
        sm.Return(0, 0, 0, 0);
        return 1;
    }
    GunCombatAnims anims = Actor_GetAimAnimsSet(sm.GetActorOwner(), pWeapon);
    int flagType = pGun->GetLocationType();
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT: {
        target = Actor_IsPredatorCloser(sm.GetActorOwner(), target);
        if (target == 1) {
            SM_GOTO(STATE_AttackPred, 0, 0, 0, 0);
            return 1;
        }
        chooseTimer -= EXBaseApp::GameFrameRateMul();
        switchTimer -= EXBaseApp::GameFrameRateMul();
        if (!target) chooseTimer -= EXBaseApp::GameFrameRateMul();
        if (0.0f >= chooseTimer) {
            int keep = 0;
            if (0.0f >= switchTimer && stage != 3) {
                switchTimer = (EXRandf() * 4.0f + 8.0f) * EXBaseApp::NominalGameFrameRate();
                if (gCurrentMap_GeoHash == 0x100006a) keep = target;
            }
            target = Actor_ChooseTarget(sm.GetActorOwner(), 0, 0.0f, keep);
            if (target == 1) {
                SM_GOTO(STATE_AttackPred, 0, 0, 0, 0);
                return 1;
            }
            chooseTimer = (EXRandf() + 3.0f) * EXBaseApp::NominalGameFrameRate();
        }
        Actor *pTarget = Actor::GetPtrFromID(target);
        if (!pTarget || pTarget->IsDead() || IsOutsideFightDist(sm.GetActorOwner(), pTarget, 0.1f)) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        pGun->UpdateWait();
        sm.GetActorOwner().SetHeadTrackMode(0);
        int old = poi;
        if (!moving) {
            Actor &actor = sm.GetActorOwner();
            if (old) {
                if (EXRand32() % 11 == 0)
                    poi = POI_UpdateNearestFlag(gAIControl.LastSeenPlayerPos(), flagType, old, prevPoi, actor.GetNavZone(),
                                                actor.CanJump(), 1, pWeapon->m_RangeMin);
            } else {
                poi = POI_GetNearestFlag(gAIControl.LastSeenPlayerPos(), flagType, 0, actor.GetNavZone(), actor.CanJump(), 1,
                                         pWeapon->m_RangeMin);
            }
        }
        if (old != poi) {
            if (old) POI_VendorUnbind(old, sm.GetActorOwner().m_RefID);
            prevPoi = old;
            if (poi) POI_VendorBind(poi, sm.GetActorOwner().m_RefID);
            stage = 0;
        }
        if (!poi) {
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(gAIControl.LastSeenPlayerPos(), 22.0f)) {
                sm.GetActorOwner().InvalidateGotoPos();
                sm.GetActorOwner().SetAimingPos(sm.GetActorOwner().GetAimPosForTarget(target));
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(gAIControl.LastSeenPlayerPos(), 22.0f);
            return 1;
        }
        EXVector stand = POI_VendorStandPos(poi, 0);
        int poly = POI_VendorStandPoly(poi, 0);
        switch (stage) {
        case 0: {
            sm.GetActorOwner().GotoPosPoly(stand, poly);
            sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
            moving = 1;
            EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), stand);
            if (0.25f > d.LengthXYZSquared()) {
                moving = 0;
                stage = 1;
                return 1;
            }
            return 1;
        }
        case 1: {
            EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), stand);
            if (d.LengthXYZSquared() > 16.0f) {
                stage = 0;
                return 1;
            }
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(Actor_GetTargetFeetPos(target), 22.0f)) {
                sm.GetActorOwner().SetAimingPos(sm.GetActorOwner().GetAimPosForTarget(target));
                if (pGun->CanFire()) {
                    stage = 2;
                    return 1;
                }
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(Actor_GetTargetFeetPos(target), 22.0f);
            return 1;
        }
        case 2:
            if (EXRand32() % 6 == 0) {
                WeaponProfile *pThrow = pWeapon;
                if (pThrow->m_AimType != 0x52000007)
                    pThrow = WeaponProfile::GetWeaponDataFromEntity(sm.GetActorOwner().GetProfile()->m_Weapon2);
                if (pThrow && pThrow->m_AimType == 0x52000007 && gAIControl.IsGrenadeAllowed() &&
                    !sm.GetActorOwner().AreThereFriendlyActorsNear(Actor_GetTargetFeetPos(target), 5.0f)) {
                    stage = 4;
                    return 1;
                }
            }
            if (stage == 2) {
                stage = pGun->StartFiring() ? 3 : 5;
                return 1;
            }
            return 1;
        case 3:
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(Actor_GetTargetFeetPos(target), 22.0f)) {
                sm.GetActorOwner().SetAimingPos(sm.GetActorOwner().GetAimPosForTarget(target));
                if (!pGun->UpdateFiring(sm.GetActorOwner(), sm.GetActorOwner().GetAimPosForTarget(target), anims.m_Anims[10], target)) {
                    stage = 1;
                    return 1;
                }
                return 1;
            }
            pGun->StopLoopingSFX();
            sm.GetActorOwner().SetTargetYRot(Actor_GetTargetFeetPos(target), 22.0f);
            return 1;
        case 4:
            sm.GetActorOwner().SetAnimType(s_aThrowAnims[EXRand32() % 1], 0, 1);
            stage = 6;
            return 1;
        case 5:
            sm.GetActorOwner().SetAnimType(anims.m_Anims[0], 1, 1);
            stage = 6;
            return 1;
        case 6:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 1;
                return 1;
            }
            return 1;
        }
        return 1;
    }
    case MSG_EXIT:
        if (poi) POI_VendorUnbind(poi, sm.GetActorOwner().m_RefID);
        pGun->StopFiring();
        return 1;
    case MSG_ENTER:
        target = sm.m_Msg.m_Data[0];
        chooseTimer = (EXRandf() + 3.0f) * EXBaseApp::NominalGameFrameRate();
        switchTimer = (EXRandf() * 4.0f + 6.0f) * EXBaseApp::NominalGameFrameRate();
        timer20 = (EXRandf() * 0.0f + 1.0f) * EXBaseApp::NominalGameFrameRate();
        timer1c = (EXRandf() * 0.40000004f + 0.8f) * EXBaseApp::NominalGameFrameRate();
        poi = 0;
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x19:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x19)) ThrowGrenade(sm, Actor_GetTargetFeetPos(target), 0, 0);
        return 1;
    AU_MSG_ATTRACT
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_INCOMING_ATTACK:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_INCOMING_ATTACK))
            AU_HandleIncomingAttack(sm, (StructMsg_IncomingAttack *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x25:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x25)) {
            SM_GOSUB(STATE_MonsterJump, 0, 0, 0, 0);
            return 1;
        }
        return 1;
    case 0x1f:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1f)) StrikeCharacter(sm, 1, flags, pWeapon, 1, target);
        return 1;
    case 0x20:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x20)) StrikeCharacter(sm, 0, flags, pWeapon, 1, target);
        return 1;
    case 0x1e:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e)) {
            StrikeCharacter(sm, 1, flags, pWeapon, 1, target);
            StrikeCharacter(sm, 0, flags, pWeapon, 1, target);
        }
        return 1;
    case 0x23:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x23)) pGun->Reload();
        return 1;
    case 0x2a:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x2a)) pGun->ForceRoundToEnd();
        return 1;
    }
    return 0;
}

// NEAR: logic read in full; player-delta copies and stand-position temporaries laid out differently
signed char STATE_Hunter1_PredAttack(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &flags = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    int &prevPoi = sm.GetIntRef(i);
    int &runPoi = sm.GetIntRef(i);
    int &grenade = sm.GetIntRef(i);
    int &holdPoi = sm.GetIntRef(i);
    float &strikeTimer = sm.GetFloatRef(i);
    float &holdTimer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    sm.GetActorOwner().SetHeadTrackMode(0);
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    ActorCarryableGun *pGun = sm.GetActorOwner().GetGunPtr(0);
    if (!pGun) {
        sm.Return(0, 0, 0, 0);
        return 1;
    }
    switch (sm.m_Msg.m_Type) {
    case MSG_EXIT:
        return 1;
    case MSG_ENTER:
        stage = 1;
        holdTimer = EXBaseApp::NominalGameFrameRate() * 10.0f;
        sm.GetActorOwner().SetIsDiscNotInHand(0);
        return 1;
    case MSG_RETURN:
        if (sm.m_Msg.m_Data[0] == (int)STATE_KnockBackHunter1) {
            holdPoi = 0;
            stage = 0xb;
            return 1;
        }
        if (stage == 4) {
            stage = 2;
            return 1;
        }
        if (stage == 10) stage = 7;
        return 1;
    case MSG_EXT: {
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        sm.GetActorOwner().TryToSpeakCombatVO(0);
        if (holdPoi) {
            holdTimer -= XApp::GameFrameRateMul();
            if (0.0f >= holdTimer) {
                holdTimer = (EXRandf() * 5.0f + 10.0f) * EXBaseApp::NominalGameFrameRate();
                holdPoi = 0;
            } else {
                EXVector stand = POI_VendorStandPos(poi, 0);
                EXMaths::vec_xyzw d = VSub(stand, Player_GetFeetPos());
                if (d.LengthXYZSquared() > 3025.0f) holdPoi = 0;
            }
        } else if (stage == 1 || stage == 2) {
            int old = poi;
            Actor &actor = sm.GetActorOwner();
            if (old)
                poi = POI_UpdateNearestFlag_Level(Player_GetFeetPos(), 0x4000, old, prevPoi, actor.GetNavZone(), actor.CanJump(), 1,
                                                  pWeapon->m_RangeMin);
            else
                poi = POI_GetNearestFlag_Level(Player_GetFeetPos(), 0x4000, 0, actor.GetNavZone(), actor.CanJump(), 1,
                                               pWeapon->m_RangeMin);
            if (old != poi) {
                if (old) POI_VendorUnbind(old, sm.GetActorOwner().m_RefID);
                prevPoi = old;
                if (poi) POI_VendorBind(poi, sm.GetActorOwner().m_RefID);
                stage = 1;
            }
            if (!poi) {
                stage = 2;
                holdPoi = 1;
            }
        }
        if (strikeTimer > 0.0f) strikeTimer -= XApp::GameFrameRateMul();
        switch (stage) {
        case 1: {
            if (0.0f >= strikeTimer) {
                EXMaths::vec_xyzw d = VSub(Player_GetFeetPos(), sm.GetActorOwner().GetFeetPos());
                if (6.25f > d.LengthXYZSquared()) {
                    float ry = sm.GetActorOwner().GetRotation().y;
                    if ((float)cos(ry) * d.z + (float)sin(ry) * d.x + d.y * 0.0f > 0.0f) {
                        sm.GetActorOwner().SetAnimType(s_aHunter1Strikes[EXRand32() & 1], 0, 1);
                        flags = 0x13;
                        strikeTimer = (EXRandf() + 1.0f) * EXBaseApp::NominalGameFrameRate() * 2.0f;
                        stage = 6;
                        return 1;
                    }
                }
            }
            EXVector stand = POI_VendorStandPos(poi, 0);
            sm.GetActorOwner().GotoPosPoly(stand, POI_VendorStandPoly(poi, 0));
            sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
            sm.GetActorOwner().SetCloak(1);
            EXMaths::vec_xyzw d = VSub(stand, sm.GetActorOwner().GetFeetPos());
            if (2.25f > d.LengthXYZSquared()) {
                stage = 2;
                holdPoi = 1;
                return 1;
            }
            return 1;
        }
        case 2:
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(Player_GetFeetPos(), 5.0f)) {
                sm.GetActorOwner().InvalidateGotoPos();
                pGun->UpdateWait();
                pGun->UpdateWait();
                pGun->UpdateWait();
                pGun->UpdateWait();
                if (!sm.GetActorOwner().IsDiscNotInHand() && pGun->CanFire()) {
                    pGun->StopFiring();
                    sm.GetActorOwner().SetAnimType(0x9000354, 0, 1);
                    stage = 4;
                    return 1;
                }
                sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(Player_GetFeetPos(), 5.0f);
            return 1;
        case 4:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 2;
                return 1;
            }
            return 1;
        case 6:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 1;
                return 1;
            }
            sm.GetActorOwner().SpinToYRot(Player_GetFeetPos());
            sm.GetActorOwner().m_InvinsibleCounter = 10.0f;
            return 1;
        case 7: {
            int old = runPoi;
            runPoi = AU_FindRunAwayPoint(sm, 0x2000, old, sm.GetActorOwner().GetNavZone(), 1);
            if (runPoi) {
                sm.GetActorOwner().SetAnimType(0x9000379, 0, 1);
                stage = 9;
            } else {
                stage = 1;
            }
            if (old) POI_VendorUnbind(old, sm.GetActorOwner().m_RefID);
            if (runPoi) POI_VendorBind(runPoi, sm.GetActorOwner().m_RefID);
            return 1;
        }
        case 8: {
            EXVector stand = POI_VendorStandPos(runPoi, 0);
            sm.GetActorOwner().GotoPosPoly(stand, POI_VendorStandPoly(runPoi, 0));
            sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
            EXMaths::vec_xyzw d = VSub(stand, sm.GetActorOwner().GetFeetPos());
            if (2.25f > d.LengthXYZSquared()) {
                stage = 12;
                sm.GetActorOwner().SetAnimType(0x9000379, 0, 1);
                return 1;
            }
            return 1;
        }
        case 9:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 8;
                return 1;
            }
            if (sm.GetActorOwner().GetAnimFrame() > 0.5f && !sm.GetActorOwner().IsCloaked()) sm.GetActorOwner().SetCloak(1);
            return 1;
        case 10:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 7;
                return 1;
            }
            sm.GetActorOwner().SpinToYRot(Player_GetFeetPos());
            sm.GetActorOwner().m_InvinsibleCounter = 10.0f;
            return 1;
        case 11:
            sm.GetActorOwner().SetAnimType(0x9000354, 0, 1);
            stage = 10;
            grenade = 1;
            return 1;
        case 12:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 1;
                return 1;
            }
            if (sm.GetActorOwner().GetAnimFrame() > 0.5f && sm.GetActorOwner().IsCloaked()) sm.GetActorOwner().SetCloak(0);
            return 1;
        }
        return 1;
    }
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR)) {
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
            holdPoi = 0;
            sm.GetActorOwner().SetCloak(0);
        }
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x19:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x19)) {
            if (grenade) {
                grenade = 0;
                ThrowGrenade(sm, Player_GetFeetPos(), 0x49000025, 0);
                return 1;
            }
            if (pWeapon->m_Id == 0x49000041) {
                EXVector hand = sm.GetActorOwner().GetHandPos(0);
                float a1 = EXRandf() * 1.5707964f - 0.7853982f + sm.GetActorOwner().GetRotation().y;
                float ry = sm.GetActorOwner().GetRotation().y;
                float a2 = EXRandf() * 1.5707964f - 0.7853982f + ry;
                EXVector zero(0.0f, 0.0f, 0.0f, 0.0f);
                EXVector dir((float)sin(a2), 0.0f, (float)cos(a1), 0.0f);
                XItemHandler_Bullet::CreateFromHash(hand, zero, dir, 0x49000041, 0, sm.GetActorOwner().m_RefID, 1, 1.0f);
                sm.GetActorOwner().SetHideCarryRight(1);
                sm.GetActorOwner().SetIsDiscNotInHand(1);
            }
        }
        return 1;
    case MSG_INCOMING_ATTACK:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_INCOMING_ATTACK))
            AU_HandleIncomingAttack(sm, (StructMsg_IncomingAttack *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x1f:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1f)) StrikeCharacter(sm, 1, flags, pWeapon, 0, 1);
        return 1;
    case 0x20:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x20)) StrikeCharacter(sm, 0, flags, pWeapon, 0, 1);
        return 1;
    case 0x1e:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e)) {
            StrikeCharacter(sm, 1, flags, pWeapon, 0, 1);
            StrikeCharacter(sm, 0, flags, pWeapon, 0, 1);
        }
        return 1;
    }
    return 0;
}

// NEAR: logic read in full; player-delta copies and the duplicated mine/strike checks laid out differently
signed char STATE_GotoLocationViaEvadePoints(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    int &evadePoi = sm.GetIntRef(i);
    int &result = sm.GetIntRef(i);
    int &locType = sm.GetIntRef(i);
    int &resumeStage = sm.GetIntRef(i);
    int &flags = sm.GetIntRef(i);
    float &mineTimer = sm.GetFloatRef(i);
    float &strikeTimer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetHeadTrackMode(0);
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT: {
        mineTimer -= XApp::GameFrameRateMul();
        strikeTimer -= XApp::GameFrameRateMul();
        switch (stage) {
        case 0:
            sm.GetActorOwner().GotoPOI(poi);
            sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
            if (4.0f > sm.GetActorOwner().SqDistToTarget()) {
                stage = 1;
            } else if (100.0f > sm.GetActorOwner().SqDistToTarget()) {
                EXMaths::vec_xyzw d = VSub(Player_GetFeetPos(), POI_Get(poi)->m_Pos);
                if (25.0f > d.LengthXYZSquared()) {
                    int old = poi;
                    Actor &actor = sm.GetActorOwner();
                    poi = POI_GetNearestType(actor.GetFeetPos(), locType, old, actor.GetNavZone(), actor.CanJump(), 1);
                    if (poi) {
                        POI_Unbind(old, sm.GetActorOwner().m_RefID);
                        if (!POI_VendorBind(poi, sm.GetActorOwner().m_RefID)) {
                            stage = 2;
                            result = 0;
                            return 1;
                        }
                    } else {
                        poi = old;
                    }
                    stage = 3;
                }
            }
            break;
        case 1:
            if (POI_AnimReady(poi, 1)) {
                if (!POI_ForceState(poi, 0, sm.GetActorOwner().m_RefID)) {
                    stage = 2;
                    result = 1;
                    return 1;
                }
                return 1;
            }
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            return 1;
        case 2:
            sm.Return(result, 0, 0, 0);
            return 1;
        case 3: {
            Actor &actor = sm.GetActorOwner();
            evadePoi = POI_GetNearestEvadeBetweenTarget(actor.GetFeetPos(), POI_Get(poi)->m_Pos, evadePoi, actor.GetNavZone(),
                                                        actor.CanJump());
            stage = evadePoi ? 4 : 0;
            return 1;
        }
        case 4:
            sm.GetActorOwner().GotoPOI(evadePoi);
            sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
            if (4.0f > sm.GetActorOwner().SqDistToTarget()) {
                stage = 3;
            } else if (100.0f > sm.GetActorOwner().SqDistToTarget()) {
                EXMaths::vec_xyzw d = VSub(Player_GetFeetPos(), POI_Get(evadePoi)->m_Pos);
                if (25.0f > d.LengthXYZSquared()) stage = 3;
            }
            break;
        case 5:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = resumeStage;
                return 1;
            }
            return 1;
        default:
            return 1;
        }
        if (0.0f >= mineTimer) {
            EXMaths::vec_xyzw d = VSub(Player_GetFeetPos(), sm.GetActorOwner().GetFeetPos());
            float distSq = d.LengthXYZSquared();
            if (distSq > 100.0f && 225.0f > distSq) {
                mineTimer = (EXRandf() + 1.0f) * EXBaseApp::NominalGameFrameRate() * 5.0f;
                SM_GOSUB(STATE_DropMine, 0, 0, 0, 0);
                return 1;
            }
        }
        if (0.0f >= strikeTimer) {
            EXMaths::vec_xyzw d = VSub(Player_GetFeetPos(), sm.GetActorOwner().GetFeetPos());
            if (4.0f > d.LengthXYZSquared()) {
                float ry = sm.GetActorOwner().GetRotation().y;
                if ((float)cos(ry) * d.z + (float)sin(ry) * d.x + d.y * 0.0f > 0.0f) {
                    resumeStage = stage;
                    stage = 5;
                    sm.GetActorOwner().SetAnimType(GetStrikeAnim(sm, pWeapon, 0, &flags), 0, 1);
                    flags = 0x13;
                    strikeTimer = (EXRandf() + 1.0f) * EXBaseApp::NominalGameFrameRate() * 2.0f;
                    return 1;
                }
            }
        }
        return 1;
    }
    case MSG_ENTER: {
        stage = 3;
        locType = sm.m_Msg.m_Data[0];
        Actor &actor = sm.GetActorOwner();
        poi = POI_GetNearestType(actor.GetFeetPos(), locType, 0, actor.GetNavZone(), actor.CanJump(), 1);
        if (!poi || !POI_VendorBind(poi, sm.GetActorOwner().m_RefID)) {
            stage = 2;
            result = 0;
            return 1;
        }
        mineTimer = (EXRandf() + 1.0f) * EXBaseApp::NominalGameFrameRate() * 5.0f;
        strikeTimer = (EXRandf() + 1.0f) * EXBaseApp::NominalGameFrameRate() * 2.0f;
        return 1;
    }
    AU_MSG_MUGGING AU_MSG_SOMEONE_UNDER_ATTACK AU_MSG_ATTACKED_BY_ACTOR AU_MSG_LOUD_NOISE AU_MSG_FIGHT_ME
    AU_MSG_DRIVE_BY AU_MSG_INCOMING_ATTACK AU_MSG_ATTRACT AU_MSG_SOMEONE_SEEN_PREDATOR AU_MSG_ATTACKED_BY_PREDATOR
    case 0x1f:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1f)) StrikeCharacter(sm, 1, flags, pWeapon, 1, 1);
        return 1;
    case 0x20:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x20)) StrikeCharacter(sm, 0, flags, pWeapon, 1, 1);
        return 1;
    case 0x1e:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e)) {
            StrikeCharacter(sm, 1, flags, pWeapon, 1, 1);
            StrikeCharacter(sm, 0, flags, pWeapon, 1, 1);
        }
        return 1;
    }
    return 0;
}

// NEAR: logic read in full; LengthXYZSquared not inlined (call vs retail inline sum), ebx/ebp swap, block layout
signed char STATE_RussianBossPredAttack(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &flags = sm.GetIntRef(i);
    int &combo = sm.GetIntRef(i);
    int &fastSkin = sm.GetIntRef(i);
    float &seenTimer = sm.GetFloatRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetHeadTrackMode(0);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        fastSkin = sm.GetActorOwner().GetSkin()->m_Hash == 0xd000096;
        seenTimer = sm.GetActorOwner().GetCombatTimeout();
        stage = 0;
        if (fastSkin)
            timer = (EXRandf() * 2.0f + 1.0f) * EXBaseApp::NominalGameFrameRate();
        else
            timer = (EXRandf() + 1.0f) * EXBaseApp::NominalGameFrameRate() * 3.0f;
        return 1;
    case MSG_EXT: {
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        Actor_CanSeeTarget(sm.GetActorOwner(), 1, seenTimer, sm.GetActorOwner().GetCombatTimeout());
        if (0.0f > seenTimer && sm.GetActorOwner().HasAnimEnded()) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        Actor &actor = sm.GetActorOwner();
        if (actor.GetCombatTimeout() * 0.5f > seenTimer)
            stage = 6;
        else if (stage == 6 && seenTimer > actor.GetCombatTimeout() * 0.5f)
            stage = 0;
        switch (stage) {
        case 6:
            sm.GetActorOwner().InvalidateGotoPos();
            if (sm.GetActorOwner().HasAnimEnded()) sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            return 1;
        case 0: {
            if (timer > 0.0f) timer -= XApp::GameFrameRateMul();
            if (0.0f > timer) {
                EXVector p = gAIControl.LastSeenPlayerPos();
                p.y = EXRandf() + p.y - 0.5f;
                if (sm.GetActorOwner().CanSeePoint(p, 0.0f)) {
                    sm.GetActorOwner().SetAnimType(0x9000354, 0, 1);
                    stage = 4;
                    sm.GetActorOwner().InvalidateGotoPos();
                }
                if (fastSkin)
                    timer = (EXRandf() * 2.0f + 1.0f) * EXBaseApp::NominalGameFrameRate();
                else
                    timer = (EXRandf() + 1.0f) * EXBaseApp::NominalGameFrameRate() * 3.0f;
                return 1;
            }
            if (!sm.GetActorOwner().CanGetToLocation(gAIControl.PlayerNavLocLastSeen())) {
                sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                return 1;
            }
            EXMaths::vec_xyzw d = VSub(gAIControl.PlayerNavLocLastSeen().FloorPos(), sm.GetActorOwner().GetFeetPos());
            if (d.LengthXYZSquared() > 16.0f) goto run;
            sm.GetActorOwner().InvalidateGotoPos();
            stage = 1;
            return 1;
        }
        case 1: {
            EXMaths::vec_xyzw d = VSub(gAIControl.PlayerNavLocLastSeen().FloorPos(), sm.GetActorOwner().GetFeetPos());
            float distSq = d.LengthXYZSquared();
            if (distSq > 64.0f) {
                stage = 0;
                return 1;
            }
            sm.GetActorOwner().TryToSpeakCombatVO(0);
            if (!sm.GetActorOwner().IsFacingYRot(gAIControl.LastSeenPlayerPos(), 5.0f)) {
                sm.GetActorOwner().SetTargetYRot(gAIControl.LastSeenPlayerPos(), 5.0f);
                return 1;
            }
            switch (EXRand32() % 11) {
            case 0:
                if (sm.GetActorOwner().CanStrafeLeft(1.5f)) {
                    sm.GetActorOwner().SetAnimType(0x4e000007, 0, 1);
                    stage = 5;
                    return 1;
                }
                break;
            case 1:
                if (sm.GetActorOwner().CanStrafeRight(1.5f)) {
                    sm.GetActorOwner().SetAnimType(0x4e000008, 0, 1);
                    stage = 5;
                    return 1;
                }
                break;
            case 2: {
                if (1.0f > gAIControl.DistOffNaviMeshLastSeen() && distSq > 16.0f &&
                    1.0f > fabsf(sm.GetActorOwner().GetFeetPos().y - gAIControl.LastSeenPlayerPos().y)) {
                    flags = 0xa;
                    sm.GetActorOwner().SetDontDoHitReaction(1);
                    XItemHandler_ScriptBase::FX_CreateLink(0x40004ab, sm.GetActorOwner().m_RefID, 0xe000015, 0xf, 0x10000e3);
                    sm.GetActorOwner().SetAnimType(0x90003e9, 0, 1);
                    stage = 3;
                    return 1;
                }
                Actor &a = sm.GetActorOwner();
                EXMaths::vec_xyzw d2 = VSub(gAIControl.LastSeenPlayerPos(), a.GetFeetPos());
                if (36.0f > d2.LengthXYZSquared() && XItemHandler_Player::Instance()->IsHanging()) {
                    XItemHandler_ScriptBase::FX_CreateLink(0x40004ab, a.m_RefID, 0xe000011, 0xf, 0x10000e3);
                    XItemHandler_ScriptBase::FX_CreateLink(0x40004ab, sm.GetActorOwner().m_RefID, 0xe000015, 0xf, 0x10000e3);
                    sm.GetActorOwner().SetAnimType(0x90003e5, 0, 1);
                    stage = 4;
                    return 1;
                }
                if (1.0f > gAIControl.DistOffNaviMeshLastSeen()) {
                    stage = 2;
                    return 1;
                }
                break;
            }
            }
            if (stage != 1) return 1;
            if (timer > 0.0f) timer -= XApp::GameFrameRateMul();
            if (0.0f > timer) {
                EXVector p = gAIControl.LastSeenPlayerPos();
                p.y = EXRandf() + p.y - 0.5f;
                if (sm.GetActorOwner().CanSeePoint(p, 0.0f)) {
                    sm.GetActorOwner().SetAnimType(0x9000354, 0, 1);
                    stage = 4;
                    sm.GetActorOwner().InvalidateGotoPos();
                }
                if (fastSkin)
                    timer = (EXRandf() * 2.0f + 1.0f) * EXBaseApp::NominalGameFrameRate();
                else
                    timer = (EXRandf() + 1.0f) * EXBaseApp::NominalGameFrameRate() * 3.0f;
                return 1;
            }
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            return 1;
        }
        case 2: {
            if (!sm.GetActorOwner().CanGetToLocation(gAIControl.PlayerNavLocLastSeen())) {
                stage = 1;
                return 1;
            }
            EXMaths::vec_xyzw d = VSub(gAIControl.PlayerNavLocLastSeen().FloorPos(), sm.GetActorOwner().GetFeetPos());
            float distSq = d.LengthXYZSquared();
            if (distSq > 36.0f || gAIControl.DistOffNaviMeshLastSeen() > 1.1f) {
                stage = 0;
                return 1;
            }
            if (2.25f > distSq) {
                sm.GetActorOwner().InvalidateGotoPos();
                if (combo >= 0) {
                    if (combo <= 2) {
                        flags = 0x13;
                        int anims[3] = { 0x90003fe, 0x90003fd, 0x90003fc };
                        sm.GetActorOwner().SetAnimType(anims[combo], 0, 1);
                    } else if (combo == 3) {
                        flags = 0xa;
                        XItemHandler_ScriptBase::FX_CreateLink(0x40004ab, sm.GetActorOwner().m_RefID, 0xe000015, 0xf, 0x10000e3);
                        sm.GetActorOwner().SetAnimType(0x90003e6, 0, 1);
                    }
                }
                sm.GetActorOwner().SetDontDoHitReaction(1);
                if (++combo > 3) combo = 0;
                stage = 3;
                return 1;
            }
            goto run;
        }
        case 3:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().SetDontDoHitReaction(0);
                stage = 1;
                return 1;
            }
            if (0.75f > sm.GetActorOwner().GetAnimFrame()) sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
            return 1;
        case 4:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 1;
                return 1;
            }
            return 1;
        case 5:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 1;
                return 1;
            }
            if (0.75f > sm.GetActorOwner().GetAnimFrame()) sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
            return 1;
        }
        return 1;
    run:
        sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
        sm.GetActorOwner().GotoLoc(gAIControl.PlayerNavLocLastSeen());
        return 1;
    }
    case MSG_EXIT:
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x19:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x19)) {
            EXVector zero(0.0f, 0.0f, 0.0f, 0.0f);
            EXVector hand = sm.GetActorOwner().GetHandPos(0);
            EXVector dir = VSub(sm.GetActorOwner().GetAimPosForTarget(1), hand);
            WeaponProfile *pShot = WeaponProfile::GetWeaponData(0x49000061);
            if (pShot) sm.GetActorOwner().PlaySFX(pShot->m_SfxFire, 0);
            XItemHandler_Bullet::CreateFromHash(hand, zero, dir, 0x49000061, 0, sm.GetActorOwner().m_RefID, 1, 1.0f);
        }
        return 1;
    case 0x1f:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1f)) StrikeCharacter(sm, 1, flags, pWeapon, 0, 1);
        return 1;
    case 0x20:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x20)) StrikeCharacter(sm, 0, flags, pWeapon, 0, 1);
        return 1;
    case 0x1e:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e)) {
            StrikeCharacter(sm, 1, flags, pWeapon, 0, 1);
            StrikeCharacter(sm, 0, flags, pWeapon, 0, 1);
        }
        return 1;
    case MSG_INCOMING_ATTACK:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_INCOMING_ATTACK))
            AU_HandleIncomingAttack(sm, (StructMsg_IncomingAttack *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

// NEAR: logic read in full; frame 0x10 larger (extra vector temps), grenade offset/scale schedule differs
signed char STATE_ServBossPredAttack(StateMachine &sm)
{
    int i = 0;
    EXVector &targetPos = sm.GetVectorRef(i);
    int &stage = sm.GetIntRef(i);
    int &flags = sm.GetIntRef(i);
    int &combo = sm.GetIntRef(i);
    int &strafed = sm.GetIntRef(i);
    int &engage = sm.GetIntRef(i);
    float &seenTimer = sm.GetFloatRef(i);
    float &targetRot = sm.GetFloatRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetHeadTrackMode(0);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        seenTimer = 180.0f;
        timer = EXBaseApp::NominalGameFrameRate() * 3.0f;
        combo = 1;
        stage = 0;
        engage = 1;
        return 1;
    case MSG_EXT: {
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        if (timer > 0.0f) timer -= XApp::GameFrameRateMul();
        signed char seen = Actor_CanSeeTarget(sm.GetActorOwner(), 1, seenTimer, 360.0f);
        if (sm.GetActorOwner().m_ConsecutiveHitCounter >= SubGroupAnims::GetSubGroupAnims(sm.GetActorOwner().m_SubGroupAnimIndex).m_Anims[11]) {
            sm.GetActorOwner().m_ConsecutiveHitCounter = 0;
            sm.GetActorOwner().m_InvinsibleCounter = (float)(int)(EXBaseApp::NominalGameFrameRate() * 2);
            combo = 3;
            stage = 4;
        }
        if (engage && seen) {
            engage = 0;
            stage = 2;
            return 1;
        }
        float health = sm.GetActorOwner().m_Health;
        if (health > 100.0f && AP_GetPtrFromIndex(sm.GetActorOwner().m_ProfileIndex)->m_Health > health)
            sm.GetActorOwner().ChangeHealth(XApp::GameFrameRateMul() * 0.02f, 2);
        switch (stage) {
        case 0:
            engage = 1;
            switch (EXRand32() % 7) {
            case 0:
                if (sm.GetActorOwner().CanMoveForward(1.5f)) {
                    sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
                    stage = 8;
                    return 1;
                }
                return 1;
            case 1:
                targetRot = EXRandf() * 3.1415927f;
                stage = 9;
                return 1;
            case 2:
                if (sm.GetActorOwner().CanMoveForward(1.5f)) {
                    sm.GetActorOwner().SetAnimType(0x900037c, 0, 1);
                    stage = 8;
                    return 1;
                }
                return 1;
            case 3:
                if (sm.GetActorOwner().CanMoveForward(1.5f)) {
                    sm.GetActorOwner().SetAnimType(0x900037d, 0, 1);
                    stage = 8;
                    return 1;
                }
                return 1;
            case 4:
                targetPos = Player_GetFeetPos();
                targetPos.x = EXRandf() * 4.0f + targetPos.x - 2.0f;
                targetPos.z = EXRandf() * 4.0f + targetPos.z - 2.0f;
                stage = 10;
                return 1;
            }
            if (0.0f >= timer && sm.GetActorOwner().CanMoveForward(3.0f)) {
                stage = 11;
                return 1;
            }
            return 1;
        case 11:
            timer = (EXRandf() + 2.0f) * EXBaseApp::NominalGameFrameRate();
            sm.GetActorOwner().SetAnimType(0x90004c9, 0, 1);
            stage = 8;
            return 1;
        case 8:
            if (sm.GetActorOwner().HasAnimEnded()) stage = 0;
            return 1;
        case 9:
        case 16:
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(targetRot, 5.0f)) {
                sm.GetActorOwner().InvalidateGotoPos();
                stage = 3;
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(targetRot, 5.0f);
            return 1;
        case 10:
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(targetPos, 5.0f)) {
                sm.GetActorOwner().InvalidateGotoPos();
                if (0.0f >= timer && sm.GetActorOwner().CanMoveForward(3.0f)) {
                    stage = 11;
                    return 1;
                }
                stage = 0;
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(targetPos, 5.0f);
            return 1;
        case 2: {
            if (!seen) {
                stage = 0;
                return 1;
            }
            EXMaths::vec_xyzw d = VSub(gAIControl.PlayerNavLocLastSeen().FloorPos(), sm.GetActorOwner().GetFeetPos());
            if (d.LengthXYZSquared() > 16.0f) {
                sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
                sm.GetActorOwner().GotoLoc(gAIControl.PlayerNavLocLastSeen());
                return 1;
            }
            sm.GetActorOwner().InvalidateGotoPos();
            stage = 3;
            return 1;
        }
        case 3: {
            if (!seen) {
                stage = 0;
                return 1;
            }
            sm.GetActorOwner().TryToSpeakCombatVO(0);
            EXMaths::vec_xyzw d = VSub(gAIControl.LastSeenPlayerPos(), sm.GetActorOwner().GetFeetPos());
            if (d.LengthXYZSquared() > 36.0f && 1.0f > gAIControl.DistOffNaviMeshLastSeen()) {
                stage = 2;
                return 1;
            }
            if (!sm.GetActorOwner().IsFacingYRot(gAIControl.LastSeenPlayerPos(), 5.0f)) {
                sm.GetActorOwner().SetTargetYRot(gAIControl.LastSeenPlayerPos(), 5.0f);
                return 1;
            }
            int r = EXRand32() % 5;
            if (strafed) r = 4;
            switch (r) {
            case 0:
                if (sm.GetActorOwner().CanStrafeLeft(1.5f)) {
                    sm.GetActorOwner().SetAnimType(0x4e00000b, 0, 1);
                    stage = 7;
                    strafed = 1;
                }
                break;
            case 1:
                if (sm.GetActorOwner().CanStrafeRight(1.5f)) {
                    sm.GetActorOwner().SetAnimType(0x4e00000c, 0, 1);
                    stage = 7;
                    strafed = 1;
                }
                break;
            default:
                strafed = 0;
                if (gAIControl.DistOffNaviMeshLastSeen() > 1.0f ||
                    fabsf(sm.GetActorOwner().GetFeetPos().y - gAIControl.LastSeenPlayerPos().y) > 1.5f) {
                    stage = 1;
                    return 1;
                }
                if (sm.GetActorOwner().CanGetToLocation(gAIControl.PlayerNavLocLastSeen()) && 1.0f > gAIControl.DistOffNaviMeshLastSeen()) {
                    stage = 4;
                    return 1;
                }
                break;
            }
            if (stage == 3) {
                sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                stage = 6;
                return 1;
            }
            return 1;
        }
        case 4: {
            EXMaths::vec_xyzw d = VSub(gAIControl.LastSeenPlayerPos(), sm.GetActorOwner().GetFeetPos());
            float distSq = d.LengthXYZSquared();
            if (distSq > 36.0f || gAIControl.DistOffNaviMeshLastSeen() > 1.1f) {
                stage = 2;
                return 1;
            }
            if (2.25f > distSq) {
                sm.GetActorOwner().InvalidateGotoPos();
                flags = 7;
                sm.GetActorOwner().SetAnimType(GetStrikeAnim(sm, pWeapon, combo, &flags), 0, 1);
                sm.GetActorOwner().SetDontDoHitReaction(1);
                stage = 5;
                if (++combo > 3) combo = 1;
                return 1;
            }
            sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
            sm.GetActorOwner().GotoLoc(gAIControl.PlayerNavLocLastSeen());
            return 1;
        }
        case 5:
            sm.GetActorOwner().SetDontDoHitReaction(1);
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().SetDontDoHitReaction(0);
                sm.GetActorOwner().m_InvinsibleCounter = 0.0f;
                stage = 3;
                return 1;
            }
            if (0.75f > sm.GetActorOwner().GetAnimFrame()) sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
            return 1;
        case 7:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 3;
                return 1;
            }
            sm.GetActorOwner().SpinToYRot(gAIControl.LastSeenPlayerPos());
            return 1;
        case 1:
            timer = (EXRandf() + 2.0f) * EXBaseApp::NominalGameFrameRate();
            sm.GetActorOwner().SetAnimType(0x90004c9, 0, 1);
            stage = 6;
            return 1;
        case 6:
            if (sm.GetActorOwner().HasAnimEnded()) stage = 3;
            return 1;
        }
        return 1;
    }
    case MSG_EXIT:
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x19:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x19)) {
            EXMaths::vec_xyzw d = VSub(Player_GetFeetPos(), sm.GetActorOwner().GetFeetPos());
            float dist = ForceSqrt(d.LengthXYZSquared());
            EXVector off(sin(sm.GetActorOwner().GetRotation().y), 0.0f, cos(sm.GetActorOwner().GetRotation().y), 0.0f);
            ThrowGrenade(sm, VAdd(off * dist, sm.GetActorOwner().GetFeetPos()), 0, 1);
        }
        return 1;
    case 0x1f:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1f)) StrikeCharacter(sm, 1, flags, pWeapon, 0, 1);
        return 1;
    case 0x20:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x20)) StrikeCharacter(sm, 0, flags, pWeapon, 0, 1);
        return 1;
    case 0x1e:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e)) {
            StrikeCharacter(sm, 1, flags, pWeapon, 0, 1);
            StrikeCharacter(sm, 0, flags, pWeapon, 0, 1);
        }
        return 1;
    case MSG_INCOMING_ATTACK:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_INCOMING_ATTACK) && stage != 5)
            AU_HandleIncomingAttack(sm, (StructMsg_IncomingAttack *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

// NEAR: logic read in full; POI/target-scan vector copies and CanJump hoist laid out differently
signed char STATE_MonsterSquad_Plasma(StateMachine &sm)
{
    int i = 0;
    EXVector &aimPos = sm.GetVectorRef(i);
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    int &prevPoi = sm.GetIntRef(i);
    int &holdPos = sm.GetIntRef(i);
    int &flags = sm.GetIntRef(i);
    float &moveTimer = sm.GetFloatRef(i);
    float &targetTimer = sm.GetFloatRef(i);
    int &target = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    sm.GetActorOwner().SetHeadTrackMode(0);
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    ActorCarryableGun *pGun = sm.GetActorOwner().GetGunPtr(0);
    if (!pGun) {
        sm.Return(0, 0, 0, 0);
        return 1;
    }
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        stage = 0;
        moveTimer = (EXRandf() * 3.0f + 5.0f) * EXBaseApp::NominalGameFrameRate();
        return 1;
    case MSG_EXIT:
    case MSG_SUSPEND:
        pGun->StopFiring();
        return 1;
    case MSG_RETURN:
        return 1;
    case MSG_EXT: {
        sm.GetActorOwner().SetIsInCombatWithPlayer(1);
        EXVector player = Player_GetFeetPos();
        if (stage == 0 || stage == 1) {
            int old = poi;
            Actor &actor = sm.GetActorOwner();
            if (old) {
                int np = POI_UpdateNearestFlag(player, 0x2000, old, prevPoi, actor.GetNavZone(), actor.CanJump(), 1, -1.0f);
                EXVector standOld = POI_VendorStandPos(poi, 0);
                EXVector standNew = POI_VendorStandPos(np, 0);
                EXMaths::vec_xyzw d1 = VSub(standOld, player);
                float len1 = ForceSqrt(d1.LengthXYZSquared());
                EXMaths::vec_xyzw d2 = VSub(standNew, player);
                float len2 = ForceSqrt(d2.LengthXYZSquared());
                if (len1 - 5.0f > len2) poi = np;
            } else {
                poi = POI_GetNearestFlag(player, 0x2000, 0, actor.GetNavZone(), actor.CanJump(), 1, -1.0f);
            }
            if (old != poi) {
                if (old) POI_VendorUnbind(old, sm.GetActorOwner().m_RefID);
                prevPoi = old;
                if (poi) POI_VendorBind(poi, sm.GetActorOwner().m_RefID);
                holdPos = 0;
                stage = 0;
            }
        }
        if (!poi) {
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            return 1;
        }
        EXVector stand = POI_VendorStandPos(poi, 0);
        if ((stage == 0 || stage == 1) && !holdPos) {
            EXMaths::vec_xyzw d = VSub(Player_GetFeetPos(), sm.GetActorOwner().GetFeetPos());
            if (4.0f > d.LengthXYZSquared()) stage = 4;
        }
        if (!target) {
            targetTimer -= XApp::GameFrameRateMul();
            if (0.0f >= targetTimer) {
                targetTimer = (EXRandf() + 1.0f) * EXBaseApp::NominalGameFrameRate() * 3.0f;
                EXVector pos = sm.GetActorOwner().GetFeetPos();
                for (Actor *p = Actor::GetFirst(pos, 10.0f, 0); p; p = p->GetNext(pos, 10.0f, 0)) {
                    if (p->GetProfile()->m_SubGroup != 0x4f000017) continue;
                    if (0.0f >= p->GetHealth()) continue;
                    if ((p->GetSurfaceFlags() >> 10) & 1) continue;
                    if (p->m_SM.GetState() == STATE_CaptivePredHanging) continue;
                    if (p->IsHanging()) continue;
                    EXMaths::vec_xyzw dp = VSub(pos, Player_GetFeetPos());
                    EXMaths::vec_xyzw da = VSub(pos, p->GetFeetPos());
                    if (da.LengthXYZSquared() > dp.LengthXYZSquared()) continue;
                    EXVector pp = p->GetFeetPos();
                    pp.y = EXRandf() + pp.y + 0.5f;
                    if (sm.GetActorOwner().CanSeePoint(pp, 0.0f)) target = p->m_RefID;
                }
            }
        } else {
            Actor *t = Actor::GetPtrFromID(target);
            if (!t || 0.0f >= t->GetHealth()) {
                target = 0;
            } else {
                EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), t->GetFeetPos());
                if (d.LengthXYZSquared() > 225.0f) target = 0;
            }
        }
        switch (stage) {
        case 0: {
            moveTimer -= XApp::GameFrameRateMul();
            if (0.0f > moveTimer) {
                pGun->m_Wait = 0.0f;
                holdPos = 1;
                moveTimer = (EXRandf() + 1.0f) * EXBaseApp::NominalGameFrameRate() * 3.0f;
                stage = 1;
                return 1;
            }
            sm.GetActorOwner().TryToSpeakCombatVO(0);
            EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), stand);
            if (0.25f > ForceSqrt(d.LengthXYZSquared())) {
                sm.GetActorOwner().InvalidateGotoPos();
                stage = 1;
                return 1;
            }
            if (sm.GetActorOwner().CanGetToPosPoly(POI_VendorStandPos(poi, 0), POI_VendorStandPoly(poi, 0))) {
                sm.GetActorOwner().GotoVendorPOI(poi, 0);
                sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
                return 1;
            }
            return 1;
        }
        case 1: {
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            EXVector look = player;
            if (target) {
                Actor *t = Actor::GetPtrFromID(target);
                if (t)
                    look = t->GetFeetPos();
                else
                    target = 0;
            }
            if (sm.GetActorOwner().IsFacingYRot(look, 40.0f)) {
                sm.GetActorOwner().TryToSpeakCombatVO(0);
                aimPos = target ? sm.GetActorOwner().GetAimPosForTarget(target) : sm.GetActorOwner().GetAimPosForTarget(1);
                sm.GetActorOwner().SetAimingPos(aimPos);
                pGun->UpdateWait();
                if (pGun->CanFire()) {
                    if (pGun->StartFiring()) {
                        stage = 3;
                        return 1;
                    }
                    pGun->Reload();
                    return 1;
                }
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(look, 40.0f);
            return 1;
        }
        case 3:
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            sm.GetActorOwner().SetAimingPos(aimPos);
            if (!pGun->UpdateFiring(sm.GetActorOwner(), aimPos, 1, 1)) {
                if (holdPos) {
                    stage = 0;
                    holdPos = 0;
                    return 1;
                }
                stage = 1;
                return 1;
            }
            return 1;
        case 2:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 1;
                return 1;
            }
            sm.GetActorOwner().SpinToYRot(Player_GetFeetPos());
            return 1;
        case 5:
            if (sm.GetActorOwner().HasAnimEnded()) {
                pGun->m_Wait = 0.0f;
                holdPos = 1;
                stage = 1;
                return 1;
            }
            sm.GetActorOwner().SpinToYRot(Player_GetFeetPos());
            return 1;
        case 4:
            sm.GetActorOwner().InvalidateGotoPos();
            flags = 0xb;
            sm.GetActorOwner().SetAnimType(0x900025b, 0, 1);
            stage = 5;
            return 1;
        }
        return 1;
    }
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x1f:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1f)) StrikeCharacter(sm, 1, flags, pWeapon, 0, 1);
        return 1;
    case 0x20:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x20)) StrikeCharacter(sm, 0, flags, pWeapon, 0, 1);
        return 1;
    case 0x1e:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e)) {
            StrikeCharacter(sm, 1, flags, pWeapon, 0, 1);
            StrikeCharacter(sm, 0, flags, pWeapon, 0, 1);
        }
        return 1;
    }
    return 0;
}

// NEAR: logic read in full; target-delta copies and shared stage-0/face tails laid out differently
signed char STATE_FightActorArmed(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &flags = sm.GetIntRef(i);
    int &target = sm.GetIntRef(i);
    float &chooseTimer = sm.GetFloatRef(i);
    float &decideTimer = sm.GetFloatRef(i);
    float &swapTimer = sm.GetFloatRef(i);
    float &switchTimer = sm.GetFloatRef(i);
    float &seeTimer = sm.GetFloatRef(i);
    int &canSee = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xc);
    sm.GetActorOwner().SetWatch(4);
    sm.GetActorOwner().SetAttrib(0x200, 1);
    WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
    ActorCarryableGun *pGun = sm.GetActorOwner().GetGunPtr(0);
    if (!pGun) {
        sm.Return(0, 0, 0, 0);
        return 1;
    }
    GunCombatAnims anims = Actor_GetAimAnimsSet(sm.GetActorOwner(), pWeapon);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT: {
        target = Actor_IsPredatorCloser(sm.GetActorOwner(), target);
        if (target == 1) {
            SM_GOTO(STATE_AttackPred, 0, 0, 0, 0);
            return 1;
        }
        chooseTimer -= EXBaseApp::GameFrameRateMul();
        switchTimer -= EXBaseApp::GameFrameRateMul();
        if (!target) chooseTimer -= EXBaseApp::GameFrameRateMul();
        if (0.0f >= chooseTimer) {
            int keep = 0;
            if (0.0f >= switchTimer && stage != 3) {
                switchTimer = (EXRandf() * 4.0f + 8.0f) * EXBaseApp::NominalGameFrameRate();
                if (gCurrentMap_GeoHash == 0x100006a) keep = target;
            }
            target = Actor_ChooseTarget(sm.GetActorOwner(), 0, 0.0f, keep);
            if (target == 1) {
                SM_GOTO(STATE_AttackPred, 0, 0, 0, 0);
                return 1;
            }
            chooseTimer = (EXRandf() + 3.0f) * EXBaseApp::NominalGameFrameRate();
        }
        Actor *pTarget = Actor::GetPtrFromID(target);
        if (!pTarget || pTarget->IsDead() || IsOutsideFightDist(sm.GetActorOwner(), pTarget, 0.1f)) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        pGun->UpdateWait();
        swapTimer -= EXBaseApp::GameFrameRateMul();
        if (0.0f > swapTimer && EXRand32() % 121 == 0 && sm.GetActorOwner().CarryingMixedWeapons()) {
            EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), Actor_GetTargetFeetPos(target));
            if (9.0f > d.LengthXYZSquared()) {
                SM_GOTO(STATE_SwapWeaponHandsActorCombat, 0, 0, 0, 0);
                return 1;
            }
        }
        sm.GetActorOwner().SetHeadTrackMode(0);
        if (seeTimer > 0.0f) {
            seeTimer -= XApp::GameFrameRateMul();
        } else {
            EXVector p = pTarget->GetFeetPos();
            p.y += 1.0f;
            if (sm.GetActorOwner().CanSeePoint(p, 0.0f)) {
                seeTimer = (EXRandf() + 1.0f) * 30.0f;
                canSee = 1;
            } else {
                seeTimer = (EXRandf() + 1.0f) * 10.0f;
                canSee = 0;
            }
        }
        switch (stage) {
        case 0: {
            sm.GetActorOwner().TryToSpeakCombatVO(pTarget);
            decideTimer -= EXBaseApp::GameFrameRateMul();
            if (0.0f >= decideTimer) {
                decideTimer = (EXRandf() * 0.40000004f + 0.8f) * EXBaseApp::NominalGameFrameRate();
                EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), Actor_GetTargetFeetPos(target));
                float distSq = d.LengthXYZSquared();
                if (9.0f > distSq && sm.GetActorOwner().GetSkelHash() != 0xd000091) {
                    int r = EXRand(2);
                    if (r) {
                        stage = 8;
                        return 1;
                    }
                }
                if (canSee && distSq >= 9.0f && pGun->CanFire()) {
                    stage = 2;
                    return 1;
                }
                int r = EXRand(4);
                if (r) {
                    float lo = pWeapon->m_RangeMin - 1.0f;
                    if (lo * lo > distSq) {
                        stage = 10;
                        return 1;
                    }
                }
                if (canSee) {
                    float hi = pWeapon->m_RangeMax + 1.0f;
                    if (!(distSq > hi * hi)) goto face;
                }
                stage = 4;
                return 1;
            }
        face:
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(Actor_GetTargetFeetPos(target), 22.0f)) {
                sm.GetActorOwner().SetAimingPos(sm.GetActorOwner().GetAimPosForTarget(target));
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(Actor_GetTargetFeetPos(target), 22.0f);
            return 1;
        }
        case 1:
            return 1;
        case 2:
            if (EXRand32() % 6 == 0) {
                WeaponProfile *pThrow = pWeapon;
                if (pThrow->m_AimType != 0x52000007)
                    pThrow = WeaponProfile::GetWeaponDataFromEntity(sm.GetActorOwner().GetProfile()->m_Weapon2);
                if (pThrow && pThrow->m_AimType == 0x52000007 && gAIControl.IsGrenadeAllowed() &&
                    !sm.GetActorOwner().AreThereFriendlyActorsNear(Actor_GetTargetFeetPos(target), 5.0f)) {
                    stage = 5;
                    return 1;
                }
            }
            if (stage == 2) {
                stage = pGun->StartFiring() ? 3 : 7;
                return 1;
            }
            return 1;
        case 3:
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(Actor_GetTargetFeetPos(target), 22.0f)) {
                sm.GetActorOwner().SetAimingPos(sm.GetActorOwner().GetAimPosForTarget(target));
                if (canSee && pGun->UpdateFiring(sm.GetActorOwner(), sm.GetActorOwner().GetAimPosForTarget(target), anims.m_Anims[10], target))
                    return 1;
                stage = 0;
                return 1;
            }
            pGun->StopLoopingSFX();
            sm.GetActorOwner().SetTargetYRot(Actor_GetTargetFeetPos(target), 22.0f);
            return 1;
        case 4:
            sm.GetActorOwner().GotoLoc(Actor::GetPtrFromID(target)->m_OrgLoc);
            if (canSee) {
                float hi = pWeapon->m_RangeMax - 1.0f;
                if (sm.GetActorOwner().SqDistToTarget() > hi * hi) {
                    sm.GetActorOwner().SetAnimType(anims.m_Anims[9], 1, 1);
                    return 1;
                }
            } else if (sm.GetActorOwner().SqDistToTarget() > 4.0f) {
                sm.GetActorOwner().SetAnimType(anims.m_Anims[9], 1, 1);
                return 1;
            }
            stage = 0;
            return 1;
        case 5:
            sm.GetActorOwner().SetAnimType(s_aThrowAnims[EXRand32() % 1], 0, 1);
            stage = 6;
            return 1;
        case 11:
            sm.GetActorOwner().SpinToYRot(Actor_GetTargetFeetPos(target));
            // fall through
        case 6:
            if (sm.GetActorOwner().HasAnimEnded()) {
                flags = 0;
                stage = 0;
                return 1;
            }
            return 1;
        case 7:
            sm.GetActorOwner().SetAnimType(anims.m_Anims[0], 1, 1);
            stage = 6;
            return 1;
        case 8: {
            EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), Actor_GetTargetFeetPos(target));
            float distSq = d.LengthXYZSquared();
            if (1.21f > distSq) {
                stage = 9;
                return 1;
            }
            if (distSq > 16.0f) {
                stage = 0;
                return 1;
            }
            sm.GetActorOwner().SetAnimType(anims.m_Anims[9], 1, 1);
            sm.GetActorOwner().SpinToYRot(Actor_GetTargetFeetPos(target));
            return 1;
        }
        case 9:
            sm.GetActorOwner().SetAnimType(GetNormalStrikeAnim(sm, pWeapon), 0, 1);
            stage = 11;
            flags = 3;
            pGun->ForceRoundToEnd();
            return 1;
        case 10: {
            EXMaths::vec_xyzw d = VSub(sm.GetActorOwner().GetFeetPos(), Actor_GetTargetFeetPos(target));
            float lo = pWeapon->m_RangeMin + 1.0f;
            if (lo * lo > d.LengthXYZSquared()) {
                if (sm.GetActorOwner().CanMoveBack(1.0f)) {
                    sm.GetActorOwner().SetAnimType(anims.m_Anims[8], 1, 1);
                    stage = 11;
                    return 1;
                }
                if (sm.GetActorOwner().CanStrafeLeft(1.0f)) {
                    sm.GetActorOwner().SetAnimType(anims.m_Anims[2], 1, 1);
                    stage = 11;
                    return 1;
                }
                if (sm.GetActorOwner().CanStrafeRight(1.0f)) {
                    sm.GetActorOwner().SetAnimType(anims.m_Anims[3], 1, 1);
                    stage = 11;
                    return 1;
                }
            }
            stage = 0;
            return 1;
        }
        }
        return 1;
    }
    case MSG_EXIT:
        pGun->StopFiring();
        sm.GetActorOwner().SetHeadTrackMode(0);
        return 1;
    case MSG_ENTER:
        target = sm.m_Msg.m_Data[0];
        chooseTimer = (EXRandf() + 3.0f) * EXBaseApp::NominalGameFrameRate();
        switchTimer = (EXRandf() * 4.0f + 6.0f) * EXBaseApp::NominalGameFrameRate();
        swapTimer = (EXRandf() * 0.0f + 1.0f) * EXBaseApp::NominalGameFrameRate();
        decideTimer = (EXRandf() * 0.40000004f + 0.8f) * EXBaseApp::NominalGameFrameRate();
        seeTimer = (EXRandf() + 1.0f) * 30.0f;
        canSee = 1;
        return 1;
    case MSG_SUSPEND:
        stage = 0;
        pGun->StopFiring();
        return 1;
    AU_MSG_ATTRACT
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x19:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x19)) ThrowGrenade(sm, Actor_GetTargetFeetPos(target), 0, 0);
        return 1;
    case 0x25:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x25)) {
            SM_GOSUB(STATE_MonsterJump, 0, 0, 0, 0);
            return 1;
        }
        return 1;
    case 0x1f:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1f)) StrikeCharacter(sm, 1, flags, pWeapon, 1, target);
        return 1;
    case 0x20:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x20)) StrikeCharacter(sm, 0, flags, pWeapon, 1, target);
        return 1;
    case 0x1e:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x1e)) {
            StrikeCharacter(sm, 1, flags, pWeapon, 1, target);
            StrikeCharacter(sm, 0, flags, pWeapon, 1, target);
        }
        return 1;
    case 0x23:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x23)) pGun->Reload();
        return 1;
    case 0x2a:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x2a)) pGun->ForceRoundToEnd();
        return 1;
    case MSG_INCOMING_ATTACK:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_INCOMING_ATTACK))
            AU_HandleIncomingAttack(sm, (StructMsg_IncomingAttack *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}
