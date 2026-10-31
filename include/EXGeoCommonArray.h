#ifndef EXGEOCOMMONARRAY_H
#define EXGEOCOMMONARRAY_H
#include "types.h"
#include "EXRelPtr.h"

class EXGeoHeader;
class EXGeoObjectArray;
class EXGeoObjectHeader;

// Debug-only value (index into the debug file's tables).
template <class T> class EXGeoDebug {
public:
    void operator=(T v) { m_Value = v; }
    operator unsigned int() const { return m_Value; }
    T m_Value;
};

// A "hash" whose top byte is clear (or top bit set) is a direct index in the low 24 bits.
inline int EXGeoHashIndex(u32 hash)
{
    if (!(hash & 0xff000000) | (hash >> 31)) return hash & 0xffffff;
    return -1;
}

// Non-template workers (EXGeoCommon.obj); the template passes its element size.
int EXGeoCommonArray_FindIndex(u32 hash, void *pArray, u32 size);
int EXGeoCommonArray_FindIndex2(u32 hash, void *pArray, u32 size);
void EXGeoCommonArray_UpdatePtrs(EXGeoHeader *pHdr, const EXGeoObjectArray *pArray, int size, int nAddr);
void EXGeoCommonArray_SetPtrs(const EXGeoHeader *pHdr, const EXGeoObjectArray *pArray, int size);
void EXGeoCommonArray_OnDelete(EXGeoHeader *pHdr, const EXGeoObjectArray *pArray, int size, int section);
void EXGeoCommonArray_OnSectionDelete(EXGeoHeader *pHdr, const EXGeoObjectArray *pArray, int size);
void EXGeoCommonArray_DeletePtrs(const EXGeoHeader *pHdr, const EXGeoObjectArray *pArray, int size, int section);

// One of the header's object lists (8 bytes): element count, hash count, relptr to the elements.
// A positive hash count means the elements are sorted by hash; otherwise -count hashes sit
// just before the element data.
template <class T> class EXGeoCommonArray {
public:
    int Size() const { return m_Size; }
    int HashSize() const { return m_HashSize; }
    T *Data() const { return m_Data; }
    template <class I> const T &operator[](I i) const { return Data()[i]; }
    int FindIndex(u32 hash, u32 size) const { return EXGeoCommonArray_FindIndex(hash, (void *)this, size); }
    int FindIndex(u32 hash) const { return EXGeoCommonArray_FindIndex(hash, (void *)this, sizeof(T)); }
    T *Find(u32 hash) const
    {
        int i = EXGeoCommonArray_FindIndex(hash, (void *)this, sizeof(T));
        if (i < 0) return 0;
        return &Data()[i];
    }
    void UpdatePtrs(EXGeoHeader *pHdr, int nAddr) const
    {
        EXGeoCommonArray_UpdatePtrs(pHdr, (const EXGeoObjectArray *)this, sizeof(T), nAddr);
    }
    void OnDelete(EXGeoHeader *pHdr, int section) const
    {
        EXGeoCommonArray_OnDelete(pHdr, (const EXGeoObjectArray *)this, sizeof(T), section);
    }
    void OnSectionDelete(EXGeoHeader *pHdr) const
    {
        EXGeoCommonArray_OnSectionDelete(pHdr, (const EXGeoObjectArray *)this, sizeof(T));
    }
    void SetPtrs(const EXGeoHeader *pHdr) const
    {
        EXGeoCommonArray_SetPtrs(pHdr, (const EXGeoObjectArray *)this, sizeof(T));
    }
    void DeletePtrs(const EXGeoHeader *pHdr, int section) const
    {
        EXGeoCommonArray_DeletePtrs(pHdr, (const EXGeoObjectArray *)this, sizeof(T), section);
    }

    s16 m_Size;               // +0
    s16 m_HashSize;           // +2
    ex::EXRelPtr<T> m_Data;   // +4
};

// Common head of every list element: hash, {section:15, created:1}, then one or more
// {file offset, runtime pointer} pairs (the first pair is the object itself).
template <class T> class EXGeoCommonArrayElement {
public:
    struct EXGeoCommonAddr {
        ex::EXABSOFFSET32 m_Offset;     // offset in the file (or its section)
        T *m_pObject;                   // runtime pointer once fixed up
    };
    u32 HashCode() const { return m_HashCode; }
    u32 Section() const { return m_Flags & 0x7fff; }
    s8 ObjectCreated() const { return m_Flags >> 15; }
    void SetObjectCreated() { m_Flags |= 0x8000; }
    void ClrObjectCreated() { m_Flags &= 0x7fff; }
    EXGeoCommonAddr &Address(int i) { return m_Addr[i]; }
    T *Object(int i) const
    {
        if (!this) return 0;
        return m_Addr[i].m_pObject;
    }

    u32 m_HashCode;           // +0
    u16 m_Flags;              // +4 section (low 15 bits), created (bit 15)
    EXGeoDebug<u16> m_Debug;  // +6
    EXGeoCommonAddr m_Addr[1];  // +8
};
#endif
