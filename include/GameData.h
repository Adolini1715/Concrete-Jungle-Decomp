#ifndef GAMEDATA_H
#define GAMEDATA_H
#include "types.h"
#include "EXStdFuncs.h"
#include "EXMemCardStandIn.h"

// Persistent game data (GameData.obj): save header, memory card, settings, stats, ritual times
// and the big save image. sizeof 0x92d4.

// Per-game statistics (sizeof 0x68).
struct GameStats {
    GameStats() { memset(this, 0, sizeof(*this)); m_Unk48 = -1; }  // TODO check emitted copy (FELang.obj)
    void Reset() { memset(this, 0, sizeof(*this)); m_Unk48 = -1; }
    void ResetAll() { memset(this, 0, sizeof(*this)); m_Unk48 = -1; }
    u32 m_Unk0[2];          // TODO names
    int m_Kills;            // 0x08 kills of honourable targets (profile honour >= 0)
    int m_KillsDishonour;   // 0x0c kills of dishonourable targets (profile honour < 0)
    u32 m_Unk10[14];
    int m_Unk48;            // 0x48 reset to -1
    u32 m_Unk4c[6];         // 0x4c headshots, decapitations, arms severed, bodies split, burnt, (unused)
    int m_Recharges;        // 0x64 energy recharges used
    int FaveWeapon() { return m_Unk48; }
    float &LongestThrow() { return *(float *)&m_Unk10[7]; }    // 0x2c metres (Actor::UpdateThrownDistStat)
    float &LongestKick() { return *(float *)&m_Unk10[8]; }     // 0x30
};

// Player gear carried between levels, one record per level in GameValues (sizeof 0x18). TODO names
struct GearCarryOver {
    void WeaponArrayRestore();
    void Restore(signed char bFull);
    void Store(signed char bFull);
    s8 m_bValid;        // 0x00
    s8 m_bValidFull;    // 0x01
    short m_Ammo[6];    // 0x02 per carried weapon
    float m_Health;     // 0x10
    float m_Energy;     // 0x14
};


// Settings and progress (sizeof 0xe94). Field meanings from GameValues::ResetToDefaults (FELang.obj).
struct GameValues {
    GameValues();
    GameValues(int);
    void ResetToDefaults(s8 bKeepSettings);
    s8 RitualUnlocked(unsigned int i) const { return (m_Rituals & (1 << i)) != 0; }
    s8 AllTrainingComplete() { return m_Training == 7; }
    void CompleteRitual(unsigned int i) { m_RitualsComplete |= 1 << i; }
    void UnlockRitual(unsigned int i) { m_Rituals |= 1 << i; }
    unsigned int CountCompletedChapters();
    unsigned int CountCompletedRituals();
    s8 WeaponUpgradeReceived(unsigned int i);
    s8 RewardAwarded(unsigned int i);
    s8 UpgradeWeapon(unsigned int i);
    s8 AwardReward(unsigned int i);

    u32 m_Magic;            // 0x00 0x89ab001d
    s8 m_bGlobalValid;      // 0x04 SettingsValidateGlobal
    s8 m_bInvertX3rd;       // 0x05 options (defaults 1,1,0,1,0)
    s8 m_bInvertY3rd;       // 0x06
    s8 m_bInvertX1st;       // 0x07
    s8 m_bInvertY1st;       // 0x08
    s8 m_FirstPersonView;   // 0x09 "Enter 1st Person": 0 camera view, 1 predator view
    u8 m_ButtonConfig;      // 0x0a 0..2, picks the button glyph text (FEText::RemapHash); default 2
    s8 m_CameraMode;        // 0x0b 0 auto, 1 manual, 2 track
    u32 m_Unkc;             // 0x0c hash, default 0x100006a
    int m_MusicVolume;      // 0x10 default 70
    int m_MovieVolume;      // 0x14 default 100 (also the sfx volume)
    int m_ControllerConfig; // 0x18 Default_ControllerConfig
    u8 m_Unk1c[8];          // 0x1c [0] difficulty (0 easy: player damage x0.5, 1 normal, 2 hard: x1.5), [1] sound mode, [2] subtitles, [3] rumble, [6] HUD waypoint markers (1 always, 0 1st person only), [7] vsync
    u32 m_Unk24[5];         // 0x24
    u32 m_Unk38[8];         // 0x38
    u16 m_Unk58;            // 0x58
    s8 m_bAutoEquip;        // 0x5a auto-equip picked-up weapons
    s8 m_bActionCamera;     // 0x5b
    float m_Unk5c;          // 0x5c default 7.0
    s8 m_bLevelValid;       // 0x60 SettingsInvalidateLevel
    float m_Unk64[3];       // 0x64 default 100.0
    u32 m_Unk70[4];         // 0x70
    u32 m_Unk80[7];         // 0x80
    short m_Rituals;        // 0x9c unlocked ritual levels (bits)
    short m_Training;       // 0x9e
    short m_RitualsComplete;// 0xa0
    u32 m_Rewards;          // 0xa4
    u32 m_WeaponUpgrades;   // 0xa8
    GameStats m_Stats;      // 0xac
    GameStats m_LevelStats[27];         // 0x114
    GearCarryOver m_LevelRecords[27];   // 0xc0c gear at the end of each level
};

// Best times for the ritual levels (sizeof 0x5a8). TODO layout
struct RitualTimes {
    RitualTimes() { Reset(); }
    void Reset();
    u32 m_Magic;            // 0x00 0xfedc000a
    u8 m_Unk4[0x5a8 - 4];   // TODO layout
};

// Whole-game save image (sizeof 0x7ca0).
struct BigSave {
    BigSave();
    void Reset();
    unsigned int CalcSavesFound();
    void ResetRitualTimes() { m_RitualTimes.Reset(); }
    unsigned int Status();
    void SetStatus(unsigned int s);
    void SetStatusAndReset(unsigned int s);
    u32 m_Magic;                // 0x00 0x89ab001d
    int m_SaveFound[7];         // 0x04 TODO (bytes 4..0x18 tested)
    GameValues m_Saves[6];      // 0x20
    u8 m_Unk5798[0x76f8 - 0x5798];  // TODO layout
    RitualTimes m_RitualTimes;  // 0x76f8
};

struct BigSaveStatus {
    BigSaveStatus() { m_Status = 0; }
    unsigned int Status() const { return m_Status; }
    void Reset() { m_Status = 0; }
    void SetStatus(unsigned int s);
    unsigned int m_Status;
};

// Pending memory-card operation, advanced by GameData::DataTransfer. TODO names
namespace GDTransfer { enum Flags { GDT_NONE = 0, GDT_LOAD = 1, GDT_SAVE = 2, GDT_CHECKPRESENT = 3, GDT_CHECKSAVE = 4 }; }

class GameData {
public:
    static GameData *Instance(signed char create);
    EXMemCard &MemCard() { return m_MemCard; }
    void SettingsStore();
    void SettingsRestore(signed char b);
    void SavePre();
    void SavePost();
    void LoadPre();
    void LoadPost();
    signed char DataTransfer();
    void Save(unsigned char slot);
    void Load(unsigned char slot);
    GameStats *Stats() { return &m_Stats; }
    GameValues *Values() { return &m_Values; }
    void SettingsInvalidateLevel() { m_Values.m_bLevelValid = 0; }
    void SettingsValidateGlobal() { m_Values.m_bGlobalValid = 1; }
    void SettingsInvalidateGlobal() { m_Values.m_bGlobalValid = 0; }
    GDTransfer::Flags Transfer() { return m_Transfer; }
    s8 Busy() { return m_Transfer != 0; }
    EXSaveInfo &SaveInfo() { return m_SaveInfo; }
    RitualTimes *GetRitualTimes() { return &m_RitualTimes; }
    BigSave *GetBigSave() { return &m_BigSave; }
    BigSaveStatus *GetBigSaveStatus() { return &m_BigSaveStatus; }

    EXSaveInfo m_SaveInfo;      // 0x0000
    EXMemCard m_MemCard;        // 0x0064
    GDTransfer::Flags m_Transfer;   // 0x0180
    u8 m_SavesMask;             // 0x0184 slots holding a save (SavesPresent)
    u8 m_Slot;                  // 0x0185 slot being loaded/saved
    GameValues m_Values;        // 0x0188
    int m_Result;               // 0x101c last memory-card result
    GameStats m_Stats;          // 0x1020
    RitualTimes m_RitualTimes;  // 0x1088
    BigSave m_BigSave;          // 0x1630
    BigSaveStatus m_BigSaveStatus;  // 0x92d0
    ~GameData() { m_pInstance = 0; }  // deleted by XApp::ExitGame
private:
    GameData();
    static GameData *m_pInstance;
};

// The classic predator skins (GameValues::m_Unk58 -1..5); [fi] copy in Camera.obj.
inline signed char IsSkinClassic(int skin)
{
    if (skin >= -1 && skin <= 5) return 1;
    return 0;
}
// Global accessors (App.obj copies). Retail calls XGetGameValues out of line.
inline GameData *XGetGameData(signed char create) { return GameData::Instance(create); }
inline GameStats *XGetGameStats() { return GameData::Instance(1)->Stats(); }
__declspec(noinline) inline GameValues *XGetGameValues() { return GameData::Instance(1)->Values(); }
inline EXSaveInfo &XGetSaveInfo() { return GameData::Instance(1)->SaveInfo(); }

// Controller removal handling (GameData.obj).
#include "FEListener.h"
#include "MemCardStandIn.h"
class GameDialogWnd;
namespace GCRS { enum Flags { GCRS_NONE = 0 }; }
class GameControllerRemovedSys {
public:
    static void ControllerSet(int pad)
    {
        m_Controller = pad;
        FEListener::SetPadOverride((FEPad::Flags)(1 << pad));
    }
    static int Controller() { return m_Controller; }
    static GCRS::Flags Status() { return m_Flags; }
    static void HideDlg()
    {
        if (m_Dialog.m_pWnd) m_Dialog.HideDlg();
    }
    static GameDialogWnd *Wnd() { return m_Dialog.m_pWnd; }
    static signed char ControllerIsValid(int pad) { return m_bValid[pad]; }
    static void UpdateControllers();
    static signed char ReconnectCheck();
    static signed char DisconnectCheck(GCRS::Flags flags);
private:
    static int m_Countdown;
    static signed char m_bValid[4];
    static int m_Controller;
    static GCRS::Flags m_Flags;
    static GameDialog m_Dialog;
};
#endif
