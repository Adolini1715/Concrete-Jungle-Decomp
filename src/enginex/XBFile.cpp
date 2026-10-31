#include "EXTextFile.h"
#include "MXFile.h"
#include "EXMemory.h"
#include "XBFileManager.h"

// C runtime (declared here; FILE is struct _iobuf).
extern "C" {
_iobuf *__cdecl fopen(const char *name, const char *mode);
int __cdecl fclose(_iobuf *f);
unsigned int __cdecl fread(void *p, unsigned int size, unsigned int n, _iobuf *f);
unsigned int __cdecl fwrite(const void *p, unsigned int size, unsigned int n, _iobuf *f);
int __cdecl fseek(_iobuf *f, long off, int origin);
void __stdcall OutputDebugStringA(const char *s);
}


volatile u32 g_uiCommandCode;
volatile u32 g_uiCommandData;
volatile u32 g_uiReplyId;

_iobuf *XbFOpen(const char *name, const char *mode, s8 enable)
{
    if (!enable) return 0;
    return fopen(name, mode);
}

int XbFClose(_iobuf *f, s8 enable)
{
    if (!enable) return 0;
    return fclose(f);
}

u32 XbFRead(void *p, u32 size, u32 n, _iobuf *f, s8 enable)
{
    if (!enable) return 0xffffffff;
    return fread(p, size, n, f);
}

void XbOutputDebugString(char *s)
{
    OutputDebugStringA(s);
}

int XbFSeek(_iobuf *f, long off, int origin, s8 enable)
{
    if (!enable) return -1;
    return fseek(f, off, origin);
}

u32 MXFile::GetLength() const
{
    return gFileManager.GetSize(m_SysFile[0].m_pName);
}

EXTextFile::EXTextFile(u32 bufSize) : EXBaseTextFile(bufSize)
{
    m_pFile = 0;
}

s8 EXTextFile::Open(const EXString &name, u32 mode)
{
    const char *pMode;
    if (mode & 4) pMode = "rb";
    if (mode & 8) pMode = "wb";
    if (mode & 0x10) pMode = "w+b";
    m_pFile = fopen(name, pMode);
    return m_pFile != 0;
}

void EXTextFile::FlushBuffer()
{
    if (m_pFile) {
        if (m_Dirty) {
            if (m_BufPos > 0) {
                fwrite(m_pBuffer, 1, m_BufPos, m_pFile);
                m_SeekPos += m_BufPos;
                m_BufPos = 0;
                m_BufLen = 0;
            }
            m_Dirty = 0;
        } else {
            if (m_SeekPos != m_FilePos) {
                fseek(m_pFile, m_FilePos, 0);
                m_SeekPos = m_FilePos;
            }
            m_BufPos = 0;
            m_BufLen = 0;
        }
    }
}

void EXTextFile::Close()
{
    if (m_pFile) {
        FlushBuffer();
        fclose(m_pFile);
        m_pFile = 0;
    }
}

void EXTextFile::SeekToBegin()
{
    if (m_pFile) {
        FlushBuffer();
        fseek(m_pFile, 0, 0);
        m_FilePos = 0;
        m_SeekPos = 0;
    }
}

s8 EXTextFile::ReadString(EXString &str)
{
    if (!m_pFile) return 0;
    if (m_Dirty) FlushBuffer();
    str.Empty();
    do {
        for (u32 i = m_BufPos; i < m_BufLen; i++) {
            char c = m_pBuffer[i];
            m_BufPos++;
            m_FilePos++;
            if (c == '\n') return 1;
            if (c != '\r') str += c;
        }
        m_BufPos = 0;
    } while ((m_BufLen = fread(m_pBuffer, 1, m_BufSize, m_pFile)) > 0);
    return str.Length() > 0;
}

s8 EXTextFile::WriteString(const EXString &str)
{
    if (!m_pFile) return 0;
    if (!m_Dirty) FlushBuffer();
    u32 len = str.Length();
    if (len > m_BufSize) {
        if (m_BufPos > 0) FlushBuffer();
        m_SeekPos += len;
        m_FilePos += len;
        return fwrite((const char *)str, 1, len, m_pFile) == len;
    }
    if (len > m_BufSize - m_BufPos) FlushBuffer();
    memcpy(m_pBuffer + m_BufPos, (const char *)str, len);
    m_FilePos += len;
    m_BufPos += len;
    m_Dirty = 1;
    return 1;
}

EXTextFile::~EXTextFile()
{
    Close();
}
