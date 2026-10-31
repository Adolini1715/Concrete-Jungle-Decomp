#ifndef FEMATHMOS_H
#define FEMATHMOS_H
#include "types.h"
#include "RGBA.h"
#include "FEFlags.h"

// Colour animator ("lava lamp"): fades m_Colour between a start and an end colour (FENavigator.obj).
// sizeof 0x28.
class FEMathmos {
public:
    FEMathmos();
    FEMathmos(ex::RGBA c);
    FEMathmos(ex::RGBA from, ex::RGBA to);
    FEMathmos(ex::RGBA from, ex::RGBA to, float speed, FEWaveform::Flags wave, signed char enabled);
    void Init(ex::RGBA from, ex::RGBA to, float speed, FEWaveform::Flags wave, signed char enabled);
    void Goto(FEMathmosPos::Flags pos);
    static ex::RGBA Validate(ex::RGBA c);
    ex::RGBA Interpolate(float f);
    static FEMathmos DefaultTint();
    static FEMathmos DefaultTintFocus();
    ex::RGBA Update();

    ex::RGBA Colour() { return m_Colour; }
    signed char IsFinished() { return m_Finished; }
    void SetEndColour(ex::RGBA c) { m_To = c; }
    static FEMathmos DefaultFadeUp()
    {
        return FEMathmos(ex::RGBA(0, 0, 0, 0), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 30.0f, FEWaveform::WF_LINEAR, 1);
    }
    static FEMathmos DefaultScroll()
    {
        return FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), ex::RGBA(0x7f, 0, 0, 0x80), 60.0f, FEWaveform::WF_LINEAR, 0);
    }
    void GotoFraction(float f)
    {
        m_Wave = (FEWaveform::Flags)6;
        m_Time = 1.0f - f;
    }

    ex::RGBA m_From;          // 0x00
    ex::RGBA m_To;            // 0x04
    ex::RGBA m_Colour;        // 0x08
    float m_Speed;            // 0x0c
    float m_Time;             // 0x10
    float m_Phase;            // 0x14
    signed char m_Enabled;    // 0x18
    signed char m_Finished;   // 0x19 at the end colour
    FEWaveform::Flags m_Wave; // 0x1c
    u32 m_Unk20;              // 0x20
    u32 m_Unk24;              // 0x24
};
#endif
