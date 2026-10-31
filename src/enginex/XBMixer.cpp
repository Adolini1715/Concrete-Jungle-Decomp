#include "XBMixer.h"
#include "EXMemory.h"

// TODO: DirectSound wrappers (xb::IDirectSound*), xbMixerThread, Init/Shutdown/CreateSpeakerChannel
// and the static helpers of this object are not done yet.

extern "C" int __stdcall QueryPerformanceCounter(s64 *pCount);
extern "C" int __stdcall QueryPerformanceFrequency(s64 *pFreq);
// DirectSound entry points (declared here from the map's names).
extern "C" long __stdcall IDirectSoundStream_SetVolume(void *pStream, long volume);
extern "C" long __stdcall IDirectSoundBuffer_GetCurrentPosition(void *pBuffer, u32 *pPlay, u32 *pWrite);
extern "C" long __stdcall IDirectSoundBuffer_Stop(void *pBuffer);
extern "C" long __stdcall IDirectSoundBuffer_Release(void *pBuffer);
extern "C" double __cdecl log10(double);
#pragma intrinsic(log10)

static short s_SpeakerBuf[5][0x1000];
XBMixer *gpMixer;
static u32 s_Unused;
static void *s_pSpeakerBuffers[5];

double xbAudioGetTime()
{
    s64 count, freq;
    QueryPerformanceCounter(&count);
    QueryPerformanceFrequency(&freq);
    return (double)count * 1000.0 / (double)freq;
}

void XBMixer_Five_One::Mix_()
{
    Mix_FiveOne((short *)((char *)s_SpeakerBuf[0] + m_WritePos), (short *)((char *)s_SpeakerBuf[1] + m_WritePos),
                (short *)((char *)s_SpeakerBuf[2] + m_WritePos), (short *)((char *)s_SpeakerBuf[3] + m_WritePos),
                (short *)((char *)s_SpeakerBuf[4] + m_WritePos));
}

void xbAudioDSS_SetVolume(void *pStream, int volume)
{
    if (pStream) {
        long v;
        if (volume <= 0) v = -10000;
        else if (volume >= 100) v = 0;
        else v = (long)((float)log10((float)volume * 0.01f) * 1500.0f);
        IDirectSoundStream_SetVolume(pStream, v);
    }
}

unsigned int XBMixer::GetPlayPos()
{
    u32 pos = 0;
    if (s_pSpeakerBuffers[0]) IDirectSoundBuffer_GetCurrentPosition(s_pSpeakerBuffers[0], &pos, 0);
    return pos;
}

void XBMixer_Five_One::Shutdown()
{
    for (int i = 0; i < 5; i++) {
        if (s_pSpeakerBuffers[i]) {
            IDirectSoundBuffer_Stop(s_pSpeakerBuffers[i]);
            IDirectSoundBuffer_Release(s_pSpeakerBuffers[i]);
            s_pSpeakerBuffers[i] = 0;
        }
    }
}
