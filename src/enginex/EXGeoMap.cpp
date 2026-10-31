#include "EXGeoMap.h"
#include "EXPrimVert.h"
#include "EXGeoFace.h"
#include "EXWnd.h"
#include "EXGeoHeader.h"
#include "EXVector.h"
#include "RGBA.h"
#include "EXBaseSoundManager.h"
#include "EXTriangle.h"
#include "EXItemAnimator_Map.h"
#include "EXBaseRenderEnv.h"
#include "EXBaseDisplay.h"
#include "EXDisplay.h"
#include "EXLightManager.h"
#include "EXItemEnv.h"
#include "EXStats.h"
#include "EXBaseApp.h"
#include "EXItemRender_Entity.h"
#include "EXGeoEntity.h"
#include "EXGeoIdentifier.h"
#include "EXMatrix.h"
#include "EXStdFuncs.h"
#include <math.h>
#include "EXu32Array.h"
#include "EXRayCastTemp.h"
#include "EXGeoColSphere.h"
#include "EXScratchPad.h"
#include "EXDistance.h"
#include "EXLine.h"
#include "EXPlane.h"
#include "EXGeoPathNode.h"

template <class T> inline const T &EXMin(const T &a, const T &b) { return a < b ? a : b; }

// Vector minus a 3-vector keeps the left w (always inlined; not in the map).
inline EXVector operator-(const EXVector &a, const EXVector3 &b)
{
    EXVector t(b, 0.0f);
    return EXVector(a.x - t.x, a.y - t.y, a.z - t.z, a.w);
}

void EXGeoMap::Debug_DrawLights() {}
void EXGeoMap::Debug_DrawMapZones(ex::RGBA) {}

void EXGeoMap::OnCreate(EXGeoHeader *pHdr, EXGeoObjectHeader *)
{
    for (int i = 0; i < m_Portals.Size(); i++) {
        EXGeoFace *pFace = m_Portals[i].GeoFace();
        if (pFace) pFace->Create(pHdr, 0);
    }
    EXBoundsBox box;
    box.SetMaxRange();
    for (unsigned int z = 0; z < m_NumZones; z++) {
        EXGeoZone &zone = m_Zones[z];
        if (zone.Section() & 0xff000000) {
            int s = pHdr->SectionList().FindIndex(zone.Section());
            if (s != -1) zone.SetSection(s);
        }
        Initialise_PlacementTree(zone.PlacementInfo()->PlacementTree(), pHdr, box);
    }
    EXBaseSoundManager *pSM = EXBaseSoundManager::GetSoundManager();
    for (int i = 0; i < m_Sounds.Size(); i++) {
        EXGeoSound &snd = m_Sounds[i];
        if (snd.m_HashCode != 0xffffffff) {
            const EXSoundDetails &d = pSM->SoundDetails(snd.m_HashCode);
            snd.m_1f = ((const u8 *)&d)[0xb];
            if (snd.m_MaxDist == 0.0f) {
                snd.m_MinDist = (float)((const s16 *)&d)[0];
                snd.m_MaxDist = (float)((const s16 *)&d)[1];
            }
        }
    }
}

int EXGeoMap::WhichZone(unsigned int hash) const
{
    for (unsigned int i = 0; i < m_NumZones; i++)
        if (m_Zones[i].m_HashRef == hash) return i;
    return 0;
}

void EXGeoLight::Debug_DrawLight(ex::RGBA) const {}

extern "C" double __cdecl fabs(double);
#pragma intrinsic(fabs)

// Light falloff at a point (map lights, EXGeoMap::CalcLightsV). Each attenuator the light has
// scales the result; any that gives 0 ends it. Results below 1/257.5 count as 0.

// Direction: facing of the surface normal n toward the light at p.
float EXGeoLight::Attenuate_DirectionAttenuator(const EXVector &p, const EXVector &q, const EXVector &n) const
{
    EXMaths::vec_xyzw d = p - q;
    float len = d.Length();
    if (len > EX_EPSILON_NORMALISE) {
        float inv = 1.0f / len;
        d.x *= inv;
        d.y *= inv;
        d.z *= inv;
        d.w *= inv;
    }
    float r = d.Dot(n);
    return r > 0.0038823530f ? r : 0.0f;
}

// Parallel: facing of the normal against the light's direction.
float EXGeoLight::Attenuate_ParallelAttenuator(const EXVector &, const EXVector &, const EXVector &n) const
{
    EXVector d = -EXVector(m_Direction, 0.0f);
    float r = d.Dot(n);
    return r > 0.0038823530f ? r : 0.0f;
}

// Beam: 1 on the beam axis, falling linearly to 0 at m_BeamAngle degrees (cosine-like on the dot).
float EXGeoLight::Attenuate_BeamAttenuator(const EXVector &p, const EXVector &q) const
{
    EXMaths::vec_xyzw d = q - p;
    float len = d.Length();
    if (len > EX_EPSILON_NORMALISE) {
        float inv = 1.0f / len;
        d.x *= inv;
        d.y *= inv;
        d.z *= inv;
        d.w *= inv;
    }
    float c = EXVector(m_Direction, 0.0f).Dot(d);
    float k = 1.0f - (1.0f - m_BeamAngle * (1.0f / 180.0f));
    float s = k != 0.0f ? 1.0f / k : 0.0f;
    float r = s * c + (1.0f - s);
    return r > 0.0038823530f ? r : 0.0f;
}

// Linear: 1 within m_Falloff of the range, then linear to 0 at m_Range; 0 outside the range box.
float EXGeoLight::Attenuate_LinearAttenuator(const EXVector &p, const EXVector &q) const
{
    EXVector d = q - p;
    float lenSq;
    if (fabs(d.x) >= m_Range || fabs(d.y) >= m_Range || fabs(d.z) >= m_Range ||
        (lenSq = d.y * d.y + d.z * d.z + d.x * d.x) >= m_Range * m_Range)
        return 0.0f;
    float inv = m_Range != 0.0f ? 1.0f / m_Range : 0.0f;
    float t = EXSqrt(lenSq) * inv;
    float r;
    if (m_Falloff >= t) {
        r = 1.0f;
    } else {
        float a, b;
        if (m_Falloff >= 1.0f) {
            a = 0.0f;
            b = 1.0f;
        } else {
            float f = 1.0f - max(0.0f, m_Falloff);
            float s = f != 0.0f ? 1.0f / f : 0.0f;
            a = 0.0f - s;
            b = 0.0f - a;
        }
        r = a * t + b;
    }
    return r;
}

float EXGeoLight::Attenuate(const EXVector &p, const EXVector &n, signed char bDirection) const
{
    float a = 1.0f;
    unsigned int f = m_Attenuators;
    EXVector pos;
    if (f & 1) {
        pos.Set(m_Position, 0.0f);
        float l = Attenuate_LinearAttenuator(pos, p);
        if (l == 0.0f) return 0.0f;
        a = l;
    }
    if (f & 4) {
        pos.Set(m_Position, 0.0f);
        float l = Attenuate_BeamAttenuator(pos, p);
        if (l == 0.0f) return 0.0f;
        a *= l;
    }
    if (f & 8) {
        float l = Attenuate_ParallelAttenuator(pos, p, n);
        if (l == 0.0f) return 0.0f;
        a *= l;
    }
    if (bDirection && (f & 2)) {
        pos.Set(m_Position, 0.0f);
        float l = Attenuate_DirectionAttenuator(pos, p, n);
        if (l == 0.0f) return 0.0f;
        a *= l;
    }
    return a;
}

void EXGeoPath::Debug_DrawPath(ex::RGBA) const {}
void EXGeoPath::Debug_DrawLinearPath(ex::RGBA) const {}
void EXGeoSplitPath::Debug_DrawPath(ex::RGBA) const {}

EXGeoIdentifier *EXGeoMap::GetZoneIdentifier(int zone) const { return m_Zones[zone].m_Identifier; }

EXGeoPath *EXGeoSplitPath::GetPath(int i) const
{
    const ex::EXRELOFFSET32 &o = ((ex::EXRELOFFSET32 *)(ex::EXRelPtr<EXGeoPath> *)m_Paths)[i];
    return o.m_Offset ? (EXGeoPath *)((char *)&o + o.m_Offset) : 0;
}

int EXGeoMap::GetPathIndex(unsigned int hash) const { return m_Paths.FindIndex(hash); }

EXGeoPath *EXGeoMap::GetPath(unsigned int hash) const
{
    int i = m_Paths.FindIndex(hash);
    if (i < 0) return 0;
    return &m_Paths.Data()[i];
}

int EXGeoMap::GetCameraIndex(unsigned int hash) const { return m_Cameras.FindIndex(hash); }

int EXGeoMap::GetCameraIndex(const EXVector &v) const
{
    float best = 3.4028235e38f;
    int index = -1;
    for (int i = 0; i < m_Cameras.Size(); i++) {
        EXMaths::vec_xyzw d = v - m_Cameras[i].Position();
        float dist = d.x * d.x + d.z * d.z + d.y * d.y;
        if (best > dist) {
            best = dist;
            index = i;
        }
    }
    return index;
}

EXGeoCamera *EXGeoMap::GetCamera(unsigned int hash) const
{
    int i = m_Cameras.FindIndex(hash);
    if (i < 0) return 0;
    return &m_Cameras.Data()[i];
}

EXGeoCamera *EXGeoMap::GetCamera(const EXVector &v) const { return GetCamera(GetCameraIndex(v)); }

int EXGeoMap::GetSpecialIndex(unsigned int hash) const { return m_Specials.FindIndex(hash); }

int EXGeoMap::GetSpecialIndex(const EXVector &v) const
{
    float best = 3.4028235e38f;
    int index = -1;
    for (int i = 0; i < m_Specials.Size(); i++) {
        EXMaths::vec_xyzw d = v - m_Specials[i].Position();
        float dist = d.x * d.x + d.z * d.z + d.y * d.y;
        if (best > dist) {
            best = dist;
            index = i;
        }
    }
    return index;
}

EXGeoSpecial *EXGeoMap::GetSpecial(unsigned int hash) const
{
    int i = m_Specials.FindIndex(hash);
    if (i < 0) return 0;
    return &m_Specials.Data()[i];
}

EXGeoSpecial *EXGeoMap::GetSpecial(const EXVector &v) const { return GetSpecial(GetSpecialIndex(v)); }

int EXGeoMap::GetLightIndex(unsigned int hash) const { return m_Lights.FindIndex(hash); }

int EXGeoMap::GetLightIndex(const EXVector &v) const
{
    float best = 3.4028235e38f;
    int index = -1;
    for (int i = 0; i < m_Lights.Size(); i++) {
        EXMaths::vec_xyzw d = v - m_Lights[i].Position();
        float dist = d.x * d.x + d.z * d.z + d.y * d.y;
        if (best > dist) {
            best = dist;
            index = i;
        }
    }
    return index;
}

EXGeoLight *EXGeoMap::GetLight(unsigned int hash) const
{
    int i = m_Lights.FindIndex(hash);
    if (i < 0) return 0;
    return &m_Lights.Data()[i];
}

EXGeoLight *EXGeoMap::GetLight(const EXVector &v) const
{
    int i = GetLightIndex(v);
    if (i >= 0) return &m_Lights.Data()[i];
    return 0;
}

EXGeoPortal *EXGeoMap::GetPortal(int i) const
{
    if ((unsigned int)i >= (unsigned int)m_Portals.Size()) return 0;
    return &m_Portals.Data()[i];
}

void EXGeoMap::InitPortals() const
{
    for (int i = 0; i < m_Portals.Size(); i++) m_Portals.Data()[i].m_Visited = 0;
}

int EXGeoBasePath::Value(int i) const
{
    const s32 *pTable = m_Values;
    u32 mask = m_Values.m_Value & 0xff;
    if (!(mask & (1 << i))) return 0;
    int n = 0;
    for (int b = 0; b < i; b++)
        if (mask & (1 << b)) n++;
    return pTable[n];
}

EXGeoPath *EXGeoMap::GetPath(const EXVector &v, unsigned int flags) const
{
    int i = GetPathIndex(v, flags);
    if (i >= 0) return &m_Paths.Data()[i];
    return 0;
}

int EXGeoMap::GetPortalIndex(const EXVector &v) const
{
    float best = 3.4028235e38f;
    int index = -1;
    for (int i = 0; i < m_Portals.Size(); i++) {
        EXVector nearest;
        float d = m_Portals.Data()[i].DistanceTo(v, nearest);
        if (best > d) {
            best = d;
            index = i;
        }
    }
    return index;
}

EXGeoPortal *EXGeoMap::GetPortal(const EXVector &v) const { return GetPortal(GetPortalIndex(v)); }

unsigned int EXGeoMap::GetMapZoneList(const EXVector &, const EXBoundsBox &box, unsigned int *pList) const
{
    EXVector centre;
    float r = box.CentreAndRadius(centre);
    return GetMapZoneList(centre, r, pList);
}

EXBoundsBox EXGeoMap::BoundsBox(const EXGeoHeader *) const
{
    EXBoundsBox box;
    box.Set(EXVector(m_BoundsBox.vctMin(), 0.0f), EXVector(m_BoundsBox.vctMax(), 0.0f));
    return box;
}

static inline EXVector FaceVtx(const GEOFACE_VTX *pVtx, unsigned int i)
{
    return EXVector(*(const EXMaths::vec_xyzw *)&pVtx[i]);
}

static __forceinline void SetTriangle(EXTriangle &tri, const EXVector &a, const EXVector &b, const EXVector &c)
{
    tri.m_v[0].set(a);
    tri.Edge0Set(b - a);
    tri.Edge1Set(c - a);
}

// NEAR: structural; distance to the face's triangle fan, or to the two triangles of the
// portal's own quad when it has no face.
float EXGeoPortal::DistanceTo(const EXVector &p, EXVector &nearest) const
{
    float best = 1.8446743e19f;
    const EXGeoFace *pFace = GeoFace();
    if (pFace) {
        const GEOFACE_VTX *pVtx = pFace->Vtx();
        EXTriangle tri;
        SetTriangle(tri, FaceVtx(pVtx, 0), FaceVtx(pVtx, 1), FaceVtx(pVtx, 2));
        for (unsigned int i = 0; i < pFace->VertexCount() - 2; i++) {
            tri.EdgeSet((i - 1) & 1, FaceVtx(pVtx, i + 2) - FaceVtx(pVtx, 0));
            float d = tri.DistanceTo(p, best, nearest);
            best = EXMin(d, best);
        }
        return best;
    }
    EXTriangle tri;
    SetTriangle(tri, EXVector(m_Verts[0], 0.0f), EXVector(m_Verts[1], 0.0f), EXVector(m_Verts[2], 0.0f));
    for (int i = 0; i < 2; i++) {
        tri.EdgeSet((i - 1) & 1, EXVector(m_Verts[i + 2], 0.0f) - EXVector(m_Verts[0], 0.0f));
        float d = tri.DistanceTo(p, best, nearest);
        best = EXMin(d, best);
    }
    return best;
}

// Retail adds the face's first vertex VertexCount() times (never indexes by i).
unsigned int EXGeoPortal::AccumalateVerticies(EXVector &sum) const
{
    const EXGeoFace *pFace = GeoFace();
    if (pFace) {
        unsigned int n = pFace->VertexCount();
        for (unsigned int i = 0; i < n; i++) sum += EXVector(*(const EXMaths::vec_xyzw *)pFace->Vtx());
        return n;
    }
    for (int i = 0; i < 4; i++) {
        sum += EXVector(m_Verts[i], 0.0f);
    }
    return 4;
}

EXVector EXGeoPortal::Centre() const
{
    EXVector sum(0.0f, 0.0f, 0.0f, 0.0f);
    unsigned int n = AccumalateVerticies(sum);
    if (n) sum *= 1.0f / (float)n;
    return sum;
}

int EXGeoBspTree::WhichNode(EXMaths::vec_xyzw p) const
{
    int i = 0;
    do {
        const EXGeoBspNode &node = m_Nodes[i];
        float d = node.m_Plane[2] * p.z + node.m_Plane[1] * p.y + node.m_Plane[0] * p.x + node.m_Plane[3];
        i = node.m_Child[0.0f > d ? 1 : 0];
    } while (i >= 1);
    return -i;
}

// NEAR: retail returns the no-portal case through a separate constant register (edi = 1).
unsigned int EXGeoMap::GetMapZoneList(const EXVector &p, float r, unsigned int *pList) const
{
    int zone = ((EXGeoBspTree *)m_BspTree)->WhichNode(p);
    pList[0] = zone;
    const EXGeoZone *pZone = &m_Zones[zone];
    unsigned int n = 1;
    int i = 0;
    if (pZone->m_NumPortalRefs <= 0) return n;
    do {
        const EXGeoZonePortalRef *pHead = &((EXGeoZonePortalRef *)pZone->m_PortalRefs)[i];
        for (int j = 0; j < pHead->m_Extra + 1; j++) {
            EXGeoPortal *pPortal = &m_Portals.Data()[pHead[j].m_Portal];
            if (pHead->m_Zone >= 0) {
                EXVector nearest;
                if (r > pPortal->DistanceTo(p, nearest)) {
                    pList[n++] = pHead->m_Zone;
                    break;
                }
            }
        }
        i += pHead->m_Extra + 1;
    } while (i < pZone->m_NumPortalRefs);
    return n;
}

// Half extents of the box swept by the box WhichNodes (read by CheckNodes for SHAPE_BOX).
static EXMaths::vec_xyzw s_BoxExtent;

// The BSP leaves (zones) touched by a point, sphere or box swept from a to b, written to pList
// as zone numbers without repeats (at most 16). The segment is clipped against each node's plane
// (pushed out by the shape's radius on the plane normal, the box's projected half extent for
// boxes); a straddling piece is split and the far part kept on a stack of up to 32.
int EXGeoBspTree::CheckNodes(EXMaths::vec_xyzw a, EXMaths::vec_xyzw b, short *pList, Shape shape, float r) const
{
    struct Piece {
        EXMaths::vec_xyzw a, b;
        float ta, tb;
        short node;
    };
    Piece stack[32];
    int count = 0;
    float ta = 0.0f;
    float tb = 1.0f;
    int sp = 0;
    a.w = 1.0f;
    b.w = 1.0f;
    int node = 0;
    for (;;) {
        do {
            const EXGeoBspNode &n = m_Nodes[node];
            const float *p = n.m_Plane;
            float da = p[1] * a.y + p[2] * a.z + p[0] * a.x + p[3] * a.w;
            float db = p[1] * b.y + p[2] * b.z + p[0] * b.x + p[3] * b.w;
            if (shape == SHAPE_BOX)
                r = fabsf(p[0] * s_BoxExtent.x) + fabsf(p[1] * s_BoxExtent.y) + fabsf(p[2] * s_BoxExtent.z);
            if (da >= r && db >= r) {
                node = n.m_Child[0];
            } else if (da < 0.0f - r && db < 0.0f - r) {
                node = n.m_Child[1];
            } else {
                // Straddles: [0, tNear] stays on side `side`, [tFar, 1] goes to the other side.
                int side;
                float tNear = 1.0f;
                float tFar;
                if (da != db) {
                    float inv = 1.0f / (da - db);
                    float t1, t2;
                    if (da <= db) {
                        t1 = da - r;
                        t2 = da + r + EX_EPSILON_UNIT;
                    } else {
                        t1 = da + r;
                        t2 = da - r - EX_EPSILON_UNIT;
                    }
                    side = da <= db;
                    float v = (t1 + EX_EPSILON_UNIT) * inv;
                    tNear = v >= 1.0f ? 1.0f : (v <= 0.0f ? 0.0f : v);
                    v = t2 * inv;
                    tFar = v >= 1.0f ? 1.0f : (v <= 0.0f ? 0.0f : v);
                } else {
                    side = 0;
                    tFar = 0.0f;
                }
                if (sp < 32) {
                    Piece &q = stack[sp++];
                    q.a.x = (b.x - a.x) * tFar + a.x;
                    q.a.y = (b.y - a.y) * tFar + a.y;
                    q.a.z = (b.z - a.z) * tFar + a.z;
                    q.a.w = (b.w - a.w) * tFar + a.w;
                    q.b = b;
                    q.ta = (tb - ta) * tFar + ta;
                    q.tb = tb;
                    q.node = n.m_Child[1 - side];
                }
                b.x = (b.x - a.x) * tNear + a.x;
                b.y = (b.y - a.y) * tNear + a.y;
                b.z = (b.z - a.z) * tNear + a.z;
                b.w = (b.w - a.w) * tNear + a.w;
                tb = (tb - ta) * tNear + ta;
                node = n.m_Child[side];
            }
        } while (node > 0);
        do {
            // A leaf: record its zone once.
            int zone = -node;
            int i;
            for (i = 0; i < count; i++)
                if (pList[i] == zone) break;
            if (i == count) {
                pList[count++] = (short)zone;
                if (count >= 16) return count;
            }
            if (sp <= 0) return count;
            Piece &q = stack[--sp];
            a = q.a;
            b = q.b;
            ta = q.ta;
            tb = q.tb;
            node = q.node;
        } while (node < 1);
    }
}

// Box version: the box from boxMin to boxMax (around the swept points) is centred, with its
// half extents taken as the larger of -boxMin and boxMax per axis. A zero box is a point.
int EXGeoBspTree::WhichNodes(const EXMaths::vec_xyzw &a, const EXMaths::vec_xyzw &b, const EXMaths::vec_xyzw &boxMin,
                             const EXMaths::vec_xyzw &boxMax, short *pList) const
{
    if (boxMin.x == 0.0f && boxMin.y == 0.0f && boxMin.z == 0.0f && boxMax.x == 0.0f && boxMax.y == 0.0f &&
        boxMax.z == 0.0f)
        return WhichNodes(a, b, pList);
    s_BoxExtent.x = 0.0f - boxMin.x;
    if (s_BoxExtent.x <= boxMax.x) s_BoxExtent.x = boxMax.x;
    s_BoxExtent.y = 0.0f - boxMin.y;
    if (s_BoxExtent.y <= boxMax.y) s_BoxExtent.y = boxMax.y;
    s_BoxExtent.z = 0.0f - boxMin.z;
    if (s_BoxExtent.z <= boxMax.z) s_BoxExtent.z = boxMax.z;
    return CheckNodes(a, b, pList, SHAPE_BOX, 0.0f);
}

int EXGeoBspTree::WhichNodes(const EXMaths::vec_xyzw &a, const EXMaths::vec_xyzw &b, short *pList) const
{
    return CheckNodes(a, b, pList, SHAPE_POINT, 0.0f);
}

int EXGeoBspTree::WhichNodes(const EXMaths::vec_xyzw &a, const EXMaths::vec_xyzw &b, float r, short *pList) const
{
    return CheckNodes(a, b, pList, SHAPE_SPHERE, r);
}

// NEAR: retail hoists the zone light-list and light-array relptr offsets out of the loop and
// spills them; colour max temporaries differ.
EXVector EXGeoMap::CalcLightsV(const EXVector &p, const EXVector &n, signed char b, int zone)
{
    EXVector col(0.0f, 0.0f, 0.0f, 0.0f);
    if (zone == 0xff) {
        zone = BspTree()->WhichNode(p);
        if (zone == 0xff) return EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    }
    const EXGeoZone *pZone = &m_Zones[zone];
    int num = pZone->NumLights();
    for (int i = 0; i < num; i++) {
        const EXGeoLight &light = m_Lights[pZone->Lights(i)];
        if (light.Flags() & 1) {
            float a = light.Attenuate(p, n, 0);
            if (a > 0.0f) {
                ex::RGBA c = light.Colour();
                EXMaths::vec_xyzw lc = EXVector((float)c.m_R * a, (float)c.m_G * a, (float)c.m_B * a, a * 128.0f);
                col = EXMaths::Max(col, lc);
            }
        }
    }
    return col * (1.0f / 128.0f);
}

// As above, also returning the strongest light in info. Retail keeps the running maximum scaled
// by 1/128 on every light (so earlier lights fade) and returns (r, g, g, g).
EXVector EXGeoMap::CalcLightsV(EXLightInfo &info, const EXVector &p, const EXVector &n, signed char b, int zone)
{
    EXVector col(0.0f, 0.0f, 0.0f, 0.0f);
    float best = -1.0f;
    if (zone == 0xff) {
        zone = BspTree()->WhichNode(p);
        if (zone == 0xff) return EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    }
    const EXGeoZone *pZone = &m_Zones[zone];
    const EXGeoLight *pBest = 0;
    for (int i = 0; i < pZone->NumLights(); i++) {
        const EXGeoLight &light = m_Lights[pZone->Lights(i)];
        if (light.Flags() & 1) {
            float a = light.Attenuate(p, n, 0);
            if (a > 0.0f) {
                ex::RGBA c = light.Colour();
                float r = (float)c.m_R;
                float g = (float)c.m_G;
                float bl = (float)c.m_B;
                EXMaths::vec_xyzw lc = EXVector(r * a, g * a, bl * a, a * 128.0f);
                col = EXMaths::Max(col, lc) * (1.0f / 128.0f);
                if (a > best) {
                    best = a;
                    float s = a * (1.0f / 128.0f);
                    info.m_Colour = EXVector(r * s, g * s, bl * s, s * 128.0f);
                    pBest = &light;
                }
            }
        }
    }
    if (pBest) {
        info.m_Position = EXVector(pBest->Position(), 0.0f);
        info.m_Hash = pBest->m_HashCode;
        info.m_Flags = pBest->m_Flags;
        info.m_Valid = 1;
        info.m_29 = 0;
    }
    return EXVector(col.x, col.y, col.y, col.y);
}

// Up to num lights reaching the point, each recorded in pInfo; the colour is a running average
// (each light halves what came before).
EXVector EXGeoMap::CalcLightsV(EXLightInfo *pInfo, int num, const EXVector &p, const EXVector &n, signed char b,
                               int zone)
{
    EXVector col(0.0f, 0.0f, 0.0f, 0.0f);
    if (zone == 0xff) {
        zone = BspTree()->WhichNode(p);
        if (zone == 0xff) return EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    }
    const EXGeoZone *pZone = &m_Zones[zone];
    int count = 0;
    for (int i = 0; i < pZone->NumLights(); i++) {
        if (count == num) break;
        const EXGeoLight &light = m_Lights[pZone->Lights(i)];
        if (light.Flags() & 1) {
            float a = light.Attenuate(p, n, 0);
            if (a > 0.0f) {
                ex::RGBA c = light.Colour();
                float r = (float)c.m_R;
                float g = (float)c.m_G;
                float bl = (float)c.m_B;
                EXVector lc(r * a, g * a, bl * a, a * 128.0f);
                col = (col + lc) * 0.5f;
                pInfo->m_Position = EXVector(light.Position(), 0.0f);
                float s = a * (1.0f / 128.0f);
                pInfo->m_Colour = EXVector(r * s, g * s, bl * s, s * 128.0f);
                pInfo->m_Hash = light.m_HashCode;
                pInfo->m_Flags = light.m_Flags;
                pInfo->m_Valid = 1;
                pInfo->m_29 = 0;
                pInfo++;
                count++;
            }
        }
    }
    return col * (1.0f / 128.0f);
}

static inline int ClampByte(float f)
{
    int i = (int)f;
    return i >= 0xff ? 0xff : (i > 0 ? i : 0);
}

ex::RGBA EXGeoMap::CalcLights(const EXVector &p, const EXVector &n, signed char b, int zone)
{
    EXVector v = CalcLightsV(p, n, b, zone) * 128.0f;
    int r = ClampByte(v.x);
    int g = ClampByte(v.y);
    int bl = ClampByte(v.z);
    int a = ClampByte(v.w);
    return ex::RGBA(r, g, bl, a);
}

// TODO stand-in: LOD selection result (EXItemRender_Entity.obj fills up to four of these).
struct EXLodInfo {
    const EXGeoEntityHeader *m_pHdr;    // +0
    u16 m_lod;                          // +4
    u16 m_flags;                        // +6 1: distance fade (high byte: layer mask), 4: geo morph
    float m_morph;                      // +8
    float m_fadeStart;                  // +0xc
    float m_fadeEnd;                    // +0x10
};

// EXGeoHeader::GeoEntityHeader, which the original inlines here.
static __forceinline EXGeoEntityHeader *MapGeoEntityHeader(const EXGeoHeader *pHdr, u32 hash)
{
    const EXGeoCommonArray<EXGeoEntityHeader> &a = pHdr->m_Entities;
    int i = EXGeoCommonArray_FindIndex(hash, (void *)&a, sizeof(EXGeoEntityHeader));
    if (i < 0) return 0;
    return &a.Data()[i];
}

// NEAR: ~97 diff lines: retail re-reads the light set word after storing it (both SetLightSet
// inlines; same in EXBaseRenderEnv::SetLightSet), loop-entry padding and a few register choices.
class EXScriptController {
public:
    static float UpdateThread_CalcTime(float start, float end, u32 mode, float t);
};
extern "C" double __cdecl cos(double);
extern "C" double __cdecl sin(double);
#pragma intrinsic(cos, sin)

// A placement with vertex sets (xtra data 0: count, mode, speed, then the set numbers) blends
// between two of them over time; mode 2 eases in and out at both ends.
void EXGeoMap::DrawZone_XtraRender(const EXGeoPlacement *pPlace, float frames) const
{
    const s16 *pXtra = (const s16 *)((const EXGeoXtraData *)&pPlace->m_XtraData)->GetXtra(0);
    if (!pXtra) return;
    int n = pXtra[0] - 1;
    if (n <= 0) return;
    frames *= ((const float *)pXtra)[1];
    int mode = ((const s8 *)pXtra)[3];
    float fn = (float)n;
    int idx;
    if (mode == 2 && n >= mode) {
        frames = EXScriptController::UpdateThread_CalcTime(0.0f, fn + 2.0f, ((const s8 *)pXtra)[2], frames);
        if (2.0f > frames) {
            idx = 0;
            frames = 1.0f - (float)cos(frames * 0.7853982f);
        } else if (frames > fn) {
            idx = n - 1;
            frames = (float)sin((frames - fn) * 0.7853982f);
        } else {
            frames -= 1.0f;
            idx = (int)frames;
            frames -= (float)idx;
        }
    } else {
        float f = EXScriptController::UpdateThread_CalcTime(0.0f, fn, ((const s8 *)pXtra)[2], frames);
        idx = (int)f;
        frames = f - (float)idx;
        if (mode) frames = ((float)cos(frames * 3.1415927f) - 1.0f) * -0.5f;
    }
    int next = idx + 1;
    frames = limit(frames, 0.0f, 0.99999f);
    EXBaseRenderEnv::ms_RenderInfo.SetVertexSet(pXtra[4 + min(n, idx)], pXtra[4 + min(n, next)], frames);
    EXBaseRenderEnv::EnableVertexSet(1);
}

void EXGeoMap::DrawZone(EXMapZoneInfoTable *pTable, EXMapZoneInfo *pZone, const EXRenderInfo &info)
{
    EXGeoHeader *pHdr = EXBaseDisplay::GeoHeader();
    if (!pHdr) return;
    u32 riBuf[sizeof(EXRenderInfo) / 4];  // EXRenderInfo copies are memcpy (rep movsd)
    memcpy(riBuf, &info, sizeof(EXRenderInfo));
    const EXRenderInfo &ri = *(const EXRenderInfo *)riBuf;
    const EXGeoRenderEnv &env = pZone->DrawRender();
    EXBaseRenderEnv::SetFogDistance(env.FogNear(), env.FogFar());
    EXBaseRenderEnv::SetFogMinMax(env.FogMin(), env.FogMax());
    EXBaseRenderEnv::SetFogColor(env.FogColor());
    EXBaseRenderEnv::SetFogEnabled(env.FogEnabled());
    EXBaseRenderEnv::SetRenderInfo(ri);
    EXLightManager *pLM = EXBaseRenderEnv::LightManager();
    if (pLM) pLM->SetupMapZoneLights(pZone->MapOn().Zone());
    EXGeoZone *pGeoZone = MapZone(pZone->MapOn().Zone());
    float zoneLight = 0.0f;
    if (PlacementGroups().Size() > 0) {
        int ls = pGeoZone->Identifier()->m_LightSet;
        if (ls >= 0) zoneLight = pTable->PlacementGroupArray()[ls].m_LightSet;
    }
    EXBaseRenderEnv::SetLightSet(zoneLight);
    if (pZone->MapSubZoneInfoArray().Size()) {
        EXGeoMapZoneEntity *pZoneEnt = (EXGeoMapZoneEntity *)pHdr->RefPointer(pGeoZone->SubZoneHashRef());
        if (pZoneEnt) {
            for (unsigned int i = 0; i < pZoneEnt->NumSubZones(); i++) {
                EXGeoBaseEntity *pEnt = (EXGeoBaseEntity *)pHdr->RefPointer(pZoneEnt->SubZoneEntity(i)->m_RefIndex);
                if (pEnt) {
                    int n = pEnt->GDiCount();
                    if (n) {
                        EXGDiDataArray &a = pZone->MapSubZoneInfoArray()[i].GDiArray();
                        if (!a.Size()) a.Init(n);
                        pEnt->Draw((EXGDiData *)a.m_00, 0);
                    }
                }
            }
        }
    }
    s8 bFade = (s8)((u8)EXBaseRenderEnv::ms_RenderInfo.m_renderFlags.m_flags >> 4) & 1;
    EXBaseDisplay *pDisp = EXBaseDisplay::GetDisplay();
    EXMatrix objToScreen = EXBaseDisplay::ObjectToScreen();
    EXMatrix objToWorld = EXBaseDisplay::ObjectToWorld();
    float frames = (float)g_Stats.FrameCount() / (float)EXBaseApp::NominalEngineFrameRate() * EXBaseApp::RenderFrameScale();
    const EXViewPortal &vp = pZone->ViewPortal();
    EXGeoPlacementInfo *pInfo = pGeoZone->PlacementInfo();
    unsigned short *pList = pInfo->PlacementList();
    EXMapPlacement *pPlacements = pTable->PlacementArray().GetData();
    EXPlacementGroup *pGroups = pTable->PlacementGroupArray().GetData();
    for (int i = 0; i < pInfo->NumPlacements(); i++) {
        int idx = pList[i];
        u32 &used = pTable->PlacementUsedArray()[idx / 32];
        u32 bit = 1 << (idx & 31);
        if (used & bit) continue;
        EXMapPlacement &p = pPlacements[idx];
        if (!(p.EngineFlags().m_flags & 0x10)) {
            if (p.GDiDataArray(0).Size()) {
                if (!ClipPortal(vp, p.BoundsBox(), objToScreen)) continue;
            }
            if (!pTable->MapZoneInfo(p.Zone())->IsLoaded()) continue;
        }
        EXGeoEntityHeader *pEH = MapGeoEntityHeader(pHdr, p.ObjectRef());
        if (!pEH) continue;
        EXLodInfo lods[4];
        u32 nLods;
        if (*(const ex::EXRelPtr24<void> *)&pEH->m_10) {
            EXVector pos = p.Position();
            float d = EXItemEnv::GetItemEnv()->MinDistanceToItemViews(pos);
            nLods = EXItemRender_Entity::CalcEntityLod(lods, pEH, 0, 3, d);
            if (!nLods) continue;
        } else {
            lods[0].m_flags = 0;
            lods[0].m_lod = 0;
            lods[0].m_pHdr = pEH;
            nLods = 1;
        }
        for (u32 j = 0; j < nLods; j++) {
            EXLodInfo *pL = &lods[j];
            EXGeoBaseEntity *pE;
            if (pL->m_pHdr && (pE = pL->m_pHdr->m_Addr[0].m_pObject) != 0) {
                if (j == 0) {
                    const EXMaths::mat_44 *pM = &p.mtx();
                    EXMatrix m;
                    if (pE->Flags() & 4) {
                        m = p.mtx() * (const EXMatrix &)EXBaseDisplay::ObjectToVap();
                        EXMaths::vec_xyzw t = p.mtx().r[3];
                        m.r[3].x = t.x;
                        m.r[3].y = t.y;
                        m.r[3].z = t.z;
                        m.r[3].w = t.w;
                        pM = &m;
                    }
                    EXBaseDisplay::SetObjectToWorld(*pM);
                    pDisp->UpdateTransforms();
                }
                if (EXDisplay::CullSphere(pE->BoundsSphere(), EXBaseDisplay::ObjectToCull())) {
                    if (j == 0) {
                        if (!p.GDiDataArray(0).Size()) p.InitialiseGDiMemory(pHdr);
                        float ls = (float)p.LightSet();
                        if (p.Group() >= 0) ls = pGroups[p.Group()].m_LightSet + ls;
                        EXBaseRenderEnv::SetLightSet(ls);
                        if (p.EngineFlags().m_flags & 0x20) DrawZone_XtraRender(&Placements().Data()[idx], frames);
                    }
                    EXGDiData *pG = (EXGDiData *)p.GDiDataArray(pL->m_lod).m_00;
                    u16 flags = pL->m_flags;
                    if (flags & 4) EXBaseRenderEnv::ms_RenderInfo.SetVertexSet(0, 1, pL->m_morph);
                    u32 fade = flags & 1;
                    if (fade) {
                        EXBaseRenderEnv::ms_DistanceFadeStart = pL->m_fadeStart;
                        EXBaseRenderEnv::ms_DistanceFade = 1;
                        EXBaseRenderEnv::ms_DistanceFadeEnd = pL->m_fadeEnd;
                        EXBaseRenderEnv::ms_DistanceFadeLayerMask = (u8)(flags >> 8);
                        if ((pL->m_flags & 2) | bFade) EXBaseRenderEnv::ms_RenderInfo.m_renderFlags.BitSet(0x10);
                        else EXBaseRenderEnv::ms_RenderInfo.m_renderFlags.BitClr(0x10);
                    }
                    pE->Draw(pG, 0);
                    if (fade) {
                        if (bFade) EXBaseRenderEnv::ms_RenderInfo.m_renderFlags.BitSet(0x10);
                        else EXBaseRenderEnv::ms_RenderInfo.m_renderFlags.BitClr(0x10);
                        EXBaseRenderEnv::ms_DistanceFade = 0;
                    }
                    EXBaseRenderEnv::ms_RenderInfo.SetVertexSet(0, 1, 0.0f);
                }
            }
            used |= bit;
        }
    }
    EXBaseDisplay::SetObjectToWorld(objToWorld);
    pDisp->UpdateTransforms();
    EXBaseRenderEnv::ms_RenderInfo.SetLightSet(0, 1, 0.0f);
    EXBaseRenderEnv::ms_DistanceFade = 0;
    EXBaseRenderEnv::ms_DistanceFadeLayerMask = 0;
}

// Screen-space test of a box against a view portal. Each of the 8 corners sets bit i in one of
// four bytes: left of the portal, right, above, below (corners behind the eye only count as
// behind). Fails when all corners are behind the near depth or all are off the same side.
int EXGeoMap::ClipPortal(const EXViewPortal &vp, const EXBoundsBox &box, const EXMatrix &m)
{
    u32 behind = 0;
    u32 clip = 0;
    for (int i = 0; i < 8; i++) {
        const float *pC = (const float *)&box;
        EXMaths::vec_xyzw p;
        p.x = pC[(i & 1) * 4];
        p.y = pC[(i & 2) * 2 + 1];
        p.z = pC[(i & 4) + 2];
        p.w = 1.0f;
        p = (const EXMaths::mat_44 &)m * p;
        u32 bit = 1 << i;
        if (vp.m_10 > p.w) behind |= bit;
        if (0.0f >= p.w) continue;
        EXVector d = EXBaseDisplay::PrimToDisp(EXVector(p * (1.0f / p.w)));
        if (vp.m_x0 > d.x) clip |= bit;
        else if (d.x > vp.m_x1) clip |= bit << 8;
        if (vp.m_y0 > d.y) clip |= bit << 16;
        else if (d.y > vp.m_y1) clip |= bit << 24;
    }
    if ((u8)behind == 0xff) return 0;
    if ((u8)clip == 0xff) return 0;
    clip >>= 8;
    if ((u8)clip == 0xff) return 0;
    clip >>= 8;
    if ((u8)clip == 0xff) return 0;
    if ((clip & 0xff00) == 0xff00) return 0;
    return -1;
}

// Portal faces of the zone the view is in, faded in by depth: a portal with a target face and a
// minimum depth (>= 1) is drawn once its nearest vertex comes within 3 units of that depth, each
// vertex's alpha ramping from 0 to half the face colour's alpha over those 3 units. The face is
// drawn back to front (reversed winding); the alpha ramp keeps the forward vertex order.
void EXGeoMap::DrawPortals(const EXMapZoneInfo *pZone, const EXRenderInfo &)
{
    EXWnd *pWnd = EXBaseDisplay::ActiveWnd();
    if (!pWnd) return;
    EXMaths::mat_44 cull = EXBaseDisplay::ObjectToCull();
    EXMapOn on = pZone->MapOn();
    EXGeoZone *pGZone = MapZone(on.Zone());
    unsigned int lastTex = 0xffffffff;
    EXTexture *pTex = 0;
    int i = 0;
    while (i < pGZone->NumPortals()) {
        const EXGeoPortalInfo *pHead = pGZone->PortalInfo(i);
        for (int j = 0; j < pHead->m_Extra + 1; j++, i++) {
            const EXGeoPortal &portal = m_Portals[pHead[j].m_Portal];
            if (!(portal.m_MinDepth >= 1.0f)) continue;
            const EXGeoFace *pFace = portal.GeoFace();
            if (!pFace) continue;
            unsigned int n = pFace->VertexCount();
            if (n >= 29) n = 29;
            float maxZ = 0.0f;
            float minZ = 100000.0f;
            EXFixedArray<EXMaths::vec_xyzw, 29> pos;
            const EXPrimVert *pSrc = (const EXPrimVert *)pFace->Vtx();
            unsigned int k;
            for (k = 0; k < n; k++) {
                EXMaths::vec_xyzw p;
                p.x = pSrc[k].x;
                p.y = pSrc[k].y;
                p.z = pSrc[k].z;
                p.w = pSrc[k].w;
                EXMaths::vec_xyzw t = pos[k] = EXVector(cull * p);
                maxZ = max(t.get_z(), maxZ);
                minZ = min(t.get_z(), minZ);
            }
            float nearZ = portal.m_MinDepth - 3.0f;
            if (nearZ > maxZ) continue;
            EXFixedArray<EXPrimVert, 29> verts;
            for (k = 0; k < n; k++) {
                const EXPrimVert &src = pSrc[n - 1 - k];
                *(EXMaths::vec_xyzw *)&verts[k].x = *(const EXMaths::vec_xyzw *)&src.x;
                verts[k].u = src.u;
                verts[k].v = src.v;
                ex::RGBA col = src.m_Color;
                float f = limit((pos[k].z - nearZ) * (1.0f / 3.0f), 0.0f, 1.0f);
                col.m_A = (u8)((int)((float)(int)((unsigned int)col >> 24) * f) / 2);
                verts[k].m_Color = col;
            }
            unsigned int ref = pFace->TextureRef();
            if (!pTex || ref != lastTex) {
                lastTex = ref;
                pTex = EXBaseDisplay::GeoHeader()->Texture(ref);
            }
            pWnd->SetTriFanBackFaceCulling((s8)((pFace->Flags() >> 16) & 1));
            pWnd->SetTriFanBlendMode(0);
            pWnd->SelectTriFanTexture(pTex);
            pWnd->DrawTriFan(n, verts);
            pWnd->SetTriFanBackFaceCulling(0);
        }
    }
}

// Deloads the sub-zone sections (type 0x23) of every loaded map zone entity.
// NEAR: 30 diff lines; stack slot order of z/i/pEnt, and retail tail-merges the inlined
// SectionHeader lookup (shared shl/add).
void EXGeoMap::OnDelete(EXGeoHeader *pHdr, EXGeoObjectHeader *)
{
    for (unsigned int z = 0; z < m_NumZones; z++) {
        const EXGeoRefPointerHeader &ref = pHdr->RefPointerList()[m_Zones[z].SubZoneHashRef()];
        if (!pHdr->IsSectionLoaded(ref.Section(), 1)) continue;
        EXGeoMapZoneEntity *pEnt = (EXGeoMapZoneEntity *)ref.m_Addr[0].m_pObject;
        if (!pEnt) continue;
        for (unsigned int i = 0; i < pEnt->NumSubZones(); i++) {
            const EXGeoRefPointerHeader &sub = pHdr->RefPointerList()[pEnt->SubZoneEntity(i)->m_RefIndex];
            if ((pHdr->SectionHeader(sub.Section())->HashCode() & 0x7f000000) == 0x23000000)
                pHdr->DeLoadSectionFile(sub.Section());
        }
    }
}

// The placement's world matrix: rotation (euler, order 6) times scale, then the position.
EXMatrix EXGeoPlacement::CalcMatrix() const
{
    EXMaths::mat_44 rot;
    rot.set_rotate(EXVector(m_rot[0], m_rot[1], m_rot[2], 0.0f), 6);
    EXMaths::mat_44 scale;
    scale.set_scale(EXVector(m_scale[0], m_scale[1], m_scale[2], 1.0f));
    EXMatrix m;
    *(EXMaths::mat_44 *)&m = scale * rot;
    m.r[3].x = m_pos[0];
    m.r[3].y = m_pos[1];
    m.r[3].z = m_pos[2];
    m.r[3].w = 1.0f;
    return m;
}

// Debug: draws every cluster of the map with its runtime state from the zone info table.
void EXGeoMap::Debug_DrawClusters(EXMapZoneInfoTable *pTable) const
{
    EXGeoHeader *pHdr = EXBaseDisplay::GeoHeader();
    if (!pHdr) return;
    EXMatrix identity;
    identity.SetIdentity();
    EXBaseDisplay::SetObjectToWorld(identity);
    EXBaseDisplay::GetDisplay()->UpdateTransforms();
    EXCluster *pCluster = pTable->ClusterArray().Clusters();
    for (u32 i = 0; i < (u32)m_Clusters.Size(); i++, pCluster++) pCluster->Draw(pHdr, &m_Clusters[i]);
}

// Adds the enabled placements of a placement tree leaf.
static inline void AddPlacementLeaf(EXGeoPlacementTree *pLeaf, EXu32Array &out)
{
    for (u32 i = 0; i < pLeaf->m_NumItems; i++)
        if (pLeaf->Item(i).m_Flags & 8) out.Add(pLeaf->Item(i).m_Index);
}

// Collects the placements whose tree nodes the ray's bounds and segment pass through.
void DoRayCast_PlacementTree(EXGeoPlacementTree *pTree, const EXRayCastTemp &t, EXu32Array &out)
{
    if (!(pTree->m_Flags & 8)) return;
    if (!pTree->m_NumChildren) {
        AddPlacementLeaf(pTree, out);
        return;
    }
    for (u32 i = 0; i < pTree->m_NumChildren; i++) {
        EXGeoPlacementTreeChild &c = pTree->Child(i);
        if (!c.m_Box.Intersect(t.m_bounds)) continue;
        if (!c.m_Box.Intersect(t.m_start, t.m_dir)) continue;
        EXGeoPlacementTree *pNode = c.m_pNode;
        if (!(pNode->m_Flags & 8)) continue;
        if (!pNode->m_NumChildren) AddPlacementLeaf(pNode, out);
        else DoRayCast_PlacementTree(pNode, t, out);
    }
}

// Collects the placements whose tree nodes a sphere touches.
void DoCollision_PlacementTree(EXGeoPlacementTree *pTree, const EXVector &p, float r, EXu32Array &out)
{
    if (!(pTree->m_Flags & 8)) return;
    if (!pTree->m_NumChildren) {
        AddPlacementLeaf(pTree, out);
        return;
    }
    for (u32 i = 0; i < pTree->m_NumChildren; i++) {
        EXGeoPlacementTreeChild &c = pTree->Child(i);
        if (!c.m_Box.Intersect(p, r)) continue;
        EXGeoPlacementTree *pNode = c.m_pNode;
        if (!(pNode->m_Flags & 8)) continue;
        if (!pNode->m_NumChildren) AddPlacementLeaf(pNode, out);
        else DoCollision_PlacementTree(pNode, p, r, out);
    }
}

// Fits a zone's placement tree to its contents: a leaf's box takes in the world bounds of each
// placement's entity; a node's children are fitted first, their stored boxes updated, then taken
// in. Boxes only grow (each child starts from its stored box).
void EXGeoMap::Initialise_PlacementTree(EXGeoPlacementTree *pTree, EXGeoHeader *pHdr, EXBoundsBox &box)
{
    if (!pTree->m_NumChildren) {
        for (u32 i = 0; i < pTree->m_NumItems; i++) {
            const EXGeoPlacement &pl = m_Placements[pTree->Item(i).m_Index];
            const EXGeoEntity *pEnt = MapGeoEntityHeader(pHdr, pl.m_entity)->Object(0);
            if (!pEnt) continue;
            EXBoundsBox b = pEnt->BoundsBox();
            b._ApplyXForm(pl.CalcMatrix());
            box.Accumulate(b);
        }
    } else {
        for (u32 i = 0; i < pTree->m_NumChildren; i++) {
            EXGeoPlacementTreeChild &c = pTree->Child(i);
            EXBoundsBox b;
            b.m_vctMin.set(c.m_Box.m_vctMin.x, c.m_Box.m_vctMin.y, c.m_Box.m_vctMin.z, 0.0f);
            b.m_vctMax.set(c.m_Box.m_vctMax.x, c.m_Box.m_vctMax.y, c.m_Box.m_vctMax.z, 0.0f);
            Initialise_PlacementTree(c.m_pNode, pHdr, b);
            c.m_Box.Set(b);
            box.Accumulate(b);
        }
    }
}

// Collision sphere trees (EXGeoColSphere_Table): a node lists its child count, the index of its
// first sphere, then one self-relative 16-bit offset per child. A node with no children is a leaf
// whose box (two sphere slots: min, max) is tested. Children are visited depth first, last child
// first, with a stack in the scratch pad of {node, hit mask, next child}.
struct EXGeoColSphere_NodeList {
    u16 m_NumChildren;  // 0x00
    u16 m_First;        // 0x02 first sphere (a leaf: its box)
    s16 m_Child[1];     // 0x04 self-relative offsets
    EXGeoColSphere_NodeList *Child(int i) { return (EXGeoColSphere_NodeList *)((u8 *)&m_Child[i] + m_Child[i]); }
};
struct ColSphereStackEntry {
    EXGeoColSphere_NodeList *m_pNode;
    u16 m_Mask;
    s16 m_Next;
};

static inline const MXBoundsBox &ColSphereBox(EXBoundsSphere *pSpheres, int i) { return *(const MXBoundsBox *)&pSpheres[i]; }

// The shared tree walk. Query provides HitSphere(sphere) and HitBox(box). Returns whether any
// leaf box reached through hit spheres is hit.
template <class Query> static __forceinline s8 WalkCollisionSpheres(EXGeoColSphere_NodeList *pNodes, EXBoundsSphere *pSpheres,
                                                             const Query &q)
{
    EXScratchPad pad;
    ColSphereStackEntry *pStack = (ColSphereStackEntry *)pad.Claim(0x100);
    // A root leaf tests the box in the first sphere slots.
    if (!pNodes->m_NumChildren) return q.HitBox(ColSphereBox(pSpheres, 0));
    int sp = 0;
    EXGeoColSphere_NodeList *pNode = pNodes;
    for (;;) {
        // Visit pNode: which child spheres are hit.
        int n = pNode->m_NumChildren;
        u32 mask = 0;
        for (int i = 0; i < n; i++)
            if (q.HitSphere(pSpheres[pNode->m_First + i])) mask |= 1 << i;
        if (mask) {
            pStack[sp].m_pNode = pNode;
            pStack[sp].m_Mask = (u16)mask;
            pStack[sp].m_Next = (s16)n;
        } else {
            n = 0;
        }
        for (;;) {
            // Scan the remaining hit children, last first: descend into nodes, test leaves.
            s8 bDescend = 0;
            while (n > 0) {
                n--;
                if (!(mask & (1 << n))) continue;
                EXGeoColSphere_NodeList *pChild = pNode->Child(n);
                if (pChild->m_NumChildren) {
                    pStack[sp].m_Next = (s16)n;
                    sp++;
                    pNode = pChild;
                    bDescend = 1;
                    break;
                }
                if (q.HitBox(ColSphereBox(pSpheres, pChild->m_First))) return 1;
            }
            if (bDescend) break;
            // Back up to the nearest ancestor with children left.
            do {
                if (--sp < 0) return 0;
                n = pStack[sp].m_Next;
            } while (n < 1);
            mask = pStack[sp].m_Mask;
            pNode = pStack[sp].m_pNode;
        }
    }
}

struct ColSphereRayQuery {
    const EXLineSeg &m_Seg;
    ColSphereRayQuery(const EXLineSeg &l) : m_Seg(l) {}
    s8 HitSphere(const EXBoundsSphere &s) const
    {
        EXMaths::vec_xyzw c;
        c.set(s.m_x, s.m_y, s.m_z, 0.0f);
        return EXSqrDistance(c, m_Seg, 0) < s.m_r * s.m_r;
    }
    s8 HitBox(const MXBoundsBox &b) const { return b._Intersect(m_Seg.m_Origin, m_Seg.m_Direction); }
};

// The sphere test subtracts the query point's w from each radius (w is normally 0).
struct ColSphereSphereQuery {
    const EXVector &m_C;
    float m_R;
    ColSphereSphereQuery(const EXVector &c, float r) : m_C(c), m_R(r) {}
    s8 HitSphere(const EXBoundsSphere &s) const
    {
        float dx = s.m_x - m_C.x, dy = s.m_y - m_C.y, dz = s.m_z - m_C.z;
        float rr = (s.m_r - m_C.w) + m_R;
        return dy * dy + dz * dz + dx * dx <= rr * rr;
    }
    s8 HitBox(const MXBoundsBox &b) const { return b._Intersect(m_C, m_R); }
};

// Whether a line segment touches any leaf box reached through spheres it passes within.
s8 DoRayCast_CollisionSpheres(EXGeoColSphere_NodeList *pNodes, EXBoundsSphere *pSpheres, const EXLineSeg &l)
{
    return WalkCollisionSpheres(pNodes, pSpheres, ColSphereRayQuery(l));
}

// Whether a sphere touches any leaf box reached through overlapping spheres.
s8 DoCollision_CollisionSpheres(EXGeoColSphere_NodeList *pNodes, EXBoundsSphere *pSpheres, const EXVector &c, float r)
{
    return WalkCollisionSpheres(pNodes, pSpheres, ColSphereSphereQuery(c, r));
}

// The node nearest p (first of equals), or -1 for an empty path.
int EXGeoPath::GetPathNode(const EXVector &p) const
{
    int n = NumNodes();
    if (n <= 0) return -1;
    const float *pPos = (const float *)Node(0);
    float best = (pPos[1] - p.y) * (pPos[1] - p.y) + (pPos[2] - p.z) * (pPos[2] - p.z) + (pPos[0] - p.x) * (pPos[0] - p.x);
    int found = 0;
    for (int i = 1; i < NumNodes(); i++) {
        pPos = (const float *)Node(i);
        float d = (pPos[1] - p.y) * (pPos[1] - p.y) + (pPos[2] - p.z) * (pPos[2] - p.z) + (pPos[0] - p.x) * (pPos[0] - p.x);
        if (d < best) {
            best = d;
            found = i;
        }
    }
    return found;
}

// The path (other than type 2 paths) with the node nearest v, ignoring nodes with any of the given
// flags; -1 when none qualifies.
int EXGeoMap::GetPathIndex(const EXVector &v, unsigned int flags) const
{
    int found = -1;
    float best = 3.4028235e38f;
    for (int i = 0; i < (short)m_Paths.Size(); i++) {
        const EXGeoPath &path = m_Paths[i];
        if (path.Type() == 2) continue;
        for (int j = 0; j < path.NumNodes(); j++) {
            const EXGeoPathNode *pNode = path.Node(j);
            if (pNode->Flags() & flags) continue;
            const float *pPos = (const float *)pNode;
            float d = (v.y - pPos[1]) * (v.y - pPos[1]) + (v.z - pPos[2]) * (v.z - pPos[2]) + (v.x - pPos[0]) * (v.x - pPos[0]);
            if (d < best) {
                found = i;
                best = d;
            }
        }
    }
    return found;
}

// Ray against one triangle (a, b, c): the plane hit at t in [0, best] counts when it lies inside
// all three edges (tested as (edge x normal) . (hit - edge start) <= 0). On a hit best and hit are
// updated. Forced inline: retail expands it in both RayCast loops.
static __forceinline void PortalRayTri(const EXVector &a, const EXVector &b, const EXVector &c, const EXLineSeg &l,
                                       float &best, EXVector &hit)
{
    EXPlane plane(a, b, c);
    float t;
    if (!plane.Intersection(t, l)) return;
    if (t < 0.0f || t > best) return;
    EXVector p = l.m_Origin + l.m_Direction * t;
    const EXVector *v[3] = {&a, &b, &c};
    // Edges in retail's order: c->a from a, a->b from b, b->c from c.
    static const int s_From[3] = {2, 0, 1};
    for (int k = 0; k < 3; k++) {
        const EXVector &s = *v[s_From[k]];
        const EXVector &e = *v[k];
        float ex = e.x - s.x, ey = e.y - s.y, ez = e.z - s.z;
        float cx = plane.C * ey - ez * plane.B;
        float cy = ez * plane.A - plane.C * ex;
        float cz = plane.B * ex - ey * plane.A;
        if (cz * (p.z - e.z) + cy * (p.y - e.y) + cx * (p.x - e.x) > 0.0f) return;
    }
    best = t;
    hit = p;
}

// Nearest hit of a line segment on the portal (t in 0..1, 1 for none): on its face's triangle
// fan when it has a face, otherwise on its two corner triangles.
float EXGeoPortal::RayCast(const EXLineSeg &l, EXVector &hit) const
{
    float best = 1.0f;
    const EXGeoFace *pFace = m_Target;
    if (pFace) {
        const EXVector &v0 = *(const EXVector *)&pFace->Vtx()[0];
        for (u32 i = 0; i < pFace->VertexCount() - 2; i++)
            PortalRayTri(v0, *(const EXVector *)&pFace->Vtx()[i + 1], *(const EXVector *)&pFace->Vtx()[i + 2], l, best, hit);
        return best;
    }
    EXVector v0(m_Verts[0].x, m_Verts[0].y, m_Verts[0].z, 0.0f);
    for (int i = 0; i < 2; i++) {
        EXVector v1(m_Verts[i + 1].x, m_Verts[i + 1].y, m_Verts[i + 1].z, 0.0f);
        EXVector v2(m_Verts[i + 2].x, m_Verts[i + 2].y, m_Verts[i + 2].z, 0.0f);
        PortalRayTri(v0, v1, v2, l, best, hit);
    }
    return best;
}
