#include "XItemHandler_Environment.h"
#include "XItemHandler_Camera.h"
#include "XItem.h"
#include "XApp.h"
#include "ActorOptions.h"
#include "EXMemory.h"
#include "FEText.h"
#include "EXGeoTrigger.h"
#include "EXDatum.h"
#include "EXLine.h"
#include "XItemHandler_Player.h"
#include "FEHud.h"
#include "EXBaseSoundManager.h"
#include "EXString.h"
#include "GameData.h"
#include "FEPause.h"
#include "ActorTimeTable.h"
#include "XItemHandler_ScriptBase.h"
#include "XItemHandler_Objective.h"
#include "XItemHandler_Camera.h"
#include "EXQuaternion.h"

s8 Within(EXVector p, EXGeoTriggerDatum d);
s8 IsValid(EXGeoTriggerDatum d);

signed char Tethys;
XItemList gEnvList;
XItemList gPickupList;

EX_IMPLEMENT_DYNCREATE_ROOT(XItemHandler_Environment)

s8 XItemHandler_Environment::Initialise()
{
    if (!EXItemHandler::Initialise()) return 0;
    m_bTeleportPending = 0;
    m_bCollided = 0;
    m_Unk85 = 0;
    m_Unk40 = 0;
    gEnvList.Add((XItem *)m_pItem);
    Tethys = 0;
    return 1;
}

void RemoveAllPendingTeleports()
{
    for (XItem *p = gEnvList.GetFirst(); p; p = gEnvList.GetNext(p)) {
        XItemHandler_Environment *h = (XItemHandler_Environment *)p->ItemHandler();
        if (h->m_Type == 0xe || h->m_Type == 0x12) h->m_bTeleportPending = 0;
    }
}

signed char IsEnvironment(EXItem *pItem)
{
    EXItemHandler *h = pItem->ItemHandler();
    if (h && h->IsKindOf(EXRUNTIME_CLASS(XItemHandler_Environment))) return 1;
    return 0;
}

EX_IMPLEMENT_DYNCREATE_ROOT(XItemHandler_AmmoPickup)

XItemHandler_AmmoPickup::~XItemHandler_AmmoPickup()
{
    gPickupList.Remove((XItem *)m_pItem);
}

s8 XItemHandler_AmmoPickup::Initialise()
{
    if (!EXItemHandler::Initialise()) return 0;
    gPickupList.Add((XItem *)m_pItem);
    m_Unk8 = 0;
    return 1;
}

EX_IMPLEMENT_DYNCREATE_ROOT(XItemHandler_Touchplate)

XItemHandler_Touchplate::~XItemHandler_Touchplate() {}

s8 XItemHandler_Touchplate::Initialise()
{
    if (!EXItemHandler::Initialise()) return 0;
    m_Timer = 0.0f;
    return 1;
}

EX_IMPLEMENT_DYNCREATE_ROOT(XItemHandler_Upgrades)

XItemHandler_Upgrades::~XItemHandler_Upgrades() {}

s8 XItemHandler_Upgrades::Initialise()
{
    if (!EXItemHandler::Initialise()) return 0;
    m_bActive = 1;
    return 1;
}

EX_IMPLEMENT_DYNCREATE_ROOT(XItemHandler_BeamTrigger)

XItemHandler_BeamTrigger::~XItemHandler_BeamTrigger() {}

s8 XItemHandler_BeamTrigger::Initialise()
{
    if (!EXItemHandler::Initialise()) return 0;
    return 1;
}

XItemHandler_Environment::~XItemHandler_Environment()
{
    switch (m_Type) {
    case 4:
        XApp::SetGlobalGameFrameRate(1.0f);
        break;
    case 6:
        if (XGetCamera(0)) XGetCamera(0)->GoFirstPerson(0);
        break;
    }
    if (m_FlagChannel) FlagChannels[m_FlagChannel] = 1;
    gEnvList.Remove((XItem *)m_pItem);
}

// Pressure plate: sets m_SetChannel while the player stands on it, optionally pulsing on/off by timer.
s8 XItemHandler_Touchplate::DoUpdate()
{
    if (m_KillChannel && FlagChannels[m_KillChannel]) {
        m_pItem->ItemFlags().Set(0x10);
        return 1;
    }
    if (!m_RequireChannel || FlagChannels[m_RequireChannel]) {
        if (m_OnTime && m_OffTime) {
            float t = m_Timer;
            if (t < 0.0f) {
                t -= XApp::GameFrameRateMul();
                m_Timer = t;
                if (0.0f - t >= (float)m_OffTime) m_Timer = 0.0f;
                return 1;
            }
            t += XApp::GameFrameRateMul();
            m_Timer = t;
            if ((float)m_OnTime <= t) m_Timer = -1.0f;
        }
        if (Within(gpPlayerItem[0]->Position(), *(EXGeoTriggerDatum *)&m_Datum)) FlagChannels[m_SetChannel] = 1;
    }
    return 1;
}

// Laser beam between two datums: sets the channel and hurts (or, with option 1, shoots back) when the player breaks it.
s8 XItemHandler_BeamTrigger::DoUpdate()
{
    if (m_Data[2] && FlagChannels[m_Data[2]]) {
        m_pItem->ItemFlags().Set(0x10);
        return 1;
    }
    if (!m_Data[1] || FlagChannels[m_Data[1]]) {
        EXDatumTemp d;
        m_pItem->GetAnimatorDatum(0x100000c8, d, 0, 0);
        EXVector start = d.m_position;
        m_pItem->GetAnimatorDatum(0x100000c9, d, 0, 0);
        EXVector end = d.m_position;
        EXVector dir(end.x - start.x, end.y - start.y, end.z - start.z, end.w - start.w);
        EXLineSeg line(start, dir);
        EXVector hit;
        float t;
        if (XItemHandler_Player::GetInstance()->CheckCollision(line, hit, t)) {
            FlagChannels[m_Data[0]] = 1;
            float amount = *(float *)&m_Data[3];
            if (amount != 0.0f) {
                if (m_Options & 1) {
                    XItemHandler_Player::GetInstance()->SendPredMsg(7, hit, amount);
                    return 1;
                }
                XItemHandler_Player::GetInstance()->SendPredMsg(0, amount);
            }
        }
    }
    return 1;
}

// Ammo box: while the player stands in it, tops up one weapon's ammo and prints how much was taken.
s8 XItemHandler_AmmoPickup::DoUpdate()
{
    if (m_Data[2]) {
        if (!FlagChannels[m_Data[2]]) return 1;
        m_pItem->SetHasRender();
        m_Data[2] = 0;
    }
    if (!Within(gpPlayerItem[0]->Position(), *(EXGeoTriggerDatum *)&m_Datum)) {
        m_Unk8 = 0;
        return 1;
    }

    int weapon;
    unsigned int msgPickup;
    unsigned int msgFull;
    s8 noHud;
    int defAmount;
    switch (m_Data[0]) {
    case 0:     // speargun
        weapon = 5;
        msgPickup = 0x51000529;
        msgFull = 0x51000569;
        noHud = 1;
        defAmount = weapon;
        break;
    case 1:     // healing charge
        weapon = 12;
        msgPickup = 0x5100052a;
        msgFull = 0x5100056a;
        noHud = 0;
        defAmount = 1;
        break;
    case 2:     // pulse mine
        weapon = 8;
        msgPickup = 0x5100052d;
        msgFull = 0x5100056d;
        noHud = 0;
        defAmount = 4;
        break;
    case 3:     // sonic trap
        weapon = 9;
        msgPickup = 0x5100052e;
        msgFull = 0x5100056e;
        noHud = 0;
        defAmount = 4;
        break;
    case 5:     // plasma mine
        weapon = 10;
        msgPickup = 0x5100052b;
        msgFull = 0x5100056b;
        noHud = 0;
        defAmount = 4;
        break;
    case 6:     // fire bomb
        weapon = 11;
        msgPickup = 0x5100052c;
        msgFull = 0x5100056c;
        noHud = 0;
        defAmount = 4;
        break;
    default:
        return 1;
    }
    if (!m_Data[1]) m_Data[1] = defAmount;

    XItemHandler_Player *pPlayer = XItemHandler_Player::GetInstance();
    short *pAmmo = &pPlayer->m_Ammo[weapon];
    if (*pAmmo >= pPlayer->GetMaxAmmo(weapon)) {
        if (!m_Unk8) Text_PrintEnvironment(Text_Get(msgFull), ex::RGBA(0xffffffff), 3, 9, 0, 0);
        m_Unk8 = 1;
        return 1;
    }
    if (!noHud && *pAmmo == 0) FEHud::Instance(1)->WeaponArrayAdd(weapon, (FEWeaponCat::Flags)4);

    int amount = m_Data[1];
    if ((int)m_Data[1] < 0) amount = 9999;
    int room = pPlayer->GetMaxAmmo(weapon) - pPlayer->m_Ammo[weapon];
    int take = amount > room ? room : amount;
    char plural = take > 1 ? 's' : 0;

    EXString text = gText.Get(msgPickup);
    EXString msg;
    msg.Format(text, take, plural);
    Text_PrintEnvironment(msg, ex::RGBA(0xffffffff), 3, 9, 0, 0);
    EXBaseSoundManager::GetSoundManager()->StartSound(0x1af0054b);
    pPlayer->AddAmmo(weapon, take);
    if ((int)m_Data[1] > 0) m_pItem->ItemFlags().Set(0x10);
    m_Unk8 = 1;
    return 1;
}

static void Upgrade_Print(unsigned int id)
{
    Text_PrintEnvironment(Text_Get(id), ex::RGBA(0xffffffff), 2, -1, 0, 0);
}

// Upgrade pickup: gives any mix of health/energy/armour/ammo upgrades, a ritual challenge, a costume and a weapon
// upgrade in one go. m_Data: 0 show channel, 1-4 health/energy/armour/bandolier upgrades, 5 ritual, 6 costume,
// 8 weapon, 9 reward index (already owned: the box vanishes without paying out again).
s8 XItemHandler_Upgrades::DoUpdate()
{
    if (m_bActive) {
        if (m_Data[9]) {
            GameValues &gv = GameData::Instance(1)->m_Values;
            u32 idx = m_Data[9];
            u32 got = idx > 100 ? gv.m_WeaponUpgrades & (1 << (idx - 100)) : gv.m_Rewards & (1 << idx);
            if (got) {
                if (!m_Data[8] || FEHud::Instance(1)->IsWeaponAvailable(m_Data[8])) {
                    m_pItem->ItemFlags().Set(0x10);
                    return 1;
                }
            }
        }
        m_bActive = 0;
    }
    if (m_Data[0] && !FlagChannels[m_Data[0]]) return 1;

    if (IsValid(*(EXGeoTriggerDatum *)&m_Datum)) {
        if (!Within(gpPlayerItem[0]->Position(), *(EXGeoTriggerDatum *)&m_Datum)) return 1;
    }

    s8 ok = 1;
    s8 awarded = 0;
    if (m_Data[9]) awarded = GameData::Instance(1)->m_Values.AwardReward(m_Data[9]);

    XItemHandler_Player *pPlayer = XItemHandler_Player::GetInstance();
    for (int i = 0; i < 4; i++) {
        if (m_Data[1 + i]) {
            if (!awarded) {
                pPlayer->AddSpecialUpgrade(i, m_Data[1 + i]);
                Upgrade_Print(0x51000336 + i);
            } else {
                ok = 0;
            }
        }
    }

    if (m_Data[5]) {
        GameValues &gv = GameData::Instance(1)->m_Values;
        if (!((1 << m_Data[5]) & (int)gv.m_Rituals)) {
            gv.m_Rituals |= (short)(1 << m_Data[5]);
            static const unsigned int ritualText[12] = {0x51000506, 0x51000507, 0x51000508, 0x51000509,
                                                        0x5100050a, 0x5100050b, 0x5100050c, 0x5100050d,
                                                        0x5100050e, 0x5100050f, 0x51000510, 0x51000511};
            Upgrade_Print(m_Data[5] >= 1 && m_Data[5] <= 12 ? ritualText[m_Data[5] - 1] : 0x51000311);
        } else {
            ok = 0;
        }
    }

    if (m_Data[6]) {
        if (!(pPlayer->m_SpecialUpgrades[4] & (1 << m_Data[6]))) {
            pPlayer->SetSpecialUpgrade(4, 1 << m_Data[6]);
            unsigned int id;
            switch (m_Data[6]) {
            case 1: id = 0x51000501; break;
            case 2: id = 0x51000502; break;
            case 3: id = 0x51000503; break;
            case 4: id = 0x51000504; break;
            default: id = 0x51000315; break;
            }
            Upgrade_Print(id);
        } else {
            ok = 0;
        }
    }

    if (m_Data[8]) {
        if (!FEHud::Instance(1)->IsWeaponAvailable(m_Data[8]))
            FEHud::Instance(1)->WeaponArrayAdd(m_Data[8], (FEWeaponCat::Flags)4);
        if (!awarded) {
            GameData::Instance(1)->m_Values.UpgradeWeapon(m_Data[9]);
            pPlayer->InGameWeaponUpgradeAdd(m_Data[8]);
            unsigned int id;
            switch (m_Data[8]) {
            case 1: id = 0x510004fa; break;     // wristblades
            case 2: id = 0x510004fc; break;     // combistick
            case 3: id = 0x510004fd; break;     // maul
            case 4: id = 0x510004fe; break;     // glaive
            case 5: id = 0x510004ff; break;     // speargun
            case 6: id = 0x510004fb; break;     // smart disc
            case 7: id = 0x51000500; break;     // plasmacaster
            default: id = 0x51000310; break;
            }
            Upgrade_Print(id);
        } else {
            ok = 0;
        }
    }

    m_pItem->ItemFlags().Set(0x10);
    if (ok) EXBaseSoundManager::GetSoundManager()->StartSound(0x1af0054b);
    return 1;
}

// Grants (or removes) one of the trigger-controlled abilities: predator messages for the player's own abilities,
// HUD weapon-array entries for the gadgets, vision mode flags and a few HUD switches.
void XItemHandler_Environment::AbilityEnable(unsigned int ability, signed char enable)
{
    XItemHandler_Player *pPlayer = XItemHandler_Player::GetInstance();
    unsigned int weapon;
    switch (ability) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 0x1c:
        pPlayer->SendPredMsg(enable ? 3 : 4, m_Ability);
        return;
    case 6: weapon = 1; break;
    case 7: weapon = 5; break;
    case 9: weapon = 2; break;
    case 0xc: weapon = 6; break;
    case 0xd: weapon = 7; break;
    case 0xf: weapon = 0x3f; break;
    case 0x16:
    case 0x1d: weapon = 4; break;
    case 0x17: weapon = 10; break;
    case 0x18: weapon = 0xb; break;
    case 0x19: weapon = 8; break;
    case 0x1a: weapon = 9; break;
    case 0x1b: weapon = 0xc; break;
    case 0x1e: weapon = 3; break;
    case 0xe:   // full vision kit
        if (enable) {
            FEHud::Instance(1)->WeaponArrayAdd(0x3f, (FEWeaponCat::Flags)4);
            pPlayer->m_VisionModeFlags = 0xffffffff;
            pPlayer->SetHelmet(1);
            FEHud::Instance(1)->m_Unk2194 = 1;
        } else {
            FEHud::Instance(1)->WeaponArrayDel(0x3f, (FEWeaponCat::Flags)4);
            pPlayer->m_VisionModeFlags = 0;
            FEHud::Instance(1)->m_Unk2194 = 0;
        }
        return;
    case 0x10:
        FEHud::Instance(1)->m_Unkd7d = enable ? 1 : 0;
        return;
    case 0x11:
        pPlayer->m_VisionModeFlags = enable ? 0xffffffff : 0;
        return;
    case 0x12:
        if (enable) pPlayer->m_VisionModeFlags |= 1;
        else pPlayer->m_VisionModeFlags &= ~1u;
        return;
    case 0x13:
        if (enable) pPlayer->m_VisionModeFlags |= 4;
        else pPlayer->m_VisionModeFlags &= ~4u;
        return;
    case 0x14:
        if (enable) pPlayer->m_VisionModeFlags |= 8;
        else pPlayer->m_VisionModeFlags &= ~8u;
        return;
    case 0x15:
        if (enable) pPlayer->m_VisionModeFlags |= 2;
        else pPlayer->m_VisionModeFlags &= ~2u;
        return;
    default:
        return;
    }
    if (enable)
        FEHud::Instance(1)->WeaponArrayAdd(weapon, (FEWeaponCat::Flags)4);
    else
        FEHud::Instance(1)->WeaponArrayDel(weapon, (FEWeaponCat::Flags)4);
}

extern EXVector Mission_RestartPosition;
extern EXVector Mission_RestartRotation;
extern s8 Raining;

// Environment trigger volume: while the player (in a mode the trigger accepts) is inside it, performs its type's effect.
s8 XItemHandler_Environment::DoUpdate()
{
    int state = theApp.GameStateCurrent()->m_State;
    if (state != 9 && state != 0x10) return 1;

    if (m_Data3 && FlagChannels[m_Data3]) m_pItem->ItemFlags().Set(0x10);
    if (m_Unk58) {
        if (!FlagChannels[m_Unk58]) return 1;
        m_pItem->SetHasRender();
        m_Unk58 = 0;
    }
    if (m_Unk85) {
        m_Unk5c -= EXBaseApp::FrameLength();
        if (m_Unk5c <= 0.0f) m_pItem->ItemFlags().Set(0x10);
    }

    // m_Data11[3]: which player modes the trigger reacts to.
    XItemHandler_Player *pPlayer = XItemHandler_Player::GetInstance();
    bool active = false;
    switch (m_Data11[3]) {
    case 1:
        active = pPlayer->GetMode() == 0xc && pPlayer->GetSubMode() != 8 && pPlayer->GetSubMode() != 9;
        break;
    case 2:
        if (pPlayer->GetMode() == 0xc) active = pPlayer->GetSubMode() == 8 || pPlayer->GetSubMode() == 9;
        else if (pPlayer->GetMode() == 4) active = pPlayer->GetSubMode() == 5 || pPlayer->GetSubMode() == 6;
        break;
    case 3: active = pPlayer->GetMode() == 0x12; break;
    case 4: active = pPlayer->GetMode() == 0x11; break;
    case 5: active = pPlayer->GetMode() == 0x18; break;
    default: active = true; break;
    }
    EXVector playerPos = gpPlayerItem[0]->Position();
    playerPos.y += 1.0f;
    if (!active) {
        m_Unk40 = 0;
        return 1;
    }

    EXGeoBaseDatum datum(m_Datum);
    if (!Within(playerPos, *(EXGeoTriggerDatum *)&datum) && !m_bCollided && !m_bTeleportPending) {
        m_Unk40 = 0;
        return 1;
    }
    if (m_Options & 8) m_pItem->ItemFlags().Set(0x10);

    // Types other than 11, 12, 16 and 19 (which print their own text) show the hint text.
    switch (m_Type) {
    case 11:
    case 12:
    case 16:
    case 19:
        break;
    default:
        if (m_Data1 > 0x51000000 && m_Data1 < 0x510009b9) {
            int secs = (int)m_Unk5c;
            if (!secs) secs = 5;
            if (m_Type == 9) {
                Text_PrintEnvironment(Text_Get(m_Data1), ex::RGBA(0x7f7f7f7f), secs, (unsigned int)this & 0xffffff, 0, 0);
            } else {
                Text_PrintEnvironment(Text_Get(m_Data1), ex::RGBA(0x7f7f7f7f), secs, -1, 0, 0);
                m_Data1 = 0;
            }
        }
        break;
    }
    m_bCollided = 0;

    switch (m_Type) {
    case 0:
    case 2:
        pPlayer->m_Wet = 1;
        if (pPlayer->GetCloak()) {
            pPlayer->BreakCloak(m_Data2);
            m_Unk40 = 1;
            return 1;
        }
        break;
    case 1:
        if (pPlayer->GetCloak()) {
            pPlayer->BreakCloak(0);
            m_Unk40 = 1;
            return 1;
        }
        break;
    case 3:
        if (m_Unk5c > 0.0f) {
            m_Unk85 = 1;
            m_Unk40 = 1;
            return 1;
        }
        m_pItem->ItemFlags().Set(0x10);
        m_Unk40 = 1;
        return 1;
    case 4:
    case 6:
        if (m_Type == 4) XApp::SetGlobalGameFrameRate(0.25f);
        else XGetCamera(0)->m_bGoFirstPerson = 1;
        if (m_Unk5c > 0.0f) {
            m_Unk85 = 1;
            m_Unk40 = 1;
            return 1;
        }
        break;
    case 5: {
        pPlayer->SendPredMsg(1, (float)(int)m_Unk60 * XApp::FrameLength());
        if (pPlayer->GetMode() == 0x18) {
            EXVector gecko = pPlayer->GetGeckoPoint();
            pPlayer->SendPredMsg(5, gecko, 0.75f);
        }
        if (pPlayer->GetHealth() <= 0.0f && (m_Options & 0x10)) {
            theApp.ApplyFadeOutIn(6.0f, ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 0, 0);
            m_pItem->ItemFlags().Set(0x10);
            m_Unk40 = 1;
            return 1;
        }
        break;
    }
    case 7:
        AbilityEnable(m_Ability, (signed char)(u8)m_Unk68);
        m_pItem->ItemFlags().Set(0x10);
        m_Unk40 = 1;
        return 1;
    case 8:
        if (XGetCamera(0)->m_Mode == 0) {
            pPlayer->m_bForceTrackCam = 1;
            if (m_Options & 4) {
                XGetCamera(0)->m_Mode = 7;
            } else if (m_Data11[0] > 0) {
                XGetCamera(0)->m_Mode = 9;
                XGetCamera(0)->m_BoundingRadius = m_Data11[0];
            } else {
                XGetCamera(0)->m_Mode = 8;
            }
            m_Unk40 = 1;
            return 1;
        }
        break;
    case 9:
    case 13:
        break;
    case 10:
        TimeTable_Set(m_Unk6c);
        m_Unk40 = 1;
        return 1;
    case 11:
        if (m_Data1 == 0x510005bb) theApp.m_bDialogLeftAlign = 1;
        theApp.CreateDialogOk(Text_Get(m_Data1), m_Data11[1]);
        FEObjectives::Instance(1)->Add(Text_MatchRev(m_Data1), Text_Match(m_Data1), 0x20);
        m_pItem->ItemFlags().Set(0x10);
        m_Unk40 = 1;
        return 1;
    case 12:
        theApp.CreateDialogYesNo(Text_Get(m_Data1), m_Data11[1], m_Data11[2]);
        m_pItem->ItemFlags().Set(0x10);
        m_Unk40 = 1;
        return 1;
    case 15:
        if (XGetCamera(0)->m_Mode == 0) {
            XGetCamera(0)->m_Mode = 0xb;
            m_Unk40 = 1;
            return 1;
        }
        break;
    case 16:
        FEObjectives::Instance(1)->Add(m_Data1, Text_Match(m_Data1), 0x20);
        m_pItem->ItemFlags().Set(0x10);
        m_Unk40 = 1;
        return 1;
    case 17:
        if (!m_Unk40) {
            const EXVector &r = m_pItem->Rotation();
            gpPlayerItem[0]->SetRotation(EXQuaternion(r.x, r.y, r.z, r.w));
            m_Unk40 = 1;
            return 1;
        }
        break;
    case 14:
    case 18:
        if (m_Type == 18) {
            if (Tethys && !m_bTeleportPending) break;
            if (pPlayer->GetCloak()) pPlayer->BreakCloak(0);
            if (!m_bTeleportPending) {
                EXVector splash(gpPlayerItem[0]->Position().x, m_Datum.m_Pos.y + m_Datum.m_08.y,
                                gpPlayerItem[0]->Position().z, gpPlayerItem[0]->Position().w);
                XItemHandler_ScriptBase::FX_Create(splash, 0x400050a, 0x40, 0x1000016);  // HT_Script_FX_Sea_Splash
                Tethys = 1;
            }
            if (!theApp.m_bCutscene) XGetCamera(0)->m_Mode = 0xc;
        }
        // Fade out and in, then move the player to the linked mission item.
        if (!m_bTeleportPending && !(m_Options & 8) && !theApp.m_bCutscene) {
            ex::RGBA col(0xff, 0xff, 0xff, 0xff);
            if (m_Options & 0x10) col = ex::RGBA(0, 0, 0, 0xff);
            theApp.ApplyFadeOutIn(4.0f, col, 1, 0);
            m_bTeleportPending = 1;
            Tethys = 1;
        }
        {
            s8 halfway = theApp.m_bHalfwayFade;
            theApp.m_bHalfwayFade = 0;
            if (!halfway && !(m_Options & 8) && !theApp.m_bCutscene) break;
        }
        {
            XItem *pDest = Mission_GetMissionItem(m_Link);
            if (pDest) {
                EXVector destPos = pDest->Position();
                pPlayer->TeleportPlayer(destPos, pDest->Rotation().y);
                if (!theApp.m_bCutscene) {
                    XGetCamera(0)->SetThirdPerson();
                    XGetCamera(0)->m_Mode = 5;
                }
            }
        }
        m_bTeleportPending = 0;
        Tethys = 0;
        RemoveAllPendingTeleports();
        m_Unk40 = 1;
        return 1;
    case 19:
        m_Unk5c -= EXBaseApp::FrameLength();
        if (m_Unk5c <= 0.0f) {
            m_Unk5c = m_Unk44;
            Text_PrintEnvironment(Text_Get(m_Data1), ex::RGBA(0x7f7f7f7f), 5, -1, 0, 0);
            m_Unk40 = 1;
            return 1;
        }
        break;
    case 20: {
        XItem *pItem = Mission_GetMissionItem(m_Link);
        if (pItem) {
            Mission_RestartPosition = pItem->Position();
            Mission_RestartRotation = pItem->Rotation();
            m_Unk40 = 1;
            return 1;
        }
        break;
    }
    case 21:
        Raining = 0;
        m_Unk40 = 1;
        return 1;
    case 22:
        pPlayer->m_Energy = 0.0f;
        break;
    }
    m_Unk40 = 1;
    return 1;
}
