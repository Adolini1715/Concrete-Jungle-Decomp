#include "EXRand.h"

EXRandFastClass g_EXRandFastClass(0);
EXRandClass g_EXRandClass(0);

void EXRandClass::SetSeed(u32 seed) { m_Seed = seed; }
void EXRandClass::SetState(const EXRandClass &o) { m_Seed = o.m_Seed; }
void EXRandClass::GetState(EXRandClass &o) const { o.m_Seed = m_Seed; }

u32 EXRandClass::Rand32()
{
    u32 a = Next();
    u32 b = Next();
    return (a & 0xffff0000) | (b >> 16);
}

int EXRandClass::Rand(int n)
{
    if (n == 0) return 0;
    return Rand32() % (u32)n;
}

int EXRandClass::Rand(int lo, int hi)
{
    return Rand(hi - lo) + lo;
}

float EXRandClass::Randf()
{
    return (float)(int)(Rand32() >> 1) / 2147483648.0f;
}

float EXRandClass::Randf(float max)
{
    return (float)(int)(Rand32() >> 1) * max / 2147483648.0f;
}

float EXRandClass::Randf(float lo, float hi)
{
    return (float)(int)(Rand32() >> 1) * (hi - lo) / 2147483648.0f + lo;
}
