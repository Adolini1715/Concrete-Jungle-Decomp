// Basic Eurocom-style integer types (names ours; the mangled names fix the underlying types).
#ifndef TYPES_H
#define TYPES_H
typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
typedef __int64 s64;
typedef unsigned __int64 u64;
typedef float f32;
#ifndef NULL
#define NULL 0
#endif
#endif
