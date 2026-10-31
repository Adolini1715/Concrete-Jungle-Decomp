#ifndef MXFILE_H
#define MXFILE_H
#include "types.h"

// Where one copy of a file lives on the system disc (four alternatives per file).
struct EXSYSFILEINFO {
    const char *m_pName;
    u32 m_Offset;
    s8 m_Valid;
};

// Platform file handle used by EXFile for asynchronous loads. sizeof 0x40.
class MXFile {
public:
    MXFile(u32 length, u32 seekPos) { Initialise(length, seekPos); }
    void Initialise(u32 length, u32 seekPos);
    u32 Length() const { return m_Length; }
    void SetLength(u32 len) { m_Length = len; }
    void *&LoadAddr() { return m_pLoadAddr; }
    u32 SeekPos() const { return m_SeekPos; }
    EXSYSFILEINFO &SysFile(int i) { return m_SysFile[i]; }
    const u32 FileHandle() const { return m_FileHandle; }
    void SetFileHandle(u32 h);
    u32 GetLength() const;

    EXSYSFILEINFO m_SysFile[4];  // 0x00
    u32 m_Length;                // 0x30
    u32 m_SeekPos;               // 0x34
    u32 m_FileHandle;            // 0x38 unique id << 8 | type
    void *m_pLoadAddr;           // 0x3c
};
#endif
