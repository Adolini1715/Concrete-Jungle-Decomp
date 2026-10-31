#ifndef EXARRAY_H
#define EXARRAY_H
#include "types.h"
#include "EXMemory.h"
#include "EXStdFuncs.h"

// The map has _EXRealloc(void *, unsigned int); EXMemory.h (other owner) declares a 3-arg form.
void *_EXRealloc(void *p, u32 size);

// Helpers in EXArray.obj (the trailing (base, size) pairs are for bounds validation in debug).
void EXArray_ValidateCopy(void *dst, const void *src, u32 n, void *base, u32 size);
void EXArray_ValidateFill(void *dst, u32 n, void *base, u32 size);
u32 EXArray_strlen(const char *s, void *base, u32 size);
void *EXArray_memcpy(void *dst, const void *src, u32 n, void *base, u32 size);
void *EXArray_memmove(void *dst, const void *src, u32 n, void *base, u32 size);
void *EXArray_memset(void *dst, int c, u32 n, void *base, u32 size);
void *EXArray_memchr(void *p, int c, u32 n, void *base, u32 size);
char *EXArray_strcpy(char *dst, const char *src, void *base, u32 size);
char *EXArray_strcat(char *dst, const char *src, void *base, u32 size);

// Growable POD array: {max, growBy, count, data}; storage from _EXRealloc, no element ctors.
template <class T, class ARG> class EXDataArray {
public:
    EXDataArray(int nSize = 0, int nGrowBy = 0)
    {
        m_nMaxSize = 0;
        m_nSize = 0;
        m_pData = 0;
        SetGrowBy(nGrowBy);
        if (nSize != 0) SetSize(nSize, -1);
    }
    ~EXDataArray() { EXFree(m_pData); }
    void Init(int nSize, int nGrowBy)
    {
        m_nMaxSize = 0;
        m_nSize = 0;
        m_pData = 0;
        SetGrowBy(nGrowBy);
        if (nSize != 0) SetSize(nSize, -1);
    }
    void SetGrowBy(int nGrowBy)
    {
        // Default step: 4 for pointer-sized elements, 16 for larger ones (all emitted instances agree).
        if (nGrowBy > 0) m_nGrowBy = nGrowBy;
        else m_nGrowBy = sizeof(T) > 4 ? 16 : 4;
    }
    int Size() const { return m_nSize; }
    int AllocSize() const { return m_nMaxSize; }  // XBGeoEntity.obj copy
    int GetUpperBound() const { return m_nSize - 1; }
    T *GetData() { return m_pData; }
    const T *GetData() const { return m_pData; }
    T GetAt(int i) const { return m_pData[i]; }
    T &ElementAt(int i) { return m_pData[i]; }
    void SetAt(int i, ARG v) { m_pData[i] = v; }  // appended by the vehicles agent (Traffic.obj)
    void SetAtGrow(int i, ARG v)
    {
        if (i >= m_nSize) SetSize(i + 1, -1);
        m_pData[i] = v;
    }
    T &operator[](int i) { return m_pData[i]; }
    // Appended by the effects agent (EXParticle.obj emits the const instance).
    const T &operator[](int i) const { return m_pData[i]; }
    void RemoveAll()
    {
        if (m_pData) EXFree(m_pData);
        m_pData = 0;
        m_nMaxSize = 0;
        m_nSize = 0;
    }
    void Empty(s8 bFree)
    {
        if (bFree) {
            if (m_pData) EXFree(m_pData);
            m_pData = 0;
            m_nMaxSize = 0;
        }
        m_nSize = 0;
    }
    void SetSize(int nNewSize, int nGrowBy)
    {
        if (nGrowBy != -1) m_nGrowBy = nGrowBy;
        if (nNewSize == 0) {
            if (m_pData) EXFree(m_pData);
            m_pData = 0;
            m_nMaxSize = 0;
        } else if (nNewSize >= m_nMaxSize) {
            if (nNewSize > m_nSize) {
                GrowArray(nNewSize);
            } else {
                m_pData = (T *)_EXRealloc(m_pData, nNewSize * sizeof(T));
                m_nMaxSize = nNewSize;
            }
        }
        m_nSize = nNewSize;
    }
    int Add(ARG v, int n = 1)
    {
        int nIndex = m_nSize;
        if (nIndex + n > m_nMaxSize) GrowArray(nIndex + n);
        for (int i = 0; i < n; i++) m_pData[m_nSize + i] = v;
        m_nSize += n;
        return nIndex;
    }
    int AddUnique(ARG v)
    {
        int nIndex = m_nSize;
        for (int i = 0; i < m_nSize; i++)
            if (m_pData[i] == v) return i;
        Add(v, 1);
        return nIndex - 1;
    }
    void RemoveAt(int nIndex, int nCount = 1)
    {
        if (nIndex + nCount < m_nSize)
            memcpy(&m_pData[nIndex], &m_pData[nIndex + nCount], (m_nSize - (nIndex + nCount)) * sizeof(T));
        m_nSize -= nCount;
    }
    // Insert nCount copies at nIndex (EXGeoCommon.obj, _EXGeoCommonObjectDef registration).
    void InsertAt(int nIndex, ARG newElement, int nCount = 1)
    {
        if (nIndex > GetUpperBound()) {
            if (m_nSize + nCount > m_nMaxSize) GrowArray(m_nSize + nCount);
            for (int i = 0; i < nCount; i++) m_pData[nIndex + i] = newElement;
            m_nSize = nIndex + nCount;
        } else {
            if (m_nSize + nCount > m_nMaxSize) GrowArray(m_nSize + nCount);
            memmove(&m_pData[nIndex + nCount], &m_pData[nIndex], (m_nSize - nIndex) * sizeof(T));
            for (int i = 0; i < nCount; i++) m_pData[nIndex + i] = newElement;
            m_nSize += nCount;
        }
    }
    // Appended (GameWnd.obj emits the EXFILEINFO instance): grow, then copy the elements raw.
    void Copy(const EXDataArray &src)
    {
        int n = src.m_nSize;
        if (n > m_nMaxSize) GrowArray(n);
        memcpy(m_pData, src.m_pData, n * sizeof(T));
        m_nSize = n;
    }
    // Reserve room (EXWatcher.obj): grows by the shortfall, not to the requested size.
    void AllocMem(int n)
    {
        if (n > m_nMaxSize) GrowArray(n - m_nMaxSize);
    }
    // Append n uninitialised elements (App.obj, EXStackBlock); the growth loop is written out here.
    T &AddSpace(int n)
    {
        int nIndex = m_nSize;
        int nNewSize = nIndex + n;
        if (nNewSize > m_nMaxSize) GrowArray(nNewSize);  // VC inlines GrowArray in some units (App.obj), not others
        m_nSize += n;
        return m_pData[nIndex];
    }
protected:
    void GrowArray(int nNewSize)
    {
        int nMax = m_nMaxSize + m_nGrowBy;
        while (nNewSize > 0 && nMax < nNewSize) nMax += m_nGrowBy;
        m_pData = (T *)_EXRealloc(m_pData, nMax * sizeof(T));
        m_nMaxSize = nMax;
    }
public:
    int m_nMaxSize;
    int m_nGrowBy;
    int m_nSize;
    T *m_pData;
};

// Array of pointers sharing one EXDataArray<void *, void *> implementation.
template <class T> class EXPtrArray : public EXDataArray<void *, void *> {
public:
    EXPtrArray(int nSize = 0, int nGrowBy = 0) : EXDataArray<void *, void *>(nSize, nGrowBy) {}
    ~EXPtrArray() {}
    T **GetData() { return (T **)m_pData; }
    const T **GetData() const { return (const T **)m_pData; }
    T *GetAt(int i) const { return (T *)m_pData[i]; }
    template <class I> T *&operator[](I i) { return ((T **)m_pData)[i]; }
    template <class I> T *operator[](I i) const { return ((T **)m_pData)[i]; }  // EXItemEnv.obj
    int Add(T *p, int n = 1) { return EXDataArray<void *, void *>::Add((void *)p, n); }
    u32 AddUnique(T *p);
    int Find(const T *p) const;
};

int EXPtrArray_Find(const EXPtrArray<void> *a, const void *p);
int EXPtrArray_AddUnique(EXPtrArray<void> *a, void *p);
template <class T> inline u32 EXPtrArray<T>::AddUnique(T *p) { return EXPtrArray_AddUnique((EXPtrArray<void> *)this, p); }
template <class T> inline int EXPtrArray<T>::Find(const T *p) const { return EXPtrArray_Find((const EXPtrArray<void> *)this, p); }

// Fixed-size inline array; the index operator is a member template (int, unsigned, enums...).
template <class T, int N> class EXFixedArray {
public:
    template <class I> T &operator[](I i) { return m_Data[i]; }
    template <class I> const T &operator[](I i) const { return m_Data[i]; }
    operator T *() { return m_Data; }
    operator const T *() const { return m_Data; }
    T *GetData() { return m_Data; }
    const T *GetData() const { return m_Data; }
    unsigned int Size() const { return N; }  // EXMemCard.obj copy
    T m_Data[N];
};

// ---- appended by the window/audio agent: MFC-style array with element construction (EXDebugTree.obj) ----
#ifndef EXARRAY_TEMPLATE_DEFINED
#define EXARRAY_TEMPLATE_DEFINED
// Placement new is only declared here (several units define their own copy); units that instantiate
// ConstructElements or EXClassAllocator include EXNew.h for the definition.
inline void *__cdecl operator new(unsigned int, void *p);
template <class T> inline void ConstructElements(T *p, int n)
{
    for (; n != 0; n--, p++) new ((void *)p) T;
}
template <class T> inline void DestructElements(T *p, int n)
{
    for (; n != 0; n--, p++) p->~T();
}
// {vptr, max, growBy, size, data}; first allocation via _EXAlloc, growth via _EXRealloc.
template <class T, class ARG> class EXArray {
public:
    EXArray()
    {
        m_pData = 0;
        m_nGrowBy = 0;
        m_nMaxSize = 0;
        m_nSize = 0;
    }
    virtual ~EXArray()
    {
        if (m_pData) {
            if (m_nSize > 0) DestructElements(m_pData, m_nSize);
            EXFree(m_pData);
            m_pData = 0;
        }
    }
    int Size() const { return m_nSize; }
    T *GetData() { return m_pData; }
    T &operator[](int i) { return m_pData[i]; }
    const T &operator[](int i) const { return m_pData[i]; }  // appended (EXItemEnv.obj, EXMapZoneInfo instance)
    T &ElementAt(int i) { return m_pData[i]; }
    void SetAt(int i, ARG v) { m_pData[i] = v; }  // appended by the vehicles agent (Traffic.obj)
    void SetSize(int nNewSize, int nGrowBy = -1)
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
            m_pData = (T *)_EXAlloc(nNewSize * sizeof(T), 0);
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
            int nNewMax;
            if (nNewSize < m_nMaxSize + nGrow) nNewMax = m_nMaxSize + nGrow;
            else nNewMax = nNewSize;
            m_pData = (T *)_EXRealloc(m_pData, nNewMax * sizeof(T));
            ConstructElements(&m_pData[m_nSize], nNewSize - m_nSize);
            m_nMaxSize = nNewMax;
            m_nSize = nNewSize;
        }
    }
    // Appended by the traffic agent (Traffic.obj instances).
    void RemoveAll()
    {
        if (m_pData) {
            DestructElements(m_pData, m_nSize);
            EXFree(m_pData);
            m_pData = 0;
        }
        m_nMaxSize = 0;
        m_nSize = 0;
    }
    // Appended by the actor agent (ActorTextureSwap.obj copy).
    void Empty(s8 bFree)
    {
        DestructElements(m_pData, m_nSize);
        if (bFree) {
            if (m_pData) EXFree(m_pData);
            m_pData = 0;
            m_nMaxSize = 0;
        }
        m_nSize = 0;
    }
    // Appended by the widgets agent (EXWidget.obj instances), MFC forms.
    void SetAtGrow(int i, ARG v)
    {
        if (i >= m_nSize) SetSize(i + 1, -1);
        m_pData[i] = v;
    }
    int Add(ARG v)
    {
        int i = m_nSize;
        SetAtGrow(i, v);
        return i;
    }
    int m_nMaxSize;  // 0x04
    int m_nGrowBy;   // 0x08
    int m_nSize;     // 0x0c
    T *m_pData;      // 0x10
};
#endif
#endif
