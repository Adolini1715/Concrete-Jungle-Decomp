#include "FEText.h"
#include "EXMemory.h"
#include "XAppStandIn.h"
#include "FELevels.h"
#include "FEHashCodes.h"

FELevelManager *FELevelManager::m_pInstance;

FELevelManager *FELevelManager::Instance(signed char create)
{
    if (!m_pInstance && create) m_pInstance = new FELevelManager;
    return m_pInstance;
}

FELevelManager::~FELevelManager() { m_pInstance = 0; }

unsigned int FELevelManager::BaseFromLevel(unsigned int level)
{
    for (int i = 0; i < m_Count; i++)
        if (m_Levels[i].m_Level == level || m_Levels[i].m_LevelAlt == level) return m_Levels[i].m_Level;
    return level;
}

FELevel *FELevelManager::LevelFromHash(unsigned int level, signed char exact)
{
    unsigned int base = BaseFromLevel(level);
    for (int i = 0; i < m_Count; i++)
        if (m_Levels[i].m_Level == base) return &m_Levels[i];
    if (exact) return 0;
    return &m_Levels[0];
}

signed char FELevelManager::IsIntroCutscene(unsigned int level)
{
    if (level == XApp::Frontend3DFile()) return 1;
    for (int i = 0; i < m_Count; i++)
        if (m_Levels[i].m_Level == level && m_Levels[i].m_LevelAlt != level) return 1;
    return 0;
}

int FELevelManager::IndexFromLevel(unsigned int level)
{
    for (int i = 0; i < m_Count; i++)
        if (m_Levels[i].m_Level == level || m_Levels[i].m_LevelAlt == level) return i;
    return -1;
}

unsigned int FELevelManager::RitualLevelHashFromBitIndex(unsigned int bit)
{
    switch (bit) {
    case 1: return HT_File_RitualAssault1Day;
    case 2: return HT_File_Copycat_Final_Ritual_Challenge;
    case 3: return HT_File_Ritual8;
    case 4: return HT_File_RitualAssault2Day;
    case 5: return HT_File_GWarfare_Ritual_Challenge;
    case 6: return HT_File_RazeTheDepot_Ritual_Challenge;
    case 7: return HT_File_RA_POTC2;
    case 8: return HT_File_Penthouse_RoS;
    }
    return 0;
}

unsigned int FELevelManager::RitualBitIndexFromLevelHash(unsigned int level)
{
    switch (level) {
    case HT_File_RitualAssault1Day: return 1;
    case HT_File_Copycat_Final_Ritual_Challenge: return 2;
    case HT_File_Ritual8: return 3;
    case HT_File_RitualAssault2Day: return 4;
    case HT_File_GWarfare_Ritual_Challenge: return 5;
    case HT_File_RazeTheDepot_Ritual_Challenge: return 6;
    case HT_File_RA_POTC2: return 7;
    case HT_File_Penthouse_RoS: return 8;
    }
    return 0;
}

int FELevelManager::TrainingIndexFromLevelHash(unsigned int level)
{
    switch (level) {
    case HT_File_Training1: return 0;
    case HT_File_Training2: return 1;
    case HT_File_Training3: return 2;
    }
    return -1;
}

unsigned int FELevelManager::NextMap(int level)
{
    unsigned int base = BaseFromLevel(level);
    for (int i = 0; i < m_Count - 1; i++)
        if (m_Levels[i].m_Level == base) return m_Levels[i + 1].m_Level;
    return base;
}
