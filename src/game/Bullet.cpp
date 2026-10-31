#include "XItemHandler_Bullet.h"
#include "XItemHandler_ScriptBase.h"
#include "XItemHandler_Player.h"
#include "XItem.h"
#include "XGameWnd.h"
#include "WeaponTable.h"
#include "GeoSectionManager.h"
#include "EXSwooshSysMgr.h"
#include "EXBaseSoundManager.h"
#include "EXRayCastTemp.h"
#include "EXGeoTexture.h"
#include "ActorOptions.h"
#include "FX_Gore.h"
#include "EXMemory.h"
#include "FEText.h"
#include "XApp.h"
#include "Actor.h"
#include "EX6DObject.h"
#include "EXQMath.h"
#include "ActorProfile.h"
#include "XItemHandler_VMO.h"
#include "XItemHandler_ClayPigeon.h"
#include "EXBaseSwooshSys.h"
#include "XItemHandler_Camera.h"
#include "EXGeoPathNode.h"
#include "EXGeoMap.h"
#include "VehiclePos.h"
#include "VehicleTypes.h"
#include "Vehicle.h"
#include "XItemHandler_AirVehicle.h"
#include "XItemHandler_CCTV.h"
#include "XItemHandler_Turret.h"
#include "ScriptRunner.h"
#include "EXGeoFile.h"
#include "EXItemAnimator_Entity.h"
#include "ActorUtil.h"
#include "AIControl.h"
#include "EXIntersection.h"
#include "EXSphere.h"
#include "EXLine.h"
#include "EXDatum.h"
#include "EXItemAnimator.h"
#include "EXRand.h"

extern XGameWnd *gpGameWnd[4];  // TODO stand-in (GameWnd.obj)

EX_IMPLEMENT_DYNCREATE_ROOT(XItemHandler_Bullet)

class Vehicle;
Vehicle *getItemOwnerVehicle(EXItem *pItem);

static EXVector s_Move;  // this frame's displacement, written by the Update_* steps and applied by DoUpdate

XItemHandler_Bullet::XItemHandler_Bullet()
{
    m_GeoSectionGeo = 0;
    m_GeoSectionItem = 0;
}

XItemHandler_Bullet::~XItemHandler_Bullet()
{
    if (m_GeoSectionGeo && m_GeoSectionItem) GeoSectionManager::Del(m_GeoSectionGeo, m_GeoSectionItem);
    if (m_pSwoosh) gpGameWnd[0]->SwooshSysMgr()->DeleteSystem(m_pSwoosh);
    m_SoundTag.RemoveAll(1);
}

s8 XItemHandler_Bullet::Initialise()
{
    if (!EXItemHandler::Initialise()) return 0;
    return 1;
}

EX_IMPLEMENT_DYNCREATE_ROOT(XItemHandler_Minesweeper)

XItemHandler_Minesweeper::~XItemHandler_Minesweeper() {}

s8 XItemHandler_Minesweeper::Initialise()
{
    if (!EXItemHandler::Initialise()) return 0;
    return 1;
}

// NEAR: esi/edi swapped between hash and the sound manager (register allocation only)
void DoImpactSFX(EXRayCastTemp *t)
{
    u32 hash;
    switch (t->m_face.GeoTexture()->GameFlags()) {
    case 1:
    case 16:
    case 64:
        return;
    case 4:
        hash = 0x1af00594;
        break;
    case 32:
        hash = 0x1af005af;
        break;
    case 2:
    default:
        hash = 0x1af005b4;
        break;
    }
    EXBaseSoundManager::GetSoundManager()->StartSound(hash, t->Intersect(), -1, 0);
}


// NEAR: plasma 0.5f-charge tail selects instead of branching; case 8 schedules Instance() before the call
float XItemHandler_Bullet::CalcDamage()
{
    WeaponProfile *p = WeaponProfile::GetWeaponData(m_WeaponHash);
    switch (m_WeaponHash) {
    case 0x49000008: {
        float d = p->GetBulletDamage();
        return d * (XItemHandler_Player::Instance()->GetWeaponUpgradeArray()[6] * 0.5f + 1.0f);
    }
    case 0x49000009: {
        float r;
        XItemHandler_Player *pl = XItemHandler_Player::Instance();
        int u = pl->GetWeaponUpgradeArray()[7];
        if (u >= 0 && u <= 1) {
            if (pl->GetPlasmaCharge() == 1.0f) r = 200.1f; else r = 100.1f;
        } else if (pl->GetPlasmaCharge() == 1.0f) r = 500.1f;
        else if (pl->GetPlasmaCharge() != 0.5f) r = 200.1f; else r = 300.1f;
        return r;
    }
    default:
        return p->GetBulletDamage();
    }
}

float XItemHandler_Bullet::CalcRange()
{
    WeaponProfile *p = WeaponProfile::GetWeaponData(m_WeaponHash);
    float r;
    switch (m_WeaponHash) {
    case 0x49000008:
        r = XItemHandler_Player::Instance()->GetWeaponUpgradeArray()[6] * 5.0f + 15.0f;
        break;
    case 0x49000009:
        r = XItemHandler_Player::Instance()->GetWeaponUpgradeArray()[7] * 10.0f + 30.0f;
        break;
    default:
        r = p->m_Range;
        break;
    }
    return r;
}

float XItemHandler_Bullet::CalcRadius()
{
    WeaponProfile *p = WeaponProfile::GetWeaponData(m_WeaponHash);
    float r;
    if (m_WeaponHash != 0x49000009) r = p->m_MineTriggerRadius;
    else {
        XItemHandler_Player *pl = XItemHandler_Player::Instance();
        if (pl->GetWeaponUpgradeArray()[7]) {
            if (pl->GetPlasmaCharge() == 0.0f) r = 2.0f;
            else r = 8.0f;
        } else r = pl->GetPlasmaCharge() == 0.0f ? 1.0f : 4.0f;
    }
    return r;
}

void XItemHandler_Bullet::Explode()
{
    if (m_pItem->ItemFlags() & 0x10) return;
    m_pItem->ItemFlags() = 0x10;
    WeaponProfile *p = WeaponProfile::GetWeaponData(m_WeaponHash);
    float dmg = CalcDamage();
    float rad = CalcRadius();
    XItemHandler_ScriptBase::Explosion_Create(m_pItem->m_position, p->m_ExplosionScript, (int)rad, (int)dmg,
                                              m_PredWeapon, m_WeaponHash, p->m_ExplosionType, 0, 0);
    EXVector pos = m_pItem->m_position - m_Velocity * 0.25f;
    Gore_ShootSplat(pos, m_Velocity, 8);
}


s8 XItemHandler_Minesweeper::DoUpdate()
{
    if (FlagChannels[m_FlagChannel]) {
        EXDListItem *pHead = theItemEnv.m_pItemLists[0].m_pHead;
        for (EXItem *p = pHead ? (EXItem *)((char *)pHead - 4) : 0; p; p = (EXItem *)p->NextItem()) {
            EXItemHandler *h = p->ItemHandler();
            if (h && h->IsKindOf(EXRUNTIME_CLASS(XItemHandler_Bullet))) p->ItemFlags() = 0x10;
        }
        m_pItem->ItemFlags() = 0x10;
    }
    return 1;
}

void Minesweeper()
{
    EXDListItem *pHead = theItemEnv.m_pItemLists[0].m_pHead;
    for (EXItem *p = pHead ? (EXItem *)((char *)pHead - 4) : 0; p; p = (EXItem *)p->NextItem()) {
        EXItemHandler *h = p->ItemHandler();
        if (h && h->IsKindOf(EXRUNTIME_CLASS(XItemHandler_Bullet))) {
            switch (((XItemHandler_Bullet *)h)->WeaponHash()) {
            case 0x49000007:
            case 0x4900000b:
            case 0x4900000c:
            case 0x4900000e:
            case 0x4900000f:
                p->ItemFlags() = 0x10;
                break;
            case 0x49000008:
                p->ItemFlags() = 0x10;
                XItemHandler_Player::Instance()->SetDiskInHand(1);
                break;
            }
        }
    }
}

void MinesweeperX(unsigned int hash)
{
    int n = 0;
    EXItem *pFirst = 0;
    EXDListItem *pHead = theItemEnv.m_pItemLists[0].m_pHead;
    for (EXItem *p = pHead ? (EXItem *)((char *)pHead - 4) : 0; p; p = (EXItem *)p->NextItem()) {
        EXItemHandler *h = p->ItemHandler();
        if (h && h->IsKindOf(EXRUNTIME_CLASS(XItemHandler_Bullet)) && ((XItemHandler_Bullet *)h)->WeaponHash() == hash) {
            if (!pFirst) pFirst = p;
            n++;
        }
    }
    if (n > 20) pFirst->ItemFlags() = 0x10;
}

XItem *XItemHandler_Bullet::Create(EXVector &a, EXVector &b, EXVector &c, unsigned short w, signed char s, unsigned int x, int y, float f)
{
    int h = WeaponProfile::GetWeaponryHashFromPredWeapon(w);
    return Create2(a, b, c, w, h, s, x, y, f);
}

XItem *XItemHandler_Bullet::CreateFromHash(EXVector &a, EXVector &b, EXVector &c, unsigned int h, signed char s, unsigned int x, int y, float f)
{
    return Create2(a, b, c, 0, h, s, x, y, f);
}

// The larger of a and b, by reference (retail's inline max).
static __forceinline const short &Bullet_Max(const short &a, const short &b) { return a >= b ? a : b; }

// Starts the fuse of every mine within radius of pos: 5 frames per unit of distance, at least 2,
// so a blast sets off a chain from the centre outwards. Mines already counting down keep theirs.
void DetonateNearbyMines(EXVector pos, float radius)
{
    float radiusSq = radius * radius;
    EXDListItem *pHead = theItemEnv.m_pItemLists[0].m_pHead;
    for (EXItem *p = pHead ? (EXItem *)((char *)pHead - 4) : 0; p; p = (EXItem *)p->NextItem()) {
        XItemHandler_Bullet *h = (XItemHandler_Bullet *)p->ItemHandler();
        if (!h || !h->IsKindOf(EXRUNTIME_CLASS(XItemHandler_Bullet)) || !h->IsMine()) continue;
        EXVector d = pos - p->Position();
        if (!(d.y * d.y + d.z * d.z + d.x * d.x < radiusSq)) continue;
        EXVector e = pos - p->Position();
        float dist = EXSqrt(e.y * e.y + e.z * e.z + e.x * e.x);
        short minN = 2;
        short n = (short)(int)(dist * 5.0f);
        h->SetCountdown(Bullet_Max(n, minN));
    }
}

// A projectile stuck in place (a speargun spear): fades out over its last frame of life and is
// deleted. While its target's rag doll can be dragged, the spear pins the target's second body
// half a step back along the flight path (made heavy, motionless) with the root body made light.
void XItemHandler_Bullet::Update_StaticTimeout()
{
    m_Life -= XApp::FrameLength();
    if (m_Life < 1.0f) {
        m_pItem->RenderInfo().RenderFlagsRef().BitSet(2);
        const EXVector &rgb = m_pItem->RenderInfo().RGBAMult();
        EXVector c(rgb.x, rgb.y, rgb.z, m_Life);
        m_pItem->RenderInfo().SetRGBAMult(c);
    }
    if (m_Life <= 0.0f) m_pItem->ItemFlags() = 0x10;
    Actor *pTarget = Actor::GetPtrFromID(m_Targets[0]);
    if (!pTarget || pTarget->DontDragMeSpear()) return;
    EX6DAnim *pAnim = (EX6DAnim *)pTarget->GetItem()->m_pItemPhysics;
    if (!pAnim || !pAnim->GetRuntimeClass()->IsDerivedFrom(EXRUNTIME_CLASS(EX6DAnim))) return;
    if (pAnim->BodyCount() <= 1 || !pAnim->m_bodies[1]) return;
    pAnim->Body(0)->SetMass(1.0f);
    EXVector half = m_Velocity * 0.5f;
    pAnim->Body(1)->m_pos = m_pItem->m_position - half;
    pAnim->Body(1)->m_velocity.SetZero();
    ((EX6DRigidBody *)pAnim->Body(1))->AngularVelocity().SetZero();
    pAnim->Body(1)->SetMass(50.0f);
}

// Ricochet: reverse with some jitter, step off the surface and face the new heading.
void XItemHandler_Bullet::Bounce(EXVector &pos)
{
    m_Velocity = -m_Velocity;
    m_Velocity.x += g_EXRandClass.Randf() - 0.5f;
    m_Velocity.z += g_EXRandClass.Randf() - 0.5f;
    m_Velocity.y += g_EXRandClass.Randf() - 0.5f;
    m_Velocity.Normalise();
    m_pItem->SetPosition(pos + m_Velocity * 0.05f);
    float yaw = (float)atan2(m_Velocity.x, m_Velocity.z);
    float pitch = (float)atan2(m_Velocity.y, m_Velocity.LengthXZ());
    m_pItem->SetRotation(EXQuaternion(-pitch, yaw, 0.0f, 0.0f));
    EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00244, pos, -1, 0);
}

static __forceinline void Bullet_Normalise(EXVector &v)
{
    float len = v.Length();
    if (len > EX_EPSILON_UNIT) v *= 1.0f / len;
}

static __forceinline const float &Bullet_Min(const float &a, const float &b) { return a <= b ? a : b; }

// Two-stage projectile: stage one slows to a stop, then either becomes a normal round (fire type 3)
// or turns towards the stage target and accelerates up to the weapon's speed.
void XItemHandler_Bullet::Update_TwoStage()
{
    if (m_Type == 8) {
        m_Speed -= XApp::GameFrameRateMul() * 0.03f;
        if (m_Speed <= 0.0f) {
            WeaponProfile *pWeapon = WeaponProfile::GetWeaponData(m_WeaponHash);
            if (pWeapon && pWeapon->m_FireType == 3) {
                m_Speed = pWeapon->m_ThrowSpeed;
                m_Type = 3;
            } else {
                m_Speed = 0.0f;
                m_Type = 9;
                m_Velocity = m_StageTarget - m_pItem->Position();
                Bullet_Normalise(m_Velocity);
            }
            if (pWeapon->m_Stage2Animator) {
                EXItemAnimator *pAnim = m_pItem->GetAnimator(pWeapon->m_ProjectileEntity);
                if (pAnim) {
                    EXGeoFile *pGeo = pAnim->GeoFile();
                    m_pItem->DeleteAnimator(pAnim);
                    m_pItem->AddAnimator(EXItemAnimator::CreateFrom(pGeo, pWeapon->m_Stage2Animator));
                }
            }
        }
    }
    if (m_Type == 9) {
        WeaponProfile *pWeapon = WeaponProfile::GetWeaponData(m_WeaponHash);
        m_Speed += XApp::GameFrameRateMul() * 0.01f;
        m_Speed = Bullet_Min(m_Speed, pWeapon->m_ThrowSpeed);
    }
    s_Move = m_Velocity * m_Speed * XApp::GameFrameRateMul();
    float yaw = (float)atan2(m_Velocity.x, m_Velocity.z);
    float pitch = (float)atan2(m_Velocity.y, m_Velocity.LengthXZ());
    m_pItem->SetRotation(EXQuaternion(-pitch, yaw, 0.0f, 0.0f));
}

static __forceinline EXQuaternion Bullet_Quat(const EXMaths::quat_xyzw &r)
{
    EXQuaternion q;
    q.x = r.x; q.y = r.y; q.z = r.z; q.w = r.w;
    return q;
}

static __forceinline float Bullet_Clamp(float t, float hi)
{
    if (t >= hi) return hi;
    if (0.0f >= t) return 0.0f;
    return t;
}

// Smart disc: chase the locked-on targets in turn (the player's chest or an actor's pelvis);
// when they are all dead it flies straight on (type 5).
void XItemHandler_Bullet::Update_SmartDisk()
{
    if (m_Targets[0] == 1) {
        m_StageTarget = XItemHandler_Player::Instance()->GetChestPos();
    } else {
        Actor *pTarget = Actor::GetPtrFromID(m_Targets[0]);
        if (!pTarget || (pTarget->IsDead() && !pTarget->CanRipBossHead())) {
            m_Targets[0] = m_Targets[1];
            m_Targets[1] = m_Targets[2];
            m_Targets[2] = m_Targets[3];
            m_Targets[3] = 0;
            if (!m_Targets[0]) {
                m_Targets[0] = 0;
                m_Type = 5;
            }
            return;
        }
        m_StageTarget = pTarget->GetPelvisPos();
    }
    EXVector diff = m_StageTarget - m_pItem->Position();
    float dist = qsqrtf(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
    EXVector dir = diff;
    dir.Normalise();
    EXMaths::quat_xyzw from, to;
    from.SetRotation(m_Velocity, 4);
    to.SetRotation(dir, 4);
    float t;
    if (m_WeaponHash == 0x49000008)       // HT_Weaponry_Predator_SmartDiskThrown
        t = Bullet_Clamp(XApp::GameFrameRateMul() / dist, 1.0f);
    else if (m_WeaponHash == 0x49000041)  // HT_Weaponry_HunterSmartDisc
        t = Bullet_Clamp(XApp::GameFrameRateMul() / dist, 0.1f);
    else
        t = 0.1f;
    EXQuaternion q = Bullet_Quat(from.slerp(to, t));
    q.GetRotation(m_Velocity, 4);
    s_Move = m_Velocity * m_Speed * XApp::GameFrameRateMul();
    float yaw = (float)atan2(m_Velocity.x, m_Velocity.z);
    float pitch = (float)atan2(m_Velocity.y, m_Velocity.LengthXZ());
    m_pItem->SetRotation(EXQuaternion(-pitch, yaw, 0.0f, 0.0f));
}

// Returning disc: home in on the thrower's hand (the player) or chest (a hunter), and catch it on arrival.
void XItemHandler_Bullet::Update_Returning()
{
    EXVector target;
    if (m_WeaponHash == 0x49000041) {  // HT_Weaponry_HunterSmartDisc
        Actor *pOwner = Actor::GetPtrFromID(m_OwnerID);
        if (pOwner) {
            target = pOwner->GetFeetPos();
            target.y += 1.0f;
        } else {
            target.SetZero();
        }
    } else {
        EXDatumTemp d;
        if (gpPlayerItem[0]->GetAnimatorDatum(0x10000006, d, 1, 0))  // HT_AnimDatum_Pred_Right_Hand
            target = d.m_position;
        else
            target = gpPlayerItem[0]->Position();
    }
    EXVector diff = target - m_pItem->Position();
    float dist = qsqrtf(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
    if (dist <= m_Speed * XApp::GameFrameRateMul()) {
        m_pItem->ItemFlags() = 0x10;
        m_SoundTag.RemoveAll(1);
        if (m_WeaponHash == 0x49000041) {
            Actor *pOwner = Actor::GetPtrFromID(m_OwnerID);
            if (pOwner) {
                pOwner->CatchDisc();
                EXBaseSoundManager::GetSoundManager()->StartSound(0x1af0036d, pOwner->GetFeetPos(), -1, 0);
            }
        } else {
            XItemHandler_Player::Instance()->SetDiskInHand(1);
            Actor::OverridePlayerCloak();
            EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00238, target, -1, 0);
        }
    } else {
        EXVector dir = diff;
        dir.Normalise();
        EXMaths::quat_xyzw from, to;
        from.SetRotation(m_Velocity, 4);
        to.SetRotation(dir, 4);
        float t = XApp::GameFrameRateMul() / dist;
        t = t >= 1.0f ? 1.0f : (0.0f >= t ? 0.0f : t);
        EXQuaternion q = Bullet_Quat(from.slerp(to, t));
        q.GetRotation(m_Velocity, 4);
        s_Move = m_Velocity * m_Speed * XApp::GameFrameRateMul();
    }
    float yaw = (float)atan2(m_Velocity.x, m_Velocity.z);
    float pitch = (float)atan2(m_Velocity.y, m_Velocity.LengthXZ());
    m_pItem->SetRotation(EXQuaternion(-pitch, yaw, 0.0f, 0.0f));
}

// Nearest hitable actor along a shot, other than the shooter. On-screen actors are tested against
// their collision shapes; off-screen ones against a sphere round the pelvis, and only hit half the time.
// Returns the distance to the hit (5000 when none) with the hit point in hit and the actor id in id.
float CheckActorsOnLine(EXLineSeg &seg, EXVector &hit, unsigned int &id, unsigned int shooter)
{
    float best = 5000.0f;
    id = 0;
    Actor *pShooter = Actor::GetPtrFromID(shooter);
    EXVector centre = seg.m_Origin + seg.m_Direction * 0.5f;
    float radius = seg.m_Direction.LengthXYZ() + 3.0f;
    for (Actor *p = Actor::GetFirst(centre, radius, 0); p; p = p->GetNext(centre, radius, 0)) {
        if (p->m_HitCountdown <= 0.0f && (!p->IsDead() || p->CanRipBossHead()) && !p->m_ActorFlags.m_bHolographic
            && p != pShooter) {
            EXVector pelvis = p->GetPelvisPos();
            EXSphere sphere;
            sphere.Set(pelvis, p->m_CollisionSphereRadius);
            if (EXIntersection(seg, sphere)) {
                if (p->m_StateFlags.m_bOnScreen) {
                    if (p->CheckCollision(seg, 0.1f, hit)) {
                        EXVector d = hit - seg.m_Origin;
                        float dist = EXSqrt(d.y * d.y + d.z * d.z + d.x * d.x);
                        if (dist < best) {
                            id = p->m_RefID;
                            best = dist;
                        }
                    }
                } else {
                    const int &r = g_EXRandClass.Rand32() & 1;
                    if (!r) {
                        hit = pelvis;
                        EXVector d = pelvis - seg.m_Origin;
                        float dist = EXSqrt(d.y * d.y + d.z * d.z + d.x * d.x);
                        if (dist < best) {
                            id = p->m_RefID;
                            best = dist;
                        }
                    }
                }
            }
        }
    }
    return best;
}

// Homing round: track the target (the player, or where he was last seen while cloaked; an actor's
// pelvis while alive), turning up to 20% of the way per frame while it is roughly ahead. Explodes
// within a metre.
void XItemHandler_Bullet::Update_Homing()
{
    WeaponProfile *pWeapon = WeaponProfile::GetWeaponData(m_WeaponHash);
    if (m_Targets[0] == 1) {
        if (pWeapon && pWeapon->m_IgnoresCloak) {
            m_StageTarget = XItemHandler_Player::Instance()->GetChestPos();
        } else if (Player_IsCloaked()) {
            m_StageTarget = gAIControl.LastSeenPlayerPos();
            m_StageTarget.y += 1.0f;
        } else {
            m_StageTarget = XItemHandler_Player::Instance()->GetChestPos();
        }
    } else {
        Actor *pTarget = Actor::GetPtrFromID(m_Targets[0]);
        if (pTarget && !pTarget->IsDead()) m_StageTarget = pTarget->GetPelvisPos();
    }
    EXVector dir = m_StageTarget - m_pItem->Position();
    if (dir.LengthXYZSquared() < 1.0f) {
        Explode();
        return;
    }
    dir.Normalise();
    if (dir.Dot(m_Velocity) > 0.5f) {
        EXMaths::quat_xyzw from, to;
        from.SetRotation(m_Velocity, 4);
        to.SetRotation(dir, 4);
        EXQuaternion q = Bullet_Quat(from.slerp(to, 0.2f));
        q.GetRotation(m_Velocity, 4);
    }
    s_Move = m_Velocity * m_Speed * XApp::GameFrameRateMul();
    float yaw = (float)atan2(m_Velocity.x, m_Velocity.z);
    float pitch = (float)atan2(m_Velocity.y, m_Velocity.LengthXZ());
    m_pItem->SetRotation(EXQuaternion(-pitch, yaw, 0.0f, 0.0f));
}

static __forceinline s8 Bullet_IsA(EXItemHandler *pHandler, const EXRuntimeClass *pClass)
{
    const EXRuntimeClass *p = pHandler->GetRuntimeClass();
    do {
        if (p == pClass) return 1;
        p = p->m_pBaseClass;
    } while (p);
    return 0;
}

static __forceinline EXVector Bullet_Gravity() { return EXVector(0.0f, -0.01f, 0.0f, 0.0f); }

static __forceinline void Bullet_NormaliseDot(EXVector &v)
{
    float len = EXSqrt(v.Dot(v));
    if (len > EX_EPSILON_UNIT) v *= 1.0f / len;
}

// Ballistic round (grenades, mines, spears): falls under gravity. Actors on the path set it off; the
// map stops it (a spear sticks, a mine arms, anything else explodes); contoured surfaces bounce mines,
// except vehicles, turrets, cameras, air vehicles and scripts expecting this weapon, which set it off.
void XItemHandler_Bullet::Update_Gravity()
{
    EXVector drop = Bullet_Gravity() * XApp::GameFrameRateMul() * XApp::GameFrameRateMul() * 0.5f;
    EXLineSeg seg;
    seg.m_Origin = m_pItem->Position();
    seg.m_Direction = s_Move + drop;
    unsigned int id = 0;
    EXVector hit;
    CheckActorsOnLine(seg, hit, id, m_OwnerID);
    if (m_pItem->Position().y < -1000.0f) {
        m_pItem->ItemFlags() = 0x10;
        return;
    }
    if (id) {
        m_pItem->SetPosition(hit);
        Explode();
        return;
    }
    EXRayCastTemp t;
    t.Set(m_pItem->Position(), s_Move + drop, 0, 0, 0, 0);
    t.m_mask = 8;
    theItemEnv.DoRayCastToMap(t, 0);
    if (t.m_bHit) {
        m_pItem->SetPosition(t.Intersect());
        switch (m_WeaponHash) {
        case 0x4900000b:  // HT_Weaponry_Predator_EMPMine
        case 0x4900000c:  // HT_Weaponry_Predator_SonicMine
        case 0x4900000e:  // HT_Weaponry_Predator_APMine
        case 0x4900000f:  // HT_Weaponry_Predator_IncendiaryMine
        case 0x4900005c:
        case 0x4900005d:
        case 0x4900005e:
        case 0x4900005f: {
            EXVector n = t.m_normal;
            float yaw = (float)atan2(n.x, n.z);
            float pitch = (float)atan2(n.y, n.LengthXZ());
            m_pItem->SetRotation(EXQuaternion(1.5707964f - pitch, yaw, 0.0f, 0.0f));
            EXGeoFile *pGeo = EXGeoFile::LoadGeoFile(0x1000016, 0, 0);  // HT_File_Pred_FX
            EXItemAnimator_Entity *pBall = EXItemAnimator_Entity::NewItem();
            if (pBall) {
                if (m_WeaponHash > 0x49000010)
                    pBall->Initialise(pGeo, 0x2000031);  // HT_Entity_RedBall
                else
                    pBall->Initialise(pGeo, 0x2000272);
                pBall->ClrHasRender();
                pBall->RenderInfo().RenderFlagsRef().BitSet(3);
                float r = WeaponProfile::GetWeaponData(((XItemHandler_Bullet *)m_pItem->ItemHandler())->WeaponHash())
                              ->GetMineTriggerRadius();
                pBall->SetScale(EXVector(r, r, r, 0.0f));
                pBall->RenderFlags().m_flags |= 3;
                m_pItem->AddAnimator(pBall);
                m_pTriggerBall = pBall;
            }
            m_Speed = 0.0f;
            m_Type = 2;
            EXBaseSoundManager::GetSoundManager()->StartSound(0x1af0022f, m_pItem->Position(), -1, 0);
            break;
        }
        case 0x49000007:  // HT_Weaponry_Predator_Speargun
            m_Life = 10.0f;
            m_Type = 7;
            m_Speed = 0.0f;
            EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00244, t.Intersect(), -1, 0);
            return;
        default:
            Explode();
            return;
        }
    } else {
        theItemEnv.DoRayCastToContoured(t);
        if (t.m_bHit) {
            if (m_WeaponHash > 0x49000010) {
                Explode();
                return;
            }
            EXItem *pOwner = 0;
            if (t.m_pAnimator && (pOwner = t.m_pAnimator->GetOwnerItem()) != 0) {
                if (getItemOwnerVehicle(pOwner)) {
                    Explode();
                    return;
                }
                EXItemHandler *pHandler = pOwner->ItemHandler();
                if (pHandler) {
                    if (Bullet_IsA(pHandler, EXRUNTIME_CLASS(XItemHandler_ScriptRunner))) {
                        if (WeaponProfile::GetWeaponData(m_WeaponHash)->m_DamageClass
                            == (int)((XItemHandler_ScriptRunner *)pHandler)->m_DamageType) {
                            Explode();
                            return;
                        }
                    } else if (Bullet_IsA(pHandler, EXRUNTIME_CLASS(XItemHandler_Turret))
                               || Bullet_IsA(pHandler, EXRUNTIME_CLASS(XItemHandler_CCTV))
                               || Bullet_IsA(pHandler, EXRUNTIME_CLASS(XItemHandler_AirVehicle))) {
                        Explode();
                        return;
                    }
                }
            }
            EXVector n(t.m_normal.x, t.m_normal.y, t.m_normal.z, 0.0f);
            if (pOwner) *(EXMaths::vec_xyzw *)&n = (const EXMaths::mat_44 &)pOwner->mtx() * n;
            Bullet_NormaliseDot(n);
            Bullet_Normalise(m_Velocity);
            float d = (m_Velocity.z * n.z + m_Velocity.y * n.y + m_Velocity.w * n.w + m_Velocity.x * n.x) * 2.0f;
            m_Velocity -= n * d;
            Bullet_Normalise(m_Velocity);
            m_pItem->SetPosition(t.Intersect());
            m_Speed *= 0.4f;
            s_Move = m_Velocity * m_Speed * XApp::GameFrameRateMul();
        }
        m_pItem->SetPosition(m_pItem->Position() + s_Move + drop);
    }
    s_Move = Bullet_Gravity() * XApp::GameFrameRateMul() + s_Move * (1.0f / XApp::GameFrameRateMul());
    m_Speed = s_Move.LengthXYZ();
    m_Velocity = s_Move;
    Bullet_Normalise(m_Velocity);
    if (m_WeaponHash == 0x49000007) {  // HT_Weaponry_Predator_Speargun
        float yaw = (float)atan2(m_Velocity.x, m_Velocity.z);
        float pitch = (float)atan2(m_Velocity.y, m_Velocity.LengthXZ());
        m_pItem->SetRotation(EXQuaternion(-pitch, yaw, 0.0f, 0.0f));
    }
}

static __forceinline const float &Bullet_MaxF(const float &a, const float &b) { return a >= b ? a : b; }

static __forceinline EXVector Bullet_NodePos(const EXGeoPath &path, int i)
{
    const EXVector3 &p = path.Node(i)->Position();
    return EXVector(p.x, p.y, p.z, 0.0f);
}

static __forceinline EXVector Bullet_CatmullRom(const EXVector &p0, const EXVector &p1, const EXVector &p2,
                                                const EXVector &p3, float t)
{
    float t2 = t * t;
    float t3 = t2 * t;
    return (p1 * 2.0f + (p2 - p0) * t + (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2
            + (p1 * 3.0f - p2 * 3.0f + p3 - p0) * t3) * 0.5f;
}

static __forceinline EXVector Bullet_CatmullRomTangent(const EXVector &p0, const EXVector &p1, const EXVector &p2,
                                                       const EXVector &p3, float t)
{
    float t2 = t * t;
    return ((p2 - p0) + (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * (t * 2.0f)
            + (p1 * 3.0f - p2 * 3.0f + p3 - p0) * (t2 * 3.0f)) * 0.5f;
}

// Armed mine: anything but its owner coming within the trigger radius (vehicles by their length,
// turrets and cameras too) sets it off. The trigger ball shows in tech vision. Hover mines patrol
// their map path on a Catmull-Rom spline at constant speed.
void XItemHandler_Bullet::Update_Mine()
{
    EXItemAnimator_Entity *pBall = m_pTriggerBall;
    bool bTrigger = false;
    if (pBall) {
        if (XItemHandler_Player::Instance()->GetVisionMode() == 3)
            pBall->SetHasRender();
        else
            pBall->ClrHasRender();
    }
    float radius = WeaponProfile::GetWeaponData(((XItemHandler_Bullet *)m_pItem->ItemHandler())->WeaponHash())
                       ->GetMineTriggerRadius();
    if (m_WeaponHash != 0x49000055 && (m_WeaponHash < 0x4900005c || m_WeaponHash > 0x4900005f)) {  // HT_Weaponry_HoverMine
        for (Actor *p = Actor::GetFirst(1); p && !bTrigger; p = p->GetNext(1)) {
            if (m_OwnerID == p->m_RefID) continue;
            EXVector d;
            if (p->m_StateFlags.m_bRagDoll) {
                d = p->GetPelvisPos() - m_pItem->Position();
            } else {
                if (p->IsDead() || p->m_ActorFlags.m_bHolographic) continue;
                d = p->GetFeetPos() - m_pItem->Position();
            }
            if (radius * radius > d.LengthXYZSquared()) bTrigger = true;
        }
    }
    if (!m_bFromPlayer && !bTrigger) {
        EXVector pp = gpPlayerItem[0]->Position();
        if (m_PathIndex > -1) pp.y += 1.0f;
        EXVector d = pp - m_pItem->Position();
        if (radius * radius > d.LengthXYZSquared()) bTrigger = true;
    }
    for (Vehicle *v = Vehicle::GetFirst(); !bTrigger && v; v = v->GetNext()) {
        EXVector vp;
        v->getCurrPos().get(vp);
        EXVector d = vp - m_pItem->Position();
        const float &len = Bullet_MaxF(v->getTypeObject()->getZDistToFront(), v->getTypeObject()->getZDistToRear());
        if (len * len > d.LengthXYZSquared()) bTrigger = true;
    }
    for (XItem *pTurret = gTurretList.GetFirst(); pTurret; pTurret = gTurretList.GetNext(pTurret)) {
        EXVector d = pTurret->Position() - m_pItem->Position();
        if (radius * radius > d.LengthXYZSquared()) bTrigger = true;
    }
    for (XItem *pCam = gCCTVList.GetFirst(); pCam; pCam = gCCTVList.GetNext(pCam)) {
        EXVector d = pCam->Position() - m_pItem->Position();
        if (radius * radius > d.y * d.y + d.z * d.z + d.x * d.x) bTrigger = true;
    }
    if (bTrigger) {
        Explode();
        EXBaseSoundManager::GetSoundManager()->StartSound(0x1af0022f, m_pItem->Position(), -1, 0);
        return;
    }
    if (m_PathIndex > -1) {
        const EXGeoPath &path = (*theItemEnv.GetPaths(EXMapOn(-1)))[m_PathIndex];
        int prev = m_PathNode - 1;
        if (prev < 0) prev = path.NumNodes() - 1;
        int next = m_PathNode + 1;
        if (next >= path.NumNodes()) next -= path.NumNodes();
        int next2 = m_PathNode + 2;
        if (next2 >= path.NumNodes()) next2 -= path.NumNodes();
        EXVector p0 = Bullet_NodePos(path, prev);
        EXVector p1 = Bullet_NodePos(path, m_PathNode);
        EXVector p2 = Bullet_NodePos(path, next);
        EXVector p3 = Bullet_NodePos(path, next2);
        EXVector pos = Bullet_CatmullRom(p0, p1, p2, p3, m_PathT);
        float speed = Bullet_CatmullRomTangent(p0, p1, p2, p3, m_PathT).LengthXYZ();
        m_pItem->SetPosition(pos);
        m_PathT += XApp::GameFrameRateMul() / speed * 0.1f;
        if (m_PathT >= 1.0f) {
            m_PathT -= 1.0f;
            m_PathNode = next;
        }
    }
}

// Spawns a projectile at pos heading along dir. target is the second-stage aim point of two-stage
// weapons, lockTarget the actor a homing round (fire type 3) tracks, speedScale scales the weapon's
// speed. Smart discs take the camera's lock-ons, plasma bolts their range to the plasma lock, and
// thrown weapons with no speed become mines (snapped to a hover path when dropped on one).
XItem *XItemHandler_Bullet::Create2(EXVector &pos, EXVector &target, EXVector &dir, unsigned short predWeapon,
                                    unsigned int hash, signed char bFromPlayer, unsigned int owner, int lockTarget,
                                    float speedScale)
{
    XItem *pItem = XItem::CreateItem(0, 0, EXRUNTIME_CLASS(XItemHandler_Bullet), EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    XItemHandler_Bullet *h = (XItemHandler_Bullet *)pItem->ItemHandler();
    pItem->GetExtRenderInfo();
    h->m_Velocity = dir;
    h->m_Velocity.w = 0.0f;
    h->m_Velocity.Normalise();
    h->m_pSwoosh = 0;
    h->m_Bounces = 0;
    h->m_Targets[1] = 0;
    h->m_Targets[2] = 0;
    h->m_Targets[3] = 0;
    h->m_WeaponHash = hash;
    WeaponProfile *pWeapon = WeaponProfile::GetWeaponData(hash);
    if (!pWeapon) return 0;
    u32 entity = pWeapon->m_ProjectileEntity;
    if (pWeapon->m_FireType == 3)
        h->m_Targets[0] = lockTarget;
    else
        h->m_Targets[0] = 0;
    h->m_bFromPlayer = bFromPlayer;
    h->m_OwnerID = owner;
    u32 swoosh = pWeapon->m_Swoosh;
    if (pWeapon->m_FlightSound) h->m_SoundTag.AddTrackingSound(pWeapon->m_FlightSound, pos, 0.0f, 0.0f);
    h->m_pTriggerBall = 0;
    h->m_HideChannel = 0;
    h->m_Range = 100000.0f;
    h->m_Countdown = -1;
    EXGeoFile *pGeo;
    switch (h->m_WeaponHash) {
    case 0x49000008: {  // HT_Weaponry_Predator_SmartDiskThrown
        pGeo = EXGeoFile::GetGeoFile(0x1000013);  // HT_File_PRED_Weapons
        if (!entity) entity = 0x2000016;  // HT_Entity_SmartDisk
        switch (XItemHandler_Player::Instance()->GetWeaponUpgradeArray()[6]) {
        case 1:
            entity = 0x4000507;  // HT_Script_Bullet_AVP_Shuriken
            swoosh = 0x1900001e;
            break;
        case 2:
            entity = 0x400064d;
            swoosh = 0x1900001d;
            break;
        }
        if (XGetCamera(0)->m_Mode != 2) {
            h->m_Targets[3] = XGetCamera(0)->m_PlasmaID;
            h->m_Targets[2] = XGetCamera(0)->m_HitList[0];
            h->m_Targets[1] = XGetCamera(0)->m_HitList[1];
            h->m_Targets[0] = XGetCamera(0)->m_HitList[2];
            int n = 3;
            do {
                if (!h->m_Targets[0]) {
                    h->m_Targets[0] = h->m_Targets[1];
                    h->m_Targets[1] = h->m_Targets[2];
                    h->m_Targets[2] = h->m_Targets[3];
                    h->m_Targets[3] = 0;
                }
            } while (--n);
        }
        XItemHandler_Player::Instance()->SetDiskInHand(0);
        break;
    }
    case 0x49000009:  // HT_Weaponry_Predator_Plasmacaster
        pGeo = EXGeoFile::GetGeoFile(0x1000016);  // HT_File_Pred_FX
        if (XGetCamera(0)->m_PlasmaID > 0) {
            Actor *pTarget = Actor::GetPtrFromID(XGetCamera(0)->m_PlasmaID);
            if (pTarget) {
                EXVector d = pos - pTarget->GetChestPosApprox();
                h->m_Range = d.LengthXYZ() + 0.2f;
            }
        }
        break;
    case 0x4900000b:  // HT_Weaponry_Predator_EMPMine
    case 0x4900000c:  // HT_Weaponry_Predator_SonicMine
    case 0x4900000e:  // HT_Weaponry_Predator_APMine
    case 0x4900000f:  // HT_Weaponry_Predator_IncendiaryMine
        MinesweeperX(h->m_WeaponHash);
    case 0x49000055:  // HT_Weaponry_HoverMine
    case 0x4900005c:
    case 0x4900005d:
    case 0x4900005e:
    case 0x4900005f:
        pGeo = EXGeoFile::LoadGeoFile(0x100001d, 0, 0);  // HT_File_Mines
        break;
    case 0x49000007:  // HT_Weaponry_Predator_Speargun
    case 0x49000041:  // HT_Weaponry_HunterSmartDisc
        pGeo = EXGeoFile::GetGeoFile(0x1000013);  // HT_File_PRED_Weapons
        break;
    case 0x49000024:  // HT_Weaponry_Drone_GrenadeLauncher
    case 0x4900004d:  // HT_Weaponry_ThrowingStar
    case 0x49000050:  // HT_Weaponry_Skull
        pGeo = EXGeoFile::GetGeoFile(0x100003c);  // HT_File_Actor_Carry
        break;
    default:
        pGeo = EXGeoFile::GetGeoFile(0x1000016);  // HT_File_Pred_FX
        break;
    }
    if (swoosh) {
        h->m_pSwoosh = (EXEffectsSys *)gpGameWnd[0]->SwooshSysMgr()->CreateSystem(swoosh);
        if (((EXBaseSwooshSys *)h->m_pSwoosh)->Initialize(EXGeoFile::LoadGeoFile(0x1000013, 0, 0), swoosh, pItem, 0, 0, 0, 0)) {
            ((EXBaseSwooshSys *)h->m_pSwoosh)->SetTessalationLevel(2);
        } else {
            gpGameWnd[0]->SwooshSysMgr()->DeleteSystem(h->m_pSwoosh);
            h->m_pSwoosh = 0;
        }
    }
    float yaw = (float)atan2(dir.x, dir.z);
    float pitch = (float)atan2(dir.y, dir.LengthXZ());
    pItem->SetRotation(EXQuaternion(-pitch, yaw, 0.0f, 0.0f));
    pItem->SetPosition(pos);
    pItem->AddAnimator(EXItemAnimator::CreateFrom(pGeo, entity));
    theItemEnv.AddItem(pItem, 0);
    h->m_PredWeapon = predWeapon;
    h->m_Type = 0;
    h->m_Life = 0.0f;
    h->m_PinTime = 0.0f;
    h->m_Speed = pWeapon->m_ThrowSpeed * speedScale;
    if (h->m_Speed == 0.0f) h->m_Speed = 0.75f;
    if (h->m_Targets[0]) {
        if (h->m_WeaponHash == 0x49000008 || h->m_WeaponHash == 0x49000041)
            h->m_Type = 4;
        else
            h->m_Type = 3;
    }
    if (pWeapon->m_AimType == 0x52000007) h->m_Type = 1;  // HT_WeaponClass_Throwing
    if (pWeapon->m_Id == 0x49000024) h->m_Type = 1;       // HT_Weaponry_Drone_GrenadeLauncher
    if (pWeapon->m_AimType == 0x52000008) {               // HT_WeaponClass_TwoStage
        h->m_Type = 8;
        h->m_StageTarget = target;
        h->m_Speed *= 0.5f;
    }
    h->m_PathIndex = -1;
    if (pWeapon->m_AimType == 0x52000007 && pWeapon->m_ThrowSpeed == 0.0f) {
        h->m_Type = 2;
        EXGeoFile *pFX = EXGeoFile::LoadGeoFile(0x1000016, 0, 0);  // HT_File_Pred_FX
        EXItemAnimator_Entity *pBall = new EXItemAnimator_Entity;
        if (pBall) {
            pBall->Initialise(pFX, 0x2000031);  // HT_Entity_RedBall
            pBall->ClrHasRender();
            pBall->RenderInfo().RenderFlagsRef().BitSet(3);
            float r = pWeapon->GetMineTriggerRadius();
            pBall->SetScale(EXVector(r, r, r, 0.0f));
            pBall->RenderFlags().m_flags |= 3;
            pItem->AddAnimator(pBall);
            h->m_pTriggerBall = pBall;
        }
        EXGeoPath *pPath = theItemEnv.GetPath(pos, 0xfffffdff);
        if (pPath && (pPath->Flags() & 0x200)) {
            for (int i = 0; i < theItemEnv.GetPaths(EXMapOn(-1))->Size(); i++) {
                if (pPath == &(*theItemEnv.GetPaths(EXMapOn(-1)))[i]) {
                    h->m_PathIndex = i;
                    h->m_PathNode = pPath->GetPathNode(pos);
                    const EXVector3 &np = pPath->Node(h->m_PathNode)->Position();
                    pItem->SetPosition(EXVector(np.x, np.y, np.z, 0.0f));
                    h->m_PathT = 0.0f;
                }
            }
        }
    }
    return pItem;
}

// Hitscan shot from from along dir, up to range: contoured items (air vehicles and scripts take the
// damage there), then the map, then actors (CheckActorsOnLine) and, unless bIgnorePlayer, the player.
// Returns whether a character was hit; pHit gets the end point, pbHitWorld whether scenery stopped it.
signed char XItemHandler_Bullet::InstantCreate(WeaponProfile *pWeapon, const EXVector &from, const EXVector &,
                                               const EXVector &dir, unsigned int shooter, signed char bIgnorePlayer,
                                               float damage, float range, EXVector *pHit, signed char *pbHitWorld)
{
    if (pbHitWorld) *pbHitWorld = 0;
    EXVector start = from;
    EXVector unit(dir.x, dir.y, dir.z, 0.0f);
    float len = EXSqrt(unit.x * unit.x + unit.z * unit.z + unit.y * unit.y);
    if (len > EX_EPSILON_UNIT) unit *= 1.0f / len;
    EXVector end = unit * range + start;
    EXVector hit = end;
    EXRayCastTemp t;
    t.Set(start, end - start, 0, 0, 0, 0);
    t.m_mask = 8;
    theItemEnv.DoRayCastToContoured(t);
    if (t.m_bHit) {
        if (pbHitWorld) *pbHitWorld = 1;
        hit = t.Intersect();
        EXItem *pOwner;
        if (t.m_pAnimator && (pOwner = t.m_pAnimator->GetOwnerItem()) != 0) {
            EXItemHandler *pHandler = pOwner->ItemHandler();
            if (pHandler) {
                if (Bullet_IsA(pHandler, EXRUNTIME_CLASS(XItemHandler_AirVehicle)))
                    ((XItemHandler_AirVehicle *)pHandler)->ApplyDamage(pWeapon->GetBulletDamage());
                if (Bullet_IsA(pHandler, EXRUNTIME_CLASS(XItemHandler_ScriptRunner))) {
                    ((XItemHandler_ScriptRunner *)pHandler)->ApplyDamage(pWeapon->GetBulletDamage(), 0x53000000);  // HT_DamageClass_Health
                    ((XItemHandler_ScriptRunner *)pHandler)->DoImpactFX(hit);
                }
            }
        }
    }
    t.Set(start, hit - start, 0, 0, 0, 0);
    t.m_mask = 8;
    theItemEnv.DoRayCastToMap(t, 0);
    if (t.m_bHit) {
        if (pbHitWorld) *pbHitWorld = 1;
        hit = t.Intersect();
    }
    if (pHit) *pHit = hit;
    EXLineSeg seg(start, hit - start);
    EXVector actorHit;
    unsigned int id;
    float dist = CheckActorsOnLine(seg, actorHit, id, shooter);
    Actor *pActor = Actor::GetPtrFromID(id);
    if (pActor && pHit) *pHit = actorHit;
    XItemHandler_Player *pPlayer = (XItemHandler_Player *)gpPlayerItem[0]->ItemHandler();
    EXVector playerHit;
    float u;
    if (!bIgnorePlayer && pPlayer->CheckCollision(seg, playerHit, u)) {
        EXVector d = playerHit - start;
        float pd = EXSqrt(d.y * d.y + d.z * d.z + d.x * d.x);
        if (dist > pd) {
            if (pHit) *pHit = unit * pd + start;
            if (pWeapon->m_SfxHit) {
                EXVector p = gpPlayerItem[0]->Position();
                p.y += 1.5f;
                EXBaseSoundManager::GetSoundManager()->StartSound(pWeapon->m_SfxHit, p, -1, 0);
            }
            if (pWeapon->m_Id == 0x49000030 && pHit)  // HT_Weaponry_Drone_MS_SniperRifle
                XItemHandler_Player::Instance()->SendPredMsg(7, *pHit, damage);
            if (pWeapon->m_Id == 0x4900002f) {  // HT_Weaponry_Drone_MS_PlasmaCannon
                if (pHit) {
                    XItemHandler_Player::Instance()->SendPredMsg(0, damage);
                    XItemHandler_Player::Instance()->SendPredMsg(5, *pHit, 0.5f);
                    return 1;
                }
            } else {
                int type = pWeapon->m_DamageClass;
                if (type == 0x53000000 || type == 0x53000002) {  // HT_DamageClass_Health, HT_DamageClass_HealthEnergy
                    if (pWeapon->m_Id == 0x4900002d || pWeapon->m_Id == 0x4900005a || pWeapon->m_FireType == 1
                        || pWeapon->m_Id == 0x4900003c)  // HT_Weaponry_Drone_MS_FreezeThrower, HT_Weaponry_Drone_M16GL_Freeze
                        XItemHandler_Player::Instance()->SendPredMsg(1, damage);
                    else
                        XItemHandler_Player::Instance()->SendPredMsg(0, damage);
                }
                if (type == 0x53000001 || type == 0x53000002)  // HT_DamageClass_Energy, HT_DamageClass_HealthEnergy
                    pPlayer->SendPredMsg(6, damage);
                EXVector p = gpPlayerItem[0]->Position();
                p.y += 1.0f;
                Gore_ShootSplat(p, (EXVector &)dir, 1);
            }
            return 1;
        }
    }
    if (pActor) {
        EXVector up = EXVector(0.0f, 0.0f, 1.0f, 1.0f).UnitVector();
        SendMsg_AttackedByActor(pActor, pWeapon, actorHit, up, damage, Actor::GetPtrFromID(shooter), 0);
        EXVector p = pActor->GetFeetPos();
        p.y += 1.0f;
        Gore_ShootSplat(p, (EXVector &)dir, pActor);
        return 1;
    }
    return 0;
}

// Per-frame projectile update: visibility (hide channel, vision modes), the type's motion step, then
// for straight-flying rounds the hit tests along this frame's move: contoured items, the map, actors
// (deflected by blocking actors), the player. Smart discs ricochet; spears stick in actors (taking a
// severed head along), pin rag dolls and stick in walls; everything else explodes or is removed.
s8 XItemHandler_Bullet::DoUpdate()
{
    if (m_Countdown >= 0 && --m_Countdown < 1) {
        Explode();
        return 1;
    }
    if (m_HideChannel) {
        if (!FlagChannels[(short)m_HideChannel]) {
            m_pItem->ClrHasRender();
            return 1;
        }
        m_HideChannel = 0;
        m_pItem->SetHasRender();
    }
    if (XItemHandler_Player::Instance()->GetVisionMode() == 3) {
        m_pItem->RenderInfo().RenderFlagsRef().BitSet(8);
        m_pItem->RenderInfo().RenderFlagsRef().BitSet(4);
        m_pItem->RenderInfo().SetRGBAMult(0.0f, 1.0f, 1.0f, 1.0f);
    } else if (XItemHandler_Player::Instance()->GetVisionMode() == 1) {
        m_pItem->RenderInfo().RenderFlagsRef().BitSet(8);
        m_pItem->RenderInfo().RenderFlagsRef().BitSet(4);
        m_pItem->RenderInfo().SetRGBAMult(1.0f, 1.0f, 0.0f, 1.0f);
    } else {
        m_pItem->RenderInfo().RenderFlagsRef().BitClr(8);
        m_pItem->RenderInfo().RenderFlagsRef().BitClr(4);
        m_pItem->RenderInfo().SetRGBAMult(1.0f, 1.0f, 1.0f, 1.0f);
    }
    if ((m_PredWeapon == 5 || m_PredWeapon == 7) && (m_pItem->ItemFlags() & 8)) return 1;
    switch (m_WeaponHash) {
    case 0x49000024:  // HT_Weaponry_Drone_GrenadeLauncher
    case 0x49000027:  // HT_Weaponry_Drone_MissileLauncher
    case 0x4900002b:  // HT_Weaponry_Drone_ShockRifle
    case 0x4900002e:  // HT_Weaponry_Drone_MS_MultiLauncher
    case 0x4900003b:  // HT_Weaponry_Ulysses_Rocket
    case 0x4900003f:  // HT_Weaponry_Sniper_Pred_Bolt
    case 0x49000044:  // HT_Weaponry_Drone_1930_Bazooka
    case 0x49000046:  // HT_Weaponry_Ulysses_Rocket_Homing
    case 0x49000050:  // HT_Weaponry_Skull
    case 0x49000052:  // HT_Weaponry_Gunship_Rocket
    case 0x49000056:
    case 0x49000062:
        if (m_pItem->ItemFlags() & 8) return 1;
    }
    WeaponProfile *pWeapon = WeaponProfile::GetWeaponData(m_WeaponHash);
    if (pWeapon) s_Move = m_Velocity * m_Speed * XApp::GameFrameRateMul();
    s_Move.w = 0.0f;
    m_Life += m_Speed * XApp::GameFrameRateMul();
    switch (m_Type) {
    case 1:
        Update_Gravity();
        return 1;
    case 2:
        Update_Mine();
        return 1;
    case 3:
        Update_Homing();
        break;
    case 4:
        Update_SmartDisk();
        break;
    case 5:
        Update_Returning();
        break;
    case 7:
        Update_StaticTimeout();
        return 1;
    case 8:
    case 9:
        Update_TwoStage();
        break;
    }
    if (m_pItem->ItemFlags() & 0x10) return 1;
    if (pWeapon && (pWeapon->IsPredatorProjectileWeapon() || pWeapon->m_Id == 0x4900003b)) {  // HT_Weaponry_Ulysses_Rocket
        for (Actor *p = Actor::GetFirst(m_pItem->Position(), pWeapon->m_AttackRadius, 0); p;
             p = p->GetNext(m_pItem->Position(), pWeapon->m_AttackRadius, 0)) {
            if (p->m_RefID == m_OwnerID) continue;
            EXVector d = p->GetFeetPos() - m_pItem->Position();
            if (m_Velocity.y * d.y + m_Velocity.z * d.z + m_Velocity.x * d.x > 0.0f)
                SendMsg_IncomingAttack(p, m_WeaponHash, -1, m_pItem->Position(), m_Velocity, m_Life);
        }
    }
    EXRayCastTemp t;
    t.Set(m_pItem->Position(), s_Move, 0, 0, 0, 0);
    t.m_mask = 8;
    s8 bHitMap = 0;
    theItemEnv.DoRayCastToContoured(t);
    if (!t.m_bHit) {
        theItemEnv.DoRayCastToMap(t, 0);
        bHitMap = t.m_bHit;
    } else {
        s8 bPassThrough = 0;
        EXItem *pOwner;
        if (t.m_pAnimator && (pOwner = t.m_pAnimator->GetOwnerItem()) != 0) {
            Vehicle *pVehicle = getItemOwnerVehicle(pOwner);
            if (pVehicle) {
                float damage = CalcDamage();
                if (m_WeaponHash == 0x49000009) damage *= 5.0f;  // HT_Weaponry_Predator_Plasmacaster
                pVehicle->ApplyAttackDamage(m_PredWeapon, damage);
            } else {
                EXItemHandler *pHandler = pOwner->ItemHandler();
                if (pHandler) {
                    if (Bullet_IsA(pHandler, EXRUNTIME_CLASS(XItemHandler_AirVehicle))) {
                        if (m_WeaponHash == 0x49000007 && ((XItemHandler_AirVehicle *)pHandler)->m_Unk110 == 1) {
                            EXVector p = t.Intersect();
                            Bounce(p);
                            ((XItemHandler_AirVehicle *)pHandler)->ApplyDamage(0.1f);
                            return 1;
                        }
                        ((XItemHandler_AirVehicle *)pHandler)->ApplyDamage(CalcDamage());
                    } else if (Bullet_IsA(pHandler, EXRUNTIME_CLASS(XItemHandler_ScriptRunner))) {
                        ((XItemHandler_ScriptRunner *)pHandler)->ApplyDamage(CalcDamage(), 0x53000000);  // HT_DamageClass_Health
                        EXVector p = t.Intersect();
                        ((XItemHandler_ScriptRunner *)pHandler)->DoImpactFX(p);
                        if (((XItemHandler_ScriptRunner *)pHandler)->m_TriggerFlags & 1) bPassThrough = 1;
                    } else if (Bullet_IsA(pHandler, EXRUNTIME_CLASS(XItemHandler_ClayPigeon))) {
                        ((XItemHandler_ClayPigeon *)pHandler)->ApplyDamage(CalcDamage());
                    } else if (Bullet_IsA(pHandler, EXRUNTIME_CLASS(XItemHandler_CCTV))
                               || Bullet_IsA(pHandler, EXRUNTIME_CLASS(XItemHandler_Turret))) {
                        if (m_WeaponHash == 0x49000007) {  // HT_Weaponry_Predator_Speargun
                            EXVector p = t.Intersect();
                            Bounce(p);
                            return 1;
                        }
                        Explode();
                        return 1;
                    } else if (Bullet_IsA(pHandler, EXRUNTIME_CLASS(XItemHandler_VMO))) {
                        if (((XItemHandler_VMO *)pHandler)->m_VMOFlags & 1) bPassThrough = 1;
                    }
                }
            }
        }
        if (m_WeaponHash == 0x49000007 && m_Type != 6 && bPassThrough) t.m_bHit = 0;
    }
    s8 bOutOfRange = 0;
    s8 bHitActor = 0;
    s8 bHitPlayer = 0;
    if (m_Type == 4) {
        if (m_WeaponHash == 0x49000008) {  // HT_Weaponry_Predator_SmartDiskThrown
            if (!m_Targets[0]) {
                EXVector d = m_pItem->Position() - gpPlayerItem[0]->Position();
                if (EXSqrt(d.y * d.y + d.z * d.z + d.x * d.x) > CalcRange()) bOutOfRange = 1;
            }
        } else if (m_Life > CalcRange()) {
            bOutOfRange = 1;
        }
    } else {
        if (m_Type != 5 && m_Life > CalcRange()) bOutOfRange = 1;
        if (m_Life > m_Range) bHitActor = 1;
    }
    EXVector hit = m_pItem->Position();
    EXLineSeg seg(m_pItem->Position(), s_Move);
    unsigned int id = 0;
    CheckActorsOnLine(seg, hit, id, m_OwnerID);
    s8 bDeflected = 0;
    if (id) {
        Actor *pActor = Actor::GetPtrFromID(id);
        if (pActor->m_BlockCounter > 0.0f || (pActor->m_ActorFlags.m_bUlysses && m_WeaponHash == 0x49000007)) {
            Actor::OverridePlayerCloak();
            pActor->m_HitCountdown = 10.0f;
            pActor->m_ForceSightingTimer = (float)XGetNominalFrameRate() * 3.0f;
            EXVector chest = pActor->GetFeetPos();
            chest.y += 1.0f;
            bHitActor = 0;
            bOutOfRange = 1;
            bDeflected = 1;
            if (m_WeaponHash == 0x49000007) {  // HT_Weaponry_Predator_Speargun
                EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00244, chest, -1, 0);
                XItemHandler_ScriptBase::FX_Create(m_pItem->m_position, 0x40003c9, 0x40, 0x1000016);  // HT_Script_Impact_Smart_Disc
            }
            if (m_WeaponHash == 0x49000008) {  // HT_Weaponry_Predator_SmartDiskThrown
                EXBaseSoundManager::GetSoundManager()->StartSound(0x1af0023b, chest, -1, 0);
                XItemHandler_ScriptBase::FX_Create(m_pItem->m_position, 0x40003c9, 0x40, 0x1000016);
            }
            m_Velocity = -m_Velocity;
            if (m_WeaponHash == 0x49000007) {
                m_Velocity.x += g_EXRandClass.Randf() - 0.5f;
                m_Velocity.z += g_EXRandClass.Randf() - 0.5f;
            }
            m_Velocity.y += 0.5f;
            Bullet_Normalise(m_Velocity);
        } else {
            bHitActor = 1;
            if (!pWeapon->m_ExplosionScript || pWeapon->m_MineTriggerRadius == 0.0f)
                SendMsg_AttackedByPredator(pActor, m_PredWeapon, hit, m_Velocity, CalcDamage(), 0);
            EXVector chest = pActor->GetFeetPos();
            chest.y += 1.0f;
            Gore_ShootSplat(chest, m_Velocity, pActor);
            Gore_BloodPool(chest, pActor);
        }
    }
    float u;
    EXVector playerHit;
    if (!m_bFromPlayer && XItemHandler_Player::Instance()->CheckCollision(seg, playerHit, u)) {
        XItemHandler_Player *pPlayer = XItemHandler_Player::Instance();
        hit = playerHit;
        int type = pWeapon->m_DamageClass;
        bHitPlayer = 1;
        if (type == 0x53000000 || type == 0x53000002) {  // HT_DamageClass_Health, HT_DamageClass_HealthEnergy
            if (pWeapon->m_Id == 0x4900002d || pWeapon->m_FireType == 1 || pWeapon->m_Id == 0x4900003c)
                pPlayer->SendPredMsg(1, CalcDamage());
            else if (pWeapon->m_Id == 0x49000041)  // HT_Weaponry_HunterSmartDisc
                pPlayer->SendPredMsg(7, playerHit, CalcDamage());
            else
                pPlayer->SendPredMsg(0, CalcDamage());
        }
        if (type == 0x53000001 || type == 0x53000002)  // HT_DamageClass_Energy, HT_DamageClass_HealthEnergy
            XItemHandler_Player::Instance()->SendPredMsg(6, CalcDamage());
        if (m_WeaponHash == 0x49000041) m_bFromPlayer = 1;
        if (pWeapon->m_SfxHit) EXBaseSoundManager::GetSoundManager()->StartSound(pWeapon->m_SfxHit, hit, -1, 0);
    }
    if (!t.m_bHit) {
        if (!bOutOfRange && !bHitActor && !bHitPlayer) {
            if (m_Type == 6) {
                m_PinTime += m_Speed * XApp::GameFrameRateMul();
                Actor *pTarget = Actor::GetPtrFromID(m_Targets[0]);
                if (pTarget && !pTarget->DontDragMeSpear()) {
                    EX6DAnim *pAnim = (EX6DAnim *)pTarget->GetItem()->m_pItemPhysics;
                    if (pAnim && pAnim->GetRuntimeClass()->IsDerivedFrom(EXRUNTIME_CLASS(EX6DAnim)) && pAnim->Body(0)) {
                        pAnim->Body(0)->m_pos = m_pItem->Position();
                        pAnim->Body(0)->m_velocity = s_Move;
                        ((EX6DRigidBody *)pAnim->Body(0))->AngularVelocity().SetZero();
                        pAnim->Body(0)->SetMass(100.0f);
                        if (m_PinTime > 30.0f || pAnim->GetTotalCollisions() > 0.0f) {
                            pAnim->Body(0)->SetMass(1.0f);
                            m_Targets[0] = 0;
                            EXVector impulse = m_Velocity * 10.0f;
                            pAnim->Body(0)->AddWorldImpulse(impulse);
                        }
                    } else {
                        m_Targets[0] = 0;
                    }
                }
            }
            m_pItem->SetPosition(m_pItem->Position() + s_Move);
            m_SoundTag.UpdatePosition(m_pItem->Position());
            return 1;
        }
    } else {
        EXVector p = t.Intersect();
        DetonateNearbyMines(p, 0.2f);
    }
    if (m_WeaponHash == 0x49000009 && bHitMap) {  // HT_Weaponry_Predator_Plasmacaster
        EXVector p = m_pItem->Position() - m_Velocity;
        Gore_ShootSplat(p, m_Velocity, 8);
    }
    if (m_WeaponHash == 0x49000008 || m_WeaponHash == 0x49000041) {  // smart discs
        if (t.m_bHit) {
            if (m_Bounces < 5) {
                EXVector n(t.m_normal.x, t.m_normal.y, t.m_normal.z, 0.0f);
                n.Normalise();
                Bullet_Normalise(m_Velocity);
                float d = (m_Velocity.z * n.z + m_Velocity.y * n.y + m_Velocity.w * n.w + n.x * m_Velocity.x) * 2.0f;
                m_Velocity -= n * d;
                Bullet_Normalise(m_Velocity);
                m_Bounces++;
                EXVector p = t.Intersect();
                u32 fx = pWeapon->m_ExplosionScript;
                if (!fx) fx = 0x40000e5;  // HT_Script_FX_Hit_Sparks
                XItemHandler_ScriptBase::FX_Create(p, fx, 0x40, 0x1000016);
                u32 snd;
                switch (t.m_face.GeoTexture()->GameFlags()) {
                case 1:
                case 0x20:
                case 0x40:
                    snd = 0x1af0023c;
                    break;
                default:
                    snd = 0x1af0023b;
                    break;
                }
                EXBaseSoundManager::GetSoundManager()->StartSound(snd, t.Intersect(), -1, 0);
            } else {
                m_pItem->SetPosition(m_pItem->Position() + s_Move);
                m_SoundTag.UpdatePosition(m_pItem->Position());
                m_Type = 5;
            }
        }
        if (bHitActor || bHitPlayer) {
            EXBaseSoundManager::GetSoundManager()->StartSound(0x1af0023a, m_pItem->Position(), -1, 0);
            if (id == (unsigned int)m_Targets[0]) {
                m_Targets[0] = m_Targets[1];
                m_Targets[1] = m_Targets[2];
                m_Targets[2] = m_Targets[3];
                m_Targets[3] = 0;
            }
            m_pItem->SetPosition(m_pItem->Position() + s_Move);
            m_SoundTag.UpdatePosition(m_pItem->Position());
        }
        if (bOutOfRange || !m_Targets[0] || id == (unsigned int)m_Targets[0] || bHitPlayer) m_Type = 5;
        return 1;
    }
    if (m_WeaponHash == 0x49000007) {  // HT_Weaponry_Predator_Speargun
        if (bOutOfRange) {
            m_Type = 1;
            m_bFromPlayer = 0;
            return 1;
        }
        SendMsg_LoudNoise(hit, pWeapon->m_NoiseRadius, 0, 1, 0);
        if (m_Type != 6 && bHitActor) {
            m_Type = 6;
            m_Targets[0] = id;
            m_Unk30 = 0;
            m_Speed *= 0.25f;
            EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00243, m_pItem->Position(), -1, 0);
            Actor *pActor = Actor::GetPtrFromID(id);
            if (pActor) {
                if (pActor->m_LostLimbs[2].m_bLost) {
                    ActorSkin *pSkin = AS_GetPtrFromIndex(pActor->m_SkinIndex);
                    EXGeoFile *pGeo = EXGeoFile::LoadGeoFile(pSkin->m_LimbFile, 0, 0);
                    EXItemAnimator_Entity *pHead = EXItemAnimator_Entity::NewItem();
                    if (pHead) {
                        pHead->Initialise(pGeo, pSkin->m_Head);
                        pHead->SetPosition(EXVector(0.0f, -0.1f, 0.0f, 0.0f));
                        EXVector rot = EXVector(0.0f, pActor->GetRotY(), 0.0f, 0.0f) - m_pItem->Rotation();
                        pHead->SetRotation(EXQuaternion(rot));
                        pHead->RenderFlags().m_flags |= 1;
                        m_pItem->AddAnimator(pHead);
                        m_pTriggerBall = pHead;
                        XItemHandler_Bullet *h = (XItemHandler_Bullet *)m_pItem->ItemHandler();
                        h->m_GeoSectionGeo = pSkin->m_LimbFile;
                        h->m_GeoSectionItem = pSkin->m_Head;
                        GeoSectionManager::Add(pSkin->m_LimbFile, pSkin->m_Head);
                    }
                }
                if (pActor->IsDead()) return 1;
            }
            XItemHandler_ScriptBase::FX_Create(hit, pWeapon->m_ExplosionScript, 0x40, 0x1000016);
            m_pItem->ItemFlags() = 0x10;
            return 1;
        }
        if (bHitMap) {
            AU_PlayerForceSightingInArea(hit, 5.0f);
            EXVector p = t.Intersect();
            m_Velocity *= 0.25f;
            p -= m_Velocity;
            m_pItem->SetPosition(p);
            if (m_Type == 6) {
                if (!m_pTriggerBall) {
                    Actor *pActor = Actor::GetPtrFromID(m_Targets[0]);
                    if (pActor) Gore_ShootSplat(p, m_Velocity, pActor);
                } else {
                    Gore_ShootSplat(p, m_Velocity, 0);
                }
            }
            m_Life = 15.0f;
            m_Type = 7;
            m_Speed = 0.0f;
            if (pWeapon->m_ExplosionScript)
                XItemHandler_ScriptBase::FX_Create(hit, pWeapon->m_ExplosionScript, 0x40, 0x1000016);
            u32 snd;
            switch (t.m_face.GeoTexture()->GameFlags()) {
            case 1:
            case 0x20:
            case 0x40:
                snd = 0x1af00245;
                break;
            default:
                snd = 0x1af00244;
                break;
            }
            EXBaseSoundManager::GetSoundManager()->StartSound(snd, t.Intersect(), -1, 0);
            return 1;
        }
    } else if (pWeapon->m_ExplosionScript && (!bOutOfRange || bDeflected)) {
        if (bHitMap) hit -= m_Velocity * 0.5f;
        float radius = CalcRadius();
        XItemHandler_ScriptBase::Explosion_Create(hit, pWeapon->m_ExplosionScript, (int)radius, (int)CalcDamage(),
                                                  m_PredWeapon, m_WeaponHash, pWeapon->m_ExplosionType, 0, 0);
        if (pWeapon->m_Id == 0x4900002e) {  // HT_Weaponry_Drone_MS_MultiLauncher
            EXVector d = Player_GetFeetPos() - hit;
            if (d.LengthXYZSquared() < 25.0f)
                XItemHandler_Player::Instance()->SendPredMsg(5, hit, (5.0f - d.LengthXYZ()) * 0.2f);
        }
    }
    m_pItem->ItemFlags() = 0x10;
    m_SoundTag.UpdatePosition(m_pItem->Position());
    return 1;
}
