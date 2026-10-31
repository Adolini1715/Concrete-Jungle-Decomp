#include "XLensFlare.h"
#include "EXVector.h"
#include "FEText.h"
#include <math.h>

signed char LensFlareActive;
signed char SunTrigger;
EXVector Sun;

XLensFlare::XLensFlare(int count)
{
    m_Count = count;
    m_Unkcc = 1.0f;
    m_Unkd4 = 1.0f;
    m_Unkd0 = 200.0f;
    m_Unkd8 = 0.0f;
    m_Unkdc = 0.5f;
    m_Unke0 = 0.5f;
    for (int i = 0; i < m_Count; i++) {
        EXFlare &f = m_Flares[i];
        if (m_Count == 1) f.m_Pos = 1.0f;
        else f.m_Pos = (float)i / (float)(m_Count - 1);
        f.m_Size = 400.0f;
        f.m_Alpha = (float)fabs(1.0f - (f.m_Pos + f.m_Pos));
        u8 c = (u8)(f.m_Pos * 128.0f);
        f.m_Colour = ex::RGBA(c, c, 0x80, 0x80);
        f.m_Texture = 0;
    }
}

// TODO: XLensFlare::Render2 (1296 bytes: projects the sun and draws the flare sprites) not written yet.
