#ifndef EXAUDIOMEMORY_H
#define EXAUDIOMEMORY_H
#include "types.h"

// First-fit allocator over a sound memory block, in 1 KB units, 20 slots. sizeof 0xf8.
class EXAudioMemory {
public:
    EXAudioMemory(u32 base, u32 size);
    ~EXAudioMemory();
    int Alloc(int size);
    void Compact();
    void DeAlloc(int addr);
    u32 GetAllocSize();
    u32 TotalSize() const { return m_Size; }  // added by the audio agent (XBFileManager::UpdateStats)
private:
    int GetUnusedSlot();
    enum { SLOT_UNUSED = 0, SLOT_FREE = 1, SLOT_USED = 2, NUM_SLOTS = 20 };
    struct Slot {
        int m_Start;  // KB offset from m_Base
        int m_Size;   // KB
        int m_State;
    };
    u32 m_Size;
    Slot m_Slots[NUM_SLOTS];
    u32 m_Base;
};

extern EXAudioMemory *gpEXAudioMemory;
#endif
