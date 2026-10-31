#ifndef XOBJVIEWWND2_H
#define XOBJVIEWWND2_H
#include "EXGeoViewerWnd2.h"

// Game object viewer 2 (ObjViewWnd2.obj).
class XObjViewWnd2 : public EXGeoViewerWnd2 {
public:
    virtual s8 MenuSelect(unsigned int id);
    XObjViewWnd2();

    u8 m_Unk520[0x5b0 - 0x520];  // TODO layout (sizeof 0x5b0 from XApp::InitGame's new)
};
#endif
