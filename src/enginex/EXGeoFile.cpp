#include <string.h>
#include "EXGeoFile.h"
#include "EXGeoHeader.h"
#include "EXGeoComplexRefObject.h"
#include "EXGeoSpreadSheet.h"
#include "EXGeoCommonObject.h"
#include "EXTexture.h"
#include "EXFileSys.h"
#include "EXMemHeap.h"
#include "EXMemory.h"
#include "EXHashCodeFile.h"
#include "EXBounds.h"
#include "EXAllocator.h"
#include "EXItemAnimator_Anim.h"

class EXGeoBaseDebug;
class EXGeoDebugExtHeader {
public:
    const char *FileName() const { return m_FileName; }
    u32 m_00;
    ex::EXRelPtr<const char> m_FileName;    // 0x04
};
class EXGeoDebugHeader {
public:
    EXGeoDebugExtHeader *ExtDebugHeader() const { return m_ExtHeader; }
    EXGeoBaseDebug *GetDebug(unsigned int i) const;
    u32 m_NumDebug;
    u32 m_04;
    ex::EXRelPtr<EXGeoDebugExtHeader> m_ExtHeader;  // 0x08
};

float EXGeoHeader::ms_GlobalAnimTexSpeedScale = 1.0f;
signed char EXGeoFile::m_AutoLoadGroupFiles = 1;
signed char EXGeoFile::m_FailSafeLoadFiles;

EX_IMPLEMENT_DYNCREATE(EXGeoFile, EXFile)

void EXGeoHeader::Debug_OnStats(EXGeoDebugStats &) {}

unsigned int EXGeoHeader::GeoObjectStructSize(unsigned int hash) const
{
    switch ((hash >> 24) & 0x7f) {
    case 4:
    case 5:
    case 7:
    case 17:
    case 25:
    case 36:
    case 37:
        return 0x10;
    case 3:
        return sizeof(EXGeoAnimHeader);
    case 13:
        return sizeof(EXGeoAnimSkinHeader);
    case 2:
    case 9:
    case 10:
    case 20:
        return 0x14;
    case 6:
        return sizeof(EXGeoTextureHeader);
    }
    return 0;
}

EXGeoObjectArray *EXGeoHeader::GeoObjectArray(unsigned int hash) const
{
    switch ((hash >> 24) & 0x7f) {
    case 4: return (EXGeoObjectArray *)&m_Scripts;
    case 3: return (EXGeoObjectArray *)&m_Anims;
    case 13: return (EXGeoObjectArray *)&m_AnimSkins;
    case 2: return (EXGeoObjectArray *)&m_Entities;
    case 6: return (EXGeoObjectArray *)&m_Textures;
    case 5: return (EXGeoObjectArray *)&m_Maps;
    case 17: return (EXGeoObjectArray *)&m_Particles;
    case 25: return (EXGeoObjectArray *)&m_Swooshes;
    case 9: return (EXGeoObjectArray *)&m_AnimModes;
    case 10: return (EXGeoObjectArray *)&m_AnimSets;
    case 20: return (EXGeoObjectArray *)&m_SpreadSheets;
    case 7: return (EXGeoObjectArray *)&m_Fonts;
    case 36: return (EXGeoObjectArray *)&m_ForceFeedbacks;
    case 37: return (EXGeoObjectArray *)&m_Materials;
    }
    return 0;
}

void EXGeoHeader::_DeLoadSectionFile(EXGeoSectionHeader *pSection) const
{
    EXFileSys::GetFileSys()->RemoveFile(pSection->m_pFile);
    if (pSection->m_pFile) {
        delete pSection->m_pFile;
        pSection->m_pFile = 0;
    }
}

void EXGeoHeader::DeLoadDebugFile()
{
    if (m_pDebugFile) {
        EXFileSys::GetFileSys()->RemoveFile(m_pDebugFile);
        if (m_pDebugFile) {
            delete m_pDebugFile;
            m_pDebugFile = 0;
        }
    }
}

EXGeoDebugHeader *EXGeoHeader::DebugHeader() const
{
    if (m_pDebugFile && m_pDebugFile->IsLoaded()) return (EXGeoDebugHeader *)m_pDebugFile->MemData();
    return 0;
}

EXGeoBaseDebug *EXGeoHeader::GetDebug(unsigned int i) const
{
    EXGeoDebugHeader *pDebug = DebugHeader();
    if (!pDebug) return 0;
    return pDebug->GetDebug(i);
}

unsigned int EXGeoHeader::DebugFileSize() const { return m_DebugEnd - m_DebugStart; }

s8 EXGeoDebugFile::OnLoad()
{
    if (!m_pMXFile) for (;;) {}
    if (m_pMem) EXFile::OnKill();
    m_pMXFile->LoadAddr() = _SystemHeapList[1]->_EXAllocPure(m_pMXFile->Length(), 0xb);
    if (!m_pMXFile->LoadAddr()) {
        EXMemHeap::ForceCompactHeaps();
        return 0;
    }
    return EXFileSys::AsyncReadFile(m_pMXFile, m_Priority);
}

s8 EXGeoDebugFile::OnLoaded()
{
    s8 ok = EXFile::OnLoaded();
    if (ok) EXAllocSetReloc(m_pMem, 1);
    return ok;
}

EXGeoSectionFile::EXGeoSectionFile(EXGeoFile *pFile, const EXGeoSectionHeader *pHdr, short index)
{
    u32 start = pHdr->m_SeekPos;
    u32 size = (pHdr->m_End - start + 0x7ff) & ~0x7ff;
    const EXFILEINFO *pInfo = EXFileSys::GetFileSys()->GetFileInfo(pFile);
    CreateMXFile(pInfo, size, start);
    m_Index = index;
    m_DeloadTime = 0;
    m_GroupFilesLoaded = 0;
    m_ObjectsDelete = 0;
    m_pGeoFile = pFile;
}

s8 EXGeoSectionFile::OnLoad()
{
    if (!m_pMXFile) for (;;) {}
    if (m_pMXFile->LoadAddr()) OnKill();
    if (m_pMem) return 1;
    if (EXFileSys::DebugLoadInfo()) EXHashCodeFile::Debug_HashCodeName(m_Index);
    m_pMXFile->SetLength((m_pMXFile->Length() + 0x7ff) & ~0x7ff);
    m_pMXFile->LoadAddr() = _SystemHeapList[1]->_EXAllocPure(m_pMXFile->Length(), 0xb);
    if (!m_pMXFile->LoadAddr()) {
        EXMemHeap::ForceCompactHeaps();
        return 0;
    }
    return EXFileSys::AsyncReadFile(m_pMXFile, m_Priority);
}

signed char EXGeoSectionFile::AllocCallbackFunc(void *p, EXMcmds cmd)
{
    switch (cmd) {
    case 2:
        // TODO layout: the block owner holds the geo file at +0x10.
        ((EXGeoFile *)((u32 *)((EXMemBlock *)p - 1)->Owner())[4])->SetForceUpdatePtrs(1);
        return 1;
    }
    return 0;
}

EXGeoFile::EXGeoFile()
{
    m_DeloadSubFiles = 0;
    m_DeloadTime = 0;
    m_ForceUpdatePtrs = 0;
    m_pGeoHeader = 0;
    m_CheckCode = 0xffffffff;
}

s8 EXGeoFile::OnLoad()
{
    if (!m_pMXFile) for (;;) {}
    if (m_pMXFile->LoadAddr()) OnKill();
    if (m_pMem) return 1;
    if (!m_pMXFile->Length()) return 0;
    m_pMXFile->SetLength((m_pMXFile->Length() + 0x7ff) & ~0x7ff);
    m_pMXFile->LoadAddr() = _SystemHeapList[1]->_EXAllocPure(m_pMXFile->Length(), 0xb);
    if (!m_pMXFile->LoadAddr()) {
        EXMemHeap::ForceCompactHeaps();
        return 0;
    }
    return EXFileSys::AsyncReadFile(m_pMXFile, m_Priority);
}

signed char EXGeoFile::AllocCallbackFunc(void *p, EXMcmds cmd)
{
    EXGeoFile *pFile = ((EXGeoHeader *)p)->m_pGeoFile;
    switch (cmd) {
    case 2:
        pFile->m_pGeoHeader = (EXGeoHeader *)p;
        pFile->m_ForceUpdatePtrs = 1;
        return 1;
    }
    return 0;
}

signed char EXGeoFile::_DeLoadFile(signed char force)
{
    if (!force) {
        switch (m_Status) {
        case 3:
            m_DeloadTime = 4;
            m_Status = 4;
            m_pGeoHeader = 0;
            EXAllocSetReloc(m_pMem, 0);
            m_ForceUpdatePtrs = 1;
            return 1;
        case 4:
            return 1;
        }
    }
    EXFileSys::GetFileSys()->RemoveFile(this);
    return 1;
}

const char *EXGeoBaseRefObject::DebugRefName() const { return "<Ref?>"; }
const char *EXGeoBaseRefObject::DebugGeoName() const { return "<GeoFile?>"; }

void EXGeoComplexRefObject::Set(const EXGeoBaseRefObject &r)
{
    m_pGeoFile = r.m_pGeoFile;
    m_hashRef = r.m_hashRef;
    m_08 = -1;
}

void EXGeoComplexRefObject::Set(const EXGeoComplexRefObject &r)
{
    m_pGeoFile = r.m_pGeoFile;
    m_hashRef = r.m_hashRef;
    m_pGeoFile = r.m_pGeoFile;
    m_08 = r.m_08;
}

void EXGeoComplexRefObject::SetGeoFile(EXGeoFile *pFile)
{
    m_pGeoFile = pFile;
    m_08 = -1;
}

void EXGeoComplexRefObject::SetHashRef(u32 hash)
{
    m_hashRef = hash;
    m_08 = -1;
}

struct EXGeoFileLoadedList {
    u32 m_HashCode;
    EXGeoFile *m_pFile;
};

static EXAllocator<EXGeoFile> s_GeoFileAllocator(128);
static EXDataArray<EXGeoFileLoadedList, EXGeoFileLoadedList &> s_LoadedList;

void EXGeoHeader::DeloadTextureArray()
{
    if (m_pTextureArray) {
        for (u32 i = 0; i < m_NumTextures; i++) (*m_pTextureArray)[i].Unload();
        delete m_pTextureArray;
        m_pTextureArray = 0;
    }
}

void EXGeoHeader::AnimateTextureArray() const
{
    int n = m_AnimTextures.Size();
    float speed = *(float *)m_pDataBlock;
    for (int i = 0; i < n; i++) (*m_pTextureArray)[m_AnimTextures[i]].Animate(ms_GlobalAnimTexSpeedScale * speed);
}

unsigned int EXGeoHeader::GeoObjectHashIndex(unsigned int hash) const
{
    EXGeoObjectArray *pArray = GeoObjectArray(hash);
    if (pArray) {
        int i = pArray->FindIndex(hash, GeoObjectStructSize(hash));
        if (i >= 0) return (hash & 0xff000000) | i | 0x80000000;
    }
    return 0xffffffff;
}

signed char EXGeoHeader::IsSectionGroupLoaded(const EXGeoSectionFile *pFile, int index) const
{
    if (pFile->m_Status != 3) return 0;
    if (pFile->GroupFilesLoaded()) return 1;
    const ex::EXRelPtr<EXGeoSectionGroupList> &child = pFile->GeoSectionHeader()->m_ChildSections;
    EXGeoSectionGroupList *pList = child;
    if (pList) {
        for (int i = 0; i < pList->m_Count; i++) {
            u32 sec = pList->m_Sections[i];
            if (sec != (u32)index && IsSectionLoaded(sec, 1) != 3) return 0;
        }
    }
    return 1;
}

signed char EXGeoHeader::LoadDebugFile() { return 0; }

const char *EXGeoHeader::DebugFileName() const
{
    EXGeoDebugHeader *pDebug = DebugHeader();
    if (pDebug) return pDebug->ExtDebugHeader()->FileName();
    return 0;
}

signed char EXGeoFile::DeLoadGeoFile(EXGeoFile *pFile, signed char force) { return pFile->_DeLoadFile(force); }

void EXGeoFile::DeloadAllGeoFiles()
{
    for (int i = 0; i < s_LoadedList.Size(); i++) EXFileSys::GetFileSys()->RemoveFile(s_LoadedList[i].m_pFile);
}

#define EX_GEOREF_LOOKUP_GeoObjectHeader(RetType, Member) Member = pFile->GeoHeader()->GeoObjectHeader(m_hashRef);
// Retail inlines the list lookup here even though the same Find is out of line elsewhere.
template <class T> static __forceinline T *EXGeoRefFind(const EXGeoCommonArray<T> &list, u32 hash)
{
    int i = EXGeoCommonArray_FindIndex(hash, (void *)&list, sizeof(T));
    if (i < 0) return 0;
    return &list.Data()[i];
}
#define EX_GEOREF_LIST_LOOKUP(List, RetType, Member) \
    Member = EXGeoRefFind(pFile->GeoHeader()->List(), m_hashRef);
#define EX_GEOREF_LOOKUP_EntityList(RetType, Member) EX_GEOREF_LIST_LOOKUP(EntityList, RetType, Member)
#define EX_GEOREF_LOOKUP_AnimList(RetType, Member) EX_GEOREF_LIST_LOOKUP(AnimList, RetType, Member)
#define EX_GEOREF_LOOKUP_AnimSkinList(RetType, Member) EX_GEOREF_LIST_LOOKUP(AnimSkinList, RetType, Member)
#define EX_GEOREF_LOOKUP_ScriptList(RetType, Member) EX_GEOREF_LIST_LOOKUP(ScriptList, RetType, Member)
#define EX_GEOREF_LOOKUP_ParticleList(RetType, Member) EX_GEOREF_LIST_LOOKUP(ParticleList, RetType, Member)
#define EX_GEOREF_LOOKUP_SwooshList(RetType, Member) EX_GEOREF_LIST_LOOKUP(SwooshList, RetType, Member)
#define EX_GEOREF_LOOKUP_MapList(RetType, Member) EX_GEOREF_LIST_LOOKUP(MapList, RetType, Member)
#define EX_GEOREF_ACCESSORS(RetType, Method, Member, Lookup)                                   \
    RetType *EXGeoComplexRefObject::Method(const EXGeoFile *pFile, unsigned int checkCode)      \
    {                                                                                           \
        if (pFile && pFile->GeoHeader()) {                                                      \
            EX_GEOREF_LOOKUP_##Lookup(RetType, Member)                                          \
            if (Member) m_08 = checkCode;                                                       \
        } else                                                                                  \
            Member = 0;                                                                         \
        return Member;                                                                          \
    }                                                                                           \
    RetType *EXGeoComplexRefObject::Method()                                                    \
    {                                                                                           \
        if (!m_pGeoFile) {                                                                      \
            Member = 0;                                                                         \
            return 0;                                                                           \
        }                                                                                       \
        if (m_08 == (s32)m_pGeoFile->CheckCode()) return Member;                                \
        return Method(m_pGeoFile, m_pGeoFile->CheckCode());                                     \
    }                                                                                           \
    RetType *EXGeoComplexRefObject::Method() const                                              \
    {                                                                                           \
        if (m_pGeoFile) {                                                                       \
            if (m_08 == (s32)m_pGeoFile->CheckCode()) return Member;                            \
            EXGeoHeader *pHdr = m_pGeoFile->GeoHeader();                                        \
            if (pHdr) return pHdr->Method(m_hashRef);                                           \
        }                                                                                       \
        return 0;                                                                               \
    }

EX_GEOREF_ACCESSORS(EXGeoEntityHeader, GeoEntityHeader, m_pEntityHeader, EntityList)
EX_GEOREF_ACCESSORS(EXGeoObjectHeader, GeoObjectHeader, m_pObjectHeader, GeoObjectHeader)
EX_GEOREF_ACCESSORS(EXGeoAnimHeader, GeoAnimHeader, m_pAnimHeader, AnimList)
EX_GEOREF_ACCESSORS(EXGeoAnimSkinHeader, GeoAnimSkinHeader, m_pAnimSkinHeader, AnimSkinList)
EX_GEOREF_ACCESSORS(EXGeoScriptHeader, GeoScriptHeader, m_pScriptHeader, ScriptList)
EX_GEOREF_ACCESSORS(EXGeoParticleHeader, GeoParticleHeader, m_pParticleHeader, ParticleList)
EX_GEOREF_ACCESSORS(EXGeoSwooshHeader, GeoSwooshHeader, m_pSwooshHeader, SwooshList)
EX_GEOREF_ACCESSORS(EXGeoMapHeader, GeoMapHeader, m_pMapHeader, MapList)

unsigned int EXGeoHeader::GeoObjectHashCode(unsigned int hash) const
{
    EXGeoObjectHeader *pObjHdr = GeoObjectHeader(hash);
    if (!pObjHdr) return (unsigned int)-1;
    return pObjHdr->HashCode();
}

void *EXGeoHeader::GeoObject(unsigned int hash) const
{
    EXGeoObjectHeader *pObjHdr = GeoObjectHeader(hash);
    if (!pObjHdr) return 0;
    return pObjHdr->m_Addr[0].m_pObject;
}

EXTexture *EXGeoHeader::Texture(unsigned int hash) const
{
    int i = TextureList().FindIndex(hash);
    if (i < 0) return 0;
    return &(*m_pTextureArray)[i];
}

void EXGeoHeader::UpdateExternalReferences() const
{
    for (int i = 0; i < m_pDataBlock->m_ExtFiles.Size(); i++) {
        EXGeoFile *pFile = m_pDataBlock->m_ExtFiles[i];
        if (pFile && pFile->m_pGeoHeader && pFile->m_pGeoHeader != this)
            pFile->m_pGeoHeader->UpdateExternalReferenceObjects();
    }
}

EXGeoCommonObject *EXGeoHeader::RefPointer(unsigned int hash) const
{
    return RefPointerList().Find(hash)->Object(0);
}

EXGeoBaseDebug *EXGeoHeader::GetObjectDebug(unsigned int hash) const
{
    EXGeoObjectHeader *pObjHdr = GeoObjectHeader(hash);
    if (!pObjHdr) return 0;
    return GetDebug(pObjHdr);
}

EXGeoBaseDebug *EXGeoHeader::GetDebug(const EXGeoObjectHeader *pObjHdr) const
{
    return GetDebug(pObjHdr->m_Debug);
}

EXGeoFileDataBlock::EXGeoFileDataBlock(const EXGeoHeader *)
{
    // NEAR: retail pushes esi after the float store.
    m_AnimTexSpeedScale = 1.0f;
    memset(&m_MapZoneSections, 0, sizeof(m_MapZoneSections));
    m_14[0] = 0;
    m_14[1] = 0;
    m_14[2] = 0;
    m_14[3] = 0;
}

signed char EXGeoHeader::DeLoadTextSectionFile(unsigned int hash, unsigned char lang) const
{
    return DeLoadSectionFile(hash | (lang << 17));
}

EXGeoFile *EXGeoFile::LoadGeoFile(EXGeoFile *pFile, signed char wait, unsigned int priority)
{
    pFile->_LoadFile(wait, priority);
    return pFile;
}

signed char EXGeoFile::LoadGeoObjectSectionFile(unsigned int fileHash, unsigned int hash)
{
    EXGeoFile *pFile = GetGeoFile(fileHash);
    if (!pFile) return 0;
    return LoadGeoObjectSectionFile(pFile, hash);
}

void EXGeoFile::UpdatePtrs(unsigned int flags, signed char updateCheckCode)
{
    if (GeoHeader()) GeoHeader()->UpdatePtrs(flags);
    if (updateCheckCode) UpdateCheckCode();
}

signed char EXGeoHeader::LoadTextSectionFile(unsigned int hash, unsigned char lang, signed char wait, unsigned int priority)
{
    return LoadSectionFile(hash | (lang << 17), wait, priority);
}

signed char EXGeoFile::DeLoadGeoFile(unsigned int fileHash, signed char force)
{
    EXGeoFile *pFile = GetGeoFile(fileHash);
    if (!pFile) return 0;
    return pFile->_DeLoadFile(force);
}

s8 EXGeoSectionFile::OnKill()
{
    if (EXFileSys::DebugLoadInfo()) EXHashCodeFile::Debug_HashCodeName(m_Index);
    if (m_pMem && !m_ObjectsDelete) m_pGeoFile->OnDelete(m_Index);
    CleanUpMemory();
    return 1;
}

s8 EXGeoFile::OnKill()
{
    if (m_pMem)
        ((EXGeoHeader *)m_pMem)->Deload();
    else
        UpdateCheckCode();
    CleanUpMemory();
    m_pGeoHeader = 0;
    return 1;
}

void EXGeoFile::OnDelete(unsigned int section)
{
    EXGeoHeader *pHdr = GeoHeader();
    if (pHdr) {
        void *pAddr = pHdr->SectionLoadAddr(section, 0);
        pHdr->OnDelete(section, pAddr ? ((u32 *)pAddr)[1] : 0xffffffff);
    }
}

EXGeoFile *EXGeoFile::_GetGeoFile(unsigned int hash)
{
    if (!EXFileSys::GetFileSys()) return 0;
    EXFILEINFO *pInfo = EXFileSys::GetFileSys()->GetFileInfo(hash);
    if (!pInfo) return 0;
    EXGeoFile *pFile = (EXGeoFile *)pInfo->m_pFile;
    if (!pFile) {
        pFile = s_GeoFileAllocator.Get();
        if (!pFile) return 0;
        new (pFile) EXGeoFile;
        pInfo->m_pFile = pFile;
    }
    return pFile;
}

signed char EXGeoFile::LoadGeoObjectSectionFile(EXGeoFile *pFile, unsigned int hash)
{
    pFile->_LoadFile(0, 0);
    EXGeoHeader *pHdr = pFile->GeoHeader();
    if (!pHdr) return 0;
    EXGeoObjectHeader *pObjHdr = pHdr->GeoObjectHeader(hash);
    if (!pObjHdr) return 0;
    return pHdr->LoadSectionFile(pObjHdr->Section(), 0, 0);
}

s8 EXGeoFile::OnLoaded()
{
    ChangeMemOwner();
    EXAllocSetReloc(m_pMem, 1);
    EXAllocSetFunc(m_pMem, AllocCallbackFunc);
    m_DeloadTime = 0;
    m_DeloadSubFiles = 0;
    m_ForceUpdatePtrs = 0;
    m_pGeoHeader = (EXGeoHeader *)m_pMem;
    m_Status = 3;
    Initialise();
    AddFileToUpdateList();
    return 1;
}

void EXGeoHeader::UpdateTextureArray() const
{
    EXGeoTextureHeader *pTexHdr = TextureList().Data();
    for (u32 i = 0; i < m_NumTextures; i++, pTexHdr++) (*m_pTextureArray)[i].UpdatePtrs(pTexHdr, 0);
}

signed char EXGeoHeader::UpdateDeloadSectionFiles()
{
    signed char deleted = 0;
    EXGeoSectionHeader *pSections = SectionList().Data();
    for (int i = 1; i < SectionList().Size(); i++) {
        EXGeoSectionFile *pFile = pSections[i].m_pFile;
        if (pFile && pFile->m_Status == 4) {
            pFile->m_DeloadTime--;
            if (pFile->m_DeloadTime <= 0) {
                m_pGeoFile->m_DeloadSubFiles--;
                pFile->m_ObjectsDelete = 1;
                deleted = 1;
            }
        }
    }
    if (deleted) OnSectionDelete();
    return deleted;
}

EXGeoFile *EXGeoFile::LoadGeoFile(unsigned int hash, signed char wait, unsigned int priority)
{
    EXGeoFile *pFile = _GetGeoFile(hash);
    if (pFile) pFile->_LoadFile(wait, priority);
    return pFile;
}

// NEAR: retail swaps ebx/edi (this vs header); 6 variants tried.
void EXGeoFile::Initialise()
{
    EXGeoHeader *pHdr = GeoHeader();
    if (pHdr) {
        pHdr->Initialise(this);
        u32 *pInfo = (u32 *)ex::EXAbsOffset(pHdr, pHdr->SectionList()[0].m_Seek);
        UpdatePtrs(pInfo[1], 1);
        pHdr->InitialiseExternalReferences();
        _EXRealloc(m_pMem, pInfo[0]);
    }
}

void EXGeoFile::AddFileToUpdateList()
{
    EXGeoFileLoadedList entry;
    int insert = 0;
    u32 hash = GeoHeader()->HashCode();
    for (int i = 0; i < s_LoadedList.Size(); i++) {
        if (s_LoadedList.GetData()[i].m_pFile == this) return;
        if (hash > s_LoadedList.GetData()[i].m_HashCode) insert = i + 1;
    }
    entry.m_pFile = this;
    entry.m_HashCode = hash;
    s_LoadedList.SetGrowBy(64);
    s_LoadedList.InsertAt(insert, entry);
}

EXGeoFile *EXGeoFile::FailSafeLoadGeoFile(unsigned int hash, signed char wait)
{
    EXGeoFile *pFile = _GetGeoFile(hash);
    if (pFile && m_FailSafeLoadFiles) pFile->_LoadFile(wait, 0);
    return pFile;
}

EXGeoFile *EXGeoFile::GetGeoFile(unsigned int hash)
{
    EXGeoFileLoadedList *pData = s_LoadedList.GetData();
    if (pData) {
        int lo = 0;
        int hi = s_LoadedList.Size() - 1;
        int mid = (lo + hi + 1) / 2;
        while (pData[mid].m_HashCode != hash) {
            if (mid == lo) goto notFound;
            int round;
            if (pData[mid].m_HashCode > hash) {
                hi = mid;
                round = 0;
            } else {
                lo = mid;
                round = 1;
            }
            mid = (round + lo + hi) / 2;
        }
        return pData[mid].m_pFile;
    }
notFound:
    return _GetGeoFile(hash);
}

EXGeoTextItem *EXGeoHeader::FindTextItem(unsigned int hash) const
{
    EXGeoSpreadSheetHeader *pSheetHdr = SpreadSheetList().Find(0);
    int n = SpreadSheetList().Size();
    for (int i = 0; i < n; i++, pSheetHdr++) {
        if (pSheetHdr->Type() == 1) {
            EXGeoTextItem *pItem = pSheetHdr->Object(0)->FindTextItem(hash, m_pGeoFile);
            if (pItem) return pItem;
        }
    }
    return 0;
}

unsigned int EXGeoHeader::IsSectionLoaded(unsigned int section, signed char checkGroup) const
{
    unsigned int index = (section & 0xff000000) ? SectionList().FindIndex(section) : section;
    if (!index) return 3;
    if (index < (unsigned int)SectionList().Size()) {
        EXGeoSectionFile *pFile = SectionList().Data()[index].m_pFile;
        if (pFile) {
            if (checkGroup && pFile->m_Status == 3 && !IsSectionGroupLoaded(pFile, index)) return 2;
            return pFile->m_Status;
        }
    }
    return 0;
}

void *EXGeoHeader::SectionLoadAddr(unsigned int section, signed char checkGroup) const
{
    if (!section) return ex::EXAbsOffset(this, SectionList().Data()->m_Seek);
    unsigned int index = (section & 0xff000000) ? SectionList().FindIndex(section) : section;
    if (index < (unsigned int)SectionList().Size()) {
        EXGeoSectionFile *pFile = SectionList().Data()[index].m_pFile;
        if (checkGroup) {
            if (pFile && pFile->m_Status == 3 && IsSectionGroupLoaded(pFile, index)) return pFile->m_pMem;
        } else if (pFile && pFile->m_pMem)
            return pFile->m_pMem;
    }
    return 0;
}

void EXGeoHeader::Initialise(EXGeoFile *pFile)
{
    m_pGeoFile = pFile;
    if (!m_pDataBlock) m_pDataBlock = new EXGeoFileDataBlock(this);
    InitialiseTextureArray();
}

void EXGeoHeader::UpdateExternalReferenceObjects() const
{
    EXGeoTextureHeader *pTexHdr = TextureList().Data();
    for (int i = 0; i < m_ExternalRefs.Size(); i++) {
        u32 ref = m_ExternalRefs[i];
        if ((ref & 0x7f000000) == 0x06000000) {
            u32 index = ref & 0xffffff;
            (*m_pTextureArray)[index].UpdatePtrs(&pTexHdr[index], 1);
        }
    }
}

void EXGeoHeader::InitialiseTextureArray()
{
    m_NumTextures = TextureList().Size();
    if (m_NumTextures) {
        m_pTextureArray = new EXTextureArray;
        if (!m_pTextureArray->Init(m_NumTextures)) for (;;) {}
        EXGeoTextureHeader *pTexHdr = TextureList().Data();
        for (u32 i = 0; i < m_NumTextures; i++, pTexHdr++) (*m_pTextureArray)[i].SetHashCode(pTexHdr->HashCode());
    } else
        m_pTextureArray = 0;
}

void EXGeoHeader::OnSectionDelete()
{
    ParticleList().OnSectionDelete(this);
    ScriptList().OnSectionDelete(this);
    MapList().OnSectionDelete(this);
    AnimSkinList().OnSectionDelete(this);
    EntityList().OnSectionDelete(this);
    AnimList().OnSectionDelete(this);
    RefPointerList().OnSectionDelete(this);
    EXGeoSectionHeader *pSections = SectionList().Data();
    for (int i = 1; i < SectionList().Size(); i++) {
        if (pSections[i].m_pFile && pSections[i].m_pFile->m_ObjectsDelete) _DeLoadSectionFile(&pSections[i]);
    }
}

// TODO name: retail reloads the file pointer here, as if through a helper taking the section.
static inline void EXGeoDeleteSectionFile(EXGeoSectionHeader *pSection)
{
    if (pSection->m_pFile) {
        delete pSection->m_pFile;
        pSection->m_pFile = 0;
    }
}

signed char EXGeoHeader::_DeLoadSectionFiles()
{
    signed char deloaded = 0;
    signed char objectsDelete = 0;
    EXGeoSectionHeader *pSections = SectionList().Data();
    for (int i = 1; i < SectionList().Size(); i++) {
        EXFile *pFile = pSections[i].File();
        if (pFile) {
            switch (pFile->Status()) {
            case 3:
            case 4:
                ((EXGeoSectionFile *)pFile)->SetObjectsDelete(1);
                objectsDelete = 1;
                deloaded = 1;
                break;
            default:
                _DeLoadSectionFile(&pSections[i]);
                deloaded = 1;
                break;
            case 0:
                EXGeoDeleteSectionFile(&pSections[i]);
                break;
            }
        }
    }
    if (objectsDelete) OnSectionDelete();
    return deloaded;
}

void EXGeoFile::UpdateFiles(signed char updateRefs)
{
    int i;
    for (i = s_LoadedList.Size() - 1; i >= 0; i--) {
        EXGeoFile *pFile = s_LoadedList.GetData()[i].m_pFile;
        if (!pFile->m_Status)
            s_LoadedList.RemoveAt(i);
        else
            pFile->Update(updateRefs);
    }
    if (updateRefs) {
        for (i = s_LoadedList.Size() - 1; i >= 0; i--) {
            EXGeoFile *pFile = s_LoadedList.GetData()[i].m_pFile;
            if ((pFile->m_Flags.m_Flags & 2) && pFile->m_pGeoHeader) pFile->m_pGeoHeader->UpdateExternalReferences();
        }
    }
}

void EXGeoHeader::SectionGroupUsage(unsigned int &usage, unsigned int section, EXu32Array &done) const
{
    EXGeoSectionFileHeader *pSecHdr = (EXGeoSectionFileHeader *)SectionLoadAddr(section, 0);
    if (pSecHdr) {
        usage |= pSecHdr->UsageFlags();
        EXGeoSectionGroupList *pList = pSecHdr->ParentSectionList();
        if (pList) {
            done.Add(section);
            for (int i = 0; i < pList->m_Count; i++) {
                u32 sec = pList->m_Sections[i];
                int n = done.Size();
                int j;
                for (j = 0; j < done.Size(); j++)
                    if (done.GetData()[j] == sec) break;
                if (j == n) SectionGroupUsage(usage, sec, done);
            }
        }
    }
}

signed char EXGeoHeader::DeLoadSectionFile(unsigned int section) const
{
    unsigned int index = section;
    if (section & 0xff000000) index = SectionList().FindIndex(section);
    if (index > 0) {
        unsigned int status = IsSectionLoaded(index, 0);
        if (status) {
            EXGeoSectionHeader *pSection = &SectionList().Data()[index];
            if (pSection) {
                EXGeoSectionFile *pFile = pSection->m_pFile;
                switch (status) {
                case 3:
                    pFile->m_DeloadTime = 4;
                    pFile->m_Status = 4;
                    EXAllocSetReloc(pFile->m_pMem, 0);
                    m_pGeoFile->m_DeloadSubFiles++;
                    m_pGeoFile->m_ForceUpdatePtrs = 1;
                    break;
                case 4:
                    break;
                default:
                    EXFileSys::GetFileSys()->RemoveFile(pFile);
                    return 1;
                }
            }
        }
    }
    return 1;
}

EXGeoObjectHeader *EXGeoHeader::GeoObjectHeader(unsigned int hash) const
{
    switch ((hash >> 24) & 0x7f) {
    case 4: return (EXGeoObjectHeader *)GeoScriptHeader(hash);
    case 3: return (EXGeoObjectHeader *)GeoAnimHeader(hash);
    case 13: return (EXGeoObjectHeader *)GeoAnimSkinHeader(hash);
    case 2: return (EXGeoObjectHeader *)GeoEntityHeader(hash);
    case 6: return (EXGeoObjectHeader *)GeoTextureHeader(hash);
    case 5: return (EXGeoObjectHeader *)GeoMapHeader(hash);
    case 17: return (EXGeoObjectHeader *)GeoParticleHeader(hash);
    case 25: return (EXGeoObjectHeader *)GeoSwooshHeader(hash);
    case 9: return (EXGeoObjectHeader *)GeoAnimModeHeader(hash);
    case 10: return (EXGeoObjectHeader *)GeoAnimSetHeader(hash);
    case 20: return (EXGeoObjectHeader *)GeoSpreadSheetHeader(hash);
    case 7: return (EXGeoObjectHeader *)GeoFontHeader(hash);
    case 36: return (EXGeoObjectHeader *)GeoForceFeedbackHeader(hash);
    case 37: return (EXGeoObjectHeader *)GeoMaterialHeader(hash);
    }
    return 0;
}

void EXGeoHeader::GetAnimMaxBoundsBox(EXBoundsBox &box) const
{
    box.SetMaxRange();
    for (int i = 0; i < AnimList().Size(); i++) {
        EXGeoAnimHeader *pAnimHdr = AnimList().Find(i);
        if (pAnimHdr && pAnimHdr->m_Addr[0].m_pObject) {
            EXBoundsBox b;
            b.Set(((EXGeoCommonObject *)pAnimHdr->m_Addr[0].m_pObject)->GetBoundsBox());
            box.Accumulate(b);
        }
    }
}

// Retail bounds the loop by the anim count, not the script count.
void EXGeoHeader::GetScriptMaxBoundsBox(EXBoundsBox &box) const
{
    box.SetMaxRange();
    for (int i = 0; i < AnimList().Size(); i++) {
        EXGeoScriptHeader *pScriptHdr = ScriptList().Find(i);
        if (pScriptHdr && pScriptHdr->m_Addr[0].m_pObject) {
            EXGeoScript *pScript = pScriptHdr->m_Addr[0].m_pObject;
            EXBoundsBox b;
            b.Set(((EXGeoCommonObject *)pScript)->GetBoundsBox());
            box.Accumulate(b);
        }
    }
}

void EXGeoHeader::DeLoadSectionFiles()
{
    if (_DeLoadSectionFiles()) {
        RefPointerList().UpdatePtrs(this, 1);
        AnimList().UpdatePtrs(this, 2);
        EntityList().UpdatePtrs(this, 1);
        AnimSkinList().UpdatePtrs(this, 1);
        MapList().UpdatePtrs(this, 1);
        ScriptList().UpdatePtrs(this, 1);
        ParticleList().UpdatePtrs(this, 1);
        TextureList().SetPtrs(this);
        MaterialList().SetPtrs(this);
        SwooshList().SetPtrs(this);
        AnimModeList().SetPtrs(this);
        AnimSetList().SetPtrs(this);
        FontList().SetPtrs(this);
        SpreadSheetList().SetPtrs(this);
        ForceFeedbackList().SetPtrs(this);
        UpdateTextureArray();
        m_pGeoFile->UpdateCheckCode();
    }
}

// NEAR: retail loads the section seek before the split offset and tests the flag with jns.
s8 EXGeoSectionFile::OnLoaded()
{
    if (EXFileSys::DebugLoadInfo()) EXHashCodeFile::Debug_HashCodeName(m_Index);
    ChangeMemOwner();
    EXAllocSetReloc(m_pMem, 1);
    EXAllocSetFunc(m_pMem, AllocCallbackFunc);
    m_Status = 3;
    m_DeloadTime = 0;
    if (m_pGeoFile) {
        EXGeoHeader *pHdr = m_pGeoFile->GeoHeader();
        if (pHdr) {
            EXu32Array done;
            EXGeoSectionFileHeader *pSecHdr = (EXGeoSectionFileHeader *)m_pMem;
            pHdr->SectionGroupUsage(pSecHdr->UsageFlags(), m_Index, done);
            m_pGeoFile->UpdatePtrs(pSecHdr->UsageFlags(), 0);
            EXGeoSectionHeader *pSection = pHdr->SectionHeader(m_Index);
            _EXRealloc(m_pMem, pSecHdr->m_SplitOffset.m_Offset - pSection->m_SeekPos);
            if ((s8)pHdr->m_Flags < 0) pHdr->UpdateSectionFiles(1);
        }
    }
    return 1;
}

void EXGeoHeader::Deload()
{
    _DeLoadSectionFiles();
    ForceFeedbackList().DeletePtrs(this, 0);
    SpreadSheetList().DeletePtrs(this, 0);
    FontList().DeletePtrs(this, 0);
    AnimSetList().DeletePtrs(this, 0);
    AnimModeList().DeletePtrs(this, 0);
    SwooshList().DeletePtrs(this, 0);
    MaterialList().DeletePtrs(this, 0);
    TextureList().DeletePtrs(this, 0);
    ParticleList().OnDelete(this, 0);
    ScriptList().OnDelete(this, 0);
    MapList().OnDelete(this, 0);
    AnimSkinList().OnDelete(this, 0);
    EntityList().OnDelete(this, 0);
    AnimList().OnDelete(this, 0);
    RefPointerList().OnDelete(this, 0);
    DeLoadDebugFile();
    DeloadTextureArray();
    m_pGeoFile->UpdateCheckCode();
    UpdateExternalReferences();
    if (m_pDataBlock) {
        delete m_pDataBlock;
        m_pDataBlock = 0;
    }
}

// TODO: stands in for ex::EXFlag<u8>::Set (EXFile::m_Flags).
static inline void EXGeoSetFlag(EXFileFlags &f, u8 b) { f.m_Flags |= b; }

// NEAR: scratch registers rotate (eax/ecx/edx) around the AddUnique calls; 6 variants tried.
void EXGeoHeader::InitialiseExternalReferences()
{
    EXPtrArray<EXGeoHeader> headers;
    int i;
    if (m_ExternalFiles.Size()) {
        EXGeoSetFlag(m_pGeoFile->m_Flags, 2);
        for (i = 0; i < m_ExternalFiles.Size(); i++) {
            EXGeoFile *pFile = EXGeoFile::GetGeoFile(m_ExternalFiles[i]);
            if (pFile) {
                EXGeoSetFlag(pFile->m_Flags, 1);
                if (pFile->m_pGeoHeader) {
                    EXGeoHeader *pHdr = pFile->m_pGeoHeader;
                    pHdr->m_pDataBlock->m_ExtFiles.AddUnique(m_pGeoFile);
                    headers.AddUnique(pHdr);
                }
            }
        }
    }
    if (m_pGeoFile->m_Flags.m_Flags & 1) {
        for (i = 0; i < s_LoadedList.Size(); i++) {
            EXGeoFile *pFile = s_LoadedList.GetData()[i].m_pFile;
            if (pFile) {
                EXGeoHeader *pHdr = pFile->m_pGeoHeader;
                if (pHdr) {
                    for (int j = 0; j < pHdr->m_ExternalFiles.Size(); j++) {
                        if (pHdr->m_ExternalFiles[j] == m_HashCode) {
                            headers.AddUnique(pHdr);
                            m_pDataBlock->m_ExtFiles.AddUnique(pFile);
                        }
                    }
                }
            }
        }
    }
    for (i = 0; i < headers.Size(); i++) headers[i]->UpdateExternalReferenceObjects();
}

void EXGeoHeader::UpdatePtrs(unsigned int flags)
{
    if (flags & 0x1000) RefPointerList().UpdatePtrs(this, 1);
    if (flags & 0x10) AnimList().UpdatePtrs(this, 2);
    if (flags & 0x1) EntityList().UpdatePtrs(this, 1);
    if (flags & 0x20) AnimSkinList().UpdatePtrs(this, 1);
    if (flags & 0x2) MapList().UpdatePtrs(this, 1);
    if (flags & 0x8) ScriptList().UpdatePtrs(this, 1);
    if (flags & 0x40) ParticleList().UpdatePtrs(this, 1);
    if (flags & 0x4) TextureList().SetPtrs(this);
    if (flags & 0x8000) MaterialList().SetPtrs(this);
    if (flags & 0x800) SwooshList().SetPtrs(this);
    if (flags & 0x80) AnimModeList().SetPtrs(this);
    if (flags & 0x100) AnimSetList().SetPtrs(this);
    if (flags & 0x400) FontList().SetPtrs(this);
    if (flags & 0x200) SpreadSheetList().SetPtrs(this);
    if (flags & 0x4000) ForceFeedbackList().SetPtrs(this);
    UpdateTextureArray();
}

void EXGeoHeader::OnDelete(unsigned int section, unsigned int flags)
{
    if (section & 0xff000000) section = SectionList().FindIndex(section);
    if (flags & 0x4000) ForceFeedbackList().DeletePtrs(this, section);
    if (flags & 0x200) SpreadSheetList().DeletePtrs(this, section);
    if (flags & 0x400) FontList().DeletePtrs(this, section);
    if (flags & 0x100) AnimSetList().DeletePtrs(this, section);
    if (flags & 0x80) AnimModeList().DeletePtrs(this, section);
    if (flags & 0x800) SwooshList().DeletePtrs(this, section);
    if (flags & 0x8000) MaterialList().DeletePtrs(this, section);
    if (flags & 0x4) TextureList().DeletePtrs(this, section);
    if (flags & 0x40) ParticleList().OnDelete(this, section);
    if (flags & 0x8) ScriptList().OnDelete(this, section);
    if (flags & 0x2) MapList().OnDelete(this, section);
    if (flags & 0x20) AnimSkinList().OnDelete(this, section);
    if (flags & 0x1) EntityList().OnDelete(this, section);
    if (flags & 0x10) AnimList().OnDelete(this, section);
    if (flags & 0x1000) RefPointerList().OnDelete(this, section);
}

// NEAR: retail forms the queue/load result as xor ecx / setne cl / mov al,cl; 7 variants tried.
signed char EXGeoFile::_LoadFile(signed char async, unsigned int priority)
{
    switch (m_Status) {
    case 3:
        return 1;
    case 4:
        m_Status = 3;
        m_pGeoHeader = (EXGeoHeader *)m_pMem;
        EXAllocSetReloc(m_pMem, 1);
        if (m_pGeoHeader) {
            EXGeoHeader *pHdr = m_pGeoHeader;
            pHdr->RefPointerList().UpdatePtrs(pHdr, 1);
            pHdr->AnimList().UpdatePtrs(pHdr, 2);
            pHdr->EntityList().UpdatePtrs(pHdr, 1);
            pHdr->AnimSkinList().UpdatePtrs(pHdr, 1);
            pHdr->MapList().UpdatePtrs(pHdr, 1);
            pHdr->ScriptList().UpdatePtrs(pHdr, 1);
            pHdr->ParticleList().UpdatePtrs(pHdr, 1);
            pHdr->TextureList().SetPtrs(pHdr);
            pHdr->MaterialList().SetPtrs(pHdr);
            pHdr->SwooshList().SetPtrs(pHdr);
            pHdr->AnimModeList().SetPtrs(pHdr);
            pHdr->AnimSetList().SetPtrs(pHdr);
            pHdr->FontList().SetPtrs(pHdr);
            pHdr->SpreadSheetList().SetPtrs(pHdr);
            pHdr->ForceFeedbackList().SetPtrs(pHdr);
            pHdr->UpdateTextureArray();
        }
        UpdateCheckCode();
        return 1;
    case 0: {
        EXFILEINFO *pInfo = EXFileSys::GetFileSys()->GetFileInfo(this);
        CreateMXFile(pInfo, pInfo->m_Size, 0);
        if (async) return EXFileSys::QueueFile(this, priority) != 0;
        return EXFileSys::LoadFile(this) != 0;
    }
    case 2:
        if (!async) EXFileSys::WaitSyncOnFile(this, 1);
        break;
    }
    return 0;
}

// NEAR: retail moves the "no file yet" (status = 0) arm out of line after the switch; 4 variants tried.
signed char EXGeoHeader::LoadSectionFile(unsigned int section, signed char async, unsigned int priority)
{
    if (section & 0xff000000) section = SectionList().FindIndex(section);
    if (!section) return 1;
    if (section >= (unsigned int)SectionList().Size()) return 0;
    EXGeoSectionHeader *pSection = &SectionList().Data()[section];
    EXGeoSectionFile *pFile = pSection->m_pFile;
    unsigned int status;
    if (pFile) {
        status = pFile->Status();
        if (status == 3) return 1;
        pFile->m_GroupFilesLoaded = 0;
    } else
        status = 0;
    switch (status) {
    case 4:
        pFile->m_Status = 3;
        RefPointerList().UpdatePtrs(this, 1);
        AnimList().UpdatePtrs(this, 2);
        EntityList().UpdatePtrs(this, 1);
        AnimSkinList().UpdatePtrs(this, 1);
        MapList().UpdatePtrs(this, 1);
        ScriptList().UpdatePtrs(this, 1);
        ParticleList().UpdatePtrs(this, 1);
        TextureList().SetPtrs(this);
        MaterialList().SetPtrs(this);
        SwooshList().SetPtrs(this);
        AnimModeList().SetPtrs(this);
        AnimSetList().SetPtrs(this);
        FontList().SetPtrs(this);
        SpreadSheetList().SetPtrs(this);
        ForceFeedbackList().SetPtrs(this);
        UpdateTextureArray();
        EXAllocSetReloc(pFile->m_pMem, 1);
        m_pGeoFile->m_DeloadSubFiles--;
        return 1;
    case 2:
        if (!async) EXFileSys::WaitSyncOnFile(pFile, 1);
        break;
    case 0:
        if (!pFile) {
            pFile = new EXGeoSectionFile(m_pGeoFile, pSection, (short)section);
            if (!pFile) return 0;
            pSection->m_pFile = pFile;
        }
        if (async) return EXFileSys::QueueFile(pFile, priority) != 0;
        return EXFileSys::LoadFile(pFile) != 0;
    }
    return 0;
}

signed char EXGeoHeader::UpdateSectionFiles(signed char async)
{
    for (int i = 1; i < SectionList().Size(); i++) {
        EXGeoSectionFileHeader *pSecHdr = (EXGeoSectionFileHeader *)SectionLoadAddr(i, 0);
        if (pSecHdr) {
            int j;
            if (!SectionHeader(i)->m_pFile->m_GroupFilesLoaded) {
                EXGeoSectionGroupList *pChildren = pSecHdr->ChildSectionList();
                if (pChildren) {
                    int n = pChildren->m_Count;
                    for (j = 0; j < n; j++) {
                        u32 sec = pChildren->m_Sections[j];
                        u32 status = IsSectionLoaded(sec, 1);
                        if (status != 3 && status != 2) LoadSectionFile(sec, async, 0);
                    }
                }
            }
            if (EXGeoFile::AutoLoadGroupFiles()) {
                EXGeoSectionGroupList *pParents = pSecHdr->ParentSectionList();
                if (pParents) {
                    int unloaded = 0;
                    int n = pParents->m_Count;
                    for (j = 0; j < n; j++)
                        if (!IsSectionLoaded(pParents->m_Sections[j], 0)) unloaded++;
                    if (unloaded == n) DeLoadSectionFile(i);
                }
            }
        }
    }
    return 0;
}

// Re-resolve every object list after the header moved (same sequence as EXGeoHeader::UpdatePtrs
// with all flags set). TODO name: retail inlines this in several places.
static __forceinline void EXGeoUpdateAllPtrs(EXGeoHeader *pHdr)
{
    pHdr->RefPointerList().UpdatePtrs(pHdr, 1);
    pHdr->AnimList().UpdatePtrs(pHdr, 2);
    pHdr->EntityList().UpdatePtrs(pHdr, 1);
    pHdr->AnimSkinList().UpdatePtrs(pHdr, 1);
    pHdr->MapList().UpdatePtrs(pHdr, 1);
    pHdr->ScriptList().UpdatePtrs(pHdr, 1);
    pHdr->ParticleList().UpdatePtrs(pHdr, 1);
    pHdr->TextureList().SetPtrs(pHdr);
    pHdr->MaterialList().SetPtrs(pHdr);
    pHdr->SwooshList().SetPtrs(pHdr);
    pHdr->AnimModeList().SetPtrs(pHdr);
    pHdr->AnimSetList().SetPtrs(pHdr);
    pHdr->FontList().SetPtrs(pHdr);
    pHdr->SpreadSheetList().SetPtrs(pHdr);
    pHdr->ForceFeedbackList().SetPtrs(pHdr);
    pHdr->UpdateTextureArray();
}

void EXGeoFile::Update(signed char ptrsOnly)
{
    unsigned int status = Status();
    if (ptrsOnly) {
        if (status == 3 && m_ForceUpdatePtrs) {
            if (m_pGeoHeader) EXGeoUpdateAllPtrs(m_pGeoHeader);
            UpdateCheckCode();
            m_ForceUpdatePtrs = 0;
        }
        return;
    }
    if (status == 4) {
        if (--m_DeloadTime <= 0) EXFileSys::GetFileSys()->RemoveFile(this);
        return;
    }
    if (status == 3) {
        signed char update = m_ForceUpdatePtrs;
        EXGeoHeader *pHdr = m_pGeoHeader;
        if (pHdr) {
            if (m_DeloadSubFiles) {
                if (pHdr->UpdateDeloadSectionFiles()) update = 1;
            } else if ((pHdr->m_Flags >> 7) & 1)
                pHdr->UpdateSectionFiles(1);
        }
        if (update) {
            if (m_pGeoHeader) EXGeoUpdateAllPtrs(m_pGeoHeader);
            m_ForceUpdatePtrs = 0;
        }
        if (pHdr) pHdr->AnimateTextureArray();
    }
}
