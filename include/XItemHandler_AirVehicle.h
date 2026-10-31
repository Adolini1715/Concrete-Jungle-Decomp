#ifndef XITEMHANDLER_AIRVEHICLE_H
#define XITEMHANDLER_AIRVEHICLE_H
#include "EXItemHandler.h"
#include "EXVector.h"
#include "EXTexSwapInfo.h"
#include "EXExtRenderInfoStandIn.h"
#include "EXSoundTagStandIn.h"

class XItemList;
class FEObject;
class TracerFire;
class EXItemAnimator_Script;
struct ScanInfo;

// Police helicopter / air traffic vehicle (FX_AirTraffic.obj). sizeof 0x15c.
// m_State (0x114): 0 waiting, 2 approaching, 5 hovering (UpdateEmergency).
class XItemHandler_AirVehicle : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_AirVehicle)
public:
    virtual ~XItemHandler_AirVehicle();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    XItemHandler_AirVehicle() {}
    void InitTracers();
    void CreateBluesAndTwos();
    void DeleteBluesAndTwos();
    void UpdateEmergency();
    signed char NodeOccupied(int node);
    static signed char ScanCallback(signed char b, FEObject *pObj, ScanInfo *pInfo);
    static XItemHandler_AirVehicle *CreateObject();
    signed char CanSeePredator();
    void UpdateCrashing();
    void DeleteMuzzleScript();
    void ApplyDamage(float f);
    void HandleVisionModeTextureSwaps(unsigned char mode);
    signed char GetScanInfo(int type, ScanInfo *pInfo);
    void PlayMuzzleScript();
    void UpdateMuzzleScript();
    void UpdateTraffic();
    void UpdatePatrol();
    EXItemAnimator_Script *m_pMuzzleScript; // 0x08
    TracerFire *m_pTracers[8];              // 0x0c
    int m_NextTracer;                       // 0x2c tracer slot used next (0..7)
    int m_DamageTimer;                      // 0x30 frames before the next hit counts; crash fuse
    EXTexSwapInfo m_TexSwapInfo;            // 0x34
    EXExtRenderInfo m_ExtRenderInfo;        // 0x48
    EXVector m_CrashDir;                    // 0xc8 direction of travel when shot down
    EXVector m_Target;                      // 0xd8 hover target; spin rate while crashing
    EXVector m_Pos;                         // 0xe8 current position (bobbing applied on top)
    int m_Path;                             // 0xf8 path (compared in NodeOccupied)
    float m_Speed;                          // 0xfc
    float m_MaxSpeed;                       // 0x100
    float m_PathT;                          // 0x104 spline parameter between m_Node and the next node
    int m_Node;                             // 0x108 current path node
    int m_PrevNode;                         // 0x10c node left last (patrol avoids going straight back)
    u16 m_Unk110;                           // 0x110 TODO
    u16 m_Unk112;                           // 0x112 1 = emergency vehicle (geo section reference held)
    u16 m_State;                            // 0x114
    u16 m_bMoving;                          // 0x116 travelling to m_Node
    float m_Timer;                          // 0x118
    float m_Time;                           // 0x11c bob phase
    float m_Health;                         // 0x120
    int m_bExploded;                        // 0x124 crash ended in the explosion
    s8 m_Unk128;                            // 0x128
    s8 m_Unk129;                            // 0x129
    u8 m_Unk12a[0x130 - 0x12a];             // TODO layout
    float m_FireTimer;                      // 0x130 gunship weapon cooldown
    int m_Burst;                            // 0x134 shots left in the burst (<0: rockets)
    int m_SeesPredator;                     // 0x138 cached CanSeePredator, -666 not checked yet
    int m_Profile;                          // 0x13c actor profile hash
    EXSoundTag m_SoundTag;                  // 0x140
    EXVector m_LastPos;                     // 0x148 item position last frame (DoUpdate)
    float m_SoundTimer;                     // 0x158 spotted-sound cooldown on patrol; rotor pitch while crashing
};

extern int gAirVehicle_Count;
extern XItemList gAirVehicleList;
extern signed char AirLights;
#endif
