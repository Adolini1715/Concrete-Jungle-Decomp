#ifndef XLENSFLARE_H
#define XLENSFLARE_H
#include "EXLensFlare.h"

class EXWnd;
class EXVector;

// Game lens flare (FX_LensFlare.obj; default ctor inline in NightAndDay.obj).
class XLensFlare : public EXLensFlare {
public:
    XLensFlare() {}
    XLensFlare(int count);
    void Render2(EXWnd *pWnd, const EXVector &a, const EXVector &b, const EXVector &c);
};

extern signed char LensFlareActive;
extern signed char SunTrigger;
extern EXVector Sun;
#endif
