#include "EXMaths.h"

#include "EXWnd.h"
#include "EXBaseDisplay.h"

namespace EXMaths {

void vec_xyzw::Print(const char *name) const
{
    EXWnd *pWnd = EXBaseDisplay::ActiveWnd();
    if (pWnd) {
        if (name) pWnd->DbgPrint("%s: \n", name);
        pWnd->DbgPrint("(%f %f %f %f)\n", x, y, z, w);
    }
}

void vec_xyzw::Trace(const char *name) const {}

// NEAR: ~120 bytes differ, the inlined min3/max3 select shape (see EXStdFuncs.h) and frame slot reuse.
// Point-in-triangle in the XZ plane (crossing count of a ray along +x).
s8 vec_xyzw::PointInTriY(const vec_xyzw *const t) const
{
    float x0 = t[0].x - x, x1 = t[1].x - x, x2 = t[2].x - x;
    if (min3(x0, x1, x2) > 0.0f) return 0;
    if (0.0f > max3(x0, x1, x2)) return 0;
    float z0 = t[0].z - z, z1 = t[1].z - z, z2 = t[2].z - z;
    if (min3(z0, z1, z2) > 0.0f) return 0;
    if (0.0f > max3(z0, z1, z2)) return 0;
    int n = 0;
    if ((0.0f > z0 && z1 >= 0.0f) || (z0 >= 0.0f && 0.0f > z1)) {
        if (x0 >= 0.0f && x1 >= 0.0f) n++;
        else if (x0 >= 0.0f || x1 >= 0.0f) {
            float d = z1 - z0;
            if (d != 0.0f && x0 - (x1 - x0) / d * z0 > 0.0f) n++;
        }
    }
    if ((0.0f > z1 && z2 >= 0.0f) || (z1 >= 0.0f && 0.0f > z2)) {
        if (x1 >= 0.0f && x2 >= 0.0f) n++;
        else if (x1 >= 0.0f || x2 >= 0.0f) {
            float d = z2 - z1;
            if (d != 0.0f && x1 - (x2 - x1) / d * z1 > 0.0f) n++;
        }
    }
    if ((0.0f > z2 && z0 >= 0.0f) || (z2 >= 0.0f && 0.0f > z0)) {
        if (x2 >= 0.0f && x0 >= 0.0f) n++;
        else if (x2 >= 0.0f || x0 >= 0.0f) {
            float d = z0 - z2;
            if (d != 0.0f && x2 - (x0 - x2) / d * z2 > 0.0f) n++;
        }
    }
    return n & 1;
}

} // namespace EXMaths
