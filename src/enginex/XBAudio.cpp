#include "EXAudio.h"
#include "EXAudioMemory.h"
#include "EXAudioVoiceManager.h"
#include "XBFileManager.h"
#include "XBMixer.h"
#include "EXMemHeap.h"
#include "EXMemory.h"

extern "C" double __cdecl sqrt(double);
#pragma intrinsic(sqrt)

EXAudio *gpEXAudio;
EXAudioMemory *gpEXAudioMemory;
static void *s_pAudioMemBase;
static int s_LoadBankHandle = -1;

void EXAudio::psiInitialiseReverb(int, int, int, int, int, int) {}

char *EXAudio::psiSFXLoadFile(char *) { return 0; }

void EXAudio::psiSFXFreeFileMem(char *) {}

void EXAudio::psiSetReverb(int level)
{
    gpMixer->SetMasterReverb(level);
}

void EXAudio::psiSFXSetMode(int) {}

void EXAudio::psiSFXPreMovieReset()
{
    gFileManager.Pause_FileSystem();
}

void EXAudio::psiSFXPostMovieReset()
{
    gFileManager.Unpause_FileSystem();
}

void EXAudio::psiPreSFXUpdate() {}

void EXAudio::psiPostSFXUpdate() {}

void EXAudio::psiBankMemoryDeAlloc(int addr)
{
    if (addr) gpEXAudioMemory->DeAlloc(addr);
}

int EXAudio::psiLoadSoundBank(XFileHandle *pHandle, unsigned int size)
{
    if (!pHandle) return 0;
    if (!pHandle->m_Name) return 0;
    u32 len = (pHandle->m_Size + 0x7ff) & ~0x7ff;
    int addr = gpEXAudioMemory->Alloc(((size ? size : pHandle->m_Size) + 0x7ff) & ~0x7ff);
    if (!addr) return 0;
    s_LoadBankHandle = psiAsyncOpenFile(pHandle, FILEMAN_SOUNDBANK, 0xffffffff);
    if (s_LoadBankHandle == -1) return 0;
    psiAsyncReadFile(s_LoadBankHandle, pHandle->m_ReadOffset, len, (char *)addr, 0x3000000);
    return addr;
}

int EXAudio::psiHasLoadSoundBankFinished()
{
    if (s_LoadBankHandle != -1) {
        if (psiAsyncHasReadFinished(s_LoadBankHandle)) {
            psiAsyncCloseFile(s_LoadBankHandle);
            s_LoadBankHandle = -1;
        } else {
            return 0;
        }
    }
    return 1;
}

void EXAudio::psiInitialiseSoundRam()
{
    gpAudioVoice->ResetResources();
}

int EXAudio::psiInitialiseAudioMemory(int size)
{
    if (!s_pAudioMemBase) {
        if (size < 0x100000) size = 0x800000;
        s_pAudioMemBase = _SystemHeapList[1]->_EXAlloc(size, 0);
        gpEXAudioMemory = new EXAudioMemory((u32)s_pAudioMemBase, size);
    }
    return 1;
}

int EXAudio::psiInitialiseSoundSystem()
{
    psiInitialiseSoundRam();
    return 1;
}

int EXAudio::psiAsyncOpenFile(XFileHandle *pHandle, FILEMAN_TYPE type, unsigned int id)
{
    switch (type) {
    case FILEMAN_SOUNDBANK: return gFileManager.OpenSoundBankFile(pHandle) | 0x1000000;
    case FILEMAN_AUDIO: return gFileManager.OpenAudioFile(pHandle) | 0x2000000;
    case FILEMAN_GAME: return gFileManager.OpenGameFile(pHandle, id) | 0x3000000;
    }
    return -1;
}

void EXAudio::psiAsyncStopAccess(int handle)
{
    switch (handle & 0xff000000) {
    case 0x1000000: gFileManager.m_pSoundBankFile->StopRead(handle); break;
    case 0x2000000: gFileManager.m_pAudioFile->StopRead(handle); break;
    case 0x3000000: gFileManager.m_pGameFile->StopRead(handle); break;
    }
}

void EXAudio::psiAsyncLockFilePriority(unsigned int p)
{
    gFileManager.SetFilePriorityLock(p);
}

void EXAudio::psiAsyncUnLockFilePriority()
{
    gFileManager.UnlockFilePriorityLock();
}

void EXAudio::psiAsyncReadFile(int handle, int offset, int length, char *pDest, unsigned int priority)
{
    switch (handle & 0xff000000) {
    case 0x1000000: gFileManager.m_pSoundBankFile->ReadFile(handle, offset, length, pDest, priority); break;
    case 0x2000000: gFileManager.m_pAudioFile->ReadFile(handle, offset, length, pDest, priority); break;
    case 0x3000000: gFileManager.m_pGameFile->ReadFile(handle, offset, length, pDest, priority); break;
    }
    gFileManager.FileThreadWakeUp();
}

int EXAudio::psiAsyncHasReadFinished(int handle)
{
    return gFileManager.HasFileFinished(handle);
}

void EXAudio::psiAsyncCloseFile(int handle)
{
    switch (handle & 0xff000000) {
    case 0x1000000: gFileManager.m_pSoundBankFile->CloseFile(handle); break;
    case 0x2000000: gFileManager.m_pAudioFile->CloseFile(handle); break;
    case 0x3000000: gFileManager.m_pGameFile->CloseFile(handle); break;
    }
}

char *EXAudio::psiAsyncCreateBuffer(int size, int)
{
    char *p = (char *)_SystemHeapList[0]->_EXAlloc(size + 0x1000, 0);
    if (!p) return p;
    return p + 0x800;
}

// NEAR: retail adds -0x800 (add) where we subtract (sub); 4 bytes.
void EXAudio::psiAsyncDeleteBuffer(char *p)
{
    if (p) {
        p -= 0x800;
        if (p) _SystemHeapList[0]->EXFree(p);
    }
}

int psiStreamHasMemCpyFinished(int) { return 1; }

void AsyncReadFile(XFileHandle *pHandle, char *pDest, unsigned int priority, unsigned int id)
{
    int handle = gpEXAudio->psiAsyncOpenFile(pHandle, FILEMAN_GAME, id);
    gpEXAudio->psiAsyncReadFile(handle, pHandle->m_ReadOffset, pHandle->m_Size, pDest, priority);
}

void AsyncStopReadFile(unsigned int handle)
{
    gFileManager.m_pGameFile->StopRead(handle & 0xff);
}

void AsyncStopFileRead() {}

void Init_FileListData(int n, XFileHandle *pHandle)
{
    gFileManager.Init_FileListData(n, pHandle);
}

void Do_Exit() {}

void EXAudio::psiDebugTextOutput(char *, char *) {}

void DEBUGTRACE(const char *pFmt, ...) {}

void EXAudio::SFXprintf(const char *pFmt, ...) {}

// NEAR: retail materialises 0.0f in xmm0 and returns it through the argument slot.
float EXAudio::psiSquareRoot(float f)
{
    if (f >= 0.0f) return (float)sqrt(f);
    f = 0.0f;
    return f;
}
