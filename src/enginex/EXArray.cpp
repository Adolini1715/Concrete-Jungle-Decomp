#include "EXArray.h"

inline void *EXmemchr(const void *p, int c, u32 n) { return memchr(p, c, n); }

void EXArray_ValidateCopy(void *dst, const void *src, u32 n, void *base, u32 size) {}
void EXArray_ValidateFill(void *dst, u32 n, void *base, u32 size) {}

u32 EXArray_strlen(const char *s, void *base, u32 size) { return strlen(s); }

void *EXArray_memcpy(void *dst, const void *src, u32 n, void *base, u32 size)
{
    EXArray_ValidateCopy(dst, src, n, base, size);
    return memcpy(dst, src, n);
}

void *EXArray_memmove(void *dst, const void *src, u32 n, void *base, u32 size)
{
    EXArray_ValidateCopy(dst, src, n, base, size);
    return memmove(dst, src, n);
}

void *EXArray_memset(void *dst, int c, u32 n, void *base, u32 size)
{
    EXArray_ValidateFill(dst, n, base, size);
    return memset(dst, c, n);
}

void *EXArray_memchr(void *p, int c, u32 n, void *base, u32 size) { return EXmemchr(p, c, n); }

char *EXArray_strcpy(char *dst, const char *src, void *base, u32 size)
{
    memcpy(dst, src, strlen(src) + 1);
    return dst;
}

char *EXArray_strcat(char *dst, const char *src, void *base, u32 size)
{
    dst += strlen(dst);
    u32 n = strlen(src) + 1;
    memcpy(dst, src, n);
    return dst;
}

int EXPtrArray_Find(const EXPtrArray<void> *a, const void *p)
{
    const void **data = a->GetData();
    for (int i = 0; i < a->Size(); i++)
        if (data[i] == p) return i;
    return -1;
}

int EXPtrArray_AddUnique(EXPtrArray<void> *a, void *p)
{
    int nIndex = a->Size();
    void **data = a->GetData();
    for (int i = 0; i < a->Size(); i++)
        if (data[i] == p) return i;
    a->Add(p);
    return nIndex - 1;  // sic: the original returns the old count minus one
}
