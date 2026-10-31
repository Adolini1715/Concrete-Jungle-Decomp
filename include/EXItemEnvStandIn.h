#ifndef EXITEMENVSTANDIN_H
#define EXITEMENVSTANDIN_H
#include "types.h"
#include "EXMapOn.h"

class EXItemAnimator;
class EXItemAnimator_Map;
class EXMatrix;
class EXRenderInfo;
class EXMapZoneInfo;
class EXVector;
class EXCollisionScoopTemp;
class EXItem;
class EXGeoPath;
class EXDList;
class EX6DManager;
struct EXGrabPointTemp;
struct EXGrabPointArrayTemp;
struct EXRayCastTemp;

// TODO stand-in: item environment (EXItemEnv.obj, engine; not this area). sizeof 0xa4. TODO layout
struct EXShadowTemp;
struct EXLightInfo;
class EXItemEnv {
public:
    virtual ~EXItemEnv();
    virtual void Initialise() = 0;
    virtual void StartMusic(unsigned int hash, unsigned int flags, int a, int b);
    virtual void UpdateItems_UpdateCode();
protected:
    virtual EXItemAnimator *CreateSky(EXItemAnimator_Map *pMap, unsigned int hash, const EXMapZoneInfo *pZone) const;
    virtual void ProcessSky(EXItemAnimator *pSky) const;
    virtual void RenderSky(EXItemAnimator *pSky, const EXMatrix &mtx, const EXRenderInfo &info, const EXMapZoneInfo *pZone) const;
public:
    EXItemEnv(unsigned int a, unsigned int b);
    static void Shutdown();  // EXBaseApp::ShutdownItemSystem
    EXMapOn GetMapOn(const EXVector &pos) const;
    EXMapOn GetMapOn(unsigned int hash) const;
    void AddItem(EXItem *p, unsigned int list);  // verified call (FX_ClayPigeon.obj)
    s8 IsZoneVisible(EXMapOn m) const;           // verified call (ScriptRunner.obj)
    u8 m_Unk4[0x65 - 4];    // TODO layout
    u8 m_RenderFlagsHi;     // 0x65 high byte of the render info flags (m_renderInfo at 0x4c)
    u8 m_Unk66[2];
    u16 m_ShadowDensity;    // 0x68 render info shadow density
    u8 m_Unk6a[0x80 - 0x6a];
    EX6DManager *m_p6DManager;  // 0x80 (verified: Player.obj RollActorRagDoll)
    u8 m_Unk84[0x90 - 0x84];
    EXDList *m_pItemLists;  // 0x90 item lists (verified: Bullet_Explosion Remove_CSMF)
    u8 m_Unk94[0xa4 - 0x94];
    // Appended by the player/camera agent (XItemPhysics.obj, FEManager.obj inlines).
    void DoCollisionScoopToMap(EXCollisionScoopTemp &t) const;
    void DoCollisionScoopToContoured(EXCollisionScoopTemp &t, EXItem *pItem) const;
    static EXItemEnv *GetItemEnv() { return m_pTheItemEnv; }
    EXGeoPath *GetPath(int hash) const;  // appended by the actor agent (AIControl.obj)
    EXGeoPath *GetPath(const EXVector &p, unsigned int flags) const;  // appended (Bullet.obj Create2)
    void RenderShadows() const;  // appended (GameWnd.obj DrawShadow)
    void RemoveAllMaps();
    EXItemAnimator_Map *AddMap(EXGeoFile *pFile, u32 hash, EXItemAnimator_Map *pMap);
    void RemoveAllAddedItems();
    void RemoveAllDynLights() const;
    void UpdateItems_LastFrame();  // appended (verified call in GameWnd.obj WaitForIsland)
    void DoShadowCastToMap(EXShadowTemp &t) const;  // appended (verified call in Player.obj UpdateDroneToGrabHand)
    class EXGeoPathArray *GetPaths(EXMapOn m) const;  // appended (Camera.obj SetCamSpline)
    struct EXGeoCamera *GetCamera(const EXVector &p) const;  // appended (Camera.obj SetCamFixed)
    EXVector CalcLightsV(EXLightInfo *pInfo, int num, const EXVector &p, const EXVector &n, s8 b, EXMapOn m) const;  // appended (CameraScript.obj LightThreadItem)
    void DoGrabPointToMap(EXGrabPointTemp &t) const;  // appended (verified call in Player.obj GetGrabPoint)
    void DoGrabPointToMap(EXGrabPointArrayTemp &t) const;  // appended (verified call in Player.obj CheckForShimmySpace)
    void DoRayCastToContoured(EXRayCastTemp &t) const;  // appended (verified call in Player.obj MoveCheckForSticky)
    void DoRayCastToMap(EXRayCastTemp &t, unsigned int flags) const;  // appended (Player.obj MoveCheckForSticky)
    void RemoveItem(EXItem *p);  // appended (verified call in Traffic.obj ~TrafficLight)
    void EnableDynLightRGBMult(s8 b);  // appended (NightAndDay.obj calls)
    void SetDynLightRGBMult(float r, float g, float b);
    // Appended (verified calls in App.obj XApp::UpdateGame).
    void UpdateItems_Collision() const;
    void ProcessEnvViews();
    void UpdateItems_Physics() const;
    void UpdateItems_InterObjCollision();
protected:
    static EXItemEnv *m_pTheItemEnv;
};
inline EXItemEnv *EXGetWorld() { return EXItemEnv::GetItemEnv(); }
#endif
