#ifndef EXPFLUID_H
#define EXPFLUID_H
#include "EXItemPhysics.h"

class EXVector3;

// Height-field water surface driven by the item's entity (sizeof 0x78). Two height buffers swap
// every step; optional per-vertex normals are packed into the vertex buffer (11:11:10).
class EXPFluid : public EXItemPhysicsExt {
public:
    virtual s8 DoUpdate();
    virtual s8 Initialise();
    EX_DECLARE_DYNAMIC(EXPFluid)
public:
    virtual ~EXPFluid();
    s8 Hit(int x, int y, float depth, float spread) const;
    float &Cell(int x, int y) const { return m_pBuffer[m_current][y * m_width + x]; }
    EXPFluid();
    s8 Initialise(u32 w, u32 h, float d, float c, float mu, s8 bNormals);
    void SyncWithVerts() const;
    static EXPFluid *CreateObject();

    int m_width;            // +0x44
    int m_height;           // +0x48
    float *m_pBuffer[2];    // +0x4c
    u32 m_current;          // +0x54 index of the current buffer
    EXVector3 *m_pNormals;  // +0x58
    float m_k1;             // +0x5c
    float m_k2;             // +0x60
    float m_k3;             // +0x64
    float m_time;           // +0x68 accumulated steps
    float m_speed;          // +0x6c
    float m_spacing;        // +0x70
    s8 m_bInitialised;      // +0x74
    s8 m_bNormals;          // +0x75
};
#endif
