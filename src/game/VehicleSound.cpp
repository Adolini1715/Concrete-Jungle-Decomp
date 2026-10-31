#include "FEText.h"
#include "Vehicle.h"
#include "EXRand.h"

// TODO stand-in: EXSoundHandle belongs to the audio area (EXBaseSoundManager.obj). TODO layout.
class EXSoundHandle {
public:
    EXSoundHandle *m_pPrev;     // 0x00
    EXSoundHandle *m_pNext;     // 0x04
    u8 m_Unk8[0x1c - 0x08];
    unsigned int m_Hash;        // 0x1c
    u8 m_Unk20[0x2a - 0x20];
    s8 m_bLooping;              // 0x2a TODO name
};

// Sound hashes per effect type (3 variants by car sound type; 17 radio stations).
static unsigned int s_brakeSfx[3] = { 0x1af002a7, 0x1af002a7, 0x1af002a7 };
static unsigned int s_awaySfx[3] = { 0x1af002ad, 0x1af002b1, 0x1af002b1 };
static unsigned int s_engineSfx[3] = { 0x1af002af, 0x1af002b3, 0x1af002b3 };
static unsigned int s_stopSfx[3] = { 0x1af002b0, 0x1af002b4, 0x1af002b4 };
static unsigned int s_hornSfx[3] = { 0x1af002b8, 0x1af002b9, 0x1af002ba };
static unsigned int s_idleSfx[3] = { 0x1af002ae, 0x1af002b2, 0x1af002b2 };
static unsigned int s_radioSfx[17] = {
    0x1af00304, 0x1af0030c, 0x1af0030d, 0x1af0030e, 0x1af0030f, 0x1af00310, 0x1af00311, 0x1af00312, 0x1af00313,
    0x1af00305, 0x1af00306, 0x1af00307, 0x1af00308, 0x1af00309, 0x1af0030a, 0x1af0030b, 0x1af00584
};

// Horn patterns: lengths in seconds, negative = pause, 0 ends.
static float s_horn0[] = { 1.936f, 0.0f };
static float s_horn1[] = { 2.611f, 0.0f };
static float s_horn2[] = { 3.429f, 0.0f };
static float s_horn3[] = { 0.89f, 2.093f, 0.0f };
static float s_horn4[] = { 0.536f, 2.464f, 0.0f };
static float s_horn5[] = { 0.81f, 0.79f, 0.694f, 2.67f, 0.0f };
static float s_horn6[] = { 2.22f, 0.317f, 1.551f, 1.184f, 0.0f };
static float s_horn7[] = { 1.751f, 0.668f, 0.25f, 1.134f, 1.719f, 0.601f, 2.069f, 0.0f };
static float s_horn8[] = { 1.892f, 0.414f, 3.195f, 0.0f };
static float s_horn9[] = { 3.932f, 0.742f, 2.508f, 5.121f, 1.217f, 4.003f, 0.231f, -1.582f, 1.667f, 0.0f };
static float s_horn10[] = { 1.389f, 2.918f, 3.415f, 2.292f, 0.586f, 2.514f, 2.921f, 0.0f };
static float s_horn11[] = { 1.316f, 1.931f, 0.533f, 0.45f, 4.185f, 3.384f, 1.734f, 0.0f };
static float s_horn12[] = { 2.719f, 2.551f, 0.533f, 2.11f, 0.287f, -0.2f, 3.913f, 0.0f };
static float s_horn13[] = { 2.173f, 4.317f, 2.724f, 0.303f, 2.174f, 1.179f, 2.091f, 0.0f };
static float s_horn14[] = { 4.425f, 1.699f, 0.401f, 3.731f, 1.078f, 1.433f, 3.12f, 0.0f };
static EXFixedArray<float *, 15> s_horns = { s_horn0, s_horn1, s_horn2, s_horn3, s_horn4, s_horn5, s_horn6,
    s_horn7, s_horn8, s_horn9, s_horn10, s_horn11, s_horn12, s_horn13, s_horn14 };

inline unsigned int *VehicleSfx::getArrayPtr(unsigned char type)
{
    switch (type) {
    case BRAKE: return s_brakeSfx;
    case AWAY: return s_awaySfx;
    case ENGINE: return s_engineSfx;
    case STOP: return s_stopSfx;
    case RADIO: return s_radioSfx;
    case HORN: return s_hornSfx;
    case IDLE: return s_idleSfx;
    }
    return 0;
}

signed char VehicleSfx::checkTypePlaying(unsigned char type, signed char bStop, signed char bAll)
{
    unsigned int *pHashes = getArrayPtr(type);
    if (!pHashes) return 0;
    EXSoundHandle *pNext;
    for (EXSoundHandle *p = m_tag.m_pHead; p; p = pNext) {
        unsigned int n = type == RADIO ? 17 : 3;
        pNext = p->m_pNext;
        unsigned int hash = p->m_Hash;
        for (unsigned int i = 0; i < n; i++) {
            if (pHashes[i] == hash) {
                if (!bStop) return 1;
                if (p->m_bLooping || bAll) m_tag.Remove(hash, 1);
                break;
            }
        }
    }
    return 0;
}

void VehicleSfx::playHorn()
{
    if (m_pHorn) return;
    if (checkTypePlaying(HORN, 0, 0)) return;
    for (unsigned int i = 0; i < 5; i++) {
        m_pHorn = s_horns[(int)(g_EXRandClass.Rand32() % 15)];
        if (m_pHorn != m_pLastHorn) break;
    }
    m_pLastHorn = m_pHorn;
    m_hornStart = frameTime;
    m_hornTime = 0.0f;
}

unsigned char VehicleSfx::checkType(unsigned int hash)
{
    for (unsigned int type = 0; type < NUM_TYPES; type++) {
        unsigned int *pHashes = getArrayPtr(type);
        unsigned int n = type == RADIO ? 17 : 3;
        for (unsigned int i = 0; i < n; i++) {
            if (pHashes[i] == hash) return type;
        }
    }
    return 0xff;
}

void VehicleSfx::play(unsigned char type, unsigned int index)
{
    if (checkTypePlaying(type, 0, 0)) return;
    signed char bTrack = 1;
    unsigned int hash = 0;
    switch (type) {
    case BRAKE:
    case AWAY:
    case STOP:
    case HORN:
        bTrack = 0;
    case ENGINE:
    case IDLE:
        index = m_pVehicle->getTypeObject()->getSfxCarType();
        break;
    case HASH:
        bTrack = 0;
        hash = index;
        break;
    }
    if (!hash) hash = getArrayPtr(type)[index];
    if (bTrack) m_tag.AddTrackingSound(hash, EXVector(m_pVehicle->getCurrPos().get(), 0.0f), 0.0f, 0.0f);
    else m_tag.StartSound(hash, EXVector(m_pVehicle->getCurrPos().get(), 0.0f), 0.0f, 0.0f, -1, 0);
}

void VehicleSfx::updateHorn()
{
    float *pHorn = m_pHorn;
    if (!pHorn) return;
    float dt = frameTime - (m_hornStart + m_hornTime);
    if (0.0f > dt) return;
    stopHorn(1);
    m_pHorn = pHorn;
    float len = *m_pHorn++;
    if (len == 0.0f) {
        m_pHorn = 0;
        return;
    }
    m_hornStart = frameTime;
    if (0.0f > len) {
        m_hornTime = -len - dt;
        return;
    }
    play(HORN, 0);
    m_hornTime = len - dt;
}

inline void VehicleSfx::updateAway()
{
    if (m_bAway && frameTime > m_awayTime + 1.0f) {
        stop(IDLE, 0);
        playEngine();
        m_bAway = 0;
    }
}

void VehicleSfx::update()
{
    updateAway();
    updateHorn();
    m_tag.UpdatePosition(EXVector(m_pVehicle->getCurrPos().get(), 0.0f));
}
