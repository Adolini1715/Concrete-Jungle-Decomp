#ifndef FELANG_H
#define FELANG_H
#include "FEWnd.h"
#include "XAppStandIn.h"

// Language-select page (FELang.obj). Singleton, sizeof 0x40c.
class FELang : public FEWnd {
public:
    virtual ~FELang()
    {
        ContentDestroy();
        m_pInstance = 0;
    }
    virtual signed char Draw();
    virtual void ContentCreate();
    virtual void ContentUpdate();
    virtual void ContentDestroy();
    virtual char *DebugName() { return "FELang"; }

    void TickPage()
    {
        float dt = XApp::FrameLength();
        m_PageStateTimer += dt;
        m_PageSubstateTimer += dt;
    }
    static FELang *Instance(signed char create);
    void Destroy();

    u32 m_Unk3fc;               // 0x3fc TODO
    u32 m_Unk400;               // 0x400 TODO
    float m_PageStateTimer;     // 0x404
    float m_PageSubstateTimer;  // 0x408

private:
    FELang() {}
    static FELang *m_pInstance;
};
inline FELang *FEGetLang(signed char create) { return FELang::Instance(create); }  // [fi] App.obj
#endif
