#ifndef EXRENDERINFO_H
#define EXRENDERINFO_H
#include "types.h"
#include "EXVector.h"
#include "EXFlag.h"
#include "EXStdFuncs.h"

class EXExtRenderInfo;
class EXTexSwapInfo;

// Per-item render parameters (sizeof 0x30, embedded in EXPlacementItem at +0x4c).
// Render flag bits: 4 RGB mult, 0x40 has ext render info, 0x200 shadow density set.
class EXRenderInfo {
public:
    // m_renderFlags is an ex::EXFlag<u16>: its constructor's store survives (EXItem.obj copy).
    EXRenderInfo() { SetDefault(); }
    EXRenderInfo(const EXRenderInfo &a, const EXRenderInfo &b) { Merge(*this, a, b); }
    // Copied as a block (rep movsd in the XBGeoEntity.obj copies), not member by member.
    EXRenderInfo(const EXRenderInfo &o) { memcpy(this, &o, sizeof(EXRenderInfo)); }
    EXRenderInfo &operator=(const EXRenderInfo &o)
    {
        memcpy(this, &o, sizeof(EXRenderInfo));
        return *this;
    }
    void SetDefault()
    {
        m_rgbaMult.set(1.0f);
        m_pExtRenderInfo = 0;
        m_pTexSwapInfo = 0;
        m_renderFlags = 0;
        m_1a = 0;
        m_shadowType = 0;
        m_shadowDensity = 0x7fff;
        m_lightSet1 = 0;
        m_lightSet2 = 0;
        m_lightSetBlend = 0;
        m_vertexSet1 = 0;
        m_vertexSet2 = 0;
        m_vertexSetBlend = 0;
    }
    static const EXRenderInfo &Merge(EXRenderInfo &out, const EXRenderInfo &a, const EXRenderInfo &b);
    const EXVector &RGBAMult() const { return m_rgbaMult; }
    EXExtRenderInfo *ExtRenderInfo() const { return m_pExtRenderInfo; }
    EXTexSwapInfo *TexSwapInfo() const { return m_pTexSwapInfo; }
    u16 RenderFlags() const { return m_renderFlags.m_flags; }
    ex::EXFlag<u16> &RenderFlagsRef() { return m_renderFlags; }
    struct Bits {
        u8 m_bit0 : 1;              // 0x1
        u8 m_alphaMult : 1;         // 0x2
        u8 m_rgbMult : 1;           // 0x4
        u8 m_bit3 : 1;              // 0x8
        u8 m_zWriteDisabled : 1;    // 0x10
        u8 m_zReadDisabled : 1;     // 0x20
        u8 m_bit6 : 2;
        u8 m_lodDisabled : 1;       // 0x100 (EXBaseRenderEnv::LodDisabled)
        u8 m_bit9 : 7;
    };
    const Bits &FlagBits() const { return *(const Bits *)&m_renderFlags; }
    // Shifts, not the bitfield: the inlined tests shift the loaded flags (EXParticle.obj, XBGeoEntity.obj).
    s8 AlphaMultEnabled() const { return (s8)((RenderFlags() >> 1) & 1) != 0; }
    s8 RGBMultEnabled() const { return (s8)((RenderFlags() >> 2) & 1) != 0; }
    u8 ShadowType() const { return m_shadowType; }
    u16 LightSet1() const { return m_lightSet1; }
    u16 LightSet2() const { return m_lightSet2; }
    u16 VertexSet1() const { return m_vertexSet1; }
    u16 VertexSet2() const { return m_vertexSet2; }
    // Emitted in EXItemAnimator_ScriptControllers.obj.
    void SetLightSet(short a, short b, float f)
    {
        m_lightSet1 = a;
        m_lightSet2 = b;
        m_lightSetBlend = (u8)(f * 256.0f);
    }
    // Emitted in EXGeoMap.obj.
    void SetLightSet(float f)
    {
        short a = (short)f;
        m_lightSet1 = a;
        m_lightSet2 = m_lightSet1 + 1;
        m_lightSetBlend = (u8)((f - a) * 256.0f);
    }
    void SetVertexSet(short a, short b, float f)
    {
        m_vertexSet1 = a;
        m_vertexSet2 = b;
        m_vertexSetBlend = (u8)(f * 256.0f);
    }

    // Appended by the front-end agent (shapes from Actor.obj copies; used in FEScript.obj).
    void EnableRGBMult(s8 b) { m_renderFlags.BitState(4, b); }
    void EnableAlphaMult(s8 b) { m_renderFlags.BitState(2, b); }
    void EnableRefractionRender(s8 b) { m_renderFlags.BitState(0x2000, b); }
    struct Bits16 {
        u16 m_lo : 10;
        u16 m_shadowDrawDisabled : 1;   // 0x400
        u16 m_bit11 : 2;
        u16 m_refractionRender : 1;     // 0x2000
        u16 m_heatRender : 1;           // 0x4000
        u16 m_hi : 1;
    };
    s8 RefractionRender() const { return ((const Bits16 *)&m_renderFlags)->m_refractionRender; }  // [fi] in CameraScript.obj
    void SetExtRenderInfo(EXExtRenderInfo *p)
    {
        m_pExtRenderInfo = p;
        m_renderFlags.BitState(0x40, p);
    }
    void SetTexSwapInfo(EXTexSwapInfo *p)
    {
        m_pTexSwapInfo = p;
        m_renderFlags.BitState(0x80, p);
    }
    // The same setters forced inline (Actor.obj CreateLostLimb inlines them).
    __forceinline void SetExtRenderInfoInl(EXExtRenderInfo *p)
    {
        m_pExtRenderInfo = p;
        m_renderFlags.BitState(0x40, p);
    }
    __forceinline void SetTexSwapInfoInl(EXTexSwapInfo *p)
    {
        m_pTexSwapInfo = p;
        m_renderFlags.BitState(0x80, p);
    }
    // Appended by the XBGeoEntity agent (copies in XBGeoEntity.obj). The extended info holds three
    // light directions (+0), their colours (+0x30), the ambient colour (+0x60) and the refraction
    // strength (+0x74).
    s8 HeatRender() const { return ((const Bits16 *)&m_renderFlags)->m_heatRender; }
    s8 ShadowDrawDisabled() const { return ((const Bits16 *)&m_renderFlags)->m_shadowDrawDisabled; }
    s8 ZWriteDisabled() const { return (s8)((RenderFlags() >> 4) & 1) != 0; }
    s8 ZReadDisabled() const { return (s8)((RenderFlags() >> 5) & 1) != 0; }
    void DisableZWrite(s8 b) { m_renderFlags.BitState(0x10, b); }
    void DisableZRead(s8 b) { m_renderFlags.BitState(0x20, b); }
    void EnableHeatRender(s8 b) { m_renderFlags.BitState(0x4000, b); }  // ActorTextureSwap.obj copy
    float LightSetBlend() const { return m_lightSetBlend * (1.0f / 256.0f); }
    float VertexSetBlend() const { return m_vertexSetBlend * (1.0f / 256.0f); }
    const EXVector &LightVector(unsigned int i) const { return ((const EXVector *)m_pExtRenderInfo)[i]; }
    const EXVector &LightColor(unsigned int i) const { return ((const EXVector *)m_pExtRenderInfo)[i + 3]; }
    const EXVector &AmbientColor() const { return ((const EXVector *)m_pExtRenderInfo)[6]; }
    float RefractionStrength() const { return ((const float *)m_pExtRenderInfo)[0x74 / 4]; }
    // Extended info flag 4: refract the blurred copy of the back buffer.
    s8 RefractionBlur() const { return (s8)(((const u32 *)m_pExtRenderInfo)[0x78 / 4] & 4); }
    void SetRGBAMult(const EXVector &v) { m_rgbaMult = v; }  // Bullet.obj copy; inlined in XBParticle.obj
    void SetRGBAMult(float r, float g, float b, float a)
    {
        m_rgbaMult.x = r;
        m_rgbaMult.y = g;
        m_rgbaMult.z = b;
        m_rgbaMult.w = a;
    }

    EXVector m_rgbaMult;                // +0
    EXExtRenderInfo *m_pExtRenderInfo;  // +0x10
    EXTexSwapInfo *m_pTexSwapInfo;      // +0x14
    ex::EXFlag<u16> m_renderFlags;     // +0x18 render flags (bits: see Bits)
    u8 m_1a;                            // +0x1a TODO name
    u8 m_shadowType;                    // +0x1b
    u16 m_shadowDensity;                // +0x1c (0..0x7fff)
    u16 m_1e;                           // +0x1e TODO
    u16 m_lightSet1;                    // +0x20
    u16 m_lightSet2;                    // +0x22
    u16 m_vertexSet1;                   // +0x24
    u16 m_vertexSet2;                   // +0x26
    u8 m_lightSetBlend;                 // +0x28 (/256)
    u8 m_vertexSetBlend;                // +0x29 (/256)
    u16 m_2a;                           // +0x2a TODO
    u32 m_2c;                           // +0x2c TODO (sizeof 0x30: DoRender stack temps)
};
#endif
