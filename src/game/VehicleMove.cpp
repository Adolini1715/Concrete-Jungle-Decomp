#include "FEText.h"
#include "Vehicle.h"
#include "TrafficSystem.h"
#include "EXRand.h"
#include "EXItem.h"
#include "EXDatum.h"
#include "XItemHandler_Bullet.h"
#include "XItemHandler_Player.h"
#include "XItem.h"
#include "ActorUtil.h"
#include "Actor.h"
#include "ActorOptions.h"
#include "XItemHandler_Objective.h"
#include "EXLineSeg.h"
#include "EXSphere.h"
#include "EXIntersection.h"

// NEAR: retail multiplies straight into v (one fewer copy of the sum).
inline EXVector &VehiclePos::getFrontBottomCentre(EXVector &v) const
{
    v = EXVector(m_transformedBoxVerts[0] + m_transformedBoxVerts[1]) * 0.5f;
    return v;
}

inline void VehiclePos::setLocLink(unsigned char i)
{
    m_loc.m_linkIndex = i;
    m_loc.m_linkId = TrGetPathLink((unsigned short)i + m_loc.getLane()->getPathLinkIndex())->getArrayIndex();
}

static __forceinline EXVector vmSub(const EXVector &a, const EXVector &b)
{
    return EXVector(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

static __forceinline float vmLengthXZ(const EXVector &d)
{
    EXMaths::vec_xyzw v = d;
    return EXSqrt(v.x * v.x + v.z * v.z);
}

inline TrafficPathNode *TrafficSafePathNodePtr::get()
{
    if (m_node == 0xffff) return 0;
    return _theTrafficSysAlloc.getSys()->getLanesPath(m_path)->getNodePtr(m_node);
}

inline TrafficPathNode *VehicleMove::getAtNode() { return m_atNode.get(); }
inline TrafficPathNode *VehicleMove::getTurnBendNode() { return m_turnBendNode.get(); }

unsigned int indicate;
EXVector lcInitialLanePos;
EXVector lcDestLanePos;
unsigned char VehicleMove::nextState = 0xff;
signed char VehicleMove::bNextStateNow = 1;
signed char VehicleMove::bNewState;
static signed char s_bKeepAmbientFlag;  // TODO name (bNewState+1)

signed char VehicleMove::areDestCrossedLanesClear(TrafficLaneDest *pDest)
{
    TrafficPathLink *pLink = pDest->getCrossedPathLink(0);
    u32 i = 0;
    if (!getNumLaneSegBlocks()) {
        float front = getVehicle()->getTypeObject()->getZDim() * 3.0f;
        float rear = getVehicle()->getTypeObject()->getZDim() * 1.5f;
        while (i < 4 && pLink) {
            if (pDest->checkVehicleProximity(front, rear)) return 0;
            pLink = pDest->getCrossedPathLink(++i);
        }
    } else {
        while (i < 4 && pLink) {
            if (pDest->checkVehicleProximity(3.0f, 3.0f)) return 0;
            pLink = pDest->getCrossedPathLink(++i);
        }
    }
    return 1;
}

void VehicleMove::initHeadToNewRoad(unsigned int type, EXVector3 &dest)
{
    const EXVector3 &pos = getVehicle()->getCurrPos().get();
    EXVector2 p(pos.x, pos.z);
    EXVector2 d(dest.x - p.x, dest.z - p.y);
    if (type != 2) {
        setFlagBit(1);
        return;
    }
    clrFlagBit(1);
    double a = atan2(d.x, d.y);
    setSlopeX((float)sin(a));
    setSlopeZ((float)cos(a));
    setLinkLengthXZ(EXSqrt(d.x * d.x + d.y * d.y));
}

void VehicleMove::addLaneSegBlock(TrafficLaneSeg *pSeg)
{
    for (u32 i = 0; i < 8; i++) {
        if (!m_laneSegBlocks[i]) {
            m_laneSegBlocks[i] = pSeg;
            m_numLaneSegBlocks++;
            return;
        }
    }
}

void VehicleMove::unblockLaneSegBlocks()
{
    if (m_numLaneSegBlocks) {
        for (u32 i = 0; i < 8; i++) {
            TrafficLaneSeg *pSeg = m_laneSegBlocks[i];
            if (pSeg) {
                pSeg->getLane()->delAIBlockedSeg(pSeg->getIndex());
                clrLaneSegBlock(i);
            }
        }
        m_numLaneSegBlocks = 0;
    }
}

// NEAR: 55 bytes differ, ours converts m_maxSpeed with x87 fild, retail with SSE cvtsi2ss.
float VehicleMove::calcSpeedForTurn(float angle)
{
    if (getVehicle()->getTypeObject()->getType() == 6) return m_maxSpeed * 0.75f;
    if (!(angle < 1.5707964f)) angle = 1.5707964f;
    return (1.0f - 1.5707964f / angle * 0.8f) * m_maxSpeed;
}

signed char VehicleMove::checkForValidLaneChange(TrafficLocation &, Vehicle *, signed char)
{
    return 0;
}

void VehicleMove::initRoute(signed char *pRoute)
{
    m_pRoute = pRoute;
    m_f320 = -1.0f;
    if (pRoute) {
        m_navMode = 1;
        signed char dest = *pRoute;
        if (dest < 0) {
            nextRouteStep();
        } else {
            m_pRoute = pRoute + 1;
            m_routeLaneEndDest = dest;
        }
    } else {
        if (!s_bKeepAmbientFlag) getVehicle()->m_flags |= 1;
        m_navMode = 0;
    }
}

void VehicleMove::setTurnDestLink(TrafficPathLink *pLink)
{
    if (!pLink) m_turnDestLink = 0xffff;
    else m_turnDestLink = pLink->getArrayIndex();
}

void VehicleMove::initTurn(float angle, TrafficPathLink *pLink, float radius)
{
    clrFlagBit(8);
    if (checkFlagBit(5)) clrFlagBit(0x80);
    setStartTurnAngle(m_phys.m_f4);
    angle -= m_phys.m_f4 - getRailsAngle();
    setTurnAngle(angle);
    setTurnDestLink(pLink);
    setTurnRadius(radius);
    m_turnHorzOffset = m_pVehicle->m_currPos.getHorzOffset(1);
    if (angle < 0.0f) {
        setFlagBit(0x10);
        m_phys.m_fc = -0.75f;
    } else {
        clrFlagBit(0x10);
        m_phys.m_fc = 0.75f;
    }
}

float dist3D_Point_to_LineSegment(EXVector &, EXLineSeg &);  // Traffic.obj
namespace ex { double fd(float f); }                         // XBApp.obj

// NEAR: 1071 vs 1056 bytes; retail copies node A and the vehicle position into EXVector locals first.
signed char VehicleMove::updateTurn()
{
    float wheelBase = m_pVehicle->getTypeObject()->getPhys_wheelBase();
    TrafficPathLink *pLink = getTurnDestLink();
    EXVector3 &a = pLink->getNodeAPtr()->getPos();
    EXVector3 &b = pLink->getNodeBPtr()->getPos();
    EXVector dir(b.x - a.x, b.y - a.y, b.z - a.z, 0.0f);
    dir.Normalise();
    dir.x *= 1000.0f;
    dir.z *= 1000.0f;
    EXLineSeg seg;
    seg.m_origin = EXVector(a.x, a.y, a.z, 0.0f) - dir * 0.5f;
    seg.m_dir = dir;
    const EXVector3 &pos = m_pVehicle->m_currPos.get();
    float best = 3.4028235e38f;
    float bestT = 0.0f;
    float bestSteer = 0.0f;
    float t;
    u32 i = 0;
    do {
        float r = wheelBase / (float)sin(m_phys.m_fc);
        float ang = m_phys.m_f4 + 1.5707964f;
        EXVector pt((float)sin(ang) * r + pos.x, pos.y, (float)cos(ang) * r + pos.z, 0.0f);
        float d = dist3D_Point_to_LineSegment(pt, seg);
        float lhs = d;
        float rhs = r;
        if (checkFlagBit(0x10)) {
            lhs = 0.0f - r;
            rhs = d;
        }
        t = 1.0f - 1.0f / (m_turnAngle / (m_phys.m_f4 - m_startTurnAngle));
        if (!(t > 0.01f)) {
            float end = m_startTurnAngle + m_turnAngle;
            clrFlagBit(8);
            m_phys.m_f4 = end;
            m_railsAngle = end;
            m_phys.m_fc = 0.0f;
            return 1;
        }
        if (!checkFlagBit(8) && t <= 0.3f) {
            setFlagBit(8);
            setStartFakeSteerAngle(m_phys.m_fc);
        }
        float steer = m_phys.m_fc;
        if (lhs > rhs) {
            if (rhs + 0.25f > lhs) goto done;
            m_phys.m_fc = steer - 0.0043633231f;
        } else if (rhs > lhs) {
            if (lhs > rhs - 0.25f) goto done;
            m_phys.m_fc += 0.0043633231f;
        }
        {
            float diff = (float)fabs(lhs - rhs);
            if (best > diff) {
                best = diff;
                bestT = t;
                bestSteer = steer;
            }
        }
    } while (i++ <= 128);
    ex::fd(bestSteer);
    m_phys.m_fc = bestSteer;
    t = bestT;
done:
    if (checkFlagBit(8)) {
        setFakeSteerAngle(0.0f);
        if (t > 0.0f) {
            float k = 1.0f - t * 6.6666665f;
            if (1.0f > k) setFakeSteerAngle(getStartFakeSteerAngle() - getStartFakeSteerAngle() * k);
        }
    }
    return 0;
}

signed char VehicleMove::checkIntersectionDestClear(TrafficIntersection *pInt, TrafficLaneDest *pDest)
{
    if (pInt->isAIExitBlocked()) return 0;
    if (!pDest) return 0;
    TrafficLight *pLight = pInt->getTrafficLight();
    TrafficBoxJunction *pBox = pInt->getBoxJunction();
    if (!pLight && pBox && pBox->getNumVehicles()) return 0;
    if (pDest->getDistFromLaneStartXZ() <= 1.0f) {
        Vehicle *pLast = pDest->getLane()->getLastVehicle();
        if (pLast) {
            float front = 0.0f;
            float rear = 0.0f;
            if (pLast->getCurrPos().getLaneDists(pDest->getLane(), front, rear)) {
                float len = getVehicle()->getTypeObject()->getZDim() + 1.0f;
                if (len > rear) {
                    if (pLast->getMove().getSpeed() <= 0.0f) return 0;
                    if (pLast->getMove().m_accel.isValidTarget() && len >= pLast->getMove().m_accel.getTargetDist() + rear) return 0;
                }
            }
        }
    }
    return 1;
}

static __forceinline float vmWrapAngle(float a)
{
    if (3.1415927f > (float)fabs(a)) return a;
    float r = (float)fmod(a, 6.2831855f);
    if (3.1415927f > (float)fabs(r)) return r;
    if (a >= 0.0f) return r - 6.2831855f;
    return r + 6.2831855f;
}

// NEAR: structural (1496 vs 1712 bytes); retail routes the vectors through extra EXVector copies.
signed char VehicleMove::initLaneChange(TrafficLocation &loc)
{
    Vehicle *pVehicle = m_pVehicle;
    VehiclePos &pos = pVehicle->m_currPos;
    TrafficPathLink *pLink = TrGetPathLink(loc.m_linkId);
    EXVector3 &na = pLink->getNodeAPtr()->getPos();
    EXVector3 &nb = pLink->getNodeBPtr()->getPos();
    TrafficExtraLinkData *pExtra = pLink->getExtraLinkData();
    EXVector a(na.x, na.y, na.z, 0.0f);
    EXVector b(nb.x, nb.y, nb.z, 0.0f);
    EXVector3 dest;
    loc.calcPos(dest);
    m_laneChange.m_destPos.x = dest.x;
    m_laneChange.m_destPos.y = dest.y;
    m_laneChange.m_destPos.z = dest.z;
    m_laneChange.m_destPos.w = 0.0f;
    m_laneChange.m_dir = b - a;
    m_laneChange.m_dir.Normalise();
    m_laneChange.m_bDirChange = pVehicle->m_currPos.m_loc.getLane()->getDir() != loc.getLane()->getDir();
    float startAngle = m_phys.m_f4;
    float linkAngle = pExtra->getAngle();
    if (m_laneChange.m_bDirChange) {
        linkAngle = vmWrapAngle(linkAngle + 3.1415927f);
        m_laneChange.m_dir = -m_laneChange.m_dir;
    }
    m_laneChange.m_angleOffset = AngleDiff(startAngle, linkAngle);
    const EXVector3 &p = m_pVehicle->m_currPos.get();
    m_laneChange.m_len = vmSub(m_laneChange.m_destPos, EXVector(p.x, p.y, p.z, 0.0f)).Length();
    pos.getLanePos(pos.getLoc().getLane(), pos.getLaneDist(0), lcInitialLanePos);
    TrafficLocation l = loc;
    l.m_horizontalOffset = 0.0f;
    EXVector3 dp;
    l.calcPos(dp);
    lcDestLanePos = EXVector(dp.x, dp.y, dp.z, 0.0f);
    m_laneChange.m_len = vmSub(lcDestLanePos, lcInitialLanePos).LengthXYZ();
    m_laneChange.m_startAngle = m_phys.m_f4;
    m_laneChange.m_startAngle = vmWrapAngle(m_laneChange.m_startAngle);
    m_laneChange.m_loc = loc;
    setFlagBit(0x88);
    m_phys.m_fc = 2.0e-7f;
    setFakeSteerAngle(0.0f);
    return 1;
}

void VehicleMove::emergencyFindReactors(TrafficLane *pLane, unsigned int idx)
{
    float myFront, myRear;
    float front, rear;
    m_pVehicle->m_currPos.getLaneDists(pLane, myFront, myRear);
    float range = getStoppingDist() * 2.5f;
    if (range < 10.0f) range = 10.0f;
    m_listCounts[idx] = 0;
    signed char side;
    Vehicle *p = m_pVehicle->m_currPos.getVehicleInFront(side, pLane, 4);
    if (!p) return;
    do {
        p->getCurrPos().getLaneDists(0, front, rear);
        if (rear - myFront > range) return;
        m_lists[idx].AddTail((EXDListItem *)&p->getMove().m_reaction);
        m_listCounts[idx]++;
        p = p->getCurrPos().getVehicleInFront(side, pLane, 4);
    } while (p != getVehicle() && p);
}

void VehicleAccel::reset(Vehicle *pVehicle)
{
    m_pVehicle = pVehicle;
    m_state = 0xff;
    m_nextState = 1;
    m_timer = 0.0f;
    clrTargets();
    m_rate = 1.0f;
    m_fa0 = 0.0f;
    m_fa4 = 0.0f;
    m_ub4 = 0;
    m_fb0 = 0.0f;
    for (u32 i = 0; i < 8; i++) m_combined[i] = 0;
}

TrafficLaneDest *VehicleMove::pickLaneEndDest()
{
    if (!m_navMode) return pickAmbientLaneEndDest();
    if (m_routeLaneEndDest == 0xff) return 0;
    return getVehicle()->getCurrPos().getLoc().getLane()->getEndDest(m_routeLaneEndDest);
}

VehicleReaction *VehicleMove::isReactee(Vehicle *pVehicle)
{
    for (u32 i = 0; i < 3; i++) {
        for (EXDListItem *p = m_lists[i].Head(); p; p = p->Next()) {
            if (((VehicleReaction *)p)->m_pVehicle == pVehicle) return (VehicleReaction *)p;
        }
    }
    return 0;
}

// NEAR: VC removes the first-batch stores that are re-stored after m_accel.reset(); retail keeps both
// batches (field values and offsets are right).
void VehicleMove::reset(Vehicle *pVehicle)
{
    m_navMode = 0;
    m_state = 0;
    m_substate = 0;
    m_toLink = 0;
    m_stoppingDist = 0;
    m_routeLaneEndDest = 0;
    m_maxSpeed = 0x48;
    m_flags = 0;
    m_pAtIntersection = 0;
    m_pPendingIntersection = 0;
    m_pFromIntersection = 0;
    m_pInBoxJunction = 0;
    m_pBlockingIntersection = 0;
    m_pLaneDest = 0;
    m_turnDestLink = 0xffff;
    m_speed = 0.0f;
    m_timer = 0.0f;
    m_slopeX = 0.0f;
    m_slopeZ = 0.0f;
    m_linkLengthXZ = 0.0f;
    m_modeTimer = 0.0f;
    m_substateTimer = 0.0f;
    m_turnAngle = 0.0f;
    m_turnRadius = 0.0f;
    m_turnHorzOffset = 0.0f;
    m_turnBendNode.m_node = 0xffff;
    m_fakeSteerAngle = 0.0f;
    m_startFakeSteerAngle = 0.0f;
    m_startTurnAngle = 0.0f;
    m_railsAngle = 0.0f;
    m_throttle = 0.5f;
    m_blockingLaneSeg = 0;
    m_numLaneSegBlocks = 0;
    m_throttle2 = 0.0f;
    m_pRouteLane = 0;
    m_pRoute = 0;
    m_f320 = 0.0f;
    m_u324 = 0;
    m_playerHitTime = 0.0f;
    m_pVehicle = pVehicle;
    m_accel.reset(pVehicle);
    m_phys.reset(pVehicle);
    setNavMode(0);
    setState(0);
    setFlags(0);
    setSpeed(0.0f);
    clrTimer();
    setSlopeX(0.0f);
    setSlopeZ(0.0f);
    setInBoxJunction(0);
    setBlockingIntersection(0);
    setBlockingLaneSeg(0xff);
    clrModeTimer();
    setMaxSpeed(0x48);
    setStoppingDist(0);
    setLaneDest(0);
    setPendingIntersection(0);
    setNumLaneSegBlocks(0);
    setThrottle(0.5f);
    for (u32 i = 0; i < 8; i++) clrLaneSegBlock(i);
    m_reaction.reset(pVehicle);
    m_u438 = 0;
    m_f320 = -1.0f;
    m_f430 = 0.0f;
}

void VehicleMove::updateRoute()
{
    if (m_navMode == 1) {
        TrafficLane *pLane = getVehicle()->getCurrPos().getLoc().getLane();
        if (m_pRouteLane != pLane) {
            if (checkFlagBit(0x800)) {
                clrFlagBit(0xc00);
                setThrottle(0.5f);
            }
            m_pRouteLane = pLane;
            nextRouteStep();
        }
    }
}

void VehicleMove::continueRoute()
{
    if (getState() == 9) m_pVehicle->initIndicator(1, (g_EXRandClass.Randf() + 1.0f) * 2.0f);
    if (getState() != 1) setNextState(1);
    m_f320 = -1.0f;
    m_pRouteLane = 0;
    updateRoute();
}

void VehicleAccel::combineTargets()
{
    clrCombinedTargetTypes();
    if (!isValidTarget()) return;
    unsigned int idx[6];
    unsigned int i;
    for (i = 0; i < m_numTargets; i++) idx[i] = i;
    for (i = 0; i < m_numTargets - 1u; i++) {
        VehicleAccelTarget *pMin = &m_targets[idx[i]];
        for (unsigned int j = i + 1; j < m_numTargets; j++) {
            VehicleAccelTarget t = m_targets[idx[j]];
            if (pMin->getDist() > t.getDist()) {
                unsigned int tmp = idx[i];
                idx[i] = idx[j];
                idx[j] = tmp;
                pMin = &t;
            }
        }
    }
    VehicleAccelTarget &first = m_targets[idx[0]];
    m_targetDist = first.getDist();
    m_targetSpeed = first.getSpeed();
    for (i = 0; i < m_numTargets; i++) {
        VehicleAccelTarget &t = m_targets[idx[i]];
        unsigned char type = t.getType();
        if (m_targetSpeed > t.getSpeed()) m_targetSpeed = t.getSpeed();
        if (!getCombinedTargetType(type)) setCombinedTargetType(type, &t);
    }
}

void VehicleAccel::clrTarget(unsigned int type)
{
    m_flags &= ~(1 << type);
    m_combined[type] = 0;
    for (unsigned char i = 0; i < m_numTargets; i++) {
        if (m_targets[i].getType() == type) {
            for (unsigned char j = i; j < m_numTargets - 1; j++) m_targets[j] = m_targets[j + 1];
            m_numTargets--;
            i--;
        }
    }
}

// NEAR: block layout differs (retail places the targetDist check before the accel block, decel after it).
void VehicleAccel::update()
{
    Vehicle *pVehicle = m_pVehicle;
    VehicleTypeBase *pType = pVehicle->getTypeObject();
    float newSpeed = pVehicle->getMove().getSpeed();
    float speed = newSpeed;
    VehicleMove &move = pVehicle->getMove();
    float maxSpeed = move.getMaxSpeed();
    combineTargets();
    m_pVehicle->clrFlag(4);
    if (isValidNextState()) {
        setState(getNextState());
        clrNextState();
    }
    switch (getState()) {
    case 0:
        newSpeed = 0.0f;
        clrTargets();
        break;
    case 1:
        if (isValidTarget()) {
            if (getTargetSpeed() > speed) {
                maxSpeed = getTargetSpeed();
            } else if (getTargetDist() > m_pVehicle->getMove().m_stoppingDist * 1.33f) {
            } else {
                goto decel;
            }
        }
        if (m_fa0 != 0.0f) {
            m_fa0 -= frameLen;
            if (m_fa0 <= 0.0f) m_fa0 = 0.0f;
        } else if (speed >= maxSpeed) {
            newSpeed = maxSpeed;
        } else {
            float t = move.getSpeed() * 0.27777779f;
            float mass = pType->getPhys_mass();
            if (move.m_phys.m_f140 != 0.0f) mass = move.m_phys.m_f140;
            newSpeed = ((-((fabs(t) * 7.0f + 35.0f) * t) + move.getThrottle() * 10000.0f) / mass * frameLen + t) * 3.6f;
            isValidFloat(newSpeed, 0);
        }
        break;
    decel:
        {
            float dv;
            if (0.1f >= getTargetDist()) {
                dv = 0.0f - speed;
            } else {
                float km = getTargetDist() * 0.001f;
                dv = (getTargetSpeed() * getTargetSpeed() - speed * speed) / (km * 2.0f) / framesPerHourDivRateMul;
            }
            if (speed > 20.0f && -0.3f > dv && 1.0f > getTargetSpeed()) m_pVehicle->m_sfx.playBrake();
            newSpeed = dv + speed;
            if (getTargetSpeed() > newSpeed) newSpeed = getTargetSpeed();
            isValidFloat(newSpeed, 0);
            if (newSpeed == 0.0f && speed != 0.0f) m_pVehicle->m_sfx.playStop();
            m_pVehicle->setFlag(4);
        }
        break;
    }
    isValidFloat(newSpeed, 0);
    move.setSpeed(newSpeed);
    move.setStoppingDist(kmh_to_stopping_dist(newSpeed));
    move.m_phys.update();
    float rate = 0.0f;
    if (speed != 0.0f) rate = newSpeed / speed;
    setRate(rate);
    if (m_pVehicle->getTypeObject()->getType() == 6) {
        float y = TrGetPathLink(m_pVehicle->m_currPos.m_loc.m_linkId)->getNodeAPtr()->getPos().y;
        if (1.0f > move.getSpeed() || fabs(y - m_pVehicle->m_currPos.m_pos.y) > 0.6f) {
            m_pVehicle->m_currPos.m_pos.y = sin(m_fb0 * 0.7f * 6.2831855f + 3.1415927f) * 0.1f + m_fac;
            m_fb0 += frameLen;
        } else {
            m_fac = m_pVehicle->m_currPos.m_pos.y;
            m_fb0 = 0.0f;
        }
    }
    move.m_phys.setRotations();
    if (newSpeed > 0.0f && 0.0f >= speed) m_pVehicle->m_sfx.playAway();
}

// NEAR: VC will not inline EXVector::operator- / Length here (retail inlines both).
void VehicleReaction::update()
{
    if (!getReactingTo() || getMode() == 7) return;
    Vehicle *pVehicle = m_pVehicle;
    unsigned short flags = pVehicle->getMove().getFlags();
    if (!(flags & 0x80)) {
        setMode(7);
        if (pVehicle->getMove().checkFlagBit(0x200)) return;
        pVehicle->getMove().setState(8);
        return;
    }
    const EXVector3 &pos = pVehicle->getCurrPos().get();
    EXVector p(pos.x, pos.y, pos.z, 0.0f);
    if (flags & 0x200) return;
    float len = (pVehicle->getMove().m_laneChange.m_destPos - p).Length();
    pVehicle->getMove().m_accel.addTargetReaction(len, 0.0f, pVehicle->getCurrPos().getFrontLaneDist(0) + len);
}

void VehicleReaction::initMode(unsigned char mode, float dist, Vehicle *pReactingTo, VehicleInLane *pInLane)
{
    setMode(mode);
    setReactingTo(pReactingTo);
    TrafficLocation loc;
    float horz;
    switch (mode) {
    case 1:
        horz = -0.9f;
        break;
    case 2:
        horz = 0.9f;
        break;
    case 5:
        horz = m_pVehicle->getCurrPos().getLoc().getLane()->getWidth() * -0.5f;
        break;
    case 6:
        horz = m_pVehicle->getCurrPos().getLoc().getLane()->getWidth() * 0.5f;
        break;
    case 3:
    case 4: {
        unsigned char link = pInLane->getLink()->m_index;
        unsigned char lane = pInLane->m_pLane->getIndex();
        unsigned char dir = pInLane->m_pLane->getDir();
        unsigned short road = m_pVehicle->getCurrPos().getLoc().getRoad()->getIndex();
        loc = TrafficLocation(road, dir, lane, link, pInLane->getFrontDistFromLinkStart() + dist, 0.0f);
        m_pVehicle->getMove().initLaneChange(loc);
        return;
    }
    default:
        return;
    }
    loc = m_pVehicle->getCurrPos().getLoc();
    loc.m_startLinkDist = m_pVehicle->getCurrPos().getInLane(0).getFrontDistFromLinkStart() + dist;
    loc.m_horizontalOffset = horz;
    m_pVehicle->getMove().initLaneChange(loc);
}

void VehicleMove::initHeadToEndDest(TrafficLaneDest *pDest)
{
    EXVector3 pos(pDest->m_pos.x, pDest->m_pos.y, pDest->m_pos.z);
    TrafficPathLink *pLink = TrGetPathLink(pDest->m_pathLink);
    TrafficExtraLinkData *pExtra = pLink->m_pExtraLinkData;
    pos.x += pExtra->getSlopeX() * 0.0f;
    pos.z += pExtra->getSlopeZ() * 0.0f;
    initHeadToNewRoad(pDest->getTurnDir(), pos);
    if (pDest->getTurnDir() != 2) {
        initTurn(pDest->getAngle(), pLink, -999.0f);
    } else {
        clrFlagBit(0xd);
        float rails = getRailsAngle();
        m_phys.m_f4 = rails;
        m_phys.m_fc = 0.0f;
        EXVector3 &rot = m_pVehicle->m_currRot;
        rot.x = 0.0f;
        rot.y = rails - 1.5707964f;
        rot.z = 0.0f;
    }
    m_loc0.setRoad(pDest->getRoad());
    m_loc0.setStartLinkDist(0.0f);
    m_loc0.setLane(pDest->getLane());
    m_loc0.m_linkIndex = TrGetPathLink(pDest->m_pathLink)->m_index;
    m_loc0.m_linkId = TrGetPathLink(m_loc0.getLane()->getPathLinkIndex() + m_loc0.getLinkIndex())->m_arrayIndex;
    setNextState(5);
    m_accel.setNextState(1);
    m_pVehicle->initIndicator(pDest->getTurnDir(), 0.0f);
}

// NEAR: VC places the fail block after the success path (retail: between them) and picks other registers.
signed char VehicleMove::doContinueChecks()
{
    TrafficLaneDest *pDest = m_continueChecks.getDest();
    if (!pDest) return 0;
    TrafficIntersection *pInt = m_continueChecks.getExitIntersection();
    TrafficBoxJunction *pBox = pInt->getBoxJunction();
    if ((!pInt->isAIExitBlocked() || pInt->getAIExitBlockRef() == m_pVehicle->getIndex()) &&
        (!pDest->getIntersection() || !pDest->getIntersection()->isAIEntryBlocked() ||
         pDest->getIntersection()->getAIEntryBlockRef() == m_pVehicle->getIndex()) &&
        checkIntersectionDestClear(pInt, pDest) && areDestCrossedLanesClear(pDest)) {
        if (!getNumLaneSegBlocks()) pDest->blockAICrossedLanes(m_pVehicle);
        if (getNumLaneSegBlocks()) {
            if (!(m_continueChecks.getFlags() & 1)) {
                m_continueChecks.setFlagBit(1);
                m_continueChecks.setTimerCmp(1.5f);
                m_continueChecks.setTimer(0.0f);
            }
            if (!m_continueChecks.incrTimer()) return 0;
        }
        setPendingIntersection(0);
        setFromIntersection(pInt);
        if (pBox) {
            pBox->addVehicle(m_pVehicle);
            setInBoxJunction(pBox);
        }
        initHeadToEndDest(pDest);
        return 1;
    }
    if (m_continueChecks.getFlags() & 1) {
        m_continueChecks.clrFlagBit(1);
        m_continueChecks.setTimerCmp(0.0f);
        return 0;
    }
    if (m_continueChecks.getTimerCmp() != 0.0f && m_continueChecks.incrTimer()) pDest->blockAICrossedLanes(m_pVehicle);
    return 0;
}

// NEAR: VC keeps _theTrafficSysAlloc in a register across the two getAtNode() calls (retail reloads).
signed char VehicleMove::updatePos()
{
    EXVector3 rot;
    EXVector3 pos;
    pos.y = m_pVehicle->m_currPos.m_pos.y;
    rot.x = m_pVehicle->m_currRot.x;
    rot.z = m_pVehicle->m_currRot.z;
    signed char ret = 0;
    if (checkFlagBit(1)) {
        ret = updateTurn();
        if (ret) m_phys.m_f4 = getStartTurnAngle() + getTurnAngle();
    } else if (getState() == 5) {
        EXVector fbc;
        m_pVehicle->m_currPos.getFrontBottomCentre(fbc);
        EXVector2 d(fbc.x - getAtNode()->getPos().x, fbc.z - getAtNode()->getPos().z);
        if (d.Length() >= getLinkLengthXZ()) ret = 1;
        goto setpos;
    } else if (checkFlagBit(0x80)) {
        ret = updateLaneChange();
        if (ret) clrFlagBit(0x80);
    } else {
        goto setpos;
    }
    rot.y = m_phys.m_f4 - 1.5707964f;
    {
        EXVector3 &r = m_pVehicle->m_currRot;
        r.y = rot.y;
        r.x = rot.x;
        r.z = rot.z;
    }
setpos:
    pos.x = m_phys.m_f1c;
    pos.z = m_phys.m_f20;
    m_pVehicle->m_currPos.set(pos);
    return ret;
}

TrafficLaneDest *VehicleMove::pickAmbientLaneEndDest()
{
    unsigned int order[6][3] = {{1, 0, 2}, {0, 1, 2}, {2, 1, 0}, {0, 2, 1}, {1, 2, 0}, {2, 0, 1}};
    float dist[3] = {-5000.0f, -5000.0f, -5000.0f};
    TrafficLaneDest *dests[3] = {0, 0, 0};
    unsigned int *pOrder = order[(int)(g_EXRandClass.Rand32() >> 21) % 6];
    for (u32 i = 0; i < 3; i++) {
        TrafficLaneDest *pDest = getVehicle()->getCurrPos().getLoc().getLane()->getEndDest(pOrder[i]);
        if (pDest && pDest->getRoad()) {
            if (pDest->isLeadingToDeadEnd()) {
                TrafficLane *pLane = pDest->getLane();
                if (!(TrGetPathLink(pLane->getIntersection(pLane->getNumIntersections() - 1)->m_pathLink)->m_flags & 0x80))
                    continue;
            }
            dist[i] = pDest->getLane()->getFreeTailDistXZ();
            dests[i] = pDest;
        }
    }
    float best = -4999.9995f;
    u32 bi = 0;
    if (dist[0] >= best) best = dist[0];
    if (dist[1] >= best) {
        best = dist[1];
        bi = 1;
    }
    if (dist[2] >= best) bi = 2;
    return dests[bi];
}

// NEAR: x87/SSE scheduling and stack slots differ slightly.
void VehiclePhys::update()
{
    float dt = frameLen;
    float v = m_pVehicle->getMove().getSpeed() * 1000.0f * 0.00027777778f;
    m_velocity.x = 0.0f;
    m_velocity.y = 0.0f;
    m_velocity.z = 0.0f;
    if ((float)fabs(m_fc) <= 1e-7f) {
        m_fc = 0.0f;
        m_f8 = 0.0f;
        if (!m_pVehicle->getMove().checkFlagBit(1)) m_f4 = m_pVehicle->getMove().getRailsAngle();
        float a = m_f4;
        float vz = cos(a) * v;
        float vx = sin(a) * v;
        m_f1c += dt * vx;
        m_f20 += vz * dt;
        m_velocity.y = 0.0f;
        m_velocity.x = vx;
        m_velocity.z = vz;
        return;
    }
    float radius = m_pVehicle->getTypeObject()->getPhys_wheelBase() / (float)sin(m_fc);
    float f = dt * 119.99999f;
    unsigned int n = (unsigned int)f;
    if (f - n > 0.05f) n++;
    dt = dt / n;
    m_f8 = v / radius;
    float vx, vz;
    for (u32 i = 0; i < n; i++) {
        float a = m_f4;
        vz = cos(a) * v;
        m_f4 = m_f8 * dt + a;
        vx = sin(a) * v;
        m_f1c += vx * dt;
        m_f20 += vz * dt;
    }
    m_velocity.x = vx;
    m_velocity.y = 0.0f;
    m_velocity.z = vz;
}

// NEAR: steer spill and a few register choices differ.
void VehiclePhys::setRotations()
{
    if (m_pVehicle->getTypeObject()->getType() == 6) {
        m_f14 *= 0.7f;
        float steer = m_pVehicle->getMove().checkFlagBit(8) ? m_pVehicle->getMove().getFakeSteerAngle() : m_pVehicle->getMove().m_phys.m_fc;
        if ((float)fabs(steer) < 1e-7f) {
            m_f18 = 0.0f;
            float spd = m_pVehicle->getMove().getSpeed();
            if (spd > 70.0f) spd = 70.0f;
            m_f14 = -0.31415927f / (70.0f / spd);
        } else {
            m_f18 = 3.1415927f / (0.98174775f / steer);
            float spd = m_pVehicle->getMove().getSpeed();
            if (70.0f > spd) m_f18 = m_f18 / (70.0f / spd);
            if (m_f18 > 0.7853982f) m_f18 = 0.7853982f;
            else if (-0.7853982f > m_f18) m_f18 = -0.7853982f;
        }
        m_dampen1.add(0.0f - m_f18);
        m_f18 = m_dampen1.get();
    } else {
        m_f10 = m_pVehicle->getMove().getSpeed() * frameLen / (m_pVehicle->getTypeObject()->getPhys_wheelLength() * 3.1415927f) + m_f10;
        m_f10 = fixAnglePositive(m_f10);
        signed char neg = 0;
        float s = m_f8;
        if (0.0f > s) {
            neg = 1;
            s = (float)fabs(s);
        }
        if (s > 2.0f) s = 2.0f;
        m_f18 = s * 0.049087387f;
        if (neg) m_f18 = 0.0f - m_f18;
        m_dampen1.add(m_f18);
        m_f18 = m_dampen1.get();
        m_f14 = 0.0f;
        float rate = m_pVehicle->getMove().m_accel.getRate();
        if (rate != 0.0f) {
            if (1.0f > rate) {
                float r = rate * 50.0f;
                if (r > 50.0f) r = 50.0f;
                else if (1.0f > r) r = 1.0f;
                float t = 49.0f - (r - 1.0f);
                if (t >= 3.0f) t = 3.0f;
                m_f14 = t * -0.015707964f;
            } else {
                float r = rate;
                if (r > 2.0f) r = 2.0f;
                r -= 1.0f;
                if (r > 0.5f) r = 0.5f;
                else if (0.0f > r) r = 0.0f;
                m_f14 = r * 0.7853982f;
                if (m_f14 > 0.065449847f) m_f14 = 0.065449847f;
            }
        }
    }
    m_dampen0.add(m_f14);
    m_f14 = m_dampen0.get();
}

// TODO stand-in: the EXGeoPathNode fields a route path uses (sizeof 0x28).
struct VmRouteNode {
    EXVector3 m_pos;    // 0x00
    u8 m_unkC[8];       // 0x0c
    s8 m_cmd;           // 0x14 route command
    u8 m_unk15;
    u8 m_arg0;          // 0x16
    u8 m_unk17;
    u8 m_arg1;          // 0x18
    u8 m_unk19;
    u8 m_arg2;          // 0x1a
    u8 m_unk1b;
    u32 m_flags;        // 0x1c bit 8: emit the turn direction
    u8 m_unk20[8];
};

static inline VmRouteNode *vmRouteNode(EXGeoPath *pPath, int i) { return (VmRouteNode *)pPath->Node(i); }

// NEAR: 1364 vs 1376 bytes; register allocation differs (retail keeps i in ebp and the turn in memory).
signed char VehicleMove::buildRouteFromPath(unsigned int id)
{
    EXGeoPath *pPath = EXItemEnv::GetItemEnv()->GetPath(id);
    if (pPath) {
    int n = 0;
    float prevAngle = 0.0f;
    for (int i = 0; i < pPath->NumNodes(); i++) {
        float angle = 0.0f;
        float turn = 0.0f;
        if (i != pPath->NumNodes() - 1) {
            VmRouteNode *a = vmRouteNode(pPath, i);
            VmRouteNode *b = vmRouteNode(pPath, i + 1);
            EXVector3 d(b->m_pos.x - a->m_pos.x, b->m_pos.y - a->m_pos.y, b->m_pos.z - a->m_pos.z);
            d.Normalise();
            angle = (float)atan2(d.x, d.z) * 57.29578f;
            turn = angle - prevAngle;
            if (turn >= 180.0f) turn = 0.0f - (360.0f - turn);
            else if (-180.0f >= turn) turn += 360.0f;
        }
        if (i) {
            float t = turn;
            if (id == 0xb000011 && i == 1) t = 90.0f;
            int dir;
            if (45.0f > (float)fabs(t)) dir = 2;
            else if (0.0f > turn) dir = 1;
            else dir = 0;
            s8 cmd = vmRouteNode(pPath, i)->m_cmd;
            if (cmd) {
                m_routeBuf[n++] = -cmd;
                switch (cmd) {
                case 1:
                case 2: {
                    TrafficLocation loc;
                    VmRouteNode *p = vmRouteNode(pPath, i);
                    EXVector pos(p->m_pos.x, p->m_pos.y, p->m_pos.z, 0.0f);
                    _theTrafficSysAlloc.getSys()->calcLocFromPosition(pos, loc, 0.0f);
                    float sd = loc.getStartLinkDist();
                    m_routeBuf[n++] = (int)(TrGetPathLink(loc.m_linkId)->getExtraLinkData()->getDistFromLaneStartXZ() + sd);
                    break;
                }
                case 4:
                case 7: {
                    TrafficLocation loc;
                    VmRouteNode *p = vmRouteNode(pPath, i);
                    EXVector pos(p->m_pos.x, p->m_pos.y, p->m_pos.z, 0.0f);
                    _theTrafficSysAlloc.getSys()->calcLocFromPosition(pos, loc, 0.0f);
                    float sd = loc.getStartLinkDist();
                    m_routeBuf[n++] = (int)(TrGetPathLink(loc.m_linkId)->getExtraLinkData()->getDistFromLaneStartXZ() + sd);
                    m_routeBuf[n++] = vmRouteNode(pPath, i)->m_arg0;
                    m_routeBuf[n++] = vmRouteNode(pPath, i)->m_arg1;
                    m_routeBuf[n++] = vmRouteNode(pPath, i)->m_arg2;
                    break;
                }
                case 6: {
                    TrafficLocation loc;
                    VmRouteNode *p = vmRouteNode(pPath, i);
                    EXVector pos(p->m_pos.x, p->m_pos.y, p->m_pos.z, 0.0f);
                    _theTrafficSysAlloc.getSys()->calcLocFromPosition(pos, loc, 0.0f);
                    float sd = loc.getStartLinkDist();
                    m_routeBuf[n++] = (int)(TrGetPathLink(loc.m_linkId)->getExtraLinkData()->getDistFromLaneStartXZ() + sd);
                    m_routeBuf[n++] = vmRouteNode(pPath, i)->m_arg0;
                    break;
                }
                case 3:
                case 5:
                    break;
                default:
                    n--;
                    break;
                }
            }
            if (vmRouteNode(pPath, i)->m_flags & 0x100) m_routeBuf[n++] = dir;
            prevAngle = angle;
        }
    }
    m_routeBuf[n] = (s8)0xbe;
    initRoute(&m_routeBuf[0]);
    return 1;
    }
    initRoute(0);
    return 0;
}

// NEAR: VC keeps a zero register (ebx) for the constant stores; retail uses immediates.
signed char VehicleMove::nextRouteStep()
{
    m_f320 = -1.0f;
    signed char c = *m_pRoute++;
    if (c < 0) {
    switch (c) {
    case -66:
        m_pRoute = 0;
        m_f320 = -1.0f;
        if (!s_bKeepAmbientFlag) getVehicle()->m_flags |= 1;
        m_navMode = 0;
        return 1;
    case -1: {
        unsigned int n = *m_pRoute++;
        m_f320 = n;
        m_routeArg = qabs(n);
        m_routeCmd = 0;
        break;
    }
    case -2: {
        unsigned int n = *m_pRoute++;
        m_f320 = n;
        m_routeArg = qabs(n);
        m_routeCmd = 10;
        break;
    }
    case -3:
        initRoute(m_pRoute = m_routeBuf);
        return 1;
    case -4:
    case -7: {
        unsigned int n = (unsigned char)*m_pRoute++;
        m_f320 = n;
        m_routeArg = qabs(n);
        m_routeCmd = 0;
        m_routeArgs[0] = *m_pRoute++;
        m_routeArgs[1] = *m_pRoute++;
        m_routeArgs[2] = *m_pRoute++;
        break;
    }
    case -6: {
        unsigned int n = (unsigned char)*m_pRoute++;
        m_f320 = n;
        m_routeArg = qabs(n);
        m_routeCmd = -6;
        m_routeArgs[0] = *m_pRoute++;
        break;
    }
    }
    setRouteLaneEndDest(0xff);
    return 0;
    }
    setRouteLaneEndDest(c);
    return 0;
}

// NEAR: esi/edi swapped and one stack slot differs.
void VehicleMove::emergencyVehicleBroadcast()
{
    VehiclePos &pos = m_pVehicle->m_currPos;
    for (int k = 0; k < 3; k++) {
        m_lists[k].Init();
        m_listCounts[k] = 0;
    }
    u32 i;
    emergencyFindReactors(0, 0);
    for (u32 side = 0; side < 2; side++) {
        for (u32 j = 0; j < pos.getNeighbourLaneCount(side); j++) emergencyFindReactors(pos.getNeighbourLane(side, j), side + 1);
    }
    signed char found = 0;
    for (i = 0; i < 3; i++) {
        for (VehicleReaction *r = (VehicleReaction *)m_lists[i].Head(); r; r = (VehicleReaction *)((EXDListItem *)r)->Next()) {
            float gap0, gap1;
            Vehicle *pV0, *pV1;
            Vehicle *pVehicle = r->m_pVehicle;
            if (r->getMode()) continue;
            if (i == 0) {
                pVehicle->m_currPos.getLaneGap(pVehicle->m_currPos.getLoc().getLane(), gap0, &pV0, gap1, &pV1);
                r->initMode(2, 10.0f, m_pVehicle, &pVehicle->m_currPos.getInLane(0));
            } else if (i == 1 || i == 2) {
                pVehicle->m_currPos.getLaneGap(pVehicle->m_currPos.getLoc().getLane(), gap0, &pV0, gap1, &pV1);
                r->initMode(1, 10.0f, m_pVehicle, &pVehicle->m_currPos.getInLane(0));
            } else {
                continue;
            }
            found = 1;
        }
    }
    if (m_reaction.getMode() == 7) m_reaction.reset();
    if (!m_reaction.getMode()) {
        m_reaction.reset();
        if (found) {
            signed char hasLeft = 0;
            if (pos.getNeighbourLaneCount(0)) hasLeft = 1;
            m_reaction.initMode(hasLeft ? 5 : 6, 20.0f, m_pVehicle, 0);
        }
    }
    if (fabs(pos.getHorzOffset(0)) > 0.15f) {
        float front = 0.0f;
        float rear = 0.0f;
        Vehicle *pFront, *pRear;
        pos.getLaneGap(pos.getLoc().getLane(), front, &pFront, rear, &pRear);
        if (front > 15.0f && rear > 5.0f && (!pFront || pFront->getMove().m_reaction.getReactingTo() != m_pVehicle)) {
            TrafficLocation loc = pos.getLoc();
            loc.m_startLinkDist = m_pVehicle->m_currPos.getInLane(0).getFrontDistFromLinkStart() + 12.0f;
            loc.m_horizontalOffset = 0.0f;
            initLaneChange(loc);
        }
    }
}

// NEAR: cos is spilled through a temp and the zero vector is stored in reverse order in retail.
void VehicleMove::ThrowGrenade()
{
    EXVector zero(0.0f, 0.0f, 0.0f, 0.0f);
    EXDatumTemp d;
    EXVector origin;
    if (m_pVehicle->GetItem()->GetAnimatorDatum(0x100000aa, d, 0, 0)) {
        origin.x = d.m_position.x;
        origin.y = d.m_position.y;
        origin.z = d.m_position.z;
        origin.w = d.m_position.w;
    } else {
        const EXVector3 &pos = m_pVehicle->getCurrPos().get();
        origin = EXVector(pos.x, pos.y, pos.z, 0.0f);
    }
    float a = m_pVehicle->m_currRot.y + 3.1415927f;
    EXVector dir(sin(a), 0.0f, cos(a), 0.0f);
    dir.y = g_EXRandClass.Randf() * 0.2f + 0.08f - 0.1f;
    dir.Normalise();
    XItemHandler_Bullet::CreateFromHash(origin, zero, dir, 0x49000025, 0, 0, 0, 1.0f);
}



// NEAR: structurally right; retail builds the vectors through extra EXVector copies (944 vs 733 bytes).
float VehicleMove::handlePlayerProximity()
{
    EXItem *pItem = m_pVehicle->GetItem();
    if (XItemHandler_Player::Instance()->GetStandOn() == pItem ||
        XItemHandler_Player::Instance()->JustOffCar() == m_pVehicle->GetItem()) {
        m_playerHitTime = 0.0f;
        m_u438 = 0;
        return -1.0f;
    }
    if (m_playerHitTime != 0.0f && frameTime >= m_playerHitTime && m_playerHitTime + 3.0f > frameTime) {
        signed char b = m_u438;
        m_u438 = 1;
        if (!b) return -1.0f;
        return 0.2f;
    }
    m_playerHitTime = 0.0f;
    m_u438 = 0;
    if (Player_IsCloaked()) return -1.0f;
    if (m_pVehicle->m_vehFlags & 4) return -1.0f;
    if (checkFlagBit(0x400)) return -1.0f;
    float ang = m_phys.m_f4;
    EXVector pp = gpPlayerItem[0]->m_position;
    VehicleTypeBase *pType = m_pVehicle->getTypeObject();
    float halfW = pType->getXDim() * 0.5f;
    float front = halfW * 0.25f + pType->m_zDistToFront;
    float s = sin(ang);
    float c = cos(ang);
    const EXVector3 &vp = m_pVehicle->getCurrPos().get();
    float fx = s * front + vp.x;
    float fz = c * front + vp.z;
    EXVector d = vmSub(EXVector(fx, vp.y, fz, 0.0f), pp);
    float range = m_stoppingDist * 1.75f + 1.0f;
    float dist = d.LengthXZ();
    if (dist > range) return -1.0f;
    halfW += 0.6f;
    EXVector start(fx, vp.y, fz, 0.0f);
    EXVector end(s * range + fx, vp.y, c * range + fz, 0.0f);
    EXLineSeg seg;
    seg.m_origin = start;
    seg.m_dir = vmSub(end, start);
    EXSphere sph;
    sph.Set(pp, halfW);
    if (!EXIntersection(seg, sph) || 0.0f > dist) return -1.0f;
    return dist + front;
}

signed char isPointInQuad2D(const EXVector2 &, const EXFixedArray<EXVector2, 4> &);  // Traffic.obj

// NEAR: structural (1424 vs 1696 bytes); retail evaluates sin/cos first and keeps more vector temporaries.
signed char VehicleMove::handleActorProximity(Actor **ppActor, float &dist)
{
    Vehicle *pVehicle = m_pVehicle;
    float halfW = pVehicle->getTypeObject()->getXDim() * 0.5f;
    float front = halfW * 0.25f + pVehicle->getTypeObject()->getZDistToFront();
    const EXVector3 &vp = pVehicle->getCurrPos().get();
    EXVector p(vp.x, vp.y, vp.z, 0.0f);
    EXFixedArray<EXMaths::vec_xyzw, 8> &verts = pVehicle->getCurrPos().getTransformedBoxVerts();
    EXFixedArray<EXVector2, 4> quad;
    quad[2].x = verts[4].x;
    quad[2].y = verts[4].z;
    quad[3].x = verts[0].x;
    quad[3].y = verts[0].z;
    quad[1].x = verts[1].x;
    quad[1].y = verts[1].z;
    quad[0].x = verts[5].x;
    quad[0].y = verts[5].z;
    float ang = pVehicle->m_currRot.y + 1.5707964f;
    float sf = (float)sin(m_phys.m_f4);
    float cf = (float)cos(m_phys.m_f4);
    float s = (float)sin(ang);
    float c = (float)cos(ang);
    quad[1].y = quad[1].y + c + s;
    quad[1].x = s - c + quad[1].x;
    quad[3].y = c - s + quad[3].y;
    quad[3].x = c + s + quad[3].x;
    quad[0].y = quad[0].y - c + s;
    quad[2].y = quad[2].y - (c + s);
    quad[2].x = quad[2].x - s + c;
    quad[0].x = quad[0].x - (c + s);
    float zFront = pVehicle->getTypeObject()->getZDistToFront();
    float zRear = pVehicle->getTypeObject()->getZDistToRear();
    float radius = vmMax(zRear, zFront) + 1.0f;
    Actor *a;
    for (a = Actor::GetFirst(p, radius, 0); a; a = a->GetNext(p, radius, 0)) {
        if (!a->IsCollisionPhysRadiusCheckValid()) continue;
        const EXVector &f = a->GetFeetPos();
        EXVector3 fp(f.x, f.y, f.z);
        if (fp.y >= verts[3].y) continue;
        float bottom = verts[0].y;
        if (bottom >= a->GetSkin()->m_Height * 1.8f + fp.y) continue;
        EXVector2 pt(fp.x, fp.z);
        if (isPointInQuad2D(pt, quad)) a->HitByVehicle(m_pVehicle, quad);
    }
    if (m_pVehicle->m_vehFlags & 4 || checkFlagBit(0x400)) {
        dist = -1.0f;
        return 0;
    }
    float range = m_stoppingDist * 1.75f + 1.0f;
    p.x = sf * front + p.x;
    p.z = cf * front + p.z;
    Actor *pBest = 0;
    a = Actor::GetFirst(p, range, 0);
    if (a) {
        halfW += 0.6f;
        do {
            EXLineSeg seg;
            seg.m_origin = p;
            seg.m_dir = vmSub(EXVector(sf * range + p.x, p.y, cf * range + p.z, p.w), p);
            EXSphere sph;
            sph.m_centre = a->GetFeetPos();
            sph.m_radius = halfW;
            if (EXIntersection(seg, sph)) {
                range = vmLengthXZ(vmSub(p, a->GetFeetPos()));
                pBest = a;
            }
            a = a->GetNext(p, range, 0);
        } while (a);
    }
    *ppActor = pBest;
    if (!pBest) {
        dist = -1.0f;
        return 0;
    }
    dist = range + front;
    return 1;
}

// NEAR: structural; retail inlines EXVector operator-/Length with extra copies (976 vs ~740 bytes).
signed char VehicleMove::updateLaneChange()
{
    VehiclePos &pos = m_pVehicle->m_currPos;
    EXVector3 p = pos.get();
    EXVector lanePos;
    pos.getLanePos(pos.getLoc().getLane(), pos.getLaneDist(0), lanePos);
    float total = vmSub(lcDestLanePos, lanePos).Length();
    EXVector d = vmSub(m_laneChange.m_destPos, EXVector(p.x, p.y, p.z, 0.0f));
    float diff = AngleDiff(m_laneChange.m_startAngle, (float)atan2(d.x * 1000.0f - d.z * 0.0f, d.z * 1000.0f + d.x * 0.0f));
    float off = vmSub(EXVector(p.x, p.y, p.z, 0.0f), lanePos).Length();
    float inv = 1.0f / m_laneChange.m_len;
    float t = (total - off) * inv;
    if (t >= 0.0f) t = inv * total;
    if (0.0f > t) {
        pos.setLoc(&m_laneChange.m_loc);
        clrFlagBit(0x88);
        m_phys.m_fc = 0.0f;
        return 1;
    }
    if (t >= 0.5f) {
        float u = (1.0f - t) * 2.0f;
        float k = (float)sin(1.5707964f * u) * u;
        m_laneChange.m_angle = InterpClamped(m_laneChange.m_startAngle, m_laneChange.m_startAngle + diff * 1.4f, k);
        m_phys.m_f4 = m_laneChange.m_angle;
        return 0;
    }
    if (0.4f >= t) m_phys.m_f4 = InterpClamped(m_laneChange.m_angle, m_laneChange.m_angleOffset + m_laneChange.m_startAngle, (0.4f - t) * 2.5f);
    return 0;
}

signed char STATE_MissionCollectionWaitForVan(StateMachine &);  // ActorMission.obj
void Mission_Fail(unsigned short);                              // Objective.obj
static const float s_minLookAhead = 3.0f;   // TODO name

// NEAR: structural (6344 vs 6464 bytes); register/stack-slot choices differ, retail inlines more vector copies.
void VehicleMove::update()
{
    VehiclePos &pos = m_pVehicle->m_currPos;
    TrafficLane *pLane = pos.m_loc.getLane();
    TrafficLocation &loc = pos.m_loc;
    TrafficPathLink *pLink = TrGetPathLink(loc.m_linkId);
    TrafficExtraLinkData *pExtra = pLink->getExtraLinkData();
    float linkLen = pExtra->getLengthXZ();
    TrafficPathNode *pNodeB = pLink->getNodeBPtr();
    float frontLaneDist = pos.getFrontLaneDist(0);
    float frontLinkDist = pos.getInLane(0).getFrontDistFromLinkStart();
    indicate = (unsigned int)-2;
    m_accel.clrTargets();
    signed char bProximity = 0;
    signed char bLaneEnd = 0;
    if (checkFlagBit(0x200)) emergencyVehicleBroadcast();
    m_reaction.update();
    clrNewState();
    Actor *pActor = 0;
    float actorDist;
    if (checkFlagBit(2)) {
        if (getTimer() > getModeTimer()) goto incrTimers;
        clrFlagBit(2);
        clrTimer();
        setNewState();
    }
    clrNextState();
    handleActorProximity(&pActor, actorDist);
    switch (getState()) {
    case 1:
    case 5: {
        if (!m_pVehicle->m_pExplosion) {
            signed char bHorn = 0;
            float d = handlePlayerProximity();
            if (d >= 0.0f) {
                d -= m_pVehicle->getTypeObject()->getZDistToFront() + 1.5f;
                if (0.0f > d) d = 0.0f;
                m_accel.addTargetPlayer(d, 0.0f, d + frontLaneDist);
                bProximity = 1;
            }
            if (pActor) {
                actorDist -= m_pVehicle->getTypeObject()->getZDistToFront() + 1.5f;
                if (0.0f > actorDist) actorDist = 0.0f;
                m_accel.addTargetActor(actorDist, 0.0f, actorDist + frontLaneDist);
                if (20.0f > getSpeed()) bHorn = 1;
                bProximity = 1;
                if (bHorn) {
                    m_pVehicle->m_sfx.playHorn();
                    goto hornDone;
                }
            }
            m_pVehicle->m_sfx.stopHorn(0);
        } else {
            float zero = 0.0f;
            float d = m_pVehicle->m_unk7d4 - frontLaneDist;
            float m = vmMax(zero, d);
            m_accel.addTargetPlayer(m, 0.0f, m + frontLaneDist);
        }
    hornDone:
        if (getNavMode() == 1 && m_f320 >= 0.0f) {
            float d = m_f320 - frontLaneDist;
            if (m_routeArg == 6) {
                if (d <= 0.0f) {
                    FlagChannels[m_routeArgs[0]] = 1;
                    continueRoute();
                }
            } else if (m_stoppingDist >= d) {
                if (0.0f > d) d = 0.0f;
                m_accel.addTargetLanestop(d, m_routeCmd, d + frontLaneDist);
                indicate = 0;
                if (1.0f > d && m_routeCmd + 1 > getSpeed()) {
                    if (m_routeArg != 2) {
                        setNextState(9);
                    } else {
                        EXVector p;
                        pos.get(p);
                        EXVector3 rot;
                        m_pVehicle->currRot_get(rot);
                        float a = rot.y + 3.1415927f;
                        EXVector dir(sin(a), 0.0f, cos(a), 0.0f);
                        Actor::SendMsg(MsgItem((MsgType)0x27), p, dir, 15.0f, 0.7853982f, 0, 0.0f);
                        Actor::SendMsg(MsgItem((MsgType)0xb), p, 15.0f, 0, 0.0f);
                        m_pVehicle->m_sfx.play(100, 0x1af000fe);
                        m_pVehicle->PlayMuzzleScript(0x4000179);
                        setFlagBit(0xc00);
                        setThrottle(1.5f);
                        continueRoute();
                    }
                }
            }
        }
        {
            signed char bSame = 0;
            Vehicle *pFront = pos.getVehicleInFront(bSame, loc.getLane(), 1);
            if (pFront) {
                float d = pos.getDistToVehicleInFront(0, 0, 0);
                if (m_pVehicle->getTypeObject()->getZDim() > d) d = pos.getLinesegDistFromFrontToOtherRear(pFront);
                float gap = d - 1.0f;
                if (0.0f > gap) gap = 0.0f;
                if (m_stoppingDist >= gap) {
                    if (0.0f > gap) gap = 0.0f;
                    m_accel.addTargetVehicle(gap, pFront->getMove().getSpeed() * 0.85f, gap + frontLaneDist);
                    if (0.1f > pFront->getMove().getSpeed()) m_accel.m_fa0 = g_EXRandClass.Randf() * 1.2f + 0.3f;
                }
            }
        }
        TrafficLocation unused;
        if (getState() != 5) {
            float look = m_stoppingDist * 1.5f;
            if (s_minLookAhead > look) look = s_minLookAhead;
            if (!(look + frontLinkDist >= linkLen)) break;
            signed char bTurn = loc.getLinkIndex() == getToLink();
            if (!(pLane->isLoop() && pNodeB->isDeadEnd() && pNodeB->isIntersection()) && !bTurn) {
            bTurn = 1;
            if (checkFlagBit(4)) {
                if (getTurnBendNode() == pNodeB) bTurn = 0;
                TrafficPathLink *pNext = pLink->getNextLink();
                if (pNext) {
                    TrafficPathNode *pNextB = pNext->getNodeBPtr();
                    if (!pNextB->isDeadEnd() && !pNextB->isIntersection()) {
                        float extra = 1.0f;
                        if (pNodeB->m_turnType == 5 && pNextB->m_turnType == 6) extra = 3.5f;
                        float endDist = pNext->getExtraLinkData()->getEndDistFromLaneStartXZ();
                        if (look + frontLaneDist > endDist + extra) {
                            float angle = pNext->getExtraLinkData()->getAngleToNextLink();
                            unsigned int side = 0;
                            if (!(3.1415927f > angle)) side = 1;
                            float off = m_pVehicle->getTypeObject()->getXDim() * 1.2f;
                            float speed = calcSpeedForTurn(fabs(angle));
                            unsigned char tt = pNodeB->getTurnType();
                            if (tt) {
                                float r = TrGetTurnRadius(tt) - m_pVehicle->getTypeObject()->getPhys_wheelBase() * 0.5f;
                                float h = pos.getHorzOffset(1);
                                if (side == 1) h = 0.0f - h;
                                off = h + r;
                                if (10.0f > speed) speed = 10.0f;
                            }
                            m_accel.addTargetBend(endDist - frontLaneDist + extra, speed,
                                                  pNext->getExtraLinkData()->getEndDistFromLaneStartXZ() + extra - off);
                        }
                    }
                }
            }
            if (bTurn) {
                float angle = TrGetPathLink(loc.m_linkId)->getExtraLinkData()->getAngleToNextLink();
                unsigned int side = 0;
                if (!(3.1415927f > angle)) side = 1;
                float off = m_pVehicle->getTypeObject()->getXDim() * 1.2f;
                float speed = calcSpeedForTurn(fabs(angle));
                float dist = linkLen - frontLinkDist;
                unsigned char tt = pNodeB->getTurnType();
                if (tt) {
                    float r = TrGetTurnRadius(tt) - m_pVehicle->getTypeObject()->getPhys_wheelBase() * 0.5f;
                    float h = pos.getHorzOffset(1);
                    if (side == 1) h = 0.0f - h;
                    off = h + r;
                    if (10.0f > speed) speed = 10.0f;
                }
                m_accel.addTargetBend(dist, speed, pExtra->getEndDistFromLaneStartXZ() - off);
                break;
            }
            {
                TrafficPathLink *pNext = pLink->getNextLink();
                if (!pNext) break;
                if (pNext->getNextLink()) break;
                TrafficPathNode *pEnd = pNext->getNodeBPtr();
                if (!pEnd->isDeadEnd()) break;
                if (!pEnd->isIntersection()) break;
                float end = pNext->getExtraLinkData()->getEndDistFromLaneStartXZ();
                if (!(look + frontLaneDist > end)) break;
                TrafficIntersection *pInt = _theTrafficSysAlloc.getSys()->getIntersection(pEnd->getIntersectionIndex());
                TrafficLaneDest *pDest = getLaneDest();
                float speed = 1.0f;
                if (!pDest) {
                    pDest = pickLaneEndDest();
                    setLaneDest(pDest);
                    getContinueChecks().setDest(pDest);
                    if (!pDest) goto noDest;
                }
                if (checkIntersectionDestClear(pInt, pDest)) {
                    speed = 500.0f;
                    if (pDest && pDest->getTurnDir() != 2) speed = calcSpeedForTurn(fabs(pDest->getAngle()));
                }
            noDest:
                end -= frontLaneDist;
                m_accel.addTargetLaneEnd(end, speed, pNext->getExtraLinkData()->getEndDistFromLaneStartXZ());
                if (pDest) indicate = pDest->getTurnDir();
                if (1.0f > end) bLaneEnd = 1;
                break;
            }
            } else {
                float speed = 1.0f;
                TrafficLaneDest *pDest = 0;
                if (pNodeB->isIntersection()) {
                    TrafficIntersection *pInt = _theTrafficSysAlloc.getSys()->getIntersection(pNodeB->getIntersectionIndex());
                    pDest = getLaneDest();
                    if (!pDest) {
                        pDest = pickLaneEndDest();
                        setLaneDest(pDest);
                        getContinueChecks().setDest(pDest);
                        if (!pDest) goto noDest2;
                    }
                    if (checkIntersectionDestClear(pInt, pDest)) {
                        speed = 500.0f;
                        if (pDest && pDest->getTurnDir() != 2) speed = calcSpeedForTurn(fabs(pDest->getAngle()));
                    }
                }
            noDest2:
                m_accel.addTargetLaneEnd(linkLen - frontLinkDist, speed, pExtra->getEndDistFromLaneStartXZ());
                if (pDest) indicate = pDest->getTurnDir();
                break;
            }
        } else {
            TrafficLane *pDestLane = m_pLaneDest->getLane();
            Vehicle *pLast = pDestLane->getLastVehicle();
            if (!pLast) break;
            float front = 0.0f;
            float rear = 0.0f;
            if (!pLast->m_currPos.getLaneDists(pDestLane, front, rear)) break;
            if (!(m_pVehicle->getTypeObject()->getZDim() + 1.0f > rear)) break;
            float d = pos.getLinesegDistFromFrontToOtherRear(pLast) - 1.0f;
            float gap = d;
            if (0.0f > gap) gap = 0.0f;
            if (!(m_stoppingDist >= gap)) break;
            if (0.0f > gap) gap = 0.0f;
            m_accel.addTargetVehicle(gap, pLast->getMove().getSpeed() * 0.85f, gap + frontLaneDist);
            if (0.1f > pLast->getMove().getSpeed()) m_accel.m_fa0 = g_EXRandClass.Randf() * 1.2f + 0.3f;
        }
        break;
    }
    case 2:
    case 3: {
        TrafficIntersection *pAt = getAtIntersection();
        if (pAt && TrGetPathLink(pAt->m_pathLink)->isUnspawnLink()) {
            unsigned char idx = m_pVehicle->getIndex();
            m_pVehicle->remove();
            m_pVehicle->setIndex(idx);
            return;
        }
        if (isNewState()) {
            TrafficLaneDest *pDest = getLaneDest();
            if (!pDest) {
                pDest = pickLaneEndDest();
                if (!pDest) goto noLaneEnd;
                setLaneDest(pDest);
                getContinueChecks().setDest(pDest);
            }
            getContinueChecks().set(pDest, getAtIntersection(), 1.0f);
            if (!m_accel.m_nextState) break;
        }
        if (doContinueChecks()) break;
        if (getNavMode() == 0) {
            TrafficLaneDest *pDest = pickAmbientLaneEndDest();
            if (!pDest) {
            noLaneEnd:
                setNextState(4);
                break;
            }
            setLaneDest(pDest);
            getContinueChecks().setDest(pDest);
        }
        m_accel.m_nextState = 0;
        break;
    }
    case 7:
        if (getModeTimer() != 0.0f) break;
    case 0:
        setToLink(loc.getLane()->getLastPathLink()->m_index);
        if (loc.getLane()->isLoop()) m_toLink++;
        setSlopeX(pExtra->getSlopeX());
        setSlopeZ(pExtra->getSlopeZ());
        setLinkLengthXZ(pExtra->getLengthXZ());
        if (getState() != 7) setNextState(1);
        break;
    case 8:
        if (pos.getDistBetweenCentres(m_reaction.getReactingTo()) > 12.0f) {
            if (frontLaneDist > m_reaction.getReactingTo()->m_currPos.getInLane(pLane, 0)->getRearLaneDist()) break;
            m_reaction.reset();
            setNextState(1);
            TrafficLocation l = loc;
            l.m_startLinkDist = m_pVehicle->m_currPos.getFrontDistFromLinkStart(0) + 15.0f;
            l.m_horizontalOffset = 0.0f;
            m_pVehicle->getMove().initLaneChange(l);
        }
        break;
    case 9: {
        setSpeed(0.0f);
        unsigned char arg = m_routeArg;
        if (arg != 4 && arg != 7) {
            XItemHandler_Objective *pObj = m_pVehicle->m_pObjective;
            if (!pObj || pObj->m_Hash != 0x46000002) break;
            float dist = 9999.0f;
            EXVector p;
            pos.get(p);
            Actor *pFound = 0;
            for (Actor *a = Actor::GetFirst(p, 20.0f, 0); a; a = a->GetNext(p, 20.0f, 0)) {
                if (a->m_SM.GetState() == STATE_MissionCollectionWaitForVan) {
                    dist = vmLengthXZ(vmSub(p, a->GetFeetPos()));
                    pFound = a;
                    if (!getSubstate()) {
                        a->SendMsg(MsgItem((MsgType)0x21, (int)p.x, (int)p.y, (int)p.z, 0), 0.0f);
                        setSubstate(1);
                    }
                    break;
                }
            }
            switch (getSubstate()) {
            case 0:
                if (getSubstateTimer() > 5.0f) setSubstate(2);
                break;
            case 1:
                if (2.0f > dist) {
                    Mission_Fail(0);
                    pFound->Delete();
                    setSubstate(4);
                    break;
                }
                if (getSubstateTimer() > 10.0f) {
                    if (dist > 8.0f || getSubstateTimer() > 20.0f) setSubstate(2);
                }
                break;
            case 2:
                if (pFound) pFound->SendMsg(MsgItem((MsgType)0x22), 0.0f);
                m_pVehicle->m_objectiveHash++;
                FlagChannels[m_pVehicle->m_objectiveHash] = 1;
                continueRoute();
                break;
            case 4:
                m_pVehicle->m_pObjective = 0;
                m_pRoute = 0;
                m_f320 = -1.0f;
                if (!s_bKeepAmbientFlag) m_pVehicle->setFlag(1);
                setNavMode(0);
                setNextState(1);
                break;
            }
        } else {
            switch (getSubstate()) {
            case 0:
                if (m_routeArgs[1]) FlagChannels[m_routeArgs[1]] = 1;
                if (m_routeArg == 7) ThrowGrenade();
                setSubstate(1);
            case 1:
                if (getSubstateTimer() >= m_routeArgs[0]) {
                    if (m_routeArgs[2]) FlagChannels[m_routeArgs[2]] = 1;
                    continueRoute();
                }
                break;
            }
        }
        break;
    }
    }
    if (!(m_pVehicle->m_vehFlags & 4)) {
        if (!checkFlagBit(0x400)) {
            if (bProximity && !(getSpeed() > 1.0f)) {
                m_f430 += frameLen;
                if (m_f430 > 5.0f) setFlagBit(0x400);
            } else {
                m_f430 = 0.0f;
            }
        } else if (m_f430 != 0.0f) {
            m_f430 -= frameLen;
            if (2.0f > m_f430) {
                clrFlagBit(0x400);
                m_f430 = 0.0f;
            }
        }
    }
    {
        signed char bMoved = 0;
        unsigned char st = getState();
        if (st != 0 && st != 7 && st != 8 && st != 9) {
            m_accel.update();
            bMoved = updatePos();
            frontLaneDist = pos.getFrontLaneDist(0);
            frontLinkDist = pos.getInLane(0).getFrontDistFromLinkStart();
        } else {
            m_phys.setRotations();
        }
        if (bMoved || bLaneEnd) {
            switch (getState()) {
            case 1:
                if (checkFlagBit(4)) {
                    clrFlagBit(5);
                    pos.setLocLink(TrGetPathLink(loc.m_linkId)->getNextLink()->m_index);
                    TrafficPathLink *l = TrGetPathLink(loc.m_linkId);
                    TrafficExtraLinkData *e = l->getExtraLinkData();
                    pNodeB = l->getNodeBPtr();
                    frontLaneDist = pos.getFrontLaneDist(0);
                    frontLinkDist = pos.getInLane(0).getFrontDistFromLinkStart();
                    setSlopeX(e->getSlopeX());
                    setSlopeZ(e->getSlopeZ());
                    setLinkLengthXZ(e->getLengthXZ());
                    if (!bLaneEnd) {
                        EXVector3 p;
                        pos.get(p);
                        pos.set(p);
                        float ang = e->getAngle();
                        m_phys.m_f4 = ang;
                        EXVector3 &r = m_pVehicle->m_currRot;
                        r.x = 0.0f;
                        r.y = ang - 1.5707964f;
                        r.z = 0.0f;
                    } else {
                        setRailsAngle(getStartTurnAngle() + getTurnAngle());
                    }
                }
                break;
            case 5: {
                pos.setLoc(&m_loc0);
                EXVector3 p, r;
                loc.calcPosAndRot(p, r);
                pos.get(p);
                pos.set(p);
                TrafficPathLink *l = TrGetPathLink(loc.m_linkId);
                TrafficExtraLinkData *e = l->getExtraLinkData();
                pNodeB = l->getNodeBPtr();
                frontLaneDist = pos.getFrontLaneDist(0);
                frontLinkDist = pos.getInLane(0).getFrontDistFromLinkStart();
                clrFlagBit(1);
                float ang = e->getAngle();
                m_phys.m_f4 = ang;
                r.y = ang - 1.5707964f;
                r.z = 0.0f;
                r.x = 0.0f;
                m_pVehicle->currRot_set(r);
                if (getInBoxJunction()) {
                    getInBoxJunction()->removeVehicle(m_pVehicle);
                    setInBoxJunction(0);
                }
                unblockLaneSegBlocks();
                setNextState(1);
                setToLink(loc.getLane()->getLastPathLink()->m_index);
                if (TrGetPathLink(loc.getLane()->getPathLinkIndex())->getNodeAPtr()->isLoop()) m_toLink++;
                setSlopeX(e->getSlopeX());
                setSlopeZ(e->getSlopeZ());
                setLinkLengthXZ(e->getLengthXZ());
                setLaneDest(0);
                setFromIntersection(0);
                m_loc0.reset();
                m_pVehicle->initIndicator((unsigned int)-1, 0.0f);
                break;
            }
            }
            m_accel.clrTargets();
        }
    }
    if (m_accel.isValidTarget()) {
        VehicleAccelTarget *pBend = m_accel.getTargetBend();
        if (pBend && frontLaneDist >= pBend->getFrontLaneDist()) {
            if (checkFlagBit(4)) {
                clrFlagBit(5);
                pos.setLocLink(TrGetPathLink(loc.m_linkId)->getNextLink()->m_index);
                TrafficPathLink *l = TrGetPathLink(loc.m_linkId);
                TrafficExtraLinkData *e = l->getExtraLinkData();
                pNodeB = l->getNodeBPtr();
                frontLaneDist = pos.getFrontLaneDist(0);
                frontLinkDist = pos.getInLane(0).getFrontDistFromLinkStart();
                setSlopeX(e->getSlopeX());
                setSlopeZ(e->getSlopeZ());
                setLinkLengthXZ(e->getLengthXZ());
                EXVector3 p;
                pos.get(p);
                pos.set(p);
                setRailsAngle(getStartTurnAngle() + getTurnAngle());
            }
            float ang = TrGetPathLink(loc.m_linkId)->getExtraLinkData()->getAngleToNextLink();
            unsigned int side = 0;
            if (!(3.1415927f > ang)) side = 1;
            TrafficPathLink *pNext = TrGetPathLink(loc.m_linkId)->getNextLink();
            TrafficExtraLinkData *ne = pNext->getExtraLinkData();
            EXVector3 &na = pNext->getNodeAPtr()->getPos();
            float k = m_pVehicle->getTypeObject()->getZDim() * s_minLookAhead;
            EXVector3 tgt;
            tgt.y = na.y;
            tgt.x = k * ne->getSlopeX() + na.x;
            tgt.z = ne->getSlopeZ() * k + na.z;
            initHeadToNewRoad(side, tgt);
            initTurn(ang, pNext, TrGetTurnRadius(pNext->getNodeAPtr()->getTurnType()));
            setTurnBendNode(pNext->getNodeAPtr());
            setFlagBit(4);
        }
        VehicleAccelTarget *pEnd = m_accel.getTargetLaneEnd();
        if (pEnd && frontLaneDist >= pEnd->getFrontLaneDist()) {
            TrafficPathNode *pNode = pNodeB;
            setAtNode(pNode);
            if (pNode->isIntersection()) {
                TrafficIntersection *pInt = _theTrafficSysAlloc.getSys()->getIntersection(pNode->getIntersectionIndex());
                setAtIntersection(pInt);
                if (!pInt->isAIExitBlocked()) {
                    setState(2);
                    clrModeTimer();
                    setNewState();
                    setNextState(2, 0.0f);
                } else {
                    setNextState(2, 2.0f);
                    m_accel.m_nextState = 0;
                }
            } else {
                setAtIntersection(0);
                setNextState(3, 2.0f);
                m_accel.m_nextState = 0;
            }
        }
    }
    if (indicate != (unsigned int)-2) m_pVehicle->initIndicator(indicate, 0.0f);
    if (getNextState() != 0xff) {
        setState(getNextState());
        clrModeTimer();
        if (isNextStateImmediate()) clrNewState();
        return;
    }
incrTimers:
    incrModeTimer();
    incrSubstateTimer();
}
