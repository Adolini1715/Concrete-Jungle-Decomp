#ifndef EXGEOIDENTIFIER_H
#define EXGEOIDENTIFIER_H
#include "types.h"
#include "RGBA.h"

// TODO stand-in: zone identifier record of a map (geo area). TODO layout: only the fields read by
// EXItemEnv.obj (RenderMaps, RenderSkys) are named.
struct EXGeoIdentifier {
    float m_FogNear;    // +0x00 the zone's fog (EXMapZoneInfoTable::Initialise copies it into the zone info)
    float m_FogFar;     // +0x04
    float m_FogMin;     // +0x08
    float m_FogMax;     // +0x0c
    u32 m_FogEnabled;   // +0x10
    u8 m_14[0xc];
    u32 m_Flags;        // +0x20 1: sky follows the camera in x/z; 0x10000: sets the env fog value at +0x28 (RenderMaps)
    u32 m_24;
    u32 m_28;           // +0x28
    ex::RGBA m_FogColor;    // +0x2c
    u8 m_30[0x38 - 0x30];
    int m_SkyIndex;     // +0x38 index into the zone table's sky array, < 0: none
    float m_SkyHeight;  // +0x3c sky y when flag 1 makes the sky follow the camera
    int m_LightSet;     // +0x40 index into the zone table's placement-group light sets, < 0: none (EXGeoMap::DrawZone)
};
#endif
