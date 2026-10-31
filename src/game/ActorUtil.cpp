#include "ActorUtil.h"
#include "Actor.h"
#include "ActorLocation.h"
#include "ActorProfile.h"
#include "XItem.h"
#include "XItemHandler_Player.h"
#include "XNavMeshHandler.h"
#include "XNavMesh.h"
#include "EXGeoFile.h"
#include "EXGeoSpreadSheet.h"
#include "FEText.h"
#include "XNavLocation.h"
#include "AIControl.h"
#include "EXDatum.h"
#include "FX_Gore.h"
#include "ActorOptions.h"
#include "EXRand.h"
#include "XApp.h"
#include "XGameWnd.h"
#include "WeaponTable.h"
#include "XItemHandler_Camera.h"
#include "EXSphere.h"
#include "EXGeoHeader.h"
#include "EXTrigger.h"
#include "EXItemAnimator_Anim.h"
#include "EXItemRender_Anim.h"
#include "FELevels.h"
#include "GameData.h"
#include "ActorCombat.h"
#include "Vehicle.h"
#include "XItemHandler_ScriptBase.h"
#include "EXGeoMap.h"
#include "EXGeoPathNode.h"
#include "ActorCarryable.h"
#include "AIFormation.h"
#include "FEHud.h"

extern unsigned int gCurrentMap_GeoHash;  // TODO stand-in (XApp/level globals)

// TODO stand-ins: other areas.
void Objective_NotifyActorDeath(Actor *pActor);
signed char STATE_BeingMugged(StateMachine &sm);
signed char STATE_FightActor(StateMachine &sm);
signed char STATE_AttackPred(StateMachine &sm);
signed char STATE_CityDistractions(StateMachine &sm);  // TODO stand-in (ActorCity.obj)

extern XGameWnd *gpGameWnd[4];  // TODO stand-in (GameWnd.obj)

// ---- data ----
// GUESS: name. Retail loads this from unnamed ActorUtil data (0x3c23d70b = 0.1f * 0.1f), not a __real.
static const float kArriveSqDist = 0.1f * 0.1f;
// GUESS: name; unnamed ActorUtil data (0.05f) read by STATE_FreeFall and STATE_FadeOutDead's alpha fade.
static const float kFreeFallLandDist = 0.05f;
unsigned int g_ulPlayerFeetPosFrame;
unsigned int g_ulPlayerHeadPosFrame;
EXVector g_vPlayerHeadPos;
EXVector g_vPlayerFeetPos;

static inline XItemHandler_Player *AU_PlayerHandler()
{
    if (!gpPlayerItem[0]) return 0;
    return (XItemHandler_Player *)gpPlayerItem[0]->m_pItemHandler;
}

// VC7.1 will not inline EXVector::operator- here; retail did (field-wise subtract).
static __forceinline EXVector AU_Sub(const EXVector &a, const EXVector &b)
{
    EXVector r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    r.w = a.w - b.w;
    return r;
}

// EXVector::operator- spelled out: VC7.1 stops inlining it in these units, retail inlined it (two temp copies).
static __forceinline EXVector AU_SubV(const EXVector &a, const EXVector &b)
{
    return EXVector(a.EXMaths::vec_xyzw::operator-(b));
}

static __forceinline EXVector AU_MulV(const EXVector &v, float f)
{
    return EXVector(v.EXMaths::vec_xyzw::operator*(f));
}

// A state that only forwards its entry to another state with a fixed location hash.
#define AU_STATE_ENTER_GOTO(name, state, hash) \
    signed char name(StateMachine &sm) \
    { \
        if (sm.m_Msg.m_Type != MSG_ENTER) return 0; \
        SM_GOTO(state, hash, 0, 0, 0); \
        return 1; \
    }
// The same with the standard message handling (WalkToLocationN, RunToLocationN ...).
#define AU_STATE_ENTER_GOTO_STD(name, state, hash) \
    signed char name(StateMachine &sm) \
    { \
        switch (sm.m_Msg.m_Type) { \
        case MSG_ENTER: \
            SM_GOTO(state, hash, 0, 0, 0); \
            return 1; \
        AU_DEFAULT_MSGS \
        } \
        return 0; \
    }

signed char AU_IsOnScreen(EXItem *pItem, const EXVector &pos, float radius)
{
    if (XGetCamera(0) && pItem) {
        EXSphere sphere;
        sphere.m_centre.x = pos.x;
        sphere.m_centre.y = pos.y;
        sphere.m_centre.z = pos.z;
        sphere.m_centre.w = pos.w;
        sphere.m_radius = radius;
        return XGetCamera(0)->BoundingVolumeOnScreen(sphere) != 0;
    }
    return 0;
}

signed char AU_ActorGroupWithinRange(int id, int group, const EXVector &pos, float radius)
{
    Actor *pSelf = Actor::GetPtrFromID(id);
    for (Actor *p = Actor::GetFirst(pos, radius, 0); p; p = p->GetNext(pos, radius, 0)) {
        if (p != pSelf && p->GetProfile()->m_Group == group) return 1;
    }
    return 0;
}

EXGeoPath *AU_GetPathFromIndex(int i) { return theItemEnv.GetPath(i); }

void Player_DoDamage(float d)
{
    if (gpPlayerItem[0] && gpPlayerItem[0]->m_pItemHandler)
        ((XItemHandler_Player *)gpPlayerItem[0]->m_pItemHandler)->SendPredMsg(0, d);
}

// NEAR: retail stores the four zero data words in reverse order (3..1).
void SendMsg_LoudNoise(const EXVector &pos, float radius, Actor *pFrom, signed char bDelay, signed char bCapped)
{
    float delay = 0.0f;
    if (bDelay) delay = -1.0f;
    if (bCapped)
        Actor::SendMsgCapped(MsgItem(MSG_LOUD_NOISE), pos, radius, pFrom, delay, -1.0f, 5.0f);
    else
        Actor::SendMsg(MsgItem(MSG_LOUD_NOISE), pos, radius, pFrom, delay);
}

void SendMsg_Attract(Actor *pActor, const EXVector &pos)
{
    if (pActor) pActor->SendMsg(MsgItem(MSG_ATTRACT), 0.0f);
}

void SendMsg_Repel(Actor *pActor, const EXVector &pos)
{
    if (pActor) pActor->SendMsg(MsgItem(MSG_REPEL), 0.0f);
}

void AU_AssignSkin(EXItemAnimator_Anim *pAnim, int file, int hash)
{
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(file, 0, 0);
    if (pFile) {
        EXGeoBaseRefObject ref;
        ref.m_pGeoFile = pFile;
        ref.m_hashRef = hash;
        pAnim->m_pSkin->SetSkinRefObject(ref);
    }
}

signed char AU_AssignSkinSkelAnim(EXItemAnimator_Anim *pAnim, int skinFile, int skinHash, int skelFile, int skelHash, int animFile,
                                  int animHash)
{
    // Zeroed on first use (compiler guard), as retail.
    struct LodInit : EXAnimLodDetails {
        LodInit()
        {
            m_dist = 0.0f;
            m_flags = 0;
            m_skinBits.ClrAll();
        }
    };
    if (pAnim && (skinFile & 0xff000000) == 0x1000000 && (skinHash & 0xff000000) == 0xd000000 &&
        (skelFile & 0xff000000) == 0x1000000 && (skelHash & 0xff000000) == 0xd000000 &&
        (animFile & 0xff000000) == 0x1000000 && (animHash & 0xff000000) == 0x3000000) {
        static LodInit s_Lod;
        pAnim->SetLodDetails(&s_Lod, 1);
        EXGeoFile *pSkin = EXGeoFile::LoadGeoFile(skinFile, 0, 0);
        EXGeoFile *pSkel = EXGeoFile::LoadGeoFile(skelFile, 0, 0);
        EXGeoFile *pAnimFile = EXGeoFile::LoadGeoFile(animFile, 0, 0);
        if (pSkin && pSkel && pAnimFile) {
            EXGeoRefObject skel;
            EXGeoRefObject anim;
            EXGeoRefObject skin;
            skin.Set(pSkin, skinHash);
            skel.Set(pSkel, skelHash);
            anim.Set(pAnimFile, animHash);
            pAnim->Initialise(anim, skel);
            pAnim->m_pSkin->SetSkinRefObject(skin);
            return 1;
        }
    }
    return 0;
}

int AU_SpawnActor(int loc, int profile, int a, unsigned int b)
{
    int poi = POI_ResolveLocation(loc, -1, 0, 0);
    if (!poi) return poi;
    POI_BIND *pBind = POI_PatternIteratorNext(poi);
    Actor *pActor = Actor::Create(profile, pBind->m_Pos, pBind->m_Poly, POI_Get(poi)->m_YRot, a, b, 0);
    if (pActor) {
        if (pActor->GetProfile()->m_StartState == 0x4d000104) POI_PatternIteratorBind(poi, pBind, pActor->GetRefID());
        return pActor->GetRefID();
    }
    return 0;
}

void AU_DrawCross(const EXVector &pos, float size, int col) {}

signed char Player_IsCloaked()
{
    if (!gpPlayerItem[0]) return 0;
    if (!gpPlayerItem[0]->m_pItemHandler) return 0;
    return ((XItemHandler_Player *)gpPlayerItem[0]->m_pItemHandler)->GetCloak() != 0;
}

int AU_NavPolyForPos(const EXVector &pos)
{
    XNavLocation loc;
    loc.SetPosPoly(pos, -1, 0);
    if (loc.m_bValid) return loc.m_nPolyHandle;
    return -1;
}

EXVector AU_SnapFloorPos(const EXVector &pos, int *pPoly, signed char b)
{
    XNavLocation loc;
    loc.SetPosPoly(pos, -1, 0);
    float x, y, z, w;
    if (loc.m_bValid) {
        x = loc.m_vPos.x;
        z = loc.m_vPos.z;
        w = loc.m_vPos.w;
        y = loc.m_vFloorPos.y;
        if (pPoly) *pPoly = loc.m_nPolyHandle;
    } else {
        x = pos.x;
        z = pos.z;
        w = pos.w;
        y = 0.0f;
    }
    EXVector r;
    r.x = x;
    r.y = y;
    r.z = z;
    r.w = w;
    return r;
}

EXVector AU_SnapRelativeFloorPos(const EXVector &pos, const EXVector &target, int *pPoly)
{
    int poly = -1;
    XNavLocation loc;
    EXVector out;
    out.x = target.x;
    out.y = target.y;
    out.z = target.z;
    out.w = target.w;
    loc.SetPosPoly(pos, -1, 0);
    if (!loc.m_bValid) {
        out.x = target.x;
        out.z = target.z;
        out.w = target.w;
        out.y = pos.y;
    } else {
        XNavMesh *pMesh = gNavMeshHandler.GetPtr((u32)loc.m_nMesh);
        if (pMesh) {
            pMesh->ValidateLine(loc, target, poly, out, 0);
            poly |= 0xff000000;
        }
    }
    if (pPoly) *pPoly = poly;
    return out;
}

// NEAR: logic complete; retail keeps poly in its own stack slot (VC reuses a dead argument slot) and loads pos first
EXVector AU_SnapRelativeToLoc(XNavLocation &loc, const EXVector &pos, int *pPoly)
{
    float x = pos.x;
    float y = pos.y;
    float z = pos.z;
    float w = pos.w;
    int poly = -1;
    EXVector out;
    out.x = x;
    out.y = y;
    out.z = z;
    out.w = w;
    if (!loc.m_bValid) {
        y = loc.m_vPos.y;
    } else {
        XNavMesh *pMesh = gNavMeshHandler.GetPtr((u32)loc.m_nMesh);
        if (pMesh) {
            pMesh->ValidateLine(loc, pos, poly, out, 0);
            x = out.x;
            y = out.y;
            z = out.z;
            w = out.w;
            poly |= 0xff000000;
        }
    }
    if (pPoly) *pPoly = poly;
    EXVector r;
    r.x = x;
    r.y = y;
    r.z = z;
    r.w = w;
    return r;
}

EXVector Player_GetFeetPos()
{
    if (gpGameWnd[0]->m_Unk384 == g_ulPlayerFeetPosFrame) return g_vPlayerFeetPos;
    g_ulPlayerFeetPosFrame = gpGameWnd[0]->m_Unk384;
    if (gpPlayerItem[0]) g_vPlayerFeetPos = gpPlayerItem[0]->m_position;
    else g_vPlayerFeetPos = EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    return g_vPlayerFeetPos;
}

EXVector Player_GetPelvisPos()
{
    XItemHandler_Player *pPlayer = XItemHandler_Player::Instance();
    if (!pPlayer) return EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    return pPlayer->GetPelvisPos();
}

// NEAR: retail copies the cached vector out in x,y,z,w order and keeps the handler in ecx.
EXVector Player_GetHeadPos()
{
    u32 frame = gpGameWnd[0]->m_Unk384;
    if (frame == g_ulPlayerHeadPosFrame) return EXVector(g_vPlayerHeadPos.x, g_vPlayerHeadPos.y, g_vPlayerHeadPos.z, g_vPlayerHeadPos.w);
    g_ulPlayerHeadPosFrame = frame;
    if (gpPlayerItem[0] && gpPlayerItem[0]->m_pItemHandler) g_vPlayerHeadPos = ((XItemHandler_Player *)gpPlayerItem[0]->m_pItemHandler)->GetTopHeadPos();
    else g_vPlayerHeadPos = EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    return EXVector(g_vPlayerHeadPos.x, g_vPlayerHeadPos.y, g_vPlayerHeadPos.z, g_vPlayerHeadPos.w);
}

EXVector Player_GetRot()
{
    if (!gpPlayerItem[0]) return EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    return gpPlayerItem[0]->m_rotation;
}

int Player_GetVisionMode()
{
    if (!gpPlayerItem[0]) return 0;
    if (!gpPlayerItem[0]->m_pItemHandler) return 0;
    return ((XItemHandler_Player *)gpPlayerItem[0]->m_pItemHandler)->m_Tint;
}

// True when the angle between (a - origin) and (b - origin) is at most `angle` degrees (4D unit vectors).
// NEAR: logic complete; retail copies each difference through two more temporaries before normalising.
signed char AU_VecAngleInside(const EXVector &a, const EXVector &origin, const EXVector &b, float angle)
{
    EXVector d0 = a - origin;
    d0.EXMaths::vec_xyzw::Normalise();
    EXVector d1 = b - origin;
    d1.EXMaths::vec_xyzw::Normalise();
    if (d0.Dot(d1) < (float)cos(angle * 0.0174532925f)) return 0;
    return 1;
}

// Attack-type names unknown; the type -> armour column mapping is read from the jump table.
void AU_AdjustDamageForArmour(StateMachine &sm, StructMsg_AttackedByPredator *pMsg)
{
    if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f000017) pMsg->m_Damage = 0.0f;
    if (gCurrentMap_GeoHash == 0x100013b && sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f000011) pMsg->m_Damage = 0.0f;
    if (sm.GetActorOwner().GetEMPTimer() > 0.0f) return;
    ArmourProfile *pArmour = ArmourProfile::GetArmourData(sm.GetActorOwner().GetProfile()->m_Armour);
    if (pArmour) {
        switch (pMsg->m_Attack) {
        case 9:
            pMsg->m_Damage = pArmour->m_Damage[9] * pMsg->m_Damage;
            break;
        case 7:
            pMsg->m_Damage = pArmour->m_Damage[5] * pMsg->m_Damage;
            break;
        case 1:
        case 38:
        case 50:
            pMsg->m_Damage = pArmour->m_Damage[0] * pMsg->m_Damage;
            break;
        case 11:
            pMsg->m_Damage = pArmour->m_Damage[8] * pMsg->m_Damage;
            break;
        case 10:
            pMsg->m_Damage = pArmour->m_Damage[10] * pMsg->m_Damage;
            break;
        case 5:
            pMsg->m_Damage = pArmour->m_Damage[6] * pMsg->m_Damage;
            break;
        case 2:
        case 31:
        case 34:
        case 51:
        case 52:
        case 53:
        case 54:
            pMsg->m_Damage = pArmour->m_Damage[2] * pMsg->m_Damage;
            break;
        case 3:
        case 32:
        case 35:
        case 59:
        case 60:
        case 61:
        case 62:
            pMsg->m_Damage = pArmour->m_Damage[4] * pMsg->m_Damage;
            break;
        case 4:
        case 33:
        case 36:
        case 55:
        case 56:
        case 57:
        case 58:
            pMsg->m_Damage = pArmour->m_Damage[3] * pMsg->m_Damage;
            break;
        case 6:
            pMsg->m_Damage = pArmour->m_Damage[7] * pMsg->m_Damage;
            break;
        case 0:
        case 30:
        case 37:
        case 49:
            pMsg->m_Damage = pArmour->m_Damage[1] * pMsg->m_Damage;
            break;
        }
    }
}

signed char AU_HandleMuggingAttacks(StateMachine &sm)
{
    SM_GOSUB(STATE_BeingMugged, 0, 0, 0, 0);
    return 1;
}

signed char AU_DoesAttackCreateLoudNoise(StructMsg_AttackedByPredator *pMsg)
{
    switch (pMsg->m_Attack) {
    case 0x1e:
        if (!(pMsg->m_Damage >= 2.0f)) return 0;
    case 0x0:
    case 0x1:
    case 0x2:
    case 0x3:
    case 0x4:
    case 0x5:
    case 0x6:
    case 0x7:
    case 0x8:
    case 0x9:
    case 0xa:
    case 0xb:
    case 0x31:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x36:
    case 0x37:
    case 0x38:
    case 0x39:
    case 0x3a:
    case 0x3b:
    case 0x3c:
    case 0x3d:
    case 0x3e:
    case 0x4b:
        return 1;
    case 0x1f:
    case 0x20:
    case 0x21:
    case 0x22:
    case 0x23:
    case 0x24:
    case 0x25:
    case 0x26:
    case 0x27:
    case 0x28:
    case 0x29:
    case 0x2a:
    case 0x2b:
    case 0x2c:
    case 0x2d:
    case 0x2e:
    case 0x2f:
    case 0x30:
    case 0x47:
    case 0x48:
    case 0x49:
    case 0x4a:
        return 0;
    }
    return 0;
}

signed char AU_DoesAttackOverrideCloak(StructMsg_AttackedByPredator *pMsg)
{
    switch (pMsg->m_Attack) {
    case 0x1e:
        if (!(pMsg->m_Damage >= 2.0f)) return 0;
    case 0x0:
    case 0x1:
    case 0x2:
    case 0x3:
    case 0x4:
    case 0x1f:
    case 0x20:
    case 0x21:
    case 0x22:
    case 0x23:
    case 0x24:
    case 0x25:
    case 0x26:
    case 0x27:
    case 0x28:
    case 0x29:
    case 0x2a:
    case 0x2b:
    case 0x2c:
    case 0x2d:
    case 0x2e:
    case 0x2f:
    case 0x30:
    case 0x31:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x36:
    case 0x37:
    case 0x38:
    case 0x39:
    case 0x3a:
    case 0x3b:
    case 0x3c:
    case 0x3d:
    case 0x3e:
    case 0x47:
    case 0x48:
    case 0x49:
    case 0x4a:
    case 0x4b:
        return 1;
    case 0x5:
    case 0x6:
    case 0x7:
    case 0x8:
    case 0x9:
    case 0xa:
    case 0xb:
        return 0;
    }
    return 0;
}

signed char AU_DoesAttackGiveActorSight(StateMachine &sm, StructMsg_AttackedByPredator *pMsg)
{
    switch (pMsg->m_Attack) {
    case 0x1e:
        if (!(pMsg->m_Damage >= 2.0f)) return 0;
    case 0x0:
    case 0x1:
    case 0x2:
    case 0x3:
    case 0x4:
    case 0x5:
    case 0x6:
    case 0x7:
    case 0x1f:
    case 0x20:
    case 0x21:
    case 0x22:
    case 0x23:
    case 0x24:
    case 0x25:
    case 0x26:
    case 0x31:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x36:
    case 0x37:
    case 0x38:
    case 0x39:
    case 0x3a:
    case 0x3b:
    case 0x3c:
    case 0x3d:
    case 0x3e:
    case 0x4a:
    case 0x4b:
        return 1;
    case 0x8:
    case 0x9:
    case 0xa:
    case 0xb:
        if (sm.GetActorOwner().m_ActorFlags.m_bBoss || sm.GetActorOwner().m_ActorFlags.m_bUlysses) return 1;
        return 0;
    case 0x27:
    case 0x28:
    case 0x29:
    case 0x2a:
    case 0x2b:
    case 0x2c:
    case 0x2d:
    case 0x2e:
    case 0x2f:
    case 0x30:
    case 0x47:
    case 0x48:
    case 0x49:
        return 0;
    }
    return 0;
}

// NEAR: logic complete; retail tests 0 < health (comiss 0, h / jae) and sums the distance y, z, x.
void AU_PlayerForceSightingInArea(const EXVector &pos, float radius)
{
    for (Actor *p = Actor::GetFirst(pos, radius, 0); p; p = p->GetNext(pos, radius, 0)) {
        if (p->GetHealth() > 0.0f && !p->IsLyingBum() && !p->m_ActorFlags2.m_bDazed && !p->IsLyingOnGround()) {
            if (p->CanSeePoint(pos, 0.0f)) {
                p->PlayerForceSighting();
            } else {
                EXVector d = AU_SubV(p->GetHeadPos(), pos);
                if (1.0f > d.LengthXYZSquared()) p->PlayerForceSighting();
            }
        }
    }
}

signed char AU_ShouldSearchForPredator() { return 0; }

signed char AU_HandleLoudNoise(StateMachine &sm)
{
    int hash = sm.GetActorOwner().GetProfile()->m_LoudNoiseState;
    sm.Gosub(GetMsgBitsForMsg(MSG_LOUD_NOISE), AP_GetStateFromHash(hash), 0, 0, 0, 0);
    return 1;
}

float Vec_AngleDifference(float a, float b);  // TODO stand-in (Vec.obj)

// NEAR: 5 bytes; retail schedules the call cleanup (add esp, 8) between the fld and the fmul.
signed char AU_PlayerObstructs(StateMachine &sm, EXVector pos, float angle)
{
    EXVector toPos = AU_SubV(sm.GetActorOwner().GetFeetPos(), pos);
    EXVector toPlayer = AU_SubV(sm.GetActorOwner().GetFeetPos(), gAIControl.m_LastSeenPlayerPos);
    if ((float)fabs(Vec_AngleDifference((float)atan2(toPos.x, toPos.z), (float)atan2(toPlayer.x, toPlayer.z))) > angle * 0.5f)
        return 0;
    return 1;
}

// NEAR: frame layout and register allocation differ (retail keeps the hit vector at esp+0x18).
// GUESS: the face-player SetRotY reads atan2((feet-head).x, (feet-head).z) with the two subtracts evaluated z-operand first; the operand pairing was inferred from the stack slots.
signed char STATE_KnockBackwardAndLive(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &timer = sm.GetIntRef(i);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXIT:
        return 1;
    case MSG_ENTER: {
        sm.GetActorOwner().InvalidateGotoPos();
        EXVector v = *(EXVector *)sm.m_Msg.m_Data[0];
        v.y = 0.0f;
        EXVector fwd((float)sin(sm.GetActorOwner().GetRotY()), 0.0f, (float)cos(sm.GetActorOwner().GetRotY()), 0.0f);
        if (v.Dot(fwd) > 0.0f) {
            switch (g_EXRandClass.Rand32() & 1) {
            case 0:
                sm.GetActorOwner().SetAnimType(0x90001c5, 0, 1);
                break;
            case 1:
                sm.GetActorOwner().SetAnimType(0x90002d6, 0, 1);
                break;
            }
            sm.GetActorOwner().SetRotY((float)atan2(v.x, v.z));
            sm.GetActorOwner().SetIsLyingFaceDown(1);
        } else {
            sm.GetActorOwner().SetRotY((float)atan2(AU_SubV(Player_GetFeetPos(), sm.GetActorOwner().GetHeadPos()).x,
                                                    AU_SubV(Player_GetFeetPos(), sm.GetActorOwner().GetHeadPos()).z));
            sm.GetActorOwner().SetAnimType(0x90002d8, 0, 1);
            v = -v;
            sm.GetActorOwner().SetRotY((float)atan2(v.x, v.z));
            sm.GetActorOwner().SetIsLyingFaceDown(0);
        }
        sm.GetActorOwner().SetDontDoHitReaction(1);
        sm.GetActorOwner().SetCanBeJuggled(0);
        sm.GetActorOwner().SetIsLyingOnGround(1);
        timer = (int)((g_EXRandClass.Randf() + 1.0f) * EXBaseApp::NominalGameFrameRate() * 2.0f);
        return 1;
    }
    case MSG_EXT:
        switch (stage) {
        case 0:
            timer -= (int)XApp::GameFrameRateMul();
            if (0.0f >= timer || sm.GetActorOwner().HasAnimEnded()) {
                if (!sm.GetActorOwner().NotAllowedToGetUp()) {
                    sm.GetActorOwner().SetDontDoHitReaction(0);
                    sm.GetActorOwner().SetCanBeJuggled(1);
                    sm.GetActorOwner().SetIsLyingOnGround(0);
                    if (sm.GetActorOwner().IsLyingFaceDown())
                        sm.GetActorOwner().SetAnimType(0x90002bf, 0, 1);
                    else
                        sm.GetActorOwner().SetAnimType(0x90002e5, 0, 1);
                    stage = 1;
                }
            }
            return 1;
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.Return(0, 0, 0, 0);
                return 1;
            }
            return 1;
        }
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

// NEAR: written for behaviour from the disassembly (the KnockBackwardAndLive entry logic, then AnimDeath).
signed char STATE_KnockBackwardAndDie(StateMachine &sm)
{
    sm.GetActorOwner().SetWatch(0);
    if (sm.m_Msg.m_Type != MSG_ENTER) return 0;
    sm.GetActorOwner().InvalidateGotoPos();
    EXVector v = *(EXVector *)sm.m_Msg.m_Data[0];
    v.y = 0.0f;
    EXVector fwd((float)sin(sm.GetActorOwner().GetRotY()), 0.0f, (float)cos(sm.GetActorOwner().GetRotY()), 0.0f);
    if (v.Dot(fwd) > 0.0f) {
        switch (g_EXRandClass.Rand32() & 1) {
        case 0:
            sm.GetActorOwner().SetAnimType(0x90001c5, 0, 1);
            break;
        case 1:
            sm.GetActorOwner().SetAnimType(0x90002d6, 0, 1);
            break;
        }
        sm.GetActorOwner().SetRotY((float)atan2(v.x, v.z));
        sm.GetActorOwner().SetIsLyingFaceDown(1);
    } else {
        sm.GetActorOwner().SetRotY((float)atan2(AU_SubV(Player_GetFeetPos(), sm.GetActorOwner().GetHeadPos()).x,
                                                AU_SubV(Player_GetFeetPos(), sm.GetActorOwner().GetHeadPos()).z));
        sm.GetActorOwner().SetAnimType(0x90002d8, 0, 1);
        v = -v;
        sm.GetActorOwner().SetRotY((float)atan2(v.x, v.z));
        sm.GetActorOwner().SetIsLyingFaceDown(0);
    }
    sm.GetActorOwner().SetIgnoreAnimChanges(1);
    SM_GOTO(STATE_AnimDeath, 0, 0, 0, 0);
    return 1;
}


signed char STATE_HitReactLyingOnGround(StateMachine &sm)
{
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (sm.GetActorOwner().HasAnimEnded()) sm.Return(0, 0, 0, 0);
        return 1;
    case MSG_ENTER:
        if (sm.GetActorOwner().IsLyingFaceDown())
            sm.GetActorOwner().SetAnimType(0x90002d0, 0, 1);
        else
            sm.GetActorOwner().SetAnimType(0x90002f2, 0, 1);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

// NEAR: logic complete; retail evaluates fsin before fcos in the facing test and keeps 2*(r+1) on the x87 stack for the get-up timer.
signed char STATE_HitReactKnockedOver(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &bFaceDown = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    float &standTimer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXIT:
        sm.GetActorOwner().SetIsLyingOnGround(0);
        sm.GetActorOwner().SetIsLyingFaceDown(0);
        sm.GetActorOwner().SetCanBeJuggled(1);
        return 1;
    case MSG_ENTER: {
        stage = 0;
        standTimer = 0.0f;
        EXVector v;
        if (sm.m_Msg.m_Data[0]) {
            const EXVector *p = (const EXVector *)sm.m_Msg.m_Data[0];
            v.x = p->x;
            v.y = p->y;
            v.z = p->z;
        } else {
            v.x = (float &)sm.m_Msg.m_Data[1];
            v.y = (float &)sm.m_Msg.m_Data[2];
            v.z = (float &)sm.m_Msg.m_Data[3];
        }
        if (v.x * (float)sin(sm.GetActorOwner().GetRotY()) + v.z * (float)cos(sm.GetActorOwner().GetRotY()) + v.y * 0.0f > 0.0f) {
            bFaceDown = 1;
            sm.GetActorOwner().SetIsLyingFaceDown(1);
        } else {
            bFaceDown = 0;
            sm.GetActorOwner().SetIsLyingFaceDown(0);
        }
        sm.GetActorOwner().SetCanBeJuggled(0);
        sm.GetActorOwner().SetIsLyingOnGround(1);
        return 1;
    }
    case MSG_EXT:
        switch (stage) {
        case 0:
            if (bFaceDown)
                sm.GetActorOwner().SetAnimType(0x90002a0, 0, 1);
            else
                sm.GetActorOwner().SetAnimType(0x90002e4, 0, 1);
            stage++;
            return 1;
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) {
                if (bFaceDown)
                    sm.GetActorOwner().SetAnimType(0x90002bc, 0, 1);
                else
                    sm.GetActorOwner().SetAnimType(0x90002e3, 0, 1);
                timer = (g_EXRandClass.Randf() + 1.0f) * 2.0f * EXBaseApp::NominalGameFrameRate();
                stage++;
            }
            return 1;
        case 2:
            if (sm.GetActorOwner().GetProfile()->m_Hash == 0x4700001d) return 1;
            timer -= XGetFrameRateMul();
            if (sm.GetActorOwner().IsPlayerStoodOnMe()) {
                if (standTimer <= 0.0f) {
                    standTimer = (g_EXRandClass.Randf() + 1.0f) * EXBaseApp::NominalGameFrameRate() * 0.1f;
                    sm.GetActorOwner().ChangeHealth(-5.0f, 1);
                    SM_GOSUB(STATE_HitReactLyingOnGround, 0, 0, 0, 0);
                    return 1;
                }
            } else if (standTimer > 0.0f) {
                standTimer -= XGetFrameRateMul();
            }
            if (timer <= 0.0f) {
                if (sm.GetActorOwner().NotAllowedToGetUp()) return 1;
                if (standTimer > 0.0f) {
                    timer = (g_EXRandClass.Randf() + 2.0f) * EXBaseApp::NominalGameFrameRate();
                    return 1;
                }
                sm.GetActorOwner().SetIsLyingOnGround(0);
                if (bFaceDown)
                    sm.GetActorOwner().SetAnimType(0x90002bf, 0, 1);
                else
                    sm.GetActorOwner().SetAnimType(0x90002e5, 0, 1);
                stage++;
            }
            return 1;
        case 3:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.Return(0, 0, 0, 0);
                return 1;
            }
            return 1;
        }
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

// NEAR: logic complete; VC duplicates the MSG_EXT return (retail jumps to the shared final return 1) and
// the later blocks shift accordingly.
signed char STATE_Barge(StateMachine &sm)
{
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (!sm.GetActorOwner().HasAnimEnded()) break;
    ret:
        sm.Return(0, 0, 0, 0);
        return 1;
    case MSG_ENTER: {
        if (sm.GetActorOwner().IsUlysses()) goto ret;
        const EXVector *p = (const EXVector *)sm.m_Msg.m_Data[0];
        EXVector v;
        v.x = p->x;
        v.y = p->y;
        v.z = p->z;
        sm.GetActorOwner().SetAnimType(0, 0, 1);
        EXVector fwd;
        fwd.x = (float)sin(sm.GetActorOwner().GetRotY());
        fwd.y = 0.0f;
        fwd.z = (float)cos(sm.GetActorOwner().GetRotY());
        EXVector side;
        side.x = -fwd.z;
        side.y = 0.0f;
        side.z = fwd.x;
        if (sm.GetActorOwner().IsBoss()) {
            if (0.0f > v.x * fwd.x + v.z * fwd.z + v.y * fwd.y)
                sm.GetActorOwner().SetAnimType(0x90004a1, 0, 1);
            else
                sm.GetActorOwner().SetAnimType(0x90004a2, 0, 1);
            return 1;
        }
        float front = v.x * fwd.x + v.z * fwd.z + v.y * fwd.y;
        float right = v.x * side.x + v.z * side.z + v.y * side.y;
        if (0.0f > front) {
            if (right > 0.0f)
                sm.GetActorOwner().SetAnimType(0x90000f1, 0, 1);
            else
                sm.GetActorOwner().SetAnimType(0x90000ca, 0, 1);
        } else if (0.0f > right) {
            sm.GetActorOwner().SetAnimType(0x90000c9, 0, 1);
        } else {
            sm.GetActorOwner().SetAnimType(0x90000bc, 0, 1);
        }
        return 1;
    }
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR)) {
            StructMsg_AttackedByPredator *pMsg = (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0];
            if (pMsg->m_Attack < 0x1f || pMsg->m_Attack > 0x21) AU_HandlePlayerAttacks(sm, pMsg);
        }
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        break;
    default:
        return 0;
    }
    return 1;
}

signed char STATE_Dazed(StateMachine &sm)
{
    int i = 0;
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(0);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        sm.GetActorOwner().SetAnimType(0x9000088, 0, 1);
        sm.GetActorOwner().PlaySFX(0x1af000f2, 0);
        timer = (g_EXRandClass.Randf() * 4.0f + 6.0f) * 60.0f;
        sm.GetActorOwner().SetIsDazed(1);
        return 1;
    case MSG_EXIT:
        sm.GetActorOwner().SetIsDazed(0);
        return 1;
    case MSG_EXT:
        timer -= XApp::GameFrameRateMul();
        if (timer <= 0.0f) sm.Return(0, 0, 0, 0);
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

signed char STATE_Special_OldBloke(StateMachine &sm)
{
    sm.GetActorOwner().SetStatus(0xb);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        sm.GetActorOwner().EnableMovement(0);
        sm.GetActorOwner().SetAnimType(0x90003b0, 0, 1);
        return 1;
    case MSG_EXIT:
        sm.GetActorOwner().EnableMovement(1);
        return 1;
    case MSG_EXT:
        return 1;
    }
    return 0;
}

signed char STATE_DeleteNow(StateMachine &sm)
{
    sm.GetActorOwner().SetStatus(0);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHealth(0.0f, 2);
    if (sm.m_Msg.m_Type != MSG_ENTER) return 0;
    sm.GetActorOwner().Delete();
    return 1;
}

signed char STATE_FS_Delete(StateMachine &sm)
{
    sm.GetActorOwner().SetStatus(0);
    if (sm.m_Msg.m_Type != MSG_ENTER) return 0;
    sm.GetActorOwner().Delete();
    if (sm.m_Msg.m_Data[0]) POI_ForceFlagChannel(sm.m_Msg.m_Data[0], sm.GetActorOwner().GetRefID());
    return 1;
}

signed char STATE_FS_Kill(StateMachine &sm)
{
    sm.GetActorOwner().SetStatus(0);
    if (sm.m_Msg.m_Type != MSG_ENTER) return 0;
    Objective_NotifyActorDeath(&sm.GetActorOwner());
    sm.GetActorOwner().Delete();
    if (sm.m_Msg.m_Data[0]) POI_ForceFlagChannel(sm.m_Msg.m_Data[0], sm.GetActorOwner().GetRefID());
    return 1;
}

signed char STATE_FS_StandStill(StateMachine &sm)
{
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(1);
    if (sm.m_Msg.m_Type != MSG_ENTER) return 0;
    sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
    return 1;
}

signed char STATE_FS_ChangeSkin(StateMachine &sm)
{
    sm.GetActorOwner().SetStatus(0xb);
    if (sm.m_Msg.m_Type != MSG_ENTER) return 0;
    sm.GetActorOwner().CheckForSkinChange(10000.0f);
    sm.Return(0, 0, 0, 0);
    return 1;
}

signed char STATE_ActorInit(StateMachine &sm) { return 0; }

signed char AU_HandleAttractNoise(StateMachine &sm)
{
    if (!sm.GetActorOwner().IsBitMsgOnStack(AW_BIT_HAVE_SEEN_PLAYER)) {
        int hash = sm.GetActorOwner().GetProfile()->m_AttractState;
        if (hash) {
            StateFn state = AP_GetStateFromHash(hash);
            sm.Gosub(GetMsgBitsForMsg(MSG_ATTRACT) | GetMsgBitsForMsg(MSG_REPEL), state, 0, 0, 0, 0);
        }
    }
    return 1;
}

signed char AU_HandleFightMe(StateMachine &sm)
{
    SM_GOSUB(STATE_FightActor, sm.m_Msg.m_Data[0], 1, 0, 0);
    return 1;
}

signed char AU_HandleDriveBy(StateMachine &sm) { return 0; }

// NEAR: retail schedules mov ecx, esi before push eax.
signed char STATE_AttackPlayerForever(StateMachine &sm)
{
    MsgType type = sm.m_Msg.m_Type;
    if (type != MSG_ENTER && type != MSG_RETURN) return 0;
    sm.Gosub(GetMsgBitsForMsg(type), STATE_AttackPred, 0, 0, 0, 0);
    return 1;
}

signed char STATE_TestAnim(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        return 1;
    case MSG_EXT:
        switch (stage) {
        case 0:
            stage = 1;
            switch (g_EXRandClass.Rand32() % 11) {
            case 0: sm.GetActorOwner().SetAnimType(0x9000378, 0, 1); return 1;
            case 1: sm.GetActorOwner().SetAnimType(0x9000379, 0, 1); return 1;
            case 2: sm.GetActorOwner().SetAnimType(0x900037c, 0, 1); return 1;
            case 3: sm.GetActorOwner().SetAnimType(0x900037b, 0, 1); return 1;
            case 4: sm.GetActorOwner().SetAnimType(0x900037d, 0, 1); return 1;
            case 5: sm.GetActorOwner().SetAnimType(0x9000376, 0, 1); return 1;
            case 6: sm.GetActorOwner().SetAnimType(0x9000377, 0, 1); return 1;
            case 7: sm.GetActorOwner().SetAnimType(0x900038b, 0, 1); return 1;
            case 8: sm.GetActorOwner().SetAnimType(0x900038c, 0, 1); return 1;
            case 9: sm.GetActorOwner().SetAnimType(0x900038a, 0, 1); return 1;
            case 10: sm.GetActorOwner().SetAnimType(0x9000389, 0, 1); return 1;
            }
            return 1;
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) stage = 0;
            return 1;
        }
        return 1;
    }
    return 0;
}

signed char AU_SetItemDraw(EXItem *pItem, signed char b)
{
    if ((pItem->m_itemFlags & 1) != b) {
        if (b) {
            pItem->m_itemFlags.BitSet(1);
            pItem->m_validMtx = 0;
            pItem->EnableParticleSystems(1);
        } else {
            pItem->m_itemFlags.BitClr(1);
            pItem->EnableParticleSystems(0);
        }
    }
    return b;
}

signed char AU_SetAnimatorDraw(EXItemAnimator *pAnim, signed char b)
{
    if ((pAnim->m_itemFlags & 1) != b) {
        if (b) {
            pAnim->m_itemFlags.BitSet(1);
            pAnim->m_validMtx = 0;
            pAnim->EnableParticleSystems(1);
        } else {
            pAnim->m_itemFlags.BitClr(1);
            pAnim->EnableParticleSystems(0);
        }
    }
    return b;
}

signed char AU_GetItemDraw(EXItem *pItem)
{
    return pItem->m_itemFlags.BitTestBool(1);
}

void AU_SetItemARGB(EXItem *pItem, float a, float r, float g, float b)
{
    if (pItem) {
        pItem->RenderInfo().EnableAlphaMult(1.0f > a);
        pItem->RenderInfo().EnableRGBMult(r != 1.0f || g != 1.0f || b != 1.0f);
        pItem->RenderInfo().SetRGBAMult(r, g, b, a);
    }
}

void AU_SetAnimatorARGB_Body(EXItemAnimator *pAnim, float a, float r, float g, float b)
{
    if (pAnim) {
        pAnim->RenderInfo().EnableAlphaMult(1.0f > a);
        pAnim->RenderInfo().EnableRGBMult(r != 1.0f || g != 1.0f || b != 1.0f);
        pAnim->RenderInfo().SetRGBAMult(r, g, b, a);
    }
}

void AU_SetAnimatorARGB_Limb(EXItemAnimator *pAnim, float a, float r, float g, float b)
{
    if (pAnim) {
        pAnim->RenderInfo().EnableAlphaMult(1.0f > a);
        pAnim->RenderInfo().EnableRGBMult(r != 1.0f || g != 1.0f || b != 1.0f);
        pAnim->RenderInfo().SetRGBAMult(r, g, b, a);
    }
}

void AU_DeleteItem(EXItem *pItem) { pItem->m_itemFlags.BitSet(0x10); }

EXVector AU_GetPosAtDistFromPlayer(float dist, int *pPoly)
{
    EXVector player = Player_GetFeetPos();
    EXVector out;
    if (gAIControl.GetDistantPos(player, dist, out, pPoly)) return out;
    return player;
}

int AU_FindClosestPathIndex(const EXVector &pos, int &node, unsigned int flags)
{
    float best = 0.0f;
    int bestPath = -1;
    int i = 0;
    for (EXGeoPath *pPath = theItemEnv.GetPath(i); pPath; pPath = theItemEnv.GetPath(++i)) {
        if (pPath->Flags() & 0x800) continue;
        if (flags && (pPath->Flags() & flags) != flags) continue;
        for (int j = 0; j < pPath->NumNodes(); j++) {
            const EXVector3 &np = pPath->Node(j)->Position();
            EXVector d = AU_SubV(EXVector(np.x, np.y, np.z, 0.0f), pos);
            float lsq = d.LengthXYZSquared();
            if (bestPath == -1 || best > lsq) {
                node = j;
                bestPath = i;
                best = lsq;
            }
        }
    }
    return bestPath;
}

void SendMsg_AttackedByPredator(Actor *pActor, int attack, const EXVector &pos, const EXVector &dir, float damage, int unk)
{
    if (pActor) {
        StructMsg_AttackedByPredator msg;
        msg.m_Attack = attack;
        msg.m_Pos.x = pos.x;
        msg.m_Pos.y = pos.y;
        msg.m_Pos.z = pos.z;
        msg.m_Pos.w = pos.w;
        msg.m_Dir.x = dir.x;
        msg.m_Dir.y = dir.y;
        msg.m_Dir.z = dir.z;
        msg.m_Dir.w = dir.w;
        msg.m_Damage = damage;
        msg.m_Unk28 = unk;
        pActor->SendMsg(MsgItem(MSG_ATTACKED_BY_PREDATOR, (int)&msg), 0.0f);
    }
}

void SendMsg_AttackedByActor(Actor *pActor, WeaponProfile *pWeapon, const EXVector &pos, const EXVector &dir, float damage, Actor *pAttacker, signed char b)
{
    if (pActor) {
        StructMsg_AttackedByActor msg;
        msg.m_pWeapon = pWeapon;
        msg.m_Pos.x = pos.x;
        msg.m_Pos.y = pos.y;
        msg.m_Pos.z = pos.z;
        msg.m_Pos.w = pos.w;
        msg.m_Dir.x = dir.x;
        msg.m_Dir.y = dir.y;
        msg.m_Dir.z = dir.z;
        msg.m_Dir.w = dir.w;
        msg.m_Damage = damage;
        msg.m_pAttacker = pAttacker;
        msg.m_Unk28 = b;
        pActor->SendMsg(MsgItem(MSG_ATTACKED_BY_ACTOR, (int)&msg), 0.0f);
    }
}

void SendMsg_IncomingAttack(Actor *pActor, int weapon, int unk, const EXVector &pos, const EXVector &dir, float f)
{
    if (pActor) {
        WeaponProfile *pWeapon = WeaponProfile::GetWeaponData(weapon);
        if (!pActor->IsBoss() && Player_IsCloaked()) {
            if (!pWeapon) return;
            if (!pWeapon->IsPredatorProjectileWeapon()) return;
        }
        StructMsg_IncomingAttack msg;
        msg.m_Weapon = weapon;
        msg.m_Unk4 = unk;
        msg.m_Pos.x = pos.x;
        msg.m_Pos.y = pos.y;
        msg.m_Pos.z = pos.z;
        msg.m_Pos.w = pos.w;
        msg.m_Dir.x = dir.x;
        msg.m_Dir.y = dir.y;
        msg.m_Dir.z = dir.z;
        msg.m_Dir.w = dir.w;
        msg.m_Unk28 = f;
        pActor->SendMsg(MsgItem(MSG_INCOMING_ATTACK, (int)&msg), 0.0f);
    }
}

int AU_SpawnActor(EXTrigger *pTrig, int b)
{
    if (pTrig->m_NavPoly < 0) pTrig->m_NavPoly = AU_NavPolyForPos(pTrig->m_Position);
    Actor *pActor = Actor::Create(pTrig->m_Data[0], pTrig->m_Position, pTrig->m_NavPoly, pTrig->m_Rotation.y, b, 0x40, 0);
    if (pActor) return pActor->GetRefID();
    return 0;
}

int AU_SpawnActorNearPlayer(int hash, int b)
{
    int poly = -1;
    EXVector player = Player_GetFeetPos();
    EXVector out;
    EXVector pos;
    if (gAIControl.GetDistantPos(player, 20.0f, out, &poly))
        pos = out;
    else
        pos = player;
    Actor *p = Actor::Create(hash, pos, poly, 0.0f, b, 0x80, 0);
    if (p) return p->m_RefID;
    return 0;
}

signed char AU_IsNavMeshValid(const EXVector &pos) { return gNavMeshHandler.GetPtr(pos, 0) != 0; }

float AU_GetFloorHeight(const EXVector &pos)
{
    XNavMesh *pMesh = gNavMeshHandler.GetPtr(pos, 0);
    if (pMesh) return pMesh->GetFloorHeight(pos);
    return 0.0f;
}

signed char STATE_ElecticShock(StateMachine &sm)
{
    int i = 0;
    EXVector &pos = sm.GetVectorRef(i);
    float &timer = sm.GetFloatRef(i);
    float &animTimer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXIT:
        sm.GetActorOwner().SetDontDoHitReaction(0);
        return 1;
    case MSG_ENTER:
        sm.GetActorOwner().SetDontDoHitReaction(1);
        timer = (g_EXRandClass.Randf() * 0.5f + 1.5f) * 60.0f;
        pos = sm.GetActorOwner().GetFeetPos();
        return 1;
    case MSG_EXT:
        timer -= XApp::GameFrameRateMul();
        if (timer <= 0.0f) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        animTimer -= XApp::GameFrameRateMul();
        if (animTimer <= 0.0f) {
            sm.GetActorOwner().SetPosition(pos, -1);
            int anims[3] = {0x90002b2, 0x90003d2, 0x90003d1};
            unsigned int r = g_EXRandClass.Rand32() % 3;
            sm.GetActorOwner().SetAnimType(anims[r], 0, 1);
            sm.GetActorOwner().SetAnimFrame(0.0f);
            animTimer = 6.0f;
            return 1;
        }
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

// NEAR: written for behaviour from the disassembly (not byte-matched). Messages 0x25/0x26 (names unknown)
// end the jump / mark it landed.
signed char STATE_MonsterJump(StateMachine &sm)
{
    int i = 0;
    EXVector &delta = sm.GetVectorRef(i);
    int &stage = sm.GetIntRef(i);
    int &timer = sm.GetIntRef(i);
    sm.GetActorOwner().SetHeadTrackMode(0);
    sm.GetActorOwner().SetWatch(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (!sm.GetActorOwner().m_ActorFlags.m_bBoss) return 1;
        switch (stage) {
        case 0:
            delta = sm.GetActorOwner().CalcJumpDelta();
            sm.GetActorOwner().SetAnimType(0x900022d, 0, 1);
            if (sm.GetActorOwner().InitTriggerJump()) {
                stage = 2;
                return 1;
            }
            sm.Return(0, 0, 0, 0);
            return 1;
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().SetAnimType(0x900022d, 0, 1);
                if (sm.GetActorOwner().InitTriggerJump()) {
                    stage = 2;
                    return 1;
                }
                sm.Return(0, 0, 0, 0);
                return 1;
            }
            break;
        case 2:
            if (--timer < 0) {
                sm.GetActorOwner().m_NavRoute.RecoverFromBrokenJump(sm.GetActorOwner().m_OrgLoc);
                stage = 3;
                return 1;
            }
            break;
        case 3:
            switch (sm.GetActorOwner().GetProfile()->m_SubGroup) {
            case 0x4f00000c:
            case 0x4f00000e:
            case 0x4f000011:
                if (1.5f > delta.y || delta.LengthXZSquared() > 25.0f) {
                    sm.GetActorOwner().SetAnimType(0x900022e, 0, 1);
                    stage = 4;
                    return 1;
                }
                goto endJump;
            case 0x4f000014:
            case 0x4f000015:
            case 0x4f000016:
            case 0x4f000017:
                sm.GetActorOwner().SetAnimType(0x900022f, 0, 1);
                stage = 4;
                return 1;
            default:
                sm.GetActorOwner().SetAnimType(0x9000268, 0, 1);
                stage = 4;
                return 1;
            }
        case 4:
            if (sm.GetActorOwner().HasAnimEnded()) {
            endJump:
                sm.GetActorOwner().m_NavRoute.EndJump();
                sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
                sm.Return(0, 0, 0, 0);
                return 1;
            }
            break;
        }
        return 1;
    case MSG_ENTER:
        stage = 0;
        timer = (int)(EXBaseApp::FrameRate() * 3.0f);
        if (!sm.GetActorOwner().m_ActorFlags.m_bBoss) sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        return 1;
    case 0x25:
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x25)) return 1;
        if (stage >= 3) {
            sm.GetActorOwner().m_NavRoute.EndJump();
            stage = 0;
        }
        return 1;
    case 0x26:
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x26)) return 1;
        if (stage == 2) stage = 3;
        return 1;
    }
    return 0;
}

// NEAR: retail keeps the constant 1 in ebx (FlagChannels stores, SetCloak, return value).
signed char STATE_NotReallyDead(StateMachine &sm)
{
    if (sm.m_Msg.m_Type != MSG_ENTER) return 0;
    sm.GetActorOwner().SetHealth(0.1f, 2);
    sm.GetActorOwner().SetCannotDie(1);
    sm.GetActorOwner().SetDontDoHitReaction(1);
    signed char on = 1;
    switch (sm.GetActorOwner().GetProfileHash()) {
    case 0x47000245:
    case 0x47000247:
        FlagChannels[0xd] = on;
        sm.Return(0, 0, 0, 0);
        return on;
    case 0x47000238:
        FlagChannels[0x85] = on;
        sm.GetActorOwner().SetCloak(on);
        break;
    case 0x47000159:
        FlagChannels[0xde] = on;
        sm.Return(0, 0, 0, 0);
        return on;
    case 0x4700027a:
        FlagChannels[0x87] = on;
        sm.GetActorOwner().SetCloak(on);
        break;
    case 0x47000279:
        FlagChannels[0x86] = on;
        sm.GetActorOwner().SetCloak(on);
        break;
    }
    sm.Return(0, 0, 0, 0);
    return on;
}

// NEAR: written for behaviour from the disassembly (not byte-matched). An overkilled body leaves a blood
// pool when the floor is level (three probes 1 m out, 120 degrees apart, within 0.1 m of each other).
signed char STATE_FadeOutDead(StateMachine &sm)
{
    int i = 0;
    float &alpha = sm.GetFloatRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHealth(0.0f, 2);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER: {
        sm.GetActorOwner().m_NavRoute.DisableJumping(1);
        sm.GetActorOwner().InvalidateGotoPos();
        sm.GetActorOwner().SetCarry(0, 1);
        sm.GetActorOwner().SetCarry(0, 0);
        if (!sm.GetActorOwner().m_ActorFlags.m_bUlysses) sm.GetActorOwner().m_StateFlags.m_bCanCollideWithPlayer = 0;
        g_Formations.FreeSlot(sm.GetActorOwner().m_RefID);
        alpha = 1.0f;
        timer = 240.0f;
        Objective_NotifyActorDeath(&sm.GetActorOwner());
        if (sm.GetActorOwner().m_ActorFlags.m_bOverkilled) {
            float lastH = 0.0f;
            int n = 0;
            do {
                EXMaths::mat_44 m;
                m.set_rotate_y(n * 2.09439516f);
                EXMaths::vec_xyzw fwd;
                fwd.set(0.0f, 0.0f, 1.0f, 0.0f);
                EXMaths::vec_xyzw off = m * fwd;
                EXVector p = sm.GetActorOwner().GetItem()->m_position + EXVector(off);
                float h;
                XNavMesh *pMesh = gNavMeshHandler.GetPtr(p, 0);
                if (pMesh) h = pMesh->GetFloorHeight(p);
                else h = 0.0f;
                if (n > 0 && (float)fabs(h - lastH) > 0.1f) goto noPool;
                lastH = h;
            } while (++n < 3);
            {
                EXVector pos = sm.GetActorOwner().GetItem()->m_position;
                pos.y += 1.0f;
                Gore_BloodPool(pos, Gore_GetBloodGroup(&sm.GetActorOwner()));
            }
        }
    noPool:
        sm.GetActorOwner().UpdateThrownDistStat();
        EXVector d = AU_SubV(XGetCamera(0)->m_CameraPos, sm.GetActorOwner().GetItem()->m_position);
        if (!sm.GetActorOwner().m_StateFlags.m_bOnScreen && d.LengthXYZSquared() >= 100.0f) sm.GetActorOwner().Delete();
        return 1;
    }
    case MSG_EXT:
        sm.GetActorOwner().m_AnimSpeedMult = 0.0f;
        timer -= XApp::GameFrameRateMul();
        if (timer <= 0.0f) {
            if (sm.GetActorOwner().m_ActorFlags.m_bUlysses) sm.GetActorOwner().m_StateFlags.m_bCanCollideWithPlayer = 0;
            alpha -= XGetFrameRateMul() * kFreeFallLandDist;
            if (0.0f > alpha) alpha = 0.0f;
            sm.GetActorOwner().SetAlpha(alpha);
            if (alpha <= 0.0f) sm.GetActorOwner().Delete();
        }
        return 1;
    }
    return 0;
}


// NEAR: written for behaviour from the disassembly (not byte-matched). Slerps a towards b by t scaled by
// the inverse distance between their forward vectors (clamped to [0, 1]).
EXQuaternion AU_InterpQuat(EXQuaternion &a, EXQuaternion &b, float t)
{
    EXVector fwd(0.0f, 0.0f, 1.0f, 0.0f);
    EXVector fa = EXVector(a.MultiplyVector(fwd));
    EXVector fb = EXVector(b.MultiplyVector(fwd));
    EXVector d = AU_Sub(fa, fb);
    float s = d.LengthXYZ() * t;
    if (s > 0.0f) {
        float r = 1.0f / s;
        if (r >= 1.0f) t = 1.0f;
        else if (0.0f < r) t = r;
        else t = 0.0f;
    } else {
        t = 0.0f;
    }
    return EXQuaternion(a.slerp(b, t));
}

signed char STATE_MonsterGotoLocation(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &flags = sm.GetIntRef(i);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXIT:
        flags = 0;
        return 1;
    case MSG_ENTER:
        poi = sm.m_Msg.m_Data[0];
        flags = sm.m_Msg.m_Data[1];
        if (!poi) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        sm.GetActorOwner().GotoPOI(poi);
        if (flags & 0x200) {
            sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
            return 1;
        }
        sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
        return 1;
    case MSG_EXT:
        if (1.0f > sm.GetActorOwner().SqDistToTarget()) sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        return 1;
    case 0x25:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x25)) SM_GOSUB(STATE_MonsterJump, 0, 0, 0, 0);
        return 1;
    }
    return 0;
}

int AU_GetNavZone(int poly)
{
    XNavMesh *pMesh = gNavMeshHandler.GetPtr(poly);
    if (pMesh) return pMesh->PolyZone(XNavMeshHandler::PolyID(poly));
    return -1;
}

EXDatumTemp Player_GetHandDatum()
{
    EXDatumTemp d;
    memset(&d, 0, sizeof(d));
    if (gpPlayerItem[0])
        gpPlayerItem[0]->GetAnimatorDatum(0x10000006, d, 1, 0);
    else
        memset(&d, 0, sizeof(d));
    return d;
}

// NEAR: logic complete; retail schedules the eye-vector build differently and compares the line-of-sight
// results against a zero register (cmp al, bl).
void SendMsg_HealingRoar(const EXVector &pos, float radius)
{
    for (Actor *p = Actor::GetFirst(pos, radius, 0); p; p = p->GetNext(pos, radius, 0)) {
        EXVector eye;
        eye.x = pos.x;
        eye.y = pos.y + 0.1f;
        eye.z = pos.z;
        eye.w = pos.w;
        if (!CheckLineOfSight(eye, p->GetFeetPos())) {
            eye.y += 0.4f;
            if (!CheckLineOfSight(eye, p->GetPelvisPos())) {
                eye.y += 0.7f;
                if (!CheckLineOfSight(eye, p->GetHeadPos())) continue;
            }
        }
        SendMsg_AttackedByPredator(p, 0x4a, pos, EXVector(0.0f, 0.0f, 0.0f, 0.0f), 0.0f, 0);
    }
}

void AU_BlockLoadGeoSubSection(int file, int hash)
{
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(file, 0, 0);
    if (!pFile) return;
    EXGeoHeader *pHeader = pFile->m_pGeoHeader;
    if (!pHeader) return;
    u32 sec = hash;
    if (hash < 0x8000001 || hash > 0x800007f) {
        EXGeoObjectHeader *pObj = pHeader->GeoObjectHeader(hash);
        if (!pObj) return;
        EXGeoSectionHeader *pSec = pHeader->SectionHeader(pObj->Section());
        if (!pSec) return;
        sec = pSec->HashCode();
    }
    if (sec && sec != 0x8000000 && !pHeader->IsSectionLoaded(sec, 1)) pHeader->LoadSectionFile(sec, 0, 0);
}

void AU_BlockDeloadGeoSubSection(int file, int hash)
{
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(file, 0, 0);
    if (!pFile) return;
    EXGeoHeader *pHeader = pFile->m_pGeoHeader;
    if (!pHeader) return;
    if (hash < 0x8000001 || hash > 0x800007f) {
        EXGeoObjectHeader *pObj = pHeader->GeoObjectHeader(hash);
        if (!pObj) return;
        EXGeoSectionHeader *pSec = pHeader->SectionHeader(pObj->Section());
        if (!pSec) return;
        hash = pSec->HashCode();
    }
    if (!hash) return;
    if (hash == 0x8000000) return;
    pHeader->DeLoadSectionFile(hash);
}

void *AU_GetWorkSheet(int file, int sheet, int col, int &count)
{
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(file, 0, 0);
    if (!pFile) return 0;
    if (!pFile->m_pGeoHeader) return 0;
    EXGeoSpreadSheet *pSheet = pFile->m_pGeoHeader->GeoSpreadSheet(sheet);
    if (!pSheet) return 0;
    count = pSheet->DataSheetSize(col);
    return pSheet->DataSheetData(col);
}

signed char STATE_RagDollDeath(StateMachine &sm)
{
    int i = 0;
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHealth(0.0f, 2);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        sm.GetActorOwner().DamageOtherActorsIfRagdoll();
        timer -= XApp::GameFrameRateMul();
        if (0.0f > timer || sm.GetActorOwner().HasRagDollComeToRest()) {
            SM_GOTO(STATE_FadeOutDead, 0, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_ENTER:
        sm.GetActorOwner().m_NavRoute.DisableJumping(1);
        sm.GetActorOwner().InvalidateGotoPos();
        sm.GetActorOwner().SetCarry(0, 1);
        sm.GetActorOwner().SetCarry(0, 0);
        sm.GetActorOwner().SetCloak(0);
        timer = 1800.0f;
        return 1;
    case MSG_EXIT:
        return 1;
    }
    return 0;
}

signed char STATE_AnimDeath(StateMachine &sm)
{
    int i = 0;
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0xe);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHealth(0.0f, 2);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        timer -= XApp::GameFrameRateMul();
        if (0.0f > timer || sm.GetActorOwner().HasAnimEnded()) {
            sm.GetActorOwner().m_AnimSpeedMult = 0.0f;
            SM_GOTO(STATE_FadeOutDead, 0, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_ENTER:
        sm.GetActorOwner().m_NavRoute.DisableJumping(1);
        sm.GetActorOwner().InvalidateGotoPos();
        sm.GetActorOwner().SetCarry(0, 1);
        sm.GetActorOwner().SetCarry(0, 0);
        sm.GetActorOwner().SetCloak(0);
        timer = sm.GetActorOwner().IsUlysses() ? 600.0f : 300.0f;
        return 1;
    }
    return 0;
}

// GUESS: name. Retail reads this table from unnamed ActorUtil data (0x396b98).
static const int s_ArmlessDeathAnims[4] = {0x9000058, 0x9000058, 0x90002b9, 0x90002ac};

// NEAR: written for behaviour from the disassembly (not byte-matched). Message 0x34 (name unknown) starts
// the head-rip finisher; stage 0 jumps into the same block when the death anim ends.
signed char STATE_DeathBoss(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &bTaunted = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXIT:
        return 1;
    case MSG_ENTER:
        sm.GetActorOwner().m_NavRoute.DisableJumping(1);
        sm.GetActorOwner().SetDontDoHitReaction(0);
        sm.GetActorOwner().InvalidateGotoPos();
        if (sm.GetActorOwner().GetSkin()->m_Hash != 0xd0000b9) {
            sm.GetActorOwner().SetCarry(0, 1);
            sm.GetActorOwner().SetCarry(0, 0);
        }
        sm.GetActorOwner().SetAnimType(0x90002ef, 0, 1);
        stage = 0;
        timer = 0.0f;
        return 1;
    case MSG_EXT:
        switch (stage) {
        case 0:
            if (sm.GetActorOwner().HasAnimEnded()) goto rip;
            break;
        case 1:
            if (sm.GetActorOwner().StartBossHeadRipped()) {
                sm.GetActorOwner().SpeakStop();
                sm.GetActorOwner().SetCanRipBossHead(0);
                int r = g_EXRandClass.Rand32() & 1;
                if (r) sm.GetActorOwner().SetAnimType(0x90002f9, 0, 1);
                else sm.GetActorOwner().SetAnimType(0x9000546, 0, 1);
                stage = 2;
                return 1;
            }
            if (!bTaunted) {
                EXVector d = AU_SubV(sm.GetActorOwner().GetItem()->m_position, Player_GetFeetPos());
                if (36.0f > d.LengthXYZSquared() && sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f000013) {
                    bTaunted = 1;
                    sm.GetActorOwner().Speak(0x1af003e6, 0);
                    return 1;
                }
            }
            break;
        case 2:
            if (sm.GetActorOwner().HasLostLimb(2)) {
                sm.GetActorOwner().AwardDeathObjective();
                timer = EXBaseApp::NominalGameFrameRate() * 3.0f;
                stage = 3;
                return 1;
            }
            break;
        case 3:
            timer -= XGetFrameRateMul();
            if (timer <= 0.0f) {
                Gore_CreateOverkillBlood(&sm.GetActorOwner());
                stage = 4;
                return 1;
            }
            break;
        case 4:
            if (sm.GetActorOwner().GetAnimFrame() > 0.9f) sm.GetActorOwner().SetAnimFrame(0.99f);
            break;
        }
        return 1;
    case 0x34:
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x34)) return 1;
        if ((sm.GetActorOwner().m_AnimMode == 0x90002f9 || sm.GetActorOwner().m_AnimMode == 0x9000546) &&
            !sm.GetActorOwner().HasLostLimb(2)) {
        rip:
            sm.GetActorOwner().SetStartBossHeadRipped(0);
            sm.GetActorOwner().SetCanRipBossHead(1);
            sm.GetActorOwner().SetAnimType(0x90002f0, 0, 1);
            stage = 1;
            return 1;
        }
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

// NEAR: logic complete; VC caches the actor pointer in edi through the default-skin branch (retail reloads it
// from sm after each call and uses edi only in the AnimDeath tail).
signed char STATE_DeathBullet(StateMachine &sm)
{
    int i = 0;
    int &ragdoll = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHealth(0.0f, 2);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (ragdoll) {
            ragdoll = 0;
            sm.GetActorOwner().EnableRagDoll();
            SM_GOTO(STATE_RagDollDeath, 0, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_ENTER:
        sm.GetActorOwner().m_NavRoute.DisableJumping(1);
        sm.GetActorOwner().InvalidateGotoPos();
        sm.GetActorOwner().SetCarry(0, 1);
        sm.GetActorOwner().SetCarry(0, 0);
        switch (sm.GetActorOwner().GetSkin()->m_SkelHash) {
        case 0xd00008d:
            if (sm.GetActorOwner().GetSkin()->m_Hash == 0xd000101)
                sm.GetActorOwner().SetAnimType(0x9000256, 0, 1);
            else
                sm.GetActorOwner().SetAnimType(0x9000294, 0, 1);
            sm.GetActorOwner().m_OrgLoc.SetPushRadius(2.0f);
            break;
        case 0xd000091: {
            int anims[2];
            anims[0] = 0x9000257;
            anims[1] = 0x9000258;
            sm.GetActorOwner().SetAnimType(anims[g_EXRandClass.Rand32() & 1], 0, 1);
            break;
        }
        case 0xd000103: {
            int anims[4];
            anims[0] = 0x90004b3;
            anims[1] = 0x90004b4;
            anims[2] = 0x90004b5;
            anims[3] = 0x90004c0;
            sm.GetActorOwner().SetAnimType(anims[g_EXRandClass.Rand32() & 3], 0, 1);
            break;
        }
        default:
            if (sm.GetActorOwner().m_StateFlags.m_bLyingOnGround) {
                if (sm.GetActorOwner().m_StateFlags.m_bLyingFaceDown)
                    sm.GetActorOwner().SetAnimType(0x90002d0, 0, 1);
                else
                    sm.GetActorOwner().SetAnimType(0x90002f2, 0, 1);
            } else if (!sm.GetActorOwner().m_ActorFlags.m_bLeftArmMissing &&
                       !sm.GetActorOwner().m_ActorFlags.m_bRightArmMissing) {
                int anims[3];
                anims[0] = 0x9000058;
                anims[1] = 0x90002b9;
                anims[2] = 0x90002ac;
                sm.GetActorOwner().SetAnimType(anims[g_EXRandClass.Rand32() % 3], 0, 1);
            } else {
                int r = g_EXRandClass.Rand32() & 1;
                if (r && sm.GetActorOwner().m_AnimType == 0x4e000003)
                    sm.GetActorOwner().SetAnimType(0x90004a6, 0, 1);
                else
                    sm.GetActorOwner().SetAnimType(s_ArmlessDeathAnims[g_EXRandClass.Rand32() & 3], 0, 1);
            }
            break;
        }
        if (!ragdoll) {
            sm.GetActorOwner().m_HitCountdown = 10.0f;
            sm.GetActorOwner().Speak(sm.GetActorOwner().GetDeathSFX(), 0);
            sm.GetActorOwner().SetHealth(0.0f, 2);
            sm.GetActorOwner().SetIgnoreAnimChanges(1);
            SM_GOTO(STATE_AnimDeath, 0, 0, 0, 0);
        }
        return 1;
    }
    return 0;
}

// NEAR: written for behaviour from the disassembly (not byte-matched). Messages 9/0xa names unknown.
signed char STATE_PickUpDroppedItem(StateMachine &sm)
{
    int i = 0;
    int &item = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xb);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (ActorCarryable::GetPtrFromID(item) && ActorCarryable::GetPtrFromID(item)->m_Owner &&
            (ActorCarryable::GetPtrFromID(item) ? ActorCarryable::GetPtrFromID(item)->m_Owner : 0) != sm.GetActorOwner().m_RefID) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        switch (stage) {
        case 0:
            if (0.25f > sm.GetActorOwner().SqDistToTarget()) {
                stage = 1;
                sm.GetActorOwner().SetAnimType(0x90000ea, 0, 1);
                return 1;
            }
            break;
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.Return(0, 0, 0, 0);
                return 1;
            }
            break;
        }
        return 1;
    case MSG_ENTER: {
        stage = 0;
        item = sm.m_Msg.m_Data[0];
        EXVector pos = ActorCarryable::GetPtrFromID(item) ? ActorCarryable::GetPtrFromID(item)->GetWorldPos()
                                                          : EXVector(0.0f, 0.0f, 0.0f, 0.0f);
        sm.GetActorOwner().GotoPosSlow(pos);
        sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
        return 1;
    }
    case MSG_EXIT:
        return 1;
    case 0xa:
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0xa)) return 1;
        sm.GetActorOwner().SetCarry(item, 0);
        return 1;
    case 9:
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)9)) return 1;
        if (ActorCarryable::GetPtrFromID(item) && ActorCarryable::GetPtrFromID(item)->m_Type == 8) {
            if (sm.GetActorOwner().GetCarryPtr(0)) sm.GetActorOwner().GetCarryPtr(0)->Delete();
            sm.GetActorOwner().SetCarry(0, 0);
        }
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// NEAR: written for behaviour from the disassembly (not byte-matched). Waits in a customer queue: walks up
// to its stand slot, faces the customer ahead, and shuffles forward when the slot ahead frees up.
signed char STATE_GotoLocationQueue(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &flags = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    float &waitTimer = sm.GetFloatRef(i);
    float &timeout = sm.GetFloatRef(i);
    sm.GetActorOwner().SetHeadTrackMode(1);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        if (sm.m_Msg.m_Data[0]) return 1;
        sm.Return(0, 0, 0, 0);
        return 1;
    case MSG_EXIT:
        POI_CustomerUnbind(poi, sm.GetActorOwner().m_RefID);
        return 1;
    case MSG_ENTER:
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        flags = sm.m_Msg.m_Data[1] & 0xfffffb09;
        if (!poi) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        SM_GOSUB(STATE_GotoLocation, poi, flags | 1, 0, 0);
        return 1;
    case MSG_EXT:
        switch (stage) {
        case 0: {
            int stand = POI_CustomerStandIndex(poi, sm.GetActorOwner().m_RefID);
            int ahead = stand - 1;
            int aheadID = POI_CustomerGetID(poi, ahead);
            sm.GetActorOwner().GotoCustomerPOI(poi, -1);
            if (kArriveSqDist > sm.GetActorOwner().SqDistToTarget()) {
                waitTimer = (g_EXRandClass.Randf() * 0.8f + 0.2f) * EXBaseApp::NominalGameFrameRate();
                if (!stand) {
                    stage = 1;
                    timeout = 600.0f;
                    return 1;
                }
                sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
                if (sm.GetActorOwner().IsFacingYRot(POI_CustomerStandPos(poi, ahead), 5.0f))
                    sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                else
                    sm.GetActorOwner().SetTargetYRot(POI_CustomerStandPos(poi, ahead), 5.0f);
                return 1;
            }
            Actor *pAhead = Actor::GetPtrFromID(aheadID);
            if (!pAhead) {
                sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
                return 1;
            }
            EXVector d = AU_SubV(POI_CustomerStandPos(poi, stand), pAhead->GetItem()->m_position);
            if (d.LengthXYZSquared() > kArriveSqDist) {
                waitTimer -= XGetFrameRateMul();
                if (0.0f > waitTimer) {
                    sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
                    waitTimer = (g_EXRandClass.Randf() * 0.8f + 0.2f) * EXBaseApp::NominalGameFrameRate();
                }
            }
            break;
        }
        case 1:
            timeout -= XApp::GameFrameRateMul();
            if (timeout <= 0.0f) {
                sm.Return(1, 0, 0, 0);
                return 1;
            }
            break;
        }
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}


// NEAR: written for behaviour from the disassembly (not byte-matched). Flags (data[1]): 0x1/0x2 customer/vendor
// bind, 0x4 customer needs the vendor there, 0x10 face the POI on arrival, 0x20 bind, 0x40/0x4000/0x8000/0x10000
// arrive early (any distance / 1 / 2 / 5 m), 0x80 pick the binding, 0x100 busy, 0x200 run, 0x800 deaf,
// 0x1000 keep idling when done, 0x2000 nudge others, 0x20000 drop carry on suspend. Names of messages
// 0x14/0x15/0x28/0x29/0x2c are not known. GUESS: the per-carry-type switch shape (retail tests are read
// faithfully, the grouping of types into cases is inferred from the jump table).
signed char STATE_GotoLocation(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    int &flags = sm.GetIntRef(i);
    int &bound = sm.GetIntRef(i);
    int &result = sm.GetIntRef(i);
    float &timeout = sm.GetFloatRef(i);
    float &nudgeTimer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetHeadTrackMode(1);
    if (flags & 0x800) {
        sm.GetActorOwner().SetWatch(0);
        switch (sm.m_Msg.m_Type) {
        case MSG_MUGGING:
        case MSG_SOMEONE_UNDER_ATTACK:
        case MSG_SOMEONE_SEEN_PREDATOR:
        case MSG_LOUD_NOISE:
        case MSG_ATTRACT:
        case MSG_FIGHT_ME:
            return 1;
        }
    }
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER: {
        stage = 0;
        timeout = 1800.0f;
        poi = sm.m_Msg.m_Data[0];
        flags = sm.m_Msg.m_Data[1];
        if (!poi) goto fail;
        signed char bThere;
        {
            EXVector d = AU_SubV(sm.GetActorOwner().GetItem()->m_position, POI_Get(poi)->m_Pos);
            if (kArriveSqDist > d.LengthXYZSquared()) {
                bThere = 1;
            } else {
                bThere = 0;
                sm.GetActorOwner().GotoPOI(poi);
                sm.GetActorOwner().SetAnimType((flags & 0x200) ? 0x4e000003 : 0x4e000002, 0, 1);
            }
        }
        if (flags & 0x20) {
            if (flags & 0x80) {
                flags &= ~0x80;
                if (POI_VendorBind(poi, sm.GetActorOwner().m_RefID)) flags |= 2;
                else if (POI_CustomerBind(poi, sm.GetActorOwner().m_RefID)) flags |= 1;
                else goto fail;
            }
            if (flags & 2) {
                if (!POI_VendorBind(poi, sm.GetActorOwner().m_RefID)) goto fail;
                if (bThere) goto arrived;
                sm.GetActorOwner().GotoVendorPOI(poi, -1);
                goto queue;
            }
            if (flags & 1) {
                signed char bVendor;
                if (flags & 4) bVendor = POI_VendorIsThere(poi);
                else bVendor = 1;
                if (!POI_CustomerBind(poi, sm.GetActorOwner().m_RefID)) goto fail;
                if (!bVendor) goto fail;
                if (bThere) goto arrived;
                sm.GetActorOwner().GotoCustomerPOI(poi, -1);
                goto queue;
            }
        }
        if (bThere) {
        arrived:
            stage = 2;
            result = 1;
            return 1;
        }
    queue: {
        POI_STRUCT *p = POI_Get(poi);
        if (p->m_QueueDist > 0 && !(flags & 2)) {
            Actor &a = sm.GetActorOwner();
            int q = POI_GetNearestType(p->m_Pos, 0x4200009b, 0, a.m_OrgLoc.NavZone(), a.CanJump(), 0);
            if (q) {
                EXVector d = AU_SubV(p->m_Pos, POI_Get(q)->m_Pos);
                if (!((float)(p->m_QueueDist * p->m_QueueDist) < d.LengthXYZSquared())) {
                    SM_GOSUB(STATE_GotoLocationQueue, q, flags, 0, 0);
                    return 1;
                }
            }
        }
        return 1;
    }
    }
    case MSG_EXIT:
        return 1;
    case MSG_EXT:
        if (flags & 0x2000) {
            nudgeTimer -= XApp::GameFrameRateMul();
            if (nudgeTimer <= 0.0f) {
                nudgeTimer = 60.0f;
                Actor::SendMsg(MsgItem((MsgType)0x2c, sm.GetActorOwner().m_RefID), sm.GetActorOwner().GetItem()->m_position, 10.0f,
                               &sm.GetActorOwner(), 0.0f);
            }
        }
        switch (stage) {
        case 0:
            if (100.0f > sm.GetActorOwner().SqDistToTarget()) {
                sm.GetActorOwner().SetAnimType((flags & 0x200) ? 0x4e000003 : 0x4e000002, 0, 1);
                if (flags & 0x10) {
                    if (kArriveSqDist > sm.GetActorOwner().SqDistToTarget()) {
                        sm.GetActorOwner().SetRotY(POI_Get(poi)->m_YRot);
                        stage = 2;
                    }
                } else {
                    int stand;
                    if (flags & 2) stand = POI_VendorStandIndex(poi, sm.GetActorOwner().m_RefID);
                    else if (flags & 1) stand = POI_CustomerStandIndex(poi, sm.GetActorOwner().m_RefID);
                    else stand = -1;
                    if (stand != -1) {
                        if (kArriveSqDist > sm.GetActorOwner().SqDistToTarget()) stage = 2;
                    } else {
                        if (flags & 0x80) {
                            flags &= ~0x80;
                            if (POI_VendorBind(poi, sm.GetActorOwner().m_RefID)) flags |= 2;
                            else if (POI_CustomerBind(poi, sm.GetActorOwner().m_RefID)) flags |= 1;
                            else goto fail;
                        }
                        if (flags & 2) {
                            if (!POI_VendorBind(poi, sm.GetActorOwner().m_RefID)) goto fail;
                            sm.GetActorOwner().GotoVendorPOI(poi, -1);
                            bound = 1;
                        } else if (flags & 1) {
                            signed char bVendor;
                            if (flags & 4) bVendor = POI_VendorIsThere(poi);
                            else bVendor = 1;
                            if (!POI_CustomerBind(poi, sm.GetActorOwner().m_RefID)) goto fail;
                            if (!bVendor) goto fail;
                            sm.GetActorOwner().GotoCustomerPOI(poi, -1);
                            bound = 1;
                        }
                    }
                }
                if (flags & 0x40) {
                    stage = 2;
                    return 1;
                }
                if ((flags & 0x10000) && 25.0f > sm.GetActorOwner().SqDistToTarget()) {
                    stage = 2;
                    return 1;
                }
                if ((flags & 0x8000) && 4.0f > sm.GetActorOwner().SqDistToTarget()) {
                    stage = 2;
                    return 1;
                }
                if ((flags & 0x4000) && 1.0f > sm.GetActorOwner().SqDistToTarget()) {
                    stage = 2;
                    return 1;
                }
            }
            break;
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 0;
                return 1;
            }
            break;
        case 2:
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            stage = 3;
        case 3:
            timeout -= XApp::GameFrameRateMul();
            if (timeout <= 0.0f) {
                POI_Unbind(poi, sm.GetActorOwner().m_RefID);
                stage = 4;
                result = 0;
                return 1;
            }
            if (POI_AnimReady(poi, (signed char)(flags & 2))) {
                if (!POI_ForceState(poi, flags, sm.GetActorOwner().m_RefID)) {
                    stage = 4;
                    result = 1;
                    return 1;
                }
            } else {
                sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                return 1;
            }
            break;
        case 4:
            if (flags & 0x1000) {
                sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                return 1;
            }
            sm.Return(result, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_SUSPEND:
        if (flags & 0x20000) sm.GetActorOwner().SetCarry(0, 0);
        return 1;
    AU_DEFAULT_MSGS
    case 0x28:
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x28)) return 1;
        if (!(flags & 0x100) && !bound && sm.m_Msg.m_Data[0]) {
            poi = sm.m_Msg.m_Data[0];
            flags = 0x1110;
            sm.GetActorOwner().GotoPOI(poi);
            sm.m_Unk9c = sm.GetActorOwner().m_RefID;
            return 1;
        }
        sm.m_Unk9c = 0;
        return 1;
    case 0x29:
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x29)) return 1;
        Actor::SendMsg(MsgItem(MSG_SOMEONE_UNDER_ATTACK), sm.GetActorOwner().GetItem()->m_position, 10.0f,
                       Actor::GetPtrFromID(sm.m_Msg.m_Data[0]), 0.0f);
        SM_GOSUB(STATE_DeathBullet, 0, 0, 0, 0);
        return 1;
    case 0x14:
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x14)) return 1;
        if (!(flags & 0x100) && !bound && !(sm.GetActorOwner().m_Attrib & 0x10)) {
            signed char bPick = 0;
            int type = ActorCarryable::GetPtrFromID(sm.m_Msg.m_Data[0]) ? (int)ActorCarryable::GetPtrFromID(sm.m_Msg.m_Data[0])->m_Type : 0;
            switch (type) {
            case 1:
            case 3:
            case 4:
            case 5:
            case 6:
            case 9:
                if (sm.GetActorOwner().GetProfile()->m_Group == 0x4b000009) bPick = 1;
                if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f000001) bPick = 1;
                break;
            case 2:
            case 7:
                if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f000001) bPick = 1;
                break;
            case 8:
                bPick = 1;
                break;
            }
            if (bPick) {
                sm.m_Unk9c = 1;
                SM_GOSUB(STATE_PickUpDroppedItem, sm.m_Msg.m_Data[0], 0, 0, 0);
                return 1;
            }
        }
        sm.m_Unk9c = 0;
        return 1;
    case 0x15:
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x15)) return 1;
        stage = 4;
        result = 0;
        return 1;
    }
    return 0;
fail:
    stage = 4;
    result = 0;
    return 1;
}

signed char STATE_DeathJuggleSlice(StateMachine &sm)
{
    sm.GetActorOwner().SetStatus(0);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHealth(0.0f, 1);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (sm.GetActorOwner().HasAnimEnded()) {
            sm.GetActorOwner().SetAnimType(0x90004fc, 0, 1);
            sm.GetActorOwner().SetIgnoreAnimChanges(1);
            SM_GOTO(STATE_AnimDeath, 0, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_ENTER:
        sm.GetActorOwner().InvalidateGotoPos();
        sm.GetActorOwner().SetCarry(0, 1);
        sm.GetActorOwner().SetCarry(0, 0);
        sm.GetActorOwner().SetAnimType(0x90004fd, 0, 1);
        Gore_CreateOverkillBlood(&sm.GetActorOwner());
        return 1;
    }
    return 0;
}


// NEAR: the inlined CanJump keeps the actor pointer in eax (retail clobbers and reloads it); otherwise matches
signed char STATE_MonsterTest(StateMachine &sm)
{
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER: {
        sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        int loc = POI_GetNearestType(sm.GetActorOwner().GetFeetPos(), 0x42000049, 0, sm.GetActorOwner().GetNavZone(),
                                     sm.GetActorOwner().CanJump(), 0);
        sm.GetLocalData()[0] = loc;
        if (!loc) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        SM_GOTO(STATE_MonsterGotoLocation, loc, 0x200, 0, 0);
        return 1;
    }
    }
    return 0;
}

// NEAR: logic complete; retail inlines the feet - nav-location subtract through two temp copies and hoists the d.y test ahead of the x/z subtracts.
signed char AR_ReleaseFromPredator(StateMachine &sm)
{
    EXVector d = AU_SubV(sm.GetActorOwner().GetFeetPos(), sm.GetActorOwner().GetNavLocation());
    if (2.0f > d.y && 0.01f > d.LengthXZ()) {
        if (SubGroupAnims::GetSubGroupAnims(sm.GetActorOwner().m_SubGroupAnimIndex).m_Anims[6])
            sm.GetActorOwner().SetCloak(1);
        sm.GetActorOwner().SetCanCollideWithPlayer(1);
        sm.GetActorOwner().SetIsTargetable(1);
        sm.GetActorOwner().EnableMovement(1);
        sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        sm.Return(0, 0, 0, 0);
        return 1;
    }
    sm.GetActorOwner().EnableRagDoll();
    EXVector force(0.0f, -1.0f, 0.0f, 0.0f);
    sm.GetActorOwner().ApplyRagDollForce(sm.GetActorOwner().GetPelvisPos(), force, 1.0f, -1);
    sm.GetActorOwner().SetHealth(0.0f, 1);
    SM_GOSUB(STATE_RagDollDeath, 0, 0, 0, 0);
    return 1;
}

// Speak(GetDeathSFX(), 0) on the owner, shared by the AR_* death reactions.
static __forceinline void AU_SpeakDeath(Actor &a) { a.Speak(a.GetDeathSFX(), 0); }

// Standing death animations (AR_HitLoseLimb, AR_HitRagdoll).
static int s_StandDeathAnims[4] = {0x9000058, 0x9000058, 0x90002b9, 0x90002ac};

static signed char AR_HitLoseLimb(StateMachine &sm, StructMsg_AttackedByPredator *pMsg)
{
    if (sm.GetActorOwner().CanLooseLimbs()) sm.GetActorOwner().ProcessHit(pMsg->m_Pos, pMsg->m_Dir, pMsg->m_Damage);
    sm.GetActorOwner().m_HitCountdown = 10.0f;
    AU_SpeakDeath(sm.GetActorOwner());
    if (sm.GetActorOwner().IsLyingOnGround()) {
        if (sm.GetActorOwner().IsLyingFaceDown())
            sm.GetActorOwner().SetAnimType(0x90002d0, 0, 1);
        else
            sm.GetActorOwner().SetAnimType(0x90002f2, 0, 1);
        sm.GetActorOwner().SetAnimFrame(0.3f);
        sm.GetActorOwner().SetIgnoreAnimChanges(1);
        SM_GOSUB(STATE_AnimDeath, 0, 0, 0, 0);
        return 1;
    }
    unsigned int r = g_EXRandClass.Rand32() & 3;
    sm.GetActorOwner().SetAnimType(s_StandDeathAnims[r], 0, 1);
    sm.GetActorOwner().SetIgnoreAnimChanges(1);
    SM_GOSUB(STATE_AnimDeath, 0, 0, 0, 0);
    return 1;
}

// NEAR: logic complete; retail keeps the two random rolls in one dead-stored stack slot and the
// feet-head subtracts schedule differently (size 1626 vs 1680).
static signed char AR_HitRagdoll(StateMachine &sm, StructMsg_AttackedByPredator *pMsg, signed char bForceRagdoll)
{
    sm.GetActorOwner().m_HitCountdown = 10.0f;
    AU_SpeakDeath(sm.GetActorOwner());
    sm.GetActorOwner().SetHealth(0.0f, 1);
    if (!bForceRagdoll) {
        int r = g_EXRandClass.Rand32() & 3;
        if (!r || !sm.GetActorOwner().IsRagDollAllowed()) {
            if (sm.GetActorOwner().IsLyingOnGround()) {
                if (sm.GetActorOwner().IsLyingFaceDown())
                    sm.GetActorOwner().SetAnimType(0x90002d0, 0, 1);
                else
                    sm.GetActorOwner().SetAnimType(0x90002f2, 0, 1);
                sm.GetActorOwner().SetAnimFrame(0.3f);
                sm.GetActorOwner().SetIgnoreAnimChanges(1);
                SM_GOSUB(STATE_AnimDeath, 0, 0, 0, 0);
                return 1;
            }
            int anim;
            if (g_EXRandClass.Rand32() & 1) {
                anim = s_StandDeathAnims[g_EXRandClass.Rand32() & 3];
            } else {
                float s = (float)sin(sm.GetActorOwner().GetRotY() + 0.785398163f);
                float c = (float)cos(sm.GetActorOwner().GetRotY() + 0.785398163f);
                float front = pMsg->m_Dir.z * c + pMsg->m_Dir.x * s + pMsg->m_Dir.y * 0.0f;
                float side = pMsg->m_Dir.z * s + pMsg->m_Dir.x * -c + pMsg->m_Dir.y * 0.0f;
                if (0.0f > front) {
                    if (side > 0.0f) {
                        sm.GetActorOwner().SetRotY((float)atan2(AU_SubV(Player_GetFeetPos(), sm.GetActorOwner().GetHeadPos()).x,
                                                                AU_SubV(Player_GetFeetPos(), sm.GetActorOwner().GetHeadPos()).z) - 1.57079633f);
                        anim = 0x90002d7;
                    } else {
                        sm.GetActorOwner().SetRotY((float)atan2(AU_SubV(Player_GetFeetPos(), sm.GetActorOwner().GetHeadPos()).x,
                                                                AU_SubV(Player_GetFeetPos(), sm.GetActorOwner().GetHeadPos()).z));
                        anim = 0x90002d8;
                    }
                } else if (0.0f > side) {
                    sm.GetActorOwner().SetRotY(-(float)atan2(AU_SubV(Player_GetFeetPos(), sm.GetActorOwner().GetHeadPos()).x,
                                                             AU_SubV(Player_GetFeetPos(), sm.GetActorOwner().GetHeadPos()).z));
                    anim = 0x90002d6;
                } else {
                    sm.GetActorOwner().SetRotY((float)atan2(AU_SubV(Player_GetFeetPos(), sm.GetActorOwner().GetHeadPos()).x,
                                                            AU_SubV(Player_GetFeetPos(), sm.GetActorOwner().GetHeadPos()).z) + 1.57079633f);
                    anim = 0x90002d5;
                }
            }
            sm.GetActorOwner().SetAnimType(anim, 0, 1);
            sm.GetActorOwner().SetIgnoreAnimChanges(1);
            SM_GOSUB(STATE_AnimDeath, 0, 0, 0, 0);
            return 1;
        }
    }
    sm.GetActorOwner().EnableRagDoll();
    float force = pMsg->m_Damage;
    if (force > 20.0f) force = 20.0f;
    sm.GetActorOwner().ApplyRagDollForce(pMsg->m_Pos, pMsg->m_Dir, (pMsg->m_Dir.y * 0.75f + 0.25f) * force, -1);
    SM_GOSUB(STATE_RagDollDeath, 0, 0, 0, 0);
    return 1;
}

static signed char AR_HitStomach(StateMachine &sm, StructMsg_AttackedByPredator *pMsg)
{
    if (sm.GetActorOwner().CanLooseLimbs() && sm.GetActorOwner().GetSkin()->m_CanLoseBelt) {
        sm.GetActorOwner().CreateLostLimb(ALL_BELT, pMsg->m_Dir, 0, 0);
        sm.GetActorOwner().m_HitCountdown = 10.0f;
        AU_SpeakDeath(sm.GetActorOwner());
        if (sm.GetActorOwner().IsLyingOnGround()) {
            if (sm.GetActorOwner().IsLyingFaceDown())
                sm.GetActorOwner().SetAnimType(0x90002d0, 0, 1);
            else
                sm.GetActorOwner().SetAnimType(0x90002f2, 0, 1);
            sm.GetActorOwner().SetAnimFrame(0.3f);
            sm.GetActorOwner().SetIgnoreAnimChanges(1);
            SM_GOSUB(STATE_AnimDeath, 0, 0, 0, 0);
        } else {
            int anims[2] = {0x900006d, 0x90002d9};
            unsigned int r = g_EXRandClass.Rand32() & 1;
            sm.GetActorOwner().SetAnimType(anims[r], 0, 1);
            sm.GetActorOwner().SetIgnoreAnimChanges(1);
            SM_GOSUB(STATE_AnimDeath, 0, 0, 0, 0);
        }
    }
    return 1;
}

static signed char AR_HitLoseHead(StateMachine &sm, StructMsg_AttackedByPredator *pMsg)
{
    if (sm.GetActorOwner().CanLooseLimbs()) {
        sm.GetActorOwner().CreateLostLimb(ALL_HEAD, pMsg->m_Dir, 0, 0);
        sm.GetActorOwner().m_HitCountdown = 10.0f;
        Actor *pA = &sm.GetActorOwner();
        if (pA->IsLyingOnGround()) {
            if (pA->IsLyingFaceDown())
                pA->SetAnimType(0x90002d0, 0, 1);
            else
                pA->SetAnimType(0x90002f2, 0, 1);
            sm.GetActorOwner().SetAnimFrame(0.3f);
            sm.GetActorOwner().SetIgnoreAnimChanges(1);
            SM_GOSUB(STATE_AnimDeath, 0, 0, 0, 0);
        } else {
            int anims[2] = {0x9000434, 0x9000433};
            unsigned int r = g_EXRandClass.Rand32() & 1;
            sm.GetActorOwner().SetAnimType(anims[r], 0, 1);
            sm.GetActorOwner().SetIgnoreAnimChanges(1);
            SM_GOSUB(STATE_AnimDeath, 0, 0, 0, 0);
        }
    }
    return 1;
}

// NEAR: 20 bytes; retail saves edi only inside the CanLooseLimbs block and picks different scratch registers.
static signed char AR_HitLoseHeadRagdoll(StateMachine &sm, StructMsg_AttackedByPredator *pMsg)
{
    if (sm.GetActorOwner().CanLooseLimbs()) {
        sm.GetActorOwner().CreateLostLimb(ALL_HEAD, pMsg->m_Dir, 0, 0);
        sm.GetActorOwner().m_HitCountdown = 10.0f;
        Actor *pA = &sm.GetActorOwner();
        if (pA->IsLyingOnGround()) {
            if (pA->IsLyingFaceDown())
                pA->SetAnimType(0x90002d0, 0, 1);
            else
                pA->SetAnimType(0x90002f2, 0, 1);
            sm.GetActorOwner().SetAnimFrame(0.3f);
        } else if (pA->IsRagDollAllowed()) {
            sm.GetActorOwner().EnableRagDoll();
            sm.GetActorOwner().ApplyRagDollForce(pMsg->m_Pos, pMsg->m_Dir, 0.0f, -1);
            sm.GetActorOwner().SetDontDragMeSpear(1);
            SM_GOSUB(STATE_RagDollDeath, 0, 0, 0, 0);
            return 1;
        } else {
            int anims[2] = {0x9000434, 0x9000433};
            unsigned int r = g_EXRandClass.Rand32() & 1;
            sm.GetActorOwner().SetAnimType(anims[r], 0, 1);
        }
        sm.GetActorOwner().SetIgnoreAnimChanges(1);
        SM_GOSUB(STATE_AnimDeath, 0, 0, 0, 0);
    }
    return 1;
}

float AngleDiff(float, float);  // TODO stand-in: EXMaths.obj

signed char STATE_HeavyFinish(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    float &height = sm.GetFloatRef(i);
    float &rotY = sm.GetFloatRef(i);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        switch (stage) {
        case 0:
            if (sm.GetActorOwner().HasAnimEnded()) {
                EXVector p;
                sm.GetActorOwner().SetHasBeenThrow(1);
                sm.GetActorOwner().GetBonePos(0xe000001, p);
                sm.GetActorOwner().SetPosition(p, -1);
                sm.GetActorOwner().JumpToPos((EXMaths::vec_xyzw)EXVector(p.x, p.y + 2.0f, p.z, p.w));
                sm.GetActorOwner().NoAnimBlend();
                sm.GetActorOwner().SetAnimType(0x9000299, 0, 1);
                height = p.y + 10.0f;
                stage++;
            }
            return 1;
        case 1:
            if (sm.GetActorOwner().GetFeetPos().y > height) sm.GetActorOwner().JumpHitCeiling();
            return 1;
        case 2:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().SetAnimType(0x90002bc, 0, 1);
                stage++;
            }
            return 1;
        }
        return 1;
    case MSG_EXIT:
        sm.GetActorOwner().SetCanBeJuggled(1);
        sm.GetActorOwner().SetBeingJuggled(0);
        return 1;
    case MSG_ENTER: {
        float r = (float &)sm.m_Msg.m_Data[0];
        rotY = r;
        sm.GetActorOwner().SetRotY(r);
        sm.GetActorOwner().SetAnimType(0x9000298, 0, 1);
        sm.GetActorOwner().SetCanBeJuggled(0);
        sm.GetActorOwner().SetBeingJuggled(1);
        XGetGameStats()->m_Unk10[12]++;  // TODO name: heavy finishes performed
        return 1;
    }
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case 0x31:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x31)) return AR_ReleaseFromPredator(sm);
        return 1;
    case 0x26:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x26)) {
            sm.GetActorOwner().SetIsLyingFaceDown(1);
            sm.GetActorOwner().SetIsLyingOnGround(1);
            sm.GetActorOwner().ChangeHealth(-5.0f, 1);
            float ry = sm.GetActorOwner().GetRotY();
            EXVector dir;
            dir.x = (float)sin(ry);
            dir.y = 0.0f;
            dir.z = (float)cos(ry);
            if (sm.GetActorOwner().m_AnimMode == 0x90004fb)
                SM_GOTO(STATE_DeathJuggleSlice, 0, 0, 0, 0);
            else
                SM_GOTO(STATE_HitReactKnockedOver, 0, (int &)dir.x, (int &)dir.y, (int &)dir.z);
        }
        return 1;
    }
    return 0;
}


// NEAR: logic complete; VC caches the actor/item pointers across the AngleDiff call (retail reloads) and keeps a zero register
signed char STATE_StealthDeathInit(StateMachine &sm)
{
    int i = 0;
    EXVector &pos = sm.GetVectorRef(i);
    float &rotY = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHealth(0.0f, 1);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXIT:
        sm.GetActorOwner().SetRotY(rotY);
        return 1;
    case MSG_ENTER:
        rotY = (float &)sm.m_Msg.m_Data[0];
        pos = sm.GetActorOwner().GetFeetPos();
        return 1;
    case MSG_EXT:
        sm.GetActorOwner().SetRotY(AngleDiff(sm.GetActorOwner().GetRotation().y, rotY) * 0.5f + sm.GetActorOwner().GetRotation().y);
        sm.GetActorOwner().SetPosition(pos, -1);
        return 1;
    case 0x31:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x31)) return AR_ReleaseFromPredator(sm);
        return 1;
    }
    return 0;
}

signed char STATE_StealthDeath(StateMachine &sm)
{
    int i = 0;
    float &rotY = sm.GetFloatRef(i);
    sm.GetActorOwner().SetStatus(0);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case 0x31:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x31)) return AR_ReleaseFromPredator(sm);
        return 1;
    case MSG_EXT:
        if (sm.GetActorOwner().GetAnimFrame() > 0.75f || sm.GetActorOwner().HasLostLimb())
            sm.GetActorOwner().CheckForSkinChange(10000.0f);
        if (sm.GetActorOwner().HasAnimEnded()) {
            sm.GetActorOwner().SetHealth(0.0f, 1);
            sm.GetActorOwner().m_AnimSpeedMult = 0.0f;
            SM_GOTO(STATE_FadeOutDead, 0, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_ENTER: {
        sm.GetActorOwner().InvalidateGotoPos();
        float r = (float &)sm.m_Msg.m_Data[0];
        rotY = r;
        sm.GetActorOwner().SetRotY(r);
        sm.GetActorOwner().SetCarry(0, 1);
        sm.GetActorOwner().SetCarry(0, 0);
        sm.GetActorOwner().SetIsBeingStealthKilled(1);
        sm.GetActorOwner().SetIsBlinded(1);
        sm.GetActorOwner().SetIsTargetable(0);
        sm.GetActorOwner().SetCanCollideWithPlayer(0);
        return 1;
    }
    }
    return 0;
}

// NEAR: logic complete; retail clears CanCollideWithPlayer through edi (load/and/store) and loads gCurrentMap_GeoHash after the Instance call.
signed char STATE_PlayerControlDeath(StateMachine &sm)
{
    int i = 0;
    float &timer = sm.GetFloatRef(i);
    int &bTimer = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(1);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
    case MSG_RETURN:
        if (sm.GetActorOwner().GetRefID() == XItemHandler_Player::Instance()->m_SpecialActorID) {
            sm.GetActorOwner().SetPlayerHolds(1);
            sm.GetActorOwner().InvalidateGotoPos();
            sm.GetActorOwner().SetCarry(0, 1);
            sm.GetActorOwner().SetCarry(0, 0);
            sm.GetActorOwner().EnableMovement(0);
            sm.GetActorOwner().SetCanCollideWithPlayer(0);
            if (!FELevelManager::Instance(1)->IsRitual(gCurrentMap_GeoHash)) {
                if (sm.GetActorOwner().OnFire() || sm.GetActorOwner().IsLeftArmMissing() || sm.GetActorOwner().IsRightArmMissing()) {
                    bTimer = 1;
                    timer = 480.0f;
                }
            }
        }
        return 1;
    case MSG_EXT:
        if (bTimer) {
            timer -= XApp::GameFrameRateMul();
            if (timer <= 0.0f) {
                XItemHandler_Player::Instance()->RagDollHeldDrone(1);
                return 1;
            }
        }
        if (sm.GetActorOwner().HasAnimEnded()) {
            switch (sm.GetActorOwner().GetAnimType()) {
            case 0x900024a:
            case 0x900024b:
                return 1;
            case 0x900024e:
                Gore_CreateOverkillBlood(&sm.GetActorOwner());
                break;
            }
            sm.GetActorOwner().SetHealth(0.0f, 1);
            sm.GetActorOwner().m_AnimSpeedMult = 0.0f;
            SM_GOTO(STATE_FadeOutDead, 0, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_SUSPEND:
        AR_ReleaseFromPredator(sm);
        return 1;
    case MSG_EXIT:
        sm.GetActorOwner().SetPlayerHolds(0);
        sm.GetActorOwner().SetIsTargetable(1);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR)) {}
        return 1;
    case 0x31:
        if (!sm.GetActorOwner().IsMsgOnStack((MsgType)0x31)) return AR_ReleaseFromPredator(sm);
        return 1;
    }
    return 0;
}

static signed char AR_HeavyFinish(StateMachine &sm, StructMsg_AttackedByPredator *pMsg)
{
    sm.GetActorOwner().m_HitCountdown = 10.0f;
    SM_GOSUB(STATE_HeavyFinish, (int &)pMsg->m_Dir.y, pMsg->m_Attack, 0, 0);
    return 1;
}

static signed char AR_StealthKillInit(StateMachine &sm, StructMsg_AttackedByPredator *pMsg)
{
    sm.GetActorOwner().m_HitCountdown = 10.0f;
    SM_GOSUB(STATE_StealthDeathInit, (int &)pMsg->m_Dir.y, 0, 0, 0);
    return 1;
}

static signed char AR_StealthKill(StateMachine &sm, StructMsg_AttackedByPredator *pMsg)
{
    int anim;
    switch (pMsg->m_Attack) {
    case 0x28: anim = 0x90001a8; goto set;
    case 0x29: anim = 0x90001a2; goto set;
    case 0x2a: anim = 0x90001ae; goto set;
    case 0x2b: anim = 0x9000276; goto set;
    case 0x2c: anim = 0x900044d; goto set;
    case 0x2d: anim = 0x9000474; goto set;
    case 0x2e: anim = 0x9000482; goto set;
    case 0x2f: anim = 0x9000486;
    set:
        sm.GetActorOwner().SetAnimType(anim, 0, 1);
        break;
    }
    SM_GOSUB(STATE_StealthDeath, (int &)pMsg->m_Dir.y, 0, 0, 0);
    return 1;
}

static signed char AR_PlayerControlDeath(StateMachine &sm, StructMsg_AttackedByPredator *pMsg)
{
    sm.GetActorOwner().SetIsTargetable(0);
    SM_GOSUB(STATE_PlayerControlDeath, (int &)pMsg->m_Dir.y, 0, 0, 0);
    return 1;
}

// NEAR: logic complete. Retail keeps the random roll in a 4-byte stack slot (dead store, then tests the and
// flags) and merges every Gosub into one call; `volatile` gets the frame and the merge but re-reads the slot.
static signed char AR_ReactFromHit(StateMachine &sm, const EXVector &dir, KnockOver ko)
{
    volatile int r;
    if (ko != KO_ZOMBIEDAZED) {
        if (sm.GetActorOwner().DontDoHitReaction()) return 1;
        if (sm.GetActorOwner().IsInvinsible()) return 1;
    }
    if (sm.GetActorOwner().IsUlysses() && XItemHandler_Player::Instance()->m_Mode == 0x14 &&
        XItemHandler_Player::Instance()->m_SpecialActorID == sm.GetActorOwner().GetRefID())
        return 1;
    sm.GetActorOwner().ResetLastHitTimer();
    while (sm.GetActorOwner().SearchBrainStateStack(STATE_Barge)) sm.PopState();
    while (sm.GetActorOwner().SearchBrainStateStack(STATE_NudgeCloaked)) sm.PopState();
    if (ko == KO_DAZED) {
        if (sm.GetActorOwner().m_SM.GetState() == STATE_Dazed) return 1;
    } else if (ko == KO_ZOMBIEDAZED) {
        if (sm.GetActorOwner().m_SM.GetState() == STATE_ZombieDazed) return 1;
    }
    if (sm.GetActorOwner().SearchBrainStateStack(STATE_HitReact)) {
        while (sm.GetActorOwner().SearchBrainStateStack(STATE_HitReact)) sm.PopState();
    } else if (sm.GetActorOwner().SearchBrainStateStack(STATE_HitReactKnockedOver)) {
        sm.GetActorOwner().ClearLastHitTimer();
        if (!sm.GetActorOwner().IsLyingOnGround()) return 1;
        while (sm.GetActorOwner().SearchBrainStateStack(STATE_HitReactLyingOnGround)) sm.PopState();
        SM_GOSUB(STATE_HitReactLyingOnGround, (int)&dir, 0, 0, 0);
        return 1;
    } else if (sm.GetActorOwner().IsLyingOnGround()) {
        while (sm.GetActorOwner().SearchBrainStateStack(STATE_HitReactLyingOnGround)) sm.PopState();
        SM_GOSUB(STATE_HitReactLyingOnGround, (int)&dir, 0, 0, 0);
        return 1;
    }
    int hash = sm.GetActorOwner().GetProfile()->m_Hash;
    if (hash != 0x47000138 && hash != 0x47000245) {
        // m_Anims[11]: hits in a row before the sub-group's knock-back (TODO name)
        if (SubGroupAnims::GetSubGroupAnims(sm.GetActorOwner().m_SubGroupAnimIndex).m_Anims[11] > 0) {
            int sub = sm.GetActorOwner().GetProfile()->m_SubGroup;
            if (sub == 0x4f000013 || sub == 0x4f000021 || sub == 0x4f00000e) {
                if (sm.GetActorOwner().GetConsecutiveHitCounter() >=
                    SubGroupAnims::GetSubGroupAnims(sm.GetActorOwner().m_SubGroupAnimIndex).m_Anims[11]) {
                    if (sub == 0x4f00000e)
                        SM_GOSUB(STATE_KnockBackServ, (int)&dir, 1, 0, 0);
                    else
                        SM_GOSUB(STATE_KnockBack, (int)&dir, 1, 0, 0);
                    return 1;
                }
            } else if (sm.GetActorOwner().GetConsecutiveHitCounter() >=
                       SubGroupAnims::GetSubGroupAnims(sm.GetActorOwner().m_SubGroupAnimIndex).m_Anims[11]) {
                return 1;
            }
        }
    }
    switch (ko) {
    case KO_KNOCKBACK_MONSTERSQUAD:
        SM_GOSUB(STATE_KnockBackMonsterSquad, (int)&dir, 1, 0, 0);
        break;
    case KO_KNOCKBACK_SERV:
        SM_GOSUB(STATE_KnockBackServ, (int)&dir, 1, 0, 0);
        break;
    case KO_KNOCKBACK_HUNTER1:
        SM_GOSUB(STATE_KnockBackHunter1, (int)&dir, 1, 0, 0);
        break;
    case KO_KNOCKBACK_RUSSIAN:
        SM_GOSUB(STATE_KnockBackRussian, (int)&dir, 1, 0, 0);
        break;
    case KO_KNOCKBACK_ZOMBIE:
        SM_GOSUB(STATE_KnockBackZombie, (int)&dir, 1, 0, 0);
        break;
    case KO_KNOCKBACKWARD:
        SM_GOSUB(STATE_KnockBackwardAndLive, (int)&dir, 0, 0, 0);
        break;
    case KO_ELECTRIC:
        SM_GOSUB(STATE_ElecticShock, (int)&dir, 0, 0, 0);
        break;
    case KO_BARGE:
        SM_GOSUB(STATE_Barge, (int)&dir, 0, 0, 0);
        break;
    case KO_NUDGE:
        SM_GOSUB(STATE_NudgeCloaked, (int)&dir, 0, 0, 0);
        break;
    case KO_MINE:
        SM_GOSUB(STATE_HitByUnknownMine, (int)&dir, 0, 0, 0);
        break;
    case KO_DAZED:
        SM_GOSUB(STATE_Dazed, (int)&dir, 0, 0, 0);
        break;
    case KO_ZOMBIEDAZED:
        SM_GOSUB(STATE_ZombieDazed, (int)&dir, 0, 0, 0);
        break;
    case KO_HITREACT_FLAG:
        SM_GOSUB(STATE_HitReact, (int)&dir, 1, 0, 0);
        break;
    case KO_JUGGLE_25: {
        r = g_EXRandClass.Rand32() & 3;
        if (!r && sm.GetActorOwner().CanBeJuggled())
            SM_GOSUB(STATE_HitReactKnockedOver, (int)&dir, 0, 0, 0);
        else
            SM_GOSUB(STATE_HitReact, (int)&dir, 0, 0, 0);
        break;
    }
    case KO_JUGGLE_50: {
        r = g_EXRandClass.Rand32() & 1;
        if (!r && sm.GetActorOwner().CanBeJuggled())
            SM_GOSUB(STATE_HitReactKnockedOver, (int)&dir, 0, 0, 0);
        else
            SM_GOSUB(STATE_HitReact, (int)&dir, 0, 0, 0);
        break;
    }
    case KO_JUGGLE_75: {
        r = g_EXRandClass.Rand32() & 3;
        if (r && sm.GetActorOwner().CanBeJuggled())
            SM_GOSUB(STATE_HitReactKnockedOver, (int)&dir, 0, 0, 0);
        else
            SM_GOSUB(STATE_HitReact, (int)&dir, 0, 0, 0);
        break;
    }
    case KO_JUGGLE:
        if (sm.GetActorOwner().CanBeJuggled())
            SM_GOSUB(STATE_HitReactKnockedOver, (int)&dir, 0, 0, 0);
        else
            SM_GOSUB(STATE_HitReact, (int)&dir, 0, 0, 0);
        break;
    case KO_NONE:
        break;
    default:
        SM_GOSUB(STATE_HitReact, (int)&dir, 0, 0, 0);
        break;
    }
    return 1;
}

static signed char AR_TakeHit(StateMachine &sm, StructMsg_AttackedByPredator *pMsg, KnockOver ko)
{
    sm.GetActorOwner().CheckBossDamageTriggers(pMsg->m_Damage);
    sm.GetActorOwner().CheckForSkinChange(pMsg->m_Damage);
    sm.GetActorOwner().ChangeHealth(-pMsg->m_Damage, 1);
    sm.GetActorOwner().m_HitCountdown = 10.0f;
    Actor *pA = &sm.GetActorOwner();
    if (pA->IsCloaked()) pA->SparkCloak(2);
    if (sm.GetActorOwner().GetHealth() > 0.0f) {
        AR_ReactFromHit(sm, pMsg->m_Dir, ko);
        return 1;
    }
    if (sm.GetActorOwner().CanRipBossHead()) {
        while (sm.GetActorOwner().SearchBrainStateStack(STATE_HitReact)) sm.PopState();
        SM_GOSUB(STATE_HitReact, (int)&pMsg->m_Dir, 0, 0, 0);
        return 1;
    }
    return 0;
}

// Applies an actor's hit; returns 1 (after the hit reaction) when the victim survives.
static signed char AR_TakeHitFromActor(StateMachine &sm, StructMsg_AttackedByActor *pMsg, KnockOver ko)
{
    sm.GetActorOwner().CheckBossDamageTriggers(pMsg->m_Damage);
    sm.GetActorOwner().CheckForSkinChange(pMsg->m_Damage);
    sm.GetActorOwner().ChangeHealth(0.0f - pMsg->m_Damage, 0);
    sm.GetActorOwner().m_HitCountdown = 10.0f;
    if (sm.GetActorOwner().GetHealth() > 0.0f) {
        AR_ReactFromHit(sm, pMsg->m_Dir, ko);
        return 1;
    }
    return 0;
}

signed char STATE_FS_LyingDownInstant(StateMachine &sm)
{
    int i = 0;
    float &standTimer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXIT:
        sm.GetActorOwner().SetIsLyingOnGround(0);
        sm.GetActorOwner().SetIsLyingFaceDown(0);
        sm.GetActorOwner().SetCanBeJuggled(1);
        return 1;
    case MSG_ENTER:
        standTimer = 0.0f;
        sm.GetActorOwner().SetIsLyingFaceDown(1);
        sm.GetActorOwner().SetCanBeJuggled(0);
        sm.GetActorOwner().SetIsLyingOnGround(1);
        sm.GetActorOwner().SetAnimType(0x90002bc, 0, 1);
        return 1;
    case MSG_EXT:
        if (sm.GetActorOwner().IsPlayerStoodOnMe()) {
            if (standTimer <= 0.0f) {
                standTimer = (g_EXRandClass.Randf() + 1.0f) * EXBaseApp::NominalGameFrameRate() * 0.1f;
                sm.GetActorOwner().ChangeHealth(-5.0f, 1);
                SM_GOSUB(STATE_HitReactLyingOnGround, 0, 0, 0, 0);
                return 1;
            }
        } else if (standTimer > 0.0f) {
            standTimer -= XGetFrameRateMul();
            return 1;
        }
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

static signed char AR_HitExplode(StateMachine &sm, StructMsg_AttackedByPredator *pMsg)
{
    static int s_Anims[3] = {0x900004c, 0x90001c5, 0x90001c4};
    if (sm.GetActorOwner().m_Health - pMsg->m_Damage > 0.0f) return AR_TakeHit(sm, pMsg, KO_JUGGLE_25);
    sm.GetActorOwner().ChangeHealth(-pMsg->m_Damage, 1);
    sm.GetActorOwner().m_HitCountdown = 10.0f;
    AU_SpeakDeath(sm.GetActorOwner());
    if (sm.GetActorOwner().IsRagDollAllowed()) {
        sm.GetActorOwner().EnableRagDoll();
        sm.GetActorOwner().ApplyRagDollForce(pMsg->m_Pos, pMsg->m_Dir, pMsg->m_Damage, -1);
        SM_GOSUB(STATE_RagDollDeath, 0, 0, 0, 0);
        return 1;
    }
    sm.GetActorOwner().SetRotY((float)atan2(AU_SubV(pMsg->m_Pos, sm.GetActorOwner().GetHeadPos()).x,
                                            AU_SubV(pMsg->m_Pos, sm.GetActorOwner().GetHeadPos()).z));
    unsigned int r = g_EXRandClass.Rand32() % 3;
    sm.GetActorOwner().SetAnimType(s_Anims[r], 0, 1);
    sm.GetActorOwner().SetIgnoreAnimChanges(1);
    SM_GOSUB(STATE_AnimDeath, 0, 0, 0, 0);
    return 1;
}

static signed char AR_TakeHitPlayerMine(StateMachine &sm, StructMsg_AttackedByPredator *pMsg, KnockOver ko)
{
    if (ko == KO_DAZED) return AR_TakeHit(sm, pMsg, ko);
    if (sm.GetActorOwner().IsLyingOnGround()) return AR_TakeHit(sm, pMsg, ko);
    if (sm.GetActorOwner().IsLyingBum()) return AR_TakeHit(sm, pMsg, ko);
    if (sm.GetActorOwner().IsBoss() || sm.GetActorOwner().IsUlysses() || sm.GetActorOwner().IsAlien())
        return AR_TakeHit(sm, pMsg, ko);
    if (sm.GetActorOwner().m_StateFlags.m_bPlayerSeen) return AR_TakeHit(sm, pMsg, ko);
    if (sm.GetActorOwner().IsBitMsgOnStack(AW_BIT_HAVE_SEEN_PLAYER)) return AR_TakeHit(sm, pMsg, ko);
    if (sm.GetActorOwner().SearchBrainStateStack(STATE_HitByUnknownMine)) return AR_TakeHit(sm, pMsg, ko);
    if (!(sm.GetActorOwner().GetWatchFlags() | 1)) return AR_TakeHit(sm, pMsg, ko);
    switch (sm.GetActorOwner().GetProfile()->m_UnderAttackState) {
    case 0x4d000011:
    case 0x4d000022:
    case 0x4d000023:
    case 0x4d000027:
    case 0x4d000028:
    case 0x4d000038:
    case 0x4d000182:
        break;
    default:
        return AR_TakeHit(sm, pMsg, ko);
    }
    sm.GetActorOwner().CheckBossDamageTriggers(pMsg->m_Damage);
    sm.GetActorOwner().CheckForSkinChange(pMsg->m_Damage);
    sm.GetActorOwner().ChangeHealth(-pMsg->m_Damage, 1);
    sm.GetActorOwner().m_HitCountdown = 10.0f;
    Actor *pA = &sm.GetActorOwner();
    if (pA->IsCloaked()) pA->SparkCloak(2);
    if (sm.GetActorOwner().GetHealth() > 0.0f) {
        AR_ReactFromHit(sm, pMsg->m_Dir, KO_MINE);
        return 1;
    }
    return 0;
}

// NEAR: retail keeps the dead ArmourProfile store (attack 9) and reloads the actor in the death block.
// GUESS: retail passes whatever is in eax as Gore_CreateActorBlood's type (the attack type on the direct
// path); written as pMsg->m_Attack. The ArmourProfile lookup for attack 9 is unused in retail too.
signed char AU_HandlePlayerAttacks_Ulysses(StateMachine &sm, StructMsg_AttackedByPredator *pMsg)
{
    switch (pMsg->m_Attack) {
    case 8:
        SM_GOSUB(STATE_UlyssesHitbyEMP, 0, 0, 0, 0);
        return 1;
    case 0:
        if (XItemHandler_Player::Instance()->m_Mode != 0x14 &&
            XItemHandler_Player::Instance()->m_SpecialActorID == sm.GetActorOwner().m_RefID)
            pMsg->m_Damage *= 0.25f;
    case 1:
    case 2:
    case 3:
    case 4:
        AR_TakeHit(sm, pMsg, KO_HITREACT);
        return 1;
    case 5:
    case 6:
    case 7:
        Gore_CreateActorBlood(&sm.GetActorOwner(), 0, 0, pMsg->m_Attack, pMsg->m_Pos, pMsg->m_Dir, 0);
        AR_TakeHit(sm, pMsg, KO_HITREACT);
        return 1;
    case 9: {
        ArmourProfile *pArmour = ArmourProfile::GetArmourData(sm.GetActorOwner().GetProfile()->m_Armour);
        (void)pArmour;
        return 1;
    }
    case 51: case 52: case 53: case 54: case 55: case 56: case 57: case 58: case 59: case 60: case 61: case 62:
        pMsg->m_Damage *= 3.0f;
    case 10:
    case 11:
        AR_TakeHit(sm, pMsg, KO_HITREACT);
        return 1;
    case 49:
    case 50:
        pMsg->m_Damage *= 1.5f;
        AR_TakeHit(sm, pMsg, KO_HITREACT);
        return 1;
    case 48: {
        int anims[2];
        anims[0] = 0x900042c;
        anims[1] = 0x9000430;
        sm.GetActorOwner().SetAnimType(anims[g_EXRandClass.Rand32() & 1], 0, 1);
        Actor::OverridePlayerCloak();
        sm.GetActorOwner().SetHealth(0.0f, 1);
        sm.GetActorOwner().SetIgnoreAnimChanges(1);
        SM_GOTO(STATE_AnimDeath, 0, 0, 0, 0);
        return 1;
    }
    case 30: case 31: case 32: case 33: case 34: case 35: case 36: case 37: case 38: case 39: case 40: case 41:
    case 42: case 43: case 44: case 45: case 46: case 47: case 71: case 72: case 73: case 74: case 75:
        return 1;
    }
    return 1;
}

// NEAR: retail loads pMsg after the SubGroupAnims call (one scheduling difference); the rest is as retail.
// GUESS: SubGroupAnims +0x20 is a float damage scale (header lists it as int m_Anims[7]).
signed char AU_HandlePlayerAttacks_Boss(StateMachine &sm, StructMsg_AttackedByPredator *pMsg)
{
    pMsg->m_Damage = *(float *)&SubGroupAnims::GetSubGroupAnims(sm.GetActorOwner()).m_Anims[7] * pMsg->m_Damage;
    int bossID = sm.GetActorOwner().m_RefID;
    FEHud *pHud = FEHud::Instance(1);
    pHud->m_BossHitID = bossID;
    pHud->m_BossHitTimer = 5.0f;
    signed char bMafia = 0;
    if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f00000d) {
        bMafia = 1;
        if (sm.GetActorOwner().IsMafiaBossVulnerable(pMsg->m_Attack)) {
            sm.GetActorOwner().m_StateFlags.m_bDontDoHitReaction = 0;
        } else {
            sm.GetActorOwner().MafiaBossSheild(1);
            sm.GetActorOwner().m_StateFlags.m_bDontDoHitReaction = 1;
        }
    }
    if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f00000f) {
        switch (pMsg->m_Attack) {
        case 3: case 4: case 5: case 6: case 7: case 8: case 9: case 48:
        case 55: case 56: case 57: case 59: case 60: case 61:
            sm.GetActorOwner().m_StateFlags.m_bDontDoHitReaction = 0;
            break;
        default:
            sm.GetActorOwner().m_StateFlags.m_bDontDoHitReaction = 1;
            break;
        }
    }
    switch (pMsg->m_Attack) {
    case 0: case 1: case 2: case 3: case 4: case 10: case 11:
        AR_TakeHit(sm, pMsg, KO_HITREACT);
        return 1;
    case 8:
        if (sm.GetActorOwner().SearchBrainStateStack(STATE_DeathBoss)) return 1;
        sm.GetActorOwner().BreakCloak(0);
        if (sm.GetActorOwner().m_Health > 0.0f &&
            sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f00000f) {
            SM_GOSUB(STATE_MonsterSquadHitbyEMP, 0, 0, 0, 0);
            return 1;
        }
        if (bMafia) {
            sm.GetActorOwner().m_EMPTimer = (EXRandf() + 1.0f) * (float)EXBaseApp::NominalGameFrameRate() * 6.0f;
            sm.GetActorOwner().m_StateFlags.m_bDontDoHitReaction = 0;
            AR_TakeHit(sm, pMsg, KO_HITREACT);
        }
        return 1;
    case 9: {
        if (sm.GetActorOwner().SearchBrainStateStack(STATE_DeathBoss)) return 1;
        ArmourProfile *pArmour = ArmourProfile::GetArmourData(sm.GetActorOwner().GetProfile()->m_Armour);
        if (pArmour && pArmour->m_Damage[9] == 0.0f) return 1;
        if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f00000c) {
            AR_ReactFromHit(sm, pMsg->m_Dir, KO_ZOMBIEDAZED);
            return 1;
        }
        AR_ReactFromHit(sm, pMsg->m_Dir, KO_BARGE);
        return 1;
    }
    case 5: case 6: case 7:
        Gore_CreateActorBlood(&sm.GetActorOwner(), 0, 0, pMsg->m_Attack, pMsg->m_Pos, pMsg->m_Dir, 0);
        AR_TakeHit(sm, pMsg, KO_HITREACT);
        return 1;
    case 48:
        sm.GetActorOwner().m_StateFlags.m_bStartBossHeadRipped = 1;
        return 1;
    case 49: case 50: case 51: case 52: case 53: case 54: case 55: case 56: case 57: case 58: case 59: case 60:
    case 61: case 62:
        if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f00000e) {
            AR_TakeHit(sm, pMsg, KO_KNOCKBACK_SERV);
            return 1;
        }
        if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f000010) {
            AR_TakeHit(sm, pMsg, KO_KNOCKBACK_RUSSIAN);
            return 1;
        }
        if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f00000c) {
            AR_TakeHit(sm, pMsg, KO_KNOCKBACK_ZOMBIE);
            return 1;
        }
        if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f000021) {
            AR_TakeHit(sm, pMsg, KO_KNOCKBACK_HUNTER1);
            return 1;
        }
        if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f00000f) {
            if (pMsg->m_Attack < 0x33 || pMsg->m_Attack > 0x36) {
                AR_TakeHit(sm, pMsg, KO_KNOCKBACK_MONSTERSQUAD);
                return 1;
            }
        }
        AR_TakeHit(sm, pMsg, KO_HITREACT);
        return 1;
    case 12: case 13: case 14: case 15: case 16: case 17: case 18: case 19: case 20: case 21: case 22: case 23:
    case 24: case 25: case 26: case 27: case 28: case 29: case 63: case 64: case 65: case 66: case 67: case 68:
    case 69: case 70:
        return 1;
    case 30: case 31: case 32: case 33: case 34: case 35: case 36: case 37: case 38: case 39: case 40: case 41:
    case 42: case 43: case 44: case 45: case 46: case 47: case 71: case 72: case 73: case 74: case 75:
        return 1;
    }
    return 1;
}

// A killing punch from the POW fist: death cry and, where allowed, a rag doll thrown along the blow.
static signed char AR_HitRagdollFromActor(StateMachine &sm, StructMsg_AttackedByActor *pMsg)
{
    sm.GetActorOwner().m_HitCountdown = 10.0f;
    AU_SpeakDeath(sm.GetActorOwner());
    if (sm.GetActorOwner().IsRagDollAllowed()) {
        sm.GetActorOwner().EnableRagDoll();
        sm.GetActorOwner().ApplyRagDollForce(pMsg->m_Pos, pMsg->m_Dir, pMsg->m_Damage, -1);
        SM_GOSUB(STATE_RagDollDeath, 0, 0, 0, 0);
    }
    return 1;
}

// An actor (or an actor's weapon) hit this one. Damage is scaled by 5. Bosses and the Russian mercs,
// Bruno, savage predators and one skin take no actor damage (the bosses listed only from the dead
// bloke); a Ronin ignores other Ronin and an enforcer with a negative EMP timer is unaffected. Hits
// from allies send the attacker message 0x2a (and do nothing to subgroups with that animation set).
// EMP breaks cloaks (Ulysses has his own reaction). Armour scales the damage unless the weapon
// ignores it. Drones take limb explosions from heavy explosives and are set alight by petrol bombs
// and grenades.
signed char AU_HandleActorAttacks(StateMachine &sm, StructMsg_AttackedByActor *pMsg)
{
    if (!pMsg) return 1;
    pMsg->m_Damage *= 5.0f;
    int profile = sm.GetActorOwner().GetProfile()->m_Hash;
    int subGroup = sm.GetActorOwner().GetProfile()->m_SubGroup;
    switch (profile) {
    case 0x4700019b:  // HT_ActorProfile_M1A_IrishBoss
    case 0x470001bf:  // HT_ActorProfile_CC_TrampBasher
    case 0x470001c3:  // HT_ActorProfile_M2H_PoliceChief
    case 0x470002dc:
    case 0x470002de:
    case 0x47000386:
        if (pMsg->m_pWeapon->m_Id != 0x49000039) return 1;  // HT_Weaponry_Dead_Bloke
        break;
    }
    switch (subGroup) {
    case 0x4f000010:  // HT_ActorSubGroup_RussianMercs
    case 0x4f000013:  // HT_ActorSubGroup_Bruno
    case 0x4f000016:  // HT_ActorSubGroup_SavagePred
        return 1;
    }
    if (sm.GetActorOwner().GetSkin()->m_Hash == 0xd000138) return 1;
    if (subGroup == 0x4f00000f) {  // HT_ActorSubGroup_MonsterSquad
        sm.GetActorOwner().m_StateFlags.m_bDontDoHitReaction = 0;
    } else if (subGroup == 0x4f00000d) {  // HT_ActorSubGroup_MafiaEnforcer
        if (0.0f > sm.GetActorOwner().GetEMPTimer()) return 1;
    } else if (subGroup == 0x4f000011) {  // HT_ActorSubGroup_Ronin
        if (pMsg->m_pAttacker && pMsg->m_pAttacker->GetProfile()->m_SubGroup == subGroup) return 1;
    }
    if (pMsg->m_pAttacker) {
        if (!AP_DoWeFight(sm.GetActorOwner().GetProfile(), pMsg->m_pAttacker->GetProfile())) {
            Actor::SendMsg(pMsg->m_pAttacker->GetRefID(), MsgItem((MsgType)0x2a, sm.GetActorOwner().GetRefID()), 0.0f);  // TODO name
            if (SubGroupAnims::GetSubGroupAnims(sm.GetActorOwner()).m_Anims[12]) pMsg->m_Damage = 0.0f;
        }
    }
    if (pMsg->m_pWeapon->m_DamageClass == 0x53000006) {  // HT_DamageClass_EMP
        if (sm.GetActorOwner().IsUlysses()) {
            SM_GOSUB(STATE_UlyssesHitbyEMP, 0, 0, 0, 0);
            return 1;
        }
        sm.GetActorOwner().CheckForSkinChange(10000.0f);
        sm.GetActorOwner().BreakCloak(0);
    } else if (0.0f >= pMsg->m_Damage && pMsg->m_pWeapon->m_Id != 0x49000039) {
        return 1;
    }
    if (!pMsg->m_pWeapon->m_IgnoreArmour) {
        ArmourProfile *pArmour = ArmourProfile::GetArmourData(sm.GetActorOwner().GetProfile()->m_Armour);
        if (pArmour) pMsg->m_Damage = pArmour->m_Damage[11] * pMsg->m_Damage;  // index 11: actor attacks
    }
    KnockOver ko = KO_HITREACT;
    if (pMsg->m_pWeapon) {
        if (!sm.GetActorOwner().IsBoss() && !sm.GetActorOwner().IsUlysses() && !sm.GetActorOwner().IsAlien() &&
            pMsg->m_pWeapon->m_Id == 0x49000039)
            ko = KO_JUGGLE_75;
        if (pMsg->m_pWeapon->m_ClipSize) ko = KO_HITREACT_FLAG;
        if (pMsg->m_pWeapon->m_Id == 0x4900002a) ko = KO_ELECTRIC;  // HT_Weaponry_Drone_ShockStick
        Gore_CreateActorBlood(&sm.GetActorOwner(), 1, 0, pMsg->m_pWeapon->m_Id, pMsg->m_Pos, pMsg->m_Dir, 0);
        sm.GetActorOwner().PlayActorWeaponImpactSFX(sm.GetActorOwner().GetRefID(), pMsg->m_Pos, pMsg->m_pWeapon, 0);
    }
    if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f00000f) ko = KO_NONE;  // HT_ActorSubGroup_MonsterSquad
    if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f000021) ko = KO_NONE;  // HT_ActorSubGroup_Hunter
    if (sm.GetActorOwner().IsUlysses()) ko = KO_NONE;
    if (AR_TakeHitFromActor(sm, pMsg, ko)) return 1;
    Actor *pA = &sm.GetActorOwner();
    if (!pA->IsBoss() && !pA->IsUlysses()) {
        if (!pMsg->m_pWeapon) return 1;
        if (pMsg->m_pWeapon->m_Id == 0x4900004b) {  // HT_Weaponry_POW_Fist
            AR_HitRagdollFromActor(sm, pMsg);
            return 1;
        }
    }
    if (!pMsg->m_pWeapon || pA->IsLyingOnGround()) return 1;
    if (pA->IsDrone() && pMsg->m_Damage >= 50.0f) {
        switch (pMsg->m_pWeapon->m_Id) {
        case 0x49000024:  // HT_Weaponry_Drone_GrenadeLauncher
        case 0x49000027:  // HT_Weaponry_Drone_MissileLauncher
        case 0x4900002e:  // HT_Weaponry_Drone_MS_MultiLauncher
        case 0x4900003b:  // HT_Weaponry_Ulysses_Rocket
        case 0x49000046:  // HT_Weaponry_Ulysses_Rocket_Homing
        case 0x49000052:  // HT_Weaponry_Gunship_Rocket
            pA->ExplodeLimbs(pMsg->m_Dir * pMsg->m_Damage, 1);
            break;
        }
    }
    if (sm.GetActorOwner().IsDrone() &&
        (pMsg->m_pWeapon->m_Id == 0x49000025 || pMsg->m_pWeapon->m_Id == 0x49000024)) {  // PetrolBomb, GrenadeLauncher
        sm.GetActorOwner().SetHealth(50.0f, 2);
        SM_GOTO(STATE_BlazinSquad, (int)&pMsg->m_Pos, 0, 0, 0);
    }
    return 1;
}

// The drone branch is unreachable (n > 1 then n < 2); retail compiles it anyway.
static void AR_HitBarge(StateMachine &sm, StructMsg_AttackedByPredator *pMsg)
{
    if (sm.GetActorOwner().IsBoss() || sm.GetActorOwner().IsUlysses()) return;
    if (sm.GetActorOwner().IsLyingOnGround()) return;
    if (sm.GetActorOwner().IsLyingBum()) return;
    if (sm.GetActorOwner().BeingJuggled()) return;
    if (sm.GetActorOwner().SearchBrainStateStack(STATE_Barge)) return;
    if (sm.GetActorOwner().SearchBrainStateStack(STATE_NudgeCloaked)) return;
    if (sm.GetActorOwner().SearchBrainStateStack(STATE_FS_LyingDownInstant)) return;
    if (sm.GetActorOwner().SearchBrainStateStack(STATE_KnockBackwardAndLive)) return;
    if (sm.GetActorOwner().SearchBrainStateStack(STATE_HitReactKnockedOver)) return;
    sm.GetActorOwner().m_HitCountdown = 10.0f;
    int n = (int)(pMsg->m_Damage + 0.5f);
    if (n > 1) {
        if (n >= 2) {
            AR_ReactFromHit(sm, pMsg->m_Dir, KO_BARGE);
            return;
        }
        if (!sm.GetActorOwner().IsDrone()) return;
        if (sm.GetActorOwner().m_StateFlags.m_bPlayerSeen) return;
        if (sm.GetActorOwner().m_Attrib & 0x200) return;
        if (sm.GetActorOwner().GetWatchFlags() & 1) return;
        if (sm.GetActorOwner().GetNudgeCount() <= 3) {
            switch (sm.GetActorOwner().GetProfileHash()) {
            case 0x4700001d:
            case 0x4700001e:
            case 0x47000020:
            case 0x47000024:
            case 0x4700025e:
                return;
            }
            AR_ReactFromHit(sm, pMsg->m_Dir, KO_NUDGE);
            return;
        }
        sm.GetActorOwner().PlayerForceSighting();
        return;
    }
}

signed char AU_HandleSomeoneUnderAttack(StateMachine &sm)
{
    if (!(sm.GetActorOwner().GetWatchFlags() & 1)) return 1;
    int hash = sm.GetActorOwner().GetProfile()->m_UnderAttackState;
    if (hash) {
        while (sm.GetActorOwner().SearchBrainStateStack(STATE_SearchForPredator)) sm.PopState();
        while (sm.GetActorOwner().SearchBrainStateStack(STATE_NudgeCloaked)) sm.PopState();
        sm.Gosub(GetMsgBitsForMsg(MSG_SOMEONE_SEEN_PREDATOR) | GetMsgBitsForMsg(MSG_SOMEONE_UNDER_ATTACK) | AW_BIT_HAVE_SEEN_PLAYER,
                 AP_GetStateFromHash(hash), 0, 0, 0, 0);
    }
    return 1;
}

// NEAR: written for behaviour from the disassembly (not byte-matched); retail tail-merges the shared
// dodge block (sub-groups 0x10 and 0x0c) and the m_BlockCounter reset (0x13 and 0x0e).
signed char AU_HandleIncomingAttack(StateMachine &sm, StructMsg_IncomingAttack *pMsg)
{
    if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f000013) {
        if (pMsg->m_Unk4 != -1) {
            sm.GetActorOwner().m_BlockCounter = 0.0f;
            return 1;
        }
        EXVector d = AU_Sub(sm.GetActorOwner().GetItem()->m_position, pMsg->m_Pos);
        if (0.0f > (float)sin(sm.GetActorOwner().GetRotY()) * d.x + (float)cos(sm.GetActorOwner().GetRotY()) * d.z) {
            SM_GOSUB(STATE_BlockServ, (int)pMsg, 0, 0, 0);
            return 1;
        }
    } else if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f00000d) {
    } else if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f000010) {
        switch (pMsg->m_Weapon) {
        case 0x49000000:
        case 0x49000001:
        case 0x49000002:
        case 0x49000003:
        case 0x49000004:
        case 0x49000005:
        case 0x49000006:
            if (g_EXRandClass.Rand32() % 3 == 0 && sm.GetActorOwner().CanDodge()) {
                sm.GetActorOwner().PerformDodge();
                if (Actor_CheckForAttack(sm.GetActorOwner(), pMsg)) {
                    SM_GOSUB(STATE_Dodge, (int)pMsg, 0, 0, 0);
                    return 1;
                }
            }
            break;
        case 0x49000007:
        case 0x49000008:
            if (Actor_CheckForAttack(sm.GetActorOwner(), pMsg)) {
                SM_GOSUB(STATE_RussianBlock, (int)pMsg, 0, 0, 0);
                return 1;
            }
            break;
        }
    } else if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f00000c) {
        if (pMsg->m_Weapon >= 0x49000002 && pMsg->m_Weapon <= 0x49000009) {
            if (g_EXRandClass.Rand32() % 3 == 0 && sm.GetActorOwner().CanDodge()) {
                sm.GetActorOwner().PerformDodge();
                if (Actor_CheckForAttack(sm.GetActorOwner(), pMsg)) {
                    SM_GOSUB(STATE_Dodge, (int)pMsg, 0, 0, 0);
                    return 1;
                }
            }
        }
    } else if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f00000e) {
        if (pMsg->m_Unk4 != -1) {
            sm.GetActorOwner().m_BlockCounter = 0.0f;
            return 1;
        }
        if (pMsg->m_Weapon == 0x49000009) {
            sm.GetActorOwner().m_BlockCounter = 0.0f;
            return 1;
        }
        if (!Player_IsCloaked()) {
            EXVector d = AU_Sub(sm.GetActorOwner().GetItem()->m_position, pMsg->m_Pos);
            if (0.0f > (float)sin(sm.GetActorOwner().GetRotY()) * d.x + (float)cos(sm.GetActorOwner().GetRotY()) * d.z) {
                SM_GOSUB(STATE_BlockServ, (int)pMsg, 0, 0, 0);
                return 1;
            }
        }
    } else {
        if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f000021 &&
            (sm.GetActorOwner().GetProfile()->m_Hash == 0x47000245 || sm.GetActorOwner().GetProfile()->m_Hash == 0x47000138) &&
            !sm.GetActorOwner().m_ActorFlags.m_bDiscNotInHand)
            return 0;
        if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f000021 && !sm.GetActorOwner().m_ActorFlags.m_bDiscNotInHand) {
            if (pMsg->m_Weapon >= 0x49000000 && pMsg->m_Weapon <= 0x49000006) {
                int r = g_EXRandClass.Rand32() & 1;
                if (r && sm.GetActorOwner().GetProfile()->m_Hash != 0x47000246) {
                    SM_GOSUB(STATE_HunterBlock, (int)pMsg, 0, 0, 0);
                    return 1;
                }
            }
        } else if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f000011) {
            if (pMsg->m_Unk4 != -1 && 9.0f > sm.GetActorOwner().m_SqDistFromPlayer) {
                if (sm.GetActorOwner().m_SM.m_State != STATE_HitReact &&
                    sm.GetActorOwner().m_SM.m_State != STATE_RoninPredAttackBlock) {
                    SM_GOSUB(STATE_RoninPredAttackLeapBack, 0, 0, 0, 0);
                    return 1;
                }
                SM_GOTO(STATE_RoninPredAttackLeapBack, 0, 0, 0, 0);
                return 1;
            }
            if (pMsg->m_Weapon == 0x49000007 || pMsg->m_Weapon == 0x49000008) {
                if (!sm.GetActorOwner().SearchBrainStateStack(STATE_GotoLocation)) {
                    SM_GOSUB(STATE_RoninPredAttackBlock, (int)pMsg, 0, 0, 0);
                    return 1;
                }
            }
        } else if (sm.GetActorOwner().GetProfile()->m_SubGroup != 0x4f000016 && !sm.GetActorOwner().m_ActorFlags.m_bBoss) {
            if (pMsg->m_Weapon == 0x49000007 || pMsg->m_Weapon == 0x49000009) {
                if (12.0f > pMsg->m_Unk28) return 0;
            }
            int chance = sm.GetActorOwner().GetProfile()->m_DodgeChance;
            int r = g_EXRandClass.Rand32() % 101;
            if (r <= chance && sm.GetActorOwner().CanDodge() && Actor_CheckForAttack(sm.GetActorOwner(), pMsg)) {
                SM_GOSUB(STATE_Dodge, (int)pMsg, 0, 0, 0);
                return 1;
            }
        }
    }
    return 0;
}

// NEAR: written for behaviour from the disassembly (not byte-matched). Two passes relax first the
// away-from-player angle test, then the 10 m player distance test, before giving up.
int AU_FindRunAwayPoint(StateMachine &sm, int flags, int except, int zone, signed char bNotFull)
{
    float bestDist = 0.0f;
    EXVector myPos = sm.GetActorOwner().GetItem()->m_position;
    EXVector playerPos = gAIControl.m_LastSeenPlayerPos;
    int best = 0;
    signed char bAngleCheck = 1;
    signed char bDistCheck = 1;
    for (;;) {
        EXArray<int, int> zones;
        if (sm.GetActorOwner().m_ActorFlags.m_bBoss && !sm.GetActorOwner().m_NavRoute.JumpDisabled()) {
            XNavMesh *pMesh = gNavMeshHandler.Single();
            if (pMesh && pMesh->NavNetwork()) pMesh->NavNetwork()->GetJumpToZoneArray(zone, zones);
        } else {
            zones.SetSize(1);
            zones[0] = zone;
        }
        for (int i = 0; i < zones.Size(); i++) {
            int group = zones[i];
            for (int poi = POI_GetFirst(group); poi; poi = POI_GetNext(group, poi)) {
                if (poi == except) continue;
                POI_STRUCT *p = POI_Get(poi);
                if (!(flags & POI_GetLocFlags(poi))) continue;
                if (bDistCheck) {
                    EXVector d = AU_SubV(playerPos, p->m_Pos);
                    if (100.0f > d.LengthXYZSquared()) continue;
                }
                if (bAngleCheck) {
                    if (AU_VecAngleInside(playerPos, myPos, p->m_Pos, 40.0f)) continue;
                }
                EXVector d = AU_SubV(myPos, p->m_Pos);
                float dist = d.LengthXYZSquared();
                if (best && !(bestDist > dist)) continue;
                if (bNotFull && POI_IsFull(poi, (LocCount::Flags)0xf)) continue;
                bestDist = dist;
                best = poi;
                p->m_UseCount++;
            }
        }
        if (best) return best;
        if (bAngleCheck) bAngleCheck = 0;
        else if (bDistCheck) bDistCheck = 0;
        else return 0;
    }
}

// NEAR: written for behaviour from the disassembly (not byte-matched). Heal points are POIs flagged 0x10000,
// or flagged 0x80000..0x400000 while the matching flag channel (0xc9..0xcc) is clear. When the player is
// within 20 m the player-distance and angle tests apply, relaxed one per pass as in AU_FindRunAwayPoint.
int AU_FindHealPoint(StateMachine &sm, int except, int zone, signed char bNotFull)
{
    float bestDist = 0.0f;
    EXVector myPos = sm.GetActorOwner().GetItem()->m_position;
    EXVector playerPos = gAIControl.m_LastSeenPlayerPos;
    EXVector toPlayer = AU_SubV(myPos, playerPos);
    int best = 0;
    signed char bPlayerNear = 1;
    if (!(400.0f > toPlayer.LengthXYZSquared())) bPlayerNear = 0;
    signed char bAngleCheck = 1;
    signed char bDistCheck = 1;
    for (;;) {
        EXArray<int, int> zones;
        if (sm.GetActorOwner().m_ActorFlags.m_bBoss && !sm.GetActorOwner().m_NavRoute.JumpDisabled()) {
            XNavMesh *pMesh = gNavMeshHandler.Single();
            if (pMesh && pMesh->NavNetwork()) pMesh->NavNetwork()->GetJumpToZoneArray(zone, zones);
        } else {
            zones.SetSize(1);
            zones[0] = zone;
        }
        for (int i = 0; i < zones.Size(); i++) {
            int group = zones[i];
            for (int poi = POI_GetFirst(group); poi; poi = POI_GetNext(group, poi)) {
                if (poi == except) continue;
                POI_STRUCT *p = POI_Get(poi);
                int locFlags = POI_GetLocFlags(poi);
                if (!((locFlags & 0x10000) || ((locFlags & 0x80000) && !FlagChannels[0xc9]) ||
                      ((locFlags & 0x100000) && !FlagChannels[0xca]) || ((locFlags & 0x200000) && !FlagChannels[0xcb]) ||
                      ((locFlags & 0x400000) && !FlagChannels[0xcc])))
                    continue;
                if (bPlayerNear) {
                    if (bDistCheck) {
                        EXVector d = AU_SubV(playerPos, p->m_Pos);
                        if (100.0f > d.LengthXYZSquared()) continue;
                    }
                    if (bAngleCheck) {
                        if (AU_VecAngleInside(playerPos, myPos, p->m_Pos, 40.0f)) continue;
                    }
                }
                EXVector d = AU_SubV(myPos, p->m_Pos);
                float dist = d.LengthXYZSquared();
                if (best && !(bestDist > dist)) continue;
                if (bNotFull && POI_IsFull(poi, (LocCount::Flags)0xf)) continue;
                bestDist = dist;
                best = poi;
            }
        }
        if (best) return best;
        if (bAngleCheck) bAngleCheck = 0;
        else if (bDistCheck) bDistCheck = 0;
        else return 0;
    }
}

// NEAR: written for behaviour from the disassembly (not byte-matched).
// GUESS: retail transforms (0, 0.5, 1, 0) by the jump heading and never uses the result (a dropped launch
// impulse?); kept as an unused local so the call stays.
signed char STATE_RagdollJump(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    float &rotY = sm.GetFloatRef(i);
    sm.GetActorOwner().m_Status = 0xb;
    sm.GetActorOwner().SetWatch(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        switch (stage) {
        case 0: {
            stage = 1;
            int anims[1];
            anims[0] = 0x9000440;
            sm.GetActorOwner().SetAnimType(anims[g_EXRandClass.Rand32() % 1], 0, 1);
            sm.GetActorOwner().m_ActorFlags.m_bMovementEnabled = 0;
            return 1;
        }
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) {
                EXMaths::mat_44 m;
                m.set_rotate_y(rotY);
                EXMaths::vec_xyzw v;
                v.set(0.0f, 0.5f, 1.0f, 0.0f);
                EXMaths::vec_xyzw impulse = m * v;
                (void)impulse;
                sm.GetActorOwner().EnableRagDoll();
                Actor &a = sm.GetActorOwner();
                a.Speak(a.GetScreamSFX(), 0);
                SM_GOSUB(STATE_RagDollDeath, 0, 0, 0, 0);
                return 1;
            }
            break;
        }
        return 1;
    case MSG_ENTER:
        stage = 0;
        rotY = (float)sm.m_Msg.m_Data[0] * 0.00277777785f * 6.28318548f;
        sm.GetActorOwner().SetRotY(rotY);
        return 1;
    case MSG_EXIT:
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// NEAR: written for behaviour from the disassembly; the case bodies follow retail's block order. Retail's shared
// lose-arm tail (Rand32() % n) is a cross-jump with n in a register; here it is a goto with a local.
signed char AU_HandlePlayerAttacks(StateMachine &sm, StructMsg_AttackedByPredator *pMsg)
{
    sm.GetActorOwner().SetHitBy(pMsg->m_Attack);
    if (sm.GetActorOwner().GetHealth() > 0.0f) sm.GetActorOwner().SpeakStop();
    if (AU_DoesAttackCreateLoudNoise(pMsg))
        Actor::SendMsg(MsgItem(MSG_LOUD_NOISE), sm.GetActorOwner().GetHeadPos(), 10.0f, &sm.GetActorOwner(), 0.0f);
    if (AU_DoesAttackOverrideCloak(pMsg)) Actor::OverridePlayerCloak();
    if (AU_DoesAttackGiveActorSight(sm, pMsg)) sm.GetActorOwner().PlayerForceSighting();
    if (sm.GetActorOwner().IsInvinsible())
        pMsg->m_Damage = 0.0f;
    else
        AU_AdjustDamageForArmour(sm, pMsg);
    switch (sm.GetActorOwner().GetSkelHash()) {
    case 0xd00008d:
        AU_HandlePlayerAttacks_Ulysses(sm, pMsg);
        return 1;
    case 0xd000091:
        AU_HandlePlayerAttacks_Boss(sm, pMsg);
        return 1;
    }
    if (sm.GetActorOwner().BeingJuggled()) {
        switch (pMsg->m_Attack) {
        case 0: case 1: case 2: case 4: case 5: case 7:
        case 0x31: case 0x32: case 0x33: case 0x34: case 0x35:
        case 0x36: case 0x37: case 0x38: case 0x39: case 0x3a:
            XGetGameStats()->m_Unk10[13]++;  // TODO name: juggle kills
            AR_HitRagdoll(sm, pMsg, 1);
            break;
        case 3: case 6: case 0x3b: case 0x3c: case 0x3d: case 0x3e:
            if (sm.GetActorOwner().m_AnimMode != 0x90004fb) {
                XGetGameStats()->m_Unk10[13]++;
                sm.GetActorOwner().SetAnimType(0x90004fb, 0, 1);
                sm.GetActorOwner().CreateLostLimb(ALL_BELT, pMsg->m_Dir, 0, 0);
                sm.GetActorOwner().SetOverkilled();
            }
            break;
        }
        return 1;
    }
    signed char bFlag = 0;
    KnockOver ko = KO_HITREACT;
    unsigned int chance;
    switch (pMsg->m_Attack) {
    case 8:
        sm.GetActorOwner().CheckForSkinChange(10000.0f);
        pMsg->m_Damage = 0.0f;
        sm.GetActorOwner().BreakCloak(0);
        AR_TakeHitPlayerMine(sm, pMsg, KO_HITREACT);
        break;
    case 9: {
        ArmourProfile *pArmour = ArmourProfile::GetArmourData(sm.GetActorOwner().GetProfile()->m_Armour);
        if (pArmour && pArmour->m_Damage[9] == 0.0f) break;
        AR_TakeHit(sm, pMsg, KO_DAZED);
        break;
    }
    case 0xb: {
        if (AR_TakeHitPlayerMine(sm, pMsg, KO_JUGGLE_25)) break;
        sm.GetActorOwner().SetHealth(50.0f, 2);
        EXVector d = AU_SubV(sm.GetActorOwner().GetFeetPos(), pMsg->m_Pos);
        if (4.0f > d.x * d.x + d.y * d.y + d.z * d.z)
            AU_AssignSkin(sm.GetActorOwner().m_pAnim, 0x10000e3, 0xd00015c);
        XGetGameStats()->m_Unk10[19]++;  // TODO name: actors set on fire
        SM_GOTO(STATE_BlazinSquad, (int)&pMsg->m_Pos, 0, 0, 0);
        return 1;
    }
    case 0xa: {
        if (AR_TakeHitPlayerMine(sm, pMsg, KO_JUGGLE_25)) break;
        EXVector d = AU_SubV(sm.GetActorOwner().GetFeetPos(), pMsg->m_Pos);
        if (4.0f > d.x * d.x + d.y * d.y + d.z * d.z)
            AU_AssignSkin(sm.GetActorOwner().m_pAnim, 0x10000e3, 0xd000130);
        AR_HitExplode(sm, pMsg);
        break;
    }
    case 1:
    case 0x32:
        if (AR_TakeHit(sm, pMsg, KO_HITREACT)) {
            chance = 20;
            goto loseArm;
        }
        if (g_EXRandClass.Rand32() % 3 == 0) AR_HitLoseLimb(sm, pMsg);
        if (pMsg->m_Attack != 0x32) break;
        SM_GOSUB(STATE_KnockBackwardAndDie, (int)&pMsg->m_Dir, 0, 0, 0);
        return 1;
    case 7:
        bFlag = 1;
        ko = KO_JUGGLE_25;
    case 0:
    case 2:
        if (pMsg->m_Attack == 0 && sm.GetActorOwner().WillThisDaze(pMsg->m_Damage)) ko = KO_DAZED;
    case 0x22: case 0x23: case 0x24: case 0x25: case 0x26:
        if (AR_TakeHit(sm, pMsg, ko)) {
            if (!bFlag) break;
            chance = 20;
            goto loseArm;
        }
        if (pMsg->m_Attack >= 0x22 && pMsg->m_Attack <= 0x26) XGetGameStats()->m_Unk10[9]++;  // TODO name
        if (pMsg->m_Attack == 0) break;
        if (pMsg->m_Attack == 7) {
            if (pMsg->m_Damage > 199.0f) {
                sm.GetActorOwner().EffectCreate(0x10000e3, 0x4000410, 0, 0.0f, 0, 0);
                sm.GetActorOwner().EffectCreate(0x10000e3, 0x4000423, 0, 0.0f, 0, 0);
                AU_AssignSkin(sm.GetActorOwner().m_pAnim, 0x10000e3, 0xd00015c);
                XGetGameStats()->m_Unk10[19]++;
                AR_HitRagdoll(sm, pMsg, 0);
                break;
            }
        } else if (pMsg->m_Attack >= 0x22 && pMsg->m_Attack <= 0x26) {
            Gore_CreateOverkillBlood(&sm.GetActorOwner());
            sm.GetActorOwner().SetOverkilled();
        }
        AR_HitRagdoll(sm, pMsg, 0);
        break;
    case 3:
    case 4:
        if (AR_TakeHit(sm, pMsg, KO_HITREACT)) {
            chance = 5;
        loseArm:
            if (g_EXRandClass.Rand32() % chance) break;
            if (!(50.0f > sm.GetActorOwner().GetHealth())) break;
            sm.GetActorOwner().LooseArmIfHit(pMsg->m_Pos, pMsg->m_Dir, pMsg->m_Damage);
            if (!sm.GetActorOwner().IsLeftArmMissing() && !sm.GetActorOwner().IsRightArmMissing()) break;
            if (!(sm.GetActorOwner().GetProfile()->m_Health * 0.7f > sm.GetActorOwner().GetHealth())) break;
            sm.GetActorOwner().SetGoingToStaggerAndDie(1);
            SM_GOTO(STATE_StaggerAndDie, 0, 0, 0, 0);
            return 1;
        } else {
            int state = 0;
            int unk = 0;
            XItemHandler_Player::Instance()->GetAttackState(state, unk);
            if (pMsg->m_Attack == 3) {
                if (unk == 2) {
                    AR_HitStomach(sm, pMsg);
                    break;
                }
                if (g_EXRandClass.Rand32() & 1) break;
                AR_HitLoseLimb(sm, pMsg);
            } else if (pMsg->m_Attack == 4) {
                switch (g_EXRandClass.Rand32() % 3) {
                case 0:
                    AR_HitLoseLimb(sm, pMsg);
                    break;
                case 1:
                    AR_HitRagdoll(sm, pMsg, 0);
                    break;
                }
            }
        }
        break;
    case 0x1e:
        AR_HitBarge(sm, pMsg);
        break;
    case 5: {
        signed char bHead = sm.GetActorOwner().IsHeadshot(pMsg->m_Pos, 0.25f);
        if (bHead) pMsg->m_Damage *= 10.0f;
        Gore_CreateActorBlood(&sm.GetActorOwner(), 0, 0, pMsg->m_Attack, pMsg->m_Pos, pMsg->m_Dir, 0);
        if (AR_TakeHit(sm, pMsg, KO_HITREACT)) break;
        if (bHead) {
            XGetGameStats()->m_Unk10[15]++;  // TODO name: headshots
            AR_HitLoseHeadRagdoll(sm, pMsg);
            break;
        }
        pMsg->m_Damage = 0.0f;
        AR_HitRagdoll(sm, pMsg, 0);
        break;
    }
    case 0x1f:
    case 0x20:
    case 0x21:
        AR_HeavyFinish(sm, pMsg);
        break;
    case 0x27:
        AR_StealthKillInit(sm, pMsg);
        break;
    case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
        sm.GetActorOwner().SetCloak(0);
        AR_StealthKill(sm, pMsg);
        break;
    case 0x47:
        sm.GetActorOwner().SetCloak(0);
        AR_PlayerControlDeath(sm, pMsg);
        break;
    case 6: {
        signed char bHead = sm.GetActorOwner().IsHeadshot(pMsg->m_Pos, 0.25f);
        if (bHead) pMsg->m_Damage *= 2.0f;
        Gore_CreateActorBlood(&sm.GetActorOwner(), 0, 0, pMsg->m_Attack, pMsg->m_Pos, pMsg->m_Dir, 0);
        if (AR_TakeHit(sm, pMsg, KO_HITREACT)) break;
        if (bHead) {
            XGetGameStats()->m_Unk10[15]++;
            AR_HitLoseHead(sm, pMsg);
            break;
        }
        float feetY = sm.GetActorOwner().GetFeetPos().y;
        float hitY = pMsg->m_Pos.y - feetY;
        float pelvisY = sm.GetActorOwner().GetPelvisPos().y - feetY;
        if (0.25f > (float)fabs(hitY - pelvisY)) {
            AR_HitStomach(sm, pMsg);
            break;
        }
        if (hitY > pelvisY) AR_HitLoseLimb(sm, pMsg);
        break;
    }
    case 0x4a:
        SM_GOSUB(STATE_ScaredOfPredRoar, 0, 0, 0, 0);
        return 1;
    case 0x35:
        if (AR_TakeHit(sm, pMsg, KO_JUGGLE_75)) break;
        SM_GOSUB(STATE_KnockBackwardAndDie, (int)&pMsg->m_Dir, 0, 0, 0);
        return 1;
    case 0x34:
        if (g_EXRandClass.Rand32() & 1) {
            if (AR_TakeHit(sm, pMsg, KO_JUGGLE)) break;
            SM_GOSUB(STATE_KnockBackwardAndDie, (int)&pMsg->m_Dir, 0, 0, 0);
            return 1;
        }
        if (AR_TakeHit(sm, pMsg, KO_KNOCKBACKWARD)) break;
        SM_GOSUB(STATE_KnockBackwardAndDie, (int)&pMsg->m_Dir, 0, 0, 0);
        return 1;
    case 0x31:
        if (sm.GetActorOwner().WillThisDaze(pMsg->m_Damage)) ko = KO_DAZED;
        if (AR_TakeHit(sm, pMsg, ko)) break;
        SM_GOSUB(STATE_KnockBackwardAndDie, (int)&pMsg->m_Dir, 0, 0, 0);
        return 1;
    case 0x36:
    case 0x3a:
    case 0x3e:
        if (AR_TakeHit(sm, pMsg, KO_KNOCKBACKWARD)) break;
        SM_GOSUB(STATE_KnockBackwardAndDie, (int)&pMsg->m_Dir, 0, 0, 0);
        return 1;
    case 0x33:
        if (AR_TakeHit(sm, pMsg, KO_KNOCKBACKWARD)) break;
        SM_GOSUB(STATE_KnockBackwardAndDie, (int)&pMsg->m_Dir, 0, 0, 0);
        return 1;
    case 0x37: {
        Gore_CreateActorBlood(&sm.GetActorOwner(), 0, 0, 0x37, pMsg->m_Pos, pMsg->m_Dir, 1);
        if (AR_TakeHit(sm, pMsg, KO_KNOCKBACKWARD)) break;
        AR_HitLoseHead(sm, pMsg);
        float feetY = sm.GetActorOwner().GetFeetPos().y;
        float hitY = pMsg->m_Pos.y - feetY;
        if (hitY > sm.GetActorOwner().GetPelvisPos().y - feetY + 0.25f) AR_HitLoseLimb(sm, pMsg);
        SM_GOSUB(STATE_KnockBackwardAndDie, (int)&pMsg->m_Dir, 0, 0, 0);
        return 1;
    }
    case 0x39: {
        signed char bHead = sm.GetActorOwner().IsHeadshot(pMsg->m_Pos, 0.5f);
        if (bHead) pMsg->m_Damage *= 2.0f;
        Gore_CreateActorBlood(&sm.GetActorOwner(), 0, 0, pMsg->m_Attack, pMsg->m_Pos, pMsg->m_Dir, 1);
        if (AR_TakeHit(sm, pMsg, KO_BARGE)) break;
        if (bHead) AR_HitLoseHead(sm, pMsg);
        float feetY = sm.GetActorOwner().GetFeetPos().y;
        float hitY = pMsg->m_Pos.y - feetY;
        float pelvisY = sm.GetActorOwner().GetPelvisPos().y - feetY;
        if (0.5f > (float)fabs(hitY - pelvisY)) {
            AR_HitStomach(sm, pMsg);
            break;
        }
        if (hitY > pelvisY) AR_HitLoseLimb(sm, pMsg);
        SM_GOSUB(STATE_KnockBackwardAndDie, (int)&pMsg->m_Dir, 0, 0, 0);
        return 1;
    }
    case 0x38: {
        Gore_CreateActorBlood(&sm.GetActorOwner(), 0, 0, 0x38, pMsg->m_Pos, pMsg->m_Dir, 1);
        if (AR_TakeHit(sm, pMsg, KO_BARGE)) break;
        float feetY = sm.GetActorOwner().GetFeetPos().y;
        float hitY = pMsg->m_Pos.y - feetY;
        float pelvisY = sm.GetActorOwner().GetPelvisPos().y - feetY;
        if (0.5f > (float)fabs(hitY - pelvisY)) {
            AR_HitStomach(sm, pMsg);
            break;
        }
        if (hitY > pelvisY) AR_HitLoseLimb(sm, pMsg);
        SM_GOSUB(STATE_KnockBackwardAndDie, (int)&pMsg->m_Dir, 0, 0, 0);
        return 1;
    }
    case 0x3b: {
        Gore_CreateActorBlood(&sm.GetActorOwner(), 0, 0, 0x3b, pMsg->m_Pos, pMsg->m_Dir, 1);
        if (AR_TakeHit(sm, pMsg, KO_BARGE)) break;
        float feetY = sm.GetActorOwner().GetFeetPos().y;
        float hitY = pMsg->m_Pos.y - feetY;
        float pelvisY = sm.GetActorOwner().GetPelvisPos().y - feetY;
        if (0.5f > (float)fabs(hitY - pelvisY)) {
            AR_HitStomach(sm, pMsg);
            break;
        }
        if (hitY > pelvisY) AR_HitLoseLimb(sm, pMsg);
        break;
    }
    case 0x3c:
    case 0x3d: {
        signed char bHead = sm.GetActorOwner().IsHeadshot(pMsg->m_Pos, 0.25f);
        if (bHead) pMsg->m_Damage *= 2.0f;
        Gore_CreateActorBlood(&sm.GetActorOwner(), 0, 0, pMsg->m_Attack, pMsg->m_Pos, pMsg->m_Dir, 1);
        if (AR_TakeHit(sm, pMsg, KO_BARGE)) break;
        if (bHead) {
            AR_HitLoseHead(sm, pMsg);
            break;
        }
        float feetY = sm.GetActorOwner().GetFeetPos().y;
        float hitY = pMsg->m_Pos.y - feetY;
        float pelvisY = sm.GetActorOwner().GetPelvisPos().y - feetY;
        if (0.5f > (float)fabs(hitY - pelvisY)) {
            AR_HitStomach(sm, pMsg);
            break;
        }
        if (hitY > pelvisY) AR_HitLoseLimb(sm, pMsg);
        break;
    }
    case 0x4b: {
        AR_HitRagdoll(sm, pMsg, 0);
        float rx = g_EXRandClass.Randf() * 0.5f - 0.25f;
        float rz = g_EXRandClass.Randf() * 0.5f - 0.25f;
        EXVector up(rx, 1.0f, rz, 0.0f);
        up.Normalise();
        sm.GetActorOwner().ApplyRagDollForce(Player_GetFeetPos(), up, 7.0f, -1);
        sm.GetActorOwner().ExplodeLimbs(AU_MulV(pMsg->m_Dir, pMsg->m_Damage), 1);
        sm.GetActorOwner().Speak(0x1af000d1, 0);
        XGetGameStats()->m_Unk10[9]++;
        break;
    }
    case 0x30:
    case 0x48:
    case 0x49:
        break;
    }
    switch (pMsg->m_Attack) {
    case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
    case 0x1e:
    case 0x4a:
        sm.GetActorOwner().ResetCivilianAttackVO();
        break;
    }
    return 1;
}

// NEAR: logic complete; retail loads both vectors whole before the inlined subtract (different temp layout).
signed char STATE_HitByVehicle(StateMachine &sm)
{
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        sm.Return(0, 0, 0, 0);
        return 1;
    case MSG_ENTER:
        sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        return 1;
    case MSG_EXT: {
        Vehicle *pVehicle = sm.GetActorOwner().m_pHitByVehicle;
        if (pVehicle) {
            EXVector d = AU_SubV(sm.GetActorOwner().HitByVehiclePos(), pVehicle->m_pItem->Position());
            if (!(d.LengthXZSquared() > 1.0f)) return 1;
        }
        sm.Return(0, 0, 0, 0);
        return 1;
    }
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

// NEAR: retail inlines EXVector::operator- through two temp copies (0x40 frame), fsin/fcos order
// differs and the non-drone path jumps to the MSG_EXT Return block.
signed char STATE_NudgeCloaked(StateMachine &sm)
{
    sm.GetActorOwner().SetWatch(1);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (sm.GetActorOwner().HasAnimEnded()) {
    ret:
            sm.Return(0, 0, 0, 0);
        }
        return 1;
    case MSG_ENTER: {
        if (!sm.GetActorOwner().IsDrone()) goto ret;
        sm.GetActorOwner().SetAnimType(0, 0, 1);
        sm.GetActorOwner().IncNudgeCount();
        float s = (float)sin(sm.GetActorOwner().GetRotY());
        float c = (float)cos(sm.GetActorOwner().GetRotY());
        EXVector d = AU_Sub(Player_GetFeetPos(), sm.GetActorOwner().GetFeetPos());
        if (d.z * -s + d.x * c > 0.0f)
            sm.GetActorOwner().SetAnimType(0x900041c, 0, 1);
        else
            sm.GetActorOwner().SetAnimType(0x900041d, 0, 1);
        sm.GetActorOwner().SayNudgedVO();
        return 1;
    }
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

// File-local tuning constant (retail loads it from ActorUtil data, not a shared __real).
static const float s_QuickEvadeArriveSq = 0.0100000007f;

// NEAR: retail cross-jumps the stage 0/1 EffectCreate tails into one block (10 bytes shorter); registers follow.
signed char STATE_ElectroSquad(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetStatus(3);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXIT:
        return 1;
    case MSG_ENTER: {
        sm.GetActorOwner().m_ActorFlags2.m_bOnFire = 1;
        stage = 0;
        timer = (g_EXRandClass.Randf() + 4.0f) * 60.0f;
        int anims[3];
        anims[0] = 0x90002b2;
        anims[1] = 0x90003d2;
        anims[2] = 0x90003d1;
        int anim = anims[g_EXRandClass.Rand32() % 3];
        sm.GetActorOwner().SetAnimType(anim, 0, 1);
        return 1;
    }
    case MSG_EXT:
        switch (stage) {
        case 0:
            timer -= XApp::GameFrameRateMul();
            if (timer <= 0.0f) {
                stage = 1;
                timer = (g_EXRandClass.Randf() + 1.5f) * 60.0f;
                sm.GetActorOwner().EffectCreate(0x10000e3, 0x4000423, 0u, 0.0f, 0, 0);
            }
            break;
        case 1:
            timer -= XApp::GameFrameRateMul();
            if (timer <= 0.0f) {
                stage = 2;
                timer = (g_EXRandClass.Randf() + 4.0f) * 60.0f;
                sm.GetActorOwner().EffectCreate(0x10000e3, 0x4000410, 0u, 0.0f, 0, 0);
            }
            break;
        case 2:
            timer -= XApp::GameFrameRateMul();
            if (timer <= 0.0f) {
                SM_GOSUB(STATE_DeathBullet, 0, 0, 0, 0);
                return 1;
            }
            break;
        }
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

signed char STATE_QuickEvadePlayer(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    sm.GetActorOwner().SetStatus(0xd);
    sm.GetActorOwner().SetWatch(4);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXIT:
        POI_VendorUnbind(poi, sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_ENTER:
        poi = AU_FindRunAwayPoint(sm, 0x2000, poi, sm.GetActorOwner().GetNavZone(), 1);
        if (!poi) goto done;
        POI_VendorBind(poi, sm.GetActorOwner().GetRefID());
        sm.GetActorOwner().GotoPOI(poi);
        return 1;
    case MSG_EXT:
        gAIControl.IncActorsInCombatCount();
        sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
        if (s_QuickEvadeArriveSq > sm.GetActorOwner().SqDistToTarget()) {
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(gAIControl.m_LastSeenPlayerPos, 5.0f)) {
                sm.GetActorOwner().InvalidateGotoPos();
            done:
                sm.Return(0, 0, 0, 0);
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(gAIControl.m_LastSeenPlayerPos, 5.0f);
            return 1;
        }
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

signed char STATE_BurnTheDanceFloor(StateMachine &sm)
{
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetStatus(3);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXIT:
        return 1;
    case MSG_ENTER:
        sm.GetActorOwner().SetOnFire();
        sm.GetActorOwner().SetAnimType(0x90002b2, 0, 1);
        sm.GetActorOwner().EffectCreate(0x10000e3, 0x4000410, 0, 0.0f, 0, 0);
        sm.GetActorOwner().EffectCreate(0x10000e3, 0x4000423, 0, 0.0f, 0, 0);
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_EXT:
        return 1;
    }
    return 0;
}

signed char STATE_RagDollGrabbed(StateMachine &sm)
{
    sm.GetActorOwner().SetStatus(0);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHealth(0.0f, 1);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_EXT:
        sm.GetActorOwner().UpdateRagdollGrabPoint();
        return 1;
    case MSG_ENTER:
        sm.GetActorOwner().InvalidateGotoPos();
        sm.GetActorOwner().SetCarry(0, 1);
        sm.GetActorOwner().SetCarry(0, 0);
        return 1;
    }
    return 0;
}

signed char AU_HandleSomeoneSeenPredator(StateMachine &sm)
{
    if (!(sm.GetActorOwner().GetWatchFlags() & 1)) return 1;
    int hash = sm.GetActorOwner().GetProfile()->m_UnderAttackState;
    if (hash) {
        EXVector d = AU_SubV(sm.GetActorOwner().GetFeetPos(), Player_GetFeetPos());
        float range = Sq(sm.GetActorOwner().GetProfile()->m_AttackRange);
        if (5.0f > range && d.LengthXYZSquared() > range) return 1;
        while (sm.GetActorOwner().SearchBrainStateStack(STATE_SearchForPredator)) sm.PopState();
        while (sm.GetActorOwner().SearchBrainStateStack(STATE_NudgeCloaked)) sm.PopState();
        sm.GetActorOwner().PlayerForceSighting();
        sm.Gosub(GetMsgBitsForMsg(MSG_SOMEONE_SEEN_PREDATOR) | GetMsgBitsForMsg(MSG_SOMEONE_UNDER_ATTACK) | AW_BIT_HAVE_SEEN_PLAYER,
                 AP_GetStateFromHash(hash), 0, 0, 0, 0);
    }
    return 1;
}

signed char STATE_FindPred(StateMachine &sm)
{
    int i = 0;
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().m_Status = 5;
    sm.GetActorOwner().SetWatch(0x16);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        timer -= XGetFrameRateMul();
        if (!(0.0f > timer) && !sm.GetActorOwner().m_StateFlags.m_bPlayerSeen) {
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(sm.GetActorOwner().GetRotY() + 0.06f, 5.0f))
                sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            else
                sm.GetActorOwner().SetTargetYRot(sm.GetActorOwner().GetRotY() + 0.06f, 5.0f);
            return 1;
        }
        sm.Return(0, 0, 0, 0);
        return 1;
    case MSG_ENTER:
        timer = EXBaseApp::NominalGameFrameRate() * 2.0f;
        sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        return 1;
    case MSG_EXIT:
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_FindHeal(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    float &heal = sm.GetFloatRef(i);
    float &lastFrame = sm.GetFloatRef(i);
    sm.GetActorOwner().m_Status = 0xb;
    sm.GetActorOwner().SetWatch(4);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        gAIControl.m_ActorsInCombatCount++;
        switch (stage) {
        case 0:
            if (poi && sm.GetActorOwner().SqDistToTarget() < 1.0f) stage = 1;
            break;
        case 1: {
            if (sm.GetActorOwner().m_ActorFlags.m_bBoss)
                sm.GetActorOwner().SetAnimType(0x9000398, 0, 1);
            else
                sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            sm.GetActorOwner().EffectCreate(0x40004ac, 0, 0, 1.0f, 0, 0);
            float health = sm.GetActorOwner().GetHealth();
            heal = sm.GetActorOwner().GetProfile()->m_Health - health;
            lastFrame = 0.0f;
            stage = 2;
            break;
        }
        case 2: {
            float frame = sm.GetActorOwner().GetAnimFrame();
            sm.GetActorOwner().ChangeHealth((frame - lastFrame) * heal, 0);
            lastFrame = frame;
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.Return(0, 0, 0, 0);
                return 1;
            }
            break;
        }
        }
        return 1;
    case MSG_ENTER:
        poi = AU_FindHealPoint(sm, 0, sm.GetActorOwner().GetNavZone(), 0);
        if (!poi) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
        sm.GetActorOwner().GotoPOI(poi);
        stage = 0;
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// NEAR: written for behaviour from the disassembly (not byte-matched); STATE_SeenBody_Panic with a
// warning shout on entry and ragdoll-jump POIs (type 0x420000bd).
signed char STATE_PanicBlind(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    int &bJump = sm.GetIntRef(i);
    sm.GetActorOwner().m_Status = 3;
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        switch (stage) {
        case 0:
            poi = AU_FindRunAwayPoint(sm, 5, poi, sm.GetActorOwner().GetNavZone(), 0);
            if (!poi) {
                sm.GetActorOwner().InvalidateGotoPos();
                sm.GetActorOwner().SetAnimType(0x9000018, 0, 1);
                stage = 2;
            } else {
                stage = 1;
                bJump = POI_Get(poi)->m_Type == 0x420000bd;
                int anims[2];
                anims[0] = 0x4e00000d;
                anims[1] = 0x900013d;
                sm.GetActorOwner().SetAnimType(anims[g_EXRandClass.Rand32() & 1], 0, 1);
                sm.GetActorOwner().GotoPOI(poi);
                sm.GetActorOwner().Speak(0x1af000e1, 0);
            }
            break;
        case 1:
            if (bJump) {
                if (kArriveSqDist > sm.GetActorOwner().SqDistToTarget()) {
                    SM_GOTO(STATE_RagdollJump, (int)(POI_Get(poi)->m_YRot * 0.159154937f * 360.0f), 0, 0, 0);
                    return 1;
                }
            } else if (1.0f > sm.GetActorOwner().SqDistToTarget()) {
                stage = 0;
            }
            break;
        case 2:
            if (sm.GetActorOwner().HasAnimEnded()) sm.GetActorOwner().SetAnimType(0x90004b2, 0, 1);
            break;
        }
        return 1;
    case MSG_ENTER:
        Actor::SendMsg(MsgItem(MSG_LOUD_NOISE), sm.GetActorOwner().GetItem()->m_position, 10.0f, &sm.GetActorOwner(), -1.0f);
        stage = 0;
        sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// NEAR: register residue only (retail loads poi into ecx for GotoPOI, which shifts later ecx/edx picks).
signed char STATE_SeenBody_Panic(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    sm.GetActorOwner().m_Status = 3;
    sm.GetActorOwner().SetWatch(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        switch (stage) {
        case 0:
            poi = AU_FindRunAwayPoint(sm, 5, poi, sm.GetActorOwner().GetNavZone(), 0);
            if (!poi) {
                sm.GetActorOwner().SetAnimType(0x9000018, 0, 1);
                stage = 2;
            } else {
                stage = 1;
                int anims[2];
                anims[0] = 0x4e00000d;
                anims[1] = 0x900013d;
                int anim = anims[g_EXRandClass.Rand32() & 1];
                sm.GetActorOwner().SetAnimType(anim, 0, 1);
                sm.GetActorOwner().GotoPOI(poi);
                sm.GetActorOwner().Speak(0x1af000e1, 0);
            }
            break;
        case 1:
            if (poi && sm.GetActorOwner().SqDistToTarget() < kArriveSqDist) stage = 0;
            break;
        case 2:
            if (sm.GetActorOwner().HasAnimEnded()) sm.GetActorOwner().SetAnimType(0x90004b2, 0, 1);
            break;
        }
        return 1;
    case MSG_ENTER:
        stage = 0;
        sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_FS_Cower(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    sm.GetActorOwner().m_Status = 3;
    sm.GetActorOwner().SetHeadTrackMode(0);
    sm.GetActorOwner().SetWatch(4);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        switch (stage) {
        case 0:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 1;
                sm.GetActorOwner().SetAnimType(0x90004b2, 0, 1);
            }
            break;
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 2;
                POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
            }
            break;
        }
        return 1;
    case MSG_EXIT:
        POI_Unbind(poi, sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_ENTER:
        poi = sm.m_Msg.m_Data[0];
        stage = 0;
        sm.GetActorOwner().SetAnimType(0x9000018, 0, 1);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_FS_Pray(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    sm.GetActorOwner().m_Status = 3;
    sm.GetActorOwner().SetHeadTrackMode(0);
    sm.GetActorOwner().SetWatch(4);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (!stage && sm.GetActorOwner().HasAnimEnded()) {
            stage = 1;
            POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
        }
        return 1;
    case MSG_EXIT:
        POI_Unbind(poi, sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_ENTER:
        poi = sm.m_Msg.m_Data[0];
        stage = 0;
        sm.GetActorOwner().SetAnimType(0x9000155, 0, 1);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_FS_Pray_Ignore(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    sm.GetActorOwner().m_Status = 3;
    sm.GetActorOwner().SetHeadTrackMode(0);
    sm.GetActorOwner().SetWatch(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (!stage && sm.GetActorOwner().HasAnimEnded()) {
            stage = 1;
            POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
        }
        return 1;
    case MSG_EXIT:
        POI_Unbind(poi, sm.GetActorOwner().GetRefID());
        return 1;
    case MSG_ENTER:
        poi = sm.m_Msg.m_Data[0];
        stage = 0;
        sm.GetActorOwner().SetAnimType(0x9000155, 0, 1);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_StandStill(StateMachine &sm)
{
    sm.GetActorOwner().m_Status = 0xb;
    sm.GetActorOwner().SetWatch(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        return 1;
    case MSG_ENTER:
        return 1;
    case MSG_EXIT:
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_GotoLocationNearestType(StateMachine &sm)
{
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER: {
        int loc = POI_GetNearestType(sm.GetActorOwner().GetFeetPos(), sm.m_Msg.m_Data[0], 0, sm.GetActorOwner().GetNavZone(),
                                     sm.GetActorOwner().CanJump(), (sm.m_Msg.m_Data[1] & 0x400) > 0);
        if (!loc) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        SM_GOTO(STATE_GotoLocation, loc, sm.m_Msg.m_Data[1], 0, 0);
        return 1;
    }
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_GotoLocationNearestFlag(StateMachine &sm)
{
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER: {
        int loc = POI_GetNearestFlag(sm.GetActorOwner().GetFeetPos(), sm.m_Msg.m_Data[0], 0, sm.GetActorOwner().GetNavZone(),
                                     sm.GetActorOwner().CanJump(), (sm.m_Msg.m_Data[1] & 0x400) > 0, -1.0f);
        if (!loc) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        SM_GOTO(STATE_GotoLocation, loc, sm.m_Msg.m_Data[1], 0, 0);
        return 1;
    }
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_POIForceState(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &flags = sm.GetIntRef(i);
    int &stateHash = sm.GetIntRef(i);
    float &rotY = sm.GetFloatRef(i);
    sm.GetActorOwner().m_Status = 0xb;
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (flags & 0x800) sm.GetActorOwner().SetWatch(0);
        if (sm.GetActorOwner().IsFacingYRot(rotY, 15.0f)) {
            SM_GOTO(AP_GetStateFromHash(stateHash), poi, 0, 0, 0);
            return 1;
        }
        sm.GetActorOwner().SetTargetYRot(rotY, 15.0f);
        return 1;
    case MSG_ENTER:
        poi = sm.m_Msg.m_Data[0];
        stateHash = sm.m_Msg.m_Data[1];
        flags = sm.m_Msg.m_Data[2];
        sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
        if (flags & 0x800) sm.GetActorOwner().SetWatch(0);
        if (POI_Get(poi)->m_nCustomers + POI_Get(poi)->m_nVendors > 1) {
            EXVector d = AU_SubV(POI_Get(poi)->m_Pos, sm.GetActorOwner().GetFeetPos());
            rotY = (float)atan2(d.x, d.z);
        } else {
            rotY = POI_Get(poi)->m_YRot;
        }
        return 1;
    case MSG_EXIT:
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_MissionEnd(StateMachine &sm)
{
    sm.GetActorOwner().m_Status = 0xb;
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        sm.GetActorOwner().Delete();
        sm.Return(0, 0, 0, 0);
        return 1;
    case MSG_ENTER:
        SM_GOSUB(STATE_GotoLocationNearestFlag, 0x1000, 0x110, 0, 0);
        return 1;
    case MSG_EXIT:
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}


signed char STATE_FreeFall(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    sm.GetActorOwner().m_Status = 0xb;
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        switch (stage) {
        case 0:
            if ((float)fabs(sm.GetActorOwner().GetFeetPos().y - sm.GetActorOwner().GetNavFloorPos().y) <= kFreeFallLandDist) {
                if (sm.GetActorOwner().IsBoss())
                    sm.GetActorOwner().SetAnimType(0x900022c, 0, 1);
                else
                    sm.GetActorOwner().SetAnimType(0x900028e, 0, 1);
                stage = 1;
            }
            break;
        case 1:
            if (sm.GetActorOwner().HasAnimEnded())
                SM_GOTO(AP_GetStateFromHash(sm.GetActorOwner().GetProfile()->m_StartState), 0, 0, 0, 0);
            break;
        }
        return 1;
    case MSG_ENTER:
        stage = 0;
        sm.GetActorOwner().SetAnimType(0x900028f, 0, 1);
        return 1;
    case MSG_EXIT:
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_RunToDestination(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (sm.GetActorOwner().SqDistToTarget() < kArriveSqDist) {
            POI_VendorBind(poi, sm.GetActorOwner().GetRefID());
            POI_VendorUnbind(poi, sm.GetActorOwner().GetRefID());
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        if (sm.GetActorOwner().m_StateFlags.m_bPlayerSeen) {
            SM_GOSUB(STATE_AttackPred, 0, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_ENTER:
        poi = POI_ResolveLocation(sm.m_Msg.m_Data[0], sm.GetActorOwner().GetNavZone(), sm.GetActorOwner().CanJump(),
                                  &sm.GetActorOwner().GetFeetPos());
        if (!poi) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        sm.GetActorOwner().GotoPOI(poi);
        sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
        return 1;
    case MSG_EXIT:
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

AU_STATE_ENTER_GOTO(STATE_RunToDestination1, STATE_RunToDestination, 0x42000049)
AU_STATE_ENTER_GOTO(STATE_RunToDestination2, STATE_RunToDestination, 0x4200004a)
AU_STATE_ENTER_GOTO(STATE_RunToDestination3, STATE_RunToDestination, 0x4200004b)
AU_STATE_ENTER_GOTO(STATE_RunToDestination4, STATE_RunToDestination, 0x4200004c)
AU_STATE_ENTER_GOTO(STATE_RunToDestination5, STATE_RunToDestination, 0x4200004d)
AU_STATE_ENTER_GOTO(STATE_RunToDestination6, STATE_RunToDestination, 0x4200004e)
AU_STATE_ENTER_GOTO(STATE_RunToDestination7, STATE_RunToDestination, 0x4200004f)
AU_STATE_ENTER_GOTO(STATE_RunToDestination8, STATE_RunToDestination, 0x42000050)
AU_STATE_ENTER_GOTO(STATE_RunToDestination9, STATE_RunToDestination, 0x42000051)
AU_STATE_ENTER_GOTO(STATE_RunToDestination10, STATE_RunToDestination, 0x42000052)
signed char STATE_CryForever(StateMachine &sm)
{
    int i = 0;
    int &anim = sm.GetIntRef(i);
    sm.GetActorOwner().m_Status = 3;
    sm.GetActorOwner().SetWatch(4);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (sm.GetActorOwner().HasAnimEnded()) sm.GetActorOwner().SetAnimType(anim, 0, 1);
        return 1;
    case MSG_ENTER: {
        int anims[2];
        anims[0] = 0x9000155;
        anims[1] = 0x90004b2;
        anim = anims[g_EXRandClass.Rand32() & 1];
        sm.GetActorOwner().SetAnimType(0x9000018, 0, 1);
        return 1;
    }
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_PanicStandForever(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    sm.GetActorOwner().m_Status = 3;
    sm.GetActorOwner().SetWatch(4);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (stage && sm.GetActorOwner().HasAnimEnded()) {
            stage = 0;
            sm.GetActorOwner().SetAnimType(0x9000155, 0, 1);
        }
        return 1;
    case MSG_ENTER: {
        sm.GetActorOwner().SetCarry(0, 0);
        sm.GetActorOwner().SetCarry(1, 0);
        stage = 0;
        int anims[2];
        anims[0] = 0x9000018;
        anims[1] = 0x9000149;
        int anim = anims[g_EXRandClass.Rand32() & 1];
        if (anim == 0x9000018) stage = 1;
        sm.GetActorOwner().SetAnimType(anim, 0, 1);
        return 1;
    }
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_RunDestSnipeStand(StateMachine &sm)
{
    sm.GetActorOwner().m_Status = 4;
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        SM_GOTO(STATE_SniperStand, 0, 0, 0, 0);
        return 1;
    case MSG_ENTER:
        SM_GOSUB(STATE_RunToDestination, sm.m_Msg.m_Data[0], 0, 0, 0);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

AU_STATE_ENTER_GOTO(STATE_RunDestSnipeStand1, STATE_RunDestSnipeStand, 0x4200005b)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeStand2, STATE_RunDestSnipeStand, 0x4200005c)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeStand3, STATE_RunDestSnipeStand, 0x42000060)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeStand4, STATE_RunDestSnipeStand, 0x42000061)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeStand5, STATE_RunDestSnipeStand, 0x42000062)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeStand6, STATE_RunDestSnipeStand, 0x42000063)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeStand7, STATE_RunDestSnipeStand, 0x42000064)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeStand8, STATE_RunDestSnipeStand, 0x42000065)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeStand9, STATE_RunDestSnipeStand, 0x42000066)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeStand10, STATE_RunDestSnipeStand, 0x42000067)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeStand11, STATE_RunDestSnipeStand, 0x42000068)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeStand12, STATE_RunDestSnipeStand, 0x42000069)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeStand13, STATE_RunDestSnipeStand, 0x4200006a)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeStand14, STATE_RunDestSnipeStand, 0x4200006b)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeStand15, STATE_RunDestSnipeStand, 0x4200006c)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeStand16, STATE_RunDestSnipeStand, 0x4200006d)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeStand17, STATE_RunDestSnipeStand, 0x4200006e)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeStand18, STATE_RunDestSnipeStand, 0x4200006f)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeStand19, STATE_RunDestSnipeStand, 0x42000070)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeStand20, STATE_RunDestSnipeStand, 0x42000071)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeStand21, STATE_RunDestSnipeStand, 0x42000072)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeStand22, STATE_RunDestSnipeStand, 0x42000073)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeStand23, STATE_RunDestSnipeStand, 0x42000074)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeStand24, STATE_RunDestSnipeStand, 0x42000075)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeStand25, STATE_RunDestSnipeStand, 0x42000076)
signed char STATE_RunDestSnipeMove(StateMachine &sm)
{
    sm.GetActorOwner().m_Status = 4;
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        SM_GOTO(STATE_SniperMove, 0, 0, 0, 0);
        return 1;
    case MSG_ENTER:
        SM_GOSUB(STATE_RunToDestination, sm.m_Msg.m_Data[0], 0, 0, 0);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

AU_STATE_ENTER_GOTO(STATE_RunDestSnipeMove1, STATE_RunDestSnipeMove, 0x4200005b)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeMove2, STATE_RunDestSnipeMove, 0x4200005c)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeMove3, STATE_RunDestSnipeMove, 0x42000060)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeMove4, STATE_RunDestSnipeMove, 0x42000061)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeMove5, STATE_RunDestSnipeMove, 0x42000062)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeMove6, STATE_RunDestSnipeMove, 0x42000063)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeMove7, STATE_RunDestSnipeMove, 0x42000064)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeMove8, STATE_RunDestSnipeMove, 0x42000065)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeMove9, STATE_RunDestSnipeMove, 0x42000066)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeMove10, STATE_RunDestSnipeMove, 0x42000067)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeMove11, STATE_RunDestSnipeMove, 0x42000068)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeMove12, STATE_RunDestSnipeMove, 0x42000069)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeMove13, STATE_RunDestSnipeMove, 0x4200006a)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeMove14, STATE_RunDestSnipeMove, 0x4200006b)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeMove15, STATE_RunDestSnipeMove, 0x4200006c)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeMove16, STATE_RunDestSnipeMove, 0x4200006d)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeMove17, STATE_RunDestSnipeMove, 0x4200006e)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeMove18, STATE_RunDestSnipeMove, 0x4200006f)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeMove19, STATE_RunDestSnipeMove, 0x42000070)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeMove20, STATE_RunDestSnipeMove, 0x42000071)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeMove21, STATE_RunDestSnipeMove, 0x42000072)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeMove22, STATE_RunDestSnipeMove, 0x42000073)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeMove23, STATE_RunDestSnipeMove, 0x42000074)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeMove24, STATE_RunDestSnipeMove, 0x42000075)
AU_STATE_ENTER_GOTO(STATE_RunDestSnipeMove25, STATE_RunDestSnipeMove, 0x42000076)
signed char STATE_RunDestCower(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    sm.GetActorOwner().m_Status = 3;
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        switch (stage) {
        case 0:
            sm.GetActorOwner().SetAnimType(0x9000018, 0, 1);
            stage = 1;
            break;
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) sm.GetActorOwner().SetAnimType(0x90004b2, 0, 1);
            break;
        }
        return 1;
    case MSG_ENTER:
        stage = 0;
        SM_GOSUB(STATE_RunToDestination, sm.m_Msg.m_Data[0], 0, 0, 0);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

AU_STATE_ENTER_GOTO(STATE_RunDestCower1, STATE_RunDestCower, 0x42000055)
AU_STATE_ENTER_GOTO(STATE_RunDestCower2, STATE_RunDestCower, 0x42000056)
signed char STATE_RunDestDelete(StateMachine &sm)
{
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        sm.GetActorOwner().Delete();
        return 1;
    case MSG_ENTER:
        SM_GOSUB(STATE_RunToDestination, sm.m_Msg.m_Data[0], 0, 0, 0);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

AU_STATE_ENTER_GOTO(STATE_RunDestDelete1, STATE_RunDestDelete, 0x42000059)
AU_STATE_ENTER_GOTO(STATE_RunDestDelete2, STATE_RunDestDelete, 0x4200005a)
AU_STATE_ENTER_GOTO(STATE_RunDestDelete3, STATE_RunDestDelete, 0x42000077)
AU_STATE_ENTER_GOTO(STATE_RunDestDelete4, STATE_RunDestDelete, 0x42000078)
AU_STATE_ENTER_GOTO(STATE_RunDestDelete5, STATE_RunDestDelete, 0x42000079)
AU_STATE_ENTER_GOTO(STATE_RunDestDelete6, STATE_RunDestDelete, 0x4200007a)
AU_STATE_ENTER_GOTO(STATE_RunDestDelete7, STATE_RunDestDelete, 0x4200007b)
AU_STATE_ENTER_GOTO(STATE_RunDestDelete8, STATE_RunDestDelete, 0x4200007c)
AU_STATE_ENTER_GOTO(STATE_RunDestDelete9, STATE_RunDestDelete, 0x4200007d)
AU_STATE_ENTER_GOTO(STATE_RunDestDelete10, STATE_RunDestDelete, 0x4200007e)
signed char STATE_WalkToLocation(StateMachine &sm)
{
    sm.GetActorOwner().m_Status = 0xb;
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        SM_GOTO(STATE_GotoLocationNearestType, sm.m_Msg.m_Data[0], 0x5a0, 0, 0);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_WalkToProstitution(StateMachine &sm)
{
    sm.GetActorOwner().m_Status = 0xb;
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        SM_GOTO(STATE_GotoLocationNearestType, 0x4200001a, 0x522, 0, 0);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_WalkToLocationIgnore(StateMachine &sm)
{
    sm.GetActorOwner().m_Status = 0xb;
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        SM_GOTO(STATE_GotoLocationNearestType, sm.m_Msg.m_Data[0], 0xda0, 0, 0);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_RunToLocation(StateMachine &sm)
{
    sm.GetActorOwner().m_Status = 0xb;
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        SM_GOTO(STATE_GotoLocationNearestType, sm.m_Msg.m_Data[0], 0x7a0, 0, 0);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_RunToLocationIgnore(StateMachine &sm)
{
    sm.GetActorOwner().m_Status = 0xb;
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        SM_GOTO(STATE_GotoLocationNearestType, sm.m_Msg.m_Data[0], 0xfa0, 0, 0);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_StandAttractedToPlayer(StateMachine &sm)
{
    int i = 0;
    EXVector &target = sm.GetVectorRef(i);
    float &timer = sm.GetFloatRef(i);
    float &rotY = sm.GetFloatRef(i);
    int &stage = sm.GetIntRef(i);
    float &voTimer = sm.GetFloatRef(i);
    int &bSaidVO = sm.GetIntRef(i);
    sm.GetActorOwner().m_Status = 5;
    sm.GetActorOwner().SetWatch(1);
    if (sm.m_Msg.m_Type == MSG_ATTRACT) {
        timer = g_EXRandClass.Randf() * 120.0f + 360.0f;
        target = Player_GetFeetPos();
        bSaidVO = 0;
        voTimer = g_EXRandClass.Randf() * 120.0f + 60.0f;
    }
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (!bSaidVO) {
            voTimer -= XApp::GameFrameRateMul();
            if (0.0f > voTimer) {
                sm.GetActorOwner().SayLuredVO();
                bSaidVO = 1;
            }
        }
        switch (stage) {
        case 0: {
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(target, 5.0f)) {
                ActorCarryable *pCarry = sm.GetActorOwner().GetCarryPtr(1);
                WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
                if (pWeapon && pCarry && pCarry->m_bGun && pWeapon->m_AimType != 0x52000004)
                    sm.GetActorOwner().SetAnimType(0x9000166, 0, 1);
                else
                    sm.GetActorOwner().SetAnimType(0x9000065, 0, 1);
            } else {
                sm.GetActorOwner().SetTargetYRot(target, 5.0f);
            }
            timer -= XApp::GameFrameRateMul();
            if (0.0f > timer) stage = 1;
            break;
        }
        case 1:
            if (sm.GetActorOwner().IsFacingYRot(rotY, 5.0f)) {
                sm.Return(1, 0, 0, 0);
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(rotY, 5.0f);
            return 1;
        }
        return 1;
    case MSG_ENTER:
        timer = g_EXRandClass.Randf() * 120.0f + 360.0f;
        rotY = sm.GetActorOwner().GetRotY();
        target = Player_GetFeetPos();
        bSaidVO = 0;
        voTimer = g_EXRandClass.Randf() * 120.0f + 60.0f;
        return 1;
    case MSG_EXIT:
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// NEAR: written for behaviour from the disassembly (not byte-matched). Picks an accomplice among the
// POI's customers (up to 20 tries), turns to it and signals it with message 0x13 (name unknown).
signed char STATE_CrimePrepare(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    int &accomplice = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().m_Status = 6;
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        switch (stage) {
        case 0:
            timer -= XApp::GameFrameRateMul();
            if (timer <= 0.0f && POI_GetActorCount(poi, 0xb) > 0) {
                unsigned int n = POI_Get(poi)->m_nCustomers;
                int tries = 0;
                for (;;) {
                    ++tries;
                    if (accomplice) break;
                    accomplice = POI_CustomerGetID(poi, n ? g_EXRandClass.Rand32() % n : 0);
                    if (tries >= 20) break;
                }
                Actor *pAccomplice = Actor::GetPtrFromID(accomplice);
                if (pAccomplice) {
                    stage = 1;
                    sm.GetActorOwner().SetRotY((float)atan2(AU_SubV(pAccomplice->GetHeadPos(), sm.GetActorOwner().GetHeadPos()).x,
                                                            AU_SubV(pAccomplice->GetHeadPos(), sm.GetActorOwner().GetHeadPos()).z));
                    pAccomplice->SendMsg(MsgItem((MsgType)0x13, sm.GetActorOwner().m_RefID), 0.0f);
                    sm.GetActorOwner().SetAnimType(0x90000e2, 0, 1);
                    return 1;
                }
                POI_SetComData(poi, 3000, 0, 0, 0);
                sm.Return(1, 0, 0, 0);
                return 1;
            }
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            return 1;
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) {
                POI_SetComData(poi, 3000, 0, 0, 0);
                sm.Return(1, 0, 0, 0);
                return 1;
            }
            break;
        }
        return 1;
    case MSG_EXIT:
        return 1;
    case MSG_ENTER:
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        timer = 300.0f;
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// Message 0x13 (name unknown): the crime leader (data[0]) signals; POI com state 3000 ends the wait.
signed char STATE_CrimePrepareAccomplice(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &bSignalled = sm.GetIntRef(i);
    sm.GetActorOwner().m_Status = 6;
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (!bSignalled) sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        if (POI_GetComData(poi, 0, 0, 0) == 3000) {
            sm.Return(1, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_ENTER:
        bSignalled = 0;
        poi = sm.m_Msg.m_Data[0];
        return 1;
    case MSG_EXIT:
        return 1;
    AU_DEFAULT_MSGS
    case 0x13: {
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x13)) return 1;
        Actor *pLeader = Actor::GetPtrFromID(sm.m_Msg.m_Data[0]);
        if (pLeader) {
            bSignalled = 1;
            sm.GetActorOwner().SetRotY((float)atan2(AU_SubV(pLeader->GetHeadPos(), sm.GetActorOwner().GetHeadPos()).x,
                                                    AU_SubV(pLeader->GetHeadPos(), sm.GetActorOwner().GetHeadPos()).z));
            sm.GetActorOwner().SetAnimType(0x90000e1, 0, 1);
        }
        return 1;
    }
    }
    return 0;
}

// NEAR: written for behaviour from the disassembly (not byte-matched); STATE_GotoLocation's flags applied
// to a plain position (data[0..2] = x, y, z bits, data[3] = flags). Message names 0x14/0x15/0x28/0x29 unknown.
signed char STATE_GotoPosition(StateMachine &sm)
{
    int i = 0;
    EXVector &pos = sm.GetVectorRef(i);
    int &bound = sm.GetIntRef(i);
    int &flags = sm.GetIntRef(i);
    if (flags & 0x800) sm.GetActorOwner().SetWatch(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_SUSPEND:
        if (flags & 0x20000) sm.GetActorOwner().SetCarry(0, 0);
        return 1;
    case MSG_ENTER:
        bound = 0;
        pos.x = (float &)sm.m_Msg.m_Data[0];
        pos.y = (float &)sm.m_Msg.m_Data[1];
        pos.z = (float &)sm.m_Msg.m_Data[2];
        flags = sm.m_Msg.m_Data[3];
        sm.GetActorOwner().GotoPosSlow(pos);
        sm.GetActorOwner().SetAnimType((flags & 0x200) ? 0x4e000003 : 0x4e000002, 0, 1);
        return 1;
    case MSG_EXT:
        if (!bound && kArriveSqDist > sm.GetActorOwner().SqDistToTarget()) {
            sm.Return(1, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_EXIT:
        return 1;
    AU_DEFAULT_MSGS
    case 0x28:
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x28)) return 1;
        if (!(flags & 0x100) && sm.m_Msg.m_Data[0]) {
            pos = POI_Get(sm.m_Msg.m_Data[0])->m_Pos;
            flags = 0x1110;
            sm.GetActorOwner().GotoPOI(sm.m_Msg.m_Data[0]);
            sm.m_Unk9c = sm.GetActorOwner().m_RefID;
            return 1;
        }
        sm.m_Unk9c = 0;
        return 1;
    case 0x29:
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x29)) return 1;
        Actor::SendMsg(MsgItem(MSG_SOMEONE_UNDER_ATTACK), sm.GetActorOwner().GetItem()->m_position, 10.0f,
                       Actor::GetPtrFromID(sm.m_Msg.m_Data[0]), 0.0f);
        SM_GOSUB(STATE_DeathBullet, 0, 0, 0, 0);
        return 1;
    case 0x14:
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x14)) return 1;
        if (!(flags & 0x100) && !(sm.GetActorOwner().m_Attrib & 0x10)) {
            signed char bPick = 0;
            int type = ActorCarryable::GetPtrFromID(sm.m_Msg.m_Data[0]) ? (int)ActorCarryable::GetPtrFromID(sm.m_Msg.m_Data[0])->m_Type : 0;
            switch (type) {
            case 1:
            case 3:
            case 4:
            case 5:
            case 6:
            case 9:
                if (sm.GetActorOwner().GetProfile()->m_Group == 0x4b000009) bPick = 1;
                if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f000001) bPick = 1;
                break;
            case 2:
            case 7:
                if (sm.GetActorOwner().GetProfile()->m_SubGroup == 0x4f000001) bPick = 1;
                break;
            case 8:
                bPick = 1;
                break;
            }
            if (bPick) {
                sm.m_Unk9c = 1;
                SM_GOSUB(STATE_PickUpDroppedItem, sm.m_Msg.m_Data[0], 0, 0, 0);
                return 1;
            }
        }
        sm.m_Unk9c = 0;
        return 1;
    case 0x15:
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x15)) return 1;
        sm.Return(0, 0, 0, 0);
        return 1;
    }
    return 0;
}

// NEAR: written for behaviour from the disassembly (not byte-matched). Walks to the player's last nav
// position (clamped to the actor's own reachable mesh), looks around, then walks home.
signed char STATE_WalkAttractedToPlayer(StateMachine &sm)
{
    int i = 0;
    EXVector &target = sm.GetVectorRef(i);
    EXVector &home = sm.GetVectorRef(i);
    float &rotY = sm.GetFloatRef(i);
    float &stopDist = sm.GetFloatRef(i);
    float &timer = sm.GetFloatRef(i);
    int &targetPoly = sm.GetIntRef(i);
    int &homePoly = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    int &walkAnim = sm.GetIntRef(i);
    float &voTimer = sm.GetFloatRef(i);
    int &bSaidVO = sm.GetIntRef(i);
    sm.GetActorOwner().m_Status = 5;
    sm.GetActorOwner().SetWatch(1);
    if (sm.m_Msg.m_Type == MSG_ATTRACT) {
        stage = 0;
        target = gAIControl.m_PlayerNavLoc.m_vFloorPos;
        targetPoly = gAIControl.m_PlayerNavLoc.m_nPolyHandle;
        timer = (g_EXRandClass.Randf() * 4.0f + 8.0f) * 60.0f;
        bSaidVO = 0;
        voTimer = g_EXRandClass.Randf() * 120.0f + 60.0f;
    }
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (!bSaidVO) {
            voTimer -= XApp::GameFrameRateMul();
            if (0.0f > voTimer) {
                sm.GetActorOwner().SayLuredVO();
                bSaidVO = 1;
            }
        }
        switch (stage) {
        case 0:
            if (sm.GetActorOwner().CanGetToPosPoly(target, targetPoly)) {
                EXVector d = AU_SubV(sm.GetActorOwner().GetItem()->m_position, target);
                if (!(stopDist * stopDist > d.LengthXYZSquared())) {
                    timer -= XApp::GameFrameRateMul();
                    if (!(0.0f >= timer)) {
                        sm.GetActorOwner().GotoPosPoly(target, targetPoly);
                        sm.GetActorOwner().SetAnimType(walkAnim, 0, 1);
                        return 1;
                    }
                }
            }
            {
                sm.GetActorOwner().InvalidateGotoPos();
                stage = 3;
                ActorCarryable *pCarry = sm.GetActorOwner().GetCarryPtr(0);
                WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
                if (pWeapon && pCarry && pCarry->m_bGun && pWeapon->m_AimType != 0x52000004)
                    sm.GetActorOwner().SetAnimType(0x9000166, 0, 1);
                else
                    sm.GetActorOwner().SetAnimType(0x9000065, 0, 1);
            }
            return 1;
        case 1:
        case 2:
            break;
        case 3:
            if (sm.GetActorOwner().HasAnimEnded()) {
                stage = 4;
                return 1;
            }
            break;
        case 4:
            sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
            sm.GetActorOwner().GotoPosPoly(home, homePoly);
            if (kArriveSqDist > sm.GetActorOwner().SqDistToTarget()) {
                stage = 5;
                return 1;
            }
            break;
        case 5:
            if (sm.GetActorOwner().IsFacingYRot(rotY, 5.0f)) {
                sm.Return(1, 0, 0, 0);
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(rotY, 5.0f);
            return 1;
        }
        return 1;
    case MSG_ENTER: {
        WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
        if (pWeapon && (pWeapon->m_AimType == 0x52000006 || pWeapon->m_AimType == 0x52000005)) {
            walkAnim = 0x9000160;
        } else {
            int anims[2];
            anims[0] = 0x900013f;
            anims[1] = 0x9000140;
            walkAnim = anims[g_EXRandClass.Rand32() & 1];
        }
        home = sm.GetActorOwner().GetItem()->m_position;
        homePoly = sm.GetActorOwner().m_OrgLoc.GetPolyHandle();
        rotY = sm.GetActorOwner().GetRotY();
        stopDist = 1.5f;
        timer = (g_EXRandClass.Randf() * 2.0f + 6.0f) * 60.0f;
        stage = 0;
        target = gAIControl.m_PlayerNavLoc.m_vFloorPos;
        targetPoly = gAIControl.m_PlayerNavLoc.m_nPolyHandle;
        if (!sm.GetActorOwner().CanReachZone(gAIControl.m_PlayerNavLoc.m_sNavZone)) {
            XNavLocation loc;
            loc = sm.GetActorOwner().m_OrgLoc;
            loc.MoveTo(target, 0);
            target = loc.m_vFloorPos;
            targetPoly = loc.m_nPolyHandle;
        }
        bSaidVO = 0;
        voTimer = g_EXRandClass.Randf() * 120.0f + 60.0f;
        return 1;
    }
    case MSG_EXIT:
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}


// NEAR: 32 bytes; the two random-anim SetAnimType calls load the actor before the table read (retail after).
signed char STATE_PanicStand(StateMachine &sm)
{
    int i = 0;
    int &bCower = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    int &count = sm.GetIntRef(i);
    int &maxCount = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().m_Status = 3;
    sm.GetActorOwner().SetWatch(4);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        timer -= XApp::GameFrameRateMul();
        switch (stage) {
        case 0:
            if (sm.GetActorOwner().HasAnimEnded()) count++;
            if (0.0f > timer && count == maxCount) {
                if (bCower) {
                    sm.GetActorOwner().SetAnimType(0x90003b6, 0, 1);
                    stage = 1;
                    return 1;
                }
                sm.Return(0, 0, 0, 0);
                return 1;
            }
            if (bCower && sm.GetActorOwner().HasAnimEnded()) sm.GetActorOwner().SetAnimType(0x9000155, 0, 1);
            break;
        case 1:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.Return(0, 0, 0, 0);
                return 1;
            }
            break;
        }
        return 1;
    case MSG_ENTER: {
        stage = 0;
        count = 0;
        maxCount = g_EXRandClass.Rand32() % 3 + 3;
        timer = (g_EXRandClass.Randf() * 2.0f + 6.0f) * EXBaseApp::NominalGameFrameRate();
        SubGroupAnims &anims = SubGroupAnims::GetSubGroupAnims(sm.GetActorOwner().m_SubGroupAnimIndex);
        WeaponProfile *pWeapon = sm.GetActorOwner().GetWeaponProfile(0);
        int aim = 0;
        if (pWeapon) aim = pWeapon->m_AimType;
        int panicAnim = anims.m_Anims[5];
        int bCowerAnims = 0;
        if (!panicAnim || panicAnim == 0x9000149) bCowerAnims = 1;
        if (aim == 0x52000004 || aim == 0x52000006 || aim == 0x52000005) {
            int a[2];
            a[0] = 0x9000305;
            a[1] = 0x9000166;
            int anim = a[g_EXRandClass.Rand32() & 1];
            sm.GetActorOwner().SetAnimType(anim, 0, 1);
            return 1;
        }
        bCower = 0;
        if (bCowerAnims) {
            int a[2];
            a[0] = 0x9000018;
            a[1] = 0x9000149;
            int anim = a[g_EXRandClass.Rand32() & 1];
            if (anim == 0x9000018) bCower = 1;
            sm.GetActorOwner().SetAnimType(anim, 0, 1);
            return 1;
        }
        int a[4];
        a[0] = 0x9000108;
        a[1] = 0x9000108;
        a[2] = 0x9000108;
        a[3] = 0x9000065;
        int anim = a[g_EXRandClass.Rand32() & 3];
        sm.GetActorOwner().SetAnimType(anim, 0, 1);
        return 1;
    }
    AU_DEFAULT_MSGS
    }
    return 0;
}

// NEAR: 2 bytes; retail sends an out-of-range stage to the shared final return-1, ours to the post-switch one.
signed char STATE_WalkToLocationCarry(StateMachine &sm)
{
    int i = 0;
    EXVector &pos = sm.GetVectorRef(i);
    int &loc = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    sm.GetActorOwner().m_Status = 0xb;
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        switch (stage) {
        case 0:
            stage = 1;
            if (sm.GetActorOwner().GetProfile()->m_Weapon)
                sm.GetActorOwner().SetCarry(ActorCarryable::Create(0x100003c, sm.GetActorOwner().GetProfile()->m_Weapon)->GetRefID(), 0);
            SM_GOSUB(STATE_GotoLocationNearestType, loc, 0x20110, 0, 0);
            break;
        case 1:
            stage = 0;
            if (sm.GetActorOwner().GetCarryPtr(0)) sm.GetActorOwner().GetCarryPtr(0)->Delete();
            sm.GetActorOwner().SetCarry(0, 0);
            SM_GOSUB(STATE_GotoPosition, *(int *)&pos.x, *(int *)&pos.y, *(int *)&pos.z, 0x100);
            return 1;
        }
        return 1;
    case MSG_RETURN:
        if (!sm.m_Msg.m_Data[0]) {
            stage = 1;
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        }
        return 1;
    case MSG_ENTER:
        stage = 0;
        pos = sm.GetActorOwner().GetFeetPos();
        loc = sm.m_Msg.m_Data[0];
        if (sm.GetActorOwner().GetCarryPtr(0)) sm.GetActorOwner().GetCarryPtr(0)->Delete();
        sm.GetActorOwner().SetCarry(0, 0);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocation1, STATE_WalkToLocation, 0x42000086)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocation2, STATE_WalkToLocation, 0x42000087)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocation3, STATE_WalkToLocation, 0x42000088)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocation4, STATE_WalkToLocation, 0x42000089)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocation5, STATE_WalkToLocation, 0x4200008a)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocation6, STATE_WalkToLocation, 0x4200008b)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocation7, STATE_WalkToLocation, 0x4200008c)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocation8, STATE_WalkToLocation, 0x4200008d)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocation9, STATE_WalkToLocation, 0x4200008e)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocation10, STATE_WalkToLocation, 0x4200008f)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocation11, STATE_WalkToLocation, 0x42000090)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocation12, STATE_WalkToLocation, 0x42000091)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocation13, STATE_WalkToLocation, 0x42000092)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocation14, STATE_WalkToLocation, 0x42000093)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocation15, STATE_WalkToLocation, 0x42000094)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocation16, STATE_WalkToLocation, 0x42000095)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocation17, STATE_WalkToLocation, 0x42000096)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocation18, STATE_WalkToLocation, 0x42000097)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocation19, STATE_WalkToLocation, 0x42000098)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocation20, STATE_WalkToLocation, 0x42000099)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationIgnore1, STATE_WalkToLocationIgnore, 0x42000086)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationIgnore2, STATE_WalkToLocationIgnore, 0x42000087)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationIgnore3, STATE_WalkToLocationIgnore, 0x42000088)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationIgnore4, STATE_WalkToLocationIgnore, 0x42000089)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationIgnore5, STATE_WalkToLocationIgnore, 0x4200008a)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationIgnore6, STATE_WalkToLocationIgnore, 0x4200008b)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationIgnore7, STATE_WalkToLocationIgnore, 0x4200008c)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationIgnore8, STATE_WalkToLocationIgnore, 0x4200008d)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationIgnore9, STATE_WalkToLocationIgnore, 0x4200008e)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationIgnore10, STATE_WalkToLocationIgnore, 0x4200008f)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationIgnore11, STATE_WalkToLocationIgnore, 0x42000090)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationIgnore12, STATE_WalkToLocationIgnore, 0x42000091)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationIgnore13, STATE_WalkToLocationIgnore, 0x42000092)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationIgnore14, STATE_WalkToLocationIgnore, 0x42000093)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationIgnore15, STATE_WalkToLocationIgnore, 0x42000094)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationIgnore16, STATE_WalkToLocationIgnore, 0x42000095)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationIgnore17, STATE_WalkToLocationIgnore, 0x42000096)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationIgnore18, STATE_WalkToLocationIgnore, 0x42000097)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationIgnore19, STATE_WalkToLocationIgnore, 0x42000098)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationIgnore20, STATE_WalkToLocationIgnore, 0x42000099)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocation1, STATE_RunToLocation, 0x42000086)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocation2, STATE_RunToLocation, 0x42000087)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocation3, STATE_RunToLocation, 0x42000088)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocation4, STATE_RunToLocation, 0x42000089)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocation5, STATE_RunToLocation, 0x4200008a)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocation6, STATE_RunToLocation, 0x4200008b)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocation7, STATE_RunToLocation, 0x4200008c)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocation8, STATE_RunToLocation, 0x4200008d)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocation9, STATE_RunToLocation, 0x4200008e)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocation10, STATE_RunToLocation, 0x4200008f)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocation11, STATE_RunToLocation, 0x42000090)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocation12, STATE_RunToLocation, 0x42000091)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocation13, STATE_RunToLocation, 0x42000092)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocation14, STATE_RunToLocation, 0x42000093)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocation15, STATE_RunToLocation, 0x42000094)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocation16, STATE_RunToLocation, 0x42000095)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocation17, STATE_RunToLocation, 0x42000096)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocation18, STATE_RunToLocation, 0x42000097)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocation19, STATE_RunToLocation, 0x42000098)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocation20, STATE_RunToLocation, 0x42000099)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocationIgnore1, STATE_RunToLocationIgnore, 0x42000086)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocationIgnore2, STATE_RunToLocationIgnore, 0x42000087)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocationIgnore3, STATE_RunToLocationIgnore, 0x42000088)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocationIgnore4, STATE_RunToLocationIgnore, 0x42000089)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocationIgnore5, STATE_RunToLocationIgnore, 0x4200008a)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocationIgnore6, STATE_RunToLocationIgnore, 0x4200008b)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocationIgnore7, STATE_RunToLocationIgnore, 0x4200008c)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocationIgnore8, STATE_RunToLocationIgnore, 0x4200008d)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocationIgnore9, STATE_RunToLocationIgnore, 0x4200008e)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocationIgnore10, STATE_RunToLocationIgnore, 0x4200008f)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocationIgnore11, STATE_RunToLocationIgnore, 0x42000090)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocationIgnore12, STATE_RunToLocationIgnore, 0x42000091)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocationIgnore13, STATE_RunToLocationIgnore, 0x42000092)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocationIgnore14, STATE_RunToLocationIgnore, 0x42000093)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocationIgnore15, STATE_RunToLocationIgnore, 0x42000094)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocationIgnore16, STATE_RunToLocationIgnore, 0x42000095)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocationIgnore17, STATE_RunToLocationIgnore, 0x42000096)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocationIgnore18, STATE_RunToLocationIgnore, 0x42000097)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocationIgnore19, STATE_RunToLocationIgnore, 0x42000098)
AU_STATE_ENTER_GOTO_STD(STATE_RunToLocationIgnore20, STATE_RunToLocationIgnore, 0x42000099)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationCarry1, STATE_WalkToLocationCarry, 0x42000086)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationCarry2, STATE_WalkToLocationCarry, 0x42000087)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationCarry3, STATE_WalkToLocationCarry, 0x42000088)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationCarry4, STATE_WalkToLocationCarry, 0x42000089)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationCarry5, STATE_WalkToLocationCarry, 0x4200008a)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationCarry6, STATE_WalkToLocationCarry, 0x4200008b)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationCarry7, STATE_WalkToLocationCarry, 0x4200008c)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationCarry8, STATE_WalkToLocationCarry, 0x4200008d)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationCarry9, STATE_WalkToLocationCarry, 0x4200008e)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationCarry10, STATE_WalkToLocationCarry, 0x4200008f)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationCarry11, STATE_WalkToLocationCarry, 0x42000090)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationCarry12, STATE_WalkToLocationCarry, 0x42000091)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationCarry13, STATE_WalkToLocationCarry, 0x42000092)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationCarry14, STATE_WalkToLocationCarry, 0x42000093)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationCarry15, STATE_WalkToLocationCarry, 0x42000094)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationCarry16, STATE_WalkToLocationCarry, 0x42000095)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationCarry17, STATE_WalkToLocationCarry, 0x42000096)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationCarry18, STATE_WalkToLocationCarry, 0x42000097)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationCarry19, STATE_WalkToLocationCarry, 0x42000098)
AU_STATE_ENTER_GOTO_STD(STATE_WalkToLocationCarry20, STATE_WalkToLocationCarry, 0x42000099)

signed char STATE_Police_ApprehendActor(StateMachine &sm)
{
    int i = 0;
    int &targetId = sm.GetIntRef(i);
    int &stage = sm.GetIntRef(i);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().m_Status = 0xa;
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT: {
        Actor *pTarget = Actor::GetPtrFromID(targetId);
        if (!pTarget) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
        sm.GetActorOwner().GotoLoc(pTarget->OrgLoc());
        if (!stage && sm.GetActorOwner().SqDistToTarget() < 25.0f) {
            stage = 1;
            pTarget->SendMsg(MsgItem(MSG_FIGHT_ME, sm.GetActorOwner().GetRefID()), 0.0f);
            SM_GOSUB(STATE_FightActor, pTarget->GetRefID(), 0, 0, 0);
        }
        return 1;
    }
    case MSG_ENTER:
        targetId = sm.m_Msg.m_Data[0];
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// NEAR: written for behaviour from the disassembly (not byte-matched). Spawns actor profile `hash` 4 m in
// front of the player and parks it in STATE_StandStill.
void AU_SpawnDebugActor(int hash)
{
    EXMaths::mat_44 m;
    m.set_rotate_y(XItemHandler_Player::Instance()->GetItem()->m_rotation.y);
    EXMaths::vec_xyzw v;
    v.set(0.0f, 0.0f, 4.0f, 0.0f);
    EXMaths::vec_xyzw off = m * v;
    EXVector pos = XItemHandler_Player::Instance()->GetItem()->m_position + EXVector(off);
    Actor *pActor = Actor::Create(hash, pos, 0, 0.0f, 0, 2, 0);
    if (pActor) pActor->m_SM.ExtGosub(0, STATE_StandStill, 0, 0, 0, 0);
}

signed char AU_EnsureAtTarget(StateMachine &sm, EXVector pos, float dist)
{
    EXVector d = AU_SubV(pos, sm.GetActorOwner().GetFeetPos());
    if (d.LengthXZ() > dist) SM_GOSUB(STATE_GotoPosition, (int &)pos.x, (int &)pos.y, (int &)pos.z, 0x100);
    return 1;
}

signed char STATE_Mosh(StateMachine &sm)
{
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        sm.GetActorOwner().SetAnimType(0x90004bf, 0, 1);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_PanicDropWeapon(StateMachine &sm)
{
    sm.GetActorOwner().SetStatus(3);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        if (sm.GetActorOwner().HasAnimEnded()) {
            SM_GOTO(STATE_PanicExtreme, 0, 0, 0, 0);
            return 1;
        }
        return 1;
    case MSG_ENTER:
        sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR))
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR))
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    }
    return 0;
}

signed char STATE_Panic(StateMachine &sm)
{
    sm.GetActorOwner().SetStatus(3);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        SM_GOTO(STATE_PanicExtreme, 0, 0, 0, 0);
        return 1;
    }
    return 0;
}

signed char STATE_FS_PanicStand(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    sm.GetActorOwner().m_Status = 3;
    sm.GetActorOwner().SetWatch(7);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
        sm.Return(0, 0, 0, 0);
        return 1;
    case MSG_ENTER:
        poi = sm.m_Msg.m_Data[0];
        SM_GOSUB(STATE_PanicStand, 0, 0, 0, 0);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// NEAR: written for behaviour from the disassembly (not byte-matched); STATE_PlayStartAnim's walk-home logic
// with a plain idle, and city actors (creation flag 0x20) hand over to STATE_CityDistractions.
signed char STATE_DefaultIdle(StateMachine &sm)
{
    int i = 0;
    EXVector &home = sm.GetVectorRef(i);
    int &homePoly = sm.GetIntRef(i);
    float &rotY = sm.GetFloatRef(i);
    float &timer = sm.GetFloatRef(i);
    int &stage = sm.GetIntRef(i);
    sm.GetActorOwner().m_Status = 0xb;
    sm.GetActorOwner().SetHeadTrackMode(1);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN: {
        if (sm.GetActorOwner().m_CreationFlags & 0x20) {
            SM_GOTO(STATE_CityDistractions, 0, 0, 0, 0);
            return 1;
        }
        EXVector d = AU_SubV(sm.GetActorOwner().GetItem()->m_position, home);
        if (d.LengthXYZSquared() > 1.0f) {
            stage = 1;
            timer = EXBaseApp::NominalGameFrameRate() * 5.0f;
        } else {
            stage = 2;
        }
        return 1;
    }
    case MSG_ENTER:
        stage = 0;
        if (sm.GetActorOwner().m_CreationFlags & 0x20) {
            SM_GOTO(STATE_CityDistractions, 0, 0, 0, 0);
            return 1;
        }
        home = sm.GetActorOwner().GetItem()->m_position;
        homePoly = sm.GetActorOwner().m_OrgLoc.GetPolyHandle();
        rotY = sm.GetActorOwner().GetRotY();
        return 1;
    case MSG_EXT:
        switch (stage) {
        case 0:
            sm.GetActorOwner().InvalidateGotoPos();
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            return 1;
        case 1:
            sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
            sm.GetActorOwner().GotoPosPoly(home, homePoly);
            if (6.25f > sm.GetActorOwner().SqDistToTarget()) {
                timer -= XApp::GameFrameRateMul();
                if (0.0f >= timer || kArriveSqDist > sm.GetActorOwner().SqDistToTarget()) stage = 2;
                return 1;
            }
            timer = EXBaseApp::NominalGameFrameRate() * 5.0f;
            return 1;
        case 2:
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(rotY, 5.0f)) stage = 0;
            else sm.GetActorOwner().SetTargetYRot(rotY, 5.0f);
            return 1;
        }
        return 1;
    case MSG_EXIT:
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// NEAR: written for behaviour from the disassembly (not byte-matched); STATE_DefaultIdle plus a Gosub to
// STATE_AttackPred once the player has been seen.
signed char STATE_AttackPlayerOnSight(StateMachine &sm)
{
    int i = 0;
    EXVector &home = sm.GetVectorRef(i);
    int &homePoly = sm.GetIntRef(i);
    float &rotY = sm.GetFloatRef(i);
    float &timer = sm.GetFloatRef(i);
    int &stage = sm.GetIntRef(i);
    sm.GetActorOwner().m_Status = 0xb;
    sm.GetActorOwner().SetHeadTrackMode(1);
    sm.GetActorOwner().SetWatch(4);
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN: {
        if (sm.GetActorOwner().m_CreationFlags & 0x20) {
            SM_GOTO(STATE_CityDistractions, 0, 0, 0, 0);
            return 1;
        }
        EXVector d = AU_SubV(sm.GetActorOwner().GetItem()->m_position, home);
        if (d.LengthXYZSquared() > 1.0f) {
            stage = 1;
            timer = EXBaseApp::NominalGameFrameRate() * 5.0f;
        } else {
            stage = 2;
        }
        return 1;
    }
    case MSG_ENTER:
        stage = 0;
        if (sm.GetActorOwner().m_CreationFlags & 0x20) {
            SM_GOTO(STATE_CityDistractions, 0, 0, 0, 0);
            return 1;
        }
        home = sm.GetActorOwner().GetItem()->m_position;
        homePoly = sm.GetActorOwner().m_OrgLoc.GetPolyHandle();
        rotY = sm.GetActorOwner().GetRotY();
        return 1;
    case MSG_EXT:
        if (sm.GetActorOwner().m_StateFlags.m_bPlayerSeen) {
            SM_GOSUB(STATE_AttackPred, 0, 0, 0, 0);
            return 1;
        }
        switch (stage) {
        case 0:
            sm.GetActorOwner().InvalidateGotoPos();
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            return 1;
        case 1:
            sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
            sm.GetActorOwner().GotoPosPoly(home, homePoly);
            if (6.25f > sm.GetActorOwner().SqDistToTarget()) {
                timer -= XApp::GameFrameRateMul();
                if (0.0f >= timer || kArriveSqDist > sm.GetActorOwner().SqDistToTarget()) stage = 2;
                return 1;
            }
            timer = EXBaseApp::NominalGameFrameRate() * 5.0f;
            return 1;
        case 2:
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(rotY, 5.0f)) stage = 0;
            else sm.GetActorOwner().SetTargetYRot(rotY, 5.0f);
            return 1;
        }
        return 1;
    case MSG_EXIT:
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}


// NEAR: written for behaviour from the disassembly (not byte-matched). Plays the profile's start anim;
// if a Gosub moved the actor it walks back to where it started and turns to its old heading.
signed char STATE_PlayStartAnim(StateMachine &sm)
{
    int i = 0;
    EXVector &home = sm.GetVectorRef(i);
    int &homePoly = sm.GetIntRef(i);
    float &rotY = sm.GetFloatRef(i);
    float &timer = sm.GetFloatRef(i);
    int &stage = sm.GetIntRef(i);
    sm.GetActorOwner().m_Status = 0xb;
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN: {
        EXVector d = AU_SubV(sm.GetActorOwner().GetItem()->m_position, home);
        if (d.LengthXYZSquared() > 1.0f) {
            stage = 1;
            timer = EXBaseApp::NominalGameFrameRate() * 5.0f;
        } else {
            stage = 2;
        }
        return 1;
    }
    case MSG_ENTER:
        stage = 0;
        home = sm.GetActorOwner().GetItem()->m_position;
        homePoly = sm.GetActorOwner().m_OrgLoc.GetPolyHandle();
        rotY = sm.GetActorOwner().GetRotY();
        return 1;
    case MSG_EXT:
        switch (stage) {
        case 0:
            sm.GetActorOwner().InvalidateGotoPos();
            sm.GetActorOwner().SetAnimType(AP_GetPtr(sm.GetActorOwner().GetProfileHash())->m_StartAnim, 0, 1);
            return 1;
        case 1:
            sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
            sm.GetActorOwner().GotoPosPoly(home, homePoly);
            if (6.25f > sm.GetActorOwner().SqDistToTarget()) {
                timer -= XApp::GameFrameRateMul();
                if (0.0f >= timer || kArriveSqDist > sm.GetActorOwner().SqDistToTarget()) stage = 2;
                return 1;
            }
            timer = EXBaseApp::NominalGameFrameRate() * 5.0f;
            return 1;
        case 2:
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(rotY, 5.0f)) stage = 0;
            else sm.GetActorOwner().SetTargetYRot(rotY, 5.0f);
            return 1;
        }
        return 1;
    case MSG_EXIT:
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// NEAR: written for behaviour from the disassembly (not byte-matched).
// GUESS: stage 3 makes an unused Rand32() call in retail (kept).
signed char STATE_SeenBody_Help(StateMachine &sm)
{
    int i = 0;
    EXVector &home = sm.GetVectorRef(i);
    int &stage = sm.GetIntRef(i);
    int &bAnimPlayed = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    sm.GetActorOwner().m_Status = 3;
    sm.GetActorOwner().SetWatch(1);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT: {
        Actor *pBody = Actor::GetPtrFromID(sm.GetActorOwner().SeenBodyID());
        if (!pBody) {
            AU_EnsureAtTarget(sm, home, 0.25f);
            stage = 2;
        }
        switch (stage) {
        case 0:
            sm.GetActorOwner().GotoLoc(pBody->m_OrgLoc);
            if (9.0f > sm.GetActorOwner().SqDistToTarget()) stage = 1;
            break;
        case 1:
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(pBody->GetItem()->m_position, 5.0f)) {
                timer -= XApp::GameFrameRateMul();
                if (bAnimPlayed && !sm.GetActorOwner().HasAnimEnded()) return 1;
                if (!(0.0f < timer)) stage = 3;
                bAnimPlayed = 1;
                int anims[4];
                anims[0] = 0x900030a;
                anims[1] = 0x9000309;
                anims[2] = 0x9000308;
                anims[3] = SubGroupAnims::GetSubGroupAnims(sm.GetActorOwner().m_SubGroupAnimIndex).m_Anims[4];
                sm.GetActorOwner().SetAnimType(anims[g_EXRandClass.Rand32() & 3], 0, 1);
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(pBody->GetItem()->m_position, 5.0f);
            return 1;
        case 2:
            sm.Return(1, 0, 0, 0);
            return 1;
        case 3:
            g_EXRandClass.Rand32();
            stage = 2;
            return 1;
        }
        return 1;
    }
    case MSG_ENTER: {
        Actor *pBody = Actor::GetPtrFromID(sm.GetActorOwner().SeenBodyID());
        if (sm.GetActorOwner().IsDrone() && sm.GetActorOwner().CanGetToLocation(pBody->m_OrgLoc)) {
            stage = 0;
            home = sm.GetActorOwner().GetItem()->m_position;
            timer = 300.0f;
            sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
            return 1;
        }
        sm.Return(0, 0, 0, 0);
        return 1;
    }
    case MSG_EXIT:
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}


// GUESS: names of messages 0x2b/0x2c (both start STATE_Police_ApprehendActor with data[0], data[1]).
// NEAR: retail lays the 0x2b check directly before the shared Gosub body and the 0x2c check after it.
signed char STATE_Police_Patrol(StateMachine &sm)
{
    int i = 0;
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    sm.GetActorOwner().m_Status = 0xa;
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        switch (stage) {
        case 0:
            stage = 1;
            break;
        case 1:
            stage = 2;
            poi = POI_GetRandomFlag(0x40000, poi, sm.GetActorOwner().GetNavZone(), sm.GetActorOwner().CanJump(), 0);
            if (!poi) poi = POI_GetRandomFlag(1, 0, sm.GetActorOwner().GetNavZone(), sm.GetActorOwner().CanJump(), 0);
            sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
            sm.GetActorOwner().GotoPOI(poi);
            break;
        case 2:
            if (sm.GetActorOwner().SqDistToTarget() < 100.0f) stage = 1;
            break;
        }
        return 1;
    case MSG_ENTER:
        stage = 0;
        return 1;
    AU_DEFAULT_MSGS
    case 0x2b:
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x2b)) return 1;
    apprehend:
        SM_GOSUB(STATE_Police_ApprehendActor, sm.m_Msg.m_Data[0], sm.m_Msg.m_Data[1], 0, 0);
        return 1;
    case 0x2c:
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x2c)) return 1;
        goto apprehend;
    }
    return 0;
}

// NEAR: written for behaviour from the disassembly; the offset maths (rotate_y of a (-0.7,0,0) vector) and the
// speed clamp are expressed through the file's usual helpers, so scheduling differs from retail.
// GUESS: the offset vector (-0.7, 0, 0, 0) is derived from the unrolled sin/cos code, not read from a call.
signed char STATE_Police_PatrolAccomplice(StateMachine &sm)
{
    int i = 0;
    int &targetId = sm.GetIntRef(i);
    sm.GetActorOwner().m_Status = 0xa;
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        targetId = sm.m_Msg.m_Data[0];
        return 1;
    case MSG_EXT: {
        Actor *pTarget = Actor::GetPtrFromID(targetId);
        if (!pTarget) {
            SM_GOTO(STATE_MissionEnd, 0, 0, 0, 0);
            return 1;
        }
        sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
        pTarget = Actor::GetPtrFromID(targetId);
        if (pTarget) {
            EXMaths::mat_44 m;
            m.set_rotate_y(pTarget->GetRotY());
            EXMaths::vec_xyzw v;
            v.set(-0.7f, 0.0f, 0.0f, 0.0f);
            EXMaths::vec_xyzw off = m * v;
            EXVector pos = pTarget->GetFeetPos() + EXVector(off);
            EXVector d = AU_SubV(pos, sm.GetActorOwner().GetFeetPos());
            float f = (d.x * d.x + d.y * d.y + d.z * d.z) * 0.25f + 1.0f;
            sm.GetActorOwner().m_AnimSpeedMult = (2.0f < f) ? 2.0f : f;
            sm.GetActorOwner().GotoPosSlow(pos);
        }
        return 1;
    }
    AU_DEFAULT_MSGS
    case 0x2b:
        if (sm.GetActorOwner().IsMsgOnStack((MsgType)0x2b)) return 1;
        SM_GOSUB(STATE_Police_ApprehendActor, sm.m_Msg.m_Data[0], sm.m_Msg.m_Data[1], 0, 0);
        return 1;
    }
    return 0;
}

// NEAR: written for behaviour from the disassembly; retail inlines the squared distance into the stage 0 test.
signed char STATE_FS_BangOnDoor(StateMachine &sm)
{
    int i = 0;
    EXVector &doorPos = sm.GetVectorRef(i);
    int &doorPoly = sm.GetIntRef(i);
    float &doorRot = sm.GetFloatRef(i);
    float &timer = sm.GetFloatRef(i);
    int &stage = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    sm.GetActorOwner().m_Status = 3;
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(0);
    int anims[2];
    anims[0] = 0x900043e;
    anims[1] = 0x900043f;
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT:
        switch (stage) {
        case 0: {
            EXVector d = AU_SubV(sm.GetActorOwner().GetFeetPos(), doorPos);
            if (d.x * d.x + d.y * d.y + d.z * d.z > 0.0625f) {
                SM_GOTO(STATE_PanicExtreme, 0, 0, 0, 0);
                return 1;
            }
            sm.GetActorOwner().InvalidateGotoPos();
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().Speak(sm.GetActorOwner().GetScreamSFX(), 0);
                Actor::SendMsg(MsgItem(MSG_LOUD_NOISE), sm.GetActorOwner().GetFeetPos(), 10.0f, &sm.GetActorOwner(), -1.0f);
                sm.GetActorOwner().SetAnimType(anims[g_EXRandClass.Rand32() & 1], 0, 1);
            }
            return 1;
        }
        case 1:
            sm.GetActorOwner().SetAnimType(0x4e00000d, 0, 1);
            sm.GetActorOwner().GotoPosPoly(doorPos, doorPoly);
            if (6.25f > sm.GetActorOwner().SqDistToTarget()) {
                timer -= XApp::GameFrameRateMul();
                if (0.0f >= timer || kArriveSqDist > sm.GetActorOwner().SqDistToTarget()) stage = 2;
                return 1;
            }
            timer = EXBaseApp::NominalGameFrameRate() * 5.0f;
            return 1;
        case 2:
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(doorRot, 5.0f)) {
                stage = 0;
                return 1;
            }
            sm.GetActorOwner().SetTargetYRot(doorRot, 5.0f);
            return 1;
        }
        return 1;
    case MSG_ENTER:
        stage = 0;
        poi = sm.m_Msg.m_Data[0];
        doorPos = sm.GetActorOwner().GetFeetPos();
        doorPoly = sm.GetActorOwner().m_OrgLoc.GetPolyHandle();
        doorRot = POI_Get(poi)->m_YRot;
        Actor::SendMsg(MsgItem(MSG_LOUD_NOISE), sm.GetActorOwner().GetFeetPos(), 10.0f, &sm.GetActorOwner(), -1.0f);
        sm.GetActorOwner().SetAnimType(anims[g_EXRandClass.Rand32() & 1], 0, 1);
        return 1;
    case MSG_EXIT:
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_EvadePlayer(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &bArrived = sm.GetIntRef(i);
    sm.GetActorOwner().m_Status = 0xd;
    sm.GetActorOwner().SetWatch(0x16);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT: {
        gAIControl.m_ActorsInCombatCount++;
        EXVector d = AU_SubV(sm.GetActorOwner().GetFeetPos(), Player_GetFeetPos());
        if (bArrived && 25.0f > d.x * d.x + d.y * d.y + d.z * d.z) {
            SM_GOTO(STATE_AttackPred, 0, 0, 0, 0);
            return 1;
        }
        if (!poi) {
            poi = AU_FindRunAwayPoint(sm, 0x2000, 0, sm.GetActorOwner().GetNavZone(), 1);
            if (poi) {
                POI_VendorBind(poi, sm.GetActorOwner().GetRefID());
                sm.GetActorOwner().GotoPOI(poi);
                sm.GetActorOwner().SetAnimType(0x4e000003, 0, 1);
            }
        }
        if (poi) {
            if (kArriveSqDist > sm.GetActorOwner().SqDistToTarget()) {
                bArrived = 1;
                sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
                if (sm.GetActorOwner().IsFacingYRot(Player_GetFeetPos(), 5.0f)) {
                    sm.GetActorOwner().InvalidateGotoPos();
                    sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                } else {
                    sm.GetActorOwner().SetTargetYRot(Player_GetFeetPos(), 5.0f);
                }
                EXVector d2 = AU_SubV(sm.GetActorOwner().GetFeetPos(), Player_GetFeetPos());
                if (100.0f > d2.x * d2.x + d2.y * d2.y + d2.z * d2.z) {
                    POI_VendorUnbind(poi, sm.GetActorOwner().GetRefID());
                    POI_ForceFlagChannel(poi, sm.GetActorOwner().GetRefID());
                    poi = 0;
                }
            }
        } else {
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(Player_GetFeetPos(), 5.0f)) {
                sm.GetActorOwner().InvalidateGotoPos();
                sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            } else {
                sm.GetActorOwner().SetTargetYRot(Player_GetFeetPos(), 5.0f);
            }
        }
        return 1;
    }
    case MSG_EXIT:
        POI_VendorUnbind(poi, sm.GetActorOwner().GetRefID());
        poi = 0;
        return 1;
    case MSG_ENTER:
        poi = 0;
        bArrived = 0;
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

signed char STATE_BOSS_RatPredEvadePlayer(StateMachine &sm)
{
    int i = 0;
    int &poi = sm.GetIntRef(i);
    int &bArrived = sm.GetIntRef(i);
    sm.GetActorOwner().m_Status = 0xd;
    sm.GetActorOwner().SetWatch(0x16);
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT: {
        EXVector d = AU_SubV(sm.GetActorOwner().GetFeetPos(), Player_GetFeetPos());
        if (bArrived && 9.0f > d.x * d.x + d.y * d.y + d.z * d.z) {
            SM_GOTO(STATE_AttackPred, 0, 0, 0, 0);
            return 1;
        }
        if (!poi) {
            poi = AU_FindRunAwayPoint(sm, 0x2000, 0, sm.GetActorOwner().GetNavZone(), 1);
            if (poi) {
                POI_VendorBind(poi, sm.GetActorOwner().GetRefID());
                sm.GetActorOwner().GotoPOI(poi);
                sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
            }
        }
        if (poi) {
            if (4.0f > sm.GetActorOwner().SqDistToTarget()) {
                bArrived = 1;
                sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
                if (sm.GetActorOwner().IsFacingYRot(Player_GetFeetPos(), 5.0f)) {
                    sm.GetActorOwner().InvalidateGotoPos();
                    sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
                } else {
                    sm.GetActorOwner().SetTargetYRot(Player_GetFeetPos(), 5.0f);
                }
                EXVector d2 = AU_SubV(sm.GetActorOwner().GetFeetPos(), Player_GetFeetPos());
                if (100.0f > d2.x * d2.x + d2.y * d2.y + d2.z * d2.z) {
                    POI_VendorUnbind(poi, sm.GetActorOwner().GetRefID());
                    poi = 0;
                }
            }
        } else {
            sm.GetActorOwner().InvalidateGotoPosButNotTargetYRot();
            if (sm.GetActorOwner().IsFacingYRot(Player_GetFeetPos(), 5.0f)) {
                sm.GetActorOwner().InvalidateGotoPos();
                sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            } else {
                sm.GetActorOwner().SetTargetYRot(Player_GetFeetPos(), 5.0f);
            }
        }
        return 1;
    }
    case MSG_EXIT:
        POI_VendorUnbind(poi, sm.GetActorOwner().GetRefID());
        poi = 0;
        sm.GetActorOwner().SetCloak(0);
        return 1;
    case MSG_ENTER:
        poi = 0;
        bArrived = 0;
        sm.GetActorOwner().SetCloak(1);
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// NEAR: written for behaviour from the disassembly; retail evaluates each facing atan2 with both operand
// subtracts inline (double evaluation, as the sibling states), scheduling differs.
// GUESS: local [0] is the actor id of the customer ahead in the queue (POI_CustomerGetID), local [3] is the
// "vendor not required" flag from the second Gosub argument.
signed char STATE_CustomerQueue(StateMachine &sm)
{
    int i = 0;
    int &aheadId = sm.GetIntRef(i);
    int &poi = sm.GetIntRef(i);
    float &timer = sm.GetFloatRef(i);
    int &bNoVendor = sm.GetIntRef(i);
    sm.GetActorOwner().m_Status = 0xb;
    switch (sm.m_Msg.m_Type) {
    case MSG_EXT: {
        if (!bNoVendor && !POI_VendorIsThere(poi)) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        int idx = POI_CustomerStandIndex(poi, sm.GetActorOwner().GetRefID());
        sm.GetActorOwner().GotoCustomerPOI(poi, -1);
        if (kArriveSqDist > sm.GetActorOwner().SqDistToTarget()) {
            timer = (EXRandf() * 0.8f + 0.2f) * (float)EXBaseApp::NominalGameFrameRate();
            sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);
            if (idx == 0) {
                sm.GetActorOwner().SetRotY((float)atan2(AU_SubV(POI_Get(poi)->m_Pos, sm.GetActorOwner().GetHeadPos()).x,
                                                        AU_SubV(POI_Get(poi)->m_Pos, sm.GetActorOwner().GetHeadPos()).z));
                sm.Return(1, idx, idx, idx);
                return 1;
            }
            sm.GetActorOwner().SetRotY(
                (float)atan2(AU_SubV(POI_CustomerStandPos(poi, idx - 1), sm.GetActorOwner().GetHeadPos()).x,
                             AU_SubV(POI_CustomerStandPos(poi, idx - 1), sm.GetActorOwner().GetHeadPos()).z));
            return 1;
        }
        Actor *pAhead = Actor::GetPtrFromID(aheadId);
        if (pAhead) {
            EXVector d = AU_SubV(POI_CustomerStandPos(poi, idx), pAhead->GetFeetPos());
            if (!(d.x * d.x + d.y * d.y + d.z * d.z > kArriveSqDist)) return 1;
            timer -= XGetFrameRateMul();
            if (0.0f <= timer) return 1;
        }
        sm.GetActorOwner().SetAnimType(0x4e000002, 0, 1);
        return 1;
    }
    case MSG_ENTER: {
        poi = sm.m_Msg.m_Data[0];
        bNoVendor = sm.m_Msg.m_Data[1];
        int idx = POI_CustomerStandIndex(poi, sm.GetActorOwner().GetRefID());
        if (idx == -1) {
            sm.Return(0, 0, 0, 0);
            return 1;
        }
        aheadId = POI_CustomerGetID(poi, idx - 1);
        return 1;
    }
    case MSG_EXIT:
        return 1;
    AU_DEFAULT_MSGS
    }
    return 0;
}

// A spawn point near from (within 20 units) that the camera can see is moved 5 units behind from,
// relative to the camera's facing, so actors do not appear in view.
EXVector AU_EnsurePointOutOfView(EXVector &pos, EXVector &from)
{
    EXVector out = pos;
    EXVector d = pos - from;
    if (d.z * d.z + d.x * d.x < 400.0f) {
        EXSphere s;
        s.Set(pos, 0.0f);
        if (XGetCamera(0)->BoundingVolumeOnScreen(s)) {
            EXMatrix m;
            m.set_rotate_y(XGetCamera(0)->Angle());
            EXVector back(0.0f, 0.0f, -5.0f, 0.0f);
            EXVector v;
            D3DXVec4Transform(&v, &back, &m);
            out = from + v;
        }
    }
    return out;
}


// Boss hit reactions by subgroup for one hit direction (retail inlines this once per quadrant).
static __forceinline int AU_BossHitAnim(Actor &a, int defaultAnim, int roninFirst, signed char bSavageRandom, int savageAnim)
{
    switch (a.GetProfile()->m_SubGroup) {
    case 0x4f00000d: {  // HT_ActorSubGroup_MafiaEnforcer
        int anims[4] = {0x9000524, 0x9000525, 0x9000526, 0x9000527};
        return anims[g_EXRandClass.Rand32() & 3];
    }
    case 0x4f00000e:  // HT_ActorSubGroup_ServiteurEnforcer
    case 0x4f000013: {  // HT_ActorSubGroup_Bruno
        int anims[4] = {0x90004c8, 0x90004c6, 0x90004c7, 0x90004c5};
        return anims[g_EXRandClass.Rand32() & 3];
    }
    case 0x4f000010: {  // HT_ActorSubGroup_RussianMercs
        int anims[4] = {0x90004cc, 0x90004cd, 0x90004ce, 0x90004cf};
        return anims[g_EXRandClass.Rand32() & 3];
    }
    case 0x4f000011: {  // HT_ActorSubGroup_Ronin
        int anims[3] = {roninFirst, 0x9000510, 0x9000511};
        return anims[g_EXRandClass.Rand32() % 3];
    }
    case 0x4f000016:  // HT_ActorSubGroup_SavagePred
    case 0x4f000017: {  // HT_ActorSubGroup_POWPred
        if (!bSavageRandom) return savageAnim;
        int anims[3] = {0x900040e, 0x900040d, 0x900040c};
        return anims[g_EXRandClass.Rand32() % 3];
    }
    default:
        return defaultAnim;
    }
}

// Hit reaction. data[0] points at the hit direction, data[1] is set for bullet hits (which are
// skipped while already playing one, and for bosses, Ulysses and aliens). The animation depends on
// the quadrant the hit came from (relative to the actor's facing, turned 45 degrees): Ulysses and
// the bosses (by subgroup) have their own sets, bullets play one of three bullet hits (with an
// "ouch"), anything else one fixed animation per quadrant. A bullet-hit actor is held in place
// until the animation ends. Further attacks are handled unless already being handled.
signed char STATE_HitReact(StateMachine &sm)
{
    int i = 0;
    int &bBullet = sm.GetIntRef(i);
    sm.GetActorOwner().SetWatch(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_ATTACKED_BY_PREDATOR:
        if (sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR)) return 1;
        AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR)) return 1;
        AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_INCOMING_ATTACK:
        if (sm.GetActorOwner().IsMsgOnStack(MSG_INCOMING_ATTACK)) return 1;
        if (sm.GetActorOwner().GetHealth() <= 0.0f) return 1;
        AU_HandleIncomingAttack(sm, (StructMsg_IncomingAttack *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_EXT:
        if (sm.GetActorOwner().HasAnimEnded() || sm.GetActorOwner().StartBossHeadRipped()) break;
        if (bBullet) sm.GetActorOwner().SetPositionEx((EXVector &)sm.GetLocalData()[4]);
        return 1;
    case MSG_ENTER: {
        const EXVector *p = (const EXVector *)sm.m_Msg.m_Data[0];
        EXVector dir;
        dir.x = p->x;
        dir.y = p->y;
        dir.z = p->z;
        bBullet = sm.m_Msg.m_Data[1];
        if (bBullet) {
            int mode = sm.GetActorOwner().m_AnimMode;
            if (mode == 0x90002b2 || mode == 0x90003d2 || mode == 0x90003d1) break;  // GEN_Bullet_Hit1/5/6
        }
        int bulletAnims[3] = {0x90002b2, 0x90003d2, 0x90003d1};
        if (bBullet && !sm.GetActorOwner().IsDrone()) break;
        sm.GetActorOwner().SetAnimType(0, 0, 1);
        Actor &a = sm.GetActorOwner();
        if (a.CanRipBossHead() && a.IsBoss()) {
            int anims[4] = {0x90003cf, 0x90003d0, 0x9000442, 0x9000443};
            sm.GetActorOwner().SetAnimType(anims[g_EXRandClass.Rand32() & 3], 0, 1);
            return 1;
        }
        if (a.IsHanging()) {
            int anims[3] = {0x900048e, 0x900048f, 0x900048d};
            sm.GetActorOwner().SetAnimType(anims[g_EXRandClass.Rand32() % 3], 0, 1);
            return 1;
        }
        if (a.IsLyingBum()) {
            a.SetAnimType(0x9000494, 0, 1);
            return 1;
        }
        float s = (float)sin(sm.GetActorOwner().GetRotY() + 0.785398163f);
        float c = (float)cos(sm.GetActorOwner().GetRotY() + 0.785398163f);
        float side = dir.x * -c + dir.z * s + dir.y * 0.0f;
        int anim;
        if (dir.x * s + dir.z * c + dir.y * 0.0f >= 0.0f) {
            if (side >= 0.0f) {
                if (a.IsUlysses()) {
                    anim = 0x9000254;
                } else if (a.IsBoss()) {
                    anim = AU_BossHitAnim(a, 0x9000221, 0x900050f, 1, 0);
                } else if (!bBullet) {
                    anim = 0x90001bd;
                } else {
                    goto bullet;
                }
            } else {
                if (a.IsUlysses()) {
                    anim = 0x9000251;
                } else if (a.IsBoss()) {
                    anim = AU_BossHitAnim(a, 0x900021f, 0x900050e, 0, 0x900040f);
                } else if (!bBullet) {
                    anim = 0x90001bb;
                } else {
                    goto bullet;
                }
            }
        } else if (side <= 0.0f) {
            if (a.IsUlysses()) {
                anim = 0x9000252;
            } else if (a.IsBoss()) {
                anim = AU_BossHitAnim(a, 0x9000221, 0x900050f, 1, 0);
            } else if (!bBullet) {
                int anims[4] = {0x90001ba, 0x90002db, 0x90002dc, 0x90002dd};
                anim = anims[g_EXRandClass.Rand32() & 3];
            } else {
                goto bullet;
            }
        } else {
            if (a.IsUlysses()) {
                anim = 0x9000253;
            } else if (a.IsBoss()) {
                anim = AU_BossHitAnim(a, 0x9000221, 0x900050f, 1, 0);
            } else if (!bBullet) {
                anim = 0x90001bc;
            } else {
                goto bullet;
            }
        }
        goto play;
    bullet:
        sm.GetActorOwner().SayBulletOuch();
        anim = bulletAnims[g_EXRandClass.Rand32() % 3];
    play:
        sm.GetActorOwner().SetAnimType(anim, 0, 1);
        if (bBullet) (EXVector &)sm.GetLocalData()[4] = sm.GetActorOwner().GetItem()->Position();
        return 1;
    }
    default:
        return 0;
    }
    sm.Return(0, 0, 0, 0);
    return 1;
}

// Searching for the cloaked predator (drones only; others return at once): for up to 10-20 seconds
// the actor walks to random points 3-6 units away, roughly away from where the player was last
// seen (or, once within 5 units of that spot, away from the player himself), picking a new point
// every 1-2 seconds, with one of two search animations.
signed char STATE_SearchForPredator(StateMachine &sm)
{
    int i = 0;
    int &bMoving = sm.GetIntRef(i);
    int &anim = sm.GetIntRef(i);
    int &bNear = sm.GetIntRef(i);
    i = 4;
    EXVector &offset = (EXVector &)sm.GetLocalData()[4];
    EXVector &target = (EXVector &)sm.GetLocalData()[8];
    EXVector &gotoPos = (EXVector &)sm.GetLocalData()[12];
    float &moveTimer = (float &)sm.GetLocalData()[16];
    float &searchTimer = (float &)sm.GetLocalData()[17];
    sm.GetActorOwner().SetStatus(5);
    sm.GetActorOwner().SetWatch(7);
    sm.GetActorOwner().m_Attrib |= 0x200;
    sm.GetActorOwner().SetHeadTrackMode(0);
    int anims[2] = {0x900013f, 0x9000140};
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        if (sm.GetActorOwner().IsBoss() || sm.GetActorOwner().IsUlysses() || sm.GetActorOwner().IsAlien()) break;
        sm.GetActorOwner().ClearBitMsgOnStack(AW_BIT_HAVE_SEEN_PLAYER | 0xf);
        anim = anims[g_EXRandClass.Rand32() & 1];
        searchTimer = (float)EXBaseApp::NominalGameFrameRate() * (g_EXRandClass.Randf() + 1.0f) * 10.0f;
        bNear = 0;
        return 1;
    case MSG_EXIT:
        return 1;
    case MSG_RETURN:
        bMoving = 0;
        return 1;
    case MSG_EXT: {
        moveTimer -= XApp::GameFrameRateMul();
        searchTimer -= XApp::GameFrameRateMul();
        if (searchTimer < 0.0f) break;
        if (bMoving && moveTimer > 0.0f) goto move;
        if (!bNear) {
            const EXVector &me = sm.GetActorOwner().GetItem()->Position();
            const EXVector &last = gAIControl.LastSeenPlayerPos();
            float dx = me.x - last.x;
            float dy = me.y - last.y;
            float dz = me.z - last.z;
            if (dx * dx + dy * dy + dz * dz < 25.0f) {
                bNear = 1;
                goto nearPlayer;
            }
            offset = me - last;
        } else {
        nearPlayer:
            offset = sm.GetActorOwner().GetItem()->Position() - XItemHandler_Player::Instance()->GetItem()->Position();
        }
        {
            EXVector back(0.0f - offset.x, 0.0f - offset.y, 0.0f - offset.z, 0.0f - offset.w);
            float r = g_EXRandClass.Randf();
            float a = (float)atan2(back.x, back.z) + r * 3.14159274f - 1.57079637f;
            Actor *pA = &sm.GetActorOwner();
            EXMatrix m;
            m.set_rotate_y(a);
            EXVector fwd(0.0f, 0.0f, (g_EXRandClass.Randf() + 1.0f) * 3.0f, 0.0f);
            EXVector v;
            D3DXVec4Transform(&v, &fwd, &m);
            target = v + pA->GetItem()->Position();
            sm.GetActorOwner().SetAnimType(anim, 0, 1);
            gotoPos = target;
            bMoving = 1;
            moveTimer = (float)EXBaseApp::NominalGameFrameRate() * (g_EXRandClass.Randf() + 1.0f) * 3.0f;
        }
    move:
        sm.GetActorOwner().GotoPosPoly(gotoPos, gAIControl.m_PlayerNavLocLastSeen.GetPolyHandle());
        return 1;
    }
    AU_DEFAULT_MSGS
    default:
        return 0;
    }
    sm.Return(0, 0, 0, 0);
    return 1;
}

// Set alight (data[0], when set, points at the source of the fire). After 0-1 s the actor bursts
// into flames; he runs screaming between run-away points (jumping off at jump-off points) for 3-6
// seconds and then dies. Close to the blast (within 3 units) he may instead die at once with one of
// several death animations turned to face it; a quarter of the time he dies on the spot anyway.
// Incendiary mine hits are ignored; other attacks are handled as usual.
signed char STATE_BlazinSquad(StateMachine &sm)
{
    EXVector &src = (EXVector &)sm.GetLocalData()[0];
    int &bHasSrc = (int &)sm.GetLocalData()[4];
    int &stage = (int &)sm.GetLocalData()[5];
    int &poi = (int &)sm.GetLocalData()[6];
    int &bJumpOff = (int &)sm.GetLocalData()[7];
    int &bBurning = (int &)sm.GetLocalData()[8];
    int &bFirst = (int &)sm.GetLocalData()[9];
    float &igniteTimer = (float &)sm.GetLocalData()[10];
    float &lifeTimer = (float &)sm.GetLocalData()[11];
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetStatus(3);
    sm.GetActorOwner().SetHeadTrackMode(0);
    switch (sm.m_Msg.m_Type) {
    case MSG_ENTER:
        sm.GetActorOwner().m_ActorFlags2.m_bOnFire = 1;
        bFirst = 1;
        stage = 0;
        igniteTimer = g_EXRandClass.Randf() * 60.0f;
        lifeTimer = (g_EXRandClass.Randf() + 1.0f) * 180.0f;
        sm.GetActorOwner().SetAnimType(0x4e000001, 0, 1);  // HT_AnimType_Stand
        if (sm.m_Msg.m_Data[0]) {
            bHasSrc = 1;
            src = *(EXVector *)sm.m_Msg.m_Data[0];
            return 1;
        }
        bHasSrc = 0;
        return 1;
    case MSG_EXIT:
        return 1;
    case MSG_EXT: {
        if (!bBurning) {
            igniteTimer -= XApp::GameFrameRateMul();
            if (igniteTimer <= 0.0f) {
                sm.GetActorOwner().EffectCreate(0x10000e3, 0x4000410, 0, 0.0f, 0, 0);  // HT_Script_ActorFX_Fire
                sm.GetActorOwner().EffectCreate(0x10000e3, 0x4000423, 0, 0.0f, 0, 0);  // HT_Script_ActorFX_Smoke
                bBurning = 1;
            }
        }
        int anim;
        if (bFirst) {
            bFirst = 0;
            poi = AU_FindRunAwayPoint(sm, 4, poi, sm.GetActorOwner().OrgLoc().NavZone(), 0);
            if (poi) {
                volatile unsigned int r = g_EXRandClass.Rand32() & 3;
                if (!r) goto run;
            }
            if (!bBurning) {
                sm.GetActorOwner().EffectCreate(0x10000e3, 0x4000410, 0, 0.0f, 0, 0);
                sm.GetActorOwner().EffectCreate(0x10000e3, 0x4000423, 0, 0.0f, 0, 0);
                bBurning = 1;
            }
            const EXVector &me = sm.GetActorOwner().GetItem()->Position();
            float dx = me.x - src.x;
            float dy = me.y - src.y;
            float dz = me.z - src.z;
            if (!bHasSrc || dx * dx + dy * dy + dz * dz >= 9.0f) {
                unsigned int r = g_EXRandClass.Rand32();
                if ((r & 1) == 0) goto onFire;
                if ((r & 1) == 1) goto explosion5;
            } else {
                switch (g_EXRandClass.Rand32() % 5) {
                case 1:
                explosion5:
                    anim = 0x90003b8;  // HT_AnimMode_GEN_Death_Explosion5
                    break;
                case 2: {
                    EXVector d = (EXVector(0.0f, 0.0f, 0.0f, 0.0f) - src) - sm.GetActorOwner().GetHeadPos();
                    sm.GetActorOwner().SetRotY((float)atan2((0.0f - src.x) - sm.GetActorOwner().GetHeadPos().x, d.z));
                    anim = 0x90001c5;  // HT_AnimMode_GEN_Death_Explosion3
                    break;
                }
                case 3: {
                    float dz2 = src.z - sm.GetActorOwner().GetHeadPos().z;
                    EXVector d = src - sm.GetActorOwner().GetHeadPos();
                    sm.GetActorOwner().SetRotY((float)atan2(d.x, dz2));
                    anim = 0x90001c4;  // HT_AnimMode_GEN_Death_Explosion4
                    break;
                }
                case 4: {
                    float dz2 = src.z - sm.GetActorOwner().GetHeadPos().z;
                    EXVector d = src - sm.GetActorOwner().GetHeadPos();
                    sm.GetActorOwner().SetRotY((float)atan2(d.x, dz2));
                    anim = 0x90002bb;  // HT_AnimMode_GEN_Death_Blown_Back
                    break;
                }
                default:
                onFire:
                    anim = 0x90003ad;  // HT_AnimMode_GEN_Death_On_Fire
                    break;
                }
                sm.GetActorOwner().SetAnimType(anim, 0, 1);
            }
            sm.GetActorOwner().SetIgnoreAnimChanges(1);
            SM_GOTO(STATE_AnimDeath, 0, 0, 0, 0);
            return 1;
        }
    run:
        lifeTimer -= XApp::GameFrameRateMul();
        if (lifeTimer <= 0.0f) {
            SM_GOSUB(STATE_DeathBullet, 0, 0, 0, 0);
            return 1;
        }
        if (stage == 0) {
            poi = AU_FindRunAwayPoint(sm, 4, poi, sm.GetActorOwner().OrgLoc().NavZone(), 0);
            if (poi) {
                stage = 1;
                POI_STRUCT *p = POI_Get(poi);
                int anims[4] = {0x9000150, 0x4e000001, 0x900014f, 0x900013d};
                bJumpOff = p->m_Type == 0x420000bd;  // HT_LocationType_JumpOff
                sm.GetActorOwner().SetAnimType(anims[g_EXRandClass.Rand32() & 3], 0, 1);
                sm.GetActorOwner().GotoPOI(poi);
                sm.GetActorOwner().Speak(0x1af000e1, 0);
                return 1;
            }
            unsigned int r = g_EXRandClass.Rand32();
            if ((r & 1) == 0)
                sm.GetActorOwner().SetAnimType(0x90003ad, 0, 1);  // HT_AnimMode_GEN_Death_On_Fire
            else if ((r & 1) == 1)
                sm.GetActorOwner().SetAnimType(0x90003b8, 0, 1);  // HT_AnimMode_GEN_Death_Explosion5
            sm.GetActorOwner().SetIgnoreAnimChanges(1);
            SM_GOTO(STATE_AnimDeath, 0, 0, 0, 0);
            return 1;
        }
        if (stage != 1) return 1;
        if (bJumpOff) {
            if (sm.GetActorOwner().SqDistToTarget() < kArriveSqDist) {
                SM_GOTO(STATE_RagdollJump, (int)(POI_Get(poi)->m_YRot * 0.159154937f * 360.0f), 0, 0, 0);
                return 1;
            }
            if (bJumpOff) return 1;
        }
        if (sm.GetActorOwner().SqDistToTarget() < 1.0f) stage = 0;
        return 1;
    }
    case MSG_ATTACKED_BY_PREDATOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR) &&
            ((StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0])->m_Attack != 0x4900000f)  // HT_Weaponry_Predator_IncendiaryMine
            AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!sm.GetActorOwner().IsMsgOnStack(MSG_ATTACKED_BY_ACTOR) &&
            ((StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0])->m_pWeapon->m_Id != 0x4900000f)
            AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    default:
        return 0;
    }
}

// Extreme panic: shouts (a loud noise), then runs. Each new leg heads for a nearby free panic point
// within reach (10 units, 4 off pavement) the player is not in the way of, else runs 8-10 units
// away from the player at a random angle (widening over five tries). Out of options he cowers
// (crying on level ground half the time) for 5 seconds. At a panic point he jumps off (jump
// points), hides (hide points, until the timer runs out) or is deleted. He screams every 8-16
// seconds (some skins have their own line).
signed char STATE_PanicExtreme(StateMachine &sm)
{
    int &stage = (int &)sm.GetLocalData()[0];
    int &bHaveTarget = (int &)sm.GetLocalData()[1];
    int &bCowering = (int &)sm.GetLocalData()[2];
    int &runAnim = (int &)sm.GetLocalData()[3];
    int &poi = (int &)sm.GetLocalData()[4];
    float &runDist = (float &)sm.GetLocalData()[5];
    int &collide = (int &)sm.GetLocalData()[6];
    float &timer = (float &)sm.GetLocalData()[7];
    float &screamTimer = (float &)sm.GetLocalData()[8];
    int &heldPoi = (int &)sm.GetLocalData()[9];
    sm.GetActorOwner().SetStatus(3);
    sm.GetActorOwner().SetWatch(0);
    sm.GetActorOwner().SetHeadTrackMode(0);
    int anims[4] = {0x4e000003, 0x9000150, 0x4e00000d, 0x900013d};  // HT_AnimType_Run, GEN_Panic_Run3, ..., GEN_Panic_Run7
    switch (sm.m_Msg.m_Type) {
    case MSG_RETURN:
        bHaveTarget = 0;
        return 1;
    case MSG_EXIT:
        if (heldPoi && POI_GetComData(heldPoi, 0, 0, 0) == sm.GetActorOwner().GetRefID())
            POI_SetComData(heldPoi, 0, 0, 0, 0);
        return 1;
    case MSG_ENTER:
        Actor::SendMsg(MsgItem(MSG_LOUD_NOISE), sm.GetActorOwner().GetItem()->Position(), 10.0f, &sm.GetActorOwner(), -1.0f);
        stage = 0;
        runAnim = anims[g_EXRandClass.Rand32() & 3];
        runDist = g_EXRandClass.Randf() * 2.0f + 8.0f;
        return 1;
    case MSG_EXT:
        switch (stage) {
        case 0: {
            if (sm.GetActorOwner().HasCollided(&collide) && collide >= 10) bHaveTarget = 0;
            screamTimer -= XApp::GameFrameRateMul();
            if (0.0f >= screamTimer) {
                float t;
                if (sm.GetActorOwner().GetSkin()->m_VOType == 0x12) {
                    sm.GetActorOwner().Speak(0x1af0061c, 0);
                    t = g_EXRandClass.Randf() * 180.0f + 240.0f;
                } else {
                    Actor *pA = &sm.GetActorOwner();
                    pA->Speak(pA->GetScreamSFX(), 0);
                    t = g_EXRandClass.Randf() * 240.0f + 480.0f;
                }
                screamTimer = t;
            }
            if (bHaveTarget) {
                if (!(0.5f > sm.GetActorOwner().SqDistToTarget()) &&
                    !AU_PlayerObstructs(sm, sm.GetActorOwner().GetGotoPos(), 3.14159274f))
                    goto checkAnim;
            }
            bHaveTarget = 1;
            timer = 300.0f;
            {
                Actor *pA = &sm.GetActorOwner();
                poi = POI_GetNearestFlag(pA->GetItem()->Position(), 0x9001001, poi, pA->OrgLoc().NavZone(), pA->CanJump(), 0, -1.0f);
                float range = 10.0f;
                if (!((sm.GetActorOwner().OrgLoc().SurfaceFlags() >> 10) & 1)) range = 4.0f;
                if (poi && !POI_GetComData(poi, 0, 0, 0)) {
                    const EXVector &me = sm.GetActorOwner().GetItem()->Position();
                    EXVector d = me - POI_Get(poi)->m_Pos;
                    if (d.y * d.y + d.z * d.z + d.x * d.x < range * range &&
                        !AU_PlayerObstructs(sm, POI_Get(poi)->m_Pos, 3.14159274f)) {
                        stage = 4;
                        POI_SetComData(poi, sm.GetActorOwner().GetRefID(), 0, 0, 0);
                        sm.GetActorOwner().GotoPOI(poi);
                        sm.GetActorOwner().SetAnimType(runAnim, 0, 1);
                        return 1;
                    }
                }
            }
            {
                float hi = 1.57079637f;
                float lo = 0.0f;
                for (int k = 0; k < 5; k++) {
                    EXVector d = sm.GetActorOwner().GetItem()->Position() - XItemHandler_Player::Instance()->GetItem()->Position();
                    float ang = (float)atan2(d.x, d.z);
                    float off = (hi - lo) * g_EXRandClass.Randf() + lo;
                    lo = hi;
                    hi += 0.314159274f;
                    volatile unsigned int r = g_EXRandClass.Rand32() & 1;
                    if (!r) off = 0.0f - off;
                    EXMatrix m;
                    m.set_rotate_y(off + ang);
                    EXVector fwd(0.0f, 0.0f, runDist, 0.0f);
                    EXVector v;
                    D3DXVec4Transform(&v, &fwd, &m);
                    EXVector delta = v;
                    sm.GetActorOwner().GotoPosDelta(delta);
                    Actor *pA = &sm.GetActorOwner();
                    EXVector left = pA->GetItem()->Position() - pA->m_DestLoc.Pos();
                    float lim = runDist * 0.75f;
                    if (lim * lim > left.y * left.y + left.z * left.z + left.x * left.x) {
                        if (k < 4) continue;
                        if (sm.GetActorOwner().OrgLoc().FloorNormal().y > 0.9f) {
                            volatile unsigned int r2 = g_EXRandClass.Rand32() & 1;
                            if (r2) {
                                stage = 1;
                                if (!bCowering) sm.GetActorOwner().SetAnimType(0x9000018, 0, 1);  // HT_AnimMode_Crouched_Crying
                                bCowering = 1;
                                return 1;
                            }
                        }
                        stage = 2;
                        sm.GetActorOwner().SetAnimType(0x900013c, 0, 1);  // HT_AnimMode_GEN_Panic1
                        return 1;
                    }
                    sm.GetActorOwner().SetAnimType(bCowering ? 0x90003b6 : runAnim, 0, 1);  // HT_AnimMode_Crouched_Crying2
                    bCowering = 0;
                    break;
                }
            }
        checkAnim:
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().SetAnimType(runAnim, 0, 1);
                return 1;
            }
            break;
        }
        case 1:
            sm.GetActorOwner().InvalidateGotoPos();
            timer -= XApp::GameFrameRateMul();
            if (sm.GetActorOwner().HasAnimEnded()) {
                sm.GetActorOwner().SetAnimType(0x90004b2, 0, 1);
                if (0.0f >= timer) {
                    stage = 0;
                    bHaveTarget = 0;
                    return 1;
                }
            }
            break;
        case 3: {
            Actor *pA = &sm.GetActorOwner();
            pA->SpinToYRot(POI_Get(heldPoi)->m_YRot);
        }
        case 2:
            timer -= XApp::GameFrameRateMul();
            if (sm.GetActorOwner().HasAnimEnded() && 0.0f >= timer) {
                stage = 0;
                bHaveTarget = 0;
                if (heldPoi && POI_GetComData(heldPoi, 0, 0, 0) == sm.GetActorOwner().GetRefID()) {
                    heldPoi = 0;
                    POI_SetComData(poi, 0, 0, 0, 0);
                    return 1;
                }
            }
            break;
        case 4:
            if (AU_PlayerObstructs(sm, POI_Get(poi)->m_Pos, 3.14159274f)) {
                stage = 0;
                return 1;
            }
            if (kArriveSqDist > sm.GetActorOwner().SqDistToTarget()) {
                if (POI_GetLocFlags(poi) & 0x1000000) {
                    SM_GOTO(STATE_RagdollJump, (int)(POI_Get(poi)->m_YRot * 0.159154937f * 360.0f), 0, 0, 0);
                    return 1;
                }
                if (POI_GetLocFlags(poi) & 0x8000000) {
                    stage = 3;
                    heldPoi = poi;
                    Actor *pA = &sm.GetActorOwner();
                    pA->SetRotY(POI_Get(poi)->m_YRot);
                    int hide[2] = {0x900043e, 0x900043f};
                    sm.GetActorOwner().SetAnimType(hide[g_EXRandClass.Rand32() & 1], 0, 1);
                    return 1;
                }
                sm.GetActorOwner().Delete();
            }
            break;
        }
        return 1;
    AU_DEFAULT_MSGS
    default:
        return 0;
    }
}
