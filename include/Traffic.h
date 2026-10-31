#ifndef TRAFFIC_H
#define TRAFFIC_H
#include "types.h"
#include "EXArray.h"
#include "EXList.h"
#include "EXVector.h"
#include "EXGeoPathNode.h"
#include "EXGeoMap.h"

class Vehicle;
class TrafficSystem;
class TrafficLane;
class TrafficRoad;
class TrafficPath;
class TrafficPathLink;
class TrafficPathNode;
class TrafficIntersection;
class TrafficBoxJunction;
class TrafficLight;
class XItemEnv;
class VehicleListItem;

// ---- Traffic system (Traffic.obj). Layouts recovered from the accessors; TODO names. ----

TrafficPathLink *TrGetPathLink(unsigned short id);
class EXLineSeg;
float dist3D_Point_to_LineSegment(EXVector &p, EXLineSeg &seg);
float dist3D_LineSegment_to_LineSegment(const EXVector &a0, const EXVector &a1, const EXVector &b0, const EXVector &b1);
TrafficLane *TrGetLane(unsigned short id);
float TrGetTurnRadius(unsigned char type);

// A position on the road network. sizeof 0x18.
class TrafficLocation {
public:
    TrafficLocation() { reset(); }
    TrafficLocation(unsigned int, unsigned int, unsigned int, unsigned int, float, float);
    ~TrafficLocation() {}
    void reset()
    {
        m_pLane = 0;
        m_flag = 0;
        m_horizontalOffset = 0.0f;
        m_linkId = 0xffff;
    }
    TrafficLane *getLane() const { return m_pLane; }
    float getStartLinkDist() const { return m_startLinkDist; }
    TrafficPath *getPath() const;
    TrafficPathLink *getPathLink() const;
    void setLink(unsigned char i);
    void calcPos(EXVector3 &);
    void calcPos(EXVector &);
    void calcPosAndRot(EXVector3 &, EXVector3 &);
    void setStartLinkDist(float f) { m_startLinkDist = f; }
    TrafficRoad *getRoad() const { return m_pRoad; }
    unsigned char getLinkIndex() const { return m_linkIndex; }
    void setRoad(TrafficRoad *p) { m_pRoad = p; }
    void setLane(TrafficLane *p) { m_pLane = p; }
    void setHorizontalOffset(float f) { m_horizontalOffset = f; }

    u8 m_linkIndex;             // 0x00 link within the lane
    u16 m_linkId;               // 0x02 global path link id (0xffff none)
    TrafficRoad *m_pRoad;       // 0x04
    TrafficLane *m_pLane;       // 0x08
    float m_startLinkDist;      // 0x0c
    float m_horizontalOffset;   // 0x10
    u8 m_flag;                  // 0x14 TODO name
};

// Per-link precomputed data. sizeof 0x20.
class TrafficExtraLinkData {
public:
    float getSlopeX() const { return m_slopeX; }
    float getSlopeZ() const { return m_slopeZ; }
    float getLengthXZ() const { return m_lengthXZ; }
    float getDistFromLaneStartXZ() const { return m_distFromLaneStartXZ; }
    float getEndDistFromLaneStartXZ() const { return m_distFromLaneStartXZ + m_lengthXZ; }
    float getAngle() const { return m_angle; }
    EXVector2 &getLeftPerpUnitVec() { return m_leftPerpUnitVec; }
    void setSlopeX(float f) { m_slopeX = f; }
    void setSlopeZ(float f) { m_slopeZ = f; }
    void setLengthXZ(float f) { m_lengthXZ = f; }
    float getAngleToNextLink() const { return m_angleToNextLink; }
    void setAngleToNextLink(float f) { m_angleToNextLink = f; }
    void setDistFromLaneStartXZ(float f) { m_distFromLaneStartXZ = f; }
    void setAngle(float f) { m_angle = f; }
    float m_slopeX;                 // 0x00
    float m_slopeZ;                 // 0x04
    float m_lengthXZ;               // 0x08
    float m_angle;                  // 0x0c
    float m_angleToNextLink;        // 0x10
    float m_distFromLaneStartXZ;    // 0x14
    EXVector2 m_leftPerpUnitVec;    // 0x18
};

// A link of the traffic path graph. sizeof 0x1c.
class TrafficPathLink : public EXGeoPathLink {
public:
    unsigned short getNodeA() const { return m_nodeA; }
    unsigned short getNodeB() const { return m_nodeB; }
    unsigned short getArrayIndex() const { return m_arrayIndex; }
    TrafficExtraLinkData *getExtraLinkData() const { return m_pExtraLinkData; }
    unsigned short getRoadIndex() const { return m_roadIndex; }
    unsigned short getLaneIndex() const { return m_laneIndex; }
    unsigned char getDir() const { return m_dir; }
    unsigned char getIndex() const { return m_index; }
    void setExtraLinkData(TrafficExtraLinkData *p) { m_pExtraLinkData = p; }
    void setRoadIndex(unsigned short i) { m_roadLaneIndex = (m_roadLaneIndex & 0xffff0000) | i; }
    void setLaneIndex(unsigned short i) { m_roadLaneIndex = (i << 16) | (m_roadLaneIndex & 0xffff); }
    void setDir(unsigned char d) { m_dirIndex = (d << 24) | (m_dirIndex & 0xffffff); }
    void setIndex(unsigned char i) { m_dirIndex = (i << 16) | (m_dirIndex & 0xff00ffff); }
    void setGeoIndex(unsigned short i) { m_geoArrayIndex = (m_geoArrayIndex & 0xffff0000) | i; }
    void setArrayIndex(unsigned short i) { m_geoArrayIndex = (m_geoArrayIndex & 0xffff) | (i << 16); }
    signed char isRemoved() const
    {
        if (isRemovedNotTriggerLink() || isIntersectionTriggerToIntersectionLink()) return 1;
        return 0;
    }
    void clrFlags(unsigned int f) { m_flags &= ~f; }
    void remove() { m_nodeA = 0x9999; }
    signed char isRemovedNotTriggerLink() const { return m_nodeA == 0x9999; }
    void setAsRoot() { m_flags |= 1; }
    void setAsLastLink() { m_flags |= 2; }
    void setAsSouthType() { m_flags |= 0x8000; }
    void setAsIntersectionType() { m_flags |= 0x80000000; }
    unsigned int isRoot() const { return m_flags & 1; }
    unsigned int isSouthType() const { return m_flags & 0x8000; }
    unsigned int isUnspawnLink() const { return m_flags & 0x80; }
    unsigned int isIntersectionLaneLink() const { return m_flags & 0x10000; }
    unsigned int isIntersectionLaneASplit() const { return m_flags & 0x20000; }
    unsigned int isIntersectionLaneBSplit() const { return m_flags & 0x40000; }
    unsigned int isIntersectionLaneNewALink() const { return m_flags & 0x80000; }
    unsigned int isIntersectionLaneNewBLink() const { return m_flags & 0x100000; }
    unsigned int isIntersectionTriggerToIntersectionLink() const { return m_flags & 0x200000; }
    unsigned int isIntersectionType() const { return m_flags & 0x80000000; }
    void swapNodes()
    {
        u16 a = m_nodeA;
        m_nodeA = m_nodeB;
        m_nodeB = a;
    }
    TrafficRoad *getRoad() const;
    TrafficLane *getLane() const;
    TrafficPath *getPath() const;
    TrafficPathNode *getNodeAPtr() const;
    TrafficPathNode *getNodeBPtr() const;
    float getDistFromLinkStartXZ(const EXVector3 *, signed char);
    float getDistFromLaneStartXZ(const EXVector3 *, unsigned int, signed char);
    TrafficPathLink *getNextLink() const;
    void getLinkHorizontalOffsetVec(float, EXVector2 &);
    void getPointOnLink(EXVector &, float, float);
    TrafficPathLink *getPrevLink() const;
    signed char doLinksIntersect(TrafficPathLink *, EXVector *, TrafficPathNode **, float);
    u16 m_nodeA;                            // 0x00
    u16 m_nodeB;                            // 0x02
    u32 m_flags;                            // 0x04
    u8 m_unk8[0x0c - 0x08];                 // 0x08 link length (3D); see setLength
    void setLength(float f) { *(float *)m_unk8 = f; }
    float getLength() const { return *(const float *)m_unk8; }
    // Getters read the narrow fields; setters rewrite the whole dword.
    union {
        u32 m_roadLaneIndex;                // 0x0c
        struct {
            u16 m_roadIndex;                // 0x0c
            u16 m_laneIndex;                // 0x0e
        };
    };
    union {
        u32 m_dirIndex;                     // 0x10
        struct {
            u16 m_unk10;                    // 0x10 TODO
            u8 m_index;                     // 0x12
            u8 m_dir;                       // 0x13
        };
    };
    union {
        u32 m_geoArrayIndex;                // 0x14
        struct {
            u16 m_geoIndex;                 // 0x14
            u16 m_arrayIndex;               // 0x16
        };
    };
    TrafficExtraLinkData *m_pExtraLinkData; // 0x18
};

// A node of the traffic path graph. sizeof 0x28.
class TrafficPathNode : public EXGeoPathNode {
public:
    TrafficPathNode() {}
    EXVector3 &getPos() const { return (EXVector3 &)m_pos; }
    float getWidth() const { return m_width; }
    unsigned short getIndex() const { return m_index; }
    unsigned char getRoadIndex() const { return m_roadIndex; }
    unsigned char getLaneIndex() const { return m_laneIndex; }
    unsigned char getIntersectionIndex() const { return m_intersectionIndex; }
    unsigned char getDir() const { return m_b18 & 3; }
    unsigned char getTurnType() const { return m_b18 >> 2; }
    void setIndex(unsigned short i) { m_index = i; }
    void setRoadIndex(unsigned char i) { m_w16 = (u16)((m_w16 & 0xff) | (i << 8)); }
    void setIntersectionIndex(unsigned char i) { m_w16 = (u16)((m_w16 & 0xff00) | i); }
    void setLaneIndex(unsigned char i) { m_w18 = (u16)((m_w18 & 0xff) | (i << 8)); }
    void setDir(unsigned char d) { m_w18 = (u16)((m_w18 & 0xff00) | (u8)((m_w18 & 0xfc) | d)); }
    void setTurnType(unsigned char t) { m_turnType = t; }
    void clrFlags() { m_flags = 0; }
    void clrAsDeadEnd() { m_flags &= ~2; }
    void setAsRoot() { m_flags |= 1; }
    void setAsDeadEnd() { m_flags |= 2; }
    void setAsIntersection() { m_flags |= 4; }
    void setAsReallyDeadEnd() { m_flags |= 0x10; }
    unsigned int isRoot() const { return m_flags & 1; }
    unsigned int isDeadEnd() const { return m_flags & 2; }
    unsigned int isIntersection() const { return m_flags & 4; }
    unsigned int isLoop() const { return m_flags & 8; }
    unsigned int isReallyDeadEnd() const { return m_flags & 0x10; }
    EXVector3 m_pos;                // 0x00
    float m_width;                  // 0x0c
    u32 m_unk10;                    // 0x10 TODO
    u16 m_index;                    // 0x14
    union {
        struct {
            u16 m_intersectionIndex : 8;    // 0x16
            u16 m_roadIndex : 8;            // 0x17
        };
        u16 m_w16;                          // 0x16 setters rewrite the whole word
    };
    union {
        struct {
            u16 m_dir : 2;                  // 0x18
            u16 m_turnType : 6;
            u16 m_laneIndex : 8;            // 0x19
        };
        u16 m_w18;
        u8 m_b18;                           // getDir/getTurnType read the byte
    };
    u32 m_flags;                    // 0x1c
    u8 m_unk20[0x28 - 0x20];        // 0x20 TODO
};

// A traffic path: overlays an EXGeoPath (geo nodes/links), extended by the system's additional
// nodes (ids from 32000) and links (ids past the geo link count). TODO layout of 0x00-0x3f.
class TrafficPath {
public:
    signed char isNorthPath() const;
    signed char isSouthPath() const;
    signed char isLanesPath() const;
    signed char isIntersectionsPath() const;
    // TODO: TrafficPath derives from EXGeoPath (sizeof 0x50); the geo accessors go through it.
    const EXGeoPath *geo() const { return (const EXGeoPath *)this; }
    unsigned short EXGeoNumNodes() const { return geo()->NumNodes(); }
    unsigned short EXGeoNumLinks() const { return geo()->NumLinks(); }
    unsigned short NumLinks() const;
    void debugTraceNodes() const {}
    EXGeoPathNode *EXGeoNode(unsigned int i) const
    {
        TrafficPathNode *p;
        if (!m_nodes) p = 0;
        else p = (TrafficPathNode *)((char *)&m_nodes + m_nodes);
        return p + i;
    }
    EXGeoPathLink *EXGeoLink(unsigned int i) const
    {
        TrafficPathLink *p;
        if (!m_links) p = 0;
        else p = (TrafficPathLink *)((char *)&m_links + m_links);
        return p + i;
    }
    unsigned short findPathLinkGeoIndex(TrafficPathLink *);
    TrafficPathNode *getNodePtr(unsigned int) const;
    TrafficPathLink *getLinkPtr(unsigned int, unsigned int) const;
    TrafficPathNode *Node(unsigned int) const;
    TrafficPathLink *Link(unsigned int) const;
    unsigned int removeRepeatLinks();
    void swapLinks(unsigned int, unsigned int);
    void insertLink(unsigned int, unsigned int);
    void reorderRoots(unsigned int);
    void reorderNodes();
    void initIntersections();
    void initLanes();
    void init();
    u8 m_unk0[0x40];                            // 0x00 TODO (EXGeoPath header)
    int m_numLinks;                             // 0x40 (EXGeoPath::NumLinks)
    s32 m_links;                                // 0x44 self-relative
    int m_numNodes;                             // 0x48
    s32 m_nodes;                                // 0x4c self-relative
};

// A blocked stretch of a lane. sizeof 0x14.
class TrafficLaneSeg {
public:
    unsigned char getIndex() const { return m_index; }
    TrafficLane *getLane() const { return m_pLane; }
    void setIndex(unsigned char i) { m_index = i; }
    void setLane(TrafficLane *p) { m_pLane = p; }
    void set(float start, float end, unsigned char ref)
    {
        m_start = start;
        m_end = end;
        m_ref = ref;
        m_length = end - start;
    }
    void reset()
    {
        m_start = -999.0f;
        m_ref = 0xff;
    }
    signed char isSet() const { return m_start >= 0.0f; }
    u8 m_index;             // 0x00
    u8 m_ref;               // 0x01 TODO name
    float m_start;          // 0x04
    float m_end;            // 0x08
    float m_length;         // 0x0c
    TrafficLane *m_pLane;   // 0x10
};

// Where a lane leads at its end. sizeof 0x54.
class TrafficLaneDest {
public:
    TrafficLaneDest() {}
    void reset();
    TrafficIntersection *getIntersection() const { return m_pIntersection; }
    TrafficRoad *getRoad() const { return m_pRoad; }
    TrafficLane *getLane() const { return m_pLane; }
    EXVector3 &getPos() { return m_pos; }
    float getDistFromLinkStartXZ() { return m_distFromLinkStartXZ; }
    float getDistFromLaneStartXZ() const { return m_distFromLaneStartXZ; }
    float getCrossedDistFromLinkStartXZ(unsigned int i) { return m_crossedDistFromLinkStartXZ[i]; }
    float getCrossedDistFromLaneStartXZ(unsigned int i) { return m_crossedDistFromLaneStartXZ[i]; }
    unsigned char getFlags() const { return m_flags; }
    unsigned char getTurnDir() { return m_turnDir; }
    float getAngle() const { return m_angle; }
    signed char isDeadEnd() const { return m_pRoad == 0; }
    signed char isLeadingToDeadEnd() const { return m_flags & 1; }
    unsigned char getDir() const;
    void setIntersection(TrafficIntersection *p) { m_pIntersection = p; }
    void setRoad(TrafficRoad *p) { m_pRoad = p; }
    void setLane(TrafficLane *p) { m_pLane = p; }
    void setPos(const EXVector3 *p)
    {
        m_pos.x = p->x;
        m_pos.y = p->y;
        m_pos.z = p->z;
    }
    void setDistFromLinkStartXZ(float f) { m_distFromLinkStartXZ = f; }
    void setDistFromLaneStartXZ(float f) { m_distFromLaneStartXZ = f; }
    void setCrossedDistFromLinkStartXZ(unsigned int i, float f) { m_crossedDistFromLinkStartXZ[i] = f; }
    void setCrossedDistFromLaneStartXZ(unsigned int i, float f) { m_crossedDistFromLaneStartXZ[i] = f; }
    void setTurnDir(unsigned char d) { m_turnDir = d; }
    void setFlagBit(unsigned char b) { m_flags |= b; }
    void setAngle(float f) { m_angle = f; }
    void setPathLink(TrafficPathLink *p) { m_pathLink = p->getArrayIndex(); }
    void setCrossedPathLink(unsigned int i, TrafficPathLink *p) { m_crossedPathLinks[i] = p->getArrayIndex(); }
    TrafficPathLink *getPathLink() const { return TrGetPathLink(m_pathLink); }
    TrafficPathLink *getCrossedPathLink(unsigned int i) { return TrGetPathLink(m_crossedPathLinks[i]); }
    Vehicle *checkVehicleProximity(float, float);
    Vehicle *checkCrossedLaneVehicleProximity(unsigned char, float, float);
    void blockAICrossedLanes(Vehicle *);
    u8 m_turnDir;                                       // 0x00
    u8 m_flags;                                         // 0x01
    TrafficIntersection *m_pIntersection;               // 0x04
    TrafficRoad *m_pRoad;                               // 0x08
    TrafficLane *m_pLane;                               // 0x0c
    u16 m_pathLink;                                     // 0x10
    EXFixedArray<u16, 4> m_crossedPathLinks;            // 0x12
    float m_distFromLinkStartXZ;                        // 0x1c
    float m_distFromLaneStartXZ;                        // 0x20
    EXVector3 m_pos;                                    // 0x24
    EXFixedArray<float, 4> m_crossedDistFromLinkStartXZ;    // 0x30
    EXFixedArray<float, 4> m_crossedDistFromLaneStartXZ;    // 0x40
    float m_angle;                                      // 0x50
};

// A lane of a road. sizeof 0x144.
class TrafficLane {
public:
    TrafficLane();
    ~TrafficLane();
    unsigned char getIndex() const { return m_index; }
    unsigned char getDir() const { return m_dir; }
    TrafficRoad *getRoad() const { return m_pRoad; }
    TrafficIntersection *getIntersection(unsigned char i) const { return m_intersections[i]; }
    TrafficLaneDest *getEndDest(unsigned int i) { return &m_endDests[i]; }
    EXLDList &getVehicleList() { return m_vehicleList; }
    unsigned char getNumPathLinks() const { return m_numPathLinks; }
    unsigned char getNumIntersections() const { return m_numIntersections; }
    unsigned short getPathLinkIndex() const { return m_pathLinkIndex; }
    void setPathLinkIndex(unsigned short i) { m_pathLinkIndex = i; }
    void setIndex(unsigned char i) { m_index = i; }
    void setDir(unsigned char d) { m_dir = d; }
    void setRoad(TrafficRoad *p) { m_pRoad = p; }
    void setLength(float f) { m_length = f; }
    void addIntersection(TrafficIntersection *);
    void addVehicle(Vehicle *, float, signed char);
    void addVehicleNEW(VehicleListItem *, float);
    void refreshVehicleList();
    TrafficLaneSeg *addAIBlockedSeg(float, float, unsigned char);
    TrafficLaneSeg *nextAIBlockedSeg(unsigned int &);
    void delAIBlockedSeg(unsigned char);
    float getFreeTailDistXZ();
    Vehicle *getFirstVehicle() const;
    Vehicle *getLastVehicle() const;
    Vehicle *getVehicle(unsigned int);
    TrafficPathLink *getPathLink(unsigned char i) { return TrGetPathLink(i + m_pathLinkIndex); }
    TrafficPathLink *getLastPathLink() { return TrGetPathLink((u8)(m_numPathLinks - 1) + m_pathLinkIndex); }
    void addPathLink(TrafficPathLink *);
    void swap(TrafficLane *);
    signed char hasValidDest();
    int calcLinkFromDist(float, float &, float &, float, float);
    signed char isLoop();
    TrafficPathLink *getPathLinkFromDist(float);
    float getWidth() { return TrGetPathLink(getPathLinkIndex())->getNodeAPtr()->getWidth(); }
protected:
    void reset();
public:
    u8 m_dir;                                           // 0x00
    u8 m_numPathLinks;                                  // 0x01
    u8 m_index;                                         // 0x02
    u8 m_numIntersections;                              // 0x03
    u8 m_numAIBlockedSegs;                              // 0x04
    u16 m_pathLinkIndex;                                // 0x06
    float m_length;                                     // 0x08
    TrafficRoad *m_pRoad;                               // 0x0c
    EXFixedArray<TrafficIntersection *, 2> m_intersections;  // 0x10
    EXLDList m_vehicleList;                             // 0x18
    EXFixedArray<TrafficLaneSeg, 2> m_aiBlockedSegs;    // 0x20
    EXFixedArray<TrafficLaneDest, 3> m_endDests;        // 0x48
};

// A road: up to 4 lanes in each direction. sizeof 0x14.
class TrafficRoad {
public:
    TrafficRoad();
    ~TrafficRoad();
    unsigned short getIndex() const { return m_index; }
    void setIndex(unsigned short i) { m_index = i; }
    unsigned char getNumLanes(unsigned char dir) const { return m_numLanes[dir]; }
    TrafficLane *getLane(unsigned char dir, unsigned char i);
    TrafficLane *addLane(TrafficPathLink *, unsigned char);
protected:
    void reset();
public:
    EXFixedArray<u8, 2> m_numLanes;                     // 0x00
    u16 m_index;                                        // 0x02
    EXFixedArray<EXFixedArray<u16, 4>, 2> m_lanes;      // 0x04 global lane ids
};

// A junction point on a road. sizeof 0x40.
class TrafficIntersection {
public:
    TrafficIntersection();
    ~TrafficIntersection();
    unsigned short getIndex() const { return m_index; }
    TrafficRoad *getRoad() const { return m_pRoad; }
    TrafficIntersection *getIntersectionA() const { return m_pIntersectionA; }
    TrafficIntersection *getIntersectionB() const { return m_pIntersectionB; }
    unsigned char getEndDir() const { return m_endDir; }
    unsigned char getStartDir() const { return m_startDir; }
    TrafficIntersection *getRelativeIntersection(unsigned int i) const { return m_relativeIntersections[i]; }
    TrafficRoad *getEndIntersectRoad() const { return m_pEndIntersectRoad; }
    unsigned char getEndIntersectDir() const { return m_endIntersectDir; }
    TrafficBoxJunction *getBoxJunction() const { return m_pBoxJunction; }
    TrafficLight *getTrafficLight() const { return m_pTrafficLight; }
    unsigned char getAIEntryBlockRef() const { return m_aiEntryBlockRef; }
    unsigned char getAIExitBlockRef() const { return m_aiExitBlockRef; }
    unsigned char isRoadStart() const { return m_flags & 1; }
    unsigned char isRoadEnd() const { return m_flags & 2; }
    unsigned char isAIEntryBlocked() const { return m_flags & 4; }
    unsigned char isAIExitBlocked() const { return m_flags & 8; }
    void setIndex(unsigned short i) { m_index = i; }
    void setRoad(TrafficRoad *p) { m_pRoad = p; }
    void setIntersectionA(TrafficIntersection *p) { m_pIntersectionA = p; }
    void setIntersectionB(TrafficIntersection *p) { m_pIntersectionB = p; }
    void setEndDir(unsigned char d) { m_endDir = d; }
    void setStartDir(unsigned char d) { m_startDir = d; }
    void setRelativeIntersection(unsigned int i, TrafficIntersection *p) { m_relativeIntersections[i] = p; }
    void setEndIntersectRoad(TrafficRoad *p) { m_pEndIntersectRoad = p; }
    void setEndIntersectDir(unsigned char d) { m_endIntersectDir = d; }
    void setBoxJunction(TrafficBoxJunction *p) { m_pBoxJunction = p; }
    void setTrafficLight(TrafficLight *p) { m_pTrafficLight = p; }
    void setAsRoadStart() { m_flags |= 1; }
    void setAsRoadEnd() { m_flags |= 2; }
    void setAsAIExitBlocked(unsigned char r)
    {
        m_flags |= 8;
        m_aiExitBlockRef = r;
    }
    void clrAsAIExitBlocked(unsigned char r)
    {
        m_flags &= ~8;
        m_aiExitBlockRef = r;
    }
    void setPathLink(TrafficPathLink *p) { m_pathLink = p->getArrayIndex(); }
    void setEndIntersectLink(TrafficPathLink *p) { m_endIntersectLink = p->getArrayIndex(); }
    TrafficPathLink *getPathLink() const { return TrGetPathLink(m_pathLink); }
    TrafficPathLink *getEndIntersectLink() const { return TrGetPathLink(m_endIntersectLink); }
    void removePendingVehicle(Vehicle *) {}
    Vehicle *firstPendingVehicle(unsigned int &);
    Vehicle *nextPendingVehicle(unsigned int &);
protected:
    void reset();
public:
    u8 m_flags;                                         // 0x00
    u8 m_endDir;                                        // 0x01
    u8 m_startDir;                                      // 0x02
    u8 m_endIntersectDir;                               // 0x03
    u8 m_aiExitBlockRef;                                // 0x04
    u8 m_aiEntryBlockRef;                               // 0x05
    u16 m_index;                                        // 0x06
    TrafficRoad *m_pRoad;                               // 0x08
    TrafficRoad *m_pEndIntersectRoad;                   // 0x0c
    u16 m_pathLink;                                     // 0x10
    u16 m_endIntersectLink;                             // 0x12
    TrafficIntersection *m_pIntersectionA;              // 0x14
    TrafficIntersection *m_pIntersectionB;              // 0x18
    EXFixedArray<TrafficIntersection *, 3> m_relativeIntersections;  // 0x1c
    TrafficBoxJunction *m_pBoxJunction;                 // 0x28
    TrafficLight *m_pTrafficLight;                      // 0x2c
    EXFixedArray<Vehicle *, 4> m_pendingVehicles;       // 0x30
};

// Box junction occupancy. TODO layout.
class TrafficBoxJunction {
public:
    void removeVehicle(Vehicle *pVehicle)
    {
        u32 i;
        for (i = 0; i < 8; i++) {
            if (m_vehicles[i] == pVehicle) goto found;
        }
        return;
    found:
        m_vehicles[i] = 0;
        m_numVehicles--;
    }
    TrafficBoxJunction() {}
    void reset()
    {
        m_numVehicles = 0;
        for (u32 i = 0; i < 4; i++) m_intersections[i] = 0;
        for (u32 j = 0; j < 8; j++) m_vehicles[j] = 0;
        m_unk1 = 0;
        m_f34 = 0.0f;
    }
    TrafficIntersection *getIntersection(unsigned int i) const { return m_intersections[i]; }
    unsigned char getNumVehicles() const { return m_numVehicles; }
    EXFixedArray<EXVector2, 4> &getQuad() { return m_quad; }
    void setTrafficLightSeqLen(unsigned char n) { m_trafficLightSeqLen = n; }
    void setIntersection(unsigned int i, TrafficIntersection *p);
    void nextTrafficLightSeq();
    // NEAR: retail standalone copy reloads i and returns an explicit 0 (27 bytes); this form
    // gives the inlined shape (TrafficBoxJunction::update matches).
    TrafficIntersection *firstIntersection(unsigned int &i)
    {
        TrafficIntersection *p;
        for (i = 0; i < 4; i++) {
            p = m_intersections[i];
            if (p) break;
        }
        return p;
    }
    TrafficIntersection *nextIntersection(unsigned int &i)
    {
        while (++i < 4) {
            TrafficIntersection *p = m_intersections[i];
            if (p) return p;
        }
        return 0;
    }
    void addVehicle(Vehicle *pVehicle)
    {
        for (u32 i = 0; i < 8; i++) {
            if (!m_vehicles[i]) {
                m_vehicles[i] = pVehicle;
                m_numVehicles++;
                return;
            }
        }
    }
    Vehicle *firstVehicle(unsigned int &i)
    {
        for (i = 0; i < 8; i++) {
            if (m_vehicles[i]) return m_vehicles[i];
        }
        return 0;
    }
    Vehicle *nextVehicle(unsigned int &i)
    {
        while (++i < 8) {
            if (m_vehicles[i]) return m_vehicles[i];
        }
        return 0;
    }
    void update();  // defined inline in Traffic.cpp (needs TrafficLight)
    u8 m_numVehicles;                       // 0x00
    u8 m_unk1;                              // 0x01 TODO
    u8 m_trafficLightSeqLen;                // 0x02
    EXFixedArray<TrafficIntersection *, 4> m_intersections;  // 0x04
    EXFixedArray<Vehicle *, 8> m_vehicles;  // 0x14
    float m_f34;                            // 0x34 TODO name
    EXFixedArray<EXVector2, 4> m_quad;      // 0x38
    u8 m_quadType;                          // 0x58 how m_quad was built (0 none, 1-3)
    u8 m_pad59[3];
};

inline void TrafficBoxJunction::setIntersection(unsigned int i, TrafficIntersection *p)
{
    m_intersections[i] = p;
    if (p) p->setBoxJunction(this);
}

class TrafficPopulace;  // TrafficSystem.h
class EXTrigger;

// A scripted spawn trigger. sizeof 0x88. TODO layout.
// A copy of a traffic trigger from the map (TrafficTriggers::add). sizeof 0x88.
class TrafficTrigger {
public:
    TrafficTrigger() {}
    u32 m_type;         // 0x00 HT_TriggerType_*
    u32 m_flags;        // 0x04 EXTrigger +0x4c
    EXVector m_pos;     // 0x08
    EXVector m_rot;     // 0x18
    u32 m_data[16];     // 0x28
    int m_links[8];     // 0x68
};

// HT_TriggerType_Vehicle data: 0 vehicle hash, 1 spawn flag channel (999 at once, 666 even on
// screen), 2 route path, 3 m_unk708, 4 m_unk730 text, 5 phys +0x140, 6 throttle, 7 max speed,
// 8 sfx (negative random), 12 health, 15 start parked (move state 7); link 0 objective trigger.
class TrafficTrigger_Vehicle : public TrafficTrigger {};

class TrafficTriggers {
public:
    TrafficTriggers() {}
    void reset() { m_numTriggers = 0; }
    TrafficTrigger *get(unsigned int i) { return &m_triggers[i]; }
    void add(EXTrigger *);  // [fi] defined in Traffic.cpp
    void preInit();
    Vehicle *spawnVehicle(TrafficTrigger_Vehicle *, int);
    void postInit(signed char);
    void update();
    EXFixedArray<TrafficTrigger, 64> m_triggers;  // 0x0000
    u16 m_numTriggers;                          // 0x2200
};
extern TrafficTriggers _theTrafficTriggers;

class TrafficSysAlloc {
public:
    TrafficSysAlloc() { m_pSys = 0; }
    ~TrafficSysAlloc();
    TrafficSystem *getSys() const { return m_pSys; }
    signed char open();
    void close();
    signed char verifyPaths();
    TrafficSystem *m_pSys;
};
extern TrafficSysAlloc _theTrafficSysAlloc;

TrafficPopulace &TrafficGetPopulace();
signed char TrafficIsInit();
extern float frameTime;
extern float frameLen;

#endif
