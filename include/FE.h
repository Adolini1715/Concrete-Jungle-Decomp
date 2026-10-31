#ifndef FE_H
#define FE_H
#include "types.h"
#include "EXStack.h"

class FEObject;

// Front-end construction helpers: a stack of parent objects plus eight marker slots (FEWnd.obj).
class FE {
public:
    static FEObject *_Marker(int i);
    static void _Push(FEObject *p);
    static void _Pop();
    static FEObject *_Peek();
    static void _SetMarker(int i);
    static void _Init(FEObject *pRoot);
private:
    static EXStack<FEObject *> m_Stack;
    static FEObject *m_pMarker[8];
};
#endif
