#ifndef FEWND_H
#define FEWND_H
#include "FEBaseWnd.h"

// Front-end window owning a root FEObject (FEWnd.obj). sizeof at least 0x3fc.
// m_State: 0x1 input, 0x20 content created, 0x40 draw object, 0x10000000 3D camera.
class FEWnd : public FEBaseWnd {
public:
    virtual ~FEWnd();
    virtual signed char Update();
    virtual signed char Draw();
    virtual void ContentCreate() {}
    virtual void ContentUpdate() { HandleInput(); }
    virtual void ContentDestroy() { m_ContentDestroyed = 1; }
    virtual char *DebugName() { return "WndBase"; }
    FEWnd();
    FEWnd(FEObject *pObj);

    void Delete() { m_DeleteNeeded = 1; }
    FEObject *Object() { return m_pObject; }
    void StateSet(unsigned int s) { m_State = s; }
    void StateAdd(unsigned int s) { m_State |= s; }
    void StateSub(unsigned int s) { m_State &= ~s; }
    unsigned int State() { return m_State; }
    void EnvironmentStateSet(unsigned int s) { m_EnvironmentState = s; }
    void EnvironmentStateAdd(unsigned int s) { m_EnvironmentState |= s; }
    unsigned int EnvironmentState() { return m_EnvironmentState; }
    unsigned int FrameCount() { return m_FrameCount; }
    void StatePush() { m_StateStack.Push(m_State); }
    void StatePop() { m_StateStack.Pop(m_State); }
    signed char ContentDestroyed() { return m_ContentDestroyed; }

    void BringToTop();
    void SendToBottom();
    static void FixupZOrder();
    FEObject *SetObject(FEObject *pObj);
    __declspec(noinline) void SetRect(EXRect r);
    void HandleInput();
    void DebugDump();
    void Cleanup();
    __declspec(noinline) void Init(FEObject *pObj);
    void AlwaysOnTop();

    FEObject *m_pObject;                 // 0x3c8
    unsigned int m_State;                // 0x3cc
    unsigned int m_EnvironmentState;     // 0x3d0 0x1 fog off while drawing
    EXStack<unsigned int> m_StateStack;  // 0x3d4
    unsigned int m_FrameCount;           // 0x3f0
    unsigned int m_LastUpdateFrame;      // 0x3f4
    signed char m_DeleteNeeded;          // 0x3f8
    signed char m_ContentDestroyed;      // 0x3f9
};
#endif
