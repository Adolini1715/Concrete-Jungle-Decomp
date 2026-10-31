#ifndef XITEMHANDLER_CAMERA_H
#define XITEMHANDLER_CAMERA_H
#include "EXItemHandler.h"
#include "EXVector.h"
#include "EXSoundTagStandIn.h"

class EXCamera;
class EXSphere;
class EXMatrix;
class XItem;
class Actor;
class Vehicle;

// The game camera (Camera.obj, Camera_Lockon.obj). sizeof 0x11c. Modes (m_Mode): 0 third person,
// 1 first person (scan/zoom), 2 first person (forced). Lock-on code lives in Camera_Lockon.obj.
class XItemHandler_Camera : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_Camera)
public:
    virtual ~XItemHandler_Camera();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    XItemHandler_Camera() {}
    static XItemHandler_Camera *CreateObject();

    int GetTargetID() { return m_TargetID; }
    float Zoom() { return m_Zoom; }
    EXVector GetCameraPosition() { return m_CameraPos; }
    void SetCameraLookAt(EXVector v);
    void SetCameraPosition(EXVector v);
    u8 GetMode() { return m_Mode; }
    int GetPlasmaID() { return m_PlasmaID; }
    int GetHitList(int i) { return m_HitList[i]; }
    EXCamera *GetCamera() { return m_pCamera; }
    void SetMode(u8 m) { m_Mode = m; }
    void SetCutsceneFOV(float f) { m_CutsceneFOV = f; }
    void SetNext(float a, float b)
    {
        m_NextAngle = a;
        m_NextElevation = b;
        m_bNext = 1;
    }
    EXVector GetCameraLookAt() { return m_CameraLookAt; }
    void GoFirstPerson(s8 b) { m_bGoFirstPerson = b; }
    void SetBoundingRadius(int r) { m_BoundingRadius = r; }
    s8 GetInvertX() { return m_bInvertX; }
    s8 GetInvertY() { return m_bInvertY; }
    s8 GetInvertFPX() { return m_bInvertFPX; }
    s8 GetInvertFPY() { return m_bInvertFPY; }
    void SetInvertX(s8 b) { m_bInvertX = b; }
    void SetInvertY(s8 b) { m_bInvertY = b; }
    void SetInvertFPX(s8 b) { m_bInvertFPX = b; }
    void SetInvertFPY(s8 b) { m_bInvertFPY = b; }
    s8 GetFPFace() { return m_bFPFace; }
    void SetFPFace(s8 b) { m_bFPFace = b; }
    void SetCamera(EXCamera *p) { m_pCamera = p; }
    float CutsceneFOV() { return m_CutsceneFOV; }
    EXVector GetListenerPosition() { return m_ListenerPos; }
    void SetViewIndex(unsigned int i) { m_ViewIndex = i; }
    float Warp() { return m_Warp; }
    float GetDistance() { return m_Distance; }
    XItem *GetVehicleTarget() { return m_pVehicleTarget; }
    XItem *GetJumpTarget() { return m_pJumpTarget; }

    int BoundingVolumeOnScreen(EXSphere &s);
    s8 IsFirstPerson();
    void SetFirstPerson();
    void SetThirdPerson();
    float Angle();
    void VehicleRemoved(Vehicle *pVehicle);
    void CameraPostUpdate();
    void SetKillCam(float t);
    float Elevation();
    void *GetLockOn(int &type);
    void ClearLockon();
    void ClearLockonX();
protected:
    void SetCamGeckoFP();
    void SetCamFirstPerson();
    float GetWhiskerDistance(float a, float b, EXMatrix *pMtx);
    void SetCamFixed();
    void SetCamGecko();
    void SetCamDebugFree();
    void SetCamDebugFixed();
    void UpdateIndicators();
    void SetCamThirdPerson();
    void SetCamSpline();
    s8 InDirectionOf(EXVector2 &dir, unsigned int flags);
    float LockNearestVehicle(float f);
    float LockNearestJumpTarget(float f);
    float LockNearestAirVehicle(float f);
    s8 IsActorValidTarget(Actor *pActor);
    s8 IsActorValidAutoTarget(Actor *pActor);
    s8 IsActorValidTargetLite(Actor *pActor);
    void AutoTarget();
    void MoveLockon();
    float LockNearestActor(float f);
    void PointCameraAt(EXVector &v);
    float GetFPDistance(EXVector &v);
    s8 IsActorValidTarget(int id);
    s8 PointWithin(EXVector &a, EXVector &b, float c, float d, float e, float f);
    s8 IsActorViableTarget(Actor *pActor, EXVector &v, float a, float b);
    float LockCentredActor(float f);
    float LockCentredVehicle(float f);
    float LockCentredAirVehicle(float f);
    float LockCentredMimicry(float f);
    float LockCentredTurret(float f);
    float LockCentredCCTV(float f);
    float LockCentredVMO(float f);
    void RangeTarget();
public:
    EXCamera *m_pCamera;            // 0x08
    float m_Angle;                  // 0x0c yaw
    float m_Elevation;              // 0x10
    float m_LeanRoll;               // 0x14 first-person lean roll (SetCamFirstPerson)
    float m_Distance;               // 0x18
    float m_TargetAngle;            // 0x1c
    float m_TargetElevation;        // 0x20
    float m_TargetDistance;         // 0x24
    float m_28;                     // 0x28 TODO (default elevation)
    float m_2c;                     // 0x2c TODO
    float m_LookSmooth;             // 0x30 look-at smoothing, in 60 Hz frames (10, 20 hanging)
    float m_Zoom;                   // 0x34
    u8 m_Unk38[0x3c - 0x38];
    int m_LockDelay;                // 0x3c frames before the stick may move the lock-on again (MoveLockon)
    float m_Warp;                   // 0x40
    float m_SideOffset;             // 0x44 look-at shifted behind the player (0..1, ramps 0.1/frame)
    float m_CutsceneFOV;            // 0x48
    float m_IdleTime;               // 0x4c seconds without stick/button input (idle orbit after 60)
    float m_NextAngle;              // 0x50
    float m_NextElevation;          // 0x54
    s8 m_bNext;                     // 0x58
    u8 m_Unk59[3];
    float m_KillAngle;              // 0x5c orbit saved by SetKillCam (-666: no kill cam view found)
    float m_KillElevation;          // 0x60
    float m_Lean;                   // 0x64 first-person lean, eased to the clamped stick
    int m_LostSight;                // 0x68 frames the locked actor has been out of line of sight
    EXVector m_CameraPos;           // 0x6c
    EXVector m_CameraPosTarget;     // 0x7c where the camera mode wants the camera
    EXVector m_CameraLookAt;        // 0x8c
    EXVector m_ListenerPos;         // 0x9c
    unsigned int m_ViewIndex;       // 0xac
    u8 m_PadIndex;                  // 0xb0
    u8 m_Unkb1[0xb8 - 0xb1];
    XItem *m_pLockIndicator;        // 0xb8 lock-on marker; its script restarts on a new target (MoveLockon)
    XItem *m_pPlasmaIndicator;      // 0xbc marker for the shoulder cannon / disk auto target (RangeTarget)
    XItem *m_pVehicleIndicator;     // 0xc0 lock-on marker for traffic vehicles (LockCentredVehicle)
    XItem *m_pHitIndicator[3];      // 0xc4 markers on the recent hit list's actors (UpdateIndicators)
    int m_TargetID;                 // 0xd0
    int m_PlasmaID;                 // 0xd4
    int m_HitList[3];               // 0xd8 TODO count
    XItem *m_pVehicleTarget;        // 0xe4
    XItem *m_pJumpTarget;           // 0xe8
    void *m_pLockTarget[4];         // 0xec TODO (air vehicle / turret / CCTV / VMO lock-ons)
    u8 m_Mode;                      // 0xfc
    u8 m_Unkfd[0x100 - 0xfd];
    s8 m_bLockHeld;                 // 0x100 lock-on button must be released before locking again
    u8 m_Unk101[3];
    unsigned int m_PrevPlayerMode;  // 0x104 player m_Mode last frame
    s8 m_bInvertX;                  // 0x108
    s8 m_bInvertY;                  // 0x109
    s8 m_bInvertFPX;                // 0x10a
    s8 m_bInvertFPY;                // 0x10b
    s8 m_bFPFace;                   // 0x10c
    s8 m_bFirstPerson;              // 0x10d first person within mode 1
    s8 m_bGoFirstPerson;            // 0x10e
    u8 m_Unk10f;
    int m_BoundingRadius;           // 0x110
    EXSoundTag m_SoundTag;          // 0x114
};
XItemHandler_Camera *XGetCamera(unsigned int i);
extern float CameraDistance;
inline XItemHandler_Camera *XGetCamera() { return XGetCamera(0); }
#endif
