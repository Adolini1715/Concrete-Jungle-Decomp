#ifndef XITEMHANDLER_RADARBOX_H
#define XITEMHANDLER_RADARBOX_H
#include "EXItemHandler.h"
#include "EXGeoTrigger.h"

// Radar display volume (FERadar.obj). sizeof 0x3c.
class XItemHandler_RadarBox : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_RadarBox)
public:
    virtual ~XItemHandler_RadarBox();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    XItemHandler_RadarBox() {}
    static XItemHandler_RadarBox *CreateObject();
    EXGeoBaseDatum m_Volume;    // 0x08 trigger collision
    u32 m_Data0;                // 0x38 trigger data 0
};
#endif
