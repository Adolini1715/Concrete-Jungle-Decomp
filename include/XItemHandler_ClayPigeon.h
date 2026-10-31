#ifndef XITEMHANDLER_CLAYPIGEON_H
#define XITEMHANDLER_CLAYPIGEON_H
#include "EXItemHandler.h"

class XItem;

// Clay pigeon spawner for the target-practice mission (FX_ClayPigeon.obj). sizeof 0x54.
class XItemHandler_ClayPigeonGenerator : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_ClayPigeonGenerator)
public:
    virtual ~XItemHandler_ClayPigeonGenerator();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    XItemHandler_ClayPigeonGenerator() {}
    static XItemHandler_ClayPigeonGenerator *CreateObject();
    void ProcessPigeonDestroyed();
    s8 m_bFirstUpdate;          // 0x08
    int m_ObjectiveIds[8];      // 0x0c mission item ids (Mission_GetMissionItem)
    XItem *m_pObjectives[8];    // 0x2c resolved on the first update
    volatile int m_Remaining;   // 0x4c pigeons left (starts at 10); volatile gives the load/dec/store
    s8 m_bPigeonActive;         // 0x50
};

// One clay pigeon circling its generator (FX_ClayPigeon.obj). sizeof 0x18.
class XItemHandler_ClayPigeon : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_ClayPigeon)
public:
    virtual ~XItemHandler_ClayPigeon();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    XItemHandler_ClayPigeon() {}
    static XItemHandler_ClayPigeon *CreateObject();
    void ApplyDamage(float damage);
    float m_Time;               // 0x08
    float m_Radius;             // 0x0c
    float m_Angle;              // 0x10
    XItem *m_pGenerator;        // 0x14
};
#endif
