#ifndef EXSOUNDHANDLE_H
#define EXSOUNDHANDLE_H
#include "types.h"
#include "EXList.h"
#include "EXVector.h"

// One playing sound (EXSoundManager.obj). Linked into its EXSoundTag through the EXLDListItem base and
// into the manager's list through m_ManagerList. TODO layout: only the fields Initialise touches.
class EXSoundHandle : public EXLDListItem {
public:
    void Initialise();
    EXSoundHandle *Next() const { return (EXSoundHandle *)m_pNext; }

private:
    void UpdateTrackingSound(const EXVector &pos);
    friend class EXSoundTag;
    friend class EXBaseSoundManager;
public:
    s8 UpdateState() const { return m_UpdateState; }
    void SetUpdateState(s8 b) { m_UpdateState = b; }
    EXODListItem m_ManagerList;  // 0x0c
    unsigned int m_Handle;       // 0x18 audio-thread handle
    unsigned int m_HashCode;     // 0x1c
    float m_TrackVolume;         // 0x20 AddTrackingSound
    float m_TrackRange;          // 0x24
    u16 m_Unk28;                 // 0x28 TODO
    s8 m_Unk2a;                  // 0x2a TODO
    s8 m_Unk2b;                  // 0x2b TODO
    s8 m_Unk2c;                  // 0x2c TODO
    s8 m_UpdateState;            // 0x2d < 0: the map zone table kills it (EXItemAnimator_Map.obj)
    s8 m_Unk2e;                  // 0x2e TODO
};
#endif
