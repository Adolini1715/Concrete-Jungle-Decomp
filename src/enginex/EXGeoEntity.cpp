#include "EXGeoEntity.h"
#include "EXGeoHeader.h"
#include "EXGeoFile.h"
#include "EXDisplay.h"
#include "EXWnd.h"
#include "EXMatrix.h"
#include "EXCollisionScoopTemp.h"
#include "EXShadowTemp.h"
#include "EXRayCastTemp.h"
#include "EXScratchPad.h"
#include "EXRand.h"
#include <stdlib.h>

class EXLineSeg;

// Model drawing hierarchy (EXGeoEntity.obj): the base entity's defaults, split (node tree) entities,
// map zone entities, the cached draw list and the entity info array used for random vertex picks.

// Split entity tree node (0x30): bounds box with the child count in min.w (> 0: the node has
// children, <= 0: a leaf) and a relative offset in max.w to the children (or the leaf's entity).
// The sibling count is |m_Count| of the first node in a run.
struct EXGeoTreeEntity_Node {
    int Count() const { return abs(m_Count); }
    EXGeoTreeEntity_Node *Child() const { return (EXGeoTreeEntity_Node *)((char *)&m_Offset + m_Offset); }
    EXGeoEntity *Entity() const { return (EXGeoEntity *)((char *)&m_Offset + m_Offset); }

    float m_Min[3];     // 0x00
    int m_Count;        // 0x0c
    float m_Max[3];     // 0x10
    s32 m_Offset;       // 0x1c
    u8 m_20[0x10];
};

// Leaf entities gathered for a shadow cast (scratch pad, 0x204).
struct EXNodeListTemp {
    int m_Count;
    EXGeoBaseEntity *m_Nodes[128];
};

// Cached draw list: entities whose draw is deferred to EXGeoEntityCachedDrawArray::Render.
struct EXGeoEntityCachedDrawElement {
    EXGeoBaseEntity *m_pEntity;
    EXGDiData *m_pData;
};
class EXGeoEntityCachedDrawArray : public EXDataArray<EXGeoEntityCachedDrawElement, const EXGeoEntityCachedDrawElement &> {
public:
    EXGeoEntityCachedDrawArray() { AllocMem(512); }
    ~EXGeoEntityCachedDrawArray() {}
    void Render(unsigned int pass);
};

#include "EXGeoColSphere.h"

EXGeoEntityCachedDrawArray GeoEntityCachedDrawArray;
EXGDiSkinData *EXGeoBaseEntity::ms_pGDiSkinData;
static unsigned int s_FaceCount;

static inline EXGDiData *GDi(EXGDiData *pData, const EXGeoBaseEntity *p)
{
    return pData ? (EXGDiData *)((char *)pData + p->m_GDiIndex * 0x68) : 0;
}

unsigned short EXGeoBaseEntity::CalcGDiCount()
{
    unsigned short n = 0;
    m_GDiCount = InitialiseGDiCount(n);
    return m_GDiCount;
}

unsigned short EXGeoBaseEntity::InitialiseGDiCount(unsigned short &n)
{
    m_GDiIndex = n;
    n++;
    return 1;
}

unsigned int EXGeoBaseEntity::GetVertexCount() const { return 0; }

static unsigned int Apply_GetFaceCount(EXGeoBaseEntity *pEnt, EXGDiData *)
{
    if (pEnt->IsKindOf(EXRUNTIME_CLASS(EXGeoEntity))) {
        EXGeoEntity *p = (EXGeoEntity *)pEnt;
        s_FaceCount += p->FaceCount() - p->DegenerateCount();
    }
    return 0;
}

unsigned int EXGeoBaseEntity::GetFaceCount()
{
    s_FaceCount = 0;
    ApplyFunction(Apply_GetFaceCount, 0);
    return s_FaceCount;
}

void EXGeoBaseEntity::BuildEntityInfoArray(EXGeoEntity_InfoArray &, EXGDiData *) {}

void *EXGeoBaseEntity::GetVertexBuffer(EXGDiData *, signed char)
{
    EXBaseDisplay::ActiveWnd()->DbgPrint("EXGeoBaseEntity::GetVertexBuffer() Not Implemented\n");
    return 0;
}

void EXGeoBaseEntity::ReleaseVertexBuffer(EXGDiData *)
{
    EXBaseDisplay::ActiveWnd()->DbgPrint("EXGeoBaseEntity::ReleaseVertexBuffer() Not Implemented\n");
}

void EXGeoBaseEntity::Draw(EXGDiData *, unsigned int) {}

unsigned int EXGeoBaseEntity::ApplyFunction(unsigned int (__cdecl *pFunc)(EXGeoBaseEntity *, EXGDiData *), EXGDiData *pData)
{
    return pFunc(this, pData);
}

unsigned int EXGeoBaseEntity::DoShadowCast(EXShadowTemp &)
{
    EXBaseDisplay::ActiveWnd()->DbgPrint("EXGeoBaseEntity::DoShadowCast() Not Implemented\n");
    return 0;
}

unsigned int EXGeoBaseEntity::DoRayCast(EXRayCastTemp &, EXGDiData *, unsigned int)
{
    EXBaseDisplay::ActiveWnd()->DbgPrint("EXGeoBaseEntity::DoRayCast() Not Implemented\n");
    return 0;
}

EXBoundsBox &EXGeoBaseEntity::GetBoundsBox() { return m_BoundsBox; }

unsigned int EXGeoMapZoneEntity::DoShadowCast(EXShadowTemp &t)
{
    EXGeoHeader *pHdr = GeoFile()->GeoHeader();
    for (unsigned int i = 0; i < m_NumSubZones; i++) {
        EXGeoBaseEntity *p = (EXGeoBaseEntity *)pHdr->RefPointer(m_Nodes[i].m_RefIndex);
        if (p) p->DoShadowCast(t);
    }
    return 0;
}

unsigned int EXGeoMapZoneEntity::DoRayCast(EXRayCastTemp &t, EXGDiData *pData, unsigned int)
{
    EXGeoHeader *pHdr = GeoFile()->GeoHeader();
    unsigned int hit = 0;
    for (unsigned int i = 0; i < m_NumSubZones; i++) {
        EXGeoBaseEntity *p = (EXGeoBaseEntity *)pHdr->RefPointer(m_Nodes[i].m_RefIndex);
        if (p) {
            hit |= p->DoRayCast(t, GDi(pData, p), 0);
            if (hit && t.m_bAnyHit) break;
        }
    }
    return hit;
}

unsigned int EXGeoMapZoneEntity::DoEntityScoop_Sphere(EXCScoopTemp &t, EXGDiData *pData, unsigned int a, unsigned int b)
{
    EXGeoHeader *pHdr = GeoFile()->GeoHeader();
    unsigned int hit = 0;
    for (unsigned int i = 0; i < m_NumSubZones; i++) {
        EXGeoBaseEntity *p = (EXGeoBaseEntity *)pHdr->RefPointer(m_Nodes[i].m_RefIndex);
        if (p) hit |= p->DoEntityScoop_Sphere(t, GDi(pData, p), a, b);
    }
    return hit;
}

unsigned int EXGeoMapZoneEntity::DoEntityScoop_Shape(EXCScoopTemp &t, EXGDiData *pData, unsigned int a, unsigned int b)
{
    EXGeoHeader *pHdr = GeoFile()->GeoHeader();
    unsigned int hit = 0;
    for (unsigned int i = 0; i < m_NumSubZones; i++) {
        EXGeoBaseEntity *p = (EXGeoBaseEntity *)pHdr->RefPointer(m_Nodes[i].m_RefIndex);
        if (p) hit |= p->DoEntityScoop_Shape(t, GDi(pData, p), a, b);
    }
    return hit;
}

void EXGeoEntityCachedDrawArray::Render(unsigned int pass)
{
    int n = m_nSize;
    for (int i = 0; i < n; i++) {
        EXGeoEntityCachedDrawElement &e = m_pData[i];
        ((EXGeoEntity *)e.m_pEntity)->EXGeoEntity::Draw(e.m_pData, pass);
    }
    m_nSize = 0;
}

void EXGeoEntity_Info::Set(EXGeoBaseEntity *pEnt, EXGDiData *pData, const EXMatrix &mtx)
{
    m_pEntity = (EXGeoEntity *)pEnt;
    m_pData = pData;
    m_Matrix = mtx;
    m_Usage = 0;
    m_VertexCount = 0;
    m_Weight = 0.0f;
}

float EXGeoEntity_InfoArray::CalcRandomUsageWeights()
{
    float total = 0.0f;
    for (int i = 0; i < m_nSize; i++) {
        EXGeoEntity_Info &info = m_pData[i];
        EXGeoEntity *p = info.m_pEntity;
        unsigned int n = p->GetVertexCount();
        total = p->m_SurfaceArea / (float)(int)n + total;
        info.m_Weight = total;
        info.m_VertexCount = (u16)n;
    }
    return total;
}

void EXGeoEntity_InfoArray::CalcRandomUsage(int n)
{
    float total = CalcRandomUsageWeights();
    for (int i = 0; i < m_nSize; i++) {
        EXGeoEntity_Info &e = m_pData[i];
        e.m_Usage = 0;
    }
    float scale = 32767.0f / total;
    for (int k = 0; k < n; k++) {
        float r = (float)(int)((g_EXRandClass.Rand32() >> 1) & 0x7fff) * (1.0f / scale);
        for (int j = 0; j < m_nSize; j++) {
            EXGeoEntity_Info &e = m_pData[j];
            if (e.m_Weight >= r) {
                e.m_Usage++;
                break;
            }
        }
    }
}

void EXGeoBaseEntity::OnCreate(EXGeoHeader *pHdr, EXGeoObjectHeader *pObj)
{
    m_pGeoFile = pHdr->GeoFile();
    for (unsigned int t = 0; t < 5; t++) {
        if (m_XtraData.HasXtraData((unsigned char)t)) {
            EXGeoCommonObject *p = (EXGeoCommonObject *)m_XtraData.GetXtra((unsigned char)t);
            p->Create(pHdr, pObj);
        }
    }
}

void EXGeoBaseEntity::OnDelete(EXGeoHeader *pHdr, EXGeoObjectHeader *pObj)
{
    for (unsigned int t = 0; t < 5; t++) {
        if (m_XtraData.HasXtraData((unsigned char)t)) ((EXGeoCommonObject *)m_XtraData.GetXtra((unsigned char)t))->OnDelete(pHdr, pObj);
    }
}

// Bounding sphere of the entity against the scoop sphere (in the scoop's platform space).
static __forceinline s8 ScoopSphereOutside(const EXBoundsSphere &s, const EXCollisionScoopTemp &t)
{
    float r = s.m_r + t.m_radius;
    EXVector c(s.m_x, s.m_y, s.m_z, 1.0f);
    EXVector p = *(const EXMaths::mat_44 *)&t.m_mtx * EXVector((const EXMaths::vec_xyzw)c);
    EXVector d = p - t.m_centre;
    return d.LengthXYZSquared() > r * r;
}

static __forceinline s8 ScoopSphereOutsideT(const EXCollisionScoopTemp &t, const EXBoundsSphere &s)
{
    float r = t.m_radius + s.m_r;
    EXVector c(s.m_x, s.m_y, s.m_z, 1.0f);
    EXVector p = *(const EXMaths::mat_44 *)&t.m_mtx * EXVector((const EXMaths::vec_xyzw)c);
    EXVector d = p - t.m_centre;
    return d.LengthXYZSquared() > r * r;
}

unsigned int EXGeoBaseEntity::DoEntityScoop_Sphere(EXCScoopTemp &tt, EXGDiData *pData, unsigned int flags, unsigned int sub)
{
    EXCollisionScoopTemp &t = (EXCollisionScoopTemp &)tt;
    if (!(flags & 3)) {
        if (ScoopSphereOutside(m_BoundsSphere, t)) return 0;
        EXBoundsBox box = m_BoundsBox;
        box._ApplyXForm(*(const EXMaths::mat_44 *)&t.m_mtx);
        if (!box.Intersect(t.m_centre, t.m_radius)) return 0;
    }
    if (t.m_numEntities >= t.m_maxEntities) return 0;
    EXCScoop_EntityInfo &e = t.m_pEntities[t.m_numEntities];
    e.m_mtx = t.m_mtx;
    e.m_40 = (u32)this;
    e.m_44 = (u32)pData;
    e.m_id = (u32)t.m_pItem;
    *(u16 *)&e.m_sub = (u16)t.m_2c;
    *(u16 *)&e.m_flags = (u16)sub;
    t.m_numEntities++;
    return 1;
}

unsigned int EXGeoBaseEntity::DoEntityScoop_Shape(EXCScoopTemp &tt, EXGDiData *pData, unsigned int flags, unsigned int sub)
{
    EXCollisionScoopTemp &t = (EXCollisionScoopTemp &)tt;
    if (!(flags & 3)) {
        if (ScoopSphereOutside(m_BoundsSphere, t)) return 0;
        EXBoundsBox box = m_BoundsBox;
        box._ApplyXForm(*(const EXMaths::mat_44 *)&t.m_mtx);
        if (!box.Intersect(t.m_centre, t.m_radius)) return 0;
    }
    if (t.m_numEntities >= t.m_maxEntities) return 0;
    EXCScoop_EntityInfo &e = t.m_pEntities[t.m_numEntities];
    e.m_mtx = t.m_mtx;
    e.m_40 = (u32)this;
    e.m_44 = (u32)pData;
    e.m_id = (u32)t.m_pItem;
    *(u16 *)&e.m_sub = (u16)t.m_2c;
    *(u16 *)&e.m_flags = (u16)sub;
    t.m_numEntities++;
    return 1;
}

int EXGeoBaseEntity::Value(int i) const
{
    const int *p = *(const ex::EXRelPtr24<int> *)&m_48;
    unsigned int mask = m_48 & 0xff;
    if (!(mask & (1 << i))) return 0;
    int n = 0;
    for (int j = 0; j < i; j++)
        if (mask & (1 << j)) n++;
    return p[n];
}

void EXGeoSplitEntity::OnDelete(EXGeoHeader *pHdr, EXGeoObjectHeader *pObj)
{
    for (unsigned int i = 0; i < EntityCount(); i++) Entity(i)->OnDelete(pHdr, pObj);
    EXGeoBaseEntity::OnDelete(pHdr, pObj);
}

unsigned short EXGeoSplitEntity::InitialiseGDiCount(unsigned short &n)
{
    if (!m_GDiCount) {
        for (unsigned int i = 0; i < EntityCount(); i++) m_GDiCount += Entity(i)->InitialiseGDiCount(n);
    }
    return m_GDiCount;
}

unsigned int EXGeoSplitEntity::GetVertexCount() const
{
    unsigned int n = 0;
    for (unsigned int i = 0; i < EntityCount(); i++) n += Entity(i)->GetVertexCount();
    return n;
}

void EXGeoSplitEntity::TreeDraw(const EXGeoTreeEntity_Node *pNode, EXGDiData *pData, signed char cull, unsigned int flags)
{
    int n = pNode->Count();
    for (int i = 0; i < n; i++) {
        int c = cull;
        if (c != -1) {
            c = EXDisplay::CullBox((const float *)&pNode[i], EXBaseDisplay::ObjectToCull());
            if (!c) continue;
        }
        if (c == -1) flags |= 1;
        if (pNode[i].m_Count > 0) TreeDraw(pNode[i].Child(), pData, (signed char)c, flags);
        else {
            EXGeoBaseEntity *p = pNode[i].Entity();
            p->Draw((EXGDiData *)((char *)pData + p->m_GDiIndex * 0x68), flags);
        }
    }
}

void EXGeoSplitEntity::Draw(EXGDiData *pData, unsigned int flags)
{
    if (!(flags & 1) && !(m_Flags & 0x20)) {
        if (!EXDisplay::CullBox((const float *)&m_BoundsBox, EXBaseDisplay::ObjectToCull())) return;
    }
    if (EntityCount() && Tree()) {
        signed char c = (m_Flags & 0x20) ? -1 : 1;
        TreeDraw(Tree(), pData, c, flags);
        return;
    }
    for (unsigned int i = 0; i < EntityCount(); i++) {
        EXGeoBaseEntity *p = Entity(i);
        p->Draw((EXGDiData *)((char *)pData + p->m_GDiIndex * 0x68), flags);
    }
}

unsigned int EXGeoSplitEntity::ApplyFunction(unsigned int (__cdecl *pFunc)(EXGeoBaseEntity *, EXGDiData *), EXGDiData *pData)
{
    unsigned int r = pFunc(this, 0);
    for (unsigned int i = 0; i < EntityCount(); i++) {
        EXGeoBaseEntity *p = Entity(i);
        r |= p->ApplyFunction(pFunc, GDi(pData, p));
    }
    return r;
}

void EXGeoSplitEntity::DoShadowCast_GetNodeList(EXGeoTreeEntity_Node *pNode, EXShadowTemp &t, EXNodeListTemp *pList)
{
    unsigned int mask = 0;
    int n = pNode->Count();
    int i;
    for (i = 0; i < n; i++) {
        EXGeoTreeEntity_Node &nd = pNode[i];
        if (nd.m_Min[0] > t.m_pos.x || nd.m_Min[2] > t.m_pos.z || t.m_pos.x > nd.m_Max[0] || t.m_pos.z > nd.m_Max[2] ||
            nd.m_Min[1] > t.m_radius || t.m_height > nd.m_Max[1])
            continue;
        if (nd.m_Count > 0) mask |= 1 << i;
        else pList->m_Nodes[pList->m_Count++] = nd.Entity();
    }
    if (mask) {
        for (i = 0; i < n; i++)
            if (mask & (1 << i)) DoShadowCast_GetNodeList(pNode[i].Child(), t, pList);
    }
}

unsigned int EXGeoSplitEntity::DoShadowCast(EXShadowTemp &t)
{
    if (EntityCount() && Tree()) {
        EXScratchPad pad;
        EXNodeListTemp *pList = (EXNodeListTemp *)pad.Claim(sizeof(EXNodeListTemp));
        pList->m_Count = 0;
        DoShadowCast_GetNodeList(Tree(), t, pList);
        int n = pList->m_Count;
        for (int i = 0; i < n; i++) pList->m_Nodes[i]->DoShadowCast(t);
        return 0;
    }
    for (unsigned int i = 0; i < EntityCount(); i++) Entity(i)->DoShadowCast(t);
    return 0;
}

unsigned int EXGeoSplitEntity::DoRayCast(EXRayCastTemp &t, EXGDiData *pData, unsigned int)
{
    if (!t.m_localBounds._Intersect(m_BoundsBox)) return 0;
    unsigned int hit = 0;
    for (unsigned int i = 0; i < EntityCount(); i++) {
        EXGeoBaseEntity *p = Entity(i);
        if (!t.m_localBounds._Intersect(p->m_BoundsBox)) continue;
        EXGeoColSphere_Table *pTab = *(ex::EXRelPtr<EXGeoColSphere_Table> *)&p->m_44;
        if (pTab && !DoRayCast_CollisionSpheres(pTab->NodeList(), pTab->Spheres(), (const EXLineSeg &)t)) continue;
        if ((hit |= p->DoRayCast(t, GDi(pData, p), 1)) && t.m_bAnyHit) break;
    }
    return hit;
}

unsigned int EXGeoSplitEntity::DoEntityScoop_Sphere(EXCScoopTemp &tt, EXGDiData *pData, unsigned int flags, unsigned int sub)
{
    EXCollisionScoopTemp &t = (EXCollisionScoopTemp &)tt;
    unsigned int hit = 0;
    if (!(flags & 3)) {
        if (ScoopSphereOutsideT(t, m_BoundsSphere)) return 0;
    }
    for (unsigned int i = 0; i < EntityCount(); i++) {
        EXGeoBaseEntity *p = Entity(i);
        if (!(flags & 1)) {
            if (ScoopSphereOutside(p->m_BoundsSphere, t)) continue;
        }
        unsigned int f = flags | 2;
        hit |= p->DoEntityScoop_Sphere(tt, GDi(pData, p), f, sub);
    }
    return hit;
}

unsigned int EXGeoSplitEntity::DoEntityScoop_Shape(EXCScoopTemp &tt, EXGDiData *pData, unsigned int flags, unsigned int sub)
{
    EXCollisionScoopTemp &t = (EXCollisionScoopTemp &)tt;
    unsigned int hit = 0;
    if (!(flags & 3)) {
        if (ScoopSphereOutsideT(t, m_BoundsSphere)) return 0;
    }
    for (unsigned int i = 0; i < EntityCount(); i++) {
        EXGeoBaseEntity *p = Entity(i);
        if (!(flags & 1)) {
            if (ScoopSphereOutside(p->m_BoundsSphere, t)) continue;
        }
        unsigned int f = flags | 2;
        hit |= p->DoEntityScoop_Shape(tt, GDi(pData, p), f, sub);
    }
    return hit;
}

void EXGeoSplitEntity::OnCreateNodes(EXGeoTreeEntity_Node *pNode, EXGeoHeader *pHdr, EXGeoObjectHeader *pObj)
{
    if ((u32)pNode & 0xf) for (;;) {}
    int n = pNode->Count();
    for (int i = 0; i < n; i++) {
        if (pNode[i].m_Count > 0) OnCreateNodes(pNode[i].Child(), pHdr, pObj);
        else pNode[i].Entity()->Create(pHdr, pObj);
    }
}

void EXGeoBaseEntity::AddCachedDraw(EXGDiData *pData, unsigned int)
{
    EXGeoEntityCachedDrawElement &e = GeoEntityCachedDrawArray.AddSpace(1);
    e.m_pEntity = this;
    e.m_pData = pData;
}

void EXGeoSplitEntity::OnCreate(EXGeoHeader *pHdr, EXGeoObjectHeader *pObj)
{
    EXGeoBaseEntity::OnCreate(pHdr, pObj);
    if (EntityCount() && Tree()) {
        OnCreateNodes(Tree(), pHdr, pObj);
        return;
    }
    for (unsigned int i = 0; i < EntityCount(); i++) ((EXGeoSplitEntity *)Entity(i))->Create(pHdr, pObj);
}

void EXGeoSplitEntity::BuildEntityInfoArray(EXGeoEntity_InfoArray &arr, EXGDiData *pData)
{
    if (arr.Flags() & 1) {
        for (unsigned int i = 0; i < EntityCount(); i++) {
            EXGeoBaseEntity *p = Entity(i);
            p->BuildEntityInfoArray(arr, GDi(pData, p));
        }
        return;
    }
    arr.AddSpace(1).Set(this, pData, arr.Matrix());
}

void EXGeoSplitEntity::AddCachedDraw(EXGDiData *pData, unsigned int flags)
{
    if (EntityCount() && Tree()) {
        signed char c = (m_Flags & 0x20) ? -1 : 1;
        TreeDraw(Tree(), pData, c, flags);
        return;
    }
    for (unsigned int i = 0; i < EntityCount(); i++) {
        EXGeoBaseEntity *p = Entity(i);
        p->AddCachedDraw((EXGDiData *)((char *)pData + p->m_GDiIndex * 0x68), flags);
    }
}

void EXGeoMapZoneEntity::OnDelete(EXGeoHeader *pHdr)
{
    for (unsigned int i = 0; i < m_NumSubZones; i++) {
        const EXGeoRefPointerHeader &ref = pHdr->RefPointerList().Data()[m_Nodes[i].m_RefIndex];
        if ((pHdr->SectionHeader(ref.Section())->HashCode() & 0x7f000000) == 0x23000000) pHdr->DeLoadSectionFile(ref.Section());
    }
}

EXGeoEntity_InfoArray::EXGeoEntity_InfoArray()
{
    m_Flags = 0;
    m_54 = 0;
}

// Picks n random vertices over the gathered pieces, weighted by surface area per vertex. Split
// entities are expanded into their pieces and recursed into (that branch does not add to the
// returned count).
unsigned int EXGeoEntity_InfoArray::GetRandomVertexArray(EXVector *pOut, int n, unsigned int flags)
{
    CalcRandomUsage(n);
    EXGeoEntity_InfoArray sub;
    unsigned int total = 0;
    for (int i = 0; i < m_nSize; i++) {
        EXGeoEntity_Info &info = m_pData[i];
        int cnt = info.m_Usage;
        if (!cnt) continue;
        EXGeoEntity *p = info.m_pEntity;
        if (!p->IsKindOf(EXRUNTIME_CLASS(EXGeoEntity))) {
            sub.Empty(0);
            sub.SetFlags(1);
            *(EXMatrix *)sub.m_10 = info.m_Matrix;
            p->BuildEntityInfoArray(sub, info.m_pData);
            pOut += sub.GetRandomVertexArray(pOut, cnt, flags);
        } else {
            unsigned int got = info.m_pEntity->GetRandomVertexArray(&info, pOut, cnt, flags);
            total += got;
            pOut += got;
        }
    }
    return total;
}
