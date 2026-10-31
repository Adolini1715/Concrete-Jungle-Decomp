#ifndef EXEXTRENDERINFOSTANDIN_H
#define EXEXTRENDERINFOSTANDIN_H
#include "types.h"
#include "EXVector.h"
#include "EXStdFuncs.h"

// TODO stand-in: extended render parameters (engine render area; inlines emitted in Actor.obj).
// sizeof 0x80: six light vectors (+0x00..+0x5f), ambient (+0x60), refraction strength (+0x74).
#ifndef EXEXTRENDERINFO_DEFINED
#define EXEXTRENDERINFO_DEFINED
class EXExtRenderInfo {
public:
    EXExtRenderInfo() { SetDefault(); }
    void SetDefault()
    {
        memset(m_Light, 0, sizeof(m_Light) + sizeof(m_Light2));
        m_Ambient.set(0.5f, 0.5f, 0.5f, 1.0f);
        m_Unk78 = 0;
        m_Unk70 = 0.5f;
        m_RefractionStrength = 0.08f;
        m_Unk7c = 0.0f;
    }
    void SetRefractionStrength(float f) { m_RefractionStrength = f; }
    void SetLightData(const EXVector &a, const EXVector &b, const EXVector &c, const EXVector &d, const EXVector &e,
                      const EXVector &f, const EXVector &amb)
    {
        m_Light[0] = a;
        m_Light[1] = b;
        m_Light[2] = c;
        m_Light2[0] = d;
        m_Light2[1] = e;
        m_Light2[2] = f;
        m_Ambient = amb;
    }
    EXVector &Ambient() { return m_Ambient; }
    EXVector m_Light[3];         // 0x00 TODO names (two arrays of 3: retail ctor runs two vector ctor iterators)
    EXVector m_Light2[3];        // 0x30
    EXVector m_Ambient;          // 0x60
    float m_Unk70;               // 0x70
    float m_RefractionStrength;  // 0x74
    u32 m_Unk78;                 // 0x78
    float m_Unk7c;               // 0x7c
};
#endif
#endif
