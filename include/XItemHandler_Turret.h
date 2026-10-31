#ifndef XITEMHANDLER_TURRET_H
#define XITEMHANDLER_TURRET_H
class XItem;
#include "EXItemHandler.h"
#include "EXTexSwapInfo.h"
#include "EXExtRenderInfoStandIn.h"

class XItemList;
class EXVector;
class FEObject;
class TracerFire;
class EXItemAnimator_Script;
struct ScanInfo;

// Gun turret (FX_Turret.obj). sizeof 0x15c. m_State 3 = disabled.
class XItemHandler_Turret : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_Turret)
public:
    virtual ~XItemHandler_Turret();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    XItemHandler_Turret() {}
    static signed char ScanCallback(signed char b, FEObject *pObj, ScanInfo *pInfo);
    static XItemHandler_Turret *CreateObject();
    signed char CanSeePoint(EXVector &p);
    void DeleteMuzzleScript();
    signed char GetScanInfo(int type, ScanInfo *pInfo);
    int CanSeePredator();
    void PlayMuzzleScript();
    void UpdateMuzzleScript();
    TracerFire *m_pTracers[8];              // 0x08
    EXItemAnimator_Script *m_pMuzzleScript; // 0x28
    u32 m_Unk2c;                            // 0x2c
    float m_Unk30;                          // 0x30
    s8 m_Unk34;                             // 0x34
    s8 m_Unk35;                             // 0x35
    u8 m_Unk36[0x38 - 0x36];
    int m_Id;                               // 0x38 trigger id
    int m_Links[8];                         // 0x3c linked trigger ids
    XItem *m_pLinks[8];                     // 0x5c resolved links
    union {
        u8 m_Options;                       // 0x7c trigger options
        struct {
            u8 m_bOption0 : 1;              // bit 0
            u8 m_bOption1 : 1;              // bit 1
            u8 m_bCloakHides : 1;           // bit 2 cloak hides the player
            u8 m_OptionBits : 5;
        };
    };
    u8 m_Unk7d[0x80 - 0x7d];
    EXTexSwapInfo m_TexSwapInfo;            // 0x80
    EXExtRenderInfo m_ExtRenderInfo;        // 0x94
    float m_SweepAngle;                     // 0x114 trigger data 0 (degrees)
    float m_BaseYaw;                        // 0x118 item rotation y at creation
    float m_BasePitch;                      // 0x11c item rotation x at creation
    float m_SweepSpeed;                     // 0x120 data 7 / 1000 (default 0.01)
    float m_Range;                          // 0x124
    float m_HAngle;                         // 0x128
    float m_VAngle;                         // 0x12c
    float m_Unk130;                         // 0x130
    int m_State;                            // 0x134
    u32 m_Unk138;
    int m_Unk13c;                           // 0x13c data 3
    int m_Unk140;                           // 0x140 data 4 (non-zero: starts inactive)
    unsigned int m_Weapon;                  // 0x144 data 5 (default 0x49000035)
    unsigned int m_Script;                  // 0x148 data 8 (script hash or 0)
    unsigned int m_Script2;                 // 0x14c data 9
    float m_Unk150;                         // 0x150 data 10 (degrees)
    float m_Unk154;                         // 0x154 data 11 (degrees)
    int m_TargetSubGroup;                   // 0x158 actor sub-group also targeted
};

extern XItemList gTurretList;
void Draw_TurretViewcones();
#endif
