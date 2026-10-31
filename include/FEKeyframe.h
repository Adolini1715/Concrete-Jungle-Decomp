#ifndef FEKEYFRAME_H
#define FEKEYFRAME_H
#include "types.h"
#include "EXList.h"
#include "EXRect.h"
#include "EXVector.h"

class FEObject;

// One queued move of an FEObject's origin (FEObject.obj; list item at 0). sizeof 0x20.
class FEKeyframe : public EXDListItem {
public:
    FEKeyframe(FEObject *pObj, EXPoint target, float time);
    ~FEKeyframe() {}
    float Update(float dt);
    void MoveToTarget();

    FEObject *m_pObject;   // 0x08
    EXVector2 m_Target;    // 0x0c
    EXVector2 m_Start;     // 0x14 origin when queued
    float m_Time;          // 0x1c time left
};
#endif
