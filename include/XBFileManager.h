#ifndef XBFILEMANAGER_H
#define XBFILEMANAGER_H
#include "EXFileManager.h"

// Xbox file manager: CreateFile/ReadFile on a worker thread (sizeof 0xc0).
class XBFileManager : public EXFileManager {
public:
    virtual void StopReadAccess();
    virtual void Init2();
    virtual int FileOpen(AsyncFileData *pData);
    virtual void FileRead(AsyncFileData *pData, unsigned char *pDest, unsigned int size);
    virtual void FileClose(AsyncFileData *pData);
    virtual unsigned int FileLength(AsyncFileData *pData);
    virtual HFL_STATUS HasFileLoaded();
    virtual unsigned int DEBUG_GetMillisecondTimer();
    virtual void __cdecl DEBUG_Print(const char *pFmt, ...);
    void UpdateStats();
    void FileThreadWakeUp();
    XBFileManager();
    void Startup();
    int GetSize(const char *pName);
    int IsDiskBad();
    void HandleDiskErrors();
protected:
    void BadDiskLockUp();
public:
    int m_ReadStatus;  // 0xb8 HFL_STATUS of the last read
    int m_hFile;       // 0xbc handle being read, -1 none
};

extern XBFileManager gFileManager;
#endif
