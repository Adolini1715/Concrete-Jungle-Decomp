#ifndef EXBASETEXTFILE_H
#define EXBASETEXTFILE_H
#include "types.h"

// Buffered text file base (sizeof 0x20).
class EXBaseTextFile {
public:
    virtual ~EXBaseTextFile();
    EXBaseTextFile(u32 bufSize);
protected:
    u8 *m_pBuffer;   // 0x04
    u32 m_BufSize;   // 0x08 0 if the buffer allocation failed
    u32 m_FilePos;   // 0x0c logical position
    u32 m_SeekPos;   // 0x10 position of the underlying file
    u32 m_BufPos;    // 0x14
    u32 m_BufLen;    // 0x18
    s8 m_Dirty;      // 0x1c buffer holds unwritten data
};
#endif
