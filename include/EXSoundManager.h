#ifndef EXSOUNDMANAGER_H
#define EXSOUNDMANAGER_H
#include "EXBaseSoundManager.h"

class XB_EXSfxCmd_Parser;

// Xbox sound manager: runs the audio command parser and mixer in-process (XBSoundManager.obj, sizeof 0x10f0).
class EXSoundManager : public EXBaseSoundManager {
public:
    virtual ~EXSoundManager();
    virtual s8 OpenSoundManager();
    virtual void Initialise();
    virtual unsigned int TransferCommandBlock(int force);
    EXSoundManager();

    XB_EXSfxCmd_Parser *m_pParser;  // 0x10ec
};
// [fi] copy in Actor.obj, emitted right after EXBaseSoundManager::GetSoundManager.
inline EXSoundManager *XGetSoundManager() { return (EXSoundManager *)EXBaseSoundManager::GetSoundManager(); }
#endif
