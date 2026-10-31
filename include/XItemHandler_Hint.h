#ifndef XITEMHANDLER_HINT_H
#define XITEMHANDLER_HINT_H
#include "EXItemHandler.h"
#include "EXGeoTrigger.h"

class XItemList;

// On-screen hint marker (FX_Hints.obj). sizeof 0x6c.
class XItemHandler_Hint : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_Hint)
public:
    virtual ~XItemHandler_Hint();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    XItemHandler_Hint() {}
    static XItemHandler_Hint *CreateObject();
    u32 m_Unk08;                // 0x08
    EXGeoBaseDatum m_Volume;    // 0x0c trigger collision (copied for Within())
    int m_Type;                 // 0x3c 1 radius, 2 trigger volume
    unsigned int m_Text;        // 0x40 hint text hash (0x51000000..0x510009b9 objectives text)
    u32 m_Unk44;                // 0x44
    int m_ActivateFlagChannel;  // 0x48
    int m_ShowFlagChannel;      // 0x4c
    int m_Radius;               // 0x50
    int m_SetFlagChannel;       // 0x54
    int m_ClearFlagChannel;     // 0x58
    int m_DeleteFlagChannel;    // 0x5c
    int m_Unk60;                // 0x60
    int m_Unk64;                // 0x64 message time (default 2)
    u32 m_Unk68;                // 0x68
};

extern XItemList gHintList;
#endif
