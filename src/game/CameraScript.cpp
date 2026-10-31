#include "FEText.h"
#include "XItemHandler_CameraScript.h"
#include "XItemHandler_Camera.h"
#include "XItemHandler_Player.h"
#include "XItem.h"
#include "EXMemory.h"
#include "XGameWnd.h"
#include "EXMenu.h"
#include "EXHashCodeFile.h"
#include "EXCamera.h"
#include "EXGeoFile.h"
#include "Actor.h"
#include "XApp.h"
#include "FX_Gore.h"
#include "XItemHandler_Mimicry.h"
#include "FEHud.h"
#include "VisionWipe.h"
#include "FETopmost.h"
#include "EXString.h"
#include "EXItemAnimator_Script.h"
#include "EXBaseRenderEnv.h"
#include "EXRenderFogInfo.h"
#include "EXBaseApp.h"
#include "EXSwooshSysMgr.h"
#include "EXBaseSwooshSys.h"
#include "EXSwooshSys.h"
#include "XGamePad.h"
#include "EXItemCtrl_SnapShot.h"
#include "EXSphere.h"
#include "EXExtRenderInfoStandIn.h"
#include "EXGeoMap.h"
#include "EXGeoEntity_FaceRef.h"
#include "EXRayCastTemp.h"
#include "EXItemRender_Anim.h"
#include "EXTexSwapInfo.h"
#include "EXItemAnimator_Entity.h"
#include "EXItemAnimator_Anim.h"
#include "FEListener.h"
#include "EXItemAnimator_Camera.h"
#include "XItemHandler_ScriptBase.h"
#include "ActorOptions.h"
#include "FEMainMenu.h"
#include "FELevels.h"
#include "GeoSectionManager.h"
#include "EXGeoScript.h"
#include "EXGeoHeader.h"
#include "GameData.h"
#include "EXBaseSoundManager.h"

extern XGameWnd *gpGameWnd[4];

XItemHandler_CameraScript *gLastCamScript;
int ShadowCount;
EXVector ShadowCentre;
extern unsigned int gCurrentMap_GeoHash;
void Minesweeper();
void Rain_Stop();
// Script event as the animator passes it (only the fields used here).
struct GEOSCRIPT_EVENT {
    u16 m_00[3];
    u16 m_Frames;           // 0x06 duration in 30 Hz frames
    u32 m_08;
    unsigned int m_Hash;    // 0x0c HT_ScriptEvents_*
    u32 m_10;
    union {
        int i;
        unsigned int u;
        float f;
        signed char b;
    } m_Param[5];           // 0x14
};
char *HashToCharStr(unsigned int hash, char *pBuf);
void Stats_InitPageValues(unsigned int level);
struct _PRED_SKINS {
    u32 m_Hash[5];
};
extern EXFixedArray<_PRED_SKINS, 14> PRED_Skins;
EXAnimatorScriptCallbacks scb(0, 0, 0, 0);
CutscenePlayer gCutscenePlayer;

unsigned int Object_Callback_Wrapper(EXItemAnimator_Script *pScript, EXScriptThread *pThread, EXGeoScript *pGeo, struct GEOSCRIPT_REFCMD *pCmd);
void Camera_Callback_Wrapper(EXItemAnimator_Script *pScript, EXGeoScript *pGeo, struct GEOSCRIPT_CAMERA *pCmd, EXItemAnimator_Camera *pCam);

XItemHandler_CameraScript::XItemHandler_CameraScript()
{
    m_SkinOverride = 0;
    m_pCamera = 0;
    m_Flags = 0;
    m_FinishFlag = 0;
    m_FileHash = 0;
    m_NextLevel = 0;
    m_Unk38 = 0;
    m_ScriptHash = 0;
}

XItemHandler_CameraScript::~XItemHandler_CameraScript()
{
    if (gLastCamScript == this) gLastCamScript = 0;
    EXBaseRenderEnv::EnableRGBAMult(0);
}

void XItemHandler_Camera::SetCameraLookAt(EXVector v)
{
    m_CameraLookAt = v;
    m_pCamera->SetTarget(v);
}

void XItemHandler_Camera::SetCameraPosition(EXVector v)
{
    m_CameraPos = v;
    m_pCamera->SetPosition(v);
}

void CutscenePlayer::Init()
{
    m_Count = 0;
    memset(m_Queue, 0, sizeof(m_Queue));
    if (gpGameWnd[0]) {
        gpGameWnd[0]->getMenu()->ItemByID(0x50, 1)->m_pSubMenu->DelAllItems();
        EXString none("None");
        unsigned int id = 0x51;
        gpGameWnd[0]->getMenu()->ItemByID(0x50, 1)->m_pSubMenu->AddItem(none, id);
    }
}

void CutscenePlayer::Add(unsigned int file, unsigned int hash)
{
    if (m_Count < 20) {
        m_Queue[m_Count].file = file;
        m_Queue[m_Count].hash = hash;
        unsigned int nc = m_Count + 1;
        m_Count = nc;
        EXMenu *pMenu = gpGameWnd[0]->getMenu()->ItemByID(0x50, 1)->m_pSubMenu;
        if (m_Count == 1) pMenu->DelAllItems();
        EXString name(EXHashCodeFile::Debug_HashCodeName(hash));
        pMenu->AddItem(name, m_Count + 0x50);
    }
}

void CutscenePlayer::Play(unsigned int i)
{
    if (i < m_Count) XItemHandler_CameraScript::Create(m_Queue[i].file, m_Queue[i].hash);
}

EXRuntimeClass *XItemHandler_CameraScript::_GetBaseClass() { return EXRUNTIME_CLASS(XItemHandler); }
const EXRuntimeClass XItemHandler_CameraScript::classXItemHandler_CameraScript = {
    EXRUNTIME_CLASS(XItemHandler), "XItemHandler_CameraScript", sizeof(XItemHandler_CameraScript),
    (void *(__cdecl *)())XItemHandler_CameraScript::CreateObject};

EXRuntimeClass *XItemHandler_CutsceneTrigger::_GetBaseClass() { return EXRUNTIME_CLASS(XItemHandler_CutsceneTrigger); }
const EXRuntimeClass XItemHandler_CutsceneTrigger::classXItemHandler_CutsceneTrigger = {
    0, "XItemHandler_CutsceneTrigger", sizeof(XItemHandler_CutsceneTrigger),
    (void *(__cdecl *)())XItemHandler_CutsceneTrigger::CreateObject};

XItemHandler_CutsceneTrigger::~XItemHandler_CutsceneTrigger() {}

s8 XItemHandler_CutsceneTrigger::Initialise() { return EXItemHandler::Initialise() != 0; }

XItemHandler_CutsceneTrigger *XItemHandler_CutsceneTrigger::CreateObject() { return new XItemHandler_CutsceneTrigger; }

XItemHandler_CameraScript *XItemHandler_CameraScript::CreateObject() { return new XItemHandler_CameraScript; }

// NEAR (see check): builds the cutscene's script animator from the item's geo reference, hooks the object
// and camera callbacks, becomes the current cutscene, and sizes and clears the per-thread item list
// (one per script thread) and the three 32/40-entry tables.
s8 XItemHandler_CameraScript::Initialise()
{
    EXItemAnimator *pAnim = EXItemAnimator::CreateFrom(m_pItem->GeoFile(), m_pItem->RefObject());
    if (!pAnim) return 0;
    m_pItem->AddAnimator(pAnim);
    scb.m_pCreateThread = (int (__cdecl *)(EXItemAnimator_Script *, EXScriptThread *, EXGeoScript *, const GEOSCRIPT_CMD *))Object_Callback_Wrapper;
    scb.m_pThreadEnd = 0;
    scb.m_pCamera = (void (__cdecl *)(EXItemAnimator_Script *, EXGeoScript *, GEOSCRIPT_CMD *, EXItemAnimator *))Camera_Callback_Wrapper;
    ((EXItemAnimator_Script *)pAnim)->RegisterCallbacks(&scb);
    gLastCamScript = this;
    m_Unk38 = 0;
    m_bFocusBlur = 0;
    m_bMotionBlur = 0;
    m_bNoise = 0;
    m_bGlow = 0;
    m_bNoPlayerControl = 0;
    m_bNoMotionBlur = 0;
    m_ThreadObject.SetSize(((EXItemAnimator_Script *)pAnim)->NumThreads(), -1);
    m_ObjectCloaked.SetSize(40, -1);
    m_StreamSound.SetSize(32, -1);
    m_StreamSoundFrame.SetSize(32, -1);
    int i;
    for (i = 0; i < m_ThreadObject.Size(); i++) m_ThreadObject[i] = 0;
    for (i = 0; i < m_ObjectCloaked.Size(); i++) m_ObjectCloaked[i] = 0;
    for (i = 0; i < m_StreamSound.Size(); i++) m_StreamSound[i] = 0;
    for (i = 0; i < m_StreamSoundFrame.Size(); i++) m_StreamSoundFrame[i] = 0;
    return 1;
}

// NEAR (see check): a script object command: the cutscene predator wears the player's skin (or the
// script's override), the thread's animator is created, and an animated or entity one gets fresh render
// settings, its controllers posed at the command's frame and the cutscene lighting; flagged objects
// (0x15000000-27) get the 0x2000 render flag, and the predator keeps the player's cloak shell (the
// cloak is then dropped). Root-script objects are recorded per thread.
unsigned int Object_Callback_Wrapper(EXItemAnimator_Script *pScript, EXScriptThread *pThread, EXGeoScript *pGeo, struct GEOSCRIPT_REFCMD *pCmd)
{
    XItemHandler_CameraScript *p = (XItemHandler_CameraScript *)pScript->GetOwnerHandler();
    if (((GEOSCRIPT_CMD *)pCmd)->m_0c == 0x15000003 && XItemHandler_Player::GetInstance()) {
        unsigned int skin = p->m_SkinOverride;
        if (!skin) skin = XItemHandler_Player::GetInstance()->GetCurrentSkinHash();
        ((GEOSCRIPT_CMD *)pCmd)->m_18 = skin;
    }
    pScript->CreateThreadAnimator(pGeo, pThread, pCmd);
    EXItemAnimator_Anim *pAnim = (EXItemAnimator_Anim *)pThread->Placement();
    if (pAnim) {
        if (pAnim->IsKindOf(EXRUNTIME_CLASS(EXItemAnimator_Anim))) goto found;
        if (pAnim->IsKindOf(EXRUNTIME_CLASS(EXItemAnimator_Entity))) goto found;
    }
    goto record;
found:
    if (pAnim->IsKindOf(EXRUNTIME_CLASS(EXItemAnimator_Anim))) pAnim->m_pSkin->RenderInfo().SetDefault();
    else pAnim->RenderInfo().SetDefault();
    pThread->UpdateThreadControllers(pScript, pGeo, (float)(short)((GEOSCRIPT_CMD *)pCmd)->m_04);
    p->LightThreadItem(pAnim);
    if (((GEOSCRIPT_CMD *)pCmd)->m_0c < 0x15000028 && p->m_ObjectCloaked.GetData()[((GEOSCRIPT_CMD *)pCmd)->m_0c - 0x15000000]) {
        const EXRuntimeClass *rc = pAnim->GetRuntimeClass();
        do {
            if (rc == EXRUNTIME_CLASS(EXItemAnimator_Anim)) {
                pAnim->m_pSkin->RenderInfo().m_renderFlags.BitSet(0x2000);
                goto cloak;
            }
            rc = rc->m_pBaseClass;
        } while (rc);
        pAnim->RenderInfo().m_renderFlags.BitSet(0x2000);
    }
cloak:
    if (((GEOSCRIPT_CMD *)pCmd)->m_0c == 0x15000003 && XItemHandler_Player::GetInstance() && XItemHandler_Player::GetInstance()->GetCloak() &&
        !p->m_ObjectCloaked.GetData()[((GEOSCRIPT_CMD *)pCmd)->m_0c - 0x15000000]) {
        EXGeoRefObject ref;
        ref.m_pGeoFile = EXGeoFile::GetGeoFile(0x1000024);
        ref.m_hashRef = 0xd0000cc;
        pAnim->AddCloneSkin(ref);
        XItemHandler_Player::GetInstance()->SetCloak(0);
    }
record:
    if (pScript == (EXItemAnimator_Script *)p->GetItem()->GetAnimator()) p->m_ThreadObject.GetData()[(u8)((GEOSCRIPT_CMD *)pCmd)->m_0a] = ((GEOSCRIPT_CMD *)pCmd)->m_0c;
    return 1;
}

// NEAR (see check): a script camera command: the cutscene follows this camera from now on, the game
// camera takes its lens, position, look-at and up at once, and the front end's attract cameras
// (0x15000017-19) are registered with the main menu.
void Camera_Callback_Wrapper(EXItemAnimator_Script *pScript, EXGeoScript *pGeo, struct GEOSCRIPT_CAMERA *pCmd, EXItemAnimator_Camera *pCam)
{
    XItemHandler_CameraScript *p = (XItemHandler_CameraScript *)pScript->GetOwnerHandler();
    if (!p) return;
    p->m_pCamera = pCam;
    if (pCam->m_focalLength != 0.0f && pCam->m_apertureHeight != 0.0f) {
        float fov = (float)atan(pCam->m_apertureHeight / (pCam->m_focalLength + pCam->m_focalLength)) * 2.0f;
        XGetCamera(0)->SetCutsceneFOV(fov);
    }
    EXVector pos(pCam->m_mtx.r[3].x, pCam->m_mtx.r[3].y, pCam->m_mtx.r[3].z, 0.0f);
    XGetCamera(0)->SetCameraPosition(pos);
    EXVector fwd(0.0f, 0.0f, 1.0f, 0.0f), f;
    D3DXVec4Transform(&f, &fwd, &pCam->m_mtx);
    EXVector look(f.x + pos.x, f.y + pos.y, f.z + pos.z, f.w);
    XGetCamera(0)->SetCameraLookAt(look);
    EXVector upv(0.0f, 1.0f, 0.0f, 0.0f), up;
    D3DXVec4Transform(&up, &upv, &pCam->m_mtx);
    XGetCamera(0)->GetCamera()->SetWorldUp(up);
    unsigned int h = ((GEOSCRIPT_CMD *)pCmd)->m_0c;
    if (h > 0x15000016 && h < 0x1500001a) FEMainMenu::Instance()->AddCamera(h, pCam);
}

EXItemAnimator *XItemHandler_CameraScript::GetThreadItem(int i)
{
    return ((EXItemAnimator_Script *)m_pItem->GetAnimator())->Thread(i).Placement();
}

// NEAR (see check): starts a cutscene script from a geo file (default the current map) unless one is
// running or the mission has failed: a script item at the origin, the player's vision mode saved and
// reset, his control taken, messages, voice-overs and mines cleared (blood too for NIS33, the fade
// cancelled for two scripts), the script preloaded and the hud told; the window clears to black.
XItem *XItemHandler_CameraScript::Create(unsigned int file, unsigned int hash)
{
    if (theApp.m_bCutscene || theApp.m_MissionState == 3 || theApp.m_MissionState == 4) return 0;
    if (!file) file = gCurrentMap_GeoHash;
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(file, 0, 0);
    FECutscene::Instance(1)->SetCutsceneGeoHash(file);
    if (!pFile) {
        Text_PrintStatus(EXString("Cutscene failed to load"), ex::RGBA(0x7f, 0, 0, 0x80), 5, -1, 0, 0);
        return 0;
    }
    XItem *pItem = XItem::CreateItem(pFile, hash, EXRUNTIME_CLASS(XItemHandler_CameraScript), EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    if (pItem) {
        XItemHandler_CameraScript *pThis = (XItemHandler_CameraScript *)pItem->m_pItemHandler;
        pItem->SetPosition(EXVector(0.0f, 0.0f, 0.0f, 0.0f));
        pItem->SetRotation(EXQuaternion(0.0f, 0.0f, 0.0f, 0.0f));
        theItemEnv.AddItem(pItem, 4);
        pThis->m_SavedVisionMode = 0;
        if (XItemHandler_Player::GetInstance()) {
            pThis->m_SavedVisionMode = XItemHandler_Player::GetInstance()->GetVisionMode();
            VisionWipe::Instance(1)->KillWipe();
            XItemHandler_Player::GetInstance()->CommitVisionMode(0);
            FEHud::Instance(1)->CommitVisionMode(0);
            XItemHandler_Player::GetInstance()->SetPlayerHasControl(0);
            Text_DeleteUnique(4);
            Text_DeleteUnique(6);
            Text_DeleteMessages(3);
            StopAllVoiceOvers();
            Minesweeper();
            if (hash == 0x4000402) Gore_Bloodcleaner();
            else if (hash == 0x4000711 || hash == 0x4000797) theApp.CancelFade();
        }
        pThis->m_ScriptHash = hash;
        theApp.m_bCutscene = 1;
        pThis->m_FileHash = file;
        if (file == gCurrentMap_GeoHash) pThis->m_FileHash = 0;
        pThis->Preload(1);
    }
    if (FEHud::Instance(0)) FEHud::Instance(1)->NotifyCutscene(1);
    m_BkColor = gpGameWnd[0]->BkColor();
    m_bClearBitSet = (gpGameWnd[0]->m_Flags >> 1) & 1;
    gpGameWnd[0]->SetBkColor(ex::RGBA(0, 0, 0, 0x80));
    gpGameWnd[0]->m_Flags.BitSet(2);
    Actor::NotifyCutscene(1);
    return pItem;
}

// NEAR (see check): loads (b) or releases the geo sections a cutscene script references: its objects
// (except the cutscene predator), the cloak effect, and the predator skin it switches to (by the selected
// skin); when loading it also lists the script's streamed sounds and their start frames. Done
// synchronously.
void XItemHandler_CameraScript::Preload(signed char b)
{
    EXGeoScript *pGeo = m_pItem->GeoFile()->GeoHeader()->GeoScript(m_pItem->RefObject());
    int n = 0;
    signed char bAsync = GeoSectionManager::GetAsyncMode();
    GeoSectionManager::SetAsyncMode(0);
    if (pGeo) {
        int off = 0;
        GEOSCRIPT_CMD *pCmd;
        do {
            pCmd = (GEOSCRIPT_CMD *)&pGeo->m_Data[off];
            switch (pCmd->m_Type) {
            case 2:
                if (pCmd->m_0c == 0x15000003) break;
                if (b) GeoSectionManager::Add(pCmd->m_14, pCmd->m_18);
                else GeoSectionManager::Del(pCmd->m_14, pCmd->m_18);
                break;
            case 5:
                if (b) {
                    unsigned int hash = pCmd->m_20;
                    if (EXBaseSoundManager::GetSoundManager()->SoundDetails(hash).m_Unka) {
                        m_StreamSound.GetData()[n] = hash;
                        m_StreamSoundFrame.GetData()[n] = (short)pCmd->m_04;
                        n++;
                    }
                }
                break;
            case 0xb: {
                unsigned int skin;
                if (pCmd->m_0c == 0x16000055) {
                    if (pCmd->m_18 == 0x15000003) break;
                    if (b) GeoSectionManager::Add(0x10000e3, 0xd00012d);
                    else GeoSectionManager::Del(0x10000e3, 0xd00012d);
                    break;
                } else if (pCmd->m_0c == 0x16000072) {
                    int sel = (short)GameData::Instance(1)->m_Values.m_Unk58;
                    if (sel == -1) sel = 0;
                    skin = PRED_Skins[sel].m_Hash[6];
                } else if (pCmd->m_0c == 0x1600008b) {
                    int sel = (short)GameData::Instance(1)->m_Values.m_Unk58;
                    if (sel == -1) sel = 0;
                    skin = PRED_Skins[sel].m_Hash[4];
                } else {
                    break;
                }
                if (b) GeoSectionManager::Add(0x1000024, skin);
                else GeoSectionManager::Del(0x1000024, skin);
                break;
            }
            }
            off += pCmd->m_Size;
        } while (pCmd->m_Type != 0x12);
    }
    GeoSectionManager::SetAsyncMode(bAsync);
}

// NEAR (see check): ends a cutscene (callable with no script: this may be 0). Releases its sections,
// sets its finish flag, drops its geo file, and moves on to its next level (the end-of-game scripts and a
// change of base go through the stats screens; same base adds the play time and pushes the level). Then
// either chains straight into the next script (file/hash, keeping flags and destinations) or hands
// control back: camera mode 5, player unhidden and unmasked (input locked briefly unless the intro),
// vision restored, cutscene hud removed and the fade in (out-in for two scripts).
void XItemHandler_CameraScript::FinishScript(unsigned int file, unsigned int hash)
{
    XGetCamera(0)->GetCamera()->SetWorldUp(EXVector(0.0f, 1.0f, 0.0f, 0.0f));
    if (!this) {
        gLastCamScript = 0;
        if (!hash) return;
    } else {
        Preload(0);
        if (m_FinishFlag) FlagChannels[m_FinishFlag] = 1;
        if (m_FileHash) EXGeoFile::DeLoadGeoFile(m_FileHash, 0);
        HashToCharStr(FELevelManager::Instance(1)->LevelFromHash(m_NextLevel, 0)->Text(), 0);
        EXHashCodeFile::Debug_HashCodeName(m_NextLevel);
        if (m_NextLevel) {
            if (m_NextLevel == 0x1000186 || m_NextLevel == 0x1000187 || m_NextLevel == 0x1000185 || m_NextLevel == 0x1000196) {
                Stats_InitPageValues(m_NextLevel);
                FEMainMenu::DisplayStats(m_NextLevel, 0, 0, 1);
            } else {
                FELevelManager *pA = FELevelManager::Instance(1);
                unsigned int cur = FELevelManager::Instance(1)->BaseFromLevel(gCurrentMap_GeoHash);
                if (pA->BaseFromLevel(m_NextLevel) != cur) {
                    Stats_InitPageValues(m_NextLevel);
                    FEMainMenu::DisplayStats(m_NextLevel, 0, 0, 0);
                    goto done;
                }
                GameData::Instance(1)->m_Values.m_Stats.m_Unk0[0] += (int)theApp.m_LevelPlayTimer;
            }
            theApp.GameStatePush((GameState::Flags)0x20, m_NextLevel);
        }
    done:
        Remove_CSMF();
        m_pItem->ItemFlags().BitSet(0x10);
        gLastCamScript = 0;
        gpGameWnd[0]->SetBkColor(m_BkColor);
        if (!m_bClearBitSet) gpGameWnd[0]->m_Flags.BitClr(2);
        if (!hash) {
            XGetCamera(0)->SetMode(5);
            XItemHandler_Player::GetInstance()->SetPlayerHasControl(1);
            XItemHandler_Player::GetInstance()->m_AnalogMask = 0;
            XItemHandler_Player::GetInstance()->m_ButtonMask = 0;
            XItemHandler_Player::GetInstance()->Hide(0, 0);
            gActorPause = 0;
            gActorHide = 0;
            if (!theApp.IsIntroCutscene()) {
                FEHud::Instance(1)->m_DisablePlayer = 10;
                XItemHandler_Player::GetInstance()->m_ButtonLock = 0xffffff5f;
            }
            VisionWipe::Instance(1)->KillWipe();
            XItemHandler_Player::GetInstance()->CommitVisionMode((unsigned char)m_SavedVisionMode);
            FEHud::Instance(1)->CommitVisionMode(m_SavedVisionMode);
            if (FECutscene::Instance(0)) delete FECutscene::Instance(0);
            if (m_ScriptHash == 0x40003e7)
                theApp.ApplyFadeOutIn(0.0f, ex::RGBA(0, 0, 0, 0x80), 0, 1);
            else if (m_ScriptHash == 0x4000733)
                theApp.ApplyFadeOutIn(0.0f, ex::RGBA(0, 0, 0, 0x80), 0, 1);
            else
                theApp.ApplyFadeIn(2.0f, ex::RGBA(0, 0, 0, 0x80), 0);
            theApp.m_bCutscene = 0;
            theApp.ResetMissionTimer();
            goto notify;
        }
    }
    {
        theApp.m_bCutscene = 0;
        XItemHandler_CameraScript *pNext = (XItemHandler_CameraScript *)Create(file, hash)->m_pItemHandler;
        if (this) {
            pNext->m_Flags = m_Flags;
            pNext->m_FinishFlag = m_FinishFlag;
            pNext->m_NextLevel = m_NextLevel;
        }
    }
notify:
    if (FEHud::Instance(0)) FEHud::Instance(1)->NotifyCutscene(0);
    Actor::NotifyCutscene(0);
    theItemEnv.UnlockMusic();
}

// NEAR (see check): a cutscene trigger: removed once its cancel flag (data 3) is set; when its start flag
// (data 0, none: at once) is set during play, starts its script passing on its options, finish flag
// (data 1) and next level (data 2). Midsummer's flag 200 instead plays the ending movie (the S variant
// when a setting is on) and moves to the second level.
s8 XItemHandler_CutsceneTrigger::DoUpdate()
{
    if (m_Data[3] && FlagChannels[m_Data[3]]) m_pItem->ItemFlags() = 0x10;
    if (m_Data[0] && !FlagChannels[m_Data[0]]) return 1;
    if (theApp.GameStateCurrent()->m_State != 9) return 1;
    if (gCurrentMap_GeoHash == 0x100006a && m_Data[0] == 200) {
        m_Data[2] = FELevelManager::Instance(1)->LevelFromIndex(1);
        Stats_InitPageValues(m_Data[2]);
        GameData::Instance(1)->SettingsStore();
        if (GameData::Instance(1)->m_Values.m_Unk1c[2])
            theApp.PlayMovie("mdsmrS", m_Data[2]);
        else
            theApp.PlayMovie("midsmr", m_Data[2]);
        m_Data[2] = 0;
        m_pItem->ItemFlags() = 0x10;
        return 1;
    }
    EXHashCodeFile::Debug_HashCodeName(m_Hash);
    EXHashCodeFile::Debug_HashCodeName(m_File);
    XItem *pItem = XItemHandler_CameraScript::Create(m_File, m_Hash);
    m_pItem->ItemFlags() = 0x10;
    if (pItem) {
        XItemHandler_CameraScript *p = (XItemHandler_CameraScript *)pItem->m_pItemHandler;
        p->m_Flags = m_Options;
        p->m_FinishFlag = m_Data[1];
        p->m_NextLevel = m_Data[2];
    }
    return 1;
}

// NEAR (see check): per frame unless handed back: precaches the script's streamed sounds 4-30 frames
// ahead, holds the player, drives the game camera from the script camera (position, look-at, up and the
// field of view from its lens; mode 4) with skip on the button after the first second unless the script
// forbids it, applies the window effects, and relights the cutscene in normal vision.
s8 XItemHandler_CameraScript::DoUpdate()
{
    if (m_Flags & 0x8000) return 1;
    EXItemAnimator_Script *pScript = (EXItemAnimator_Script *)m_pItem->GetAnimator();
    for (int i = 0; i < 32; i++) {
        if (!m_StreamSound.GetData()[i]) continue;
        int d = m_StreamSoundFrame.GetData()[i] - (int)pScript->ObjectFrame();
        if (d < 4)
            m_StreamSound.GetData()[i] = 0;
        else if (d < 30)
            EXBaseSoundManager::GetSoundManager()->PreCache(m_StreamSound.GetData()[i], 2.0f);
    }
    if (m_bNoPlayerControl) XItemHandler_Player::GetInstance()->SetPlayerHasControl(0);
    if (m_pCamera) {
        if (m_pCamera->m_focalLength != 0.0f && m_pCamera->m_apertureHeight != 0.0f) {
            float fov = (float)atan(m_pCamera->m_apertureHeight / (m_pCamera->m_focalLength + m_pCamera->m_focalLength)) * 2.0f;
            XGetCamera(0)->SetCutsceneFOV(fov);
        }
        EXVector pos(m_pCamera->m_mtx.r[3].x, m_pCamera->m_mtx.r[3].y, m_pCamera->m_mtx.r[3].z, 0.0f);
        XGetCamera(0)->SetCameraPosition(pos);
        EXVector fwd(0.0f, 0.0f, 1.0f, 0.0f), f;
        D3DXVec4Transform(&f, &fwd, &m_pCamera->m_mtx);
        EXVector look(f.x + pos.x, f.y + pos.y, f.z + pos.z, f.w);
        XGetCamera(0)->SetCameraLookAt(look);
        EXVector upv(0.0f, 1.0f, 0.0f, 0.0f), up;
        D3DXVec4Transform(&up, &upv, &m_pCamera->m_mtx);
        XGetCamera(0)->GetCamera()->SetWorldUp(up);
        XGetCamera(0)->SetMode(4);
        if (pScript->ObjectFrame() > 60.0f && !(m_Flags & 1) &&
            FEListener::QueryButton(0x2000, GameControllerRemovedSys::Controller(), 1)) {
            FinishScript(0, 0);
            XItemHandler_Player::GetInstance()->m_ButtonLock = 0xffffffff;
        }
    }
    if (m_bFocusBlur) {
        gpGameWnd[0]->Effects().BitSet(4);
        gpGameWnd[0]->SetFocusBlurFactor(m_FocusBlurFactor);
        gpGameWnd[0]->SetFocusBlurDistance(m_FocusBlurDistance);
    }
    if (m_bMotionBlur) {
        gpGameWnd[0]->Effects().BitSet(8);
        if (m_bNoMotionBlur) gpGameWnd[0]->SetMotionBlurFactor(0.0f);
        else gpGameWnd[0]->SetMotionBlurFactor(m_MotionBlur);
    }
    if (m_bNoise) {
        gpGameWnd[0]->Effects().BitSet(2);
        gpGameWnd[0]->SetNoiseIntensity(m_Noise);
    }
    if (m_bGlow) {
        gpGameWnd[0]->SetFocusBlurDistance(0.0f);
        gpGameWnd[0]->SetGlowIntensity(m_Glow);
        gpGameWnd[0]->SetFocusBlurFactor((int)(m_Glow * 10.0f));
        gpGameWnd[0]->Effects() = 0xd;
        if (m_bNoMotionBlur) gpGameWnd[0]->SetMotionBlurFactor(0.0f);
        else gpGameWnd[0]->SetMotionBlurFactor(m_Glow * 0.5f);
        m_Glow -= XApp::GameFrameRateMul() * 0.001f;
        if (m_Glow <= 0.0f) m_bGlow = 0;
    }
    m_bNoMotionBlur = 0;
    if (!XItemHandler_Player::GetInstance()->GetVisionMode()) UpdateLighting();
    return 1;
}

// NEAR (see check): puts the cutscene's animated thread items into or out of thermal vision (mode 1): heat
// render with the temperature, drone and predator body textures swapped for their thermal versions, or
// the swaps stopped; colour multipliers reset either way.
void XItemHandler_CameraScript::HandleVisionModeTextureSwaps(unsigned char mode)
{
    EXItemAnimator_Script *pScript = (EXItemAnimator_Script *)m_pItem->GetAnimator();
    if (!pScript) return;
    for (unsigned int i = 0; i < pScript->NumThreads(); i++) {
        EXItemAnimator *pAnim = pScript->Thread(i).Placement();
        if (!pAnim) continue;
        const EXRuntimeClass *rc = pAnim->GetRuntimeClass();
        do {
            if (rc == EXRUNTIME_CLASS(EXItemAnimator_Anim)) {
                EXExtRenderInfo *pExt = pAnim->GetExtRenderInfo();
                EXTexSwapInfo *pSwap = pAnim->SetTexSwapInfo();
                if (pExt && pSwap) {
                    pAnim->RenderInfo().EnableAlphaMult(0);
                    pAnim->RenderInfo().EnableRGBMult(0);
                    if (mode == 1) {
                        pAnim->RenderInfo().m_renderFlags.BitSet(8);
                        pAnim->RenderInfo().SetRGBAMult(1.0f, 1.0f, 1.0f, 1.0f);
                        pAnim->RenderInfo().EnableHeatRender(1);
                        pSwap->SetSwap(0x6000075, 0x1000016, 0x6000072);
                        pSwap->SetSwap(0x6000077, 0x1000016, 0x6000061);
                        pSwap->SetSwap(0x6000076, 0x1000016, 0x6000062);
                        pSwap->SetSwap(0x6000074, 0x1000016, 0x6000063);
                        pSwap->SetSwap(0x6000078, 0x1000016, 0x6000062);
                        pSwap->SetSwap(0x6000047, 0x1000016, 0x6000062);
                        pSwap->SetSwap(0x6000048, 0x1000016, 0x6000062);
                        pSwap->SetSwap(0x6000049, 0x1000016, 0x6000063);
                        pSwap->SetSwap(0x6000046, 0x1000016, 0x6000062);
                    } else {
                        pAnim->RenderInfo().m_renderFlags.BitClr(8);
                        pAnim->RenderInfo().SetRGBAMult(1.0f, 1.0f, 1.0f, 1.0f);
                        pAnim->RenderInfo().EnableHeatRender(0);
                        pSwap->StopAllSwaps();
                    }
                }
                break;
            }
            rc = rc->m_pBaseClass;
        } while (rc);
    }
}

// NEAR (see check): lights an animated cutscene item like the map would: three map lights around a point
// half a unit above its root (directions from the lights, their colours; the key light defaults to twice
// the ground colour under it), ambient half the ground colour. Refraction-rendered items get flat
// lighting. Items on screen add to the shadow centre.
void XItemHandler_CameraScript::LightThreadItem(EXItemAnimator_Anim *pAnim)
{
    EXExtRenderInfo *pExt = pAnim->GetExtRenderInfo();
    if (!pExt) return;
    u16 flags;
    const EXRuntimeClass *rc = pAnim->GetRuntimeClass();
    do {
        if (rc == EXRUNTIME_CLASS(EXItemAnimator_Anim)) {
            flags = pAnim->m_pSkin->RenderInfo().m_renderFlags;
            goto got;
        }
        rc = rc->m_pBaseClass;
    } while (rc);
    flags = pAnim->RenderInfo().m_renderFlags;
got:
    if ((flags >> 13) & 1) {
        pExt->SetDefault();
        pExt->Ambient().set(1.0f, 1.0f, 1.0f, 0.0f);
        return;
    }
    EXVector dir[3];
    EXVector col[3];
    EXLightInfo lights[3];
    memset(dir, 0, sizeof dir);
    memset(col, 0, sizeof col);
    memset(lights, 0, sizeof lights);
    {
        float len = (float)sqrt(3.0);
        dir[0].x = -1.0f;
        dir[0].y = -1.0f;
        dir[0].z = -1.0f;
        dir[0].w = 0.0f;
        if (len > 1e-5f) {
            float k = 1.0f / len;
            dir[0].x = k * -1.0f;
            dir[0].y = k * -1.0f;
            dir[0].z = k * -1.0f;
            dir[0].w = k * 0.0f;
        }
    }
    EXVector pos = pAnim->Position();
    EXMotionData md;
    u8 order = pAnim->CalcMotionData(md, 0, 0xbf00);
    EXVector off = md.m_pos;
    EXMatrix m;
    m.set_rotate(pAnim->m_rotation, order);
    EXVector o;
    D3DXVec4Transform(&o, &off, &m);
    off = o;
    pos.x += o.x;
    pos.y += o.y;
    pos.z += o.z;
    pos.w = 1.0f;
    EXVector w;
    D3DXVec4Transform(&w, &pos, &m_pItem->m_mtx);
    EXVector p(w.x, w.y + 0.5f, w.z, w.w);
    EXRayCastTemp r;
    EXVector down(0.0f, -15.0f, 0.0f, 0.0f);
    r.Set(p, down, 0, 0, 0, 0);
    r.m_layer.m_flags |= 0x400;
    theItemEnv.DoRayCastToMap(r, 0);
    EXVector ground;
    if (r.m_bHit)
        ground = r.m_face.GetFaceColourV(2, p);
    else
        ground = EXVector(0.5f, 0.5f, 0.5f, 0.0f);
    EXVector up(0.0f, 1.0f, 0.0f, 0.0f);
    theItemEnv.CalcLightsV(lights, 3, p, up, 0, EXMapOn(-1));
    for (int i = 0; i < 3; i++) {
        if (!lights[i].m_Valid) continue;
        col[i] = lights[i].m_Colour;
        dir[i] = p - lights[i].m_Position;
        dir[i].w = 0.0f;
        col[i].w = 1.0f;
        dir[i].Normalise();
    }
    if (!lights[0].m_Valid) col[0] = ground * 2.0f;
    EXVector amb = ground * 0.5f;
    pExt->SetLightData(dir[0], dir[1], dir[2], col[0], col[1], col[2], amb);
    EXSphere sph;
    sph.Set(p, 3.0f);
    if (XGetCamera(0)->BoundingVolumeOnScreen(sph)) {
        ShadowCentre.x += p.x;
        ShadowCentre.y += p.y;
        ShadowCentre.z += p.z;
        ShadowCentre.w += p.w;
        ShadowCount++;
    }
}

// NEAR (see check): relights every animated or entity thread item of the cutscene (LightThreadItem) and
// averages the shadow casters' centre.
void XItemHandler_CameraScript::UpdateLighting()
{
    EXItemAnimator_Script *pScript = (EXItemAnimator_Script *)m_pItem->GetAnimator();
    if (!pScript) return;
    ShadowCentre.x = 0.0f;
    ShadowCentre.y = 0.0f;
    ShadowCentre.z = 0.0f;
    ShadowCentre.w = 0.0f;
    ShadowCount = 0;
    for (unsigned int i = 0; i < pScript->NumThreads(); i++) {
        EXItemAnimator *pAnim = pScript->Thread(i).Placement();
        if (!pAnim) continue;
        const EXRuntimeClass *rc = pAnim->GetRuntimeClass();
        do {
            if (rc == EXRUNTIME_CLASS(EXItemAnimator_Anim)) goto light;
            rc = rc->m_pBaseClass;
        } while (rc);
        rc = pAnim->GetRuntimeClass();
        do {
            if (rc == EXRUNTIME_CLASS(EXItemAnimator_Entity)) goto light;
            rc = rc->m_pBaseClass;
        } while (rc);
        continue;
    light:
        LightThreadItem((EXItemAnimator_Anim *)pAnim);
    }
    if (ShadowCount <= 1) return;
    float k = 1.0f / (float)ShadowCount;
    ShadowCentre.x *= k;
    ShadowCentre.y *= k;
    ShadowCentre.z *= k;
    ShadowCentre.w *= k;
}

// NEAR (see check): the cutscene script's events (HT_ScriptEvents_*): end of script, flag channels,
// freezing and hiding the player and actors, vision mode (with a wipe), subtitles, letterbox and credits,
// cloaking objects, window effects (focus and motion blur, noise, glow with its fog), rumble, text, the
// next camera angles, fades (seven colours), head and belt textures cleared, swooshes and muzzle effects
// on objects, the predator's health and skin, rain and hud scripts.
u32 XItemHandler_CameraScript::DoScriptCmdEvent(GEOSCRIPT_EVENT *pEvent, EXItemAnimator *pAnim)
{
    m_Unk38 = pEvent->m_Hash;
    XItemHandler_Player *pPlayer = XItemHandler_Player::GetInstance();
    if (!theApp.m_bCutscene) return 0;
    EXItemAnimator_Script *pScript = (EXItemAnimator_Script *)pAnim;
    switch (pEvent->m_Hash) {
    case 0x16000015:
        m_SkinOverride = 0;
        FinishScript(0, 0);
        return 0;
    case 0x16000038: {
        int f = (int)pEvent->m_Param[0].f;
        if (f >= 0 && f < 290) FlagChannels[f] = 1;
        break;
    }
    case 0x16000039:
        XItemHandler_Player::GetInstance()->SetPlayerHasControl(0);
        m_bNoPlayerControl = 1;
        return 0;
    case 0x1600003a:
        XItemHandler_Player::GetInstance()->Hide(1, pEvent->m_Param[1].b);
        return 0;
    case 0x1600003b:
        gActorPause = 1;
        return 0;
    case 0x1600003c:
        gActorHide = 1;
        return 0;
    case 0x1600003d:
        XItemHandler_Player::GetInstance()->SetPlayerHasControl(0);
        m_bNoPlayerControl = 1;
        XItemHandler_Player::GetInstance()->Hide(1, 0);
        gActorPause = 1;
        gActorHide = 1;
        return 0;
    case 0x16000045:
        if (pEvent->m_Param[1].i)
            VisionWipe::Instance(1)->Start(pEvent->m_Param[0].b);
        else
            XItemHandler_Player::GetInstance()->CommitVisionMode(pEvent->m_Param[0].b);
        return 0;
    case 0x16000048: {
        signed char a = pEvent->m_Param[1].i > 0;
        signed char b = pEvent->m_Param[3].i > 0;
        FECutscene::Instance(1)->SetSubtitle(pEvent->m_Param[0].u, pEvent->m_Param[2].u, a, b, (float)pEvent->m_Frames * 2.0f);
        return 0;
    }
    case 0x16000049:
        FECutscene::Instance(1)->SetLetterBox(1);
        return 0;
    case 0x16000055:
        m_ObjectCloaked.GetData()[pEvent->m_Param[1].u - 0x15000000] = pEvent->m_Param[0].b;
        for (unsigned int i = 0; i < pScript->NumThreads(); i++) {
            if (m_ThreadObject.GetData()[i] != pEvent->m_Param[1].u) continue;
            EXItemAnimator_Anim *pItem = (EXItemAnimator_Anim *)GetThreadItem(i);
            if (!pItem || !pItem->GetExtRenderInfo()) continue;
            const EXRuntimeClass *rc = pItem->GetRuntimeClass();
            do {
                if (rc == EXRUNTIME_CLASS(EXItemAnimator_Anim)) {
                    {
                        ex::EXFlag<u16> &fl = pItem->m_pSkin->RenderInfo().m_renderFlags;
                        fl.m_flags = pEvent->m_Param[0].b ? (fl.m_flags | 0x2000) : (fl.m_flags & ~0x2000);
                    }
                    EXGeoRefObject ref;
                    if (m_ThreadObject.GetData()[i] == 0x15000003) {
                        ref.m_pGeoFile = EXGeoFile::GetGeoFile(0x1000024);
                        ref.m_hashRef = 0xd0000cc;
                    } else {
                        ref.m_pGeoFile = EXGeoFile::GetGeoFile(0x10000e3);
                        ref.m_hashRef = 0xd00012d;
                    }
                    pItem->AddCloneSkin(ref);
                    goto next55;
                }
                rc = rc->m_pBaseClass;
            } while (rc);
            {
                ex::EXFlag<u16> &fl = pItem->RenderInfo().m_renderFlags;
                fl.m_flags = pEvent->m_Param[0].b ? (fl.m_flags | 0x2000) : (fl.m_flags & ~0x2000);
            }
        next55:;
        }
        break;
    case 0x1600005b:
        m_bFocusBlur = pEvent->m_Param[0].b;
        m_FocusBlurFactor = (int)pEvent->m_Param[1].f;
        m_FocusBlurDistance = pEvent->m_Param[2].f;
        return 0;
    case 0x1600005c:
        m_bMotionBlur = pEvent->m_Param[0].b;
        m_MotionBlur = pEvent->m_Param[1].f;
        m_bNoMotionBlur = 1;
        return 0;
    case 0x1600005d:
        XItemHandler_Player::GetInstance()->SetCloak(pEvent->m_Param[0].b);
        return 0;
    case 0x1600005e:
        ((XGamePad *)EXBaseGamePad::GetGamePad())->PlayRumble(pEvent->m_Param[0].u, pEvent->m_Param[1].f, 0xff);
        return 0;
    case 0x1600005f: {
        EXVector pos = pAnim->GetOwnerItem()->Position();
        ((XGamePad *)EXBaseGamePad::GetGamePad())->PlayRumble3D(pEvent->m_Param[0].u, pos, pEvent->m_Param[2].f, pEvent->m_Param[3].f,
                                                               pEvent->m_Param[1].f, 0xff);
        return 0;
    }
    case 0x16000062:
        Text_PrintEnvironment(Text_Get(pEvent->m_Param[0].u), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80),
                              (int)((float)pEvent->m_Frames / EXBaseApp::FrameRate()), -1, 0, 0);
        return 0;
    case 0x16000066: {
        float a = pEvent->m_Param[0].f * 0.017453292f;
        float e = pEvent->m_Param[1].f * 0.017453292f;
        XGetCamera(0)->SetNext(a, e);
        return 0;
    }
    case 0x16000068: {
        int t = (int)((float)pEvent->m_Frames * 0.06666667f + 0.5f);
        ex::RGBA col;
        switch (pEvent->m_Param[1].i) {
        case 0: col = ex::RGBA(0x7f, 0x7f, 0x7f, 0x80); break;
        case 1: col = ex::RGBA(0x7f, 0, 0, 0x80); break;
        case 2: col = ex::RGBA(0, 0x7f, 0, 0x80); break;
        case 3: col = ex::RGBA(0, 0, 0x7f, 0x80); break;
        case 4: col = ex::RGBA(0, 0x7f, 0x7f, 0x80); break;
        case 5: col = ex::RGBA(0x7f, 0, 0x7f, 0x80); break;
        case 6: col = ex::RGBA(0x7f, 0x7f, 0, 0x80); break;
        case 7: col = ex::RGBA(0, 0, 0, 0x80); break;
        }
        switch (pEvent->m_Param[0].i) {
        case 0:
            theApp.ApplyFadeOut((float)t, col, 1);
            return 0;
        case 1:
            theApp.ApplyFadeIn((float)t, col, 1);
            return 0;
        case 2:
            theApp.ApplyFadeOutIn((float)t, col, 1, 0);
            return 0;
        }
        break;
    }
    case 0x16000069:
        for (unsigned int i = 0; i < pScript->NumThreads(); i++) {
            if (m_ThreadObject.GetData()[i] != pEvent->m_Param[0].u) continue;
            EXItemAnimator *pItem = GetThreadItem(i);
            if (!pItem) continue;
            EXExtRenderInfo *pExt = pItem->GetExtRenderInfo();
            EXTexSwapInfo *pSwap = pItem->SetTexSwapInfo();
            if (pExt && pSwap) pSwap->SetSwap(0x6000049, 0x100004c, 0x6000045);
        }
        break;
    case 0x1600006e:
        for (unsigned int i = 0; i < pScript->NumThreads(); i++) {
            if (m_ThreadObject.GetData()[i] != pEvent->m_Param[0].u) continue;
            EXItemAnimator *pItem = GetThreadItem(i);
            if (!pItem) continue;
            EXExtRenderInfo *pExt = pItem->GetExtRenderInfo();
            EXTexSwapInfo *pSwap = pItem->SetTexSwapInfo();
            if (pExt && pSwap) pSwap->SetSwap(0x6000046, 0x100004c, 0x6000045);
        }
        break;
    case 0x16000072: {
        pPlayer->SetHealth(pPlayer->GetMaxHealth());
        int sel = (short)XGetGameValues()->m_Unk58;
        if (sel == -1) sel = 0;
        m_SkinOverride = PRED_Skins[sel].m_Hash[6];
        return 0;
    }
    case 0x16000075:
        for (unsigned int i = 0; i < pScript->NumThreads(); i++) {
            if (m_ThreadObject.GetData()[i] != pEvent->m_Param[0].u) continue;
            EXItemAnimator *pItem = GetThreadItem(i);
            if (!pItem) continue;
            EXSwooshSys *pSys = gpGameWnd[0]->SwooshSysMgr()->CreateSystem(pEvent->m_Param[1].u);
            if (!pSys) continue;
            EXGeoFile *pFile = EXGeoFile::LoadGeoFile(0x1000013, 0, 0);
            if (pFile && pSys->Initialize(pFile, pEvent->m_Param[1].u, m_pItem, (EXItemAnimator_Entity *)pItem, 0, 0, 0))
                pSys->SetTessalationLevel(3);
        }
        break;
    case 0x16000076:
        for (unsigned int i = 0; i < pScript->NumThreads(); i++) {
            if (m_ThreadObject.GetData()[i] != pEvent->m_Param[0].u) continue;
            if (!GetThreadItem(i)) continue;
            EXSwooshSys *pSys = gpGameWnd[0]->SwooshSysMgr()->FindSwoosh(pEvent->m_Param[1].u, m_pItem);
            if (pSys) pSys->KillSwoosh();
        }
        break;
    case 0x16000079:
        for (unsigned int i = 0; i < pScript->NumThreads(); i++) {
            if (m_ThreadObject.GetData()[i] != pEvent->m_Param[0].u) continue;
            EXItemAnimator *pItem = GetThreadItem(i);
            if (pItem) XItemHandler_ScriptBase::FX_CreateCSMF(pEvent->m_Param[1].u, pItem, 0x10000037, 0x40, 0x1000016);
        }
        break;
    case 0x1600007b:
        m_bNoise = pEvent->m_Param[0].b;
        m_Noise = pEvent->m_Param[1].f;
        return 0;
    case 0x1600007c:
        Rain_Stop();
        return 0;
    case 0x1600007d:
        FECutscene::Instance(1)->CreditsStart(3.0f);
        return 0;
    case 0x1600007e:
        FECutscene::Instance(1)->CreditsStop();
        return 0;
    case 0x16000084:
        m_bGlow = !m_bGlow;
        if (m_bGlow) {
            m_Glow = 1.0f;
            m_bNoMotionBlur = 1;
            EXBaseRenderEnv::RenderOverrideFogInfo().SetFogDistance(32.0f, 120.0f);
            EXBaseRenderEnv::RenderOverrideFogInfo().SetFogMinMax(1.0f, 0.15f);
            EXBaseRenderEnv::RenderOverrideFogInfo().SetFogColor(ex::RGBA(0x20, 0x2d, 0x40, 0x7f));
            EXBaseRenderEnv::RenderOverrideFogInfo().SetFogEnabled(1);
            return 0;
        }
        break;
    case 0x16000086:
        FECutscene::Instance(1)->HUDScriptOn(pEvent->m_Param[0].u, pEvent->m_Param[1].f,
                                             EXVector(pEvent->m_Param[2].f, pEvent->m_Param[3].f, pEvent->m_Param[4].f, 0.0f));
        return 0;
    case 0x16000088:
        FECutscene::Instance(1)->HUDScriptOff(pEvent->m_Param[0].u);
        return 0;
    case 0x16000089:
        FECutscene::Instance(1)->OpenMimicry();
        return 0;
    case 0x1600008a:
        FECutscene::Instance(1)->CloseMimicry();
        return 0;
    case 0x1600008b: {
        XItemHandler_Player::GetInstance()->SetHealth(20.0f);
        int sel = (short)XGetGameValues()->m_Unk58;
        if (sel == -1) sel = 0;
        m_SkinOverride = PRED_Skins[sel].m_Hash[4];
        return 0;
    }
    }
    return 0;
}
