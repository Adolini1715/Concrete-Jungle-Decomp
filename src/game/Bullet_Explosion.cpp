#include "XItemHandler_ScriptBase.h"
#include "XItemHandler_Player.h"
#include "XItem.h"
#include "Actor.h"
#include "FX_Gore.h"
#include "EXItemAnimator_Script.h"
#include "EXGeoFile.h"
#include "EXDatum.h"
#include "EXMemory.h"
#include "EXBaseGamePad.h"
#include "XGamePad.h"
#include "EXRayCastTemp.h"
#include "FEText.h"
#include "WeaponTable.h"
#include "Vehicle.h"
#include "ActorUtil.h"
#include "ScriptRunner.h"
#include "XItemHandler_AirVehicle.h"
#include "XItemHandler_CCTV.h"
#include "XItemHandler_Turret.h"
#include "XItemHandler_AntiCloakSmoke.h"

extern XItemList gHitableList;
void DetonateNearbyMines(EXVector pos, float radius);

// TODO stand-in: geometry script record (EngineX). TODO layout.
class EXGeoScript {
public:
    u32 m_Unk0;
    int m_NumFrames;    // 0x04 default effect life
};

XItemList gBloodTintList;

EX_IMPLEMENT_DYNCREATE(XItemHandler_ScriptBase, EXItemHandler)

XItemHandler_ScriptBase::XItemHandler_ScriptBase() {}

XItemHandler_ScriptBase::~XItemHandler_ScriptBase()
{
    if (m_Flags & 1) gBloodTintList.Remove((XItem *)m_pItem);
}

signed char XItemHandler_ScriptBase::Initialise()
{
    m_pItem->AddAnimator(EXItemAnimator::CreateFrom(m_pItem->GeoFile(), m_pItem->RefObject()));
    m_pAnim = 0;
    return 1;
}

void XItemHandler_ScriptBase::SetBloodTintColour(int mode)
{
    if (!(m_Flags & 1)) return;
    if (mode != 0) {
        if (mode > 0 && mode <= 3) {
            float r, g, b;
            Gore_GetVisionModeBloodColour(mode, r, g, b);
            Gore_TintToBloodColourRGB((XItem *)m_pItem, r, g, b);
        }
    } else {
        Gore_TintToBloodColourRGB((XItem *)m_pItem, m_R, m_G, m_B);
    }
}

void XItemHandler_ScriptBase::TintAllBloodToVisionMode(int mode)
{
    for (XItem *p = gBloodTintList.GetFirst(); p; p = gBloodTintList.GetNext(p))
        ((XItemHandler_ScriptBase *)p->ItemHandler())->SetBloodTintColour(mode);
}

// NEAR: ~49 bytes differ, position/rotation store order (retail stores rotation w,z,y,x)
signed char XItemHandler_ScriptBase::DoUpdate()
{
    if (m_Flags & 2) {
        switch (XItemHandler_Player::Instance()->m_Tint) {
        case 1:
            m_pItem->RenderInfo().RenderFlagsRef().BitSet(8);
            m_pItem->RenderInfo().RenderFlagsRef().BitSet(4);
            m_pItem->RenderInfo().SetRGBAMult(1.0f, 1.0f, 0.0f, 1.0f);
            break;
        default:
            m_pItem->RenderInfo().RenderFlagsRef().BitClr(8);
            m_pItem->RenderInfo().RenderFlagsRef().BitClr(4);
            break;
        }
    }
    EXDatumTemp d;
    void *pFound;
    if (m_Flags & 4) {
        if (!m_pAnim) goto kill;
        pFound = m_pAnim->GetDatum(m_DatumHash, d, 0, EXItemAnimator::m_pNullDatumPtr);
    } else {
        if (!m_ActorID) goto countdown;
        EXItemHandler *pOwner;
        if (m_ActorID > 0) {
            pOwner = Actor::GetPtrFromID(m_ActorID);
            if (!pOwner) goto countdown;
        } else {
            pOwner = XItemHandler_Player::Instance();
        }
        EXItem *pOwnerItem = pOwner->GetItem();
        if (!pOwnerItem) goto countdown;
        pFound = pOwnerItem->GetAnimatorDatum(m_DatumHash, d, 0, 0);
    }
    if (pFound) {
        m_pItem->SetPosition(d.m_position);
        EXVector rot;
        d.m_rotation.GetRotation(rot, 4);
        m_pItem->SetRotation(EXQuaternion(rot.x, rot.y, rot.z, rot.w));
    }
countdown:
    if (m_Life > 0) {
        if (--m_Life == 0) {
kill:
            m_pItem->ItemFlags().Set(0x10);
        }
    }
    return 1;
}


void XItemHandler_ScriptBase::DeleteIfCSMF()
{
    if (m_Flags & 4) {
        m_pAnim = 0;
        m_pItem->ItemFlags().Set(0x10);
    }
}

void Remove_CSMF()
{
    EXDListItem *pHead = XGetWorld()->m_pItemLists[0].m_pHead;
    for (EXItem *p = pHead ? (EXItem *)((char *)pHead - 4) : 0; p; p = (EXItem *)p->NextItem()) {
        EXItemHandler *h = p->ItemHandler();
        if (h && h->IsKindOf(&XItemHandler_ScriptBase::classXItemHandler_ScriptBase)) ((XItemHandler_ScriptBase *)h)->DeleteIfCSMF();
    }
}

XItem *XItemHandler_ScriptBase::FX_CreateLink(unsigned int script, int actor, unsigned int datum, int life, unsigned int file)
{
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(file, 0, 0);
    EXGeoScript *pScript = pFile->GeoHeader()->GeoScript(script);
    if (pScript) {
        XItem *pItem = XItem::CreateItem(pFile, 0x1000016, (EXRuntimeClass *)&classXItemHandler_ScriptBase, EXVector(0.0f, 0.0f, 0.0f, 0.0f));
        EXItemAnimator_Script *pAnim = new EXItemAnimator_Script;
        pAnim->Initialise(pFile, script);
        pItem->AddAnimator(pAnim);
        theItemEnv.AddItem(pItem, 0);
        ((XItemHandler_ScriptBase *)pItem->ItemHandler())->m_Life = life < 0 ? life : pScript->m_NumFrames;
        ((XItemHandler_ScriptBase *)pItem->ItemHandler())->m_ActorID = actor;
        ((XItemHandler_ScriptBase *)pItem->ItemHandler())->m_DatumHash = datum;
        ((XItemHandler_ScriptBase *)pItem->ItemHandler())->m_Flags = 0;
        return pItem;
    }
    return 0;
}

XItem *XItemHandler_ScriptBase::FX_CreateLinkBlood(unsigned int script, int actor, unsigned int datum, int group, int life)
{
    XItem *pItem = FX_CreateLink(script, actor, datum, life, 0x1000016);
    if (pItem) {
        XItemHandler_ScriptBase *h = (XItemHandler_ScriptBase *)pItem->ItemHandler();
        if (h) {
            h->m_Flags |= 1;
            gBloodTintList.Add(pItem);
            Gore_GetBloodColour(group, h->m_R, h->m_G, h->m_B);
            h->SetBloodTintColour(XItemHandler_Player::Instance()->m_Tint);
        }
    }
    return pItem;
}

XItem *XItemHandler_ScriptBase::FX_Create(EXVector &pos, unsigned int script, int life, unsigned int file)
{
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(file, 0, 0);
    EXGeoScript *pScript = pFile->GeoHeader()->GeoScript(script);
    if (pScript) {
        XItem *pItem = XItem::CreateItem(pFile, 0x1000016, (EXRuntimeClass *)&classXItemHandler_ScriptBase, EXVector(0.0f, 0.0f, 0.0f, 0.0f));
        pItem->SetPosition(pos);
        EXItemAnimator_Script *pAnim = new EXItemAnimator_Script;
        pAnim->Initialise(pFile, script);
        pItem->AddAnimator(pAnim);
        theItemEnv.AddItem(pItem, 0);
        ((XItemHandler_ScriptBase *)pItem->ItemHandler())->m_Life = pScript->m_NumFrames;
        ((XItemHandler_ScriptBase *)pItem->ItemHandler())->m_ActorID = 0;
        ((XItemHandler_ScriptBase *)pItem->ItemHandler())->m_Flags = 0;
        return pItem;
    }
    return 0;
}

XItem *XItemHandler_ScriptBase::FX_CreateBlood(EXVector &pos, unsigned int script, int group, int life)
{
    XItem *pItem = FX_Create(pos, script, life, 0x1000016);
    if (pItem) {
        XItemHandler_ScriptBase *h = (XItemHandler_ScriptBase *)pItem->ItemHandler();
        if (h) {
            h->m_Flags |= 1;
            gBloodTintList.Add(pItem);
            Gore_GetBloodColour(group, h->m_R, h->m_G, h->m_B);
            h->SetBloodTintColour(XItemHandler_Player::Instance()->m_Tint);
        }
    }
    return pItem;
}
XItem *XItemHandler_ScriptBase::FX_CreateCSMF(unsigned int script, EXItemAnimator *pAnim, unsigned int datum, int life, unsigned int file)
{
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(file, 0, 0);
    EXGeoScript *pScript = pFile->GeoHeader()->GeoScript(script);
    if (!pScript)
        return 0;
    XItem *pItem = XItem::CreateItem(pFile, 0x1000016, (EXRuntimeClass *)&classXItemHandler_ScriptBase, EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    EXItemAnimator_Script *pScr = new EXItemAnimator_Script;
    pScr->Initialise(pFile, script);
    pItem->AddAnimator(pScr);
    u8 *raw = (u8 *)pScr;
    raw[0x7c] |= 3;
    float r = g_EXRandClass.Randf() * 2.0f + 1.0f;
    *(float *)(raw + 0xe8) = r;
    *(float *)(raw + 0xec) = r;
    *(float *)(raw + 0xf0) = r;
    *(float *)(raw + 0xf4) = 0.0f;
    raw[0x7e] = 0;
    theItemEnv.AddItem(pItem, 0);
    XItemHandler_ScriptBase *h = (XItemHandler_ScriptBase *)pItem->ItemHandler();
    h->m_Life = life < 0 ? life : pScript->m_NumFrames;
    h->m_pAnim = pAnim;
    h->m_DatumHash = datum;
    h->m_ActorID = 0;
    h->m_Flags = 4;
    return pItem;
}

// Area damage helper: full damage inside half the radius, then falling to zero at the edge (doubled slope).
static __forceinline float Explosion_Falloff(float dist, float radius, float damage)
{
    if (dist > radius * 0.5f)
        return ((radius - dist) * damage * 2.0f) / radius;
    return damage;
}

static __forceinline float Explosion_Dist(const EXVector &a, const EXVector &b)
{
    EXVector d = a - b;
    float sq = d.LengthXYZSquared();
    return sq < 0.0f ? 0.0f : EXSqrt(sq);
}

// Detonates an explosion at pos: spawns the effect script, makes noise, then damages everything within
// radius. a = script, b = radius, c = damage, d = predator weapon (0 = ordinary weapon, 0x1e = no
// follow-up), e = weapon hash, f = player-fired, g = owner ref id (not hurt), h = skip line of sight.
XItem *XItemHandler_ScriptBase::Explosion_Create(EXVector &pos, unsigned int a, int b, int c, int d, unsigned int e, signed char f, int g, signed char h)
{
    XItem *pResult = 0;
    if (e == 0x49000050 || e == 0x49000062) {
        CreateAntiCloakSmoke(pos);
    } else if (a > 0x4000000 && a < 0x40007d9) {
        EXGeoFile *pFile = EXGeoFile::LoadGeoFile(a == 0x40004e1 ? 0x1000026 : 0x1000016, 0, 0);
        EXGeoScript *pScript = pFile->GeoHeader()->GeoScript(a);
        if (pScript) {
            pResult = XItem::CreateItem(pFile, 0x1000016, (EXRuntimeClass *)&classXItemHandler_ScriptBase, EXVector(0.0f, 0.0f, 0.0f, 0.0f));
            pResult->SetPosition(pos);
            EXItemAnimator_Script *pAnim = new EXItemAnimator_Script;
            pAnim->Initialise(pFile, a);
            pResult->AddAnimator(pAnim);
            theItemEnv.AddItem(pResult, 0);
            XItemHandler_ScriptBase *hs = (XItemHandler_ScriptBase *)pResult->ItemHandler();
            hs->m_Life = pScript->m_NumFrames;
            hs->m_ActorID = 0;
            hs->m_Flags = 2;
        }
    }

    float radius = (float)b;
    SendMsg_LoudNoise(pos, radius * 3.0f, 0, 1, d == 0x1e);

    for (Actor *pActor = Actor::GetFirst(pos, radius, 0); pActor; pActor = pActor->GetNext(pos, radius, 0)) {
        if (pActor->m_RefID == g) continue;
        if (pActor->m_BlockCounter > 0.0f) continue;
        if (e == 0x49000050 && AP_GetPtrFromIndex(pActor->m_ProfileIndex)->m_Hash == 0x47000128) continue;
        if (!h) {
            EXVector org(pos.x, pos.y + 0.1f, pos.z, pos.w);
            if (!CheckLineOfSight(org, pActor->GetFeetPos())) {
                org.y += 0.4f;
                if (!CheckLineOfSight(org, pActor->GetPelvisPos())) {
                    org.y += 0.7f;
                    if (!CheckLineOfSight(org, pActor->GetHeadPos())) continue;
                }
            }
        }
        EXVector diff = pActor->GetPelvisPos() - pos;
        float distSq = diff.LengthXYZSquared();
        if (distSq > (float)(b * b)) continue;
        float dist = distSq < 0.0f ? 0.0f : EXSqrt(distSq);
        float dmg = (float)c;
        if (dist > radius * 0.5f)
            dmg = ((radius - dist) / radius) * dmg * 2.0f;
        EXVector dir(diff.x, diff.y + 1.0f, diff.z, diff.w);
        if (!pActor->m_StateFlags.m_bRagDoll) {
            if (d == 0)
                SendMsg_AttackedByActor(pActor, WeaponProfile::GetWeaponData(e), pos, dir, dmg, pActor, 0);
            else if (d != 0xe)
                SendMsg_AttackedByPredator(pActor, d, pos, dir, dmg, 0);
        } else if (dist < radius * 0.75f) {
            dmg = ((radius - dist) / radius) * (float)c;
            pActor->ApplyRagDollForce(pos, dir, dmg, -1);
        }
    }

    if (d == 0x1e)
        return pResult;

    WeaponProfile *pWeapon = WeaponProfile::GetWeaponData(e);
    unsigned int dmgClass = pWeapon ? (unsigned int)pWeapon->m_DamageClass : 0;

    for (Vehicle *pVeh = Vehicle::GetFirst(pos, radius); pVeh; pVeh = pVeh->GetNext(pos, radius)) {
        float dist = Explosion_Dist(EXVector(pVeh->getCurrPos().m_pos.x, pVeh->getCurrPos().m_pos.y, pVeh->getCurrPos().m_pos.z, 0.0f), pos);
        float dmg = (float)c;
        if (dist > radius * 0.5f)
            dmg = (radius - dist / radius) * dmg * 2.0f;
        pVeh->ApplyAttackDamage(d ? d : e, dmg);
    }

    for (XItem *pAir = gAirVehicleList.GetFirst(); pAir; pAir = gAirVehicleList.GetNext(pAir)) {
        EXVector diff = pAir->Position() - pos;
        float distSq = diff.LengthXYZSquared();
        if (distSq < (float)(b * b)) {
            float dist = distSq < 0.0f ? 0.0f : EXSqrt(distSq);
            float dmg = (float)c;
            if (dist > radius * 0.5f)
                dmg = ((radius - dist) * dmg * 2.0f) / radius;
            if (dmgClass == 0x53000006)
                dmg = 1000000.0f;
            ((XItemHandler_AirVehicle *)pAir->ItemHandler())->ApplyDamage(dmg);
        }
    }

    XItemHandler_Player *pPlayer = XItemHandler_Player::Instance();
    float distPlayer = Explosion_Dist(gpPlayerItem[0]->Position(), pos);
    float best = distPlayer;
    float distChest = Explosion_Dist(pPlayer->GetChestPos(), pos);
    if (distChest < best) best = distChest;
    float distHead = Explosion_Dist(pPlayer->GetTopHeadPos(), pos);
    if (distHead < best) best = distHead;
    if (best < radius) {
        bool bHit = true;
        if (!h && !CheckLineOfSight(gpPlayerItem[0]->Position(), pos)
            && !CheckLineOfSight(pPlayer->GetChestPos(), pos)
            && !CheckLineOfSight(pPlayer->GetTopHeadPos(), pos))
            bHit = false;
        if (bHit) {
            if (pWeapon && pWeapon->m_DamageClass == 0x53000006) {
                pPlayer->EMPPlayer();
            } else {
                float dmg = (float)c;
                if (best > radius * 0.5f)
                    dmg = ((radius - best) * dmg * 2.0f) / radius;
                if (e == 0x49000009)
                    dmg *= 0.33333334f;
                if (!f) {
                    pPlayer->SendPredMsg(0, dmg);
                } else if (radius * 0.5f <= best) {
                    pPlayer->SendPredMsg(7, pos, dmg);
                } else {
                    pPlayer->SendPredMsg(0, dmg);
                    pPlayer->SendPredMsg(5, pos, 0.75f);
                }
            }
        }
    }

    if (dmgClass == 0) {
        dmgClass = 0x53000000;
    } else if (dmgClass == 0x53000006) {
        for (XItem *pT = gTurretList.GetFirst(); pT; pT = gTurretList.GetNext(pT)) {
            if ((pT->Position() - pos).LengthXYZSquared() < (float)(b * b))
                ((XItemHandler_Turret *)pT->ItemHandler())->m_State = 3;
        }
        for (XItem *pC = gCCTVList.GetFirst(); pC; pC = gCCTVList.GetNext(pC)) {
            if ((pC->Position() - pos).LengthXYZSquared() < (float)(b * b))
                ((XItemHandler_CCTV *)pC->ItemHandler())->m_State = 3;
        }
        c = 100;
    }

    for (XItem *pH = gHitableList.GetFirst(); pH; pH = gHitableList.GetNext(pH)) {
        EXVector diff = pH->Position() - pos;
        float distSq = diff.LengthXYZSquared();
        if (distSq < (float)(b * b)) {
            float dist = distSq < 0.0f ? 0.0f : EXSqrt(distSq);
            float dmg = Explosion_Falloff(dist, radius, (float)c);
            ((XItemHandler_ScriptRunner *)pH->ItemHandler())->ApplyDamage(dmg, dmgClass);
        }
    }

    DetonateNearbyMines(pos, radius);
    Gore_BloodPool(pos, 8);
    return pResult;
}

// Script event as the animator passes it to DoScriptCmdEvent (only the fields used here).
struct GEOSCRIPT_EVENT {
    u32 m_Unk0[3];
    unsigned int m_Hash;    // 0x0c event hash (HT_ScriptEvents_*), -1 at the end of the script
    u32 m_Unk10;
    union Param {
        unsigned int u;
        float f;
        signed char b;
    } m_Param[4];           // 0x14
};

// Script events for scripted FX items: gamepad rumble (flat and positional), cloak (refraction
// render) and, at the end of the item's own script, the item is flagged to be removed.
unsigned int XItemHandler_ScriptBase::DoScriptCmdEvent(GEOSCRIPT_EVENT *pEvent, EXItemAnimator *pAnim)
{
    switch (pEvent->m_Hash) {
    case 0x16000060: // HT_ScriptEvents_SR_Cloak
        m_pItem->RenderInfo().EnableRefractionRender(pEvent->m_Param[0].b);
        return 0;
    case 0x1600005e: // HT_ScriptEvents_Rumble
        ((XGamePad *)EXBaseGamePad::GetGamePad())->PlayRumble(pEvent->m_Param[0].u, pEvent->m_Param[1].f, 0xff);
        return 0;
    case 0x1600005f: { // HT_ScriptEvents_Rumble3D
        EXVector pos = pAnim->GetOwnerItem()->m_position;
        ((XGamePad *)EXBaseGamePad::GetGamePad())->PlayRumble3D(pEvent->m_Param[0].u, pos, pEvent->m_Param[2].f, pEvent->m_Param[3].f,
                                                               pEvent->m_Param[1].f, 0xff);
        return 0;
    }
    case 0xffffffff:
        if (pAnim == m_pItem->GetAnimator()) m_pItem->ItemFlags().Set(0x10);
        return 0;
    }
    return 0;
}

// Clear line between two points? Casts against the map, then against contoured items.
signed char CheckLineOfSight(const EXVector &a, const EXVector &b)
{
    EXRayCastTemp r;
    r.m_face.m_pEntity = 0;
    r.Set(a, b - a, 0, 0, 0, 0);
    r.m_mask = 8;
    theItemEnv.DoRayCastToMap(r, 0);
    if (!r.m_bHit) return 1;
    r.Set(a, b - a, 0, 0, 0, 0);
    r.m_mask = 8;
    theItemEnv.DoRayCastToContoured(r);
    return !r.m_bHit;
}
