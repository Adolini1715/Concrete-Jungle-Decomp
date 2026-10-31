#ifndef TRAFFICLIGHT_H
#define TRAFFICLIGHT_H
// Traffic light with four item heads (Traffic.obj). sizeof 0x28. Layout from the accessors and ctor.
#include "types.h"
#include "EXArray.h"
#include "XItem.h"

class TrafficIntersection;

class TrafficLight {
public:
    TrafficLight(unsigned int);
    ~TrafficLight();
    EXItem *getItem(unsigned int i) { return m_items[i]; }
    void setPos(EXVector &v)
    {
        m_items[0]->SetPosition(v);
        m_items[1]->SetPosition(v);
        m_items[2]->SetPosition(v);
        m_items[3]->SetPosition(v);
    }
    void setRot(EXVector &v)
    {
        m_items[0]->SetRotation((EXQuaternion &)v);
        m_items[1]->SetRotation((EXQuaternion &)v);
        m_items[2]->SetRotation((EXQuaternion &)v);
        m_items[3]->SetRotation((EXQuaternion &)v);
    }
    unsigned int getState() const { return m_state; }
    TrafficIntersection *getIntersection(unsigned int i) const { return m_intersections[i]; }
    unsigned char getNumIntersections() const { return m_numIntersections; }
    unsigned int getSequence() const { return m_sequence; }
    unsigned short getDuration() const { return m_duration; }
    signed char isSequenceOn(unsigned char n)
    {
        signed char r;
        if (!n) r = m_sequence & 1;
        else r = (m_sequence / (n * 10)) & 1;
        return r;
    }
    void setIndex(unsigned short i) { m_index = i; }
    void addIntersection(TrafficIntersection *p)
    {
        m_intersections[m_numIntersections] = p;
        m_numIntersections++;
    }
    void setSequence(unsigned int s) { m_sequence = s; }
    void setDuration(unsigned short d) { m_duration = d; }
    void changeState();
    void update();
    void setState(unsigned int s)
    {
        m_state = s;
        update();
    }
    u8 m_unk0;                                          // 0x00 TODO
    u8 m_numIntersections;                              // 0x01
    u16 m_index;                                        // 0x02
    u16 m_duration;                                     // 0x04
    u32 m_state;                                        // 0x08
    u32 m_sequence;                                     // 0x0c
    float m_timer;                                      // 0x10
    EXFixedArray<TrafficIntersection *, 1> m_intersections;  // 0x14
    EXFixedArray<EXItem *, 4> m_items;                  // 0x18
};

#endif
