#ifndef EXLIGHTMANAGER_H
#define EXLIGHTMANAGER_H
#include "EXDynLight.h"
#include "EXArray.h"
#include "EXBit128.h"

class EXGeoMap;
struct EXBoundsBox;

// Dynamic light pool and map-zone light table (EXLightManager.obj), sizeof 0x10c8.
// Lights live in up to 8 blocks of 16; bit n of m_zoneLights[zone] = light n touches that zone.
class EXLightManager {
public:
    virtual ~EXLightManager();
    EXLightManager();
    // Shape verified against the EXItemEnv.obj copy (item agent): plain vector copy, then w.
    void SetDynLightRGBMult(const EXVector &v)
    {
        EXMaths::vec_xyzw t = (const EXMaths::vec_xyzw &)v;
        (EXMaths::vec_xyzw &)m_rgbMult = t;
        m_rgbMult.w = 1.0f;
    }
    void EnableDynLightRGBMult(s8 b) { m_bRGBMult = b; }
    u32 MapZoneLightCount() const { return m_nZoneLights; }
    EXDynLight *Light(u32 i) { return &m_pLights[i >> 4][i & 0xf]; }
    void RemoveDynLight(EXDynLight *pLight) const;
    void Debug_DrawDynLightMarkers();
    EXDynLight *NewDynLight();
    void RemoveAllDynLights();
    void UpdateLightInMapZoneTable(const EXGeoMap *pMap);
    void SetupMapZoneLights(u32 zone);
    void SetRenderEnvLights(const EXBoundsBox &box);

    EXVector m_rgbMult;                         // +0x04
    s8 m_bRGBMult;                              // +0x14
    EXFixedArray<EXDynLight *, 8> m_pLights;    // +0x18
    u32 m_nLights;                              // +0x38
    EXFixedArray<ex::EXBit128, 256> m_zoneTable; // +0x3c
    u32 m_maxZone;                              // +0x103c
    u32 m_curZone;                              // +0x1040
    EXFixedArray<u8, 128> m_zoneLights;         // +0x1044
    u32 m_nZoneLights;                          // +0x10c4
};
#endif
