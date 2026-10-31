// Compiler-match experiment, batch 1. Types are minimal stand-ins: only the fields these
// functions touch, at the offsets the retail code uses.
typedef signed char s8;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned __int64 u64;

extern "C" float __cdecl sinf(float);
extern "C" float __cdecl cosf(float);
#pragma intrinsic(sinf, cosf)

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

// ---------------------------------------------------------------- ActorCarry.obj
struct ScriptHandle { u8 pad[0x10]; u8 m_Flags; };
class ActorCarryableGun {
public:
    u8 pad[0x8c];
    ScriptHandle *m_pImpactScript;
    ScriptHandle *m_pImpactScript2;
    void DeleteSceneryImpactScript();
};
void ActorCarryableGun::DeleteSceneryImpactScript()
{
    if (m_pImpactScript) { m_pImpactScript->m_Flags |= 0x10; m_pImpactScript = 0; }
    if (m_pImpactScript2) { m_pImpactScript2->m_Flags |= 0x10; m_pImpactScript2 = 0; }
}

// ---------------------------------------------------------------- Player.obj
void SetRotation(EXVector &v, float angle, float dist)
{
    v.x = sinf(angle) * dist;
    v.y = 0.0f;
    v.z = cosf(angle) * dist;
    v.w = 0.0f;
}

// ---------------------------------------------------------------- EXPhysicsSoft.obj
class EXCloth2;
class EXCloth2Manager {
public:
    EXDataArray<void *, void *> m_Cloths;
    void AddCloth(EXCloth2 *p);
};
void EXCloth2Manager::AddCloth(EXCloth2 *p) { m_Cloths.Add(p); }

// ---------------------------------------------------------------- EXPhysics6D.obj
class EX6DManager;
class EX6DObject {
public:
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09(); virtual void v10(); virtual void v11();
    virtual void v12(); virtual void v13(); virtual void v14(); virtual void v15();
    virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v20(); virtual void v21(); virtual void v22(); virtual void v23();
    virtual void OnAdd();
    u8 pad[0x48];
    EX6DManager *m_pManager;
};
class EX6DManager {
public:
    EXDataArray<void *, void *> m_Bodies;
    void AddBody(EX6DObject *p);
};
void EX6DManager::AddBody(EX6DObject *p)
{
    m_Bodies.Add(p);
    ((EX6DObject *)m_Bodies[m_Bodies.m_Count - 1])->OnAdd();
    p->m_pManager = this;
}

// ---------------------------------------------------------------- EXAudioVoice.obj
struct SampleHeaderData { int m_0; int m_Id; };
class Voice { public: u8 pad[0x25]; u8 m_Reserved; };
class EXAudioVoiceManager { public: Voice *VoiceAlloc(int id, int prio); };
class XBAudioVoiceManager : public EXAudioVoiceManager {};
extern XBAudioVoiceManager *gpAudioVoice;
class EXAudio { public: virtual int psiRequestVoiceHandle(SampleHeaderData *p, int prio); };
int EXAudio::psiRequestVoiceHandle(SampleHeaderData *p, int prio)
{
    Voice *v = gpAudioVoice->VoiceAlloc(p->m_Id, prio);
    if (v) { v->m_Reserved = 1; return (int)v; }
    return -1;
}

// ---------------------------------------------------------------- EXFileSys.obj
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
    const struct EXFILEINFO *GetFileInfo(const class EXFile *f) const;
protected:
    static EXFileSys *m_pFileSys;
    friend class EXGeoSectionFile;
};
s8 EXFileSys::GetLoadingStatus() const
{
    return (m_Pending.Count() + (m_pCurrent != 0) + m_nQueued) != 0;
}

// ---------------------------------------------------------------- ActorUtil.obj
class XItemHandler_Player {
public:
    EXVector GetPelvisPos();
    void AddWeaponLoad(u32 id);
    void InitPlayerBodyWeapons();
    u8 pad[0x4d0];
    int m_nWeaponLoads;
protected:
    static XItemHandler_Player *m_pInstance;
    friend EXVector Player_GetPelvisPos();
};
EXVector Player_GetPelvisPos()
{
    if (!XItemHandler_Player::m_pInstance) return EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    return XItemHandler_Player::m_pInstance->GetPelvisPos();
}

class Actor {
public:
    void Delete();
    u8 pad0[0x770];
    int m_POIChannel;
    u8 pad1[0x7e4 - 0x774];
    int m_7e4;
    u8 pad2[0x93c - 0x7e8];
    u32 m_StateFlags;
};
class StateMachine {
public:
    u8 pad0[0x50];
    Actor *m_pActor;
    u8 pad1[0x88 - 0x54];
    u32 m_State;
    int m_POI;
};
s8 POI_ForceFlagChannel(int poi, int channel);
s8 STATE_FS_Delete(StateMachine &sm)
{
    sm.m_pActor->m_7e4 = 0;
    if (sm.m_State != 0xf0000001) return 0;
    sm.m_pActor->Delete();
    if (sm.m_POI) POI_ForceFlagChannel(sm.m_POI, sm.m_pActor->m_POIChannel);
    return 1;
}

// ---------------------------------------------------------------- ActorCombat.obj
s8 Actor_DelayedSightToTarget(Actor &a, int &delay, int &sight, int time)
{
    int canSee = (s8)((a.m_StateFlags & 0x200000) == 0x200000);
    if (canSee != sight && delay == 0) delay = time;
    else if (delay == 1) { sight = canSee; delay = 0; }
    return (s8)sight;
}

// ---------------------------------------------------------------- FELayout.obj
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
        FEObject *p = m_Pages.m_pData[i];
        if (p && p->m_Id == id) {
            if (pIndex) *pIndex = i;
            return m_Pages[i];
        }
    }
    return 0;
}

// ---------------------------------------------------------------- EXGeoFile.obj
struct EXFILEINFO;
class EXFile {
public:
    EXFile();
    virtual ~EXFile();
    u8 pad[0x20 - 4];
protected:
    void CreateMXFile(const EXFILEINFO *info, u32 size, u32 start);
};
class EXFileSys2;
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
    CreateMXFile(EXFileSys::m_pFileSys->GetFileInfo(pFile), size, start);
    m_Index = index;
    m_b28 = m_b29 = m_b2a = 0;
    m_pFile = pFile;
}

// ---------------------------------------------------------------- Player.obj
struct _ATTACH_DATA { u32 m_WeaponId; u32 m_4; u8 m_Avail; u8 m_b9; u8 m_bA; u8 pad[0x28 - 0xb]; };
extern EXFixedArray<_ATTACH_DATA, 5> Attach_Data;
class FEHud {
public:
    static FEHud *Instance(s8 create);
    s8 IsWeaponAvailable(u32 id);
};
void XItemHandler_Player::InitPlayerBodyWeapons()
{
    for (int i = 0; i < 5; i++) {
        Attach_Data[i].m_Avail = 0;
        Attach_Data[i].m_bA = 0;
        Attach_Data[i].m_b9 = 0;
        if (FEHud::Instance(1)->IsWeaponAvailable(Attach_Data[i].m_WeaponId))
            Attach_Data[i].m_Avail = 1;
    }
}

struct _WEAPON_LOAD { u32 m_Id; };
extern EXFixedArray<_WEAPON_LOAD, 32> WeaponLoadList;
void XItemHandler_Player::AddWeaponLoad(u32 id)
{
    int i;
    for (i = 0; i < m_nWeaponLoads; i++)
        if (WeaponLoadList[i].m_Id == id) return;
    WeaponLoadList[i].m_Id = id;
    m_nWeaponLoads++;
}

// ---------------------------------------------------------------- NavNetwork.obj
class XNavLink {
public:
    u16 m_0, m_2, m_NodeA, m_NodeB, m_8, m_A;
    float m_Cost;
    XNavLink();
};
XNavLink::XNavLink()
{
    m_0 = 0xffff; m_2 = 0; m_NodeA = 0xffff; m_NodeB = 0xffff; m_8 = 0xffff; m_A = 0xffff;
    m_Cost = 0.0f;
}

// ---------------------------------------------------------------- FELang.obj
class XApp { protected: static float m_FrameLength; friend class FELang; };
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
    m_Timer1 += XApp::m_FrameLength;
    m_Timer2 += XApp::m_FrameLength;
    FEListener::LastInputReset();
    FEWnd::HandleInput();
}

// ---------------------------------------------------------------- EXMaths.obj
extern const u32 CrcTable[256];
u32 EXCalculateChecksum(const u8 *p, u32 n)
{
    u32 crc = 0xffffffff;
    if (n) {
        do {
            crc = CrcTable[(*p ^ crc) & 0xff] ^ (crc >> 8);
            p++;
        } while (--n);
    }
    return ~crc;
}

// ---------------------------------------------------------------- Traffic.obj
class TrafficSystem { public: void close(); ~TrafficSystem(); };
void EXFree(void *p);
class TrafficSysAlloc {
public:
    TrafficSystem *m_p;
};
extern TrafficSysAlloc _theTrafficSysAlloc;
void TrafficDeinit()
{
    if (_theTrafficSysAlloc.m_p) {
        _theTrafficSysAlloc.m_p->close();
        TrafficSystem *p = _theTrafficSysAlloc.m_p;
        if (p) { p->~TrafficSystem(); EXFree(p); }
        _theTrafficSysAlloc.m_p = 0;
    }
}

// ---------------------------------------------------------------- EXMemoryManager.obj
class EXMemHeap { public: void *_sysCacheDel(u32 a, u16 b, u16 c, void **d, u32 e); };
extern EXMemHeap *_SystemHeap;
void *_EXCacheDel(u32 a, short b, short c, void **d, u32 e)
{
    return _SystemHeap->_sysCacheDel(a, b, c, d, e);
}

// ---------------------------------------------------------------- WeaponTable.obj
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
            if (ms_pWeaponTable[i].m_MapHash == gCurrentMap_GeoHash) return i;
            if (ms_pWeaponTable[i].m_MapHash == 0) found = i;
        }
    }
    return found;
}

// ---------------------------------------------------------------- Text.obj
class EXString { public: s8 IsEmpty() const; void Empty(); };
struct TextInfo { u8 pad0[0xc]; int m_Mode; u8 pad1[0x18 - 0x10]; EXString m_Str; u8 pad2[0x20 - 0x18 - sizeof(EXString)]; float m_Time; u8 pad3[0x2c - 0x24]; };
extern TextInfo TextBuffer[32];
static inline void Text_Clear(TextInfo *t)
{
    if (t) {
        if (!t->m_Str.IsEmpty()) t->m_Str.Empty();
        t->m_Time = 0.0f;
        t->m_Mode = 6;
    }
}
void Text_Shutdown()
{
    for (int i = 0; i < 32; i++) Text_Clear(&TextBuffer[i]);
}

// ---------------------------------------------------------------- FEScript.obj
class FEItem { public: u8 pad[0x70]; u32 m_Flags; };
class EXItemPos { public: u8 pad[0xc8]; EXVector m_Pos; };
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
    return m_pPos->m_Pos;
}

// ---------------------------------------------------------------- NavNetwork.obj
class XNavNode { public: u8 pad[0x10]; u16 m_Id; u8 pad2[0x16 - 0x12]; u16 m_nLinks; u8 pad3[0x20 - 0x18]; u16 *m_pLinks; };
class XNavLinkRef { public: u16 m_0, m_2, m_NodeA, m_NodeB; };
class XNavSection {
public:
    XNavLinkRef *Link(u16 i);
    XNavNode *Node(u16 i);
};
class XNavNetwork {
public:
    u8 pad[8];
    XNavSection m_Section;
    XNavNode *ConnectedNodePtr(XNavNode *pNode, u16 i, XNavLinkRef **ppLink);
};
XNavNode *XNavNetwork::ConnectedNodePtr(XNavNode *pNode, u16 i, XNavLinkRef **ppLink)
{
    if (i >= pNode->m_nLinks || !pNode->m_pLinks) return 0;
    u16 li = pNode->m_pLinks[i * 2];
    if (li == 0xffff) return 0;
    XNavLinkRef *l = m_Section.Link(li);
    *ppLink = l;
    if (l->m_NodeA == 0xffff || l->m_NodeB == 0xffff) return 0;
    if (l->m_NodeA == pNode->m_Id) return m_Section.Node(l->m_NodeB);
    return m_Section.Node(l->m_NodeA);
}

// ---------------------------------------------------------------- EXStdFuncs.obj
u32 Write_Multi_Char(u16 *buf, u16 ch, u32 count, u32 pos)
{
    if (count == 0) return pos;
    u16 *p = buf + pos;
    for (u32 n = count; n; n--) *p++ = ch;
    return pos + count;
}
