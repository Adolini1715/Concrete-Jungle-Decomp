#include <math.h>
#include "FEText.h"
#include "XItemPhysics.h"
#include "XItem.h"
#include "EXItemAnimator.h"
#include "EXItemHandler.h"
#include "EXCollisionScoopTemp.h"
#include "EXMemory.h"
#include "XApp.h"
#include "Vehicle.h"

// TODO stand-ins: other areas.
Vehicle *getItemOwnerVehicle(EXItem *pItem);
s8 IsTouchplate(EXItem *pItem);
s8 IsEnvironment(EXItem *pItem);
class XItemHandler_MissionTrigger {  // TODO stand-in
public:
    void SetCollided() { m_bCollided = 1; }
    u8 m_Unk0[0x82];
    s8 m_bCollided;             // 0x82
};
class XItemHandler_Environment {  // TODO stand-in
public:
    void SetCollided() { m_bCollided = 1; }
    u8 m_Unk0[0x84];
    s8 m_bCollided;             // 0x84
};
class XItemHandler_AirVehicle {  // TODO stand-in
public:
    static const EXRuntimeClass classXItemHandler_AirVehicle;
    u8 m_Unk0[0xfc];
    float m_HitForce;           // 0xfc
    u8 m_Unk100[0x148 - 0x100];
    EXVector m_Position;        // 0x148
};
class XItemHandler_Player {  // TODO stand-in
public:
    static XItemHandler_Player *Instance() { return m_pInstance; }
    u8 m_Unk0[4];
    EXItem *m_pItem;
protected:
    static XItemHandler_Player *m_pInstance;
};
extern float frameTime;

// TODO stand-in: object/object collision query (EXCollision.obj, engine). TODO layout.
class EXInterCollisionTemp {
public:
    EXFixedArray<EXVector, 2> m_Pos;                // 0x00
    EXVector m_Normal;                              // 0x20
    u8 m_Unk30[0x20];
    EXFixedArray<EXItem *, 2> m_pItem;              // 0x50
    EXFixedArray<EXCollisionType *, 2> m_pCol;      // 0x58
    EXFixedArray<const EXMatrix *, 2> m_pMtx;       // 0x60
    u8 m_Unk68[0x40];
    EXFixedArray<float, 2> m_a8;                    // 0xa8
    EXFixedArray<float, 2> m_Weight;                // 0xb0
    u8 m_Unkb8[0x20];
    float m_Depth;                                  // 0xd8
    u8 m_Unkdc[8];
    EXFixedArray<u8, 2> m_bActive;                  // 0xe4
    u8 m_Unke6[2];
    u8 m_Index;                                     // 0xe8
};

EXRuntimeClass *XItemPhysics::_GetBaseClass() { return EXRUNTIME_CLASS(EXItemPhysics); }
const EXRuntimeClass XItemPhysics::classXItemPhysics = {
    EXRUNTIME_CLASS(EXItemPhysics), "XItemPhysics", sizeof(XItemPhysics), (void *(__cdecl *)())XItemPhysics::CreateObject};

void XItemPhysics::SetPause(s8 b) { m_bPause = b; }

// NEAR: register allocation (retail keeps this in ecx, zero byte in dl, late stack frame).
s8 XItemPhysics::DoUpdate()
{
    m_PrevPos = m_pItem->m_position;
    m_PrevRot = m_pItem->m_rotation;
    m_FrameRateDiv = XApp::GameFrameRateDiv();
    if (m_bPhysics && !m_bPause) {
        if (!m_bCollision) m_DeltaMove.set_zero();
        float mul = XApp::GameFrameRateMul();
        if (!(m_physicsFlags & 4)) {
            EXMaths::vec_xyzw d = m_DeltaMove.vec_xyzw::operator*(mul);
            float y = d.y;
            if (!(m_physicsFlags & 1)) y = y - m_Gravity * mul * mul * 0.5f;
            d.y = m_Accel.y * mul * mul * 0.5f + y;
            EXMaths::vec_xyzw p = d + m_pItem->m_position;
            m_pItem->m_position = p + m_PFDeltaMove;
            m_pItem->m_validMtx = 0;
            EXMaths::vec_xyzw r = m_RotDelta + m_pItem->m_rotation;
            m_pItem->m_rotation = m_PFRotDelta + r;
            m_pItem->m_validMtx = 0;
        }
        if (!(m_physicsFlags & 1)) m_DeltaMove.y -= m_Gravity * mul;
        m_DeltaMove.y = m_Accel.y * mul + m_DeltaMove.y;
        m_DeltaMove.x *= m_Friction;
        m_DeltaMove.z *= m_Friction;
        m_PFDeltaMove.set_zero();
        m_PFRotDelta.set_zero();
        m_PrevCollisionFlags = m_collisionFlags;
        m_Accel.set_zero();
    }
    return 1;
}

void XItemPhysics::EnableCollision(s8 b) { m_bCollision = b; }
void XItemPhysics::EnablePhysics(s8 b) { m_bPhysics = b; }

void XItemPhysics::SetCollisionHeightAndWidth(float h)
{
    m_ColOffset.set_zero();
    m_ColHeight = h;
    m_ColWidth = h;
}

void XItemPhysics::SetCollisionHeightAndWidth(float h, float w)
{
    m_ColOffset.Set(0.0f, h * 0.5f, 0.0f, 0.0f);
    m_ColHeight = h * 0.5f;
    m_ColWidth = w * 0.5f;
}

// NEAR: sum order of the squared terms (retail w, z, x) will not stick.
float XItemPhysics::RealSpeed()
{
    EXVector d(m_pItem->Position() - m_PrevPos);
    d.y = 0.0f;
    return d.Length();
}

float XItemPhysics::RealFallSpeed()
{
    EXVector d = m_pItem->Position() - m_PrevPos;
    return d.y;
}

float XItemPhysics::GetThrustToAttainHeight(float h)
{
    h = m_Gravity * h * 2.0f;
    if (h > 0.0f) return (float)sqrt(h);
    return 0.0f;
}

void XItemPhysics::SetThrustToAttainHeight(float h)
{
    h = m_Gravity * h * 2.0f;
    m_DeltaMove.y = h > 0.0f ? (float)sqrt(h) : 0.0f;
}

void XItemPhysics::SetCollisionObject(float a, float b, float c, float d)
{
    EXCapsule &cap = m_ColObject.Capsule();
    cap.Radius() = b;
    cap.Origin().Set(d, a, 0.0f, 0.0f);
    cap.Direction().Set(0.0f, c, 0.0f, 0.0f);
    m_ColObject.SetType(EXCollisionType::COL_CAPSULE);
}

void XItemPhysics::CheckGameFlags(EXCScoop_FaceData *pFace)
{
    EXVector *pN = (EXVector *)pFace->m_00;
    if ((float)fabs(pN->y) < 0.1f && !(pFace->FaceFlags() & 0x4000)) {
        m_collisionFlags |= 4;
        m_GameFlagPos = pN[0];
        m_GameFlagNorm = pN[2];
    }
}

void XItemPhysics::HitByVehicle(Vehicle *pVehicle, EXCScoop_FaceData *pFace)
{
    if (m_pItem == XItemHandler_Player::Instance()->m_pItem) pVehicle->getMove().m_playerHitTime = frameTime;
    if (pFace->m_flags & 2) return;
    if (m_pHitVehicle) return;
    m_pHitVehicle = pVehicle;
    m_HitVehicleVel = EXVector(pVehicle->getMove().m_phys.m_velocity, 0.0f);
}

XItemPhysics::XItemPhysics()
{
    m_DeltaMove.set_zero();
    m_RotDelta.set_zero();
    m_PFDeltaMove.set_zero();
    m_PFRotDelta.set_zero();
    m_Accel.set_zero();
    SetCollisionHeightAndWidth(2.0f, 5.0f);
    m_Gravity = 0.016333334f;
    m_Friction = 0.5f;
    m_3c = 1.0f;
    m_40 = 0.98f;
    m_PrevCollisionFlags = 0;
    m_bPause = 0;
    m_bCollision = 1;
    m_bPhysics = 1;
    m_CollisionResponse.set_zero();
    m_FrameRateDiv = 1.0f;
    ResetHitByVehicle();
    m_bVertWallReaction = 0;
    m_bIgnoreCars = 0;
    m_130 = 0.0f;
    m_ImpactSpeed = 0.0f;
    SetCollisionObject(0.75f, 0.75f, 0.75f, 0.0f);
}

s8 XItemPhysics::Initialise()
{
    m_physicsFlags = 0;
    m_collisionFlags = 0;
    SetCollisionObject(0.75f, 0.75f, 0.75f, 0.0f);
    return 1;
}

// NEAR: VC stops inlining the by-value EXVector ops here (retail inlines them); register choice.
void XItemPhysics::FaceCollisionCallBack(EXCScoop_FaceData *pFace, EXItemPhysics &phys)
{
    XItemPhysics &xp = (XItemPhysics &)phys;
    xp.CheckGameFlags(pFace);
    if (pFace->m_flags & 2) xp.m_FloorNorm += *(EXVector *)pFace->m_00;
    EXPlacementItem *pItem = (EXPlacementItem *)((EXCScoop_EntityInfo *)pFace->m_34)->m_id;
    if (!pItem) return;
    while (pItem->IsKindOf(EXRUNTIME_CLASS(EXItemAnimator))) {
        pItem = ((EXItemAnimator *)pItem)->GetOwnerItem();
        if (!pItem) return;
    }
    EXItem *pX = (EXItem *)pItem;
    Vehicle *pVehicle = getItemOwnerVehicle(pX);
    if (pVehicle) {
        if (xp.m_bIgnoreCars) pFace->m_flags |= 8;
        else xp.HitByVehicle(pVehicle, pFace);
    }
    EXItemHandler *pH = pX->ItemHandler();
    if (pH && pH->IsKindOf(EXRUNTIME_CLASS(XItemHandler_AirVehicle))) {
        XItemHandler_AirVehicle *pAir = (XItemHandler_AirVehicle *)pX->ItemHandler();
        EXMaths::vec_xyzw d = (const EXMaths::vec_xyzw &)pX->Position() - pAir->m_Position;
        float len = d.Length();
        if (len > EX_EPSILON_UNIT) d *= 1.0f / len;
        EXVector f = d * pAir->m_HitForce;
        xp.HitByFlyingVehicle((XItem *)pX, f);
    }
    if (!pVehicle) {
        if (IsTouchplate(pX)) {
            ((XItemHandler_MissionTrigger *)pX->ItemHandler())->SetCollided();
            pFace->m_flags |= 8;
            return;
        }
        if (IsEnvironment(pX)) {
            ((XItemHandler_Environment *)pX->ItemHandler())->SetCollided();
            pFace->m_flags |= 8;
        }
    }
}

s8 XItemPhysics::InterObjCollisions(EXInterCollisionTemp &t)
{
    t.m_pCol[t.m_Index] = &m_ColObject;
    t.m_bActive[t.m_Index] = 1;
    t.m_pItem[t.m_Index] = m_pItem;
    t.m_pMtx[t.m_Index] = &m_pItem->mtx();
    t.m_Pos[t.m_Index] = m_pItem->Position();
    t.m_a8[t.m_Index] = 5.0f;
    t.m_Weight[t.m_Index] = 80.0f;
    return 1;
}

// NEAR: by-value EXVector ops / MakeIdentity are not all inlined as in retail.
void XItemPhysics::InterObjCallback(EXInterCollisionTemp &t)
{
    if (!m_bPhysics || m_bPause) return;
    u8 other = t.m_Index == 0 ? 1 : 0;
    if (t.m_Weight[other] < 100.0f) return;
    EXMaths::vec_xyzw push = t.m_Normal.vec_xyzw::operator*(t.m_Depth);
    if (t.m_Normal.y > 0.05f) {
        m_collisionFlags |= 1;
        push.z = 0.0f;
        push.x = 0.0f;
    }
    m_pItem->SetPosition(EXVector(m_pItem->Position().vec_xyzw::operator+(push)));
    u32 flags = 0;
    m_pItem->CalcMatrix_InPlace(EXMatrix::MakeIdentity(), flags);
    t.m_Pos[t.m_Index] += push;
}

s8 XItemPhysics::CollideCapsule(EXCapsule &c, EXVector &out)
{
    EXCollisionType col;
    EXCapsule &cap = col.Capsule();
    cap.m_radius = c.m_radius;
    cap.m_seg.m_Origin.x = c.m_seg.m_Origin.x;
    cap.m_seg.m_Origin.y = c.m_seg.m_Origin.y;
    cap.m_seg.m_Origin.z = c.m_seg.m_Origin.z;
    cap.m_seg.m_Origin.w = c.m_seg.m_Origin.w;
    cap.m_seg.m_Direction.x = c.m_seg.m_Direction.x;
    cap.m_seg.m_Direction.y = c.m_seg.m_Direction.y;
    cap.m_seg.m_Direction.z = c.m_seg.m_Direction.z;
    cap.m_seg.m_Direction.w = c.m_seg.m_Direction.w;
    col.SetType(EXCollisionType::COL_CAPSULE);
    EXCollisionScoopTemp scoop;
    scoop.Initialise();
    scoop.m_xform.SetIdentity();
    scoop.m_numObjects = 1;
    scoop.m_pObjects = &col;
    scoop.CalcBounds(scoop.m_xform, 4);
    EXGetWorld()->DoCollisionScoopToMap(scoop);
    EXGetWorld()->DoCollisionScoopToContoured(scoop, m_pItem);
    scoop.CalculateFaceArray();
    if (!scoop.m_numFaces) return 0;
    out = *(EXVector *)(scoop.m_pFaces->m_00 + 0x20);
    return 1;
}

s8 XItemPhysics::CollideSphereClose(EXSphere &s, EXVector &a, EXVector &b)
{
    EXCollisionType col;
    EXSphere &sp = col.Sphere();
    sp.m_radius = s.m_radius;
    sp.m_centre.x = s.m_centre.x;
    sp.m_centre.y = s.m_centre.y;
    sp.m_centre.z = s.m_centre.z;
    sp.m_centre.w = s.m_centre.w;
    col.SetType(EXCollisionType::COL_SPHERE);
    EXCollisionScoopTemp scoop;
    scoop.Initialise();
    scoop.m_flags |= 2;
    scoop.m_xform.SetIdentity();
    scoop.m_numObjects = 1;
    scoop.m_pObjects = &col;
    scoop.CalcBounds(scoop.m_xform, 4);
    EXGetWorld()->DoCollisionScoopToMap(scoop);
    EXGetWorld()->DoCollisionScoopToContoured(scoop, m_pItem);
    scoop.CalculateFaceArray();
    u32 n = scoop.m_numFaces;
    if (!n) return 0;
    {
        float best = s.m_radius * s.m_radius;
        int found = -1;
        EXCScoop_FaceData *pFace = scoop.m_pFaces;
        for (u32 i = 0; i < scoop.m_numFaces; i++, pFace++) {
            if (pFace->FaceFlags() & 0x10) {
                EXVector d = s.m_centre - *(EXVector *)(pFace->m_00 + 0x20);
                float dist = d.LengthXYZSquared();
                if (best > dist) {
                    best = dist;
                    found = i;
                }
            }
        }
        if (found == -1) return 0;
        EXCScoop_FaceData *pBest = &scoop.m_pFaces[found];
        a = *(EXVector *)(pBest->m_00 + 0x20);
        b = *(EXVector *)pBest->m_00;
        return 1;
    }
}

// Finds the vertical wall and the ledge edge nearest a sphere, for climbing and wall moves.
// Wall: the nearest scooped face that is near vertical (|normal y| <= 0.1) within 10 units;
// returns its contact point and normal. For a double-sided face (face flag 0x100) the stored
// normal is turned to face against the direction from the sphere side to the face side. Edge:
// the nearest edge face (flag 0x10) inside the sphere. Faces with flag 8 are ignored. Returns 0
// when neither is found.
s8 XItemPhysics::CollideSphereVertWall(EXSphere &s, s8 &bWall, EXVector &wallPoint, EXVector &wallNormal, s8 &bEdge,
                                       EXVector &edgePoint, EXVector &edgeNormal)
{
    EXCollisionType col;
    EXSphere &sp = col.Sphere();
    sp.m_radius = s.m_radius;
    sp.m_centre.x = s.m_centre.x;
    sp.m_centre.y = s.m_centre.y;
    sp.m_centre.z = s.m_centre.z;
    sp.m_centre.w = s.m_centre.w;
    col.SetType(EXCollisionType::COL_SPHERE);
    EXCollisionScoopTemp scoop;
    scoop.Initialise();
    scoop.m_pObjects = &col;
    scoop.m_xform.SetIdentity();
    scoop.m_numObjects = 1;
    scoop.CalcBounds(scoop.m_xform, 4);
    EXGetWorld()->DoCollisionScoopToMap(scoop);
    EXGetWorld()->DoCollisionScoopToContoured(scoop, m_pItem);
    scoop.CalculateFaceArray();
    if (!scoop.m_numFaces) return 0;

    int wall = -1;
    int edge = -1;
    float wallBest = 100.0f;
    float edgeBest = s.m_radius * s.m_radius;
    EXCScoop_FaceData *pFace = scoop.m_pFaces;
    for (u32 i = 0; i < scoop.m_numFaces; i++, pFace++) {
        unsigned short flags = pFace->FaceFlags();
        if (flags & 8) continue;
        if (fabsf(pFace->Normal().y) <= 0.1f) {
            EXVector d = s.m_centre - pFace->P1();
            float dist = d.LengthXYZSquared();
            if (dist < wallBest) {
                wall = i;
                wallBest = dist;
            }
        }
        if (flags & 0x10) {
            EXVector d = s.m_centre - pFace->P1();
            float dist = d.LengthXYZSquared();
            if (dist < edgeBest) {
                edge = i;
                edgeBest = dist;
            }
        }
    }
    if (wall == -1) {
        if (edge == -1) return 0;
        bWall = 0;
    } else {
        EXCScoop_FaceData &f = scoop.m_pFaces[wall];
        EXVector dir = f.P1() - f.P0();
        float len = dir.LengthXYZSquared();
        if (len >= 0.0f) {
            len = sqrtf(len);
            if (len > 1e-7f) {
                float inv = 1.0f / len;
                dir.x *= inv;
                dir.y *= inv;
                dir.z *= inv;
            }
        }
        EXVector n = f.Normal();
        if ((s8)f.m_3e < 0) {
            n.x = 0.0f - n.x;
            n.y = 0.0f - n.y;
            n.z = 0.0f - n.z;
            n.w = 0.0f - n.w;
        }
        // Keep only the horizontal part of the direction for walls, the vertical part for
        // floors and ceilings.
        if (n.y >= 0.8f || n.y <= -0.8f) {
            dir.x = 0.0f;
            dir.z = 0.0f;
        } else {
            dir.y = 0.0f;
        }
        if ((f.FaceFlags() & 0x100) && n.z * dir.z + n.x * dir.x + n.y * dir.y >= 0.0f) {
            f.Normal().x = 0.0f - n.x;
            f.Normal().y = 0.0f - n.y;
            f.Normal().z = 0.0f - n.z;
            f.Normal().w = 0.0f - n.w;
        }
        bWall = 1;
        wallPoint = f.P1();
        // Retail returns the stored normal, which is still unflipped for a back-facing single
        // sided face.
        wallNormal = f.Normal();
    }
    if (edge == -1) {
        bEdge = 0;
        return 1;
    }
    bEdge = 1;
    edgePoint = scoop.m_pFaces[edge].P1();
    edgeNormal = scoop.m_pFaces[edge].Normal();
    return 1;
}

// Moves the item from m_PrevPos to its new position through the world in steps of at most 0.5:
// each step scoops the map and contoured geometry around the capsule (radius x1.2 while
// gathering), marks faces (no-collide material: 0x42; with vertical-wall reaction, steep
// walls: 0x20), and asks for two push-outs: n1 (walls and floors) and n2 (vertical only).
// The item is pushed out, the velocity loses its component into each surface (scaled by 1,
// or 1/8 without vertical-wall reaction), and the rest of the move is projected the same
// way. On landing m_ImpactSpeed keeps the lowest vertical speed seen. Afterwards: floor
// normal (default up), and when not grounded but pushed up hard (normalised response
// y > 0.888) the item counts as grounded (m_130 = 10 frames of grace, downward speed cut).
void XItemPhysics::DoCollisions()
{
    float wallK = 1.0f;
    if (!m_bCollision || !m_bPhysics || m_bPause) return;
    m_collisionFlags = 0;
    m_FloorNorm.x = 0.0f;
    m_FloorNorm.y = 0.0f;
    m_FloorNorm.z = 0.0f;
    m_FloorNorm.w = 0.0f;
    if (!m_bVertWallReaction) wallK = 0.125f;
    m_pHitVehicle = 0;
    m_pHitFlyingVehicle = 0;
    EXItem *pItem = m_pItem;
    float px = m_PrevPos.x, py = m_PrevPos.y, pz = m_PrevPos.z, pw = m_PrevPos.w;
    float mx = pItem->m_position.x - m_PrevPos.x;
    float my = pItem->m_position.y - m_PrevPos.y;
    float mz = pItem->m_position.z - m_PrevPos.z;
    float mw = pItem->m_position.w - m_PrevPos.w;
    const float zk = wallK * 0.0f;
    float rx = 0.0f, ry = 0.0f, rz = 0.0f, rw = 0.0f;
    float impact = 0.0f;
    signed char more;
    do {
        float sx, sy, sz, sw, ex, ey, ez, ew;
        float sq = mz * mz + my * my + mx * mx;
        float len = sq < 0.0f ? 0.0f : sqrtf(sq);
        if (0.5f < len) {
            float s2 = mz * mz + my * my + mx * mx;
            sx = mx; sy = my; sz = mz; sw = 0.0f;
            if (s2 >= 0.0f && sqrtf(s2) > 1e-5f) {
                float inv = 1.0f / sqrtf(s2);
                sz = mz * inv;
                sw = inv * 0.0f;
                sx = inv * mx;
                sy = inv * my;
            }
            sz = sz * 0.5f;
            sx = sx * 0.5f;
            sy = sy * 0.5f;
            ez = mz - sz;
            sw = sw * 0.5f;
            ex = mx - sx;
            ey = my - sy;
            ew = mw - sw;
            more = 1;
        } else {
            ew = 0.0f; ez = 0.0f; ey = 0.0f; ex = 0.0f;
            more = 0;
            sx = mx; sy = my; sz = mz; sw = mw;
        }
        pw = pw + sw;
        pz = pz + sz;
        py = py + sy;
        px = px + sx;
        EXCollisionScoopTemp scoop;
        scoop.Initialise();
        pItem = m_pItem;
        EXVector rot = pItem->m_rotation;
        if (m_bVertWallReaction) {
            rot.z = 0.0f;
            rot.x = 0.0f;
        }
        scoop.m_xform.set_rotate(rot, ((EXPlacementItem *)pItem)->RotOrder());
        scoop.m_xform.r[3].x = px;
        scoop.m_xform.r[3].y = py;
        scoop.m_xform.r[3].z = pz;
        scoop.m_pObjects = &m_ColObject;
        scoop.m_xform.r[3].w = 1.0f;
        scoop.m_flags |= 2;
        scoop.m_numObjects = 1;
        m_ColObject.m_shape[8] = m_ColObject.m_shape[8] * 1.2f;
        scoop.CalcBounds(scoop.m_xform, 4);
        EXGetWorld()->DoCollisionScoopToMap(scoop);
        EXGetWorld()->DoCollisionScoopToContoured(scoop, m_pItem);
        scoop.CalculateFaceArray();
        m_ColObject.m_shape[8] = m_ColObject.m_shape[8] * 0.8333333f;
        EXVector n2(0.0f, 0.0f, 0.0f, 0.0f), n1(0.0f, 0.0f, 0.0f, 0.0f);
        scoop.CalculateFaceTypes(1);
        for (u32 i = 0; i < scoop.m_numFaces; i++) {
            EXCScoop_FaceData *pFace = &scoop.m_pFaces[i];
            if (!(pFace->FaceFlags() & 4)) {
                if (m_bVertWallReaction && m_130 <= 0.0f && !(pFace->m_flags & 0x12)) {
                    EXVector e = *(EXVector *)(pFace->m_00 + 0x20) - *(EXVector *)(pFace->m_00 + 0x10);
                    float l = e.y * e.y + e.z * e.z + e.w * e.w + e.x * e.x;
                    float ny = e.y;
                    if (l >= 0.0f && sqrtf(l) > 1e-5f) ny = (1.0f / sqrtf(l)) * e.y;
                    if ((float)fabs(ny) < 0.8f) pFace->m_flags |= 0x20;
                }
            } else
                pFace->m_flags |= 0x42;
        }
        scoop.CalculateDualCollisionResponse(*this, n1, n2, m_collisionFlags, 4, FaceCollisionCallBack);
        n1.w = 0.0f;
        n2.w = 0.0f;
        if (m_collisionFlags & 1) impact = impact <= m_DeltaMove.y ? impact : m_DeltaMove.y;
        px = px + n1.x;
        py = n1.y + n2.y + py;
        ry = n1.y + n2.y + ry;
        pz = pz + n1.z;
        rx = rx + n1.x;
        rz = rz + n1.z;
        {
            float l = n1.z * n1.z + n1.y * n1.y + n1.x * n1.x;
            if (l >= 0.0f && sqrtf(l) > 1e-5f) {
                float inv = 1.0f / sqrtf(l);
                n1.y = n1.y * inv;
                n1.z = n1.z * inv;
                n1.x = inv * n1.x;
                n1.w = inv * 0.0f;
            }
        }
        {
            float l = n2.x * n2.x + n2.z * n2.z + n2.y * n2.y;
            if (l >= 0.0f && sqrtf(l) > 1e-5f) {
                float inv = 1.0f / sqrtf(l);
                n2.x = inv * n2.x;
                n2.y = n2.y * inv;
                n2.z = n2.z * inv;
                n2.w = inv * 0.0f;
            }
        }
        float d = 0.0f - (m_DeltaMove.z * n1.z + m_DeltaMove.y * n1.y + m_DeltaMove.w * n1.w + m_DeltaMove.x * n1.x);
        float cw = d * n1.w * wallK;
        float cx = d * n1.x * wallK;
        float cy = n1.y * d * wallK;
        m_DeltaMove.x = cx + m_DeltaMove.x;
        float cz = n1.z * d * wallK;
        m_DeltaMove.y = cy + m_DeltaMove.y;
        m_DeltaMove.z = cz + m_DeltaMove.z;
        m_DeltaMove.w = cw + m_DeltaMove.w;
        d = 0.0f - (m_DeltaMove.z * n2.z + m_DeltaMove.y * n2.y + m_DeltaMove.w * n2.w + n2.x * m_DeltaMove.x);
        float dw = d * n2.w;
        float dy = n2.y * d * wallK;
        dw = dw * wallK;
        m_DeltaMove.x = zk + m_DeltaMove.x;
        m_DeltaMove.y = dy + m_DeltaMove.y;
        m_DeltaMove.z = zk + m_DeltaMove.z;
        m_DeltaMove.w = dw + m_DeltaMove.w;
        float f = 0.0f - (ew * n1.w + n1.x * ex + n1.y * ey + n1.z * ez);
        ey = n1.y * f + ey;
        ew = f * n1.w + ew;
        ez = n1.z * f + ez;
        ex = ex + f * n1.x;
        f = 0.0f - (ew * n2.w + n2.x * ex + n2.y * ey + n2.z * ez);
        my = n2.y * f + ey;
        mw = f * n2.w + ew;
        mx = ex;
        mz = ez;
    } while (more);
    if (m_collisionFlags & 1) m_ImpactSpeed = impact;
    pItem = m_pItem;
    pItem->m_position.x = px;
    pItem->m_position.y = py;
    pItem->m_position.z = pz;
    pItem->m_position.w = pw;
    pItem->m_validMtx = 0;
    m_CollisionResponse.x = rx;
    m_CollisionResponse.y = ry;
    m_CollisionResponse.z = rz;
    m_CollisionResponse.w = rw;
    if (m_FloorNorm.x * m_FloorNorm.x + m_FloorNorm.y * m_FloorNorm.y + m_FloorNorm.z * m_FloorNorm.z <= 0.001f) {
        m_FloorNorm.x = 0.0f;
        m_FloorNorm.y = 1.0f;
        m_FloorNorm.z = 0.0f;
        m_FloorNorm.w = 0.0f;
    } else {
        float l = m_FloorNorm.x * m_FloorNorm.x + m_FloorNorm.y * m_FloorNorm.y + m_FloorNorm.z * m_FloorNorm.z + m_FloorNorm.w * m_FloorNorm.w;
        if (l >= 0.0f && sqrtf(l) > 1e-5f) {
            float inv = 1.0f / sqrtf(l);
            m_FloorNorm.x = inv * m_FloorNorm.x;
            m_FloorNorm.y = inv * m_FloorNorm.y;
            m_FloorNorm.z = m_FloorNorm.z * inv;
            m_FloorNorm.w = m_FloorNorm.w * inv;
        }
    }
    if (0.0f < m_130) m_130 = m_130 - XApp::GameFrameRateMul();
    unsigned short fl = m_collisionFlags;
    if (!(fl & 1)) {
        float y = m_CollisionResponse.y;
        float l = m_CollisionResponse.x * m_CollisionResponse.x + y * y + m_CollisionResponse.w * m_CollisionResponse.w +
                  m_CollisionResponse.z * m_CollisionResponse.z;
        if (l >= 0.0f && sqrtf(l) > 1e-5f) y = (1.0f / sqrtf(l)) * y;
        if (y > 0.888f && (float)fabs(my) < 0.02f) {
            float v = m_DeltaMove.y;
            m_130 = 10.0f;
            m_ImpactSpeed = v;
            if (0.0f > m_DeltaMove.y) m_DeltaMove.y = 0.0f;
            m_collisionFlags = fl | 1;
            m_FloorNorm.x = 0.0f;
            m_FloorNorm.y = 1.0f;
            m_FloorNorm.z = 0.0f;
            m_FloorNorm.w = 0.0f;
        }
    }
}

XItemPhysics *XItemPhysics::CreateObject() { return new XItemPhysics; }
