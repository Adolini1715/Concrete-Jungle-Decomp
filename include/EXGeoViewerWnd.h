#ifndef EXGEOVIEWERWND_H
#define EXGEOVIEWERWND_H
#include "EXWnd.h"

// TODO stand-in: engine geometry viewer window (EXGeoViewer.obj, not this area). Only the virtual
// overrides and the one field the game touches are declared.
class EXGeoViewerWnd : public EXWnd {
public:
    virtual ~EXGeoViewerWnd();
    virtual s8 InitialUpdate();
    virtual s8 Update();
    virtual s8 Draw();
    virtual void ButtonDown(unsigned int);
    virtual void ButtonUp(unsigned int);
    virtual void MouseMove(int, int);
    virtual s8 MenuSelect(unsigned int);
    EXGeoViewerWnd();
    u8 m_Unk29c[0x434 - 0x29c];     // TODO layout
    float m_Unk434;                 // 0x434 TODO name (XObjViewWnd sets 45.0)
    u8 m_Unk438[0x5a0 - 0x438];     // TODO layout/size
};
#endif
