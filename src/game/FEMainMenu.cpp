#include "FEText.h"
#include "EXMemory.h"
#include "XAppStandIn.h"
#include "EXBaseSoundManager.h"
#include "EXSoundManager.h"
#include "FEMainMenu.h"
#include "GameData.h"
#include "FELevels.h"
#include "FETopmost.h"
#include "EXTrigger.h"
#include "EXItemAnimator_Camera.h"
#include "EXItem.h"
#include "XItemHandler_CameraScript.h"
#include "XGameWnd.h"
#include "EXCamera.h"
#include "EXBaseDisplay.h"
#include "EXStdFuncs.h"
#include "MemCardStandIn.h"
#include "FEObjectScript.h"
#include "XItem.h"
#include "EXItemAnimator_Entity.h"
#include "EXGeoFile.h"
#include "FEHud.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

// GameValues inlines (the copies emitted in FEMainMenu.obj).
inline unsigned int CountBitsSet(unsigned int v)
{
    v = ((v >> 1) & 0x55555555) + (v & 0x55555555);
    v = ((v >> 2) & 0x33333333) + (v & 0x33333333);
    v = ((v >> 4) & 0x0f0f0f0f) + (v & 0x0f0f0f0f);
    return (v * 0x01010101) >> 24;
}
inline unsigned int GameValues::CountCompletedChapters()
{
    unsigned int i;
    for (i = 0; i < (unsigned int)FELevelManager::Instance(1)->StoryLevelCount(); i++)
        if (!m_LevelStats[i].m_Unk0[0]) break;
    return i;
}
inline unsigned int GameValues::CountCompletedRituals() { return CountBitsSet(m_RitualsComplete); }
inline s8 GameValues::WeaponUpgradeReceived(unsigned int i)
{
    if (i > 100) i -= 100;
    return (m_WeaponUpgrades & (1 << i)) != 0;
}
inline s8 GameValues::RewardAwarded(unsigned int i)
{
    if (i > 100) return WeaponUpgradeReceived(i);
    return (m_Rewards & (1 << i)) != 0;
}

extern unsigned int gCurrentMap_GeoHash;
extern float Mission_RitualCompleteTime;
extern unsigned int MemCard_PrevLevelHash;
extern unsigned int gPrevMap_GeoHash;
extern int Stats_FaveWeapon;
void SetMemCardFlow(int flow);
signed char MemCardAutoSaveEnabled();
signed char __cdecl MainMenu_Callback(FEObject *, FEObjEvent::Flags);
void MemCardEnableAutoSave(signed char b);
void MemCardSetSlot(unsigned int slot);
unsigned int MemCardSlot();
void MemCard_SaveInit();
void SetChoosingSlots(signed char a, signed char b);
unsigned int MemCardSavesFound();
signed char MemCardDlgIsOn();
int MemCardDlgUpdate(signed char reset);
unsigned int MemCardDlgMessageHash();
unsigned int MemCardDlgResult();
void MemCardDlgHide(signed char b);
void SetSlotsText();
void Show_MemCardExtraDialog_Query(unsigned int msg, int a, int b, int c, unsigned int d, int e, int f, unsigned int *p);
int MemCard_CheckSavePossible(signed char reset);
int MemCard_CheckSavesPresent(signed char reset, unsigned int slot);
signed char MemCardBusy();
void Hide_MemCardExtraDialog_NoDestroy();
void Show_MemCardExtraDialog_NoCreate();
int MemCard_Save(signed char reset, unsigned int slot);
void MemCard_LoadInit();
int MemCard_Load(signed char reset);
int MemCard_CardChanged(signed char reset);
signed char MemCard_HasBeenRemoved();
void ResetMemCardSaveFoundBits();
extern signed char MemCard_bNotFormatted;
signed char __cdecl Pause_ChangeVolume(FEObject *, FEObjEvent::Flags);

// Bonus tasks, weapon upgrades, attribute upgrades, costumes, reward index and weapon-upgrade base index
// unlocked by each level.
static const EXFixedArray<EXFixedArray<unsigned char, 6>, 27> s_LevelUpgrades = { {
    { { 1, 0, 0, 1, 1, 0 } },
    { { 1, 1, 1, 0, 2, 101 } },
    { { 0, 0, 0, 0, 0, 0 } },
    { { 1, 1, 1, 0, 3, 102 } },
    { { 0, 0, 0, 0, 0, 0 } },
    { { 0, 1, 0, 0, 0, 103 } },
    { { 1, 0, 1, 0, 4, 0 } },
    { { 1, 1, 0, 1, 5, 104 } },
    { { 1, 0, 1, 0, 6, 0 } },
    { { 0, 1, 0, 0, 0, 105 } },
    { { 1, 0, 1, 0, 7, 0 } },
    { { 1, 0, 1, 0, 8, 0 } },
    { { 0, 1, 0, 0, 0, 106 } },
    { { 1, 0, 0, 1, 9, 0 } },
    { { 1, 0, 0, 1, 10, 0 } },
    { { 0, 0, 0, 0, 0, 0 } },
    { { 1, 0, 1, 0, 11, 0 } },
    { { 1, 4, 1, 0, 12, 107 } },
    { { 0, 0, 0, 0, 0, 0 } },
    { { 1, 1, 1, 0, 13, 111 } },
    { { 1, 0, 1, 0, 14, 0 } },
    { { 1, 1, 1, 0, 15, 112 } },
    { { 0, 0, 0, 0, 0, 0 } },
    { { 1, 1, 1, 0, 16, 113 } },
    { { 1, 0, 0, 1, 17, 0 } },
    { { 1, 1, 0, 1, 18, 114 } },
    { { 0, 0, 0, 0, 0, 0 } }
} };

unsigned int FEMainMenu::ms_StatsState = 6;
int FEMainMenu::ms_RitualTimeBeaten = -1;
EXFixedArray<char, 16> FEMainMenu::ms_NameEdit = { "Predator" };
unsigned int FEMainMenu::ms_InitialLoadedFocus = 6;
unsigned int LazerState = 0x65;
EXItem *pLazerItem;
EXItem *pLazerItem2;
FEMainMenu *FEMainMenu::ms_pInstance;
signed char FEMainMenu::ms_bDisplayStats;
signed char FEMainMenu::ms_bLevelWasQuit;
signed char FEMainMenu::ms_bPredSkinLoaded;
signed char FEMainMenu::ms_bDisableSaveGame;
unsigned int Remember_RewardsAwarded;
unsigned int Remember_WeaponUpgradedReceived;
short Remember_RitualsUnlocked;
// View of RitualTimes (GameData.h has it as TODO layout): eight best times per ritual level.
struct RitualTimesView {
    u32 m_Magic;
    s8 m_Unk4;
    float m_Times[9][8];       // 0x008
    char m_Names[9][8][16];    // 0x128
};
// File statics (not in the map).
static FEObject *s_SaveLayouts[4];
static FEObject *s_SaveLayout;
static FEObject *s_LoadLayouts[4];
static FEObject *s_LoadLayout;
static signed char s_RememberAutoSave;
static unsigned int s_RememberSlot;
static signed char s_ChoosingSlots;
// Not in the map, but reloaded after calls like globals: external linkage.
unsigned int s_ChapterFocusLevel;
int s_ChapterFocusIndex;
unsigned int s_RitualFocusLevel;
EXFixedArray<XItemHandler_FETrophy *, 58> XItemHandler_FETrophy::ms_Objects;
unsigned int XItemHandler_FETrophy::ms_nObjects;
unsigned int XItemHandler_FETrophy::ms_DirFudgedDir;
unsigned int XItemHandler_FETrophy::ms_DirFudgedFrom;
signed char XItemHandler_FETrophy::ms_bWeaponsView;

void FEMainMenu::SET_INITIAL_LOADED_FOCUS()
{
    ms_InitialLoadedFocus = 5;
    if (GameData::Instance(1)->m_Values.m_LevelStats[0].m_Unk0[0] != 0 ||
        GameData::Instance(1)->Values()->AllTrainingComplete())
        ms_InitialLoadedFocus = 6;
}

void FEMainMenu::ResetUpgradesFromQuit()
{
    int i = FEGetLevels(1)->IndexFromLevel(gCurrentMap_GeoHash);
    if (i >= 0 && i < FELevelManager::Instance(1)->StoryLevelCount()) {
        GameData::Instance(1)->m_Values.m_Rewards = Remember_RewardsAwarded;
        GameData::Instance(1)->m_Values.m_WeaponUpgrades = Remember_WeaponUpgradedReceived;
        GameData::Instance(1)->m_Values.m_Rituals = Remember_RitualsUnlocked;
    }
}

signed char FEMainMenu::IngameIsRitualTimeBeaten()
{
    unsigned int bit = FEGetLevels(1)->RitualBitIndexFromLevelHash(gCurrentMap_GeoHash);
    RitualTimesView *rt = (RitualTimesView *)GameData::Instance(1)->GetRitualTimes();
    for (unsigned int i = 0; i < 8; i++)
        if (rt->m_Times[bit][i] > Mission_RitualCompleteTime) return 1;
    return 0;
}

signed char FEMainMenu::IsDisplayStats() { return ms_bDisplayStats; }

EXRuntimeClass *XItemHandler_FETrophy::_GetBaseClass() { return EXRUNTIME_CLASS(XItemHandler_FETrophy); }
const EXRuntimeClass XItemHandler_FETrophy::classXItemHandler_FETrophy = {
    0, "XItemHandler_FETrophy", sizeof(XItemHandler_FETrophy),
    (void *(__cdecl *)())XItemHandler_FETrophy::CreateObject };

XItemHandler_FETrophy *XItemHandler_FETrophy::CreateObject() { return new XItemHandler_FETrophy; }

XItemHandler_FETrophy::~XItemHandler_FETrophy() { ms_nObjects--; }

signed char XItemHandler_FETrophy::Initialise() { return EXItemHandler::Initialise() != 0; }

signed char XItemHandler_FETrophy::DoUpdate() { return 1; }

void FEMainMenu::DeleteContent() {}

void FEMainMenu::Exit()
{
    int v = GameData::Instance(1)->m_Values.m_MovieVolume;
    EXBaseSoundManager::GetSoundManager()->SetSfxVolume(v);
}

signed char FEMainMenu::UpdateStateNone(FEObjEvent::Flags ev) { return 1; }

unsigned int SetSaveLoadLayouts(unsigned int state)
{
    unsigned int i = 0;
    switch (state) {
    case 2: i = 0; break;
    case 3: i = 1; break;
    case 18: i = 2; break;
    case 12: i = 3; break;
    }
    s_SaveLayout = s_SaveLayouts[i];
    s_LoadLayout = s_LoadLayouts[i];
    return i;
}

void FEMainMenu::SetInitialChapterSelectFocus()
{
    s_ChapterFocusLevel = 0x100006a;
    if (ms_bDisplayStats) {
        if (ms_StatsState == 6 || ms_StatsState == 0x15) s_ChapterFocusLevel = MemCard_PrevLevelHash;
    } else {
        GameData *pData = GameData::Instance(1);
        if (!FEGetLevels(1)->IsRitual(pData->m_Values.m_Unkc)) s_ChapterFocusLevel = GameData::Instance(1)->m_Values.m_Unkc;
    }
    s_ChapterFocusIndex = FEGetLevels(1)->IndexFromLevel(s_ChapterFocusLevel);
    s_ChapterFocusLevel = FEGetLevels(1)->LevelAltFromIndex(s_ChapterFocusIndex);
}

void FEMainMenu::SetInitialRitualSelectFocus(signed char reset)
{
    s_RitualFocusLevel = 0x10000b1;
    if (reset) return;
    if (ms_bDisplayStats) {
        if (ms_StatsState == 7 || ms_StatsState == 0x16) s_RitualFocusLevel = MemCard_PrevLevelHash;
        return;
    }
    GameData *pData = GameData::Instance(1);
    if (FEGetLevels(1)->IsRitual(pData->m_Values.m_Unkc))
        s_RitualFocusLevel = GameData::Instance(1)->m_Values.m_Unkc;
}

void FEMainMenu::SetPredSkinUpdateState(short s)
{
    m_PredSkinUpdateState = s;
    m_PredSkinUpdateTime = 0.0f;
}

void SetLazerState(unsigned int s) { LazerState = s; }

FEObject *FEMainMenu::AddScrollGridText(FELayoutScrollGrid *pGrid, unsigned int col, unsigned int row,
                                        unsigned __int64 data, EXString text, EXW::TextAlign align, EXString help,
                                        signed char bNoFocus)
{
    FEObject *p = pGrid->ObjectAdd(col, row,
                                   new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, data, text, help, align, 0,
                                                0, 0, 0, (EXW::TextAlign)4));
    p->SetHelpText(help);
    if (bNoFocus) p->StateSub(8);
    return p;
}

char *FEMainMenu::GetStatsTimeStr(unsigned int t, char *pBuf, signed char bHours)
{
    unsigned int secs = t % 60;
    t /= 60;
    unsigned int mins = t % 60;
    t /= 60;
    unsigned int hours = t % 60;
    pBuf[0] = 0;
    if (hours || bHours) sprintf(pBuf + strlen(pBuf), "%d:", hours);
    sprintf(pBuf + strlen(pBuf), "%02d:%02d", mins, secs);
    return pBuf;
}

// NEAR: retail uses a direct 24-entry jump table with eax preset to 0 and a separate case-0 block;
// VC folds the zero cases into the default and emits a byte-indexed table.
// DIFF: orig routes each lane through stack temporaries (208 B).
__forceinline EXVector Interp(const EXVector &a, const EXVector &b, float t) { return EXVector(a.EXMaths::vec_xyzw::Interp(b, t)); }


void GearCarryOver::Store(signed char bFull)
{
    XItemHandler_Player *p = XItemHandler_Player::Instance();
    if (!p) return;
    if (bFull) m_bValidFull = 1;
    else m_bValid = 1;
    m_Ammo[0] = p->m_Ammo[8];
    m_Ammo[1] = p->m_Ammo[9];
    m_Ammo[2] = p->m_Ammo[10];
    m_Ammo[3] = p->m_Ammo[11];
    m_Ammo[4] = p->m_Ammo[5];
    m_Ammo[5] = p->m_Ammo[12];
    float lo = 25.0f;
    float hp = p->m_Health;
    m_Health = max(hp, lo);
    m_Energy = p->m_Energy;
}

char *FEMainMenu::Debug_StateName(unsigned int state)
{
    char *p = "?";
    switch (state) {
    case 0:
        return "State_None";
    case 1:
        return "State_Main";
    case 2:
        return "State_New_Game";
    case 3:
        return "State_Load_Game";
    case 13:
        return "State_Options";
    case 14:
        return "State_Options_Controller";
    case 15:
        return "State_Options_Camera";
    case 16:
        return "State_Options_Audio";
    case 23:
        return "State_Done";
    case 4:
        return "State_Loaded";
    case 6:
        return "State_Story_Level_Select";
    case 7:
        return "State_Ritual_Challenge_Select";
    case 8:
        return "State_View_Records";
    case 9:
        return "State_Hunting_Gear";
    case 10:
        return "State_Costume_Select";
    case 11:
        return "State_View_Trophies";
    case 18:
        return "State_Autosave";
    case 19:
        return "State_EditName";
    case 5:
        return "State_Training";
    case 12:
        return "State_Save_Game";
    case 17:
        return "State_Options_Game";
    case 20:
        return "State_Credits";
    case 21:
        return "State_Story_Level_Stats";
    case 22:
        p = "State_Ritual_Challenge_Times";
        break;
    }
    return p;
}

unsigned int FEMainMenu::CameraIndexFromState(unsigned int state)
{
    unsigned int i = 0;
    switch (state) {
    case 0:
        return 0;
    case 1:
        return 1;
    case 2:
        i = 2;
        break;
    case 3:
        return 0;
    case 4:
        return 0;
    case 6:
        return 1;
    case 7:
        i = 2;
        break;
    case 8:
        return 0;
    case 9:
        return 1;
    case 10:
        i = 2;
        break;
    case 11:
        return 0;
    case 13:
        return 1;
    case 14:
        i = 2;
        break;
    case 15:
        return 0;
    case 16:
        return 1;
    case 18:
        return 1;
    case 19:
        i = 2;
        break;
    case 23:
        i = 2;
        break;
    }
    return i;
}

void FEMainMenu::SetInfoObjText(char *pText, unsigned int help)
{
    m_InfoHelpText = help;
    if (!pText) {
        m_pInfoObj->StateSub(0x40);
        m_InfoHelpText = 0;
        return;
    }
    m_pInfoObj->SetText(EXString(pText), 0, 0);
    m_pInfoObj->StateAdd(0x40);
}

unsigned int GetLevelUpgrade_BonusTasks(unsigned int level) { return s_LevelUpgrades[level][0]; }
unsigned int GetLevelUpgrade_WeaponUpgrades(unsigned int level) { return s_LevelUpgrades[level][1]; }
unsigned int GetLevelUpgrade_AttributeUpgrades(unsigned int level) { return s_LevelUpgrades[level][2]; }
unsigned int GetLevelUpgrade_Costumes(unsigned int level) { return s_LevelUpgrades[level][3]; }
unsigned int GetLevelUpgrade_RewardIndex(unsigned int level) { return s_LevelUpgrades[level][4]; }
unsigned int GetLevelUpgrade_WeaponUpgradesBaseIndex(unsigned int level) { return s_LevelUpgrades[level][5]; }

void FEMainMenu::AddFEMarker(EXTrigger *pTrig)
{
    ms_FEMarkers[pTrig->m_Data[0]].Set(*(u32 *)&pTrig->m_Position.x, pTrig->m_Position.y, pTrig->m_Position.z, pTrig->m_Position.w);
}

FEMarker *FEMainMenu::Marker(unsigned int i) { return &ms_FEMarkers[i]; }


// NEAR: 2796/2800. In the 0x65 branch retail copies i to eax before the < 6 test and subtracts 6 in place
// (mov eax, ecx / sub eax, 6); VC here copy-propagates and emits lea eax, [ecx - 6].
void XItemHandler_FETrophy::Init()
{
    ms_Objects[ms_nObjects++] = this;
    EXGeoFile *pGeo = EXGeoFile::LoadGeoFile(XApp::Frontend3DFile(), 0, 0);
    int hash = m_Unk8;
    EXItem *pItem = m_pItem;
    unsigned int anim = m_UnkC;
    int x = 0;
    int y = 0;
    m_Unk20 = 0;
    if (hash >= 1000) {
        unsigned int k = hash - 1000u;
        m_Unk1c = 1000;
        m_Unk20 = k;
        EXVector sc(0.125f, 0.125f, 0.125f, 0.0f);
        if (k == 1) sc = EXVector(0.0125f, 0.0125f, 0.0125f, 0.0f);
        pItem->SetScale(sc);
    } else if (hash >= 0xc9) {
        int i = hash - 0xc9;
        int wx = 0;
        int wy = 0;
        m_Unk1c = 0xc9;
        m_Unk20 = i;
        m_Unk18 = anim;
        switch (i) {
        case 0:
            wx = 0;
            wy = 1;
            m_Scale = 12.7f;
            m_Unk3c = 8;
            m_Unk40 = 0;
            m_Unk24 = 0x51000999;
            m_Unk34 = 0x510005a5;
            break;
        case 1:
            wx = 0;
            wy = 2;
            m_Scale = 15.8f;
            m_Unk3c = 12;
            m_Unk40 = 0;
            m_Unk24 = 0x5100099a;
            m_Unk34 = 0x51000010;
            m_Unk30 = 0.45f;
            break;
        case 2:
            wx = 1;
            wy = 2;
            m_Scale = 10.7f;
            m_Unk3c = 5;
            m_Unk40 = 1;
            m_Unk24 = 0x51000994;
            m_Unk34 = 0x51000009;
            m_Unk38 = 0x51000981;
            m_Unk30 = 0.35f;
            break;
        case 3:
            wx = 2;
            wy = 2;
            m_Scale = 13.4f;
            m_Unk3c = 5;
            m_Unk40 = 0;
            m_Unk24 = 0x51000993;
            m_Unk34 = 0x51000009;
            break;
        case 4:
            wx = 3;
            wy = 2;
            m_Scale = 7.4f;
            m_Unk3c = 5;
            m_Unk24 = 0x51000995;
            m_Unk34 = 0x51000009;
            m_Unk30 = 0.6f;
            m_Unk40 = 2;
            m_Unk38 = 0x51000982;
            break;
        case 5:
            wx = 4;
            wy = 3;
            m_Scale = 6.5f;
            m_Unk3c = 1;
            m_Unk40 = 1;
            m_Unk24 = 0x51000985;
            m_Unk34 = 0x51000033;
            m_Unk38 = 0x51000981;
            m_Unk30 = 1.0f;
            break;
        case 6:
            wx = 5;
            wy = 3;
            m_Scale = 8.7f;
            m_Unk3c = 1;
            m_Unk40 = 0;
            m_Unk24 = 0x51000984;
            m_Unk34 = 0x51000033;
            m_Unk30 = 1.0f;
            break;
        case 7:
            wx = 6;
            wy = 3;
            m_Scale = 5.0f;
            m_Unk3c = 1;
            m_Unk24 = 0x51000986;
            m_Unk34 = 0x51000033;
            m_Unk30 = 1.0f;
            m_Unk40 = 2;
            m_Unk38 = 0x51000982;
            break;
        case 8:
            wx = 5;
            wy = 2;
            m_Scale = 6.6f;
            m_Unk3c = 7;
            m_Unk40 = 0;
            m_Unk24 = 0x51000996;
            m_Unk34 = 0x5100000b;
            m_Unk30 = -0.3f;
            break;
        case 9:
            wx = 4;
            wy = 2;
            m_Scale = 7.5f;
            m_Unk3c = 7;
            m_Unk40 = 1;
            m_Unk24 = 0x51000997;
            m_Unk34 = 0x5100000b;
            m_Unk38 = 0x51000981;
            m_Unk30 = -0.5f;
            break;
        case 10:
            wx = 6;
            wy = 2;
            m_Scale = 6.3f;
            m_Unk3c = 7;
            m_Unk24 = 0x51000998;
            m_Unk34 = 0x5100000b;
            m_Unk30 = -0.2f;
            m_Unk40 = 2;
            m_Unk38 = 0x51000982;
            break;
        case 11:
            wx = 4;
            wy = 1;
            m_Scale = 6.7f;
            m_Unk3c = 6;
            m_Unk40 = 1;
            m_Unk24 = 0x51000991;
            m_Unk34 = 0x510000b7;
            m_Unk38 = 0x51000981;
            break;
        case 12:
            wx = 5;
            wy = 1;
            m_Scale = 9.4f;
            m_Unk3c = 6;
            m_Unk40 = 0;
            m_Unk24 = 0x51000990;
            m_Unk34 = 0x510000b7;
            break;
        case 13:
            wx = 6;
            wy = 1;
            m_Scale = 8.0f;
            m_Unk3c = 6;
            m_Unk24 = 0x51000992;
            m_Unk34 = 0x510000b7;
            m_Unk40 = 2;
            m_Unk38 = 0x51000982;
            break;
        case 14:
            wx = 2;
            wy = 0;
            m_Scale = 2.6f;
            m_Unk3c = 4;
            m_Unk40 = 0;
            m_Unk24 = 0x5100098a;
            m_Unk34 = 0x51000007;
            m_Unk2c = 0.3f;
            break;
        case 15:
            wx = 1;
            wy = 0;
            m_Scale = 2.6f;
            m_Unk3c = 4;
            m_Unk40 = 1;
            m_Unk24 = 0x5100098b;
            m_Unk34 = 0x51000007;
            m_Unk38 = 0x51000981;
            m_Unk2c = 0.3f;
            break;
        case 16:
            wx = 3;
            wy = 0;
            m_Scale = 2.6f;
            m_Unk3c = 4;
            m_Unk24 = 0x5100098c;
            m_Unk34 = 0x51000007;
            m_Unk2c = 0.3f;
            m_Unk40 = 2;
            m_Unk38 = 0x51000982;
            break;
        case 17:
            wx = 4;
            wy = 0;
            m_Scale = 3.2f;
            m_Unk3c = 3;
            m_Unk40 = 1;
            m_Unk24 = 0x5100098e;
            m_Unk34 = 0x51000006;
            m_Unk38 = 0x51000981;
            m_Unk2c = 0.3f;
            m_Unk30 = -0.3f;
            break;
        case 18:
            wx = 5;
            wy = 0;
            m_Scale = 3.2f;
            m_Unk3c = 3;
            m_Unk40 = 0;
            m_Unk24 = 0x5100098d;
            m_Unk34 = 0x51000006;
            m_Unk2c = 0.3f;
            m_Unk30 = -0.6f;
            break;
        case 19:
            wx = 6;
            wy = 0;
            m_Scale = 3.0f;
            m_Unk3c = 3;
            m_Unk24 = 0x5100098f;
            m_Unk34 = 0x51000006;
            m_Unk2c = 0.3f;
            m_Unk30 = -0.3f;
            m_Unk40 = 2;
            m_Unk38 = 0x51000982;
            break;
        case 20:
            wx = 8;
            wy = 0;
            m_Scale = 2.4f;
            m_Unk3c = 2;
            m_Unk40 = 0;
            m_Unk24 = 0x51000987;
            m_Unk34 = 0x51000004;
            m_Unk2c = 0.3f;
            break;
        case 21:
            wx = 7;
            wy = 0;
            m_Scale = 2.0f;
            m_Unk3c = 2;
            m_Unk40 = 1;
            m_Unk24 = 0x51000988;
            m_Unk34 = 0x51000004;
            m_Unk38 = 0x51000981;
            m_Unk2c = 0.3f;
            break;
        case 22:
            wx = 9;
            wy = 0;
            m_Scale = 2.2f;
            m_Unk3c = 2;
            m_Unk24 = 0x51000989;
            m_Unk34 = 0x51000004;
            m_Unk2c = 0.3f;
            m_Unk40 = 2;
            m_Unk38 = 0x51000982;
            break;
        }
        x = wx;
        y = wy;
    } else if (hash >= 0x65) {
        hash -= 0x65;
        int i = hash;
        m_Unk1c = 0x65;
        m_Unk20 = i;
        anim = 0x200024e;
        if (m_Unk18 == 0 || m_Unk18 == 0x2000000) m_Unk18 = 0x2000282;
        m_Scale = 0.4f;
        int j = i;
        if (j < 6) {
            x = j % 3 + 4;
            y = j / 3;
        } else {
            j -= 6;
            x = j % 4 + 4;
            y = j / 4 + 2;
        }
        EXFixedArray<unsigned int, 18> text = { { 0x51000741, 0x51000742, 0x51000743, 0x51000744, 0x51000745, 0x51000746,
                                  0x51000747, 0x51000748, 0x51000749, 0x5100074a, 0x5100074b, 0x5100074c,
                                  0x5100074d, 0x5100074e, 0x5100074f, 0x51000750, 0x51000751, 0x51000752 } };
        m_Unk24 = text[(unsigned int)i];
    } else {
        hash--;
        int i = hash;
        m_Unk1c = 1;
        m_Unk20 = i;
        if (m_Scale == 0.0f) {
            float s;
            switch (i) {
            case 10: s = 14.0f; break;
            case 11: s = 14.5f; break;
            case 12: s = 15.0f; break;
            case 13: s = 9.0f; break;
            case 14: s = 11.0f; break;
            default: s = 17.0f; break;
            }
            m_Scale = s;
        }
        x = i % 4;
        y = i / 4;
        EXFixedArray<EXFixedArray<unsigned int, 3>, 15> info = { {
            { 0x51000640, 0x5100096f, 0 },
            { 0x51000641, 0x51000970, 0x51000979 },
            { 0x51000642, 0x51000970, 0x5100097a },
            { 0x51000643, 0x51000970, 0x5100097b },
            { 0x51000644, 0x51000971, 0 },
            { 0x51000645, 0x51000972, 0 },
            { 0x51000646, 0x51000973, 0x5100097c },
            { 0x51000647, 0x51000973, 0x5100097d },
            { 0x51000648, 0x51000973, 0x5100097e },
            { 0x51000649, 0x51000973, 0x5100097f },
            { 0x5100064a, 0x51000974, 0 },
            { 0x5100064e, 0x51000978, 0 },
            { 0x5100064b, 0x51000975, 0 },
            { 0x5100064d, 0x51000977, 0 },
            { 0x5100064c, 0x51000976, 0 }
        } };
        m_Unk24 = info[(unsigned int)i][0];
        m_Unk34 = info[(unsigned int)i][1];
        m_Unk38 = info[(unsigned int)i][2];
    }
    m_Unk10 = x;
    m_Unk14 = y;
    if (anim) {
        EXItemAnimator_Entity *pAnim = new EXItemAnimator_Entity;
        pAnim->Initialise(pGeo, anim);
        pItem->AddAnimator(pAnim);
    }
}

XItemHandler_FETrophy *XItemHandler_FETrophy::Object(unsigned int hash)
{
    for (unsigned int i = 0; i < ms_nObjects; i++)
        if (ms_Objects[i]->m_Unk8 == hash) return ms_Objects[i];
    return 0;
}

XItemHandler_FETrophy *XItemHandler_FETrophy::Object(EXPoint pt, signed char bVisible)
{
    for (unsigned int i = 0; i < ms_nObjects; i++) {
        if (ms_bWeaponsView) {
            if (ms_Objects[i]->m_Unk1c != 0xc9) continue;
        } else {
            if (ms_Objects[i]->m_Unk1c == 0xc9) continue;
        }
        if (ms_Objects[i]->m_Unk10 == pt.x && ms_Objects[i]->m_Unk14 == pt.y) {
            if (bVisible) {
                EXItem *pItem = ms_Objects[i]->m_pItem;
                if (!(pItem->m_itemFlags & 1)) return 0;
                if (pItem->m_renderInfo.RenderFlags() & 2) return 0;
            }
            return ms_Objects[i];
        }
    }
    return 0;
}

XItemHandler_FETrophy *XItemHandler_FETrophy::DirFudge(unsigned int from, unsigned int dir, EXPoint &pt)
{
    pt = GridPos();
    if (ms_DirFudgedFrom) {
        XItemHandler_FETrophy *p = Object(ms_DirFudgedFrom);
        ms_DirFudgedFrom = 0;
        switch (dir) {
        case 0xb:
            if (ms_DirFudgedDir == 0xc) {
                pt = p->GridPos();
                return p;
            }
            break;
        case 0xc:
            if (ms_DirFudgedDir == 0xb) {
                pt = p->GridPos();
                return p;
            }
            break;
        case 0xd:
            if (ms_DirFudgedDir == 0xe) {
                pt = p->GridPos();
                return p;
            }
            break;
        case 0xe:
            if (ms_DirFudgedDir == 0xd) {
                pt = p->GridPos();
                return p;
            }
            break;
        }
    }
    EXPoint old = pt;
    if (!ms_bWeaponsView) {
        switch (dir) {
        case 0xd:
            switch (m_Unk8) {
            case 9:
            case 13:
                pt.y++;
            case 1:
            case 5:
                pt.x = 8;
                break;
            }
            break;
        case 0xe:
            switch (m_Unk8) {
            case 0x72:
            case 0x76:
                pt.y--;
            case 0x67:
            case 0x6a:
            case 0x6e:
                pt.x = -1;
                break;
            }
            break;
        case 0xb:
            switch (m_Unk8) {
            case 3:
            case 4:
                pt.x--;
            case 1:
            case 2:
                pt.y = 4;
                break;
            case 0x65:
            case 0x66:
            case 0x67:
            case 0x6e:
                pt.y = 5;
                break;
            }
            break;
        case 0xc:
            switch (m_Unk8) {
            case 0xf:
                pt.x++;
            case 0xd:
            case 0xe:
            case 0x73:
            case 0x74:
            case 0x75:
            case 0x76:
                pt.y = -1;
                break;
            }
            break;
        }
        if (pt.x == old.x && pt.y == old.y) {
            switch (m_Unk8) {
            case 0xb:
            case 0xc:
                if (dir == 0xc) pt.x--;
                break;
            case 0xf:
                if (dir == 0xe) pt.y++;
                break;
            case 0x6f:
            case 0x73:
                if (dir == 0xd) pt.y--;
                break;
            }
        }
    } else {
        switch (dir) {
        case 0xe:
            if (GridPos() == EXPoint(6, 1) || GridPos() == EXPoint(6, 1) || GridPos() == EXPoint(6, 2)||
                GridPos() == EXPoint(6, 3)) {
                pt = EXPoint(7, 0);
            } else if (GridPos() == EXPoint(9, 0)) {
                pt = EXPoint(1, 0);
            }
            break;
        case 0xd:
            if (GridPos() == EXPoint(1, 0) || GridPos() == EXPoint(0, 1) || GridPos() == EXPoint(0, 2)) {
                pt = EXPoint(9, 0);
            } else if (GridPos() == EXPoint(4, 3)) {
                pt = EXPoint(3, 2);
            }
            break;
        case 0xb:
            if (GridPos() == EXPoint(1, 0)) {
                pt = EXPoint(1, 2);
            } else if (GridPos() == EXPoint(2, 0)) {
                pt = EXPoint(2, 2);
            } else if (GridPos() == EXPoint(3, 0)) {
                pt = EXPoint(3, 2);
            } else if (GridPos() == EXPoint(4, 0)) {
                pt = EXPoint(4, 3);
            } else if (GridPos() == EXPoint(5, 0)) {
                pt = EXPoint(5, 3);
            } else if (GridPos() == EXPoint(6, 0)) {
                pt = EXPoint(6, 3);
            }
            break;
        case 0xc:
            if (GridPos() == EXPoint(1, 2)) {
                pt = EXPoint(1, 0);
            } else if (GridPos() == EXPoint(2, 2)) {
                pt = EXPoint(2, 0);
            } else if (GridPos() == EXPoint(3, 2)) {
                pt = EXPoint(3, 0);
            } else if (GridPos() == EXPoint(4, 3)) {
                pt = EXPoint(4, 0);
            } else if (GridPos() == EXPoint(5, 3)) {
                pt = EXPoint(5, 0);
            } else if (GridPos() == EXPoint(6, 3)) {
                pt = EXPoint(6, 0);
            }
            break;
        }
    }
    if (pt.x != old.x || pt.y != old.y) {
        ms_DirFudgedFrom = from;
        ms_DirFudgedDir = dir;
        if (ms_bWeaponsView) return Object(pt, 0);
    }
    return 0;
}

FEMainMenu::~FEMainMenu()
{
    ms_bDisplayStats = 0;
    ms_bLevelWasQuit = 0;
    ms_pInstance = 0;
    ms_StatsState = 4;
}

void FEMainMenu::AddContent()
{
    MakeSafeRect();
    FEObject *p = ObjectAdd(4, new FEObject((FEObjType::Flags)4, 0x10000e5, 0x40004f9, 0xfdffffff, FE_Blank, FE_Blank,
                                            (EXW::TextAlign)3, 0, 0, 0, 0, (EXW::TextAlign)3));
    m_pFadeExtra = p;
    p->Script(0)->SetScale(1.5f);
    p->StateSub(0x48);
    p->MakeSafeRect();
    p->SetOrigin(EXPoint(-0x46, -0x235), (EXW::TextAlign)0);
    p->StateAdd(0x80000342);
    p->Script(0)->Play(0, 0);
    p->Script(0)->SetFrame(203.0f);
    p->Script(0)->Stop();

    m_pTitle = new FELayoutLayered(2, -0x46, 0x64, 0xfa, 0x18, (EXW::TextAlign)0);
    ObjectAdd(3, m_pTitle);
    m_pTitle->StateSub(0x48);
    p = m_pTitle->ObjectAdd(0, new FEObject((FEObjType::Flags)1, XApp::Frontend2DFile(), 0x6000234, 0xfdffffff, FE_Blank,
                                             FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
    p->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0x23, 0x23, 0x23, 0x58), 1);
    p->StateModify(0x40000, 8);
    m_pTitle->ObjectAdd(1, new FEObject((FEObjType::Flags)2, 0x100003a, 0x700000c, 0x5f5f5f5f74786554ui64, EXString("XXX"),
                                        FE_Blank, (EXW::TextAlign)7, 0, 0, 0, 0, (EXW::TextAlign)4))
        ->StateModify(0x40000, 8);

    m_pSlotsBackground = new FELayoutLayered(2, EXBaseDisplay::GetDisplay()->SafeFrame().Width() - 0x19f,
                                             EXBaseDisplay::GetDisplay()->SafeFrame().Height() - 0x19, 0x1b3, 0x18,
                                             (EXW::TextAlign)0);
    ObjectAdd(2, m_pSlotsBackground);
    m_pSlotsBackground->StateSub(0x48);
    p = m_pSlotsBackground->ObjectAdd(0, new FEObject((FEObjType::Flags)1, XApp::Frontend2DFile(), 0x6000234, 0xfdffffff,
                                                       FE_Blank, FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
    p->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0x23, 0x23, 0x23, 0x58), 1);
    p->StateSub(8);
    FEObject *pGrid = m_pSlotsBackground->ObjectAdd(1, new FELayoutFixedGrid(1, 4, 0, 0, 0, 0, (EXW::TextAlign)4));
    m_pInfoObj = pGrid->ObjectAdd(0, 0, new FEObject((FEObjType::Flags)2, 0x100003a, 0x700000c, 0xfdffffff, EXString("(Info)"),
                                                     FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4))
                     ->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0x5f, 0x5f, 0x5f, 0x80), 1);
    m_pSlotsInfo = pGrid->ObjectAdd(0, 1, new FEObject((FEObjType::Flags)2, 0x100003a, 0x700000c, 0xfdffffff, Text_Get(0x51000473),
                                                       FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4))
                       ->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0x5f, 0x5f, 0x5f, 0x80), 1);
    m_pHelpText = pGrid->ObjectAdd(0, 2, new FEObject((FEObjType::Flags)2, 0x100003a, 0x700000c, 0xfdffffff, Text_Get(0x510004d7),
                                                      FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4))
                      ->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0x5f, 0x5f, 0x5f, 0x80), 1);
    m_pCreditsObj = pGrid->ObjectAdd(0, 3, new FEObject((FEObjType::Flags)2, 0x100003a, 0x700000c, 0xfdffffff, EXString("(Set Later)"),
                                                        FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4))
                        ->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0x5f, 0x5f, 0x5f, 0x80), 1);

    m_pBackground = new FELayoutLayered(1, 0x12c, 0x12c, 0x64, 0x64, (EXW::TextAlign)0);
    ObjectAdd(0, m_pBackground);
    m_pBackground->StateSub(0x24a);
    p = m_pBackground->ObjectAdd(0, new FEObject((FEObjType::Flags)1, XApp::Frontend2DFile(), 0x6000234, 0xfdffffff, FE_Blank,
                                                  FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
    p->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0x23, 0x23, 0x28, 0x50), 1);
    p->StateSub(8);

    m_pLayout = new FELayoutLayered(0x18, 0, 0, EXBaseDisplay::GetDisplay()->SafeFrame().Width(),
                                    EXBaseDisplay::GetDisplay()->SafeFrame().Height(), (EXW::TextAlign)0);
    ObjectAdd(1, m_pLayout);

    p = ObjectAdd(6, new FEObject((FEObjType::Flags)4, 0x100001e, 0x400052d, 0xfdffffff, FE_Blank, FE_Blank,
                                  (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
    m_pArrowL = p;
    p->Script(0)->SetScale(0.2f);
    p->StateSub(8);
    p->SetOrigin(EXPoint(400, 400), (EXW::TextAlign)4);
    p->StateAdd(0x100);
    p->Script(0)->Play(0, 0);

    p = ObjectAdd(5, new FEObject((FEObjType::Flags)4, 0x100001e, 0x400052e, 0xfdffffff, FE_Blank, FE_Blank,
                                  (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
    m_pArrowR = p;
    p->Script(0)->SetScale(0.7f);
    p->StateSub(8);
    p->SetOrigin(EXPoint(200, 200), (EXW::TextAlign)4);
    p->StateAdd(0x100);
    p->Script(0)->Play(0, 0);
}

static inline unsigned int StatGrey(u8 v) { return ex::RGBA(v, v, v, 0x80); }
void FEMainMenu::InitStatsLayout(unsigned int level, FELayoutScrollGrid *pGrid, GameStats *pStats, char *pName, signed char b,
                                 EXString help)
{
    char buf[32];
    char timeStr[32];
    signed char bTotal = pName != 0;
    unsigned int lbl = 0;
    unsigned int id = 0;
    unsigned int row;
    if (bTotal) {
        GetStatsTimeStr(pStats->m_Unk0[0], timeStr, 0);
        if (!b) {
            AddScrollGridText(pGrid, lbl++, 0, id++, EXString("Total Game Time"), (EXW::TextAlign)7, help, 0)->StateSub(0x4000);
        }
        AddScrollGridText(pGrid, 0, 1, 0xfdffffff, EXString(timeStr), (EXW::TextAlign)7, help, 0);
        if (!b) {
            AddScrollGridText(pGrid, lbl++, 0, id++, EXString(""), (EXW::TextAlign)7, help, 0);
        }
        AddScrollGridText(pGrid, 1, 1, 0xfdffffff, EXString(""), (EXW::TextAlign)7, help, 0);
        row = 2;
    } else {
        GetStatsTimeStr(pStats->m_Unk0[0], timeStr, 0);
        if (!b) {
            AddScrollGridText(pGrid, lbl++, 0, id++, EXString("Time"), (EXW::TextAlign)7, help, 0)->StateSub(0x4000);
        }
        sprintf(buf, "%s", timeStr);
        AddScrollGridText(pGrid, 0, 1, 0xfdffffff, EXString(buf), (EXW::TextAlign)7, help, 0);
        GetStatsTimeStr(pStats->m_Unk0[1], timeStr, 0);
        if (!b) {
            AddScrollGridText(pGrid, lbl++, 0, id++, EXString("Best time"), (EXW::TextAlign)7, help, 0);
        }
        sprintf(buf, "%s", timeStr);
        AddScrollGridText(pGrid, 1, 1, 0xfdffffff, EXString(buf), (EXW::TextAlign)7, help, 0);
        if (!b) {
            AddScrollGridText(pGrid, lbl++, 0, id++, EXString(""), (EXW::TextAlign)7, help, 0);
        }
        AddScrollGridText(pGrid, 2, 1, 0xfdffffff, EXString(""), (EXW::TextAlign)7, help, 0);
        row = 3;
    }
    if (!pStats->m_Unk0[0]) pGrid->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0x5f, 0x5f, 0x5f, 0x80), 1);
    else pGrid->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 1);

    unsigned int nTasks = 0;
    unsigned int nWeapons = 0;
    unsigned int nAttribs = 0;
    unsigned int nCostumes = 0;
    unsigned int gotTasks = 0;
    unsigned int gotWeapons = 0;
    unsigned int gotAttribs = 0;
    unsigned int gotCostumes = 0;
    if (bTotal) {
        for (unsigned int i = 0; i < 27; i++) {
            nTasks += GetLevelUpgrade_BonusTasks(i);
            nWeapons += GetLevelUpgrade_WeaponUpgrades(i);
            nAttribs += GetLevelUpgrade_AttributeUpgrades(i);
            nCostumes += GetLevelUpgrade_Costumes(i);
            unsigned int r = GetLevelUpgrade_RewardIndex(i);
            if (r && GameData::Instance(1)->Values()->RewardAwarded(r)) {
                gotTasks++;
                if (GetLevelUpgrade_AttributeUpgrades(i)) gotAttribs++;
                if (GetLevelUpgrade_Costumes(i)) gotCostumes++;
            }
            unsigned int u = GetLevelUpgrade_WeaponUpgradesBaseIndex(i);
            if (u) {
                for (unsigned int k = 0; k < GetLevelUpgrade_WeaponUpgrades(i); k++)
                    if (GameData::Instance(1)->Values()->RewardAwarded(u++)) gotWeapons++;
            }
        }
        unsigned int chapters = GameData::Instance(1)->Values()->CountCompletedChapters();
        if (!b) {
            AddScrollGridText(pGrid, lbl++, 0, id++, EXString("Chapters completed"), (EXW::TextAlign)7, help, 0);
        }
        sprintf(buf, "%d/%d", chapters, 27);
        AddScrollGridText(pGrid, row++, 1, 0xfdffffff, EXString(buf), (EXW::TextAlign)7, help, 0);
        if (!b) {
            AddScrollGridText(pGrid, lbl++, 0, id++, EXString("Ritual Challenges completed"), (EXW::TextAlign)7, help, 0);
        }
        sprintf(buf, "%d/%d", GameData::Instance(1)->Values()->CountCompletedRituals(), 8);
        AddScrollGridText(pGrid, row++, 1, 0xfdffffff, EXString(buf), (EXW::TextAlign)7, help, 0);
        if (!b) {
            AddScrollGridText(pGrid, lbl++, 0, id++, EXString(""), (EXW::TextAlign)7, help, 0);
        }
        AddScrollGridText(pGrid, row++, 1, 0xfdffffff, EXString(""), (EXW::TextAlign)7, help, 0);
    } else {
        nTasks += GetLevelUpgrade_BonusTasks(level);
        nWeapons += GetLevelUpgrade_WeaponUpgrades(level);
        nAttribs += GetLevelUpgrade_AttributeUpgrades(level);
        nCostumes += GetLevelUpgrade_Costumes(level);
        unsigned int r = GetLevelUpgrade_RewardIndex(level);
        if (r && GameData::Instance(1)->Values()->RewardAwarded(r)) {
            gotTasks++;
            if (GetLevelUpgrade_AttributeUpgrades(level)) gotAttribs++;
            if (GetLevelUpgrade_Costumes(level)) gotCostumes++;
        }
        unsigned int u = GetLevelUpgrade_WeaponUpgradesBaseIndex(level);
        if (u) {
            for (unsigned int k = 0; k < nWeapons; k++)
                if (GameData::Instance(1)->Values()->RewardAwarded(u++)) gotWeapons++;
        }
    }

    ex::RGBA col(0x5f, 0x5f, 0x5f, 0x80);
    if (pStats->m_Unk0[0]) {
        col = !nTasks ? StatGrey(0x58) : StatGrey(0x7f);
        if (nTasks && gotTasks == nTasks) col = ex::RGBA(0x65, 0x65, 0x65, 0x80);
    }
    lbl = row;
    id = row;
    AddScrollGridText(pGrid, lbl++, 0, id++, EXString("Bonus tasks completed"), (EXW::TextAlign)7, help, 0)->SetMathmos((FEObjColour::Flags)3, col, 1);
    sprintf(buf, "%d/%d", gotTasks, nTasks);
    AddScrollGridText(pGrid, row++, 1, 0xfdffffff, EXString(buf), (EXW::TextAlign)7, help, 0)->SetMathmos((FEObjColour::Flags)3, col, 1);
    if (pStats->m_Unk0[0]) {
        u8 c = 0x58;
        if (nWeapons) c = 0x7f;
        col = StatGrey(c);
        if (nWeapons && gotWeapons == nWeapons) col = ex::RGBA(0x65, 0x65, 0x65, 0x80);
    }
    AddScrollGridText(pGrid, lbl++, 0, id++, EXString("Weapon upgrades received"), (EXW::TextAlign)7, help, 0)->SetMathmos((FEObjColour::Flags)3, col, 1);
    sprintf(buf, "%d/%d", gotWeapons, nWeapons);
    AddScrollGridText(pGrid, row++, 1, 0xfdffffff, EXString(buf), (EXW::TextAlign)7, help, 0)->SetMathmos((FEObjColour::Flags)3, col, 1);
    if (pStats->m_Unk0[0]) {
        u8 c = 0x58;
        if (nAttribs) c = 0x7f;
        col = StatGrey(c);
        if (gotAttribs && gotAttribs == nAttribs) col = ex::RGBA(0x65, 0x65, 0x65, 0x80);
    }
    AddScrollGridText(pGrid, lbl++, 0, id++, EXString("Attribute upgrades received"), (EXW::TextAlign)7, help, 0)->SetMathmos((FEObjColour::Flags)3, col, 1);
    sprintf(buf, "%d/%d", gotAttribs, nAttribs);
    AddScrollGridText(pGrid, row++, 1, 0xfdffffff, EXString(buf), (EXW::TextAlign)7, help, 0)->SetMathmos((FEObjColour::Flags)3, col, 1);
    if (pStats->m_Unk0[0]) {
        u8 c = 0x58;
        if (nCostumes) c = 0x7f;
        col = StatGrey(c);
        if (gotCostumes && gotCostumes == nCostumes) col = ex::RGBA(0x65, 0x65, 0x65, 0x80);
    }
    AddScrollGridText(pGrid, lbl++, 0, id++, EXString("Alternate costumes received"), (EXW::TextAlign)7, help, 0)->SetMathmos((FEObjColour::Flags)3, col, 1);
    sprintf(buf, "%d/%d", gotCostumes, nCostumes);
    AddScrollGridText(pGrid, row++, 1, 0xfdffffff, EXString(buf), (EXW::TextAlign)7, help, 0)->SetMathmos((FEObjColour::Flags)3, col, 1);

#define STAT_LABEL(t) \
    if (!b) AddScrollGridText(pGrid, lbl++, 0, id++, EXString(t), (EXW::TextAlign)7, help, 0)
#define STAT_VALUE(s) AddScrollGridText(pGrid, row++, 1, 0xfdffffff, EXString(s), (EXW::TextAlign)7, help, 0)
#define STAT_INT(t, v)          \
    STAT_LABEL(t);              \
    sprintf(buf, "%d", v);      \
    STAT_VALUE(buf)
#define STAT_BLANK() \
    STAT_LABEL("");  \
    STAT_VALUE("")

    STAT_BLANK();
    STAT_LABEL("Honorable kills");
    sprintf(buf, "%d", pStats->m_Kills);
    FEObject *pHonour = STAT_VALUE(buf);
    STAT_INT("Dishonorable kills", pStats->m_KillsDishonour);
    STAT_BLANK();
    STAT_INT("Executions", pStats->m_Unk10[0]);
    STAT_INT("Backs broken", pStats->m_Unk10[1]);
    STAT_INT("Skulls ripped", pStats->m_Unk10[2]);
    STAT_INT("Throats sliced", pStats->m_Unk10[3]);
    STAT_INT("Necks snapped", pStats->m_Unk10[4]);
    STAT_INT("Heads crushed", pStats->m_Unk10[5]);
    STAT_BLANK();
    STAT_INT("Grapple kills", pStats->m_Unk10[6]);
    STAT_LABEL("Furthest distance opponent thrown");
    sprintf(buf, "%.2fm", ex::fd(pStats->LongestThrow()));
    STAT_VALUE(buf);
    STAT_LABEL("Furthest distance opponent kicked");
    sprintf(buf, "%.2fm", ex::fd(pStats->LongestKick()));
    STAT_VALUE(buf);
    STAT_INT("Ground kills", pStats->m_Unk10[9]);
    STAT_INT("Railing kills", pStats->m_Unk10[10]);
    STAT_INT("Wall kills", pStats->m_Unk10[11]);
    STAT_BLANK();
    STAT_INT("Opponents impaled", pStats->m_Unk10[12]);
    STAT_INT("Juggle kills", pStats->m_Unk10[13]);
    STAT_BLANK();
    STAT_LABEL("Favorite weapon");
    if (pStats->m_Unk48 >= 0) {
        FEWeapon w = FEHud::Instance(1)->GetWeaponPreset(pStats->m_Unk48, -1, 0);
        unsigned int tex = w.m_Icon;
        int h = pHonour->LocalRect().h;
        pGrid->ObjectAdd(row++, 1, new FEObject((FEObjType::Flags)1, 0x1000025, tex, 0xfdffffff, FE_Blank, FE_Blank,
                                                (EXW::TextAlign)7, 0, 0, h, h, (EXW::TextAlign)7))
            ->StateAdd(0x102)
            ->SetBlendMode(2);
    } else row++;
    STAT_INT("Headshots", pStats->m_Unk4c[0]);
    STAT_INT("Decapitations", pStats->m_Unk4c[1]);
    STAT_INT("Arms severed", pStats->m_Unk4c[2]);
    STAT_INT("Bodies split", pStats->m_Unk4c[3]);
    STAT_INT("Opponents burnt to death", pStats->m_Unk4c[4]);
    STAT_BLANK();
    STAT_INT("Healing Charges used", pStats->m_Unk4c[5]);
    STAT_LABEL("Recharge points used")->StateSub(0x4000);
    sprintf(buf, "%d", pStats->m_Recharges);
    STAT_VALUE(buf);
#undef STAT_LABEL
#undef STAT_VALUE
#undef STAT_INT
#undef STAT_BLANK
}

void FEMainMenu::SetState(unsigned int state)
{
    m_PrevState = m_pManager->m_PageState;
    if (m_bUpdateOnChange) UpdateState(m_PrevState, (FEObjEvent::Flags)0x22);
    m_pManager->SetPageState(state);
    m_GoState = 0;
    m_bGoing = 0;
    if (m_bUpdateOnChange) UpdateState(state, (FEObjEvent::Flags)0x20);
    m_bAutoSave = MemCardAutoSaveEnabled();
}

signed char FEMainMenu::GoStateUpdate()
{
    if (!m_bGoing) return 1;
    if (!m_GoState) return 1;
    signed char done = 0;
    if (m_bFadeOut) {
        if (UpdateFade(0)) {
            m_bFadeOut = 0;
            m_FadeTime = 0.0f;
            UpdateState(m_GoState, (FEObjEvent::Flags)0x23);
        }
    } else {
        done = UpdateFade(1);
        if (done) SetState(m_GoState);
    }
    return done;
}

void FEMainMenu::SetPlayerSkin(short skin)
{
    if (m_PlayerSkin != skin) {
        m_PlayerSkin = skin;
        SetPredSkinUpdateState(0);
        ms_bPredSkinLoaded = 0;
    }
}

void FEMainMenu::UpdateCameraTransition() {}

void FEMainMenu::AddCamera(unsigned int hash, EXItemAnimator_Camera *pCam)
{
    unsigned int i;
    switch (hash) {
    case 0x15000017: i = 0; break;
    case 0x15000018: i = 1; break;
    case 0x15000019: i = 2; break;
    default: return;
    }
    m_Cameras[i].m_Mtx = pCam->m_mtx;
}

FEMainMenu::FEMainMenu()
{
    ms_pInstance = this;
    InitLayered(7, 0, 0, 0, 0, (EXW::TextAlign)4);
    m_pManager = FEManager::Instance(1);
    m_pLayout = 0;
    m_pUpdateObject = this;
    m_Unk274 = 0;
    m_bNewSubstate = 0;
    m_Unk1fc = 0;
    m_bAutoSave = 0;
    m_PreDlgFocus = 0;
    m_PlayerSkin = 0;
    m_PredSkinHash = 0;
    m_Unk82c = 3;
    m_SubstateArg[1] = 0;
    m_SubstateArg[0] = 0;
    m_pDimmedLayout = 0;
    m_pSlotsInfo = 0;
    SetPredSkinUpdateState(0);
    m_Unk81c = 0xe;
    m_Unk844 = 0;
    m_PrevState = 0;
    m_Unk84c = 0;
    m_InfoHelpText = 0;
    m_Unk28c = -1;
    m_Unk290 = -1;
    m_Unk818 = -1;
    m_bUpdateOnChange = 0;
    m_b1 = 0;
    m_b4 = 1;
    m_bDialogOn = 0;
    m_b6 = 0;
    m_b7 = 0;
    m_bIgnoreNextPageSelect = 0;
    m_b9 = 0;
    m_b10 = 0;
    for (int i = 0; i < 24; i++) m_UpdateListeners[i] = 0;
    SetDoneStateAction(0, 0);
    SetState(0);
    SetMemCardFlow(0);
}

__declspec(noinline) signed char FEMainMenu::GoState(unsigned int state)
{
    EXPointf range;
    if (m_GoState) return 0;
    unsigned int cur = m_pManager->m_PageState;
    m_GoState = state;
    UpdateState(cur, (FEObjEvent::Flags)0x21);
    m_bGoing = 1;
    m_bFadeOut = 1;
    SetInfoObjText(0, 0);
    UpdateState(state, (FEObjEvent::Flags)0x1f);
    range.x = 0.0f;
    range.y = 1.0f;
    SetFadeRange(range, 1);
    SetFadeRange(range, 0);
    return GoStateUpdate();
}

// NEAR: retail builds the EXRect(0, 0, 0, 0) argument with immediate stores; VC 7.1 here uses zeroed
// registers (and one more saved register).
signed char FEMainMenu::UpdateStateLoadGame(FEObjEvent::Flags ev)
{
    SetSaveLoadLayouts(m_ListenerIdx);
    switch (ev) {
    case 0x23:
        SetTitleAndBackgr("Load Game", EXRect(0, 0, 0, 0));
        ms_pInstance->m_pSlotsInfo->SetText(Text_Get(0x51000835), 0, 0);
        return 0;
    case 0x20:
        SetSubstate(3);
        return 0;
    case 0x1f:
        s_RememberAutoSave = MemCardAutoSaveEnabled();
        m_b4 = 1;
        SetChoosingSlots(1, 1);
        MemCardEnableAutoSave(0);
        MemCardSetSlot(0);
        SetSaveLoadLayouts(m_ListenerIdx);
        s_LoadLayout->StateSub(0x40);
        return 0;
    }
    return UpdateStateNewGame(ev);
}

// NEAR: retail builds the EXRect(0, 0, 0, 0) argument with immediate stores; VC 7.1 here uses zeroed
// registers (and one more saved register).
signed char FEMainMenu::UpdateStateAutosave(FEObjEvent::Flags ev)
{
    SetSaveLoadLayouts(m_ListenerIdx);
    switch (ev) {
    case 0x15:
        Layout(m_ListenerIdx)->StateSub(0x48);
        break;
    case 0x1f:
        s_RememberAutoSave = MemCardAutoSaveEnabled();
        m_b4 = 1;
        SetChoosingSlots(1, 1);
        return 0;
    case 0x20: {
        signed char b = MemCardAutoSaveEnabled();
        MemCard_SaveInit();
        MemCardEnableAutoSave(b);
        if (MemCardAutoSaveEnabled()) {
            SetSubstate(0x14);
            return 0;
        }
        SetSubstate(0x17);
        return 0;
    }
    case 0x23:
        SetTitleAndBackgr(0, EXRect(0, 0, 0, 0));
        return 0;
    }
    return UpdateStateNewGame(ev);
}

// NEAR: retail builds the EXRect(0, 0, 0, 0) argument with immediate stores; VC 7.1 here uses zeroed
// registers (and one more saved register).
signed char FEMainMenu::UpdateStateSaveGame(FEObjEvent::Flags ev)
{
    SetSaveLoadLayouts(m_ListenerIdx);
    switch (ev) {
    case 0x23:
        SetTitleAndBackgr("Save Game", EXRect(0, 0, 0, 0));
        ms_pInstance->m_pSlotsInfo->SetText(Text_Get(0x51000835), 0, 0);
        return 0;
    case 0x20:
        SetSubstate(2);
        return 0;
    case 0x1f:
        s_RememberAutoSave = MemCardAutoSaveEnabled();
        s_RememberSlot = MemCardSlot();
        GameData::Instance(1)->GetBigSave()->SetStatusAndReset(0);
        m_b4 = 1;
        SetChoosingSlots(1, 1);
        return 0;
    }
    return UpdateStateNewGame(ev);
}

signed char FEMainMenu::UpdateStateHuntingGear(FEObjEvent::Flags ev) { return UpdateStateViewTrophies(ev); }

void FEMainMenu::Init()
{
    m_Data = 0x756e654d6e69614dULL;  // "MainMenu"
    SetEventCallback(MainMenu_Callback, 1);
}

// NEAR: retail also stores the zeroed layout pointer to a stack slot (push ecx / mov [esp+4], eax).
void FEMainMenu::RefreshPostLoadContent()
{
    FELayout *p = 0;
    m_ListenerIdx = 6;
    if (m_pLayout) p = (FELayout *)m_pLayout->Object(6);
    m_pCurLayout = p;
    UpdateStateStoryLevelSelect((FEObjEvent::Flags)0x24);
    m_Unk1fc &= ~(1 << m_ListenerIdx);
    UpdateState(0x15, (FEObjEvent::Flags)0x24);
    UpdateState(7, (FEObjEvent::Flags)0x24);
    UpdateState(8, (FEObjEvent::Flags)0x24);
    UpdateState(9, (FEObjEvent::Flags)0x24);
    UpdateState(10, (FEObjEvent::Flags)0x24);
    UpdateState(11, (FEObjEvent::Flags)0x24);
    UpdateState(0x12, (FEObjEvent::Flags)0x24);
    UpdateState(0x16, (FEObjEvent::Flags)0x24);
    UpdateState(0xe, (FEObjEvent::Flags)0x24);
    UpdateState(0x10, (FEObjEvent::Flags)0x24);
    UpdateState(0xf, (FEObjEvent::Flags)0x24);
    UpdateState(0x11, (FEObjEvent::Flags)0x24);
}

EXFixedArray<FEMarker, 16> FEMainMenu::ms_FEMarkers;

extern XItemHandler_CameraScript *gLastCamScript;  // TODO stand-in (CameraScript.obj)
extern XGameWnd *gpGameWnd[4];                     // TODO stand-in (GameWnd.obj)

void FEMainMenu::CameraVectors(unsigned int cam, EXVector &pos, EXVector &target)
{
    FECamera &c = m_Cameras[cam];
    XGetCamera(0)->SetCutsceneFOV(0.47108996f);
    pos = c.m_Mtx.GetTranslation();
    pos.w = 0.0f;
    target = pos + EXVector(c.m_Mtx.EXMaths::mat_44::operator*(EXVector(0.0f, 0.0f, 1.0f, 0.0f)));
}

void FEMainMenu::SetCamera(unsigned int cam, signed char finish)
{
    if (finish) {
        gLastCamScript->FinishScript(0, 0);
        theApp.m_bCutscene = 1;
    }
    EXVector pos, target;
    CameraVectors(cam, pos, target);
    XGetCamera(0)->SetCameraLookAt(target);
    XGetCamera(0)->SetCameraPosition(pos);
    XGetCamera(0)->m_Mode = 4;
    XGetCamera(0)->DoUpdate();
    XItemHandler_Camera *pCam = XGetCamera(0);
    EXRect &r = gpGameWnd[0]->m_PrimRect;
    pCam->m_pCamera->SetVFOV(EXRectf((float)r.x, (float)r.y, (float)r.w, (float)r.h), 0.47108996f);
}

void FEMainMenu::InitCameraTransition(EXVector to, float duration)
{
    gLastCamScript->SetDisableUpdate();
    m_b9 = 1;
    m_CamTransFrom = XGetCamera(0)->GetCameraLookAt();
    m_CamTransTo = to;
    m_CamTransDuration = duration;
    m_CamTransTime = 0.0f;
    XGetCamera(0)->GetCamera()->UpdateDisplay(0);
    m_CamTransDisp = EXBaseDisplay::WorldToDisp(XGetCamera(0)->GetCameraLookAt());
}


void SetSlotsHighlights()
{
    if (!s_ChoosingSlots) return;
    MemCardSavesFound();
    FEListener *pL = FEMainMenu::Instance()->UpdateListener();
    FEObject *pF = 0;
    if (pL) pF = pL->Focus();
    unsigned int d = 0xfdffffff;
    if (pF) d = pF->Data32();
    unsigned int focus = d - 100;
    for (unsigned int i = 0; i < 6; i++) {
        FEObject *pSlot = s_LoadLayout->Object(i);
        FEObject *pText = s_LoadLayout->Object(i + 6)->Object(0, 0);
        ex::RGBA col(0x58, 0x58, 0x58, 0x80);
        ex::RGBA textCol;
        if (i == focus) {
            col = ex::RGBA(0x78, 0x23, 0x28, 0x80);
            pSlot->SetTexture(XApp::Frontend3DFile(), 0x600030c);
            pText = pText->Object(1, 0);
            textCol = ex::RGBA(0x65, 0x65, 0x65, 0x80);
        } else {
            pSlot->SetTexture(XApp::Frontend3DFile(), 0x600030d);
            pText = pText->Object(1, 0);
            textCol = ex::RGBA(0x58, 0x58, 0x58, 0x80);
        }
        pText->SetMathmos((FEObjColour::Flags)3, textCol, 1);
        pSlot->SetMathmos((FEObjColour::Flags)3, col, 1);
    }
}

static signed char s_bSaveSlotChosen;  // set while saving once a slot is picked (NewGame); empty slots then read "Damaged"

// NEAR: one operand order differs (`[edi+eax]` vs `[eax+edi]` on the slot-found byte load).
void SetSlotsText()
{
    unsigned int state = FEMainMenu::Instance()->m_ListenerIdx;
    SetSaveLoadLayouts(state);
    if (state == 0x12) return;
    char timeStr[0x100];
    char buf1[0x100];
    char buf2[0x100];
    for (unsigned int i = 0; i < 6; i++) {
        FEObject *pText = s_LoadLayout->Object(i + 6)->Object(0, 0);
        GameValues *pV = &GameData::Instance(1)->m_BigSave.m_Saves[i];
        if ((char)GameData::Instance(1)->m_BigSave.m_SaveFound[i]) {
            unsigned int chapters = pV->CountCompletedChapters();
            unsigned int t = pV->m_Stats.m_Unk0[0];
            timeStr[0] = 0;
            unsigned int secs = t % 60;
            t /= 60;
            unsigned int mins = t % 60;
            t /= 60;
            unsigned int hours = t % 60;
            sprintf(timeStr + strlen(timeStr), "%d:", hours);
            sprintf(timeStr + strlen(timeStr), "%02d:%02d", mins, secs);
            if (chapters < FELevelManager::Instance(1)->StoryLevelCount()) {
                EXString name = gText.Get(FELevelManager::Instance(1)->TextFromIndex(chapters));
                sprintf(buf1, name);
            } else {
                sprintf(buf1, "Game Complete");
            }
            strcat(buf1, "\n");
            sprintf(buf2, "<v>Time played: %s\n", timeStr);
            strcat(buf1, buf2);
            unsigned int hash = pV->m_Unk1c[0];
            switch (hash) {
            case 0: hash = 0x510001bb; break;
            case 1: hash = 0x510001bc; break;
            case 2: hash = 0x510001bd; break;
            }
            {
                EXString diff = gText.Get(hash);
                sprintf(buf2, "Chapters completed: %d\nDifficulty: %s", chapters, (const char *)diff);
            }
            strcat(buf1, buf2);
            pText->Object(1, 0)->SetText(EXString(buf1), 0, 0);
            pText->Object(1, 0)->StateAdd(0x20000);
        } else {
            const char *pStr = "Empty";
            unsigned int st = FEMainMenu::Instance()->m_ListenerIdx;
            if ((st == 2 || st == 12) && s_bSaveSlotChosen) pStr = "Damaged";
            pText->Object(1, 0)->SetText(EXString(pStr), 0, 0);
            pText->Object(1, 0)->StateSub(0x20000);
        }
    }
    SetSlotsHighlights();
}

static float s_LazerTimer;

void UpdateLazer()
{
    s_LazerTimer += EXBaseApp::FrameLength();
    float a = 1.0f;
    switch (LazerState) {
    case 0x65:
        a = 0.0f;
        break;
    case 0x66:
        s_LazerTimer = 0.0f;
        LazerState = 0x67;
    case 0x67:
        a = min(s_LazerTimer * 3.0f, 1.0f);
        break;
    case 0x68:
        s_LazerTimer = 0.0f;
        LazerState = 0x69;
    case 0x69:
        a = max(1.0f - s_LazerTimer * 4.0f, 0.0f);
        break;
    }
    pLazerItem->m_renderInfo.EnableAlphaMult(a != 1.0f);
    pLazerItem2->m_renderInfo.EnableAlphaMult(a != 1.0f);
    pLazerItem->m_renderInfo.m_rgbaMult.set(1.0f, 1.0f, 1.0f, a);
    pLazerItem2->m_renderInfo.m_rgbaMult.set(1.0f, 1.0f, 1.0f, a);
}

void FEMainMenu::HideSlotsLayoutUpdate()
{
    if (m_bGoing) {
        if (m_b6) m_b6 = 0;
        return;
    }
    if (m_ListenerIdx != 3 && m_ListenerIdx != 12 && m_ListenerIdx != 2) {
        m_b10 = 0;
        if (s_LoadLayout) s_LoadLayout->StateSub(0x40);
        return;
    }
    if (!s_LoadLayout) return;
    if (s_ChoosingSlots && GameData::Instance(1)->GetBigSave()->Status() != 1) SetSlotsText();
    signed char hide;
    if (MemCardDlgIsOn() || fDialogOn() || GameDialogWnd::NumInstances() || m_Unk84c >= 0)
        hide = 1;
    else
        hide = 0;
    if (m_pManager->m_PageSubstate != 0xc || m_pManager->m_PageSubstateTimer <= 0.15f) hide = 1;
    if (!m_b10) {
        if ((s_LoadLayout->State() & 0x40) && hide) {
            s_LoadLayout->StateSub(0x40);
            m_pSlotsBackground->StateSub(0x40);
            m_b10 = 1;
        }
    } else if (!hide) {
        s_LoadLayout->StateAdd(0x40);
        m_pSlotsBackground->StateAdd(0x40);
        m_b10 = 0;
    } else {
        s_LoadLayout->StateSub(0x40);
        m_pSlotsBackground->StateSub(0x40);
    }
}

void FEMainMenu::SetTitleAndBackgr(char *pTitle, EXRect r)
{
    if (pTitle && *pTitle && m_ListenerIdx && m_ListenerIdx != 0x17) {
        EXString s(pTitle);
        s += "  ";
        FEObject *p = m_pTitle->FindChildObject(0x5f5f5f5f74786554);
        p->SetText(s, 0, 0);
        p->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0x5f, 0x5f, 0x5f, 0x80), 1);
        m_pTitle->StateAdd(0x40040);
    } else {
        m_pTitle->StateSub(0x40);
    }
    m_pBackground->SetRect(r);
    m_pBackground->StateAdd(0x40);
    m_pSlotsInfo->SetText(Text_Get(0x51000473), 0, 0);
}

void SetChoosingSlots(signed char b, signed char force)
{
    FEMainMenu *pMM = FEMainMenu::Instance();
    unsigned int page = pMM->m_ListenerIdx;
    SetSaveLoadLayouts(page);
    FEListener *pL = pMM->m_UpdateListeners[page];
    if (page == 0x12) return;
    if (b) {
        SetSlotsText();
        pL->SetFocus(0, 0);
        unsigned int slot = MemCardSlot() + 100;
        if (slot != 0xffffffff)
            pL->SetFocusByData(slot, 0);
        else
            pL->SetFocus(0, 0);
        SetSlotsHighlights();
    }
    if (b == s_ChoosingSlots && !force && b) {
        if (!(s_SaveLayout->State() & 0x40) && (s_LoadLayout->State() & 0x40)) return;
        if ((s_SaveLayout->State() & 0x40) && !(s_LoadLayout->State() & 0x40)) return;
    }
    s_ChoosingSlots = b;
    if (!b) pL->SetFocus(0, 0);
    s_SaveLayout->m_Flags = (s_SaveLayout->m_Flags & ~0x40) | (b ? 0 : 0x40);
    unsigned int i;
    for (i = 0; i < 3; i++) {
        FEObject *p = s_SaveLayout->Object(i, 0);
        p->m_Flags = (p->m_Flags & ~8) | (b ? 0 : 8);
    }
    s_LoadLayout->m_Flags = (s_LoadLayout->m_Flags & ~0x40) | (b ? 0x40 : 0);
    for (i = 0; i < 6; i++) {
        FEObject *p = s_LoadLayout->Object(i + 6)->Object(0, 0)->Object(0, 0);
        p->m_Flags = (p->m_Flags & ~8) | (b ? 8 : 0);
    }
    if (!b) pL->SetFocusByData(GameData::Instance(1)->Values()->m_Unk1c[0], 0);
    pMM = FEMainMenu::Instance();
    if ((!pMM->m_bGoing || !pMM->m_bFadeOut) && page == pMM->m_ListenerIdx) {
        if (b) {
            FEMainMenu::Instance()->SetTitleAndBackgr(page == 3 ? "Load Game" : "Save Game", EXRect(0, 0, 0, 0));
            FEMainMenu::Instance()->m_pSlotsInfo->SetText(Text_Get(0x51000835), 0, 0);
            FEMainMenu::Instance()->SetInfoObjText(0, 0);
        } else {
            FEMainMenu::Instance()->SetTitleAndBackgr("Difficulty Select", s_SaveLayout->Rect());
        }
    }
    FEMainMenu::Instance()->HideSlotsLayoutUpdate();
}

signed char FEMainMenu::UpdateStateDone(FEObjEvent::Flags ev)
{
    switch (ev) {
    case 0x23:
        SetTitleAndBackgr(0, EXRect(0, 0, 0, 0));
        return 0;
    case 0x20:
        switch (m_DoneState) {
        case 0:
            break;
        case 2: {
            int idx = FELevelManager::Instance(1)->IndexFromLevel(GameData::Instance(1)->m_Values.m_Unkc);
            unsigned int level = FELevelManager::Instance(1)->LevelFromIndex(idx);
            Remember_RewardsAwarded = GameData::Instance(1)->m_Values.m_Rewards;
            Remember_WeaponUpgradedReceived = GameData::Instance(1)->m_Values.m_WeaponUpgrades;
            Remember_RitualsUnlocked = GameData::Instance(1)->m_Values.m_Rituals;
            if (level == FELevelManager::Instance(1)->LevelFromIndex(1)) {
                if (GameData::Instance(1)->m_Values.m_Unk1c[2])
                    theApp.PlayMovie("cpcatS", level);
                else
                    theApp.PlayMovie("cpycat", level);
            } else {
                theApp.GameStatePush((GameState::Flags)0x22, level);
            }
            GameData::Instance(1)->SettingsValidateGlobal();
            XGetSoundManager()->SetSfxVolume(GameData::Instance(1)->m_Values.m_MovieVolume);
            break;
        }
        case 3: {
            theApp.GameStatePush((GameState::Flags)0x22, m_DoneAction);
            GameData::Instance(1)->SettingsValidateGlobal();
            XGetSoundManager()->SetSfxVolume(GameData::Instance(1)->m_Values.m_MovieVolume);
            return 0;
        }
        default:
            m_DoneState = 0;
            return 0;
        }
    }
    return 0;
}

signed char __cdecl MainMenu_Callback(FEObject *pObj, FEObjEvent::Flags ev)
{
    FEMainMenu *pMM = FEMainMenu::Instance();
    if (!pMM) return 1;
    if (FEManager::Instance(1)->AttractMode()) {
        XGetSoundManager()->SetSfxVolume(0);
        return 1;
    }
    int v = GameData::Instance(1)->m_Values.m_MovieVolume;
    XGetSoundManager()->SetSfxVolume(v);
    int m = GameData::Instance(1)->m_Values.m_MusicVolume;
    XGetSoundManager()->SetMusicVolume(m);
    if (pMM->m_pManager->m_PageState == 2 && pMM->m_pManager->m_PageSubstate == 1 && !pMM->m_ReturnSubstate)
        pMM->SetPreDlgFocus(0xfdffffff);
    if (MemCardDlgIsOn() || pMM->fDialogOn() || pMM->IgnoreNextPageSelect() || GameDialogWnd::NumInstances()) {
        if (ev == 0x15) {
            unsigned int h = MemCardDlgMessageHash();
            if (h == 0x510001e3 || (h && h == pMM->InfoHelpText())) {
                switch (MemCardDlgResult()) {
                case 1:
                    if (h == 0x510001e3) pMM->GoState(1);
                case 2:
                    MemCardDlgHide(0);
                    break;
                }
            }
        } else if (ev >= 0x16) {
            if (ev > 0x17) {
                if (ev == 0x28) return 1;
            } else {
                return 0;
            }
        }
    }
    pMM->SetUpdateObject(pObj);
    if (ev == 0xd || ev == 0xe || ev == 0xb || ev == 0xc) pMM->SetUpdateObject(pMM);
    signed char handle = 0;
    switch (ev) {
    case 9: case 10: case 17: case 25: case 40:
        handle = 1;
        break;
    }
    if (pMM->UpdateObject() != pMM && !handle) return 1;
    return pMM->HandleFrontendEvent(ev);
}

void FEMainMenu::UpdateCameraScreenPos()
{
    EXVector pos = XGetCamera(0)->GetCameraPosition();
    EXVector look = XGetCamera(0)->GetCameraLookAt();
    SetCamera(1, 0);
    XGetCamera(0)->GetCamera()->UpdateDisplay(0);
    EXBaseDisplay *pDisp = EXBaseDisplay::GetDisplay();
    EXVector2 centre(pDisp->Width() * 0.5f, pDisp->Height() * 0.5f);
    m_CameraDispOffset = EXBaseDisplay::WorldToDisp(look) - centre;
    XGetCamera(0)->SetCameraPosition(pos);
    XGetCamera(0)->SetCameraLookAt(look);
    XGetCamera(0)->GetCamera()->UpdateDisplay(0);
    for (unsigned int i = 0; i < 24; i++) {
        FEObject *p = Layout(i);
        if (!p) continue;
        EXRect r = p->LocalRect();
        r.x = (int)(m_LayoutPos[i].x - m_CameraDispOffset.x);
        r.y = (int)(m_LayoutPos[i].y - m_CameraDispOffset.y);
        p->SetRect(r);
    }
}

void FEMainMenu::NewFocus(FEObject *pFocus, unsigned int state)
{
    EXString help;
    if (pFocus && (pFocus->m_Flags & 8)) {
        help = pFocus->m_HelpText;
        if (help.IsEmpty()) help = Text_Get(0x51000476);
    }
    m_pHelpText->SetText(help, 0, 0);
    if (m_b4 || !m_ListenerIdx || m_ListenerIdx == 0x17) {
        m_b4 = 0;
        return;
    }
    if (pFocus) {
        switch (state) {
        case 1: case 4: case 5: case 13:
            break;
        case 2: case 3: case 12:
            if (!s_ChoosingSlots) break;
        default:
            pFocus = 0;
            break;
        }
    }
    if (m_bGoing || !pFocus || pFocus->m_Text.IsEmpty()) {
        m_pArrowL->StateSub(0x40);
        m_pArrowR->StateSub(0x40);
        return;
    }
    if (!m_b1) return;
    m_pArrowL->StateAdd(0x40);
    m_pArrowR->StateAdd(0x40);
    EXRect r = pFocus->TextRect();
    int x = r.x, w = r.w;
    int y = r.h / 2 + r.y;
    m_pArrowL->SetOrigin(EXPoint(x - 20, y), (EXW::TextAlign)4);
    m_pArrowL->Script(0)->Stop();
    m_pArrowL->Script(0)->Play(0, 0);
    m_pArrowR->SetOrigin(EXPoint(w / 2 + x - 8, y + 8), (EXW::TextAlign)4);
    m_pArrowR->Script(0)->Stop();
    m_pArrowR->Script(0)->Play(0, 0);
}

void FEMainMenu::UpdatePredSkin()
{
    XItemHandler_Player *pl = XItemHandler_Player::Instance();
    if (!pl) return;
    m_PredSkinUpdateTime += EXBaseApp::FrameLength();
    switch (m_PredSkinUpdateState) {
    case 0:
        if (m_PlayerSkin == pl->GetSelectedSkin()) return;
        pl->SparkSkin(100.0f);
        m_PredSkinUpdateState = 10;
        break;
    case 10:
        if (!(m_PredSkinUpdateTime >= 0.5f)) return;
        EXBaseSoundManager::GetSoundManager()->StartSound(0x1af001c4);
        m_PredSkinUpdateState = 11;
        break;
    case 11:
        if (!(m_PredSkinUpdateTime >= 0.5f)) return;
        pl->SetCloakDirect(1);
        m_PredSkinUpdateState = 20;
        break;
    case 20:
        if (!(m_PredSkinUpdateTime >= 0.3f)) return;
        m_PredSkinHash = pl->GetCurrentSkinHash();
        pl->SetSelectedSkin(m_PlayerSkin);
        memcpy(XItemHandler_Player::Instance()->GetWeaponUpgradeArray(), XGetGameValues()->m_Unk38, 0x20);
        pl->RefreshAllWeapons();
        m_PredSkinUpdateState = 21;
        m_PredSkinUpdateTime = 0.0f;
        return;
    case 21:
        if (!(m_PredSkinUpdateTime >= 0.3f)) return;
        pl->HandleDamageSkins();
        if (!ms_bPredSkinLoaded) return;
        m_PredSkinUpdateState = 30;
        break;
    case 30:
        if (!(m_PredSkinUpdateTime >= 0.5f)) return;
        EXBaseSoundManager::GetSoundManager()->StartSound(0x1af001c3);
        m_PredSkinUpdateState = 31;
        break;
    case 31:
        if (!(m_PredSkinUpdateTime >= 0.5f)) return;
        pl->SetCloakDirect(0);
        m_PredSkinUpdateState = 40;
        break;
    case 40:
        if (!(m_PredSkinUpdateTime >= 1.0f)) return;
        m_PredSkinUpdateState = 50;
        break;
    case 50:
        m_PredSkinUpdateTime = 0.0f;
        m_PredSkinUpdateState = 0;
        pl->SetSparkTimer(0.01f);
        return;
    default:
        return;
    }
    m_PredSkinUpdateTime = 0.0f;
}

void FEMainMenu::InitRitualTimesLayout(FELayoutScrollGrid *pGrid, unsigned int level)
{
    RitualTimesView *rt = (RitualTimesView *)GameData::Instance(1)->GetRitualTimes();
    unsigned int bit = FELevelManager::Instance(1)->RitualBitIndexFromLevelHash(level);
    EXString help(" ");
    for (unsigned int i = 0; i < 8; i++) {
        if (i < 3) {
            EXFixedArray<unsigned int, 3> tex;
            tex[0] = 0x6000296;
            tex[1] = 0x6000297;
            tex[2] = 0x6000298;
            FEObject *p = pGrid->ObjectAdd(i, 0, new FEObject(FEObjType::OT_SPRITE, 0x1000025, tex[i], 0xfdffffff, FE_Blank,
                                                               FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
            p->StateAdd(0x102);
            p->SetBlendMode(2);
            p->SetSize(EXPoint(p->m_Rect.h, p->m_Rect.h));
        }
        AddScrollGridText(pGrid, i, 1, 0, EXString(rt->m_Names[bit][i]), (EXW::TextAlign)2, help, 0)->StateSub(8);
        char buf[32];
        float t = rt->m_Times[bit][i];
        buf[0] = 0;
        unsigned int secs = (unsigned int)t;
        int tenths = (int)((t - (float)secs) * 10.0f);
        unsigned int sec = secs % 60;
        unsigned int min = secs / 60 % 60;
        sprintf(buf, "%02d:%02d", min, sec);
        sprintf(buf + strlen(buf), ".%d", tenths);
        AddScrollGridText(pGrid, i, 2, 0, EXString(buf), (EXW::TextAlign)2, help, 0)->StateSub(8);
    }
}

signed char FEMainMenu::UpdateFade(signed char out)
{
    EXPointf &r = out ? m_FadeOut : m_FadeIn;
    float a = r.x, b = r.y;
    signed char rev = 0;
    if (a > b) {
        float t = a;
        a = b;
        b = t;
        rev = 1;
    }
    if (a >= 0.0f) {
        float f = 0.0f;
        if (m_FadeTime >= a) {
            if (m_FadeTime >= b)
                f = 1.0f;
            else
                f = 1.0f / ((b - a) / (m_FadeTime - a));
        }
        if (rev) f = 1.0f - f;
        f = min(f, 1.0f);
        f = max(f, 0.0f);
        unsigned int state = out ? m_GoState : m_pManager->m_PageState;
        FEObject *p;
        if (m_pLayout && (p = m_pLayout->Object(state)) != 0) {
            if (out) {
                if (m_FadeTime != 0.0f) {
                    if (m_GoState != 3 && m_GoState != 12 && m_GoState != 18) {
                        p->StateAdd(0x40);
                        if (m_GoState != 11 && m_GoState != 9) m_pSlotsBackground->StateAdd(0x40);
                        m_b6 = 1;
                    }
                } else {
                    p->StateSub(0x40);
                    m_pSlotsBackground->StateSub(0x40);
                    m_b6 = 0;
                }
            }
            FEMathmos *m = p->Mathmos((FEObjColour::Flags)2);
            m->GotoFraction(f);
            m->Update();
            p->SetMathmos((FEObjColour::Flags)2, *m, 1);
            m_pSlotsBackground->SetMathmos((FEObjColour::Flags)2, *m, 1);
            m_pTitle->SetMathmos((FEObjColour::Flags)2, *m, 1);
            m_pBackground->SetMathmos((FEObjColour::Flags)2, *m, 1);
            if ((!out && (m_GoState == 11 || m_GoState == 9 || m_GoState == 20 || m_GoState == 19 || m_GoState == 14)) ||
                (out && (m_pManager->m_PageState == 11 || m_pManager->m_PageState == 9 || m_pManager->m_PageState == 20 ||
                         m_pManager->m_PageState == 19 || m_pManager->m_PageState == 14)))
                m_pFadeExtra->SetMathmos((FEObjColour::Flags)2, *m, 1);
        }
        if (f == 1.0f) {
            if (out)
                m_FadeOut.x = -1.0f;
            else
                m_FadeIn.x = -1.0f;
            return 1;
        }
    }
    m_FadeTime += XApp::FrameLength() * 6.0f;
    return 0;
}

void FEMainMenu::SetFadeRange(EXPointf &r, signed char out)
{
    if (r.x == -1.0f) return;
    m_FadeTime = 0.0f;
    unsigned int state = out ? m_GoState : m_pManager->m_PageState;
    FEObject *p;
    if (!m_pLayout)
        p = 0;
    else
        p = m_pLayout->Object(state);
    if (out) {
        if (p)
            p->SetMathmos((FEObjColour::Flags)2, FEMathmos(ex::RGBA(0, 0, 0, 0), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 599940.0f,
                                                             (FEWaveform::Flags)6, 1), 1);
        if (m_GoState != 3 && m_GoState != 2 && m_GoState != 12)
            m_pSlotsBackground->SetMathmos((FEObjColour::Flags)2, FEMathmos(ex::RGBA(0, 0, 0, 0), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80),
                                                                            599940.0f, (FEWaveform::Flags)6, 1), 1);
        m_pTitle->SetMathmos((FEObjColour::Flags)2, FEMathmos(ex::RGBA(0, 0, 0, 0), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 599940.0f,
                                                              (FEWaveform::Flags)6, 1), 1);
        m_pBackground->SetMathmos((FEObjColour::Flags)2, FEMathmos(ex::RGBA(0, 0, 0, 0), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 599940.0f,
                                                                   (FEWaveform::Flags)6, 1), 1);
        m_FadeOut = r;
        UpdateFade(1);
        return;
    }
    if (p)
        p->SetMathmos((FEObjColour::Flags)2, m_pManager->m_PageState == 0x13
                                                 ? FEMathmos(ex::RGBA(0, 0, 0, 0), ex::RGBA(0, 0, 0, 0), 599940.0f, (FEWaveform::Flags)6, 1)
                                                 : FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), ex::RGBA(0, 0, 0, 0), 599940.0f, (FEWaveform::Flags)6, 1), 1);
    m_pSlotsBackground->SetMathmos((FEObjColour::Flags)2, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), ex::RGBA(0, 0, 0, 0), 599940.0f,
                                                                    (FEWaveform::Flags)6, 1), 1);
    m_pTitle->SetMathmos((FEObjColour::Flags)2, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), ex::RGBA(0, 0, 0, 0), 599940.0f,
                                                          (FEWaveform::Flags)6, 1), 1);
    m_pBackground->SetMathmos((FEObjColour::Flags)2, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), ex::RGBA(0, 0, 0, 0), 599940.0f,
                                                               (FEWaveform::Flags)6, 1), 1);
    if (m_GoState == 11 || m_GoState == 9 || m_GoState == 0x14 || m_GoState == 0x13 || m_GoState == 0xe) {
        m_pFadeExtra->SetMathmos((FEObjColour::Flags)2, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), ex::RGBA(0, 0, 0, 0), 599940.0f,
                                                                  (FEWaveform::Flags)6, 1), 1);
        if (m_GoState != 0x13 && m_GoState != 0xe) theApp.ApplyFadeOutIn(2.0f, ex::RGBA(0, 0, 0, 0x80), 0, 1);
    }
    m_FadeIn = r;
    UpdateFade(0);
}

static FEObject *s_MainButtonLoad;
static FEObject *s_MainGrid;

signed char FEMainMenu::UpdateStateMain(FEObjEvent::Flags ev)
{
    switch (ev) {
    case 0x1e: {
        FEObject *pPage = m_pLayout->ObjectAdd(m_ListenerIdx, new FELayoutLayered(2, 0x5a, 0xb1, 0xb4, 0x86, (EXW::TextAlign)0));
        pPage->m_Flags &= ~8;
        s_MainGrid = pPage->ObjectAdd(1, new FELayoutFixedGrid(3, 1, 0, 0, 0xb4, 0x86, (EXW::TextAlign)0));
        s_MainGrid->ObjectAdd(0, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 2, Text_Get(0x510001b8), FE_Blank,
                                                 (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        s_MainButtonLoad = s_MainGrid->ObjectAdd(1, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 3, Text_Get(0x510001b9), FE_Blank,
                                                                    (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        s_MainGrid->ObjectAdd(2, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 0xd, Text_Get(0x510001ba), FE_Blank,
                                                 (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        s_MainGrid->ListenerAdd(new FEListener(0x5f5f5f5f5f76616eull, 0x401, 0, (FEPad::Flags)0xf));
        FEListener *pNav = s_MainGrid->Listener(0x5f5f5f5f5f76616eull);
        SetUpdateListener(pNav);
        s_MainGrid->Listener(0x5f5f5f5f5f76616eull)->SetFocusByData(2, 0);
        s_MainGrid->SetMathmos((FEObjColour::Flags)0,
                               FEMathmos(ex::RGBA(0x7f, 0, 0, 0x80), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 60.0f, (FEWaveform::Flags)1, 0), 1);
        s_MainGrid->SetMathmos((FEObjColour::Flags)1, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80)), 1);
        return 1;
    }
    case 0x23:
        SetTitleAndBackgr("", s_MainGrid->Rect());
        ms_bDisplayStats = 0;
        ms_InitialLoadedFocus = 9;
        GameData::Instance(1)->m_Values.m_Unkc = 0x1000136;
        GameData::Instance(1)->Stats()->Reset();
        GameData::Instance(1)->m_Values.m_Unk1c[0] = 1;
        for (unsigned int off = 0; off < 0xaf8; off += 0x68)
            ((GameStats *)((char *)GameData::Instance(1)->m_Values.m_LevelStats + off))->Reset();
        GameData::Instance(1)->GetBigSave()->ResetRitualTimes();
        GameData::Instance(1)->GetRitualTimes()->Reset();
        GameData::Instance(1)->m_Values.m_Unk1c[4] = 0;
        GameData::Instance(1)->m_Values.m_Unk1c[5] = 0;
        GameData::Instance(1)->GetBigSave()->SetStatusAndReset(0);
        FEListener::SetPadOverride((FEPad::Flags)0xf);
        GameData::Instance(1)->Values()->ResetToDefaults(1);
        if (m_PlayerSkin != 0) {
            m_PlayerSkin = 0;
            m_PredSkinUpdateState = 0;
            m_PredSkinUpdateTime = 0.0f;
            ms_bPredSkinLoaded = 0;
        }
        ms_bDisableSaveGame = 0;
        m_Unk81c = 0xe;
        if (MemCardSavesFound() & 0x3f) m_Unk82c = 3;
        if (m_pManager->m_PageState == 4) s_MainGrid->Listener(0x5f5f5f5f5f76616eull)->SetFocusByData(2, 0);
        m_Unk844 = 0;
    case 0x15:
        if (m_Unk82c != 3 && m_Unk82c != 4) {
            s_MainButtonLoad->m_Flags &= ~8;
            s_MainButtonLoad->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0x40, 0x40, 0x40, 0x6e), 1);
        } else {
            if (s_MainButtonLoad->m_Flags & 8) return 0;
            s_MainButtonLoad->m_Flags |= 8;
            s_MainButtonLoad->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 1);
        }
        return 0;
    case 0x17:
        GoState((unsigned int)m_UpdateListeners[m_ListenerIdx]->m_pFocus->m_Data);
        return 1;
    }
    return 0;
}


static FELayoutScrollGrid *s_RecordsGrid;

signed char FEMainMenu::UpdateStateViewRecords(FEObjEvent::Flags ev)
{
    switch (ev) {
    case 0x1e: {
        char name[1];
        static float widths[2] = { 0.77f, 0.23f };
        static unsigned int rows = 0x27;
        FEObject *pPage = m_pLayout->ObjectAdd(m_ListenerIdx, new FELayoutLayered(3, 0, 0, 0, 0, (EXW::TextAlign)4));
        s_RecordsGrid = (FELayoutScrollGrid *)pPage->ObjectAdd(2, new FELayoutScrollGrid(rows, 2, 0x20, 0x8d, 0x190, 0xe0, (EXW::TextAlign)4));
        s_RecordsGrid->SetGridWidthProportions(widths);
        s_RecordsGrid->SetUpDownScrollArrowForceY(0);
        s_RecordsGrid->SetVisibleCount(0xb, 2);
        s_RecordsGrid->ListenerAdd(new FEListener(0x5f5f5f5f5f76616e, 0x401, 0, (FEPad::Flags)0xf));
        FEListener *pl = s_RecordsGrid->Listener(0x5f5f5f5f5f76616e);
        SetUpdateListener(pl);
        s_RecordsGrid->SetMathmos((FEObjColour::Flags)1, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80)), 1);
        s_RecordsGrid->Listener(0x5f5f5f5f5f76616e)->SetCallback(MainMenu_Callback);
        EXString blank(" ");
        InitStatsLayout(0, s_RecordsGrid, &GameData::Instance(1)->m_Values.m_Stats, name, 0, blank);
        return 0;
    }
    case 0x23:
        SetTitleAndBackgr("Records", s_RecordsGrid->Rect());
        return 0;
    case 0x1f:
        UpdateStateViewRecords((FEObjEvent::Flags)0x1e);
        m_UpdateListeners[m_ListenerIdx]->SetFocusByData(0, 0);
        return 0;
    case 9: {
        FEListener *&l = m_UpdateListeners[m_ListenerIdx];
        if (!l) break;
        FEObject *pFocus = l->m_pFocus;
        unsigned int top = s_RecordsGrid->TopIndex().y;
        unsigned int bottom = top + 10;
        if (l->UpdateTimeStamp() == m_Unk290) {
            if ((unsigned int)pFocus->m_Data > top) {
                m_Unk290 = -1;
                l->SetFocusByData(top - 1, 1);
                return 0;
            }
        } else if (l->UpdateTimeStamp() == m_Unk28c) {
            if ((unsigned int)pFocus->m_Data < bottom) {
                m_Unk28c = -1;
                l->SetFocusByData(bottom + 1, 1);
            }
        }
        break;
    }
    }
    return 0;
}

static FELayoutScrollGrid *s_RitualTimesGrid;
static FEObject *s_RitualTimesTitle;

signed char FEMainMenu::UpdateStateRitualChallengeTimes(FEObjEvent::Flags ev)
{
    switch (ev) {
    case 0x23:
        SetTitleAndBackgr("Ritual Times", s_RitualTimesGrid->Rect());
        m_pSlotsInfo->SetText(EXString(" "), 0, 0);
        s_RitualTimesTitle->SetText(Text_Get(FEGetLevels(1)->TextFromLevel(s_RitualFocusLevel)), 0, 0);
        return 0;
    case 0x1e: {
        static float widths[3] = { 0.2f, 0.5f, 0.3f };
        FEObject *pPage = m_pLayout->ObjectAdd(m_ListenerIdx, new FELayoutLayered(2, 0, 0, 0, 0, (EXW::TextAlign)4));
        SetUpdateListener(0);
        FEObject *pG = pPage->ObjectAdd(0, new FELayoutScrollGrid(8, 3, 0x28, 0xa7, 0x190, 0xc8, (EXW::TextAlign)4));
        s_RitualTimesGrid = (FELayoutScrollGrid *)pG->StateSet(pG->State() & ~8);
        s_RitualTimesGrid->SetGridWidthProportions(widths);
        s_RitualTimesGrid->SetUpDownScrollArrowForceY(0);
        s_RitualTimesGrid->SetVisibleCount(8, 3);
        FEObject *pHeader = new FELayoutLayered(2, EXBaseDisplay::GetDisplay()->SafeFrame().Width() / 2 - 0x8c, 0x82, 0x17c, 0x18, (EXW::TextAlign)0);
        pPage->ObjectAdd(1, pHeader);
        pHeader->StateModify(0, 8);
        FEObject *pBg = pHeader->ObjectAdd(0, new FEObject(FEObjType::OT_SPRITE, XApp::Frontend2DFile(), 0x6000234, 0xfdffffff, FE_Blank,
                                                           FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        pBg->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0x23, 0x23, 0x23, 0x58), 1);
        pBg->StateModify(0, 8);
        FEObject *pGrid = pHeader->ObjectAdd(1, new FELayoutFixedGrid(1, 1, 0, 0, 0, 0, (EXW::TextAlign)4));
        s_RitualTimesTitle = new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x700000c, 0xfdffffff, EXString("Blah"), FE_Blank,
                                          (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4);
        pGrid->ObjectAdd(0, 0, s_RitualTimesTitle);
    }
    case 0x1f:
        InitRitualTimesLayout(s_RitualTimesGrid, s_RitualFocusLevel);
    }
    return 0;
}

static FEObject *s_CreditsLayout;

signed char FEMainMenu::UpdateStateCredits(FEObjEvent::Flags ev)
{
    switch (ev) {
    case 0xb:
    case 0xc:
    case 0xd:
    case 0xe:
    case 0x17:
        return 0;
    case 0x1e:
        s_CreditsLayout = m_pLayout->ObjectAdd(m_ListenerIdx, new FELayoutFixedGrid(1, 1, 0, 0, EXBaseDisplay::GetDisplay()->SafeFrame().w,
                                                                                  EXBaseDisplay::GetDisplay()->SafeFrame().Height() - 0x1a,
                                                                                  (EXW::TextAlign)0));
        return 0;
    case 0x23:
        SetTitleAndBackgr(0, s_CreditsLayout->Rect());
        m_pBackground->StateModify(0, 0x40);
        m_pHelpText->SetText(EXString(""), 0, 0);
        m_pSlotsInfo->SetText(EXString(""), 0, 0);
        return 0;
    case 0x1f:
        SetSubstate(0);
        m_pBackground->StateSub(0x40);
        return 0;
    case 0x20:
        gLastCamScript->FinishScript(XApp::Frontend3DFile(), 0x400075b);
        m_pBackground->StateSub(0x40);
        return 0;
    case 0x15:
        m_pBackground->StateModify(0, 0x40);
        switch (m_pManager->PageSubstate()) {
        case 0: {
            signed char b = m_bNewSubstate;
            m_bNewSubstate = 0;
            if (b) {
                theApp.UnpauseFade();
                m_b6 = 0;
                m_pHelpText->SetText(EXString(""), 0, 0);
                m_pSlotsInfo->SetText(EXString(""), 0, 0);
            }
            m_b6 = 1;
            m_pCreditsObj->StateModify(0x40, 0);
            SetSubstate(1);
            return 1;
        }
        case 1:
            if (!FECutscene::Instance(1)->CreditsRunning()) {
                if (m_pManager->PageSubstateTimerSecs() > 0.7f) FECutscene::Instance(1)->CreditsStart(3.0f);
                if (m_pManager->PageSubstateTimerSecs() > 5.0f) {
                    SetSubstate(6);
                    return 1;
                }
            } else {
                FECutscene::Instance(1)->CreditsUpdate();
                return 1;
            }
            break;
        case 6: {
            signed char b = m_bNewSubstate;
            m_bNewSubstate = 0;
            if (b) {
                m_pSlotsBackground->SetMathmos((FEObjColour::Flags)2,
                                               FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), ex::RGBA(0, 0, 0, 0), 30.0f, FEWaveform::WF_LINEAR, 1), 1);
                FECutscene::Instance(1)->CreditsStop();
            }
            theApp.CancelFade();
            theApp.ApplyFadeOutIn(2.0f, ex::RGBA(0, 0, 0, 0x80), 0, 1);
            SetSubstate(7);
            return 1;
        }
        case 7:
            if (m_pManager->PageSubstateTimerSecs() >= 1.0f) {
                theApp.CancelFade();
                theApp.ApplyFadeIn(1.0f, ex::RGBA(0, 0, 0, 0x80), 0);
                gLastCamScript->FinishScript(XApp::Frontend3DFile(), 0x40004ef);
                GoState(0x11);
            }
            break;
        }
        return 1;
    case 0x16:
        if (m_pManager->PageSubstate() == 1) {
            XGetSoundManager()->StartSound(0x1af00158);
            SetSubstate(6);
        }
        return 0;
    }
    return 0;
}

static FEObject *s_TrainingGrid;
static FEObject *s_LoadedSaveItem;
static FEObject *s_LoadedRitualItem;
static FEObject *s_LoadedGrid;

// NEAR: retail constructs the Save Game FEMathmos in place and shares one stack frame across the cases.
signed char FEMainMenu::UpdateStateLoaded(FEObjEvent::Flags ev)
{
    switch (ev) {
    case 0x1e: {
        s_LoadedGrid = m_pLayout->ObjectAdd(m_ListenerIdx, new FELayoutFixedGrid(9, 1, 0x28, 0x8c, 0x140, 0xd2, (EXW::TextAlign)0));
        s_LoadedGrid->ObjectAdd(0, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 5, EXString("Training"), FE_Blank,
                                                   (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        s_LoadedGrid->ObjectAdd(1, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 6, EXString("Chapter Select"), FE_Blank,
                                                   (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        s_LoadedRitualItem = s_LoadedGrid->ObjectAdd(2, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 7, EXString("Ritual Challenges"), FE_Blank,
                                                   (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        s_LoadedGrid->ObjectAdd(3, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 8, EXString("Records"), FE_Blank,
                                                   (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        s_LoadedGrid->ObjectAdd(4, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 9, EXString("Hunting Gear"), FE_Blank,
                                                   (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        s_LoadedGrid->ObjectAdd(5, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 0xa, EXString("Costume Select"), FE_Blank,
                                                   (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        s_LoadedGrid->ObjectAdd(6, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 0xb, EXString("Trophies"), FE_Blank,
                                                   (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        s_LoadedGrid->ObjectAdd(7, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 0xd, EXString("Options"), FE_Blank,
                                                   (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        s_LoadedSaveItem = s_LoadedGrid->ObjectAdd(8, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 0xc, EXString("Save Game"), FE_Blank,
                                                   (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        s_LoadedGrid->ListenerAdd(new FEListener(0x5f5f5f5f5f76616eull, 0x401, 0, (FEPad::Flags)0xf));
        FEListener *pl = s_LoadedGrid->Listener(0x5f5f5f5f5f76616eull);
        SetUpdateListener(pl);
        s_LoadedGrid->SetMathmos((FEObjColour::Flags)0,
                                 FEMathmos(ex::RGBA(0x7f, 0, 0, 0x80), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 60.0f, FEWaveform::WF_LINEAR, 0), 1);
        s_LoadedGrid->SetMathmos((FEObjColour::Flags)1, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80)), 1);
        return 0;
    }
    default:
        return 0;
    case 0x23:
        SetTitleAndBackgr(0, s_LoadedGrid->Rect());
        if (ms_bDisableSaveGame) {
            s_LoadedSaveItem->StateSub(8);
            s_LoadedSaveItem->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0x40, 0x40, 0x40, 0x6e), 1);
        } else {
            s_LoadedSaveItem->StateAdd(8);
            s_LoadedSaveItem->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 1);
        }
        m_Unk844 = 1;
        return 0;
    case 0x1f:
        SetInitialChapterSelectFocus();
        m_b4 = 1;
        s_RitualFocusLevel = 0x10000b1;
        s_LoadedGrid->Listener(0x5f5f5f5f5f76616eull)->SetFocusByData(ms_InitialLoadedFocus, 0);
        return 0;
    case 0x17:
        ms_InitialLoadedFocus = (unsigned int)m_UpdateListeners[m_ListenerIdx]->m_pFocus->m_Data;
        GoState(ms_InitialLoadedFocus);
        return 1;
    }
}

signed char FEMainMenu::UpdateStateTraining(FEObjEvent::Flags ev)
{
    switch (ev) {
    case 0x15:
        return 0;
    case 0x1e: {
        FEObject *pPage = m_pLayout->ObjectAdd(m_ListenerIdx, new FELayoutLayered(2, 0x5a, 0xb1, 0xb4, 0x86, (EXW::TextAlign)0));
        pPage->StateModify(0, 8);
        s_TrainingGrid = pPage->ObjectAdd(1, new FELayoutFixedGrid(3, 1, 0, 0, 0x140, 0x86, (EXW::TextAlign)0));
        s_TrainingGrid->ObjectAdd(0, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 0x1000160, Text_Get(0x51000572), FE_Blank,
                                                     (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        s_TrainingGrid->ObjectAdd(1, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 0x1000161, Text_Get(0x51000573), FE_Blank,
                                                     (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        s_TrainingGrid->ObjectAdd(2, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 0x1000162, Text_Get(0x51000574), FE_Blank,
                                                     (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        s_TrainingGrid->ListenerAdd(new FEListener(0x5f5f5f5f5f76616e, 0x401, 0, (FEPad::Flags)0xf));
        FEListener *pl = s_TrainingGrid->Listener(0x5f5f5f5f5f76616e);
        SetUpdateListener(pl);
        s_TrainingGrid->SetMathmos((FEObjColour::Flags)0,
                                   FEMathmos(ex::RGBA(0x7f, 0, 0, 0x80), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 60.0f, FEWaveform::WF_LINEAR, 0), 1);
        s_TrainingGrid->SetMathmos((FEObjColour::Flags)1, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80)), 1);
        return 1;
    }
    case 0x23:
        SetTitleAndBackgr("Training", s_TrainingGrid->Rect());
        return 0;
    case 0x1f: {
        m_b4 = 1;
        unsigned int data = 0x1000160;
        if (GameData::Instance(1)->m_Values.m_Training != 7) {
            switch (GameData::Instance(1)->m_Values.m_Training) {
            case 3:
                data = 0x1000162;
                break;
            case 1:
            case 5:
                data = 0x1000161;
                break;
            case 2:
            case 4:
            case 6:
                data = 0x1000160;
                break;
            }
        }
        s_TrainingGrid->Listener(0x5f5f5f5f5f76616e)->SetFocusByData(data, 0);
        return 0;
    }
    case 0x17:
        unsigned int a = (unsigned int)s_TrainingGrid->Listener(0x5f5f5f5f5f76616e)->m_pFocus->m_Data;
        m_DoneState = 3;
        m_DoneAction = a;
        GoState(0x17);
        return 1;
    }
    return 0;
}

// Shared by the Options pages (the 0x48de48 static in the original).
static FEObject *s_OptionsGrid;

signed char FEMainMenu::UpdateStateOptions(FEObjEvent::Flags ev)
{
    FEObject *pGrid = s_OptionsGrid;
    switch (ev) {
    case 0x1e: {
        FEObject *p = m_pLayout->ObjectAdd(m_ListenerIdx, new FELayoutFixedGrid(4, 1, 0x5a, 0xa7, 0xb4, 0x9a, (EXW::TextAlign)0));
        s_OptionsGrid = p;
        s_OptionsGrid->ObjectAdd(0, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 0xe, Text_Get(0x510001bf), FE_Blank,
                                                    (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        s_OptionsGrid->ObjectAdd(1, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 0xf, Text_Get(0x510001c0), FE_Blank,
                                                    (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        s_OptionsGrid->ObjectAdd(2, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 0x10, Text_Get(0x510001c1), FE_Blank,
                                                    (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        s_OptionsGrid->ObjectAdd(3, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 0x11, "Game", FE_Blank,
                                                    (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        s_OptionsGrid->ListenerAdd(new FEListener(0x5f5f5f5f5f76616e, 0x401, 0, (FEPad::Flags)0xf));
        FEListener *pl = s_OptionsGrid->Listener(0x5f5f5f5f5f76616e);
        SetUpdateListener(pl);
        s_OptionsGrid->Listener(0x5f5f5f5f5f76616e)->SetFocusByData(0xe, 0);
        s_OptionsGrid->SetMathmos((FEObjColour::Flags)0,
                                  FEMathmos(ex::RGBA(0x7f, 0, 0, 0x80), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 60.0f, FEWaveform::WF_LINEAR, 0), 1);
        s_OptionsGrid->SetMathmos((FEObjColour::Flags)1, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80)), 1);
        break;
    }
    case 0x23:
        SetTitleAndBackgr("Options", s_OptionsGrid->Rect());
        return 0;
    case 0x1f:
        m_b4 = 1;
        pGrid->Listener(0x5f5f5f5f5f76616e)->SetFocusByData(m_Unk81c, 0);
        return 0;
    case 0x17:
        GoState(m_Unk81c = (unsigned int)m_pCurLayout->Listener(0x5f5f5f5f5f76616e)->m_pFocus->m_Data);
        return 1;
    }
    return 0;
}

// NEAR: retail hoists the GameData::Instance() call in the InitStatsLayout argument list above the constant pushes and
// calls FEObject::Rect out of line in case 0x23; both are inlined/ordered differently here.
static FEObject *s_StatsTitle;
static FEObject *s_StatsGrid;

signed char FEMainMenu::UpdateStateStoryLevelStats(FEObjEvent::Flags ev)
{
    FEListener *pL = m_UpdateListeners[m_ListenerIdx];
    static EXString blank(" ");
    switch (ev) {
    case 0x1e: {
        static float widths[2] = { 0.77f, 0.23f };
        static unsigned int rows = 0x25;
        FEObject *pPage = m_pLayout->ObjectAdd(m_ListenerIdx, new FELayoutLayered(2, 0, 0, 0, 0, (EXW::TextAlign)4));
        FELayoutScrollGrid *pSG = (FELayoutScrollGrid *)pPage->ObjectAdd(0, new FELayoutScrollGrid(rows, 2, 0x20, 0xa5, 0x190, 0xcd, (EXW::TextAlign)4));
        s_StatsGrid = pSG;
        pSG->SetGridWidthProportions(widths);
        pSG->SetUpDownScrollArrowForceY(0);
        pSG->SetVisibleCount(8, 2);
        s_StatsGrid->ListenerAdd(new FEListener(0x5f5f5f5f5f76616e, 0x401, 0, (FEPad::Flags)0xf));
        FEListener *pl = s_StatsGrid->Listener(0x5f5f5f5f5f76616e);
        SetUpdateListener(pl);
        s_StatsGrid->SetMathmos((FEObjColour::Flags)1, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80)), 1);
        s_StatsGrid->Listener(0x5f5f5f5f5f76616e)->SetCallback(MainMenu_Callback);
        FEObject *pHeader = new FELayoutLayered(2, EXBaseDisplay::GetDisplay()->SafeFrame().Width() / 2 - 0xb4, 0x85, 0x17c, 0x18, (EXW::TextAlign)0);
        pPage->ObjectAdd(1, pHeader);
        pHeader->StateModify(0, 8);
        FEObject *pBg = pHeader->ObjectAdd(0, new FEObject(FEObjType::OT_SPRITE, XApp::Frontend2DFile(), 0x6000234, 0xfdffffff, FE_Blank,
                                                           FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        pBg->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0x23, 0x23, 0x23, 0x58), 1);
        pBg->StateModify(0, 8);
        FEObject *pGrid = pHeader->ObjectAdd(1, new FELayoutFixedGrid(1, 1, 0, 0, 0, 0, (EXW::TextAlign)4));
        EXString name = Text_Get(FEGetLevels(1)->TextFromIndex(s_ChapterFocusIndex));
        s_StatsTitle = new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x700000c, 0xfdffffff, EXString("Blah"), FE_Blank,
                                    (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4);
        pGrid->ObjectAdd(0, 0, s_StatsTitle);
        InitStatsLayout(s_ChapterFocusIndex, (FELayoutScrollGrid *)s_StatsGrid, &GameData::Instance(1)->m_Values.m_LevelStats[s_ChapterFocusIndex], 0, 0, blank);
        return 0;
    }
    case 0x23:
        SetTitleAndBackgr("Chapter Statistics", s_StatsGrid->Rect());
        return 0;
    case 0x1f: {
        EXString name = Text_Get(FEGetLevels(1)->TextFromIndex(s_ChapterFocusIndex));
        s_StatsTitle->SetText(name, 0, 0);
        InitStatsLayout(s_ChapterFocusIndex, (FELayoutScrollGrid *)s_StatsGrid, &GameData::Instance(1)->m_Values.m_LevelStats[s_ChapterFocusIndex], 0, 1, blank);
        pL->SetFocusByData(0, 0);
        return 0;
    }
    case 9:
        if (pL) {
            unsigned int top = ((FELayoutScrollGrid *)s_StatsGrid)->TopIndex().y;
            if (pL->UpdateTimeStamp() == m_Unk290) {
                if (top < (unsigned int)pL->m_pFocus->m_Data) {
                    m_Unk290 = -1;
                    pL->SetFocusByData(top - 1, 1);
                    return 0;
                }
            } else if (pL->UpdateTimeStamp() == m_Unk28c) {
                if ((unsigned int)pL->m_pFocus->m_Data < top + 7) {
                    m_Unk28c = -1;
                    pL->SetFocusByData(top + 8, 1);
                }
            }
        }
        break;
    }
    return 0;
}

// NEAR: register allocation around the slider Bind calls and the pObj prologue differs (150).
static FEObject *s_OptionsAudioPage;
static FEObject *s_OptionsGamePage;

// NEAR: VC hoists a zero register through this function; retail uses immediate zeros.
signed char FEMainMenu::UpdateStateOptionsGame(FEObjEvent::Flags ev)
{
    FELayout *pCur = m_pCurLayout;
    FEObject *pObj = pCur ? LayoutObject(pCur, 1) : 0;
    switch (ev) {
    case 0x1e: {
        static float widths[2] = { 0.6f, 0.4f };
        s_OptionsGamePage = m_pLayout->ObjectAdd(m_ListenerIdx, new FELayoutLayered(2, 0, 0, 0, 0, (EXW::TextAlign)4));
        FELayoutScrollGrid *pGrid = new FELayoutScrollGrid(3, 2, 0x48, 0xa0, 0x1a8, 0x78, (EXW::TextAlign)0);
        s_OptionsGamePage->ObjectAdd(1, pGrid);
        pGrid->SetGridWidthProportions(widths);
        pGrid->SetUpDownScrollArrowForceY(0);
        pGrid->SetVisibleCount(3, 2);
        FEObject *pText = AddScrollGridText(pGrid, 0, 0, 0x5f71456f74754174ull, EXString("Auto-equip Weapon"), (EXW::TextAlign)2, EXString(""), 1);
        FEObject *pCheck = pGrid->ObjectAdd(0, 1, new FELayoutCheckbox(0, 0, 0, 0, 0, (EXW::TextAlign)4));
        pCheck->m_Data = 0x5f5f71456f747541ull;
        pCheck->Bind((FEDataType::Flags)1, &GameData::Instance(1)->Values()->m_bAutoEquip);
        pCheck->SetHelpText(Text_Get(0x510004d7));
        pText->MathmosLink(pCheck);
        pText = AddScrollGridText(pGrid, 1, 0, 0x6b72614d79615774ull, Text_Get(0x510002b6), (EXW::TextAlign)2, EXString(""), 1);
        FEObject *pSub = pGrid->ObjectAdd(1, 1, new FELayoutScrollGrid(1, 2, 0, 0, 0, 0, (EXW::TextAlign)4));
        pSub->m_Data = 0x5f6b72614d796157ull;
        pText->MathmosLink(pSub);
        pSub->ObjectAdd(0, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 1, Text_Get(0x510002b7), FE_Blank,
                                           (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4))
            ->SetHelpText(Text_Get(0x51000474))->m_AltData = 0x416b72614d796157ull;
        pSub->ObjectAdd(0, 1, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 0, Text_Get(0x510002ba), FE_Blank,
                                           (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4))
            ->SetHelpText(Text_Get(0x51000474))->m_AltData = 0x426b72614d796157ull;
        pSub->SetVisibleCount(1, 1);
        pSub->Bind((FEDataType::Flags)1, &GameData::Instance(1)->Values()->m_Unk1c[6]);
        pText = AddScrollGridText(pGrid, 2, 0, 0x7374696465724374ull, EXString("Credits"), (EXW::TextAlign)2, EXString(""), 0);
        pText->StateSub(8);
        FEObject *pBtn = AddScrollGridText(pGrid, 2, 1, 0x5f73746964657243ull, EXString(""), (EXW::TextAlign)2, EXString(""), 0);
        pBtn->StateAdd(8);
        pText->MathmosLink(pBtn);
        pGrid->ListenerAdd(new FEListener(0x5f5f5f5f5f76616e, 0xc03, 0, (FEPad::Flags)0xf));
        FEListener *pl = pGrid->Listener(0x5f5f5f5f5f76616e);
        SetUpdateListener(pl);
        pGrid->Listener(0x5f5f5f5f5f76616e)->SetFocusByData(0x5f5f71456f747541ull, 0);
        pGrid->SetMathmos((FEObjColour::Flags)0,
                          FEMathmos(ex::RGBA(0x7f, 0, 0, 0x80), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 60.0f, FEWaveform::WF_LINEAR, 0), 1);
        pGrid->SetMathmos((FEObjColour::Flags)1, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80)), 1);
        return 0;
    }
    default:
        return 0;
    case 0x23:
        SetTitleAndBackgr("Game Options", pObj->Rect());
        return 0;
    case 0x1f:
        pObj->Listener(0x5f5f5f5f5f76616e)->SetFocusByData(m_pManager->m_PageState == 0x14 ? 0x5f73746964657243ull : 0x5f5f71456f747541ull, 0);
        return 0;
    case 9: {
        FEListener *pL = m_UpdateListeners[m_ListenerIdx];
        if (!pL) break;
        if (!pL->m_pFocus) break;
        unsigned int help = 0;
        if (pL->m_pFocus->m_AltData == 0x416b72614d796157ull || pL->m_pFocus->m_AltData == 0x426b72614d796157ull)
            help = 0x51000844;
        if (help)
            ms_pInstance->SetInfoObjText("~X Help", help);
        else
            ms_pInstance->SetInfoObjText(0, 0);
        return 0;
    }
    case 0x17:
        if (m_UpdateListeners[m_ListenerIdx]->m_pFocus->m_Data == 0x5f73746964657243ull) {
            GoState(0x14);
            return 1;
        }
        break;
    }
    return 0;
}

signed char FEMainMenu::UpdateStateOptionsAudio(FEObjEvent::Flags ev)
{
    FELayout *pCur = m_pCurLayout;
    FEObject *pObj = pCur ? pCur->Object(1) : 0;
    switch (ev) {
    case 0x19:
        return 0;
    case 0x1e: {
        static float widths[2] = { 0.55f, 0.45f };
        s_OptionsAudioPage = m_pLayout->ObjectAdd(m_ListenerIdx, new FELayoutLayered(2, 0, 0, 0, 0, (EXW::TextAlign)4));
        FELayoutScrollGrid *pGrid = new FELayoutScrollGrid(3, 2, 0x20, 0x9b, 0x15c, 0xdc, (EXW::TextAlign)0);
        s_OptionsAudioPage->ObjectAdd(1, pGrid);
        pGrid->SetGridWidthProportions(widths);
        pGrid->SetUpDownScrollArrowForceY(0);
        pGrid->SetVisibleCount(3, 2);
        FEObject *pText = AddScrollGridText(pGrid, 0, 0, 0x5f5f636973756d74, Text_Get(0x510001d7), (EXW::TextAlign)2, EXString(""), 1);
        FEObject *pSlider = pGrid->ObjectAdd(0, 1, new FELayoutSlider(0, 100, 0, 0, 0, 0, (EXW::TextAlign)4));
        pText->MathmosLink(pSlider);
        pSlider->m_Data = 0x5f5f5f636973756d;
        pSlider->SetRange(0, 100);
        pSlider->SetDelta(5);
        pSlider->StateModify(0, 0x8000000);
        pSlider->SetHelpText(Text_Get(0x51000474));
        pSlider->Bind((FEDataType::Flags)3, &GameData::Instance(1)->Values()->m_MusicVolume);
        pSlider->SetEventCallback(Pause_ChangeVolume, 1);
        pText = AddScrollGridText(pGrid, 1, 0, 0x5f5f5f5f78667374, Text_Get(0x510001d8), (EXW::TextAlign)2, EXString(""), 1);
        pSlider = pGrid->ObjectAdd(1, 1, new FELayoutSlider(0, 100, 0, 0, 0, 0, (EXW::TextAlign)4));
        pText->MathmosLink(pSlider);
        pSlider->m_Data = 0x5f5f5f5f5f786673;
        pSlider->SetRange(0, 100);
        pSlider->SetDelta(5);
        pSlider->StateModify(0, 0x8000000);
        pSlider->SetHelpText(Text_Get(0x51000474));
        pSlider->Bind((FEDataType::Flags)3, &GameData::Instance(1)->Values()->m_MovieVolume);
        pSlider->SetEventCallback(Pause_ChangeVolume, 1);
        pText = AddScrollGridText(pGrid, 2, 0, 0x5f5f5f7462755374, Text_Get(0x510001de), (EXW::TextAlign)2, EXString(""), 1);
        FEObject *pCheck = pGrid->ObjectAdd(2, 1, new FELayoutCheckbox(0, 0, 0, 0, 0, (EXW::TextAlign)4));
        pCheck->m_Data = 0x5f5f5f5f74627553;
        pCheck->Bind((FEDataType::Flags)1, &GameData::Instance(1)->Values()->m_Unk1c[2]);
        pCheck->SetHelpText(Text_Get(0x510004d7));
        pText->MathmosLink(pCheck);
        pGrid->ListenerAdd(new FEListener(0x5f5f5f5f5f76616e, 0xc03, 0, (FEPad::Flags)0xf));
        FEListener *pl = pGrid->Listener(0x5f5f5f5f5f76616e);
        SetUpdateListener(pl);
        pGrid->Listener(0x5f5f5f5f5f76616e)->SetFocus(0, 0);
        pGrid->SetMathmos((FEObjColour::Flags)0,
                          FEMathmos(ex::RGBA(0x7f, 0, 0, 0x80), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 60.0f, FEWaveform::WF_LINEAR, 0), 1);
        pGrid->SetMathmos((FEObjColour::Flags)1, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80)), 1);
        return 0;
    }
    case 0x1f:
        if (!pObj->Listener(0x5f5f5f5f5f76616e)->m_pFocus) {
            pObj->Listener(0x5f5f5f5f5f76616e)->SetFocusByData(0x5f5f5f636973756d, 0);
            return 0;
        }
        break;
    case 0x23:
        SetTitleAndBackgr("Audio Options", pObj->Rect());
        return 0;
    case 0x21:
        pObj->Listener(0x5f5f5f5f5f76616e)->SetFocus(0, 0);
        break;
    }
    return 0;
}

// NEAR: retail keeps a zeroed edi live through the loop (null compares/stores) and spills differently.
__declspec(noinline) signed char FEMainMenu::UpdateState(unsigned int state, FEObjEvent::Flags ev)
{
    signed char all = 0;
    signed char r = 0;
    if (state == 0xffffffff) {
        state = 0;
        all = 1;
    }
    do {
        m_ListenerIdx = state;
        m_pCurLayout = m_pLayout ? (FELayout *)m_pLayout->Object(state) : 0;
        if (ev != 0x15 || !m_bGoing) {
            if (ev == 0x1f && !(m_Unk1fc & (1 << (m_ListenerIdx & 31)))) {
                m_pLayout->ObjectDel(state);
                UpdateState(state, (FEObjEvent::Flags)0x1e);
            }
            switch (state) {
            case 0: r = 1; break;
            case 1: r = UpdateStateMain(ev); break;
            case 2: r = UpdateStateNewGame(ev); break;
            case 3: r = UpdateStateLoadGame(ev); break;
            case 13: r = UpdateStateOptions(ev); break;
            case 14: r = UpdateStateOptionsController(ev); break;
            case 15: r = UpdateStateOptionsCamera(ev); break;
            case 16: r = UpdateStateOptionsAudio(ev); break;
            case 23: r = UpdateStateDone(ev); break;
            case 4: r = UpdateStateLoaded(ev); break;
            case 6: r = UpdateStateStoryLevelSelect(ev); break;
            case 7: r = UpdateStateRitualChallengeSelect(ev); break;
            case 8: r = UpdateStateViewRecords(ev); break;
            case 10: r = UpdateStateCostumeSelect(ev); break;
            case 9:
            case 11: r = UpdateStateViewTrophies(ev); break;
            case 18: r = UpdateStateAutosave(ev); break;
            case 19: r = UpdateStateEditName(ev); break;
            case 5: r = UpdateStateTraining(ev); break;
            case 12: r = UpdateStateSaveGame(ev); break;
            case 17: r = UpdateStateOptionsGame(ev); break;
            case 20: r = UpdateStateCredits(ev); break;
            case 21: r = UpdateStateStoryLevelStats(ev); break;
            case 22: r = UpdateStateRitualChallengeTimes(ev); break;
            }
        }
        switch (ev) {
        case 0x1e:
            m_pCurLayout = m_pLayout ? (FELayout *)m_pLayout->Object(m_ListenerIdx) : 0;
            if (m_pCurLayout) {
                m_pCurLayout->SetData(m_ListenerIdx);
                m_pCurLayout->StateSub(0x40);
            }
            m_Unk1fc |= 1 << (m_ListenerIdx & 31);
            break;
        case 0x20:
            if (m_UpdateListeners[m_ListenerIdx]) NewFocus(m_UpdateListeners[m_ListenerIdx]->m_pFocus, state);
            break;
        case 0x21:
            m_pHelpText->SetText(FE_Blank, 0, 0);
            if (!m_b4 && m_ListenerIdx != 0 && m_ListenerIdx != 0x17) {
                m_pArrowL->StateSub(0x40);
                m_pArrowR->StateSub(0x40);
            } else {
                m_b4 = 0;
            }
            break;
        case 0x22:
            if (m_pCurLayout) {
                m_pCurLayout->StateSub(0x40);
                m_pCurLayout->StateSub(0x20);
            }
            break;
        case 0x23: {
            signed char hide = 0;
            char *txt;
            switch (m_ListenerIdx) {
            case 1:
            case 0x12:
            case 0x13:
            case 0x14:
                hide = 1;
                goto setflag;
            default: txt = "~B Back"; break;
            case 4: txt = "~X Quit";
            }
            m_pCreditsObj->SetText(EXString(txt), 0, 0);
        setflag:
            m_pCreditsObj->StateModify(hide ? 0 : 0x40, 0x40);
            break;
        }
        case 0x24: m_Unk1fc &= ~(1 << (m_ListenerIdx & 31)); break;
        case 0x26:
            if (!m_UpdateListeners[m_ListenerIdx]) m_Unk274 = 0;
            else m_Unk274 = (u32)m_UpdateListeners[m_ListenerIdx]->m_pFocus;
            break;
        case 0x27: {
            EXBaseSoundManager::GetSoundManager()->StartMusic(0x1b000049, 2, 1, 1, 0);
            FEListener *pL = m_UpdateListeners[m_ListenerIdx];
            if (pL) {
                m_b4 = 1;
                pL->SetFocus(0, 0);
                pL->SetFocus((FEObject *)m_Unk274, 0);
                m_b4 = 0;
            }
            break;
        }
        }
        if (!all || ++state > 0x17) return r;
    } while (1);
}

static FELayoutScrollGrid *s_StoryGrid;

signed char FEMainMenu::UpdateStateStoryLevelSelect(FEObjEvent::Flags ev)
{
    static EXString sSelect;
    unsigned int count = FELevelManager::Instance(1)->StoryLevelCount();
    switch (ev) {
    case 0x1e: {
        FEObject *pPage = m_pLayout->ObjectAdd(m_ListenerIdx, new FELayoutLayered(1, 0, 0, 0, 0, (EXW::TextAlign)4));
        sSelect = Text_Get(0x51000476);
        FELayoutScrollGrid *pGrid = (FELayoutScrollGrid *)pPage->ObjectAdd(0, new FELayoutScrollGrid(count, 1, 0x32, 0x85, 0x17c, 0xe6, (EXW::TextAlign)0));
        s_StoryGrid = pGrid;
        pGrid->SetHelpText(sSelect);
        pGrid->m_Data = 0xfdffffffull;
        pGrid->StateSub(8);
        pGrid->SetUpDownScrollArrowForceY(0);
        FEObject *pFocusObj = 0;
        for (unsigned int i = 0; i < count; i++) {
            EXString name = Text_Get(FELevelManager::Instance(1)->TextFromIndex(i));
            FEObject *pObj = pGrid->ObjectAdd(i, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 0xfdffffffull, name, FE_Blank,
                                                                       (EXW::TextAlign)2, 0, 0, 0, 0, (EXW::TextAlign)4));
            pObj->StateAdd(8);
            pObj->m_Data = FELevelManager::Instance(1)->LevelAltFromIndex(i);
            pObj->SetHelpText(sSelect);
            if (i > 0 && !GameData::Instance(1)->Values()->m_LevelStats[i - 1].m_Unk0[0]) {
                pObj->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0x40, 0x40, 0x40, 0x6e), 1);
                pObj->m_Data = i;
                pObj->SetHelpText(EXString(" "));
            } else {
                pObj->SetMathmos((FEObjColour::Flags)3,
                                 GameData::Instance(1)->Values()->m_LevelStats[i].m_Unk0[0] ? ex::RGBA(0x7f, 0x7f, 0x7f, 0x80) : ex::RGBA(0x65, 0x65, 0x65, 0x80), 1);
            }
            unsigned __int64 data = pObj->m_Data;
            if (data == FELevelManager::Instance(1)->m_Levels[s_ChapterFocusIndex].m_LevelAlt)
                pFocusObj = pObj;
        }
        pGrid->SetVisibleCount(9, 1);
        s_StoryGrid->Deflate(10, 10);
        pGrid->EnsureVisible(pFocusObj, 0);
        s_StoryGrid->ListenerAdd(new FEListener(0x5f5f5f5f5f76616eull, 0x401, 0, (FEPad::Flags)0xf));
        FEListener *pl = s_StoryGrid->Listener(0x5f5f5f5f5f76616eull);
        SetUpdateListener(pl);
        s_StoryGrid->SetMathmos((FEObjColour::Flags)0,
                                FEMathmos(ex::RGBA(0x7f, 0, 0, 0x80), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 60.0f, FEWaveform::WF_LINEAR, 0), 1);
        s_StoryGrid->SetMathmos((FEObjColour::Flags)1, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80)), 1);
        s_StoryGrid->Listener(0x5f5f5f5f5f76616eull)->SetCallback(MainMenu_Callback);
        return 0;
    }
    default:
        return 0;
    case 0x23:
        SetTitleAndBackgr("Chapter Select", s_StoryGrid->Rect());
        SetInfoObjText("~X Stats", 0);
        return 0;
    case 0x1f: {
        int idx = s_ChapterFocusIndex;
        SetInfoObjText(0, 0);
        if (idx != 0) idx--;
        if (!GameData::Instance(1)->Values()->m_LevelStats[idx].m_Unk0[0])
            s_StoryGrid->Listener(0x5f5f5f5f5f76616eull)->SetFocusByData((unsigned int)s_ChapterFocusIndex, 0);
        else
            s_StoryGrid->Listener(0x5f5f5f5f5f76616eull)->SetFocusByData(s_ChapterFocusLevel, 0);
        return 0;
    }
    case 3: {
        if (!s_StoryGrid->Listener(0x5f5f5f5f5f76616eull)) return 0;
        if (!s_StoryGrid->Listener(0x5f5f5f5f5f76616eull)->m_pFocus) return 0;
        unsigned int level = (unsigned int)s_StoryGrid->Listener(0x5f5f5f5f5f76616eull)->m_pFocus->m_Data;
        if (level < 0x64) return 0;
        s_ChapterFocusIndex = FELevelManager::Instance(1)->IndexFromLevel(level);
        s_ChapterFocusLevel = FELevelManager::Instance(1)->m_Levels[s_ChapterFocusIndex].m_LevelAlt;
        XGetSoundManager()->StartSound(0x1af0015c);
        GoState(0x15);
        return 0;
    }
    case 9:
        if (!s_StoryGrid->Listener(0x5f5f5f5f5f76616eull)) return 0;
        if (!s_StoryGrid->Listener(0x5f5f5f5f5f76616eull)->m_pFocus) return 0;
        if ((unsigned int)s_StoryGrid->Listener(0x5f5f5f5f5f76616eull)->m_pFocus->m_Data >= 0x64)
            SetInfoObjText("~X Stats", 0);
        else
            SetInfoObjText(0, 0);
        return 0;
    case 0x17: {
        if (!s_StoryGrid->Listener(0x5f5f5f5f5f76616eull)) return 0;
        if (!s_StoryGrid->Listener(0x5f5f5f5f5f76616eull)->m_pFocus) return 0;
        unsigned int idx = (unsigned int)s_StoryGrid->Listener(0x5f5f5f5f5f76616eull)->m_pFocus->m_Data;
        unsigned int level = idx;
        if (idx < 0x64) {
            level = FELevelManager::Instance(1)->LevelFromIndex(idx);
            if (idx > 0 && !XGetGameValues()->m_LevelStats[idx - 1].m_Unk0[0]) return 0;
        }
        XGetGameValues()->m_Unkc = level;
        unsigned int slot = MemCardSlot();
        m_DoneState = 2;
        m_DoneAction = slot;
        GoState(0x17);
        return 1;
    }
    }
}

static FELayoutScrollGrid *s_RitualGrid;

signed char FEMainMenu::UpdateStateRitualChallengeSelect(FEObjEvent::Flags ev)
{
    static EXString sSelect;
    switch (ev) {
    case 0x1e: {
        FEObject *pPage = m_pLayout->ObjectAdd(m_ListenerIdx, new FELayoutLayered(1, 0, 0, 0, 0, (EXW::TextAlign)4));
        if (GameData::Instance(1)->m_Values.m_Rituals == 0)
            sSelect = EXString(" ");
        else
            sSelect = Text_Get(0x51000476);
        FELayoutScrollGrid *pGrid = (FELayoutScrollGrid *)pPage->ObjectAdd(0, new FELayoutScrollGrid(8, 1, 0x32, 0x85, 0x17c, 0xe6, (EXW::TextAlign)0));
        s_RitualGrid = pGrid;
        pGrid->SetHelpText(sSelect);
        pGrid->m_Data = 0xfdffffffull;
        pGrid->StateSub(8);
        pGrid->SetUpDownScrollArrowForceY(0);
        FEObject *pFocusObj = 0;
        for (unsigned int bit = 1; bit - 1 < 8; bit++) {
            unsigned int hash = FELevelManager::Instance(1)->RitualLevelHashFromBitIndex(bit);
            int idx = FELevelManager::Instance(1)->IndexFromLevel(hash);
            EXString name = Text_Get(FELevelManager::Instance(1)->TextFromIndex(idx));
            FEObject *pObj = pGrid->ObjectAdd(bit - 1, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 0xfdffffffull, name, FE_Blank,
                                                                              (EXW::TextAlign)2, 0, 0, 0, 0, (EXW::TextAlign)4));
            pObj->StateAdd(8);
            pObj->m_Data = bit;
            pObj->SetHelpText(sSelect);
            if (!(GameData::Instance(1)->m_Values.m_Rituals & (1 << bit)))
                pObj->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0x40, 0x40, 0x40, 0x6e), 1);
            else
                pObj->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 1);
            if (hash == s_RitualFocusLevel) pFocusObj = pObj;
        }
        pGrid->SetVisibleCount(8, 1);
        s_RitualGrid->Inflate(-10, -10);
        pGrid->EnsureVisible(pFocusObj, 0);
        s_RitualGrid->ListenerAdd(new FEListener(0x5f5f5f5f5f76616eull, 0x401, 0, (FEPad::Flags)0xf));
        FEListener *pl = s_RitualGrid->Listener(0x5f5f5f5f5f76616eull);
        SetUpdateListener(pl);
        s_RitualGrid->SetMathmos((FEObjColour::Flags)0,
                                 FEMathmos(ex::RGBA(0x7f, 0, 0, 0x80), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 60.0f, FEWaveform::WF_LINEAR, 0), 1);
        s_RitualGrid->SetMathmos((FEObjColour::Flags)1, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80)), 1);
        return 0;
    }
    default:
        return 0;
    case 0x23:
        SetTitleAndBackgr("Ritual Select", s_RitualGrid->Rect());
        SetInfoObjText(0, 0);
        if (!s_RitualGrid->Listener(0x5f5f5f5f5f76616eull)) return 0;
        if (!s_RitualGrid->Listener(0x5f5f5f5f5f76616eull)->m_pFocus) return 0;
        {
            unsigned int bit = (unsigned int)s_RitualGrid->Listener(0x5f5f5f5f5f76616eull)->m_pFocus->m_Data;
            if (XGetGameValues()->RitualUnlocked(bit)) SetInfoObjText("~X Times", 0);
        }
        return 0;
    case 0x1f: {
        FELevelManager *pLM = FELevelManager::Instance(1);
        s_RitualGrid->Listener(0x5f5f5f5f5f76616eull)->SetFocusByData(pLM->RitualBitIndexFromLevelHash(s_RitualFocusLevel), 0);
        return 0;
    }
    case 3: {
        if (!s_RitualGrid->Listener(0x5f5f5f5f5f76616eull)) return 0;
        if (!s_RitualGrid->Listener(0x5f5f5f5f5f76616eull)->m_pFocus) return 0;
        unsigned int bit = (unsigned int)s_RitualGrid->Listener(0x5f5f5f5f5f76616eull)->m_pFocus->m_Data;
        if (!XGetGameValues()->RitualUnlocked(bit)) return 0;
        s_RitualFocusLevel = FELevelManager::Instance(1)->RitualLevelHashFromBitIndex(bit);
        XGetSoundManager()->StartSound(0x1af0015c);
        GoState(0x16);
        return 0;
    }
    case 9: {
        if (!s_RitualGrid->Listener(0x5f5f5f5f5f76616eull)) return 0;
        if (!s_RitualGrid->Listener(0x5f5f5f5f5f76616eull)->m_pFocus) return 0;
        unsigned int bit = (unsigned int)s_RitualGrid->Listener(0x5f5f5f5f5f76616eull)->m_pFocus->m_Data;
        if (XGetGameValues()->RitualUnlocked(bit))
            SetInfoObjText("~X Times", 0);
        else
            SetInfoObjText(0, 0);
        return 0;
    }
    case 0x17: {
        if (!s_RitualGrid->Listener(0x5f5f5f5f5f76616eull)) return 0;
        if (!s_RitualGrid->Listener(0x5f5f5f5f5f76616eull)->m_pFocus) return 0;
        unsigned int bit = (unsigned int)s_RitualGrid->Listener(0x5f5f5f5f5f76616eull)->m_pFocus->m_Data;
        if (!XGetGameValues()->RitualUnlocked(bit)) return 0;
        unsigned int level = FELevelManager::Instance(1)->RitualLevelHashFromBitIndex(bit);
        XGetGameValues()->m_Unkc = level;
        unsigned int slot = MemCardSlot();
        m_DoneState = 2;
        m_DoneAction = slot;
        GoState(0x17);
        return 1;
    }
    }
}

static FELayoutScrollGrid *s_CostumeGrid;
static short s_CostumeSkin;
static unsigned int s_CostumeFocus;

signed char FEMainMenu::UpdateStateCostumeSelect(FEObjEvent::Flags ev)
{
    switch (ev) {
    case 0x1e: {
        static float widths[2] = { 0.5f, 0.5f };
        static unsigned int count = 1;
        unsigned int mask = GameData::Instance(1)->m_Values.m_Unk24[4] | 1;
        count = 7;
        s_CostumeFocus = (short)GameData::Instance(1)->m_Values.m_Unk58 != -1 ? (short)GameData::Instance(1)->m_Values.m_Unk58 : 0;
        s_CostumeSkin = (short)s_CostumeFocus;
        FELayoutScrollGrid *pGrid = (FELayoutScrollGrid *)m_pLayout->ObjectAdd(m_ListenerIdx, new FELayoutScrollGrid(count, 2, 0xa, 0x85, 0x154, 0xe6, (EXW::TextAlign)0));
        s_CostumeGrid = pGrid;
        pGrid->SetGridWidthProportions(widths);
        pGrid->m_Data = 0xfdffffffull;
        pGrid->StateSub(8);
        FEObject *pFocusObj = 0;
        const EXFixedArray<unsigned int, 7> rows = { { 0, 3, 5, 1, 2, 4, 6 } };
        for (unsigned int i = 0; i < 7; i++) {
            EXString name;
            EXString place;
            int skin;
            switch (i) {
            case 1:
                name = "Alien Hunter";
                skin = 6;
                place = "Antarctica 2004";
                break;
            case 2:
                name = "City Hunter";
                skin = 4;
                place = "Los Angeles 1997";
                break;
            case 3:
                name = "Jungle Hunter";
                skin = 2;
                place = "Central America 1987";
                break;
            case 4:
                name = "Ritual Armor";
                skin = 0xa;
                place = "Neonopolis 2030";
                break;
            case 5:
                name = "Dark Blade Clan";
                skin = 8;
                place = "Neonopolis 2030";
                break;
            case 6:
                name = "Bad Blood";
                skin = 0xc;
                place = "Neonopolis 2030";
                break;
            default:
                name = "Concrete Jungle";
                skin = 0;
                place = "Neonopolis 2030";
                break;
            }
            FEObject *pName = pGrid->ObjectAdd(rows[i], 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 0xfdffffffull, name, FE_Blank,
                                                                        (EXW::TextAlign)2, 0, 0, 0, 0, (EXW::TextAlign)4));
            pName->StateAdd(8);
            pName->m_Data = skin;
            FEObject *pPlace = pGrid->ObjectAdd(rows[i], 1, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x700000a, 0xfdffffffull, place, FE_Blank,
                                                                         (EXW::TextAlign)2, 0, 0, 0, 0, (EXW::TextAlign)4));
            if (!(mask & (1 << i))) {
                pName->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0x40, 0x40, 0x40, 0x6e), 1);
                pPlace->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0x40, 0x40, 0x40, 0x6e), 1);
                pName->m_Data = (unsigned __int64)-1;
                pName->SetHelpText(EXString(" "));
            } else {
                pName->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 1);
                pPlace->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0x65, 0x65, 0x65, 0x80), 1);
                pName->SetHelpText(gText.Get(0x51000476));
            }
            if (pName->m_Data == s_CostumeFocus) pFocusObj = pName;
        }
        pGrid->SetVisibleCount(7, 2);
        s_CostumeGrid->Inflate(-10, -10);
        pGrid->EnsureVisible(pFocusObj, 0);
        s_CostumeGrid->ListenerAdd(new FEListener(0x5f5f5f5f5f76616eull, 0x401, 0, (FEPad::Flags)0xf));
        FEListener *pl = s_CostumeGrid->Listener(0x5f5f5f5f5f76616eull);
        SetUpdateListener(pl);
        s_CostumeGrid->SetMathmos((FEObjColour::Flags)0,
                                  FEMathmos(ex::RGBA(0x7f, 0, 0, 0x80), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 60.0f, FEWaveform::WF_LINEAR, 0), 1);
        s_CostumeGrid->SetMathmos((FEObjColour::Flags)1, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80)), 1);
        return 0;
    }
    case 0x23: {
        SetTitleAndBackgr("Costume Select", s_CostumeGrid->Rect());
        FEListener *pL = m_UpdateListeners[m_ListenerIdx];
        if (!pL) return 0;
        if (!pL->m_pFocus) return 0;
        if ((int)pL->m_pFocus->m_Data != m_PlayerSkin) return 0;
        m_pHelpText->SetText(EXString(" "), 0, 0);
        return 0;
    }
    case 0x1f:
        s_CostumeFocus = (short)GameData::Instance(1)->m_Values.m_Unk58 != -1 ? (short)GameData::Instance(1)->m_Values.m_Unk58 : 0;
        s_CostumeGrid->Listener(0x5f5f5f5f5f76616eull)->SetFocusByData(s_CostumeFocus, 0);
        s_CostumeGrid->Listener(0x5f5f5f5f5f76616eull)->SubListenFlags(0x401);
        return 0;
    case 9: {
        FEListener *pL = s_CostumeGrid->Listener(0x5f5f5f5f5f76616eull);
        if (!pL) return 0;
        if (!pL->m_pFocus) return 0;
        if ((int)pL->m_pFocus->m_Data != m_PlayerSkin) return 0;
        m_pHelpText->SetText(EXString(" "), 0, 0);
        return 0;
    }
    case 0x15: {
        short sel = XItemHandler_Player::Instance()->GetSelectedSkin();
        if (m_PredSkinUpdateState && m_PredSkinUpdateState < 0x1e) {
            s_CostumeGrid->Listener(0x5f5f5f5f5f76616eull)->SubListenFlags(0x401);
            FEListener *pL = m_UpdateListeners[m_ListenerIdx];
            if (pL->m_pFocus) pL->SetFocus(0, 0);
            m_pHelpText->SetText(EXString(" "), 0, 0);
            return 0;
        }
        s_CostumeGrid->Listener(0x5f5f5f5f5f76616eull)->AddListenFlags(0x401);
        FEListener *pL = m_UpdateListeners[m_ListenerIdx];
        if (!pL->m_pFocus) {
            pL->SetFocusByData((unsigned int)m_PlayerSkin, 0);
            m_pHelpText->SetText(EXString(" "), 0, 0);
        } else if ((int)pL->m_pFocus->m_Data == -1) {
            m_pHelpText->SetText(EXString(" "), 0, 0);
        } else if ((int)pL->m_pFocus->m_Data == m_PlayerSkin) {
            m_pHelpText->SetText(EXString(" "), 0, 0);
        } else {
            m_pHelpText->SetText(Text_Get(0x51000476), 0, 0);
        }
        if (s_CostumeSkin != sel && s_CostumeSkin != m_PlayerSkin) {
            SetPlayerSkin(s_CostumeSkin);
            XGetGameValues()->m_Unk58 = s_CostumeSkin;
        }
        return 0;
    }
    case 0x17: {
        FEListener *pL = m_UpdateListeners[m_ListenerIdx];
        if (!pL) return 0;
        if (!pL->m_pFocus) return 0;
        short s = (short)pL->m_pFocus->m_Data;
        if (s == -1) return 0;
        if (s_CostumeSkin != s) XGetSoundManager()->StartSound(0x1af0015c);
        s_CostumeSkin = s;
        return 0;
    }
    }
    return 0;
}

static FEObject *s_OptionsCameraPage;

signed char FEMainMenu::UpdateStateOptionsCamera(FEObjEvent::Flags ev)
{
    FELayout *pCur = m_pCurLayout;
    FEObject *pObj = pCur ? pCur->Object(1) : 0;
    switch (ev) {
    case 0x1e: {
        static float widths[2] = { 0.6f, 0.4f };
        s_OptionsCameraPage = m_pLayout->ObjectAdd(m_ListenerIdx, new FELayoutLayered(2, 0, 0, 0, 0, (EXW::TextAlign)4));
        FELayoutScrollGrid *pGrid = new FELayoutScrollGrid(7, 2, 0x20, 0x91, 0x1d0, 0xf0, (EXW::TextAlign)0);
        s_OptionsCameraPage->ObjectAdd(1, pGrid);
        pGrid->SetGridWidthProportions(widths);
        pGrid->SetUpDownScrollArrowForceY(0);
        pGrid->SetVisibleCount(7, 2);
        FEObject *pText = AddScrollGridText(pGrid, 0, 0, 0x5f5f5f58766e4974ull, Text_Get(0x510001f8), (EXW::TextAlign)2, EXString(""), 1);
        FEObject *pCheck = pGrid->ObjectAdd(0, 1, new FELayoutCheckbox(1, 0, 0, 0, 0, (EXW::TextAlign)4));
        pCheck->m_Data = 0x5f5f5f5f58766e49ull;
        pCheck->Bind((FEDataType::Flags)1, &GameData::Instance(1)->Values()->m_bInvertX3rd);
        pCheck->SetHelpText(Text_Get(0x510004d7));
        pText->MathmosLink(pCheck);
        pText = AddScrollGridText(pGrid, 1, 0, 0x5f5f5f59766e4974ull, Text_Get(0x510001f9), (EXW::TextAlign)2, EXString(""), 1);
        pCheck = pGrid->ObjectAdd(1, 1, new FELayoutCheckbox(1, 0, 0, 0, 0, (EXW::TextAlign)4));
        pCheck->m_Data = 0x5f5f5f5f59766e49ull;
        pCheck->Bind((FEDataType::Flags)1, &GameData::Instance(1)->Values()->m_bInvertY3rd);
        pCheck->SetHelpText(Text_Get(0x510004d7));
        pText->MathmosLink(pCheck);
        pText = AddScrollGridText(pGrid, 2, 0, 0x5f585046766e4974ull, Text_Get(0x51000470), (EXW::TextAlign)2, EXString(""), 1);
        pCheck = pGrid->ObjectAdd(2, 1, new FELayoutCheckbox(0, 0, 0, 0, 0, (EXW::TextAlign)4));
        pCheck->m_Data = 0x5f5f585046766e49ull;
        pCheck->Bind((FEDataType::Flags)1, &GameData::Instance(1)->Values()->m_bInvertX1st);
        pCheck->SetHelpText(Text_Get(0x510004d7));
        pText->MathmosLink(pCheck);
        pText = AddScrollGridText(pGrid, 3, 0, 0x5f595046766e4974ull, Text_Get(0x51000471), (EXW::TextAlign)2, EXString(""), 1);
        pCheck = pGrid->ObjectAdd(3, 1, new FELayoutCheckbox(0, 0, 0, 0, 0, (EXW::TextAlign)4));
        pCheck->m_Data = 0x5f5f595046766e49ull;
        pCheck->Bind((FEDataType::Flags)1, &GameData::Instance(1)->Values()->m_bInvertY1st);
        pCheck->SetHelpText(Text_Get(0x510004d7));
        pText->MathmosLink(pCheck);
        pText = AddScrollGridText(pGrid, 4, 0, 0x68746c6165745374ull, EXString("Action Camera"), (EXW::TextAlign)2, EXString(""), 1);
        pCheck = pGrid->ObjectAdd(4, 1, new FELayoutCheckbox(0, 0, 0, 0, 0, (EXW::TextAlign)4));
        pCheck->m_Data = 0x5f68746c61657453ull;
        pCheck->Bind((FEDataType::Flags)1, &GameData::Instance(1)->Values()->m_bActionCamera);
        pCheck->SetHelpText(Text_Get(0x510004d7));
        pText->MathmosLink(pCheck);
        pText = AddScrollGridText(pGrid, 5, 0, 0x656361466d614374ull, Text_Get(0x510005e2), (EXW::TextAlign)2, EXString(""), 1);
        FEObject *pSub = pGrid->ObjectAdd(5, 1, new FELayoutScrollGrid(1, 2, 0, 0, 0, 0, (EXW::TextAlign)4));
        pSub->m_Data = 0x5f656361466d6143ull;
        pText->MathmosLink(pSub);
        pSub->ObjectAdd(0, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 0, Text_Get(0x510006b9), FE_Blank,
                                           (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4))
            ->SetHelpText(Text_Get(0x51000474))->m_AltData = 0x41656361466d6143ull;
        pSub->ObjectAdd(0, 1, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 1, Text_Get(0x510006b8), FE_Blank,
                                           (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4))
            ->SetHelpText(Text_Get(0x51000474))->m_AltData = 0x42656361466d6143ull;
        pSub->SetVisibleCount(1, 1);
        pSub->Bind((FEDataType::Flags)1, &GameData::Instance(1)->Values()->m_FirstPersonView);
        pText = AddScrollGridText(pGrid, 6, 0, 0x65646f4d6d614374ull, Text_Get(0x51000204), (EXW::TextAlign)2, EXString(""), 1);
        pSub = pGrid->ObjectAdd(6, 1, new FELayoutScrollGrid(1, 3, 0, 0, 0, 0, (EXW::TextAlign)4));
        pSub->m_Data = 0x5f65646f4d6d6143ull;
        pText->MathmosLink(pSub);
        pSub->ObjectAdd(0, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 0, Text_Get(0x51000207), FE_Blank,
                                           (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4))
            ->SetHelpText(Text_Get(0x51000474));
        pSub->ObjectAdd(0, 1, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 1, Text_Get(0x51000208), FE_Blank,
                                           (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4))
            ->SetHelpText(Text_Get(0x51000474));
        pSub->ObjectAdd(0, 2, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 2, Text_Get(0x510002c8), FE_Blank,
                                           (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4))
            ->SetHelpText(Text_Get(0x51000474));
        pSub->SetVisibleCount(1, 1);
        pSub->Bind((FEDataType::Flags)1, &GameData::Instance(1)->Values()->m_CameraMode);
        pGrid->ListenerAdd(new FEListener(0x5f5f5f5f5f76616eull, 0xc03, 0, (FEPad::Flags)0xf));
        FEListener *pl = pGrid->Listener(0x5f5f5f5f5f76616eull);
        SetUpdateListener(pl);
        pGrid->Listener(0x5f5f5f5f5f76616eull)->SetFocusByData(0x5f5f5f5f58766e49ull, 0);
        pGrid->SetMathmos((FEObjColour::Flags)0,
                          FEMathmos(ex::RGBA(0x7f, 0, 0, 0x80), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 60.0f, FEWaveform::WF_LINEAR, 0), 1);
        pGrid->SetMathmos((FEObjColour::Flags)1, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80)), 1);
        return 0;
    }
    case 0x23:
        SetTitleAndBackgr("Camera Options", pObj->Rect());
        return 0;
    case 0x1f:
        pObj->Listener(0x5f5f5f5f5f76616eull)->SetFocusByData(0x5f5f5f5f58766e49ull, 0);
        return 0;
    case 9: {
        FEListener *pL = m_UpdateListeners[m_ListenerIdx];
        if (!pL) break;
        if (!pL->m_pFocus) break;
        unsigned int help = 0;
        if (pL->m_pFocus->m_Data == 0x5f68746c61657453ull)
            help = 0x5100083a;
        else if (pL->m_pFocus->m_AltData == 0x41656361466d6143ull || pL->m_pFocus->m_AltData == 0x42656361466d6143ull)
            help = 0x51000839;
        if (help)
            ms_pInstance->SetInfoObjText("~X Help", help);
        else
            ms_pInstance->SetInfoObjText(0, 0);
        return 0;
    }
    case 0x19: {
        FEListener *pL = m_UpdateListeners[m_ListenerIdx];
        if (!pL) break;
        FELayoutCheckbox *pCB = (FELayoutCheckbox *)pL->m_pFocus;
        if (!pCB) break;
        if (pCB->m_Data != 0x5f68746c61657453ull) break;
        if (MemCardDlgMessageHash() != 0x5100083a) break;
        pCB->SetValue(!pCB->Value());
        pCB->BindUpdate(0xfeffffff);
        break;
    }
    }
    return 0;
}

signed char FEMainMenu::UpdateStateOptionsController(FEObjEvent::Flags ev)
{
    FELayout *pCur = m_pCurLayout;
    FEObject *pObj = pCur ? pCur->Object(2) : 0;
    static EXPoint s_Origin;
    static signed char s_bOriginSet;
    static FEObject *s_pPad;
    static unsigned int s_PadHash;
    switch (ev) {
    case 0x1e: {
        static float widths[2] = { 0.5f, 0.5f };
        FEObject *pPage = m_pLayout->ObjectAdd(m_ListenerIdx, new FELayoutLayered(4, 0, 0, 0, 0, (EXW::TextAlign)4));
        FEObject *pBack = pPage->ObjectAdd(0, new FEObject(FEObjType::OT_SPRITE, XApp::Frontend2DFile(), 0x6000234, 0xfdffffffull, FE_Blank, FE_Blank,
                                                          (EXW::TextAlign)0, 0, 0, 0, 0, (EXW::TextAlign)4));
        pBack->SetRect(EXRect(0x48, 0xc2, 0x1ea, 0xd7));
        pBack->StateAdd(0x302);
        pBack->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0x23, 0x23, 0x28, 0x50), 1);
        pBack->StateSub(8);
        FELayoutScrollGrid *pGrid = new FELayoutScrollGrid(3, 2, 0x48, 0x3c, 0x1a8, 0x4c, (EXW::TextAlign)0);
        pPage->ObjectAdd(2, pGrid);
        pBack = pPage->ObjectAdd(1, new FEObject(FEObjType::OT_SPRITE, XApp::Frontend2DFile(), 0x6000234, 0xfdffffffull, FE_Blank, FE_Blank,
                                                (EXW::TextAlign)0, 0, 0, 0, 0, (EXW::TextAlign)4));
        EXRect sf = EXBaseDisplay::GetDisplay()->SafeFrame();
        pBack->SetRect(EXRect(sf.x + 0x48, sf.y + 0x3c, 0x1a8, 0x4c));
        pBack->StateAdd(0x302);
        pBack->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0x23, 0x23, 0x28, 0x50), 1);
        pBack->StateSub(8);
        pGrid->SetGridWidthProportions(widths);
        pGrid->SetUpDownScrollArrowForceY(0);
        pGrid->SetVisibleCount(3, 2);
        pGrid->StateSub(8);
        FEObject *pText = AddScrollGridText(pGrid, 0, 0, 0x7974536c72744374ull, Text_Get(0x510005e3), (EXW::TextAlign)2, EXString(""), 1);
        FEObject *pSub = pGrid->ObjectAdd(0, 1, new FELayoutScrollGrid(1, 5, 0, 0, 0, 0, (EXW::TextAlign)4));
        pSub->m_Data = 0x5f7974536c727443ull;
        pText->MathmosLink(pSub);
        FEObject *pFocusObj = 0;
        for (unsigned int i = 0; i < ((XGamePad *)EXBaseGamePad::GetGamePad())->NumConfigs(); i++) {
            EXString name;
            unsigned int cfg;
            switch (i) {
            case 1:
                name = Text_Get(0x510001ff);
                cfg = 1;
                break;
            case 2:
                name = Text_Get(0x51000335);
                cfg = 2;
                break;
            case 3:
                name = Text_Get(0x510003e8);
                cfg = 3;
                break;
            case 4:
                name = Text_Get(0x510003e9);
                cfg = 4;
                break;
            case 5:
                name = Text_Get(0x51000497);
                cfg = 5;
                break;
            case 6:
                name = Text_Get(0x51000499);
                cfg = 6;
                break;
            case 7:
                name = Text_Get(0x5100049b);
                cfg = 7;
                break;
            case 8:
                name = Text_Get(0x5100049d);
                cfg = 8;
                break;
            case 9:
                name = Text_Get(0x5100049f);
                cfg = 9;
                break;
            default:
                name = Text_Get(0x510001fe);
                cfg = 0;
                break;
            }
            FEObject *pItem = pSub->ObjectAdd(0, i, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, cfg, name, FE_Blank,
                                                                (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4))
                                  ->SetHelpText(Text_Get(0x51000474));
            if (cfg == GameData::Instance(1)->Values()->m_ControllerConfig) pFocusObj = pItem;
            ((XGamePad *)EXBaseGamePad::GetGamePad())->SetActions(i);
        }
        pSub->SetVisibleCount(1, 1);
        pSub->Bind((FEDataType::Flags)3, &GameData::Instance(1)->Values()->m_ControllerConfig);
        pText->EnsureVisible(pFocusObj, 0);
        pText = AddScrollGridText(pGrid, 1, 0, 0x6574617262695674ull, Text_Get(0x510001fd), (EXW::TextAlign)2, EXString(""), 1);
        FEObject *pCheck = pGrid->ObjectAdd(1, 1, new FELayoutCheckbox(0, 0, 0, 0, 0, (EXW::TextAlign)4));
        pCheck->m_Data = 0x5f65746172626956ull;
        pCheck->Bind((FEDataType::Flags)1, &GameData::Instance(1)->Values()->m_Unk1c[3]);
        pCheck->SetHelpText(Text_Get(0x510004d7));
        pText->MathmosLink(pCheck);
        pText = AddScrollGridText(pGrid, 2, 0, 0x5f6e61654c504674ull, Text_Get(0x510007fd), (EXW::TextAlign)2, EXString(""), 1);
        pSub = pGrid->ObjectAdd(2, 1, new FELayoutScrollGrid(1, 3, 0, 0, 0, 0, (EXW::TextAlign)4));
        pSub->m_Data = 0x5f5f6e61654c5046ull;
        pText->MathmosLink(pSub);
        pSub->ObjectAdd(0, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 0, EXString("Off"), FE_Blank,
                                           (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4))
            ->SetHelpText(Text_Get(0x51000474));
        pSub->ObjectAdd(0, 1, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 1, Text_Get(0x510007fe), FE_Blank,
                                           (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4))
            ->SetHelpText(Text_Get(0x51000474));
        pSub->ObjectAdd(0, 2, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 2, Text_Get(0x510007ff), FE_Blank,
                                           (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4))
            ->SetHelpText(Text_Get(0x51000474));
        pSub->SetVisibleCount(1, 1);
        pSub->Bind((FEDataType::Flags)2, &GameData::Instance(1)->Values()->m_ButtonConfig);
        int config = ((XGamePad *)EXBaseGamePad::GetGamePad())->GetActions();
        s_PadHash = 0x20001e3;
        FEObject *pPad = new FEObject(FEObjType::OT_SCRIPT, XApp::Frontend2DFile(), s_PadHash, 0xfdffffffull, FE_Blank, FE_Blank,
                                      (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4);
        s_pPad = pPad;
        pPad->Script(0)->SetScale(0.05f);
        pPad->StateSub(8);
        pPage->ObjectAdd(3, pPad);
        if (!s_bOriginSet) {
            s_Origin = pPad->FindDatum2D(0x100000ed);
            s_Origin = -s_Origin;
            s_Origin.x += EXBaseDisplay::GetDisplay()->Width() / 2;
            s_Origin.y += EXBaseDisplay::GetDisplay()->Height() / 2 + 0x35;
        }
        s_bOriginSet = 1;
        pPad->SetOrigin(s_Origin, (EXW::TextAlign)0);
        pPad->StateAdd(0x302);
        ((XGamePad *)EXBaseGamePad::GetGamePad())->SetActions(config);
        return 0;
    }
    case 0x23:
        SetTitleAndBackgr(0, EXRect(0, 0, 0, 0));
    case 0x15:
        if (m_b1) {
            ((XGamePad *)EXBaseGamePad::GetGamePad())->SetActions(GameData::Instance(1)->Values()->m_ControllerConfig);
            EXFixedArray<EXW::TextAlign, 18> alignB = { { (EXW::TextAlign)2, (EXW::TextAlign)2, (EXW::TextAlign)2, (EXW::TextAlign)2,
                                                          (EXW::TextAlign)7, (EXW::TextAlign)2, (EXW::TextAlign)7, (EXW::TextAlign)7,
                                                          (EXW::TextAlign)2, (EXW::TextAlign)2, (EXW::TextAlign)5, (EXW::TextAlign)5,
                                                          (EXW::TextAlign)7, (EXW::TextAlign)7, (EXW::TextAlign)7, (EXW::TextAlign)7,
                                                          (EXW::TextAlign)7, (EXW::TextAlign)2 } };
            EXFixedArray<EXW::TextAlign, 18> alignA = { { (EXW::TextAlign)2, (EXW::TextAlign)2, (EXW::TextAlign)2, (EXW::TextAlign)2,
                                                          (EXW::TextAlign)7, (EXW::TextAlign)2, (EXW::TextAlign)7, (EXW::TextAlign)2,
                                                          (EXW::TextAlign)2, (EXW::TextAlign)2, (EXW::TextAlign)7, (EXW::TextAlign)7,
                                                          (EXW::TextAlign)5, (EXW::TextAlign)7, (EXW::TextAlign)7, (EXW::TextAlign)7,
                                                          (EXW::TextAlign)7, (EXW::TextAlign)2 } };
            EXW::TextAlign *pAlign = s_PadHash == 0x20001e1 ? (EXW::TextAlign *)alignB : (EXW::TextAlign *)alignA;
            ex::RGBA c(0x7f, 0x7f, 0x7f, 0x80);
            unsigned int datum = 0x100000d7;
            for (unsigned int i = 0; i < 18; i++, datum++)
                s_pPad->DrawText((FERenderPass::Flags)3, 0x100003a, 0x700000a, c, 0, datum,
                                 ((XGamePad *)EXBaseGamePad::GetGamePad())->GetControllerString(datum), pAlign[i], 0);
        }
        return 0;
    case 0x1f: {
        pObj->ListenerAdd(new FEListener(0x5f5f5f5f5f76616eull, 0xc03, MainMenu_Callback, (FEPad::Flags)0xf));
        FEListener *pl = pObj->Listener(0x5f5f5f5f5f76616eull);
        SetUpdateListener(pl);
        pObj->Listener(0x5f5f5f5f5f76616eull)->SetFocusByData(GameData::Instance(1)->Values()->m_ControllerConfig, 0);
        pObj->SetMathmos((FEObjColour::Flags)0,
                         FEMathmos(ex::RGBA(0x7f, 0, 0, 0x80), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 60.0f, FEWaveform::WF_LINEAR, 0), 1);
        pObj->SetMathmos((FEObjColour::Flags)1, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80)), 1);
        return 0;
    }
    case 0x21:
        pObj->ListenerClear();
        break;
    }
    return 0;
}

static float s_EditWidths[13] = { 0.0625f, 0.0625f, 0.0625f, 0.0625f, 0.0625f, 0.0625f, 0.0625f, 0.125f,
                                  0.0625f, 0.0625f, 0.0625f, 0.0625f, 0.0f };
static FEObject *s_EditPage;
static FEObject *s_EditPred;
static FEObject *s_EditKeys;
static FEObject *s_EditNameObj;
static signed char s_bEditTyped;

signed char FEMainMenu::UpdateStateEditName(FEObjEvent::Flags ev)
{
    static signed char s_bCaps = 1;
    char str[32];
    str[1] = 0;
    unsigned int ch;
    switch (ev) {
    case 0x1e: {
        s_EditPage = m_pLayout->ObjectAdd(m_ListenerIdx, new FELayoutLayered(3, 0, 0, 0, 0, (EXW::TextAlign)4));
        s_EditPage->StateSub(8);
        FEObject *pPred = s_EditPage->ObjectAdd(0, new FEObject(FEObjType::OT_SCRIPT2, 0x1000192, 0x4000790, 0xfdffffffull, FE_Blank, FE_Blank,
                                                                (EXW::TextAlign)3, 0, 0, 0, 0, (EXW::TextAlign)4));
        s_EditPred = pPred;
        pPred->Script(0)->SetScale(1.8f);
        pPred->SetOrigin(EXPoint(0x2d, 0x14), (EXW::TextAlign)0);
        pPred->StateAdd(0x342);
        pPred->Script(0)->Play(0, 0);
        s_EditKeys = s_EditPage->ObjectAdd(1, new FELayoutScrollGrid(4, 13, 0x32, 0xdb, 0x1a0, 0x59, (EXW::TextAlign)0));
        FEObject *pKeys = s_EditKeys;
        float sum = 0.0f;
        for (unsigned int i = 0; i < 12; i++) sum += s_EditWidths[i];
        s_EditWidths[12] = 1.0f - sum;
        ((FELayoutScrollGrid *)pKeys)->SetGridWidthProportions(s_EditWidths);
        pKeys->SetVisibleCount(4, 13);
        s_bCaps = 0;
        ch = 'A';
        unsigned int r, c;
        for (r = 0; r < 4; r++) {
            for (c = 0; c < 7; c++) {
                if (r == 3) {
                    if (c == 5)
                        ch = '.';
                    else if (c == 6)
                        break;
                }
                str[0] = (char)ch;
                s_EditKeys->ObjectAdd(r, c, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, ch, EXString(str), FE_Blank,
                                                         (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4))
                    ->StateAdd(0xc000);
                ch++;
            }
        }
        ch = '1';
        for (r = 0; r < 4; r++) {
            for (c = 0; c < 3; c++) {
                if (r == 3) {
                    if (c != 0) break;
                    ch = '0';
                }
                str[0] = (char)ch;
                s_EditKeys->ObjectAdd(r, c + 8, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, ch, EXString(str), FE_Blank,
                                                             (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4))
                    ->StateAdd(0xc000);
                ch++;
            }
        }
        s_EditKeys->ObjectAdd(0, 0xc, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x700000a, 0x3e8, EXString("Caps Lock"), FE_Blank,
                                                   (EXW::TextAlign)2, 0, 0, 0, 0, (EXW::TextAlign)4))
            ->StateAdd(0xc000);
        s_EditKeys->ObjectAdd(1, 0xc, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x700000a, 0x3e9, EXString("Backspace"), FE_Blank,
                                                   (EXW::TextAlign)2, 0, 0, 0, 0, (EXW::TextAlign)4))
            ->StateAdd(0xc000);
        s_EditKeys->ObjectAdd(2, 0xc, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x700000a, 0x3ea, EXString("Clear"), FE_Blank,
                                                   (EXW::TextAlign)2, 0, 0, 0, 0, (EXW::TextAlign)4))
            ->StateAdd(0xc000);
        s_EditKeys->ObjectAdd(3, 0xc, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x700000a, 0x3eb, EXString("Enter"), FE_Blank,
                                                   (EXW::TextAlign)2, 0, 0, 0, 0, (EXW::TextAlign)4))
            ->StateAdd(0xc000);
        FEObject *pName = s_EditPage->ObjectAdd(2, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 0xfdffffffull, EXString(ms_NameEdit.GetData()), FE_Blank,
                                                                (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        s_EditNameObj = pName;
        pName->SetRect(EXRect(0x83, 0x167, 0xdc, 0x14));
        pName->StateSub(8);
        pName->StateAdd(0x302);
        s_EditKeys->ListenerAdd(new FEListener(0x5f5f5f5f5f76616eull, 0xc03, 0, (FEPad::Flags)0xf));
        FEListener *pl = s_EditKeys->Listener(0x5f5f5f5f5f76616eull);
        SetUpdateListener(pl);
        s_EditKeys->SetMathmos((FEObjColour::Flags)0,
                               FEMathmos(ex::RGBA(0x7f, 0, 0, 0x80), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 60.0f, FEWaveform::WF_LINEAR, 0), 1);
        s_EditKeys->SetMathmos((FEObjColour::Flags)1, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80)), 1);
        return 1;
    }
    case 0x23:
        SetTitleAndBackgr(0, EXRect(0, 0, 0, 0));
        m_pSlotsInfo->SetText(Text_Get(0x51000835), 0, 0);
        s_EditKeys->SetMathmos((FEObjColour::Flags)2, ex::RGBA(0, 0, 0, 0), 1);
        s_EditNameObj->SetMathmos((FEObjColour::Flags)2, ex::RGBA(0, 0, 0, 0), 1);
        s_EditKeys->StateSub(0x40);
        s_EditNameObj->StateSub(0x40);
        return 0;
    case 0x1f:
        s_EditKeys->Listener(0x5f5f5f5f5f76616eull)->SetFocus(0, 0);
        s_EditKeys->Listener(0x5f5f5f5f5f76616eull)->SetFocusByData(0x3eb, 0);
        s_bEditTyped = 0;
        s_EditKeys->StateSub(0x40);
        s_EditNameObj->StateSub(0x40);
        return 0;
    case 0x15:
        if (MemCardDlgIsOn()) {
            MemCardDlgUpdate(0);
            Layout(m_ListenerIdx)->StateSub(0x40);
            m_pBackground->StateSub(0x40);
        } else {
            Layout(m_ListenerIdx)->StateAdd(0x40);
            m_pBackground->StateAdd(0x40);
        }
        switch (m_pManager->m_PageSubstate) {
        case 0:
            SetSubstate(0x63);
            return 0;
        case 0x63:
            if (IsNewSubstate()) {
                XGetSoundManager()->StartSound(0x1af006aa);
            }
            m_pInfoObj->StateSub(0x40);
            m_pHelpText->StateSub(0x40);
            m_pSlotsInfo->StateSub(0x40);
            s_EditKeys->SetMathmos((FEObjColour::Flags)2, ex::RGBA(0, 0, 0, 0), 1);
            s_EditNameObj->SetMathmos((FEObjColour::Flags)2, ex::RGBA(0, 0, 0, 0), 1);
            s_EditKeys->StateSub(0x40);
            s_EditNameObj->StateSub(0x40);
            if (SubstateTime() > 0.15f) {
                s_EditKeys->StateAdd(0x40);
                s_EditNameObj->StateAdd(0x40);
                s_EditKeys->SetMathmos((FEObjColour::Flags)2,
                                       FEMathmos(ex::RGBA(0, 0, 0, 0), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 24.0f, FEWaveform::WF_LINEAR, 1), 1);
                s_EditNameObj->SetMathmos((FEObjColour::Flags)2,
                                          FEMathmos(ex::RGBA(0, 0, 0, 0), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 30.0f, FEWaveform::WF_LINEAR, 1), 1);
                SetSubstate(0x64);
            }
            return 0;
        case 0x64:
            m_pInfoObj->StateAdd(0x40);
            m_pInfoObj->SetText(EXString("~X Space"), 0, 0);
            m_pHelpText->StateAdd(0x40);
            m_pSlotsInfo->StateAdd(0x40);
            return 0;
        case 1:
            m_pInfoObj->StateSub(0x40);
            m_pHelpText->StateSub(0x40);
            m_pSlotsInfo->StateSub(0x40);
            if (IsNewSubstate()) {
                FEObject *pPred = s_EditPage->ObjectAdd(0, new FEObject(FEObjType::OT_SCRIPT2, 0x1000192, 0x400078f, 0xfdffffffull, FE_Blank, FE_Blank,
                                                                        (EXW::TextAlign)3, 0, 0, 0, 0, (EXW::TextAlign)4));
                s_EditPred = pPred;
                pPred->Script(0)->SetScale(1.8f);
                pPred->SetOrigin(EXPoint(0x2d, 0x14), (EXW::TextAlign)0);
                pPred->StateAdd(0x342);
                pPred->Script(0)->Play(0, 0);
                XGetSoundManager()->StartSound(0x1af006a9);
            }
            if (SubstateTime() > 0.25f) {
                s_EditKeys->SetMathmos((FEObjColour::Flags)2,
                                       FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), ex::RGBA(0, 0, 0, 0), 24.0f, FEWaveform::WF_LINEAR, 1), 1);
                s_EditNameObj->SetMathmos((FEObjColour::Flags)2,
                                          FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), ex::RGBA(0, 0, 0, 0), 30.0f, FEWaveform::WF_LINEAR, 1), 1);
                SetSubstate(2);
            }
            return 0;
        case 2:
            if (SubstateTime() > 0.25f) {
                s_EditPred->SetMathmos((FEObjColour::Flags)2,
                                       FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), ex::RGBA(0, 0, 0, 0), 24.0f, FEWaveform::WF_LINEAR, 1), 1);
                SetSubstate(3);
            }
            return 0;
        case 3:
            if (SubstateTime() > 0.25f) GoState(MemCardAutoSaveEnabled() ? 0x12 : ms_StatsState);
            return 0;
        }
        return 0;
    case 0x17: {
        if (m_pManager->m_PageSubstate != 0x64) return 1;
        FEObject *pFocus = m_UpdateListeners[m_ListenerIdx]->m_pFocus;
        if (!pFocus) return 1;
        ch = (unsigned int)pFocus->m_Data;
        goto handle;
    }
    case 3:
        if (m_pManager->m_PageSubstate != 0x64) return 0;
        ch = 0x20;
    handle: {
        signed char bBad = 0;
        unsigned int len = strlen(ms_NameEdit);
        switch (ch) {
        case 0x3e8: {
            s_bCaps = !s_bCaps;
            ch = s_bCaps ? 'a' : 'A';
            for (unsigned int r = 0; r < 4; r++) {
                for (unsigned int c = 0; c < 7; c++) {
                    if (r == 3 && c > 4) break;
                    str[0] = (char)ch;
                    s_EditKeys->Object(r, c)->SetText(EXString(str), 0, 0);
                    s_EditKeys->Object(r, c)->m_Data = ch;
                    ch++;
                }
            }
            break;
        }
        case 0x3e9:
            if (ms_NameEdit[0]) ms_NameEdit[len - 1] = 0;
            bBad = 1;
            break;
        case 0x3ea:
            ms_NameEdit[0] = 0;
            bBad = 1;
            break;
        case 0x3eb: {
            typedef char NameRec[16];
            NameRec *pNames = (NameRec *)GameData::Instance(1);
            strcpy(pNames[0x11b + ((short *)&ms_RitualTimeBeaten)[1] * 8 + (ms_RitualTimeBeaten & 0xffff)], &ms_NameEdit[0]);
            ms_RitualTimeBeaten = -1;
            SetSubstate(1);
            break;
        }
        default:
            if (!s_bEditTyped) {
                if (ch == 0x20) XGetSoundManager()->StartSound(0x1af0015c);
                ms_NameEdit[0] = (char)ch;
                ms_NameEdit[1] = 0;
                s_bEditTyped = 1;
            } else if (len > 7) {
                bBad = 1;
            } else {
                if (ch == 0x20) XGetSoundManager()->StartSound(0x1af0015c);
                ms_NameEdit[len] = (char)ch;
                ms_NameEdit[len + 1] = 0;
            }
            break;
        }
        s_EditNameObj->SetText(EXString(&ms_NameEdit[0]), 0, 0);
        if (!bBad) return 1;
        XGetSoundManager()->StartSound(0x1af00158);
        return 0;
    }
    case 0x22:
        m_pHelpText->StateAdd(0x40);
        m_pSlotsInfo->StateAdd(0x40);
        return 0;
    }
    return 0;
}

static unsigned int s_SaveReturnSubstate;
static short s_LoadedSkin;
static GameValues s_SlotBackup(0);
static int s_SlotBackupFound;
static unsigned int s_NextChapterQuery = 0x2000;

signed char FEMainMenu::UpdateStateNewGame(FEObjEvent::Flags ev)
{
    unsigned int page = m_ListenerIdx;
    unsigned int layout = 0;
    layout = SetSaveLoadLayouts(page);
    signed char bLoad = layout == 1;
    signed char bAuto = layout == 2;
    switch (ev) {
    case 0x1e: {
        FEObject *pRoot = m_pLayout->ObjectAdd(m_ListenerIdx, new FELayoutFixedGrid(1, 1, 0, 0, EXBaseDisplay::GetDisplay()->m_SafeFrame.w,
                                                                                  EXBaseDisplay::GetDisplay()->m_SafeFrame.h, (EXW::TextAlign)0));
        if (m_ListenerIdx == 0x12) {
            SetUpdateListener(0);
            return 0;
        }
        FEObject *pPage = pRoot->ObjectAdd(0, 0, new FELayoutLayered(3, 0, 0, 0, 0, (EXW::TextAlign)4));
        unsigned int x0 = 0x5a;
        if (m_ListenerIdx != 2) x0 = 0x1388;
        s_SaveLayout = pPage->ObjectAdd(0, new FELayoutFixedGrid(3, 1, x0, 0xb1, 0xb4, 0x86, (EXW::TextAlign)0));
        s_SaveLayout->ObjectAdd(0, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 0, Text_Get(0x510001bb), FE_Blank,
                                                   (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        s_SaveLayout->ObjectAdd(1, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 1, Text_Get(0x510001bc), FE_Blank,
                                                   (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        s_SaveLayout->ObjectAdd(2, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 2, Text_Get(0x510001bd), FE_Blank,
                                                   (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        FEObject *pLoad = pPage->ObjectAdd(2, new FELayoutLayered(0xc, 0, 0, 0, 0, (EXW::TextAlign)4));
        s_LoadLayout = pLoad;
        unsigned int w = ((unsigned int)EXBaseDisplay::GetDisplay()->m_SafeFrame.w - 0x38) / 2;
        ex::RGBA c(0x65, 0x65, 0x65, 0x80);
        unsigned int slot = 0;
        unsigned int y = 0x85;
        for (unsigned int r = 0; r < 3; r++, y += 0x54) {
            unsigned int x = 0x16;
            for (unsigned int col = 0; col < 2; col++, slot++, x += w + 0xc) {
                FEObject *pBox = pLoad->ObjectAdd(slot, new FEObject(FEObjType::OT_SPRITE, XApp::Frontend3DFile(), 0x600030d, 0xfdffffffull,
                                                                     FE_Blank, FE_Blank, (EXW::TextAlign)0, 0, 0, 0, 0, (EXW::TextAlign)4));
                pBox->SetRect(EXRect(EXBaseDisplay::GetDisplay()->m_SafeFrame.x + x - 10, EXBaseDisplay::GetDisplay()->m_SafeFrame.y + y - 10,
                                     w + 0x14, 0x5c));
                pBox->StateSub(8);
                pBox->StateAdd(0x302);
                FEObject *pGrid = pLoad->ObjectAdd(slot + 6, new FELayoutFixedGrid(1, 1, x, y, w, 0x48, (EXW::TextAlign)0));
                FEObject *pFlex = pGrid->ObjectAdd(0, 0, new FELayoutFlexGrid(2, 1, (EXW::TextAlign)4));
                char buf[16];
                sprintf(buf, "Predator %c\n", slot + 'A');
                FEObject *pName = new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, r + 100, EXString(buf), FE_Blank,
                                               (EXW::TextAlign)3, 0, 0, 0, 0, (EXW::TextAlign)4);
                pName->StateAdd(0x8000);
                pName->m_Data = slot + 100;
                pFlex->ObjectAdd(0, 0, pName)->LockTextRect(w, 0x18, 0, 0);
                FEObject *pInfo = pFlex->ObjectAdd(1, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x700000c, 0, EXString("Blah"), FE_Blank,
                                                                      (EXW::TextAlign)4, 0, 0, w, 0x30, (EXW::TextAlign)4));
                pInfo->StateSub(8);
                pInfo->StateAdd(0x20000);
                pInfo->SetMathmos((FEObjColour::Flags)3, c, 1);
            }
        }
        s_SaveLayouts[layout] = s_SaveLayout;
        s_LoadLayouts[layout] = s_LoadLayout;
        pRoot->ListenerAdd(new FEListener(0x5f5f5f5f5f76616eull, 0xc03, 0, (FEPad::Flags)0xf));
        FEListener *pl = pRoot->Listener(0x5f5f5f5f5f76616eull);
        SetUpdateListener(pl);
        pRoot->SetMathmos((FEObjColour::Flags)0,
                          FEMathmos(ex::RGBA(0x7f, 0, 0, 0x80), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 60.0f, FEWaveform::WF_LINEAR, 0), 1);
        pRoot->SetMathmos((FEObjColour::Flags)1, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80)), 1);
        return 0;
    }
    case 0x23:
        if (s_ChoosingSlots)
            SetTitleAndBackgr("Save Game", s_LoadLayout->Rect());
        else
            SetTitleAndBackgr("Difficulty Select", s_SaveLayout->Rect());
        return 0;
    case 0x1f:
        s_RememberAutoSave = MemCardAutoSaveEnabled();
        m_b4 = 1;
        SetChoosingSlots(0, 1);
        GameData::Instance(1)->Values()->ResetToDefaults(1);
        GameData::Instance(1)->GetBigSave()->SetStatusAndReset(0);
        ResetMemCardSaveFoundBits();
        SetSlotsText();
        MemCardEnableAutoSave(0);
        MemCardSetSlot(0);
        s_bSaveSlotChosen = 0;
        return 0;
    case 0x20:
        SetSubstate(0);
        return 0;
    case 0x15: {
        int r;
        switch (m_pManager->m_PageSubstate) {
        case 0:
            if (IsNewSubstate()) {
                SetChoosingSlots(0, 0);
                return 1;
            }
            return 1;
        case 1:
            if (IsNewSubstate()) {
                if (page == 2 && m_Unk82c == 4 && ReturnSubstate() != 0) {
                    Show_MemCardExtraDialog_Query(0x51000820, 0xd, 0xc, 1, 0, 0, -1, 0);
                } else {
                    SetChoosingSlots(0, 0);
                    s_LoadLayout->StateSub(0x40);
                    Show_MemCardExtraDialog_Query(0x51000963, 2, 4, 0, 3, 5, -1, 0);
                }
            }
            r = MemCardDlgUpdate(0);
            if (r) SetSubstate(r);
            return 1;
        case 16:
            Trace_Substate("State_SaveDone", 1);
            MemCardEnableAutoSave(1);
            if (m_ListenerIdx != 12) SET_INITIAL_LOADED_FOCUS();
            if (!bAuto) {
                RefreshPostLoadContent();
                GameControllerRemovedSys::ControllerSet(GameDialogWnd::LastPad());
                GoState(4);
                return 1;
            }
            SetSubstate(0x17);
            return 1;
        case 4:
            Trace_Substate("State_Done", 1);
            if (page == 2 && ReturnSubstate() == 1) ms_bDisableSaveGame = 1;
            if (m_ListenerIdx != 12) MemCardEnableAutoSave(0);
            if (m_ListenerIdx != 12) SET_INITIAL_LOADED_FOCUS();
            if (!bAuto) {
                RefreshPostLoadContent();
                GameControllerRemovedSys::ControllerSet(GameDialogWnd::LastPad());
                GoState(4);
                return 1;
            }
            SetSubstate(0x17);
            return 1;
        case 23: {
            int idx = FELevelManager::Instance(1)->IndexFromLevel(XGetGameValues()->m_Unkc);
            if (IsNewSubstate()) {
                SetInitialChapterSelectFocus();
                SetInitialRitualSelectFocus(0);
                if (!FELevelManager::Instance(1)->IsStory(MemCard_PrevLevelHash) || idx == 0 ||
                    (unsigned int)idx >= (unsigned int)FELevelManager::Instance(1)->StoryLevelCount()) {
                    GoState(ms_StatsState);
                    return 1;
                }
                Show_MemCardExtraDialog_Query(0x51000836, 0x17, ms_StatsState, 0, 0xa, 0, -1, &s_NextChapterQuery);
            }
            r = MemCardDlgUpdate(0);
            if (!r) return 1;
            if (r == 0x17) {
                m_DoneState = 2;
                m_DoneAction = MemCardSlot();
            }
            GoState(r);
            return 1;
        }
        case 5:
            Trace_Substate("State_Error", 1);
            if (page == 12) {
                GoState(4);
                return 1;
            }
            if (!bAuto) {
                GoState(1);
                return 1;
            }
            GoState(ms_StatsState);
            return 1;
        case 2:
            if (IsNewSubstate()) {
                s_bSaveSlotChosen = 0;
                SetChoosingSlots(1, 0);
                s_LoadLayout->StateSub(0x40);
                MemCard_CheckSavePossible(1);
            }
            switch (MemCard_CheckSavePossible(0)) {
            case -6:
                SetSubstate(1);
                return 1;
            case -5:
                GameData::Instance(1)->GetBigSave()->SetStatusAndReset(0);
                ResetMemCardSaveFoundBits();
                SetSubstate(2);
                return 1;
            case -3:
            case 1:
                SetSubstate(3);
                return 1;
            case -1:
                ms_bDisableSaveGame = 1;
                SetSubstate(4);
                return 1;
            case 0:
                return 1;
            default:
                SetSubstate(5);
                return 1;
            }
        case 3:
            if (IsNewSubstate()) {
                s_LoadLayout->StateSub(0x40);
                s_bSaveSlotChosen = 0;
                s_SaveReturnSubstate = ReturnSubstate();
                MemCard_SaveInit();
                MemCard_CheckSavesPresent(1, 0);
            }
            r = MemCard_CheckSavesPresent(0, 0);
            if (!bLoad) {
                switch (r) {
                case -10:
                case -4:
                    s_bSaveSlotChosen = 1;
                    if (m_ListenerIdx != 2) {
                        SetSubstate(0x13);
                        return 1;
                    }
                    if (m_Unk82c != 4) return 1;
                    SetSubstate(6);
                    return 1;
                case -9:
                    SetSubstate(0xa);
                    return 1;
                case -2:
                    SetSubstate(7);
                    return 1;
                case -1:
                    SetSubstate(8);
                    return 1;
                case 0:
                    return 1;
                case 1:
                    SetSubstate(6);
                    return 1;
                default:
                    SetSubstate(5);
                    return 1;
                }
            }
            switch (r) {
            case -10:
            case -4:
                SetSubstate(0x13);
                return 1;
            case -9:
            case -2:
                SetSubstate(7);
                return 1;
            case -1:
                SetSubstate(8);
                return 1;
            case 0:
                return 1;
            case 1:
                SetSubstate(6);
                return 1;
            default:
                SetSubstate(5);
                return 1;
            }
        case 6:
            if (IsNewSubstate()) {
                SetChoosingSlots(1, 0);
                SetSubstate(0xc);
            }
            return 1;
        case 7:
            if (IsNewSubstate()) {
                if (ReturnSubstate() == 0xd) {
                    Show_MemCardExtraDialog_Query(0x5100082a, 0xd, 5, 0, 8, 4, -1, 0);
                } else if (bLoad) {
                    Show_MemCardExtraDialog_Query(0x51000829, 5, 5, 0, 5, 0, -1, 0);
                } else {
                    SetSubstate(6);
                    return 1;
                }
            }
            r = MemCardDlgUpdate(0);
            if (r) {
                SetSubstate(r);
                return 1;
            }
            if (!MemCard_HasBeenRemoved()) return 1;
            GameData::Instance(1)->GetBigSave()->SetStatusAndReset(0);
            MemCardDlgHide(0);
            SetSlotsText();
            SetSubstate(bLoad ? 3 : ReturnSubstate());
            return 1;
        case 8:
            if (bAuto) {
                SetSubstate(0x15);
                return 1;
            }
            if (IsNewSubstate()) {
                if (!bLoad) {
                    SetSubstate(4);
                    return 1;
                }
                Show_MemCardExtraDialog_Query(0x51000826, ReturnSubstate(), 5, 0, 1, 0, -1, 0);
            }
            r = MemCardDlgUpdate(0);
            if (r) SetSubstate(r);
            return 1;
        case 9:
            if (bAuto) {
                SetSubstate(0x15);
                return 1;
            }
            if (IsNewSubstate()) Show_MemCardExtraDialog_Query(0x51000821, ReturnSubstate(), 5, 1, 9, 4, -1, 0);
            r = MemCardDlgUpdate(0);
            if (r) {
                SetSubstate(r);
                return 1;
            }
            if (!MemCard_HasBeenRemoved()) return 1;
            GameData::Instance(1)->GetBigSave()->SetStatusAndReset(0);
            MemCardDlgHide(0);
            SetSlotsText();
            SetSubstate(ReturnSubstate());
            return 1;
        case 10:
            Trace_Substate("State_Format", 1);
            ResetMemCardSaveFoundBits();
            SetChoosingSlots(1, 0);
            s_LoadLayout->StateSub(0x40);
            MemCardSetSlot(0);
            SetSubstate(6);
            return 1;
        case 12:
            Trace_Substate("State_Choose", 1);
            if (!MemCard_HasBeenRemoved()) return 1;
            if (bLoad) {
                GameData::Instance(1)->GetBigSave()->SetStatusAndReset(0);
                SetSubstate(3);
                return 1;
            }
            GameData::Instance(1)->GetBigSave()->SetStatusAndReset(0);
            SetSubstate(2);
            return 1;
        case 13:
            if (IsNewSubstate()) {
                if (!MemCard_bNotFormatted && !MemCardSavesFound() && ReturnSubstate() != 7 && ReturnSubstate() != 0xe &&
                    ReturnSubstate() != 0x14 &&
                    !((ms_pInstance->m_ListenerIdx == 2 || ms_pInstance->m_ListenerIdx == 0xc) && s_bSaveSlotChosen)) {
                    SetSubstate(7);
                    return 1;
                }
                if (m_ListenerIdx == 0xc || m_ListenerIdx == 2) {
                    BigSave *pBS = GameData::Instance(1)->GetBigSave();
                    s_SlotBackup = pBS->m_Saves[MemCardSlot()];
                    s_SlotBackupFound = pBS->m_SaveFound[MemCardSlot()];
                }
                MemCard_Save(1, 0);
            }
            switch (MemCard_Save(0, 0)) {
            case -8:
                SetSubstate(4);
                return 1;
            case -7:
            case -4:
                SetSubstate(0xe);
                return 1;
            case -6:
                if (bAuto) {
                    SetSubstate(0x10);
                    return 1;
                }
                if (m_ListenerIdx == 0xc || m_ListenerIdx == 2) {
                    BigSave *pBS = GameData::Instance(1)->GetBigSave();
                    pBS->m_Saves[MemCardSlot()] = s_SlotBackup;
                    pBS->m_SaveFound[MemCardSlot()] = s_SlotBackupFound;
                }
                if (m_ListenerIdx == 0xc) {
                    MemCardSetSlot(s_RememberSlot);
                    SetSubstate(s_RememberAutoSave ? 0x10 : 4);
                    return 1;
                }
                SetSubstate(MemCard_bNotFormatted ? 5 : 0xc);
                return 1;
            case -5:
                if (MemCard_bNotFormatted) {
                    SetSubstate(2);
                    return 1;
                }
                SetSubstate(0xd);
                return 1;
            case -3:
                SetSubstate(9);
                return 1;
            case -1:
                SetSubstate(8);
                return 1;
            case 0:
                return 1;
            case 1:
                SetSubstate(0xf);
                return 1;
            default:
                SetSubstate(5);
                return 1;
            }
        case 14:
            if (IsNewSubstate()) {
                Show_MemCardExtraDialog_Query(bAuto ? 0x51000833 : 0x51000832, 5, 0, 0, 0xb, 0, -1, 0);
                MemCardEnableAutoSave(0);
            }
            r = MemCardDlgUpdate(0);
            if (r) SetSubstate(r);
            return 1;
        case 15:
            if (IsNewSubstate()) {
                SetSlotsText();
                Show_MemCardExtraDialog_Query(m_bAutoSave ? 0x51000237 : 0x5100082f, 0x10, 0, 0, 5, 0, -1, 0);
                return 1;
            }
            r = MemCardDlgUpdate(0);
            if (r) SetSubstate(r);
            return 1;
        case 22:
            if (IsNewSubstate()) MemCard_CardChanged(1);
            r = MemCard_CardChanged(0);
            if (r == -5) {
                SetSubstate(m_SubstateArg[1]);
                return 1;
            }
            if (r == -1) {
                SetSubstate(m_SubstateArg[1]);
                return 1;
            }
            if (!r) return 1;
            SetSubstate(m_SubstateArg[0]);
            return 1;
        case 20:
            SetSubstate(0x16, 0xd, 0x15, 0x15);
            return 1;
        case 21:
            if (IsNewSubstate()) Show_MemCardExtraDialog_Query(0x51000834, 4, 0, 0, 0xc, 0, -1, 0);
            r = MemCardDlgUpdate(0);
            if (r) SetSubstate(r);
            return 1;
        case 17:
            if (IsNewSubstate()) {
                MemCard_LoadInit();
                MemCard_Load(1);
                s_LoadedSkin = XGetGameValues()->m_Unk58;
            }
            switch (MemCard_Load(0)) {
            case -4:
            case -2:
            case -1:
                SetSubstate(0x13);
                return 1;
            case 0:
                return 1;
            case 1:
                SetSubstate(0x12);
                return 1;
            default:
                SetSubstate(5);
                return 1;
            }
        case 18: {
            Trace_Substate("State_Loaded", 1);
            GameData::Instance(1)->m_Values.m_bGlobalValid = 1;
            GameData::Instance(1)->SettingsRestore(0);
            short skin = XGetGameValues()->m_Unk58;
            XItemHandler_Player::Instance()->SetSelectedSkin(s_LoadedSkin);
            XItemHandler_Player::Instance()->SetSelectedSkin(s_LoadedSkin);
            SetPlayerSkin(skin != (short)0xffff ? skin : 0);
            SET_INITIAL_LOADED_FOCUS();
            RefreshPostLoadContent();
            GameControllerRemovedSys::ControllerSet(GameDialogWnd::LastPad());
            GoState(4);
            return 1;
        }
        case 19:
            if (IsNewSubstate()) Show_MemCardExtraDialog_Query(0x51000828, 5, 5, 0, 0xb, 0, -1, 0);
            r = MemCardDlgUpdate(0);
            if (r) SetSubstate(r);
            return 1;
        default:
            Trace_Substate("State_Unknown", 1);
            return 1;
        }
    }
    case 9: {
        SetSlotsHighlights();
        if (m_pManager->m_PageSubstate != 0) return 0;
        FEListener *pL = m_UpdateListeners[m_ListenerIdx];
        if (!pL) return 0;
        if (!pL->m_pFocus) return 0;
        unsigned int help;
        switch ((int)pL->m_pFocus->m_Data) {
        case 0:
            help = 0x5100083d;
            break;
        case 1:
            help = 0x5100083e;
            break;
        case 2:
            help = 0x5100083f;
            break;
        default:
            ms_pInstance->SetInfoObjText(0, 0);
            return 0;
        }
        ms_pInstance->SetInfoObjText("~X Help", help);
        return 0;
    }
    case 0x17: {
        FEListener *pL = m_UpdateListeners[page];
        if (!pL) return 1;
        if (!pL->m_pFocus) return 1;
        int data = (int)pL->m_pFocus->m_Data;
        unsigned int sub = m_pManager->m_PageSubstate;
        if (sub == 0) {
            GameDialogWnd::SetLastPad(FEListener::LastPad());
            XGetSoundManager()->StartSound(0x1af0015c);
            XGetGameValues()->m_Unk1c[0] = (u8)data;
            SetSubstate(1);
            return 0;
        }
        if (sub != 0xc) return 1;
        GameDialogWnd::SetLastPad(FEListener::LastPad());
        XGetSoundManager()->StartSound(0x1af0015c);
        MemCardSetSlot(data - 100);
        if (!bLoad) {
            if (m_ListenerIdx == 2 && m_Unk82c == 4) {
                SetSubstate(1);
                return 0;
            }
            SetSubstate(0xd);
            return 0;
        }
        if (MemCardSavesFound() & (1 << MemCardSlot())) SetSubstate(0x11);
        return 0;
    }
    }
    return 0;
}

static unsigned int s_TrophyType = 1;
static int s_TrophyTextX = 0xe0;
static int s_TrophyTextY = 0x131;

// Lazer target offset on a weapon exhibit. Not in the map: always inlined.
static __forceinline void TrophyLazerOffset(XItemHandler_FETrophy *p, EXVector &v)
{
    if (p->m_Unk1c == 0xc9) {
        switch (p->m_Unk20) {
        case 5: v.y -= 0.175f; v.z -= 0.15f; break;
        case 6: v.y -= 0.12f; v.z -= 0.12f; break;
        case 7: v.y -= 0.2f; v.z -= 0.22f; break;
        case 8: v.z -= 0.12f; break;
        case 9: v.z -= 0.1f; break;
        case 10: v.z -= 0.14f; break;
        }
    }
}

// NEAR: 8248/8272. Same instructions; the frame is 0x10 smaller (retail keeps one more 16-byte home,
// around 0x7c), so several stack slots and disp8/disp32 encodings differ, and the FEMathmos temps
// of the create, substate 0 and substate 3 calls rotate.
signed char FEMainMenu::UpdateStateViewTrophies(FEObjEvent::Flags ev)
{
    static EXPoint s_GridSize(8, 5);
    static EXVector s_LazerPos(0.0f, 0.0f, 0.0f, 0.0f);
    static FEObject *s_pTrophyObj[2];
    static signed char s_bWeapons;
    static FEObject *s_pBackObj[2];
    static FEObject *s_pPage[2];
    static FEObject *s_pGrid[2];
    static FEObject *s_pText1[2];
    static FEObject *s_pText2[2];
    static FEObject *s_pText3[2];
    static struct { int x, y; } s_Text2Size;
    static struct { int x, y; } s_Text3Size;
    static unsigned int s_Current;
    static unsigned int s_Event;
    static unsigned int s_Target;

    signed char bWeapons = m_ListenerIdx == 9;
    int idx = !bWeapons;
    FEObject *pBackObj = s_pBackObj[idx];
    s_bWeapons = bWeapons;
    XItemHandler_FETrophy::ms_bWeaponsView = bWeapons;
    FEObject *pTrophyObj = s_pTrophyObj[idx];
    FEObject *pPage = s_pPage[idx];
    FEObject *pGrid = s_pGrid[idx];
    FEObject *pText2 = s_pText2[idx];
    FEObject *pText1 = s_pText1[idx];
    FEObject *pText3 = s_pText3[idx];

    switch (ev) {
    case 0x1e: {
        pPage = m_pLayout->ObjectAdd(m_ListenerIdx, new FELayoutLayered(6, 0, 0, EXBaseDisplay::GetDisplay()->SafeFrame().w,
                                                                        EXBaseDisplay::GetDisplay()->SafeFrame().Height(), (EXW::TextAlign)0));
        s_pPage[idx] = pPage;
        pGrid = pPage->ObjectAdd(0, new FELayoutFixedGrid(1, 1, (unsigned int)-1000, (unsigned int)-1000, 0xb4, 0x86, (EXW::TextAlign)4));
        s_pGrid[idx] = pGrid;
        FEObject *pSoon = pGrid->ObjectAdd(0, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 0x5f5f5f5f74786554ull, EXString("Soon..."),
                                                              FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        pSoon->SetHelpText(EXString(" "));
        pGrid->ListenerAdd(new FEListener(0x5f5f5f5f5f76616eull, 0xc03, MainMenu_Callback, (FEPad::Flags)0xf));
        FEListener *pl = pGrid->Listener(0x5f5f5f5f5f76616eull);
        SetUpdateListener(pl);
        pGrid->SetMathmos((FEObjColour::Flags)0,
                          FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 60.0f, (FEWaveform::Flags)1, 0), 1);
        pGrid->SetMathmos((FEObjColour::Flags)1, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80)), 1);
        pBackObj = new FEObject(FEObjType::OT_SCRIPT, XApp::Frontend3DFile(), 0x200026c, 0xfdffffff, FE_Blank, FE_Blank,
                                (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4);
        s_pBackObj[idx] = pBackObj;
        pBackObj->Script(0)->SetScale(1.3f);
        pPage->ObjectAdd(1, pBackObj);
        pBackObj->SetOrigin(EXPoint(0x32, 0x24), (EXW::TextAlign)0);
        pBackObj->StateAdd(0x302);
        pTrophyObj = new FEObject(FEObjType::OT_SCRIPT, XApp::Frontend3DFile(), 0x200026f, 0xfdffffff, FE_Blank, FE_Blank,
                                  (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4);
        s_pTrophyObj[idx] = pTrophyObj;
        pTrophyObj->StateSub(8);
        pPage->ObjectAdd(2, pTrophyObj);
        pTrophyObj->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0, 0, 0, 0), 1);
        FEObject *pFlex = pPage->ObjectAdd(3, new FELayoutFlexGrid(1, 1, (EXW::TextAlign)4));
        pFlex->SetOrigin(EXPoint(0x1bf, 0x103), (EXW::TextAlign)0);
        pFlex->StateAdd(0x200);
        pText1 = pFlex->ObjectAdd(0, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x700000c, 0xfdffffff, EXString("Blah!\netc..."), FE_Blank,
                                                     (EXW::TextAlign)0, 0, 0, 0, 0, (EXW::TextAlign)4))
                     ->LockTextRect(s_TrophyTextX, s_TrophyTextY, 0, 0);
        s_pText1[idx] = pText1;
        pText1->StateSub(8);
        pText1->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0, 0, 0, 0), 1);
        pFlex = pPage->ObjectAdd(4, new FELayoutFlexGrid(1, 1, (EXW::TextAlign)4));
        pFlex->SetOrigin(EXPoint(0x1bf, 0x4f), (EXW::TextAlign)0);
        pFlex->StateAdd(0x200);
        s_Text2Size.y = s_TrophyTextX + 8;
        s_Text2Size.x = 0x1e;
        pText2 = pFlex->ObjectAdd(0, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 0xfdffffff, EXString("Text2"), FE_Blank,
                                                     (EXW::TextAlign)4, 0, 0, s_Text2Size.y, s_Text2Size.x, (EXW::TextAlign)4));
        s_pText2[idx] = pText2;
        pText2->StateSub(8);
        pText2->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0, 0, 0, 0), 1);
        pFlex = pPage->ObjectAdd(5, new FELayoutFlexGrid(1, 1, (EXW::TextAlign)4));
        pFlex->SetOrigin(EXPoint(0xc4, 0x191), (EXW::TextAlign)0);
        pFlex->StateAdd(0x200);
        s_Text3Size.y = s_TrophyTextX + 8;
        s_Text3Size.x = 0x1e;
        pText3 = pFlex->ObjectAdd(0, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x700000c, 0xfdffffff, EXString("Text3"), FE_Blank,
                                                     (EXW::TextAlign)4, 0, 0, s_Text3Size.y, s_Text3Size.x, (EXW::TextAlign)4));
        s_pText3[idx] = pText3;
        pText3->StateSub(8);
        pText3->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0, 0, 0, 0), 1);
        pLazerItem = XItemHandler_FETrophy::Object(1000)->GetItem();
        pLazerItem2 = XItemHandler_FETrophy::Object(1001)->GetItem();
        return 0;
    }
    case 0x23:
        SetTitleAndBackgr(0, pGrid->Rect());
        m_pSlotsBackground->StateSub(0x40);
        return 0;
    case 0x1f: {
        pGrid->Listener(0x5f5f5f5f5f76616eull)->SetFocus(pBackObj, 0);
        SetSubstate(0);
        pBackObj->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0, 0, 0, 0), 1);
        s_Current = 0;
        s_Event = 0;
        s_TrophyType = 1;
        unsigned int mask1 = 0;
        unsigned int mask2 = GameData::Instance(1)->m_Values.m_Rewards >> 1;
        unsigned int i;
        for (i = 0; i < (unsigned int)FEGetLevels(1)->StoryLevelCount(); i++) {
            if (GameData::Instance(1)->m_Values.m_LevelStats[i].m_Unk0[0]) {
                unsigned int bits = 0;
                switch (i) {
                case 2: bits = 1; break;
                case 4: bits = 0xe; break;
                case 6: bits = 0x10; break;
                case 9: bits = 0x20; break;
                case 12: bits = 0x3c0; break;
                case 15: bits = 0x400; break;
                case 18: bits = 0x5000; break;
                case 22: bits = 0x2000; break;
                case 26: bits = 0x800; break;
                }
                mask1 |= bits;
            }
        }
        for (i = 0; i < 15; i++) {
            XItemHandler_FETrophy *p = XItemHandler_FETrophy::Find(i + 1);
            EXItem *pItem = p->GetItem();
            if (mask1 & (1 << i)) {
                pItem->m_renderInfo.EnableAlphaMult(0);
                if (!s_Current) {
                    s_TrophyType = p->m_Unk1c;
                    s_Current = p->m_Unk8;
                }
            } else {
                pItem->m_renderInfo.EnableAlphaMult(1);
                pItem->m_renderInfo.m_rgbaMult.set(1.0f, 1.0f, 1.0f, 0.25f);
            }
        }
        for (i = 0; i < 18; i++) {
            XItemHandler_FETrophy *p = XItemHandler_FETrophy::Find(i + 0x65);
            EXItem *pItem = p->GetItem();
            if ((1 << i) & mask2) {
                pItem->m_renderInfo.EnableAlphaMult(0);
                if (!s_Current) {
                    s_Current = p->m_Unk8;
                    s_TrophyType = p->m_Unk1c;
                }
            } else {
                pItem->m_renderInfo.EnableAlphaMult(1);
                pItem->m_renderInfo.m_rgbaMult.set(1.0f, 1.0f, 1.0f, 0.4f);
            }
        }
        if (s_bWeapons) s_Current = 0;
        for (i = 0; i < 0x17; i++) {
            XItemHandler_FETrophy *p = XItemHandler_FETrophy::Find(i + 0xc9);
            int k = p->m_Unk3c;
            if (k >= 8) k = 0;
            EXItem *pItem = p->GetItem();
            if ((i != 0x12 || GameData::Instance(1)->m_Values.m_LevelStats[10].m_Unk0[0]) &&
                (int)GameData::Instance(1)->m_Values.m_Unk38[k] >= p->m_Unk40) {
                pItem->m_renderInfo.EnableAlphaMult(0);
            } else {
                pItem->m_renderInfo.EnableAlphaMult(1);
                pItem->m_renderInfo.m_rgbaMult.set(1.0f, 1.0f, 1.0f, 0.0f);
            }
        }
        if (s_bWeapons) {
            XItemHandler_FETrophy *p = XItemHandler_FETrophy::Find(0xd7);
            s_Current = p->m_Unk8;
            s_TrophyType = p->m_Unk1c;
        }
        if (!s_Current) s_Current = s_bWeapons ? 0xc9 : 1;
        if (!s_Current) {
            LazerState = 0x65;
            pLazerItem->ClrHasRender();
            pLazerItem2->ClrHasRender();
        } else {
            LazerState = 0x64;
            pLazerItem->SetHasRender();
            pLazerItem2->SetHasRender();
            EXVector pos = XItemHandler_FETrophy::Find(s_Current)->GetItem()->Position();
            if (!s_bWeapons) pos.z -= 0.3f;
            pLazerItem->SetPosition(pos);
            pLazerItem2->SetPosition(pos);
            float a = s_bWeapons ? 1.5707964f : 0.0f;
            const EXVector &r1 = pLazerItem->Rotation();
            pLazerItem->SetRotation(EXQuaternion(r1.x, a, r1.z, r1.w));
            const EXVector &r2 = pLazerItem2->Rotation();
            pLazerItem2->SetRotation(EXQuaternion(r2.x, a, r2.z, r2.w));
        }
        s_GridSize = s_bWeapons ? EXPoint(10, 4) : EXPoint(8, 5);
        XItemHandler_FETrophy::ResetDirFudge();
        return 0;
    }
    case 0x20:
        gLastCamScript->FinishScript(XApp::Frontend3DFile(), bWeapons ? 0x4000610 : 0x400060f);
        return 0;
    case 0x15: {
        XItemHandler_FETrophy *pCur = XItemHandler_FETrophy::Object(s_Current);
        EXVector lr = pLazerItem->Rotation();
        float spin = XApp::FrameLength() * -8.0f;
        EXQuaternion lq(lr.x, lr.y, spin + lr.z, lr.w);
        pLazerItem->SetRotation(lq);
        pLazerItem2->SetRotation(lq);
        EXVector tr = pTrophyObj->Script(0)->m_pItem->Rotation();
        float tspin = XApp::FrameLength() * -2.0f;
        EXQuaternion tq(pCur->m_Unk2c, tspin + tr.y, tr.z, tr.w);
        pTrophyObj->Script(0)->m_pItem->SetRotation(tq);
        pTrophyObj->Script(0)->m_pItem->SetPosition(s_LazerPos);
        if (s_Event == 0x16 && Substate() == 1) SetSubstate(6);
        switch (Substate()) {
        case 0:
            if (IsNewSubstate()) {
                theApp.UnpauseFade();
                LazerState = 0x65;
                m_pSlotsBackground->StateSub(0x40);
                m_b6 = 0;
                m_pHelpText->SetText(pCur->IsVisible() ? Text_Get(0x51000476) : EXString(""), 0, 0);
                m_pSlotsInfo->SetText(Text_Get(0x51000835), 0, 0);
            }
            if (SubstateTime() >= 1.8f && LazerState == 0x65) {
                LazerState = 0x66;
                m_pSlotsBackground->SetMathmos((FEObjColour::Flags)2,
                                               FEMathmos(ex::RGBA(0, 0, 0, 0), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 45.0f, (FEWaveform::Flags)1, 1), 1);
                m_pSlotsBackground->StateAdd(0x40);
                m_b6 = 1;
            }
            if (gLastCamScript && gLastCamScript->LastEventCmdHash() == -1) SetSubstate(1);
            break;
        case 1:
            if (s_Event) {
                EXPoint pt = pCur->GridPos();
                XItemHandler_FETrophy *pNew = 0;
                switch (s_Event) {
                case 0xd:
                    pNew = pCur->DirFudge(s_Current, s_Event, pt);
                    if (!pNew) {
                        int x = pt.x;
                        do {
                            if (--x <= -1) break;
                            pNew = XItemHandler_FETrophy::Object(EXPoint(x, pt.y), 0);
                        } while (!pNew);
                    }
                    break;
                case 0xe:
                    pNew = pCur->DirFudge(s_Current, s_Event, pt);
                    if (!pNew) {
                        int x = pt.x;
                        do {
                            if (++x >= s_GridSize.x) break;
                            pNew = XItemHandler_FETrophy::Object(EXPoint(x, pt.y), 0);
                        } while (!pNew);
                    }
                    break;
                case 0xb:
                    pNew = pCur->DirFudge(s_Current, s_Event, pt);
                    if (!pNew) {
                        int y = pt.y;
                        do {
                            if (--y <= -1) break;
                            pNew = XItemHandler_FETrophy::Object(EXPoint(pt.x, y), 0);
                        } while (!pNew);
                    }
                    break;
                case 0xc:
                    pNew = pCur->DirFudge(s_Current, s_Event, pt);
                    if (!pNew) {
                        int y = pt.y;
                        do {
                            if (++y >= s_GridSize.y) break;
                            pNew = XItemHandler_FETrophy::Object(EXPoint(pt.x, y), 0);
                        } while (!pNew);
                    }
                    break;
                case 0x17:
                    if (XItemHandler_FETrophy::Object(s_Current)->IsVisible()) SetSubstate(3);
                    break;
                }
                if (pNew) {
                    if (s_Event == 0xb || s_Event == 0xc)
                        XGetSoundManager()->StartSound(0x1af00547);
                    else
                        XGetSoundManager()->StartSound(0x1af00548);
                    m_pHelpText->SetText(!pNew->IsVisible() ? EXString("") : Text_Get(0x51000476), 0, 0);
                    s_Target = pNew->m_Unk8;
                    SetSubstate(2);
                }
                s_Event = 0;
            }
            break;
        case 2: {
            float f = min(SubstateTime() * 2.5f, 1.0f);
            float s = (float)sin(f * 1.5707964f);
            EXVector from = pCur->GetItem()->Position();
            XItemHandler_FETrophy *pTgt = XItemHandler_FETrophy::Object(s_Target);
            EXVector to = pTgt->GetItem()->Position();
            TrophyLazerOffset(pTgt, to);
            TrophyLazerOffset(pCur, from);
            EXVector pos = Interp(from, to, s);
            if (!s_bWeapons) pos.z -= 0.3f;
            pLazerItem->SetPosition(pos);
            pLazerItem2->SetPosition(pos);
            if (f >= 1.0f) {
                s_Current = s_Target;
                s_Target = 0;
                s_TrophyType = pTgt->m_Unk1c;
                SetSubstate(1);
            }
            break;
        }
        case 3:
            if (IsNewSubstate()) {
                XGetSoundManager()->StartSound(0x1af0015c);
                XGetSoundManager()->StartSound(0x1af0015b);
                m_pHelpText->SetText(EXString(""), 0, 0);
                m_pSlotsInfo->SetText(EXString(""), 0, 0);
                pBackObj->SetMathmos((FEObjColour::Flags)3,
                                     FEMathmos(ex::RGBA(0, 0, 0, 0), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 30.0f, (FEWaveform::Flags)1, 1), 1);
                unsigned int hash = pCur->m_Unk18;
                pTrophyObj = new FEObject(FEObjType::OT_SCRIPT, XApp::Frontend3DFile(), hash, 0xfdffffff, FE_Blank, FE_Blank,
                                          (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4);
                s_pTrophyObj[idx] = pTrophyObj;
                pTrophyObj->Script(0)->SetScale(pCur->m_Scale);
                pTrophyObj->StateSub(8);
                pPage->ObjectAdd(2, pTrophyObj);
                pTrophyObj->SetOrigin(EXPoint(0xa9, 0xc3), (EXW::TextAlign)4);
                pTrophyObj->StateAdd(0x302);
                XItem *pIt = pTrophyObj->Script(0)->m_pItem;
                pTrophyObj->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0, 0, 0, 0), 1);
                EXQuaternion q(0.0f, 0.0f, 0.0f, 0.0f);
                q.y = 0.7853982f;
                pIt->SetRotation(q);
                s_LazerPos = pIt->Position();
                s_LazerPos.z -= 10.0f;
                s_LazerPos.y -= 0.5f;
                s_LazerPos.x += 0.7f;
                s_LazerPos.y += pCur->m_Unk30;
                pIt->SetPosition(s_LazerPos);
                pText1->SetText(Text_Get(XItemHandler_FETrophy::Object(s_Current)->m_Unk24), 0, 0);
                pText1->LockTextRect(s_TrophyTextX, s_TrophyTextY, 0, 0);
                pText1->StateAdd(0x20000);
                pText1->SetMathmos((FEObjColour::Flags)3,
                                   FEMathmos(ex::RGBA(0, 0, 0, 0), ex::RGBA(0x65, 0x65, 0x65, 0x80), 36.0f, (FEWaveform::Flags)1, 1), 1);
                pText2->SetText(Text_Get(XItemHandler_FETrophy::Object(s_Current)->m_Unk34), 0, 0);
                pText2->SetMathmos((FEObjColour::Flags)3,
                                   FEMathmos(ex::RGBA(0, 0, 0, 0), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 36.0f, (FEWaveform::Flags)1, 1), 1);
                pText3->SetText(Text_Get(XItemHandler_FETrophy::Object(s_Current)->m_Unk38), 0, 0);
                pText3->SetMathmos((FEObjColour::Flags)3,
                                   FEMathmos(ex::RGBA(0, 0, 0, 0), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 36.0f, (FEWaveform::Flags)1, 1), 1);
                LazerState = 0x68;
            }
            pTrophyObj->Script(0)->GetXItem()->SetPosition(s_LazerPos);
            if (pBackObj->Mathmos((FEObjColour::Flags)3)->m_Finished) SetSubstate(4);
            break;
        case 4:
            if (IsNewSubstate()) {
                LazerState = 0x65;
                pTrophyObj->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 1);
            }
            pTrophyObj->Script(0)->GetXItem()->SetPosition(s_LazerPos);
            if (s_Event == 0x16) SetSubstate(5);
            break;
        case 5:
            if (IsNewSubstate()) {
                XGetSoundManager()->StartSound(0x1af00159);
                pBackObj->SetMathmos((FEObjColour::Flags)3,
                                     FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), ex::RGBA(0, 0, 0, 0), 36.0f, (FEWaveform::Flags)1, 1), 1);
                pTrophyObj->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0, 0, 0, 0), 1);
                pText1->SetMathmos((FEObjColour::Flags)3,
                                   FEMathmos(ex::RGBA(0x65, 0x65, 0x65, 0x80), ex::RGBA(0, 0, 0, 0), 36.0f, (FEWaveform::Flags)1, 1), 1);
                pText2->SetMathmos((FEObjColour::Flags)3,
                                   FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), ex::RGBA(0, 0, 0, 0), 36.0f, (FEWaveform::Flags)1, 1), 1);
                pText3->SetMathmos((FEObjColour::Flags)3,
                                   FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), ex::RGBA(0, 0, 0, 0), 36.0f, (FEWaveform::Flags)1, 1), 1);
                LazerState = 0x66;
            }
            pTrophyObj->Script(0)->GetXItem()->SetPosition(s_LazerPos);
            if (pBackObj->Mathmos((FEObjColour::Flags)3)->m_Finished) {
                m_pHelpText->SetText(pCur->IsVisible() ? Text_Get(0x51000476) : EXString(""), 0, 0);
                m_pSlotsInfo->SetText(Text_Get(0x51000835), 0, 0);
                LazerState = 0x64;
                SetSubstate(1);
            }
            break;
        case 6:
            if (IsNewSubstate()) {
                XGetSoundManager()->StartSound(0x1af00158);
                gLastCamScript->FinishScript(XApp::Frontend3DFile(), s_bWeapons ? 0x400064b : 0x400064c);
                LazerState = 0x68;
                m_pSlotsBackground->SetMathmos((FEObjColour::Flags)2,
                                               FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), ex::RGBA(0, 0, 0, 0), 30.0f, (FEWaveform::Flags)1, 1), 1);
            }
            if (SubstateTime() > 0.5f) {
                theApp.CancelFade();
                theApp.ApplyFadeOutIn(2.0f, ex::RGBA(0, 0, 0, 0x80), 0, 1);
                SetSubstate(7);
            }
            break;
        case 7:
            if (SubstateTime() >= 1.0f) {
                theApp.CancelFade();
                theApp.ApplyFadeIn(1.0f, ex::RGBA(0, 0, 0, 0x80), 0);
                gLastCamScript->FinishScript(XApp::Frontend3DFile(), 0x40004ef);
                GoState(4);
            }
            break;
        }
        UpdateLazer();
        if (s_Event) s_Event = 0;
        return 1;
    }
    case 0xb:
    case 0xc:
    case 0xd:
    case 0xe:
    case 0x16:
    case 0x17:
        s_Event = ev;
        return 0;
    }
    return 0;
}

static unsigned int s_InfoQuery = 0;
static unsigned int s_QuitQuery = 0x2000;

void FEMainMenu::DisplayStats(unsigned int level, signed char quit, unsigned int page, signed char noFlush)
{
    ms_bDisplayStats = 1;
    MemCard_PrevLevelHash = gCurrentMap_GeoHash;
    if (FEGetLevels(1)->IndexFromLevel(gCurrentMap_GeoHash) == -1) MemCard_PrevLevelHash = gPrevMap_GeoHash;
    theApp.m_Unke54 = level;
    if (gCurrentMap_GeoHash != 0x100006a) {
        if (!quit) GameData::Instance(1)->SettingsStore();
        else ResetUpgradesFromQuit();
    }
    GameData::Instance(1)->m_Values.m_Unkc = level;
    ms_StatsState = 4;
    ms_InitialLoadedFocus = 0xc;
    ms_RitualTimeBeaten = -1;
    ms_bLevelWasQuit = quit;
    if (quit) {
        if (page) ms_StatsState = page;
        GameData::Instance(1)->m_Values.m_Stats.m_Unk0[0] += (int)theApp.m_LevelPlayTimer;
        ms_InitialLoadedFocus = 5;
        if (GameData::Instance(1)->m_Values.m_LevelStats[0].m_Unk0[0] || GameData::Instance(1)->m_Values.m_Training == 7)
            ms_InitialLoadedFocus = 6;
        if (!noFlush) theApp.GameStateFlush((GameState::Flags)0x20, XApp::Frontend3DFile());
        return;
    }
    {
        switch (FEGetLevels(1)->LevelFromHash(MemCard_PrevLevelHash, 0)->Type()) {
        case 2: {
            GameData::Instance(1)->m_Values.m_Stats.m_Unk0[0] += (int)theApp.m_LevelPlayTimer;
            int t = FEGetLevels(1)->TrainingIndexFromLevelHash(MemCard_PrevLevelHash);
            GameData::Instance(1)->m_Values.m_Training |= 1 << t;
            break;
        }
        case 1: {
            GameData::Instance(1)->m_Values.m_Stats.m_Unk0[0] += (int)theApp.m_LevelPlayTimer;
            ms_StatsState = 0x16;
            unsigned int bit = FEGetLevels(1)->RitualBitIndexFromLevelHash(MemCard_PrevLevelHash);
            GameData::Instance(1)->m_Values.CompleteRitual(bit);
            float t = Mission_RitualCompleteTime;
            RitualTimesView *rt = (RitualTimesView *)GameData::Instance(1)->GetRitualTimes();
            for (unsigned int i = 0; i < 8; i++) {
                if (rt->m_Times[bit][i] > t) {
                    for (unsigned int j = 7; j > i; j--) {
                        rt->m_Times[bit][j] = rt->m_Times[bit][j - 1];
                        strcpy(rt->m_Names[bit][j], rt->m_Names[bit][j - 1]);
                    }
                    rt->m_Times[bit][i] = t;
                    sprintf(rt->m_Names[bit][i], "_Player");
                    ms_RitualTimeBeaten = (bit << 16) | i;
                    rt->m_Unk4 = 0;
                    break;
                }
            }
            break;
        }
        case 0: {
            ms_StatsState = 0x15;
            ms_InitialLoadedFocus = 6;
            GameData::Instance(1)->m_Stats.m_Unk48 = Stats_FaveWeapon;
            int idx = FEGetLevels(1)->IndexFromLevel(gCurrentMap_GeoHash);
            if (idx < 0 || idx >= FEGetLevels(1)->StoryLevelCount()) break;
            XGetGameStats()->m_Unk0[0] = (int)theApp.m_LevelPlayTimer;
            unsigned int best = XGetGameValues()->m_LevelStats[idx].m_Unk0[1];
            XGetGameValues()->m_LevelStats[idx] = *XGetGameStats();
            if (best && XGetGameStats()->m_Unk0[0] >= best) XGetGameValues()->m_LevelStats[idx].m_Unk0[1] = best;
            else XGetGameValues()->m_LevelStats[idx].m_Unk0[1] = XGetGameStats()->m_Unk0[0];
            GameStats *s = XGetGameStats();
            GameStats *tot = &XGetGameValues()->m_Stats;
            tot->m_Unk0[0] += s->m_Unk0[0];
            tot->m_Kills += s->m_Kills;
            tot->m_KillsDishonour += s->m_KillsDishonour;
            tot->m_Unk4c[0] += s->m_Unk4c[0];
            tot->m_Unk10[0] += s->m_Unk10[0];
            tot->m_Unk10[6] += s->m_Unk10[6];
            tot->m_Unk4c[1] += s->m_Unk4c[1];
            tot->m_Unk4c[2] += s->m_Unk4c[2];
            tot->m_Unk4c[3] += s->m_Unk4c[3];
            tot->m_Unk4c[5] += s->m_Unk4c[5];
            tot->m_Recharges += s->m_Recharges;
            tot->m_Unk10[13] += s->m_Unk10[13];
            tot->m_Unk10[12] += s->m_Unk10[12];
            tot->m_Unk10[9] += s->m_Unk10[9];
            tot->m_Unk10[10] += s->m_Unk10[10];
            tot->m_Unk10[11] += s->m_Unk10[11];
            tot->m_Unk10[1] += s->m_Unk10[1];
            tot->m_Unk10[2] += s->m_Unk10[2];
            tot->m_Unk10[3] += s->m_Unk10[3];
            tot->m_Unk10[4] += s->m_Unk10[4];
            tot->m_Unk10[5] += s->m_Unk10[5];
            tot->m_Unk4c[4] += s->m_Unk4c[4];
            if (s->LongestThrow() > tot->LongestThrow()) tot->LongestThrow() = s->LongestThrow();
            if (s->LongestKick() > tot->LongestKick()) tot->LongestKick() = s->LongestKick();
            EXFixedArray<int, 14> counts;
            memset(&counts, 0xff, sizeof(counts));
            for (int i = 0; i < FEGetLevels(1)->StoryLevelCount(); i++) {
                int w = GameData::Instance(1)->m_Values.m_LevelStats[i].FaveWeapon();
                if (w >= 0) counts[w]++;
            }
            int fave = -1;
            int most = 0;
            for (int k = 0; k < 14; k++) {
                if (counts[k] >= most) {
                    fave = k;
                    most = counts[k];
                }
            }
            tot->m_Unk48 = fave;
            EXFixedArray<signed char, 27> carry = {{0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0}};
            if (carry[idx]) GameData::Instance(1)->Values()->m_LevelRecords[idx].Store(0);
            else if (idx == 12 || idx == 17 || idx == 22) GameData::Instance(1)->Values()->m_LevelRecords[idx].Store(1);
            break;
        }
        }
        SetInitialChapterSelectFocus();
        SetInitialRitualSelectFocus(0);
    }
    ms_InitialLoadedFocus = 5;
    if (GameData::Instance(1)->m_Values.m_LevelStats[0].m_Unk0[0] || GameData::Instance(1)->m_Values.m_Training == 7)
        ms_InitialLoadedFocus = 6;
    if (!noFlush) theApp.GameStateFlush((GameState::Flags)0x20, XApp::Frontend3DFile());
}

signed char FEMainMenu::HandleFrontendEvent(FEObjEvent::Flags ev)
{
    FEObject * volatile pCur = 0;
    if (m_bGoing && IsPadEvent(ev)) return 0;
    unsigned int state = m_ListenerIdx;
    pCur = Layout(state);
    volatile signed char bUpdate = 1;
    switch (ev) {
    case 0x13: {
        SetState(0);
        if (m_bUpdateOnChange) return 1;
        m_bUpdateOnChange = 1;
        AddContent();
        UpdateState(0xffffffff, (FEObjEvent::Flags)0x1e);
        unsigned int st;
        if (ms_RitualTimeBeaten != -1) {
            st = 0x13;
        } else {
            if (!ms_bDisplayStats) st = 1;
            else if (!ms_bLevelWasQuit) st = 0x12;
            else st = ms_StatsState;
            if (st == 1) GameData::Instance(1)->GetBigSave()->SetStatusAndReset(0);
        }
        GoState(st);
        XGetSoundManager()->SetSfxVolume(XGetGameValues()->m_MovieVolume);
        theApp.m_Unke54 = 0;
        for (unsigned int i = 0; i < 24; i++) {
            FEObject *p = Layout(i);
            if (p) {
                EXRect r = p->Rect();
                m_LayoutPos[i].x = (float)r.x;
                m_LayoutPos[i].y = (float)r.y;
            }
        }
        short skin = GameData::Instance(1)->m_Values.m_Unk58;
        if (skin == -1) skin = 0;
        XItemHandler_Player::Instance()->SetSelectedSkin(skin);
        SetPlayerSkin(skin);
        m_Unk82c = 3;
        if (st == 1) {
            MemCard_CheckSavesPresent(1, 0);
            m_Unk82c = 1;
        }
        m_b1 = 1;
    }
    case 0x15: {
        if (m_Unk82c == 1) {
            int r = MemCard_CheckSavesPresent(0, 0);
            if (r) {
                if (r == 1 && (MemCardSavesFound() & 0x3f)) m_Unk82c = 3;
                else if (r == -4) m_Unk82c = 4;
                else m_Unk82c = 2;
            }
        }
        UpdatePredSkin();
        signed char bOne = 1;
        if (m_GoState) {
            bOne = 0;
            if (GameControllerRemovedSys::Wnd()) {
                GameControllerRemovedSys::HideDlg();
                m_bDialogOn = 0;
                m_b7 = 0;
            }
            GameControllerRemovedSys::UpdateControllers();
        } else {
            switch (m_ListenerIdx) {
            case 13:
            case 14:
            case 15:
            case 16:
            case 17:
                if (!m_Unk844) goto no_check;
            default:
            check:
                m_bIgnoreNextPageSelect = 0;
                if (!m_b7) {
                    if (state != 0 && state != 0x17) m_b7 = GameControllerRemovedSys::DisconnectCheck((GCRS::Flags)2);
                    if (m_b7) Hide_MemCardExtraDialog_NoDestroy();
                    else Show_MemCardExtraDialog_NoCreate();
                } else {
                    m_b7 = !GameControllerRemovedSys::ReconnectCheck();
                    if (!m_b7) m_bIgnoreNextPageSelect = 1;
                }
                break;
            case 20:
                if (!m_Unk844) goto no_check;
            case 9:
            case 11: {
                unsigned int sub = Substate();
                if (sub && (sub <= 5 || sub > 7)) goto check;
            }
            case 0:
            case 1:
            case 2:
            case 3:
            no_check:
                bOne = 0;
                GameControllerRemovedSys::UpdateControllers();
                break;
            }
        }
        if (state != 1) FEListener::LastInputReset();
        if (FEListener::QueryButton(0x4000, bOne ? 1 << GameControllerRemovedSys::Controller() : 0xf, 1) && !m_bGoing &&
            !MemCardDlgIsOn() && !m_bIgnoreNextPageSelect && !GameDialogWnd::NumInstances()) {
            if (state == 6) {
                UpdateStateStoryLevelSelect((FEObjEvent::Flags)3);
            } else if (state == 0x13) {
                UpdateStateEditName((FEObjEvent::Flags)3);
            } else if (state == 7) {
                UpdateStateRitualChallengeSelect((FEObjEvent::Flags)3);
            } else if (InfoHelpText()) {
                XGetSoundManager()->StartSound(0x1af0015c);
                Show_MemCardExtraDialog_Query(InfoHelpText(), 0, 0, 0, 0xa, 0, -1, &s_InfoQuery);
                return 0;
            } else if (state == 4) {
                Show_MemCardExtraDialog_Query(0x510001e3, 0, 0, 0, 0xa, 0, -1, &s_QuitQuery);
                XGetSoundManager()->StartSound(0x1af00158);
                return 0;
            }
        }
        for (unsigned int i = 0; i < 24; i++) {
            FEObject *p = Layout(i);
            if (p) p->StateSub(0x10000);
        }
        if (pCur && !m_bGoing) pCur->StateAdd(0x10000);
        GoStateUpdate();
        unsigned int newState = m_pManager->m_PageState;
        if (newState != state || state != m_ListenerIdx) {
            state = newState;
            m_ListenerIdx = newState;
            pCur = Layout(newState);
            m_pCurLayout = (FELayout *)pCur;
        }
        if (MemCardDlgIsOn() || GameDialogWnd::NumInstances() || MemCardBusy()) {
            if (!m_bDialogOn) {
                m_b6 = (m_pSlotsBackground->m_Flags & 0x40) != 0;
                m_PreDlgFocus = 0xfdffffff;
                m_Unk818 = state;
                if (UpdateListener()) {
                    if (UpdateListener()->Focus()) m_PreDlgFocus = UpdateListener()->Focus()->Data();
                    UpdateListener()->Disable();
                }
                if (state != 0xb && state != 9 && state != 0x14) NewFocus(0, state);
                m_pDimmedLayout = pCur;
                if (pCur) {
                    pCur->SetMathmos((FEObjColour::Flags)0,
                                     FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x6e), ex::RGBA(0x7f, 0x7f, 0x7f, 0x6e), 60.0f, (FEWaveform::Flags)1, 0), 0);
                    pCur->SetMathmos((FEObjColour::Flags)1, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x6e)), 1);
                }
            }
            m_bDialogOn = 1;
            m_Unk84c = 4;
            m_pSlotsBackground->StateSub(0x40);
        } else if (--m_Unk84c <= 0) {
            if (state == 0 || state == 0x17) {
                m_pSlotsBackground->StateSub(0x40);
            } else if (m_bDialogOn) {
                if (m_b6) m_pSlotsBackground->StateAdd(0x40);
                if (state == (unsigned int)m_Unk818 && m_PreDlgFocus != 0xfdffffff)
                    NewFocus(FEListener::FindObjectByData(UpdateListener()->Owner(), m_PreDlgFocus), state);
                if (m_pDimmedLayout && m_pDimmedLayout == Layout(m_Unk818)) {
                    m_pDimmedLayout->SetMathmos((FEObjColour::Flags)0,
                                                FEMathmos(ex::RGBA(0x7f, 0, 0, 0x80), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 60.0f, (FEWaveform::Flags)1, 0), 0);
                    m_pDimmedLayout->SetMathmos((FEObjColour::Flags)1, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80)), 1);
                }
                if (m_UpdateListeners[m_Unk818]) m_UpdateListeners[m_Unk818]->Enable();
            }
            m_bDialogOn = 0;
        }
        HideSlotsLayoutUpdate();
        break;
    }
    case 0x16:
        if (MemCardDlgIsOn() || GameDialogWnd::NumInstances() || MemCardBusy()) break;
        switch (state) {
        case 0:
        case 1:
        case 4:
        case 18:
        case 19:
        case 23:
            return 0;
        case 5:
        case 6:
        case 7:
        case 8:
        case 10:
            GoState(4);
            bUpdate = 0;
            break;
        case 9:
        case 11:
            if (UpdateStateViewTrophies((FEObjEvent::Flags)0x16)) GoState(4);
            return 0;
        case 12:
            MemCardEnableAutoSave(s_RememberAutoSave);
            GoState(4);
            bUpdate = 0;
            break;
        case 13:
            if (m_Unk844) {
                GoState(4);
                bUpdate = 0;
                break;
            }
            GoState(1);
            bUpdate = 0;
            break;
        case 14:
        case 15:
        case 16:
        case 17:
            GoState(0xd);
            bUpdate = 0;
            break;
        case 20:
            if (Substate() == 1) {
                XGetSoundManager()->StartSound(0x1af00158);
                SetSubstate(6);
            }
            return 0;
        case 21:
            GoState(6);
            bUpdate = 0;
            break;
        case 22:
            GoState(7);
            bUpdate = 0;
            break;
        default:
            GoState(1);
            bUpdate = 0;
            break;
        }
        break;
    case 9:
        NewFocus(UpdateObject(), state);
        HideSlotsLayoutUpdate();
        break;
    case 0x1f:
        m_Unk28c = m_Unk290 = -1;
        SetMemCardFlow(0);
        break;
    case 11:
        if (UpdateListener()) m_Unk290 = UpdateListener()->UpdateTimeStamp();
        break;
    case 12:
        if (UpdateListener()) m_Unk28c = UpdateListener()->UpdateTimeStamp();
        break;
    }
    if (bUpdate && (!m_bGoing || ev != 0x15)) return UpdateState(state, ev);
    return 1;
}
