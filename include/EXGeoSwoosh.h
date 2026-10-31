#ifndef EXGEOSWOOSH_H
#define EXGEOSWOOSH_H
#include "types.h"
#include "EXVector.h"
#include "EXRelPtr.h"

// Swoosh (trail) definition in a geo file. Plain data; accessor offsets from the EXSwoosh.obj,
// XBSwoosh.obj and EXGeoViewer.obj copies.
class EXGeoSwoosh {
public:
    u32 TypeHashRef() const { return m_typeHashRef; }
    float Life() const { return m_life; }
    float HorizontalTiling() const { return m_hTiling; }
    float VerticalTiling() const { return m_vTiling; }
    int MaxSegments() const { return m_maxSegments; }
    int BlendMode() const { return m_blendMode; }
    int UseTaperController() const { return m_useTaper; }
    float StartYScale() const { return m_startYScale; }
    float EndYScale() const { return m_endYScale; }
    float VanishingScale() const { return m_vanishingScale; }
    int UseAlphaController() const { return m_useAlpha; }
    float StartAlphaScale() const { return m_startAlpha; }
    float EndAlphaScale() const { return m_endAlpha; }
    int UseColourController() const { return m_useColour; }
    float StartRScale() const { return m_startR; }
    float StartGScale() const { return m_startG; }
    float StartBScale() const { return m_startB; }
    float EndRScale() const { return m_endR; }
    float EndGScale() const { return m_endG; }
    float EndBScale() const { return m_endB; }
    u32 NumHashs() const { return m_numHashs; }
    u32 HashEntry(u32 i) const { return ((const u32 *)m_hashes)[i]; }
    const EXVector &VertexColour(u32 i) const { return ((const EXVector *)m_colours)[i]; }
    u32 TextureHash() const { return m_textureHash; }

    u32 m_typeHashRef;          // +0x00
    u32 m_04;
    float m_life;               // +0x08
    float m_hTiling;            // +0x0c
    float m_vTiling;            // +0x10
    int m_maxSegments;          // +0x14
    int m_blendMode;            // +0x18
    int m_useTaper;             // +0x1c
    float m_startYScale;        // +0x20
    float m_endYScale;          // +0x24
    float m_vanishingScale;     // +0x28
    int m_useAlpha;             // +0x2c
    float m_startAlpha;         // +0x30
    float m_endAlpha;           // +0x34
    int m_useColour;            // +0x38
    float m_startR, m_startG, m_startB;     // +0x3c
    float m_endR, m_endG, m_endB;           // +0x48
    u32 m_numHashs;             // +0x54
    ex::EXRelPtr<u32> m_hashes;             // +0x58
    ex::EXRelPtr<EXVector> m_colours;       // +0x5c
    u32 m_textureHash;          // +0x60
};
#endif
