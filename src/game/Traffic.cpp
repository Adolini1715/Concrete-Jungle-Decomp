#include "FEText.h"
#include "TrafficSystem.h"
#include "EXMemory.h"
#include "EXNew.h"
#include "TrafficLight.h"
#include "XApp.h"
#include "EXItemAnimator_Entity.h"
#include "EXGeoFile.h"
#include "EXLineSeg.h"
#include "XItem.h"
#include "EXTrigger.h"
#include "Vehicle.h"
#include "VehicleTypes.h"
#include "XItemHandler_Objective.h"
#include "EXRand.h"
#include "ActorOptions.h"
#include "ActorUtil.h"
#include "EXSphere.h"
#include "EXIntersection.h"

class XGameWnd;
class XApp;
extern XGameWnd *gpGameWnd[];  // TODO stand-in (GameWnd.obj)
extern XApp theApp;
extern unsigned int gCurrentMap_GeoHash;

void updateVehicleSubFiles();
void TrafficSpawnerUpdateVisibility();
signed char FindSpawnPlaceOnLink(Vehicle *, TrafficPathLink *, TrafficLocation &, unsigned char);
void TrafficMissionUpdate();
extern float framesPerHour;
extern float oneKDivFramesPerHour;
extern float framesPerHourDivRateMul;
extern float lastFrameTime;

XItemEnv *pWorld;
XGameWnd *pWnd;
XApp *pApp;
int nVisPathLinks;

inline TrafficSysAlloc &alloc() { return _theTrafficSysAlloc; }
inline TrafficTriggers &trig() { return _theTrafficTriggers; }
inline TrafficSystem *sys() { return alloc().getSys(); }
inline TrafficPopulace &pop() { return sys()->getPopulace(); }
inline XItemEnv *getWorld() { return pWorld; }
inline unsigned int getSysUpdateCount() { return sys()->getUpdateCount(); }

inline TrafficLane *TrafficRoad::getLane(unsigned char dir, unsigned char i)
{
    return sys()->getLane(m_lanes[dir][i]);
}

inline float angle2d_between_vectors(const EXVector &a, const EXVector &b)
{
    return (float)atan2((double)(b.get_x() * a.get_z()) - b.get_z() * a.get_x(), (double)(a.get_x() * b.get_x()) + b.get_z() * a.get_z());
}

inline float fixAngleLeft(float a)
{
    if (a > 0.0f) a = 0.0f - a;
    if (-3.1415927f >= a) a = -6.2831855f - a;
    return a;
}

inline float fixAngleRight(float a)
{
    if (0.0f > a) a = 0.0f - a;
    if (a >= 3.1415927f) a = 6.2831855f - a;
    return a;
}

// NEAR: retail forms the lane index as (lane + dir*4) + road*10 (39 bytes).
inline TrafficLane *TrafficPathLink::getLane() const
{
    unsigned char lane = (u8)getLaneIndex();
    unsigned char dir = getDir();
    return getRoad()->getLane(dir, lane);
}

// NEAR: lane index order, see getLane.
inline float TrafficPathLink::getDistFromLaneStartXZ(const EXVector3 *p, unsigned int index, signed char b)
{
    TrafficLane *pLane = getLane();
    float dist = getDistFromLinkStartXZ(p, b);
    if (index == 0x9999) index = getIndex();
    if (index) dist = pLane->getPathLink(index)->getExtraLinkData()->getDistFromLaneStartXZ() + dist;
    return dist;
}

// NEAR: retail recomputes the lane address for each getLane() (see getLane).
inline TrafficPathLink *TrafficPathLink::getNextLink() const
{
    if (getIndex() < getLane()->getNumPathLinks() - 1) return getLane()->getPathLink(getIndex() + 1);
    if (getLane()->getPathLink(0)->getNodeAPtr()->isLoop()) return getLane()->getPathLink(0);
    return 0;
}

inline TrafficPathLink *TrafficPathLink::getPrevLink() const
{
    if (getIndex() > 0) return getLane()->getPathLink(getIndex() - 1);
    if (getLane()->getPathLink(0)->getNodeAPtr()->isLoop()) return getLane()->getLastPathLink();
    return 0;
}

inline signed char Vehicle::isUpdated() const
{
    return getSysUpdateCount() == m_updateCount;
}

// NEAR: follows firstIntersection's register choice (3 bytes).
inline void TrafficBoxJunction::update()
{
    if (!m_trafficLightSeqLen) return;
    m_f34 += frameLen;
    unsigned int i;
    for (TrafficIntersection *p = firstIntersection(i); p; p = nextIntersection(i)) {
        TrafficLight *pLight = p->getTrafficLight();
        if (pLight && m_f34 > pLight->getDuration()) {
            if (pLight->getState() <= 1 && !m_numVehicles) {
                nextTrafficLightSeq();
                m_f34 = 0.0f;
            } else {
                m_f34 -= 2.0f;
                if (m_f34 < 0.0f) m_f34 = 0.0f;
            }
            return;
        }
    }
}

void TrafficDraw()
{
}

void TrafficInitTriggers()
{
    _theTrafficTriggers.reset();
}

signed char TrafficIsInit()
{
    return _theTrafficSysAlloc.getSys() != 0;
}

TrafficPopulace &TrafficGetPopulace()
{
    return _theTrafficSysAlloc.getSys()->getPopulace();
}

TrafficLane::~TrafficLane()
{
}

void TrafficLane::addIntersection(TrafficIntersection *p)
{
    m_intersections[m_numIntersections] = p;
    m_numIntersections++;
}

void TrafficLane::addVehicle(Vehicle *, float, signed char)
{
}

void TrafficLane::addVehicleNEW(VehicleListItem *pItem, float dist)
{
    pItem->SetList(&m_vehicleList);
    pItem->setDist(dist);
    pItem->setLane(this);
    VehicleListItem *p = (VehicleListItem *)m_vehicleList.m_pHead;
    while (p) {
        VehicleListItem *pNext = p->Next();
        if (p == pItem) {
            p->Remove();
        } else if (dist > p->getDist()) {
            ((EXDList &)m_vehicleList).Insert((EXDListItem *)p, (EXDListItem *)pItem);
            return;
        }
        p = pNext;
    }
    m_vehicleList.AddTail(pItem);
}

void TrafficLane::refreshVehicleList()
{
}

TrafficLaneSeg *TrafficLane::addAIBlockedSeg(float start, float end, unsigned char ref)
{
    for (u32 i = 0; i < 2; i++) {
        if (!m_aiBlockedSegs[i].isSet()) {
            TrafficLaneSeg *pSeg = &m_aiBlockedSegs[i];
            pSeg->set(start, end, ref);
            m_numAIBlockedSegs++;
            return pSeg;
        }
    }
    return 0;
}

TrafficLaneSeg *TrafficLane::nextAIBlockedSeg(unsigned int &i)
{
    for (i++; i < 2; i++) {
        if (m_aiBlockedSegs[i].isSet()) return &m_aiBlockedSegs[i];
    }
    return 0;
}

void TrafficLane::delAIBlockedSeg(unsigned char seg)
{
    for (VehicleListItem *p = (VehicleListItem *)m_vehicleList.m_pHead; p; p = p->Next()) {
        Vehicle *pVehicle = p->m_pVehicle;
        if (pVehicle->m_move.isLaneBlockedAtSeg(seg)) pVehicle->m_move.cbLaneSegUnblocked();
    }
    m_aiBlockedSegs[seg].reset();
    m_numAIBlockedSegs--;
}

float TrafficLane::getFreeTailDistXZ()
{
    return 1000.0f;
}

Vehicle *TrafficLane::getFirstVehicle() const
{
    VehicleListItem *p = (VehicleListItem *)m_vehicleList.m_pHead;
    return p ? p->m_pVehicle : 0;
}

Vehicle *TrafficLane::getLastVehicle() const
{
    VehicleListItem *p = (VehicleListItem *)m_vehicleList.m_pTail;
    return p ? p->m_pVehicle : 0;
}

Vehicle *TrafficLane::getVehicle(unsigned int n)
{
    unsigned int i = 0;
    for (VehicleListItem *p = (VehicleListItem *)m_vehicleList.m_pHead; p; p = p->Next()) {
        if (n == i++) return p->m_pVehicle;
    }
    return 0;
}

inline void TrafficLaneDest::reset()
{
    m_pIntersection = 0;
    m_pRoad = 0;
    m_pLane = 0;
    m_pathLink = 0xffff;
    m_pos.Set(0.0f, 0.0f, 0.0f);
    m_distFromLinkStartXZ = -999.0f;
    m_distFromLaneStartXZ = -999.0f;
    m_flags = 0;
    for (u32 i = 0; i < 4; i++) m_crossedPathLinks[i] = 0xffff;
    for (u32 j = 0; j < 4; j++) m_crossedDistFromLinkStartXZ[j] = 999.0f;
    m_angle = 0.0f;
}

unsigned char TrafficLaneDest::getDir() const
{
    return m_pLane->getDir();
}

TrafficIntersection::~TrafficIntersection()
{
}

void TrafficIntersection::reset()
{
    m_index = 0x9999;
    m_pRoad = (TrafficRoad *)0x99999999;
    m_pathLink = 0xffff;
    m_pIntersectionB = 0;
    m_pIntersectionA = 0;
    m_flags = 0;
    m_endDir = 0x99;
    m_startDir = 0x99;
    for (u32 i = 0; i < 3; i++) m_relativeIntersections[i] = 0;
    m_endIntersectDir = 0x99;
    m_pEndIntersectRoad = 0;
    m_endIntersectLink = 0xffff;
    m_pBoxJunction = 0;
    m_aiEntryBlockRef = 0xff;
    m_aiExitBlockRef = 0xff;
    for (u32 j = 0; j < 4; j++) m_pendingVehicles[j] = 0;
    m_pTrafficLight = 0;
}

TrafficRoad::~TrafficRoad()
{
}

void TrafficRoad::reset()
{
    m_index = 0x9999;
    m_numLanes[0] = 0;
    m_numLanes[1] = 0;
}

TrafficPath *TrafficLocation::getPath() const
{
    return _theTrafficSysAlloc.getSys()->getLanesPath(m_pLane->getDir());
}

void TrafficPopulace::removeVehicle(Vehicle *pVehicle)
{
    pVehicle->remove();
    m_nVehicles--;
}

TrafficLight::~TrafficLight()
{
    for (u32 i = 0; i < 4; i++) {
        if (m_items[i]) {
            if (pWorld->m_pItemLists->IsMember(&m_items[i]->m_node)) pWorld->RemoveItem(m_items[i]);
            delete m_items[i];
            m_items[i] = 0;
        }
    }
}

void TrafficLight::changeState()
{
    if (m_state == 0) {
        m_state = 3;
        m_timer = 0.0f;
    } else if (m_state == 1) {
        m_state = 2;
        m_timer = 0.0f;
    }
}

signed char trValidate(unsigned int, unsigned int *, const char *, ...)
{
    return 1;
}

void trTracef(const char *, ...)
{
}

void trTraceRawf(const char *, ...)
{
}

void initTraceFile()
{
}

inline void TrafficTriggers::add(EXTrigger *p)
{
    TrafficTrigger &t = m_triggers[m_numTriggers++];
    t.m_type = p->m_Type;
    t.m_flags = p->m_Unk4c;
    t.m_pos = p->m_Position;
    t.m_rot = p->m_Rotation;
    for (int i = 0; i < 16; i++) t.m_data[i] = p->m_Data[i];
    for (int j = 0; j < 8; j++) t.m_links[j] = p->m_Links[j];
}

void TrafficAddTrigger(EXTrigger *pTrigger)
{
    _theTrafficTriggers.add(pTrigger);
}

TrafficLane *TrGetLane(unsigned short id)
{
    return _theTrafficSysAlloc.getSys()->getLane(id);
}

// NEAR: structure matches; retail keeps the dot products in a different register/spill order.
float dist3D_Point_to_LineSegment(EXVector &p, EXLineSeg &seg)
{
    EXVector w = p - seg.m_origin;
    float c1 = w.Dot(seg.m_dir);
    if (c1 > 0.0f) {
        float c2 = seg.m_dir.Dot(seg.m_dir);
        if (1e-7f >= c2) return 0.0f;
        if (c1 >= c2) {
            w -= seg.m_dir;
        } else {
            float b = c1 / c2;
            w = w - seg.m_dir * b;
        }
    }
    return w.LengthXYZ();
}

// NEAR: xmm register numbering differs.
float dist2D_Point_to_LineSegment(const EXVector2 &p, const EXVector2 &a, const EXVector2 &b)
{
    float wx = p.x - a.x;
    float wy = p.y - a.y;
    float vx = b.x - a.x;
    float vy = b.y - a.y;
    float c1 = vx * wx + vy * wy;
    if (c1 > 0.0f) {
        float c2 = vx * vx + vy * vy;
        if (1e-7f >= c2) return 0.0f;
        if (!(c1 >= c2)) {
            float t = c1 / c2;
            vx *= t;
            vy *= t;
        }
        wx -= vx;
        wy -= vy;
    }
    return EXSqrt(wx * wx + wy * wy);
}

static inline float trAbs(float f) { return f < 0.0f ? 0.0f - f : f; }

// Closest distance between segments [a0,a1] and [b0,b1] (the classic parametric solve, clamped
// to both segments; 1e-8 is the parallel threshold).
float dist3D_LineSegment_to_LineSegment(const EXVector &a0, const EXVector &a1, const EXVector &b0, const EXVector &b1)
{
    EXVector u = a1 - a0;
    EXVector v = b1 - b0;
    EXVector w = a0 - b0;
    float a = u.DotXYZ(u);
    float b = u.DotXYZ(v);
    float c = v.DotXYZ(v);
    float d = u.DotXYZ(w);
    float e = v.DotXYZ(w);
    float D = a * c - b * b;
    float sN, sD = D;
    float tN, tD = D;
    if (D < 1e-8f) {
        sN = 0.0f;
        sD = 1.0f;
        tN = e;
        tD = c;
    } else {
        sN = b * e - c * d;
        tN = a * e - b * d;
        if (sN < 0.0f) {
            sN = 0.0f;
            tN = e;
            tD = c;
        } else if (sN > sD) {
            sN = sD;
            tN = e + b;
            tD = c;
        }
    }
    if (tN < 0.0f) {
        tN = 0.0f;
        if (-d < 0.0f) sN = 0.0f;
        else if (-d > a) sN = sD;
        else {
            sN = -d;
            sD = a;
        }
    } else if (tN > tD) {
        tN = tD;
        if (-d + b < 0.0f) sN = 0.0f;
        else if (-d + b > a) sN = sD;
        else {
            sN = -d + b;
            sD = a;
        }
    }
    float sc = trAbs(sN) < 1e-8f ? 0.0f : sN / sD;
    float tc = trAbs(tN) < 1e-8f ? 0.0f : tN / tD;
    EXVector dP = w + u * sc - v * tc;
    return dP.LengthXYZ();
}

// Segments [a0,a1] and [b0,b1] come within dist. When pOut is given it gets a point beside the
// a-end nearer to segment b, stepped sideways off a's direction by that end's distance (on
// whichever side is nearer b), at b1's height.
signed char doLineSegsIntersect(const EXVector &a0, const EXVector &a1, const EXVector &b0, const EXVector &b1,
                                EXVector *pOut, float dist)
{
    if (!(dist3D_LineSegment_to_LineSegment(a0, a1, b0, b1) < dist)) return 0;
    if (pOut) {
        EXLineSeg seg;
        seg.m_origin = b0;
        seg.m_dir = b1 - b0;
        EXVector p = a1;
        float d1 = dist3D_Point_to_LineSegment(p, seg);
        p = a0;
        float d0 = dist3D_Point_to_LineSegment(p, seg);
        const EXVector *pEnd = &a1;
        float d = d1;
        if (d0 < d1) {
            pEnd = &a0;
            d = d0;
        }
        float ang = (float)atan2(a1.x - a0.x, a1.z - a0.z);
        float cx = (float)cos(ang) * d;
        float sz = (float)sin(ang) * d;
        if (pEnd == &a0) {
            cx = 0.0f - cx;
            sz = 0.0f - sz;
        }
        p.x = pEnd->x - sz;
        p.y = b1.y;
        p.z = pEnd->z - cx;
        p.w = 0.0f;
        if (dist3D_Point_to_LineSegment(p, seg) > d) {
            p.x = cx * 2.0f + p.x;
            p.z = sz * 2.0f + p.z;
        }
        pOut->z = p.z;
        pOut->x = p.x;
        pOut->y = p.y;
        pOut->w = 0.0f;
    }
    return 1;
}

// doLineSegsIntersect for two path links (by their node positions). ppNode gets the end of this
// link nearer to the other link, pOut the sidestep point beside it.
signed char TrafficPathLink::doLinksIntersect(TrafficPathLink *pOther, EXVector *pOut, TrafficPathNode **ppNode,
                                              float dist)
{
    TrafficPath *pPath = getPath();
    TrafficPath *pOtherPath = pOther->getPath();
    TrafficPathNode *pA0 = pPath->Node(m_nodeA);
    TrafficPathNode *pA1 = pPath->Node(m_nodeB);
    TrafficPathNode *pB0 = pOtherPath->Node(pOther->m_nodeA);
    TrafficPathNode *pB1 = pOtherPath->Node(pOther->m_nodeB);
    if (!(dist3D_LineSegment_to_LineSegment(EXVector(pA0->getPos(), 0.0f), EXVector(pA1->getPos(), 0.0f), EXVector(pB0->getPos(), 0.0f),
                                            EXVector(pB1->getPos(), 0.0f)) < dist))
        return 0;
    if (ppNode || pOut) {
        EXLineSeg seg;
        seg.m_origin = EXVector(pB0->getPos(), 0.0f);
        seg.m_dir = EXVector(pB1->getPos(), 0.0f) - EXVector(pB0->getPos(), 0.0f);
        EXVector p(pA1->getPos(), 0.0f);
        float d1 = dist3D_Point_to_LineSegment(p, seg);
        EXVector q(pA0->getPos(), 0.0f);
        float d0 = dist3D_Point_to_LineSegment(q, seg);
        TrafficPathNode *pEnd = pA1;
        float d = d1;
        if (d0 < d1) {
            pEnd = pA0;
            d = d0;
        }
        if (ppNode) *ppNode = pEnd;
        if (pOut) {
            float ang = (float)atan2(pA1->getPos().x - pA0->getPos().x, pA1->getPos().z - pA0->getPos().z);
            float cx = (float)cos(ang) * d;
            float sz = (float)sin(ang) * d;
            if (pEnd == pA0) {
                cx = 0.0f - cx;
                sz = 0.0f - sz;
            }
            EXVector r(pEnd->getPos().x - sz, pB1->getPos().y, pEnd->getPos().z - cx, 0.0f);
            if (dist3D_Point_to_LineSegment(r, seg) > d) {
                r.x = cx * 2.0f + r.x;
                r.z = sz * 2.0f + r.z;
            }
            pOut->z = r.z;
            pOut->x = r.x;
            pOut->y = r.y;
            pOut->w = 0.0f;
        }
    }
    return 1;
}

TrafficPathLink *TrGetPathLink(unsigned short id)
{
    if (id >= 0xfff0) return 0;
    return _theTrafficSysAlloc.getSys()->m_pathLinkPtrs[id];
}

static float s_turnRadii[7] = { 0.0f, 6.25f, 10.75f, 19.2675f, 23.75f, 4.64f, 10.65f };

float TrGetTurnRadius(unsigned char type)
{
    return s_turnRadii[type];
}

TrafficIntersection *TrafficSystem::addIntersection(TrafficPathLink *pLink)
{
    int i = m_numIntersections++;
    TrafficIntersection *p = getIntersection(i);
    p->setIndex(i);
    p->setPathLink(pLink);
    return p;
}

TrafficRoad *TrafficSystem::addRoad()
{
    int i = m_numRoads++;
    TrafficRoad *p = getRoad(i);
    p->setIndex(i);
    return p;
}

TrafficBoxJunction *TrafficSystem::getBoxJunction(unsigned short i)
{
    return &m_boxJunctions.ElementAt(i);
}

void TrafficSystem::addLoc(unsigned short i, TrafficLocation *pLoc)
{
    m_locs.SetAt(i, *pLoc);
}

void TrafficLane::reset()
{
    m_index = 0x99;
    m_pRoad = (TrafficRoad *)0x99999999;
    m_numIntersections = 0;
    m_numPathLinks = 0;
    m_dir = 0x99;
    for (u32 i = 0; i < 3; i++) {
        m_endDests[i].reset();
        m_endDests[i].setTurnDir(i);
    }
    m_numAIBlockedSegs = 0;
    for (u32 j = 0; j < 2; j++) {
        m_aiBlockedSegs[j].setIndex(j);
        m_aiBlockedSegs[j].reset();
        m_aiBlockedSegs[j].setLane(this);
    }
}

TrafficIntersection::TrafficIntersection()
{
    reset();
}

TrafficRoad::TrafficRoad()
{
    reset();
}

Vehicle *TrafficPopulace::getVehicle(unsigned short i)
{
    return &m_vehicles.ElementAt(i);
}

void TrafficPopulace::OnSubFileLoaded(unsigned int subFile)
{
    for (unsigned int i = 0; i < m_nVehicles; i++) {
        Vehicle *pVehicle = getVehicle(i);
        if (pVehicle->getTypeObject() && pVehicle->m_pItem && pVehicle->getTypeObject()->getSubFile() == subFile)
            pVehicle->OnSubFileLoaded();
    }
}

// NEAR: 2 instructions swapped in case 2 (m_validMtx store before the word or).
// NEAR: register allocation of the inlined memsets differs.
unsigned short TrafficSystem::addAdditionalPathNode(TrafficPathNode *pSrc, TrafficPathNode **ppOut)
{
    unsigned int i = m_numAdditionalPathNodes++;
    TrafficPathNode *pNode = getAdditionalPathNode(i);
    memset(pNode, 0, sizeof(TrafficPathNode));
    unsigned short id = i + 32000;
    if (ppOut) *ppOut = pNode;
    if (pSrc) memcpy(pNode, pSrc, sizeof(TrafficPathNode));
    else memset(pNode, 0, sizeof(TrafficPathNode));
    pNode->setIndex(id);
    return id;
}

unsigned short TrafficPath::findPathLinkGeoIndex(TrafficPathLink *pLink)
{
    int n = NumLinks();
    for (int i = 0; i < n; i++) {
        if ((TrafficPathLink *)EXGeoLink(i) == pLink) return i;
    }
    return 0xffff;
}

void TrafficBoxJunction::nextTrafficLightSeq()
{
    if (++m_unk1 >= m_trafficLightSeqLen) m_unk1 = 0;
    unsigned int it;
    for (TrafficIntersection *p = firstIntersection(it); p; p = nextIntersection(it)) {
        TrafficLight *pLight = p->getTrafficLight();
        if (!pLight) continue;
        if (pLight->isSequenceOn(m_unk1)) {
            if (pLight->getState() != 1) continue;
        } else if (pLight->getState() != 0) continue;
        {
            if (pLight->getState() == 0) {
                pLight->m_state = 3;
                pLight->m_timer = 0.0f;
            } else if (pLight->getState() == 1) {
                pLight->m_state = 2;
                pLight->m_timer = 0.0f;
            }
        }
    }
}

void TrafficLight::update()
{
    TrafficIntersection *pInt = m_intersections[0];
    switch (m_state) {
    case 0:
        m_items[2]->SetHasRender();
        m_items[1]->ItemFlags().BitClr(1);
        m_items[3]->ItemFlags().BitClr(1);
        if (pInt) pInt->clrAsAIExitBlocked(0xff);
        break;
    case 1:
        m_items[1]->SetHasRender();
        m_items[2]->ItemFlags().BitClr(1);
        m_items[3]->ItemFlags().BitClr(1);
        if (pInt) pInt->setAsAIExitBlocked(0xff);
        break;
    case 2:
        if (m_timer < 3.0f) {
            m_items[1]->SetHasRender();
            m_items[3]->SetHasRender();
            m_items[2]->ItemFlags().BitClr(1);
            if (pInt) pInt->setAsAIExitBlocked(0xff);
        } else {
            m_state = 0;
            update();
            if (pInt) pInt->clrAsAIExitBlocked(0xff);
        }
        break;
    case 3:
        if (m_timer < 3.0f) {
            m_items[3]->SetHasRender();
            m_items[1]->ItemFlags().BitClr(1);
            m_items[2]->ItemFlags().BitClr(1);
        } else {
            m_state = 1;
            update();
        }
        if (pInt) pInt->setAsAIExitBlocked(0xff);
        break;
    }
    m_timer += frameLen;
}

void TrafficPathLink::getLinkHorizontalOffsetVec(float offset, EXVector2 &v)
{
    if (offset > 0.0f) {
        float a = getExtraLinkData()->getAngle() + 1.5707964f;
        v.x = (float)sin(a) * offset;
        v.y = (float)cos(a) * offset;
    } else {
        float a = getExtraLinkData()->getAngle() - 1.5707964f;
        v.x = -((float)sin(a) * offset);
        v.y = -((float)cos(a) * offset);
    }
}

__declspec(noinline) signed char isPointInQuad2D(const EXVector2 &p, const EXFixedArray<EXVector2, 4> &q)
{
    float px = p.x, py = p.y;
    float ax = q[0].x, ay = q[0].y, bx = q[1].x, by = q[1].y;
    float cx = q[2].x, cy = q[2].y, dx = q[3].x, dy = q[3].y;
    float c1 = (cx - bx) * (py - by) - (cy - by) * (px - bx);
    float c0 = (bx - ax) * (py - ay) - (by - ay) * (px - ax);
    if (c0 * c1 > 0.0f) {
        float c2 = (dx - cx) * (py - cy) - (dy - cy) * (px - cx);
        if (c2 * c1 > 0.0f) {
            float c3 = (py - dy) * (ax - dx) - (ay - dy) * (px - dx);
            if (c3 * c2 > 0.0f) return 1;
        }
    }
    return 0;
}

EXFixedArray<float, 25> LastVclVisTimes;

// NEAR: retail keeps index in edi plus a byte spill in the parameter slot and returns the flag from bl.
signed char checkForAndUnspawnVehicle(Vehicle *pVehicle)
{
    unsigned int index = pVehicle->getIndex();
    signed char removed = 0;
    if (pVehicle->m_bFading == 1 && 0.0f >= pVehicle->m_alpha) goto unspawn;
    if (pVehicle->isOnScreen()) {
        LastVclVisTimes[index] = frameTime;
        return removed;
    }
    if (frameTime > LastVclVisTimes[index] + 4.0f) {
    unspawn:
        removed = 1;
        pVehicle->remove();
        pVehicle->setIndex(index);
    }
    return removed;
}

signed char IsVisPathLink(TrafficPathLink *pLink)
{
    for (int i = 0; i < nVisPathLinks; i++) {
        if (VisPathLinks[i] == pLink) return 1;
    }
    return 0;
}

// NEAR: 11 bytes differ, inlined setArrayIndex schedules and/shl in the other order.
unsigned short TrafficSystem::addPathLink(TrafficPathLink *pLink, unsigned char dir)
{
    unsigned short i = m_numPathLinks++;
    m_pathLinkPtrs[i] = pLink;
    m_numLanePathLinks[dir]++;
    pLink->setArrayIndex(i);
    return i;
}

TrafficBoxJunction *TrafficSystem::newBoxJunction()
{
    int i = m_numBoxJunctions++;
    TrafficBoxJunction *p = getBoxJunction(i);
    p->reset();
    return p;
}

TrafficLane::TrafficLane()
{
    reset();
}

// NEAR: 21 bytes differ, register choice and inlined setArrayIndex and/shl order.
void TrafficLane::addPathLink(TrafficPathLink *pLink)
{
    int n = m_numPathLinks++;
    unsigned short i = _theTrafficSysAlloc.getSys()->addPathLink(pLink, m_dir);
    if (!n) m_pathLinkIndex = i;
}

signed char TrafficLane::hasValidDest()
{
    for (u32 i = 0; i < 3; i++) {
        TrafficLaneDest *pDest = getEndDest(i);
        if (pDest) {
            if (!pDest->isDeadEnd()) return 1;
            TrafficIntersection *pInt = pDest->getIntersection();
            if (pInt && pInt->getPathLink()->isUnspawnLink()) return 1;
        }
    }
    return 0;
}

TrafficLane *TrafficRoad::addLane(TrafficPathLink *pLink, unsigned char dir)
{
    int i = m_numLanes[dir]++;
    unsigned short id = _theTrafficSysAlloc.getSys()->m_numLanes++;
    TrafficLane *pLane = TrGetLane(id);
    m_lanes[dir][i] = id;
    pLane->setRoad(this);
    pLane->setIndex(i);
    pLane->setDir(dir);
    pLane->addPathLink(pLink);
    return pLane;
}

TrafficPopulace::~TrafficPopulace()
{
    m_vehicles.SetSize(0);
}

TrafficLight::TrafficLight(unsigned int type)
{
    m_unk0 = 0;
    m_timer = -1.0f;
    m_numIntersections = 0;
    m_intersections[0] = 0;
    m_sequence = 0;
    m_duration = 0;
    for (u32 j = 0; j < 4; j++) m_items[j] = 0;
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(0x1000027, 0, 0);
    if (!pFile) return;
    for (u32 i = 0; i < 4; i++) {
        EXVector pos(0.0f, 0.0f, 0.0f, 0.0f);
        m_items[i] = XItem::CreateItem(pFile, 0x1000027, 0, pos);
        if (m_items[i]) {
            EXItemAnimator_Entity *pAnim = new EXItemAnimator_Entity;
            if (pAnim) {
                unsigned int anims[4] = { 0x2000041, 0x2000044, 0x2000043, 0x2000042 };
                unsigned int anim = anims[i];
                if (i == 0 && type) {
                    if (type - 1 == 0) anim = 0x2000056;
                }
                pAnim->Initialise(pFile, anim);
                m_items[i]->AddAnimator(pAnim);
                pWorld->AddItem(m_items[i], i ? 1 : 3);
            }
        }
    }
    m_state = 3;
    update();
}

void TrafficSpawnerDeInit()
{
    nVisPathLinks = 0;
    VisPathLinks.SetSize(0);
}

void TrafficHandleMemCompaction()
{
    if (_theTrafficSysAlloc.getSys()) _theTrafficSysAlloc.getSys()->handleMemCompaction();
}

int TrafficLane::calcLinkFromDist(float dist, float &rBefore, float &rAfter, float back, float front)
{
    rBefore = 0.0f;
    rAfter = 0.0f;
    for (int i = 0; i < getNumPathLinks(); i++) {
        TrafficExtraLinkData *pData = getPathLink(i)->getExtraLinkData();
        if (dist >= pData->getDistFromLaneStartXZ() && pData->getLengthXZ() + pData->getDistFromLaneStartXZ() > dist) {
            float t = pData->getDistFromLaneStartXZ() - (dist - back);
            if (t > 0.0f) rBefore = t;
            float u = (dist + front) - (pData->getLengthXZ() + pData->getDistFromLaneStartXZ());
            if (u > 0.0f) rAfter = u;
            return i;
        }
    }
    return 0x9999;
}

// NEAR: retail inlines getDistFromLinkStartXZ inside the inlined getDistFromLaneStartXZ; VC7.1 calls it here.
Vehicle *TrafficLaneDest::checkVehicleProximity(float back, float front)
{
    VehicleListItem *pItem = (VehicleListItem *)getLane()->getVehicleList().m_pHead;
    if (!pItem) return 0;
    float negBack = 0.0f - back;
    for (; pItem; pItem = pItem->Next()) {
        Vehicle *pVehicle = pItem->m_pVehicle;
        TrafficPathLink *pLink = pVehicle->getCurrPos().getLoc().getPathLink();
        float dist = pLink->getDistFromLaneStartXZ((const EXVector3 *)&pVehicle->m_pItem->Position(), 0x9999, 0);
        float rel = dist - getDistFromLaneStartXZ();
        float half = pVehicle->getTypeObject()->getZDim() * 0.5f;
        float lo = rel - half;
        float hi = half + rel;
        if (rel >= negBack && front > rel) return pVehicle;
        if (negBack >= lo && hi > front) return pVehicle;
    }
    return 0;
}

// NEAR: as checkVehicleProximity.
Vehicle *TrafficLaneDest::checkCrossedLaneVehicleProximity(unsigned char i, float back, float front)
{
    TrafficLane *pLane = getCrossedPathLink(i)->getLane();
    float crossDist = getCrossedDistFromLaneStartXZ(i);
    VehicleListItem *pItem = (VehicleListItem *)pLane->getVehicleList().m_pHead;
    if (!pItem) return 0;
    float negBack = 0.0f - back;
    for (; pItem; pItem = pItem->Next()) {
        Vehicle *pVehicle = pItem->m_pVehicle;
        TrafficPathLink *pLink = pVehicle->getCurrPos().getLoc().getPathLink();
        float dist = pLink->getDistFromLaneStartXZ((const EXVector3 *)&pVehicle->m_pItem->Position(), 0x9999, 0);
        float rel = dist - crossDist;
        float half = pVehicle->getTypeObject()->getZDim() * 0.5f;
        float lo = rel - half;
        float hi = half + rel;
        if (rel >= negBack && front > rel) return pVehicle;
        if (negBack >= lo && hi > front) return pVehicle;
    }
    return 0;
}

void TrafficLaneDest::blockAICrossedLanes(Vehicle *pVehicle)
{
    unsigned char ref = pVehicle->getIndex();
    for (unsigned int i = 0; i < 4; i++) {
        TrafficPathLink *pLink = getCrossedPathLink(i);
        if (!pLink) return;
        float dist = pLink->getExtraLinkData()->getDistFromLaneStartXZ() + getCrossedDistFromLinkStartXZ(i);
        float start = dist - 3.0f;
        float end = dist + 3.0f;
        if (0.0f > start) start = 0.0f;
        pVehicle->getMove().addLaneSegBlock(pLink->getLane()->addAIBlockedSeg(start, end, ref));
    }
}

unsigned int TrafficPath::removeRepeatLinks()
{
    unsigned int numRemoved = 0;
    for (unsigned int i = 0; i < NumLinks(); i++) {
        TrafficPathLink *pA = Link(i);
        for (unsigned int j = i + 1; j < NumLinks(); j++) {
            TrafficPathLink *pB = Link(j);
            if (pB->isRemoved()) continue;
            if (pA->getNodeA() == pB->getNodeA() && pA->getNodeB() == pB->getNodeB()) {
                pB->remove();
                numRemoved++;
            }
        }
    }
    return numRemoved;
}

void TrafficPath::swapLinks(unsigned int a, unsigned int b)
{
    if (a == b) return;
    TrafficPathLink t = *Link(a);
    *Link(a) = *Link(b);
    *Link(b) = t;
}

// NEAR: 2 bytes, lea base/index operand order in two link address computations.
void TrafficPath::insertLink(unsigned int from, unsigned int to)
{
    if (from == to) return;
    unsigned int i;
    for (i = from; i < to + 3 && i < NumLinks(); i++) {
    }
    TrafficPathLink t = *Link(to);
    for (i = from; i < to; i++) {
        TrafficPathLink t2 = *Link(i);
        *Link(i) = t;
        t = t2;
    }
    *Link(i) = t;
    for (i = from; i < to + 2 && i < NumLinks(); i++) {
    }
}

TrafficLocation::TrafficLocation(unsigned int road, unsigned int dir, unsigned int lane, unsigned int link, float dist, float offset)
{
    reset();
    setRoad(sys()->getRoad(road));
    setLane(getRoad()->getLane(dir, lane));
    setStartLinkDist(dist);
    setLink(link);
    setHorizontalOffset(offset);
    m_flag = 0;
}

// NEAR: register allocation (retail avoids ebx).
void TrafficLocation::calcPos(EXVector3 &v)
{
    TrafficPathLink *pLink = getPathLink();
    v = pLink->getNodeAPtr()->getPos();
    TrafficExtraLinkData *pData = pLink->getExtraLinkData();
    v.x += pData->getSlopeX() * m_startLinkDist;
    v.z += pData->getSlopeZ() * m_startLinkDist;
    if (m_horizontalOffset != 0.0f) {
        EXVector2 off;
        pLink->getLinkHorizontalOffsetVec(m_horizontalOffset, off);
        v.x = v.x + off.x;
        v.z = v.z + off.y;
    }
}

void TrafficLocation::calcPos(EXVector &v)
{
    EXVector3 p;
    calcPos(p);
    v.Set(p, 0.0f);
}

static unsigned int s_firstFreeVehicle;  // TODO name: file static, not in the map

Vehicle *TrafficPopulace::addVehicle(signed char bAppend)
{
    unsigned int i;
    if (!bAppend) {
        for (i = s_firstFreeVehicle; i < getNumVehicles(); i++) {
            Vehicle *pVehicle = getVehicle(i);
            if (!pVehicle->isInit()) {
                pVehicle->setIndex(i);
                return pVehicle;
            }
        }
    }
    if (m_nVehicles == 25) return 0;
    if (m_nVehicles >= m_vehicles.Size()) return 0;
    i = m_nVehicles++;
    Vehicle *pVehicle = getVehicle(i);
    pVehicle->setIndex(i);
    return pVehicle;
}

// NEAR: register allocation differs (82 bytes).
Vehicle *TrafficPopulace::addVehicle(unsigned char type, TrafficLocation *pLoc)
{
    Vehicle *pVehicle;
    unsigned int i;
    for (i = s_firstFreeVehicle; i < getNumVehicles(); i++) {
        pVehicle = getVehicle(i);
        if (!pVehicle->isInit()) goto found;
    }
    if (m_nVehicles == 25) return 0;
    if (m_nVehicles >= m_vehicles.Size()) return 0;
    i = m_nVehicles++;
    pVehicle = getVehicle(i);
found:
    pVehicle->setIndex(i);
    pVehicle->create(type, pLoc);
    return pVehicle;
}

unsigned char vehicleTypeFromHash(unsigned int);

Vehicle *TrafficPopulace::addVehicleFromHash(unsigned int hash, TrafficLocation *pLoc)
{
    unsigned char type = vehicleTypeFromHash(hash);
    return addVehicle(type, pLoc);
}

void TrafficPathLink::getPointOnLink(EXVector &v, float dist, float offset)
{
    v = EXVector(getNodeAPtr()->getPos(), 0.0f);
    TrafficExtraLinkData *pData = getExtraLinkData();
    v.x += pData->getSlopeX() * dist;
    v.z += pData->getSlopeZ() * dist;
    if (offset != 0.0f) {
        EXVector2 off;
        getLinkHorizontalOffsetVec(offset, off);
        v.x += off.x;
        v.z += off.y;
    }
}

signed char isPointInQuad2D(const EXVector2 &, const EXFixedArray<EXVector2, 4> &);

signed char isPointInQuad2D(const EXVector &v, const EXFixedArray<EXVector2, 4> &q)
{
    EXVector2 p;
    p.x = v.x;
    p.y = v.z;
    return isPointInQuad2D(p, q);
}

signed char isPointInQuad2D(const EXVector3 &v, const EXFixedArray<EXVector2, 4> &q)
{
    EXVector2 p;
    p.x = v.x;
    p.y = v.z;
    return isPointInQuad2D(p, q);
}

int iLastVisLinkIndexChecked;

// NEAR: inlined addVehicle uses ecx/eax swapped.
void TrafficSpawnerInit()
{
    if (s_firstFreeVehicle) {
        VisPathLinks.RemoveAll();
        VisPathLinks.SetSize(64);
    }
    for (unsigned int i = 0; i < s_firstFreeVehicle; i++) {
        pop().addVehicle((signed char)1);
        LastVclVisTimes[i] = -1.0f;
    }
    for (unsigned int j = 0; j < 25; j++) LastVclVisTimes[j] = -1.0f;
    iLastVisLinkIndexChecked = 0;
}

int CountVehiclesOnLink(TrafficPathLink *pLink)
{
    int count = 0;
    int n = pop().getNumVehicles();
    for (int i = 0; i < n; i++) {
        Vehicle *pVehicle = pop().getVehicle(i);
        if (pVehicle->isInit() && pVehicle->getCurrPos().getLoc().getPathLink() == pLink) count++;
    }
    return count;
}

// NEAR: 8 bytes, the intersections-path load is scheduled after the loop-count compare.
void TrafficSystem::handleMemCompaction()
{
    m_pIntersectionsPath = (TrafficPath *)getWorld()->GetPath(0xb000006);
    m_lanesPaths[0] = (TrafficPath *)getWorld()->GetPath(0xb000004);
    m_lanesPaths[1] = (TrafficPath *)getWorld()->GetPath(0xb000005);
    TrafficPath *pIPath = getIntersectionsPath();
    int n = 0;
    for (; n < m_numLanePathLinks[2]; n++) {
        if (m_pathLinkGeoIndices[n] != 0xffff) m_pathLinkPtrs[n] = pIPath->Link(m_pathLinkGeoIndices[n]);
    }
    for (unsigned int dir = 0; dir < 2; dir++) {
        TrafficPath *pPath = m_lanesPaths[dir];
        for (int j = 0; j < m_numLanePathLinks[dir]; j++, n++) {
            if (m_pathLinkGeoIndices[n] != 0xffff) m_pathLinkPtrs[n] = pPath->Link(m_pathLinkGeoIndices[n]);
        }
    }
}

float dist3D_LineSegment_to_LineSegment(const EXVector &, const EXVector &, const EXVector &, const EXVector &);

inline float EXCosineRule(float a, float b, float c)
{
    float d = a * b * 2.0f;
    float cs = 0.0f;
    if (d != 0.0f) {
        cs = (a * a + b * b - c * c) / d;
        if (cs > 1.0f) cs = 1.0f;
        if (cs < -1.0f) cs = -1.0f;
    }
    return EXACos(cs);
}

__forceinline float EXCosineRule(const EXVector &a, const EXVector &b, const EXVector &c)
{
    EXVector ab = b - a;
    EXVector bc = c - b;
    EXVector ca = a - c;
    float lab = ab.Length();
    float lca = ca.Length();
    float lbc = bc.Length();
    return EXCosineRule(lbc, lca, lab);
}

// NEAR: structure only.
void TrafficSystem::fixLaneOrder()
{
restart:
    unsigned int numRoads = getNumRoads();
    for (unsigned int r = 0; r < numRoads; r++) {
        TrafficRoad *pRoad = getRoad(r);
        for (unsigned int dir = 0; dir < 2; dir++) {
            TrafficPath *pPath = getLanesPath(dir);
            unsigned int numLanes = pRoad->getNumLanes(dir);
            for (unsigned int i = 0; i < numLanes; i++) {
                TrafficLane *pLaneA = pRoad->getLane(dir, i);
                for (unsigned int j = i + 1; j < numLanes; j++) {
                    TrafficLane *pLaneB = pRoad->getLane(dir, j);
                    unsigned int numA = pLaneA->getNumPathLinks();
                    for (unsigned int a = 0; a < numA; a++) {
                        TrafficPathLink *pLinkA = pLaneA->getPathLink(a);
                        if (pLinkA->isRemoved()) continue;
                        TrafficPathNode *pA0 = pPath->getNodePtr(pLinkA->getNodeA());
                        TrafficPathNode *pA1 = pPath->getNodePtr(pLinkA->getNodeB());
                        float dxA = pA1->getPos().x - pA0->getPos().x;
                        float dzA = pA1->getPos().z - pA0->getPos().z;
                        EXVector dirA(dxA, 0.0f, dzA, 0.0f);
                        unsigned int numB = pLaneB->getNumPathLinks();
                        for (unsigned int b = 0; b < numB; b++) {
                            TrafficPathLink *pLinkB = pLaneB->getPathLink(b);
                            if (pLinkB->isRemoved()) continue;
                            TrafficPathNode *pB0 = pPath->getNodePtr(pLinkB->getNodeA());
                            TrafficPathNode *pB1 = pPath->getNodePtr(pLinkB->getNodeB());
                            EXVector3 &a0 = pA0->getPos();
                            EXVector3 &a1 = pA1->getPos();
                            EXVector3 &b0 = pB0->getPos();
                            EXVector3 &b1 = pB1->getPos();
                            float dist = dist3D_LineSegment_to_LineSegment(EXVector(a0.x, a0.y, a0.z, 0.0f),
                                EXVector(a1.x, a1.y, a1.z, 0.0f), EXVector(b0.x, b0.y, b0.z, 0.0f),
                                EXVector(b1.x, b1.y, b1.z, 0.0f));
                            if (dist > 7.5f) continue;
                            EXVector dirB(b1.x - b0.x, 0.0f, b1.z - b0.z, 0.0f);
                            if (EXCosineRule(dirA, dirB, EXVector(0.0f, 0.0f, 0.0f, 0.0f)) >= 0.7853982f) continue;
                            float ang = (float)atan2(dxA, dzA);
                            EXVector off(-(float)cos(ang), 0.0f, (float)sin(ang), 0.0f);
                            EXVector na0 = EXVector(a0.x, a0.y, a0.z, 0.0f) + off;
                            EXVector na1 = EXVector(a1.x, a1.y, a1.z, 0.0f) + off;
                            if (dist3D_LineSegment_to_LineSegment(na0, na1, EXVector(b0.x, b0.y, b0.z, 0.0f),
                                    EXVector(b1.x, b1.y, b1.z, 0.0f)) > dist) {
                                pLaneA->swap(pLaneB);
                                goto restart;
                            }
                        }
                    }
                }
            }
        }
    }
}

void TrafficPath::reorderRoots(unsigned int i)
{
    unsigned int next = i;
    for (unsigned int k = i; k < NumLinks(); k++) {
        TrafficPathLink *pLink = Link(k);
        if (pLink->isRemovedNotTriggerLink()) continue;
        if (pLink->isRoot()) {
            if (Node(pLink->getNodeB())->isRoot()) pLink->swapNodes();
            swapLinks(k, next);
            next++;
        } else if (this == sys()->getIntersectionsPath()) {
            if (!pLink->isIntersectionTriggerToIntersectionLink()) pLink->setAsRoot();
        } else if (Node(pLink->getNodeA())->isRoot()) {
            pLink->swapNodes();
        }
    }
}

// NEAR: the position copy uses ebp in ours (register allocation).
void TrafficLocation::calcPosAndRot(EXVector3 &pos, EXVector3 &rot)
{
    TrafficPathLink *pLink = getPathLink();
    TrafficPathNode *pA = pLink->getNodeAPtr();
    TrafficPathNode *pB = pLink->getNodeBPtr();
    pos = pA->getPos();
    calcPos(pos);
    EXVector ref(0.0f, 0.0f, 1000.0f, 0.0f);
    EXVector d;
    d.x = pB->getPos().x - pA->getPos().x;
    d.z = pB->getPos().z - pA->getPos().z;
    float a = angle2d_between_vectors(ref, d);
    if (0.0f > a) a += 6.2831855f;
    rot.x = 0.0f;
    rot.y = a - 1.5707964f;
    rot.z = 0.0f;
}

TrafficPopulace::TrafficPopulace()
{
    reset();
}

TrafficSystem::TrafficSystem()
{
    m_bOpen = 0;
}

// NEAR: retail inlines EXArray<Vehicle>::RemoveAll (DestructElements called); VC inlines
// DestructElements into RemoveAll instead and then calls RemoveAll (same in removeVehicles).
void TrafficSystem::close()
{
    pop().removeVehicles();
    m_roads.RemoveAll();
    m_intersections.RemoveAll();
    m_additionalPathNodes.RemoveAll();
    m_additionalPathLinks[0].RemoveAll();
    m_additionalPathLinks[1].RemoveAll();
    m_extraLinkData.RemoveAll();
    TrafficSpawnerDeInit();
    m_bOpen = 0;
}

// NEAR: 25 bytes differ, retail memset stores at [esi+0x14]/[esi+0x18] directly (ours via lea ecx).
void TrafficSystem::reset()
{
    m_numRoads = 0;
    m_roads.SetSize(60);
    m_numIntersections = 0;
    m_intersections.SetSize(120);
    m_numAdditionalPathNodes = 0;
    m_additionalPathNodes.SetSize(120);
    m_numAdditionalPathLinks[0] = 0;
    m_additionalPathLinks[0].SetSize(30);
    m_numAdditionalPathLinks[1] = 0;
    m_additionalPathLinks[1].SetSize(30);
    m_populace.reset();
    m_numBoxJunctions = 0;
    m_boxJunctions.SetSize(40);
    for (u32 i = 0; i < 16; i++) m_trafficLights[i] = 0;
    m_numTrafficLights = 0;
    m_unk1 = 0;
    m_updateCount = 0;
    m_locs.RemoveAll();
    m_numPathLinks = 0;
    m_pathLinkPtrs.SetSize(420);
    memset(&m_numLanePathLinks, 0, sizeof(m_numLanePathLinks));
    m_numLanes = 0;
    m_lanes.SetSize(120);
}


namespace ex { double fd(float f); }

inline void offset2d_atan2(float x, float z, float r, EXVector &v)
{
    float ang = (float)atan2(x, z);
    v.x = (float)cos(ang) * r;
    v.z = (float)sin(ang) * r;
}

// GUESS: node/link splitting in the middle section is inferred, not read from the disassembly (agent 2026-10-18). Rest follows retail.
void TrafficSystem::buildFromPaths()
{
    float *pDists = (float *)_EXAlloc(getNumIntersections() * sizeof(float), 0);
    for (unsigned int dir = 0; dir < 2; dir++) {
        TrafficPath *pPath = getLanesPath(dir);
        signed char bLoop = 0;
        signed char bPendingDest = 0;
        TrafficRoad *pRoad = 0;
        TrafficRoad *pNextRoad = 0;
        TrafficLane *pLane = 0;
        TrafficLane *pNextLane = 0;
        int lastInt = -1;
        TrafficLaneDest *pDest = 0;
        TrafficIntersection *pPrevInt = 0;
        unsigned int numSplits = 0;
        if (!pPath) continue;
        for (int i = 0; i < pPath->NumLinks(); i++) {
            TrafficPathLink *pLink = pPath->Link(i);
            if (pLink->isRemoved()) continue;
            if (!bLoop) {
                pRoad = pNextRoad;
                pNextRoad = 0;
                pLane = pNextLane;
                pNextLane = 0;
            }
            TrafficPathNode *pA = pPath->Node(pLink->getNodeA());
            TrafficPathNode *pB = pPath->Node(pLink->getNodeB());
            if (pLink->isRoot() && !bLoop) {
                pRoad = 0;
                pLane = 0;
            }
            trTracef("link %f %f %f - %f %f %f\n", ex::fd(pA->getPos().x), ex::fd(pA->getPos().y),
                ex::fd(pA->getPos().z), ex::fd(pB->getPos().x), ex::fd(pB->getPos().y), ex::fd(pB->getPos().z));
            if (pRoad && !pLane) pLane = pRoad->addLane(pLink, dir);
            if (pA->isLoop()) bLoop = 1;
            int j;
            for (j = 0; j < getNumIntersections(); j++) pDists[j] = 3.4028235e38f;
            for (j = 0; j < getNumIntersections(); j++) {
                TrafficIntersection *pInt = getIntersection(j);
                TrafficPathLink *pIL = pInt->getPathLink();
                TrafficPathNode *pI0 = getIntersectionsPath()->Node(pIL->getNodeA());
                TrafficPathNode *pI1 = getIntersectionsPath()->Node(pIL->getNodeB());
                EXVector3 &i0 = pI0->getPos();
                EXVector3 &i1 = pI1->getPos();
                if (dist3D_LineSegment_to_LineSegment(EXVector(pA->getPos().x, pA->getPos().y, pA->getPos().z, 0.0f),
                        EXVector(pB->getPos().x, pB->getPos().y, pB->getPos().z, 0.0f),
                        EXVector(i0.x, i0.y, i0.z, 0.0f), EXVector(i1.x, i1.y, i1.z, 0.0f)) > 0.3f) {
                    EXLineSeg seg;
                    seg.m_origin = EXVector(i0.x, i0.y, i0.z, 0.0f);
                    seg.m_dir = EXVector(i1.x - i0.x, i1.y - i0.y, i1.z - i0.z, 0.0f);
                    EXVector pt(pA->getPos().x, pA->getPos().y, pA->getPos().z, 0.0f);
                    pDists[j] = dist3D_Point_to_LineSegment(pt, seg);
                }
            }
            signed char bFound = 0;
            for (;;) {
                float best = 3.4028235e38f;
                int k = -1;
                for (j = 0; j < getNumIntersections(); j++) {
                    if (pDists[j] != 3.4028235e38f && best > pDists[j]) {
                        best = pDists[j];
                        k = j;
                    }
                }
                if (k == -1) break;
                pDists[k] = 3.4028235e38f;
                TrafficIntersection *pInt = getIntersection(k);
                TrafficPathLink *pIL = pInt->getPathLink();
                TrafficPathNode *pI0 = getIntersectionsPath()->Node(pIL->getNodeA());
                TrafficPathNode *pI1 = getIntersectionsPath()->Node(pIL->getNodeB());
                TrafficPathNode *pSplit = 0;
                signed char bNew = 0;
                if (pIL->isIntersectionLaneLink()) {
                    TrafficRoad *pR = pInt->getRoad();
                    if (pR == (TrafficRoad *)0x99999999) pR = addRoad();
                    pNextRoad = pR;
                    pNextLane = pR->addLane(pLink, dir);
                }
                EXLineSeg seg;
                seg.m_origin = EXVector(pI0->getPos().x, pI0->getPos().y, pI0->getPos().z, 0.0f);
                seg.m_dir = EXVector(pI1->getPos().x - pI0->getPos().x, pI1->getPos().y - pI0->getPos().y,
                    pI1->getPos().z - pI0->getPos().z, 0.0f);
                if (pIL->isIntersectionLaneASplit()) pSplit = dir ? pB : pA;
                else if (pIL->isIntersectionLaneBSplit()) pSplit = dir ? pA : pB;
                else if (pIL->isIntersectionLaneNewALink()) bNew = numSplits == 0;
                else if (pIL->isIntersectionLaneNewBLink()) bNew = numSplits == 1;
                if (pSplit) {
                    EXVector pt(pSplit->getPos().x, pSplit->getPos().y, pSplit->getPos().z, 0.0f);
                    if (dist3D_Point_to_LineSegment(pt, seg) > best) {
                        signed char bDead = !pSplit->isDeadEnd();
                        signed char bReallyDead = !pSplit->isReallyDeadEnd();
                        EXVector off;
                        offset2d_atan2(pB->getPos().x - pA->getPos().x, pB->getPos().z - pA->getPos().z, best, off);
                        TrafficPathNode *pNew;
                        unsigned short nodeB = pLink->getNodeB();
                        pLink->m_nodeB = addAdditionalPathNode(pSplit, &pNew);
                        if (bDead) pNew->setAsReallyDeadEnd();
                        if (bReallyDead) pNew->clrAsDeadEnd();
                        else pNew->setAsDeadEnd();
                        TrafficPathLink *pNewLink;
                        insertAdditionalPathLink(dir, numSplits + 1, pLink, &pNewLink);
                        TrafficPathNode *pNew2;
                        pNewLink->m_nodeA = addAdditionalPathNode(pSplit, &pNew2);
                        pNewLink->m_nodeB = nodeB;
                        pNewLink->setAsRoot();
                        pNew2->getPos().x -= off.z;
                        pNew2->getPos().z -= off.x;
                        pNew2->clrFlags();
                        pNew2->setAsRoot();
                        pDest = pLane->getEndDest(2);
                        lastInt = k;
                        bPendingDest = 1;
                        pPrevInt = pInt;
                        pSplit->setAsRoot();
                        pSplit->setAsDeadEnd();
                    }
                }
                if (bNew) {
                    pB->setAsIntersection();
                    pB->setIntersectionIndex(k);
                    pB->setAsDeadEnd();
                    if (pDest) {
                        pDest->setPos(&pB->getPos());
                        pDest->setDistFromLinkStartXZ(pDest->getPathLink()->getDistFromLinkStartXZ(&pB->getPos(), 1));
                        pDest = 0;
                    }
                }
                if (pInt->getRoad() == (TrafficRoad *)0x99999999) pInt->setRoad(pNextRoad);
                if (pInt->getPathLink()->getRoadIndex() == 0x9999) pInt->getPathLink()->setRoadIndex(pNextRoad->getIndex());
                if (pA->isRoot()) {
                    pInt->setAsRoadStart();
                    pInt->setStartDir(dir);
                }
                if (pB->isDeadEnd()) {
                    pInt->setAsRoadEnd();
                    pInt->setEndDir(dir);
                }
                if (!pIL->isIntersectionLaneLink()) pLane->addIntersection(pInt);
                bFound = 1;
            }
            if (bFound && bPendingDest) {
                bPendingDest = 0;
                if (!pPrevInt && pNextRoad) {
                    pNextRoad = addRoad();
                    pNextLane = pNextRoad->addLane(pLink, dir);
                }
            }
            if (pDest) {
                pDest->setRoad(pNextRoad);
                pDest->setIntersection(0);
                pDest->setLane(pNextLane);
                pDest->setPathLink(pLink);
                pDest->setPos(&pA->getPos());
                pDest->setDistFromLinkStartXZ(pLink->getDistFromLinkStartXZ(&pA->getPos(), 0));
                pDest = 0;
            }
            if (pLane) {
                pLane->addPathLink(pLink);
                if (pB->isLoop()) {
                    bLoop = 0;
                    pNextRoad = 0;
                    pNextLane = 0;
                } else {
                    pNextRoad = pRoad;
                    pNextLane = pLane;
                }
                pLink->setIndex(pLane->getNumPathLinks() - 1);
                pLink->setRoadIndex(pRoad->getIndex());
                pLink->setLaneIndex(pLane->getIndex());
                pLink->setDir(dir);
                pA->setRoadIndex(pRoad->getIndex());
                pA->setLaneIndex(pLane->getIndex());
                pA->setDir(dir);
                pB->setRoadIndex(pRoad->getIndex());
                pB->setLaneIndex(pLane->getIndex());
                pB->setDir(dir);
            }
            numSplits++;
        }
    }
    EXFree(pDists);
}

signed char doLineSegsIntersect(const EXVector &, const EXVector &, const EXVector &, const EXVector &, EXVector *, float);

// NEAR: structure only (10.8K; register allocation, inlining and block layout differ).
void TrafficSystem::postBuildSetup()
{
    fixLaneOrder();
    unsigned int numRoads = getNumRoads();
    unsigned int numLinks = 0;
    if (getNorthLanesPath()) numLinks = getNorthLanesPath()->NumLinks();
    if (getSouthLanesPath()) numLinks += getSouthLanesPath()->NumLinks();
    m_extraLinkData.SetSize(numLinks);

    // Per-link geometry.
    unsigned int extraIdx = 0;
    unsigned int r;
    for (r = 0; r < numRoads; r++) {
        TrafficRoad *pRoad = getRoad(r);
        for (unsigned int dir = 0; dir < 2; dir++) {
            TrafficPath *pPath = getLanesPath(dir);
            unsigned int numLanes = pRoad->getNumLanes(dir);
            for (unsigned int l = 0; l < numLanes; l++) {
                TrafficLane *pLane = pRoad->getLane(dir, l);
                float dist = 0.0f;
                float length = 0.0f;
                unsigned int n = pLane->getNumPathLinks();
                for (unsigned int k = 0; k < n; k++) {
                    TrafficPathLink *pLink = pLane->getPathLink(k);
                    if (pLink->isRemoved()) continue;
                    TrafficExtraLinkData *pData = &m_extraLinkData[extraIdx++];
                    pLink->setExtraLinkData(pData);
                    TrafficPathNode *pA = pPath->Node(pLink->getNodeA());
                    TrafficPathNode *pB = pPath->Node(pLink->getNodeB());
                    float dx = pB->getPos().x - pA->getPos().x;
                    float dz = pB->getPos().z - pA->getPos().z;
                    float angle = (float)atan2(dx, dz);
                    float lenSq = dx * dx + dz * dz;
                    pData->setSlopeX((float)sin(angle));
                    pData->setSlopeZ((float)cos(angle));
                    float len = EXSqrt(lenSq);
                    pData->setLengthXZ(len);
                    length += len;
                    pData->setAngle(angle);
                    float perp = angle - 1.5707964f;
                    pData->getLeftPerpUnitVec().x = (float)sin(perp);
                    pData->getLeftPerpUnitVec().y = (float)cos(perp);
                    if (k == n - 1) {
                        pLink->setAsLastLink();
                        if (!pLane->getPathLink(0)->getNodeAPtr()->isLoop()) {
                            pData->setAngleToNextLink(0.0f);
                            goto setDist;
                        }
                    }
                    {
                        EXVector v1, v2;
                        TrafficPathNode *pA1 = pPath->Node(pLink->getNodeA());
                        TrafficPathNode *pB1 = pPath->Node(pLink->getNodeB());
                        v1.x = pB1->getPos().x - pA1->getPos().x;
                        v1.z = pB1->getPos().z - pA1->getPos().z;
                        TrafficPathLink *pNext = pLink->getNextLink();
                        TrafficPathNode *pB2 = pPath->Node(pNext->getNodeB());
                        TrafficPathNode *pA2 = pPath->Node(pNext->getNodeA());
                        v2.x = pB2->getPos().x - pA2->getPos().x;
                        v2.z = pB2->getPos().z - pA2->getPos().z;
                        float a = angle2d_between_vectors(v1, v2);
                        if (0.0f > a) {
                            if (-3.1415927f >= a) a = -6.2831855f - a;
                        } else if (a >= 3.1415927f) {
                            a = 6.2831855f - a;
                        }
                        pData->setAngleToNextLink(a);
                    }
                setDist:
                    pData->setDistFromLaneStartXZ(dist);
                    TrafficPathNode *pRoot = pPath->getNodePtr(pLink->getNodeA());
                    if (!k) pRoot->setAsRoot();
                    if (pRoot->isRoot() && !pRoot->isLoop()) pRoot->setAsDeadEnd();
                    dist += pData->getLengthXZ();
                }
                pLane->setLength(length);
            }
        }
    }

    // Orient the intersection links along the roads.
    unsigned int numInter = getNumIntersections();
    unsigned int i;
    for (i = 0; i < numInter; i++) {
        TrafficIntersection *pInter = getIntersection(i);
        TrafficRoad *pRoad = pInter->getRoad();
        TrafficPathLink *pLink = pInter->getPathLink();
        EXVector v1, v2;
        if (pInter->isRoadEnd()) {
            TrafficPathLink *pLast = pRoad->getLane(pInter->getEndDir(), 0)->getLastPathLink();
            TrafficPath *pPath = getLanesPath(pInter->getEndDir());
            TrafficPathNode *pB = pPath->Node(pLast->getNodeB());
            TrafficPathNode *pA = pPath->Node(pLast->getNodeA());
            v1.x = pB->getPos().x - pA->getPos().x;
            v1.z = pB->getPos().z - pA->getPos().z;
            TrafficPath *pIPath = getIntersectionsPath();
            pB = pIPath->Node(pLink->getNodeB());
            pA = pIPath->Node(pLink->getNodeA());
            v2.x = pB->getPos().x - pA->getPos().x;
            v2.z = pB->getPos().z - pA->getPos().z;
        } else if (pInter->isRoadStart()) {
            TrafficPathLink *pFirst = pRoad->getLane(pInter->getStartDir(), 0)->getPathLink(0);
            TrafficPath *pPath = getLanesPath(pInter->getStartDir());
            TrafficPath *pIPath = getIntersectionsPath();
            TrafficPathNode *pB = pIPath->Node(pLink->getNodeB());
            TrafficPathNode *pA = pIPath->Node(pLink->getNodeA());
            v1.x = pB->getPos().x - pA->getPos().x;
            v1.z = pB->getPos().z - pA->getPos().z;
            pB = pPath->Node(pFirst->getNodeB());
            pA = pPath->Node(pFirst->getNodeA());
            v2.x = pB->getPos().x - pA->getPos().x;
            v2.z = pB->getPos().z - pA->getPos().z;
        } else {
            continue;
        }
        if (0.0f > angle2d_between_vectors(v1, v2)) {
            pLink->swapNodes();
            TrafficIntersection *t = pInter->getIntersectionA();
            pInter->setIntersectionA(pInter->getIntersectionB());
            pInter->setIntersectionB(t);
        }
    }

    // Road ends: link up the joining road starts, or find a road the dead end runs into.
    for (i = 0; i < numInter; i++) {
        TrafficIntersection *pInter = getIntersection(i);
        if (!pInter->isRoadEnd()) continue;
        TrafficIntersection *pB = pInter->getIntersectionB();
        TrafficIntersection *pA = pInter->getIntersectionA();
        if (pB && pB->isRoadStart()) pInter->setRelativeIntersection(0, pB);
        if (pA) {
            if (pA->isRoadStart()) pInter->setRelativeIntersection(1, pA);
            continue;
        }
        if (pB) continue;
        TrafficRoad *pRoad = pInter->getRoad();
        TrafficPathLink *pLast = pRoad->getLane(pInter->getEndDir(), 0)->getLastPathLink();
        TrafficPath *pPath = pLast->getPath();
        TrafficPathNode *pNA = pPath->Node(pLast->getNodeA());
        TrafficPathNode *pNB = pPath->Node(pLast->getNodeB());
        float ang = (float)atan2(pNB->getPos().x - pNA->getPos().x, pNB->getPos().z - pNA->getPos().z);
        EXVector ahead((float)sin(ang) * 6.0f + pNB->getPos().x, pNB->getPos().y, (float)cos(ang) * 6.0f + pNB->getPos().z, 0.0f);
        unsigned int n = sys()->getNumRoads();
        for (unsigned int r2 = 0; r2 < n; r2++) {
            TrafficRoad *pRoad2 = sys()->getRoad(r2);
            if (pRoad2 == pRoad) continue;
            for (unsigned int dir = 0; dir < 2; dir++) {
                TrafficPath *pPath2 = getLanesPath(dir);
                unsigned int numLanes = pRoad2->getNumLanes(dir);
                for (unsigned int l = 0; l < numLanes; l++) {
                    TrafficLane *pLane = pRoad2->getLane(dir, l);
                    unsigned int numL = pLane->getNumPathLinks();
                    for (unsigned int k = 0; k < numL; k++) {
                        TrafficPathLink *pLink = pLane->getPathLink(k);
                        if (pLink->isRemoved()) continue;
                        EXVector b(pPath2->Node(pLink->getNodeB())->getPos(), 0.0f);
                        EXVector a(pPath2->Node(pLink->getNodeA())->getPos(), 0.0f);
                        EXVector e(pNB->getPos(), 0.0f);
                        if (0.3f > dist3D_LineSegment_to_LineSegment(e, ahead, a, b)) {
                            pInter->setEndIntersectRoad(pRoad2);
                            pInter->setEndIntersectLink(pLink);
                            pInter->setEndIntersectDir(dir);
                            goto nextInter;
                        }
                    }
                }
            }
        }
    nextInter:;
    }

    // Lane end destinations.
    EXVector hit;
    hit.x = 0.0f;
    hit.y = 0.0f;
    hit.z = 0.0f;
    for (r = 0; r < numRoads; r++) {
        TrafficRoad *pRoad = getRoad(r);
        for (unsigned int dir = 0; dir < 2; dir++) {
            TrafficPath *pPath = getLanesPath(dir);
            unsigned int turn[2];
            unsigned int cnt[2];
            turn[0] = 0x99;
            turn[1] = 0x99;
            unsigned int side = 0x99;
            unsigned int numLanes = pRoad->getNumLanes(dir);
            signed char found = 0;
            TrafficIntersection *pI;
            TrafficRoad *pEndRoad;
            for (unsigned int l = 0; l < numLanes; l++) {
                TrafficLane *pLane = pRoad->getLane(dir, l);
                if (!l && (!pLane->getEndDest(0)->getRoad() || !pLane->getEndDest(1)->getRoad()) &&
                    pLane->getNumIntersections() > 1) {
                    pI = pLane->getIntersection(pLane->getNumIntersections() - 1);
                    if (pI->isRoadEnd()) {
                        for (unsigned int j = 0; j < 2; j++) {
                            TrafficIntersection *pRel = pI->getRelativeIntersection(j);
                            if (pRel && pRel->isRoadStart()) {
                                found = 1;
                                break;
                            }
                        }
                        side = 0x99;
                    }
                    if (!found) {
                        pEndRoad = pI->getEndIntersectRoad();
                        if (pEndRoad) {
                            EXVector v1;
                            TrafficPathLink *pLast = pLane->getLastPathLink();
                            TrafficPathNode *pB = pPath->Node(pLast->getNodeB());
                            TrafficPathNode *pA = pPath->Node(pLast->getNodeA());
                            v1.x = pB->getPos().x - pA->getPos().x;
                            v1.z = pB->getPos().z - pA->getPos().z;
                            for (unsigned int d = pI->getEndIntersectDir(); d < 2; d++) {
                                if (side == 0x99) {
                                    EXVector v2;
                                    TrafficPathLink *pEnd = pI->getEndIntersectLink();
                                    TrafficPath *pPath2 = getLanesPath(d);
                                    TrafficPathNode *pB2 = pPath2->Node(pEnd->getNodeB());
                                    TrafficPathNode *pA2 = pPath2->Node(pEnd->getNodeA());
                                    v2.x = pB2->getPos().x - pA2->getPos().x;
                                    v2.z = pB2->getPos().z - pA2->getPos().z;
                                    if (0.0f > angle2d_between_vectors(v1, v2)) side = 1;
                                    else side = 0;
                                } else {
                                    if (!pEndRoad->getNumLanes(d)) break;
                                    side = side != 1;
                                }
                                turn[d] = side;
                                cnt[d] = pEndRoad->getNumLanes(d);
                            }
                        }
                    }
                }
                if (found) {
                    side = 2;
                    for (unsigned int j = 0; j < 2; j++) {
                        TrafficIntersection *pRel = pI->getRelativeIntersection(j);
                        if (!pRel || !pRel->isRoadStart()) continue;
                        TrafficRoad *pRelRoad = pRel->getRoad();
                        unsigned int li = l;
                        if (li >= pRelRoad->getNumLanes(pRel->getStartDir())) li = pRelRoad->getNumLanes(pRel->getStartDir()) - 1;
                        TrafficLane *pLane2 = pRelRoad->getLane(pRel->getStartDir(), li);
                        TrafficPathLink *pFirst = pLane2->getPathLink(0);
                        TrafficLaneDest *pDest = pLane->getEndDest(j);
                        pDest->setIntersection(pRel);
                        pDest->setRoad(pRelRoad);
                        pDest->setLane(pLane2);
                        pDest->setPathLink(pFirst);
                        pDest->setPos(&pFirst->getNodeAPtr()->getPos());
                        pDest->setDistFromLinkStartXZ(pDest->getPathLink()->getDistFromLinkStartXZ(&pDest->getPos(), 1));
                    }
                    continue;
                }
                if (side == 0x99) continue;
                {
                    TrafficPathLink *pLast = pLane->getLastPathLink();
                    TrafficPathNode *pA = pPath->Node(pLast->getNodeA());
                    TrafficPathNode *pB = pPath->Node(pLast->getNodeB());
                    float ang = (float)atan2(pB->getPos().x - pA->getPos().x, pB->getPos().z - pA->getPos().z);
                    EXVector3 saved = pA->getPos();
                    pA->getPos().y = pB->getPos().y;
                    pA->getPos().z = pB->getPos().z + (float)cos(ang) * 50.0f;
                    pA->getPos().x = pB->getPos().x + (float)sin(ang) * 50.0f;
                    for (unsigned int d = 0; d < 2; d++) {
                        side = turn[d];
                        if (side == 0x99) continue;
                        TrafficLaneDest *pDest = pLane->getEndDest(side);
                        unsigned int li = l;
                        if (li >= cnt[d]) li = cnt[d] - 1;
                        TrafficPath *pPath2 = getLanesPath(d);
                        TrafficLane *pLane2 = pEndRoad->getLane(d, li);
                        unsigned int numL = pLane2->getNumPathLinks();
                        for (unsigned int k = 0; k < numL; k++) {
                            TrafficPathLink *pLink = pLane2->getPathLink(k);
                            if (pLink->isRemoved()) continue;
                            if (!pLast->doLinksIntersect(pLink, &hit, 0, 0.3f)) continue;
                            TrafficPathNode *pB2 = pPath2->Node(pLink->getNodeB());
                            float a2 = (float)atan2(pB2->getPos().x - hit.x, pB2->getPos().z - hit.z);
                            pDest->setIntersection(0);
                            pDest->setRoad(pEndRoad);
                            pDest->setLane(pLane2);
                            pDest->setPathLink(pLink);
                            hit.z += (float)cos(a2) * 3.0f;
                            hit.x += (float)sin(a2) * 3.0f;
                            pDest->setPos((EXVector3 *)&hit);
                            pDest->setDistFromLinkStartXZ(pDest->getPathLink()->getDistFromLinkStartXZ(&pDest->getPos(), 1));
                            break;
                        }
                    }
                    pA->getPos() = saved;
                }
            }
        }
    }

    // Lanes crossed on the way to each destination.
    for (r = 0; r < numRoads; r++) {
        TrafficRoad *pRoad = getRoad(r);
        for (unsigned int dir = 0; dir < 2; dir++) {
            TrafficPath *pPath = getLanesPath(dir);
            unsigned int numLanes = pRoad->getNumLanes(dir);
            for (unsigned int l = 0; l < numLanes; l++) {
                TrafficLane *pLane = pRoad->getLane(dir, l);
                for (unsigned int d = 0; d < 3; d++) {
                    TrafficLaneDest *pDest = pLane->getEndDest(d);
                    if (!pDest->getIntersection()) continue;
                    TrafficPathLink *pLast = pLane->getLastPathLink();
                    EXVector p0(pPath->Node(pLast->getNodeB())->getPos(), 0.0f);
                    EXVector p1(pDest->getPos(), 0.0f);
                    unsigned int nCross = 0;
                    for (unsigned int r2 = 0; r2 < numRoads; r2++) {
                        if (r2 == r) continue;
                        TrafficRoad *pRoad2 = getRoad(r2);
                        for (unsigned int dir2 = 0; dir2 < 2; dir2++) {
                            TrafficPath *pPath2 = getLanesPath(dir2);
                            unsigned int numLanes2 = pRoad2->getNumLanes(dir2);
                            for (unsigned int l2 = 0; l2 < numLanes2; l2++) {
                                TrafficLane *pLane2 = pRoad2->getLane(dir2, l2);
                                unsigned int numL = pLane2->getNumPathLinks();
                                for (unsigned int k = 0; k < numL; k++) {
                                    TrafficPathLink *pLink = pLane2->getPathLink(k);
                                    if (!pLink->getIndex()) {
                                        TrafficPathNode *pA = pPath2->Node(pLink->getNodeA());
                                        if (pDest->getPos().x == pA->getPos().x && pDest->getPos().z == pA->getPos().z &&
                                            0.3f > (float)fabs(pDest->getPos().y - pA->getPos().y))
                                            continue;
                                    }
                                    EXVector b(pPath2->Node(pLink->getNodeB())->getPos(), 0.0f);
                                    EXVector a(pPath2->Node(pLink->getNodeA())->getPos(), 0.0f);
                                    EXVector x;
                                    if (doLineSegsIntersect(p0, p1, a, b, &x, 0.1f)) {
                                        EXVector3 h;
                                        h.x = x.x;
                                        h.y = x.y;
                                        h.z = x.z;
                                        pDest->setCrossedPathLink(nCross, pLink);
                                        float dd = pLink->getDistFromLinkStartXZ(&h, 1);
                                        pDest->setCrossedDistFromLinkStartXZ(nCross, dd);
                                        pDest->setCrossedDistFromLaneStartXZ(nCross, dd + pLink->getExtraLinkData()->getDistFromLaneStartXZ());
                                        nCross++;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Box junctions.
    for (i = 0; i < numInter; i++) {
        TrafficIntersection *pInter = getIntersection(i);
        if (!pInter->isRoadEnd()) continue;
        TrafficIntersection *pB = pInter->getIntersectionB();
        TrafficIntersection *pA = pInter->getIntersectionA();
        TrafficIntersection *pRel = pInter->getRelativeIntersection(2);
        if (!pRel) {
            TrafficIntersection *p;
            if (pA) {
                p = pA->getIntersectionA();
                if (p == pInter || !p) p = pA->getIntersectionB();
                if (p != pInter && p) goto haveRel;
            }
            if (!pB) goto noRel;
            p = pB->getIntersectionA();
            if (p == pInter || !p) p = pB->getIntersectionB();
            if (p == pInter || !p) goto noRel;
        haveRel:
            pInter->setRelativeIntersection(2, p);
            pRel = p;
            if (p->isRoadStart() && p->getStartDir() == pInter->getEndDir()) {
                TrafficRoad *pRoad2 = p->getRoad();
                TrafficRoad *pRoad = pInter->getRoad();
                for (unsigned int l = 0; l < pRoad->getNumLanes(pInter->getEndDir()); l++) {
                    TrafficLaneDest *pDest = pRoad->getLane(pInter->getEndDir(), l)->getEndDest(2);
                    TrafficLane *pLane2 = pRoad2->getLane(pInter->getEndDir(), l);
                    TrafficPathLink *pFirst = pLane2->getPathLink(0);
                    pDest->setIntersection(p);
                    pDest->setRoad(pRoad2);
                    pDest->setLane(pLane2);
                    pDest->setPathLink(pFirst);
                    pDest->getPos() = pFirst->getNodeAPtr()->getPos();
                    pDest->setDistFromLinkStartXZ(0.0f);
                }
            }
        }
    noRel:
        TrafficBoxJunction *pBJ = pInter->getBoxJunction();
        if (pRel || pA || pB) {
            if (!pBJ) pBJ = newBoxJunction();
            pBJ->setIntersection(3, pInter);
            pBJ->setIntersection(0, pB);
            pBJ->setIntersection(2, pRel);
            pBJ->setIntersection(1, pA);
        }
    }
    for (i = 0; i < numInter; i++) {
        TrafficIntersection *pInter = getIntersection(i);
        TrafficIntersection *pRel = pInter->getRelativeIntersection(2);
        if (pRel && !pRel->isRoadStart()) pInter->setRelativeIntersection(2, 0);
    }
    m_boxJunctions.SetSize(m_numBoxJunctions);

    // Destination distances, angles and dead-end flags.
    for (r = 0; r < numRoads; r++) {
        TrafficRoad *pRoad = getRoad(r);
        for (unsigned int dir = 0; dir < 2; dir++) {
            unsigned int numLanes = pRoad->getNumLanes(dir);
            for (unsigned int l = 0; l < numLanes; l++) {
                TrafficLane *pLane = pRoad->getLane(dir, l);
                for (unsigned int d = 0; d < 3; d++) {
                    TrafficLaneDest *pDest = pLane->getEndDest(d);
                    if (!pDest->getRoad()) {
                        pDest->setFlagBit(1);
                        continue;
                    }
                    float dist = pDest->getDistFromLinkStartXZ();
                    unsigned int idx = pDest->getPathLink()->getIndex();
                    if (idx) {
                        for (unsigned int j = 0; j < idx - 1; j++)
                            dist = pLane->getPathLink(j)->getExtraLinkData()->getDistFromLaneStartXZ() + dist;
                    }
                    pDest->setDistFromLaneStartXZ(dist);
                    if (d != 2) {
                        EXVector ref(0.0f, 0.0f, 10000.0f, 0.0f);
                        EXVector v1, v2;
                        TrafficPathNode *pB = pLane->getLastPathLink()->getNodeBPtr();
                        TrafficPathNode *pA = pLane->getLastPathLink()->getNodeAPtr();
                        v1.x = pB->getPos().x - pA->getPos().x;
                        v1.z = pB->getPos().z - pA->getPos().z;
                        float a1 = angle2d_between_vectors(ref, v1);
                        TrafficPathNode *pB2 = pDest->getPathLink()->getNodeBPtr();
                        v2.x = pB2->getPos().x - pDest->getPos().x;
                        v2.z = pB2->getPos().z - pDest->getPos().z;
                        float diff = angle2d_between_vectors(ref, v2) - a1;
                        if (d == 1) diff = fixAngleLeft(diff);
                        else diff = fixAngleRight(diff);
                        pDest->setAngle(diff);
                    }
                    if (!pDest->isLeadingToDeadEnd()) {
                        TrafficLane *pDestLane = pDest->getLane();
                        signed char dead = 1;
                        for (unsigned int k = 0; k < 3 && dead; k++) {
                            TrafficLaneDest *pD = pDestLane->getEndDest(k);
                            if (!pD->getRoad()) continue;
                            TrafficLane *pL = pD->getLane();
                            for (unsigned int m = 0; m < 3; m++) {
                                if (!pL->getEndDest(m)->isDeadEnd()) {
                                    dead = 0;
                                    break;
                                }
                            }
                        }
                        if (dead) pDest->setFlagBit(1);
                    }
                }
            }
        }
    }

    // Box junction light sequences and quads.
    for (unsigned int b = 0; b < m_numBoxJunctions; b++) {
        TrafficBoxJunction *pBJ = &m_boxJunctions[b];
        unsigned int seqLen = 0;
        unsigned short duration = 15;
        unsigned int it;
        TrafficIntersection *p;
        for (p = pBJ->firstIntersection(it); p; p = pBJ->nextIntersection(it)) {
            TrafficLight *pLight = p->getTrafficLight();
            if (!pLight) continue;
            unsigned int digits = 1;
            for (unsigned int s = pLight->getSequence() / 10; s; s /= 10) digits++;
            if (digits > seqLen) seqLen = digits;
            if (pLight->getDuration()) duration = pLight->getDuration();
        }
        pBJ->setTrafficLightSeqLen(seqLen);
        for (p = pBJ->firstIntersection(it); p; p = pBJ->nextIntersection(it)) {
            TrafficLight *pLight = p->getTrafficLight();
            if (pLight) pLight->setDuration(duration);
        }
        p = pBJ->firstIntersection(it);
        EXFixedArray<EXVector2, 4> &q = pBJ->getQuad();
        q[0].x = p->getPathLink()->getNodeAPtr()->getPos().x;
        q[0].y = p->getPathLink()->getNodeAPtr()->getPos().z;
        q[2].x = p->getPathLink()->getNodeBPtr()->getPos().x;
        q[2].y = p->getPathLink()->getNodeBPtr()->getPos().z;
        pBJ->m_quadType = 0;
        TrafficIntersection *pRel = p->getRelativeIntersection(2);
        if (pRel) {
            q[1].x = pRel->getPathLink()->getNodeBPtr()->getPos().x;
            q[1].y = pRel->getPathLink()->getNodeBPtr()->getPos().z;
            q[3].x = pRel->getPathLink()->getNodeAPtr()->getPos().x;
            q[3].y = pRel->getPathLink()->getNodeAPtr()->getPos().z;
            pBJ->m_quadType = 1;
        } else if ((pRel = p->getRelativeIntersection(1)) != 0) {
            TrafficPathLink *pLink = p->getPathLink();
            TrafficPathNode *pRA = pRel->getPathLink()->getNodeAPtr();
            TrafficPathNode *pA = pLink->getNodeAPtr();
            if ((float)fabs(pRA->getPos().x - pA->getPos().x) < 0.001f && (float)fabs(pRA->getPos().y - pA->getPos().y) < 0.001f &&
                (float)fabs(pRA->getPos().z - pA->getPos().z) < 0.001f) {
                q[1].x = pRel->getPathLink()->getNodeBPtr()->getPos().x;
                q[1].y = pRel->getPathLink()->getNodeBPtr()->getPos().z;
            } else {
                q[1].x = pRel->getPathLink()->getNodeAPtr()->getPos().x;
                q[1].y = pRel->getPathLink()->getNodeAPtr()->getPos().z;
            }
            float dx = q[1].x - q[0].x;
            float dz = q[1].y - q[0].y;
            float a = (float)atan2(dx, dz);
            float lenSq = dx * dx + dz * dz;
            float s = (float)sin(a);
            float c = (float)cos(a);
            float len = EXSqrt(lenSq);
            q[3].x = len * s + q[2].x;
            q[3].y = c * len + q[2].y;
            pBJ->m_quadType = 2;
        } else if ((pRel = p->getRelativeIntersection(0)) != 0) {
            TrafficPathLink *pLink = p->getPathLink();
            TrafficPathNode *pRA = pRel->getPathLink()->getNodeAPtr();
            TrafficPathNode *pB = pLink->getNodeBPtr();
            if ((float)fabs(pRA->getPos().x - pB->getPos().x) < 0.001f && (float)fabs(pRA->getPos().y - pB->getPos().y) < 0.001f &&
                (float)fabs(pRA->getPos().z - pB->getPos().z) < 0.001f) {
                q[3].x = pRel->getPathLink()->getNodeBPtr()->getPos().x;
                q[3].y = pRel->getPathLink()->getNodeBPtr()->getPos().z;
            } else {
                q[3].x = pRel->getPathLink()->getNodeAPtr()->getPos().x;
                q[3].y = pRel->getPathLink()->getNodeAPtr()->getPos().z;
            }
            float dx = q[3].x - q[2].x;
            float dz = q[3].y - q[2].y;
            float a = (float)atan2(dx, dz);
            float lenSq = dx * dx + dz * dz;
            float s = (float)sin(a);
            float c = (float)cos(a);
            float len = EXSqrt(lenSq);
            q[1].x = s * len + q[0].x;
            q[1].y = c * len + q[0].y;
            pBJ->m_quadType = 3;
        }
    }

    // Geo indices of every path link.
    TrafficPath *pIPath = getIntersectionsPath();
    int n = 0;
    for (; n < m_numLanePathLinks[2]; n++) {
        unsigned short g = 0xffff;
        if (m_pathLinkGeoIndices[n] != 0xfffe) g = pIPath->findPathLinkGeoIndex(m_pathLinkPtrs[n]);
        m_pathLinkGeoIndices[n] = g;
        m_pathLinkPtrs[n]->setGeoIndex(g);
    }
    for (unsigned int dir = 0; dir < 2; dir++) {
        TrafficPath *pPath = m_lanesPaths[dir];
        for (int j = 0; j < m_numLanePathLinks[dir]; j++, n++) {
            unsigned short g = 0xffff;
            if (m_pathLinkGeoIndices[n] != 0xfffe) g = pPath->findPathLinkGeoIndex(m_pathLinkPtrs[n]);
            m_pathLinkGeoIndices[n] = g;
            m_pathLinkPtrs[n]->setGeoIndex(g);
        }
    }
}

TrafficPathLink *TrafficLane::getPathLinkFromDist(float dist)
{
    TrafficPathLink *pLink = getPathLink(0);
    if (0.0f >= dist) return pLink;
    if (dist >= m_length) return getLastPathLink();
    while (!(pLink->getExtraLinkData()->getEndDistFromLaneStartXZ() > dist)) pLink = pLink->getNextLink();
    return pLink;
}

void TrafficPath::initLanes()
{
    reorderNodes();
}

TrafficSystem::~TrafficSystem()
{
    if (m_bOpen) close();
}

// NEAR: register allocation in the road/lane loops differs (2 bytes shorter).
void TrafficSystem::update()
{
    if (!m_bOpen) return;
    m_updateCount++;
    updateVehicleSubFiles();
    framesPerHour = XApp::FrameRate() * 3600.0f;
    oneKDivFramesPerHour = 1000.0f / framesPerHour;
    framesPerHourDivRateMul = framesPerHour / XApp::GameFrameRateMul();
    frameLen = XApp::FrameLength();
    lastFrameTime = frameTime;
    frameTime = frameLen + lastFrameTime;
    TrafficSpawnerUpdateVisibility();
    trig().update();
    for (unsigned short l = 0; l < getNumTrafficLights(); l++) getTrafficLight(l)->update();
    for (unsigned short b = 0; b < m_numBoxJunctions; b++) getBoxJunction(b)->update();
    TrafficMissionUpdate();
    unsigned int numRoads = getNumRoads();
    for (unsigned short r = 0; r < numRoads; r++) {
        TrafficRoad *pRoad = getRoad(r);
        for (unsigned int dir = 0; dir <= 1; dir++) {
            unsigned int numLanes = pRoad->getNumLanes(dir);
            for (unsigned char i = 0; i < numLanes; i++) {
                TrafficLane *pLane = pRoad->getLane(dir, i);
                VehicleListItem *p = (VehicleListItem *)pLane->getVehicleList().m_pHead;
                while (p) {
                    Vehicle *pVehicle = p->m_pVehicle;
                    VehicleListItem *pNext = p->Next();
                    if (!pVehicle->isUpdated() && pVehicle->m_currPos.getLoc().getLane() == pLane) pVehicle->update();
                    p = pNext;
                }
            }
        }
    }
}

signed char TrafficMissionInit(unsigned int);
void resetVehicleSubFiles();
static int s_unk4905f8;  // TODO name: file static between pApp and lastFrameTime, not in the map

// NEAR: this/constant register swap (esi/edi) and the order of the map-0x1000020 dest patch.
void TrafficSystem::open()
{
    if (m_bOpen) close();
    if (gCurrentMap_GeoHash == 0x1000020) return;
    reset();
    trig().preInit();
    TrafficMissionInit(0);
    m_pIntersectionsPath = (TrafficPath *)getWorld()->GetPath(0xb000006);
    if (m_pIntersectionsPath) m_pIntersectionsPath->init();
    m_lanesPaths[0] = (TrafficPath *)getWorld()->GetPath(0xb000004);
    if (m_lanesPaths[0]) m_lanesPaths[0]->init();
    m_lanesPaths[1] = (TrafficPath *)getWorld()->GetPath(0xb000005);
    if (m_lanesPaths[1]) m_lanesPaths[1]->init();
    signed char b;
    if (getNorthLanesPath() || getSouthLanesPath()) b = 1;
    else b = 0;
    m_bOpen = b;
    if (!m_bOpen) return;
    m_bNightLevel = 0;
    if (gCurrentMap_GeoHash == 0x10000a2) m_bNightLevel = 1;
    buildFromPaths();
    s_firstFreeVehicle = 20;
    s_unk4905f8 = 0;
    if (gCurrentMap_GeoHash == 0x1000020) {
        TrafficLaneDest *pDest = getRoad(1)->getLane(0, 0)->getEndDest(1);
        TrafficLane *pLane = getRoad(0)->getLane(0, 0);
        TrafficPathLink *pLink = pLane->getPathLink(0);
        pDest->setIntersection(getIntersection(3));
        pDest->setRoad(getRoad(0));
        pDest->setLane(pLane);
        pDest->setPathLink(pLink);
        pDest->getPos() = pLink->getNodeAPtr()->getPos();
        pDest->setDistFromLinkStartXZ(0.0f);
        pDest = getRoad(0)->getLane(1, 0)->getEndDest(0);
        pLane = getRoad(1)->getLane(1, 0);
        pLink = pLane->getPathLink(0);
        pDest->setIntersection(getIntersection(2));
        pDest->setRoad(getRoad(1));
        pDest->setLane(pLane);
        pDest->setPathLink(pLink);
        pDest->getPos() = pLink->getNodeAPtr()->getPos();
        pDest->setDistFromLinkStartXZ(0.0f);
    }
    postBuildSetup();
    lastFrameTime = 0.0f;
    frameTime = 0.0f;
    frameLen = 0.0f;
    framesPerHour = XApp::FrameRate() * 3600.0f;
    oneKDivFramesPerHour = 1000.0f / framesPerHour;
    framesPerHourDivRateMul = framesPerHour / XApp::GameFrameRateMul();
    resetVehicleSubFiles();
    trig().postInit(0);
}

void TrafficUpdate()
{
    if (_theTrafficSysAlloc.getSys()) _theTrafficSysAlloc.getSys()->update();
}

// XItemEnv derives from EXItemEnv at offset 0 (XItem.h still uses the EXItemEnv stand-in).
#define ITEMENV (&theItemEnv)

inline signed char TrafficSysAlloc::verifyPaths()
{
    if (ITEMENV->GetPath(0xb000004)) return 1;
    if (ITEMENV->GetPath(0xb000005)) return 1;
    return 0;
}

void TrafficSysAlloc::close()
{
    if (m_pSys) {
        m_pSys->close();
        delete m_pSys;
        m_pSys = 0;
    }
}

void TrafficDeinit()
{
    _theTrafficSysAlloc.close();
}

TrafficSysAlloc::~TrafficSysAlloc()
{
    close();
}

void TrafficIslandSectionDeLoaded(unsigned int section)
{
    if (gCurrentMap_GeoHash == 0x100006e && section == 0x8000002) TrafficDeinit();
}

signed char TrafficSysAlloc::open()
{
    if (!m_pSys) {
        if (!verifyPaths()) return 0;
        m_pSys = new TrafficSystem;
        if (!m_pSys) return 0;
    } else {
        if (!verifyPaths()) {
            close();
            return 0;
        }
        m_pSys->close();
    }
    m_pSys->open();
    return 1;
}

signed char TrafficInit()
{
    pWorld = &theItemEnv;
    pWnd = gpGameWnd[0];
    pApp = &theApp;
    return _theTrafficSysAlloc.open();
}

void TrafficIslandSectionLoaded(unsigned int section)
{
    if (gCurrentMap_GeoHash == 0x100006e && section == 0x8000002) TrafficInit();
}

TrafficPathLinkPtrArray VisPathLinks;
TrafficSysAlloc _theTrafficSysAlloc;
TrafficTriggers _theTrafficTriggers;

extern signed char DBG_bActorsAndVehicles;  // Actor.obj
template <class T> static inline const T &trMax(const T &a, const T &b) { return a < b ? b : a; }
void TrafficObjectiveVehicleSpawned(Vehicle *pVehicle, XItemHandler_Objective *pObjective);  // TrafficMission.obj

// Builds the traffic lights from their triggers (data 0 sequence, 1 light type, 2 duration) and
// sizes the location table to the highest HT_TriggerType_TrafficLoc index.
void TrafficTriggers::preInit()
{
    short maxLoc = -1;
    for (unsigned int i = 0; i < m_numTriggers; i++) {
        TrafficTrigger *pTrig = &m_triggers[i];
        if (pTrig->m_type == 0x1e000027) {  // HT_TriggerType_TrafficLight
            TrafficSystem *pSys = _theTrafficSysAlloc.getSys();
            TrafficLight *pLight = new TrafficLight(pTrig->m_data[1]);
            pLight->setIndex(pSys->m_numTrafficLights);
            pSys->m_trafficLights[pSys->m_numTrafficLights++] = pLight;
            pLight->setPos(pTrig->m_pos);
            pLight->setRot(pTrig->m_rot);
            pLight->setSequence(pTrig->m_data[0]);
            pLight->m_state = !(pTrig->m_data[0] & 1);
            pLight->update();
            pLight->setDuration((unsigned short)pTrig->m_data[2]);
        } else if (pTrig->m_type == 0x1e00002f) {  // HT_TriggerType_TrafficLoc
            short idx = (short)pTrig->m_data[0];
            maxLoc = trMax(maxLoc, idx);
        }
    }
    if (maxLoc >= 0) _theTrafficSysAlloc.getSys()->getLocs().SetSize(maxLoc + 1);
}

// Spawns a trigger's vehicle at the traffic location nearest the trigger. Unless channel 666 is
// given, a vehicle is never spawned on screen once traffic is running.
Vehicle *TrafficTriggers::spawnVehicle(TrafficTrigger_Vehicle *pTrig, int channel)
{
    if (channel == 999) channel = 0;
    TrafficLocation loc;
    _theTrafficSysAlloc.getSys()->calcLocFromPosition(pTrig->m_pos, loc, 0.0f);
    unsigned char unk = 0;
    if (channel != 666) {
        unk = 1;
        if (_theTrafficSysAlloc.getSys()->getUpdateCount() > 2) {
            EXVector3 p3;
            loc.calcPos(p3);
            EXVector p(p3, 0.0f);
            if (Vehicle::isOnScreen(pTrig->m_data[0], p)) return 0;
        }
    }
    Vehicle *pVehicle =
        _theTrafficSysAlloc.getSys()->getPopulace().addVehicle(vehicleTypeFromHash(pTrig->m_data[0]), &loc);
    if (!pVehicle) return 0;
    pVehicle->m_unk708 = unk;
    pVehicle->m_vehFlags = pTrig->m_flags;
    if (pTrig->m_flags & 2) pVehicle->initHealthIndicator();
    if ((unsigned short)pTrig->m_data[7]) pVehicle->getMove().setMaxSpeed((unsigned short)pTrig->m_data[7]);
    float throttle = *(float *)&pTrig->m_data[6];
    if (throttle != 0.0f) pVehicle->getMove().setThrottle(throttle);
    unsigned int sfx = pTrig->m_data[8];
    if (sfx) {
        if ((int)sfx < 0) sfx = g_EXRandClass.Rand32() & 0xf;
        pVehicle->m_sfx.play(4, sfx);
    }
    if (*(float *)&pTrig->m_data[5] != 0.0f) pVehicle->getMove().m_phys.m_f140 = *(float *)&pTrig->m_data[5];
    if (pTrig->m_data[15]) pVehicle->getMove().setState(7);
    if (pTrig->m_data[12]) {
        float health = (float)(int)pTrig->m_data[12];
        pVehicle->m_health = health;
        pVehicle->m_maxHealth = health;
    }
    if (pTrig->m_links[0] != -1) {
        XItem *pItem = Mission_GetMissionItem(pTrig->m_links[0]);
        if (pItem) {
            EXItemHandler *pHandler = pItem->m_pItemHandler;
            if (pHandler->IsKindOf(EXRUNTIME_CLASS(XItemHandler_Objective))) {
                XItemHandler_Objective *pObjective = (XItemHandler_Objective *)pHandler;
                pVehicle->objectiveInit(pObjective, pTrig->m_data[1]);
                TrafficObjectiveVehicleSpawned(pVehicle, pObjective);
                if ((pObjective->m_SuccessMask & 0x80) || (pObjective->m_FailMask & 0x80)) pObjective->m_DestroyCount++;
            }
        }
    }
    if (pTrig->m_data[2] && pTrig->m_data[2] != 0xb000000) pVehicle->getMove().buildRouteFromPath(pTrig->m_data[2]);
    if ((unsigned char)pTrig->m_data[3]) pVehicle->m_unk708 = (unsigned char)pTrig->m_data[3];
    if (pTrig->m_data[4] && pTrig->m_data[4] != 0x51000000) pVehicle->m_unk730 = pTrig->m_data[4];
    pTrig->m_data[1] = 0;
    return pVehicle;
}

// Spawns each waiting vehicle trigger once its flag channel is raised (999: at once).
void TrafficTriggers::update()
{
    for (unsigned int i = 0; i < m_numTriggers; i++) {
        TrafficTrigger *pTrig = &m_triggers[i];
        if (pTrig->m_type == 0x1e00002e && DBG_bActorsAndVehicles && pTrig->m_data[1]) {  // HT_TriggerType_Vehicle
            int channel = pTrig->m_data[1];
            if (channel == 999) channel = 0;
            else if (!FlagChannels[channel]) continue;
            spawnVehicle((TrafficTrigger_Vehicle *)pTrig, channel);
        }
    }
}

// File statics of Traffic.obj (between the map's pWorld/framesPerHour entries): the vehicle budget
// and the 25-slot random spawn table filled by postInit.
static unsigned int s_maxSpawnVehicles;
EXFixedArray<unsigned char, 25> SpawnVehicleTypes;

// Second pass over the triggers once the map is up (bReset: only respawn the placed vehicles,
// at once, even on screen). Vehicle triggers spawn here too; location triggers fill the location
// table; the 0x4d trigger gives up to four (vehicle type, count) pairs, otherwise the budget is
// split a third each over three default types. The pairs are scattered over SpawnVehicleTypes
// (random slot, probing on) and every type used has its sub files loaded.
void TrafficTriggers::postInit(signed char bReset)
{
    unsigned int total = 0;
    unsigned int numTypes = 0;
    unsigned int counts[4];
    unsigned int types[4];
    if (bReset) _theTrafficSysAlloc.getSys()->getPopulace().removeVehicles();
    for (unsigned int i = 0; i < m_numTriggers; i++) {
        TrafficTrigger *pTrig = &m_triggers[i];
        if (bReset && pTrig->m_type != 0x1e00002e) continue;
        switch (pTrig->m_type) {
        case 0x1e00002e: {  // HT_TriggerType_Vehicle
            if (pTrig->m_data[13]) s_maxSpawnVehicles = pTrig->m_data[13];
            if (pTrig->m_data[0] == 0x4a000000 || pTrig->m_data[0] == 0) {
                pTrig->m_data[1] = 0;
            } else {
                int channel = pTrig->m_data[1];
                if (channel == 999) channel = 0;
                else if (channel != 0 && !FlagChannels[channel]) break;
                if (!bReset) {
                    if ((s_maxSpawnVehicles == 0 && total == 0) || channel != 0)
                        spawnVehicle((TrafficTrigger_Vehicle *)pTrig, 0);
                } else {
                    spawnVehicle((TrafficTrigger_Vehicle *)pTrig, 0x29a);
                }
            }
            break;
        }
        case 0x1e00002f: {  // HT_TriggerType_TrafficLoc
            TrafficLocation loc;
            _theTrafficSysAlloc.getSys()->calcLocFromPosition(pTrig->m_pos, loc, *(float *)&pTrig->m_data[1]);
            _theTrafficSysAlloc.getSys()->getLocs()[(unsigned short)pTrig->m_data[0]] = loc;
            break;
        }
        case 0x1e00004d:
            s_unk4905f8 = pTrig->m_data[0];
            for (int k = 0; k < 4; k++) {
                unsigned int hash = pTrig->m_data[1 + k];
                if (hash != 0 && hash != 0x4a000000) {
                    unsigned int count = pTrig->m_data[5 + k];
                    types[numTypes] = vehicleTypeFromHash(hash) & 0xff;
                    total += count;
                    counts[numTypes] = count;
                    numTypes++;
                }
            }
            break;
        }
    }
    if (total == 0) {
        counts[0] = s_maxSpawnVehicles / 3;
        counts[1] = counts[0];
        total = s_maxSpawnVehicles;
        numTypes = 3;
        types[0] = 1;
        types[1] = 2;
        types[2] = 0;
        counts[2] = s_maxSpawnVehicles - counts[0] * 2;
    }
    s_maxSpawnVehicles = total;
    memset(&SpawnVehicleTypes, 0xff, total);
    for (unsigned int t = 0; t < numTypes; t++) {
        unsigned int type = types[t];
        VehicleTypeBase *pType = vehicleTypeObjFromType((unsigned char)type);
        pType->loadSubFile(0);
        pType->loadSubFile(0);
        pType->loadSubFile(0);
        for (unsigned int n = counts[t]; n != 0; n--) {
            unsigned int slot = 0;
            unsigned int tries = 0;
            bool placed = false;
            do {
                slot = total ? g_EXRandClass.Rand32() % total : 0;
                if (SpawnVehicleTypes[slot] == 0xff) {
                    placed = true;
                    break;
                }
                tries++;
            } while (tries < 8);
            if (!placed) {
                slot = total ? g_EXRandClass.Rand32() % total : 0;
                for (;;) {
                    if (slot == total) slot = 0;
                    if (SpawnVehicleTypes[slot] == 0xff) break;
                    slot++;
                }
            }
            SpawnVehicleTypes[slot] = (unsigned char)type;
        }
    }
    TrafficSpawnerInit();
}

// Per-frame spawner: first collects up to 64 path links crossing a 40 unit sphere 35 units ahead
// of the player (VisPathLinks), then walks the vehicle slots. Empty slots take at most two new
// vehicles a frame, on the next visible link (or the nearest linked one) with room for another
// (35 units a vehicle) and not on screen; live ones unspawn once off screen for 4 time units, at
// most one a frame. Channel s_unk4905f8 (a mission trigger) clears everything instead.
void TrafficSpawnerUpdateVisibility()
{
    TrafficSystem *pSys = _theTrafficSysAlloc.getSys();
    if (s_unk4905f8 && FlagChannels[s_unk4905f8]) {
        for (unsigned int i = 0; i < s_maxSpawnVehicles; i++) {
            Vehicle *pVehicle = pSys->getPopulace().getVehicle((unsigned short)i);
            pVehicle->remove();
            pVehicle->setIndex((unsigned char)i);
        }
        s_maxSpawnVehicles = 0;
        s_unk4905f8 = 0;
        return;
    }
    if (!s_maxSpawnVehicles) return;

    EXVector feet = Player_GetFeetPos();
    feet.y = 0.0f;
    float rotY = Player_GetRot().y;
    float sinY = (float)sin(rotY);
    float cosY = (float)cos(rotY);
    nVisPathLinks = 0;
    feet.x = sinY * 35.0f + feet.x;
    feet.z = cosY * 35.0f + feet.z;
    EXSphere sphere;
    sphere.Set(feet, 40.0f);

    for (int i = 0; i < pSys->m_numPathLinks - pSys->m_numIntersections; i++) {
        TrafficPathLink *pLink = pSys->m_pathLinkPtrs[pSys->m_numIntersections + i];
        TrafficPath *pPath = pLink->getPath();
        TrafficPathNode *pA = pPath->Node(pLink->getNodeA());
        TrafficPathNode *pB = pPath->Node(pLink->getNodeB());
        EXLineSeg seg;
        seg.m_origin = EXVector(pA->getPos(), 0.0f);
        seg.m_dir = EXVector(pB->getPos().x - pA->getPos().x, pB->getPos().y - pA->getPos().y,
                             pB->getPos().z - pA->getPos().z, 0.0f);
        if (EXIntersection(seg, sphere)) {
            VisPathLinks[nVisPathLinks++] = pLink;
            if (nVisPathLinks == 64) break;
        }
    }

    int spawned = 0;
    int removed = 0;
    for (int v = 0; v < (int)s_maxSpawnVehicles; v++) {
        Vehicle *pVehicle = pSys->getPopulace().getVehicle((unsigned short)v);
        if (!pVehicle->isInit()) {
            if (nVisPathLinks == 0 || spawned >= 2) continue;
            if (iLastVisLinkIndexChecked >= nVisPathLinks) iLastVisLinkIndexChecked = 0;
            TrafficPathLink *pLink = VisPathLinks[iLastVisLinkIndexChecked++];
            if (!pLink->getLane()->hasValidDest()) continue;
            TrafficPathLink *pUse = pLink;
            if (CountVehiclesOnLink(pUse) >= (int)pUse->getExtraLinkData()->getLengthXZ() / 35 + 1) {
                pUse = pLink->getNextLink();
                while (pUse && CountVehiclesOnLink(pUse) >= (int)pUse->getExtraLinkData()->getLengthXZ() / 35 + 1)
                    pUse = pUse->getNextLink();
                if (!pUse) {
                    pUse = pLink->getPrevLink();
                    while (pUse && CountVehiclesOnLink(pUse) >= (int)pUse->getExtraLinkData()->getLengthXZ() / 35 + 1)
                        pUse = pUse->getPrevLink();
                    if (!pUse) continue;
                }
            }
            unsigned char type = SpawnVehicleTypes[v];
            TrafficLocation loc;
            if (!FindSpawnPlaceOnLink(pVehicle, pUse, loc, type)) continue;
            EXVector3 p3;
            loc.calcPos(p3);
            EXVector pos(p3, 0.0f);
            if (Vehicle::isOnScreen(type, pos)) continue;
            pVehicle->create(type, &loc);
            if (v % 5 == 0) pVehicle->m_sfx.play(4, g_EXRandClass.Rand32() & 0xf);
            spawned++;
            LastVclVisTimes[v] = frameTime;
        } else if (removed < 1) {
            if (checkForAndUnspawnVehicle(pVehicle)) removed++;
        }
    }
}

// Builds the traffic location nearest pos: the link (over every lane of every road) whose
// segment is closest, with the XZ distance from that link's start as the start distance.
signed char TrafficSystem::calcLocFromPosition(EXVector &pos, TrafficLocation &loc, float offset)
{
    float bestDist = 3.4028235e+38f;
    TrafficPathLink *pBest = 0;
    for (unsigned int r = 0; r < m_numRoads; r++) {
        TrafficRoad *pRoad = getRoad((unsigned short)r);
        for (unsigned int dir = 0; dir < 2; dir++) {
            for (unsigned int l = 0; l < pRoad->getNumLanes((unsigned char)dir); l++) {
                TrafficLane *pLane = pRoad->getLane((unsigned char)dir, (unsigned char)l);
                for (unsigned int k = 0; k < pLane->getNumPathLinks(); k++) {
                    TrafficPathLink *pLink = pLane->getPathLink((unsigned char)k);
                    TrafficPathNode *pA = pLink->getNodeAPtr();
                    TrafficPathNode *pB = pLink->getNodeBPtr();
                    EXLineSeg seg;
                    seg.m_origin = EXVector(pA->getPos(), 0.0f);
                    seg.m_dir = EXVector(pB->getPos().x - pA->getPos().x, pB->getPos().y - pA->getPos().y,
                                         pB->getPos().z - pA->getPos().z, 0.0f);
                    float d = dist3D_Point_to_LineSegment(pos, seg);
                    if (d < bestDist) {
                        pBest = pLink;
                        bestDist = d;
                    }
                }
            }
        }
    }
    TrafficLane *pLane = pBest->getLane();
    float dist = pBest->getDistFromLinkStartXZ((const EXVector3 *)&pos, 0);
    loc = TrafficLocation(pLane->getRoad()->getIndex(), pLane->getDir(), pLane->getIndex(), pBest->getIndex(), dist,
                          offset);
    return 1;
}

// Finds a free stretch of a link's lane for a new vehicle of the given type (0xff: the vehicle's
// own) and returns it as a location in loc. Gaps between the vehicles already on the link (and the
// turn margins at either end) must hold the vehicle plus 10 units; one of up to 16 gaps is picked at
// random and the vehicle placed at a random spot at least 5 units in.
signed char FindSpawnPlaceOnLink(Vehicle *pVehicle, TrafficPathLink *pLink, TrafficLocation &loc, unsigned char type)
{
    VehicleTypeBase *pType = type == 0xff ? pVehicle->getTypeObject() : vehicleTypeObjFromType(type);
    float len = pType->m_zDim + 2.0f;
    TrafficExtraLinkData *pExtra = pLink->getExtraLinkData();
    float minGap = len + 10.0f;
    TrafficLane *pLane = pLink->getLane();
    float start = s_turnRadii[pLink->getNodeAPtr()->getTurnType()] + pExtra->getDistFromLaneStartXZ();
    float end = (pExtra->getLengthXZ() + pExtra->getDistFromLaneStartXZ()) -
                s_turnRadii[pLink->getNodeBPtr()->getTurnType()];
    float gapEnd[16];
    float gapStart[16];
    if (end - start <= minGap) return 0;
    unsigned int n = 0;
    VehicleListItem *pItem = (VehicleListItem *)pLane->getVehicleList().m_pHead;
    if (pItem) {
        for (; pItem; pItem = (VehicleListItem *)pItem->m_pNext) {
            Vehicle *pOther = pItem->m_pVehicle;
            if (pOther == pVehicle) continue;
            if (pOther->getCurrPos().getLoc().getPathLink() != pLink) continue;
            float rear = pOther->getCurrPos().getInLane(0).getRearLaneDist();
            if (minGap <= rear - start) {
                gapStart[n] = start;
                gapEnd[n] = rear;
                if (end < rear) gapEnd[n] = end - 1e-5f;
                n++;
                if (n == 16) break;
            }
            start = pOther->getCurrPos().getInLane(0).getFrontLaneDist();
        }
    }
    if (n == 0) {
        if (end <= start + minGap) return 0;
        n = 1;
        gapEnd[0] = end;
        gapStart[0] = start;
    }
    unsigned int r = g_EXRandClass.Rand32();
    float s0 = gapStart[r % n];
    float room = gapEnd[r % n] - s0;
    if (!(minGap < room)) return 0;
    float rearDist = pType->m_zDistToRear;
    float place = (rearDist + 1.0f + ((room - len) - 5.0f) * g_EXRandClass.Randf() + 5.0f + s0) -
                  pExtra->getDistFromLaneStartXZ();
    loc = TrafficLocation(pLink->getRoad()->getIndex(), pLink->getDir(), pLane->getIndex(), pLink->getIndex(), place,
                          0.0f);
    loc.m_flag = 0;
    return 1;
}

static int s_randRoadDir = 1;  // flips each call: which way the road search walks

// Picks a random spawn spot clear of the traffic: a random road (walking up or down from it if
// that fails), either direction, a random lane that leads somewhere, and a spot on it at least
// a+b from the lane ends and a+b clear of every vehicle already there. loc.m_flag is set when
// nothing was found anywhere.
void TrafficSystem::getRandLocation(TrafficLocation &loc, float a, float b)
{
    unsigned int roadsTried = 0;
    int phase = 0;
    unsigned int road = m_numRoads ? g_EXRandClass.Rand32() % m_numRoads : 0;
    unsigned int dirSel = 0;
    float clear = a + b;
    s_randRoadDir = -s_randRoadDir;
    for (;;) {
        if (phase != 0) {
            if (phase == 1) {
                dirSel ^= 1;
            } else {
                unsigned int numRoads = m_numRoads;
                roadsTried++;
                phase = 0;
                if (roadsTried == numRoads) {
                    loc.m_flag = 1;
                    return;
                }
                if (s_randRoadDir == -1) {
                    if (road == 0) road = numRoads;
                    road--;
                } else {
                    road++;
                    if (road == numRoads) road = 0;
                }
            }
        }
        int nextPhase = phase + 1;
        if (phase == 0) dirSel = (g_EXRandClass.Rand32() >> 13) & 1;
        phase = nextPhase;
        if (!m_lanesPaths[dirSel]) continue;
        TrafficRoad *pRoad = getRoad((unsigned short)road);
        unsigned int numLanes = pRoad->getNumLanes((unsigned char)dirSel);
        if (!numLanes) continue;
        unsigned int laneSel = numLanes ? g_EXRandClass.Rand32() % numLanes : 0;
        TrafficLane *pLane = pRoad->getLane((unsigned char)dirSel, (unsigned char)laneSel);
        TrafficPathLink *pFirst = pLane->getPathLink(0);
        if (!(pFirst->getNodeAPtr()->Flags() & 8)) {
            TrafficLaneDest *pDest = pLane->getEndDest(2);
            if (!pDest || pDest->isDeadEnd() || pDest->isLeadingToDeadEnd()) continue;
        }
        unsigned int numLinks = pLane->getNumPathLinks();
        TrafficPathLink *pLast = pLane->getPathLink((unsigned char)(numLinks - 1));
        float laneLen = pLast->getExtraLinkData()->getDistFromLaneStartXZ() + pLast->getExtraLinkData()->getLengthXZ();
        if (laneLen <= clear) continue;
        if (pLane->getVehicleList().m_pHead) {
            unsigned int linkIdx = numLinks ? g_EXRandClass.Rand32() % numLinks : 0;
            TrafficPathLink *pLink = pLane->getPathLink((unsigned char)linkIdx);
            TrafficPathNode *pNode = pLink->getNodeAPtr();
            TrafficExtraLinkData *pExtra = pLink->getExtraLinkData();
            float linkLen = pExtra->getLengthXZ();
            if (clear > linkLen) continue;
            float at = g_EXRandClass.Randf() * linkLen;
            if (at < clear) at = clear;
            if (!(at + clear < linkLen)) continue;
            TrafficLocation tryLoc(road, dirSel, laneSel, linkIdx, at, 0.0f);
            EXVector3 pos;
            pos.x = pExtra->getSlopeX() * at + pNode->getPos().x;
            pos.y = pNode->getPos().y;
            pos.z = pExtra->getSlopeZ() * at + pNode->getPos().z;
            float myDist = pLink->getDistFromLaneStartXZ(&pos, linkIdx, 1);
            bool blocked = false;
            for (VehicleListItem *pItem = (VehicleListItem *)pLane->getVehicleList().m_pHead; pItem;
                 pItem = (VehicleListItem *)pItem->m_pNext) {
                Vehicle *pOther = pItem->m_pVehicle;
                EXVector3 otherPos;
                otherPos.x = pOther->GetItem()->Position().x;
                otherPos.y = pOther->GetItem()->Position().y;
                otherPos.z = pOther->GetItem()->Position().z;
                VehicleTypeBase *pType = pOther->getTypeObject();
                float otherDist =
                    pOther->getCurrPos().getLoc().getPathLink()->getDistFromLaneStartXZ(&otherPos, 0x9999, 1);
                float diff = (myDist - otherDist) + pType->m_zDistToFront;
                if (!(fabsf(diff) >= clear) || !(fabsf(diff - pType->m_zDim) >= clear)) {
                    blocked = true;
                    break;
                }
            }
            if (blocked) continue;
            loc = tryLoc;
            loc.m_flag = 0;
            return;
        }
        float spot = (((laneLen - b) - 0.1f) - a) * g_EXRandClass.Randf() + a;
        float before, after;
        int idx = pLane->calcLinkFromDist(spot - a, before, after, a, b);
        if (idx == 0x9999) continue;
        bool restart = false;
        for (;;) {
            float step;
            if (before != 0.0f) {
                if (after != 0.0f) {
                    restart = true;
                    break;
                }
                step = before;
            } else {
                if (after == 0.0f) break;
                if (laneLen <= spot + b) {
                    restart = true;
                    break;
                }
                step = clear;
            }
            spot += step;
            idx = pLane->calcLinkFromDist(spot - a, before, after, a, b);
            if (idx == 0x9999) {
                restart = true;
                break;
            }
        }
        if (restart) continue;
        loc = TrafficLocation(road, dirSel, laneSel, idx,
                              spot - pLane->getPathLink((unsigned char)idx)->getExtraLinkData()->getDistFromLaneStartXZ(),
                              0.0f);
        loc.m_flag = 0;
        return;
    }
}

// Puts a path's links into chains: each link followed by one that starts where it ends (flipping
// links the wrong way round), roots first. Then flags chain ends as dead ends and chain starts as
// roots, and stamps every used node with its own index.
void TrafficPath::reorderNodes()
{
    reorderRoots(0);
    for (unsigned int i = 0; (int)i < (int)NumLinks(); i++) {
        TrafficPathLink *pLink = Link(i);
        if (pLink->isRemovedNotTriggerLink()) continue;
        if (!pLink->isRoot() && Node(pLink->getNodeB())->isLoop()) {
            if (i != 0) {
                TrafficPathLink *pPrev = Link(i - 1);
                if (Node(pLink->getNodeA()) == Node(pPrev->getNodeB())) continue;
            }
            reorderRoots(i);
            pLink = Link(i);
        }
        for (unsigned int j = i + 1; (int)j < (int)NumLinks(); j++) {
            TrafficPathLink *pOther = Link(j);
            if (pOther->isRemovedNotTriggerLink()) continue;
            if (pLink->getNodeB() == pOther->getNodeA()) {
                swapLinks(j, i + 1);
                if ((int)(i + 2) < (int)j) insertLink(i + 2, j);
                break;
            }
            if (pLink->getNodeB() == pOther->getNodeB()) {
                pOther->swapNodes();
                swapLinks(j, i + 1);
                if ((int)(i + 2) < (int)j) insertLink(i + 2, j);
                break;
            }
        }
    }
    for (unsigned int j = 0; (int)j < (int)NumLinks(); j++) {
        TrafficPathLink *pLink = Link(j);
        if (pLink->isRemovedNotTriggerLink()) continue;
        Node(pLink->getNodeA())->setIndex(pLink->getNodeA());
        Node(pLink->getNodeB())->setIndex(pLink->getNodeB());
        TrafficPathLink *pNext = 0;
        for (unsigned int k = j + 1; (int)k < (int)NumLinks();) {
            TrafficPathLink *p = Link(k);
            k++;
            if (!p || p->getNodeA() != 0x9999) {
                pNext = p;
                break;
            }
        }
        TrafficPathNode *pEnd = Node(pLink->getNodeB());
        if (!pNext) {
            if (!pEnd->isLoop()) pEnd->setAsDeadEnd();
            return;
        }
        if (pLink->getNodeB() == pNext->getNodeA()) {
            pNext->clrFlags(1);
            continue;
        }
        if (!pEnd->isLoop()) pEnd->setAsDeadEnd();
        if (!pNext->isRoot()) pNext->setAsRoot();
        TrafficPathNode *pStart = Node(pNext->getNodeA());
        if (!pStart->isRoot()) pStart->setAsRoot();
    }
}

// Swaps two lanes' contents (they keep their own slot indices), re-stamping the lane index on each
// lane's links and nodes, then repoints every other road's lane end destinations that led to one
// lane at the other.
void TrafficLane::swap(TrafficLane *pOther)
{
    TrafficPath *pPath = _theTrafficSysAlloc.getSys()->getLanesPath(m_dir);
    unsigned int indices[2];
    TrafficLaneDest *pDests[16];
    unsigned int numDests = 0;
    indices[0] = m_index;
    indices[1] = pOther->m_index;
    unsigned char tmp[0x144];
    memcpy(tmp, this, 0x144);
    memcpy(this, pOther, 0x144);
    memcpy(pOther, tmp, 0x144);
    TrafficLane *pLane = this;
    for (unsigned int a = 0; a < 2; a++) {
        unsigned char index = (unsigned char)indices[a];
        pLane->m_index = index;
        for (unsigned int k = 0; k < pLane->m_numPathLinks; k++) {
            TrafficPathLink *pLink = pLane->getPathLink((unsigned char)k);
            pLink->setLaneIndex((unsigned short)indices[a]);
            pPath->Node(pLink->getNodeA())->setLaneIndex(index);
            pPath->Node(pLink->getNodeB())->setLaneIndex(index);
        }
        TrafficRoad *pRoad = pLane->m_pRoad;
        TrafficSystem *pSys = _theTrafficSysAlloc.getSys();
        for (unsigned int r = 0; r < pSys->m_numRoads; r++) {
            TrafficRoad *pOtherRoad = pSys->getRoad((unsigned short)r);
            if (pOtherRoad == pRoad) continue;
            for (unsigned int d = 0; d < 2; d++) {
                for (unsigned int l = 0; l < pOtherRoad->getNumLanes((unsigned char)d); l++) {
                    TrafficLane *pLane2 = pOtherRoad->getLane((unsigned char)d, (unsigned char)l);
                    for (unsigned int e = 0; e < 3; e++) {
                        TrafficLaneDest *pDest = pLane2->getEndDest(e);
                        if (pDest->getRoad() && pDest->getLane() == pLane) {
                            if (a == 0) pDests[numDests++] = pDest;
                            else pDest->setLane(this);
                        }
                    }
                }
            }
        }
        pLane = pOther;
    }
    for (unsigned int i = 0; i < numDests; i++) pDests[i]->setLane(pOther);
}

// Adds a link to a direction's additional-link array and slots it into the lane path at index,
// shifting the later links up one (re-stamping their indices). It starts as a copy of pSrc (or
// zeroed), gets its length from its end nodes, and ppOut gets it.
unsigned short TrafficSystem::insertAdditionalPathLink(unsigned char dir, unsigned char index, TrafficPathLink *pSrc,
                                                       TrafficPathLink **ppOut)
{
    unsigned short n = m_numAdditionalPathLinks[dir];
    TrafficPath *pPath = m_lanesPaths[dir];
    m_numAdditionalPathLinks[dir] = n + 1;
    memset(&m_additionalPathLinks[dir].ElementAt(n), 0, sizeof(TrafficPathLink));
    TrafficPathLink *pLink = pPath->Link(index);
    if (index < pPath->NumLinks()) {
        unsigned int last = pPath->NumLinks() - 1;
        while (index < last) {
            TrafficPathLink *pFrom = pPath->Link(last - 1);
            TrafficPathLink *pTo = pPath->Link(last);
            *pTo = *pFrom;
            pTo->setIndex((unsigned char)last);
            last--;
        }
    }
    if (ppOut) *ppOut = pLink;
    if (pSrc) *pLink = *pSrc;
    else memset(pLink, 0, sizeof(TrafficPathLink));
    pLink->setIndex(index);
    TrafficPathNode *pA = pLink->getNodeAPtr();
    TrafficPathNode *pB = pLink->getNodeBPtr();
    float sq = (pB->getPos().x - pA->getPos().x) * (pB->getPos().x - pA->getPos().x) +
               (pB->getPos().y - pA->getPos().y) * (pB->getPos().y - pA->getPos().y) +
               (pB->getPos().z - pA->getPos().z) * (pB->getPos().z - pA->getPos().z);
    pLink->setLength(sq >= 0.0f ? EXSqrt(sq) : 0.0f);
    return index;
}

// A path node's link-time data: if it has not been claimed already (lane unassigned, index not
// the 0x9999 "none"), its old index byte becomes its turn type and the road/lane/direction/
// intersection fields are reset to 0x99 ("unset"), the index to the given value.
static __forceinline void InitPathNode(TrafficPathNode *pNode, unsigned short index)
{
    if (pNode->m_index != 0x9999 && pNode->m_roadIndex != 0x9999 && pNode->m_laneIndex != 0x99) {
        pNode->m_turnType = (u8)pNode->m_index;
        pNode->m_index = index;
        pNode->m_roadIndex = 0x99;
        pNode->setLaneIndex(0x99);
        pNode->m_dir = 0;
        pNode->m_intersectionIndex = 0x99;
    }
}

// First pass over a path's links: resets their lane bookkeeping, tags intersection / south links,
// resets their end nodes, and registers intersection links (direction 2) with the system. Then
// removes repeated links and orders the nodes (or builds the intersections).
void TrafficPath::init()
{
    TrafficPath *pIntersections = _theTrafficSysAlloc.getSys()->getIntersectionsPath();
    for (unsigned int i = 0; i < NumLinks(); i++) {
        TrafficPathLink *pLink = Link(i);
        pLink->clrFlags(0x80008000);
        if (this == _theTrafficSysAlloc.getSys()->getIntersectionsPath()) pLink->setAsIntersectionType();
        else if (this == _theTrafficSysAlloc.getSys()->getSouthLanesPath()) pLink->setAsSouthType();
        pLink->setIndex(0x99);
        pLink->setRoadIndex(0x9999);
        pLink->setLaneIndex(0x9999);
        pLink->setDir(0x99);
        pLink->setGeoIndex(0xfffe);
        TrafficPathNode *pA = Node(pLink->getNodeA());
        InitPathNode(pA, 0x9999);
        TrafficPathNode *pB = Node(pLink->getNodeB());
        InitPathNode(pB, pLink->getNodeB());
        if (this == pIntersections) {
            pLink->setGeoIndex((unsigned short)i);
            pLink->setDir(2);
            _theTrafficSysAlloc.getSys()->addPathLink(pLink, 2);
            pA->m_dir = 2;
            pB->m_dir = 2;
        }
    }
    removeRepeatLinks();
    if (this != _theTrafficSysAlloc.getSys()->getIntersectionsPath()) reorderNodes();
    else initIntersections();
}

// Builds the intersection table from the intersection path: one intersection per ordinary link,
// traffic lights matched to the trigger links (which sit on a light's pole) and attached to the
// intersections those links cross, and every intersection's neighbours at either end of its link.
void TrafficPath::initIntersections()
{
    reorderNodes();
    TrafficSystem *pSys = _theTrafficSysAlloc.getSys();
    unsigned int count = 0;
    for (unsigned int i = 0; (int)i < (int)NumLinks(); i++) {
        TrafficPathLink *pLink = Link(i);
        if (!pLink->isRemovedNotTriggerLink() && !pLink->isIntersectionTriggerToIntersectionLink()) count++;
    }
    pSys->getIntersections().SetSize(count);
    for (unsigned int i = 0; (int)i < (int)NumLinks(); i++) {
        TrafficPathLink *pLink = Link(i);
        if (!pLink->isRemovedNotTriggerLink() && !pLink->isIntersectionTriggerToIntersectionLink())
            pSys->addIntersection(pLink);
    }

    TrafficPathLink *triggers[64];
    TrafficLight *lights[64];
    unsigned int numTriggers = 0;
    for (unsigned int i = 0; (int)i < (int)NumLinks(); i++) {
        TrafficPathLink *pLink = Link(i);
        if (pLink->isIntersectionTriggerToIntersectionLink()) triggers[numTriggers++] = pLink;
    }
    for (unsigned int i = 0; i < numTriggers; i++) lights[i] = 0;

    for (unsigned int k = 0; k < numTriggers; k++) {
        TrafficPathLink *pTrigger = triggers[k];
        if (!pTrigger->isIntersectionTriggerToIntersectionLink()) continue;
        if (pTrigger->isRoot()) {
            TrafficPathNode *pA = Node(pTrigger->getNodeA());
            TrafficPathNode *pB = Node(pTrigger->getNodeB());
            for (unsigned int l = 0; l < pSys->m_numTrafficLights; l++) {
                TrafficLight *pLight = pSys->getTrafficLight((unsigned short)l);
                if (pLight->getNumIntersections() == 0) {
                    EXVector p = pLight->getItem(0)->Position();
                    EXVector da = EXVector(p.x - pA->getPos().x, p.y - pA->getPos().y, p.z - pA->getPos().z, p.w);
                    EXVector db = EXVector(p.x - pB->getPos().x, p.y - pB->getPos().y, p.z - pB->getPos().z, p.w);
                    if (da.Length() < 1.0f || db.Length() < 1.0f) {
                        lights[k] = pLight;
                        break;
                    }
                }
            }
        }
        for (unsigned int m = 0; m < pSys->m_numIntersections; m++) {
            TrafficIntersection *pInt = pSys->getIntersection((unsigned short)m);
            if (!pTrigger->doLinksIntersect(pInt->getPathLink(), 0, 0, 0.1f)) continue;
            int idx = k;
            if (!pTrigger->isRoot()) {
                idx = (int)k - 1;
                while (idx >= 0 && !triggers[idx]->isRoot()) idx--;
            }
            if (idx < 0) continue;
            TrafficLight *pLight = lights[idx];
            pLight->addIntersection(pInt);
            pInt->setTrafficLight(pLight);
            if (pLight->getState()) pInt->setAsAIExitBlocked(0xff);
            else pInt->clrAsAIExitBlocked(0xff);
        }
    }

    for (unsigned int a = 0; a < pSys->m_numIntersections; a++) {
        TrafficIntersection *pIntA = pSys->getIntersection((unsigned short)a);
        TrafficPathLink *pLinkA = pIntA->getPathLink();
        for (unsigned int b = 0; b < pSys->m_numIntersections; b++) {
            if (a == b) continue;
            TrafficIntersection *pIntB = pSys->getIntersection((unsigned short)b);
            EXVector hit;
            TrafficPathNode *pNode;
            if (pLinkA->doLinksIntersect(pIntB->getPathLink(), &hit, &pNode, 0.1f)) {
                pNode->getPos().x = hit.x;
                pNode->getPos().y = hit.y;
                pNode->getPos().z = hit.z;
                if (pNode == Node(pLinkA->getNodeA())) pIntA->setIntersectionA(pIntB);
                else pIntA->setIntersectionB(pIntB);
            }
        }
    }
}
