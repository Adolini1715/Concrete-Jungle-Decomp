#ifndef EXBASESTATS_H
#define EXBASESTATS_H
#include "types.h"

// Platform-independent frame statistics (sizeof 0x18).
class EXBaseStats {
public:
    virtual ~EXBaseStats();
    EXBaseStats();
    u32 FrameCount() const { return m_FrameCount; }
    void FrameUpdate(s8 paused);
protected:
    // 0x04: padding (MSVC aligns the first member after the vptr to the class's 8-byte alignment)
    u32 m_FrameCount; // 0x08
    u32 m_Unk0C;      // 0x0c TODO
    u64 m_GameTime;   // 0x10 advances 0x10aaaa per frame
};
#endif
