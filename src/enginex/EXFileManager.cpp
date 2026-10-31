#include "EXFileManager.h"
#include "FileQueue.h"
#include "EXAudio.h"
#include "EXMemory.h"

extern "C" int __cdecl printf(const char *pFmt, ...);

static int s_DMAEnabled;
static FM_DMASTATES s_DMAState[3];  // per FILEMAN_TYPE
static EXFileManager *s_pFileManager;
static int s_FilesToClose;
static FileQueue *s_pFileQueue;

FileQueue::FileQueue()
{
    RtlInitializeCriticalSection(&m_Lock);
    m_pList = new EXSortList(0x91);
}

void FileQueue::Add(AsyncFileData *pData)
{
    RtlEnterCriticalSection(&m_Lock);
    m_pList->Add(pData, pData->m_Priority);
    RtlLeaveCriticalSection(&m_Lock);
}

AsyncFileData *FileQueue::GetActive()
{
    RtlEnterCriticalSection(&m_Lock);
    int i;
    AsyncFileData *p = (AsyncFileData *)m_pList->GetHead(i);
    while (p) {
        if (!p->m_Stop && !p->m_Close && p->m_Status == AFS_QUEUED) {
            if (s_pFileManager->m_PriorityLock && p->m_FileType == 2 && (u8)p->m_ID != 0x10) p = 0;
            break;
        }
        m_pList->Remove(i);
        p = (AsyncFileData *)m_pList->GetHead(i);
    }
    RtlLeaveCriticalSection(&m_Lock);
    return p;
}

AsyncFile::AsyncFile() {}

AsyncFile::~AsyncFile() {}

void AsyncFile::Init(int count)
{
    m_Next = 0;
    m_Count = count;
    m_pFiles = (AsyncFileData *)_EXAlloc(count * sizeof(AsyncFileData), 0);
    for (int i = 0; i < count; i++) {
        m_pFiles[i].m_Open = 0;
        m_pFiles[i].m_Status = AFS_CLOSED;
        m_pFiles[i].m_hFile = -1;
        m_pFiles[i].m_Close = 0;
        m_pFiles[i].m_Finished = 0;
        m_pFiles[i].m_Stop = 0;
        m_pFiles[i].m_Priority = 0xf;
    }
}

// NEAR: ~60 bytes differ, digit-parse arithmetic shape and the shared epilogue.
int AsyncFile::OpenFile_(int handle, XFileHandle *pHandle)
{
    int id = handle;
    handle &= 0xff;
    if (m_pFiles[handle].m_Status == AFS_CLOSED) {
        m_pFiles[handle].m_Status = AFS_IDLE;
        m_pFiles[handle].m_ID = id;
        m_pFiles[handle].m_Open = 0;
        m_pFiles[handle].m_Close = 0;
        m_pFiles[handle].m_FileType = FileType();
        m_pFiles[handle].m_Handle = *pHandle;
        if (m_pFiles[handle].m_Handle.m_Unk19c) {
            for (int i = 0; i < 0x5e; i++) {
                if (pHandle->m_Name[i] == '.') {
                    i++;
                    m_pFiles[handle].m_Number = (pHandle->m_Name[i] - '0') * 100 + (pHandle->m_Name[i + 1] - '0') * 10
                                                + (pHandle->m_Name[i + 2] - '0');
                    break;
                }
            }
        }
        return handle;
    }
    return -1;
}

int AsyncFile::OpenFile(XFileHandle *pHandle, unsigned int id)
{
    if (id == 0xffffffff) {
        for (u32 i = 0; i < m_Count; i++) {
            int h = (m_Next + i) % m_Count;
            if (OpenFile_(h, pHandle) != -1) return h;
        }
        s_pFileManager->DEBUG_Print("OpenFile fail: Too many open files\n");
        return -1;
    }
    if (OpenFile_(id, pHandle) != -1) return id & 0xff;
    return -1;
}

// NEAR: register allocation of the parameters (retail loads length/dest up front).
void AsyncFile::ReadFile(int handle, int offset, int length, char *pDest, unsigned int priority)
{
    AsyncFileData *p = &m_pFiles[handle & 0xffffff];
    int seek = offset + p->m_Handle.m_Offset;
    p->m_Length = length;
    p->m_pDest = pDest;
    u32 sector = s_pFileManager->GetCDSectorSize();
    if (sector) {
        u32 mis = (sector - 1) & seek;
        pDest -= mis;
        length += mis;
        if ((u32)pDest & 3) {
            s_pFileManager->DEBUG_Print("ReadFile: DestBuffer not DWORD aligned\n");
            return;
        }
        seek &= ~(sector - 1);
        length = (length + sector - 1) & ~(sector - 1);
    }
    p->m_SeekPos = seek;
    p->m_pReadDest = pDest;
    p->m_ReadLength = length;
    p->m_Priority = priority;
    p->m_Finished = 0;
    p->m_Stop = 0;
    p->m_Status = AFS_QUEUED;
    s_pFileQueue->Add(p);
}

void AsyncFile::CloseFile(int handle)
{
    int i = handle & 0xffffff;
    AsyncFileData *p = &m_pFiles[i];
    if (p->m_Status == AFS_IDLE || p->m_Status == AFS_QUEUED) {
        p->m_Close = 1;
        s_FilesToClose++;
    } else {
        s_pFileManager->DEBUG_Print("Illegal close file %s\n", p->m_Handle.m_Name);
        s_pFileManager->DEBUG_Print("Files[FileHandle].AsyncFileStatus %d\n", m_pFiles[i].m_Status);
        s_pFileManager->DEBUG_Print("FileType() %d\n", FileType());
    }
}

void AsyncFile::StopRead(int handle)
{
    AsyncFileData *p = &m_pFiles[handle & 0xffffff];
    if (p->m_Stop != 1) {
        switch (p->m_Status) {
        case AFS_IDLE:
            return;
        case AFS_READING:
            s_pFileManager->StopReadAccess();
            break;
        case AFS_CLOSED:
            s_pFileManager->DEBUG_Print("Illegal Stop Read %s\n", p->m_Handle.m_Name);
            return;
        }
        p->m_Stop = 1;
    }
}

void AsyncFile::FinishedLoad(AsyncFileData *pData)
{
    m_Next = (m_Next + 1) % m_Count;
    pData->m_Finished = 1;
    int type = FileType();
    pData->m_Status = AFS_IDLE;
    if (type == 2) {
        pData->m_Close = 1;
        s_FilesToClose++;
    }
}

int AsyncFile::OnLoad(AsyncFileData *pData, int n) { return 1; }

int AsyncFile::CheckDMA(int n) { return 1; }

int AsyncFile::FileType() { return 0; }

int AsyncFile::HasFileFinished(int handle)
{
    handle &= 0xffffff;
    if (!m_pFiles[handle].m_Stop || m_pFiles[handle].m_Status == AFS_READING) {
        if (m_pFiles[handle].m_Status == AFS_CLOSED) return 1;
        return m_pFiles[handle].m_Status == AFS_IDLE;
    }
    return 1;
}

s8 EXFileManager::HasWorkToDo()
{
    return s_pFileQueue->HasWorkToDo();
}

void EXFileManager::Init(FM_DMASTATES soundDMA, FM_DMASTATES gameDMA, unsigned int dmaChunk, unsigned int chunk,
                         unsigned int sectorSize, s8 synchronous)
{
    m_Synchronous = synchronous;
    m_DMAWait = 0;
    m_CallbackPending = 0;
    m_Busy = 0;
    if (soundDMA == FM_DMA_ON || gameDMA == FM_DMA_ON) s_DMAEnabled = 1;
    else s_DMAEnabled = 0;
    s_DMAState[2] = gameDMA;
    s_DMAState[0] = soundDMA;
    s_DMAState[1] = FM_DMA_OFF;
    m_Chunk = chunk;
    m_DMAChunk = dmaChunk;
    m_ChunkSize[0] = s_DMAState[0] == FM_DMA_ON ? dmaChunk : chunk;
    m_ChunkSize[1] = s_DMAState[1] == FM_DMA_ON ? dmaChunk : chunk;
    m_ChunkSize[2] = s_DMAState[2] == FM_DMA_ON ? dmaChunk : chunk;
    printf("%d\n", m_ChunkSize[2]);
    m_SectorSize = sectorSize;
    m_DMASize = 0;
    m_TimeLast = DEBUG_GetMillisecondTimer();
    m_TypeTime[0] = 0;
    m_TypeTime[1] = 0;
    m_TypeTime[2] = 0;
    m_MaxSeekLast = 0;
    m_MaxSeek = 0;
    m_LastSeek = 0;
    if (s_DMAEnabled == 1) {
        char *p = gpEXAudio->psiAsyncCreateBuffer(sectorSize * 3 + dmaChunk * 2, 0);
        m_pDMABuffer[0] = p + sectorSize;
        m_pDMABuffer[1] = p + sectorSize * 2 + dmaChunk;
        m_DMAState[0] = 0;
        m_DMAState[1] = 0;
        m_pDMAFile[0] = 0;
        m_pDMAFile[1] = 0;
    }
    Init2();
}

EXFileManager::~EXFileManager() {}

void EXFileManager::CheckFinishedLoad(int ch)
{
    if (s_DMAEnabled == 1) {
        if (m_pDMAFile[0] == m_pActive[ch] || m_pDMAFile[1] == m_pActive[ch]) return;
    }
    if (m_pActive[ch]->m_ReadLength == 0) m_pActiveFile[ch]->FinishedLoad(m_pActive[ch]);
}

int EXFileManager::CallbackUpdate()
{
    int ret = 1;
    switch (HasFileLoaded()) {
    case HFL_LOADED:
        m_pActive[m_Channel]->m_Status = AFS_QUEUED;
        if (m_pActive[m_Channel]->m_Stop) {
            m_pActiveFile[m_Channel]->FinishedLoad(m_pActive[m_Channel]);
            m_pActive[m_Channel]->m_Status = AFS_IDLE;
            m_DMAState[m_Channel] = 0;
            m_CallbackPending = 0;
            break;
        }
        if (m_pActiveFile[m_Channel]->OnLoad(m_pActive[m_Channel], m_Channel)) {
            unsigned int t = DEBUG_GetMillisecondTimer();
            m_TypeTime[m_pActiveFile[m_Channel]->FileType()] += t - m_ReadStart;
            m_LastSeek += m_ReadSize;
            m_pActive[m_Channel]->m_ReadLength -= m_ReadSize;
            m_pActive[m_Channel]->m_Length -= m_DMASize;
            m_pActive[m_Channel]->m_pReadDest += m_ReadSize;
            m_pActive[m_Channel]->m_pDest += m_ReadSize;
            m_pActive[m_Channel]->m_SeekPos += m_ReadSize;
            m_TotalRead += m_ReadSize;
            m_CallbackPending = 0;
            if (s_DMAState[m_pActiveFile[m_Channel]->FileType()] == FM_DMA_OFF) {
                CheckFinishedLoad(m_Channel);
                break;
            }
            if (m_pActive[m_Channel]->m_ReadLength == 0) m_pActive[m_Channel]->m_Status = AFS_DMA;
            m_pDMAFile[m_Channel] = m_pActive[m_Channel];
            m_DMAState[m_Channel] = 2;
        }
        break;
    case HFL_ERROR:
        m_pActive[m_Channel]->m_Status = AFS_QUEUED;
        m_CallbackPending = 0;
        m_DMAState[m_Channel] = 0;
        ret = 0;
        break;
    }
    return ret;
}

void EXFileManager::Update_CheckDMAStatus()
{
    if (s_DMAEnabled == 1) {
        m_DMAWait = 0;
        for (int i = 0; i < 2; i++) {
            if (m_DMAState[i] == 2) {
                if (m_pActiveFile[i]->CheckDMA(i)) {
                    m_pDMAFile[i] = 0;
                    m_DMAState[i] = 0;
                    CheckFinishedLoad(i);
                } else {
                    m_DMAWait = 1;
                }
            }
        }
    }
}

void EXFileManager::Update_SFXActiveData()
{
    AsyncFileData *p = m_pGameFile->m_pFiles;
    for (int i = 0; i < 17; i++, p++) {
        gpEXAudio->m_FileStatus[i] = (p->m_Finished ? 0x80 : 0) | (p->m_ID & 0xffffff00) | p->m_Status;
    }
}

void EXFileManager::Update_CheckFilesToClose()
{
    AsyncFileData *p = m_pGameFile->m_pFiles;
    int i;
    for (i = 0; i < (int)m_pGameFile->m_Count; i++, p++) {
        if (p->m_Stop && p->m_Status == AFS_QUEUED) {
            p->m_Close = 1;
            s_FilesToClose++;
        }
        if (p->m_Status != AFS_READING && p->m_Close) {
            s_FilesToClose--;
            if (p->m_Open) FileClose(p);
            p->m_hFile = -1;
            p->m_Open = 0;
            p->m_Close = 0;
            p->m_Stop = 0;
            p->m_Status = AFS_CLOSED;
        }
    }
    AsyncFileData *pS = m_pSoundBankFile->m_pFiles;
    AsyncFileData *pA = m_pAudioFile->m_pFiles;
    for (i = 0; i < 0x40; i++, pS++, pA++) {
        if (pS->m_Status != AFS_READING && pS->m_Close) {
            s_FilesToClose--;
            if (pS->m_Open) FileClose(pS);
            pS->m_hFile = -1;
            pS->m_Open = 0;
            pS->m_Close = 0;
            pS->m_Status = AFS_CLOSED;
        }
        if (pA->m_Status != AFS_READING && pA->m_Close) {
            s_FilesToClose--;
            if (pA->m_Open) FileClose(pA);
            pA->m_hFile = -1;
            pA->m_Open = 0;
            pA->m_Close = 0;
            pA->m_Status = AFS_CLOSED;
        }
    }
}

void EXFileManager::Update_DoCallback()
{
    if (m_CallbackPending && !m_Synchronous) CallbackUpdate();
}

s8 EXFileManager::Update_CheckDMAFree()
{
    if (m_CallbackPending) return 0;
    if (s_DMAEnabled == 1) {
        m_Channel = -1;
        for (int i = 0; i < 2; i++) {
            if (m_DMAState[i] == 0) {
                m_Channel = i;
                break;
            }
        }
        return m_Channel != -1;
    }
    return 1;
}

s8 EXFileManager::Update_DebugSpeedLimit()
{
    return 1;
}

void EXFileManager::Update_LoadingStats()
{
    if (m_DebugStats) {
        m_TimeNow = DEBUG_GetMillisecondTimer();
        unsigned int total = m_TypeTime[1] + m_TypeTime[2] + m_TypeTime[0];
        unsigned int elapsed = m_TimeNow - m_TimeLast;
        if (total < elapsed) total = elapsed;
        if (elapsed > 1000) {
            for (int i = 0; i < 3; i++) {
                m_TypePercent[i] = m_TypeTime[i] * 100 / total;
                if (m_TypePercent[i] > 100) m_TypePercent[i] = 100;
                m_TypeTime[i] = 0;
            }
            m_MaxSeekLast = m_MaxSeek;
            m_MaxSeek = 0;
            m_TimeLast = m_TimeNow;
        }
    }
}

void EXFileManager::ReadFileChunk(AsyncFileData *pData, int size, unsigned int dmaSize, FM_DMASTATES dma)
{
    m_Busy = 1;
    m_ReadStart = DEBUG_GetMillisecondTimer();
    if (!pData->m_Open) {
        if (FileOpen(pData)) {
            pData->m_Open = 1;
        } else {
            DEBUG_Print("File Open Fail: %s\n", pData->m_Handle.m_Name);
            return;
        }
    }
    pData->m_Status = AFS_READING;
    if (dma == FM_DMA_OFF) {
        FileRead(pData, (unsigned char *)pData->m_pReadDest, size);
    } else {
        m_DMAState[m_Channel] = 1;
        FileRead(pData, (unsigned char *)m_pDMABuffer[m_Channel], size);
    }
    int dist = pData->m_SeekPos - m_LastSeek;
    if (dist < 0) dist = -dist;
    if (dist > m_MaxSeek) m_MaxSeek = dist;
    m_LastSeek = pData->m_SeekPos;
    if (m_Synchronous) {
        if (HasFileLoaded() != HFL_LOADED) m_CallbackPending = 1;
    } else {
        m_CallbackPending = 1;
        CallbackUpdate();
    }
}

int EXFileManager::OpenSoundBankFile(XFileHandle *pHandle)
{
    pHandle->m_Unk19c = 0;
    return m_pSoundBankFile->OpenFile(pHandle, 0xffffffff);
}

int EXFileManager::OpenAudioFile(XFileHandle *pHandle)
{
    pHandle->m_Unk19c = 0;
    return m_pAudioFile->OpenFile(pHandle, 0xffffffff);
}

int EXFileManager::OpenGameFile(XFileHandle *pHandle, unsigned int id)
{
    return m_pGameFile->OpenFile(pHandle, id);
}

int EXFileManager::HasFileFinished(int handle)
{
    switch (handle & 0xff000000) {
    case 0x1000000: return m_pSoundBankFile->HasFileFinished(handle);
    case 0x2000000: return m_pAudioFile->HasFileFinished(handle);
    case 0x3000000: return m_pGameFile->HasFileFinished(handle);
    }
    return 1;
}

void EXFileManager::Init_FileListData(int, XFileHandle *) {}

EXFileManager::EXFileManager()
{
    s_pFileQueue = new FileQueue;
    m_Paused = 0;
    m_Channel = 0;
    m_PriorityLock = 0;
    m_Synchronous = 0;
    m_TotalRead = 0;
    m_Unk70 = 0;
    m_LoadingSpeed = 0;
    m_DebugStats = 0;
    m_IsDVD = 1;
    s_pFileManager = this;
    m_pSoundBankFile = new EXSoundBankFile;
    m_pAudioFile = new EXAudioFile;
    m_pGameFile = new EXGameFile;
}

void EXFileManager::Update_ReadChunk()
{
    AsyncFileData *p = s_pFileQueue->GetActive();
    if (p) {
        switch (p->m_FileType) {
        case 0:
            m_pActiveFile[m_Channel] = m_pSoundBankFile;
            m_pActive[m_Channel] = p;
            if (m_pActive[m_Channel]->m_ReadLength < m_ChunkSize[0]) m_ReadSize = m_pActive[m_Channel]->m_ReadLength;
            else m_ReadSize = m_ChunkSize[0];
            if (s_DMAState[0] == FM_DMA_ON) {
                if ((u32)m_pActive[m_Channel]->m_Length < m_ChunkSize[0]) m_DMASize = m_pActive[m_Channel]->m_Length;
                else m_DMASize = m_ChunkSize[0];
            }
            ReadFileChunk(m_pActive[m_Channel], m_ReadSize, m_DMASize, s_DMAState[0]);
            return;
        case 1:
            m_pActiveFile[m_Channel] = m_pAudioFile;
            m_pActive[m_Channel] = p;
            m_ReadSize = m_pActive[m_Channel]->m_ReadLength;
            ReadFileChunk(m_pActive[m_Channel], m_ReadSize, m_pActive[m_Channel]->m_ReadLength, FM_DMA_OFF);
            return;
        case 2:
            m_pActiveFile[m_Channel] = m_pGameFile;
            m_pActive[m_Channel] = p;
            if (m_pActive[m_Channel]->m_ReadLength < m_ChunkSize[2]) m_ReadSize = m_pActive[m_Channel]->m_ReadLength;
            else m_ReadSize = m_ChunkSize[2];
            if (s_DMAState[2] == FM_DMA_ON) {
                if ((u32)m_pActive[m_Channel]->m_Length < m_ChunkSize[2]) m_DMASize = m_pActive[m_Channel]->m_Length;
                else m_DMASize = m_ChunkSize[2];
            }
            ReadFileChunk(m_pActive[m_Channel], m_ReadSize, m_DMASize, s_DMAState[2]);
            return;
        }
    }
    m_Busy = 0;
}

int EXFileManager::Update_()
{
    Update_CheckDMAStatus();
    Update_SFXActiveData();
    Update_CheckFilesToClose();
    Update_DoCallback();
    if (Update_CheckDMAFree()) Update_ReadChunk();
    Update_LoadingStats();
    return m_Busy;
}

int EXFileManager::Update()
{
    Update_CheckDMAStatus();
    Update_SFXActiveData();
    Update_CheckFilesToClose();
    Update_DoCallback();
    if (Update_CheckDMAFree()) Update_ReadChunk();
    Update_LoadingStats();
    return m_Busy;
}
