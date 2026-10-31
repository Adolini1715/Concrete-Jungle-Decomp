#ifndef EXBASEPARTICLESYS_H
#define EXBASEPARTICLESYS_H
#include "EXEffectsSys.h"
#include "EXVector.h"
#include "RGBA.h"
#include "EXRand.h"
#include "EXGeoParticle.h"

class EXParticle;
class EXParticleSysMgr;
class EXGeoParticle;
class EXMatrix;
class EXEmitter;
class EXGDiDataArray;
class EXMapOn;

namespace ParticleMip { enum Type { NEAR_MIP = 0, FAR_MIP = 1 }; }
namespace ColourMode { enum Type { COLOURMODE_0 = 0 }; }     // TODO values
namespace Variation { enum Type { VARIATION_0 = 0 }; }       // TODO values

// Level-of-detail emitter settings (12 bytes).
struct CParticleMip {
    float GetEmitsPerFrame() const { return m_emitsPerFrame; }
    float GetDistance() const { return m_distance; }
    u32 GetParticleLimit() const { return m_particleLimit; }
    void SetEmitsPerFrame(float f) { m_emitsPerFrame = f; }
    void SetDistance(float f) { m_distance = f; }
    void SetParticleLimit(u32 n) { m_particleLimit = n; }
    float m_distance;
    float m_emitsPerFrame;
    u32 m_particleLimit;
};

typedef u32 (__cdecl *EXParticlePosCallback)(EXItemAnimator *pAnim, EXVector *pOut, u32 n);

// Platform-independent particle system (EXParticle.obj), sizeof 0x27c.
// TODO layout: only the fields recovered so far are named.
class EXBaseParticleSys : public EXEffectsSys {
    EX_DECLARE_DYNAMIC(EXBaseParticleSys)
public:
    virtual ~EXBaseParticleSys();
    virtual void UpdateRenderInfo(const EXRenderInfo &info);
    virtual s8 Initialize(EXGeoFile *pFile, u32 hash);
    virtual s8 Update(EXItemAnimator *pAnim);
    virtual void Draw(u32 pass);
    virtual EXParticle *CreateParticle(const EXVector &pos, const EXMatrix &mtx);
    virtual u32 UpdateParticles(const EXGeoParticle *pGeo);
    virtual EXParticle *CreateParticleWithDir(const EXVector &pos, const EXVector &dir, const EXMatrix &mtx);
    EXBaseParticleSys();
    static EXBaseParticleSys *CreateObject();
    void SetParticlePosCallback(EXParticlePosCallback pFunc) { m_pPosCallback = pFunc; }
    EXParticlePosCallback GetParticlePosCallback() const { return m_pPosCallback; }
    void Enable(s8 b) { m_bEnabled = b; }
    s8 IsEnabled() const { return m_bEnabled; }
    s8 IsVisible() const { return m_bVisible; }
    void Visible(s8 b) { m_bVisible = b; }
    void SetParticleSysMgr(EXParticleSysMgr *p);
    const EXGeoComplexRefObject &GeoRefEmitter() const { return m_geoRefEmitter; }
    void EndParticles(s8 b) { m_bEndParticles = b; }
    void SetLocalSpace(s8 b) { m_bLocalSpace = b; }
    s8 GetLocalSpace() const { return m_bLocalSpace; }
    void SetFps(float f) { m_fps = f; }
    float GetRFps() const { return m_rfps; }
    u16 SystemFlags() const { return m_systemFlags; }
    EXVector LastEmitPos() const { return m_lastEmitPos; }
    EXVector GetGlobalScale() const { return m_globalScale; }
    const CParticleMip &GetParticleMip(ParticleMip::Type t) const { return m_mips[t]; }
    void SetParticleMip(ParticleMip::Type t, const CParticleMip &m) { m_mips[t] = m; }
    ColourMode::Type GetColourMode() const { return m_colourMode; }
    Variation::Type GetColourVar() const { return m_colourVar; }
    Variation::Type GetTextureMode() const { return m_textureMode; }
    u32 GetHashCode() const;
    void SetParticleFade(const EXVector &fade);
    void Release();
    EXParticle *GetRandomParticle();
    EXParticle *GetParticle(u32 i);
    u32 GetParticles(u32 n, EXParticle **pOut);
    void CalculateMipSettings();
    EXParticle *GetFreeParticle();
    void BaseSetupEmitterColours(EXParticle *p, const EXGeoParticle *pGeo, float t) const;
    void BaseUpdateEmitterColours(EXParticle *p, const EXGeoParticle *pGeo, float t) const;
    float BaseUpdateEmitterLife(const EXGeoParticle *pGeo);
    __forceinline EXVector CalculateInitialSize() const
    {
        if (0x40 & m_systemFlags) {
            float r = ((1.0f / 1073741824.0f) * (float)(int)(EXRandFast32() >> 1) - 1.0f) * m_sizeVar.x;
            EXVector tmp51 = EXVector(m_sizeBase.x + r, m_sizeBase.y + r, m_sizeBase.z + r, 0.0f);
return tmp51;
        }
        EXVector r(((float)(int)(EXRandFast32() >> 1) * (1.0f / 1073741824.0f) - 1.0f),
                   ((float)(int)(EXRandFast32() >> 1) * (1.0f / 1073741824.0f) - 1.0f),
                   ((float)(int)(EXRandFast32() >> 1) * (1.0f / 1073741824.0f) - 1.0f), 0.0f);
        EXVector tmp4 = EXVector(m_sizeVar.x * r.x + m_sizeBase.x, m_sizeVar.y * r.x + m_sizeBase.y, m_sizeVar.z * r.x + m_sizeBase.z, 0.0f);
return tmp4;
    }
    __forceinline u32 CalculateInitialTexture()
    {
        const EXGeoParticle *g = m_pGeo;
        if (g->m_textures.Size() > 1) {
            if (m_textureMode == 0) {
                m_texStep = 1;
                if (m_texIndex > g->m_textures.Size() - 1) m_texIndex = 0;
            } else if (m_textureMode == 1) {
                if (m_texIndex == g->m_textures.Size() - 1) m_texStep = -1;
                else if (m_texIndex == 0) m_texStep = 1;
            } else if (m_textureMode == 2) {
                m_texStep = 0;
                int tmp69 = g->m_textures.Size();
m_texIndex = (int)((float)(int)(EXRandFast32() >> 1) * (float)tmp69 * (1.0f / 2147483648.0f));
            }
            u32 t = g->m_textures[m_texIndex];
            m_texIndex += m_texStep;
            return t;
        }
        return g->m_textures[0];
    }
    void BaseUpdateParticle(EXParticle *p) const;
    void BaseUpdateParticleWithFade(EXParticle *p);
    void CalculateEmitterPosition(const EXItemAnimator *pAnim);
    void CalculateFogSettings(EXMapOn mapOn);
    s8 CheckMapVisibility(const EXItemAnimator *pAnim);
    void BaseUpdateParticleLifetimeNoColour(EXParticle *p, const EXGeoParticle *pGeo) const;
    void BaseUpdateParticleLifetime(EXParticle *p, const EXGeoParticle *pGeo) const;
    void UpdateParticleLifetime(EXParticle *p, const EXGeoParticle *pGeo) const;
    void EmitParticles(const EXMatrix &mtx, u32 a, u32 b);
    s8 CheckVisibility();
    void CreateParticles(const EXItemAnimator *pAnim, const EXGeoParticle *pGeo);
    void CalculateInitialColour(EXParticle *p);
    void CalculateInitialRotation(EXParticle *p, const EXVector &dir) const;
    s8 Update(const EXItemAnimator *pAnim, const EXGeoParticle *pGeo);
    void CalculateStandardParticleSettings(EXParticle *p);
    EXParticle *BaseCreateParticle(const EXVector &pos, const EXMatrix &mtx);
protected:
    s8 CreateEXEmitter(const EXGeoParticle *pGeo);
public:

    const EXMaths::mat_44 *m_pViewToScreenCoords;   // +0x54
    const EXMaths::mat_44 *m_pWorldToView;          // +0x58
    EXParticlePosCallback m_pPosCallback;   // +0x5c
    EXGDiDataArray *m_pGDiArrays;           // +0x60 new[]
    u32 m_64;
    EXGeoComplexRefObject m_geoRefEmitter;  // +0x68
    u16 m_systemFlags;                      // +0x78 8 additive, 0x10 subtractive, 0x80 fogged, 0x1c06 particles rotate
    u8 m_7a[2];
    EXVector m_7c;                          // +0x7c emit offset (also the billboard matrix translation, unused by the draw)
    EXVector m_8c;                          // +0x8c +- random emit offset spread
    EXVector m_fade;                        // +0x9c
    EXVector m_sizeBase;                    // +0xac initial particle scale
    EXVector m_sizeVar;                     // +0xbc +- random scale variation (x alone when uniform, flag 0x40)
    EXVector m_force;                       // +0xcc per-frame particle acceleration (geo force * m_frameScale, in world space with geo flag 4)
    EXVector m_dc;                          // +0xdc velocity multiplier used by the no-colour emitter (EXEmitter_NRNC)
    EXVector m_damping;                     // +0xec per-frame velocity multiplier (geo damping, frame-rate corrected)
    EXVector m_lastEmitPos;                 // +0xfc
    EXVector m_10c;                         // +0x10c system rotation angles (entity particles in local space)
    EXVector m_emitterVel;                  // +0x11c added to new particles' velocity when m_bAddEmitterVel
    EXVector m_globalScale;                 // +0x12c
    EXVector m_fadedColour;                 // +0x13c scratch: particle colour times m_fade (UpdateParticleWithFade)
    int m_nextParticle;                     // +0x14c round-robin search start
    float m_fps;                            // +0x150
    float m_rfps;                           // +0x154
    u32 m_158;                              // +0x158 multiplies the camera angles of the quad billboard (0 = unrotated)
    float m_pitch;                          // +0x15c emit direction (radians from +y)
    float m_pitchVar;                       // +0x160
    float m_yaw;                            // +0x164
    float m_yawVar;                         // +0x168
    float m_speed;                          // +0x16c
    float m_speedVar;                       // +0x170
    float m_emitterLife;                    // +0x174 extra emit speed, keyed by type 0xa when m_systemFlags & 0x200
    float m_emitAccum;                      // +0x178 fractional emits carried between frames
    float m_emitterTime;                    // +0x17c time along the emitter keys
    u32 m_180;
    float m_emitterRate;                    // +0x184 emitter time step per frame (position-callback systems prime while rate * 4 > time)
    float m_gradientTime;                   // +0x188 colour gradient position 0..100 (CalculateGradientColourTime)
    float m_curEmitsPerFrame;               // +0x18c from the mips by view distance
    float m_frameScale;                     // +0x190
    int m_curParticleLimit;                 // +0x194
    int m_gradientDir;                      // +0x198 +1 / -1 gradient direction
    int m_texIndex;                         // +0x19c next texture (CalculateInitialTexture)
    int m_texStep;                          // +0x1a0 +1 / -1 / 0
    CParticleMip m_mips[2];                 // +0x1a4
    EXParticle *m_pParticles;               // +0x1bc
    int m_maxParticles;                     // +0x1c0
    int m_nextNew;                          // +0x1c4 next never-used slot
    int m_numActive;                        // +0x1c8
    u32 m_1cc;                              // +0x1cc nonzero: particles take the geo's first entity hash
    ColourMode::Type m_colourMode;          // +0x1d0
    Variation::Type m_colourVar;            // +0x1d4
    Variation::Type m_textureMode;          // +0x1d8
    u32 m_emitterType;                      // +0x1dc
    EXEmitter *m_pEmitter;                  // +0x1e0
    s8 m_bEnabled;                          // +0x1e4
    s8 m_bVisible;                          // +0x1e5
    s8 m_bUseEmitter;                       // +0x1e6
    s8 m_bEndParticles;                     // +0x1e7
    s8 m_bFade;                             // +0x1e8
    s8 m_bLocalSpace;                       // +0x1e9
    s8 m_bAddEmitterVel;                    // +0x1ea
    s8 m_bNoCreate;                         // +0x1eb
    s8 m_bEmitWithDir;                      // +0x1ec emit from entity vertex pairs (position, direction)
    s8 m_bUseMips;                          // +0x1ed TODO name
    s8 m_bAllUsed;                          // +0x1ee every slot handed out once
    u8 m_1ef;                               // +0x1ef
    s8 m_bFirstUpdate;                      // +0x1f0 run the geo's pre-updates on the next manager update
    u8 m_1f1[3];
    const EXGeoParticle *m_pGeo;            // +0x1f4
    EXMaths::mat_44 m_1f8;                  // +0x1f8 local space matrix (positions when m_bLocalSpace == 1)
    u32 m_geoHash;                          // +0x238
    float m_SortBias;                       // +0x23c added to the view depth in the window's alpha sort
    int m_Layer;                            // +0x240 alpha layer (EXWnd::SortAndRenderEnitityArray)
    float m_fogNear;                        // +0x244 fog block copied from the map zone (CalculateFogSettings)
    float m_fogFar;                         // +0x248
    float m_fogStart;                       // +0x24c
    float m_fogEnd;                         // +0x250
    ex::RGBA m_fogColour;                   // +0x254
    s8 m_258;                               // +0x258 fog on
    u8 m_259[3];
    int m_values[8];                        // +0x25c EXGeoParticle::Value(0..7)
};

#include "EXParticleSysMgr.h"
inline void EXBaseParticleSys::SetParticleSysMgr(EXParticleSysMgr *p) { m_pMgr = p; }
#endif
