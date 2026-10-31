#ifndef EXSTDFUNCS_H
#define EXSTDFUNCS_H
#include "types.h"

// STL-style helpers (emitted as min<float>, max<int>, min3<float>, ...).
template <class T> inline const T &min(const T &a, const T &b) { return a <= b ? a : b; }
template <class T> inline const T &max(const T &a, const T &b) { return a >= b ? a : b; }
// NEAR: min3/max3 bodies select with cmov where the original branches (XBVector.obj, EXDatum.obj).
template <class T> inline const T &min3(const T &a, const T &b, const T &c) { return a <= b ? min(a, c) : min(b, c); }
template <class T> inline const T &max3(const T &a, const T &b, const T &c) { return a >= b ? max(a, c) : max(b, c); }
// Clamp v to [lo, hi] (limit<float> emitted in ActorUtil.obj; shape from EXItemAnimator_DynLight::DoRender).
template <class T> inline T limit(const T &v, const T &lo, const T &hi)
{
    if (v >= hi) return hi;
    if (lo >= v) return lo;
    return v;
}
// The int instance compares v against lo the other way round (limit<int> in Player.obj, XBGeoEntity.obj).
template <> inline int limit<int>(const int &v, const int &lo, const int &hi)
{
    if (v >= hi) return hi;
    if (v <= lo) return lo;
    return v;
}
#endif

// C runtime entry points (our own prototypes; memcpy/memset/strlen/strcpy/strcat/strcmp expand inline).
#ifndef EXSTDFUNCS_CRT
#define EXSTDFUNCS_CRT
extern "C" {
void *__cdecl memcpy(void *, const void *, unsigned int);
void *__cdecl memset(void *, int, unsigned int);
void *__cdecl memmove(void *, const void *, unsigned int);
void *__cdecl memchr(const void *, int, unsigned int);
int __cdecl memcmp(const void *, const void *, unsigned int);
unsigned int __cdecl strlen(const char *);
char *__cdecl strcpy(char *, const char *);
char *__cdecl strcat(char *, const char *);
int __cdecl strcmp(const char *, const char *);
double __cdecl sqrt(double);
double __cdecl sin(double);
double __cdecl cos(double);
double __cdecl fabs(double);
double __cdecl atan2(double, double);
double __cdecl acos(double);
double __cdecl asin(double);
double __cdecl tan(double);
double __cdecl atan(double);
double __cdecl fmod(double, double);
}
#pragma intrinsic(memcpy, memset, memcmp, strlen, strcpy, strcat, strcmp, sqrt, sin, cos, fabs, atan2, acos, asin, tan, atan, fmod)
#endif

// Byte compare wrapper (emitted in EXItemRender_Anim.obj; appended by the item agent). Inlined
// callers keep the byte-wise repe cmpsb even for constant sizes.
#ifndef EXSTDFUNCS_MEMCMP
#define EXSTDFUNCS_MEMCMP
inline int __cdecl EXmemcmp(const void *a, const void *b, unsigned int n) { return memcmp(a, b, n); }
#endif

// Wide-character (u16) string helpers in EXStdFuncs.obj.
#ifndef EXSTDFUNCS_WIDE
#define EXSTDFUNCS_WIDE
u32 Wcslen(const u16 *s);
u16 *Wcscpy(u16 *d, const u16 *s);
u16 *Wcscpy(u16 *d, const u8 *s);
u16 *Wcsncpy(u16 *d, const u16 *s, u32 n);
u16 *Wcsncpy(u16 *d, const u8 *s, u32 n);
u16 *Wcschr(u16 *s, u16 c);
u16 *Wcsstr(u16 *s, const u16 *sub);
int Wcscmp(const u16 *a, const u16 *b);
u32 WriteWChar(u16 *d, u16 c, u32 n);
u32 Write_Multi_Char(u16 *d, u16 c, u32 n, u32 pos);
u32 Write_String(u16 *d, const u16 *s, u32 n, u32 pos);
u32 WOutput(u16 *d, const u16 *fmt, char *args);
u32 Wsprintf(u16 *d, const u16 *fmt, ...);
u32 WVsprintf(u16 *d, const u16 *fmt, char *args);
#endif
