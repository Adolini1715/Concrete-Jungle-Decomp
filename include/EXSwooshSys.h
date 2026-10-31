#ifndef EXSWOOSHSYS_H
#define EXSWOOSHSYS_H
#include "EXBaseSwooshSys.h"

struct EXGeoEntity_VtxDataC;

// Xbox swoosh renderer (XBSwoosh.obj), sizeof 0xd0.
class EXSwooshSys : public EXBaseSwooshSys {
    EX_DECLARE_DYNAMIC(EXSwooshSys)
public:
    virtual ~EXSwooshSys();
    virtual void Draw(u32 pass);
    static EXSwooshSys *CreateObject();
    EXSwooshSys();
private:
    void SetSwooshRenderStates();
    EXGeoEntity_VtxDataC EXSwooshSplinePos(SwooshVTX_Data *pData, int a, int b, float t);
    void DrawTessalatedSwoosh();
public:
    u32 m_cc;                   // +0xcc
};
#endif
