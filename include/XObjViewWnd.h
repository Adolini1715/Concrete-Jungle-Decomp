#ifndef XOBJVIEWWND_H
#define XOBJVIEWWND_H
#include "EXGeoViewerWnd.h"

// Game object viewer (ObjViewWnd.obj).
class XObjViewWnd : public EXGeoViewerWnd {
public:
    virtual s8 MenuSelect(unsigned int id);
    XObjViewWnd();
};
#endif
