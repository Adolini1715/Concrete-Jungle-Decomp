#ifndef VEHICLE_H
#define VEHICLE_H
#include "types.h"
#include "EXBaseItem.h"
#include "EXRuntimeClass.h"
#include "EXVector.h"
#include "EXArray.h"
#include "EXTexSwapInfo.h"
#include "VehicleTypes.h"
#include "VehiclePos.h"
#include "VehicleMove.h"
#include "VehicleSfx.h"
#include "Traffic.h"
#include <string.h>

class EXItem;
class EXItemAnimator_Entity;
class EXItemAnimator_Script;
class XItemHandler_Objective;
class EXTriangle;
class EXCapsule;
class EXLineSeg;
class FEObject;
struct ScanInfo;

// Light/indicator state block of a vehicle (0x80 bytes at Vehicle+0x73c). TODO names.
struct VehicleLights {
    VehicleLights()
    {
        memset(m_data, 0, sizeof(m_data));
        m_colour.Set(0.5f, 0.5f, 0.5f, 1.0f);
        m_70 = 0.5f;
        m_74 = 0.08f;
        m_78 = 0;
        m_7c = 0.0f;
    }
    u32 m_data[24];     // 0x00
    EXVector m_colour;  // 0x60 TODO name
    float m_70, m_74;
    u32 m_78;
    float m_7c;
};

// A traffic vehicle (Vehicle.obj). sizeof 0x7fc. Runtime-class root.
class Vehicle : public EXBaseItem {
    EX_DECLARE_DYNAMIC(Vehicle)
public:
    Vehicle();
    virtual ~Vehicle();
    static Vehicle *CreateObject();
    EXItem *GetItem() const { return m_pItem; }
    const EXVector &getPos() const;
    VehicleMove &getMove() { return m_move; }
    VehiclePos &getCurrPos() { return m_currPos; }
    VehicleTypeBase *getTypeObject() const { return m_pTypeObject; }
    const signed char isInit() const
    {
        if (m_pTypeObject && m_pItem) return 1;
        return 0;
    }
    unsigned char getIndex() const { return m_index; }
    void setIndex(unsigned char i) { m_index = i; }
    signed char isUpdated() const;
    void SetItem(EXItem *p) { m_pItem = p; }
    unsigned char getFlags() const { return m_flags; }
    void setFlag(unsigned char b) { m_flags |= b; }
    void clrFlag(unsigned char b) { m_flags &= ~b; }
    const EXVector3 &currRot_get() const { return m_currRot; }
    void currRot_get(EXVector &v)
    {
        v.x = m_currRot.x;
        v.y = m_currRot.y;
        v.z = m_currRot.z;
        v.w = 0.0f;
    }
    void currRot_set(EXVector3 &v)
    {
        m_currRot.x = v.x;
        m_currRot.y = v.y;
        m_currRot.z = v.z;
    }
    void UpdateAttackedCountDown()
    {
        if (m_attackedCountDown != 0.0f) {
            m_attackedCountDown -= frameLen;
            if (m_attackedCountDown <= 0.0f) {
                m_attackedCountDown = 0.0f;
                m_attackedBy = -1;
            }
        }
    }
    void currRot_get(EXVector3 &v)
    {
        v.x = m_currRot.x;
        v.y = m_currRot.y;
        v.z = m_currRot.z;
    }
    void removeItemFromWorld();
    void freeAllocs();
    void landedOn();
    void objectiveInit(XItemHandler_Objective *, unsigned int);
    void objectiveUpdate();
    Vehicle *GetNext();
    void initIndicator(unsigned int, float);
    void setItemPosAndRot();
    void updateTransparency();
    void DeleteMuzzleScript();
    signed char CanSeePoint(EXVector &, float);
    signed char CheckSeePlayer();
    int CheckCollision(const EXTriangle &, float, EXVector &);
    int CheckCollision(const EXCapsule &, EXVector &);
    int CheckCollision(const EXVector *, float, EXVector &);
    static Vehicle *GetFirst();
    Vehicle *GetNext(const EXVector &, float);
    signed char ApplyAttackDamage(unsigned int, float);
    void initHealthIndicator();
    void updateHealthIndicator();
    void setAttachmentRender(int, signed char);
    signed char isOnScreen();
    static signed char isOnScreen(unsigned int, EXVector &);
    static Vehicle *GetFromIndexOnwards(unsigned int &, signed char);
    static signed char ScanCallback(signed char, FEObject *, ScanInfo *);
    void reset();
    void remove();
    void PlayMuzzleScript(int);
    void UpdateMuzzleScript();
    int CheckCollision(const EXLineSeg &, float, EXVector &);
    static Vehicle *GetFirst(const EXVector &, float);
    void updateLights();
    void HandleVisionModeTextureSwaps(int);
    signed char GetScanInfo(int, ScanInfo *);
    signed char OnSubFileLoaded();
    void update();
    signed char create(unsigned char, TrafficLocation *);

    u8 m_index;                                 // 0x1c
    u8 m_flags;                                 // 0x1d
    VehicleTypeBase *m_pTypeObject;             // 0x20
    VehiclePos m_currPos;                       // 0x24
    EXItem *m_pItem;                            // 0x214
    u32 m_updateCount;                          // 0x218
    EXFixedArray<EXItemAnimator_Entity *, 17> m_attachments;  // 0x21c
    VehicleMove m_move;                         // 0x260
    VehicleSfx m_sfx;                           // 0x6d4
    u8 m_unk6f8;                                // 0x6f8 TODO
    XItemHandler_Objective *m_pObjective;       // 0x6fc
    u32 m_objectiveHash;                        // 0x700
    u32 m_objectiveState;                       // 0x704
    u8 m_unk708;                                // 0x708 TODO
    u32 m_vehFlags;                             // 0x70c TODO name (bit 2: objective target)
    float m_health;                             // 0x710
    float m_maxHealth;                          // 0x714
    int m_attackedBy;                           // 0x718
    float m_attackedCountDown;                  // 0x71c
    float m_alpha;                              // 0x720
    float m_alphaRate;                          // 0x724
    EXItem *m_pHealthIndicator;                 // 0x728
    u8 m_bFading;                               // 0x72c
    u32 m_unk730;                               // 0x730 TODO
    float m_indicatorTimer;                     // 0x734
    float m_indicatorTime;                      // 0x738
    VehicleLights m_lights;                     // 0x73c TODO name
    EXVector3 m_lightColour;                    // 0x7bc TODO name
    float m_unk7c8;                             // 0x7c8 TODO
    float m_unk7cc;                             // 0x7cc TODO
    EXItem *m_pExplosion;                       // 0x7d0
    float m_unk7d4;                             // 0x7d4 TODO
    EXTexSwapInfo m_texSwap;                    // 0x7d8
    EXVector3 m_currRot;                        // 0x7ec
    EXItemAnimator_Script *m_pMuzzleScript;     // 0x7f8
};
#endif
