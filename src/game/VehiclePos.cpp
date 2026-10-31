#include "FEText.h"
#include "Vehicle.h"
#include "EXLine.h"
#include "EXBounds.h"
#include "EXLozenge.h"
#include "EXDistance.h"

float VehicleInLane::getFrontDistFromLinkStart()
{
    return getVehicle()->getTypeObject()->getZDistToFront() + m_linkDist;
}

float VehicleInLane::getRearDistFromLinkStart()
{
    return m_linkDist - getVehicle()->getTypeObject()->getZDistToRear();
}

void VehiclePos::setLocLane(TrafficLane *pLane, float)
{
    m_loc.setLane(pLane);
}

// NEAR: x87/SSE sum order (retail z+x+y) and subtraction order differ.
float VehiclePos::getDistBetweenCentres(Vehicle *pOther)
{
    const EXVector3 &o = pOther->getCurrPos().get();
    EXVector3 d(m_pos.x - o.x, m_pos.y - o.y, m_pos.z - o.z);
    return d.Length();
}

void VehicleInLane::setLink(TrafficPathLink *pLink)
{
    if (!pLink) m_link = 0xffff;
    else m_link = pLink->getArrayIndex();
}

signed char VehiclePos::getLaneDists(TrafficLane *pLane, float &front, float &rear)
{
    if (!pLane) pLane = m_loc.getLane();
    unsigned int i;
    for (i = 0; i < 3; i++) {
        if (m_inLanes[i].m_pLane == pLane) goto found;
    }
    return 0;
found:
    front = m_inLanes[i].getFrontLaneDist();
    rear = m_inLanes[i].getRearLaneDist();
    return 1;
}

float VehicleInLane::getFrontDistFromLinkEnd()
{
    return getLink()->getExtraLinkData()->getLengthXZ() - getFrontDistFromLinkStart();
}

float VehicleInLane::getRearDistFromLinkEnd()
{
    return getLink()->getExtraLinkData()->getLengthXZ() - getRearDistFromLinkStart();
}

void VehiclePos::setLoc(TrafficLocation *pLoc)
{
    m_unk8 = (u32)m_loc.getLane();
    m_loc = *pLoc;
    m_loc.setLane(pLoc->getLane());
}

VehicleInLane *VehiclePos::getInLane(TrafficLane *pLane, unsigned int *pIndex)
{
    unsigned int i;
    for (i = 0; i < 3; i++) {
        if (m_inLanes[i].m_pLane == pLane) goto found;
    }
    return 0;
found:
    if (pIndex) *pIndex = i - 1;
    return &m_inLanes[i];
}

float VehiclePos::getHorzOffset(signed char bReal)
{
    if (!bReal && getVehicle()->getMove().checkFlagBit(1)) return getVehicle()->getMove().getTurnHorzOffset();
    return getInLane(m_loc.getLane(), 0)->getHorzOffset();
}

float VehiclePos::getDistToNextNode(TrafficLane *pLane)
{
    if (!pLane) pLane = m_loc.getLane();
    VehicleInLane *pInLane = getInLane(pLane, 0);
    if (!pInLane) return 10000.0f;
    return pInLane->getFrontDistFromLinkEnd();
}

void VehiclePos::getLanePos(TrafficLane *pLane, float dist, EXVector &out)
{
    float before, after;
    int linkIdx = pLane->calcLinkFromDist(dist, before, after, 0.0f, 0.0f);
    TrafficPathLink *pLink = TrGetPathLink((unsigned short)((unsigned char)linkIdx + pLane->getPathLinkIndex()));
    TrafficLocation loc(pLane->getRoad()->getIndex(), pLane->getDir(), pLane->getIndex(), linkIdx,
                        dist - pLink->getExtraLinkData()->getDistFromLaneStartXZ(), 0.0f);
    EXVector3 pos;
    loc.calcPos(pos);
    out.x = pos.x;
    out.y = pos.y;
    out.z = pos.z;
    out.w = 0.0f;
}

// Re-files the vehicle in each lane it occupies (its three lane slots): out of the old lane list,
// into the new one at its front distance, noting whether its centre is really inside the lane.
void VehiclePos::updateListItems()
{
    for (int i = 0; i < 3; i++) {
        VehicleInLane &lane = m_inLanes[i];
        if (lane.m_listItem.isUsed()) {
            lane.m_listItem.Remove();
            lane.m_listItem.setUsed(0);
        }
        TrafficLane *pLane = lane.m_pLane;
        if (pLane) {
            float horz = lane.m_horzOffset;
            float xDim = m_pVehicle->getTypeObject()->m_xDim;
            TrafficPathLink *pFirst = TrGetPathLink(pLane->getPathLinkIndex());
            TrafficPathNode *pNode = pFirst->getNodeAPtr();
            lane.m_bReallyInLane = (horz < 0.0f ? 0.0f - horz : horz) - xDim * 0.5f < pNode->getWidth() * 0.5f;
            pLane->addVehicleNEW(&lane.m_listItem, lane.m_frontLaneDist);
            lane.m_listItem.setUsed(1);
        }
    }
}

// Distance between this vehicle's front edge (box verts 0-1) and the other's rear edge (verts 4-5).
float VehiclePos::getLinesegDistFromFrontToOtherRear(Vehicle *pOther)
{
    EXFixedArray<EXMaths::vec_xyzw, 8> &other = pOther->getCurrPos().getTransformedBoxVerts();
    EXVector b1 = (EXVector &)other[5];
    EXVector b0 = (EXVector &)other[4];
    EXVector a1 = (EXVector &)m_transformedBoxVerts[1];
    EXVector a0 = (EXVector &)m_transformedBoxVerts[0];
    return dist3D_LineSegment_to_LineSegment(a0, a1, b0, b1);
}

// Fills the lists of lanes beside this vehicle's lane: side 0 the higher lanes of the same
// direction then all the opposing ones, side 1 the lower lanes of the same direction (nearest first).
signed char VehiclePos::calcNeighbouringLanes()
{
    memset(&m_neighbourLanes, 0, sizeof(m_neighbourLanes));
    TrafficLane *pLane = m_loc.getLane();
    unsigned char dir = pLane->getDir();
    TrafficRoad *pRoad = m_loc.getRoad();
    m_neighbourLaneCount[1] = 0;
    m_neighbourLaneCount[0] = 0;
    unsigned char i = pLane->getIndex() + 1;
    while (i < pRoad->getNumLanes(dir)) {
        m_neighbourLanes[0][m_neighbourLaneCount[0]++] = TrGetLane(pRoad->m_lanes[dir][i]);
        i++;
    }
    i = pLane->getIndex() - 1;
    while (i != 0xff) {
        m_neighbourLanes[1][m_neighbourLaneCount[1]++] = TrGetLane(pRoad->m_lanes[dir][i]);
        i--;
    }
    unsigned int otherDir = dir == 0;
    unsigned char j = 0;
    while (j < pRoad->getNumLanes((unsigned char)otherDir)) {
        m_neighbourLanes[0][m_neighbourLaneCount[0]++] = TrGetLane(pRoad->m_lanes[otherDir][j]);
        j++;
    }
    return m_neighbourLaneCount[0] + m_neighbourLaneCount[1] != 0;
}

// Whether this vehicle and the other overlap sideways enough in the lane that the other does not
// block it. Flags: 1 compare widths, 2 this vehicle must be really in the lane, 4 the other must be.
signed char VehiclePos::doesVehiclePassOtherHorz(TrafficLane *pLane, Vehicle *pOther, unsigned int flags)
{
    VehicleInLane *pMine = getInLane(pLane, 0);
    VehicleInLane *pTheirs = pOther->getCurrPos().getInLane(pLane, 0);
    float mineHorz = pMine->m_horzOffset;
    float otherHorz = pTheirs->m_horzOffset;
    if (((flags & 2) && !pMine->m_bReallyInLane) || ((flags & 4) && !pTheirs->m_bReallyInLane)) return 1;
    if (flags & 1) {
        float otherHalf = pOther->getTypeObject()->m_xDim * 0.5f;
        if (mineHorz <= otherHorz) {
            if (m_pVehicle->getTypeObject()->m_xDim * 0.5f + mineHorz < otherHorz - otherHalf) return 1;
        } else if (otherHalf + otherHorz < mineHorz - m_pVehicle->getTypeObject()->m_xDim * 0.5f) {
            return 1;
        }
    }
    return 0;
}

// The vehicle next ahead in the lane (the previous list entry), wrapping round a looping lane.
// Vehicles that pass this one sideways (see doesVehiclePassOtherHorz) are skipped.
Vehicle *VehiclePos::getVehicleInFront(signed char &bWrapped, TrafficLane *pLane, unsigned int flags)
{
    VehicleListItem *pItem;
    if (!pLane) {
        pLane = m_loc.getLane();
        pItem = &m_inLanes[0].m_listItem;
    } else {
        pItem = &getInLane(pLane, 0)->m_listItem;
    }
    for (;;) {
        Vehicle *pV = pItem->Prev() ? pItem->Prev()->m_pVehicle : 0;
        bWrapped = 0;
        if (!pV) {
            if (TrGetPathLink(pLane->getPathLinkIndex())->getNodeAPtr()->isLoop()) {
                pV = ((VehicleListItem *)pItem->m_pList->m_pTail)->m_pVehicle;
                if (!pV) return 0;
                bWrapped = 1;
            }
        }
        if (pV == m_pVehicle) return 0;
        if (!pV || !flags || !doesVehiclePassOtherHorz(pLane, pV, flags)) return pV;
        pItem = &pV->getCurrPos().getInLane(pLane, 0)->m_listItem;
    }
}

// The vehicle next behind in the lane (the next list entry), wrapping round a looping lane.
Vehicle *VehiclePos::getVehicleBehind(signed char &bWrapped, TrafficLane *pLane, unsigned int flags)
{
    VehicleListItem *pItem;
    if (!pLane) {
        pLane = m_loc.getLane();
        pItem = &m_inLanes[0].m_listItem;
    } else {
        pItem = &getInLane(pLane, 0)->m_listItem;
    }
    for (;;) {
        Vehicle *pV = pItem->Next() ? pItem->Next()->m_pVehicle : 0;
        bWrapped = 0;
        if (!pV) {
            if (TrGetPathLink(pLane->getPathLinkIndex())->getNodeAPtr()->isLoop()) {
                if (TrGetPathLink(pLane->getPathLinkIndex())->getNodeAPtr()->isLoop()) {
                    pV = ((VehicleListItem *)pItem->m_pList->m_pHead)->m_pVehicle;
                    if (!pV) return 0;
                    bWrapped = 1;
                }
            }
        }
        if (pV == m_pVehicle) return 0;
        if (!pV || !flags || !doesVehiclePassOtherHorz(pLane, pV, flags)) return pV;
        pItem = &pV->getCurrPos().getInLane(pLane, 0)->m_listItem;
    }
}

// Gap from this vehicle's front to the one ahead, or 10000 when nobody is.
float VehiclePos::getDistToVehicleInFront(TrafficLane *pLane, Vehicle **ppOut, unsigned int flags)
{
    if (!pLane) pLane = m_loc.getLane();
    signed char bWrapped;
    Vehicle *pV = getVehicleInFront(bWrapped, pLane, flags);
    if (!pV) {
        if (ppOut) *ppOut = 0;
        return 10000.0f;
    }
    float mine = getInLane(pLane, 0)->m_frontLaneDist;
    float theirs = pV->getCurrPos().getInLane(pLane, 0)->m_rearLaneDist;
    if (ppOut) *ppOut = pV;
    return theirs - mine;
}

// Gap from the vehicle behind to this vehicle's rear, or 10000 when nobody is.
float VehiclePos::getDistToVehicleBehind(TrafficLane *pLane, Vehicle **ppOut, unsigned int flags)
{
    if (!pLane) pLane = m_loc.getLane();
    signed char bWrapped;
    Vehicle *pV = getVehicleBehind(bWrapped, pLane, flags);
    if (!pV) {
        if (ppOut) *ppOut = 0;
        return 10000.0f;
    }
    float mine = getInLane(pLane, 0)->m_rearLaneDist;
    float theirs = pV->getCurrPos().getInLane(pLane, 0)->m_frontLaneDist;
    if (ppOut) *ppOut = pV;
    return mine - theirs;
}

// Free space ahead (to the next node or the vehicle in front, whichever is nearer) and behind.
void VehiclePos::getLaneGap(TrafficLane *pLane, float &frontGap, Vehicle **ppFront, float &rearGap, Vehicle **ppBehind)
{
    if (m_pVehicle->getMove().checkFlagBit(4)) {
        frontGap = 0.0f;
        rearGap = 0.0f;
        if (ppFront) *ppFront = 0;
        if (ppBehind) *ppBehind = 0;
        return;
    }
    float toNode = getDistToNextNode(pLane);
    if (toNode < 0.0f) toNode = 0.0f;
    Vehicle *pV;
    float toVeh = getDistToVehicleInFront(pLane, &pV, 4);
    if (toVeh < 0.0f) toVeh = 0.0f;
    if (toVeh <= toNode) {
        frontGap = toVeh;
    } else {
        frontGap = toNode;
        pV = 0;
    }
    if (ppFront) *ppFront = pV;
    Vehicle *pB = 0;
    float behind = getDistToVehicleBehind(pLane, &pB, 4);
    if (behind < 0.0f) behind = 0.0f;
    rearGap = behind;
    if (ppBehind) *ppBehind = pB;
}

// The link of the lane closest to the given point.
TrafficPathLink *VehiclePos::nearestPathLink(EXVector3 &pos, TrafficLane *pLane)
{
    float best = 3.4028235e38f;
    EXVector pt(pos.x, pos.y, pos.z, 0.0f);
    TrafficPathLink *pBest = 0;
    unsigned char n = pLane->getNumPathLinks();
    if (!n) return 0;
    unsigned char i = 0;
    do {
        TrafficPathLink *pLink = pLane->getPathLink(i);
        TrafficPathNode *pA = pLink->getNodeAPtr();
        TrafficPathNode *pB = pLink->getNodeBPtr();
        float bx = pB->getPos().x, by = pB->getPos().y;
        float ax = pA->getPos().x, ay = pA->getPos().y;
        float dz = pB->getPos().z - pA->getPos().z;
        EXLineSeg seg;
        TrafficPathNode *pA2 = pLink->getNodeAPtr();
        seg.m_Origin = EXVector(pA2->getPos().x, pA2->getPos().y, pA2->getPos().z, 0.0f);
        seg.m_Direction = EXVector(bx - ax, by - ay, dz, 0.0f);
        float d = dist3D_Point_to_LineSegment(pt, seg);
        if (d < best) {
            best = d;
            pBest = pLink;
        }
        i++;
    } while (i < pLane->getNumPathLinks());
    return pBest;
}

// Works out where the vehicle sits in one of its lane slots: the link (the next one, the one the
// mover is on, the current one, or the nearest), its distance along the lane and sideways offset.
// pLane of -1 means "the next link of the current lane"; idx 999 means "from the mover's own link".
void VehiclePos::calcLaneLoc(TrafficLane *pLane, unsigned int idx)
{
    TrafficPathLink *pLink = TrGetPathLink(m_loc.m_linkId);
    EXVector3 p(m_pos.x, 0.0f, m_pos.z);
    if (pLane == (TrafficLane *)-1) {
        pLane = m_loc.getLane();
        pLink = pLink->getNextLink();
    } else if (idx == 999) {
        idx = 1;
        pLink = TrGetPathLink(m_pVehicle->getMove().m_loc0.m_linkId);
    } else if (idx == 0) {
        pLane = m_loc.getLane();
    } else {
        pLink = nearestPathLink(p, pLane);
    }
    TrafficPathNode *pNode = pLink->getNodeAPtr();
    TrafficExtraLinkData *pData = pLink->getExtraLinkData();
    float nx = pNode->getPos().x, nz = pNode->getPos().z;
    float sx = pData->getSlopeX(), sz = pData->getSlopeZ();
    EXLine road(EXVector(nx, 0.0f, nz, 0.0f), EXVector(sx, 0.0f, sz, 0.0f));
    EXVector2 off;
    float t, tLeft, tRight;
    pLink->getLinkHorizontalOffsetVec(100.0f, off);
    EXLine perp;
    perp.m_Origin = EXVector(p.x, 0.0f, p.z, 0.0f);
    perp.m_Direction = EXVector(off.x, 0.0f, off.y, 0.0f);
    float d0 = EXSqrDistance(road, perp, &tLeft, 0);
    pLink->getLinkHorizontalOffsetVec(-100.0f, off);
    perp.m_Origin = EXVector(p.x, 0.0f, p.z, 0.0f);
    perp.m_Direction = EXVector(off.x, 0.0f, off.y, 0.0f);
    float d1 = EXSqrDistance(road, perp, &tRight, 0);
    float horz;
    if (d1 <= d0) {
        float dx = (sx * tRight + nx) - p.x;
        float dz = (sz * tRight + nz) - p.z;
        float sq = dz * dz + dx * dx;
        t = tRight;
        horz = 0.0f;
        if (0.0f <= sq) horz = EXSqrt(sq);
    } else {
        float dx = (sx * tLeft + nx) - p.x;
        float dz = (sz * tLeft + nz) - p.z;
        float sq = dz * dz + dx * dx;
        t = tLeft;
        if (sq < 0.0f) sq = 0.0f;
        else sq = 0.0f - EXSqrt(sq);
        horz = sq;
    }
    VehicleTypeBase *pType = m_pVehicle->getTypeObject();
    float laneDist = pData->getDistFromLaneStartXZ() + t;
    float front = pType->getZDistToFront();
    float rear = pType->getZDistToRear();
    VehicleInLane &in = m_inLanes[idx];
    in.m_pLane = pLane;
    in.m_link = pLink->getArrayIndex();
    in.m_laneDist = laneDist;
    in.m_frontLaneDist = front + laneDist;
    in.m_rearLaneDist = laneDist - rear;
    in.m_horzOffset = horz;
    in.m_linkDist = t;
    if (idx == 0) {
        m_loc.setStartLinkDist(t);
        m_loc.setHorizontalOffset(horz);
    }
}

// Re-derives everything from the vehicle's position and heading: the world-space collision box and
// its corners, the lanes it occupies and where in each it sits. While turning (move flag 4) the
// second lane slot is the lane being turned into and the first slot's distances are blended with
// the turn arc.
void VehiclePos::recalc()
{
    VehicleTypeBase *pType = m_pVehicle->getTypeObject();
    float angle = m_pVehicle->currRot_get().y - 1.5707963f;
    EXVector3 pos = m_pos;
    float s, c;
    EXSinCos(angle, s, c);
    m_transformedBox.m_extent[0] = pType->m_xDim;
    m_transformedBox.m_extent[1] = pType->m_yDim;
    m_transformedBox.m_extent[2] = pType->m_zDim;
    m_transformedBox.m_axis[0] = EXVector(0.5f, 0.0f, 0.0f, 0.0f);
    m_transformedBox.m_axis[1] = EXVector(0.0f, 0.5f, 0.0f, 0.0f);
    m_transformedBox.m_axis[2] = EXVector(0.0f, 0.0f, 0.5f, 0.0f);
    m_transformedBox.m_centre.y = m_transformedBox.m_extent[1] * 0.5f;
    m_transformedBox.m_centre.z = (pType->m_zDistToFront - (pType->m_zDim - pType->m_zDistToFront)) * -0.5f;
    m_transformedBox.m_centre.x = 0.0f;
    m_transformedBox.m_centre.w = 0.0f;
    EXMatrix m;
    m.r[0].x = c;    m.r[0].y = 0.0f; m.r[0].z = -s;   m.r[0].w = 0.0f;
    m.r[1].x = 0.0f; m.r[1].y = 1.0f; m.r[1].z = 0.0f; m.r[1].w = 0.0f;
    m.r[2].x = s;    m.r[2].y = 0.0f; m.r[2].z = c;    m.r[2].w = 0.0f;
    m.r[3].x = pos.x; m.r[3].y = pos.y; m.r[3].z = pos.z; m.r[3].w = 1.0f;
    m_transformedBox.ApplyXForm(m);
    m_transformedBox.ComputeVertices(m_transformedBoxVerts);
    removeInLanes();
    calcNeighbouringLanes();
    resetInLaneLanes();
    calcLaneLoc(0, 0);
    Vehicle *pVeh = m_pVehicle;
    unsigned short flags = pVeh->getMove().m_flags;
    bool bBlend = false;
    float rear;
    if (!(flags & 4)) {
        if (pVeh->getMove().m_state == 5 && (flags & 1)) bBlend = true;
        else goto neighbours;
    } else {
        float r = pVeh->getMove().m_turnRadius;
        if (r == 0.0f) goto neighbours;
        if (r == -999.0f) {
            bBlend = true;
        } else {
            calcLaneLoc((TrafficLane *)-1, 1);
            float d0 = m_inLanes[0].m_laneDist;
            float d1 = m_inLanes[1].m_laneDist;
            float end = TrGetPathLink(m_inLanes[0].m_link)->getExtraLinkData()->getEndDistFromLaneStartXZ();
            float start = end - r;
            float d = (d0 - start) + (d1 - end) + start;
            m_inLanes[0].m_laneDist = d;
            m_inLanes[0].m_frontLaneDist = m_pVehicle->getTypeObject()->getZDistToFront() + d;
            rear = d - m_pVehicle->getTypeObject()->getZDistToFront();
        }
    }
    if (bBlend) {
        calcLaneLoc(pVeh->getMove().m_loc0.getLane(), 999);
        TrafficPathNode *pB = TrGetPathLink(m_inLanes[0].m_link)->getNodeBPtr();
        float bx = pB->getPos().x, bz = pB->getPos().z;
        TrafficPathNode *pA = TrGetPathLink(m_inLanes[1].m_link)->getNodeAPtr();
        float dx = pA->getPos().x - bx, dz = pA->getPos().z - bz;
        float sq = dx * dx + dz * dz;
        float len;
        if (sq < 0.0f) len = 0.0f;
        else len = EXSqrt(sq);
        float d0 = m_inLanes[0].m_laneDist;
        float d1 = m_inLanes[1].m_laneDist;
        TrafficExtraLinkData *pData = TrGetPathLink(m_inLanes[0].m_link)->getExtraLinkData();
        float end = pData->getEndDistFromLaneStartXZ();
        float d = len * 0.70710678f + d1 + (d0 - end) + end;
        m_inLanes[0].m_laneDist = d;
        m_inLanes[0].m_frontLaneDist = m_pVehicle->getTypeObject()->getZDistToFront() + d;
        rear = d - m_pVehicle->getTypeObject()->getZDistToFront();
    }
    m_inLanes[0].m_rearLaneDist = rear;
    m_inLanes[0].m_horzOffset = m_pVehicle->getMove().m_turnHorzOffset;
    m_inLanes[1].m_pLane = 0;
neighbours:
    for (int i = 0; i < 2; i++) {
        if (m_neighbourLaneCount[i]) {
            TrafficLane *pLane = m_neighbourLanes[i][0];
            if (pLane->getDir() == m_loc.getLane()->getDir()) {
                calcLaneLoc(pLane, 1);
                m_neighbourLaneCount[i] = 1;
            } else {
                m_neighbourLaneCount[i] = 0;
            }
        }
    }
}

// Moves the box into the transform's space: the centre as a point, the axes as vectors.
void MHBox::ApplyXForm(const EXMatrix &m)
{
    m_centre.w = 1.0f;
    EXMaths::vec_xyzw out;
    D3DXVec4Transform(&out, &m_centre, &m);
    m_centre.x = out.x;
    m_centre.y = out.y;
    m_centre.z = out.z;
    m_centre.w = out.w;
    m_centre.w = 0.0f;
    for (int i = 0; i < 3; i++) {
        D3DXVec4Transform(&out, &m_axis[i], &m);
        m_axis[i].x = out.x;
        m_axis[i].y = out.y;
        m_axis[i].z = out.z;
        m_axis[i].w = out.w;
    }
}

// Axis-aligned bounds of the eight corners.
void MHBox::GetBoundsBox(EXBoundsBox &box) const
{
    EXFixedArray<EXMaths::vec_xyzw, 8> v;
    ComputeVertices(v);
    EXBoundsBox t;
    t.m_vctMin.x = v[0].x;
    t.m_vctMax.x = v[0].x;
    t.m_vctMax.y = v[0].y;
    t.m_vctMax.z = v[0].z;
    t.m_vctMax.w = v[0].w;
    t.m_vctMin.y = v[0].y;
    t.m_vctMin.z = v[0].z;
    t.m_vctMin.w = v[0].w;
    for (int i = 1; i < 8; i++) {
        if (v[i].w <= t.m_vctMin.w) t.m_vctMin.w = v[i].w;
        if (v[i].z <= t.m_vctMin.z) t.m_vctMin.z = v[i].z;
        if (v[i].y <= t.m_vctMin.y) t.m_vctMin.y = v[i].y;
        if (v[i].x <= t.m_vctMin.x) t.m_vctMin.x = v[i].x;
        if (t.m_vctMax.w <= v[i].w) t.m_vctMax.w = v[i].w;
        if (t.m_vctMax.z <= v[i].z) t.m_vctMax.z = v[i].z;
        if (t.m_vctMax.y <= v[i].y) t.m_vctMax.y = v[i].y;
        if (t.m_vctMax.x <= v[i].x) t.m_vctMax.x = v[i].x;
    }
    box.m_vctMin.x = t.m_vctMin.x;
    box.m_vctMax.x = t.m_vctMax.x;
    box.m_vctMin.y = t.m_vctMin.y;
    box.m_vctMax.y = t.m_vctMax.y;
    box.m_vctMax.z = t.m_vctMax.z;
    box.m_vctMin.z = t.m_vctMin.z;
    box.m_vctMax.w = t.m_vctMax.w;
    box.m_vctMin.w = t.m_vctMin.w;
}

// The eight corners of the box: centre plus or minus each scaled axis, bottom face first.
void MHBox::ComputeVertices(EXFixedArray<EXMaths::vec_xyzw, 8> &v) const
{
    EXVector a0 = m_axis[0] * m_extent[0];
    EXVector a1 = m_axis[1] * m_extent[1];
    EXVector a2 = m_axis[2] * m_extent[2];
    v[0] = ((m_centre - a0) - a1) - a2;
    v[1] = ((a0 + m_centre) - a1) - a2;
    v[2] = (a0 + m_centre + a1) - a2;
    v[3] = ((m_centre - a0) + a1) - a2;
    v[4] = ((m_centre - a0) - a1) + a2;
    v[5] = ((a0 + m_centre) - a1) + a2;
    v[6] = a0 + m_centre + a1 + a2;
    v[7] = ((m_centre - a0) + a1) + a2;
}

// Builds the box from a lozenge (a rectangle swept by a radius): the two edge directions and
// their cross product give the axes, the extents are half the edge lengths plus the radius.
void MHBox::Set(const EXLozenge &l)
{
    EXVector o = l.m_rect.m_origin;
    EXVector e0 = l.m_rect.m_edge0;
    EXVector e1 = l.m_rect.m_edge1;
    float r = l.m_radius;
    m_axis[0] = e0.UnitVector();
    m_axis[1] = e1.UnitVector();
    m_axis[2] = m_axis[0].Cross(m_axis[1]).UnitVector();
    EXVector a0r = m_axis[0] * r;
    EXVector a2r = m_axis[2] * r;
    EXVector a1r = m_axis[1] * r;
    EXVector p = o + a2r;
    m_centre = ((((o + e0) + e1) - a2r) - p) * 0.5f + p;
    m_extent[0] = ((o - a0r) - (a0r + e0 + o)).LengthXYZ() * 0.5f;
    m_extent[1] = ((o - a1r) - (a1r + e1 + o)).LengthXYZ() * 0.5f;
    m_extent[2] = r;
}
