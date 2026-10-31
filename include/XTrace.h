#ifndef XTRACE_H
#define XTRACE_H

// TODO owning header(s) unknown: game debug-trace macros, compiled out in retail (empty varargs inlines;
// [fi] copies in Actor.obj). COMDAT order puts the first five right after the XApp.h frame-rate
// helpers and the per-programmer four right after FEGetHud (FEHud.h). Arguments are still evaluated.
inline void __cdecl TRBUILDTRACE(...) {}
inline void __cdecl TRTRACE(...) {}
inline void __cdecl TRTRACE_RAW(...) {}
inline void __cdecl TRACE_MH(...) {}
inline void __cdecl VCLTRACE(...) {}
inline void __cdecl SW_TRACE(...) {}
inline void __cdecl KG_TRACE(...) {}
inline void __cdecl BUFFY_TRACE(...) {}
inline void __cdecl GSM_TRACE(...) {}
#endif
