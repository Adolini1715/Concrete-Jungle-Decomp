#ifndef XITEMHANDLER_TESTDRONE_H
#define XITEMHANDLER_TESTDRONE_H
#include "EXItemHandler.h"
#include "EXVector.h"

class EXItemAnimator;
struct GEOSCRIPT_EVENT;

// A hit on the test drone (TestDrone.obj). sizeof 0x24.
struct TestHit {
    EXVector m_Pos;     // 0x00
    EXVector m_Dir;     // 0x10
    float m_Force;      // 0x20
};

// Severed limb flying off (TestDrone.obj). sizeof 0x1c.
class XItemHandler_LostLimb : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_LostLimb)
public:
    virtual signed char Initialise();
    virtual signed char DoUpdate();
    XItemHandler_LostLimb() {}
    static XItemHandler_LostLimb *CreateObject();
    EXVector m_Vel;     // 0x08
    int m_Life;         // 0x18 frames
};

// Test target drone (TestDrone.obj). sizeof 0x98.
class XItemHandler_TestDrone : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_TestDrone)
public:
    virtual ~XItemHandler_TestDrone();
    virtual signed char Initialise();
    virtual signed char DoUpdate();
protected:
    virtual unsigned int DoScriptCmdEvent(GEOSCRIPT_EVENT *pEvent, EXItemAnimator *pAnim);
public:
    XItemHandler_TestDrone();
    void TakeAHit(const TestHit &hit);
    static XItemHandler_TestDrone *CreateObject();
protected:
    int FindHitLimb(const TestHit &hit);
    void ProcessHit();
    void CheckModeChange(unsigned int mode);
    void ForceModeChange(unsigned int mode);
    void MoveNormal();
    void SetupDepthPtr(float *p, unsigned int stack);
    void SetupIdle();
    void SetupMove();
    void DoIdle();
    void DoMove();
public:
    int m_Unk8;             // 0x08
    int m_bHit;             // 0x0c hit pending
    TestHit m_Hit;          // 0x10
    u8 m_Unk34[0x3c - 0x34];
    float m_MoveDepth;      // 0x3c
    float m_IdleDepth;      // 0x40
    int m_Mode;             // 0x44 0 idle, 1 move
    u8 m_Unk48[0x6c - 0x48];
    int m_Unk6c;            // 0x6c
    u8 m_Unk70[0x98 - 0x70];
};
#endif
