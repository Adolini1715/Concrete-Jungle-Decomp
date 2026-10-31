#include "FEText.h"
#include "EXMemory.h"
#include "XAppStandIn.h"
#include "FEHud.h"
#include "FEObject.h"
#include "FERadar.h"
#include "Actor.h"
#include "EXBaseDisplay.h"
#include "FE.h"
#include "EXQMath.h"
#include "EXMatrix.h"
#include "EXCamera.h"
#include "EXQuaternion.h"
#include "ActorProfile.h"
#include "FEObjectScript.h"
#include "FELayout.h"
#include "EXItemAnimator_Entity.h"
#include "EXGeoFile.h"
#include "EXBaseRenderEnv.h"
#include "GameData.h"
#include "XGamePad.h"
#include "XItem.h"
#include "XItemHandler_Camera.h"
#include "FETopmost.h"
#include "XGameWnd.h"
#include "EXRayCastTemp.h"
#include "EXString.h"
#include <stdio.h>
#include <math.h>

void Text_Update();

extern float Mission_AlarmLevel;
extern RadarMap CurrentMap;
extern signed char Mission_AlarmMeterHidden;

FEHud *FEHud::m_pInstance;

inline ex::RGBA FEHud::TintColour(int tint)
{
    static ex::RGBA Thermal(0x40, 0x40, 0x7f, 0x80);
    static ex::RGBA Neuro(0x7f, 0x40, 0x40, 0x80);
    static ex::RGBA PredTech(0x40, 0x7f, 0x40, 0x80);
    static ex::RGBA Normal(0x64, 0x24, 0, 0x80);
    switch (tint) {
    case 1: return Thermal;
    case 2: return Neuro;
    case 3: return PredTech;
    }
    return Normal;
}

static unsigned int s_WeaponHashes[23] = { 0x43, 0x44, 0x45, 0x3ea, 0x3eb, 0x3e9, 0x3e8, 0, 1, 2, 4, 3,
                                           5,    6,    7,    0xa,   0xb,   8,     9,     0xc, 0x3f, 0x40, 0xd };

FEWeapon *FEHud::GetWeapon(unsigned int hash)
{
    for (int i = 0; i < 6; i++)
        for (int j = 0; j < 8; j++)
            if (m_Weapons[i][j].m_Hash == hash) return &m_Weapons[i][j];
    return &m_Weapons[0][0];
}

// NEAR: evaluation/register order of the four rect terms differs
EXRect FEHud::GetRadarRect(FERadarInfo &info)
{
    float scale = info.m_Small ? 0.75f : 1.0f;
    float r = scale * info.m_Radius;
    return EXRect(info.m_Centre.x - (int)r - 4, info.m_Centre.y - (int)info.m_Radius - 4, 8 - (int)(r * -2.0f),
                  8 - (int)(info.m_Radius * -2.0f));
}

int FEHud::WeaponIndex(unsigned int hash)
{
    for (unsigned int i = 0; i < 23; i++)
        if (s_WeaponHashes[i] == hash) return i;
    return 0;
}

void FEHud::WeaponArrayDestroy()
{
    for (int i = 0; i < 6; i++)
        for (int j = 0; j < 8; j++) m_Weapons[i][j].Init();
}

void FEHud::InitWaypoints()
{
    m_nWaypoints[0] = 0;
    m_nWaypoints[1] = 0;
    m_WaypointBuf = 0;
}

void FEHud::CommitVisionMode(int mode) { m_Tint = FEMathmos(TintColour(mode)); }

void FEHud::CloseMimicry()
{
    FEMimicry::Delete(m_pObject->Object(0)->Object(0, 0));
    m_MimicryOpen = 0;
}

// NEAR: FERadarInfo/EXExtRenderInfo member-init stores scheduled differently
FEHud::FEHud()
{
    WeaponArrayDestroy();
    FERadar::Instance(1);
    m_nWaypoints[0] = 0;
    m_nWaypoints[1] = 0;
    m_WaypointBuf = 0;
    m_Unkd7d = 1;
    m_Unk2194 = 1;
}

FEHud *FEHud::Instance(signed char create)
{
    if (!m_pInstance && create) {
        m_pInstance = new FEHud;
        FEWnd::FixupZOrder();
    }
    return m_pInstance;
}

FEHud::~FEHud()
{
    WeaponArrayDestroy();
    m_pInstance = 0;
}

signed char FEHud::IsWeaponAvailable(unsigned int hash)
{
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 8; j++) {
            if (m_Weapons[i][j].m_Hash == hash) {
                if (!m_Weapons[i][j].m_Unk26 && m_Weapons[i][j].m_UsesAmmo &&
                    !XItemHandler_Player::Instance()->m_Ammo[m_Weapons[i][j].m_Hash])
                    return 0;
                return 1;
            }
        }
    }
    return 0;
}

// NEAR: loop/stack layout of the per-category counters differs
signed char FEHud::WeaponArrayCreate(unsigned int *pList, unsigned int n)
{
    unsigned int count[6] = { 0, 0, 0, 0, 0, 0 };
    WeaponArrayDestroy();
    if (pList) {
        for (unsigned int i = 0; i < n; i++) {
            FEWeapon w = GetWeaponPreset(pList[i], -1, 0);
            m_Weapons[w.m_Cat][count[w.m_Cat]++] = w;
        }
    } else {
        n = 0;
        GetWeaponPreset(0, 0, &n);
        for (unsigned int i = 0; i < n; i++) {
            FEWeapon w = GetWeaponPreset(0, i, 0);
            m_Weapons[w.m_Cat][count[w.m_Cat]++] = w;
        }
    }
    XItemHandler_Player::Instance()->InitPlayerBodyWeapons();
    return 1;
}

// NEAR: retail keeps 0 in edx late; ours hoists it into ebx (register allocation)
signed char FEHud::WeaponArrayDel(unsigned int hash, FEWeaponCat::Flags cat)
{
    FEWeapon w = GetWeaponPreset(hash, -1, 0);
    if (cat == 4) cat = w.m_Cat;
    unsigned int j;
    for (j = 0; j < 8; j++)
        if (m_Weapons[cat][j].m_Hash == hash) break;
    if (j >= 8) return 0;
    for (; j < 7; j++) m_Weapons[cat][j] = m_Weapons[cat][j + 1];
    m_Weapons[cat][7].Init();
    if (XItemHandler_Player::Instance()->m_CurrentWeapon[0] == hash) XItemHandler_Player::Instance()->SetCurrentWeapon(0, 0xd);
    if (XItemHandler_Player::Instance()->m_CurrentWeapon[1] == hash) XItemHandler_Player::Instance()->SetCurrentWeapon(1, 0xd);
    if (XItemHandler_Player::Instance()->m_CurrentWeapon[2] == hash) XItemHandler_Player::Instance()->SetCurrentWeapon(2, 0xd);
    if (XItemHandler_Player::Instance()->m_CurrentWeapon[3] == hash) XItemHandler_Player::Instance()->SetCurrentWeapon(3, 0xd);
    return 1;
}

signed char FEHud::Draw()
{
    Text_Update();
    if ((State() & 0x40) && (!theApp.IsFadeActive() || !theApp.GivenPlayerControl())) {
        RenderWaypoints();
        return FEWnd::Draw();
    }
    InitWaypoints();
    if (m_pObject) m_pObject->MathmosGoto((FEMathmosPos::Flags)1);
    m_Fade.SetEndColour(ex::RGBA(0, 0, 0, 0));
    m_Fade.Goto((FEMathmosPos::Flags)2);
    return FEWnd::Draw();
}

void FEHud::NotifyCutscene(signed char b)
{
    if (b) {
        StatePush();
        StateSub(0x40);
    } else {
        StatePop();
    }
    if (m_pObject) m_pObject->Pause(b);
}

EXRect FEHud::TextRect()
{
    EXRect r(0, 0, 0, 0);
    if (m_pObject) {
        FEObject *p = m_pObject->Object(0);
        if (p) {
            FEObject *q = p->Object(0, 0);
            if (q) {
                EXPoint pt = q->FindDatum2D(0x100000c0);
                pt = q->OutPoint(pt);
                r = EXRect(pt.x, pt.y, EXBaseDisplay::GetDisplay()->Width() - pt.x * 2, 1000);
            }
        }
    }
    return r;
}

void FEHud::HealthStringUpdate(signed char force)
{
    float h = XItemHandler_Player::Instance()->m_Health;
    if (h > 0.0f) h += 0.5f;
    if (h == m_HealthDisp && !force) return;
    if (force) {
        m_HealthDisp = h;
    } else {
        float mul = EXBaseApp::GameFrameRateMul();
        if (m_HealthDisp - mul > h)
            m_HealthDisp = m_HealthDisp - mul;
        else if (h > m_HealthDisp + mul)
            m_HealthDisp = m_HealthDisp + mul;
        else
            m_HealthDisp = h;
    }
    m_Strings2[0].Format("%d", m_HealthDisp <= 0.0f ? 0 : (m_HealthDisp <= 1.0f ? 1 : (int)m_HealthDisp));
}

void FEHud::EnergyStringUpdate(signed char force)
{
    float e = XItemHandler_Player::Instance()->m_Energy;
    e -= XItemHandler_Player::Instance()->GetPlasmaChargeEnergy();
    if (e > 0.0f) e += 0.5f;
    if (e == m_EnergyDisp && !force) return;
    if (force) {
        m_EnergyDisp = e;
    } else {
        float mul = EXBaseApp::GameFrameRateMul();
        if (m_EnergyDisp - mul > e)
            m_EnergyDisp = m_EnergyDisp - mul;
        else if (e > m_EnergyDisp + mul)
            m_EnergyDisp = m_EnergyDisp + mul;
        else
            m_EnergyDisp = e;
    }
    m_Strings2[1].Format("%d", m_EnergyDisp <= 0.0f ? 0 : (m_EnergyDisp <= 1.0f ? 1 : (int)m_EnergyDisp));
}

ex::RGBA FEHud::GetVisibilityAlpha()
{
    int vis = 0;
    for (Actor *a = Actor::GetFirst(1); a; a = a->GetNext(1)) {
        if (a->WasPlayerSeen()) {
            vis = 2;
            break;
        }
        if (a->CouldPlayerBeSeen()) vis = 1;
    }
    m_VisibilityTimer -= XApp::GameFrameRateMul();
    if (m_VisibilityTimer <= 0.0f || vis > m_Visibility) {
        m_Visibility = vis;
        m_VisibilityTimer = 60.0f;
    }
    ex::RGBA c(0x7f, 0x7f, 0x7f, 0x80);
    switch (m_Visibility) {
    case 0: c.m_A = 0x20; break;
    case 1: c.m_A = 0x40; break;
    case 2: c.m_A = 0x80; break;
    }
    return c;
}

void FEHud::UpdateTopRight()
{
    if (!(m_pObject->Object(0)->m_Flags & 0x40)) return;
    FEObject *p = m_pObject->Object(0)->Object(0, 1);
    if (!p) {
        p = m_pObject->Object(0)->ObjectAdd(0, 1, new FEObject(FEObjType::OT_SCRIPT, 0x1000025, 0x200022a, 0xfdffffff, FE_Blank,
                                                               FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)6));
        FE::_Init(p);
        FE::_Pop();
    }
    RenderWeaponData(0, 0x100000c4, 0x100000c5, p, (EXW::TextAlign)7);
    RenderWeaponData(2, 0x100000c6, 0x100000c7, p, (EXW::TextAlign)7);
    RenderWeaponData(3, 0x100000f3, 0x100000f4, p, (EXW::TextAlign)7);
}

void FEHud::UpdateBottomRight()
{
    EnergyStringUpdate(0);
    if (!(m_pObject->Object(0)->m_Flags & 0x40)) return;
    FEObject *p = m_pObject->Object(0)->Object(1, 1);
    if (!p) {
        p = m_pObject->Object(0)->ObjectAdd(1, 1, new FEObject(FEObjType::OT_SCRIPT, 0x1000025, 0x2000239, 0xfdffffff, FE_Blank,
                                                               FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)8));
        FE::_Init(p);
        FE::_Pop();
    }
    p->DrawText((FERenderPass::Flags)2, 0x100003a, 0x7000004, TintColour(XItemHandler_Player::Instance()->m_Tint), 2,
                0x100000f6, m_Strings2[1], (EXW::TextAlign)7, 0);
}

void FEHud::RenderWeaponData(int i, unsigned int datum, unsigned int datum2, FEObject *p, EXW::TextAlign align)
{
    FEWeapon *w = GetWeapon(XItemHandler_Player::Instance()->GetCurrentWeapon(i));
    if (w->m_Hash == 0xd) return;
    p->DrawSprite((FERenderPass::Flags)2, 0x1000025, w->m_Icon, m_Mathmos[i].Update(), 2, datum, EXPoint(0x18, 0x18),
                  (EXW::TextAlign)4);
    if (w->m_UsesAmmo) {
        m_Strings[i].Format("%d", XItemHandler_Player::Instance()->GetAmmo(XItemHandler_Player::Instance()->GetCurrentWeapon(i)));
        p->DrawText((FERenderPass::Flags)2, 0x100003a, 0x7000004, TintColour(XItemHandler_Player::Instance()->m_Tint), 2, datum2,
                    m_Strings[i], align, 0);
        return;
    }
    int cost = w->m_Unk20;
    if (cost > 0) {
        float energy = XItemHandler_Player::Instance()->m_Energy;
float fcost = (float)cost;
        if (energy - XItemHandler_Player::Instance()->GetPlasmaChargeEnergy() >= fcost)
            p->DrawSprite((FERenderPass::Flags)2, 0x1000025, 0x6000294, TintColour(XItemHandler_Player::Instance()->m_Tint), 2,
                          datum2, EXPoint(0x18, 0x18), align);
    }
}

void FEHud::ContentUpdate()
{
    m_ActionsChanged = ((XGamePad *)EXBaseGamePad::GetGamePad())->GetActionsChanged();
    m_Mathmosc3c.Update();
    if (theApp.GetCutscene()) return;
    if (!m_pObject) return;
    unsigned int state = m_State;
    if (GameData::Instance(1)->m_Values.m_Unk1c[3] != m_Rumble) {
        EXBaseGamePad::GetGamePad()->EnableAllRumble(GameData::Instance(1)->m_Values.m_Unk1c[3]);
        m_Rumble = GameData::Instance(1)->m_Values.m_Unk1c[3];
    }
    m_Tint.Update();
    UpdateWeaponSelect();
    UpdateBossBar();
    UpdateTopLeft();
    UpdateTopRight();
    UpdateBottomLeft();
    UpdateBottomRight();
    UpdateFirstPerson();
    UpdateThermoScan();
    UpdateNeuroScan();
    UpdateTechScan();
    UpdateTargeting();
    m_State = state;
    UpdateDisablePlayer();
    if (!GameData::Instance(1)->Busy() && !FEListener::DialogListener())
        StateAdd(1);
    else
        StateSub(1);
    HandleInput();
}

void FEHud::DrawWaypoint(XItem *pItem)
{
    if (!GameData::Instance(1)->m_Values.m_Unk1c[6] && !XGetCamera(0)->IsFirstPerson()) return;
    if (!theApp.IsRunning()) return;
    m_Waypoints[m_WaypointBuf][m_nWaypoints[m_WaypointBuf]].m_Type = 0;
    m_Waypoints[m_WaypointBuf][m_nWaypoints[m_WaypointBuf]].m_pItem = pItem;
    m_Waypoints[m_WaypointBuf][m_nWaypoints[m_WaypointBuf]].m_Pos = pItem->Position() + EXVector(0.0f, 1.0f, 0.0f, 0.0f);
    m_Waypoints[m_WaypointBuf][m_nWaypoints[m_WaypointBuf]].m_Unk4 = 0;
    m_nWaypoints[m_WaypointBuf]++;
}

signed char FEHud::GetRadarInfo(FERadarInfo &info)
{
    FEObject *root = m_pObject;
    if (!root) return 0;
    FEObject *p = root->Object(0)->Object(0, 0);
    if (!p) return 0;
    info.m_Centre = p->FindDatum2D(0x10000034);
    info.m_Centre = p->OutPoint(info.m_Centre);
    EXPoint d = info.m_Centre - p->OutPoint(p->FindDatum2D(0x1000004f));
    info.m_Radius = EXSqrt((float)(d.y * d.y + d.x * d.x));
    d = info.m_Centre - p->OutPoint(p->FindDatum2D(0x100000ca));
    info.m_Radius2 = EXSqrt((float)(d.y * d.y + d.x * d.x));
    info.m_Range = 25.0f;
    info.m_PlayerPos = ((XItem *)XItemHandler_Player::Instance()->GetItem())->Position();
    info.m_pObject = p;
    info.m_Unk28 = 0;
    info.m_Small = EXBaseRenderEnv::WideScreen();
    return 1;
}

void FEHud::UpdateBottomLeft()
{
    HealthStringUpdate(0);
    if (!(m_pObject->Object(0)->m_Flags & 0x40)) return;
    FEObject *p = m_pObject->Object(0)->Object(1, 0);
    if (!p) {
        p = m_pObject->Object(0)->ObjectAdd(1, 0, new FEObject(FEObjType::OT_SCRIPT, 0x1000025, 0x2000238, 0xfdffffff, FE_Blank,
                                                               FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)1));
        FE::_Init(p);
        FE::_Pop();
    }
    p->DrawText((FERenderPass::Flags)2, 0x100003a, 0x7000004, TintColour(XItemHandler_Player::Instance()->m_Tint), 2,
                0x100000f5, m_Strings2[0], (EXW::TextAlign)2, 0);
}


void FEHud::MaskCreate()
{
    m_MaskActive = 0;
    m_pMask = XItem::CreateItem(0, 0, (EXRuntimeClass *)&FEObjectHandler::classFEObjectHandler, EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    EXItemAnimator_Entity *pEntity = new EXItemAnimator_Entity;
    EXGeoFile *pFilec = EXGeoFile::LoadGeoFile(0x1000025, 0, 0);
    pEntity->Initialise(pFilec, 0x200020c);
    m_pMask->AddAnimator(pEntity);
    m_pMask->ItemFlags().BitSet(0x1020);
    m_pMask->ItemFlags().BitClr(1);
    m_pMask->RenderInfo().SetExtRenderInfo(&m_ExtRenderInfo);
    m_ExtRenderInfo.m_RefractionStrength = 0.1f;
    m_ExtRenderInfo.m_Unk78 |= 4;
    pEntity = new EXItemAnimator_Entity;
    EXGeoFile *pFile9 = EXGeoFile::LoadGeoFile(0x1000025, 0, 0);
    pEntity->Initialise(pFile9, 0x2000279);
    pEntity->m_renderInfo.m_renderFlags.m_flags |= 8;
    pEntity->ItemFlags().BitClr(1);
    m_pMask->AddAnimator(pEntity);
    float sx = 0.018f;
    if (EXBaseRenderEnv::WideScreen()) sx = 0.02396f;
    m_pMask->SetScale(EXVector(sx, 0.016f, 0.018f, 0.0f));
    theItemEnv.AddItem(m_pMask, 0);
    MaskUpdate();
}

void FEHud::DrawAffiliation(Actor *pActor)
{
    if (!theApp.IsRunning()) return;
    EXVector pos = pActor->GetHeadPos() + EXVector(0.0f, 0.5f, 0.0f, 0.0f);
    EXVector d = XItemHandler_Player::Instance()->GetItem()->Position() - pos;
    float range = XGetCamera(0)->m_Zoom * 20.0f;
    if (d.y * d.y + d.z * d.z + d.x * d.x > range * range) return;
    m_Waypoints[m_WaypointBuf][m_nWaypoints[m_WaypointBuf]].m_Type = 1;
    m_Waypoints[m_WaypointBuf][m_nWaypoints[m_WaypointBuf]].m_pItem = (XItem *)pActor;
    m_Waypoints[m_WaypointBuf][m_nWaypoints[m_WaypointBuf]].m_Pos = pos;
    m_Waypoints[m_WaypointBuf][m_nWaypoints[m_WaypointBuf]].m_Unk4 = 0;
    m_nWaypoints[m_WaypointBuf]++;
}

void FEHud::ContentCreate()
{
    m_Unkd20 = 0.0f;
    m_MimicryOpen = 0;
    m_Unkd24 = 0;
    m_Unkb94 = -1;
    m_DisablePlayer = 0;
    m_Unkb90[0] = 0;
    m_Unkb98 = 8;
    m_SelectWeapons[0] = 0;
    m_SelectWeapons[1] = 0;
    m_SelectWeapons[2] = 0;
    m_SelectWeapons[3] = 0;
    m_ThermoScanOpen = 0;
    m_ThermoScanFilled = 0;
    m_NeuroScanOpen = 0;
    m_NeuroScanFilled = 0;
    m_TechScanOpen = 0;
    m_TechScanFilled = 0;
    m_Unkd2b = 0;
    m_ActionsChanged = 0;
    m_LockState = 0;
    m_Visibility = 0;
    m_VisibilityTimer = 0.0f;
    m_BossHitID = 0;
    m_BossHitTimer = 0.0f;
    m_HealthDisp = 0.0f;
    m_EnergyDisp = 0.0f;
    m_SelectCategory = 0;
    InitWaypoints();
    m_Mathmos[0] = FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80));
    m_Mathmos[1] = FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80));
    m_Mathmos[2] = FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80));
    m_Mathmos[3] = FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80));
    m_Mathmosc3c = FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), ex::RGBA(0x7f, 0x7f, 0x7f, 0x40), 30.0f, FEWaveform::WF_LINEAR, 0);
    m_Fade = FEMathmos::DefaultTint();
    m_Rumble = GameData::Instance(1)->m_Values.m_Unk1c[3];
    EXBaseGamePad::GetGamePad()->EnableAllRumble(GameData::Instance(1)->m_Values.m_Unk1c[3]);
    memset(&m_ScanInfo, 0, sizeof(m_ScanInfo));
    memset(m_Unkb7c, 0, sizeof(m_Unkb7c));
    SetObject(new FELayoutLayered(9, 0, 0, 0, 0, (EXW::TextAlign)4))->MakeSafeRect();
    m_pObject->ObjectAdd(0, new FELayoutFixedGrid(2, 2, 0, 0, 0, 0, (EXW::TextAlign)4));
    MaskCreate();
    m_Tint = FEMathmos(ex::RGBA(0, 0, 0, 0), TintColour(XItemHandler_Player::Instance()->GetVisionMode()), 30.0f, FEWaveform::WF_LINEAR, 1);
    m_pObject->SetRemoteMathmos((FEObjColour::Flags)3, &m_Tint, 1);
    FEObject *pObj = m_pObject;
    if (pObj) pObj->SetMathmos((FEObjColour::Flags)2, FEMathmos::DefaultFadeUp(), 1);
    StateAdd(0x60);
    EnvironmentStateAdd(2);
    FEListener::RemapModeSet((FERemap::Flags)0);
    HealthStringUpdate(1);
    EnergyStringUpdate(1);
}

void FEHud::UpdateBossBar()
{
    FEObject *pBar = m_pObject->Object(8);
    Actor *pActor = Actor::GetPtrFromID(XGetCamera(0)->m_TargetID);
    if (pActor && pActor->HasBossBar() && pActor->GetProfile()->m_Honour > 0 && XGetCamera(0)->m_Mode == 1)
        RegisterBossHit(pActor->m_RefID);
    else
        pActor = Actor::GetPtrFromID(m_BossHitID);
    if (pActor && pActor->GetProfile()->m_Honour > 0 && pActor->HasBossBar()) {
        if (!pBar) {
            pBar = m_pObject->ObjectAdd(8, new FEObject(FEObjType::OT_SCRIPT2, 0x1000025, 0x400038a, 0xfdffffff, FE_Blank, FE_Blank,
                                                        (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)3));
            FE::_Init(pBar);
            FE::_Peek()->Script(0)->Play(0, 0x180000);
            FE::_Peek()->SetMathmos((FEObjColour::Flags)3, ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 1);
            FE::_Peek()->SetMathmos((FEObjColour::Flags)2,
                                    FEMathmos(ex::RGBA(0, 0, 0, 0), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 64.0f, FEWaveform::WF_LINEAR, 1), 1);
            FE::_Peek()->Script(0)->TweakPosition(EXVector(0.0f, 0.0f, -1.0f, 0.0f));
            FE::_Pop();
        } else if (m_BossHitTimer == 5.0f) {
            ex::RGBA to(0x7f, 0x7f, 0x7f, 0x80);
            ex::RGBA from = pBar->Mathmos((FEObjColour::Flags)2)->m_Colour;
            pBar->SetMathmos((FEObjColour::Flags)2, FEMathmos(from, to, 64.0f, FEWaveform::WF_LINEAR, 1), 1);
        } else if (m_BossHitTimer < 1.0f) {
            pBar->SetMathmos((FEObjColour::Flags)2, ex::RGBA(0x7f, 0x7f, 0x7f, (u8)(int)(m_BossHitTimer * 127.0f)), 1);
        }
        pBar->Script(0)->SetAmountPlayed(pActor->m_Health / pActor->GetProfile()->m_Health);
    } else if (pBar) {
        pBar->m_DeleteNeeded = 1;
    }
    if (m_BossHitTimer > 0.0f) {
        m_BossHitTimer -= XApp::FrameLength();
        if (m_BossHitTimer <= 0.0f) m_BossHitID = 0;
    }
}

void FEHud::UpdateTopLeft()
{
    FEObject *p = m_pObject->Object(0)->Object(0, 0);
    if (!p) {
        p = m_pObject->Object(0)->ObjectAdd(0, 0, new FEObject(FEObjType::OT_SCRIPT2, 0x1000025, 0x400010d, 0xfdffffff, FE_Blank,
                                                               FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)0));
        FE::_Init(p);
        FE::_Peek()->Script(0)->Play(0, 0);
        FE::_Peek()->ScriptAdd(0x5f5f5f6d72616c61, FEObjType::OT_SCRIPT2, 0x1000025, 0x40004e0, (EXW::TextAlign)4);
        FE::_Peek()->Script(0x5f5f5f6d72616c61)->SetAttachedTo(0);
        FE::_Peek()->Script(0x5f5f5f6d72616c61)->SetRemoteMathmos(&m_Fade);
        FE::_Peek()->Script(0x5f5f5f6d72616c61)->Play(0, 0x80000);
        FE::_Peek()->ScriptAdd(0x5f5f5f5f7a7a7566, FEObjType::OT_SCRIPT2, 0x1000025, 0x400077f, (EXW::TextAlign)4);
        FE::_Peek()->Script(0x5f5f5f5f7a7a7566)->SetAttachedTo(0);
        FE::_Peek()->Script(0x5f5f5f5f7a7a7566)->Play(0, 0x80000);
        FE::_Pop();
    }
    if (m_LockState == 2 && (m_ThermoScanOpen || m_NeuroScanOpen || m_TechScanOpen)) {
        m_pObject->Object(0)->StateSub(0x40);
        FERadar::Instance(1)->StateSub(0x40);
    } else {
        m_pObject->Object(0)->StateAdd(0x40);
        FERadar::Instance(1)->StateAdd(0x40);
    }
    if (CurrentMap.m_Hash)
        p->Script(0x5f5f5f5f7a7a7566)->StateSub(0x40);
    else
        p->Script(0x5f5f5f5f7a7a7566)->StateAdd(0x40);
    float alarm = Mission_AlarmLevel;
    if (Mission_AlarmMeterHidden) alarm = 0.0f;
    float f = alarm * 0.01f;
    p->Script(0x5f5f5f6d72616c61)->SetAmountPlayed(f);
    ex::RGBA c;
    c.m_R = 0x7f;
    c.m_G = 0x7f;
    c.m_B = 0x7f;
    u8 a;
    if (alarm >= 100.0f)
        a = m_Mathmosc3c.Colour().m_A;
    else
        a = 0x40 - (u8)(int)(f * -64.0f);
    c.m_A = (u8)(int)((float)(int)((unsigned int)p->Mathmos((FEObjColour::Flags)2)->Colour() >> 24) * 0.0078125f * a);
    m_Fade.SetEndColour(c);
    m_Fade.Goto((FEMathmosPos::Flags)2);
    if (m_Unkd20 != 0.0f) {
        if (m_Unkd24)
            FEMimicry::Create(p, (int)m_Unkd20, 0x400054f, 0x4000780);
        else
            FEMimicry::Create(p, (int)m_Unkd20, 0x400054f, 0x4000781);
        m_Unkd20 = 0.0f;
        m_MimicryOpen = 1;
    }
    if (m_MimicryOpen) m_MimicryOpen = FEMimicry::Update(p, !theApp.IsRunning());
}

FEWeapon FEHud::GetWeaponPreset(unsigned int hash, int idx, unsigned int *pCount)
{
    static FEWeapon s_Presets[23] = {
        FEWeapon((FEWeaponCat::Flags)5, s_WeaponHashes[0], 0x60002fc, 0x510007ee, 0x510007e6, 0, 0, 0, 0, 0, 0),
        FEWeapon((FEWeaponCat::Flags)5, s_WeaponHashes[1], 0x60002fd, 0x510007ef, 0x510007e7, 0, 0, 0, 0, 0, 0),
        FEWeapon((FEWeaponCat::Flags)5, s_WeaponHashes[2], 0x60002fe, 0x510007f0, 0x510007e8, 0, 0, 0, 0, 0, 0),
        FEWeapon((FEWeaponCat::Flags)5, s_WeaponHashes[3], 0x600029b, 0x510007f3, 0x510007ea, 0, 0, 0, 0, 1, 0),
        FEWeapon((FEWeaponCat::Flags)5, s_WeaponHashes[4], 0x60002f9, 0x510007f4, 0x510007eb, 0, 0, 0, 0, 1, 0),
        FEWeapon((FEWeaponCat::Flags)5, s_WeaponHashes[5], 0x60002fa, 0x510007f5, 0x510007ec, 0, 0, 0, 0, 1, 0),
        FEWeapon((FEWeaponCat::Flags)5, s_WeaponHashes[6], 0x60002fb, 0x510007f6, 0x510007ed, 0, 0, 0, 0, 1, 0),
        FEWeapon((FEWeaponCat::Flags)3, s_WeaponHashes[7], 0x6000018, 0x51000032, 0x510005fd, 0, 0, 0, 0, 0, 0),
        FEWeapon((FEWeaponCat::Flags)3, s_WeaponHashes[8], 0x6000017, 0x51000033, 0x510005fe, 0x510006c2, 0x510006c3, 0, 0, 1, 0),
        FEWeapon((FEWeaponCat::Flags)1, s_WeaponHashes[9], 0x6000019, 0x51000004, 0x510005ff, 0x510006c4, 0x510006c5, 0, 0, 1, 0),
        FEWeapon((FEWeaponCat::Flags)1, s_WeaponHashes[10], 0x600001e, 0x51000007, 0x51000600, 0x510006c6, 0x510006c7, 0, 0, 1, 0),
        FEWeapon((FEWeaponCat::Flags)1, s_WeaponHashes[11], 0x600001d, 0x51000006, 0x51000601, 0x510006c8, 0x510006c9, 0, 0, 1, 0),
        FEWeapon((FEWeaponCat::Flags)0, s_WeaponHashes[12], 0x600001c, 0x51000009, 0x51000603, 0x510006cc, 0x510006cd, 0, 1, 1, 1),
        FEWeapon((FEWeaponCat::Flags)0, s_WeaponHashes[13], 0x60000bb, 0x510000b7, 0x51000602, 0x510006ca, 0x510006cb, 10, 0, 1, 0),
        FEWeapon((FEWeaponCat::Flags)0, s_WeaponHashes[14], 0x600001f, 0x5100000b, 0x51000604, 0x510006ce, 0x510006cf, 25, 0, 1, 0),
        FEWeapon((FEWeaponCat::Flags)0, s_WeaponHashes[15], 0x6000030, 0x5100000d, 0x510006d0, 0, 0, 0, 1, 0, 0),
        FEWeapon((FEWeaponCat::Flags)0, s_WeaponHashes[16], 0x6000031, 0x5100000e, 0x510006d1, 0, 0, 0, 1, 0, 0),
        FEWeapon((FEWeaponCat::Flags)0, s_WeaponHashes[17], 0x600002a, 0x51000014, 0x510006d2, 0, 0, 0, 1, 0, 0),
        FEWeapon((FEWeaponCat::Flags)0, s_WeaponHashes[18], 0x6000041, 0x51000015, 0x510006d3, 0, 0, 0, 1, 0, 0),
        FEWeapon((FEWeaponCat::Flags)2, s_WeaponHashes[19], 0x6000035, 0x51000010, 0x51000784, 0, 0, 0, 1, 0, 0),
        FEWeapon((FEWeaponCat::Flags)2, s_WeaponHashes[20], 0x6000088, 0x51000012, 0x510007e9, 0, 0, 0, 0, 0, 0),
        FEWeapon((FEWeaponCat::Flags)2, s_WeaponHashes[21], 0x6000089, 0x51000013, 0x510007e9, 0, 0, 0, 0, 0, 0),
        FEWeapon((FEWeaponCat::Flags)4, s_WeaponHashes[22], 0, 0x510002be, 0, 0, 0, 0, 0, 0, 0),
    };
    if (pCount) *pCount = 22;
    if (idx <= -1) {
        for (unsigned int i = 0; i < 23; i++) {
            if (s_Presets[i].m_Hash == hash) {
                idx = i;
                break;
            }
        }
    }
    if (idx < 0) return s_Presets[0];
    return s_Presets[idx];
}

void FEHud::MaskUpdate()
{
    signed char vis = m_pMask->ItemFlags().m_flags & 1;
    if (!XGetCamera(0)->IsFirstPerson() && vis) {
        m_pMask->ItemFlags().BitClr(1);
    } else if (XGetCamera(0)->IsFirstPerson() && !vis) {
        if (m_Unk2194) m_pMask->SetHasRender();
        return;
    }
    if (!vis) return;
    XItemHandler_Camera *pc = XGetCamera(0);
    float t = qtanf(0.39269909f);
    EXVector up(0.0f, 1.0f, 0.0f, 0.0f);
    float ratio = t / qtanf(0.39269909f / pc->m_Zoom);
    float s = 1.0f / ratio;
    EXVector offset(0.0f, s * -0.002f, 0.1f, 0.0f);
    m_pMask->SetScale(EXVector(s * 0.018f, s * 0.016f, 0.018f, 0.0f));
    EXMatrix m;
    m.SetZero();
    EXCamera *pCam = XGetCamera(0)->GetCamera();
    EXVector look = XGetCamera(0)->m_CameraLookAt;
    EXVector pos = XGetCamera(0)->m_CameraPos;
    m.LookAt(pos, look, pCam->m_WorldUp, up);
    m_pMask->SetRotOrder(0);
    EXQuaternion q;
    m.GetRotation(q);
    offset *= m;
    offset += XGetCamera(0)->m_CameraPos;
    m_pMask->SetPosition(offset);
    m_pMask->SetRotation(q);
    EXItemAnimator *pAnim = m_pMask->GetAnimator(0x200020c);
    if (!m_MaskActive) {
        if (XItemHandler_Player::Instance()->GetCloak()) {
            pAnim->m_renderInfo.m_renderFlags.m_flags |= 0x2000;
            m_MaskActive = 1;
            pAnim->m_renderInfo.m_renderFlags.m_flags |= 8;
        }
    } else if (!XItemHandler_Player::Instance()->GetCloak()) {
        pAnim->m_renderInfo.m_renderFlags.m_flags &= ~0x2000;
        m_MaskActive = 0;
        pAnim->m_renderInfo.m_renderFlags.m_flags &= ~8;
    }
    pAnim = m_pMask->GetAnimator(0x2000279);
    if (XItemHandler_Player::Instance()->SparkTimer() != 0.0f)
        pAnim->SetHasRender();
    else
        pAnim->ClrHasRender();
}

// Adds a weapon to its category row, keeping the row in the fixed weapon order.
// Returns 0 when the weapon is already there or the row is full.
signed char FEHud::WeaponArrayAdd(unsigned int hash, FEWeaponCat::Flags cat)
{
    FEWeapon w = GetWeaponPreset(hash, -1, 0);
    if (cat != 4) w.m_Cat = cat;
    int c = w.m_Cat;
    XItemHandler_Player::Instance()->PickedUpWeapon(w.m_Hash);
    unsigned int order;
    for (order = 0; order < 23; order++)
        if (s_WeaponHashes[order] == hash) break;
    if (order >= 23) order = 0;
    if (hash == 0xd && cat == 2) w.m_Text = 0x51000969;  // "No Item"
    if (WeaponArrayDel(0xd, (FEWeaponCat::Flags)c)) XItemHandler_Player::Instance()->SetCurrentWeapon(c, w.m_Hash);
    for (unsigned int j = 0; j < 8; j++) {
        unsigned int h = m_Weapons[c][j].m_Hash;
        if (h == hash) return 0;
        if (h == 0xfeffffff) {
            m_Weapons[c][j] = w;
            return 1;
        }
        unsigned int o;
        for (o = 0; o < 23; o++)
            if (s_WeaponHashes[o] == h) break;
        if (o >= 23) o = 0;
        if ((int)order < (int)o) {
            for (unsigned int k = 7; k > j; k--) m_Weapons[c][k] = m_Weapons[c][k - 1];
            m_Weapons[c][j] = w;
            return 1;
        }
    }
    return 0;
}

extern XGameWnd *gpGameWnd[4];
extern int Actor_CalledRayCast;
float Vec_AngleDifference(float a, float b);

// Scales a vector to unit length (left alone when it is degenerate).
static void WP_Normalise4(float *v)
{
    float lenSq = v[0] * v[0] + v[1] * v[1] + v[2] * v[2] + v[3] * v[3];
    if (lenSq >= 0.0f) {
        float len = (float)sqrt(lenSq);
        if (len > 1e-5f) {
            float inv = 1.0f / len;
            v[0] *= inv;
            v[1] *= inv;
            v[2] *= inv;
            v[3] *= inv;
        }
    }
}

// Draws the queued waypoint markers: arrows for off-screen targets, a distance marker for far or hidden ones.
// The waypoint with the stalest visibility test is ray cast against the map each frame.
void FEHud::RenderWaypoints()
{
    if (m_nWaypoints[m_WaypointBuf] > 0) {
        gpGameWnd[0]->SetupCamera();
        int other = (m_WaypointBuf == 0);
        int oldest = -1;
        int oldestAge = 0x7fff;
        for (int i = 0; i < m_nWaypoints[m_WaypointBuf]; i++) {
            FEWaypoint &w = m_Waypoints[m_WaypointBuf][i];
            int sy = (int)EXBaseDisplay::WorldToDisp(w.m_Pos).y;
            int sx = (int)EXBaseDisplay::WorldToDisp(w.m_Pos).x;
            w.m_ScreenX = sx;
            w.m_ScreenY = sy;
            // Inherit the visibility result from last frame's entry for the same item.
            for (int j = 0; j < m_nWaypoints[other]; j++) {
                if (w.m_pItem == m_Waypoints[other][j].m_pItem) {
                    w.m_bHit = m_Waypoints[other][j].m_bHit;
                    w.m_Unk4 = m_Waypoints[other][j].m_Unk4;
                    break;
                }
            }
            if (w.m_Unk4 < oldestAge) {
                oldestAge = w.m_Unk4;
                oldest = i;
            }
        }
        if (oldest >= 0) {
            FEWaypoint &w = m_Waypoints[m_WaypointBuf][oldest];
            EXRayCastTemp r;
            EXVector start = XGetCamera(0)->GetItem()->Position();
            EXVector dir(w.m_Pos.x - start.x, w.m_Pos.y - start.y, w.m_Pos.z - start.z, w.m_Pos.w - start.w);
            r.Set(start, dir, 0, 0, 0, 0);
            r.FindAnyHit();
            theItemEnv.DoRayCastToContoured(r);
            w.m_bHit = r.m_bHit;
            if (!r.m_bHit) {
                dir = EXVector(w.m_Pos.x - start.x, w.m_Pos.y - start.y, w.m_Pos.z - start.z, w.m_Pos.w - start.w);
                r.Set(start, dir, 0, 0, 0, 0);
                Actor_CalledRayCast++;
                r.FindAnyHit();
                theItemEnv.DoRayCastToMap(r, 0);
                w.m_bHit = r.m_bHit;
            }
            w.m_Unk4 = m_FrameCount;
        }
    }
    if (m_nWaypoints[m_WaypointBuf] > 0) {
        FEBaseWnd::SetupCamera(0);
        EXRect safe = EXBaseDisplay::GetDisplay()->SafeFrame();
        int xMin = safe.x + 0x20;
        int yMin = safe.y + 0x10;
        int xRange = safe.w - 0x40;
        int yRange = safe.h - 0x30;
        for (int i = 0; i < m_nWaypoints[m_WaypointBuf]; i++) {
            FEWaypoint &w = m_Waypoints[m_WaypointBuf][i];
            unsigned char alpha = w.m_bHit ? 0x40 : 0x80;
            ex::RGBA grey(0x7f, 0x7f, 0x7f, alpha);
            ex::RGBA red(0x7f, 0, 0, alpha);
            XItemHandler_Camera *pCam;
            if (w.m_Type == 0) {
                pCam = XGetCamera(0);
                float toWp[4] = { pCam->m_CameraPos.x - w.m_Pos.x, pCam->m_CameraPos.y - w.m_Pos.y,
                                  pCam->m_CameraPos.z - w.m_Pos.z, pCam->m_CameraPos.w - w.m_Pos.w };
                pCam = XGetCamera(0);
                EXVector look = pCam->m_CameraLookAt;
                pCam = XGetCamera(0);
                float toLook[4] = { pCam->m_CameraPos.x - look.x, pCam->m_CameraPos.y - look.y,
                                    pCam->m_CameraPos.z - look.z, pCam->m_CameraPos.w - look.w };
                const EXVector &pp = XItemHandler_Player::Instance()->GetItem()->Position();
                float dx = pp.x - w.m_Pos.x, dy = pp.y - w.m_Pos.y, dz = pp.z - w.m_Pos.z;
                float distSq = dx * dx + dy * dy + dz * dz;
                float dist = distSq < 0.0f ? 0.0f : (float)sqrt(distSq);
                WP_Normalise4(toLook);
                WP_Normalise4(toWp);
                float dot = toLook[0] * toWp[0] + toLook[1] * toWp[1] + toLook[2] * toWp[2] + toLook[3] * toWp[3];
                if (dot <= 0.0f) {
                    // Behind the camera: arrow on the side the target is on.
                    float angWp = (float)atan2(toWp[0], toWp[2]);
                    float angCam = (float)atan2(toLook[0], toLook[2]);
                    if (Vec_AngleDifference(angWp, angCam) <= 0.0f)
                        DrawSprite((FERenderPass::Flags)3, 0x1000025, 0x6000085, red, 0,
                                   EXPoint(xRange + xMin, yMin + yRange / 2), EXPoint(0x10, 0x10), (EXW::TextAlign)4);
                    else
                        DrawSprite((FERenderPass::Flags)3, 0x1000025, 0x6000084, red, 0,
                                   EXPoint(xMin, yMin + yRange / 2), EXPoint(0x10, 0x10), (EXW::TextAlign)4);
                } else if (dist >= 20.0f || w.m_bHit) {
                    // In front but far away or hidden: clamp to the safe frame and label with the distance.
                    if (w.m_ScreenX < xMin) w.m_ScreenX = xMin;
                    else if (w.m_ScreenX > xRange + xMin) w.m_ScreenX = xRange + xMin;
                    if (w.m_ScreenY < yMin) w.m_ScreenY = yMin;
                    else if (w.m_ScreenY > yRange + yMin) w.m_ScreenY = yRange + yMin;
                    float cx = (float)EXBaseDisplay::GetDisplay()->Width() * 0.5f - (float)w.m_ScreenX;
                    float cy = (float)EXBaseDisplay::GetDisplay()->Height() * 0.5f - (float)w.m_ScreenY;
                    float lenSq = cx * cx + cy * cy;
                    if (lenSq >= 0.0f) {
                        float len = (float)sqrt(lenSq);
                        if (len > 1e-5f) {
                            float inv = 1.0f / len;
                            cx *= inv;
                            cy *= inv;
                        }
                    }
                    cx *= 32.0f;
                    cy *= 32.0f;
                    char buf[16];
                    EXString text;
                    sprintf(buf, "%d", (int)dist);
                    text = buf;
                    DrawSprite((FERenderPass::Flags)3, 0x1000016, 0x60001a8, grey, 0, EXPoint(w.m_ScreenX, w.m_ScreenY),
                               EXPoint(0x20, 0x20), (EXW::TextAlign)4);
                    DrawText((FERenderPass::Flags)3, 0x100003a, 0x7000004, grey, 0,
                             EXPoint(w.m_ScreenX + (int)cx, w.m_ScreenY + (int)cy), text, (EXW::TextAlign)4, 0);
                }
            } else if (w.m_Type == 1) {
                pCam = XGetCamera(0);
                float toWp[4] = { pCam->m_CameraPos.x - w.m_Pos.x, pCam->m_CameraPos.y - w.m_Pos.y,
                                  pCam->m_CameraPos.z - w.m_Pos.z, pCam->m_CameraPos.w - w.m_Pos.w };
                pCam = XGetCamera(0);
                EXVector look = pCam->m_CameraLookAt;
                pCam = XGetCamera(0);
                float toLook[4] = { pCam->m_CameraPos.x - look.x, pCam->m_CameraPos.y - look.y,
                                    pCam->m_CameraPos.z - look.z, pCam->m_CameraPos.w - look.w };
                WP_Normalise4(toLook);
                WP_Normalise4(toWp);
                float dot = toLook[0] * toWp[0] + toLook[1] * toWp[1] + toLook[2] * toWp[2] + toLook[3] * toWp[3];
                if (dot > 0.9f)
                    DrawSprite((FERenderPass::Flags)3, 0x1000025, 0x600025d, grey, 0, EXPoint(w.m_ScreenX, w.m_ScreenY),
                               EXPoint(0x10, 0x10), (EXW::TextAlign)4);
            }
        }
    }
    m_WaypointBuf = (m_WaypointBuf == 0);
    m_nWaypoints[m_WaypointBuf] = 0;
}
