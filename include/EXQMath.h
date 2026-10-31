#ifndef EXQMATH_H
#define EXQMATH_H
#include <math.h>

// Quick float maths (x87 intrinsics). Bodies verified against the Actor.obj copies.
// TODO owner: the maths header these came from is unknown; move if it turns up.
inline float qsinf(float x) { return (float)sin(x); }
inline float qcosf(float x) { return (float)cos(x); }
inline void qsincosf(float x, float &s, float &c)
{
    s = (float)sin(x);
    c = (float)cos(x);
}
inline float qasinf(float x)
{
    if (x < -1.0f) return -1.5707964f;
    if (x > 1.0f) return 1.5707964f;
    return asinf(x);
}
inline float qacosf(float x)
{
    if (x < -1.0f) return 3.1415927f;
    if (x > 1.0f) return 0.0f;
    return acosf(x);
}
inline float qtanf(float x) { return tanf(x); }
inline float qatan2f(float y, float x) { return (float)atan2(y, x); }
inline float qfabsf(float x) { return (float)fabs(x); }
inline float qsqrtf(float x)
{
    if (x >= 0.0f) return (float)sqrt(x);
    return 0.0f;
}
inline float DegToRad(float d) { return d * 0.017453292f; }
inline float RadToDeg(float r) { return r * 57.29578f; }
#endif
