#ifndef EXSFXCMD_PARSER_H
#define EXSFXCMD_PARSER_H
#include "types.h"

// Audio command ids (CommandDebugText names the first 35).
enum AUDIOCMDS {
    CMD_EXIT, CMD_SFX_INITIALISE, CMD_SFX_INITIALISE2, CMD_SFXLOADSOUNDBANK, CMD_SFXUNLOADSOUNDBANK,
    CMD_START_3D, CMD_START_MUSICXFADE, CMD_REMOVE, CMD_SET_SFX_VOLUME, CMD_SET_MUSIC_VOLUME,
    CMD_SFX_FADESFX_UP, CMD_SFX_FADESFX_DOWN, CMD_SFX_FADEMUSIC_UP, CMD_SFX_FADEMUSIC_DOWN,
    CMD_SETUP_LISTENER, CMD_SETNUMBEROF_LISTENERS, CMD_SFX_UPDATE, CMD_SFX_PRECACHE, CMD_SET_ENVIRONMENT,
    CMD_START_CUTSCENE, CMD_END_CUTSCENE, CMD_SET_TESTMODE, CMD_POST_MOVIE_RESET, CMD_PRE_MOVIE_RESET,
    CMD_ASYNC_STARTFILEREAD, CMD_ASYNC_STOPFILEREAD, CMD_DEBUG, CMD_END, CMD_SFXSETVOLUMEOVERRIDE,
    CMD_SFXSETPITCHOVERRIDE, CMD_SFXSETPOSITIONANDVELOCITY, CMD_SETMODE, CMD_ASYNC_SETLOADINGSPEED,
    CMD_SFX_SETGAMEEFFECT, CMD_SET_FILEPROFILER, CMD_35, CMD_SETDOPPLER, CMD_SETREVERBTYPE,
    CMD_ENABLEFILESTATS, CMD_SETISDVD
};

// Every command starts with this header; the next command is at (char *)this + m_Size.
struct EXSfxCmd {
    int m_Cmd;
    int m_Size;
};

// Executes batches of audio commands (sizeof 0xc4).
class EXSfxCmd_Parser {
public:
    virtual ~EXSfxCmd_Parser();
    virtual void Init();
    int CheckProcessCommands();
    int IsSFXToBeRemovedThisFrame(void *pObj, int hash);
    EXSfxCmd_Parser();
    void SendCommand(char *pCmd);
    void ProcessCommands();
protected:
    int ProcessCommand(AUDIOCMDS cmd, char *pCmd);
public:
    struct Removed {
        void *m_pObj;
        int m_Hash;
    };
    char *m_pBuffer[2];      // 0x04 batch buffers
    u32 m_Unk0c[3];          // 0x0c TODO
    char *m_pBatch[2];       // 0x18 batches waiting to be processed
    int m_RemoveCount;       // 0x20
    Removed m_Remove[20];    // 0x24 SFX removed this frame (CMD_REMOVE)
};

class XB_EXSfxCmd_Parser : public EXSfxCmd_Parser {
public:
    virtual void Init();
    virtual void SendBatchCommands(char *pCmds, int size);
    XB_EXSfxCmd_Parser() {}
};

extern char *CommandDebugText[];
#endif
