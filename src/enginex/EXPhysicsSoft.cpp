#include <float.h>
#include <string.h>
#include "EXPCloth.h"
#include "EXCloth2.h"
#include "EXItem.h"
#include "EXItemAnimator_Entity.h"
#include "EXItemRender.h"
#include "EXGeoComplexRefObject.h"
#include "EXGeoHeader.h"
#include "EXGeoEntity.h"
#include "EXMatrix.h"
#include "EXPhysicsStandIn.h"
#include "EXMemory.h"
#include "RGBA.h"
#include "EXBaseDisplay.h"
#include "EXQuaternion.h"
#include "EXCollisionScoopTemp.h"
#include "EXDatum.h"
#include "EXRand.h"
#include "EXBaseApp.h"
#include "EXScratchPad.h"
#include "EXCollisionType.h"
#include "EXItemAnimator_Anim.h"
#include "EXItemRender_Anim.h"

// TODO stand-in: item environment (EXItemEnv.obj).
class EXItemEnv {
public:
    static EXItemEnv *TheItemEnv() { return m_pTheItemEnv; }
    EXCloth2Manager *ClothManager();
protected:
    static EXItemEnv *m_pTheItemEnv;
};

// TODO layout: cloth extra data of a geo entity (XtraData type 3), geo area.
struct EXGeoEntity_VtxConnEdge {
    u32 m_hash;         // +0x00
    u16 m_i1;           // +0x04
    u16 m_i2;           // +0x06
    float m_min;        // +0x08
    float m_max;        // +0x0c
    float m_rigidity;   // +0x10
};
// A render vertex that duplicates a simulated one (seams): m_dst follows m_src.
struct EXGeoEntity_VtxConnCopy {
    u16 m_src;          // +0x00
    u16 m_dst;          // +0x02
};
// One simulated vertex and the vertices it connects to (TODO names of +4..+8).
struct EXGeoEntity_VtxConn {
    u16 m_numNeighbours;    // +0x00
    u16 m_vertex;           // +0x02
    u8 m_04[5];
    u8 m_flags;             // +0x09 0x40 simulated, 0x80 fixed
    u16 m_neighbours[1];    // +0x0a
};
struct EXPClothXtra {
    u16 m_numVerts;     // +0x00
    u16 m_numCopies;    // +0x02
    u16 m_numEdges;     // +0x04
    u16 m_06;
    ex::EXRelPtr<EXGeoEntity_VtxConnCopy> m_copies;  // +0x08
    ex::EXRelPtr<EXGeoEntity_VtxConnEdge> m_edges;   // +0x0c
    ex::EXRelPtr<EXGeoEntity_VtxConn> m_conns[1];  // +0x10 one per vertex
};

// TODO layout: EXGeoEntity (geo area) keeps a relptr to its source vertices at +0x5c.
static inline EXGeoEntity_VtxData *EntitySourceVerts(const EXGeoEntity *pEnt)
{
    return *(const ex::EXRelPtr<EXGeoEntity_VtxData> *)((const u8 *)pEnt + 0x5c);
}

EX_IMPLEMENT_DYNCREATE(EXPCloth, EXItemPhysicsExt)
EX_IMPLEMENT_DYNCREATE(EXPAttachedClothWS, EXPCloth)
EX_IMPLEMENT_DYNCREATE(EXCloth2Base, EXItemPhysics)
EX_IMPLEMENT_DYNCREATE(EXCloth2, EXCloth2Base)

void EXPParticle::QuickUpdatePosition()
{
    m_pos.w = 0.0f;
    EXVector old = m_pos;
    m_pos = m_pos * (2.0f - m_drag) - m_oldPos * (1.0f - m_drag) + m_accel;
    m_oldPos = old;
}

void EXPParticle::VariableUpdatePosition(float t)
{
    m_pos.w = 0.0f;
    EXVector delta = m_pos * (1.0f - m_drag) - m_oldPos * (1.0f - m_drag) + m_accel * t;
    m_pos = delta + m_pos;
    m_oldPos = m_pos - delta;
}

// NEAR: ~820 bytes differ, the EXVector operators are not inlined in this large function (retail inlines all of them).
float EXPParticle::CheckCollision(const EXVector &p, const EXVector &n, float d, float f)
{
    float result = 0.0f;
    float dist = n.Dot(m_pos - p);
    if (d > dist) {
        float pen = d - dist;
        result = pen;
        m_pos += n * pen;
        if (d > EX_EPSILON_NORMALISE) {
            EXVector vel = m_oldPos - m_pos;
            EXVector tang = vel - n * n.Dot(vel);
            if (tang.LengthXYZSquared() < 1e-6f) {
                m_pos = m_oldPos;
                return result;
            }
            m_pos += tang * f;
        }
    }
    return result;
}

EXPClothEdge::EXPClothEdge()
{
    m_p1 = 0;
    m_p2 = 0;
    m_lengthSq = 0.0f;
    m_rigidity = 1.0f;
}

// NEAR: 48 bytes differ, scheduling of the temporary copy (sum order y,z,x in retail).
void EXPClothEdge::SetParticles(EXPParticle *a, EXPParticle *b)
{
    m_p1 = a;
    m_p2 = b;
    m_lengthSq = (b->GetPosition() - a->GetPosition()).LengthXYZSquared();
    if (m_lengthSq < EX_EPSILON_UNIT) m_lengthSq = EX_EPSILON_UNIT;
}

void EXPClothEdge::Draw(EXWnd *, ex::RGBA, const EXVector &) const {}

EXVector EXPClothEdge::GetVector() const { return m_p1->GetPosition() - m_p2->GetPosition(); }

void EXPVarClothEdge::Set(float fMin, float fMax, float fRigidity, u32 hash)
{
    m_hash = hash;
    float len = Length();
    float lo = len * fMin;
    float hi = len * fMax;
    m_minSq = lo * lo;
    m_maxSq = hi * hi;
    float r = 1.0f - fRigidity;
    m_rigidity = r * r * r;
}

void EXPClothCollisionConstraint::SatisfyConstraints()
{
    float d = m_pParticle->GetPosition().Dot(m_normal) - m_d;
    if (d > 0.0f) m_pParticle->SetPosition(m_pParticle->GetPosition() - m_normal * 0.8f * d);
}

s8 EXPCloth::Initialise() { return 1; }

void EXPCloth::MoveGrabbedVert(const EXVector &v)
{
    if (m_grabbed >= 0) m_pParticles[m_grabbed].AddToPositionAndOld(v);
}

void EXPCloth::UpdateGrabbedVert(const EXVector &v, u32 i)
{
    if (i < m_numParticles) m_pParticles[i].SetPosition(v);
}

void EXPCloth::SetDrag(float f)
{
    for (u32 i = 0; i < m_numParticles; i++) m_pParticles[i].SetDrag(f);
}

void EXPCloth::SetElasticity(float f)
{
    float r = 1.0f - f;
    r = r * r * r;
    for (u32 i = 0; i < m_numEdges; i++) m_pEdges[i].SetRigidity(r);
}

void EXPCloth::SetElasticity(float f, u32 i)
{
    float r = 1.0f - f;
    m_pEdges[i].SetRigidity(r * r * r);
}

s8 EXPAttachedClothWS::Initialise() { return 1; }
s8 EXPAttachedClothWS::DoUpdate() { return 1; }

void EXPAttachedClothWS::SetWind(EXVector v)
{
    m_windRaw = v;
    m_wind = v;
    m_wind.Normalise();
}

void EXCloth2Base::EntityNormals(const EXGeoEntity *, EXGeoEntity_VtxData *, const EXCloth2EntityInfo *) {}

void EXPParticle::Reset()
{
    m_pos.SetZero();
    m_oldPos.SetZero();
    m_accel.SetZero();
    m_drag = 0.0f;
    m_34 = 0.0f;
    m_flags = 0;
}

// NEAR: ~50 bytes differ, store order of the two transformed endpoints.
void EXPClothEdge::Draw(EXWnd *pWnd, const EXMatrix &m) const
{
    EXVector a = m_p1->GetPosition();
    EXVector b = m_p2->GetPosition();
    a.w = 1.0f;
    a *= m;
    b.w = 1.0f;
    b *= m;
}

EXPCloth::~EXPCloth()
{
    if (m_bOwnArrays) {
        EXFree(m_pParticles);
        EXFree(m_pEdges);
    }
    if (m_94) EXFree(m_94);
    if (m_6c) EXFree(m_6c);
    if (m_98) EXFree(m_98);
    m_varEdges.Empty(1);
    m_collisions.Empty(1);
}


void EXPCloth::ActivateExtraEdges(u32 hash)
{
    int n = m_varEdges.Size();
    for (int i = 0; i < n; i++) {
        if (m_varEdges[i].Hash() == hash || !hash) m_varEdges[i].Activate(1);
    }
}

void EXPCloth::DeactivateExtraEdges(u32 hash)
{
    int n = m_varEdges.Size();
    for (int i = 0; i < n; i++) {
        if (m_varEdges[i].Hash() == hash || !hash) m_varEdges[i].Activate(0);
    }
}

s8 EXPCloth::DoUpdate()
{
    if (!m_bOwnArrays) return 0;
    if (m_pItem->ClipFlags() & 2) return 0;
    if (!(m_physicsFlags & 0x20)) {
        u32 iter = m_iterations;
        m_c8 = m_cc * EXBaseApp::GameFrameRateMul();
        m_iterations = (u32)((float)(int)iter * m_c8);
        if (m_iterations < 1) m_iterations = 1;
        s8 ok = VerletUpdate(m_c8);
        m_iterations = iter;
        if (!ok) return 0;
    }
    EXItem *pItem = m_pItem;
    if (!pItem) return 0;
    EXItemAnimator_Entity *pAnim = (EXItemAnimator_Entity *)pItem->GetAnimator();
    if (!pAnim->GeoRefObject().m_pGeoFile) return 0;
    if (!*(void **)((u8 *)pAnim->GeoRefObject().m_pGeoFile + 0x20)) return 0;  // TODO layout: EXGeoFile header
    EXGeoEntityHeader *pHdr = pAnim->GeoRefObject().GeoEntityHeader();
    if (!pHdr) return 0;
    EXGeoEntity *pEnt = pHdr->m_Addr[0].m_pObject;
    if (!pEnt) return 0;
    SyncWithVerts(pEnt, (EXGDiData *)pAnim->GDiDataArray(0).m_00);
    return 1;
}

void EXPCloth::PauseSync()
{
    if (!m_bOwnArrays) return;
    if (m_pItem->ClipFlags() & 2) return;
    EXItem *pItem = m_pItem;
    if (!pItem) return;
    EXItemAnimator_Entity *pAnim = (EXItemAnimator_Entity *)pItem->GetAnimator();
    if (!pAnim->GeoRefObject().m_pGeoFile) return;
    if (!*(void **)((u8 *)pAnim->GeoRefObject().m_pGeoFile + 0x20)) return;  // TODO layout: EXGeoFile header
    EXGeoEntityHeader *pHdr = pAnim->GeoRefObject().GeoEntityHeader();
    if (!pHdr) return;
    EXGeoEntity *pEnt = pHdr->m_Addr[0].m_pObject;
    if (!pEnt) return;
    SyncWithVerts(pEnt, (EXGDiData *)pAnim->GDiDataArray(0).m_00);
}

// NEAR: ~57 bytes differ, esi/edi swapped for the vector pointer and best index.
void EXPCloth::Grab(const EXVector &v)
{
    float best = 3.402823466e+38f;
    u32 bi = 0;
    for (u32 i = 0; i < m_numParticles; i++) {
        float d = (m_pParticles[i].GetPosition() - v).LengthXYZSquared();
        if (d < best) {
            best = d;
            bi = i;
        }
    }
    if (m_grabbed >= 0) m_pParticles[m_grabbed].Flags().BitClr(3);
    m_grabbed = bi;
    m_pParticles[bi].Flags().BitSet(3);
}

void EXPCloth::ReleaseGrabbedVert()
{
    if (m_grabbed >= 0) m_pParticles[m_grabbed].Flags().BitClr(3);
    m_grabbed = -1;
}

void EXPCloth::GrabVerts(u32 n, const u32 *p)
{
    if (!n) return;
    for (u32 i = 0; i < n; i++) m_pParticles[p[i]].Flags().BitSet(3);
}

void EXPCloth::ReleaseGrabbedVerts(u32 n, const u32 *p)
{
    for (u32 i = 0; i < n; i++) m_pParticles[p[i]].Flags().BitClr(3);
}

void EXPCloth::RenderModifier_Callback(u32 cmd, void *p)
{
    switch (cmd) {
    case 1:
        if (p) {
            EXItemPhysics *pPhys = ((EXItem *)((EXItemRender *)p)->GetBaseOwner())->ItemPhysics();
            if (pPhys && pPhys->GetRuntimeClass()->IsDerivedFrom(EXRUNTIME_CLASS(EXPCloth))) ((EXPCloth *)pPhys)->VerletUpdate(-1.0f);
        }
        break;
    }
}

// NEAR: register choice (this in ecx vs esi) and temporary vector copy scheduling differ.
void EXCloth2Base::UpdatePositions(float t)
{
    float ratio = t / m_50;
    int n = m_particles.Size();
    for (int i = 0; i < n; i++) {
        s8 bFree = m_particles[i].Flags() == 0;
        if (!(bFree & 1)) continue;
        EXVector old = m_particles[i].OldPosition();
        m_particles[i].SetOldPosition(m_particles[i].Position());
        EXVector a = m_particles[i].Acceleration() * t * t;
        EXVector v = (m_particles[i].Position() - old) * ((1.0f - m_particles[i].Drag()) * ratio);
        EXVector np = m_particles[i].Position() + v + a;
        m_particles[i].SetPosition(np);
        m_particles[i].m_pos.w = 0.0f;
    }
}

// NEAR: 850 vs 784 bytes; vector temporaries for the position updates are spilled differently.
void EXCloth2Base::SatisfyConstraints(u32 n)
{
    int nEdges = m_edges.Size();
    for (u32 it = 0; it < n; it++) {
        for (int e = 0; e < nEdges; e++) {
            u32 r2 = m_edges[e].Ref2();
            s8 b2 = (s8)(m_particles[r2].Flags() & 1);
            u32 r1 = m_edges[e].Ref1();
            s8 b1 = (s8)(m_particles[r1].Flags() & 1);
            if (b1 && b2) continue;
            EXVector d = m_particles[r2].Position() - m_particles[r1].Position();
            float k = (m_edges[e].LengthSq() / (d.LengthXYZSquared() + m_edges[e].LengthSq()) - 0.5f) * m_edges[e].Rigidity();
            d *= k;
            if (b1) {
                d *= 2.0f;
                m_particles[r2].AddToPosition(d);
            } else if (b2) {
                d *= 2.0f;
                m_particles[r1].AddToPosition(-d);
            } else {
                m_particles[r1].AddToPosition(-d);
                m_particles[r2].AddToPosition(d);
            }
        }
    }
}

void EXCloth2Base::PhysicsUpdate(float t)
{
    UpdatePositions(t);
    SatisfyConstraints((u32)(t * m_54));
    m_50 = t;
}

void EXCloth2Base::Syncronise(EXGeoEntity *pEnt, EXGDiData *pGDi, const EXMatrix &m)
{
    int n = m_entities.Size();
    if (pEnt->IsKindOf(EXRUNTIME_CLASS(EXGeoSplitEntity))) {
        for (int i = 0; i < n; i++) {
            // TODO layout: EXGeoSplitEntity (geo area) keeps relptrs to its pieces at +0x5c.
            ex::EXRelPtr<EXGeoEntity> *pList = (ex::EXRelPtr<EXGeoEntity> *)((u8 *)pEnt + 0x5c);
            SyncroniseEntity(pList[i], pGDi, m_entities[i], m);
        }
    } else {
        SyncroniseEntity(pEnt, pGDi, m_entities[0], m);
    }
}

void EXCloth2::GetEntity(EXGeoEntity *&pEnt, EXGDiData *&pGDi) const
{
    EXItemAnimator_Entity *pAnim = (EXItemAnimator_Entity *)m_pItem->GetAnimator();
    EXGeoEntityHeader *pHdr = pAnim->GeoRefObject().GeoEntityHeader();
    pEnt = pHdr ? pHdr->m_Addr[0].m_pObject : 0;
    pGDi = (EXGDiData *)pAnim->GDiDataArray(0).m_00;
}

void EXCloth2::DoStep(float t)
{
    PhysicsUpdate(t);
    EXGeoEntity *pEnt;
    EXGDiData *pGDi;
    GetEntity(pEnt, pGDi);
    EXMatrix m = EXMatrix::MakeIdentity();
    Syncronise(pEnt, pGDi, m);
}

void EXCloth2Manager::RemoveCloth(const EXCloth2 *p)
{
    for (int i = 0; i < m_cloths.Size(); i++) {
        if (m_cloths[i] == p) m_cloths.RemoveAt(i, 1);
    }
}

void EXCloth2Manager::DoStep(float t)
{
    for (int i = 0; i < m_cloths.Size(); i++) m_cloths[i]->DoStep(t);
}

void EXPCloth::DoCollision(EXItemPhysics *pOther, EXCScoop_FaceData *pData)
{
    EXItem *pItem = m_pItem;
    if (!pItem) return;
    EXItemAnimator *pAnim = pItem->GetAnimator();
    if (!pAnim->GeoRefObject().m_pGeoFile) return;
    if (!*(void **)((u8 *)pAnim->GeoRefObject().m_pGeoFile + 0x20)) return;  // TODO layout: EXGeoFile header
    EXGeoEntityHeader *pHdr = pAnim->GeoRefObject().GeoEntityHeader();
    if (!pHdr) return;
    EXGeoEntity *pEnt = pHdr->m_Addr[0].m_pObject;
    if (!pEnt) return;
    DoCollision(pEnt, pData);
}

s8 EXCloth2Base::PhysicsInitialise(const EXGeoEntity *pEnt)
{
    if (!pEnt->IsKindOf(EXRUNTIME_CLASS(EXGeoSplitEntity))) {
        if (!InitialiseEntity(pEnt)) return 0;
    } else {
        for (u32 i = 0; i < ((const EXGeoSplitEntity *)pEnt)->EntityCount(); i++) {
            // TODO layout: EXGeoSplitEntity (geo area): piece count at +0x54, relptrs at +0x5c.
            const ex::EXRelPtr<EXGeoEntity> *pList = (const ex::EXRelPtr<EXGeoEntity> *)((const u8 *)pEnt + 0x5c);
            if (!InitialiseEntity(pList[i])) return 0;
        }
    }
    return 1;
}

EXCloth2Base::~EXCloth2Base()
{
    int n = m_entities.Size();
    for (int i = 0; i < n; i++) {
        EXCloth2EntityInfo *pInfo = m_entities[i];
        if (pInfo->m_pData) EXFree(pInfo->m_pData);
        EXFree(pInfo);
    }
    m_entities.RemoveAll();
}

void EXCloth2Manager::AddCloth(EXCloth2 *p) { m_cloths.Add(p); }

EXPParticle::EXPParticle() { Reset(); }

s8 EXPCloth::InitialiseExtraEdges(const EXGeoEntity *pEnt)
{
    EXPClothXtra *pXtra = (EXPClothXtra *)pEnt->XtraData(3);
    if (!pXtra) return 0;
    EXGeoEntity_VtxData *pVerts = EntitySourceVerts(pEnt);
    for (u32 i = 0; i < pXtra->m_numEdges; i++) {
        EXPVarClothEdge edge;
        EXGeoEntity_VtxConnEdge *pEdges = pXtra->m_edges;
        EXGeoEntity_VtxConnEdge &e = pEdges[i];
        float lenSq = ((const EXVector &)pVerts[e.m_i1] - (const EXVector &)pVerts[e.m_i2]).LengthXYZSquared();
        edge.m_p1 = &m_pParticles[e.m_i1];
        edge.m_p2 = &m_pParticles[e.m_i2];
        edge.m_lengthSq = lenSq;
        edge.Set(e.m_min, e.m_max, e.m_rigidity, e.m_hash);
        m_varEdges.Add(edge, 1);
    }
    return 1;
}

s8 EXPCloth::SplitEntityInitialise(const EXGeoSplitEntity *pSplit, u8 flags)
{
    u32 total = 0;
    m_b8 = pSplit->EntityCount();
    for (u32 i = 0; i < m_b8; i++) {
        // TODO layout: EXGeoSplitEntity (geo area) keeps relptrs to its pieces at +0x5c.
        const ex::EXRelPtr<EXGeoEntity> *pList = (const ex::EXRelPtr<EXGeoEntity> *)((const u8 *)pSplit + 0x5c);
        EXPClothXtra *pXtra = (EXPClothXtra *)((const EXGeoEntity *)pList[i])->XtraData(3);
        if (!pXtra) return 0;
        total += pXtra->m_numVerts;
    }
    m_numParticles = total;
    if (!total) return 0;
    m_pParticles = new EXPParticle[total];
    return 1;
}

s8 EXPCloth::ClothInit(u8 flags)
{
    EXItem *pItem = m_pItem;
    if (!pItem) return 0;
    EXItemAnimator *pAnim = pItem->GetAnimator();
    if (!pAnim->GeoRefObject().m_pGeoFile) return 0;
    if (!*(void **)((u8 *)pAnim->GeoRefObject().m_pGeoFile + 0x20)) return 0;
    EXGeoEntityHeader *pHdr = pAnim->GeoRefObject().GeoEntityHeader();
    if (!pHdr) return 0;
    EXGeoEntity *pEnt = pHdr->m_Addr[0].m_pObject;
    if (!pEnt) return 0;
    return Initialise(pEnt, flags);
}

void EXPCloth::DoClothCollision(EXCollisionType *pCol, u32 n)
{
    EXItem *pItem = m_pItem;
    if (!pItem) return;
    EXItemAnimator_Entity *pAnim = (EXItemAnimator_Entity *)pItem->GetAnimator();
    if (!pAnim) return;
    if (!pAnim->GeoRefObject().m_pGeoFile) return;
    if (!*(void **)((u8 *)pAnim->GeoRefObject().m_pGeoFile + 0x20)) return;  // TODO layout: EXGeoFile header
    EXGeoEntityHeader *pHdr = pAnim->GeoRefObject().GeoEntityHeader();
    if (!pHdr) return;
    EXGeoEntity *pEnt = pHdr->m_Addr[0].m_pObject;
    if (!pEnt) return;
    SyncWithVerts(pEnt, (EXGDiData *)pAnim->GDiDataArray(0).m_00);
    DoClothCollision(m_pItem, pCol, n, pEnt, (EXGDiData *)pAnim->GDiDataArray(0).m_00);
}

// NEAR: 56 bytes differ, retail tests pEnt straight from edi (register shuffle around the null tests).
void EXPAttachedClothWS::DoClothCollision(EXCollisionType *pCol, u32 n)
{
    EXGDiData *pGDi = 0;
    EXGeoBaseEntity *pTmp = 0;
    GetEntity(pTmp, pGDi);
    EXGeoBaseEntity *pEnt = pTmp;
    if (!pEnt) return;
    if (!pGDi) return;
    SyncWithVerts(pEnt, pGDi);
    EXPCloth::DoClothCollision((EXItem *)m_11c, pCol, n, pEnt, pGDi);
}

void EXPAttachedClothWS::DoCollision(EXItemPhysics *pOther, EXCScoop_FaceData *pData)
{
    EXGeoBaseEntity *pEnt;
    EXGDiData *pGDi;
    pGDi = 0;
    pEnt = 0;
    GetEntity(pEnt, pGDi);
    if (pEnt && pGDi) EXPCloth::DoCollision((EXGeoEntity *)pEnt, pData);
}

// NEAR: ~51 bytes differ, member initialisation store order.
EXPCloth::EXPCloth()
{
    m_bc = 0.001087f;
    m_c0 = 0.5f;
    m_bOwnArrays = 0;
    m_iterations = 4;
    m_windRaw.Set(0.0f, 0.0f, -0.002f, 0.0f);
    m_numParticles = 0;
    m_numEdges = 0;
    m_9c = 0;
    m_90 = 0;
    m_94 = 0;
    m_6c = 0;
    m_d6 = 0;
    m_c4 = 0.0f;
    m_d4 = 1;
    m_grabbed = -1;
    m_c8 = 0.0f;
    m_cc = 1.0f;
    m_clothFlags = 0;
    m_wind.SetZero();
    m_98 = 0;
    m_b4 = 0;
    m_b8 = 0;
}

// NEAR: 9 bytes differ, ecx/eax and vptr store position.
EXPAttachedClothWS::EXPAttachedClothWS()
{
    m_120 = 0;
    m_118 = 0;
    m_124 = 2;
m_11c = 0;
    m_128 = 0.0f;
    m_clothFlags.BitSet(2);
}

void EXCloth2::Initialise(u8 flags)
{
    m_flags = flags;
    EXGeoEntityHeader *pHdr = m_pItem->GetAnimator()->GeoRefObject().GeoEntityHeader();
    PhysicsInitialise(pHdr ? pHdr->m_Addr[0].m_pObject : 0);
    EXItemEnv::TheItemEnv()->ClothManager()->AddCloth(this);
}

// Retail keeps only the transforms of the two end points; the drawing itself is compiled out.
void EXPVarClothEdge::Debug_DrawConstraint(const EXVector *pPos, const EXQuaternion *pRot) const
{
    if (!m_bActive || !EXBaseDisplay::ActiveWnd() || !pPos || !pRot) return;
    EXMatrix m;
    m.set_rotate_quat(*pRot);
    EXMaths::vec_xyzw v;
    D3DXVec4Transform(&v, &m_p1->m_pos, &m);
    D3DXVec4Transform(&v, &m_p2->m_pos, &m);
}

// The entity the cloth drapes: the first entity of the attached item (m_11c) when there is one,
// else the entity attached to bone m_120 of the owner's (or m_118's) skin.
void EXPAttachedClothWS::GetEntity(EXGeoBaseEntity *&pEnt, EXGDiData *&pGDi)
{
    if (m_11c) {
        EXItemAnimator_Entity *pAnim = (EXItemAnimator_Entity *)((EXItem *)m_11c)->GetAnimator();
        if (!pAnim) return;
        if (!pAnim->GeoRefObject().m_pGeoFile) return;
        if (!*(void **)((u8 *)pAnim->GeoRefObject().m_pGeoFile + 0x20)) return;  // TODO layout: EXGeoFile header
        EXGeoEntityHeader *pHdr = pAnim->GeoRefObject().GeoEntityHeader();
        pEnt = pHdr ? pHdr->m_Addr[0].m_pObject : 0;
        pGDi = (EXGDiData *)pAnim->GDiDataArray(0).m_00;
        return;
    }
    EXItem *pItem = (EXItem *)m_118;
    if (!pItem) pItem = m_pItem;
    EXItemAnimator_Anim *pAnim = (EXItemAnimator_Anim *)pItem->GetAnimator();
    if (!pAnim) return;
    EXItemRender_SkinAnim *pSkin = pAnim->m_pSkin;
    EXItemRender_SkinAnimSection *pSec = pSkin->GetAnimBoneSection(m_120, 0);
    if (pSec) pEnt = pSkin->FindAttachedEntity(pSec->m_bone, pGDi, 0);
}

// Wind is given in world space: keep its direction and its value in the item's frame, and only
// blow when that is not negligible.
void EXPCloth::SetWind(EXVector v)
{
    m_windRaw = v;
    m_wind = v;
    m_wind.Normalise();
    EXMaths::quat_xyzw q;
    if (m_pItem->m_rotOrder == 0)
        q.set(m_pItem->Rotation());
    else
        q.SetRotation(m_pItem->Rotation(), m_pItem->m_rotOrder);
    (EXMaths::vec_xyzw &)m_windRaw = EXQuaternion(-q.x, -q.y, -q.z, q.w).MultiplyVector(EXMaths::vec_xyzw(m_windRaw));
    if (m_windRaw.LengthXYZSquared() < 1e-6f)
        m_d4 = 0;
    else
        m_d4 = 1;
}

// Scoop the faces of the entity pEnt (placed by pItem) that the collision shapes reach and run the
// cloth against each of them.
void EXPCloth::DoClothCollision(EXItem *pItem, EXCollisionType *pCol, u32 n, EXGeoBaseEntity *pEnt, EXGDiData *pGDi)
{
    EXCollisionScoopTemp temp;
    temp.Initialise();
    temp.m_xform.SetIdentity();
    temp.m_numObjects = n;
    temp.m_pObjects = pCol;
    temp.CalcBounds(temp.m_xform, 4);
    temp.SetTemporaryVars(pItem, pItem->m_mtx, 0);
    pEnt->DoEntityScoop_Sphere(*(EXCScoopTemp *)&temp, pGDi, 0, 0);
    temp.CalculateFaceArray();
    if (temp.m_numFaces) {
        for (u32 i = 0; i < temp.m_numFaces; i++) {
            EXCScoop_FaceData *pFace = &temp.m_pFaces[i];
            pFace->m_flags = pFace->m_flags & ~1;
            DoCollision((EXGeoEntity *)pEnt, pFace);
        }
    }
}

static inline float ScoreAbs(float f) { return f < 0.0f ? 0.0f - f : f; }

// How well two edges line up with each other and with the direction v (used to pick the edges
// that carry grab and bend constraints): the sum of the absolute cosines.
float EXPCloth::ScoreEdges(int a, int b, const EXVector &v) const
{
    EXVector e1 = m_pEdges[a].GetVector().UnitVectorXYZ();
    EXVector e2 = m_pEdges[b].GetVector().UnitVectorXYZ();
    float s = ScoreAbs(e2.Dot(e1));
    s += ScoreAbs(v.Dot(e1));
    return ScoreAbs(v.Dot(e2)) + s;
}

// A scooped face of a collision entity pushes the cloth particles of the face's vertices out along
// the face normal (taken into the item's frame unless the cloth simulates in world space). Faces
// touching a fixed particle are ignored.
void EXPCloth::DoCollision(EXGeoEntity *pEnt, EXCScoop_FaceData *pFace)
{
    if (!pEnt->HasXtraData(3)) return;
    EXPClothXtra *pXtra = (EXPClothXtra *)pEnt->XtraData(3);
    u32 idx[3];
    idx[0] = pFace->idx0();
    idx[1] = pFace->idx1();
    idx[2] = pFace->idx2();
    u32 i;
    for (i = 0; i < 3; i++) {
        u32 v = idx[i];
        if (v > m_numParticles - 1) {
            for (u32 j = 0; j < pXtra->m_numCopies; j++) {
                EXGeoEntity_VtxConnCopy *pCopies = pXtra->m_copies;
                if (pCopies[j].m_dst == v) {
                    EXGeoEntity_VtxConnCopy *pc = pXtra->m_copies;
                    v = pc[j].m_src;
                }
            }
            idx[i] = v;
        }
    }
    for (i = 0; i < 3; i++)
        if (m_pParticles[idx[i]].m_flags & 1) return;
    EXVector n = (pFace->P0() - pFace->P1()).UnitVectorXYZ();
    if (!(m_clothFlags & 2)) {
        EXMaths::quat_xyzw q;
        if (m_pItem->m_rotOrder == 0)
            q.set(m_pItem->Rotation());
        else
            q.SetRotation(m_pItem->Rotation(), m_pItem->m_rotOrder);
        (EXMaths::vec_xyzw &)n = EXQuaternion(-q.x, -q.y, -q.z, q.w).MultiplyVector(EXMaths::vec_xyzw(n));
    }
    for (i = 0; i < 3; i++) {
        EXPClothCollisionConstraint c;
        EXPParticle *p = &m_pParticles[idx[i]];
        c.Set(n, p, p->m_pos.Dot(n) - pFace->m_dist);
        m_collisions.Add(c, 1);
    }
}

// EXPParticle::ApplyXForm, which retail inlines here (it stays out of line elsewhere in this unit).
static __forceinline void ParticleApplyXForm(EXPParticle &p, const EXMatrix &m, s8 bOld)
{
    p.m_pos.ApplyXForm(m);
    if (bOld) p.m_oldPos.ApplyXForm(m);
}

// Build the cloth from the attached entity and move its particles from the attachment frame (the
// datum m_120 of the owner, or the owner itself) into world space, keeping the inverse frame.
s8 EXPAttachedClothWS::Initialise(u32 hash, u8 flags)
{
    EXItem *pItem = (EXItem *)m_118;
    if (!pItem) pItem = m_pItem;
    m_120 = hash;
    EXGDiData *pGDi = 0;
    EXGeoBaseEntity *pEnt = 0;
    GetEntity(pEnt, pGDi);
    if (!pEnt || !pGDi) return 0;
    SetWind(EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    if (!EXPCloth::Initialise((const EXGeoEntity *)pEnt, flags)) return 0;
    EXMatrix m;
    EXDatumTemp d;
    if (m_120 && pItem->GetAnimatorDatum(m_120, d, 0, 0)) {
        m.set_rotate_quat(d.m_rotation);
        m.r[3].x = d.m_position.x;
        m.r[3].y = d.m_position.y;
        m.r[3].z = d.m_position.z;
    } else {
        m.set_rotate(pItem->m_rotation, pItem->m_rotOrder);
        float z = pItem->m_position.z, y = pItem->m_position.y;
        m.r[3].x = pItem->m_position.x;
        m.r[3].y = y;
        m.r[3].z = z;
    }
    m.r[3].w = 1.0f;
    m_invMtx = m.FastInverse();
    for (u32 i = 0; i < m_numParticles; i++) {
        EXPParticle &p = m_pParticles[i];
        if (p.m_flags.BitTest(1))
            ParticleApplyXForm(p, m, 0);
        else
            ParticleApplyXForm(p, m, 1);
    }
    return 1;
}

// Write the particles of one entity into its vertex buffer through the entity's vertex map
// (0x7fffffff: vertex not simulated), then the normals packed 11:11:10. Retail clears and
// renormalises the normal array without accumulating face normals in between.
void EXCloth2Base::SyncroniseEntity(EXGeoEntity *pEnt, EXGDiData *pGDi, const EXCloth2EntityInfo *pInfo,
                                    const EXMatrix &m)
{
    EXGeoEntity_VtxData *pVerts = (EXGeoEntity_VtxData *)pEnt->GetVertexBuffer(pGDi, 1);
    u32 i;
    for (i = 0; i < pInfo->m_00; i++) {
        u32 p = ((const u32 *)pInfo->m_pData)[i];
        if (p == 0x7fffffff) continue;
        EXVector v = m_particles.m_pData[p].m_pos;
        v.ApplyXForm(m);
        EXVector3 &dst = *(EXVector3 *)((u8 *)pVerts + i * 0x18);
        dst.x = v.x;
        dst.y = v.y;
        dst.z = v.z;
    }
    int n = m_20.Size();
    memset(m_20.m_pData, 0, n * sizeof(EXVector));
    for (int k = 0; k < n; k++) {
        m_20.m_pData[k].w = 0.0f;
        m_20.m_pData[k].Normalise();
    }
    for (i = 0; i < pInfo->m_00; i++) {
        u32 p = ((const u32 *)pInfo->m_pData)[i];
        if (p == 0x7fffffff) continue;
        const EXVector &nrm = m_20.m_pData[p];
        *(s32 *)((u8 *)pVerts + i * 0x18 + 0xc) =
            ((((int)(nrm.y * 1023.0f) & 0x7ff) | ((int)(nrm.z * 511.0f) << 11)) << 11) | ((int)(nrm.x * 1023.0f) & 0x7ff);
    }
    pEnt->ReleaseVertexBuffer(pGDi);
}

// Map the simulated vertices of one entity to particles (vertices at the same place share one),
// add a particle for each new position (fixed when the vertex is flagged so) and an edge for each
// connection to a higher simulated vertex.
s8 EXCloth2Base::InitialiseEntity(const EXGeoEntity *pEnt)
{
    if (!pEnt->HasXtraData(3)) return 0;
    EXPClothXtra *pXtra = (EXPClothXtra *)pEnt->XtraData(3);
    if (!pXtra) return 0;
    EXGeoEntity_VtxData *pVerts = EntitySourceVerts(pEnt);
    if (!pVerts) return 0;
    u32 n = pXtra->m_numVerts;
    if (!n) return 1;
    EXCloth2EntityInfo *pInfo = (EXCloth2EntityInfo *)_EXAlloc(8, 0);
    pInfo->m_pData = _EXAlloc(n * 4, 0);
    pInfo->m_00 = n;
    m_entities.Add(pInfo);
    u32 i;
    for (i = 0; i < n; i++) {
        EXGeoEntity_VtxConn *pConn = pXtra->m_conns[i];
        u32 v = pConn->m_vertex;
        if (!(pConn->m_flags & 0x40)) {
            ((u32 *)pInfo->m_pData)[v] = 0x7fffffff;
            continue;
        }
        if (v > n - 1) {
            EXFree(pInfo->m_pData);
            EXFree(pInfo);
            return 0;
        }
        const EXVector &pos = *(const EXVector *)((u8 *)pVerts + v * 0x18);
        EXVector p = pos;
        int j = 0;
        int last = m_particles.Size() - 1;
        for (; j < last; j++) {
            const EXVector &q = m_particles.m_pData[j].m_pos;
            if ((p.y - q.y) * (p.y - q.y) + (p.z - q.z) * (p.z - q.z) + (p.x - q.x) * (p.x - q.x) < 1.7e-13f) {
                ((u32 *)pInfo->m_pData)[v] = j;
                goto next;
            }
        }
        {
            EXCloth2Particle &np = m_particles.AddSpace(1);
            np.Init();
            np.SetFullPosition(p);
            if (pConn->m_flags & 0x80) np.m_flags = np.m_flags | 1;
            ((u32 *)pInfo->m_pData)[pConn->m_vertex] = m_particles.Size() - 1;
        }
    next:;
    }
    m_20.SetSize(m_particles.Size(), -1);
    for (i = 0; i < n; i++) {
        EXGeoEntity_VtxConn *pConn = pXtra->m_conns[i];
        if (((u32 *)pInfo->m_pData)[pConn->m_vertex] == 0x7fffffff) continue;
        for (u32 k = 0; k < pConn->m_numNeighbours; k++) {
            u32 b = pConn->m_neighbours[k];
            if (b > pConn->m_vertex && ((u32 *)pInfo->m_pData)[b] != 0x7fffffff) {
                EXCloth2Edge &e = m_edges.AddSpace(1);
                e.Init();
                e.m_ref1 = ((u32 *)pInfo->m_pData)[pConn->m_vertex];
                e.m_ref2 = ((u32 *)pInfo->m_pData)[b];
                const EXVector &a = m_particles.m_pData[e.m_ref1].m_pos;
                const EXVector &c = m_particles.m_pData[e.m_ref2].m_pos;
                float dx = a.x - c.x, dy = a.y - c.y, dz = a.z - c.z;
                e.m_lengthSq = dy * dy + dz * dz + dx * dx;
            }
        }
    }
    return 1;
}

static inline float VerletAbs(float f) { return f < 0.0f ? 0.0f - f : f; }

// One Verlet step: integrate the free particles, relax the edge, variable edge and collision
// constraints m_iterations times (pushing particles out of the collision shapes on the first and
// last pass), then set each particle's acceleration for the next step from gravity and the wind
// (gusting m_c0; per mode m_d6: random, by the face normal of two edges, by the vertex normal, or
// full strength), and grow the edges towards their targets while m_b4 counts down.
s8 EXPCloth::VerletUpdate(float t)
{
    EXMatrix inv;
    inv.SetIdentity();
    if (m_clothFlags & 1) inv = m_pItem->m_mtx.FastInverse();
    if (!m_bOwnArrays) return 0;
    EXScratchPad sp;
    u32 i;
    for (i = 0; i < m_numParticles; i++) {
        EXPParticle &p = m_pParticles[i];
        if (p.m_flags & 1) continue;
        if (t < 0.0f)
            p.QuickUpdatePosition();
        else
            p.VariableUpdatePosition(t);
    }
    int nVar = m_varEdges.Size();
    EXPClothEdge *pEdges = m_pEdges;
    int nCol = m_collisions.Size();
    for (u32 it = 0; it < m_iterations; it++) {
        u32 j;
        for (j = 0; j < m_numEdges; j++) pEdges[j].SatisfyConstraints();
        int k;
        for (k = 0; k < nVar; k++) m_varEdges.m_pData[k].SatisfyConstraints();
        for (k = 0; k < nCol; k++) m_collisions.m_pData[k].SatisfyConstraints();
        if (m_9c && (it == 0 || it == m_iterations - 1)) {
            for (j = 0; j < m_numParticles; j++) {
                if (m_pParticles[j].m_flags & 1) continue;
                for (u32 c = 0; c < m_9c; c++) {
                    EXCollisionType ct = ((EXCollisionType *)m_90)[c];
                    if (m_clothFlags & 1) ct.ApplyXForm(inv);
                    m_pParticles[j].KeepAwayFrom(ct, 1);
                }
            }
        }
    }
    if (nCol) m_collisions.m_nSize = 0;
    if (m_d4) {
        m_c0 += (EXRandFastf(1.0f) - 0.5f) * 0.05f;
        if (m_c0 > 1.0f) m_c0 = 1.0f;
        if (0.5f > m_c0) m_c0 = 0.5f;
    }
    for (i = 0; i < m_numParticles; i++) {
        if (!m_d4) {
            m_pParticles[i].m_accel.Set(0.0f, 0.0f - m_bc, 0.0f, 0.0f);
            continue;
        }
        if (m_d6 == 0) {
            float r = EXRandFastf(1.0f) - m_c4;
            m_pParticles[i].m_accel = EXVector(0.0f, 0.0f - m_bc, 0.0f, 0.0f) + m_windRaw * m_c0 * r;
            continue;
        }
        float f;
        if (m_d6 == 1) {
            const int *pPair = &((const int *)m_94)[i * 2];
            EXVector n = pEdges[pPair[0]].GetVector().Cross(pEdges[pPair[1]].GetVector());
            n.Normalise();
            f = VerletAbs(n.Dot(m_wind));
        } else if (m_d6 == 2) {
            f = VerletAbs(m_wind.Dot(((const EXVector *)m_6c)[i]));
        } else {
            f = 1.0f;
        }
        m_pParticles[i].m_accel = EXVector(0.0f, 0.0f - m_bc, 0.0f, 0.0f) + m_windRaw * f;
    }
    if (m_b4) {
        for (i = 0; i < m_numEdges; i++) {
            float len = EXSqrt(m_pEdges[i].m_lengthSq) + ((float *)m_98)[i];
            m_pEdges[i].m_lengthSq = len * len;
        }
        m_b4--;
    }
    return 1;
}


// EXGeoEntity::SetVertexNormal / GetVertexNormal, which retail inlines in the vertex syncs (they
// stay out of line elsewhere in this unit).
#define PackNormal(v, n) \
    ((v).m_Normal = ((((int)((n).y * 1023.0f) & 0x7ff) | ((int)((n).z * 511.0f) << 11)) << 11) | ((int)((n).x * 1023.0f) & 0x7ff))
static __forceinline EXVector3 UnpackNormal(const EXGeoEntity_VtxData &v)
{
    s32 n = v.m_Normal;
    s32 x = (n << 21) >> 21;
    s32 y = (n << 10) >> 21;
    s32 z = n >> 22;
    return EXVector3((float)x * (1.0f / 1023.0f), (float)y * (1.0f / 1023.0f), (float)z * (1.0f / 511.0f));
}

// Copy the particle positions into the entity's vertex buffer, with face normals from the two
// edges recorded per particle (mode 1) or vertex normals from FindNormals (mode 2), then refresh
// the seam copies.
void EXPCloth::SyncWithVerts(EXGeoBaseEntity *pBase, EXGDiData *pGDi)
{
    EXGeoEntity *pEnt = (EXGeoEntity *)pBase;
    if (!m_bOwnArrays) return;
    if (!pEnt->HasXtraData(3)) return;
    EXPClothXtra *pXtra = (EXPClothXtra *)pEnt->XtraData(3);
    if (!pGDi) return;
    EXGeoEntity_VtxData *pVerts = (EXGeoEntity_VtxData *)pEnt->GetVertexBuffer(pGDi, 1);
    if (!pVerts) return;
    u32 i;
    for (i = 0; i < pXtra->m_numVerts; i++) {
        EXGeoEntity_VtxConn *pConn = pXtra->m_conns[i];
        EXGeoEntity_VtxData &v = pVerts[pConn->m_vertex];
        v.m_x = m_pParticles[i].m_pos.x;
        v.m_y = m_pParticles[i].m_pos.y;
        v.m_z = m_pParticles[i].m_pos.z;
        if (m_d6 == 1) {
            const int *pPair = &((const int *)m_94)[i * 2];
            EXVector n = m_pEdges[pPair[0]].GetVector().Cross(m_pEdges[pPair[1]].GetVector());
            n.Normalise();
            PackNormal(v, n);
        }
    }
    if (m_d6 == 2) {
        FindNormals(pEnt, pVerts);
        for (i = 0; i < pXtra->m_numVerts; i++) {
            EXGeoEntity_VtxConn *pConn = pXtra->m_conns[i];
            PackNormal(pVerts[pConn->m_vertex], (const EXVector3 &)((EXVector *)m_6c)[pConn->m_vertex]);
        }
    }
    for (i = 0; i < pXtra->m_numCopies; i++) {
        EXGeoEntity_VtxConnCopy *pc = pXtra->m_copies;
        u32 src = pc[i].m_src;
        EXGeoEntity_VtxConnCopy *pd = pXtra->m_copies;
        EXGeoEntity_VtxData &d = pVerts[pd[i].m_dst];
        d.m_x = pVerts[src].m_x;
        d.m_y = pVerts[src].m_y;
        d.m_z = pVerts[src].m_z;
        d.m_Normal = pVerts[src].m_Normal;
        if (m_d6) PackNormal(d, UnpackNormal(pVerts[src]));
    }
    pEnt->ReleaseVertexBuffer(pGDi);
}

// Rebuild the particles and edges from the entity's rest pose while keeping the current motion.
// With a blend time f the edges then grow from their old lengths to the new ones over m_b4 steps.
void EXPCloth::ResetClothLengths(const EXGeoEntity *pEnt, float f)
{
    EXPClothXtra *pXtra = (EXPClothXtra *)pEnt->XtraData(3);
    EXGeoEntity_VtxData *pVerts = EntitySourceVerts(pEnt);
    EXPParticle *pOld = m_pParticles;
    m_pParticles = new EXPParticle[pXtra->m_numVerts];
    u32 i;
    for (i = 0; i < pXtra->m_numVerts; i++) {
        EXGeoEntity_VtxConn *pConn = pXtra->m_conns[i];
        const EXVector &v = *(const EXVector *)&pVerts[pConn->m_vertex];
        m_pParticles[i].m_pos = v;
        m_pParticles[i].m_oldPos = v;
        m_pParticles[i].m_accel.Set(0.0f, 0.0f, 0.0f, 0.0f);
        if (pConn->m_flags & 0x80) m_pParticles[i].m_flags = m_pParticles[i].m_flags | 1;
    }
    u32 nEdges = 0;
    for (i = 0; i < pXtra->m_numVerts; i++) {
        EXGeoEntity_VtxConn *pConn = pXtra->m_conns[i];
        for (u32 k = 0; k < pConn->m_numNeighbours; k++)
            if (pConn->m_neighbours[k] > pConn->m_vertex) nEdges++;
    }
    m_b4 = (int)(m_cc * f * 60.0f);
    float steps = (float)(int)m_b4;
    if (m_b4 && !m_98) m_98 = _EXAlloc(nEdges * 4, 0);
    u32 e = 0;
    for (i = 0; i < m_numParticles; i++) {
        EXGeoEntity_VtxConn *pConn = pXtra->m_conns[i];
        for (u32 k = 0; k < pConn->m_numNeighbours; k++) {
            u32 b = pConn->m_neighbours[k];
            if (b <= pConn->m_vertex) continue;
            u32 idx = b;
            for (u32 j = 0; j < pXtra->m_numVerts; j++) {
                EXGeoEntity_VtxConn *pc = pXtra->m_conns[j];
                if (pc->m_vertex == b) idx = j;
            }
            if (idx >= m_numParticles) continue;
            float old = 0.0f;
            if (m_b4) old = EXSqrt(m_pEdges[e].m_lengthSq);
            EXPClothEdge &edge = m_pEdges[e];
            EXPParticle *p1 = &m_pParticles[i];
            EXPParticle *p2 = &m_pParticles[idx];
            edge.m_p1 = p1;
            edge.m_p2 = p2;
            float dx = p2->m_pos.x - p1->m_pos.x;
            float dy = p2->m_pos.y - p1->m_pos.y;
            float dz = p2->m_pos.z - p1->m_pos.z;
            edge.m_lengthSq = dy * dy + dz * dz + dx * dx;
            if (edge.m_lengthSq < 1e-5f) edge.m_lengthSq = 1e-5f;
            if (m_b4 && m_98) {
                ((float *)m_98)[e] = (EXSqrt(m_pEdges[e].m_lengthSq) - old) / steps;
                float len = ((float *)m_98)[e] + old;
                m_pEdges[e].m_lengthSq = len * len;
            }
            e++;
        }
    }
    for (i = 0; i < m_numParticles; i++) {
        m_pParticles[i].m_pos = pOld[i].m_pos;
        if (!(m_pParticles[i].m_flags & 1)) m_pParticles[i].m_oldPos = pOld[i].m_oldPos;
        m_pParticles[i].m_accel = pOld[i].m_accel;
        m_pParticles[i].m_drag = pOld[i].m_drag;
    }
    EXFree(pOld);
    InitialiseExtraEdges(pEnt);
}

// Record edge e for particle p in mode 1: each particle keeps the two edges that best describe
// its surface for the normal n (lowest ScoreEdges).
#define CLOTH_PAIR_EDGE(p, e, n)                                                                     \
    {                                                                                                \
        int *pPair = &((int *)m_94)[(p) * 2];                                                        \
        if (pPair[0] < 0) {                                                                          \
            pPair[0] = (e);                                                                          \
        } else if (pPair[1] < 0) {                                                                   \
            pPair[1] = (e);                                                                          \
            pScore[p] = ScoreEdges(((int *)m_94)[(p) * 2], ((int *)m_94)[(p) * 2 + 1], (n));          \
        } else {                                                                                     \
            float s1 = ScoreEdges(pPair[0], (e), (n));                                               \
            float s2 = ScoreEdges((e), pPair[1], (n));                                               \
            if (s1 < pScore[p] || s2 < pScore[p]) {                                                  \
                if (s2 <= s1) {                                                                      \
                    pPair[0] = (e);                                                                  \
                    pScore[p] = s2;                                                                  \
                } else {                                                                             \
                    pPair[1] = (e);                                                                  \
                    pScore[p] = s1;                                                                  \
                }                                                                                    \
            }                                                                                        \
        }                                                                                            \
    }

// Build the cloth from a mesh entity: a particle per simulated vertex (fixed where flagged), an
// edge per connection, and per mode the edge pairs used for face normals (1) or a vertex normal
// array (2). Split entities go through SplitEntityInitialise instead.
s8 EXPCloth::Initialise(const EXGeoEntity *pEnt, u8 mode)
{
    if (!pEnt) return 0;
    if (pEnt->IsKindOf(EXRUNTIME_CLASS(EXGeoSplitEntity))) {
        m_pItem->m_position.Trace("Cloth position:");
        return 0;
    }
    if (!pEnt->HasXtraData(3)) return 0;
    EXPClothXtra *pXtra = (EXPClothXtra *)pEnt->XtraData(3);
    if (!pXtra) return 0;
    m_numParticles = pXtra->m_numVerts;
    if (!m_numParticles) return 0;
    m_pParticles = new EXPParticle[m_numParticles];
    EXGeoEntity_VtxData *pVerts = EntitySourceVerts(pEnt);
    if (!pVerts) {
        EXFree(m_pParticles);
        return 0;
    }
    u32 i;
    for (i = 0; i < pXtra->m_numVerts; i++) {
        EXGeoEntity_VtxConn *pConn = pXtra->m_conns[i];
        const EXVector &v = *(const EXVector *)&pVerts[pConn->m_vertex];
        m_pParticles[i].m_pos = v;
        m_pParticles[i].m_oldPos = v;
        m_pParticles[i].m_accel.Set(0.0f, 0.0f, 0.0f, 0.0f);
        if (pConn->m_flags & 0x80) m_pParticles[i].m_flags = m_pParticles[i].m_flags | 1;
    }
    u32 nEdges = 0;
    for (i = 0; i < pXtra->m_numVerts; i++) {
        EXGeoEntity_VtxConn *pConn = pXtra->m_conns[i];
        for (u32 k = 0; k < pConn->m_numNeighbours; k++)
            if (pConn->m_neighbours[k] > pConn->m_vertex) nEdges++;
    }
    m_numEdges = nEdges;
    m_pEdges = new EXPClothEdge[nEdges];
    m_d6 = mode;
    float *pScore = 0;
    if (mode == 1) {
        m_94 = _EXAlloc(m_numParticles * 8, 0);
        pScore = (float *)_EXAlloc(m_numParticles * 4, 0);
        for (i = 0; i < m_numParticles; i++) {
            ((int *)m_94)[i * 2] = -1;
            ((int *)m_94)[i * 2 + 1] = -1;
            pScore[i] = 3.402823466e+38f;
        }
    }
    if (m_d6 == 2) {
        m_6c = new EXVector[m_numParticles];
        FindNormals(pEnt, pVerts);
    }
    int e = 0;
    for (i = 0; i < m_numParticles; i++) {
        EXGeoEntity_VtxConn *pConn = pXtra->m_conns[i];
        EXVector3 vn0 = pEnt->GetVertexNormal(pVerts[i]);
        EXVector n(vn0.x, vn0.y, vn0.z, 0.0f);
        n.Normalise();
        for (u32 k = 0; k < pConn->m_numNeighbours; k++) {
            u32 b = pConn->m_neighbours[k];
            if (b <= pConn->m_vertex) continue;
            u32 idx = b;
            for (u32 j = 0; j < pXtra->m_numVerts; j++) {
                EXGeoEntity_VtxConn *pc = pXtra->m_conns[j];
                if (pc->m_vertex == b) idx = j;
            }
            if (idx >= m_numParticles) continue;
            EXPClothEdge &edge = m_pEdges[e];
            EXPParticle *p1 = &m_pParticles[i];
            EXPParticle *p2 = &m_pParticles[idx];
            edge.m_p1 = p1;
            edge.m_p2 = p2;
            EXVector d = p2->m_pos - p1->m_pos;
            edge.m_lengthSq = d.y * d.y + d.z * d.z + d.x * d.x;
            if (edge.m_lengthSq < 1e-5f) edge.m_lengthSq = 1e-5f;
            if (m_d6 == 1) {
                CLOTH_PAIR_EDGE(i, e, n)
                CLOTH_PAIR_EDGE(idx, e, n)
            }
            e++;
        }
    }
    InitialiseExtraEdges(pEnt);
    if (m_d6 == 1) {
        for (i = 0; i < pXtra->m_numVerts; i++) {
            EXGeoEntity_VtxConn *pConn = pXtra->m_conns[i];
            EXVector3 vn = pEnt->GetVertexNormal(pVerts[pConn->m_vertex]);
            int *pPair = &((int *)m_94)[i * 2];
            EXVector a = m_pEdges[pPair[1]].GetVector();
            EXVector c = m_pEdges[pPair[0]].GetVector().Cross(a);
            c.w = 0.0f;
            if (c.x * vn.x + c.y * vn.y + c.z * vn.z + 0.0f < 0.0f) {
                int t = pPair[0];
                pPair[0] = pPair[1];
                pPair[1] = t;
            }
        }
    }
    m_pItem->SetClipType(1);
    m_bOwnArrays = 1;
    if (pScore) EXFree(pScore);
    return 1;
}

// Per frame: rebuild the attachment frame; when following it with lag (m_128 > 0) blend the free
// particles towards where the frame's motion since last time carries them, and pin the fixed ones
// to their rest positions (kept in m_oldPos) in the new frame. Then step the cloth with the
// iteration count scaled to the frame rate and write it back to the entity.
void EXPAttachedClothWS::Syncronise()
{
    if (!m_bOwnArrays) return;
    EXItem *pItem = (EXItem *)m_118;
    if (!pItem) pItem = m_pItem;
    EXGDiData *pGDi = 0;
    EXGeoBaseEntity *pEnt = 0;
    GetEntity(pEnt, pGDi);
    if (!pEnt || !pGDi) return;
    EXMatrix m;
    EXDatumTemp d;
    if (m_120 && pItem->GetAnimatorDatum(m_120, d, 0, 0)) {
        m.set_rotate_quat(d.m_rotation);
        m.r[3].x = d.m_position.x;
        m.r[3].y = d.m_position.y;
        m.r[3].z = d.m_position.z;
    } else {
        m.set_rotate(pItem->m_rotation, pItem->m_rotOrder);
        float z = pItem->m_position.z, y = pItem->m_position.y;
        m.r[3].x = pItem->m_position.x;
        m.r[3].y = y;
        m.r[3].z = z;
    }
    m.r[3].w = 1.0f;
    EXMatrix delta;
    if (m_128 > FLT_EPSILON) {
        delta = m.mat_44::operator*(m_invMtx);
        m_invMtx = m.Inverse();
    }
    u32 i;
    for (i = 0; i < m_numParticles; i++) {
        EXPParticle &p = m_pParticles[i];
        u32 f = p.m_flags;
        if (!(f & 1) || (f & 2)) {
            if (m_128 > FLT_EPSILON) {
                EXVector t = p.m_oldPos;
                t.ApplyXForm(delta);
                p.m_oldPos = p.m_oldPos * (1.0f - m_128) + t * m_128;
                EXVector u = p.m_pos;
                u.ApplyXForm(delta);
                p.m_pos = p.m_pos * (1.0f - m_128) + u * m_128;
            }
        } else {
            p.SetPosition(p.GetOldPosition());
            p.m_pos.ApplyXForm(m);
        }
    }
    u32 it = m_iterations;
    m_c8 = m_cc * EXBaseApp::GameFrameRateMul();
    m_iterations = (u32)((int)it * m_c8);
    if (m_iterations < 1) m_iterations = 1;
    s8 ok = VerletUpdate(m_c8);
    m_iterations = it;
    if (ok) SyncWithVerts(pEnt, pGDi);
}

// As Syncronise while the game is paused: follow the attachment frame without stepping the cloth.
void EXPAttachedClothWS::PauseSync()
{
    EXItem *pItem = (EXItem *)m_118;
    if (!pItem) pItem = m_pItem;
    EXGDiData *pGDi = 0;
    EXGeoBaseEntity *pEnt = 0;
    GetEntity(pEnt, pGDi);
    if (!pEnt || !pGDi) return;
    EXMatrix m;
    EXDatumTemp d;
    if (m_120 && pItem->GetAnimatorDatum(m_120, d, 0, 0)) {
        m.set_rotate_quat(d.m_rotation);
        m.r[3].x = d.m_position.x;
        m.r[3].y = d.m_position.y;
        m.r[3].z = d.m_position.z;
    } else {
        m.set_rotate(pItem->m_rotation, pItem->m_rotOrder);
        float z = pItem->m_position.z, y = pItem->m_position.y;
        m.r[3].x = pItem->m_position.x;
        m.r[3].y = y;
        m.r[3].z = z;
    }
    m.r[3].w = 1.0f;
    EXMatrix delta;
    if (m_128 > FLT_EPSILON) {
        delta = m.mat_44::operator*(m_invMtx);
        m_invMtx = m.Inverse();
    }
    u32 i;
    for (i = 0; i < m_numParticles; i++) {
        EXPParticle &p = m_pParticles[i];
        u32 f = p.m_flags;
        if (!(f & 1) || (f & 2)) {
            if (m_128 > FLT_EPSILON) {
                EXVector t = p.m_oldPos;
                t.ApplyXForm(delta);
                p.m_oldPos = p.m_oldPos * (1.0f - m_128) + t * m_128;
                EXVector u = p.m_pos;
                u.ApplyXForm(delta);
                p.m_pos = p.m_pos * (1.0f - m_128) + u * m_128;
            }
        } else {
            p.SetPosition(p.GetOldPosition());
            p.m_pos.ApplyXForm(m);
        }
    }
    SyncWithVerts(pEnt, pGDi);
}

// Write the world space cloth back into the attached entity's vertices, in the attachment frame
// (moving the attached item onto the datum). For the first m_124 calls the free particles are
// first reset from the mesh. Normals as EXPCloth::SyncWithVerts, mode 1 normals rotated into the
// datum frame.
void EXPAttachedClothWS::SyncWithVerts(EXGeoBaseEntity *pBase, EXGDiData *pGDi)
{
    EXGeoEntity *pEnt = (EXGeoEntity *)pBase;
    if (!m_bOwnArrays) return;
    if (!pEnt->HasXtraData(3)) return;
    EXPClothXtra *pXtra = (EXPClothXtra *)pEnt->XtraData(3);
    EXItem *pItem = (EXItem *)m_118;
    if (!pItem) pItem = m_pItem;
    EXMatrix m;
    EXDatumTemp d;
    if (m_120 && pItem->GetAnimatorDatum(m_120, d, 0, 0)) {
        m.set_rotate_quat(d.m_rotation);
        m.r[3].x = d.m_position.x;
        m.r[3].y = d.m_position.y;
        m.r[3].z = d.m_position.z;
        m.r[3].w = 1.0f;
        ((EXItem *)m_11c)->SetPosition(d.m_position);
        ((EXItem *)m_11c)->SetRotation(d.m_rotation);
    } else {
        m.set_rotate(pItem->m_rotation, pItem->m_rotOrder);
        float z = pItem->m_position.z, y = pItem->m_position.y;
        m.r[3].x = pItem->m_position.x;
        m.r[3].y = y;
        m.r[3].z = z;
        m.r[3].w = 1.0f;
    }
    if (!pGDi) return;
    EXGeoEntity_VtxData *pVerts = (EXGeoEntity_VtxData *)pEnt->GetVertexBuffer(pGDi, 1);
    if (!pVerts) return;
    EXQuaternion iq;
    if (m_d6 == 1) {
        iq.w = d.m_rotation.w;
        iq.x = 0.0f - d.m_rotation.x;
        iq.y = 0.0f - d.m_rotation.y;
        iq.z = 0.0f - d.m_rotation.z;
    }
    u32 i;
    if (m_124) {
        for (i = 0; i < m_numParticles; i++) {
            EXPParticle &p = m_pParticles[i];
            if (p.m_flags & 2) continue;
            p.SetPosition(*(const EXVector *)&pVerts[i]);
            if (!(p.m_flags & 1))
                ParticleApplyXForm(p, m, 1);
            else
                ParticleApplyXForm(p, m, 0);
        }
        m_124--;
    }
    m = m.FastInverse();
    for (i = 0; i < pXtra->m_numVerts; i++) {
        EXVector t = m_pParticles[i].m_pos;
        t.ApplyXForm(m);
        EXGeoEntity_VtxConn *pConn = pXtra->m_conns[i];
        EXGeoEntity_VtxData &v = pVerts[pConn->m_vertex];
        v.m_x = t.x;
        v.m_y = t.y;
        v.m_z = t.z;
        if (m_d6 == 1) {
            const int *pPair = &((const int *)m_94)[i * 2];
            EXVector n = m_pEdges[pPair[0]].GetVector().Cross(m_pEdges[pPair[1]].GetVector()).UnitVector();
            EXVector r = iq.MultiplyVector(EXMaths::vec_xyzw(n));
            PackNormal(v, r);
        }
    }
    if (m_d6 == 2) {
        FindNormals(pEnt, pVerts);
        for (i = 0; i < pXtra->m_numVerts; i++) {
            EXGeoEntity_VtxConn *pConn = pXtra->m_conns[i];
            PackNormal(pVerts[pConn->m_vertex], ((EXVector *)m_6c)[pConn->m_vertex]);
        }
    }
    for (i = 0; i < pXtra->m_numCopies; i++) {
        EXGeoEntity_VtxConnCopy *pc = pXtra->m_copies;
        u32 src = pc[i].m_src;
        EXGeoEntity_VtxConnCopy *pd = pXtra->m_copies;
        EXGeoEntity_VtxData &dst = pVerts[pd[i].m_dst];
        dst.m_x = pVerts[src].m_x;
        dst.m_y = pVerts[src].m_y;
        dst.m_z = pVerts[src].m_z;
        dst.m_Normal = pVerts[src].m_Normal;
        if (m_d6) PackNormal(dst, UnpackNormal(pVerts[src]));
    }
    pEnt->ReleaseVertexBuffer(pGDi);
}
