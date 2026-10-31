#ifndef ACTOROPTIONS_H
#define ACTOROPTIONS_H
#include "types.h"
#include "EXVector.h"
#include "EXArray.h"
#include "XItemHandler_Player.h"
#include <math.h>

class Actor;
extern EXFixedArray<unsigned char, 290> FlagChannels;

// A trigger box of actor options (Trigger_CreateActorOptions; sizeof 0x58). Flag channels are set
// from what is inside it each frame (PostProcess).
class ActorOptions {
public:
    ActorOptions()
    {
        m_Pos.x = 0.0f;
        m_Pos.y = 0.0f;
        m_Pos.z = 0.0f;
        m_Pos.w = 0.0f;
        m_Extent.x = 0.0f;
        m_Extent.y = 0.0f;
        m_Extent.z = 0.0f;
        m_Extent.w = 0.0f;
        m_Count = 0;
        m_bMimicryUsed = 0;
        m_BodyAwareness = 0;
        m_Profile = 0;
        m_CountChannel = 0;
        m_OccupiedChannel = 0;
        m_PlayerInChannel = 0;
        m_PlayerEnteredChannel = 0;
        m_VisionMode = 0;
        m_MimicryChannel = 0;
        m_PlayerOutChannel = 0;
        m_MimicrySound = 0;
        m_MimicrySoundChannel = 0;
        m_Flags = 0;
    }
    ActorOptions(EXVector pos, EXVector extent, int a, unsigned int b, int c, int d, int e, int f, int g, int h, int i,
                 unsigned int j, int k, int l);
    void MimicryUsed() { m_bMimicryUsed = 1; }
    signed char BodyAwareness() { return m_BodyAwareness > 0; }
    signed char ForceBodyAwareness() { return m_BodyAwareness == 2; }
    signed char Within(Actor *pActor);
    void PostProcess();
    unsigned int MimicrySound()
    {
        if (m_MimicrySoundChannel && !FlagChannels[m_MimicrySoundChannel]) return 0;
        return m_MimicrySound;
    }

    EXVector m_Pos;                 // 0x00 box centre
    EXVector m_Extent;              // 0x10 half extents
    int m_Count;                    // 0x20 matching actors inside this frame
    s8 m_bMimicryUsed;              // 0x24
    int m_BodyAwareness;            // 0x28 > 0 on, 2 forced
    int m_Profile;                  // 0x2c profile hash (0x47000000 = any)
    int m_CountChannel;             // 0x30 set while m_Count != 0
    int m_OccupiedChannel;          // 0x34 latched once m_Count != 0
    int m_PlayerInChannel;          // 0x38 set while the player is inside (in m_VisionMode)
    int m_PlayerEnteredChannel;     // 0x3c latched when the player enters (in m_VisionMode)
    int m_VisionMode;               // 0x40 0 = any
    int m_MimicryChannel;           // 0x44 latched on mimicry with the player inside
    int m_PlayerOutChannel;         // 0x48 set while the player is outside
    unsigned int m_MimicrySound;    // 0x4c
    int m_MimicrySoundChannel;      // 0x50
    u32 m_Flags;                    // 0x54 bit 0: the player is invisible inside
};

// All actor option boxes of the level (Actor.obj).
class ActorOptionsManager {
public:
    static void SetPlayerVisible(signed char b) { m_bPlayerVisible = b; }
    static signed char PlayerVisible() { return m_bPlayerVisible; }
    static ActorOptions *Defaults() { return &m_Defaults; }
    static void Init();
    static void Register(ActorOptions *p);
    ActorOptionsManager();
    ~ActorOptionsManager();
    static ActorOptions *Options(Actor *pActor);
    static void PostProcess();
    static void MimicryUsed() { m_bMimicryUsed = 1; }
private:
    static signed char m_bPlayerVisible;
    static int m_Count;
    static signed char m_bMimicryUsed;
    static ActorOptions *m_pData[32];
    static ActorOptions m_Defaults;
    friend class ActorOptions;
};
#endif
