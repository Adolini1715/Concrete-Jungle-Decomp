#ifndef EXAUDIOSTREAM_H
#define EXAUDIOSTREAM_H
#include "types.h"
#include "AsyncFile.h"

// EuroSound streaming (EXAudioStream.obj, C-style). Streams 0 and 1 are the stereo music pair
// (crossfades), 2..6 mono SFX streams. Each stream reads its file through StreamBufferData blocks
// (four for music, two for SFX) and copies them into the psi playback buffer half by half.

// End/jump marker armed when its buffer starts playing (sizeof 0xc).
struct TestMarker {
    int m_Pending;      // 0x0
    int m_Armed;        // 0x4 waiting for the play position to pass m_Pos
    int m_Pos;          // 0x8
};

// One file read block (sizeof 0x68).
struct StreamBufferData {
    char *m_pData;          // 0x00 (kept across clears)
    int m_FilePos;          // 0x04
    int m_ReadPos;          // 0x08
    int m_DestOffset;       // 0x0c playback buffer offset
    int m_NextOffset;       // 0x10
    int m_Size;             // 0x14
    int m_SetToNull;        // 0x18 end of data: fill with silence
    int m_Loading;          // 0x1c
    int m_Pending;          // 0x20
    TestMarker m_EndMarker;     // 0x24
    TestMarker m_JumpMarker;    // 0x30
    TestMarker m_LoopMarker;    // 0x3c
    int m_HasMarker;        // 0x48
    int m_MarkerOffset;     // 0x4c
    int m_File;             // 0x50 psiAsyncOpenFile handle
    int m_Read;             // 0x54 read issued
    int m_Reading;          // 0x58
    int m_Stopped;          // 0x5c psiAsyncStopAccess called
    void *m_pMarker;        // 0x60
    void *m_pSection;       // 0x64
};

// Stream volume fade (sizeof 0x18).
struct StreamFade {
    int m_State;            // 0x00 1 up, -1 down
    int m_Volume;           // 0x04 0..10000000
    int m_Speed;            // 0x08
    s8 m_Unk0c;             // 0x0c
    s8 m_FadeUpWait;        // 0x0d
    s8 m_PauseWait;         // 0x0e
    s8 m_Unk0f;             // 0x0f
    int m_Time;             // 0x10
    s8 m_PauseAfter;        // 0x14
    s8 m_Paused;            // 0x15
    s8 m_Unk16[2];
};

// Stream state (sizeof 0x524).
struct StreamData {
    int m_LoopEnd;          // 0x000
    int m_LoopStart;        // 0x004
    int m_PlayFilePos;      // 0x008
    int m_FileEnd;          // 0x00c
    int m_StartedOn;        // 0x010 StartedOnFrameCounter at start
    StreamFade m_Fade;      // 0x014
    int m_Ended;            // 0x02c
    int m_PauseTime;        // 0x030
    int m_Index;            // 0x034
    int m_Hash;             // 0x038 (music) or stream file index
    int m_Section;          // 0x03c
    int m_State;            // 0x040 0 free, 1 waiting, 2 running, 3..5,0xe,0xf initialising, 0xc stopped
    int m_HeaderFile;       // 0x044
    int m_psiHandle;        // 0x048
    int m_Unk04c;           // 0x04c
    int m_Unk050;           // 0x050
    int m_Channels;         // 0x054 1 mono, 2 stereo
    int m_Unk058;           // 0x058
    int m_Unk05c;           // 0x05c
    int m_Paused;           // 0x060
    int m_Unk064;           // 0x064
    int m_Volume;           // 0x068
    int m_Unk06c;           // 0x06c
    int m_Music;            // 0x070
    int m_Frequency;        // 0x074
    int m_Active;           // 0x078
    int m_DataOffset;       // 0x07c
    int m_Playing;          // 0x080
    int m_Started;          // 0x084
    int m_Loaded;           // 0x088
    int m_KeyOn;            // 0x08c
    int m_KeyOff;           // 0x090
    int m_KeyOffTime;       // 0x094
    int m_Unlocked;         // 0x098
    int m_KeyOnArg;         // 0x09c
    int m_Unk0a0;           // 0x0a0
    int m_WaitIndex;        // 0x0a4
    int m_JumpCount;        // 0x0a8
    int m_EndArmed;         // 0x0ac
    int m_JumpArmed;        // 0x0b0
    int m_LoopArmed;        // 0x0b4
    TestMarker m_EndMarker;     // 0x0b8
    TestMarker m_JumpMarker;    // 0x0c4
    TestMarker m_LoopMarker;    // 0x0d0
    int m_LastBuffer;       // 0x0dc
    int m_Unk0e0;           // 0x0e0
    int m_Unk0e4;           // 0x0e4
    int m_CopiedPos;        // 0x0e8
    int m_ReadPos;          // 0x0ec
    int m_Unk0f0[2];        // 0x0f0
    int m_BufferSize;       // 0x0f8 playback buffer bytes
    int m_HalfSize;         // 0x0fc
    int m_ReadSize;         // 0x100
    int m_PlayPos;          // 0x104
    int m_LastPlayPos;      // 0x108
    int m_PlayHalf;         // 0x10c
    int m_psiPos;           // 0x110
    int m_LastpsiPos;       // 0x114
    StreamBufferData *m_pBuffers[4];    // 0x118
    StreamBufferData m_History[5];      // 0x128
    int m_HistoryIndex;     // 0x330
    int m_NumBuffers;       // 0x334
    int m_NumReadBuffers;   // 0x338
    int m_Unk33c;           // 0x33c
    int m_Unk340;           // 0x340
    int m_CurBuffer;        // 0x344
    int m_ReadBuffer;       // 0x348
    int m_Unk34c;           // 0x34c
    int m_ShutdownVolume;   // 0x350
    int m_Unk354;           // 0x354
    char *m_pHeader;        // 0x358
    char *m_pMarker;        // 0x35c
    int m_Unk360;           // 0x360
    char *m_pSection;       // 0x364
    char *m_pMarkers;       // 0x368
    char *m_pSections;      // 0x36c
    int m_Unk370;           // 0x370
    int m_Unk374;           // 0x374
    int m_Unk378;           // 0x378
    int m_Unk37c;           // 0x37c
    int m_ReadCount;        // 0x380
    XFileHandle m_File;     // 0x384
};

// Queued ES_InitialiseStream request (sizeof 0x24).
struct StreamIntialiseDetails {
    int m_Hash;             // 0x00
    int m_Stream;           // 0x04
    int m_Unk08;            // 0x08
    int m_State;            // 0x0c 0xd: cancelled
    int m_Unk10[2];         // 0x10
    int m_Section;          // 0x18
    int m_StartSection;     // 0x1c MusicStartSection
    int m_ForceRestart;     // 0x20 MusicForceRestart
};

// Pending copy of a read block into the playback buffer (sizeof 0x28).
struct StreamMemCpyHandle {
    int m_psiHandle;        // 0x00
    int m_DestOffset;       // 0x04
    char *m_pSrc;           // 0x08
    int m_Size;             // 0x0c
    int m_Done;             // 0x10
    int m_Unk14;            // 0x14
    int m_State;            // 0x18 9: active
    StreamData *m_pStream;  // 0x1c
    int m_Busy;             // 0x20
    int m_Copied;           // 0x24
};

// Remembered play position of a music track (sizeof 0x10).
struct MusicFilePosStore {
    int m_Hash;             // 0x0
    int m_Pos;              // 0x4
    int m_MaxPos;           // 0x8
    unsigned int m_Age;     // 0xc
};

// Pending music crossfade (sizeof 0x1c0).
struct _STARTMUSICDATA {
    int m_Start;            // 0x000
    int m_Hash;             // 0x004 -1: none
    int m_Section;          // 0x008
    int m_FadeIn;           // 0x00c
    int m_FadeOut;          // 0x010
    int m_FadeAndStop;      // 0x014
    XFileHandle m_File;     // 0x018
    int m_ForceRestart;     // 0x1b8
    int m_Unk1bc;           // 0x1bc
};

struct StreamLookupFileDetails {
    int m_Offset;
};

extern StreamBufferData StreamBuffers[24];
extern _STARTMUSICDATA StartMusicData;
extern MusicFilePosStore MusicFilePosStoreData[20];
extern StreamIntialiseDetails StreamWaitingIntialise[64];
extern StreamMemCpyHandle StreamMemCpyHandles[7];
extern StreamData Streams[7];
extern int StartedOnFrameCounter;
extern int StreamOffsetToAudioData;
extern int StreamsInitialised;
extern StreamLookupFileDetails *StreamLookupFileData;
extern int MusicStartSection;
extern int MusicFilePosStoreIndex;
extern int MusicForceRestart;

void ES_MusicStoreMyFilePosCreateNew(StreamData *s);
void ES_MusicFindMyStoredFilePos(StreamData *s);
int ES_FindpsiStreamHandle(int i);
int ES_FindStreamShutdownVolume(int i);
int ES_HasStreamEnded(int i);
void ES_UnlockStream(int i);
StreamData *ES_GetStreamPointer(int i);
void ES_StreamPause(StreamData *s);
void ES_StreamUnPause(StreamData *s);
void ES_StreamKeyOn(int i, int arg);
int ES_MonoStreamActiveCount();
int ES_StreamActiveCount();
void ES_InitialiseAudioStreamSystem();
void SFXStartMusicXFade(int hash, int section, int fadeIn, int fadeOut, int fadeAndStop, XFileHandle *pFile,
                        int forceRestart);
void ES_StreamKeyOff(int i);
void ES_MusicSetVolume();
void ES_MusicStoreMyFilePos(StreamData *s);
void ES_StopMusic();
void ES_MusicPause();
void ES_MusicUnPause();
void ES_StopAllStreams();
int ES_IsStreamPlaying(int i);
void ES_UpdateStreamMemCpy();
void ES_StreamUpdate();
int ES_InitialiseStream(int hash, int stream, XFileHandle *pFile, int section);
#endif
