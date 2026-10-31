// Compiler-match experiment, batch 1. Types are minimal stand-ins: only the fields these
// functions touch, at the offsets the retail code uses.
typedef signed char s8;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned __int64 u64;

extern "C" double __cdecl sin(double);
extern "C" double __cdecl cos(double);
#pragma intrinsic(sin, cos)
void EXFree(void *p);
inline void __cdecl operator delete(void *p) { EXFree(p); }

class EXVector {
public:
    float x, y, z, w;
    EXVector() {}
    EXVector(float x_, float y_, float z_, float w_ = 0.0f) { x = x_; y = y_; z = z_; w = w_; }
};
class EXMatrix { public: EXVector r[4]; };

template <class T, class A> class EXDataArray {
public:
    int m_Max;
    int m_Grow;
    int m_Count;
    T *m_pData;
    void Add(T v) { if (m_Count + 1 > m_Max) GrowArray(m_Count + 1); m_pData[m_Count] = v; m_Count++; }
    T &operator[](int i) { return m_pData[i]; }
protected:
    void GrowArray(int n);
};

template <class T, int N> class EXFixedArray {
public:
    T m_Data[N];
    T &operator[](int i) { return m_Data[i]; }
};



struct EXFILEINFO;
class EXFile {
public:
    EXFile();
    virtual ~EXFile();
    u8 pad[0x20 - 4];
protected:
    void CreateMXFile(const EXFILEINFO *info, u32 size, u32 start);
};
class EXFileSys {
public:
    const EXFILEINFO *GetFileInfo(const EXFile *f) const;
protected:
    static EXFileSys *m_pFileSys;
    friend class EXGeoSectionFile;
};
class EXGeoFile : public EXFile {};
class EXGeoSectionHeader { public: u32 m_0; u32 m_Start; u32 m_End; };
class EXGeoSectionFile : public EXFile {
public:
    EXGeoSectionFile(EXGeoFile *pFile, const EXGeoSectionHeader *pHdr, short index);
    virtual ~EXGeoSectionFile();
    int m_Index;
    EXGeoFile *m_pFile;
    u8 m_b28, m_b29, m_b2a;
};
EXGeoSectionFile::EXGeoSectionFile(EXGeoFile *pFile, const EXGeoSectionHeader *pHdr, short index)
{
    u32 start = pHdr->m_Start;
    u32 size = (pHdr->m_End - start + 0x7ff) & ~0x7ff;
    const EXFILEINFO *info = EXFileSys::m_pFileSys->GetFileInfo(pFile);
    CreateMXFile(info, size, start);
    m_Index = index;
    m_b28 = 0;
    m_b29 = 0;
    m_b2a = 0;
    m_pFile = pFile;
}

struct _ATTACH_DATA { u32 m_WeaponId; u32 m_4; u8 m_Avail; u8 m_b9; u8 m_bA; u8 pad[0x28 - 0xb]; };
extern EXFixedArray<_ATTACH_DATA, 5> Attach_Data;
struct _WEAPON_LOAD { u32 m_Id; };
extern EXFixedArray<_WEAPON_LOAD, 32> WeaponLoadList;
class FEHud {
public:
    static FEHud *Instance(s8 create);
    s8 IsWeaponAvailable(u32 id);
};
class XItemHandler_Player {
public:
    void InitPlayerBodyWeapons();
    u8 pad[0x4d0];
    int m_nWeaponLoads;
protected:
    void AddWeaponLoad(u32 id);
};
void XItemHandler_Player::InitPlayerBodyWeapons()
{
    _ATTACH_DATA *d = &Attach_Data[0];
    for (int n = 5; n; n--, d++) {
        d->m_Avail = 0;
        d->m_bA = 0;
        d->m_b9 = 0;
        FEHud *hud = FEHud::Instance(1);
        if (hud->IsWeaponAvailable(d->m_WeaponId)) d->m_Avail = 1;
    }
}
void XItemHandler_Player::AddWeaponLoad(u32 id)
{
    int i;
    for (i = 0; i < m_nWeaponLoads; i++)
        if (WeaponLoadList[i].m_Id == id) return;
    WeaponLoadList[i].m_Id = id;
    m_nWeaponLoads++;
}

class FEObject { public: u8 pad[0x60]; u64 m_Id; };
template <class T> class FEPtrList {
public:
    T **m_pData;
    u32 m_Count;
    u32 Count() const { return m_Count; }
    T *operator[](u32 i) { return m_pData[i]; }
};
class FELayoutPaged {
public:
    virtual FEObject *Page(u64 id, int *pIndex);
    u8 pad[0x1b8 - 4];
    FEPtrList<FEObject> m_Pages;
};
FEObject *FELayoutPaged::Page(u64 id, int *pIndex)
{
    for (u32 i = 0; i < m_Pages.Count(); i++) {
        if (m_Pages[i] && id == m_Pages[i]->m_Id) {
            if (pIndex) *pIndex = i;
            return m_Pages[i];
        }
    }
    return 0;
}

class WeaponProfile {
public:
    int m_Id;
    u32 m_MapHash;
    u8 pad[0xe8 - 8];
    static int GetWeaponDataIndex(int id);
private:
    static WeaponProfile *ms_pWeaponTable;
    static int ms_iWeaponTableCount;
};
extern u32 gCurrentMap_GeoHash;
int WeaponProfile::GetWeaponDataIndex(int id)
{
    int found = -1;
    for (int i = 0; i < ms_iWeaponTableCount; i++) {
        if (ms_pWeaponTable[i].m_Id == id) {
            if (ms_pWeaponTable[i].m_MapHash == gCurrentMap_GeoHash) { found = i; break; }
            if (ms_pWeaponTable[i].m_MapHash == 0) found = i;
        }
    }
    return found;
}
