#ifndef EXGEORENDERENV_H
#define EXGEORENDERENV_H
#include "types.h"
#include "RGBA.h"

// Fog settings of a map zone (sizeof 0x18); accessors emitted in EXItemEnv.obj.
struct EXGeoRenderEnv {
    float FogNear() const { return m_FogNear; }
    float FogFar() const { return m_FogFar; }
    float FogMin() const { return m_FogMin; }
    float FogMax() const { return m_FogMax; }
    ex::RGBA FogColor() const { return m_FogColor; }
    s8 FogEnabled() const { return m_FogEnabled; }
    float m_FogNear;        // +0x00
    float m_FogFar;         // +0x04
    float m_FogMin;         // +0x08
    float m_FogMax;         // +0x0c
    ex::RGBA m_FogColor;    // +0x10
    s8 m_FogEnabled;        // +0x14
    u8 m_15[3];
};
#endif
