#ifndef EXAUDIO_H
#define EXAUDIO_H
#include "EXAudioBase.h"

// Sample description handed to the platform layer. TODO layout beyond the fields used.
struct SampleHeaderData {
    u8 m_Flags;        // 0x00 bit 0: looping
    u8 m_Pad01[3];
    int m_Handle;      // 0x04 data pointer, replaced by the sound slot in psiLoadSampleInitialise
    u32 m_Unk08;
    int m_SampleRate;  // 0x0c
    int m_Length;      // 0x10
    u32 m_Unk14;
    int m_LoopStart;   // 0x18
    u32 m_Unk1c;       // 0x1c (sizeof 0x20 from ES_RequestVoiceHandle)
};

// Xbox implementation of the sound engine platform interface (psi*); methods in XBAudio.obj.
class EXAudio : public EXAudioBase {
public:
    virtual int psiInitialiseAudioMemory(int);
    virtual int psiInitialiseSoundSystem();
    virtual int psiRequestVoiceHandle(SampleHeaderData *, int);
    virtual int psiIsSamplePlaying(int);
    virtual void psiSetSampleFrequency(int, int);
    virtual int psiSampleKeyOn(int, int);
    virtual int psiSampleKeyOff(int);
    virtual void psiSamplePause(int);
    virtual void psiSampleUnPause(int);
    virtual void psiSetSampleVolume(int, int);
    virtual void psiInitialiseSoundRam();
    virtual void psiLoadSampleInitialise(SampleHeaderData *);
    virtual void psiUnLoadSampleInitialise(SampleHeaderData *);
    virtual void psiPreSFXUpdate();
    virtual void psiPostSFXUpdate();
    virtual void psiSFXSetupListener(SFXPoint3 *, SFXPoint3 *, SFXPoint3 *, SFXPoint3 *, SFXPoint3 *);
    virtual void psiUnLockVoiceHandle(int);
    virtual void psiLockVoiceHandle(int);
    virtual void psiInitialiseSound(psiSFX *);
    virtual void psiUpdateSound(psiSFX *);
    virtual void psiDebugTextOutput(char *, char *);
    virtual int psiAsyncOpenFile(XFileHandle *, FILEMAN_TYPE, unsigned int);
    virtual void psiAsyncCloseFile(int);
    virtual void psiAsyncReadFile(int, int, int, char *, unsigned int);
    virtual void psiAsyncLockFilePriority(unsigned int);
    virtual void psiAsyncUnLockFilePriority();
    virtual void psiAsyncStopAccess(int);
    virtual int psiAsyncHasReadFinished(int);
    virtual char *psiAsyncCreateBuffer(int, int);
    virtual void psiAsyncDeleteBuffer(char *);
    virtual int psiLoadSoundBank(XFileHandle *, unsigned int);
    virtual int psiHasLoadSoundBankFinished();
    virtual void psiBankMemoryDeAlloc(int);
    virtual unsigned int psiSFXGetTimer();
    virtual char *psiSFXLoadFile(char *);
    virtual void psiSFXFreeFileMem(char *);
    virtual void psiSFXSetMode(int);
    virtual void psiSetReverb(int);
    virtual void psiSFXPreMovieReset();
    virtual void psiSFXPostMovieReset();
    virtual void psiInitialiseReverb(int, int, int, int, int, int);
    virtual float psiSquareRoot(float);
    virtual void SFXprintf(const char *, ...);
    EXAudio() {}
};

extern EXAudio *gpEXAudio;

// Free functions of XBAudio.obj.
void AsyncReadFile(XFileHandle *pHandle, char *pDest, unsigned int length, unsigned int priority);
void AsyncStopReadFile(unsigned int handle);
void Do_Exit();
void DEBUGTRACE(const char *pFmt, ...);
#endif
