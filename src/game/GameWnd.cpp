#include "XGameWnd.h"
#include "XApp.h"
#include "XItem.h"
#include "XDebug.h"
#include "AIControl.h"
#include "ActorUtil.h"
#include "GameData.h"
#include "GeoSectionManager.h"
#include "EXGeoFile.h"
#include "EXFileSys.h"
#include "EXMenu.h"
#include "EXString.h"
#include "EXHashCodeFile.h"
#include "EXBaseSoundManager.h"
#include "EXApp.h"
#include "EXMemory.h"
#include "FEText.h"

#include "XItemHandler_CameraScript.h"
#include "FEManager.h"
#include "XItemHandler_Player.h"
#include "EXBaseApp.h"
#include "EXMatrix.h"
#include "XItemHandler_CCTV.h"
#include "XItemHandler_Turret.h"
#include "XItemHandler_Voosh.h"
#include <math.h>
#include "XItemHandler_Camera.h"
#include "XDebug.h"
#include "EXDebugWnd.h"
#include "EXDebugTreeWnd.h"
#include "EXWatcher.h"
#include "EXBaseDisplay.h"
#include "EX6DObject.h"

void TrafficInitTriggers();

static const int kDefaultSfxVolume = 100;
int gSfxVolume = kDefaultSfxVolume;
EXGeoFile *pMapGeoFile;
int m_WaitPause;
XGameWnd *gpGameWnd[4];
unsigned int numGameWnds;
signed char DBG_bSkipMoviesAndLegal;
unsigned int gCurrentMap_GeoHash = XApp::Frontend2DFile();
static const float kDummy = 0.0f;
unsigned int gPrevMap_GeoHash = XApp::Frontend2DFile();

void XGameWnd::WaitForIslandInit()
{
    switch (gCurrentMap_GeoHash) {
    case 0x1000072:
    case 0x10000a2:
        m_Unk400 = 0;
        Island_Initialise();
        break;
    default:
        m_Unk400 = -1;
        break;
    }
    m_WaitPause = 30;
}

signed char XGameWnd::WaitForIsland()
{
    XItemEnv *pEnv = &theItemEnv;
    pEnv->UpdateItems_UpdateCode();
    theItemEnv.UpdateItems_LastFrame();
    if (EXFileSys::GetFileSys()->GetLoadingStatus()) {
        m_WaitPause = 30;
        return 0;
    }
    return --m_WaitPause <= 0;
}

s8 XGameWnd::DrawWindow() { return EXBaseWnd::DrawWindow(); }

void XGameWnd::Debug_TestEngineFunctions() {}

void XGameWnd::LevelPreLoad()
{
    if (!m_Unk380) TrafficInitTriggers();
    m_Unk400 = -1;
    gAIControl.PreLoadInit();
    gCutscenePlayer.Init();
    GeoSectionManager::Init(0);
    XItemEnv *pEnv = &theItemEnv;
    pEnv->Initialise();
    EXBaseSoundManager::GetSoundManager()->StartMusic(0x1b00001e, 1, 1, 1, 1);
}

EXGeoFile *XGameWnd::FileLoad(unsigned int hash, int *pLen)
{
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(hash, 1, 0);
    if (!pFile) {
        EXHashCodeFile::Debug_HashCodeName(hash);
        ex::EXDebugBreak();
    }
    if (pLen) *pLen = pFile->GetFilelength();
    return pFile;
}

void XGameWnd::LevelPreRun() { gAIControl.LevelPreRun(); }

struct SoundBankRow {
    unsigned int m_Map;     // level geo hash
    unsigned int m_Bank;    // sound bank hash
    int m_CombatMusic;      // combat music hash
};

void LoadSoundBank()
{
    int n;
    SoundBankRow *p = (SoundBankRow *)AU_GetWorkSheet(0x1000081, 0x1400000a, 0, n);
    if (!p) return;
    unsigned int bank = 0;
    for (int i = 0; i < n; i++) {
        if (p[i].m_Map == gCurrentMap_GeoHash) {
            SoundBankRow *r = &p[i];
            bank = r->m_Bank;
            gAIControl.SetCombatMusicHash(r->m_CombatMusic);
            break;
        }
    }
    if (!bank) bank = 0x1ae00012;
    EXBaseSoundManager::GetSoundManager()->DeloadSoundBank(-1);
    EXBaseSoundManager::GetSoundManager()->LoadSoundBank(bank);
    int mode = (short)GameData::Instance(1)->Values()->m_Unk58;
    if (mode < -1 || mode > 5) EXBaseSoundManager::GetSoundManager()->LoadSoundBank(0x1ae00036);
    else EXBaseSoundManager::GetSoundManager()->LoadSoundBank(0x1ae00035);
}

s8 XGameWnd::Update()
{
    if (m_bShowBars) {
        m_pBarWnd[0]->Flags().BitClr(0x8000);
        m_pBarWnd[1]->Flags().BitClr(0x8000);
    } else {
        m_pBarWnd[0]->Flags().BitSet(0x8000);
        m_pBarWnd[1]->Flags().BitSet(0x8000);
    }
    if (m_bShowDisplay) m_pDisplayWnd->Flags().BitClr(0x8000);
    else m_pDisplayWnd->Flags().BitSet(0x8000);
    XDebug::GetDebug()->UpdateDebugWindows();
    return 1;
}

EXGeoFile *XGameWnd::LevelLoad(unsigned int hash, int *pLen)
{
    if (hash != gCurrentMap_GeoHash) {
        gPrevMap_GeoHash = gCurrentMap_GeoHash;
        gCurrentMap_GeoHash = hash;
    }
    EXHashCodeFile::Debug_HashCodeName(hash);
    EXGeoFile *pFile = FileLoad(gCurrentMap_GeoHash, pLen);
    pMapGeoFile = EXGeoFile::GetGeoFile(gCurrentMap_GeoHash);
    return pFile;
}

void XGameWnd::UpdateLevelMenu()
{
    EXFileSys::GetFileSys()->UpdateFileLists();
    EXFileList *pList = EXFileSys::GetFileSys()->FileList(0);
    EXFileInfoArray files;
    files.Copy(pList->m_Files);
    files.SortByName();
    for (int i = 0; i < files.Size(); i++) {
        EXFILEINFO &f = files[i];
        if ((f.m_Flags & 0x80000004) == 0x80000004) {
            EXStringArray parts(0, -1);
            EXString name(f.m_pName);
            int n;
            {
                EXString sep("\\");
                n = name.Tokenise(parts, sep, 0);
            }
            m_pLevelMenu->AddItem(parts[n - 1], f.m_Hash);
        }
    }
}

extern unsigned int gSystemCameraUser;

XGameWnd::XGameWnd()
{
    gpGameWnd[numGameWnds] = this;
    m_Unk380 = numGameWnds;
    numGameWnds++;
    m_Unk354 = EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    m_Unk384 = 0;
    m_Unk388[0] = 0;
    m_Unk388[1] = 0;
    m_Unk388[2] = 1;
    m_Unk388[3] = 1;
    m_Unk388[4] = 0;
    m_pMenu = new EXMenu;
    EXMenu *pSub;
    {
        EXString s("Level");
        m_pLevelMenu = m_pMenu->AddSubMenu(s, 0);
    }
    {
        EXString s("-----------");
        m_pMenu->AddItem(s, 0);
    }
    {
        EXString s("Nav Mesh");
        pSub = m_pMenu->AddSubMenu(s, 0);
    }
    {
        EXString s("Draw Nav Mesh");
        pSub->AddItem(s, 0x46);
    }
    {
        EXString s("Draw Nav Routing");
        pSub->AddItem(s, 0x47);
    }
    {
        EXString s("Draw Nav Paths");
        pSub->AddItem(s, 0x48);
    }
    {
        EXString s("Actor");
        pSub = m_pMenu->AddSubMenu(s, 0);
    }
    {
        EXString s("States");
        pSub->AddItem(s, 0x49);
    }
    {
        EXString s("Locations");
        pSub->AddItem(s, 0x4a);
    }
    {
        EXString s("TimeTable");
        pSub->AddItem(s, 0x4b);
    }
    {
        EXString s("-----------");
        pSub->AddItem(s, 0);
    }
    {
        EXString s("Skin Test");
        pSub->AddItem(s, 0x4f);
    }
    {
        EXString s("Spawn Male");
        pSub->AddItem(s, 0x66);
    }
    {
        EXString s("Spawn Female");
        pSub->AddItem(s, 0x67);
    }
    {
        EXString s("Spawn Predator");
        pSub->AddItem(s, 0x68);
    }
    {
        EXString s("Tester");
        pSub = m_pMenu->AddSubMenu(s, 0);
    }
    {
        EXString s("Performance Window");
        pSub->AddItem(s, 0x6a);
    }
    {
        EXString s("Tester Window");
        pSub->AddItem(s, 0x6d);
    }
    {
        EXString s("Safe Frame");
        pSub->AddItem(s, 0x6b);
    }
    {
        EXString s("Invincible");
        pSub->AddItem(s, 0x6c);
    }
    {
        EXString s("Red Square < 30 fps");
        pSub->AddItem(s, 0x6e);
    }
    {
        EXString s("Ignore Objectives");
        pSub->AddItem(s, 0x6f);
    }
    {
        EXString s("Toggle frame rate");
        pSub->AddItem(s, 0x71);
    }
    {
        EXString s("No Mask");
        pSub->AddItem(s, 0x72);
    }
    {
        EXString s("Hide HUD");
        pSub->AddItem(s, 0x73);
    }
    {
        EXString s("Toggle full Combo");
        pSub->AddItem(s, 0x74);
    }
    {
        EXString s("Tri Count");
        pSub->AddItem(s, 0x75);
    }
    {
        EXString s("No Actors");
        pSub->AddItem(s, 0x76);
    }
    {
        EXString s("Unlocks");
        pSub = m_pMenu->AddSubMenu(s, 0);
    }
    {
        EXString s("Rituals");
        pSub->AddItem(s, 1000);
    }
    {
        EXString s("Upgrades");
        pSub->AddItem(s, 1001);
    }
    {
        EXString s("Trophies");
        pSub->AddItem(s, 1002);
    }
    {
        EXString s("Skins");
        pSub->AddItem(s, 1003);
    }
    {
        EXString s("Levels/Skulls");
        pSub->AddItem(s, 1004);
    }
    {
        EXString s("-----------");
        m_pMenu->AddItem(s, 0);
    }
    {
        EXString s("Front End");
        m_pMenu->AddItem(s, 0x4c);
    }
    {
        EXString s("Front End 2D");
        m_pMenu->AddItem(s, 0xb0);
    }
    {
        EXString s("File");
        pSub = m_pMenu->AddSubMenu(s, 0);
    }
    {
        EXString s("Save");
        pSub->AddItem(s, 0x4d);
    }
    {
        EXString s("Load");
        pSub->AddItem(s, 0x4e);
    }
    {
        EXString s("GSM Display");
        pSub->AddItem(s, 0x65);
    }
    {
        EXString s("-----------");
        m_pMenu->AddItem(s, 0);
    }
    {
        EXString s("Cutscene");
        m_pMenu->AddSubMenu(s, 0x50);
    }
    {
        EXString s("-----------");
        m_pMenu->AddItem(s, 0);
    }
    {
        EXString s("Triggers");
        pSub = m_pMenu->AddSubMenu(s, 0);
    }
    {
        EXString s("Trigger Collision");
        pSub->AddItem(s, 0xc8);
    }
    {
        EXString s("Environment Collision");
        pSub->AddItem(s, 0xc9);
    }
    {
        EXString s("Kill Counts");
        pSub->AddItem(s, 0xca);
    }
    {
        EXString s("Trigger Debug Text");
        pSub->AddItem(s, 0xcb);
    }
    {
        EXString s("Trigger Map");
        pSub->AddItem(s, 0xcc);
    }
    {
        EXString s("-----------");
        m_pMenu->AddItem(s, 0);
    }
    {
        EXString s("Slow-Mo");
        pSub = m_pMenu->AddSubMenu(s, 0);
    }
    {
        EXString s("4X");
        pSub->AddItem(s, 0x12c);
    }
    {
        EXString s("2X");
        pSub->AddItem(s, 0x12d);
    }
    {
        EXString s("1X");
        pSub->AddItem(s, 0x12e);
    }
    {
        EXString s("1/2X");
        pSub->AddItem(s, 0x12f);
    }
    {
        EXString s("1/4X");
        pSub->AddItem(s, 0x130);
    }
    {
        EXString s("-----------");
        m_pMenu->AddItem(s, 0);
    }
    {
        EXString s("Reload Current");
        m_pMenu->AddItem(s, 60);
    }
    m_pMenu->SetTopLevel(0);
    m_bShowBars = 0;
    m_bShowDisplay = 0;
    *(u8 *)&m_Unk37e = 1;
    XDebug::GetDebug()->InitDebugWindows();
    UpdateLevelMenu();
    gPrevMap_GeoHash = XApp::Frontend2DFile();
    gCurrentMap_GeoHash = gPrevMap_GeoHash;
    {
        XGameState state((GameState::Flags)0x20, gPrevMap_GeoHash);
        theApp.m_GameStates.Push(state);
        theApp.m_bGameStateChanged = 1;
    }
    theItemEnv.m_p6DManager = new EX6DManager;
}

XGameWnd::~XGameWnd()
{
    EXDebugWndMgr::GetDebugWndMgr()->RegisterMenu(0, 0);
    if (m_pBarWnd[0]) {
        delete m_pBarWnd[0];
        m_pBarWnd[0] = 0;
    }
    if (m_pBarWnd[1]) {
        delete m_pBarWnd[1];
        m_pBarWnd[1] = 0;
    }
    if (m_pDisplayWnd) {
        delete m_pDisplayWnd;
        m_pDisplayWnd = 0;
    }
}

// Creates the debug performance / memory bars and registers the loading watches.
signed char XGameWnd::InitialUpdate()
{
    EXStatsBarWnd *pStats = new EXStatsBarWnd;
    m_pBarWnd[0] = pStats;
    pStats->SetRect(EXRect(10, 5, EXBaseDisplay::GetDisplay()->Width() - 0x14, 10));
    pStats->m_Unk2b4 = ex::RGBA(0x50, 0x50, 0x50, 0x80);
    pStats->m_Unk2ac = 0x100;
    pStats->m_Unk2b0 = 4;
    pStats->m_Unk2b8 = 1;
    pStats->m_Unk2b9 = 1;
    pStats->m_Unk2ba = 0x14;
    pStats->Flags().BitSet(0x1000);

    pStats = new EXStatsBarWnd;
    m_pBarWnd[1] = pStats;
    pStats->SetRect(EXRect(10, 0xd, EXBaseDisplay::GetDisplay()->Width() - 0x14, 10));
    pStats->m_Unk2b4 = ex::RGBA(0x50, 0x50, 0x50, 0x80);
    pStats->m_Unk2ac = 0x100;
    pStats->m_Unk2b0 = 4;
    pStats->m_Unk2b8 = 0;
    pStats->m_Unk2b9 = 1;
    pStats->m_Unk2ba = 0x14;
    pStats->Flags().BitSet(0x1000);

    EXMemBarWnd *pMem = new EXMemBarWnd;
    m_pDisplayWnd = pMem;
    pMem->SetRect(EXRect(10, 0x15, EXBaseDisplay::GetDisplay()->Width() - 0x14, 10));
    pMem->m_Color = ex::RGBA(0x50, 0x50, 0x50, 0x80);
    pMem->m_BarHeight = 4;
    pMem->m_ShowStats = 0;
    pMem->Flags().BitSet(0x1000);

    AddChild(m_pBarWnd[0]);
    AddChild(m_pBarWnd[1]);
    AddChild(m_pDisplayWnd);

    EXWatcher *pWatcher = EXWatcher::GetWatcher();
    unsigned int h = pWatcher->AddItem("PerfBars", pWatcher->SectionHandle("Display", 2), (EXWATCH::DataType)8, &m_bShowBars);
    EXWatcher::GetWatcher()->EnableItemEdit(h, 1);
    pWatcher = EXWatcher::GetWatcher();
    h = pWatcher->AddItem("MemBar", pWatcher->SectionHandle("Display", 2), (EXWATCH::DataType)8, &m_bShowDisplay);
    EXWatcher::GetWatcher()->EnableItemEdit(h, 1);
    pWatcher = EXWatcher::GetWatcher();
    pWatcher->AddSection("Loading", 0u, "-Game-", 2);
    pWatcher = EXWatcher::GetWatcher();
    h = pWatcher->AddItem("Super Mega Hyper Turbo Loading", pWatcher->SectionHandle("Loading", 2), (EXWATCH::DataType)8, &m_Unk37e);
    EXWatcher::GetWatcher()->EnableItemEdit(h, 1);
    RegisterItemEnvView(1);
    return 1;
}

// Points the window camera at the full 640x640 clip area with the game camera's zoom.
void XGameWnd::SetupCamera()
{
    if (gSystemCameraUser != 1) {
        m_Camera.SetClipSize(640, 640, 0);
        m_Camera.SetOriginInline(m_Camera.Origin());
        m_Camera.SetVFOV(PrimRect(), 0.7853982f / XGetCamera(0)->Zoom());
        m_Camera.UpdateDisplay(0);
        gSystemCameraUser = 1;
    }
}

extern EXVector ShadowCentre;
void TrafficDraw();
void Rain_Update();
void Mission_Update();
void Draw_WeaponBeams();
signed char MemCard_BootChecks(signed char b);

// Renders the world through the window camera, then the per-frame overlays (viewcones, beams, etc.).
signed char XGameWnd::Draw()
{
    MemCard_BootChecks(1);
    if (FEManager::Instance(1) && FEManager::Instance(1)->m_Attract && FEManager::Instance(1)->m_AttractTime > 20.0f) return 1;
    if (!XItemHandler_Player::GetInstance()) return 1;

    m_Camera.SetClipSize(640, 640, 0);
    m_Camera.SetOrigin(Origin());
    float fov;
    if (XGetCamera(0)->m_CutsceneFOV == 0.0f)
        fov = 0.7853982f / XGetCamera(0)->m_Zoom;
    else
        fov = XGetCamera(0)->m_CutsceneFOV;
    m_Camera.SetVFOV(PrimRect(), fov);
    m_Camera.UpdateDisplay(0);
    gSystemCameraUser = 1;

    // View matrix for the item environment: the world-to-view rotation with the listener position (shifted
    // along the view's z axis) as translation.
    EXMatrix view(EXBaseDisplay::WorldToView().transpose());
    EXVector fwd(view.r[2].x, view.r[2].y, view.r[2].z, 0.0f);
    float lenSq = fwd.z * fwd.z + fwd.y * fwd.y + fwd.x * fwd.x;
    if (lenSq >= 0.0f) {
        float len = (float)sqrt(lenSq);
        if (len > 1e-7f) {
            float inv = 1.0f / len;
            fwd.x *= inv;
            fwd.y *= inv;
            fwd.z *= inv;
            fwd.w *= inv;
        }
    }
    const EXVector &listener = XGetCamera(0)->m_ListenerPos;
    m_Unk354 = EXVector(listener.x + fwd.x, listener.y + fwd.y, listener.z + fwd.z, listener.w + fwd.w);
    view.r[3].x = m_Unk354.x;
    view.r[3].y = m_Unk354.y;
    view.r[3].z = m_Unk354.z;
    view.r[3].w = 1.0f;
    RenderItemEnvView(m_Camera, view);

    TrafficDraw();
    Rain_Update();
    Mission_Update();
    Draw_CameraViewcones();
    Draw_TurretViewcones();
    Draw_WeaponBeams();
    Draw_Voosh();

    EXDebugWnd *pProfile = XDebug::GetDebug()->ProfileWnd();
    if (pProfile) {
        pProfile->SetDbgPos(0, 3);
        pProfile->DbgPrint("Tris:%d/%d", 0, 0);
        if (EXBaseApp::GetApp()->FrameSkipIndicator())
            pProfile->SetBkColor(ex::RGBA(0x40, 0, 0, 0x40));
        else
            pProfile->SetBkColor(ex::RGBA(0, 0, 0x40, 0x40));
    }
    m_Unk384++;
    gSystemCameraUser = 0;
    return 1;
}

// Renders the player's shadow from a projector hovering above the player (or the cutscene shadow centre).
void XGameWnd::DrawShadow()
{
    if (!gpPlayerItem[0]) return;
    XItemHandler_Player *pPlayer = (XItemHandler_Player *)gpPlayerItem[0]->m_pItemHandler;
    if (gSystemCameraUser != 3) {
        EXVector centre;
        if (theApp.m_bCutscene)
            centre = ShadowCentre;
        else
            centre = gpPlayerItem[0]->Position();
        m_Projector.SetPosition(EXVector(centre.x, centre.y + 30.0f, centre.z, centre.w));
        m_Projector.SetTarget(centre);
        m_Projector.SetPerspective(0);
        m_Projector.UpdateDisplay(0);
        gSystemCameraUser = 3;
    }
    if (!pPlayer->GetCloak() && !pPlayer->m_Tint) {
        theItemEnv.m_RenderFlagsHi |= 2;
        theItemEnv.m_ShadowDensity = 0x1fff;
        theItemEnv.RenderShadows();
    }
}

#include "EXSwooshSysMgr.h"
#include "EXParticleSysMgr.h"
#include "FE.h"
#include "FEHud.h"
#include "FELevels.h"
#include "FETopmost.h"
#include "FEPause.h"
#include "EVD.h"
#include "VisionWipe.h"
#include "XGamePad.h"
#include "FERadar.h"
#include "EXMemHeap.h"
void TrafficDeinit();

// Tears down everything LevelLoad built. 'full' also drops the persistent geo files.
void XGameWnd::LevelDeinit(signed char full)
{
    m_Unk400 = -1;
    EXBaseSoundManager::GetSoundManager()->FadeSFXDown(10, 0);
    EXBaseSoundManager::GetSoundManager()->FadeMusicDown(10, 0);
    // WaitDrawSync, then flush window memory
    EXBaseDisplay::GetDisplay()->WaitDrawSync();
    gpGameWnd[0]->SwooshSysMgr()->DeleteAllSystems();
    gpGameWnd[0]->ParticleSysMgr()->DeleteAllSystems();
    EXBaseDisplay::GetDisplay()->FlushAllWindowMem();
    theItemEnv.RemoveAllMaps();
    ((XGamePad *)EXBaseGamePad::GetGamePad())->StopAllRumble();
    Text_Shutdown();
    EXBaseSoundManager::DeleteSoundDetailsTable(1);
    if (m_Unk380 == 0) TrafficDeinit();
    theItemEnv.RemoveAllAddedItems();
    theItemEnv.RemoveAllDynLights();
    gAIControl.UnloadLevel();
    for (unsigned int h = 0x1000000; h < 0x100019c; h++) {
        if (h == 0x100004e) continue;  // loading screen stays resident
        if (!full && m_Unk37e & 0xff) {
            switch (h) {
            case 0x1000076: case 0x100003a: case 0x1000025: case 0x100003b:
            case 0x1000005: case 0x1000024: case 0x1000016: case 0x1000013:
            case 0x100000f: case 0x100004c: case 0x10000e3: case 0x100004f:
            case 0x100001d: {
                // keep these resident, just drop their section files
                EXGeoFile *f = EXGeoFile::GetGeoFile(h);
                if (f && f->m_pGeoHeader) f->m_pGeoHeader->DeLoadSectionFiles();
                continue;
            }
            }
        }
        EXGeoFile::DeLoadGeoFile(h, 1);
    }
    if (FEGetHud(0)) delete FEGetHud(0);
    if (FEGetRadar(0)) delete FEGetRadar(0);
    if (FEGetManager(0)) delete FEGetManager(0);
    if (FEGetPause(0)) delete FEGetPause(0);
    if (FEGetCutscene(0)) delete FEGetCutscene(0);
    if (FEGetDialog(0)) delete FEGetDialog(0);
    if (EVD::Instance(0)) delete EVD::Instance(0);
    if (XGetVisionWipe(0)) delete XGetVisionWipe(0);
    if (FEGetObjectives(0)) delete FEGetObjectives(0);
    if (FEGetLevels(0)) delete FEGetLevels(0);
    theApp.m_bLevelLoaded = 0;
    EXMemHeap::ForceCompactHeaps();
}

#include "WeaponTable.h"
#include "FX_Gore.h"
#include "XItemHandler_Camera.h"
extern float Mission_AlarmLevel;
extern EXVector Mission_RestartPosition;
extern EXVector Mission_RestartRotation;
extern EXVector gStartPosition;
extern EXVector gStartRotation;
extern signed char gStartCloaked;
extern unsigned int gCurrentMap_GeoHash;
extern EXGeoFile *pMapGeoFile;
void Trigger_PreLoad();
void Trigger_SetPlayerWeaponsCreated(signed char b);
signed char Trigger_PlayerWeaponsCreated();
unsigned int TriggerInit(EXTrigger *pTrigger, EXGeoTrigger *pGeo, EXGeoMap *pMap, EXMapOn mapOn, unsigned short index);
void Mission_Initialise();
signed char TrafficInit();
void AirTraffic_Initialise();
void Rain_Initialise();
void NightAndDay_Initialise();

// Creates the player and camera items for this view and runs every post-load initialiser.
void XGameWnd::LevelPostLoad()
{
    Mission_AlarmLevel = 0.0f;
    theApp.m_bLevelLoaded = 1;
    gpPlayerItem[0] = 0;

    XItem *pPlayer = XItem::CreateItem(EXGeoFile::GetGeoFile(0x1000024), 0x1000005,
                                       EXRUNTIME_CLASS(XItemHandler_Player), EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    float idx = (float)m_Unk380;
    pPlayer->SetPosition(EXVector(idx + 22.0f, 0.0f, idx - 235.0f, 0.0f));
    pPlayer->SetRotation(EXQuaternion(0.0f, 7.854f, 0.0f, 0.0f));
    pPlayer->SetMapOn(theItemEnv.GetMapOn(pPlayer->Position()));
    pPlayer->ItemFlags().BitSet(4);
    gpPlayerItem[m_Unk380] = pPlayer;
    if (pPlayer->ItemHandler()) ((XItemHandler_Player *)pPlayer->ItemHandler())->m_PlayerIndex = m_Unk380;
    theItemEnv.AddItem(pPlayer, 5);

    gpCameraItem[m_Unk380] = XItem::CreateItem(0, 0, EXRUNTIME_CLASS(XItemHandler_Camera), EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    gpCameraItem[m_Unk380]->SetPosition(EXVector(0.0f, 0.0f, -100.0f, 0.0f));
    theItemEnv.AddItem(gpCameraItem[m_Unk380], 0);
    XItemHandler_Camera *pCam = (XItemHandler_Camera *)gpCameraItem[m_Unk380]->ItemHandler();
    if (pCam) {
        pCam->SetCamera(&m_Camera);
        pCam->SetViewIndex(m_Unk380);
    }

    EXItemAnimator_Map *pMap = theItemEnv.AddMap(pMapGeoFile, 0x85000000, 0);
    Trigger_PreLoad();
    WeaponProfile::Init();
    ArmourProfile::Init();
    Trigger_SetPlayerWeaponsCreated(0);
    theItemEnv.InitTriggers(pMap, TriggerInit);
    if (!Trigger_PlayerWeaponsCreated()) {
        if (!theApp.m_bIntroCutscene) FEHud::Instance(1)->WeaponArrayCreate(0, 0);
        XItemHandler_Player::Instance()->SetCurrentWeapon(0, 6);
        XItemHandler_Player::Instance()->SetCurrentWeapon(1, 3);
        XItemHandler_Player::Instance()->SetCurrentWeapon(2, 12);
        XItemHandler_Player::Instance()->SetCurrentWeapon(3, 1);
    }
    Mission_Initialise();
    if (!theApp.m_bIntroCutscene) TrafficInit();

    if (gCurrentMap_GeoHash != 0x1000007) {
        float i = (float)m_Unk380;
        gpPlayerItem[m_Unk380]->SetPosition(EXVector(i + 30.0f, 0.0f, i + 15.0f, 0.0f));
    }
    if (gCurrentMap_GeoHash == 0x1000009) {
        XItem *p = gpPlayerItem[0];
        p->SetPosition(EXVector(-242.0f, 5.0f, -11.0f, 0.0f));
        p->SetMapOn(theItemEnv.GetMapOn(p->Position()));
    }
    if (gStartPosition.x != -999.0f) {
        gpPlayerItem[m_Unk380]->SetPosition(gStartPosition);
        gpPlayerItem[m_Unk380]->SetRotation(EXQuaternion(gStartRotation.x, gStartRotation.y, gStartRotation.z, gStartRotation.w));
        Mission_RestartPosition = gStartPosition;
        Mission_RestartRotation = gStartRotation;
        m_Camera.SetPosition(gStartPosition);
    }
    XItemHandler_Player::Instance()->SetCloakDirect(gStartCloaked);
    if (!theApp.m_bIntroCutscene) {
        gAIControl.PostLoadInit();
        AirTraffic_Initialise();
    }
    Rain_Initialise();
    NightAndDay_Initialise();
    Text_Initialise();
    Gore_Initialise();
}

#include "EXStateSaver.h"
#include "EXResourceWnd.h"
#include "FEMainMenu.h"
#include "EXRenderInfo.h"
#include "EXBaseRenderEnv.h"
extern s8 gbDrawNavMesh, gbDrawNavRouting, gbDrawNavPaths;
extern s8 gDrawActorState, gShowLocations, gShowTimeTable, gSkinTest;
extern s8 Cheat_IgnoreObjectives, DBG_bActorsAndVehicles;
extern s8 gTriggerCollision, gEnvironmentCollision, gKillCounts, gTriggerDebugText, gVisualTriggers;

// Show a debug window if hidden, hide it if shown.
#define TOGGLE_WND(pWnd)                                                  \
    do {                                                                  \
        if (EXBaseDisplay::GetDisplay()->IsWndVisible(pWnd))              \
            EXBaseDisplay::GetDisplay()->RemoveWnd(pWnd);                 \
        else                                                              \
            EXBaseDisplay::GetDisplay()->AddWnd(pWnd);                    \
        return 1;                                                         \
    } while (0)

// Adds a command line switch if it is not already present, then restarts the app.
#define ADD_ARG_AND_RESTART(name)                                         \
    do {                                                                  \
        EXBaseApp *pApp = EXBaseApp::GetApp();                            \
        EXStringArray &args = pApp->Args();                               \
        s8 found = 0;                                                     \
        for (int i = 0; i < args.Size(); i++)                             \
            if (args[i] == EXString(name)) found = 1;                     \
        if (!found) args.Add(EXString(name));                             \
        pApp->Restart();                                                  \
        return 1;                                                         \
    } while (0)

// Sets the engine debug render mode: 'wire' is the render flag, 'mode' the shading mode byte.
static inline void SetDebugRenderMode(int wire, u8 mode)
{
    if (wire) EXBaseRenderEnv::GRenderInfo().RenderFlagsRef().BitSet(1);
    else EXBaseRenderEnv::GRenderInfo().RenderFlagsRef().BitClr(1);
    EXBaseRenderEnv::GRenderInfo().m_1a = mode;
}

// Debug menu commands (menu ids from the XGameWnd constructor's menu table).
s8 XGameWnd::MenuSelect(unsigned int id)
{
    if (id & 0x1000000) {
        // Level change request: drop the front-end singletons and push the new level.
        if (FEGetHud(0)) delete FEGetHud(0);
        if (FEGetRadar(0)) delete FEGetRadar(0);
        if (FEGetManager(0)) delete FEGetManager(0);
        if (FEGetPause(0)) delete FEGetPause(0);
        if (FEGetCutscene(0)) delete FEGetCutscene(0);
        if (FEGetDialog(0)) delete FEGetDialog(0);
        if (EVD::Instance(0)) delete EVD::Instance(0);
        if (XGetVisionWipe(0)) delete XGetVisionWipe(0);
        if (FEGetObjectives(0)) delete FEGetObjectives(0);
        if (FEGetLevels(0)) delete FEGetLevels(0);
        XGameState s((GameState::Flags)0x20, id);
        theApp.m_GameStates.Push(s);
        theApp.m_bGameStateChanged = 1;
        return 1;
    }
    if (id == 0x3c) {
        XGameState s((GameState::Flags)0x20, gCurrentMap_GeoHash);
        theApp.m_GameStates.Push(s);
        theApp.m_bGameStateChanged = 1;
        return 1;
    }
    switch (id) {
    case 1: ADD_ARG_AND_RESTART("viewer");
    case 2: EXBaseApp::GetApp()->Restart(); return 1;
    case 3: EXBaseDisplay::GetDisplay()->AddWnd(new EXResourceWnd); return 1;
    case 4: SetDebugRenderMode(0, 0); return 1;
    case 5: SetDebugRenderMode(1, 6); return 1;
    case 6: SetDebugRenderMode(0, 1); return 1;
    case 7: SetDebugRenderMode(1, 0); return 1;
    case 8: SetDebugRenderMode(0, 2); return 1;
    case 9: EXBaseDisplay::GetDisplay()->AddWnd(new EXWatchWnd); return 1;
    case 10: EXBaseDisplay::GetDisplay()->AddWnd(new EXStatsWnd); return 1;
    case 0xb: case 0xc: case 0xd: case 0xe: {
        // Select the display mode; remembered in the state file and applied by restarting
        EXStateSaver saver("RenderMode");
        if (saver.Restore(EXString("OldMode"), 0u) != id - 0xb) {
            saver.Save(EXString("OldMode"), id - 0xb);
            EXBaseApp::GetApp()->Restart();
        }
        SetDebugRenderMode(0, 0);
        return 1;
    }
    case 0xf: ADD_ARG_AND_RESTART("viewer2");
    case 0x10: SetDebugRenderMode(1, 3); return 1;
    case 0x11: SetDebugRenderMode(0, 5); return 1;

    case 0x46: gbDrawNavMesh = ~gbDrawNavMesh; return 1;
    case 0x47: gbDrawNavRouting = ~gbDrawNavRouting; return 1;
    case 0x48: gbDrawNavPaths = ~gbDrawNavPaths; return 1;
    case 0x49: gDrawActorState = gDrawActorState == 0; return 1;
    case 0x4a: gShowLocations = gShowLocations == 0; return 1;
    case 0x4b: gShowTimeTable = gShowTimeTable == 0; return 1;
    case 0x4c: { XGameState s((GameState::Flags)0x20, XApp::Frontend3DFile()); theApp.m_GameStates.Push(s); theApp.m_bGameStateChanged = 1; return 1; }
    case 0x4d: GameData::Instance(1)->Save(0); return 1;
    case 0x4e: GameData::Instance(1)->Load(0); return 1;
    case 0x4f: gSkinTest = gSkinTest == 0; return 1;
    case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
    case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f:
    case 0x60: case 0x61: case 0x62: case 0x63: case 0x64:
        gCutscenePlayer.Play(id - 0x51);
        return 1;
    case 0x66: AU_SpawnDebugActor(0x470000ca); return 1;  // HT_ActorProfile_TEST_MALE
    case 0x67: AU_SpawnDebugActor(0x470000cb); return 1;  // HT_ActorProfile_TEST_FEMALE
    case 0x68: AU_SpawnDebugActor(0x470000cc); return 1;  // HT_ActorProfile_TEST_PREDATOR
    case 0x6a: case 0x1008: TOGGLE_WND(XDebug::GetDebug()->PerformanceWnd());
    case 0x6b: m_Unk388[0] = m_Unk388[0] == 0; return 1;
    case 0x6c: m_Unk388[1] = m_Unk388[1] == 0; return 1;
    case 0x6d: case 0x1007: TOGGLE_WND(XDebug::GetDebug()->TesterWnd());
    case 0x6e: m_Unk388[2] = m_Unk388[2] == 0; return 1;
    case 0x6f: Cheat_IgnoreObjectives = Cheat_IgnoreObjectives == 0; return 1;
    case 0x71:
        m_Unk388[3] = m_Unk388[3] == 0;
        EXBaseApp::SetMaxFrameRate(m_Unk388[3] ? 30 : 60);
        return 1;
    case 0x74: m_Unk388[4] = m_Unk388[4] == 0; return 1;
    case 0x75: case 0x1001: TOGGLE_WND(XDebug::GetDebug()->ProfileWnd());
    case 0x76: DBG_bActorsAndVehicles = DBG_bActorsAndVehicles == 0; return 1;

    case 0xb0: { XGameState s((GameState::Flags)0x20, XApp::Frontend2DFile()); theApp.m_GameStates.Push(s); theApp.m_bGameStateChanged = 1; return 1; }
    case 200: gTriggerCollision = gTriggerCollision == 0; return 1;
    case 0xc9: gEnvironmentCollision = gEnvironmentCollision == 0; return 1;
    case 0xca: gKillCounts = gKillCounts == 0; return 1;
    case 0xcb: gTriggerDebugText = gTriggerDebugText == 0; return 1;
    case 0xcc: gVisualTriggers = gVisualTriggers == 0; return 1;
    case 300: XApp::SetGlobalGameFrameRate(4.0f); return 1;
    case 0x12d: XApp::SetGlobalGameFrameRate(2.0f); return 1;
    case 0x12e: XApp::SetGlobalGameFrameRate(1.0f); return 1;
    case 0x12f: XApp::SetGlobalGameFrameRate(0.5f); return 1;
    case 0x130: XApp::SetGlobalGameFrameRate(0.25f); return 1;

    case 0x3e8: {
        // Unlock every ritual level
        for (unsigned int bit = 0; bit < 0x20; bit++) {
            if (FELevelManager::Instance(1)->RitualLevelHashFromBitIndex(bit))
                XGetGameValues()->m_Rituals |= 1 << bit;
        }
        break;
    }
    case 0x3e9: {
        // Max out every weapon and special upgrade and mirror them into the saved values
        XItemHandler_Player *pPlayer = XItemHandler_Player::Instance();
        if (!pPlayer) return 1;
        for (unsigned int i = 0; i < 8; i++) pPlayer->SetWeaponUpgrade(i, 2);
        pPlayer->SetSpecialUpgrade(0, 3);
        pPlayer->SetSpecialUpgrade(1, 3);
        pPlayer->SetSpecialUpgrade(2, 3);
        pPlayer->SetSpecialUpgrade(3, 3);
        int *pSpecial = pPlayer->GetSpecialUpgradeArray();
        GameValues *pv = XGetGameValues();
        pv->m_Unk24[0] = pSpecial[0];
        pv->m_Unk24[1] = pSpecial[1];
        pv->m_Unk24[2] = pSpecial[2];
        pv->m_Unk24[3] = pSpecial[3];
        pv->m_Unk24[4] = pSpecial[4];
        int *pWeapon = pPlayer->GetWeaponUpgradeArray();
        pv = XGetGameValues();
        for (int i = 0; i < 8; i++) pv->m_Unk38[i] = pWeapon[i];
        break;
    }
    case 0x3ea: XGetGameValues()->m_Rewards = 0x7ffff; break;
    case 0x3eb:
        if (XItemHandler_Player::Instance()) XItemHandler_Player::Instance()->SetSpecialUpgrade(4, 0x7f);
        XGetGameValues()->m_Unk24[4] = 0x7f;
        break;
    case 0x3ec:
        for (int i = 0; i < (int)FELevelManager::Instance(1)->m_Unk4; i++) {
            if (XGetGameValues()->m_LevelStats[i].m_Unk0[0] == 0)
                XGetGameValues()->m_LevelStats[i].m_Unk0[0] = 0xe0f;
        }
        break;
    case 0x1000: TOGGLE_WND(XDebug::GetDebug()->HelpWnd());
    case 0x1002: TOGGLE_WND(XDebug::GetDebug()->PlayerWnd());
    case 0x1003: TOGGLE_WND(XDebug::GetDebug()->Player2Wnd());
    case 0x1004: TOGGLE_WND(XDebug::GetDebug()->CameraWnd());
    case 0x1005: TOGGLE_WND(XDebug::GetDebug()->AIWnd());
    case 0x1006: TOGGLE_WND(XDebug::GetDebug()->InventoryWnd());
    case 0x1009: TOGGLE_WND(XDebug::GetDebug()->FlagChannelWnd());
    default: return 1;
    }
    // cheat menu entries refresh the main menu if it is up
    if (FEMainMenu::Instance()) FEMainMenu::Instance()->RefreshPostLoadContent();
    return 1;
}
