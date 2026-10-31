#ifndef EXTEXTFILE_H
#define EXTEXTFILE_H
#include "EXBaseTextFile.h"

#include "EXString.h"
struct _iobuf;

// Open mode bits: 4 read ("rb"), 8 write ("wb"), 0x10 read/write ("w+b").
class EXTextFile : public EXBaseTextFile {
public:
    virtual ~EXTextFile();
    virtual s8 Open(const EXString &name, u32 mode);
    virtual void Close();
    virtual void SeekToBegin();
    virtual s8 ReadString(EXString &str);
    virtual s8 WriteString(const EXString &str);
    EXTextFile(u32 bufSize);
protected:
    void FlushBuffer();
    _iobuf *m_pFile;  // 0x20
};
#endif
