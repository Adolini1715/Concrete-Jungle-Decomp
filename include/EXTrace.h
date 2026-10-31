#ifndef EXTRACE_H
#define EXTRACE_H

// Debug trace output, compiled out in retail (empty varargs inline; copy emitted in Actor.obj).
// Arguments are still evaluated.
namespace ex {
inline void __cdecl TRACE(...) {}
}
#endif
