#ifndef EXPARTICLE_H
#define EXPARTICLE_H
#include "types.h"
#include "EXVector.h"
#include "RGBA.h"

// One live particle (EXParticle.obj), sizeof 0xb8 (array stride in EXBaseParticleSys).
// TODO layout: only the fields recovered so far are named.
class EXParticle {
public:
    void SetTime(float t) { m_time = t; }
    s8 IsActive() const
    {
        if (m_life != 0.0f) return 1;
        return 0;
    }
    void SetScale(const EXVector &v) { m_scale = v; }
    void SetDefaultValues()
    {
        m_time = 0.0f;
        m_08 = 0;
        m_4c.set_zero();
        m_7c.set_zero();
        m_8c.set_zero();
        m_9c.set_zero();
        m_6c.set_zero();
    }
    EXVector Pos() const { return m_pos; }
    EXVector Scale() const { return m_scale; }

    float m_life;               // +0x00 0 = free slot
    float m_time;               // +0x04
    u32 m_08;
    EXVector m_pos;             // +0x0c
    EXVector m_1c;              // +0x1c entity mode: rotation angles when local space and EXGeoParticle bit 0x46.0
    EXVector m_2c;              // +0x2c entity mode: previous position (m_pos - m_2c is the facing direction)
    EXVector m_scale;           // +0x3c
    EXVector m_4c;              // +0x4c rotation angles (radians, Z used alone when x = y = 0)
    EXVector m_5c;              // +0x5c entity mode colour: rgb / 128 and a / 64 give the render info multiplier
    EXVector m_6c;              // +0x6c
    EXVector m_7c;              // +0x7c
    EXVector m_8c;              // +0x8c
    EXVector m_9c;              // +0x9c
    ex::RGBA m_colour;          // +0xac quad colour (argb() goes to the vertex)
    u32 m_b0;
    u32 m_texture;              // +0xb4 sort key: texture index, or entity hash in entity mode
};
#endif
