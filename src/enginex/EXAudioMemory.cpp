#include "EXAudioMemory.h"

EXAudioMemory::EXAudioMemory(u32 base, u32 size)
{
    m_Size = size;
    size >>= 10;
    for (int i = 1; i < NUM_SLOTS; i++) m_Slots[i].m_State = SLOT_UNUSED;
    m_Slots[0].m_Size = size;
    m_Base = base;
    m_Slots[0].m_State = SLOT_FREE;
    m_Slots[0].m_Start = 0;
}

EXAudioMemory::~EXAudioMemory() {}

int EXAudioMemory::GetUnusedSlot()
{
    for (int i = 0; i < NUM_SLOTS; i++) {
        if (m_Slots[i].m_State == SLOT_UNUSED) return i;
    }
    return -1;
}

int EXAudioMemory::Alloc(int size)
{
    int kb = (size + 0x3ff) / 1024;
    for (int i = 0; i < NUM_SLOTS; i++) {
        if (m_Slots[i].m_State == SLOT_FREE && m_Slots[i].m_Size >= kb) {
            if (m_Slots[i].m_Size != kb) {
                int j = GetUnusedSlot();
                m_Slots[j].m_State = SLOT_FREE;
                m_Slots[j].m_Start = m_Slots[i].m_Start + kb;
                m_Slots[j].m_Size = m_Slots[i].m_Size - kb;
                m_Slots[i].m_Size = kb;
            }
            m_Slots[i].m_State = SLOT_USED;
            return (m_Slots[i].m_Start << 10) + m_Base;
        }
    }
    return 0;
}
void EXAudioMemory::Compact()
{
    for (int i = 0; i < NUM_SLOTS; i++) {
        if (m_Slots[i].m_State == SLOT_FREE) {
            for (int j = 0; j < NUM_SLOTS; j++) {
                if (m_Slots[j].m_State == SLOT_FREE && m_Slots[i].m_Start + m_Slots[i].m_Size == m_Slots[j].m_Start) {
                    m_Slots[i].m_Size += m_Slots[j].m_Size;
                    m_Slots[j].m_State = SLOT_UNUSED;
                }
            }
        }
    }
}

void EXAudioMemory::DeAlloc(int addr)
{
    int kb = (int)(addr - m_Base) / 1024;
    for (int i = 0; i < NUM_SLOTS; i++) {
        if (m_Slots[i].m_State == SLOT_USED && m_Slots[i].m_Start == kb) {
            m_Slots[i].m_State = SLOT_FREE;
            Compact();
            return;
        }
    }
}

u32 EXAudioMemory::GetAllocSize()
{
    u32 total = 0;
    for (int i = 0; i < NUM_SLOTS; i++) {
        if (m_Slots[i].m_State == SLOT_USED) total += m_Slots[i].m_Size << 10;
    }
    return total;
}
