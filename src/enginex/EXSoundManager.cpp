#include "EXBaseSoundManager.h"
#include "EXSfxCmd_Parser.h"
#include "EXStats.h"
#include "EXFileSys.h"
#include "EXMemory.h"
#include "EXSoundHandle.h"
#include "EXSoundTagStandIn.h"
#include "EXItemEnv.h"
#include <math.h>

// TODO: the rest of EXSoundHandle / EXSoundTag (StartSound needs EXSoundTag to derive from EXLDList),
// EXSoundDetailsFile and the larger EXBaseSoundManager functions
// (Update, LoadSoundBank, BuildSoundFileList, _StartSound ...) are not done yet.

extern "C" void *__cdecl memset(void *, int, unsigned int);
extern "C" void *__cdecl memcpy(void *, const void *, unsigned int);
extern "C" char *__cdecl strcpy(char *, const char *);
#pragma intrinsic(strcpy)

EXSoundDetails EXBaseSoundManager::DefaultSFXOutputData = { 10, 20, 0, 0, 1, 0, 0, 0.33333334f };
EXPtrArray<EXSoundDetailsFile> EXBaseSoundManager::m_SoundDetailsFileArray;
static s8 s_FirstSetMode = 1;
EXBaseSoundManager *EXBaseSoundManager::m_pSoundManager;
static EXMaths::vec_xyzw s_NoPos;  // position for non-positional sounds (never written)
static EXSfxCmd s_EndCmd;
static unsigned int s_BadBankHash;
static unsigned int s_BadMusicHash;

#define CMD(n) (((int *)m_Cmd)[n])

void EXSoundListener::UpdatePosition(const EXVector &pos, const EXVector &vel, const EXVector &front,
                                     const EXVector &top, const EXVector &right)
{
    m_Pos = pos;
    m_Vel = vel;
    m_Front = front;
    m_Top = top;
    m_Right = right;
}

EXSoundDetailsFile::EXSoundDetailsFile(const char *pName, unsigned int hash) : EXFile(pName, 0, 0)
{
    m_Hash = hash;
    m_Unk24 = 0;
    m_Unk28 = 0;
}

s8 EXSoundDetailsFile::OnLoad()
{
    return EXFile::OnLoad(_SystemHeapList[1]);
}

s8 EXSoundDetailsFile::OnLoaded()
{
    EXFile::OnLoaded();
    u32 *p = (u32 *)m_pMem;
    if (!p) return 0;
    EXAllocSetReloc(p, 1);
    if (p[0] != 0x5853554d) {
        for (;;) {}
    }
    m_Unk24 = p[8];
    m_Unk28 = p[9];
    return 1;
}

s8 EXSoundDetailsFile::OnKill()
{
    return EXFile::OnKill();
}

s8 EXBaseSoundManager::Init(EXBaseSoundManager *pManager)
{
    m_pSoundManager = pManager;
    return 1;
}

void EXBaseSoundManager::SfxSetEnvironment(s8 a, s8 b)
{
    m_Env0 = a;
    m_Env1 = b;
}

void EXBaseSoundManager::SfxSetReverbDepth(int depth)
{
    m_ReverbDepth = depth;
}

int EXBaseSoundManager::SfxVolume() const
{
    return m_SfxVolume;
}

int EXBaseSoundManager::MusicVolume() const
{
    return m_MusicVolume;
}

s8 EXBaseSoundManager::SfxIsPlaying(const EXSoundHandle *pHandle) const
{
    int hash = *(const int *)((const char *)pHandle + 0x1c) & 0xffffff;
    const char *pObj = (const char *)pHandle + 0x18;
    for (int i = 0; i < 60; i++) {
        if (m_Requested[i].m_Timer > 0 && (m_Requested[i].m_pObj == pObj || !pObj)) {
            if (m_Requested[i].m_Hash == hash || hash == -1) return 1;
        }
    }
    SFX_Debug_Data *pStatus = m_pAudioStatus;
    int n = pStatus->m_NumPlaying;
    if (n) {
        if (!pObj && hash == -1) return 1;
        for (int i = 0; i < n; i++) {
            if (pStatus->m_Playing[i].m_pObj == pObj || !pObj) {
                if (pStatus->m_Playing[i].m_Hash == hash || hash == -1) return 1;
            }
        }
    }
    return 0;
}

void EXBaseSoundManager::Exit() {}

void EXBaseSoundManager::SendCommandBlock(int force)
{
    if (m_CmdBuffer.m_Size) {
        if (force) {
            TransferCommandBlock(1);
            m_CmdBuffer.SwapBuffers();
        } else if (TransferCommandBlock(0)) {
            m_CmdBuffer.SwapBuffers();
        }
    } else {
        TransferCommandBlock(force);
    }
}

void EXBaseSoundManager::SendCommand(char *pCmd)
{
    int size = ((EXSfxCmd *)pCmd)->m_Size;
    if (((EXSfxCmd *)pCmd)->m_Cmd != CMD_END && m_CmdBuffer.m_Size + size + 8 > 0x4000) SendCommandBlock(1);
    memcpy(m_CmdBuffer.m_pWrite, pCmd, size);
    m_CmdBuffer.IncPtr(size);
}

void EXBaseSoundManager::Initialise2(const XFileHandle *pProject, const XFileHandle *pStream)
{
    char *p = m_Cmd;
    ((int *)p)[0] = CMD_SFX_INITIALISE2;
    ((int *)p)[1] = 0x348;
    *(XFileHandle *)(p + 8) = *pProject;
    *(XFileHandle *)(p + 0x1a8) = *pStream;
    SendCommand(p);
}

void EXBaseSoundManager::PreMovieReset()
{
    int *pCmd = (int *)m_Cmd;
    pCmd[0] = CMD_PRE_MOVIE_RESET;
    pCmd[1] = 8;
    SendCommand((char *)pCmd);
}

void EXBaseSoundManager::PostMovieReset()
{
    int *pCmd = (int *)m_Cmd;
    pCmd[0] = CMD_POST_MOVIE_RESET;
    pCmd[1] = 8;
    SendCommand((char *)pCmd);
}

void EXBaseSoundManager::LoadSoundBank2(int hash, const XFileHandle *pFile)
{
    char *p = m_Cmd;
    ((int *)p)[2] = hash;
    ((int *)p)[0] = CMD_SFXLOADSOUNDBANK;
    ((int *)p)[1] = 0x1ac;
    *(XFileHandle *)(p + 0xc) = *pFile;
    SendCommand(p);
}

void EXBaseSoundManager::UnLoadSoundBank(unsigned int hash)
{
    int *pCmd = (int *)m_Cmd;
    pCmd[0] = CMD_SFXUNLOADSOUNDBANK;
    pCmd[1] = 0xc;
    pCmd[2] = hash;
    SendCommand((char *)pCmd);
}

// NEAR: 2 instructions swapped (retail sets ecx = 0x68 before lea edi for the XFileHandle copy).
void EXBaseSoundManager::StartMusicXFade(int a, int b, int c, int d, int e, const XFileHandle *pFile, int f)
{
    char *p = m_Cmd;
    ((int *)p)[2] = a;
    ((int *)p)[3] = b;
    ((int *)p)[4] = c;
    ((int *)p)[5] = d;
    ((int *)p)[6] = e;
    ((int *)p)[0] = CMD_START_MUSICXFADE;
    ((int *)p)[1] = 0x1c0;
    ((int *)p)[7] = f;
    if (pFile) memcpy(p + 0x20, pFile, sizeof(XFileHandle));
    SendCommand(p);
}

void EXBaseSoundManager::Remove(const void *pObj, int hash)
{
    int *pCmd = (int *)m_Cmd;
    pCmd[3] = hash;
    pCmd[0] = CMD_REMOVE;
    pCmd[1] = 0x10;
    pCmd[2] = (int)pObj;
    SendCommand((char *)pCmd);
}

void EXBaseSoundManager::SetVolumeOverRide(const EXSoundHandle *pHandle, int volume)
{
    int *pCmd = (int *)m_Cmd;
    int hash = *(const int *)((const char *)pHandle + 0x1c) & 0xffffff;
    pCmd[0] = CMD_SFXSETVOLUMEOVERRIDE;
    pCmd[1] = 0x14;
    pCmd[2] = volume;
    pCmd[3] = (int)((const char *)pHandle + 0x18);
    pCmd[4] = hash;
    SendCommand((char *)pCmd);
}

void EXBaseSoundManager::SetPitchOverRide(const EXSoundHandle *pHandle, int pitch)
{
    int *pCmd = (int *)m_Cmd;
    int hash = *(const int *)((const char *)pHandle + 0x1c) & 0xffffff;
    pCmd[0] = CMD_SFXSETPITCHOVERRIDE;
    pCmd[1] = 0x14;
    pCmd[2] = pitch;
    pCmd[3] = (int)((const char *)pHandle + 0x18);
    pCmd[4] = hash;
    SendCommand((char *)pCmd);
}

void EXSoundHandle::Initialise()
{
    Init();
    m_ManagerList.Init();
    m_HashCode = 0;
    m_Unk2e = 0;
    m_Unk28 = 0;
    m_UpdateState = 0;
    m_Handle = 0;
    m_Unk2b = 0;
    m_Unk2c = 0;
}

void EXSoundTag::SetVolume(u32 vol) const
{
    for (EXSoundHandle *p = m_pHead; p; p = p->Next()) EXBaseSoundManager::GetSoundManager()->SetVolumeOverRide(p, vol);
}

void EXSoundTag::SetPitch(int pitch) const
{
    for (EXSoundHandle *p = m_pHead; p; p = p->Next()) EXBaseSoundManager::GetSoundManager()->SetPitchOverRide(p, pitch);
}

void EXSoundTag::SetVolume(unsigned int hash, unsigned int vol) const
{
    for (EXSoundHandle *p = m_pHead; p; p = p->Next()) {
        if (p->m_HashCode == hash) EXBaseSoundManager::GetSoundManager()->SetVolumeOverRide(p, vol);
    }
}

void EXSoundTag::SetPitch(unsigned int hash, int pitch) const
{
    for (EXSoundHandle *p = m_pHead; p; p = p->Next()) {
        if (p->m_HashCode == hash) EXBaseSoundManager::GetSoundManager()->SetPitchOverRide(p, pitch);
    }
}

signed char EXSoundTag::IsPlaying(unsigned int hash) const
{
    for (EXSoundHandle *p = m_pHead; p; p = p->Next()) {
        if (p->m_HashCode == hash && EXBaseSoundManager::GetSoundManager()->SfxIsPlaying(p)) return 1;
    }
    return 0;
}

signed char EXSoundTag::IsPlaying() const
{
    for (EXSoundHandle *p = m_pHead; p; p = p->Next()) {
        if (EXBaseSoundManager::GetSoundManager()->SfxIsPlaying(p)) return 1;
    }
    return 0;
}

void EXBaseSoundManager::SetPosition(EXSoundHandle *pHandle, const EXVector &pos)
{
    int *pCmd = (int *)m_Cmd;
    int hash = *(int *)((char *)pHandle + 0x1c);
    pCmd[0] = CMD_SFXSETPOSITIONANDVELOCITY;
    pCmd[1] = 0x1c;
    ((float *)pCmd)[2] = pos.x;
    ((float *)pCmd)[3] = pos.y;
    ((float *)pCmd)[4] = pos.z;
    pCmd[5] = (int)((char *)pHandle + 0x18);
    pCmd[6] = hash & 0xffffff;
    SendCommand((char *)pCmd);
}

void EXBaseSoundManager::SetupListener(const EXVector &pos, const EXVector &vel, const EXVector &front,
                                       const EXVector &top, const EXVector &right, int index)
{
    int *pCmd = (int *)m_Cmd;
    pCmd[0] = CMD_SETUP_LISTENER;
    pCmd[1] = 0x48;
    pCmd[2] = ((const int *)&pos)[0];
    pCmd[3] = ((const int *)&pos)[1];
    pCmd[4] = ((const int *)&pos)[2];
    pCmd[5] = ((const int *)&vel)[0];
    pCmd[6] = ((const int *)&vel)[1];
    pCmd[7] = ((const int *)&vel)[2];
    pCmd[8] = ((const int *)&front)[0];
    pCmd[9] = ((const int *)&front)[1];
    pCmd[10] = ((const int *)&front)[2];
    pCmd[11] = ((const int *)&top)[0];
    pCmd[12] = ((const int *)&top)[1];
    pCmd[13] = ((const int *)&top)[2];
    pCmd[14] = ((const int *)&right)[0];
    pCmd[15] = ((const int *)&right)[1];
    pCmd[16] = ((const int *)&right)[2];
    pCmd[17] = index;
    SendCommand((char *)pCmd);
}

void EXBaseSoundManager::SetNumberOfListeners(int n)
{
    int *pCmd = (int *)m_Cmd;
    pCmd[0] = CMD_SETNUMBEROF_LISTENERS;
    pCmd[1] = 0xc;
    pCmd[2] = n;
    SendCommand((char *)pCmd);
}

void EXBaseSoundManager::AsyncReadFile_(XFileHandle *pHandle, char *pDest, unsigned int length,
                                        unsigned int priority)
{
    char *p = m_Cmd;
    ((int *)p)[0] = CMD_ASYNC_STARTFILEREAD;
    ((int *)p)[1] = 0x1b4;
    *(XFileHandle *)(p + 8) = *pHandle;
    *(char **)(p + 0x1a8) = pDest;
    *(unsigned int *)(p + 0x1ac) = length;
    *(unsigned int *)(p + 0x1b0) = priority;
    SendCommand(p);
}

void EXBaseSoundManager::AsyncStopFileRead(unsigned int handle)
{
    int *pCmd = (int *)m_Cmd;
    pCmd[0] = CMD_ASYNC_STOPFILEREAD;
    pCmd[1] = 0xc;
    pCmd[2] = handle;
    SendCommand((char *)pCmd);
}

void EXBaseSoundManager::SetEnvironment(int a, int b, int c)
{
    int *pCmd = (int *)m_Cmd;
    pCmd[2] = a;
    pCmd[3] = b;
    pCmd[0] = CMD_SET_ENVIRONMENT;
    pCmd[1] = 0x14;
    pCmd[4] = c;
    SendCommand((char *)pCmd);
}

void EXBaseSoundManager::StartCutscene()
{
    int *pCmd = (int *)m_Cmd;
    pCmd[0] = CMD_START_CUTSCENE;
    pCmd[1] = 8;
    SendCommand((char *)pCmd);
}

void EXBaseSoundManager::EndCutscene()
{
    int *pCmd = (int *)m_Cmd;
    pCmd[0] = CMD_END_CUTSCENE;
    pCmd[1] = 8;
    SendCommand((char *)pCmd);
}

void EXBaseSoundManager::End()
{
    s_EndCmd.m_Cmd = CMD_END;
    s_EndCmd.m_Size = 8;
    SendCommand((char *)&s_EndCmd);
}

void EXBaseSoundManager::SetLoadingSpeed(s8 speed)
{
    int *pCmd = (int *)m_Cmd;
    pCmd[0] = CMD_ASYNC_SETLOADINGSPEED;
    pCmd[1] = 0xc;
    ((char *)pCmd)[8] = speed;
    SendCommand((char *)pCmd);
}

void EXBaseSoundManager::EnableDebugStats(s8 enable)
{
    int *pCmd = (int *)m_Cmd;
    pCmd[0] = CMD_ENABLEFILESTATS;
    pCmd[1] = 0xc;
    ((char *)pCmd)[8] = enable;
    SendCommand((char *)pCmd);
}

void EXBaseSoundManager::SetIsDVD(s8 dvd)
{
    int *pCmd = (int *)m_Cmd;
    pCmd[0] = CMD_SETISDVD;
    pCmd[1] = 0xc;
    ((char *)pCmd)[8] = dvd;
    SendCommand((char *)pCmd);
}

void EXBaseSoundManager::SFXGameEffectMode(SFXGAMEEFFECT mode)
{
    int *pCmd = (int *)m_Cmd;
    pCmd[0] = CMD_SFX_SETGAMEEFFECT;
    pCmd[1] = 0xc;
    pCmd[2] = mode;
    SendCommand((char *)pCmd);
}

void EXBaseSoundManager::SFXSetGameEffectValues(int a, float b, int c, int d, int e)
{
    int *pCmd = (int *)m_Cmd;
    pCmd[2] = a;
    pCmd[3] = (int)(b * 1000.0f);
    pCmd[4] = c;
    pCmd[5] = d;
    pCmd[0] = CMD_SET_FILEPROFILER;
    pCmd[1] = 0x1c;
    pCmd[6] = e;
    SendCommand((char *)pCmd);
}

void EXBaseSoundManager::SFXSetReverbType(SFXREVERBTYPES type)
{
    int *pCmd = (int *)m_Cmd;
    pCmd[0] = CMD_SETREVERBTYPE;
    pCmd[2] = type;
    SendCommand((char *)pCmd);
}

void EXBaseSoundManager::SetDoppler(unsigned int doppler)
{
    int *pCmd = (int *)m_Cmd;
    pCmd[0] = CMD_SETDOPPLER;
    pCmd[1] = 8;
    pCmd[2] = doppler;
    SendCommand((char *)pCmd);
}

int EXBaseSoundManager::GetLanguage() const
{
    return m_Language;
}

void EXBaseSoundManager::SetMode(int mode)
{
    if (s_FirstSetMode) {
        int *pCmd = (int *)m_Cmd;
        m_Mode = mode;
        s_FirstSetMode = 0;
        pCmd[0] = CMD_SETMODE;
        pCmd[1] = 0xc;
        pCmd[2] = mode;
        SendCommand((char *)pCmd);
    }
}

int EXBaseSoundManager::GetMode() const
{
    return m_Mode;
}

void EXBaseSoundManager::SetDebugFlags() {}

void EXBaseSoundManager::SFXAddToRequestedList(int hash, const void *pObj)
{
    if (!pObj && hash == -1) return;
    for (int i = 0; i < 60; i++) {
        if (!m_Requested[i].m_Timer) {
            m_Requested[i].m_Timer = 5;
            m_Requested[i].m_Hash = hash;
            m_Requested[i].m_pObj = pObj;
            return;
        }
    }
}

void EXBaseSoundManager::SFXUpdateRequestedList()
{
    for (int i = 0; i < 60; i++) {
        if (m_Requested[i].m_Timer > 0) m_Requested[i].m_Timer--;
    }
}

s8 EXBaseSoundManager::IsSoundBankLoaded(unsigned int hash) const
{
    SFX_Debug_Data *pStatus = m_pAudioStatus;
    if (!pStatus->m_BanksActive) return 1;
    hash &= 0xffffff;
    for (int i = 0; i < 10; i++) {
        if (pStatus->m_BankHash[i] == hash) return 1;
    }
    return 0;
}

unsigned int EXBaseSoundManager::GetFileStatus(int type) const
{
    if ((unsigned int)type >= 17) return 0;
    return m_pAudioStatus->m_FileStatus[type];
}

s8 EXBaseSoundManager::StreamOpen(const char *pName)
{
    if (m_StreamOpen) return 0;
    memset(&m_StreamFile, 0, sizeof(XFileHandle));
    strcpy(m_StreamFile.m_Name, pName);
    m_StreamOpen = 1;
    return 1;
}

void EXBaseSoundManager::StreamRead(void *pDest, int offset, int size)
{
    if (m_StreamOpen) {
        m_StreamFile.m_Size = size;
        m_StreamFile.m_ReadOffset = offset;
        m_StreamHandle += 0x100;
        m_pSoundManager->AsyncReadFile(&m_StreamFile, (char *)pDest, 0, m_StreamHandle);
    }
}

s8 EXBaseSoundManager::StreamIsBusy() const
{
    if (!m_StreamOpen) return 0;
    unsigned int handle = m_StreamHandle;
    return m_pSoundManager->GetFileStatus(handle & 0xff) != ((handle & 0xffffff83) | 0x83);
}

void EXBaseSoundManager::StreamClose()
{
    m_StreamOpen = 0;
}

void EXBaseSoundManager::RegisterWatcherVars() const {}

void EXBaseSoundManager::SendExitCommand()
{
    int *pCmd = (int *)m_Cmd;
    pCmd[0] = CMD_EXIT;
    pCmd[1] = 8;
    SendCommand((char *)pCmd);
    Exit();
    for (int i = 50; i; i--) SendCommandBlock(1);
}

void EXBaseSoundManager::Shutdown()
{
    m_pSoundManager->SendExitCommand();
    if (m_pSoundManager) {
        delete m_pSoundManager;
        m_pSoundManager = 0;
    }
}

void EXBaseSoundManager::RemoveListener(EXSoundListener *pListener)
{
    for (int i = m_Listeners.Size() - 1; i >= 0; i--) {
        if (m_Listeners[i] == pListener) {
            m_Listeners.RemoveAt(i, 1);
            if (pListener) delete pListener;
            return;
        }
    }
}

s8 EXBaseSoundManager::DeleteSoundDetailsTable(unsigned int hash)
{
    for (int i = 0; i < m_SoundDetailsFileArray.Size(); i++) {
        EXSoundDetailsFile *pFile = m_SoundDetailsFileArray[i];
        if (pFile->m_Hash == hash) {
            delete pFile;
            m_SoundDetailsFileArray.RemoveAt(i, 1);
            return 1;
        }
    }
    return 0;
}

s8 EXBaseSoundManager::RemoveSoundDetailsTable(const EXSoundDetailsFile *pFile)
{
    for (int i = 0; i < m_SoundDetailsFileArray.Size(); i++) {
        if (m_SoundDetailsFileArray[i] == pFile) {
            m_SoundDetailsFileArray.RemoveAt(i, 1);
            return 1;
        }
    }
    return 0;
}

s8 EXBaseSoundManager::IsSoundDetailsTableLoaded(unsigned int hash)
{
    for (int i = 0; i < m_SoundDetailsFileArray.Size(); i++) {
        if (m_SoundDetailsFileArray[i]->m_Hash == hash) return 1;
    }
    return 0;
}

void EXSoundListener::SystemUpdate(int index) const
{
    EXBaseSoundManager::GetSoundManager()->SetupListener(m_Pos, m_Vel, m_Front, m_Top, m_Right, index);
}

// The remove command as it is inlined into the tag and handle loops (field stores in this order).
inline void EXBaseSoundManager::RemoveCmd(const EXSoundHandle *pHandle)
{
    int *pCmd = (int *)m_Cmd;
    pCmd[3] = pHandle->m_HashCode & 0xffffff;
    pCmd[2] = (int)&pHandle->m_Handle;
    pCmd[0] = CMD_REMOVE;
    pCmd[1] = 0x10;
    SendCommand((char *)pCmd);
}

void EXBaseSoundManager::KillSoundPlaying(const EXSoundHandle *pHandle)
{
    int *pCmd = (int *)m_Cmd;
    int hash = *(const int *)((const char *)pHandle + 0x1c) & 0xffffff;
    pCmd[2] = (int)((const char *)pHandle + 0x18);
    pCmd[0] = CMD_REMOVE;
    pCmd[1] = 0x10;
    pCmd[3] = hash;
    SendCommand((char *)pCmd);
}

// NEAR: 2 instructions differ: the inlined KillSoundPlaying stores the handle address after the hash
// through ecx in retail (edx here).
void EXSoundTag::KillSoundPlayingLooping() const
{
    EXBaseSoundManager *pMgr = EXBaseSoundManager::GetSoundManager();
    EXSoundHandle *p = m_pHead;
    while (p) {
        EXSoundHandle *pNext = p->Next();
        if (p->m_Unk2a) {
            pMgr->KillSoundPlaying(p);
            p->m_Unk2c = 0;
            p->Remove();
        }
        p = pNext;
    }
}

int EXBaseSoundManager::SfxNumberOfListeners() const
{
    int b = m_Listeners.Size();
    int a = m_NumListeners;
    return *(a <= b ? &a : &b);
}

void EXBaseSoundManager::SetSfxVolume(int volume)
{
    int *pCmd = (int *)m_Cmd;
    pCmd[0] = CMD_SET_SFX_VOLUME;
    pCmd[1] = 0xc;
    pCmd[2] = volume;
    SendCommand((char *)pCmd);
    m_SfxVolume = volume;
}

void EXBaseSoundManager::SetMusicVolume(int volume)
{
    int *pCmd = (int *)m_Cmd;
    pCmd[0] = CMD_SET_MUSIC_VOLUME;
    pCmd[1] = 0xc;
    pCmd[2] = volume;
    SendCommand((char *)pCmd);
    m_MusicVolume = volume;
}

void EXBaseSoundManager::FadeMusicDown(int time, s8 keep)
{
    int *pCmd = (int *)m_Cmd;
    pCmd[2] = time;
    pCmd[0] = CMD_SFX_FADEMUSIC_DOWN;
    pCmd[1] = 0x10;
    pCmd[3] = keep;
    SendCommand((char *)pCmd);
    if (!keep) {
        m_MusicHash = 0xffffffff;
        m_MusicKeep = keep;
    }
}

void EXBaseSoundManager::FadeMusicUp(int time)
{
    int *pCmd = (int *)m_Cmd;
    pCmd[0] = CMD_SFX_FADEMUSIC_UP;
    pCmd[1] = 0xc;
    pCmd[2] = time;
    SendCommand((char *)pCmd);
}

void EXBaseSoundManager::FadeSFXDown(int time, s8 keep)
{
    int *pCmd = (int *)m_Cmd;
    pCmd[2] = time;
    pCmd[0] = CMD_SFX_FADESFX_DOWN;
    pCmd[1] = 0x10;
    pCmd[3] = keep;
    SendCommand((char *)pCmd);
}

void EXBaseSoundManager::FadeSFXUp(int time)
{
    int *pCmd = (int *)m_Cmd;
    pCmd[0] = CMD_SFX_FADESFX_UP;
    pCmd[1] = 0xc;
    pCmd[2] = time;
    SendCommand((char *)pCmd);
}

void EXBaseSoundManager::PreCache(int hash, float amount)
{
    int *pCmd = (int *)m_Cmd;
    pCmd[3] = (int)(amount * 100.0f);
    pCmd[0] = CMD_SFX_PRECACHE;
    pCmd[1] = 0x10;
    pCmd[2] = hash & 0xffffff;
    SendCommand((char *)pCmd);
}

void EXBaseSoundManager::DeloadSoundBank(unsigned int hash)
{
    int *pCmd;
    if (hash == 0xffffffff) {
        pCmd = (int *)m_Cmd;
        pCmd[2] = hash;
    } else {
        if ((hash & 0x1ae00000) != 0x1ae00000) {
            s_BadBankHash = hash;
            return;
        }
        pCmd = (int *)m_Cmd;
        pCmd[2] = hash & 0xffffff;
    }
    pCmd[0] = CMD_SFXUNLOADSOUNDBANK;
    pCmd[1] = 0xc;
    SendCommand((char *)pCmd);
}

void EXBaseSoundManager::StartMusic(unsigned int hash, int a, int b, s8 c, s8 d)
{
    if ((hash & 0x1b000000) != 0x1b000000) {
        s_BadMusicHash = hash;
        return;
    }
    int bank = (hash >> 12) & 0xfff;
    int file = (hash & 0xfff) | 0xf000;
    if ((bank & 0xe00) == 0xe00) {
        s_BadMusicHash = hash;
        return;
    }
    XFileHandle handle;
    if (GetXFileFromHash(&handle, file, 0)) {
        StartMusicXFade(file, bank, a, b, c, &handle, d);
        m_MusicHash = hash;
        m_MusicKeep = d;
        m_MusicTimer = 10;
    }
}

void EXBaseSoundManager::Start3D(int hash, const EXVector &pos, const void *pObj, int a, int b, int c, int d)
{
    char *p = m_Cmd;
    ((int *)p)[0] = CMD_START_3D;
    ((int *)p)[1] = 0x38;
    ((int *)p)[2] = hash;
    ((float *)p)[3] = pos.x;
    ((float *)p)[4] = pos.y;
    ((float *)p)[5] = pos.z;
    ((const void **)p)[9] = pObj;
    ((int *)p)[10] = a;
    ((int *)p)[11] = (int)((float)b * m_DistanceScale);
    ((int *)p)[12] = (int)((float)c * m_DistanceScale);
    ((int *)p)[13] = d;
    SFXAddToRequestedList(hash, pObj);
    SendCommand(p);
}

void EXBaseSoundManager::CommandUpdate(unsigned int interval)
{
    unsigned int now = g_Stats.UpTime();
    if (now - m_LastStatsTime > 1000) {
        m_LastStatsTime = now;
        for (int i = 0; i < 99; i++) {
            m_StatsHistory3[i] = m_StatsHistory3[i + 1];
            m_StatsHistory[i][0] = m_StatsHistory[i + 1][0];
            m_StatsHistory[i][1] = m_StatsHistory[i + 1][1];
            m_StatsHistory[i][2] = m_StatsHistory[i + 1][2];
            m_StatsHistory2[i] = m_StatsHistory2[i + 1];
        }
        SFX_Debug_Data *pStatus = m_pAudioStatus;
        m_StatsHistory[99][0] = pStatus->m_FilePercent[0];
        m_StatsHistory[99][1] = pStatus->m_FilePercent[1];
        m_StatsHistory[99][2] = pStatus->m_FilePercent[2];
        m_StatsHistory2[99] = pStatus->m_FileMaxSeek;
        m_StatsHistory3[99] = m_Unkf3c;
        m_Unkf3c = 0;
    }
    if (now - m_LastCommandTime >= interval) {
        m_LastCommandTime = now;
        SendCommandBlock(0);
        SFXUpdateRequestedList();
    }
}

void EXBaseSoundManager::AsyncReadFile(XFileHandle *pHandle, char *pDest, unsigned int length, unsigned int priority)
{
    char *p = m_Cmd;
    ((int *)p)[0] = CMD_ASYNC_STARTFILEREAD;
    ((int *)p)[1] = 0x1b4;
    *(XFileHandle *)(p + 8) = *pHandle;
    *(char **)(p + 0x1a8) = pDest;
    *(unsigned int *)(p + 0x1ac) = length;
    *(unsigned int *)(p + 0x1b0) = priority;
    SendCommand(p);
}

void EXBaseSoundManager::SetLanguage(int language)
{
    if (language != m_Language) {
        m_Language = language;
        m_Unk6c = 0;
        DeleteSoundDetailsTable(1);
    }
}

void EXBaseSoundManager::GetDriveActivity() const
{
    if (m_pAudioStatus->m_DriveActivity == -1) return;
    u8 *pAct = (u8 *)&g_EXFileLoadActivity.m_Unk04;
    if (m_pAudioStatus->m_DriveActivity == 0x4000000) pAct[0] = 8;
    if (m_pAudioStatus->m_DriveActivity == 0x3000000) pAct[1] = 8;
    if (m_pAudioStatus->m_DriveActivity == 0) pAct[3] = 8;
    if (m_pAudioStatus->m_DriveActivity == 0x1000000) pAct[3] = 8;
    if (m_pAudioStatus->m_DriveActivity == 0x2000000) pAct[2] = 8;
}

EXVector EXBaseSoundManager::GetListenerPos() const
{
    if (!m_Listeners.Size()) return EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    return m_Listeners.GetAt(0)->m_Pos;
}

s8 EXBaseSoundManager::StartSound(unsigned int hash)
{
    return _StartSound(hash) != 0;
}

s8 EXBaseSoundManager::StartSound(unsigned int hash, const EXVector &pos, float a, float b, int c, int d)
{
    return _StartSound(hash, pos, a, b, c, d) != 0;
}

s8 EXBaseSoundManager::StartSound(unsigned int hash, const EXVector &pos, int a, int b)
{
    return _StartSound(hash, pos, a, b) != 0;
}

EXBaseSoundManager::EXBaseSoundManager() : m_HandleAllocator(0x20)
{
    m_MusicKeep = 0;
    m_MusicTimer = 0;
    m_ReverbDepth = 0;
    m_NumListeners = 0;
    m_Env0 = 0;
    m_Env1 = 0;
    m_Language = 0;
    m_MusicEnabledWithMovie = 0;
    m_Unk6c = 0;
    m_Unk760 = 0;
    m_LastCommandTime = 0;
    m_LastStatsTime = 0;
    m_MusicHash = 0xffffffff;
    m_MusicVolume = 70;
    m_SfxVolume = 100;
    m_Mode = 1;
    memset(&m_Unk474, 0, 0x2d4);
    memset(m_StatsHistory, 0, sizeof(m_StatsHistory));
    memset(m_StatsHistory3, 0, sizeof(m_StatsHistory3));
    m_Unkf3c = 0;
    m_StreamOpen = 0;
    m_StreamHandle = 0x40000010;
    m_DistanceScale = 1.0f;
    memset(m_StatsHistory2, 0, sizeof(m_StatsHistory2));
    m_Unkf40 = 0;
}

EXSoundHandle *EXBaseSoundManager::AllocateHandle()
{
    EXSoundHandle *p = m_HandleAllocator.Get();
    p->Initialise();
    m_Handles.AddHead(p->m_ManagerList, p);
    return p;
}

EXSoundHandle *EXBaseSoundManager::_StartSound(unsigned int hash)
{
    EXSoundHandle *p = AllocateHandle();
    p->m_HashCode = hash;
    Start3D(hash & 0xffffff, (const EXVector &)s_NoPos, &p->m_Handle, -1, -1, -1, 0);
    return p;
}

EXSoundHandle *EXBaseSoundManager::_StartSound(unsigned int hash, const EXVector &pos, float vol, float range, int a,
                                               int b)
{
    EXSoundHandle *p = AllocateHandle();
    const EXSoundDetails &d = SoundDetails(hash);
    p->m_HashCode = hash;
    p->m_Unk2a = d.m_bLooping;
    if (vol == 0.0f) {
        vol = (float)d.m_MinDist;
        range = (float)d.m_MaxDist;
    }
    Start3D(hash & 0xffffff, pos, &p->m_Handle, a, (int)vol, (int)range, b);
    return p;
}

EXSoundHandle *EXBaseSoundManager::_StartSound(unsigned int hash, const EXVector &pos, int a, int b)
{
    EXSoundHandle *p = AllocateHandle();
    const EXSoundDetails &d = SoundDetails(hash);
    p->m_HashCode = hash;
    p->m_Unk2a = d.m_bLooping;
    if (!d.m_b3D) {
        Start3D(hash & 0xffffff, (const EXVector &)s_NoPos, &p->m_Handle, -1, -1, -1, 0);
        return p;
    }
    Start3D(hash & 0xffffff, pos, &p->m_Handle, a, d.m_MinDist, d.m_MaxDist, b);
    return p;
}

void EXBaseSoundManager::RemoveHandle(EXSoundHandle *pHandle, s8 force)
{
    if (force) RemoveCmd(pHandle);
    pHandle->Remove();
    m_Handles.Remove(pHandle->m_ManagerList);
    m_HandleAllocator.Remove(pHandle);
}

void EXBaseSoundManager::SfxRemoveAll(s8 force)
{
    while (m_Handles.Head()) {
        EXSoundHandle *p = (EXSoundHandle *)m_Handles.Head()->Object();
        if (force) RemoveCmd(p);
        p->Remove();
        m_Handles.Remove(p->m_ManagerList);
        m_HandleAllocator.Remove(p);
    }
}

EXSoundHandle *EXSoundTag::StartSound(unsigned int hash)
{
    EXSoundHandle *p = EXBaseSoundManager::GetSoundManager()->_StartSound(hash);
    if (p) ((EXLDList *)this)->AddHead(p);
    return p;
}

EXSoundHandle *EXSoundTag::StartSound(u32 hash, const EXVector &pos, float vol, float range, int reverb, int flags)
{
    EXSoundHandle *p = EXBaseSoundManager::GetSoundManager()->_StartSound(hash, pos, vol, range, reverb, flags);
    if (p) ((EXLDList *)this)->AddHead(p);
    return p;
}

EXSoundHandle *EXSoundTag::StartSound(u32 hash, const EXVector &pos, int vol, int flags)
{
    EXSoundHandle *p = EXBaseSoundManager::GetSoundManager()->_StartSound(hash, pos, vol, flags);
    if (p) ((EXLDList *)this)->AddHead(p);
    return p;
}

void EXSoundTag::RemoveAll(s8 force) const
{
    EXBaseSoundManager *pMgr = EXBaseSoundManager::GetSoundManager();
    EXSoundHandle *p;
    while ((p = m_pHead) != 0) {
        if (force || p->m_Unk2a) pMgr->RemoveCmd(p);
        p->m_Unk2c = 0;
        p->Remove();
    }
}

void EXSoundTag::Remove(unsigned int hash, s8 force) const
{
    EXBaseSoundManager *pMgr = EXBaseSoundManager::GetSoundManager();
    EXSoundHandle *p = m_pHead;
    while (p) {
        EXSoundHandle *pNext = p->Next();
        if (p->m_HashCode == hash) {
            if ((force || p->m_Unk2a) && force) pMgr->RemoveCmd(p);
            p->m_Unk2c = 0;
            p->Remove();
        }
        p = pNext;
    }
}

void EXSoundTag::KillSoundPlayingLooping(unsigned int hash) const
{
    EXBaseSoundManager *pMgr = EXBaseSoundManager::GetSoundManager();
    EXSoundHandle *p = m_pHead;
    while (p) {
        EXSoundHandle *pNext = p->Next();
        if (p->m_HashCode == hash && p->m_Unk2a) {
            pMgr->RemoveCmd(p);
            p->m_Unk2c = 0;
            p->Remove();
        }
        p = pNext;
    }
}

EXSoundListener::EXSoundListener()
{
    m_Active = 1;
    m_Pos = EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    m_Vel = EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    m_Front = EXVector(0.0f, 0.0f, 1.0f, 0.0f);
    m_Top = EXVector(0.0f, 1.0f, 0.0f, 0.0f);
    m_Right = EXVector(1.0f, 0.0f, 0.0f, 0.0f);
}

void EXSoundListener::UpdatePosition(const EXVector &pos, const EXVector &vel, const EXMatrix &mtx)
{
    m_Pos = pos;
    m_Vel = vel;
    m_Front = mtx.GetDir();
    m_Front.NormaliseXYZ();
    m_Top = mtx.GetUp();
    m_Top.NormaliseXYZ();
    m_Right = mtx.GetNorm();
    m_Right.NormaliseXYZ();
}

// Per frame: tick the music timer, send the active listeners, the environment, and age the handles
// (a handle is dropped once its audio-thread sound has stopped, unless it is a tracking sound).
void EXBaseSoundManager::Update()
{
    if (m_MusicTimer < 1) {
        if (m_pAudioStatus[1].m_FileStatus[0xf] == 0) m_MusicHash = 0xffffffff;
    } else {
        m_MusicTimer--;
    }
    m_NumListeners = 0;
    for (int i = 0; i < m_Listeners.Size(); i++) {
        EXSoundListener *l = m_Listeners[i];
        if (l->m_Active) {
            GetSoundManager()->SetupListener(l->m_Pos, l->m_Vel, l->m_Front, l->m_Top, l->m_Right, m_NumListeners++);
        }
    }
    SetNumberOfListeners(m_NumListeners);
    SetEnvironment(m_ReverbDepth, m_Env0, m_Env1);
    CommandUpdate(0x10);

    EXODListItem *pItem = m_Handles.Head();
    while (pItem) {
        EXSoundHandle *p = (EXSoundHandle *)pItem->Object();
        pItem = pItem->Next();
        if (p->m_Unk2e <= 1) {
            p->m_Unk2b = 1;
            p->m_Unk2e++;
            continue;
        }
        if (p->m_Unk28 != 0) {
            p->m_Unk28--;
            if (p->m_Unk28 == 0) {
                p->Remove();
                m_Handles.Remove(p->m_ManagerList);
                m_HandleAllocator.Remove(p);
                continue;
            }
        }
        if (!SfxIsPlaying(p)) {
            if (!p->m_Unk2c) {
                p->Remove();
                m_Handles.Remove(p->m_ManagerList);
                m_HandleAllocator.Remove(p);
            } else {
                p->m_Unk2b = 0;
            }
        }
    }
}

// Collect every sound-type file (id class 0x21) of all the file lists.
void EXBaseSoundManager::BuildSoundFileList()
{
    m_SoundFiles.SetGrowBy(0x10);
    m_SoundFiles.RemoveAll();
    EXFileSys *pSys = EXFileSys::GetFileSys();
    for (unsigned int i = 0; i < pSys->FileListCount(); i++) {
        EXFileList *pList = pSys->FileList(i);
        for (int j = 0; j < pList->m_Files.Size(); j++) {
            EXFILEINFO *pInfo = &pList->m_Files[j];
            if ((pInfo->m_Hash & 0x7f000000) == 0x21000000) {
                EXSoundFileEntry e;
                e.m_pInfo = pInfo;
                e.m_pList = pList;
                m_SoundFiles.Add(e);
            }
        }
    }
}

// Queue the (once only) audio project/stream setup, make sure the main details table is loaded and
// send the bank load command for the requested sound bank.
void EXBaseSoundManager::LoadSoundBank(unsigned int hash)
{
    if (!m_Unk6c) {
        XFileHandle project;
        XFileHandle stream;
        memset(&project, 0, sizeof(project));
        memset(&stream, 0, sizeof(stream));
        if (!GetXFileFromHash(&project, 0xc000, 0)) for (;;) {}
        GetXFileFromHash(&stream, (m_Language << 16) | 0xd000, 0);
        Initialise2(&project, &stream);
        m_Unk6c = 1;
    }
    bool found = false;
    for (int i = 0; i < m_SoundDetailsFileArray.Size(); i++) {
        if (m_SoundDetailsFileArray[i]->m_Hash == 1) {
            found = true;
            break;
        }
    }
    if (!found) {
        int lang = m_Language;
        DeleteSoundDetailsTable(1);
        EXFILEINFO *pInfo = EXFileSys::GetFileSys()->GetFileInfo((lang << 16) | 0x2100b000);
        if (pInfo) AddSoundDetailsTable(pInfo->m_pName, 1);
    }
    if ((hash & 0x1ae00000) != 0x1ae00000) {
        s_BadBankHash = hash;
        return;
    }
    XFileHandle bank;
    if (!GetXFileFromHash(&bank, (m_Language << 16) | (hash & 0xfff) | 0xe000, 0)) for (;;) {}
    LoadSoundBank2(hash & 0xffffff, &bank);
}

EXSoundListener *EXBaseSoundManager::AllocateListener()
{
    EXSoundListener *p = new EXSoundListener;
    m_Listeners.Add(p);
    return p;
}

EXBaseSoundManager::~EXBaseSoundManager()
{
    SfxRemoveAll(0);
    for (int i = 0; i < m_SoundDetailsFileArray.Size(); i++) {
        EXSoundDetailsFile *p = m_SoundDetailsFileArray[i];
        if (p) delete p;
    }
    for (int i = 0; i < m_Listeners.Size(); i++) EXFree(m_Listeners[i]);
}

s8 EXBaseSoundManager::AddSoundDetailsTable(const char *pName, unsigned int hash)
{
    if (hash) DeleteSoundDetailsTable(hash);
    if (!EXFileSys::GetFileSys()->GetFileInfo(pName)) return 0;
    EXSoundDetailsFile *pFile = new EXSoundDetailsFile(pName, hash);
    if (!pFile) return 0;
    m_SoundDetailsFileArray.Add(pFile);
    return EXFileSys::GetFileSys()->OnQueueFile(pFile, 0, 0) != 0;
}

s8 EXBaseSoundManager::AddSoundDetailsTable(unsigned int fileHash, unsigned int hash)
{
    if (hash) DeleteSoundDetailsTable(hash);
    EXFILEINFO *pInfo = EXFileSys::GetFileSys()->GetFileInfo(fileHash);
    if (!pInfo) return 0;
    return AddSoundDetailsTable(pInfo->m_pName, hash);
}

// Fill 'pHandle' with the four disc copies of the sound file whose hash is 'hash'.
int EXBaseSoundManager::GetXFileFromHash(XFileHandle *pHandle, unsigned int hash, int) const
{
    for (int i = 0; i < m_SoundFiles.Size(); i++) {
        const EXFILEINFO *pInfo = m_SoundFiles[i].m_pInfo;
        const EXFileList *pList = m_SoundFiles[i].m_pList;
        if (pInfo->m_Hash == (hash | 0x21000000)) {
            pHandle->m_Size = pInfo->m_Size;
            char *pName = pHandle->m_Name;
            for (unsigned int j = 0; j < 4; j++) {
                (&pHandle->m_Offset)[j] = pInfo->m_Sys[j].m_Offset;
                (&pHandle->m_ReadOffset)[j] = 0;
                pHandle->m_Unk19c = 1;
                const char *pDisc = (const char *)pList->m_DiscNames[pInfo->m_Sys[j].m_NameIndex];
                if (!pDisc) pDisc = "";
                EXFileSys::GetFileListFileName(pName, pDisc);
                pName += 0x5e;
            }
            return 1;
        }
    }
    return 0;
}

s8 EXBaseSoundManager::SfxIsValidRange(const EXVector &pos, float range)
{
    int n = m_Listeners.Size();
    if (m_NumListeners < n) n = m_NumListeners;
    for (int i = 0; i < n; i++) {
        EXSoundListener *l = m_Listeners[i];
        if (!l->m_Active) continue;
        float dx = pos.x - l->m_Pos.x;
        float dy = pos.y - l->m_Pos.y;
        float dz = pos.z - l->m_Pos.z;
        float dw = pos.w - l->m_Pos.w;
        float d2 = dy * dy + dz * dz + dw * dw + dx * dx;
        float d = d2 < 0.0f ? 0.0f : sqrtf(d2);
        if (d < range) return 1;
    }
    return 0;
}

void EXBaseSoundManager::SfxRemoveAllNonLoopingStreams()
{
    EXODListItem *pItem = m_Handles.Head();
    while (pItem) {
        EXSoundHandle *p = (EXSoundHandle *)pItem->Object();
        EXODListItem *pNext = pItem->Next();
        const EXSoundDetails &d = SoundDetails(p->m_HashCode);
        if (!d.m_bLooping && d.m_Unka) {
            RemoveCmd(p);
            p->Remove();
            m_Handles.Remove(p->m_ManagerList);
            m_HandleAllocator.Remove(p);
        }
        pItem = pNext;
    }
}

void EXSoundTag::Remove(EXSoundHandle *pHandle, s8 force) const
{
    EXBaseSoundManager *pMgr = EXBaseSoundManager::GetSoundManager();
    if ((force || pHandle->m_Unk2a) && force) pMgr->RemoveCmd(pHandle);
    pHandle->m_Unk2c = 0;
    pHandle->Remove();
}

// Positional sound that follows a world position: starts/stops itself as the position moves in and
// out of the audible range of a listener and of a soundable zone.
void EXSoundHandle::UpdateTrackingSound(const EXVector &pos)
{
    EXBaseSoundManager *pMgr = EXBaseSoundManager::GetSoundManager();
    EXMapOn mapOn = EXItemEnv::GetItemEnv()->GetMapOn(pos);
    if (mapOn.IsValid()) {
        if (EXItemEnv::GetItemEnv()->IsZoneSoundable(mapOn)) {
            float range = m_TrackRange;
            if (range == 0.0f) range = (float)pMgr->SoundDetails(m_HashCode).m_MaxDist;
            if (pMgr->SfxIsValidRange(pos, range)) {
                if (m_Unk2b) {
                    pMgr->SetPosition(this, pos);
                    return;
                }
                unsigned int hash = m_HashCode;
                const EXSoundDetails &d = pMgr->SoundDetails(hash);
                float r = m_TrackRange;
                float v = m_TrackVolume;
                if (r == 0.0f) {
                    r = (float)d.m_MaxDist;
                    v = (float)d.m_MinDist;
                }
                m_Unk2a = d.m_bLooping;
                m_Unk2b = 1;
                pMgr->Start3D(hash & 0xffffff, pos, &m_Handle, -1, (int)v, (int)r, 0);
                return;
            }
        }
        if (m_Unk2b) pMgr->KillSoundPlaying(this);
    }
}

const EXSoundDetails &EXBaseSoundManager::SoundDetails(unsigned int hash) const
{
    if ((hash & 0x7f000000) == 0x1a000000) {
        for (int i = 0; i < m_SoundDetailsFileArray.Size(); i++) {
            EXSoundDetailsFile *f = m_SoundDetailsFileArray[i];
            if (f->IsLoaded())
                return *(const EXSoundDetails *)((char *)f->DetailsHeader() + 0x28 + (hash & 0xfffff) * 0xc);
        }
    }
    return DefaultSFXOutputData;
}

void EXBaseSoundManager::SfxRemoveAll(unsigned int hash, s8 force)
{
    EXODListItem *pItem = m_Handles.Tail();
    while (pItem) {
        EXSoundHandle *p = (EXSoundHandle *)pItem->Object();
        pItem = pItem->Prev();
        if (p->m_HashCode == hash) {
            if (force) RemoveCmd(p);
            p->Remove();
            m_Handles.Remove(p->m_ManagerList);
            m_HandleAllocator.Remove(p);
        }
    }
}

void EXBaseSoundManager::SfxRemoveAllHandle(unsigned int handle, s8 force)
{
    EXODListItem *pItem = m_Handles.Tail();
    while (pItem) {
        EXSoundHandle *p = (EXSoundHandle *)pItem->Object();
        pItem = pItem->Prev();
        if (p->m_Handle == handle) {
            if (force) RemoveCmd(p);
            p->Remove();
            m_Handles.Remove(p->m_ManagerList);
            m_HandleAllocator.Remove(p);
        }
    }
}

void EXSoundTag::UpdatePosition(const EXVector &pos) const
{
    EXBaseSoundManager *pMgr = EXBaseSoundManager::GetSoundManager();
    for (EXSoundHandle *p = m_pHead; p; p = p->Next()) {
        if (!p->m_Unk2c) pMgr->SetPosition(p, pos);
        else p->UpdateTrackingSound(pos);
    }
}

EXSoundHandle *EXSoundTag::AddTrackingSound(u32 hash, const EXVector &pos, float vol, float range)
{
    EXSoundHandle *p = EXBaseSoundManager::GetSoundManager()->AllocateHandle();
    p->m_TrackVolume = vol;
    p->m_HashCode = hash;
    p->m_TrackRange = range;
    p->m_Unk2c = 1;
    ((EXLDList *)this)->AddHead(p);
    p->UpdateTrackingSound(pos);
    return p;
}
