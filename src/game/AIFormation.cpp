#include "AIFormation.h"
#include "AIControl.h"
#include "Actor.h"
#include "ActorProfile.h"
#include "XNavMesh.h"
#include "EXRand.h"
#include "EXBaseApp.h"
#include "EXMemory.h"
#include "FEText.h"

int g_iCombatInnerCircle = 5;
int g_iCombatMiddleCircle = 6;
int g_iCombatOuterCircle = 6;
CFormation g_Formations;

CFormationItem::CFormationItem()
{
    m_ID = -1;
    m_pFormation = 0;
    m_bActive = 0;
    m_Mode = 0;
    m_Timer = 0.0f;
    m_Angle = 0.0f;
    m_Pos.SetZero();
}

void CFormationItem::Update()
{
    m_Angle += 0.01f;
    if (m_Angle > 6.2831855f) m_Angle -= 6.2831855f;
    switch (m_Mode) {
    case 1: {
        float r = EXRandf(1.0f, 2.0f);
        m_Timer = r * EXBaseApp::NominalGameFrameRate();
        m_Mode++;
        break;
    }
    case 2:
        m_Timer -= EXBaseApp::GameFrameRateMul();
        if (m_Timer <= 0.0f) m_Mode = 0;
        break;
    }
}

EXVector CFormationItem::GetWorldAng() const
{
    float nx = -m_Pos.x;
    float nz = -m_Pos.z;
    EXVector r;
    r.x = 0.0f;
    r.y = (float)atan2(nx, nz) + m_pFormation->GetAng().y;
    r.z = 0.0f;
    r.w = 0.0f;
    return r;
}

signed char CFormationItem::AttackDistanceAngle(float &dist, float &ang)
{
    dist = m_Pos.Length();
    ang = (float)atan2(m_Pos.x, m_Pos.z) + m_pFormation->GetAng().y;
    if (1.0f > dist) dist = 1.0f;
    return 1;
}

CFormation::CFormation()
{
    m_Count = 0;
    for (int i = 0; i < 32; i++) m_Items[i].SetFormation(this);
    m_NavLoc.Reset();
    m_Ang.SetZero();
    m_v61c.SetZero();
    m_v62c.SetZero();
    m_w63e = 0;
    m_w63c = 0;
    m_640 = 0;
}

int CFormation::AllocSlot(int id)
{
    for (int i = 0; i < 32; i++)
        if (m_Items[i].GetID() == id) return i;
    if (m_Count == 32) return -1;
    m_Items[m_Count].SetActive(1);
    m_Items[m_Count].SetID(id);
    m_Count++;
    return m_Count - 1;
}

// NEAR: register allocation differs (retail: ebx flags, esi slot, two stack locals).
void CFormationItem::CalcItemPos(int slot, float radius)
{
    m_Flags &= ~7;
    float ang;
    float r;
    int inner = g_iCombatInnerCircle;
    float fslot = (float)slot;
    if (slot < inner) {
        ang = (6.2831855f / (float)inner) * fslot;
        r = 2.0f;
        m_Flags |= 2;
    } else {
        int middle = g_iCombatMiddleCircle;
        int n2 = middle + inner;
        if (slot < n2) {
            ang = (fslot - (float)inner + 0.5f) * (6.2831855f / (float)middle);
            r = 4.0f;
            m_Flags |= 1;
        } else {
            int outer = g_iCombatOuterCircle;
            int n3 = inner + (middle + outer);
            m_Flags |= 4;
            if (slot < n3) {
                ang = (fslot - (float)n2 + 0.5f) * (6.2831855f / (float)outer);
                r = 7.5f;
            } else {
                ang = (fslot - (float)n3) * 0.31415927f + 0.15707964f;
                r = 12.5f;
            }
        }
    }
    float a = m_Angle;
    m_Pos.y = 0.0f;
    m_Pos.w = 0.0f;
    m_Pos.x = (float)sin(ang) * r + (float)sin(a) * radius;
    m_Pos.z = (float)cos(ang) * r + (float)cos(a) * radius;
}

// NEAR: 56 bytes; esi/edi swapped (retail keeps this in esi, the item pointer in edi), 9 bytes shorter.
void CFormation::Update()
{
    m_v61c = gAIControl.LastSeenPlayerPos();
    m_NavLoc = gAIControl.PlayerNavLocLastSeen();
    m_v61c.y = m_NavLoc.FloorPos().y;
    float d = (m_v61c - m_v62c).Length();
    if (d > 3.0f) m_w63e |= 1;
    if (m_w63e & 1) {
        m_v62c = m_v61c;
        if (0.001f > d) m_w63e &= ~1;
        m_w63c = 0;
    } else if (++m_w63c > EXBaseApp::NominalGameFrameRate() * 2) {
        m_w63e |= 1;
    }
    int n = 0;
    for (int i = 0; i < 32; i++) {
        m_Items[i].Update();
        if (m_Items[i].GetActive()) {
            float radius = 0.0f;
            Actor *pActor = Actor::GetPtrFromID(m_Items[i].GetID());
            if (pActor && pActor->GetCombatCircleStopTime() <= 0.0f) {
                ActorProfile *pProfile = pActor->GetProfile();
                if (pProfile) radius = pProfile->m_FormationRadius;
            }
            m_Items[i].CalcItemPos(n, radius);
            if (++n >= m_Count) break;
        }
    }
    static float s_AttackTimer = EXRandf(1.0f, 2.0f) * 20.0f;
    s_AttackTimer -= EXBaseApp::GameFrameRateMul();
    int k = 0;
    if (m_Count && s_AttackTimer <= 0.0f) {
        for (int i = 0; i < 32; i++) {
            if (m_Items[i].GetActive()) {
                if (k < g_iCombatInnerCircle && m_Items[i].GetMode() == 0) m_Items[i].SetMode(1);
                k++;
            }
        }
        float r = EXRandf(0.3f, 1.0f);
        s_AttackTimer = r * EXBaseApp::NominalGameFrameRate();
    }
}

int CFormation::ItemNumFromActorID(int id) const
{
    for (int i = 0; i < 32; i++)
        if (m_Items[i].GetID() == id) return i;
    return -1;
}

CFormationItem *CFormation::ItemPtrFromActorID(int id, signed char create)
{
    for (;;) {
        int i = ItemNumFromActorID(id);
        if (i >= 0) return &m_Items[i];
        if (!create) return 0;
        if (AllocSlot(id) < 0) return 0;
        create = 0;
    }
}

signed char CFormation::GetAction(int id)
{
    CFormationItem *p = ItemPtrFromActorID(id, 1);
    if (p) return p->GetAction();
    return 0;
}

signed char CFormation::GetSlotDistAng(int id, float &dist, float &ang)
{
    CFormationItem *p = ItemPtrFromActorID(id, 1);
    if (p) return p->AttackDistanceAngle(dist, ang);
    return 0;
}

void CFormation::GetOrigin(EXVector &out)
{
    out = EXVector(GetPos().x, GetPos().y, GetPos().z, 0.0f);
}

// NEAR: retail copies GetPos() with an int x load and stores w first (pos half differs).
void CFormation::GetOriginAndAngle(EXVector &pos, EXVector &ang)
{
    pos = GetPos();
    ang = EXVector(GetAng().x, GetAng().y, GetAng().z, 0.0f);
    pos.w = 0.0f;
}

void CFormation::Initialise()
{
    m_NavLoc.Reset();
}

int CFormation::GetAllocatedSlotCount() const
{
    int n = 0;
    for (int i = 0; i < 32; i++)
        if (m_Items[i].GetActive()) n++;
    return n;
}

// NEAR: retail builds the difference through an EXVector temporary on the stack (0x24 frame); squares
// w, z, x of it.
float CFormation::GetDistSqClosestActor()
{
    float best = 3.4028235e38f;
    for (int i = 0; i < 32; i++) {
        if (m_Items[i].GetActive()) {
            Actor *pActor = Actor::GetPtrFromID(m_Items[i].GetID());
            if (pActor) {
                EXMaths::vec_xyzw d = GetPos() - pActor->GetFeetPos();
                float dsq = d.x * d.x + d.w * d.w + d.z * d.z;
                if (best > dsq) best = dsq;
            }
        }
    }
    return best;
}

// NEAR: ~25 bytes, retail loads the origin's x, z, y, w before the adds.
EXVector CFormationItem::GetWorldPos() const
{
    EXVector pos;
    pos.SetZero();
    float a = m_pFormation->m_Ang.y;
    float s = (float)sin(a);
    float c = (float)cos(a);
    pos.x = m_Pos.z * s + m_Pos.x * c;
    pos.z = m_Pos.z * c - m_Pos.x * s;
    const EXVector &o = m_pFormation->m_NavLoc.Pos();
    EXVector t(o);
    pos = EXVector(t.x + pos.x, t.y, t.z + pos.z, t.w);
    EXVector from = m_pFormation->GetPos();
    XNavMesh *pMesh = gNavMeshHandler.GetPtr(from, 0);
    if (pMesh) {
        int poly = -1;
        pMesh->ValidateLine(m_pFormation->NavLoc(), pos, poly, pos, 0);
    }
    return pos;
}

signed char CFormation::FreeSlot(int id)
{
    int i = ItemNumFromActorID(id);
    if (i >= 0) {
        for (int j = i + 1; j < 32; j++) m_Items[j - 1] = m_Items[j];
        m_Count--;
        m_Items[31].m_bActive = 0;
        m_Items[31].m_ID = -1;
        return 1;
    }
    return 0;
}

signed char CFormation::GetSlotPosAndAngle(int id, EXVector &pos, EXVector &ang)
{
    CFormationItem *p = ItemPtrFromActorID(id, 1);
    if (p) {
        pos = p->GetWorldPos();
        ang = p->GetWorldAng();
        pos.w = 0.0f;
        return 1;
    }
    return 0;
}

signed char CFormation::GetSlotPos(int id, EXVector &pos)
{
    CFormationItem *p = ItemPtrFromActorID(id, 1);
    if (p) {
        pos = p->GetWorldPos();
        pos.w = 0.0f;
        return 1;
    }
    return 0;
}

