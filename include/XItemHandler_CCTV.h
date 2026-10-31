#ifndef XITEMHANDLER_CCTV_H
#define XITEMHANDLER_CCTV_H
#include "EXItemHandler.h"
#include "EXSoundTagStandIn.h"

class XItem;
class XItemList;
class EXVector;
class FEObject;
struct ScanInfo;

// Security camera (FX_CCTV.obj). sizeof 0x90. m_State 3 = disabled.
class XItemHandler_CCTV : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_CCTV)
public:
    virtual ~XItemHandler_CCTV();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    XItemHandler_CCTV() {}
    static XItemHandler_CCTV *CreateObject();
    static signed char ScanCallback(signed char b, FEObject *pObj, ScanInfo *pInfo);
    signed char CanSeePredator();
    signed char GetScanInfo(int type, ScanInfo *pInfo);
    signed char CanSeePoint(EXVector &p);
    s8 m_bAlarm;                // 0x08
    s8 m_bActive;               // 0x09
    u8 m_Unk0a[2];
    int m_Id;                   // 0x0c trigger id
    int m_Links[8];             // 0x10 linked trigger ids
    XItem *m_pLinks[8];         // 0x30
    union {
        u32 m_Options;          // 0x50 trigger options
        struct {
            u32 m_bCloakHides : 1;  // cloak hides the player
            u32 m_OptionBits : 31;
        };
    };
    float m_SweepAngle;         // 0x54 (radians)
    float m_BaseYaw;            // 0x58 item yaw at creation
    float m_SweepSpeed;         // 0x5c (default 0.01)
    float m_Range;              // 0x60
    float m_HAngle;             // 0x64 half field of view (yaw)
    float m_VAngle;             // 0x68 half field of view (pitch, defaults to m_HAngle)
    u32 m_Unk6c;                // 0x6c
    unsigned int m_Script;      // 0x70 (> 0x4000000 or 0)
    float m_Unk74;              // 0x74 (radians)
    float m_Unk78;              // 0x78 (radians, default pi)
    u8 m_Unk7c[0x84 - 0x7c];    // 0x7c TODO
    int m_State;                // 0x84
    EXSoundTag m_SoundTag;      // 0x88
};

extern XItemList gCCTVList;
void Draw_CameraViewcones();
#endif
