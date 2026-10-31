#ifndef EVD_H
#define EVD_H
#include "types.h"
#include "EXVector.h"

// TODO stand-in: crime type flags (owner unknown; only the enum name is needed for mangling).
struct CrimeType {
    enum Flags { NONE = 0, VANDALISM = 1 };  // TODO names (1: graffiti, STATE_FS_Graffiti)
};

// One emergency call (sizeof 0x24).
class EVDCall {
public:
    EVDCall() {}
    s8 m_bActive;           // 0x00
    s8 m_bDispatched;       // 0x01
    u32 m_Time;             // 0x04 game window time stamp
    EXVector m_Pos;         // 0x08
    int m_Unk18;            // 0x18
    int m_POI;              // 0x1c point of interest for POI_VendorGetID/POI_CustomerGetID
    CrimeType::Flags m_Crime; // 0x20
};

// Emergency vehicle dispatch (Fx_evd.obj): a ring of four calls. sizeof 0x94.
class EVD {
public:
    ~EVD() { m_pInstance = 0; }
    void Initialise();
    void Update();
    void Dial999(EXVector pos, int poi, int a, CrimeType::Flags crime);
    static EVD *Instance(signed char create);
private:
    EVD();
    static EVD *m_pInstance;
public:
    EVDCall m_Calls[4];     // 0x00
    int m_Next;             // 0x90
};
#endif
