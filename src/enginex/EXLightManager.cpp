#include "EXLightManager.h"
#include "EXBaseRenderEnv.h"
#include "EXBaseDisplay.h"
#include "EXGeoMap.h"
#include "EXBounds.h"
#include "EXMemory.h"
#include <string.h>

#define kByteToUnit (1.0f / 255.0f)

void EXLightManager::RemoveDynLight(EXDynLight *pLight) const
{
    pLight->SetActive(0);
}

void EXLightManager::Debug_DrawDynLightMarkers()
{
}

void EXDynLight::SetPointLight(ex::XRGBA c, const EXVector &pos, float inner, float outer)
{
    m_color = c;
    EXMaths::vec_xyzw &col = m_colorF;
    col.x = c.r * kByteToUnit;
    col.y = c.g * kByteToUnit;
    col.z = c.b * kByteToUnit;
    col.w = c.a * kByteToUnit;
    m_pos = (const EXMaths::vec_xyzw)pos;
    m_dir.set_zero();
    m_innerRadius = inner;
    m_outerRadius = outer;
    m_directional = 0;
    m_changed = 1;
}

void EXDynLight::SetPointLight(ex::XRGBA c, float inner, float outer)
{
    m_color = c;
    EXMaths::vec_xyzw &col = m_colorF;
    col.x = c.r * kByteToUnit;
    col.y = c.g * kByteToUnit;
    col.z = c.b * kByteToUnit;
    col.w = c.a * kByteToUnit;
    m_pos.set_zero();
    m_dir.set_zero();
    m_innerRadius = inner;
    m_outerRadius = outer;
    m_directional = 0;
    m_changed = 1;
}

// NEAR: 20 bytes differ, retail reloads the inner radius from the stack after storing m_innerRadius.
void EXDynLight::SetDirPointLight(ex::XRGBA c, const EXVector &pos, const EXVector &dir, float inner, float outer)
{
    m_color = c;
    EXMaths::vec_xyzw &col = m_colorF;
    col.x = c.r * kByteToUnit;
    col.y = c.g * kByteToUnit;
    col.z = c.b * kByteToUnit;
    col.w = c.a * kByteToUnit;
    m_pos = (const EXMaths::vec_xyzw)pos;
    SetDir(dir);
    m_innerRadius = inner;
    m_outerRadius = inner;
    m_directional = 1;
    m_changed = 1;
}

// NEAR: 31 bytes differ, same inner-radius reload plus one shr/push ebx scheduling swap.
void EXDynLight::SetDirPointLight(ex::XRGBA c, const EXVector &dir, float inner, float outer)
{
    m_color = c;
    EXMaths::vec_xyzw &col = m_colorF;
    col.x = c.r * kByteToUnit;
    col.y = c.g * kByteToUnit;
    col.z = c.b * kByteToUnit;
    col.w = c.a * kByteToUnit;
    m_pos.set_zero();
    SetDir(dir);
    m_innerRadius = inner;
    m_outerRadius = inner;
    m_directional = 1;
    m_changed = 1;
}

void EXDynLight::UpdateCol()
{
    m_colorF.set(m_color.r * kByteToUnit, m_color.g * kByteToUnit, m_color.b * kByteToUnit, m_color.a * kByteToUnit);
}

// NEAR: 7 bytes differ, lea edi / mov ecx order before the zone-table rep stosd.
EXLightManager::EXLightManager()
{
    for (int i = 0; i < 8; i++) m_pLights[i] = 0;
    memset(&m_zoneTable, 0, sizeof(m_zoneTable));
    m_nLights = 0;
    m_maxZone = 0;
    m_curZone = 0xffff;
    m_rgbMult.set(1.0f, 1.0f, 1.0f, 1.0f);
    m_bRGBMult = 0;
}

EXDynLight *EXLightManager::NewDynLight()
{
    u32 n = m_nLights;
    u32 found = 0xffff;
    u32 block, slot;
    for (int i = n - 1; i >= 0; i--) {
        slot = i & 0xf;
        block = i >> 4;
        if (!m_pLights[block][slot].IsActive()) {
            found = i;
            break;
        }
    }
    if (found == 0xffff) {
        if (n >= 0x80) {
            slot = 0;
            block = 0;
        } else {
            block = n >> 4;
            slot = n & 0xf;
            if (!m_pLights[block]) {
                m_pLights[block] = (EXDynLight *)_EXAlloc(16 * sizeof(EXDynLight), 0);
                for (int j = 0; j < 16; j++) {
                    m_pLights[block][j].SetActive(0);
                    m_pLights[block][j].SetChangedFlag(0);
                }
            }
            m_nLights++;
        }
    }
    EXDynLight *pLight = &m_pLights[block][slot];
    pLight->m_affectsOneZone = 0;
    pLight->SetActive(1);
    return &m_pLights[block][slot];
}

void EXLightManager::RemoveAllDynLights()
{
    for (int i = 0; i < 8; i++) {
        if (m_pLights[i]) {
            EXFree(m_pLights[i]);
            m_pLights[i] = 0;
        }
    }
    m_nLights = 0;
    m_maxZone = 0;
    m_nZoneLights = 0;
    m_curZone = 0xffff;
}

void EXLightManager::UpdateLightInMapZoneTable(const EXGeoMap *pMap)
{
    EXFixedArray<u32, 256> zones;
    for (int i = 0; i < m_nLights; i++) {
        EXDynLight *pLight = Light(i);
        if (!pLight->HasChanged()) continue;
        for (u32 z = 0; z <= m_maxZone; z++) m_zoneTable[z].Clr(i);
        if (pLight->IsActive()) {
            if (pLight->AffectsOneZone()) {
                int zone = ((const EXGeoBspTree *)pMap->m_BspTree)->WhichNode(pLight->Pos());
                if (zone >= 0) {
                    m_zoneTable[zone].Set(i);
                    if ((u32)zone > m_maxZone) m_maxZone = zone;
                }
            } else {
                u32 n = pMap->GetMapZoneList(pLight->Pos(), pLight->OuterRadius(), zones);
                for (u32 k = 0; k < n; k++) {
                    u32 zone = zones[k];
                    m_zoneTable[zone].Set(i);
                    if (zone > m_maxZone) m_maxZone = zone;
                }
            }
        }
        pLight->SetChangedFlag(0);
        m_curZone = 0xffff;
    }
}

void EXLightManager::SetupMapZoneLights(u32 zone)
{
    if (zone == m_curZone) return;
    m_curZone = zone;
    if (zone >= 256) return;
    ex::EXBit128 &bits = m_zoneTable[zone];
    m_nZoneLights = 0;
    if (!bits.IsAnySet()) return;
    for (int i = 0; i < m_nLights; i++) {
        if (bits.IsSet(i)) m_zoneLights[m_nZoneLights++] = i;
    }
}

void EXLightManager::SetRenderEnvLights(const EXBoundsBox &box)
{
    EXVector centre;
    float radius = box.CentreAndRadius(centre);
    EXMaths::mat_44 m = EXBaseDisplay::ObjectToWorld();
    centre.w = 1.0f;
    *(EXMaths::vec_xyzw *)&centre = m * centre;
    u32 count = 0;
    for (u32 i = 0; i < m_nZoneLights; i++) {
        u32 idx = m_zoneLights[i];
        EXDynLight *pLight = Light(idx);
        EXVector d = pLight->Pos() - centre;
        float r = pLight->OuterRadius() + radius;
        if (d.LengthXYZSquared() < r * r) EXBaseRenderEnv::SetDynLight(count++, pLight->Data());
        if (count == 4) break;
    }
    EXBaseRenderEnv::SetDynLightCount(count);
    EXBaseRenderEnv::EnableDynLightRGBMult(m_bRGBMult);
    if (m_bRGBMult) EXBaseRenderEnv::SetDynLightRGBMult(EXVector(m_rgbMult));
}

EXLightManager::~EXLightManager()
{
    RemoveAllDynLights();
}
