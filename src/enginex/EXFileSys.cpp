#include "EXFileSys.h"
#include "EXFile.h"
#include "EXMemHeap.h"
#include "EXBaseSoundManager.h"
#include "EXWatcher.h"
#include "EXString.h"
#include "EXMemory.h"

extern "C" int __cdecl sprintf(char *, const char *, ...);
extern "C" int __cdecl _stricmp(const char *, const char *);
extern "C" void __cdecl qsort(void *, unsigned int, unsigned int, int (__cdecl *)(const void *, const void *));

// TODO layout: minimal stand-in for a class outside this area.
class EXGeoFile {
public:
    static void UpdateFiles(s8 force);
};

EXFileSys *EXFileSys::m_pFileSys;
s8 EXFileSys::m_CdRomBuild;

s8 EXFileSys::Init(EXFileSys *pFileSys)
{
    m_pFileSys = pFileSys;
    return 1;
}

void EXFileSys::UpdateFileLists() const {}

void EXFileSys::GetFileListFileName(char *pOut, const char *pName)
{
    sprintf(pOut, "%s", pName);
}

// NEAR: switch layout differs (retail jump table order / status reload).
void EXFileSys::WaitForFileCompletion(const EXFile *pFile) const
{
    for (;;) {
        switch (pFile->m_Status) {
        case 2: {
            u32 handle = pFile->m_pMXFile->FileHandle();
            u32 type = handle & 0xff;
            if (((EXBaseSoundManager::GetSoundManager()->GetFileStatus(type) ^ handle) & 0xffffff00) == 0
                && (EXBaseSoundManager::GetSoundManager()->GetFileStatus(type) & 0xf) == 3) return;
            break;
        }
        case 0: case 1: case 3: case 4: case 5:
            return;
        }
        EXBaseSoundManager::GetSoundManager()->CommandUpdate(1);
    }
}

// NEAR: loop structure differs (retail: switch-like sub/je on status, tail-jump to UpdateAllHeaps).
void EXFileSys::WaitSyncOnFile(const EXFile *pFile, s8 updateHeaps)
{
    EXFileSys *pSys = m_pFileSys;
    pSys->Poll();
    EXBaseSoundManager::GetSoundManager()->CommandUpdate(1);
    if (pSys->m_Status != 2) {
        for (;;) {
            if (pFile->m_Status == 3 || pFile->m_Status == 5) return;
            pSys->Poll();
            EXBaseSoundManager::GetSoundManager()->CommandUpdate(1);
            if (pSys->m_Status == 2) break;
        }
    }
    if (updateHeaps) EXMemHeap::UpdateAllHeaps(1, 0);
}

void EXFileSys::UpdateLoadedFiles(s8 force) const
{
    EXGeoFile::UpdateFiles(force);
}

void EXFileSys::Update()
{
    Poll();
    EXGeoFile::UpdateFiles(0);
}

s8 EXFileSys::GetLoadingStatus() const
{
    return (m_QueuedList.Count() + (m_Status != 0) + m_ActiveCount) ? 1 : 0;
}

int EXFileInfoArray::QSortFn_SortByName(const void *a, const void *b)
{
    return strcmp(((const EXFILEINFO *)a)->m_pName, ((const EXFILEINFO *)b)->m_pName);
}

int EXFileInfoArray::QSortFn_SortByHash(const void *a, const void *b)
{
    u32 ha = ((const EXFILEINFO *)a)->m_Hash;
    u32 hb = ((const EXFILEINFO *)b)->m_Hash;
    if (hb > ha) return -1;
    return hb < ha;
}

// NEAR: control flow of the sound-manager status test differs.
void EXFileSys::RemoveFile(EXFile *pFile)
{
    if (!this) return;
    u32 i;
    for (i = 0; i < 16; i++) {
        if (m_pActive[i] == pFile) break;
    }
    if (i < 16) {
        u32 handle = pFile->m_pMXFile->FileHandle();
        if ((((EXBaseSoundManager::GetSoundManager()->GetFileStatus(i) ^ handle) & 0xffffff00) == 0
             && ((EXBaseSoundManager::GetSoundManager()->GetFileStatus(i) & 0xf) == 1
                 || (EXBaseSoundManager::GetSoundManager()->GetFileStatus(i) & 0xf) == 2))
            || pFile->m_Status == 2) {
            EXBaseSoundManager::GetSoundManager()->AsyncStopFileRead(pFile->m_pMXFile->FileHandle());
            WaitForFileCompletion(pFile);
            m_pActive[i]->m_Status = 0;
        }
        m_ActiveCount--;
        m_pActive[i] = 0;
    }
    pFile->m_Status = 0;
    pFile->OnKill();
    if (m_QueuedList.IsMember(&pFile->m_Link)) m_QueuedList.Remove(&pFile->m_Link);
    if (m_LoadedList.IsMember(&pFile->m_Link)) m_LoadedList.Remove(&pFile->m_Link);
}

void EXFileSys::WaitSync(s8 updateHeaps)
{
    EXFileSys *pSys = m_pFileSys;
    do {
        pSys->Poll();
        EXBaseSoundManager::GetSoundManager()->CommandUpdate(1);
        if (pSys->m_Status == 2 && updateHeaps) EXMemHeap::UpdateAllHeaps(1, 0);
    } while (pSys->m_QueuedList.Count() + (pSys->m_Status != 0) + pSys->m_ActiveCount);
}

// Loads a file list ("Filelist.bin"): one entry per game file with its size, hash and the offsets of
// its four disc copies. The names are stored obfuscated (each char shifted by 0x16 - entry - index).
EXFileList::EXFileList(const EXFILE &file) : m_DiscNames(0, -1)
{
    m_File = file;
    m_pData = 0;
    m_Unk34[0] = 0;

    EXStringArray parts(0, -1);
    EXString name(file.m_pName);
    EXFILEINFO info;
    info.m_pName = name;
    info.m_Size = file.m_Size;
    EXFile listFile(&info, file.m_Size, file.m_SeekPos);
    if (!EXFileSys::GetFileSys()->OnLoadFile(&listFile)) return;

    char *pHdr = (char *)listFile.m_pMem;
    if (!pHdr) return;
    if (*(int *)pHdr != 7) for (;;) {}

    // Disc file names: <list name without extension>.000, .001 ...
    parts.RemoveAll();
    EXString upper = name;
    upper.ToUpper();
    upper.Tokenise(parts, EXString("."), 0);
    int nDiscs = *(unsigned short *)(pHdr + 0xe);
    int i = 0;
    do {
        EXString discName;
        discName.Format("%s.%3.3d", parts[0].c_str(), i);
        m_DiscNames.Add(discName);
        i++;
    } while (i < nDiscs + 1);

    int count = *(int *)(pHdr + 8);
    m_Files.SetSize(count, -1);
    m_Unk34[1] = nDiscs;

    int nameTable = *(int *)(pHdr + 0x10);
    if (nameTable) nameTable += 0x10 + (int)pHdr;

    // First pass: de-obfuscate the names and total their length.
    int nameBytes = 0;
    for (int e = 0; e < count; e++) {
        char *pStr = (char *)(*(int *)(nameTable + e * 4) + nameTable + e * 4);
        int k = 0;
        char *pc;
        do {
            pStr[k] = pStr[k] + (char)((0x16 - e) - k);
            pc = pStr + k;
            k++;
        } while (*pc != 0);
        char *pEnd = pStr;
        while (*pEnd++ != 0) {}
        m_Files[e].m_Hash = 0xfffffffe;
        nameBytes += (int)(pEnd - (pStr + 1));
    }

    char *pPool = (char *)_EXAlloc(nameBytes, 0);
    m_pData = pPool;
    if (!pPool) return;

    int *pEntry = (int *)(pHdr + 0x14);
    for (int e = 0; e < count; e++) {
        EXFILEINFO &fi = m_Files[e];
        fi.m_pName = pPool;
        char *pStr = (char *)(*(int *)(nameTable + e * 4) + nameTable + e * 4);
        pPool += sprintf(pPool, pStr) + 1;

        fi.m_Unk38 = pEntry[2];
        fi.m_Hash = pEntry[1];
        fi.m_Flags = pEntry[3] | 0x80000000;
        fi.m_pFile = 0;
        fi.m_Unk2C = (u32)&m_File;
        fi.m_Size = pEntry[0];
        for (u32 k = 0; k < 4; k++) {
            const int *pSys = (k < (u32)pEntry[4]) ? pEntry + 5 + k * 2 : pEntry + 5;
            fi.m_Sys[k].m_Offset = pSys[0];
            fi.m_Sys[k].m_NameIndex = pSys[1];
        }
        m_Unk34[0] += fi.m_Size;

        // Only sound (0x21) and 0x01 class hashes are kept as given (and must be unique); the rest
        // get a generated id.
        u32 cls = fi.m_Hash & 0x7f000000;
        u8 keep = (cls == 0x21000000 ? 2 : 0) | (cls == 0x01000000 ? 1 : 0);
        if (keep) {
            for (int j = 0; j < m_Files.Size(); j++) {
                if (m_Files[j].m_Hash == fi.m_Hash && &m_Files[j] != &fi) {
                    if (cls == 0x21000000) for (;;) {}
                    keep = 0;
                }
            }
        }
        if (!keep) fi.m_Hash = e | 0x81000000;

        if ((fi.m_Hash & 0x7f000000) == 0x01000000 && fi.m_Unk38 != 0) {
            if (fi.m_Unk38 != 0xfa) {
                fi.m_Flags &= 0x7fffffff;
                fi.m_Hash = e | 0x81000000;
            }
        } else if ((fi.m_Hash & 0x7f000000) == 0x21000000 && fi.m_Unk38 != 0 && fi.m_Unk38 != 0xffffffff && fi.m_Unk38 != 5) {
            fi.m_Flags &= 0x7fffffff;
        }
        pEntry += pEntry[4] * 2 + 5;
    }
    m_Files.SortByHash();
}

EXFileList::~EXFileList()
{
    if (m_pData) EXFree(m_pData);
}

// NEAR: binary-search control flow/register use differ from retail.
EXFILEINFO *EXFileList::FindFile(u32 hash) const
{
    int n = m_Files.Size();
    const EXFILEINFO *p = m_Files.GetData();
    int i;
    if (n > 4 && m_Files.SortMethod() == 1) {
        int lo = 0;
        int hi = n - 1;
        i = (hi + 1) / 2;
        while (p[i].m_Hash != hash) {
            if (i == lo) return 0;
            int round;
            if (p[i].m_Hash > hash) {
                hi = i;
                round = 0;
            } else {
                lo = i;
                round = 1;
            }
            i = (round + hi + lo) / 2;
        }
        return (EXFILEINFO *)&p[i];
    }
    for (i = 0; i < n; i++) {
        if (p[i].m_Hash == hash) return (EXFILEINFO *)&p[i];
    }
    return 0;
}

// NEAR: retail keeps the data pointer in a stack slot and computes &p[i] after the loop.
EXFILEINFO *EXFileList::FindFile(const char *pName) const
{
    const EXFILEINFO *p = m_Files.GetData();
    for (int i = 0; i < m_Files.Size(); i++) {
        if (_stricmp(pName, p[i].m_pName) == 0) return (EXFILEINFO *)&p[i];
    }
    return 0;
}

EXFILEINFO *EXFileList::FindFile(const EXFile *pFile) const
{
    const EXFILEINFO *p = m_Files.GetData();
    int n = m_Files.Size();
    for (int i = 0; i < n; i++) {
        if (p[i].m_pFile == pFile) return (EXFILEINFO *)&p[i];
    }
    return 0;
}

void EXFileInfoArray::SortByName()
{
    qsort(m_pData, m_nSize, sizeof(EXFILEINFO), QSortFn_SortByName);
    m_SortMethod = 0;
}

void EXFileInfoArray::SortByHash()
{
    qsort(m_pData, m_nSize, sizeof(EXFILEINFO), QSortFn_SortByHash);
    m_SortMethod = 1;
}

EXFILEINFO *EXFileSys::GetFileInfo(u32 hash) const
{
    int n = m_FileLists.Size();
    for (int i = 0; i < n; i++) {
        EXFILEINFO *pInfo = m_FileLists.GetAt(i)->FindFile(hash);
        if (pInfo && (pInfo->m_Flags & 0x80000000)) return pInfo;
    }
    return 0;
}

EXFILEINFO *EXFileSys::GetFileInfo(const char *pName) const
{
    for (int i = 0; i < m_FileLists.Size(); i++) {
        EXFILEINFO *pInfo = m_FileLists.GetAt(i)->FindFile(pName);
        if (pInfo && (pInfo->m_Flags & 0x80000000)) return pInfo;
    }
    return 0;
}

// NEAR: 5 bytes differ around the inlined FindFile(EXFile *) result test.
EXFILEINFO *EXFileSys::GetFileInfo(const EXFile *pFile) const
{
    int n = m_FileLists.Size();
    for (int i = 0; i < n; i++) {
        EXFILEINFO *pInfo = m_FileLists.GetAt(i)->FindFile(pFile);
        if (pInfo && (pInfo->m_Flags & 0x80000000)) return pInfo;
    }
    return 0;
}

s8 EXFileSys::GetSysFileInfo(MXFile *pFile, const char *pName) const
{
    int i;
    for (i = 0; i < m_FileLists.Size(); i++) {
        EXFileList *pList = m_FileLists.GetAt(i);
        EXFILEINFO *pInfo = pList->FindFile(pName);
        if (pInfo) {
            for (int j = 0; j < 4; j++) {
                pFile->SysFile(j).m_pName = pList->m_DiscNames[pInfo->m_Sys[j].m_NameIndex];
                pFile->SysFile(j).m_Offset = pInfo->m_Sys[j].m_Offset;
                pFile->SysFile(j).m_Valid = 1;
            }
            return 1;
        }
    }
    for (i = 0; i < 4; i++) {
        pFile->SysFile(i).m_pName = pName;
        pFile->SysFile(i).m_Offset = 0;
        pFile->SysFile(i).m_Valid = 0;
    }
    return 1;
}

// NEAR: retail shares the return tail for the status != 0 path; ours duplicates it.
EXFile *EXFileSys::OnQueueFile(EXFile *pFile, u32 priority, s8 wait)
{
    if (pFile->m_Status == 0) {
        if (!m_QueuedList.IsMember(&pFile->m_Link)) {
            m_QueuedList.AddTail(&pFile->m_Link);
            pFile->SetPriority(priority);
            pFile->m_Status = 1;
            if (wait) {
                WaitSyncOnFile(pFile, 1);
                WaitSync(1);
            }
        }
    }
    return pFile;
}

EXFile *EXFileSys::OnLoadFile(EXFile *pFile)
{
    if (pFile->m_Status == 2) WaitSyncOnFile(pFile, 1);
    if (pFile->m_Status == 0 && !m_LoadedList.IsMember(&pFile->m_Link) && pFile->m_Status == 0
        && !m_QueuedList.IsMember(&pFile->m_Link)) {
        m_QueuedList.AddTail(&pFile->m_Link);
        pFile->m_Status = 1;
        pFile->m_Priority = 0x80000000;
        WaitSyncOnFile(pFile, 1);
        WaitSync(1);
    }
    return pFile;
}

EXFileSys::~EXFileSys()
{
    EXFile *pFile;
    for (pFile = EXFileFromItem(m_QueuedList.Head()); pFile; pFile = EXFileFromItem(pFile->m_Link.Next())) pFile->OnKill();
    for (pFile = EXFileFromItem(m_LoadedList.Head()); pFile; pFile = EXFileFromItem(pFile->m_Link.Next())) pFile->OnKill();
    for (int i = 0; i < m_FileLists.Size(); i++) {
        EXFileList *pList = m_FileLists.GetAt(i);
        if (pList) delete pList;
    }
}

// Refresh this list's entries from a newer list of the same files (matched by hash).
// NEAR retail quirk kept: the entry that gets cleared afterwards is indexed by the match position
// in the other list, but taken from this list.
void EXFileList::UpdateList(EXFileList *pOther)
{
    for (int i = 0; i < m_Files.Size(); i++) {
        EXFILEINFO &mine = m_Files[i];
        for (int j = 0; j < pOther->m_Files.Size(); j++) {
            if (mine.m_Hash == pOther->m_Files[j].m_Hash) {
                const EXFILEINFO &src = pOther->m_Files[j];
                mine.m_pFile = src.m_pFile;
                mine.m_pName = src.m_pName;
                mine.m_Size = src.m_Size;
                for (int k = 0; k < 4; k++) mine.m_Sys[k] = src.m_Sys[k];
                m_Files[j].m_pFile = 0;
                m_Files[j].m_Hash = 0xffffffff;
                break;
            }
        }
    }
    m_Files.SortByHash();
}

// Hand a file to the audio thread's async reader (all four disc copies, sorted by priority).
s8 EXFileSys::AsyncReadFile(MXFile *pFile, u32 priority)
{
    XFileHandle h;
    h.m_Size = pFile->m_Length;
    for (u32 i = 0; i < 4; i++) {
        (&h.m_Offset)[i] = pFile->m_SysFile[i].m_Offset + pFile->m_SeekPos;
        (&h.m_ReadOffset)[i] = 0;
        sprintf(h.m_Name, "%s", pFile->m_SysFile[0].m_pName);
    }
    h.m_Unk19c = (short)pFile->m_SysFile[0].m_Valid;
    EXBaseSoundManager::GetSoundManager()->AsyncReadFile(&h, (char *)pFile->m_pLoadAddr, ((-(int)priority - 1) & 0xffffff) | 0x4000000,
                                                         pFile->m_FileHandle);
    return 1;
}

// One scheduling step: start queued files on free slots, run the load/finish callbacks of the
// active ones. m_Status: 1 = a read was started, 2 = a callback failed / blocked.
void EXFileSys::Poll()
{
    if (m_Status == 2) m_Status = 0;
    bool started = false;
    for (u32 n = 0; n < 16; n++) {
        u32 slot = (m_PollStart + n) & 0xf;
        if (m_ActiveCount < 16 && m_pActive[slot] == 0) {
            EXDListItem *pItem = m_QueuedList.Head();
            if (pItem && EXFileFromItem(pItem)) {
                m_QueuedList.Remove(pItem);
                m_pActive[slot] = EXFileFromItem(pItem);
                m_ActiveCount++;
                EXFileFromItem(pItem)->m_pMXFile->SetFileHandle(slot);
            }
        }
        EXFile *pFile = m_pActive[slot];
        if (!pFile) continue;
        switch (pFile->m_Status) {
        default:
            if (m_Status != 2) {
                if (!pFile->OnLoad()) {
                    u8 old = pFile->m_Status;
                    pFile->m_Status = 5;
                    pFile->OnKill();
                    if (old == 6) {
                        m_ActiveCount--;
                        m_pActive[slot] = 0;
                    }
                    m_Status = 2;
                } else {
                    pFile->m_Status = 2;
                    m_Status |= 1;
                    started = true;
                }
            }
            break;
        case 2:
            if (pFile->GetFileLoadedStatus()) {
                m_ActiveCount--;
                m_pActive[slot] = 0;
                if (!pFile->OnLoaded()) {
                    pFile->m_Status = 5;
                    pFile->OnKill();
                    m_Status = 2;
                } else {
                    pFile->m_Status = 3;
                    m_LoadedList.AddTail(&pFile->m_Link);
                }
            }
            break;
        }
    }
    if (started) m_PollStart++;
    if (m_Status != 2 && m_ActiveCount == 0) m_Status = 0;
}

void EXFileSys::RegisterWatcherVars() const
{
    EXWatcher *pWatcher = EXWatcher::GetWatcher();
    unsigned int section = pWatcher->AddSection("Load Activity", 0u, "-Engine-", 1);
    unsigned int h = pWatcher->AddItem("Display", section, EXWATCH::DT_8, &g_EXFileLoadActivity);
    pWatcher->EnableItemEdit(h, 1);
    h = pWatcher->AddItem("Position", section, EXWATCH::DT_1, g_EXFileLoadActivity.pPosition());
    pWatcher->EnableItemEdit(h, 0u, 7u, 1u, 1);
    h = pWatcher->AddItem("Size", section, EXWATCH::DT_1, g_EXFileLoadActivity.pSize());
    pWatcher->EnableItemEdit(h, 0u, 7u, 1u, 1);
    h = pWatcher->AddItem("Alpha", section, EXWATCH::DT_1, g_EXFileLoadActivity.pAlpha());
    pWatcher->EnableItemEdit(h, 0u, 0xffu, 1u, 1);
}

void EXFileSys::Shutdown()
{
    if (m_pFileSys) {
        delete m_pFileSys;
        m_pFileSys = 0;
    }
}

EXFileSys::EXFileSys()
{
    m_Status = 0;
    m_PollStart = 0;
    m_UniqueID = 0;
    m_ActiveCount = 0;
    for (int i = 0; i < 16; i++) m_pActive[i] = 0;
}

void EXFileSys::AddFileList(const EXFILE &file)
{
    m_FileLists.Add(new EXFileList(file));
    EXBaseSoundManager::GetSoundManager()->BuildSoundFileList();
}
