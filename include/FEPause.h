#ifndef FEPAUSE_H
#define FEPAUSE_H
#include "FEWnd.h"
#include "FEMathmos.h"
#include "EXString.h"

// Pause menu window and the mission objective list it shows (FEPause.obj).

// One objective (sizeof 0x18). Text is either a string-table hash or a literal string.
class FEObjective {
public:
    FEObjective() {}
    EXString m_Short;           // 0x00
    unsigned int m_ShortHash;   // 0x04
    EXString m_Msg;             // 0x08
    unsigned int m_MsgHash;     // 0x0c
    int m_Type;                 // 0x10 message type flags
    signed char m_bRead;        // 0x14
    signed char m_bComplete;    // 0x15
};

// Singleton list of up to 32 objectives (sizeof 0x304).
class FEObjectives {
public:
    ~FEObjectives()  // emitted in App.obj
    {
        Initialise();
        m_pInstance = 0;
    }
    int Count() { return m_nCount; }
    void Initialise();
    __declspec(noinline) void _Add(unsigned int shortHash, EXString shortMsg, unsigned int msgHash, EXString msg, int type);
    int MsgType(int i);
    signed char IsRead(int i);
    void MarkRead(int i);
    signed char IsComplete(int i);
    void MarkComplete(unsigned int hash);
    signed char NewObjective(int type);
    static FEObjectives *Instance(signed char create);
    void Add(EXString msg, int type);
    void Add(EXString shortMsg, EXString msg, int type);
    void Add(unsigned int hash, int type);
    void Add(unsigned int shortHash, unsigned int msgHash, int type);
    EXString Msg(int i);
    EXString ShortMsg(int i);

    FEObjective m_Objectives[32];  // 0x000
    int m_nCount;                  // 0x300

private:
    FEObjectives()
    {
        m_nCount = 0;
        Initialise();
    }
    static FEObjectives *m_pInstance;
};

// Pause menu (sizeof 0x478).
class FEPause : public FEWnd {
public:
    virtual ~FEPause()
    {
        ContentDestroy();
        m_pInstance = 0;
    }
    virtual void ContentCreate();
    virtual void ContentUpdate();
    virtual void ContentDestroy();
    virtual char *DebugName() { return "WndPause"; }
    FEMathmos *ReadMathmos() { return &m_Mathmos; }
    static FEPause *Instance(signed char create);

    FEMathmos m_Mathmos;    // 0x3fc
    FEMathmos m_Mathmos2;   // 0x424
    FEMathmos m_Mathmos3;   // 0x44c
    u32 m_Unk474;           // 0x474 TODO

private:
    FEPause() { FEWnd::FixupZOrder(); }
    static FEPause *m_pInstance;
};
inline FEPause *FEGetPause(signed char create) { return FEPause::Instance(create); }  // [fi] App.obj
inline FEObjectives *FEGetObjectives(signed char create) { return FEObjectives::Instance(create); }
#endif
