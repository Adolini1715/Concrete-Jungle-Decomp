#include "FEText.h"
#include "XItemHandler_Camera.h"
#include "XItemHandler_Player.h"
#include "XItem.h"
#include "Actor.h"
#include "Vehicle.h"
#include "TrafficSystem.h"
#include "EXBaseSoundManager.h"
#include "XItemHandler_AirVehicle.h"
#include "XItemHandler_JumpTarget.h"
#include "EXBaseGamePad.h"
#include "XGameWnd.h"
#include "EXCamera.h"
#include "EXBaseDisplay.h"
#include "EXItemAnimator_Script.h"
#include "GameData.h"
#include "ActorProfile.h"
#include "XItemHandler_Mimicry.h"
#include "XItemHandler_Turret.h"
#include "XItemHandler_CCTV.h"
#include "XItemHandler_VMO.h"
#include "EXDatum.h"
#include <math.h>

extern unsigned int gCurrentMap_GeoHash;
extern unsigned int gSystemCameraUser;
extern XGameWnd *gpGameWnd[];
float Vec_AngleDifference(float a, float b);
signed char LineOfSight(EXVector &from, EXVector &to, signed char b);
signed char LineOfSightThermal(EXVector &from, EXVector &to);

// NEAR (see check): locks onto the nearest live traffic vehicle (not the one being ridden) closer than
// maxDist, within 30 degrees of the camera heading and in sight from the camera; returns its distance.
float XItemHandler_Camera::LockNearestVehicle(float maxDist)
{
    if (!TrafficIsInit()) return maxDist;
    TrafficPopulace &pop = TrafficGetPopulace();
    unsigned int n = pop.getNumVehicles();
    for (int i = 0; i < (int)n; i++) {
        Vehicle *pVehicle = pop.getVehicle(i);
        if (!pVehicle->getTypeObject()) continue;
        EXItem *pItem = pVehicle->m_pItem;
        if (!pItem || !(pVehicle->m_health > 0.0f) || pItem == XItemHandler_Player::GetInstance()->GetStandOn()) continue;
        EXItem *pMe = gpPlayerItem[m_ViewIndex];
        EXVector d = pItem->Position() - pMe->Position();
        float l = d.y * d.y + d.z * d.z + d.x * d.x;
        float dist = 0.0f;
        if (!(l < 0.0f)) dist = (float)sqrt(l);
        if (!(dist < maxDist)) continue;
        if (!((float)fabs(Vec_AngleDifference((float)atan2(d.x, d.z), m_Angle)) < 0.52359879f)) continue;
        EXVector p(pItem->Position().x, pItem->Position().y + 1.0f, pItem->Position().z, pItem->Position().w);
        if (!LineOfSight(m_CameraPos, p, 0)) continue;
        maxDist = l < 0.0f ? 0.0f : (float)sqrt(l);
        m_pVehicleTarget = (XItem *)pItem;
        EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00201);
    }
    return maxDist;
}

// NEAR (see check): locks onto the nearest jump target between 4 and maxDist away, within 30 degrees of
// the camera heading and in sight; returns its distance.
float XItemHandler_Camera::LockNearestJumpTarget(float maxDist)
{
    for (XItem *pItem = gJumpTargetList.GetFirst(); pItem; pItem = gJumpTargetList.GetNext(pItem)) {
        EXItem *pMe = gpPlayerItem[m_ViewIndex];
        EXVector d = pItem->Position() - pMe->Position();
        float l = d.y * d.y + d.z * d.z + d.x * d.x;
        if (l < 0.0f) continue;
        float dist = (float)sqrt(l);
        if (!(dist > 4.0f) || !(dist < maxDist)) continue;
        if ((float)fabs(Vec_AngleDifference((float)atan2(d.x, d.z), m_Angle)) < 0.52359879f &&
            LineOfSight(m_CameraPos, pItem->m_position, 1)) {
            m_pJumpTarget = pItem;
            maxDist = dist;
        }
    }
    return maxDist;
}

// NEAR (see check): locks onto the nearest active air vehicle (not leaving, not ridden) closer than
// maxDist, within 30 degrees of the camera heading and in sight; returns its distance.
float XItemHandler_Camera::LockNearestAirVehicle(float maxDist)
{
    for (XItem *pItem = gAirVehicleList.GetFirst(); pItem; pItem = gAirVehicleList.GetNext(pItem)) {
        XItemHandler_AirVehicle *pAir = (XItemHandler_AirVehicle *)pItem->m_pItemHandler;
        if (!pAir->m_Unk110 || pAir->m_State == 4 || pItem == XItemHandler_Player::GetInstance()->GetStandOn()) continue;
        EXItem *pMe = gpPlayerItem[m_ViewIndex];
        EXVector d = pItem->Position() - pMe->Position();
        float l = d.y * d.y + d.z * d.z + d.x * d.x;
        float dist = 0.0f;
        if (!(l < 0.0f)) dist = (float)sqrt(l);
        if (!(dist < maxDist)) continue;
        if ((float)fabs(Vec_AngleDifference((float)atan2(d.x, d.z), m_Angle)) < 0.52359879f &&
            LineOfSight(m_CameraPos, pItem->m_position, 0)) {
            maxDist = l < 0.0f ? 0.0f : (float)sqrt(l);
            m_pLockTarget[0] = pItem;
        }
    }
    return maxDist;
}

// NEAR (see check): a lock-on candidate: alive (or a boss whose head can be ripped) and on screen. The
// POW predator is only lockable outside first person; Ronin only in first person on the Empire levels;
// everything else must be targetable, fully visible and not lying down in first person.
s8 XItemHandler_Camera::IsActorValidTarget(Actor *pActor)
{
    if (!pActor || (pActor->m_Health <= 0.0f && !pActor->m_StateFlags.m_bCanRipBossHead) || !pActor->m_StateFlags.m_bOnScreen24)
        return 0;
    if (pActor->GetProfile()->m_SubGroup == 0x4f000017) return !IsFirstPerson();
    if (pActor->GetProfile()->m_SubGroup == 0x4f000011) {
        if ((gCurrentMap_GeoHash == 0x1000124 || gCurrentMap_GeoHash == 0x100013b) && !IsFirstPerson()) return 0;
        return pActor->IsTargetable();
    }
    if (pActor->GetProfile()->m_SubGroup == 0x4f000017) return IsFirstPerson() != 0;
    return pActor->IsTargetable() && pActor->IsSolid() && (!pActor->m_StateFlags.m_bLyingOnGround || !m_bFirstPerson);
}

// The current lock-on and its kind: 1 actor, 2 traffic vehicle, 4 jump target, 3 air vehicle, 7 turret,
// 5 CCTV, 6 VMO; only while aiming (mode 1 or first person).
void *XItemHandler_Camera::GetLockOn(int &type)
{
    if (m_Mode == 1 || m_bFirstPerson) {
        if (m_TargetID) {
            type = 1;
            return Actor::GetPtrFromID(m_TargetID);
        }
        if (m_pVehicleTarget) {
            type = 2;
            return m_pVehicleTarget;
        }
        if (m_pJumpTarget) {
            type = 4;
            return m_pJumpTarget;
        }
        if (m_pLockTarget[0]) {
            type = 3;
            return m_pLockTarget[0];
        }
        if (m_pLockTarget[1]) {
            type = 7;
            return m_pLockTarget[1];
        }
        if (m_pLockTarget[2]) {
            type = 5;
            return m_pLockTarget[2];
        }
        if (m_pLockTarget[3]) {
            type = 6;
            return m_pLockTarget[3];
        }
    }
    type = 0;
    return 0;
}

// NEAR (see check): third-person auto target. Keeps a valid target within 5.5, then picks the actor
// within 5 that best lines up with where the player is attacking (his facing, reversed for attacks
// behind, or the stick direction, then only within 45 degrees), scoring angle times distance.
void XItemHandler_Camera::AutoTarget()
{
    if (m_Mode == 2) return;
    if (m_TargetID > 0) {
        Actor *pActor = Actor::GetPtrFromID(m_TargetID);
        if (IsActorValidTarget(pActor)) {
            EXItem *pMe = gpPlayerItem[m_ViewIndex];
            EXVector d = pActor->GetItem()->Position() - pMe->Position();
            float l = d.y * d.y + d.z * d.z + d.x * d.x;
            if (l < 0.0f || !((float)sqrt(l) > 5.5f)) goto pick;
        }
        m_TargetID = -1;
    }
pick:
    float best = 9999.9f;
    EXItem *pMe = gpPlayerItem[m_ViewIndex];
    int id = 0;
    float dir = pMe->Rotation().y;
    signed char bStick = 0;
    if (XItemHandler_Player::GetInstance()->IsAttackingBehind()) dir += 3.1415927f;
    u8 pad = m_PadIndex;
    EXBaseGamePad *pPad = EXBaseGamePad::GetGamePad();
    if ((float)fabs(pPad->Analog(pad, (EXPad::AnalogChan)1)) > 0.25f || (float)fabs(pPad->Analog(pad, (EXPad::AnalogChan)0)) > 0.25f) {
        bStick = 1;
        dir = (float)atan2(pPad->Analog(pad, (EXPad::AnalogChan)1), pPad->Analog(pad, (EXPad::AnalogChan)0)) + m_Angle + 1.5707964f;
    }
    EXVector pos = pMe->Position();
    for (Actor *pActor = Actor::GetFirst(pos, 5.0f, 0); pActor; pActor = pActor->GetNext(pos, 5.0f, 0)) {
        if (!IsActorValidTarget(pActor) || pActor->m_StateFlags.m_bCanRipBossHead || pActor->m_StateFlags.m_bLyingOnGround) continue;
        EXVector d = pActor->GetItem()->Position() - pos;
        float a = Vec_AngleDifference((float)atan2(d.x, d.z), dir);
        if (bStick && !((float)fabs(a) <= 0.78539819f)) continue;
        float l = d.y * d.y + d.z * d.z + d.x * d.x;
        float dist = l < 0.0f ? 0.0f : (float)sqrt(l);
        float score = (float)fabs(a) * dist * 5.0f;
        if (score < best) {
            id = pActor->GetRefID();
            best = score;
        }
    }
    if (id != m_TargetID) m_TargetID = id;
}

// NEAR (see check): flicks the lock-on with a stick (the right stick, or in first person the left stick
// first per the controller config): picks the on-screen actor within 35 (in sight) or traffic vehicle
// within 30 (XZ) nearest on screen to the current target in the pushed direction; a vehicle wins. A new
// actor target restarts the marker script and pushes an honourable old target onto the hit list.
void XItemHandler_Camera::MoveLockon()
{
    XGameWnd *pWnd = gpGameWnd[0];
    if (m_LockDelay) {
        m_LockDelay--;
        return;
    }
    if (gSystemCameraUser != 1) {
        EXBaseCamera *pCam = (EXBaseCamera *)&gpGameWnd[0]->m_Camera;
        pCam->SetClipSize(640, 640, 0);
        ((EXBaseCamera *)&gpGameWnd[0]->m_Camera)->SetOrigin(((EXBaseCamera *)&gpGameWnd[0]->m_Camera)->Origin());
        XItemHandler_Camera *pHandler = XGetCamera(0);
        EXRectf r((float)gpGameWnd[0]->PrimRect().x, (float)gpGameWnd[0]->PrimRect().y, (float)gpGameWnd[0]->PrimRect().w, (float)gpGameWnd[0]->PrimRect().h);
        ((EXCommonCamera *)&gpGameWnd[0]->m_Camera)->SetVFOV(r, 0.7853982f / pHandler->Zoom());
        ((EXBaseCamera *)&gpGameWnd[0]->m_Camera)->UpdateDisplay(0);
        gSystemCameraUser = 1;
    }
    Actor *pBest = 0;
    Actor *pCur = Actor::GetPtrFromID(m_TargetID);
    float best = 1e8f;
    EXVector me = gpPlayerItem[m_ViewIndex]->Position();
    unsigned int dir = 0;
    XItem *pBestVehicle = 0;
    EXItem *pFrom = pCur ? pCur->GetItem() : (EXItem *)m_pVehicleTarget;
    EXVector2 from = EXBaseDisplay::WorldToDisp(pFrom->Position());
    EXBaseGamePad *pPad;
    if (!m_bFirstPerson) {
        u8 pad = m_PadIndex;
        pPad = EXBaseGamePad::GetGamePad();
        if (pPad->Analog(pad, (EXPad::AnalogChan)2) > 0.5f) dir = 8;
        if (pPad->Analog(pad, (EXPad::AnalogChan)2) < -0.5f) dir |= 4;
        if (pPad->Analog(pad, (EXPad::AnalogChan)3) > 0.5f) dir |= 1;
        if (pPad->Analog(pad, (EXPad::AnalogChan)3) < -0.5f) dir |= 2;
        if (!dir) return;
    } else {
        if (GameData::Instance(1)->m_Values.m_ButtonConfig != 1) {
            u8 pad = m_PadIndex;
            pPad = EXBaseGamePad::GetGamePad();
            if (pPad->Analog(pad, (EXPad::AnalogChan)0) > 0.5f) dir = 8;
            if (pPad->Analog(pad, (EXPad::AnalogChan)0) < -0.5f) dir |= 4;
            if (pPad->Analog(pad, (EXPad::AnalogChan)1) > 0.5f) dir |= 1;
            if (pPad->Analog(pad, (EXPad::AnalogChan)1) < -0.5f) dir |= 2;
        }
        if (GameData::Instance(1)->m_Values.m_ButtonConfig == 2) {
            if (!dir) return;
        } else if (!dir) {
            u8 pad = m_PadIndex;
            pPad = EXBaseGamePad::GetGamePad();
            if (pPad->Analog(pad, (EXPad::AnalogChan)2) > 0.5f) dir = 8;
            if (pPad->Analog(pad, (EXPad::AnalogChan)2) < -0.5f) dir |= 4;
            if (pPad->Analog(pad, (EXPad::AnalogChan)3) > 0.5f) dir |= 1;
            if (pPad->Analog(pad, (EXPad::AnalogChan)3) < -0.5f) dir |= 2;
            if (!dir) return;
        }
    }
    for (Actor *pActor = Actor::GetFirst(1); pActor; pActor = pActor->GetNext(1)) {
        if (pActor == pCur || !IsActorValidTarget(pActor)) continue;
        EXItem *pItem = pActor->GetItem();
        EXVector d = pItem->Position() - me;
        if (!(d.x * d.x + d.y * d.y + d.z * d.z < 1225.0f)) continue;
        EXVector2 off = EXBaseDisplay::WorldToDisp(pItem->Position());
        off -= from;
        if (!InDirectionOf(off, dir)) continue;
        float l = off.LengthSquared();
        if (!(l < best)) continue;
        EXVector p(pActor->GetItem()->Position().x, pActor->GetItem()->Position().y + 1.0f, pActor->GetItem()->Position().z,
                   pActor->GetItem()->Position().w);
        if (LineOfSight(m_CameraPos, p, 1)) {
            best = l;
            pBest = pActor;
        }
    }
    if (TrafficIsInit()) {
        unsigned int n = TrafficGetPopulace().getNumVehicles();
        if (n) {
            for (int i = 0; i < (int)n; i++) {
                Vehicle *pVehicle = TrafficGetPopulace().getVehicle(i);
                if (!pVehicle->getTypeObject() || !pVehicle->m_pItem || !(pVehicle->m_health > 0.0f) || !pVehicle->isOnScreen()) continue;
                XItem *pItem = (XItem *)pVehicle->m_pItem;
                if (pItem == XItemHandler_Player::GetInstance()->GetStandOn() || pItem == m_pVehicleTarget ||
                    !(pItem->ItemFlags() & 1) || (pItem->ClipFlags() & 3))
                    continue;
                EXVector d = pItem->Position() - gpPlayerItem[m_ViewIndex]->Position();
                float l = d.z * d.z + d.x * d.x;
                if (!(l < 0.0f) && !((float)sqrt(l) < 30.0f)) continue;
                EXVector2 off = EXBaseDisplay::WorldToDisp(pItem->Position());
                off -= from;
                if (!InDirectionOf(off, dir)) continue;
                float sl = off.LengthSquared();
                if (sl < best) {
                    best = sl;
                    pBestVehicle = pItem;
                }
            }
            if (pBestVehicle) goto vehicle;
        }
    }
    if (pBest) {
        if (pBest->GetRefID() != m_TargetID) {
            ((EXItemAnimator_Script *)m_pLockIndicator->GetAnimator())->SetScriptStatus(2);
            EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00201);
            if (pCur && pCur->GetProfile()->m_Honour > 0) {
                m_HitList[2] = m_HitList[1];
                m_HitList[1] = m_HitList[0];
                m_HitList[0] = m_TargetID;
            }
        }
        m_TargetID = pBest->GetRefID();
        m_LockDelay = 10;
        m_pVehicleTarget = 0;
    }
    if (!pBestVehicle) return;
vehicle:
    m_pVehicleTarget = pBestVehicle;
    m_LockDelay = 10;
    m_TargetID = 0;
    EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00201);
}

// NEAR (see check): locks onto the nearest valid actor closer than maxDist that the thermal line of sight
// from the player's head reaches, in up to three passes: honourable targets within 72 degrees of his
// facing, then honourable ones anywhere, then dishonourable ones (potc_fut: everyone, two passes).
// A new target restarts and resets the marker; returns its distance.
float XItemHandler_Camera::LockNearestActor(float maxDist)
{
    int pass = 0;
    signed char bHonour = 1;
    if (gCurrentMap_GeoHash == 0x100011c) bHonour = 0;
    while (bHonour || pass != 2) {
        for (Actor *pActor = Actor::GetFirst(1); pActor; pActor = pActor->GetNext(1)) {
            if (!IsActorValidTarget(pActor)) continue;
            if (bHonour) {
                if (pass == 2) {
                    if (pActor->GetProfile()->m_Honour >= 0) continue;
                } else {
                    if (pActor->GetProfile()->m_Honour < 0) continue;
                }
            }
            EXItem *pMe = gpPlayerItem[m_ViewIndex];
            EXVector d = pActor->GetItem()->Position() - pMe->Position();
            float l = d.y * d.y + d.z * d.z + d.x * d.x;
            float dist = 0.0f;
            if (!(l < 0.0f)) dist = (float)sqrt(l);
            if (!(dist < maxDist)) continue;
            float a = Vec_AngleDifference((float)atan2(d.x, d.z), pMe->Rotation().y);
            if (pass) a = 0.0f;
            if (!((float)fabs(a) < 1.2566371f)) continue;
            EXItem *pP = gpPlayerItem[m_ViewIndex];
            EXVector from(pP->Position().x, pP->Position().y + 1.5f, pP->Position().z, pP->Position().w);
            EXVector to(from.x + d.x, d.y + from.y, d.z + from.z, from.w);
            if (!LineOfSightThermal(from, to)) continue;
            maxDist = l < 0.0f ? 0.0f : (float)sqrt(l);
            m_TargetID = pActor->GetRefID();
            ((EXItemAnimator_Script *)m_pLockIndicator->GetAnimator())->SetScriptStatus(2);
            EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00201);
            XItem *pMark = m_pLockIndicator;
            pMark->m_rotation = EXVector(0.0f, 0.0f, 0.0f, 0.0f);
            pMark->m_validMtx = 0;
        }
        if (m_TargetID || ++pass == 3) break;
    }
    return maxDist;
}

// NEAR (see check): turns the camera to look at a point (from the player, or from the camera in first
// person, where third person aims 0.2 radians lower); straight down while riding a vehicle.
void XItemHandler_Camera::PointCameraAt(EXVector &p)
{
    EXItem *pMe = gpPlayerItem[m_ViewIndex];
    float x = pMe->Position().x;
    float y = pMe->Position().y;
    float z = pMe->Position().z;
    if (m_bFirstPerson) {
        x = m_CameraPos.x;
        y = m_CameraPos.y;
        z = m_CameraPos.z;
    }
    float dx = p.x - x;
    float py = p.y;
    float dz = p.z - z;
    float h = dz * dz + dx * dx;
    m_TargetAngle = 1.5707964f - (float)atan2(dz, dx);
    float e = 1.5707964f - (float)atan2(py - y, h < 0.0f ? 0.0f : (float)sqrt(h));
    m_TargetElevation = e;
    if (!m_bFirstPerson) m_TargetElevation = e + 0.19634955f;
    if (XItemHandler_Player::GetInstance()->GetMode() == 0x14) m_TargetElevation = 1.5707964f;
}

// NEAR (see check): how far a point is from the first-person aim: its screen distance from the centre
// times its distance from the camera (999999 behind the camera).
float XItemHandler_Camera::GetFPDistance(EXVector &p)
{
    EXVector look = m_CameraLookAt - m_CameraPos;
    EXVector d = p - m_CameraPos;
    if (d.y * look.y + d.z * look.z + d.w * look.w + d.x * look.x < 0.0f) return 999999.0f;
    EXVector2 s = EXBaseDisplay::WorldToDisp(p);
    float sx = s.x - (float)(EXBaseDisplay::GetDisplay()->Width() >> 1);
    float sy = s.y - (float)(EXBaseDisplay::GetDisplay()->Height() >> 1);
    float l = d.y * d.y + d.z * d.z + d.x * d.x;
    float dist = l < 0.0f ? 0.0f : (float)sqrt(l);
    float sl = sy * sy + sx * sx;
    return (sl < 0.0f ? 0.0f : (float)sqrt(sl)) * dist;
}

// NEAR (see check): whether a point lies within range of from, within maxYaw of the heading and
// within maxPitch of level.
s8 XItemHandler_Camera::PointWithin(EXVector &from, EXVector &to, float range, float heading, float maxYaw, float maxPitch)
{
    EXVector d = to - from;
    if (!(d.y * d.y + d.z * d.z + d.x * d.x <= range * range)) return 0;
    if (!((float)fabs(Vec_AngleDifference((float)atan2(d.x, d.z), heading)) < maxYaw)) return 0;
    float h = d.z * d.z + d.x * d.x;
    if (!((float)fabs((float)atan2(d.y, h < 0.0f ? 0.0f : (float)sqrt(h))) < maxPitch)) return 0;
    return 1;
}

// NEAR (see check): whether an actor can be aimed at from a point: a unit above it (along its own up
// for aliens on walls), within range*k (wall crawling: any direction, but only on-screen actors unless
// in thermal vision; otherwise within 30*k degrees of the camera heading, 7 radians in thermal, and
// 90*k degrees of level), and in sight.
s8 XItemHandler_Camera::IsActorViableTarget(Actor *pActor, EXVector &from, float range, float k)
{
    EXItem *pItem = pActor->GetItem();
    EXVector p = pItem->Position();
    EXVector up(0.0f, 1.0f, 0.0f, 0.0f);
    if (pActor->m_ActorFlags.m_bAlien) {
        EXMatrix m = pItem->m_mtx;
        EXVector o;
        D3DXVec4Transform(&o, &up, &m);
        up = o;
    }
    p += up;
    if (XItemHandler_Player::GetInstance()->GetMode() == 0x18) {
        EXVector d = p - from;
        if (!(d.y * d.y + d.z * d.z + d.x * d.x < range * k * range * k)) return 0;
        if (XItemHandler_Player::GetInstance()->GetVisionMode() != 1 && !pActor->m_StateFlags.m_bOnScreen) return 0;
    } else {
        float yaw = k * 0.52359879f;
        if (XItemHandler_Player::GetInstance()->GetVisionMode() == 1) yaw = 7.0f;
        if (!PointWithin(from, p, range * k, m_Angle, yaw, k * 1.5707964f)) return 0;
    }
    return LineOfSight(from, p, 1);
}

// First-person lock-ons (LockCentred*): the candidate nearest the aim (GetFPDistance) and in sight wins;
// a new pick restarts its marker's script, no pick clears the lock.

// NEAR (see check): actors, by their approximate chest (thermal line of sight).
float XItemHandler_Camera::LockCentredActor(float best)
{
    Actor *pBest = 0;
    for (Actor *pActor = Actor::GetFirst(1); pActor; pActor = pActor->GetNext(1)) {
        if (!IsActorValidTarget(pActor)) continue;
        EXVector p = pActor->GetChestPosApprox();
        float d = GetFPDistance(p);
        if (d < best && LineOfSightThermal(m_CameraPos, p)) {
            pBest = pActor;
            best = d;
        }
    }
    if (pBest) {
        if (pBest->GetRefID() != m_TargetID) {
            m_TargetID = pBest->GetRefID();
            ((EXItemAnimator_Script *)m_pLockIndicator->GetAnimator())->SetScriptStatus(2);
        }
        return best;
    }
    m_TargetID = 0;
    return best;
}

// NEAR (see check): live traffic vehicles (not the ridden one), half a unit above their origin.
float XItemHandler_Camera::LockCentredVehicle(float best)
{
    XItem *pBest = 0;
    if (TrafficIsInit()) {
        TrafficPopulace &pop = TrafficGetPopulace();
        unsigned int n = pop.getNumVehicles();
        if (n) {
            for (int i = 0; i < (int)n; i++) {
                Vehicle *pVehicle = pop.getVehicle(i);
                if (!pVehicle->getTypeObject()) continue;
                XItem *pItem = (XItem *)pVehicle->m_pItem;
                if (!pItem || !(pVehicle->m_health > 0.0f) || pItem == XItemHandler_Player::GetInstance()->GetStandOn()) continue;
                EXVector p(pItem->Position().x, pItem->Position().y + 0.5f, pItem->Position().z, pItem->Position().w);
                float d = GetFPDistance(p);
                if (d < best && LineOfSight(m_CameraPos, p, 0)) {
                    best = d;
                    pBest = pItem;
                }
            }
            if (pBest) {
                if (pBest != m_pVehicleTarget) {
                    m_pVehicleTarget = pBest;
                    ((EXItemAnimator_Script *)m_pVehicleIndicator->GetAnimator())->SetScriptStatus(2);
                }
                return best;
            }
        }
    }
    m_pVehicleTarget = 0;
    return best;
}

// NEAR (see check): active air vehicles not leaving.
float XItemHandler_Camera::LockCentredAirVehicle(float best)
{
    XItem *pBest = 0;
    for (XItem *pItem = gAirVehicleList.GetFirst(); pItem; pItem = gAirVehicleList.GetNext(pItem)) {
        XItemHandler_AirVehicle *pAir = (XItemHandler_AirVehicle *)pItem->m_pItemHandler;
        if (!pAir->m_Unk110 || pAir->m_State == 4) continue;
        EXVector p = pItem->Position();
        float d = GetFPDistance(p);
        if (d < best && LineOfSight(m_CameraPos, p, 0)) {
            pBest = pItem;
            best = d;
        }
    }
    if (pBest) {
        if (pBest != m_pLockTarget[0]) {
            m_pLockTarget[0] = pBest;
            ((EXItemAnimator_Script *)m_pLockIndicator->GetAnimator())->SetScriptStatus(2);
        }
        return best;
    }
    m_pLockTarget[0] = 0;
    return best;
}

// NEAR (see check): active vocal mimicry points at least a unit from the camera; one that starts on its
// flag channel plays at once instead of being locked.
float XItemHandler_Camera::LockCentredMimicry(float best)
{
    XItem *pBest = 0;
    for (XItem *pItem = gMimicryList.GetFirst(); pItem; pItem = gMimicryList.GetNext(pItem)) {
        if (!((XItemHandler_Mimicry *)pItem->m_pItemHandler)->m_bActive) continue;
        EXVector p = pItem->Position();
        EXVector c = p - m_CameraPos;
        if (!(c.y * c.y + c.z * c.z + c.x * c.x >= 1.0f)) continue;
        float d = GetFPDistance(p);
        if (d < best && LineOfSight(m_CameraPos, p, 0)) {
            pBest = pItem;
            best = d;
        }
    }
    if (pBest) {
        if (pBest == m_pLockTarget[1]) return best;
        XItemHandler_Mimicry *pM = (XItemHandler_Mimicry *)pBest->m_pItemHandler;
        if (!(pM->m_Unk08 & 1)) {
            m_pLockTarget[1] = pBest;
            ((EXItemAnimator_Script *)m_pLockIndicator->GetAnimator())->SetScriptStatus(2);
            return best;
        }
        pM->Play();
    }
    m_pLockTarget[1] = 0;
    return best;
}

// NEAR (see check): turrets not destroyed.
float XItemHandler_Camera::LockCentredTurret(float best)
{
    XItem *pBest = 0;
    for (XItem *pItem = gTurretList.GetFirst(); pItem; pItem = gTurretList.GetNext(pItem)) {
        if (((XItemHandler_Turret *)pItem->m_pItemHandler)->m_State == 3) continue;
        EXVector p = pItem->Position();
        float d = GetFPDistance(p);
        if (d < best && LineOfSight(m_CameraPos, p, 0)) {
            pBest = pItem;
            best = d;
        }
    }
    if (pBest) {
        if (pBest != m_pLockTarget[2]) {
            m_pLockTarget[2] = pBest;
            ((EXItemAnimator_Script *)m_pLockIndicator->GetAnimator())->SetScriptStatus(2);
        }
        return best;
    }
    m_pLockTarget[2] = 0;
    return best;
}

// NEAR (see check): CCTV cameras not destroyed.
float XItemHandler_Camera::LockCentredCCTV(float best)
{
    XItem *pBest = 0;
    for (XItem *pItem = gCCTVList.GetFirst(); pItem; pItem = gCCTVList.GetNext(pItem)) {
        if (((XItemHandler_CCTV *)pItem->m_pItemHandler)->m_State == 3) continue;
        EXVector p = pItem->Position();
        float d = GetFPDistance(p);
        if (d < best && LineOfSight(m_CameraPos, p, 0)) {
            pBest = pItem;
            best = d;
        }
    }
    if (pBest) {
        if (pBest != m_pLockTarget[3]) {
            m_pLockTarget[3] = pBest;
            ((EXItemAnimator_Script *)m_pLockIndicator->GetAnimator())->SetScriptStatus(2);
        }
        return best;
    }
    m_pLockTarget[3] = 0;
    return best;
}

// NEAR (see check): scannable objects with scan text (shares the CCTV slot; never cleared here).
float XItemHandler_Camera::LockCentredVMO(float best)
{
    XItem *pBest = 0;
    for (XItem *pItem = gVMOList.GetFirst(); pItem; pItem = gVMOList.GetNext(pItem)) {
        if (!((XItemHandler_VMO *)pItem->m_pItemHandler)->m_ScanText) continue;
        EXVector p = pItem->Position();
        float d = GetFPDistance(p);
        if (d < best && LineOfSight(m_CameraPos, p, 0)) {
            pBest = pItem;
            best = d;
        }
    }
    if (pBest && pBest != m_pLockTarget[3]) {
        m_pLockTarget[3] = pBest;
        ((EXItemAnimator_Script *)m_pLockIndicator->GetAnimator())->SetScriptStatus(2);
    }
    return best;
}

// NEAR (see check): the auto target for the ranged body weapons (6 the disk: 15 + 5 per upgrade, 7 the
// shoulder cannon: 30 + 10 per upgrade) from the cannon muzzle. A current target stays while it is a
// valid auto target and viable (5% slack); otherwise the farthest viable one is taken and its marker
// restarted.
void XItemHandler_Camera::RangeTarget()
{
    EXDatumTemp d;
    memset(&d, 0, sizeof d);
    ((EXItemAnimator *)XItemHandler_Player::GetInstance()->GetWeaponItem(10))->GetDatum(0x10000037, d, 1, EXItemAnimator::m_pNullDatumPtr);
    EXVector muzzle;
    D3DXVec4Transform(&muzzle, &d.m_position, &gpPlayerItem[m_ViewIndex]->m_mtx);
    float range;
    XItemHandler_Player *pPlayer = XItemHandler_Player::GetInstance();
    if (pPlayer->GetCurrentWeapon(0) == 6)
        range = (float)pPlayer->GetWeaponUpgradeArray()[6] * 5.0f + 15.0f;
    else if (pPlayer->GetCurrentWeapon(0) == 7)
        range = (float)pPlayer->GetWeaponUpgradeArray()[7] * 10.0f + 30.0f;
    else
        return;
    if (m_PlasmaID > 0) {
        Actor *pActor = Actor::GetPtrFromID(m_PlasmaID);
        if (!IsActorValidAutoTarget(pActor) || !IsActorViableTarget(pActor, muzzle, range, 1.05f)) m_PlasmaID = -1;
    }
    if (m_PlasmaID < 1) {
        float best = 0.0f;
        int id = 0;
        for (Actor *pActor = Actor::GetFirst(1); pActor; pActor = pActor->GetNext(1)) {
            if (!IsActorValidAutoTarget(pActor) || !IsActorViableTarget(pActor, muzzle, range, 1.0f)) continue;
            EXVector v = pActor->GetItem()->Position() - muzzle;
            float l = v.y * v.y + v.z * v.z + v.x * v.x;
            if (best < l) {
                id = pActor->GetRefID();
                best = l;
            }
        }
        if (id != m_PlasmaID) {
            m_PlasmaID = id;
            ((EXItemAnimator_Script *)m_pPlasmaIndicator->GetAnimator())->SetScriptStatus(2);
        }
    }
}

void XItemHandler_Camera::ClearLockon()
{
    m_pVehicleTarget = 0;
    m_pJumpTarget = 0;
    m_pLockTarget[0] = 0;
    m_pLockTarget[1] = 0;
    m_pLockTarget[2] = 0;
    m_pLockTarget[3] = 0;
    m_bFirstPerson = 0;
    m_SoundTag.RemoveAll(1);
    m_LostSight = 0;
}

void XItemHandler_Camera::ClearLockonX()
{
    m_pVehicleTarget = 0;
    m_pJumpTarget = 0;
    m_pLockTarget[0] = 0;
    m_pLockTarget[1] = 0;
    m_pLockTarget[2] = 0;
    m_pLockTarget[3] = 0;
    m_SoundTag.RemoveAll(1);
    m_LostSight = 0;
}

s8 XItemHandler_Camera::IsActorValidTarget(int id) { return IsActorValidTarget(Actor::GetPtrFromID(id)); }

s8 XItemHandler_Camera::InDirectionOf(EXVector2 &dir, unsigned int flags)
{
    if ((flags & 1) && dir.y <= 0.0f) return 0;
    if ((flags & 2) && dir.y >= 0.0f) return 0;
    if ((flags & 4) && dir.x >= 0.0f) return 0;
    if ((flags & 8) && dir.x <= 0.0f) return 0;
    if ((flags == 1 || flags == 2) && (float)fabs(dir.x) > (float)fabs(dir.y)) return 0;
    if ((flags == 4 || flags == 8) && (float)fabs(dir.y) > (float)fabs(dir.x)) return 0;
    return 1;
}

s8 XItemHandler_Camera::IsActorValidAutoTarget(Actor *pActor)
{
    if (!pActor || pActor->m_Health <= 0.0f || !pActor->IsTargetable() || pActor->m_Alpha != 1.0f ||
        (pActor->m_StateFlags.m_bLyingOnGround && m_bFirstPerson) || pActor->GetProfile()->m_Honour <= 0)
        return 0;
    unsigned int aff = Actor::GlobalAffiliation();
    if (aff != 0xffffffff && pActor->Affiliation() != aff) return 0;
    return 1;
}

s8 XItemHandler_Camera::IsActorValidTargetLite(Actor *pActor)
{
    if (!pActor) return 0;
    if (pActor->m_Health <= 0.0f && !pActor->m_StateFlags.m_bCanRipBossHead) return 0;
    if (pActor->GetProfile()->m_SubGroup == 0x4f000011) {
        if ((gCurrentMap_GeoHash == 0x1000124 || gCurrentMap_GeoHash == 0x100013b) && !IsFirstPerson()) return 0;
        return pActor->IsTargetable();
    }
    if (pActor->GetProfile()->m_SubGroup == 0x4f000017) return IsFirstPerson() != 0;
    return pActor->IsTargetable() != 0;
}
