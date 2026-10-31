#include "EXSfxCmd_Parser.h"
#include "EXAudio.h"
#include "XBFileManager.h"
#include "EXMemory.h"

void SFXStartMusicXFade(int, int, int, int, int, XFileHandle *, int);  // EXAudioStream.obj

char *CommandDebugText[] = {
    "CMD_EXIT", "CMD_SFX_INITIALISE", "CMD_SFX_INITIALISE2", "CMD_SFXLOADSOUNDBANK", "CMD_SFXUNLOADSOUNDBANK",
    "CMD_START_3D", "CMD_START_MUSICXFADE", "CMD_REMOVE", "CMD_SET_SFX_VOLUME", "CMD_SET_MUSIC_VOLUME",
    "CMD_SFX_FADESFX_UP", "CMD_SFX_FADESFX_DOWN", "CMD_SFX_FADEMUSIC_UP", "CMD_SFX_FADEMUSIC_DOWN",
    "CMD_SETUP_LISTENER", "CMD_SETNUMBEROF_LISTENERS", "CMD_SFX_UPDATE", "CMD_SFX_PRECACHE", "CMD_SET_ENVIRONMENT",
    "CMD_START_CUTSCENE", "CMD_END_CUTSCENE", "CMD_SET_TESTMODE", "CMD_POST_MOVIE_RESET", "CMD_PRE_MOVIE_RESET",
    "CMD_ASYNC_STARTFILEREAD", "CMD_ASYNC_STOPFILEREAD", "CMD_DEBUG", "CMD_END", "CMD_SFXSETVOLUMEOVERRIDE",
    "CMD_SFXSETPITCHOVERRIDE", "CMD_SFXSETPOSITIONANDVELOCITY", "CMD_SETMODE", "CMD_ASYNC_SETLOADINGSPEED",
    "CMD_SFX_SETGAMEEFFECT", "CMD_SET_FILEPROFILER"
};

EXSfxCmd_Parser::~EXSfxCmd_Parser() {}

void EXSfxCmd_Parser::Init() {}

int EXSfxCmd_Parser::CheckProcessCommands()
{
    for (int i = 0; i < 2; i++) {
        if (m_pBatch[i]) return 1;
    }
    return 0;
}

int EXSfxCmd_Parser::IsSFXToBeRemovedThisFrame(void *pObj, int hash)
{
    for (int i = 0; i < m_RemoveCount; i++) {
        if (m_Remove[i].m_pObj == pObj && m_Remove[i].m_Hash == hash) return 1;
    }
    return 0;
}

#define ARG(n) (((int *)pCmd)[n])

int EXSfxCmd_Parser::ProcessCommand(AUDIOCMDS cmd, char *pCmd)
{
    switch (cmd) {
    case CMD_EXIT:
        Do_Exit();
        return 0;
    case CMD_SFX_INITIALISE:
        gpEXAudio->SFXInitialise();
        break;
    case CMD_SFX_INITIALISE2:
        DEBUGTRACE("CMD_SFX_INITIALISE2 : proj(%s)  stream(%s)\n", pCmd + 0x2c, pCmd + 0x1cc);
        gpEXAudio->SFXInitialise2((XFileHandle *)(pCmd + 8), (XFileHandle *)(pCmd + 0x1a8));
        break;
    case CMD_SFXLOADSOUNDBANK:
        DEBUGTRACE("CMD_SFXLOADSOUNDBANK : Hashcode %d  %s\n", ARG(2), pCmd + 0x30);
        gpEXAudio->SFXLoadSoundBank(ARG(2), (XFileHandle *)(pCmd + 0xc));
        break;
    case CMD_SFXUNLOADSOUNDBANK:
        gpEXAudio->SFXUnLoadSoundBank(ARG(2));
        break;
    case CMD_START_3D: {
        int hash = ARG(2);
        void *pObj = (void *)ARG(9);
        int a = ARG(10);
        int b = ARG(11);
        int c = ARG(12);
        int d = ARG(13);
        DEBUGTRACE("CMD_START_3D 0x%8.8x\n", hash);
        if (!IsSFXToBeRemovedThisFrame(pObj, hash))
            gpEXAudio->SFXStart3D(hash, (SFXPoint3 *)(pCmd + 0xc), pObj, a, b, c, d);
        break;
    }
    case CMD_START_MUSICXFADE:
        SFXStartMusicXFade(ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), (XFileHandle *)(pCmd + 0x20), ARG(7));
        break;
    case CMD_REMOVE:
        gpEXAudio->SFXRemove((void *)ARG(2), ARG(3));
        break;
    case CMD_SET_SFX_VOLUME:
        gpEXAudio->SFXSetSFXVolume(ARG(2));
        break;
    case CMD_SET_MUSIC_VOLUME:
        gpEXAudio->SFXSetMusicVolume(ARG(2));
        break;
    case CMD_SFX_FADESFX_UP:
        gpEXAudio->SFXFadeSFXUp(ARG(2));
        break;
    case CMD_SFX_FADESFX_DOWN:
        gpEXAudio->SFXFadeSFXDown(ARG(2), ARG(3));
        break;
    case CMD_SFX_FADEMUSIC_UP:
        gpEXAudio->SFXFadeMusicUp(ARG(2));
        break;
    case CMD_SFX_FADEMUSIC_DOWN:
        gpEXAudio->SFXFadeMusicDown(ARG(2), ARG(3));
        break;
    case CMD_SETUP_LISTENER:
        gpEXAudio->SFXSetupListener((SFXPoint3 *)(pCmd + 8), (SFXPoint3 *)(pCmd + 0x14), (SFXPoint3 *)(pCmd + 0x20),
                                    (SFXPoint3 *)(pCmd + 0x2c), (SFXPoint3 *)(pCmd + 0x38), ARG(17));
        break;
    case CMD_SETNUMBEROF_LISTENERS:
        gpEXAudio->SFXSetNumberOfListeners(ARG(2));
        break;
    case CMD_SET_ENVIRONMENT:
        gpEXAudio->SFXSetEnvironment(ARG(2), ARG(3), ARG(4));
        break;
    case CMD_START_CUTSCENE:
        gpEXAudio->SFXStartCutscene();
        break;
    case CMD_END_CUTSCENE:
        gpEXAudio->SFXEndCutscene();
        break;
    case CMD_POST_MOVIE_RESET:
        gpEXAudio->SFXPostMovieReset();
        break;
    case CMD_PRE_MOVIE_RESET:
        gpEXAudio->SFXPreMovieReset();
        break;
    case CMD_SET_TESTMODE:
        gpEXAudio->SFXSetTestMode(ARG(2));
        break;
    case CMD_ASYNC_STARTFILEREAD:
        AsyncReadFile((XFileHandle *)(pCmd + 8), (char *)ARG(0x6a), ARG(0x6b), ARG(0x6c));
        break;
    case CMD_ASYNC_STOPFILEREAD:
        AsyncStopReadFile(ARG(2));
        break;
    case CMD_ASYNC_SETLOADINGSPEED:
        gFileManager.SetLoadingSpeed(pCmd[8]);
        break;
    case CMD_ENABLEFILESTATS:
        gFileManager.EnableDebugStats(pCmd[8]);
        break;
    case CMD_SETISDVD:
        gFileManager.SetIsDVD(pCmd[8]);
        break;
    case CMD_SFX_PRECACHE:
        gpEXAudio->SFXPreCache(ARG(2), ARG(3));
        break;
    case CMD_DEBUG:
        gpEXAudio->SFXSetDebug((SFX_Debug_Data *)pCmd);
        break;
    case CMD_END:
        return 0;
    case CMD_SFXSETVOLUMEOVERRIDE:
        gpEXAudio->SFXSetVolumeOverRide(ARG(2), (const void *)ARG(3), ARG(4));
        break;
    case CMD_SFXSETPITCHOVERRIDE:
        gpEXAudio->SFXSetPitchOverRide(ARG(2), (const void *)ARG(3), ARG(4));
        break;
    case CMD_SFXSETPOSITIONANDVELOCITY:
        gpEXAudio->SFXSetPosition((SFXPoint3 *)(pCmd + 8), (const void *)ARG(5), ARG(6));
        break;
    case CMD_SETMODE:
        gpEXAudio->SFXSetMode(ARG(2));
        break;
    case CMD_SFX_SETGAMEEFFECT:
        gpEXAudio->SFXGameEffectMode((SFXGAMEEFFECT)ARG(2));
        break;
    case CMD_SET_FILEPROFILER:
        gpEXAudio->SFXGameEffectModeValues(ARG(2), ARG(3), ARG(4), ARG(5), ARG(6));
        break;
    case CMD_SETDOPPLER:
        gpEXAudio->SetDoppler(ARG(2));
    case CMD_SETREVERBTYPE:
        gpEXAudio->SFXSetReverbType((SFXREVERBTYPES)ARG(2));
        break;
    }
    return 1;
}

EXSfxCmd_Parser::EXSfxCmd_Parser() {}

void EXSfxCmd_Parser::SendCommand(char *pCmd)
{
    ProcessCommand((AUDIOCMDS)((EXSfxCmd *)pCmd)->m_Cmd, pCmd);
}

void EXSfxCmd_Parser::ProcessCommands()
{
    m_RemoveCount = 0;
    int i;
    for (i = 0; i < 2; i++) {
        EXSfxCmd *p = (EXSfxCmd *)m_pBatch[i];
        if (p) {
            while (p->m_Cmd != CMD_EXIT && p->m_Cmd != CMD_END) {
                if (p->m_Cmd == CMD_REMOVE) {
                    m_Remove[m_RemoveCount].m_pObj = ((void **)p)[2];
                    m_Remove[m_RemoveCount].m_Hash = ((int *)p)[3];
                    if (++m_RemoveCount >= 20) m_RemoveCount = 19;
                }
                p = (EXSfxCmd *)((char *)p + p->m_Size);
            }
        }
    }
    for (i = 0; i < 2; i++) {
        EXSfxCmd *p = (EXSfxCmd *)m_pBatch[i];
        if (p) {
            while (ProcessCommand((AUDIOCMDS)p->m_Cmd, (char *)p)) p = (EXSfxCmd *)((char *)p + p->m_Size);
            m_pBatch[i] = 0;
        }
    }
}
