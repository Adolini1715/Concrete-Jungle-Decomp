#ifndef EXAUDIOBASE_H
#define EXAUDIOBASE_H
#include "types.h"
#include "AsyncFile.h"
#include "SFXPoint3.h"
#include "SFXItem.h"
#include "EXAudio_Fader.h"
#include "EXAudio_SoundBank.h"
#include "EXAudio_VoiceManager.h"

// Status block shared with the game thread; overlays EXAudioBase from 0x52c (m_FileStatus) and is
// what EXBaseSoundManager::m_pAudioStatus points to. (SFXSetDebug's parameter has the same mangled
// type but is really the CMD_DEBUG command block: TODO reconcile.)
struct SFX_Debug_Data {
    u32 m_FileStatus[17];    // 0x000 per game-file slot: id & 0xffffff00 | 0x80 finished | status
    int m_NumPlaying;        // 0x044
    struct Playing {
        int m_Hash;
        const void *m_pObj;
    } m_Playing[60];         // 0x048 TODO: count unverified
    u8 m_Unk228[0x10];       // 0x228 TODO layout
    int m_DriveActivity;     // 0x238 file type being read (-1 none)
    u32 m_BankHash[10];      // 0x23c loaded sound banks
    u32 m_Unk264[2];         // 0x264 TODO
    int m_BanksActive;       // 0x26c 0: banks not tracked yet
    int m_FileMaxSeek;       // 0x270 (EXAudioBase 0x79c)
    int m_FilePercent[3];    // 0x274
};
struct SampleHeaderData;
// One sample choice of an SFX (the SFXParameters::Sample record, 8 bytes): sample index (<0: a stream)
// and the pitch / volume / pan with their random ranges.
struct SFXSamplePoolFiles {
    s16 m_Sample;       // 0x0
    s8 m_Pitch;         // 0x2 half steps of the pitch table
    u8 m_PitchRand;     // 0x3
    s8 m_Volume;        // 0x4
    s8 m_VolumeRand;    // 0x5
    s8 m_Pan;           // 0x6
    s8 m_PanRand;       // 0x7
};
struct PjtMemorySlots;
struct SFXPoint3;
// Per-sound parameters handed to the platform layer (psiInitialiseSound / psiUpdateSound).
// TODO layout: fields not read by EXAudioVoice.obj.
struct psiSFX {
    int m_Voice;            // 0x00 Voice * handle
    int m_MinRadius;        // 0x04
    int m_MaxRadius;        // 0x08
    int m_Volume;           // 0x0c 0..100
    int m_Freq;             // 0x10
    int m_Streamed;         // 0x14 m_Voice is a stream
    int m_Pan;              // 0x18 -100..100
    SFXPoint3 *m_pPos;      // 0x1c
    int m_20;
    SFXPoint3 *m_pVel;      // 0x24
    int m_Mode;             // 0x28 bit 0: 3D
    int m_Filter[3];        // 0x2c game effect filter values (effect mode 3)
};
enum SFXGAMEEFFECT { SFXGAMEEFFECT_0 };
enum SFXREVERBTYPES { SFXREVERBTYPES_0 };
enum FILEMAN_TYPE { FILEMAN_SOUNDBANK = 0, FILEMAN_AUDIO = 1, FILEMAN_GAME = 2 };

// Per-SFX parameters from the project file. TODO layout beyond the fields used.
struct SFXParameters {
    struct Sample {
        s16 m_Sample;   // 0x0 <0: none
        s16 m_Unk2[3];
    };
    s16 m_DuckTime;     // 0x00 ducker duration / 10
    s16 m_CueMin;       // 0x02
    s16 m_CueMax;       // 0x04
    s8 m_KeyOnArg;      // 0x06 passed to ES_AudioKeyOn
    u8 m_Type;          // 0x07 3/5/7: positioned relative to the listener
    s8 m_MaxInstances;  // 0x08 0: unlimited instances of this hash
    s8 m_Importance;    // 0x09
    s8 m_DuckVolume;    // 0x0a 0: no ducking
    s8 m_Volume;        // 0x0b percent
    s16 m_Group;        // 0x0c culling group
    u8 m_GroupMax;      // 0x0e 0: no group limit
    u8 m_Unk0f;
    u8 m_RejectWhenFull;  // 0x10 refuse to start rather than steal
    u8 m_NoFade;        // 0x11 ignored by SFXFadeSFXDown
    u8 m_NoVolumeScale; // 0x12 skip the master/ducker volumes
    u8 m_MultiSample;   // 0x13 play the samples as a sequence (or all at once with m_PlayAll)
    u8 m_Unk14;
    u8 m_Shuffle;       // 0x15 sequence in random order
    u8 m_Unk16;         // 0x16 never distance culled
    u8 m_PlayAll;       // 0x17
    u8 m_NoUnderWater;  // 0x18
    u8 m_PauseOnFade;   // 0x19
    u8 m_Unk1a;         // 0x1a not counted as an active voice
    u8 m_StealQuietest; // 0x1b steal the quietest instance (and take over its position)
    u8 m_Duck;          // 0x1c
    u8 m_GroupPriority; // 0x1d removes other group members rather than counting them
    u8 m_GroupReject;   // 0x1e reject when only the group is full
    u8 m_OncePerFrame;  // 0x1f
    u8 m_GameEffect;    // 0x20 affected by SFXGameEffectMode
    u8 m_Unk21[0xf];
    u8 m_Doppler;       // 0x30 percent
    u8 m_Unk31;
    s16 m_NumSamples;   // 0x32
    Sample m_Samples[1];  // 0x34
};

// Memory slot of a sound bank. TODO layout.
struct PjtMemorySlots {
    int m_Unk0;
    int m_Unk4;
    int m_Type;         // 0x8 1: resident
};

// Sound bank load job (sizeof 0x1a4).
struct ES_LoadSoundBankDetails {
    int m_Hash;              // 0x000
    XFileHandle m_File;      // 0x004
};

// Platform-independent EuroSound core (EXAudioBase.obj). sizeof 0x11860 (from OpenSoundManager).
class EXAudioBase {
public:
    virtual ~EXAudioBase();
    virtual void Init();
    virtual int psiInitialiseAudioMemory(int) = 0;
    virtual int psiInitialiseSoundSystem() = 0;
    virtual int psiRequestVoiceHandle(SampleHeaderData *, int) = 0;
    virtual int psiIsSamplePlaying(int) = 0;
    virtual void psiSetSampleFrequency(int, int) = 0;
    virtual int psiSampleKeyOn(int, int) = 0;
    virtual int psiSampleKeyOff(int) = 0;
    virtual void psiSamplePause(int) = 0;
    virtual void psiSampleUnPause(int) = 0;
    virtual void psiSetSampleVolume(int, int) = 0;
    virtual void psiInitialiseSoundRam() = 0;
    virtual void psiLoadSampleInitialise(SampleHeaderData *) = 0;
    virtual void psiUnLoadSampleInitialise(SampleHeaderData *) = 0;
    virtual void psiPreSFXUpdate() = 0;
    virtual void psiPostSFXUpdate() = 0;
    virtual void psiSFXSetupListener(SFXPoint3 *, SFXPoint3 *, SFXPoint3 *, SFXPoint3 *, SFXPoint3 *) = 0;
    virtual void psiUnLockVoiceHandle(int) = 0;
    virtual void psiLockVoiceHandle(int) = 0;
    virtual void psiInitialiseSound(psiSFX *) = 0;
    virtual void psiUpdateSound(psiSFX *) = 0;
    virtual void psiDebugTextOutput(char *, char *) = 0;
    virtual int psiAsyncOpenFile(XFileHandle *, FILEMAN_TYPE, unsigned int) = 0;
    virtual void psiAsyncCloseFile(int) = 0;
    virtual void psiAsyncReadFile(int, int, int, char *, unsigned int) = 0;
    virtual void psiAsyncLockFilePriority(unsigned int) = 0;
    virtual void psiAsyncUnLockFilePriority() = 0;
    virtual void psiAsyncStopAccess(int) = 0;
    virtual int psiAsyncHasReadFinished(int) = 0;
    virtual char *psiAsyncCreateBuffer(int, int) = 0;
    virtual void psiAsyncDeleteBuffer(char *) = 0;
    virtual int psiLoadSoundBank(XFileHandle *, unsigned int) = 0;
    virtual int psiHasLoadSoundBankFinished() = 0;
    virtual void psiBankMemoryDeAlloc(int) = 0;
    virtual unsigned int psiSFXGetTimer() = 0;
    virtual char *psiSFXLoadFile(char *) = 0;
    virtual void psiSFXFreeFileMem(char *) = 0;
    virtual void psiSFXSetMode(int) = 0;
    virtual void psiSetReverb(int) = 0;
    virtual void psiSFXPreMovieReset() = 0;
    virtual void psiSFXPostMovieReset() = 0;
    virtual void psiInitialiseReverb(int, int, int, int, int, int) = 0;
    virtual float psiSquareRoot(float) = 0;
    virtual void SFXprintf(const char *, ...) = 0;
    void SetDoppler(unsigned int d) { m_Doppler = d; }
    void SFXSetTestMode(int);
    unsigned int ES_Rnd(int);
    int ES_Rndf();
    int ES_Rndsf();
    void ES_DuckerStart(int, int, SFXItem *);
    void ES_DuckerStop();
    void SFXSetDebug(SFX_Debug_Data *);
    SFXItem *ES_FindSubSFX(void *);
    void SFXSetEnvironment(int, int, int);
    int ES_ConvertValueToMusicFade(int);
    void SFXStartCutscene();
    void SFXEndCutscene();
    void SFXFadeMusicDown(int, int);
    void SFXSetSFXVolume(int);
    int SFXGetSFXVolume();
    void SFXSetPosition(SFXPoint3 *, const void *, int);
    int SFXGetLanguage();
    void SFXSetLanguage(int);
    void SFXSetMode(int);
    int SFXGetMode();
    void SFXRemove(const void *, int);
    void SFXSetVolumeOverRide(int, const void *, int);
    void SFXSetPitchOverRide(int, const void *, int);
    void SFXLoadSoundBank(int, XFileHandle *);
    bool ES_LoadSoundBankHasJobs();
    void SFXPreMovieReset();
    void SFXPostMovieReset();
    void SFXInitialise2(XFileHandle *, XFileHandle *);
    void SFXInitialise2Update();
    int ES_SetCueTimer(SFXParameters *);
    void SFXGameEffectMode(SFXGAMEEFFECT);
    void SFXGameEffectModeValues(int, int, int, int, int);
    void SFXSetReverbType(SFXREVERBTYPES);
    void SFXSetupListener(SFXPoint3 *, SFXPoint3 *, SFXPoint3 *, SFXPoint3 *, SFXPoint3 *, int);
    void SFXSetNumberOfListeners(int);
    void SFXSetMusicVolume(int);
    int SFXGetMusicVolume();
    void SFXInitialise();
    EXAudioBase();
    void ES_UnLoadSoundBankRelease(int);
    void SFXFadeSFXDown(int, int);
    unsigned int SFXStart3D(int, SFXPoint3 *, void *, int, int, int, int);
    void SFXUnLoadSoundBankForced(int);
    void SFXFadeSFXUp(int);
    void SFXFadeMusicUp(int);
    int ES_AudioStart(SFXSamplePoolFiles *, int, SFXItem *, int);
    void SFXPreCache(int, int);
    void SFXUnLoadSoundBank(int);
    void SFXUpdate();
private:
    int ES_FindSoundBankSlot(int);
    int ES_GetSFXHashcode(int, int);
    void ES_CacheBankHashCodesReset();
    void ES_CacheBankHashCodes(int);
    void ES_UnLoadCacheBankHashCodes(int);
    void ES_SFXListInitialise();
    int ES_HasTimerPassed(unsigned int, int);
    void ES_PrintHashCode(int);
    void ES_ResetEnvironment();
    void ES_UpdateEnvironment();
    void ES_TryPlayMusic();
    void ES_UnLoadSoundBankReleaseFinished(int);
    ES_LoadSoundBankDetails *ES_LB_GetFree();
    ES_LoadSoundBankDetails *ES_LB_GetMe();
    void ES_LB_GetNext();
    char *ES_LB_StartFileLoad(XFileHandle *, int *, char *, int, int);
    void ES_LB_EndFileLoad(int);
    PjtMemorySlots *SoundBankGetMemorySlot(int);
    void ES_UnLoadSoundBankUpdate();
    SFXItem *ES_SFXNew();
    void ES_DuckerUpdate();
    void ES_FadeReset();
    void ES_FillpsiSFXData(psiSFX *, SFXItem *, int);
    void ES_LoadSoundBankUpdate();
    void ES_FadeUpdate();
    void ES_SfxUpdateAll();
public:
    u32 m_Unk004;                 // 0x004
    int m_Unk008;                 // 0x008 nonzero: SFXInitialise2 state 4
    int m_Unk00c;                 // 0x00c
    int m_Unk010;                 // 0x010
    int m_TestMode;               // 0x014
    XFileHandle m_StreamFile;     // 0x018 SFXInitialise2 second file
    int m_Cutscene;               // 0x1b8
    int m_CutsceneEnd;            // 0x1bc end requested while the music fader holds
    int m_Unk1c0;                 // 0x1c0
    int m_Ticks;                  // 0x1c4 timer ticks this update (fader step multiplier)
    int m_Unk1c8;                 // 0x1c8
    int m_Unk1cc;                 // 0x1cc
    int m_Unk1d0;                 // 0x1d0
    int m_SFXFadedDown;           // 0x1d4 set by SFXFadeSFXDown, cleared by SFXFadeSFXUp
    int m_Language;               // 0x1d8
    int m_Mode;                   // 0x1dc
    int m_Unk1e0;                 // 0x1e0
    int m_Unk1e4;                 // 0x1e4
    int m_Unk1e8;                 // 0x1e8
    unsigned int m_Doppler;       // 0x1ec
    int m_DuckerActive;           // 0x1f0
    int m_DuckerVolume;           // 0x1f4
    u32 m_DuckerStart;            // 0x1f8 psiSFXGetTimer at ES_DuckerStart
    int m_Unk1fc;                 // 0x1fc
    int m_Unk200;                 // 0x200
    SFXItem *m_pDuckerSFX;        // 0x204
    int m_DuckerFlag;             // 0x208
    int m_SFXVolume;              // 0x20c
    int m_MusicVolume;            // 0x210
    int m_SFXVolumeReq;           // 0x214
    int m_MusicVolumeReq;         // 0x218
    EXAudio_Fader m_SFXFader;     // 0x21c
    EXAudio_Fader m_MusicFader;   // 0x22c
    int m_Reverb;                 // 0x23c
    int m_Unk240;                 // 0x240
    int m_UnderWater;             // 0x244
    int m_Unk248;                 // 0x248
    int m_GameEffect;             // 0x24c
    int m_Unk250;                 // 0x250
    int m_Unk254;                 // 0x254
    int m_Unk258;                 // 0x258
    int m_Unk25c;                 // 0x25c
    int m_Unk260;                 // 0x260
    int m_Unk264;                 // 0x264
    int m_Unk268;                 // 0x268
    int m_Unk26c;                 // 0x26c
    int m_GameEffectValues[5];    // 0x270
    int m_Unk284;                 // 0x284
    int m_LBWrite;                // 0x288 load-bank job ring (10)
    int m_LBRead;                 // 0x28c
    int m_LBState;                // 0x290
    int m_LBBlocked;              // 0x294
    int m_BankSlot[105];          // 0x298 hash per sound bank slot (-1 free)
    int m_Unk43c[60];             // 0x43c
    u32 m_FileStatus[17];         // 0x52c per game-file slot: id & 0xffffff00 | 0x80 finished | status
    struct ActiveSFX {
        int m_Hash;
        const void *m_pObj;
    };
    int m_NumActive;              // 0x570 debug snapshot of the active SFX list (SFXUpdate)
    ActiveSFX m_Active[60];       // 0x574
    int m_StreamJump;             // 0x754 EXAudioStream: jump target section
    int m_Unk758;                 // 0x758
    int m_StreamJumpDone;         // 0x75c set to m_StreamJump when a jump marker is reached
    int m_Unk760;                 // 0x760
    int m_Unk764;                 // 0x764
    int m_LoadedBanks[10];        // 0x768 debug: hashes of the loaded sound banks
    int m_SFXFaderState;          // 0x790 debug: 0 idle, 1 fading, 2 faded out
    int m_MusicFaderState;        // 0x794
    int m_VersionOK;              // 0x798 project file version matches the code
    int m_FileMaxSeek;            // 0x79c loading stats from XBFileManager::UpdateStats
    int m_FilePercent[3];         // 0x7a0
    u8 m_Unk7ac[0x30];            // 0x7ac TODO layout
    u32 m_AudioMemFree;           // 0x7dc
    u32 m_AudioMemUsed;           // 0x7e0
    int m_Unk7e4;                 // 0x7e4
    int m_Busy;                   // 0x7e8 debug: m_Unk1e0 or m_Unk1e4 set
    int m_Unk7ec;                 // 0x7ec
    char *m_pStreamMem;           // 0x7f0 first mono stream buffer (ES_InitialStreamBuffer)
    int m_NumListeners;           // 0x7f4
    SFXPoint3 m_ListenerA;        // 0x7f8
    SFXPoint3 m_ListenerB;        // 0x804
    SFXPoint3 m_ListenerC;        // 0x810
    SFXPoint3 m_ListenerPos[4];   // 0x81c
    float m_Unk84c[3];            // 0x84c
    SFXPoint3 m_PrevListenerPos;  // 0x858
    SFXPoint3 m_Unk864;           // 0x864
    SFXPoint3 m_Unk870;           // 0x870
    SFXPoint3 m_Unk87c;           // 0x87c
    SFXPoint3 m_Unk888;           // 0x888
    int m_Unk894;                 // 0x894
    int m_Unk898;                 // 0x898
    int m_Debug[16];              // 0x89c SFXSetDebug
    int m_PreCache;               // 0x8dc
    int m_Unk8e0;                 // 0x8e0
    int m_Unk8e4;                 // 0x8e4
    int m_Unk8e8;                 // 0x8e8
    int m_Unk8ec;                 // 0x8ec
    int m_Unk8f0;                 // 0x8f0
    u32 m_FrameHashes[60];        // 0x8f4 SFX started this frame (once-per-frame SFX)
    int m_NumFrameHashes;         // 0x9e4
    SFXItem m_SFX[60];            // 0x9e8
    SFXItem *m_pSFXList;          // 0xe278 active list
    SFXItem *m_pFreeList;         // 0xe27c
    ES_LoadSoundBankDetails m_LB[10];       // 0xe280
    EXAudio_VoiceManager m_VoiceManager;    // 0xf2e8
    EXAudio_SoundBank m_Banks[105];         // 0xf928
    XFileHandle m_ProjectFile;    // 0x116b0 SFXInitialise2 first file
    int m_Unk11850;               // 0x11850
    int m_Unk11854;               // 0x11854
    int m_InitState;              // 0x11858
    int m_Unk1185c;               // 0x1185c
};
#endif
