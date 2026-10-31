#include "XBFileManager.h"
#include "EXAudio.h"
#include "EXAudioMemory.h"
#include "EXMemory.h"
#include "RGBA.h"
#include "EXDisplay.h"

// Xbox API (declared here from the map's names).
extern "C" {
int __stdcall CreateFileA(const char *pName, u32 access, u32 share, void *pSec, u32 disposition, u32 flags, void *hTemplate);
int __stdcall CloseHandle(int h);
u32 __stdcall SetFilePointer(int h, int dist, int *pDistHigh, u32 method);
int __stdcall ReadFile(int h, void *pBuf, u32 size, u32 *pRead, void *pOverlapped);
u32 __stdcall GetFileSize(int h, u32 *pHigh);
int __stdcall GetFileAttributesExA(const char *pName, int level, void *pInfo);
int __stdcall SetEvent(int h);
u32 __stdcall WaitForSingleObject(int h, u32 ms);
int __stdcall CreateEventA(void *pSec, int manual, int initial, const char *pName);
int __stdcall CreateThread(void *pSec, u32 stack, unsigned long (__stdcall *pFunc)(void *), void *pParam, u32 flags, u32 *pID);
void __stdcall Sleep(u32 ms);
u32 __stdcall GetTickCount();
int __stdcall D3DXCreateTextureFromFileExA(void *pDevice, const char *pFile, u32 w, u32 h, u32 mips, u32 usage,
                                           u32 format, u32 pool, u32 filter, u32 mipFilter, u32 colorKey,
                                           void *pInfo, void *pPalette, void **ppTex);
void __stdcall D3DDevice_Clear(u32 count, const void *pRects, u32 flags, u32 color, float z, u32 stencil);
void __stdcall D3DDevice_Swap(u32 flags);
}

// WIN32_FILE_ATTRIBUTE_DATA
struct XBFileAttributes {
    u32 m_Attributes;
    u32 m_Times[6];
    u32 m_SizeHigh;
    u32 m_SizeLow;
};

void XBRenderTexture(void *pTex, float, float, float, float, int, float, float, float, float, float, ex::RGBA col);


static int s_DiskBad;
static int s_hFileEvent;


void XBFileManager::UpdateStats()
{
    if (gpEXAudio) {
        gpEXAudio->m_FilePercent[0] = m_TypePercent[0];
        gpEXAudio->m_FilePercent[1] = m_TypePercent[1];
        gpEXAudio->m_FilePercent[2] = m_TypePercent[2];
        gpEXAudio->m_FileMaxSeek = m_MaxSeekLast;
    }
    if (gpEXAudioMemory) {
        u32 used = gpEXAudioMemory->GetAllocSize();
        gpEXAudio->m_AudioMemFree = gpEXAudioMemory->TotalSize() - used;
        gpEXAudio->m_AudioMemUsed = gpEXAudioMemory->GetAllocSize();
    }
}

void XBFileManager::FileThreadWakeUp()
{
    SetEvent(s_hFileEvent);
}

static unsigned long __stdcall xbFileThread(void *)
{
    for (;;) {
        WaitForSingleObject(s_hFileEvent, 8);
        while (gFileManager.Update()) {}
    }
}

XBFileManager::XBFileManager()
{
    m_hFile = -1;
    m_ReadStatus = 0;
}

// Something before gFileManager used up $E1/$E2 (its initialiser is $E3, the atexit dtor $E4);
// a const-float initialiser reproduces the numbering (see XBBenchmark.cpp).
static const float s_EPadInit = 0.0f;
static float s_EPad = s_EPadInit;
XBFileManager gFileManager;

void XBFileManager::Startup()
{
    Init(FM_DMA_OFF, FM_DMA_OFF, 0x800, 0x40000, 0x800, 0);
    s_hFileEvent = CreateEventA(0, 0, 0, 0);
    CreateThread(0, 0, xbFileThread, 0, 0, 0);
    Sleep(1);
}

void XBFileManager::Init2() {}

int XBFileManager::FileOpen(AsyncFileData *pData)
{
    int h = CreateFileA(pData->m_Handle.m_Name, 0x80000000, 1, 0, 3, 0x20000000, 0);
    pData->m_hFile = h;
    pData->m_Open = h != -1;
    return pData->m_Open;
}

void XBFileManager::FileClose(AsyncFileData *pData)
{
    if (pData->m_Open && pData->m_hFile != -1) CloseHandle(pData->m_hFile);
    pData->m_Open = 0;
}

void XBFileManager::StopReadAccess() {}

unsigned int XBFileManager::DEBUG_GetMillisecondTimer()
{
    return GetTickCount();
}

void __cdecl XBFileManager::DEBUG_Print(const char *pFmt, ...) {}

int XBFileManager::GetSize(const char *pName)
{
    XBFileAttributes attr;
    return GetFileAttributesExA(pName, 0, &attr) ? attr.m_SizeLow : 0;
}

int EXAudioFile::OnLoad(AsyncFileData *pData, int n) { return 1; }
int EXAudioFile::CheckDMA(int n) { return 1; }
int EXGameFile::OnLoad(AsyncFileData *pData, int n) { return 1; }
int EXGameFile::CheckDMA(int n) { return 1; }
int EXSoundBankFile::OnLoad(AsyncFileData *pData, int n) { return 1; }
int EXSoundBankFile::CheckDMA(int n) { return 1; }

int XBFileManager::IsDiskBad()
{
    return s_DiskBad;
}

void XBFileManager::BadDiskLockUp()
{
    s_DiskBad = 1;
    for (;;) Sleep(123);
}

void XBFileManager::HandleDiskErrors()
{
    if (s_DiskBad) {
        void *pDevice = ((EXDisplay *)EXBaseDisplay::GetDisplay())->Device();
        if (pDevice) {
            void *pTex = 0;
            D3DXCreateTextureFromFileExA(pDevice, "d:\\BadDisk_512x512.bmp", 0x200, 0x200, 1, 0, 0xc, 0,
                                         0xffffffff, 0xffffffff, 0, 0, 0, &pTex);
            D3DDevice_Clear(0, 0, 0xf0, pTex ? 0 : 0xffffff, 1.0f, 0);
            if (pTex) XBRenderTexture(pTex, 0, 0, 640.0f, 480.0f, 0, 0, 640.0f, 480.0f, 0, 0, ex::RGBA(0xff, 0x80, 0x80, 0x80));
            D3DDevice_Swap(0);
        }
        for (;;) Sleep(123);
    }
}

void XBFileManager::FileRead(AsyncFileData *pData, unsigned char *pDest, unsigned int size)
{
    while (!HasFileLoaded()) {}
    if (!pDest) BadDiskLockUp();
    if ((int)size <= 0) BadDiskLockUp();
    if (pData->m_SeekPos < 0) BadDiskLockUp();
    if ((u32)pDest & 3) BadDiskLockUp();
    if (pData->m_SeekPos & 0x7ff) BadDiskLockUp();
    if (size & 0x7ff) BadDiskLockUp();
    m_hFile = pData->m_hFile;
    m_ReadStatus = 1;
    SetFilePointer(pData->m_hFile, pData->m_SeekPos, 0, 0);
    u32 read = 0;
    s8 ok = ReadFile(pData->m_hFile, pDest, size, &read, 0) != 0;
    if (!read) ok = 0;
    m_ReadStatus = ok ? 0 : 2;
}

HFL_STATUS XBFileManager::HasFileLoaded()
{
    if (m_hFile == -1) return HFL_LOADED;
    switch (m_ReadStatus) {
    case 1: return HFL_BUSY;
    case 2: BadDiskLockUp(); break;
    default: return HFL_LOADED;
    }
}

unsigned int XBFileManager::FileLength(AsyncFileData *pData)
{
    if (!pData->m_hFile) {
        XBFileAttributes attr;
        return GetFileAttributesExA(pData->m_Handle.m_Name, 0, &attr) ? attr.m_SizeLow : 0;
    }
    return GetFileSize(pData->m_hFile, 0);
}
