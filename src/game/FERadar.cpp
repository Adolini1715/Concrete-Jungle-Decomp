#include "FEText.h"
#include "EXMemory.h"
#include "EXStdFuncs.h"
#include "FERadar.h"
#include "FELayout.h"
#include "XItemHandler_RadarBox.h"
#include "XItemHandler_Player.h"
#include "XItem.h"
#include "XItemHandler_MissionTrigger.h"
#include "XItemHandler_Objective.h"
#include "FELevels.h"
#include "XItemHandler_Hint.h"
#include "Recharge.h"
#include "ScriptRunner.h"
#include "XItemHandler_CCTV.h"
#include "XItemHandler_Camera.h"
#include "EXMaths.h"
#include <math.h>

extern unsigned int gCurrentMap_GeoHash;
float fMapVolume;
FERadar *FERadar::m_pInstance;
RadarMap CurrentMap;

// NEAR: only the inlined FERadarInfo ctor's first int-zero scheduling differs (see FEHud.h)
FERadar::FERadar()
{
    memset(&CurrentMap, 0, 0x48);
    fMapVolume = 0.0f;
    StateSub(0x10000000);
    SetObject(new FELayoutLayered(1, 0, 0, 0, 0, (EXW::TextAlign)4));
    m_Unk428 = 1.0f;
    m_Unk42c = 0.0f;
    m_Unk430 = 0.0f;
}

FERadar *FERadar::Instance(signed char create)
{
    if (!m_pInstance && create) m_pInstance = new FERadar;
    return m_pInstance;
}

s8 XItemHandler_RadarBox::Initialise() { return EXItemHandler::Initialise() != 0; }

XItemHandler_RadarBox *XItemHandler_RadarBox::CreateObject() { return new XItemHandler_RadarBox; }

FERadarItem::FERadarItem()
{
    m_Colour = ex::RGBA(0x7f, 0x7f, 0x7f, 0x80);
    m_Unk11 = 0;
    m_Pos = EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    m_Unk10 = 1;
    m_Sprite = 0x60000a7;
    m_Unk1c = 0x10;
    m_Unk20 = 0x10;
    m_Unk24 = 0;
    EXString t = gText.m_Empty;
    m_Text = t;
}

void FERadar::DrawPredator(FERadarInfo &info)
{
    FERadarItem item;
    if (XItemHandler_Player::Instance()->m_Tint == 3)
        item.m_Colour = ex::RGBA(0, 0, 0x7f, 0x7f);
    else
        item.m_Colour = ex::RGBA(0, 0x7f, 0, 0x7f);
    item.m_Unk11 = 1;
    item.m_Sprite = 0x60000a7;
    item.m_Unk24 = 0;
    item.m_Pos = XItemHandler_Player::Instance()->GetItem()->Position();
    DrawItemOnRadar(item, info, (FERenderPass::Flags)3);
}

void FERadar::DrawHints(FERadarInfo &info)
{
    FERadarItem item;
    if (XItemHandler_Player::Instance()->m_Tint != 0) return;
    for (XItem *p = gHintList.GetFirst(); p; p = gHintList.GetNext(p)) {
        if (!(signed char)((XItemHandler_Hint *)p->ItemHandler())->m_Unk68) {
            item.m_Unk24 = 0;
            item.m_Unk10 = 0;
            item.m_Colour = ex::RGBA(0, 0x7f, 0, 0x80);
            item.m_Unk1c = 0x10;
            item.m_Unk20 = 0x10;
            item.m_Sprite = 0x600028f;
            if (p->m_itemFlags & 1) {
                item.m_Pos = p->Position();
                DrawItemOnRadar(item, info, (FERenderPass::Flags)2);
            }
        }
    }
}

RadarMap::RadarMap(unsigned int hash, EXGeoTriggerDatum d)
{
    m_Valid = 1;
    m_Hash = hash;
    m_Datum = d;
    m_W = (int)(m_Datum.m_Pos.x - m_Datum.m_08.x);
    m_H = (int)(m_Datum.m_Pos.z - m_Datum.m_08.z);
    m_X = (int)(m_Datum.m_08.x * 2.0f);
    m_Y = (int)(m_Datum.m_08.z * 2.0f);
}

s8 Within(EXVector p, EXGeoTriggerDatum d);

// NEAR: retail copies the datum argument before building the position vector.
s8 XItemHandler_RadarBox::DoUpdate()
{
    EXVector pos = XItemHandler_Player::Instance()->GetItem()->Position();
    if (Within(EXVector(pos.x, pos.y + 1.0f, pos.z, pos.w), *(EXGeoTriggerDatum *)&m_Volume)) {
        float vol = m_Volume.m_08.z * m_Volume.m_08.y * m_Volume.m_08.x;
        if (fMapVolume == 0.0f || vol < fMapVolume) {
            fMapVolume = m_Volume.m_08.z * m_Volume.m_08.y * m_Volume.m_08.x;
            CurrentMap = RadarMap(m_Data0, *(EXGeoTriggerDatum *)&m_Volume);
        }
    }
    return 1;
}

void FERadar::DrawObjectives(FERadarInfo &info)
{
    FERadarItem item;
    item.m_Unk11 = FELevelManager::Instance(1)->LevelFromHash(gCurrentMap_GeoHash, 0)->IsRitual();
    for (XItem *p = gTrigList.GetFirst(); p; p = gTrigList.GetNext(p)) {
        item.m_Unk24 = 0;
        item.m_Unk10 = 0;
        item.m_Colour = ex::RGBA(0x7f, 0x7f, 0x7f, 0x80);
        item.m_Sprite = 0x6000055;
        if ((p->m_itemFlags & 1) && p->ItemHandler()) {
            EXItemHandler *pHandler = p->ItemHandler();
            if (pHandler->IsKindOf(EXRUNTIME_CLASS(XItemHandler_MissionTrigger))) {
                XItemHandler_MissionTrigger *pMT = (XItemHandler_MissionTrigger *)pHandler;
                if (!(pMT->m_Options & 0x101)) {
                    item.m_Sprite = 0x6000055;
                    if (pMT->m_Type == 0)
                        item.m_Colour = ex::RGBA(0x7f, 0x7f, 0x7f, 0x80);
                    else
                        item.m_Colour = ex::RGBA(0x7f, 0x7f, 0x7f, 0x80);
                    item.m_Unk1c = 0x10;
                    item.m_Unk20 = 0x10;
                    FEHud::Instance(1)->DrawWaypoint(p);
                    item.m_Pos = p->Position();
                    DrawItemOnRadar(item, info, (FERenderPass::Flags)2);
                }
            }
        }
    }
}

extern XItemList gHitableList;
extern XItemList gRechargeList;
extern XItemList gBeaconList;

void FERadar::DrawSpecialObjects(FERadarInfo &info)
{
    FERadarItem item;
    if (XItemHandler_Player::Instance()->m_Tint == 3) {
    item.m_Unk24 = 0;
    item.m_Unk10 = 0;
    item.m_Colour = ex::RGBA(0, 0, 0x7f, 0x80);
    item.m_Unk1c = 0x10;
    item.m_Unk20 = 0x10;
    item.m_Sprite = 0x60002b3;
    for (XItem *p = gRechargeList.GetFirst(); p; p = gRechargeList.GetNext(p)) {
        if (((XItemHandler_RechargePoint *)p->ItemHandler())->m_State != 2) {
            item.m_Sprite = 0x60002b2;
            item.m_Pos = p->Position();
            DrawItemOnRadar(item, info, (FERenderPass::Flags)2);
        }
    }
    for (XItem *p = gHitableList.GetFirst(); p; p = gHitableList.GetNext(p)) {
        XItemHandler_ScriptRunner *h = (XItemHandler_ScriptRunner *)p->ItemHandler();
        if (h && h->m_pMissionItem && h->m_Health > 0.0f) {
            item.m_Pos = p->Position();
            item.m_Sprite = 0x60002b3;
            DrawItemOnRadar(item, info, (FERenderPass::Flags)2);
        }
    }
    for (XItem *p = gCCTVList.GetFirst(); p; p = gCCTVList.GetNext(p)) {
        XItemHandler_CCTV *h = (XItemHandler_CCTV *)p->ItemHandler();
        if (h && h->m_State != 3) {
            item.m_Pos = p->Position();
            item.m_Sprite = 0x60002b5;
            DrawItemOnRadar(item, info, (FERenderPass::Flags)2);
        }
    }
    for (XItem *p = gBeaconList.GetFirst(); p; p = gBeaconList.GetNext(p)) {
        if (((XItemHandler_Beacon *)p->ItemHandler())->m_bActivated != 1) {
            item.m_Pos = p->Position();
            item.m_Sprite = 0x6000055;
            DrawItemOnRadar(item, info, (FERenderPass::Flags)2);
        }
    }
    }
}

s8 Within(EXVector p, EXGeoTriggerDatum d);

signed char FERadar::DrawItemOnRadar(FERadarItem &item, FERadarInfo &info, FERenderPass::Flags pass)
{
    if (info.m_Radius == 0.0f) return 0;
    if (info.m_Range == 0.0f) return 0;
    if (CurrentMap.m_Valid && !item.m_Unk11) {
        if (!Within(item.m_Pos, *(EXGeoTriggerDatum *)&CurrentMap.m_Datum)) return 0;
    }
    if (item.m_Colour.m_A <= 10) return 0;
    ex::RGBA col = item.m_Colour;
    EXVector d = item.m_Pos - info.m_PlayerPos;
    if (d.Length() > info.m_Range) {
        if (item.m_Unk10) return 0;
        col.m_A >>= 1;
    }
    float k = info.m_Radius / info.m_Range;
    d.x = d.x * k;
    d.y = d.y * k;
    d.z = d.z * k;
    if (EXSqrt(d.x * d.x + d.y * d.y + d.z * d.z) > info.m_Radius) {
        float k2 = info.m_Radius / EXSqrt(d.x * d.x + d.y * d.y + d.z * d.z);
        d.x = d.x * k2;
        d.y = d.y * k2;
        d.z = d.z * k2;
    }
    float scale = info.m_Small ? 0.5f : 1.0f;
    float ang = info.m_Unk28 ? 0.0f : XGetCamera(0)->Angle();
    float sn = (float)sin(ang);
    float cs = (float)cos(ang);
    int x = (int)((cs * d.x - sn * d.z) * scale) + info.m_Centre.x - info.m_pObject->m_pOwnerWnd->m_Rect.x;
    int y = info.m_Centre.y - (int)(cs * d.z + sn * d.x) - info.m_pObject->m_pOwnerWnd->m_Rect.y;
    int up = 0;
    if (d.y < -1.0f) {
        up = (int)d.y;
        info.m_pObject->DrawSprite(pass, 0x1000025, 0x6000054, col, item.m_Unk24, EXPoint(x, y), EXPoint(1, -up), (EXW::TextAlign)3);
        y -= up;
    } else if (d.y > 1.0f) {
        up = (int)d.y;
        info.m_pObject->DrawSprite(pass, 0x1000025, 0x6000054, col, item.m_Unk24, EXPoint(x, y), EXPoint(1, up), (EXW::TextAlign)5);
        y -= up;
    }
    if (item.m_Sprite)
        info.m_pObject->DrawSprite(pass, 0x1000025, item.m_Sprite, col, item.m_Unk24, EXPoint(x, y), EXPoint(item.m_Unk1c, item.m_Unk20), (EXW::TextAlign)4);
    if (item.m_Text.Length())
        info.m_pObject->DrawText(pass, 0x100003a, 0x7000004, col, item.m_Unk24, EXPoint(x, y), item.m_Text, (EXW::TextAlign)4, 0);
    return 1;
}

#include "EXPrimVert.h"
#include "EXGeoHeader.h"
#include "EXGeoFile.h"
#include "EXWnd.h"

// Draws a textured, rotated quad (centre.xy, size.xy) on the radar window. The 2D rotation is applied
// about the origin and then 'offset' is added; the x extent shrinks to 3/4 on small radars.
void FERadar::DrawPoly2D(FERadarInfo &info, EXVector offset, EXVector centre, EXVector size, unsigned int file, unsigned int tex,
                         int blend, ex::RGBA col, float angle)
{
    EXGeoFile *pFile = EXGeoFile::GetGeoFile(file);
    EXTexture *pTex = (pFile ? pFile->m_pGeoHeader : 0)->Texture(tex);
    if (!pTex) return;
    EXPrimVert v[4];
    memset(v, 0, sizeof(v));
    float hw = size.x * 0.5f;
    float hh = size.y * 0.5f;
    v[0].x = centre.x - hw; v[0].y = centre.y - hh; v[0].u = 0.0f; v[0].v = 1.0f;
    v[1].x = centre.x - hw; v[1].y = centre.y + hh; v[1].u = 0.0f; v[1].v = 0.0f;
    v[2].x = centre.x + hw; v[2].y = centre.y - hh; v[2].u = 1.0f; v[2].v = 1.0f;
    v[3].x = centre.x + hw; v[3].y = centre.y + hh; v[3].u = 1.0f; v[3].v = 0.0f;
    for (int i = 0; i < 4; i++) v[i].m_Color = *(u32 *)&col;
    float sn = (float)sin(angle);
    float cs = (float)cos(angle);
    float rot[16] = { cs, sn, 0, 0, -sn, cs, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
    for (int i = 0; i < 4; i++) {
        EXVector in(v[i].x, v[i].y, v[i].z, v[i].w);
        EXVector out;
        D3DXVec4Transform(&out, &in, rot);
        float sx = info.m_Small ? 0.75f : 1.0f;
        v[i].x = offset.x + sx * out.x;
        v[i].y = offset.y + out.y;
        v[i].z = offset.z + out.z;
        v[i].w = offset.w + out.w;
    }
    EXWnd *pWnd = (EXWnd *)info.m_pObject->m_pOwnerWnd;
    pWnd->SetTriStripZMode(1, 1);
    pWnd->SetTriStripBlendMode(blend);
    pWnd->SelectTriStripTexture(pTex);
    pWnd->DrawTriStrip(4, v);
}

#include "XApp.h"
#include "Actor.h"
#include "ActorProfile.h"
#include "XItemHandler_AirVehicle.h"
extern XItemList gAirVehicleList;

// Draws every living (or boss-head) actor as a dot coloured by faction/vision mode, then the air vehicles.
void FERadar::DrawActors(FERadarInfo &info)
{
    FERadarItem item;
    for (Actor *a = Actor::GetFirst(1); a; a = a->GetNext(1)) {
        if (!(a->m_Health > 0.0f) && !(((u32 &)a->m_StateFlags) & 0x1000)) continue;
        if (((u32 &)a->m_ActorFlags) & 4) continue;  // holographic
        EXVector head = a->GetHeadPosApprox();
        if (CurrentMap.m_Valid && !Within(head, *(EXGeoTriggerDatum *)&CurrentMap.m_Datum)) continue;
        item.m_Unk24 = 0;
        item.m_Unk1c = 0x10;
        item.m_Unk20 = 0x10;
        item.m_Pos = a->GetItem()->Position();
        switch (a->m_RadarType) {
        case 1: item.m_Sprite = 0x6000290; break;
        case 2: item.m_Sprite = 0x6000291; break;
        default: item.m_Sprite = a->m_Health > 0.0f ? 0x60000a7 : 0x6000291;  // HT_Texture_FERadarDot
        }
        switch (AS_GetPtrFromIndex(a->m_SkinIndex)->m_Type) {
        case 0: case 1: item.m_Colour = ex::RGBA(0x7f, 0x7f, 0x7f, 0x80); break;
        case 2: case 3: case 7: item.m_Colour = ex::RGBA(0, 0, 0x7f, 0x80); break;
        case 4: item.m_Colour = ex::RGBA(0x7f, 0, 0, 0x80); break;
        case 5: item.m_Colour = ex::RGBA(0, 0x7f, 0, 0x80); break;
        default: item.m_Colour = ex::RGBA(0x7f, 0x7f, 0x7f, 0x80); break;
        }
        ActorSkin *pSkin = AS_GetPtrFromIndex(a->m_SkinIndex);
        u32 stateFlags = (u32 &)a->m_StateFlags;
        u32 actorFlags = (u32 &)a->m_ActorFlags;
        switch (XItemHandler_Player::Instance()->m_Tint) {
        case 0:
            if (stateFlags & 0x200) item.m_Colour = ex::RGBA(0, 0, 0, 0);
            else if (Actor::GlobalAffiliation() != 0xffffffff)
                item.m_Colour = a->Affiliation() == Actor::GlobalAffiliation() ? ex::RGBA(0x7f, 0x7f, 0x7f, 0x80) : ex::RGBA(0x40, 0x40, 0x40, 0x80);
            break;
        case 1:
            if (!pSkin->m_Unk40[0] || (stateFlags & 0x200)) item.m_Colour = ex::RGBA(0, 0, 0, 0);
            break;
        case 2:
            if (!pSkin->m_Unk40[1] || (stateFlags & 0x200) || (actorFlags & 0x20000)) item.m_Colour = ex::RGBA(0, 0, 0, 0);
            else if (Actor::GlobalAffiliation() != 0xffffffff)
                item.m_Colour = a->Affiliation() == Actor::GlobalAffiliation() ? ex::RGBA(0x7f, 0x7f, 0x7f, 0x80) : ex::RGBA(0x40, 0x40, 0x40, 0x80);
            break;
        case 3:
            if ((!pSkin->m_Unk40[2] && !(stateFlags & 0x200)) || (actorFlags & 0x20000)) item.m_Colour = ex::RGBA(0, 0, 0, 0);
            break;
        }
        if (item.m_Colour == ex::RGBA(0, 0, 0, 0)) continue;
        if (a->m_FiredBulletTime + 5.0f > theApp.Time()) item.m_Colour = ex::RGBA(0x7f, 0, 0, 0x80);
        if (a->m_RefID != XGetCamera(0)->m_TargetID && a->m_RadarType == 0) {
            item.m_Unk10 = 1;
            DrawItemOnRadar(item, info, (FERenderPass::Flags)2);
        } else {
            item.m_Unk10 = 0;
            DrawItemOnRadar(item, info, (FERenderPass::Flags)3);
        }
    }
    unsigned char tint = XItemHandler_Player::Instance()->m_Tint;
    if (tint == 3 || tint == 0) {
        for (XItem *p = gAirVehicleList.GetFirst(); p; p = gAirVehicleList.GetNext(p)) {
            if (((XItemHandler_AirVehicle *)p->ItemHandler())->m_Unk110 == 1) {
                item.m_Unk1c = 0x10;
                item.m_Unk20 = 0x10;
                item.m_Unk24 = 0;
                item.m_Sprite = 0x6000290;
                item.m_Pos = p->Position();
                item.m_Colour = ex::RGBA(0, 0x7f, 0x7f, 0x80);
                item.m_Unk10 = 1;
                DrawItemOnRadar(item, info, (FERenderPass::Flags)2);
            }
        }
    }
}

signed char FERadar::Draw()
{
    if (theApp.m_bCutscene) return 1;
    FEHud *hud = FEHud::Instance(0);
    if (hud && (hud->m_State & 0x20) && (hud->m_State & 0x40) && (m_State & 0x40)) {
        hud->GetRadarInfo(m_Info);
        m_Info.m_pObject = m_pObject;
        SetRect(hud->GetRadarRect(m_Info));
        ex::RGBA hudCol = hud->m_pObject->Mathmos((FEObjColour::Flags)2)->m_Colour;
        m_pObject->SetMathmos((FEObjColour::Flags)2, FEMathmos(hudCol), 1);
        FEObject *mask = FEHud::Instance(1)->m_pObject->Object(0)->Object(0, 0);
        if (mask && CurrentMap.m_Valid) {
            // circular mask sprite behind the radar
            EXVector origin = mask->FindDatum(0x10000034);
            origin.y -= 0.03f;
            origin.z -= 2.0f;
            EXVector radius = mask->FindDatum(0x1000004f);
            EXVector d = origin - radius;
            float len2 = d.x * d.x + d.y * d.y;
            float len = len2 >= 0.0f ? (float)sqrt((double)len2) : 0.0f;
            len *= 1.3333334f;
            float k = m_Info.m_Small ? 1.33f : 1.0f;
            DrawPoly2D(m_Info, origin, EXVector(0.0f, 0.0f, 0.0f, 0.0f), EXVector(k * len * 2.0f, len * 2.0f, 0.0f, 0.0f), 0x1000025,
                       0x60000b9, 0, ex::RGBA(0x7f, 0x7f, 0x7f, 2), 0.0f);

            // the level's radar map, rotated by the camera
            origin = mask->FindDatum(0x10000034);
            origin.z -= -1.0f;
            float scale = 1.0f / m_Info.m_Range;
            float cy = (float)((CurrentMap.m_Y >> 1) + CurrentMap.m_H) - m_Info.m_PlayerPos.z;
            cy *= scale;
            float cx = (float)((CurrentMap.m_X >> 1) + CurrentMap.m_W) - m_Info.m_PlayerPos.x;
            cx *= scale;
            float sy = (float)CurrentMap.m_Y * scale;
            float sx = (float)CurrentMap.m_X * scale;
            ex::RGBA mapCol = m_pObject->Mathmos((FEObjColour::Flags)3)->m_Colour;
            float ang = XGetCamera(0)->Angle();
            ex::RGBA col = FEObject::MultiplyColour(m_pObject->MultiplyMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 2), mapCol);
            DrawPoly2D(m_Info, origin, EXVector(cx, cy, 0.0f, 0.0f), EXVector(sx, sy, 0.0f, 0.0f), gCurrentMap_GeoHash, CurrentMap.m_Hash, 2,
                       col, ang);
        }
        DrawActors(m_Info);
        DrawSpecialObjects(m_Info);
        DrawObjectives(m_Info);
        DrawHints(m_Info);
        DrawPredator(m_Info);

        // north marker
        FERadarItem item;
        if (XItemHandler_Player::Instance()->m_Tint == 3) {
            item.m_Colour = ex::RGBA(0, 0, 0x7f, 0x80);
            item.m_Unk24 = 0;
        } else {
            item.m_Colour = ex::RGBA(0x40, 0x40, 0x40, 0xff);
            item.m_Unk24 = 2;
        }
        item.m_Unk10 = 1;
        item.m_Sprite = 0x600008f;  // HT_Texture_FENorth
        item.m_Unk1c = 0x10;
        item.m_Unk20 = 0x10;
        EXVector p = XItemHandler_Player::Instance()->GetItem()->Position();
        p.z += 21.0f;
        item.m_Pos = p;
        DrawItemOnRadar(item, m_Info, (FERenderPass::Flags)4);
    }
    fMapVolume = 0.0f;
    return FEWnd::Draw();
}
