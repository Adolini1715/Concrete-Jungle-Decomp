#ifndef EXRAND_H
#define EXRAND_H
#include "types.h"

// Linear congruential generator (MSVC rand constants), two steps per 32-bit result.
class EXRandClass {
public:
    EXRandClass(u32 seed) { m_Seed = seed; }
    void SetSeed(u32 seed);
    void SetState(const EXRandClass &o);
    void GetState(EXRandClass &o) const;
    u32 Rand32();
    // appended by the nav agent ([fi] copy in NavNetwork.obj)
    __forceinline int Rand31() { return Rand32() >> 1; }
    int Rand(int n);
    int Rand(int lo, int hi);
    float Randf();
    float Randf(float max);
    float Randf(float lo, float hi);
private:
    u32 Next() { m_Seed = m_Seed * 0x343fd + 0x269ec3; return m_Seed; }
    u32 m_Seed;
};

class EXRandFastClass {
public:
    EXRandFastClass(u32 seed) { m_Seed = seed; }
    u32 Rand32() { m_Seed = m_Seed * 0x19660d + 0x3c6ef35f; return m_Seed; }
private:
    u32 m_Seed;
};

extern EXRandFastClass g_EXRandFastClass;
extern EXRandClass g_EXRandClass;
inline unsigned int EXRandFast32() { return g_EXRandFastClass.Rand32(); }  // [fi] in EXParticle.obj
inline int EXRandFast(int n)
{
    if (!n) return 0;
    return EXRandFast32() % n;
}
inline float EXRandFastf(float max) { return (float)(int)(EXRandFast32() >> 1) * max * (1.0f / 2147483648.0f); }
inline float EXRandFastf(float lo, float hi) { return (float)(int)(EXRandFast32() >> 1) * (hi - lo) * (1.0f / 2147483648.0f) + lo; }
// appended by the nav agent ([fi] copy in NavNetwork.obj)
__forceinline int EXRand() { return g_EXRandClass.Rand31(); }
// appended by the actor agent ([fi] copies in Actor.obj)
inline unsigned int EXRand32() { return g_EXRandClass.Rand32(); }
inline int EXRand(int n)
{
    if (!n) return 0;
    return g_EXRandClass.Rand32() % n;
}
inline int EXRand(int lo, int hi) { return EXRand(hi - lo) + lo; }
inline float EXRandf() { return g_EXRandClass.Randf(); }
inline float EXRandf(float max) { return g_EXRandClass.Randf() * max; }
inline float EXRandf(float lo, float hi) { return g_EXRandClass.Randf() * (hi - lo) + lo; }
#endif
