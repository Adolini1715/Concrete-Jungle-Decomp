#include "EXFile.h"
#include "EXFileSys.h"
#include "EXMemHeap.h"
#include "EXBaseTextFile.h"
#include "EXArray.h"
#include "EXBaseSoundManager.h"

// Root runtime class with a factory (EXRuntimeClass.h has no ROOT+DYNCREATE macro yet).
EXRuntimeClass *EXFile::_GetBaseClass() { return EXRUNTIME_CLASS(EXFile); }
const EXRuntimeClass EXFile::classEXFile = { 0, "EXFile", sizeof(EXFile), (void *(__cdecl *)())EXFile::CreateObject };

namespace EXF {
EXFixedArray<const char *, 9> Debug_FileStatus = {
    "NotLoaded", "Queued", "Loading", "Loaded", "DeLoading", "Failed", "NotFound", "Stopping", "Stopped"
};
}

const char *EXFileNotInit = "EXFile Not initialised";

void MXFile::Initialise(u32 length, u32 seekPos)
{
    for (int i = 0; i < 4; i++) {
        m_SysFile[i].m_pName = 0;
        m_SysFile[i].m_Offset = 0;
    }
    m_Length = length;
    m_SeekPos = seekPos;
    m_FileHandle = 0;
    m_pLoadAddr = 0;
}

void MXFile::SetFileHandle(u32 type)
{
    m_FileHandle = (EXFileSys::GetFileSys()->GetUniqueID() << 8) | type;
}

void EXFile::DeleteMXFile()
{
    if (m_pMXFile) {
        delete m_pMXFile;
        m_pMXFile = 0;
    }
}

void EXFile::CleanUpMemory()
{
    if (m_pMXFile && m_pMXFile->LoadAddr()) {
        EXFree(m_pMXFile->LoadAddr());
        m_pMXFile->LoadAddr() = 0;
    }
    if (m_pMem) {
        EXFree(m_pMem);
        m_pMem = 0;
    }
}

void *EXFile::MemDetach()
{
    void *p = m_pMem;
    m_pMem = 0;
    return p;
}

s8 EXFile::OnKill()
{
    CleanUpMemory();
    return 1;
}

s8 EXFile::GetFileLoadedStatus() const
{
    if (m_pMXFile) {
        u32 handle = m_pMXFile->FileHandle();
        u32 status = EXBaseSoundManager::GetSoundManager()->GetFileStatus(handle & 0xff);
        if (((status ^ handle) & 0xffffff00) == 0 && (u8)status == 0x83) return 1;
    }
    return 0;
}

EXFile::~EXFile()
{
    EXFileSys::GetFileSys()->RemoveFile(this);
    CleanUpMemory();
    DeleteMXFile();
}

void EXFile::Initalise()
{
    m_Link.Init();
    m_pMXFile = 0;
    m_pMem = 0;
    m_Priority = 0;
    m_Flags.m_Flags = 0;
    m_pFilename = EXFileNotInit;
    m_Status = 0;
}

void EXFile::ChangeMemOwner()
{
    if (m_pMXFile) EXAllocSetOwner(m_pMXFile->LoadAddr(), &m_pMem);
    DeleteMXFile();
}

void EXFile::CreateMXFile(const EXFILEINFO *pInfo, u32 length, u32 seekPos)
{
    DeleteMXFile();
    m_pMXFile = new MXFile(length, seekPos);
    if (!m_pMXFile) for (;;) {}
    EXFileSys::GetFileSys()->GetSysFileInfo(m_pMXFile, pInfo->m_pName);
    m_pFilename = pInfo->m_pName;
}

void EXFile::CreateMXFile(const char *pName, u32 length, u32 seekPos)
{
    DeleteMXFile();
    m_pMXFile = new MXFile(length, seekPos);
    if (!m_pMXFile) for (;;) {}
    EXFileSys::GetFileSys()->GetSysFileInfo(m_pMXFile, pName);
    m_pFilename = pName;
}

// NEAR: 28 bytes differ, register choice/scheduling around the length store before the heap alloc.
s8 EXFile::OnLoad(EXMemHeap *pHeap)
{
    if (!m_pMXFile) for (;;) {}
    if (m_pMem || m_pMXFile->LoadAddr()) EXFile::OnKill();
    u32 len = m_pMXFile->Length();
    if (!len) {
        EXFILEINFO *pInfo = EXFileSys::GetFileSys()->GetFileInfo(m_pFilename);
        if (!pInfo || !(len = pInfo->m_Size)) {
            len = m_pMXFile->GetLength();
            if (!len) {
                m_Status = 6;
                return 0;
            }
        }
    }
    m_pMXFile->SetLength((len + 0x7ff) & ~0x7ff);
    void *pMem = pHeap->_EXAlloc(m_pMXFile->Length(), 0xb);
    m_pMXFile->LoadAddr() = pMem;
    if (!m_pMXFile->LoadAddr()) return 0;
    return EXFileSys::AsyncReadFile(m_pMXFile, m_Priority);
}

// NEAR: retail constructs the two locals with (non-unrolled) loops and keeps the unused
// EXFILEINFO; ours elides it. Frame layout matches (EXFILEINFO 0x3c + MXFile 0x40).
int EXFile::GetFilelength() const
{
    EXFILEINFO *pInfo = EXFileSys::GetFileSys()->GetFileInfo(m_pFilename);
    if (pInfo) return pInfo->m_Size;
    EXFILEINFO info;
    MXFile file(0, 0);
    EXFileSys::GetFileSys()->GetSysFileInfo(&file, m_pFilename);
    return file.GetLength();
}

s8 EXFile::OnLoad()
{
    return OnLoad(_SystemHeapList[0]);
}

s8 EXFile::OnLoaded()
{
    ChangeMemOwner();
    m_Status = 3;
    return 1;
}

EXFile::EXFile()
{
    Initalise();
}

EXFile::EXFile(const EXFILEINFO *pInfo, u32 length, u32 seekPos)
{
    Initalise();
    CreateMXFile(pInfo, length, seekPos);
}

EXFile::EXFile(const char *pName, u32 length, u32 seekPos)
{
    Initalise();
    CreateMXFile(pName, length, seekPos);
}

EXFile *EXFile::CreateObject()
{
    return new EXFile;
}

EXBaseTextFile::EXBaseTextFile(u32 bufSize)
{
    m_pBuffer = (u8 *)_EXAlloc(bufSize, 2);
    m_BufSize = m_pBuffer ? bufSize : 0;
    m_FilePos = 0;
    m_SeekPos = 0;
    m_Dirty = 0;
    m_BufPos = 0;
    m_BufLen = 0;
}

EXBaseTextFile::~EXBaseTextFile()
{
    if (m_pBuffer) EXFree(m_pBuffer);
}
