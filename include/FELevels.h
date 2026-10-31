#ifndef FELEVELS_H
#define FELEVELS_H
#include "types.h"

// One playable map: hash (and alternative hash), kind, title text and loading screen (FELevels.obj).
// sizeof 0x14. m_Type: 1 ritual, 2 training.
class FELevel {
public:
    FELevel()
    {
        m_Level = 0;
        m_LevelAlt = 0;
        m_Type = 0;
        m_Text = 0;
        m_LoadingTexture = 0;
    }
    FELevel(unsigned int level, unsigned int type, unsigned int text, unsigned int tex)
    {
        m_Level = level;
        m_LevelAlt = level;
        m_Type = type;
        m_Text = text;
        m_LoadingTexture = tex;
    }
    FELevel(unsigned int level, unsigned int alt, unsigned int type, unsigned int text, unsigned int tex)
    {
        m_Level = level;
        m_LevelAlt = alt;
        m_Type = type;
        m_Text = text;
        m_LoadingTexture = tex;
    }
    unsigned int Level() { return m_Level; }
    unsigned int LevelAlt() { return m_LevelAlt; }
    unsigned int Text() { return m_Text; }
    unsigned int LoadingTexture() { return m_LoadingTexture; }
    unsigned int Type() { return m_Type; }
    signed char IsStory() { return m_Type == 0; }
    signed char IsRitual() { return m_Type == 1; }
    signed char IsTraining() { return m_Type == 2; }

    unsigned int m_Level;           // 0x00
    unsigned int m_LevelAlt;        // 0x04
    unsigned int m_Type;            // 0x08
    unsigned int m_Text;            // 0x0c
    unsigned int m_LoadingTexture;  // 0x10
};

// Table of all levels (singleton, sizeof 0x508).
class FELevelManager {
public:
    static FELevelManager *Instance(signed char create);
    ~FELevelManager();
    unsigned int BaseFromLevel(unsigned int level);
    FELevel *LevelFromHash(unsigned int level, signed char exact);
    signed char IsIntroCutscene(unsigned int level);
    int IndexFromLevel(unsigned int level);
    unsigned int RitualLevelHashFromBitIndex(unsigned int bit);
    unsigned int RitualBitIndexFromLevelHash(unsigned int level);
    int TrainingIndexFromLevelHash(unsigned int level);
    unsigned int NextMap(int level);
    signed char IsStory(unsigned int level) { return LevelFromHash(level, 0)->IsStory(); }

    int LevelCount() { return m_Count; }
    unsigned int LevelFromIndex(int i) { return m_Levels[i].Level(); }
    // Verified against the copies emitted in FEMainMenu.obj.
    int StoryLevelCount() { return m_Unk4; }
    unsigned int TextFromIndex(int i) { return m_Levels[i].m_Text; }
    unsigned int LevelAltFromIndex(int i) { return m_Levels[i].m_LevelAlt; }
    unsigned int TextFromLevel(unsigned int level) { return LevelFromHash(level, 0)->Text(); }
    unsigned int LoadingTextureFromLevel(unsigned int level)
    {
        FELevel *p = LevelFromHash(level, 1);
        if (p) return p->LoadingTexture();
        return 0x600024d;
    }
    signed char IsRitual(unsigned int level) { return LevelFromHash(level, 0)->IsRitual(); }
    signed char IsTraining(unsigned int level) { return LevelFromHash(level, 0)->IsTraining(); }

    int m_Count;            // 0x000
    u32 m_Unk4;             // 0x004
    FELevel m_Levels[64];   // 0x008

private:
    FELevelManager();
    static FELevelManager *m_pInstance;
};

inline FELevelManager *FEGetLevels(signed char create) { return FELevelManager::Instance(create); }
#endif
