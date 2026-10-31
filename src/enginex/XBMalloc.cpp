#include "EXMemHeap.h"
#include "EXHeapSizes.h"

// Xbox kernel API (declared here; no XDK headers).
extern "C" void *__stdcall XPhysicalAlloc(u32 size, u32 address, u32 alignment, u32 protect);
extern "C" void __stdcall XPhysicalFree(void *p);
extern "C" void *__cdecl memset(void *, int, unsigned int);

u32 _get_max_heap_size()
{
    return 0x4000000;
}

void _initalize_heap(EXMemHeap &heap, EXMheap id, u8 **ppMem, u32 size, const char *pName)
{
    *ppMem = (u8 *)XPhysicalAlloc(size, 0xffffffff, 0, 4);
    heap.Init(*ppMem, size, id, pName);
}

void _free_heap(EXMemHeap &heap)
{
    u32 id = heap.HeapId();
    if (_SystemHeap == _SystemHeapList[id]) {
        _SystemHeap = 0;
        for (int i = 0; i < 4; i++) {
            if (_SystemHeapList[i]) {
                _SystemHeap = _SystemHeapList[i];
                break;
            }
        }
    }
    _SystemHeapList[id] = 0;
    XPhysicalFree(heap.MemAlloc());
}

extern "C" void _initalize_heap_memory()
{
    EXHeapSizes sizes(0x4000000, _SDataHeapSize, _LDataHeapSize);
    _adjust_heap_sizes(sizes);
    memset(_SystemHeapList, 0, sizeof(_SystemHeapList));
    _initalize_heap(_SDataHeap, EXMHEAP_DEFAULT, &_SDataHeapMemory, sizes.Heap0Size(), "Default Heap");
    _initalize_heap(_LDataHeap, EXMHEAP_LARGEDATA, &_LDataHeapMemory, sizes.Heap1Size(), "Large Data Heap");
    _SystemHeap = _SystemHeapList[0];
}

extern "C" void _free_heap_memory()
{
    for (int i = 3; i >= 0; i--) {
        if (_SystemHeapList[i]) _free_heap(*_SystemHeapList[i]);
    }
}
