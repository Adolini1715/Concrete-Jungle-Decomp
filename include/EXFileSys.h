#ifndef EXFILESYS_H
#define EXFILESYS_H
#include "types.h"
#include "EXArray.h"
#include "EXList.h"
#include "EXString.h"

class EXFile;
class MXFile;
class EXFileList;
// File-list description: name plus the length / seek position of the list inside it.
struct EXFILE {
    char *m_pName;
    int m_Size;
    int m_SeekPos;
};

// Offset of one disc copy and the index of its disc file name.
struct EXFILESYSENTRY {
    EXFILESYSENTRY() { m_NameIndex = 0; m_Offset = 0; }
    u32 m_Offset;
    u32 m_NameIndex;
};

// Per-file entry in a file list (sizeof 0x3c).
struct EXFILEINFO {
    void Set(const char *pName, u32 size, u32 a, u32 b);
    const char *m_pName;       // 0x00
    u32 m_Size;                // 0x04
    EXFILESYSENTRY m_Sys[4];   // 0x08
    EXFile *m_pFile;           // 0x28
    u32 m_Unk2C;               // 0x2c TODO
    u32 m_Hash;                // 0x30
    int m_Flags;               // 0x34 sign bit: present
    u32 m_Unk38;               // 0x38 TODO
};

// File list entries, sortable by name (0) or hash (1).
class EXFileInfoArray : public EXDataArray<EXFILEINFO, const EXFILEINFO &> {
public:
    EXFileInfoArray() { m_SortMethod = -1; }  // appended (GameWnd.obj copy; EXFileList ctor stores -1 too)
    int SortMethod() const { return m_SortMethod; }
    void SortByName();
    void SortByHash();
protected:
    static int QSortFn_SortByName(const void *a, const void *b);
    static int QSortFn_SortByHash(const void *a, const void *b);
    int m_SortMethod;  // 0x10
};

// One loaded file-list file (sizeof 0x3c).
class EXFileList {
public:
    EXFileList(const EXFILE &file);
    ~EXFileList();
    EXFILEINFO *FindFile(u32 hash) const;
    EXFILEINFO *FindFile(const char *pName) const;
    EXFILEINFO *FindFile(const EXFile *pFile) const;
    void UpdateList(EXFileList *pOther);

    EXFILE m_File;              // 0x00 copy of the description it was built from
    EXStringArray m_DiscNames;  // 0x0c names of the disc files holding the data
    EXFileInfoArray m_Files;    // 0x1c
    void *m_pData;              // 0x30
    u32 m_Unk34[2];             // 0x34 TODO
};

// Load-activity indicator settings (sizeof 0x38).
class EXFileLoadActivity {
public:
    EXFileLoadActivity() { m_Enabled = 0; m_Position = 2; m_Alpha = 0x50; m_Size = 2; m_Unk04 = 0; }
    s8 *pEnabled() { return &m_Enabled; }
    u8 *pPosition() { return &m_Position; }
    u8 *pAlpha() { return &m_Alpha; }
    u8 *pSize() { return &m_Size; }
    void Update(class EXWnd *pWnd);
    s8 m_Enabled;
    u8 m_Position;
    u8 m_Alpha;
    u8 m_Size;
    u32 m_Unk04;
    u8 m_Unk08[0x30];  // TODO layout
};

// File system front end (sizeof 0x70, not virtual).
// m_Status bits: 1 = a read was started this poll, value 2 = busy/blocked.
class EXFileSys {
public:
    static EXFileSys *GetFileSys() { return m_pFileSys; }
    EXFileList *FileList(int i) const { return m_FileLists.GetAt(i); }
    static s8 CdRomBuild() { return m_CdRomBuild; }
    u32 FileListCount() const { return m_FileLists.Size(); }
    static s8 &DebugLoadInfo() { return m_DebugLoadInfo; }
    u32 Status() const { return m_Status; }
    static s8 Init(EXFileSys *pFileSys);
    void UpdateFileLists() const;
    static void GetFileListFileName(char *pOut, const char *pName);
    static void WaitSyncOnFile(const EXFile *pFile, s8 updateHeaps);
    void UpdateLoadedFiles(s8 force) const;
    void Update();
    s8 GetLoadingStatus() const;
    void RegisterWatcherVars() const;
    void RemoveFile(EXFile *pFile);
    static s8 AsyncReadFile(MXFile *pFile, u32 priority);
    static void WaitSync(s8 updateHeaps);
    EXFILEINFO *GetFileInfo(u32 hash) const;
    EXFILEINFO *GetFileInfo(const char *pName) const;
    EXFILEINFO *GetFileInfo(const EXFile *pFile) const;
    s8 GetSysFileInfo(MXFile *pFile, const char *pName) const;
    EXFile *OnQueueFile(EXFile *pFile, u32 priority, s8 wait);
    EXFile *OnLoadFile(EXFile *pFile);
    // Typed wrappers (copies emitted in EXGeoFile.obj; appended by the geometry-layer agent).
    template <class T> static T *QueueFile(T *pFile, u32 priority) { return (T *)GetFileSys()->OnQueueFile(pFile, priority, 0); }
    template <class T> static T *LoadFile(T *pFile) { return (T *)GetFileSys()->OnLoadFile(pFile); }
    ~EXFileSys();
    static void Shutdown();
    EXFileSys();
    void AddFileList(const EXFILE &file);
    u32 GetUniqueID()
    {
        u32 id = m_UniqueID;
        m_UniqueID = (id + 1) & 0xffffff;
        return id;
    }
    const EXDList &FileQueuedList() const { return m_QueuedList; }
    const EXDList &FileLoadedList() const { return m_LoadedList; }
protected:
    void Poll();
    void WaitForFileCompletion(const EXFile *pFile) const;
    static EXFileSys *m_pFileSys;
    static s8 m_DebugLoadInfo;
    static s8 m_CdRomBuild;
public:
    EXPtrArray<EXFileList> m_FileLists;  // 0x00
    EXDList m_QueuedList;                // 0x10
    EXDList m_LoadedList;                // 0x18
    u32 m_Status;                        // 0x20
    EXFile *m_pActive[16];               // 0x24 files being read
    u32 m_ActiveCount;                   // 0x64
    u32 m_UniqueID;                      // 0x68
    u32 m_PollStart;                     // 0x6c
};

// Files are linked through their EXDListItem at +4.
inline EXFile *EXFileFromItem(EXDListItem *p) { return p ? (EXFile *)((u8 *)p - 4) : 0; }

extern EXFileLoadActivity g_EXFileLoadActivity;
#endif
