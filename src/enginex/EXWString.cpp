#include "EXWString.h"
#include "EXArray.h"

inline void *__cdecl operator new(unsigned int, void *p) { return p; }

inline int tolower(u16 c) { if (c >= 'A' && c <= 'Z') return c + 0x20; return c; }
inline int toupper(u16 c) { if (c >= 'a' && c <= 'z') return c - 0x20; return c; }
inline s8 isdigit(u16 c) { if (c >= '0' && c <= '9') return 1; return 0; }
inline int atoi(const u16 *s)
{
    int n = 0;
    while (*s && isdigit(*s)) {
        n = n * 10 + *s - '0';
        s++;
    }
    return n;
}

u16 EXWString::Length() const
{
    if (m_pchData) return GetData()->nDataLength;
    return 0;
}

u16 EXWString::Capacity() const
{
    if (m_pchData) return GetData()->nAllocLength;
    return 0;
}

s8 EXWString::IsEmpty() const
{
    if (m_pchData) return GetData()->nDataLength == 0;
    return 1;
}

void EXWString::ReleaseBuffer()
{
    u16 *p = m_pchData;
    if (p) TextPtrToHdrPtr(p)->nDataLength = Wcslen(p);
}

int EXWString::Find(u16 ch, int nStart) const
{
    if (m_pchData && nStart >= 0) {
        u16 n = GetData()->nDataLength;
        if (nStart < n) {
            for (int i = nStart; i < n; i++)
                if (m_pchData[i] == ch) return i;
        }
    }
    return -1;
}

s8 operator==(const EXWString &a, const EXWString &b)
{
    if (a.m_pchData == b.m_pchData) return 1;
    int n = a.Length();
    if (n != b.Length()) return 0;
    for (int i = 0; i < n; i++)
        if (a.m_pchData[i] != b.m_pchData[i]) return 0;
    return 1;
}

s8 operator!=(const EXWString &a, const EXWString &b) { return !(a == b); }

u16 EXWString::operator[](int i) const
{
    u16 n = Length();
    if (i > n) return 0;
    return m_pchData[i];
}

void EXWString::DeallocMem()
{
    if (m_pchData) {
        EXStringData *p = GetData();
        if (--p->nRefs == 0) EXFree(p);
    }
    m_pchData = 0;
}

int EXWString::IncRefCount()
{
    if (m_pchData) return ++GetData()->nRefs;
    return 0;
}

void EXWString::SetLength(u16 n)
{
    if (m_pchData) GetData()->nDataLength = n;
}

static void ConstructElements(EXWString *p, int nCount)
{
    for (; nCount; nCount--, p++) new (p) EXWString;
}

void EXWStringArray::FreeExtra()
{
    if (m_nSize != m_nMaxSize) {
        EXWString *pNewData = 0;
        if (m_nSize != 0) _EXRealloc(m_pData, m_nSize * sizeof(EXWString));
        else EXFree(m_pData);
        m_pData = pNewData;
        m_nMaxSize = m_nSize;
    }
}

void EXWStringArray::SetAt(int i, const EXWString &s) { m_pData[i] = s; }

void EXWString::Empty() { DeallocMem(); }

s8 EXWString::Compare(const EXWString &s) const
{
    if (m_pchData == s.m_pchData) return 0;
    u16 n1 = Length();
    u16 n2 = s.Length();
    u16 n = min(n1, n2);
    for (u16 i = 0; i < n; i++) {
        if (m_pchData[i] < s.m_pchData[i]) return -1;
        if (m_pchData[i] > s.m_pchData[i]) return 1;
    }
    if (n1 > n2) return 1;
    if (n1 < n2) return -1;
    return 0;
}

s8 EXWString::CompareNoCase(const EXWString &s) const
{
    if (m_pchData == s.m_pchData) return 0;
    u16 n1 = Length();
    u16 n2 = s.Length();
    u16 n = min(n1, n2);
    for (u16 i = 0; i < n; i++) {
        u16 a = m_pchData[i];
        if (a >= 'A' && a <= 'Z') a += 0x20;
        u16 b = s.m_pchData[i];
        if (b >= 'A' && b <= 'Z') b += 0x20;
        if (a < b) return -1;
        if (a > b) return 1;
    }
    if (n1 > n2) return 1;
    if (n1 < n2) return -1;
    return 0;
}

EXWString &EXWString::operator=(const EXWString &s)
{
    if (&s != this) {
        DeallocMem();
        m_pchData = s.m_pchData;
        if (m_pchData) GetData()->nRefs++;
    }
    return *this;
}

void EXWString::AllocMem(u16 nLength, s8 bExact)
{
    u16 n = nLength + 1;
    if (m_pchData) {
        if (n > GetData()->nAllocLength) DeallocMem();
        else {
            GetData()->nDataLength = nLength;
            return;
        }
    }
    n = n * 2 + sizeof(EXStringData);
    if (!bExact) {
        if (n < 0x20) n = 0x20;
        else if (n < 0x40) n = 0x40;
        else if (n < 0x80) n = 0x80;
        else if (n < 0x100) n = 0x100;
        else if (n < 0x200) n = 0x200;
    }
    EXStringData *p = AllocPtrToHdrPtr(_EXAlloc(n, 0));
    p->nRefs = 1;
    p->nDataLength = nLength;
    p->nAllocLength = n - sizeof(EXStringData);
    m_pchData = AllocPtrToTextPtr(p);
}

void EXWString::ReallocMem(u16 nLength)
{
    u16 *pch = m_pchData;
    if (pch) {
        EXStringData *pOld = TextPtrToHdrPtr(pch);
        if (pOld->nRefs <= 1 && nLength < pOld->nAllocLength) {
            pOld->nDataLength = nLength;
            return;
        }
        u16 n = nLength + 1;
        int nn = n;
        if (nn < 0x18) n = 0x18;
        else if (nn < 0x38) n = 0x38;
        else if (nn < 0x78) n = 0x78;
        else if (nn < 0xf8) n = 0xf8;
        else if (nn < 0x1f8) n = 0x1f8;
        EXStringData *p = AllocPtrToHdrPtr(_EXAlloc(n * 2 + sizeof(EXStringData), 0));
        memcpy(AllocPtrToTextPtr(p), m_pchData, pOld->nDataLength * 2 + 2);
        p->nRefs = 1;
        p->nDataLength = nLength;
        p->nAllocLength = n;
        DeallocMem();
        m_pchData = AllocPtrToTextPtr(p);
    } else
        AllocMem(nLength, 0);
}

void EXWString::MakeUnique()
{
    if (m_pchData && GetData()->nRefs > 1) {
        EXStringData *pOld = GetData();
        EXStringData *p = AllocPtrToHdrPtr(_EXAlloc(pOld->nAllocLength + sizeof(EXStringData), 0));
        memcpy(AllocPtrToTextPtr(p), m_pchData, pOld->nAllocLength * 2);
        p->nRefs = 1;
        p->nDataLength = pOld->nDataLength;
        p->nAllocLength = pOld->nAllocLength;
        pOld->nRefs--;
        m_pchData = AllocPtrToTextPtr(p);
    }
}

static void CopyElements(EXWString *pDest, const EXWString *pSrc, int nCount)
{
    while (nCount--) *pDest++ = *pSrc++;
}


EXWString::EXWString(const u16 *psz)
{
    m_pchData = 0;
    if (psz) {
        AllocMem(Wcslen(psz), 1);
        Wcscpy(m_pchData, psz);
    }
}

EXWString::EXWString(const u16 *psz, u16 nLength)
{
    m_pchData = 0;
    if (psz && *psz) {
        u16 nLen = Wcslen(psz);
        u16 n = min(nLength, nLen);
        AllocMem(n, 1);
        Wcsncpy(m_pchData, psz, n);
        m_pchData[n] = 0;
    }
}

EXWString::EXWString(const char *psz)
{
    m_pchData = 0;
    if (psz && *psz) {
        u16 n = strlen(psz);
        AllocMem(n, 1);
        u32 i;
        for (i = 0; i < n; i++) m_pchData[i] = (u8)psz[i];
        m_pchData[i] = 0;
    }
}

u16 *EXWString::GetBuffer(u16 nMinLength)
{
    u16 n = nMinLength;
    if (m_pchData) n = max(nMinLength, GetData()->nDataLength);
    ReallocMem(n);
    return m_pchData;
}

EXWString &EXWString::operator=(const u16 *psz)
{
    AllocMem(Wcslen(psz), 1);
    Wcscpy(m_pchData, psz);
    return *this;
}

// NEAR: 10 bytes differ, esi/edi swapped in the inlined AllocMem.
EXWString &EXWString::operator=(const char *psz)
{
    AllocMem(strlen(psz), 1);
    Wcscpy(m_pchData, (const u8 *)psz);
    return *this;
}

// sic: the three appending operators below copy to the start of the buffer.
const EXWString &EXWString::operator+=(const EXWString &s)
{
    ReallocMem(Length() + s.Length());
    Wcscpy(m_pchData, s.c_str());
    return *this;
}

const EXWString &EXWString::operator+=(const u16 *psz)
{
    if (psz) {
        u32 nAdd = Wcslen(psz);
        ReallocMem(nAdd + Length());
        Wcscpy(m_pchData, psz);
    }
    return *this;
}

// NEAR: 17 bytes differ, esi/edi swapped (this vs. length).
const EXWString &EXWString::operator+=(const char *psz)
{
    if (psz) {
        u32 nAdd = strlen(psz);
        u32 n = Length();
        ReallocMem(nAdd + n);
        if (n > 0) Wcscpy(m_pchData + n, (const u8 *)psz);
        else Wcscpy(m_pchData, (const u8 *)psz);
    }
    return *this;
}

const EXWString &EXWString::operator+=(char ch)
{
    u32 n = Length();
    ReallocMem(n + 1);
    m_pchData[n] = ch;
    m_pchData[n + 1] = 0;
    return *this;
}

EXWString operator+(const EXWString &a, const EXWString &b)
{
    EXWString s;
    u32 n = a.Length();
    s.AllocMem(b.Length() + n, 0);
    s.m_pchData[0] = 0;
    Wcscpy(s.m_pchData, a.m_pchData);
    Wcscpy(s.m_pchData + n, b.m_pchData);
    return s;
}

EXWString operator+(const u16 *a, const EXWString &b)
{
    EXWString s(a);
    s += b;
    return s;
}

EXWString operator+(const EXWString &a, const u16 *b)
{
    EXWString s(a);
    s += b;
    return s;
}

void EXWString::ToUpper()
{
    MakeUnique();
    u16 n = Length();
    for (u16 i = 0; i < n; i++) m_pchData[i] = toupper(m_pchData[i]);
}

void EXWString::ToLower()
{
    MakeUnique();
    u16 n = Length();
    for (u16 i = 0; i < n; i++) m_pchData[i] = tolower(m_pchData[i]);
}

// NEAR: register allocation (orig keeps pEnd in ebx and result in memory); same code as EXString::Token.
EXWString EXWString::Token(int n, u16 sep, s8 bMerge) const
{
    EXWString result;
    if (m_pchData) {
        const u16 *p = m_pchData;
        const u16 *pEnd = p + GetData()->nDataLength;
        int i = -1;
        while (i < n) {
            if (p >= pEnd) break;
            if (!bMerge) {
                while (*p == sep) {
                    if (p >= pEnd) goto next;
                    p++;
                }
            } else if (*p == sep)
                p++;
            if (p >= pEnd) continue;
            {
                i++;
                const u16 *pTok = p;
                while (*p != sep && p < pEnd) p++;
                if (i == n) result = EXWString(pTok, p - pTok);
            }
        next:;
        }
    }
    return result;
}

EXWString EXWString::Token(int n, const EXWString &seps, s8 bMerge) const
{
    EXWString result;
    if (m_pchData) {
        const u16 *p = m_pchData;
        const u16 *pEnd = p + GetData()->nDataLength;
        int i = -1;
        while (i < n) {
            if (p >= pEnd) break;
            if (!bMerge) {
                while (seps.Find(*p, 0) >= 0) {
                    if (p >= pEnd) goto next;
                    p++;
                }
            } else if (seps.Find(*p, 0) >= 0)
                p++;
            if (p >= pEnd) continue;
            {
                i++;
                const u16 *pTok = p;
                while (seps.Find(*p, 0) < 0 && p < pEnd) p++;
                if (i == n) result = EXWString(pTok, p - pTok);
            }
        next:;
        }
    }
    return result;
}

typedef char *EX_va_list;
#define EX_va_start(ap, v) (ap = (EX_va_list)&v + sizeof(v))
#define EX_va_arg(ap, t) (*(t *)((ap += (sizeof(t) + 3) & ~3) - ((sizeof(t) + 3) & ~3)))

// Estimates the formatted length (one pass over the format), sizes the buffer, then vsprintf's.
// NEAR: ~350 bytes differ, register choice only (max/min results land in eax where the original uses
// ecx, shifting the later nMaxLen adds); the narrow EXString::Format with the same source matches.
short __cdecl EXWString::Format(const u16 *fmt, ...)
{
    EX_va_list args;
    EX_va_start(args, fmt);
    int nMaxLen = 0;
    for (const u16 *p = fmt; *p; p++) {
        if (*p != '%' || *++p == '%') {
            nMaxLen++;
            continue;
        }
        int nItemLen = 0;
        int nWidth = 0;
        for (; *p; p++) {
            if (*p == '#') nMaxLen += 2;
            else if (*p == '*') nWidth = EX_va_arg(args, int);
            else if (*p == '-' || *p == '+' || *p == '0' || *p == ' ') ;
            else break;
        }
        if (nWidth == 0) {
            nWidth = atoi(p);
            for (; *p && isdigit(*p); p++) ;
        }
        int nPrecision = 0;
        if (*p == '.') {
            p++;
            if (*p == '*') {
                nPrecision = EX_va_arg(args, int);
                p++;
            } else {
                nPrecision = atoi(p);
                for (; *p && isdigit(*p); p++) ;
            }
        }
        switch (*p) {
        case 'h':
        case 'l':
        case 'F':
        case 'N':
        case 'L':
            p++;
            break;
        }
        switch (*p) {
        case 'c':
        case 'C':
            nItemLen = 2;
            EX_va_arg(args, int);
            break;
        case 's':
        case 'S': {
            nItemLen = Wcslen((EX_va_arg(args, const u16 *)));
            nItemLen = max(1, nItemLen);
            break;
        }
        }
        if (nItemLen != 0) {
            nItemLen = max(nItemLen, nWidth);
            if (nPrecision != 0) nItemLen = min(nItemLen, nPrecision);
        } else {
            switch (*p) {
            case 'd':
            case 'i':
            case 'u':
            case 'x':
            case 'X':
            case 'o':
                EX_va_arg(args, int);
                nItemLen = 32;
                nItemLen = max(nItemLen, nWidth + nPrecision);
                break;
            case 'e':
            case 'f':
            case 'g':
            case 'G':
                EX_va_arg(args, double);
                nItemLen = 128;
                nItemLen = max(nItemLen, nWidth + nPrecision);
                break;
            case 'p':
                EX_va_arg(args, void *);
                nItemLen = 32;
                nItemLen = max(nItemLen, nWidth + nPrecision);
                break;
            case 'n':
                EX_va_arg(args, int *);
                break;
            }
        }
        nMaxLen += nItemLen;
    }
    EX_va_list args2;
    EX_va_start(args2, fmt);
    short n = WVsprintf(GetBuffer(nMaxLen + 1), fmt, args2);
    ReleaseBuffer();
    return n;
}

// NEAR: 11 bytes differ, register choice for the separator string pointer.
int EXWString::Tokenise(EXWStringArray &out, const EXWString &seps, s8 bMerge) const
{
    EXFixedArray<u16, 256> buf;
    if (!m_pchData) return 0;
    const u16 *pEnd = m_pchData + GetData()->nDataLength;
    int nStart = out.Size();
const u16 *p = m_pchData;
    while (p < pEnd) {
        if (!bMerge) {
            while (seps.Find(*p, 0) >= 0) {
                if (p >= pEnd) goto done;
                p++;
            }
        } else if (seps.Find(*p, 0) >= 0) {
            if (p >= pEnd) goto done;
            p++;
        }
        if (p >= pEnd) break;
        {
            const u16 *pTok = p;
            while (seps.Find(*p, 0) < 0 && p < pEnd) p++;
            int nLen = p - pTok;
            int nMax = 0xff;
            nLen = min(nMax, nLen);
            memcpy(buf.GetData(), pTok, nLen * 2);
            buf[nLen] = 0;
            EXWString tok(buf);
            out.Add(tok);
        }
    }
done:
    return out.m_nSize - nStart;
}

// ---- EXWStringArray

static void DestructElements(EXWString *p, int nCount)
{
    for (; nCount; nCount--, p++) p->~EXWString();
}

void EXWStringArray::SetGrowBy(int nGrowBy)
{
    int nMin = 8;
    m_nGrowBy = max(nGrowBy, nMin);
}

void EXWStringArray::RemoveAll()
{
    if (m_pData) {
        DestructElements(m_pData, m_nSize);
        EXFree(m_pData);
        m_pData = 0;
    }
    m_nMaxSize = 0;
    m_nSize = 0;
}

void EXWStringArray::RemoveAt(int i, int nCount)
{
    int nMove = m_nSize - i - nCount;
    DestructElements(&m_pData[i], nCount);
    if (nMove) memmove(&m_pData[i], &m_pData[i + nCount], nMove * sizeof(EXWString));
    m_nSize -= nCount;
}

EXWStringArray::~EXWStringArray()
{
    if (m_pData) {
        DestructElements(m_pData, m_nSize);
        EXFree(m_pData);
        m_pData = 0;
    }
    m_nMaxSize = 0;
    m_nSize = 0;
}

void EXWStringArray::SetSize(int nNewSize, int nGrowBy)
{
    if (nGrowBy != -1) m_nGrowBy = nGrowBy;
    if (nNewSize == 0) {
        if (m_pData) {
            DestructElements(m_pData, m_nSize);
            EXFree(m_pData);
            m_pData = 0;
        }
        m_nMaxSize = 0;
        m_nSize = 0;
    } else if (m_pData == 0) {
        m_pData = (EXWString *)_EXAlloc(nNewSize * sizeof(EXWString), 0);
        ConstructElements(m_pData, nNewSize);
        m_nMaxSize = nNewSize;
        m_nSize = nNewSize;
    } else if (nNewSize <= m_nMaxSize) {
        if (nNewSize > m_nSize) ConstructElements(&m_pData[m_nSize], nNewSize - m_nSize);
        else if (m_nSize > nNewSize) DestructElements(&m_pData[nNewSize], m_nSize - nNewSize);
        m_nSize = nNewSize;
    } else {
        int nGrow = m_nGrowBy;
        if (nGrow == 0) {
            nGrow = m_nSize / 8;
            nGrow = (nGrow < 4) ? 4 : ((nGrow > 1024) ? 1024 : nGrow);
        }
        int nNewMax = (nNewSize < m_nMaxSize + nGrow) ? m_nMaxSize + nGrow : nNewSize;
        m_pData = (EXWString *)_EXRealloc(m_pData, nNewMax * sizeof(EXWString));
        ConstructElements(&m_pData[m_nSize], nNewSize - m_nSize);
        m_nMaxSize = nNewMax;
        m_nSize = nNewSize;
    }
}

void EXWStringArray::SetAtGrow(int i, const EXWString &s)
{
    if (i >= m_nSize) SetSize(i + 1, -1);
    m_pData[i] = s;
}

int EXWStringArray::Add(const EXWString &s)
{
    int i = m_nSize;
    SetAtGrow(i, s);
    return i;
}

int EXWStringArray::Append(const EXWStringArray &a)
{
    int nOld = m_nSize;
    SetSize(m_nSize + a.m_nSize, -1);
    CopyElements(m_pData + nOld, a.m_pData, a.m_nSize);
    return nOld;
}

void EXWStringArray::Copy(const EXWStringArray &a)
{
    SetSize(a.m_nSize, -1);
    CopyElements(m_pData, a.m_pData, a.m_nSize);
}

void EXWStringArray::InsertAt(int i, const EXWString &s, int nCount)
{
    if (i >= m_nSize) SetSize(i + nCount, -1);
    else {
        int nOld = m_nSize;
        SetSize(m_nSize + nCount, -1);
        DestructElements(&m_pData[nOld], nCount);
        memmove(&m_pData[i + nCount], &m_pData[i], (nOld - i) * sizeof(EXWString));
        ConstructElements(&m_pData[i], nCount);
    }
    while (nCount--) m_pData[i++] = s;
}

void EXWStringArray::InsertAt(int i, const EXWStringArray &a)
{
    if (a.m_nSize > 0) {
        InsertAt(i, a.m_pData[0], a.m_nSize);
        for (int j = 0; j < a.m_nSize; j++) SetAt(i + j, a.m_pData[j]);
    }
}

EXWStringArray::EXWStringArray(int nSize, int nGrowBy)
{
    int nMin = 8;
    m_nMaxSize = 0;
    m_nSize = 0;
    m_pData = 0;
    m_nGrowBy = max(nGrowBy, nMin);
    if (nSize > 0) SetSize(nSize, -1);
}
