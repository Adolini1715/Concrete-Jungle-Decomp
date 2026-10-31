#include "EXObject.h"
#include "EXStdFuncs.h"

EX_IMPLEMENT_DYNAMIC_ROOT(EXObject)

s8 EXIsValidAddress(const void *p) { return 1; }

void EXAssertValidObject(const EXObject *p)
{
    if (!p)
        for (;;) {}
}

u32 Wcslen(const u16 *s)
{
    const u16 *start = s;
    while (*s++) {}
    return s - (start + 1);
}

u16 *Wcscpy(u16 *d, const u16 *s)
{
    u32 i = 0;
    u16 c;
    do {
        c = s[i];
        d[i] = c;
        i++;
    } while (c);
    return d;
}

u16 *Wcscpy(u16 *d, const u8 *s)
{
    u32 i;
    for (i = 0; s[i]; i++) d[i] = s[i];
    d[i] = 0;
    return d;
}

u16 *Wcsncpy(u16 *d, const u16 *s, u32 n)
{
    for (u32 i = 0; i < n; i++) d[i] = s[i];
    return d;
}

u16 *Wcsncpy(u16 *d, const u8 *s, u32 n)
{
    for (u32 i = 0; i < n; i++) d[i] = s[i];
    return d;
}

u16 *Wcschr(u16 *s, u16 c)
{
    for (u32 i = 0; s[i]; i++)
        if (s[i] == c) return &s[i];
    return 0;
}

u16 *Wcsstr(u16 *s, const u16 *sub)
{
    if (!s) return 0;
    if (!sub) return 0;
    u32 n = Wcslen(s);
    u32 m = Wcslen(sub);
    for (u32 i = 0; i < n; i++) {
        if (s[i] == sub[0]) {
            if (i + m > n) return 0;
            s8 bMatch = 1;
            for (u32 j = 0; j < m; j++) {
                if (s[i + j] != sub[j]) {
                    bMatch = 0;
                    break;
                }
            }
            if (bMatch) return &s[i];
        }
    }
    return 0;
}

u32 WriteWChar(u16 *d, u16 c, u32 n)
{
    d[n] = c;
    return n + 1;
}

u32 Write_Multi_Char(u16 *d, u16 c, u32 n, u32 pos)
{
    for (u32 i = 0; i < n; i++) d[pos++] = c;
    return pos;
}

u32 Write_String(u16 *d, const u16 *s, u32 n, u32 pos)
{
    for (u32 i = 0; i < n; i++) d[pos++] = s[i];
    return pos;
}

int Wcscmp(const u16 *a, const u16 *b)
{
    u16 n1 = Wcslen(a);
    u16 n2 = Wcslen(b);
    u16 n = min(n1, n2);
    for (u16 i = 0; i < n; i++) {
        if (a[i] < b[i]) return -1;
        if (a[i] > b[i]) return 1;
    }
    if (n1 > n2) return 1;
    if (n1 < n2) return -1;
    return 0;
}

u32 Wsprintf(u16 *d, const u16 *fmt, ...)
{
    u32 n = WOutput(d, fmt, (char *)(&fmt + 1));
    d[n] = 0;
    return n;
}

u32 WVsprintf(u16 *d, const u16 *fmt, char *args)
{
    u32 n = WOutput(d, fmt, args);
    d[n] = 0;
    return n;
}
