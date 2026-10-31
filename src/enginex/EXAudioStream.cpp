#include "EXAudioStream.h"
#include "EXAudio.h"
#include <string.h>

extern "C" int printf(const char *, ...);
void psiStreamPause(int handle);
void psiStreamUnPause(int handle);
void psiStreamStop(int handle);
void psiStreamSetVolume(int handle, int volume);
void psiSetStreamFrequency(int handle, int frequency);
int psiStreamGetPlayPos(int handle);
void psiStreamPlay(int handle, int arg);
void psiStreamSetToNull(int size, char *pData);
void psiStreamMemCpy(int handle, int dest, char *pSrc, int size, int channels);
int psiStreamHasMemCpyFinished(int handle);
int psiStreamCreatePlaybackBuffer(int frequency, int channels, int size, int flags, int unused);

// Marker in a stream header (sizeof 0x14).
struct StreamMarker {
    int m_Section;      // 0x00
    int m_Pos;          // 0x04
    int m_Type;         // 0x08 5 stop, 6/7 loop, 9 end
    int m_LoopPos;      // 0x0c
    int m_Next;         // 0x10 marker index to continue at
};

// Section of a music header (sizeof 0x18).
struct StreamSection {
    int m_Unk00;
    int m_Pos;          // 0x04
    int m_Unk08[3];
    int m_Marker;       // 0x14 first marker
};

StreamBufferData StreamBuffers[24];
_STARTMUSICDATA StartMusicData;
MusicFilePosStore MusicFilePosStoreData[20];
StreamIntialiseDetails StreamWaitingIntialise[64];
StreamMemCpyHandle StreamMemCpyHandles[7];
StreamData Streams[7];
static char s_MonoHeaders[5][0x2e0];
static int s_InitState = 2;
int MusicForceRestart = 1;
static unsigned int s_MusicAge = 1;
static int s_SFXUse = 1;
static int s_StatFrames = 1;
static int s_MusicUse = 1;
int StartedOnFrameCounter;
static int s_FirstUpdate;
int StreamOffsetToAudioData;
static int s_WaitRead;
static int s_WaitWrite;
int StreamsInitialised;
static char *s_pStreamMem;
StreamLookupFileDetails *StreamLookupFileData;
static char *s_pMonoHeader;
static int s_Unk4ee438;
int MusicStartSection;
static char *s_pHeaderBuf;
int MusicFilePosStoreIndex;
static int s_VolumeDebug;
static char s_HeaderName[0x100];
static int s_InitHash;
static StreamData *s_pInitStream;
static int s_InitForceRestart;
static int s_InitStartSection;
static int s_InitSection;
static int s_UpdateCount;
static int s_StreamUse[7];
static int s_StatFrame;

void ES_MusicStoreMyFilePosCreateNew(StreamData *s)
{
    int found = -1;
    int hash = s->m_Hash;
    unsigned int oldest = 0xffffffff;
    s8 isNew = 1;
    for (int i = 0; i < 20; i++) {
        if (MusicFilePosStoreData[i].m_Age < oldest) {
            oldest = MusicFilePosStoreData[i].m_Age;
            found = i;
        }
        if (MusicFilePosStoreData[i].m_Hash == hash) {
            found = i;
            isNew = 0;
            break;
        }
    }
    if (found == -1) found = 0;
    MusicFilePosStoreData[found].m_Hash = hash;
    MusicFilePosStoreData[found].m_Pos = s->m_ReadPos;
    if (isNew) MusicFilePosStoreData[found].m_Age = s_MusicAge++;
}

void ES_MusicFindMyStoredFilePos(StreamData *s)
{
    for (int i = 0; i < 20; i++) {
        if (MusicFilePosStoreData[i].m_Hash == s->m_Hash) {
            s->m_ReadPos = MusicFilePosStoreData[i].m_Pos;
            return;
        }
    }
}

static int ES_StreamAlign(int v)
{
    return v / 64 * 64;
}

int ES_FindpsiStreamHandle(int i)
{
    return Streams[i].m_psiHandle;
}

int ES_FindStreamShutdownVolume(int i)
{
    return Streams[i].m_ShutdownVolume;
}

int ES_HasStreamEnded(int i)
{
    int state = Streams[i].m_State;
    if (state == 0xc || state == 0) return 1;
    return 0;
}

void ES_UnlockStream(int i)
{
    Streams[i].m_Unlocked = 1;
    if (Streams[i].m_State == 0xc) {
        Streams[i].m_State = 0;
        Streams[i].m_Active = 0;
    }
}

static int ES_IsStreamInitialised(StreamData *s)
{
    if (s->m_State == 0 || s->m_State == 1 || s->m_State == 0xc) return 0;
    if (s->m_Active) return 1;
    return 0;
}

StreamData *ES_GetStreamPointer(int i)
{
    return &Streams[i];
}

void ES_StreamPause(StreamData *s)
{
    if (s->m_Playing && !s->m_Fade.m_Paused) {
        psiStreamPause(s->m_psiHandle);
        s->m_Paused = 1;
    }
}

void ES_StreamUnPause(StreamData *s)
{
    if (!s->m_Fade.m_Paused) {
        psiStreamUnPause(s->m_psiHandle);
        s->m_Paused = 0;
    }
}

void ES_StreamKeyOn(int i, int arg)
{
    Streams[i].m_KeyOn = 1;
    Streams[i].m_KeyOnArg = arg;
}

static void ES_LimitStreamBufferIndex(int *p, StreamData *s, int line)
{
    int n = s->m_NumBuffers;
    int v = *p;
    if (!n) {
        gpEXAudio->SFXprintf("ES_LimitStreamBufferIndex() - Stream[%d] has no buffers! Line %d\n", s->m_Index, line);
        for (;;) {
        }
    }
    while (v < 0) v += n;
    while (v >= n) v -= n;
    *p = v;
}

static void ES_LimitStreamLastReadBufferIndex(int *p, StreamData *s)
{
    int n = s->m_NumReadBuffers;
    int v = *p;
    if (!n) {
        gpEXAudio->SFXprintf("ES_LimitStreamLastReadBufferIndex() - Stream[%d] has no buffers!\n", s->m_Index);
        for (;;) {
        }
    }
    while (v < 0) v += n;
    while (v >= n) v -= n;
    *p = v;
}

static void ES_LimitPendingBufferIndex(int *p)
{
    int v = *p;
    while (v < 0) v += 5;
    while (v >= 5) v -= 5;
    *p = v;
}

static int ES_HasStreamMemCpyFinished(StreamData *s)
{
    return StreamMemCpyHandles[s->m_Index].m_State != 9;
}

static void ES_InitialiseGetNextStream()
{
    s_WaitRead = (s_WaitRead + 1) & 0x3f;
    s_InitState = 2;
}

static int ES_RequestStreamBuffers(StreamData *s)
{
    if (s->m_KeyOff) return 0;
    if (s->m_Channels == 1) {
        if (s->m_Active) return 1;
        int group = s->m_Index - 1;
        if (group < 0) group = 0;
        StreamBufferData *b = &StreamBuffers[group * 4];
        for (int i = 0; i < 2; i++) {
            char *pData = b->m_pData;
            memset(b, 0, sizeof(StreamBufferData));
            b->m_pData = pData;
            s->m_pBuffers[i] = b;
            s->m_pBuffers[i]->m_File = gpEXAudio->psiAsyncOpenFile(&s->m_File, FILEMAN_AUDIO, 0xffffffff);
            if (s->m_pBuffers[i]->m_File == -1) {
                gpEXAudio->SFXprintf("### ES_InitialiseStreamUpdate() FAILED with psiAsyncOpenFile().\n");
                for (;;) {
                }
            }
            b++;
        }
        s->m_NumBuffers = 2;
        return 1;
    }
    StreamData *o = &Streams[1 - s->m_Index];
    if (!s->m_Active) {
        StreamBufferData *bufs[4];
        int n;
        if (!o->m_State) {
            n = 4;
            bufs[0] = &StreamBuffers[0];
            bufs[1] = &StreamBuffers[1];
            bufs[2] = &StreamBuffers[2];
            bufs[3] = &StreamBuffers[3];
        } else {
            n = 2;
            if (o->m_NumBuffers == 4) {
                if (o->m_KeyOff) return 0;
                int last = o->m_ReadBuffer;
                int next = last + 1;
                int found = 0;
                ES_LimitStreamLastReadBufferIndex(&next, o);
                int stopped = 0;
                int i;
                for (i = 0; i < 4; i++) {
                    if (i != last && i != next && !gpEXAudio->psiAsyncHasReadFinished(o->m_pBuffers[i]->m_File)) {
                        gpEXAudio->psiAsyncStopAccess(o->m_pBuffers[i]->m_File);
                        stopped = 1;
                        o->m_pBuffers[i]->m_Stopped = 1;
                    }
                }
                if (stopped) return 0;
                StreamBufferData *pLast = 0;
                StreamBufferData *pNext = 0;
                for (i = 0; i < 4; i++) {
                    if (i == last) {
                        pLast = o->m_pBuffers[i];
                    } else if (i == next) {
                        pNext = o->m_pBuffers[i];
                    } else {
                        gpEXAudio->psiAsyncCloseFile(o->m_pBuffers[i]->m_File);
                        bufs[found++] = o->m_pBuffers[i];
                    }
                }
                int idx;
                for (int k = 1; k < 3; k++) {
                    idx = o->m_CurBuffer - k;
                    ES_LimitStreamBufferIndex(&idx, o, 1000);
                    if (!o->m_pBuffers[idx]->m_Reading && o->m_pBuffers[idx]->m_Read) break;
                }
                if (idx == last) {
                    o->m_CurBuffer = 1;
                } else {
                    o->m_CurBuffer = 0;
                    if (idx != next) idx = next;
                }
                o->m_ReadPos = o->m_pBuffers[idx]->m_FilePos;
                o->m_pMarker = (char *)o->m_pBuffers[idx]->m_pMarker;
                o->m_pSection = (char *)o->m_pBuffers[idx]->m_pSection;
                o->m_NumBuffers -= 2;
                o->m_pBuffers[0] = pLast;
                o->m_pBuffers[1] = pNext;
                o->m_ReadBuffer = 0;
                if (found != 2) {
                    gpEXAudio->SFXprintf("Failed to find 2 Free Buffers Stream2->FileBufferCount==2 WAS 4\n");
                    for (;;) {
                    }
                }
            } else if (o->m_NumBuffers == 2) {
                int found = 0;
                for (StreamBufferData *p = &StreamBuffers[0]; p < &StreamBuffers[4]; p++) {
                    int isFree = 1;
                    for (int j = 0; j < o->m_NumBuffers; j++)
                        if (o->m_pBuffers[j] == p) isFree = 0;
                    if (isFree) bufs[found++] = p;
                }
                if (found != 2) {
                    for (;;) {
                    }
                }
            } else {
                bufs[0] = &StreamBuffers[0];
                bufs[1] = &StreamBuffers[1];
            }
        }
        for (int i = 0; i < n; i++) {
            StreamBufferData *b = bufs[i];
            char *pData = b->m_pData;
            memset(b, 0, sizeof(StreamBufferData));
            b->m_pData = pData;
            s->m_pBuffers[i] = b;
            s->m_pBuffers[i]->m_File = gpEXAudio->psiAsyncOpenFile(&s->m_File, FILEMAN_AUDIO, 0xffffffff);
            if (s->m_pBuffers[i]->m_File == -1) {
                gpEXAudio->SFXprintf("### ES_InitialiseStreamUpdate() FAILED with psiAsyncOpenFile().\n");
                for (;;) {
                }
            }
        }
        s->m_NumBuffers = n;
        return 1;
    }
    if (s->m_NumBuffers == 4) return 1;
    if (o->m_State) return 1;
    for (int i = 0; i < 2; i++) {
        char *pData = o->m_pBuffers[i]->m_pData;
        memset(o->m_pBuffers[i], 0, sizeof(StreamBufferData));
        o->m_pBuffers[i]->m_pData = pData;
        s->m_pBuffers[i + 2] = o->m_pBuffers[i];
        s->m_pBuffers[i + 2]->m_File = gpEXAudio->psiAsyncOpenFile(&s->m_File, FILEMAN_AUDIO, 0xffffffff);
        if (s->m_pBuffers[i + 2]->m_File == -1) {
            gpEXAudio->SFXprintf("### ES_InitialiseStreamUpdate() FAILED with psiAsyncOpenFile().\n");
            for (;;) {
            }
        }
    }
    s->m_NumBuffers = 4;
    if (s->m_ReadBuffer == 1) {
        s->m_CurBuffer = 1 - s->m_CurBuffer;
        s->m_ReadBuffer = 0;
        StreamBufferData *t = s->m_pBuffers[0];
        s->m_pBuffers[0] = s->m_pBuffers[1];
        s->m_pBuffers[1] = t;
    }
    return 1;
}

int ES_MonoStreamActiveCount()
{
    int n = 0;
    for (int i = 2; i < 7; i++)
        if (Streams[i].m_State) n++;
    return n;
}

int ES_StreamActiveCount()
{
    int n = 0;
    for (int i = 0; i < 7; i++)
        if (Streams[i].m_State) n++;
    return n;
}

static void ES_JumpRequestComplete(StreamData *s)
{
    gpEXAudio->m_StreamJumpDone = gpEXAudio->m_StreamJump;
    s->m_Unk370 = s->m_Unk374;
    s->m_Unk374 = 0;
}

static void ES_SetupEndMarker(TestMarker *m, int pos)
{
    m->m_Armed = 1;
    m->m_Pos = pos;
    m->m_Pending = 1;
}

static void ES_ClearEndMarker(TestMarker *m)
{
    m->m_Armed = 0;
    m->m_Pos = -1;
    m->m_Pending = 0;
}

static int ES_CheckEndMarker(StreamData *s, TestMarker *m)
{
    int wrapped = 0;
    if (s->m_PlayPos < s->m_LastPlayPos) wrapped = 1;
    if (m->m_Armed) {
        if (s->m_PlayPos < m->m_Pos) m->m_Armed = 0;
        if (wrapped) m->m_Armed = 0;
        wrapped = 0;
    }
    if (!m->m_Armed && (s->m_PlayPos > m->m_Pos || wrapped)) {
        m->m_Pending = 0;
        return 1;
    }
    return 0;
}

static void ES_HandleStreamBufferLoaded(StreamData *s)
{
    int old = s->m_CurBuffer;
    int next = old + 1;
    ES_LimitStreamBufferIndex(&next, s, 0x709);
    s->m_CurBuffer = next;
    s->m_pBuffers[next]->m_NextOffset = s->m_pBuffers[old]->m_Size + s->m_pBuffers[old]->m_DestOffset;
    s->m_ReadPos = s->m_pBuffers[old]->m_FilePos;
    if (s->m_pBuffers[next]->m_NextOffset == s->m_BufferSize) s->m_pBuffers[next]->m_NextOffset = 0;
}

static int ES_StreamCloseBufferAccess(StreamData *s)
{
    if (!s->m_NumBuffers) return 1;
    int stopped = 0;
    int i;
    for (i = 0; i < s->m_NumBuffers; i++) {
        if (!gpEXAudio->psiAsyncHasReadFinished(s->m_pBuffers[i]->m_File)) {
            gpEXAudio->psiAsyncStopAccess(s->m_pBuffers[i]->m_File);
            stopped = 1;
            s->m_pBuffers[i]->m_Stopped = 1;
        }
    }
    if (stopped) return 0;
    for (i = 0; i < s->m_NumBuffers; i++) gpEXAudio->psiAsyncCloseFile(s->m_pBuffers[i]->m_File);
    s->m_NumBuffers = 0;
    return 1;
}

static void ES_SaveMarkersForThisBuffer(StreamData *s, int i)
{
    s->m_pBuffers[i]->m_pMarker = s->m_pMarker;
    s->m_pBuffers[i]->m_pSection = s->m_pSection;
}

static void ES_UpdateMusicMarkers(StreamData *s, int reset)
{
    s->m_pBuffers[s->m_CurBuffer]->m_Size = 0;
    s->m_pBuffers[s->m_CurBuffer]->m_Loading = 1;
    int cur = s->m_CurBuffer;
    if (cur == s->m_ReadBuffer && !s->m_pBuffers[cur]->m_Pending && s->m_Started == 1) return;
    s->m_pBuffers[cur]->m_Pending = 0;
    s->m_pBuffers[s->m_CurBuffer]->m_Loading = 0;
    s->m_pBuffers[cur]->m_DestOffset = s->m_pBuffers[cur]->m_NextOffset;
    ES_ClearEndMarker(&s->m_pBuffers[cur]->m_LoopMarker);
    ES_ClearEndMarker(&s->m_pBuffers[cur]->m_JumpMarker);
    ES_ClearEndMarker(&s->m_pBuffers[cur]->m_EndMarker);
    int size = s->m_BufferSize;
    if (reset) s->m_pBuffers[cur]->m_DestOffset = 0;
    else size -= s->m_pBuffers[cur]->m_DestOffset;
    if (size > s->m_BufferSize) size = s->m_BufferSize;
    s->m_pBuffers[cur]->m_HasMarker = 0;
    s->m_pBuffers[cur]->m_Size = size;
    s->m_pBuffers[cur]->m_SetToNull = 0;
    s->m_pBuffers[cur]->m_ReadPos = s->m_ReadPos;
    if (s->m_LastBuffer) {
        if (s->m_pBuffers[cur]->m_Size > s->m_ReadSize) s->m_pBuffers[cur]->m_Size = s->m_ReadSize;
        s->m_pBuffers[cur]->m_SetToNull = 1;
        return;
    }
    if (s->m_ReadPos > ((StreamMarker *)s->m_pMarker)->m_Pos) {
        do s->m_pMarker += sizeof(StreamMarker);
        while (s->m_ReadPos > ((StreamMarker *)s->m_pMarker)->m_Pos);
    }
    while (s->m_ReadPos + size > ((StreamMarker *)s->m_pMarker)->m_Pos) {
        if (((StreamMarker *)s->m_pMarker)->m_Type != 7) {
            if (s->m_Unk378 && !s->m_JumpMarker.m_Pending) {
                ES_SetupEndMarker(&s->m_pBuffers[cur]->m_JumpMarker,
                                  s->m_pBuffers[cur]->m_DestOffset + s->m_pBuffers[cur]->m_Size);
                s->m_Unk378 = 0;
            }
            if (s->m_Unk37c) {
                s->m_Unk378 = s->m_Unk37c;
                s->m_Unk37c = 0;
            }
        }
        int type = ((StreamMarker *)s->m_pMarker)->m_Type;
        if (type == 9 || type == 5) {
            s->m_pBuffers[cur]->m_Size = ES_StreamAlign(((StreamMarker *)s->m_pMarker)->m_Pos - s->m_ReadPos);
            if (((StreamMarker *)s->m_pMarker)->m_Type == 5) s->m_Unk05c = 1;
            ES_SetupEndMarker(&s->m_pBuffers[cur]->m_EndMarker,
                              s->m_pBuffers[cur]->m_DestOffset + s->m_pBuffers[cur]->m_Size);
            s->m_LastBuffer = 1;
            ES_SaveMarkersForThisBuffer(s, cur);
            return;
        }
        if (type != 7) {
            s->m_pBuffers[cur]->m_HasMarker = 1;
            s->m_pBuffers[cur]->m_MarkerOffset = ES_StreamAlign(((StreamMarker *)s->m_pMarker)->m_Pos - s->m_ReadPos);
        }
        type = ((StreamMarker *)s->m_pMarker)->m_Type;
        if (type == 6 || type == 7) {
            s->m_pBuffers[cur]->m_Size = ES_StreamAlign(((StreamMarker *)s->m_pMarker)->m_Pos - s->m_ReadPos);
            s->m_pBuffers[cur]->m_FilePos = ES_StreamAlign(((StreamMarker *)s->m_pMarker)->m_LoopPos);
            s->m_LoopEnd = ((StreamMarker *)s->m_pMarker)->m_Pos;
            s->m_LoopStart = s->m_pBuffers[cur]->m_FilePos;
            s->m_pMarker = s->m_pMarkers + ((StreamMarker *)s->m_pMarker)->m_Next * sizeof(StreamMarker);
            s->m_pSection = s->m_pSections + ((StreamMarker *)s->m_pMarker)->m_Section * sizeof(StreamSection);
            ES_SaveMarkersForThisBuffer(s, cur);
            return;
        }
        s->m_pMarker += sizeof(StreamMarker);
    }
    ES_SaveMarkersForThisBuffer(s, cur);
    s->m_pBuffers[cur]->m_FilePos = s->m_pBuffers[cur]->m_Size + s->m_ReadPos;
}

static void ES_StreamKeyOffMemCpy(StreamData *s)
{
    int i = s->m_Index;
    StreamMemCpyHandles[i].m_State = 0;
    StreamMemCpyHandles[i].m_Busy = 0;
}

static void ES_StartStreamMemCpy(int handle, int dest, char *pSrc, int size, StreamData *s)
{
    int i = s->m_Index;
    StreamMemCpyHandles[i].m_psiHandle = handle;
    StreamMemCpyHandles[i].m_DestOffset = dest;
    StreamMemCpyHandles[i].m_pSrc = pSrc;
    StreamMemCpyHandles[i].m_Size = size;
    StreamMemCpyHandles[i].m_State = 9;
    StreamMemCpyHandles[i].m_Done = 0;
    StreamMemCpyHandles[i].m_Unk14 = 0;
    StreamMemCpyHandles[i].m_pStream = s;
    StreamMemCpyHandles[i].m_Busy = 0;
}

void ES_InitialiseAudioStreamSystem()
{
    StreamsInitialised = 0;
    s_InitState = 2;
    s_FirstUpdate = 1;
    for (int i = 0; i < 7; i++) {
        Streams[i].m_State = 0;
        Streams[i].m_Active = 0;
        Streams[i].m_Index = i;
        StreamMemCpyHandles[i].m_State = 0;
        StreamMemCpyHandles[i].m_Busy = 0;
    }
}

static void ES_InitialStreamBuffer()
{
    if (!s_FirstUpdate) return;
    memset(MusicFilePosStoreData, 0, sizeof(MusicFilePosStoreData));
    s_Unk4ee438 = 1;
    s_FirstUpdate = 0;
    int size = 0x1100;
    int i;
    for (i = 0; i < 7; i++) {
        if (i < 2) {
            if (i == 0) size += 0x82000;
        } else
            size += 0x21000;
    }
    char *p = gpEXAudio->psiAsyncCreateBuffer(size, 0);
    s_pStreamMem = p;
    p += 0x800;
    s_pHeaderBuf = p;
    p += 0x900;
    for (i = 0; i < 7; i++) {
        Streams[i].m_psiHandle = -1;
        if (i < 2) {
            Streams[i].m_psiHandle = psiStreamCreatePlaybackBuffer(0xac44, 2, 0x10000, 0x80, 0);
            if (i == 0) {
                for (int j = 0; j < 4; j++) {
                    StreamBuffers[j].m_pData = p;
                    p += 0x20800;
                }
            }
        } else {
            if (!gpEXAudio->m_pStreamMem) gpEXAudio->m_pStreamMem = p;
            Streams[i].m_psiHandle = psiStreamCreatePlaybackBuffer(0x5622, 1, 0x8000, 1, 0);
            for (int j = 0; j < 2; j++) {
                StreamBuffers[i * 4 - 4 + j].m_pData = p;
                p += 0x10800;
            }
        }
    }
}

static int ES_StreamAsyncReadFile(int file, int pos, int size, char *pDest, StreamData *s)
{
    int priority = s->m_ReadCount + (s->m_Music ? 0 : 0x1000000);
    if (++s->m_ReadCount >= 0x1000000) s->m_ReadCount = 0;
    if (s->m_Music && pos + size > s->m_FileEnd) {
        printf("Stream read of end of file -> Appling counter measure.\n");
        s->m_pSection = s->m_pSections;
        s->m_pMarker = s->m_pMarkers;
        s->m_ReadPos = ES_StreamAlign(((StreamSection *)s->m_pSections)->m_Pos);
        psiStreamSetToNull(size, pDest);
        return 0;
    }
    gpEXAudio->psiAsyncReadFile(file, s->m_DataOffset + pos, size, pDest, priority);
    return 1;
}

static void ES_CancelMusicCrossFades()
{
    StartMusicData.m_Start = 0;
}

void SFXStartMusicXFade(int hash, int section, int fadeIn, int fadeOut, int fadeAndStop, XFileHandle *pFile,
                        int forceRestart)
{
    if (fadeOut < 0) fadeOut = 0;
    else if (fadeOut > 150) fadeOut = 150;
    if (fadeIn < 0) fadeIn = 0;
    else if (fadeIn > 150) fadeIn = 150;
    if (gpEXAudio->m_Debug[3] && gpEXAudio->m_MusicFader.m_State == -1 && gpEXAudio->m_MusicFader.m_Stop)
        gpEXAudio->SFXprintf("EW: Music %6.6X Started during FadeAndStop\n", hash);
    if (StartMusicData.m_Hash == hash && StartMusicData.m_Section == section) {
        if (!gpEXAudio->m_Debug[3]) return;
        gpEXAudio->SFXprintf("EW: Crossfade stopped by !ForceReStart flag\n");
    } else {
        StartMusicData.m_Start = 1;
        StartMusicData.m_Hash = hash;
        StartMusicData.m_Section = section;
        StartMusicData.m_FadeIn = gpEXAudio->ES_ConvertValueToMusicFade(fadeIn);
        StartMusicData.m_FadeOut = gpEXAudio->ES_ConvertValueToMusicFade(fadeOut);
        StartMusicData.m_FadeAndStop = fadeAndStop;
        StartMusicData.m_File = *pFile;
        StartMusicData.m_ForceRestart = forceRestart;
        StartMusicData.m_Unk1bc = 0;
    }
    if (gpEXAudio->m_Debug[3]) {
        char sTrue[] = "True";
        char sFalse[] = "False";
        gpEXAudio->SFXprintf("EI: Crossfade Start: In %d Out %d Fade? %s Music 0%x Section %d ForceReStart? %s\n",
                             fadeOut, fadeIn, fadeAndStop ? sTrue : sFalse, hash, section,
                             forceRestart ? sTrue : sFalse);
    }
}

void ES_StreamKeyOff(int i)
{
    StreamData *s = &Streams[i];
    if (s->m_State == 1) {
        StreamWaitingIntialise[s->m_WaitIndex].m_State = 0xd;
        s->m_State = 0xc;
        return;
    }
    s->m_KeyOff = 1;
    ES_StreamKeyOffMemCpy(s);
    s->m_KeyOn = 0;
    if (s->m_Playing) {
        psiSetStreamFrequency(s->m_psiHandle, s->m_Frequency);
        if (s->m_Fade.m_Paused) {
            s->m_Fade.m_Paused = 0;
            psiStreamUnPause(s->m_psiHandle);
            s->m_Paused = 0;
        }
        if (s->m_Paused) ES_StreamUnPause(s);
        psiStreamSetVolume(s->m_psiHandle, 0);
        psiStreamStop(s->m_psiHandle);
        s->m_Playing = 0;
    }
}

void ES_MusicSetVolume()
{
    for (int i = 0; i < 2; i++) {
        StreamData *s = &Streams[i];
        if (ES_IsStreamInitialised(s)) {
            int vol = gpEXAudio->m_MusicVolume * gpEXAudio->m_Unk200 * s->m_Volume / 10000;
            EXAudio *a = gpEXAudio;
            vol = vol * s->m_ShutdownVolume / 100;
            vol = vol * vol / 100;
            vol = vol * s->m_Fade.m_Volume / 10000000;
            vol = vol * a->m_MusicFader.m_Volume / 10000000;
            if (s->m_Paused) vol = 0;
            if (a->m_Debug[5]) vol = 100;
            if (a->m_Debug[4] && s_VolumeDebug-- < 0) {
                s_VolumeDebug = 60;
                a->SFXprintf("Music CombinedVolume = %d Base Vol = %d\n", vol, s->m_Volume);
            }
            psiStreamSetVolume(s->m_psiHandle, vol);
        }
    }
}

void ES_MusicStoreMyFilePos(StreamData *s)
{
    if (!s->m_Music) return;
    EXAudio *a = gpEXAudio;
    if (a->m_MusicFader.m_State == -1) return;
    if (a->m_MusicFader.m_State == 0 && a->m_MusicFader.m_Volume != 10000000) return;
    if (!s->m_Playing) return;
    for (int i = 0; i < 20; i++) {
        if (MusicFilePosStoreData[i].m_Hash == s->m_Hash) {
            int pos = ES_StreamAlign(s->m_psiPos + s->m_PlayFilePos);
            if (s->m_LoopEnd && pos >= ES_StreamAlign(s->m_LoopEnd)) pos -= ES_StreamAlign(s->m_LoopEnd - s->m_LoopStart);
            MusicFilePosStoreData[i].m_Pos = pos;
            if (pos > MusicFilePosStoreData[i].m_MaxPos) MusicFilePosStoreData[i].m_MaxPos = pos;
            if (pos > s->m_FileEnd) MusicFilePosStoreData[i].m_Pos = s->m_FileEnd;
            return;
        }
    }
}

void ES_StopMusic()
{
    for (int i = 0; i < 2; i++) {
        ES_StreamKeyOff(i);
        ES_UnlockStream(i);
    }
    ES_CancelMusicCrossFades();
memset(MusicFilePosStoreData, 0, sizeof(MusicFilePosStoreData));
    StartMusicData.m_Hash = -1;
}

void ES_MusicPause()
{
    for (int i = 0; i < 2; i++) {
        StreamData *s = &Streams[i];
        if (ES_IsStreamInitialised(s) && s->m_Playing) {
            psiStreamSetVolume(s->m_psiHandle, 0);
            ES_StreamPause(s);
            s->m_Unk064 = 1;
        }
    }
}

void ES_MusicUnPause()
{
    for (int i = 0; i < 2; i++) {
        StreamData *s = &Streams[i];
        if (ES_IsStreamInitialised(s) && s->m_Unk064) {
            ES_StreamUnPause(s);
            s->m_Unk064 = 0;
        }
    }
}

void ES_StopAllStreams()
{
    ES_StopMusic();
    for (int i = 2; i < 7; i++) {
        ES_StreamKeyOff(i);
        ES_UnlockStream(i);
    }
}

int ES_IsStreamPlaying(int i)
{
    if (ES_IsStreamInitialised(&Streams[i])) return 1;
    return 0;
}

void ES_UpdateStreamMemCpy()
{
    for (int i = 0; i < 7; i++) {
        StreamMemCpyHandle *h = &StreamMemCpyHandles[i];
        StreamData *s = h->m_pStream;
        if (h->m_State == 9 && !h->m_Busy) {
            int playPos = -1;
            if (s->m_Playing) {
                playPos = psiStreamGetPlayPos(s->m_psiHandle);
                if (playPos == -2) goto check;
            } else if (s->m_Loaded) {
                goto check;
            }
            {
                int done = h->m_Done;
                int size = h->m_Size - done;
                if (size > s->m_ReadSize) size = s->m_ReadSize;
                while (h->m_DestOffset + done >= s->m_HalfSize) h->m_DestOffset -= s->m_HalfSize;
                int dest = h->m_DestOffset + done;
                if (dest + size > s->m_HalfSize) size = s->m_HalfSize - done - h->m_DestOffset;
                if (playPos == -1 || dest + size < playPos || dest > playPos) {
                    if (size) psiStreamMemCpy(h->m_psiHandle, dest, h->m_pSrc + done, size, s->m_Channels);
                    h->m_Busy = 1;
                    h->m_Copied = size;
                }
            }
        }
    check:
        if (h->m_Busy && psiStreamHasMemCpyFinished(h->m_psiHandle)) {
            h->m_Busy = 0;
            s->m_CopiedPos = h->m_Done + h->m_Copied + h->m_DestOffset;
            s->m_Unk0e4 = 1;
            s->m_Unk0e0 = 1;
            h->m_Done += h->m_Copied;
            if (h->m_Done == h->m_Size) {
                s->m_pBuffers[s->m_ReadBuffer]->m_Pending = 1;
                int next = s->m_ReadBuffer + 1;
                h->m_State = 0;
                ES_LimitStreamBufferIndex(&next, s, 0x309);
                s->m_ReadBuffer = next;
                if (s->m_HistoryIndex == -1) s->m_HistoryIndex = 0;
                s->m_HistoryIndex++;
                ES_LimitPendingBufferIndex(&s->m_HistoryIndex);
            }
            if (!s->m_Loaded) s->m_Loaded = 1;
        }
    }
}

static void ES_InitialiseStreamUpdate()
{
    if (s_WaitRead == s_WaitWrite) return;
    StreamIntialiseDetails *w = &StreamWaitingIntialise[s_WaitRead];
    if (w->m_State == 0xd) {
        ES_InitialiseGetNextStream();
        return;
    }
    if (s_InitState == 2) {
        int stream = w->m_Stream;
        s_InitSection = w->m_Section;
        int hash = w->m_Hash;
        s_InitStartSection = w->m_StartSection;
        StreamData *s = &Streams[stream];
        int keyOn = s->m_KeyOn;
        int unlocked = s->m_Unlocked;
        s_InitForceRestart = w->m_ForceRestart;
        int keyOnArg = s->m_KeyOnArg;
        int handle = s->m_psiHandle;
        XFileHandle file = s->m_File;
        StreamFade fade = s->m_Fade;
        int startedOn = s->m_StartedOn;
        memset(s, 0, sizeof(StreamData));
        s->m_Fade = fade;
        s->m_StartedOn = startedOn;
        s->m_File = file;
        s->m_KeyOn = keyOn;
        s->m_KeyOnArg = keyOnArg;
        s->m_Unlocked = unlocked;
        s->m_Index = stream;
        s_pInitStream = s;
        s_InitHash = hash;
        s->m_psiHandle = handle;
        s->m_State = 1;
        s->m_Hash = hash;
        s->m_Section = s_InitSection;
        s->m_HistoryIndex = -1;
        s->m_ReadSize = 0x2000;
        if (hash < 0) {
            s_InitHash = -1 - hash;
            s->m_Channels = 1;
            s->m_Frequency = 0x5622;
            s->m_Music = 0;
            s->m_BufferSize = 0x10000;
            s->m_HalfSize = 0x8000;
        } else {
            s->m_Channels = 2;
            s->m_Frequency = 0xac44;
            s->m_Music = 1;
            s->m_BufferSize = 0x20000;
            s->m_HalfSize = 0x10000;
            s->m_Unk378 = 1;
            s->m_Unlocked = 1;
        }
        s_InitState = 0xe;
        s->m_State = 0xe;
    }
    if (s_InitState == 0xe) {
        if (s_pInitStream->m_KeyOff) return;
        if (ES_RequestStreamBuffers(s_pInitStream)) {
            s_InitState = 0xf;
            s_pInitStream->m_State = 0xf;
            s_pInitStream->m_NumReadBuffers = s_pInitStream->m_NumBuffers;
        }
    }
    if (s_InitState == 0xf) {
        if (!s_pInitStream->m_Music) {
            s_pInitStream->m_HeaderFile = gpEXAudio->psiAsyncOpenFile(&gpEXAudio->m_StreamFile, FILEMAN_AUDIO, 0xffffffff);
            if (s_pInitStream->m_HeaderFile != -1) {
                gpEXAudio->psiAsyncReadFile(s_pInitStream->m_HeaderFile,
                                            StreamLookupFileData[s_InitHash].m_Offset + StreamOffsetToAudioData, 0xb8,
                                            s_pHeaderBuf, 0x2000000);
                s_InitState = 4;
                s_pInitStream->m_State = 4;
            }
        } else {
            s_pInitStream->m_HeaderFile = gpEXAudio->psiAsyncOpenFile(&s_pInitStream->m_File, FILEMAN_AUDIO, 0xffffffff);
            if (s_pInitStream->m_HeaderFile != -1) {
                gpEXAudio->psiAsyncReadFile(s_pInitStream->m_HeaderFile, 0, 0x38, s_pHeaderBuf, 0x2000000);
                s_InitState = 3;
                s_pInitStream->m_State = 3;
            }
        }
        if (s_pInitStream->m_HeaderFile == -1) {
            gpEXAudio->SFXprintf("### ES_InitialiseStreamUpdate() open header FAILED with psiAsyncOpenFile().\n");
            ES_InitialiseGetNextStream();
            return;
        }
    }
    if (s_InitState == 3) {
        if (s_pInitStream->m_KeyOff) return;
        if (gpEXAudio->psiAsyncHasReadFinished(s_pInitStream->m_HeaderFile)) {
            gpEXAudio->psiAsyncCloseFile(s_pInitStream->m_HeaderFile);
            int *pHeader = (int *)s_pHeaderBuf;
            s_pInitStream->m_pHeader = gpEXAudio->psiAsyncCreateBuffer(pHeader[9], 0);
            if (!s_pInitStream->m_pHeader) {
                gpEXAudio->SFXprintf("### ES_InitialiseStreamUpdate() FAILED with psiSFXLoadFile( %s )\n", s_HeaderName);
                ES_InitialiseGetNextStream();
                return;
            }
            s_pInitStream->m_HeaderFile = gpEXAudio->psiAsyncOpenFile(&s_pInitStream->m_File, FILEMAN_AUDIO, 0xffffffff);
            gpEXAudio->psiAsyncReadFile(s_pInitStream->m_HeaderFile, pHeader[8], pHeader[9], s_pInitStream->m_pHeader,
                                        0x2000000);
            s_InitState = 4;
            s_pInitStream->m_State = 4;
        }
    }
    if (s_InitState == 4) {
        if (s_pInitStream->m_KeyOff) return;
        if (gpEXAudio->psiAsyncHasReadFinished(s_pInitStream->m_HeaderFile)) {
            if (s_pInitStream->m_Music) {
                int *pHeader = (int *)s_pHeaderBuf;
                gpEXAudio->psiAsyncCloseFile(s_pInitStream->m_HeaderFile);
                s_pInitStream->m_DataOffset = pHeader[10];
                s_pInitStream->m_FileEnd = pHeader[11];
            } else {
                int *pHeader = (int *)s_pHeaderBuf;
                s_pInitStream->m_DataOffset = pHeader[1] + StreamOffsetToAudioData;
                s_pInitStream->m_pHeader = s_MonoHeaders[s_pInitStream->m_Index - 2];
                memcpy(s_pInitStream->m_pHeader, pHeader + 3, 0x2b * 4);
                s_pMonoHeader = (char *)pHeader;
                gpEXAudio->psiAsyncCloseFile(s_pInitStream->m_HeaderFile);
            }
            s_InitState = 5;
            s_pInitStream->m_State = 5;
        }
    }
    if (s_InitState == 5) {
        StreamData *s = s_pInitStream;
        int *pHeader = (int *)s->m_pHeader;
        s->m_pMarkers = (char *)pHeader + pHeader[3];
        s->m_pSections = (char *)pHeader + pHeader[2];
        s->m_pMarker = s->m_pMarkers;
        s->m_pSection = s->m_pSections;
        if (s->m_Music) {
            gpEXAudio->m_StreamJump = s_InitStartSection;
            gpEXAudio->m_StreamJumpDone = s_InitStartSection;
            s->m_pSection = s->m_pSections + s_InitStartSection * sizeof(StreamSection);
            s->m_pMarker = s->m_pMarkers + ((StreamSection *)s->m_pSection)->m_Marker * sizeof(StreamMarker);
            s->m_ReadPos = ES_StreamAlign(((StreamSection *)s->m_pSection)->m_Pos);
            if (!s_InitForceRestart) ES_MusicFindMyStoredFilePos(s);
            ES_MusicStoreMyFilePosCreateNew(s);
            ES_MusicStoreMyFilePos(s);
            s->m_PlayFilePos = s->m_ReadPos;
            gpEXAudio->m_Unk1e4 = 1;
            s->m_Unk37c = 0;
        } else {
            s->m_ReadPos = 0;
        }
        s->m_Volume = ((int *)s->m_pHeader)[4];
        s->m_ShutdownVolume = 100;
        ES_MusicSetVolume();
        s_pInitStream->m_State = 2;
        s_pInitStream->m_Active = 1;
        ES_InitialiseGetNextStream();
    }
}

void ES_StreamUpdate()
{
    gpEXAudio->psiPreSFXUpdate();
    ES_InitialStreamBuffer();
    if (gpEXAudio->m_Unk1e8 || gpEXAudio->m_MusicFader.m_Volume || gpEXAudio->m_SFXFader.m_Volume)
        ES_InitialiseStreamUpdate();
    ES_MusicSetVolume();
    s_StatFrame++;
    if (gpEXAudio->m_Debug[1]) {
        if (s_StatFrame % 120 == 0) {
            int mus = s_MusicUse * 100 / s_StatFrames;
            int sfx = s_SFXUse * 100 / s_StatFrames;
            gpEXAudio->SFXprintf("-> Stream Stats <- Average Use MUS %d.%2.2d  SFX %d.%2.2d\n", mus / 100, mus % 100,
                                 sfx / 100, sfx % 100);
        }
    } else {
        s_MusicUse = 1;
        s_SFXUse = 1;
        s_StatFrames = 1;
        for (int j = 0; j < 7; j++) s_StreamUse[j] = 1;
    }
    s_StatFrames++;
    s_UpdateCount++;
    gpEXAudio->m_Unk1e4 = 0;
    for (int i = 0; i < 7; i++) {
        StreamData *s = &Streams[i];
        if (gpEXAudio->m_Debug[1] && s_StatFrame % 120 == 0) {
            gpEXAudio->SFXprintf("%d) ", s->m_Index);
            if (!s->m_State) {
                gpEXAudio->SFXprintf("FREE ");
            } else {
                gpEXAudio->SFXprintf("HC %6.6X ", s->m_Section);
                if (s->m_Paused) gpEXAudio->SFXprintf("Pau ");
                else gpEXAudio->SFXprintf("Run ");
                gpEXAudio->SFXprintf("Pos %8.8d ", s->m_ReadPos);
            }
            gpEXAudio->SFXprintf("Used %3.3d pct\n", s_StreamUse[s->m_Index] * 100 / s_StatFrames);
        }
        if (ES_IsStreamInitialised(s)) {
            int readBuf = s->m_ReadBuffer;
            int cur = s->m_CurBuffer;
            s_StreamUse[s->m_Index]++;
            if (s->m_Music) s_MusicUse++;
            else s_SFXUse++;
            if (!s->m_pMarker) ES_StreamKeyOff(s->m_Index);
            if (s->m_KeyOff) {
                if (ES_StreamCloseBufferAccess(s)) {
                    if (s->m_Channels == 2) gpEXAudio->psiAsyncDeleteBuffer(s->m_pHeader);
                    s->m_State = 0xc;
                    s->m_KeyOff = 0;
                    goto unlock;
                }
                goto play;
            }
            ES_RequestStreamBuffers(s);
            ES_MusicStoreMyFilePos(s);
            if (s->m_Music) {
                gpEXAudio->m_Unk1e4 = 1;
                if (!s->m_Paused) psiSetStreamFrequency(s->m_psiHandle, s->m_Frequency);
                if (s->m_Loaded && (!s->m_Playing || s->m_Fade.m_PauseAfter)) {
                    if (s->m_Fade.m_PauseWait) {
                        StreamData *o = &Streams[1 - s->m_Index];
                        if (s->m_StartedOn > o->m_StartedOn) {
                            o->m_Fade.m_State = -1;
                            o->m_Fade.m_Speed = s->m_Fade.m_Time;
                        }
                        s->m_Fade.m_PauseWait = 0;
                        s->m_PauseTime = o->m_Fade.m_Volume / o->m_Fade.m_Speed + 1;
                    } else if (s->m_Fade.m_PauseAfter && !s->m_PauseTime) {
                        s->m_Fade.m_PauseAfter = 0;
                        s->m_Fade.m_Paused = 0;
                        ES_StreamUnPause(s);
                    }
                }
                if (s->m_Fade.m_State && s->m_Playing && !s->m_Fade.m_Paused) {
                    if (s->m_Fade.m_State == 1) {
                        if (s->m_Fade.m_FadeUpWait) {
                            StreamData *o = &Streams[1 - s->m_Index];
                            if (s->m_StartedOn > o->m_StartedOn) {
                                o->m_Fade.m_State = -1;
                                o->m_Fade.m_Speed = s->m_Fade.m_Time;
                            }
                            s->m_Fade.m_FadeUpWait = 0;
                        }
                        s->m_Fade.m_Volume += gpEXAudio->m_Ticks * s->m_Fade.m_Speed;
                        if (s->m_Fade.m_Volume > 10000000) {
                            s->m_Fade.m_Volume = 10000000;
                            s->m_Fade.m_State = 0;
                        }
                    } else {
                        s->m_Fade.m_Volume -= gpEXAudio->m_Ticks * s->m_Fade.m_Speed;
                        if (s->m_Fade.m_Volume < 0) {
                            s->m_Fade.m_Volume = 0;
                            s->m_Fade.m_State = 0;
                            ES_StreamKeyOff(s->m_Index);
                        }
                    }
                }
            }
            if (s->m_PauseTime) {
                s->m_PauseTime -= gpEXAudio->m_Ticks;
                if (s->m_PauseTime <= 0) {
                    if (s->m_Fade.m_Paused) {
                        s->m_Fade.m_Paused = 0;
                        psiStreamUnPause(s->m_psiHandle);
                        s->m_Paused = 0;
                    } else {
                        s->m_KeyOn = 1;
                    }
                    s->m_PauseTime = 0;
                }
            }
            if (s->m_Playing && !s->m_Paused) {
                s->m_LastPlayPos = s->m_PlayPos;
                s->m_LastpsiPos = s->m_psiPos;
                s->m_psiPos = psiStreamGetPlayPos(s->m_psiHandle);
                if (s->m_psiPos < s->m_LastpsiPos) {
                    s->m_PlayFilePos += s->m_HalfSize;
                    if (s->m_LoopEnd && s->m_PlayFilePos >= s->m_LoopEnd)
                        s->m_PlayFilePos = s->m_LoopStart - s->m_LoopEnd + s->m_PlayFilePos;
                    s->m_PlayHalf = (s->m_PlayHalf + 1) % 2;
                }
                s->m_PlayPos = s->m_PlayHalf * s->m_HalfSize + s->m_psiPos;
                int n = cur;
                if (cur < readBuf) n = s->m_NumBuffers + cur;
                s->m_Unk33c = s->m_Unk340;
                s->m_Unk340 = n - readBuf;
            }
            if (s->m_JumpMarker.m_Pending && s->m_JumpArmed && ES_CheckEndMarker(s, &s->m_JumpMarker)) {
                s->m_Unk370 = 0;
                s->m_JumpArmed = 0;
            }
            if (s->m_LoopMarker.m_Pending && s->m_LoopArmed && ES_CheckEndMarker(s, &s->m_LoopMarker)) {
                ES_JumpRequestComplete(s);
                s->m_LoopArmed = 0;
            }
            if (s->m_LoopArmed && s->m_JumpCount-- == 1) {
                ES_JumpRequestComplete(s);
                s->m_LoopArmed = 0;
            }
            if (s->m_EndMarker.m_Pending && s->m_EndArmed && ES_CheckEndMarker(s, &s->m_EndMarker)) {
                s->m_Ended = 1;
                ES_StreamKeyOff(s->m_Index);
                goto play;
            }
            {
                int wait = 1;
                int prev = cur - 1;
                ES_LimitStreamBufferIndex(&prev, s, 0x90c);
                if (cur == readBuf || prev == readBuf) wait = 0;
                for (int j = 0; j < s->m_NumBuffers; j++) {
                    if (s->m_pBuffers[j]->m_Reading == 1 && !s->m_pBuffers[j]->m_Stopped &&
                        gpEXAudio->psiAsyncHasReadFinished(s->m_pBuffers[j]->m_File))
                        s->m_pBuffers[j]->m_Reading = 0;
                }
                while (!s->m_Started ||
                       ((cur != readBuf || s->m_pBuffers[cur]->m_Pending) &&
                        gpEXAudio->psiAsyncHasReadFinished(s->m_pBuffers[cur]->m_File) && !s->m_pBuffers[cur]->m_Stopped &&
                        !wait)) {
                    s->m_NumReadBuffers = s->m_NumBuffers;
                    ES_UpdateMusicMarkers(s, 0);
                    StreamBufferData *b = s->m_pBuffers[cur];
                    if (b->m_Loading) break;
                    if (b->m_Size) {
                        if (b->m_SetToNull) {
                            psiStreamSetToNull(b->m_Size, b->m_pData);
                        } else if (!ES_StreamAsyncReadFile(s->m_pBuffers[cur]->m_File, s->m_ReadPos,
                                                           s->m_pBuffers[cur]->m_Size, s->m_pBuffers[cur]->m_pData, s)) {
                            s->m_pBuffers[cur]->m_FilePos = s->m_ReadPos;
                        }
                        s->m_pBuffers[cur]->m_Reading = 1;
                        s->m_pBuffers[cur]->m_Read = 1;
                    }
                    ES_HandleStreamBufferLoaded(s);
                    cur = s->m_CurBuffer;
                    s->m_Started = 1;
                }
            }
            if (StreamMemCpyHandles[s->m_Index].m_State != 9 && s->m_State != 7 && s->m_State != 0xb &&
                (!s->m_Loaded || s->m_Playing)) {
                int h = s->m_HistoryIndex;
                if (h != -1) {
                    int p = h - 1;
                    ES_LimitPendingBufferIndex(&p);
                    if (!s->m_EndMarker.m_Pending && s->m_History[p].m_EndMarker.m_Pending) {
                        s->m_EndArmed = 1;
                        s->m_EndMarker = s->m_History[p].m_EndMarker;
                    }
                    if (!s->m_JumpMarker.m_Pending && s->m_History[p].m_JumpMarker.m_Pending) {
                        s->m_JumpArmed = 1;
                        s->m_JumpMarker = s->m_History[p].m_JumpMarker;
                    }
                    if (!s->m_LoopMarker.m_Pending && s->m_History[p].m_LoopMarker.m_Pending) {
                        s->m_JumpCount = 0xf0;
                        s->m_LoopArmed = 1;
                        s->m_LoopMarker = s->m_History[p].m_LoopMarker;
                    }
                }
                StreamBufferData *b = s->m_pBuffers[readBuf];
                if (!b->m_Reading) {
                    if (h == -1) s->m_History[0] = *b;
                    else s->m_History[h] = *b;
                    ES_StartStreamMemCpy(s->m_psiHandle, s->m_pBuffers[readBuf]->m_DestOffset,
                                         s->m_pBuffers[readBuf]->m_pData, s->m_pBuffers[readBuf]->m_Size, s);
                }
            }
        play:
            if (s->m_Loaded && !s->m_Playing && s->m_KeyOn) {
                int quiet = 0;
                if (!gpEXAudio->m_MusicFader.m_Volume && !gpEXAudio->m_SFXFader.m_Volume) quiet = 1;
                if (gpEXAudio->m_Unk1e8 || !quiet) {
                    psiStreamPlay(s->m_psiHandle, s->m_KeyOnArg);
                    s->m_Playing = 1;
                    s->m_KeyOn = 2;
                }
            }
        } else if (s->m_KeyOff) {
            if (ES_StreamCloseBufferAccess(s)) {
                if (s->m_State) {
                    if (s->m_State > 2 && s->m_State <= 4) {
                        if (!gpEXAudio->psiAsyncHasReadFinished(s->m_HeaderFile)) {
                            gpEXAudio->psiAsyncStopAccess(s->m_HeaderFile);
                            goto keyoffwait;
                        }
                        gpEXAudio->psiAsyncCloseFile(s->m_HeaderFile);
                        if (s->m_Channels == 2 && s->m_pHeader) gpEXAudio->psiAsyncDeleteBuffer(s->m_pHeader);
                        ES_InitialiseGetNextStream();
                    }
                    s->m_State = 0xc;
                }
                if (s->m_KeyOffTime >= 500)
                    gpEXAudio->SFXprintf("Stream KeyOff Finally Finished! Index=%d  FC=%d\n", s->m_Index,
                                         s->m_KeyOffTime);
                s->m_KeyOff = 0;
            }
        keyoffwait:
            if (s->m_KeyOff && s->m_KeyOffTime++ == 500)
                gpEXAudio->SFXprintf("Stream KeyOff Time Excessive! Index=%d\n", s->m_Index);
        }
    unlock:
        if (s->m_Unlocked && s->m_State == 0xc) {
            s->m_State = 0;
            s->m_Active = 0;
        }
    }
    ES_UpdateStreamMemCpy();
    gpEXAudio->m_Unk1e8 = 0;
    gpEXAudio->psiPostSFXUpdate();
}

int ES_InitialiseStream(int hash, int unused, XFileHandle *pFile, int section)
{
    int first = 0;
    int end = 7;
    if (!StreamsInitialised) {
        if (gpEXAudio->m_Debug[2]) gpEXAudio->SFXprintf(">>> !StreamsInitialised\n");
        return -1;
    }
    if (hash < 0) {
        if (StreamLookupFileData[-hash - 1].m_Offset == -1) {
            gpEXAudio->SFXprintf("Stream not here\n");
            return -1;
        }
        first = 2;
    } else {
        end = 2;
    }
    for (int i = first; i < end; i++) {
        if (!Streams[i].m_State) {
            StreamWaitingIntialise[s_WaitWrite].m_Hash = hash;
            StreamWaitingIntialise[s_WaitWrite].m_Stream = i;
            StreamWaitingIntialise[s_WaitWrite].m_State = 0;
            StreamWaitingIntialise[s_WaitWrite].m_Section = section;
            StreamWaitingIntialise[s_WaitWrite].m_StartSection = MusicStartSection;
            StreamWaitingIntialise[s_WaitWrite].m_ForceRestart = MusicForceRestart;
            Streams[i].m_StartedOn = StartedOnFrameCounter++;
            Streams[i].m_Unlocked = 0;
            Streams[i].m_Active = 0;
            Streams[i].m_KeyOn = 0;
            Streams[i].m_Paused = 0;
            Streams[i].m_Unk064 = 0;
            Streams[i].m_State = 1;
            if (!pFile) pFile = &gpEXAudio->m_StreamFile;
            Streams[i].m_File = *pFile;
            Streams[i].m_WaitIndex = s_WaitWrite;
            memset(&Streams[i].m_Fade, 0, sizeof(StreamFade));
            s_WaitWrite = (s_WaitWrite + 1) & 0x3f;
            ES_InitialiseStreamUpdate();
            return i;
        }
    }
    return -1;
}
