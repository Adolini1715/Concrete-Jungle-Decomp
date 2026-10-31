#ifndef EXFILEMANAGER_H
#define EXFILEMANAGER_H
#include "types.h"
#include "AsyncFile.h"

enum FM_DMASTATES { FM_DMA_OFF = 0, FM_DMA_ON = 1 };
enum HFL_STATUS { HFL_BUSY = 0, HFL_LOADED = 1, HFL_ERROR = 2 };

// Asynchronous file reader shared by sound and game files (sizeof 0xb8).
// Per read "channel" (two, for double-buffered DMA) it tracks the active file and DMA state.
class EXFileManager {
public:
    virtual ~EXFileManager();
    virtual int Update();
    virtual void StopReadAccess() = 0;
    virtual void Init2() = 0;
    virtual int FileOpen(AsyncFileData *pData) = 0;
    virtual void FileRead(AsyncFileData *pData, unsigned char *pDest, unsigned int size) = 0;
    virtual void FileClose(AsyncFileData *pData) = 0;
    virtual unsigned int FileLength(AsyncFileData *pData) = 0;
    virtual HFL_STATUS HasFileLoaded() = 0;
    virtual unsigned int DEBUG_GetMillisecondTimer() = 0;
    virtual void __cdecl DEBUG_Print(const char *pFmt, ...) = 0;

    void SetIsDVD(s8 b) { m_IsDVD = b; }
    void EnableDebugStats(s8 b) { m_DebugStats = b; }
    void SetLoadingSpeed(s8 s) { m_LoadingSpeed = s; }
    void Pause_FileSystem() { m_Paused = 1; }
    void Unpause_FileSystem() { m_Paused = 0; }
    void SetFilePriorityLock(unsigned int) { m_PriorityLock = 1; }
    void UnlockFilePriorityLock() { m_PriorityLock = 0; }
    unsigned int GetCDSectorSize() const { return m_SectorSize; }
    s8 HasWorkToDo();
    void Init(FM_DMASTATES soundDMA, FM_DMASTATES gameDMA, unsigned int dmaChunk, unsigned int chunk,
              unsigned int sectorSize, s8 synchronous);
    void CheckFinishedLoad(int ch);
    int CallbackUpdate();
    int OpenSoundBankFile(XFileHandle *pHandle);
    int OpenAudioFile(XFileHandle *pHandle);
    int OpenGameFile(XFileHandle *pHandle, unsigned int id);
    int HasFileFinished(int handle);
    void Init_FileListData(int, XFileHandle *);
    EXFileManager();
    int Update_();
protected:
    void Update_CheckDMAStatus();
    void Update_SFXActiveData();
    void Update_CheckFilesToClose();
    void Update_DoCallback();
    s8 Update_CheckDMAFree();
    s8 Update_DebugSpeedLimit();
    void Update_LoadingStats();
    void ReadFileChunk(AsyncFileData *pData, int size, unsigned int dmaSize, FM_DMASTATES dma);
    void Update_ReadChunk();
public:
    int m_DMAWait;                     // 0x04
    char *m_pDMABuffer[2];             // 0x08
    int m_DMAState[2];                 // 0x10 0 free, 1 reading, 2 waiting for DMA
    AsyncFileData *m_pDMAFile[2];      // 0x18
    int m_Channel;                     // 0x20
    unsigned int m_DMAChunk;           // 0x24
    unsigned int m_Chunk;              // 0x28
    unsigned int m_ChunkSize[3];       // 0x2c per FILEMAN_TYPE
    unsigned int m_SectorSize;         // 0x38
    EXSoundBankFile *m_pSoundBankFile; // 0x3c
    EXAudioFile *m_pAudioFile;         // 0x40
    EXGameFile *m_pGameFile;           // 0x44
    u32 m_Unk48;                       // 0x48 TODO
    int m_PriorityLock;                // 0x4c
    s8 m_IsDVD;                        // 0x50
    s8 m_DebugStats;                   // 0x51
    int m_LoadingSpeed;                // 0x54
    int m_Paused;                      // 0x58
    int m_CallbackPending;             // 0x5c
    int m_Busy;                        // 0x60
    unsigned int m_ReadSize;           // 0x64
    unsigned int m_DMASize;            // 0x68
    unsigned int m_TotalRead;          // 0x6c
    u32 m_Unk70;                       // 0x70
    AsyncFileData *m_pActive[2];       // 0x74
    AsyncFile *m_pActiveFile[2];       // 0x7c
    unsigned int m_TimeNow;            // 0x84
    unsigned int m_TimeLast;           // 0x88
    unsigned int m_ReadStart;          // 0x8c
    unsigned int m_TypeTime[3];        // 0x90
    unsigned int m_TypePercent[3];     // 0x9c
    int m_MaxSeekLast;                 // 0xa8
    int m_MaxSeek;                     // 0xac
    int m_LastSeek;                    // 0xb0
    s8 m_Synchronous;                  // 0xb4
};
#endif
