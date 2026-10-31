#ifndef FEHUD_H
#define FEHUD_H
#include "types.h"
#include "RGBA.h"
#include "EXRect.h"
#include "EXString.h"
#include "FEWnd.h"
#include "FEMathmos.h"
#include "EXVector.h"
#include "ScanInfo.h"
#include "EXExtRenderInfoStandIn.h"

class XItem;
class Actor;

namespace FEWeaponCat {
enum Flags { WC_NONE = 4 };
}

// One weapon slot of the HUD weapon selector (FEHud.obj); sizeof 0x28.
class FEWeapon {
public:
    FEWeapon() { Init(); }
    FEWeapon(FEWeaponCat::Flags cat, unsigned int hash, unsigned int a, unsigned int text, unsigned int text2,
             unsigned int b, unsigned int c, int d, signed char e, signed char f, signed char g)
    {
        m_Cat = cat;
        m_Hash = hash;
        m_Icon = a;
        m_Text = text;
        m_Text2 = text2;
        m_Unk14 = b;
        m_Unk18 = c;
        m_Unk20 = d;
        m_UsesAmmo = e;
        m_Shown = 1;
        m_Unk25 = f;
        m_Unk26 = g;
    }
    void Init()
    {
        m_Cat = (FEWeaponCat::Flags)4;
        m_Hash = 0xfeffffff;
        m_Icon = 0;
        m_Text = 0x5100005a;
        m_Text2 = 0x5100005a;
        m_Unk20 = 0;
        m_UsesAmmo = 0;
        m_Shown = 0;
        m_Unk25 = 1;
        m_Unk26 = 1;
    }
    FEWeaponCat::Flags m_Cat;  // 0x00
    unsigned int m_Hash;       // 0x04 weapon hash (0xfeffffff empty)
    unsigned int m_Icon;       // 0x08 icon texture (HUD file)
    unsigned int m_Text;       // 0x0c
    unsigned int m_Text2;      // 0x10
    unsigned int m_Unk14;      // 0x14
    unsigned int m_Unk18;      // 0x18
    signed char m_Shown;       // 0x1c listed in the weapon selector (UpdateWeaponSelect)
    int m_Unk20;               // 0x20
    signed char m_UsesAmmo;    // 0x24 shows an ammo count; hidden when empty unless equipped
    signed char m_Unk25;       // 0x25
    signed char m_Unk26;       // 0x26 always available
};

// One waypoint marker queued for RenderWaypoints (FEHud.obj); sizeof 0x28.
struct FEWaypoint {
    FEWaypoint() {}
    int m_Type;         // 0x00
    int m_Unk4;         // 0x04
    XItem *m_pItem;     // 0x08
    EXVector m_Pos;     // 0x0c
    int m_ScreenX;      // 0x1c
    int m_ScreenY;      // 0x20
    s8 m_bHit;          // 0x24 line of sight to the camera is blocked
    u8 m_Pad25[3];
};

// Radar placement (FEHud.obj); sizeof 0x2c. NEAR: ctor schedules the first int zero differently.
struct FERadarInfo {
    FERadarInfo()
    {
        m_Range = 0.0f;
        m_Centre.x = 0;
        m_Centre.y = 0;
        m_Radius = 0.0f;
        m_Radius2 = 0.0f;
        m_PlayerPos.x = 0.0f;
        m_PlayerPos.y = 0.0f;
        m_PlayerPos.z = 0.0f;
        m_PlayerPos.w = 0.0f;
        m_pObject = 0;
        m_Small = 0;
    }
    float m_Range;        // 0x00 world range (25.0)
    EXPoint m_Centre;     // 0x04
    float m_Radius;       // 0x0c
    float m_Radius2;      // 0x10 inner radius
    EXVector m_PlayerPos; // 0x14
    FEObject *m_pObject;  // 0x24 radar object
    signed char m_Unk28;  // 0x28
    signed char m_Small;  // 0x29 three-quarter size (widescreen)
};

// In-game HUD window (FEHud.obj, FEHudVision.obj). Singleton, sizeof 0x22ec.
class FEHud : public FEWnd {
public:
    virtual ~FEHud();
    virtual signed char Draw();
    virtual void ContentCreate();
    virtual void ContentUpdate();
    virtual char *DebugName() { return "WndHud"; }

    signed char IsWeaponSelectorOpen() { return m_Unkb90[0]; }  // App.obj copy
    void DisablePlayer(unsigned int buttons, int frames);       // inline in App.cpp (needs the player)
    void UpdateDisablePlayer()
    {
        if (m_DisablePlayer > 0) m_DisablePlayer--;
    }
    signed char IsAnyScanOpen()
    {
        if (m_ThermoScanOpen) return 1;
        if (m_NeuroScanOpen) return 1;
        if (m_TechScanOpen) return 1;
        return 0;
    }
    // Header inline in retail (emitted in FEHud.obj); defined in FEHud.cpp because its function-local
    // statics would shift the $E numbering of every unit that includes this header.
    static ex::RGBA TintColour(int tint);
    FEWeapon GetWeaponPreset(unsigned int hash, int a, unsigned int *p);
    FEWeapon *GetWeapon(unsigned int hash);
    signed char GetRadarInfo(FERadarInfo &info);
    EXRect GetRadarRect(FERadarInfo &info);
    int WeaponIndex(unsigned int hash);
    void WeaponArrayDestroy();
    void InitWaypoints();
    void CommitVisionMode(int mode);
    void CloseMimicry();
    static FEHud *Instance(signed char create);
    void RegisterBossHit(int id)
    {
        m_BossHitID = id;
        m_BossHitTimer = 5.0f;
    }
    signed char IsWeaponAvailable(unsigned int hash);
    signed char WeaponArrayCreate(unsigned int *p, unsigned int n);
    signed char WeaponArrayDel(unsigned int hash, FEWeaponCat::Flags cat);
    void RenderWaypoints();
    EXRect TextRect();
    void MaskUpdate();
    signed char WeaponArrayAdd(unsigned int hash, FEWeaponCat::Flags cat);
    void DrawWaypoint(XItem *pItem);
    void DrawAffiliation(Actor *pActor);
    void MaskCreate();
    void NotifyCutscene(signed char b);
    signed char GetScanInfo(int i, ScanInfo *p);
    // Header inline (FEHudVision.obj copy); body in FEHudVision.cpp, its only user, since this
    // header does not see FEObject.
    EXRect MakeRect(FEObject *pObj, unsigned int tl, unsigned int br);

protected:
    void HealthStringUpdate(signed char b);
    void EnergyStringUpdate(signed char b);
    ex::RGBA GetVisibilityAlpha();
    void UpdateBottomLeft();
    void UpdateBottomRight();
    void UpdateBossBar();
    void RenderWeaponData(int i, unsigned int a, unsigned int b, FEObject *pObj, EXW::TextAlign align);
    void UpdateTopLeft();
    void UpdateTopRight();
    void UpdateFirstPerson();
    void UpdateTargeting();
    void UpdateThermoScan();
    void UpdateNeuroScan();
    void UpdateTechScan();
    void UpdateWeaponSelect();

public:
    FEWeapon m_Weapons[6][8];          // 0x3fc
    u32 m_Unkb7c[4];                   // 0xb7c TODO
    int m_DisablePlayer;               // 0xb8c frames left with the player disabled
    u8 m_Unkb90[4];                    // 0xb90 [0] weapon selector open (UpdateWeaponSelect)
    int m_Unkb94;                      // 0xb94 -1 on create
    int m_Unkb98;                      // 0xb98 8 on create
    FEMathmos m_Mathmos[4];            // 0xb9c
    FEMathmos m_Mathmosc3c;            // 0xc3c
    FEMathmos m_Fade;                  // 0xc64
    FEMathmos m_Mathmosc8c;            // 0xc8c
    FEMathmos m_Tint;                  // 0xcb4 vision-mode tint
    FEMathmos m_Mathmoscdc;            // 0xcdc
    EXString m_Strings[4];             // 0xd04
    EXString m_Strings2[2];            // 0xd14
    signed char m_MimicryOpen;         // 0xd1c
    u8 m_Unkd1d[0xd20 - 0xd1d];        // 0xd1d TODO
    float m_Unkd20;                    // 0xd20 (Player.obj SendVMMessages sets 59999940)
    signed char m_Unkd24;              // 0xd24 (Player.obj SendVMMessages clears)
    signed char m_ThermoScanOpen;      // 0xd25
    signed char m_ThermoScanFilled;    // 0xd26 page text built (once the page script has played in)
    signed char m_NeuroScanOpen;       // 0xd27
    signed char m_NeuroScanFilled;     // 0xd28
    signed char m_TechScanOpen;        // 0xd29
    signed char m_TechScanFilled;      // 0xd2a
    u8 m_Unkd2b;
    int m_LockTarget;                  // 0xd2c locked-on actor id, or item pointer for other lock types (UpdateTargeting)
    FERadarInfo m_RadarInfo;           // 0xd30
    u32 m_SelectStartWeapons[4];       // 0xd5c equipped weapons when the selector opened
    u32 m_SelectWeapons[4];            // 0xd6c selector choices, bound to each category row
    signed char m_ActionsChanged;      // 0xd7c pad GetActionsChanged this frame
    signed char m_Unkd7d;              // 0xd7d
    u8 m_Rumble;                       // 0xd7e last applied rumble setting
    u8 m_Unkd7f;
    float m_HealthDisp;                // 0xd80 displayed health (eases toward the player's)
    float m_EnergyDisp;                // 0xd84
    FEWaypoint m_Waypoints[2][64];     // 0xd88 double-buffered
    int m_nWaypoints[2];               // 0x2188
    int m_WaypointBuf;                 // 0x2190 buffer being filled
    signed char m_Unk2194;             // 0x2194
    u8 m_Unk2195[3];
    ScanInfo m_ScanInfo;               // 0x2198 cleared on create
    int m_LockState;                   // 0x224c 0 no lock, 1 reticle closing in, 2 locked (UpdateTargeting)
    XItem *m_pMask;                    // 0x2250 predator mask overlay item (MaskCreate)
    signed char m_MaskActive;          // 0x2254
    u8 m_Unk2255[3];
    EXExtRenderInfo m_ExtRenderInfo;   // 0x2258
    int m_BossHitID;                   // 0x22d8 boss actor shown on the hud (RegisterBossHit)
    float m_BossHitTimer;              // 0x22dc
    int m_Visibility;                  // 0x22e0 0 hidden, 1 could be seen, 2 seen
    float m_VisibilityTimer;           // 0x22e4
    int m_SelectCategory;              // 0x22e8 category row focused when the selector last closed

private:
    FEHud();
    static FEHud *m_pInstance;
};
inline FEHud *FEGetHud(signed char create) { return FEHud::Instance(create); }
#endif
