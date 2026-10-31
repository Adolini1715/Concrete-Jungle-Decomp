#include "EXMaths.h"
#include "EXVector.h"
#include "EXMatrix.h"
#include "EXQuaternion.h"
#include "EXLine.h"

#define EX_PI 3.1415927f
#define EX_TWOPI 6.2831855f

extern const u32 _EXCRCTable[256];

s8 isValidFloat(float f, s8 warn);

float EXRoundToNearest(float f, float step)
{
    if (step == 0.0f) return 0.0f;
    if (step < 0.0f) step *= -1.0f;
    if (f > 0.0f) return (float)(int)((step * 0.5f + f) / step) * step;
    if (f < 0.0f) return (float)(int)((f - step * 0.5f) / step) * step;
    return 0.0f;
}

s8 isValidFloat(const EXVector2 &v)
{
    if (isValidFloat(v.x, 0) && isValidFloat(v.y, 0)) return 1;
    return 0;
}

s8 isValidFloat(const EXVector3 &v)
{
    if (isValidFloat(v.x, 0) && isValidFloat(v.y, 0) && isValidFloat(v.z, 0)) return 1;
    return 0;
}

u32 EXCalculateChecksum(const u8 *p, u32 n)
{
    u32 crc = 0xffffffff;
    while (n--) {
        crc = _EXCRCTable[(*p ^ crc) & 0xff] ^ (crc >> 8);
        p++;
    }
    return ~crc;
}

static inline float EXNormaliseAngle(float a)
{
    if (a >= 0.0f && a < EX_TWOPI) return a;
    a = (float)fmod(a, EX_TWOPI);
    if (a >= 0.0f) return a;
    return a + EX_TWOPI;
}

float AngleDiff(float a, float b)
{
    float d = EXNormaliseAngle(b) - EXNormaliseAngle(a);
    if ((float)fabs(d) < EX_PI) return d;
    if (d < 0.0f) return d + EX_TWOPI;
    return d - EX_TWOPI;
}

float InterpAngle(float a, float b, float t)
{
    float d = b - a;
    if (d < -EX_PI) a -= EX_TWOPI;
    else if (d > EX_PI) a += EX_TWOPI;
    float r = (b - a) * t + a;
    float res;
    if ((float)fabs(r) < EX_PI) res = r;
    else {
        float m = (float)fmod(r, EX_TWOPI);
        if ((float)fabs(m) < EX_PI) res = m;
        else if (r >= 0.0f) res = m - EX_TWOPI;
        else res = m + EX_TWOPI;
    }
    return res;
}

s8 isValidFloat(const EXMaths::vec_xyzw &v)
{
    if (isValidFloat(v.get_x(), 0) && isValidFloat(v.get_y(), 0) && isValidFloat(v.get_z(), 0) && isValidFloat(v.get_w(), 0)) return 1;
    return 0;
}

s8 isValidFloat(const EXVector &v)
{
    return isValidFloat((const EXMaths::vec_xyzw &)v);
}

s8 isValidFloat(const EXMaths::mat_44 &m)
{
    if (isValidFloat(m.r[0]) && isValidFloat(m.r[1]) && isValidFloat(m.r[2]) && isValidFloat(m.r[3])) return 1;
    return 0;
}

s8 isValidFloat(const EXMatrix &m)
{
    if (isValidFloat(m.r[0]) && isValidFloat(m.r[1]) && isValidFloat(m.r[2]) && isValidFloat(m.r[3])) return 1;
    return 0;
}

s8 isValidFloat(const EXScaleMatrix &m)
{
    if (isValidFloat(m.r[0]) && isValidFloat(m.r[1]) && isValidFloat(m.r[2]) && isValidFloat(m.r[3])) return 1;
    return 0;
}

s8 isValidFloat(const EXQuaternion &q)
{
    if (isValidFloat(q.get_x(), 0) && isValidFloat(q.get_y(), 0) && isValidFloat(q.get_z(), 0) && isValidFloat(q.get_w(), 0)) return 1;
    return 0;
}
