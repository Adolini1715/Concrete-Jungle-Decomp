#ifndef ASYNCFILE_H
#define ASYNCFILE_H
#include "types.h"

// File request handed to the audio/file manager (sizeof 0x1a0). TODO layout.
struct XFileHandle {
    u32 m_Offset;        // 0x00 start offset added to every read
    u32 m_Unk04[3];      // 0x04 TODO
    u32 m_ReadOffset;    // 0x10 offset passed to psiAsyncReadFile
    u32 m_Unk14[3];      // 0x14 TODO
    u32 m_Size;          // 0x20
    char m_Name[0x178];  // 0x24
    u16 m_Unk19c;        // 0x19c cleared by OpenSoundBankFile/OpenAudioFile; nonzero: parse ".nnn" suffix
    u16 m_Unk19e;
};

// AsyncFileStatus values.
enum { AFS_IDLE = 0, AFS_QUEUED = 1, AFS_READING = 2, AFS_CLOSED = 3, AFS_DMA = 4 };

// One open file slot (sizeof 0x1dc).
struct AsyncFileData {
    int m_Status;            // 0x000 AsyncFileStatus
    int m_Open;              // 0x004 FileOpen succeeded
    XFileHandle m_Handle;    // 0x008 (name at 0x2c)
    int m_hFile;             // 0x1a8 platform handle, -1 when closed
    int m_SeekPos;           // 0x1ac
    int m_Length;            // 0x1b0 requested length
    u32 m_ReadLength;        // 0x1b4 bytes still to read (sector aligned)
    char *m_pReadDest;       // 0x1b8
    char *m_pDest;           // 0x1bc requested destination
    u32 m_Priority;          // 0x1c0
    int m_Close;             // 0x1c4 close requested
    int m_Finished;          // 0x1c8
    int m_Stop;              // 0x1cc
    int m_ID;                // 0x1d0 id passed to OpenFile_
    int m_Number;            // 0x1d4 from the ".nnn" file name suffix
    int m_FileType;          // 0x1d8 FileType()
};

// Pool of open files of one type (sizeof 0x10).
class AsyncFile {
protected:
    virtual int OnLoad(AsyncFileData *pData, int n);
    virtual int CheckDMA(int n);
    virtual int FileType();
public:
    virtual ~AsyncFile();
    AsyncFile();
    void ReadFile(int handle, int offset, int length, char *pDest, unsigned int priority);
    void CloseFile(int handle);
    void StopRead(int handle);
protected:
    void Init(int count);
    int OpenFile_(int handle, XFileHandle *pHandle);
    int OpenFile(XFileHandle *pHandle, unsigned int id);
    void FinishedLoad(AsyncFileData *pData);
    int HasFileFinished(int handle);
public:
    AsyncFileData *m_pFiles;  // 0x04
    u32 m_Next;               // 0x08 next slot to try
    u32 m_Count;              // 0x0c
    friend class EXFileManager;
};

class EXGameFile : public AsyncFile {
public:
    EXGameFile() { Init(0x11); }
protected:
    virtual int OnLoad(AsyncFileData *pData, int n);
    virtual int CheckDMA(int n);
    virtual int FileType() { return 2; }
};

class EXAudioFile : public AsyncFile {
public:
    EXAudioFile() { Init(0x40); }
protected:
    virtual int OnLoad(AsyncFileData *pData, int n);
    virtual int CheckDMA(int n);
    virtual int FileType() { return 1; }
};

class EXSoundBankFile : public AsyncFile {
public:
    EXSoundBankFile() { Init(0x40); }
protected:
    virtual int OnLoad(AsyncFileData *pData, int n);
    virtual int CheckDMA(int n);
    virtual int FileType() { return 0; }
};
#endif
