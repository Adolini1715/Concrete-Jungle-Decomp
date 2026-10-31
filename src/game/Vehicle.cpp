#include "FEText.h"
#include "Vehicle.h"
#include "TrafficSystem.h"
#include "EXMemory.h"
#include "EXItem.h"
#include "EXItemEnv.h"
#include "EXItemAnimator_Script.h"
#include "XItemHandler_Camera.h"
#include "XItemHandler_Player.h"
#include "Actor.h"
#include "EXString.h"
#include "XApp.h"
#include "EXSphere.h"
#include "EXCapsule.h"
#include "EXDatum.h"
#include "EXTriangle.h"
#include "EXRayCastTemp.h"
#include "EXDistance.h"
#include "EXGeoFile.h"
#include "ScanInfo.h"
#include "XItemHandler_ScriptBase.h"
#include "FERadar.h"
#include "EXItemAnimator_Entity.h"
#include "ActorUtil.h"

class XItemHandler_Objective {  // TODO stand-in (Objective.obj). TODO layout.
public:
    void ProcessObjectDestroyed();
    u8 m_unk0[0x58];
    u8 m_58;            // 0x58 TODO
    u8 m_59;
    u8 m_5a;            // 0x5a TODO
    u8 m_unk5b[0xb8 - 0x5b];
    u8 m_state;         // 0xb8 TODO
};

extern unsigned int gCurrentMap_GeoHash;
class XItemEnv;
extern XItemEnv *pWorld;
extern XItemEnv theItemEnv;
// XItemEnv derives from EXItemEnv at offset 0 (XItem.h still uses the EXItemEnv stand-in).
#define WORLD ((EXItemEnv *)(void *)pWorld)
#define ITEMENV ((EXItemEnv *)(void *)&theItemEnv)
extern unsigned int indicate;
void Mission_Success();
unsigned short Objective_GetMissionState();
void Objective_NotifyVehicleDestroyed(Vehicle *);
class XItem;
class FEObject;

EXFixedArray<unsigned int, 6> CollisionDatums = { { 0x10000083, 0x10000084, 0x10000085, 0x10000086, 0x10000087, 0 } };

EXRuntimeClass *Vehicle::_GetBaseClass() { return EXRUNTIME_CLASS(Vehicle); }
const EXRuntimeClass Vehicle::classVehicle = { 0, "Vehicle", sizeof(Vehicle), (void *(__cdecl *)())Vehicle::CreateObject };

void Vehicle::removeItemFromWorld()
{
    TrafficBoxJunction *pBox = m_move.getInBoxJunction();
    if (pBox) {
        pBox->removeVehicle(this);
        m_move.setInBoxJunction(0);
    }
    EXItem *pItem = m_pItem;
    if (pItem) {
        if (WORLD->ItemList(0).IsMember(&pItem->m_node)) WORLD->RemoveItem(pItem);
        m_pItem = 0;
        delete pItem;
    }
}

void Vehicle::freeAllocs()
{
}

void Vehicle::landedOn()
{
    if (gCurrentMap_GeoHash == 0x1000054 && m_pTypeObject->getType() == Vcl::SPORTS2) Mission_Success();
}

void Vehicle::objectiveInit(XItemHandler_Objective *pObjective, unsigned int hash)
{
    m_pObjective = pObjective;
    m_objectiveHash = hash;
    m_objectiveState = 0;
}

void Vehicle::objectiveUpdate()
{
    s8 bGo = 0;
    switch (m_pObjective->m_state) {
    case 0:
    case 2:
        bGo = 1;
        break;
    }
    switch (Objective_GetMissionState()) {
    case 0:
    case 2:
        break;
    default:
        if (!bGo) return;
        break;
    }
    m_flags |= 0x21;
    m_unk708 = 0;
    m_move.initRoute(0);
}

Vehicle *Vehicle::GetNext()
{
    unsigned int i = m_index + 1;
    TrafficPopulace &pop = TrafficGetPopulace();
    unsigned int n = pop.getNumVehicles();
    for (; i < n; i++) {
        Vehicle *p = pop.getVehicle(i);
        if (p->isInit()) return p;
    }
    return 0;
}

void Vehicle::initIndicator(unsigned int type, float time)
{
    indicate = type;
    m_indicatorTime = time;
    switch (type) {
    case 0:
        clrFlag(8);
        if (!(m_flags & 0x10)) {
            setFlag(0x10);
            m_indicatorTimer = 0.0f;
        }
        break;
    case 1:
        clrFlag(0x10);
        if (!(m_flags & 8)) {
            setFlag(8);
            m_indicatorTimer = 0.0f;
        }
        break;
    default:
        m_indicatorTime = 0.0f;
        clrFlag(0x18);
        break;
    }
}

inline void setItemAlpha(EXItem *pItem, float a)
{
    if (pItem) {
        pItem->RenderInfo().EnableAlphaMult(a < 1.0f);
        EXVector c = pItem->RenderInfo().RGBAMult();
        pItem->RenderInfo().SetRGBAMult(c.x, c.y, c.z, a);
        if (a != 0.0f) pItem->SetHasRender();
        else pItem->ClrHasRender();
    }
}

void Vehicle::updateTransparency()
{
    if (m_bFading == 1) {
        m_alpha -= m_alphaRate * XApp::GameFrameRateMul();
        float zero = 0.0f;
        m_alpha = max(zero, m_alpha);
    } else {
        m_alpha = 1.0f;
        float dist = (XGetCamera(0)->GetItem()->Position() - m_pItem->Position()).LengthXYZ();
        dist -= m_pTypeObject->m_zDim;
        float fade = XGetCamera(0)->m_Zoom * 90.0f;
        if (dist > fade) {
            if (dist >= fade + 25.0f) m_alpha = 0.0f;
            else m_alpha -= 1.0f / (25.0f / (dist - fade));
        }
    }
    if (!m_pExplosion) {
        setItemAlpha(m_pItem, m_alpha);
        return;
    }
    setItemAlpha(m_pItem, 0.0f);
    setItemAlpha(m_pExplosion, m_alpha);
}

void Vehicle::DeleteMuzzleScript()
{
    if (m_pMuzzleScript) {
        m_pMuzzleScript->ItemFlags().m_flags |= 0x10;
        m_pMuzzleScript = 0;
    }
}

Vehicle *Vehicle::GetFirst()
{
    if (!TrafficIsInit()) return 0;
    TrafficPopulace &pop = TrafficGetPopulace();
    if (!pop.getNumVehicles()) return 0;
    Vehicle *p = pop.getVehicle(0);
    if (p->isInit()) return p;
    return p->GetNext();
}

Vehicle *Vehicle::GetNext(const EXVector &pos, float range)
{
    float r2 = range * range;
    unsigned int i = m_index + 1;
    TrafficPopulace &pop = TrafficGetPopulace();
    unsigned int n = pop.getNumVehicles();
    if (i >= n) return 0;
    for (; i < n; i++) {
        Vehicle *p = pop.getVehicle(i);
        if (p->isInit()) {
            EXVector vp(p->m_currPos.m_pos.x, p->m_currPos.m_pos.y, p->m_currPos.m_pos.z, 0.0f);
            EXVector d = pos - vp;
            if (r2 > d.LengthXZSquared()) return p;
        }
    }
    return 0;
}

// NEAR: rotation stores go through the stack copy instead of registers.
signed char Vehicle::ApplyAttackDamage(unsigned int attacker, float damage)
{
    if (m_attackedCountDown != 0.0f && attacker == (unsigned int)m_attackedBy) return 0;
    m_attackedBy = attacker;
    m_attackedCountDown = 0.4f;
    if (!(0.0f >= m_health)) {
        m_health -= damage;
        if (m_health <= 0.0f) {
            m_health = 0.0f;
            if ((m_vehFlags & 2) && m_pObjective) {
                if ((m_pObjective->m_58 & 0x80) || (m_pObjective->m_5a & 0x80)) {
                    Objective_NotifyVehicleDestroyed(this);
                    m_pObjective->ProcessObjectDestroyed();
                }
            }
            m_pItem->UpdateGroups(ITEMENV, 0);
            {
                EXVector pos(m_currPos.m_pos.x, m_currPos.m_pos.y, m_currPos.m_pos.z, 0.0f);
                m_pExplosion = (EXItem *)(void *)XItemHandler_ScriptBase::Explosion_Create(pos, 0x40004e1, 10, 0x7d, 0xe, 0, 1, 0, 0);
            }
            if (m_pExplosion) {
                EXVector rot;
                currRot_get(rot);
                m_pExplosion->m_rotation = rot;
                m_pExplosion->Invalidate();
                float speed = m_move.getSpeed();
                float minSpeed = 50.0f;
                m_unk7d4 = max(speed, minSpeed) * 0.1f + m_currPos.getFrontLaneDist(0);
            }
            m_alphaRate = m_alpha * 0.0125f;
            m_bFading = 1;
            setFlag(1);
            XGetCamera(0)->VehicleRemoved(this);
            XItemHandler_Player::Instance()->VehicleRemoved(this);
            Actor::VehicleRemoved(this);
        }
    }
    return 1;
}

void Vehicle::initHealthIndicator()
{
}

// NEAR: retail loads pos.y into a register before adding (operand order).
void Vehicle::updateHealthIndicator()
{
    if (m_pHealthIndicator) {
        EXVector rot(0.0f, XGetCamera(0)->Angle(), 0.0f, 0.0f);
        m_pHealthIndicator->m_rotation = rot;
        m_pHealthIndicator->Invalidate();
        EXVector pos(m_currPos.m_pos.x, m_currPos.m_pos.y, m_currPos.m_pos.z, 0.0f);
        pos.y = m_pTypeObject->getYDim() + pos.y + 0.3f;
        m_pHealthIndicator->SetPosition(pos);
        ((EXItemAnimator_Script *)m_pHealthIndicator->GetAnimator())->SetObjectFrame(101.0f - m_health / m_maxHealth * 100.0f);
        m_pHealthIndicator->SetHasRender();
        setItemAlpha(m_pHealthIndicator, m_alpha);
    }
}

void Vehicle::setAttachmentRender(int i, signed char bOn)
{
    EXItemAnimator_Entity *p = m_attachments[i];
    if (p) {
        if (bOn) ((EXPlacementItem *)p)->SetHasRender();
        else ((EXPlacementItem *)p)->ClrHasRender();
    }
}

signed char Vehicle::isOnScreen()
{
    XItemHandler_Player *pPlayer = XItemHandler_Player::Instance();
    if (pPlayer) {
        EXItem *pItem = m_pItem;
        if ((EXItem *)(void *)pPlayer->GetStandOn() == pItem) return 1;
    }
    EXVector c(m_currPos.m_pos.x, m_currPos.m_pos.y, m_currPos.m_pos.z, 0.0f);
    EXSphere s;
    s.Set(c, m_pTypeObject->getRadius());
    return XGetCamera(0)->BoundingVolumeOnScreen(s) != 0;
}

signed char Vehicle::isOnScreen(unsigned int type, EXVector &pos)
{
    if (type >= Vcl::NUM_TYPES) type = vehicleTypeFromHash(type);
    VehicleTypeBase *pType = vehicleTypeObjFromType(type);
    EXSphere s;
    s.Set(pos, pType->getRadius());
    return XGetCamera(0)->BoundingVolumeOnScreen(s) != 0;
}

Vehicle *Vehicle::GetFromIndexOnwards(unsigned int &index, signed char bOnScreen)
{
    if (!TrafficIsInit()) return 0;
    TrafficPopulace &pop = TrafficGetPopulace();
    if (!pop.getNumVehicles()) return 0;
    if (index >= pop.getNumVehicles()) index = 0;
    unsigned int start = index;
    do {
        Vehicle *p = pop.getVehicle(index);
        if (p->isInit()) {
            if (!bOnScreen || p->isOnScreen()) {
                index++;
                return p;
            }
        }
        if (++index == pop.getNumVehicles()) index = 0;
    } while (index != start);
    return 0;
}

// Not in the map (always inlined): the vehicle owning an item, if any.
inline Vehicle *VehicleFromItem(EXItem *pItem)
{
    if (pItem) {
        EXItem *pOwner = pItem->GetXItemOwner();
        if (pOwner && pOwner->IsKindOf(EXRUNTIME_CLASS(Vehicle))) return (Vehicle *)(void *)pOwner;
    }
    return 0;
}

signed char Vehicle::ScanCallback(signed char, FEObject *, ScanInfo *pInfo)
{
    Vehicle *pVehicle = VehicleFromItem(pInfo->m_pItem);
    pInfo->m_a4 = pVehicle->m_health * 0.001f;
    int health = (int)pVehicle->m_health;
    int zero = 0;
    pInfo->m_text.Format("Health : %d", max(zero, health));
    return 1;
}

void Vehicle::reset()
{
    removeItemFromWorld();
    m_alpha = 1.0f;
    m_index = 0x99;
    m_flags = 0;
    m_vehFlags = 0;
    m_bFading = 0;
    m_alphaRate = 0.0f;
    m_move.reset(this);
    m_sfx.reset(this);
    m_currPos.reset(this);
    m_updateCount = 0xffffffff;
    m_unk6f8 = 0;
    m_pObjective = 0;
    if (m_pTypeObject) {
        m_pTypeObject->deLoadSubFile();
        m_pTypeObject = 0;
    }
    DeleteMuzzleScript();
    m_unk708 = 0;
    m_health = 1000.0f;
    m_maxHealth = 1000.0f;
    m_unk730 = 0;
    m_attackedBy = -1;
    m_attackedCountDown = 0.0f;
    for (int i = 0; i < 17; i++) m_attachments[i] = 0;
    m_lightColour.x = 1.0f;
    m_lightColour.y = 1.0f;
    m_lightColour.z = 1.0f;
    m_unk7c8 = 0.0f;
    m_unk7cc = 0.0f;
}

void Vehicle::remove()
{
    XGetCamera(0)->VehicleRemoved(this);
    XItemHandler_Player::Instance()->VehicleRemoved(this);
    Actor::VehicleRemoved(this);
    if (m_pHealthIndicator) {
        ITEMENV->RemoveItem(m_pHealthIndicator);
        delete m_pHealthIndicator;
        m_pHealthIndicator = 0;
    }
    if (m_pExplosion) {
        ITEMENV->RemoveItem(m_pExplosion);
        delete m_pExplosion;
        m_pExplosion = 0;
    }
    reset();
}

// NEAR: rotation stores go through a pointer (add eax, 0xd8) and register choice differs.
void Vehicle::PlayMuzzleScript(int hash)
{
    EXDatumTemp d;
    if (!m_pItem->GetAnimatorDatum(0x10000037, d, 4, 0)) return;
    DeleteMuzzleScript();
    m_pMuzzleScript = new EXItemAnimator_Script;
    if (!m_pMuzzleScript) return;
    EXGeoFile *pFile = EXGeoFile::GetGeoFile(0x1000016);
    if (!pFile) return;
    if (!m_pMuzzleScript->Initialise(pFile, hash)) {
        m_pMuzzleScript->ItemFlags().BitSet(0x10);
        m_pMuzzleScript = 0;
        return;
    }
    m_pMuzzleScript->SetPosition(EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    m_pMuzzleScript->m_rotation = EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    m_pMuzzleScript->Invalidate();
    m_pMuzzleScript->RenderFlags().BitSet(1);
    m_pItem->AddAnimator(m_pMuzzleScript);
}

// NEAR: rotation store sequence (retail loads y,z,w,x and stores without a pointer add).
void Vehicle::UpdateMuzzleScript()
{
    if (!m_pMuzzleScript) return;
    EXDatumTemp d;
    if (!m_pItem->GetAnimatorDatum(0x10000037, d, 4, 0)) return;
    EXVector rot;
    d.m_rotation.GetRotation(rot, 4);
    m_pMuzzleScript->SetPosition(d.m_position);
    m_pMuzzleScript->m_rotation = rot;
    m_pMuzzleScript->Invalidate();
    if (m_pMuzzleScript->m_11a) DeleteMuzzleScript();
}

int Vehicle::CheckCollision(const EXLineSeg &seg, float radius, EXVector &out)
{
    EXCapsule c;
    c.Set(seg.m_Origin, seg.m_Direction, radius);
    return CheckCollision(c, out);
}

// NEAR: vector-copy scheduling and squared-range operand order differ.
Vehicle *Vehicle::GetFirst(const EXVector &pos, float range)
{
    Vehicle *p = GetFirst();
    if (!p) return 0;
    EXVector vp(p->m_currPos.m_pos.x, p->m_currPos.m_pos.y, p->m_currPos.m_pos.z, 0.0f);
    EXVector d = pos - vp;
    float len = d.LengthXZSquared();
    if (len < range * range) return p;
    return p->GetNext(pos, range);
}

signed char Vehicle::GetScanInfo(int type, ScanInfo *pInfo)
{
    switch (type) {
    case 2:
        pInfo->m_a0 = 0x400057c;
        pInfo->m_Text[0][0] = 0x51000555;
        pInfo->m_Text[0][1] = 0x51000556;
        pInfo->m_Text[1][0] = 0x51000557;
        break;
    default:
        pInfo->m_a0 = 0;
        break;
    }
    pInfo->m_a4 = m_health * 0.001f;
    s8 r = pInfo->m_a0 > 0;
    pInfo->m_ac = r;
    pInfo->m_pCallback = ScanCallback;
    return r;
}

// NEAR: one EXBaseItem flag store scheduled after push edi.
Vehicle::Vehicle()
{
    SetItem(0);
    m_pTypeObject = 0;
    m_move.setInBoxJunction(0);
    m_pMuzzleScript = 0;
    m_pHealthIndicator = 0;
    m_pExplosion = 0;
    reset();
}

Vehicle::~Vehicle()
{
    remove();
}

Vehicle *Vehicle::CreateObject() { return new Vehicle; }

extern signed char bIDontKnowWhyTheRadarCrashesTheFrameAfterPauseButThisFlagShouldGetRoundItHopefully;
void Objective_NotifyPlayerSeen(Vehicle *);
signed char checkForAndUnspawnVehicle(Vehicle *);

// Looks for the player's head (not when cloaked); a sighting counts towards the objective.
signed char Vehicle::CheckSeePlayer()
{
    if (Player_IsCloaked()) return 0;
    EXVector head = Player_GetHeadPos();
    signed char seen = CanSeePoint(head, 75.0f);
    if (seen) Objective_NotifyPlayerSeen(this);
    return seen;
}

// Attaches the body model animator and one animator per attachment the type defines.
signed char Vehicle::OnSubFileLoaded()
{
    EXItem *pItem = m_pItem;
    VehicleTypeBase *pType = m_pTypeObject;
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(0x1000026, 0, 0);
    EXItemAnimator_Entity *pBody = EXItemAnimator_Entity::NewItem();
    if (pBody) {
        pBody->Initialise(pFile, pType->m_entityHash);
        pItem->AddAnimator(pBody);
        for (int i = 0; i < 17; i++) {
            unsigned int hash = pType->m_attachments[i];
            if (hash) {
                EXItemAnimator_Entity *pAtt = EXItemAnimator_Entity::NewItem();
                m_attachments[i] = pAtt;
                pAtt->Initialise(pFile, hash);
                pItem->AddAnimator(pAtt);
                pAtt->ItemFlags().m_flags |= 1;
            }
        }
        updateLights();
        return 1;
    }
    removeItemFromWorld();
    return 0;
}

static __forceinline void SetAttRender(EXItemAnimator_Entity *p, signed char bOn)
{
    if (p) {
        if (bOn) ((EXPlacementItem *)p)->SetHasRender();
        else ((EXPlacementItem *)p)->ClrHasRender();
    }
}

// Switches the lamp attachments: head lamps at night, brake lamps while braking (and for half a
// second after), and the blinking indicators.
void Vehicle::updateLights()
{
    EXPlacementItem *pGlow = (EXPlacementItem *)m_attachments[14];
    if (pGlow) {
        pGlow->SetHasRender();
        pGlow->RenderInfo().EnableAlphaMult(1);
        pGlow->RenderInfo().SetRGBAMult(1.0f, 1.0f, 1.0f, 0.3f);
    }
    signed char bNight = (m_flags & 2) != 0;
    SetAttRender(m_attachments[6], bNight);
    SetAttRender(m_attachments[7], bNight);
    SetAttRender(m_attachments[15], bNight);
    SetAttRender(m_attachments[16], bNight);
    signed char bBrake = (m_flags >> 2) & 1;
    if (!bBrake) {
        if (frameTime - m_unk7cc < 0.5f) bBrake = 1;
    } else {
        m_unk7cc = frameTime;
    }
    SetAttRender(m_attachments[8], bBrake);
    SetAttRender(m_attachments[9], bBrake);
    unsigned char f = m_flags;
    signed char bLeft = (f >> 3) & 1;
    signed char bRight = (f >> 4) & 1;
    if (m_indicatorTime <= 0.0f || (m_indicatorTime -= frameLen, 0.0f < m_indicatorTime)) {
        if (bLeft || bRight) {
            float t = frameLen + m_indicatorTimer;
            bool bWrap = 0.7f <= t;
            m_indicatorTimer = t;
            if (bWrap) m_indicatorTimer = t - 0.7f;
            if (!(m_indicatorTimer < 0.4f || 0.7f < m_indicatorTimer)) {
                bRight = 0;
                bLeft = 0;
            }
        }
    } else {
        m_flags = f & 0xe7;
        bRight = 0;
        bLeft = 0;
    }
    SetAttRender(m_attachments[10], bLeft);
    SetAttRender(m_attachments[12], bLeft);
    SetAttRender(m_attachments[11], bRight);
    SetAttRender(m_attachments[13], bRight);
}

// Per-frame update: movement, sound, item placement, unspawning, radar blip and sight checks.
void Vehicle::update()
{
    m_updateCount = _theTrafficSysAlloc.getSys()->getUpdateCount();
    UpdateAttackedCountDown();
    if (m_pObjective) objectiveUpdate();
    m_move.updateRoute();
    m_move.update();
    if (m_pTypeObject && m_pItem) {
        m_sfx.update();
        setItemPosAndRot();
        updateTransparency();
        UpdateMuzzleScript();
        if ((m_flags & 1) &&
            (((m_pExplosion || m_bFading == 1) && m_alpha != 0.0f) || !(m_flags & 0x20) || !theApp.m_bCutscene) &&
            checkForAndUnspawnVehicle(this))
            return;
        if (!bIDontKnowWhyTheRadarCrashesTheFrameAfterPauseButThisFlagShouldGetRoundItHopefully && m_unk708) {
            FERadarItem item;
            ex::RGBA col;
            if (m_unk708 == 2) col = ex::RGBA(0, 0xff, 0, 0xff);
            else col = ex::RGBA(0xff, 0, 0, 0xff);
            item.m_Pos = m_pItem->m_position;
            item.m_Unk10 = 0;
            item.m_Colour = col;
            if (m_unk730) item.m_Text = Text_Get(m_unk730);
            FERadar::Instance(1)->DrawItemOnRadar(item, FERadar::Instance(1)->m_Info, (FERenderPass::Flags)3);
        }
        if (m_vehFlags & 1) CheckSeePlayer();
        updateHealthIndicator();
        updateLights();
    }
}

// Builds the vehicle of the given type at a location (a random place when none is given).
signed char Vehicle::create(unsigned char type, TrafficLocation *pLoc)
{
    m_pTypeObject = vehicleTypeObjFromType(type);
    if (!m_pTypeObject) return 0;
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(0x1000026, 0, 0);
    if (!pFile) return 0;
    EXItem *pItem = m_pItem;
    if (!pItem) {
        pItem = new EXItem;
        if (!pItem) return 0;
    }
    m_pItem = pItem;
    pItem->RenderInfo().SetExtRenderInfo((EXExtRenderInfo *)&m_lights);
    pItem->RenderInfo().SetTexSwapInfo(&m_texSwap);
    if (XItemHandler_Player::Instance()) HandleVisionModeTextureSwaps(XItemHandler_Player::Instance()->m_Tint);
    pItem->SetXItemOwner((EXItem *)this);
    pItem->SetGeoFile(pFile);
    pItem->SetRefObject(0x1000026);
    if (m_pTypeObject->loadSubFile(1)) {
        if (!OnSubFileLoaded()) return 0;
    }
    TrafficLocation loc;
    TrafficLocation *p = pLoc;
    if (!pLoc) {
        _theTrafficSysAlloc.getSys()->getRandLocation(loc, m_pTypeObject->m_zDistToRear + 1.0f,
                                                      m_pTypeObject->m_zDistToFront + 1.0f);
        p = &loc;
        if (loc.m_flag) {
            m_currPos.m_loc.m_flag = 1;
            return 0;
        }
    }
    m_currPos.setLoc(p);
    EXVector3 pos, rot;
    m_currPos.m_loc.calcPosAndRot(pos, rot);
    pos.y = m_pTypeObject->m_yOffset + pos.y;
    float angle = rot.y + 1.5707964f;
    m_move.m_phys.m_f4 = angle;
    m_move.m_railsAngle = angle;
    m_currRot.x = rot.x;
    m_currRot.y = rot.y;
    m_currRot.z = rot.z;
    m_currPos.set(pos);
    m_move.m_accel.m_fac = pos.y;
    setItemPosAndRot();
    EXVector at(pos.x, pos.y, pos.z, 0.0f);
    WORLD->GetMapOn(at);
    WORLD->AddItem(pItem, 2);
    m_move.m_phys.m_f1c = pos.x;
    m_move.m_phys.m_f20 = pos.z;
    if (!m_sfx.checkTypePlaying(6, 0, 0)) m_sfx.play(6, 0);
    if (m_pTypeObject->m_type == 6) {
        m_move.m_phys.m_dampen0.m_len = 20;
        m_move.m_phys.m_dampen1.m_len = 14;
    }
    m_move.m_pRouteLane = m_currPos.m_loc.m_pLane;
    if (_theTrafficSysAlloc.getSys()->isNightLevel()) m_flags |= 2;
    return 1;
}

// Collision against a triangle (a swing): the first collision datum capsule within reach returns
// its hash, and the hit point is pushed back out along the line from the triangle.
int Vehicle::CheckCollision(const EXTriangle &tri, float r, EXVector &out)
{
    if (CollisionDatums[0]) {
        const unsigned int *pHash = &CollisionDatums[0];
        int i = 0;
        do {
            EXDatumTemp d;
            if (m_pItem->GetAnimatorDatum(*pHash, d, 0, 0)) {
                EXCapsule c;
                d.Get(c);
                float t = 0.0f;
                float s0 = 0.0f;
                if (EXSqrt(EXSqrDistance(c.Segment(), tri, &t, &s0, 0)) <= c.Radius() + r) {
                    EXVector pt = c.Origin() + c.Direction() * t;
                    out = pt;
                    EXVector dir = (pt - tri.Origin()).UnitVectorXYZ();
                    out = dir * (s0 - (c.Radius() + 0.3f)) + pt;
                    return CollisionDatums[i];
                }
            }
            pHash++;
            i++;
        } while (*pHash);
    }
    return 0;
}

// Collision against a capsule: the first collision datum capsule within reach returns its hash
// and a point along the capsule's axis.
int Vehicle::CheckCollision(const EXCapsule &cap, EXVector &out)
{
    int i = 0;
    if (CollisionDatums[0]) {
        const unsigned int *pHash = &CollisionDatums[0];
        do {
            EXDatumTemp d;
            if (m_pItem->GetAnimatorDatum(*pHash, d, 0, 0)) {
                EXCapsule c;
                d.Get(c);
                float s = 0.0f;
                float t = 0.0f;
                if (EXSqrt(EXSqrDistance(cap.Segment(), c.Segment(), &s, &t)) <= cap.Radius() + c.Radius()) {
                    EXVector dir = cap.Direction().UnitVectorXYZ();
                    s = s - (c.Radius() - 0.3f);
                    out = cap.Origin() + dir * s;
                    return CollisionDatums[i];
                }
            }
            pHash++;
            i++;
        } while (*pHash);
    }
    return 0;
}

static __forceinline void Vehicle_SetTriangle(EXTriangle &t, const EXVector &a, const EXVector &b, const EXVector &c)
{
    t.m_v[0].set(a);
    t.Edge0Set(b - a);
    t.Edge1Set(c - a);
}

// Collision against a quad given as four corners, tested as the triangles (0, 1, 2) and (1, 3, 2).
int Vehicle::CheckCollision(const EXVector *p, float r, EXVector &out)
{
    EXTriangle t;
    Vehicle_SetTriangle(t, p[0], p[1], p[2]);
    int hit = CheckCollision(t, r, out);
    if (hit) return hit;
    Vehicle_SetTriangle(t, p[1], p[3], p[2]);
    return CheckCollision(t, r, out);
}

extern EXVector zeroVec;

// Datum of each attachment slot (wheels 0-3, 4-5, lamps...); slot 14 (the shadow) has none.
static const unsigned int s_AttachDatums[17] = {
    0x10000015, 0x10000016, 0x10000017, 0x10000018, 0x100000f7, 0x100000f8, 0x10000089, 0x1000008a, 0x1000008d,
    0x1000008e, 0x10000093, 0x10000092, 0x10000091, 0x10000090, 0,          0x10000095, 0x10000094,
};

// Places the item at the vehicle's position and tilts it by the body spring angles, then moves
// every attachment onto its datum. The wheels (slots 0-5) are also steered and rolled against the
// body tilt.
void Vehicle::setItemPosAndRot()
{
    EXItem *pItem = m_pItem;
    float y = m_currPos.m_pos.y - m_pTypeObject->m_bodyHeightAdjust;
    float z = m_currPos.m_pos.z;
    float rotY = m_currRot.y;
    pItem->m_position.x = m_currPos.m_pos.x;
    pItem->m_position.y = y;
    pItem->m_position.z = z;
    pItem->m_position.w = 0.0f;
    pItem->m_validMtx = 0;
    pItem->m_rotation = zeroVec;
    pItem->m_validMtx = 0;
    float roll = m_move.m_phys.m_f18;
    float pitch = m_move.m_phys.m_f14;
    float steer;
    if (!(m_move.m_flags & 8)) steer = m_move.m_phys.m_fc;
    else steer = m_move.m_fakeSteerAngle;
    for (int i = 0; i < 17; i++) {
        EXPlacementItem *pAtt = (EXPlacementItem *)m_attachments[i];
        if (!pAtt) continue;
        if (i == 14) {
            float shadow = (m_pTypeObject->m_shadowYOffset + y) - y;
            pAtt->m_position.x = 0.0f;
            pAtt->m_position.y = shadow;
            pAtt->m_position.z = 0.0f;
            pAtt->m_position.w = 0.0f;
            pAtt->m_rotation.x = 0.0f;
            pAtt->m_rotation.y = 0.0f;
            pAtt->m_rotation.z = 0.0f - pitch;
            pAtt->m_rotation.w = 0.0f;
            pAtt->m_validMtx = 0;
            continue;
        }
        EXDatumTemp d;
        if (!pItem->GetAnimatorDatum(s_AttachDatums[i], d, 5, 0)) continue;
        if (i < 6) {
            EXVector euler;
            d.m_rotation.GetRotation(euler, 4);
            float adjZ, adjY;
            if (i < 2) {
                euler.y = euler.y + steer;
                adjZ = m_pTypeObject->m_frontWheelAdjustZ;
                adjY = m_pTypeObject->m_frontWheelAdjustY;
            } else {
                adjZ = m_pTypeObject->m_rearWheelAdjustZ;
                adjY = m_pTypeObject->m_rearWheelAdjustY;
            }
            float tx = d.m_position.x + adjZ;
            float ty = d.m_position.y + adjY;
            float a = 0.0f - roll;
            float sa = (float)sin(a);
            float ca = (float)cos(a);
            float b = 0.0f - pitch;
            float sb = (float)sin(b);
            float cb = (float)cos(b);
            EXMaths::mat_44 rotX, rotZ, trans, tmp;
            rotX.r[0].x = 1.0f; rotX.r[0].y = 0.0f; rotX.r[0].z = 0.0f; rotX.r[0].w = 0.0f;
            rotX.r[1].x = 0.0f; rotX.r[1].y = ca; rotX.r[1].z = sa; rotX.r[1].w = 0.0f;
            rotX.r[2].x = 0.0f; rotX.r[2].y = 0.0f - sa; rotX.r[2].z = ca; rotX.r[2].w = 0.0f;
            rotX.r[3].x = 0.0f; rotX.r[3].y = 0.0f; rotX.r[3].z = 0.0f; rotX.r[3].w = 1.0f;
            rotZ.r[0].x = cb; rotZ.r[0].y = sb; rotZ.r[0].z = 0.0f; rotZ.r[0].w = 0.0f;
            rotZ.r[1].x = 0.0f - sb; rotZ.r[1].y = cb; rotZ.r[1].z = 0.0f; rotZ.r[1].w = 0.0f;
            rotZ.r[2].x = 0.0f; rotZ.r[2].y = 0.0f; rotZ.r[2].z = 1.0f; rotZ.r[2].w = 0.0f;
            rotZ.r[3].x = 0.0f; rotZ.r[3].y = 0.0f; rotZ.r[3].z = 0.0f; rotZ.r[3].w = 1.0f;
            trans.set_identity();
            trans.r[3].x = tx;
            trans.r[3].y = ty;
            trans.r[3].z = d.m_position.z;
            trans.r[3].w = 1.0f;
            D3DXMatrixMultiply(&tmp, &trans, &rotX);
            trans = tmp;
            D3DXMatrixMultiply(&tmp, &trans, &rotZ);
            trans = tmp;
            euler.x = euler.x + a;
            pAtt->m_position.x = trans.r[3].x;
            pAtt->m_position.y = trans.r[3].y;
            pAtt->m_position.z = trans.r[3].z;
            pAtt->m_position.w = trans.r[3].w;
            pAtt->m_validMtx = 0;
            euler.z = (6.2831855f - m_move.m_phys.m_f10) + b + euler.z;
            pAtt->m_rotation.x = euler.x;
            pAtt->m_rotation.z = euler.z;
            pAtt->m_rotation.y = euler.y;
            pAtt->m_rotation.w = euler.w;
            pAtt->m_validMtx = 0;
        } else {
            pAtt->m_position.x = d.m_position.x;
            pAtt->m_position.y = d.m_position.y;
            pAtt->m_position.z = d.m_position.z;
            pAtt->m_position.w = d.m_position.w;
            pAtt->m_validMtx = 0;
        }
    }
    pItem->m_rotation.x = roll;
    pItem->m_rotation.y = rotY;
    pItem->m_rotation.z = pitch;
    pItem->m_rotation.w = 0.0f;
    pItem->m_validMtx = 0;
    EXItem *pExp = m_pExplosion;
    if (pExp) {
        pExp->m_position.x = pItem->m_position.x;
        pExp->m_position.y = pItem->m_position.y;
        pExp->m_position.z = pItem->m_position.z;
        pExp->m_position.w = pItem->m_position.w;
        pExp->m_validMtx = 0;
        pExp = m_pExplosion;
        pExp->m_rotation.x = pItem->m_rotation.x;
        pExp->m_rotation.y = pItem->m_rotation.y;
        pExp->m_rotation.z = pItem->m_rotation.z;
        pExp->m_rotation.w = pItem->m_rotation.w;
        pExp->m_validMtx = 0;
    }
}

extern int Actor_CalledRayCast;

// Whether the vehicle (roughly) faces the point, within range and about 5.5 above or below its
// middle, with an unobstructed (jittered) line to the map.
signed char Vehicle::CanSeePoint(EXVector &p, float range)
{
    EXVector from(m_currPos.m_pos.x, m_pTypeObject->m_yDim * 0.5f + m_pTypeObject->m_yOffset + m_currPos.m_pos.y,
                  m_currPos.m_pos.z, 0.0f);
    float dy = p.y - from.y;
    if ((dy < 0.0f ? -dy : dy) <= 5.5f) {
        float dx = p.x - from.x;
        float dz = p.z - from.z;
        if (dy * dy + dz * dz + dx * dx <= range * range) {
            float ang = m_currRot.y + 1.5707964f;
            float s = (float)sin(ang);
            float c = (float)cos(ang);
            EXVector flat = EXVector(dx, 0.0f, dz, 0.0f).UnitVector();
            if (0.27f < flat.z * c + flat.x * s) {
                EXRayCastTemp ray;
                EXVector jitter;
                jitter.y = EXRandf() * 2.0f - 1.0f;
                jitter.x = EXRandf() * 2.0f - 1.0f;
                jitter.z = EXRandf() * 2.0f - 1.0f;
                jitter.w = 0.0f;
                EXVector target = p + jitter * 0.2f;
                ray.Set(from, target - from, 0, 0, 0, 0);
                Actor_CalledRayCast++;
                ITEMENV->DoRayCastToMap(ray, 0);
                return !ray.m_bHit;
            }
        }
    }
    return 0;
}
