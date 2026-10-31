#ifndef VEHICLEPOS_H
#define VEHICLEPOS_H
#include "types.h"
#include <string.h>
#include "EXVector.h"
#include "EXArray.h"
#include "EXList.h"
#include "VehicleSubClass.h"
#include "Traffic.h"

class EXLozenge;
class EXMatrix;
struct EXBoundsBox;

// Oriented box: centre, three axes, three half extents. sizeof 0x4c.
class MHBox {
public:
    MHBox() {}
    EXVector &Centre() { return m_centre; }
    EXVector &Axis(int i) { return m_axis[i]; }
    float &Extent(int i) { return m_extent[i]; }
    void Set(const EXLozenge &);
    void ComputeVertices(EXFixedArray<EXMaths::vec_xyzw, 8> &) const;
    void ApplyXForm(const EXMatrix &);
    void GetBoundsBox(EXBoundsBox &) const;
    EXVector m_centre;                  // 0x00
    EXFixedArray<EXVector, 3> m_axis;   // 0x10
    EXFixedArray<float, 3> m_extent;    // 0x40
};

// Link of a vehicle in a lane's vehicle list. sizeof 0x1c.
class VehicleListItem : public EXLDListItem {
public:
    VehicleListItem() { reset(0); }
    VehicleListItem *Prev() { return (VehicleListItem *)m_pPrev; }
    VehicleListItem *Next() { return (VehicleListItem *)m_pNext; }
    float getDist() const { return m_dist; }
    void setDist(float f) { m_dist = f; }
    void setLane(TrafficLane *p) { m_pLane = p; }
    void reset(Vehicle *pVehicle)
    {
        m_bUsed = 0;
        m_pList = 0;
        m_pVehicle = pVehicle;
        m_pPrev = 0;
        m_pNext = 0;
    }
    signed char isUsed() const { return m_bUsed; }
    void setUsed(signed char b) { m_bUsed = b; }
    Vehicle *m_pVehicle;    // 0x0c
    s8 m_bUsed;             // 0x10
    float m_dist;           // 0x14
    TrafficLane *m_pLane;   // 0x18
};

// A vehicle's presence in one lane. sizeof 0x40.
class VehicleInLane : public VehicleSubClass {
public:
    VehicleInLane()
    {
        m_pLane = 0;
        m_bReallyInLane = 0;
    }
    float getRearLaneDist() const { return m_rearLaneDist; }
    float getFrontLaneDist() const { return m_frontLaneDist; }
    VehicleListItem &getListItem() { return m_listItem; }
    void setLane(TrafficLane *p) { m_pLane = p; }
    TrafficLane *getLane() const { return m_pLane; }
    TrafficPathLink *getLink() const { return TrGetPathLink(m_link); }
    float getLaneDist() const { return m_laneDist; }
    float getHorzOffset() const { return m_horzOffset; }
    signed char isReallyInLane() const { return m_bReallyInLane; }
    void setLaneDist(float f) { m_laneDist = f; }
    void setRearLaneDist(float f) { m_rearLaneDist = f; }
    void setFrontLaneDist(float f) { m_frontLaneDist = f; }
    void setHorzOffset(float f) { m_horzOffset = f; }
    void setLinkDist(float f) { m_linkDist = f; }
    void setReallyInLane(signed char b) { m_bReallyInLane = b; }
    float getFrontDistFromLinkStart();
    float getRearDistFromLinkStart();
    void setLink(TrafficPathLink *);
    float getFrontDistFromLinkEnd();
    float getRearDistFromLinkEnd();

    s8 m_bReallyInLane;         // 0x04
    TrafficLane *m_pLane;       // 0x08
    u16 m_link;                 // 0x0c path link id
    float m_laneDist;           // 0x10
    float m_rearLaneDist;       // 0x14
    float m_frontLaneDist;      // 0x18
    float m_horzOffset;         // 0x1c
    float m_linkDist;           // 0x20
    VehicleListItem m_listItem; // 0x24
};

// Vehicle position and lane occupancy (VehiclePos.obj). sizeof 0x1f0.
class VehiclePos : public VehicleSubClass {
public:
    VehiclePos() { reset(0); }
    ~VehiclePos() {}
    void get(EXVector &v)
    {
        v.x = m_pos.x;
        v.y = m_pos.y;
        v.z = m_pos.z;
        v.w = 0.0f;
    }
    const EXVector3 &get() const { return m_pos; }
    void get(EXVector3 &v)
    {
        v.x = m_pos.x;
        v.y = m_pos.y;
        v.z = m_pos.z;
    }
    TrafficLocation &getLoc() { return m_loc; }
    VehicleInLane &getInLane(unsigned int i) { return m_inLanes[i]; }
    void set(EXVector3 &v)
    {
        if (!m_bSet) m_prevPos = v;
        else m_prevPos = m_pos;
        m_pos = v;
        m_bSet = 1;
        recalc();
        updateListItems();
    }
    void removeInLanes()
    {
        for (int i = 0; i < 3; i++) {
            if (m_inLanes[i].m_listItem.isUsed()) {
                m_inLanes[i].m_listItem.Remove();
                m_inLanes[i].m_listItem.setUsed(0);
            }
        }
    }
    float getFrontLaneDist(unsigned int i) { return m_inLanes[i].m_frontLaneDist; }
    void reset(Vehicle *pVehicle)
    {
        m_bSet = 0;
        if (m_pVehicle) removeInLanes();
        m_unk8 = 0;
        m_pVehicle = pVehicle;
        m_loc.reset();
        for (int i = 0; i < 3; i++) {
            m_inLanes[i].m_pLane = 0;
            m_inLanes[i].m_listItem.setUsed(0);
            m_inLanes[i].m_listItem.m_pVehicle = pVehicle;
            m_inLanes[i].setVehicle(pVehicle);
            m_inLanes[i].m_listItem.SetList(0);
        }
        m_neighbourLaneCount[1] = 0;
        m_neighbourLaneCount[0] = 0;
        memset(&m_neighbourLanes, 0, sizeof(m_neighbourLanes));
    }
    EXFixedArray<EXMaths::vec_xyzw, 8> &getTransformedBoxVerts() { return m_transformedBoxVerts; }
    float getRearLaneDist(TrafficLane *pLane) { return getInLane(pLane, 0)->m_rearLaneDist; }
    EXVector &getFrontBottomCentre(EXVector &) const;
    float getLaneDist(unsigned int i) { return m_inLanes[i].m_laneDist; }
    float getFrontDistFromLinkStart(unsigned int i) { return m_inLanes[i].getFrontDistFromLinkStart(); }
    unsigned int getNeighbourLaneCount(unsigned int i) const { return m_neighbourLaneCount[i]; }
    TrafficLane *getNeighbourLane(unsigned int side, unsigned int i) { return m_neighbourLanes[side][i]; }
    void setLocLink(unsigned char i);
    MHBox &getTransformedBox() { return m_transformedBox; }
    void setLocLane(TrafficLane *, float);
    float getDistBetweenCentres(Vehicle *);
    void resetInLaneLanes()
    {
        m_inLanes[0].m_pLane = 0;
        m_inLanes[1].m_pLane = 0;
        m_inLanes[2].m_pLane = 0;
    }
    void updateListItems();
    signed char getLaneDists(TrafficLane *, float &, float &);
    void setLoc(TrafficLocation *);
    signed char calcNeighbouringLanes();
    VehicleInLane *getInLane(TrafficLane *, unsigned int *);
    float getDistToNextNode(TrafficLane *);
    float getLinesegDistFromFrontToOtherRear(Vehicle *);
    float getHorzOffset(signed char);
    void getLanePos(TrafficLane *, float, EXVector &);
    TrafficPathLink *nearestPathLink(EXVector3 &, TrafficLane *);
    signed char doesVehiclePassOtherHorz(TrafficLane *, Vehicle *, unsigned int);
    void calcLaneLoc(TrafficLane *, unsigned int);
    Vehicle *getVehicleInFront(signed char &, TrafficLane *, unsigned int);
    Vehicle *getVehicleBehind(signed char &, TrafficLane *, unsigned int);
    float getDistToVehicleInFront(TrafficLane *, Vehicle **, unsigned int);
    float getDistToVehicleBehind(TrafficLane *, Vehicle **, unsigned int);
    void getLaneGap(TrafficLane *, float &, Vehicle **, float &, Vehicle **);
    void recalc();

    s8 m_bSet;                                          // 0x04
    u32 m_unk8;                                         // 0x08 TODO
    EXVector3 m_pos;                                    // 0x0c
    EXVector3 m_prevPos;                                // 0x18
    TrafficLocation m_loc;                              // 0x24
    MHBox m_transformedBox;                             // 0x3c
    EXFixedArray<EXMaths::vec_xyzw, 8> m_transformedBoxVerts;  // 0x88
    EXFixedArray<VehicleInLane, 3> m_inLanes;           // 0x108
    EXFixedArray<EXFixedArray<TrafficLane *, 4>, 2> m_neighbourLanes;  // 0x1c8
    EXFixedArray<unsigned int, 2> m_neighbourLaneCount; // 0x1e8
};
#endif
