#ifndef EXSTRING_H
#define EXSTRING_H
#include "types.h"
#include "EXMemory.h"
#include "EXStdFuncs.h"

class EXStringArray;
void *_EXRealloc(void *p, u32 size);  // map signature (EXMemory.h has a 3-arg form)

// Header in front of every string buffer; m_pchData points just past it.
struct EXStringData {
    u32 nRefs;
    u16 nDataLength;
    u16 nAllocLength;
    char *data() { return (char *)(this + 1); }
};

// Reference-counted string (one pointer, MFC CString style). A null pointer is the empty string.
class EXString {
public:
    EXString() { m_pchData = 0; }
    EXString(const EXString &s) { m_pchData = s.m_pchData; IncRefCount(); }
    EXString(const char *psz);
    EXString(const char *psz, u16 nLength);
    ~EXString() { DeallocMem(); }

    const char *c_str() const { return m_pchData ? m_pchData : ""; }
    const char *data() const { return m_pchData ? m_pchData : ""; }
    operator const char *() const { return m_pchData ? m_pchData : ""; }
    const char *RawData() const { return m_pchData; }  // no empty-string substitution (XBMemCard.obj)
    u16 Length() const;
    u16 Capacity() const;
    s8 IsEmpty() const;
    void Empty();
    char *GetBuffer(u16 nMinLength);
    void ReleaseBuffer();
    int Find(char ch, int nStart) const;
    void MakeUnique();
    s8 Compare(const EXString &s) const;
    s8 CompareNoCase(const EXString &s) const;
    EXString &operator=(const EXString &s);
    EXString &operator=(const char *psz);
    const EXString &operator+=(const EXString &s);
    const EXString &operator+=(const char *psz);
    const EXString &operator+=(char ch);
    void ToUpper();
    void ToLower();
    EXString Token(int n, char sep, s8 bMerge) const;
    EXString Token(int n, const EXString &seps, s8 bMerge) const;
    short __cdecl Format(const char *fmt, ...);
    int Tokenise(EXStringArray &out, const EXString &seps, s8 bMerge) const;

    friend EXString operator+(const EXString &a, const EXString &b);
    friend EXString operator+(const char *a, const EXString &b);
    friend EXString operator+(const EXString &a, const char *b);
protected:
    EXStringData *GetData() const { return (EXStringData *)m_pchData - 1; }
    void DeallocMem();
    int IncRefCount();
    void SetLength(u16 n);
    void AllocMem(u16 nLength, s8 bExact);
    void ReallocMem(u16 nLength);
public:
    char *m_pchData;
};

s8 operator==(const EXString &a, const EXString &b);
s8 operator!=(const EXString &a, const EXString &b);

// Array of strings with MFC CStringArray layout {max, growBy, size, data}.
class EXStringArray {
public:
    EXStringArray(int nSize, int nGrowBy);
    ~EXStringArray();
    int Size() const { return m_nSize; }
    EXString &operator[](int i) { return m_pData[i]; }
    const EXString &operator[](int i) const { return m_pData[i]; }
    const EXString &GetAt(int i) const { return m_pData[i]; }
    void SetSize(int nNewSize, int nGrowBy);
    void SetGrowBy(int nGrowBy);
    void FreeExtra();
    void RemoveAll();
    void SetAt(int i, const EXString &s);
    void SetAtGrow(int i, const EXString &s);
    int Add(const EXString &s);
    int Append(const EXStringArray &a);
    void Copy(const EXStringArray &a);
    void InsertAt(int i, const EXString &s, int nCount);
    void InsertAt(int i, const EXStringArray &a);
    void RemoveAt(int i, int nCount);
    int m_nMaxSize;
    int m_nGrowBy;
    int m_nSize;
    EXString *m_pData;
};
#endif
