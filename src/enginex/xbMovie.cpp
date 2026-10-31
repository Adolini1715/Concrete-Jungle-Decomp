#include "xbd3d8.h"
#include "XBBaseRenderState.h"
#include "EXBaseSoundManager.h"
#include "EXBaseDisplay.h"
#include "EXWnd.h"
#include "EXMemory.h"
#include "EXMemHeap.h"
#include "XBFileManager.h"
#include "RGBA.h"
#include <string.h>

// XMV movie player: the file is streamed through two 512K read-ahead buffers and fed to the XDK
// decoder packet by packet; each frame is decoded into one of two YUY2 textures and drawn over the
// active window.

namespace xb {
struct _DSMIXBINS;
struct IDirectSoundStream {
    virtual unsigned long __stdcall AddRef() = 0;
    virtual unsigned long __stdcall Release() = 0;
};
}

struct _XMVVIDEO_DESC {
    xb::DWORD Width;
    xb::DWORD Height;
    xb::DWORD FramesPerSecond;
    xb::DWORD AudioStreamCount;
};
enum _XMVRESULT { XMV_NOFRAME = 0, XMV_NEWFRAME = 1, XMV_ENDOFFILE = 2, XMV_FAIL = 3 };
struct XMVDecoder;
typedef long(__stdcall *PFNXMVGETNEXTPACKET)(unsigned long, void **, unsigned long *);
typedef long(__stdcall *PFNXMVRELEASEPREVIOUSPACKET)(unsigned long, __int64, unsigned long);

extern "C" {
long __stdcall XMVDecoder_CreateDecoderForPackets(unsigned long dwFlags, void *pvFirstPacket, unsigned long dwContext,
                                                  PFNXMVGETNEXTPACKET pfnGetNextPacket,
                                                  PFNXMVRELEASEPREVIOUSPACKET pfnReleasePreviousPacket,
                                                  XMVDecoder **ppDecoder);
void __stdcall XMVDecoder_CloseDecoder(XMVDecoder *pDecoder);
void __stdcall XMVDecoder_GetVideoDescriptor(XMVDecoder *pDecoder, _XMVVIDEO_DESC *pDesc);
long __stdcall XMVDecoder_EnableAudioStream(XMVDecoder *pDecoder, unsigned long dwStream, unsigned long dwFlags,
                                            xb::_DSMIXBINS *pMixBins, xb::IDirectSoundStream **ppStream);
void __stdcall XMVDecoder_GetAudioStream(XMVDecoder *pDecoder, unsigned long dwStream,
                                         xb::IDirectSoundStream **ppStream);
long __stdcall XMVDecoder_GetNextFrame(XMVDecoder *pDecoder, xb::D3DSurface *pSurface, _XMVRESULT *pResult,
                                       unsigned long *pTime);
void __stdcall DirectSoundDoWork();
void __stdcall XGSetTextureHeader(unsigned int w, unsigned int h, unsigned int levels, unsigned long usage,
                                  xb::D3DFORMAT fmt, unsigned long pool, xb::D3DTexture *pTex, unsigned long data,
                                  unsigned long pitch);
}

// XDK wrapper (copies emitted here).
struct XMVDecoder {
    void CloseDecoder() { XMVDecoder_CloseDecoder(this); }
    void GetVideoDescriptor(_XMVVIDEO_DESC *pDesc) { XMVDecoder_GetVideoDescriptor(this, pDesc); }
    long EnableAudioStream(unsigned long dwStream, unsigned long dwFlags, xb::_DSMIXBINS *pMixBins,
                           xb::IDirectSoundStream **ppStream)
    {
        return XMVDecoder_EnableAudioStream(this, dwStream, dwFlags, pMixBins, ppStream);
    }
    void GetAudioStream(unsigned long dwStream, xb::IDirectSoundStream **ppStream)
    {
        XMVDecoder_GetAudioStream(this, dwStream, ppStream);
    }
    long GetNextFrame(xb::D3DSurface *pSurface, _XMVRESULT *pResult, unsigned long *pTime)
    {
        return XMVDecoder_GetNextFrame(this, pSurface, pResult, pTime);
    }
};

void XBRenderTexture(void *pTex, float x, float y, float w, float h, int mode, float a7, float srcW, float srcH,
                     float srcX, float srcY, ex::RGBA col);
void xbAudioDSS_SetMixBins(void *pStream);
void xbAudioDSS_SetVolume(void *pStream, int volume);

enum { MOVIE_IDLE, MOVIE_CLOSING, MOVIE_OPENING, MOVIE_PLAYING };

// Read-ahead buffer.
struct XBMovieRAC {
    unsigned char *m_pBuf;      // 0x00 0x80000 bytes
    int m_Pos;                  // 0x04 file offset of the buffer
};

// Player state (sizeof 0xb4, cleared on close).
static struct XBMovie {
    int m_State;                            // 0x00
    int m_AudioStream;                      // 0x04
    void *m_pTexMem[2];                     // 0x08
    xb::D3DTexture m_Tex[2];                // 0x10
    xb::D3DSurface *m_pSurf[2];             // 0x38
    int m_CurTex;                           // 0x40 decoding into
    int m_PrevTex;                          // 0x44 last decoded frame (-1 none)
    XMVDecoder *m_pDecoder;                 // 0x48
    xb::IDirectSoundStream *m_pStream;      // 0x4c
    _XMVVIDEO_DESC m_Desc;                  // 0x50
    int m_FileSize;                         // 0x60
    int m_Frames;                           // 0x64
    unsigned long m_PacketSize;             // 0x68
    unsigned char *m_pPacketMem;            // 0x6c two packets
    unsigned char *m_pPacket;               // 0x70 being read
    unsigned char *m_pDecodePacket;         // 0x74 with the decoder
    unsigned char *m_pFreePacket;           // 0x78 released by the decoder
    bool m_FileOpen;                        // 0x7c
    int m_FilePos;                          // 0x80
    unsigned long m_NextPacketSize;         // 0x84
    int m_RACWrite;                         // 0x88 buffer being filled
    int m_RACRead;                          // 0x8c buffer being copied from
    int m_ReadOffset;                       // 0x90 pending packet read
    unsigned char *m_pReadDest;             // 0x94
    int m_ReadSize;                         // 0x98
    u32 m_Unk9c;                            // 0x9c
    XBMovieRAC m_RAC[2];                    // 0xa0
    bool m_RACInit;                         // 0xb0
} s_Movie;
static int s_MovieVolume = 100;

static s8 xbMovieFopen(const char *pName)
{
    s_Movie.m_FileSize = gFileManager.GetSize(pName);
    if (s_Movie.m_FileSize <= 0) return 0;
    s_Movie.m_FileOpen = 1;
    s_Movie.m_FilePos = 0;
    EXBaseSoundManager::GetSoundManager()->StreamOpen(pName);
    return 1;
}

static bool xbMovieHasReadDone()
{
    EXBaseSoundManager::GetSoundManager()->CommandUpdate(0x10);
    return !EXBaseSoundManager::GetSoundManager()->StreamIsBusy();
}

static void xbMovieFclose()
{
    while (!xbMovieHasReadDone()) {
    }
    s_Movie.m_FileOpen = 0;
    EXBaseSoundManager::GetSoundManager()->StreamClose();
}

static void xbMovieFread(void *pDest, int size, bool wait)
{
    if (s_Movie.m_FilePos + size >= s_Movie.m_FileSize) size = s_Movie.m_FileSize - s_Movie.m_FilePos;
    if (size >= 1) {
        EXBaseSoundManager::GetSoundManager()->StreamRead(pDest, s_Movie.m_FilePos, size);
        s_Movie.m_FilePos += size;
        while (wait && !xbMovieHasReadDone()) {
        }
    }
}

static void xbMovieMemFree(void *p)
{
    if (p) _SystemHeapList[1]->EXFree(p);
}

static void *xbMovieMemAlloc(int size)
{
    return _SystemHeapList[1]->_EXAlloc(size, 0);
}

static s8 xbRACInit()
{
    for (int i = 0; i < 2; i++) {
        s_Movie.m_RAC[i].m_pBuf = (unsigned char *)xbMovieMemAlloc(0x80000);
        if (!s_Movie.m_RAC[i].m_pBuf) return 0;
    }
    s_Movie.m_RACWrite = 0;
    s_Movie.m_RACRead = 0;
    s_Movie.m_RAC[0].m_Pos = 0;
    xbMovieFread(s_Movie.m_RAC[0].m_pBuf, 0x80000, false);
    s_Movie.m_RACInit = 1;
    return 1;
}

static void xbRACKill()
{
    for (int i = 0; i < 2; i++) {
        xbMovieMemFree(s_Movie.m_RAC[i].m_pBuf);
        s_Movie.m_RAC[i].m_pBuf = 0;
    }
}

static void xbRACNextRead(int offset, unsigned char *pDest, int size)
{
    s_Movie.m_ReadOffset = offset;
    s_Movie.m_pReadDest = pDest;
    s_Movie.m_ReadSize = size;
}

static s8 xbRACReadDone()
{
    bool done = xbMovieHasReadDone();
    int end = s_Movie.m_RAC[s_Movie.m_RACRead].m_Pos + 0x80000;
    if (done && s_Movie.m_RACRead == s_Movie.m_RACWrite && s_Movie.m_FilePos < s_Movie.m_FileSize) {
        s_Movie.m_RACWrite = (s_Movie.m_RACWrite + 1) % 2;
        s_Movie.m_RAC[s_Movie.m_RACWrite].m_Pos = s_Movie.m_FilePos;
        xbMovieFread(s_Movie.m_RAC[s_Movie.m_RACWrite].m_pBuf, 0x80000, false);
    }
    if (s_Movie.m_ReadOffset + s_Movie.m_ReadSize > end) {
        if (!done || !xbMovieHasReadDone()) return 0;
        int offset = s_Movie.m_ReadOffset - s_Movie.m_RAC[s_Movie.m_RACRead].m_Pos;
        int first = 0x80000 - offset;
        memcpy(s_Movie.m_pReadDest, s_Movie.m_RAC[s_Movie.m_RACRead].m_pBuf + offset, first);
        s_Movie.m_RACRead = (s_Movie.m_RACRead + 1) % 2;
        memcpy(s_Movie.m_pReadDest + first, s_Movie.m_RAC[s_Movie.m_RACRead].m_pBuf, s_Movie.m_ReadSize - first);
    } else {
        memcpy(s_Movie.m_pReadDest,
               s_Movie.m_RAC[s_Movie.m_RACRead].m_pBuf + s_Movie.m_ReadOffset - s_Movie.m_RAC[s_Movie.m_RACRead].m_Pos,
               s_Movie.m_ReadSize);
    }
    s_Movie.m_ReadSize = 0;
    s_Movie.m_ReadOffset = 0;
    return 1;
}

static long __stdcall xbMovieGetNextPacket(unsigned long context, void **ppPacket, unsigned long *pOffsetToNextPacket)
{
    if (xbRACReadDone()) {
        s_Movie.m_pFreePacket = s_Movie.m_pDecodePacket;
        s_Movie.m_pDecodePacket = s_Movie.m_pPacket;
        s_Movie.m_pPacket = 0;
        *pOffsetToNextPacket = s_Movie.m_NextPacketSize;
        *ppPacket = s_Movie.m_pDecodePacket;
    } else {
        *pOffsetToNextPacket = 0;
        *ppPacket = 0;
    }
    return 0;
}

static long __stdcall xbMovieReleasePreviousPacketReadNext(unsigned long context, __int64 nextReadByteOffset,
                                                           unsigned long nextPacketSize)
{
    if (nextPacketSize) {
        if (nextPacketSize > s_Movie.m_PacketSize) return 0x80004005;
        s_Movie.m_pPacket = s_Movie.m_pFreePacket;
        s_Movie.m_pFreePacket = 0;
        s_Movie.m_NextPacketSize = nextPacketSize;
        xbRACNextRead((int)nextReadByteOffset, s_Movie.m_pPacket, nextPacketSize);
    }
    return 0;
}

static void xbMovieFlushCPU()
{
    __asm wbinvd
}

static xb::D3DSurface *xbMovieTextureGetSurface(xb::D3DTexture *pTex)
{
    if (!pTex) return 0;
    return D3DTexture_GetSurfaceLevel2(pTex, 0);
}

static void xbMovieTextureFreeSurface(xb::D3DSurface *pSurf)
{
    if (pSurf) D3DResource_Release(pSurf);
}

static void xbMovieDeleteTexture(int i)
{
    XBRenderState.SetTexture(0, 0);
    s_Movie.m_Tex[i].BlockUntilNotBusy();
}

static void xbMovieCreateTexture(void *pMem, int i)
{
    XGSetTextureHeader(s_Movie.m_Desc.Width, s_Movie.m_Desc.Height, 1, 0, (xb::D3DFORMAT)0x24, 0, &s_Movie.m_Tex[i], 0,
                       s_Movie.m_Desc.Width * 2);
    s_Movie.m_Tex[i].Register((void *)(((u32)pMem + 0x7f) & ~0x7f));
}

void xbMovieClose()
{
    if (s_Movie.m_State) s_Movie.m_State = MOVIE_CLOSING;
}

void xbMovieSetVolume(int volume)
{
    s_MovieVolume = volume;
    xbAudioDSS_SetVolume(s_Movie.m_pStream, volume);
}

void xbMovieUpdate()
{
    switch (s_Movie.m_State) {
    case MOVIE_PLAYING:
        DirectSoundDoWork();
        if (s_Movie.m_pDecoder) {
            _XMVRESULT result = XMV_NOFRAME;
            do {
                s_Movie.m_pDecoder->GetNextFrame(s_Movie.m_pSurf[s_Movie.m_CurTex], &result, 0);
            } while (!s_Movie.m_Frames && result == XMV_NOFRAME);
            if (result == XMV_NEWFRAME) {
                xbMovieFlushCPU();
                s_Movie.m_PrevTex = s_Movie.m_CurTex;
                s_Movie.m_CurTex = (s_Movie.m_CurTex + 1) % 2;
            }
            if (result == XMV_ENDOFFILE || result == XMV_FAIL) xbMovieClose();
            s_Movie.m_Frames++;
            EXWnd *pWnd = EXBaseDisplay::ActiveWnd();
            if (s_Movie.m_PrevTex != -1 && pWnd) {
                const EXRect &r = pWnd->Rect();
                EXPoint pos = r.TopLeft();
                XBRenderTexture(&s_Movie.m_Tex[s_Movie.m_PrevTex], (float)pos.x, (float)pos.y, (float)r.w, (float)r.h, 3,
                                0.0f, (float)s_Movie.m_Desc.Width, (float)s_Movie.m_Desc.Height, 0.0f, 0.0f,
                                ex::RGBA(0xff, 0x80, 0x80, 0x80));
                XBRenderState.SetTexture(0, 0);
            }
        }
        break;
    case MOVIE_OPENING:
        if (xbMovieHasReadDone()) {
            unsigned char *pFirst = s_Movie.m_RAC[s_Movie.m_RACWrite].m_pBuf;
            unsigned long packetSize = ((unsigned long *)pFirst)[2];
            unsigned long nextSize = ((unsigned long *)pFirst)[1];
            s_Movie.m_pPacketMem = (unsigned char *)xbMovieMemAlloc(packetSize * 2);
            s_Movie.m_pPacket = s_Movie.m_pPacketMem;
            s_Movie.m_PacketSize = packetSize;
            s_Movie.m_pDecodePacket = s_Movie.m_pPacketMem + packetSize;
            XMVDecoder_CreateDecoderForPackets(4, pFirst, 0, xbMovieGetNextPacket, xbMovieReleasePreviousPacketReadNext,
                                               &s_Movie.m_pDecoder);
            if (!s_Movie.m_pDecoder) {
                s_Movie.m_State = MOVIE_CLOSING;
            } else {
                s_Movie.m_NextPacketSize = nextSize;
                xbRACNextRead(0, s_Movie.m_pPacketMem, nextSize);
                s_Movie.m_pDecoder->GetVideoDescriptor(&s_Movie.m_Desc);
                if (s_Movie.m_Desc.Height < 8 || s_Movie.m_Desc.Height > 0x400 || s_Movie.m_Desc.Width < 8 ||
                    s_Movie.m_Desc.Width > 0x400) {
                    s_Movie.m_State = MOVIE_CLOSING;
                    break;
                }
                for (int i = 0; i < 2; i++) {
                    s_Movie.m_pTexMem[i] = xbMovieMemAlloc(s_Movie.m_Desc.Width * s_Movie.m_Desc.Height * 4 + 0x80);
                    xbMovieCreateTexture(s_Movie.m_pTexMem[i], i);
                    s_Movie.m_pSurf[i] = xbMovieTextureGetSurface(&s_Movie.m_Tex[i]);
                    if (!s_Movie.m_pTexMem[i] || !s_Movie.m_pSurf[i]) {
                        s_Movie.m_State = MOVIE_CLOSING;
                        return;
                    }
                }
                if (!EXBaseSoundManager::GetSoundManager()->GetMusicEnabledWithMovie() && s_Movie.m_Desc.AudioStreamCount > 0) {
                    if (s_Movie.m_AudioStream >= (int)s_Movie.m_Desc.AudioStreamCount) s_Movie.m_AudioStream = 0;
                    s_Movie.m_pDecoder->EnableAudioStream(s_Movie.m_AudioStream, 0, 0, 0);
                    s_Movie.m_pDecoder->GetAudioStream(s_Movie.m_AudioStream, &s_Movie.m_pStream);
                    xbAudioDSS_SetMixBins(s_Movie.m_pStream);
                }
                xbMovieSetVolume(s_MovieVolume);
                s_Movie.m_CurTex = 0;
                s_Movie.m_PrevTex = -1;
                s_Movie.m_State = MOVIE_PLAYING;
            }
        }
        break;
    case MOVIE_CLOSING:
        if (xbMovieHasReadDone()) {
            if (s_Movie.m_pStream) s_Movie.m_pStream->Release();
            if (s_Movie.m_pDecoder) s_Movie.m_pDecoder->CloseDecoder();
            for (int i = 0; i < 2; i++) {
                xbMovieTextureFreeSurface(s_Movie.m_pSurf[i]);
                xbMovieDeleteTexture(i);
                xbMovieMemFree(s_Movie.m_pTexMem[i]);
            }
            xbMovieFclose();
            xbMovieMemFree(s_Movie.m_pPacketMem);
            xbRACKill();
            memset(&s_Movie, 0, sizeof(s_Movie));
            EXBaseSoundManager::GetSoundManager()->PostMovieReset();
            s_Movie.m_State = MOVIE_IDLE;
        }
        break;
    }
}

void xbMovieOpen(const char *pName, int audioStream)
{
    if (!s_Movie.m_State) {
        s_Movie.m_State = MOVIE_OPENING;
        s_Movie.m_AudioStream = audioStream;
        EXBaseSoundManager::GetSoundManager()->PreMovieReset();
        if (!xbMovieFopen(pName) || !xbRACInit()) xbMovieClose();
    }
}

s8 xbMovieIsBusy()
{
    return s_Movie.m_State != MOVIE_IDLE;
}

s8 xbMovieCheckFreeMem()
{
    void *p = xbMovieMemAlloc(0x400000);
    if (!p) return 0;
    xbMovieMemFree(p);
    return 1;
}
