#include "FEText.h"
#include "XItemHandler_Camera.h"
#include "XItemHandler_Player.h"
#include "XItem.h"
#include "EXWatcher.h"
#include "EXMemory.h"
#include "Vehicle.h"
#include "XApp.h"
#include "Actor.h"
#include "ActorUtil.h"
#include "EXCamera.h"
#include "EXRayCastTemp.h"
#include "EXItemAnimator.h"
#include "ScriptRunner.h"
#include "XItemHandler_VMO.h"
#include "XGameWnd.h"
#include "EXDisplay.h"
#include "EXGeoEntity.h"
#include "EXSphere.h"
#include "GameData.h"
#include "ActorProfile.h"
#include "EXBaseGamePad.h"
#include "EXItemAnimator_Script.h"
#include "EXGeoFile.h"
#include "EXGeoMap.h"
#include "EXGeoPathNode.h"
#include "EXLine.h"
#include "VisionWipe.h"
#include "XItemHandler_Environment.h"
#include "FEMainMenu.h"
#include "ActorOptions.h"
#include "XGamePad.h"
#include "EXBaseSoundManager.h"
#include "EXGeoTrigger.h"
#include <math.h>

// TODO stand-ins: other areas.
inline EXWatcher *XGetWatcher() { return EXWatcher::GetWatcher(); }

// Camera tuning values (watcher-editable).
float CameraDistance = 7.0f;
float FP_Sensitivity = 2.0f;
short FP_Power = 3;
float PeerHorizontal = 0.5f;
float PeerVertical = 0.23333333f;
float AngleDiff(float a, float b);
extern unsigned int gSystemCameraUser;
extern XGameWnd *gpGameWnd[];
float PrevInterp = 1.0f;    // lock-on look-at blend (SetCamThirdPerson)
float Wobbler;              // first-person idle sway phase
float Fisher;               // look-at height lift for the current distance/elevation
int PrevID;                 // lock-on target the blend belongs to
int ZargMode;               // follow the player's heading after a mode 2 (jump) start, until the stick moves
static s8 sManualCam;       // camera style 1 or the right stick moved this frame: no auto-rotation
static float sFollow;       // auto-rotation follow accumulator (0..1.2)
static float sPrevHeading;  // player heading last frame
float Vec_AngleDifference(float a, float b);
signed char LineOfSight(EXVector &from, EXVector &to, signed char b);
s8 Within(EXVector p, EXGeoTriggerDatum d);
extern float gCamRotation;
extern unsigned int gCurrentMap_GeoHash;
extern XItemList gEnvList;

XItemHandler_Camera *XGetCamera(unsigned int i)
{
    XItem *pItem = gpCameraItem[i];
    if (pItem) return (XItemHandler_Camera *)pItem->ItemHandler();
    return 0;
}

EX_IMPLEMENT_DYNCREATE_ROOT(XItemHandler_Camera)

XItemHandler_Camera::~XItemHandler_Camera()
{
    gpCameraItem[m_ViewIndex] = 0;
    XGetWatcher()->DelSection(XGetWatcher()->SectionHandle("Camera", 0));
    m_SoundTag.RemoveAll(1);
}

s8 XItemHandler_Camera::IsFirstPerson()
{
    if (m_Mode == 2) return 1;
    if (m_Mode == 1 && m_bFirstPerson) return 1;
    return 0;
}

void XItemHandler_Camera::SetFirstPerson()
{
    m_TargetDistance = 0.0f;
    if (m_Mode != 1) m_TargetID = 0;
    m_Mode = 2;
    XItemHandler_Player::GetInstance()->SetAnalogMask(0xff);
    XItemHandler_Player::GetInstance()->SetButtonMask(0x3a);
    XItemHandler_Player::GetInstance()->Hide(1, 0);
    switch (XItemHandler_Player::GetInstance()->GetMode()) {
    case 0xc:
        if (m_bFPFace) {
            float a = gpPlayerItem[m_ViewIndex]->Rotation().y;
            m_Angle = a;
            m_TargetAngle = a;
            m_Elevation = 1.4137167f;
            m_TargetElevation = 1.4137167f;
        }
        break;
    case 0x18: {
        float a = 0.0f;
        m_Angle = a;
        m_TargetAngle = a;
        m_Elevation = 1.5707964f;
        m_TargetElevation = 1.5707964f;
        break;
    }
    default:
        if (m_bFPFace) {
            float a = gpPlayerItem[m_ViewIndex]->Rotation().y;
            m_Angle = a;
            m_TargetAngle = a;
            m_Elevation = 1.5707964f;
            m_TargetElevation = 1.5707964f;
        }
        break;
    }
}

void XItemHandler_Camera::SetThirdPerson()
{
    m_TargetAngle = gpPlayerItem[m_ViewIndex]->Rotation().y;
    m_TargetElevation = m_28;
    m_TargetDistance = CameraDistance;
    m_Mode = 0;
    m_bFirstPerson = 0;
    if (XItemHandler_Player::GetInstance()->GetMode() == 0x18) {
        m_TargetElevation = 1.9634954f;
        m_TargetAngle = 0.0f;
    }
    XItemHandler_Player::GetInstance()->SetAnalogMask(0);
    XItemHandler_Player::GetInstance()->SetButtonMask(0);
    XItemHandler_Player::GetInstance()->Hide(0, 0);
}

// NEAR: the by-value EXVector difference is not inlined as in retail.
float XItemHandler_Camera::Angle()
{
    if (XItemHandler_Player::GetInstance() && XItemHandler_Player::GetInstance()->GetMode() == 0x18) {
        EXVector d = m_CameraLookAt - m_CameraPos;
        return (float)atan2(d.x, d.z);
    }
    return m_Angle;
}

void XItemHandler_Camera::VehicleRemoved(Vehicle *pVehicle)
{
    if (this && m_pVehicleTarget && pVehicle->m_pItem == m_pVehicleTarget) m_pVehicleTarget = 0;
}

void XItemHandler_Camera::CameraPostUpdate() {}

// Third-person framing: the look-at point (player height by movement mode, special cases for
// hanging, heavy finishers and lock-on), smoothed toward its target; the distance, limited by
// five "whisker" rays and eased in or out; then the camera position on the sphere around the
// look-at point from m_Angle / m_Elevation. The angle chase itself is in DoUpdate.
// GUESS: written from the disassembly for behaviour (not byte-matched yet).
void XItemHandler_Camera::SetCamThirdPerson()
{
    XItemHandler_Player *pPlayer = XItemHandler_Player::GetInstance();
    EXVector look;
    pPlayer->GetLookAt(look);
    look.w = 0.0f;
    float base = pPlayer->GetBaseHeight();
    float h;
    switch (pPlayer->m_Mode) {
    case 2: {
        float c = pPlayer->GetCrouchTimer() * 3.0f;
        float one = 1.0f;
        h = (1.88f - (c < one ? c : one) * 0.88f) + base;
        break;
    }
    case 0x11:
    case 0x12:
        h = base + 1.0f;
        break;
    case 0x14:
        h = base;
        break;
    default:
        h = base + 1.88f;
        break;
    }
    float d = 3.1415927f - m_Distance;
    float f = (float)sin((d > 0.0f ? d : 0.0f) * 0.5f) * 0.4f;
    if (m_Elevation > 1.5707964f)
        Fisher = (3.1415927f - m_Elevation) * f * 0.63661975f;
    else
        Fisher = f;
    look.y = Fisher + h;
    if (pPlayer->m_Mode == 0 && pPlayer->m_SubMode == 4) look.y = (Fisher + h) - 1.0f;

    if (m_Mode == 1) {
        Actor *pActor = Actor::GetPtrFromID(m_TargetID);
        if (pActor && pActor->m_SM.GetState() == STATE_HeavyFinish) {
            EXVector c = pActor->GetChestPos();
            look.x = (c.x + look.x) * 0.5f;
            look.y = (c.y + look.y) * 0.5f;
            look.z = (c.z + look.z) * 0.5f;
            look.w = (c.w + look.w) * 0.5f;
            m_TargetDistance = 15.0f;
        }
    }
    if (pPlayer->m_Mode == 4 && (pPlayer->m_SubMode == 10 || pPlayer->m_SubMode == 9)) {
        look = pPlayer->GetTopHeadPos();
        m_LookSmooth = 10.0f;
    }
    m_Warp = 10.0f;

    float side = 0.0f;
    if (pPlayer->m_Mode == 0xc) side = 1.0f;
    if (pPlayer->m_Mode == 4) {
        m_LookSmooth = 10.0f;
        switch (pPlayer->m_SubMode) {
        case 0:
        case 1:
            m_LookSmooth = 20.0f;
            break;
        case 5:
        case 6:
        case 7:
            side = 1.0f;
            break;
        }
    }
    if (m_SideOffset > side) {
        float v = m_SideOffset - 0.1f;
        m_SideOffset = side >= v ? side : v;
    }
    if (side > m_SideOffset) {
        float v = m_SideOffset + 0.1f;
        m_SideOffset = v >= side ? side : v;
    }
    if (m_SideOffset != 0.0f) {
        float rot = gpPlayerItem[m_ViewIndex]->Rotation().y;
        look.x = look.x - (float)sin(rot) * m_SideOffset;
        look.z = look.z - (float)cos(rot) * m_SideOffset;
    }

    float n = m_LookSmooth / XApp::GameFrameRateMul();
    float one = 1.0f;
    float k = one / (one >= n ? one : n);
    EXVector delta;
    delta.x = (look.x - m_CameraLookAt.x) * k;
    delta.y = (look.y - m_CameraLookAt.y) * k;
    delta.z = (look.z - m_CameraLookAt.z) * k;
    delta.w = (look.w - m_CameraLookAt.w) * k;
    m_CameraLookAt.x += delta.x;
    m_CameraLookAt.y += delta.y;
    m_CameraLookAt.z += delta.z;
    m_CameraLookAt.w += delta.w;
    m_pCamera->SetTarget(m_CameraLookAt);

    if (m_Mode == 1) {
        float half = 0.5f;
        PrevInterp = PrevInterp * 0.95f;
        if (half >= PrevInterp) PrevInterp = half;
        if (PrevID != m_TargetID) PrevInterp = 1.0f;
        Actor *pActor = Actor::GetPtrFromID(m_TargetID);
        if (pActor) {
            EXVector c = pActor->GetChestPos();
            c.y -= 2.0f;
            float r = 1.0f - PrevInterp;
            EXVector t;
            t.x = m_CameraLookAt.x * PrevInterp + c.x * r;
            t.y = m_CameraLookAt.y * PrevInterp + c.y * r;
            t.z = m_CameraLookAt.z * PrevInterp + c.z * r;
            t.w = m_CameraLookAt.w * PrevInterp + c.w * r;
            m_pCamera->SetTarget(t);
        }
        PrevID = m_TargetID;
    } else {
        PrevID = 0;
    }

    if (!pPlayer->m_bScanMode) {
        float w0 = GetWhiskerDistance(0.0f, 0.0f, 0);
        float w1 = GetWhiskerDistance(0.1308997f, 0.0f, 0);
        float w2 = GetWhiskerDistance(-0.1308997f, 0.0f, 0);
        float w3 = GetWhiskerDistance(0.0f, 0.1308997f, 0);
        float w4 = GetWhiskerDistance(0.0f, -0.1308997f, 0);
        if (w1 < m_TargetDistance) m_TargetDistance = w1;
        if (w2 < m_TargetDistance) m_TargetDistance = w2;
        if (w3 < m_TargetDistance) m_TargetDistance = w3;
        if (w4 < m_TargetDistance) m_TargetDistance = w4;
        float div = 7.0f;
        if (m_TargetDistance > m_Distance) div = 30.0f;
        m_Distance = (m_TargetDistance - m_Distance) / div + m_Distance;
        if (m_Distance < 0.1f) m_Distance = 0.1f;
        if (w0 < m_Distance) m_Distance = w0;
    }

    float dist = m_Distance;
    float se = (float)sin(m_Elevation);
    float sa = -((float)sin(m_Angle) * se);
    float ca = -((float)cos(m_Angle) * se);
    float ce = -(float)cos(m_Elevation);
    m_CameraPosTarget.x = dist * sa + m_CameraLookAt.x;
    m_CameraPosTarget.y = m_CameraLookAt.y + dist * ce;
    m_CameraPosTarget.z = m_CameraLookAt.z + dist * ca;
    m_CameraPosTarget.w = m_CameraLookAt.w + dist * look.w;
}

// NEAR (see check; retail keeps the path-count loop unrotated and builds EXMapOn(-1) in the arg slot
// on every call). Spline camera: rides the nearest camera path (map path flag 0x400). The camera target is the point
// on the two path segments around the node nearest the player that is closest to the player (pushed
// back along the zone's map camera, if any), looking at the player's head.
void XItemHandler_Camera::SetCamSpline()
{
    float best = 3.4028235e38f;
    EXGeoPath *pPath = 0;
    int i;
    i = 0;
top:
    if (i < theItemEnv.GetPaths(EXMapOn(-1))->Size()) {
        EXGeoPath *p = &(*theItemEnv.GetPaths(EXMapOn(-1)))[i];
        if (p->Flags() & 0x400) {
            EXVector d = gpPlayerItem[0]->Position() - EXVector(p->Position(), 0.0f);
            EXVector t = d;
            float l = t.x * t.x + t.y * t.y + t.z * t.z;
            if (l < best) {
                best = l;
                pPath = p;
            }
        }
        i++;
        goto top;
    }
    if (pPath) {
        int n = pPath->NumNodes();
        int nearest = 0;
        best = 3.4028235e38f;
        for (i = 0; i < n; i++) {
            EXVector d = EXVector(pPath->Node(i)->Position(), 0.0f) - gpPlayerItem[0]->Position();
            EXVector t = d;
            float l = t.x * t.x + t.y * t.y + t.z * t.z;
            if (l < best) {
                nearest = i;
                best = l;
            }
        }
        int prev = nearest - 1;
        if (prev < 0) prev = 0;
        int next = nearest + 1;
        if (next >= n) next = n - 1;
        EXLineSeg s1(EXVector(pPath->Node(prev)->Position(), 0.0f),
                     EXVector(pPath->Node(nearest)->Position() - pPath->Node(prev)->Position(), 0.0f));
        EXLineSeg s2(EXVector(pPath->Node(nearest)->Position(), 0.0f),
                     EXVector(pPath->Node(next)->Position() - pPath->Node(nearest)->Position(), 0.0f));
        EXVector p = gpPlayerItem[0]->Position();
        EXGeoCamera *pMapCam = theItemEnv.GetCamera(p);
        if (pMapCam) {
            EXVector3 off = (pMapCam->m_Position - pMapCam->m_Target) * 5.0f;
            p.x += off.x;
            p.y += off.y;
            p.z += off.z;
        }
        EXVector n1, n2;
        s1.NearestTo(p, n1);
        s2.NearestTo(p, n2);
        EXVector d1 = n1 - gpPlayerItem[0]->Position();
        EXVector d2 = n2 - gpPlayerItem[0]->Position();
        EXVector t1 = d1;
        EXVector t2 = d2;
        if (t2.y * t2.y + t2.z * t2.z + t2.x * t2.x > t1.y * t1.y + t1.z * t1.z + t1.x * t1.x)
            m_CameraPosTarget = n1;
        else
            m_CameraPosTarget = n2;
        EXItem *pMe = gpPlayerItem[m_ViewIndex];
        EXVector look;
        look.x = pMe->Position().x;
        look.z = pMe->Position().z;
        look.y = XItemHandler_Player::GetInstance()->GetBaseHeight() + 1.8f;
        m_CameraLookAt.x = look.x;
        m_CameraLookAt.z = look.z;
        m_CameraLookAt.w = 0.0f;
        m_CameraLookAt.y = look.y;
        m_pCamera->SetTarget(m_CameraLookAt);
        EXVector d = EXVector(look.x, look.y, look.z, 0.0f) - m_CameraPos;
        float a2 = (float)atan2(d.x, d.z);
        EXVector t = d;
        float h = t.z * t.z + t.x * t.x;
        m_Angle = a2;
        m_TargetAngle = a2;
        float hl = h < 0.0f ? 0.0f : (float)sqrt(h);
        m_Mode = 0;
        float e = 1.5707964f - (float)atan2(d.y, hl);
        m_Elevation = e;
        m_TargetElevation = e;
    } else {
        m_Mode = 0;
    }
}

// Sign of f (-1, 0, 1) and f^FP_Power for the first-person stick response.
static __forceinline int CamSign(float f)
{
    if (f < 0.0f) return -1;
    if (f == 0.0f) return 0;
    return 1;
}
static __forceinline float CamStickPower(float s)
{
    float r = s;
    for (short i = 1; i < FP_Power; i++) r *= s;
    if (CamSign(r) != CamSign(s)) r = 0.0f - r;
    return r;
}

// NEAR (see check; one fewer call: the two zoom sounds share their StartSound and GameData lookup,
// where retail keeps separate lookups). The camera's frame: scripted mode 4 just pushes the script's position/look-at; otherwise the
// listener, mode 5 reset, stick input (third person turn, first person look with FP_Power response),
// idle orbit, auto target, lock-on button and lock-on target tracking, then the per-mode SetCam*.
s8 XItemHandler_Camera::DoUpdate()
{
    EXGamePad *pPad = XGetGamePad();
    if (m_Mode == 4) {
        m_pCamera->SetTarget(m_CameraLookAt);
        m_pCamera->SetPosition(m_CameraPos);
        GetItem()->SetPosition(m_CameraPos);
        EXVector d = m_CameraLookAt - m_CameraPos;
        float a = (float)atan2(d.x, d.z);
        EXVector t = d;
        float h = t.z * t.z + t.x * t.x;
        m_Angle = a;
        m_TargetAngle = a;
        float e = 1.5707964f - (float)atan2(t.y, h < 0.0f ? 0.0f : (float)sqrt(h));
        m_Elevation = e;
        m_TargetElevation = e;
        if (gCurrentMap_GeoHash == XApp::Frontend3DFile()) {
            m_pCamera->UpdateDisplay(0);
            FEMainMenu::SetCameraLookDispPos(EXBaseDisplay::WorldToDisp(m_CameraLookAt));
        }
        ClearLockon();
        m_pLockIndicator->ClrHasRender();
        m_pPlasmaIndicator->ClrHasRender();
        m_pVehicleIndicator->ClrHasRender();
        m_ListenerPos = m_CameraPos;
        m_pCamera->SetWorldUp(EXVector(0.0f, 1.0f, 0.0f, 0.0f));
        return 1;
    }
    if (m_Zoom == 1.0f) {
        m_ListenerPos = m_CameraPos;
    } else {
        Actor *pTarget = Actor::GetPtrFromID(m_TargetID);
        if (pTarget) {
            EXVector t = (pTarget->GetChestPosApprox() - m_CameraPos) * (1.0f / m_Zoom);
            m_ListenerPos += ((pTarget->GetChestPosApprox() - t) - m_ListenerPos) * 0.125f;
        } else {
            m_ListenerPos = m_CameraPos;
        }
    }
    m_CutsceneFOV = 0.0f;
    if (m_Mode == 12) {
        m_Mode = 0;
        return 1;
    }
    if (m_Mode == 5) {
        m_Angle = m_TargetAngle = gpPlayerItem[m_ViewIndex]->Rotation().y + gCamRotation;
        m_Elevation = m_TargetElevation = 1.9634955f;
        m_TargetDistance = CameraDistance;
        m_Distance = CameraDistance;
        m_2c = 1.0f;
        m_LookSmooth = 1.0f;
        m_Mode = 0;
        m_bFirstPerson = 0;
        gCamRotation = 0.0f;
        if (m_bNext) {
            m_Angle = m_TargetAngle = m_NextAngle;
            m_Elevation = m_TargetElevation = m_NextElevation;
            m_bNext = 0;
        }
    }
    for (Actor *pActor = Actor::GetFirst(1); pActor; pActor = pActor->GetNext(1)) {
        if (pActor->GetTargettedFlagChannel()) FlagChannels[pActor->GetTargettedFlagChannel()] = 0;
    }
    sManualCam = GameData::Instance(1)->m_Values.m_CameraMode == 1;
    XItemHandler_Player *pPlayer = (XItemHandler_Player *)gpPlayerItem[0]->ItemHandler();
    EXVector me = gpPlayerItem[m_ViewIndex]->Position();
    float invX = m_bInvertX ? -1.0f : 1.0f;
    float invY = m_bInvertY ? -1.0f : 1.0f;
    float invFPX = m_bInvertFPX ? -1.0f : 1.0f;
    float invFPY = m_bInvertFPY ? -1.0f : 1.0f;
    if (m_Mode != 2 && m_Mode != 1) {
        m_TargetAngle += pPad->Analog(m_PadIndex, (EXPad::AnalogChan)2) * invX * 0.0392625f * XApp::GameFrameRateMul();
        m_TargetElevation += pPad->Analog(m_PadIndex, (EXPad::AnalogChan)3) * invY * 0.03271875f * XApp::GameFrameRateMul();
    }
    if ((m_Mode == 2 || (m_Mode == 1 && m_bFirstPerson))) {
        if (pPlayer->GetHelmet() && ((XGamePad *)pPad)->XActionPressed(m_PadIndex, 0x4000) && m_Zoom < 8.0f) {
            m_Zoom *= 2.0f;
            if (m_Zoom > 8.0f) m_Zoom = 1.0f;
            unsigned int hash = 0x1af0055e;
            if (IsSkinClassic((short)GameData::Instance(1)->m_Values.m_Unk58)) hash = 0x1af00209;
            EXBaseSoundManager::GetSoundManager()->StartSound(hash, GetItem()->Position(), -1, 0);
            m_Warp = 10.0f;
        } else if (pPlayer->GetHelmet() && ((XGamePad *)pPad)->XActionPressed(m_PadIndex, 0x100) && m_Zoom > 1.0f) {
            m_Zoom *= 0.5f;
            if (m_Zoom < 1.0f) m_Zoom = 8.0f;
            unsigned int hash = 0x1af0055e;
            if (IsSkinClassic((short)GameData::Instance(1)->m_Values.m_Unk58)) hash = 0x1af00209;
            EXBaseSoundManager::GetSoundManager()->StartSound(hash, GetItem()->Position(), -1, 0);
            m_Warp = 10.0f;
        }
        if (pPlayer->m_MoveFlags & 0x400) {
            VisionWipe::Instance(1)->KillWipe();
            SetThirdPerson();
        }
    }
    if (m_Mode == 2) {
        float s;
        switch (GameData::Instance(1)->m_Values.m_ButtonConfig) {
        case 1:
            s = pPad->Analog(m_PadIndex, (EXPad::AnalogChan)2);
            break;
        case 2:
            s = pPad->Analog(m_PadIndex, (EXPad::AnalogChan)0);
            break;
        default:
            s = (float)fabs(pPad->Analog(m_PadIndex, (EXPad::AnalogChan)0)) > (float)fabs(pPad->Analog(m_PadIndex, (EXPad::AnalogChan)2))
                    ? pPad->Analog(m_PadIndex, (EXPad::AnalogChan)0)
                    : pPad->Analog(m_PadIndex, (EXPad::AnalogChan)2);
            break;
        }
        m_TargetAngle += CamStickPower(s) / m_Zoom * XApp::GameFrameRateMul() * FP_Sensitivity * invFPX * 0.015f;
        switch (GameData::Instance(1)->m_Values.m_ButtonConfig) {
        case 0:
            s = (float)fabs(pPad->Analog(m_PadIndex, (EXPad::AnalogChan)1)) > (float)fabs(pPad->Analog(m_PadIndex, (EXPad::AnalogChan)3))
                    ? pPad->Analog(m_PadIndex, (EXPad::AnalogChan)1)
                    : pPad->Analog(m_PadIndex, (EXPad::AnalogChan)3);
            break;
        case 1:
            s = pPad->Analog(m_PadIndex, (EXPad::AnalogChan)3);
            break;
        case 2:
            s = pPad->Analog(m_PadIndex, (EXPad::AnalogChan)1);
            break;
        }
        m_TargetElevation += CamStickPower(s) / m_Zoom * XApp::GameFrameRateMul() * FP_Sensitivity * invFPY * 0.015f;
    }
    if (!(m_Mode == 2 || (m_Mode == 1 && m_bFirstPerson))) {
        m_Lean = 0.0f;
        m_LeanRoll = 0.0f;
    }
    if (pPad->Analog(m_PadIndex, (EXPad::AnalogChan)2) != 0.0f || pPad->Analog(m_PadIndex, (EXPad::AnalogChan)3) != 0.0f)
        sManualCam = 1;
    m_IdleTime += EXBaseApp::FrameLength();
    if (pPad->Analog(m_PadIndex, (EXPad::AnalogChan)0) != 0.0f || pPad->Analog(m_PadIndex, (EXPad::AnalogChan)1) != 0.0f ||
        pPad->Analog(m_PadIndex, (EXPad::AnalogChan)2) != 0.0f || pPad->Analog(m_PadIndex, (EXPad::AnalogChan)3) != 0.0f ||
        ((XGamePad *)pPad)->XActionPressed(m_PadIndex, 0xffffffff) || (m_Mode == 2 || (m_Mode == 1 && m_bFirstPerson)))
        m_IdleTime = 0.0f;
    if (m_IdleTime > 60.0f) {
        m_TargetAngle += XApp::GameFrameRateMul() * 0.001f;
        m_TargetElevation = (float)sin(m_IdleTime * 0.05f) * 0.3926991f + 1.5707964f;
    }
    switch (XItemHandler_Player::GetInstance()->m_Mode) {
    case 4:
    case 5:
    case 12:
    case 19:
        m_IdleTime = 0.0f;
        break;
    case 18:
        if (m_IdleTime > 60.0f) m_TargetElevation += 0.3926991f;
        break;
    case 24:
        if (m_IdleTime > 60.0f) m_TargetElevation = 1.9634955f;
        break;
    }
    if (sManualCam) {
        m_KillAngle = -666.0f;
        m_KillElevation = -666.0f;
    }
    s8 bRanged = pPlayer->m_CurrentWeapon[0] >= 6 && pPlayer->m_CurrentWeapon[0] <= 7;
    if (m_Mode != 1 && m_Mode != 2) {
        AutoTarget();
        if (bRanged) RangeTarget();
        else m_PlasmaID = 0;
    }
    s8 bLock;
    switch (pPlayer->m_Mode) {
    case 4:
    case 5:
    case 24:
        bLock = 0;
        break;
    case 12:
    case 17:
        bLock = (m_Mode == 2 || (m_Mode == 1 && m_bFirstPerson));
        break;
    case 7: {
        s8 b = 1;
        if (pPlayer->GetSubMode() >= 0x10 && pPlayer->GetSubMode() <= 0x11) b = 0;
        bLock = !(m_Mode == 2 || (m_Mode == 1 && m_bFirstPerson)) && b;
        break;
    }
    default:
        bLock = 1;
        break;
    }
    if (bLock && ((XGamePad *)pPad)->XActionDown(m_PadIndex, 0x80)) {
        if (!m_bLockHeld) {
            if (m_Mode == 2) m_bFirstPerson = 1;
            if (m_Mode != 1) m_TargetID = 0;
            m_Mode = 1;
        }
    } else {
        m_bLockHeld = 0;
        if (m_Mode == 1) {
            if (m_bFirstPerson) {
                m_Mode = 2;
            } else {
                m_Mode = 0;
                m_TargetElevation = m_28;
            }
            m_TargetID = 0;
        }
    }
    if (pPlayer->m_CurrentWeapon[0] != 6 || m_Mode != 1) {
        m_HitList[0] = 0;
        m_HitList[1] = 0;
        m_HitList[2] = 0;
    }
    if (m_Mode == 1) {
        int type;
        if (!GetLockOn(type)) {
            if (m_bFirstPerson) {
                if (gSystemCameraUser != 1) {
                    EXBaseCamera *pCam = gpGameWnd[0]->GetCamera();
                    pCam->SetClipSize(640, 640, 0);
                    (gpGameWnd[0]->GetCamera())->SetOriginInline((gpGameWnd[0]->GetCamera())->Origin());
                    XItemHandler_Camera *pHandler = XGetCamera(0);
                    gpGameWnd[0]->GetCamera()->SetVFOV(gpGameWnd[0]->PrimRect(), 0.7853982f / pHandler->Zoom());
                    (gpGameWnd[0]->GetCamera())->UpdateDisplay(0);
                    gSystemCameraUser = 1;
                }
                float f = LockCentredActor(m_Zoom * 1600.0f);
                f = LockCentredVehicle(f);
                f = LockCentredAirVehicle(f);
                f = LockCentredMimicry(f);
                f = LockCentredTurret(f);
                f = LockCentredCCTV(f);
                LockCentredVMO(f);
                if (m_pLockTarget[3]) {
                    m_pLockTarget[2] = 0;
                    m_pLockTarget[1] = 0;
                    m_pLockTarget[0] = 0;
                    m_pVehicleTarget = 0;
                    m_TargetID = 0;
                }
                if (m_pLockTarget[2]) {
                    m_pLockTarget[1] = 0;
                    m_pLockTarget[0] = 0;
                    m_pVehicleTarget = 0;
                    m_TargetID = 0;
                }
                if (m_pLockTarget[1]) {
                    m_pLockTarget[0] = 0;
                    m_pVehicleTarget = 0;
                    m_TargetID = 0;
                }
                if (m_pLockTarget[0]) {
                    m_pVehicleTarget = 0;
                    m_TargetID = 0;
                }
                if (m_pVehicleTarget) m_TargetID = 0;
            } else {
                float f = LockNearestActor(30.0f);
                f = LockNearestVehicle(f);
                f = LockNearestJumpTarget(f);
                LockNearestAirVehicle(f);
                if (m_pLockTarget[0]) {
                    m_pJumpTarget = 0;
                    m_pVehicleTarget = 0;
                    m_TargetID = 0;
                }
                if (m_pJumpTarget) {
                    m_pVehicleTarget = 0;
                    m_TargetID = 0;
                }
                if (m_pVehicleTarget) m_TargetID = 0;
                m_pLockTarget[1] = 0;
            }
        }
        void *pLockOn = GetLockOn(type);
        switch (type) {
        case 1: {
            Actor *pActor = Actor::GetPtrFromID(m_TargetID);
            s8 bInRange = 0;
            if (pActor) {
                EXVector d = pActor->GetItem()->Position() - me;
                EXVector t = d;
                if (m_Zoom * m_Zoom * 1225.0f > t.y * t.y + t.z * t.z + t.x * t.x) bInRange = 1;
                if ((!pActor->m_StateFlags.m_bLyingOnGround || !m_bFirstPerson) && bInRange) {
                    EXVector chest = pActor->GetChestPos();
                    if (LineOfSight(m_CameraPos, chest, 0)) m_LostSight = 0;
                    else m_LostSight++;
                    if (m_LostSight <= 60 && IsActorValidTargetLite(pActor)) {
                        if (pActor->GetTargettedFlagChannel()) FlagChannels[pActor->GetTargettedFlagChannel()] = 1;
                        EXVector at = pActor->GetChestPos();
                        const EXVector &r = m_pLockIndicator->Rotation();
                        m_pLockIndicator->SetRotation(EXQuaternion(r.x, r.y + 0.05f, r.z, r.w));
                        if (!m_bFirstPerson) at.y -= 2.0f;
                        PointCameraAt(at);
                        if (pActor->m_SM.GetState() == STATE_HeavyFinish) {
                            m_LookSmooth = 10.0f;
                            m_2c = 10.0f;
                            m_TargetElevation = 3.1415927f;
                        }
                        u8 tint = XItemHandler_Player::GetInstance()->m_Tint;
                        if (tint == 1 || tint == 2) {
                            unsigned int hash;
                            switch (pActor->m_Status) {
                            case 3:
                            case 4:
                            case 5:
                            case 12:
                            case 13:
                                hash = 0x1af00561;
                                break;
                            case 6:
                            case 8:
                                hash = 0x1af00560;
                                break;
                            default:
                                hash = 0x1af0055f;
                                break;
                            }
                            if (!m_SoundTag.IsPlaying(hash)) {
                                m_SoundTag.RemoveAll(1);
                                m_SoundTag.StartSound(hash);
                            }
                        } else {
                            m_SoundTag.RemoveAll(1);
                        }
                        if (m_bFirstPerson) {
                            if ((float)fabs(pPad->Analog(m_PadIndex, (EXPad::AnalogChan)2)) > 0.5f ||
                                (float)fabs(pPad->Analog(m_PadIndex, (EXPad::AnalogChan)3)) > 0.5f ||
                                (float)fabs(pPad->Analog(m_PadIndex, (EXPad::AnalogChan)0)) > 0.5f ||
                                (float)fabs(pPad->Analog(m_PadIndex, (EXPad::AnalogChan)1)) > 0.5f)
                                MoveLockon();
                            else
                                m_LockDelay = 0;
                        } else {
                            if ((float)fabs(pPad->Analog(m_PadIndex, (EXPad::AnalogChan)2)) > 0.5f ||
                                (float)fabs(pPad->Analog(m_PadIndex, (EXPad::AnalogChan)3)) > 0.5f)
                                MoveLockon();
                            else
                                m_LockDelay = 0;
                        }
                        if (bRanged) m_PlasmaID = m_TargetID;
                        break;
                    }
                }
            }
            m_TargetID = -1;
            m_PlasmaID = -1;
            m_Mode = m_bFirstPerson ? 2 : 0;
            m_bFirstPerson = 0;
            m_bLockHeld = 1;
            break;
        }
        case 2: {
            XItem *pVehicle = m_pVehicleTarget;
            if (pVehicle == pPlayer->GetStandOn()) {
                m_Mode = 0;
                m_pVehicleTarget = 0;
                m_bLockHeld = 1;
                break;
            }
            if (pPlayer->m_Mode == 3 && pPlayer->GetSubMode() == 3) break;
            EXVector at = pVehicle->Position();
            if (m_bFirstPerson) at.y += 0.5f;
            else at.y -= 2.0f;
            PointCameraAt(at);
            if (m_bFirstPerson) {
                if ((float)fabs(pPad->Analog(m_PadIndex, (EXPad::AnalogChan)2)) > 0.5f ||
                    (float)fabs(pPad->Analog(m_PadIndex, (EXPad::AnalogChan)3)) > 0.5f ||
                    (float)fabs(pPad->Analog(m_PadIndex, (EXPad::AnalogChan)0)) > 0.5f ||
                    (float)fabs(pPad->Analog(m_PadIndex, (EXPad::AnalogChan)1)) > 0.5f)
                    MoveLockon();
                else
                    m_LockDelay = 0;
            } else {
                if ((float)fabs(pPad->Analog(m_PadIndex, (EXPad::AnalogChan)2)) > 0.5f ||
                    (float)fabs(pPad->Analog(m_PadIndex, (EXPad::AnalogChan)3)) > 0.5f)
                    MoveLockon();
                else
                    m_LockDelay = 0;
            }
            break;
        }
        case 4: {
            EXVector d = m_pJumpTarget->Position() - me;
            EXVector t = d;
            float l = t.z * t.z + t.y * t.y + t.x * t.x;
            if (l < 0.0f || 1.0f > (float)sqrt(l)) m_pJumpTarget = 0;
            else PointCameraAt((EXVector &)m_pJumpTarget->Position());
            break;
        }
        case 3:
        case 5:
        case 6:
        case 7: {
            XItem *pLock = (XItem *)pLockOn;
            EXVector d = pLock->Position() - me;
            EXVector t = d;
            if (t.y * t.y + t.z * t.z + t.x * t.x > m_Zoom * m_Zoom * 2025.0f) {
                m_pLockTarget[0] = 0;
                m_pLockTarget[1] = 0;
                m_pLockTarget[2] = 0;
                m_pLockTarget[3] = 0;
            } else {
                PointCameraAt((EXVector &)pLock->Position());
            }
            break;
        }
        default:
            m_bLockHeld = 1;
            m_Mode = m_bFirstPerson ? 2 : 0;
            m_bFirstPerson = 0;
            break;
        }
    } else {
        ClearLockon();
    }
    if (!(m_Mode == 2 || (m_Mode == 1 && m_bFirstPerson)) && ((XGamePad *)pPad)->XActionPressed(m_PadIndex, 0x100)) {
        switch ((int)CameraDistance) {
        case 7:
            CameraDistance = 10.0f;
            break;
        case 10:
            CameraDistance = 5.0f;
            break;
        default:
            CameraDistance = 7.0f;
            break;
        }
    }
    int type2;
    if (((XGamePad *)pPad)->XActionPressed(m_PadIndex, 0x80) && !GetLockOn(type2)) {
        if (m_bFirstPerson) {
            m_Mode = 2;
            m_bFirstPerson = 0;
        }
        if (m_Mode != 2) {
            if (pPlayer->m_Mode == 0x18) {
                m_TargetElevation = m_28;
                m_TargetAngle = 0.0f;
            } else {
                m_TargetAngle = gpPlayerItem[m_ViewIndex]->Rotation().y;
                m_TargetElevation = m_28;
            }
        }
    }
    if (((XGamePad *)pPad)->XActionPressed(m_PadIndex, 0x4000) && m_Mode == 0) {
        if (pPlayer->m_Mode == 0x18) {
            m_TargetElevation = m_28;
            m_TargetAngle = 0.0f;
        } else {
            m_TargetAngle = gpPlayerItem[m_ViewIndex]->Rotation().y;
            m_TargetElevation = m_28;
        }
    }
    if (((XGamePad *)pPad)->XActionPressed(m_PadIndex, 0x40) || m_bGoFirstPerson) {
        if (m_bFirstPerson) {
            if (!VisionWipe::Instance(1)->IsWiping()) VisionWipe::Instance(1)->Start(0x65);
        } else if (m_Mode == 2) {
            if (!m_bGoFirstPerson) VisionWipe::Instance(1)->Start(0x65);
        } else if (pPlayer->m_MoveFlags & 0x400) {
            pPlayer->PlayNegativeSound(0);
        } else {
            VisionWipe::Instance(1)->Start(0x64);
        }
    }
    if (pPlayer->m_CurrentWeapon[0] != 6 || m_Mode != 1) {
        m_HitList[0] = 0;
        m_HitList[1] = 0;
        m_HitList[2] = 0;
    }
    UpdateIndicators();
    unsigned int pm = pPlayer->m_Mode;
    if (pm == 1 && (pPlayer->GetSubMode() == 3 || pPlayer->GetSubMode() == 4)) {
        m_TargetAngle = gpPlayerItem[m_ViewIndex]->Rotation().y;
    } else if (!sManualCam && m_Mode == 0 && pm != 3 && pm != 12) {
        if (pm == 0x18) {
            if (GameData::Instance(1)->m_Values.m_CameraMode != 1 && m_TargetAngle != 0.0f &&
                0.0f > pPad->Analog(m_PadIndex, (EXPad::AnalogChan)1)) {
                float t = (float)fabs(m_TargetAngle);
                float lim = t >= 0.03f ? 0.03f : (0.0f < t ? t : 0.0f);
                m_TargetAngle -= (float)CamSign(m_TargetAngle) * lim;
            }
        } else if (GameData::Instance(1)->m_Values.m_CameraMode == 2) {
            if ((float)fabs(Vec_AngleDifference(m_Angle, pPlayer->GetItem()->Rotation().y)) > 0.2f) {
                float ly = pPad->Analog(m_PadIndex, (EXPad::AnalogChan)1);
                if (ly > 0.0f && 0.1f > (float)fabs(pPad->Analog(m_PadIndex, (EXPad::AnalogChan)0))) {
                    float v = ly * XApp::GameFrameRateMul() * 0.05235988f;
                    if (pPad->Analog(m_PadIndex, (EXPad::AnalogChan)0) < 0.0f) m_TargetAngle -= v;
                    else m_TargetAngle += v;
                } else {
                    m_TargetAngle += pPad->Analog(m_PadIndex, (EXPad::AnalogChan)0) * XApp::GameFrameRateMul() * 0.03f;
                }
            }
        } else {
            float rot = gpPlayerItem[m_ViewIndex]->Rotation().y;
            float acc;
            if ((float)fabs(Vec_AngleDifference(sPrevHeading, rot)) > 0.10908308f / XApp::GameFrameRateDiv() ||
                XItemHandler_Player::GetInstance()->GetSpeed() == 0.0f)
                acc = 0.0f;
            else
                acc = sFollow + XApp::GameFrameRateMul() * 0.04f;
            sPrevHeading = rot;
            sFollow = acc >= 1.2f ? 1.2f : (0.0f >= acc ? 0.0f : acc);
            float sp = XItemHandler_Player::GetInstance()->GetSpeed() + 0.5f;
            if (sFollow > 1.0f)
                m_TargetAngle += (sFollow - 1.0f) * 5.0f * 0.032f * pPad->Analog(m_PadIndex, (EXPad::AnalogChan)0) * sp * XApp::GameFrameRateMul();
        }
    }
    if (m_Mode < 0x5a && !(m_Mode == 2 || (m_Mode == 1 && m_bFirstPerson))) {
        s8 bChanged = m_PrevPlayerMode != pPlayer->m_Mode;
        if (pPlayer->m_Mode == 0x18) {
            if (bChanged) {
                EXMatrix mq;
                EXQuaternion q = XItemHandler_Player::GetInstance()->GetGeckoAngle();
                mq.set_rotate_quat(q);
                EXMatrix inv = mq.inverse();
                EXVector d = m_CameraLookAt - m_CameraPos;
                EXVector o;
                D3DXVec4Transform(&o, &d, &inv);
                float a = (float)atan2(o.x, o.z);
                EXVector t = o;
                float h = t.x * t.x + t.z * t.z;
                float hl = h < 0.0f ? 0.0f : (float)sqrt(h);
                float e = (float)atan2(t.y, hl);
                m_Angle = a;
                m_TargetElevation = 1.9634955f;
                m_TargetAngle = 0.0f;
                m_Elevation = 1.5707964f - e;
            }
            if (m_TargetAngle > 3.1415927f) m_TargetAngle -= 6.2831855f;
            if (-3.1415927f > m_TargetAngle) m_TargetAngle += 6.2831855f;
        }
        if (bChanged && m_PrevPlayerMode == 0x18) {
            EXVector d = m_CameraLookAt - m_CameraPos;
            float a = (float)atan2(d.x, d.z);
            EXVector t = d;
            float h = t.x * t.x + t.z * t.z;
            float hl = h < 0.0f ? 0.0f : (float)sqrt(h);
            float e = (float)atan2(t.y, hl);
            m_Angle = a;
            m_TargetAngle = a;
            m_2c = 50.0f;
            m_TargetElevation = m_28;
            m_LookSmooth = 10.0f;
            m_Elevation = 1.5707964f - e;
        }
        if (pPlayer->m_Mode == 10) {
            if (!sManualCam && pPlayer->GetSubMode() == 2) m_TargetAngle = gpPlayerItem[m_ViewIndex]->Rotation().y;
        } else if (!sManualCam && bChanged && (m_PrevPlayerMode == 9 || m_PrevPlayerMode == 3)) {
            m_TargetElevation = m_28;
        }
        if (pPlayer->m_Mode == 3 && !sManualCam && pPlayer->GetSubMode() == 11) {
            EXVector lt;
            float la;
            pPlayer->GetLedgeTarget(lt, la);
            float d = Vec_AngleDifference(m_Angle, la);
            float lim = XApp::GameFrameRateMul() * 0.3f;
            float nl = 0.0f - lim;
            m_TargetAngle = (d >= lim ? lim : (nl >= d ? nl : d)) + m_Angle;
        }
        if (pPlayer->m_Mode == 12 || pPlayer->m_Mode == 4) {
            if (m_PrevPlayerMode == 12 || m_PrevPlayerMode == 4) bChanged = 0;
            if (bChanged && !sManualCam) {
                m_TargetAngle = gpPlayerItem[m_ViewIndex]->Rotation().y;
                if (pPlayer->m_Mode != 10) m_TargetElevation = m_28;
            } else {
                float d = AngleDiff(m_TargetAngle, gpPlayerItem[m_ViewIndex]->Rotation().y);
                if (d > 1.5707964f) m_TargetAngle = gpPlayerItem[m_ViewIndex]->Rotation().y - 1.5707964f;
                if (-1.5707964f > d) m_TargetAngle = gpPlayerItem[m_ViewIndex]->Rotation().y + 1.5707964f;
            }
        }
        if (pPlayer->m_Mode == 0x11 && bChanged) m_TargetElevation = 1.4137167f;
        if (pPlayer->m_Mode == 0x13 && pPlayer->GetSubMode() >= 4) {
            float d = AngleDiff(m_TargetAngle, gpPlayerItem[m_ViewIndex]->Rotation().y + 3.1415927f);
            if (d > 1.5707964f) m_TargetAngle = gpPlayerItem[m_ViewIndex]->Rotation().y + 3.1415927f - 1.5707964f;
            if (-1.5707964f > d) m_TargetAngle = gpPlayerItem[m_ViewIndex]->Rotation().y + 4.712389f;
        }
        if (bChanged) {
            if (m_KillAngle != -666.0f) {
                m_TargetAngle = m_KillAngle;
                m_TargetElevation = m_KillElevation;
                m_KillAngle = -666.0f;
                m_KillElevation = -666.0f;
            }
            if (pPlayer->m_Mode == 7) {
                switch (pPlayer->GetSubMode()) {
                case 0:
                case 4:
                case 5:
                case 6:
                case 7:
                case 8:
                case 9:
                case 10:
                case 11:
                case 16:
                case 17:
                    SetKillCam(gpPlayerItem[m_ViewIndex]->Rotation().y);
                    break;
                }
            }
            if (pPlayer->m_Mode == 0x17) {
                switch (pPlayer->GetSubMode()) {
                case 8:
                case 9:
                    SetKillCam(gpPlayerItem[m_ViewIndex]->Rotation().y);
                    break;
                case 11:
                    SetKillCam(gpPlayerItem[m_ViewIndex]->Rotation().y - 0.6f);
                    break;
                case 12:
                    SetKillCam(gpPlayerItem[m_ViewIndex]->Rotation().y + 0.6f);
                    break;
                }
            }
            if (pPlayer->m_Mode == 0x14 && pPlayer->GetSubMode() == 2) SetKillCam(gpPlayerItem[m_ViewIndex]->Rotation().y);
        }
        if (pPlayer->m_Mode == 2) {
            if (bChanged) ZargMode = 1;
            if (sManualCam) {
                ZargMode = 0;
            } else if (ZargMode) {
                float d = Vec_AngleDifference(m_TargetAngle, gpPlayerItem[m_ViewIndex]->Rotation().y);
                m_TargetAngle += d >= 0.04f ? 0.04f : (-0.04f >= d ? -0.04f : d);
            }
        }
        if (bChanged && !sManualCam && pPlayer->m_Mode == 3) {
            if (pPlayer->GetSubMode() >= 7 && pPlayer->GetSubMode() <= 10) {
                m_2c = 10.0f;
            } else {
                m_2c = 50.0f;
            }
            m_LookSmooth = 10.0f;
        }
    }
    if (m_TargetDistance != 0.0f) {
        EXVector v = pPlayer->m_Speed;
        float h = v.x * v.x + v.z * v.z;
        m_TargetDistance = (h < 0.0f ? 0.0f : (float)sqrt(h)) + CameraDistance;
        if (1.5707964f > m_Elevation || m_Elevation > 1.9634955f) {
            EXVector v2 = pPlayer->m_Speed;
            float h2 = v2.x * v2.x + v2.z * v2.z;
            if (h2 >= 0.0f && (float)sqrt(h2) > 0.5f && !sManualCam)
                m_TargetElevation = 1.9634955f - AngleDiff(m_TargetElevation, 1.9634955f) * 0.95f;
        }
    }
    float two = 2.0f;
    float v = m_2c * 0.975f;
    m_2c = *(two >= v ? &two : &v);
    v = m_LookSmooth * 0.98f;
    two = 2.0f;
    m_LookSmooth = *(two >= v ? &two : &v);
    float k = 8.0f;
    if (m_Mode == 2) {
        k = 4.0f;
        m_2c = 4.0f;
    }
    float inv = 1.0f / XApp::GameFrameRateMul();
    float one = 1.0f;
    v = k * inv;
    float sa = *(one >= v ? &one : &v);
    v = inv * m_2c;
    one = 1.0f;
    float sp = *(one >= v ? &one : &v);
    float ia = 1.0f / sa;
    m_Angle -= AngleDiff(m_TargetAngle, m_Angle) * ia;
    m_Elevation -= AngleDiff(m_TargetElevation, m_Elevation) * ia;
    float ip = 1.0f / sp;
    m_CameraPos.x = (m_CameraPosTarget.x - m_CameraPos.x) * ip + m_CameraPos.x;
    m_CameraPos.y = (m_CameraPosTarget.y - m_CameraPos.y) * ip + m_CameraPos.y;
    m_CameraPos.z = (m_CameraPosTarget.z - m_CameraPos.z) * ip + m_CameraPos.z;
    float lo, hi;
    unsigned int mode = XItemHandler_Player::GetInstance()->m_Mode;
    if (mode == 0x18) {
        hi = 2.7415926f;
        lo = 0.4f;
    } else if ((m_Mode == 2 || (m_Mode == 1 && m_bFirstPerson))) {
        lo = 0.1f;
        hi = 3.0415928f;
    } else if (mode == 0 && XItemHandler_Player::GetInstance()->GetSubMode() == 4) {
        lo = 1.7278761f;
        hi = 3.0415928f;
    } else {
        hi = 3.0415928f;
        lo = 0.7f;
    }
    m_TargetElevation = m_TargetElevation >= hi ? hi : (lo >= m_TargetElevation ? lo : m_TargetElevation);
    m_Elevation = m_Elevation >= hi ? hi : (lo >= m_Elevation ? lo : m_Elevation);
    if (m_Mode == 0)
        m_28 = m_Elevation >= 1.9634955f ? 1.9634955f : (1.5707964f >= m_Elevation ? 1.5707964f : m_Elevation);
    if ((m_Mode == 2 || (m_Mode == 1 && m_bFirstPerson))) {
        m_pCamera->SetWorldUp(EXVector(0.0f, 1.0f, 0.0f, 0.0f));
    } else if (XItemHandler_Player::GetInstance()->m_Mode != 0x18) {
        EXCamera *pCam = m_pCamera;
        const EXVector &up = pCam->WorldUp();
        pCam->SetWorldUp((EXVector(0.0f, 1.0f, 0.0f, 0.0f) - up) * 0.125f + up);
    }
    switch (m_Mode) {
    case 1:
        if (m_bFirstPerson) {
            SetCamFirstPerson();
        } else {
            m_Zoom = 1.0f;
            SetCamThirdPerson();
        }
        break;
    case 2:
        SetCamFirstPerson();
        break;
    case 7:
    case 8:
    case 9:
        m_Zoom = 1.0f;
        SetCamFixed();
        goto done;
    case 11:
        m_Zoom = 1.0f;
        SetCamSpline();
        break;
    case 90:
        m_Zoom = 1.0f;
        SetCamDebugFree();
        goto done;
    case 91:
        m_Zoom = 1.0f;
        SetCamDebugFixed();
        goto done;
    default:
        m_Zoom = 1.0f;
        if (XItemHandler_Player::GetInstance()->m_Mode == 0x18) SetCamGecko();
        else SetCamThirdPerson();
        break;
    }
    if (!(m_Mode == 2 || (m_Mode == 1 && m_bFirstPerson)) && !XItemHandler_Player::GetInstance()->GetScanMode()) {
        EXRayCastTemp r;
        r.Set(m_CameraLookAt, m_CameraPos - m_CameraLookAt, 0, 0, 0, 0);
        r.m_mask = 0x20;
        theItemEnv.DoRayCastToMap(r, 0);
        if (r.m_bHit) {
            EXVector d = r.Intersect() - m_CameraLookAt;
            EXVector t = d;
            float l = t.y * t.y + t.z * t.z + t.x * t.x;
            float len = (l < 0.0f ? 0.0f : (float)sqrt(l)) - 0.05f;
            float l4 = t.w * t.w + t.y * t.y + t.z * t.z + t.x * t.x;
            if (l4 >= 0.0f) {
                float s = (float)sqrt(l4);
                if (s > 1e-5f) t = t * (1.0f / s);
            }
            m_CameraPos = t * len + m_CameraLookAt;
        }
        for (XItem *p = gEnvList.GetFirst(); p; p = gEnvList.GetNext(p)) {
            XItemHandler_Environment *h = (XItemHandler_Environment *)p->ItemHandler();
            if (h->m_Type == 0x12 && Within(m_CameraPos, *(EXGeoTriggerDatum *)&h->m_Datum))
                m_CameraPos.y = h->m_Datum.m_Pos.y + h->m_Datum.m_08.y;
        }
    }
done:
    m_pCamera->SetPosition(m_CameraPos);
    GetItem()->SetPosition(m_CameraPos);
    m_PrevPlayerMode = pPlayer->m_Mode;
    return 1;
}


// NEAR (see check): nothing solid (any hit, mask 0x4008) between two points on the map, or also among
// the contoured items when b.
signed char LineOfSight(EXVector &from, EXVector &to, signed char b)
{
    EXRayCastTemp r;
    r.Set(from, to - from, 0, 0, 0, 0);
    r.m_mask = 0x4008;
    r.FindAnyHit();
    theItemEnv.DoRayCastToMap(r, 0);
    if (!r.m_bHit) {
        if (b) theItemEnv.DoRayCastToContoured(r);
    }
    return !r.m_bHit;
}

// NEAR (see check): line of sight through items too; in thermal vision, script items flagged see-through
// don't block, and visible VMOs always count as seen.
signed char LineOfSightThermal(EXVector &from, EXVector &to)
{
    EXRayCastTemp r;
    r.Set(from, to - from, 0, 0, 0, 0);
    r.m_mask = 0x4008;
    r.FindAnyHit();
    theItemEnv.DoRayCastToMap(r, 0);
    if (!r.m_bHit) {
        theItemEnv.DoRayCastToContoured(r);
        if (r.m_bHit && XItemHandler_Player::GetInstance()->GetVisionMode() == 1 && r.m_pAnimator) {
            EXItem *pItem = r.m_pAnimator->GetOwnerItem();
            EXItemHandler *h;
            if (pItem && (h = pItem->m_pItemHandler) != 0) {
                const EXRuntimeClass *rc = h->GetRuntimeClass();
                do {
                    if (rc == EXRUNTIME_CLASS(XItemHandler_ScriptRunner)) {
                        if (((XItemHandler_ScriptRunner *)h)->m_TriggerFlags & 1) r.m_bHit = 0;
                        break;
                    }
                    rc = rc->m_pBaseClass;
                } while (rc);
                rc = h->GetRuntimeClass();
                do {
                    if (rc == EXRUNTIME_CLASS(XItemHandler_VMO)) {
                        if (((XItemHandler_VMO *)h)->m_VMOFlags & 1) return 1;
                        break;
                    }
                    rc = rc->m_pBaseClass;
                } while (rc);
            }
        }
    }
    return !r.m_bHit;
}

// NEAR (see check): whether a sphere is inside the view (cull test against world-to-view x view-to-cull),
// after making sure the system camera is set up for the game view.
int XItemHandler_Camera::BoundingVolumeOnScreen(EXSphere &s)
{
    if (gSystemCameraUser != 1) {
        m_pCamera->SetClipSize(640, 640, 0);
        m_pCamera->SetOrigin(m_pCamera->Origin());
        XItemHandler_Camera *pHandler = gpCameraItem[0] ? (XItemHandler_Camera *)gpCameraItem[0]->m_pItemHandler : 0;
        EXRectf r;
        r.x = (float)gpGameWnd[0]->PrimRect().x;
        r.y = (float)gpGameWnd[0]->PrimRect().y;
        r.w = (float)gpGameWnd[0]->PrimRect().w;
        r.h = (float)gpGameWnd[0]->PrimRect().h;
        ((EXCommonCamera *)m_pCamera)->SetVFOV(r, 0.7853982f / pHandler->Zoom());
        m_pCamera->UpdateDisplay(0);
        gSystemCameraUser = 1;
    }
    EXMatrix m;
    D3DXMatrixMultiply(&m, &EXBaseDisplay::WorldToView(), &EXBaseDisplay::ViewToCull());
    EXBoundsSphere bs;
    EXMaths::mat_44 mm = m.operator const EXMaths::mat_44 &();  // retail references the conversion ([fi] copy)
    bs.SetCentre(s.m_centre.x, s.m_centre.y, s.m_centre.z);
    bs.SetRadius(s.m_radius);
    return EXDisplay::CullSphere(bs, mm);
}

// The camera's pitch: from its look direction while wall crawling, else the orbit elevation.
float XItemHandler_Camera::Elevation()
{
    if (XItemHandler_Player::GetInstance() && XItemHandler_Player::GetInstance()->GetMode() == 0x18) {
        EXVector d = m_CameraLookAt - m_CameraPos;
        float h = d.z * d.z + d.x * d.x;
        return 1.5707964f - (float)atan2(d.y, h < 0.0f ? 0.0f : (float)sqrt(h));
    }
    return m_Elevation;
}

// NEAR (see check): the kill cam (when the option is on): saves the orbit, then looks for a clear view
// 4 units out at the same height, behind the kill direction or 135 degrees either side of it (seen
// from the player at look-at height); takes the first clear one looking straight across, or marks
// none found.
void XItemHandler_Camera::SetKillCam(float a)
{
    if (!GameData::Instance(1)->m_Values.m_bActionCamera) return;
    float t = a + 3.1415927f;
    m_KillAngle = m_Angle;
    m_KillElevation = m_Elevation;
    EXItem *pMe = gpPlayerItem[m_ViewIndex];
    float se = (float)sin(1.5707963705062866);
    EXVector from(pMe->Position().x, m_CameraLookAt.y, pMe->Position().z, pMe->Position().w);
    float dy = (float)cos(1.5707963705062866) * -4.0f;
    EXVector to;
    to.x = -((float)sin(t) * 4.0f * se) + m_CameraLookAt.x;
    to.y = m_CameraLookAt.y + dy;
    to.z = -((float)cos(t) * 4.0f * se) + m_CameraLookAt.z;
    to.w = m_CameraLookAt.w + to.w;
    if (!LineOfSight(from, to, 1)) {
        t = a + 2.3561945f;
        to.y = m_CameraLookAt.y + dy;
        to.w = m_CameraLookAt.w + to.w;
        to.x = -((float)sin(t) * 4.0f * se) + m_CameraLookAt.x;
        to.z = -((float)cos(t) * 4.0f * se) + m_CameraLookAt.z;
        if (!LineOfSight(from, to, 1)) {
            t = a - 2.3561945f;
            to.y = m_CameraLookAt.y + dy;
            to.w = m_CameraLookAt.w + to.w;
            to.x = -((float)sin(t) * 4.0f * se) + m_CameraLookAt.x;
            to.z = -((float)cos(t) * 4.0f * se) + m_CameraLookAt.z;
            if (!LineOfSight(from, to, 1)) {
                m_KillAngle = -666.0f;
                m_KillElevation = -666.0f;
                return;
            }
        }
    }
    m_TargetAngle = t;
    m_TargetElevation = 1.5707964f;
}

// NEAR (see check): turns direction a towards b by at most maxStep in heading and in elevation; the
// result is a unit direction.
EXVector SphericalInterpolate(EXVector &a, EXVector &b, float maxStep)
{
    float ay = (float)atan2(a.x, a.z);
    float ah = a.z * a.z + a.x * a.x;
    float ap = (float)atan2(a.y, ah < 0.0f ? 0.0f : (float)sqrt(ah));
    float bh = b.z * b.z + b.x * b.x;
    float by = (float)atan2(b.x, b.z);
    float be = 1.5707964f - (float)atan2(b.y, bh < 0.0f ? 0.0f : (float)sqrt(bh));
    float ae = 1.5707964f - ap;
    float dy = AngleDiff(ay, by);
    dy = dy < maxStep ? (-maxStep < dy ? dy : -maxStep) : maxStep;
    float de = AngleDiff(ae, be);
    de = de < maxStep ? (-maxStep < de ? de : -maxStep) : maxStep;
    float y = dy + ay;
    float e = de + ae;
    EXVector r;
    r.w = 0.0f;
    float se = (float)sin(e);
    r.x = (float)sin(y) * se;
    r.y = (float)cos(e);
    r.z = (float)cos(y) * se;
    return r;
}

// NEAR (see check): puts a lock-on marker on an actor: just above its root, standing along the floor
// normal (straight up on steep or special surfaces), or copying an alien's orientation; half as big
// again on Ulysses.
void SetIndicatorToActor(XItem *pInd, Actor *pActor)
{
    EXVector n(0.0f, 1.0f, 0.0f, 0.0f);
    EXItem *pItem = pActor->GetItem();
    EXVector p(pItem->Position().x, pItem->Position().y + 0.05f, pItem->Position().z, pItem->Position().w);
    pInd->SetScale(EXVector(1.0f, 1.0f, 1.0f, 0.0f));
    if (pActor->m_ActorFlags.m_bAlien) {
        pInd->m_rotOrder = 0;
        pInd->m_rotation = pActor->GetItem()->Rotation();
    } else {
        if (pActor->m_ActorFlags.m_bUlysses) pInd->SetScale(EXVector(1.5f, 1.5f, 1.5f, 0.0f));
        const EXVector *pN;
        if (pActor->m_OrgLoc.m_wSurfaceFlags & 0x50) {
            n.x = 0.0f;
            n.z = 0.0f;
            n.w = 0.0f;
            pN = &n;
        } else {
            pN = &pActor->m_OrgLoc.m_vFloorNormal;
        }
        float x = pN->x, z = pN->z;
        float y = (float)atan2(x, z);
        float h = z * z + x * x;
        float ny = pN->y;
        pInd->m_rotation.y = y;
        pInd->m_rotation.x = 1.5707964f - (float)atan2(ny, h < 0.0f ? 0.0f : (float)sqrt(h));
    }
    pInd->m_validMtx = 0;
    pInd->SetPosition(p);
    pInd->SetHasRender();
}

// NEAR (see check): shows or hides the lock-on markers: the target (aiming, third person; coloured by
// honour), the shoulder cannon target (not aiming), the traffic vehicle (raised 0.14 below its root,
// twice the size), and the recent hit list (as many as the disk upgrade allows, not the target).
void XItemHandler_Camera::UpdateIndicators()
{
    if (m_TargetID < 1 || m_Mode == 2 || m_Mode != 1 || m_bFirstPerson) {
        m_pLockIndicator->ClrHasRender();
    } else {
        Actor *pActor = Actor::GetPtrFromID(m_TargetID);
        if (!pActor || (pActor->m_Health <= 0.0f && !pActor->m_StateFlags.m_bCanRipBossHead) || !pActor->IsTargetable()) {
            m_TargetID = 0;
        } else {
            SetIndicatorToActor(m_pLockIndicator, pActor);
            int honour = pActor->GetProfile()->m_Honour;
            XItem *pInd = m_pLockIndicator;
            pInd->RenderInfo().m_rgbaMult.w = 1.0f;
            pInd->RenderInfo().m_rgbaMult.z = 0.0f;
            if (honour < 0) {
                pInd->RenderInfo().m_rgbaMult.x = 0.0f;
                pInd->RenderInfo().m_rgbaMult.y = 1.0f;
            } else {
                pInd->RenderInfo().m_rgbaMult.x = 1.0f;
                pInd->RenderInfo().m_rgbaMult.y = 0.0f;
            }
        }
    }
    if (m_PlasmaID < 1 || m_Mode == 2 || m_Mode == 1) {
        m_pPlasmaIndicator->ClrHasRender();
    } else {
        Actor *pActor = Actor::GetPtrFromID(m_PlasmaID);
        if (!pActor) m_PlasmaID = 0;
        else SetIndicatorToActor(m_pPlasmaIndicator, pActor);
    }
    if (!m_pVehicleTarget || m_Mode == 2 || (m_Mode == 1 && m_bFirstPerson)) {
        m_pVehicleIndicator->ClrHasRender();
    } else {
        m_pVehicleIndicator->SetHasRender();
        m_pVehicleIndicator->SetScale(EXVector(2.0f, 2.0f, 2.0f, 0.0f));
        XItem *pV = m_pVehicleTarget;
        float z = pV->Position().z, w = pV->Position().w;
        XItem *pInd = m_pVehicleIndicator;
        pInd->m_position.x = pV->Position().x;
        pInd->m_position.y = -0.14f;
        pInd->m_position.z = z;
        pInd->m_position.w = w;
        pInd->m_validMtx = 0;
    }
    for (int i = 0; i < 3; i++) {
        if (XItemHandler_Player::GetInstance()->m_Unkcf0[6] < i) m_HitList[i] = 0;
        if (m_HitList[i] != m_TargetID) {
            Actor *pActor = Actor::GetPtrFromID(m_HitList[i]);
            if (pActor && (pActor->m_Health > 0.0f || pActor->m_StateFlags.m_bCanRipBossHead)) {
                SetIndicatorToActor(m_pHitIndicator[i], pActor);
                continue;
            }
            m_HitList[i] = 0;
        }
        m_pHitIndicator[i]->ClrHasRender();
    }
}

// NEAR (see check): debug free camera: looks along the target orbit angles from where it is, and the
// left stick (y) flies it along the view.
void XItemHandler_Camera::SetCamDebugFree()
{
    m_Elevation = m_TargetElevation;
    m_Angle = m_TargetAngle;
    float stick = EXBaseGamePad::GetGamePad()->Analog(m_PadIndex, (EXPad::AnalogChan)1);
    m_CameraLookAt = m_CameraPos;
    float se = (float)sin(m_Elevation);
    EXCamera *pCam = m_pCamera;
    m_CameraLookAt.x = (float)sin(m_Angle) * se + m_CameraLookAt.x;
    m_CameraLookAt.z = (float)cos(m_Angle) * se + m_CameraLookAt.z;
    m_CameraLookAt.y = (float)cos(m_Elevation) + m_CameraLookAt.y;
    pCam->SetTarget(m_CameraLookAt);
    m_CameraPosTarget -= (m_CameraLookAt - m_CameraPos) * stick;
}

// NEAR (see check): debug fixed camera: stays put and turns to the player's head (base height + 1.8).
void XItemHandler_Camera::SetCamDebugFixed()
{
    EXItem *pMe = gpPlayerItem[m_ViewIndex];
    float px = pMe->Position().x;
    float pz = pMe->Position().z;
    float py = XItemHandler_Player::GetInstance()->GetBaseHeight() + 1.8f;
    EXCamera *pCam = m_pCamera;
    pCam->SetLookAt(m_CameraLookAt);
    float dx = px - m_CameraPos.x;
    float dy = py - m_CameraPos.y;
    float dz = pz - m_CameraPos.z;
    float a = (float)atan2(dx, dz);
    m_CameraPosTarget = m_CameraPos;
    float h = dz * dz + dx * dx;
    m_Angle = a;
    m_TargetAngle = a;
    float e = 1.5707964f - (float)atan2(dy, h < 0.0f ? 0.0f : (float)sqrt(h));
    m_Elevation = e;
    m_TargetElevation = e;
}

// NEAR (see check): how far the camera can sit along a whisker offset (da, de) from its current orbit
// (optionally turned by a matrix): a ray 10 long from the orbit point two units out (lowered by the
// look-at lift) is cast against items (mask 0x20) and the map; the nearer hit's distance, less 0.05
// (30 when nothing is hit).
float XItemHandler_Camera::GetWhiskerDistance(float da, float de, EXMatrix *pMtx)
{
    float se = (float)sin(m_Elevation);
    EXRayCastTemp r1, r2;
    EXVector base;
    base.w = 0.0f;
    base.x = -((float)sin(m_Angle) * se);
    base.z = -((float)cos(m_Angle) * se);
    base.y = -(float)cos(m_Elevation);
    if (pMtx) {
        EXVector o;
        D3DXVec4Transform(&o, &base, pMtx);
        base = o;
    }
    float a = da + m_Angle;
    float e = de + m_Elevation;
    EXVector whisk;
    whisk.w = 0.0f;
    float se2 = (float)sin(e);
    whisk.x = -((float)sin(a) * se2);
    whisk.z = -((float)cos(a) * se2);
    whisk.y = -(float)cos(e);
    if (pMtx) {
        EXVector o;
        D3DXVec4Transform(&o, &whisk, pMtx);
        whisk = o;
    }
    EXVector p = (m_CameraLookAt - base * 2.0f) + whisk * 2.0f;
    EXVector start(p.x, p.y - Fisher, p.z, p.w);
    EXVector dir(whisk.x * 10.0f, whisk.y * 10.0f, whisk.z * 10.0f, 0.0f);
    r1.Set(start, dir, 0, 0, 0, 0);
    r1.m_mask = 0x20;
    r2 = r1;
    theItemEnv.DoRayCastToContoured(r1);
    theItemEnv.DoRayCastToMap(r2, 0);
    if (!r1.m_bHit) r1.m_t = 30.0f;
    float t2 = r2.m_bHit ? r2.m_t : 30.0f;
    EXVector v;
    if (t2 <= r1.m_t)
        v = r2.Intersect() - r2.m_start;
    else
        v = r1.Intersect() - r1.m_start;
    float l = v.x * v.x + v.y * v.y + v.z * v.z;
    return (l < 0.0f ? 0.0f : (float)sqrt(l)) - 0.05f;
}

// NEAR (see check): sets up the game camera: behind the player at the default distance and elevation,
// third person with inverted look, its watcher entries (distance, first person face, sensitivity,
// power, peer limits), and the lock-on markers (target red, cannon and hit list cyan, vehicle).
s8 XItemHandler_Camera::Initialise()
{
    s8 r = EXItemHandler::Initialise();
    if (!r) return r;
    m_Angle = 4.712389f;
    m_Elevation = 1.9634954f;
    m_Distance = CameraDistance;
    m_LeanRoll = 0.0f;
    m_bInvertX = 1;
    m_bInvertY = 1;
    m_bInvertFPX = 0;
    m_bInvertFPY = 1;
    m_bFPFace = 0;
    *(u32 *)&m_Lean = 0;
    m_LostSight = 0;
    m_Mode = 5;
    m_CameraPos.SetZero();
    m_CameraLookAt.SetZero();
    EXWatcher *w = EXWatcher::GetWatcher();
    w->AddSection("Camera", (unsigned int)0, "-Game-", 2);
    unsigned int h = w->AddItem("Distance", "Camera", EXWATCH::DT_7, &CameraDistance, 2);
    w->EnableItemEdit(h, -32.0f, 32.0f, 0.1f, 1);
    h = w->AddItem("FP Face", "Camera", EXWATCH::DT_8, &m_bFPFace, 2);
    w->EnableItemEdit(h, 1);
    h = w->AddItem("FP Sensitivity", "Camera", EXWATCH::DT_7, &FP_Sensitivity, 2);
    w->EnableItemEdit(h, 0.0f, 2.0f, 0.05f, 1);
    h = w->AddItem("FP Power", "Camera", EXWATCH::DT_5, &FP_Power, 2);
    w->EnableItemEdit(h, 1, 10, 1, 1);
    h = w->AddItem("Peer Horizontal", "Camera", EXWATCH::DT_7, &PeerHorizontal, 2);
    w->EnableItemEdit(h, 0.0f, 2.0f, 0.05f, 1);
    h = w->AddItem("Peer Vertical", "Camera", EXWATCH::DT_7, &PeerVertical, 2);
    w->EnableItemEdit(h, 0.0f, 2.0f, 0.05f, 1);
    m_TargetAngle = m_Angle;
    m_TargetElevation = m_Elevation;
    m_TargetDistance = m_Distance;
    m_CameraPosTarget = m_CameraPos;
    m_28 = 1.9634954f;
    m_2c = 1.0f;
    m_LookSmooth = 1.0f;
    m_KillAngle = -666.0f;
    m_KillElevation = -666.0f;
    m_ListenerPos.SetZero();
    *(int *)&m_PadIndex = GameControllerRemovedSys::Controller();
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(0x1000016, 0, 0);
    m_pLockIndicator = XItem::CreateItem(pFile, 0x1000016, 0, EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    EXItemAnimator_Script *pScript = new EXItemAnimator_Script;
    pScript->Initialise(pFile, 0x400005d);
    m_pLockIndicator->AddAnimator(pScript);
    m_pLockIndicator->ClrHasRender();
    m_pLockIndicator->RenderInfo().m_renderFlags.BitSet(8);
    m_pLockIndicator->RenderInfo().m_renderFlags.BitSet(4);
    m_pLockIndicator->RenderInfo().SetRGBAMult(1.0f, 0.0f, 0.0f, 1.0f);
    theItemEnv.AddItem(m_pLockIndicator, 0);
    m_pPlasmaIndicator = XItem::CreateItem(pFile, 0x1000016, 0, EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    pScript = new EXItemAnimator_Script;
    pScript->Initialise(pFile, 0x400005d);
    m_pPlasmaIndicator->AddAnimator(pScript);
    m_pPlasmaIndicator->ClrHasRender();
    m_pPlasmaIndicator->RenderInfo().m_renderFlags.BitSet(8);
    m_pPlasmaIndicator->RenderInfo().m_renderFlags.BitSet(4);
    m_pPlasmaIndicator->RenderInfo().SetRGBAMult(0.0f, 1.0f, 1.0f, 1.0f);
    theItemEnv.AddItem(m_pPlasmaIndicator, 0);
    m_pVehicleIndicator = XItem::CreateItem(pFile, 0x1000016, 0, EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    pScript = new EXItemAnimator_Script;
    pScript->Initialise(pFile, 0x4000358);
    m_pVehicleIndicator->AddAnimator(pScript);
    m_pVehicleIndicator->ClrHasRender();
    m_pVehicleIndicator->RenderInfo().m_renderFlags.BitSet(8);
    theItemEnv.AddItem(m_pVehicleIndicator, 0);
    for (int i = 0; i < 3; i++) {
        m_pHitIndicator[i] = XItem::CreateItem(pFile, 0x1000016, 0, EXVector(0.0f, 0.0f, 0.0f, 0.0f));
        pScript = new EXItemAnimator_Script;
        pScript->Initialise(pFile, 0x400005d);
        m_pHitIndicator[i]->AddAnimator(pScript);
        m_pHitIndicator[i]->ClrHasRender();
        m_pHitIndicator[i]->RenderInfo().m_renderFlags.BitSet(8);
        m_pHitIndicator[i]->RenderInfo().m_renderFlags.BitSet(4);
        m_pHitIndicator[i]->RenderInfo().SetRGBAMult(0.0f, 1.0f, 1.0f, 1.0f);
        theItemEnv.AddItem(m_pHitIndicator[i], 0);
    }
    m_TargetID = 0;
    m_PlasmaID = 0;
    m_HitList[0] = 0;
    m_HitList[1] = 0;
    m_HitList[2] = 0;
    m_Zoom = 1.0f;
    m_pVehicleTarget = 0;
    m_pJumpTarget = 0;
    m_pLockTarget[0] = 0;
    m_pLockTarget[1] = 0;
    m_pLockTarget[2] = 0;
    m_pLockTarget[3] = 0;
    *(u32 *)m_Unk38 = 0;
    m_LockDelay = 0;
    m_BoundingRadius = 0;
    m_Unkfd[0] = 0;
    m_Unkfd[2] = 0;
    m_Unkfd[1] = 0;
    m_bLockHeld = 0;
    m_bFirstPerson = 0;
    m_Warp = 0.0f;
    m_SideOffset = 0.0f;
    m_bGoFirstPerson = 0;
    m_CutsceneFOV = 0.7853982f;
    m_IdleTime = 0.0f;
    m_NextAngle = 0.0f;
    m_NextElevation = 0.0f;
    m_bNext = 0;
    return 1;
}

// NEAR (see check): first person while wall crawling: the eye at the top of the head, looking along the
// orbit angles in the frame of the wall (the crawl orientation turned back 90 degrees about x), with the
// wall's up as the camera's up.
void XItemHandler_Camera::SetCamGeckoFP()
{
    EXVector head = XItemHandler_Player::GetInstance()->GetTopHeadPos();
    EXQuaternion q = XItemHandler_Player::GetInstance()->GetGeckoAngle();
    EXMatrix mq;
    mq.set_rotate_quat(q);
    EXMatrix mr;
    mr.set_rotate_xyz(EXVector(-1.5707964f, 0.0f, 0.0f, 0.0f));
    EXMatrix m;
    D3DXMatrixMultiply(&m, &mr, &mq);
    m_CameraLookAt = head;
    m_CameraPosTarget = m_CameraLookAt;
    m_CameraPos = m_CameraLookAt;
    mq = m;
    m_CameraPos.w = 0.0f;
    m_CameraLookAt.w = 0.0f;
    if (m_Warp > 0.1f)
        m_Warp -= XApp::GameFrameRateMul();
    else
        m_Warp = 0.0f;
    float se = (float)sin(m_Elevation);
    EXVector d, o;
    d.x = (float)sin(m_Angle) * se;
    d.z = (float)cos(m_Angle) * se;
    d.y = (float)cos(m_Elevation);
    d.w = 0.0f;
    D3DXVec4Transform(&o, &d, &mq);
    m_CameraLookAt += o;
    m_pCamera->SetTarget(m_CameraLookAt);
    EXVector upv(0.0f, 1.0f, 0.0f, 0.0f), up;
    D3DXVec4Transform(&up, &upv, &mq);
    m_pCamera->SetWorldUp(up);
}

// NEAR (see check): fixed cameras: look at the player's head (the wall point when crawling); mode 9 lets
// the look-at lag within the bounding radius of it. A map camera covering that point supplies the eye
// (and in mode 7 its own look-at) and the field of view from its lens; the orbit angles are then taken
// from eye to target.
void XItemHandler_Camera::SetCamFixed()
{
    EXVector t;
    if (XItemHandler_Player::GetInstance()->GetMode() == 0x18) {
        t = XItemHandler_Player::GetInstance()->GetGeckoPoint();
    } else {
        EXItem *pMe = gpPlayerItem[m_ViewIndex];
        t.z = pMe->Position().z;
        t.x = pMe->Position().x;
        t.w = 0.0f;
        t.y = XItemHandler_Player::GetInstance()->GetBaseHeight() + 1.8f;
    }
    if (m_Mode == 9) {
        EXVector d = t - m_CameraLookAt;
        float l = d.y * d.y + d.z * d.z + d.x * d.x;
        float dist = 0.0f;
        if (!(l < 0.0f)) dist = (float)sqrt(l);
        if (dist <= (float)m_BoundingRadius) {
            t = m_CameraLookAt;
        } else {
            EXVector n = d;
            float l4 = d.w * d.w + d.y * d.y + d.z * d.z + d.x * d.x;
            if (!(l4 < 0.0f)) {
                float len = (float)sqrt(l4);
                if (len > 1e-5f) {
                    float k = 1.0f / len;
                    n.x = d.x * k;
                    n.y = d.y * k;
                    n.z = d.z * k;
                    n.w = d.w * k;
                }
            }
            float r = (float)m_BoundingRadius;
            t = (d + m_CameraLookAt) - n * r;
        }
    }
    m_CameraLookAt = t;
    m_pCamera->SetTarget(m_CameraLookAt);
    EXGeoCamera *pMapCam = theItemEnv.GetCamera(t);
    if (pMapCam) {
        m_CameraPos.y = pMapCam->m_Position.y;
        m_CameraPos.x = pMapCam->m_Position.x;
        m_CameraPos.z = pMapCam->m_Position.z;
        m_CameraPos.w = 0.0f;
        if (m_Mode == 7) {
            t.x = pMapCam->m_Target.x;
            t.y = pMapCam->m_Target.y;
            t.z = pMapCam->m_Target.z;
            m_CameraLookAt.x = t.x;
            m_CameraLookAt.y = t.y;
            m_CameraLookAt.z = t.z;
            m_CameraLookAt.w = 0.0f;
            m_pCamera->SetTarget(m_CameraLookAt);
        }
        m_CutsceneFOV = qatanf(pMapCam->m_ApertureHeight / (pMapCam->m_FocalLength + pMapCam->m_FocalLength)) * 2.0f;
    }
    float dx = t.x - m_CameraPos.x;
    float cy = m_CameraPos.y;
    float dz = t.z - m_CameraPos.z;
    float a = (float)atan2(dx, dz);
    m_CameraPosTarget = m_CameraPos;
    float h = dz * dz + dx * dx;
    m_Angle = a;
    m_TargetAngle = a;
    float hl = h < 0.0f ? 0.0f : (float)sqrt(h);
    m_Mode = 0;
    float e = 1.5707964f - (float)atan2(t.y - cy, hl);
    m_Elevation = e;
    m_TargetElevation = e;
    m_pCamera->SetWorldUp(EXVector(0.0f, 1.0f, 0.0f, 0.0f));
}

// NEAR (see check): first person: the eye at head height (crouch heights in mode 0xc, nudged back), with
// a slight idle sway; looking down past level lets the eye peer forward and down; the configured stick
// leans the eye sideways up to 0.5 short of any wall, rolling the view with it. Wall crawling has its
// own camera. The player's facing follows the view in the free movement modes.
void XItemHandler_Camera::SetCamFirstPerson()
{
    Wobbler += EXBaseApp::FrameLength();
    m_Elevation = (float)sin(Wobbler) * 0.001f + m_Elevation;
    m_Angle = (float)cos(Wobbler) * 0.002f + m_Angle;
    EXItem *pMe = gpPlayerItem[m_ViewIndex];
    EXVector eye = pMe->Position();
    unsigned int mode = XItemHandler_Player::GetInstance()->GetMode();
    if (mode == 0xc) {
        float h = 1.89f;
        if (XItemHandler_Player::GetInstance()->CanPullUp()) h = 2.2f;
        eye.y += h;
        eye.x -= (float)sin(pMe->Rotation().y) * 0.5f;
        eye.z -= (float)cos(pMe->Rotation().y) * 0.5f;
    } else if (mode == 0x11) {
        eye.y += 1.89f;
    } else if (mode == 0x18) {
        SetCamGeckoFP();
        return;
    } else {
        eye.y += 1.89f;
        if (m_Elevation > 1.5707964f) {
            float e2 = m_Elevation - 1.5707964f;
            float s2 = (float)sin(e2);
            eye.x = (float)sin(pMe->Rotation().y) * s2 * PeerHorizontal + eye.x;
            eye.z = (float)cos(pMe->Rotation().y) * s2 * PeerHorizontal + eye.z;
            eye.y = (eye.y - PeerVertical) + (float)cos(e2) * PeerVertical;
        }
        EXBaseGamePad *pPad = EXBaseGamePad::GetGamePad();
        EXCamera *pCam;
        if (!GameData::Instance(1)->m_Values.m_ButtonConfig) {
            pCam = m_pCamera;
            m_Lean = 0.0f;
            m_LeanRoll = 0.0f;
            pCam->SetWorldUp(EXVector(0.0f, 1.0f, 0.0f, 0.0f));
        } else {
            float stick;
            if (GameData::Instance(1)->m_Values.m_ButtonConfig == 1)
                stick = pPad->Analog(m_PadIndex, (EXPad::AnalogChan)0);
            else
                stick = pPad->Analog(m_PadIndex, (EXPad::AnalogChan)2);
            if ((float)fabs(stick) >= 1.1920929e-7f || (float)fabs(m_Lean) >= 1.1920929e-7f) {
                float side = m_Angle + 1.5707964f;
                int sz = stick < 0.0f ? -1 : (stick != 0.0f ? 1 : 0);
                int sx = stick < 0.0f ? -1 : (stick != 0.0f ? 1 : 0);
                float ss = (float)sin(side);
                EXVector dir;
                dir.y = 0.0f;
                dir.w = 0.0f;
                EXRayCastTemp r;
                dir.x = (float)(sx * 2) * ss;
                float cs = (float)cos(side);
                dir.z = (float)(sz * 2) * cs;
                r.Set(eye, dir, 0, 0, 0, 0);
                r.m_mask = 0x20;
                theItemEnv.DoRayCastToMap(r, 0);
                theItemEnv.DoRayCastToContoured(r);
                float lean = stick;
                if (r.m_bHit) {
                    EXVector d = r.Intersect() - eye;
                    float l = d.x * d.x + d.y * d.y + d.z * d.z;
                    float dist = l < 0.0f ? 0.0f : (float)sqrt(l);
                    float room = dist - 0.5f;
                    float zero = 0.0f;
                    float m = *(room > 0.0f ? &room : &zero);
                    lean = stick < m ? (-m < stick ? stick : -m) : m;
                }
                float se = (float)sin(m_Elevation);
                m_Lean = (lean - m_Lean) * 0.25f + m_Lean;
                pCam = m_pCamera;
                float k = (float)fabs(se) * m_Lean;
                float roll = k * 0.3926991f;
                m_LeanRoll = roll;
                eye.x = ss * k + eye.x;
                eye.z = cs * k + eye.z;
                float c = (float)cos(roll);
                float s = (float)sin(roll);
                float e2 = m_Elevation - 1.5707964f;
                float se2 = (float)sin(e2);
                float ce2 = (float)cos(e2);
                float t = 0.0f - (se2 * c + ce2 * 0.0f);
                float uy = ce2 * c - se2 * 0.0f;
                float sa = (float)sin(m_Angle);
                float ca = (float)cos(m_Angle);
                EXVector up;
                up.z = 0.0f - (ca * t + sa * s);
                up.x = ca * s - sa * t;
                up.y = uy;
                up.w = 0.0f;
                pCam->SetWorldUp(up);
            } else {
                pCam = m_pCamera;
                m_Lean = 0.0f;
                m_LeanRoll = 0.0f;
                pCam->SetWorldUp(EXVector(0.0f, 1.0f, 0.0f, 0.0f));
            }
        }
    }
    m_CameraLookAt.x = eye.x;
    m_CameraLookAt.y = eye.y;
    m_CameraLookAt.z = eye.z;
    m_CameraLookAt.w = 0.0f;
    m_CameraPosTarget = m_CameraLookAt;
    m_CameraPos = m_CameraLookAt;
    m_CameraPos.w = 0.0f;
    m_CameraLookAt.w = 0.0f;
    float w = 0.0f;
    if (m_Warp > 0.1f) w = m_Warp - XApp::GameFrameRateMul();
    EXCamera *pCam = m_pCamera;
    float se = (float)sin(m_Elevation);
    m_Warp = w;
    m_CameraLookAt.x = (float)sin(m_Angle) * se + m_CameraLookAt.x;
    m_CameraLookAt.z = (float)cos(m_Angle) * se + m_CameraLookAt.z;
    m_CameraLookAt.y = (float)cos(m_Elevation) + m_CameraLookAt.y;
    pCam->SetTarget(m_CameraLookAt);
    switch (XItemHandler_Player::GetInstance()->GetMode()) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 8:
    case 9: {
        EXPlacementItem *p = gpPlayerItem[m_ViewIndex];
        EXVector r(p->Rotation().x, m_Angle, p->Rotation().z, p->Rotation().w);
        p->m_rotation = r;
        p->m_validMtx = 0;
        break;
    }
    }
}

// NEAR (see check): third person while wall crawling, in the frame of the wall: the look-at eases (by
// the look smoothing) towards a unit out from the wall point, the distance is pulled in by five whiskers
// (centre and 7.5 degrees each way) and eases out slowly / in faster (never under 0.1 or past the centre
// whisker), and the camera's up eases towards the wall's.
void XItemHandler_Camera::SetCamGecko()
{
    EXVector gp = XItemHandler_Player::GetInstance()->GetGeckoPoint();
    EXVector off(0.0f, 0.0f, 1.0f, 0.0f);
    EXMatrix mq;
    EXQuaternion q = XItemHandler_Player::GetInstance()->GetGeckoAngle();
    mq.set_rotate_quat(q);
    EXVector o;
    D3DXVec4Transform(&o, &off, &mq);
    EXMatrix mr;
    mr.set_rotate_xyz(EXVector(-1.5707964f, 0.0f, 0.0f, 0.0f));
    off = o;
    EXMatrix m;
    D3DXMatrixMultiply(&m, &mr, &mq);
    EXVector look = m_CameraLookAt;
    mq = m;
    float k = 1.0f / m_LookSmooth;
    m_CameraLookAt.x = k * ((gp.x - off.x) - look.x) + m_CameraLookAt.x;
    m_CameraLookAt.y = ((gp.y - off.y) - look.y) * k + m_CameraLookAt.y;
    m_CameraLookAt.z = ((gp.z - off.z) - look.z) * k + m_CameraLookAt.z;
    m_CameraLookAt.w = ((gp.w - off.w) - look.w) * k + m_CameraLookAt.w;
    m_pCamera->SetTarget(m_CameraLookAt);
    float d0 = GetWhiskerDistance(0.0f, 0.0f, &mq);
    float d1 = GetWhiskerDistance(0.1308997f, 0.0f, &mq);
    float d2 = GetWhiskerDistance(-0.1308997f, 0.0f, &mq);
    float d3 = GetWhiskerDistance(0.0f, 0.1308997f, &mq);
    float d4 = GetWhiskerDistance(0.0f, -0.1308997f, &mq);
    m_TargetDistance = *(d1 < m_TargetDistance ? &d1 : &m_TargetDistance);
    m_TargetDistance = *(d2 < m_TargetDistance ? &d2 : &m_TargetDistance);
    m_TargetDistance = *(d3 < m_TargetDistance ? &d3 : &m_TargetDistance);
    m_TargetDistance = *(d4 < m_TargetDistance ? &d4 : &m_TargetDistance);
    float rate = 5.0f;
    if (m_TargetDistance > m_Distance) rate = 15.0f;
    m_Distance = (m_TargetDistance - m_Distance) / rate + m_Distance;
    float lo = 0.1f;
    m_Distance = *(m_Distance < lo ? &lo : &m_Distance);
    m_Distance = *(d0 < m_Distance ? &d0 : &m_Distance);
    float se = (float)sin(m_Elevation);
    EXVector dir, od;
    dir.w = 0.0f;
    dir.x = -((float)sin(m_Angle) * se);
    dir.z = -((float)cos(m_Angle) * se);
    dir.y = -(float)cos(m_Elevation);
    D3DXVec4Transform(&od, &dir, &mq);
    float dist = m_Distance;
    m_CameraPosTarget.x = dist * od.x + m_CameraLookAt.x;
    m_CameraPosTarget.y = m_CameraLookAt.y + dist * od.y;
    m_CameraPosTarget.z = m_CameraLookAt.z + dist * od.z;
    m_CameraPosTarget.w = m_CameraLookAt.w + dist * od.w;
    EXVector upv(0.0f, 1.0f, 0.0f, 0.0f), up;
    D3DXVec4Transform(&up, &upv, &mq);
    EXCamera *pCam = m_pCamera;
    EXVector cur = pCam->WorldUp();
    EXVector nu((up.x - cur.x) * 0.125f + cur.x, (up.y - cur.y) * 0.125f + cur.y, (up.z - cur.z) * 0.125f + cur.z,
                (up.w - cur.w) * 0.125f + cur.w);
    pCam->SetWorldUp(nu);
}
