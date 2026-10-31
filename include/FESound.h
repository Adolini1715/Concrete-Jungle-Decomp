#ifndef FESOUND_H
#define FESOUND_H
#include "types.h"

// Reference-counted SFX fade for front-end screens (FEWnd.obj).
class FESound {
public:
    static void Init();
    static void FadeUp(int time);
    static void FadeDown(int time);
private:
    static int m_RefCount;
};
#endif
