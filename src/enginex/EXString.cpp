#include "EXString.h"
#include "EXArray.h"

inline void *__cdecl operator new(unsigned int, void *p) { return p; }
extern "C" char *__cdecl strncpy(char *, const char *, unsigned int);

inline int tolower(char c) { if (c >= 'A' && c <= 'Z') return c + 0x20; return c; }
inline int toupper(char c) { if (c >= 'a' && c <= 'z') return c - 0x20; return c; }
inline char EXToLowerC(char c) { if (c >= 'A' && c <= 'Z') c += 0x20; return c; }
inline s8 isdigit(char c) { if (c >= '0' && c <= '9') return 1; return 0; }
inline void *EXmemmove(void *d, const void *s, u32 n) { return memmove(d, s, n); }

static int EXString_atoi(const char *s)
{
    int n = 0;
    while (*s && isdigit(*s)) {
        n = n * 10 + *s - '0';
        s++;
    }
    return n;
}

void EXString::DeallocMem()
{
    if (m_pchData) {
        EXStringData *p = GetData();
        if (--p->nRefs == 0) EXFree(p);
    }
    m_pchData = 0;
}

int EXString::IncRefCount()
{
    if (m_pchData) return ++GetData()->nRefs;
    return 0;
}

void EXString::SetLength(u16 n)
{
    if (m_pchData) GetData()->nDataLength = n;
}

u16 EXString::Length() const
{
    if (m_pchData) return GetData()->nDataLength;
    return 0;
}

u16 EXString::Capacity() const
{
    if (m_pchData) return GetData()->nAllocLength;
    return 0;
}

s8 EXString::IsEmpty() const
{
    if (m_pchData) return GetData()->nDataLength == 0;
    return 1;
}

void EXString::Empty() { DeallocMem(); }

void EXString::ReleaseBuffer()
{
    if (m_pchData) GetData()->nDataLength = strlen(m_pchData);
}

int EXString::Find(char ch, int nStart) const
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

EXString &EXString::operator=(const EXString &s)
{
    if (&s != this) {
        DeallocMem();
        m_pchData = s.m_pchData;
        if (m_pchData) GetData()->nRefs++;
    }
    return *this;
}

s8 operator==(const EXString &a, const EXString &b)
{
    if (a.m_pchData == b.m_pchData) return 1;
    int n = a.Length();
    if (n != b.Length()) return 0;
    for (int i = 0; i < n; i++)
        if (a.m_pchData[i] != b.m_pchData[i]) return 0;
    return 1;
}

s8 operator!=(const EXString &a, const EXString &b) { return !(a == b); }

static void CopyElements(EXString *pDest, const EXString *pSrc, int nCount)
{
    while (nCount--) *pDest++ = *pSrc++;
}

void EXStringArray::FreeExtra()
{
    if (m_nSize != m_nMaxSize) {
        EXString *pNewData = 0;
        if (m_nSize != 0) _EXRealloc(m_pData, m_nSize * sizeof(EXString));
        else EXFree(m_pData);
        m_pData = pNewData;
        m_nMaxSize = m_nSize;
    }
}

void EXStringArray::SetAt(int i, const EXString &s) { m_pData[i] = s; }

void EXString::AllocMem(u16 nLength, s8 bExact)
{
    u16 n = nLength + 1;
    if (m_pchData) {
        if (n > GetData()->nAllocLength) DeallocMem();
        else {
            GetData()->nDataLength = nLength;
            return;
        }
    }
    n += sizeof(EXStringData);
    if (!bExact) {
        if (n < 0x20) n = 0x20;
        else if (n < 0x40) n = 0x40;
        else if (n < 0x80) n = 0x80;
        else if (n < 0x100) n = 0x100;
        else if (n < 0x200) n = 0x200;
    }
    EXStringData *p = (EXStringData *)_EXAlloc(n, 0);
    m_pchData = p->data();
    p->nRefs = 1;
    p->nDataLength = nLength;
    p->nAllocLength = n - sizeof(EXStringData);
}

void EXString::ReallocMem(u16 nLength)
{
    char *pch = m_pchData;
    if (pch) {
        EXStringData *pOld = (EXStringData *)pch - 1;
        if (pOld->nRefs <= 1 && nLength < pOld->nAllocLength) {
            pOld->nDataLength = nLength;
            return;
        }
        u16 n = nLength + 1;
        int nn = n;
        if (nn < 0x38) n = 0x38;
        else if (nn < 0x18) n = 0x18;
        else if (nn < 0x78) n = 0x78;
        else if (nn < 0xf8) n = 0xf8;
        else if (nn < 0x1f8) n = 0x1f8;
        EXStringData *p = (EXStringData *)_EXAlloc(n + sizeof(EXStringData), 0);
        memcpy(p->data(), m_pchData, pOld->nDataLength + 1);
        p->nRefs = 1;
        p->nDataLength = nLength;
        p->nAllocLength = n;
        DeallocMem();
        m_pchData = p->data();
    } else
        AllocMem(nLength, 0);
}

void EXString::MakeUnique()
{
    if (m_pchData && GetData()->nRefs > 1) {
        EXStringData *pOld = GetData();
        EXStringData *p = (EXStringData *)_EXAlloc(pOld->nAllocLength + sizeof(EXStringData), 0);
        memcpy(p->data(), m_pchData, pOld->nAllocLength);
        p->nRefs = 1;
        p->nDataLength = pOld->nDataLength;
        p->nAllocLength = pOld->nAllocLength;
        pOld->nRefs--;
        m_pchData = p->data();
    }
}

EXString::EXString(const char *psz)
{
    m_pchData = 0;
    if (psz && *psz) {
        AllocMem(strlen(psz), 1);
        strcpy(m_pchData, psz);
    }
}

EXString::EXString(const char *psz, u16 nLength)
{
    m_pchData = 0;
    if (psz && *psz) {
        u16 nLen = strlen(psz);
        u16 n = min(nLength, nLen);
        AllocMem(n, 1);
        strncpy(m_pchData, psz, n);
        m_pchData[n] = 0;
    }
}

char *EXString::GetBuffer(u16 nMinLength)
{
    u16 n = nMinLength;
    if (m_pchData) n = max(nMinLength, GetData()->nDataLength);
    ReallocMem(n);
    return m_pchData;
}

s8 EXString::Compare(const EXString &s) const
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

// NEAR: 29 bytes differ, al/cl swapped for the two lowered characters.
s8 EXString::CompareNoCase(const EXString &s) const
{
    if (m_pchData == s.m_pchData) return 0;
    u16 n1 = Length();
    u16 n2 = s.Length();
    u16 n = min(n1, n2);
    for (u16 i = 0; i < n; i++) {
        char a = EXToLowerC(m_pchData[i]);
        char b = EXToLowerC(s.m_pchData[i]);
        if (a < b) return -1;
        if (a > b) return 1;
    }
    if (n1 > n2) return 1;
    if (n1 < n2) return -1;
    return 0;
}

EXString &EXString::operator=(const char *psz)
{
    AllocMem(strlen(psz), 1);
    strcpy(m_pchData, psz);
    return *this;
}

const EXString &EXString::operator+=(const EXString &s)
{
    u16 n = Length();
    ReallocMem(n + s.Length());
    strcpy(m_pchData + n, s.c_str());
    return *this;
}

const EXString &EXString::operator+=(const char *psz)
{
    if (psz) {
        u32 nAdd = strlen(psz);
        u32 n = Length();
        ReallocMem(nAdd + n);
        strcpy(m_pchData + n, psz);
    }
    return *this;
}

const EXString &EXString::operator+=(char ch)
{
    u32 n = Length();
    ReallocMem(n + 1);
    m_pchData[n] = ch;
    m_pchData[n + 1] = 0;
    return *this;
}

EXString operator+(const EXString &a, const EXString &b)
{
    EXString s;
    u32 n = a.Length();
    s.AllocMem(b.Length() + n, 0);
    s.m_pchData[0] = 0;
    strcpy(s.m_pchData, a.c_str());
    strcpy(s.m_pchData + n, b.c_str());
    return s;
}

EXString operator+(const char *a, const EXString &b)
{
    EXString s(a);
    s += b;
    return s;
}

EXString operator+(const EXString &a, const char *b)
{
    EXString s(a);
    s += b;
    return s;
}

void EXString::ToUpper()
{
    MakeUnique();
    u16 n = Length();
    for (u16 i = 0; i < n; i++) m_pchData[i] = toupper(m_pchData[i]);
}

void EXString::ToLower()
{
    MakeUnique();
    u16 n = Length();
    for (u16 i = 0; i < n; i++) m_pchData[i] = tolower(m_pchData[i]);
}

EXString EXString::Token(int n, char sep, s8 bMerge) const
{
    EXString result;
    if (m_pchData) {
        const char *p = m_pchData;
        const char *pEnd = p + GetData()->nDataLength;
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
                const char *pTok = p;
                while (*p != sep && p < pEnd) p++;
                if (i == n) result = EXString(pTok, p - pTok);
            }
        next:;
        }
    }
    return result;
}

EXString EXString::Token(int n, const EXString &seps, s8 bMerge) const
{
    EXString result;
    if (m_pchData) {
        const char *p = m_pchData;
        const char *pEnd = p + GetData()->nDataLength;
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
                const char *pTok = p;
                while (seps.Find(*p, 0) < 0 && p < pEnd) p++;
                if (i == n) result = EXString(pTok, p - pTok);
            }
        next:;
        }
    }
    return result;
}

typedef char *EX_va_list;
#define EX_va_start(ap, v) (ap = (EX_va_list)&v + sizeof(v))
#define EX_va_arg(ap, t) (*(t *)((ap += (sizeof(t) + 3) & ~3) - ((sizeof(t) + 3) & ~3)))
extern "C" int __cdecl vsprintf(char *, const char *, EX_va_list);

// Estimates the formatted length (one pass over the format), sizes the buffer, then vsprintf's.
short __cdecl EXString::Format(const char *fmt, ...)
{
    EX_va_list args;
    EX_va_start(args, fmt);
    int nMaxLen = 0;
    for (const char *p = fmt; *p; p++) {
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
            nWidth = EXString_atoi(p);
            for (; *p && isdigit(*p); p++) ;
        }
        int nPrecision = 0;
        if (*p == '.') {
            p++;
            if (*p == '*') {
                nPrecision = EX_va_arg(args, int);
                p++;
            } else {
                nPrecision = EXString_atoi(p);
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
            const char *psz = EX_va_arg(args, const char *);
            nItemLen = strlen(psz);
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
    short n = vsprintf(GetBuffer(nMaxLen + 1), fmt, args2);
    if (n > nMaxLen) for (;;) ;
    ReleaseBuffer();
    return n;
}

// NEAR: the original calls AllocMem for the token (inliner budget); here it is inlined.
int EXString::Tokenise(EXStringArray &out, const EXString &seps, s8 bMerge) const
{
    EXFixedArray<char, 256> buf;
    if (!m_pchData) return 0;
    int nStart = out.m_nSize;
    const char *p = m_pchData;
    const char *pEnd = p + GetData()->nDataLength;
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
            const char *pTok = p;
            while (seps.Find(*p, 0) < 0 && p < pEnd) p++;
            int nLen = p - pTok;
            int nMax = 0xff;
            nLen = min(nMax, nLen);
            memcpy(buf.GetData(), pTok, nLen);
            buf[nLen] = 0;
            EXString tok(buf);
            out.Add(tok);
        }
    }
done:
    return out.m_nSize - nStart;
}

// ---- EXStringArray
static void ConstructElements(EXString *p, int nCount)
{
    for (; nCount; nCount--, p++) new (p) EXString;
}

static void DestructElements(EXString *p, int nCount)
{
    for (; nCount; nCount--, p++) p->EXString::~EXString();
}

void EXStringArray::SetGrowBy(int nGrowBy)
{
    int nMin = 8;
    m_nGrowBy = max(nGrowBy, nMin);
}

void EXStringArray::RemoveAll()
{
    if (m_pData) {
        DestructElements(m_pData, m_nSize);
        EXFree(m_pData);
        m_pData = 0;
    }
    m_nMaxSize = 0;
    m_nSize = 0;
}

void EXStringArray::RemoveAt(int i, int nCount)
{
    int nMove = m_nSize - i - nCount;
    DestructElements(&m_pData[i], nCount);
    if (nMove) memmove(&m_pData[i], &m_pData[i + nCount], nMove * sizeof(EXString));
    m_nSize -= nCount;
}

EXStringArray::~EXStringArray()
{
    if (m_pData) {
        DestructElements(m_pData, m_nSize);
        EXFree(m_pData);
        m_pData = 0;
    }
    m_nMaxSize = 0;
    m_nSize = 0;
}

void EXStringArray::SetSize(int nNewSize, int nGrowBy)
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
        m_pData = (EXString *)_EXAlloc(nNewSize * sizeof(EXString), 0);
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
        m_pData = (EXString *)_EXRealloc(m_pData, nNewMax * sizeof(EXString));
        ConstructElements(&m_pData[m_nSize], nNewSize - m_nSize);
        m_nMaxSize = nNewMax;
        m_nSize = nNewSize;
    }
}

void EXStringArray::SetAtGrow(int i, const EXString &s)
{
    if (i >= m_nSize) SetSize(i + 1, -1);
    m_pData[i] = s;
}

int EXStringArray::Add(const EXString &s)
{
    int i = m_nSize;
    SetAtGrow(i, s);
    return i;
}

int EXStringArray::Append(const EXStringArray &a)
{
    int nOld = m_nSize;
    SetSize(m_nSize + a.m_nSize, -1);
    CopyElements(m_pData + nOld, a.m_pData, a.m_nSize);
    return nOld;
}

void EXStringArray::Copy(const EXStringArray &a)
{
    SetSize(a.m_nSize, -1);
    CopyElements(m_pData, a.m_pData, a.m_nSize);
}

void EXStringArray::InsertAt(int i, const EXString &s, int nCount)
{
    if (i >= m_nSize) SetSize(i + nCount, -1);
    else {
        int nOld = m_nSize;
        SetSize(m_nSize + nCount, -1);
        DestructElements(&m_pData[nOld], nCount);
        memmove(&m_pData[i + nCount], &m_pData[i], (nOld - i) * sizeof(EXString));
        ConstructElements(&m_pData[i], nCount);
    }
    while (nCount--) m_pData[i++] = s;
}

void EXStringArray::InsertAt(int i, const EXStringArray &a)
{
    if (a.m_nSize > 0) {
        InsertAt(i, a.m_pData[0], a.m_nSize);
        for (int j = 0; j < a.m_nSize; j++) SetAt(i + j, a.m_pData[j]);
    }
}

EXStringArray::EXStringArray(int nSize, int nGrowBy)
{
    int nMin = 8;
    m_nMaxSize = 0;
    m_nSize = 0;
    m_pData = 0;
    m_nGrowBy = max(nGrowBy, nMin);
    if (nSize > 0) SetSize(nSize, -1);
}
