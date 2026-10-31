#ifndef EXGEOVIEWERWND2_H
#define EXGEOVIEWERWND2_H
#include "EXWnd.h"

// TODO stand-in: engine geometry viewer window 2 (EXGeoViewer2.obj, not this area).
class EXGeoViewerWnd2 : public EXWnd {
public:
    virtual ~EXGeoViewerWnd2();
    virtual s8 InitialUpdate();
    virtual s8 Update();
    virtual s8 Draw();
    EXGeoViewerWnd2();
    u8 m_Unk29c[0x520 - 0x29c];     // TODO layout/size
};
#endif
