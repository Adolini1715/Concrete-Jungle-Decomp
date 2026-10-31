#ifndef VEHICLEMOVE_H
#define VEHICLEMOVE_H
#include "types.h"
#include "EXVector.h"
#include "EXArray.h"
#include "EXList.h"
#include "VehicleSubClass.h"
#include "Traffic.h"

class TrafficLaneDest;
class TrafficLaneSeg;
class Actor;
extern float frameLen;
extern float framesPerHourDivRateMul;
signed char isValidFloat(float, signed char);

float AngleDiff(float, float);  // EXMaths.obj
extern EXVector lcInitialLanePos;
extern EXVector lcDestLanePos;
// NEAR: retail duplicates the lerp tail into the t >= 1 branch.
inline float InterpClamped(float a, float b, float t)
{
    if (t >= 1.0f) t = 1.0f;
    else t = max(0.0f, t);
    return (b - a) * t + a;
}
extern "C" int __cdecl abs(int);
#pragma intrinsic(abs)
inline int qabs(int i) { return abs(i); }
template <class T> inline const T &vmMax(const T &a, const T &b) { return a > b ? a : b; }
// NEAR: VC tail-jumps to __CIfmod; retail calls it and returns.
inline float fixAnglePositive(float a)
{
    if (a <= 3.1415927f && a >= -3.1415927f) return a;
    return fmod(a, 6.2831855f);
}
inline float feet_to_m(float f) { return f * 0.30478588f; }
inline float mile_to_km(float f) { return f * 1.6093f; }
inline float km_to_mile(float f) { return f * 0.6213882f; }
// NEAR: 29 bytes differ, VC factors mph out of the polynomial (retail keeps mph*mph*0.025f + mph).
inline float kmh_to_stopping_dist(float kmh)
{
    float mph = km_to_mile(kmh);
    float d = feet_to_m(mph * mph * 0.025f + mph);
    return d < 1.1f ? 1.1f : d;
}

// Path node reference by (index, path) that survives path reallocation. sizeof 4.
class TrafficSafePathNodePtr {
public:
    TrafficPathNode *get();     // inline in VehicleMove.cpp (needs TrafficSystem)
    void set(TrafficPathNode *p)
    {
        if (p) {
            m_node = p->getIndex();
            m_path = p->getDir();
        } else {
            m_node = 0xffff;
        }
    }
    u16 m_node;     // 0x00
    u16 m_path;     // 0x02 lanes path (dir)
};

// Pending checks before a vehicle may continue into a junction. sizeof 0x14.
class VehicleMoveContinueChecks {
public:
    VehicleMoveContinueChecks() { reset(); }
    void reset()
    {
        m_pDest = 0;
        m_pExitIntersection = 0;
        m_timer = 0.0f;
        m_timerCmp = 0.0f;
        m_flags = 0;
    }
    TrafficLaneDest *getDest() const { return m_pDest; }
    TrafficIntersection *getExitIntersection() const { return m_pExitIntersection; }
    float getTimerCmp() const { return m_timerCmp; }
    unsigned char getFlags() const { return m_flags; }
    void set(TrafficLaneDest *pDest, TrafficIntersection *pInt, float t)
    {
        m_timerCmp = t;
        m_pDest = pDest;
        m_pExitIntersection = pInt;
        m_timer = 0.0f;
        m_flags = 0;
    }
    void setDest(TrafficLaneDest *p) { m_pDest = p; }
    void setTimer(float f) { m_timer = f; }
    void setTimerCmp(float f) { m_timerCmp = f; }
    void setFlagBit(unsigned char b) { m_flags |= b; }
    void clrFlagBit(unsigned char b) { m_flags &= ~b; }
    signed char incrTimer()
    {
        m_timer = frameLen + m_timer;
        if (m_timer >= m_timerCmp) {
            m_timerCmp = 0.0f;
            return 1;
        }
        return 0;
    }
    u8 m_flags;                                 // 0x00
    float m_timer;                              // 0x04
    float m_timerCmp;                           // 0x08
    TrafficLaneDest *m_pDest;                   // 0x0c
    TrafficIntersection *m_pExitIntersection;   // 0x10
};

// Running average over the last m_len samples. sizeof 0x88.
class FloatDampen {
public:
    FloatDampen()
    {
        for (int i = 0; i < 32; i++) m_data[i] = 0.0f;
        m_index = 0;
    }
    void setLen(unsigned int n)
    {
        if (n > 32) n = 32;
        m_len = n;
    }
    void add(float f)
    {
        m_data[m_index] = f;
        if (++m_index == m_len) m_index = 0;
    }
    float get()
    {
        float sum = 0.0f;
        for (unsigned int i = 0; i < m_len; i++) sum += m_data[i];
        return sum / m_len;
    }
    unsigned int m_index;               // 0x00
    unsigned int m_len;                 // 0x04
    EXFixedArray<float, 32> m_data;     // 0x08
};

// Body physics (pitch/roll springs). sizeof 0x144.
class VehiclePhys : public VehicleSubClass {
public:
    VehiclePhys() { reset(0); }
    void reset(Vehicle *pVehicle)
    {
        m_pVehicle = pVehicle;
        m_f1c = 0.0f;
        m_f20 = 0.0f;
        m_f4 = 0.0f;
        m_f8 = 0.0f;
        m_fc = 0.0f;
        m_f10 = 0.0f;
        m_f14 = 0.0f;
        m_f18 = 0.0f;
        m_dampen0.setLen(10);
        m_dampen1.setLen(8);
        m_velocity.x = 0.0f;
        m_velocity.y = 0.0f;
        m_velocity.z = 0.0f;
        m_f140 = 0.0f;
    }
    void update();
    void setRotations();
    float m_f4, m_f8, m_fc, m_f10, m_f14, m_f18, m_f1c, m_f20;  // 0x04 TODO names
    FloatDampen m_dampen0;      // 0x24
    FloatDampen m_dampen1;      // 0xac
    EXVector3 m_velocity;       // 0x134
    float m_f140;               // 0x140 TODO name
};

// One acceleration target. sizeof 0x10.
class VehicleAccelTarget {
public:
    void set(float dist, float speed, float frontLaneDist, unsigned char type)
    {
        m_type = type;
        if (dist < 0.0f) dist = 0.0f;
        m_dist = dist;
        if (speed < 0.0f) speed = 0.0f;
        m_speed = speed;
        m_frontLaneDist = frontLaneDist;
    }
    unsigned char getType() const { return m_type; }
    float getDist() const { return m_dist; }
    float getSpeed() const { return m_speed; }
    float getFrontLaneDist() const { return m_frontLaneDist; }
    u8 m_type;              // 0x00
    float m_dist;           // 0x04
    float m_speed;          // 0x08
    float m_frontLaneDist;  // 0x0c
};

// Acceleration control (VehicleMove+0x90). sizeof 0xb8.
class VehicleAccel : public VehicleSubClass {
public:
    void reset(Vehicle *);
    void clrTarget(unsigned int);
    void combineTargets();
    void update();
    unsigned char getState() const { return m_state; }
    unsigned char getNextState() const { return m_nextState; }
    float getRate() const { return m_rate; }
    void setFlags(unsigned char f) { m_flags = f; }
    void setState(unsigned char s) { m_state = s; }
    void setTimer(float f) { m_timer = f; }
    void setNextState(unsigned char s) { m_nextState = s; }
    void setRate(float f) { m_rate = f; }
    void clrTargets()
    {
        m_flags = 0;
        m_numTargets = 0;
    }
    VehicleAccelTarget &addTarget() { return m_targets[m_numTargets++]; }
    void addTargetVehicle(float d, float s, float f)
    {
        m_flags |= 1;
        addTarget().set(d, s, f, 0);
    }
    void addTargetLaneEnd(float d, float s, float f)
    {
        m_flags |= 2;
        addTarget().set(d, s, f, 1);
    }
    void addTargetBend(float d, float s, float f)
    {
        m_flags |= 4;
        addTarget().set(d, s, f, 2);
    }
    void addTargetPlayer(float d, float s, float f)
    {
        m_flags |= 0x10;
        addTarget().set(d, s, f, 4);
    }
    void addTargetActor(float d, float s, float f)
    {
        m_flags |= 0x20;
        addTarget().set(d, s, f, 5);
    }
    void addTargetReaction(float d, float s, float f)
    {
        m_flags |= 0x40;
        addTarget().set(d, s, f, 6);
    }
    void addTargetLanestop(float d, float s, float f)
    {
        m_flags |= 0x80;
        addTarget().set(d, s, f, 7);
    }
    void clrCombinedTargetTypes()
    {
        for (unsigned int i = 0; i < 8; i++) m_combined[i] = 0;
    }
    VehicleAccelTarget *getCombinedTargetType(unsigned int t) const { return m_combined[t]; }
    void setCombinedTargetType(unsigned int t, VehicleAccelTarget *p) { m_combined[t] = p; }
    VehicleAccelTarget *getTargetLaneEnd() const { return m_combined[1]; }
    VehicleAccelTarget *getTargetBend() const { return m_combined[2]; }
    float getTargetDist() const { return m_targetDist; }
    float getTargetSpeed() const { return m_targetSpeed; }
    void clrNextState() { m_nextState = 0xff; }
    signed char isValidTarget() const { return m_flags; }
    signed char isValidNextState() const { return m_nextState != 0xff; }
    u8 m_flags;                 // 0x04
    u8 m_state;                 // 0x05
    u8 m_nextState;             // 0x06
    u8 m_unk7;                  // 0x07 TODO
    u8 m_numTargets;            // 0x08
    float m_timer;              // 0x0c
    float m_unk10;              // 0x10 TODO
    float m_rate;               // 0x14
    float m_targetDist;         // 0x18
    float m_targetSpeed;        // 0x1c
    EXFixedArray<VehicleAccelTarget, 6> m_targets;      // 0x20
    EXFixedArray<VehicleAccelTarget *, 8> m_combined;   // 0x80 per target type
    float m_fa0;                // 0xa0 TODO name
    float m_fa4;                // 0xa4 TODO name
    u32 m_ua8;                  // 0xa8 TODO
    float m_fac;                // 0xac TODO name (resting height)
    float m_fb0;                // 0xb0 TODO name
    u32 m_ub4;                  // 0xb4 TODO name
};

// Reaction to another vehicle (VehicleMove+0x460). sizeof 0x14.
class VehicleReaction {
public:
    VehicleReaction() {}
    void reset()
    {
        m_pReactingTo = 0;
        m_mode = 0;
    }
    void reset(Vehicle *pVehicle)
    {
        m_pVehicle = pVehicle;
        reset();
    }
    void update();
    void initMode(unsigned char, float, Vehicle *, VehicleInLane *);
    unsigned char getMode() const { return m_mode; }
    Vehicle *getReactingTo() const { return m_pReactingTo; }
    void setMode(unsigned char m) { m_mode = m; }
    void setReactingTo(Vehicle *p) { m_pReactingTo = p; }
    u8 m_unk0[8];               // 0x00 TODO
    Vehicle *m_pVehicle;        // 0x08
    u8 m_mode;                  // 0x0c
    Vehicle *m_pReactingTo;     // 0x10
};

// Lane change in progress (VehicleMove+0x2cc). sizeof 0x4c TODO.
class VehicleLaneChange {
public:
    ~VehicleLaneChange() {}
    u8 m_bDirChange;            // 0x00 TODO name (lane direction differs)
    float m_len;                // 0x04 TODO name
    float m_startAngle;         // 0x08
    float m_angle;              // 0x0c TODO name
    float m_angleOffset;        // 0x10 TODO name
    EXVector m_destPos;         // 0x14 TODO name
    EXVector m_dir;             // 0x24 TODO name
    TrafficLocation m_loc;      // 0x34 TODO name
};

// Vehicle movement / AI driving (VehicleMove.obj). sizeof 0x474. TODO: most of the layout.
class VehicleMove : public VehicleSubClass {
public:
    ~VehicleMove() {}
    void reset(Vehicle *);
    TrafficLaneDest *pickAmbientLaneEndDest();
    signed char areDestCrossedLanesClear(TrafficLaneDest *);
    void initHeadToNewRoad(unsigned int, EXVector3 &);
    void addLaneSegBlock(TrafficLaneSeg *);
    void unblockLaneSegBlocks();
    float calcSpeedForTurn(float);
    signed char checkForValidLaneChange(TrafficLocation &, Vehicle *, signed char);
    void ThrowGrenade();
    void setTurnDestLink(TrafficPathLink *);
    TrafficLaneDest *pickLaneEndDest();
    void initTurn(float, TrafficPathLink *, float);
    signed char updateTurn();
    signed char checkIntersectionDestClear(TrafficIntersection *, TrafficLaneDest *);
    signed char initLaneChange(TrafficLocation &);
    signed char updateLaneChange();
    float handlePlayerProximity();
    signed char handleActorProximity(Actor **, float &);
    void emergencyFindReactors(TrafficLane *, unsigned int);
    VehicleReaction *isReactee(Vehicle *);
    signed char buildRouteFromPath(unsigned int);
    signed char nextRouteStep();
    void initHeadToEndDest(TrafficLaneDest *);
    signed char doContinueChecks();
    signed char updatePos();
    void emergencyVehicleBroadcast();
    void updateRoute();
    void continueRoute();
    void update();
    float getSpeed() const { return m_speed; }
    TrafficBoxJunction *getInBoxJunction() const { return m_pInBoxJunction; }
    float getFakeSteerAngle() const { return m_fakeSteerAngle; }
    unsigned short checkFlagBit(unsigned short b) const { return m_flags & b; }
    void setInBoxJunction(TrafficBoxJunction *p) { m_pInBoxJunction = p; }
    void setRailsAngle(float f) { m_railsAngle = f; }
    void setSubstateTimer(float f) { m_substateTimer = f; }
    void setSubstate(unsigned char s)
    {
        m_substate = s;
        m_substateTimer = 0.0f;
    }
    TrafficLaneDest *getLaneDest() { return m_pLaneDest; }
    void setState(unsigned char s)
    {
        m_state = s;
        setSubstate(0);
    }
    void setMaxSpeed(unsigned short s) { m_maxSpeed = s; }
    void setThrottle(float f)
    {
        m_throttle = f;
        m_throttle2 = f;
    }
    signed char isLaneBlockedAtSeg(unsigned char s) const { return m_blockingLaneSeg == s; }
    void cbLaneSegUnblocked() { setState(1); }
    unsigned char getSubstate() const { return m_substate; }
    float getSubstateTimer() const { return m_substateTimer; }
    unsigned char getNavMode() const { return m_navMode; }
    unsigned char getState() const { return m_state; }
    unsigned short getToLink() const { return m_toLink; }
    float getTimer() const { return m_timer; }
    TrafficIntersection *getAtIntersection() const { return m_pAtIntersection; }
    float getLinkLengthXZ() const { return m_linkLengthXZ; }
    unsigned short getFlags() const { return m_flags; }
    float getModeTimer() const { return m_modeTimer; }
    VehicleMoveContinueChecks &getContinueChecks() { return m_continueChecks; }
    TrafficLaneSeg *getLaneSegBlock(unsigned int i) { return m_laneSegBlocks[i]; }
    unsigned char getNumLaneSegBlocks() const { return m_numLaneSegBlocks; }
    unsigned short getMaxSpeed() const { return m_maxSpeed; }
    float getStoppingDist() const { return m_stoppingDist; }
    float getTurnAngle() const { return m_turnAngle; }
    TrafficPathLink *getTurnDestLink() const { return TrGetPathLink(m_turnDestLink); }
    float getStartFakeSteerAngle() const { return m_startFakeSteerAngle; }
    float getStartTurnAngle() const { return m_startTurnAngle; }
    float getRailsAngle() const { return m_railsAngle; }
    float getThrottle() const { return m_throttle; }
    void setNavMode(unsigned char m) { m_navMode = m; }
    void setSpeed(float f) { m_speed = f; }
    void setToLink(unsigned char l) { m_toLink = l; }
    void setTimer(float f) { m_timer = f; }
    void setAtIntersection(TrafficIntersection *p) { m_pAtIntersection = p; }
    void setSlopeX(float f) { m_slopeX = f; }
    void setSlopeZ(float f) { m_slopeZ = f; }
    void setLinkLengthXZ(float f) { m_linkLengthXZ = f; }
    void setFlags(unsigned short f) { m_flags = f; }
    void setFlagBit(unsigned short b) { m_flags |= b; }
    void setBlockingIntersection(TrafficIntersection *p) { m_pBlockingIntersection = p; }
    void setBlockingLaneSeg(unsigned char s) { m_blockingLaneSeg = s; }
    void setLaneDest(TrafficLaneDest *p) { m_pLaneDest = p; }
    void setNumLaneSegBlocks(unsigned char n) { m_numLaneSegBlocks = n; }
    void setStoppingDist(unsigned char d) { m_stoppingDist = d; }
    void setTurnAngle(float f) { m_turnAngle = f; }
    void setTurnRadius(float f) { m_turnRadius = f; }
    void setTurnHorzOffset(float f) { m_turnHorzOffset = f; }
    void setFakeSteerAngle(float f) { m_fakeSteerAngle = f; }
    void setStartFakeSteerAngle(float f) { m_startFakeSteerAngle = f; }
    void setStartTurnAngle(float f) { m_startTurnAngle = f; }
    void setPendingIntersection(TrafficIntersection *p) { m_pPendingIntersection = p; }
    void setFromIntersection(TrafficIntersection *p) { m_pFromIntersection = p; }
    void setRouteLaneEndDest(unsigned char d) { m_routeLaneEndDest = d; }
    void clrFlagBit(unsigned short b) { m_flags &= ~b; }
    void clrTimer() { m_timer = 0.0f; }
    void clrModeTimer() { m_modeTimer = 0.0f; }
    void clrLaneSegBlock(unsigned int i) { m_laneSegBlocks[i] = 0; }
    void incrModeTimer() { m_modeTimer = frameLen + m_modeTimer; }
    void incrSubstateTimer() { m_substateTimer = frameLen + m_substateTimer; }
    float getTurnRadius() const { return m_turnRadius; }
    float getTurnHorzOffset() const { return m_turnHorzOffset; }
    void initRoute(signed char *);
    TrafficPathNode *getAtNode();         // inline in VehicleMove.cpp (needs TrafficSystem)
    TrafficPathNode *getTurnBendNode();   // inline in VehicleMove.cpp
    void setAtNode(TrafficPathNode *p) { m_atNode.set(p); }
    void setTurnBendNode(TrafficPathNode *p) { m_turnBendNode.set(p); }
    static unsigned char getNextState() { return nextState; }
    static signed char isNextStateImmediate() { return bNextStateNow; }
    static signed char isNewState() { return bNewState; }
    void clrNextState()
    {
        nextState = 0xff;
        bNextStateNow = 1;
    }
    void setNextState(unsigned char s)
    {
        nextState = s;
        clrFlagBit(2);
        bNextStateNow = 1;
    }
    void setNextState(unsigned char s, float t)
    {
        nextState = s;
        setFlagBit(2);
        setTimer(t);
        bNextStateNow = 1;
    }
    void clrNewState() { bNewState = 0; }
    void setNewState() { bNewState = 1; }

    u8 m_navMode;                           // 0x04
    u8 m_state;                             // 0x05
    u8 m_substate;                          // 0x06
    u8 m_toLink;                            // 0x07
    u8 m_stoppingDist;                      // 0x08
    u8 m_routeLaneEndDest;                  // 0x09
    u16 m_maxSpeed;                         // 0x0a
    u16 m_flags;                            // 0x0c
    TrafficIntersection *m_pAtIntersection;         // 0x10
    TrafficIntersection *m_pPendingIntersection;    // 0x14
    TrafficIntersection *m_pFromIntersection;       // 0x18
    TrafficSafePathNodePtr m_atNode;        // 0x1c
    TrafficBoxJunction *m_pInBoxJunction;   // 0x20
    TrafficIntersection *m_pBlockingIntersection;   // 0x24
    TrafficLaneDest *m_pLaneDest;           // 0x28
    u16 m_turnDestLink;                     // 0x2c
    float m_speed;                          // 0x30
    float m_timer;                          // 0x34
    float m_slopeX;                         // 0x38
    float m_slopeZ;                         // 0x3c
    float m_linkLengthXZ;                   // 0x40
    float m_modeTimer;                      // 0x44
    float m_substateTimer;                  // 0x48
    u8 m_unk4c[0x58 - 0x4c];                // 0x4c TODO
    float m_turnAngle;                      // 0x58
    float m_turnRadius;                     // 0x5c
    float m_turnHorzOffset;                 // 0x60
    TrafficSafePathNodePtr m_turnBendNode;  // 0x64
    float m_fakeSteerAngle;                 // 0x68
    float m_startFakeSteerAngle;            // 0x6c
    float m_startTurnAngle;                 // 0x70
    float m_railsAngle;                     // 0x74
    float m_throttle;                       // 0x78
    VehicleMoveContinueChecks m_continueChecks;     // 0x7c
    VehicleAccel m_accel;                   // 0x90
    VehiclePhys m_phys;                     // 0x148
    u8 m_blockingLaneSeg;                   // 0x28c
    u8 m_numLaneSegBlocks;                  // 0x28d
    EXFixedArray<TrafficLaneSeg *, 8> m_laneSegBlocks;  // 0x290
    float m_throttle2;                      // 0x2b0
    TrafficLocation m_loc0;                 // 0x2b4 TODO name
    VehicleLaneChange m_laneChange;         // 0x2cc
    TrafficLane *m_pRouteLane;              // 0x318
    signed char *m_pRoute;                  // 0x31c
    float m_f320;                           // 0x320 TODO name
    u32 m_u324;                             // 0x324 TODO name
    EXFixedArray<signed char, 256> m_routeBuf;  // 0x328 TODO name
    u8 m_routeArg;                          // 0x428 TODO name
    s8 m_routeCmd;                          // 0x429 TODO name
    u8 m_routeArgs[3];                      // 0x42a TODO name
    u8 m_unk42d[3];                         // 0x42d TODO
    float m_f430;                           // 0x430 TODO name
    float m_playerHitTime;                  // 0x434
    u8 m_u438;                              // 0x438 TODO name
    EXFixedArray<EXDList, 3> m_lists;       // 0x43c TODO name
    u32 m_listCounts[3];                    // 0x454 TODO name
    VehicleReaction m_reaction;             // 0x460
protected:
    static unsigned char nextState;
    static signed char bNextStateNow;
    static signed char bNewState;
};
#endif
