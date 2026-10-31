#ifndef EXBASESOUNDMANAGER_H
#define EXBASESOUNDMANAGER_H
#include "types.h"
#include "EXArray.h"
#include "EXList.h"
#include "EXVector.h"
#include "EXFile.h"
#include "EXAudioBase.h"
#include "AsyncFile.h"
#include "EXAllocator.h"

struct XFileHandle;
class EXSoundHandle;
class EXSoundListener;
class EXMatrix;
class EXFileList;
struct EXFILEINFO;

// One entry of the sound file list: file-list info plus its list.
struct EXSoundFileEntry {
    EXFILEINFO *m_pInfo;
    EXFileList *m_pList;
};

class EXSoundFileListArray : public EXDataArray<EXSoundFileEntry, const EXSoundFileEntry &> {
public:
    EXSoundFileListArray() : EXDataArray<EXSoundFileEntry, const EXSoundFileEntry &>(0, 16) {}
    ~EXSoundFileListArray() {}
};

// Double-buffered audio command block (sizeof 0x14).
class SFXCommandBuffer {
public:
    SFXCommandBuffer() {}
    ~SFXCommandBuffer() {}
    void Init()
    {
        m_Current = 0;
        m_pWrite = m_pBuffer[0];
        m_Size = 0;
    }
    void SwapBuffers()
    {
        m_Current ^= 1;
        m_pWrite = m_pBuffer[m_Current];
        m_Size = 0;
    }
    void IncPtr(int n)
    {
        m_Size += n;
        m_pWrite += n;
    }
    char *m_pBuffer[2];  // 0x00
    int m_Current;       // 0x08
    char *m_pWrite;      // 0x0c
    unsigned int m_Size; // 0x10
};

// Per-sound playback parameters from a sound details table (sizeof 0x10). TODO layout.
struct EXSoundDetails {
    s16 m_MinDist;      // 0x00
    s16 m_MaxDist;      // 0x02
    u32 m_Unk4;
    u8 m_bLooping;      // 0x08 copied to EXSoundHandle::m_Unk2a
    u8 m_Unk9;
    u8 m_Unka;
    u8 m_b3D;           // 0x0b positional (checked against the listener range)
    float m_Unkc;       // 0x0c
};

struct EXSoundDetailsFileHeader;

// A loaded sound details table ("MUSX" file).
class EXSoundDetailsFile : public EXFile {
public:
    EXSoundDetailsFile(const char *pName, unsigned int hash);
    virtual s8 OnLoad();
    virtual s8 OnLoaded();
    virtual s8 OnKill();
    unsigned int DetailsHandle() const { return m_Hash; }
    EXSoundDetailsFileHeader *DetailsHeader() const { return (EXSoundDetailsFileHeader *)m_pMem; }

    unsigned int m_Hash;  // 0x20
    u32 m_Unk24;          // 0x24 copied from the header (+0x20)
    u32 m_Unk28;          // 0x28 copied from the header (+0x24)
};

// Listener frame (sizeof 0x54).
class EXSoundListener {
public:
    EXSoundListener();
    void UpdatePosition(const EXVector &pos, const EXVector &vel, const EXVector &front, const EXVector &top,
                        const EXVector &right);
    void UpdatePosition(const EXVector &pos, const EXVector &vel, const EXMatrix &mtx);  // appended by the item agent
    void SystemUpdate(int index) const;
    const EXVector &Position() const { return m_Pos; }
    s8 IsActive() const { return m_Active; }

    EXVector m_Pos;    // 0x00
    EXVector m_Vel;    // 0x10
    EXVector m_Front;  // 0x20
    EXVector m_Top;    // 0x30
    EXVector m_Right;  // 0x40
    s8 m_Active;       // 0x50
};

// Platform-independent front end of the sound system: queues audio commands for the
// audio thread (sizeof 0x10ec). Commands are built in m_Cmd and copied into m_CmdBuffer.
class EXBaseSoundManager {
public:
    virtual ~EXBaseSoundManager();
    virtual s8 OpenSoundManager() = 0;
    virtual void AsyncReadFile(XFileHandle *pHandle, char *pDest, unsigned int length, unsigned int priority);
    virtual void Initialise() = 0;
    virtual void Exit();
    virtual unsigned int TransferCommandBlock(int force) = 0;

    static EXBaseSoundManager *GetSoundManager() { return m_pSoundManager; }
    unsigned int MusicHash() const { return m_MusicHash; }
    bool GetMusicEnabledWithMovie() const { return m_MusicEnabledWithMovie; }

    static s8 Init(EXBaseSoundManager *pManager);
    void SfxSetEnvironment(s8 a, s8 b);
    void SfxSetReverbDepth(int depth);
    int SfxVolume() const;
    int MusicVolume() const;
    void PreMovieReset();
    void PostMovieReset();
    void AsyncReadFile_(XFileHandle *pHandle, char *pDest, unsigned int length, unsigned int priority);
    void AsyncStopFileRead(unsigned int handle);
    void SetEnvironment(int a, int b, int c);
    void StartCutscene();
    void EndCutscene();
    void SetLoadingSpeed(s8 speed);
    void EnableDebugStats(s8 enable);
    void SetIsDVD(s8 dvd);
    void SFXGameEffectMode(SFXGAMEEFFECT mode);
    void SFXSetGameEffectValues(int a, float b, int c, int d, int e);
    void SFXSetReverbType(SFXREVERBTYPES type);
    void SetDoppler(unsigned int doppler);
    int GetLanguage() const;
    void SetMode(int mode);
    int GetMode() const;
    void SetDebugFlags();
    s8 IsSoundBankLoaded(unsigned int hash) const;
    unsigned int GetFileStatus(int type) const;
    s8 StreamOpen(const char *pName);
    void StreamRead(void *pDest, int offset, int size);
    s8 StreamIsBusy() const;
    void StreamClose();
    void RegisterWatcherVars() const;
    void SendExitCommand();
    static void Shutdown();
    void RemoveListener(EXSoundListener *pListener);
    const EXSoundDetails &SoundDetails(unsigned int hash) const;
    static s8 DeleteSoundDetailsTable(unsigned int hash);
    static s8 RemoveSoundDetailsTable(const EXSoundDetailsFile *pFile);
    static s8 IsSoundDetailsTableLoaded(unsigned int hash);
    int GetXFileFromHash(XFileHandle *pHandle, unsigned int hash, int unused) const;
    void KillSoundPlaying(const EXSoundHandle *pHandle);
    int SfxNumberOfListeners() const;
    void SetSfxVolume(int volume);
    void SetMusicVolume(int volume);
    void FadeMusicDown(int time, s8 keep);
    void FadeMusicUp(int time);
    void FadeSFXDown(int time, s8 keep);
    void FadeSFXUp(int time);
    void PreCache(int hash, float amount);
    void DeloadSoundBank(unsigned int hash);
    void StartMusic(unsigned int hash, int a, int b, s8 c, s8 d);
    void CommandUpdate(unsigned int interval);
    void SetLanguage(int language);
    void GetDriveActivity() const;
    EXVector GetListenerPos() const;
    void SfxRemoveAll(s8 force);
    void SfxRemoveAllNonLoopingStreams();
    void SfxRemoveAll(unsigned int hash, s8 force);
    void SfxRemoveAllHandle(unsigned int hash, s8 force);
    s8 SfxIsValidRange(const EXVector &pos, float range);
    void BuildSoundFileList();
    void Update();
    EXSoundListener *AllocateListener();
    static s8 AddSoundDetailsTable(const char *pName, unsigned int hash);
    static s8 AddSoundDetailsTable(unsigned int a, unsigned int b);
    void LoadSoundBank(unsigned int hash);
    s8 StartSound(unsigned int hash);
    s8 StartSound(unsigned int hash, const EXVector &pos, float a, float b, int c, int d);
    s8 StartSound(unsigned int hash, const EXVector &pos, int a, int b);
    EXBaseSoundManager();
protected:
    s8 SfxIsPlaying(const EXSoundHandle *pHandle) const;
    void SendCommandBlock(int force);
    void SendCommand(char *pCmd);
    inline void RemoveCmd(const EXSoundHandle *pHandle);
    void Initialise2(const XFileHandle *pProject, const XFileHandle *pStream);
    void LoadSoundBank2(int hash, const XFileHandle *pFile);
    void UnLoadSoundBank(unsigned int hash);
    void StartMusicXFade(int a, int b, int c, int d, int e, const XFileHandle *pFile, int f);
    void Remove(const void *pObj, int hash);
    void SetVolumeOverRide(const EXSoundHandle *pHandle, int volume);
    void SetPitchOverRide(const EXSoundHandle *pHandle, int pitch);
    void SetPosition(EXSoundHandle *pHandle, const EXVector &pos);
    void SetupListener(const EXVector &pos, const EXVector &vel, const EXVector &front, const EXVector &top,
                       const EXVector &right, int index);
    void SetNumberOfListeners(int n);
    void End();
    void SFXAddToRequestedList(int hash, const void *pObj);
    void SFXUpdateRequestedList();
    __declspec(noinline) void Start3D(int hash, const EXVector &pos, const void *pObj, int a, int b, int c, int d);  // never inlined in retail (6 calls)
    void RemoveHandle(EXSoundHandle *pHandle, s8 force);
    EXSoundHandle *AllocateHandle();
    EXSoundHandle *_StartSound(unsigned int hash);
    EXSoundHandle *_StartSound(unsigned int hash, const EXVector &pos, float a, float b, int c, int d);
    EXSoundHandle *_StartSound(unsigned int hash, const EXVector &pos, int a, int b);

    static EXSoundDetails DefaultSFXOutputData;
    static EXPtrArray<EXSoundDetailsFile> m_SoundDetailsFileArray;
    static EXBaseSoundManager *m_pSoundManager;
    friend class EXSoundListener;
    friend class EXSoundTag;
    friend class EXSoundHandle;

public:
    struct Requested {
        int m_Hash;
        const void *m_pObj;
        int m_Timer;
    };
    EXSoundFileListArray m_SoundFiles;                 // 0x004
    EXPtrArray<EXSoundListener> m_Listeners;           // 0x014
    EXAllocator<EXSoundHandle> m_HandleAllocator;      // 0x024
    EXODList m_Handles;                                // 0x038
    int m_NumListeners;                                // 0x040
    unsigned int m_MusicHash;                          // 0x044
    s8 m_MusicKeep;                                    // 0x048
    int m_ReverbDepth;                                 // 0x04c
    s8 m_Env0;                                         // 0x050
    s8 m_Env1;                                         // 0x051
    float m_DistanceScale;                             // 0x054
    int m_MusicTimer;                                  // 0x058
    int m_MusicVolume;                                 // 0x05c
    int m_SfxVolume;                                   // 0x060
    int m_Language;                                    // 0x064
    int m_Mode;                                        // 0x068
    int m_Unk6c;                                       // 0x06c
    bool m_MusicEnabledWithMovie;                      // 0x070
    char m_Cmd[0x403];                                 // 0x071 command being built
    int m_Unk474;                                      // 0x474
    Requested m_Requested[60];                         // 0x478
    SFX_Debug_Data *m_pAudioStatus;                    // 0x748
    SFXCommandBuffer m_CmdBuffer;                      // 0x74c
    int m_Unk760;                                      // 0x760
    unsigned int m_LastCommandTime;                    // 0x764
    unsigned int m_LastStatsTime;                      // 0x768
    u32 m_StatsHistory[100][3];                        // 0x76c
    u32 m_StatsHistory2[100];                          // 0xc1c
    u32 m_StatsHistory3[100];                          // 0xdac
    u32 m_Unkf3c;                                      // 0xf3c
    u32 m_Unkf40;                                      // 0xf40
    XFileHandle m_StreamFile;                          // 0xf44 the open stream
    u32 m_StreamHandle;                                // 0x10e4
    s8 m_StreamOpen;                                   // 0x10e8
};
#endif
