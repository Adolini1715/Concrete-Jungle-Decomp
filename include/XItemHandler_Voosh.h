#ifndef XITEMHANDLER_VOOSH_H
#define XITEMHANDLER_VOOSH_H
#include "EXItemHandler.h"
#include "EXVector.h"

class XItem;
class XItemList;
struct EXDatumTemp;

// One trail point (sizeof 0x28).
struct _VOOSH {
    _VOOSH() {}
    EXVector m_Pos;         // 0x00
    EXVector m_Vel;         // 0x10
    float m_Unk20;          // 0x20 texture coordinate along the trail
    float m_Life;           // 0x24
};

// Swoosh trail following an actor's datum (Voosh.obj). sizeof 0x51c; 32-point ring.
class XItemHandler_Voosh : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_Voosh)
public:
    virtual ~XItemHandler_Voosh();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    XItemHandler_Voosh() {}
    static XItemHandler_Voosh *CreateObject();
    static XItem *Voosh_Create(int actorId, unsigned int datum, int life);
    void Render();
protected:
    void Add(EXDatumTemp &d);
    void Update();
public:
    int m_Life;             // 0x08 deleted when negative
    int m_ActorId;          // 0x0c >0 actor id, <0 the player, 0 none
    unsigned int m_Datum;   // 0x10 animator datum the trail follows
    int m_Head;             // 0x14 next ring slot
    int m_Count;            // 0x18
    _VOOSH m_Points[32];    // 0x1c
};

extern XItemList gVooshList;
void Draw_Voosh();
#endif
