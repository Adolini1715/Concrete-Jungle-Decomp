#ifndef EXHEAPSIZES_H
#define EXHEAPSIZES_H
#include "types.h"

// Heap size request passed to the game's _adjust_heap_sizes() at boot.
class EXHeapSizes {
public:
    EXHeapSizes(u32 maxSize, u32 heap0, u32 heap1) { SetMaxHeapSize(maxSize); SetHeap0Size(heap0); SetHeap1Size(heap1); }
    void SetHeap0Size(u32 s) { m_Heap0Size = s; }
    void SetHeap1Size(u32 s) { m_Heap1Size = s; }
    u32 Heap0Size() const { return m_Heap0Size; }
    u32 Heap1Size() const { return m_Heap1Size; }
private:
    void SetMaxHeapSize(u32 s) { m_MaxHeapSize = s; }
    u32 m_MaxHeapSize;
    u32 m_Heap0Size;
    u32 m_Heap1Size;
};

void _adjust_heap_sizes(EXHeapSizes &sizes);
extern u32 _SDataHeapSize;
extern u32 _LDataHeapSize;
#endif
