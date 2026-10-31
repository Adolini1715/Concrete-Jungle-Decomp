#include "XItemHandler_AirVehicle.h"
#include "XItemHandler_Player.h"
#include "XItem.h"
#include "XApp.h"
#include "ActorProfile.h"
#include "ScanInfo.h"
#include "TracerFire.h"
#include "GeoSectionManager.h"
#include "EXItemAnimator_Script.h"
#include "EXGeoFile.h"
#include "EXDatum.h"
#include "EXRand.h"
#include "EXMemory.h"
#include "FEText.h"
#include "XItemHandler_ScriptBase.h"
#include "XItemHandler_Environment.h"
#include "EXRayCastTemp.h"
#include "EXQMath.h"
#include "EXSoundManager.h"
#include "WeaponTable.h"
#include "XItemHandler_Bullet.h"
#include "EXGeoMap.h"
#include <math.h>

signed char LineOfSight(EXVector &a, EXVector &b, signed char c);  // (Bullet.obj)
float Vec_AngleDifference(float a, float b);  // Vec.obj
float AngleDiff(float a, float b);  // EXMaths.obj
void Objective_NotifyPlayerSeenByAirVehicle(EXItem *pItem);
void Objective_NotifyAirVehicleDestroyed(EXItem *pItem);
EXItemAnimator_Script *AddScriptToDatum(EXItem *pItem, unsigned int file, unsigned int script, unsigned int datum);

int gAirVehicle_Count;
XItemList gAirVehicleList;

EX_IMPLEMENT_DYNCREATE_ROOT(XItemHandler_AirVehicle)

XItemHandler_AirVehicle::~XItemHandler_AirVehicle()
{
    gAirVehicleList.Remove((XItem *)m_pItem);
    gAirVehicle_Count--;
    m_SoundTag.RemoveAll(1);
    for (int i = 0; i < 8; i++) {
        if (m_pTracers[i]) m_pTracers[i]->Reset();
    }
    if (m_Unk112 == 1) GeoSectionManager::Del(0x100003c, 0x20000da);
}

void XItemHandler_AirVehicle::InitTracers()
{
    for (int i = 0; i < 8; i++) m_pTracers[i] = TracerFireManager::GetFreeTracer();
}

void XItemHandler_AirVehicle::CreateBluesAndTwos()
{
    AddScriptToDatum(m_pItem, 0x10000cc, 0x4000421, 0x100000a6);
    AddScriptToDatum(m_pItem, 0x10000cc, 0x4000421, 0x100000a7);
    AddScriptToDatum(m_pItem, 0x10000cc, 0x4000422, 0x100000a8);
    AddScriptToDatum(m_pItem, 0x10000cc, 0x4000422, 0x100000a9);
}

void XItemHandler_AirVehicle::DeleteBluesAndTwos()
{
    EXItemAnimator *p;
    while ((p = m_pItem->GetAnimator(0x4000421)) != 0) m_pItem->DeleteAnimator(p);
    while ((p = m_pItem->GetAnimator(0x4000422)) != 0) m_pItem->DeleteAnimator(p);
}

// NEAR: VC7.1 will not inline LengthXZ/Normalise here (retail did: inliner budget); state machine matches
void XItemHandler_AirVehicle::UpdateEmergency()
{
    switch (m_State) {
    case 0:
        if (m_Timer > 0.0f) m_Timer -= XApp::FrameLength();
        else m_Pos.y += 0.1f;
        break;
    case 2: {
        EXVector d(m_Target.x - m_Pos.x, m_Target.y - m_Pos.y, m_Target.z - m_Pos.z, m_Target.w - m_Pos.w);
        d.y = 0.0f;
        if (m_Speed > d.LengthXZ()) {
            m_Pos = EXVector(m_Target.x, m_Pos.y, m_Target.z, m_Target.w);
            m_Speed = 0.0f;
            m_State = 5;
            m_Unk129 = 0;
        } else {
            d.Normalise();
            m_Pos += d * m_Speed;
        }
        break;
    }
    case 5:
        if ((float)fabs(m_Target.y - m_pItem->m_position.y) < 10.0f) {
            m_State = 0;
            m_Timer = 30.0f;
        } else {
            m_Pos.y -= 0.1f;
        }
        break;
    }
    m_Time += XApp::FrameLength();
    m_pItem->SetPosition(EXVector(m_Pos.x + sin(m_Time * 2.2f) * 0.125f, m_Pos.y + cos(m_Time * 2.0f) * 0.125f,
                                  m_Pos.z + cos(m_Time * 1.7f) * 0.125f, m_Pos.w));
}

signed char XItemHandler_AirVehicle::NodeOccupied(int node)
{
    s8 r = 0;
    for (XItem *p = gAirVehicleList.GetFirst(); p; p = gAirVehicleList.GetNext(p)) {
        XItemHandler_AirVehicle *pAir = (XItemHandler_AirVehicle *)p->ItemHandler();
        if (pAir->m_Path == m_Path && pAir->m_Node == node) {
            r = 1;
            break;
        }
    }
    return r;
}

signed char XItemHandler_AirVehicle::ScanCallback(signed char b, FEObject *pObj, ScanInfo *pInfo)
{
    XItemHandler_AirVehicle *pAir = (XItemHandler_AirVehicle *)pInfo->m_pItem->ItemHandler();
    ActorProfile *pProf = AP_GetPtr(pAir->m_Profile);
    float f = pAir->m_Health / pProf->m_Health;
    pInfo->m_a4 = f;
    if (f <= 0.05f) {
        pInfo->m_text.Format("Wrecked");
        return 1;
    }
    if (f < 0.5f) {
        pInfo->m_text.Format("Broken");
        return 1;
    }
    if (f < 1.0f) {
        pInfo->m_text.Format("Damaged");
        return 1;
    }
    pInfo->m_text.Format("Undamaged");
    return 1;
}

// NEAR: retail keeps &m_pTracers in eax (lea before the int stores); ours reuses esi
s8 XItemHandler_AirVehicle::Initialise()
{
    if (!EXItemHandler::Initialise()) return 0;
    gAirVehicleList.Add((XItem *)m_pItem);
    gAirVehicle_Count++;
    m_pItem->RenderInfo().SetTexSwapInfo(&m_TexSwapInfo);
    m_pItem->RenderInfo().SetExtRenderInfo(&m_ExtRenderInfo);
    m_pItem->RenderInfo().RenderFlagsRef().BitSet(2);
    m_pItem->RenderInfo().SetRGBAMult(1.0f, 1.0f, 1.0f, 0.0f);
    m_pMuzzleScript = 0;
    m_Node = 0;
    m_PrevNode = -1;
    m_PathT = 0.0f;
    m_Unk112 = 0;
    m_State = 0;
    m_bMoving = 0;
    m_Timer = 0.0f;
    m_Time = g_EXRandClass.Randf() * 2.7f;
    m_Health = 100.0f;
    m_bExploded = 0;
    m_Unk128 = 0;
    m_Unk129 = 0;
    m_Profile = 0;
    m_FireTimer = 0.0f;
    m_LastPos.set(0.0f, 0.0f, 0.0f, 0.0f);
    m_Burst = 0;
    m_DamageTimer = 0;
    m_NextTracer = 0;
    m_SoundTimer = 0.0f;
    memset(m_pTracers, 0, sizeof(m_pTracers));
    return 1;
}

void XItemHandler_AirVehicle::DeleteMuzzleScript()
{
    if (m_pMuzzleScript) {
        m_pMuzzleScript->ItemFlags().m_flags |= 0x10;
        m_pMuzzleScript = 0;
    }
}

// Once per frame (cached in m_SeesPredator, -666 until checked): the uncloaked player within the
// profile's attack range, within 60 degrees of the nose in heading and pitch, and in line of sight.
// Being seen raises the alarm.
signed char XItemHandler_AirVehicle::CanSeePredator()
{
    if (m_SeesPredator == -666) {
        if (!XItemHandler_Player::GetInstance()->GetCloak()) {
            ActorProfile *pProf = AP_GetPtr(m_Profile);
            const EXVector &pp = gpPlayerItem[0]->Position();
            EXVector target(pp.x, pp.y + 1.5f, pp.z, pp.w);
            EXItem *pItem = m_pItem;
            EXVector d = target - pItem->m_position;
            if (d.y * d.y + d.z * d.z + d.x * d.x <= pProf->m_AttackRange * pProf->m_AttackRange
                && fabs(Vec_AngleDifference(qatan2f(d.x, d.z), pItem->m_rotation.y)) < 1.0471976f
                && fabs(Vec_AngleDifference(qatan2f(d.y, EXSqrt(d.z * d.z + d.x * d.x)), 0.0f - m_pItem->m_rotation.x)) < 1.0471976f) {
                m_SeesPredator = LineOfSight(m_pItem->m_position, target, 0);
                if (m_SeesPredator) Objective_NotifyPlayerSeenByAirVehicle(m_pItem);
                return (signed char)m_SeesPredator;
            }
        }
        m_SeesPredator = 0;
    }
    return (signed char)m_SeesPredator;
}

// Shot down: keep flying along the crash direction (speed easing towards 0.5, dropping), spinning,
// with the rotor sound winding up, until the movement ray hits the map, the vehicle falls below
// -200 or the fuse runs out; then explode.
void XItemHandler_AirVehicle::UpdateCrashing()
{
    if (m_bExploded == 1) return;
    if (m_Speed >= 0.5f) m_Speed = m_Speed - XApp::GameFrameRateMul() * 0.002f;
    else m_Speed = XApp::GameFrameRateMul() * 0.002f + m_Speed;
    EXVector from = m_pItem->m_position;
    float k = m_Speed;
    EXVector step(m_CrashDir.x * k * XApp::GameFrameRateMul(), m_CrashDir.y * k * XApp::GameFrameRateMul(),
                  m_CrashDir.z * k * XApp::GameFrameRateMul(), m_CrashDir.w * k * XApp::GameFrameRateMul());
    if (m_Health >= -500000.0f) m_CrashDir.y = m_CrashDir.y - XApp::GameFrameRateMul() * 0.03f;
    else m_Speed = m_Speed * 0.85f;
    m_CrashDir.Normalise();
    m_pItem->SetPosition(EXVector(step.x + from.x, step.y + from.y, step.z + from.z, step.w + from.w));
    m_pItem->SetRotation(EXQuaternion(m_Target.x * XApp::GameFrameRateMul() + m_pItem->m_rotation.x,
                                      m_Target.y * XApp::GameFrameRateMul() + m_pItem->m_rotation.y,
                                      m_Target.z * XApp::GameFrameRateMul() + m_pItem->m_rotation.z,
                                      m_Target.w * XApp::GameFrameRateMul() + m_pItem->m_rotation.w));
    EXRayCastTemp ray;
    ray.Set(from, (m_pItem->m_position - from) * 2.0f, 0, 0, 0, 0);
    theItemEnv.DoRayCastToMap(ray, 0);
    m_SoundTimer = m_SoundTimer + 50.0f;
    m_SoundTag.SetPitch((int)m_SoundTimer);
    EXVector at;
    if (ray.m_bHit) {
        at = ray.Intersect();
    } else {
        if (from.y >= -200.0f && m_DamageTimer > 0) return;
        at = m_pItem->m_position;
    }
    EXVector pos(at.x, at.y + 0.3f, at.z, at.w);
    XItemHandler_ScriptBase::Explosion_Create(pos, 0x400027e, 10, 50, 0, 0, 0, 0, 0);  // HT_Script_Impact_Explosion_Normal
    m_bExploded = 1;
    DeleteBluesAndTwos();
    m_SoundTag.RemoveAll(1);
}

// Damage (one hit per 10 frames). At zero health the vehicle starts crashing along its last
// movement with a random spin and a 150 frame fuse; a patrolling vehicle that is hit (state 1)
// instead heads for the point between itself and the player with its lights on.
void XItemHandler_AirVehicle::ApplyDamage(float f)
{
    if (m_DamageTimer == 0) {
        m_Health = m_Health - f;
        m_DamageTimer = 10;
    }
    if (m_State == 4) return;
    if (m_Health <= 0.0f) {
        EXItem *pItem = m_pItem;
        m_State = 4;
        EXVector d = pItem->m_position - m_LastPos;
        m_Speed = d.LengthXYZ() / XApp::GameFrameRateMul() * 0.75f;
        m_CrashDir = pItem->m_position - m_LastPos;
        m_CrashDir.Normalise();
        m_Target.x = g_EXRandClass.Randf() * 0.06f - 0.03f;
        m_Target.y = g_EXRandClass.Randf() * 0.06f - 0.03f;
        m_DamageTimer = 150;
        m_Target.z = g_EXRandClass.Randf() * 0.06f - 0.03f;
        DeleteMuzzleScript();
        Objective_NotifyAirVehicleDestroyed(m_pItem);
        m_SoundTag.RemoveAll(1);
        m_SoundTag.AddTrackingSound(0x1af002de, m_pItem->m_position, 0, 0);
        m_SoundTag.SetPitch(20);
        return;
    }
    if (m_State == 1) {
        EXItem *pItem = m_pItem;
        m_State = 2;
        m_Target = (gpPlayerItem[0]->Position() + pItem->m_position) * 0.5f;
        m_Timer = 0.0f;
        CreateBluesAndTwos();
    }
}

// NEAR: 3 bytes differ, retail picks eax (ours ecx) for m_pItem in the shared BitClr(0x4000) tail
void XItemHandler_AirVehicle::HandleVisionModeTextureSwaps(unsigned char mode)
{
    EXTexSwapInfo &tex = m_TexSwapInfo;
    switch (mode) {
    case 1:
        m_pItem->RenderInfo().RenderFlagsRef().BitClr(2);
        m_pItem->RenderInfo().RenderFlagsRef().BitClr(4);
        m_pItem->RenderInfo().RenderFlagsRef().BitSet(8);
        m_pItem->RenderInfo().SetRGBAMult(1.0f, 1.0f, 1.0f, 1.0f);
        m_pItem->RenderInfo().RenderFlagsRef().BitSet(0x4000);
        tex.SetSwap(0x6000075, 0x1000016, 0x6000072);
        tex.SetSwap(0x6000077, 0x1000016, 0x6000061);
        tex.SetSwap(0x6000076, 0x1000016, 0x6000062);
        tex.SetSwap(0x6000074, 0x1000016, 0x6000063);
        tex.SetSwap(0x60001ec, 0x100004c, 0x6000045);
        tex.SetSwap(0x6000236, 0x100004c, 0x6000045);
        return;
    case 3:
        m_pItem->RenderInfo().RenderFlagsRef().BitClr(2);
        m_pItem->RenderInfo().RenderFlagsRef().BitSet(4);
        m_pItem->RenderInfo().RenderFlagsRef().BitSet(8);
        m_pItem->RenderInfo().SetRGBAMult(0.0f, 3.0f, 3.0f, 1.0f);
        break;
    default:
        m_pItem->RenderInfo().RenderFlagsRef().BitClr(2);
        m_pItem->RenderInfo().RenderFlagsRef().BitClr(4);
        m_pItem->RenderInfo().RenderFlagsRef().BitClr(8);
        break;
    }
    m_pItem->RenderInfo().RenderFlagsRef().BitClr(0x4000);
    tex.StopAllSwaps();
    if (!AirLights) tex.SetSwap(0x6000236, 0x100004c, 0x6000045);
}

signed char XItemHandler_AirVehicle::GetScanInfo(int type, ScanInfo *pInfo)
{
    switch (type) {
    case 0:
        pInfo->m_a0 = 0x400057d;
        pInfo->m_Text[0][0] = 0x51000607;
        pInfo->m_Text[1][0] = 0x510007d3;
        break;
    case 2:
        pInfo->m_a0 = 0x400077a;
        pInfo->m_Text[0][0] = 0x510004b9;
        pInfo->m_ModelFile = 0x100003c;
        pInfo->m_ModelEntity = 0x20000da;
        pInfo->m_Text[1][0] = 0x510000e7;
        break;
    default:
        pInfo->m_a0 = 0;
        break;
    }
    pInfo->m_a4 = m_Health / AP_GetPtr(m_Profile)->m_Health;
    s8 r = pInfo->m_a0 > 0;
    pInfo->m_ac = r;
    pInfo->m_pCallback = ScanCallback;
    return r;
}

void XItemHandler_AirVehicle::PlayMuzzleScript()
{
    DeleteMuzzleScript();
    m_pMuzzleScript = new EXItemAnimator_Script;
    if (!m_pMuzzleScript) return;
    EXGeoFile *pFile = EXGeoFile::GetGeoFile(0x1000016);
    m_pMuzzleScript->Initialise(pFile, 0x4000179);
    ex::EXFlag<u8> &f = m_pMuzzleScript->RenderFlags();
    f.Set(f.m_flags | 3);
    m_pMuzzleScript->SetScale(EXVector(3.0f, 3.0f, 3.0f, 0.0f));
    m_pItem->AddAnimator(m_pMuzzleScript);
    EXDatumTemp d;
    if (m_pItem->GetAnimatorDatum(0x10000088, d, 4, 0)) {
        m_pMuzzleScript->SetPosition(d.m_position);
        EXVector rot;
        d.m_rotation.GetRotation(rot, 4);
        m_pMuzzleScript->SetRotation(EXQuaternion(rot.x, rot.y, rot.z, rot.w));
    } else {
        m_pMuzzleScript->SetPosition(m_pItem->Position());
    }
}

void XItemHandler_AirVehicle::UpdateMuzzleScript()
{
    if (m_pMuzzleScript && m_pMuzzleScript->m_11a) DeleteMuzzleScript();
}

// TODO: 752-byte DoUpdate not written yet (virtual: kept as a stub for the vtable).
// Air patrol path data: nodes (sizeof 0x28) with a self-relative list of link indices, links
// (sizeof 0x1c) joining two nodes.
struct AirPathNode {
    EXVector3 m_Pos;    // 0x00
    u8 m_0c[0x24 - 0x0c];
    s16 m_Links;        // 0x24 self-relative offset of the u16 link index list
    u16 m_nLinks;       // 0x26
    const u16 *Links() const { return m_Links ? (const u16 *)((const char *)&m_Links + m_Links) : 0; }
};
struct AirPathLink {
    u16 m_A;            // 0x00
    u16 m_B;            // 0x02
    u8 m_04[0x1c - 4];
};
static __forceinline AirPathNode *Air_Node(const EXGeoPath *p, int i)
{
    int o = *(const int *)((const char *)p + 0x4c);
    return (AirPathNode *)(o ? (const char *)p + 0x4c + o : 0) + i;
}
static __forceinline AirPathLink *Air_Link(const EXGeoPath *p, int i)
{
    int o = *(const int *)((const char *)p + 0x44);
    return (AirPathLink *)(o ? (const char *)p + 0x44 + o : 0) + i;
}
// The node across link k of node n.
static __forceinline int Air_Neighbour(const EXGeoPath *p, int n, int k)
{
    AirPathLink *l = Air_Link(p, Air_Node(p, n)->Links()[k]);
    int next = l->m_A;
    if (next == n) next = l->m_B;
    return next;
}
static __forceinline EXVector Air_NodePos(const EXGeoPath *p, int n)
{
    const EXVector3 &v = Air_Node(p, n)->m_Pos;
    return EXVector(v.x, v.y, v.z, 0.0f);
}

// Catmull-Rom point and tangent on the segment p1..p2 (x/z only for the tangent heading helper).
static __forceinline EXVector Air_Spline(const EXVector3 &p0, const EXVector3 &p1, const EXVector3 &p2,
                                         const EXVector3 &p3, float t)
{
    float t2 = t * t, t3 = t2 * t;
    return EXVector((p1.x * 2.0f + (p2.x - p0.x) * t + (p0.x * 2.0f - p1.x * 5.0f + p2.x * 4.0f - p3.x) * t2
                     + (p1.x * 3.0f - p2.x * 3.0f + p3.x - p0.x) * t3) * 0.5f,
                    (p1.y * 2.0f + (p2.y - p0.y) * t + (p0.y * 2.0f - p1.y * 5.0f + p2.y * 4.0f - p3.y) * t2
                     + (p1.y * 3.0f - p2.y * 3.0f + p3.y - p0.y) * t3) * 0.5f,
                    (p1.z * 2.0f + (p2.z - p0.z) * t + (p0.z * 2.0f - p1.z * 5.0f + p2.z * 4.0f - p3.z) * t2
                     + (p1.z * 3.0f - p2.z * 3.0f + p3.z - p0.z) * t3) * 0.5f,
                    0.0f);
}
static __forceinline EXVector Air_SplineTangent(const EXVector3 &p0, const EXVector3 &p1, const EXVector3 &p2,
                                                const EXVector3 &p3, float t)
{
    float t2 = t * t * 3.0f, tt = t * 2.0f;
    return EXVector(((p2.x - p0.x) + (p0.x * 2.0f - p1.x * 5.0f + p2.x * 4.0f - p3.x) * tt
                     + (p1.x * 3.0f - p2.x * 3.0f + p3.x - p0.x) * t2) * 0.5f,
                    ((p2.y - p0.y) + (p0.y * 2.0f - p1.y * 5.0f + p2.y * 4.0f - p3.y) * tt
                     + (p1.y * 3.0f - p2.y * 3.0f + p3.y - p0.y) * t2) * 0.5f,
                    ((p2.z - p0.z) + (p0.z * 2.0f - p1.z * 5.0f + p2.z * 4.0f - p3.z) * tt
                     + (p1.z * 3.0f - p2.z * 3.0f + p3.z - p0.z) * t2) * 0.5f,
                    0.0f);
}

// Civilian air traffic: fly a closed loop path as a Catmull-Rom spline through the nodes around
// m_Node at constant speed, nose along the tangent, banking by half the heading change between
// 0.1 behind and 0.1 ahead on the curve.
void XItemHandler_AirVehicle::UpdateTraffic()
{
    EXGeoPath *pPath = &(*theItemEnv.GetPaths(EXMapOn(-1)))[m_Path];
    int n = pPath->NumNodes();
    int i0 = m_Node - 1;
    if (i0 < 0) i0 = n - 1;
    int i1 = m_Node;
    int i2 = m_Node + 1;
    if (i2 >= n) i2 -= n;
    int i3 = m_Node + 2;
    if (i3 >= n) i3 -= n;
    const EXVector3 &p0 = Air_Node(pPath, i0)->m_Pos;
    const EXVector3 &p1 = Air_Node(pPath, i1)->m_Pos;
    const EXVector3 &p2 = Air_Node(pPath, i2)->m_Pos;
    const EXVector3 &p3 = Air_Node(pPath, i3)->m_Pos;
    EXVector pos = Air_Spline(p0, p1, p2, p3, m_PathT);
    EXVector tan = Air_SplineTangent(p0, p1, p2, p3, m_PathT);
    float len = EXSqrt(tan.x * tan.x + tan.y * tan.y + tan.z * tan.z);
    EXVector ahead;
    if (m_PathT > 0.9f) {
        short i4 = (short)(m_Node + 3);
        if (i4 >= n) i4 = (short)(i4 - n);
        ahead = Air_SplineTangent(p1, p2, p3, Air_Node(pPath, i4)->m_Pos, m_PathT - 0.9f);
    } else {
        ahead = Air_SplineTangent(p0, p1, p2, p3, m_PathT + 0.1f);
    }
    float hAhead = qatan2f(ahead.x, ahead.z);
    EXVector behind;
    if (m_PathT < 0.1f) {
        short im = (short)(m_Node - 2);
        if (im < 0) im = (short)(im + n);
        behind = Air_SplineTangent(Air_Node(pPath, im)->m_Pos, p0, p1, p2, m_PathT + 0.9f);
    } else {
        behind = Air_SplineTangent(p0, p1, p2, p3, m_PathT - 0.1f);
    }
    float hBehind = qatan2f(behind.x, behind.z);
    float bank = AngleDiff(hAhead, hBehind);
    m_pItem->SetPosition(pos);
    float yaw = qatan2f(tan.x, tan.z);
    float pitch = qatan2f(tan.y, EXSqrt(tan.z * tan.z + tan.x * tan.x));
    m_pItem->SetRotation(EXQuaternion(-pitch, yaw, bank * 0.5f, 0.0f));
    m_PathT = m_Speed / len * XApp::GameFrameRateMul() + m_PathT;
    if (m_PathT >= 1.0f) {
        m_PathT = m_PathT - 1.0f;
        m_Node = i2;
    }
}

// Patrol (state 1): wander the path from node to node (random free neighbour, not straight back),
// idling in a slow turn between hops; on spotting the Predator switch to pursuit (state 2) with the
// lights on. Pursuit: hop to the neighbour nearest the target while that gains 4 units, otherwise
// hold and face the target; gunships (m_Unk112) fire: the helicopter skin in 8 shot cannon bursts,
// others alternating cannon bursts and rocket pairs. Lose the target for 10s to go back to patrol.
// The hover bob is applied on top of m_Pos.
void XItemHandler_AirVehicle::UpdatePatrol()
{
    EXGeoPath *pPath = &(*theItemEnv.GetPaths(EXMapOn(-1)))[m_Path];
    m_Time += XApp::FrameLength();
    if (m_pMuzzleScript && m_pMuzzleScript->m_11a) {
        m_pMuzzleScript->ItemFlags().m_flags |= 0x10;
        m_pMuzzleScript = 0;
    }
    float st = 0.0f;
    if (m_SoundTimer > 0.0f) st = m_SoundTimer - XApp::FrameLength();
    m_SoundTimer = st;
    if (m_State == 2) {
        if (!CanSeePredator()) {
            m_Timer = m_Timer + XApp::FrameLength();
            if (m_Timer >= 10.0f) {
                m_State = 1;
                DeleteBluesAndTwos();
            }
        } else {
            m_Target = gpPlayerItem[0]->Position();
            m_Target.y = m_Target.y + 1.5f;
            m_Timer = 0.0f;
        }
        if (!m_bMoving) {
            float best = (Air_NodePos(pPath, m_Node) - m_Target).LengthXYZ() - 4.0f;
            int bestNode = -1;
            for (u16 i = 0; i < Air_Node(pPath, m_Node)->m_nLinks; i++) {
                int next = Air_Neighbour(pPath, m_Node, i);
                float d = (Air_NodePos(pPath, next) - m_Target).LengthXYZ();
                if (d < best) {
                    best = d;
                    bestNode = next;
                }
            }
            if (bestNode != -1 && !NodeOccupied(bestNode)) {
                m_Node = bestNode;
                m_bMoving = 1;
            }
            if (!m_bMoving) {
                EXVector d = m_Target - m_Pos;
                float yaw = qatan2f(d.x, d.z);
                float pitch = 0.0f - qatan2f(d.y, EXSqrt(d.z * d.z + d.x * d.x));
                EXItem *pItem = m_pItem;
                float ry = pItem->m_rotation.y;
                float rw = pItem->m_rotation.w;
                float rx = pItem->m_rotation.x;
                float rz = pItem->m_rotation.z;
                rx = Vec_AngleDifference(rx, pitch) * 0.125f + rx;
                float dyaw = Vec_AngleDifference(m_pItem->m_rotation.y, yaw);
                float droll = Vec_AngleDifference(m_pItem->m_rotation.z, 0.0f);
                if (m_Unk112 == 1) {
                    rx = rx * 0.95f;
                    if (rx >= 0.25f) rx = 0.25f;
                    else if (rx <= -0.25f) rx = -0.25f;
                }
                m_pItem->SetRotation(EXQuaternion(rx, dyaw * 0.125f + ry, droll * 0.125f + rz, rw));
                if (m_SoundTimer <= 0.0f && CanSeePredator()) {
                    m_SoundTag.StartSound(0x1af0063e, m_pItem->m_position, -1, 0);
                    m_SoundTimer = g_EXRandClass.Randf() * 10.0f + 3.0f;
                }
                if (m_Unk112 == 1) {
                    if (m_FireTimer > 0.0f) m_FireTimer -= XApp::FrameLength();
                    if (m_FireTimer <= 0.0f && CanSeePredator()) {
                        int skin = AP_GetPtr(m_Profile)->m_Skins[0];
                        EXDatumTemp dt;
                        memset(&dt, 0, sizeof dt);
                        if (skin == 0xd0000c2) {  // HT_AnimSkin_FV_Helicopper
                            if (m_pItem->GetAnimatorDatum(0x10000088, dt, 1, 0)) {  // HT_AnimDatum_AV_GunMuzzle
                                WeaponProfile::GetWeaponData(0x49000051);  // HT_Weaponry_Gunship_Cannon
                                m_Burst++;
                                float cd = 0.1f;
                                if (m_Burst > 7) {
                                    m_Burst = 0;
                                    cd = 1.0f;
                                }
                                m_FireTimer = cd;
                                XGetSoundManager()->StartSound(0x1af00135, dt.m_position, -1, 0);
                                PlayMuzzleScript();
                                if (m_pTracers[m_NextTracer])
                                    m_pTracers[m_NextTracer]->Init(dt.m_position, m_Target - dt.m_position,
                                                                   WeaponProfile::GetWeaponDataIndex(0x49000051), 0);
                                m_NextTracer = (m_NextTracer + 1) % 8;
                            }
                        } else {
                            if (m_Burst == 0) m_Burst = g_EXRandClass.Rand32() % 10 > 2 ? 10 : -4;
                            if (m_Burst < 0) {
                                // HT_AnimDatum_AV_RightMissile / LeftMissile alternately
                                if (m_pItem->GetAnimatorDatum(0x100000c2 - ((m_Burst & 1) != 0), dt, 1, 0)) {
                                    WeaponProfile *pW = WeaponProfile::GetWeaponData(0x49000052);  // HT_Weaponry_Gunship_Rocket
                                    EXVector dir = gpPlayerItem[0]->Position() - dt.m_position;
                                    XItemHandler_Bullet::CreateFromHash(dt.m_position, gpPlayerItem[0]->m_position, dir,
                                                                        0x49000052, 0, 0, 1, 1.0f);
                                    m_FireTimer = 0.5f;
                                    XGetSoundManager()->StartSound(pW->m_SfxFire, dt.m_position, -1, 0);
                                }
                                if (++m_Burst == 0) m_FireTimer = 1.0f;
                            } else {
                                if (m_pItem->GetAnimatorDatum(0x10000088, dt, 1, 0)) {
                                    WeaponProfile *pW = WeaponProfile::GetWeaponData(0x49000051);
                                    m_FireTimer = 0.15f;
                                    XGetSoundManager()->StartSound(pW->m_SfxFire, dt.m_position, -1, 0);
                                    PlayMuzzleScript();
                                    if (m_pTracers[m_NextTracer])
                                        m_pTracers[m_NextTracer]->Init(dt.m_position, m_Target - dt.m_position,
                                                                       WeaponProfile::GetWeaponDataIndex(0x49000051), 0);
                                    m_NextTracer = (m_NextTracer + 1) % 8;
                                }
                                if (--m_Burst == 0) m_FireTimer = 1.0f;
                            }
                        }
                    }
                }
            }
        }
    } else if (m_State == 1) {
        if (!m_bMoving) {
            if (CanSeePredator()) {
                m_State = 2;
                m_Target = gpPlayerItem[0]->Position();
                m_Target.y = m_Target.y + 1.5f;
                m_Timer = 0.0f;
                CreateBluesAndTwos();
            } else {
                m_Timer -= XApp::FrameLength();
                if (m_Timer <= 0.0f) {
                    int next = 0;
                    m_Timer = 0.0f;
                    s8 bFound = 0;
                    for (int tries = 0; tries < 6; tries++) {
                        if (bFound) break;
                        u32 n = Air_Node(pPath, m_Node)->m_nLinks;
                        u32 k = n ? g_EXRandClass.Rand32() % n : 0;
                        next = Air_Neighbour(pPath, m_Node, k);
                        if (!NodeOccupied(next)) bFound = 1;
                        if (next == m_PrevNode && tries < 5) bFound = 0;
                    }
                    if (bFound) {
                        m_PrevNode = m_Node;
                        m_Node = next;
                        m_bMoving = 1;
                    } else {
                        m_Timer = 3.0f;
                    }
                }
            }
            if (!m_bMoving) {
                EXItem *pItem = m_pItem;
                float ry = pItem->m_rotation.y;
                if (m_Node & 1) ry = ry + 0.01f;
                else ry = ry - 0.01f;
                pItem->SetRotation(EXQuaternion(pItem->m_rotation.x * 0.95f, ry, pItem->m_rotation.z, pItem->m_rotation.w));
            }
        }
    }
    if (m_bMoving == 1) {
        EXVector d = Air_NodePos(pPath, m_Node) - m_Pos;
        float sq = d.y * d.y + d.z * d.z + d.x * d.x;
        float len = EXSqrt(sq);
        if (sq >= 0.0f && len >= 0.2f) {
            if (m_Speed * m_Speed * 499.99997f < len) {
                float s = XApp::GameFrameRateMul() * 0.01f + m_Speed;
                m_Speed = s < m_MaxSpeed ? s : m_MaxSpeed;
                if (m_State == 1) {
                    float half = m_MaxSpeed * 0.5f;
                    m_Speed = m_Speed < half ? m_Speed : half;
                }
            } else {
                m_Speed = m_Speed - XApp::GameFrameRateMul() * 0.001f;
            }
            d.Normalise();
            m_Pos += d * m_Speed * XApp::GameFrameRateMul();
            if (m_State == 2) d = m_Target - m_Pos;
            float yaw = qatan2f(d.x, d.z);
            float pitch = 0.0f - qatan2f(d.y, EXSqrt(d.z * d.z + d.x * d.x));
            EXItem *pItem = m_pItem;
            float ry = pItem->m_rotation.y;
            float rx = pItem->m_rotation.x;
            float rw = pItem->m_rotation.w;
            float rz = pItem->m_rotation.z;
            rx = Vec_AngleDifference(rx, pitch) * 0.055555556f + rx;
            float dyaw = Vec_AngleDifference(m_pItem->m_rotation.y, yaw);
            float droll = Vec_AngleDifference(m_pItem->m_rotation.z, 0.0f);
            if (m_Unk112 == 1) {
                if (rx >= 0.25f) rx = 0.25f;
                else if (rx <= -0.25f) rx = -0.25f;
            }
            m_pItem->SetRotation(EXQuaternion(rx, dyaw * 0.055555556f + ry, droll * 0.055555556f + rz, rw));
        } else {
            m_Timer = 3.0f;
            m_bMoving = 0;
            m_Speed = 0.0f;
        }
    }
    m_pItem->SetPosition(EXVector(m_Pos.x + sin(m_Time * 2.2f) * 0.125f, m_Pos.y + cos(m_Time * 2.0f) * 0.125f,
                                  m_Pos.z + cos(m_Time * 1.7f) * 0.125f, m_Pos.w));
}

s8 XItemHandler_AirVehicle::DoUpdate() { return 1; }
