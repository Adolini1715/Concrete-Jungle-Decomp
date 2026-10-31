#ifndef EXCACHEDGEOENTITY_H
#define EXCACHEDGEOENTITY_H
#include "types.h"
#include "EXRenderInfo.h"
#include "EXRenderFogInfo.h"
#include "EXMaths.h"
#include "EXVector.h"

class EXGeoHeader;
class EXGDiData;
class EXGDiSkinData;
class EXGeoEntity;
struct EXDynLightData;

// One entity queued for drawing: EXGeoEntity::Draw snapshots the transforms and render environment
// into one of these, EXWnd keeps arrays of them (stride 0x1f8) and EXGeoEntity::DrawDirectX draws it.
class EXCachedGeoEntity {
public:
    EXGeoHeader *m_pHeader;             // 0x00
    EXGeoEntity *m_pEntity;             // 0x04
    EXGDiData *m_pGDiData;              // 0x08
    EXMaths::mat_44 m_ObjectToWorld;    // 0x0c (ms_mtxArray[4]; lights, environment map, camera vector)
    EXMaths::mat_44 m_WorldToView;      // 0x4c (ms_mtxArray[3]; environment map)
    EXMaths::mat_44 m_ObjectToClip;     // 0x8c vertex shader c0..c3: object to clip, or to the shadow map when casting
    EXMaths::mat_44 m_ObjectToScreen;   // 0xcc (ms_mtxArray[11]): w after the transform is the view depth the window sorts on
    EXMaths::mat_44 m_10c;              // 0x10c TODO not written by the Xbox entity code
    EXRenderFogInfo m_Fog;              // 0x14c vertex shader fog
    float m_FadeOffset;                 // 0x164 distance fade start (c75.z); -1000 * scale when off
    float m_FadeScale;                  // 0x168 1 / fade length (c75.w); 1 when off
    s8 m_DistanceFade;                  // 0x16c distance fade on (forces the alpha path)
    u8 m_FadeLayerMask;                 // 0x16d strips whose layer bit is set ignore the fade
    s8 m_ShadowCaster;                  // 0x16e drawn into the shadow or light map (untextured, never glows)
    s8 m_ShadowReceiver;                // 0x16f inside the shadow cull volume: sample the shadow map
    s8 m_ZReadDisabled;                 // 0x170 Z func ALWAYS
    s8 m_ZWriteDisabled;                // 0x171
    float m_Diffuse[4];                 // 0x174 vertex shader c11 (RGB mult, alpha mult)
    float m_Ambient[4];                 // 0x184 vertex shader c12
    EXRenderInfo m_RenderInfo;          // 0x194 (tex swap info at 0x1a8)
    s8 m_ReflectionRender;              // 0x1c4 drawn mirrored: cull mode flipped to CW
    u32 m_1c8;                          // 0x1c8 TODO
    EXGDiSkinData *m_pSkinData;         // 0x1cc skinning state of the item (shared by its pieces; the shadow pass groups on it)
    u32 m_DynLightCount;                // 0x1d0
    EXDynLightData *m_DynLights[4];     // 0x1d4 dynamic lights touching the bounds (EXBaseRenderEnv::SetDynLights)
    EXMaths::vec_xyzw m_DynLightRGBMult;  // 0x1e4
    s8 m_DynLightRGBMultEnabled;        // 0x1f4
    // Matrix the shadow pass sorts on (w is the depth from the shadow camera).
    const EXMaths::mat_44 &ShadowTransform() const { return m_ObjectToClip; }
    // Object to screen: w after the transform is the view depth the window sorts on.
    const EXMaths::mat_44 &Transform() const { return m_ObjectToScreen; }
};
#endif
