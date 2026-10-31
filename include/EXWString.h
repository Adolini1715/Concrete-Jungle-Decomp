#ifndef EXWSTRING_H
#define EXWSTRING_H
#include "EXString.h"

class EXWStringArray;

inline EXStringData *AllocPtrToHdrPtr(void *p) { return (EXStringData *)p; }
inline u16 *AllocPtrToTextPtr(void *p) { return (u16 *)((u8 *)p + sizeof(EXStringData)); }
inline EXStringData *TextPtrToHdrPtr(void *p) { return (EXStringData *)((u8 *)p - sizeof(EXStringData)); }

// Reference-counted wide (u16) string; same header layout as EXString, lengths in characters.
class EXWString {
public:
    EXWString() { m_pchData = 0; }
    EXWString(const EXWString &s) { m_pchData = s.m_pchData; if (m_pchData) GetData()->nRefs++; }
    EXWString(const u16 *psz);
    EXWString(const u16 *psz, u16 nLength);
    EXWString(const char *psz);
    ~EXWString() { DeallocMem(); }

    const u16 *c_str() const { return m_pchData ? m_pchData : (const u16 *)""; }
    u16 Length() const;
    u16 Capacity() const;
    s8 IsEmpty() const;
    void Empty();
    u16 *GetBuffer(u16 nMinLength);
    void ReleaseBuffer();
    int Find(u16 ch, int nStart) const;
    u16 operator[](int i) const;
    void MakeUnique();
    s8 Compare(const EXWString &s) const;
    s8 CompareNoCase(const EXWString &s) const;
    EXWString &operator=(const EXWString &s);
    EXWString &operator=(const u16 *psz);
    EXWString &operator=(const char *psz);
    const EXWString &operator+=(const EXWString &s);
    const EXWString &operator+=(const u16 *psz);
    const EXWString &operator+=(const char *psz);
    const EXWString &operator+=(char ch);
    void ToUpper();
    void ToLower();
    EXWString Token(int n, u16 sep, s8 bMerge) const;
    EXWString Token(int n, const EXWString &seps, s8 bMerge) const;
    short __cdecl Format(const u16 *fmt, ...);
    int Tokenise(EXWStringArray &out, const EXWString &seps, s8 bMerge) const;

    friend EXWString operator+(const EXWString &a, const EXWString &b);
    friend EXWString operator+(const u16 *a, const EXWString &b);
    friend EXWString operator+(const EXWString &a, const u16 *b);
protected:
    EXStringData *GetData() const { return TextPtrToHdrPtr(m_pchData); }
    void DeallocMem();
    int IncRefCount();
    void SetLength(u16 n);
    void AllocMem(u16 nLength, s8 bExact);
    void ReallocMem(u16 nLength);
public:
    u16 *m_pchData;
};

s8 operator==(const EXWString &a, const EXWString &b);
s8 operator!=(const EXWString &a, const EXWString &b);

class EXWStringArray {
public:
    EXWStringArray(int nSize, int nGrowBy);
    ~EXWStringArray();
    int Size() const { return m_nSize; }
    EXWString &operator[](int i) { return m_pData[i]; }
    const EXWString &GetAt(int i) const { return m_pData[i]; }
    void SetSize(int nNewSize, int nGrowBy);
    void SetGrowBy(int nGrowBy);
    void FreeExtra();
    void RemoveAll();
    void SetAt(int i, const EXWString &s);
    void SetAtGrow(int i, const EXWString &s);
    int Add(const EXWString &s);
    int Append(const EXWStringArray &a);
    void Copy(const EXWStringArray &a);
    void InsertAt(int i, const EXWString &s, int nCount);
    void InsertAt(int i, const EXWStringArray &a);
    void RemoveAt(int i, int nCount);
    int m_nMaxSize;
    int m_nGrowBy;
    int m_nSize;
    EXWString *m_pData;
};
#endif
