#ifndef EXGEOPARTICLE_H
#define EXGEOPARTICLE_H
#include "EXGeoCommonObject.h"
#include "EXBounds.h"

// Emitter key (0x10): the value of channel m_type is m_scale * t + m_base from m_time on. The list
// ends with m_type -1. Channels 6..9 emitter colour r, g, b, a; 0xa emitter life; 0xb particle life.
struct EXGeoParticleKey {
    int m_type;
    float m_time;
    float m_base;
    float m_scale;
};

// Counted u32 list through a self-relative offset (same shape as EXRelArray<u32>; kept local so
// units with their own ex::EXRelPtr stand-in can include this header).
struct EXGeoParticleList {
    int Size() const { return m_Size; }
    const u32 *Data() const
    {
        if (!m_Offset) return 0;
        return (const u32 *)((const char *)&m_Offset + m_Offset);
    }
    u32 operator[](int i) const { return Data()[i]; }
    int m_Size;
    s32 m_Offset;
};

// Particle emitter definition (sizeof 0x118).
class EXGeoParticle : public EXGeoCommonObject {
    EX_DECLARE_DYNAMIC(EXGeoParticle)
    EX_DECLARE_GEOOBJECT(EXGeoParticle)
public:
    virtual EXBoundsBox &GetBoundsBox();
    EXGeoParticle() {}
    const EXBoundsBox &BoundsBox() const { return m_BoundsBox; }
    const EXGeoParticleKey *Key(u32 i) const { return &m_keys[i]; }
    int Value(unsigned int i) const
    {
        int o = m_xtra >> 8;
        const int *p = o ? (const int *)((const char *)&m_xtra + o) : 0;
        unsigned int mask = m_xtra & 0xff;
        if (!(mask & (1 << i))) return 0;
        int n = 0;
        for (int j = 0; j < (int)i; j++)
            if (mask & (1 << j)) n++;
        return p[n];
    }
    unsigned int GetTextureHash(unsigned int i) const { return m_textures.Data()[i]; }

    u32 m_04;                   // 0x04 nonzero for a usable emitter (EXBaseParticleSys::Initialize)
    u32 m_EmitterHash;          // 0x08 entity the particles are emitted from (0: none)
    u32 m_EmitterType;          // 0x0c 1, 3, 4: emitter shapes driven by EXEmitter
    EXBoundsBox m_BoundsBox;    // 0x10
    u16 m_sysFlags;             // 0x30 EXBaseParticleSys::m_systemFlags
    u16 m_sysFlags2;            // 0x32
    float m_preUpdates;         // 0x34 updates run on the first frame (EXParticleSysMgr::Update)
    u8 m_38[4];
    float m_rfps;               // 0x3c frame time scale (EXBaseParticleSys m_rfps; damping exponent)
    float m_sortBias;           // 0x40
    u8 m_layer;                 // 0x44
    u8 m_45;
    u8 m_flags46;               // 0x46 bit 0: entity particles take their rotation from m_1c / their motion
    u8 m_47;
    EXVector3 m_emitOffset;     // 0x48 particle start offset
    EXVector3 m_emitSpread;     // 0x54 +- random start offset spread
    EXVector3 m_sizeBase;       // 0x60
    EXVector3 m_scaleVar;       // 0x6c random scale variation (+-) per axis, unless system flag 0x40
    EXVector3 m_force;          // 0x78 particle acceleration per frame
    EXVector3 m_damping;        // 0x84 velocity multiplier per frame at the nominal rate
    EXVector3 m_dampingBase;    // 0x90 raised to m_rfps * frame scale otherwise (0: unchanged)
    float m_pitch;              // 0x9c
    float m_pitchVar;           // 0xa0
    float m_yaw;                // 0xa4
    float m_yawVar;             // 0xa8
    float m_speed;              // 0xac
    float m_speedVar;           // 0xb0
    float m_life;               // 0xb4 particle life (time units for 100 key time)
    float m_lifeVar;            // 0xb8 +- random life variation
    float m_mip0Distance;       // 0xbc
    float m_mip0Emits;          // 0xc0
    u32 m_maxParticles;         // 0xc4
    float m_mip1Distance;       // 0xc8
    float m_mip1Emits;          // 0xcc
    u32 m_mip1Limit;            // 0xd0
    u8 m_useMips;               // 0xd4
    u8 m_d5[3];
    u32 m_textureMode;          // 0xd8
    u32 m_colourMode;           // 0xdc
    u32 m_colourVar;            // 0xe0
    u32 m_e4;                   // 0xe4 EXBaseParticleSys::m_158
    u32 m_e8;
    u8 m_emitWithDir;           // 0xec
    u8 m_ed;
    u8 m_localSpace;            // 0xee
    u8 m_ef;
    s32 m_xtra;                 // 0xf0 8-bit value mask, 24-bit relptr to the values (Value)
    u32 m_type;                 // 0xf4 particle system type id (EXParticleSysMgr::CreateSystem)
    EXGeoParticleList m_textures; // 0xf8 texture hashes (CalculateInitialTexture)
    EXGeoParticleList m_entities; // 0x100 entity hashes (entity particles take the first)
    EXGeoParticleKey m_keys[1]; // 0x108 open-ended

};
#endif
