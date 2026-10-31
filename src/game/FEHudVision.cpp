// FEHudVision.obj: the HUD's first-person overlay, targeting reticle, the three vision-mode scan
// pages (thermal, neuro, tech) and the weapon selector.
#include "FEText.h"
#include "EXMemory.h"
#include "FEHud.h"
#include "FEObject.h"
#include "FEObjectScript.h"
#include "FE.h"
#include "Actor.h"
#include "Vehicle.h"
#include "XItem.h"
#include "XItemHandler_Camera.h"
#include "XItemHandler_Player.h"
#include "XItemHandler_AirVehicle.h"
#include "XItemHandler_Turret.h"
#include "XItemHandler_CCTV.h"
#include "XItemHandler_VMO.h"
#include "ScanInfo.h"
#include "EXString.h"
#include "XApp.h"
#include "FELayout.h"
#include "XGameWnd.h"
#include "EXBaseDisplay.h"
#include "ActorProfile.h"
#include "EXQMath.h"
#include "FEListener.h"
#include "FESound.h"
#include "XGamePad.h"
#include "GameData.h"
#include "EXBaseSoundManager.h"
#include <math.h>

Vehicle *getItemOwnerVehicle(EXItem *pItem);
extern XGameWnd *gpGameWnd[4];

// The screen rectangle spanned by two of an object's datums.
inline EXRect FEHud::MakeRect(FEObject *pObj, unsigned int tl, unsigned int br)
{
    EXPoint a = pObj->FindDatum2D(tl);
    EXPoint b = pObj->FindDatum2D(br);
    return EXRect(a.x, a.y, b.x - a.x, b.y - a.y);
}

// Reticle colours: red for honourable targets, green for dishonourable ones (profile honour < 0).
static inline ex::RGBA LockColour(int honour)
{
    return honour < 0 ? ex::RGBA(0, 0x7f, 0, 0x80) : ex::RGBA(0x7f, 0, 0, 0x80);
}

// The first-person view: the crosshair (unless the weapon selector or the zoom scope is up) and
// the scope background, whose script is scrubbed to show the zoom level (2x, 4x, 8x steps). The
// background is hidden while the cloak flag 0x224c is 2 with a vision mode active.
void FEHud::UpdateFirstPerson()
{
    FEObject *pBG = m_pObject->Object(2);
    if (!XGetCamera(0)->IsFirstPerson() || !m_Unk2194) {
        if (pBG) pBG->m_DeleteNeeded = 1;
        return;
    }
    if (!m_Unkb90[0] && !m_pObject->Object(7))
        DrawSprite((FERenderPass::Flags)2, 0x1000025, 0x6000056, ex::RGBA(0x7f, 0, 0, 0x80), 0,
                   EXPoint(Rect().w / 2, Rect().h / 2), EXPoint(0x10, 0x10), (EXW::TextAlign)4);
    if (!pBG) {
        pBG = m_pObject->ObjectAdd(2, new FEObject(FEObjType::OT_SCRIPT2, 0x1000025, 0x4000551, 0xfdffffff, FE_Blank,
                                                   FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        FE::_Init(pBG);
        FE::_Peek()->Script(0)->SetScale(EXVector(2.3f, 2.2f, 0.0f, 0.0f));
        FE::_Peek()->ScriptAdd(0x5f5f5f5f6d6f6f7a, FEObjType::OT_SCRIPT2, 0x1000025, 0x40005ed, (EXW::TextAlign)4);
        FE::_Peek()->Script(0x5f5f5f5f6d6f6f7a)->SetScale(2.0f);
        FE::_Peek()->Script(0x5f5f5f5f6d6f6f7a)->Play(0, 0x80000);
        FE::_Pop();
    }
    if (m_LockState == 2 && XItemHandler_Player::Instance()->m_Tint) {
        pBG->Script(0x5f5f5f5f6d6f6f7a)->m_State &= ~0x40;
        return;
    }
    pBG->Script(0x5f5f5f5f6d6f6f7a)->m_State |= 0x40;
    float played;
    if (XGetCamera(0)->m_Zoom >= 8.0f) played = 1.0f;
    else if (XGetCamera(0)->m_Zoom >= 4.0f) played = 0.75f;
    else if (XGetCamera(0)->m_Zoom >= 2.0f) played = 0.5f;
    else played = 0.25f;
    pBG->Script(0x5f5f5f5f6d6f6f7a)->SetAmountPlayed(played);
}

// Fills the scan page for whatever the camera is locked on to. The lock-on type picks the
// provider: 1 actor, 2 ground vehicle, 3 air vehicle, 5 turret, 6 CCTV camera or other vision
// object. Returns whether the page has content.
signed char FEHud::GetScanInfo(int page, ScanInfo *pInfo)
{
    if (!pInfo) return 0;
    pInfo->m_ac = 0;
    pInfo->m_pItem = 0;
    pInfo->m_ModelFile = 0;
    pInfo->m_ModelEntity = 0;
    u32 *pWords = &pInfo->m_Text[0][0];
    for (int i = 0; i < (0x90 - 0x10) / 4; i++) pWords[i] = 0;
    pInfo->m_id90 = 0;
    *(u32 *)&pInfo->m_94[0] = 0;
    *(u32 *)&pInfo->m_94[4] = 0;
    *(u32 *)&pInfo->m_94[8] = 0;
    pInfo->m_a4 = 0.0f;
    pInfo->m_pCallback = 0;
    pInfo->m_text = EXString(gText.m_Empty);  // an empty text entry (Text_Get(0) folded)

    int type = 0;
    void *pLockOn = XGetCamera(0)->GetLockOn(type);
    pInfo->m_pItem = (EXItem *)pLockOn;
    pInfo->m_0 = page;
    switch (type) {
    case 1:
        if (pLockOn) ((Actor *)pLockOn)->GetScanInfo(page, pInfo);
        break;
    case 3:
        if (pLockOn)
            ((XItemHandler_AirVehicle *)((EXItem *)pLockOn)->m_pItemHandler)->GetScanInfo(page, pInfo);
        break;
    case 5:
        if (pLockOn) ((XItemHandler_Turret *)((EXItem *)pLockOn)->m_pItemHandler)->GetScanInfo(page, pInfo);
        break;
    case 6:
        if (pLockOn) {
            EXItemHandler *pHandler = ((EXItem *)pLockOn)->m_pItemHandler;
            if (pHandler->IsKindOf(EXRUNTIME_CLASS(XItemHandler_CCTV)))
                ((XItemHandler_CCTV *)pHandler)->GetScanInfo(page, pInfo);
            else
                ((XItemHandler_VMO *)pHandler)->GetScanInfo(page, pInfo);
        }
        break;
    case 2:
        if (pLockOn) getItemOwnerVehicle((EXItem *)pLockOn)->GetScanInfo(page, pInfo);
        break;
    }
    return pInfo->m_ac;
}

// The lock-on reticle (the triangle target). While first person with the camera in mode 1, it
// sits over whatever the camera is locked on to and its script plays in from the edges; the
// reticle is scaled by zoom over distance (actors and most items at 50 units, vehicles at 200)
// and shrinks as it closes in. m_LockState becomes 2 once the closing script has finished.
void FEHud::UpdateTargeting()
{
    FEObject *pTri = m_pObject->Object(7);
    if (!XGetCamera(0)->IsFirstPerson() || XGetCamera(0)->m_Mode != 1 || theApp.m_bCutscene) {
        if (!pTri) return;
        pTri->m_DeleteNeeded = 1;
        m_LockState = 0;
        return;
    }
    if (!pTri) {
        pTri = m_pObject->ObjectAdd(7, new FEObject(FEObjType::OT_SCRIPT2, 0x1000025, 0x400021e, 0xfdffffff, FE_Blank,
                                                    FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        pTri->m_Flags |= 0x10;
        pTri->Script(0)->SetRemoteMathmos(&m_Mathmoscdc);
        pTri->Script(0)->m_State |= 0x2000010;
        m_LockTarget = 0;
        m_Mathmoscdc = FEMathmos(ex::RGBA(0x7f, 0, 0, 0x80));
    }
    int type;
    void *pLockOn = XGetCamera(0)->GetLockOn(type);
    if (!type) {
        if (pTri) pTri->m_DeleteNeeded = 1;
        m_LockState = 0;
        return;
    }
    EXPoint pt;
    if (type == 1) {
        if (XGetCamera(0)->m_TargetID != m_LockTarget) {
            m_LockTarget = XGetCamera(0)->m_TargetID;
            pTri->Script(0)->Play(0, 0);
            Actor *pNew = Actor::GetPtrFromID(m_LockTarget);
            if (pNew) m_Mathmoscdc = FEMathmos(LockColour(AP_GetPtrFromIndex(pNew->m_ProfileIndex)->m_Honour));
        }
        Actor *pActor = Actor::GetPtrFromID(m_LockTarget);
        if (!pActor) return;
        EXVector d = pActor->GetFeetPos() - XGetCamera(0)->GetItem()->Position();
        float dist = qsqrtf(d.x * d.x + d.y * d.y + d.z * d.z) * 0.02f;
        float played = pTri->Script(0)->AmountPlayed();
        float t = played < 0.8f ? played : 0.8f;
        pTri->Script(0)->SetScale((1.0f - t) * (1.0f / dist) * XGetCamera(0)->m_Zoom);
        gpGameWnd[0]->SetupCamera();
        pt = EXPoint((int)EXBaseDisplay::WorldToDisp(pActor->GetChestPos()).x,
                     (int)EXBaseDisplay::WorldToDisp(pActor->GetChestPos()).y);
    } else {
        EXItem *pItem = (EXItem *)pLockOn;
        if ((int)pItem != m_LockTarget) {
            m_LockTarget = (int)pItem;
            pTri->Script(0)->Play(0, 0);
        }
        float range = (type == 3 || type == 2) ? 200.0f : 50.0f;
        EXVector d = pItem->Position() - XGetCamera(0)->GetItem()->Position();
        float dist = qsqrtf(d.x * d.x + d.y * d.y + d.z * d.z) / range;
        float played = pTri->Script(0)->AmountPlayed();
        float t = played < 0.8f ? played : 0.8f;
        pTri->Script(0)->SetScale((1.0f - t) * (1.0f / dist) * XGetCamera(0)->m_Zoom);
        gpGameWnd[0]->SetupCamera();
        EXVector at = pItem->Position();
        if (type == 2) at.y += 0.5f;
        pt = EXPoint((int)EXBaseDisplay::WorldToDisp(at).x, (int)EXBaseDisplay::WorldToDisp(at).y);
    }
    pTri->Script(0)->SetPosition(WndToWorld(pt));
    m_LockState = pTri->Script(0)->AmountPlayed() >= 1.0f ? 2 : 1;
}

// The three scan pages (thermal, neuro, tech) share one shape: a layered page object on the HUD
// that opens once the reticle has locked on to something with content for that page, plays the
// target's scan script in, then fills in four columns of text. Retail inlines all of this in each
// Update*Scan; it is factored into helpers here.

// Whether the page for vision mode `mode` can stay up: the mode is active, first person with the
// camera in mode 1, still locked on to the scanned item, the page has content and no cutscene.
static signed char ScanPageValid(FEHud *pHud, u8 mode)
{
    int type = 0;
    return XItemHandler_Player::Instance()->m_Tint == mode && XGetCamera(0)->m_Mode == 1 &&
           XGetCamera(0)->IsFirstPerson() && XGetCamera(0)->GetLockOn(type) == pHud->m_ScanInfo.m_pItem &&
           pHud->m_ScanInfo.m_ac && !theApp.m_bCutscene;
}

// Closes the page (and lowers the weapon) when it can no longer stay up.
static void ScanPageCheck(FEHud *pHud, FEObject *pPage, u8 mode, signed char &open)
{
    if (ScanPageValid(pHud, mode)) return;
    if (pPage) pPage->m_DeleteNeeded = 1;
    if (open) {
        XItemHandler_Player::Instance()->ForceSafeWeapon();
        open = 0;
    }
}

// Opens the page at HUD slot `slot` once locked on with content for `page`: a layered object
// whose layer 0 plays the target's scan script.
static FEObject *ScanPageOpen(FEHud *pHud, unsigned int slot, unsigned int layers, int page, u8 mode,
                              signed char &open, signed char &filled)
{
    if (pHud->m_LockState != 2) return 0;
    if (XItemHandler_Player::Instance()->m_Tint != mode) return 0;
    if (!pHud->GetScanInfo(page, &pHud->m_ScanInfo)) return 0;
    open = 1;
    filled = 0;
    FEObject *pPage = pHud->m_pObject->ObjectAdd(slot, new FELayoutLayered(layers, 0, 0, 0, 0, (EXW::TextAlign)4));
    FE::_Init(pPage);
    FE::_Push(FE::_Peek()->ObjectAdd(0, new FEObject(FEObjType::OT_SCRIPT2, 0x1000025, pHud->m_ScanInfo.m_a0, 0xfdffffff,
                                                     FE_Blank, FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0,
                                                     (EXW::TextAlign)4)));
    FE::_Peek()->Script(0)->SetScale(1.1f);
    FE::_Peek()->Script(0)->m_State |= 0x2000000;
    FE::_Peek()->Script(0)->Play(0, 0);
    FE::_Pop();
    FE::_Pop();
    return pPage;
}

// The page is filled once its scan script has played all the way in.
static signed char ScanPageReadyToFill(FEObject *pPage, signed char open, signed char filled)
{
    return !filled && open && pPage->Object(0, 0)->Object(0)->Script(0)->AmountPlayed() >= 1.0f;
}

// Fades a scan page element in from black.
static inline FEMathmos ScanFadeIn()
{
    return FEMathmos(ex::RGBA(0, 0, 0, 0), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 60.0f, FEWaveform::WF_LINEAR, 1);
}

// Adds the four text columns (objects 1-4) inside the rectangles given by the scan script's
// datums, leaving the FE stack initialised on the page. Each column joins its text hashes one per
// line; a blank hash leaves an empty line and zero hashes are skipped. rects receives the column
// rectangles.
static void ScanColumnsAdd(FEHud *pHud, FEObject *pPage, EXRect rects[4])
{
    EXW::TextAlign aligns[4];
    rects[0] = pHud->MakeRect(pPage->Object(0), 0x10000157, 0x10000158);
    aligns[0] = (EXW::TextAlign)0;
    rects[1] = pHud->MakeRect(pPage->Object(0), 0x10000153, 0x10000154);
    aligns[1] = (EXW::TextAlign)0;
    rects[2] = pHud->MakeRect(pPage->Object(0), 0x10000155, 0x10000156);
    aligns[2] = (EXW::TextAlign)0;
    rects[3] = pHud->MakeRect(pPage->Object(0), 0x10000159, 0x1000015a);
    aligns[3] = (EXW::TextAlign)4;
    FE::_Init(pPage);
    for (int i = 0; i < 4; i++) {
        EXString text;
        for (int j = 0; j < 8; j++) {
            u32 hash = pHud->m_ScanInfo.m_Text[i][j];
            if (hash == 0x5100005a) text += "\n";
            else if (hash) text += EXString(gText.Get(hash)) + "\n";
        }
        if (text.Length()) {
            FE::_Push(FE::_Peek()->ObjectAdd(i + 1, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x700000a, 0xfdffffff, text,
                                                                 FE_Blank, aligns[i], rects[i].x, rects[i].y, rects[i].w,
                                                                 rects[i].h, (EXW::TextAlign)4)));
            FE::_Peek()->SetBlendMode(2);
            FE::_Peek()->m_Flags |= 0x20000;
            FE::_Peek()->SetMathmos((FEObjColour::Flags)2, ScanFadeIn(), 1);
            FE::_Pop();
        }
    }
}

// Thermal vision (mode 1) scan page, slot 4: the text columns, the summary line and the health
// gauge.
void FEHud::UpdateThermoScan()
{
    FEObject *pPage = m_pObject->Object(4);
    ScanPageCheck(this, pPage, 1, m_ThermoScanOpen);
    if (!pPage) {
        pPage = ScanPageOpen(this, 4, 7, 0, 1, m_ThermoScanOpen, m_ThermoScanFilled);
        if (!pPage) return;
    }
    if (ScanPageReadyToFill(pPage, m_ThermoScanOpen, m_ThermoScanFilled)) {
        m_ThermoScanFilled = 1;
        EXRect rects[4];
        ScanColumnsAdd(this, pPage, rects);
        // The summary line (ScanInfo text) shares the third column's rectangle.
        FE::_Push(FE::_Peek()->ObjectAdd(5, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x700000a, 0xfdffffff, FE_Blank,
                                                         FE_Blank, (EXW::TextAlign)4, rects[2].x, rects[2].y, rects[2].w,
                                                         rects[2].h, (EXW::TextAlign)4)));
        FE::_Peek()->SetMathmos((FEObjColour::Flags)2, ScanFadeIn(), 1);
        FE::_Pop();
        // The health gauge, a little larger than its datum.
        EXRect r = MakeRect(pPage->Object(0), 0x1000015d, 0x1000015d);
        FE::_Push(FE::_Peek()->ObjectAdd(6, new FEObject(FEObjType::OT_SCRIPT2, 0x1000025, 0x40002f6, 0xfdffffff, FE_Blank,
                                                         FE_Blank, (EXW::TextAlign)4, r.x - 4, r.y - 4, r.w + 8, r.h + 8,
                                                         (EXW::TextAlign)4)));
        FE::_Peek()->Script(0)->SetScale(1.1f);
        FE::_Peek()->Script(0)->m_State |= 0x2000000;
        FE::_Peek()->Script(0)->Play(0, 0x180000);
        FE::_Peek()->SetBlendMode(2);
        FE::_Peek()->SetMathmos((FEObjColour::Flags)3, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80)), 1);
        FE::_Peek()->SetMathmos((FEObjColour::Flags)2, ScanFadeIn(), 1);
        FE::_Pop();
        FE::_Pop();
        if (m_ScanInfo.m_pCallback) m_ScanInfo.m_pCallback(1, pPage, &m_ScanInfo);
    }
    if (pPage && m_ThermoScanFilled && m_ThermoScanOpen) {
        if (m_ScanInfo.m_pCallback) m_ScanInfo.m_pCallback(0, pPage, &m_ScanInfo);
        pPage->Object(6)->Script(0)->SetAmountPlayed(m_ScanInfo.m_a4);
        pPage->Object(5)->SetText(m_ScanInfo.m_text, 0, 0);
    }
}

// Neuro vision (mode 2) scan page, slot 5: just the text columns. Outside neuro vision (and
// normal vision) the global affiliation highlight is cleared.
void FEHud::UpdateNeuroScan()
{
    FEObject *pPage = m_pObject->Object(5);
    ScanPageCheck(this, pPage, 2, m_NeuroScanOpen);
    u8 tint = XItemHandler_Player::Instance()->m_Tint;
    if (tint != 2 && tint != 0) Actor::ResetGlobalAffiliation();
    if (!pPage) {
        pPage = ScanPageOpen(this, 5, 5, 1, 2, m_NeuroScanOpen, m_NeuroScanFilled);
        if (!pPage) return;
    }
    if (ScanPageReadyToFill(pPage, m_NeuroScanOpen, m_NeuroScanFilled)) {
        m_NeuroScanFilled = 1;
        EXRect rects[4];
        ScanColumnsAdd(this, pPage, rects);
        FE::_Pop();
        if (m_ScanInfo.m_pCallback) m_ScanInfo.m_pCallback(1, pPage, &m_ScanInfo);
    }
    if (pPage && m_NeuroScanFilled && m_NeuroScanOpen && m_ScanInfo.m_pCallback)
        m_ScanInfo.m_pCallback(0, pPage, &m_ScanInfo);
}

// Tech vision (mode 3) scan page, slot 6: the text columns and the scanned object's model, which
// spins while the game runs.
void FEHud::UpdateTechScan()
{
    FEObject *pPage = m_pObject->Object(6);
    ScanPageCheck(this, pPage, 3, m_TechScanOpen);
    if (!pPage) {
        pPage = ScanPageOpen(this, 6, 6, 2, 3, m_TechScanOpen, m_TechScanFilled);
        if (!pPage) return;
    }
    if (ScanPageReadyToFill(pPage, m_TechScanOpen, m_TechScanFilled)) {
        m_TechScanFilled = 1;
        EXRect rects[4];
        ScanColumnsAdd(this, pPage, rects);
        // Retail only pops the FE stack when there is a model.
        if (m_ScanInfo.m_ModelFile && m_ScanInfo.m_ModelEntity) {
            FE::_Push(FE::_Peek()->ObjectAdd(5, new FEObject(FEObjType::OT_SCRIPT, m_ScanInfo.m_ModelFile,
                                                             m_ScanInfo.m_ModelEntity, 0xfdffffff, FE_Blank, FE_Blank,
                                                             (EXW::TextAlign)4, rects[2].x, rects[2].y, rects[2].w,
                                                             rects[2].h, (EXW::TextAlign)4)));
            FE::_Peek()->m_Flags |= 0x80000000;
            FE::_Peek()->Script(0)->m_State |= 0x2000000;
            FE::_Peek()->Script(0)->TweakPosition(EXVector(0.0f, 0.0f, -10.0f, 0.0f));
            // Scale the model so its largest extent fills the frame.
            EXVector size = FE::_Peek()->Script(0)->BoundsBox();
            const float *pMax = size.y >= size.z ? &size.y : &size.z;
            if (size.x >= *pMax) pMax = &size.x;
            FE::_Peek()->Script(0)->SetScale(2.5f / *pMax);
            FE::_Peek()->SetMathmos((FEObjColour::Flags)3, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80)), 1);
            FE::_Peek()->SetMathmos((FEObjColour::Flags)2, ScanFadeIn(), 1);
            FE::_Pop();
        }
        if (m_ScanInfo.m_pCallback) m_ScanInfo.m_pCallback(1, pPage, &m_ScanInfo);
    }
    if (pPage && m_TechScanFilled && m_TechScanOpen) {
        if (m_ScanInfo.m_pCallback) m_ScanInfo.m_pCallback(0, pPage, &m_ScanInfo);
        if (pPage->Object(5) && theApp.IsRunning())
            pPage->Object(5)->Script(0)->TweakRotation(EXVector(0.0f, 0.1f, 0.0f, 0.0f));
    }
}

// Header inline defined in App.cpp; repeated here for the selector's close.
inline void FEHud::DisablePlayer(unsigned int buttons, int frames)
{
    m_DisablePlayer = frames;
    XItemHandler_Player::Instance()->SetButtonLock(buttons);
}

// One weapon entry of the selector, inside the row grid on top of the FE stack: a spacer, the
// name (with the ammo count for weapons that use ammo), the icon (tinted when focused) and a
// trailing spacer.
static void AddWeaponEntry(const FEWeapon &w, unsigned int nameWidth)
{
    FE::_Peek()->ObjectAdd(0, 0, new FEObject(FEObjType::OT_NONE, 0, 0, 0xfdffffff, FE_Blank, FE_Blank, (EXW::TextAlign)4,
                                              0, 0, 0x10, 0x1e, (EXW::TextAlign)4));
    FE::_Push(FE::_Peek()->ObjectAdd(0, 1, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 0xfdffffff,
                                                        gText.Get(w.m_Text), FE_Blank, (EXW::TextAlign)4, 0, 0,
                                                        nameWidth, 0x1e, (EXW::TextAlign)4)));
    if (w.m_UsesAmmo) {
        EXString name = gText.Get(w.m_Text);
        FE::_Peek()->Text().Format("%s [%d]", name.data(), XItemHandler_Player::Instance()->m_Ammo[w.m_Hash]);
    }
    FE::_Pop();
    FE::_Push(FE::_Peek()->ObjectAdd(0, 2, new FEObject(FEObjType::OT_SPRITE, 0x1000025, w.m_Icon, 0xfdffffff, FE_Blank,
                                                        FE_Blank, (EXW::TextAlign)4, 0, 0, 0x20, 0x1e,
                                                        (EXW::TextAlign)4)));
    FE::_Peek()->SetMathmos((FEObjColour::Flags)0, FEMathmos::DefaultTintFocus(), 1);
    FE::_Peek()->SetMathmos((FEObjColour::Flags)1, FEMathmos::DefaultTint(), 1);
    FE::_Peek()->SetBlendMode(2);
    FE::_Pop();
    FE::_Peek()->ObjectAdd(0, 3, new FEObject(FEObjType::OT_NONE, 0, 0, 0xfdffffff, FE_Blank, FE_Blank, (EXW::TextAlign)4,
                                              0, 0, 0x10, 0x1e, (EXW::TextAlign)4));
}

// The weapon selector. Holding the select button (0x80000) while the game runs opens it: the game
// pauses (game state 0x14), the scan pages are hidden, and for each of the four weapon categories
// a row lists the weapons the player has (weapons that use ammo are left out when empty, unless
// equipped). A category with several weapons gets a scrolling list, otherwise a single entry.
// Each row is bound to m_SelectWeapons. Pressing select or 0x8000 again closes it, equips the
// changed choices and briefly locks the player's buttons.
void FEHud::UpdateWeaponSelect()
{
    if (XItemHandler_Player::Instance()->m_bScanMode) return;
    FEObject *pSel = m_pObject->Object(3);
    if (FEListener::QueryButton(0x80000, GameControllerRemovedSys::Controller(), 1) &&
        XItemHandler_Player::Instance()->CanAssignWeapon() && theApp.IsRunning() && !theApp.m_FadeActive && !pSel) {
        m_pObject->Pause(1);
        for (int slot = 4; slot <= 6; slot++) {
            if (m_pObject->Object(slot)) {
                m_pObject->Object(slot)->StatePush();
                m_pObject->Object(slot)->m_Flags &= ~0x40;
            }
        }
        for (int i = 0; i < 4; i++) {
            m_SelectWeapons[i] = XItemHandler_Player::Instance()->m_CurrentWeapon[i];
            m_SelectStartWeapons[i] = XItemHandler_Player::Instance()->m_CurrentWeapon[i];
        }
        FE::_Init(m_pObject->ObjectAdd(3, new FELayoutLayered(2, 0, 0, 0, 0, (EXW::TextAlign)4)));
        FE::_Peek()->SetMathmos((FEObjColour::Flags)3, FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80)), 1);
        FE::_Peek()->SetMathmos((FEObjColour::Flags)2, FEMathmos::DefaultFadeUp(), 1);
        // The backdrop.
        FE::_Push(FE::_Peek()->ObjectAdd(0, new FEObject(FEObjType::OT_SCRIPT2, 0x1000025, 0x4000138, 0xfdffffff, FE_Blank,
                                                         FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4)));
        FE::_Peek()->Script(0)->SetSize(300, 0x8c, 0);
        FE::_Peek()->Script(0)->Play(0, 0);
        FE::_Pop();
        FE::_Push(FE::_Peek()->ObjectAdd(1, new FELayoutFlexGrid(1, 2, (EXW::TextAlign)4)));
        // Left column: the button for each category.
        FE::_Push(FE::_Peek()->ObjectAdd(0, 0, new FELayoutFixedGrid(4, 1, 0, 0, 0x28, 0x78, (EXW::TextAlign)4)));
        static const int s_Actions[4] = {1, 2, 4, 8};
        for (int i = 0; i < 4; i++)
            FE::_Peek()->ObjectAdd(i, 0, new FEObject(FEObjType::OT_TEXT, 0x100003a, 0x7000004, 0xfdffffff,
                                                      ((XGamePad *)EXBaseGamePad::GetGamePad())->GetActionString(s_Actions[i]),
                                                      FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4));
        FE::_Pop();
        // Right column: the category rows.
        FE::_Push(FE::_Peek()->ObjectAdd(0, 1, new FELayoutFixedGrid(4, 1, 0, 0, 0xfa, 0x78, (EXW::TextAlign)4)));
        FE::_Peek()->SetMathmos((FEObjColour::Flags)1, FEMathmos(ex::RGBA(0x5f, 0x5f, 0x5f, 0x80)), 1);
        FE::_Peek()->SetMathmos((FEObjColour::Flags)0, FEMathmos(ex::RGBA(0xff, 0xff, 0xff, 0x80)), 1);
        for (unsigned int c = 0; c < 4; c++) {
            FEWeapon *pRow = m_Weapons[c];
            int shown = 0;
            for (int j = 0; j < 8; j++) {
                FEWeapon &w = pRow[j];
                w.m_Shown = 1;
                if (w.m_Hash == 0xfeffffff) w.m_Shown = 0;
                else if (w.m_UsesAmmo && !XItemHandler_Player::Instance()->m_Ammo[w.m_Hash] && m_SelectWeapons[c] != w.m_Hash)
                    w.m_Shown = 0;
                else
                    shown++;
            }
            if (shown > 1) {
                FE::_Push(FE::_Peek()->ObjectAdd(c, 0, new FELayoutScrollGrid(1, shown, 0, 0, 0, 0, (EXW::TextAlign)4)));
                if (c == (unsigned int)m_SelectCategory) FE::_SetMarker(0);
                FE::_Peek()->SetAltData(c);
                int row = 0;
                for (int j = 0; j < 8; j++) {
                    if (!pRow[j].m_Shown) continue;
                    FE::_Push(FE::_Peek()->ObjectAdd(0, row++, new FELayoutFlexGrid(1, 4, (EXW::TextAlign)4)));
                    FE::_Peek()->m_Flags |= 8;
                    FE::_Peek()->SetData(pRow[j].m_Hash);
                    FE::_Peek()->SetAltData(c);
                    AddWeaponEntry(pRow[j], 0xba);
                    FE::_Pop();
                }
            } else {
                FE::_Push(FE::_Peek()->ObjectAdd(c, 0, new FELayoutFlexGrid(1, 4, (EXW::TextAlign)4)));
                if (c == (unsigned int)m_SelectCategory) FE::_SetMarker(0);
                int first = 0;
                for (int j = 0; j < 8; j++) {
                    if (pRow[j].m_Shown) {
                        first = j;
                        break;
                    }
                }
                FE::_Peek()->m_Flags |= 8;
                FE::_Peek()->SetData(pRow[first].m_Hash);
                FE::_Peek()->SetAltData(c);
                AddWeaponEntry(pRow[first], 0xb0);
            }
            FE::_Peek()->SetVisibleCount(1, 1);
            FE::_Peek()->Bind((FEDataType::Flags)4, &m_SelectWeapons[c]);
            FE::_Pop();
        }
        FE::_Peek()->ListenerAdd(new FEListener(0x5f5f5f5f5f76616e, 0xc03, 0, (FEPad::Flags)0));
        FE::_Peek()->Listener(0x5f5f5f5f5f76616e)->SetFocus(FE::_Marker(0), 0);
        FE::_Pop();
        FE::_Pop();
        FE::_Pop();
        theApp.GameStatePush((GameState::Flags)0x14, 0);
        FESound::FadeDown(10);
        EXBaseSoundManager::GetSoundManager()->StartSound(0x1af0015b);
        m_Unkb90[0] = 1;
        return;
    }
    if (!pSel) return;
    if (!(m_State & 1)) return;
    if (!FEListener::QueryButton(0x80000, GameControllerRemovedSys::Controller(), 1) &&
        !FEListener::QueryButton(0x8000, GameControllerRemovedSys::Controller(), 1))
        return;
    for (int i = 0; i < 4; i++)
        if (m_SelectStartWeapons[i] != m_SelectWeapons[i])
            XItemHandler_Player::Instance()->SetCurrentWeapon(i, m_SelectWeapons[i]);
    m_SelectCategory = (int)pSel->Object(1)->Object(0, 1)->Listener(0x5f5f5f5f5f76616e)->Focus()->AltData();
    for (int slot = 4; slot <= 6; slot++)
        if (m_pObject->Object(slot)) m_pObject->Object(slot)->StatePop();
    pSel->m_DeleteNeeded = 1;
    DisablePlayer(0xffffff5f, 10);
    m_pObject->Pause(0);
    theApp.GameStatePop();
    FESound::FadeUp(10);
    EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00159);
    m_Unkb90[0] = 0;
}
