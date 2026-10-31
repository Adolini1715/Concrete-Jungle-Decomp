// The original included the CRT <math.h>: its powf (emitted as _powf) and float overloads
// (fmod(float, float) widens a float constant) are what Player.obj was built with.
#include <math.h>
#include "FEText.h"
#include "XItemHandler_Player.h"
#include "XItemHandler_Camera.h"
#include "XItemPhysics.h"
#include "XItem.h"
#include "XGameWnd.h"
#include "FEHud.h"
#include "EXItemAnimator_Anim.h"
#include "EXSwooshSysMgr.h"
#include "EXBaseSwooshSys.h"
#include "EXBaseSoundManager.h"
#include "EXGeoFile.h"
#include "EXGeoHeader.h"
#include "EXGeoAnim.h"
#include "EXMemory.h"
#include "EXItemRender_Anim.h"
#include "XApp.h"
#include "Vehicle.h"
#include "VisionWipe.h"
#include "XItemHandler_ScriptBase.h"
#include "Actor.h"
#include "EXDatum.h"
#include "EXCapsule.h"
#include "EX6DObject.h"
#include "EXItemAnimator_Sound.h"
#include "Recharge.h"
#include "XGamePad.h"
#include "ActorUtil.h"
#include "EXGrabPointTemp.h"
#include "EXWatcher.h"
#include "EXRayCastTemp.h"
#include "EXSwooshSys.h"
#include "GameData.h"
#include "EXItemAnimator_AnimModifier.h"
#include "EXItemAnimator_Entity.h"
#include "EXShadowTemp.h"
#include "EXItemCtrl_SnapShot.h"
#include "FEMainMenu.h"
#include "EXRand.h"
#include "GeoSectionManager.h"
#include "WeaponTable.h"
#include "FX_Gore.h"
#include "XItemHandler_Mimicry.h"
#include "ActorOptions.h"
#include "ActorCombat.h"
#include "EXDistance.h"
#include "EXTriangle.h"
#include "EXSphere.h"
#include "EXPlane.h"
#include "XItemHandler_Bullet.h"
#include "EXMatrix.h"
#include "ScriptRunner.h"
#include "EXLine.h"
float Vec_AngleDifference(float a, float b);
float SlerpConst(EXQuaternion &a, EXQuaternion &b, EXQuaternion &out, float t);

// TODO stand-ins: other areas.
class ActorTextureSwap {  // TODO stand-in
public:
    static void Apply(Actor *pActor);
};
struct GEOSCRIPT_BASECMD {  // TODO stand-in (geo script commands)
    u8 m_Type;
    u8 m_01[0x15 - 0x01];
    u8 m_Reverb;    // 0x15
    u8 m_16[0x20 - 0x16];
    u32 m_Hash;     // 0x20
};
struct GEOSCRIPT_EVENT {  // TODO stand-in (geo script events, layout from Player.obj DoScriptCmdEvent)
    u16 m_00[3];
    u16 m_Frames;           // 0x06 duration in frames (30 Hz: x2 for the 60 Hz counters)
    u32 m_08;
    unsigned int m_Hash;    // 0x0c HT_ScriptEvents_*
    u32 m_10;
    union {
        int i;
        unsigned int u;
        float f;
    } m_Param[4];           // 0x14
};
Vehicle *getItemOwnerVehicle(EXItem *pItem);
void SetFog(unsigned char mode);
void SetWatcherEffects(unsigned char mode);
void Update_VMO(unsigned char mode);
void Objective_NotifyPlayerSeen(Vehicle *pVehicle);
void Objective_NotifyActorJustKilled(Actor *pActor);
void Objective_NotifyActorDeath(Actor *pActor);
extern XGameWnd *gpGameWnd[4];
extern unsigned int gCurrentMap_GeoHash;

// Player data tables (Player.obj initialised data). TODO: define with their contents.
// A weapon carried on the body while not in hand (UpdatePlayerBodyWeapons).
struct _ATTACH_DATA {
    u32 m_WeaponId;
    u32 m_ModelHash;     // 0x04 model currently loaded into the item
    u8 m_Avail;          // 0x08 the weapon has been collected
    u8 m_bLoaded;        // 0x09
    u8 m_bShown;         // 0x0a
    u8 m_Unkb;
    u32 m_DatumHash;     // 0x0c body datum it hangs from
    int m_Item;          // 0x10 index into m_pWeaponItem
    u16 m_VertexSet;     // 0x14
    u16 m_Unk16;
    float m_Rot[3];      // 0x18 euler offset from the datum
    float m_OffsetZ;     // 0x24
};
extern EXFixedArray<_ATTACH_DATA, 5> Attach_Data;
struct _WEAPON_LOAD {
    u32 m_Id;
};
extern EXFixedArray<_WEAPON_LOAD, 32> WeaponLoadList;
struct _PRED_SKINS {
    union {
        u32 m_Hash[5];
        float m_DiskY;   // 0x00 height of the disk on the back (UpdatePlayerBodyWeapons)
    };
};
extern EXFixedArray<_PRED_SKINS, 14> PRED_Skins;
struct _COMBO_SEQ {
    int m_Pos;
    int m_Result;
    int m_Moves[8];
};
extern _COMBO_SEQ Combo_Seq[7];
extern EXFixedArray<unsigned int, 3> WristBladeExtendHash;
extern EXFixedArray<EXFixedArray<unsigned int, 3>, 3> ShoulderCannonExtendHash;
struct _WEAPON_SHAPE {
    int m_Id;
    int m_VertexSet;      // 0x04
    u32 m_08;
    float m_VertexScale;  // 0x0c
    u32 m_10;
    u32 m_Hash[3];        // 0x14 per upgrade level
    u32 m_20[2];
};
extern EXFixedArray<_WEAPON_SHAPE, 6> Weapon_Shapes;
extern EXFixedArray<EXFixedArray<unsigned int, 4>, 7> GroundSounds;
extern EXFixedArray<EXFixedArray<unsigned int, 6>, 14> WeaponAttackAnims;
extern EXFixedArray<EXFixedArray<unsigned int, 2>, 14> WeaponAttackAnimsBlend;
extern EXFixedArray<unsigned int, 27> VM_Hash;
extern EXFixedArray<EXFixedArray<unsigned int, 2>, 14> WeaponHash;
extern EXFixedArray<unsigned int, 11> CollisionHash;
extern EXFixedArray<EXFixedArray<float, 3>, 5> DamageMatrix;
extern XItemList gHitableList;
struct _COMBO_MOVES {
    unsigned int m_Moves[4];    // move anim per combo step
};
extern EXFixedArray<_COMBO_MOVES, 5> Combo_Moves;
// Movement flags per move: m_Key is (mode << 16) | sub, or (mode << 16) | 0xffff for every sub of a
// mode; SetupMove ORs the m_Flags of every matching entry into m_MoveFlags (0x800: no damage,
// 0x20 / 0x40: root motion, 0x80: low friction, keep the move direction).
struct _PREDMOVEFLAGS {
    unsigned int m_Key;
    unsigned int m_Flags;
};
extern EXFixedArray<_PREDMOVEFLAGS, 70> PredMoveFlags;
extern EXFixedArray<EXFixedArray<unsigned int, 6>, 14> WeaponAt;   // attack anim per weapon and attack state

// Player.obj-only maths helpers (map [fi] copies with no callers: __forceinline).
__forceinline float qpowf(float x, float y)
{
    if (x == 0.0f && !(y >= 0.0f)) return 0.0f;
    return powf(x, y);
}
__forceinline float EXMakeAngleBiPolar(float a)
{
    if (3.1415927f > (float)fabs(a)) return a;
    float r = fmod(a, 6.2831855f);
    if (3.1415927f > (float)fabs(r)) return r;
    if (a >= 0.0f) return r - 6.2831855f;
    return r + 6.2831855f;
}
__forceinline float Interp(float a, float b, float t, float p)
{
    t = 1.0f - t;
    return (a - b) * qpowf(t, p) + b;
}

template <class T> inline void PlayerClampMax(T &a, const T &b) { if (a > b) a = b; }

EXRuntimeClass *XItemHandler_Player::_GetBaseClass() { return EXRUNTIME_CLASS(EXItemHandler); }
const EXRuntimeClass XItemHandler_Player::classXItemHandler_Player = {
    EXRUNTIME_CLASS(EXItemHandler), "XItemHandler_Player", sizeof(XItemHandler_Player),
    (void *(__cdecl *)())XItemHandler_Player::CreateObject};
XItemHandler_Player *XItemHandler_Player::m_pInstance;

void SetRotation(EXVector &v, float angle, float dist)
{
    v.x = (float)sin(angle) * dist;
    v.w = 0.0f;
    v.y = 0.0f;
    v.z = (float)cos(angle) * dist;
}

void XItemHandler_Player::ButtonUsed(int b)
{
    m_ButtonsPressed &= ~b;
    m_ButtonsHeld &= ~b;
    m_ButtonsReleased &= ~b;
}

void CalculateShape(float f, int &a, int &b, float &frac)
{
    a = (int)f;
    frac = f - (float)a;
    b = a + 1;
}

void XItemHandler_Player::GetLookAt(EXVector &v) { v = m_pItem->Position(); }

float XItemHandler_Player::GetBaseHeight() { return m_BaseHeight; }

Actor *XItemHandler_Player::GetLockonActor()
{
    XItemHandler_Camera *pCam = XGetCamera(m_PlayerIndex);
    if (pCam->GetMode() == 1 && pCam->GetTargetID() > 0) return Actor::GetPtrFromID(pCam->GetTargetID());
    return 0;
}

void XItemHandler_Player::PlayNegativeSound(int type)
{
    unsigned int hash;
    switch (type) {
    case 1: hash = 0x1af005db; break;
    case 2: hash = 0x1af001c5; break;
    default: hash = 0x1af00576; break;
    }
    EXBaseSoundManager::GetSoundManager()->StartSound(hash, m_pItem->Position(), -1, 0);
}

void XItemHandler_Player::UseEnergy(float f) { m_Energy -= f; }

XItem *XItemHandler_Player::GetStandOn()
{
    if (m_Mode != 0x15) return 0;
    return m_pStandOn;
}

void XItemHandler_Player::SetInertia(float f)
{
    m_Speed.x = (float)sin(m_pItem->Rotation().y) * f;
    m_Speed.z = (float)cos(m_pItem->Rotation().y) * f;
}

float GetRotation(EXVector &v) { return (float)atan2(v.x, v.z); }

void XItemHandler_Player::SetHelmet(signed char b) { m_bHelmet = b; }

void XItemHandler_Player::LoadPredatorSkin()
{
    m_SkinC4 = -1;
    m_SkinBC = -1;
    m_SkinC0 = 0;
}

void XItemHandler_Player::DoEquip() {}

void XItemHandler_Player::ResetAngleStore() {}

void XItemHandler_Player::DoClimb() {}

void XItemHandler_Player::ResetCombo()
{
    for (int i = 0; i < 7; i++) Combo_Seq[i].m_Pos = 0;
}

int XItemHandler_Player::GetComboAvailable()
{
    for (int i = 0; i < 7; i++)
        if (Combo_Seq[i].m_Moves[Combo_Seq[i].m_Pos] == -1) return Combo_Seq[i].m_Result;
    return -1;
}

Actor *XItemHandler_Player::GetLockOnActor()
{
    int type;
    void *p = XGetCamera(0)->GetLockOn(type);
    return type == 1 ? (Actor *)p : 0;
}

Actor *XItemHandler_Player::GetTargetActor()
{
    int id = XGetCamera(m_PlayerIndex)->GetTargetID();
    if (id > 0) {
        Actor *p = Actor::GetPtrFromID(id);
        if (p) return p;
    }
    return 0;
}

signed char XItemHandler_Player::CheckForThrowSpace(float) { return 1; }

float CalculateAngle(float speed, float dist, float height, float grav, signed char bHigh);

// NEAR (see check; ours merges the four hand-datum GetAnimatorDatum call sites into one, retail keeps them
// apart: 33 vs 36 calls, same callees otherwise). The melee hit test for one swing hand (flags 1 left, 2 right). The weapon is a point
// (the hand weapon datum) or, for the blade weapons (2, 3, 4) in the left hand, a blade between two datums
// of the weapon item; its path since the last consecutive check is swept as a capsule (radius 0.5, x2
// on m_Unk214 1) or two triangles. Actors within 2 units are hit through their own collision
// (SendMsg_AttackedByPredator with the combo's attack id and DamageMatrix damage, plus blood), hitable
// scenery within 10 takes 10 damage with its impact effect, vehicles within 3.5 take the attack damage.
void XItemHandler_Player::CheckForCharacterHit(int flags)
{
    EXDatumTemp tip, base;
    memset(&tip, 0, sizeof tip);
    memset(&base, 0, sizeof base);
    int bLeft = flags & 1;
    float radius = 0.0f;
    unsigned int hand = 2;
    int bBlade = 0;
    if (bLeft) hand = 0;
    int bRight = flags & 2;
    if (bRight) hand = 1;
    else if (hand > 1) return;
    int at = m_AttackType;
    if (at == 2) {
        if (bLeft == 1) {
            ((EXItemAnimator *)m_pWeaponItem[0])->GetDatum(0x1000002b, tip, 0, EXItemAnimator::m_pNullDatumPtr);
            ((EXItemAnimator *)m_pWeaponItem[0])->GetDatum(0x10000053, base, 0, EXItemAnimator::m_pNullDatumPtr);
            EXVector o;
            D3DXVec4Transform(&o, &tip.m_position, &m_pItem->m_mtx);
            tip.m_position = o;
            D3DXVec4Transform(&o, &base.m_position, &m_pItem->m_mtx);
            base.m_position = o;
            radius = 0.5f;
        }
        bBlade = bLeft == 1;
        if (bRight != 2) goto swept;
        m_pItem->GetAnimatorDatum(0x10000006, tip, 0, 0);
        bBlade = 0;
    } else if (at == 3) {
        if (bLeft == 1) {
            ((EXItemAnimator *)m_pWeaponItem[0])->GetDatum(0x10000024, tip, 0, EXItemAnimator::m_pNullDatumPtr);
            ((EXItemAnimator *)m_pWeaponItem[0])->GetDatum(0x10000023, base, 0, EXItemAnimator::m_pNullDatumPtr);
            EXVector o;
            D3DXVec4Transform(&o, &tip.m_position, &m_pItem->m_mtx);
            tip.m_position = o;
            D3DXVec4Transform(&o, &base.m_position, &m_pItem->m_mtx);
            base.m_position = o;
            radius = 0.5f;
        }
        bBlade = bLeft == 1;
        if (bRight != 2) goto swept;
        m_pItem->GetAnimatorDatum(0x10000006, tip, 0, 0);
        bBlade = 0;
    } else if (at == 4) {
        ((EXItemAnimator *)m_pWeaponItem[0])->GetDatum(0x1000002e, tip, 0, EXItemAnimator::m_pNullDatumPtr);
        ((EXItemAnimator *)m_pWeaponItem[0])->GetDatum(0x10000050, base, 0, EXItemAnimator::m_pNullDatumPtr);
        EXVector o;
        D3DXVec4Transform(&o, &tip.m_position, &m_pItem->m_mtx);
        tip.m_position = o;
        D3DXVec4Transform(&o, &base.m_position, &m_pItem->m_mtx);
        base.m_position = o;
        bBlade = 1;
    } else {
        if (hand == 0) m_pItem->GetAnimatorDatum(0x10000019, tip, 0, 0);
        else m_pItem->GetAnimatorDatum(0x1000001a, tip, 0, 0);
        bBlade = 0;
    }
    radius = 0.5f;
swept:
    if (m_Unk214 == 1) radius *= 2.0f;
    unsigned int gap = m_UpdateCount - m_SwingCount[hand];
    SwingTrail &rec = m_Swing[hand];
    if (bBlade == 1) {
        if (gap < 2) {
            rec.m_TipPrev = rec.m_Tip;
            rec.m_BasePrev = rec.m_Base;
        } else {
            rec.m_TipPrev = tip.m_position;
            rec.m_BasePrev = base.m_position;
        }
        rec.m_Tip = tip.m_position;
        rec.m_Base = base.m_position;
    } else {
        if (gap < 2) rec.m_TipPrev = rec.m_Tip;
        else rec.m_TipPrev = tip.m_position;
        rec.m_Tip = tip.m_position;
    }
    m_SwingCount[hand] = m_UpdateCount;
    EXCapsule cap;
    cap.Origin() = rec.m_Tip;
    cap.Direction() = rec.m_TipPrev - rec.m_Tip;
    cap.Radius() = radius;
    float dmg = 10.0f;
    at = m_AttackType;
    if (at >= 0 && at < 5) dmg = ((float *)&DamageMatrix)[at * 3 + GetWeaponUpgrade(at)];
    if (m_MoveFlags & 0x200) dmg *= 1.5f;

    EXVector hit, rel;
    for (Actor *pA = Actor::GetFirst(tip.m_position, 2.0f, 0); pA; pA = pA->GetNext(tip.m_position, 2.0f, 0)) {
        if (pA->m_HitCountdown > 0.0f) continue;
        if (!(pA->m_Health > 0.0f) && !pA->m_StateFlags.m_bCanRipBossHead) continue;
        if (pA->m_ActorFlags.m_bHolographic) continue;
        int r;
        if (bBlade == 1) r = pA->CheckCollision(&rec.m_Tip, radius, hit);
        else r = pA->CheckCollision(cap, hit);
        if (!r) continue;
        rel = hit - m_pItem->Position();
        signed char b;
        if (r == 1) b = 1;
        else {
            b = 0;
            if (m_AttackType >= 0 && m_AttackType < 5) m_WeaponUse[m_AttackType]++;
            int msg = m_AttackType;
            if (m_MoveFlags & 0x200) {
                switch (m_Combo) {
                case 0:
                    switch (msg) {
                    case 0: msg = 0x31; break;
                    case 1: msg = 0x32; break;
                    case 2: msg = 0x33; break;
                    case 3: msg = 0x3b; break;
                    case 4: msg = 0x37; break;
                    }
                    break;
                case 1:
                    if (msg == 2) msg = 0x34;
                    else if (msg == 3) msg = 0x3c;
                    else if (msg == 4) msg = 0x38;
                    break;
                case 2:
                    if (msg == 2) msg = 0x35;
                    else if (msg == 3) msg = 0x3d;
                    else if (msg == 4) msg = 0x39;
                    break;
                case 3:
                    if (msg == 2) msg = 0x36;
                    else if (msg == 3) msg = 0x3e;
                    else if (msg == 4) msg = 0x3a;
                    break;
                }
            }
            EXVector d = pA->GetItem()->Position() - m_pItem->Position();
            float l = d.x * d.x + d.w * d.w + d.z * d.z + d.y * d.y;
            if (l >= 0.0f && (l = sqrtf(l)) > 1e-7f) {
                l = 1.0f / l;
                d.x *= l;
                d.y *= l;
                d.z *= l;
                d.w *= l;
            }
            SendMsg_AttackedByPredator(pA, msg, hit, d, dmg, r);
        }
        Gore_CreateActorBlood(pA, 0, b, m_AttackType, hit, rel, 1);
    }

    for (XItem *pI = gHitableList.GetFirst(); pI; pI = gHitableList.GetNext(pI)) {
        XItemHandler_ScriptRunner *pH = (XItemHandler_ScriptRunner *)pI->ItemHandler();
        if (!pH->m_bHitable || pH->m_HitTimer > 0.0f || pH->m_Health < 0.0f) continue;
        EXVector c(pH->m_HitCentre[0], pH->m_HitCentre[1], pH->m_HitCentre[2], pH->m_HitCentre[3]);
        EXVector d = m_pItem->Position() - c;
        float l = d.x * d.x + d.y * d.y + d.z * d.z;
        if (l >= 0.0f && sqrtf(l) > 10.0f) continue;
        float r = pH->m_HitRadius + radius;
        float r2 = r * r;
        if (bBlade == 1) {
            float s0, s1;
            EXTriangle t1;
            t1.m_v[0] = rec.m_Tip;
            t1.m_v[1] = rec.m_TipPrev - rec.m_Tip;
            t1.m_v[2] = rec.m_Base - rec.m_Tip;
            if (EXSqrDistance(c, t1, &s0, &s1) > r2) {
                EXTriangle t2;
                t2.m_v[0] = rec.m_TipPrev;
                t2.m_v[1] = rec.m_BasePrev - rec.m_TipPrev;
                t2.m_v[2] = rec.m_Base - rec.m_TipPrev;
                if (EXSqrDistance(c, t2, &s0, &s1) > r2) continue;
                hit = (c + t2.m_v[0]) * 0.5f;
            } else {
                hit = (c + t1.m_v[0]) * 0.5f;
            }
        } else {
            float t;
            EXLineSeg seg(cap.Origin(), cap.Direction());
            if (EXSqrDistance(c, seg, &t) > r2) continue;
            hit = cap.Direction() * t + cap.Origin();
        }
        pH->DoImpactFX(hit);
        pH->ApplyDamage(10.0f, 0x53000000);
    }

    for (Vehicle *pV = Vehicle::GetFirst(tip.m_position, 3.5f); pV; pV = pV->GetNext(tip.m_position, 3.5f)) {
        int r;
        if (bBlade == 1) r = pV->CheckCollision(&rec.m_Tip, radius, hit);
        else r = pV->CheckCollision(cap, hit);
        if (r && pV->ApplyAttackDamage(m_AttackType, dmg)) {
            EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00211, hit, -1, 0);
            XItemHandler_ScriptBase::FX_Create(hit, 0x40000e5, 0x40, 0x1000016);
        }
    }
}

// Can a throw from the hand reach the locked-on target? (launch angle under 90 degrees)
signed char XItemHandler_Player::CheckTargetThrowAngle()
{
    int type;
    void *p = XGetCamera(0)->GetLockOn(type);
    EXVector t;
    switch (type) {
    case 1:
        if (!p) return 1;
        t = ((Actor *)p)->GetChestPos();
        break;
    case 2:
    case 3:
    case 5:
    case 6:
        if (!p) return 1;
        t = ((EXItem *)p)->Position();
        if (type == 2) t.y += 0.5f;
        break;
    default:
        return 1;
    }
    const EXVector &pos = m_pItem->Position();
    float rot = m_pItem->Rotation().y;
    float s, c;
    EXSinCos(rot, s, c);
    EXMaths::vec_xyzw r;
    r.x = t.x - (s * 1.2f + pos.x);
    r.y = t.y - (pos.y + 1.4f);
    r.z = t.z - (pos.z + c * 1.2f);
    r.w = t.w - pos.w;
    EXMaths::vec_xyzw r2 = r;
    EXVector e(r2);
    EXMaths::vec_xyzw w = e;
    if (CalculateAngle(0.375f, EXSqrt(w.x * w.x + w.z * w.z), w.y, 0.01f, 1) > 90.0f) return 0;
    return 1;
}

// Ballistic launch angle for a projectile of the given speed to hit a target dist away and height above,
// under gravity grav (low arc, or high arc when bHigh). Returns 100 when the target is out of range.
// NEAR: discriminant scheduling (retail spills the first partial sum to the speed slot; VC keeps v^4 differently).
float CalculateAngle(float speed, float dist, float height, float grav, signed char bHigh)
{
    float v2 = speed * speed;
    float d2 = dist * dist;
        float v4 = v2 * v2;
    float disc = -(grav * grav * d2);
    disc += v2 * height * grav * -2.0f;
    disc += v4;
    disc *= d2 * d2 * v4;
    float h2 = height * height;
    signed char bNeg = 0;
    if (0.0f > disc) return 100.0f;
    float t;
    if (bHigh) {
        float n, d;
        if (height < 0.0f) {
            n = (height * grav + v2) * d2 * v2 + EXSqrt(disc);
            d = d2 - h2;
        } else {
            n = (-(height * grav) + v2) * d2 * v2 + EXSqrt(disc);
            d = h2 + d2;
        }
        d *= v4;
        if (!(d > 0.0f) || n / d > 2.0f) bNeg = 1;
        t = (-(height * grav) + v2) * d2 * v2 + EXSqrt(disc);
    } else {
        t = (-(height * grav) + v2) * d2 * v2 - EXSqrt(disc);
    }
    t /= (h2 + d2) * v4;
    if (t < 0.0f) return 100.0f;
    if (bHigh) {
        float a = EXACos((float)(EXSqrt(t) / sqrt(2.0)));
        if (bNeg) return -a;
        return a;
    }
    return EXACos((float)(EXSqrt(t) / sqrt(2.0)));
}

signed char XItemHandler_Player::IsHoldingActor()
{
    if (m_Mode == 0x16) return 1;
    return m_HoldState == 1;
}

signed char XItemHandler_Player::UBBing()
{
    EXItemAnimator_Anim *pAnim = (EXItemAnimator_Anim *)m_pItem->GetAnimator(EXRUNTIME_CLASS(EXItemAnimator_Anim));
    if (pAnim->Controller(1)) return 1;
    return 0;
}

// NEAR: retail tests the low byte directly (and al, 1); ours zero-extends first.
signed char XItemHandler_Player::HasLanded()
{
    return (s8)(((XItemPhysics *)m_pItem->ItemPhysics())->CollisionFlags() & 1);
}

// NEAR (see check): a wall to slam a held drone into: a vertical wall in front (within 45 degrees) that is
// flat over a 1.2-wide, 0.2 to 2.0 high patch (four rays from 0.6 off the wall must all hit it at once);
// lines the player up 1.2 from the wall facing it.
signed char XItemHandler_Player::CheckForWallSlam()
{
    EXItem *pMe = m_pItem;
    float ry = pMe->Rotation().y;
    float s = (float)sin(ry);
    float c = (float)cos(ry);
    EXSphere sph;
    sph.m_centre = EXVector(pMe->Position().x + s * 0.1f, pMe->Position().y + 1.1f, pMe->Position().z + c * 0.1f, pMe->Position().w);
    sph.m_radius = 1.5f;
    signed char bWall, bFace;
    EXVector wallPos, wallNormal, facePos, faceNormal;
    if (!((XItemPhysics *)pMe->m_pItemPhysics)->CollideSphereVertWall(sph, bWall, wallPos, wallNormal, bFace, facePos, faceNormal)) return 0;
    pMe = m_pItem;
    if (!bWall) return 0;
    float a = (float)atan2(wallNormal.x, wallNormal.z);
    if (!((float)fabs(Vec_AngleDifference(a, pMe->Rotation().y + 3.1415927f)) < 0.78539819f)) return 0;
    float y = pMe->Position().y;
    EXVector n = wallNormal * 0.6f;
    EXVector pts[5];
    pts[0].x = n.z + wallPos.x;
    pts[2].x = n.z + wallPos.x;
    pts[0].z = wallPos.z - n.x;
    pts[2].z = wallPos.z - n.x;
    pts[0].y = y + 0.2f;
    pts[1].y = y + 0.2f;
    pts[0].w = wallPos.w;
    pts[1].w = wallPos.w;
    pts[1].x = wallPos.x - n.z;
    pts[1].z = wallPos.z + n.x;
    pts[2].w = wallPos.w;
    pts[3].x = wallPos.x - n.z;
    pts[3].z = wallPos.z + n.x;
    pts[3].w = wallPos.w;
    pts[2].y = y + 2.0f;
    pts[3].y = y + 2.0f;
    pts[4].x = n.x + wallPos.x;
    pts[4].y = n.y + (y + 1.1f);
    pts[4].z = n.z + wallPos.z;
    pts[4].w = n.w + wallPos.w;
    EXRayCastTemp r;
    for (int i = 0; ; i++) {
        EXVector d = (pts[i] - pts[4]) * 10.0f;
        r.Set(pts[4], d, 0, 0, 0, 0);
        r.m_mask = 8;
        theItemEnv.DoRayCastToMap(r, 0);
        theItemEnv.DoRayCastToContoured(r);
        if (!r.m_bHit || (float)fabs(r.m_t - 0.1f) > 0.01f) return 0;
        if (i + 1 > 3) break;
    }
    a += 3.1415927f;
    m_TargetAdjustTime = a;
    m_TargetAdjust.w = 0.0f;
    m_TargetAdjust.y = 0.0f;
    m_TargetAdjust.x = (float)sin(a) * -1.2f;
    m_TargetAdjust.z = (float)cos(a) * -1.2f;
    m_TargetAdjust += wallPos;
    return 1;
}

// NEAR (see check): the gecko facing on the wall: the line where the wall plane meets the plane through
// the player across his side axis is aligned to, then turned a quarter about x.
signed char XItemHandler_Player::GetGeckoValidPoint()
{
    EXPlane wall(m_GeckoPoint, m_GeckoNormal);
    EXVector side(1.0f, 0.0f, 0.0f, 0.0f);
    side = EXVector(m_GeckoAngle.MultiplyVector(side));
    EXPlane across;
    across = EXPlane(m_pItem->Position(), side);
    EXLine l;
    across.Intersection(wall, l);
    EXMatrix m;
    m.AlignToDir(l.m_Direction, m_GeckoNormal, m_GeckoNormal);
    EXQuaternion q;
    q.SetRotation(m);
    m_GeckoValidAngle = q;
    EXMaths::quat_xyzw r;
    EXVector ra(1.5707964f, 0.0f, 0.0f, 0.0f);
    r.SetRotation(ra, 4);
    m_GeckoValidAngle *= r;
    return 1;
}

void XItemHandler_Player::CheckModeChange(unsigned int mode)
{
    EXItemAnimator_Anim *pAnim = (EXItemAnimator_Anim *)m_pItem->GetAnimator(EXRUNTIME_CLASS(EXItemAnimator_Anim));
    unsigned int cur = ((XItem *)m_pItem)->m_Unk160;
    if (mode != cur) {
        pAnim->ChangeToAnimMode(cur, mode, 0x9000004);
        ((XItem *)m_pItem)->m_Unk160 = mode;
    }
}

void XItemHandler_Player::CheckForVehicleHit() {}

signed char XItemHandler_Player::GetCloak() { return m_bCloak; }

void XItemHandler_Player::SetCloakDirect(signed char b)
{
    if (b) {
        m_bCloak = 1;
        m_CloakState = 2;
    } else {
        m_bCloak = 0;
        m_CloakState = 0;
    }
}

void XItemHandler_Player::BreakCloak(int n)
{
    m_bCloakBroken = 1;
    m_CloakBreak = n;
}

void XItemHandler_Player::SetVisionMode(unsigned char mode)
{
    if (mode != m_Tint) VisionWipe::Instance(1)->Start(mode);
}

void XItemHandler_Player::DeleteSwoosh(unsigned int hash, EXItemAnimator_Entity *pEntity)
{
    EXSwooshSys *p = gpGameWnd[0]->SwooshSysMgr()->FindSwoosh(hash, pEntity);
    if (p) ((EXBaseSwooshSys *)p)->KillSwoosh();
}

void XItemHandler_Player::DeleteAllSwoosh() { gpGameWnd[0]->SwooshSysMgr()->DeleteAllSystems(m_pItem); }

signed char XItemHandler_Player::GetAttackState(int &a, int &b)
{
    if (m_Mode != 6) return 0;
    if (m_SubMode) return 0;
    a = m_AttackType;
    b = m_AttackState;
    return 1;
}

signed char XItemHandler_Player::CanAssignWeapon()
{
    EXItemAnimator_Anim *pAnim = (EXItemAnimator_Anim *)m_pItem->GetAnimator(EXRUNTIME_CLASS(EXItemAnimator_Anim));
    if (pAnim->Controller(1)) return 0;
    unsigned int mode = m_Mode;
    if (mode >= 6 && (mode <= 7 || mode == 0x17)) return 0;
    return 1;
}

void XItemHandler_Player::VehicleRemoved(Vehicle *pVehicle)
{
    if (!this) return;
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->ItemPhysics();
    if (pPhys->m_pHitVehicle == pVehicle) pPhys->m_pHitVehicle = 0;
    if (pVehicle->m_pItem == m_pStandOn) m_pStandOn = 0;
}

void XItemHandler_Player::PickedUpWeapon(int id)
{
    m_PickupTimer = 5.0f;
    m_PickupWeapon = id;
}

void XItemHandler_Player::AddWeaponLoad(unsigned int id)
{
    int i;
    for (i = 0; i < m_nWeaponLoads; i++)
        if (WeaponLoadList[i].m_Id == id) return;
    WeaponLoadList[i].m_Id = id;
    m_nWeaponLoads++;
}

void XItemHandler_Player::InitPlayerBodyWeapons()
{
    _ATTACH_DATA *p = &Attach_Data[0];
    for (int i = 5; i != 0; i--, p++) {
        p->m_Avail = 0;
        p->m_bShown = 0;
        p->m_bLoaded = 0;
        FEHud *pHud = FEHud::Instance(1);
        if (pHud->IsWeaponAvailable(p->m_WeaponId)) p->m_Avail = 1;
    }
}

signed char XItemHandler_Player::HoldingHeavyWeapon()
{
    if ((int)m_CurrentWeapon[1] >= 2 && (int)m_CurrentWeapon[1] <= 4) return 1;
    return 0;
}

// NEAR (see check): the weapons carried on the body. Each collected one gets its upgrade's model (loaded
// on change), is hidden while it is in hand (the disk also while not held), and otherwise is placed on its
// body datum turned by its euler offset and pushed out along the datum's z (the disk also up by the
// skin's disk height).
void XItemHandler_Player::UpdatePlayerBodyWeapons()
{
    if (!m_bWeaponsLoaded) return;
    for (int i = 0; i < 5; i++) {
        _ATTACH_DATA &a = Attach_Data[i];
        if (!a.m_Avail) continue;
        int wid = (int)a.m_WeaponId;
        int lvl = (wid < 0 || wid > 7) ? 0 : m_Unkcf0[wid];
        unsigned int hash = Weapon_Shapes[i].m_Hash[lvl];
        if (!a.m_bLoaded || a.m_ModelHash != hash) {
            EXGeoFile *pFile = EXGeoFile::LoadGeoFile(0x1000013, 0, 0);
            m_pWeaponItem[a.m_Item]->Initialise(pFile, hash);
            m_pWeaponItem[a.m_Item]->RenderInfo().m_renderFlags.BitSet(0x1000);
            EXItemAnimator_Entity *pW = m_pWeaponItem[a.m_Item];
            pW->RenderInfo().SetVertexSet(a.m_VertexSet, a.m_VertexSet, 0.0f);
            a.m_bLoaded = 1;
            a.m_ModelHash = hash;
        }
        int id = a.m_WeaponId;
        signed char bInHand = 0;
        if ((m_MoveFlags & 0x9000) != 0x9000 && (m_HandWeapon[0] == id || m_HandWeapon[1] == id)) bInHand = 1;
        if ((id == 6 && !m_bDiskInHand) || bInHand) {
            if (a.m_bShown == 1) {
                m_pWeaponItem[a.m_Item]->ClrHasRender();
                a.m_bShown = 0;
            }
        } else if (!a.m_bShown) {
            m_pWeaponItem[a.m_Item]->SetHasRender();
            a.m_bShown = 1;
        }
        if (!a.m_bShown) continue;
        EXDatumTemp d;
        if (!m_pItem->GetAnimatorDatum(a.m_DatumHash, d, 5, 0)) continue;
        EXMatrix m;
        d.m_rotation.GetRotation(m);
        EXQuaternion q;
        q.SetRotation(m);
        EXVector e(a.m_Rot[0], a.m_Rot[1], a.m_Rot[2], 0.0f);
        EXMaths::quat_xyzw r;
        r.SetRotation(e, 4);
        q *= r;
        EXVector rot;
        q.GetRotation(rot, 4);
        EXItemAnimator_Entity *pW = m_pWeaponItem[a.m_Item];
        pW->m_rotation = rot;
        pW->m_validMtx = 0;
        EXVector off(0.0f, a.m_WeaponId == 6 ? PRED_Skins[m_SkinBC].m_DiskY : 0.0f, a.m_OffsetZ, 0.0f);
        EXVector o = q.MultiplyVector(off);
        EXVector p = d.m_position + o;
        pW = m_pWeaponItem[a.m_Item];
        pW->m_position = p;
        pW->m_validMtx = 0;
    }
}

// NEAR (see check): puts a hand weapon item on its hand's weapon datum. The right hand's shoulder-mounted
// set (weapon 6, 8-11) is moved 0.08 down the hand and turned 21 degrees about y.
void XItemHandler_Player::SetHandWeaponPosRot(int hand)
{
    EXDatumTemp d;
    EXVector rot;
    EXItemAnimator *pW;
    switch (hand) {
    case 0:
        if (!m_pItem->GetAnimatorDatum(0x10000019, d, 5, 0)) return;
        pW = (EXItemAnimator *)m_pWeaponItem[0];
        pW->m_position = d.m_position;
        pW->m_validMtx = 0;
        d.m_rotation.GetRotation(rot, 4);
        pW = (EXItemAnimator *)m_pWeaponItem[0];
        break;
    case 1: {
        if (!m_pItem->GetAnimatorDatum(0x1000001a, d, 5, 0)) return;
        d.m_rotation.GetRotation(rot, 4);
        int w = m_HandWeapon[1];
        if (w == 6 || (w > 7 && w < 0xc)) {
            EXVector off(0.0f, -0.08f, 0.0f, 0.0f), o;
            EXMatrix m;
            d.m_rotation.GetRotation(m);
            D3DXVec4Transform(&o, &off, &m);
            d.m_position += o;
            EXQuaternion q;
            q.SetRotation(m);
            EXVector ya(0.0f, 0.36651915f, 0.0f, 0.0f);
            EXMaths::quat_xyzw r;
            r.SetRotation(ya, 4);
            q *= r;
            q.GetRotation(rot, 4);
        }
        pW = (EXItemAnimator *)m_pWeaponItem[1];
        pW->m_position = d.m_position;
        pW->m_validMtx = 0;
        pW = (EXItemAnimator *)m_pWeaponItem[1];
        break;
    }
    default:
        return;
    }
    pW->m_rotation = rot;
    pW->m_validMtx = 0;
}

int XItemHandler_Player::GetMaxAmmo(int weapon)
{
    switch (weapon) {
    case 5: return (m_Unkcf0[5] + 1) * 10;
    case 0xc: return 3;
    }
    return GetMaxAmmo();
}

int XItemHandler_Player::GetFaveWeapon()
{
    int fave = -1;
    int best = 0;
    for (int i = 0; i < 14; i++) {
        if (m_WeaponUse[i] > best) {
            fave = i;
            best = m_WeaponUse[i];
        }
    }
    return fave;
}

unsigned int XItemHandler_Player::GetCurrentSkinHash() { return PRED_Skins[m_SkinBC].m_Hash[m_SkinC0 + 1]; }

int XItemHandler_Player::WeaponIdToSlot(int id)
{
    for (int i = 0; i < 4; i++)
        if (m_CurrentWeapon[i] == (unsigned int)id) return i;
    return 3;
}

void XItemHandler_Player::InitWristBladeGFX()
{
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(0x1000013, 0, 0);
    ((EXItemAnimator *)m_pWeaponItem[7])->Initialise(pFile, WristBladeExtendHash[m_Unkcf0[1]]);
    ((EXItemAnimator *)m_pWeaponItem[7])->SetHasRender();
}

// NEAR (see check): eases the player onto the grab point while m_Unk360 runs down (60 per second): the
// offset from the grab vector to the clamber point is spread over it on the first frame; x and z
// rotation are levelled by up to a degree a frame.
void XItemHandler_Player::HandleGrabAdjustment()
{
    m_Unk2b8.SetZero();
    if (!(m_Unk360 > 0.0f)) return;
    if (m_bUnk364) {
        m_bUnk364 = 0;
        EXVector v;
        GetGrabVector(0, v);
        m_GrabStep = v - m_ClamberPos;
        m_GrabBase = m_pItem->Position() - m_GrabStep;
        m_GrabStep *= 1.0f / m_Unk360;
    }
    m_Unk360 -= XApp::FrameLength() * 60.0f;
    if (m_Unk360 < 0.0f) m_Unk360 = 0.0f;
    EXVector p = m_GrabBase + m_GrabStep * m_Unk360;
    m_pItem->m_position = p;
    m_pItem->m_validMtx = 0;
    EXPlacementItem *pItem = m_pItem;
    EXVector rot = pItem->Rotation();
    float r = XApp::GameFrameRateMul() * 0.017453292f;
    float dx = Vec_AngleDifference(rot.x, 0.0f);
    float sx = dx < r ? (-r < dx ? dx : -r) : r;
    float dz = Vec_AngleDifference(rot.z, 0.0f);
    float sz = dz < r ? (-r < dz ? dz : -r) : r;
    rot.z = sz + rot.z;
    rot.x = sx + rot.x;
    pItem->m_rotation = rot;
    pItem->m_validMtx = 0;
    m_RootPosStart = p;
}

void XItemHandler_Player::ResetWeaponDown()
{
    m_WeaponDown = 0.0f;
    m_WeaponDownT[0] = -1.0f;
    m_WeaponDownT[1] = -1.0f;
    m_WeaponDownT[2] = -1.0f;
    m_WeaponDownT[3] = -1.0f;
}

void XItemHandler_Player::LevelWeaponUpgradeLoad(int i) { LoadUpgradeWeaponGFX(i, 1); }

void XItemHandler_Player::SetWeaponUpgrade(int i, int n)
{
    if (i >= 0 && i < 8 && n >= 0 && n <= 2) m_Unkcf0[i] = n;
}

void XItemHandler_Player::SetSpecialUpgrade(int i, int n)
{
    if (i < 0 || i >= 5) return;
    if (i != 4) {
        if (n >= 0 && n <= 3) m_SpecialUpgrades[i] = n;
        return;
    }
    m_SpecialUpgrades[4] |= n;
}

void XItemHandler_Player::AddSpecialUpgrade(int i, int n)
{
    if (i >= 0 && i < 5) {
        m_SpecialUpgrades[i] += n;
        PlayerClampMax(m_SpecialUpgrades[i], 3);
    }
}

void XItemHandler_Player::ForceSafeWeapon() { SetWeapon(3); }

XItemHandler_Player *XItemHandler_Player::CreateObject() { return new XItemHandler_Player; }

// NEAR (see check): keeps the player centred under monkey bars: three unit-radius polygon probes at
// 120 degree spacing around him (behind, then either side) each push him to 0.5 from any bar further
// than that; once hanging (submode > 2) his height follows the average bar height (hang 2.39 below).
// The total shift goes into this frame's PFDeltaMove.
signed char XItemHandler_Player::MonkeyCheckAround(EXVector &out)
{
    EXVector p = m_pItem->Position();
    float a = -3.1415927f;
    float sumY = 0.0f;
    out.SetZero();
    p.y = m_ClamberPos.y;
    for (int i = 3; ; ) {
        float r = a + m_pItem->Rotation().y;
        EXVector t(p.x + (float)sin(r), p.y, p.z + (float)cos(r), p.w);
        EXGrabPointTemp g;
        g.Set(t, 1.0f, (EXGrabPointTemp::GRABFITYPE)2);
        theItemEnv.DoGrabPointToMap(g);
        if (g.m_24 == 2 || g.m_24 == 4) {
            EXVector d = g.m_30 - t;
            sumY += g.m_30.y;
            d.y = 0.0f;
            d.w = 0.0f;
            if (d.LengthXZ() > 0.5f) {
                EXVector off = d - d.UnitVector() * 0.5f;
                p += off;
                out += off;
            }
        }
        i--;
        a += 2.0943952f;
        if (i == 0) break;
    }
    if (m_SubMode > 2) {
        sumY *= 0.33333334f;
        m_ClamberPos.y = sumY;
        out.y = sumY - (m_pItem->Position().y + 2.39f);
    }
    ((XItemPhysics *)m_pItem->m_pItemPhysics)->m_PFDeltaMove += out;
    return 1;
}

signed char XItemHandler_Player::PerchCheckAhead(EXGrabPointTemp &) { return 0; }

// NEAR (see check): hanging from a ledge (mode 0xc). Wall jump off (button 0x10), drop (stick hard down,
// crouch, death), pull up into a clamber when there is room, shimmy, and the idle hang variations.
void XItemHandler_Player::DoHang()
{
    float t = 20.0f;
    signed char bDrop = !(m_Health > 0.0f);
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
    HandleGrabAdjustment();
    if (m_ButtonsHeld & 0x10) {
        switch (m_SubMode) {
        case 0: case 2: case 5: case 8: case 9:
            if (!(m_StateFlags & 0x40)) {
                m_HandLock = -1.0f;
                pPhys->m_physicsFlags &= ~3;
                SetupMove(10, 2, -1.0f);
                m_Unk308 = 1.0f;
                return;
            }
        }
    }
    if (m_Pad.z < -0.9f) {
        switch (m_SubMode) {
        case 0: case 2: case 5:
            break;
        case 8: case 9:
            t = 3600.0f;
            break;
        default:
            goto jump;
        }
        m_HandLock = -1.0f;
        bDrop = 1;
    }
jump:
    if (m_ButtonsHeld & 8) {
        switch (m_SubMode) {
        case 8: case 9:
            t = 10.0f;
        case 0: case 2: case 5:
            m_ButtonsHeld &= ~8;
            m_ButtonsPressed &= ~8;
            m_ButtonsReleased &= ~8;
            m_HandLock = -1.0f;
            goto drop;
        }
    }
    if (bDrop) {
drop:
        pPhys->m_physicsFlags &= ~3;
        pPhys->m_DeltaMove.y = 0.0f;
        m_Unk358 = t * 0.016666668f;
        SetupMove(9, 0, -1.0f);
        return;
    }
    switch (m_SubMode) {
    case 0: case 2: case 5: {
        if (m_Unk360 > 0.0f) return;
        char up = m_Pad.z > 0.9f;
        if (m_ButtonsHeld & 0x20) up = 2;
        if (up) {
            if (!(m_StateFlags & 0x10) && CheckForClamberSpace()) {
                unsigned int fl = m_StateFlags;
                m_HandLock = -1.0f;
                if (fl & 0x100) {
                    SetupMove(4, (fl & 0x40) ? 9 : 10, -1.0f);
                    m_HangYaw = m_pItem->Rotation().y;
                    return;
                }
                if (fl & 0x80) {
                    if (fl & 0x40) SetupMove(4, 8, -1.0f);
                    else SetupMove(4, 2, -1.0f);
                    return;
                }
                if (fl & 0x40) { SetupMove(4, 7, -1.0f); return; }
                if (up != 1) { SetupMove(4, 1, -1.0f); return; }
                SetupMove(4, 0, -1.0f);
                return;
            }
            m_ButtonLock = 0x20;
            m_AnalogLock = 1;
            EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00576, m_pItem->Position(), -1, 0);
        }
        if ((m_Unk360 > 0.0f || !CheckForShimmy(1)) && AnimEnded()) {
            if (m_SubMode == 0 && g_EXRandClass.Rand32() % 3 == 0) {
                SetupMove(0xc, 2, -1.0f);
                return;
            }
            if (m_SubMode == 2) SetupMove(0xc, 0, -1.0f);
        }
        break;
    }
    case 3: case 6:
        if (AnimEnded()) {
settle:
            if (m_StateFlags & 0x40) SetupMove(0xc, 5, -1.0f);
            else SetupMove(0xc, 0, -1.0f);
            return;
        }
        break;
    case 4: case 7:
        GetGrabPoint(3);
        m_Unk360 = 1.0f;
        m_bUnk364 = 1;
        HandleGrabAdjustment();
        HandleShimmyAngle(m_ClamberPos, m_pItem->Rotation().y + 1.5707964f, 1, t);
        if (AnimEnded() && !CheckForShimmy(0)) goto settle;
        break;
    case 8: case 9:
        if (AnimEnded() && m_SubMode == 8 && g_EXRandClass.Rand32() % 3 == 0) {
            SetupMove(0xc, 9, -1.0f);
            return;
        }
        if (m_Pad.z > 0.9f || (m_ButtonsHeld & 0x20)) {
            if (m_ClamberSide) SetupMove(4, 5, -1.0f);
            else SetupMove(4, 6, -1.0f);
        }
        break;
    }
}

// NEAR (see check): wall moves from a sphere test ahead of the chest (1.25 up, 0.1 forward, radius 1.5).
// Facing a wall within 90 degrees: crouch (button 0x20) side-kicks off it to the left/right when it is
// more than 22.5 degrees off, button 0x10 wall jumps straight; both once a second (m_Unk35c 60).
// Otherwise, with a near-vertical wall face (normal y within 0.6) ahead and not rising, a clear line to the
// contact point starts the gecko wall crawl (mode 0x18) on it.
signed char XItemHandler_Player::CheckWallMove()
{
    EXItem *pMe = m_pItem;
    float ry = pMe->Rotation().y;
    XItemPhysics *pPhys = (XItemPhysics *)pMe->m_pItemPhysics;
    EXVector off(sinf(ry) * 0.1f, 0.0f, cosf(ry) * 0.1f, 0.0f);
    EXVector c(pMe->Position().x + off.x, pMe->Position().y + 1.25f, pMe->Position().z + off.z, pMe->Position().w);
    EXSphere sph;
    sph.Set(c, 1.5f);
    signed char bWall, bFace;
    EXVector wallPos, wallNormal, facePos, faceNormal;
    if (!pPhys->CollideSphereVertWall(sph, bWall, wallPos, wallNormal, bFace, facePos, faceNormal)) return 0;
    float yaw = m_pItem->Rotation().y;
    if (bWall) {
        int b = 0;
        if (m_ButtonsHeld & 0x20) b = 1;
        if (m_ButtonsHeld & 0x10) b = 2;
        else if (b <= 0) goto gecko;
        if (m_Unk35c > 0.0f) return 0;
        float a = (float)atan2(wallNormal.x, wallNormal.z);
        float d = Vec_AngleDifference(a, yaw + 3.1415927f);
        if ((float)fabs(d) >= 1.5707964f) return 0;
        if ((float)fabs(c.y - wallPos.y) >= 0.1f) return 0;
        m_Unk308 = 1.0f;
        if (d < -0.39269909f || d > 0.39269909f) {
            if (b != 1) goto gecko;
            SetupMove(10, d < -0.39269909f ? 0 : b, -1.0f);
            m_Unk34c = a - d;
        } else {
            if (b != 2) goto gecko;
            SetupMove(10, b, -1.0f);
            m_Unk34c = a;
        }
        m_Unk350 = a;
        m_Unk35c = 60.0f;
        DeleteAllSwoosh();
        return 1;
    }
gecko:
    if (!bFace) return 0;
    if (m_Unk358 > 0.0f) return 0;
    if ((float)fabs(faceNormal.y) > 0.6f) return 0;
    if (pPhys->m_DeltaMove.y >= 0.2f) return 0;
    {
        float back = yaw + 3.1415927f;
        if ((float)fabs(Vec_AngleDifference((float)atan2(faceNormal.x, faceNormal.z), back)) >= 1.5707964f) return 0;
        EXVector v = facePos - c;
        if ((float)fabs(Vec_AngleDifference((float)atan2(v.x, v.z), back)) < 1.5707964f) return 0;
        EXRayCastTemp r;
        r.Set(c, v, 0, 0, 0, 0);
        r.m_mask = 0x18;
        r.m_bAnyHit = 1;
        theItemEnv.DoRayCastToMap(r, 0);
        if (r.m_bHit) return 0;
        m_GeckoPoint = facePos;
        m_GeckoNormal = faceNormal;
        EXMatrix m;
        m.set_rotate_xyz(m_pItem->Rotation());
        EXQuaternion q;
        q.SetRotation(m);
        m_GeckoAngle = q;
        GetGeckoValidPoint();
        m_GeckoAngle = m_GeckoValidAngle;
        EXQuaternion fq;
        EXPlane pl;
        if (GetGeckoFormQuart(fq, pl, 0, 0.0f, 0.0f) != 7) return 0;
        float nw = 0.0f;
        float dist = m_GeckoPoint.y * pl.B + m_GeckoPoint.z * pl.C + m_GeckoPoint.w * nw + m_GeckoPoint.x * pl.A + pl.D;
        m_GeckoPoint -= pl.Normal() * dist;
        m_GeckoAngle = fq;
        DeadStop(0);
        EXBaseSoundManager::GetSoundManager()->StartSound(0x1af0020e, m_pItem->Position(), -1, 0);
        SetupMove(0x18, 0, -1.0f);
        DeleteAllSwoosh();
        return 1;
    }
}

void XItemHandler_Player::DoPreJump()
{
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->ItemPhysics();
    if (AnimEnded()) {
        pPhys->SetThrustToAttainHeight(3.217f);
        SetupMove(3, 1, -1.0f);
    }
}

void XItemHandler_Player::LandedOnVehicle()
{
    Vehicle *pVehicle;
    EXItem *pOwner;
    if (m_pStandOn && (pOwner = m_pStandOn->m_pXItemOwner) != 0 && pOwner->IsKindOf(EXRUNTIME_CLASS(Vehicle)))
        pVehicle = (Vehicle *)pOwner;
    else
        pVehicle = 0;
    pVehicle->landedOn();
    if (pVehicle->m_vehFlags & 1) Objective_NotifyPlayerSeen(pVehicle);
}

int XItemHandler_Player::GetUpgradedSkin()
{
    int skin = m_SelectedSkin >= 0 ? m_SelectedSkin : 0;
    switch (gCurrentMap_GeoHash) {
    case 0x100006a:
    case 0x10000b4:
    case 0x1000136:
        if (!m_bHelmet) skin++;
        return skin;
    }
    m_bHelmet = 1;
    return skin;
}

float CalculateVelocity(float a, float b, float c)
{
    float v = b * c / (float)sin(a + a);
    if (0.0f > v) return 0.0f;
    return EXSqrt(v);
}

void XItemHandler_Player::ForceModeChange(unsigned int mode)
{
    m_bForceModeChange = 1;
    EXItemAnimator_Anim *pAnim = (EXItemAnimator_Anim *)m_pItem->GetAnimator(EXRUNTIME_CLASS(EXItemAnimator_Anim));
    pAnim->ChangeToAnimMode(((XItem *)m_pItem)->m_Unk160, mode, 0x9000004);
    ((XItem *)m_pItem)->m_Unk160 = mode;
}

int XItemHandler_Player::HoldingWeapon(int id)
{
    if (HasMoveFlag(0x9000)) return -1;
    if (m_HandWeapon[0] == id) return 0;
    return m_HandWeapon[1] == id ? 1 : -1;
}

void XItemHandler_Player::LockHand(int idx, int t, signed char b)
{
    if (!idx) {
        UnlockHand();
        return;
    }
    m_bHandLockFlag = b;
    m_LockHandIdx = idx;
    GetHandLockPos(idx, m_HandLockPos);
    m_HandLock = (float)t;
}

signed char XItemHandler_Player::CheckDeadFall()
{
    if (m_Health <= 0.0f) {
        m_DeadFallTimer -= XApp::FrameLength();
        if (m_DeadFallTimer < 0.0f) {
            theApp.SetMissionDead();
            return 1;
        }
    }
    return 0;
}

void XItemHandler_Player::EMPPlayer()
{
    if (m_EMPTimer <= 0.0f) {
        m_SavedEnergy = m_Energy;
        m_Energy = 0.0f;
    }
    m_EMPTimer = 5.0f;
    if (m_Tint) VisionWipe::Instance(1)->Start(0);
}

void XItemHandler_Player::SetDiskInHand(signed char b)
{
    if (m_bDiskInHand == b) return;
    m_bDiskInHand = b;
    if (HasMoveFlag(0x9000)) return;
    if (m_HandWeapon[0] == 6 || m_HandWeapon[1] == 6) UpdateWeaponGFX(0, 0.0f);
}

void XItemHandler_Player::SetCurrentWeapon(int slot, int weapon)
{
    if (m_CurrentWeapon[slot] == (unsigned int)weapon) return;
    m_CurrentWeapon[slot] = weapon;
    if (slot == m_CurrentSlot) SetWeapon(3);
    UpdateWeaponGFX(slot, 0.0f);
    UpdateSectionFiles();
}

void XItemHandler_Player::SendPredMsg(int msg, EXVector &v, float f)
{
    switch (msg) {
    case 5:
        ReactExplode(v, f);
        break;
    case 7:
        InflictDamage(f, 2);
        ReactStumble(v);
        break;
    }
}

void XItemHandler_Player::InflictDamageFromFall(float speed)
{
    float d = (0.0f - speed - 1.0435f) * 39.439262f + 5.0f;
    if (!(4.0f > d)) InflictDamage(d, 3);
}

int XItemHandler_Player::GetWeaponSection(int weapon)
{
    switch (weapon) {
    case 2: return 2;
    case 3: return 3;
    case 4: return 4;
    case 5: return 5;
    case 6: return 6;
    }
    return 0;
}

void XItemHandler_Player::SafeBreakoutHeadRip()
{
    if (m_Mode == 7 && m_SubMode == 0) {
        Actor *pActor = Actor::GetPtrFromID(m_SpecialActorID);
        if (pActor) {
            MsgItem msg((MsgType)0x34);
            pActor->SendMsg(msg, 0);
            m_SpecialActorID = 0;
        }
    }
}

void XItemHandler_Player::SetTargetAdjustment(EXVector v, float t, int type)
{
    m_TargetAdjust.x = v.x;
    m_TargetAdjust.y = v.y;
    m_TargetAdjust.z = v.z;
    m_TargetAdjust.w = v.w;
    m_TargetAdjustTime = t;
    m_bTargetAdjust = 1;
    m_TargetAdjustType = (float)type;
}

signed char XItemHandler_Player::CheckForClamberSpace()
{
    EXVector p = m_ClamberPos;
    return CheckForActorSpace(p, 1.5f, 1.5f);
}

void XItemHandler_Player::ResetAttack()
{
    for (int i = 0; i < 7; i++) Combo_Seq[i].m_Pos = 0;
    gpGameWnd[0]->SwooshSysMgr()->DeleteAllSystems(m_pItem);
    m_bAttacking = 0;
    m_AttackState = 0;
    m_AttackHit = -1;
}

void XItemHandler_Player::CommitVisionMode(unsigned char mode)
{
    m_Tint = mode;
    HandleVisionModeTextureSwaps(mode);
    SetFog(mode);
    for (Actor *p = Actor::GetFirst(1); p; p = p->GetNext(1)) ActorTextureSwap::Apply(p);
    Update_VMO(mode);
    XItemHandler_ScriptBase::TintAllBloodToVisionMode(mode);
}

void XItemHandler_Player::InitUpgrades()
{
    for (int i = 0; i < 8; i++) m_Unkcf0[i] = 0;
    for (int j = 0; j < 5; j++) m_SpecialUpgrades[j] = 0;
}

void XItemHandler_Player::SendPredMsg(int msg, unsigned int n)
{
    switch (msg) {
    case 3:
        m_PredMsgFlags |= 1 << n;
        break;
    case 4:
        m_PredMsgFlags &= ~(1 << n);
        break;
    default:
        SendPredMsg(msg, (float)n);
        break;
    }
}

void XItemHandler_Player::SendPredMsg(int msg, EXVector &a, EXVector &b, float f)
{
    switch (msg) {
    case 0:
        InflictDamage(f, a, b, 0);
        break;
    case 2:
        InflictDamage(f, a, b, 4);
        break;
    case 7:
        InflictDamage(f, a, b, 2);
        ReactStumble(a);
        break;
    }
}

float ClampInertiaToBlendAnim(float f)
{
    if (0.0001f >= f) return 0.0f;
    if (0.45f >= f) return 1.0f;
    if (0.9f >= f) return 2.0f;
    if (1.0f >= f) return 3.0f;
    if (2.0f >= f) return 4.0f;
    return 0.0f;
}

// NEAR: retail keeps both constant multiplies (f * 2.2222f * 0.01171875f); VC folds them into one here.
float InertiaToRunSpeed(float f)
{
    if (0.0001f >= f) return 0.0f;
    if (0.45f >= f) return f * 2.2222223f * 0.01171875f;
    if (0.9f >= f) return (f - 0.45f) * 2.2222223f * 0.03515625f + 0.01171875f;
    return (f - 0.9f) * 10.0f * 0.046875f + 0.046875f;
}

static inline float PlayerWrapAngle(float a)
{
    if (a >= 0.0f && a < 6.2831855f) return a;
    a = fmod(a, 6.2831855f);
    if (a < 0.0f) a += 6.2831855f;
    return a;
}

// NEAR: retail stores the fmod result with fstp and reloads it for the sign test (fst + compare here).
float Vec_AngleDifference(float a, float b)
{
    float wb = PlayerWrapAngle(b);
    float wa = PlayerWrapAngle(a);
    float d = wb - wa;
    if (3.1415927f > (float)fabs(d)) return d;
    if (d < 0.0f) return d + 6.2831855f;
    return d - 6.2831855f;
}

int XItemHandler_Player::GetPlasmaChargeSpeed(float &f)
{
    f = 1.25f;
    if (m_PlasmaChargeTime > 120.0f && m_Energy >= 50.0f) return 2;
    if (m_PlasmaChargeTime > 60.0f && m_Energy >= 30.0f) return 1;
    return 0;
}

// Blend helper (not in the map: always inlined). Arguments evaluate right to left.
template <class T> inline T PlayerBlend(const T &k, const T &a, const T &b) { return (a - b) * k + b; }
template <class T> inline const T &PlayerMax(const T &a, const T &b) { return a >= b ? a : b; }

// NEAR: retail spills both members and the powf result to the stack and blends with SSE.
float XItemHandler_Player::CalculateMSpeed(float)
{
    m_MSpeed = PlayerBlend(powf(0.75f, XApp::GameFrameRateMul()), m_MSpeed, m_MSpeedTarget);
    return 0.0f;
}

void XItemHandler_Player::SparkSkin(float t)
{
    m_SparkTimer = PlayerMax(m_SparkTimer, t);
    if (!m_bSparkSkin) {
        EXGeoRefObject ref;
        ref.m_pGeoFile = EXGeoFile::GetGeoFile(0x1000024);
        ref.m_hashRef = 0xd0000cc;
        ((EXItemAnimator_Anim *)m_pItem->GetAnimator(EXRUNTIME_CLASS(EXItemAnimator_Anim)))->AddCloneSkin(ref);
        m_bSparkSkin = 1;
    }
}

void XItemHandler_Player::DebugDrawPredCollision()
{
    for (int i = 1; i < 11; i++) {
        EXCapsule c;
        EXDatumTemp d;
        if (m_pItem->GetAnimatorDatum(CollisionHash[i], d, 0, 0)) d.Get(c);
    }
}

// NEAR (see check): the gecko roll angle as seen from the camera: where the camera's line through a
// point a unit above the wall point meets the wall plane (m_GeckoNormal as A, B, C, D), measured
// against the player's up axis, 0..2pi around the wall normal.
void XItemHandler_Player::GetGeckoAngle(float &angle)
{
    EXVector n(m_GeckoNormal.x, m_GeckoNormal.y, m_GeckoNormal.z, 0.0f);
    EXVector up(0.0f, 1.0f, 0.0f, 0.0f);
    EXVector u = EXVector(m_GeckoAngle.MultiplyVector(up));
    const EXVector &cam = XGetCamera(0)->m_CameraPos;
    EXVector dir = m_GeckoPoint + EXVector(0.0f, 1.0f, 0.0f, 0.0f) - cam;
    float l = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z + dir.w * dir.w;
    if (l >= 0.0f && (l = sqrtf(l)) > 1e-5f) {
        l = 1.0f / l;
        dir.x *= l;
        dir.y *= l;
        dir.z *= l;
    }
    float denom = m_GeckoNormal.w * 0.0f + dir.y * m_GeckoNormal.y + dir.z * m_GeckoNormal.z + dir.x * m_GeckoNormal.x;
    float t;
    if ((float)fabs(denom) >= 1e-10f)
        t = -(m_GeckoNormal.x * cam.x + m_GeckoNormal.z * cam.z + m_GeckoNormal.y * cam.y + m_GeckoNormal.w) / denom;
    EXVector v(dir.x * t + cam.x - m_GeckoPoint.x, dir.y * t + cam.y - m_GeckoPoint.y,
               dir.z * t + cam.z - m_GeckoPoint.z, t * 0.0f - m_GeckoPoint.w);
    l = v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
    if (l >= 0.0f && (l = sqrtf(l)) > 1e-5f) {
        l = 1.0f / l;
        v.w *= l;
        v.x *= l;
        v.y *= l;
        v.z *= l;
    }
    float cx = v.z * n.y - v.y * n.z;
    float cy = n.z * v.x - v.z * n.x;
    float cz = v.y * n.x - n.y * v.x;
    float d = v.w * u.w + v.z * u.z + v.y * u.y + v.x * u.x;
    float a = EXACos(d >= 1.0f ? 1.0f : (d <= -1.0f ? -1.0f : d));
    if (u.w * 0.0f + cz * u.z + cy * u.y + cx * u.x <= 0.0f) angle = a;
    else angle = 6.2831855f - a;
}

void XItemHandler_Player::SetupDepthPtr(float *p, unsigned int n)
{
    if (m_pAnim) {
        EXItemCtrl *pCtrl = m_pAnim->Controller(n);
        if (pCtrl && pCtrl->IsKindOf(EXRUNTIME_CLASS(EXItemCtrl_Anim))) ((EXItemCtrl_Anim *)pCtrl)->m_frameDepth.SetPtr(p);
    }
}

// NEAR (see check): the point to aim at: the camera's lock-on (an actor's chest, or an item's position,
// half a unit up for type 2). For the plasma caster (weapon 7) a lost lock falls back to the camera's
// plasma target, the actor's feet are used, and the point is led by the target's tracked speed against
// the bolt speed (EXGetPursuitAngles), then raised back to chest height; no target clears the tracking.
signed char XItemHandler_Player::GetTargetPos(EXVector &out, int weapon)
{
    int type;
    void *pLock = XGetCamera(0)->GetLockOn(type);
    Actor *pActor;
    switch (type) {
    case 0: {
        if (weapon != 7) return 0;
        int id = XGetCamera(0)->m_PlasmaID;
        if (id <= 0) goto clear;
        pActor = Actor::GetPtrFromID(id);
        pLock = pActor;
        type = 1;
        goto actor;
    }
    case 1:
        pActor = (Actor *)pLock;
actor:
        if (!pActor) break;
        if (weapon == 7) {
            out = pActor->GetItem()->Position();
            goto track;
        }
        out = pActor->GetChestPos();
        goto done;
    case 4:
        if (weapon != -1) break;
    case 2:
    case 3:
    case 5:
    case 6:
        pActor = 0;
        if (!pLock) break;
        out = ((EXItem *)pLock)->Position();
        if (type == 2) out.y += 0.5f;
done:
        if (weapon != 7) return 1;
track: {
        if (pLock != m_pTrackTarget) {
            m_pTrackTarget = pLock;
            m_TrackTime = 0.0f;
            m_TrackLastPos = out;
        }
        EXVector vel((out.x - m_TrackLastPos.x) * XApp::GameFrameRateDiv(), (out.y - m_TrackLastPos.y) * XApp::GameFrameRateDiv(),
                     (out.z - m_TrackLastPos.z) * XApp::GameFrameRateDiv(), (out.w - m_TrackLastPos.w) * XApp::GameFrameRateDiv());
        m_TrackTime += XApp::GameFrameRateMul();
        m_TrackLastPos = out;
        float s2 = vel.y * vel.y + vel.z * vel.z + vel.x * vel.x;
        float spd;
        if (s2 >= 0.0f && (spd = sqrtf(s2)) > 0.0001f) {
            float bolt;
            GetPlasmaChargeSpeed(bolt);
            EXVector from = m_pItem->Position();
            bolt *= 0.75f;
            EXVector dir(vel.x, vel.y, vel.z, 0.0f);
            if (s2 >= 0.0f && sqrtf(s2) > 1e-5f) {
                float l = 1.0f / sqrtf(s2);
                dir.y = vel.y * l;
                dir.z = vel.z * l;
                dir.x = l * vel.x;
                dir.w = l * 0.0f;
            }
            EXVector r;
            EXGetPursuitAngles(from, bolt, out, dir, spd, r);
            out = r;
        }
        if (pActor && !pActor->m_ActorFlags.m_bAlien) {
            EXItem *pItem = pActor->GetItem();
            out.y = pActor->GetChestPos().y - pItem->Position().y + out.y;
        }
        return 1;
    }
    }
    if (weapon == 7) {
clear:
        m_pTrackTarget = 0;
        m_TrackTime = 0.0f;
        m_TrackLastPos.SetZero();
    }
    return 0;
}

unsigned int XItemHandler_Player::DoScriptCmdObject(GEOSCRIPT_BASECMD *pCmd, EXItemAnimator *pAnim)
{
    u32 r = 0;
    if (pCmd->m_Type == 5) {
        EXItemAnimator *p = EXItemAnimator::CreateFrom(pAnim->GeoFile(), pCmd->m_Hash);
        if (p) {
            ((EXItemAnimator_Sound *)p)->SetSoundDetails(0.0f, 0.0f, pCmd->m_Reverb, 0, 0);
            pAnim->AddAnimator(p);
            p->DoUpdate();
        }
    }
    return r;
}

void XItemHandler_Player::RollActorRagDoll(Actor *pActor, int)
{
    EXItem *pItem = pActor->GetItem();
    if (pItem) {
        EX6DObject *p = (EX6DObject *)pItem->m_pItemPhysics;
        if (p->IsKindOf(EXRUNTIME_CLASS(EX6DAnim))) {
            for (int i = 0; i < 2; i++) {
                p->UpdateMovement(EXBaseApp::FrameLength());
                p->ClearForces();
                for (int j = 0; j < 6; j++) theItemEnv.m_p6DManager->SatisfyConstraints(EXBaseApp::FrameLength());
            }
        }
    }
}

template <class T> inline const T &PlayerMin(const T &a, const T &b) { return a <= b ? a : b; }

// NEAR: retail keeps max in a stack local and the weapon index in esi (register/slot choice).
signed char XItemHandler_Player::AddAmmo(int weapon, int n)
{
    int max = GetMaxAmmo(weapon);
    if (m_Ammo[weapon] >= max) return 0;
    n += m_Ammo[weapon];
    m_Ammo[weapon] = (short)PlayerMin(n, max);
    return 1;
}

void XItemHandler_Player::SetWeapon(int slot)
{
    int weapon = m_CurrentWeapon[slot];
    UpdateWeaponGFX(slot, 0.0f);
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(0x1000005, 0, 0);
    if (pFile->GeoHeader()->IsSectionLoaded(GetWeaponSection(weapon) + 0x8000000, 1) != 3) {
        m_CurrentSlot = 3;
        m_AttackType = m_CurrentWeapon[slot];
    } else {
        m_CurrentSlot = slot;
        m_AttackType = weapon;
    }
}

void XItemHandler_Player::SafeBreakoutAll(int n)
{
    SafeBreakoutHeadRip();
    RagDollHeldDrone(n);
    m_pStandOn = 0;
    if (m_pSpecialItem) {
        if (m_pSpecialItem->ItemHandler()->IsKindOf(EXRUNTIME_CLASS(XItemHandler_RechargePoint))) {
            XItemHandler_RechargePoint *p = (XItemHandler_RechargePoint *)m_pSpecialItem->ItemHandler();
            if (p) {
                p->EndRecharge();
                ((XGamePad *)EXBaseGamePad::GetGamePad())->StopRumbleID(0x54000003);
            }
        }
        m_pSpecialItem = 0;
    }
}

void XItemHandler_Player::InGameWeaponUpgradeAdd(int i)
{
    if (i >= 0 && i < 8) {
        m_Unkcf0[i]++;
        PlayerClampMax(m_Unkcf0[i], 2);
        if (i == 1)
            InitWristBladeGFX();
        else if (i == 7)
            InitPlasmaCasterGFX();
        else {
            UpdateWeaponUpgradeGFX(i);
            UpdateWeaponGFX(WeaponIdToSlot(i), 0.0f);
        }
        m_PickupTimer = 5.0f;
        m_PickupWeapon = i;
    }
}

void XItemHandler_Player::RefreshAllWeapons()
{
    InitWristBladeGFX();
    InitPlasmaCasterGFX();
    UpdateWeaponUpgradeGFX(-1);
    UpdateWeaponGFX(0, 0.0f);
    UpdateWeaponGFX(1, 0.0f);
    UpdateWeaponGFX(2, 0.0f);
    UpdateWeaponGFX(3, 0.0f);
}

// NEAR (see check): per-frame weapon dressing. Shows or hides the hand weapons with the move flags; runs
// each weapon shape's extend/retract morph (Weapon_Shapes: the wrist blades with the HUD, the hand weapons
// while held or attacking) with its sounds; places the wrist blades, the plasma caster's charge sound,
// muzzle effect and rumble; raises and lowers the shoulder cannon (with energy, weapon 7) and aims it
// at the lock-on (led, through GetTargetPos), the head, or a rest pose; places the hand weapons; applies
// the vision fade to all weapon items; and eases the base height to the lowest foot or hand on a clamber.
void XItemHandler_Player::UpdatePlayerWeapons()
{
    if ((m_MoveFlags & 0x9000) == 0x9000) {
        if (m_Unk2b4[0]) {
            ((EXItemAnimator *)m_pWeaponItem[0])->m_itemFlags.BitClr(1);
            ((EXItemAnimator *)m_pWeaponItem[1])->m_itemFlags.BitClr(1);
            m_Unk2b4[0] = 0;
        }
    } else if (!m_Unk2b4[0]) {
        int i;
        for (i = 0; i < 4; i++)
            if (m_CurrentWeapon[i] == (unsigned int)m_HandWeapon[0]) break;
        if (i == 4) i = 3;
        UpdateWeaponGFX(i, 0.0f);
        for (i = 0; i < 4; i++)
            if (m_CurrentWeapon[i] == (unsigned int)m_HandWeapon[1]) break;
        if (i == 4) i = 3;
        UpdateWeaponGFX(i, 0.0f);
        m_Unk2b4[0] = 1;
    }

    _WEAPON_SHAPE *pS = (_WEAPON_SHAPE *)&Weapon_Shapes;
    for (int n = 6; n; n--, pS++) {
        int id = pS->m_Id;
        s8 &bOn = *(s8 *)&pS->m_08;
        float &t = pS->m_VertexScale;
        float rate = *(float *)&pS->m_10;
        if (id == 1) {
            s8 b = m_CurrentWeapon[3] == 1;
            if ((m_MoveFlags & 0x9000) == 0x9000) b = 0;
            if (m_HandWeapon[1] != -1) b = 0;
            if ((m_MoveFlags & 0x8100) == 0x8100 || b) {
                if (FEHud::Instance(1)->IsWeaponAvailable(1)) goto on;
            }
            bOn = 0;
        } else {
            if ((m_MoveFlags & 0x9000) == 0x9000 || (m_HandWeapon[0] != id && m_HandWeapon[1] != id)) {
                if (1.0f > t) {
                    t = 1.0f;
                    bOn = 0;
                }
                continue;
            }
            bOn = m_AttackType == id;
            if (m_Mode == 0x17 && m_SubMode == 0xb && id >= 2 && id <= 4) {
on:
                bOn = 1;
            }
        }
        {
            s8 bSound = 1;
            if (theApp.m_bCutscene && m_bHidden) bSound = 0;
            if (bOn) {
                if (t >= 1.0f && pS->m_20[0] && bSound)
                    EXBaseSoundManager::GetSoundManager()->StartSound(pS->m_20[0], m_pItem->Position(), -1, 0);
                if (t > 0.0f) {
                    t -= XApp::GameFrameRateMul() / rate;
                    if (0.0f > t) t = 0.0f;
                }
            } else {
                if (!(0.0f < t) && pS->m_20[1] && bSound)
                    EXBaseSoundManager::GetSoundManager()->StartSound(pS->m_20[1], m_pItem->Position(), -1, 0);
                if (1.0f > t) {
                    t = XApp::GameFrameRateMul() / rate + t;
                    if (t > 1.0f) t = 1.0f;
                }
            }
            float f = (float)pS->m_VertexSet * t;
            int v = (int)f;
            float frac = f - (float)v;
            EXItemAnimator *pW;
            if (pS->m_Id == 1) {
                pW = (EXItemAnimator *)m_pWeaponItem[7];
                pW->m_renderInfo.m_renderFlags.m_flags |= 0x1000;
                pW = (EXItemAnimator *)m_pWeaponItem[7];
            } else {
                if ((m_MoveFlags & 0x9000) == 0x9000) continue;
                int h;
                if (m_HandWeapon[0] == pS->m_Id) h = 0;
                else if (m_HandWeapon[1] == pS->m_Id) h = 1;
                else continue;
                pW = (EXItemAnimator *)m_pWeaponItem[h];
                pW->m_renderInfo.m_renderFlags.m_flags |= 0x1000;
                pW = (EXItemAnimator *)m_pWeaponItem[h];
            }
            pW->m_renderInfo.m_vertexSet1 = (u16)v;
            pW->m_renderInfo.m_vertexSet2 = (u16)(v + 1);
            pW->m_renderInfo.m_vertexSetBlend = (u8)(frac * 256.0f);
        }
    }

    EXDatumTemp d;
    EXVector rot;
    if (m_pItem->GetAnimatorDatum(0x10000030, d, 5, 0)) {
        d.m_rotation.GetRotation(rot, 4);
        EXItemAnimator *pW = (EXItemAnimator *)m_pWeaponItem[7];
        pW->m_position = d.m_position;
        pW->m_validMtx = 0;
        pW = (EXItemAnimator *)m_pWeaponItem[7];
        pW->m_rotation = rot;
        pW->m_validMtx = 0;
    }

    if (m_PlasmaChargeTime > 0.0f) {
        if (!m_PlasmaSound) {
            m_PlasmaSound = 0x1af00540;
            m_SoundTag.AddTrackingSound(0x1af00540, m_pItem->Position(), 0.0f, 0.0f);
        } else {
            m_SoundTag.UpdatePosition(m_pItem->Position());
        }
        if (!m_pHeadItem) m_pHeadItem = XItemHandler_ScriptBase::FX_CreateLink(0x40006e8, -1, 0x10000037, -1, 0x1000016);
        float k = m_PlasmaChargeTime * 0.016666668f;
        float one = 1.0f;
        float r = k >= one ? one : k;
        XGamePad *pPad = (XGamePad *)EXBaseGamePad::GetGamePad();
        if (pPad->IsRumbleActive(0x54000006, m_PlasmaSoundIdx))
            ((XGamePad *)EXBaseGamePad::GetGamePad())->AlterRumbleMult(m_PlasmaSoundIdx, r);
        else
            m_PlasmaSoundIdx = ((XGamePad *)EXBaseGamePad::GetGamePad())->PlayRumble(0x54000006, r, m_PadIndex);
    } else {
        if (m_PlasmaSound) {
            m_SoundTag.Remove(m_PlasmaSound, 1);
            m_PlasmaSound = 0;
            m_PlasmaSoundIdx = -1;
        }
        if (m_pHeadItem) {
            m_pHeadItem->m_itemFlags.m_flags = 0x10;
            m_pHeadItem = 0;
        }
    }

    if (m_CannonTimer > 0.0f) m_CannonTimer -= XApp::GameFrameRateMul();
    {
        s8 bWant = m_CurrentWeapon[0] == 7;
        if (25.0f > m_Energy) bWant = 0;
        if (m_Mode != 0x11 && bWant) {
            if (!m_bCannonUp && m_CannonState == -1) {
                m_CannonState = 0;
                m_CannonTimer = 15.0f;
                EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00234, m_pItem->Position(), -1, 0);
            }
        } else if (m_bCannonUp && m_CannonState == -1) {
            m_CannonState = 10;
            m_CannonTimer = 15.0f;
            EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00237, m_pItem->Position(), -1, 0);
        }
    }
    if (m_CannonState == 0) {
        m_bCannonRaised = 1;
        if (!(0.0f < m_CannonTimer)) {
            m_bCannonUp = 1;
            m_CannonState = -1;
        }
    } else if (m_CannonState == 10) {
        m_bCannonUp = 0;
        if (!(0.0f < m_CannonTimer)) {
            m_bCannonRaised = 0;
            m_CannonState = -1;
        }
    }

    EXDatumTemp sh;
    if (!m_pItem->GetAnimatorDatum(0x10000035, sh, 5, 0)) memset(&sh, 0, sizeof sh);
    EXMatrix shm;
    sh.m_rotation.GetRotation(shm);
    sh.m_rotation.GetRotation(rot, 4);
    EXItemAnimator *pW = (EXItemAnimator *)m_pWeaponItem[8];
    pW->m_position = sh.m_position;
    pW->m_validMtx = 0;
    pW = (EXItemAnimator *)m_pWeaponItem[8];
    pW->m_rotation = rot;
    pW->m_validMtx = 0;
    EXQuaternion base = sh.m_rotation;
    EXVector off(-0.032f, 0.083f, -0.121f, 0.0f), o;
    D3DXVec4Transform(&o, &off, &shm);
    sh.m_position += o;
    EXQuaternion q;
    if (m_bCannonRaised) {
        EXVector a(0.0f, 0.0f, 0.0f, 0.0f);
        switch (m_Mode) {
        case 2: case 4: case 0xc: case 0x12: case 0x15:
            a.x = 0.0f;
            break;
        default:
            a.x = -0.78539819f;
            break;
        case 0x18:
            a.x = -1.5707964f;
            break;
        }
        q.EXMaths::quat_xyzw::SetRotation(a, 4);
    } else {
        EXVector a(-2.6179938f, 0.0f, 0.0f, 0.0f);
        q.EXMaths::quat_xyzw::SetRotation(a, 4);
        q = EXQuaternion(base * q);
    }
    SlerpConst(m_CannonBase, q, m_CannonBase, XApp::GameFrameRateMul() * 0.13089969f);
    EXQuaternion cb = m_CannonBase;
    cb.GetRotation(rot, 4);
    pW = (EXItemAnimator *)m_pWeaponItem[9];
    pW->m_position = sh.m_position;
    pW->m_validMtx = 0;
    pW = (EXItemAnimator *)m_pWeaponItem[9];
    pW->m_rotation = rot;
    pW->m_validMtx = 0;
    {
        EXVector up(0.0f, 0.3f, 0.0f, 0.0f), u;
        shm.set_rotate_quat(cb);
        D3DXVec4Transform(&u, &up, &shm);
        sh.m_position += u;
    }
    pW = (EXItemAnimator *)m_pWeaponItem[10];
    pW->m_position = sh.m_position;
    pW->m_validMtx = 0;
    pW = (EXItemAnimator *)m_pWeaponItem[11];
    pW->m_position = sh.m_position;
    pW->m_validMtx = 0;
    ((EXItemAnimator *)m_pWeaponItem[11])->m_itemFlags.BitClr(1);

    EXQuaternion cur = m_CannonAim;
    EXQuaternion target;
    float speed = 0.13089969f;
    if (!m_bCannonUp) {
        EXQuaternion b2 = m_CannonBase;
        EXVector a(1.5707964f, 0.0f, 0.0f, 0.0f);
        EXMaths::quat_xyzw r;
        r.SetRotation(a, 4);
        target = EXQuaternion(b2 * r);
    } else {
        int aim = 0;
        if (m_Mode == 0) {
            if (m_SubMode >= 6 && m_SubMode <= 7) {
                aim = 2;
                goto pick;
            }
        } else if (m_Mode == 0x17) {
            if (m_SubMode == 0xd) {
                aim = 2;
                goto pick;
            }
        } else if (m_Mode == 0x18) {
            aim = 1;
        }
        {
            EXVector tp;
            if (GetTargetPos(tp, 7)) {
                speed = m_TrackTime * 0.0087266462f + 0.13089969f;
                m_AimPos = tp;
                EXPlacementItem *pItem = m_pItem;
                tp -= pItem->Position();
                EXMatrix m;
                m.set_rotate_xyz(pItem->Rotation());
                m = EXMatrix(m.inverse3());
                EXVector l;
                D3DXVec4Transform(&l, &tp, &m);
                tp = l + m_pItem->Position();
                EXVector fwd(0.0f, 0.0f, 1.0f, 0.0f);
                EXVector yup(0.0f, 1.0f, 0.0f, 0.0f);
                EXVector eye = m_pItem->Position() + sh.m_position;
                EXMatrix look;
                look.LookAt(eye, tp, yup, fwd);
                target.SetRotation(look);
                pW = (EXItemAnimator *)m_pWeaponItem[11];
                pW->m_itemFlags.BitSet(1);
                pW->m_validMtx = 0;
                aim = -1;
            }
        }
pick:
        if (aim == 0) {
            target = EXQuaternion(0.0f, 0.0f, 0.0f, 1.0f);
        } else if (aim == 1) {
            EXVector a(-1.5707964f, 0.0f, 0.0f, 0.0f);
            target.EXMaths::quat_xyzw::SetRotation(a, 4);
        } else if (aim == 2) {
            EXDatumTemp hd;
            EXVector hr;
            if (m_pItem->GetAnimatorDatum(0x10000036, hd, 5, 0)) hd.m_rotation.GetRotation(hr, 4);
            else hr.SetZero();
            target.EXMaths::quat_xyzw::SetRotation(hr, 4);
            speed = 0.087266462f;
        }
    }
    EXQuaternion out;
    SlerpConst(cur, target, out, speed * XApp::GameFrameRateMul());
    float l = out.w * out.w + out.z * out.z + out.y * out.y + out.x * out.x;
    if (l >= 0.0f && (l = sqrtf(l)) > 1e-7f) {
        l = 1.0f / l;
        out.x = l * out.x;
        out.y = l * out.y;
        out.z = l * out.z;
        out.w = l * out.w;
    }
    m_CannonAim = out;
    out.GetRotation(rot, 4);
    if (!m_bCannonUp) ((EXItemAnimator *)m_pWeaponItem[11])->m_itemFlags.BitClr(1);
    pW = (EXItemAnimator *)m_pWeaponItem[10];
    pW->m_rotation = rot;
    pW->m_validMtx = 0;
    pW = (EXItemAnimator *)m_pWeaponItem[11];
    pW->m_rotation = rot;
    pW->m_validMtx = 0;

    EXDatumTemp hw;
    if (m_pItem->GetAnimatorDatum(0x10000019, hw, 5, 0)) {
        pW = (EXItemAnimator *)m_pWeaponItem[0];
        pW->m_position = hw.m_position;
        pW->m_validMtx = 0;
        hw.m_rotation.GetRotation(rot, 4);
        pW = (EXItemAnimator *)m_pWeaponItem[0];
        pW->m_rotation = rot;
        pW->m_validMtx = 0;
    }
    SetHandWeaponPosRot(1);
    EXItemAnimator_Entity **ppW = m_pWeaponItem;
    for (int i = 12; i; i--, ppW++) {
        EXItemAnimator *p = (EXItemAnimator *)*ppW;
        if (m_bVisionFade) p->m_renderInfo.m_renderFlags.m_flags |= 2;
        else p->m_renderInfo.m_renderFlags.m_flags &= ~2;
        p = (EXItemAnimator *)*ppW;
        p->m_renderInfo.m_rgbaMult.x = 1.0f;
        p->m_renderInfo.m_rgbaMult.y = 1.0f;
        p->m_renderInfo.m_rgbaMult.z = 1.0f;
        p->m_renderInfo.m_rgbaMult.w = m_VisionFade;
    }

    if (m_Mode == 4) {
        switch (m_SubMode) {
        case 0: case 1: case 3: case 4: {
            EXDatumTemp a, b;
            if (!m_pItem->GetAnimatorDatum(0x10000031, a, 1, 0)) memset(&a, 0, sizeof a);
            if (!m_pItem->GetAnimatorDatum(0x10000032, b, 1, 0)) memset(&b, 0, sizeof b);
            float toe = b.m_position.y >= a.m_position.y ? a.m_position.y : b.m_position.y;
            if (!m_pItem->GetAnimatorDatum(0x10000005, a, 1, 0)) memset(&a, 0, sizeof a);
            if (!m_pItem->GetAnimatorDatum(0x10000006, b, 1, 0)) memset(&b, 0, sizeof b);
            const float &hand = b.m_position.y >= a.m_position.y ? a.m_position.y : b.m_position.y;
            float low = hand >= toe ? toe : hand;
            if (m_BaseState != 1) {
                m_BaseState = 1;
                m_BaseLevel = low;
                m_BaseHeight = low;
                return;
            }
            float dd = low - m_BaseLevel;
            if ((float)fabs(dd) > 1.0f) {
                float lim = XApp::GameFrameRateMul() * 0.1f;
                float step = dd >= lim ? lim : (0.0f - lim >= dd ? 0.0f - lim : dd);
                m_BaseLevel += step;
                m_BaseHeight = m_BaseLevel;
                return;
            }
            m_BaseHeight = m_BaseLevel;
            return;
        }
        }
    }
    m_BaseHeight = m_pItem->Position().y;
    m_BaseState = 0;
}

void XItemHandler_Player::UseWeaponJump(int slot)
{
    switch (m_CurrentWeapon[slot]) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 8:
    case 9:
    case 10:
    case 11:
        if (!((EXItemAnimator_Anim *)m_pItem->GetAnimator(EXRUNTIME_CLASS(EXItemAnimator_Anim)))->Controller(1)) {
            SetWeapon(slot);
            SetupMoveBlend(6, 0, 0.0f);
            m_Unk224 = 1.0f;
        }
        break;
    }
}

void XItemHandler_Player::DoWallJump()
{
    if (!(m_pItem->m_pItemPhysics->PhysicsFlags() & 1)) MoveJump();
    if (AnimEnded()) {
        switch (m_SubMode) {
        case 0:
        case 1:
            SetupMove(3, 5, -1.0f);
            return;
        case 2:
            SetupMove(9, 0, -1.0f);
            return;
        }
    }
    if (m_SubMode == 2 && m_Unk358 > 0.0f) return;
    CheckForLand();
}

float XItemHandler_Player::GetPlasmaChargeEnergy()
{
    if (m_PlasmaChargeTime > 120.0f && m_Energy >= 50.0f) return 50.0f;
    if (m_PlasmaChargeTime > 60.0f && m_Energy >= 30.0f) return 30.0f;
    if (m_PlasmaChargeTime > 0.0f && m_Energy >= 25.0f) return 25.0f;
    return 0.0f;
}

void XItemHandler_Player::AdvanceCombo(int move, signed char b)
{
    int bit;
    switch (move) {
    case 0: bit = 1; break;
    case 1: bit = 2; break;
    case 2:
    case 3:
    case 4: bit = 4; break;
    default: return;
    }
    m_ComboTimer = 5.0f;
    for (int i = 0; i < 7; i++) {
        if (!(Combo_Seq[i].m_Moves[Combo_Seq[i].m_Pos] & bit))
            Combo_Seq[i].m_Pos = 0;
        else if (b)
            Combo_Seq[i].m_Pos++;
    }
}

// NEAR (see check): the player's field defaults (Initialise and level restarts): the UBB test animation's
// section bits, input, movement, weapons, jump/ledge/gecko/cannon state, health and energy from the
// upgrades, cloak and vision state, ammo, and the weapon shapes fully retracted.
void XItemHandler_Player::InitClassVars()
{
    EXGeoAnim *pAnim = EXGeoFile::GetGeoFile(0x1000005)->GeoHeader()->AnimList().Find(0x300000f)->Object(0);
    m_SectionBits = pAnim->SelectiveSectionFlags();
    m_UpdateCount = 0;
    m_PlayerIndex = 0;
    *(int *)&m_PadIndex = GameControllerRemovedSys::Controller();
    m_Unkd4 = 0;
    m_bScanMode = 0;
    m_StickAngle = 0.0f;
    m_StickMag = 0.0f;
    m_ButtonMask = 0;
    m_AnalogMask = 0;
    m_ButtonLock = 0;
    m_AnalogLock = 0;
    m_ButtonsPressed = 0;
    m_ButtonsHeld = 0;
    m_ButtonsReleased = 0;
    m_SelectedSkin = -1;
    m_Pad.SetZero();
    m_HitMoveScale = 1.0f;
    m_DeadFallTimer = 1.0f;
    m_PadAngle = 0.0f;
    m_PrevStickAngle = 0.0f;
    m_CamAngle = 0.0f;
    m_TOSCharge = 0.0f;
    m_TOSPress = 0.0f;
    m_HangYaw = 0.0f;
    m_ThrowWeapon = 0;
    m_HoldState = 0;
    m_Unk12d[0] = 0;
    m_WeaponDown = 0.0f;
    m_WeaponDownT[0] = -1.0f;
    m_WeaponDownT[1] = -1.0f;
    m_WeaponDownT[2] = -1.0f;
    m_WeaponDownT[3] = -1.0f;
    for (int c = 0; c < 7; c++) Combo_Seq[c].m_Pos = 0;
    {
        u32 *p = (u32 *)m_Swing;
        for (int n = 0x20; n; n--) *p++ = 0;
    }
    m_SwingCount[0] = 0;
    m_SwingCount[1] = 0;
    m_HitEventFlags = 0;
    m_bAttacking = 0;
    m_AttackHit = -1;
    m_AttackState = 0;
    m_ReactTimer = 0.0f;
    m_Combo = -1;
    m_ComboTimer = 0.0f;
    m_Unk1ec = 0.0f;
    m_PlasmaCharge = 0.0f;
    m_PlasmaChargeTime = 0.0f;
    m_PlasmaSoundIdx = -1;
    m_pHeadItem = 0;
    m_PlasmaSound = 0;
    m_VMSound = 0;
    m_bDamageStandOn = 0;
    s8 diff = GameData::Instance(1)->Values()->m_Unk1c[0];
    m_Mode = 0;
    m_SubMode = 0;
    m_Unk214 = 0xffffffff;
    m_MSpeed = 0.0f;
    m_Unk21c = 0.0f;
    m_ShimmyDir = 1.0f;
    m_Unk224 = 1.0f;
    m_Unk228 = 1.0f;
    m_CrouchTimer = 0.0f;
    m_Unk230 = 0.0f;
    m_Unk234 = 0;
    m_bAnimEnded = 0;
    m_AnimEndParam = 0;
    m_MoveFlags = 0;
    m_AnimSpeed = 1.0f;
    m_MoveBlend = 0.0f;
    m_Unk24c = 0.0f;
    m_ReactRecover = 0.0f;
    m_bIdleHitReact = 0;
    m_PickupWeapon = 0;
    m_PickupTimer = 0.0f;
    m_HitAngle = 0.0f;
    m_bForceModeChange = 0;
    *(int *)&m_Unk265[3] = 0;
    *(int *)&m_Unk265[7] = 0;
    m_bUnk209 = diff == 0;
    m_RootMotionStart.SetZero();
    m_RootPosStart.SetZero();
    m_HandWeapon[0] = 0;
    m_HandWeapon[1] = 0;
    m_CurrentWeapon[0] = 0xd;
    m_CurrentWeapon[1] = 0xd;
    m_CurrentWeapon[2] = 0xd;
    m_CurrentWeapon[3] = 0xd;
    m_AttackType = 0;
    m_CurrentSlot = -1;
    m_PrevWeapon = -1;
    m_Unk2b4[0] = 1;
    m_Unk2b8.SetZero();
    m_LastPos.SetZero();
    m_Delta.SetZero();
    m_Speed.SetZero();
    m_MSpeedTarget = 0.0f;
    m_IdleTime = 0.0f;
    m_ClamberSide = 1;
    m_Unk308 = 0.0f;
    m_bPlayerHasControl = 1;
    m_PredMsgFlags = 0xffffffff;
    m_bTargetAdjust = 0;
    m_TargetAdjustType = 0.0f;
    m_TargetAdjustTime = 0.0f;
    m_TargetAngle = 0.0f;
    m_TargetAdjust.SetZero();
    m_TargetDelta.SetZero();
    m_Unk344 = 0.0f;
    m_JumpBaseY = 0.0f;
    m_Unk34c = 0.0f;
    m_Unk350 = 0.0f;
    m_FallTimer = 0.0f;
    m_Unk358 = 0.0f;
    m_Unk35c = 0.0f;
    m_Unk360 = 0.0f;
    m_bUnk364 = 1;
    m_bTurnToTarget = 0;
    m_TurnTime = 0.0f;
    m_TurnRate = 0.0f;
    m_bUnk370 = 0;
    m_LockHandIdx = 0;
    m_HandLockPos.SetZero();
    m_HandLock = -1.0f;
    m_bHandLockFlag = 0;
    m_Unk390 = 0;
    m_SkidTurn = 0.0f;
    for (int ph = 0; ph < 4; ph++) {
        m_PadHistory[ph].m_Angle = 0.0f;
        m_PadHistory[ph].m_Update = 0;
    }
    m_bJumpTarget = 0;
    m_bJumpTargetNear = 0;
    m_pJustOffCar = 0;
    m_JumpTarget.SetZero();
    m_bLedgeTarget = 0;
    m_LedgeTarget.SetZero();
    m_LedgeAngle = 0.0f;
    m_JumpTargetType = 0;
    m_pTrackTarget = 0;
    m_AimPos.SetZero();
    m_TrackTime = 0.0f;
    m_TrackLastPos.SetZero();
    m_GeckoAngle = EXQuaternion(0.0f, 0.0f, 0.0f, 1.0f);
    m_GeckoPoint.SetZero();
    m_GeckoNormal.SetZero();
    *(int *)&m_GeckoPlane.x = 0;
    *(int *)&m_GeckoPlane.y = 0;
    *(int *)&m_GeckoPlane.z = 0;
    *(int *)&m_GeckoPlane.w = 0;
    m_GeckoStartAngle = EXQuaternion(0.0f, 0.0f, 0.0f, 1.0f);
    m_GeckoValidAngle = EXQuaternion(0.0f, 0.0f, 0.0f, 1.0f);
    m_GeckoStartPos.SetZero();
    m_GeckoTargetPos.SetZero();
    m_GeckoRoll = 0.0f;
    m_GeckoBlend = 0.0f;
    *(int *)m_Unk49c = 0;
    m_bCannonUp = 0;
    m_bCannonRaised = 0;
    m_CannonState = -1;
    m_CannonAim = EXQuaternion(0.0f, 0.0f, 0.0f, 1.0f);
    m_CannonBase = EXQuaternion(0.0f, 0.0f, 0.0f, 1.0f);
    m_CannonTimer = 0.0f;
    m_bDiskInHand = 1;
    m_nWeaponLoads = 0;
    m_bWeaponsLoaded = 0;
    m_SpecialActorID = 0;
    m_pSpecialItem = 0;
    m_GroundAttackID = 0;
    m_SpecialMoveAvailable = 0;
    m_ClamberPos.SetZero();
    m_StateFlags = 0;
    m_GrabStep.SetZero();
    m_GrabBase.SetZero();
    m_bUnkc40 = 0;
    m_Cards[0] = 0;
    m_Cards[1] = 1;
    m_Cards[2] = 2;
    m_Cards[3] = 3;
    m_CardIdx = 4;
    *(int *)m_Unkc58 = -1;
    m_Health = GetMaxHealth();
    m_Energy = GetMaxEnergy();
    m_SavedEnergy = 0.0f;
    m_bCloak = 0;
    m_CloakState = 0;
    m_Tint = 0;
    m_Unkc71[0] = 0xff;
    m_SparkTimer = 0.0f;
    m_EMPTimer = 0.0f;
    m_CloakBreakTimer = 0.0f;
    m_bCloakBroken = 0;
    m_CloakRefraction = 0.0f;
    m_RefractionBoost = 0.0f;
    m_CloakBreak = 0;
    m_VisionModeFlags = 0xffff;
    m_bSparkSkin = 0;
    m_bHelmet = 0;
    for (int u = 0; u < 14; u++) m_WeaponUse[u] = 0;
    SetDefaultAmmo();
    m_BaseHeight = 0.0f;
    m_BaseLevel = 0.0f;
    m_BaseState = 0;
    m_bForceTrackCam = 0;
    m_Unkd34 = -1.0f;
    m_Unkd38 = 0;
    m_Unkd3c.SetZero();
    m_ShimmyMid.SetZero();
    m_bShimmyMid = 0;
    m_AmbientColour = EXVector(1.0f, 1.0f, 1.0f, 1.0f);
    m_FloorType = 3;
    m_FloorNormal = EXVector(0.0f, 1.0f, 0.0f, 0.0f);
    m_Wet = 0;
    m_bVisionFade = 0;
    m_VisionFade = 1.0f;
    m_bHidden = 0;
    _WEAPON_SHAPE *pS = (_WEAPON_SHAPE *)&Weapon_Shapes;
    for (int k = 0; k < 6; k++, pS++) {
        pS->m_VertexScale = 1.0f;
        *(s8 *)&pS->m_08 = 0;
    }
}

void XItemHandler_Player::Hide(signed char bHide, signed char bBreakout)
{
    if (bHide) {
        m_bHidden = 1;
        m_pItem->m_itemFlags.BitClr(1);
        Actor::DeleteHeldHead();
        if (theApp.m_bCutscene) {
            m_pStandOn = 0;
            if (bBreakout) SafeBreakoutAll(2);
        }
    } else {
        m_bHidden = 0;
        m_pItem->SetHasRender();
    }
    if (m_Mode == 0x16 || m_HoldState == 1) {
        if (m_SpecialActorID) {
            Actor *pActor = Actor::GetPtrFromID(m_SpecialActorID);
            if (pActor) pActor->SetHide(bHide);
        }
    }
}

void XItemHandler_Player::SendPredMsg(int msg, float f)
{
    switch (msg) {
    case 0: InflictDamage(f, 0); break;
    case 1: InflictDamage(f, 1); break;
    case 2: InflictDamage(f, 4); break;
    case 6: m_Energy = PlayerMax(0.0f, m_Energy - f); break;
    }
}

void InertiaToStrafeSpeed(EXVector &in, EXVector &out) { out = in * 0.046875f; }

void XItemHandler_Player::RagDollHeldDrone(int flags)
{
    Actor *pActor = Actor::GetPtrFromID(m_SpecialActorID);
    if (pActor && pActor->IsDrone()) {
        if (pActor->GetItem()) pActor->SetHide(0);
        if (!(flags & 2) && !theApp.m_bCutscene) {
            m_SpecialActorID = 0;
            pActor->EnableRagDoll();
            pActor->SetHealth(0.0f, 1);
            pActor->m_SM.ExtGosub(0, STATE_RagDollDeath, 0, 0, 0, 0);
        } else {
            Objective_NotifyActorJustKilled(pActor);
            Objective_NotifyActorDeath(pActor);
            pActor->Delete();
        }
        m_SpecialActorID = 0;
        m_HoldState = 0;
        m_bUnkc40 = 0;
    }
}

signed char XItemHandler_Player::CheckForStandingSpace(EXVector &pos, float h)
{
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
    EXVector out;
    EXCapsule c;
    c.Origin() = pos;
    c.Origin().y += 0.66f;
    c.Direction() = EXVector(0.0f, h - 0.45f - 0.2f, 0.0f, 0.0f);
    c.Radius() = 0.45f;
    return !pPhys->CollideCapsule(c, out);
}

// NEAR (see check): the ledge angle under the hands: grab points 1.0 around each hand (along the facing
// angle a; on a hang the hands' own offsets, else one unit either side) must both be ledges (1) or
// corners (3). Records their midpoint and returns the facing onto the ledge, optionally applying it.
signed char XItemHandler_Player::HandleShimmyAngle(EXVector &pos, float a, signed char bSet, float &angle)
{
    float s = sinf(a);
    float c = cosf(a);
    float l = -1.0f;
    float r = 1.0f;
    if (m_Mode == 0xc) {
        EXDatumTemp d;
        if (m_pItem->GetAnimatorDatum(0x10000005, d, 5, 0)) l = d.m_position.x;
        else l = -0.25f;
        if (m_pItem->GetAnimatorDatum(0x10000006, d, 5, 0)) r = d.m_position.x;
        else r = 0.25f;
    }
    EXVector pl = pos + EXVector(s, 0.0f, c, 0.0f) * l;
    EXVector pr = pos + EXVector(s, 0.0f, c, 0.0f) * r;
    EXGrabPointTemp gl;
    gl.Set(pl, 1.0f, (EXGrabPointTemp::GRABFITYPE)1);
    theItemEnv.DoGrabPointToMap(gl);
    if (gl.m_24 != 1 && gl.m_24 != 3) return 0;
    EXGrabPointTemp gr;
    gr.Set(pr, 1.0f, (EXGrabPointTemp::GRABFITYPE)1);
    theItemEnv.DoGrabPointToMap(gr);
    if (gr.m_24 != 1 && gr.m_24 != 3) return 0;
    m_ShimmyMid = (gr.m_30 + gl.m_30) * 0.5f;
    m_bShimmyMid = 1;
    float t = (float)atan2(gl.m_30.x - gr.m_30.x, gl.m_30.z - gr.m_30.z) + 1.5707964f;
    angle = t;
    if (bSet) {
        EXPlacementItem *pItem = m_pItem;
        EXVector rot(pItem->Rotation().x, t, pItem->Rotation().z, pItem->Rotation().w);
        pItem->m_rotation = rot;
        pItem->m_validMtx = 0;
    }
    return 1;
}

signed char XItemHandler_Player::GetGrabPoint(int)
{
    EXVector v;
    GetGrabVector(0, v);
    EXGrabPointTemp t;
    t.Set(v, 0.5f, (EXGrabPointTemp::GRABFITYPE)1);
    theItemEnv.DoGrabPointToMap(t);
    if (t.m_24 == 1) {
        m_ClamberPos = t.m_30;
        m_StateFlags = (u16)t.m_28;
        return 1;
    }
    return 0;
}

// NEAR (see check): a scenery takedown spot for a held drone: a wall to slam into (1), or a takedown edge
// (grab flag 0x80) within 1.5 at foot level (3); lines the player up beside the edge (1.75 out, 1.65 for
// flag 0x800) facing along it.
int XItemHandler_Player::CheckForTakeDownScenery()
{
    if (CheckForWallSlam()) return 1;
    EXGrabPointTemp g;
    g.Set(m_pItem->Position(), 1.5f, (EXGrabPointTemp::GRABFITYPE)1);
    theItemEnv.DoGrabPointToMap(g);
    if (g.m_24 != 1 || !(g.m_28 & 0x80) || (float)fabs(g.m_30.y - m_pItem->Position().y) > 0.1f) return 0;
    float a = (float)atan2(g.m_Normal.x, g.m_Normal.z);
    m_StateFlags = g.m_28;
    float s = (float)sin(a + 1.5707964f);
    float c = (float)cos(a + 1.5707964f);
    if (!(g.m_28 & 0x800)) {
        m_TargetAdjust.x = s * 1.75f + g.m_30.x;
        m_TargetAdjust.y = g.m_30.y + 0.0f;
        m_TargetAdjust.z = c * 1.75f + g.m_30.z;
    } else {
        m_TargetAdjust.x = g.m_30.x + s * 1.65f;
        m_TargetAdjust.y = g.m_30.y + 0.0f;
        m_TargetAdjust.z = g.m_30.z + c * 1.65f;
    }
    m_TargetAdjust.w = g.m_30.w + 0.0f;
    m_TargetAdjustTime = a - 1.5707964f;
    return 3;
}

EXVector XItemHandler_Player::GetChestPos()
{
    EXDatumTemp d;
    if (m_pItem->GetAnimatorDatum(0xe000005, d, 0, 0)) return d.Position();
    return m_pItem->Position();
}

EXVector XItemHandler_Player::GetPelvisPos()
{
    EXDatumTemp d;
    if (m_pItem->GetAnimatorDatum(0xe000001, d, 0, 0)) return d.Position();
    return m_pItem->Position();
}

EXVector XItemHandler_Player::GetTopHeadPos()
{
    EXDatumTemp d;
    if (m_pItem->GetAnimatorDatum(0x1000001c, d, 0, 0)) return d.Position();
    return m_pItem->Position();
}

// NEAR: the inlined GetMaxHealth/GetMaxEnergy constants 166 and 200 land in swapped xmm registers.
void XItemHandler_Player::Resurrect()
{
    m_DeadFallTimer = 1.0f;
    m_Health = GetMaxHealth();
    m_Energy = GetMaxEnergy();
    SetDefaultAmmo();
}

// Player vs actor collision: pushes the predator out of nearby actors (Ulysses gets an elongated radius
// along his facing) and, when moving fast enough, knocks them with SendMsg_AttackedByPredator (strength
// 0..4 from the run speed). Always returns 0.
// NEAR: frame 0x20 larger (stack-slot packing), x87 instead of SSE for the max-radius sum, and retail
// reloads the actor's rotation for sin and cos separately.
// In the air. With a jump target (a platform, CalculateTargetJump) or a ledge, the jump
// homes in on it: the player turns toward the landing heading (up to 5 degrees a frame)
// and the horizontal move is the remaining distance, capped (0.25 platform, 0.094 ledge).
// Otherwise the stick adds a small push while airborne (nothing pushing up), horizontal
// speed is capped at ~0.1875, the player turns toward the direction of travel, and pitch
// and roll level out. Horizontal motion is carried in m_Unk2b8/m_Speed, not DeltaMove.
void XItemHandler_Player::MoveJump()
{
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
    float yaw = m_pItem->Rotation().y;
    float limit;
    EXVector target;

    if (m_bJumpTarget) {
        limit = 0.25f;
        target = m_JumpTarget;
        int type;
        EXItem *pPlat = GetPlatformItem(type);
        if (pPlat && type > 0 && type <= 2) {
            EXDatumTemp d;
            if (!pPlat->GetAnimatorDatum(0x10000060, d, 1, 0)) memset(&d, 0, sizeof d);
            EXVector r;
            d.m_rotation.GetRotation(r, 4);
            yaw = type == 1 ? r.y + 1.5707964f : r.y;
        }
    } else if (m_bLedgeTarget) {
        limit = 0.09375f;
        target = m_LedgeTarget;
        yaw = m_LedgeAngle;
    } else {
        // free jump
        float mul;
        EXVector push;
        m_Unk2b8 = pPhys->m_DeltaMove;
        m_Unk2b8.w = 0.0f;
        m_Unk2b8.y = 0.0f;
        push.x = 0.0f;
        push.z = 0.0f;
        if (!(m_MoveFlags & 1) && 0.001f > pPhys->m_CollisionResponse.y) {
            float h = sqrtf(m_Pad.x * m_Pad.x + m_Pad.z * m_Pad.z) * 0.01499968f;
            push.x = h * (float)sin(m_PadAngle);
            push.z = (float)cos(m_PadAngle) * h;
        }
        mul = XApp::GameFrameRateMul();
        m_Unk2b8.x += push.x * mul;
        m_Unk2b8.y += mul * 0.0f;
        m_Unk2b8.z += push.z * mul;
        m_Unk2b8.w += mul * 0.0f;
        if (sqrtf(m_Unk2b8.x * m_Unk2b8.x + m_Unk2b8.z * m_Unk2b8.z) > 0.187496f) {
            float l = sqrtf(m_Unk2b8.x * m_Unk2b8.x + m_Unk2b8.y * m_Unk2b8.y + m_Unk2b8.z * m_Unk2b8.z + m_Unk2b8.w * m_Unk2b8.w);
            if (l > 1e-5f) {
                float k = 1.0f / l;
                m_Unk2b8.x *= k; m_Unk2b8.y *= k; m_Unk2b8.z *= k; m_Unk2b8.w *= k;
            }
            m_Unk2b8.x *= 0.187496f; m_Unk2b8.y *= 0.187496f; m_Unk2b8.z *= 0.187496f; m_Unk2b8.w *= 0.187496f;
        }
        if (sqrtf(m_Unk2b8.x * m_Unk2b8.x + m_Unk2b8.z * m_Unk2b8.z) > 0.1f && !(m_MoveFlags & 2)) {
            float rate = 0.1f;
            if (m_Mode == 0xa && m_SubMode == 2) rate = m_bUnk370 ? 0.09f : 0.0f;
            rate *= XApp::GameFrameRateMul();
            float a = (float)atan2(m_Unk2b8.x, m_Unk2b8.z);
            if (m_MoveFlags & 8) a += 3.1415927f;
            EXPlacementItem *pItem = m_pItem;
            float diff = Vec_AngleDifference(pItem->Rotation().y, a);
            if ((float)fabs(diff) < 6.2831855f) {
                float step = diff > rate ? rate : (diff < -rate ? -rate : diff);
                EXVector rot = pItem->Rotation();
                rot.y += step;
                pItem->m_rotation = rot;
                pItem->m_validMtx = 0;
            }
        }
        if (!(m_HandLock > 0.0f)) {
            EXPlacementItem *pItem = m_pItem;
            EXVector rot = pItem->Rotation();
            float r = XApp::GameFrameRateMul() * 0.18f;
            float dx = Vec_AngleDifference(rot.x, 0.0f);
            float sx = dx >= r ? r : (-r >= dx ? -r : dx);
            float dz = Vec_AngleDifference(rot.z, 0.0f);
            float sz = dz >= r ? r : (-r >= dz ? -r : dz);
            rot.x += sx;
            rot.z += sz;
            pItem->m_rotation = rot;
            pItem->m_validMtx = 0;
        }
        pPhys->m_DeltaMove.x = 0.0f;
        pPhys->m_DeltaMove.z = 0.0f;
        pPhys->m_DeltaMove.w = 0.0f;
        m_Unk2b8.w = 0.0f;
        m_Unk2b8.y = 0.0f;
        return;
    }

    // homing onto the target: turn
    {
        EXPlacementItem *pItem = m_pItem;
        EXVector rot = pItem->Rotation();
        float diff = Vec_AngleDifference(rot.y, yaw);
        float r = XApp::GameFrameRateMul();
        float step;
        if (diff > 0.0f) { r *= 0.08726646f; step = r < diff ? r : diff; }
        else { r *= -0.08726646f; step = diff >= r ? diff : r; }
        rot.y += step;
        pItem->m_rotation = rot;
        pItem->m_validMtx = 0;
    }
    // ... and move: the remaining horizontal distance this frame, capped
    {
        const EXVector &pos = m_pItem->Position();
        float div = XApp::GameFrameRateDiv();
        EXVector d;
        d.x = (target.x - pos.x) * div;
        d.y = div * 0.0f;
        d.z = (target.z - pos.z) * div;
        d.w = d.y;
        if (sqrtf(d.x * d.x + d.z * d.z) > limit) {
            float l = sqrtf(d.x * d.x + d.y * d.y + d.z * d.z + d.w * d.w);
            if (l > 1e-5f) {
                float k = 1.0f / l;
                d.x *= k; d.y *= k; d.z *= k; d.w *= k;
            }
            d.x *= limit; d.y *= limit; d.z *= limit; d.w *= limit;
        }
        m_Speed = d;
        m_Unk2b8 = d;
        m_Unk2b8.w = 0.0f;
        m_Unk2b8.y = 0.0f;
    }
    pPhys->m_DeltaMove.x = 0.0f;
    pPhys->m_DeltaMove.z = 0.0f;
    pPhys->m_DeltaMove.w = 0.0f;
}

// NEAR (see check): a ledge level with the feet (within 0.3) in reach (3.0) whose edge has room and no
// special flags: a small hop (3.217) onto it, homing on a point half a unit back from the edge.
signed char XItemHandler_Player::CheckForFlipGrab()
{
    EXGrabPointTemp g;
    g.Set(m_pItem->Position(), 3.0f, (EXGrabPointTemp::GRABFITYPE)1);
    theItemEnv.DoGrabPointToMap(g);
    if (g.m_24 != 1) return 0;
    if ((float)fabs(g.m_30.y - m_pItem->Position().y) > 0.3f) return 0;
    u32 fl = *(u32 *)&g.m_28;
    if (fl & 0x20) return 0;
    if (fl & 8) return 0;
    if (fl & 0x100) return 0;
    if (fl & 0x400) return 0;
    float a;
    if (!CheckLineSegSpace(g, a)) return 0;
    if (!HandleShimmyAngle(g.m_30, a, 0, m_LedgeAngle)) return 0;
    ((XItemPhysics *)m_pItem->m_pItemPhysics)->SetThrustToAttainHeight(3.217f);
    SetupMove(3, 0xb, -1.0f);
    m_Unk358 = 0.33333334f;
    DeleteAllSwoosh();
    EXItem *pItem = m_pItem;
    m_bLedgeTarget = 1;
    float dx = pItem->Position().x - g.m_30.x;
    float dy = pItem->Position().y - g.m_30.y;
    float dz = pItem->Position().z - g.m_30.z;
    float dw = pItem->Position().w - g.m_30.w;
    float l = dy * dy + dz * dz + dw * dw + dx * dx;
    if (l >= 0.0f) {
        l = sqrtf(l);
        if (l > 1e-5f) {
            l = 1.0f / l;
            dx *= l;
            dy *= l;
            dz *= l;
            dw *= l;
        }
    }
    m_LedgeTarget.x = g.m_30.x - dx * 0.5f;
    m_LedgeTarget.y = g.m_30.y - dy * 0.5f;
    m_LedgeTarget.z = g.m_30.z - dz * 0.5f;
    m_LedgeTarget.w = g.m_30.w - dw * 0.5f;
    return 1;
}

// NEAR (logic from the disassembly; the Ghidra draft mislabels the grab flags): whether the jump catches
// something. Rising (or mode 1): a pole above the hands (result 1). Falling (or mode 2): a perch plane or a
// perch (5), else a ledge in reach whose wall faces the player within 3pi/8 (7pi/16 for flag 8):
// 2 (flag 8), 3, or 4 (flag 0x100, the ledge's back side). On a catch: fall damage and rumble for a hard
// catch, vertical speed zeroed, m_ClamberPos = the hit, the attack stack cleared, the grab sound.
int XItemHandler_Player::CheckGrabPoint(int mode)
{
    int result = -1;
    if (m_HoldState) return -1;
    if (!(0.0f < m_Health)) return -1;
    if (m_Unk358 > 0.0f && mode == 0) return -1;
    EXItem *pItem = m_pItem;
    EXVector hand;
    hand.w = pItem->Position().w;
    hand.z = pItem->Position().z;
    hand.x = pItem->Position().x;
    float mul = XApp::GameFrameRateMul();
    hand.y = pItem->Position().y + 1.25f;
    XItemPhysics *pPhys = (XItemPhysics *)pItem->m_pItemPhysics;
    EXGrabPointTemp g;
    if (pPhys->m_DeltaMove.y > 0.0f || mode == 1) {
        g.Set(hand, 1.5f, (EXGrabPointTemp::GRABFITYPE)2);
        theItemEnv.DoGrabPointToMap(g);
        if (g.m_24 != 2 || (*(u32 *)&g.m_28 & 0x100)) return -1;
        result = 1;
    }
    if (pPhys->m_DeltaMove.y < 0.0f || mode == 2) {
        float r = mul * 2.5f;
        g.Set(m_pItem->Position(), r, (EXGrabPointTemp::GRABFITYPE)2);
        theItemEnv.DoGrabPointToMap(g);
        if (CheckPerchPlaneLand(g)) {
            result = 5;
        } else {
            g.Set(hand, r, (EXGrabPointTemp::GRABFITYPE)1);
            theItemEnv.DoGrabPointToMap(g);
            u32 fl = *(u32 *)&g.m_28;
            if (g.m_24 != 1) return result;
            if (fl & 0x20) return result;
            if (CheckPerchLand(g)) {
                result = 5;
            } else {
                if (mul * 0.5f < (float)fabs(g.m_30.y - hand.y)) return result;
                pItem = m_pItem;
                float dx = g.m_30.x - pItem->Position().x;
                float dz = g.m_30.z - pItem->Position().z;
                float d2 = dz * dz + dx * dx;
                if (d2 >= 0.0f && sqrtf(d2) > 1.5f) return result;
                EXVector rot = pItem->Rotation();
                float a = (float)atan2(g.m_Normal.x, g.m_Normal.z);
                EXVector off;
                off.x = pItem->Position().x - g.m_EdgePos.x;
                off.y = pItem->Position().y - g.m_EdgePos.y;
                off.z = pItem->Position().z - g.m_EdgePos.z;
                off.w = pItem->Position().w - g.m_EdgePos.w;
                float flipped = 0.0f;
                if (Vec_AngleDifference((float)atan2(off.x, off.z), a) < 0.0f) {
                    if (!(fl & 0x100)) return result;
                    flipped = 1.0f;
                    a += 3.1415927f;
                }
                float diff = Vec_AngleDifference(rot.y, a);
                if (fl & 8) {
                    if ((float)fabs(diff + 1.5707964f) > 1.3744468f) return result;
                } else if ((float)fabs(diff + 1.5707964f) > 1.1780972f)
                    return result;
                if (!CheckLineSegSpace(g, a)) return result;
                if (!HandleShimmyAngle(g.m_30, a, 0, a)) return result;
                float yaw = a;
                if (flipped == 0.0f) yaw = a + 3.1415927f;
                pItem = m_pItem;
                pItem->m_rotation.y = yaw;
                pItem->m_rotation.z = rot.z;
                pItem->m_rotation.x = rot.x;
                pItem->m_rotation.w = rot.w;
                pItem->m_validMtx = 0;
                if (fl & 8)
                    result = 2;
                else
                    result = ((fl & 0x100) != 0) + 3;
            }
        }
    } else if (result == -1)
        return -1;
    float vy = pPhys->m_DeltaMove.y;
    pPhys->SetPhysicsFlagsOn(3);
    if (vy < -0.5f) {
        InflictDamageFromFall(vy);
        float s = (float)fabs(vy);
        float k;
        if (s < 1.0f) {
            k = 0.5f;
            if (0.5f < s) k = s;
        } else
            k = 1.0f;
        ((XGamePad *)EXBaseGamePad::GetGamePad())->PlayRumble(0x54000007, k, m_PadIndex);
    }
    pPhys->m_DeltaMove.y = 0.0f;
    m_StateFlags = *(u32 *)&g.m_28 & 0xffff;
    m_ClamberPos.x = g.m_30.x;
    m_ClamberPos.y = g.m_30.y;
    m_ClamberPos.z = g.m_30.z;
    m_ClamberPos.w = g.m_30.w;
    m_bLedgeTarget = 0;
    ResetAttack();
    m_pAnim->DeleteStack(1);
    EXBaseSoundManager::GetSoundManager()->StartSound(0x1af0020e, m_pItem->Position(), -1, 0);
    return result;
}

// The jump state (mode 3), every frame in the air.
// - Dead: the dead-fall timer runs down, then the mission is failed (dead).
// - Styles 7-10 (attack jumps): turn toward the target actor at up to 0.16 rad a frame.
// - MoveJump does the air movement (homing onto a jump target, or stick steering).
// - Reaching a jump target: pounce on the actor (type 2) or land on the vehicle (type 1).
// - Otherwise look for something to catch: rising, a ledge in reach (1.5) above; falling, a perch
//   plane, a perch, or a ledge whose facing, clearance and shimmy angle work. A catch ends the jump:
//   hard landings hurt and rumble, then the matching move starts (shimmy, hang, clamber, perch).
// - Nothing caught: wall moves, a second jump press 4+ frames in gives an extra upward push
//   (m_Unk344 window of 12 frames), and the landing check.
void XItemHandler_Player::DoJump()
{
    if (0.0f >= m_Health) {
        m_DeadFallTimer -= XApp::FrameLength();
        if (0.0f > m_DeadFallTimer) {
            theApp.m_MissionState = 4;
            theApp.m_MissionTimer = 0.0f;
            return;
        }
    }
    if (m_Mode == 3 && m_bAnimEnded) {
        m_bAnimEnded = 0;
        if (m_SubMode >= 7 && m_SubMode <= 10) SetupMove(3, 10, -1.0f);
    }
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
    if (m_Mode == 3) {
        if (m_SubMode == 0xc && 0.17f > pPhys->m_DeltaMove.y) SetupMove(3, 0xd, -1.0f);
        if (m_SubMode >= 7 && m_SubMode <= 10) {
            Actor *pTarget = GetTargetActor();
            EXPlacementItem *pItem = m_pItem;
            float x, z;
            if (pTarget) {
                EXVector d;
                const EXVector &tp = ((EXPlacementItem *)pTarget->GetItem())->Position();
                const EXVector &pp = pItem->Position();
                d.x = tp.x - pp.x; d.y = tp.y - pp.y; d.z = tp.z - pp.z; d.w = tp.w - pp.w;
                x = d.x; z = d.z;
            } else {
                float yaw = pItem->Rotation().y;
                x = (float)sin(yaw) * 10.0f;
                z = (float)cos(yaw) * 10.0f;
            }
            float diff = Vec_AngleDifference(pItem->Rotation().y, (float)atan2(x, z));
            EXVector rot = pItem->Rotation();
            float step = diff >= 0.16f ? 0.16f : (-0.16f >= diff ? -0.16f : diff);
            rot.y += step;
            pItem->m_rotation = rot;
            pItem->m_validMtx = 0;
        }
    }
    MoveJump();

    if (m_bJumpTarget) {
        const EXVector &pp = m_pItem->Position();
        float dx = pp.x - m_JumpTarget.x, dy = pp.y - m_JumpTarget.y, dz = pp.z - m_JumpTarget.z;
        if (0.5f > sqrtf(dx * dx + dy * dy + dz * dz)) {
            if (m_JumpTargetType == 2) {
                Actor *pActor = GetTargetActor();
                if (pActor) {
                    DeadStop(1);
                    pPhys->SetPhysicsFlagsOn(3);
                    m_SpecialActorID = pActor->m_RefID;
                    if (AS_GetPtrFromIndex(pActor->m_SkinIndex)->m_Hash == 0xd000068 ||
                        AS_GetPtrFromIndex(pActor->m_SkinIndex)->m_Hash == 0xd000179) {
                        SetupMove(0x14, 2, -1.0f);
                        EXVector a(0.0f, 0.0f, 0.0f, 0.0f), b(0.0f, 0.0f, 0.0f, 0.0f);
                        SendMsg_AttackedByPredator(pActor, 0x30, a, b, 0.0f, 0);
                    } else {
                        SetupMove(0x14, 0, -1.0f);
                    }
                    EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00353, m_pItem->Position(), -1, 0);
                    return;
                }
            } else if (m_JumpTargetType == 1) {
                DeadStop(1);
                SetupMove(0x15, 0, -1.0f);
                LandedOnVehicle();
                EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00353, m_pItem->Position(), -1, 0);
                return;
            }
        }
    }

    int result = -1;
    EXGrabPointTemp g;
    if (!m_HoldState && !(0.0f >= m_Health) && !(m_Unk358 > 0.0f)) {
        EXPlacementItem *pItem = m_pItem;
        pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
        float mul = XApp::GameFrameRateMul();
        EXVector probe = pItem->Position();
        probe.y += 1.25f;
        int ok = 1;
        if (pPhys->m_DeltaMove.y > 0.0f) {
            // rising: a ledge in reach
            g.Set(probe, 1.5f, (EXGrabPointTemp::GRABFITYPE)2);
            theItemEnv.DoGrabPointToMap(g);
            if (g.m_24 != 2 || (g.m_28 & 0x100)) ok = 0;
            else result = 1;
        }
        if (ok && 0.0f > pPhys->m_DeltaMove.y) {
            // falling: perch plane, perch, then a ledge
            float r = mul * 2.5f;
            result = -1;
            g.Set(m_pItem->Position(), r, (EXGrabPointTemp::GRABFITYPE)2);
            theItemEnv.DoGrabPointToMap(g);
            if (CheckPerchPlaneLand(g)) {
                result = 5;
            } else {
                g.Set(probe, r, (EXGrabPointTemp::GRABFITYPE)1);
                theItemEnv.DoGrabPointToMap(g);
                unsigned int flags = g.m_28;
                if (g.m_24 == 1 && !(flags & 0x20)) {
                    if (CheckPerchLand(g)) {
                        result = 5;
                    } else if (!((float)fabs(g.m_30.y - probe.y) > mul * 0.5f)) {
                        EXPlacementItem *pI = m_pItem;
                        float hx = g.m_30.x - pI->Position().x, hz = g.m_30.z - pI->Position().z;
                        if (!(sqrtf(hx * hx + hz * hz) > 1.5f)) {
                            float a = (float)atan2(g.m_Normal.x, g.m_Normal.z);
                            EXVector rot = pI->Rotation();
                            float ex = pI->Position().x - g.m_EdgePos.x, ez = pI->Position().z - g.m_EdgePos.z;
                            float flip = 0.0f;
                            int fail = 0;
                            if (Vec_AngleDifference((float)atan2(ex, ez), a) < 0.0f) {
                                if (!(flags & 0x100)) fail = 1;
                                else { flip = 1.0f; a += 3.1415927f; }
                            }
                            if (!fail) {
                                float e = (float)fabs(Vec_AngleDifference(rot.y, a) + 1.5707964f);
                                if (flags & 8) { if (e > 1.3744468f) fail = 1; }
                                else if (e > 1.1780972f) fail = 1;
                            }
                            if (!fail && CheckLineSegSpace(g, a) && HandleShimmyAngle(g.m_30, a, 0, a)) {
                                float yaw = a;
                                if (flip != 0.0f) yaw += 3.1415927f;
                                pI->m_rotation = EXVector(rot.x, yaw, rot.z, rot.w);
                                pI->m_validMtx = 0;
                                result = (flags & 8) ? 2 : ((flags & 0x100) ? 4 : 3);
                            }
                        }
                    }
                }
            }
        }
        if (result != -1) {
            // caught something: the landing
            float dy = pPhys->m_DeltaMove.y;
            pPhys->m_physicsFlags |= 3;
            if (-0.5f > dy) {
                InflictDamageFromFall(dy);
                float f = (float)fabs(dy);
                float k = f >= 1.0f ? 1.0f : (0.5f >= f ? 0.5f : f);
                ((XGamePad *)EXBaseGamePad::GetGamePad())->PlayRumble(0x54000007, k, m_PadIndex);
            }
            pPhys->m_DeltaMove.y = 0.0f;
            m_StateFlags = g.m_28;
            m_ClamberPos = g.m_30;
            m_bLedgeTarget = 0;
            ResetAttack();
            m_pAnim->DeleteStack(1);
            EXBaseSoundManager::GetSoundManager()->StartSound(0x1af0020e, m_pItem->Position(), -1, 0);
        }
    }

    switch (result) {
    case 1: {
        m_ShimmyDir = GetSpeed() + 1.0f;
        if (1.0f > m_ShimmyDir) m_ShimmyDir = 1.0f;
        if (m_ShimmyDir > 2.0f) m_ShimmyDir = 2.0f;
        DeadStop(0);
        SetupMove(0x11, 0, -1.0f);
        return;
    }
    case 2:
        DeadStop(0);
        m_ClamberSide = 1;
        m_LockHandIdx = 2;
        SetupMove(0xc, 8, -1.0f);
        return;
    case 3:
        if (m_StateFlags & 0x40) {
            DeadStop(0);
            SetupMove(0xc, 5, -1.0f);
            return;
        }
        DeadStop(0);
        if (m_Pad.z > 0.85f && !(m_StateFlags & 0x10) && !(m_StateFlags & 0x200) && CheckForClamberSpace()) {
            SetupMove(4, (GetCardRandom() & 1) ? 4 : 3, -1.0f);
            return;
        }
        SetupMove(0xc, 0, -1.0f);
        return;
    case 4:
        DeadStop(0);
        SetupMove(0xc, (m_StateFlags & 0x40) ? 5 : 0, -1.0f);
        return;
    case 5:
        DeadStop(0);
        m_ShimmyDir = 1.5f;
        SetupMove(0x12, 0, -1.0f);
        m_HangYaw = m_pItem->Rotation().y;
        m_Unk360 = 8.0f;
        m_bUnk364 = 1;
        return;
    }
    if (!m_HoldState && m_Health > 0.0f && CheckWallMove()) return;
    if (m_Unk344 > 0.0f) {
        if (0.0f >= m_Unk344 - 8.0f && (m_ButtonsPressed & 0x20)) pPhys->m_Accel.y = 0.02440000f;
        m_Unk344 -= XApp::GameFrameRateMul();
    }
    CheckForLand();
}

// NEAR (see check): builds the player: the predator animator (orange skin, idle), the upgrade-scaled
// health and energy with per-level overrides (KillBruno and Midsummer start at 25, the Labs and
// 0x1000186 at 10 health and no energy, ComeToMama at 30% energy), the physics, the twelve weapon
// entities (each with its own render info and tex swap) and the upgrade graphics.
signed char XItemHandler_Player::Initialise()
{
    EXItemAnimator_AnimModifier *pAnim = new EXItemAnimator_AnimModifier;
    if (!pAnim) return 0;
    InitUpgrades();
    InitClassVars();
    GameData::Instance(1)->SettingsRestore(1);
    m_Health = GetMaxHealth();
    m_Energy = GetMaxEnergy();
    switch (gCurrentMap_GeoHash) {
    case 0x10000cd:
        m_Energy = GetMaxEnergy() * 0.3f;
        break;
    case 0x1000056:
        m_Energy = 25.0f;
        break;
    case 0x1000136:
        m_Health = 25.0f;
        m_Energy = 25.0f;
        break;
    case 0x10000b4:
    case 0x1000186:
        m_Health = 10.0f;
        m_bUnk209 = 0;
        m_Energy = 0.0f;
        break;
    }
    SetDefaultAmmo();
    GameData::Instance(1)->SettingsRestore(1);
    m_SkinC4 = -1;
    m_SkinBC = -1;
    m_SkinC0 = 0;
    AU_AssignSkinSkelAnim(pAnim, 0x1000024, 0xd0000cc, 0x1000024, 0xd000010, 0x1000005, 0x3000002);
    m_pAnim = pAnim;
    m_pItem->AddAnimator(pAnim);
    m_pAnim->InitControllerStack(0, 6, 1);
    m_pAnim->InitControllerStack(1, 6, 1);
    m_pAnim->InitControllerStack(2, 6, 1);
    m_pAnim->SetFrameRate60(60.0f);
    m_pAnim->RenderInfo().SetExtRenderInfo(&m_PlayerExtRenderInfo);
    m_pAnim->RenderInfo().SetTexSwapInfo(&m_PlayerTexSwap);
    ((XItem *)m_pItem)->m_Unk160 = 0x9000002;
    m_pItem->SetItemPhysics((EXItemPhysics *)EXRUNTIME_CLASS(XItemPhysics)->CreateObject());
    m_bPlayerHasControl = 0;
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(0x1000013, 0, 0);
    for (int w = 1; w < 8; w++) {
        LoadUpgradeWeaponGFX(w, 0);
        if (gCurrentMap_GeoHash == 0x1000008 || gCurrentMap_GeoHash == 0x10000d1) {
            LoadUpgradeWeaponGFX(w, -2);
            LoadUpgradeWeaponGFX(w, -1);
            LoadUpgradeWeaponGFX(w, 1);
            LoadUpgradeWeaponGFX(w, 2);
        }
    }
    for (int i = 0; i < 12; i++) {
        m_pWeaponItem[i] = new EXItemAnimator_Entity;
        m_pWeaponItem[i]->Initialise(pFile, 0x2000146);
        m_pItem->AddAnimator(m_pWeaponItem[i]);
        m_pWeaponItem[i]->RenderFlags().BitSet(1);
        m_pWeaponItem[i]->ClrHasRender();
        m_pWeaponItem[i]->RenderInfo().SetExtRenderInfo(&m_WeaponExtRenderInfo[i]);
        m_pWeaponItem[i]->RenderInfo().SetTexSwapInfo(&m_WeaponTexSwap[i]);
        m_WeaponTexSwap[i].StopAllSwaps();
    }
    UpdateWeaponUpgradeGFX(-1);
    InitPlayerBodyWeapons();
    UpdateSectionFiles();
    if (gCurrentMap_GeoHash == 0x10000b4) SetupMove(0x13, 4, -1.0f);
    return 1;
}

// NEAR: written for behaviour (3.4 KB, many x87 temporaries); call order checked against retail.
// Firing / using the weapon in slot `slot`: per-weapon checks (ammo, energy, cool-down), first-person
// shots, the plasma caster (muzzle datum, camera ray in first person, else the lock-on target or the
// barrel's forward axis), the hang/medikit/vision items, then the melee and special-move entries.
void XItemHandler_Player::UseWeapon(int slot)
{
    unsigned int w = m_CurrentWeapon[slot];
    if (w == 0xd) return;
    if ((m_Mode == 0x16 || m_HoldState == 1) && w != 7) {
        if ((int)w < 0x3f) return;
        if ((int)w > 0x41) return;
    }
    switch (w) {
    case 5:
    case_ammo:
        if (m_Unk1ec > 0.0f) return;
        if (m_Ammo[m_CurrentWeapon[slot]] < 1) goto fail;
        break;
    case 6:
        if (m_Unk1ec > 0.0f) return;
        if (m_Energy >= 10.0f && m_bDiskInHand) break;
        goto fail;
    case 7:
        if (m_Unk1ec > 0.0f) return;
        if (m_Mode == 0x17 || !m_bCannonUp) goto fail;
        break;
    case 8:
    case 9:
    case 10:
    case 11:
        if (CheckTargetThrowAngle()) goto case_ammo;
        goto fail;
    case 12:
        if (GetMaxHealth() - 1.0f <= m_Health || m_Ammo[12] < 1) goto fail;
        break;
    }
    if (XGetCamera(0)->IsFirstPerson()) {
        switch (m_CurrentWeapon[slot]) {
        case 5:
        case 6:
        case 8:
        case 9:
        case 10:
        case 11:
            if (m_Mode == 0x11) return;
            m_ThrowWeapon = m_CurrentWeapon[slot];
            ShootWeapon(m_CurrentWeapon[slot], 0);
            return;
        }
    }
    w = m_CurrentWeapon[slot];
    switch (w) {
    case 7: {
        EXDatumTemp d;
        if (!m_pItem->GetAnimatorDatum(0x10000037, d, 0, 0)) return;   // HT_AnimDatum_Cannon_Barrel_Muzzle
        EXVector dir;
        d.m_rotation.GetRotation(dir, 4);
        EXMatrix mat;
        d.m_rotation.GetRotation(mat);
        EXVector start, aim;
        float speed;
        if (XGetCamera(0)->IsFirstPerson()) {
            float e = XGetCamera(0)->Elevation();
            float dx = (float)(sin(XGetCamera(0)->Angle()) * sin(e));
            e = XGetCamera(0)->Elevation();
            float dz = (float)(cos(XGetCamera(0)->Angle()) * sin(e));
            float dy = (float)cos(XGetCamera(0)->Elevation());
            float dw = 0.0f;
            XItemHandler_Camera *pCam = XGetCamera(0);
            start = *(EXVector *)((char *)pCam + 0x6c);   // camera position
            EXVector delta(dx * 25.0f, dy * 25.0f, dz * 25.0f, dw * 25.0f);
            EXRayCastTemp ray;
            ray.m_face.m_pEntity = 0;
            ray.Set(start, delta, 0, 0, 0, 0);
            theItemEnv.DoRayCastToContoured(ray);
            theItemEnv.DoRayCastToMap(ray, 0);
            float t = ray.m_t * 25.0f;
            aim.x = t * dx + start.x;
            aim.y = t * dy + start.y;
            aim.z = t * dz + start.z;
            aim.w = t * dw + start.w;
            // fire from beside the camera (the shoulder), not from its centre
            start.x = (float)(sin(XGetCamera(0)->Angle() - 1.5707964f) * 0.1f + start.x);
            start.z = (float)(cos(XGetCamera(0)->Angle() - 1.5707964f) * 0.1f + start.z);
            aim.x -= start.x;
            aim.y -= start.y;
            aim.z -= start.z;
            aim.w -= start.w;
            float l = aim.w * aim.w + aim.y * aim.y + aim.z * aim.z + aim.x * aim.x;
            if (l >= 0.0f) {
                l = sqrtf(l);
                if (l > 1e-5f) {
                    float k = 1.0f / l;
                    aim.x *= k;
                    aim.y *= k;
                    aim.z *= k;
                    aim.w *= k;
                }
            }
        } else {
            start = d.m_position;
            if (m_pTrackTarget) {
                // the lock-on target's position (0x3f0)
                const EXVector &tgt = m_AimPos;
                aim.x = tgt.x - start.x;
                aim.y = tgt.y - start.y;
                aim.z = tgt.z - start.z;
                aim.w = tgt.w - start.w;
            } else {
                EXVector fwd(0.0f, 0.0f, 1.0f, 0.0f), out;
                D3DXVec4Transform(&out, &fwd, &mat);   // the barrel's forward axis
                aim = out;
            }
        }
        unsigned int snd;
        int level = GetPlasmaChargeSpeed(speed);
        if (level == 1) {
            m_PlasmaCharge = 0.5f;
            m_Energy -= 30.0f;
            snd = 0x1af00543;
        } else if (level == 2) {
            m_PlasmaCharge = 1.0f;
            m_Energy -= 50.0f;
            snd = 0x1af00236;
            if (m_bCloak) SparkSkin(0.5f);
        } else {
            m_PlasmaCharge = 0.0f;
            m_Energy -= 25.0f;
            snd = 0x1af00542;
        }
        XItemHandler_Bullet::Create(start, dir, aim, 7, 1, 0, 0, speed);
        m_Unk1ec = 0.5f;
        Actor::OverridePlayerCloak();
        EXBaseSoundManager::GetSoundManager()->StartSound(snd, m_pItem->Position(), -1, 0);
        WeaponProfile *pw = WeaponProfile::GetWeaponData(0x49000009);   // HT_Weaponry_Predator_Plasmacaster
        m_WeaponUse[7]++;
        if (*(int *)((char *)pw + 0xdc))
            XItemHandler_ScriptBase::FX_CreateLink(*(int *)((char *)pw + 0xdc), -1, 0x10000037, 0x40, 0x1000016);
        return;
    }
    case 8:
    case 9:
    case 10:
    case 11:
        if (m_Mode == 0xc && !(m_StateFlags & 8)) {
            // throwing from a hang
            if (m_SubMode == 3) return;
            if (m_SubMode == 6) return;
            m_ThrowWeapon = w;
            m_HandLock = -1.0f;
            if (m_StateFlags & 0x40)
                SetupMove(0xc, 6, -1.0f);
            else
                SetupMove(0xc, 3, -1.0f);
            return;
        }
        break;
    case 12:
        // medikit
        if (m_Mode < 2) {
            DeadStop(1);
            SetWeapon(3);
            SetupMove(0x17, 9, -1.0f);
            ResetAttack();
            return;
        }
        if (m_Mode == 0x11) return;
        goto fail;
    case 0x3f:
    case 0x40:
    case 0x41:
        if (XGetCamera(0)->IsFirstPerson()) {
            if (!*((char *)FEHud::Instance(1) + 0xd1c)) {
                SendVMMessages(m_CurrentWeapon[slot]);
                return;
            }
            goto fail;
        }
        if (m_Mode < 2) {
            DeadStop(1);
            SetupMove(0x17, 10, -1.0f);
            return;
        }
        if (m_Mode == 0x11) return;
        goto fail;
    }
    switch (w) {
    case 0:
    case 1:
        if ((m_SpecialMoveAvailable & 2) && DoGrabMove(1)) return;
        if ((m_SpecialMoveAvailable & 1) && DoStealthKill(1)) return;
        if ((m_SpecialMoveAvailable & 8) && DoGroundAttack(slot)) return;
        if (CheckForRechargePoint()) return;
        if (CheckForBeaconPoint()) return;
        if (CheckForTOS()) return;
        break;
    case 2:
    case 3:
    case 4:
        if ((m_SpecialMoveAvailable & 8) && DoGroundAttack(slot)) return;
        break;
    }
    switch (m_Mode) {
    case 1:
        // attacking from a run
        if (UBBing()) return;
        SetWeapon(slot);
        switch (m_CurrentWeapon[slot]) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
            SetupMoveBlend(6, 0, 0.0f);
            m_Unk224 = 1.0f;
            m_Unk214 = 1;
            return;
        case 5:
        case 6:
        case 8:
        case 9:
        case 10:
        case 11:
            SetupMoveBlend(6, 0, 0.0f);
            m_Unk224 = 1.0f;
            return;
        }
        // other weapons: as from standing
    case 0: {
        SetWeapon(slot);
        unsigned int cw = m_CurrentWeapon[slot];
        if ((int)cw >= 0 && (int)cw < 5) {
            signed char bHeavy = 0, bChain = 0;
            CalculateHitType(cw, bHeavy, bChain);
            if (bChain && WeaponAttackAnims[m_CurrentWeapon[slot]][4]) m_AttackState = (~(s8)m_AttackState & 1) | 4;
            m_Unk224 = bHeavy ? 2.0f : 1.0f;
        }
        SetupMove(6, 0, -1.0f);
        return;
    }
    default:
        return;
    case 3:
    case 9:
        switch (m_SubMode) {
        case 0:
        case 1:
        case 2:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
            UseWeaponJump(slot);
            return;
        default:
            return;
        }
    case 5:
    case 0xc:
    case 0x18:
        goto fail;
    case 10:
        if (m_SubMode != 2) return;
        UseWeaponJump(slot);
        return;
    case 0x12:
        switch (m_CurrentWeapon[slot]) {
        case 5:
        case 6:
        case 8:
        case 9:
        case 10:
        case 11:
            if (UBBing()) return;
            SetWeapon(slot);
            SetupMoveBlend(6, 0, 0.0f);
            m_Unk224 = 1.0f;
            return;
        }
        goto fail;
    }
fail:
    EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00576, m_pItem->Position(), -1, 0);
}

// NEAR: register allocation and block layout (22 calls in the same order as retail). The weapon/cloak/vision buttons: cloak toggle, vision cycle, wrist blade/disc swap,
// plasma caster charge, finishing move, and UseWeapon per held weapon button.
void XItemHandler_Player::HandleWeaponButtons()
{
    int i = 0;
    if (!m_bPlayerHasControl || *(int *)((char *)FEHud::Instance(1) + 0xb8c) > 0) {
        m_PlasmaChargeTime = 0.0f;
        return;
    }
    if (m_ButtonsHeld & 0x800) {
        if (!m_bCloak) {
            if (!(m_PredMsgFlags & 0x10000000))
                EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00576, m_pItem->Position(), -1, 0);
            else if (m_Wet) {
                EXBaseSoundManager::GetSoundManager()->StartSound(0x1af001c5, m_pItem->Position(), -1, 0);
                SparkSkin(2.5f);
            } else if (m_Energy <= 0.0f)
                EXBaseSoundManager::GetSoundManager()->StartSound(0x1af001c5, m_pItem->Position(), -1, 0);
            else
                SetCloak(1);
        } else
            SetCloak(0);
    }
    if (m_ButtonsHeld & 0x1000) {
        if (m_EMPTimer <= 0.0f) {
            u8 cur = m_Tint;
            if (cur == 0 && m_Unkc71[0] != 0xff) {
                SetVisionMode(m_Unkc71[0]);
                goto vision_done;
            }
            short modes[4];
            modes[0] = 0;
            modes[1] = 1;
            modes[2] = 4;
            modes[3] = 2;
            int v = cur;
            short n = 0;
            do {
                v++;
                if ((short)v > 3) v = 1;
                if (m_VisionModeFlags & modes[(short)v]) {
                    if ((u8)v != cur) VisionWipe::Instance(1)->Start((u8)v);
                    m_Unkc71[0] = (u8)v;
                    goto vision_done;
                }
                n++;
            } while (n < 4);
        }
        EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00576, m_pItem->Position(), -1, 0);
    }
vision_done:
    if ((m_ButtonsHeld & 0x2000) && m_Tint) VisionWipe::Instance(1)->Start(0);
    if ((m_ButtonsHeld & 0x400) && (signed char)(m_MoveFlags >> 8) >= 0) {
        if (m_CurrentWeapon[3] == 1)
            SetCurrentWeapon(3, 0);
        else if (FEHud::Instance(1)->IsWeaponAvailable(1))
            SetCurrentWeapon(3, 1);
        else
            EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00576, m_pItem->Position(), -1, 0);
    }
    if (m_Mode == 7 || m_Mode == 0x17 || m_Mode == 0x13) {
        m_PlasmaChargeTime = 0.0f;
        return;
    }
    if (!(m_ButtonsHeld & 1)) {
plasma:
        if (m_CurrentWeapon[0] == 7 && m_bCannonUp) {
            if (!(m_ButtonsPressed & 1)) {
                if (m_PlasmaChargeTime > 0.0f) {
                    UseWeapon(0);
                    m_PlasmaChargeTime = 0.0f;
                    return;
                }
                goto buttons;
            }
            if (m_Energy < 25.0f)
                EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00576, m_pItem->Position(), -1, 0);
            else {
                m_PlasmaChargeTime += XApp::GameFrameRateMul();
                if (m_PlasmaChargeTime < 540.0f) goto buttons;
            }
            m_ButtonLock = 1;
        }
    } else if (m_CurrentWeapon[0] == 7) {
        if (m_Energy < 25.0f)
            EXBaseSoundManager::GetSoundManager()->StartSound(0x1af005db, m_pItem->Position(), -1, 0);
        goto plasma;
    }
    m_PlasmaChargeTime = 0.0f;
buttons:
    if (m_Mode != 0x16 && m_HoldState != 1) {
        switch (m_Mode) {
        case 0:
        case 1:
        case 2:
        case 6:
        case 8:
            if ((m_ButtonsPressed & 2) && (m_ButtonsPressed & 8) && (int)m_CurrentWeapon[3] >= 0 && (int)m_CurrentWeapon[3] < 2 &&
                (m_SpecialMoveAvailable & 4) && DoFinishingMove(1))
                return;
        }
    }
    unsigned int bit = 8;
    do {
        if (i == 0) bit = 1;
        else if (i == 1) bit = 2;
        else if (i == 2) bit = 4;
        else if (i == 3) bit = 8;
        if ((m_ButtonsHeld & bit) && m_CurrentWeapon[i] != 7) UseWeapon(i);
        i++;
    } while (i < 4);
}

// Running on the ground (mode 1). flags: 1 no stick input, 4 walk (speed capped at 0.5),
// 8 no attack or jump checks.
// - A reversal of more than 100 degrees within 3 updates while moving (target speed > 0.09)
//   starts a skid (mode 0xe) and stops dead.
// - Otherwise the facing eases toward the stick (InterpAngle, 0.12) and the body banks into
//   the turn (up to pi/8, at most 0.061 rad a frame).
// - Speed: toward the stick magnitude, cut while turning hard. Speeding up closes 20% of the
//   gap a frame (pow(0.8, mul)); slowing down keeps 93.75% a frame (pow(0.9375, mul)), the slide.
// - Then a chained attack, a jump or a fall may take over.
float InterpAngle(float a, float b, float t, float p);
signed char XItemHandler_Player::MoveNormal(int flags)
{
    EXPlacementItem *pItem = m_pItem;
    EXVector rot = pItem->Rotation();
    float diff = Vec_AngleDifference(rot.y, m_PadAngle);
    m_PadHistory[0] = m_PadHistory[1];
    m_PadHistory[1] = m_PadHistory[2];
    m_PadHistory[2] = m_PadHistory[3];
    m_PadHistory[3].m_Angle = m_PadAngle;
    m_PadHistory[3].m_Update = m_UpdateCount;
    float reversal = 0.0f;
    if (m_UpdateCount - m_PadHistory[0].m_Update == 3) reversal = (float)fabs(Vec_AngleDifference(m_PadHistory[0].m_Angle, m_PadAngle));
    float mag = m_StickMag;
    if (flags & 1) mag = 0.0f;
    if (flags & 4) mag = mag > 0.5f ? 0.5f : mag;
    if (reversal > 1.7453293f && m_MSpeedTarget > 0.09f) {
        XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
        m_Speed = EXVector(0.0f, 0.0f, 0.0f, 0.0f);
        m_Unk2b8 = EXVector(0.0f, 0.0f, 0.0f, 0.0f);
        pPhys->m_DeltaMove = EXVector(0.0f, 0.0f, 0.0f, 0.0f);
        EXPlacementItem *pI = m_pItem;
        pI->m_rotation = EXVector(0.0f, pI->Rotation().y, 0.0f, 0.0f);
        pI->m_validMtx = 0;
        m_SkidTurn = Vec_AngleDifference(rot.y + 3.1415927f, m_PadAngle) * 0.045454547f;
        SetupMove(0xe, diff < 0.0f ? 0 : 1, -1.0f);
        return 1;
    }
    float turn = InterpAngle(0.0f, diff, 0.12f, XApp::GameFrameRateMul());
    rot.y += turn;
    float bank = diff * -0.16666667f;
    if (bank >= 0.3926991f) bank = 0.3926991f;
    else if (bank <= -0.3926991f) bank = -0.3926991f;
    float bd = Vec_AngleDifference(m_MSpeedTarget * 5.333447f * bank, rot.z);
    float lim = XApp::GameFrameRateMul() * 0.061086524f;
    float step = bd < lim ? (-lim < bd ? bd : -lim) : lim;
    pItem = m_pItem;
    pItem->m_rotation = EXVector(rot.x, rot.y, rot.z - step, rot.w);
    pItem->m_validMtx = 0;
    float t2 = (float)fabs(turn) + (float)fabs(turn);
    if (t2 > 1.0f) mag /= t2;
    float speed = sqrtf(m_Speed.x * m_Speed.x + m_Speed.z * m_Speed.z);
    float ns;
    if (mag + 0.0001f < speed) ns = (float)pow(0.9375, (double)XApp::GameFrameRateMul()) * speed;
    else ns = (speed - mag) * (float)pow(0.7999999970197678, (double)XApp::GameFrameRateMul()) + mag;
    if (ns < mag) ns = ns > 0.0f ? ns : 0.0f;
    else ns = mag;
    pItem = m_pItem;
    m_Speed.x = (float)sin(pItem->Rotation().y) * ns;
    m_Speed.z = (float)cos(pItem->Rotation().y) * ns;
    if (m_Unk214 == 1) {
        if (flags & 8) return CheckForFall() != 0;
        CheckChainButtonPress();
        if (m_bAttacking && (m_AttackHit == 1 || m_AttackHit == 3)) {
            SetWeapon(m_AttackHit);
            m_Unk224 = 2.0f;
            m_AttackState = 1;
            m_AttackHit = -1;
            m_bAttacking = 0;
            m_HitMoveScale = 1.0f;
            SetupMove(6, 0, -1.0f);
            EXPlacementItem *pI = m_pItem;
            EXVector r = pI->Rotation();
            pI->m_rotation = EXVector(r.x, r.y, 0.0f, r.w);
            pI->m_validMtx = 0;
            return 0;
        }
        m_bAttacking = 0;
    }
    if (!(flags & 8) && CheckForJump(1)) return 1;
    return CheckForFall() != 0;
}

// Moving while locked on (mode 1, submodes 3-7), every frame.
// - Faces the lock-on target (chest for an actor, the target point for others; straight ahead
//   without one), turning at InterpAngle rate 0.16.
// - Jump held: a jump in the stick's direction relative to the target (back flip 3/7, side or
//   forward 3/8, straight up 3/9), 3.217 high; standing on a vehicle/actor platform with the
//   target in reach and in front: an attack jump onto it (3/3), height from the distance.
// - Otherwise: when the movement direction is more than 15 degrees off the stick, the speed is
//   steered toward the stick (0.2 a frame, cap 0.9) and the strafe-turn move (1/3) or the strafe
//   idle (1/6) plays; else the speed eases toward the stick like the run (0.8 / 0.9375).
// - m_Unk230 is the strafe blend (-3..3: back left .. back right), eased with InterpAngle.
void XItemHandler_Player::MoveStrafe()
{
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
    m_CamAngle = XGetCamera(m_PlayerIndex)->Angle();
    float padx = 0.0f, padz = 0.0f;
    EXVector to;
    int type;
    void *pLock = XGetCamera(0)->GetLockOn(type);
    switch (type) {
    case 1:
        if (!pLock) goto ahead;
        to = ((Actor *)pLock)->GetChestPos();
        to -= m_pItem->Position();
        break;
    case 2: case 3: case 4: case 5: case 6:
        if (pLock) {
            to = ((Actor *)pLock)->LocalVelocity();
            if (type == 2) to.y += 0.5f;
            to -= m_pItem->Position();
            break;
        }
    default:
    ahead: {
        float ry = m_pItem->Rotation().y;
        to.y = 0.0f;
        to.w = 0.0f;
        to.x = (float)sin(ry) * 10.0f;
        to.z = (float)cos(ry) * 10.0f;
    }
    }
    signed char flag = 1;
    int ptype;
    EXItem *pPlat = GetPlatformItem(ptype);
    if (pPlat && ptype == 2) {
        int t2;
        Actor *pA = (Actor *)XGetCamera(0)->GetLockOn(t2);
        if (t2 == 1 && pA) {
            EXDatumTemp d;
            if (!pPlat->GetAnimatorDatum(0x10000060, d, 1, 0)) {
                memset(&d, 0, sizeof d);
                d.m_position = ((EXPlacementItem *)pPlat)->Position();
            }
            EXVector dv = d.m_position - m_pItem->Position();
            if (dv.z * dv.z + dv.x * dv.x <= 144.0f) {
                if (!pA->SearchBrainStateStack(STATE_UlyssesHitbyEMP)) {
                    EXVector r;
                    d.m_rotation.GetRotation(r, 4);
                    if ((float)fabs(Vec_AngleDifference(r.y, (float)atan2(dv.x, dv.z))) > 0.7853982f) {
                        flag = 0;
                        goto jumpcheck;
                    }
                }
                if (!m_SpecialMoveAvailable) {
                    m_SpecialMoveAvailable = 0x10;
                    m_GroundAttackID = pA->m_RefID;
                }
            } else
                flag = 0;
        }
    }
jumpcheck:
    if (!(*(unsigned char *)&m_ButtonsHeld & 0x20)) {
        EXPlacementItem *pI = m_pItem;
        float want = (float)atan2(to.x, to.z);
        EXVector rot = pI->Rotation();
        float turn = InterpAngle(0.0f, Vec_AngleDifference(rot.y, want), 0.16f, XApp::GameFrameRateMul());
        flag = 0;
        m_Unk21c = (float)fabs(turn * 0.2f);
        pI->m_rotation = EXVector(rot.x, rot.y + turn, rot.z, rot.w);
        pI->m_validMtx = 0;
        XItemHandler_Camera *pCam = XGetCamera(m_PlayerIndex);
        float rel;
        float sp = sqrtf(m_Speed.x * m_Speed.x + m_Speed.z * m_Speed.z);
        if (sp < 0.001f) rel = m_StickAngle;
        else {
            float a = (float)atan2(m_Speed.x, m_Speed.z);
            rel = a - pCam->Angle();
        }
        if ((float)fabs(rel) >= 3.1415927f) {
            float w = (float)fmod((double)rel, 6.2831854820251465);
            if ((float)fabs(w) >= 3.1415927f) {
                if (rel < 0.0f) w += 6.2831855f;
                else w -= 6.2831855f;
            }
            rel = w;
        }
        if ((float)fabs(rel) >= 0.2617994f) {
            if (sqrtf(m_Pad.x * m_Pad.x + m_Pad.z * m_Pad.z) > 0.001f) {
                padx = (float)sin(m_PadAngle) * 0.9f;
                padz = (float)cos(m_PadAngle) * 0.9f;
            }
            float old = sqrtf(m_Speed.z * m_Speed.z + m_Speed.x * m_Speed.x);
            EXVector dv(padx - m_Speed.x, 0.0f - m_Speed.y, padz - m_Speed.z, 0.0f - m_Speed.w);
            float step = XApp::GameFrameRateMul() * 0.2f;
            if (step < sqrtf(dv.z * dv.z + dv.x * dv.x)) {
                dv.Normalise();
                dv *= step;
            }
            m_Speed += dv;
            float now = sqrtf(m_Speed.z * m_Speed.z + m_Speed.x * m_Speed.x);
            if (sqrtf(m_Speed.z * m_Speed.z + m_Speed.x * m_Speed.x) >= 0.8f || old - now < 0.0f) {
                if (m_SubMode != 3) {
                    flag = 1;
                    float f = -1.0f;
                    if (m_Mode == 1 && (m_SubMode == 0 || m_SubMode == 4))
                        f = ((EXItemAnimator_Anim *)m_pItem->GetAnimator(EXRUNTIME_CLASS(EXItemAnimator_Anim)))->Controller(0)->Frame() * 34.0f + 1.0f;
                    SetupMove(1, 3, f);
                }
            } else if (m_SubMode != 6) {
                m_Unk234 = 0;
                m_Unk21c = 0.0f;
                SetupMove(1, 6, -1.0f);
            }
        } else {
            float mag = m_StickMag;
            float ns;
            if (mag <= GetSpeed()) ns = (float)pow(0.9375, (double)XApp::GameFrameRateMul()) * GetSpeed();
            else {
                float cur = GetSpeed();
                ns = (cur - mag) * (float)pow(0.7999999970197678, (double)XApp::GameFrameRateMul()) + mag;
            }
            if (ns < mag) mag = ns > 0.0f ? ns : 0.0f;
            float pa = m_PadAngle;
            m_Speed.w = 0.0f;
            m_Speed.y = 0.0f;
            signed char moving = 0;
            m_Speed.x = (float)sin(pa) * mag;
            m_Speed.z = (float)cos(pa) * mag;
            switch (m_SubMode) {
            case 0: case 1: case 2: case 3: case 4:
                if (m_StickMag == 0.0f) {
                    EXPlacementItem *pR = m_pItem;
                    pR->m_rotation = EXVector(pR->Rotation().x, pR->Rotation().y, 0.0f, pR->Rotation().w);
                    pR->m_validMtx = 0;
                    SetupMove(1, m_MSpeedTarget >= 0.18f ? 7 : 6, -1.0f);
                    m_Unk234 = 0;
                    m_Unk21c = 0.0f;
                    return;
                }
            case 6:
                moving = 1;
                break;
            case 7:
                moving = 1;
                if (AnimEnded()) SetupMove(1, 6, -1.0f);
                break;
            }
            if (m_StickMag != 0.0f && moving && m_SubMode != 4) {
                if (m_Mode == 1 && (m_SubMode == 0 || m_SubMode == 3 || m_SubMode == 2)) {
                    float f = ((EXItemAnimator_Anim *)m_pItem->GetAnimator(EXRUNTIME_CLASS(EXItemAnimator_Anim)))->Controller(0)->Frame() * 34.0f + 1.0f;
                    if (f >= 0.0f) {
                        SetupMove(1, 5, f);
                        goto blend;
                    }
                }
                SetupMove(1, 4, -1.0f);
            }
        }
    blend:
        pCam = XGetCamera(m_PlayerIndex);
        {
            float a = (float)atan2(m_Speed.x, m_Speed.z);
            rel = a - pCam->Angle();
            if ((float)fabs(rel) >= 3.1415927f) {
                float w = (float)fmod((double)rel, 6.2831854820251465);
                if ((float)fabs(w) >= 3.1415927f) {
                    if (rel < 0.0f) w += 6.2831855f;
                    else w -= 6.2831855f;
                }
                rel = w;
            }
        }
        float b;
        if ((float)fabs(rel) >= 1.5717964f) {
            float sgn = rel >= 0.0f ? 1.0f : -1.0f;
            float v = (((float)fabs(rel) - 1.5707964f) * 0.63661975f + 2.0f) * sgn;
            b = v < 3.0f ? (v > -3.0f ? v : -3.0f) : 3.0f;
            if (b > -2.0f && b < 0.0f) b = -2.0f;
            else if (b > 0.0f && b < 2.0f) b = 2.0f;
        } else {
            float v = (rel + 1.5707964f) * 0.31830987f * 2.0f - 1.0f;
            b = v < 1.0f ? (v > -1.0f ? v : -1.0f) : 1.0f;
        }
        if (flag) m_Unk230 = b;
        float mul = XApp::GameFrameRateMul();
        if ((m_Unk230 > -2.0f && m_Unk230 < -1.0f) || (m_Unk230 > 1.0f && m_Unk230 < 2.0f)) {
            m_Unk230 = InterpAngle(m_Unk230 * 0.33333334f * 3.1415927f, b * 0.33333334f * 3.1415927f, 0.1f, XApp::GameFrameRateMul()) * 0.31830987f * 3.0f;
        } else {
            float from = m_Unk230 * 0.33333334f * 3.1415927f;
            float tob = b * 0.33333334f * 3.1415927f;
            float r = InterpAngle(from, tob, 0.25f, XApp::GameFrameRateMul()) * 0.31830987f * 3.0f;
            if ((r > -2.0f && r < -1.0f) || (r > 1.0f && r < 2.0f))
                m_Unk230 = InterpAngle(from, tob, 0.1f, mul) * 0.31830987f * 3.0f;
            else
                m_Unk230 = r;
        }
        if (m_Unk214 == 1) {
            CheckChainButtonPress();
            if (m_bAttacking && (m_AttackHit == 1 || m_AttackHit == 3)) {
                SetWeapon(m_AttackHit);
                m_AttackState = 1;
                m_AttackHit = -1;
                m_bAttacking = 0;
                m_Unk224 = 2.0f;
                SetupMove(6, 0, -1.0f);
                return;
            }
            m_bAttacking = 0;
        }
        CheckForFall();
        return;
    }
    {
        int dir = 0;
        if (m_StickMag > 0.2f) {
            float a = m_StickAngle;
            if (a < -2.3561945f || a > 2.3561945f) { dir = 1; goto jump; }
            if (a < -0.7853982f) { dir = 2; goto jump; }
            if (a >= 0.7853982f) { dir = 4; goto jump; }
            dir = 3;
        }
        if (!pPlat || !flag) {
        jump:
            m_Unk344 = 0.0f;
            pPhys->SetThrustToAttainHeight(3.217f);
            EXItem *pItem = m_pItem;
            gpGameWnd[0]->SwooshSysMgr()->DeleteAllSystems(pItem);
            switch (dir) {
            case 0: SetupMove(3, 9, -1.0f); return;
            case 1: SetupMove(3, 7, -1.0f); return;
            case 2: case 3: case 4:
                m_Unk228 = m_StickAngle * 0.15915494f * 4.0f;
                SetupMove(3, 8, -1.0f);
                return;
            }
            return;
        }
        float dist = sqrtf(to.y * to.y + to.x * to.x + to.z * to.z);
        float cap = 25.0f;
        float h = (dist < cap ? dist : cap) * 0.4f + 1.0f;
        float lo, hi = 10.0f;
        if (ptype < 2 || ptype > 3) lo = 1.0f;
        else lo = 3.217f;
        h = h < hi ? (h > lo ? h : lo) : hi;
        m_Unk344 = 0.0f;
        pPhys->SetThrustToAttainHeight(h);
        EXItem *pItem = m_pItem;
        m_JumpTargetType = ptype;
        gpGameWnd[0]->SwooshSysMgr()->DeleteAllSystems(pItem);
        SetupMove(3, 3, -1.0f);
    }
}

// Landing, every frame in the air (DoJump): once the physics reports ground contact.
// - Horizontal speed on touchdown is fixed by the run speed being aimed for: 1.0 (above 0.18),
//   0.9 (above 0.09), 0.45 (above 0.01) or 0, in the direction of the air movement.
// - Landing at under 0.69 down: soft. Stick nearly idle: land in place (mode 0/0xb, its speed
//   from the impact); otherwise straight into the run (1/0, or 1/2 above 0.3 down; carrying
//   someone: 0x16/3).
// - Faster (or stomping an actor): fall damage (not for a stomp) and the hard landing (mode 8),
//   with the landing effect, cloak spark and rumble.
void XItemHandler_Player::CheckForLand()
{
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
    if (!(pPhys->m_collisionFlags & 1)) return;
    float speed = 1.0f;
    if (0.18f > m_MSpeedTarget) speed = 0.9f;
    if (0.09f > m_MSpeedTarget) speed = 0.45f;
    if (0.01f > m_MSpeedTarget) speed = 0.0f;
    float a = (float)atan2(m_Unk2b8.x, m_Unk2b8.z);
    m_Speed.w = 0.0f;
    m_Speed.y = 0.0f;
    m_Speed.x = (float)sin(a) * speed;
    m_Speed.z = (float)cos(a) * speed;
    m_bLedgeTarget = 0;
    m_Unk35c = 0.0f;
    m_Unk358 = 0.0f;
    m_FallTimer = 0.0f;
    m_Unk2b8 = EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    EXItem *pItem = m_pItem;
    gpGameWnd[0]->SwooshSysMgr()->DeleteAllSystems(pItem);
    pPhys->m_physicsFlags &= ~3;
    m_CamAngle = XGetCamera(m_PlayerIndex)->Angle();
    if (m_Health <= 0.0f) {
        SetupMove(0, 0, -1.0f);
        KillPlayer();
        return;
    }
    EXPlacementItem *pI = m_pItem;
    pI->m_rotation = EXVector(0.0f, pI->Rotation().y, 0.0f, pI->Rotation().w);
    pI->m_validMtx = 0;
    if (theApp.m_FadeActive) {
        SetupMove(0, 0, -1.0f);
        return;
    }
    int sound;
    if (!CheckForActorStomp()) {
        float v = pPhys->m_ImpactSpeed;
        if (v >= -0.69f) {
            if (m_StickMag <= 0.3f) {
                float f = v;
                if (f < 0.0f) { if (f <= -0.66f) f = -0.66f; }
                else f = 0.0f;
                m_ShimmyDir = ((0.0f - f) - 0.25f) * 2.4390244f + 1.0f;
                SetupMove(0, 0xb, -1.0f);
            } else if (!m_HoldState) {
                SetupMove(1, v >= -0.3f ? 0 : 2, -1.0f);
            } else {
                SetupMove(0x16, 3, -1.0f);
            }
            sound = 2;
            goto landed;
        }
        InflictDamageFromFall(v);
    }
    SetupMove(8, 0, -1.0f);
    sound = 3;
landed:
    EXBaseSoundManager::GetSoundManager()->StartSound(GroundSounds[m_FloorType][sound], m_pItem->Position(), -1, 0);
    {
        EXVector feet = GetMidFeetPos();
        feet.y = m_pItem->Position().y;
        if (sound == 2) {
            if (m_Wet) XItemHandler_ScriptBase::FX_Create(feet, 0x4000519, 0x40, 0x1000016);
        } else if (sound == 3) {
            if (m_bCloak) SparkSkin(0.5f);
            m_RefractionBoost = 0.5f;
            Actor::OverridePlayerCloak();
            XItemHandler_ScriptBase::Explosion_Create(feet, m_Wet ? 0x4000518 : 0x40002d7, 3, 3, 0x1e, 0, 0, 0, 0);
            ((XGamePad *)EXBaseGamePad::GetGamePad())->PlayRumble(0x54000000, 1.0f, m_PadIndex);
        }
    }
}

signed char XItemHandler_Player::MoveCheckForActor()
{
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->ItemPhysics();
    signed char bCanBump = 1;
    if (m_Mode == 6) bCanBump = 0;
    if (m_Mode == 7 && m_SubMode == 12) bCanBump = 0;
    EXVector &dm = pPhys->DeltaMove();
    float mul = XApp::GameFrameRateMul();
    EXVector pos = m_pItem->Position() + EXVector(dm.x * mul, 0.0f, dm.z * mul, 0.0f);
    EXVector push(0.0f, 0.0f, 0.0f, 0.0f);
    float radius = (m_MoveFlags & 0x4000) ? 0.98f : 0.49f;
    float range = (Actor::GetMaxCollisionRadius() + radius) * 1.5f;
    for (Actor *pActor = Actor::GetFirst(pos, range, 0); pActor; pActor = pActor->GetNext(pos, range, 0)) {
        if (!pActor->CanCollideWithPlayer() && !pActor->m_StateFlags.m_bCanRipBossHead) continue;
        if (pActor->m_Status == 1) continue;
        EXVector d = pos - pActor->GetFeetPos();
        if ((float)fabs(d.y) > 0.5f) continue;
        float dist = d.LengthXZ();
        float rad = pActor->m_CollisionRadius;
        if (pActor->m_ActorFlags.m_bUlysses) {
            // Ulysses: radius grows up to 1.5x towards his sides.
            EXVector f;
            SetRotation(f, pActor->GetRotY(), 1.0f);
            EXVector u = -d;
            u.y = 0.0f;
            u.Normalise();
            rad *= (1.0f - (float)fabs(u.x * f.x + u.z * f.z + u.y * f.y)) * 0.5f + 1.0f;
        }
        float sum = rad + radius;
        if (dist > sum) continue;
        EXVector p;
        if (dist > 0.01f) p = d * ((sum - dist) / dist);
        else p = EXVector(0.0f, 0.0f, 0.0f, 0.0f);
        if (!(m_MoveFlags & 0x2000)) push += p * pActor->GetPlayerToActorColAdj();
        if (pActor->m_HitCountdown > 0.0f) continue;
        if (pActor->IsDead() && !pActor->m_StateFlags.m_bCanRipBossHead) continue;
        if (pActor->m_ActorFlags.m_bHolographic) continue;
        int level;
        if (m_MoveFlags & 0x4000) level = 3;
        else {
            if (!bCanBump) continue;
            level = 4;
            if (m_MSpeedTarget < 0.37f) level = 3;
            if (m_MSpeedTarget < 0.18f) level = 2;
            if (m_MSpeedTarget < 0.09f) level = 1;
            if (m_MSpeedTarget < 0.01f) level = 0;
            // Retail quirk: level 0 is unreachable, anything below 0.02 is skipped.
            if (m_MSpeedTarget < 0.02f) continue;
        }
        SendMsg_AttackedByPredator(pActor, 30, pos, -p, (float)level, 0);
    }
    push.y = 0.0f;
    push.w = 0.0f;
    if (push.LengthXZ() > 0.01f && m_bHandLockFlag) m_HandLock = -1.0f;
    pPhys->AddPFDeltaMove(push);
    return 0;
}

void XItemHandler_Player::DeadStop(signed char bRot)
{
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
    m_Speed.SetZero();
    m_Unk2b8.SetZero();
    pPhys->SetThrust(0.0f);
    pPhys->SetDeltaMove(EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    pPhys->SetYAccel(0.0f);
    if (bRot) m_pItem->SetRotation(EXQuaternion(0.0f, m_pItem->Rotation().y, 0.0f, 0.0f));
}

XItemHandler_Player::~XItemHandler_Player()
{
    m_SoundTag.RemoveAll(0);
    m_pInstance = 0;
    gpPlayerItem[0] = 0;
    EXWatcher::GetWatcher()->DelSection(EXWatcher::GetWatcher()->SectionHandle("Upgrades", 0));
    EXWatcher::GetWatcher()->DelSection(EXWatcher::GetWatcher()->SectionHandle("Player Values", 0));
}

signed char XItemHandler_Player::MonkeyCheckPullUp(EXGrabPointTemp &t)
{
    EXVector v = m_pItem->Position();
    v.y = m_ClamberPos.y + 0.3f;
    v.x += (float)sin(m_pItem->Rotation().y) * 0.75f;
    v.z += (float)cos(m_pItem->Rotation().y) * 0.75f;
    t.Set(v, 0.5f, (EXGrabPointTemp::GRABFITYPE)1);
    theItemEnv.DoGrabPointToMap(t);
    return t.m_24 == 1;
}

EXSwooshSys *XItemHandler_Player::CreateSwoosh(unsigned int hash, EXItem *pItem, EXItemAnimator_Entity *pEntity)
{
    EXSwooshSys *p = gpGameWnd[0]->SwooshSysMgr()->FindSwoosh(hash, pItem);
    if (p) gpGameWnd[0]->SwooshSysMgr()->DeleteSystem(p);
    p = gpGameWnd[0]->SwooshSysMgr()->CreateSystem(hash);
    if (p) {
        EXGeoFile *pFile = EXGeoFile::LoadGeoFile(0x1000013, 0, 0);
        if (pFile && p->Initialize(pFile, hash, pItem, pEntity, 0, 0, 0)) {
            p->SetTessalationLevel(3);
            return p;
        }
        gpGameWnd[0]->SwooshSysMgr()->DeleteSystem(p);
    }
    return 0;
}

void XItemHandler_Player::CheckChainButtonPress()
{
    m_WeaponDown += XApp::FrameLength();
    if (m_CurrentSlot != -1) {
        if (m_CurrentWeapon[1] != 13 && (m_ButtonsHeld & 2)) m_WeaponDownT[1] = m_WeaponDown;
        if (m_CurrentWeapon[3] != 13 && (m_ButtonsHeld & 8)) m_WeaponDownT[3] = m_WeaponDown;
        if (m_WeaponDownT[1] > -1.0f || m_WeaponDownT[3] > -1.0f) {
            if (m_WeaponDownT[1] > m_WeaponDownT[3])
                m_AttackHit = 1;
            else
                m_AttackHit = 3;
        }
    }
}

void XItemHandler_Player::RestorePreviousWeapon()
{
    if (!GameData::Instance(1)->Values()->m_bAutoEquip) {
        switch (m_PrevWeapon) {
        case 0:
        case 1:
            SetCurrentWeapon(3, m_PrevWeapon);
            break;
        case 2:
        case 3:
        case 4:
            SetCurrentWeapon(1, m_PrevWeapon);
            break;
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
            SetCurrentWeapon(0, m_PrevWeapon);
            break;
        case 12:
            SetCurrentWeapon(2, 12);
            break;
        }
    }
    m_PrevWeapon = -1;
}

void XItemHandler_Player::DoLand()
{
    if (IsDead()) {
        KillPlayer();
        return;
    }
    if (((EXItemAnimator_Anim *)m_pItem->GetAnimator(EXRUNTIME_CLASS(EXItemAnimator_Anim)))->Controller(0)->CtrlFlags().BitTest(0x20000)) {
        if (m_HoldState)
            SetupMove(0x16, 1, -1.0f);
        else
            SetupMove(0, 0, -1.0f);
        return;
    }
    if (m_SubMode == 0 && ((EXItemAnimator_Anim *)m_pItem->GetAnimator(EXRUNTIME_CLASS(EXItemAnimator_Anim)))->Controller(0)->Frame() > 0.64f)
        CheckForIdleBreakOut(7);
}

int XItemHandler_Player::GetCardRandom()
{
    if (m_CardIdx == 4) {
        int last = m_Cards[3];
        for (int i = 0; i < 5; i++) {
            int a = g_EXRandClass.Rand32() & 1;
            int b = g_EXRandClass.Rand32() & 1;
            int t = m_Cards[a];
            m_Cards[a] = m_Cards[b + 2];
            m_Cards[b + 2] = t;
        }
        if (m_Cards[0] == last) {
            int r = g_EXRandClass.Rand32() % 3;
            int t = m_Cards[0];
            m_Cards[0] = m_Cards[r + 1];
            m_Cards[r + 1] = t;
        }
        m_CardIdx = 0;
    }
    int i = m_CardIdx++;
    return m_Cards[i];
}

// NEAR (see check): perched on a ledge top (mode 0x12). Jump hops off; button 0x10 hops up to a higher
// ledge above (a grab point 0.1 around m_ClamberPos with flag 0x100 and not 0x400); otherwise the feet are
// kept on the ledge (lower toe 0.03 above it), the stick turns the perch facing and switches to the turn
// moves past 90 degrees, and a run-off ledge (state 0x1000) hops off.
void XItemHandler_Player::DoPerch()
{
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
    HandleGrabAdjustment();
    if (!(m_HandLock > 0.0f) && (m_SubMode == 0 || (m_SubMode > 1 && m_SubMode <= 3))) {
        if (CheckForJump(0)) {
            EXPlacementItem *pItem = m_pItem;
            EXVector rot(pItem->Rotation().x, m_HangYaw, pItem->Rotation().z, pItem->Rotation().w);
            pItem->m_rotation = rot;
            pItem->m_validMtx = 0;
            pPhys->m_physicsFlags &= ~3;
            m_Unk358 = 0.33333334f;
            return;
        }
        if (m_ButtonsPressed & 0x10) {
            EXVector c = m_ClamberPos;
            EXGrabPointTemp g;
            g.Set(c, 0.1f, (EXGrabPointTemp::GRABFITYPE)1);
            theItemEnv.DoGrabPointToMap(g);
            u32 fl = *(u32 *)&g.m_28;
            if (g.m_24 == 1 && (fl & 0x100) && !(fl & 0x400)) {
                pPhys->m_physicsFlags &= ~3;
                pPhys->SetThrustToAttainHeight(3.217f);
                SetupMove(3, 0xb, -1.0f);
                float a = (float)atan2(g.m_Normal.x, g.m_Normal.z);
                m_Unk358 = 0.6666667f;
                m_bLedgeTarget = 1;
                a = a - 1.5707964f + 3.1415927f;
                m_LedgeAngle = a;
                if ((m_StateFlags & 0x40) && (float)fabs(Vec_AngleDifference(m_HangYaw, a)) > 1.5707964f)
                    m_LedgeAngle = a + 3.1415927f;
                SetRotation(m_LedgeTarget, m_LedgeAngle, -1.0f);
                m_LedgeTarget += m_ClamberPos;
                return;
            }
        }
    }
    if (m_bAnimEnded) {
        m_bAnimEnded = 0;
        switch (m_SubMode) {
        case 3:
            m_Speed.SetZero();
        case 1:
        case 2:
            m_ShimmyDir = 1.5f;
        case 0:
            SetupMove(0x12, 0, -1.0f);
            return;
        }
        return;
    }
    EXVector p = m_pItem->Position();
    EXDatumTemp l, r;
    if (!m_pItem->GetAnimatorDatum(0x10000031, l, 1, 0)) memset(&l, 0, sizeof l);
    if (!m_pItem->GetAnimatorDatum(0x10000032, r, 1, 0)) memset(&r, 0, sizeof r);
    const float &toe = r.m_position.y >= l.m_position.y ? l.m_position.y : r.m_position.y;
    p.y -= toe + 0.03f - m_ClamberPos.y;
    m_pItem->m_position = p;
    m_pItem->m_validMtx = 0;
    if (m_SubMode == 3) {
        m_Speed.SetZero();
        m_ShimmyDir = 1.5f;
        SetupMove(0x12, 0, -1.0f);
    } else if (m_SubMode <= 1) {
        if (m_StickMag > 0.5f) {
            if (m_StateFlags & 0x1000) {
                pPhys->m_physicsFlags &= ~3;
                pPhys->SetThrustToAttainHeight(3.217f);
                SetupMove(3, 1, -1.0f);
                m_Unk358 = 0.6666667f;
                return;
            }
            float yaw = m_pItem->Rotation().y;
            float d = Vec_AngleDifference(m_HangYaw, m_PadAngle);
            if (d > 0.0f) {
                float lim = XApp::GameFrameRateMul() * 0.08726646f;
                m_HangYaw += lim >= d ? d : lim;
            } else {
                float lim = XApp::GameFrameRateMul() * -0.08726646f;
                m_HangYaw += d >= lim ? d : lim;
            }
            float a = Vec_AngleDifference(yaw, m_HangYaw);
            if (a < -1.5707964f) {
                m_Unk228 = 1.0f;
                SetupMove(0x12, 2, -1.0f);
            }
            if (a > 1.5707964f) {
                m_Unk228 = 2.0f;
                SetupMove(0x12, 2, -1.0f);
            }
            m_ShimmyDir = (Vec_AngleDifference(yaw, m_HangYaw) + 1.5707964f) * 0.31830987f + 1.0f;
            float sd = m_ShimmyDir;
            m_ShimmyDir = sd >= 2.0f ? 2.0f : (1.0f >= sd ? 1.0f : sd);
        }
    } else if (m_SubMode == 2) {
        EXPlacementItem *pItem = m_pItem;
        EXVector rot = pItem->Rotation();
        float k = XApp::GameFrameRateMul() * 0.039269909f;
        if (m_Unk228 < 1.5f) rot.y -= k;
        else rot.y = k + rot.y;
        pItem->m_rotation = rot;
        pItem->m_validMtx = 0;
    }
}

// NEAR (see check): room on the ledge under a grab point: the segment it lies on must give 0.4 units each
// side of the point (continuing onto neighbouring segments); angle gets the facing onto the ledge.
signed char XItemHandler_Player::CheckLineSegSpace(EXGrabPointTemp &g, float &angle)
{
    EXVector c = g.m_30;
    EXGrabPointArrayTemp arr;
    arr.Set(c, 2.0f, (EXGrabPointArrayTemp::GRABFITYPE)3);
    theItemEnv.DoGrabPointToMap(arr);
    EXVector pos = c;
    EXVector a, b;
    if (!GetSegmentOn(arr, pos, a, b)) return 0;
    EXVector d = a - b;
    float ang = GetRotation(d);
    pos = b;
    float space = 0.4f - (b - c).LengthXZ();
    if (space > 0.0f && !GetSegmentSpace(arr, pos, 1, space, ang, 0.5410521f)) return 0;
    space = 0.4f - (a - c).LengthXZ();
    pos = a;
    if (space > 0.0f && !GetSegmentSpace(arr, pos, 0, space, ang, 0.5410521f)) return 0;
    angle = ang + 3.1415927f;
    return 1;
}

signed char XItemHandler_Player::CheckForShimmy(signed char dir)
{
    float x = m_Pad.x;
    if ((float)fabs(x) < 0.9f) return 0;
    if (x < 0.0f) {
        if (!dir && m_ShimmyDir > 0.5f) return 0;
        dir = 0;
        m_ShimmyDir = 0.0f;
    } else {
        if (!dir && m_ShimmyDir < 0.5f) return 0;
        dir = 1;
        m_ShimmyDir = 1.0f;
    }
    if (!CheckForShimmySpace(dir)) return 0;
    if (m_StateFlags & 0x40)
        SetupMove(0xc, 7, -1.0f);
    else
        SetupMove(0xc, 4, -1.0f);
    return 1;
}

signed char XItemHandler_Player::OpenToAttack(EXVector &from)
{
    if (m_Mode == 0 || m_Mode == 0x17) return 1;
    if (m_Mode == 6) {
        EXMaths::vec_xyzw t;
        const EXVector &p = m_pItem->Position();
        t.x = p.x - from.x;
        t.y = p.y - from.y;
        t.z = p.z - from.z;
        t.w = p.w - from.w;
        EXVector d = (EXMaths::vec_xyzw)t;
        if ((float)fabs(Vec_AngleDifference(m_pItem->Rotation().y, GetRotation(d))) < 1.2217305f) return 1;
    }
    return 0;
}

void XItemHandler_Player::SetFloorType(int type)
{
    switch (type) {
    case 1: m_FloorType = 0; break;
    case 2: m_FloorType = 1; break;
    case 4: m_FloorType = 2; break;
    case 16: m_FloorType = 4; break;
    case 32: m_FloorType = 5; break;
    case 64: m_FloorType = 6; break;
    default: m_FloorType = 3; break;
    }
}

void XItemHandler_Player::LoadUpgradeWeaponGFX(int weapon, int add)
{
    int lvl = limit<int>(GetWeaponUpgrade(weapon) + add, 0, 2);
    if (weapon > 0) {
        unsigned int hash;
        if (weapon > 6) {
            if (weapon != 7) return;
            hash = ShoulderCannonExtendHash[lvl][0];
        } else {
            int i;
            for (i = 0; i < 6; i++)
                if (Weapon_Shapes[i].m_Id == weapon) goto found;
            return;
        found:
            hash = Weapon_Shapes[i].m_Hash[lvl];
        }
        if (hash > 0) {
            unsigned int id = GeoSectionManager::GetSubSection(0x1000013, hash);
            int j;
            for (j = 0; j < m_nWeaponLoads; j++)
                if (WeaponLoadList[j].m_Id == id) return;
            WeaponLoadList[j].m_Id = id;
            m_nWeaponLoads++;
        }
    }
}

EXItem *XItemHandler_Player::GetPlatformItem(int &type)
{
    XItem *pVehicle = XGetCamera(m_PlayerIndex)->GetVehicleTarget();
    if (pVehicle) {
        type = 1;
        return pVehicle;
    }
    int t;
    Actor *pActor = (Actor *)XGetCamera(0)->GetLockOn(t);
    if (t == 1 && pActor) {
        if (pActor->GetProfile()->m_SubGroup == 0x4f000019) {
            EXItem *pItem = pActor->GetItem();
            if (pItem) {
                type = 2;
                return pItem;
            }
        } else if (pActor->m_StateFlags.m_bLyingOnGround) {
            EXItem *pItem = pActor->GetItem();
            type = 3;
            return pItem;
        }
    }
    XItem *pJump = XGetCamera(0)->GetJumpTarget();
    if (pJump) {
        type = 4;
        return pJump;
    }
    type = 0;
    return 0;
}

signed char XItemHandler_Player::CheckForJump(signed char b)
{
    if ((m_PredMsgFlags & 0x20) && (m_ButtonsHeld & 0x20)) {
        XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
        if (m_PredMsgFlags & 0x10) {
            m_JumpBaseY = m_pItem->Position().y;
            m_Unk344 = 12.0f;
        }
        if (m_MSpeedTarget < 0.01f) {
            pPhys->SetThrustToAttainHeight(3.217f);
            SetupMove(3, 0, -1.0f);
        } else if (b) {
            DeadStop(1);
            SetupMove(0x10, 0, -1.0f);
        } else {
            pPhys->SetThrustToAttainHeight(3.217f);
            SetupMove(3, 1, -1.0f);
        }
        DeleteAllSwoosh();
        return 1;
    }
    return 0;
}

void XItemHandler_Player::SendAttackMessage(int weapon, int b)
{
    WeaponProfile *pWeapon = WeaponProfile::GetWeaponData(WeaponProfile::GetWeaponryHashFromPredWeapon(weapon));
    if (pWeapon) {
        for (Actor *pActor = Actor::GetFirst(m_pItem->Position(), pWeapon->m_AttackRadius, 0); pActor;
             pActor = pActor->GetNext(m_pItem->Position(), pWeapon->m_AttackRadius, 0)) {
            EXVector zero;
            zero.SetZero();
            EXVector pos = m_pItem->Position();
            SendMsg_IncomingAttack(pActor, pWeapon->m_Id, b, pos, zero, 0.0f);
        }
    }
}

void XItemHandler_Player::UpdateSectionFiles()
{
    unsigned int mask = 0;
    for (int i = 0; i < 4; i++) mask |= 1 << GetWeaponSection(m_CurrentWeapon[i]);
    EXGeoHeader *pHdr = EXGeoFile::LoadGeoFile(0x1000005, 0, 0)->GeoHeader();
    for (int j = 1; j < 10; j++) {
        if (!(mask & (1 << j))) {
            if (pHdr->IsSectionLoaded(j + 0x8000000, 1) == 3) pHdr->DeLoadSectionFile(j + 0x8000000);
        } else if (pHdr->IsSectionLoaded(j + 0x8000000, 1) != 3)
            pHdr->LoadSectionFile(j + 0x8000000, 1, 0);
    }
}

// NEAR (see check): fires a ranged weapon (5 disc, 6 net/spear, 7 plasma, 8-11 shoulder sets) from the
// right hand (first person: the camera). Aims at the lock-on target when there is one within 22.5
// degrees of the facing in third person (the disc pitches the throw; the shoulder sets lob along
// CalculateAngle), else along the camera or the facing; a first-person shot blocked at the muzzle only
// plays the refusal sound. bThrow drops the weapon forward and up. The start is pulled back to the first
// wall between the chest and the muzzle; uses ammo, plays the fire sound, costs net energy, and spawns
// the weapon's muzzle effect.
void XItemHandler_Player::ShootWeapon(int w, signed char bThrow)
{
    float scale = 1.0f;
    EXDatumTemp d;
    if (!m_pItem->GetAnimatorDatum(0x10000006, d, 1, 0)) return;
    EXVector rot;
    d.m_rotation.GetRotation(rot, 4);
    EXVector s;
    if (XGetCamera(0)->IsFirstPerson()) s = XGetCamera(0)->m_CameraPos;
    else s = d.m_position;
    EXVector dir;
    signed char bTarget = 0;
    if (GetTargetPos(dir, w)) {
        dir.x -= s.x;
        dir.y -= s.y;
        dir.z -= s.z;
        dir.w -= s.w;
        bTarget = 1;
    }
    if (!XGetCamera(0)->IsFirstPerson() && (w == 5 || (w > 7 && w <= 0xb))) {
        if ((float)fabs(Vec_AngleDifference(m_pItem->Rotation().y, (float)atan2(dir.x, dir.z))) > 0.39269909f) bTarget = 0;
    }
    if (bThrow) {
        if (XGetCamera(0)->IsFirstPerson()) {
            float e = XGetCamera(0)->Elevation();
            dir.x = (float)sin(XGetCamera(0)->Angle()) * (float)sin(e);
            e = XGetCamera(0)->Elevation();
            dir.z = (float)cos(XGetCamera(0)->Angle()) * (float)sin(e);
            dir.y = (float)cos(XGetCamera(0)->Elevation());
            dir.w = 0.0f;
        } else {
            float ry = m_pItem->Rotation().y;
            dir.w = 0.0f;
            dir.y = 1.0f;
            scale = 0.6667f;
            dir.x = (float)sin(ry) * 0.4f;
            dir.z = (float)cos(ry) * 0.4f;
        }
    } else if (!bTarget) {
        if (XGetCamera(0)->IsFirstPerson()) {
            float e = XGetCamera(0)->Elevation();
            dir.x = (float)sin(XGetCamera(0)->Angle()) * (float)sin(e);
            e = XGetCamera(0)->Elevation();
            dir.z = (float)cos(XGetCamera(0)->Angle()) * (float)sin(e);
            dir.y = (float)cos(XGetCamera(0)->Elevation());
            dir.w = 0.0f;
            EXRayCastTemp r;
            r.Set(s, dir, 0, 0, 0, 0);
            r.m_mask = 8;
            r.m_bAnyHit = 1;
            theItemEnv.DoRayCastToContoured(r);
            if (r.m_bHit) {
                EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00576, m_pItem->Position(), -1, 0);
                return;
            }
            theItemEnv.DoRayCastToMap(r, 0);
            if (r.m_bHit) {
                EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00576, m_pItem->Position(), -1, 0);
                return;
            }
            if (w == 5) s += dir;
        } else {
            float yaw = m_Mode == 0x12 ? m_HangYaw : m_pItem->Rotation().y;
            switch (w) {
            case 5:
            case 6:
                m_Unk224 = 0.0f;
                dir.w = 0.0f;
                dir.y = 0.0f;
                dir.x = (float)sin(yaw);
                dir.z = (float)cos(yaw);
                break;
            case 8:
            case 9:
            case 10:
            case 11:
                dir.w = 0.0f;
                dir.y = 0.3f;
                dir.x = (float)sin(yaw);
                dir.z = (float)cos(yaw);
                break;
            }
        }
    } else if (w == 5) {
        if (XGetCamera(0)->IsFirstPerson()) {
            float nx = dir.x, ny = dir.y, nz = dir.z;
            float l = dir.y * dir.y + dir.x * dir.x + dir.z * dir.z;
            if (l >= 0.0f && (l = sqrtf(l)) > 1e-5f) {
                l = 1.0f / l;
                nx = l * dir.x;
                ny = l * dir.y;
                nz = l * dir.z;
            }
            s.z = nz + s.z;
            s.x = nx + s.x;
            s.y = ny + s.y;
            s.w = s.w + dir.w;
        }
        float h = dir.x * dir.x + dir.z * dir.z;
        h = h < 0.0f ? 0.0f : sqrtf(h);
        m_Unk224 = (float)atan2(dir.y / h, 1.0f) * 57.29578f;
    } else if (w > 7 && w <= 0xb) {
        float yaw = (float)atan2(dir.x, dir.z);
        float h = dir.x * dir.x + dir.z * dir.z;
        h = h < 0.0f ? 0.0f : sqrtf(h);
        float p = CalculateAngle(0.375f, h, dir.y, 0.01f, 1);
        if (p > 90.0f) p = 5.0f > h ? 0.0f : 0.78539819f;
        float cp = (float)cos(p);
        dir.x = (float)sin(yaw) * cp;
        dir.z = (float)cos(yaw) * cp;
        dir.y = (float)sin(p);
        dir.w = 0.0f;
    }
    switch (w) {
    case 5:
    case 6:
    case 8:
    case 9:
    case 10:
    case 11: {
        EXRayCastTemp r;
        EXVector chest = GetChestPos();
        EXVector v = s - chest;
        r.Set(chest, v, 0, 0, 0, 0);
        r.m_mask = 8;
        theItemEnv.DoRayCastToContoured(r);
        theItemEnv.DoRayCastToMap(r, 0);
        if (r.m_bHit) {
            float t = r.m_t - 0.1f;
            s = v * t + chest;
        }
        break;
    }
    }
    if (m_Mode == 0x18 && !XGetCamera(0)->IsFirstPerson()) {
        EXVector fwd(0.0f, 0.0f, 1.0f, 0.0f);
        dir = EXVector(m_GeckoAngle.MultiplyVector(fwd));
    }
    if (m_Ammo[w] > 0) m_Ammo[w]--;
    if (w >= 8 && w <= 0xb && m_Ammo[w] <= 0) UpdateWeaponGFX(0, 0.0f);
    XItemHandler_Bullet::Create(s, rot, dir, (unsigned short)w, 1, 0, 0, scale);
    m_WeaponUse[w]++;
    Actor::OverridePlayerCloak();
    unsigned int snd;
    switch (w) {
    case 5:
        snd = 0x1af00242;
        break;
    case 6:
        snd = 0x1af00240;
        m_Energy -= 10.0f;
        break;
    case 8:
    case 9:
    case 10:
    case 11:
        snd = 0x1af0058e;
        break;
    default:
        goto done;
    }
    EXBaseSoundManager::GetSoundManager()->StartSound(snd, m_pItem->Position(), -1, 0);
done:
    m_Unk1ec = 0.5f;
    WeaponProfile *pProf = WeaponProfile::GetWeaponData(WeaponProfile::GetWeaponryHashFromPredWeapon(w));
    if (pProf && pProf->m_MuzzleScript)
        XItemHandler_ScriptBase::FX_CreateLink(pProf->m_MuzzleScript, -1, 0x10000006, 0x40, 0x1000016);
}

void XItemHandler_Player::FootStepEvent(signed char bLeft)
{
    if (m_Mode == 6 && m_Unk224 < 1.5f) return;
    int side = bLeft == 0;
    int vol = (int)limit<float>(m_MSpeedTarget * 909.0751953125f, 0.0f, 100.0f);
    EXBaseSoundManager::GetSoundManager()->StartSound(GroundSounds[m_FloorType][side], m_pItem->Position(), vol, 0);
    if (m_Wet && m_MSpeedTarget > 0.02f) {
        EXDatumTemp d;
        if (m_pItem->GetAnimatorDatum(bLeft ? 0x10000031 : 0x10000032, d, 0, 0))
            XItemHandler_ScriptBase::FX_Create(d.m_position, 0x4000516, 0x40, 0x1000016);
    }
}

float QuartAngleDiff(EXQuaternion &a, EXQuaternion &b)
{
    EXQuaternion q1 = a;
    EXQuaternion q2 = b;
    float d = q2.Dot(q1);
    float r;
    if ((float)fabs(d) > 1.0f)
        d = 0.0f;
    else {
        r = EXACos(d);
        d = r * 2.0f;
    }
    return d;
}

// NEAR: register allocation in the inlined Normalise (retail sums z,x,w,y; VC7.1 gives z,w,x,y here).
float SlerpConst(EXQuaternion &a, EXQuaternion &b, EXQuaternion &out, float t)
{
    float angle = QuartAngleDiff(a, b);
    float f;
    if (angle > 0.0f)
        f = 1.0f / angle;
    else
        f = 1.0f;
    t = limit(f * t, 0.0f, 1.0f);
    out = a.slerp(b, t);
    out.Normalise();
    return angle;
}

float InterpStrafeAngle(float a, float b, float t, float p)
{
    float d = b - a;
    if (d < -3.0f)
        a -= 6.0f;
    else if (d > 3.0f)
        a += 6.0f;
    a = Interp(a, b, t, p);
    while (a < -3.0f) a += 3.0f;
    while (a > 3.0f) a -= 3.0f;
    return a;
}

// NEAR (see check): riding on a vehicle's landing point (mode 0x14). The player is pinned to the point
// (PFDeltaMove to it, its rotation copied); jump or the vehicle's death drops him off with a small hop.
void XItemHandler_Player::DoUlysses()
{
    if (m_bAnimEnded) {
        m_bAnimEnded = 0;
        SetupMove(0x14, 0, -1.0f);
        return;
    }
    signed char bOff = 0;
    if (!(m_Health > 0.0f)) bOff = 1;
    Actor *pActor = Actor::GetPtrFromID(m_SpecialActorID);
    if (pActor) {
        Actor::OverridePlayerCloak();
        XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
        EXItem *pItem = pActor->GetItem();
        EXDatumTemp d;
        if (!pItem->GetAnimatorDatum(0x10000060, d, 1, 0)) {
            memset(&d, 0, sizeof d);
            d.m_position = pItem->Position();
        }
        pPhys->m_PFDeltaMove = d.m_position - m_pItem->Position();
        EXVector r;
        d.m_rotation.GetRotation(r, 4);
        m_pItem->m_rotation = r;
        m_pItem->m_validMtx = 0;
        if (m_bAnimEnded) {
            m_bAnimEnded = 0;
            switch (m_SubMode) {
            case 0:
            case 1:
                SetupMove(0x14, 0, -1.0f);
                break;
            case 2:
                bOff = 1;
                break;
            }
        }
        if (m_SubMode == 0 && (m_ButtonsHeld & 8)) SetupMove(0x14, 1, -1.0f);
        if (pActor->m_Health > 0.0f || m_SubMode == 2) goto check;
    }
    bOff = 1;
check:
    if (((m_ButtonsHeld & 0x20) && m_SubMode < 2) || bOff) {
        XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
        pPhys->m_physicsFlags &= ~3;
        m_SpecialActorID = 0;
        m_Unk344 = 0.0f;
        pPhys->SetThrustToAttainHeight(3.217f);
        SetupMove(3, 1, -1.0f);
    }
}

// NEAR (see check): riding on a car roof (mode 0x15). Pinned to the car's landing point, facing 90
// degrees off its heading; jump, death, losing the car or being knocked sideways (a collision response
// pushes him off at 0.047 a frame) drops him off with a hop (or the target jump when one is set).
void XItemHandler_Player::DoCarRide()
{
    signed char bOff = !(m_Health > 0.0f);
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
    if (m_pStandOn) {
        pPhys->m_physicsFlags |= 1;
        XItem *pCar = m_pStandOn;
        EXDatumTemp d;
        if (!pCar->GetAnimatorDatum(0x10000060, d, 1, 0)) {
            memset(&d, 0, sizeof d);
            d.m_position = pCar->Position();
        }
        pPhys->m_PFDeltaMove = d.m_position - m_pItem->Position();
        EXVector r;
        d.m_rotation.GetRotation(r, 4);
        r.y += 1.5707964f;
        m_pItem->m_rotation = r;
        m_pItem->m_validMtx = 0;
        if (m_bAnimEnded) {
            m_bAnimEnded = 0;
            if (m_SubMode == 1) SetupMove(0x15, 0, -1.0f);
        }
        if (m_SubMode == 0 && (m_ButtonsHeld & 8)) SetupMove(0x15, 1, -1.0f);
    } else {
        bOff = 1;
    }
    if (m_ButtonsHeld & 0x20) bOff = 1;
    EXVector cr = pPhys->m_CollisionResponse;
    if (cr.LengthXZ() > 0.001f) {
        m_Speed.w = 0.0f;
        m_Speed.y = 0.0f;
        float a = (float)atan2(cr.x, cr.z);
        m_Speed.x = (float)sin(a) * 0.046875f;
        m_Speed.z = (float)cos(a) * 0.046875f;
        m_Unk2b8 = EXVector(m_Speed.x, 0.0f, m_Speed.z, 0.0f);
    } else if (!bOff) {
        return;
    }
    pPhys->m_physicsFlags &= ~3;
    m_SpecialActorID = 0;
    m_pJustOffCar = m_pStandOn;
    m_pStandOn = 0;
    m_Unk344 = 0.0f;
    pPhys->SetThrustToAttainHeight(3.217f);
    if (m_bJumpTarget)
        SetupMove(3, 3, -1.0f);
    else
        SetupMove(3, 0, -1.0f);
}

signed char XItemHandler_Player::CheckForIdleBreakOut(int flags)
{
    if ((flags & 1) && XGetCamera(m_PlayerIndex)->GetMode() == 1) {
        SetupMove(1, 6, -1.0f);
        return 1;
    }
    if ((flags & 2) && (m_ButtonsPressed & 0x20) && (m_PredMsgFlags & 0x20)) {
        SetupMove(2, 0, -1.0f);
        return 1;
    }
    if (flags & 4) {
        if (m_StickMag != 0.0f) {
            if (m_HoldState)
                SetupMove(0x16, 3, -1.0f);
            else
                SetupMove(1, 0, 21.071007f);
            return 1;
        }
        m_Speed.SetZero();
    }
    return 0;
}

// NEAR: register choice for the reloaded m_SpecialActorID argument (eax/ecx vs ecx/edx).
void XItemHandler_Player::PlaySpecialFXScript(int n)
{
    Actor *pActor = Actor::GetPtrFromID(m_SpecialActorID);
    if (pActor) {
        switch (n) {
        case 0:
            XItemHandler_ScriptBase::FX_CreateLinkBlood(0x40000e2, m_SpecialActorID, 0xe000009, Gore_GetBloodGroup(pActor), 0x40);
            break;
        case 1: {
            XItemHandler_ScriptBase::FX_CreateLinkBlood(0x4000409, m_SpecialActorID, 0xe000003, Gore_GetBloodGroup(pActor), 0x40);
            float h = pActor->m_Health;
            float dmg = h > 41.0f ? 40.0f : h - 1.0f;
            pActor->ChangeHealth(-dmg, 2);
            break;
        }
        case 2:
            XItemHandler_ScriptBase::FX_CreateLinkBlood(0x4000409, m_SpecialActorID, 0xe000003, Gore_GetBloodGroup(pActor), 0x40);
            m_SpecialActorID = 0;
            break;
        }
    }
}

signed char XItemHandler_Player::CheckForActorStomp()
{
    if (((XItemPhysics *)m_pItem->m_pItemPhysics)->m_ImpactSpeed > -0.2f) return 0;
    signed char hit = 0;
    for (Actor *pActor = Actor::GetFirst(m_pItem->Position(), 0.5f, 0); pActor; pActor = pActor->GetNext(m_pItem->Position(), 0.5f, 0)) {
        if (pActor->m_HitCountdown <= 0.0f && (!(pActor->m_Health <= 0.0f) || pActor->m_StateFlags.m_bCanRipBossHead) &&
            !pActor->m_ActorFlags.m_bHolographic && pActor->m_StateFlags.m_bLyingOnGround) {
            hit = 1;
            SendMsg_AttackedByPredator(pActor, 0x4b, EXVector(0.0f, 0.0f, 0.0f, 0.0f), EXVector(0.0f, 0.0f, 0.0f, 0.0f), 0.0f, 0);
        }
    }
    return hit;
}

void XItemHandler_Player::SetupMoveBlend(unsigned int move, unsigned int sub, float blend)
{
    EXItemAnimator_Anim *pAnim = (EXItemAnimator_Anim *)m_pItem->GetAnimator(EXRUNTIME_CLASS(EXItemAnimator_Anim));
    if (m_Unkd4) return;
    if (move == 6 && sub == 0) {
        ResetAttack();
        AdvanceCombo(m_AttackType, 1);
        pAnim->ChangeTo(1, WeaponAttackAnimsBlend[m_AttackType][0], 0x12);
        SetupDepthPtr(&m_Unk224, 1);
        SendAttackMessage(m_AttackType, -1);
        m_WeaponDown = 0.0f;
        m_WeaponDownT[0] = -1.0f;
        m_WeaponDownT[1] = -1.0f;
        m_WeaponDownT[2] = -1.0f;
        m_WeaponDownT[3] = -1.0f;
    }
    m_MoveBlend = blend;
    m_Unk24c = 0.0f;
    pAnim->ControllerStack(1).SetSelectiveSectionBits(m_SectionBits);
    pAnim->ControllerStack(1).m_blend = m_MoveBlend;
}

// NEAR (see check): the grab on a downed drone (CheckForGrabMove): stand 1.2 units from it on the line
// towards the player, facing it, tell it it is being grabbed and play its head-grab animation.
signed char XItemHandler_Player::DoGrabMove(signed char b)
{
    if (!CheckForGrabMove(b)) return 0;
    Actor *pActor = Actor::GetPtrFromID(m_GroundAttackID);
    if (!pActor) return 0;
    EXItem *pThem = pActor->GetItem();
    EXItem *pMe = m_pItem;
    float dx = pThem->Position().x - pMe->Position().x;
    float dz = pThem->Position().z - pMe->Position().z;
    float a = (float)atan2(dx, dz) + 3.1415927f;
    float s = (float)sin(a);
    float c = (float)cos(a);
    m_TargetAdjust.z = pThem->Position().z + c * 1.2f;
    m_TargetAdjust.x = pThem->Position().x + s * 1.2f;
    m_TargetAdjust.y = pThem->Position().y;
    m_TargetAdjust.w = pThem->Position().w;
    m_TargetAdjustTime = a + 3.1415927f;
    m_bTargetAdjust = 1;
    m_TargetAdjustType = 8.0f;
    m_SpecialActorID = m_GroundAttackID;
    m_bUnkc40 = 0;
    SetupMove(0x16, 0, -1.0f);
    EXVector r1(0.0f, a, 0.0f, 0.0f);
    EXVector r2(0.0f, a, 0.0f, 0.0f);
    SendMsg_AttackedByPredator(Actor::GetPtrFromID(m_SpecialActorID), 0x47, r1, r2, 0.0f, 0);
    pActor->SetAnimType(0x900024b, 0, 1);
    m_CurrentSlot = -1;
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
    m_Speed.SetZero();
    m_Unk2b8.SetZero();
    pPhys->m_DeltaMove.SetZero();
    pPhys->m_Accel.y = 0.0f;
    EXPlacementItem *pItem = m_pItem;
    EXVector rot(0.0f, pItem->Rotation().y, 0.0f, 0.0f);
    pItem->m_rotation = rot;
    pItem->m_validMtx = 0;
    m_SpecialMoveAvailable = 0;
    return 1;
}

void XItemHandler_Player::SetCloak(signed char bOn)
{
    if (bOn) {
        if (0.0f >= m_Energy) return;
        if (m_SparkTimer > 0.0f) return;
        if (m_CloakState) return;
        EXBaseSoundManager::GetSoundManager()->StartSound(0x1af001c4, m_pItem->Position(), -1, 0);
        SparkSkin(1.0f);
        m_CloakState = 1;
        return;
    }
    switch (m_CloakState) {
    case 2:
        EXBaseSoundManager::GetSoundManager()->StartSound(0x1af001c3, m_pItem->Position(), -1, 0);
        SparkSkin(1.0f);
        m_CloakState = 3;
        break;
    case 4:
        EXBaseSoundManager::GetSoundManager()->StartSound(0x1af001c5, m_pItem->Position(), -1, 0);
        SparkSkin(10.0f);
        m_CloakState = 0;
        break;
    }
}

// NEAR: in the first SetHasRender retail sets up ecx = 1 before the m_validMtx store (scheduling).
void XItemHandler_Player::InitPlasmaCasterGFX()
{
    int lvl = m_Unkcf0[7];
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(0x1000013, 0, 0);
    ((EXItemAnimator *)m_pWeaponItem[8])->Initialise(pFile, ShoulderCannonExtendHash[lvl][0]);
    ((EXItemAnimator *)m_pWeaponItem[9])->Initialise(pFile, ShoulderCannonExtendHash[lvl][1]);
    ((EXItemAnimator *)m_pWeaponItem[10])->Initialise(pFile, ShoulderCannonExtendHash[lvl][2]);
    ((EXItemAnimator *)m_pWeaponItem[11])->Initialise(pFile, 0x2000068);
    switch (gCurrentMap_GeoHash) {
    case 0x1000056:
    case 0x100006a:
    case 0x1000070:
    case 0x1000136:
    case 0x100014c:
    case 0x100016f:
        ((EXItemAnimator *)m_pWeaponItem[8])->m_itemFlags.BitClr(1);
        ((EXItemAnimator *)m_pWeaponItem[9])->m_itemFlags.BitClr(1);
        ((EXItemAnimator *)m_pWeaponItem[10])->m_itemFlags.BitClr(1);
        ((EXItemAnimator *)m_pWeaponItem[11])->m_itemFlags.BitClr(1);
        break;
    default:
        ((EXItemAnimator *)m_pWeaponItem[8])->SetHasRender();
        ((EXItemAnimator *)m_pWeaponItem[9])->SetHasRender();
        ((EXItemAnimator *)m_pWeaponItem[10])->SetHasRender();
        ((EXItemAnimator *)m_pWeaponItem[11])->SetHasRender();
        break;
    }
}

signed char XItemHandler_Player::CheckForStop(int hold)
{
    if (m_StickMag == 0.0f) {
        const EXVector &r = m_pItem->Rotation();
        m_pItem->SetRotation(EXQuaternion(r.x, r.y, 0.0f, r.w));
        if (m_MSpeedTarget < 0.18f) {
            switch (hold) {
            case 0: SetupMove(0, 0, -1.0f); break;
            case 1: SetupMove(0x16, 1, -1.0f); break;
            case 2: SetupMove(1, 6, -1.0f); break;
            }
        } else {
            switch (hold) {
            case 0:
                if (m_ShimmyDir < 1.5f)
                    SetupMove(0, 9, -1.0f);
                else
                    SetupMove(0, 10, -1.0f);
                break;
            case 1: SetupMove(0x16, 2, -1.0f); break;
            case 2: SetupMove(1, 7, -1.0f); break;
            }
        }
        return 1;
    }
    return 0;
}

signed char XItemHandler_Player::CheckForFall()
{
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
    if (pPhys->CollisionFlags() & 1) {
        m_FallTimer = 0.0f;
        m_FloorNormal = pPhys->GetFloorNorm();
        if (m_FloorNormal.y <= 0.8f) m_FloorNormal = EXVector(0.0f, 1.0f, 0.0f, 0.0f);
        return 0;
    }
    m_FallTimer += XApp::FrameLength();
    if (m_FallTimer > 0.083333336f) {
        m_FloorNormal = EXVector(0.0f, 1.0f, 0.0f, 0.0f);
        DeleteAllSwoosh();
        m_bTargetAdjust = 0;
        SetupMove(9, 0, -1.0f);
        return 1;
    }
    return 0;
}

float InterpAngle(float a, float b, float t, float p)
{
    float d = b - a;
    if (d < -3.1415927f)
        a -= 6.2831855f;
    else if (d > 3.1415927f)
        a += 6.2831855f;
    a = Interp(a, b, t, p);
    return EXMakeAngleBiPolar(a);
}

void XItemHandler_Player::SendVMMessages(int type)
{
    unsigned int hash = 0;
    if (type == 0x3f) {
        int t;
        Actor *pActor = (Actor *)XGetCamera(0)->GetLockOn(t);
        if (t == 1) {
            hash = pActor->RemapVocalMimicrySoundHash(0);
            if (!hash) {
                switch (gCurrentMap_GeoHash) {
                case 0x1000056:
                case 0x1000070:
                case 0x1000170: hash = 0x1af006eb; break;
                case 0x100006a: hash = 0x1af006e0; break;
                case 0x100016f: hash = 0x1af006ec; break;
                default: {
                    int n = XGetGameValues()->CountCompletedChapters();
                    if (n >= 27) n = 26;
                    hash = VM_Hash[n];
                    break;
                }
                }
            }
            SendMsg_Attract(pActor, m_pItem->Position());
        } else if (t == 7) {
            hash = ((XItemHandler_Mimicry *)((XItem *)pActor)->ItemHandler())->Play();
        } else {
            PlayNegativeSound(0);
            return;
        }
        ActorOptionsManager::MimicryUsed();
    }
    m_VMSound = hash;
    m_SoundTag.StartSound(hash);
    FEHud *pHud = FEHud::Instance(1);
    pHud->m_Unkd20 = 59999940.0f;
    pHud->m_Unkd24 = 0;
}

void XItemHandler_Player::UpdateVisionMode()
{
    float step = XApp::GameFrameRateMul() * 0.1f;
    float target;
    if (XGetCamera(0)->GetMode() <= 1) {
        if (XGetCamera(0)->GetDistance() > 1.95f || (m_bCloak && !m_Tint))
            target = 1.0f;
        else
            target = 0.05f;
    } else
        target = 1.0f;
    m_VisionFade += limit<float>(target - m_VisionFade, -step, step);
    if (m_VisionFade >= 1.0f) {
        m_pAnim->RenderInfo().RenderFlagsRef().BitClr(2);
        m_bVisionFade = 0;
    } else {
        m_pAnim->RenderInfo().RenderFlagsRef().BitSet(2);
        m_pAnim->RenderInfo().SetRGBAMult(1.0f, 1.0f, 1.0f, m_VisionFade);
        m_bVisionFade = 1;
    }
    SetWatcherEffects(m_Tint);
}

void XItemHandler_Player::LoadWeaponsAsync()
{
    if (m_bWeaponsLoaded) return;
    signed char bAll = 1;
    for (int i = 0; i < m_nWeaponLoads; i++) {
        EXGeoHeader *pHdr = EXGeoFile::LoadGeoFile(0x1000013, 0, 0)->GeoHeader();
        unsigned int sub = GeoSectionManager::GetSubSection(0x1000013, WeaponLoadList[i].m_Id);
        switch (pHdr->IsSectionLoaded(sub, 1)) {
        case 0:
        case 5:
            pHdr->LoadSectionFile(sub, 1, 0);
        case 1:
        case 2:
        case 4:
            bAll = 0;
            break;
        }
    }
    if (bAll) {
        m_bWeaponsLoaded = 1;
        InitWristBladeGFX();
        InitPlasmaCasterGFX();
        UpdateWeaponGFX(0, 0.0f);
        UpdateWeaponGFX(1, 0.0f);
        UpdateWeaponGFX(2, 0.0f);
        UpdateWeaponGFX(3, 0.0f);
    }
}

void XItemHandler_Player::KillPlayer()
{
    if (XGetCamera(0)->GetMode() == 2) XGetCamera(0)->SetThirdPerson();
    m_ButtonsPressed = 0;
    m_ButtonsHeld = 0;
    m_ButtonsReleased = 0;
    m_StickMag = 0.0f;
    m_Pad.SetZero();
    m_pItem->m_pItemPhysics->SetPhysicsFlagsOff(3);
    switch (m_Mode) {
    case 3:
    case 9:
    case 10:
        break;
    case 4:
    case 5:
    case 12:
    case 17:
    case 19:
    case 20:
        SetupMove(9, 0, -1.0f);
        m_Unk358 = 60.0f;
        break;
    case 0x17:
        switch (m_SubMode) {
        case 9:
            break;
        case 10:
            break;
        default:
            return;
        }
    default:
        SafeBreakoutAll(0);
        SetCloak(0);
        SetupMove(0x17, 8, -1.0f);
        break;
    case 0x18:
        DoGeckoJumpOff(0);
        break;
    }
}

signed char XItemHandler_Player::MoveCheckForSticky()
{
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
    if (pPhys->m_DeltaMove.y > 0.001f) return 0;
    EXVector from;
    EXVector dir;
    EXRayCastTemp rc;
    from = m_pItem->Position();
    from.y += 0.1f;
    dir = EXVector(0.0f, -1.0f, 0.0f, 0.0f);
    rc.Set(from, dir, 0, 0, 0, 0);
    rc.m_flags = 0x40;
    theItemEnv.DoRayCastToContoured(rc);
    theItemEnv.DoRayCastToMap(rc, 0);
    if (!rc.m_bHit) return 0;
    pPhys->AddPFDeltaMove(EXVector(0.0f, -0.1f - rc.m_t, 0.0f, 0.0f));
    return 1;
}

signed char XItemHandler_Player::CheckPerchLand(EXGrabPointTemp &t)
{
    if (!(t.m_28 & 0x100)) return 0;
    if ((t.m_30 - m_pItem->Position()).LengthXZ() > 0.5f) return 0;
    if (fabs(t.m_30.y - m_pItem->Position().y) > XApp::GameFrameRateMul() * 0.3f) return 0;
    return 1;
}

signed char XItemHandler_Player::CheckPerchPlaneLand(EXGrabPointTemp &t)
{
    if (t.m_24 != 2) return 0;
    if (!(t.m_28 & 0x100)) return 0;
    const EXVector &p = m_pItem->Position();
    EXMaths::vec_xyzw r;
    r.x = t.m_30.x - p.x;
    r.y = t.m_30.y - p.y;
    r.z = t.m_30.z - p.z;
    r.w = t.m_30.w - p.w;
    EXMaths::vec_xyzw r2 = r;
    EXVector e(r2);
    EXMaths::vec_xyzw w = e;
    if (EXSqrt(w.x * w.x + w.z * w.z) > 0.5f) return 0;
    if (fabs(t.m_30.y - m_pItem->Position().y) > XApp::GameFrameRateMul() * 0.3f) return 0;
    return 1;
}

void XItemHandler_Player::DoMove()
{
    if ((m_ButtonsPressed & 0x10) && CheckForFlipGrab()) return;
    if (m_SubMode == 2 && m_bAnimEnded) SetupMove(1, 0, -1.0f);
    if (m_ButtonsPressed & 0x80) {
        const EXVector &r = m_pItem->Rotation();
        m_pItem->SetRotation(EXQuaternion(r.x, r.y, 0.0f, r.w));
        MoveStrafe();
        CheckForGrabMove(1);
        CheckForStealthKill(1);
        CheckForGroundAttack();
        CheckForFinishingMove(1);
        return;
    }
    if (CheckForStop(0)) return;
    if (m_Unk214 == 1 || m_SubMode == 0 || m_SubMode == 2) {
        MoveNormal(0);
        return;
    }
    float blend = -1.0f;
    if (m_Mode == 1 && (m_SubMode == 3 || m_SubMode == 4)) {
        EXItemAnimator_Anim *pAnim = (EXItemAnimator_Anim *)m_pItem->GetAnimator(EXRUNTIME_CLASS(EXItemAnimator_Anim));
        blend = pAnim->Controller(0)->Frame() * 34.0f + 1.0f;
    }
    SetupMove(1, 1, blend);
    MoveNormal(0);
}

// NEAR: 6 bytes, retail schedules the d.w copy load before the fxch of the fabs compare.
signed char XItemHandler_Player::CheckForActorSpace(EXVector &pos, float dist, float radius)
{
    float a = m_pItem->Rotation().y;
    EXVector p;
    p.x = (float)sin(a);
    p.z = (float)cos(a);
    p.x *= dist;
    p.z *= dist;
    p.x = pos.x + p.x;
    p.y = pos.y;
    p.z = pos.z + p.z;
    p.w = pos.w;
    for (Actor *pActor = Actor::GetFirst(p, radius, 0); pActor; pActor = pActor->GetNext(p, radius, 0)) {
        if (pActor->CanCollideWithPlayer() || pActor->m_StateFlags.m_bCanRipBossHead) {
            EXVector d = p - pActor->GetFeetPos();
            if (!((float)fabs(d.y) > 0.2f)) return 0;
        }
    }
    return 1;
}

void XItemHandler_Player::DoSkid()
{
    m_Speed.SetZero();
    if (CheckForFall()) return;
    if (CheckForJump(0)) return;
    EXQuaternion q = m_pItem->Rotation();
    q.y += m_SkidTurn * XApp::GameFrameRateMul();
    m_pItem->SetRotation(q);
    if (!AnimEnded()) return;
    m_Speed.x = sin(m_pItem->Rotation().y);
    m_Speed.z = cos(m_pItem->Rotation().y);
    m_MSpeed = 0.187496f;
    if (m_HoldState) {
        switch (m_SubMode) {
        case 0: SetupMove(0x16, 3, 19.009838f); break;
        case 1: SetupMove(0x16, 3, 2.32059f); break;
        }
        DoGrab();
    } else {
        switch (m_SubMode) {
        case 0: SetupMove(1, 1, 19.009838f); break;
        case 1: SetupMove(1, 1, 2.32059f); break;
        }
        DoMove();
    }
}

// NEAR (see check): thrown by an explosion at pos with strength f (clamped 0.1-1): breaks out of every
// move, levels the player and pushes him away from it (up by the blast direction's height, out by its
// spread), then plays the blown-back (more than 90 degrees from his facing) or blown-forward reaction.
// Note retail tests m_SubMode (not m_Mode) against 0x17 to pick the second variants.
void XItemHandler_Player::ReactExplode(EXVector &pos, float f)
{
    if (m_MoveFlags & 0x800) return;
    if (m_ReactTimer > 0.0f) return;
    if (!(m_Health > 0.0f)) return;
    if (m_Mode == 0x17 && m_SubMode < 8) return;
    m_Unk358 = 60.0f;
    m_Unk35c = 60.0f;
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
    m_bLedgeTarget = 0;
    m_bJumpTarget = 0;
    SafeBreakoutAll(0);
    RestorePreviousWeapon();
    m_pItem->SetRotation(EXQuaternion(0.0f, m_pItem->Rotation().y, 0.0f, 0.0f));
    pPhys->m_physicsFlags &= ~3;
    ResetAttack();
    f = f < 1.0f ? (f > 0.1f ? f : 0.1f) : 1.0f;
    EXVector d = GetPelvisPos() - pos;
    float a = (float)atan2(d.x, d.z);
    EXMaths::vec_xyzw n = d;
    n.w = 0.0f;
    float l = n.y * n.y + n.z * n.z + n.x * n.x;
    if (!(l < 0.0f)) {
        l = (float)sqrt(l);
        if (l > 1e-5f) {
            float inv = 1.0f / l;
            n.x = inv * n.x; n.y = inv * n.y; n.z = inv * n.z; n.w = inv * n.w;
        }
    }
    float up = n.y < 1.0f ? (n.y > 0.0f ? n.y : 0.0f) : 1.0f;
    pPhys->SetThrustToAttainHeight(up * f * 4.862f);
    float h = n.z * n.z + n.x * n.x;
    h = h < 0.0f ? 0.0f : (float)sqrt(h);
    h = h < 1.0f ? (h > 0.0f ? h : 0.0f) : 1.0f;
    float k = h * f * 0.09375f;
    m_Unk2b8.w = 0.0f;
    m_Unk2b8.y = 0.0f;
    m_Unk2b8.x = (float)sin(a) * k;
    m_Unk2b8.z = (float)cos(a) * k;
    pPhys->m_DeltaMove += m_Unk2b8;
    m_Speed = m_Unk2b8;
    if ((float)fabs(Vec_AngleDifference(m_pItem->Rotation().y, a)) > 1.5707964f) {
        if (m_SubMode == 0x17) SetupMove(0x17, 1, -1.0f);
        else SetupMove(0x17, 0, -1.0f);
    } else {
        if (m_SubMode == 0x17) SetupMove(0x17, 5, -1.0f);
        else SetupMove(0x17, 4, -1.0f);
    }
}

void XItemHandler_Player::TeleportPlayer(EXVector &pos, float angle)
{
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
    pPhys->SetPhysicsFlagsOff(3);
    DeadStop(1);
    SafeBreakoutAll(2);
    m_pItem->SetPosition(pos);
    pPhys->SetPreviousPosition(pos);
    SetupMove(0, 0, -1.0f);
    m_pItem->SetMapOn(theItemEnv.GetMapOn(m_pItem->Position()));
    if (angle < 100.0f) {
        EXQuaternion q = m_pItem->Rotation();
        q.y = angle;
        m_pItem->SetRotation(q);
    }
}

// NEAR: retail lays out the Position() fallback inline and the averaged datums last (block order).
EXVector XItemHandler_Player::GetMidFeetPos()
{
    EXDatumTemp a;
    EXDatumTemp b;
    if (m_pItem->GetAnimatorDatum(0x10000031, a, 0, 0) && m_pItem->GetAnimatorDatum(0x10000032, b, 0, 0))
        return (a.m_position + b.m_position) * 0.5f;
    return m_pItem->Position();
}

void XItemHandler_Player::PlayerPostUpdate()
{
    UpdateDroneToGrabHand();
    if (Actor::GetActorIDPlayerHasHead()) {
        Actor *pActor = Actor::GetPtrFromID(Actor::GetActorIDPlayerHasHead());
        if (pActor) {
            EXDatumTemp d;
            if (m_pItem->GetAnimatorDatum(0x10000006, d, 0, 0)) {
                EXVector rot;
                d.m_rotation.GetRotation(rot, 4);
                pActor->SetRippedHeadPosRot(d.m_position, EXQuaternion(rot.x, rot.y, rot.z, rot.w));
            }
        }
    }
    if (m_pHeadItem) {
        if (XGetCamera(0)->IsFirstPerson()) {
            m_pHeadItem->m_itemFlags.BitClr(1);
            return;
        }
        m_pHeadItem->SetHasRender();
        EXDatumTemp d;
        if (m_pItem->GetAnimatorDatum(0x10000037, d, 0, 0)) {
            m_pHeadItem->SetPosition(d.m_position);
            EXVector rot;
            d.m_rotation.GetRotation(rot, 4);
            m_pHeadItem->SetRotation(EXQuaternion(rot.x, rot.y, rot.z, rot.w));
        }
    }
}

void XItemHandler_Player::DoGeckoJumpOff(signed char b)
{
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
    pPhys->SetPhysicsFlagsOff(3);
    pPhys->SetThrust(0.0f);
    DeadStop(0);
    float lim = b ? 0.1f : -0.1f;
    float t;
    if (lim > m_pItem->Rotation().x) {
        SetupMove(9, 0, -1.0f);
        t = 1.0f;
    } else {
        m_Unk308 = 1.0f - m_pItem->Rotation().x * 0.63661975f;
        m_bHandLockFlag = 0;
        m_LockHandIdx = 15;
        GetHandLockPos(15, m_HandLockPos);
        m_HandLock = 6.0f;
        SetupMove(10, 2, -1.0f);
        t = 0.33333334f;
    }
    m_Unk358 = t;
    m_Unk35c = 1.0f;
    EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00366, m_pItem->Position(), -1, 0);
}

void XItemHandler_Player::SetDefaultAmmo()
{
    for (int i = 0; i < 14; i++) m_Ammo[i] = (short)GetMaxAmmo(i);
    SetAmmo(11, 0);
    SetAmmo(10, 0);
    SetAmmo(9, 0);
    SetAmmo(8, 0);
    m_Ammo[5] /= 2;
    switch (gCurrentMap_GeoHash) {
    case 0x1000118:
        m_Ammo[12] = 2;
        break;
    case 0x10000b4:
        m_Ammo[12] = 0;
    case 0x100006a:
    case 0x1000125:
        m_Ammo[5] = 0;
        break;
    }
}

void XItemHandler_Player::UpdateWeaponGFX(int slot, float)
{
    signed char bShow = 1;
    int weapon = m_CurrentWeapon[slot];
    if (weapon >= 14) return;
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(0x1000013, 0, 0);
    switch (weapon) {
    case 6:
        if (!m_bDiskInHand) bShow = 0;
        break;
    case 12:
        if (m_Mode != 0x17) return;
        if (m_SubMode == 9) break;
        return;
    case 8:
    case 9:
    case 10:
    case 11:
        if (m_Ammo[weapon] <= 0) bShow = 0;
        break;
    case 7:
        return;
    }
    int item = WeaponHash[weapon][1];
    if (!bShow) {
        ((EXItemAnimator *)m_pWeaponItem[item])->m_itemFlags.BitClr(1);
        return;
    }
    unsigned int hash = WeaponHash[weapon][0];
    if (hash) {
        ((EXItemAnimator *)m_pWeaponItem[item])->Initialise(pFile, hash);
        if (HasMoveFlag(0x9000))
            ((EXItemAnimator *)m_pWeaponItem[item])->m_itemFlags.BitClr(1);
        else
            ((EXItemAnimator *)m_pWeaponItem[item])->SetHasRender();
        if (weapon != 1) {
            for (int i = 0; i < 6; i++) {
                if (Weapon_Shapes[i].m_Id == weapon) {
                    float f = (float)Weapon_Shapes[i].m_VertexSet * Weapon_Shapes[i].m_VertexScale;
                    int n = (int)f;
                    ((EXItemAnimator *)m_pWeaponItem[item])->RenderInfo().RenderFlagsRef().BitSet(0x1000);
                    ((EXItemAnimator *)m_pWeaponItem[item])->RenderInfo().SetVertexSet((short)n, (short)(n + 1), f - (float)n);
                }
            }
        }
        m_HandWeapon[item] = weapon;
        SetHandWeaponPosRot(item);
    } else {
        ((EXItemAnimator *)m_pWeaponItem[item])->m_itemFlags.BitClr(1);
        m_HandWeapon[item] = -1;
    }
}

// NEAR (see check): picks the attack variant. bTurn: the attack direction (the locked-on target, else
// the stick) is more than 10 degrees off the facing, or there is room for a finishing move while
// pushing the stick or more than 1 from the target; bBehind: a combo attack (types 2-4) at a target
// more than 90 degrees round. A target in front scales the lunge by two thirds of its distance (1-3).
void XItemHandler_Player::CalculateHitType(int, signed char &bTurn, signed char &bBehind)
{
    bBehind = 0;
    bTurn = 0;
    XItemHandler_Camera *pCam = XGetCamera(m_PlayerIndex);
    Actor *pTarget = pCam->GetTargetID() > 0 ? Actor::GetPtrFromID(pCam->GetTargetID()) : 0;
    float facing = 0.0f;
    float a;
    if (pTarget) {
        EXItem *pMe = m_pItem;
        EXVector d = pTarget->GetItem()->Position() - pMe->Position();
        facing = Vec_AngleDifference(pMe->Rotation().y, (float)atan2(d.x, d.z));
        EXVector d2 = pTarget->GetItem()->Position() - pMe->Position();
        a = (float)atan2(d2.x, d2.z);
    } else {
        a = m_PadAngle;
    }
    float diff = Vec_AngleDifference(m_pItem->Rotation().y, a);
    if ((float)fabs(diff) > 0.17453292f) bTurn = 1;
    if (m_AttackType > 1 && m_AttackType < 5 && pTarget && (float)fabs(diff) > 1.5707964f) {
        bBehind = 1;
        bTurn = 1;
    }
    m_HitMoveScale = 1.0f;
    if (pTarget) {
        EXVector d = pTarget->GetItem()->Position() - m_pItem->Position();
        float dist = d.LengthXZ();
        if ((float)fabs(facing) < 1.5707964f) {
            float s = dist * 0.6666667f;
            m_HitMoveScale = s < 3.0f ? (s > 1.0f ? s : 1.0f) : 3.0f;
        }
        if (dist > 1.0f) goto space;
    }
    if (!(m_StickMag > 0.95f)) return;
space:
    if (CheckForFinishMoveSpace(0, 0.7f, 0.8f, 0.55f, 0.05f)) {
        EXVector p = m_pItem->Position();
        if (CheckForActorSpace(p, 0.7f, 0.8f)) bTurn = 1;
    }
}

// NEAR: the target lookup lands in eax with a fallthrough null (retail: ecx, shared xor block), and the
// position subtraction registers differ (the vec_xyzw field-subtract form fixed the inlining).
void XItemHandler_Player::CalculateHitMove(int)
{
    Actor *pTarget;
    int id = XGetCamera(m_PlayerIndex)->GetTargetID();
    if (id > 0 && (pTarget = Actor::GetPtrFromID(id)) != 0) {
    } else
        pTarget = 0;
    if (m_Unk224 > 1.5f && !m_bAttacking) {
        float ang;
        if (pTarget) {
            const EXVector &q0 = m_pItem->Position();
            EXMaths::vec_xyzw t;
            t.x = pTarget->GetFeetPos().x - q0.x;
            t.y = pTarget->GetFeetPos().y - q0.y;
            t.z = pTarget->GetFeetPos().z - q0.z;
            t.w = pTarget->GetFeetPos().w - q0.w;
            EXVector d = (EXMaths::vec_xyzw)t;
            ang = GetRotation(d);
            if (m_AttackState > 3) ang += 3.1415927f;
        } else
            ang = m_PadAngle;
        float diff = Vec_AngleDifference(m_pItem->Rotation().y, ang);
        float lim = XApp::GameFrameRateMul() * 0.17453292f;
        EXQuaternion q = m_pItem->Rotation();
        q.y += limit<float>(diff, -lim, lim);
        m_pItem->SetRotation(q);
    }
}

// NEAR (see check): fits the gecko pose to the wall. Optionally first rolls the pose by roll about z and
// moves the point up it by move a frame. Three spheres (0.75) around the body (shoulders at +-0.75, 1.5
// up; feet 1.25 down) are pushed out of the walls; when all three touch (returns 7) their plane gives the
// wall (plane and m_GeckoNormal) and the new pose (q) faces along it, and with bMove the point and the
// hand lock are pushed back out by the deepest penetration past 0.25.
int XItemHandler_Player::GetGeckoFormQuart(EXQuaternion &q, EXPlane &plane, signed char bMove, float move, float roll)
{
    EXQuaternion g = m_GeckoAngle;
    EXVector P = m_GeckoPoint;
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
    if (bMove) {
        EXVector ra(0.0f, 0.0f, roll, 0.0f);
        EXMaths::quat_xyzw r;
        r.SetRotation(ra, 4);
        float gx = g.x, gy = g.y, gz = g.z;
        g.x = g.w * r.x + r.z * gy + r.w * gx - r.y * gz;
        g.y = gz * r.x + r.y * g.w + r.w * gy - r.z * gx;
        g.z = r.z * g.w + r.w * gz + r.y * gx - gy * r.x;
        g.w = r.w * g.w - r.x * gx - r.y * gy - r.z * gz;
        EXVector off(0.0f, XApp::GameFrameRateMul() * move, 0.0f, 0.0f);
        P += EXVector(g.MultiplyVector(off));
    }
    EXVector upv(0.0f, 1.5f, 0.0f, 0.0f);
    EXVector probe[3];
    probe[0] = EXVector(-0.75f, 1.5f, 0.0f, 0.0f);
    probe[1] = EXVector(0.75f, 1.5f, 0.0f, 0.0f);
    probe[2] = EXVector(0.0f, -1.25f, 0.0f, 0.0f);
    int mask = 0;
    EXVector U = EXVector(g.MultiplyVector(upv));
    for (int i = 0; i < 3; i++) {
        probe[i] = EXVector(g.MultiplyVector(probe[i]));
        probe[i] += P;
    }
    EXVector orig[3];
    EXSphere sph;
    EXVector n;
    orig[0] = probe[0];
    sph.Set(probe[0], 0.75f);
    if (pPhys->CollideSphereClose(sph, probe[0], n)) mask = 1;
    orig[1] = probe[1];
    sph.Set(probe[1], 0.75f);
    if (pPhys->CollideSphereClose(sph, probe[1], n)) mask |= 2;
    orig[2] = probe[2];
    sph.Set(probe[2], 0.75f);
    if (pPhys->CollideSphereClose(sph, probe[2], n)) mask |= 4;
    if (mask != 7) return mask;

    float nx = (probe[1].y - probe[0].y) * (probe[2].z - probe[1].z) - (probe[1].z - probe[0].z) * (probe[2].y - probe[1].y);
    float ny = (probe[1].z - probe[0].z) * (probe[2].x - probe[1].x) - (probe[2].z - probe[1].z) * (probe[1].x - probe[0].x);
    float nz = (probe[2].y - probe[1].y) * (probe[1].x - probe[0].x) - (probe[1].y - probe[0].y) * (probe[2].x - probe[1].x);
    float len = nx * nx + ny * ny + nz * nz;
    len = len < 0.0f ? 0.0f : sqrtf(len);
    float A, B, C, D;
    if ((float)fabs(len) != 0.0f) {
        float inv = 1.0f / len;
        B = ny * inv;
        C = nz * inv;
        A = inv * nx;
        D = 0.0f - (A * probe[0].x + C * probe[0].z + B * probe[0].y);
    } else {
        B = 0.0f;
        C = 0.0f;
        D = 0.0f;
        A = 1.0f;
    }
    EXVector T = U + P;
    float dist = C * T.z + B * T.y + A * T.x + T.w * 0.0f + D;
    EXVector f(T.x - A * dist - P.x, T.y - B * dist - P.y, T.z - C * dist - P.z, T.w - dist * 0.0f - P.w);
    EXVector nv(A, B, C, 0.0f);
    EXMatrix m;
    m.AlignToDir(f, nv, nv);
    EXQuaternion Q;
    Q.SetRotation(m);
    EXVector xa(1.5707964f, 0.0f, 0.0f, 0.0f);
    EXMaths::quat_xyzw R;
    R.SetRotation(xa, 4);
    q.z = R.y * Q.x + Q.z * R.w + R.z * Q.w - Q.y * R.x;
    q.w = R.w * Q.w - R.x * Q.x - R.y * Q.y - R.z * Q.z;
    q.y = Q.z * R.x + Q.y * R.w + R.y * Q.w - R.z * Q.x;
    q.x = Q.w * R.x + R.w * Q.x + R.z * Q.y - Q.z * R.y;
    plane.A = A;
    plane.B = B;
    plane.C = C;
    plane.D = D;
    m_GeckoNormal.x = A;
    m_GeckoNormal.y = B;
    m_GeckoNormal.z = C;
    m_GeckoNormal.w = D;
    if (bMove) {
        EXVector sum(0.0f, 0.0f, 0.0f, 0.0f);
        float maxPen = 0.0f;
        for (int i = 0; i < 3; i++) {
            EXVector &d = orig[i];
            d -= probe[i];
            float l = d.x * d.x + d.z * d.z + d.y * d.y;
            if (l >= 0.0f && sqrtf(l) > 0.25f) {
                float pen = sqrtf(l) - 0.25f;
                if (maxPen < pen) maxPen = pen;
                float l4 = d.x * d.x + d.z * d.z + d.w * d.w + d.y * d.y;
                if (l4 >= 0.0f && (l4 = sqrtf(l4)) > 1e-5f) {
                    l4 = 1.0f / l4;
                    d.x = l4 * d.x;
                    d.y = l4 * d.y;
                    d.z = l4 * d.z;
                    d.w = l4 * d.w;
                }
                d.x *= pen;
                d.y *= pen;
                d.z = pen * d.z;
                d.w *= pen;
                sum.x += d.x;
                sum.y += d.y;
                sum.z += d.z;
                sum.w += d.w;
            }
        }
        float l = sum.x * sum.x + sum.y * sum.y + sum.z * sum.z + sum.w * sum.w;
        if (l >= 0.0f && (l = sqrtf(l)) > 1e-5f) {
            l = 1.0f / l;
            sum.x = l * sum.x;
            sum.y *= l;
            sum.z *= l;
            sum.w *= l;
        }
        m_HandLockPos.y -= sum.y * maxPen;
        m_HandLockPos.z -= sum.z * maxPen;
        m_HandLockPos.x -= sum.x * maxPen;
        m_HandLockPos.w -= sum.w * maxPen;
        m_GeckoPoint.x = P.x - sum.x * maxPen;
        m_GeckoPoint.y = P.y - sum.y * maxPen;
        m_GeckoPoint.z = P.z - sum.z * maxPen;
        m_GeckoPoint.w = P.w - sum.w * maxPen;
    }
    return mask;
}

// Gecko (wall crawl) offset: pushes 0.38 below the wall surface under the gecko point (further when the
// point is already behind the wall plane) and rotates the offset (0, -1, off) into the gecko frame q.
EXVector XItemHandler_Player::GetGeckoAdjustPoint(EXQuaternion &q)
{
    float off = -0.38f;
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->ItemPhysics();
    EXSphere s;
    s.Set(m_GeckoPoint, 1.0f);
    EXVector norm, pos;
    if (pPhys->CollideSphereClose(s, pos, norm)) {
        EXPlane pl;
        pl = EXPlane(pos, norm);
        float d = pl.DistanceToNormalised(m_GeckoPoint);
        if (d < 0.0f) off = d - 0.38f;
    }
    EXVector v(0.0f, -1.0f, off, 0.0f);
    return EXVector(q.MultiplyVector((EXMaths::vec_xyzw)v));
}

// NEAR (see check): keeps a held drone in the right hand. The drone faces away from the player (snapping
// at 10 degrees a frame, or when merely carried turning 5 degrees a frame within a -10 to 90 degree
// window), tilts nose-down as the hand nears the ground (below 1.7), and its grab datum is pinned to the
// hand; slams and throws (grab moves 5-10) instead let it settle to the player's height at 0.05 a frame.
void XItemHandler_Player::UpdateDroneToGrabHand()
{
    if (!m_HoldState) return;
    signed char bAttach = 1;
    Actor *pActor;
    EXItem *pDrone;
    if (!m_SpecialActorID || !(pActor = Actor::GetPtrFromID(m_SpecialActorID)) || pActor->m_StateFlags.m_bRagDoll ||
        !(pDrone = pActor->GetItem())) {
        m_SpecialActorID = 0;
        m_HoldState = 0;
        return;
    }
    Actor::OverridePlayerCloak();
    signed char bSnapTurn = 1;
    signed char bSettle = 0;
    signed char bTilt = 1;
    if (m_Mode == 2) bSnapTurn = 0;
    if (m_Mode == 0x16) {
        switch (m_SubMode) {
        case 5:
        case 6:
            bTilt = 0;
            bSettle = 1;
            bAttach = 0;
            break;
        case 7:
        case 8:
        case 9:
        case 10:
            bSettle = 1;
            bTilt = 0;
            break;
        case 3:
            bSnapTurn = 0;
            break;
        }
    }
    EXVector rot = pActor->GetItem()->Rotation();
    EXDatumTemp hand;
    if (!m_pItem->GetAnimatorDatum(0x1000001a, hand, 0, 0)) {
        memset(&hand, 0, sizeof hand);
        hand.m_position = m_pItem->Position();
    }
    float tilt;
    if (bTilt) {
        EXShadowTemp sh;
        sh.Set(hand.m_position, hand.m_position.y - 10.0f, hand.m_position.y, 0, 0);
        theItemEnv.DoShadowCastToMap(sh);
        if (hand.m_position.y - sh.m_faces[0].m_dist < 1.7f) {
            float f = (hand.m_position.y - sh.m_faces[0].m_dist) * 0.5882353f;
            if (f < -1.0f) tilt = -1.5707964f - 1.5707964f;
            else if (f > 1.0f) tilt = 1.5707964f - 1.5707964f;
            else tilt = (float)asin(f) - 1.5707964f;
            goto tilted;
        }
    }
    tilt = 0.0f;
tilted:
    float step = (bSnapTurn ? 0.17453292f : 0.043633230f) * XApp::GameFrameRateMul();
    float diff = Vec_AngleDifference(rot.y, m_pItem->Rotation().y + 3.1415927f);
    float turn;
    if (bSnapTurn) {
        turn = diff < step ? (-step < diff ? diff : -step) : step;
    } else if (diff >= 1.5707964f || diff <= -0.17453292f) {
        if (diff >= 0.0f) turn = diff - 1.5707964f;
        else turn = diff - -0.17453292f;
    } else {
        turn = diff < step ? (-step < diff ? diff : -step) : step;
    }
    EXVector r(tilt, rot.y + turn, rot.z, rot.w);
    pActor->GetItem()->m_rotation = r;
    pActor->GetItem()->m_validMtx = 0;
    pActor->m_OrgLoc.m_v20 = r;
    EXVector pos = pActor->GetItem()->Position();
    if (bAttach) {
        EXDatumTemp grab;
        if (!pDrone->GetAnimatorDatum(0xe000009, grab, 0, 0)) {
            memset(&grab, 0, sizeof grab);
            grab.m_position = pDrone->Position();
        }
        float a = m_pItem->Rotation().y + 0.75f;
        grab.m_position.y += 0.05f;
        grab.m_position.x += (float)sin(a) * -0.06f;
        grab.m_position.z += (float)cos(a) * -0.06f;
        pos += hand.m_position - grab.m_position;
        if (bSettle) pos.y = pActor->GetItem()->Position().y;
        pActor->SetPositionEx(pos);
    }
    if (bSettle) {
        float s = XApp::GameFrameRateMul() * 0.05f;
        float dy = pActor->GetItem()->Position().y - m_pItem->Position().y;
        pos.y = pActor->GetItem()->Position().y - (dy < s ? (-s < dy ? dy : -s) : s);
        pActor->SetPositionEx(pos);
    }
}

void XItemHandler_Player::HandleDamageSkins()
{
    if (!gpPlayerItem[0]) return;
    if (gCurrentMap_GeoHash != XApp::Frontend3DFile() && theApp.m_bCutscene) return;
    m_SkinBC = GetUpgradedSkin();
    int dmg;
    if (m_Health < 25.0f)
        dmg = 3;
    else if (m_Health < 50.0f)
        dmg = 2;
    else if (m_Health < 75.0f)
        dmg = 1;
    else
        dmg = 0;
    FEMainMenu::SetPredSkinLoaded(1);
    if (dmg == m_SkinC0 && m_SkinC4 == m_SkinBC) return;
    FEMainMenu::SetPredSkinLoaded(0);
    EXGeoFile *pFile = EXGeoFile::GetGeoFile(0x1000024);
    if (!pFile) return;
    EXGeoHeader *pHdr = pFile->m_pGeoHeader;
    unsigned int sub = GeoSectionManager::GetSubSection(0x1000024, PRED_Skins[m_SkinBC].m_Hash[dmg + 1]);
    switch (pHdr->IsSectionLoaded(sub, 1)) {
    case 0:
    case 5:
        pHdr->LoadSectionFile(sub, 1, 0);
        return;
    case 1:
    case 2:
    case 4:
        return;
    }
    m_SkinC0 = dmg;
    m_SkinC4 = m_SkinBC;
    AU_AssignSkin((EXItemAnimator_Anim *)m_pItem->GetAnimator(EXRUNTIME_CLASS(EXItemAnimator_Anim)), 0x1000024, PRED_Skins[m_SkinBC].m_Hash[dmg + 1]);
    HandleVisionModeTextureSwaps(m_Tint);
    for (unsigned int s = 0x8000001; s <= 0x800007f; s++)
        if (s != sub && pHdr->IsSectionLoaded(s, 1) == 3) pHdr->DeLoadSectionFile(s);
}

// NEAR (see check): after the animators: the scripted turn (m_TurnTime frames at m_TurnRate), the hand
// lock (an anchored hand drags the body with it, wall-crawl point kept on the wall plane; broken by any
// sideways collision push), the weapons, pending swing hits, damage to a ridden vehicle, cloak and vision.
signed char XItemHandler_Player::DoPostUpdate()
{
    if (!m_pItem->GetAnimator(EXRUNTIME_CLASS(EXItemAnimator_Anim))) return 0;
    if (m_TurnTime > 0.0f) {
        EXPlacementItem *pItem = m_pItem;
        EXVector r(pItem->Rotation().x, m_TurnRate * XApp::GameFrameRateMul() + pItem->Rotation().y, pItem->Rotation().z, pItem->Rotation().w);
        pItem->m_rotation = r;
        pItem->m_validMtx = 0;
        m_TurnTime -= XApp::FrameLength() * 60.0f;
    }
    if (m_HandLock > 0.0f && m_bHandLockFlag) {
        EXVector cr = ((XItemPhysics *)m_pItem->m_pItemPhysics)->m_CollisionResponse;
        float l = cr.z * cr.z + cr.x * cr.x;
        if (!(l < 0.0f) && (float)sqrt(l) > 0.001f) m_HandLock = -1.0f;
    }
    if (m_HandLock > 0.0f) {
        ((XItemPhysics *)m_pItem->m_pItemPhysics)->m_DeltaMove.SetZero();
        EXVector h(0.0f, 0.0f, 0.0f, 0.0f);
        GetHandLockPos(m_LockHandIdx, h);
        EXVector d = m_HandLockPos - h;
        if (m_Mode == 0x18 && m_SubMode == 3) {
            m_GeckoPoint += d;
            float dist = m_GeckoPoint.y * m_GeckoPlane.y + m_GeckoPoint.z * m_GeckoPlane.z + m_GeckoPlane.x * m_GeckoPoint.x +
                         m_GeckoPoint.w * 0.0f + m_GeckoPlane.w;
            m_GeckoPoint.x -= m_GeckoPlane.x * dist;
            m_GeckoPoint.y -= m_GeckoPlane.y * dist;
            m_GeckoPoint.z -= m_GeckoPlane.z * dist;
            m_GeckoPoint.w -= dist * 0.0f;
        }
        if (m_bHandLockFlag) {
            m_FallTimer = 0.0f;
            d.y = 0.0f;
        }
        EXPlacementItem *pItem = m_pItem;
        pItem->m_position = pItem->Position() + d;
        pItem->m_validMtx = 0;
        m_HandLock -= XApp::FrameLength() * 60.0f;
    }
    UpdatePlayerWeapons();
    UpdatePlayerBodyWeapons();
    if (m_HitEventFlags & 1) CheckForCharacterHit(1);
    if (m_HitEventFlags & 2) CheckForCharacterHit(2);
    m_HitEventFlags = 0;
    if (m_bDamageStandOn) {
        m_bDamageStandOn = 0;
        Vehicle *pVehicle = getItemOwnerVehicle(m_pStandOn);
        if (pVehicle) pVehicle->ApplyAttackDamage(0, 25.0f);
    }
    UpdateCloak();
    UpdateVisionMode();
    m_Wet = 0;
    return 1;
}

// NEAR: retail sets ecx before the last push of the shared SetupMove (scheduling).
signed char XItemHandler_Player::CheckForCombo()
{
    m_Combo = GetComboAvailable();
    if (m_Combo != -1) {
        ResetAttack();
        ResetCombo();
        DeadStop(1);
        LockHand(12, 5, 1);
        switch (m_Combo) {
        case 0:
        case 1:
        case 2:
            SetWeapon(1);
            break;
        case 3:
            SetWeapon(1);
            if (DoFinishingMove(0)) return 1;
            m_Combo = 3;
            break;
        case 4:
            SetWeapon(3);
            if (DoGrabMove(0)) return 1;
            m_Combo = 0;
            break;
        case 5:
            SetWeapon(3);
            m_Combo = 0;
            break;
        default:
            return 1;
        }
        SetupMove(7, 12, -1.0f);
        return 1;
    }
    return 0;
}

// Every grab point found by an array query, in order (the EXSegArray1 walk retail inlines: whole segments
// up to the tail, then the tail's used slots).
#define GRAB_FOREACH(arr, p, body)                                                              \
    {                                                                                          \
        EXSegArrayHeader1 *seg_ = (arr).m_points.m_pHead;                                      \
        u32 i_ = 0;                                                                            \
        if ((arr).m_points.m_count) {                                                          \
            if (seg_ != (arr).m_points.m_pTail) {                                              \
                do {                                                                           \
                    {                                                                          \
                        EXGrabPointData *p = (EXGrabPointData *)seg_ + (i_ & 31) + 1;          \
                        body                                                                   \
                    }                                                                          \
                    i_++;                                                                      \
                } while ((i_ & 31) || (seg_ = seg_->m_pNext, seg_ != (arr).m_points.m_pTail)); \
            }                                                                                  \
            do {                                                                               \
                {                                                                              \
                    EXGrabPointData *p = (EXGrabPointData *)seg_ + (i_ & 31) + 1;              \
                    body                                                                       \
                }                                                                              \
                i_++;                                                                          \
            } while (i_ < (arr).m_points.m_count);

// NEAR (see check): the ledge segment (not a polygon hit) nearest to pos among the query's points: a gets
// its end, b its start.
signed char XItemHandler_Player::GetSegmentOn(EXGrabPointArrayTemp &arr, EXVector &pos, EXVector &a, EXVector &b)
{
    signed char bFirst = 1;
    float best = 0.0f;
    GRAB_FOREACH(arr, p, {
        if (p->m_Type != 2) {
            EXLineSeg s(p->m_EdgePos, p->m_EdgeEnd - p->m_EdgePos);
            float t;
            float d = EXSqrDistance(pos, s, &t);
            if (d < best || bFirst) {
                a = p->m_EdgeEnd;
                bFirst = 0;
                b = p->m_EdgePos;
                best = d;
            }
        }
    })
            if (!bFirst) return 1;
        }
    }
    return 0;
}

// NEAR (see check): the ledge segment continuing from pos: the one whose start (dir 0) or end (dir 1) is
// nearest, if within 0.01; a gets its end, b its start.
signed char XItemHandler_Player::GetSegmentNext(EXGrabPointArrayTemp &arr, EXVector &pos, signed char dir, EXVector &a, EXVector &b)
{
    signed char bFirst = 1;
    float best = 0.0f;
    GRAB_FOREACH(arr, p, {
        if (p->m_Type != 2) {
            float d;
            if (!dir) {
                float l = (p->m_EdgePos.y - pos.y) * (p->m_EdgePos.y - pos.y) + (p->m_EdgePos.z - pos.z) * (p->m_EdgePos.z - pos.z) +
                          (p->m_EdgePos.x - pos.x) * (p->m_EdgePos.x - pos.x);
                d = l < 0.0f ? 0.0f : sqrtf(l);
            } else {
                float l = (p->m_EdgeEnd.y - pos.y) * (p->m_EdgeEnd.y - pos.y) + (p->m_EdgeEnd.z - pos.z) * (p->m_EdgeEnd.z - pos.z) +
                          (p->m_EdgeEnd.x - pos.x) * (p->m_EdgeEnd.x - pos.x);
                d = l < 0.0f ? 0.0f : sqrtf(l);
            }
            if (d < best || bFirst) {
                a = p->m_EdgeEnd;
                bFirst = 0;
                b = p->m_EdgePos;
                best = d;
            }
        }
    })
            if (best <= 0.01f && !bFirst) return 1;
        }
    }
    return 0;
}

// Walks the grab-point ledge from pos segment by segment (towards dir) and returns 1 once the ledge has
// covered dist without any segment turning more than maxTurn away from the previous one (angle tracks it).
// NEAR: retail schedules `add esp, 8` before the fxch of the fabs compare (3 bytes).
signed char XItemHandler_Player::GetSegmentSpace(EXGrabPointArrayTemp &arr, EXVector &pos, signed char dir, float dist, float angle, float maxTurn)
{
    int idx = dir != 0;
    EXVector cur = pos;
    EXVector seg[2];
    if (!GetSegmentNext(arr, cur, dir, seg[0], seg[1])) return 0;
    do {
        EXVector sd = seg[0] - seg[1];
        float a = GetRotation(sd);
        if ((float)fabs(Vec_AngleDifference(a, angle)) > maxTurn) return 0;
        angle = a;
        dist -= (cur - seg[idx]).LengthXZ();
        if (dist < 0.0f) return 1;
        cur = seg[idx];
    } while (GetSegmentNext(arr, cur, dir, seg[0], seg[1]));
    return 0;
}

// Ledge shimmy test: finds the ledge segment under the clamber point and checks there is 1.43 units of
// ledge to shimmy along in dir (dir is flipped when the segment faces against the player).
// NEAR: retail reuses a's stack slot for the LengthXZ temporaries (stack-slot packing only).
signed char XItemHandler_Player::CheckForShimmySpace(signed char dir)
{
    EXGrabPointArrayTemp arr;
    arr.Set(m_ClamberPos, 2.0f, (EXGrabPointArrayTemp::GRABFITYPE)3);
    theItemEnv.DoGrabPointToMap(arr);
    EXVector pos = m_ClamberPos;
    EXVector a, b;
    if (!GetSegmentOn(arr, pos, a, b)) return 0;
    EXVector ab = a - b;
    float angle = GetRotation(ab);
    if (Vec_AngleDifference(angle, m_pItem->Rotation().y) < 0.0f) dir = !dir;
    if (dir) pos = b;
    else pos = a;
    float space = 1.43f - (pos - m_ClamberPos).LengthXZ();
    if (space < 0.0f) return 1;
    if (GetSegmentSpace(arr, pos, dir, space, angle, 0.5410521f)) return 1;
    return 0;
}

// NEAR (see check): the gecko wall crawl (mode 0x18). Sub 0 starts it: the pose and position are blended
// onto the wall point over five frames (sub 1). Crawling (2 moving, 3 still): the stick moves along the
// wall, steered either relative to the camera (camera modes 7-9) or tank-style, through
// GetGeckoFormQuart; a ledge just above the head while upright grabs it (hang, mode 0xc). The weapon
// button fires the shoulder weapons, the jump buttons push off the wall (DoGeckoJumpOff).
void XItemHandler_Player::DoGecko()
{
    static float sStill;
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
    if (m_SubMode == 0) {
        sStill = 30.0f;
        m_Speed.SetZero();
        pPhys->m_physicsFlags |= 3;
        pPhys->m_DeltaMove.SetZero();
        SetupMove(0x18, 1, -1.0f);
        m_GeckoStartPos = m_pItem->Position();
        EXMatrix m;
        m.set_rotate_xyz(m_pItem->Rotation());
        EXQuaternion q;
        q.SetRotation(m);
        m_GeckoStartAngle = q;
        m_GeckoValidAngle = m_GeckoAngle;
        m_GeckoTargetPos = m_GeckoPoint;
        m_GeckoBlend = 0.0f;
    }
    if (m_SubMode == 1) {
        if (m_GeckoBlend > 1.0f) {
            m_GeckoAngle = m_GeckoValidAngle;
            m_GeckoPoint = m_GeckoTargetPos;
            SetupMove(0x18, 3, -1.0f);
            GetGeckoAngle(m_GeckoRoll);
        } else {
            m_GeckoAngle = EXQuaternion(m_GeckoStartAngle.slerp(m_GeckoValidAngle, m_GeckoBlend));
            float t = m_GeckoBlend;
            EXVector adj = GetGeckoAdjustPoint(m_GeckoAngle) * t;
            float b = m_GeckoBlend;
            EXVector p(m_GeckoStartPos.x + (m_GeckoTargetPos.x - m_GeckoStartPos.x) * b,
                       m_GeckoStartPos.y + (m_GeckoTargetPos.y - m_GeckoStartPos.y) * b,
                       m_GeckoStartPos.z + (m_GeckoTargetPos.z - m_GeckoStartPos.z) * b,
                       m_GeckoStartPos.w + (m_GeckoTargetPos.w - m_GeckoStartPos.w) * b);
            m_pItem->m_position = p + adj;
            m_pItem->m_validMtx = 0;
            m_GeckoBlend = XApp::GameFrameRateMul() * 0.2f + m_GeckoBlend;
        }
    }
    if (m_SubMode == 2 || m_SubMode == 3) {
        float move, roll;
        if (!(m_HandLock > 0.0f)) {
            unsigned char cm = XGetCamera(m_PlayerIndex)->m_Mode;
            if (cm < 7 || cm > 9) {
                move = m_Pad.z;
                if (m_Pad.z < 0.0f) move = 0.0f;
                roll = m_Pad.x * -0.05f * XApp::GameFrameRateMul();
            } else {
                float a;
                GetGeckoAngle(a);
                if (m_StickMag <= 0.001f) m_GeckoRoll = a;
                else {
                    a = Vec_AngleDifference(m_GeckoRoll, a);
                    float lim = (float)fabs(Vec_AngleDifference(m_GeckoRoll, 6.2831855f - m_StickAngle)) * 8.0f * XApp::GameFrameRateMul();
                    m_GeckoRoll += a < lim ? (-lim < a ? a : -lim) : lim;
                }
                roll = 0.0f;
                move = m_StickMag;
                if (move <= 0.01f) goto form;
                float e = Vec_AngleDifference(m_GeckoRoll, 6.2831855f - m_StickAngle);
                float lim = XApp::GameFrameRateMul() * 0.1f;
                roll = e < lim ? (0.0f - lim < e ? e : 0.0f - lim) : lim;
            }
            m_GeckoRoll += roll;
        } else {
            move = 0.0f;
            roll = 0.0f;
        }
form:
        move *= 0.05625f;
        EXVector saved = m_GeckoPoint;
        EXQuaternion q;
        EXPlane pl;
        if (GetGeckoFormQuart(q, pl, 1, move, roll) == 7) {
            float dist = m_GeckoPoint.y * pl.B + m_GeckoPoint.z * pl.C + pl.A * m_GeckoPoint.x + m_GeckoPoint.w * 0.0f + pl.D;
            m_GeckoPoint.x -= pl.A * dist;
            m_GeckoPoint.y -= pl.B * dist;
            m_GeckoPoint.z -= pl.C * dist;
            m_GeckoPoint.w -= dist * 0.0f;
            m_GeckoAngle = q;
            EXVector d = saved - m_GeckoPoint;
            float speed = d.x * d.x + d.y * d.y + d.z * d.z;
            speed = speed < 0.0f ? 0.0f : sqrtf(speed);
            speed *= XApp::GameFrameRateDiv();
            float ar = (float)fabs(roll);
            if (ar > 0.01f) {
                float lo = 0.05f;
                speed = speed < 0.05f ? lo : speed;
            }
            if (move <= 0.001f && ar <= 0.001f && sStill > 10.0f) sStill = 10.0f;
            float st = sStill;
            if (speed <= 0.02f) {
                m_GeckoPoint = saved;
                if (st <= 0.0f) goto still;
                m_AnimSpeed = 0.52f;
                sStill = st - XApp::GameFrameRateMul();
            } else {
                m_AnimSpeed = speed * 26.0f;
                if (m_SubMode != 2) {
                    float blend = 0.0f;
                    if (m_SubMode == 3 && m_Unk224 > 1.5f) blend = 15.0f;
                    SetupMove(0x18, 2, blend);
                    sStill = 30.0f;
                }
            }
        } else {
still:
            if (m_SubMode != 3) {
                m_Unk224 = (float)(int)m_Unk390;
                SetupMove(0x18, 3, -1.0f);
                m_AnimSpeed = 1.0f;
            }
        }
        if ((float)fabs(m_pItem->Rotation().x) < 0.034906585f && (float)fabs(m_pItem->Rotation().z) < 0.17453292f) {
            EXVector top(m_GeckoPoint.x, m_GeckoPoint.y + 1.3f, m_GeckoPoint.z, m_GeckoPoint.w);
            EXGrabPointTemp g;
            g.Set(top, 0.1f, (EXGrabPointTemp::GRABFITYPE)1);
            theItemEnv.DoGrabPointToMap(g);
            if (g.m_24 == 1) {
                m_ClamberPos = g.m_30;
                m_StateFlags = *(u32 *)&g.m_28;
                SetupMove(0xc, 1, -1.0f);
                m_Unk360 = 16.0f;
                return;
            }
        }
    }
    if (m_SubMode < 4) {
        EXVector r;
        m_GeckoAngle.GetRotation(r, 4);
        if ((float)fabs(r.x) > 1.5f) {
            EXQuaternion a = m_GeckoAngle;
            EXVector ta(0.0017453292f, 0.0f, 0.0f, 0.0f);
            EXMaths::quat_xyzw b;
            b.SetRotation(ta, 4);
            float ax = a.x, ay = a.y, az = a.z;
            a.z = b.y * ax + b.z * a.w + b.w * az - ay * b.x;
            a.y = az * b.x + b.y * a.w + b.w * ay - b.z * ax;
            a.x = b.w * ax + a.w * b.x + b.z * ay - b.y * az;
            a.w = b.w * a.w - b.x * ax - b.y * ay - b.z * az;
            a.GetRotation(r, 4);
        }
        m_pItem->m_rotation = r;
        m_pItem->m_validMtx = 0;
    }
    if (m_SubMode > 1 && m_SubMode < 4) {
        EXVector p = GetGeckoAdjustPoint(m_GeckoAngle) + m_GeckoPoint;
        m_pItem->m_position = p;
        m_pItem->m_validMtx = 0;
    }
    if (m_SubMode == 2 || m_SubMode == 3) {
        if (m_ButtonsHeld & 1) {
            int w = m_CurrentWeapon[0];
            if (w > 7 && w < 0xc) {
                if (m_Ammo[w] < 1)
                    EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00576, m_pItem->Position(), -1, 0);
                else if (!(m_Unk1ec > 0.0f)) ShootWeapon(w, 0);
            }
        }
        unsigned int held = m_ButtonsHeld;
        unsigned int mask;
        signed char b;
        if (held & 0x10) {
            mask = ~0x10u;
            b = 0;
        } else {
            if (held & 8) mask = ~8u;
            else if (held & 0x20) mask = ~0x20u;
            else return;
            b = 1;
        }
        m_ButtonsPressed &= mask;
        m_ButtonsHeld = held & mask;
        m_ButtonsReleased &= mask;
        DoGeckoJumpOff(b);
    }
}

// NEAR: retail converts dmg * 0.5f to int before the Rand32 call (kept in esi); ours converts after.
// Damage to the player. flags: 1 no blood/impact effect, 2 no hit grunt, 4 sparks instead of
// blood (armour hit). Armour upgrades scale damage by 0.95/0.9/0.8 (GetMaxArmour); the
// difficulty setting (GameValues m_Unk1c[0]) then halves it on easy and adds half on hard.
void XItemHandler_Player::InflictDamage(float dmg, int flags)
{
    if (m_MoveFlags & 0x800) return;
    if (theApp.m_bCutscene) return;
    if (!(flags & 1) && dmg > 0.0f) {
        EXVector p = (m_pItem->Position() + GetTopHeadPos()) * 0.5f;
        p.x += g_EXRandClass.Randf() * 0.5f - 0.25f;
        p.z += g_EXRandClass.Randf() * 0.5f - 0.25f;
        p.y += g_EXRandClass.Randf();
        if (!(flags & 4)) {
            XItem *pBlood = XItemHandler_ScriptBase::FX_CreateBlood(p, 0x4000228, 1, 0x40);
            pBlood->RenderInfo().RenderFlagsRef().BitSet(8);
            Gore_BloodPool(m_pItem->m_position, 1);
        } else {
            XItemHandler_ScriptBase::FX_Create(p, 0x400073d, 0x40, 0x1000016);
            EXBaseSoundManager::GetSoundManager()->StartSound(0x1af003ec, p, -1, 0);
        }
    }
    Actor::OverridePlayerCloak();
    if (m_bCloak) SparkSkin(0.25f);
    if (0.016666668f > m_ReactRecover) {
        m_ReactRecover = 0.5f;
        m_bIdleHitReact = 1;
        m_HitAngle = g_EXRandClass.Randf() * 6.2831855f - 3.1415927f;
        if (!(flags & 2))
            EXBaseSoundManager::GetSoundManager()->StartSound(0x1af001fe, m_pItem->Position(), -1, 0);
    }
    dmg *= GetMaxArmour();
    switch (GameData::Instance(1)->m_Values.m_Unk1c[0]) {
    case 0: dmg *= 0.5f; break;
    case 2: dmg *= 1.5f; break;
    }
    m_Health -= dmg;
    if (m_Health <= 0.0f) {
        KillPlayer();
        m_Health = 0.0f;
    }
    float r = limit<float>(dmg * 0.02f + 0.5f, 0.0f, 1.0f);
    ((XGamePad *)EXBaseGamePad::GetGamePad())->PlayRumble(0x54000001, r, m_PadIndex);
}

void XItemHandler_Player::InflictDamage(float dmg, EXVector &pos, EXVector &dir, int flags)
{
    if (m_MoveFlags & 0x800) return;
    if (theApp.m_bCutscene) return;
    float ang = GetRotation(dir);
    if (!(flags & 1) && dmg > 0.0f) {
        int n = limit<int>((int)(dmg * 0.5f) + g_EXRandClass.Rand32() % 3 - 1, 0, 4);
        unsigned int fx;
        switch (n) {
        case 1: fx = 0x40004d3; break;
        case 2: fx = 0x40004d4; break;
        case 3: fx = 0x40004d1; break;
        case 4: fx = 0x40004d2; break;
        default: fx = 0x40004d8; break;
        }
        XItem *p = XItemHandler_ScriptBase::FX_CreateBlood(pos, fx, 1, 0x40);
        if (p) p->SetRotation(EXQuaternion(0.0f, ang, 0.0f, 0.0f));
    }
    InflictDamage(dmg, flags | 1);
    m_HitAngle = EXMakeAngleBiPolar(ang - m_pItem->Rotation().y);
}

XItemHandler_Player::XItemHandler_Player()
{
    m_pInstance = this;
}

// NEAR: SSE register allocation of the position add and capsule stores (retail adds pos.x into p.x,
// stores the capsule direction's y before its zeros).
signed char XItemHandler_Player::CheckForFinishMoveSpace(signed char bGround, float dist, float y, float radius, float h)
{
    float a = m_pItem->Rotation().y;
    EXVector p;
    p.x = (float)sin(a);
    p.z = (float)cos(a);
    p.x *= dist;
    p.z *= dist;
    p.w = 0.0f;
    p.y = 0.0f;
    p += m_pItem->Position();
    p.y += y;
    EXCapsule cap;
    EXVector out;
    cap.Origin() = p;
    cap.Direction() = EXVector(0.0f, h, 0.0f, 0.0f);
    cap.Radius() = radius;
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
    if (!pPhys->CollideCapsule(cap, out)) {
        if (!bGround) return 1;
        EXRayCastTemp rc;
        EXVector d(0.0f, -10.0f, 0.0f, 0.0f);
        rc.Set(p, d, 0, 0, 0, 0);
        theItemEnv.DoRayCastToMap(rc, 0);
        if (0.05f > (float)fabs(rc.m_t * 10.0f - y)) return 1;
    }
    return 0;
}

// NEAR (see check): a finishing move is available (special move flag 4) with a heavy weapon (2-4) on a
// hitable, standing, juggleable target (the camera's target, or the lock-on when b) that is level with
// the player (0.2), within 3, not facing away from him by more than 135 degrees, and has room around it
// and 1.5 in front of it.
signed char XItemHandler_Player::CheckForFinishingMove(signed char b)
{
    m_SpecialMoveAvailable &= ~4;
    if (m_CurrentWeapon[1] < 2 || m_CurrentWeapon[1] > 4) return 0;
    Actor *pActor;
    if (!b) {
        XItemHandler_Camera *pCam = XGetCamera(m_PlayerIndex);
        if (pCam->GetTargetID() <= 0) return 0;
        pActor = Actor::GetPtrFromID(pCam->GetTargetID());
    } else {
        int type;
        pActor = (Actor *)XGetCamera(0)->GetLockOn(type);
        if (type != 1) return 0;
    }
    if (!pActor || pActor->m_HitCountdown > 0.0f || (pActor->m_Health <= 0.0f && !pActor->m_StateFlags.m_bCanRipBossHead) ||
        pActor->m_ActorFlags.m_bHolographic || pActor->m_StateFlags.m_bLyingOnGround || !pActor->CanBeJuggled())
        return 0;
    EXItem *pMe = m_pItem;
    EXItem *pThem = pActor->GetItem();
    EXVector d = pThem->Position() - pMe->Position();
    if ((float)fabs(d.y) > 0.2f) return 0;
    float l = d.y * d.y + d.z * d.z + d.x * d.x;
    if (!(l < 0.0f) && (float)sqrt(l) > 3.0f) return 0;
    if ((float)fabs(Vec_AngleDifference(pMe->Rotation().y, pThem->Rotation().y + 3.1415927f)) > 2.3561945f) return 0;
    EXVector p = pThem->Position();
    if (!CheckForStandingSpace(p, 5.0f)) return 0;
    float a = (float)atan2(d.x, d.z);
    EXVector off(0.059f, 0.0f, 1.5f, 0.0f), out;
    EXMatrix m;
    float s = (float)sin(a + 3.1415927f), c = (float)cos(a + 3.1415927f);
    m.r[0].set(c, 0.0f, -s, 0.0f);
    m.r[1].set(0.0f, 1.0f, 0.0f, 0.0f);
    m.r[2].set(s, 0.0f, c, 0.0f);
    m.r[3].set(0.0f, 0.0f, 0.0f, 1.0f);
    D3DXVec4Transform(&out, &off, &m);
    EXVector q = out + pActor->GetItem()->Position();
    if (!CheckForStandingSpace(q, 1.5f)) return 0;
    m_GroundAttackID = pActor->GetRefID();
    m_SpecialMoveAvailable |= 4;
    return 1;
}

// NEAR (see check): starts the finishing move found by CheckForFinishingMove: stand 1.5 in front of the
// target facing it, draw the heavy weapon and play its finisher (move 7, 13-15 for weapons 2-4) with the
// matching attack message (0x1f-0x21) to the target.
signed char XItemHandler_Player::DoFinishingMove(signed char b)
{
    if (!CheckForFinishingMove(b)) return 0;
    Actor *pActor = Actor::GetPtrFromID(m_GroundAttackID);
    if (!pActor) return 0;
    EXVector d = pActor->GetItem()->Position() - m_pItem->Position();
    float a = (float)atan2(d.x, d.z);
    EXVector off(0.059f, 0.0f, 1.5f, 0.0f), out;
    float r = a + 3.1415927f;
    EXMatrix m;
    float s = (float)sin(r), c = (float)cos(r);
    m.r[0].set(c, 0.0f, -s, 0.0f);
    m.r[1].set(0.0f, 1.0f, 0.0f, 0.0f);
    m.r[2].set(s, 0.0f, c, 0.0f);
    m.r[3].set(0.0f, 0.0f, 0.0f, 1.0f);
    D3DXVec4Transform(&out, &off, &m);
    m_TargetAdjust = pActor->GetItem()->Position() + out;
    m_TargetAdjustTime = a;
    m_SpecialActorID = m_GroundAttackID;
    m_bUnkc40 = 0;
    SetWeapon(1);
    switch (m_CurrentWeapon[1]) {
    case 2: {
        SetupMove(7, 0xd, -1.0f);
        EXVector r1(0.0f, r, 0.0f, 0.0f);
        EXVector r2(0.0f, 0.0f, 0.0f, 0.0f);
        SendMsg_AttackedByPredator(Actor::GetPtrFromID(m_SpecialActorID), 0x1f, r2, r1, 0.0f, 0);
        break;
    }
    case 3: {
        SetupMove(7, 0xe, -1.0f);
        EXVector r1(0.0f, r, 0.0f, 0.0f);
        EXVector r2(0.0f, 0.0f, 0.0f, 0.0f);
        SendMsg_AttackedByPredator(Actor::GetPtrFromID(m_SpecialActorID), 0x20, r2, r1, 0.0f, 0);
        break;
    }
    case 4: {
        SetupMove(7, 0xf, -1.0f);
        EXVector r1(0.0f, r, 0.0f, 0.0f);
        EXVector r2(0.0f, 0.0f, 0.0f, 0.0f);
        SendMsg_AttackedByPredator(Actor::GetPtrFromID(m_SpecialActorID), 0x21, r2, r1, 0.0f, 0);
        break;
    }
    }
    DeadStop(1);
    m_SpecialMoveAvailable = 0;
    return 1;
}

void XItemHandler_Player::EnableCloak(signed char b)
{
    m_bCloak = b;
    if (b && !m_Tint) {
        float f = m_CloakRefraction;
        ((EXItemAnimator_Anim *)m_pItem->GetAnimator(EXRUNTIME_CLASS(EXItemAnimator_Anim)))->m_pSkin->RenderInfo().RenderFlagsRef().BitSet(0x2000);
        float r = f * 0.25f;
        m_pAnim->RenderInfo().ExtRenderInfo()->SetRefractionStrength(r);
        m_pAnim->RenderInfo().ExtRenderInfo()->m_Unk78 |= 4;
        m_pAnim->RenderInfo().RenderFlagsRef().BitClr(0x4000);
        int i = 0;
        do {
            if (i != 11) {
                ((EXItemAnimator *)m_pWeaponItem[i])->RenderInfo().RenderFlagsRef().BitSet(0x2000);
                ((EXItemAnimator *)m_pWeaponItem[i])->RenderInfo().ExtRenderInfo()->SetRefractionStrength(r);
                ((EXItemAnimator *)m_pWeaponItem[i])->RenderInfo().ExtRenderInfo()->m_Unk78 |= 4;
            } else {
                ((EXItemAnimator *)m_pWeaponItem[11])->RenderInfo().RenderFlagsRef().BitClr(0x2000);
            }
            ((EXItemAnimator *)m_pWeaponItem[i])->RenderInfo().RenderFlagsRef().BitClr(0x4000);
            ((EXItemAnimator *)m_pWeaponItem[i])->RenderInfo().RenderFlagsRef().BitClr(4);
            ((EXItemAnimator *)m_pWeaponItem[i])->RenderInfo().SetRGBAMult(1.0f, 1.0f, 1.0f, m_VisionFade);
        } while (++i < 12);
        return;
    }
    s8 bHeat;
    if (m_Tint == 1 || m_Tint == 3 || m_Tint == 2)
        bHeat = 1;
    else
        bHeat = 0;
    ((EXItemAnimator_Anim *)m_pItem->GetAnimator(EXRUNTIME_CLASS(EXItemAnimator_Anim)))->m_pSkin->RenderInfo().RenderFlagsRef().BitClr(0x2000);
    m_pAnim->RenderInfo().RenderFlagsRef().BitState(0x4000, bHeat);
    int j = 0;
    do {
        ((EXItemAnimator *)m_pWeaponItem[j])->RenderInfo().RenderFlagsRef().BitClr(0x2000);
        ((EXItemAnimator *)m_pWeaponItem[j])->RenderInfo().RenderFlagsRef().BitState(0x4000, bHeat);
        ((EXItemAnimator *)m_pWeaponItem[j])->RenderInfo().RenderFlagsRef().BitSet(4);
    } while (++j < 12);
}

extern signed char KillMeImAllWrong;
float TimeToDie;

signed char XItemHandler_Player::DoUpdate()
{
    EXItemAnimator_Anim *pAnim = (EXItemAnimator_Anim *)m_pItem->GetAnimator(EXRUNTIME_CLASS(EXItemAnimator_Anim));
    if (!pAnim) return 0;
    LoadWeaponsAsync();
    EXItemCtrl *pCtrl = pAnim->Controller(0);
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
    if (m_bHidden && theApp.GetCutscene()) {
        if (pPhys) pPhys->EnablePhysics(0);
        if (pCtrl) pCtrl->CtrlFlags().BitClr(2);
        if (m_PlasmaChargeTime > 0.0f) {
            m_PlasmaChargeTime = 0.0f;
            if (m_PlasmaSound) {
                m_SoundTag.Remove(m_PlasmaSound, 1);
                m_PlasmaSound = 0;
                m_PlasmaSoundIdx = -1;
            }
        }
        return 1;
    }
    if (pPhys) pPhys->EnablePhysics(1);
    if (pCtrl) pCtrl->CtrlFlags().BitSet(2);
    if (m_Mode == 0x13)
        pPhys->SetCollisionObject(0.4f, 0.4f, 1.45f, 0.0f);
    else if (m_HoldState)
        pPhys->SetCollisionObject(0.75f, 0.75f, 0.75f, 0.25f);
    else
        pPhys->SetCollisionObject(0.75f, 0.75f, 0.75f, 0.0f);
    float rate;
    if (m_Mode == 0x18 && m_SubMode == 2)
        rate = m_AnimSpeed * XApp::GlobalGameFrameRate();
    else
        rate = XApp::GlobalGameFrameRate();
    pAnim->SetFrameRate60(rate * 60.0f);
    CalculateTargetJump();
    signed char r = UpdatePlayer();
    PostUpdatePlayer();
    if (m_bAnimEnded) m_bAnimEnded = 0;
    m_Unk390 = 0;
    m_pItem->m_mapOn = theItemEnv.GetMapOn(m_pItem->Position());
    if (KillMeImAllWrong) {
        TimeToDie -= XApp::GameFrameRateMul();
        if (0.0f > TimeToDie) {
            __asm {
                rdtsc
                sub esp, 0x64
                mov ebp, eax
                ret
            }
        }
    }
    return r;
}

void XItemHandler_Player::ReactStumble(EXVector &from)
{
    if (m_MoveFlags & 0x800) return;
    if (m_ReactTimer > 0.0f) return;
    if (m_Mode == 0x17 && (m_SubMode == 0xf || m_SubMode == 0xe)) return;
    if (0.0f >= m_Health) return;
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
    m_Unk358 = 60.0f;
    m_Unk35c = 60.0f;
    pPhys->SetPhysicsFlagsOff(3);
    RestorePreviousWeapon();
    DeadStop(1);
    ResetAttack();
    SafeBreakoutAll(0);
    EXMaths::vec_xyzw t;
    t.x = m_pItem->Position().x - from.x;
    t.y = m_pItem->Position().y - from.y;
    t.z = m_pItem->Position().z - from.z;
    t.w = m_pItem->Position().w - from.w;
    EXVector d = (EXMaths::vec_xyzw)t;
    float ang = GetRotation(d);
    EXQuaternion q = m_pItem->Rotation();
    if ((float)fabs(Vec_AngleDifference(q.y, ang)) > 1.5707964f) {
        SetupMove(0x17, 0xf, -1.0f);
        ang += 3.1415927f;
    } else
        SetupMove(0x17, 0xe, -1.0f);
    q.y = ang;
    m_pItem->SetRotation(q);
}

// NEAR: block merging; retail keeps a dist store per case body and cross-jumps the mode 4 sub-cases
// into the identical mode 0xc bodies (ours tail-merges the stores instead).
void XItemHandler_Player::GetGrabVector(int, EXVector &v)
{
    float h = 0.0f;
    float dist = 0.0f;
    switch (m_Mode) {
    case 0x11:
        if (m_SubMode == 0) {
            h = 2.39f;
            dist = 0.0f;
        }
        break;
    case 0xc:
        switch (m_SubMode) {
        case 0:
        case 2:
        case 3:
        case 4:
            if (m_StateFlags & 0x100) {
                h = 2.208f;
                dist = 0.76f;
            } else {
                h = 2.195f;
                dist = 0.54f;
            }
            break;
        case 5:
        case 6:
        case 7:
            if (m_StateFlags & 0x100) {
                h = 2.528f;
                dist = 0.313f;
            } else {
                h = 2.51f;
                dist = 0.15f;
            }
            break;
        case 8:
        case 9:
            h = 2.505f;
            dist = 0.0f;
            break;
        }
        break;
    case 4:
        switch (m_SubMode) {
        case 0:
        case 1:
            h = 2.195f;
            dist = 0.54f;
            break;
        case 3:
            h = 1.805f;
            dist = 0.54f;
            break;
        case 4:
            h = 1.515f;
            dist = 0.54f;
            break;
        case 5:
        case 6:
            h = 2.505f;
            dist = 0.0f;
            break;
        case 9:
            h = 2.528f;
            dist = 0.313f;
            break;
        case 10:
            h = 2.208f;
            dist = 0.76f;
            break;
        }
        break;
    }
    SetRotation(v, m_pItem->Rotation().y, dist);
    v.y = h;
    v += m_pItem->Position();
}

signed char XItemHandler_Player::CheckForGroundAttack()
{
    m_SpecialMoveAvailable &= ~8;
    int type;
    Actor *pActor = (Actor *)XGetCamera(0)->GetLockOn(type);
    if (type != 1) return 0;
    if (!pActor) return 0;
    int bBoss = pActor->m_StateFlags.m_bCanRipBossHead;
    if (!bBoss) {
        if (!(0.0f >= pActor->m_HitCountdown)) return 0;
        if (0.0f >= pActor->m_Health) return 0;
        if (pActor->m_ActorFlags.m_bHolographic) return 0;
        if (!pActor->m_StateFlags.m_bLyingOnGround) return 0;
    }
    EXMaths::vec_xyzw t;
    const EXVector &q = m_pItem->Position();
    const EXVector &p = pActor->GetFeetPos();
    t.x = p.x - q.x;
    t.y = p.y - q.y;
    t.z = p.z - q.z;
    t.w = p.w - q.w;
    EXVector d = (EXMaths::vec_xyzw)t;
    if (d.LengthXYZ() > 3.0f) return 0;
    float ang = Vec_AngleDifference(m_pItem->Rotation().y, GetRotation(d));
    if (bBoss == 1) {
        if ((float)fabs(d.y) > 0.6f) return 0;
        if ((float)fabs(ang) > 1.3962634f) return 0;
    } else if ((float)fabs(d.y) > 0.2f || (float)fabs(ang) > 1.0471976f)
        return 0;
    m_GroundAttackID = pActor->m_RefID;
    m_SpecialMoveAvailable |= 8;
    return 1;
}

signed char XItemHandler_Player::MonkeyCheckAhead()
{
    float a = m_pItem->Rotation().y;
    EXVector p;
    float k = 0.75f;
    p.x = (float)sin(a);
    p.z = (float)cos(a);
    p.x *= k;
    p.z *= k;
    p.w = 0.0f;
    p.y = 0.0f;
    p += m_pItem->Position();
    p.y = m_ClamberPos.y;
    EXGrabPointTemp t;
    t.Set(p, 0.5f, (EXGrabPointTemp::GRABFITYPE)2);
    theItemEnv.DoGrabPointToMap(t);
    if (t.m_24 != 2) return 0;
    EXMaths::vec_xyzw r;
    r.x = p.x - t.m_30.x;
    r.y = p.y - t.m_30.y;
    r.z = p.z - t.m_30.z;
    r.w = p.w - t.m_30.w;
    EXMaths::vec_xyzw r2 = r;
    EXVector e(r2);
    EXMaths::vec_xyzw w = e;
    if (!(EXSqrt(w.x * w.x + w.z * w.z) > 0.1f)) return 1;
    return 0;
}

static __forceinline void SetHeatSwaps(EXTexSwapInfo &t)
{
    XItemHandler_Player *pPlayer = XItemHandler_Player::GetInstance();
    switch (pPlayer->m_Tint) {
    case 2:
        if (pPlayer->m_bCloak) {
            t.SetSwap(0x6000075, 0x1000016, 0x6000122);
            t.SetSwap(0x6000077, 0x1000016, 0x6000122);
            t.SetSwap(0x6000076, 0x1000016, 0x6000122);
            t.SetSwap(0x6000074, 0x1000016, 0x6000122);
        } else {
            t.SetSwap(0x6000075, 0x1000016, 0x6000073);
            t.SetSwap(0x6000077, 0x1000016, 0x6000073);
            t.SetSwap(0x6000076, 0x1000016, 0x6000073);
            t.SetSwap(0x6000074, 0x1000016, 0x6000073);
        }
        break;
    case 3:
        if (pPlayer->m_bCloak) {
            t.SetSwap(0x6000075, 0x1000016, 0x60000bc);
            t.SetSwap(0x6000077, 0x1000016, 0x60000bc);
            t.SetSwap(0x6000076, 0x1000016, 0x60000bc);
            t.SetSwap(0x6000074, 0x1000016, 0x60000bc);
        } else {
            t.SetSwap(0x6000075, 0x1000016, 0x6000073);
            t.SetSwap(0x6000077, 0x1000016, 0x6000073);
            t.SetSwap(0x6000076, 0x1000016, 0x6000073);
            t.SetSwap(0x6000074, 0x1000016, 0x6000073);
        }
        break;
    default:
        if (pPlayer->m_bCloak) {
            t.SetSwap(0x6000075, 0x1000016, 0x60000c7);
            t.SetSwap(0x6000077, 0x1000016, 0x60000c7);
            t.SetSwap(0x6000076, 0x1000016, 0x60000c7);
            t.SetSwap(0x6000074, 0x1000016, 0x60000c7);
        } else {
            t.SetSwap(0x6000075, 0x1000016, 0x6000072);
            t.SetSwap(0x6000077, 0x1000016, 0x6000061);
            t.SetSwap(0x6000076, 0x1000016, 0x6000062);
            t.SetSwap(0x6000074, 0x1000016, 0x6000063);
        }
        break;
    }
}

void XItemHandler_Player::SetWeaponsToHeatRender(unsigned char b)
{
    int i = 0;
    do {
        ((EXItemAnimator *)m_pWeaponItem[i])->RenderInfo().RenderFlagsRef().BitState(0x4000, b);
        ((EXItemAnimator *)m_pWeaponItem[i])->RenderInfo().RenderFlagsRef().BitState(8, b);
        ((EXItemAnimator *)m_pWeaponItem[i])->RenderInfo().RenderFlagsRef().BitState(4, !b);
    } while (++i < 12);
    if (b) {
        int j = 0;
        do {
            SetHeatSwaps(m_WeaponTexSwap[j]);
        } while (++j < 12);
    } else {
        int j = 0;
        do {
            m_WeaponTexSwap[j].StopAllSwaps();
        } while (++j < 12);
    }
}

void XItemHandler_Player::HandleVisionModeTextureSwaps(unsigned char mode)
{
    switch (mode) {
    case 2:
        gpGameWnd[0]->m_Flags.BitClr(0x40000);
        m_pAnim->RenderInfo().RenderFlagsRef().BitSet(0x4000);
        SetWeaponsToHeatRender(1);
        if (m_bCloak) {
            m_PlayerTexSwap.SetSwap(0x6000075, 0x1000016, 0x6000122);
            m_PlayerTexSwap.SetSwap(0x6000077, 0x1000016, 0x6000122);
            m_PlayerTexSwap.SetSwap(0x6000076, 0x1000016, 0x6000122);
            m_PlayerTexSwap.SetSwap(0x6000074, 0x1000016, 0x6000122);
        } else {
            m_PlayerTexSwap.SetSwap(0x6000075, 0x1000016, 0x6000073);
            m_PlayerTexSwap.SetSwap(0x6000077, 0x1000016, 0x6000073);
            m_PlayerTexSwap.SetSwap(0x6000076, 0x1000016, 0x6000073);
            m_PlayerTexSwap.SetSwap(0x6000074, 0x1000016, 0x6000073);
        }
        break;
    case 1:
        gpGameWnd[0]->m_Flags.BitClr(0x40000);
        m_pAnim->RenderInfo().RenderFlagsRef().BitSet(0x4000);
        SetWeaponsToHeatRender(1);
        if (!m_bCloak) {
            m_PlayerTexSwap.SetSwap(0x6000075, 0x1000016, 0x6000072);
            m_PlayerTexSwap.SetSwap(0x6000077, 0x1000016, 0x6000061);
            m_PlayerTexSwap.SetSwap(0x6000076, 0x1000016, 0x6000062);
            m_PlayerTexSwap.SetSwap(0x6000074, 0x1000016, 0x6000063);
        } else {
            m_PlayerTexSwap.SetSwap(0x6000075, 0x1000016, 0x60000c7);
            m_PlayerTexSwap.SetSwap(0x6000077, 0x1000016, 0x60000c7);
            m_PlayerTexSwap.SetSwap(0x6000076, 0x1000016, 0x60000c7);
            m_PlayerTexSwap.SetSwap(0x6000074, 0x1000016, 0x60000c7);
        }
        break;
    case 3:
        gpGameWnd[0]->m_Flags.BitClr(0x40000);
        m_pAnim->RenderInfo().RenderFlagsRef().BitSet(0x4000);
        SetWeaponsToHeatRender(1);
        if (!m_bCloak) {
            m_PlayerTexSwap.SetSwap(0x6000075, 0x1000016, 0x6000073);
            m_PlayerTexSwap.SetSwap(0x6000077, 0x1000016, 0x6000073);
            m_PlayerTexSwap.SetSwap(0x6000076, 0x1000016, 0x6000073);
            m_PlayerTexSwap.SetSwap(0x6000074, 0x1000016, 0x6000073);
        } else {
            m_PlayerTexSwap.SetSwap(0x6000075, 0x1000016, 0x60000bc);
            m_PlayerTexSwap.SetSwap(0x6000077, 0x1000016, 0x60000bc);
            m_PlayerTexSwap.SetSwap(0x6000076, 0x1000016, 0x60000bc);
            m_PlayerTexSwap.SetSwap(0x6000074, 0x1000016, 0x60000bc);
        }
        break;
    default: {
        gpGameWnd[0]->m_Flags.BitSet(0x40000);
        m_pAnim->RenderInfo().RenderFlagsRef().BitClr(0x4000);
        unsigned char b = 0;
        int i = 0;
        do {
            ((EXItemAnimator *)m_pWeaponItem[i])->RenderInfo().RenderFlagsRef().BitState(0x4000, b);
            ((EXItemAnimator *)m_pWeaponItem[i])->RenderInfo().RenderFlagsRef().BitState(8, b);
            ((EXItemAnimator *)m_pWeaponItem[i])->RenderInfo().RenderFlagsRef().BitState(4, !b);
        } while (++i < 12);
        int j = 0;
        do {
            m_WeaponTexSwap[j].StopAllSwaps();
        } while (++j < 12);
        m_PlayerTexSwap.StopAllSwaps();
        break;
    }
    case 0x14d: {
        gpGameWnd[0]->m_Flags.BitClr(0x40000);
        m_pAnim->RenderInfo().RenderFlagsRef().BitClr(0x4000);
        unsigned char b = 0;
        int i = 0;
        do {
            ((EXItemAnimator *)m_pWeaponItem[i])->RenderInfo().RenderFlagsRef().BitState(0x4000, b);
            ((EXItemAnimator *)m_pWeaponItem[i])->RenderInfo().RenderFlagsRef().BitState(8, b);
            ((EXItemAnimator *)m_pWeaponItem[i])->RenderInfo().RenderFlagsRef().BitState(4, !b);
        } while (++i < 12);
        int j = 0;
        do {
            m_WeaponTexSwap[j].StopAllSwaps();
        } while (++j < 12);
        m_PlayerTexSwap.StopAllSwaps();
        break;
    }
    }
}

void XItemHandler_Player::UpdateWeaponUpgradeGFX(int weapon)
{
    if (weapon == 1) return;
    if (weapon == -1) {
        for (int i = 0; i < 6; i++) {
            int id = Weapon_Shapes[i].m_Id;
            if (id != 1) WeaponHash[id][0] = Weapon_Shapes[i].m_Hash[GetWeaponUpgrade(id)];
        }
    } else {
        for (int i = 0; i < 6; i++) {
            if (Weapon_Shapes[i].m_Id == weapon) WeaponHash[weapon][0] = Weapon_Shapes[i].m_Hash[GetWeaponUpgrade(weapon)];
        }
    }
}

void XItemHandler_Player::DoClamber()
{
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
    HandleGrabAdjustment();
    if (!m_bAnimEnded) return;
    m_bAnimEnded = 0;
    switch (m_SubMode) {
    case 9:
    case 10:
        m_bHandLockFlag = 0;
        m_LockHandIdx = 0xc;
        GetHandLockPos(0xc, m_HandLockPos);
        m_HandLock = 8.0f;
        m_ShimmyDir = 1.5f;
        SetupMove(0x12, 0, -1.0f);
        return;
    case 5:
    case 6:
        pPhys->SetPhysicsFlagsOn(3);
        m_ClamberSide = 1 - m_ClamberSide;
        if (!GetGrabPoint(3)) {
            m_Unk358 = 0.33333334f;
            SetupMove(9, 0, -1.0f);
            return;
        }
        if (m_StateFlags & 8) {
            if (m_Pad.z > 0.9f) {
                if (m_ClamberSide)
                    SetupMove(4, 5, -1.0f);
                else
                    SetupMove(4, 6, -1.0f);
            } else
                SetupMove(0xc, 8, -1.0f);
        } else
            SetupMove(0xc, 0, -1.0f);
        m_Unk360 = 10.0f;
        m_bUnk364 = 1;
        return;
    default:
        pPhys->SetPhysicsFlagsOff(3);
        pPhys->SetThrust(0.0f);
        switch (m_SubMode) {
        case 0:
        case 1:
        case 2:
        case 7:
        case 8:
            m_FallTimer = 0.0f;
            if (m_StickMag > 0.9f)
                SetupMove(1, 0, -1.0f);
            else
                SetupMove(0, 2, -1.0f);
            return;
        case 3:
            m_FallTimer = 0.0f;
            SetInertia(1.0f);
            m_MSpeed = 3.0f;
            SetupMove(1, 1, 13.564025f);
            return;
        case 4:
            m_FallTimer = 0.0f;
            SetInertia(1.0f);
            m_MSpeed = 3.0f;
            SetupMove(1, 1, 27.75143f);
            return;
        }
    }
}

void XItemHandler_Player::UpdateCloak()
{
    switch (m_CloakState) {
    case 0:
        EnableCloak(0);
        break;
    case 2:
        EnableCloak(1);
        break;
    case 1:
        EnableCloak(0);
        if (m_SparkTimer <= 0.5f) {
            EnableCloak(1);
            if (m_Tint) HandleVisionModeTextureSwaps(m_Tint);
            m_CloakState = 2;
        }
        break;
    case 3:
        EnableCloak(1);
        if (m_SparkTimer <= 0.5f) {
            EnableCloak(0);
            if (m_Tint) HandleVisionModeTextureSwaps(m_Tint);
            m_CloakState = 0;
        }
        break;
    case 4:
        EnableCloak(0);
        if (m_Tint) HandleVisionModeTextureSwaps(m_Tint);
        break;
    }
    if (m_SparkTimer > 0.0f) {
        m_SparkTimer -= XApp::FrameLength();
        if (m_SparkTimer <= 0.0f) {
            m_SparkTimer = 0.0f;
            if (m_bSparkSkin) {
                EXGeoRefObject ref;
                ref.m_pGeoFile = EXGeoFile::GetGeoFile(0x1000024);
                ref.m_hashRef = 0xd0000cc;
                ((EXItemAnimator_Anim *)m_pItem->GetAnimator(EXRUNTIME_CLASS(EXItemAnimator_Anim)))->DeleteCloneSkin(ref, 0);
                m_bSparkSkin = 0;
            }
        }
    }
    if (gCurrentMap_GeoHash == XApp::Frontend3DFile()) return;
    if (m_bCloak && m_bPlayerHasControl && !theApp.GetCutscene()) {
        if (!(m_Mode == 7 && m_SubMode == 1)) m_Energy -= XApp::GameFrameRateMul() * 0.00925f;
        if (m_Energy <= 0.0f) {
            SetCloak(0);
            m_Energy = 0.0f;
        }
    }
    if (m_bCloakBroken) {
        m_bCloakBroken = 0;
        m_CloakBreakTimer += XApp::FrameLength();
        if (m_CloakBreakTimer >= (float)m_CloakBreak && m_CloakState == 2) {
            m_CloakState = 4;
            SetCloak(0);
            EnableCloak(0);
            if (m_Tint) HandleVisionModeTextureSwaps(m_Tint);
        }
    } else {
        float t = m_CloakBreakTimer - XApp::FrameLength();
        m_CloakBreakTimer = max(0.0f, t);
    }
}

void XItemHandler_Player::ReactWeaponPickup(int weapon, signed char b)
{
    if (b && !((EXItemAnimator_Anim *)m_pItem->GetAnimator(EXRUNTIME_CLASS(EXItemAnimator_Anim)))->Controller(1) &&
        (m_Mode < 6 || (m_Mode > 7 && m_Mode != 0x17))) {
        if (!GameData::Instance(1)->Values()->m_bAutoEquip) return;
        m_PickupWeapon = 0;
        m_PickupTimer = 0.0f;
        switch (weapon) {
        case 1:
            SetCurrentWeapon(3, weapon);
            SetWeapon(3);
            return;
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
            SetCurrentWeapon(0, weapon);
            SetWeapon(0);
            return;
        case 2:
        case 3:
        case 4:
            SetCurrentWeapon(1, weapon);
            SetWeapon(1);
            return;
        }
        return;
    }
    if (m_Mode == 0x16) return;
    if (m_HoldState == 1) return;
    if (m_Mode > 1) return;
    m_PickupWeapon = 0;
    m_PickupTimer = 0.0f;
    switch (weapon) {
    case 1:
        DeadStop(1);
        m_PrevWeapon = m_CurrentWeapon[3];
        SetCurrentWeapon(3, weapon);
        SetWeapon(3);
        SetupMove(0x17, 0xc, -1.0f);
        break;
    case 5:
    case 6:
        DeadStop(1);
        m_PrevWeapon = m_CurrentWeapon[0];
        SetCurrentWeapon(0, weapon);
        SetWeapon(0);
        SetupMove(0x17, 0xc, -1.0f);
        break;
    case 2:
    case 3:
    case 4:
        DeadStop(1);
        m_PrevWeapon = m_CurrentWeapon[1];
        SetCurrentWeapon(1, weapon);
        SetWeapon(1);
        SetupMove(0x17, 0xb, -1.0f);
        break;
    case 7:
        DeadStop(1);
        m_PrevWeapon = m_CurrentWeapon[0];
        SetCurrentWeapon(0, weapon);
        SetWeapon(0);
        SetupMove(0x17, 0xd, -1.0f);
        break;
    case 8:
    case 9:
    case 10:
    case 11:
        if (CanAssignWeapon() && XGetGameValues()->m_bAutoEquip) {
            SetCurrentWeapon(0, weapon);
            SetWeapon(0);
        }
        break;
    }
}

void XItemHandler_Player::DoIdle()
{
    if (gCurrentMap_GeoHash == XApp::Frontend3DFile()) return;
    if (m_Health <= 0.0f) {
        KillPlayer();
        return;
    }
    if (CheckForFall()) return;
    if (!m_HoldState) {
        if (m_bIdleHitReact) {
            m_Unk224 = m_HitAngle;
            SetupMove(0, 8, -1.0f);
            m_bIdleHitReact = 0;
            return;
        }
        if (CheckForIdleBreakOut(1)) return;
        if ((m_ButtonsPressed & 0x10) && CheckForFlipGrab()) return;
    }
    if (m_SubMode >= 5 && m_SubMode <= 7 && m_PlasmaChargeTime > 0.0f) {
        SetupMove(0, 0, -1.0f);
        m_IdleTime = 0.0f;
        return;
    }
    if (CheckForIdleBreakOut(6)) return;
    if (!AnimEnded()) return;
    switch (m_SubMode) {
    case 11:
        if (m_HoldState)
            SetupMove(0x16, 1, -1.0f);
        else
            SetupMove(0, 0, -1.0f);
        return;
    case 0:
    case 1: {
        signed char b = 1;
        if (m_IdleTime < 10.0f) b = 0;
        if (gCurrentMap_GeoHash == 0x1000070) b = 0;
        if (XGetCamera(0)->IsFirstPerson()) b = 0;
        if (!(m_PlasmaChargeTime > 0.0f) && b) SetupMove(0, 5, -1.0f);
        return;
    }
    case 5: {
        if (XGetCamera(0)->IsFirstPerson()) return;
        if (g_EXRandClass.Rand32() % 21 != 10) return;
        int n;
        if (m_SkinBC == 12 || m_SkinBC == 13)
            n = 1;
        else
            n = GetCardRandom() & 1;
        switch (n) {
        case 0:
            SetupMove(0, 6, -1.0f);
            SetWeapon(3);
            break;
        case 1:
            SetupMove(0, 7, -1.0f);
            break;
        }
        return;
    }
    case 8:
    case 9:
    case 10:
        SetupMove(0, 0, -1.0f);
        return;
    case 6:
    case 7:
        SetupMove(0, 5, 31.0f);
        return;
    }
}

// NEAR (see check): the nearest openable door within reach. A plain door (flag 0x20 off) within 2.0 and
// level with the player (0.2): line up half a unit in front of it, on the side the player is on. A
// flag-0x20 door within 3.0: the door turns to the quarter facing the player, then line up 1.6 in front.
// Either starts the door move (mode 0x13).
signed char XItemHandler_Player::CheckForTOS()
{
    XItem *pDoor = DoorTest_GetNearest(m_pItem->Position(), 1000000.0f);
    if (!pDoor) return 0;
    EXItem *pMe = m_pItem;
    float dist = (pDoor->Position() - pMe->Position()).LengthXYZ();
    if ((float)fabs(pDoor->Position().y - pMe->Position().y) > 0.2f) return 0;
    float yaw;
    EXVector t;
    if (!(((XItemHandler_ScriptRunner *)pDoor->ItemHandler())->m_Flags & 0x20)) {
        if (dist > 2.0f) return 0;
        float a = (float)atan2(pDoor->Position().x - pMe->Position().x, pDoor->Position().z - pMe->Position().z);
        yaw = pDoor->Rotation().y;
        if ((float)fabs(Vec_AngleDifference(a, yaw)) > 1.5707964f) yaw += 3.1415927f;
        float s = (float)sin(yaw);
        float c = (float)cos(yaw);
        t.x = s * -0.5f + pDoor->Position().x;
        t.y = pDoor->Position().y;
        t.z = pDoor->Position().z + c * -0.5f;
        t.w = pDoor->Position().w;
    } else {
        if (dist > 3.0f) return 0;
        yaw = pDoor->Rotation().y;
        float a = (float)atan2(pDoor->Position().x - pMe->Position().x, pDoor->Position().z - pMe->Position().z);
        a = Vec_AngleDifference(a, yaw);
        for (int i = 0; i < 4; i++) {
            if (a < 0.78539819f && a > -0.78539819f) break;
            yaw = EXMakeAngleBiPolar(yaw - 1.5707964f);
            a = EXMakeAngleBiPolar(a - 1.5707964f);
        }
        float s = (float)sin(yaw);
        float c = (float)cos(yaw);
        t.x = pDoor->Position().x + s * -1.6f;
        t.y = pDoor->Position().y;
        t.z = pDoor->Position().z + c * -1.6f;
        t.w = pDoor->Position().w;
        EXVector r(pDoor->Rotation().x, yaw, pDoor->Rotation().z, pDoor->Rotation().w);
        pDoor->m_rotation = r;
        pDoor->m_validMtx = 0;
    }
    m_TargetAdjust = t;
    m_TargetAdjustType = 16.0f;
    m_TargetAdjustTime = yaw;
    m_bTargetAdjust = 1;
    m_pSpecialItem = pDoor;
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
    m_Speed.SetZero();
    m_Unk2b8.SetZero();
    pPhys->m_DeltaMove.SetZero();
    pPhys->m_Accel.y = 0.0f;
    EXPlacementItem *pItem = m_pItem;
    EXVector rot(0.0f, pItem->Rotation().y, 0.0f, 0.0f);
    pItem->m_rotation = rot;
    pItem->m_validMtx = 0;
    DeleteAllSwoosh();
    SetupMove(0x13, 0, -1.0f);
    m_ButtonsPressed &= ~8;
    m_ButtonsHeld &= ~8;
    m_ButtonsReleased &= ~8;
    return 1;
}

// NEAR (see check): within 1.5 of the nearest beacon: step to 0.01 short of it, facing it, and play the
// beacon move (7, variant 2).
signed char XItemHandler_Player::CheckForBeaconPoint()
{
    XItem *pBeacon = Beacon_GetNearest(m_pItem->Position());
    if (!pBeacon) return 0;
    const EXVector &q = m_pItem->Position();
    EXMaths::vec_xyzw t;
    t.x = pBeacon->Position().x - q.x;
    t.y = pBeacon->Position().y - q.y;
    t.z = pBeacon->Position().z - q.z;
    t.w = pBeacon->Position().w - q.w;
    EXVector d = (EXMaths::vec_xyzw)t;
    if (d.LengthXYZ() > 1.5f) return 0;
    EXMaths::vec_xyzw u;
    u.x = pBeacon->Position().x - q.x;
    u.y = pBeacon->Position().y - q.y;
    u.z = pBeacon->Position().z - q.z;
    u.w = pBeacon->Position().w - q.w;
    EXVector e = (EXMaths::vec_xyzw)u;
    float a = (float)atan2(e.x, e.z);
    EXVector p;
    float k = -0.01f;
    p.x = (float)sin(a);
    p.z = (float)cos(a);
    p.x *= k;
    p.z *= k;
    p.x = q.x + p.x;
    p.y = q.y;
    p.z = q.z + p.z;
    p.w = q.w;
    SetTargetAdjustment(p, a, 8);
    m_pSpecialItem = pBeacon;
    DeadStop(1);
    SetupMove(7, 2, -1.0f);
    m_ButtonsPressed &= ~8;
    m_CurrentSlot = -1;
    m_ButtonsHeld &= ~8;
    m_ButtonsReleased &= ~8;
    return 1;
}

// NEAR: retail does not thread the negative-length EXSqrt path past the > 2.0f test (zero kept in xmm1).
signed char XItemHandler_Player::CheckForRechargePoint()
{
    if (m_EMPTimer > 0.0f) return 0;
    XItem *pRecharge = Recharge_GetNearest(m_pItem->Position());
    if (!pRecharge) return 0;
    const EXVector &q = m_pItem->Position();
    EXMaths::vec_xyzw t;
    t.x = pRecharge->Position().x - q.x;
    t.y = pRecharge->Position().y - q.y;
    t.z = pRecharge->Position().z - q.z;
    t.w = pRecharge->Position().w - q.w;
    EXVector d = (EXMaths::vec_xyzw)t;
    if (d.LengthXYZ() > 2.0f) return 0;
    float a = pRecharge->Rotation().y;
    EXVector p;
    float k = -0.93f;
    p.x = (float)sin(a);
    p.z = (float)cos(a);
    p.x *= k;
    p.z *= k;
    p.x = pRecharge->Position().x + p.x;
    p.y = pRecharge->Position().y;
    p.z = pRecharge->Position().z + p.z;
    p.w = pRecharge->Position().w;
    SetTargetAdjustment(p, a, 8);
    m_pSpecialItem = pRecharge;
    DeadStop(1);
    SetupMove(7, 1, -1.0f);
    m_ButtonsPressed &= ~8;
    m_ButtonsHeld &= ~8;
    m_ButtonsReleased &= ~8;
    m_CurrentSlot = -1;
    if (!GameData::Instance(1)->Values()->m_Unk1c[0]) m_bUnk209 = 1;
    return 1;
}

// NEAR: retail keeps the EXSqrt zero in xmm1 and compares it (VC7.1 threads the branch); target store order.
void XItemHandler_Player::CalculateTargetJump()
{
    XItemHandler_Camera *pCam = XGetCamera(m_PlayerIndex);
    int type;
    EXItem *pItem = GetPlatformItem(type);
    if (m_Mode == 0x17) pItem = 0;
    if (m_Mode == 3 && m_SubMode != 3) pItem = 0;
    if (m_Mode != 0x15 && type == 1) m_pStandOn = (XItem *)pItem;
    if (pItem && pCam->m_Mode == 1) {
        EXVector pos;
        EXDatumTemp d;
        if ((type < 3 || type > 4) && pItem->GetAnimatorDatum(0x10000060, d, 1, 0))
            pos = d.m_position;
        else
            pos = pItem->Position();
        m_bJumpTarget = 1;
        m_JumpTarget = pos;
        EXMaths::vec_xyzw r;
        const EXVector &p = m_pItem->Position();
        r.x = p.x - m_JumpTarget.x;
        r.y = p.y - m_JumpTarget.y;
        r.z = p.z - m_JumpTarget.z;
        r.w = p.w - m_JumpTarget.w;
        EXMaths::vec_xyzw r2 = r;
        EXVector e(r2);
        EXMaths::vec_xyzw w = e;
        if (25.0f > EXSqrt(w.x * w.x + w.z * w.z)) {
            m_bJumpTargetNear = 1;
            return;
        }
    } else
        m_bJumpTarget = 0;
    m_bJumpTargetNear = 0;
}

void XItemHandler_Player::DoCrouch()
{
    m_Speed.SetZero();
    if (m_bAnimEnded) {
        if (m_SubMode == 0 || (m_SubMode > 1 && m_SubMode <= 3)) SetupMove(2, 1, -1.0f);
    }
    if (m_CrouchTimer >= 0.33333334f) {
        EXQuaternion q = m_pItem->Rotation();
        float d = Vec_AngleDifference(q.y, m_PadAngle);
        q.y += d > 0.0f ? min(d, XApp::GameFrameRateMul() * 0.04363323f) : max(d, XApp::GameFrameRateMul() * -0.04363323f);
        m_pItem->SetRotation(q);
        if (m_SubMode <= 1) {
            if (d > 0.0017453292f && m_SubMode != 3) SetupMove(2, 3, -1.0f);
            if (-0.0017453292f > d && m_SubMode != 2) SetupMove(2, 2, -1.0f);
        }
    }
    if (m_ButtonsPressed & 0x20) {
        m_CrouchTimer += XApp::FrameLength();
        CheckForFall();
        return;
    }
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
    m_Speed.SetZero();
    if (m_CrouchTimer >= 0.33333334f && (m_PredMsgFlags & 2)) {
        pPhys->SetThrustToAttainHeight(10.0f);
        SetupMove(3, 2, -1.0f);
    } else if (m_CrouchTimer >= 0.1f && (m_PredMsgFlags & 0x10)) {
        pPhys->SetThrustToAttainHeight(6.507f);
        SetupMove(3, 0, -1.0f);
    } else if (m_PredMsgFlags & 0x20) {
        pPhys->SetThrustToAttainHeight(3.217f);
        SetupMove(3, 0, -1.0f);
    } else
        SetupMove(0, 0, -1.0f);
    m_CrouchTimer = 0.0f;
    CheckForFall();
}

// NEAR: register allocation only; retail keeps 0 in ebx (null check, SetupMove args, m_SubMode > 0).
void XItemHandler_Player::DoTOS()
{
    EXItemAnimator_Anim *pAnim = (EXItemAnimator_Anim *)m_pItem->GetAnimator(EXRUNTIME_CLASS(EXItemAnimator_Anim));
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
    EXItemCtrl *pCtrl = pAnim->Controller(0);
    if (pCtrl->CtrlFlags().m_flags & 0x20000) {
        Actor::OverridePlayerCloak();
        switch (m_SubMode) {
        case 0:
            SetupMove(0x13, 1, -1.0f);
            m_TOSCharge = 0.0f;
            m_TOSPress = 0.0f;
            break;
        case 2:
            if (m_TOSCharge >= 1.6666666f) {
                SetupMove(0x13, 3, -1.0f);
                break;
            }
        case 1:
            if (m_TOSPress <= 0.0f)
                SetupMove(0x13, 1, -1.0f);
            else
                SetupMove(0x13, 2, -1.0f);
            break;
        case 4:
            SetupMove(0x13, 5, -1.0f);
            m_TOSCharge = 0.0f;
            m_TOSPress = 0.0f;
            break;
        case 5:
            if (m_TOSPress <= 0.0f)
                SetupMove(0x13, 5, -1.0f);
            else
                SetupMove(0x13, 6, -1.0f);
            break;
        case 6:
            if (m_TOSCharge >= 1.6666666f) {
                FlagChannels[0x7b] = 1;
                SetupMove(0x13, 7, -1.0f);
            } else if (m_TOSPress <= 0.0f)
                SetupMove(0x13, 5, -1.0f);
            else
                SetupMove(0x13, 6, -1.0f);
            break;
        case 3:
        case 7:
            m_pSpecialItem = 0;
            SetupMove(0, 0, -1.0f);
            return;
        }
    }
    if (m_ButtonsHeld & 8) {
        m_TOSPress = 0.5f;
        m_TOSCharge += 0.33333334f;
    }
    if (m_TOSPress > 0.0f) m_TOSPress -= XApp::FrameLength();
    if (m_TOSCharge > 0.0f) m_TOSCharge -= XApp::FrameLength();
    switch (m_SubMode) {
    case 1:
        if (m_TOSPress > 0.0f) SetupMove(0x13, 2, -1.0f);
        break;
    case 2:
        if (m_TOSCharge >= 1.6666666f) SetupMove(0x13, 3, -1.0f);
        break;
    case 5:
        if (m_TOSPress > 0.0f) SetupMove(0x13, 6, -1.0f);
        break;
    }
    if (m_SubMode > 0 && m_SubMode <= 2) {
        if (m_ButtonsHeld & 0x20) {
            m_pSpecialItem = 0;
            m_Unk344 = 0.0f;
            pPhys->SetThrustToAttainHeight(3.217f);
            SetupMove(3, 0, -1.0f);
            return;
        }
        if (m_StickMag > 0.95f && (float)fabs(Vec_AngleDifference(m_pItem->Rotation().y, m_PadAngle)) > 1.5707964f) {
            m_pSpecialItem = 0;
            SetupMove(0, 0, -1.0f);
        }
    }
}

// NEAR (see check): holding a drone by the head (mode 0x16). Sub 0 grab, 1/2 walk/idle with it, 3 run
// with it; a throw request (0x12d) rag-dolls it towards the lock-on target (within 22.5 degrees, arced by
// CalculateAngle past 5 units) or straight ahead 22.5 degrees up; jump/attack (button 8) does a finishing
// slam or neck snap on the spot, button 2 (or 8 with a lock-on) a scenery takedown: onto the camera's
// lock-on (7), a takedown point (10 / 8) or a wall (9). The finishers' end counts the kill statistics.
void XItemHandler_Player::DoGrab()
{
    Actor *pActor;
    EXItem *pItem;
    if (!m_SpecialActorID || !(pActor = Actor::GetPtrFromID(m_SpecialActorID)) || !(pItem = pActor->GetItem()))
        goto release;
    if (m_bAnimEnded) {
        m_bAnimEnded = 0;
        switch (m_SubMode) {
        case 0:
            pActor->SetAnimType(0x900024a, 0, 1);
            SetupMove(0x16, 1, 26.0f);
            m_HoldState = 1;
            return;
        case 2:
            SetupMove(0x16, 1, -1.0f);
            break;
        case 8:
            XGetGameStats()->m_Unk10[10]++;
        case 6:
            XGetGameStats()->m_Unk10[1]++;
            goto finished;
        case 10:
            XGetGameStats()->m_Unk10[11]++;
            XGetGameStats()->m_Unk10[5]++;
        case 5:
        case 7:
        case 9:
finished:
            GameData::Instance(1)->m_Stats.m_Unk10[6]++;
release:
            m_HoldState = 0;
            m_SpecialActorID = 0;
            SetupMove(0, 0, -1.0f);
            return;
        }
    }
    if ((m_SubMode == 0 || (m_SubMode > 4 && m_SubMode <= 10)) && CheckForFall()) {
        RagDollHeldDrone(0);
        return;
    }
    if (m_SubMode != 0) {
        if (m_SubMode <= 2) {
            if (MoveNormal(8)) return;
            CheckForIdleBreakOut(6);
        } else if (m_SubMode == 3) {
            if (MoveNormal(8)) return;
            if (m_StickMag == 0.0f) {
                EXPlacementItem *pMe = m_pItem;
                EXVector rot(pMe->Rotation().x, pMe->Rotation().y, 0.0f, pMe->Rotation().w);
                pMe->m_rotation = rot;
                pMe->m_validMtx = 0;
                SetupMove(0x16, 0.18f > m_MSpeedTarget ? 1 : 2, -1.0f);
                DeadStop(1);
            } else {
                if (CheckForJump(1)) return;
                if (m_SubMode != 3) SetupMove(0x16, 3, -1.0f);
            }
        }
    }
    if (m_Unk12d[0]) {
        m_Unk12d[0] = 0;
        EXDatumTemp d;
        if (!pItem->GetAnimatorDatum(0xe000009, d, 0, 0)) {
            memset(&d, 0, sizeof d);
            d.m_position = pItem->Position();
        }
        EXVector f;
        Actor *pT = GetLockOnActor();
        if (pT) {
            EXItem *pTi = pT->GetItem();
            EXItem *pMe = m_pItem;
            EXVector v = pTi->Position() - pMe->Position();
            if ((float)fabs(Vec_AngleDifference((float)atan2(v.x, v.z), pMe->Rotation().y)) < 0.39269909f) {
                EXVector w = pTi->Position() - d.m_position;
                float dist = w.x * w.x + w.z * w.z;
                if (dist >= 0.0f) dist = sqrtf(dist);
                else dist = 0.0f;
                float da = Vec_AngleDifference((float)atan2(w.x, w.z), pMe->Rotation().y);
                float c = da >= 0.39269909f ? 0.39269909f : (-0.39269909f >= da ? -0.39269909f : da);
                float yaw = pMe->Rotation().y - c;
                float pitch;
                if (5.0f > dist) pitch = 0.0f;
                else {
                    pitch = CalculateAngle(1.0909091f, dist, w.y, 0.12f, 1);
                    if (pitch > 0.78539819f) pitch = 0.78539819f;
                }
                float cp = cosf(pitch);
                f.x = cp * sinf(yaw);
                f.z = cp * cosf(yaw);
                f.y = sinf(pitch);
                goto thrown;
            }
        }
        {
            float ry = m_pItem->Rotation().y;
            f.x = (float)(sin(ry) * cos(0.39269908169872414));
            f.z = (float)(cos(ry) * cos(0.39269908169872414));
            f.y = (float)sin(0.39269908169872414);
        }
thrown:
        f.w = 0.0f;
        {
            float l = f.y * f.y + f.z * f.z + f.x * f.x;
            if (l >= 0.0f) {
                l = sqrtf(l);
                if (l > 1e-5f) {
                    l = 1.0f / l;
                    f.w = l * 0.0f;
                    f.z = l * f.z;
                    f.y = l * f.y;
                    f.x = l * f.x;
                }
            }
        }
        pActor->SetHealth(0.0f, 1);
        pActor->EnableRagDoll();
        if (m_SubMode == 9) pActor->SetHasBeenKicked(1);
        else pActor->SetHasBeenThrow(1);
        pActor->ApplyRagDollForceAll(f, 12.0f);
        RollActorRagDoll(pActor, 2);
        m_HoldState = 0;
        m_bUnkc40 = 0;
        pActor->m_SM.ExtGosub(0, STATE_RagDollDeath, 0, 0, 0, 0);
    }
    signed char bTake = 0;
    if (m_ButtonsHeld & 8) {
        if (GetLockOnActor()) bTake = 1;
        else if (m_SubMode == 1 || m_SubMode == 2) {
            if (CheckForFinishMoveSpace(1, 1.3f, 1.0f, 0.9f, 0.05f)) {
                DeadStop(1);
                EXVector off(0.63f, 0.0f, 0.16f, 0.0f), out;
                EXMatrix m;
                m.set_rotate_xyz(m_pItem->Rotation());
                D3DXVec4Transform(&out, &off, &m);
                EXVector p = m_pItem->Position();
                p.x += out.x;
                p.y = pActor->GetItem()->Position().y;
                p.z += out.z;
                p.w += out.w;
                pActor->SetPosition(p, -1);
                if (GetCardRandom() & 1) {
                    SetupMove(0x16, 6, -1.0f);
                    pActor->SetAnimType(0x9000250, 0, 1);
                    return;
                }
                SetupMove(0x16, 5, -1.0f);
                pActor->SetAnimType(0x900024e, 0, 1);
                return;
            }
            EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00576, m_pItem->Position(), -1, 0);
        }
    }
    if (((m_ButtonsHeld & 2) || bTake) && (m_SubMode == 3 || m_SubMode == 1 || m_SubMode == 2)) {
        int type;
        Actor *pL = (Actor *)XGetCamera(0)->GetLockOn(type);
        if (type == 1 && pL) {
            DeadStop(1);
            EXItem *pMe = m_pItem;
            EXVector v = pL->GetItem()->Position() - pMe->Position();
            SetTargetAdjustment(pMe->Position(), (float)atan2(v.x, v.z), 0x10);
            SetupMove(0x16, 7, -1.0f);
            pActor->SetAnimType(0x900024c, 0, 1);
            return;
        }
        int t = CheckForTakeDownScenery();
        if (t == 1) {
            DeadStop(1);
            SetupMove(0x16, 10, -1.0f);
            pActor->SetAnimType(0x90001dc, 0, 1);
        } else if (t == 3) {
            DeadStop(1);
            SetupMove(0x16, 8, -1.0f);
            pActor->SetAnimType(0x90001ef, 0, 1);
        } else if (CheckForFinishMoveSpace(0, 0.85f, 1.75f, 0.85f, 0.05f)) {
            DeadStop(1);
            SetupMove(0x16, 9, -1.0f);
            pActor->SetAnimType(0x9000290, 0, 1);
        } else {
            EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00576, m_pItem->Position(), -1, 0);
        }
    }
}

// NEAR: register allocation only; retail keeps this in esi and m_pItem in a lazily saved edi.
void XItemHandler_Player::DoHitSpecial()
{
    if (((EXItemAnimator_Anim *)m_pItem->GetAnimator(EXRUNTIME_CLASS(EXItemAnimator_Anim)))->Controller(0)->CtrlFlags().m_flags & 0x20000) {
        switch (m_SubMode) {
        case 0:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
            m_SpecialActorID = 0;
        }
        if (m_SubMode >= 13 && m_SubMode <= 15)
            SetupMove(0, 3, -1.0f);
        else
            SetupMove(0, 0, -1.0f);
        return;
    }
    switch (m_SubMode) {
    case 13:
    case 14:
    case 15:
        if (!Actor::GetPtrFromID(m_SpecialActorID)) {
            SetupMove(0, 3, -1.0f);
            m_SpecialActorID = 0;
        }
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
        if (CheckForFall()) RagDollHeldDrone(0);
        break;
    case 0:
    case 1:
    case 2:
    case 16:
    case 17:
    case 18:
    case 19:
        if (CheckForFall()) {
            SafeBreakoutHeadRip();
            m_SpecialActorID = 0;
        }
        break;
    case 12:
        if (m_bTurnToTarget) {
            m_bTurnToTarget = 0;
            Actor *pTarget = GetTargetActor();
            if (pTarget) {
                const EXVector &p = pTarget->GetFeetPos();
                EXMaths::vec_xyzw t;
                t.x = p.x - m_pItem->Position().x;
                t.y = p.y - m_pItem->Position().y;
                t.z = p.z - m_pItem->Position().z;
                t.w = p.w - m_pItem->Position().w;
                EXVector d = (EXMaths::vec_xyzw)t;
                float ang = Vec_AngleDifference(m_pItem->Rotation().y, GetRotation(d));
                float lim = XApp::GameFrameRateMul() * 0.17453292f;
                EXQuaternion q = m_pItem->Rotation();
                q.y += limit(ang, -lim, lim);
                m_pItem->SetRotation(q);
            }
        }
        if (!CheckForFall() && CheckForJump(1)) m_HandLock = -1.0f;
        break;
    }
}

// NEAR: register allocation (retail keeps the two flags in stack bytes, mine uses ebx for 0); call order and logic match.
// Move mode 6: a melee attack. The speed is zeroed every frame (attacks root the player).
void XItemHandler_Player::DoHit()
{
    signed char bHeavy = 0;
    signed char bChain = 0;
    m_Speed.SetZero();
    if (m_bAnimEnded) {
        m_bAnimEnded = 0;
        ResetAttack();
        if (m_StickMag > 0.0f)
            SetupMove(1, 0, -1.0f);
        else
            SetupMove(0, 0, -1.0f);
        return;
    }
    if (CheckForFall() || CheckForJump(1)) {
        ResetAttack();
        return;
    }
    if (m_ButtonsPressed & 0x80) {
        CheckForGrabMove(1);
        CheckForGroundAttack();
        CheckForFinishingMove(1);
    }
    if (m_bAttacking && m_StickMag > 0.95f) {
        Actor *pTarget = GetTargetActor();
        float a;
        if (pTarget) {
            EXVector d = ((EXPlacementItem *)pTarget->GetItem())->Position() - m_pItem->Position();
            a = (float)atan2(d.x, d.z);
        } else
            a = m_pItem->Rotation().y;
        if ((float)fabs(Vec_AngleDifference(a, m_PadAngle)) > 0.7853982f) {
            // pushing away from the target mid-combo: break off into a run
            ResetAttack();
            m_Speed.SetZero();
            SetupMove(1, 0, -1.0f);
            return;
        }
    }
    CheckChainButtonPress();
    if (!m_bAttacking) {
        CalculateHitMove(m_AttackType);
        m_bAttacking = 0;
        return;
    }
    switch (m_AttackHit) {
    case 1:
    case 3:
        break;
    default:
        m_bAttacking = 0;
        return;
    }
    CalculateHitType(m_CurrentWeapon[m_AttackHit], bHeavy, bChain);
    SetWeapon(m_AttackHit);
    signed char bSetTime = 1;
    if (bChain && WeaponAttackAnims[m_CurrentWeapon[m_AttackHit]][4]) {
        bSetTime = 0;
        if (1.5f > m_Unk224)
            m_AttackState = 5;
        else
            m_AttackState = (~(s8)m_AttackState & 1) | 4;
    } else {
        int s = m_AttackState;
        if (1.5f > m_Unk224) {
            if ((s == 0 || s == 2) && bHeavy) bHeavy = 0;
        } else {
            if ((s == 0 || s == 2) && !bHeavy) bHeavy = 1;
        }
        m_AttackState = s + 1;
        if (m_AttackState >= 4) {
            bSetTime = 0;
            int n = m_AttackState;
            do n -= 4; while (n >= 4);
            m_AttackState = n;
        }
    }
    m_AttackHit = -1;
    m_bAttacking = 0;
    if (bSetTime) m_Unk224 = bHeavy ? 2.0f : 1.0f;
    SetupMove(6, 0, -1.0f);
    if (m_AttackState > 3) m_Unk224 = 2.0f;
}

// NEAR: register allocation (retail: this in edi, ~1 mask in esi, actor in ebp) and fail-block placement.
signed char XItemHandler_Player::CheckForStealthKill(signed char bLockOn)
{
    m_SpecialMoveAvailable &= ~1;
    Actor *pActor;
    if (bLockOn) {
        int type;
        void *p = XGetCamera(0)->GetLockOn(type);
        if (type == 1)
            pActor = (Actor *)p;
        else
            pActor = 0;
    } else
        pActor = GetTargetActor();
    if (!pActor) {
        m_SpecialMoveAvailable &= ~1;
        return 0;
    }
    if (!(0.0f >= pActor->m_HitCountdown)) return 0;
    if (0.0f >= pActor->m_Health && !pActor->m_StateFlags.m_bCanRipBossHead) return 0;
    if (pActor->m_ActorFlags.m_bHolographic) return 0;
    if (!pActor->CanBeStealthKilled()) return 0;
    EXMaths::vec_xyzw t;
    t.x = pActor->GetFeetPos().x - m_pItem->Position().x;
    t.y = pActor->GetFeetPos().y - m_pItem->Position().y;
    t.z = pActor->GetFeetPos().z - m_pItem->Position().z;
    t.w = pActor->GetFeetPos().w - m_pItem->Position().w;
    EXVector d = (EXMaths::vec_xyzw)t;
    if ((float)fabs(d.y) > 0.2f) return 0;
    if (d.LengthXYZ() > 2.0f) return 0;
    float ry = pActor->GetRotY();
    if ((float)fabs(Vec_AngleDifference(m_pItem->Rotation().y, ry)) > 0.7853982f) return 0;
    float a = GetRotation(d);
    if ((float)fabs(Vec_AngleDifference(ry, a)) > 0.7853982f) return 0;
    EXVector p;
    float k = -1.5f;
    p.x = (float)sin(a);
    p.z = (float)cos(a);
    p.x *= k;
    p.z *= k;
    p.x = pActor->GetFeetPos().x + p.x;
    p.y = pActor->GetFeetPos().y;
    p.z = pActor->GetFeetPos().z + p.z;
    p.w = pActor->GetFeetPos().w;
    if (!CheckForStandingSpace(p, 1.5f)) return 0;
    m_GroundAttackID = pActor->m_RefID;
    m_SpecialMoveAvailable |= 1;
    return 1;
}

void XItemHandler_Player::AdjustTargetAdjustment()
{
    float f;
    m_TargetAdjustType -= XApp::GameFrameRateMul();
    if (m_TargetAdjustType <= 0.0f) {
        f = 1.0f;
        m_TargetAdjustType = 0.0f;
        m_bTargetAdjust = 0;
    } else
        f = (m_TargetAdjustType + 1.0f) * XApp::GameFrameRateDiv();
    const EXVector &p = m_pItem->Position();
    EXMaths::vec_xyzw r;
    r.x = m_TargetAdjust.x - p.x;
    r.y = m_TargetAdjust.y - p.y;
    r.z = m_TargetAdjust.z - p.z;
    r.w = m_TargetAdjust.w - p.w;
    EXMaths::vec_xyzw r2 = r;
    m_TargetDelta = r2;
    m_TargetAngle = Vec_AngleDifference(m_pItem->Rotation().y, m_TargetAdjustTime);
    float k = 1.0f / f;
    m_TargetDelta *= k;
    m_TargetAngle *= k;
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
    EXQuaternion q = m_pItem->Rotation();
    q.y += m_TargetAngle;
    m_pItem->SetRotation(q);
    m_TargetDelta.w = 0.0f;
    m_TargetDelta.y = 0.0f;
    pPhys->AddPFDeltaMove(m_TargetDelta);
}

signed char XItemHandler_Player::CheckCollision(EXLineSeg &seg, EXVector &out, float &t)
{
    EXDatumTemp d;
    EXSphere s;
    EXCapsule cap;
    EXMaths::vec_xyzw c;
    EXMaths::vec_xyzw a, a2, b, b2;
    float ta = 0.0f;
    float tb = 0.0f;
    if (m_pItem->GetAnimatorDatum(CollisionHash[0], d, 0, 0)) {
        d.Get(s);
        c = s.m_centre;
        if (EXSqrt(EXSqrDistance(c, seg, &ta)) > s.m_radius) return 0;
    }
    for (int i = 1; i < 11; i++) {
        if (m_pItem->GetAnimatorDatum(CollisionHash[i], d, 0, 0)) {
            d.Get(cap);
            if (cap.Radius() >= EXSqrt(EXSqrDistance(cap.Segment(), seg, &ta, &tb))) {
                a.x = seg.m_Direction.x * tb;
                a.y = seg.m_Direction.y * tb;
                a.z = seg.m_Direction.z * tb;
                a.w = seg.m_Direction.w * tb;
                a2 = a;
                b.x = seg.m_Origin.x + a2.x;
                b.y = seg.m_Origin.y + a2.y;
                b.z = seg.m_Origin.z + a2.z;
                b.w = seg.m_Origin.w + a2.w;
                b2 = b;
                out = b2;
                t = tb;
                return 1;
            }
        }
    }
    return 0;
}

void XItemHandler_Player::DoReact()
{
    float len, r;
    if (m_bAnimEnded) {
        m_bAnimEnded = 0;
        switch (m_SubMode) {
        case 14:
        case 15:
            m_ReactTimer = 0.5f;
            if (m_HoldState)
                SetupMove(0x16, 1, -1.0f);
            else
                SetupMove(0, 0, -1.0f);
            return;
        case 0:
            SetupMove(0x17, 1, -1.0f);
            break;
        case 4:
            SetupMove(0x17, 5, -1.0f);
            break;
        case 2:
        case 6:
            m_ReactTimer = 0.5f;
            SetupMove(0, 0, -1.0f);
            return;
        case 9:
        case 10:
            SetupMove(0, 0, -1.0f);
            return;
        case 11:
        case 12:
        case 13:
            RestorePreviousWeapon();
            SetupMove(0, 0, -1.0f);
            return;
        case 3:
        case 7:
        case 8:
            theApp.m_MissionState = 4;
            theApp.m_MissionTimer = 0.0f;
            break;
        }
    }
    if (m_ReactRecover > 0.0f) {
        switch (m_SubMode) {
        case 9:
        case 10:
            SetupMove(0, 0, -1.0f);
            return;
        case 11:
        case 12:
        case 13:
            RestorePreviousWeapon();
            SetupMove(0, 0, -1.0f);
            return;
        }
    }
    switch (m_SubMode) {
    case 0:
    case 4:
        MoveJump();
        return;
    case 1:
    case 5:
        if (CheckDeadFall()) return;
        MoveJump();
        if (((XItemPhysics *)m_pItem->m_pItemPhysics)->CollisionFlags() & 1) {
            if (m_Health <= 0.0f) {
                if (m_SubMode == 1)
                    SetupMove(0x17, 3, -1.0f);
                else
                    SetupMove(0x17, 7, -1.0f);
            } else if (m_SubMode == 1)
                SetupMove(0x17, 2, -1.0f);
            else
                SetupMove(0x17, 6, -1.0f);
            m_Unk2b8.SetZero();
        }
        return;
    case 2:
    case 3:
    case 6:
    case 7: {
        if (CheckForFall()) return;
        EXMaths::vec_xyzw v = m_Speed;
        float sq = v.x * v.x + v.z * v.z;
        len = sq >= 0.0f ? (float)sqrt(sq) : 0.0f;
        r = PlayerMax(0.0f, len - XApp::GameFrameRateMul() * 0.05f);
        if (r > 0.001f)
            m_Speed *= r / len;
        else
            m_Speed.SetZero();
        return;
    }
    case 9:
    case 10:
    case 14:
    case 15:
        CheckForFall();
        return;
    case 11:
    case 12:
    case 13:
        if (CheckForJump(0)) {
            RestorePreviousWeapon();
            return;
        }
        if (m_ButtonsHeld & 8) {
            RestorePreviousWeapon();
            ButtonUsed(8);
            SetupMove(0, 0, -1.0f);
        }
        return;
    }
}

// Reads this frame's pad state: analogue stick (m_Pad, magnitude m_StickMag, camera-relative
// direction m_PadAngle) and the action bits (masked by m_ButtonMask / m_ButtonLock).
void XItemHandler_Player::GetPadData()
{
    EXBaseGamePad *pPad = EXBaseGamePad::GetGamePad();
    pPad->SetDeadZoneMode(m_PadIndex, (EXPad::DeadZoneMode)1);
    EXVector2 stick;
    if (m_bPlayerHasControl && m_Health > 0.0f) {
        stick.x = pPad->Analog(m_PadIndex, (EXPad::AnalogChan)0);
        stick.y = 0.0f - pPad->Analog(m_PadIndex, (EXPad::AnalogChan)1);
        if (m_AnalogMask & 1) stick.x = 0.0f;
        if (m_AnalogMask & 2) stick.y = 0.0f;
    } else {
        stick.SetZero();
    }
    m_Pad.z = stick.y;
    m_Pad.x = stick.x;
    m_Pad.y = 0.0f;
    m_Pad.w = 0.0f;
    m_StickMag = EXSqrt(stick.x * stick.x + stick.y * stick.y);
    m_StickMag = min(m_StickMag, 1.0f);
    if (m_StickMag != 0.0f) m_StickAngle = (float)atan2(stick.x, stick.y);
    if (GameData::Instance(1)->Values()->m_CameraMode != 2 && !m_bForceTrackCam) {
        if (m_StickMag != 0.0f)
            m_PadAngle = XGetCamera(m_PlayerIndex)->Angle() + m_StickAngle;
        else
            m_PadAngle = m_pItem->Rotation().y;
        m_PrevStickAngle = m_StickAngle;
        m_CamAngle = XGetCamera(0)->Angle();
    } else {
        m_bForceTrackCam = 0;
        XItemHandler_Camera *pCam = XGetCamera(m_PlayerIndex);
        if (m_StickMag != 0.0f) {
            float camAngle = pCam->Angle();
            float d = Vec_AngleDifference(camAngle, m_CamAngle);
            float lim = (float)fabs(Vec_AngleDifference(m_PrevStickAngle, m_StickAngle)) * 6.73f;
            lim = lim * XApp::GameFrameRateMul() * 0.18f;
            float lo = -lim;
            d = limit(d, lo, lim);
            m_CamAngle -= d;
            m_PrevStickAngle = m_StickAngle;
            m_PadAngle = m_StickAngle + m_CamAngle;
        } else {
            m_CamAngle = pCam->Angle();
            m_PadAngle = m_pItem->Rotation().y;
        }
    }
    if (m_bPlayerHasControl && m_Health > 0.0f) {
        m_ButtonsPressed = pPad->Actions(m_PadIndex);
        m_ButtonsHeld = pPad->ActionPressed(m_PadIndex);
        m_ButtonsReleased = pPad->ActionReleased(m_PadIndex);
    } else {
        m_ButtonsReleased = 0;
        m_ButtonsHeld = 0;
        m_ButtonsPressed = 0;
    }
    m_ButtonLock &= m_ButtonsPressed;
    if (m_AnalogLock & 1) {
        if (m_Pad.z <= 0.0f)
            m_AnalogLock &= ~1;
        else
            m_Pad.z = 0.0f;
    }
    unsigned int mask = m_ButtonMask | m_ButtonLock;
    m_ButtonsPressed &= ~mask;
    m_ButtonsHeld &= ~mask;
    m_ButtonsReleased &= ~mask;
}

// EXVector::operator- that VC7.1 will not inline in large functions here (retail inlined it).
static __forceinline EXVector PlayerVecSub(const EXVector &a, const EXVector &b)
{
    return EXVector(a.EXMaths::vec_xyzw::operator-(b));
}

// Grab move availability: the target (camera lock-on, or the camera's target actor) must be
// grabbable, level with us, within 3m, facing away (behind it) with standing room 1.2m behind it.
// Sets bit 2 of m_SpecialMoveAvailable and m_GroundAttackID when available.
// NEAR: retail copies the difference vector once more (SSE copy back into the first temp's slot)
// before the distance tests, and schedules the sin/cos stores differently.
signed char XItemHandler_Player::CheckForGrabMove(signed char bLockOn)
{
    m_SpecialMoveAvailable &= ~2;
    Actor *pActor = 0;
    if (bLockOn) {
        int type;
        Actor *pLock = (Actor *)XGetCamera(0)->GetLockOn(type);
        if (type != 1) return 0;
        if (!pLock) return 0;
        pActor = pLock;
        if (m_CurrentWeapon[3]) return 0;
        if (!pLock->m_ActorFlags2.m_bDazed && !pLock->IsNegativeOrZeroThreat() &&
            pLock->NearToRecentLastKnownLocation())
            return 0;
    } else {
        int id = XGetCamera(m_PlayerIndex)->GetTargetID();
        if (id > 0) {
            Actor *p = Actor::GetPtrFromID(id);
            if (p) pActor = p;
        }
    }
    if (pActor) {
        if (!pActor->CanBeGrabbed()) return 0;
        const EXVector &p = pActor->GetItem()->Position();
        const EXVector &q = m_pItem->Position();
        EXVector d = PlayerVecSub(p, q);
        if ((float)fabs(d.y) > 0.2f) return 0;
        if (d.LengthXYZ() > 3.0f) return 0;
        if ((float)fabs(Vec_AngleDifference(m_pItem->Rotation().y, pActor->GetItem()->Rotation().y + 3.1415927f)) >
            2.3561945f)
            return 0;
        float a = m_pItem->Rotation().y;
        float k = -1.2f;
        EXVector pos;
        pos.x = (float)sin(a);
        pos.z = (float)cos(a);
        pos.x *= k;
        pos.z *= k;
        pos.x = p.x + pos.x;
        pos.y = p.y;
        pos.z = p.z + pos.z;
        pos.w = p.w;
        if (!CheckForStandingSpace(pos, 1.5f)) return 0;
        m_GroundAttackID = pActor->m_RefID;
        m_SpecialMoveAvailable |= 2;
        return 1;
    }
    m_SpecialMoveAvailable &= ~2;
    return 0;
}

// NEAR (see check): the stealth kill found by CheckForStealthKill: step to 1.5 behind the victim facing
// it and play one of four random kills (bare-handed: moves 4, 6, 10, 11; armed: 5, 7, 8, 9) with its
// attack message (0x28-0x2f); most count towards a per-kill-type stat, all towards the stealth kill total.
signed char XItemHandler_Player::DoStealthKill(signed char b)
{
    if (!CheckForStealthKill(b)) return 0;
    Actor *pActor = Actor::GetPtrFromID(m_GroundAttackID);
    if (!pActor) return 0;
    EXItem *pThem = pActor->GetItem();
    EXVector d = pThem->Position() - m_pItem->Position();
    float a = (float)atan2(d.x, d.z);
    float s = (float)sin(a);
    float c = (float)cos(a);
    m_TargetAdjust.z = pThem->Position().z + c * -1.5f;
    m_TargetAdjust.x = pThem->Position().x + s * -1.5f;
    m_TargetAdjust.y = pThem->Position().y;
    m_TargetAdjust.w = pThem->Position().w;
    m_TargetAdjustTime = a;
    m_bTargetAdjust = 1;
    m_TargetAdjustType = 8.0f;
    m_SpecialActorID = m_GroundAttackID;
    m_bUnkc40 = 0;
    m_CurrentSlot = -1;
    int r = GetCardRandom();
    if (m_CurrentWeapon[3] == 0) {
        switch (r) {
        case 1: {
            SetupMove(7, 6, -1.0f);
            EXVector v2(0.0f, a, 0.0f, 0.0f), v1(0.0f, 0.0f, 0.0f, 0.0f);
            SendMsg_AttackedByPredator(Actor::GetPtrFromID(m_SpecialActorID), 0x2a, v1, v2, 0.0f, 0);
            XGetGameStats()->m_Unk10[5]++;
            break;
        }
        case 2: {
            SetupMove(7, 10, -1.0f);
            EXVector v2(0.0f, a, 0.0f, 0.0f), v1(0.0f, 0.0f, 0.0f, 0.0f);
            SendMsg_AttackedByPredator(Actor::GetPtrFromID(m_SpecialActorID), 0x2e, v1, v2, 0.0f, 0);
            XGetGameStats()->m_Unk10[1]++;
            break;
        }
        case 3: {
            SetupMove(7, 0xb, -1.0f);
            EXVector v2(0.0f, a, 0.0f, 0.0f), v1(0.0f, 0.0f, 0.0f, 0.0f);
            SendMsg_AttackedByPredator(Actor::GetPtrFromID(m_SpecialActorID), 0x2f, v1, v2, 0.0f, 0);
            break;
        }
        default: {
            SetupMove(7, 4, -1.0f);
            EXVector v2(0.0f, a, 0.0f, 0.0f), v1(0.0f, 0.0f, 0.0f, 0.0f);
            SendMsg_AttackedByPredator(Actor::GetPtrFromID(m_SpecialActorID), 0x28, v1, v2, 0.0f, 0);
            XGetGameStats()->m_Unk10[4]++;
            break;
        }
        }
    } else {
        switch (r) {
        case 1: {
            SetupMove(7, 7, -1.0f);
            EXVector v2(0.0f, a, 0.0f, 0.0f), v1(0.0f, 0.0f, 0.0f, 0.0f);
            SendMsg_AttackedByPredator(Actor::GetPtrFromID(m_SpecialActorID), 0x2b, v1, v2, 0.0f, 0);
            XGetGameStats()->m_Unk10[2]++;
            break;
        }
        case 2: {
            SetupMove(7, 8, -1.0f);
            EXVector v2(0.0f, a, 0.0f, 0.0f), v1(0.0f, 0.0f, 0.0f, 0.0f);
            SendMsg_AttackedByPredator(Actor::GetPtrFromID(m_SpecialActorID), 0x2c, v1, v2, 0.0f, 0);
            break;
        }
        case 3: {
            SetupMove(7, 9, -1.0f);
            EXVector v2(0.0f, a, 0.0f, 0.0f), v1(0.0f, 0.0f, 0.0f, 0.0f);
            SendMsg_AttackedByPredator(Actor::GetPtrFromID(m_SpecialActorID), 0x2d, v1, v2, 0.0f, 0);
            XGetGameStats()->m_Unk10[1]++;
            break;
        }
        default: {
            SetupMove(7, 5, -1.0f);
            EXVector v2(0.0f, a, 0.0f, 0.0f), v1(0.0f, 0.0f, 0.0f, 0.0f);
            SendMsg_AttackedByPredator(Actor::GetPtrFromID(m_SpecialActorID), 0x29, v1, v2, 0.0f, 0);
            XGetGameStats()->m_Unk10[3]++;
            break;
        }
        }
    }
    DeadStop(1);
    m_SpecialMoveAvailable = 0;
    GameData::Instance(1)->m_Stats.m_Unk10[0]++;
    return 1;
}

// Starts a ground attack (stomp/stab on a floored actor) with weapon slot `type` if
// CheckForGroundAttack found a target: boss head rips need type 3 and step in 0.9m behind the
// boss; otherwise the player is moved 2.22m from the victim (wrist blade types 2..4 for type 1).
// NEAR: register/slot choice in the sin/cos target position (retail adds x after the cos store and
// keeps the position in the difference vector's stack slot).
signed char XItemHandler_Player::DoGroundAttack(int type)
{
    if (!CheckForGroundAttack()) return 0;
    Actor *pActor = Actor::GetPtrFromID(m_GroundAttackID);
    if (!pActor) return 0;
    EXMaths::vec_xyzw t;
    EXItem *pItem = pActor->GetItem();
    const EXVector &q = m_pItem->Position();
    t.x = pItem->Position().x - q.x;
    t.y = pItem->Position().y - q.y;
    t.z = pItem->Position().z - q.z;
    t.w = pItem->Position().w - q.w;
    EXVector d = (EXMaths::vec_xyzw)t;
    float rot = m_pItem->Rotation().y;
    float diff = Vec_AngleDifference(rot, GetRotation(d));
    if (pActor->m_StateFlags.m_bCanRipBossHead) {
        if (type != 3) return 0;
        rot = diff + rot;
        float k = -0.9f;
        EXVector pos;
        pos.x = (float)sin(rot);
        pos.z = (float)cos(rot);
        pos.x *= k;
        pos.z *= k;
        pos.x = pItem->Position().x + pos.x;
        pos.y = pItem->Position().y;
        pos.z = pItem->Position().z + pos.z;
        pos.w = pItem->Position().w;
        m_TargetAdjust.x = pos.x;
        m_TargetAdjust.y = pos.y;
        m_TargetAdjust.z = pos.z;
        m_TargetAdjust.w = pos.w;
        m_TargetAdjustTime = rot - 0.06981317f;
        SetupMove(7, 0, -1.0f);
        SendMsg_AttackedByPredator(pActor, 0x30, EXVector(0.0f, 0.0f, 0.0f, 0.0f), EXVector(0.0f, 0.0f, 0.0f, 0.0f), 0.0f, 0);
        XGetGameStats()->m_Unk10[2]++;
    } else {
        if (type == 1 && ((int)m_CurrentWeapon[1] < 2 || (int)m_CurrentWeapon[1] > 4)) return 0;
        rot = diff + rot;
        float k = -2.22f;
        EXVector pos;
        pos.x = (float)sin(rot);
        pos.z = (float)cos(rot);
        pos.x *= k;
        pos.z *= k;
        pos.x = pItem->Position().x + pos.x;
        pos.y = pItem->Position().y;
        pos.z = pItem->Position().z + pos.z;
        pos.w = pItem->Position().w;
        SetTargetAdjustment(pos, rot, 16);
        pActor->SetNotAllowedToGetUp();
        if (type == 3) {
            SetupMove(7, 0x13, -1.0f);
        } else {
            switch (m_CurrentWeapon[1]) {
            case 2: SetupMove(7, 0x10, -1.0f); break;
            case 3: SetupMove(7, 0x11, -1.0f); break;
            case 4: SetupMove(7, 0x12, -1.0f); break;
            }
        }
    }
    m_SpecialActorID = m_GroundAttackID;
    m_bUnkc40 = 0;
    DeadStop(1);
    SetWeapon(type);
    m_SpecialMoveAvailable = 0;
    return 1;
}

// End-of-frame movement bookkeeping: frame delta (m_Delta) and horizontal speed from the position
// change, idle timer (counts while not moving), both scaled to the physics frame rate, and the
// smoothed movement speed m_MSpeed eased toward m_MSpeedTarget.
// NEAR: 1 instruction, retail loads XApp::m_FrameLength first and adds m_IdleTime from memory.
void XItemHandler_Player::PostUpdatePlayer()
{
    EXMaths::vec_xyzw t;
    const EXVector &p = m_pItem->Position();
    t.x = p.x - m_LastPos.x;
    t.y = p.y - m_LastPos.y;
    t.z = p.z - m_LastPos.z;
    t.w = p.w - m_LastPos.w;
    EXVector d = (EXMaths::vec_xyzw)t;
    m_Delta = d;
    m_MSpeedTarget = m_Delta.LengthXZ();
    m_LastPos = m_pItem->Position();
    float tmp6 = m_Delta.LengthXYZSquared();
if (tmp6 < 0.0001f)
        m_IdleTime += XApp::FrameLength();
    else
        m_IdleTime = 0.0f;
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
    m_MSpeedTarget = pPhys->m_FrameRateDiv * m_MSpeedTarget;
    m_Delta *= pPhys->m_FrameRateDiv;
    m_MSpeed = Interp(m_MSpeed, m_MSpeedTarget, 0.25f, XApp::GameFrameRateMul());
}

// NEAR: written from the disassembly for behaviour (DIFF: register allocation and a larger frame).
// The player's per-frame update (called from DoUpdate): timers, cloak refraction and energy
// recharge, damage from being hit by a car or a flying vehicle, weapon buttons, the pad, the
// handler for the current m_Mode, then the movement for the frame. Most modes move by the
// animation's root motion (rotated by the facing); the free-moving ones (idle, run, jump...)
// move along m_Speed at InertiaToRunSpeed, tilted onto the floor; skid (0xe) also turns by the
// animation's rotation.
signed char XItemHandler_Player::UpdatePlayer()
{
    m_UpdateCount++;
    HandleDamageSkins();
    if (!theApp.m_bCutscene && m_bPlayerHasControl && !XGetCamera(0)->IsFirstPerson()) {
        m_bHidden = 0;
        m_pItem->SetHasRender();
    }
    if (!((EXItemAnimator_Anim *)m_pItem->GetAnimator(EXRUNTIME_CLASS(EXItemAnimator_Anim)))->Controller(1)) {
        m_Unk214 = 0xffffffff;
    } else {
        ((EXItemAnimator_Anim *)m_pItem->GetAnimator(EXRUNTIME_CLASS(EXItemAnimator_Anim)))->ControllerStack(1).m_blend = m_MoveBlend;
        m_MoveBlend = limit<float>(m_MoveBlend + m_Unk24c * XApp::GameFrameRateMul(), 0.0f, 1.0f);
    }
    if (m_ComboTimer > 0.0f) {
        m_ComboTimer -= XApp::FrameLength();
        if (m_ComboTimer <= 0.0f) ResetCombo();
    }
    if (m_Unk1ec > 0.0f) m_Unk1ec -= XApp::FrameLength();
    if (m_EMPTimer > 0.0f && !theApp.m_bCutscene) {
        m_EMPTimer -= XApp::FrameLength();
        if (m_EMPTimer <= 0.0f) m_Energy = m_SavedEnergy;
    }
    float target;
    if (m_RefractionBoost > 0.0f) {
        m_RefractionBoost -= XApp::FrameLength();
        target = 4.0f;
    } else {
        target = m_MSpeedTarget * 16.0f + 0.4f;
    }
    float step = XApp::GameFrameRateMul() * 0.1f;
    m_CloakRefraction -= limit<float>(m_CloakRefraction - target, step * -0.5f, step);
    if (m_ReactTimer > 0.0f) m_ReactTimer -= XApp::FrameLength();
    if (m_ReactRecover > 0.0f) {
        m_ReactRecover -= XApp::FrameLength();
        if (m_ReactRecover <= 0.0f) m_bIdleHitReact = 0;
    }
    if (m_Unk358 > 0.0f) m_Unk358 -= XApp::FrameLength();
    if (m_PickupTimer > 0.0f) {
        m_PickupTimer -= XApp::FrameLength();
        if (m_PickupTimer <= 0.0f) {
            if (m_PickupWeapon > 0) ReactWeaponPickup(m_PickupWeapon, 1);
            m_PickupWeapon = 0;
        }
    }
    if (m_bUnk209 && 20.0f > m_Energy && !m_bCloak) m_Energy += XApp::GameFrameRateMul() * 0.01f;
    if (m_VMSound && !m_SoundTag.IsPlaying(m_VMSound)) {
        FEHud::Instance(1)->CloseMimicry();
        m_VMSound = 0;
    }
    EXItemAnimator_Anim *pAnim = (EXItemAnimator_Anim *)m_pItem->GetAnimator(EXRUNTIME_CLASS(EXItemAnimator_Anim));
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
    if (m_PickupWeapon > 0) ReactWeaponPickup(m_PickupWeapon, 0);

    // Run over by a car: a hard hit knocks the player down, a softer one makes him stumble.
    Vehicle *pVeh = pPhys->m_pHitVehicle;
    if (pVeh && m_Mode != 0x15) {
        if (pVeh->m_vehFlags & 1) Objective_NotifyPlayerSeen(pVeh);
        if (m_Mode != 0x17 && m_Mode != 9) {
            EXVector rel = pPhys->m_HitVehicleVel * 0.033333335f - m_Delta;
            float spd = rel.LengthXZ() * 30.0f;
            float dmg = limit<float>(spd, 0.0f, 10.0f);
            m_ReactTimer = 0.0f;
            if (spd > 9.0f) {
                EXVector from = pVeh->m_pItem->Position();
                EXBaseSoundManager::GetSoundManager()->StartSound(0x1af001fb, m_pItem->Position(), -1, 0);
                ReactExplode(from, limit<float>((spd - 9.0f) * 0.25f, 0.0f, 0.75f) + 0.25f);
                InflictDamage(dmg, 2);
            } else if (spd > 6.0f) {
                EXVector from = pVeh->m_pItem->Position();
                EXBaseSoundManager::GetSoundManager()->StartSound(0x1af001fb, m_pItem->Position(), -1, 0);
                ReactStumble(from);
                InflictDamage(dmg, 1);
            }
        }
    }
    // Hit by a flying vehicle: always a knockdown.
    XItem *pFly = pPhys->m_pHitFlyingVehicle;
    if (pFly && m_Mode != 0x17) {
        EXVector rel = pPhys->m_FlyingVehicleVel * 0.033333335f - m_Delta;
        float spd = rel.LengthXZ() * 30.0f;
        float dmg = limit<float>(spd, 0.0f, 10.0f);
        m_ReactTimer = 0.0f;
        EXVector from = pFly->Position();
        EXBaseSoundManager::GetSoundManager()->StartSound(0x1af001fb, m_pItem->Position(), -1, 0);
        ReactExplode(from, limit<float>((spd - 9.0f) * 0.25f, 0.0f, 0.75f) + 0.25f);
        InflictDamage(dmg, 2);
    }

    HandleWeaponButtons();
    EXMatrix mtx = EXMatrix::MakeIdentity();
    EXVector ang(0.0f, 0.0f, 0.0f, 0.0f);
    gpGameWnd[0]->DbgPrint("\n\n\n\n\n");
    m_SpecialMoveAvailable = 0;
    m_pJustOffCar = 0;
    if (m_Unk35c > 0.0f) m_Unk35c -= XApp::FrameLength();
    GetPadData();
    switch (m_Mode) {
    case 0: DoIdle(); break;
    case 1: DoMove(); break;
    case 2: DoCrouch(); break;
    case 3:
    case 9: DoJump(); break;
    case 4: DoClamber(); break;
    case 6: DoHit(); break;
    case 7: DoHitSpecial(); break;
    case 8: DoLand(); break;
    case 0xa: DoWallJump(); break;
    case 0xc: DoHang(); break;
    case 0xe: DoSkid(); break;
    case 0x10: DoPreJump(); break;
    case 0x11: DoMonkey(); break;
    case 0x12: DoPerch(); break;
    case 0x13: DoTOS(); break;
    case 0x14: DoUlysses(); break;
    case 0x15: DoCarRide(); break;
    case 0x16: DoGrab(); break;
    case 0x17: DoReact(); break;
    case 0x18: DoGecko(); break;
    }

    EXMotionData md;
    EXVector out;
    if (m_Mode != 4 && !(m_MoveFlags & 0x20) && !(m_Mode == 6 && m_Unk224 > 1.5f)) {
        if (m_Mode == 0xe) {
            // Skid: the animation turns the player as well as moving him.
            pAnim->CalcMotionData(md, 3, 2);
            md.m_pos *= XApp::GameFrameRateDiv();
            md.m_rot.GetRotation(ang, 4);
            ang = EXVector(m_pItem->m_rotation.x, m_pItem->m_rotation.y + ang.y, m_pItem->m_rotation.z, m_pItem->m_rotation.w);
            m_pItem->m_rotation = ang;
            m_pItem->m_validMtx = 0;
            mtx.set_rotate_y(m_pItem->m_rotation.y);
            out = EXVector(mtx.EXMaths::mat_44::operator*(md.m_pos));
            pPhys->m_DeltaMove += out;
            return 1;
        }
        if (m_Mode != 0xc && m_Mode != 0x11) {
            // Free movement along the current speed, at run speed for that inertia, on the floor plane.
            EXVector dir;
            if (m_MoveFlags & 0x80) dir = m_Unk2b8;
            else dir = m_Speed.UnitVector();
            dir *= InertiaToRunSpeed(GetSpeed());
            EXVector up = m_FloorNormal.UnitVector();
            mtx.GetRotationTo(EXVector(0.0f, 1.0f, 0.0f, 0.0f), up);
            if (m_bTargetAdjust) {
                AdjustTargetAdjustment();
                dir.z = 0.0f;
                dir.x = 0.0f;
            }
            dir.w = 0.0f;
            dir.y = 0.0f;
            out = EXVector(mtx.EXMaths::mat_44::operator*(dir));
            if (out.y > 0.0f) out.y = 0.0f;
            pPhys->m_DeltaMove += out;
            MoveCheckForActor();
            MoveCheckForSticky();
            return 1;
        }
    }
    // Root motion from the animation, rotated by the facing.
    mtx.set_rotate_y(m_pItem->m_rotation.y);
    if (m_Mode == 4) {
        // Clamber: follow the animation's total motion since the move began, from where it began.
        pAnim->CalcMotionData(md, 2, 0);
        out = EXVector(mtx.EXMaths::mat_44::operator*(md.m_pos));
        if (m_bForceModeChange) {
            m_bForceModeChange = 0;
            m_RootMotionStart = out;
            m_RootPosStart = m_pItem->Position();
            out = EXVector(0.0f, 0.0f, 0.0f, 0.0f);
        } else {
            out = (m_RootPosStart + (out - m_RootMotionStart) - m_pItem->Position()) * XApp::GameFrameRateDiv();
        }
    } else {
        pAnim->CalcMotionData(md, 3, 0);
        md.m_pos *= XApp::GameFrameRateDiv();
        if (m_Mode == 6) md.m_pos *= m_HitMoveScale;
        out = EXVector(mtx.EXMaths::mat_44::operator*(md.m_pos));
    }
    if (m_bTargetAdjust) AdjustTargetAdjustment();
    if (m_MoveFlags & 0x40) {
        pPhys->m_DeltaMove.x = out.x;
        pPhys->m_DeltaMove.z = out.z;
    } else {
        pPhys->m_DeltaMove = out;
    }
    MoveCheckForActor();
    MoveCheckForSticky();
    return 1;
}

// NEAR (see check; retail calls the GetMaxHealth inline out of line here, 56 vs our 55 calls): the player's
// animation script events: hit windows, shots, combo marks, footsteps,
// drone attack messages (the jab on a held drone, the slam and the shoulder-blade kill), swooshes on the
// hands or the wrist blade, the end-of-move mark, weapon equips, auto turn, PRED_Special actions (roar,
// recharge/beacon use, throw request, drone rag-doll, wall-jump push, head rips, scenery damage, medikit),
// hand locks, spark skin, move adjust, upper-body blend, FX scripts and rumble.
unsigned int XItemHandler_Player::DoScriptCmdEvent(GEOSCRIPT_EVENT *pEvent, EXItemAnimator *pAnim)
{
    EXItem *pItem = m_pItem;
    XItemPhysics *pPhys = (XItemPhysics *)pItem->m_pItemPhysics;
    m_Unkd4 = 1;
    switch (pEvent->m_Hash) {
    case 0x16000003:
        m_HitEventFlags |= pEvent->m_Param[0].u;
        break;
    case 0x16000006:
        if (pEvent->m_Param[0].i == 0) ShootWeapon(m_AttackType, 0);
        else if (pEvent->m_Param[0].i == 1) ShootWeapon(m_ThrowWeapon, 1);
        break;
    case 0x16000007:
        m_bAttacking = 1;
        break;
    case 0x16000008:
        m_bAttacking = 0;
        break;
    case 0x1600000c:
        m_ShimmyDir = 1.0f;
        FootStepEvent(1);
        break;
    case 0x1600000d:
        m_ShimmyDir = 2.0f;
        FootStepEvent(0);
        break;
    case 0x16000012: {
        int msg;
        float dmg;
        EXVector pos, dir;
        switch (pEvent->m_Param[0].i) {
        case 3: {
            EXDatumTemp d;
            if (pItem->GetAnimatorDatum(0x1000001a, d, 0, 0)) {
                XItemHandler_ScriptBase::FX_Create(d.m_position, 0x40004a2, 0x40, 0x1000016);
                if (m_SpecialActorID) {
                    EXVector z(0.0f, 0.0f, 0.0f, 0.0f);
                    SendMsg_AttackedByPredator(Actor::GetPtrFromID(m_SpecialActorID), 0, d.m_position, z, 500.0f, 0);
                } else if (getItemOwnerVehicle(m_pStandOn)) {
                    m_bDamageStandOn = 1;
                }
            }
            goto done;
        }
        case 4:
            if (m_CurrentWeapon[1] == 3) msg = 0x23;
            else if (m_CurrentWeapon[1] == 4) msg = 0x24;
            else msg = 0x22;
            dir = EXVector(0.0f, 0.0f, 0.0f, 0.0f);
            pos = EXVector(0.0f, 0.0f, 0.0f, 0.0f);
            dmg = 500000.0f;
            break;
        case 5:
            if (m_CurrentWeapon[3] == 1) {
                msg = 0x26;
                dmg = 20.0f;
                if (m_Unkcf0[1] == 1) dmg = 30.0f;
                else if (m_Unkcf0[1] == 2) dmg = 40.0f;
            } else {
                msg = 0x25;
                dmg = 10.0f;
            }
            dir = EXVector(0.0f, 0.0f, 0.0f, 0.0f);
            pos = EXVector(0.0f, 0.0f, 0.0f, 0.0f);
            break;
        default:
            goto done;
        }
        SendMsg_AttackedByPredator(Actor::GetPtrFromID(m_SpecialActorID), msg, pos, dir, dmg, 0);
        Actor *pA = Actor::GetPtrFromID(m_SpecialActorID);
        if (pA) {
            XItemHandler_ScriptBase::FX_CreateLinkBlood(0x4000409, m_SpecialActorID, 0xe000003, Gore_GetBloodGroup(pA), 0x40);
            m_SpecialActorID = 0;
        }
        break;
    }
    case 0x16000013: {
        unsigned int fl = pEvent->m_Param[1].u;
        int h = !(fl & 1);
        if (fl & 2) h = 1;
        if (fl & 4) {
            if (!m_AttackType) break;
            h = 7;
        }
        CreateSwoosh(pEvent->m_Param[0].u, pItem, m_pWeaponItem[h]);
        break;
    }
    case 0x16000014: {
        unsigned int fl = pEvent->m_Param[1].u;
        int h = !(fl & 1);
        if (fl & 2) h = 1;
        if (fl & 4) h = 7;
        DeleteSwoosh(pEvent->m_Param[0].u, m_pWeaponItem[h]);
        break;
    }
    case 0x16000015:
        m_bAnimEnded = 1;
        m_AnimEndParam = pEvent->m_Param[0].u;
        break;
    case 0x16000016:
        switch (pEvent->m_Param[0].i) {
        case 0: SetWeapon(0); break;
        case 1: SetWeapon(1); break;
        case 2: SetWeapon(2); break;
        case 3: SetWeapon(3); break;
        }
        break;
    case 0x16000022:
        m_bTurnToTarget = 1;
        break;
    case 0x16000025:
        switch (pEvent->m_Param[0].i) {
        case 0:
            SendMsg_LoudNoise(pItem->Position(), 25.0f, 0, 1, 0);
            if ((int)pEvent->m_Param[1].f == 1) SendMsg_HealingRoar(m_pItem->Position(), 10.0f);
            break;
        case 2:
            if (m_pSpecialItem) {
                const EXRuntimeClass *rc = m_pSpecialItem->ItemHandler()->GetRuntimeClass();
                do {
                    if (rc == EXRUNTIME_CLASS(XItemHandler_RechargePoint)) {
                        if ((int)pEvent->m_Param[1].f == 1) {
                            ((XItemHandler_RechargePoint *)m_pSpecialItem->ItemHandler())->StartRecharge();
                            ((XGamePad *)EXBaseGamePad::GetGamePad())->PlayRumble(0x54000003, 1.0f, m_PadIndex);
                        } else {
                            ((XItemHandler_RechargePoint *)m_pSpecialItem->ItemHandler())->EndRecharge();
                            m_pSpecialItem = 0;
                        }
                        goto done;
                    }
                    rc = rc->m_pBaseClass;
                } while (rc);
                rc = m_pSpecialItem->ItemHandler()->GetRuntimeClass();
                do {
                    if (rc == EXRUNTIME_CLASS(XItemHandler_Beacon)) {
                        ((XItemHandler_Beacon *)m_pSpecialItem->ItemHandler())->Activate();
                        break;
                    }
                    rc = rc->m_pBaseClass;
                } while (rc);
            }
            break;
        case 3:
            m_Unk12d[0] = 1;
            break;
        case 4:
            if (m_StateFlags & 0x800) {
                Actor *pA = Actor::GetPtrFromID(m_SpecialActorID);
                if (pA) {
                    EXBaseSoundManager::GetSoundManager()->StartSound(0x1af000ea, pA->GetPelvisPos(), -1, 0);
                    RagDollHeldDrone(1);
                    RollActorRagDoll(pA, 1);
                    m_SpecialActorID = pA->m_RefID;
                }
            }
            break;
        case 5: {
            pPhys->m_physicsFlags &= ~1;
            pPhys->SetThrustToAttainHeight(3.217f);
            SetRotation(m_Speed, m_Unk34c, m_Unk308);
            pPhys->m_DeltaMove += m_Speed * 0.2f;
            EXVector c = GetChestPos();
            XItem *pFX = XItemHandler_ScriptBase::FX_Create(c, 0x40002d7, 0x40, 0x1000016);
            EXVector r(0.0f, m_Unk350 + 1.5707964f, 1.5707964f, 0.0f);
            pFX->m_rotation = r;
            pFX->m_validMtx = 0;
            break;
        }
        case 6:
            if ((int)pEvent->m_Param[1].f == 0) m_bUnk370 = 0;
            else if ((int)pEvent->m_Param[1].f == 1) m_bUnk370 = 1;
            break;
        case 7:
            m_HoldState = (int)pEvent->m_Param[1].f != 0;
            break;
        case 8: {
            Actor *pA = Actor::GetPtrFromID(m_SpecialActorID);
            if (pA) {
                int k = (int)pEvent->m_Param[1].f;
                EXVector v(0.0f, 0.0f, 0.0f, 0.0f);
                if (k == 0) pA->CreateLostLimb((ActorLostLimb)2, v, 1, 0);
                else if (k == 1) pA->CreateLostLimb((ActorLostLimb)2, v, 0, 1);
                else if (k == 2) {
                    EXItem *pMe = m_pItem;
                    EXVector p(pMe->Position().x, pMe->Position().y + 1.75f, pMe->Position().z, pMe->Position().w);
                    float ry = pMe->Rotation().y;
                    EXVector d((float)sin(ry), 0.0f, (float)cos(ry), 0.0f);
                    Gore_ShootSplat(p, d, Gore_GetBloodGroup(pA));
                    pA->CreateLostLimb((ActorLostLimb)2, v, 0, 1);
                }
            }
            break;
        }
        case 9:
            if (m_pSpecialItem) {
                const EXRuntimeClass *rc = m_pSpecialItem->ItemHandler()->GetRuntimeClass();
                do {
                    if (rc == EXRUNTIME_CLASS(XItemHandler_ScriptRunner)) {
                        ((XItemHandler_ScriptRunner *)m_pSpecialItem->ItemHandler())->ApplyDamage(pEvent->m_Param[1].f, 0x53000000);
                        break;
                    }
                    rc = rc->m_pBaseClass;
                } while (rc);
            }
            break;
        case 10:
            XGetGameStats()->m_Unk4c[5]++;
            m_Ammo[0xc]--;
            m_Health = GetMaxHealth();
            if (m_bCloak) {
                SparkSkin(3.0f);
                SetCloak(0);
            }
            break;
        case 11: {
            float t = (float)pEvent->m_Frames * 2.0f;
            m_TurnTime = t;
            m_TurnRate = pEvent->m_Param[1].f / t * 0.017453292f;
            break;
        }
        case 12:
            m_bUnkc40 = 1;
            break;
        }
        break;
    case 0x16000027:
        LockHand(pEvent->m_Param[0].i, pEvent->m_Frames * 2, 0);
        break;
    case 0x16000028:
        SparkSkin(pEvent->m_Param[0].f);
        break;
    case 0x16000029:
        if (pEvent->m_Param[0].i == 1) m_Unk390 = 1;
        else if (pEvent->m_Param[0].i == 2) m_Unk390 = 2;
        break;
    case 0x16000041:
        SetTargetAdjustment(m_TargetAdjust, m_TargetAdjustTime, pEvent->m_Frames * 2);
        break;
    case 0x16000042: {
        float r = 1.0f / ((float)pEvent->m_Frames * 2.0f);
        switch (pEvent->m_Param[0].i) {
        case 0:
            r = 0.0f;
            m_MoveBlend = pEvent->m_Param[1].f;
        case 3:
            m_Unk24c = r;
            break;
        case 1:
            m_MoveBlend = 0.0f;
            m_Unk24c = r;
            break;
        case 2:
            m_MoveBlend = 1.0f;
        case 4:
            m_Unk24c = 0.0f - r;
            break;
        }
        break;
    }
    case 0x1600004d:
        PlaySpecialFXScript(pEvent->m_Param[0].i);
        break;
    case 0x1600005e:
        ((XGamePad *)EXBaseGamePad::GetGamePad())->PlayRumble(pEvent->m_Param[0].u, pEvent->m_Param[1].f, 0xff);
        break;
    case 0x1600005f: {
        EXVector p = pAnim->GetOwnerItem()->Position();
        ((XGamePad *)EXBaseGamePad::GetGamePad())->PlayRumble3D(pEvent->m_Param[0].u, p, pEvent->m_Param[2].f, pEvent->m_Param[3].f, pEvent->m_Param[1].f, 0xff);
        break;
    }
    }
done:
    m_Unkd4 = 0;
    return 0;
}

// NEAR: written from the disassembly for behaviour (switch tables drafted with tools/scratch/swx.py).
// Changes the player's move: picks the animation for (mode, sub) and the field its depth
// (blend parameter) follows, sets m_Mode/m_SubMode (some moves rewrite sub), applies the frame-rate
// multiplier when one is given (>= 0), then rebuilds m_MoveFlags from PredMoveFlags and sets the
// physics friction from them.
void XItemHandler_Player::SetupMove(unsigned int mode, unsigned int sub, float speed)
{
    EXItemAnimator_Anim *pAnim = (EXItemAnimator_Anim *)m_pItem->GetAnimator(EXRUNTIME_CLASS(EXItemAnimator_Anim));
    if (m_Unkd4) return;
    switch (mode) {
    case 0x18:  // gecko (wall walk)
        switch (sub) {
        case 0: ForceModeChange(0x90002ed); m_Unk224 = 1.0f; SetupDepthPtr(&m_Unk224, 0); break;
        case 2: ForceModeChange(0x90002b4); break;
        case 3: ForceModeChange(0x90002ed); SetupDepthPtr(&m_Unk224, 0); break;
        }
        break;
    case 0x17:  // react (hit reactions, knockdowns)
        switch (sub) {
        case 0: ForceModeChange(0x90001d6); break;
        case 1: ForceModeChange(0x90001d8); break;
        case 2: ForceModeChange(0x90001d7); break;
        case 3: ForceModeChange(0x90004da); break;
        case 4: ForceModeChange(0x90001d1); break;
        case 5: ForceModeChange(0x90001d2); break;
        case 6: ForceModeChange(0x90001d0); break;
        case 7: ForceModeChange(0x90004db); break;
        case 8: ForceModeChange(0x90002bd); break;
        case 9: ForceModeChange(0x90002e1); break;
        case 10: ForceModeChange(0x90002f7); break;
        case 11: ForceModeChange(0x90003e8); break;
        case 12: ForceModeChange(0x90003e7); break;
        case 13: ForceModeChange(0x900049e); break;
        case 14: ForceModeChange(0x900038d); break;
        case 15: ForceModeChange(0x9000300); break;
        }
        break;
    case 0x16:  // grab
        switch (sub) {
        case 0: ForceModeChange(0x900019c); break;
        case 1: pAnim->BlendTo(0, 0x40002bd, 11, 16); break;
        case 3: ForceModeChange(0x90002e0); SetupDepthPtr(&m_MSpeed, 0); break;
        case 5: ForceModeChange(0x9000197); break;
        case 6: ForceModeChange(0x900019b); break;
        case 7: ForceModeChange(0x900024f); break;
        case 8: ForceModeChange(0x90001ee); break;
        case 9: ForceModeChange(0x90001da); break;
        case 10: ForceModeChange(0x90001db); break;
        case 2: ForceModeChange(0x9000458); break;
        }
        break;
    case 0:     // idle
        switch (sub) {
        case 0:
        case 1: ForceModeChange(0x9000002); break;
        case 2: pAnim->ChangeTo(0, 0x4000002, 3); sub = 0; break;
        case 3: pAnim->BlendTo(0, 0x4000002, 11, 16); sub = 0; break;
        case 4: ForceModeChange(0x900026f); break;
        case 5: ForceModeChange(0x9000271); break;
        case 6: ForceModeChange(0x9000408); break;
        case 7: ForceModeChange(0x90000a0); break;
        case 9: ForceModeChange(0x9000458); break;
        case 10: ForceModeChange(0x900044e); break;
        case 8: ForceModeChange(0x900019d); SetupDepthPtr(&m_Unk224, 0); break;
        case 11: ForceModeChange(0x900010b); SetupDepthPtr(&m_ShimmyDir, 0); break;
        }
        break;
    case 2:     // crouch
        switch (sub) {
        case 0: ForceModeChange(0x9000493); break;
        case 1: ForceModeChange(0x90004c3); break;
        case 2: ForceModeChange(0x900049f); break;
        case 3: ForceModeChange(0x90004ba); break;
        }
        break;
    case 0x10:  // pre-jump
        ForceModeChange(0x900047e);
        SetupDepthPtr(&m_ShimmyDir, 0);
        break;
    default:
        break;
    case 3:     // jump
        switch (sub) {
        case 0:
        case 2:
        case 9: ForceModeChange(0x900043c); break;
        case 4:
        case 6:
        case 13: ForceModeChange(0x900010a); break;
        case 1:
            switch (g_EXRandClass.Rand32() & 3) {
            case 0: ForceModeChange(0x900049a); break;
            case 1: ForceModeChange(0x9000495); break;
            case 2: ForceModeChange(0x9000498); break;
            case 3: ForceModeChange(0x9000499); break;
            }
            break;
        case 5: ForceModeChange(0x90000fa); break;
        case 3:
        case 12: ForceModeChange(0x900010d); break;
        case 7:
        case 11: ForceModeChange(0x900013a); break;
        case 10: ForceModeChange(0x9000010); break;
        case 8: ForceModeChange(0x900017e); SetupDepthPtr(&m_Unk228, 0); break;
        }
        break;
    case 1:     // move
        switch (sub) {
        case 0: ForceModeChange(0x9000003); SetupDepthPtr(&m_MSpeed, 0); break;
        case 1:
            pAnim->BlendTo(0, 0x4000003, 11, 0);
            ForceModeChange(0x9000003);
            SetupDepthPtr(&m_MSpeed, 0);
            sub = 0;
            break;
        case 2: ForceModeChange(0x90004a0); SetupDepthPtr(&m_MSpeed, 0); break;
        case 4: pAnim->BlendTo(0, 0x4000003, 11, 8); SetupDepthPtr(&m_MSpeed, 0); break;
        case 5: pAnim->BlendTo(0, 0x4000003, 11, 0); SetupDepthPtr(&m_MSpeed, 0); sub = 4; break;
        case 3:
            pAnim->BlendTo(0, 0x40000ca, 11, 8);
            m_Unk230 = 0.0f;
            m_Unk234 = 0;
            SetupDepthPtr(&m_Unk230, 0);
            break;
        case 6: pAnim->BlendTo(0, 0x4000003, 11, 8); SetupDepthPtr(&m_Unk21c, 0); break;
        case 7: ForceModeChange(0x9000458); break;
        }
        break;
    case 0xe:   // skid
        switch (sub) {
        case 0: ForceModeChange(0x9000480); break;
        case 1: ForceModeChange(0x900047b); break;
        }
        break;
    case 4:     // clamber
        switch (sub) {
        case 0: ForceModeChange(0x900015d); break;
        case 1: ForceModeChange(0x900016a); break;
        case 2: ForceModeChange(0x90002ff); break;
        case 7: ForceModeChange(0x9000169); break;
        case 8: ForceModeChange(0x9000168); break;
        case 9: ForceModeChange(0x900043a); break;
        case 10: ForceModeChange(0x900043b); break;
        case 3: ForceModeChange(0x900005b); m_Unk360 = 10.0f; m_bUnk364 = 1; break;
        case 4: ForceModeChange(0x90001d9); m_Unk360 = 10.0f; m_bUnk364 = 1; break;
        case 5: ForceModeChange(0x90001ac); break;
        case 6: ForceModeChange(0x90001ad); break;
        }
        break;
    case 0x13:  // TOS (button mash)
        switch (sub) {
        case 0: ForceModeChange(0x90002f6); break;
        case 1: ForceModeChange(0x90002f5); break;
        case 2: ForceModeChange(0x90002f4); break;
        case 3: ForceModeChange(0x90002f3); break;
        case 4:
        case 5: ForceModeChange(0x9000471); break;
        case 6: ForceModeChange(0x9000470); break;
        case 7: ForceModeChange(0x9000472); break;
        }
        break;
    case 0x14:  // Ulysses
        switch (sub) {
        case 0: ForceModeChange(0x90003a9); break;
        case 1: ForceModeChange(0x90003aa); break;
        case 2: ForceModeChange(0x90003ab); break;
        }
        break;
    case 0x15:  // car ride
        switch (sub) {
        case 0: ForceModeChange(0x90003a9); break;
        case 1: ForceModeChange(0x90003aa); break;
        }
        break;
    case 6:     // attack: the next step of the weapon's combo, unless a combo move takes over
        if (sub == 0) {
            AdvanceCombo(m_AttackType, 1);
            if (CheckForCombo()) return;
            ForceModeChange(WeaponAt[m_AttackType][WeaponAt[m_AttackType][m_AttackState] ? m_AttackState : 0]);
            SetupDepthPtr(&m_Unk224, 0);
            ResetWeaponDown();
            SendAttackMessage(m_AttackType, -1);
        }
        break;
    case 7:     // special hits
        switch (sub) {
        case 0:
            if (GetCardRandom() & 1) ForceModeChange(0x9000542);
            else ForceModeChange(0x90002f8);
            break;
        case 1: ForceModeChange(0x90000ab); break;
        case 2: ForceModeChange(0x9000439); break;
        case 4: ForceModeChange(0x90001a7); break;
        case 5: ForceModeChange(0x90001a1); break;
        case 6: ForceModeChange(0x90001af); break;
        case 7: ForceModeChange(0x9000277); break;
        case 8: ForceModeChange(0x900044c); break;
        case 9: ForceModeChange(0x9000473); break;
        case 10: ForceModeChange(0x9000485); break;
        case 11: ForceModeChange(0x9000487); break;
        case 12: {  // combo finisher for the current weapon
            int col;
            switch (m_AttackType) {
            case 2: col = 0; goto finisher;
            case 4: col = 1; goto finisher;
            case 3: col = 2; goto finisher;
            case 0: col = 3; goto finisher;
            case 1: col = 4;
            finisher:
                ForceModeChange(Combo_Moves[col].m_Moves[m_Combo]);
            }
            SendAttackMessage(m_AttackType, m_Combo);
            break;
        }
        case 13: ForceModeChange(0x9000297); break;
        case 14: ForceModeChange(0x9000395); break;
        case 15: ForceModeChange(0x90004f1); break;
        case 16: ForceModeChange(0x90002c5); break;
        case 17: ForceModeChange(0x900037a); break;
        case 18: ForceModeChange(0x90004f2); break;
        case 19: ForceModeChange(0x90002e2); break;
        }
        break;
    case 8:     // land
        m_Speed = EXVector(0.0f, 0.0f, 0.0f, 0.0f);
        switch (sub) {
        case 0: ForceModeChange(0x9000110); break;
        case 2: ForceModeChange(0x90001bf); break;
        }
        break;
    case 9:     // fall / dead fall
        m_pItem->m_pItemPhysics->SetPhysicsFlagsOff(3);
        ForceModeChange(0x9000010);
        break;
    case 0xa:   // wall jump
        DeadStop(0);
        m_pItem->m_pItemPhysics->SetPhysicsFlagsOn(1);
        switch (sub) {
        case 0: ForceModeChange(0x900005d); break;
        case 1: ForceModeChange(0x900005e); break;
        case 2:
            ForceModeChange(0x9000011);
            m_Unk34c = m_pItem->m_rotation.y + 3.1415927f;
            m_Unk350 = m_pItem->m_rotation.y;
            break;
        }
        break;
    case 0x11:  // monkey bars
        switch (sub) {
        case 0: m_Unk360 = 10.0f; m_bUnk364 = 1; ForceModeChange(0x9000128); SetupDepthPtr(&m_ShimmyDir, 0); break;
        case 1: ForceModeChange(0x9000139); break;
        case 2: ForceModeChange(0x9000157); break;
        case 3: ForceModeChange(0x9000127); break;
        case 4: ForceModeChange(0x9000138); break;
        case 5: ForceModeChange(0x9000148); break;
        case 6: ForceModeChange(0x9000147); break;
        }
        break;
    case 0x12:  // perch
        switch (sub) {
        case 0: ForceModeChange(0x9000159); SetupDepthPtr(&m_ShimmyDir, 0); break;
        case 1: ForceModeChange(0x9000158); break;
        case 2: ForceModeChange(0x90001ab); SetupDepthPtr(&m_Unk228, 0); break;
        case 3: m_Unk224 = 1.5f; ForceModeChange(0x90001c0); SetupDepthPtr(&m_Unk228, 0); break;
        }
        break;
    case 0xc:   // hang / shimmy
        switch (sub) {
        case 0: m_Unk360 = 10.0f; m_bUnk364 = 1; ForceModeChange(0x9000162); break;
        case 1: m_Unk360 = 10.0f; m_bUnk364 = 1; sub = 0; pAnim->BlendTo(sub, 0x40001a3, 11, 16); break;
        case 2: ForceModeChange(0x9000161); break;
        case 4: ForceModeChange(0x90000ac); SetupDepthPtr(&m_ShimmyDir, 0); break;
        case 7: ForceModeChange(0x900016f); SetupDepthPtr(&m_ShimmyDir, 0); break;
        case 5: m_Unk360 = 10.0f; m_bUnk364 = 1; ForceModeChange(0x900015f); break;
        case 3: ForceModeChange(0x9000165); break;
        case 6: ForceModeChange(0x9000163); break;
        case 8: m_Unk360 = 10.0f; m_bUnk364 = 1; ForceModeChange(0x90000e0); break;
        case 9: ForceModeChange(0x90000f4); break;
        }
        if (sub == 8 || sub == 9) {
            m_ShimmyDir = m_ClamberSide ? 1.0f : 2.0f;
            SetupDepthPtr(&m_ShimmyDir, 0);
        }
        break;
    }
    m_Mode = mode;
    m_SubMode = sub;
    if (speed >= 0.0f) pAnim->Controller(0)->SetFrameRateMult(speed);
    unsigned int key = (m_Mode << 16) | (m_SubMode & 0xffff);
    unsigned int any = (m_Mode << 16) | 0xffff;
    m_MoveFlags = 0;
    for (int i = 0; i < 70; i++) {
        if (PredMoveFlags[i].m_Key == any) m_MoveFlags |= PredMoveFlags[i].m_Flags;
        if (PredMoveFlags[i].m_Key == key) m_MoveFlags |= PredMoveFlags[i].m_Flags;
    }
    if (m_Mode == 0x16 || m_HoldState == 1) m_MoveFlags |= 0x9000;
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
    if (m_MoveFlags & 0x80) {
        pPhys->m_Friction = 1.0f;
        pPhys->m_bVertWallReaction = 0;
    } else {
        pPhys->m_Friction = 0.5f;
        pPhys->m_bVertWallReaction = 1;
    }
    pPhys->m_bIgnoreCars = m_Mode == 0x15;
}

// NEAR: only the final scale's operand order differs (retail multiplies count*x, y*count, ...).
void XItemHandler_Player::GetHandLockPos(int mask, EXVector &out)
{
    EXVector sum;
    sum.SetZero();
    EXDatumTemp d;
    memset(&d, 0, sizeof(d));
    float count = 0.0f;
    if (mask) {
        if (mask & 1) {
            m_pItem->GetAnimatorDatum(0x10000005, d, 1, 0);
            sum = d.m_position;
            count = 1.0f;
        }
        if (mask & 2) {
            m_pItem->GetAnimatorDatum(0x10000006, d, 1, 0);
            sum += d.m_position;
            count = count + 1.0f;
        }
        if (mask & 4) {
            m_pItem->GetAnimatorDatum(0x10000031, d, 1, 0);
            sum += d.m_position;
            count = count + 1.0f;
        }
        if (mask & 8) {
            m_pItem->GetAnimatorDatum(0x10000032, d, 1, 0);
            sum += d.m_position;
            count = count + 1.0f;
        }
        if (mask & 0x10) {
            m_pItem->GetAnimatorDatum(0xe000001, d, 1, 0);
            sum += d.m_position;
            count = count + 1.0f;
        }
        count = 1.0f / count;
        out = sum * count;
    }
}

// NEAR: retail builds the probe vector through float temporaries (extra spills) and returns 0 from an inline epilogue.
signed char XItemHandler_Player::MoveCheckForLedge()
{
    EXItem *pItem = m_pItem;
    XItemPhysics *pPhys = (XItemPhysics *)pItem->m_pItemPhysics;
    signed char climbing = 0;
    float radius;
    if (m_Mode == 0x11) {
        climbing = 1;
        radius = 0.3f;
    } else {
        if (m_StickMag > 0.9f) return 0;
        radius = 0.5f;
    }
    float fx = pItem->m_position.x + pPhys->m_PFDeltaMove.x + pPhys->m_DeltaMove.x;
    float fy = pItem->m_position.y;
    float fz = pItem->m_position.z + pPhys->m_PFDeltaMove.z + pPhys->m_DeltaMove.z;
    float fw = pItem->m_position.w;
    float y = fy;
    if (climbing) y = m_ClamberPos.y + 0.3f;
    EXVector probe;
    probe.x = fx;
    probe.y = y;
    probe.z = fz;
    probe.w = fw;
    EXGrabPointTemp t;
    t.Set(probe, radius + 0.1f, (EXGrabPointTemp::GRABFITYPE)1);
    theItemEnv.DoGrabPointToMap(t);
    if (t.m_24 == 1 && !(*(u32 *)&t.m_28 & 0x20) && !(*(u32 *)&t.m_28 & 0x100) &&
        (float)fabs(y - t.m_30.y) <= 0.01f) {
        float dx = probe.x - t.m_30.x;
        float dz = probe.z - t.m_30.z;
        float dist = EXSqrt(dz * dz + dx * dx);
        if (dist < radius) {
            float nx = dx, nz = dz, ny = 0.0f;
            float l2 = dz * dz + dx * dx;
            if (0.0f <= l2) {
                float len = EXSqrt(l2);
                if (len > EX_EPSILON_UNIT) {
                    float inv = 1.0f / len;
                    ny = inv * 0.0f;
                    nz = dz * inv;
                    nx = inv * dx;
                }
            }
            float k = radius - dist;
            EXVector out;
            out.x = k * nx;
            out.z = nz * k;
            out.y = k * ny;
            out.w = k * ny;
            pPhys->AddPFDeltaMove(out);
            return 1;
        }
    }
    return 0;
}

// NEAR: retail does the yaw update as x87 fadd/fstp straight from the InterpAngle result; here it goes through SSE.
void XItemHandler_Player::DoMonkey()
{
    XItemPhysics *pPhys = (XItemPhysics *)m_pItem->m_pItemPhysics;
    HandleGrabAdjustment();
    if (!(m_ButtonsHeld & 0x28)) {
        EXVector v;
        MonkeyCheckAround(v);
        switch (m_SubMode) {
        default:
            if (m_bAnimEnded) {
                pPhys->SetPhysicsFlagsOff(2);
                SetupMove(0x11, 1, -1.0f);
                return;
            }
            break;
        case 1:
        case 2:
            if (AnimEnded()) {
                if (m_SubMode != 1) {
                    SetupMove(0x11, 1, -1.0f);
                    return;
                }
                if (g_EXRandClass.Rand32() % 3 == 0) {
                    SetupMove(0x11, 2, -1.0f);
                    return;
                }
            }
            if (m_StickMag > 0.9f) {
                if (MonkeyCheckAhead()) {
                    SetupMove(0x11, 4, -1.0f);
                    return;
                }
                EXItem *pItem = m_pItem;
                v = pItem->m_rotation;
                pItem->m_rotation = EXVector(v.x, InterpAngle(0.0f, Vec_AngleDifference(v.y, m_PadAngle), 0.0872665f, XApp::GameFrameRateMul()) + v.y, v.z, v.w);
                pItem->m_validMtx = 0;
                return;
            }
            break;
        case 3:
        case 4: {
            EXItem *pItem = m_pItem;
            v = pItem->m_rotation;
            pItem->m_rotation = EXVector(v.x, InterpAngle(0.0f, Vec_AngleDifference(v.y, m_PadAngle), 0.0872665f, XApp::GameFrameRateMul()) + v.y, v.z, v.w);
            pItem->m_validMtx = 0;
            if (m_Unk390) {
                if (m_StickMag <= 0.9f || !MonkeyCheckAhead()) {
                    if (m_SubMode != 4 && m_Unk390 != 1) {
                        SetupMove(0x11, 5, 1.5f);
                        return;
                    }
                    SetupMove(0x11, 6, 1.5f);
                    return;
                }
                if (m_SubMode == 4) {
                    SetupMove(0x11, 3, 1.5f);
                    return;
                }
            }
        }
        }
    } else {
        pPhys->SetPhysicsFlagsOff(3);
        pPhys->m_DeltaMove.y = 0.0f;
        m_Unk358 = 0.33333334f;
        SetupMove(9, 0, -1.0f);
        m_ButtonsPressed &= ~0x28u;
        m_ButtonsHeld &= ~0x28u;
        m_ButtonsReleased &= ~0x28u;
    }
}
