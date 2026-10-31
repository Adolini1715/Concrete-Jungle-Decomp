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


void SetRotation(EXVector &v, float angle, float dist)
{
    v.x = sin(angle) * dist;
    v.w = 0.0f;
    v.y = 0.0f;
    v.z = cos(angle) * dist;
}

class Actor { public: u8 pad[0x93c]; u32 m_StateFlags; s8 IsStateFlag(u32 f) const { return (m_StateFlags & f) == f; } };
s8 Actor_DelayedSightToTarget(Actor &a, int &delay, int &sight, int time)
{
    int canSee = a.IsStateFlag(0x200000);
    if (canSee != sight && !delay) delay = time;
    else if (delay == 1) { sight = canSee; delay = 0; }
    return (s8)sight;
}

class XApp { public: static float FrameLength() { return m_FrameLength; } protected: static float m_FrameLength; };
class FEListener { public: static void LastInputReset(); };
class FEWnd { public: void HandleInput(); virtual void ContentUpdate(); };
class FELang : public FEWnd {
public:
    virtual void ContentUpdate();
    u8 pad[0x404 - 4];
    float m_Timer1;
    float m_Timer2;
};
void FELang::ContentUpdate()
{
    float dt = XApp::FrameLength();
    m_Timer1 += dt;
    m_Timer2 += dt;
    FEListener::LastInputReset();
    FEWnd::HandleInput();
}

class EXDList { public: u32 Count() const; };
class EXFileSys {
public:
    u8 pad0[0x10];
    EXDList m_Pending;
    u8 pad1[0x20 - 0x10 - sizeof(EXDList)];
    void *m_pCurrent;
    u8 pad2[0x64 - 0x24];
    int m_nQueued;
    s8 GetLoadingStatus() const;
};
s8 EXFileSys::GetLoadingStatus() const
{
    return (m_Pending.Count() + (m_pCurrent ? 1 : 0) + m_nQueued) ? 1 : 0;
}

class TrafficSystem { public: void close(); ~TrafficSystem(); };
class TrafficSysAlloc { public: TrafficSystem *m_p; };
extern TrafficSysAlloc _theTrafficSysAlloc;
void TrafficDeinit()
{
    if (_theTrafficSysAlloc.m_p) {
        _theTrafficSysAlloc.m_p->close();
        delete _theTrafficSysAlloc.m_p;
        _theTrafficSysAlloc.m_p = 0;
    }
}

class FEItem { public: u8 pad[0x70]; u32 m_Flags; };
class EXItemPos { public: u8 pad[0xc8]; EXVector m_Pos; const EXVector &Pos() const { return m_Pos; } };
class FEObjectScript {
public:
    u8 pad[0x1c];
    FEItem *m_pItem;
    EXItemPos *m_pPos;
    EXVector Position();
};
EXVector FEObjectScript::Position()
{
    if (!m_pPos) return EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    m_pItem->m_Flags &= ~0x20;
    const EXVector &p = m_pPos->Pos();
    return EXVector(p.x, p.y, p.z, p.w);
}

u32 Write_Multi_Char(u16 *buf, u16 ch, u32 count, u32 pos)
{
    if (count > 0) {
        u16 *p = buf + pos;
        for (u32 n = count; n; n--) *p++ = ch;
        pos += count;
    }
    return pos;
}

class EXGeoSectionFile;
struct EXFILEINFO;
class EXFile {
public:
    EXFile();
    virtual ~EXFile();
    u8 pad[0x20 - 4];
protected:
    void CreateMXFile(const EXFILEINFO *info, u32 size, u32 start);
};
class EXFileSysG {
public:
    const EXFILEINFO *GetFileInfo(const EXFile *f) const;
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
