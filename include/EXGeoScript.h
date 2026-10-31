#ifndef EXGEOSCRIPT_H
#define EXGEOSCRIPT_H
#include "EXGeoCommonObject.h"
#include "EXBounds.h"
#include "EXRelPtr.h"

#include "EXVector.h"
#include "EXQuaternion.h"

// Decoded placement of a script command (DecodeScriptCmd). TODO the rest of the layout.
struct EXScriptDecodeStruct {
    EXVector m_pos;         // +0x00
    EXQuaternion m_rot;     // +0x10
    EXVector m_scale;       // +0x20 TODO
};

// Script command as stored in the file: type, byte size, payload. Type 0x12 ends the list.
struct GEOSCRIPT_CMD {
    u8 m_Type;      // +0
    u8 m_Size;      // +1
    u16 m_02;
    u32 m_04;
    u16 m_Thread;   // +8 thread / controller index
    u16 m_0a;
    u32 m_0c;
    u32 m_10;
    u32 m_14;
    u32 m_18;
    u32 m_1c;
    u32 m_20;
};

// Geo script: timeline of commands on threads (sizeof 0x64).
class EXGeoScript : public EXGeoCommonObject {
    EX_DECLARE_DYNAMIC(EXGeoScript)
    EX_DECLARE_GEOOBJECT(EXGeoScript)
public:
    virtual EXBoundsBox &GetBoundsBox();
    EXGeoScript() {}
    unsigned int NumFrames() const { return m_NumFrames; }
    unsigned int NumThreads() const { return m_NumThreads; }
    unsigned int NumTimeJumps() const { return m_NumTimeJumps; }
    unsigned int ScriptFlags() const { return m_ScriptFlags; }
    float FrameRate() const { return m_FrameRate; }
    unsigned int UsedControllerTypes() const { return m_UsedControllerTypes; }
    unsigned short ThreadControllerChannelCount() const { return m_ThreadControllerChannelCount; }
    ex::EXRELOFFSET32 *ThreadControllerTable() const { return m_ThreadControllerTable; }
    unsigned short *ThreadInfo() const { return m_ThreadInfo; }
    GEOSCRIPT_CMD *Data(int i) const { return (GEOSCRIPT_CMD *)&m_Data[i]; }
    signed char GetFirstCmdObject(unsigned int type, unsigned int &a, unsigned int &b) const;
    unsigned int DecodeScriptCmd(const GEOSCRIPT_CMD *pCmd, EXScriptDecodeStruct *pOut, int start, int count) const;

    u32 m_NumFrames;                // 0x04
    u8 m_NumThreads;                // 0x08
    u8 m_NumTimeJumps;              // 0x09
    u16 m_ScriptFlags;              // 0x0a
    float m_FrameRate;              // 0x0c
    EXBoundsBox m_BoundsBox;        // 0x10
    u32 m_30;
    ex::EXRelPtr<ex::EXRELOFFSET32> m_ThreadControllerTable;  // 0x34
    ex::EXRelPtr<u16> m_ThreadInfo;                           // 0x38
    u16 m_3c;
    u16 m_ThreadControllerChannelCount;                       // 0x3e
    u32 m_UsedControllerTypes;                                // 0x40
    u8 m_Data[0x20];                                          // 0x44 command stream
};
#endif
