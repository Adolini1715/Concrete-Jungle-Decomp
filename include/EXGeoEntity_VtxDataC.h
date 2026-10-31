#ifndef EXGEOENTITY_VTXDATAC_H
#define EXGEOENTITY_VTXDATAC_H
#include "types.h"
#include "EXVector.h"
#include "EXStdFuncs.h"

// Vertex with colour (sizeof 0x1c): the sprite shader's input (layout 3: position, packed normal at 12,
// colour at 16, uv at 20). TODO layout: field names from XBParticle.obj only; EXSwooshSys returns one.
struct EXGeoEntity_VtxDataC {
    __forceinline EXGeoEntity_VtxDataC() {}
    // Copies the position as an EXVector, then the whole vertex (XBSwoosh.obj copies).
    EXGeoEntity_VtxDataC(const EXGeoEntity_VtxDataC &v)
    {
        *(EXVector *)this = *(const EXVector *)&v;
        m_x = v.m_x;
        m_y = v.m_y;
        m_z = v.m_z;
        m_Normal = v.m_Normal;
        m_Colour = v.m_Colour;
        m_u = v.m_u;
        m_v = v.m_v;
    }
    EXGeoEntity_VtxDataC &operator=(const EXGeoEntity_VtxDataC &v)
    {
        *(EXVector *)this = *(const EXVector *)&v;
        m_x = v.m_x;
        m_y = v.m_y;
        m_z = v.m_z;
        m_Normal = v.m_Normal;
        m_Colour = v.m_Colour;
        m_u = v.m_u;
        m_v = v.m_v;
        return *this;
    }
    float m_x, m_y, m_z;    // 0x00
    s32 m_Normal;           // 0x0c packed normal
    u32 m_Colour;           // 0x10 D3DCOLOR (argb)
    float m_u, m_v;         // 0x14
};
#endif
