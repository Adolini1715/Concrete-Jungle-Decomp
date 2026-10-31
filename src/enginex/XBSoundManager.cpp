#include "EXSoundManager.h"
#include "EXAudioVoiceManager.h"
#include "EXAudio.h"
#include "XBMixer.h"
#include "EXSfxCmd_Parser.h"
#include "XBFileManager.h"
#include "EXMemory.h"

extern "C" u32 __stdcall GetTickCount();
extern "C" unsigned long __stdcall XGetAudioFlags();

XBAudioVoiceManager *gpAudioVoice;

EXSoundManager::~EXSoundManager() {}

unsigned int EXSoundManager::TransferCommandBlock(int force)
{
    if (m_CmdBuffer.m_Size) {
        End();
        if (m_pParser) m_pParser->SendBatchCommands(m_CmdBuffer.m_pBuffer[m_CmdBuffer.m_Current], m_CmdBuffer.m_Size);
    }
    gFileManager.UpdateStats();
    gFileManager.HandleDiskErrors();
    if (m_pParser) m_pParser->ProcessCommands();
    gpEXAudio->SFXUpdate();
    if (gpAudioVoice->m_Active) gpAudioVoice->CheckForStoppedVoices();
    return 1;
}

void EXSoundManager::Initialise()
{
    m_pAudioStatus = (SFX_Debug_Data *)gpEXAudio->m_FileStatus;
    int *pCmd = (int *)m_Cmd;
    pCmd[0] = CMD_SFX_INITIALISE;
    pCmd[1] = 8;
    SendCommand((char *)pCmd);
}

void XBAudioVoiceManager::Init()
{
    m_Active = 1;
}

void XBAudioVoiceManager::Kill()
{
    m_Active = 0;
}

int XBAudioVoiceManager::GetTime()
{
    return GetTickCount();
}

EXSoundManager::EXSoundManager()
{
    m_CmdBuffer.m_pBuffer[0] = (char *)_EXAlloc(0x4000, 0);
    m_CmdBuffer.m_pBuffer[1] = (char *)_EXAlloc(0x4000, 0);
    m_CmdBuffer.Init();
    m_pParser = new XB_EXSfxCmd_Parser;
    m_pParser->Init();
}

s8 EXSoundManager::OpenSoundManager()
{
    gpEXAudio = new EXAudio;
    gpEXAudio->Init();
    gpAudioVoice = new XBAudioVoiceManager;
    gpAudioVoice->Init();
    gpMixer = (XBMixer *)new XBMixer_Five_One;
    gpMixer->Init();
    gFileManager.Startup();
    Initialise();
    SetEnvironment(m_ReverbDepth, m_Env0, m_Env1);
    SetMusicVolume(m_MusicVolume);
    SetSfxVolume(m_SfxVolume);
    SetLanguage(m_Language);
    u32 flags = XGetAudioFlags() & 0xffff;
    if (flags == 1) SetMode(0);
    else if (flags == 0) SetMode(1);
    else SetMode(4);
    return 1;
}
