#ifndef VEHICLESFX_H
#define VEHICLESFX_H
#include "types.h"
#include "VehicleSubClass.h"
#include "EXSoundTagStandIn.h"

extern float frameTime;
class EXRandClass;

// Vehicle sound effects (VehicleSound.obj). sizeof 0x24.
class VehicleSfx : public VehicleSubClass {
public:
    enum { BRAKE = 0, AWAY = 1, ENGINE = 2, STOP = 3, RADIO = 4, HORN = 5, IDLE = 6, NUM_TYPES = 7, HASH = 100 };
    VehicleSfx() {}
    ~VehicleSfx() {}
    void playRadio(int i);
    void reset(Vehicle *pVehicle)
    {
        for (int i = 7; i != 0; i--) checkTypePlaying(IDLE, 1, 1);
        m_tag.RemoveAll(0);
        m_pVehicle = pVehicle;
        m_hornStart = -10.0f;
        m_pHorn = 0;
        m_pLastHorn = 0;
        m_bAway = 0;
    }
    void playIdle()
    {
        if (!checkTypePlaying(IDLE, 0, 0)) play(IDLE, 0);
    }
    void stop(unsigned char type, signed char bAll) { checkTypePlaying(type, 1, bAll); }
    void playAway()
    {
        if (checkTypePlaying(AWAY, 0, 0)) {
            if (m_bAway) return;
            if (checkTypePlaying(ENGINE, 0, 0)) return;
            m_awayTime = frameTime;
            m_bAway = 1;
            return;
        }
        stop(STOP, 0);
        play(AWAY, 0);
        m_awayTime = frameTime;
        m_bAway = 1;
    }
    void playStop()
    {
        if (checkTypePlaying(STOP, 0, 0)) return;
        stop(AWAY, 0);
        play(STOP, 0);
        stop(ENGINE, 0);
        playIdle();
    }
    void playBrake()
    {
        if (!checkTypePlaying(BRAKE, 0, 0)) play(BRAKE, 0);
    }
    void stopHorn(signed char bAll)
    {
        m_pHorn = 0;
        stop(HORN, bAll);
    }
    static unsigned int *getArrayPtr(unsigned char type);
    signed char checkTypePlaying(unsigned char type, signed char bStop, signed char bAll);
    void playHorn();
    static unsigned char checkType(unsigned int hash);
    void play(unsigned char type, unsigned int index);
    void updateHorn();
    void playEngine()
    {
        if (!checkTypePlaying(ENGINE, 0, 0)) play(ENGINE, 0);
    }
    void updateAway();
    void update();

    u8 m_unk4;              // 0x04
    s8 m_bAway;             // 0x05
    float m_hornStart;      // 0x08
    float m_hornTime;       // 0x0c
    float *m_pHorn;         // 0x10 current horn pattern (lengths, 0-terminated; negative = pause)
    float *m_pLastHorn;     // 0x14
    float m_awayTime;       // 0x18
    EXSoundTag m_tag;       // 0x1c
};
#endif
