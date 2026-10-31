#include "EXTrigger.h"
#include "EXGeoMap.h"
#include "EXGeoFile.h"
#include "EXGeoHeader.h"
#include "EXItemAnimator.h"
#include "EXQuaternion.h"
#include "XItem.h"
#include "GeoSectionManager.h"
#include "ActorUtil.h"
#include "Recharge.h"
#include "XItemHandler_JumpTarget.h"
#include "XItemHandler_Graffiti.h"
#include "XItemHandler_Water.h"
#include "XItemHandler_CCTV.h"
#include "XItemHandler_ClayPigeon.h"
#include "XItemHandler_Mimicry.h"
#include "XItemHandler_Environment.h"
#include "XItemHandler_Bullet.h"
#include "XItemHandler_Player.h"
#include "XItemHandler_TestCloth.h"
#include "ActorOptions.h"
#include "XItemHandler_Objective.h"
#include "XItemHandler_Reinforcements.h"
#include "XItemHandler_Hint.h"
#include "XItemHandler_MissionTrigger.h"
#include "ScriptRunner.h"
#include "FEMainMenu.h"
#include "XItemHandler_CameraScript.h"
#include "XItemHandler_TestDrone.h"
#include "XLensFlare.h"
#include "XItemHandler_RadarBox.h"
#include "XItemHandler_VMO.h"
#include "XItemHandler_Turret.h"
#include "GameData.h"
#include "FELevels.h"
#include "FEHud.h"
#include "ActorLocation.h"
#include "EXMemory.h"
#include "FEText.h"

extern unsigned int gCurrentMap_GeoHash;

float gCamRotation;
signed char gStartCloaked;
signed char gTriggerCollision;
signed char gEnvironmentCollision;
signed char gKillCounts;
signed char gTriggerDebugText;
signed char gVisualTriggers;
unsigned int gTriggerFile_GeoHash;
signed char bPlayerWeaponsCreated;
EXVector gStartRotation;
EXVector gStartPosition;

EXRuntimeClass *EXTrigger::_GetBaseClass() { return EXRUNTIME_CLASS(EXTrigger); }
const EXRuntimeClass EXTrigger::classEXTrigger = { 0, "EXTrigger", sizeof(EXTrigger), (void *(__cdecl *)())EXTrigger::CreateObject };

EXTrigger::~EXTrigger() {}

XItem *Trigger_CreateItemFromTrigger(EXTrigger *pTrigger, EXRuntimeClass *pClass, signed char bAnim)
{
    unsigned int file = pTrigger->m_Unk50;
    if (!file) file = gCurrentMap_GeoHash;
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(file, 0, 0);
    if (pTrigger->m_Hash) {
        int section = GeoSectionManager::GetSubSection(file, pTrigger->m_Hash);
        if (section > 0) pFile->GeoHeader()->LoadSectionFile(section, 0, 0);
    }
    XItem *p = XItem::CreateItem(pFile, file, pClass, EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    p->SetPosition(pTrigger->m_Position);
    p->SetRotation(*(EXQuaternion *)&pTrigger->m_Rotation);
    p->SetScale(pTrigger->m_Scale);
    if (bAnim && pTrigger->m_Hash) p->AddAnimator(EXItemAnimator::CreateFrom(pFile, pTrigger->m_Hash));
    return p;
}

// NEAR: 8 bytes; retail stores the map-on word before pushing AddItem's item argument.
void Trigger_CreateWater(EXTrigger *pTrigger)
{
    if (!pTrigger->m_Unk50) pTrigger->m_Unk50 = gCurrentMap_GeoHash;
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(pTrigger->m_Unk50, 0, 0);
    XItem *p = XItem::CreateItem(pFile, pTrigger->m_Unk50, EXRUNTIME_CLASS(XItemHandler_Water), pTrigger->m_Position);
    XItemHandler_Water *pWater = (XItemHandler_Water *)p->ItemHandler();
    p->SetRotation(*(EXQuaternion *)&pTrigger->m_Rotation);
    pWater->WaterInit(pTrigger);
    p->SetMapOn(theItemEnv.GetMapOn(p->Position()));
    p->SetClipType(2);
    theItemEnv.AddItem(p, 1);
}

void Trigger_CreateRechargePoint(EXTrigger *pTrigger)
{
    XItem *p = Trigger_CreateItemFromTrigger(pTrigger, EXRUNTIME_CLASS(XItemHandler_RechargePoint), 1);
    XItemHandler_RechargePoint *pRecharge = (XItemHandler_RechargePoint *)p->ItemHandler();
    p->SetClipType(2);
    pRecharge->m_Unkb0 = 0;
    pRecharge->m_State = 0;
    pRecharge->m_IdleScript = pTrigger->m_Hash;
    pRecharge->m_StartScript = pTrigger->m_Data[0];
    pRecharge->m_EmptyScript = pTrigger->m_Data[1];
    pRecharge->m_EmptyFlagChannel = pTrigger->m_Data[2];
    theItemEnv.AddItem(p, 2);
}

void Trigger_CreateGraffiti(EXTrigger *pTrigger)
{
    XItem *p = Trigger_CreateItemFromTrigger(pTrigger, EXRUNTIME_CLASS(XItemHandler_Graffiti), 1);
    p->SetClipType(2);
    theItemEnv.AddItem(p, 0);
}

// NEAR: 18 bytes; the link-copy loop: VC addresses the source relative to the destination pointer.
void Trigger_CreateCCTV(EXTrigger *pTrigger)
{
    XItem *p = Trigger_CreateItemFromTrigger(pTrigger, EXRUNTIME_CLASS(XItemHandler_CCTV), 1);
    XItemHandler_CCTV *pCCTV = (XItemHandler_CCTV *)p->ItemHandler();
    p->SetClipType(2);
    pCCTV->m_Id = pTrigger->m_Id;
    XItem **pp = pCCTV->m_pLinks;
    int *pLink = pTrigger->m_Links;
    for (int i = 8; i; i--) {
        pp[-8] = (XItem *)*pLink++;
        *pp++ = 0;
    }
    pCCTV->m_Options = pTrigger->m_Unk4c;
    pCCTV->m_SweepAngle = (int)pTrigger->m_Data[0] * 0.017453292f;
    pCCTV->m_BaseYaw = p->Rotation().y;
    pCCTV->m_State = 0;
    pCCTV->m_HAngle = (int)pTrigger->m_Data[1] * 0.017453292f;
    pCCTV->m_Range = (float)(int)pTrigger->m_Data[2];
    pCCTV->m_Unk6c = pTrigger->m_Data[3];
    pCCTV->m_VAngle = (int)pTrigger->m_Data[4] * 0.017453292f;
    pCCTV->m_SweepSpeed = (int)pTrigger->m_Data[5] * 0.001f;
    pCCTV->m_Script = pTrigger->m_Data[6];
    pCCTV->m_Unk74 = (int)pTrigger->m_Data[7] * 0.017453292f;
    pCCTV->m_Unk78 = (int)pTrigger->m_Data[8] * 0.017453292f;
    pCCTV->m_SoundTag.UpdatePosition(pTrigger->m_Position);
    if (pCCTV->m_VAngle == 0.0f) pCCTV->m_VAngle = pCCTV->m_HAngle;
    if (pCCTV->m_SweepSpeed == 0.0f) pCCTV->m_SweepSpeed = 0.01f;
    if (pCCTV->m_Script <= 0x4000000) pCCTV->m_Script = 0;
    if (pCCTV->m_Unk78 == 0.0f) pCCTV->m_Unk78 = 3.1415927f;
    theItemEnv.AddItem(p, 2);
}

void Trigger_CreateActorCannon(EXTrigger *pTrigger) {}

void Trigger_SetPlayerWeaponsCreated(signed char b) { bPlayerWeaponsCreated = b; }

signed char Trigger_PlayerWeaponsCreated() { return bPlayerWeaponsCreated; }

void Trigger_CreateJumpTarget(EXTrigger *pTrigger)
{
    XItem *p = XItem::CreateItem(0, 0, EXRUNTIME_CLASS(XItemHandler_JumpTarget), EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    p->SetPosition(pTrigger->m_Position);
    theItemEnv.AddItem(p, 0);
}

// NEAR: the map-on word store is scheduled after the zero stores (retail: right after the link clear).
// NEAR: 10 bytes; loop register assignment (retail: eax = objective slots, ecx = links).
void Trigger_CreateClayPigeon(EXTrigger *pTrigger)
{
    XItem *p = XItem::CreateItem(0, 0, EXRUNTIME_CLASS(XItemHandler_ClayPigeonGenerator), EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    p->SetPosition(pTrigger->m_Position);
    theItemEnv.AddItem(p, 0);
    XItemHandler_ClayPigeonGenerator *pGen = (XItemHandler_ClayPigeonGenerator *)p->ItemHandler();
    XItem **pp = pGen->m_pObjectives;
    int *pLink = pTrigger->m_Links;
    for (int i = 8; i; i--) {
        pp[-8] = (XItem *)*pLink++;
        *pp++ = 0;
    }
}

void Trigger_CreateVoiceOver(EXTrigger *pTrigger)
{
    XItem *p = XItem::CreateItem(0, 0, EXRUNTIME_CLASS(XItemHandler_VoiceOver), EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    p->SetPosition(pTrigger->m_Position);
    theItemEnv.AddItem(p, 0);
    XItemHandler_VoiceOver *pVO = (XItemHandler_VoiceOver *)p->ItemHandler();
    pVO->m_StartFlagChannel = pTrigger->m_Data[0];
    pVO->m_StopFlagChannel = pTrigger->m_Data[1];
    pVO->m_Sound = pTrigger->m_Data[2];
}

EXTrigger::EXTrigger()
{
    EXDListItem::Init();
    m_MapOn = EXMapOn(0);
    m_Id = 0;
    m_Type = 0;
    m_Flags = 0;
    m_Unk4c = 0;
    m_Unk50 = 0;
    m_Hash = 0;
    m_NavPoly = -1;
    m_Position.SetZero();
    m_Rotation.SetZero();
    memset(m_Data, 0, sizeof(m_Data));
    memset(m_Links, 0, sizeof(m_Links));
}

// NEAR: retail keeps pGeo in ebp and the map id in its own stack slot (push ecx frame); stores match.
void EXTrigger::Init(EXGeoTrigger *pGeo, EXGeoMap *pMap, EXMapOn mapOn, unsigned short index)
{
    EXDListItem::Init();
    m_MapOn = mapOn;
    unsigned int mapId = (gCurrentMap_GeoHash & 0xfff) << 16;
    m_Id = index + mapId;
    m_Type = pMap->TriggerTypes(pGeo->m_TypeIndex)->m_Hash;
    m_Flags = pGeo->m_Flags;
    m_Unk4c = pGeo->m_04;
    m_Position.Set(pGeo->m_Pos, 0.0f);
    m_Rotation.Set(pGeo->m_Rot, 0.0f);
    m_Scale.Set(pGeo->m_Scale, 0.0f);
    m_NavPoly = AU_NavPolyForPos(m_Position);
    u32 *pData = pGeo->m_Data;
    unsigned int i;
    for (i = 0; i < 16; i++) m_Data[i] = TrigFlag(1 << i) ? *pData++ : 0;
    for (i = 0; i < 8; i++) {
        m_Links[i] = TrigFlag(0x10000 << i) ? *pData++ : -1;
        if (m_Links[i] >= 0) m_Links[i] += mapId;
    }
    if (TrigFlag(0x1000000)) {
        m_Hash = *pData++;
        m_Unk50 = TrigFlag(0x2000000) ? *pData++ : 0;
    } else {
        m_Hash = 0;
        m_Unk50 = 0;
    }
    if (TrigFlag(0x8000000)) {
        EXGeoTriggerDatum *pCol = pMap->TriggerCollisions(*pData++);
        m_Collision.m_Type = pCol->m_Type;
        m_Collision.m_Unk04 = pCol->m_04;
        m_Collision.m_Unk06 = pCol->m_06;
        m_Collision.m_Unk07 = pCol->m_07;
        m_Collision.m_Unk08 = pCol->m_08;
        m_Collision.m_Pos.x = pCol->m_Pos.x;
        m_Collision.m_Pos.y = pCol->m_Pos.y;
        m_Collision.m_Pos.z = pCol->m_Pos.z;
        m_Collision.m_Unk20 = pCol->m_20;
        m_Collision.m_Pos += pGeo->m_Pos;
    } else {
        memset(&m_Collision, 0, sizeof(m_Collision));
        m_Collision.m_Unk06 = 0xff;
    }
    if (TrigFlag(0x10000000)) m_Colour = *(ex::RGBA *)pData;
    else m_Colour.Set(0x80, 0x80, 0x80, 0xff);
}

void Trigger_CreateBeacon(EXTrigger *pTrigger)
{
    XItem *p = Trigger_CreateItemFromTrigger(pTrigger, EXRUNTIME_CLASS(XItemHandler_Beacon), 1);
    XItemHandler_Beacon *pBeacon = (XItemHandler_Beacon *)p->ItemHandler();
    p->SetClipType(2);
    pBeacon->m_ActivateFlagChannel = pTrigger->m_Data[0];
    pBeacon->m_ActivateScript = pTrigger->m_Data[1];
    pBeacon->m_ShowFlagChannel = pTrigger->m_Data[2];
    p->RenderInfo().RenderFlagsRef().BitSet(0x2000);
    p->RenderInfo().RenderFlagsRef().BitSet(8);
    if (pBeacon->m_ShowFlagChannel) {
        p->ItemFlags().BitClr(1);
        theItemEnv.AddItem(p, 0);
    } else {
        theItemEnv.AddItem(p, 2);
    }
}

void Trigger_CreateVocalMimicry(EXTrigger *pTrigger)
{
    pTrigger->m_Unk50 = 0x1000016;
    pTrigger->m_Hash = 0x4000414;
    XItem *p = Trigger_CreateItemFromTrigger(pTrigger, EXRUNTIME_CLASS(XItemHandler_Mimicry), 1);
    p->SetPosition(pTrigger->m_Position);
    p->RenderInfo().RenderFlagsRef().BitSet(8);
    XItemHandler_Mimicry *pMimicry = (XItemHandler_Mimicry *)p->ItemHandler();
    pMimicry->m_ShowFlagChannel = pTrigger->m_Data[0];
    pMimicry->m_HideFlagChannel = pTrigger->m_Data[1];
    pMimicry->m_PlayFlagChannel = pTrigger->m_Data[2];
    pMimicry->m_Sound = pTrigger->m_Data[3];
    pMimicry->m_Unk08 = pTrigger->m_Unk4c;
    pMimicry->m_bActive = pMimicry->m_ShowFlagChannel == 0;
    theItemEnv.AddItem(p, 0);
}

EXTrigger *EXTrigger::CreateObject() { return new EXTrigger; }

EXTrigger::EXTrigger(EXGeoTrigger *pGeo, EXGeoMap *pMap, EXMapOn mapOn, unsigned short index)
{
    Init(pGeo, pMap, mapOn, index);
}

void Trigger_CreateBeamTrigger(EXTrigger *pTrigger)
{
    XItem *p = Trigger_CreateItemFromTrigger(pTrigger, EXRUNTIME_CLASS(XItemHandler_BeamTrigger), 1);
    XItemHandler_BeamTrigger *pBeam = (XItemHandler_BeamTrigger *)p->ItemHandler();
    p->SetClipType(2);
    pBeam->m_Options = pTrigger->m_Unk4c;
    pBeam->m_Data[0] = pTrigger->m_Data[0];
    pBeam->m_Data[1] = pTrigger->m_Data[1];
    pBeam->m_Data[2] = pTrigger->m_Data[2];
    pBeam->m_Data[3] = pTrigger->m_Data[3];
    theItemEnv.AddItem(p, 0);
}

void Trigger_CreateMinesweeper(EXTrigger *pTrigger)
{
    XItem *p = XItem::CreateItem(0, 0, EXRUNTIME_CLASS(XItemHandler_Minesweeper), EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    theItemEnv.AddItem(p, 0);
    ((XItemHandler_Minesweeper *)p->ItemHandler())->m_FlagChannel = pTrigger->m_Data[0];
}

void Trigger_PostLoad()
{
    EXDListItem *pHead = theItemEnv.m_pItemLists[0].m_pHead;
    for (EXItem *p = pHead ? (EXItem *)((char *)pHead - 4) : 0; p; p = (EXItem *)p->NextItem()) {
        if (p->ItemHandler() && p->ItemHandler()->IsKindOf(EXRUNTIME_CLASS(XItemHandler_Upgrades))) {
        }
    }
}

void Trigger_CreateTouchplate(EXTrigger *pTrigger)
{
    XItem *p = XItem::CreateItem(0, 0, EXRUNTIME_CLASS(XItemHandler_Touchplate), EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    p->SetPosition(pTrigger->m_Position);
    theItemEnv.AddItem(p, 0);
    XItemHandler_Touchplate *pPlate = (XItemHandler_Touchplate *)p->ItemHandler();
    pPlate->m_Datum = *(EXGeoBaseDatum *)&pTrigger->m_Collision;
    pPlate->m_SetChannel = pTrigger->m_Data[0];
    pPlate->m_RequireChannel = pTrigger->m_Data[1];
    pPlate->m_KillChannel = pTrigger->m_Data[2];
    pPlate->m_OnTime = pTrigger->m_Data[3];
    pPlate->m_OffTime = pTrigger->m_Data[4];
}

void Trigger_CreateUpgrades(EXTrigger *pTrigger)
{
    XItem *p = Trigger_CreateItemFromTrigger(pTrigger, EXRUNTIME_CLASS(XItemHandler_Upgrades), 1);
    p->SetClipType(2);
    theItemEnv.AddItem(p, 0);
    XItemHandler_Upgrades *pUp = (XItemHandler_Upgrades *)p->ItemHandler();
    pUp->m_Datum = *(EXGeoBaseDatum *)&pTrigger->m_Collision;
    for (int i = 0; i < 10; i++) pUp->m_Data[i] = pTrigger->m_Data[i];
    if (pUp->m_Data[8]) XItemHandler_Player::Instance()->LevelWeaponUpgradeLoad(pUp->m_Data[8]);
}

void Trigger_CreateTestCloth(unsigned int &file, unsigned int &hash, EXVector &pos, EXVector &rot)
{
    if (!file) file = gCurrentMap_GeoHash;
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(file, 0, 0);
    XItem *p = XItem::CreateItem(pFile, file, EXRUNTIME_CLASS(XItemHandler_TestCloth), pos);
    p->SetRotation(*(EXQuaternion *)&rot);
    ((XItemHandler_TestCloth *)p->ItemHandler())->ClothInit(hash);
    p->SetMapOn(theItemEnv.GetMapOn(p->Position()));
    p->SetClipType(2);
    theItemEnv.AddItem(p, 1);
}

void Trigger_CreateClothOrWater(EXTrigger *pTrigger)
{
    if (!pTrigger->m_Unk50) pTrigger->m_Unk50 = gCurrentMap_GeoHash;
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(pTrigger->m_Unk50, 0, 0);
    XItem *p = XItem::CreateItem(pFile, pTrigger->m_Unk50, EXRUNTIME_CLASS(XItemHandler_TestCloth), pTrigger->m_Position);
    p->SetRotation(*(EXQuaternion *)&pTrigger->m_Rotation);
    ((XItemHandler_TestCloth *)p->ItemHandler())->ClothOrWaterInit(pTrigger);
    p->SetMapOn(theItemEnv.GetMapOn(p->Position()));
    p->SetClipType(2);
    theItemEnv.AddItem(p, 1);
}

void Trigger_CreateTestRope(unsigned int &file, unsigned int &hash, EXVector &pos, EXVector &rot)
{
    if (!file) file = gCurrentMap_GeoHash;
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(file, 0, 0);
    XItem *p = XItem::CreateItem(pFile, file, EXRUNTIME_CLASS(XItemHandler_TestRope), pos);
    p->SetRotation(*(EXQuaternion *)&rot);
    ((XItemHandler_TestRope *)p->ItemHandler())->RopeInit(hash);
    p->SetMapOn(theItemEnv.GetMapOn(p->Position()));
    p->SetClipType(2);
    theItemEnv.AddItem(p, 2);
}

void Trigger_CreateActorOptions(EXTrigger *pTrigger)
{
    ActorOptions *p = new ActorOptions(
        EXVector(pTrigger->m_Collision.m_Pos.x, pTrigger->m_Collision.m_Pos.y, pTrigger->m_Collision.m_Pos.z, 0.0f),
        EXVector(pTrigger->m_Collision.m_Unk08.x, pTrigger->m_Collision.m_Unk08.y, pTrigger->m_Collision.m_Unk08.z, 0.0f),
                                       pTrigger->m_Data[0],
                                       pTrigger->m_Data[1], pTrigger->m_Data[2], pTrigger->m_Data[3], pTrigger->m_Data[4],
                                       pTrigger->m_Data[5], pTrigger->m_Data[6], pTrigger->m_Data[7], pTrigger->m_Data[8],
                                       pTrigger->m_Data[9], pTrigger->m_Data[10], pTrigger->m_Unk4c);
    ActorOptionsManager::Register(p);
}

void Trigger_CreateReinforcements(EXTrigger *pTrigger)
{
    XItem *p = XItem::CreateItem(0, 0, EXRUNTIME_CLASS(XItemHandler_Reinforcements), EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    p->SetPosition(pTrigger->m_Position);
    p->SetRotation(*(EXQuaternion *)&pTrigger->m_Rotation);
    XItemHandler_Reinforcements *pR = (XItemHandler_Reinforcements *)p->ItemHandler();
    pR->m_Profile = pTrigger->m_Data[0];
    pR->m_Count = pTrigger->m_Data[1];
    pR->m_RepeatDelay = pTrigger->m_Data[2];
    theItemEnv.AddItem(p, 0);
}

void Trigger_CreateMissionSuccess(EXTrigger *pTrigger)
{
    XItem *p = XItem::CreateItem(0, 0, EXRUNTIME_CLASS(XItemHandler_Objective), EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    XItemHandler_Objective *pObj = (XItemHandler_Objective *)p->ItemHandler();
    pObj->m_Id = pTrigger->m_Id;
    XItem **pp = pObj->m_pLinkItems;
    int *pLink = pTrigger->m_Links;
    for (int i = 8; i; i--) {
        pp[-8] = (XItem *)*pLink++;
        *pp++ = 0;
    }
    pObj->m_ObjFlags = 0;
    pObj->m_Pos = pTrigger->m_Position;
    pObj->m_Hash = 0xaceface;
    pObj->m_SuccessMask = 0;
    pObj->m_FailMask = 0;
    pObj->m_Profile = 0;
    pObj->m_Count = 0;
    pObj->m_TimeLimit = 0;
    if (pTrigger->m_Unk4c & 1) pObj->m_Hash = 0x1cef00d;
    p->ItemFlags().BitClr(1);
    theItemEnv.AddItem(p, 0);
}

void Trigger_CreateBonusMissionSuccess(EXTrigger *pTrigger)
{
    XItem *p = XItem::CreateItem(0, 0, EXRUNTIME_CLASS(XItemHandler_Objective), EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    XItemHandler_Objective *pObj = (XItemHandler_Objective *)p->ItemHandler();
    pObj->m_Id = pTrigger->m_Id;
    XItem **pp = pObj->m_pLinkItems;
    int *pLink = pTrigger->m_Links;
    for (int i = 8; i; i--) {
        pp[-8] = (XItem *)*pLink++;
        *pp++ = 0;
    }
    pObj->m_ObjFlags = 0;
    pObj->m_Pos = pTrigger->m_Position;
    pObj->m_Hash = 0xac1dcafe;
    pObj->m_SuccessMask = 0;
    pObj->m_FailMask = 0;
    pObj->m_Profile = 0;
    pObj->m_Count = 0;
    pObj->m_TimeLimit = 0;
    p->ItemFlags().BitClr(1);
    theItemEnv.AddItem(p, 0);
}

extern unsigned int FailMessages[16];
extern unsigned int BonusFailMessages[16];

void Trigger_PreLoad()
{
    gStartPosition = EXVector(-999.0f, 0.0f, 0.0f, 0.0f);
    gStartCloaked = 0;
    for (int i = 0; i < 16; i++) {
        FailMessages[i] = 0;
        BonusFailMessages[i] = 0;
    }
}

void Trigger_CreateHint(EXTrigger *pTrigger)
{
    int type = pTrigger->m_Data[3];
    int radius = (int)pTrigger->m_Data[4] > 0 ? pTrigger->m_Data[4] : 3;
    pTrigger->m_Unk50 = 0x1000016;
    pTrigger->m_Hash = 0x4000759;
    XItem *p;
    if (type == 1) p = Trigger_CreateItemFromTrigger(pTrigger, EXRUNTIME_CLASS(XItemHandler_Hint), 1);
    else p = XItem::CreateItem(0, 0, EXRUNTIME_CLASS(XItemHandler_Hint), EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    p->SetPosition(pTrigger->m_Position);
    XItemHandler_Hint *pHint = (XItemHandler_Hint *)p->ItemHandler();
    pHint->m_Volume = *(EXGeoBaseDatum *)&pTrigger->m_Collision;
    pHint->m_Unk08 = pTrigger->m_Unk4c;
    pHint->m_Text = pTrigger->m_Data[0];
    pHint->m_Unk44 = pTrigger->m_Data[1];
    pHint->m_ActivateFlagChannel = pTrigger->m_Data[2];
    pHint->m_Type = type;
    pHint->m_Radius = radius;
    pHint->m_ShowFlagChannel = pTrigger->m_Data[5];
    pHint->m_SetFlagChannel = pTrigger->m_Data[6];
    pHint->m_ClearFlagChannel = pTrigger->m_Data[7];
    pHint->m_DeleteFlagChannel = pTrigger->m_Data[8];
    pHint->m_Unk60 = pTrigger->m_Data[9];
    pHint->m_Unk64 = pTrigger->m_Data[10];
    pHint->m_Unk68 = (int)pTrigger->m_Data[11] > 0;
    theItemEnv.AddItem(p, 0);
}

void Trigger_CreateAmmoPickup(EXTrigger *pTrigger)
{
    XItem *p = Trigger_CreateItemFromTrigger(pTrigger, EXRUNTIME_CLASS(XItemHandler_AmmoPickup), 1);
    XItemHandler_AmmoPickup *pAmmo = (XItemHandler_AmmoPickup *)p->ItemHandler();
    p->SetClipType(2);
    p->RenderInfo().SetExtRenderInfo(&pAmmo->m_ExtRenderInfo);
    p->RenderInfo().SetTexSwapInfo(&pAmmo->m_TexSwap);
    pAmmo->m_Options = pTrigger->m_Unk4c;
    pAmmo->m_Datum = *(EXGeoBaseDatum *)&pTrigger->m_Collision;
    pAmmo->m_Data[0] = pTrigger->m_Data[0];
    pAmmo->m_Data[1] = pTrigger->m_Data[1];
    pAmmo->m_Data[2] = pTrigger->m_Data[2];
    p->SetPosition(pTrigger->m_Position);
    p->SetRotation(*(EXQuaternion *)&pTrigger->m_Rotation);
    theItemEnv.AddItem(p, 0);
    if (!(pAmmo->m_Options & 1)) {
        p->RenderInfo().EnableRefractionRender(1);
        p->RenderInfo().RenderFlagsRef().BitSet(8);
    }
    if (pAmmo->m_Data[2]) p->ItemFlags().BitClr(1);
}

void Trigger_CreateEnvironment(EXTrigger *pTrigger)
{
    XItem *p;
    if (!pTrigger->m_Hash) {
        p = XItem::CreateItem(0, 0, EXRUNTIME_CLASS(XItemHandler_Environment), EXVector(0.0f, 0.0f, 0.0f, 0.0f));
        p->SetPosition(pTrigger->m_Position);
    } else {
        p = Trigger_CreateItemFromTrigger(pTrigger, EXRUNTIME_CLASS(XItemHandler_Environment), 1);
    }
    XItemHandler_Environment *pEnv = (XItemHandler_Environment *)p->ItemHandler();
    pEnv->m_Datum = *(EXGeoBaseDatum *)&pTrigger->m_Collision;
    pEnv->m_Options = pTrigger->m_Unk4c;
    pEnv->m_Link = pTrigger->m_Links[0];
    pEnv->m_Type = pTrigger->m_Data[0];
    pEnv->m_Data1 = pTrigger->m_Data[1];
    pEnv->m_Data2 = pTrigger->m_Data[2];
    pEnv->m_Data3 = pTrigger->m_Data[3];
    pEnv->m_Unk58 = pTrigger->m_Data[4];
    pEnv->m_Unk5c = (float)(int)pTrigger->m_Data[5];
    pEnv->m_Unk60 = pTrigger->m_Data[6];
    pEnv->m_Ability = pTrigger->m_Data[7];
    pEnv->m_Unk68 = pTrigger->m_Data[8];
    pEnv->m_Unk6c = pTrigger->m_Data[9];
    pEnv->m_FlagChannel = pTrigger->m_Data[10];
    pEnv->m_Data11[0] = pTrigger->m_Data[11];
    pEnv->m_Data11[1] = pTrigger->m_Data[12];
    pEnv->m_Data11[2] = pTrigger->m_Data[13];
    pEnv->m_Data11[3] = pTrigger->m_Data[14];
    pEnv->m_Unk44 = pEnv->m_Unk5c;
    if (pEnv->m_Unk68 == 1) pEnv->AbilityEnable(pEnv->m_Ability, 0);
    theItemEnv.AddItem(p, (pTrigger->m_Unk4c & 1) ? 2 : 0);
    if (pEnv->m_Unk58) p->ItemFlags().BitClr(1);
}

void Trigger_CreatePlayerAbility(EXTrigger *pTrigger)
{
    int mode = pTrigger->m_Data[1];
    unsigned int text = 0x5100005a;
    switch (pTrigger->m_Data[0]) {
    case 1: text = (mode != 1 ? 0xc : 0) + 0x5100009f; break;
    case 2: text = (mode != 1 ? 0xc : 0) + 0x510000a0; break;
    case 3: text = (mode != 1 ? 0xc : 0) + 0x510000a1; break;
    case 4: text = (mode != 1 ? 0xc : 0) + 0x510000a2; break;
    case 5: text = (mode != 1 ? 0xc : 0) + 0x510000a3; break;
    case 6: text = (mode != 1 ? 0xc : 0) + 0x510000a4; break;
    case 7: text = (mode != 1 ? 0xc : 0) + 0x510000a5; break;
    case 8: text = (mode != 1 ? 0xc : 0) + 0x510000a6; break;
    case 9: text = (mode != 1 ? 0xc : 0) + 0x510000a7; break;
    case 10: text = (mode != 1 ? 0xc : 0) + 0x510000a8; break;
    case 12: text = (mode != 1 ? 0xc : 0) + 0x510000aa; break;
    }
    pTrigger->m_Data[1] = text;
    pTrigger->m_Data[7] = pTrigger->m_Data[0];
    pTrigger->m_Data[0] = 7;
    pTrigger->m_Data[2] = 0;
    pTrigger->m_Data[3] = 0;
    pTrigger->m_Data[4] = 0;
    pTrigger->m_Data[5] = 0;
    pTrigger->m_Data[6] = 0;
    pTrigger->m_Data[8] = mode;
    Trigger_CreateEnvironment(pTrigger);
}

void Trigger_CreateExit(EXTrigger *pTrigger)
{
    XItem *p;
    if (pTrigger->m_Unk4c & 1) {
        p = XItem::CreateItem(0, 0, EXRUNTIME_CLASS(XItemHandler_MissionTrigger), EXVector(0.0f, 0.0f, 0.0f, 0.0f));
        p->SetPosition(pTrigger->m_Position);
        p->SetRotation(*(EXQuaternion *)&pTrigger->m_Rotation);
    } else {
        if (!pTrigger->m_Hash) {
            pTrigger->m_Unk50 = 0x1000016;
            pTrigger->m_Hash = 0x40006bc;
        }
        p = Trigger_CreateItemFromTrigger(pTrigger, EXRUNTIME_CLASS(XItemHandler_MissionTrigger), 1);
    }
    XItemHandler_MissionTrigger *pMT = (XItemHandler_MissionTrigger *)p->ItemHandler();
    pMT->m_Volume = *(EXGeoBaseDatum *)&pTrigger->m_Collision;
    pMT->m_Type = 2;
    pMT->m_Options = pTrigger->m_Unk4c;
    pMT->m_bInside = 0;
    pMT->m_Radius = pTrigger->m_Data[0];
    pMT->m_Text = pTrigger->m_Data[1];
    if (!pMT->m_Radius) pMT->m_Radius = 3;
    if (pMT->m_Text <= 0x51000000 || pMT->m_Text >= 0x510009b9) pMT->m_Text = 0;
    if (pTrigger->m_Unk4c & 1) p->ItemFlags().BitClr(1);
    if (pTrigger->m_Unk4c & 8) pMT->m_Radius = 0;
    p->RenderInfo().RenderFlagsRef().BitSet(8);
    p->SetClipType(2);
    theItemEnv.AddItem(p, 0);
}

void Trigger_CreateMissionTrigger(EXTrigger *pTrigger)
{
    if (pTrigger->m_Unk4c & 2) pTrigger->m_Unk4c |= 1;
    XItem *p;
    if (pTrigger->m_Unk4c & 1) {
        p = XItem::CreateItem(0, 0, EXRUNTIME_CLASS(XItemHandler_MissionTrigger), EXVector(0.0f, 0.0f, 0.0f, 0.0f));
        p->SetPosition(pTrigger->m_Position);
    } else {
        if (!pTrigger->m_Hash) {
            pTrigger->m_Unk50 = 0x1000016;
            pTrigger->m_Hash = 0x4000050;
        }
        p = Trigger_CreateItemFromTrigger(pTrigger, EXRUNTIME_CLASS(XItemHandler_MissionTrigger), 1);
    }
    XItemHandler_MissionTrigger *pMT = (XItemHandler_MissionTrigger *)p->ItemHandler();
    pMT->m_Id = pTrigger->m_Id;
    XItem **pp = pMT->m_pLinks;
    int *pLink = pTrigger->m_Links;
    for (int i = 8; i; i--) {
        pp[-8] = (XItem *)*pLink++;
        *pp++ = 0;
    }
    *(EXGeoTriggerVolume *)&pMT->m_Volume = *(const EXGeoTriggerVolume *)&pTrigger->m_Collision;
    pMT->m_Type = 0;
    pMT->m_Options = pTrigger->m_Unk4c;
    pMT->m_bInside = 0;
    p->SetHasRender();
    pMT->m_MissionHash = pTrigger->m_Data[0];
    pMT->m_Unk88 = pTrigger->m_Data[1];
    pMT->m_Radius = pTrigger->m_Data[2];
    pMT->m_PlayerSeenInterval = pTrigger->m_Data[3];
    pMT->m_Text2 = pTrigger->m_Data[4];
    pMT->m_Text = pTrigger->m_Data[5];
    pMT->m_TriggerFile = pTrigger->m_Data[6];
    pMT->m_EscapeRadius = pTrigger->m_Data[7];
    pMT->m_AlarmResetDelay.u = pTrigger->m_Data[8];
    pMT->m_AlarmResetSpeed.u = pTrigger->m_Data[9];
    if (!pMT->m_Radius) pMT->m_Radius = 3;
    if (pMT->m_Text2 <= 0x51000000 || pMT->m_Text2 >= 0x510009b9) pMT->m_Text2 = 0;
    if (pMT->m_Text <= 0x51000000 || pMT->m_Text >= 0x510009b9) pMT->m_Text = 0;
    if (pMT->m_TriggerFile == 0x1000000) pMT->m_TriggerFile = 0;
    if (!pMT->m_EscapeRadius) pMT->m_EscapeRadius = 50;
    if (pTrigger->m_Unk4c & 1) p->ItemFlags().BitClr(1);
    if (pTrigger->m_Unk4c & 8) pMT->m_Radius = 0;
    p->RenderInfo().RenderFlagsRef().BitSet(8);
    p->SetClipType(2);
    theItemEnv.AddItem(p, 0);
}

void Trigger_CreateDestination(EXTrigger *pTrigger)
{
    XItem *p;
    if (pTrigger->m_Unk4c & 1) {
        p = XItem::CreateItem(0, 0, EXRUNTIME_CLASS(XItemHandler_MissionTrigger), EXVector(0.0f, 0.0f, 0.0f, 0.0f));
        p->SetPosition(pTrigger->m_Position);
        p->SetRotation(*(EXQuaternion *)&pTrigger->m_Rotation);
    } else {
        if (!pTrigger->m_Hash) {
            pTrigger->m_Unk50 = 0x1000016;
            pTrigger->m_Hash = 0x4000050;
        }
        p = Trigger_CreateItemFromTrigger(pTrigger, EXRUNTIME_CLASS(XItemHandler_MissionTrigger), 1);
    }
    XItemHandler_MissionTrigger *pMT = (XItemHandler_MissionTrigger *)p->ItemHandler();
    pMT->m_Id = pTrigger->m_Id;
    XItem **pp = pMT->m_pLinks;
    int *pLink = pTrigger->m_Links;
    for (int i = 8; i; i--) {
        pp[-8] = (XItem *)*pLink++;
        *pp++ = 0;
    }
    *(EXGeoTriggerVolume *)&pMT->m_Volume = *(const EXGeoTriggerVolume *)&pTrigger->m_Collision;
    pMT->m_Type = 1;
    pMT->m_Options = pTrigger->m_Unk4c;
    pMT->m_bInside = 0;
    p->ItemFlags().BitClr(1);
    pMT->m_Radius = pTrigger->m_Data[0];
    pMT->m_Text = pTrigger->m_Data[1];
    if (!pMT->m_Radius) pMT->m_Radius = 3;
    if (pMT->m_Text <= 0x51000000 || pMT->m_Text >= 0x510009b9) pMT->m_Text = 0;
    if (pTrigger->m_Unk4c & 1) p->ItemFlags().BitClr(1);
    if (pTrigger->m_Unk4c & 8) pMT->m_Radius = 0;
    p->RenderInfo().RenderFlagsRef().BitSet(8);
    p->SetClipType(2);
    theItemEnv.AddItem(p, 0);
}

void Trigger_CreateScriptPlacement(EXTrigger *pTrigger)
{
    if (!pTrigger->m_Hash) return;
    XItem *p = Trigger_CreateItemFromTrigger(pTrigger, EXRUNTIME_CLASS(XItemHandler_ScriptRunner), 0);
    EXVector pos = p->Position();
    pos.y += 0.5f;
    EXMapOn m = theItemEnv.GetMapOn(pos);
    p->SetMapOn(m);
    XItemHandler_ScriptRunner *pSR = (XItemHandler_ScriptRunner *)p->ItemHandler();
    pSR->m_GeoHash = pTrigger->m_Unk50;
    if (!pSR->m_GeoHash) pSR->m_GeoHash = gCurrentMap_GeoHash;
    pSR->m_ScriptHash = pTrigger->m_Hash;
    pSR->m_Flags = pTrigger->m_Data[0];
    pSR->m_TriggerFlags = pTrigger->m_Data[1];
    pSR->m_Range.u = pTrigger->m_Data[2];
    pSR->m_DeleteChannel = pTrigger->m_Data[3];
    pSR->m_DamageType = pTrigger->m_Data[4];
    pSR->m_ImpactType = pTrigger->m_Data[5];
    if (!pSR->m_DamageType) pSR->m_DamageType = 0x53000000;
    pSR->m_MissionId = pTrigger->m_Links[0];
    pSR->m_pMissionItem = 0;
    pSR->m_Flags2 = pTrigger->m_Unk4c;
    theItemEnv.AddItem(p, 0);
    if (pSR->m_TriggerFlags & 0x18) p->ItemFlags().BitClr(1);
}

void Trigger_CreateFETrophy(EXTrigger *pTrigger)
{
    XItem *p = XItem::CreateItem(0, 0, EXRUNTIME_CLASS(XItemHandler_FETrophy), EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    theItemEnv.AddItem(p, 0);
    p->SetPosition(pTrigger->m_Position);
    p->SetRotation(*(EXQuaternion *)&pTrigger->m_Rotation);
    p->SetScale(pTrigger->m_Scale);
    XItemHandler_FETrophy *pT = (XItemHandler_FETrophy *)p->ItemHandler();
    pT->m_Unk8 = pTrigger->m_Data[0];
    pT->m_UnkC = pTrigger->m_Data[1];
    pT->m_Unk18 = pTrigger->m_Data[2];
    pT->m_Scale = pTrigger->DataF(3);
    pT->Init();
}

void Trigger_CreateScriptCutScene(EXTrigger *pTrigger)
{
    if (!pTrigger->m_Hash) return;
    gCutscenePlayer.Add(pTrigger->m_Unk50, pTrigger->m_Hash);
    XItem *p = XItem::CreateItem(0, 0, EXRUNTIME_CLASS(XItemHandler_CutsceneTrigger), EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    XItemHandler_CutsceneTrigger *pCS = (XItemHandler_CutsceneTrigger *)p->ItemHandler();
    pCS->m_Options = pTrigger->m_Unk4c;
    pCS->m_Data[0] = pTrigger->m_Data[0];
    pCS->m_Data[1] = pTrigger->m_Data[1];
    pCS->m_Data[2] = pTrigger->m_Data[2];
    pCS->m_Data[3] = pTrigger->m_Data[3];
    pCS->m_File = pTrigger->m_Unk50;
    pCS->m_Hash = pTrigger->m_Hash;
    theItemEnv.AddItem(p, 0);
    if (pCS->m_Data[2] <= 0x1000000 || pCS->m_Data[2] >= 0x100019c) pCS->m_Data[2] = 0;
}

void Trigger_CreateTestDrone(EXVector &pos, EXVector &rot)
{
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(0x1000006, 0, 0);
    XItem *p = XItem::CreateItem(pFile, 0x1000006, EXRUNTIME_CLASS(XItemHandler_TestDrone), EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    p->SetPosition(pos);
    p->SetRotation(*(EXQuaternion *)&rot);
    p->SetMapOn(theItemEnv.GetMapOn(p->Position()));
    p->SetClipType(2);
    theItemEnv.AddItem(p, 1);
}

void Trigger_CreateRadarBox(EXTrigger *pTrigger)
{
    XItem *p = XItem::CreateItem(0, 0, EXRUNTIME_CLASS(XItemHandler_RadarBox), EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    p->SetPosition(pTrigger->m_Position);
    XItemHandler_RadarBox *pRB = (XItemHandler_RadarBox *)p->ItemHandler();
    pRB->m_Volume = *(EXGeoBaseDatum *)&pTrigger->m_Collision;
    pRB->m_Data0 = pTrigger->m_Data[0];
    theItemEnv.AddItem(p, 0);
}

void Trigger_CreateObjective(EXTrigger *pTrigger)
{
    XItem *p = XItem::CreateItem(0, 0, EXRUNTIME_CLASS(XItemHandler_Objective), EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    XItemHandler_Objective *pObj = (XItemHandler_Objective *)p->ItemHandler();
    pObj->m_Id = pTrigger->m_Id;
    XItem **pp = pObj->m_pLinkItems;
    int *pLink = pTrigger->m_Links;
    for (int i = 8; i; i--) {
        pp[-8] = (XItem *)*pLink++;
        *pp++ = 0;
    }
    pObj->m_ObjFlags = (u16)pTrigger->m_Unk4c;
    pObj->m_Pos = pTrigger->m_Position;
    pObj->m_Hash = pTrigger->m_Data[0];
    pObj->m_SuccessMask = (u16)pTrigger->m_Data[1];
    pObj->m_FailMask = (u16)pTrigger->m_Data[2];
    pObj->m_Profile = pTrigger->m_Data[3];
    pObj->m_Count = pTrigger->m_Data[4];
    pObj->m_TimeLimit = pTrigger->m_Data[5];
    int spawn = pTrigger->m_Data[6];
    int spawn2 = pTrigger->m_Data[7];
    pObj->m_SpawnCount = pTrigger->m_Data[8];
    pObj->m_FlagChannel = pTrigger->m_Data[9];
    pObj->m_AlarmFame = pTrigger->m_Data[10];
    pObj->m_StatsLevel = pTrigger->m_Data[11];
    pObj->m_TargetFlagChannel = pTrigger->m_Data[12];
    pObj->m_Text = pTrigger->m_Data[13];
    int show = pTrigger->m_Data[14];
    *(int *)&pObj->m_Unk84 = show >= 2 ? 2 : (show <= 0 ? 0 : show);
    pObj->m_CompleteText = pTrigger->m_Data[15];
    if (pObj->m_Profile == 0x47000000) pObj->m_Profile = 0;
    if (spawn2 == 0x42000000) spawn2 = 0;
    if (spawn == 0x43000000 || !spawn) pObj->m_SpawnPoint = spawn2;
    else pObj->m_SpawnPoint = spawn;
    if (pObj->m_StatsLevel <= 0x1000000 || pObj->m_StatsLevel > 0x2000000) pObj->m_StatsLevel = 0;
    p->ItemFlags().BitClr(1);
    theItemEnv.AddItem(p, 0);
}

void Trigger_CreateVMO(EXTrigger *pTrigger)
{
    XItem *p = Trigger_CreateItemFromTrigger(pTrigger, EXRUNTIME_CLASS(XItemHandler_VMO), 1);
    XItemHandler_VMO *pVMO = (XItemHandler_VMO *)p->ItemHandler();
    EXRenderInfo &ri = p->RenderInfo();
    p->SetClipType(2);
    ri.SetExtRenderInfo(&pVMO->m_ExtRenderInfo);
    ri.SetTexSwapInfo(&pVMO->m_TexSwapInfo);
    pVMO->m_Colour = EXVector(pTrigger->m_Colour.m_R * 0.0078125f, pTrigger->m_Colour.m_G * 0.0078125f,
                              pTrigger->m_Colour.m_B * 0.0078125f, 1.0f);
    pVMO->m_VMOFlags = pTrigger->m_Unk4c;
    pVMO->m_DeleteFlagChannel = pTrigger->m_Data[0];
    pVMO->m_ShowFlagChannel = pTrigger->m_Data[1];
    pVMO->m_ScanText = pTrigger->m_Data[2];
    pVMO->m_ScanText2 = pTrigger->m_Data[3];
    p->SetPosition(pTrigger->m_Position);
    p->SetRotation(*(EXQuaternion *)&pTrigger->m_Rotation);
    theItemEnv.AddItem(p, 2);
    if (pVMO->m_VMOFlags & 0x18) p->ItemFlags().BitClr(1);
    if (!(fabs(pVMO->m_Colour.x - 1.0f) < 0.001f && fabs(pVMO->m_Colour.y - 1.0f) < 0.001f &&
          fabs(pVMO->m_Colour.z - 1.0f) < 0.001f && fabs(pVMO->m_Colour.w - 1.0f) < 0.001f)) {
        ri.RenderFlagsRef().BitSet(4);
        ri.SetRGBAMult(pVMO->m_Colour);
    }
    if (pVMO->m_ScanText <= 0x4000000) pVMO->m_ScanText = 0;
    if (pVMO->m_ScanText2 <= 0x51000000) pVMO->m_ScanText2 = 0;
}

void Trigger_CreatePlayerWeapons(EXTrigger *pTrigger)
{
    unsigned int weapons[32] = { 0 };
    unsigned int n = 0;
    int cur0 = 0, cur1 = 0, cur2 = 0, cur3 = 0;
    unsigned int f = pTrigger->m_Data[0];
    if (f & 1) weapons[n++] = 2;
    if (f & 8) weapons[n++] = 4;
    if (f & 4) weapons[n++] = 3;
    if (f & 0x100) cur0 = 2;
    if (f & 0x400) cur0 = 3;
    if (f & 0x800) cur0 = 4;
    f = pTrigger->m_Data[1];
    if (f & 1) weapons[n++] = 5;
    if (f & 2) weapons[n++] = 6;
    if (f & 8) weapons[n++] = 7;
    if (f & 0x20) weapons[n++] = 10;
    if (f & 0x40) weapons[n++] = 11;
    if (f & 0x100) cur1 = 5;
    if (f & 0x200) cur1 = 6;
    if (f & 0x800) cur1 = 7;
    if (f & 0x2000) cur1 = 10;
    if (f & 0x4000) cur1 = 11;
    f = pTrigger->m_Data[2];
    if (f & 1) weapons[n++] = 12;
    if (f & 4) weapons[n++] = 0x3f;
    if (f & 0x10) weapons[n++] = 8;
    if (f & 0x20) weapons[n++] = 9;
    if (f & 0x100) cur2 = 12;
    if (f & 0x400) cur2 = 0x3f;
    if (f & 0x1000) cur2 = 8;
    if (f & 0x2000) cur2 = 9;
    f = pTrigger->m_Data[3];
    weapons[n++] = 0;
    if (f & 2) weapons[n++] = 1;
    if (f & 0x100) cur3 = 0;
    if (f & 0x200) cur3 = 1;
    int vision = pTrigger->m_Data[4];
    if (vision > 0) XItemHandler_Player::Instance()->m_VisionModeFlags = vision;
    else if (!(vision & 0x10)) XItemHandler_Player::Instance()->m_VisionModeFlags = 0xffffffff;
    FEHud::Instance(1)->WeaponArrayCreate(weapons, n);
    int idx = FELevelManager::Instance(1)->IndexFromLevel(gCurrentMap_GeoHash);
    if (idx > 0 && idx < FELevelManager::Instance(1)->StoryLevelCount())
        GameData::Instance(1)->Values()->m_LevelRecords[idx - 1].WeaponArrayRestore();
    if (!pTrigger->m_Data[1]) FEHud::Instance(1)->WeaponArrayAdd(0xd, (FEWeaponCat::Flags)0);
    else XItemHandler_Player::Instance()->SetCurrentWeapon(0, cur1);
    if (!pTrigger->m_Data[2]) FEHud::Instance(1)->WeaponArrayAdd(0xd, (FEWeaponCat::Flags)2);
    else XItemHandler_Player::Instance()->SetCurrentWeapon(2, cur2);
    if (!pTrigger->m_Data[0]) FEHud::Instance(1)->WeaponArrayAdd(0xd, (FEWeaponCat::Flags)1);
    else XItemHandler_Player::Instance()->SetCurrentWeapon(1, cur0);
    if (!pTrigger->m_Data[3]) FEHud::Instance(1)->WeaponArrayAdd(0xd, (FEWeaponCat::Flags)3);
    else XItemHandler_Player::Instance()->SetCurrentWeapon(3, cur3);
    bPlayerWeaponsCreated = 1;
}

void Trigger_CreateTurret(EXTrigger *pTrigger)
{
    XItem *p = Trigger_CreateItemFromTrigger(pTrigger, EXRUNTIME_CLASS(XItemHandler_Turret), 1);
    XItemHandler_Turret *pT = (XItemHandler_Turret *)p->ItemHandler();
    p->SetClipType(2);
    pT->m_Id = pTrigger->m_Id;
    XItem **pp = pT->m_pLinks;
    int *pLink = pTrigger->m_Links;
    for (int i = 8; i; i--) {
        pp[-8] = (XItem *)*pLink++;
        *pp++ = 0;
    }
    pT->m_SweepAngle = (int)pTrigger->m_Data[0] * 0.017453292f;
    pT->m_BaseYaw = p->Rotation().y;
    pT->m_BasePitch = p->Rotation().x;
    pT->m_State = 1;
    pT->m_HAngle = (int)pTrigger->m_Data[1] * 0.017453292f;
    pT->m_Range = (float)(int)pTrigger->m_Data[2];
    pT->m_Unk13c = pTrigger->m_Data[3];
    pT->m_Unk140 = pTrigger->m_Data[4];
    pT->m_Weapon = pTrigger->m_Data[5];
    pT->m_VAngle = (int)pTrigger->m_Data[6] * 0.017453292f;
    pT->m_SweepSpeed = (int)pTrigger->m_Data[7] * 0.001f;
    pT->m_Script = pTrigger->m_Data[8];
    pT->m_Script2 = pTrigger->m_Data[9];
    pT->m_Unk150 = (int)pTrigger->m_Data[10] * 0.017453292f;
    pT->m_Unk154 = (int)pTrigger->m_Data[11] * 0.017453292f;
    pT->m_TargetSubGroup = pTrigger->m_Data[12];
    *(u32 *)&pT->m_Options = pTrigger->m_Unk4c;
    if (pT->m_Unk140) pT->m_State = 0;
    if (pT->m_Weapon <= 0x49000000) pT->m_Weapon = 0x49000035;
    if (pT->m_VAngle == 0.0f) pT->m_VAngle = pT->m_HAngle;
    if (pT->m_SweepSpeed == 0.0f) pT->m_SweepSpeed = 0.01f;
    if (pT->m_Script <= 0x4000000) pT->m_Script = 0;
    if (pT->m_Script2 <= 0x4000000) pT->m_Script2 = 0;
    if ((unsigned int)pT->m_TargetSubGroup <= 0x4f000000) pT->m_TargetSubGroup = 0;
    theItemEnv.AddItem(p, 2);
}
void TrafficAddTrigger(EXTrigger *pTrigger);    // TODO stand-in (Traffic.obj)
void AirTraffic_Trigger(EXTrigger *pTrigger);   // TODO stand-in (FX_AirTrafficHandler.obj)

unsigned int TriggerInit(EXTrigger *pTrigger, EXGeoTrigger *pGeo, EXGeoMap *pMap, EXMapOn mapOn, unsigned short index)
{
    unsigned int type = pMap->TriggerTypes(pGeo->m_TypeIndex)->m_Hash;
    pTrigger->Init(pGeo, pMap, mapOn, index);
    switch (type) {
    case 0x1e000001:
        gStartPosition = pTrigger->m_Position;
        gStartRotation = pTrigger->m_Rotation;
        gStartCloaked = pTrigger->m_Unk4c & 1;
        gCamRotation = (int)pTrigger->m_Data[0] * 0.017453292f;
        break;
    case 0x1e00001e:
        Trigger_CreateTestCloth(pTrigger->m_Unk50, pTrigger->m_Hash, pTrigger->m_Position, pTrigger->m_Rotation);
        break;
    case 0x1e00002b:
        Trigger_CreateClothOrWater(pTrigger);
        break;
    case 0x1e00001f:
        Trigger_CreateTestRope(pTrigger->m_Unk50, pTrigger->m_Hash, pTrigger->m_Position, pTrigger->m_Rotation);
        break;
    case 0x1e000026:
        AU_DelaySpawnActor(pTrigger);
        break;
    case 0x1e000025:
        POI_Add(pTrigger, gTriggerFile_GeoHash != 0);
        break;
    case 0x1e000017:
        Trigger_CreateRechargePoint(pTrigger);
        break;
    case 0x1e00001c:
        Trigger_CreateMissionTrigger(pTrigger);
        break;
    case 0x1e000020:
        Trigger_CreateDestination(pTrigger);
        break;
    case 0x1e000029:
        Trigger_CreateObjective(pTrigger);
        break;
    case 0x1e00002c:
        Trigger_CreateMissionSuccess(pTrigger);
        break;
    case 0x1e000039:
        Trigger_CreateBonusMissionSuccess(pTrigger);
        break;
    case 0x1e000021:
        Trigger_CreateScriptPlacement(pTrigger);
        break;
    case 0x1e000030:
        Trigger_CreateScriptCutScene(pTrigger);
        break;
    case 0x1e000027:
    case 0x1e00002e:
    case 0x1e00002f:
    case 0x1e00004d:
        TrafficAddTrigger(pTrigger);
        break;
    case 0x1e000031:
        Trigger_CreateGraffiti(pTrigger);
        break;
    case 0x1e000032:
        Trigger_CreateCCTV(pTrigger);
        break;
    case 0x1e000033:
        Trigger_CreateTurret(pTrigger);
        break;
    case 0x1e000034:
        Trigger_CreateEnvironment(pTrigger);
        break;
    case 0x1e000046:
        Trigger_CreateHint(pTrigger);
        break;
    case 0x1e000035:
        Trigger_CreatePlayerAbility(pTrigger);
        break;
    case 0x1e000036:
        Trigger_CreatePlayerWeapons(pTrigger);
        break;
    case 0x1e000038:
        Trigger_CreateReinforcements(pTrigger);
        break;
    case 0x1e00003a:
        Trigger_CreateWater(pTrigger);
        break;
    case 0x1e00003b:
        Trigger_CreateVMO(pTrigger);
        break;
    case 0x1e00003d:
        Trigger_CreateJumpTarget(pTrigger);
        break;
    case 0x1e00003e:
        Trigger_CreateClayPigeon(pTrigger);
        break;
    case 0x1e00003f:
        Trigger_CreateAmmoPickup(pTrigger);
        break;
    case 0x1e000040:
        Trigger_CreateRadarBox(pTrigger);
        break;
    case 0x1e000042:
        Trigger_CreateActorOptions(pTrigger);
        break;
    case 0x1e000041:
        AirTraffic_Trigger(pTrigger);
        break;
    case 0x1e000043:
        Trigger_CreateTouchplate(pTrigger);
        break;
    case 0x1e000044: {
        EXVector vel(0.0f, 0.0f, 0.0f, 0.0f);
        EXVector dir(0.0f, -1.0f, 0.0f, 0.0f);
        unsigned int hash;
        switch (pTrigger->m_Data[0]) {
        case 0: hash = 0x4900005c; break;
        case 1: hash = 0x4900005d; break;
        case 2: hash = 0x4900005e; break;
        case 3: hash = 0x4900005f; break;
        case 4: hash = 0x49000055; break;
        default: return 1;
        }
        XItem *p = XItemHandler_Bullet::CreateFromHash(pTrigger->m_Position, vel, dir, hash, 0, 0, 0, 1.0f);
        ((XItemHandler_Bullet *)p->ItemHandler())->m_HideChannel = (u16)pTrigger->m_Data[1];
        break;
    }
    case 0x1e000045:
        for (int i = 0; i < 16; i++) {
            FailMessages[i] = pTrigger->m_Data[i];
            if (FailMessages[i] <= 0x51000000) FailMessages[i] = 0;
        }
        break;
    case 0x1e000052:
        for (int j = 0; j < 16; j++) {
            BonusFailMessages[j] = pTrigger->m_Data[j];
            if (BonusFailMessages[j] <= 0x51000000) BonusFailMessages[j] = 0;
        }
        break;
    case 0x1e000047:
        Trigger_CreateUpgrades(pTrigger);
        break;
    case 0x1e000048:
        Trigger_CreateBeamTrigger(pTrigger);
        break;
    case 0x1e000049:
        Trigger_CreateBeacon(pTrigger);
        break;
    case 0x1e00004a:
        Trigger_CreateVocalMimicry(pTrigger);
        break;
    case 0x1e00004b:
        Trigger_CreateActorCannon(pTrigger);
        break;
    case 0x1e00004c:
        Trigger_CreateMinesweeper(pTrigger);
        break;
    case 0x1e00004e:
        FEMainMenu::AddFEMarker(pTrigger);
        break;
    case 0x1e00004f:
        Trigger_CreateExit(pTrigger);
        break;
    case 0x1e000050: {
        float a = (int)pTrigger->m_Data[0] * 0.017453292f;
        float b = (int)pTrigger->m_Data[1] * 0.017453292f;
        Sun.x = (float)(sin(a) * sin(b));
        Sun.z = (float)(cos(a) * sin(b));
        Sun.y = (float)cos(b);
        Sun *= 256.0f;
        SunTrigger = 1;
        break;
    }
    case 0x1e000051:
        Trigger_CreateFETrophy(pTrigger);
        break;
    case 0x1e000053:
        Trigger_CreateVoiceOver(pTrigger);
        break;
    }
    return 1;
}
