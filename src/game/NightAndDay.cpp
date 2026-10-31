#include "XItem.h"
#include "XItemHandler_LightSet.h"
#include "XItemHandler_Environment.h"
#include "XItemHandler_Camera.h"
#include "XGameWnd.h"
#include "XLensFlare.h"
#include "EXItemAnimator_Script.h"
#include "EXGeoFile.h"
#include "EXBaseRenderEnv.h"
#include "EXWatcher.h"
#include "EXRand.h"
#include "EXMemory.h"
#include "EXBaseApp.h"
#include "ActorOptions.h"
#include "GameData.h"
#include "XItemHandler_Player.h"
#include "Vehicle.h"
#include "EXExtRenderInfoStandIn.h"
#include "EXRayCastTemp.h"
#include "EXGeoEntity_FaceRef.h"
#include "EXGeoTexture.h"
#include "EXGeoMap.h"
#include "EXMapOn.h"
#include "EXMatrix.h"
#include "EXQuaternion.h"
#include "EXItem.h"
#include "EXPrimVert.h"
#include "XApp.h"
#include "XItemHandler_Camera.h"
#include "EXGeoTrigger.h"

// Vision modes, fog and bloom per level, rain and the item light set (NightAndDay.obj).

extern XGameWnd *gpGameWnd[4];
extern unsigned int gCurrentMap_GeoHash;
s8 Within(EXVector p, EXGeoTriggerDatum d);

// Post effects of one vision mode (sizeof 0x2c), edited in the watcher under Effects.
struct VM_Data {
    VM_Data() {}
    ex::RGBA m_Colour;          // 0x00 dynamic light colour multiplier
    s8 m_FocusBlur;             // 0x04 effect 4
    s8 m_Glow;                  // 0x05 effect 1
    s8 m_Inverse;               // 0x06 effect 0x40
    s8 m_EnhancedInverse;       // 0x07 effect 0x80
    s8 m_MotionBlur;            // 0x08 effect 8
    s8 m_Noise;                 // 0x09 effect 2
    float m_NoiseIntensity;     // 0x0c
    float m_GlowIntensity;      // 0x10
    float m_InverseFactor;      // 0x14
    s8 m_InvMaskR;              // 0x18
    s8 m_InvMaskG;              // 0x19
    s8 m_InvMaskB;              // 0x1a
    float m_MotionBlurFactor;   // 0x1c
    float m_MotionBlurZoom;     // 0x20
    int m_FocusBlurFactor;      // 0x24
    float m_FocusBlurDistance;  // 0x28
};

XLensFlare Flare;
XLensFlare ThermalFlare;
ex::RGBA RainColour = 0x40505050;
int AddedThisFrame;
s8 Storm;
EXMaths::vec_xyzw RainVel;
float WetYMin[192];
s8 Raining;
int RainLimit;
s8 AVP_Thermo;
EXVector LightDir[3];
EXVector ambvector;
VM_Data Thermal;
static unsigned int s_RainTime = EXBaseApp::NominalGameFrameRate() * 15;
EXVector Wet[192];
VM_Data Tech;
VM_Data Brain;
ex::RGBA BloomColour;
EXVector LightRGB[3];
static unsigned int s_LightVehicleIdx;

void Init_VM_Data()
{
    Thermal.m_Colour = ex::RGBA(0, 0, 0x7f, 0xff);
    Thermal.m_FocusBlur = 1;
    Thermal.m_Glow = 1;
    Thermal.m_Inverse = 0;
    Thermal.m_EnhancedInverse = 0;
    Thermal.m_MotionBlur = 0;
    Thermal.m_Noise = 1;
    Thermal.m_NoiseIntensity = 0.1f;
    Thermal.m_GlowIntensity = 0.17f;
    Thermal.m_InverseFactor = 1.5f;
    Thermal.m_InvMaskR = 0;
    Thermal.m_InvMaskG = 0;
    Thermal.m_InvMaskB = 0;
    Thermal.m_MotionBlurFactor = 0.5f;
    Thermal.m_MotionBlurZoom = 1.0f;
    Thermal.m_FocusBlurFactor = 5;
    Thermal.m_FocusBlurDistance = 10.0f;

    Brain.m_Colour = ex::RGBA(0x5a, 0, 0, 0xff);
    Brain.m_FocusBlur = 0;
    Brain.m_Glow = 0;
    Brain.m_Inverse = 0;
    Brain.m_EnhancedInverse = 1;
    Brain.m_MotionBlur = 0;
    Brain.m_Noise = 0;
    Brain.m_NoiseIntensity = 0.06f;
    Brain.m_GlowIntensity = 1.0f;
    Brain.m_InverseFactor = 1.5f;
    Brain.m_InvMaskR = 0;
    Brain.m_InvMaskG = 0;
    Brain.m_InvMaskB = 1;
    Brain.m_MotionBlurFactor = 0.1f;
    Brain.m_MotionBlurZoom = 1.0f;
    Brain.m_FocusBlurFactor = 4;
    Brain.m_FocusBlurDistance = 0.5f;

    Tech.m_Colour = ex::RGBA(0, 0x82, 0x1e, 0xff);
    Tech.m_FocusBlur = 0;
    Tech.m_Glow = 1;
    Tech.m_Inverse = 0;
    Tech.m_EnhancedInverse = 0;
    Tech.m_MotionBlur = 1;
    Tech.m_Noise = 1;
    Tech.m_NoiseIntensity = 0.05f;
    Tech.m_GlowIntensity = 0.5f;
    Tech.m_InverseFactor = 1.3f;
    Tech.m_InvMaskR = 1;
    Tech.m_InvMaskG = 0;
    Tech.m_InvMaskB = 1;
    Tech.m_MotionBlurFactor = 0.2f;
    Tech.m_MotionBlurZoom = 0.99f;
    Tech.m_FocusBlurFactor = 2;
    Tech.m_FocusBlurDistance = 40.0f;
}

void SetEffects(VM_Data *d, unsigned int &effects, unsigned int &mask)
{
    if (d->m_FocusBlur) effects |= 4;
    if (d->m_Glow) effects |= 1;
    if (d->m_Inverse) effects |= 0x40;
    if (d->m_EnhancedInverse) effects |= 0x80;
    if (d->m_MotionBlur) effects |= 8;
    if (d->m_Noise) effects |= 2;
    gpGameWnd[0]->SetNoiseIntensity(d->m_NoiseIntensity);
    gpGameWnd[0]->SetGlowIntensity(d->m_GlowIntensity);
    gpGameWnd[0]->SetInverseColorFactor(d->m_InverseFactor);
    if (d->m_InvMaskR) mask |= 0xff0000ff;
    if (d->m_InvMaskG) mask |= 0xff00ff00;
    if (d->m_InvMaskB) mask |= 0xffff0000;
    gpGameWnd[0]->SetMotionBlurFactor(d->m_MotionBlurFactor);
    gpGameWnd[0]->m_FocusBlurFactor = d->m_FocusBlurFactor;
    gpGameWnd[0]->m_FocusBlurDistance = d->m_FocusBlurDistance;
    float zoom = d->m_MotionBlurZoom;
    EXRectf &r = gpGameWnd[0]->m_MotionBlurDrawRect;
    float o = (zoom - 1.0f) * -0.5f;
    r.x = o;
    r.y = o;
    r.w = zoom;
    r.h = zoom;
}

void Rain_Stop() { RainLimit = 192; }

EX_IMPLEMENT_DYNCREATE(XItemHandler_LightSet, EXItemHandler)

XItemHandler_LightSet::XItemHandler_LightSet() { m_pItem = 0; }

XItemHandler_LightSet::~XItemHandler_LightSet()
{
    if (m_pItem) theItemEnv.RemoveItem(m_pItem);
}

s8 XItemHandler_LightSet::Initialise() { return 1; }

s8 XItemHandler_LightSet::DoUpdate() { return 1; }

void XItemHandler_LightSet::StartLightSetScript()
{
    EXGeoFile *pFile = EXGeoFile::GetGeoFile(gCurrentMap_GeoHash);
    XItem *pItem = XItem::CreateItem(pFile, 0x40001af, EXRUNTIME_CLASS(XItemHandler_LightSet), EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    theItemEnv.AddItem(pItem, 0);
    EXItemAnimator_Script *pScript = new EXItemAnimator_Script;
    if (!pScript->Initialise(pFile, 0x40001af)) {
        theItemEnv.RemoveItem(pItem);
        return;
    }
    ((XItemHandler_LightSet *)pItem->m_pItemHandler)->m_pItem = pItem;
    pItem->AddAnimator(pScript);
}

u32 XItemHandler_LightSet::DoScriptCmdEvent(GEOSCRIPT_EVENT *pEvent, EXItemAnimator *)
{
    return ((const int *)pEvent)[3] != -1 ? 0 : 2;
}

void Add_VM_Data(VM_Data *d, char *pName)
{
    EXWatcher *w = EXWatcher::GetWatcher();
    unsigned int h;
    w->AddSection(pName, "Effects", "-Game-", 2);
    h = w->AddItem("Global Colour", pName, EXWATCH::DT_9, &d->m_Colour, 2);
    w->EnableItemEdit(h, 1);
    h = w->AddItem("Focus Blur", pName, EXWATCH::DT_8, &d->m_FocusBlur, 2);
    w->EnableItemEdit(h, 1);
    h = w->AddItem("Glow", pName, EXWATCH::DT_8, &d->m_Glow, 2);
    w->EnableItemEdit(h, 1);
    h = w->AddItem("Inverse", pName, EXWATCH::DT_8, &d->m_Inverse, 2);
    w->EnableItemEdit(h, 1);
    h = w->AddItem("Enhanced Inverse", pName, EXWATCH::DT_8, &d->m_EnhancedInverse, 2);
    w->EnableItemEdit(h, 1);
    h = w->AddItem("Motion Blur", pName, EXWATCH::DT_8, &d->m_MotionBlur, 2);
    w->EnableItemEdit(h, 1);
    h = w->AddItem("Noise", pName, EXWATCH::DT_8, &d->m_Noise, 2);
    w->EnableItemEdit(h, 1);
    h = w->AddItem("Noise Intensity", pName, EXWATCH::DT_7, &d->m_NoiseIntensity, 2);
    w->EnableItemEdit(h, 0.0f, 1.0f, 0.01f, 1);
    h = w->AddItem("Glow Intensity", pName, EXWATCH::DT_7, &d->m_GlowIntensity, 2);
    w->EnableItemEdit(h, 0.0f, 1.0f, 0.01f, 1);
    h = w->AddItem("Inverse Factor", pName, EXWATCH::DT_7, &d->m_InverseFactor, 2);
    w->EnableItemEdit(h, 0.0f, 2.0f, 0.1f, 1);
    h = w->AddItem("Inv Mask R", pName, EXWATCH::DT_8, &d->m_InvMaskR, 2);
    w->EnableItemEdit(h, 1);
    h = w->AddItem("Inv Mask G", pName, EXWATCH::DT_8, &d->m_InvMaskG, 2);
    w->EnableItemEdit(h, 1);
    h = w->AddItem("Inv Mask B", pName, EXWATCH::DT_8, &d->m_InvMaskB, 2);
    w->EnableItemEdit(h, 1);
    h = w->AddItem("M. Blur Factor", pName, EXWATCH::DT_7, &d->m_MotionBlurFactor, 2);
    w->EnableItemEdit(h, 0.0f, 1.0f, 0.01f, 1);
    h = w->AddItem("M. Blur Zoom", pName, EXWATCH::DT_7, &d->m_MotionBlurZoom, 2);
    w->EnableItemEdit(h, 0.0f, 2.0f, 0.01f, 1);
    h = w->AddItem("F. Blur Factor", pName, EXWATCH::DT_3, &d->m_FocusBlurFactor, 2);
    w->EnableItemEdit(h, 0, 20, 1, 1);
    h = w->AddItem("F. Blur Distance", pName, EXWATCH::DT_7, &d->m_FocusBlurDistance, 2);
    w->EnableItemEdit(h, 0.0f, 100.0f, 0.5f, 1);
}

void SetWatcherEffects(unsigned char mode)
{
    unsigned int effects = 0;
    unsigned int mask = 0xff000000;
    switch (mode) {
    case 1:
        SetEffects(&Thermal, effects, mask);
        theItemEnv.EnableDynLightRGBMult(1);
        theItemEnv.SetDynLightRGBMult(Thermal.m_Colour.m_R * (1.0f / 255.0f), Thermal.m_Colour.m_G * (1.0f / 255.0f), Thermal.m_Colour.m_B * (1.0f / 255.0f));
        break;
    case 2:
        SetEffects(&Brain, effects, mask);
        theItemEnv.EnableDynLightRGBMult(1);
        theItemEnv.SetDynLightRGBMult(Brain.m_Colour.m_R * (1.0f / 255.0f), Brain.m_Colour.m_G * (1.0f / 255.0f), Brain.m_Colour.m_B * (1.0f / 255.0f));
        break;
    case 3:
        SetEffects(&Tech, effects, mask);
        theItemEnv.EnableDynLightRGBMult(1);
        theItemEnv.SetDynLightRGBMult(Tech.m_Colour.m_R * (1.0f / 255.0f), Tech.m_Colour.m_G * (1.0f / 255.0f), Tech.m_Colour.m_B * (1.0f / 255.0f));
        break;
    default:
        if (XGetCamera(0)->IsFirstPerson() && XGetCamera(0)->Warp() > 0.0f) {
            effects = 4;
            gpGameWnd[0]->m_FocusBlurFactor = (int)XGetCamera(0)->Warp();
            gpGameWnd[0]->m_FocusBlurDistance = 0.0f;
        }
        theItemEnv.EnableDynLightRGBMult(0);
        break;
    }
    gpGameWnd[0]->SetNoiseIntensity(0.06f);
    gpGameWnd[0]->m_Effects = effects & ~2;
    gpGameWnd[0]->SetInverseColorMask(mask);
}

void InitWatcherEffects()
{
    EXWatcher *w = EXWatcher::GetWatcher();
    unsigned int h;
    w->AddSection("Effects", (unsigned int)0, "-Game-", 2);
    h = w->AddItem("Rain Colour", "Effects", EXWATCH::DT_9, &RainColour, 2);
    w->EnableItemEdit(h, 1);
    Init_VM_Data();
    Add_VM_Data(&Thermal, "Thermal");
    Add_VM_Data(&Brain, "Brain");
    Add_VM_Data(&Tech, "Tech");
    BloomColour = gpGameWnd[0]->BloomColor();
    h = w->AddItem("Bloom Colour", "Effects", EXWATCH::DT_9, &BloomColour, 2);
    w->EnableItemEdit(h, 1);
    h = w->AddItem("AVP Thermo", "Effects", EXWATCH::DT_8, &AVP_Thermo, 2);
    w->EnableItemEdit(h, 1);
}

void Rain_Initialise()
{
    EXVector *pW = Wet;
    float *pY = WetYMin;
    for (int i = 192; i; i--) {
        pW->x = (float)EXRand(140) * 0.1f - 7.0f;
        pW->y = (float)EXRand(140) * 0.1f - 7.0f;
        pW->z = (float)EXRand(140) * 0.1f - 7.0f;
        *pY = -4000.0f;
        pW++;
        pY++;
    }
    Raining = 0;
    for (XItem *p = gEnvList.GetFirst(); p; p = gEnvList.GetNext(p)) {
        XItemHandler_Environment *h = (XItemHandler_Environment *)p->m_pItemHandler;
        if (h->m_Type == 2 && (!h->m_Unk58 || FlagChannels[h->m_Unk58])) Raining = 1;
    }
    Storm = 0;
    switch (gCurrentMap_GeoHash) {
    case 0x100011c:
    case 0x100012a:
    case 0x100012c:
        Raining = 1;
        Storm = 1;
        break;
    }
    RainVel.set(0.0f, 1.0f, 0.0f, 1.0f);
    RainLimit = -1;
}

void HideSky(ex::RGBA c)
{
    gpGameWnd[0]->m_Flags.BitSet(2);
    gpGameWnd[0]->m_BkColor = c;
}

void ShowSky() { gpGameWnd[0]->m_Flags.BitClr(2); }

void SetLevelFogBloom();

static inline void SetVisionFog(ex::RGBA c, float n, float f, s8 enable)
{
    EXBaseRenderEnv::SetOverrideFog(c, n, f, 1.0f, 0.0f, enable);
}

void SetFog(unsigned char mode)
{
    ex::RGBA c, b;
    switch (mode) {
    case 1:
        if (AVP_Thermo || IsSkinAVP((s16)GameData::Instance(1)->m_Values.m_Unk58)) {
            Thermal.m_Colour = ex::RGBA(0x40, 0, 0x18, 0xff);
            c = ex::RGBA(0x20, 0, 0, 0xff);
        } else {
            Thermal.m_Colour = ex::RGBA(0, 0, 0x7f, 0xff);
            c = ex::RGBA(0x0e, 0x3e, 0x96, 0xff);
        }
        b = ex::RGBA(0x40, 0x40, 0x40, 0x40);
        SetVisionFog(c, 59.0f, 89.0f, 1);
        break;
    case 2:
        Brain.m_Colour = ex::RGBA(0x5a, 0x2d, 0, 0xff);
        c = ex::RGBA(0x2d, 0x16, 0, 0xff);
        b = ex::RGBA(0, 0, 0, 0x40);
        SetVisionFog(c, 49.0f, 160.0f, 1);
        break;
    case 3:
        Tech.m_Colour = ex::RGBA(0, 0x82, 0x1e, 0xff);
        c = ex::RGBA(0, 0x19, 0x0c, 0xff);
        b = ex::RGBA(0x0c, 0x0c, 0x0c, 0x40);
        SetVisionFog(c, 20.0f, 160.0f, 0);
        break;
    default:
        SetLevelFogBloom();
        ShowSky();
        return;
    }
    HideSky(c);
    BloomColour = b;
}

void Rain_Add(int i)
{
    AddedThisFrame++;
    XItem *p = gEnvList.GetFirst();
while (p) {
XItemHandler_Environment *h = (XItemHandler_Environment *)p->m_pItemHandler;
        if (h->m_Type == 2 && (!h->m_Unk58 || FlagChannels[h->m_Unk58])) {
            if (Within(Wet[i], *(EXGeoTriggerDatum *)&h->m_Datum)) {
                WetYMin[i] = *(float *)((char *)&h->m_Datum + 0x18) - *(float *)((char *)&h->m_Datum + 0xc);
                return;
            }
        }
p = gEnvList.GetNext(p);
}
    Wet[i].y = -4000.0f;
}

void NightAndDay_Initialise()
{
    SetLevelFogBloom();
    if (SunTrigger) {
        EXGeoFile *pFile = EXGeoFile::LoadGeoFile(0x1000189, 0, 0);
        EXTexture *t0 = pFile->GeoHeader()->Texture(0x60002ff);
        EXTexture *t1 = pFile->GeoHeader()->Texture(0x6000300);
        EXTexture *t2 = pFile->GeoHeader()->Texture(0x6000301);

        XLensFlare f(7);
        f.m_Unkd0 = 50.0f;
        f.m_Unkd4 = 0.125f;
        f.m_Unkd8 = f.m_Unkd4;
        f.m_Flares[0] = EXFlare(1.0f, 0.0f, ex::XRGBA(0x7f, 0x40, 0x00, 0x7f), 0.2f, t0);
        f.m_Flares[1] = EXFlare(1.5f, 0.0f, ex::XRGBA(0x40, 0x40, 0x7f, 0x26), 0.3f, t1);
        f.m_Flares[2] = EXFlare(0.5f, 0.0f, ex::XRGBA(0x7f, 0x7f, 0x7f, 0x7f), 0.35f, t0);
        f.m_Flares[3] = EXFlare(1.0f, 0.0f, ex::XRGBA(0x7f, 0x19, 0x00, 0x40), 0.7f, t1);
        f.m_Flares[4] = EXFlare(3.0f, 0.0f, ex::XRGBA(0x7f, 0x40, 0x00, 0x10), 0.8f, t1);
        f.m_Flares[5] = EXFlare(0.2f, 0.0f, ex::XRGBA(0x7f, 0x7f, 0x00, 0x7f), 1.0f, t0);
        f.m_Flares[6] = EXFlare(1.0f, 0.0f, ex::XRGBA(0x7f, 0x7f, 0x7f, 0x7f), 0.0f, t2);
        Flare = f;

        XLensFlare tf(1);
        tf.m_Unkd0 = 50.0f;
        tf.m_Unkd4 = 0.125f;
        tf.m_Unkd8 = tf.m_Unkd4;
        tf.m_Flares[0] = EXFlare(5.0f, 0.0f, ex::XRGBA(0x7f, 0x40, 0x00, 0x7f), 0.0f, t0);
        ThermalFlare = tf;
    }
    LensFlareActive = SunTrigger;
    SunTrigger = 0;
}


// Lights an item for the frame: drops a ray under it for the floor colour (blended into the ambient and giving
// the floor type), then picks up to three map lights for LightDir/LightRGB/ambvector and copies them into pInfo.
// Level fog (colour is 0xBBGGRR; the alpha is always 0x7f) and the bloom colour per level.
static void LevelFog(unsigned int rgb, float n, float f, float mn, float mx)
{
    EXBaseRenderEnv::SetOverrideFog(ex::RGBA(0x7f000000 | rgb), n, f, mn, mx, 1);
}

void SetLevelFogBloom()
{
    switch (gCurrentMap_GeoHash) {
    case 0x10000a5:     // RazeTheDepot
    case 0x10000e6:
    case 0x10000f3:
        LevelFog(0x244959, 60.0f, 200.0f, 0.5f, 0.3f);
        break;
    case 0x1000031:     // Future1
    case 0x1000135:     // Penthouse_RoS
        LevelFog(0x090606, 60.0f, 1000.0f, 0.7f, 0.5f);
        break;
    case 0x100005a:
    case 0x10000cf:
    case 0x10000e1:
    case 0x10000ea:
        LevelFog(0x5a50c3, 42.0f, 109.0f, 0.2f, 0.0f);
        break;
    case 0x1000070:     // MuseumInterior
        LevelFog(0x645044, 41.0f, 120.0f, 0.57f, 0.35f);
        break;
    case 0x1000073:     // GWarfare
    case 0x10000e4:
    case 0x1000160:
    case 0x1000161:
    case 0x1000162:
        LevelFog(0x3f2d5a, 41.0f, 120.0f, 0.62f, 0.0f);
        break;
    case 0x10000e9:     // Clanship_Tutorial
    case 0x1000118:
        LevelFog(0x706464, 50.0f, 142.0f, 0.5f, 0.0f);
        break;
    case 0x10000f2:     // Aliens
        LevelFog(0x2a2826, 20.0f, 72.0f, 1.0f, 0.0f);
        break;
    case 0x1000125:     // Labs_2
        LevelFog(0x2e2d26, 24.0f, 120.0f, 0.7f, 0.0f);
        break;
    case 0x10000f4:     // LaFamiglia
        LevelFog(0x91583c, 106.0f, 200.0f, 0.5f, 0.0f);
        break;
    case 0x100011c:     // potc
    case 0x100012a:
    case 0x100012c:
        LevelFog(0x1c1710, 100.0f, 630.0f, 1.0f, 0.5f);
        break;
    case 0x100014c:     // Front_End
        LevelFog(0x1e160a, 41.0f, 120.0f, 0.5f, 0.0f);
        break;
    case 0x100014e:     // DeadMan
        LevelFog(0x6b4939, 41.0f, 120.0f, 0.5f, 0.0f);
        break;
    case 0x100015b:
        LevelFog(0x402d20, 32.0f, 120.0f, 1.0f, 0.15f);
        break;
    default:
        EXBaseRenderEnv::SetOverrideFogEnabled(0);
        break;
    }

    switch (gCurrentMap_GeoHash) {
    case 0x10000f4:
        BloomColour = 0x14141414;
        break;
    case 0x1000031:
    case 0x100011c:
    case 0x100012a:
    case 0x100012c:
    case 0x1000135:
        BloomColour = 0x52303030;
        break;
    case 0x1000056:
    case 0x100006a:
    case 0x1000070:
    case 0x1000073:
    case 0x10000e4:
    case 0x1000136:
        BloomColour = 0x80191919;
        break;
    case 0x10000a5:
    case 0x10000e6:
    case 0x10000f3:
        BloomColour = 0x40402019;
        break;
    case 0x10000b1:
    case 0x10000b2:
    case 0x10000e1:
    case 0x10000ea:
    case 0x10000fa:
    case 0x100011d:
    case 0x100014e:
        BloomColour = 0x52202020;
        break;
    case 0x10000cd:
    case 0x100011a:
    case 0x100011f:
    case 0x100015b:
        BloomColour = 0x1e1e1e1e;
        break;
    case 0x10000f2:
    case 0x1000185:
        BloomColour = 0x8000202a;
        break;
    case 0x1000124:
        BloomColour = 0x3c3c3c3c;
        break;
    case 0x100014f:
        BloomColour = 0x80001015;
        break;
    default:
        BloomColour = 0x40404040;
        break;
    }
}

int SetLighting(EXItem *pItem, EXVector &ambient, EXExtRenderInfo *pInfo, s8 useLights, float rayLen, float ambScale,
                s8 isPlayer)
{
    pItem->RenderInfo().RenderFlagsRef().m_flags &= 0xfff3;
    const EXVector &pos = pItem->Position();
    EXVector start;
    EXVector end;
    int floorType = 0;
    XItemHandler_Player *pPlayer = XItemHandler_Player::GetInstance();
    if (pItem == gpPlayerItem[0] && pPlayer->m_Mode == 0x18) {
        // Clinging to a wall: cast along the gecko's own axis.
        EXMatrix m;
        m.set_identity();
        EXQuaternion q = pPlayer->GetGeckoAngle();
        m.set_rotate_quat(q);
        EXVector pt = pPlayer->GetGeckoPoint();
        EXVector v(0.0f, 0.0f, -0.5f, 0.0f);
        EXVector o;
        D3DXVec4Transform(&o, &v, &m);
        start = EXVector(o.x + pt.x, o.y + pt.y, o.z + pt.z, o.w + pt.w);
        v = EXVector(0.0f, 0.0f, rayLen, 0.0f);
        D3DXVec4Transform(&o, &v, &m);
        end = EXVector(o.x + pt.x, o.y + pt.y, o.z + pt.z, o.w + pt.w);
    } else {
        start = EXVector(pos.x, pos.y + 0.5f, pos.z, pos.w);
        end = EXVector(pos.x, pos.y - rayLen, pos.z, pos.w);
    }
    EXVector dir(end.x - start.x, end.y - start.y, end.z - start.z, end.w - start.w);

    EXRayCastTemp r;
    r.Set(start, dir, 0, 0, 0, 0);
    r.m_layer.m_flags |= 0x400;
    r.m_mask = 0x4008;
    r.m_bHit = 0;
    float one = 1.0f;
    if ((isPlayer && (theItemEnv.DoRayCastToContoured(r), r.m_bHit)) ||
        (theItemEnv.DoRayCastToMap(r, 0), r.m_bHit)) {
        floorType = r.m_face.GeoTexture()->GameFlags();
        float depth = (pos.y - r.Intersect().y) * 10.0f;
        if (depth < 4.0f) depth = 4.0f;
        EXVector c = r.m_face.GetFaceColourV(2, pos);
        float k = one / depth;
        ambient.x += k * (c.x - ambient.x);
        ambient.y += k * (c.y - ambient.y);
        ambient.z += k * (c.z - ambient.z);
        ambient.w += k * (c.w - ambient.w);
    }

    for (int i = 0; i < 3; i++) {
        LightDir[i] = EXVector(0.0f, 0.0f, 0.0f, 0.0f);
        LightRGB[i] = EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    }

    if (!useLights) {
        ambvector = EXVector(one, one, one, 0.0f);
    } else {
        EXLightInfo infos[6];
        for (int i = 0; i < 6; i++) {
            infos[i].m_Valid = 0;
            infos[i].m_29 = 0;
        }
        EXVector lp(pos.x, pos.y + one, pos.z, pos.w);
        EXVector up(0.0f, one, 0.0f, 0.0f);
        theItemEnv.CalcLightsV(infos, 6, lp, up, 0, EXMapOn(-1));

        // Third light: a fixed fill from above, spun round to follow the camera.
        EXVector &fill = LightDir[2];
        fill = EXVector(-1.0f, -1.0f, -0.5f, 0.0f);
        float len = EXSqrt(fill.x * fill.x + fill.y * fill.y + fill.z * fill.z);
        if (len > 1e-5f) {
            float inv = one / len;
            fill.x = inv * -1.0f;
            fill.y = inv * -1.0f;
            fill.z = inv * -0.5f;
            fill.w = inv * 0.0f;
        }
        LightRGB[2] = EXVector(ambient.x * 3.0f, ambient.y * 3.0f, ambient.z * 3.0f, ambient.w * 3.0f);
        if (LightRGB[2].x < 0.2f) LightRGB[2].x = 0.2f;
        if (LightRGB[2].y < 0.2f) LightRGB[2].y = 0.2f;
        if (LightRGB[2].z < 0.2f) LightRGB[2].z = 0.2f;

        float angle = XGetCamera(0)->Angle();
        float nx = (float)cos(angle) * fill.x - (float)sin(XGetCamera(0)->Angle()) * fill.z;
        angle = XGetCamera(0)->Angle();
        float sx = (float)sin(XGetCamera(0)->Angle());
        float oldX = fill.x;
        fill.x = nx;
        fill.z = -(sx * oldX + (float)cos(angle) * fill.z);

        int n = 0;
        for (int i = 0; n < 3 && i < 6; i++) {
            if (!infos[i].m_Valid) continue;
            LightRGB[n] = EXVector(infos[i].m_Colour.x * 0.5f, infos[i].m_Colour.y * 0.5f, infos[i].m_Colour.z * 0.5f,
                                   infos[i].m_Colour.w * 0.5f);
            LightRGB[n].w = one;
            LightDir[n] = EXVector(lp.x - infos[i].m_Position.x, lp.y - infos[i].m_Position.y,
                                   lp.z - infos[i].m_Position.z, lp.w - infos[i].m_Position.w);
            LightDir[n].w = 0.0f;
            float l2 = LightDir[n].x * LightDir[n].x + LightDir[n].y * LightDir[n].y + LightDir[n].z * LightDir[n].z +
                       LightDir[n].w * LightDir[n].w;
            if (l2 >= 0.0f) {
                float l = EXSqrt(l2);
                if (l > 1e-5f) {
                    float inv = one / l;
                    LightDir[n].x *= inv;
                    LightDir[n].y *= inv;
                    LightDir[n].z *= inv;
                    LightDir[n].w *= inv;
                }
            }
            n++;
        }

        ambvector = ambient;
        if (ambScale != one) {
            ambvector.x *= ambScale;
            ambvector.y *= ambScale;
            ambvector.z *= ambScale;
            ambvector.w *= ambScale;
            if (ambvector.x > one) ambvector.x = one;
            if (ambvector.y > one) ambvector.y = one;
            if (ambvector.z > one) ambvector.z = one;
        }
    }

    pInfo->SetLightData(LightDir[0], LightDir[1], LightDir[2], LightRGB[0], LightRGB[1], LightRGB[2], ambvector);
    return floorType;
}

void NightAndDay_Update()
{
    if (!gpPlayerItem[0] || !gpPlayerItem[0]->m_pItemHandler) return;
    XItemHandler_Player *pP = (XItemHandler_Player *)gpPlayerItem[0]->m_pItemHandler;

    gpPlayerItem[0]->RenderInfo().RenderFlagsRef().BitState(2, 0);

    EXVector amb = pP->m_AmbientColour;
    int floor = SetLighting(gpPlayerItem[0], amb, &pP->m_PlayerExtRenderInfo,
                            (!pP->GetCloak() && pP->m_Tint == 0 && !pP->m_bHidden), 10.0f, 0.5f, 1);
    pP->SetFloorType(floor);
    pP->m_AmbientColour = amb;

    for (int i = 0; i < 12; i++)
        pP->GetWeaponExtRenderInfo(i)->SetLightData(LightDir[0], LightDir[1], LightDir[2], LightRGB[0], LightRGB[1],
                                                    LightRGB[2], ambvector);

    if (pP->m_Tint == 0) {
        // Two vehicles per frame, round robin; skip ones already lit this frame.
        unsigned int lit[2];
        int nLit = 0;
        for (int n = 2; n != 0; n--) {
            Vehicle *v = Vehicle::GetFromIndexOnwards(s_LightVehicleIdx, 1);
            for (int j = 0; v; j++) {
                if (nLit <= j) {
                    EXVector3 col = v->m_lightColour;
                    EXVector tmp(col.x, col.y, col.z, 0.0f);
                    SetLighting(v->m_pItem, tmp, (EXExtRenderInfo *)&v->m_lights, 1, 4.0f, 1.5f, 0);
                    v->m_lightColour = col;
                    lit[nLit++] = v->m_index;
                    break;
                }
                if (v->m_index == lit[j]) v = 0;
            }
        }
    }

    VM_Data *vm = 0;
    switch (pP->m_Tint) {
    case 1: vm = &Thermal; break;
    case 2: vm = &Brain; break;
    case 3: vm = &Tech; break;
    default:
        theItemEnv.EnableDynLightRGBMult(0);
        theItemEnv.SetDynLightRGBMult(1.0f, 1.0f, 1.0f);
        return;
    }
    theItemEnv.SetDynLightRGBMult(vm->m_Colour.m_R * (1.0f / 255.0f), vm->m_Colour.m_G * (1.0f / 255.0f),
                                  vm->m_Colour.m_B * (1.0f / 255.0f));
    theItemEnv.EnableDynLightRGBMult(1);
    gpPlayerItem[0]->RenderInfo().RenderFlagsRef().BitState(8, 1);
    if (pP->m_Tint == 3) {
        gpPlayerItem[0]->RenderInfo().RenderFlagsRef().BitState(4, 1);
        gpPlayerItem[0]->RenderInfo().SetRGBAMult(0.5f, 1.0f, 1.0f, 1.0f);
    } else {
        gpPlayerItem[0]->RenderInfo().RenderFlagsRef().BitState(4, 0);
    }
}

// Lens flare for the sun, the level bloom colour and the rain streaks (192 drops in a storm).
void Rain_Update()
{
    if (LensFlareActive) {
        XItemHandler_Player *pPlayer = XItemHandler_Player::GetInstance();
        if (pPlayer->m_Tint == 0 || pPlayer->m_Tint == 1) {
            XLensFlare &fl = pPlayer->m_Tint == 0 ? Flare : ThermalFlare;
            EXVector cam = XGetCamera(0)->m_CameraPos;
            EXVector look = XGetCamera(0)->m_CameraLookAt;
            EXVector dir(look.x - cam.x, look.y - cam.y, look.z - cam.z, look.w - cam.w);
            EXVector at = XGetCamera(0)->m_CameraPos;
            EXVector sunPos(at.x + Sun.x, at.y + Sun.y, at.z + Sun.z, at.w + Sun.w);
            fl.Render2(gpGameWnd[0], sunPos, cam, dir);
            if (pPlayer->m_Tint == 0)
                ThermalFlare.m_Unkd8 = ThermalFlare.m_Unkd4;
            else
                Flare.m_Unkd8 = Flare.m_Unkd4;
        } else {
            Flare.m_Unkd8 = Flare.m_Unkd4;
            ThermalFlare.m_Unkd8 = ThermalFlare.m_Unkd4;
        }
    }

    gpGameWnd[0]->SetBloomColor(BloomColour);

    if (!Storm) {
        Raining = 0;
        for (XItem *p = gEnvList.GetFirst(); p; p = gEnvList.GetNext(p)) {
            XItemHandler_Environment *h = (XItemHandler_Environment *)p->m_pItemHandler;
            if (h->m_Type == 2 && (!h->m_Unk58 || FlagChannels[h->m_Unk58])) {
                Raining = 1;
                break;
            }
        }
    }
    if (!Raining) {
        if (Storm) Raining = 1;
        return;
    }

    XItemHandler_Player *pPlayer = XItemHandler_Player::GetInstance();
    if (Storm) {
        pPlayer->m_Wet = 1;
        if (pPlayer->GetCloak()) pPlayer->BreakCloak(0);
    }
    AddedThisFrame = 0;

    EXVector camPos = gpCameraItem[0]->Position();
    float camX = camPos.x;
    float camY = camPos.y;
    float camZ = camPos.z;
    float off = (float)fabs(sin(XGetCamera(0)->Elevation()) * 10.0f);
    camX = (float)(sin(XGetCamera(0)->Angle()) * off + camX);
    camZ = (float)(cos(XGetCamera(0)->Angle()) * off + camZ);

    EXPrimVert a;
    EXPrimVert b;
    if (pPlayer->m_Tint == 0) {
        a.m_Color = (unsigned int)RainColour & 0x00ffffff;
        b.m_Color = (unsigned int)RainColour;
    } else {
        a.m_Color = 0;
        b.m_Color = 0xff000000;
    }

    float v1x = RainVel.x * 2.0f, v1y = RainVel.y * 2.0f, v1z = RainVel.z * 2.0f, v1w = RainVel.w * 2.0f;
    float v2x = v1x * 2.0f, v2y = v1y * 2.0f, v2z = v1z * 2.0f, v2w = v1w * 2.0f;

    int state = theApp.GameStateCurrent()->m_State;
    s8 playing = state == 9;
    int count;
    int maxAdd;
    if (Storm) {
        count = 192;
        maxAdd = 40;
        if (playing) {
            // The wind wanders, limited to +-0.5.
            float x = RainVel.x + (g_EXRandClass.Randf() * 0.1f - 0.05f);
            RainVel.x = x >= 0.5f ? 0.5f : (x <= -0.5f ? -0.5f : x);
            float z = RainVel.z + (g_EXRandClass.Randf() * 0.1f - 0.05f);
            RainVel.z = z >= 0.5f ? 0.5f : (z <= -0.5f ? -0.5f : z);
        }
    } else {
        count = 96;
        maxAdd = 20;
    }
    if (RainLimit >= 0) {
        if (count > RainLimit) count = RainLimit;
        int next = RainLimit - 1;
        RainLimit = next > 0 ? next : 0;
    }

    float floorY = camY - 10.0f;
    for (int i = 0; i < count; i++) {
        EXVector &d = Wet[i];
        if (playing) {
            d.x -= v1x;
            d.y -= v1y;
            d.z -= v1z;
            d.w -= v1w;
            if (floorY > d.y || WetYMin[i] > d.y) {
                if (AddedThisFrame < maxAdd) {
                    d.x = g_EXRandClass.Randf() * 14.0f + camX - 7.0f;
                    d.y = g_EXRandClass.Randf() * 5.0f + camY + 7.0f;
                    d.z = g_EXRandClass.Randf() * 14.0f + camZ - 7.0f;
                    if (!Storm)
                        Rain_Add(i);
                    else
                        AddedThisFrame++;
                } else {
                    d.y = -4000.0f;
                }
            }
        }
        a.x = d.x;
        a.y = d.y;
        a.z = d.z;
        a.w = d.w;
        b.x = d.x - v2x;
        b.y = d.y - v2y;
        b.z = d.z - v2z;
        b.w = d.w - v2w;
        gpGameWnd[0]->DrawLine(a, b);
    }
}
