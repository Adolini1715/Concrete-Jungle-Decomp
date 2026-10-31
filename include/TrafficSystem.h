#ifndef TRAFFICSYSTEM_H
#define TRAFFICSYSTEM_H
// TrafficPopulace / TrafficSystem: need the complete Vehicle (EXArray<Vehicle> instantiates its virtual dtor).
#include "Traffic.h"
#include "Vehicle.h"

class VehicleArray : public EXArray<Vehicle, Vehicle &> {};
class TrafficRoadArray : public EXArray<TrafficRoad, TrafficRoad &> {};
class TrafficIntersectionArray : public EXArray<TrafficIntersection, TrafficIntersection &> {};
class TrafficLocationArray : public EXArray<TrafficLocation, TrafficLocation &> {};
class TrafficPathLinkPtrArray : public EXArray<TrafficPathLink *, TrafficPathLink *&> {};
class TrafficPathNodeArray : public EXArray<TrafficPathNode, TrafficPathNode &> {};
class TrafficPathLinkArray : public EXArray<TrafficPathLink, TrafficPathLink &> {};
class TrafficBoxJunctionArray : public EXArray<TrafficBoxJunction, TrafficBoxJunction &> {};
class TrafficExtraLinkDataArray : public EXArray<TrafficExtraLinkData, TrafficExtraLinkData &> {};
class TrafficLaneArray : public EXArray<TrafficLane, TrafficLane &> {};

class TrafficPopulace {
public:
    TrafficPopulace();
    ~TrafficPopulace();
    unsigned short getNumVehicles() const { return m_nVehicles; }
    void reset()
    {
        m_nVehicles = 0;
        m_vehicles.SetSize(25);
    }
    void removeVehicles()
    {
        m_vehicles.RemoveAll();
        reset();
    }
    void removeVehicle(Vehicle *);
    Vehicle *getVehicle(unsigned short);
    void OnSubFileLoaded(unsigned int);
    Vehicle *addVehicle(signed char);
    Vehicle *addVehicle(unsigned char, TrafficLocation *);
    Vehicle *addVehicleFromHash(unsigned int, TrafficLocation *);
    u16 m_nVehicles;            // 0x00
    VehicleArray m_vehicles;    // 0x04
};

// The whole road network (Traffic.obj). sizeof 0x494.
class TrafficSystem {
public:
    TrafficSystem();
    ~TrafficSystem();
    void open();
    void close();
    void update();
    void handleMemCompaction();
    signed char isNightLevel() const { return m_bNightLevel; }
    unsigned short getNumLanes() const { return m_numLanes; }
    void setNumLanes(unsigned short n) { m_numLanes = n; }
    TrafficPathLinkPtrArray &getPathLinkPtrs() { return m_pathLinkPtrs; }
    unsigned short getNumLanePathLinks() const { return m_numPathLinks - m_numIntersections; }
    TrafficLight *getTrafficLight(unsigned short i) { return m_trafficLights[i]; }
    unsigned short getNumRoads() const { return m_numRoads; }
    unsigned short getNumIntersections() const { return m_numIntersections; }
    unsigned short getNumAdditionalPathLinks(unsigned char i) const { return m_numAdditionalPathLinks[i]; }
    unsigned short getNumTrafficLights() const { return m_numTrafficLights; }
    unsigned int getUpdateCount() const { return m_updateCount; }
    TrafficIntersectionArray &getIntersections() { return m_intersections; }
    TrafficPath *getLanesPath(unsigned char i) const { return m_lanesPaths[i]; }
    TrafficPath *getNorthLanesPath() const { return m_lanesPaths[0]; }
    TrafficPath *getSouthLanesPath() const { return m_lanesPaths[1]; }
    TrafficPath *getIntersectionsPath() const { return m_pIntersectionsPath; }
    void setIntersectionsPath(TrafficPath *p) { m_pIntersectionsPath = p; }
    TrafficPopulace &getPopulace() { return m_populace; }
    TrafficLocationArray &getLocs() { return m_locs; }
    void draw() const {}
    TrafficLane *getLane(unsigned short i) { return &m_lanes[i]; }
    TrafficPathLink *getPathLink(unsigned short i)
    {
        if (i >= 0xfff0) return 0;
        return m_pathLinkPtrs[i];
    }
    TrafficIntersection *getIntersection(unsigned short i) { return &m_intersections.ElementAt(i); }
    TrafficRoad *getRoad(unsigned short i) { return &m_roads.ElementAt(i); }
    TrafficPathNode *getAdditionalPathNode(unsigned short i) { return &m_additionalPathNodes.ElementAt(i); }
    TrafficPathLink *getAdditionalPathLink(unsigned char dir, unsigned short i) { return &m_additionalPathLinks[dir].ElementAt(i); }
    TrafficBoxJunction *getBoxJunction(unsigned short);
    TrafficIntersection *addIntersection(TrafficPathLink *);
    void addLoc(unsigned short, TrafficLocation *);
    unsigned short addPathLink(TrafficPathLink *, unsigned char);
    TrafficBoxJunction *newBoxJunction();
    void getRandLocation(TrafficLocation &, float, float);
    signed char calcLocFromPosition(EXVector &, TrafficLocation &, float);
protected:
    TrafficRoad *addRoad();
    unsigned short addAdditionalPathNode(TrafficPathNode *, TrafficPathNode **);
    unsigned short insertAdditionalPathLink(unsigned char, unsigned char, TrafficPathLink *, TrafficPathLink **);
    void fixLaneOrder();
    void reset();
    void buildFromPaths();
    void postBuildSetup();
public:
    s8 m_bOpen;                                         // 0x00
    u8 m_unk1;                                          // 0x01 TODO
    s8 m_bNightLevel;                                   // 0x02
    u16 m_numRoads;                                     // 0x04
    u16 m_numIntersections;                             // 0x06
    u16 m_numAdditionalPathNodes;                       // 0x08
    EXFixedArray<u16, 2> m_numAdditionalPathLinks;      // 0x0a
    u16 m_numBoxJunctions;                              // 0x0e
    u16 m_numTrafficLights;                             // 0x10
    u16 m_numPathLinks;                                 // 0x12
    EXFixedArray<u16, 3> m_numLanePathLinks;            // 0x14 per direction (+ one more, TODO)
    u16 m_numLanes;                                     // 0x1a
    u32 m_updateCount;                                  // 0x1c
    EXFixedArray<TrafficPath *, 2> m_lanesPaths;        // 0x20
    TrafficPath *m_pIntersectionsPath;                  // 0x28
    EXFixedArray<TrafficLight *, 16> m_trafficLights;   // 0x2c
    TrafficRoadArray m_roads;                           // 0x6c
    TrafficIntersectionArray m_intersections;           // 0x80
    TrafficPathNodeArray m_additionalPathNodes;          // 0x94
    TrafficPathLinkArray m_additionalPathLinks[2];       // 0xa8
    TrafficBoxJunctionArray m_boxJunctions;             // 0xd0
    TrafficExtraLinkDataArray m_extraLinkData;        // 0xe4
    TrafficPopulace m_populace;                         // 0xf8
    TrafficLocationArray m_locs;                        // 0x110
    TrafficLaneArray m_lanes;                           // 0x124
    TrafficPathLinkPtrArray m_pathLinkPtrs;             // 0x138
    EXFixedArray<u16, 420> m_pathLinkGeoIndices;        // 0x14c per path link (0xfffe: none)
};

inline signed char TrafficPath::isNorthPath() const { return this == _theTrafficSysAlloc.getSys()->getNorthLanesPath(); }
inline signed char TrafficPath::isSouthPath() const { return this == _theTrafficSysAlloc.getSys()->getSouthLanesPath(); }
inline signed char TrafficPath::isLanesPath() const
{
    if (isNorthPath() || isSouthPath()) return 1;
    return 0;
}
inline signed char TrafficPath::isIntersectionsPath() const { return this == _theTrafficSysAlloc.getSys()->getIntersectionsPath(); }
inline unsigned short TrafficPath::NumLinks() const
{
    return EXGeoNumLinks() + (isLanesPath() ? _theTrafficSysAlloc.getSys()->getNumAdditionalPathLinks(!isNorthPath()) : 0);
}
inline TrafficPathNode *TrafficPath::getNodePtr(unsigned int i) const
{
    if (!EXGeoNumNodes()) return 0;
    if (i < EXGeoNumNodes()) return (TrafficPathNode *)EXGeoNode(i);
    return _theTrafficSysAlloc.getSys()->getAdditionalPathNode(i - 32000);
}
inline TrafficPathLink *TrafficPath::getLinkPtr(unsigned int dir, unsigned int i) const
{
    if (i < EXGeoNumLinks()) return (TrafficPathLink *)EXGeoLink(i);
    return _theTrafficSysAlloc.getSys()->getAdditionalPathLink(dir, i - EXGeoNumLinks());
}
inline TrafficPathNode *TrafficPath::Node(unsigned int i) const
{
    TrafficPathNode *p;
    if (isLanesPath()) p = getNodePtr(i);
    else p = (TrafficPathNode *)EXGeoNode(i);
    return p;
}
inline TrafficPathLink *TrafficPath::Link(unsigned int i) const
{
    TrafficPathLink *p;
    if (isLanesPath()) p = getLinkPtr((unsigned char)!isNorthPath(), i);
    else p = (TrafficPathLink *)EXGeoLink(i);
    return p;
}
inline void TrafficLocation::setLink(unsigned char i)
{
    m_linkIndex = i;
    m_linkId = getLane()->getPathLink(i)->getArrayIndex();
}
inline TrafficPathLink *TrafficLocation::getPathLink() const { return _theTrafficSysAlloc.getSys()->getPathLink(m_linkId); }
inline TrafficPath *TrafficPathLink::getPath() const
{
    if (isIntersectionType()) return _theTrafficSysAlloc.getSys()->getIntersectionsPath();
    if (isSouthType()) return _theTrafficSysAlloc.getSys()->getSouthLanesPath();
    return _theTrafficSysAlloc.getSys()->getNorthLanesPath();
}
inline TrafficPathNode *TrafficPathLink::getNodeAPtr() const
{
    TrafficPath *pPath = getPath();
    return pPath->Node(getNodeA());
}
inline TrafficPathNode *TrafficPathLink::getNodeBPtr() const
{
    TrafficPath *pPath = getPath();
    return pPath->Node(getNodeB());
}
inline TrafficRoad *TrafficPathLink::getRoad() const { return _theTrafficSysAlloc.getSys()->getRoad(getRoadIndex()); }
inline float TrafficPathLink::getDistFromLinkStartXZ(const EXVector3 *p, signed char)
{
    TrafficPath *pPath = getPath();
    TrafficPathNode *pA = pPath->Node(getNodeA());
    float dx = p->x - pA->getPos().x;
    float dz = p->z - pA->getPos().z;
    return EXSqrt(dx * dx + dz * dz);
}

extern TrafficPathLinkPtrArray VisPathLinks;
extern int nVisPathLinks;

#endif
