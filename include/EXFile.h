#ifndef EXFILE_H
#define EXFILE_H
#include "types.h"
#include "EXRuntimeClass.h"
#include "MXFile.h"
#include "EXList.h"

class EXMemHeap;
struct EXFILEINFO;
namespace ex { template <class T> class EXFlag; }

// TODO layout: stand-in for ex::EXFlag<u8> (its constructor clears the byte).
struct EXFileFlags {
    EXFileFlags() { m_Flags = 0; }
    u8 m_Flags;
};

extern const char *EXFileNotInit;

// A loadable file (sizeof 0x20, runtime class with factory).
// Status values: 0 NotLoaded, 1 Queued, 2 (loading), 3 Loaded, 4 DeLoading, 5 Failed, 6 NotFound,
// 7 Stopping, 8 Stopped (EXF::Debug_FileStatus).
class EXFile {
    EX_DECLARE_DYNAMIC(EXFile)
public:
    virtual ~EXFile();
    virtual s8 OnLoad();
    virtual s8 OnLoaded();
    virtual s8 OnKill();
    static EXFile *CreateObject();
    void *MemData() const { return m_pMem; }
    s8 IsLoaded() const { return m_Status == 3; }
    const char *Filename() const { return m_pFilename; }
    u32 Status() const { return m_Status; }
    void SetStatus(int s) { m_Status = (u8)s; }
    void SetPriority(u32 p) { m_Priority = 0xffffffff - p; }
    MXFile *GetMXFile() const { return m_pMXFile; }
    void CleanUpMemory();
    void *MemDetach();
    s8 GetFileLoadedStatus() const;
    void Initalise();
    void ChangeMemOwner();
    int GetFilelength() const;
    s8 OnLoad(EXMemHeap *pHeap);
    EXFile();
    EXFile(const EXFILEINFO *pInfo, u32 length, u32 seekPos);
    EXFile(const char *pName, u32 length, u32 seekPos);
protected:
    void DeleteMXFile();
    void CreateMXFile(const EXFILEINFO *pInfo, u32 length, u32 seekPos);
    void CreateMXFile(const char *pName, u32 length, u32 seekPos);
public:
    EXDListItem m_Link;       // 0x04
    MXFile *m_pMXFile;        // 0x0c
    const char *m_pFilename;  // 0x10
    void *m_pMem;             // 0x14
    u8 m_Status;              // 0x18
    EXFileFlags m_Flags;      // 0x19 TODO: ex::EXFlag<u8> (Flags() accessor)
    u32 m_Priority;           // 0x1c stored inverted (0xffffffff - p)
};
#endif
